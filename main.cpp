#include "GUI/GUI.h"

Session Session_Bootstrap;
GUI GUI_Bootstrap;

int main() {
    if (!GUI::Initialize(1280, 720, "Branchat App")) {
        return -1;
    }

   GUI::StartRenderLoop();
   return 0;
}