#include "../Headers/Group.h"

void Group::PrintData() {
    std::cout << std::format("groupId: {}, name: {}", id, name) << std::endl;
    for (const auto& rchat : roots) {
        rchat.PrintData();
    }
}