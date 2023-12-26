#ifndef SP_SYNC_TOOL_CLIENT_H
#define SP_SYNC_TOOL_CLIENT_H

#include <vector>
#include <sstream>

class Client {

private:
    bool autoSync;
    std::string syncPath;
    double autoSyncInterval;

    bool setAutoSync(const std::string& opt) {
        if (opt == "on") {
            autoSync = true;
            return true;
        } else if (opt == "off") {
            autoSync = false;
            return true;
        }
        return false;
    }

    void setSyncPath(const std::string& path) {
        syncPath = path;
    }

    bool setAutoSyncInterval(double n) {
        if (n <= 0) {
            return false;
        } else {
            autoSyncInterval = n;
            return true;
        }
    }

    std::vector<std::string> split(const std::string& input) {
        std::istringstream stream(input);
        std::vector<std::string> tokens;
        std::string token;

        while (stream >> token) {
            tokens.push_back(token);
        }

        return tokens;
    }

    void processCommand(const std::string& command, const std::vector<std::string>& args) {
        if (command == "sync") {
            //TODO Implementuj logiku pre manuálnu synchronizáciu súborov
            std::cout << "Synchronization in process\n";
        } else if (command == "autosync") {
            if (args.size() == 1) {
                if (setAutoSync(args[0])) {
                    std::cout << "Automatic synchronization " << args[0] << std::endl;
                } else {
                    std::cout << "Error: Setting is not specified (on/off).\n";
                }
            } else {
                std::cout << "Error: Setting is not specified (on/off).\n";
            }
        } else if (command == "autocheck") {
            if (args.size() == 1) {
                double interval = stod(args[0]);
                if (setAutoSyncInterval(interval)) {
                    std::cout << "Automatic scanning every " << args[0] << " hours" << std::endl;
                } else {
                    std::cout << "Error: Not specified interval of scanning.\n";
                }
            } else {
                std::cout << "Error: Not specified interval of scanning.\n";
            }
        } else if (command == "setpath") {
            if (args.size() == 1) {
                setSyncPath(args[0]);
                std::cout << "Synchronization path set to: " << args[0] << std::endl;
            } else {
                std::cout << "Error: Path is not specified.\n";
            }
        } else if (command == "help" || command == "commands") {
            std::cout << "Available commands:\n"
                         "- sync - synchronizes files\n"
                         "- autosync [on/off] - turns on/off automatic synchronization\n"
                         "- autocheck [duration_in_hours] - sets interval (in hours) how frequently the files should update\n"
                         "- setpath - sets path to the directory, that will be synchronized\n"
                         "- exit - shuts down the application\n";
        } else {
            std::cout << "Error: Unknown command.\n";
        }
    }

public:
    void run() {
        std::cout << "Program is running as client." << std::endl;
        std::string userInput;
        while (true) {

            std::cout << "> ";
            std::getline(std::cin, userInput);

            std::vector<std::string> tokens = split(userInput);

            if (tokens.empty()) {
                continue;
            }

            std::string command = tokens[0];
            if (command == "exit") {
                return;
            }

            std::vector<std::string> args(tokens.begin() + 1, tokens.end());

            processCommand(command, args);
        }
    }
};

#endif //SP_SYNC_TOOL_CLIENT_H
