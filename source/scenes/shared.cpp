#ifndef SHARED_CPP
#define SHARED_CPP

#include <raylib.h>
#include <raymath.h>

#define PL_MPEG_IMPLEMENTATION
#include "../pl_mpeg.h"

#include "../reasings.c"

#define RLIGHTS_IMPLEMENTATION
#include "../rlights.h"

// počet vertexů na hlavní části světla
#define LIGHT_BASE_VERTICES 287

#define DEBUG_TIMER

#ifdef DEBUG_TIMER
#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <fstream>
#include <istream>
#include <sstream>
#include <iterator>

double t_Previous;
std::map<std::string, double> t_Breakpoints;

void t_Init() {
    t_Previous = GetTime();
}

void t_Breakpoint(std::string name) {
    double time = GetTime() - t_Previous;
    if(t_Breakpoints.count(name)) {
        t_Breakpoints[name] = (t_Breakpoints[name] + time) / 2.f;
    } else {
        t_Breakpoints[name] = time;
    }
    t_Previous = GetTime();
}

void t_Summary() {
    std::vector<std::pair<std::string, double>> pairs;
    for (auto iterator = t_Breakpoints.begin(); iterator != t_Breakpoints.end(); ++iterator)
        pairs.push_back(*iterator);

    std::sort(pairs.begin(), pairs.end(), [=](std::pair<std::string, double>& a, std::pair<std::string, double>& b)
    {
        return a.second > b.second;
    }
    );

    TraceLog(LOG_INFO, "Time breakpoint summary");

    int place = 1;
    double sum = 0.f;
    for(std::pair<std::string, double> pair : pairs) {
        TraceLog(LOG_INFO, "%d. %s (%f seconds)", place, pair.first.c_str(), pair.second);
        sum += pair.second;
        place++;
    }

    TraceLog(LOG_INFO, "Performance sum: %f, fps: %f", sum, 1000.f / sum);
}
#endif

namespace Shared {
    class Shared_Data {
    public:
        Font medium_Font;
        Font bold_Font;

        Font handwriting;

        bool custom_Font = false;

        class Button {
        private:
            Vector2 size;
            float size_X;
            Font font;

        public:
            const char* text;
            float font_Size;

            Button() {}

            Button(const char* text, float font_Size, Font font) : text(text), font_Size(font_Size), font(font) {
                size = MeasureTextEx(font, text, font_Size, 0.f);
                // float size_X = size.x * 1.5f;
                // float size_X = EaseCubicOut(size.x, 0.f, 300.f, 200.f);
                // float size_X = size.x * 1.2f;
                size_X = size.x + GetScreenWidth() / 50.f;
            }

            bool Update(Vector2 position, unsigned char alpha = 255, bool enabled = true);
        };

        class Tick_Box {
        public:
            Vector2 position;
            const char* caption;
            bool ticked = false;

            bool set = false;

            Tick_Box() {}

            Tick_Box(Vector2 position, const char* caption) : position(position), caption(caption) {}
            Tick_Box(Vector2 position, const char* caption, bool ticked) : position(position), caption(caption), ticked(ticked) {}

            void Update(unsigned char alpha = 255);
        };

        class Slider {
        public:
            Vector2 position;
            const char* caption;

            float progress = 0;
            bool holding = false;
            
            bool set = false;

            Slider() {}

            Slider(Vector2 position, const char* caption) : position(position), caption(caption) {}
            Slider(Vector2 position, const char* caption, float default_Progress) : position(position), caption(caption), progress(default_Progress) {}

            bool Update(unsigned char alpha = 255, float display_Multiplier = 1.f);
        };

        class Tab_Selection {
        public:
            int selection;
            std::vector<std::string> options;

            RenderTexture render_Texture;
            Image processing;
            Texture render;

            bool rendered = false;

            bool set = false;

            void PreRender();

            Tab_Selection(std::vector<std::string> options) : options(options), selection(0), set(true) { PreRender(); }
            Tab_Selection(std::vector<std::string> options, int selection) : options(options), selection(selection), set(true) { PreRender(); }
        
