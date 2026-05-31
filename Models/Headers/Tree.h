#pragma once
#include <unordered_map>
#include "../Headers/Chat.h"

#ifndef BRANCHAT1_TREE_H
#define BRANCHAT1_TREE_H
struct Tree {
    inline static int rootId;
    inline static std::vector<Chat> topBranches;
    static void SetTree(const std::unordered_map<int, Chat>& flatMap);
    static void Clear();
    static void PrintData();
    static Chat* FindChatById(std::vector<Chat>& chatList, int targetId);
};
#endif //BRANCHAT1_TREE_H
