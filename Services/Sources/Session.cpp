#include "../Headers/Session.h"


Session::Session() {
    DBAPI::init();
    AIAPI::init();
    Base::groups = DBAPI::GetBaseGroups();
}

void Session::rehookChatPtr() {
    RootChat* ptr = Tree::FindChatById(activeChatId);
    if (ptr == nullptr) {
        chatPtr = &Base::FindGroup(activeGroupId).FindRootChat(activeChatId);
        return;
    }
    chatPtr = ptr;
}

void Session::LoadTree(const int rootId) {
    Tree::branchingChats = DBAPI::GetChatTree(rootId);

}
void Session::resetFocus() {
    activeChatId = -1;
    activeGroupId = -1;
    pointingAtRoot = false;
    chatPtr = nullptr;
    currentContextPtr = nullptr;
    additionalContextPtr = nullptr;
}

std::string Session::ReceiveAIResponse() {
    std::vector<Message> contextPayload;
    if (additionalContextPtr != nullptr) {
        contextPayload.insert(contextPayload.end(), additionalContextPtr->begin(), additionalContextPtr->end());
    }
    contextPayload.insert(contextPayload.end(), currentContextPtr->begin(), currentContextPtr->end());
    // for (auto& msg : contextPayload) {
    //     std::cout <<msg.Role<< ": " <<msg.Content << std::endl;
    // }
    auto response = AIAPI::GetAIResponse(contextPayload);
    return response;
}

void Session::AddBranch(const int parentId, const int rootId,const int groupId, const std::string& name) {
    const auto newChat = DBAPI::SaveBranchingChat(name, parentId, rootId, groupId);
    Tree::branchingChats.emplace(newChat.id, newChat);
    Tree::branchingChats[newChat.id].FocusChat();
}
void Session::ReloadTree() {
    Tree::branchingChats = DBAPI::GetChatTree(Tree::rootPtr->id);
    if (!pointingAtRoot) {
        if (!Tree::branchingChats.contains(activeChatId)) {
            Session::resetFocus();
            return;
        }
        Session::rehookChatPtr();
    }
}
-
void Session::SaveMessage(const Message& msg) {
    DBAPI::SaveMessage(msg);
}

RootChat Session::AddRoot(const int groupId, const std::string &name) {
    return DBAPI::SaveRootChat(groupId, name);
}
Group Session::AddGroup(const std::string &name) {
    return DBAPI::SaveGroup(name);
}

void Session::ReloadBase() {
    Base::groups = DBAPI::GetBaseGroups();
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