            Tab_Selection() {}

            int Update(unsigned char alpha = 255);
        };

        struct Mpeg_Video {
            plm_t *plm;

            double framerate;
            int samplerate;	

            int width, height;

            plm_frame_t *frame = NULL;
            plm_samples_t *sample = NULL;

            Image imFrame = { 0 };
            Texture texture;

            float baseTime;
            bool playing = false;
            Mpeg_Video() {}

            void Load(const char* path, bool loop) {
                plm = plm_create_with_filename(path);
                framerate = plm_get_framerate(plm);
                samplerate = plm_get_samplerate(plm);

                plm_set_loop(plm, loop);
                plm_set_audio_enabled(plm, 0);

                width = plm_get_width(plm);
                height = plm_get_height(plm);

                imFrame.width = width;
                imFrame.height = height;
                imFrame.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
                imFrame.mipmaps = 1;
                imFrame.data = (unsigned char*)malloc(width * height * 3);
            
                texture = LoadTextureFromImage(imFrame);
            }

            void Play() {
                baseTime = GetTime();
                plm_seek(plm, 0.f, 0);
                playing = true;
            }

            void Stop() {
                playing = false;
            }

            void Pause() {
                playing = false;
            }

            void Resume() {
                playing = true;
            }

            Texture* Update(Music *music = nullptr) {
                if(playing) {
                    double time = (GetTime() - baseTime);
                    if(time > (1.0 / framerate)) {
                        if(music) SeekMusicStream(*music, plm_get_time(plm));

                        baseTime = GetTime();

                        frame = plm_decode_video(plm);
                        if(!frame)
                            return &texture;

                        plm_frame_to_rgb(frame, (unsigned char*)imFrame.data, width * 3);

                        UpdateTexture(texture, imFrame.data);
                    }
                }

                return &texture;
            }
        };

        class Input {
        public:
            bool hover = false;
            std::string input = "";

            Vector2 size;

            bool set = false;

            Vector2 position;
            float font_Size;

            float width;
            int max_Characters;

            float size_X;

            Input() {}

            Input(float font_Size, float width, int max_Characters);

            void Update(Vector2 position, unsigned char alpha);
        };

        Button settings_Button {};
        Button new_Game_Button {};
        Button multiplayer_Button {};
        Button mission_Button {};

        // nastavení
        Tick_Box show_Fps {};
        Tick_Box test_Mode {};
        Tick_Box mobile_Mode {};

        Slider volume {};
        Slider max_Fps {};

        Button page_2_Button {};
        Button page_1_Button {};

        Slider fov {};
        Slider sensitivity {};

        Button low_Quality_Button {};
        Button medium_Quality_Button {};
        Button high_Quality_Button {};

        Button easy_Difficulty_Button {};
        Button medium_Difficulty_Button {};
        Button hard_Difficulty_Button {};
        Button very_Hard_Difficulty_Button {};

        Button play_Button {};

        Button start_Game_Button {};

        Tick_Box show_Tutorial {};

        Tab_Selection multiplayer_Tabs {};

        Input game_Address_Input {};
        Input game_Name_Input {};

        Model pribinacek;
        ModelAnimation *animations;
        int animation_Frame_Counter = 0;

        int coins = 0;

        Model house;
        std::vector<BoundingBox> house_BBoxes;

        Shader lighting;

        Light flashlight;

        Vector2 display_Resolution = {0.f, 0.f};
        int quality;

        float fog_Density = 0.1f;
        Mpeg_Video tv_Video;
        Music tv_Sound;

        int game_Difficulty = 2;
        bool play_Again;
    } data;

    void DrawTextExC(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
        Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
        DrawTextEx(font, text, Vector2Subtract(position, Vector2Divide(size, {2.f, 2.f})), fontSize, spacing, tint);
    }

