#include "../Headers/Session.h"


Session::Session() {
    DBAPI::init();
    base = DBAPI::GetBase();
}

void Session::LoadChatTree(const int rootId) {
  ChatTree = DBAPI::GetChatTree(rootId);
}
void Session::FocusChat(const int chatId) {
    chatPtr.reset(&ChatTree.branchingChats[chatId]);
}
