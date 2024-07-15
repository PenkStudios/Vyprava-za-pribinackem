#include <raylib.h>
#include <time.h>
#include <iterator>
#include "scene.cpp"
#include "mod_loader.cpp"

#ifdef PLATFORM_ANDROID
#include "ad.cpp"
#define ASSETS_ROOT ""
#else
#define ASSETS_ROOT "assets/"
#endif

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

// Velice kvalitní zabezpečení
std::vector<unsigned char> Encrypt(std::string string) {
    std::vector<unsigned char> output {};
    int index = 0;
    for(char character : string) {
        output.push_back(index % 2 == 0 ? (character ^ 'Z') : (character ^ 'W'));
        index++;
    }
    return output;
}

std::string Decrypt(std::vector<unsigned char> data) {
    std::string output = "";
    int index = 0;
    for(unsigned char byte : data) {
        output.push_back(index % 2 == 0 ? (byte ^ 'Z') : (byte ^ 'W'));
        index++;
    }
    return output;
}

#ifdef PLATFORM_ANDROID
#include <android_native_app_glue.h>
extern "C" android_app* GetAndroidApp();
#endif

void Ready() {
    srand(time(NULL));

    SetConfigFlags(FLAG_MSAA_4X_HINT);

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    InitWindow(0, 0, "Výprava za pribináčkem");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 600, "Výprava za pribináčkem");
#endif

    InitAudioDevice();
    
    ChangeDirectory(GetApplicationDirectory());

    Set_Scene_Data({
        {INTRO,   {Intro::Init,   Intro::On_Switch,   Intro::Update}},
        {MENU,    {Menu::Init,    Menu::On_Switch,    Menu::Update}},
        {GAME,    {Game::Init,    Game::On_Switch,    Game::Update}}
    });

    SetTargetFPS(60);
    EnableCursor();

    SetExitKey(0);

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    std::string root = std::string(GetAndroidApp()->activity->internalDataPath) + "/";
#else
    std::string root = ASSETS_ROOT;
#endif

    std::fstream data_Stream(root + "player.txt", std::ios::in | std::ios::binary);
    data_Stream.unsetf(std::ios::skipws);

    data_Stream.seekg(0, std::ios::end);
    std::streampos file_Size = data_Stream.tellg();
    data_Stream.seekg(0, std::ios::beg);

    std::vector<unsigned char> data;
    data.reserve(Clamp((int)file_Size, 1, 99999));

    data.insert(data.begin(),
               std::istream_iterator<unsigned char>(data_Stream),
               std::istream_iterator<unsigned char>());

    std::string text = Decrypt(data);
    std::istringstream iss(text);

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
            } else if(strings[0] == "TEST_MODE") {
                Shared::data.test_Mode.ticked = (bool)std::stoi(strings[2]);
            } else if(strings[0] == "MOBILE_MODE") {
                Shared::data.mobile_Mode.ticked = (bool)std::stoi(strings[2]);
            } else if(strings[0] == "VOLUME") {
                Shared::data.volume.progress = std::stof(strings[2]);
            } else if(strings[0] == "FPS_LIMIT") {
                Shared::data.max_Fps.progress = std::stof(strings[2]);
            } else if(strings[0] == "FOV") {
                Shared::data.fov.progress = std::stof(strings[2]);
                Menu::data.camera.fovy = std::stof(strings[2]) * 179.f;
            } else if(strings[0] == "SENSITIVITY") {
                Shared::data.sensitivity.progress = std::stof(strings[2]);
            }
        }
    }
    data_Stream.close();

    cursor = LoadTexture(ASSETS_ROOT "textures/cursor.png");
    SetTextureFilter(cursor, TEXTURE_FILTER_BILINEAR);

    HideCursor();
    
    Mod_Load_Directory(ASSETS_ROOT "mods/");
    Mod_Callback("Init", (void*)&Shared::data);

    Init_Scenes();
    Switch_To_Scene(MENU);
}

void Update() {
    BeginDrawing(); {
        Update_Scene();

#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_IOS)
        bool show_Cursor = true;
        if(scene == Scene::GAME) show_Cursor = false;
        if(scene == Scene::GAME && Game::data.game_Paused) show_Cursor = true;
        if(scene == Scene::GAME && Game::data.death_Animation_Tick - 2.f > (float)Game::data.death_Animation.size() - 1.f) show_Cursor = true;

        if(IsCursorOnScreen() && show_Cursor)
            DrawTextureEx(cursor, Vector2Add(GetMousePosition(), {-10.f, 0.f}), 0.f, 0.5f, WHITE);
#endif

    } EndDrawing();

    if(IsWindowResized()) {
        Game::Init_UI();
        Menu::Init_UI();
    }
}

void Save_Data() {
    std::string string = "COINS = " + std::to_string(Shared::data.coins) + "\n";
    string += "SHOW_FPS = " + std::to_string(Shared::data.show_Fps.ticked) + "\n";
    string += "TEST_MODE = " + std::to_string(Shared::data.test_Mode.ticked) + "\n";
    string += "MOBILE_MODE = " + std::to_string(Shared::data.mobile_Mode.ticked) + "\n";
    string += "VOLUME = " + std::to_string(Shared::data.volume.progress) + "\n";
    string += "FPS_LIMIT = " + std::to_string(Shared::data.max_Fps.progress) + "\n";
    string += "FOV = " + std::to_string(Shared::data.fov.progress) + "\n";
    string += "SENSITIVITY = " + std::to_string(Shared::data.sensitivity.progress) + "\n";
    std::vector<unsigned char> data = Encrypt(string);

#if defined(PLATFORM_ANDROID)
    std::string root = std::string(GetAndroidApp()->activity->internalDataPath) + "/";
#else
    std::string root = ASSETS_ROOT;
#endif

    std::fstream data_Stream(root + "player.txt", std::ios::out);
    data_Stream.write((char *)&data[0], data.size());
    data_Stream.close();
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
