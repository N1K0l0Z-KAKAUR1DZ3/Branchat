#include "Session.h"


Session::Session() {
    DBAPI::init();
    AIAPI::init();
    Base::groups = DBAPI::GetBaseGroups();
}

void Session::AttemptRehook() {
    // std::cout<<"Rehooking\n";
    if (activeChatId == -1) {
        return;
    }
    // std::cout<<"RootChat* hook = Tree::FindChatById(Tree::topBranches, activeChatId);\n";
    RootChat* hook = Tree::FindChatById(Tree::topBranches, activeChatId);
    if (hook == nullptr) {
        // std::cout<<" hook = Base::FindGroup(activeGroupId)->FindRootChat(activeChatId);\n";
        hook = Base::FindGroup(activeGroupId)->FindRootChat(activeChatId);
    }
    if (hook == nullptr) {
        // std::cout<<" resetFocus();\n";
      resetFocus();
        return;
    }
    // std::cout<<" hook->FocusChat();\n";
    hook->FocusChat();
    // std::cout<<"Rehooked\n";
}


void Session::resetFocus() {
    activeChatId = -1;
    activeGroupId = -1;
    pointingAtRoot = false;
    chatPtr = nullptr;
}

std::vector<Message>* Session::FetchAdditionalContext() {
    if (pointingAtRoot) {
        return nullptr;
    }
    auto tempChatPtr = static_cast<Chat*>(chatPtr);
    int parentId = tempChatPtr->parentId;

    if (Tree::FindChatById(Tree::topBranches, parentId) == nullptr) {
        return &Base::FindGroup(activeGroupId)->FindRootChat(parentId)->messages;
    }
    return &Tree::FindChatById(Tree::topBranches, parentId)->messages;
}

std::string Session::ReceiveAIResponse() {
    std::vector<Message> contextPayload;
    auto additionalContext = FetchAdditionalContext();
    if (additionalContext != nullptr) {
        contextPayload.insert(contextPayload.end(), additionalContext->begin(), additionalContext->end());
    }
    contextPayload.insert(contextPayload.end(), chatPtr->messages.begin(), chatPtr->messages.end());
    // for (auto& msg : contextPayload) {
    //     std::cout <<msg.Role<< ": " <<msg.Content << std::endl;
    // }
    auto response = AIAPI::GetAIResponse(contextPayload);
    return response;
}

void Session::ReloadTree() {
    Tree::SetTree(DBAPI::GetChatTree(Tree::rootId));
    if (Tree::topBranches.empty()) {
        Base::FindGroup(activeChatId)->FindRootChat(Tree::rootId)->hasChildren = false;
    }
    AttemptRehook();
}

void Session::ReloadBase() {
    std::cout<<"Reloading base\n";
    Base::groups = DBAPI::GetBaseGroups();
    auto groupPtr = Base::FindGroup(activeGroupId);
    if (Tree::rootId != -1){
       if ( groupPtr == nullptr ) {
           Tree::Clear();
       } else if(groupPtr->FindRootChat(Tree::rootId) == nullptr) {
        Tree::Clear();
        }
    }
    AttemptRehook();
    std::cout<<"Reloaded base\n";
}