#include "../Headers/Tree.h"

void Tree::PrintData()  {
    std::cout << "\n\n=====================================================\n";
    for (auto& ptr : branchingChats) {
        ptr.second.PrintData();
    }
    std::cout << "=====================================================\n\n";
}

Chat* Tree::FindChatById(int targetId) {
    // Iterate through every key-value pair in the unordered_map
    for (auto& [key, chat] : branchingChats) {
        if (chat.id == targetId) {
            // Return the memory address of the struct living inside the map
            return &chat;
        }
    }
    return nullptr;
}
void Tree::Clear() {
    rootPtr = nullptr;
    branchingChats.clear();
}
