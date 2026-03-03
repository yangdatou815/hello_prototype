#ifndef SERVER_HPP
#define SERVER_HPP

#include "Peer.hpp"
#include "Session.hpp"
#include <asio.hpp>

using asio::ip::tcp;

class Server : public Peer {
public:
    Server(asio::io_context& io_context, unsigned short port);
    void start();

private:
    void accept();
    void handle_message(std::shared_ptr<Session> session, const std::string& message);

    tcp::acceptor acceptor_;
    asio::io_context& io_context_;
};

#endif // SERVER_HPP
