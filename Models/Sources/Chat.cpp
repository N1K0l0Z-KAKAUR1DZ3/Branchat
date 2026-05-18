#include "../Headers/Chat.h"

void Chat::PrintData() const {
    std::cout<< "\t\t"<< std::format("rootID: {}, parentId: {}, id: {}, name: {}",rootId, parentId, id, name)<< std::endl;
    PrintConversation();
}