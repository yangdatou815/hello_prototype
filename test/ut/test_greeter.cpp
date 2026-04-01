#include <hello_prototype/Peer.hpp>
#include <hello_prototype/Crypto.hpp>
#include <hello_prototype/Database.hpp>
#include <hello_prototype/Client.hpp>
#include <hello_prototype/Server.hpp>
#include <gtest/gtest.h>
#include <cstdio>   // std::remove
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <atomic>
#include <sys/wait.h>

namespace {

bool wait_until(const std::function<bool()>& pred, std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return pred();
}

int run_command_get_exit(const std::string& cmd) {
    int status = std::system(cmd.c_str());
    if (status == -1) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 128;
}

} // namespace

// ─── Peer tests ───────────────────────────────────────────────
TEST(PeerTest, InitialStateIsOffline) {
    class ConcretePeer : public Peer {
    public:
        void goOnline()    { setState(PeerState::ONLINE);    }
        void goOffline()   { setState(PeerState::OFFLINE);   }
        void goDetected()  { setState(PeerState::DETECTED);  }
        void goConnected() { setState(PeerState::CONNECTED); }
    };

    ConcretePeer peer;
    EXPECT_EQ(peer.getState(),    PeerState::OFFLINE);
    EXPECT_EQ(peer.getStateStr(), "OFFLINE");
}

TEST(PeerTest, HandshakeStateProgression) {
    class ConcretePeer : public Peer {
    public:
        void set(PeerState s) { setState(s); }
    };
    ConcretePeer peer;
    peer.set(PeerState::DETECTED);
    EXPECT_EQ(peer.getStateStr(), "DETECTED");
    peer.set(PeerState::CONNECTED);
    EXPECT_EQ(peer.getStateStr(), "CONNECTED");
    peer.set(PeerState::ONLINE);
    EXPECT_EQ(peer.getStateStr(), "ONLINE");
}

// ─── Crypto tests ─────────────────────────────────────────────
TEST(CryptoTest, EncryptDecryptRoundTrip) {
    const std::string plain = "Hello, World!";
    std::string cipher = Crypto::encrypt(plain);
    EXPECT_NE(cipher, plain);
    EXPECT_EQ(Crypto::decrypt(cipher), plain);
}

TEST(CryptoTest, EncryptProducesBase64) {
    std::string cipher = Crypto::encrypt("test message");
    for (char c : cipher) {
        EXPECT_TRUE(isalnum(c) || c == '+' || c == '/' || c == '=')
            << "Non-Base64 char: " << c;
    }
}

TEST(CryptoTest, DifferentPlaintextsProduceDifferentCiphertexts) {
    EXPECT_NE(Crypto::encrypt("Alice"), Crypto::encrypt("Bob"));
}

TEST(CryptoTest, EmptyStringRoundTrip) {
    EXPECT_EQ(Crypto::decrypt(Crypto::encrypt("")), "");
}

TEST(CryptoTest, HandshakeMessagesRoundTrip) {
    for (const auto& msg : {"DETECTED", "CONNECTED", "ONLINE", "ACK: ONLINE"}) {
        EXPECT_EQ(Crypto::decrypt(Crypto::encrypt(msg)), msg);
    }
}

// ─── Database tests ───────────────────────────────────────────
class DatabaseTest : public ::testing::Test {
protected:
    const std::string db_path = "/tmp/ut_test.db";
    void SetUp() override   { std::remove(db_path.c_str()); }
    void TearDown() override { std::remove(db_path.c_str()); }
};

TEST_F(DatabaseTest, OpenAndCreateTables) {
    EXPECT_NO_THROW(Database db(db_path));
}

TEST_F(DatabaseTest, PeerStateDefaultOffline) {
    Database db(db_path);
    EXPECT_EQ(db.getPeerState("client"), PeerState::OFFLINE);
}

TEST_F(DatabaseTest, UpdateAndGetPeerState) {
    Database db(db_path);
    db.updatePeerState("client", PeerState::DETECTED);
    EXPECT_EQ(db.getPeerState("client"), PeerState::DETECTED);
    db.updatePeerState("client", PeerState::CONNECTED);
    EXPECT_EQ(db.getPeerState("client"), PeerState::CONNECTED);
    db.updatePeerState("client", PeerState::ONLINE);
    EXPECT_EQ(db.getPeerState("client"), PeerState::ONLINE);
}

