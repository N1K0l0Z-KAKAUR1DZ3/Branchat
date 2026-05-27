#include "../Headers/DBAPI.h"
#include <cstdlib>
std::chrono::system_clock::time_point DBAPI::parseSqlTimestamp(const std::string& timeStr) {
    std::tm timeStruct = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&timeStruct, "%Y-%m-%d %H:%M:%S");
    const std::time_t unixTime = std::mktime(&timeStruct);
    return std::chrono::system_clock::from_time_t(unixTime);
}
 std::string DBAPI::formatSqlTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    const std::time_t unixTime = std::chrono::system_clock::to_time_t(timePoint);
    const std::tm* timeStruct = std::localtime(&unixTime);
    std::ostringstream oss;
    oss << std::put_time(timeStruct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
std::vector<Message> DBAPI::GetBaseMessages() {
    // std::cout << "fetching messages" << "\n";
    std::vector<Message> baseMessages;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetBaseMessages()}");
    nanodbc::result result = nanodbc::execute(stmt);

    while (result.next()) {
        // std::cout << "building message" << std::endl;
        const auto rootId = result.get<int>("rootId");
        const auto chatId = result.get<int>("chatId");
        const auto role = result.get<std::string>("role");
        const auto timesStamp = parseSqlTimestamp(result.get<std::string>("timestamp"));
        const auto content = result.get<std::string>("content");

        auto currentMsg = Message(rootId, chatId, role, content, timesStamp);
        baseMessages.push_back(currentMsg);
    }
    // std::cout << "fetched messages" << "\n";
    return baseMessages;
}
 std::vector<Message> DBAPI::GetTreeMessages(const int RootId) {
    // std::cout << "fetching messages" << "\n";
    std::vector<Message> treeMessages;
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.GetTreeMessages(?)}");
        stmt.bind(0, &RootId);
        nanodbc::result result = nanodbc::execute(stmt);

        while (result.next()) {
            // std::cout << "building message" << std::endl;
            const auto rootId = result.get<int>("rootId");
            const auto chatId = result.get<int>("chatId");
            const auto role = result.get<std::string>("role");
            const auto timesStamp = parseSqlTimestamp(result.get<std::string>("timestamp"));
            const auto content = result.get<std::string>("content");

            auto currentMsg = Message(rootId, chatId, role, content, timesStamp);
            treeMessages.push_back(currentMsg);
        }
    // std::cout << "fetched messages" << "\n";
    return treeMessages;
}
 std::vector<Message> DBAPI::ExtractChatMessages(const std::vector<Message>& allMessages, const int ChatId) {
    // std::cout << std::format("Fetching messages of chat with id {}", ChatId) << "\n";
    auto [first, last] = std::ranges::equal_range(allMessages, ChatId, {}, &Message::ChatID);
    // std::cout << std::format("Fetched messages", ChatId) << "\n";
    return std::vector<Message>(first, last);
}
 std::vector<RootChat> DBAPI::ExtractGroupRoots(const std::vector<RootChat>& allRoots, const int GroupId) {
    // std::cout << std::format("Fetching roots of chat group id {}", GroupId) << "\n";
    auto [first, last] = std::ranges::equal_range(allRoots, GroupId, {}, &RootChat::groupId);
    // std::cout << std::format("Fetched messages", GroupId) << "\n";
    return std::vector<RootChat>(first, last);
}
 int DBAPI::GetBranchCount(const int rootId) {
    // std::cout << std::format("Fetching branch count of root {}", rootId) << "\n";
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.GetBranchCount(?)}");
        stmt.bind(0, &rootId);
        nanodbc::result result = nanodbc::execute(stmt);
        result.next();
        // std::cout << "fetched count" << "\n";
        return result.get<int>("count");
}
 std::vector<RootChat> DBAPI::GetRootS() {
    // std::cout << "fetching all roots" << std::endl;
    std::vector<RootChat> roots;
    std::vector<Message> baseMessages = GetBaseMessages();
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetRoots()}");
    nanodbc::result result = nanodbc::execute(stmt);
    while (result.next()) {
        const auto rootId =  result.get<int>("id");
        auto msgs = ExtractChatMessages(baseMessages, rootId);
        auto newRoot = RootChat(result.get<std::string>("name"),msgs, rootId, result.get<int>("groupId"));
        roots.push_back(newRoot);
    }
    // std::cout << " fetched all roots" << std::endl;
    return roots;
}
 void DBAPI::init() {
        auto constring = std::getenv("CON_STRING");
        dbConnection = std::make_unique<nanodbc::connection>(constring);
        if (!dbConnection->connected()) {
            throw std::runtime_error("Connection object created but not connected.");
        }
        // std::cout << "Connected to Branchat successfully!" << std::endl;
}

 std::vector<Group> DBAPI::GetBaseGroups() {
    // std::cout << "fetching all groups" << std::endl;
    auto roots = GetRootS();
    std::vector<Group> groups;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetGroups()}");
    nanodbc::result result = nanodbc::execute(stmt);
    while (result.next()) {
        const int currentGroupId = result.get<int>("id");
        auto currentGroupsRoots = ExtractGroupRoots(roots, currentGroupId);
        auto newGroup = Group(currentGroupId, result.get<std::string>("name"), currentGroupsRoots);
        groups.push_back(newGroup);
    }
    // std::cout << " fetched all groups" << std::endl;
    return groups;
}
 std::unordered_map<int, Chat> DBAPI::GetChatTree (const int rootId) {
    // std::cout << "fetching Tree of root " << rootId << "\n";
    const int treeChatCount = GetBranchCount(rootId);
    if (treeChatCount == 0) {
        std::cout << "tree with root " << rootId << " does not exist";
        throw std::invalid_argument(std::format("tree with the root if of {} does not exist", rootId));
    }

    std::unordered_map<int, Chat> chats;
    std::vector<Message> treeMessages = GetTreeMessages(rootId);
    chats.reserve(treeChatCount - 1);
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.GetChatTree(?)}");
        stmt.bind(0, &rootId);
        nanodbc::result result = nanodbc::execute(stmt);

        while (result.next()) {
            const int chatId = result.get<int>("id");
            auto chatMessages = ExtractChatMessages(treeMessages, chatId);
            chats.emplace(chatId, Chat(result.get<int>("parentId"),
                                                              result.get<std::string>("name"),
                                                                chatMessages,
                                                             result.get<int>("rootId"),
                                                                chatId,
                                                                result.get<int>("groupId")));
        }
        // std::cout << "fetched Tree with chatcount = " << chats.size()<< "\n";
        return chats;
}
//
  Group DBAPI::SaveGroup(const std::string& name) {
    // std::cout << "saving group" <<std::endl;
    int newId = -1;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{? = CALL Branchat.SaveGroup(?)}");
    stmt.bind(0, &newId, 1, nanodbc::statement::PARAM_RETURN);
    stmt.bind(1, name.c_str());
    nanodbc::execute(stmt);
    // std::cout << "saved Group" <<std::endl;
    std::vector<RootChat> newRoots = {};
    return Group(newId, name, newRoots);
}
  RootChat DBAPI::SaveRootChat(int groupId, const std::string& name) {
    // std::cout << "saving root chat" <<std::endl;
    int newId = -1;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{? = CALL Branchat.SaveChat(?, ?, ?, ?)}");
    stmt.bind(0, &newId, 1, nanodbc::statement::PARAM_RETURN);
    stmt.bind(1, name.c_str());
    stmt.bind_null(2);
    stmt.bind_null(3);
    stmt.bind(4, &groupId);
    nanodbc::execute(stmt);
    std::vector<Message> msgs = {};
    // std::cout << "saved root Chat" <<std::endl;
    return RootChat(name, msgs, newId, groupId);
}
  Chat DBAPI::SaveBranchingChat(const std::string& name, const int parentId, const  int rootId, const int groupId) {
    // std::cout << "saving branching chat" <<std::endl;
    int newId = -1;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{? = CALL Branchat.SaveChat(?, ?, ?, ?)}");
    stmt.bind(0, &newId, 1, nanodbc::statement::PARAM_RETURN);
    stmt.bind(1, name.c_str());
    stmt.bind(2, &parentId);
    stmt.bind(3, &rootId);
    stmt.bind(4, &groupId);
    nanodbc::execute(stmt);
    std::vector<Message> msgs = {};
    // std::cout << "saved branching Chat" <<std::endl;
    return Chat(parentId, name, msgs, rootId, newId, groupId);
}
  void DBAPI::SaveMessage(const Message& msg) {
    const std::string timeStr = formatSqlTimestamp(msg.Timestamp);
    // std::cout << "Saving message" << std::endl;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.SaveMessage(?, ?, ?, ?, ?)}");
    stmt.bind(0, &msg.RootId);
    stmt.bind(1, &msg.ChatID);
    stmt.bind(2, msg.Role.c_str());
    stmt.bind(4, timeStr.c_str());
    std::wstring wideName(msg.Content.begin(), msg.Content.end());
    stmt.bind(3, msg.Content.c_str());
    nanodbc::execute(stmt);
    // std::cout << "Saved message" << std::endl;
}

  void DBAPI::UpdateGroupName(const int GroupId,  const std::string & newName) {
    // std::cout << "updating group name" <<std::endl;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.UpdateGroup(?, ?)}");
    stmt.bind(0, &GroupId);
    stmt.bind(1, newName.c_str());
    nanodbc::execute(stmt);
    // std::cout << "updated Group name to: " << newName <<std::endl;
}
  void DBAPI::UpdateChatName(const int chatId,  const std::string & newName) {
    // std::cout << "updating chat name" <<std::endl;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.UpdateChat(?, ?)}");
    stmt.bind(0, &chatId);
    stmt.bind(1, newName.c_str());
    nanodbc::execute(stmt);
    // std::cout << "updated chat name to: " << newName <<std::endl;
}

  void DBAPI::DeleteChat(const int chatId) {
    // std::cout << "Deleting tree starting at id " << chatId << std::endl;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.DeleteChatBranch(?)}");
    stmt.bind(0, &chatId);
    nanodbc::execute(stmt);
    // std::cout << "Delete success" << std::endl;
}
  void DBAPI::DeleteGroup(const int groupId) {
    // std::cout << "Deleting group with id " << groupId << std::endl;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.DeleteGroup(?)}");
    stmt.bind(0, &groupId);
    nanodbc::execute(stmt);
    // std::cout << "Delete success" << std::endl;
}