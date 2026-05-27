#include "APIs/Headers/DBAPI.h"
#include <thread>
#include <curl/curl.h>

#include "Services/Headers/Session.h"

void Pause() {

    // 3. Pause the program for exactly 1 second
    std::this_thread::sleep_for(std::chrono::seconds(2));

}

Session bootstrap;

int main() {
    Base::FindGroup(5).FindRootChat(28).FocusChat();
    Session::chatPtr->LoadTree();
    Session::chatPtr->PrintData();
    Session::chatPtr->CreateBranch("Branch2");
    Session::chatPtr->SendPrompt("what was your third response to me?");
    Session::chatPtr->PrintData();
    // Tree::PrintData();

}