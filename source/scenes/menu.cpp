#ifndef MENU_CXX
#define MENU_CXX

#include <raylib.h>
#include <raymath.h>

#include "../mod_loader.cpp"
#include "../scene.cpp"

namespace Menu {
    class Menu_Data {
    public:
    } data;

    void Init() {
        Mod_Callback("Init_Menu", (void*)&data);
    }

    void Update() {
        ClearBackground(BLACK);
        Mod_Callback("Update_Menu", (void*)&data, 0);
    }
};

#endif // MENU_CXX