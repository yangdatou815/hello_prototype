#ifndef PEER_HPP
#define PEER_HPP

#include <string>

enum class PeerState {
    OFFLINE,
    ONLINE
};

class Peer {
public:
    Peer();
    virtual ~Peer() = default;

    PeerState getState() const;
    std::string getStateStr() const;

protected:
    void setState(PeerState state);

private:
    PeerState state_;
};

#endif // PEER_HPP
