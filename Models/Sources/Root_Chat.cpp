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

void RootChat::SendPrompt(const std::string& prompt) {
    try {
        std::cout << "[ROOT] SendPrompt triggered on RootChat ID: " << id << std::endl;
        messages.push_back(Message(id, id, "user", prompt));
        DBAPI::SaveMessage(messages.back());

        waitingResponse = true;
        std::thread([this, prompt]() {
            try {
                std::string response = Session::ReceiveAIResponse();
                messages.push_back(Message(id, id, "model", response));
                DBAPI::SaveMessage(messages.back());
            } catch (const std::exception& e) {
                Session::SetError(std::string("AI Thread Error: ") + e.what());
                std::cerr << "[ERROR] AI Thread: " << e.what() << std::endl;
            }
            waitingResponse = false;
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