    void DrawTextExC(Font font, std::vector<const char*> lines, Vector2 position, float font_Size, float spacing, Color tint) {
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

    Shared_Data::Input::Input(float font_Size, float width, int max_Characters) :
                font_Size(font_Size), width(width), max_Characters(max_Characters) {

        size = MeasureTextEx(Shared::data.medium_Font, "A", font_Size, 0.f);
        size.x = width;
        // float size_X = size.x * 1.5f;
        // float size_X = EaseCubicOut(size.x, 0.f, 300.f, 200.f);
        // float size_X = size.x * 1.2f;
        size_X = size.x + GetScreenWidth() / 50.f;

        set = true;
    }

    void Shared_Data::Input::Update(Vector2 position, unsigned char alpha) {
        Rectangle rectangle = {position.x - size_X / 2.f, position.y - size.y * 1.5f / 2.f, size_X, size.y * 1.5f};

        float spacing = GetScreenHeight() / 240.f;
        float border_Width = GetScreenHeight() / 120.f;

        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, input.c_str(), font_Size, 0.f);
        float text_Scale = std::min(width / text_Size.x, 1.f);

        DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Fade(GRAY, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(hover ? BLACK : Color {64, 64, 64, 255}, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));
    
        Vector2 i_Size = MeasureTextEx(Shared::data.medium_Font, "I", text_Scale * font_Size, 0.f);
        float offset = 0.f;
        if(!data.custom_Font) offset = GetScreenHeight() / 600.f * 2.f; // Text correction (kvůli divnýmu fontu)

        DrawTextExC(Shared::data.medium_Font, {input.c_str()}, {rectangle.x + rectangle.width / 2.f - i_Size.x / 2.f, rectangle.y + rectangle.height / 2.f + offset}, text_Scale * font_Size, 0.f, BLACK);

        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(CheckCollisionPointRec(GetMousePosition(), rectangle)) {
                hover = !hover;
                // TODO: Popup klávesnice na androidu
            } else {
                hover = false;
            }
        }

