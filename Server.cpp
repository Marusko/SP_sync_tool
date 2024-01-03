#include "Server.h"

namespace fs = std::filesystem;

ResponseMessage server::handleClientRequest(const CommandMessage& command) {
    ResponseMessage response;
    response.code = RESPONSE_OK;

    switch (command.code) {
        case CMD_LIST_FILES:
            response.code = RESPONSE_FILE_LIST;
            response.data = getFileList();
            break;
        case CMD_DOWNLOAD_FILE:
        case CMD_UPLOAD_FILE:
        case CMD_DELETE_FILE:
        case CMD_MARK_DELETE_FILE:
            response.data = command.argument;
            break;
        default:
            response.code = RESPONSE_ERROR;
            response.data = "Unknown error";
    }

    return response;
}

std::string server::caesarCipher(const std::string& input, int s) {
    std::string encryptedText = input;

    // Posun pre písmená
    for (char& c : encryptedText) {
        if (isalpha(c)) {
            char base = (isupper(c) ? 'A' : 'a');
            c = ((c - base + s) % 26 + 26) % 26 + base;
        }
    }

    // Posun pre čísla
    for (char& c : encryptedText) {
        if (isdigit(c)) {
            c = ((c - '0' + s) % 10 + 10) % 10 + '0';
        }
    }

    return encryptedText;
}

bool server::downloadFileFromClient(int clientSocket, CommandMessage& cmd) {
    auto s = cmd.argument;
    std::string delim("/");
    auto fileName = s.substr(0, s.find(delim));
    s.erase(0, s.find(delim) + delim.length());
    int fileSize = std::stoi(s.substr(0, s.find(delim)));
    s.erase(0, s.find(delim) + delim.length());
    unsigned long lastWrite = std::stoul(s);

    auto rem = std::find_if(serverFiles.begin(), serverFiles.end(), [&fileName](const FileInfo &fileInfo){ return fileInfo.filename == fileName; });
    if (rem != serverFiles.end()) {
        rem.operator*().lastModified = lastWrite;
        rem.operator*().deleted = 0;
    } else {
        serverFiles.push_back({fileName, 0, lastWrite});
    }

    std::string f(dirPath + "/" + fileName);
    std::ofstream file(f, std::ios::binary);
    if (!file.is_open()) {
        time_t now = time(nullptr);
        std::cerr << "[" << strtok(ctime(&now), "\n") << "][E] Failed to create file " << fileName << std::endl;
        ResponseMessage resp{RESPONSE_ERROR, "Failed to create file " + fileName};
        char ser2[1024];
        auto str2 = caesarCipher(resp.serialize(), shift);
        strcpy(ser2, str2.c_str());
        send(clientSocket, ser2, sizeof(ser2), 0);
        return false;
    }

    char ser[1024];
    auto str = caesarCipher(handleClientRequest(cmd).serialize(), shift);
    strcpy(ser, str.c_str());
    send(clientSocket, ser, sizeof(ser), 0);

    // Prijmi a zapíš obsah súboru
    char buffer[1024];
    int remainingBytes = fileSize;
    while (remainingBytes > 0) {
        int bytesRead = recv(clientSocket, buffer, std::min<int>(remainingBytes, sizeof(buffer)), 0);
        std::string ciph = buffer;
        auto deciph = caesarCipher(ciph, -shift);
        const int length = deciph.length();
        char tmp[length + 1];
        strcpy(tmp, deciph.c_str());
        file.write(tmp, bytesRead);
        remainingBytes -= bytesRead;
    }
    file.close();

    ResponseMessage resp{RESPONSE_OK, "OK"};
    char ser2[1024];
    auto str2 = caesarCipher(resp.serialize(), shift);
    strcpy(ser2, str2.c_str());
    send(clientSocket, ser2, sizeof(ser2), 0);
    time_t now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] File " << fileName << " downloaded from client [" << clientSocket << "]" << std::endl;
    return true;
}

bool server::sendFileList(int clientSocket) {
    ResponseMessage resp{RESPONSE_OK, "OK"};
    char ser2[1024];
    auto str2 = caesarCipher(resp.serialize(), shift);
    strcpy(ser2, str2.c_str());
    send(clientSocket, ser2, sizeof(ser2), 0);

    char buf[1024];
    recv(clientSocket, buf, sizeof(buf), 0);
    auto deser = CommandMessage::deserialize(caesarCipher(buf, -shift));

    char ser[1024];
    auto str = caesarCipher(handleClientRequest(deser).serialize(), shift);
    strcpy(ser, str.c_str());

    send(clientSocket, ser, sizeof(ser), 0);
    time_t now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Sent FILE_LIST to client [" << clientSocket << "]" << std::endl;
    return true;
}

