#include "../Headers/Root_chat.h"


void RootChat::PrintData() const {
    std::cout << "\t"<< std::format("rootID: {}, name: {}", id, name)<< std::endl;
    PrintConversation();
}

void RootChat::PrintConversation() const {
    for (const auto& msg : messages) {
        msg.Print();
    }
}

void RootChat::FocusChat() {

}

