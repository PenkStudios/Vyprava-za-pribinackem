#include <raylib.h>

#include <time.h>
#include <iterator>
#include "scene.cpp"
#include "mod_loader.cpp"

#ifdef PLATFORM_ANDROID
#include "android.cpp"
#define ASSETS_ROOT ""
#else
#define ASSETS_ROOT "assets/"
#endif

#include "mission.cpp"

#include "scenes/intro.cpp"
#include "scenes/menu.cpp"
#include "scenes/game.cpp"
#include "scenes/shared.cpp"

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

Texture cursor;

#ifdef PLATFORM_ANDROID
#include <android_native_app_glue.h>
extern "C" android_app* GetAndroidApp();
#endif

void Ready() {
    srand(time(NULL));

    SetConfigFlags(FLAG_MSAA_4X_HINT);
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    std::string root = std::string(GetAndroidApp()->activity->internalDataPath) + "/";
#else
    std::string root = ASSETS_ROOT;
#endif

    Shared::data.quality = 2;

    std::istringstream iss = Shared::Load_From_File("player.txt");
    std::string line;
    while (std::getline(iss, line)) {
        std::stringstream string_Stream(line);
            
        std::vector<std::string> strings = {};
        std::string string;

        while(string_Stream >> string) {
            strings.push_back(string);
        }

        if(strings.size() > 2) {
            if(strings[0] == "COINS") {
                Shared::data.coins = std::stoi(strings[2]);
            } else if(strings[0] == "SHOW_FPS") {
                Shared::data.show_Fps.ticked = (bool)std::stoi(strings[2]);
                Shared::data.show_Fps.set = true;
            } else if(strings[0] == "TEST_MODE") {
                Shared::data.test_Mode.ticked = (bool)std::stoi(strings[2]);
                Shared::data.test_Mode.set = true;
            } else if(strings[0] == "MOBILE_MODE") {
                Shared::data.mobile_Mode.ticked = (bool)std::stoi(strings[2]);
                Shared::data.mobile_Mode.set = true;
            } else if(strings[0] == "VOLUME") {
                Shared::data.volume.progress = std::stof(strings[2]);
                Shared::data.volume.set = true;
            } else if(strings[0] == "FPS_LIMIT") {
                Shared::data.max_Fps.progress = std::stof(strings[2]);
                Shared::data.max_Fps.set = true;
            } else if(strings[0] == "FOV") {
                Shared::data.fov.progress = std::stof(strings[2]);
                Menu::data.camera.fovy = std::stof(strings[2]) * 179.f;
                Shared::data.fov.set = true;
            } else if(strings[0] == "SENSITIVITY") {
                Shared::data.sensitivity.progress = std::stof(strings[2]);
                Shared::data.sensitivity.set = true;
            } else if(strings[0] == "RESOLUTION_X") {
                Shared::data.display_Resolution.x = std::stoi(strings[2]);
            } else if(strings[0] == "RESOLUTION_Y") {
                Shared::data.display_Resolution.y = std::stoi(strings[2]);
            } else if(strings[0] == "QUALITY") {
                Shared::data.quality = std::stoi(strings[2]);
            } else if(strings[0] == "TUTORIAL") {
                Shared::data.show_Tutorial.ticked = std::stoi(strings[2]);
                Shared::data.show_Tutorial.set = true;
            }
        }
    }

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    //
    /*
    Vector2 display_Size = {(float)GetScreenWidth(), (float)GetScreenHeight()};

    float max_Width = 1000.f;
    float scale = max_Width / display_Size.x;

    display_Size = Vector2Scale(display_Size, scale);
    window_Downscaling = 1.f / scale;
    */

    float downscaling = 1.f;
    if(Shared::data.quality == 1 /* LOW */) downscaling = 2.f;
    InitWindow(Shared::data.display_Resolution.x / downscaling, Shared::data.display_Resolution.y / downscaling, "Výprava za Pribináčkem");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 600, "Výprava za Pribináčkem");
