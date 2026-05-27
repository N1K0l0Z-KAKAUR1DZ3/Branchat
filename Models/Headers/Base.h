#pragma once
#include "../Headers/Group.h"

#ifndef BRANCHAT1_BASE_H
#define BRANCHAT1_BASE_H
struct Base {
    static std::vector<Group> groups;
    Base() = default;
    static void PrintData();
    static Group& FindGroup(int targetId);
    static void AddGroup(const std::string& name);
    static void DeleteGroup(int id);
};

#endif //BRANCHAT1_BASE_H
