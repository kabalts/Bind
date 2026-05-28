#include "client/Client.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>

int main(int argc, char* argv[]) {
    const char* host = "127.0.0.1";
    uint16_t port = 8888;
    const char* default_db = "";

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--host") == 0) {
            if (i + 1 < argc) {
                host = argv[++i];
            }
        } else if (std::strcmp(argv[i], "-p") == 0 || std::strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port = static_cast<uint16_t>(std::atoi(argv[++i]));
            }
        } else if (std::strcmp(argv[i], "-d") == 0 || std::strcmp(argv[i], "--db") == 0) {
            if (i + 1 < argc) {
                default_db = argv[++i];
            }
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Bind Client" << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -h, --host <host>    Server host (default: 127.0.0.1)" << std::endl;
            std::cout << "  -p, --port <port>    Server port (default: 8888)" << std::endl;
            std::cout << "  -d, --db <dbname>    Default database" << std::endl;
            std::cout << "  --help               Show this help" << std::endl;
            return 0;
        }
    }

    TcpSocket::initialize_network();

    Client client;
    if (!client.connect(host, port, default_db)) {
        std::cerr << "Connection failed: " << client.last_error().c_str() << std::endl;
        TcpSocket::shutdown_network();
        return 1;
    }

    std::cout << "Connected to Bind server at " << host << ":" << port << std::endl;

    client.run();

    TcpSocket::shutdown_network();
    return 0;
}
