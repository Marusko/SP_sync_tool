#ifndef SYNK_MESSAGES_H
#define SYNK_MESSAGES_H
#include "json.hpp"

using json = nlohmann::json;
enum CommandCode {
    CMD_LIST_FILES,
    CMD_START_SYNC,
    CMD_END_SYNC,
    CMD_DOWNLOAD_FILE,
    CMD_UPLOAD_FILE,
    CMD_DELETE_FILE,
    CMD_MARK_DELETE_FILE,
    CMD_CLOSE
};

enum ResponseCode {
    RESPONSE_OK,
    RESPONSE_ERROR,
    RESPONSE_FILE_LIST
};

struct CommandMessage {
    CommandCode code;
    std::string argument;

    std::string serialize() const {
        json j;
        j["code"] = static_cast<int>(code);
        j["argument"] = argument;
        return j.dump();
    }

    // Deserializácia z formátu JSON
    static CommandMessage deserialize(const std::string& serialized) {
        json j = json::parse(serialized);
        CommandMessage command;
        command.code = static_cast<CommandCode>(j["code"].get<int>());
        command.argument = j["argument"].get<std::string>();
        return command;
    }
};

struct ResponseMessage {
    ResponseCode code;
    std::string data;

    // Serializácia do formátu JSON
    std::string serialize() const {
        json j;
        j["code"] = static_cast<int>(code);
        j["data"] = data;
        return j.dump();
    }

    // Deserializácia z formátu JSON
    static ResponseMessage deserialize(const std::string& serialized) {
        json j = json::parse(serialized);
        ResponseMessage response;
        response.code = static_cast<ResponseCode>(j["code"].get<int>());
        response.data = j["data"].get<std::string>();
        return response;
    }
};

struct FileInfo {
    std::string filename;
    int deleted = 0;
    unsigned long lastModified;  // Timestamp of the last modification
};

struct FileSync {
    std::string filename;
    unsigned long syncTime;
};

#endif //SYNK_MESSAGES_H
