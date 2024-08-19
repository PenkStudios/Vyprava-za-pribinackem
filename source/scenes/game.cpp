#ifndef GAME_CXX
#define GAME_CXX

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "../reasings.c"

#include "../scene.cpp"
#include "../mod_loader.cpp"

#if defined(PLATFORM_ANDROID)
#include "../android.cpp"
#endif

#include "shared.cpp"
#include "../mission.cpp"

/*
float Triangular_Modulo(float x, float y) {
    float m = fmod(x, y);
    if((int)(x / y) % 2 == 0) {
        return y - m;
    } else {
        return m;
    }
}
*/

// Přemění blender souřadnice na raylib souřadnice
#define B2RL(x, y, z) {x, z, -y}

// Konstanty mapy. Mohlo by být v `config.txt`
#define CHAIR_POSITION B2RL(-8.2f, -7.5f, 20)
#define EATING_ROTATION 90.f
#define PRIBINACEK_WIN_ANIMATION {-4.60577, 18.2854, 7.5}
// #define SPOON_ROTATION_WIN_ANIMATION {0.f, 0.f, 0.f}
#define SPOON_WIN_ANIMATION {-4.38755, 18.2854, 8.88417}
#define PLAYERS_ROOM_TABLE {{-6.04131, 17.3548, 3.39275}, {-0.713218, 18.2854, 12.3022}}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DEBUG_TIMER
#define NOT_SPAWNED {0.f, 100.f, 0.f}

namespace Game {
    class Game_Data {
    public:
        class Door_Data {
        public:
            BoundingBox start_Bbox;

            Vector3 position;
            Vector3 scale;
            float start_Rotation_Range;
            float end_Rotation_Range;

            Material* material;
            int type;

            float rotation;

            bool opening;
            Door_Data(Vector3 door_Position, Vector3 door_Scale, float start_Rotation_Range, float end_Rotation_Range, int type, Material* material);
        };

        class Variable_Sound {
        public:
            std::vector<Sound> sounds = {};
            int old_Sound_Index = -1;

            Variable_Sound(const char* folder_Path) {
                // FilePathList file_List = LoadDirectoryFiles(folder_Path);
                // ^ nefunguje na androidu .___________.
                // + std::filesystem taky .____________.
                for (int index = 0; index < 100; index++) {
                    sounds.push_back(LoadSound(TextFormat("%s/%d.wav", folder_Path, index)));
                    if(!IsSoundReady(sounds.back())) {
                        sounds.pop_back();
                        break;
                    }
                }
            }

            Variable_Sound() {}

            // Pokud jakýkoliv zvuk hraje
            bool Playing() {
                for(Sound &sound : sounds) {
                    if(IsSoundPlaying(sound)) return true;
                }
                return false;
            }

            void Play(int index, float volume = 1.f) {
                Sound sound = sounds[index];
                SetSoundVolume(sound, volume);
                PlaySound(sound);
            }

            void Play(float volume = 1.f) {
                int sound_Index = rand() % sounds.size();
                while(sound_Index == old_Sound_Index)
                    sound_Index = rand() % sounds.size();

                Sound sound = sounds[sound_Index];
                SetSoundVolume(sound, volume);
                PlaySound(sound);

                old_Sound_Index = sound_Index;
            }
        };

        std::vector<Door_Data> doors = {};

        class Father_Point {
        public:
            Vector3 position;
            std::vector<bool> door_States;

            Father_Point(Vector3 position, int doors_Size) : position(position) {
                for(int index = 0; index < doors_Size; index++)
                    door_States.push_back(false);
            }

            Father_Point(Vector3 position, std::vector<bool> door_States) : position(position), door_States(door_States) {}
        };
        
        Model father;
        Camera3D camera;
        Model door;
        ModelAnimation *animations;
        Mesh door_Handle;
        float fall_Acceleration = 0.f;

        bool crouching = false;
        int frame_Counter = 0;
        float animation_Frame_Count = 0;

        std::vector<Father_Point> father_Points = {};
        float keyframe_Tick = 0.f;
        int keyframe = 0;

        class Directional_Position {
        public:
            Vector3 position;
            Vector3 target;

            Directional_Position(Vector3 position, Vector3 target) : position(position), target(target) {}
        };
        
        std::vector<Directional_Position> wake_Animation = {};
        float wake_Animation_Tick = 0.f;
        bool wake_Animation_Finished = false;

        class Death_Animation_Keyframe {
        public:
            Vector3 camera_Position;
            Vector3 camera_Target;

            Vector3 father_Position;
            float father_Rotation;

            Death_Animation_Keyframe(Vector3 camera_Position, Vector3 camera_Target, Vector3 father_Position, float father_Rotation) :
                camera_Position(camera_Position), camera_Target(camera_Target),
                father_Position(father_Position), father_Rotation(father_Rotation) {}
        };

        std::vector<Death_Animation_Keyframe> death_Animation = {};
        float death_Animation_Tick;
        bool death_Animation_Finished;

        Texture joystick_Base;
        Texture joystick_Pointer;

        Texture crouch;
        Texture un_Crouch;

        Vector3 camera_Rotation = {0.f, 180.f, 0.f};
        Vector2 old_Mouse_Position = {0.f, 0.f}; // Pozice kurzoru na předchozím framu
        Vector2 start_Mouse_Position = {0.f, 0.f}; // Pozice kurzoru na začátku draggování
        bool previous_Rotated = false; // Pokud byl rotate "event" předchozí frame

        class Joystick_Data {
        public:
            float rotation;
            bool moving;

            Joystick_Data(float rotation, bool moving) : rotation(rotation), moving(moving) {}
        };

        class Joystick {
        public:
            Vector2 center;
            float size;

            bool dragging = false;
            int draggin_Id = -1;

            Joystick(Vector2 center, float size) : center(center), size(size) {}
            Joystick() {}

            bool Can_Update(int touch_Id) {
                return !(CheckCollisionPointCircle(GetTouchPosition(touch_Id), center, size) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && !(dragging && draggin_Id == GetTouchPointId(touch_Id));
            }

            void Render();

            Joystick_Data Update(int touch_Id);
        };

        Joystick movement;

        #define MAX_ITEMS 8 // NONE, PRIBINACEK, SPOON, KEY, TV_REMOTE, CLOCK, POPCORN_PACKAGE, POPCORN_DONE, LOCK 
        // ...proč je jich 9?
        enum Item {NONE, PRIBINACEK, SPOON, KEY, TV_REMOTE, CLOCK, POPCORN_PACKAGE, POPCORN_DONE, LOCK, _LAST};
        class Item_Data {
        public:
            Vector3 position;
            Vector3 rotation;

            float fall_Acceleration;
            RayCollision collision;
            bool falling;

            int item_Id;
            bool pickable;

            void Calculate_Collision();

            Item_Data(int item_Id, Vector3 position, bool pickable, Vector3 rotation) : item_Id(item_Id), position(position), pickable(pickable), rotation(rotation), fall_Acceleration(0), falling(true) {}
            Item_Data(int item_Id, Vector3 position, bool pickable) : item_Id(item_Id), position(position), pickable(pickable), rotation({0.f, 0.f, 0.f}), fall_Acceleration(0), falling(true) {}
        };

        std::vector<Item_Data> item_Data;
        Item holding_Item = Item::NONE;

        Model spoon;
        Model key;
        Model lock;

        Vector3 camera_Target = {0.f, 0.f, 1.f}; // "cíl" kamery z bodu {0, 0, 0}

        class Guide_Caption {
        public:
            std::string target_Text;
            bool can_Skip; // zmáčkněte mezerník pro skipnutí (pro příběh)
            bool done = false;

            std::string text; // pomalu napsaný
            float frame = 1;

            std::string _Utf8_Substr(const std::string& str, unsigned int start, unsigned int leng) {
                if (leng==0) { return ""; }
                unsigned int c, i, ix, q, min=4294967295, max=4294967295;
                for (q=0, i=0, ix=str.length(); i < ix; i++, q++)
                {
                    if (q==start){ min=i; }
                    if (q<=start+leng || leng==std::string::npos){ max=i; }

                    c = (unsigned char) str[i];
                    if      (
                            //c>=0   &&
                            c<=127) i+=0;
                    else if ((c & 0xE0) == 0xC0) i+=1;
                    else if ((c & 0xF0) == 0xE0) i+=2;
                    else if ((c & 0xF8) == 0xF0) i+=3;
                    //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
                    //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
                    else return "";//invalid utf8
                }
                if (q<=start+leng || leng==std::string::npos){ max=i; }
                if (min==std::string::npos || max==std::string::npos) { return ""; }
                return str.substr(min,max);
            }

            // Vrátí true pokud přeskakuji tento frame
            bool Update();

            Guide_Caption(std::string text) : target_Text(text), can_Skip(true), text("") {}
            Guide_Caption(std::string text, bool can_Skip) : target_Text(text), can_Skip(can_Skip), text("") {}
        };

        int guide_Index = 0;
        std::vector<Guide_Caption> guide_Texts = {};

        bool guide_Finished = false;
        std::vector<Item_Data> key_Spawns;

        const BoundingBox players_Room_Table = PLAYERS_ROOM_TABLE;
        
        Model drawer;
        std::vector<BoundingBox> drawer_BBoxes;

        class Drawer_Data {
        public:
            Vector3 position;
            Vector3 original_Position;
            bool opening = false;
            bool has_Lock = false;

            std::vector<Item_Data*> childs = {};

            Drawer_Data(Vector3 position, bool has_Lock);
        };
        std::vector<Drawer_Data> drawers = {};

        class Win_Animation {
        public:
            bool playing = false;
            float tick = 0.f;

            int walk_Finish;
            int fetch_Finish; // "sbírání" Pribináčka + lžíci
            int open_Finish; // otevířání přibiňáčka
            int eat_Finish; // jezení ho
            int show_Finish; // ukázání win screeny

            int pribinacek_Fetch;
            int spoon_Fetch;

            Vector3 from_Rotation;
            Vector3 from_Position;

            Vector3 from_Pribinacek_Position;
            Vector3 from_Spoon_Rotation;
            Vector3 from_Spoon_Position;

            void Play();
            void Update();
        };

        Model clock;

        Win_Animation win;

        Vector3 father_Position = {0.f, 0.f, 0.f};
        float father_Rotation = 0.f;

        Vector3 father_Start_Position;
        float father_Start_Rotation;

        int target_Keyframe = -1;

        enum Father_State {NORMAL_AI, INSPECT, RETURN};
        Father_State father_State;

        float return_Tick = 0.f;
        int holding_Crouch = -1;

        bool action_Used = false;
        bool death_Animation_Playing;

        // death obrazovka
        Shared::Shared_Data::Button play_Again_Button;
        Shared::Shared_Data::Button menu_Button;

        // pause screen
        Shared::Shared_Data::Button pause_Continue_Button;
        Shared::Shared_Data::Button pause_Menu_Button;

        Texture skip;

        Vector3 camera_Start_Target;
        Vector3 camera_Start_Position;

        class Fuse_Box {
        public:
            Vector3 position;

            Model model;
            ModelAnimation *animations;

            float lever_Tick;
            bool lever_Turning;

            Fuse_Box() : lever_Tick(0.f), lever_Turning(false) {}
            void Reset() { lever_Tick = 0.f; lever_Turning = false; }
        } fuse_Box;

        Shader default_Shader;
        std::vector<Light> lights;

        bool debug = false;
        bool game_Paused = false;

        Texture sprint;
        Texture walk;

        int holding_Sprint = -1;
        bool sprinting = false;

        bool clock_Fell;
        float clock_Fall_Cooldown;

        class Sounds {
        public:
            float hear_Cooldown = 0.f;
            Variable_Sound hear{};

            float see_Cooldown = 0.f;
            Variable_Sound see{};

            Sound wake;
            Sound stand;

            Sound walk;
            Sound father_Walk;

            Sound clock_Fall;
            Variable_Sound lever{};

            Sound eat;
        } sounds;

        Texture pause;
        int holding_Pause = -1;
    
        Model tv_Remote;
        float time = 0.f;

        float father_Speed = 1.f;
        Material microwave_Door;

        Model popcorn_Package;
        Model popcorn_Done;

        Sound microwave_Sound;

        float microwave_Animation_Playing = false;
        float microwave_Animation_Tick = 0.f;
    
        Model safe_Door;
    } data;

    // https://www.reddit.com/r/raylib/comments/1b1nw51/bounding_boxes_for_rotated_models_are_completly/
    BoundingBox Get_Model_BBox_Matrix(Model model, Matrix transform) {
        Mesh mesh = model.meshes[0];
        BoundingBox bb = {Vector3Zero(), Vector3Zero()};

        float x = model.meshes[0].vertices[0];
        float y = model.meshes[0].vertices[1];
        float z = model.meshes[0].vertices[2];

        Vector3 v = {x, y, z};
        v = Vector3Transform(v, transform);

        bb.min = v;
        bb.max = v;

        for(int i = 0; i < model.meshes[0].vertexCount * 3.f; i += 3) {
            x = model.meshes[0].vertices[i];
            y = model.meshes[0].vertices[i+1];
            z = model.meshes[0].vertices[i+2];

            v = {x, y, z};
            v = Vector3Transform(v, transform);

            if(v.x < bb.min.x) {
                bb.min.x = v.x;
            }
            if(v.y < bb.min.y) {
                bb.min.y = v.y;
            }
            if(v.z < bb.min.z) {
                bb.min.z = v.z;
            }
            if(v.x > bb.max.x) {
                bb.max.x = v.x;
            }
            if(v.y > bb.max.y) {
                bb.max.y = v.y;
            }
            if(v.z > bb.max.z) {
                bb.max.z = v.z;
            }

        }

        return bb;
    }

