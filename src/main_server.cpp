#include "server/Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>

int main(int argc, char* argv[]) {
    uint16_t port = 8888;
    const char* data_dir = "data";

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-p") == 0 || std::strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port = static_cast<uint16_t>(std::atoi(argv[++i]));
            }
        } else if (std::strcmp(argv[i], "-d") == 0 || std::strcmp(argv[i], "--data-dir") == 0) {
            if (i + 1 < argc) {
                data_dir = argv[++i];
            }
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Bind Server" << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -p, --port <port>       Listen port (default: 8888)" << std::endl;
            std::cout << "  -d, --data-dir <dir>    Data directory (default: data)" << std::endl;
            std::cout << "  -h, --help              Show this help" << std::endl;
            return 0;
        }
    }

    TcpSocket::initialize_network();

    Server server(data_dir);
    if (!server.start(port)) {
        std::cerr << "Failed to start server: " << server.last_error().c_str() << std::endl;
        TcpSocket::shutdown_network();
        return 1;
    }

    TcpSocket::shutdown_network();
    return 0;
}
