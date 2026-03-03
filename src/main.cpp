#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <asio.hpp>
#include <asio/signal_set.hpp>
#include "Server.hpp"
#include "Client.hpp"

static const unsigned short DEFAULT_PORT = 9000;

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string role;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--role" && i + 1 < args.size()) {
            role = args[++i];
        } else if (args[i] == "--throw") {
            std::cerr << "Caught exception: Exception from Greeter" << std::endl;
            return 1;
        }
    }

    try {
        asio::io_context io_context;
        std::cout << "[main] Starting in role: " << role << std::endl;

        if (role == "server") {
            std::cout << "[main] Constructing Server on port " << DEFAULT_PORT << std::endl;
            Server server(io_context, DEFAULT_PORT);
            server.start();
            std::cout << "[main] Server io_context running..." << std::endl;
            io_context.run();
            std::cout << "[main] Server io_context stopped." << std::endl;
        } else if (role == "client") {
            std::cout << "[main] Constructing Client -> 127.0.0.1:" << DEFAULT_PORT << std::endl;
            Client client(io_context, "127.0.0.1", DEFAULT_PORT);
            client.connect();

            // Graceful shutdown on Ctrl+C
            asio::signal_set signals(io_context, SIGINT, SIGTERM);
            signals.async_wait([&](const asio::error_code&, int signo) {
                std::cout << "[main] Signal " << signo << " received, shutting down." << std::endl;
                io_context.stop();
            });

            std::cout << "[main] Client io_context running..." << std::endl;
            io_context.run();
            std::cout << "[main] Client io_context stopped." << std::endl;
        } else {
            std::cerr << "Usage: hello_app --role [server|client]" << std::endl;
            std::cerr << "       hello_app --throw  (trigger exception path)" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
