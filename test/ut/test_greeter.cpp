#include "Peer.hpp"
#include "Crypto.hpp"
#include <gtest/gtest.h>

// ─── Peer tests ───────────────────────────────────────────────
TEST(PeerTest, InitialStateIsOffline) {
    class ConcretePeer : public Peer {
    public:
        void goOnline()  { setState(PeerState::ONLINE);  }
        void goOffline() { setState(PeerState::OFFLINE); }
    };

    ConcretePeer peer;
    EXPECT_EQ(peer.getState(),    PeerState::OFFLINE);
    EXPECT_EQ(peer.getStateStr(), "OFFLINE");
}

TEST(PeerTest, StateChangesToOnline) {
    class ConcretePeer : public Peer {
    public:
        void goOnline() { setState(PeerState::ONLINE); }
    };

    ConcretePeer peer;
    peer.goOnline();
    EXPECT_EQ(peer.getState(),    PeerState::ONLINE);
    EXPECT_EQ(peer.getStateStr(), "ONLINE");
}

// ─── Crypto tests ─────────────────────────────────────────────
TEST(CryptoTest, EncryptDecryptRoundTrip) {
    const std::string plain = "Hello, World!";
    std::string cipher = Crypto::encrypt(plain);
    EXPECT_NE(cipher, plain);           // ciphertext differs from plaintext
    std::string recovered = Crypto::decrypt(cipher);
    EXPECT_EQ(recovered, plain);        // round-trip restores original
}

TEST(CryptoTest, EncryptProducesBase64) {
    // Base64 characters: A-Z a-z 0-9 + / =
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
    std::string cipher = Crypto::encrypt("");
    EXPECT_EQ(Crypto::decrypt(cipher), "");
}

TEST(CryptoTest, AckMessageRoundTrip) {
    const std::string msg = "ACK: Hello, World!";
    EXPECT_EQ(Crypto::decrypt(Crypto::encrypt(msg)), msg);
}

