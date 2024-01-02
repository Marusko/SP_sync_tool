#ifndef SYNK_SERVER_H
#define SYNK_SERVER_H
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "messages.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <filesystem>
#include <chrono>

class server {
private:
    sockaddr_in serverAddr{};
    int serverSocket = 0;

    std::mutex syncMutex;
    std::condition_variable syncFree;
    bool syncUsed = false;
    std::string dirPath = "/home/kucera8/sp/s";
    std::vector<FileInfo> serverFiles{};

    std::mutex mainMutex;
    std::condition_variable mainFree;
    bool mainUsed = false;
    int connectedClients = 0;
    bool canAccept = true;

    ResponseMessage handleClientRequest(const CommandMessage& command);
    bool downloadFileFromClient(int clientSocket, CommandMessage& cmd);
    bool uploadFileToClient(int clientSocket, CommandMessage& cmd);
    bool markDeleteFile(int clientSocket, CommandMessage cmd);
    bool deleteFile(int clientSocket, CommandMessage cmd);
    bool sendFileList(int clientSocket);

    std::vector<std::filesystem::directory_entry> getLocalFileEntry();
    std::string getFileList();

    void syncFiles(int clientSocket);
    void handleExitCommand();

    int shift = 2;
    std::string caesarCipher(const std::string& input, int s);

public:
    server(): syncMutex(), syncFree(), mainMutex(), mainFree(){}
    int mainFunction();
};


#endif //SYNK_SERVER_H