#endif

    InitAudioDevice();
    
    ChangeDirectory(GetApplicationDirectory());

    Set_Scene_Data({
        {INTRO,   {Intro::Init,   Intro::On_Switch,   Intro::Update}},
        {MENU,    {Menu::Init,    Menu::On_Switch,    Menu::Update}},
        {GAME,    {Game::Init,    Game::On_Switch,    Game::Update}}
    });

    Mission::Init_Missions();

    if(FloatEquals(Shared::data.display_Resolution.x, 0.f))
        Shared::data.display_Resolution = {(float)GetScreenWidth(), (float)GetScreenHeight()};

    SetTargetFPS(60);
    EnableCursor();

    SetExitKey(0);

    cursor = LoadTexture(ASSETS_ROOT "textures/cursor.png");
    SetTextureFilter(cursor, TEXTURE_FILTER_BILINEAR);

    HideCursor();
    
    Mod_Load_Directory(ASSETS_ROOT "mods/");
    Mod_Callback("Init", (void*)&Shared::data);

    Init_Scenes();
    Switch_To_Scene(INTRO);
}

void Update() {
    BeginDrawing(); {
        Update_Scene();

#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_IOS)
        bool show_Cursor = true;
        if(scene == Scene::GAME) show_Cursor = false;
        if(scene == Scene::GAME && Game::data.game_Paused) show_Cursor = true;
        if(scene == Scene::GAME && Game::data.win.tick > Game::data.win.eat_Finish) show_Cursor = true;
        if(scene == Scene::GAME && Game::data.death_Animation_Tick - 2.f > (float)Game::data.death_Animation.size() - 1.f) show_Cursor = true;
        if(scene == Scene::INTRO) show_Cursor = false;
        if(scene == Scene::GAME && Game::data.safe_Animation_Playing) show_Cursor = true;

        if(IsCursorOnScreen() && show_Cursor)
            DrawTextureEx(cursor, Vector2Add(GetMousePosition(), {-10.f, 0.f}), 0.f, 0.5f, WHITE);
#endif

    } EndDrawing();

    if(IsWindowResized()) {
        Game::Init_UI();
        Menu::Init_UI();
    }

    if(IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));

        Game::Init_UI();
        Menu::Init_UI();
    }
}

void Save_Data() {
    Mission::Save_Missions();

    std::string string = "COINS = " + std::to_string(Shared::data.coins) + "\n";
    string += "SHOW_FPS = " + std::to_string(Shared::data.show_Fps.ticked) + "\n";
    string += "TEST_MODE = " + std::to_string(Shared::data.test_Mode.ticked) + "\n";
    string += "MOBILE_MODE = " + std::to_string(Shared::data.mobile_Mode.ticked) + "\n";
    string += "VOLUME = " + std::to_string(Shared::data.volume.progress) + "\n";
    string += "FPS_LIMIT = " + std::to_string(Shared::data.max_Fps.progress) + "\n";
    string += "FOV = " + std::to_string(Shared::data.fov.progress) + "\n";
    string += "SENSITIVITY = " + std::to_string(Shared::data.sensitivity.progress) + "\n";
    string += "RESOLUTION_X = " + std::to_string(Shared::data.display_Resolution.x) + "\n";
    string += "RESOLUTION_Y = " + std::to_string(Shared::data.display_Resolution.y) + "\n";
    string += "QUALITY = " + std::to_string(Shared::data.quality) + "\n";
    string += "TUTORIAL = " + std::to_string(Shared::data.show_Tutorial.ticked) + "\n";
    Shared::Save_To_File(string, "player.txt");
}

#if defined(PLATFORM_ANDROID)
extern "C" {
    void Java_com_zahon_pribinacek_NativeLoader_saveData(JNIEnv *env, jobject this_Object) {
        Save_Data();
    }
}
#endif

void Destroy() {
    CloseWindow();

    Save_Data();
#if defined(PLATFORM_ANDROID)
    Release_Native_Loader();
#endif
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
