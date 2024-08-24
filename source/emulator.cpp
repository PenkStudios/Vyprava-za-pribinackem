#ifndef EMULATOR_CPP
#define EMULATOR_CPP

/*
 Mobilní "emulátor"
 ------------------
 Emulátor namespace obsahuje Main
 funkci pro standalone emulování
*/

#include <raylib.h>
#include <raymath.h>

#include <ctime>
#include "scenes/shared.cpp"

namespace Emulator {
    bool Ok_Dialog(const char* title, const char* text) {
        Vector2 screen = {(float)GetScreenWidth(), (float)GetScreenHeight()};
        Vector2 size = {screen.x / 1.1f, screen.y / 3.25f};
        Rectangle rectangle = {screen.x / 2.f - size.x / 2.f, screen.y / 2.f - size.y / 2.f,
                               size.x, size.y};

        DrawRectangleRec(rectangle, WHITE);

        float small_Font_Size = (screen.x + screen.y) / 2.f / 27.f;
        float large_Font_Size = (screen.x + screen.y) / 2.f / 22.5f;

        float margin = (screen.x + screen.y) / 2.f / 20.f;

        DrawTextEx(Shared::data.medium_Font, title,
                   {rectangle.x + margin, rectangle.y + margin},
                   large_Font_Size, 0.f, BLACK);

        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font,
                                          text, small_Font_Size, 0.f);

        DrawTextEx(Shared::data.medium_Font, text,
                   {rectangle.x + margin, rectangle.y + rectangle.height / 2.f - text_Size.y / 2.f},
                   small_Font_Size, 0.f, Color {153, 153, 153, 255});

        Vector2 ok_Position = {rectangle.x + rectangle.width - small_Font_Size * 2.5f, rectangle.y + rectangle.height - small_Font_Size * 2.f};

        DrawTextEx(Shared::data.bold_Font, "OK",
                   ok_Position, small_Font_Size, 0.f, Color {159, 108, 244, 255});

        Rectangle ok_Button_Rectangle = {ok_Position.x, ok_Position.y, small_Font_Size * 1.4f, small_Font_Size};
        if(CheckCollisionPointRec(GetMousePosition(), ok_Button_Rectangle) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            return true;
        }

