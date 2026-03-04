#include "Peer.hpp"
#include <iostream>

Peer::Peer() : state_(PeerState::OFFLINE) {
    std::cout << "[Peer] Initialized. State: OFFLINE" << std::endl;
}

PeerState Peer::getState() const {
    return state_;
}

std::string Peer::getStateStr() const {
    switch (state_) {
        case PeerState::OFFLINE:   return "OFFLINE";
        case PeerState::DETECTED:  return "DETECTED";
        case PeerState::CONNECTED: return "CONNECTED";
        case PeerState::ONLINE:    return "ONLINE";
        default:                   return "UNKNOWN";
    }
}

void Peer::setState(PeerState state) {
    std::string old_str = getStateStr();
    state_ = state;
    std::cout << "[Peer] State changed to: " << getStateStr()
              << " (was: " << old_str << ")" << std::endl;
}
