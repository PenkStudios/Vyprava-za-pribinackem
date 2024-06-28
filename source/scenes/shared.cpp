#ifndef SHARED_CPP
#define SHARED_CPP

#include <raylib.h>
#include <raymath.h>

#define RLIGHTS_IMPLEMENTATION
#include "../rlights.h"

namespace Shared {
    // TODO: opravit font offset
    void DrawTextExC(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
        Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
        DrawTextEx(font, text, Vector2Subtract(position, Vector2Divide(size, {2.f, 2.f})), fontSize, spacing, tint);
    }

    void DrawTextExOutline(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint, unsigned char alpha = 255) {
        for(int angle = 0; angle < 360; angle += 20) {
            Vector2 offset = {cos(angle * DEG2RAD) * 3.f, sin(angle * DEG2RAD) * 3.f};

            DrawTextExC(font, text, Vector2Add(position, offset), fontSize, spacing, Fade(BLACK, (float)alpha / 255.f));
        }
        DrawTextExC(font, text, position, fontSize, spacing, Fade(WHITE, (float)alpha / 255.f));
    }

    Font medium_Font;
    Font bold_Font;

    class Settings {
    public:
        Settings() {}

        bool debug = false;
        bool show_Fps = false;
    } settings;

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

            DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
            DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Fade(GRAY, (float)alpha / 255.f));
            DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(BLACK, (float)alpha / 255.f));
            DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));
        
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

    bool Slider::Update(unsigned char alpha) {
        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
        float spacing = GetScreenHeight() / 240.f;
        float font_Size = size;
        float border_Width = GetScreenHeight() / 120.f;

        Rectangle rectangle = {position.x - size * 5.f / 2.f, position.y - size / 5.f / 2.f, size * 5.f, size / 5.f};
        DrawTextExOutline(medium_Font, caption, {rectangle.x + rectangle.width / 2.f, rectangle.y - size * 1.125f}, font_Size, 0.f, WHITE, alpha);

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
        if(mobile_Mode.ticked) {
            hitbox.x -= size / 2.f;
            hitbox.y -= size / 2.f;
            hitbox.width += size;
            hitbox.height += size;
        }

        if(settings.debug) {
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

    void TickBox::Update(unsigned char alpha) {
        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 20.f;
        float spacing = GetScreenHeight() / 240.f;
        float font_Size = size;
        float border_Width = GetScreenHeight() / 120.f;

        Rectangle rectangle = {position.x - size / 2.f, position.y - size / 2.f, size, size};
        DrawTextExOutline(medium_Font, caption, {rectangle.x + rectangle.width / 2.f, rectangle.y - size / 1.7f}, font_Size, 0.f, WHITE, alpha);

        DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Fade(GRAY, (float)alpha / 255.f));
        if(ticked) {
            DrawLineEx({rectangle.x + rectangle.width / 5.f, rectangle.y + rectangle.height / 2.f}, {rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height - rectangle.width / 5.f}, 4.f, Fade(BLACK, (float)alpha / 255.f));
            DrawLineEx({rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height - rectangle.width / 5.f}, {rectangle.x + rectangle.width - rectangle.width / 5.f, rectangle.y + rectangle.width / 5.f}, 4.f, Fade(BLACK, (float)alpha / 255.f));
        }
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(BLACK, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));

        Rectangle hitbox = rectangle;
        if(mobile_Mode.ticked) {
            hitbox.x -= size / 2.f;
            hitbox.y -= size / 2.f;
            hitbox.width += size;
            hitbox.height += size;
        }

        if(settings.debug) {
            DrawRectangleLinesEx(hitbox, 2.5f, RED);
        }

        if(CheckCollisionPointRec(GetMousePosition(), hitbox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ticked = !ticked;
        }
    }

    Model pribinacek;
    ModelAnimation *animations;
    int animation_Frame_Counter = 0;

    int coins = 0;

    Model house;
    Shader lighting;

    Light flashlight;

    void Init() {
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
        lighting = LoadShader(ASSETS_ROOT "shaders/vertex100.glsl", ASSETS_ROOT "shaders/fragment100.glsl");
#else
        lighting = LoadShader(ASSETS_ROOT "shaders/vertex330.glsl", ASSETS_ROOT "shaders/fragment330.glsl");
#endif

        lighting.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lighting, "matModel");
        lighting.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lighting, "viewPos");

        int ambientLoc = GetShaderLocation(lighting, "ambient");
        float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
        SetShaderValue(lighting, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

        Shared::flashlight = CreateLight(LIGHT_POINT, {0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, WHITE, Shared::lighting);

        pribinacek = LoadModel(ASSETS_ROOT "models/pribinacek.glb");

        for(int material = 0; material < pribinacek.materialCount; material++)
            pribinacek.materials[material].shader = lighting;

        int animation_Count = 0;
        animations = LoadModelAnimations(ASSETS_ROOT "models/pribinacek.glb", &animation_Count);

        medium_Font = LoadFontEx(ASSETS_ROOT "fonts/medium.ttf", 96, nullptr, 0x5ff);
        SetTextureFilter(medium_Font.texture, TEXTURE_FILTER_BILINEAR);

        bold_Font = LoadFontEx(ASSETS_ROOT "fonts/bold.ttf", 96, nullptr, 0x5ff);
        SetTextureFilter(bold_Font.texture, TEXTURE_FILTER_BILINEAR);

        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        settings_Button = Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 0.f}, "Nastavení", font_Size, medium_Font);
        play_Button = Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height * 1.f}, "Hrát", font_Size, medium_Font);
        
        back_Button = Button({GetScreenWidth() / 2.f, GetScreenHeight() / 1.2f}, "Zpět", font_Size, medium_Font);

        show_Fps = TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 4.f}, u8"Ukázat FPS");
        test_Mode = TickBox({GetScreenWidth() / 3.f, GetScreenHeight() / 2.2f}, u8"Testový mód");
        mobile_Mode = TickBox({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 2.2f}, u8"Mobilní mód");

        volume = Slider({GetScreenWidth() / 3.f * 2.f, GetScreenHeight() / 4.f}, u8"Hlasitost");
        max_Fps = Slider({GetScreenWidth() / 2.f, GetScreenHeight() / 1.6f}, u8"FPS limiter", 0.167f);

        house = LoadModel(ASSETS_ROOT "models/house.glb");

        for(int material = 0; material < house.materialCount; material++) {
            SetTextureFilter(house.materials[material].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_BILINEAR);
        }
    }
};

#endif // SHARED_CPP