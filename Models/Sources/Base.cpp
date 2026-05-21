#include "../Headers/Base.h"
#include "../../Services/Headers/Session.h"
Base::Base(std::vector<Group>& _groups): groups(std::move(_groups)){}

void Base::PrintData() {
    std::cout << "\n\n=====================================================\n";
    for (auto& group : groups) {
        group.PrintData();
    }
    std::cout << "=====================================================\n\n";
}
void Base::AddGroup(const std::string &name) {
    groups.push_back(Session::AddGroup(name));
}
Group& Base::FindGroup(const int targetId) {
    auto iterator = std::find_if(groups.begin(), groups.end(), [targetId](Group& g) {
        return g.id == targetId;
    });
    if (iterator != groups.end()) {
        return *iterator;
    }
    throw std::runtime_error("Error: Group with ID " + std::to_string(targetId) + " not found!");
}
void Base::DeleteGroup(int id) {
    std::erase_if(groups, [id](const Group& element) {
        return element.id == id;
    });
}
