#include "Session.h"
#include <iostream>

void Session::SetError(const std::string &err) {
    std::lock_guard<std::mutex> lock(globalErrorMutex);
    globalErrorMessage = err;
}

Session::Session() {
    std::cout << "[SESSION] Session initializing. Initializing APIs and loading Base groups..." << std::endl;
    DBAPI::init();
    AIAPI::init();
    Base::groups = DBAPI::GetBaseGroups();
    contextLimit = 30;
    std::cout << "[SESSION] Session initialized. Loaded " << Base::groups.size() << " groups." << std::endl;
}

void Session::AttemptRehook() {
    std::cout << "[SESSION] Attempting to rehook active context. ActiveChatID: " << activeChatId << std::endl;
    if (activeChatId == -1) {
        std::cout << "[SESSION] AttemptRehook skipped: No active chat." << std::endl;
        return;
    }

    RootChat* hook = Tree::FindChatById(Tree::topBranches, activeChatId);
    if (hook == nullptr) {
        std::cout << "[SESSION] Hook not found in Tree branches, checking Base Groups..." << std::endl;
        auto group = Base::FindGroup(activeGroupId);
        if (group) {
            hook = group->FindRootChat(activeChatId);
        }
    }

    if (hook == nullptr) {
        std::cout << "[SESSION] Rehook failed: Chat ID " << activeChatId << " not found. Resetting focus." << std::endl;
        resetFocus();
        return;
    }

    hook->FocusChat();
    std::cout << "[SESSION] Rehook successful. Focused Chat ID: " << activeChatId << std::endl;
}

void Session::resetFocus() {
    std::cout << "[SESSION] Resetting session focus." << std::endl;
    activeChatId = -1;
    activeGroupId = -1;
    pointingAtRoot = false;
    chatPtr = nullptr;
}

std::vector<Message>* Session::FetchAdditionalContext() {
    if (pointingAtRoot) {
        std::cout << "[SESSION] FetchAdditionalContext skipped: Pointing at Root." << std::endl;
        return nullptr;
    }

    auto tempChatPtr = static_cast<Chat*>(chatPtr);
    int parentId = tempChatPtr->parentId;
    std::cout << "[SESSION] Fetching context. ParentID: " << parentId << std::endl;

    if (Tree::FindChatById(Tree::topBranches, parentId) == nullptr) {
        auto group = Base::FindGroup(activeGroupId);
        if (group) {
            auto root = group->FindRootChat(parentId);
            if (root) return &root->messages;
        }
        std::cout << "[SESSION] Additional context source not found." << std::endl;
        return nullptr;
    }
    return &Tree::FindChatById(Tree::topBranches, parentId)->messages;
}

std::string Session::ReceiveAIResponse() {
    std::cout << "[SESSION] Receiving AI Response..." << std::endl;
    std::vector<Message> contextPayload;
    auto additionalContext = FetchAdditionalContext();

    if (additionalContext != nullptr) {
        std::cout << "[SESSION] Appending " << additionalContext->size() << " additional context messages." << std::endl;
        contextPayload.insert(contextPayload.end(), additionalContext->begin(), additionalContext->end());
    }

    contextPayload.insert(contextPayload.end(), chatPtr->messages.begin(), chatPtr->messages.end());
    if (contextPayload.size() > 10) {
        if (contextPayload.size() > (contextLimit)) {
            printf("history too long, trinning \n");
            size_t countToRemove = contextPayload.size() - contextLimit;
            contextPayload.erase(contextPayload.begin(), contextPayload.begin() + countToRemove);
        }
    }
    auto response = AIAPI::GetAIResponse(contextPayload);

    std::cout << "[SESSION] AI Response received. Length: " << response.length() << std::endl;
    return response;
}

void Session::ReloadTree() {
    std::cout << "[SESSION] Reloading Tree for RootID: " << Tree::rootId << std::endl;
    Tree::SetTree(DBAPI::GetChatTree(Tree::rootId));

    if (Tree::topBranches.empty()) {
        auto group = Base::FindGroup(activeGroupId);
        if (group) {
            auto root = group->FindRootChat(Tree::rootId);
            if (root) root->hasChildren = false;
        }
    }
    AttemptRehook();
}

void Session::ReloadBase() {
    std::cout << "[SESSION] Reloading Base groups from DB." << std::endl;
    Base::groups = DBAPI::GetBaseGroups();

    auto groupPtr = Base::FindGroup(activeGroupId);
    if (Tree::rootId != -1){
       if (groupPtr == nullptr || groupPtr->FindRootChat(Tree::rootId) == nullptr) {
           std::cout << "[SESSION] Active tree node lost during reload. Clearing Tree." << std::endl;
           Tree::Clear();
       }
    }
    AttemptRehook();
    std::cout << "[SESSION] Base reload complete." << std::endl;
}