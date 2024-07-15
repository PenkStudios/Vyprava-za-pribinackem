#ifndef SHARED_CPP
#define SHARED_CPP

#include <raylib.h>
#include <raymath.h>

#include "../reasings.c"

#define RLIGHTS_IMPLEMENTATION
#include "../rlights.h"

namespace Shared {
    void DrawTextExC(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
        Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
        DrawTextEx(font, text, Vector2Subtract(position, Vector2Divide(size, {2.f, 2.f})), fontSize, spacing, tint);
    }

    void DrawTextExOutline(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint, unsigned char alpha = 255) {
        float outline_Size = (GetScreenWidth() + GetScreenHeight()) / 400.f;
        for(int angle = 0; angle < 360; angle += 20) {
            Vector2 offset = {cos(angle * DEG2RAD) * outline_Size, sin(angle * DEG2RAD) * outline_Size};

            DrawTextExC(font, text, Vector2Add(position, offset), fontSize, spacing, Fade(BLACK, (float)alpha / 255.f));
        }
        DrawTextExC(font, text, position, fontSize, spacing, Fade(WHITE, (float)alpha / 255.f));
    }

    void Draw_Pack(Rectangle rectangle) {
        float spacing = GetScreenHeight() / 240.f;
        float border_Width = GetScreenHeight() / 120.f;
        float roundness = (GetScreenWidth() + GetScreenHeight()) / 2.f / 800.f / 10.f;

        DrawRectangleRounded(rectangle, roundness, 10, Color {10, 10, 10, 255});

        /*
        DrawRectangleRounded(rectangle, roundness, 10, WHITE);
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, roundness, 10, border_Width, GRAY);
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, roundness, 10, border_Width, BLACK);
        DrawRectangleRoundedLinesEx(rectangle, roundness, 10, border_Width / 1.8f, WHITE);
        */
    }

    class Shared_Data {
    public:
        Font medium_Font;
        Font bold_Font;

        bool custom_Font = false;

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
                // float size_X = size.x * 1.5f;
                // float size_X = EaseCubicOut(size.x, 0.f, 300.f, 200.f);
                // float size_X = size.x * 1.2f;
                float size_X = size.x + GetScreenWidth() / 50.f;
                rectangle = {position.x - size_X / 2.f, position.y - size.y * 1.5f / 2.f, size_X, size.y * 1.5f};
            }

            bool Update(unsigned char alpha = 255, bool enabled = true);
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

            bool Update(unsigned char alpha = 255, float display_Multiplier = 1.f);
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

        Button page_2_Button {};
        Button page_1_Button {};

        Slider fov {};
        Slider sensitivity {};

        Model pribinacek;
        ModelAnimation *animations;
        int animation_Frame_Counter = 0;

        int coins = 0;

        Model house;
        Shader lighting;

