#include "../Headers/Chat.h"
#include "../../Session/Session.h"
#include <iostream>
#include <thread>

void Chat::PrintData() const {
    std::cout << "[CHAT] Printing Chat Data. ID: " << id << ", Name: " << name
              << ", RootID: " << rootId << ", ParentID: " << parentId << std::endl;
    PrintConversation();
}

void Chat::SendPrompt(const std::string& prompt) {
    try {
        std::cout << "[CHAT] SendPrompt triggered on Chat ID: " << id << std::endl;
        messages.push_back(Message(rootId, id, "user", prompt));
        DBAPI::SaveMessage(messages.back());

        waitingResponse = true;

        std::thread([this, prompt]() {
            try {
                std::string response = Session::ReceiveAIResponse();
                messages.push_back(Message(rootId, id, "model", response));
                DBAPI::SaveMessage(messages.back());
            } catch (const std::exception& e) {
                Session::SetError(std::string("AI Thread Error: ") + e.what());
                std::cerr << "[ERROR] AI Thread (Chat ID " << id << "): " << e.what() << std::endl;
            }
            waitingResponse = false;
        }).detach();
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to send prompt: ") + e.what());
        waitingResponse = false;
        std::cerr << "[ERROR] SendPrompt (Chat ID " << id << "): " << e.what() << std::endl;
    }
}

void Chat::CreateBranch(const std::string &newChatName) {
    try {
        std::cout << "[CHAT] CreateBranch called: '" << newChatName << "' as child of ID: " << id << std::endl;
        branches.push_back(DBAPI::SaveBranchingChat(newChatName, id, rootId, groupId));
        branches.back().FocusChat();
        std::cout << "[CHAT] Branch created. Total branches: " << branches.size() << std::endl;
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to create branch: ") + e.what());
        std::cerr << "[ERROR] CreateBranch: " << e.what() << std::endl;
    }
}

void Chat::FocusChat() {
    std::cout << "[CHAT] Focusing Chat: " << name << " (ID: " << id << ")" << std::endl;
    Session::pointingAtRoot = false;
    Session::activeChatId = id;
    Session::chatPtr = this;
}

void Chat::Delete() {
    try {
        std::cout << "[CHAT] Delete requested for Chat ID: " << id << " (Name: " << name << ")" << std::endl;
        DBAPI::DeleteChat(id);
        Session::ReloadTree();
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to delete chat: ") + e.what());
        std::cerr << "[ERROR] Delete: " << e.what() << std::endl;
    }
}