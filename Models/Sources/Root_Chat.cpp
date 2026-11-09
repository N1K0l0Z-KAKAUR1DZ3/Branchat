#include "../Headers/Root_chat.h"
#include "../../Session/Session.h"
#include <iostream>
#include <thread>

void RootChat::PrintData() const {
    std::cout << "\t" << std::format("rootID: {}, name: {}, has children: {}", id, name, hasChildren) << std::endl;
    PrintConversation();
}

void RootChat::PrintConversation() const {
    std::cout << "[ROOT] Printing conversation for RootChat ID: " << id << std::endl;
    for (const auto& msg : messages) {
        msg.Print();
    }
}

void RootChat::FocusChat() {
    std::cout << "[ROOT] Focusing RootChat: " << name << " (ID: " << id << ")" << std::endl;
    Session::pointingAtRoot = true;
    Session::activeChatId = id;
    Session::activeGroupId = groupId;
    Session::chatPtr = this;
}

void RootChat::SendPrompt(const std::string& prompt, std::vector<Attachment> attachments) {
    try {
        std::cout << "[ROOT] SendPrompt triggered on RootChat ID: " << id
                  << " with " << attachments.size() << " image(s)" << std::endl;
        Message userMsg(id, id, "user", prompt);
        userMsg.Attachments = std::move(attachments);
        // Save before adding to the chat, so a failed save doesn't leave an unsaved message on screen
        DBAPI::SaveMessage(userMsg);
        messages.push_back(std::move(userMsg));

        waitingResponse = true;
        // Build the request here (UI thread) so the worker shares no state with us.
        std::vector<Message> payload = Session::BuildContextPayload();
        const int chatId = id, grpId = groupId, rootForMsg = id;
        std::thread([payload = std::move(payload), chatId, grpId, rootForMsg]() mutable {
            Session::RunAIRequest(std::move(payload), chatId, grpId, rootForMsg);
        }).detach();
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to send prompt: ") + e.what());
        waitingResponse = false;
    }
}

void RootChat::CreateBranch(const std::string& newChatName) {
    try {
        std::cout << "[ROOT] CreateBranch called: '" << newChatName << "' under RootChat ID: " << id << std::endl;
        if (Tree::rootId != id) {
            LoadTree();
        }
        Tree::topBranches.push_back(DBAPI::SaveBranchingChat(newChatName, id, id, groupId));
        hasChildren = true;
        Tree::topBranches.back().FocusChat();
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to create branch: ") + e.what());
        std::cerr << "[ERROR] CreateBranch: " << e.what() << std::endl;
    }
}

void RootChat::LoadTree() {
    try {
        std::cout << "[ROOT] LoadTree requested for RootChat ID: " << id << std::endl;
        Tree::SetTree(DBAPI::GetChatTree(id));
        Tree::rootId = id;
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to load tree: ") + e.what());
        std::cerr << "[ERROR] LoadTree: " << e.what() << std::endl;
    }
}

void RootChat::Delete() {
    try {
        std::cout << "[ROOT] Delete requested for RootChat ID: " << id << std::endl;
        DBAPI::DeleteChat(id);
        Session::ReloadBase();
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to delete chat: ") + e.what());
        std::cerr << "[ERROR] Delete: " << e.what() << std::endl;
    }
}

void RootChat::Rename(const std::string &newName) {
    try {
        std::cout << "[ROOT] Rename requested: '" << name << "' -> '" << newName << "' (ID: " << id << ")" << std::endl;
        DBAPI::UpdateChatName(id, newName);
        name = newName;
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to rename chat: ") + e.what());
        std::cerr << "[ERROR] Rename: " << e.what() << std::endl;
    }
}