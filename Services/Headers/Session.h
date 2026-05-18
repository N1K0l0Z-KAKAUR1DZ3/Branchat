#pragma once
#include "../../APIs/Headers/DBAPI.h"

#ifndef BRANCHAT1_SESSION_H
#define BRANCHAT1_SESSION_H
class Session {
public:
    static Base base;
    static Tree ChatTree;
    static std::unique_ptr<RootChat> chatPtr;
    static int Conte
    Session();
    static void LoadChatTree(const int rootId);
    static void FocusChat(const int chatId);
    static void
};
#endif //BRANCHAT1_SESSION_H