        if(hover) {
            int key = GetCharPressed();

            while (key > 0) {
                text_Size = MeasureTextEx(Shared::data.medium_Font, input.c_str(), text_Scale * font_Size, 0.f);
                bool can_Insert = input.size() < max_Characters;

                if ((key >= 32) && (key <= 125) && can_Insert) {
                    input += (char)key;
                }

                key = GetCharPressed();
            }

            if(IsKeyPressed(KEY_BACKSPACE)) {
                if(!input.empty())
                    input.pop_back();
            }

            text_Size = MeasureTextEx(Shared::data.medium_Font, input.c_str(), text_Scale * font_Size, 0.f);
            if ((int)GetTime() % 2 == 0) DrawTextEx(Shared::data.medium_Font, "I",
                                                    {rectangle.x + rectangle.width / 2.f - i_Size.x / 4.f + text_Size.x / 2.f,
                                                    rectangle.y + rectangle.height / 2.f - i_Size.y / 2.f + (GetScreenHeight() / 200.f * text_Scale) + offset},
                                                    text_Scale * font_Size, 0.f, BLACK);
        }
    }

    void Shared_Data::Tab_Selection::PreRender() {
        if(rendered) {
            UnloadRenderTexture(render_Texture);
            UnloadImage(processing);
            // UnloadTexture(render);
            // ^
            // | Produkuje grafický glitch.
            //   I bez této řádky program tak neleakuje (nebo hodně málo).
        }

        render_Texture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

        BeginTextureMode(render_Texture); {
            ClearBackground(BLANK);
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 25.f;
            float margin = font_Size / 4.f;

            float x_Offset = 0.f;
            int index = 0;

            float roundness = (GetScreenWidth() + GetScreenHeight()) / 2.f / 800.f / 4.f;
            float line_Thick = (GetScreenWidth() + GetScreenHeight()) / 2.f / 200.f;

            Rectangle screen = {
                0.f, font_Size * 1.5f - line_Thick,
                (float)GetScreenWidth(), GetScreenHeight() - font_Size * 1.5f + line_Thick
            };

            DrawRectangleRec(screen, WHITE);
            DrawRectangleLinesEx(screen, line_Thick, BLACK);

            for(std::string option : options) {
                Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, option.c_str(), font_Size, 0.f);

                Rectangle rectangle = {
                    x_Offset + line_Thick, line_Thick, text_Size.x + margin * 2.f, text_Size.y + margin
                };

                DrawRectangleRounded(rectangle, roundness, 10, WHITE);
                DrawRectangleRoundedLinesEx(rectangle, roundness, 10, line_Thick, BLACK);

                // DrawTextEx(Shared::data.medium_Font, option.c_str(), {x_Offset + margin, margin}, font_Size, 0.f, selection == index ? BLACK : GRAY);
                
                if(selection == index) {
                    Rectangle overlay = {
                        line_Thick + x_Offset, text_Size.y * 1.5f - line_Thick * 3.f,
                        text_Size.x + margin * 2.f, line_Thick * 6.f
                    };

                    DrawRectangleRec(overlay, WHITE);
                }

                x_Offset += text_Size.x + margin * 2.f + line_Thick;
                index++;
            }
        } EndTextureMode();
        
        processing = LoadImageFromTexture(render_Texture.texture);
        for(int x = 0; x < processing.width; x++) {
            for(int y = 0; y < processing.height; y++) {
                Color color = GetImageColor(processing, x, y);
                /*inverted = {
                    (unsigned char)(255 - inverted.r),
                    (unsigned char)(255 - inverted.g),
                    (unsigned char)(255 - inverted.b),
                    (unsigned char)(255 - inverted.a)
                };*/
                if(color.r == 255 && color.a == 255) // WHITE
                    color = BLANK;
                else if(color.r == 0 && color.a == 255) // BLACK
                    color = BLACK;
                else if(color.r == 0 && color.a == 0) // BLANK
                    color = WHITE;
                ImageDrawPixelV(&processing, {(float)x, (float)y}, color);
            }
        }
        
        render = LoadTextureFromImage(processing);

        rendered = true;
    }

    int Shared_Data::Tab_Selection::Update(unsigned char alpha) {
        Color tint = Fade(WHITE, (float)(alpha) / 255.f);

        DrawTexturePro(render, {0.f, 0.f, (float)render.width, -(float)render.height}, {0.f, 0.f, (float)GetScreenWidth(), (float)GetScreenHeight()}, {0.f, 0.f}, 0.f, ColorTint(WHITE, tint));

        float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 25.f;
        float margin = font_Size / 4.f;

        float x_Offset = 0.f;
        int index = 0;

        float roundness = (GetScreenWidth() + GetScreenHeight()) / 2.f / 800.f / 4.f;
        float line_Thick = (GetScreenWidth() + GetScreenHeight()) / 2.f / 200.f;

        bool re_Render = false;

        for(std::string option : options) {
            Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, option.c_str(), font_Size, 0.f);

            Rectangle rectangle = {
                x_Offset + line_Thick, line_Thick, text_Size.x + margin * 2.f, text_Size.y + margin
            };

            DrawTextEx(Shared::data.medium_Font, option.c_str(), {x_Offset + margin, margin}, font_Size, 0.f, ColorTint(selection == index ? WHITE : LIGHTGRAY, tint));

            if(CheckCollisionPointRec(GetMousePosition(), rectangle) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                selection = index;
                re_Render = true;
            }

            x_Offset += text_Size.x + margin * 2.f + line_Thick;
            index++;
        }

        if(re_Render) PreRender();

        return selection;
    }

    // Velice kvalitní zabezpečení
    std::vector<unsigned char> Encrypt(std::string string) {
        std::vector<unsigned char> output {};
        int index = 0;
        for(char character : string) {
            output.push_back(index % 2 == 0 ? (character ^ 'Z') : (character ^ 'W'));
            index++;
        }
        return output;
    }

    std::string Decrypt(std::vector<unsigned char> data) {
        std::string output = "";
        int index = 0;
        for(unsigned char byte : data) {
            output.push_back(index % 2 == 0 ? (byte ^ 'Z') : (byte ^ 'W'));
            index++;
        }
        return output;
    }

