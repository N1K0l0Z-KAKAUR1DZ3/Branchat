#pragma once
#include "../../APIs/Headers/DBAPI.h"
#include "../../APIs/Headers/AIAPI.h"

#ifndef BRANCHAT1_SESSION_H
#define BRANCHAT1_SESSION_H
class Session {

public:
    inline static Base base;
    inline static Tree ChatTree;
    inline static std::unique_ptr<RootChat> chatPtr;

    Session();
    static void LoadChatTree(int rootId);
    static void FocusChat(int chatId);

};
#endif //BRANCHAT1_SESSION_H
