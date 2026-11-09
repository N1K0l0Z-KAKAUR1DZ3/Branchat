#include "../Headers/DBAPI.h"
#include "Session.h"
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

std::vector<Message> DBAPI::ReadMessages(nanodbc::result& result) {
    std::vector<Message> messages;
    while (result.next()) {
        // Read columns in SELECT order: Text is a MAX column and must come last.
        const auto id = result.get<int>("ID");
        const auto Root_ID = result.get<int>("Root_ID");
        const auto chatId = result.get<int>("Chat_ID");
        const auto role = result.get<std::string>("Role");
        const auto timesStamp = parseSqlTimestamp(result.get<std::string>("Timestamp"));
        const auto content = result.get<std::string>("Text", "");

        std::cout << "[LOG] Read Message -> ID: " << id
                  << ", Root_ID: " << Root_ID
                  << ", Chat_ID: " << chatId
                  << ", Role: " << role
                  << ", Text: " << content << "\n";

        auto currentMsg = Message(Root_ID, chatId, role, content, timesStamp);
        currentMsg.Id = id;
        messages.push_back(std::move(currentMsg));
    }
    return messages;
}

void DBAPI::ReadAttachments(nanodbc::result& result, std::vector<Message>& messages) {
    std::unordered_map<int, Message*> byId;
    for (auto& msg : messages) byId[msg.Id] = &msg;

    int count = 0;
    while (result.next()) {
        // Read columns in SELECT order: Data is a MAX column and must come last.
        const auto messageId = result.get<int>("Message_ID");
        Attachment attachment;
        attachment.id = result.get<int>("ID");
        attachment.mimeType = result.get<std::string>("Mime_Type");
        attachment.fileName = result.get<std::string>("File_Name", "");
        attachment.data = std::make_shared<const Attachment::Bytes>(
            result.get<std::vector<std::uint8_t>>("Data"));

        auto it = byId.find(messageId);
        if (it != byId.end()) {
            it->second->Attachments.push_back(std::move(attachment));
            ++count;
        }
    }
    std::cout << "[LOG] Read " << count << " attachment(s)\n";
}

std::vector<Message> DBAPI::GetBaseMessages() {
    std::cout << "fetching messages" << "\n";
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetBaseMessages()}");
    nanodbc::result result = nanodbc::execute(stmt);
    std::vector<Message> baseMessages = ReadMessages(result);

    nanodbc::statement attStmt(*dbConnection);
    nanodbc::prepare(attStmt, "{CALL Branchat.GetBaseAttachments()}");
    nanodbc::result attResult = nanodbc::execute(attStmt);
    ReadAttachments(attResult, baseMessages);

    std::cout << "fetched messages" << "\n";
    return baseMessages;
}

std::vector<Message> DBAPI::GetTreeMessages(const int RootId) {
    std::cout << "fetching messages" << "\n";
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetTreeMessages(?)}");
    stmt.bind(0, &RootId);
    nanodbc::result result = nanodbc::execute(stmt);
    std::vector<Message> treeMessages = ReadMessages(result);

    nanodbc::statement attStmt(*dbConnection);
    nanodbc::prepare(attStmt, "{CALL Branchat.GetTreeAttachments(?)}");
    attStmt.bind(0, &RootId);
    nanodbc::result attResult = nanodbc::execute(attStmt);
    ReadAttachments(attResult, treeMessages);

    std::cout << "fetched messages" << "\n";
    return treeMessages;
}

std::vector<Message> DBAPI::ExtractChatMessages(const std::vector<Message>& allMessages, const int ChatId) {
    auto [first, last] = std::ranges::equal_range(allMessages, ChatId, {}, &Message::ChatID);
    return std::vector<Message>(first, last);
}

std::vector<RootChat> DBAPI::ExtractGroupRoots(const std::vector<RootChat>& allRoots, const int GroupId) {
    auto [first, last] = std::ranges::equal_range(allRoots, GroupId, {}, &RootChat::groupId);
    return std::vector<RootChat>(first, last);
}

std::vector<RootChat> DBAPI::GetRootS() {
    std::cout << "fetching all roots" << std::endl;
    std::vector<RootChat> roots;
    std::vector<Message> baseMessages = GetBaseMessages();
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetRoots()}");
    nanodbc::result result = nanodbc::execute(stmt);

    while (result.next()) {
        const auto Root_ID = result.get<int>("ID");
        auto name = result.get<std::string>("Name");
        int groupId = result.get<int>("Group_ID");
        int branchCount = result.get<int>("branchCount");

        std::cout << "[LOG] Read RootChat -> ID: " << Root_ID
                  << ", Name: " << name
                  << ", Group_ID: " << groupId
                  << ", branchCount: " << branchCount << "\n";

        auto msgs = ExtractChatMessages(baseMessages, Root_ID);
        auto newRoot = RootChat(name, msgs, Root_ID, groupId);
        newRoot.hasChildren = branchCount > 0;
        roots.push_back(newRoot);
    }
    std::cout << "fetched all roots" << std::endl;
    return roots;
}

