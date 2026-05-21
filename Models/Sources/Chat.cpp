#include "../Headers/Chat.h"
#include "../../Services/Headers/Session.h"
void Chat::PrintData() const {
    std::cout<< "\t\t"<< std::format("rootID: {}, parentId: {}, id: {}, name: {}",rootId, parentId, id, name)<< std::endl;
    PrintConversation();
}

// void Chat::SendPrompt(const std::string& prompt) {
//     const auto promptMsg = Message(rootId, id, "User", prompt);
//     messages.push_back(promptMsg);
//     const auto responseMsg = Session::ReceiveAIResponse(promptMsg);
//     messages.push_back(responseMsg);
// }
Chat::~Chat() {
    if (Session::chatPtr == this) {
        Session::resetFocus();
    }
}
void Chat::CreateBranch(const std::string &newChatName) {
    Session::AddBranch(id, rootId, groupId, newChatName);
}
void Chat::FocusChat() {
    Session::pointingAtRoot = false;
    Session::chatPtr = this;
}
void Chat::Delete() {
    Session::DeleteChat(id);
    Session::ReloadTree();
}
