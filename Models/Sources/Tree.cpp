#include "../Headers/Tree.h"

void Tree::PrintData() const {
    std::cout << "\n\n=====================================================\n";
    rootChat.PrintData();
    for (auto& ptr : branchingChats) {
        ptr.second.PrintData();
    }
    std::cout << "=====================================================\n\n";
}