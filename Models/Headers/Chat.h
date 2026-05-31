#pragma once
#include "../Headers/Root_chat.h"

#ifndef BRANCHAT1_CHAT_H
#define BRANCHAT1_CHAT_H

struct Chat : RootChat{
    int parentId;
    int rootId;
    std::vector<Chat> branches;
    explicit Chat() = default;
    explicit Chat(const int _parentId, std::string _name, std::vector<Message>& _messages, const int _rootId, const int _id, const int _groupId, std::vector<Chat>& _branches) : RootChat(std::move(_name), _messages, _id, _groupId),
        parentId(_parentId),
        rootId(_rootId),
        branches(std::move(_branches)){}
    void PrintData() const override;
    void SendPrompt(const std::string& prompt) override;
    void CreateBranch(const std::string &newChatName) override;
    void FocusChat() override;
    void Delete() override;
};

#endif //BRANCHAT1_CHAT_H
