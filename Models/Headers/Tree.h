#pragma once
#include <unordered_map>
#include "../Headers/Chat.h"

#ifndef BRANCHAT1_TREE_H
#define BRANCHAT1_TREE_H
struct Tree {
    inline static RootChat *rootPtr;
    inline static std::unordered_map<int, Chat> branchingChats;
    static void PrintData() ;
    static Chat* FindChatById(int targetId);
    static void Clear();
};
#endif //BRANCHAT1_TREE_H
