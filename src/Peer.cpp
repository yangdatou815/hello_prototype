#include "Peer.hpp"
#include <iostream>

Peer::Peer() : state_(PeerState::OFFLINE) {
    std::cout << "[Peer] Initialized. State: OFFLINE" << std::endl;
}

PeerState Peer::getState() const {
    return state_;
}

std::string Peer::getStateStr() const {
    return (state_ == PeerState::ONLINE) ? "ONLINE" : "OFFLINE";
}

void Peer::setState(PeerState state) {
    std::string old_str = getStateStr();
    state_ = state;
    std::cout << "[Peer] State changed to: " << getStateStr()
              << " (was: " << old_str << ")" << std::endl;
}
