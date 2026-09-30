#include <gtest/gtest.h>

#include <atomic>
#include <csignal>
#include <random>
#include <thread>
#include <vector>

#include "TestUtil.h"

using namespace network;
using namespace network::testutil;
using namespace std::chrono_literals;

using AcceptOutcome = std::expected<ConnectionSocket, int>;

class TcpTest : public ::testing::Test {
protected:
    void SetUp() override {
        port_ = nextPort();
        listener_ = listenOn(port_);
        ASSERT_NE(listener_, nullptr) << "failed to listen on port " << port_;
    }

    // Starts Accept() on another thread.
    AsyncResult<AcceptOutcome> acceptAsync(std::shared_ptr<AddrInfo> info = nullptr) const {
        return AsyncResult<AcceptOutcome>([listener = listener_, info = std::move(info)] {
            return ConnectionSocket::Accept(*listener, info.get());
        });
    }

    uint16_t port_ = 0;
    std::shared_ptr<ListeningSocket> listener_;
};

// ---------------------------------------------------------------------------------------------
// Connect / Accept / send / recv
// ---------------------------------------------------------------------------------------------

TEST_F(TcpTest, ConnectAcceptSendRecvRoundTrip) {
    auto info = std::make_shared<AddrInfo>();
    auto acceptor = acceptAsync(info);

    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value()) << "connect failed: " << client.error();

    auto accepted = acceptor.waitFor();
    ASSERT_TRUE(accepted.has_value()) << "Accept did not return";
    ASSERT_TRUE(accepted->has_value()) << "Accept failed: " << accepted->error();
    ConnectionSocket server = std::move(*accepted).value();

    EXPECT_EQ(info->address, kLoopback);
    EXPECT_NE(info->port, 0);

    ASSERT_TRUE(sendString(*client, "hello").has_value());
    auto atServer = recvExactly(server, 5);
    ASSERT_TRUE(atServer.has_value());
    EXPECT_EQ(*atServer, "hello");

    ASSERT_TRUE(sendString(server, "world").has_value());
    auto atClient = recvExactly(*client, 5);
    ASSERT_TRUE(atClient.has_value());
    EXPECT_EQ(*atClient, "world");
}

TEST_F(TcpTest, AcceptWithoutAddrInfoWorks) {
    auto acceptor = acceptAsync(nullptr);
    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value());

    auto accepted = acceptor.waitFor();
    ASSERT_TRUE(accepted.has_value());
    EXPECT_TRUE(accepted->has_value());
}

// Regression: the listening socket is non-blocking internally. On BSD/macOS (and Windows) accepted
// sockets inherit that, which would make recv() fail immediately instead of waiting for data.
TEST_F(TcpTest, AcceptedSocketBlocksUntilDataArrives) {
    auto acceptor = acceptAsync();
    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value());

    auto accepted = acceptor.waitFor();
    ASSERT_TRUE(accepted.has_value());
    ASSERT_TRUE(accepted->has_value());
    auto server = std::make_shared<ConnectionSocket>(std::move(*accepted).value());

    AsyncResult<std::expected<int, int>> receiver([server] {
        char buffer[8];
        return server->recv({buffer, sizeof(buffer)});
    });

    std::this_thread::sleep_for(200ms);
    EXPECT_FALSE(receiver.ready()) << "recv returned before any data was sent (socket is non-blocking?)";

    ASSERT_TRUE(sendString(*client, "x").has_value());
    auto received = receiver.waitFor();
    ASSERT_TRUE(received.has_value()) << "recv never returned after data was sent";
    ASSERT_TRUE(received->has_value()) << "recv failed: " << received->error();
    EXPECT_EQ(received->value(), 1);
}

// Regression: a failed Connect used to look like a success (POSIX returned a socket with fd -1).
TEST_F(TcpTest, ConnectToPortWithNoListenerFails) {
    auto result = ConnectionSocket::Connect(kLoopback, nextPort());
    EXPECT_FALSE(result.has_value());
}

TEST_F(TcpTest, ListenOnPortAlreadyInUseFails) {
    auto second = ListeningSocket::Listen(port_);
    EXPECT_FALSE(second.has_value());
}

TEST_F(TcpTest, PortCanBeReusedAfterListenerIsDestroyed) {
    listener_.reset();
    auto again = ListeningSocket::Listen(port_);
    EXPECT_TRUE(again.has_value());
}

TEST_F(TcpTest, RecvReportsPeerCloseAsErrorZero) {
    auto acceptor = acceptAsync();
    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value());

    auto accepted = acceptor.waitFor();
    ASSERT_TRUE(accepted.has_value());
    ASSERT_TRUE(accepted->has_value());
    ConnectionSocket server = std::move(*accepted).value();

    client->close();

    char buffer[8];
    auto result = server.recv({buffer, sizeof(buffer)});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), 0);
}

// Regression: on POSIX, sending to a peer that has gone away raised SIGPIPE and killed the process.
// If this regresses, the whole test binary dies instead of one test failing.
TEST_F(TcpTest, SendToClosedPeerReturnsErrorInsteadOfKillingProcess) {
#ifndef _WIN32
    // A parent process (shell, IDE, CI runner) may have set SIGPIPE to "ignore", and that is inherited.
    // Restore the default (fatal) action so this test can actually detect a regression.
    std::signal(SIGPIPE, SIG_DFL);
#endif

    auto acceptor = acceptAsync();
    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value());

    {
        auto accepted = acceptor.waitFor();
        ASSERT_TRUE(accepted.has_value());
        ASSERT_TRUE(accepted->has_value());
        ConnectionSocket server = std::move(*accepted).value();
    } // server side closed here

    // The first send after a FIN usually succeeds locally; a later one hits the reset.
    bool sawError = false;
    for (int i = 0; i < 200 && !sawError; ++i) {
        sawError = !sendString(*client, "x").has_value();
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_TRUE(sawError) << "sending to a closed peer never failed";
}

