#include "TestUtil.h"

#include <atomic>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace network;
using namespace network::testutil;
using namespace std::chrono_literals;

using RecvOutcome = std::expected<int, int>;

namespace {

// recv() on another thread. The buffer lives inside the thread, so the test never shares stack memory with it.
AsyncResult<RecvOutcome> recvAsync(std::shared_ptr<ConnectionSocket> socket) {
    return AsyncResult<RecvOutcome>([socket = std::move(socket)] {
        char buffer[64];
        return socket->recv({buffer, sizeof(buffer)});
    });
}

// Sends 64 KiB chunks until one fails and returns that error. If the peer isn't reading, this blocks inside
// send() once the socket buffers are full, which is exactly the state close() has to be able to interrupt.
AsyncResult<int> sendUntilErrorAsync(std::shared_ptr<ConnectionSocket> socket) {
    return AsyncResult<int>([socket = std::move(socket)] {
        const std::string chunk(64 * 1024, 'x');
        while (true) {
            auto result = socket->send({chunk.data(), chunk.size()});
            if (!result.has_value()) {
                return result.error();
            }
        }
    });
}

} // namespace

class ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto pair = makeConnectedPair();
        ASSERT_TRUE(pair.has_value()) << "could not establish a loopback connection";
        client_ = std::move(pair->client);
        server_ = std::move(pair->server);
    }

    std::shared_ptr<ConnectionSocket> client_;
    std::shared_ptr<ConnectionSocket> server_;
};

// ---------------------------------------------------------------------------------------------
// close() from another thread must interrupt blocked I/O
// ---------------------------------------------------------------------------------------------

TEST_F(ConnectionTest, CloseWakesBlockedRecv) {
    auto reader = recvAsync(server_);

    std::this_thread::sleep_for(150ms);
    ASSERT_FALSE(reader.ready()) << "recv returned although nothing was sent";

    server_->close();

    auto result = reader.waitFor();
    ASSERT_TRUE(result.has_value()) << "recv stayed blocked after close() from another thread";
    EXPECT_FALSE(result->has_value()) << "recv reported data on a connection nobody wrote to";
}

TEST_F(ConnectionTest, CloseWakesBlockedExactSizeRecv) {
    AsyncResult<std::expected<void, int>> reader([server = server_] {
        char buffer[16];
        return server->recv({buffer, sizeof(buffer)}, sizeof(buffer));
    });

    std::this_thread::sleep_for(150ms);
    ASSERT_FALSE(reader.ready());

    server_->close();

    auto result = reader.waitFor();
    ASSERT_TRUE(result.has_value()) << "exact-size recv stayed blocked after close() from another thread";
    EXPECT_FALSE(result->has_value());
}

TEST_F(ConnectionTest, CloseWakesBlockedSend) {
    // The peer never reads, so the writer fills the socket buffers and then blocks inside send().
    auto writer = sendUntilErrorAsync(server_);

    std::this_thread::sleep_for(500ms);
    ASSERT_FALSE(writer.ready()) << "send never blocked; the test cannot exercise a blocked send";

    server_->close();

    auto result = writer.waitFor();
    ASSERT_TRUE(result.has_value()) << "send stayed blocked after close() from another thread";
    EXPECT_NE(*result, 0);
}

TEST_F(ConnectionTest, ClosePropagatesToPeerAsEndOfStream) {
    server_->close();

    auto result = recvAsync(client_).waitFor();
    ASSERT_TRUE(result.has_value()) << "peer never noticed the close";
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), 0) << "peer should see a clean end of stream";
}

// ---------------------------------------------------------------------------------------------
// close() semantics
// ---------------------------------------------------------------------------------------------

TEST_F(ConnectionTest, CloseIsIdempotent) {
    server_->close();
    server_->close();
    server_->close();

    auto result = recvAsync(client_).waitFor();
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value());
}

TEST_F(ConnectionTest, ConcurrentCloseFromManyThreadsIsSafe) {
    auto reader = recvAsync(server_);
    std::this_thread::sleep_for(100ms);

    std::vector<std::thread> closers;
    for (int i = 0; i < 4; ++i) {
        closers.emplace_back([server = server_] { server->close(); });
    }
    for (auto& t : closers) {
        t.join();
    }

    auto result = reader.waitFor();
    ASSERT_TRUE(result.has_value()) << "recv stayed blocked after concurrent close()";
    EXPECT_FALSE(result->has_value());
}

TEST_F(ConnectionTest, SendAfterCloseFails) {
    server_->close();
    EXPECT_FALSE(sendString(*server_, "x").has_value());
}