    Matrix Get_Door_Matrix(Game_Data::Door_Data door_Data) {
        Matrix matrix = MatrixIdentity();
        matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));
        matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));
        matrix = MatrixMultiply(matrix, MatrixRotateY(door_Data.rotation * DEG2RAD));
        matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));
        matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));
        return matrix;
    }

    Game::Game_Data::Door_Data::Door_Data(Vector3 door_Position, Vector3 door_Scale, float start_Rotation_Range, float end_Rotation_Range, int type, Material* material) :
            position(door_Position), scale(door_Scale), start_Rotation_Range(start_Rotation_Range),
            end_Rotation_Range(end_Rotation_Range), rotation(start_Rotation_Range /* (start_Rotation_Range + end_Rotation_Range) / 2.f */),
            material(material), type(type), opening(false) {
        if(type != 0) {
            Matrix matrix = Get_Door_Matrix(*this);
            start_Bbox = Get_Model_BBox_Matrix(data.door, matrix);
        }
    }

    bool Game::Game_Data::Guide_Caption::Update() {
        if(!data.game_Paused)
            frame += 20.f * GetFrameTime();
        if(frame > target_Text.size()) {
            frame = target_Text.size();
        }

        text = _Utf8_Substr(target_Text, 0, (int)frame);

        float margin = GetScreenWidth() / 50.f;
        float height = GetScreenHeight() / 6.f;
        Rectangle rectangle = {margin, margin, GetScreenWidth() - margin * 2.f, height};

        float caption_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 30.f;

        DrawRectangleRounded(rectangle, 0.1f, 20, {20, 20, 20, 255});

        Vector2 text_Size = MeasureTextEx(Shared::data.medium_Font, text.c_str(), caption_Font_Size, 0.f);
        DrawTextEx(Shared::data.medium_Font, text.c_str(), {rectangle.x + rectangle.width / 2.f - text_Size.x / 2.f, rectangle.y + rectangle.height / 2.f - text_Size.y / 2.f}, caption_Font_Size, 0.f, WHITE);

        float skip_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 45.f;
        Vector2 skip_Text_Size = MeasureTextEx(Shared::data.bold_Font, u8"Zmáčkněte E pro přeskočení", skip_Font_Size, 0.f);

        float button_Size = skip_Font_Size / 17.7f;

        if(!Shared::data.mobile_Mode.ticked) DrawTextEx(Shared::data.bold_Font, u8"Zmáčkněte E pro přeskočení", {(float)GetScreenWidth() - skip_Text_Size.x / 2.f - (data.skip.width * button_Size) / 2.f - skip_Text_Size.x / 2.f, (float)GetScreenHeight() - (data.skip.height * button_Size) * 1.75f - skip_Text_Size.y / 2.f}, skip_Font_Size, 0.f, WHITE);
        DrawTextureEx(data.skip, {(float)GetScreenWidth() - skip_Text_Size.x / 2.f - (data.skip.width * button_Size), (float)GetScreenHeight() - (data.skip.width * button_Size) * 1.5f}, 0.f, button_Size, WHITE);

        bool skip;

        if(Shared::data.mobile_Mode.ticked) {
            skip = can_Skip && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        } else {
            skip = can_Skip && IsKeyPressed(KEY_SPACE);
        }

        return skip;
    }

    void Game_Data::Win_Animation::Play() {
        // Potom co se hráč dostal do místnosti s Pribináčkem a lžicí na stole
        tick = 0.f;

        walk_Finish = Vector3Distance(data.camera.position, CHAIR_POSITION) * 10;
        pribinacek_Fetch = Vector3DistanceSqr(data.item_Data[Game_Data::PRIBINACEK].position, PRIBINACEK_WIN_ANIMATION) * 5.f;
        spoon_Fetch = Vector3DistanceSqr(data.item_Data[Game_Data::SPOON].position, SPOON_WIN_ANIMATION) * 5.f;
        fetch_Finish = MAX(pribinacek_Fetch, spoon_Fetch) + walk_Finish;
        open_Finish = fetch_Finish + Shared::data.animations[0].frameCount - 1;
        eat_Finish = open_Finish + 300.f;
        show_Finish = eat_Finish + 50.f;

        from_Rotation = data.camera_Rotation;
        from_Position = data.camera.position;

        from_Pribinacek_Position = data.item_Data[Game_Data::PRIBINACEK].position;
        from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
        from_Spoon_Rotation = data.item_Data[Game_Data::SPOON].rotation;

        playing = true;
    }

    Vector3 Lerp_Rotation(Vector3 source, Vector3 target, float amount) {
        Vector3 addition = {(float)(((((int)target.x - (int)source.x) % 360) + 540) % 360) - 180,
                            (float)(((((int)target.y - (int)source.y) % 360) + 540) % 360) - 180,
                            (float)(((((int)target.z - (int)source.z) % 360) + 540) % 360) - 180};
        return Vector3Add(source, Vector3Multiply(addition, {amount, amount, amount}));
    }

    void Set_TV_State(bool state) {
        if(state) {
            Shared::data.tv_Video.Play();
            PlayMusicStream(Shared::data.tv_Sound);

            Shared::data.house.materials[30].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            Shared::data.house.materials[30].maps[MATERIAL_MAP_DIFFUSE].texture = Shared::data.tv_Video.texture;
        } else {
            StopMusicStream(Shared::data.tv_Sound);

            Shared::data.house.materials[30].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;
            Shared::data.house.materials[30].maps[MATERIAL_MAP_DIFFUSE].texture = LoadMaterialDefault().maps[MATERIAL_MAP_DIFFUSE].texture;
        }
    }

    void On_Switch() {
        DisableCursor();

        for(Game_Data::Drawer_Data &drawer : data.drawers) {
            drawer.position = drawer.original_Position;
            drawer.opening = false;
        }

        for(Game_Data::Door_Data &door : data.doors) {
            door.rotation = door.start_Rotation_Range;
            door.opening = false;
        }

        data.item_Data.clear();
        data.item_Data.push_back(Game_Data::Item_Data(0, {0.f, 0.f, 0.f}, false)); //                                       NONE
        data.item_Data.push_back(Game_Data::Item_Data(1, B2RL(26.5f, -41.f, 8.f), true)); //                                PRIBINACEK
        data.item_Data.push_back(Game_Data::Item_Data(2, B2RL(-26.5f, 10.f, 4.4f), true)); //                               SPOON
        data.item_Data.push_back(Game_Data::Item_Data(3, {0.f, 0.f, 0.f}, true)); //                                        KEY
        data.item_Data.push_back(Game_Data::Item_Data(4, {-28.336458f, 3.205174f, 4.820066f}, true, {0.f, -45.f, 0.f})); // TV_REMOTE
        data.item_Data.push_back(Game_Data::Item_Data(5, {-10.3036f, 16.3811f, 27.5464f}, false, {0.f, -90.f, 0.f})); //    CLOCK
        data.item_Data.push_back(Game_Data::Item_Data(6, {1.652715f, 4.066285f, 12.127025f}, true, {0.f, -135.9, 0.f})); // POPCORN_PACKAGE
        data.item_Data.push_back(Game_Data::Item_Data(7, NOT_SPAWNED, true, {0.f, 0.f, 0.f})); //                           POPCORN_DONE

        int key_Position_Index = rand() % data.key_Spawns.size();
        data.item_Data[3] = Game_Data::Item_Data(3, data.key_Spawns[key_Position_Index].position, true, data.key_Spawns[key_Position_Index].rotation);

        for(Game_Data::Item_Data &item : data.item_Data) {
            item.fall_Acceleration = 0.1f;
            item.Calculate_Collision();
            item.falling = true;
        }

        data.guide_Texts.clear();
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Probudíš se uprostřed noci a máš nepřekonatelnou chuť na Pribináčka, "));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("tvým cílem je jít pro Pribináčka a sníst si ho tady v pokoji."));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Jo a táta vždycky na noc vypíná pojistky, abych nemohl být na wifině."));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("...je trochu přehnaňe starostlivý."));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("1. Najdi Pribináček a\n    vezmi ho do pokoje\n    polož ho na stůl", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("2. Najdi klíč", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("3. Odemkni šuplík s\n     lžičkou a vezmi\n     ji do pokoje a polož\n     ji na stůl", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("4. Sněz si Pribináček\n     v pokoji", false));

        data.guide_Finished = false;
        data.guide_Index = 0;

        data.death_Animation_Tick = 0.f;
        data.death_Animation_Finished = false;
        data.death_Animation_Playing = false;

        data.wake_Animation_Tick = 0.f;
        data.wake_Animation_Finished = 0.f;

        data.father_State = Game_Data::NORMAL_AI;
        data.keyframe = 0;
        data.keyframe_Tick = 0.f;

        data.camera_Target = {0.f, 0.f, 1.f};
        data.fuse_Box.Reset();

        Shared::data.fog_Density = 0.1f;

        /*
        data.camera.position = {-10.f, 20.f, 23.5f}; // {0.f, 7.5f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.target = {0.f, 20.f, 23.5f};
        */

        data.camera_Rotation = {0.f, 180.f, 0.f};
        data.old_Mouse_Position = {0.f, 0.f}; // Pozice kurzoru na předchozím framu
        data.start_Mouse_Position = {0.f, 0.f}; // Pozice kurzoru na začátku draggování
        data.previous_Rotated = false; // Pokud byl rotate "event" předchozí frame

        data.sounds.hear_Cooldown = 0.f;
        data.crouching = false;

        Shared::data.fog_Density = 0.1f;
        
        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
            if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = Shared::data.lighting;
            }
        }

        for(Light &light : data.lights) {
            light.enabled = false;
            UpdateLightValues(Shared::data.lighting, light);
        }

        data.debug = false;
        data.game_Paused = false;

        data.sprinting = false;
        data.camera.fovy = Shared::data.fov.progress * 179.f;

        data.win.tick = 0.f;
        data.win.playing = false;

        data.clock_Fell = false;
        data.clock_Fall_Cooldown = (float)(rand() % 15 + 10) / 10.f;

        data.sounds.hear_Cooldown = rand() % 10 + 15;
        int fogDensityLoc = GetShaderLocation(Shared::data.lighting, "fogDensity");
        SetShaderValue(Shared::data.lighting, fogDensityLoc, &Shared::data.fog_Density, SHADER_UNIFORM_FLOAT);

        if(!Shared::data.show_Tutorial.ticked) {
            data.guide_Finished = true;

            int index = 0;
            for(Game::Game_Data::Guide_Caption &caption : data.guide_Texts) {
                if(caption.can_Skip == false) {
                    data.guide_Index = index;
                    data.guide_Finished = true;
                    break;
                }
                index++;
            }
        }

        Set_TV_State(false);
        data.time = 0.f;

        switch(Shared::data.game_Difficulty) {
            case 1: { // lehká
                data.father_Speed = 0.75f;
                break;
            }

            case 2: { // normální
                data.father_Speed = 1.f;
                break;
            }

            case 3: { // těžká
                data.father_Speed = 3.f;
                break;
            }

            case 4: { // velmi těžká
                data.father_Speed = 6.f;
                break;
            }
        }

        data.microwave_Animation_Tick = 0.f;
        data.microwave_Animation_Playing = false;

        Mod_Callback("Switch_Game", (void*)&data);
    }

    void Main_Menu(bool play_Again) {
        Shared::data.fog_Density = 0.1f;
        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
            if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = Shared::data.lighting;
            }
        }

        for(Light &light : data.lights) {
            light.enabled = false;
            UpdateLightValues(Shared::data.lighting, light);
        }

        int fogDensityLoc = GetShaderLocation(Shared::data.lighting, "fogDensity");
        SetShaderValue(Shared::data.lighting, fogDensityLoc, &Shared::data.fog_Density, SHADER_UNIFORM_FLOAT);

        Shared::data.play_Again = play_Again;
        Switch_To_Scene(MENU);
    }

    void Game_Data::Win_Animation::Update() {
        if(!playing) return;
        if(!data.game_Paused)
            tick += GetFrameTime() * 50.f;

        // std::cout << data.item_Data[Game_Data::SPOON].rotation.x << " " << data.item_Data[Game_Data::SPOON].rotation.y << " " << data.item_Data[Game_Data::SPOON].rotation.z << std::endl;
        // 0 -90 0 začátek
        // -75 0 0 začátek ponoření

        // nečitelný kód, ale funguje
        if(tick < walk_Finish) {
            int addition = (((((int)EATING_ROTATION - (int)from_Rotation.y) % 360) + 540) % 360) - 180;
            data.camera_Rotation = Vector3Lerp(from_Rotation, {0.f, from_Rotation.y + addition, 0.f}, (float)tick / (float)walk_Finish);
            data.camera.position = Vector3Lerp(from_Position, CHAIR_POSITION, (float)tick / (float)walk_Finish);
        } else if(tick < fetch_Finish) {
            float spoon_Fetch_Tick = Clamp((float)(tick - walk_Finish) / (float)spoon_Fetch, 0.f, 1.f);
            float pribinacek_Fetch_Tick = Clamp((float)(tick - walk_Finish) / (float)pribinacek_Fetch, 0.f, 1.f);
            data.item_Data[Game_Data::PRIBINACEK].position = Vector3Lerp(from_Pribinacek_Position, PRIBINACEK_WIN_ANIMATION, pribinacek_Fetch_Tick);
            data.item_Data[Game_Data::SPOON].rotation = Vector3Lerp(from_Spoon_Rotation, {0.f, -90.f, 0.f}, spoon_Fetch_Tick);
            data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, SPOON_WIN_ANIMATION, spoon_Fetch_Tick);
            // data.item_Data[Game_Data::SPOON].rotation = Vector3Lerp(from_Spoon_Rotation, SPOON_ROTATION_WIN_ANIMATION, Clamp((float)(tick - walk_Finish) / (float)spoon_Fetch, 0.f, 1.f));
            if(tick + 1 >= fetch_Finish) {
                from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                from_Spoon_Rotation = data.item_Data[Game_Data::SPOON].rotation;
            }

            float next_Tick = tick + GetFrameTime() * 50.f;
            if(next_Tick > fetch_Finish)
                PlaySound(data.sounds.eat);
        } else if(tick < open_Finish) {
            UpdateModelAnimation(Shared::data.pribinacek, Shared::data.animations[0], ((float)(tick - fetch_Finish) / (float)(open_Finish - fetch_Finish)) * Shared::data.animations[0].frameCount);
        } else if(tick < eat_Finish) {
            float stage_Tick = Clamp((float)(tick - open_Finish) / (float)(eat_Finish - open_Finish), 0.f, 1.f);
            float next_Stage_Tick = Clamp((float)(tick + (GetFrameTime() * 50.f) - open_Finish) / (float)(eat_Finish - open_Finish), 0.f, 1.f);
            if(stage_Tick < 0.2f) {
                // 0 - 0.2
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                //target.x -= 0.2f;
                target.y += 2.5f;
                target.z += 0.6f;
                
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, stage_Tick / 0.2f);
                data.item_Data[Game_Data::SPOON].rotation = Lerp_Rotation(from_Spoon_Rotation, {-75.f, 0.f, 0.f}, stage_Tick / 0.2f);
                if(next_Stage_Tick < 0.2f != true) {
                    data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, 1.f);
                    data.item_Data[Game_Data::SPOON].rotation = Lerp_Rotation(from_Spoon_Rotation, {-75.f, 0.f, 0.f}, 1.f);

                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                }
            } else if(stage_Tick < 0.4f) {
                // 0.2 - 0.4
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                target.y += 1.f;
                target.z += 0.4f; // 0.8
                
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.2f) / 0.2f);
                if(next_Stage_Tick < 0.4f != true) {
                    data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, 1.f);
                    
                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                }
            } else if(stage_Tick < 0.6f) {
                // 0.4 - 0.6
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                // target.x -= 0.2f;
                target.y += 2.f;
                target.z += 0.6f;
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.4f) / 0.2f);
                if(next_Stage_Tick < 0.6f != true) {
                    data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, 1.f);

                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                    from_Spoon_Rotation = data.item_Data[Game_Data::SPOON].rotation;
                }
            } else if(stage_Tick < 0.8f) {
                // 0.6 - 0.8
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                // target.x -= 0.2f;
                target.y += 2.f;
                target.z += 0.1f;

                data.item_Data[Game_Data::SPOON].rotation = Lerp_Rotation(from_Spoon_Rotation, {0.f, 90.f, 0.f}, (stage_Tick - 0.6f) / 0.2f);
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.6f) / 0.2f);
                if(next_Stage_Tick < 0.8f != true) {
                    data.item_Data[Game_Data::SPOON].rotation = Lerp_Rotation(from_Spoon_Rotation, {0.f, 90.f, 0.f}, 1.f);
                    data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, 1.f);

                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                }
            } else if(stage_Tick < 1.f) {
                // 0.8 - 1
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                target.x -= 2.f;
                target.y += 2.f;
                target.z += 0.1f;
                
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.8f) / 0.2f);
            }

            if(next_Stage_Tick < 1.f != true) {
                EnableCursor();
                HideCursor();

#if defined(PLATFORM_ANDROID)
                if(rand() % 3 == 0)
                    Show_Interstitial_Ad();
#endif
            }
        } else {
            Color tint = {255, 255, 255, (unsigned char)Clamp(Remap(tick, eat_Finish, show_Finish, 0.f, 255.f), 0.f, 255.f)};
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorTint(BLACK, tint));

            if(tick > show_Finish) {
                Color tint_UI = {255, 255, 255, (unsigned char)Clamp(Remap(tick, show_Finish, show_Finish + 50, 0.f, 255.f), 0.f, 255.f)};

                float font_Size = GetScreenHeight() / 15.f;
                Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Vyhrál jsi", {GetScreenWidth() / 2.f, GetScreenHeight() / 4.f}, font_Size, 1.f, WHITE, tint_UI.a);
                
                int quests_Offset = 0;
                for(int index = 0; index < data.guide_Texts.size(); index++) {
                    if(!data.guide_Texts[index].can_Skip) {
                        quests_Offset = index;
                        break;
                    }
                }

                int coins = 0;
                for(int index = 4; index < data.guide_Texts.size(); index++) {
                    coins += data.guide_Texts[index].done;
                }

                coins *= 2;

                Shared::DrawTextExC(Shared::data.bold_Font, TextFormat(u8"+%d peněz", coins), {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, ColorTint(WHITE, tint_UI));
            
                if(data.play_Again_Button.Update(tint_UI.a)) { Main_Menu(true); Shared::data.coins += coins; } // resetuje všechen progress
                else if(data.menu_Button.Update(tint_UI.a)) {
                    Shared::data.show_Tutorial.ticked = false;
                    Main_Menu(false);
                    Shared::data.coins += coins;
                }
            }
        }
    }

    Game_Data::Drawer_Data::Drawer_Data(Vector3 position, bool has_Lock) : has_Lock(has_Lock), position(position), original_Position(position), opening(false) {
        for(int index = 0; index < MAX_ITEMS; index++)
            childs.push_back(nullptr);
    }

    // Získat kolize s střelou (mapa + dveře + šuplíky)
    RayCollision Get_Collision_Ray(Ray ray, Game_Data::Item_Data *item = nullptr) {
        RayCollision collision = { 0 };
        collision.distance = 1000000.f;
        for (int m = 0; m < Shared::data.house.meshCount; m++)
        {
            // NOTE: We consider the model.transform for the collision check but 
            // it can be checked against any transform Matrix, used when checking against same
            // model drawn multiple times with multiple transforms
            RayCollision houseCollision = GetRayCollisionBox(ray, Shared::data.house_BBoxes[m]);

            /*
            Vector3 size = Vector3Subtract(Shared::data.house_BBoxes[m].max, Shared::data.house_BBoxes[m].min);
            Vector3 position = Vector3Divide(Vector3Add(Shared::data.house_BBoxes[m].max, Shared::data.house_BBoxes[m].min), {2.f, 2.f, 2.f});
            DrawCubeV(position, size, ColorFromHSV((m * 70) % 360, 1.f, 1.f));
            */

            if (houseCollision.hit)
            {
                if(houseCollision.distance < collision.distance) {
                    collision = houseCollision;
                }
            }
        }

        for(Game_Data::Door_Data &door_Data : data.doors) {
            // Fun fact: Pořádí v matrixové multiplikaci záleží
            // aha ono je to reálně napsaný v dokumentaci MatrixMultiply ._.

            Matrix matrix = Get_Door_Matrix(door_Data);

            RayCollision doorCollision = GetRayCollisionMesh(ray, data.door.meshes[0], matrix);
            if (doorCollision.hit)
            {
                if(doorCollision.distance < collision.distance) {
                    collision = doorCollision;
                }
            }
        }

        bool sticked = false;

        for(Game_Data::Drawer_Data &drawer_Data : data.drawers) {
            for(BoundingBox bbox : data.drawer_BBoxes) {
                BoundingBox bbox_New = bbox;
                bbox_New.min = Vector3Add(bbox_New.min, drawer_Data.position);
                bbox_New.max = Vector3Add(bbox_New.max, drawer_Data.position);
                RayCollision doorCollision = GetRayCollisionBox(ray, bbox_New);
                if (doorCollision.hit)
                {
                    if(doorCollision.distance < collision.distance) {
                        if(item != nullptr) {
                            drawer_Data.childs[item->item_Id] = item;
                            sticked = true;
                        }
                        collision = doorCollision;
                    }
                }
            }
        }

        if(item != nullptr && !sticked)
            for(int drawer_Index = 0; drawer_Index < data.drawers.size(); drawer_Index++)
                data.drawers[drawer_Index].childs[item->item_Id] = nullptr;

        return collision;
    }
    
    void Game_Data::Item_Data::Calculate_Collision() {
        Ray down = {position, {0.f, -0.1f, 0.f}};
        collision = Get_Collision_Ray(down, this);
    }

    Game_Data::Joystick_Data Game_Data::Joystick::Update(int touch_Id) {
        Vector2 mouse_Point = GetTouchPosition(touch_Id);

        Vector2 diff = Vector2Subtract(mouse_Point, center);
        float angle = atan2(diff.y, diff.x);

        if(!CheckCollisionPointCircle(GetTouchPosition(touch_Id), center, size)) {
            mouse_Point = Vector2Add({cos(angle) * size, sin(angle) * size}, center);
        }

        if((!dragging && CheckCollisionPointCircle(GetTouchPosition(touch_Id), center, size) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) || (dragging && draggin_Id == GetTouchPointId(touch_Id))) {
            DrawTextureEx(data.joystick_Pointer, {mouse_Point.x - size / 2.f, mouse_Point.y - size / 2.f}, 0.f, size / data.joystick_Pointer.width, WHITE);
            if(!dragging) {
                dragging = true;
                draggin_Id = GetTouchPointId(touch_Id);
            }
        }

        /*
        if(IsMouseButtonUp(MOUSE_BUTTON_LEFT) && dragging) {
            dragging = false;
        }
         */

        // LOG_VERBOSE("Joystick: dragging %d, id %d\n", dragging, dragging_Id);

        return Game_Data::Joystick_Data(angle * RAD2DEG, (CheckCollisionPointCircle(GetTouchPosition(touch_Id), center, size) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) || dragging);
    }

    void Game_Data::Joystick::Render() {
        DrawTextureEx(data.joystick_Base, {center.x - size, center.y - size}, 0.f, size * 2.f / data.joystick_Base.width, WHITE);
    }

    float Get_Distance_Volume(Vector3 target) {
        float distance = Vector3Distance(data.camera.position, target);
        // distance += fabs(target.y - data.camera.position.y);
        float volume = Remap(distance, 5.f, 40.f, 1.f, 0.f);
        return Clamp(volume, 0.0f, 1.2f);
    }

    void See_Player() {
        data.father_Start_Position = data.father_Position;
        data.father_Start_Rotation = data.father_Rotation;
        if(data.sounds.see_Cooldown <= 0.f) {
            data.sounds.see_Cooldown = rand() % 2 + 5;

            bool death_Screen = data.death_Animation_Tick - 2.f > (float)data.death_Animation.size() - 1.f;

            if(!data.sounds.hear.Playing() && !data.sounds.see.Playing() && !data.death_Animation_Playing & !data.win.playing) {
                data.sounds.see.Play(Get_Distance_Volume(data.father_Position));
            }
        } else {
            if(data.clock_Fell)
                data.sounds.see_Cooldown -= GetFrameTime();
        }
        data.father_State = Game_Data::INSPECT;
    }

    void Init_UI() {
        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        data.play_Again_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height}, u8"Hrát znovu", font_Size, Shared::data.medium_Font);
        data.menu_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, u8"Zpátky do menu", font_Size, Shared::data.medium_Font);

        data.pause_Continue_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, u8"Pokračovat ve hře", font_Size, Shared::data.medium_Font);
        data.pause_Menu_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + button_Height}, u8"Zpátky do menu", font_Size, Shared::data.medium_Font);
    }

    void Init() {
        data.camera.position = {-10.f, 20.f, 23.5f}; // {0.f, 7.5f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.target = {0.f, 20.f, 23.5f};
        data.camera.fovy = 90.f;
        data.camera.projection = CAMERA_PERSPECTIVE;

        if(Shared::data.mobile_Mode.ticked) {
            data.joystick_Base = LoadTexture(ASSETS_ROOT "textures/joystick_base.png");
            SetTextureFilter(data.joystick_Base, TEXTURE_FILTER_BILINEAR);

            data.joystick_Pointer = LoadTexture(ASSETS_ROOT "textures/joystick.png");
            SetTextureFilter(data.joystick_Pointer, TEXTURE_FILTER_BILINEAR);

            float margin = GetScreenHeight() / 30.f;
            data.movement = Game_Data::Joystick({GetScreenHeight() / 5.f + margin, GetScreenHeight() - GetScreenHeight() / 5.f - margin}, GetScreenHeight() / 5.f);

            data.crouch = LoadTexture(ASSETS_ROOT "textures/crouch.png");
            SetTextureFilter(data.crouch, TEXTURE_FILTER_BILINEAR);

            data.un_Crouch = LoadTexture(ASSETS_ROOT "textures/uncrouch.png");
            SetTextureFilter(data.un_Crouch, TEXTURE_FILTER_BILINEAR);

            data.sprint = LoadTexture(ASSETS_ROOT "textures/sprint.png");
            data.walk = LoadTexture(ASSETS_ROOT "textures/walk.png");
        }

        data.default_Shader = LoadMaterialDefault().shader;

        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
            Shared::data.house_BBoxes.push_back(GetMeshBoundingBox(Shared::data.house.meshes[mesh]));
        }

        data.father = LoadModel(ASSETS_ROOT "models/human.iqm");

        Texture human = LoadTexture(ASSETS_ROOT "textures/human.png");
        SetMaterialTexture(&data.father.materials[0], MATERIAL_MAP_DIFFUSE, human);

        data.door = LoadModel(ASSETS_ROOT "models/door.glb");
        for(int material = 0; material < data.door.materialCount; material++)
            data.door.materials[material].shader = Shared::data.lighting;

        int animation_Count = 0; // (1)
        data.animations = LoadModelAnimations(ASSETS_ROOT "models/human.iqm", &animation_Count);

        for(int material = 0; material < data.father.materialCount; material++)
            data.father.materials[material].shader = Shared::data.lighting;

        for(int material = 0; material < Shared::data.house.materialCount; material++)
            Shared::data.house.materials[material].shader = Shared::data.lighting;

        data.door_Handle = GenMeshSphere(0.5f, 15, 15);
        
        int tex_Coords_size = data.door_Handle.vertexCount * 2;
        for(int i = 0; i < tex_Coords_size; i += 2) {
            Vector2 vector = {data.door_Handle.texcoords[i], data.door_Handle.texcoords[i + 1]};
            vector = Vector2Multiply(vector, {0.5f, 0.5f});
            data.door_Handle.texcoords[i] = vector.x;
            data.door_Handle.texcoords[i + 1] = vector.y;
        }
        UpdateMeshBuffer(data.door_Handle, 1, data.door_Handle.texcoords, tex_Coords_size * sizeof(float), 0);

        std::istringstream ai_File(LoadFileText(ASSETS_ROOT "ai.txt"));

        std::string line;
        while (std::getline(ai_File, line)) {
            data.father_Points.push_back(Game_Data::Father_Point({0.f, 0.f, 0.f}, {}));
            std::stringstream string_Stream(line);

            float value;
            int index = 0;
            while (string_Stream >> value) {
                if(index == 0) {
                    data.father_Points.back().position.x = value;
                } else if(index == 1) {
                    data.father_Points.back().position.y = value;
                } else if(index == 2) {
                    data.father_Points.back().position.z = value;
                } else {
                    data.father_Points.back().door_States.push_back(value);
                }
                index++;
            }
        }

        std::istringstream doors_File(LoadFileText(ASSETS_ROOT "doors.txt"));
        float position_X, position_Y, position_Z,
              scale_X, scale_Y, scale_Z,
              rotation_Start, rotation_End;

        data.fuse_Box.model = LoadModel(ASSETS_ROOT "models/fusebox.glb");
        for(int material = 0; material < data.fuse_Box.model.materialCount; material++)
            data.fuse_Box.model.materials[material].shader = Shared::data.lighting;

        SetTextureFilter(data.fuse_Box.model.materials[1].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(data.fuse_Box.model.materials[2].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_BILINEAR);
        
        int fuse_Box_Animation_Count = 2 /* 2 animace */ - 1;
        data.fuse_Box.animations = LoadModelAnimations(ASSETS_ROOT "models/fusebox.glb", &fuse_Box_Animation_Count);

        data.safe_Door = LoadModel(ASSETS_ROOT "models/safe_door.glb");
        for(int material = 0; material < data.safe_Door.materialCount; material++)
            data.safe_Door.materials[material].shader = Shared::data.lighting;

        int door_Type;
        Material *material;

        while(doors_File >> position_X >> position_Y >> position_Z >>
                            scale_X >> scale_Y >> scale_Z >>
                            rotation_Start >> rotation_End >> door_Type) {
            switch(door_Type) {
                case 0: {
                    material = &Shared::data.house.materials[12];
                    break;
                }
                case 1: {
                    material = &Shared::data.house.materials[12];
                    break;
                }
                case 2: {
                    material = &Shared::data.house.materials[0];
                    break;
                }
                case 3: {
                    material = &data.fuse_Box.model.materials[1];
                    break;
                }
                case 4: {
                    material = &data.microwave_Door;
                    break;
                }
                case 5: {
                    material = &data.safe_Door.materials[1];
                    break;
                }
            }
            data.doors.push_back(Game_Data::Door_Data(Vector3 {position_X, position_Y, position_Z},
                                                      Vector3 {scale_X, scale_Y, scale_Z},
                                                      rotation_Start, rotation_End, door_Type, material));
        }

        std::istringstream animation_File(LoadFileText(ASSETS_ROOT "spawn_animation.txt"));
        float keyframe_X, keyframe_Y, keyframe_Z,
              target_X, target_Y, target_Z;


        while(animation_File >> keyframe_X >> keyframe_Y >> keyframe_Z >>
                                target_X >> target_Y >> target_Z) {
            data.wake_Animation.push_back(Game_Data::Directional_Position(Vector3 {keyframe_X, keyframe_Y, keyframe_Z},
                                                                   Vector3 {target_X, target_Y, target_Z}));
        }

        std::istringstream death_Animation_File(LoadFileText(ASSETS_ROOT "death_animation.txt"));
        float keyframe_Camera_X, keyframe_Camera_Y, keyframe_Camera_Z,
              target_Camera_X, target_Camera_Y, target_Camera_Z,
              keyframe_Father_X, keyframe_Father_Y, keyframe_Father_Z,
              father_Rotation;


        while(death_Animation_File >> keyframe_Camera_X >> keyframe_Camera_Y >> keyframe_Camera_Z >>
                                      target_Camera_X >> target_Camera_Y >> target_Camera_Z >>
                                      keyframe_Father_X >> keyframe_Father_Y >> keyframe_Father_Z >>
                                      father_Rotation) {
            data.death_Animation.push_back(Game_Data::Death_Animation_Keyframe(Vector3 {keyframe_Camera_X, keyframe_Camera_Y, keyframe_Camera_Z},
                                                                               Vector3 {target_Camera_X, target_Camera_Y, target_Camera_Z},
                                                                               Vector3 {keyframe_Father_X, keyframe_Father_Y, keyframe_Father_Z},
                                                                               father_Rotation));
        }

        data.spoon = LoadModel(ASSETS_ROOT "models/spoon.glb");
        for(int material = 0; material < data.spoon.materialCount; material++)
            data.spoon.materials[material].shader = Shared::data.lighting;

        data.key = LoadModel(ASSETS_ROOT "models/key.glb");
        for(int material = 0; material < data.key.materialCount; material++)
            data.key.materials[material].shader = Shared::data.lighting;

        data.drawer = LoadModel(ASSETS_ROOT "models/drawer.glb");
        for(int material = 0; material < data.drawer.materialCount; material++)
            data.drawer.materials[material].shader = Shared::data.lighting;
        
        data.lock = LoadModel(ASSETS_ROOT "models/lock.glb");
        for(int material = 0; material < data.lock.materialCount; material++)
            data.lock.materials[material].shader = Shared::data.lighting;

        for(int mesh = 0; mesh < data.drawer.meshCount; mesh++) {
            data.drawer_BBoxes.push_back(GetMeshBoundingBox(data.drawer.meshes[mesh]));
        }

        std::istringstream config_File(LoadFileText(ASSETS_ROOT "config.txt"));

        while (std::getline(config_File, line)) {
            std::stringstream string_Stream(line);
            
            std::vector<std::string> strings = {};
            std::string string;

            while(string_Stream >> string) {
                strings.push_back(string);
            }
            
            if(strings.size() > 2 && strings[1] == "=") {
                if(strings[0] == "KEY_SPAWNS" && (strings.size() - 2) % 4 == 0) {
                    for(int index = 2; index < strings.size(); index += 4) {
                        data.key_Spawns.push_back(Game_Data::Item_Data(-1, {std::stof(strings[index]), std::stof(strings[index + 1]), std::stof(strings[index + 2])}, true, {0.f, std::stof(strings[index + 3]), 0.f}));
                    }
                } else if(strings[0] == "DRAWERS" && (strings.size() - 2) % 4 == 0) {
                    for(int index = 2; index < strings.size(); index += 4) {
                        data.drawers.push_back(Game_Data::Drawer_Data({std::stof(strings[index]), std::stof(strings[index + 1]), std::stof(strings[index + 2])}, (bool)std::stoi(strings[index + 3])));
                    }
                } else if(strings[0] == "FUSE_BOX" && strings.size() - 2 == 3) {
                    data.fuse_Box.position = {std::stof(strings[2]), std::stof(strings[3]), std::stof(strings[4])};
                } else if(strings[0] == "CUSTOM_FONT" && strings.size() == 3) {
                    if(strings[2] == "TRUE") {
                        Shared::data.custom_Font = true;
                    } else if(strings[2] == "FALSE") {
                        Shared::data.custom_Font = false;
                    }

                    // TraceLog(LOG_INFO, "Custom font: %d", Shared::data.custom_Font);
                } else {
                    TraceLog(LOG_WARNING, "Unknown variable or bad value %s", strings[0].c_str());
                }
            }
        }

        data.item_Data.push_back(Game_Data::Item_Data(0, {0.f, 0.f, 0.f}, false)); //                                       NONE
        data.item_Data.push_back(Game_Data::Item_Data(1, B2RL(26.5f, -41.f, 8.f), true)); //                                PRIBINACEK
        data.item_Data.push_back(Game_Data::Item_Data(2, B2RL(-26.5f, 10.f, 4.4f), true)); //                               SPOON
        data.item_Data.push_back(Game_Data::Item_Data(3, {0.f, 0.f, 0.f}, true)); //                                        KEY
        data.item_Data.push_back(Game_Data::Item_Data(4, {-28.336458f, 3.205174f, 4.820066f}, true, {0.f, -45.f, 0.f})); // TV_REMOTE
        data.item_Data.push_back(Game_Data::Item_Data(5, {-10.3036f, 16.3811f, 27.5464f}, false, {0.f, -90.f, 0.f})); //    CLOCK
        data.item_Data.push_back(Game_Data::Item_Data(6, {1.652715f, 4.066285f, 12.127025f}, true, {0.f, -135.9, 0.f})); // POPCORN_PACKAGE
        data.item_Data.push_back(Game_Data::Item_Data(7, NOT_SPAWNED, true, {0.f, 0.f, 0.f})); //                           POPCORN_DONE

        Init_UI();

        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
            if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                BoundingBox bbox = GetMeshBoundingBox(Shared::data.house.meshes[mesh]);
                Vector3 center = Vector3Lerp(bbox.min, bbox.max, 0.5f);

                data.lights.push_back(CreateLight(LIGHT_POINT, center, Vector3Add(center, {0.f, -1.f, 0.f}), WHITE, Shared::data.lighting));
                
                data.lights.back().enabled = false;
                UpdateLightValues(Shared::data.lighting, data.lights.back());
            }
        }

        data.skip = LoadTexture(ASSETS_ROOT "textures/skip.png");
        data.sounds.hear = Game_Data::Variable_Sound(ASSETS_ROOT "audio/hear");
        data.sounds.see = Game_Data::Variable_Sound(ASSETS_ROOT "audio/see");

        int fogDensityLoc = GetShaderLocation(Shared::data.lighting, "fogDensity");
        SetShaderValue(Shared::data.lighting, fogDensityLoc, &Shared::data.fog_Density, SHADER_UNIFORM_FLOAT);

        data.clock = LoadModel(ASSETS_ROOT "models/clock.glb");
        for(int material = 0; material < data.clock.materialCount; material++)
            data.clock.materials[material].shader = Shared::data.lighting;

        data.sounds.wake = LoadSound(ASSETS_ROOT "audio/wake.wav");
        data.sounds.stand = LoadSound(ASSETS_ROOT "audio/stand.wav");

        data.sounds.walk = LoadSound(ASSETS_ROOT "audio/walk.wav");
        data.sounds.father_Walk = LoadSound(ASSETS_ROOT "audio/father_walk.wav");

        data.sounds.clock_Fall = LoadSound(ASSETS_ROOT "audio/clock_fall.wav");
        data.sounds.lever = Game_Data::Variable_Sound(ASSETS_ROOT "audio/lever");

        data.pause = LoadTexture(ASSETS_ROOT "textures/pause.png");
        data.sounds.eat = LoadSound(ASSETS_ROOT "audio/eat.wav");

        data.tv_Remote = LoadModel(ASSETS_ROOT "models/remote.glb");
        for(int material = 0; material < data.tv_Remote.materialCount; material++)
            data.tv_Remote.materials[material].shader = Shared::data.lighting;

        data.microwave_Door = LoadMaterialDefault();
        data.microwave_Door.shader = Shared::data.lighting;
        data.microwave_Door.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture(ASSETS_ROOT "textures/microwave_door.png");

        data.popcorn_Package = LoadModel(ASSETS_ROOT "models/popcorn_package.glb");
        for(int material = 0; material < data.popcorn_Package.materialCount; material++)
            data.popcorn_Package.materials[material].shader = Shared::data.lighting;

        data.popcorn_Done = LoadModel(ASSETS_ROOT "models/popcorn_done.glb");
        for(int material = 0; material < data.popcorn_Done.materialCount; material++)
            data.popcorn_Done.materials[material].shader = Shared::data.lighting;

        data.microwave_Sound = LoadSound(ASSETS_ROOT "audio/microwave.wav");

        Mod_Callback("Init_Game", (void*)&data);
    }

    bool Get_Collision_Sphere(Vector3 center, float radius) {
        for(int m = 0; m < Shared::data.house.meshCount; m++) {
            bool houseCollision = CheckCollisionBoxSphere(Shared::data.house_BBoxes[m], center, radius);
            if(data.debug) DrawBoundingBox(Shared::data.house_BBoxes[m], ColorFromHSV((m * 70) % 360, 1.f, 1.f));

            if(houseCollision) return true;
        }

        for(Game_Data::Door_Data &door_Data : data.doors) {
            if(door_Data.type != 0) {
                // Bohužel raylib nemá systém kolizi natočených bboxů + koulí, 
                // takže jednoduše (moc ne teda) uděláme opak (otočíme tu kouli
                // okolo pantů dveří o 
                // -(door_Data.rotation - door_Data.start_Rotation_Range) stupňů)
                BoundingBox bbox = door_Data.start_Bbox;

                Matrix matrix = MatrixIdentity();
                matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));
                matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));
                matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

                Vector3 door_Hinge = {0.f, 0.f, 0.f};
                door_Hinge = Vector3Transform(door_Hinge, matrix);

                Vector3 sphere_Center = center;
                sphere_Center = Vector3Subtract(sphere_Center, door_Hinge);
                Vector3 calculated_Sphere_Center = Vector3RotateByQuaternion(sphere_Center, QuaternionFromEuler(0.f, -(door_Data.rotation - door_Data.start_Rotation_Range) * DEG2RAD, 0.f));
                calculated_Sphere_Center = Vector3Add(calculated_Sphere_Center, door_Hinge);

                bool door_Collision = CheckCollisionBoxSphere(bbox, calculated_Sphere_Center, radius);
                if(door_Collision) return true;
            }
        }

        return false;
    }

    bool Ray_Sides_Collision(Vector3 camera_Position, Vector3 old_Position) {
        bool collision = Get_Collision_Sphere(camera_Position, 0.5f);

        /*
        bool collision_1 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.f, 0.f, 0.1f}}, old_Position);
        bool collision_2 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.f, 0.f, -0.1f}}, old_Position);
        bool collision_3 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.1f, 0.f, 0.f}}, old_Position);
        bool collision_4 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {-0.1f, 0.f, 0.f}}, old_Position);
        */

        return collision;
    }

    std::vector<bool> Get_Door_States() {
        std::vector<bool> states = {};
        for(Game_Data::Door_Data door_Data : data.doors) {
            states.push_back(door_Data.opening);
        }

        return states;
    }

    void Update_Camera_Android(int touch_Id, bool update_Camera) {
        data.camera_Target = Vector3RotateByQuaternion({0.f, 0.f, 10.f}, QuaternionFromEuler(data.camera_Rotation.x * DEG2RAD, data.camera_Rotation.y * DEG2RAD, data.camera_Rotation.z * DEG2RAD));

        Vector2 delta = Vector2Subtract(data.old_Mouse_Position, GetTouchPosition(touch_Id));
        data.old_Mouse_Position = GetTouchPosition(touch_Id);

        if(data.debug) {
            DrawLineEx(data.start_Mouse_Position, data.old_Mouse_Position, 5.f, BLUE);
            DrawCircleV(data.start_Mouse_Position, 25.f, RED);
            DrawCircleV(data.old_Mouse_Position, 25.f, RED);
        }

        if(roundf(data.start_Mouse_Position.x) == -1 && roundf(data.start_Mouse_Position.y) == -1)
            data.start_Mouse_Position = GetTouchPosition(touch_Id);

        if(!update_Camera)
            return;

        delta.x *= ((Shared::data.sensitivity.progress * 30.f) / 100.f) * 2.f;
        delta.y *= ((Shared::data.sensitivity.progress * 30.f) / 100.f) * 2.f;

        data.camera_Rotation = Vector3Add(data.camera_Rotation, {-delta.y, delta.x, 0.f});
        data.camera_Rotation.x = Clamp(data.camera_Rotation.x, -85.f, 85.f);
    }

    void Update_Camera_Desktop(float speed) {
        UpdateCameraPro(&data.camera,
            (Vector3){
                (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))*speed*GetFrameTime()*7.f -       // Move forward-backward
                (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))*speed*GetFrameTime()*7.f,    
                (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))*speed*GetFrameTime()*7.f -    // Move right-left
                (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))*speed*GetFrameTime()*7.f,
                0.0f                                                                     // Move up-down
            },
            (Vector3){
                0.f, 0.f, 0.f                                                            // Rotation: roll, yaw, pitch
            },
            0.f);

        Vector2 delta = GetMouseDelta();
        delta.x *= ((Shared::data.sensitivity.progress * 30.f) / 100.f) / 2.f;
        delta.y *= ((Shared::data.sensitivity.progress * 30.f) / 100.f) / 2.f;

        data.camera_Rotation = Vector3Add(data.camera_Rotation, {delta.y, -delta.x, 0.f});
        data.camera_Rotation.x = Clamp(data.camera_Rotation.x, -85.f, 85.f);
    }

    bool Player_Visible() {
        Vector3 father_Position = Vector3Add(data.father_Position, {0.f, 6.5f, 0.f});
        Ray father_Ray = {father_Position, Vector3Normalize(Vector3Subtract(data.camera.position, father_Position))};

        RayCollision father_Camera_Collision = GetRayCollisionSphere(father_Ray, data.camera.position, 0.2f);
        RayCollision scene_Collision = Get_Collision_Ray(father_Ray);

        float player_Distance = Vector3Distance(father_Camera_Collision.point, father_Position);
        float scene_Distance = Vector3Distance(scene_Collision.point, father_Position);

        bool player_Visible = player_Distance < scene_Distance;
        return player_Visible;
    }

    void Handle_Click() {
        bool door_Opened = false;
        for(Game_Data::Door_Data &door_Data : data.doors) {
            // DrawModelEx(door, door_Data.position, {0.f, 1.f, 0.f}, door_Data.rotation, door_Data.scale, WHITE);
            // DrawLine3D(data.camera.position, door_Data.position, RED);

            Matrix matrix = Get_Door_Matrix(door_Data);

            // DrawMesh(door_Data.type == 5 ? data.safe_Door.meshes[0]: data.door.meshes[0], *door_Data.material, matrix);
            
            Matrix matrixDoorHandle = MatrixIdentity();
            // matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(0.75f, 0.75f, 0.75f));
            float size = door_Data.scale.y / 5.f * 0.75f;
            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(size, size, size));

            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.scale.x + door_Data.scale.x / 2.f, door_Data.scale.y / 4.f, -door_Data.scale.z));
            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixRotateY(door_Data.rotation * DEG2RAD));
            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));
            // DrawMesh(data.door_Handle, *door_Data.material, matrixDoorHandle);

            Ray ray = GetMouseRay({(float)GetScreenWidth() / 2.f, (float)GetScreenHeight() / 2.f}, data.camera);
            RayCollision collision = GetRayCollisionMesh(ray, data.door.meshes[0], matrix);

            if(collision.hit && collision.distance < 10.f && !door_Opened && !data.action_Used) {
                door_Data.opening = !door_Data.opening;
                door_Opened = true;
                data.action_Used = true;
            }
        }

        Ray player_Ray = GetMouseRay({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, data.camera);
        RayCollision player_Map_Collision = Get_Collision_Ray(player_Ray);

        float nearest_Item_Hit = 10000.f;
        int nearest_Item_Id = -1;

        for(int index = 0; index < data.item_Data.size(); index++) {
            if((Game_Data::Item)index != Game_Data::NONE && (Game_Data::Item)index != Game_Data::_LAST) {
                BoundingBox bbox = {{-1.f, -1.f, -1.f}, {1.f, 1.f, 1.f}};
                bbox.min = Vector3Add(bbox.min, data.item_Data[index].position);
                bbox.max = Vector3Add(bbox.max, data.item_Data[index].position);
                
                RayCollision player_Item_Collision = GetRayCollisionBox(player_Ray, bbox);
                bool item_Visible = player_Item_Collision.hit && player_Item_Collision.distance < 5.f;

                if(item_Visible && data.holding_Item != (Game_Data::Item)index && data.item_Data[index].pickable) {
                    if(nearest_Item_Hit > player_Item_Collision.distance) {
                        nearest_Item_Hit = player_Item_Collision.distance;
                        nearest_Item_Id = index;
                    }
                }
                
                /*
                if(data.holding_Item == (Game_Data::Item)index && !data.action_Used && !data.win.playing) {
                    data.item_Data[index].fall_Acceleration = 0.1f;
                    data.item_Data[index].Calculate_Collision();
                    data.item_Data[index].falling = true;
                    data.holding_Item = Game_Data::NONE;
                    data.action_Used = true;
                } else if(item_Visible && !data.action_Used && data.holding_Item == Game_Data::NONE &&
                        player_Map_Collision.distance > player_Item_Collision.distance && !data.win.playing) {
                    data.holding_Item = (Game_Data::Item)index;
                    data.action_Used = true;

                    if((Game_Data::Item)index == Game_Data::PRIBINACEK && data.guide_Index == 4)
                        data.guide_Index++;

                    if((Game_Data::Item)index == Game_Data::SPOON && data.guide_Index == 5)
                        data.guide_Index++;
                }
                */
            }
        }

        float nearest_Hit = 10000.f;
        int nearest_Hit_Id = -1;

        int index = 0;
        for(Game_Data::Drawer_Data &drawer : data.drawers) {
            Ray ray = GetMouseRay({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, data.camera);
            for(int mesh = 0; mesh < data.drawer.meshCount; mesh++) {
                BoundingBox bbox = data.drawer_BBoxes[mesh];
                bbox.min = Vector3Add(bbox.min, drawer.position);
                bbox.max = Vector3Add(bbox.max, drawer.position);
                RayCollision collision = GetRayCollisionBox(ray, bbox);
                if(nearest_Hit > collision.distance && collision.hit) {
                    nearest_Hit = collision.distance;
                    nearest_Hit_Id = index;
                }
            }

            index++;
        }

        bool action_Item = (nearest_Item_Id != -1) && ((nearest_Hit > nearest_Item_Hit) && (player_Map_Collision.distance > nearest_Item_Hit));
        bool action_Drawer = (nearest_Hit_Id != -1) && ((nearest_Item_Hit > nearest_Hit) && FloatEquals(player_Map_Collision.distance, nearest_Hit));
        if(action_Drawer) {
            if(nearest_Hit < 5.f && nearest_Hit_Id != -1 && !data.action_Used) {
                if(data.drawers[nearest_Hit_Id].has_Lock) {
                    if(data.holding_Item == Game_Data::KEY) {
                        data.action_Used = true;
                        data.drawers[nearest_Hit_Id].opening = !data.drawers[nearest_Hit_Id].opening;
                    }
                } else {
                    data.action_Used = true;
                    data.drawers[nearest_Hit_Id].opening = !data.drawers[nearest_Hit_Id].opening;
                }
            }
        } else if(action_Item) {
            if(!data.action_Used && data.holding_Item == Game_Data::NONE && !data.win.playing) {
                data.holding_Item = (Game_Data::Item)nearest_Item_Id;
                data.action_Used = true;

                if((Game_Data::Item)nearest_Item_Id == Game_Data::KEY)
                    data.guide_Texts[5].done = true;
            }
        }

        if(data.holding_Item != Game_Data::NONE && !data.action_Used && !data.win.playing) {
            data.item_Data[data.holding_Item].fall_Acceleration = 0.1f;
            data.item_Data[data.holding_Item].Calculate_Collision();
            data.item_Data[data.holding_Item].falling = true;
            data.holding_Item = Game_Data::NONE;
            data.action_Used = true;
        }

        Mesh mesh = data.fuse_Box.model.meshes[1];
        
        BoundingBox bbox = GetMeshBoundingBox(mesh);
        bbox.min = Vector3Add(bbox.min, data.fuse_Box.position);
        bbox.max = Vector3Add(bbox.max, data.fuse_Box.position);

        RayCollision collision = GetRayCollisionBox(player_Ray, bbox);

        if((player_Map_Collision.distance > collision.distance) && collision.hit) {
            data.fuse_Box.lever_Turning = !data.fuse_Box.lever_Turning;
        }
    }

    void On_Game_Pause() {
        data.game_Paused = true;
        EnableCursor();
        HideCursor();

        PauseSound(data.sounds.wake);
        PauseSound(data.sounds.walk);
        PauseSound(data.sounds.father_Walk);

        PauseSound(data.microwave_Sound);

        Shared::data.tv_Video.Pause();
        PauseMusicStream(Shared::data.tv_Sound);
    }

    void On_Game_Resume() {
        data.game_Paused = false;
        DisableCursor();

        ResumeSound(data.sounds.wake);
        ResumeSound(data.sounds.walk);
        ResumeSound(data.sounds.father_Walk);

        ResumeSound(data.microwave_Sound);

        Shared::data.tv_Video.Resume();
        ResumeMusicStream(Shared::data.tv_Sound);
    }

    void Update() {
        data.time += GetFrameTime();

        #ifdef DEBUG_TIMER
        t_Init();
        #endif

        ClearBackground(BLACK);
        SetShaderValue(Shared::data.lighting, Shared::data.lighting.locs[SHADER_LOC_VECTOR_VIEW], &data.camera.position.x, SHADER_UNIFORM_VEC3);

        Shared::data.flashlight.position = data.camera.position;
        UpdateLightValues(Shared::data.lighting, Shared::data.flashlight);

        Vector3 old_Position = data.camera.position;
        int fogDensityLoc = GetShaderLocation(Shared::data.lighting, "fogDensity");
        SetShaderValue(Shared::data.lighting, fogDensityLoc, &Shared::data.fog_Density, SHADER_UNIFORM_FLOAT);

        #ifdef DEBUG_TIMER
        t_Breakpoint("Lightning věci");
        #endif

        data.action_Used = false; // If any action was used this frame (preventing click-through)
        if(!Shared::data.mobile_Mode.ticked)
            data.sprinting = IsKeyDown(KEY_LEFT_SHIFT);

        bool walking = (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) ||
                       IsKeyDown(KEY_S) || IsKeyDown(KEY_D)) && data.guide_Finished && !data.game_Paused && !data.win.playing && !data.death_Animation_Playing;

        float speed = 1.f;
        if(data.crouching) speed = 0.8f;
        else if(data.sprinting) speed = 1.5f;
        SetSoundPitch(data.sounds.walk, speed);

        if(IsSoundPlaying(data.sounds.walk) && !walking) PauseSound(data.sounds.walk);
        else if(!IsSoundPlaying(data.sounds.walk) && walking) ResumeSound(data.sounds.walk);

        if(walking && !IsSoundPlaying(data.sounds.walk)) PlaySound(data.sounds.walk);

        bool father_Walking = data.guide_Finished && !data.game_Paused && !data.win.playing && !data.death_Animation_Playing;
        SetSoundVolume(data.sounds.father_Walk, Get_Distance_Volume(data.father_Position) / 4.f);

        if(IsSoundPlaying(data.sounds.father_Walk) && !father_Walking) PauseSound(data.sounds.father_Walk);
        else if(!IsSoundPlaying(data.sounds.father_Walk) && father_Walking) ResumeSound(data.sounds.father_Walk);

        if(father_Walking && !IsSoundPlaying(data.sounds.father_Walk)) PlaySound(data.sounds.father_Walk);
        const BoundingBox microwave_Bbox = BoundingBox {{1.210129f, 16.935200f, 10.476102f}, {2.835903f, 18.817120f, 13.290030f}};

        BeginMode3D(data.camera); {
            Mod_Callback("Update_Game_3D", (void*)&data);

            data.frame_Counter++;
            if(data.frame_Counter > 100) data.frame_Counter = 0;

            if(!data.death_Animation_Playing) UpdateModelAnimation(data.father, data.animations[0], data.animation_Frame_Count);

            if(!data.game_Paused)
                data.animation_Frame_Count += 50.f * GetFrameTime();

            if(data.animation_Frame_Count >= data.animations[0].frameCount)
                data.animation_Frame_Count = 0;

            /* WAKE ANIMATION */ {
                if(data.debug) {
                    for(int index = 0; index < data.wake_Animation.size(); index++) {
                        DrawCube(data.wake_Animation[index].position, 1.f, 1.f, 1.f, RED);
                        DrawLine3D(data.wake_Animation[index].position, Vector3Add(data.wake_Animation[index].position, data.wake_Animation[index].target), WHITE);
                    }
                }

                if(data.wake_Animation_Tick < data.wake_Animation.size() - 1) {
                    Vector3 source_Position = data.wake_Animation[(int)data.wake_Animation_Tick].position;
                    Vector3 target_Position = data.wake_Animation[(int)data.wake_Animation_Tick + 1].position;

                    #define WAKE_EASING EaseSineInOut
                    #define WAKE_EASING_2 EaseLinearNone

                    Vector3 current_Position = {
                        (WAKE_EASING(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Position.x, target_Position.x - source_Position.x, 1.f) +
                            WAKE_EASING_2(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Position.x, target_Position.x - source_Position.x, 1.f)) / 2.f,
                        
                        (WAKE_EASING(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Position.y, target_Position.y - source_Position.y, 1.f) +
                            WAKE_EASING_2(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Position.y, target_Position.y - source_Position.y, 1.f)) / 2.f,
                        
                        (WAKE_EASING(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Position.z, target_Position.z - source_Position.z, 1.f) +
                            WAKE_EASING_2(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Position.z, target_Position.z - source_Position.z, 1.f)) / 2.f
                    };

                    Vector3 source_Target = data.wake_Animation[(int)data.wake_Animation_Tick].target;
                    Vector3 target_Target = data.wake_Animation[(int)data.wake_Animation_Tick + 1].target;

                    Vector3 current_Target = {
                        (WAKE_EASING(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Target.x, target_Target.x - source_Target.x, 1.f) + 
                            WAKE_EASING_2(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Target.x, target_Target.x - source_Target.x, 1.f)) / 2.f,
                        
                        (WAKE_EASING(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Target.y, target_Target.y - source_Target.y, 1.f) + 
                            WAKE_EASING_2(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Target.y, target_Target.y - source_Target.y, 1.f)) / 2.f,
                        
                        (WAKE_EASING(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Target.z, target_Target.z - source_Target.z, 1.f) + 
                            WAKE_EASING_2(data.wake_Animation_Tick - (int)data.wake_Animation_Tick, source_Target.z, target_Target.z - source_Target.z, 1.f)) / 2.f
                    };

                    bool can_Update = true;
                    if(data.wake_Animation_Tick && data.guide_Index < 1) can_Update = false;
                    if((int)data.wake_Animation_Tick == 3 && !data.guide_Finished) can_Update = false;

                    if(can_Update && !data.game_Paused) {
                        data.wake_Animation_Tick += 1.f * GetFrameTime();

                        float previous_Tick = data.wake_Animation_Tick - 1.f * GetFrameTime();
                        if(data.wake_Animation_Tick > 3.1f && previous_Tick < 3.1f) PlaySound(data.sounds.stand);
                    }

                    if(data.wake_Animation_Tick >= data.wake_Animation.size() - 1) {
                        data.wake_Animation_Finished = true;
                    }
                    
                    data.camera.position = current_Position;
                    data.camera_Target = current_Target;
                } else {
                    bool clock_Visible = data.camera_Target.z > 0.f;
                    if(!clock_Visible && !data.game_Paused) data.clock_Fall_Cooldown -= GetFrameTime();
                    if(data.clock_Fall_Cooldown < 0.f && !data.clock_Fell) {
                        data.clock_Fell = true;
                        SetSoundVolume(data.sounds.clock_Fall, Get_Distance_Volume(data.item_Data[Game_Data::Item::CLOCK].position));
                        PlaySound(data.sounds.clock_Fall);
                        data.item_Data[Game_Data::Item::CLOCK].position = {-10.392f, 13.2766f, 25.2803f};
                        data.item_Data[Game_Data::Item::CLOCK].rotation.x = 90.f;
                        data.item_Data[Game_Data::Item::CLOCK].Calculate_Collision();
                        data.sounds.hear.Play(2, 0.25f);
                    }
                }
            }

            #ifdef DEBUG_TIMER
            t_Breakpoint("Wake animace");
            #endif

            bool microwave_Mission_Can_Trigger = CheckCollisionBoxSphere(microwave_Bbox, data.item_Data[Game_Data::POPCORN_PACKAGE].position, 0.1f) &&
                                                                        data.holding_Item != Game_Data::POPCORN_PACKAGE &&
                                                                        data.fuse_Box.lever_Turning;
            
            if(microwave_Mission_Can_Trigger) {
                if(!data.microwave_Animation_Playing) {
                    PlaySound(data.microwave_Sound);
                    data.microwave_Animation_Tick = 0.f;
                    data.microwave_Animation_Playing = true;
                }
            }

            if(data.microwave_Animation_Playing) {
                data.microwave_Animation_Tick += GetFrameTime();

                Vector3 diff = Vector3Subtract(Vector3Lerp(microwave_Bbox.min, microwave_Bbox.max, 0.5f), data.camera.position);
                float yaw = (-atan2(diff.z, diff.x) * RAD2DEG) + 90.f;

                // https://stackoverflow.com/a/4036151/18373960
                // Velký milestone: už vím jak získat pitch rotaci
                float pitch = (atan2(-diff.y, sqrt(diff.z*diff.z + diff.x*diff.x)) * RAD2DEG);

                /*
                Vector3 destination = Vector3Lerp(microwave_Bbox.min, microwave_Bbox.max, 0.5f);

                Vector3 target_Angle;
                target_Angle.x = -atan2f(destination.x - data.camera.position.x, destination.y - data.camera.position.y) / PI * 180.0f + 180.0f;
                target_Angle.y = asinf((destination.z - data.camera.position.z) / Vector3Distance(data.camera.position, destination)) * RAD2DEG;
                target_Angle.z = 0.0f;
                */

                data.camera_Rotation = Lerp_Rotation(data.camera_Rotation, {pitch, yaw, 0.f}, 0.05f);

                if(data.microwave_Animation_Tick > 14.66f) {
                    for(Game_Data::Door_Data &door : data.doors) {
                        if(door.type == 4)
                            door.opening = true;
                    }
                } else if(data.microwave_Animation_Tick > 8.16f) {
                    if(microwave_Mission_Can_Trigger) {
                        data.item_Data[Game_Data::POPCORN_DONE].position = Vector3Lerp(microwave_Bbox.min, microwave_Bbox.max, 0.5f);
                        data.item_Data[Game_Data::POPCORN_DONE].position.y = microwave_Bbox.min.y;
                        data.item_Data[Game_Data::POPCORN_DONE].Calculate_Collision();
                        data.item_Data[Game_Data::POPCORN_PACKAGE].position = NOT_SPAWNED;
                    }
                } else if(data.microwave_Animation_Tick > 1.66f) {
                    for(Game_Data::Door_Data &door : data.doors) {
                        if(door.type == 4)
                            door.opening = false;
                    }
                }

                if(data.microwave_Animation_Tick > (data.microwave_Sound.frameCount/data.microwave_Sound.stream.sampleRate)) {
                    data.microwave_Animation_Playing = false;
                    Mission::Complete_Mission("Komedie", data.time);
                }
            }

            #ifdef DEBUG_TIMER
            t_Breakpoint("Cutscéna s mikrovlnkou");
            #endif

            /* DEATH ANIMATION */ if(data.death_Animation_Playing) {
                /*
                for(int index = 0; index < data.death_Animation.size(); index++) {
                    DrawCube(data.death_Animation[index].camera_Position, 1.f, 1.f, 1.f, RED);
                    DrawLine3D(data.death_Animation[index].camera_Position, Vector3Add(data.death_Animation[index].camera_Position, data.death_Animation[index].camera_Target), WHITE);

                    DrawCube(data.death_Animation[index].father_Position, 1.f, 1.f, 1.f, BLUE);
                }
                */

                bool can_Update = true;
                
                if(data.death_Animation_Tick < 2.f) {
                    const float range = 2.5f;
                    Vector3 target_Camera_Position = {data.father_Position.x + sinf((data.father_Rotation + 90.f) * DEG2RAD) * range, data.father_Position.y + 10.f, data.father_Position.z + cosf((data.father_Rotation + 90.f) * DEG2RAD) * range};
                    Vector3 target_Camera_Target = {sinf((data.father_Rotation + 90.f + 180.f) * DEG2RAD) * 5.f, 0.f, cosf((data.father_Rotation + 90.f + 180.f) * DEG2RAD) * 5.f};
                    if(data.death_Animation_Tick < 0.5f) {
                        #define DEATH_ANIMATION_EASING EaseElasticOut
                        
                        data.camera.position = {
                            DEATH_ANIMATION_EASING(data.death_Animation_Tick, data.camera_Start_Position.x, target_Camera_Position.x - data.camera_Start_Position.x, 0.5f),
                            DEATH_ANIMATION_EASING(data.death_Animation_Tick, data.camera_Start_Position.y, target_Camera_Position.y - data.camera_Start_Position.y, 0.5f),
                            DEATH_ANIMATION_EASING(data.death_Animation_Tick, data.camera_Start_Position.z, target_Camera_Position.z - data.camera_Start_Position.z, 0.5f)
                        };

                        data.camera_Target = {
                            DEATH_ANIMATION_EASING(data.death_Animation_Tick, data.camera_Start_Target.x, target_Camera_Target.x - data.camera_Start_Target.x, 0.5f),
                            DEATH_ANIMATION_EASING(data.death_Animation_Tick, data.camera_Start_Target.y, target_Camera_Target.y - data.camera_Start_Target.y, 0.5f),
                            DEATH_ANIMATION_EASING(data.death_Animation_Tick, data.camera_Start_Target.z, target_Camera_Target.z - data.camera_Start_Target.z, 0.5f)
                        };

                        // data.camera.position = Vector3Lerp(data.camera_Start_Position, target_Camera_Position, data.death_Animation_Tick * 4.f);
                        // data.camera_Target = Vector3Lerp(data.camera_Start_Target, target_Camera_Target, data.death_Animation_Tick * 4.f);
                    } else {
                        data.camera.position = target_Camera_Position;
                        data.camera_Target = target_Camera_Target;
                    }

                    float next_Tick = data.death_Animation_Tick + 1.f * GetFrameTime();
                    if(next_Tick > 2.f) PlaySound(data.sounds.wake);
                } else {
                    if(data.death_Animation_Tick - 2.f < data.death_Animation.size() - 1) {
                        Vector3 source_Camera_Position = data.death_Animation[(int)(data.death_Animation_Tick - 2.f)].camera_Position;
                        Vector3 target_Camera_Position = data.death_Animation[(int)(data.death_Animation_Tick - 2.f) + 1].camera_Position;

                        #define WAKE_EASING EaseSineInOut
                        #define WAKE_EASING_2 EaseLinearNone

                        Vector3 current_Camera_Position = {
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Position.x, target_Camera_Position.x - source_Camera_Position.x, 1.f) +
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Position.x, target_Camera_Position.x - source_Camera_Position.x, 1.f)) / 2.f,
                            
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Position.y, target_Camera_Position.y - source_Camera_Position.y, 1.f) +
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Position.y, target_Camera_Position.y - source_Camera_Position.y, 1.f)) / 2.f,
                            
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Position.z, target_Camera_Position.z - source_Camera_Position.z, 1.f) +
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Position.z, target_Camera_Position.z - source_Camera_Position.z, 1.f)) / 2.f
                        };

                        Vector3 source_Camera_Target = data.death_Animation[(int)(data.death_Animation_Tick - 2.f)].camera_Target;
                        Vector3 target_Camera_Target = data.death_Animation[(int)(data.death_Animation_Tick - 2.f) + 1].camera_Target;

                        Vector3 current_Camera_Target = {
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Target.x, target_Camera_Target.x - source_Camera_Target.x, 1.f) + 
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Target.x, target_Camera_Target.x - source_Camera_Target.x, 1.f)) / 2.f,
                            
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Target.y, target_Camera_Target.y - source_Camera_Target.y, 1.f) + 
                                WAKE_EASING_2(data.death_Animation_Tick - (int)(data.death_Animation_Tick - 2.f), source_Camera_Target.y, target_Camera_Target.y - source_Camera_Target.y, 1.f)) / 2.f,
                            
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Target.z, target_Camera_Target.z - source_Camera_Target.z, 1.f) + 
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Camera_Target.z, target_Camera_Target.z - source_Camera_Target.z, 1.f)) / 2.f
                        };

                        // ------------------

                        Vector3 source_Father_Position = data.death_Animation[(int)(data.death_Animation_Tick - 2.f)].father_Position;
                        Vector3 target_Father_Position = data.death_Animation[(int)(data.death_Animation_Tick - 2.f) + 1].father_Position;

                        Vector3 current_Father_Position = {
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Father_Position.x, target_Father_Position.x - source_Father_Position.x, 1.f) +
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Father_Position.x, target_Father_Position.x - source_Father_Position.x, 1.f)) / 2.f,
                            
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Father_Position.y, target_Father_Position.y - source_Father_Position.y, 1.f) +
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Father_Position.y, target_Father_Position.y - source_Father_Position.y, 1.f)) / 2.f,
                            
                            (WAKE_EASING(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Father_Position.z, target_Father_Position.z - source_Father_Position.z, 1.f) +
                                WAKE_EASING_2(data.death_Animation_Tick - 2.f - (int)(data.death_Animation_Tick - 2.f), source_Father_Position.z, target_Father_Position.z - source_Father_Position.z, 1.f)) / 2.f
                        };

                        float source_Father_Rotation = data.death_Animation[(int)(data.death_Animation_Tick - 2.f)].father_Rotation;
                        float target_Father_Rotation = data.death_Animation[(int)(data.death_Animation_Tick - 2.f) + 1].father_Rotation;
                        float current_Father_Rotation = Lerp_Rotation({0.f, source_Father_Rotation, 0.f}, {0.f, target_Father_Rotation, 0.f}, data.death_Animation_Tick - (int)data.death_Animation_Tick).y;

                        if(data.death_Animation_Tick - 2.f && data.guide_Index < 1) can_Update = false;
                        if((int)(data.death_Animation_Tick - 2.f) == 3 && !data.guide_Finished) can_Update = false;

                        if(data.death_Animation_Tick - 2.f >= data.death_Animation.size() - 1) {
                            data.death_Animation_Finished = true;
                            data.death_Animation_Tick -= 1.f * GetFrameTime();
                        }
                        
                        data.camera.position = current_Camera_Position;
                        data.camera_Target = current_Camera_Target;

                        data.father_Position = current_Father_Position;
                        data.father_Rotation = current_Father_Rotation;
                    }
                }

                if(can_Update && !data.game_Paused) {
                    data.death_Animation_Tick += 1.f * GetFrameTime();
                }
            }

            #ifdef DEBUG_TIMER
            t_Breakpoint("Death animace");
            #endif

            /* DOORS */ {
                bool door_Opened = false;
                for(Game_Data::Door_Data &door_Data : data.doors) {
                    // DrawModelEx(door, door_Data.position, {0.f, 1.f, 0.f}, door_Data.rotation, door_Data.scale, WHITE);
                    // DrawLine3D(data.camera.position, door_Data.position, RED);

                    Matrix matrix = Get_Door_Matrix(door_Data);

                    DrawMesh(door_Data.type == 5 ? data.safe_Door.meshes[0]: data.door.meshes[0], *door_Data.material, matrix);
                    
                    if(door_Data.type != 4 && door_Data.type != 5) {
                        Matrix matrix_Door_Handle = MatrixIdentity();

                        // matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(0.75f, 0.75f, 0.75f));
                        float size = Remap(door_Data.scale.y, 0.f, 5.f, 0.35f, 0.75f);
                        matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixScale(size, size, size));

                        matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixTranslate(door_Data.scale.x + door_Data.scale.x / 2.f, door_Data.scale.y / 4.f, -door_Data.scale.z));
                        matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixRotateY(door_Data.rotation * DEG2RAD));
                        matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

                        matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));
                        
                        DrawMesh(data.door_Handle, *door_Data.material, matrix_Door_Handle);
                    }

                    if(door_Data.type != 0) {
                        if(door_Data.start_Rotation_Range < door_Data.end_Rotation_Range) {
                            if(door_Data.opening) {
                                if(door_Data.rotation < door_Data.end_Rotation_Range) {
                                    if(!data.game_Paused)
                                        door_Data.rotation += 100.f * GetFrameTime();
                                }
                            } else {
                                if(door_Data.rotation > door_Data.start_Rotation_Range) {
                                    if(!data.game_Paused)
                                        door_Data.rotation -= 100.f * GetFrameTime();
                                }
                            }
                        } else {
                            if(door_Data.opening) {
                                if(door_Data.rotation > door_Data.end_Rotation_Range) {
                                    if(!data.game_Paused)
                                        door_Data.rotation -= 100.f * GetFrameTime();
                                }
                            } else {
                                if(door_Data.rotation < door_Data.start_Rotation_Range) {
                                    if(!data.game_Paused)
                                        door_Data.rotation += 100.f * GetFrameTime();
                                }
                            }
                        }
                    } else {
                        bool door_Updated = false;
                        if(Vector3Distance(door_Data.position, data.camera.position) < 5.f) {
                            Matrix matrix = Get_Door_Matrix(door_Data);

                            Vector3 target = Vector3Transform({0.f, 0.f, 0.f}, matrix);
                            Ray raycast = {data.camera.position, Vector3Divide(Vector3Subtract(target, data.camera.position), {10.f, 10.f, 10.f})};

                            RayCollision collision = GetRayCollisionMesh(raycast, data.door.meshes[0], matrix);

                            if(collision.hit) {
                                Vector3 diff = Vector3Subtract(collision.point, Vector3Add(collision.point, collision.normal));
                                float angle = fmod((atan2(diff.x, diff.z) * RAD2DEG) + 180.f, 360.f);
                                float normalized_Rotation = Wrap(door_Data.rotation, 0.f, 360.f);
                                Vector2 distances = {fabs(normalized_Rotation - angle), fabs(normalized_Rotation - fmod(angle + 180.f, 360.f))};
                                
                                if(distances.x < distances.y) {
                                    if(!data.game_Paused)
                                        door_Data.rotation += (data.sprinting ? 150.f : 100.f) * GetFrameTime();
                                } else {
                                    if(!data.game_Paused)
                                        door_Data.rotation -= (data.sprinting ? 150.f : 100.f) * GetFrameTime();
                                }

                                door_Updated = true;
                            }
                        }

                        Vector3 father_Position = Vector3Add(data.father_Position, {0.f, 6.5f, 0.f});
                        if(Vector3Distance(door_Data.position, father_Position) < 5.f) {
                            Matrix matrix = Get_Door_Matrix(door_Data);

                            Vector3 target = Vector3Transform({0.f, 0.f, 0.f}, matrix);
                            Ray raycast = {father_Position, Vector3Divide(Vector3Subtract(target, father_Position), {10.f, 10.f, 10.f})};

                            RayCollision collision = GetRayCollisionMesh(raycast, data.door.meshes[0], matrix);

                            if(collision.hit) {
                                Vector3 diff = Vector3Subtract(collision.point, Vector3Add(collision.point, collision.normal));
                                float angle = fmod((atan2(diff.x, diff.z) * RAD2DEG) + 180.f, 360.f);
                                float normalized_Rotation = Wrap(door_Data.rotation, 0.f, 360.f);
                                Vector2 distances = {fabs(normalized_Rotation - angle), fabs(normalized_Rotation - fmod(angle + 180.f, 360.f))};
                                
                                if(distances.x < distances.y) {
                                    if(!data.game_Paused)
                                        door_Data.rotation += 100.f * GetFrameTime();
                                } else {
                                    if(!data.game_Paused)
                                        door_Data.rotation -= 100.f * GetFrameTime();
                                }

                                door_Updated = true;
                            }
                        }

                        if(!door_Updated) {
                            door_Data.rotation = ((door_Data.rotation * 1.95f) +
                                                    ((door_Data.start_Rotation_Range + door_Data.end_Rotation_Range) / 2.f) * 0.05f) / 2.f;
                        }

                        if(door_Data.start_Rotation_Range < door_Data.end_Rotation_Range)
                            door_Data.rotation = Clamp(door_Data.rotation, door_Data.start_Rotation_Range, door_Data.end_Rotation_Range);
                        else
                            door_Data.rotation = Clamp(door_Data.rotation, door_Data.end_Rotation_Range, door_Data.start_Rotation_Range);
                    }
                }
            }

            #ifdef DEBUG_TIMER
            t_Breakpoint("Dveře");
            #endif

            Shared::Draw_Model_Optimized(data.camera.position, Shared::data.house_BBoxes, Shared::data.house, {0.f, 0.f, 0.f}, 1.f, WHITE);

            #ifdef DEBUG_TIMER
            t_Breakpoint("Renderace domu");
            #endif

            // ------------------

            /* ITEMS */ {
                for(int index = 0; index < data.item_Data.size(); index++) {
                    switch(index) {
                        case Game_Data::PRIBINACEK: {
                            Shared::data.pribinacek.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                                data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                                data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(Shared::data.pribinacek, data.item_Data[index].position, 1.f, WHITE);
                            break;
                        }

                        case Game_Data::SPOON: {
                            data.spoon.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                    data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                    data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.spoon, data.item_Data[index].position, 1.f, WHITE);
                            break;
                        }

                        case Game_Data::KEY: {
                            data.key.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.key, data.item_Data[index].position, 1.f, WHITE);
                            break;
                        }

                        case Game_Data::TV_REMOTE: {
                            data.tv_Remote.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.tv_Remote, data.item_Data[index].position, 1.f, WHITE);
                            break;
                        }

                        case Game_Data::CLOCK: {
                            data.clock.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                     data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                     data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.clock, data.item_Data[index].position, 1.f, WHITE);
                            break;
                        }

                        case Game_Data::POPCORN_PACKAGE: {
                            data.popcorn_Package.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                               data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                               data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.popcorn_Package, data.item_Data[index].position, 1.f, WHITE);
                            break;
                        }

                        case Game_Data::POPCORN_DONE: {
                            data.popcorn_Done.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                            data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                            data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.popcorn_Done, data.item_Data[index].position, 1.f, WHITE);
                            break;
                        }

                        default: break;
                    }

                    if((Game_Data::Item)index != Game_Data::NONE && (Game_Data::Item)index != Game_Data::_LAST) {
                        if(data.holding_Item == (Game_Data::Item)index) {
                            data.item_Data[index].position = Vector3Add(data.camera.position, Vector3Multiply(Vector3Normalize(Vector3Subtract(data.camera.target, data.camera.position)), {3.f, 3.f, 3.f}));
                            
                            data.item_Data[index].rotation.y = -atan2(data.camera_Target.z, data.camera_Target.x) * RAD2DEG;
                        } else {
                            if(data.item_Data[index].collision.point.y > data.item_Data[index].position.y) {
                                data.item_Data[index].position.y = data.item_Data[index].collision.point.y;
                                data.item_Data[index].falling = false;
                            }

                            if(data.item_Data[index].falling) {
                                // data.item_Data[index].position.y -= 1.f * GetFrameTime();
                                if(data.debug) DrawLine3D(data.item_Data[index].position, data.item_Data[index].collision.point, RED);

                                if(!data.game_Paused)
                                    data.item_Data[index].position.y -= data.item_Data[index].fall_Acceleration * 50.f * GetFrameTime();
                                
                                data.item_Data[index].fall_Acceleration += 1.f * GetFrameTime();
                            }
                        }

                        // std::cout << index << ": " << data.item_Data[index].position.x << " " << data.item_Data[index].position.y << " " << data.item_Data[index].position.z << std::endl;

                        BoundingBox bbox = {{-1.f, -1.f, -1.f}, {1.f, 1.f, 1.f}};
                        bbox.min = Vector3Add(bbox.min, data.item_Data[index].position);
                        bbox.max = Vector3Add(bbox.max, data.item_Data[index].position);
                        
                        if(data.debug) DrawBoundingBox(bbox, RED);
                    }
                }
            }

            #ifdef DEBUG_TIMER
            t_Breakpoint("Předměty");
            #endif

            float nearest_Hit = 10000.f;
            int nearest_Hit_Id = -1;

            int index = 0;
            for(Game_Data::Drawer_Data &drawer : data.drawers) {
                DrawModel(data.drawer, drawer.position, 1.f, WHITE);
                if(drawer.has_Lock) DrawModel(data.lock, Vector3Add(drawer.position, {1.95f, 1.f, 0.f}), 1.f, WHITE);
                if(drawer.opening) {
                    if(!data.game_Paused)
                        drawer.position.x += 10.f * GetFrameTime();
                        
                    for(Game_Data::Item_Data* child : drawer.childs)
                        if(child != nullptr)
                            if(!data.game_Paused)
                                child->position.x += 10.f * GetFrameTime();

                    if(drawer.position.x > drawer.original_Position.x + 2.f) {
                        drawer.position.x -= 10.f * GetFrameTime();
                        for(Game_Data::Item_Data* child : drawer.childs)
                            if(child != nullptr)
                                if(!data.game_Paused)
                                    child->position.x -= 10.f * GetFrameTime();
                    }
                } else {
                    drawer.position.x -= 10.f * GetFrameTime();
                    for(Game_Data::Item_Data* child : drawer.childs)
                        if(child != nullptr)
                            if(!data.game_Paused)
                                child->position.x -= 10.f * GetFrameTime();
            
                    if(drawer.position.x < drawer.original_Position.x) {
                        if(!data.game_Paused)
                            drawer.position.x += 10.f * GetFrameTime();
                        for(Game_Data::Item_Data* child : drawer.childs)
                            if(child != nullptr)
                                if(!data.game_Paused)
                                    child->position.x += 10.f * GetFrameTime();
                        
                    }
                }

                index++;
            }

            #ifdef DEBUG_TIMER
            t_Breakpoint("Šuplíky");
            #endif

            // ----------------------

            if(data.debug) Shared::data.fog_Density += GetMouseWheelMove() / 100.f;
            if(IsKeyPressed(KEY_LEFT_CONTROL)) data.crouching = !data.crouching;

            if(CheckCollisionBoxSphere(data.players_Room_Table, data.item_Data[Game_Data::PRIBINACEK].position, 0.1f))
                data.guide_Texts[4].done = true;

            if(CheckCollisionBoxSphere(data.players_Room_Table, data.item_Data[Game_Data::SPOON].position, 0.1f))
                data.guide_Texts[6].done = true;

            if(CheckCollisionBoxSphere(data.players_Room_Table, data.item_Data[Game_Data::PRIBINACEK].position, 0.1f) &&
               CheckCollisionBoxSphere(data.players_Room_Table, data.item_Data[Game_Data::SPOON].position, 0.1f) && !data.win.playing &&
               data.holding_Item != Game_Data::PRIBINACEK && data.holding_Item != Game_Data::SPOON) {
                data.guide_Texts[7].done = true;
                data.win.Play();
            }

            UpdateModelAnimation(data.fuse_Box.model, data.fuse_Box.animations[0], data.fuse_Box.lever_Tick);
            DrawModel(data.fuse_Box.model, data.fuse_Box.position, 1.f, WHITE);

            if(data.fuse_Box.lever_Turning) {
                if(!data.game_Paused) {
                    data.fuse_Box.lever_Tick += GetFrameTime() * 80.f;

                    float max_Lever_Tick = data.fuse_Box.animations[0].frameCount - 1.f;
                    if((data.fuse_Box.lever_Tick < max_Lever_Tick / 2.f) && ((data.fuse_Box.lever_Tick + GetFrameTime() * 80.f) > max_Lever_Tick / 2.f)) {
                        Shared::data.fog_Density = 0.025f;
                        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
                            if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                                Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = data.default_Shader;
                            }
                        }

                        for(Light &light : data.lights) {
                            light.enabled = true;
                            UpdateLightValues(Shared::data.lighting, light);
                        }

                        data.sounds.lever.Play();
                    }
                }
            } else {
                if(!data.game_Paused) {
                    data.fuse_Box.lever_Tick -= GetFrameTime() * 80.f;

                    float max_Lever_Tick = data.fuse_Box.animations[0].frameCount - 1.f;
                    if((data.fuse_Box.lever_Tick > max_Lever_Tick / 2.f) && ((data.fuse_Box.lever_Tick - GetFrameTime() * 80.f) < max_Lever_Tick / 2.f)) {
                        Shared::data.fog_Density = 0.1f;
                        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
                            if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                                Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = Shared::data.lighting;
                            }
                        }

                        for(Light &light : data.lights) {
                            light.enabled = false;
                            UpdateLightValues(Shared::data.lighting, light);
                        }

                        data.sounds.lever.Play();
                    }
                }
            }

            data.fuse_Box.lever_Tick = Clamp(data.fuse_Box.lever_Tick, 0.f, data.fuse_Box.animations[0].frameCount - 1.f);
            
            #ifdef DEBUG_TIMER
            t_Breakpoint("Vstup & věci");
            #endif

            /* FATHER */ {
                if(data.debug) {
                    if(IsKeyPressed(KEY_SPACE)) {
                        data.father_Points.push_back(Game_Data::Father_Point({data.camera.position.x, data.camera.position.y - 4.5f, data.camera.position.z}, Get_Door_States()));
                    } else if(IsKeyPressed(KEY_BACKSPACE)) {
                        data.father_Points.back() = Game_Data::Father_Point({data.camera.position.x, data.camera.position.y - 4.5f, data.camera.position.z}, Get_Door_States());
                    } else if(IsKeyPressed(KEY_ENTER)) {
                        for(Game_Data::Father_Point point : data.father_Points) {
                            std::string doors = "";
                            for(bool state : point.door_States) {
                                doors += " " + std::to_string(state);
                            }
                
                            std::cout << point.position.x << " " << point.position.y << " " << point.position.z << doors << std::endl;
                        }
                    }

                    Vector3 previous_Point = {0.f, 0.f, 0.f};
                    int index = 0;
                    for(Game_Data::Father_Point point : data.father_Points) {
                        if(index > 0) {
                            DrawLine3D(previous_Point, point.position, RED);
                        }
                        bool invalid = Get_Collision_Sphere(point.position, 1.5f);
                        DrawSphere(point.position, .5f, invalid ? RED : BLUE);
                        previous_Point = point.position;
                        index++;
                    }
                }

                if(!data.death_Animation_Playing) {
                    switch(data.father_State) {
                        case Game_Data::INSPECT: {
                            Vector3 old_Position = data.father_Position;

                            if(!data.game_Paused)
                                data.father_Position = Vector3MoveTowards(data.father_Position, data.camera.position, GetFrameTime() * 10.f * data.father_Speed);

                            bool collide_X = Get_Collision_Sphere({data.father_Position.x, data.father_Position.y + 6.5f, old_Position.z}, 1.5f);
                            if(collide_X) data.father_Position.x = old_Position.x;

                            bool collide_Z = Get_Collision_Sphere({old_Position.x, data.father_Position.y + 6.5f, data.father_Position.z}, 1.5f);
                            if(collide_Z) data.father_Position.z = old_Position.z;

                            Vector3 diff = Vector3Subtract(data.camera.position, data.father_Position);
                            float angle = -atan2(diff.z, diff.x) * RAD2DEG;

                            float addition = (((((int)angle - (int)data.father_Rotation) % 360) + 540) % 360) - 180;

                            if(!data.game_Paused)
                                data.father_Rotation += addition / 20.f;

                            if(collide_X || collide_Z) {
                                data.father_State = Game_Data::RETURN;
                                data.father_Start_Position = data.father_Position;
                                data.father_Start_Rotation = data.father_Rotation;
                                data.target_Keyframe = 0;
                                float nearest_Keyframe_Distance = 10000.f;
                                int index = 0;
                                for(Game_Data::Father_Point &keyframe : data.father_Points) {
                                    float distance = Vector3Distance(data.father_Position, keyframe.position);

                                    if(distance < nearest_Keyframe_Distance && fabs(data.father_Position.y + 3.5f - keyframe.position.y) < 3.f) {
                                        data.target_Keyframe = index;
                                        nearest_Keyframe_Distance = distance;
                                    }
                                    index++;
                                }
                                data.return_Tick = 0.f;
                            }
                            break;
                        }

                        case Game_Data::RETURN: {
                            Game_Data::Father_Point keyframe = data.father_Points[data.target_Keyframe];
                            data.father_Position = Vector3Lerp(data.father_Start_Position, keyframe.position, data.return_Tick);

                            Vector3 diff = Vector3Subtract(keyframe.position, data.father_Position);
                            float angle = -atan2(diff.z, diff.x) * RAD2DEG;

                            float addition = (((((int)angle - (int)data.father_Rotation) % 360) + 540) % 360) - 180;
                            if(!data.game_Paused)
                                data.father_Rotation += addition / 20.f;

                            if(!data.game_Paused)
                                data.return_Tick += 1.f * data.father_Speed * GetFrameTime();
                            
                            if(data.return_Tick > 1.f) {
                                data.father_State = Game_Data::NORMAL_AI;
                                data.keyframe = data.target_Keyframe;
                                data.keyframe_Tick = 0.f;
                                data.father_Rotation = angle;
                                data.father_Position = keyframe.position;
                            }

                            break;
                        }

                        case Game_Data::NORMAL_AI: {
                            Vector3 source = data.father_Points[data.keyframe].position;
                            Vector3 target = data.father_Points[(data.keyframe + 1) % data.father_Points.size()].position;

                            float max = Vector3Distance(source, target) / 4.f;

                            if(!data.game_Paused && data.clock_Fell)
                                data.keyframe_Tick += 1.f * data.father_Speed * GetFrameTime();

                            data.father_Position = {Remap(data.keyframe_Tick, 0.f, max, source.x, target.x),
                                                    Remap(data.keyframe_Tick, 0.f, max, source.y, target.y),
                                                    Remap(data.keyframe_Tick, 0.f, max, source.z, target.z)};

                            Vector3 difference = Vector3Subtract(target, source);
                            float angle = -atan2(difference.z, difference.x) * RAD2DEG;
                            data.father_Rotation = angle;

                            float angle_Source = angle;
                            if(data.keyframe > 0) {
                                Vector3 difference_Source = Vector3Subtract(source, data.father_Points[data.keyframe - 1].position);
                                angle_Source = -atan2(difference_Source.z, difference_Source.x) * RAD2DEG;

                                float lerp = Clamp(data.keyframe_Tick * 2.f, 0.f, 1.f);
                                float addition = (((((int)angle - (int)angle_Source) % 360) + 540) % 360) - 180;

                                data.father_Rotation = angle_Source + addition * lerp;
                            }

                            if(data.keyframe_Tick > max) {
                                data.keyframe_Tick = 0.f;
                                data.keyframe++;
                            }

                            if(data.keyframe > data.father_Points.size() - 1) {
                                data.keyframe = 0;
                            }
                            break;
                        }
                    }
                }

                #ifdef DEBUG_TIMER
                t_Breakpoint("Táta");
                #endif

                Ray ray = {{data.father_Position.x, data.father_Position.y + 3.5f, data.father_Position.z}, {0.f, -0.1f, 0.f}};
                float y = Get_Collision_Ray(ray).point.y;

                data.father_Position.y = y;

                DrawModelEx(data.father, data.father_Position, {0.f, 1.f, 0.f}, data.father_Rotation + 90.f, Vector3 {12.f, 12.f, 12.f}, WHITE);
            }

            if(Player_Visible()) {
                See_Player();
            } else {
                data.sounds.see_Cooldown = 0.f;
            }

            /* CLIP
            if(GetFPS() < 30) SetTargetFPS(60);
            if(IsKeyPressed(KEY_C)) SetTargetFPS(1);
            */

            // ITEM DUMP
            if(IsKeyPressed(KEY_I)) {
                for(Game_Data::Item_Data item : data.item_Data) {
                    TraceLog(LOG_INFO, "Item dump: item id %d, pozice: %f %f %f, y rotace %f",
                                    item.item_Id, item.position.x, item.position.y, item.position.z,
                                    item.rotation.y);
                }
            }

            // BBOX DUMP
            if(IsKeyDown(KEY_KP_ADD)) {
                static int bbox_Index = 0;

                if(IsKeyPressed(KEY_KP_6)) {
                    bbox_Index++;
                }

                if(IsKeyPressed(KEY_KP_4)) {
                    bbox_Index--;
                }

                BoundingBox *bbox = &Shared::data.house_BBoxes[bbox_Index];
                DrawBoundingBox(*bbox, RED);

                if(IsKeyPressed(KEY_B)) {
                    TraceLog(LOG_INFO, "Bounding box dump: BoundingBox {{%f, %f, %f}, {%f, %f, %f}}",
                                    bbox->min.x, bbox->min.y, bbox->min.z,
                                    bbox->max.x, bbox->max.y, bbox->max.z);
                }
            }
        } EndMode3D();

        if(Shared::data.tv_Video.playing) {
            Shared::data.tv_Video.Update(&Shared::data.tv_Sound);
            
            UpdateMusicStream(Shared::data.tv_Sound);
            SetMusicVolume(Shared::data.tv_Sound, Get_Distance_Volume({5.57f, 7.13f, 1.68f}));
        
            if(plm_has_ended(Shared::data.tv_Video.plm)) {
                Set_TV_State(false);
                Shared::data.tv_Video.Stop();
            }
        }

        Mod_Callback("Update_Game_2D", (void*)&data, false);

        float crosshair_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 80.f;
        float crosshair_Thinkness = crosshair_Size / 5.f;
        DrawLineEx({GetScreenWidth() / 2.f - crosshair_Size, GetScreenHeight() / 2.f}, {GetScreenWidth() / 2.f + crosshair_Size, GetScreenHeight() / 2.f}, crosshair_Thinkness, Fade(WHITE, 0.2f));
        DrawLineEx({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f - crosshair_Size}, {GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + crosshair_Size}, crosshair_Thinkness, Fade(WHITE, 0.2f));

        if(Shared::data.mobile_Mode.ticked) {
            data.camera.target = Vector3Add(data.camera.position, data.camera_Target);
            data.camera_Target = Vector3RotateByQuaternion({0.f, 0.f, 10.f}, QuaternionFromEuler(data.camera_Rotation.x * DEG2RAD, data.camera_Rotation.y * DEG2RAD, data.camera_Rotation.z * DEG2RAD));

            if(data.wake_Animation_Finished && !data.death_Animation_Playing) {
                data.movement.Render();
                bool rotation_Updated = false;

                bool id_Found = false;
                for(int id = 0; id < GetTouchPointCount(); id++) {
                    if(data.movement.draggin_Id == GetTouchPointId(id))
                        id_Found = true;
                }

                if(!id_Found)
                    data.movement.dragging = false;

                float size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 2400.f;
                float margin = GetScreenHeight() / 30.f;

                DrawTextureEx(data.pause, {(float)GetScreenWidth() - data.pause.width * size - margin, margin}, 0.f, size, WHITE);
                Rectangle pause_Rectangle = {(float)GetScreenWidth() - data.pause.width * size - margin, margin, data.pause.width * size, data.pause.height * size};

                pause_Rectangle.x -= pause_Rectangle.width / 2.f;
                pause_Rectangle.y -= pause_Rectangle.height / 2.f;
                pause_Rectangle.width *= 2.f;
                pause_Rectangle.height *= 2.f;

                if(data.holding_Pause >= GetTouchPointCount() || !CheckCollisionPointRec(GetTouchPosition(data.holding_Pause), pause_Rectangle)) {
                    data.holding_Pause = -1;
                }

                Texture texture = data.crouching ? data.un_Crouch : data.crouch;
                DrawTextureEx(texture, {(float)GetScreenWidth() - texture.width * size - margin, margin * 2.f + texture.height * size}, 0.f, size, WHITE);
                Rectangle crouch_Rectangle = {(float)GetScreenWidth() - texture.width * size - margin, margin * 2.f + texture.height * size, texture.width * size, texture.height * size};

                crouch_Rectangle.x -= crouch_Rectangle.width / 2.f;
                crouch_Rectangle.y -= crouch_Rectangle.height / 2.f;
                crouch_Rectangle.width *= 2.f;
                crouch_Rectangle.height *= 2.f;

                if(data.holding_Crouch >= GetTouchPointCount() || !CheckCollisionPointRec(GetTouchPosition(data.holding_Crouch), crouch_Rectangle)) {
                    data.holding_Crouch = -1;
                }

                Texture texture_Sprint = data.sprinting ? data.walk : data.sprint;
                DrawTextureEx(texture_Sprint, {(float)GetScreenWidth() - texture_Sprint.width * size - margin, margin * 3.f + texture.height * 2.f * size}, 0.f, size, WHITE);
                Rectangle sprint_Rectangle = {(float)GetScreenWidth() - texture.width * size - margin, margin * 3.f + texture.height * 2.f * size, texture.width * size, texture.height * size};

                sprint_Rectangle.x -= sprint_Rectangle.width / 2.f;
                sprint_Rectangle.y -= sprint_Rectangle.height / 2.f;
                sprint_Rectangle.width *= 2.f;
                sprint_Rectangle.height *= 2.f;

                if(data.holding_Sprint >= GetTouchPointCount() || !CheckCollisionPointRec(GetTouchPosition(data.holding_Sprint), sprint_Rectangle)) {
                    data.holding_Sprint = -1;
                }

                bool camera_Rotated = false;
                for(int index = 0; index < GetTouchPointCount(); index++) {
                    int id = GetTouchPointId(index);

                    bool can_Update = data.movement.Can_Update(index);
                    Game_Data::Joystick_Data joystick = data.movement.Update(index);

                    if(CheckCollisionPointRec(GetTouchPosition(index), pause_Rectangle)) {
                        if(data.holding_Pause != id) {
                            On_Game_Pause();
                            data.holding_Pause = id;
                        }
                    } else if(CheckCollisionPointRec(GetTouchPosition(index), crouch_Rectangle)) {
                        if(data.holding_Crouch != id) {
                            data.crouching = !data.crouching;
                            data.holding_Crouch = id;
                        }
                    } else if(CheckCollisionPointRec(GetTouchPosition(index), sprint_Rectangle)) {
                        if(data.holding_Sprint != id) {
                            data.sprinting = !data.sprinting;
                            data.holding_Sprint = id;
                        }
                    } else {
                        if (can_Update) {
                            rotation_Updated = true;
                            if (data.wake_Animation_Finished && !data.death_Animation_Playing && !data.win.playing && !data.game_Paused) {
                                Update_Camera_Android(id, data.previous_Rotated);
                                camera_Rotated = true;
                            }
                        } else {
                            if (joystick.moving && data.wake_Animation_Finished && !data.death_Animation_Playing && !data.win.playing) {
                                float speed = 1.f;
                                if (data.crouching) speed /= 1.5f;
                                if (!data.crouching && data.sprinting) speed *= 1.5f;
                                if(data.microwave_Animation_Playing) speed = 0.f;

                                float camera_Angle = atan2(data.camera_Target.z, data.camera_Target.x) +
                                                    90.f * DEG2RAD;

                                Vector3 offset = {
                                        cos(joystick.rotation * DEG2RAD + camera_Angle) *
                                        GetFrameTime() * 7.f * speed, 0.f,
                                        sin(joystick.rotation * DEG2RAD + camera_Angle) *
                                        GetFrameTime() * 7.f * speed};
                                data.camera.position = Vector3Add(data.camera.position, offset);
                            }
                        }
                    }
                }

                if(!camera_Rotated) {
                    if(roundf(data.start_Mouse_Position.x) != -1 && roundf(data.start_Mouse_Position.y) != -1) {
                        bool tap = Vector2Distance(data.start_Mouse_Position, data.old_Mouse_Position) < 5.f;
                        if(tap) Handle_Click();
                    }
                    data.start_Mouse_Position = {-1.f, -1.f};
                }

                data.previous_Rotated = rotation_Updated;
            }
        } else {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) Handle_Click();

            data.camera.target = Vector3Add(data.camera.position, data.camera_Target);
            data.camera_Target = Vector3RotateByQuaternion({0.f, 0.f, 10.f}, QuaternionFromEuler(data.camera_Rotation.x * DEG2RAD, data.camera_Rotation.y * DEG2RAD, data.camera_Rotation.z * DEG2RAD));

            if(data.wake_Animation_Finished && !data.death_Animation_Playing && !data.win.playing && !data.game_Paused) {
                float speed = 1.f;
                if(data.crouching) speed /= 1.5f;
                if(!data.crouching && data.sprinting) speed *= 1.5f;
                if(data.microwave_Animation_Playing) speed = 0.f;

                Update_Camera_Desktop(speed);
            }
        }

        if(data.holding_Item == Game_Data::TV_REMOTE && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !data.action_Used) {
            float distance = Vector3Distance(data.camera.position, {5.57f, 7.13f, 1.68f});
            if(distance < 25.f && data.fuse_Box.lever_Turning) {
                Mission::Complete_Mission("Večerníček", data.time);
                Set_TV_State(true);
            }
        }

        
        if(IsKeyPressed(KEY_L)) {
            data.item_Data[Game_Data::POPCORN_PACKAGE].position = Vector3Lerp(microwave_Bbox.min, microwave_Bbox.max, 0.5f);
            data.item_Data[Game_Data::POPCORN_PACKAGE].position.y = microwave_Bbox.min.y;
            data.item_Data[Game_Data::POPCORN_PACKAGE].Calculate_Collision();
            data.fuse_Box.lever_Turning = true;
        }
        

        #ifdef DEBUG_TIMER
        t_Breakpoint("Platform-depended updatace & vykreslování");
        #endif

        /* COLLISIONS */ {
            if(Ray_Sides_Collision({old_Position.x, data.camera.position.y, data.camera.position.z}, old_Position))
                data.camera.position.z = old_Position.z;

            // (pokud data.debug) nechceme aby se koule renderovali dvakrát
            bool is_Debug = data.debug;
            data.debug = true;
            if(Ray_Sides_Collision({data.camera.position.x, data.camera.position.y, old_Position.z}, old_Position))
                data.camera.position.x = old_Position.x;
            data.debug = is_Debug;

            RayCollision collision_Legs = {0};
            if(data.wake_Animation_Finished && !data.death_Animation_Playing) {
                Ray bottom = {Vector3Add(data.camera.position, {0.f, -1.75f, 0.f}), {0.f, -0.1f, 0.f}};
                collision_Legs = Get_Collision_Ray(bottom);
                if(collision_Legs.hit) {
                    Vector3 target = Vector3Add(collision_Legs.point, {0.f, data.crouching ? 5.0f : 6.5f, 0.f});
                    if(target.y > 20.f && data.win.playing)
                        data.crouching = true;
                    if(data.camera.position.y < target.y) {
                        data.camera.position = target;
                        data.fall_Acceleration = 0.1f;
                    } else if(data.camera.position.y < target.y + 0.1f && data.camera.position.y > target.y - 0.1f) {
                        // Nedělat nic tady
                        data.fall_Acceleration = 0.1f;
                    } else {
                        if(!data.game_Paused)
                            data.camera.position.y -= data.fall_Acceleration * 50.f * GetFrameTime();
                        
                        data.fall_Acceleration += 1.f * GetFrameTime();
                    }
                } else
                    data.camera.position = old_Position;
            }

            if(data.debug) {
                DrawText(TextFormat("Legs raycast position: {%f, %f, %f}, angled surface: %d, %f", collision_Legs.point.x, collision_Legs.point.y, collision_Legs.point.z,
                                                                                                    collision_Legs.normal.y < 0.99f || collision_Legs.normal.y > 1.01), 5, 5, 15, WHITE);
                
                Vector3 source = data.father_Points[data.keyframe].position;
                Vector3 target = data.father_Points[(data.keyframe + 1) % data.father_Points.size()].position;
                float max_Tick = Vector3Distance(source, target) / 4.f;

                DrawText(TextFormat("AI data: keyframe tick %f/%f, keyframe %d/%d", data.keyframe_Tick, max_Tick, data.keyframe, data.father_Points.size()), 5, 5 + 15, 15, WHITE);
                DrawText(TextFormat("Basic data: camera position {%f, %f, %f}", data.camera.position.x, data.camera.position.y, data.camera.position.z), 5, 5 + 15 * 2, 15, WHITE);
            }
        }

        #ifdef DEBUG_TIMER
        t_Breakpoint("Kolize");
        #endif

        if(!data.guide_Finished) {
            if(data.guide_Texts[data.guide_Index].Update()) {
                data.guide_Index++;
                if(data.guide_Index == 1) PlaySound(data.sounds.wake);
                if(data.guide_Index > 3) {
                    data.guide_Finished = true;
                }
            }
            if(Shared::data.mobile_Mode.ticked) {
                float skip_Font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 45.f;
                Vector2 skip_Text_Size = MeasureTextEx(Shared::data.bold_Font, u8"Zmáčkněte E pro přeskočení", skip_Font_Size, 0.f);
                float button_Size = skip_Font_Size / 17.7f;

                Rectangle rectangle = {(float)GetScreenWidth() - skip_Text_Size.x / 2.f - (data.skip.width * button_Size), (float)GetScreenHeight() - (data.skip.width * button_Size) * 1.5f, data.skip.width * button_Size, data.skip.height * button_Size};
                rectangle.x -= button_Size * 30.f;
                rectangle.y -= button_Size * 30.f;
                rectangle.width += button_Size * 60.f;
                rectangle.height += button_Size * 60.f;

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rectangle) && !data.guide_Finished) {
                    if(data.wake_Animation_Tick < 1.1f) PlaySound(data.sounds.wake);
                    int index = 0;
                    for(Game::Game_Data::Guide_Caption &caption : data.guide_Texts) {
                        if(caption.can_Skip == false) {
                            data.guide_Index = index;
                            data.guide_Finished = true;
                            break;
                        }
                        index++;
                    }
                }
            } else {
                if(IsKeyPressed(KEY_E) && !data.guide_Finished) {
                    if(data.wake_Animation_Tick < 1.1f) PlaySound(data.sounds.wake);
                    int index = 0;
                    for(Game::Game_Data::Guide_Caption &caption : data.guide_Texts) {
                        if(caption.can_Skip == false) {
                            data.guide_Index = index;
                            data.guide_Finished = true;
                            break;
                        }
                        index++;
                    }
                }
            }
        }

        if(data.guide_Finished && Shared::data.show_Tutorial.ticked) {
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 30.f;
            float font_Size_Smaller = (GetScreenWidth() + GetScreenHeight()) / 2.f / 35.f;

            float margin = GetScreenWidth() / 50.f;
            Vector2 rectangle_Size = MeasureTextEx(Shared::data.medium_Font, data.guide_Texts[4].target_Text.c_str(), font_Size, 0.f);
            for(int i = 4 + 1; i < data.guide_Texts.size(); i++) {
                rectangle_Size.y += MeasureTextEx(Shared::data.medium_Font, data.guide_Texts[i].target_Text.c_str(), font_Size, 0.f).y;
            }

            Rectangle rectangle = {margin, margin, rectangle_Size.x, rectangle_Size.y};
            rectangle.width += margin / 2.f;
            DrawRectangleRounded(rectangle, 0.1f, 20, Color {20, 20, 20, 255});
            rectangle.width -= margin / 2.f;

            Vector2 text_Box_Size = MeasureTextEx(Shared::data.medium_Font, data.guide_Texts[4].target_Text.c_str(), font_Size_Smaller, 0.f);
            for(int i = 4 + 1; i < data.guide_Texts.size(); i++) {
                text_Box_Size.y += MeasureTextEx(Shared::data.medium_Font, data.guide_Texts[i].target_Text.c_str(), font_Size_Smaller, 0.f).y;
            }

            int y = 0;
            for(int i = 0; i < 4; i++) {
                const char* text = data.guide_Texts[4 + i].target_Text.c_str();
                Vector2 line = MeasureTextEx(Shared::data.medium_Font, text, font_Size_Smaller, 0.f);
                DrawTextEx(Shared::data.medium_Font, text, {rectangle.x + rectangle.width / 2.f - text_Box_Size.x / 2.f, rectangle.y + rectangle.height / 2.f - text_Box_Size.y / 2.f + y}, font_Size_Smaller, 0.f, data.guide_Texts[4 + i].done ? GREEN : WHITE);
                y += line.y;
            }
        }

        #ifdef DEBUG_TIMER
        t_Breakpoint("Guide renderace");
        #endif

        if(IsKeyPressed(KEY_TAB) && Shared::data.test_Mode.ticked) data.debug = !data.debug;

        if(data.death_Animation_Playing) {
            if(data.death_Animation_Tick < 2.f) {
                if(data.death_Animation_Tick > 1.5f) {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, (data.death_Animation_Tick - 1.5f) * 2.f));
                }
            } else {
                if(data.death_Animation_Tick - 2.f < 1.f) {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 1.f - (data.death_Animation_Tick - 2.f)));
                } else if(data.death_Animation_Tick - 2.f > (float)data.death_Animation.size() - 2.f) {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, data.death_Animation_Tick - 2.f - ((float)data.death_Animation.size() - 2.f)));
                }

                if(data.death_Animation_Tick - 2.f > (float)data.death_Animation.size() - 1.f) {
                    float alpha = 1.f - Clamp((data.death_Animation_Tick - 2.f - ((float)data.death_Animation.size() - 1.f)) * 0.5f, 0.f, 1.f);
                    Color color = Fade(WHITE, alpha);

                    float font_Size = GetScreenHeight() / 15.f;
                    Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Prohrál jsi", {GetScreenWidth() / 2.f, GetScreenHeight() / 4.f}, font_Size, 1.f, WHITE);
                    
                    int quests_Offset = 0;
                    for(int index = 0; index < data.guide_Texts.size(); index++) {
                        if(!data.guide_Texts[index].can_Skip) {
                            quests_Offset = index;
                            break;
                        }
                    }

                    int coins = 0;
                    for(int index = 4; index < data.guide_Texts.size(); index++) {
                        coins += data.guide_Texts[index].done;
                    }
                    Shared::DrawTextExC(Shared::data.bold_Font, TextFormat(u8"+%d peněz", coins), {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, WHITE);
                
                    if(data.play_Again_Button.Update()) { Main_Menu(true); Shared::data.coins += coins; } // resetuje všechen progress
                    else if(data.menu_Button.Update()) {
                        Main_Menu(false);
                        Shared::data.coins += coins;
                    }

                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorTint(color, BLACK));
                } else if((data.death_Animation_Tick + 1.f * GetFrameTime()) - 2.f > (float)data.death_Animation.size() - 1.f) {
                    EnableCursor();
                    HideCursor();

#if defined(PLATFORM_ANDROID)
                    if(rand() % 3 == 0)
                        Show_Interstitial_Ad();
#endif
                }
            }
        }

        #ifdef DEBUG_TIMER
        t_Breakpoint("Death animace");
        #endif


        // ------------------------------------------------------------------
        if(Vector3Distance(Vector3Add(data.father_Position, {0.f, 6.5f, 0.f}), data.camera.position) < 2.5f) {
            data.death_Animation_Playing = true;
            data.holding_Item = Game_Data::Item::NONE;
            data.camera_Start_Position = data.camera.position;
            data.camera_Start_Target = data.camera_Target;
            Shared::data.fog_Density = 0.1f;
            for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
                if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                    Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = Shared::data.lighting;
                }
            }

            for(Light &light : data.lights) {
                light.enabled = false;
                UpdateLightValues(Shared::data.lighting, light);
            }
            UpdateModelAnimation(data.father, data.animations[0], 16);
        }

        if(data.sounds.hear_Cooldown <= 0.f) {
            data.sounds.hear_Cooldown = rand() % 10 + 15;

            bool death_Screen = data.death_Animation_Tick - 2.f > (float)data.death_Animation.size() - 1.f;

            float distance = Vector3Distance(data.camera.position, data.father_Position);
            if(distance > 100.f && !data.sounds.see.Playing() && !data.sounds.hear.Playing() && !data.death_Animation_Playing & !data.win.playing) {
                data.sounds.hear.Play(Get_Distance_Volume(data.father_Position));
            }
        } else {
            if(data.clock_Fell)
                data.sounds.hear_Cooldown -= GetFrameTime();
        }

        data.win.Update();

        /*
        if(IsKeyPressed(KEY_P)) {
            data.item_Data[Game_Data::Item::SPOON].position = Vector3Add(data.players_Room_Table.min, {0.f, 1.f, 0.f});
            data.item_Data[Game_Data::Item::PRIBINACEK].position = Vector3Add(data.players_Room_Table.min, {0.f, 1.f, 0.f});
        }
        */

        if(IsKeyPressed(KEY_ESCAPE)) {
            data.game_Paused = !data.game_Paused;
            if(data.game_Paused) {
                On_Game_Pause();
            } else {
                On_Game_Resume();
            }
        }

        if(data.game_Paused) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color {0, 0, 0, 200});

            Vector2 size = {GetScreenWidth() / 2.2f, GetScreenHeight() / 2.f};
            Rectangle rectangle = {GetScreenWidth() / 2.f - size.x / 2.f, GetScreenHeight() / 2.f - size.y / 2.f, size.x, size.y};
            Shared::Draw_Pack(rectangle);

            float font_Size = GetScreenHeight() / 15.f;
            Shared::DrawTextExOutline(Shared::data.bold_Font, u8"Hra pozastavena", {GetScreenWidth() / 2.f, GetScreenHeight() / 3.f}, font_Size, 1.f, WHITE);

            if(data.pause_Menu_Button.Update()) {
                Main_Menu(false);
            }
            if(data.pause_Continue_Button.Update()) { On_Game_Resume(); }
        }

        Mod_Callback("Update_Game_2D", (void*)&data, true);

        if(Shared::data.show_Fps.ticked) {
            const char* text = TextFormat("FPS: %d", GetFPS());
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;

            Vector2 size = MeasureTextEx(Shared::data.medium_Font, text, font_Size, 0.f);
            Shared::DrawTextExOutline(Shared::data.medium_Font, text, {GetScreenWidth() - size.x / 2.f - font_Size, GetScreenHeight() - size.y / 2.f - font_Size}, font_Size, 0.f, WHITE);
        }

        Mission::Update_Mission_Overlay();

        #ifdef DEBUG_TIMER
        t_Breakpoint("Konečné věci");

        if(IsKeyPressed(KEY_KP_0)) t_Summary();
        #endif
    }
};

#endif // GAME_CXX
