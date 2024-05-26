#ifndef CREDITS_CXX
#define CREDITS_CXX

#include <raylib.h>
#include "menu.cpp"

#include <vector>
#include <string>
#include <sstream>

namespace Credits {
    class Credits_Data {
    public:
        class Credit_Line {
        public:
            std::string text;
            float size;
            bool magic = false; // ~~text~~

            Credit_Line(std::string line, float size, bool magic) : text(line), size(size), magic(magic) {}
            Credit_Line(std::string line_Raw) {
                if(line_Raw[0] == '~' && line_Raw[1] == '~') {
                    size = 30.f;
                    text = line_Raw.substr(2, line_Raw.size() - 4);
                    magic = true;
                } else if(line_Raw[0] == '~') {
                    size = 45.f;
                    text = line_Raw.substr(1, line_Raw.size() - 2);
                } else {
                    size = 30.f;
                    text = line_Raw;
                }
            }
        };
        std::vector<Credit_Line> credits;
        float y = 0.f;

        class Ball {
        public:
            Vector2 center;
            float size;

            int cooldown = 0;

            Ball();
            // Vrátí true pokud se má koule zničit
            bool Update() {
                cooldown--;
                if(cooldown < 0) {
                    return true;
                }
                return false;
            }
        };
        
        std::vector<Ball> balls;
        int ball_Spawn_Cooldown = 100;

        class Player {
        public:
            Vector2 position;
            Vector2 acceleration;

            bool falling;
            bool dead;

            Player() {
                position = {GetScreenWidth() / 2.f, GetScreenHeight() / 5.f};
                acceleration = {0.f, 0.f};

                falling = false;
                dead = false;
            }

            void Die() {
                acceleration.y = -2.f;
                dead = true;
            }
        } player;
    } data;

    Credits_Data::Ball::Ball() {
        center = {(float)(rand() % (GetScreenWidth() / 2) + GetScreenWidth() / 4), (float)(rand() % (GetScreenHeight() / 2) + GetScreenHeight() / 4 + data.y)};
        size = rand() % 100 + 150;
        cooldown = GetFPS() * 3;
    }

    void Init() {
        std::istringstream credits_File(LoadFileText("credits.txt"));
        std::string line;

        while(std::getline(credits_File, line)) {
            data.credits.push_back(Credits_Data::Credit_Line(line));
        }

        data.player = Credits_Data::Player();
    }

    void On_Switch() {

    }

    int _Utf8_Length(const char *text) {
        int current_Codepoint = -1;
        int next_Character_Index = 0;
        int codepoint_Index = 0;

        while(true) {
            int codepoint_Size = 0;
            if((text + next_Character_Index)[0] == '\0') break;
            int currentCodepoint = GetCodepointNext(text + next_Character_Index, &codepoint_Size);
            codepoint_Index++;
            next_Character_Index += codepoint_Size;  // It could be 1 byte or more
        }

        return codepoint_Index;
    }

    int _Utf8_Index_Codepoint(const char *text, unsigned int index) {
        int current_Codepoint = -1;
        int next_Character_Index = 0;
        int codepoint_Index = 0;

        while(current_Codepoint != 0) {
            int codepointSize = 0;
            int currentCodepoint = GetCodepointNext(text + next_Character_Index, &codepointSize);
            if(codepoint_Index == index) return currentCodepoint;
            codepoint_Index++;
            next_Character_Index += codepointSize;
        }

        return 0;
    }

    bool Collide() {
        if(data.player.dead) return false;

        float y_Pointer = 0.f;
        for(Credits_Data::Credit_Line line : data.credits) {
            Vector2 size = MeasureTextEx(Menu::data.medium_Font, line.text.c_str(), line.size, 0.f);
            Vector2 position = {GetScreenWidth() / 2.f - size.x / 2.f, GetScreenHeight() / 2.f - size.y / 2.f - data.y + y_Pointer};
            
            if(!line.text.empty()) {
                Rectangle line_Hitbox = {position.x, position.y, size.x, size.y};

                if(CheckCollisionCircleRec(Vector2Add(data.player.position, {0.f, -data.y}), 5.f, line_Hitbox)) {
                    return true;
                }
            }
            y_Pointer += size.y;
        }
        return false;
    }

    void Move_In_Steps(int steps) {
        for(int step = 0; step < steps; step++) {
            data.player.position.x += data.player.acceleration.x / steps;

            if(Collide()) {
                data.player.position.x -= data.player.acceleration.x / steps;
                data.player.acceleration.x = 0.f;
            }

            data.player.position.y += data.player.acceleration.y / steps;

            bool collision = Collide();
            if(collision) {
                data.player.position.y -= data.player.acceleration.y / steps;
                data.player.acceleration.y = 0.f;

                data.player.falling = false;
            }

            if(!collision && fabs(data.player.acceleration.y) > 0.5f)
                data.player.falling = true;
        }
    }

