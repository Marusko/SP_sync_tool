#ifndef SYNK_CLIENT_H
#define SYNK_CLIENT_H
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

class client {
private:
    int clientSocket = 0;
    struct sockaddr_in serverAddr{};

    bool autoSync = false;
    bool autoSyncThreadRun = true;
    std::string syncPath;

    bool autoSyncRunning = false;
    bool syncRunning = false;

    std::thread thread;
    std::mutex mutex;
    std::condition_variable cv;
    void stopThread();

    int autoSyncInterval = 10;
    bool setAutoSync(const std::string& opt);
    void setSyncPath(const std::string& path);
    bool setAutoSyncInterval(int n);
    std::vector<std::string> split(const std::string& input);
    void processCommand(const std::string& command, const std::vector<std::string>& args);

    void sendCommandToServer(const CommandCode& code, const std::string &arg);
    ResponseMessage receiveResponseFromServer();
    bool downloadFileFromServer(FileInfo fileEntry);
    bool uploadFileToServer(std::filesystem::directory_entry &fileEntry);
    bool deleteFileOnServer(const std::string& fileName, CommandCode code);
    bool deleteFile(const std::string& fileName);
    void getServerFiles();

    void parseFileList(const std::string& data);
    std::vector<FileInfo> getLocalFileList();
    std::vector<std::filesystem::directory_entry> getLocalFileEntry();
    std::vector<std::string> getLocalFileStringList();
    bool isFileUpdated(FileInfo file);
    bool isFileNewOrUpdated(std::filesystem::directory_entry &fileEntry);

    void synchronizeFromServer();
    void synchronizeToServer();

    void autoSyncMethod();
    void syncMethod();

    int shift = 2;
    std::string caesarCipher(const std::string& input, int s);

    std::vector<FileInfo> serverFiles;
    std::vector<FileSync> syncTimes;

public:
    client() : mutex{}, cv{}{}
    int mainMethod();
};


#endif //SYNK_CLIENT_H
