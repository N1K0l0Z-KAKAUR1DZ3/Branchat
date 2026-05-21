#pragma once
#include <unordered_map>
#include "../Headers/Chat.h"

#ifndef BRANCHAT1_TREE_H
#define BRANCHAT1_TREE_H
struct Tree {
    RootChat *rootPtr;
    std::unordered_map<int, Chat> branchingChats;
    void PrintData() const ;
    void Clear();
};
#endif //BRANCHAT1_TREE_H
