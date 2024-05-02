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
        
    }

    void Update() {
        ClearBackground(BLACK);
        Mod_Callback("Update_Menu", (void*)&data);
    }
};

#endif // MENU_CXX