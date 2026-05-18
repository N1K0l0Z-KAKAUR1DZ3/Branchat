#pragma once
#include "../../Models/Headers/Base.h"
#include "../../Models/Headers/Tree.h"
#include "../../Dependencies/nanodbc/nanodbc.h"
#include <fstream>
#include <algorithm>

#ifndef BRANCHAT1_DBAPI_H
#define BRANCHAT1_DBAPI_H
class DBAPI {
    inline static std::unique_ptr<nanodbc::connection> dbConnection;

    static std::chrono::system_clock::time_point parseSqlTimestamp(const std::string& timeStr);
    static std::string formatSqlTimestamp(const std::chrono::system_clock::time_point& timePoint);
    static std::vector<Message> GetTreeMessages(int RootId);
    static std::vector<Message> ExtractChatMessages(const std::vector<Message>& allMessages, int ChatId);
    static std::vector<RootChat> ExtractGroupRoots(const std::vector<RootChat>& allRoots, int GroupId);
    static int GetBranchCount(int rootId);
    static std::vector<RootChat> GetRootS();
public:
    static void init();
    // read
    static Base GetBase();
    static Tree GetChatTree(int rootId);
    // create
    static Group SaveGroup(const std::string& name);
    static RootChat SaveRootChat(int groupId, const std::string& name);
    static Chat SaveBranchingChat(const std::string& name, int parentId, int rootId, int groupId);
    static void SaveMessage(const Message& msg);
    // update
    static void UpdateGroupName( int GroupId, const std::string& newName);
    static void UpdateChatName(int chatId, const std::string& newName);
    // delete
    static void DeleteChat(int chatId);
    static void DeleteGroup(int groupId);
};
#endif //BRANCHAT1_DBAPI_H
