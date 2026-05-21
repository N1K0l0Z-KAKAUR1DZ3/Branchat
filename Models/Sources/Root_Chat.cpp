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
    Session::chatPtr = this;
    Session::ChatTree.rootPtr = this;
}
RootChat::~RootChat() {
    if (Session::chatPtr == this) {
        Session::resetFocus();
    }
    if (Session::ChatTree.rootPtr == this) {
        Session::ChatTree.Clear();
    }
}

// void RootChat::SendPrompt(const std::string& prompt) {
//     const auto promptMsg = Message(id, id, "User", prompt);
//     messages.push_back(promptMsg);
//     const auto responseMsg = Session::ReceiveAIResponse(promptMsg);
//     messages.push_back(responseMsg);
// }

void RootChat::CreateBranch(const std::string& newChatName) {
    Session::AddBranch(id, id, groupId, newChatName);
}
void RootChat::LoadTree() {
    Session::LoadTree(id);
    Session::ChatTree.rootPtr = this;
}
void RootChat::Delete() {
    Session::DeleteChat(id);
    Session::base.FindGroup(groupId).DeleteRootChat(id);
    Session::ReloadBase();
}
void RootChat::Rename(const std::string &newName) {
    Session::RenameChat(id, newName);
    name = newName;
}

