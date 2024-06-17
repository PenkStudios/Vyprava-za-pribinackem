#ifndef MENU_CXX
#define MENU_CXX

#include <raylib.h>
#include <raymath.h>

#include "../mod_loader.cpp"
#include "../scene.cpp"

namespace Menu {
    // Note: Fade() ._.
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

        class TickBox {
        public:
            Vector2 position;
            const char* caption;
            bool ticked = false;

            TickBox() {}

            TickBox(Vector2 position, const char* caption) : position(position), caption(caption) {}
            TickBox(Vector2 position, const char* caption, bool ticked) : position(position), caption(caption), ticked(ticked) {}

            void Update(unsigned char alpha = 255);
        };

        class Slider {
        public:
            Vector2 position;
            const char* caption;

            float progress = 0;
            bool holding = false;

            Slider() {}

            Slider(Vector2 position, const char* caption) : position(position), caption(caption) {}
            Slider(Vector2 position, const char* caption, float default_Progress) : position(position), caption(caption), progress(default_Progress) {}

            bool Update(unsigned char alpha = 255);
        };

        Button settings_Button {};
        Button play_Button {};

        // nastavení
        Button back_Button {};
        TickBox show_Fps {};
        TickBox test_Mode {};
        TickBox mobile_Mode {};

        Slider volume {};
        Slider max_Fps {};

        class Settings {
        public:
            Settings() {}

