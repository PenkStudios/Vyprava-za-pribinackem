#ifndef MENU_CXX
#define MENU_CXX

#include <raylib.h>
#include <raymath.h>

#include "../mod_loader.cpp"
#include "../scene.cpp"

#include "shared.cpp"

namespace Menu {
    class Menu_Data {
    public:
        enum Menu_Scene {MAIN, SETTINGS};
        Menu_Scene scene;

        bool changing_Scene = false;
        float scene_Change_Tick = 0.f;
        Menu_Scene next_Scene;
        Menu_Scene old_Scene;

        std::map<Menu_Scene, Vector3> scene_Perspectives = {
            {MAIN, {0.f, 0.f, -5.f}},
            {SETTINGS, {0.f /* raylib nemá rádo perfektní zezhora dolu pohledy ._. */, 2.f, -0.1f}}
        };

        Camera camera;
        Texture coin;
    } data;

    void Init_UI() {
        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        Shared::data.settings_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 0.f}, "Nastavení", font_Size, Shared::data.medium_Font);
        Shared::data.play_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 1.f}, "Hrát", font_Size, Shared::data.medium_Font);
        
        Shared::data.back_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 1.2f}, "Zpět", font_Size, Shared::data.medium_Font);

        Shared::data.show_Fps = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 4.f}, u8"Ukázat FPS");
        Shared::data.test_Mode = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 2.2f}, u8"Testový mód");
        Shared::data.mobile_Mode = Shared::Shared_Data::TickBox({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 2.2f}, u8"Mobilní mód");

        Shared::data.volume = Shared::Shared_Data::Slider({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 4.f}, u8"Hlasitost");
        Shared::data.max_Fps = Shared::Shared_Data::Slider({GetScreenWidth() / 2.f, GetScreenHeight() / 1.6f}, u8"FPS limiter", 0.167f);
    }

    void Init() {
        Shared::Init();

        #if defined(PLATFORM_IOS) || defined(PLATFORM_ANDROID)
        Shared::data.mobile_Mode.ticked = true;
        #endif

        data.camera.position = data.scene_Perspectives[Menu_Data::MAIN];
        data.camera.target = {0.f, 0.f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.fovy = 45.f;
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
        EnableCursor();
        UpdateModelAnimation(Shared::data.pribinacek, Shared::data.animations[0], 0);

        Mod_Callback("Switch_Menu", (void*)&data);
    }

    void Switch_Scene(Menu_Data::Menu_Scene scene) {
        if(data.changing_Scene || data.scene == scene)
            return;

        data.scene_Change_Tick = 0.f;
        data.changing_Scene = true;

        data.old_Scene = data.scene;
        data.next_Scene = scene;
    }

    void Update() {
        if(data.changing_Scene) {
            data.scene_Change_Tick += 1.f * GetFrameTime();
            data.camera.position = Vector3Lerp(data.scene_Perspectives[data.old_Scene],
                                               data.scene_Perspectives[data.next_Scene],
                                               data.scene_Change_Tick);

            if(data.scene != data.next_Scene && data.scene_Change_Tick > 0.5f) {
                data.scene = data.next_Scene;
            }

            if(data.scene_Change_Tick > 1.f) {
                data.changing_Scene = false;
            }
        }

        ClearBackground(BLACK);

        BeginMode3D(data.camera); {
            DrawModel(Shared::data.house, {-26.5f, -7.5f, -41.f}, 1.f, WHITE);
            DrawModel(Shared::data.pribinacek, {0.f, -0.5f, 0.f}, 1.f, WHITE);
        } EndMode3D();

        SetShaderValue(Shared::data.lighting, Shared::data.lighting.locs[SHADER_LOC_VECTOR_VIEW], &data.camera.position.x, SHADER_UNIFORM_VEC3);

        Shared::data.flashlight.position = data.camera.position;
        UpdateLightValues(Shared::data.lighting, Shared::data.flashlight);

        Mod_Callback("Update_Menu_2D", (void*)&data, false);

        //DrawCircleGradient(GetScreenWidth() / 2, GetScreenHeight() / 2, GetScreenHeight() / 2.2f, BLANK, BLACK);
        //DrawRing({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, GetScreenHeight() / 2.2f - GetScreenHeight() / 500.f, 10000, 0.f, 360.f, 32, BLACK);

        bool fading_In = data.old_Scene == data.scene;
        unsigned char alpha = fading_In ? Clamp(Remap(data.scene_Change_Tick, 0.f, 0.5f, 255.f, 0.f), 0.f, 255.f) : Clamp(Remap(data.scene_Change_Tick, 0.5f, 1.f, 0.f, 255.f), 0.f, 255.f);

        switch(data.scene) {
            case Menu::Menu_Data::Menu_Scene::MAIN: {
                float font_Size = GetScreenHeight() / 15.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, "Výprava za pribináčkem", {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, WHITE, alpha);

                if(Shared::data.settings_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS);
                if(Shared::data.play_Button.Update(alpha)) Switch_To_Scene(GAME);
                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS: {
                Shared::data.show_Fps.Update(alpha);
                
                Shared::data.test_Mode.Update(alpha);
                Shared::data.settings.debug = Shared::data.test_Mode.ticked;

                if(Shared::data.volume.Update(alpha)) SetMasterVolume(Shared::data.volume.progress);
                Shared::data.mobile_Mode.Update(alpha);

                int fps = Shared::data.max_Fps.progress * 360;
                if(Shared::data.max_Fps.progress > 0.8f) fps = 0;

                if(Shared::data.max_Fps.Update(alpha)) SetTargetFPS(fps);

                if(Shared::data.back_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::MAIN);
                break;
            }
        }

        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 750.f;
        float margin = size * 7.5f;

        DrawTextureEx(data.coin, {(float)GetScreenWidth() - data.coin.width * size - margin, margin}, 0.f, size, WHITE);

        const char* text = TextFormat("%d", Shared::data.coins);
        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, text, data.coin.height * size, 0.f);
        if(!Shared::data.settings.custom_Font) text_Size.y += 12.f;
        Shared::DrawTextExOutline(Shared::data.medium_Font, text, {(float)GetScreenWidth() - text_Size.x / 2.f - data.coin.width * size - margin * 2.f, margin + text_Size.y / 2.f}, data.coin.height * size, 0.f, WHITE);

        Mod_Callback("Update_Menu_2D", (void*)&data, true);

        if(Shared::data.show_Fps.ticked) {
            const char* text = TextFormat("FPS: %d", GetFPS());
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;

            Vector2 size = MeasureTextEx(Shared::data.medium_Font, text, font_Size, 0.f);
            Shared::DrawTextExOutline(Shared::data.medium_Font, text, {GetScreenWidth() - size.x / 2.f - font_Size, GetScreenHeight() - size.y / 2.f - font_Size}, font_Size, 0.f, WHITE);
        }

        /*
        const char *test_Text = "TEXT text y";
        Vector2 test_Size = MeasureTextEx(Shared::data.medium_Font, test_Text, 35.f, 0.f);
        DrawTextEx(Shared::data.medium_Font, test_Text, {0.f, 0.f}, 35.f, 0.f, WHITE);
        DrawRectangleLines(0, 0, (int)(test_Size.x), (int)(test_Size.y), RED);
        */

        if(IsWindowResized()) {
            Init_UI();
        }
    }
};

#endif // MENU_CXX