#include "../Headers/Tree.h"
#include <functional>
// void Tree::PrintData()  {
//     std::cout << "\n\n=====================================================\n";
//     std::function<void(Chat&)> printNode = [&](Chat& chat) {
//         chat.PrintData();
//         for (auto& child : chat.branches) {
//             printNode(child);
//         }
//     };
//     for (auto& topChat : topBranches) {
//         printNode(topChat);
//     }
//     std::cout << "=====================================================\n\n";
// }

void Tree::PrintData() {
    std::function<void(Chat&, int)> printNode = [&](Chat& chat, int depth) {
        if (depth > 50) {
            std::cout << "CRITICAL ERROR: Data cycle detected at Chat ID: " << chat.id << std::endl;
            return;
        }
        chat.PrintData();
        for (auto& child : chat.branches) {
            printNode(child, depth + 1);
        }
    };
    for (auto& topChat : topBranches) {
        printNode(topChat, 0);
    }
}

Chat* Tree::FindChatById(std::vector<Chat>& chatList,const int targetId) {
    for (auto& chat : chatList) {
        if (chat.id == targetId) {
            // std::cout << "found chat\n\n";
            return &chat;
        }
        if (!chat.branches.empty()) {
            Chat* current = FindChatById(chat.branches, targetId);
            if (current != nullptr) {
                return current;
            }
        }
    }
    std::cout << "chat not found\n\n";
    return nullptr;
}


void Tree::SetTree(const std::unordered_map<int, Chat>& flatMap) {
    topBranches.clear();
    std::unordered_map<int, std::vector<int>> childIdMap;
    std::vector<int> topLevelIds;
    for (const auto& [id, chat] : flatMap) {
        if (flatMap.find(chat.parentId) == flatMap.end()) {
            topLevelIds.push_back(id);
        } else {
            // Otherwise, it's a child. File it under its parent!
            childIdMap[chat.parentId].push_back(id);
        }
    }
    std::function<Chat(int)> buildNestedNode = [&](int nodeId) -> Chat {

        Chat currentNode = flatMap.at(nodeId);

        auto it = childIdMap.find(nodeId);
        if (it != childIdMap.end()) {
            for (int childId : it->second) {
                currentNode.branches.push_back(buildNestedNode(childId));
            }
        }
        return currentNode;
    };
    for (int topId : topLevelIds) {
        topBranches.push_back(buildNestedNode(topId));
    }
}

void Tree::Clear() {
    rootId = -1;
    topBranches.clear();
}