            bool debug = false;
            bool show_Fps = false;
        } settings;
    } data;

    void DrawTextExC(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
        Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
        DrawTextEx(font, text, Vector2Subtract(position, Vector2Divide(size, {2.f, 2.f})), fontSize, spacing, tint);
    }

    void DrawTextExOutline(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint, unsigned char alpha = 255) {
        for(int angle = 0; angle < 360; angle += 20) {
            Vector2 offset = {cos(angle * DEG2RAD) * 3.f, sin(angle * DEG2RAD) * 3.f};

            DrawTextExC(font, text, Vector2Add(position, offset), fontSize, spacing, Alpha_Modify(BLACK, alpha));
        }
        DrawTextExC(font, text, position, fontSize, spacing, Alpha_Modify(WHITE, alpha));
    }

    bool Menu_Data::Slider::Update(unsigned char alpha) {
        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
        float spacing = GetScreenHeight() / 240.f;
        float font_Size = size;
        float border_Width = GetScreenHeight() / 120.f;

        Rectangle rectangle = {position.x - size * 5.f / 2.f, position.y - size / 5.f / 2.f, size * 5.f, size / 5.f};
        DrawTextExOutline(data.medium_Font, caption, {rectangle.x + rectangle.width / 2.f, rectangle.y - size * 1.125f}, font_Size, 0.f, WHITE, alpha);

        DrawRectangleRounded(rectangle, 0.3f, 10, Alpha_Modify(WHITE, alpha));
        // DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Alpha_Modify(GRAY, alpha));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Alpha_Modify(BLACK, alpha));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Alpha_Modify(WHITE, alpha));

        Vector2 pointer_Position = {rectangle.x + rectangle.width * progress, rectangle.y + rectangle.height / 2.f};
        Rectangle pointer = {pointer_Position.x - size / 5.f / 2.f, pointer_Position.y - size / 2.f, size / 5.f, size};

        DrawRectangleRounded(pointer, 0.3f, 10, Alpha_Modify(GRAY, alpha));
        // DrawRectangleRoundedLinesEx({pointer.x + spacing, pointer.y + spacing, pointer.width - spacing * 3.f, pointer.height - spacing * 3.f}, 0.3f, 10, border_Width, Alpha_Modify(DARKGRAY, alpha));
        DrawRectangleRoundedLinesEx({pointer.x + spacing, pointer.y + spacing, pointer.width - spacing * 2.f, pointer.height - spacing * 2.f}, 0.3f, 10, border_Width, Alpha_Modify(BLACK, alpha));
        DrawRectangleRoundedLinesEx(pointer, 0.3f, 10, border_Width / 1.8f, Alpha_Modify(WHITE, alpha));

        Rectangle hitbox = pointer;
        if(data.mobile_Mode.ticked) {
            hitbox.x -= size / 2.f;
            hitbox.y -= size / 2.f;
            hitbox.width += size;
            hitbox.height += size;
        }

        if(data.settings.debug) {
            DrawRectangleLinesEx(hitbox, 2.5f, RED);
        }

        Vector2 mouse_Position = GetMousePosition();
        bool to_Return = false;

        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if(CheckCollisionPointRec(mouse_Position, hitbox)) {
                holding = true;
            }
        } else {
            holding = false;
            to_Return = true;
        }

        if(holding) {
            progress = Clamp(Remap(mouse_Position.x, position.x - size * 5.f / 2.f, position.x + size * 5.f / 2.f, 0.f, 1.f), 0.f, 1.f);
        }

        return to_Return;
    }

    void Menu_Data::TickBox::Update(unsigned char alpha) {
        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
        float spacing = GetScreenHeight() / 240.f;
        float font_Size = size;
        float border_Width = GetScreenHeight() / 120.f;

        Rectangle rectangle = {position.x - size / 2.f, position.y - size / 2.f, size, size};
        DrawTextExOutline(data.medium_Font, caption, {rectangle.x + rectangle.width / 2.f, rectangle.y - size / 1.7f}, font_Size, 0.f, WHITE, alpha);

        DrawRectangleRounded(rectangle, 0.3f, 10, Alpha_Modify(WHITE, alpha));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Alpha_Modify(GRAY, alpha));
        if(ticked) {
            DrawLineEx({rectangle.x + rectangle.width / 5.f, rectangle.y + rectangle.height / 2.f}, {rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height - rectangle.width / 5.f}, 4.f, Alpha_Modify(BLACK, alpha));
            DrawLineEx({rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height - rectangle.width / 5.f}, {rectangle.x + rectangle.width - rectangle.width / 5.f, rectangle.y + rectangle.width / 5.f}, 4.f, Alpha_Modify(BLACK, alpha));
        }
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Alpha_Modify(BLACK, alpha));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Alpha_Modify(WHITE, alpha));

        Rectangle hitbox = rectangle;
        if(data.mobile_Mode.ticked) {
            hitbox.x -= size / 2.f;
            hitbox.y -= size / 2.f;
            hitbox.width += size;
            hitbox.height += size;
        }

        if(data.settings.debug) {
            DrawRectangleLinesEx(hitbox, 2.5f, RED);
        }

        if(CheckCollisionPointRec(GetMousePosition(), hitbox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ticked = !ticked;
        }
    }

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

        data.show_Fps = Menu_Data::TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 4.f}, u8"Ukázat FPS");
        data.test_Mode = Menu_Data::TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 2.2f}, u8"Testový mód");
        data.mobile_Mode = Menu_Data::TickBox({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 2.2f}, u8"Mobilní mód");

        data.volume = Menu_Data::Slider({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 4.f}, u8"Hlasitost");
        data.max_Fps = Menu_Data::Slider({GetScreenWidth() / 2.f, GetScreenHeight() / 1.6f}, u8"FPS limiter");

        #if defined(PLATFORM_IOS) || defined(PLATFORM_ANDROID)
        data.mobile_Mode.ticked = true;
        #endif

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
                DrawTextExOutline(data.bold_Font, "Výprava za pribináčkem", {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, WHITE, alpha);

                if(data.settings_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::SETTINGS);
                if(data.play_Button.Update(alpha)) Switch_To_Scene(GAME);
                break;
            }
            case Menu::Menu_Data::Menu_Scene::SETTINGS: {
                data.show_Fps.Update(alpha);
                
                data.test_Mode.Update(alpha);
                data.settings.debug = data.test_Mode.ticked;

                if(data.volume.Update(alpha)) SetMasterVolume(data.volume.progress);
                data.mobile_Mode.Update(alpha);

                int fps = data.max_Fps.progress * 360;
                if(data.max_Fps.progress > 0.8f) fps = 0;

                if(data.max_Fps.Update(alpha)) SetTargetFPS(fps);

                if(data.back_Button.Update(alpha)) Switch_Scene(Menu::Menu_Data::Menu_Scene::MAIN);
                break;
            }
        }

        Mod_Callback("Update_Menu_2D", (void*)&data, true);

        if(data.show_Fps.ticked) {
            const char* text = TextFormat("FPS: %d", GetFPS());
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;

            Vector2 size = MeasureTextEx(data.medium_Font, text, font_Size, 0.f);
            DrawTextExOutline(data.medium_Font, text, {GetScreenWidth() - size.x / 2.f - font_Size, size.y / 2.f + font_Size}, font_Size, 0.f, WHITE);
        }
    }
};

#endif // MENU_CXX