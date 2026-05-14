#pragma once
#include <vector>
#include <iostream>
struct Base {
    std::vector<Group> groups;
    explicit Base(std::vector<Group> _groups): groups(_groups){}
    void PrintData() {
        std::cout << "\n\n=====================================================\n";
        for (auto& group : groups) {
            group.PrintData();
        }
        std::cout << "=====================================================\n\n";
    }
};
