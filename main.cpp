#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "You need to specify a program argument -client or -server!" << std::endl;
        return 1;
    }

    std::string argument = argv[1];

    if (argument == "-client") {
        std::cout << "Program is running as client." << std::endl;
        // TODO start client
    } else if (argument == "-server") {
        std::cout << "Program is running as server." << std::endl;
        // TODO start server
    } else {
        std::cout << "Invalid startup argument! Use -client or -server!" << std::endl;
    }

    return 0;
}
