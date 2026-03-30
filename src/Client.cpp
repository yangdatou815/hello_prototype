#include "Client.hpp"
#include "Database.hpp"
#include <iostream>

Client::Client(asio::io_context& io_context,
               const std::string& host,
               unsigned short port)
    : io_context_(io_context),
      resolver_(io_context),
      socket_(io_context),
      host_(host),
      port_(port),
      db_(std::make_unique<Database>("client.db")) {
    std::cout << "[Client] Constructed. Target: " << host << ":" << port << std::endl;
    db_->updatePeerState("server", PeerState::OFFLINE);
}

//client connect
void Client::connect() {
    std::cout << "[Client] Resolving " << host_ << ":" << port_ << "..." << std::endl;
    auto endpoints = resolver_.resolve(host_, std::to_string(port_));
    std::cout << "[Client] Connecting..." << std::endl;
    asio::async_connect(socket_, endpoints,
        [this](const asio::error_code& error, const tcp::endpoint& ep) {
            if (!error) {
                std::cout << "[Client] TCP connect OK -> "
                          << ep.address().to_string() << ":" << ep.port() << std::endl;
            }
            handle_connect(error);
        });
}

void Client::handle_connect(const asio::error_code& error) {
    if (!error) {
        std::cout << "[Client] Session established. Starting handshake..." << std::endl;

        session_ = std::make_shared<Session>(
            std::move(socket_),
            [this](const std::string& msg) { handle_message(msg); },
            [this]() {
                std::cerr << "[Client] Connection lost. State: " << getStateStr() << std::endl;
                db_->updatePeerState("server", PeerState::OFFLINE);
                setState(PeerState::OFFLINE);
            }
        );
        session_->start();

        // Step 1: send DETECTED
        setState(PeerState::DETECTED);
        db_->updatePeerState("server", PeerState::DETECTED);
        std::cout << "[Client] Handshake step 1: sending \"DETECTED\"" << std::endl;
        session_->write("DETECTED");
    } else {
        std::cerr << "[Client] Connection failed: " << error.message() << std::endl;
    }
}

void Client::handle_message(const std::string& message) {
    std::cout << "[Client] Message received: \"" << message << "\"" << std::endl;

    // Store every received message
    db_->insertMessage("server", message);

    if (message == "CONNECTED" && getState() == PeerState::DETECTED) {
        // Step 2 ACK received: advance to CONNECTED, send ONLINE
        setState(PeerState::CONNECTED);
        db_->updatePeerState("server", PeerState::CONNECTED);
        std::cout << "[Client] Handshake step 3: sending \"ONLINE\"" << std::endl;
        session_->write("ONLINE");
    } else if (message == "ACK: ONLINE" && getState() == PeerState::CONNECTED) {
        // Step 3 ACK received: handshake complete
        setState(PeerState::ONLINE);
        db_->updatePeerState("server", PeerState::ONLINE);
        std::cout << "[Client] Handshake complete. State: " << getStateStr() << std::endl;
    } else {
        std::cout << "[Client] Unrecognised message, ignoring." << std::endl;
    }
}

void Client::send(const std::string& msg) {
    if (!is_ready()) {
        std::cerr << "[Client] Cannot send: not connected or not ONLINE (state="
                  << getStateStr() << ")" << std::endl;
        return;
    }
    std::cout << "[Client] send() -> \"" << msg << "\"" << std::endl;
    session_->write(msg);
}

bool Client::is_ready() const {
    return session_ != nullptr && getState() == PeerState::ONLINE;
}
