#ifndef POLETIME_CPP
#define POLETIME_CPP

#include <raylib.h>
#include <raymath.h>

#include <vector>

class Poletime : virtual public App {
public:
    Poletime() { name = "Poletíme"; }

    Texture backgroundTexture;
    Texture flappyTexture;
    Texture pipeTexture;
    Texture buttonTexture;

    Font font;

    struct FlappyData {
        float y = 0.f;
        float acceleration = 0.f;
        float rotation = 0.f;
    } flappyData;

    struct PipeData {
        float x = 0.f;
        float yCenter = 0.f;
    };

    std::vector<PipeData> pipes;

    enum Scene {MENU, GAME, END};
    Scene scene = MENU;

    void DrawPipes(Vector2 screen, bool update) {
        for(PipeData &pipeData : pipes) {
            Rectangle rectangleTop = {pipeData.x, pipeData.yCenter - 250.f, (float)pipeTexture.width, (float)pipeTexture.height};
            Rectangle rectangleBottom = {pipeData.x, pipeData.yCenter + 250.f, (float)pipeTexture.width, (float)pipeTexture.height};

            DrawTexturePro(pipeTexture, {0.f, 0.f, (float)pipeTexture.width, (float)pipeTexture.height},
                        rectangleTop, {(float)pipeTexture.width / 2.f, (float)pipeTexture.height / 2.f}, 0.f, WHITE);
            
            DrawTexturePro(pipeTexture, {0.f, 0.f, (float)pipeTexture.width, (float)pipeTexture.height},
                        rectangleBottom, {(float)pipeTexture.width / 2.f, (float)pipeTexture.height / 2.f}, 0.f, WHITE);
            
            if(update) {
                pipeData.x -= 100.f * GetFrameTime();
                if(pipeData.x <= -100.f) {
                    pipeData.x = (float)screen.x + 100.f;
                    pipeData.yCenter = screen.y / 2 + (rand() % (int)(screen.y / 4) - (screen.y / 8));
                }

                Rectangle rectangleTopCollision = {rectangleTop.x - rectangleTop.width / 2.f, rectangleTop.y - rectangleTop.height / 2.f, rectangleTop.width, rectangleTop.height};
                Rectangle rectangleBottomCollision = {rectangleBottom.x - rectangleBottom.width / 2.f, rectangleBottom.y - rectangleBottom.height / 2.f, rectangleBottom.width, rectangleBottom.height};
                
                if(CheckCollisionCircleRec({screen.x * 0.33f, flappyData.y}, 35.f, rectangleTopCollision) || CheckCollisionCircleRec({screen.x * 0.33f, flappyData.y}, 35.f, rectangleBottomCollision)) {
                    scene = END;
                }
            }
        }
    }

