#include "Server.hpp"
#include "Database.hpp"
#include <iostream>

Server::Server(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      db_(std::make_unique<Database>("server.db")) {
    std::cout << "[Server] Constructed. Bound to port " << port << std::endl;
    db_->updatePeerState("client", PeerState::OFFLINE);
}

void Server::start() {
    std::cout << "[Server] Listening on port "
              << acceptor_.local_endpoint().port() << std::endl;
    accept();
}

void Server::accept() {
    std::cout << "[Server] Waiting for next connection..." << std::endl;
    acceptor_.async_accept(
        [this](const asio::error_code& error, tcp::socket socket) {
            if (!error) {
                std::string remote =
                    socket.remote_endpoint().address().to_string()
                    + ":" + std::to_string(socket.remote_endpoint().port());
                std::cout << "[Server] New connection from " << remote << std::endl;

                auto session = std::make_shared<Session>(
                    std::move(socket),
                    nullptr,
                    [this]() {
                        std::cout << "[Server] A session encountered an error." << std::endl;
                        db_->updatePeerState("client", PeerState::OFFLINE);
                        setState(PeerState::OFFLINE);
                    }
                );
                std::weak_ptr<Session> weak_session = session;
                session->set_on_message([this, weak_session](const std::string& msg) {
                    if (auto s = weak_session.lock()) {
                        handle_message(s, msg);
                    }
                });
                session->start();
            } else {
                std::cerr << "[Server] Accept error: " << error.message() << std::endl;
            }
            accept();
        });
}

void Server::handle_message(std::shared_ptr<Session> session, const std::string& message) {
    std::cout << "[Server] Processing message: \"" << message << "\"" << std::endl;

    // Store every received message
    db_->insertMessage("client", message);

    if (message == "DETECTED") {
        // Handshake step 1: client detected us → reply CONNECTED
        setState(PeerState::DETECTED);
        db_->updatePeerState("client", PeerState::DETECTED);
        std::cout << "[Server] Handshake step 2: sending \"CONNECTED\"" << std::endl;
        session->write("CONNECTED");
    } else if (message == "ONLINE") {
        // Handshake step 3: client confirmed online → reply ACK: ONLINE
        setState(PeerState::ONLINE);
        db_->updatePeerState("client", PeerState::ONLINE);
        std::cout << "[Server] Handshake complete. Sending \"ACK: ONLINE\"" << std::endl;
        session->write("ACK: ONLINE");
    } else {
        std::cout << "[Server] Unknown message, ignoring." << std::endl;
    }
}
