#include "Client.hpp"
#include <iostream>

Client::Client(asio::io_context& io_context,
               const std::string& host,
               unsigned short port)
    : io_context_(io_context),
      resolver_(io_context),
      socket_(io_context),
      host_(host),
      port_(port) {
    std::cout << "[Client] Constructed. Target: " << host << ":" << port << std::endl;
}

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
            }
        );
        session_->start();

        std::cout << "[Client] Sending handshake: \"Hello, World!\"" << std::endl;
        session_->write("Hello, World!");
    } else {
        std::cerr << "[Client] Connection failed: " << error.message() << std::endl;
    }
}

void Client::handle_message(const std::string& message) {
    std::cout << "[Client] Message received: \"" << message << "\"" << std::endl;
    if (message.find("ACK") != std::string::npos) {
        setState(PeerState::ONLINE);
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
