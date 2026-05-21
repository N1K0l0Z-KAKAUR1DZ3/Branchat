#include "../Headers/Session.h"


Session::Session() {
    DBAPI::init();
    base = DBAPI::GetBase();
}
void Session::LoadTree(const int rootId) {
    ChatTree.branchingChats = DBAPI::GetChatTree(rootId);
}
void Session::resetFocus() {
    chatPtr = nullptr;
    pointingAtRoot = false;
    currentContextPtr = nullptr;
    additionalContextPtr = nullptr;
}

Message Session::ReceiveAIResponse() {
    std::vector<Message> contextPayload;
    if (additionalContextPtr != nullptr) {
        contextPayload.insert(contextPayload.begin(), additionalContextPtr->begin(), additionalContextPtr->end());
    }
    contextPayload.insert(contextPayload.begin(), currentContextPtr->begin(), currentContextPtr->end());
    auto response = AIAPI::GetAIResponse(contextPayload);
    return response;
}

void Session::AddBranch(const int parentId, const int rootId,const int groupId, const std::string& name) {
    const auto newChat = DBAPI::SaveBranchingChat(name, parentId, rootId, groupId);
    ChatTree.branchingChats.emplace(newChat.id, newChat);
    ChatTree.branchingChats[newChat.id].FocusChat();
}
void Session::ReloadTree() {
    if (!pointingAtRoot) {
        const auto currentChatId = chatPtr->id;
        ChatTree.branchingChats = DBAPI::GetChatTree(ChatTree.rootPtr->id);
        if (!ChatTree.branchingChats.contains(currentChatId)) {
            chatPtr = nullptr;
        }
        chatPtr = &ChatTree.branchingChats[currentChatId];
        return;
    }
    ChatTree.branchingChats = DBAPI::GetChatTree(ChatTree.rootPtr->id);
}

RootChat Session::AddRoot(const int groupId, const std::string &name) {
    return DBAPI::SaveRootChat(groupId, name);
}
Group Session::AddGroup(const std::string &name) {
    return DBAPI::SaveGroup(name);
}

void Session::ReloadBase() {
    base = DBAPI::GetBase();
}

void Session::RenameChat(const int chatId, const std::string &newName) {
    DBAPI::UpdateChatName(chatId, newName);
}
void Session::RenameGroup(const int groupId, const std::string &newName) {
    DBAPI::UpdateGroupName(groupId, newName);
}
void Session::DeleteChat(const int chatId) {
    DBAPI::DeleteChat(chatId);
}
void Session::DeleteGroup(const int groupId) {
    DBAPI::DeleteGroup(groupId);
}
