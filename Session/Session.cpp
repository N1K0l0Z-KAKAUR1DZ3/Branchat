#include "Session.h"
#include <iostream>
#include <thread>
#include "../GUI/GUI.h"

void Session::SetError(const std::string &err) {
    std::lock_guard<std::mutex> lock(globalErrorMutex);
    globalErrorMessage = err;
}

Session::Session() {
    std::cout << "[SESSION] Session initializing. Initializing APIs and loading Base groups..." << std::endl;
    contextLimit = 30;
    // Catch startup failures (e.g. missing env vars, DB unreachable) so the app
    // opens and shows the error in its banner instead of terminating before main().
    try {
        DBAPI::init();
        AIAPI::init();
        Base::groups = DBAPI::GetBaseGroups();
        std::cout << "[SESSION] Session initialized. Loaded " << Base::groups.size() << " groups." << std::endl;
    } catch (const std::exception& e) {
        SetError(std::string("Startup error: ") + e.what());
        std::cerr << "[FATAL] Session init failed: " << e.what() << std::endl;
    }
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
    if (chatPtr->messages.size() > contextLimit) {
        std::cout << "[SESSION] FetchAdditionalContext skipped: Current chats contexts is above the limit" << std::endl;
        return nullptr;
    }

    auto tempChatPtr = static_cast<Chat*>(chatPtr);
    int parentId = tempChatPtr->parentId;
    std::cout << "[SESSION] Fetching context. ParentID: " << parentId << std::endl;

    if (Tree::FindChatById(Tree::topBranches, parentId) == nullptr) {
        return &Base::FindGroup(activeGroupId)->FindRootChat(parentId)->messages;
    }
    return &Tree::FindChatById(Tree::topBranches, parentId)->messages;
}

// Runs on the UI thread. Assembles the messages to send (parent context +
// this chat's messages, trimmed to the context limit) into a self-contained copy.
std::vector<Message> Session::BuildContextPayload() {
    std::cout << "[SESSION] Building context payload..." << std::endl;
    std::vector<Message> contextPayload;
    auto additionalContext = FetchAdditionalContext();

    if (additionalContext != nullptr) {
        std::cout << "[SESSION] Appending " << additionalContext->size() << " additional context messages." << std::endl;
        contextPayload.insert(contextPayload.end(), additionalContext->begin(), additionalContext->end());
    }

    contextPayload.insert(contextPayload.end(), chatPtr->messages.begin(), chatPtr->messages.end());
    if (contextPayload.size() > static_cast<size_t>(contextLimit)) {
        printf("history too long, trimming \n");
        size_t countToRemove = contextPayload.size() - contextLimit;
        contextPayload.erase(contextPayload.begin(), contextPayload.begin() + countToRemove);
    }
    return contextPayload;
}

// Runs on a worker thread. Touches ONLY its own payload copy and the result
// queue; it never reads or writes any chat, the DB, or the GUI directly.
void Session::RunAIRequest(std::vector<Message> payload, int chatId, int groupId, int rootIdForMsg) {
    PendingAIResult result;
    result.chatId = chatId;
    result.groupId = groupId;
    result.rootIdForMsg = rootIdForMsg;
    try {
        result.text = AIAPI::GetAIResponse(payload);
        result.ok = true;
    } catch (const std::exception& e) {
        result.ok = false;
        result.text = std::string("AI Thread Error: ") + e.what();
        std::cerr << "[ERROR] AI Thread (Chat ID " << chatId << "): " << e.what() << std::endl;
    }
    std::lock_guard<std::mutex> lock(resultMutex);
    pendingResults.push_back(std::move(result));
}

// Runs on the UI thread, once per frame. Appends finished replies to their chats
// and persists them. Re-locates each chat by id, so a reply whose chat was
// deleted meanwhile is simply dropped.
void Session::ApplyPendingResults() {
    std::vector<PendingAIResult> results;
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        if (pendingResults.empty()) return;
        results.swap(pendingResults);
    }

    for (auto& r : results) {
        RootChat* target = Tree::FindChatById(Tree::topBranches, r.chatId);
        if (target == nullptr) {
            auto group = Base::FindGroup(r.groupId);
            if (group) target = group->FindRootChat(r.chatId);
        }

        if (target == nullptr) {
            std::cout << "[SESSION] AI reply dropped: chat " << r.chatId << " no longer exists." << std::endl;
            if (!r.ok) SetError(r.text);
            continue;
        }

        if (r.ok) {
            Message reply(r.rootIdForMsg, r.chatId, "model", r.text);
            DBAPI::SaveMessage(reply); // DB touched only on the UI thread
            target->messages.push_back(std::move(reply));
            GUI::scrollDown = true;
        } else {
            SetError(r.text);
        }
        target->waitingResponse = false;
    }
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