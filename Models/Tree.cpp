#pragma once
#include <unordered_map>
#include <iostream>
#include "Root_Chat.cpp"
#include "Chat.cpp"
struct ChatTree {
    RootChat rootChat;
    std::unordered_map<int, Chat> branchingChats;
    void PrintData() {
        std::cout << "\n\n=====================================================\n";
        rootChat.PrintData();
        for (auto& ptr : branchingChats) {
            ptr.second.PrintData();
        }
        std::cout << "=====================================================\n\n";
    }
};