TEST_F(DatabaseTest, InsertAndGetMessages) {
    Database db(db_path);
    db.insertMessage("client", "DETECTED");
    db.insertMessage("client", "ONLINE");
    auto msgs = db.getMessages();
    ASSERT_EQ(msgs.size(), 2u);
    EXPECT_EQ(msgs[0].sender,  "client");
    EXPECT_EQ(msgs[0].content, "DETECTED");
    EXPECT_EQ(msgs[1].content, "ONLINE");
}

TEST_F(DatabaseTest, MessageWithSpecialChars) {
    Database db(db_path);
    std::string tricky = "it's a test: O'Brien said \"hello\"";
    db.insertMessage("server", tricky);
    auto msgs = db.getMessages();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].content, tricky);
}

TEST(ClientTest, SendBeforeReadyDoesNotCrash) {
    asio::io_context io;
    Client client(io, "127.0.0.1", 65530);
    EXPECT_EQ(client.getState(), PeerState::OFFLINE);
    EXPECT_NO_THROW(client.send("hello"));
    EXPECT_EQ(client.getState(), PeerState::OFFLINE);
}

TEST(ClientTest, ConnectFailureKeepsOfflineState) {
    asio::io_context io;
    Client client(io, "127.0.0.1", 65531);

    client.connect();
    std::thread t([&]() { io.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    io.stop();
    t.join();

    EXPECT_EQ(client.getState(), PeerState::OFFLINE);
}

TEST(HandshakeIntegrationTest, ClientAndServerReachOnline) {
    std::remove("client.db");
    std::remove("server.db");

    constexpr unsigned short kPort = 19090;
    {
        asio::io_context server_io;
        asio::io_context client_io;

        Server server(server_io, kPort);
        server.start();

        Client client(client_io, "127.0.0.1", kPort);
        client.connect();

        std::thread ts([&]() { server_io.run(); });
        std::thread tc([&]() { client_io.run(); });

        // Give async handshake enough time to complete before stopping loops.
        std::this_thread::sleep_for(std::chrono::seconds(2));

        client_io.stop();
        server_io.stop();
        tc.join();
        ts.join();
    }

    Database client_db("client.db");
    Database server_db("server.db");
    EXPECT_EQ(client_db.getPeerState("server"), PeerState::ONLINE);
    EXPECT_EQ(server_db.getPeerState("client"), PeerState::ONLINE);
}

TEST(MainBehaviorTest, ThrowAndUsagePathsAreReachable) {
#ifndef HELLO_APP_PATH
    GTEST_SKIP() << "HELLO_APP_PATH not defined by CMake";
#else
    std::string app = HELLO_APP_PATH;
    if (!std::filesystem::exists(app)) {
        GTEST_SKIP() << "hello_app not found at: " << app;
    }

    int throw_exit = run_command_get_exit(app + " --throw > /tmp/ut_main_throw.log 2>&1");
    EXPECT_NE(throw_exit, 0);

    int usage_exit = run_command_get_exit(app + " > /tmp/ut_main_usage.log 2>&1");
    EXPECT_NE(usage_exit, 0);

    std::ifstream usage_log("/tmp/ut_main_usage.log");
    std::string content((std::istreambuf_iterator<char>(usage_log)),
                        std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("Usage: hello_app --role [server|client]"), std::string::npos);
#endif
}

TEST(SessionTest, DecryptErrorThenPeerCloseTriggersOnError) {
    asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 0));
    const unsigned short port = acceptor.local_endpoint().port();

    tcp::socket server_socket(io);
    std::thread accept_thread([&]() { acceptor.accept(server_socket); });

    tcp::socket client_socket(io);
    client_socket.connect(tcp::endpoint(asio::ip::address_v4::loopback(), port));
    accept_thread.join();

    std::atomic<int> on_error_calls{0};
    auto server_session = std::make_shared<Session>(
        std::move(server_socket),
        [](const std::string&) {},
        [&]() { on_error_calls.fetch_add(1); });
    server_session->start();

    std::thread io_thread([&]() { io.run(); });

    asio::write(client_socket, asio::buffer(std::string("@@@\n")));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client_socket.close();

    bool got_error = wait_until(
        [&]() { return on_error_calls.load() > 0; },
        std::chrono::seconds(2));

    io.stop();
    io_thread.join();
    EXPECT_TRUE(got_error);
}