    void DrawFlappy(Vector2 screen, bool update) {
        DrawTexturePro(flappyTexture, {0.f, 0.f, (float)flappyTexture.width, (float)flappyTexture.height},
                            {screen.x * 0.33f, flappyData.y, (float)flappyTexture.width, (float)flappyTexture.height},
                            {(float)flappyTexture.width / 2.f, (float)flappyTexture.height / 2.f}, flappyData.rotation, WHITE);

        if(update) {
            flappyData.acceleration += 8.f * GetFrameTime();
            if(flappyData.acceleration > 5.f) flappyData.acceleration = 5.f;

            flappyData.y += flappyData.acceleration;

            if(flappyData.acceleration < 0) flappyData.rotation = Remap(flappyData.acceleration, -6.f, 0.f, -90.f, 0.f);
            else flappyData.rotation = Remap(flappyData.acceleration, 0.f, 5.f, 0.f, 90.f);

            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Mouse_On_Mobile()) {
                flappyData.acceleration = -6.f;
            }
        }
    }

    void On_Create(Vector2 screen) {
        backgroundTexture = LoadTexture(ASSETS_ROOT "poletime/background.png");
        flappyTexture = LoadTexture(ASSETS_ROOT "poletime/flappy.png");
        pipeTexture = LoadTexture(ASSETS_ROOT "poletime/pipe.png");
        buttonTexture = LoadTexture(ASSETS_ROOT "poletime/button.png");
        font = LoadFontEx(ASSETS_ROOT "poletime/font.ttf", 96, nullptr, 256);

        flappyData.y = screen.y * 0.5f;
        pipes.push_back(PipeData {(float)screen.x, (float)screen.y / 2.f});
        pipes.push_back(PipeData {(float)screen.x * 1.85f, (float)(screen.y / 2 + (rand() % (int)(screen.y / 4) - (screen.y / 8)))});
    }

    void On_Destroy(Vector2 screen) {
        UnloadTexture(backgroundTexture);
        UnloadTexture(flappyTexture);
        UnloadTexture(pipeTexture);
        UnloadTexture(buttonTexture);
        UnloadFont(font);
    }

    bool Render_Content_Raw(Vector2 screen) {
        DrawTexturePro(backgroundTexture, {0.f, 0.f, (float)backgroundTexture.width, (float)backgroundTexture.height},
                                {0.f, 0.f, (float)screen.x, (float)screen.y}, {0.f, 0.f}, 0.f, WHITE);

        DrawFlappy(screen, scene == GAME);
        DrawPipes(screen, scene == GAME);

        if(scene == MENU) {
            Vector2 size = MeasureTextEx(font, "Klikni pro start", 45.f, 0.f);
            DrawTextEx(font, "Klikni pro start", {(float)screen.x / 2.f - size.x / 2.f, (float)screen.y / 1.75f - size.y / 2.f}, 45.f, 0.f, BLACK);
        
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Mouse_On_Mobile()) {
                flappyData.acceleration = -6.f;
                scene = GAME;
            }
        } else if(scene == END) {
            Vector2 size = MeasureTextEx(font, "Zemrel jsi", 45.f, 0.f);
            DrawTextEx(font, "Zemrel jsi", {(float)screen.x / 2.f - size.x / 2.f, (float)screen.y / 2.f - ((float)screen.y / 6.f) - size.y / 2.f}, 45.f, 0.f, BLACK);

            Vector2 size2 = MeasureTextEx(font, "   v      ", 25.f, 0.f);
            DrawTextEx(font, "   v      ", {(float)screen.x / 2.f - size2.x / 2.f, (float)screen.y / 2.f - ((float)screen.y / 6.f) - size2.y}, 25.f, 0.f, BLACK);
        
            DrawTextureEx(buttonTexture, {(float)screen.x / 2.f - buttonTexture.width / 2.f, (float)screen.y / 2.f - buttonTexture.height / 2.f}, 0.f, 1.f, WHITE);

            Vector2 size3 = MeasureTextEx(font, u8"Hrát znovu", 30.f, 0.f);
            DrawTextEx(font, u8"Hrát znovu", {(float)screen.x / 2.f - size3.x / 2.f, (float)screen.y / 2.f - size3.y / 2.f}, 30.f, 0.f, BLACK);

            DrawTextureEx(buttonTexture, {(float)screen.x / 2.f - buttonTexture.width / 2.f, (float)screen.y / 2.f + ((float)screen.y / 6.f) - buttonTexture.height / 2.f}, 0.f, 1.f, WHITE);

            Vector2 size4 = MeasureTextEx(font, u8"Ukoncit hru", 30.f, 0.f);
            DrawTextEx(font, u8"Ukoncit hru", {(float)screen.x / 2.f - size4.x / 2.f, (float)screen.y / 2.f + ((float)screen.y / 6.f) - size4.y / 2.f}, 30.f, 0.f, BLACK);

            Vector2 size5 = MeasureTextEx(font, "    v     ", 25.f, 0.f);
            DrawTextEx(font, "    v     ", {(float)screen.x / 2.f - size5.x / 2.f, (float)screen.y / 2.f + ((float)screen.y / 6.f) - size5.y}, 25.f, 0.f, BLACK);

            if(CheckCollisionPointRec(Get_Mouse_Position(screen), {(float)screen.x / 2.f - buttonTexture.width / 2.f, (float)screen.y / 2.f - buttonTexture.height / 2.f, (float)buttonTexture.width, (float)buttonTexture.height}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                scene = MENU;

                pipes.clear();

                flappyData.y = screen.y * 0.5f;
                flappyData.rotation = 0.f;
                flappyData.acceleration = 0.f;

                pipes.push_back(PipeData {(float)screen.x, (float)screen.y / 2.f});
                pipes.push_back(PipeData {(float)screen.x * 1.85f, (float)(screen.y / 2 + (rand() % (int)(screen.y / 4) - (screen.y / 8)))});
            }

            if(CheckCollisionPointRec(Get_Mouse_Position(screen), {(float)screen.x / 2.f - buttonTexture.width / 2.f, (float)screen.y / 2.f + ((float)screen.y / 6.f) - buttonTexture.height / 2.f, (float)buttonTexture.width, (float)buttonTexture.height}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                return false;
            }
        }

        return true;
    }
};

#endif // POLETIME_CPP