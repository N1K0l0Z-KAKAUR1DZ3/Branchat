#include "../Headers/Tree.h"

void Tree::PrintData() const {
    std::cout << "\n\n=====================================================\n";
    for (auto& ptr : branchingChats) {
        ptr.second.PrintData();
    }
    std::cout << "=====================================================\n\n";
}

void Tree::Clear() {
    rootPtr = nullptr;
    branchingChats.clear();
}
