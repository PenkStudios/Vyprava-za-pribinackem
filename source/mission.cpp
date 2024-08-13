#ifndef MISSION_CPP
#define MISSION_CPP

#include <raylib.h>
#include <vector>
#include <cassert>

#include "scenes/shared.cpp"
#include "reasings.c"

namespace Mission {
    class Mission {
    private:
        float popup_Tick;

    public:
        const char* caption;
        std::vector<const char*> text;
        Texture thumbnail;

        float best_Time = -1.f;
        bool best_Time_Set = false;
        bool done = false;

        Mission(const char* caption, std::vector<const char*> text, const char* thumbnail_Path) :
            caption(caption), text(text), done(false) {
                thumbnail = LoadTexture(thumbnail_Path);
                SetTextureFilter(thumbnail, TEXTURE_FILTER_BILINEAR);
            }

        Mission(const char* caption, std::vector<const char*> text, const char* thumbnail_Path, bool done) :
            caption(caption), text(text), done(done) {
                thumbnail = LoadTexture(thumbnail_Path);
                SetTextureFilter(thumbnail, TEXTURE_FILTER_BILINEAR);
            }

        void Complete(float time) {
            done = true;
            popup_Tick = 0.f;
            if(!best_Time_Set) {
                best_Time = time;
                best_Time_Set = true;
            } else {
                if(time < best_Time) best_Time = time;
            }
        }

        void Render_Popup() {
            if(popup_Tick < 5.f && done) {
                popup_Tick += GetFrameTime();
                Vector2 size = {GetScreenWidth() / 4.f, GetScreenHeight() / 7.f};
                float margin = GetScreenHeight() / 30.f;
                Vector2 position = {0.f, margin};
                
                float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 30.f;

                if(popup_Tick < 1.f) {
                    position.x = EaseBounceOut(popup_Tick, GetScreenWidth(), -size.x - margin, 1.f);
                } else if(popup_Tick > 3.5) {
                    position.x = EaseSineOut(popup_Tick - 3.5f, GetScreenWidth() - size.x - margin, size.x + margin, 1.5f);
                } else {
                    position.x = GetScreenWidth() - size.x - margin;
                }

                Rectangle rectangle = {position.x, position.y, size.x, size.y};

                float spacing = GetScreenHeight() / 240.f;
                float border_Width = GetScreenHeight() / 120.f;

                DrawRectangleRounded(rectangle, 0.3f, 10, Color {20, 20, 20, 255});

                Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, "Mise dokončena", font_Size, 0.f);
                DrawTextEx(Shared::data.medium_Font, "Mise dokončena", {rectangle.x + rectangle.width / 2.f - text_Size.x / 2.f, rectangle.y}, font_Size, 0.f, WHITE);
            
                text_Size = MeasureTextEx(Shared::data.medium_Font, caption, font_Size, 0.f);
                DrawTextEx(Shared::data.medium_Font, caption, {rectangle.x + rectangle.width / 2.f - text_Size.x / 2.f, rectangle.y + rectangle.height / 2.f - text_Size.y / 2.f}, font_Size, 0.f, WHITE);
            }
        }
    };

    std::vector<Mission> missions = {};

    void Init_Missions() {
        // Init_Missions() by mělo být zavoláno akorát v main() jednou
        assert(missions.empty());

        missions = {
            Mission(
                "Večerníček",
                {"Podívej se", "na televizi"},
                ASSETS_ROOT "textures/missions/tv.png"
            ),
            Mission(
                "Komedie",
                {"Udělej si popkorn"},
                ASSETS_ROOT "textures/missions/popcorn.png"
            )
        };
    }

    void Draw_Text_Centered(Font font, std::vector<const char*> lines, Vector2 position, float font_Size, float spacing, Color tint) {
        int line_Index = 0;

        float line_Height = MeasureTextEx(font, "aA", font_Size, spacing).y; 
        float y = position.y - (line_Height * lines.size()) / 2.f;
        for(const char* line : lines) {
            float line_Width = MeasureTextEx(font, line, font_Size, spacing).x;
            DrawTextEx(font, line, {position.x - line_Width / 2.f, y}, font_Size, spacing, tint);
            y += line_Height;
            line_Index++;
        }
    }

    void Draw_Mission_Preview(Mission mission, unsigned char alpha = 255) {
        float margin_V = GetScreenWidth() / 5.f;
        float margin_H = GetScreenHeight() / 5.f;
        Rectangle rectangle = {margin_V, margin_H, GetScreenWidth() - margin_V * 2.f, GetScreenHeight() - margin_H * 2.f};

        float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 30.f;
        DrawRectangleRounded(rectangle, 0.1f, 20, {20, 20, 20, alpha});

        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, mission.caption, font_Size, 0.f);
        DrawTextEx(Shared::data.medium_Font, mission.caption, {rectangle.x + rectangle.width / 2.f - text_Size.x / 2.f, rectangle.y + rectangle.height / 10.f - text_Size.y / 2.f}, font_Size, 0.f, {255, 255, 255, alpha});

        Draw_Text_Centered(Shared::data.medium_Font, mission.text, {rectangle.x + rectangle.width / 4.f, rectangle.y + rectangle.height / 2.f}, font_Size, 0.f, {255, 255, 255, alpha});

        float scale = (rectangle.width / 2.1f) / mission.thumbnail.width;
        DrawTextureEx(mission.thumbnail, {rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height / 2.f - (mission.thumbnail.height * scale / 2.f)}, 0.f, scale, {255, 255, 255, alpha});
    }

    void Complete_Mission(int mission_Index, float time) {
        Mission *mission = &missions[mission_Index];
        mission->Complete(time);
    }

    void Complete_Mission(const char* mission_Caption, float time) {
        int index = 0;
        for(Mission &mission : missions) {
            if(TextIsEqual(mission.caption, mission_Caption)) {
                Complete_Mission(index, time);
                return;
            }
            index++;
        }
    }

    void Update_Mission_Overlay() {
        for(Mission &mission : missions) {
            mission.Render_Popup();
        }
    }
};

#endif //MISSION_CPP