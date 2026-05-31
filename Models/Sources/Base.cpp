#include "../Headers/Base.h"
#include "../../Session/Session.h"
std::vector<Group> Base::groups;
void Base::PrintData() {
    std::cout << "\n\n=====================================================\n";
    for (auto& group : groups) {
        group.PrintData();
    }
    std::cout << "=====================================================\n\n";
}
void Base::AddGroup(const std::string &name) {
    groups.push_back(DBAPI::SaveGroup(name));
}
 Group* Base::FindGroup(const int targetId) {
    for (auto& group : groups) {
        if (group.id == targetId) {
            return &group;
        }
    }
    return nullptr;
}