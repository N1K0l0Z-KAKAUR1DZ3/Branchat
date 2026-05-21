#include "../Headers/Group.h"
#include "../../Services/Headers/Session.h"
void Group::PrintData() {
    std::cout << std::format("groupId: {}, name: {}", id, name) << std::endl;
    for (const auto& rchat : roots) {
        rchat.PrintData();
    }
}
void Group::AddRoot(const std::string &Name) {
    roots.push_back(Session::AddRoot(id, Name));
    roots.back().FocusChat();
}
void Group::Delete() const {
    Session::DeleteGroup(id);
    Session::base.DeleteGroup(id);
    Session::ReloadBase();
}
void Group::Rename(const std::string &newName) {
    Session::RenameGroup(id, newName);
    name = newName;
}
RootChat& Group::FindRootChat(const int targetId) {
     auto iterator = std::find_if(roots.begin(), roots.end(), [targetId](RootChat& rChat) {
        return rChat.id == targetId;
    });
    if (iterator != roots.end()) {
        return *iterator;
    }
    throw std::runtime_error("Error: rootChat with ID " + std::to_string(targetId) + " not found!");
}
void Group::DeleteRootChat(const int targetRootId) {
    std::erase_if(roots, [targetRootId](const RootChat& element) {
    return element.id == targetRootId;
});
}

