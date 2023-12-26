#include <iostream>
#include "Client.h"
#include "Server.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "You need to specify a program argument -client or -server!" << std::endl;
        return 1;
    }

    std::string argument = argv[1];

    if (argument == "-client") {
        Client client;
        client.run();
    } else if (argument == "-server") {
        Server server;
        server.run();
    } else {
        std::cout << "Invalid startup argument! Use -client or -server!" << std::endl;
        return 1;
    }

    return 0;
}
