#pragma once
#include "../../APIs/Headers/DBAPI.h"
#include "../../APIs/Headers/AIAPI.h"

#ifndef BRANCHAT1_SESSION_H
#define BRANCHAT1_SESSION_H
class Session {
    static void SaveMessage();
public:
    inline static Base base;
    inline static Tree ChatTree;
    inline static RootChat* chatPtr;
    inline static bool pointingAtRoot;
    inline static std::vector<Message>* additionalContextPtr;
    inline static std::vector<Message>* currentContextPtr;

    Session();

    static void ReloadBase();
    static void ReloadTree();

    static void LoadTree(int rootId);
    static void resetFocus();
    static Message ReceiveAIResponse();

    static void AddBranch(int parentId, int rootId, int groupId, const std::string& name);
    static RootChat AddRoot(int groupId, const std::string& name);
    static Group AddGroup(const std::string& name);

    static void DeleteGroup(int groupId);
    static void DeleteChat(int chatId);

    static void RenameChat(int chatId, const std::string& newName);
    static void RenameGroup(int groupId, const std::string& newName);
};
#endif //BRANCHAT1_SESSION_H
