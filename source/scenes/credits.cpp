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
    } data;

    void Init() {
        std::istringstream credits_File(LoadFileText("credits.txt"));
        std::string line;

        while(std::getline(credits_File, line)) {
            data.credits.push_back(Credits_Data::Credit_Line(line));
        }
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

    void Update() {
        ClearBackground(BLACK);
        data.y += 100.f * GetFrameTime();
        float y_Pointer = 0;
        
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