bool server::uploadFileToClient(int clientSocket, CommandMessage& cmd) {
    char ser[1024];
    auto resp = handleClientRequest(cmd);
    std::string f = dirPath + "/" + cmd.argument;
    std::ifstream file(f, std::ios::binary);
    if (!file.is_open()) {
        time_t now = time(nullptr);
        std::cerr << "[" << strtok(ctime(&now), "\n") << "][E] Failed to open file " << cmd.argument << std::endl;
        resp = {RESPONSE_ERROR, "Failed to open file " + cmd.argument};
        char ser2[1024];
        auto str2 = caesarCipher(resp.serialize(), shift);
        strcpy(ser2, str2.c_str());
        send(clientSocket, ser2, sizeof(ser2), 0);
        return false;
    }

    // Získaj veľkosť súboru
    file.seekg(0, std::ios::end);
    long fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    resp.data = std::to_string(fileSize);

    auto str = caesarCipher(resp.serialize(), shift);
    strcpy(ser, str.c_str());
    send(clientSocket, ser, sizeof(ser), 0);

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::string deciph = buffer;
        auto ciph = caesarCipher(deciph, shift);
        const int length = ciph.length();
        char tmp[length + 1];
        strcpy(tmp, ciph.c_str());
        send(clientSocket, tmp, file.gcount(), 0);
    }
    file.close();

    time_t now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Sent " << cmd.argument << " to client [" << clientSocket << "]" << std::endl;
    return true;
}

bool server::markDeleteFile(int clientSocket, CommandMessage cmd) {
    char ser[1024];
    auto resp = handleClientRequest(cmd);
    auto rem = std::find_if(serverFiles.begin(), serverFiles.end(), [&cmd](const FileInfo &fileInfo){ return fileInfo.filename == cmd.argument; });
    if (rem != serverFiles.end()) {
        rem.operator*().deleted = 1;
    }

    auto str = caesarCipher(resp.serialize(), shift);
    strcpy(ser, str.c_str());
    send(clientSocket, ser, sizeof(ser), 0);
    time_t now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Deleted " << cmd.argument << " from client [" << clientSocket << "]" << " and server" << std::endl;
    return true;
}

bool server::deleteFile(int clientSocket, CommandMessage cmd) {
    char ser[1024];
    auto resp = handleClientRequest(cmd);

    std::string path(dirPath + "/" + cmd.argument);
    const int length = path.length();
    char tmp[length + 1];
    strcpy(tmp, path.c_str());
    int res = std::remove(tmp);
    if (res != 0) {
        time_t now = time(nullptr);
        std::cerr << "[" << strtok(ctime(&now), "\n") << "][E] Failed to delete file " << cmd.argument << std::endl;
        resp = {RESPONSE_ERROR, "Failed to delete file " + cmd.argument};
        char ser2[1024];
        auto str2 = caesarCipher(resp.serialize(), shift);
        strcpy(ser2, str2.c_str());
        send(clientSocket, ser2, sizeof(ser2), 0);
        return false;
    }
    auto rem = serverFiles.erase(std::find_if(serverFiles.begin(), serverFiles.end(), [&cmd](const FileInfo &fileInfo){ return fileInfo.filename == cmd.argument; }));

    auto str = caesarCipher(resp.serialize(), shift);
    strcpy(ser, str.c_str());
    send(clientSocket, ser, sizeof(ser), 0);
    time_t now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Deleted " << cmd.argument << " from client [" << clientSocket << "]" << " and server" << std::endl;
    return true;
}

std::string server::getFileList() {
    std::stringstream fileList;
    for (const auto& file : serverFiles) {
        fileList << file.filename << "/" << file.lastModified << "/" << file.deleted << " ";
    }
    return fileList.str();
}

std::vector<fs::directory_entry> server::getLocalFileEntry() {
    std::vector<fs::directory_entry> out{};

    for (const auto & entry : fs::directory_iterator(dirPath)) {
        out.push_back(entry);
    }
    return out;
}

