#include <exception>
#include <iostream>
#include "server.hpp"
#include "utils.hpp"

namespace {
    const int LISTEN_PORT = 1;
    const int FORWARD_HOST = 2;
    const int FORWARD_PORT = 3;
}

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Invalid arguments! Usage: " << argv[0] << " listen_port" << std::endl;
        exit(EXIT_SUCCESS);
    }

    try {
        int listen_port = Utils::parse_port(argv[LISTEN_PORT]);
        Server server(listen_port);
        server.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}