// ---------------------------------------------------------------------------------------------
// ListeningSocket::close() waking a blocked Accept()
// ---------------------------------------------------------------------------------------------

TEST_F(TcpTest, CloseWakesBlockedAccept) {
    auto acceptor = acceptAsync();

    std::this_thread::sleep_for(150ms);
    ASSERT_FALSE(acceptor.ready()) << "Accept returned with no client connecting";

    listener_->close();

    auto result = acceptor.waitFor();
    ASSERT_TRUE(result.has_value()) << "Accept stayed blocked after close()";
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), kAcceptAborted);
}

TEST_F(TcpTest, AcceptAfterCloseReturnsAbortedImmediately) {
    listener_->close();

    auto result = acceptAsync().waitFor(2s);
    ASSERT_TRUE(result.has_value()) << "Accept blocked on an already-closed listener";
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), kAcceptAborted);
}

TEST_F(TcpTest, AcceptKeepsReturningAbortedAfterClose) {
    listener_->close();
    for (int i = 0; i < 3; ++i) {
        auto result = acceptAsync().waitFor(2s);
        ASSERT_TRUE(result.has_value());
        ASSERT_FALSE(result->has_value());
        EXPECT_EQ(result->error(), kAcceptAborted);
    }
}

TEST_F(TcpTest, CloseIsIdempotent) {
    listener_->close();
    listener_->close();
    listener_->close();

    auto result = acceptAsync().waitFor(2s);
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), kAcceptAborted);
}

TEST_F(TcpTest, ConcurrentCloseFromManyThreadsIsSafe) {
    auto acceptor = acceptAsync();
    std::this_thread::sleep_for(100ms);

    std::vector<std::thread> closers;
    for (int i = 0; i < 4; ++i) {
        closers.emplace_back([listener = listener_] { listener->close(); });
    }
    for (auto& t : closers) {
        t.join();
    }

    auto result = acceptor.waitFor();
    ASSERT_TRUE(result.has_value()) << "Accept stayed blocked after concurrent close()";
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), kAcceptAborted);
}

TEST_F(TcpTest, ClosedListenerRefusesNewConnections) {
    listener_->close();
    // Give an Accept-side close a chance to happen; with nobody in Accept, the socket may legitimately
    // stay open until the listener is destroyed, so drive one Accept to observe the abort first.
    ASSERT_TRUE(acceptAsync().waitFor(2s).has_value());

    auto result = ConnectionSocket::Connect(kLoopback, port_);
    EXPECT_FALSE(result.has_value());
}

// Connections that are queued or arriving while close() runs must never hang the acceptor or
// crash. Whatever mix of accepted / refused / reset the clients see, the acceptor must end with
// kAcceptAborted. Each iteration uses a fresh port.
TEST(TcpStressTest, CloseWhileClientsAreConnecting) {
    std::mt19937 rng(12345);

    // Client threads from every iteration. On Windows a refused loopback connect can take 1-2 seconds to
    // fail, and clients are mid-Connect when the listener closes. The acceptor's result doesn't depend on
    // them, so they are only joined once, when this object goes out of scope, letting those slow
    // refusals overlap across iterations. The destructor also runs on an early ASSERT return: it stops
    // every client first, so a failing test can't leave threads spinning or hit std::terminate.
    struct ClientThreads {
        std::vector<std::shared_ptr<std::atomic<bool>>> stopFlags;
        std::vector<std::thread> threads;

        ~ClientThreads() {
            for (auto& stop : stopFlags) {
                *stop = true;
            }
            for (auto& thread : threads) {
                thread.join();
            }
        }
    } clients;

    constexpr int kIterations = 10;
    for (int iter = 0; iter < kIterations; ++iter) {
        const uint16_t port = nextPort();
        auto listener = listenOn(port);
        ASSERT_NE(listener, nullptr) << "failed to listen on port " << port;

        AsyncResult<int> acceptor([listener] {
            while (true) {
                auto result = ConnectionSocket::Accept(*listener, nullptr);
                if (!result.has_value()) {
                    return result.error();
                }
                // Drop the accepted connection immediately.
            }
        });

        // Per-iteration flag held by shared_ptr: these threads can outlive the iteration.
        auto stop = std::make_shared<std::atomic<bool>>(false);
        clients.stopFlags.push_back(stop);
        for (int c = 0; c < 3; ++c) {
            clients.threads.emplace_back([port, stop] {
                while (!*stop) {
                    auto socket = ConnectionSocket::Connect(kLoopback, port);
                    (void)socket;
                }
            });
        }

        std::this_thread::sleep_for(std::chrono::microseconds(rng() % 3000));
        listener->close();
        *stop = true;

        auto result = acceptor.waitFor();
        ASSERT_TRUE(result.has_value()) << "acceptor hung (iteration " << iter << ")";
        EXPECT_EQ(*result, kAcceptAborted) << "iteration " << iter;
    }
}
