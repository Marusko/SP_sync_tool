#include <iostream>
#include <string>
#include "client.h"
#include "server.h"

int main() {
    std::string input;
    while (true) {
        std::cout << "Enter \n[ 's' / 'server' ] to run as server\n[ 'c' / 'client' ] to run as client\n";
        std::getline(std::cin, input);

        if (input == "s" || input == "server") {
            server s;
            return s.mainFunction();
        } else if (input == "c" || input == "client") {
            client c;
            return c.mainMethod();
        } else {
            std::cout << "Invalid input, please enter valid arguments!" << std::endl;
        }
    }
}
