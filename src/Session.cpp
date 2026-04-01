#include <hello_prototype/Session.hpp>
#include <hello_prototype/Crypto.hpp>
#include <iostream>

Session::Session(tcp::socket socket,
                 std::function<void(const std::string&)> on_message,
                 std::function<void()> on_error)
    : socket_(std::move(socket)),
      on_message_(on_message),
      on_error_(on_error) {
    std::cout << "[Session] Created. Remote: "
              << socket_.remote_endpoint().address().to_string()
              << ":" << socket_.remote_endpoint().port() << std::endl;
}

void Session::start() {
    std::cout << "[Session] Started, waiting for data..." << std::endl;
    read();
}

void Session::set_on_message(std::function<void(const std::string&)> on_message) {
    on_message_ = on_message;
}

void Session::write(const std::string& msg) {
    std::string encrypted;
    try {
        encrypted = Crypto::encrypt(msg);
    } catch (const std::exception& e) {
        std::cerr << "[Session] Encrypt error: " << e.what() << std::endl;
        return;
    }
    std::cout << "[Session] >> Sending " << msg.size() << " bytes (plain): \"" << msg << "\"" << std::endl;
    std::cout << "[Session] >> Encrypted (" << encrypted.size() << " bytes B64): \"" << encrypted << "\"" << std::endl;
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(encrypted + "\n"),
        [this, self](const asio::error_code& error, size_t bytes) {
            if (!error) {
                std::cout << "[Session] >> Send OK (" << bytes << " bytes written)" << std::endl;
            }
            handle_write(error);
        });
}

void Session::read() {
    auto self(shared_from_this());
    asio::async_read_until(socket_, buffer_, '\n',
        [this, self](const asio::error_code& error, size_t bytes_transferred) {
            std::cout << "[Session] << Read callback: " << bytes_transferred
                      << " bytes, error=" << error.message() << std::endl;
            handle_read(error, bytes_transferred);
        });
}

void Session::handle_read(const asio::error_code& error, size_t bytes_transferred) {
    if (!error) {
        std::istream is(&buffer_);
        std::string message;
        std::getline(is, message);

        if (!message.empty() && message.back() == '\r') {
            message.pop_back();
        }

        std::cout << "[Session] << Received encrypted (" << message.size() << " bytes B64): \"" << message << "\"" << std::endl;

        std::string decrypted;
        try {
            decrypted = Crypto::decrypt(message);
        } catch (const std::exception& e) {
            std::cerr << "[Session] Decrypt error: " << e.what() << std::endl;
            read();
            return;
        }

        std::cout << "[Session] << Decrypted message: \"" << decrypted << "\"" << std::endl;

        if (on_message_) {
            on_message_(decrypted);
        }
        read();
    } else {
        if (error == asio::error::eof) {
            std::cout << "[Session] Connection closed by peer (EOF)." << std::endl;
        } else if (error == asio::error::connection_reset) {
            std::cout << "[Session] Connection reset by peer." << std::endl;
        } else {
            std::cerr << "[Session] Read error: " << error.message() << std::endl;
        }
        if (on_error_) {
            on_error_();
        }
    }
}

void Session::handle_write(const asio::error_code& error) {
    if (error) {
        std::cerr << "[Session] Write error: " << error.message() << std::endl;
        if (on_error_) {
            on_error_();
        }
    }
}
