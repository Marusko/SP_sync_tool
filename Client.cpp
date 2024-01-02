#include "Client.h"
namespace fs = std::filesystem;

std::string client::caesarCipher(const std::string& input, int s) {
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

void client::stopThread() {
    std::unique_lock<std::mutex> lock(mutex);
    autoSyncThreadRun = false;
    autoSync = false;
    lock.unlock();
    cv.notify_one();
    thread.join();
}

bool client::setAutoSync(const std::string& opt){
    if (opt == "on") {
        autoSync = true;
        return true;
    } else if (opt == "off") {
        autoSync = false;
        return true;
    }
    return false;
}
void client::setSyncPath(const std::string& path){
    syncPath = path;
}
bool client::setAutoSyncInterval(int n){
    if (n <= 0) {
        return false;
    } else {
        autoSyncInterval = n;
        return true;
    }
}
std::vector<std::string> client::split(const std::string& input){
    std::istringstream stream(input);
    std::vector<std::string> tokens;
    std::string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}
void client::processCommand(const std::string& command, const std::vector<std::string>& args){
    if (command == "sync") {
        syncMethod();
    } else if (command == "autosync") {
        if (args.size() == 1) {
            if (setAutoSync(args[0])) {
                std::cout << "[I] Automatic synchronization " << args[0] << std::endl;
            } else {
                std::cerr << "[E] Setting not specified (on/off)" << std::endl;
            }
        } else {
            std::cerr << "[E] Setting not specified (on/off)" << std::endl;
        }
    } else if (command == "autocheck") {
        if (args.size() == 1) {
            int interval = stoi(args[0]);
            if (setAutoSyncInterval(interval)) {
                std::cout << "[I] Automatic scan every " << args[0] << " seconds" << std::endl;
            } else {
                std::cerr << "[E] Time not specified" << std::endl;
            }
        } else {
            std::cerr << "[E] Time not specified" << std::endl;
        }
    } else if (command == "setpath") {
        if (args.size() == 1) {
            setSyncPath(args[0]);
            std::cout << "[I] Path to sync folder set to: " << args[0] << std::endl;
        } else {
            std::cerr << "[E] Path not specified" << std::endl;
        }
    } else if (command == "delete") {
        if (args.size() == 1) {
            std::cout << "[I] File " << args[0] << " will be deleted" << std::endl;
            deleteFileOnServer(args[0], CMD_MARK_DELETE_FILE);
        } else {
            std::cerr << "[E] File not specified" << std::endl;
        }
    } else if (command == "sdelete") {
        if (args.size() == 1) {
            std::cout << "[I] File " << args[0] << " will be deleted on server" << std::endl;
            deleteFileOnServer(args[0], CMD_DELETE_FILE);
        } else {
            std::cerr << "[E] File not specified" << std::endl;
        }
    } else if (command == "restore") {
        if (args.size() == 1) {
            std::cout << "[I] File " << args[0] << " will be restored" << std::endl;
            auto tmp = downloadFileFromServer({args[0], 0, 0});
            if (tmp) {
                std::cout << "[I] File " << args[0] << " was restored" << std::endl;
            } else {
                std::cerr << "[E] File " << args[0] << " was not restored" << std::endl;
            }
        } else {
            std::cerr << "[E] File not specified" << std::endl;
        }
    } else if (command == "slist") {
        getServerFiles();
    } else if (command == "help" || command == "commands") {
        std::cout << "Available commands:\n- slist - get list of files on server\n- sync - synchronize files with server\n- autosync [on|off] - turn on / off automatic synchronization\n- autocheck [s] - in seconds, sets how often the sync folder should be scanned for file updates\n- setpath [path] - sets the path to the folder whose contents will be synchronized\n- delete [file] - deletes the specified file\n- sdelete [file] - deletes the specified file on server\n- restore [file] - restores the specified file\n- exit - closes the application" << std::endl;
    } else {
        std::cerr << "[E] Unknown command" << std::endl;
    }
}

void client::sendCommandToServer(const CommandCode &code, const std::string &arg) {
    CommandMessage cm{code, arg};
    auto str = caesarCipher(cm.serialize(), shift);
    int length = str.length();
    char ser[length + 1];
    strcpy(ser, str.c_str());
    send(clientSocket, ser, sizeof(ser), 0);
}
ResponseMessage client::receiveResponseFromServer() {
    char buf[1024];
    recv(clientSocket, buf, sizeof(buf), 0);
    auto rm = ResponseMessage::deserialize(caesarCipher(buf, -shift));
    return rm;
}

bool client::downloadFileFromServer(FileInfo fileEntry) {
    std::string f(syncPath + "/" + fileEntry.filename);
    std::ofstream file(f, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[E] Failed to create file" << std::endl;
        return false;
    }

    sendCommandToServer(CMD_DOWNLOAD_FILE, fileEntry.filename);
    auto rm = receiveResponseFromServer();
    if (rm.code != RESPONSE_OK) {
        std::cerr << "[E] An error occurred on the server - " << rm.data << std::endl;
        std::string path(f);
        const int length = path.length();
        char tmp[length + 1];
        strcpy(tmp, path.c_str());
        int res = std::remove(tmp);
        return false;
    }
    std::cout << "[I] Downloading a file "<< fileEntry.filename <<" from the server" << std::endl;

    int fileSize = std::stoi(rm.data);

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

    std::cout << "[I] File contents of "<< fileEntry.filename <<" was downloaded" << std::endl;

    auto rem = std::find_if(syncTimes.begin(), syncTimes.end(), [&fileEntry](const FileSync &fileSync){ return fileSync.filename == fileEntry.filename; });
    auto now = std::chrono::file_clock::now();
    if (rem != syncTimes.end()) {
        rem.operator*().syncTime = (unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    } else {
        syncTimes.push_back({fileEntry.filename, (unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count()});
    }

    return true;
}
bool client::uploadFileToServer(fs::directory_entry &fileEntry) {
    std::string f = syncPath + "/" + fileEntry.path().filename().string() ;
    std::ifstream file(f, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[E] Failed to open file" << std::endl;
        return false;
    }

    // Získaj veľkosť súboru
    file.seekg(0, std::ios::end);
    long fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    auto uploadString(fileEntry.path().filename().string()
                      + "/" +
                      std::to_string(fileSize)
                      + "/" +
                      std::to_string((unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(fileEntry.last_write_time()).time_since_epoch().count()));

    sendCommandToServer(CMD_UPLOAD_FILE, uploadString);
    auto rm = receiveResponseFromServer();
    if (rm.code != RESPONSE_OK) {
        std::cerr << "[E] An error occurred on the server - " << rm.data << std::endl;
        return false;
    }
    std::cout << "[I] Uploading "<< fileEntry.path().filename() <<" to the server" << std::endl;

    // Odosli obsah súboru
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

    rm = receiveResponseFromServer();
    if (rm.code != RESPONSE_OK) {
        std::cerr << "[E] File "<< fileEntry.path().filename() <<" was not uploaded" << std::endl;
        return false;
    }
    std::cout << "[I] File "<< fileEntry.path().filename() <<" was uploaded" << std::endl;

    std::string filename = fileEntry.path().filename();
    auto rem = std::find_if(syncTimes.begin(), syncTimes.end(), [&filename](const FileSync &fileSync){ return fileSync.filename == filename; });
    auto now = std::chrono::file_clock::now();
    if (rem != syncTimes.end()) {
        rem.operator*().syncTime = (unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    } else {
        syncTimes.push_back({filename, (unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count()});
    }
    return true;
}
bool client::deleteFileOnServer(const std::string &fileName, CommandCode code) {
    auto res = deleteFile(fileName);

    sendCommandToServer(code, fileName);
    auto rm = receiveResponseFromServer();
    if (rm.code != RESPONSE_OK) {
        std::cerr << "[E] An error occurred on the server - " << rm.data << std::endl;
        return false;
    }
    auto rem = syncTimes.erase(std::remove_if(syncTimes.begin(), syncTimes.end(), [&fileName](const FileSync &fileSync){ return fileSync.filename == fileName; }), syncTimes.end());
    if (res) {
        std::cout << "[I] File "<< fileName <<" was deleted" << std::endl;
        return true;
    } else {
        std::cout << "[I] File "<< fileName <<" was deleted only on server" << std::endl;
        return false;
    }

}
bool client::deleteFile(const std::string &fileName) {
    std::string path(syncPath + "/" + fileName);
    const int length = path.length();
    char tmp[length + 1];
    strcpy(tmp, path.c_str());
    int res = std::remove(tmp);
    return res == 0;
}
void client::getServerFiles() {
    sendCommandToServer(CMD_START_SYNC, "start");
    ResponseMessage responseOK = receiveResponseFromServer();
    if (responseOK.code != RESPONSE_OK) {
        return;
    }
    sendCommandToServer(CMD_LIST_FILES, "all");
    ResponseMessage response = receiveResponseFromServer();
    if (response.code == RESPONSE_FILE_LIST) {
        parseFileList(response.data);
        std::cout << "Files on server:" << std::endl;
        for (auto file : serverFiles) {
            std::cout << " - " << file.filename << (file.deleted == 1 ? "[D]" : "") << std::endl;
        }
    } else {
        std::cerr << "[E] Error retrieving file list from server" << std::endl;
    }
    sendCommandToServer(CMD_END_SYNC, "end");
}

void client::parseFileList(const std::string &data) {
    std::istringstream stream(data);
    std::string file;
    serverFiles.clear();
    while (stream >> file) {
        std::string delim("/");
        std::string fileName = file.substr(0, file.find(delim));
        file.erase(0, file.find(delim) + delim.length());
        unsigned long lastWrite = std::stoul(file.substr(0, file.find(delim)));
        file.erase(0, file.find(delim) + delim.length());
        int deleted = std::stoi(file);
        FileInfo fi{fileName, deleted ,lastWrite};
        serverFiles.push_back(fi);
    }
}
std::vector<FileInfo> client::getLocalFileList() {
    std::vector<FileInfo> out{};

    for (const auto & entry : fs::directory_iterator(syncPath)) {
        out.push_back({entry.path().filename(), false, (unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(entry.last_write_time()).time_since_epoch().count()});
    }
    return out;
}
std::vector<fs::directory_entry> client::getLocalFileEntry() {
    std::vector<fs::directory_entry> out{};

    for (const auto & entry : fs::directory_iterator(syncPath)) {
        out.push_back(entry);
    }
    return out;
}
std::vector<std::string> client::getLocalFileStringList() {
    std::vector<std::string> out{};
    for (auto &f : getLocalFileList()) {
        out.push_back(f.filename);
    }
    return out;
}
bool client::isFileUpdated(FileInfo file) {
    auto localFiles = getLocalFileList();
    auto rem = std::find_if(localFiles.begin(), localFiles.end(), [&file](const FileInfo &fileInfo){ return fileInfo.filename == file.filename; });
    auto f = rem.operator*();
    return file.lastModified > f.lastModified;
}
bool client::isFileNewOrUpdated(fs::directory_entry &fileEntry) {
    std::string file = fileEntry.path().filename();
    auto rem = std::find_if(syncTimes.begin(), syncTimes.end(), [&file](const FileSync &fileSync){ return fileSync.filename == file; });
    auto srem = std::find_if(serverFiles.begin(), serverFiles.end(), [&file](const FileInfo &fileInfo){ return fileInfo.filename == file; });
    if (rem == syncTimes.end() || srem == serverFiles.end()) {
        return true;
    } else {
        unsigned long time = (unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(fileEntry.last_write_time()).time_since_epoch().count();
        auto sTime = rem.operator*().syncTime;
        auto ssTime = srem.operator*().lastModified;
        return (time > sTime && time > ssTime);
    }
}

void client::synchronizeFromServer() {
    ResponseMessage responseOK = receiveResponseFromServer();
    if (responseOK.code != RESPONSE_OK) {
        return;
    }
    sendCommandToServer(CMD_LIST_FILES, "all");
    ResponseMessage response = receiveResponseFromServer();

    if (response.code == RESPONSE_FILE_LIST) {
        parseFileList(response.data);
        auto localFileList = getLocalFileStringList();
        for (const auto& serverFile : serverFiles) {
            if (std::find(localFileList.begin(), localFileList.end(), serverFile.filename) == localFileList.end()) {
                // Súbor chýba lokálne, stiahni ho zo servera
                if (serverFile.deleted == 0) {
                    downloadFileFromServer(serverFile);
                }
            } else {
                // Súbor existuje lokálne, skontroluj či sa zmenil
                if (serverFile.deleted == 0) {
                    if (isFileUpdated(serverFile)) {
                        // Súbor sa zmenil, stiahni aktualizovanú verziu zo servera
                        downloadFileFromServer(serverFile);
                    }
                } else {
                    std::string input;
                    std::cout << "[Q] Do you want to delete " << serverFile.filename << " from your machine [y|n]: ";
                    std::getline(std::cin, input);
                    if (input == "y") {
                        auto res = deleteFile(serverFile.filename);
                        if (res) {
                            std::cout << "[I] File " << serverFile.filename << " was deleted" << std::endl;
                        } else {
                            std::cerr << "[E] File " << serverFile.filename << " was not deleted" << std::endl;
                        }
                    } else {
                        std::cout << "[I] File " << serverFile.filename << " was not deleted" << std::endl;
                    }
                }
            }
        }
    } else {
        std::cerr << "[E] Error retrieving file list from server" << std::endl;
    }
}
void client::synchronizeToServer() {
    auto localFileList = getLocalFileEntry();
    for (auto& localFile : localFileList) {
        std::string filename(localFile.path().filename());
        auto find = std::find_if(serverFiles.begin(), serverFiles.end(), [&filename](const FileInfo &fileInfo){ return fileInfo.filename == filename; });
        if (find == serverFiles.end()) {
            if (isFileNewOrUpdated(localFile)) {
                // Súbor je nový alebo zmenený, odosli ho na server
                uploadFileToServer(localFile);
            }
        } else {
            if (find.operator*().deleted == 1) {
                std::string input;
                std::cout << "[Q] Do you want to upload " << filename << " from your machine [y|n]: ";
                std::getline(std::cin, input);
                if (input == "y") {
                    auto res = uploadFileToServer(localFile);
                    if (res) {
                        std::cout << "[I] File " << filename << " was uploaded" << std::endl;
                    } else {
                        std::cerr << "[E] File " << filename << " was not uploaded" << std::endl;
                    }
                } else {
                    std::cout << "[I] File " << filename << " was not uploaded" << std::endl;
                }
            } else {
                if (isFileNewOrUpdated(localFile)) {
                    // Súbor je nový alebo zmenený, odosli ho na server
                    uploadFileToServer(localFile);
                }
            }
        }
    }
}

void client::autoSyncMethod() {
    while (autoSyncThreadRun) {
        while (autoSync) {
            std::unique_lock<std::mutex> lock(mutex);
            if (!syncRunning) {
                autoSyncRunning = true;
                sendCommandToServer(CMD_START_SYNC, "start");
                synchronizeFromServer();
                synchronizeToServer();
                sendCommandToServer(CMD_END_SYNC, "end");
                std::cout << "[I] Automatic synchronization complete" << std::endl;
                autoSyncRunning = false;
            }
            cv.wait_for(lock, std::chrono::seconds(autoSyncInterval));
            lock.unlock();
        }
    }
}
void client::syncMethod() {
    if (!autoSyncRunning) {
        syncRunning = true;
        std::cout << "[I] Synchronization in progress" << std::endl;
        sendCommandToServer(CMD_START_SYNC, "start");
        synchronizeFromServer();
        synchronizeToServer();
        sendCommandToServer(CMD_END_SYNC, "end");
        std::cout << "[I] Synchronization complete" << std::endl;
        syncRunning = false;
    } else {
        std::cout << "[I] Automatic synchronization is in progress, manual synchronization cannot be started" << std::endl;
    }
}

int client::mainMethod() {

    std::cout << "[Q] Enter server IP address: " << std::endl;
    std::string server;
    std::getline(std::cin, server);
    std::cout << "[Q] Enter server port number: " << std::endl;
    std::string input;
    std::getline(std::cin, input);
    int port = std::stoi(input);
    std::cout << "[I] The client will connect to [" << server << "] and use port " << port << std::endl;

    for (auto &entry : getLocalFileEntry()) {
        std::string fileName = entry.path().filename();
        unsigned long fileTime = (unsigned long)std::chrono::time_point_cast<std::chrono::milliseconds>(entry.last_write_time()).time_since_epoch().count();
        syncTimes.push_back({fileName, fileTime});
    }

    std::string userInput;
    thread = std::thread(&client::autoSyncMethod, this);


    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "[E] Failed to create socket" << std::endl;
        stopThread();
        return EXIT_FAILURE;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "[E] Invalid or unsupported address" << std::endl;
        stopThread();
        return EXIT_FAILURE;
    }

    if ((connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) < 0) {
        std::cerr << "[E] The connection to the server failed" << std::endl;
        stopThread();
        return EXIT_FAILURE;
    }

    while (true) {

        std::cout << "> ";
        std::getline(std::cin, userInput);
        std::vector<std::string> tokens = split(userInput);

        if (tokens.empty()) {
            continue;
        }

        std::string command = tokens[0];
        if (command == "exit") {
            std::cout << "[I] Turning off helper services" << std::endl;
            stopThread();
            sendCommandToServer(CMD_CLOSE, "close");
            close(clientSocket);
            return EXIT_SUCCESS;
        }

        std::vector<std::string> args(tokens.begin() + 1, tokens.end());
        processCommand(command, args);
    }
}
