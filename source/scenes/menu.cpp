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
void Restart_App() {}
#endif


namespace Menu {
    class Menu_Data {
    public:
        enum Menu_Scene {MAIN, SETTINGS_PAGE_1, SETTINGS_PAGE_2};
        Menu_Scene scene;

        bool changing_Scene = false;
        float scene_Change_Tick = 0.f;
        Menu_Scene next_Scene;
        Menu_Scene old_Scene;

        std::map<Menu_Scene, Vector3> scene_Perspectives = {
            {MAIN, {0.f, 0.f, -5.f}},
            {SETTINGS_PAGE_1, {0.f /* raylib nemá rádo perfektní top-down pohledy ._. */, 2.f, -0.1f}},
            {SETTINGS_PAGE_2, {1.5f, 0.f, 0.f}}
        };

        Camera camera {0};
        Texture coin;
    } data;

    void Init_UI() {
        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        Shared::data.settings_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 0.f}, "Nastavení", font_Size, Shared::data.medium_Font);
        Shared::data.play_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 1.f}, "Hrát", font_Size, Shared::data.medium_Font);
        
        Shared::data.back_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 1.15f}, "Zpět", font_Size, Shared::data.medium_Font);

        Shared::data.show_Fps = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 3.f}, u8"Ukázat FPS", Shared::data.show_Fps.ticked);
        Shared::data.test_Mode = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 1.9f}, u8"Testový mód", Shared::data.test_Mode.ticked);
        Shared::data.mobile_Mode = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 1.9f}, u8"Mobilní mód", Shared::data.mobile_Mode.ticked);

        Shared::data.volume = Shared::Shared_Data::Slider({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 3.f}, u8"Hlasitost", Shared::data.volume.progress);
        Shared::data.max_Fps = Shared::Shared_Data::Slider({GetScreenWidth() / 2.f, GetScreenHeight() / 1.4f}, u8"FPS limiter", FloatEquals(Shared::data.max_Fps.progress, 0.f) ? 0.167f : Shared::data.max_Fps.progress);

        Shared::data.page_2_Button = Shared::Shared_Data::Button({GetScreenWidth() - GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, ">", font_Size, Shared::data.medium_Font);
        Shared::data.page_1_Button = Shared::Shared_Data::Button({GetScreenWidth() / 8.f, GetScreenHeight() / 2.f}, "<", font_Size, Shared::data.medium_Font);

        Shared::data.sensitivity = Shared::Shared_Data::Slider({GetScreenWidth() / 3.f, GetScreenHeight() / 2.5f}, u8"Senzitivita", FloatEquals(Shared::data.sensitivity.progress, 0.f) ? 0.6666f : Shared::data.sensitivity.progress);
        Shared::data.fov = Shared::Shared_Data::Slider({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 2.5f}, u8"FOV", FloatEquals(Shared::data.fov.progress, 0.f) ? 0.5f : Shared::data.fov.progress);

        Shared::data.low_Quality_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f - GetScreenWidth() / 5.f, GetScreenHeight() / 1.33f}, "Nízká", font_Size, Shared::data.medium_Font);
        Shared::data.medium_Quality_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 1.33f}, "Střední", font_Size, Shared::data.medium_Font);
        Shared::data.high_Quality_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f + GetScreenWidth() / 5.f, GetScreenHeight() / 1.33f}, "Vysoká", font_Size, Shared::data.medium_Font);
    
        // Shared::data.mobile_Mode = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 1.9f}, u8"Mobilní mód", Shared::data.mobile_Mode.ticked);
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

        Init_UI();
        Mod_Callback("Init_Menu", (void*)&data);
    }

    void On_Switch() {
        UpdateModelAnimation(Shared::data.pribinacek, Shared::data.animations[0], 0);
        Mod_Callback("Switch_Menu", (void*)&data);
    }

    void Switch_To_Menu_Scene(Menu_Data::Menu_Scene scene) {
        if(data.changing_Scene || data.scene == scene)
            return;

        data.scene_Change_Tick = 0.f;
        data.changing_Scene = true;

        data.old_Scene = data.scene;
        data.next_Scene = scene;
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
            Shared::Draw_Model_Optimized(data.camera.position, Shared::data.house_BBoxes, Shared::data.house, {-26.5f, -7.05f, -41.f}, 1.f, WHITE);
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

                #ifdef DEBUG_TIMER
                t_Breakpoint("MAIN: Renderace textu");
                #endif

                if(Shared::data.settings_Button.Update(alpha))
                    Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_1);
                
                #ifdef DEBUG_TIMER
                t_Breakpoint("MAIN: Tlačítko 1");
                #endif
                
                if(Shared::data.play_Button.Update(alpha)) Switch_To_Scene(GAME);
                
                #ifdef DEBUG_TIMER
                t_Breakpoint("MAIN: Tlačítko 2");
                #endif
                
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

                if(Shared::data.back_Button.Update(alpha))
                    Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::MAIN);

                float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;
                Shared::DrawTextExOutline(Shared::data.medium_Font, "*Hra potřebuje restart po\nmodifikování mobilního módu", {font_Size * 7.25f, (float)GetScreenHeight() - font_Size}, font_Size, 0.f, WHITE, alpha);
                
                Shared::data.page_1_Button.Update(alpha, false);
                if(Shared::data.page_2_Button.Update(alpha))
                    Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_2);
                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_2: {
                float text_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Nastavení hry", {GetScreenWidth() / 2.f, GetScreenHeight() / 8.5f}, text_Font_Size, 0.f, WHITE, alpha);

                Shared::data.sensitivity.Update(alpha, 30.f);
                if(Shared::data.fov.Update(alpha, 180.f)) data.camera.fovy = Shared::data.fov.progress * 179.f;

                float label_Font_Size = GetScreenHeight() / 15.f;
                Shared::DrawTextExOutline(Shared::data.medium_Font, u8"Kvalita grafiky", {GetScreenWidth() / 2.f, GetScreenHeight() / 1.8f}, label_Font_Size, 0.f, WHITE, alpha);
                
                if(Shared::data.low_Quality_Button.Update(alpha, Shared::data.quality != 1)) { Shared::data.quality = 1; Restart_App(); }
                if(Shared::data.medium_Quality_Button.Update(alpha, Shared::data.quality != 2)) { Shared::data.quality = 2; Restart_App(); }
                if(Shared::data.high_Quality_Button.Update(alpha, Shared::data.quality != 3)) { Shared::data.quality = 3; Restart_App(); }

                if(Shared::data.page_1_Button.Update(alpha))
                    Switch_To_Menu_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS_PAGE_1);
                Shared::data.page_2_Button.Update(alpha, false);
                break;
            }
        }

        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 750.f;
        float margin = size * 7.5f;

        DrawTextureEx(data.coin, {(float)GetScreenWidth() - data.coin.width * size - margin, margin}, 0.f, size, WHITE);

        const char* text = TextFormat("%d", Shared::data.coins);
        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, text, data.coin.height * size, 0.f);
        if(!Shared::data.custom_Font) text_Size.y += 12.f;
        Shared::DrawTextExOutline(Shared::data.medium_Font, text, {(float)GetScreenWidth() - text_Size.x / 2.f - data.coin.width * size - margin * 2.f, margin + text_Size.y / 2.f}, data.coin.height * size, 0.f, WHITE);

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