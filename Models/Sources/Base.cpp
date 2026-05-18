#include "../Headers/Base.h"

Base::Base(std::vector<Group>& _groups): groups(std::move(_groups)){}

void Base::PrintData() {
    std::cout << "\n\n=====================================================\n";
    for (auto& group : groups) {
        group.PrintData();
    }
    std::cout << "=====================================================\n\n";
}