TEST_F(ConnectionTest, RecvAfterCloseFailsInsteadOfBlocking) {
    server_->close();

    auto result = recvAsync(server_).waitFor(2s);
    ASSERT_TRUE(result.has_value()) << "recv on a closed socket blocked";
    EXPECT_FALSE(result->has_value());
}

TEST_F(ConnectionTest, CloseOnMovedFromSocketIsSafe) {
    ConnectionSocket moved = std::move(*server_);
    server_->close(); // moved-from: must be a no-op, not a null dereference
    moved.close();
}

// Regression for the descriptor-reuse race. close() must only signal (shutdown); the descriptor itself has
// to stay allocated until the object is destroyed. If close() releases it, the OS can hand the same number to
// the next socket, and a thread that still uses the old ConnectionSocket (it legitimately may: close() was
// called from another thread) would then read from or write into somebody else's connection.
//
// The OS gives out the lowest free descriptor, so after releasing one, opening enough new connections makes
// one of them reuse that number. That is a POSIX property; Windows socket handles are not reused this
// eagerly, so on Windows this is only a sanity check.
TEST(ConnectionLifetimeTest, UseAfterCloseNeverTouchesAnotherSocket) {
    const uint16_t port = nextPort();
    auto listener = listenOn(port);
    ASSERT_NE(listener, nullptr);

    auto closed = connectPair(listener, port);
    ASSERT_TRUE(closed.has_value());
    closed->client->close();

    // Many more connections than there can be free descriptors below the closed one.
    constexpr int kOthers = 48;
    std::vector<ConnectedPair> others;
    others.reserve(kOthers);

    // Always shut the other connections down, even on an early ASSERT, so the reader threads below end.
    struct CloseAll {
        std::vector<ConnectedPair>& pairs;
        ~CloseAll() {
            for (auto& pair : pairs) {
                pair.server->close();
                pair.client->close();
            }
        }
    } closeAll{others};

    for (int i = 0; i < kOthers; ++i) {
        auto pair = connectPair(listener, port);
        ASSERT_TRUE(pair.has_value()) << "connection " << i;
        others.push_back(std::move(*pair));
    }

    std::vector<AsyncResult<RecvOutcome>> readers;
    readers.reserve(kOthers);
    for (auto& other : others) {
        readers.push_back(recvAsync(other.server));
    }

    // Using the closed socket must fail without reaching any other connection.
    EXPECT_FALSE(sendString(*closed->client, "leak").has_value());
    auto staleRecv = recvAsync(closed->client).waitFor(2s);
    ASSERT_TRUE(staleRecv.has_value()) << "recv on a closed socket blocked: it is reading some other socket";

    std::this_thread::sleep_for(300ms);
    for (size_t i = 0; i < readers.size(); ++i) {
        EXPECT_FALSE(readers[i].ready()) << "connection " << i << " received data that was written to a closed socket";
    }
}

// ---------------------------------------------------------------------------------------------
// Concurrent reader + writer + close()
// ---------------------------------------------------------------------------------------------

// A session has one thread reading and one writing the same socket while another thread may close it at any
// moment. Every I/O call must come back, whenever close() lands. Run under ThreadSanitizer to also catch
// data races on the socket's internal state.
TEST(ConnectionStressTest, ReaderAndWriterRunWhileCloseFires) {
    std::mt19937 rng(4242);

    constexpr int kIterations = 20;
    for (int iter = 0; iter < kIterations; ++iter) {
        auto pair = makeConnectedPair();
        ASSERT_TRUE(pair.has_value()) << "iteration " << iter;
        auto server = pair->server;
        auto client = pair->client;

        // The server socket is used by a reader and a writer at the same time.
        AsyncResult<int> reader([server] {
            char buffer[256];
            while (true) {
                auto result = server->recv({buffer, sizeof(buffer)});
                if (!result.has_value()) {
                    return result.error();
                }
            }
        });
        // Nothing reads on the client side, so this fills the buffers and blocks in send().
        AsyncResult<int> writer = sendUntilErrorAsync(server);
        // Keeps the reader busy with real data until the connection goes away.
        AsyncResult<int> feeder([client] {
            while (true) {
                auto result = sendString(*client, "x");
                if (!result.has_value()) {
                    return result.error();
                }
                std::this_thread::sleep_for(100us);
            }
        });

        std::this_thread::sleep_for(std::chrono::microseconds(rng() % 3000));
        server->close();

        ASSERT_TRUE(reader.waitFor().has_value()) << "reader hung (iteration " << iter << ")";
        ASSERT_TRUE(writer.waitFor().has_value()) << "writer hung (iteration " << iter << ")";
        ASSERT_TRUE(feeder.waitFor().has_value()) << "feeder hung (iteration " << iter << ")";
    }
}