void DBAPI::init() {
    std::cout << "Connecting to Branchat " << std::endl;
    const char* envCon = std::getenv("CON_STRING");
    if (envCon == nullptr || envCon[0] == '\0') {
        throw std::runtime_error("CON_STRING is not set. Set it to your SQL Server ODBC connection string before starting the app.");
    }
    std::string constring = envCon;
    dbConnection = std::make_unique<nanodbc::connection>(constring);
    if (!dbConnection->connected()) {
        throw std::runtime_error("Connection object created but not connected.");
    }
    std::cout << "Connected to Branchat successfully!" << std::endl;
}

std::vector<Group> DBAPI::GetBaseGroups() {
    std::cout << "fetching all groups" << std::endl;
    auto roots = GetRootS();
    std::vector<Group> groups;
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetGroups()}");
    nanodbc::result result = nanodbc::execute(stmt);

    while (result.next()) {
        const int currentGroupId = result.get<int>("ID");
        std::string groupName = result.get<std::string>("name");

        std::cout << "[LOG] Read Group -> ID: " << currentGroupId
                  << ", Name: " << groupName << "\n";

        auto currentGroupsRoots = ExtractGroupRoots(roots, currentGroupId);
        auto newGroup = Group(currentGroupId, groupName, currentGroupsRoots);
        groups.push_back(newGroup);
    }
    std::cout << " fetched all groups" << std::endl;
    return groups;
}

std::unordered_map<int, Chat> DBAPI::GetChatTree(const int Root_ID) {
    std::unordered_map<int, Chat> chats;
    std::vector<Message> treeMessages = GetTreeMessages(Root_ID);
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{CALL Branchat.GetChatTree(?)}");
    stmt.bind(0, &Root_ID);
    nanodbc::result result = nanodbc::execute(stmt);

    while (result.next()) {
        const int chatId = result.get<int>("ID");
        int parentId = result.get<int>("Parent_ID");
        std::string chatName = result.get<std::string>("Name");
        int rootId = result.get<int>("Root_ID");
        int groupId = result.get<int>("Group_ID");

        std::cout << "[LOG] Read Chat Tree -> ID: " << chatId
                  << ", Parent_ID: " << parentId
                  << ", Name: " << chatName
                  << ", Root_ID: " << rootId
                  << ", Group_ID: " << groupId << "\n";

        auto chatMessages = ExtractChatMessages(treeMessages, chatId);
        std::vector<Chat> branches = {};
        chats.emplace(chatId, Chat(parentId, chatName, chatMessages, rootId, chatId, groupId, branches));
    }
    return chats;
}

Group DBAPI::SaveGroup(const std::string& Name) {
    try {
        int newId = -1;
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{? = CALL Branchat.SaveGroup(?)}");
        stmt.bind(0, &newId, 1, nanodbc::statement::PARAM_RETURN);
        stmt.bind(1, Name.c_str());
        nanodbc::execute(stmt);
        std::vector<RootChat> roots = {};
        return Group(newId, Name, roots);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to save group.");
        std::cerr << "[ERROR] SaveGroup: " << e.what() << std::endl;
        throw;
    }
}

RootChat DBAPI::SaveRootChat(int Group_ID, const std::string& Name) {
    try {
        int newId = -1;
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{? = CALL Branchat.SaveChat(?, ?, ?, ?)}");
        stmt.bind(0, &newId, 1, nanodbc::statement::PARAM_RETURN);
        stmt.bind(1, Name.c_str());
        stmt.bind_null(2);
        stmt.bind_null(3);
        stmt.bind(4, &Group_ID);
        nanodbc::execute(stmt);
        std::vector<Message> msgs = {};
        return RootChat(Name, msgs, newId, Group_ID);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to save root chat.");
        std::cerr << "[ERROR] SaveRootChat: " << e.what() << std::endl;
        throw;
    }
}

Chat DBAPI::SaveBranchingChat(const std::string& name, const int parentId, const int rootId, const int groupId) {
    try {
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
        std::vector<Chat> chats = {};
        return Chat(parentId, name, msgs, rootId, newId, groupId, chats);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to save branch.");
        std::cerr << "[ERROR] SaveBranchingChat: " << e.what() << std::endl;
        throw;
    }
}

