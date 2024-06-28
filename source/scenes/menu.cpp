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

    void Init() {
        Shared::Init();

        #if defined(PLATFORM_IOS) || defined(PLATFORM_ANDROID)
        data.mobile_Mode.ticked = true;
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

        Mod_Callback("Init_Menu", (void*)&data);
    }

    void On_Switch() {
        EnableCursor();
        UpdateModelAnimation(Shared::pribinacek, Shared::animations[0], 0);

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
            DrawModel(Shared::house, {-26.5f, -7.5f, -41.f}, 1.f, WHITE);
            DrawModel(Shared::pribinacek, {0.f, -0.5f, 0.f}, 1.f, WHITE);
        } EndMode3D();

        SetShaderValue(Shared::lighting, Shared::lighting.locs[SHADER_LOC_VECTOR_VIEW], &data.camera.position.x, SHADER_UNIFORM_VEC3);

        Shared::flashlight.position = data.camera.position;
        UpdateLightValues(Shared::lighting, Shared::flashlight);

        Mod_Callback("Update_Menu_2D", (void*)&data, false);

        //DrawCircleGradient(GetScreenWidth() / 2, GetScreenHeight() / 2, GetScreenHeight() / 2.2f, BLANK, BLACK);
        //DrawRing({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, GetScreenHeight() / 2.2f - GetScreenHeight() / 500.f, 10000, 0.f, 360.f, 32, BLACK);

        bool fading_In = data.old_Scene == data.scene;
        unsigned char alpha = fading_In ? Clamp(Remap(data.scene_Change_Tick, 0.f, 0.5f, 255.f, 0.f), 0.f, 255.f) : Clamp(Remap(data.scene_Change_Tick, 0.5f, 1.f, 0.f, 255.f), 0.f, 255.f);

        switch(data.scene) {
            case Menu::Menu_Data::Menu_Scene::MAIN: {
                float font_Size = GetScreenHeight() / 15.f;
                Shared::DrawTextExOutline(Shared::bold_Font, "Výprava za pribináčkem", {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, WHITE, alpha);

                if(Shared::settings_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS);
                if(Shared::play_Button.Update(alpha)) Switch_To_Scene(GAME);
                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS: {
                Shared::show_Fps.Update(alpha);
                
                Shared::test_Mode.Update(alpha);
                Shared::settings.debug = Shared::test_Mode.ticked;

                if(Shared::volume.Update(alpha)) SetMasterVolume(Shared::volume.progress);
                Shared::mobile_Mode.Update(alpha);

                int fps = Shared::max_Fps.progress * 360;
                if(Shared::max_Fps.progress > 0.8f) fps = 0;

                if(Shared::max_Fps.Update(alpha)) SetTargetFPS(fps);

                if(Shared::back_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::MAIN);
                break;
            }
        }

        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 750.f;
        float margin = size * 7.5f;

        DrawTextureEx(data.coin, {(float)GetScreenWidth() - data.coin.width * size - margin, margin}, 0.f, size, WHITE);

        const char* text = TextFormat("%d", Shared::coins);
        Vector2 text_Size = MeasureTextEx(Shared::medium_Font, text, data.coin.height * size, 0.f);
        Shared::DrawTextExOutline(Shared::medium_Font, text, {(float)GetScreenWidth() - text_Size.x / 2.f - data.coin.width * size - margin * 2.f, margin + text_Size.y / 2.f}, data.coin.height * size, 0.f, WHITE);

        Mod_Callback("Update_Menu_2D", (void*)&data, true);

        if(Shared::show_Fps.ticked) {
            const char* text = TextFormat("FPS: %d", GetFPS());
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;

            Vector2 size = MeasureTextEx(Shared::medium_Font, text, font_Size, 0.f);
            Shared::DrawTextExOutline(Shared::medium_Font, text, {GetScreenWidth() - size.x / 2.f - font_Size, GetScreenHeight() - size.y / 2.f - font_Size}, font_Size, 0.f, WHITE);
        }
    }
};

#endif // MENU_CXX