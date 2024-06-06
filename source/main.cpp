#include <raylib.h>
#include <time.h>
#include "scene.cpp"
#include "mod_loader.cpp"

#ifdef PLATFORM_ANDROID
#define ASSETS_ROOT ""
#else
#define ASSETS_ROOT "assets/"
#endif

#include "scenes/intro.cpp"
#include "scenes/menu.cpp"
#include "scenes/game.cpp"

#ifdef PLATFORM_IOS // IOS struktura
extern "C" {
    void ios_ready();
    void ios_update();
    void ios_destroy();
}

#define Ready ios_ready
#define Update ios_update
#define Destroy ios_destroy
#endif

void Ready() {
    srand(time(NULL));
    SetConfigFlags(FLAG_MSAA_4X_HINT);

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    InitWindow(0, 0, "Výprava za pribináčkem");
#else
    InitWindow(1000, 600, "Výprava za pribináčkem");
#endif

    InitAudioDevice();
    
    ChangeDirectory(GetApplicationDirectory());

    Set_Scene_Data({
        {INTRO,   {Intro::Init,   Intro::On_Switch,   Intro::Update}},
        {MENU,    {Menu::Init,    Menu::On_Switch,    Menu::Update}},
        {GAME,    {Game::Init,    Game::On_Switch,    Game::Update}}
    });

    Init_Scenes();
    Switch_To_Scene(MENU);

    Mod_Load_Directory(ASSETS_ROOT "mods/");
    Mod_Callback("Init", nullptr);

    SetTargetFPS(60);
    EnableCursor();
}

void Update() {
    BeginDrawing(); {
        Update_Scene();
    } EndDrawing();
}

void Destroy() {
    CloseWindow();
}

#ifndef PLATFORM_IOS // Normální main funkce
int main() {
    Ready();

    while(!WindowShouldClose()) {
        Update();
    }

    Destroy();
    return 0;
}
#endif