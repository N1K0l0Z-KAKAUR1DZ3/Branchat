#include "../Headers/Chat.h"
#include "../../Session/Session.h"
void Chat::PrintData() const {
    std::cout<< "\t\t"<< std::format("rootID: {}, parentId: {}, id: {}, name: {}",rootId, parentId, id, name)<< std::endl;
    PrintConversation();
}
void Chat::SendPrompt(const std::string& prompt) {
    messages.push_back(Message(rootId, id, "user", prompt));
    DBAPI::SaveMessage(messages.back());
    messages.push_back(Message(rootId, id, "model", Session::ReceiveAIResponse()));
    DBAPI::SaveMessage(messages.back());
 }


void Chat::CreateBranch(const std::string &newChatName) {
    branches.push_back(DBAPI::SaveBranchingChat(newChatName, id, rootId, groupId));
    branches.back().FocusChat();
}
void Chat::FocusChat() {
    // std::cout << std::format("Focusing chat: {} \n", name);
    Session::pointingAtRoot = false;
    Session::activeChatId = id;
    Session::chatPtr = this;
    // std::cout << std::format("Focused chat\n");
}

void Chat::Delete() {
    DBAPI::DeleteChat(id);
    Session::ReloadTree();
}



