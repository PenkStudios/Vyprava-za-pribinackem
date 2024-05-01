#include <raylib.h>
#include "scene.cpp"
#include "mod_loader.cpp"

#include "scenes/intro.cpp"
#include "scenes/menu.cpp"
#include "scenes/game.cpp"

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1000, 600, "Výprava za pribiňáčkem");
    InitAudioDevice();
    
    ChangeDirectory(GetApplicationDirectory());
    ChangeDirectory("assets");

    Set_Scene_Data({
        {INTRO, {Intro::Init,   Intro::Update}},
        {MENU,  {Menu::Init,    Menu::Update}},
        {GAME,  {Game::Init,    Game::Update}}
    });

    Switch_To_Scene(GAME);
    Init_Scenes();

    Mod_Load_Directory("mods/");

    SetTargetFPS(60);

    while(!WindowShouldClose()) {
        BeginDrawing(); {
            Update_Scene();
            Mod_Callback("Update_2D");
        } EndDrawing();
    }

    CloseWindow();

    return 0;
}