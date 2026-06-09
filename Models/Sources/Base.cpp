#include "../Headers/Base.h"
#include "../../Session/Session.h"
#include <iostream>

std::vector<Group> Base::groups;

void Base::PrintData() {
    std::cout << "\n\n=====================================================\n";
    for (auto& group : groups) {
        group.PrintData();
    }
    std::cout << "=====================================================\n\n";
}

void Base::AddGroup(const std::string &name) {
    try {
        std::cout << "[BASE] Attempting to add group: '" << name << "'" << std::endl;
        groups.push_back(DBAPI::SaveGroup(name));
        std::cout << "[BASE] Group '" << name << "' added successfully." << std::endl;
    } catch (const std::exception& e) {
        // Since this is a core setup action, surfacing this error is critical
        Session::SetError(std::string("Failed to add group: ") + e.what());
        std::cerr << "[ERROR] Base::AddGroup: " << e.what() << std::endl;
    }
}

Group* Base::FindGroup(const int targetId) {
    // This is a search operation; it shouldn't trigger DB errors,
    // but we log the search attempt for debugging clarity.
    for (auto& group : groups) {
        if (group.id == targetId) {
            return &group;
        }
    }
    return nullptr;
}