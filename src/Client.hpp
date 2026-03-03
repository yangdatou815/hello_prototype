#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Peer.hpp"
#include "Session.hpp"
#include <asio.hpp>
#include <string>

using asio::ip::tcp;

class Client : public Peer {
public:
    Client(asio::io_context& io_context,
           const std::string& host,
           unsigned short port);
    void connect();
    void send(const std::string& msg);  // 握手完成后可继续发消息

private:
    void handle_connect(const asio::error_code& error);
    void handle_message(const std::string& message);
    bool is_ready() const;              // 检查是否已连接且 ONLINE

    asio::io_context& io_context_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string host_;
    unsigned short port_;
    std::shared_ptr<Session> session_;
};

#endif // CLIENT_HPP
