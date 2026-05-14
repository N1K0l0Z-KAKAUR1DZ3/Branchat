#pragma once
#include <string>
#include <vector>
#include <iostream>

struct RootChat {
    std::string name;
    std::vector<Message> messages;
    int id;
    int groupId;
    explicit RootChat(std::string _name, std::vector<Message>& _messages , const int _id,const int _groupId) : name(std::move(_name)), messages(std::move(_messages)), id(_id), groupId(_groupId){}
    explicit RootChat() = default;
    virtual ~RootChat() = default;
protected:
    void PrintConversation() const {
        for (const auto& msg : messages) {
            msg.Print();
        }
    }
public:
    virtual void PrintData() const {
        std::cout << "\t"<< std::format("rootID: {}, name: {}", id, name)<< std::endl;
        PrintConversation();
    }
};