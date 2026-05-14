#pragma once
#include <iostream>
#include <vector>
#include <format>
#include "Root_Chat.cpp"
struct Group {
    int id;
    std::string name;
    std::vector<RootChat> roots;
    explicit Group(const int _id,  std::string _name, std::vector<RootChat> &_roots) : id(_id), name(std::move(_name)), roots(std::move(_roots)){}
    void PrintData() {
        std::cout << std::format("groupId: {}, name: {}", id, name) << std::endl;
        for (const auto& rchat : roots) {
            rchat.PrintData();
        }
    }
};