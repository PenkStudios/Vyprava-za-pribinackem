#ifndef MISSION_CPP
#define MISSION_CPP

#include <raylib.h>
#include <vector>
#include <cassert>

#include "scenes/shared.cpp"
#include "reasings.c"

namespace Mission {
    class Best_Time {
    public:
        bool set = false;
        float time = -1.f;

        void Try_Set(float time) {
            if(!set) { this->time = time; set = true; }
            else if(time < this->time) { this->time = time; set = true; }
        }
    };

    class Mission {
    private:
        float popup_Tick;

    public:
        const char* caption;
        std::vector<const char*> text;
        Texture thumbnail;

        Best_Time easy_Time;
        Best_Time normal_Time;
        Best_Time hard_Time;
        Best_Time very_Hard_Time;

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
            switch(Shared::data.game_Difficulty) {
                case 1: {
                    easy_Time.Try_Set(time);
                    break;
                }
                case 2: {
                    normal_Time.Try_Set(time);
                    break;
                }
                case 3: {
                    hard_Time.Try_Set(time);
                    break;
                }
                case 4: {
                    very_Hard_Time.Try_Set(time);
                    break;
                }
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

                Vector2 text_Size = MeasureTextEx(Shared::data.bold_Font, "Mise dokončena", font_Size, 0.f);
                DrawTextEx(Shared::data.bold_Font, "Mise dokončena", {rectangle.x + rectangle.width / 2.f - text_Size.x / 2.f, rectangle.y}, font_Size, 0.f, WHITE);
            
                Vector2 text_Size2 = MeasureTextEx(Shared::data.medium_Font, caption, font_Size, 0.f);
                DrawTextEx(Shared::data.medium_Font, caption, {rectangle.x + rectangle.width / 2.f - text_Size2.x / 2.f, rectangle.y + rectangle.height / 2.f - text_Size2.y / 2.f + text_Size.y / 2.f}, font_Size, 0.f, WHITE);
            }
        }
    };

    std::vector<Mission> missions = {};

    void Draw_Mission_Preview(Mission mission, Vector2 offset, unsigned char alpha = 255) {
        float margin_V = GetScreenWidth() / 5.f;
        float margin_H = GetScreenHeight() / 5.f;
        Rectangle rectangle = {margin_V + offset.x, margin_H + offset.y, GetScreenWidth() - margin_V * 2.f, GetScreenHeight() - margin_H * 2.f};

        float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 30.f;
        DrawRectangleRounded(rectangle, 0.1f, 20, {20, 20, 20, alpha});

        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, mission.caption, font_Size, 0.f);
        DrawTextEx(Shared::data.medium_Font, mission.caption, {rectangle.x + rectangle.width / 2.f - text_Size.x / 2.f, rectangle.y + rectangle.height / 10.f - text_Size.y / 2.f}, font_Size, 0.f, {255, 255, 255, alpha});

        if(mission.easy_Time.set || mission.normal_Time.set || mission.hard_Time.set || mission.very_Hard_Time.set) {
            std::vector<const char*> best_Times = {"Nejlepší časy:"};
            if(mission.easy_Time.set) best_Times.push_back(TextFormat("Lehká: %.2f", mission.easy_Time.time));
            if(mission.normal_Time.set) best_Times.push_back(TextFormat("Normální: %.2f", mission.normal_Time.time));
            if(mission.hard_Time.set) best_Times.push_back(TextFormat("Těžká: %.2f", mission.hard_Time.time));
            if(mission.very_Hard_Time.set) best_Times.push_back(TextFormat("Velmi těžká: %.2f", mission.very_Hard_Time.time));

            Shared::DrawTextExC(Shared::data.medium_Font, mission.text, {rectangle.x + rectangle.width / 4.f, rectangle.y + (best_Times.size() <= 3 ? rectangle.height / 5.f * 2.f : rectangle.height / 3.f)}, font_Size, 0.f, {255, 255, 255, alpha});
            Shared::DrawTextExC(Shared::data.medium_Font, best_Times, {rectangle.x + rectangle.width / 4.f, rectangle.y + (best_Times.size() <= 3 ? rectangle.height / 5.f * 3.f : rectangle.height / 3.f * 2.f)}, font_Size / 1.2f, 0.f, {0, 255, 0, alpha});
        } else {
            Shared::DrawTextExC(Shared::data.medium_Font, mission.text, {rectangle.x + rectangle.width / 4.f, rectangle.y + rectangle.height / 2.f}, font_Size, 0.f, {255, 255, 255, alpha});
        }

        float scale = (rectangle.width / 2.1f) / mission.thumbnail.width;
        DrawTextureEx(mission.thumbnail, {rectangle.x + rectangle.width / 2.f, rectangle.y + rectangle.height / 2.f - (mission.thumbnail.height * scale / 2.f)}, 0.f, scale, {255, 255, 255, alpha});
    }

    float mission_Transition_Tick = 0.f;
    char mission_Transition_Direction = 0;
    float mission_Transition_Target_Tick = 0.f;
    bool buffer = false;

    void Draw_Missions(unsigned char alpha = 255) {
        int mission_Index = 0;
        for(Mission &mission : missions) {
            unsigned char mission_Alpha = alpha;
            mission_Alpha /= Clamp((fabs((mission_Transition_Tick - mission_Index) * 2.f) + 1.f), 0.f, 255.f);
            Draw_Mission_Preview(mission, {((float)mission_Index - mission_Transition_Tick) * (float)GetScreenWidth(), 0.f}, mission_Alpha);
            mission_Index++;
        }

        switch(mission_Transition_Direction) {
            case -1: {
                if(mission_Transition_Tick < mission_Transition_Target_Tick) {
                    mission_Transition_Direction = 0;
                }
                break;
            }

            case 1: {
                if(mission_Transition_Tick > mission_Transition_Target_Tick) {
                    mission_Transition_Direction = 0;
                }
                break;
            }

            default: break;
        }

        bool page_Back_Enabled = mission_Transition_Tick > 0.1f;
        bool page_Forward_Enabled = mission_Transition_Tick < missions.size() - 1.1f;
        if(Shared::data.page_1_Button.Update(alpha, page_Back_Enabled) && page_Back_Enabled) {
            if(mission_Transition_Direction == 0) {
                mission_Transition_Direction = -1;
                mission_Transition_Target_Tick = mission_Transition_Tick - 1.f;
            } else buffer = true;
        }
        if(Shared::data.page_2_Button.Update(alpha, page_Forward_Enabled) && page_Forward_Enabled) {
            if(mission_Transition_Direction == 0) {
                mission_Transition_Direction = 1;
                mission_Transition_Target_Tick = mission_Transition_Tick + 1.f;
            } else buffer = true;
        }

        if(buffer) {
            mission_Transition_Tick = mission_Transition_Target_Tick - ((float)mission_Transition_Direction / 10.f);
            buffer = false;
        } else {
            mission_Transition_Tick += GetFrameTime() * (float)mission_Transition_Direction * 2.f;
        }
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

    void Load_Missions() {
        std::istringstream iss = Shared::Load_From_File("missions.txt");
        float easy, medium, hard, very_Hard;
        bool easy_Set, medium_Set, hard_Set, very_Hard_Set;
        int index = 0;
        
        while (iss >> easy >> easy_Set >> medium >> medium_Set >> hard >> hard_Set >> very_Hard >> very_Hard_Set) {
            missions[index].easy_Time.time = easy;
            missions[index].easy_Time.set = easy_Set;

            missions[index].normal_Time.time = medium;
            missions[index].normal_Time.set = medium_Set;

            missions[index].hard_Time.time = hard;
            missions[index].hard_Time.set = hard_Set;

            missions[index].very_Hard_Time.time = very_Hard;
            missions[index].very_Hard_Time.set = very_Hard_Set;
            index++;
        }
    }

    void Save_Missions() {
        std::string string = "";
        for(Mission &mission : missions) {
            string += std::to_string(mission.easy_Time.time) + " " +
                      std::to_string(mission.easy_Time.set) + " " +

                      std::to_string(mission.normal_Time.time) + " " +
                      std::to_string(mission.normal_Time.set) + " " +

                      std::to_string(mission.hard_Time.time) + " " +
                      std::to_string(mission.hard_Time.set) + " " +

                      std::to_string(mission.very_Hard_Time.time) + " " +
                      std::to_string(mission.very_Hard_Time.set) + "\n";
        }
        Shared::Save_To_File(string, "missions.txt");
    }

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

        Load_Missions();
    }
};

#endif //MISSION_CPP