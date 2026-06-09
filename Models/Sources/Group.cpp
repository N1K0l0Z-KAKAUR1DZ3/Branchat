#include "../Headers/Group.h"
#include "../../Session/Session.h"
#include <iostream>

void Group::PrintData() {
    std::cout << "[DEBUG] Printing Group Data. ID: " << id << ", Name: " << name << std::endl;
    for (const auto& rchat : roots) {
        rchat.PrintData();
    }
}

void Group::AddRoot(const std::string &Name) {
    try {
        std::cout << "[GROUP] AddRoot requested: '" << Name << "' under Group ID: " << id << std::endl;
        roots.push_back(DBAPI::SaveRootChat(id, Name));
        roots.back().FocusChat();
        std::cout << "[GROUP] RootChat created successfully." << std::endl;
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to add root: ") + e.what());
        std::cerr << "[ERROR] AddRoot: " << e.what() << std::endl;
    }
}

void Group::Delete() const {
    try {
        std::cout << "[GROUP] Delete requested for Group ID: " << id << std::endl;
        DBAPI::DeleteGroup(id);
        Session::ReloadBase();
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to delete group: ") + e.what());
        std::cerr << "[ERROR] Delete: " << e.what() << std::endl;
    }
}

void Group::Rename(const std::string &newName) {
    try {
        std::cout << "[GROUP] Rename requested: '" << name << "' -> '" << newName << "' (ID: " << id << ")" << std::endl;
        DBAPI::UpdateGroupName(id, newName);
        name = newName;
    } catch (const std::exception& e) {
        Session::SetError(std::string("Failed to rename group: ") + e.what());
        std::cerr << "[ERROR] Rename: " << e.what() << std::endl;
    }
}

RootChat* Group::FindRootChat(const int targetId) {
    std::cout << "[GROUP] Searching for RootChat ID: " << targetId << " within Group ID: " << id << std::endl;
    for (auto& root : roots) {
        if (root.id == targetId) {
            return &root;
        }
    }
    return nullptr;
}