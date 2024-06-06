#ifndef MENU_CXX
#define MENU_CXX

#include <raylib.h>
#include <raymath.h>

#include "../mod_loader.cpp"
#include "../scene.cpp"

namespace Menu {
    Color Alpha_Modify(Color color, unsigned char alpha) { return ColorTint(color, {255, 255, 255, alpha}); }

    class Menu_Data {
    public:
        enum Menu_Scene {MAIN, SETTINGS};
        Menu_Scene scene;

        bool changing_Scene = false;
        float scene_Change_Tick = 0.f;
        Menu_Scene next_Scene;
        Menu_Scene old_Scene;

        std::map<Menu_Scene, Vector3> scene_Perspectives = {
            {MAIN, {-2.f, 0.f, 0.f}},
            {SETTINGS, {-0.01f /* raylib doesnt like perfect top-down views ._. */, 2.f, 0.f}}
        };

        Model pribinacek;
        ModelAnimation *animations;
        int animation_Frame_Counter = 0;
        Camera camera;

        Font medium_Font;
        Font bold_Font;

        class Button {
        private:
            Vector2 size;
            Rectangle rectangle;
            Font font;

        public:
            Vector2 position;

            const char* text;
            float font_Size;

            Button() {}
            
            Button(Vector2 position, const char* text, float font_Size, Font font) : position(position), text(text), font_Size(font_Size), font(font) {
                size = MeasureTextEx(font, text, font_Size, 0.f);
                rectangle = {position.x - size.x * 1.5f / 2.f, position.y - size.y * 1.5f / 2.f, size.x * 1.5f, size.y * 1.5f};
            }

            bool Update(unsigned char alpha = 255) {
                float spacing = GetScreenHeight() / 240.f;
                float border_Width = GetScreenHeight() / 120.f;

                DrawRectangleRounded(rectangle, 0.3f, 10, Alpha_Modify(WHITE, alpha));
                DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Alpha_Modify(GRAY, alpha));
                DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Alpha_Modify(BLACK, alpha));
                DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Alpha_Modify(WHITE, alpha));
            
                DrawTextEx(font, text, Vector2Subtract(position, {size.x / 2.f, size.y / 2.f}), font_Size, 0.f, BLACK);

                return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rectangle);
            }
        };

        Button settings_Button {};
        Button play_Button {};

        Button back_Button {};
    } data;

    void Init() {
        data.pribinacek = LoadModel(ASSETS_ROOT "models/pribinacek.glb");

        int animation_Count = 0;
        data.animations = LoadModelAnimations(ASSETS_ROOT "models/pribinacek.glb", &animation_Count);

        data.medium_Font = LoadFontEx(ASSETS_ROOT "fonts/medium.ttf", 96, nullptr, 0x5ff);
        SetTextureFilter(data.medium_Font.texture, TEXTURE_FILTER_BILINEAR);

        data.bold_Font = LoadFontEx(ASSETS_ROOT "fonts/bold.ttf", 96, nullptr, 0x5ff);
        SetTextureFilter(data.bold_Font.texture, TEXTURE_FILTER_BILINEAR);

        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        data.settings_Button = Menu_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 0.f}, "Nastavení", font_Size, data.medium_Font);
        data.play_Button = Menu_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 1.f}, "Hrát", font_Size, data.medium_Font);
        
        data.back_Button = Menu_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 1.2f}, "Zpět", font_Size, data.medium_Font);

        Mod_Callback("Init_Menu", (void*)&data);
        
        data.camera.position = {-2.f, 0.f, 0.f};
        data.camera.target = {0.f, 0.f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.fovy = 45.f;
        data.camera.projection = CAMERA_PERSPECTIVE;
    }

    void On_Switch() {
        EnableCursor();

        for(int material = 0; material < data.pribinacek.materialCount; material++) {
            data.pribinacek.materials[material].shader = LoadMaterialDefault().shader;
        }

        UpdateModelAnimation(data.pribinacek, data.animations[0], 0);

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

    void DrawTextExC(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
        Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
        DrawTextEx(font, text, Vector2Subtract(position, Vector2Divide(size, {2.f, 2.f})), fontSize, spacing, tint);
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
            DrawModel(data.pribinacek, {0.f, -0.5f, 0.f}, 1.f, WHITE);
        } EndMode3D();

        Mod_Callback("Update_Menu_2D", (void*)&data, false);

        DrawCircleGradient(GetScreenWidth() / 2, GetScreenHeight() / 2, GetScreenHeight() / 2.2f, BLANK, BLACK);
        DrawRing({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, GetScreenHeight() / 2.2f - GetScreenHeight() / 500.f, 10000, 0.f, 360.f, 32, BLACK);

        bool fading_In = data.old_Scene == data.scene;
        unsigned char alpha = fading_In ? Clamp(Remap(data.scene_Change_Tick, 0.f, 0.5f, 255.f, 0.f), 0.f, 255.f) : Clamp(Remap(data.scene_Change_Tick, 0.5f, 1.f, 0.f, 255.f), 0.f, 255.f);

        switch(data.scene) {
            case Menu::Menu_Data::Menu_Scene::MAIN: {
                float font_Size = GetScreenHeight() / 15.f;
                for(int angle = 0; angle < 360; angle += 20) {
                    Vector2 offset = {cos(angle * DEG2RAD) * 3.f, sin(angle * DEG2RAD) * 3.f};

                    DrawTextExC(data.bold_Font, "Výprava za pribináčkem", Vector2Add({GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, offset), font_Size, 1.f, Alpha_Modify(BLACK, alpha));
                }
                DrawTextExC(data.bold_Font, "Výprava za pribináčkem", {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, Alpha_Modify(WHITE, alpha));

                if(data.settings_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS);
                if(data.play_Button.Update(alpha)) Switch_To_Scene(GAME);
                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS: {
                if(data.back_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::MAIN);
                break;
            }
        }

        Mod_Callback("Update_Menu_2D", (void*)&data, true);
    }
};

#endif // MENU_CXX