void server::syncFiles(int clientSocket) {
    while (true) {
        char buf[1024];
        recv(clientSocket, buf, sizeof(buf), 0);
        auto deser = CommandMessage::deserialize(caesarCipher(buf, -shift));
        if (deser.code == CMD_CLOSE) {
            std::unique_lock<std::mutex> lock(mainMutex);
            while (mainUsed) {
                mainFree.wait(lock);
            }
            mainUsed = true;
            close(clientSocket);
            connectedClients--;
            mainUsed = false;
            mainFree.notify_one();
            lock.unlock();
            time_t now = time(nullptr);
            std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Client [" << clientSocket << "] disconnected" << std::endl;
            break;
        } else {
            std::unique_lock<std::mutex> lock(syncMutex);
            while (syncUsed) {
                syncFree.wait(lock);
            }
            syncUsed = true;
            if (deser.code == CMD_START_SYNC) {
                sendFileList(clientSocket);
                while (true) {
                    char cmd[1024];
                    recv(clientSocket, cmd, sizeof(cmd), 0);
                    auto dwn = CommandMessage::deserialize(caesarCipher(cmd, -shift));
                    if (dwn.code == CMD_END_SYNC) {
                        break;
                    } else if (dwn.code == CMD_DOWNLOAD_FILE) {
                        uploadFileToClient(clientSocket, dwn);
                    } else if (dwn.code == CMD_UPLOAD_FILE) {
                        downloadFileFromClient(clientSocket, dwn);
                    }
                }
            } else if (deser.code == CMD_MARK_DELETE_FILE) {
                markDeleteFile(clientSocket, deser);
            } else if (deser.code == CMD_DELETE_FILE) {
                deleteFile(clientSocket, deser);
            } else if (deser.code == CMD_DOWNLOAD_FILE) {
                uploadFileToClient(clientSocket, deser);
            }
            syncUsed = false;
            syncFree.notify_one();
            lock.unlock();
        }
    }
}

void server::handleExitCommand() {
    std::string userInput;
    std::getline(std::cin, userInput);
    if (userInput == "exit") {
        std::unique_lock<std::mutex> lock(mainMutex);
        while (mainUsed) {
            mainFree.wait(lock);
        }
        mainUsed = true;
        canAccept = false;
        mainUsed = false;
        mainFree.notify_one();
        lock.unlock();
        time_t now = time(nullptr);
        std::cout << "[" << strtok(ctime(&now), "\n") << "][I] The server will shut down when all clients disconnect" << std::endl;
    }
}

int server::handleConnections() {
    time_t now = time(nullptr);
    std::vector<std::thread> clientThreads{};
    sockaddr_in clientAddr{};
    socklen_t clientAddrSize = sizeof(clientAddr);
    bool exit = false;
    while (!exit) {
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            now = time(nullptr);
            std::cerr << "[" << strtok(ctime(&now), "\n") << "][E] Error accepting connection" << std::endl;
            close(serverSocket);
            return EXIT_FAILURE;
        }
        now = time(nullptr);
        std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Client [" << clientSocket << "] connected" << std::endl;

        std::thread clientT(&server::syncFiles, this, clientSocket);
        clientThreads.push_back(std::move(clientT));

        std::unique_lock<std::mutex> lock(mainMutex);
        while (mainUsed) {
            mainFree.wait(lock);
        }
        mainUsed = true;
        connectedClients++;
        if (connectedClients == 0 || !canAccept) {
            exit = true;
        }
        mainUsed = false;
        mainFree.notify_one();
        lock.unlock();
    }
    for (auto& t : clientThreads) {
        t.join();
    }
    return 0;
}

int server::mainFunction() {
    std::cout << "[Q] Enter server port number: " << std::endl;
    std::string input;
    std::getline(std::cin, input);
    int port = std::stoi(input);

    std::cout << "[Q] Enter path to directory: " << std::endl;
    std::string file;
    std::getline(std::cin, file);
    dirPath = file;

    time_t now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] The server will use port " << port << std::endl;

    now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] The server is turning on" << std::endl;
    std::thread exitThread(&server::handleExitCommand, this);
    for (auto &entry : getLocalFileEntry()) {
        std::string fileName = entry.path().filename();
        unsigned long fileTime = std::chrono::time_point_cast<std::chrono::milliseconds>(entry.last_write_time()).time_since_epoch().count();
        serverFiles.push_back({fileName, 0, fileTime});
    }
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        now = time(nullptr);
        std::cerr << "[" << strtok(ctime(&now), "\n") << "][E] Error creating socket" << std::endl;
        return EXIT_FAILURE;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        now = time(nullptr);
        std::cerr << "[" << strtok(ctime(&now), "\n") << "][E] Error binding to port" << std::endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 5) == -1) {
        now = time(nullptr);
        std::cerr << "[" << strtok(ctime(&now), "\n") << "][E] Error listening on port" << std::endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }

    auto connT = std::thread(&server::handleConnections, this);
    now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Server ON" << std::endl;
    connT.join();
    exitThread.join();
    close(serverSocket);
    now = time(nullptr);
    std::cout << "[" << strtok(ctime(&now), "\n") << "][I] Server OFF" << std::endl;
    return EXIT_SUCCESS;
}
