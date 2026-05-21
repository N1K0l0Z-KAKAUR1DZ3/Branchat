#pragma once
#include "../Headers/Group.h"

#ifndef BRANCHAT1_BASE_H
#define BRANCHAT1_BASE_H
struct Base {
    std::vector<Group> groups;
    explicit Base(std::vector<Group>& _groups);
    void PrintData();
    Base() = default;
    Group& FindGroup(int targetId);
    void AddGroup(const std::string& name);
    void DeleteGroup(int id);
};

#endif //BRANCHAT1_BASE_H
