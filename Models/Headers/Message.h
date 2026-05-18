#pragma once
#include <chrono>
#include <iostream>

#ifndef BRANCHAT1_MESSAGE_H
#define BRANCHAT1_MESSAGE_H

struct Message {
    int RootId;
    int ChatID;
    std::string Role;
    std::string Content;
    std::chrono::system_clock::time_point Timestamp;

    Message(const int rootId, const int chatId, std::string role, std::string content = "", const std::chrono::system_clock::time_point& timestamp = std::chrono::system_clock::now()) :
        RootId(rootId),
        ChatID(chatId),
        Role(std::move(role)),
        Content(std::move(content)),
        Timestamp(timestamp){}
    void Print() const;
};

#endif