    void DrawCircleLinesFancy(Vector2 center, float radius, Color color) {
        for(int angle = 0; angle < 360; angle += 10) {
            Vector2 start = {cosf(angle * DEG2RAD) * radius, sinf(angle * DEG2RAD) * radius};
            Vector2 end = {cosf((angle + 5) * DEG2RAD) * radius, sinf((angle + 5) * DEG2RAD) * radius};

            DrawLineEx(Vector2Add(start, center), Vector2Add(end, center), 2.5f, color);
        }
    }

    void Update() {
        ClearBackground(BLACK);
        data.y += 100.f * GetFrameTime();
        float y_Pointer = 0;
        
        for(int index = 0; index < data.balls.size(); ) {
            float radius = data.balls[index].size + sinf(data.balls[index].cooldown * 0.05f) * 10.f;
            if(data.balls[index].cooldown > GetFPS() * 0.5) {
                DrawCircleLinesFancy(Vector2Add(data.balls[index].center, {0.f, -data.y}), radius, WHITE);
            } else {
                DrawCircleV(Vector2Add(data.balls[index].center, {0.f, -data.y}), radius, RED);
                DrawCircleGradient(data.balls[index].center.x, data.balls[index].center.y - data.y, radius * 2, RED, BLANK);
                if(CheckCollisionCircles(Vector2Add(data.balls[index].center, {0.f, -data.y}), radius, Vector2Add(data.player.position, {0.f, -data.y}), 5.f))
                    data.player.Die();
            }
            if(data.balls[index].Update()) {
                data.balls.erase(std::next(data.balls.begin(), index));
            } else {
                index++;
            }
        }

        if(((data.player.position.y - data.y > GetScreenHeight()) || (data.player.position.y - data.y < 0)) && !data.player.dead)
            data.player.Die();

        DrawRectangleRec({data.player.position.x - 5.f, data.player.position.y - 5.f - data.y, 10.f, 10.f}, WHITE);
    
        if((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && !data.player.falling)
            data.player.acceleration.y = -6.f;

        if(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            data.player.acceleration.x -= 0.5f;

        if(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            data.player.acceleration.x += 0.5f;

        data.player.acceleration.x *= 0.95f;
        data.player.acceleration.y += 0.25f;

        Move_In_Steps(fabs(data.player.acceleration.x) + fabs(data.player.acceleration.y));

        data.ball_Spawn_Cooldown--;
        if(data.ball_Spawn_Cooldown < 0) {
            data.ball_Spawn_Cooldown = GetFPS() * 2;
            data.balls.push_back(Credits_Data::Ball());
        }

        DrawFPS(0, 0);

        for(Credits_Data::Credit_Line line : data.credits) {
            Vector2 size = MeasureTextEx(Menu::data.medium_Font, line.text.c_str(), line.size, 0.f);
            Vector2 position = {GetScreenWidth() / 2.f - size.x / 2.f, GetScreenHeight() / 2.f - size.y / 2.f - data.y + y_Pointer};

            if(line.magic) {
                Vector2 offset = position;
                for(int character = 0; character < _Utf8_Length(line.text.c_str()); character++) {
                    int codepoint = _Utf8_Index_Codepoint(line.text.c_str(), character);
                    
                    const char text[2] = {line.text[character], '\0'};
                    Vector2 character_Size = MeasureTextEx(Menu::data.medium_Font, text, line.size, 0.f);

                    Vector2 current_Offset = offset;

                    float angle = (character + GetTime()) * 6.f;
                    current_Offset = Vector2Add(offset, {cos(angle) * 3.f, sin(angle) * 3.f});
                    
                    Color color = ColorFromHSV(character * 15.f + GetTime() * 120.f, 1.f, 1.f);
                    color = Color {(unsigned char)(((int)color.r + 255) / 2),
                                   (unsigned char)(((int)color.g + 255) / 2),
                                   (unsigned char)(((int)color.b + 255) / 2), 255};
                    DrawTextCodepoint(Menu::data.medium_Font, codepoint, current_Offset, line.size, color);

                    offset.x += character_Size.x;
                }
            } else {
                DrawTextEx(Menu::data.medium_Font, line.text.c_str(), position, line.size, 0.f, WHITE);
            }
            y_Pointer += size.y;
        }
    }
};

#endif // CREDITS_CXX