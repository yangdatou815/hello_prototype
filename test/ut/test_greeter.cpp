#include "Peer.hpp"
#include <gtest/gtest.h>

TEST(PeerTest, InitialStateIsOffline) {
    // Peer is abstract; use a concrete subclass for testing
    class ConcretePeer : public Peer {
    public:
        void goOnline() { setState(PeerState::ONLINE); }
        void goOffline() { setState(PeerState::OFFLINE); }
    };

    ConcretePeer peer;
    EXPECT_EQ(peer.getState(), PeerState::OFFLINE);
    EXPECT_EQ(peer.getStateStr(), "OFFLINE");
}

TEST(PeerTest, StateChangesToOnline) {
    class ConcretePeer : public Peer {
    public:
        void goOnline() { setState(PeerState::ONLINE); }
    };

    ConcretePeer peer;
    peer.goOnline();
    EXPECT_EQ(peer.getState(), PeerState::ONLINE);
    EXPECT_EQ(peer.getStateStr(), "ONLINE");
}