int DBAPI::SaveAttachment(const int messageId, const Attachment& attachment) {
    int newId = -1;
    // nanodbc binds binary parameters as a batch, so wrap the single blob in a vector
    const std::vector<std::vector<std::uint8_t>> blob{*attachment.data};
    nanodbc::statement stmt(*dbConnection);
    nanodbc::prepare(stmt, "{? = CALL Branchat.SaveAttachment(?, ?, ?, ?)}");
    stmt.bind(0, &newId, 1, nanodbc::statement::PARAM_RETURN);
    stmt.bind(1, &messageId);
    stmt.bind(2, attachment.mimeType.c_str());
    stmt.bind(3, attachment.fileName.c_str());
    stmt.bind(4, blob);
    nanodbc::execute(stmt);
    return newId;
}

void DBAPI::SaveMessage(Message& msg) {
    try {
        const std::string timeStr = formatSqlTimestamp(msg.Timestamp);
        // Message and attachments are saved together, or not at all
        nanodbc::transaction transaction(*dbConnection);

        int newId = -1;
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{? = CALL Branchat.SaveMessage(?, ?, ?, ?, ?)}");
        stmt.bind(0, &newId, 1, nanodbc::statement::PARAM_RETURN);
        stmt.bind(1, &msg.RootId);
        stmt.bind(2, &msg.ChatID);
        stmt.bind(3, msg.Role.c_str());
        stmt.bind(4, msg.Content.c_str());
        stmt.bind(5, timeStr.c_str());
        nanodbc::execute(stmt);
        if (newId <= 0) {
            throw std::runtime_error("SaveMessage returned no ID. Has Database/001_add_image_attachments.sql been run?");
        }

        std::vector<int> attachmentIds;
        for (const auto& attachment : msg.Attachments) {
            attachmentIds.push_back(SaveAttachment(newId, attachment));
        }
        transaction.commit();

        msg.Id = newId;
        for (size_t i = 0; i < attachmentIds.size(); ++i) {
            msg.Attachments[i].id = attachmentIds[i];
        }
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to save message.");
        std::cerr << "[ERROR] SaveMessage: " << e.what() << std::endl;
        throw;
    }
}

void DBAPI::UpdateGroupName(const int GroupId, const std::string& newName) {
    try {
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.UpdateGroup(?, ?)}");
        stmt.bind(0, &GroupId);
        stmt.bind(1, newName.c_str());
        nanodbc::execute(stmt);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to update group Name.");
        std::cerr << "[ERROR] UpdateGroupName: " << e.what() << std::endl;
        throw;
    }
}

void DBAPI::UpdateChatName(const int chatId, const std::string& newName) {
    try {
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.UpdateChat(?, ?)}");
        stmt.bind(0, &chatId);
        stmt.bind(1, newName.c_str());
        nanodbc::execute(stmt);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to update chat Name.");
        std::cerr << "[ERROR] UpdateChatName: " << e.what() << std::endl;
        throw;
    }
}

void DBAPI::UpdateMessageText(const int messageId, const std::string& newText) {
    try {
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.UpdateMessage(?, ?)}");
        stmt.bind(0, &messageId);
        stmt.bind(1, newText.c_str());
        nanodbc::execute(stmt);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to update message.");
        std::cerr << "[ERROR] UpdateMessageText: " << e.what() << std::endl;
        throw;
    }
}

void DBAPI::DeleteMessage(const int messageId) {
    try {
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.DeleteMessage(?)}");
        stmt.bind(0, &messageId);
        nanodbc::execute(stmt);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to delete message.");
        std::cerr << "[ERROR] DeleteMessage: " << e.what() << std::endl;
        throw;
    }
}

void DBAPI::DeleteChat(const int chatId) {
    try {
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.DeleteChatBranch(?)}");
        stmt.bind(0, &chatId);
        nanodbc::execute(stmt);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to delete chat.");
        std::cerr << "[ERROR] DeleteChat: " << e.what() << std::endl;
        throw;
    }
}

void DBAPI::DeleteGroup(const int Group_ID) {
    try {
        nanodbc::statement stmt(*dbConnection);
        nanodbc::prepare(stmt, "{CALL Branchat.DeleteGroup(?)}");
        stmt.bind(0, &Group_ID);
        nanodbc::execute(stmt);
    } catch (const std::exception& e) {
        Session::SetError("DBAPI: Failed to delete group.");
        std::cerr << "[ERROR] DeleteGroup: " << e.what() << std::endl;
        throw;
    }
}