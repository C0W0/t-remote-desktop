#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <thread>

#include "TestUtil.h"
#include "tcp/Server.h"

using namespace network;
using namespace network::testutil;
using namespace std::chrono_literals;

class TcpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        port_ = nextPort();
        auto result = TcpServer::CreateServer(port_);
        ASSERT_TRUE(result.has_value()) << "CreateServer failed: " << result.error();
        server_ = std::make_shared<TcpServer>(std::move(result).value());
    }

    AsyncResult<std::expected<AddrInfo, int>> acceptAsync() const {
        return AsyncResult<std::expected<AddrInfo, int>>([server = server_] { return server->accept(); });
    }

    uint16_t port_ = 0;
    std::shared_ptr<TcpServer> server_;
};

TEST_F(TcpServerTest, AcceptInvokesHandlerWithTheConnection) {
    struct Captured {
        AddrInfo info;
        std::optional<ConnectionSocket> socket;
    };
    auto captured = std::make_shared<Captured>();

    server_->onAccept([captured](AddrInfo info, ConnectionSocket socket, TcpServer&) {
        captured->info = std::move(info);
        captured->socket.emplace(std::move(socket));
    });

    auto acceptor = acceptAsync();
    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value());

    auto result = acceptor.waitFor();
    ASSERT_TRUE(result.has_value()) << "accept() did not return";
    ASSERT_TRUE(result->has_value()) << "accept() failed: " << result->error();
    EXPECT_EQ(result->value().address, kLoopback);

    // The handler ran on the acceptor thread before accept() returned.
    ASSERT_TRUE(captured->socket.has_value());
    EXPECT_EQ(captured->info.address, kLoopback);

    ASSERT_TRUE(sendString(*client, "ping").has_value());
    auto received = recvExactly(*captured->socket, 4);
    ASSERT_TRUE(received.has_value());
    EXPECT_EQ(*received, "ping");
}

TEST_F(TcpServerTest, AcceptWithoutHandlerStillSucceeds) {
    auto acceptor = acceptAsync();
    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value());

    auto result = acceptor.waitFor();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->has_value());
}

TEST_F(TcpServerTest, AbortListeningWakesBlockedAccept) {
    auto acceptor = acceptAsync();
    std::this_thread::sleep_for(150ms);
    ASSERT_FALSE(acceptor.ready());

    server_->abortListening();

    auto result = acceptor.waitFor();
    ASSERT_TRUE(result.has_value()) << "accept() stayed blocked after abortListening()";
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), kAcceptAborted);
}

// The handler may call abortListening() itself (the host app does this from its connection thread).
TEST_F(TcpServerTest, HandlerCanAbortListening) {
    server_->onAccept([](AddrInfo, ConnectionSocket, TcpServer& server) { server.abortListening(); });

    auto first = acceptAsync();
    auto client = ConnectionSocket::Connect(kLoopback, port_);
    ASSERT_TRUE(client.has_value());
    auto accepted = first.waitFor();
    ASSERT_TRUE(accepted.has_value());
    EXPECT_TRUE(accepted->has_value());

    auto second = acceptAsync().waitFor(2s);
    ASSERT_TRUE(second.has_value()) << "accept() blocked after the handler aborted listening";
    ASSERT_FALSE(second->has_value());
    EXPECT_EQ(second->error(), kAcceptAborted);
}
