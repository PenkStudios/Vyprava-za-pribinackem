#ifndef MENU_CXX
#define MENU_CXX

#include <raylib.h>
#include <raymath.h>

#include "../mod_loader.cpp"
#include "../scene.cpp"

#include "shared.cpp"

#ifdef PLATFORM_ANDROID
#include "../android.cpp"
#else
// TODO: Restart_App na androidu
void Restart_App() {}
#endif

#include "../mission.cpp"
#include "../multiplayer.cpp"

namespace Menu {
    class Menu_Data {
    public:
        enum Menu_Scene {MAIN, NEW_GAME, MULTIPLAYER, SETTINGS_PAGE_1, SETTINGS_PAGE_2, SETTINGS_PAGE_3, MISSIONS, WAITING_ROOM};
        Menu_Scene scene;

        bool changing_Scene = false;
        float scene_Change_Tick = 0.f;
        Menu_Scene next_Scene;
        Menu_Scene old_Scene;

        std::map<Menu_Scene, Vector3> scene_Perspectives = {
            {MAIN, {0.f, 0.f, -5.f}},
            {NEW_GAME, {1.5f, 0.f, -1.5f}},
            {MULTIPLAYER, {1.5f, 0.f, 0.f}},
            {SETTINGS_PAGE_1, {0.f /* raylib nemá rádo perfektní top-down pohledy ._. */, 2.f, -0.1f}},
            {SETTINGS_PAGE_2, {1.5f, 0.f, 0.f}},
            {SETTINGS_PAGE_3, {0.f, 0.f, 1.5f}},
            {MISSIONS, {1.5f, 0.f, 0.f}},
            {WAITING_ROOM, {0.f, 0.f, 1.5f}}
        };

        Camera camera {0};
        Texture coin;

        Model menu_House;
        Texture back;

        // Multiplayer
        const char *game_Code;
        bool hosting;
    } data;

