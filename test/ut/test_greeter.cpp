#include <hello_prototype/Peer.hpp>
#include <hello_prototype/Crypto.hpp>
#include <hello_prototype/Database.hpp>
#include <gtest/gtest.h>
#include <cstdio>   // std::remove

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


