#include "Server.hpp"
#include <iostream>

Server::Server(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "[Server] Constructed. Bound to port " << port << std::endl;
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
                std::cout << "[Server] New connection from "
                          << socket.remote_endpoint().address().to_string()
                          << ":" << socket.remote_endpoint().port() << std::endl;

                auto session = std::make_shared<Session>(
                    std::move(socket),
                    nullptr,
                    [this]() { std::cout << "[Server] A session encountered an error." << std::endl; }
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
    if (message == "Hello, World!") {
        std::cout << "[Server] Hello received. Sending ACK." << std::endl;
        session->write("ACK: Hello, World!");
    } else {
        std::cout << "[Server] Unknown message, ignoring." << std::endl;
    }
}