        return false;
    }

    class Emulator_Data {
    public:
        Texture background;
        Vector2 start_Point;

        Camera2D camera;

        bool open_Animation_Playing = false;
        float open_Animation_Tick = 0.f;
        float start_Offset;

        bool desktop = false;

        class App {
        public:
            Texture icon;
            const char* name;

            bool opening = false;
            float open_Tick = 0.f;

            bool opened = false;

            void Render_Icon(int index);
            App() {}

            virtual void On_Create() = 0;
            virtual void On_Destroy() = 0;

            virtual bool Render_Content_Raw() = 0; // "return false" pro quit aplikace
            void Render_Content();
        };

        #include "poletime.cpp"

        class Mad_Birds : virtual public App {
        public:
            Mad_Birds() { name = "Mad birds"; }

            void On_Create() {}
            void On_Destroy() {}

            bool Render_Content_Raw() {
                return !Ok_Dialog("Chyba", "Mad birds podporuje\npouze verzi operačního\nsystému 6 a výše");
            }
        };

        Poletime poletime;
        Mad_Birds mad_Birds;
    } data;

    void Emulator_Data::App::Render_Icon(int index) {
        Vector2 screen = {(float)GetScreenWidth(), (float)GetScreenHeight()};
        float margin = (screen.x + screen.y) / 100.f;
        float size = (screen.x + screen.y) / 15.f;
        Rectangle target_Rectangle = {margin + (margin + size) * index, margin - screen.y, size, size};
    
        if(opening) {
            target_Rectangle.x = Lerp(target_Rectangle.x, 0.f, open_Tick);
            target_Rectangle.y = Lerp(target_Rectangle.y, -screen.y, open_Tick);
            target_Rectangle.width = Lerp(target_Rectangle.width, screen.x, open_Tick);
            target_Rectangle.height = Lerp(target_Rectangle.height, screen.y, open_Tick);

            open_Tick += GetFrameTime() * 8.f;
            if(open_Tick > 1.f) {
                open_Tick = 1.f;
                opening = false;
                opened = true;

                data.camera.offset = {0.f, 0.f};
                On_Create();
            }
        }

        DrawTexturePro(icon,
                        {0.f, 0.f, (float)icon.width, (float)icon.height},
                        target_Rectangle,
                        Vector2Zero(), 0.f, WHITE);

        float font_Size = (screen.x + screen.y) / 2.f / 30.f;

        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, name,
                                        font_Size, 0.f);

        DrawTextEx(Shared::data.medium_Font, name, {target_Rectangle.x + target_Rectangle.width / 2.f - text_Size.x / 2.f,
                                                    target_Rectangle.y + target_Rectangle.height + margin / 4.f},
                                                    font_Size, 0.f, WHITE);
    
        if(CheckCollisionPointRec(GetScreenToWorld2D(GetMousePosition(), data.camera), target_Rectangle) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !opening) {
            opening = true;
            open_Tick = 0.f;
        }
    }

    void Emulator_Data::App::Render_Content() {
        if(!Render_Content_Raw()) {
            On_Destroy();
            open_Tick = 0.f;
            opened = false;
            data.camera.offset.y = (float)GetScreenHeight();
        }
    }

    void Init() {
        data.background = LoadTexture(ASSETS_ROOT "textures/emulator/background.png");
    
        data.camera.offset = {0.f, 0.f};
        data.camera.rotation = 0.f;
        data.camera.target = {0.f, 0.f};
        data.camera.zoom = 1.f;

        data.poletime.icon = LoadTexture(ASSETS_ROOT "textures/emulator/poletime.png");
        SetTextureFilter(data.poletime.icon, TEXTURE_FILTER_BILINEAR);

        data.mad_Birds.icon = LoadTexture(ASSETS_ROOT "textures/emulator/mad birds.png");
        SetTextureFilter(data.mad_Birds.icon, TEXTURE_FILTER_BILINEAR);
    }

    void Update_Desktop() {
        if(data.mad_Birds.opened) {
            data.mad_Birds.Render_Content();
        } else if(data.poletime.opened) {
            data.poletime.Render_Content();
        } else {
            DrawTexturePro(data.background,
                        {0.f, 0.f, (float)data.background.width, (float)data.background.height},
                        {0.f, -(float)GetScreenHeight(), (float)GetScreenWidth(), (float)GetScreenHeight()},
                        Vector2Zero(), 0.f, WHITE);

            data.mad_Birds.Render_Icon(0);
            data.poletime.Render_Icon(1);
        }
    }

    void Update_Home_Screen() {
        DrawTexturePro(data.background,
                    {0.f, 0.f, (float)data.background.width, (float)data.background.height},
                    {0.f, 0.f, (float)GetScreenWidth(), (float)GetScreenHeight()},
                    Vector2Zero(), 0.f, WHITE);

        time_t current_Time;
        struct tm *local_Time;

        time(&current_Time);
        local_Time = localtime(&current_Time);

        int hour = local_Time->tm_hour;
        int minute = local_Time->tm_min;

        float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 5.f;
        
        char text[30];
        strftime(text, 30, "%H:%M", local_Time);

        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, text,
                                        font_Size, 0.f);

        DrawTextEx(Shared::data.medium_Font, text,
                {GetScreenWidth() / 2.f - text_Size.x / 2.f,
                GetScreenHeight() / 3.f - text_Size.y / 2.f},
                font_Size, 0.f, WHITE);

        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            data.start_Point = GetMousePosition();
        }

        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !data.open_Animation_Playing) {
            Vector2 end_Point = GetMousePosition();
            if(data.start_Point.y > end_Point.y) {
                data.camera.offset.y = fabs(data.start_Point.y - end_Point.y);
            }
        }

        if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if(data.camera.offset.y > GetScreenHeight() / 2.f) {
                data.open_Animation_Playing = true;
                data.open_Animation_Tick = 0.f;
                data.start_Offset = data.camera.offset.y;
            } else {
                data.camera.offset.y = 0.f;
            }
        }

        if(data.open_Animation_Playing) {
            data.open_Animation_Tick += GetFrameTime() * 4.f;
            data.camera.offset.y = EaseSineInOut(data.open_Animation_Tick, data.start_Offset, GetScreenHeight() - data.start_Offset, 1.f);
            if(data.open_Animation_Tick > 1.f) {
                data.open_Animation_Tick = 0.f;
                data.open_Animation_Playing = false;
                data.desktop = true;
            }
        }

        if(data.camera.offset.y > 0.1f) {
            Update_Desktop();
        }
    }

    void Update() {
        ClearBackground(BLACK);
        BeginMode2D(data.camera); {
            if(data.desktop) {
                Update_Desktop();
            } else {
                Update_Home_Screen();
            }
        }; EndMode2D();
    }

    int Main() {
        // 9:16
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(350, 600, "Standalone emulátor");
        Init();

        Shared::Init();

        SetTargetFPS(30);
        while(!WindowShouldClose()) {
            BeginDrawing(); {
                Update();
                DrawFPS(0, 0);
            } EndDrawing();
        }

        CloseWindow();
        return 0;
    }
};

#endif // EMULATOR_CPP