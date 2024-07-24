#ifndef INTRO_CXX
#define INTRO_CXX

#include <raylib.h>
#include <raymath.h>

#include "../mod_loader.cpp"
#include "../scene.cpp"

namespace Intro {
    class Intro_Data {
    public:
        Sound intro_Sound;
        Texture intro_Texture;

        int tick = 0.f;
        int blink_Countdown = 0;
    } data;
    
    void Init() {
        data.intro_Sound = LoadSound(ASSETS_ROOT "audio/intro.wav");
        data.intro_Texture = LoadTexture(ASSETS_ROOT "textures/logo.png");

        data.tick = 0.f;
        data.blink_Countdown = 0;

        Mod_Callback("Init_Intro", (void*)&data);
    }

    void On_Switch() {
        PlaySound(data.intro_Sound);
        Mod_Callback("Switch_Intro", (void*)&data);
    }

    void Update() {
        ClearBackground(BLACK);

        Mod_Callback("Update_Intro_2D", (void*)&data, false);

        // TODO: Přidat nějaké užitečné komentáře

        data.tick++;
        float opacity_Logo = 0.f;
        float opacity_Light = 0.f;

        if(data.tick < GetFPS() * 2) {
            opacity_Light = 255;
            if(data.blink_Countdown < 0) {
                data.blink_Countdown = rand() % 20;
            } else {
                opacity_Light -= data.blink_Countdown * 10.f;
                data.blink_Countdown--;
            }
            opacity_Light = opacity_Light > 200 ? 255 : 0;
        } else {
            opacity_Light = 255;
        }

        opacity_Logo = 255;

        if(GetFPS() != 0) {
            if(data.tick > GetFPS() * 6) {
                Switch_To_Scene(MENU);
            }
        }

        opacity_Logo = Clamp(opacity_Logo, 0.f, 255.f);
        opacity_Light = Clamp(opacity_Light, 0.f, 255.f);

        float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 1.5f / 800.f;
        DrawEllipse(GetScreenWidth() / 2, GetScreenHeight() / 1.5f, 200.f * size, 100.f * size, Color {255, 255, 255, (unsigned char)opacity_Light});

        DrawTextureEx(data.intro_Texture, {GetScreenWidth() / 2.f - (data.intro_Texture.width * size) / 2.f, GetScreenHeight() / 1.5f - (data.intro_Texture.height * size)}, 0.f, size, Color {255, 255, 255, (unsigned char)((opacity_Logo + opacity_Light) / 2.f)});
        DrawTriangle({GetScreenWidth() / 2.f - 200.f * size, GetScreenHeight() / 1.5f},
                     {GetScreenWidth() / 2.f + 200.f * size, GetScreenHeight() / 1.5f},
                     {GetScreenWidth() / 2.f, 50.f}, Color {255, 255, 255, (unsigned char)(opacity_Light / 4.f)});
        
        DrawTriangle({GetScreenWidth() / 2.f - 200.f * size, GetScreenHeight() / 1.5f},
                     {GetScreenWidth() / 2.f, GetScreenHeight() / 1.5f + 100.f * size},
                     {GetScreenWidth() / 2.f + 200.f * size, GetScreenHeight() / 1.5f}, Color {255, 255, 255, (unsigned char)(opacity_Light / 4.f)});

        if(data.tick < GetFPS() * 1) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color {0, 0, 0, (unsigned char)Remap(data.tick, 0.f, GetFPS() * 1.f, 255.f, 0.f)});
        } else if(data.tick > GetFPS() * 3) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color {0, 0, 0, (unsigned char)Remap(Clamp(data.tick, GetFPS() * 3.f, GetFPS() * 5.f), GetFPS() * 3.f, GetFPS() * 5.f, 0.f, 255.f)});
        }

        Mod_Callback("Update_Intro", (void*)&data, true);
    }
};

#endif // INTRO_CXX