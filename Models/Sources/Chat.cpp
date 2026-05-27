#include "../Headers/Chat.h"
#include "../../Services/Headers/Session.h"
void Chat::PrintData() const {
    std::cout<< "\t\t"<< std::format("rootID: {}, parentId: {}, id: {}, name: {}",rootId, parentId, id, name)<< std::endl;
    PrintConversation();
}
void Chat::SendPrompt(const std::string& prompt) {
    messages.push_back(Message(rootId, id, "user", prompt));
    Session::SaveMessage(messages.back());
    messages.push_back(Message(rootId, id, "model", Session::ReceiveAIResponse()));
    Session::SaveMessage(messages.back());
 }

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
    Session::activeChatId = id;
    Session::activeGroupId = groupId;
    Session::rehookChatPtr();
    Session::currentContextPtr = &messages;
    if (!Tree::branchingChats.contains(parentId)) {
        Session::additionalContextPtr = &Tree::rootPtr->messages;
        return;
    }
    Session::additionalContextPtr = &Tree::branchingChats[parentId].messages;
}
void Chat::Delete() {
    Session::DeleteChat(id);
    Session::ReloadTree();
}
