#pragma once
#include "../Headers/Message.h"

#ifndef BRANCHAT1_ROOT_CHAT_H
#define BRANCHAT1_ROOT_CHAT_H

struct RootChat {
    std::string name;
    std::vector<Message> messages;
    int id;
    int groupId;
    explicit RootChat(std::string _name, std::vector<Message>& _messages , const int _id,const int _groupId) :
        name(std::move(_name)),
        messages(std::move(_messages)),
        id(_id),
        groupId(_groupId){}
    explicit RootChat() = default;
    virtual ~RootChat() = default;
protected:
    void PrintConversation() const;
public:
    virtual void PrintData() const;
    virtual void FocusChat();
    void CreateBranch();
};

#endif //BRANCHAT1_ROOT_CHAT_H
