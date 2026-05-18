#include "APIs/Headers/DBAPI.h"
#include <thread>

void Pause() {

    // 3. Pause the program for exactly 1 second
    std::this_thread::sleep_for(std::chrono::seconds(2));

}

int main() {
    DBAPI::init();
    auto base = DBAPI::GetBase();
    base.PrintData();
}