#include "../Headers/Root_chat.h"
#include "../../Session/Session.h"

void RootChat::PrintData() const {
    std::cout << "\t"<< std::format("rootID: {}, name: {}, has children: {}", id, name, hasChildren)<< std::endl;
    PrintConversation();
}
void RootChat::PrintConversation() const {
    for (const auto& msg : messages) {
        msg.Print();
    }
}
void RootChat::FocusChat() {
    Session::pointingAtRoot = true;
    Session::activeChatId = id;
    Session::activeGroupId = groupId;
    Session::chatPtr = this;
}

void RootChat::SendPrompt(const std::string& prompt) {
    messages.push_back(Message(id, id, "user", prompt));
    DBAPI::SaveMessage(messages.back());
    messages.push_back(Message(id, id, "model", Session::ReceiveAIResponse()));
    DBAPI::SaveMessage(messages.back());
}

/*works*/void RootChat::CreateBranch(const std::string& newChatName) {
    if (Tree::rootId != id) {
        LoadTree();
    }
    Tree::topBranches.push_back(DBAPI::SaveBranchingChat(newChatName, id, id, groupId));
    hasChildren = true;
    Tree::topBranches.back().FocusChat();
}
/*works*/void RootChat::LoadTree() {
    Tree::SetTree(DBAPI::GetChatTree(id));
    Tree::rootId = id;
}
/*works*/ void RootChat::Delete() {
    DBAPI::DeleteChat(id);
    Session::ReloadBase();
}

/*works*/void RootChat::Rename(const std::string &newName) {
    std::cout << "renaming chat" << std::endl;
    DBAPI::UpdateChatName(id, newName);
    name = newName;
}

