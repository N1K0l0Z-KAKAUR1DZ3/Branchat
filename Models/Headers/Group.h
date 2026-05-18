#pragma once
#include "../Headers/Root_chat.h"

#ifndef BRANCHAT1_GROUP_H
#define BRANCHAT1_GROUP_H

struct Group {
    int id;
    std::string name;
    std::vector<RootChat> roots;
    explicit Group(const int _id,  std::string _name, std::vector<RootChat> &_roots):
        id(_id),
        name(std::move(_name)),
        roots(std::move(_roots)){}
    void PrintData();
};
#endif //BRANCHAT1_GROUP_H