#if defined(PLATFORM_ANDROID)
    std::string save_Root = std::string(GetAndroidApp()->activity->internalDataPath) + "/";
#else
    std::string save_Root = ASSETS_ROOT;
#endif

    void Save_To_File(std::string string, std::string file) {
        std::vector<unsigned char> data = Encrypt(string);

        std::fstream data_Stream(save_Root + file, std::ios::out);
        data_Stream.write((char *)&data[0], data.size());
        data_Stream.close();
    }

    std::istringstream Load_From_File(std::string file) {
        std::fstream data_Stream(save_Root + file, std::ios::in | std::ios::binary);
        data_Stream.unsetf(std::ios::skipws);

        data_Stream.seekg(0, std::ios::end);
        std::streampos file_Size = data_Stream.tellg();
        data_Stream.seekg(0, std::ios::beg);

        std::vector<unsigned char> data;
        data.reserve(Clamp((int)file_Size, 1, 99999));

        data.insert(data.begin(),
                std::istream_iterator<unsigned char>(data_Stream),
                std::istream_iterator<unsigned char>());
        data_Stream.close();

        std::string text = Decrypt(data);
        return std::istringstream(text);
    }

    float Box_SDF(Vector3 p, Vector3 b) {
        Vector3 q = Vector3Subtract({p.x > -p.x ? p.x : -p.x,
                                        p.y > -p.y ? p.y : -p.y,
                                        p.z > -p.z ? p.z : -p.z}, b);
        return Vector3Length(Vector3Max(q, {0.f, 0.f, 0.f})) + std::min(std::max(q.x, std::max(q.y, q.z)), 0.f);
    }

    int Draw_Model_Optimized(Vector3 viewer_Position, std::vector<BoundingBox> bboxes, Model model, Vector3 position, float scale, Color tint) {
        // Calculate transformation matrix from function parameters
        // Get transform matrix (rotation -> scale -> translation)
        Matrix matScale = MatrixScale(scale, scale, scale);
        // Matrix matRotation = MatrixRotate({rotationAxis}, rotationAngle*DEG2RAD);
        Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);

        Matrix matTransform = MatrixMultiply(matScale, matTranslation);

        // Combine model transformation matrix (model.transform) with matrix generated by function parameters (matTransform)
        model.transform = MatrixMultiply(model.transform, matTransform);

        int culled = 0;
        for (int i = 0; i < model.meshCount; i++) {
            Vector3 center = Vector3Add(Vector3Scale(Vector3Add(bboxes[i].min, bboxes[i].max), 0.5f), position);
            Vector3 size = Vector3Subtract(bboxes[i].max, bboxes[i].min);
            size.x = size.x > -size.x ? size.x : -size.x;
            size.y = size.y > -size.y ? size.y : -size.y;
            size.z = size.z > -size.z ? size.z : -size.z;
            float distance = 1.8f / data.fog_Density;
            bool cull = Box_SDF(Vector3Subtract(viewer_Position, center), size) > distance;
            if(cull) { culled++; continue; }

            Color color = model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;

            Color colorTint = WHITE;
            colorTint.r = (unsigned char)(((int)color.r*(int)tint.r)/255);
            colorTint.g = (unsigned char)(((int)color.g*(int)tint.g)/255);
            colorTint.b = (unsigned char)(((int)color.b*(int)tint.b)/255);
            colorTint.a = (unsigned char)(((int)color.a*(int)tint.a)/255);

            model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = colorTint;
            DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]], model.transform);
            model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
        }

        return culled;
    }

    void DrawTextExOutline(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint, unsigned char alpha = 255) {
        float outline_Size = (GetScreenWidth() + GetScreenHeight()) / 400.f;
        for(int angle = 0; angle < 360; angle += 22) {
            Vector2 offset = {cos(angle * DEG2RAD) * outline_Size, sin(angle * DEG2RAD) * outline_Size};

            DrawTextExC(font, text, Vector2Add(position, offset), fontSize, spacing, Fade(BLACK, (float)alpha / 255.f));
        }
        DrawTextExC(font, text, position, fontSize, spacing, Fade(WHITE, (float)alpha / 255.f));
    }

    void Draw_Pack(Rectangle rectangle) {
        // float spacing = GetScreenHeight() / 240.f;
        // float border_Width = GetScreenHeight() / 120.f;
        float roundness = (GetScreenWidth() + GetScreenHeight()) / 2.f / 800.f / 10.f;

        DrawRectangleRounded(rectangle, roundness, 10, Color {10, 10, 10, 255});

        /*
        DrawRectangleRounded(rectangle, roundness, 10, WHITE);
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, roundness, 10, border_Width, GRAY);
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, roundness, 10, border_Width, BLACK);
        DrawRectangleRoundedLinesEx(rectangle, roundness, 10, border_Width / 1.8f, WHITE);
        */
    }

    bool Shared_Data::Button::Update(Vector2 position, unsigned char alpha, bool enabled) {
        Rectangle rectangle = {position.x - size_X / 2.f, position.y - size.y * 1.5f / 2.f, size_X, size.y * 1.5f};
        
        float spacing = GetScreenHeight() / 240.f;
        float border_Width = GetScreenHeight() / 120.f;

        DrawRectangleRounded(rectangle, 0.3f, 10, Fade(WHITE, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 3.f, rectangle.height - spacing * 3.f}, 0.3f, 10, border_Width, Fade(GRAY, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx({rectangle.x + spacing, rectangle.y + spacing, rectangle.width - spacing * 2.f, rectangle.height - spacing * 2.f}, 0.3f, 10, border_Width, Fade(enabled ? BLACK : Color {64, 64, 64, 255}, (float)alpha / 255.f));
        DrawRectangleRoundedLinesEx(rectangle, 0.3f, 10, border_Width / 1.8f, Fade(WHITE, (float)alpha / 255.f));

        float offset = 0.f;
        if(!data.custom_Font) offset = GetScreenHeight() / 600.f * 4.f;
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
        bool to_Return = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_Position, hitbox);
        // | pro live updataci off
        // v
        // bool to_Return = IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_Position, hitbox);

        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if(CheckCollisionPointRec(mouse_Position, hitbox)) {
                holding = true;
            }
        } else {
            holding = false;
        }

        if(holding) {
            progress = Clamp(Remap(mouse_Position.x, position.x - size * 5.f / 2.f, position.x + size * 5.f / 2.f, 0.f, 1.f), 0.f, 1.f);
        }

        return to_Return;
    }

    void Shared_Data::Tick_Box::Update(unsigned char alpha) {
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

        data.medium_Font = LoadFontEx(ASSETS_ROOT "fonts/medium.ttf", 64, nullptr, 0x5ff);
        SetTextureFilter(data.medium_Font.texture, TEXTURE_FILTER_BILINEAR);

        data.bold_Font = LoadFontEx(ASSETS_ROOT "fonts/bold.ttf", 64, nullptr, 0x5ff);
        SetTextureFilter(data.bold_Font.texture, TEXTURE_FILTER_BILINEAR);

        data.handwriting = LoadFontEx(ASSETS_ROOT "fonts/handwriting.ttf", 64, nullptr, 0xff);
        SetTextureFilter(data.handwriting.texture, TEXTURE_FILTER_BILINEAR);

        data.house = LoadModel(ASSETS_ROOT "models/house.glb");

        for(int material = 0; material < data.house.materialCount; material++) {
            SetTextureFilter(data.house.materials[material].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_BILINEAR);
        }

        data.tv_Video = Shared_Data::Mpeg_Video();
        data.tv_Video.Load(ASSETS_ROOT "vecernicek.mpg", false);

        data.tv_Sound = LoadMusicStream(ASSETS_ROOT "audio/vecernicek.mp3");
    }
};

#endif // SHARED_CPP