    void Init_UI() {
        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        Shared::data.settings_Button = Shared::Shared_Data::Button("Nastavení", font_Size, Shared::data.medium_Font);
        Shared::data.new_Game_Button = Shared::Shared_Data::Button("Nová hra", font_Size, Shared::data.medium_Font);
        Shared::data.multiplayer_Button = Shared::Shared_Data::Button("Multiplayer", font_Size, Shared::data.medium_Font);
        Shared::data.mission_Button = Shared::Shared_Data::Button("Mise", font_Size, Shared::data.medium_Font);

        Shared::data.show_Fps = Shared::Shared_Data::Tick_Box({GetScreenWidth() / 3.f, GetScreenHeight() / 2.5f}, u8"Ukázat FPS", Shared::data.show_Fps.ticked);
        Shared::data.test_Mode = Shared::Shared_Data::Tick_Box({GetScreenWidth() / 3.f, GetScreenHeight() / 1.65f}, u8"Testový mód", Shared::data.test_Mode.ticked);
        Shared::data.mobile_Mode = Shared::Shared_Data::Tick_Box({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 1.65f}, u8"Mobilní mód", Shared::data.mobile_Mode.ticked);

        Shared::data.volume = Shared::Shared_Data::Slider({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 2.5f}, u8"Hlasitost", Shared::data.volume.set ? Shared::data.volume.progress : 0.5f);
        Shared::data.max_Fps = Shared::Shared_Data::Slider({GetScreenWidth() / 2.f, GetScreenHeight() / 1.2f}, u8"FPS limiter", Shared::data.max_Fps.set ? Shared::data.max_Fps.progress : 0.167f);

        Shared::data.page_2_Button = Shared::Shared_Data::Button(">", font_Size, Shared::data.medium_Font);
        Shared::data.page_1_Button = Shared::Shared_Data::Button("<", font_Size, Shared::data.medium_Font);

        Shared::data.sensitivity = Shared::Shared_Data::Slider({GetScreenWidth() / 3.f, GetScreenHeight() / 2.5f}, u8"Senzitivita", Shared::data.sensitivity.set ? Shared::data.sensitivity.progress : 0.6666f);
        Shared::data.fov = Shared::Shared_Data::Slider({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 2.5f}, u8"FOV", Shared::data.fov.set ? Shared::data.fov.progress : 0.5f);

        Shared::data.low_Quality_Button = Shared::Shared_Data::Button("Nízká", font_Size, Shared::data.medium_Font);
        Shared::data.medium_Quality_Button = Shared::Shared_Data::Button("Střední", font_Size, Shared::data.medium_Font);
        Shared::data.high_Quality_Button = Shared::Shared_Data::Button("Vysoká", font_Size, Shared::data.medium_Font);

        Shared::data.easy_Difficulty_Button = Shared::Shared_Data::Button("Lehká", font_Size, Shared::data.medium_Font);
        Shared::data.medium_Difficulty_Button = Shared::Shared_Data::Button("Střední", font_Size, Shared::data.medium_Font);
        Shared::data.hard_Difficulty_Button = Shared::Shared_Data::Button("Těžká", font_Size, Shared::data.medium_Font);
        Shared::data.very_Hard_Difficulty_Button = Shared::Shared_Data::Button("Velmi těžká", font_Size, Shared::data.medium_Font);

        // Shared::data.mobile_Mode = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 1.9f}, u8"Mobilní mód", Shared::data.mobile_Mode.ticked);
        Shared::data.show_Tutorial = Shared::Shared_Data::Tick_Box({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, u8"Zapnout tutoriál", Shared::data.show_Tutorial.set ? Shared::data.show_Tutorial.ticked : true);
    
        Shared::data.play_Button = Shared::Shared_Data::Button("Hrát", font_Size, Shared::data.medium_Font);
        
        if(Shared::data.multiplayer_Tabs.set) {
            Shared::data.multiplayer_Tabs = Shared::Shared_Data::Tab_Selection({"Připojit se do hry", "Vytvořit hru"},
                                                                                Shared::data.multiplayer_Tabs.selection);
        } else {
            Shared::data.multiplayer_Tabs = Shared::Shared_Data::Tab_Selection({"Připojit se do hry", "Vytvořit hru"});
        }

        float input_Width = (GetScreenWidth() + GetScreenHeight()) / 2.f / 4.f;

        if(Shared::data.game_Address_Input.set) {
            bool old_Hover = Shared::data.game_Address_Input.hover;
            std::string old_Input = Shared::data.game_Address_Input.input;

            Shared::data.game_Address_Input = Shared::Shared_Data::Input(font_Size, input_Width, 20);

            Shared::data.game_Address_Input.hover = old_Hover;
            Shared::data.game_Address_Input.input = old_Input;
        } else {
            Shared::data.game_Address_Input = Shared::Shared_Data::Input(font_Size, input_Width, 20);
        }

        if(Shared::data.game_Name_Input.set) {
            bool old_Hover = Shared::data.game_Name_Input.hover;
            std::string old_Input = Shared::data.game_Name_Input.input;

            Shared::data.game_Name_Input = Shared::Shared_Data::Input(font_Size, input_Width, 20);

            Shared::data.game_Name_Input.hover = old_Hover;
            Shared::data.game_Name_Input.input = old_Input;
        } else {
            Shared::data.game_Name_Input = Shared::Shared_Data::Input(font_Size, input_Width, 20);
        }

        Shared::data.start_Game_Button = Shared::Shared_Data::Button("Začít hru", font_Size, Shared::data.medium_Font);
    }

    void Init() {
        Shared::Init();

        #if defined(PLATFORM_IOS) || defined(PLATFORM_ANDROID)
        Shared::data.mobile_Mode.ticked = true;
        #endif

        data.camera.position = data.scene_Perspectives[Menu_Data::MAIN];
        data.camera.target = {0.f, 0.f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.fovy = FloatEquals(data.camera.fovy, 0.f) ? 90.f : data.camera.fovy;
        data.camera.projection = CAMERA_PERSPECTIVE;

        data.coin = LoadTexture(ASSETS_ROOT "textures/coin.png");
        SetTextureFilter(data.coin, TEXTURE_FILTER_BILINEAR);
        
        //for(int material = 0; material < data.house.materialCount; material++) {
        //    data.house.materials[material].shader = 
        //}

        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        data.menu_House = LoadModel(ASSETS_ROOT "models/house_menu.glb");
        for(int material = 0; material < data.menu_House.materialCount; material++) {
            data.menu_House.materials[material].shader = Shared::data.lighting;
        }

        data.back = LoadTexture(ASSETS_ROOT "textures/back.png");
        SetTextureFilter(data.back, TEXTURE_FILTER_BILINEAR);

        Shared::data.game_Address_Input.set = false;
        Shared::data.multiplayer_Tabs.set = false;

        Init_UI();
        Mod_Callback("Init_Menu", (void*)&data);
    }

    void Switch_To_Menu_Scene(Menu_Data::Menu_Scene scene) {
        if(data.changing_Scene || data.scene == scene)
            return;

        data.scene_Change_Tick = 0.f;
        data.changing_Scene = true;

        data.old_Scene = data.scene;
        data.next_Scene = scene;
    }

    void Switch_To_Menu_Scene_Instant(Menu_Data::Menu_Scene scene) {
        if(data.scene == scene)
            return;

        data.old_Scene = data.scene;
        data.next_Scene = scene;
        
        data.scene = scene;

        data.scene_Change_Tick = 1.f;
        data.changing_Scene = false;

        data.camera.position = Vector3Lerp(data.scene_Perspectives[data.old_Scene],
                                            data.scene_Perspectives[data.next_Scene],
                                            data.scene_Change_Tick);

        if(data.scene != data.next_Scene && data.scene_Change_Tick > 0.5f) {
            data.scene = data.next_Scene;
        }
    }

    void Switch_To_Menu_Scene_Buffer(Menu_Data::Menu_Scene scene) {
        if(data.next_Scene == scene)
            Switch_To_Menu_Scene_Instant(scene);
        Switch_To_Menu_Scene(scene);
    }

    void On_Switch() {
        Shared::data.game_Difficulty = 2; // normální
        UpdateModelAnimation(Shared::data.pribinacek, Shared::data.animations[0], 0);

        if(Shared::data.play_Again) // hloupý work-around
            Switch_To_Menu_Scene_Instant(Menu::Menu_Data::Menu_Scene::NEW_GAME);
        else
            Switch_To_Menu_Scene_Instant(Menu::Menu_Data::Menu_Scene::MAIN);

        Mod_Callback("Switch_Menu", (void*)&data);
    }

    void Update() {
        #ifdef DEBUG_TIMER
        t_Init();
        #endif

        if(data.changing_Scene) {
            data.scene_Change_Tick += 1.f * GetFrameTime();
            
            if(data.scene_Change_Tick > 1.f) {
                data.scene_Change_Tick = 1.f;
                data.changing_Scene = false;
            }

            data.camera.position = Vector3Lerp(data.scene_Perspectives[data.old_Scene],
                                               data.scene_Perspectives[data.next_Scene],
                                               data.scene_Change_Tick);

            if(data.scene != data.next_Scene && data.scene_Change_Tick > 0.5f) {
                data.scene = data.next_Scene;
            }
        }

        #ifdef DEBUG_TIMER
        t_Breakpoint("Changing scene věci");
        #endif

        ClearBackground(BLACK);

        BeginMode3D(data.camera); {
            DrawModel(data.menu_House, {-26.5f, -7.05f, -41.f}, 1.f, WHITE);
            DrawModel(Shared::data.pribinacek, {0.f, -0.5f, 0.f}, 1.f, WHITE);
        } EndMode3D();

        #ifdef DEBUG_TIMER
        t_Breakpoint("Renderace domu");
        #endif

        SetShaderValue(Shared::data.lighting, Shared::data.lighting.locs[SHADER_LOC_VECTOR_VIEW], &data.camera.position.x, SHADER_UNIFORM_VEC3);

        Shared::data.flashlight.position = data.camera.position;
        UpdateLightValues(Shared::data.lighting, Shared::data.flashlight);

        Mod_Callback("Update_Menu_2D", (void*)&data, false);

        //DrawCircleGradient(GetScreenWidth() / 2, GetScreenHeight() / 2, GetScreenHeight() / 2.2f, BLANK, BLACK);
        //DrawRing({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, GetScreenHeight() / 2.2f - GetScreenHeight() / 500.f, 10000, 0.f, 360.f, 32, BLACK);

        bool fading_In = data.old_Scene == data.scene;
        unsigned char alpha = fading_In ? Clamp(Remap(data.scene_Change_Tick, 0.f, 0.5f, 255.f, 0.f), 0.f, 255.f) : Clamp(Remap(data.scene_Change_Tick, 0.5f, 1.f, 0.f, 255.f), 0.f, 255.f);

        #ifdef DEBUG_TIMER
        t_Breakpoint("Shader věci, flashlight, mod update callback");
        #endif

        switch(data.scene) {
            case Menu::Menu_Data::Menu_Scene::MAIN: {
                float font_Size = GetScreenHeight() / 15.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, "Výprava za Pribináčkem", {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, WHITE, alpha);

                if(Shared::data.settings_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + GetScreenHeight() / 8.f * 0.f}, alpha))
                    Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_1);

                if(Shared::data.mission_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + GetScreenHeight() / 8.f * 3.f}, alpha))
                    Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::MISSIONS);
                
                if(Shared::data.new_Game_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + GetScreenHeight() / 8.f * 1.f}, alpha))
                    Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::NEW_GAME);

                if(Shared::data.multiplayer_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + GetScreenHeight() / 8.f * 2.f}, alpha, false))
                    {} /* Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::MULTIPLAYER); */
                
                break;
            }
            case Menu::Menu_Data::Menu_Scene::NEW_GAME: {
                float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Nová hra", {GetScreenWidth() / 2.f, GetScreenHeight() / 8.5f}, text_Font_Size, 0.f, WHITE, alpha);

                float label_Font_Size = GetScreenHeight() / 15.f;
                Shared::DrawTextExOutline(Shared::data.medium_Font, u8"Obtížnost", {GetScreenWidth() / 2.f, GetScreenHeight() / 2.75f}, label_Font_Size, 0.f, WHITE, alpha);

                if(Shared::data.easy_Difficulty_Button.Update({GetScreenWidth() / 2.f - (GetScreenWidth() / 9.f * 3.f), GetScreenHeight() / 1.8f}, alpha, Shared::data.game_Difficulty != 1)) { Shared::data.game_Difficulty = 1; }
                if(Shared::data.medium_Difficulty_Button.Update({GetScreenWidth() / 2.f - GetScreenWidth() / 8.f, GetScreenHeight() / 1.8f}, alpha, Shared::data.game_Difficulty != 2)) { Shared::data.game_Difficulty = 2; }
                if(Shared::data.hard_Difficulty_Button.Update({GetScreenWidth() / 2.f + GetScreenWidth() / 15.f, GetScreenHeight() / 1.8f}, alpha, Shared::data.game_Difficulty != 3)) { Shared::data.game_Difficulty = 3; }
                if(Shared::data.very_Hard_Difficulty_Button.Update({GetScreenWidth() / 2.f + (GetScreenWidth() / 10.f * 3.f), GetScreenHeight() / 1.8f}, alpha, Shared::data.game_Difficulty != 4)) { Shared::data.game_Difficulty = 4; }

                if(Shared::data.play_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 1.2f}, alpha))
                    Switch_To_Scene(Scene::GAME);

                break;
            }
            case Menu::Menu_Data::Menu_Scene::MULTIPLAYER: {
                switch(Shared::data.multiplayer_Tabs.Update(alpha)) {
                    case 0: {
                        float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                        Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Připojit se do hry", {GetScreenWidth() / 2.f, GetScreenHeight() / 6.5f}, text_Font_Size, 0.f, WHITE, alpha);

                        float label_Font_Size = GetScreenHeight() / 15.f;
                        
                        Shared::DrawTextExOutline(Shared::data.medium_Font, u8"Kód místnosti", {GetScreenWidth() / 2.f - GetScreenWidth() / 6.f, GetScreenHeight() / 2.2f}, label_Font_Size, 0.f, WHITE, alpha);
                        Shared::data.game_Address_Input.Update({GetScreenWidth() / 2.f - GetScreenWidth() / 6.f, GetScreenHeight() / 1.8f}, alpha);

                        Shared::DrawTextExOutline(Shared::data.medium_Font, u8"Jméno", {GetScreenWidth() / 2.f + GetScreenWidth() / 6.f, GetScreenHeight() / 2.2f}, label_Font_Size, 0.f, WHITE, alpha);
                        Shared::data.game_Name_Input.Update({GetScreenWidth() / 2.f + GetScreenWidth() / 6.f, GetScreenHeight() / 1.8f}, alpha);

                        if(Shared::data.play_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 1.2f}, alpha)) {
                            Multiplayer::Join_Room(Shared::data.game_Address_Input.input.c_str());
                            Multiplayer::Set_Name(Shared::data.game_Name_Input.input.c_str());

                            data.game_Code = Shared::data.game_Address_Input.input.c_str();
                            data.hosting = false;

                            Switch_To_Menu_Scene(Menu_Data::Menu_Scene::WAITING_ROOM);
                        }
                        break;
                    }

                    case 1: {
                        float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                        Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Vytvořit hru", {GetScreenWidth() / 2.f, GetScreenHeight() / 6.5f}, text_Font_Size, 0.f, WHITE, alpha);

                        float label_Font_Size = GetScreenHeight() / 15.f;
                        
                        Shared::DrawTextExOutline(Shared::data.medium_Font, u8"Obtížnost", {GetScreenWidth() / 2.f, GetScreenHeight() / 3.4f}, label_Font_Size, 0.f, WHITE, alpha);

                        if(Shared::data.easy_Difficulty_Button.Update({GetScreenWidth() / 2.f - (GetScreenWidth() / 9.f * 3.f), GetScreenHeight() / 2.5f}, alpha, Shared::data.game_Difficulty != 1)) { Shared::data.game_Difficulty = 1; }
                        if(Shared::data.medium_Difficulty_Button.Update({GetScreenWidth() / 2.f - GetScreenWidth() / 8.f, GetScreenHeight() / 2.5f}, alpha, Shared::data.game_Difficulty != 2)) { Shared::data.game_Difficulty = 2; }
                        if(Shared::data.hard_Difficulty_Button.Update({GetScreenWidth() / 2.f + GetScreenWidth() / 15.f, GetScreenHeight() / 2.5f}, alpha, Shared::data.game_Difficulty != 3)) { Shared::data.game_Difficulty = 3; }
                        if(Shared::data.very_Hard_Difficulty_Button.Update({GetScreenWidth() / 2.f + (GetScreenWidth() / 10.f * 3.f), GetScreenHeight() / 2.5f}, alpha, Shared::data.game_Difficulty != 4)) { Shared::data.game_Difficulty = 4; }

                        Shared::DrawTextExOutline(Shared::data.medium_Font, u8"Jméno", {GetScreenWidth() / 2.f, GetScreenHeight() / 1.8f}, label_Font_Size, 0.f, WHITE, alpha);
                        Shared::data.game_Name_Input.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 1.5f}, alpha);

                        if(Shared::data.play_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 1.2f}, alpha)) {
                            data.game_Code = Multiplayer::Create_Room();
                            data.hosting = true;
                            Multiplayer::Set_Name(Shared::data.game_Name_Input.input.c_str());

                            Switch_To_Menu_Scene(Menu_Data::Menu_Scene::WAITING_ROOM);
                        }
                        break;
                    }
                }

                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_1: {
                float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Hlavní nastavení", {GetScreenWidth() / 2.f, GetScreenHeight() / 8.5f}, text_Font_Size, 0.f, WHITE, alpha);

                Shared::data.show_Fps.Update(alpha);
                
                Shared::data.test_Mode.Update(alpha);

                if(Shared::data.volume.Update(alpha)) SetMasterVolume(Shared::data.volume.progress);
                Shared::data.mobile_Mode.Update(alpha);

                int fps = Shared::data.max_Fps.progress * 360;
                if(Shared::data.max_Fps.progress > 0.8f) fps = 0;

                if(Shared::data.max_Fps.Update(alpha, 360.f)) SetTargetFPS(fps);

                float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;
                Shared::DrawTextExOutline(Shared::data.medium_Font, "*Hra potřebuje restart po\nmodifikování mobilního módu", {font_Size * 7.25f, (float)GetScreenHeight() - font_Size}, font_Size, 0.f, WHITE, alpha);
                
                Shared::data.page_1_Button.Update({GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, alpha, false);
                if(Shared::data.page_2_Button.Update({GetScreenWidth() - GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, alpha)) {
                    Switch_To_Menu_Scene_Buffer(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_2);
                }
                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_2: {
                float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Nastavení hry", {GetScreenWidth() / 2.f, GetScreenHeight() / 8.5f}, text_Font_Size, 0.f, WHITE, alpha);

                Shared::data.sensitivity.Update(alpha, 30.f);
                if(Shared::data.fov.Update(alpha, 180.f)) data.camera.fovy = Shared::data.fov.progress * 179.f;

                float label_Font_Size = GetScreenHeight() / 15.f;
                Shared::DrawTextExOutline(Shared::data.medium_Font, u8"Kvalita grafiky", {GetScreenWidth() / 2.f, GetScreenHeight() / 1.8f}, label_Font_Size, 0.f, WHITE, alpha);
                
                if(Shared::data.low_Quality_Button.Update({GetScreenWidth() / 2.f - GetScreenWidth() / 5.f, GetScreenHeight() / 1.33f}, alpha, Shared::data.quality != 1)) { Shared::data.quality = 1; Restart_App(); }
                if(Shared::data.medium_Quality_Button.Update({GetScreenWidth() / 2.f, GetScreenHeight() / 1.33f}, alpha, Shared::data.quality != 2)) { Shared::data.quality = 2; Restart_App(); }
                if(Shared::data.high_Quality_Button.Update({GetScreenWidth() / 2.f + GetScreenWidth() / 5.f, GetScreenHeight() / 1.33f}, alpha, Shared::data.quality != 3)) { Shared::data.quality = 3; Restart_App(); }

                if(Shared::data.page_1_Button.Update({GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, alpha))
                    Switch_To_Menu_Scene_Buffer(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_1);
                if(Shared::data.page_2_Button.Update({GetScreenWidth() - GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, alpha))
                    Switch_To_Menu_Scene_Buffer(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_3);
                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_3: {
                float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Jiné nastavení", {GetScreenWidth() / 2.f, GetScreenHeight() / 8.5f}, text_Font_Size, 0.f, WHITE, alpha);

                Shared::data.show_Tutorial.Update(alpha);

                if(Shared::data.page_1_Button.Update({GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, alpha))
                    Switch_To_Menu_Scene_Buffer(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_2);
                Shared::data.page_2_Button.Update({GetScreenWidth() - GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, alpha, false);
                break;
            }
            case Menu::Menu_Data::Menu_Scene::MISSIONS: {
                float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Mise", {GetScreenWidth() / 2.f, GetScreenHeight() / 8.5f}, text_Font_Size, 0.f, WHITE, alpha);

                Mission::Draw_Missions(alpha);

                break;
            }
            case Menu::Menu_Data::Menu_Scene::WAITING_ROOM: {
                float roundness = 0.1f;
                Rectangle main_Rectangle = {
                    GetScreenWidth() / 10.f,
                    GetScreenHeight() / 10.f,
                    GetScreenWidth() - GetScreenWidth() / 10.f * 2.f,
                    GetScreenHeight() - GetScreenHeight() / 10.f * 2.f
                };

                DrawRectangleRounded(main_Rectangle, roundness, 10, Color {20, 20, 20, alpha});

                if(data.hosting) {
                    if(Shared::data.start_Game_Button.Update({main_Rectangle.x + main_Rectangle.width - GetScreenWidth() / 8.5f, main_Rectangle.y + main_Rectangle.height - GetScreenHeight() / 12.f}, alpha)) {
                        Multiplayer::Start_Game();
                    }
                }

                float players_Margin = (GetScreenWidth() + GetScreenHeight()) / 2.f / 50.f;
                float players_Width = (GetScreenWidth() + GetScreenHeight()) / 2.f / 2.5f;

                Rectangle players_Rectangle = {
                    main_Rectangle.x + players_Margin,
                    main_Rectangle.y + players_Margin,
                    players_Width - players_Margin * 2.f,
                    main_Rectangle.height - players_Margin * 2.f
                };

                DrawRectangleRounded(players_Rectangle, roundness * 2.f, 10, Color {50, 50, 50, alpha});

                float spacing = GetScreenHeight() / 240.f;
                float border_Width = GetScreenHeight() / 120.f;

                float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 25.f;
                float text_Margin = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                
                int index = 0;
                for(Multiplayer::Player &player : Multiplayer::players) {
                    Rectangle rectangle = players_Rectangle;
                    
                    rectangle.x += players_Margin;
                    rectangle.y += players_Margin;
                    
                    rectangle.width -= 2.f * players_Margin;
                    rectangle.height -= 2.f * players_Margin;

                    rectangle.height /= 8.f;
                    rectangle.y += (rectangle.height + players_Margin) * index;
                    
                    DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
                    DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Fade(GRAY, (float)alpha / 255.f));
                    DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(BLACK, (float)alpha / 255.f));
                    DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));
                
                    Shared::DrawTextExC(Shared::data.medium_Font, {player.name.c_str()}, {rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height / 2.f}, font_Size, 0.f, BLACK);

                    index++;
                }

                Shared::DrawTextExC(Shared::data.medium_Font, {TextFormat("Kód hry: %s", data.game_Code)}, {main_Rectangle.x + main_Rectangle.width - text_Margin * 3.f, main_Rectangle.y + text_Margin}, font_Size, 0.f, WHITE);

                break;
            }
        }

        if(data.scene == Menu::Menu_Data::SETTINGS_PAGE_1 ||
           data.scene == Menu::Menu_Data::MISSIONS ||
           data.scene == Menu::Menu_Data::NEW_GAME ||
           data.scene == Menu::Menu_Data::MULTIPLAYER) {
            float x = GetScreenWidth() / 10.f;

            Rectangle destination = {
                x / 8.f, x / 8.f,
                x, x
            };

            if(data.scene == Menu::Menu_Data::MULTIPLAYER) {
                destination.y += GetScreenHeight() / 14.f;
            }

            DrawTexturePro(data.back, {0.f, 0.f, (float)data.back.width, (float)data.back.height}, destination, {0.f, 0.f}, 0.f, Color {255, 255, 255, alpha});

            if(CheckCollisionPointRec(GetMousePosition(), destination) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::MAIN);
            }
        }

        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 750.f;
        float margin = size * 7.5f;

        float y_Offset = 0.f;
        
        float tick = 0.f;
        if(data.next_Scene == Menu::Menu_Data::MULTIPLAYER) {
            tick = data.scene_Change_Tick;
        } else if(data.next_Scene == Menu::Menu_Data::MAIN && data.old_Scene == Menu::Menu_Data::MULTIPLAYER) {
            tick = 1.f - data.scene_Change_Tick;
        }
        tick = EaseExpoInOut(tick, 0.f, 1.f, 1.f);
        y_Offset = Lerp(0.f, GetScreenHeight() / 14.f, tick);

        DrawTextureEx(data.coin, {(float)GetScreenWidth() - data.coin.width * size - margin, margin + y_Offset}, 0.f, size, WHITE);

        const char* text = TextFormat("%d", Shared::data.coins);
        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, text, data.coin.height * size, 0.f);
        if(!Shared::data.custom_Font) text_Size.y += GetScreenHeight() / 600.f * 12.f;
        Shared::DrawTextExOutline(Shared::data.medium_Font, text, {(float)GetScreenWidth() - text_Size.x / 2.f - data.coin.width * size - margin * 2.f, margin + text_Size.y / 2.f + y_Offset}, data.coin.height * size, 0.f, WHITE);

        Mod_Callback("Update_Menu_2D", (void*)&data, true);

        if(Shared::data.show_Fps.ticked) {
            const char* text = TextFormat("FPS: %d", GetFPS());
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;

            Vector2 size = MeasureTextEx(Shared::data.medium_Font, text, font_Size, 0.f);
            Shared::DrawTextExOutline(Shared::data.medium_Font, text, {GetScreenWidth() - size.x / 2.f - font_Size, GetScreenHeight() - size.y / 2.f - font_Size}, font_Size, 0.f, WHITE);
        }

        #ifdef DEBUG_TIMER
        t_Breakpoint("Menu věci, které nevím jak pojmenovat ._.");
        #endif

        if(IsKeyPressed(KEY_KP_0)) t_Summary();

        /*
        const char *test_Text = "TEXT text y";
        Vector2 test_Size = MeasureTextEx(Shared::data.medium_Font, test_Text, 35.f, 0.f);
        DrawTextEx(Shared::data.medium_Font, test_Text, {0.f, 0.f}, 35.f, 0.f, WHITE);
        DrawRectangleLines(0, 0, (int)(test_Size.x), (int)(test_Size.y), RED);
        */
    }
};

#endif // MENU_CXX