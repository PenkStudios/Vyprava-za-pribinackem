#include <raylib.h>
#include <time.h>
#include "scene.cpp"
#include "mod_loader.cpp"
#include "path.cpp"

#include "scenes/intro.cpp"
#include "scenes/menu.cpp"
#include "scenes/game.cpp"

int main() {
    srand(time(NULL));
    SetConfigFlags(FLAG_MSAA_4X_HINT);

#ifdef PLATFORM_ANDROID
    InitWindow(0, 0, "Výprava za pribináčkem");
#else
    InitWindow(1000, 600, "Výprava za pribináčkem");
#endif

    InitAudioDevice();
    
    ChangeDirectory(GetApplicationDirectory());
    ChangeDirectory("assets");

    Set_Scene_Data({
        {INTRO,   {Intro::Init,   Intro::On_Switch,   Intro::Update}},
        {MENU,    {Menu::Init,    Menu::On_Switch,    Menu::Update}},
        {GAME,    {Game::Init,    Game::On_Switch,    Game::Update}}
    });

    Init_Scenes();
    Switch_To_Scene(MENU);

    Init_Path(50, Game::data.house_BBoxes, Game::data.house_BBox);

    Mod_Load_Directory("mods/");
    Mod_Callback("Init", nullptr);

    SetTargetFPS(60);
    EnableCursor();

    while(!WindowShouldClose()) {
        BeginDrawing(); {
            Update_Scene();
        } EndDrawing();
    }

    CloseWindow();

    return 0;
}
