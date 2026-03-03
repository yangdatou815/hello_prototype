#ifndef SESSION_HPP
#define SESSION_HPP

#include <asio.hpp>
#include <memory>
#include <functional>

using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, std::function<void(const std::string&)> on_message, std::function<void()> on_error);

    void start();
    void write(const std::string& msg);
    void set_on_message(std::function<void(const std::string&)> on_message);

private:
    void read();
    void handle_read(const asio::error_code& error, size_t bytes_transferred);
    void handle_write(const asio::error_code& error);

    tcp::socket socket_;
    asio::streambuf buffer_;
    std::function<void(const std::string&)> on_message_;
    std::function<void()> on_error_;
};

#endif // SESSION_HPP
