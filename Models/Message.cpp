#pragma once
#include <string>
#include <iostream>
#include <chrono>

static std::string formatSqlTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    const std::time_t unixTime = std::chrono::system_clock::to_time_t(timePoint);
    const std::tm* timeStruct = std::localtime(&unixTime);
    std::ostringstream oss;
    oss << std::put_time(timeStruct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
struct Message {
    int RootId;
    int ChatID;
    std::string Role;
    std::string Content;
    std::chrono::system_clock::time_point Timestamp;

    Message(const int rootId, const int chatId, std::string role, std::string content = "", const std::chrono::system_clock::time_point& timestamp = std::chrono::system_clock::now()) :
    RootId(rootId), ChatID(chatId), Role(std::move(role)),Content(std::move(content)), Timestamp(timestamp) {  }

    void Print() const  {
        std::cout << "\t\t\t"<< formatSqlTimestamp(Timestamp) <<" - "<<  Role << ": "<< Content << std::endl;
    }
};