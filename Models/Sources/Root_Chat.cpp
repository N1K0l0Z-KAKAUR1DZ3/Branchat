#include "../Headers/Root_chat.h"
#include "../../Services/Headers/Session.h"

void RootChat::PrintData() const {
    std::cout << "\t"<< std::format("rootID: {}, name: {}", id, name)<< std::endl;
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
    Tree::rootPtr = this;
    Session::additionalContextPtr = nullptr;
    Session::currentContextPtr = &messages;
    Session::rehookChatPtr();
}
RootChat::~RootChat() {
    if (Session::chatPtr == this) {
        Session::resetFocus();
    }
    if (Tree::rootPtr == this) {
        Tree::Clear();
    }
}

void RootChat::SendPrompt(const std::string& prompt) {
    messages.push_back(Message(id, id, "user", prompt));
    Session::SaveMessage(messages.back());
    messages.push_back(Message(id, id, "model", Session::ReceiveAIResponse()));
    Session::SaveMessage(messages.back());
}

void RootChat::CreateBranch(const std::string& newChatName) {
    Session::AddBranch(id, id, groupId, newChatName);
}
void RootChat::LoadTree() {
    Session::LoadTree(id);
    Tree::rootPtr = this;
}
void RootChat::Delete() {
    Session::DeleteChat(id);
    Base::FindGroup(groupId).DeleteRootChat(id);
    Session::ReloadBase();
}
void RootChat::Rename(const std::string &newName) {
    Session::RenameChat(id, newName);
    name = newName;
}

