#include "../Headers/Group.h"
#include "../../Session/Session.h"
void Group::PrintData() {
    std::cout << std::format("groupId: {}, name: {}", id, name) << std::endl;
    for (const auto& rchat : roots) {
        rchat.PrintData();
    }
}
void Group::AddRoot(const std::string &Name) {
    roots.push_back(DBAPI::SaveRootChat(id, Name));
    roots.back().FocusChat();
}
void Group::Delete() const {
    DBAPI::DeleteGroup(id);
    Session::ReloadBase();
}
void Group::Rename(const std::string &newName) {
    DBAPI::UpdateGroupName(id, newName);
    name = newName;
}
RootChat* Group::FindRootChat(const int targetId) {
    for (auto& root : roots) {
        if (root.id == targetId) {
            return &root;
        }
    }
    return nullptr;
}

