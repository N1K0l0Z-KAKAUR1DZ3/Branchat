#pragma once
#include <iostream>
#include <format>
#include <vector>

struct Chat : RootChat{
    int parentId;
    int rootId;
    explicit Chat() = default;
    explicit Chat(const int _parentId, std::string _name, std::vector<Message>& _messages, const int _rootId, const int _id, const int _groupId) : RootChat(std::move(_name), _messages, _id, _groupId),
    parentId(_parentId),
    rootId(_rootId){}

    void PrintData() const override {
        std::cout<< "\t\t"<< std::format("rootID: {}, parentId: {}, id: {}, name: {}",rootId, parentId, id, name)<< std::endl;
        PrintConversation();
    }
};