        Light flashlight;
    } data;

    bool Shared_Data::Button::Update(unsigned char alpha, bool enabled) {
        float spacing = GetScreenHeight() / 240.f;
        float border_Width = GetScreenHeight() / 120.f;

        DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Fade(GRAY, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(enabled ? BLACK : Color {64, 64, 64, 255}, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));
    
        float offset = 0.f;
        if(!data.custom_Font) offset = 4.f;
        DrawTextEx(font, text, Vector2Add(Vector2Subtract(position, {size.x / 2.f, size.y / 2.f}), {0.f, offset}), font_Size, 0.f, enabled ? BLACK : GRAY);

        if(enabled)
            return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rectangle);
        return false;
    }

    bool Shared_Data::Slider::Update(unsigned char alpha, float display_Multiplier) {
        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
        float spacing = GetScreenHeight() / 240.f;
        float font_Size = size;
        float border_Width = GetScreenHeight() / 120.f;

        Rectangle rectangle = {position.x - size * 5.f / 2.f, position.y - size / 5.f / 2.f, size * 5.f, size / 5.f};
        DrawTextExOutline(data.medium_Font, TextFormat("%s: %.2f", caption, progress * display_Multiplier), {rectangle.x + rectangle.width / 2.f, rectangle.y - size * 1.125f}, font_Size, 0.f, WHITE, alpha);

        DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
        // DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Alpha_Modify(GRAY, alpha));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(BLACK, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));

        Vector2 pointer_Position = {rectangle.x + rectangle.width * progress, rectangle.y + rectangle.height / 2.f};
        Rectangle pointer = {pointer_Position.x - size / 5.f / 2.f, pointer_Position.y - size / 2.f, size / 5.f, size};

        DrawRectangleRounded(pointer, 0.3f, 10, Fade(GRAY, (float)alpha / 255.f));
        // DrawRectangleRoundedLinesEx({pointer.x + spacing, pointer.y + spacing, pointer.width - spacing * 3.f, pointer.height - spacing * 3.f}, 0.3f, 10, border_Width, Alpha_Modify(DARKGRAY, alpha));
        DrawRectangleRoundedLinesEx({pointer.x + spacing, pointer.y + spacing, pointer.width - spacing * 2.f, pointer.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(BLACK, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx(pointer, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));

        Rectangle hitbox = pointer;
        if(data.mobile_Mode.ticked) {
            hitbox.x -= size / 2.f;
            hitbox.y -= size / 2.f;
            hitbox.width += size;
            hitbox.height += size;
        }

        if(data.test_Mode.ticked) {
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

    void Shared_Data::TickBox::Update(unsigned char alpha) {
        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
        float spacing = GetScreenHeight() / 240.f;
        float font_Size = size;
        float border_Width = GetScreenHeight() / 120.f;

        Rectangle rectangle = {position.x - size / 2.f, position.y - size / 2.f, size, size};
        DrawTextExOutline(data.medium_Font, caption, {rectangle.x + rectangle.width / 2.f, rectangle.y - size / 1.7f}, font_Size, 0.f, WHITE, alpha);

        DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Fade(GRAY, (float)alpha / 255.f));
        if(ticked) {
            DrawLineEx({rectangle.x + rectangle.width / 5.f, rectangle.y + rectangle.height / 2.f}, {rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height - rectangle.width / 5.f}, 4.f, Fade(BLACK, (float)alpha / 255.f));
            DrawLineEx({rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height - rectangle.width / 5.f}, {rectangle.x + rectangle.width - rectangle.width / 5.f, rectangle.y + rectangle.width / 5.f}, 4.f, Fade(BLACK, (float)alpha / 255.f));
        }
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(BLACK, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));

        Rectangle hitbox = rectangle;
        if(data.mobile_Mode.ticked) {
            hitbox.x -= size / 2.f;
            hitbox.y -= size / 2.f;
            hitbox.width += size;
            hitbox.height += size;
        }

        if(data.test_Mode.ticked) {
            DrawRectangleLinesEx(hitbox, 2.5f, RED);
        }

        if(CheckCollisionPointRec(GetMousePosition(), hitbox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ticked = !ticked;
        }
    }

    void Init() {
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
        data.lighting = LoadShader(ASSETS_ROOT "shaders/vertex100.glsl", ASSETS_ROOT "shaders/fragment100.glsl");
#else
        data.lighting = LoadShader(ASSETS_ROOT "shaders/vertex330.glsl", ASSETS_ROOT "shaders/fragment330.glsl");
#endif

        data.lighting.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(data.lighting, "matModel");
        data.lighting.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(data.lighting, "viewPos");

        int ambientLoc = GetShaderLocation(data.lighting, "ambient");
        float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
        SetShaderValue(data.lighting, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

        data.flashlight = CreateLight(LIGHT_POINT, {0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, WHITE, data.lighting);

        data.pribinacek = LoadModel(ASSETS_ROOT "models/pribinacek.glb");

        for(int material = 0; material < data.pribinacek.materialCount; material++)
            data.pribinacek.materials[material].shader = data.lighting;

        int animation_Count = 0;
        data.animations = LoadModelAnimations(ASSETS_ROOT "models/pribinacek.glb", &animation_Count);

        data.medium_Font = LoadFontEx(ASSETS_ROOT "fonts/medium.ttf", 96, nullptr, 0x5ff);
        SetTextureFilter(data.medium_Font.texture, TEXTURE_FILTER_BILINEAR);

        data.bold_Font = LoadFontEx(ASSETS_ROOT "fonts/bold.ttf", 96, nullptr, 0x5ff);
        SetTextureFilter(data.bold_Font.texture, TEXTURE_FILTER_BILINEAR);

        data.house = LoadModel(ASSETS_ROOT "models/house.glb");

        for(int material = 0; material < data.house.materialCount; material++) {
            SetTextureFilter(data.house.materials[material].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_BILINEAR);
        }
    }
};

#endif // SHARED_CPP