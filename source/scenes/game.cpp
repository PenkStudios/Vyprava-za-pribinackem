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

#include "../ad.cpp"

#include "shared.cpp"

// počet vertexů na hlavní části světla
#define LIGHT_BASE_VERTICES 287

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

namespace Game {
    class Game_Data {
    public:
        class Door_Data {
        public:
            Vector3 position;
            Vector3 scale;
            float start_Rotation_Range;
            float end_Rotation_Range;

            Material* material;
            int type;

            float rotation;

            bool opening;
            Door_Data(Vector3 door_Position, Vector3 door_Scale, float start_Rotation_Range, float end_Rotation_Range, int type, Material* material) :
                position(door_Position), scale(door_Scale), start_Rotation_Range(start_Rotation_Range),
                end_Rotation_Range(end_Rotation_Range), rotation(start_Rotation_Range /* (start_Rotation_Range + end_Rotation_Range) / 2.f */),
                material(material), type(type), opening(false) {}
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

        std::vector<BoundingBox> house_BBoxes;
        
        Model father;
        Camera3D camera;
        Model door;
        ModelAnimation *animations;
        Mesh door_Handle;
        float fall_Acceleration = 0.f;

        float fog_Density = 0.15f;
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
                return !(CheckCollisionPointCircle(GetTouchPosition(touch_Id), center, size) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && !(dragging && draggin_Id == touch_Id);
            }

            void Render();

            Joystick_Data Update(int touch_Id);
        };

        Joystick movement;

        #define MAX_ITEMS 5 // NONE, PRIBINACEK, SPOON, KEY, LOCK
        enum Item {NONE, PRIBINACEK, SPOON, KEY, LOCK, _LAST};
        class Item_Data {
        public:
            Vector3 position;
            Vector3 rotation;

            float fall_Acceleration;
            RayCollision collision;
            bool falling;

            int item_Id;

            void Calculate_Collision();

            Item_Data(int item_Id, Vector3 position, Vector3 rotation) : item_Id(item_Id), position(position), rotation(rotation), fall_Acceleration(0), falling(true) {}
            Item_Data(int item_Id, Vector3 position) : item_Id(item_Id), position(position), rotation({0.f, 0.f, 0.f}), fall_Acceleration(0), falling(true) {}
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
            int fetch_Finish; // "sbírání" pribináčka + lžíci
            int open_Finish; // otevířání přibiňáčka
            int eat_Finish; // jezení ho

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

        float hear_Cooldown = 0.f;
        Variable_Sound hear{};

        bool debug = false;
    } data;

    bool Game::Game_Data::Guide_Caption::Update() {
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

        DrawTextEx(Shared::data.bold_Font, u8"Zmáčkněte E pro přeskočení", {(float)GetScreenWidth() - skip_Text_Size.x / 2.f - (data.skip.width * button_Size) / 2.f - skip_Text_Size.x / 2.f, (float)GetScreenHeight() - (data.skip.height * button_Size) * 1.75f - skip_Text_Size.y / 2.f}, skip_Font_Size, 0.f, WHITE);
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
        // Potom co se hráč dostal do místnosti s pribináčkem a lžicí na stole
        tick = 0.f;

        walk_Finish = Vector3Distance(data.camera.position, CHAIR_POSITION) * 10;
        pribinacek_Fetch = Vector3DistanceSqr(data.item_Data[Game_Data::PRIBINACEK].position, PRIBINACEK_WIN_ANIMATION) * 5.f;
        spoon_Fetch = Vector3DistanceSqr(data.item_Data[Game_Data::SPOON].position, SPOON_WIN_ANIMATION) * 5.f;
        fetch_Finish = MAX(pribinacek_Fetch, spoon_Fetch) + walk_Finish;
        open_Finish = fetch_Finish + Shared::data.animations[0].frameCount - 1;
        eat_Finish = open_Finish + GetFPS() * (0.4166f / 2.f);

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

    void Game_Data::Win_Animation::Update() {
        if(!playing) return;
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
        } else if(tick < open_Finish) {
            UpdateModelAnimation(Shared::data.pribinacek, Shared::data.animations[0], ((float)(tick - fetch_Finish) / (float)(open_Finish - fetch_Finish)) * Shared::data.animations[0].frameCount);
        } else if(tick < eat_Finish) {
            float stage_Tick = Clamp((float)(tick - open_Finish) / (float)(eat_Finish - open_Finish), 0.f, 1.f);
            float next_Stage_Tick = Clamp((float)(tick + 1 - open_Finish) / (float)(eat_Finish - open_Finish), 0.f, 1.f);
            if(stage_Tick < 0.2f) {
                // 0 - 0.2
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                //target.x -= 0.2f;
                target.y += 2.5f;
                target.z += 0.6f;
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, stage_Tick / 0.2f);
                data.item_Data[Game_Data::SPOON].rotation = Lerp_Rotation(from_Spoon_Rotation, {-75.f, 0.f, 0.f}, stage_Tick / 0.2f);
                if(next_Stage_Tick < 0.2f != true) {
                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                }
            } else if(stage_Tick < 0.4f) {
                // 0.2 - 0.4
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                // target.x -= 0.2f;
                target.y += 1.f;
                target.z += 0.4f; // 0.8
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.2f) / 0.2f);
                if(next_Stage_Tick < 0.4f != true) {
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
                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                }
            } else {
                // 0.8 - 1
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                target.x -= 2.f;
                target.y += 2.f;
                target.z += 0.1f;
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.8f) / 0.2f);
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
            RayCollision houseCollision = GetRayCollisionBox(ray, data.house_BBoxes[m]);
            if(data.debug) DrawBoundingBox(data.house_BBoxes[m], ColorFromHSV((m * 70) % 360, 1.f, 1.f));

            /*
            Vector3 size = Vector3Subtract(data.house_BBoxes[m].max, data.house_BBoxes[m].min);
            Vector3 position = Vector3Divide(Vector3Add(data.house_BBoxes[m].max, data.house_BBoxes[m].min), {2.f, 2.f, 2.f});
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

            Matrix matrix = MatrixIdentity();
            matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));
            
            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));

            matrix = MatrixMultiply(matrix, MatrixRotateY(door_Data.rotation * DEG2RAD));
            
            matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

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

        if((!dragging && CheckCollisionPointCircle(GetTouchPosition(touch_Id), center, size) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) || (dragging && draggin_Id == touch_Id)) {
            DrawTextureEx(data.joystick_Pointer, {mouse_Point.x - size / 2.f, mouse_Point.y - size / 2.f}, 0.f, size / data.joystick_Pointer.width, WHITE);
            if(!dragging) {
                dragging = true;
                draggin_Id = touch_Id;
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

    void See_Player() {
        data.father_Start_Position = data.father_Position;
        data.father_Start_Rotation = data.father_Rotation;
        data.father_State = Game_Data::INSPECT;
    }

    void Init_UI() {
        float font_Size = GetScreenHeight() / 15.f;
        float button_Height = GetScreenHeight() / 8.f;

        data.play_Again_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 4.f * 2.f + button_Height}, u8"Hrát znovu", font_Size, Shared::data.medium_Font);
        data.menu_Button = Shared::Shared_Data::Button({GetScreenWidth() / 2.f, GetScreenHeight() / 4.f * 2.f}, u8"Zpátky do menu", font_Size, Shared::data.medium_Font);
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
        }

        data.default_Shader = LoadMaterialDefault().shader;

        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
            data.house_BBoxes.push_back(GetMeshBoundingBox(Shared::data.house.meshes[mesh]));
        }

        data.father = LoadModel(ASSETS_ROOT "models/human.iqm");

        Texture human = LoadTexture(ASSETS_ROOT "textures/human.png");
        SetMaterialTexture(&data.father.materials[0], MATERIAL_MAP_DIFFUSE, human);

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

        int door_Type;
        Material *material;

        while(doors_File >> position_X >> position_Y >> position_Z >>
                            scale_X >> scale_Y >> scale_Z >>
                            rotation_Start >> rotation_End >> door_Type) {
            switch(door_Type) {
                case 0: {
                    material = &Shared::data.house.materials[14];
                    break;
                }
                case 1: {
                    material = &Shared::data.house.materials[14];
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

        data.door = LoadModel(ASSETS_ROOT "models/door.glb");
        for(int material = 0; material < data.door.materialCount; material++)
            data.door.materials[material].shader = Shared::data.lighting;

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
                        data.key_Spawns.push_back(Game_Data::Item_Data(-1, {std::stof(strings[index]), std::stof(strings[index + 1]), std::stof(strings[index + 2])}, {0.f, std::stof(strings[index + 3]), 0.f}));
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

        data.item_Data.push_back(Game_Data::Item_Data(0, {0.f, 0.f, 0.f})); //              NONE
        data.item_Data.push_back(Game_Data::Item_Data(1, B2RL(26.5f, -41.f, 8.f))); //      PRIBINACEK
        data.item_Data.push_back(Game_Data::Item_Data(2, B2RL(-26.5f, 10.f, 4.4f))); //     SPOON
        data.item_Data.push_back(Game_Data::Item_Data(3, {0.f, 0.f, 0.f})); //              KEY

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
        data.hear = Game_Data::Variable_Sound(ASSETS_ROOT "audio/hear");

        int fogDensityLoc = GetShaderLocation(Shared::data.lighting, "fogDensity");
        SetShaderValue(Shared::data.lighting, fogDensityLoc, &data.fog_Density, SHADER_UNIFORM_FLOAT);

        Mod_Callback("Init_Game", (void*)&data);
    }

    bool Ray_Side_Collision(Ray ray, Vector3 old_Position) {
        RayCollision collision = Get_Collision_Ray(ray);
        if(collision.hit && collision.distance < 2.f) {
            return true;
        }

        if(data.debug) {
            DrawLine3D(Vector3Add(data.camera.position, {0.f, -0.75f, 0.f}), collision.point, Color {255, 0, 0, 32});
            DrawSphere(collision.point, 0.5f, Color {0, 0, 255, 32});
        }

        return false;
    }

    bool Ray_Sides_Collision(Vector3 camera_Position, Vector3 old_Position) {
        bool collision_1 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.f, 0.f, 0.1f}}, old_Position);
        bool collision_2 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.f, 0.f, -0.1f}}, old_Position);
        bool collision_3 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.1f, 0.f, 0.f}}, old_Position);
        bool collision_4 = Ray_Side_Collision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {-0.1f, 0.f, 0.f}}, old_Position);

        return collision_1 || collision_2 || collision_3 || collision_4;
    }

    std::vector<bool> Get_Door_States() {
        std::vector<bool> states = {};
        for(Game_Data::Door_Data door_Data : data.doors) {
            states.push_back(door_Data.opening);
        }

        return states;
    }

    void On_Switch() {
        DisableCursor();

        data.item_Data.clear();
        data.item_Data.push_back(Game_Data::Item_Data(0, {0.f, 0.f, 0.f})); //              NONE
        data.item_Data.push_back(Game_Data::Item_Data(1, B2RL(26.5f, -41.f, 8.f))); //      PRIBINACEK
        data.item_Data.push_back(Game_Data::Item_Data(2, B2RL(-26.5f, 10.f, 4.4f))); //     SPOON
        data.item_Data.push_back(Game_Data::Item_Data(3, {0.f, 0.f, 0.f})); //              KEY

        int key_Position_Index = rand() % data.key_Spawns.size();
        data.item_Data[3] = Game_Data::Item_Data(3, data.key_Spawns[key_Position_Index].position, data.key_Spawns[key_Position_Index].rotation);

        for(Game_Data::Item_Data &item : data.item_Data) {
            item.fall_Acceleration = 0.1f;
            item.Calculate_Collision();
            item.falling = true;
        }

        data.guide_Texts.clear();
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Probudíš se uprostřed noci a máš nepřekonatelnou chuť na pribináčka"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Tvým cílem je jít pro pribináčka a sníst si ho tady v pokoji"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Jo a táta vždycky na noc vypíná pojistky, aby jsem nemohl být na mobilu"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("...je trochu přehnaňe starostlivý"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("1. najdi pribináček a\n    vem ho do pokoje", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("2. najdi klíč", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("3. odemkni šuplík s\n     lžičkou a vem\n     ji do pokoje", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("4. dej si pribináček\n     v pokoji", false));

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

        data.fog_Density = 0.15f;

        /*
        data.camera.position = {-10.f, 20.f, 23.5f}; // {0.f, 7.5f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.target = {0.f, 20.f, 23.5f};
        */

        data.camera_Rotation = {0.f, 180.f, 0.f};
        data.old_Mouse_Position = {0.f, 0.f}; // Pozice kurzoru na předchozím framu
        data.start_Mouse_Position = {0.f, 0.f}; // Pozice kurzoru na začátku draggování
        data.previous_Rotated = false; // Pokud byl rotate "event" předchozí frame

        data.hear_Cooldown = 0.f;
        data.crouching = false;

        for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
            if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = Shared::data.lighting;
            }
        }

        data.debug = false;

        Mod_Callback("Switch_Game", (void*)&data);
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

        data.camera_Rotation = Vector3Add(data.camera_Rotation, {-delta.y * GetFrameTime() * 10.f, delta.x * GetFrameTime() * 10.f, 0.f});
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
        data.camera_Rotation = Vector3Add(data.camera_Rotation, {delta.y * GetFrameTime() * 5.f, -delta.x * GetFrameTime() * 5.f, 0.f});
        data.camera_Rotation.x = Clamp(data.camera_Rotation.x, -85.f, 85.f);
    }

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

    // Získat kolizi cylindru (ze předu delší) s mapou
    bool Get_Collision_Cylinder(Vector3 position, float radius) {
        for (int m = 0; m < Shared::data.house.meshCount; m++) {
            bool collide = CheckCollisionBoxSphere(data.house_BBoxes[m], position, radius);
            if(collide) return true;
        }
        for(Game_Data::Door_Data &door_Data : data.doors) {
            if(door_Data.opening) continue;

            Matrix matrix = MatrixIdentity();
            matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));
            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));
            
            matrix = MatrixMultiply(matrix, MatrixRotateY(door_Data.rotation * DEG2RAD));
            matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));
            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

            BoundingBox door = Get_Model_BBox_Matrix(data.door, matrix);
            bool collide = CheckCollisionBoxSphere(door, position, radius);
            if(collide) return true;
        }
        return false;
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

            Matrix matrix = MatrixIdentity();
            matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));

            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));
            matrix = MatrixMultiply(matrix, MatrixRotateY(door_Data.rotation * DEG2RAD));
            matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

            DrawMesh(data.door.meshes[0], *door_Data.material, matrix);
            
            Matrix matrixDoorHandle = MatrixIdentity();
            // matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(0.75f, 0.75f, 0.75f));
            float size = door_Data.scale.y / 5.f * 0.75f;
            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(size, size, size));

            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.scale.x + door_Data.scale.x / 2.f, door_Data.scale.y / 4.f, -door_Data.scale.z));
            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixRotateY(door_Data.rotation * DEG2RAD));
            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

            matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));
            DrawMesh(data.door_Handle, *door_Data.material, matrixDoorHandle);

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

                if(item_Visible && data.holding_Item != (Game_Data::Item)index) {
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

            if(data.fuse_Box.lever_Turning) {
                data.fog_Density = 0.05f;
                for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
                    if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                        Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = data.default_Shader;
                    }
                }

                for(Light &light : data.lights) {
                    light.enabled = true;
                    UpdateLightValues(Shared::data.lighting, light);
                }
            } else {
                data.fog_Density = 0.15f;
                for(int mesh = 0; mesh < Shared::data.house.meshCount; mesh++) {
                    if(Shared::data.house.meshes[mesh].vertexCount == LIGHT_BASE_VERTICES) {
                        Shared::data.house.materials[Shared::data.house.meshMaterial[mesh]].shader = Shared::data.lighting;
                    }
                }

                for(Light &light : data.lights) {
                    light.enabled = false;
                    UpdateLightValues(Shared::data.lighting, light);
                }
            }
        }
    }

    void Update() {
        ClearBackground(BLACK);
        SetShaderValue(Shared::data.lighting, Shared::data.lighting.locs[SHADER_LOC_VECTOR_VIEW], &data.camera.position.x, SHADER_UNIFORM_VEC3);

        Shared::data.flashlight.position = data.camera.position;
        UpdateLightValues(Shared::data.lighting, Shared::data.flashlight);

        Vector3 old_Position = data.camera.position;
        int fogDensityLoc = GetShaderLocation(Shared::data.lighting, "fogDensity");
        SetShaderValue(Shared::data.lighting, fogDensityLoc, &data.fog_Density, SHADER_UNIFORM_FLOAT);

        data.action_Used = false; // If any action was used this frame (preventing click-through)

        BeginMode3D(data.camera); {
            Mod_Callback("Update_Game_3D", (void*)&data);

            data.frame_Counter++;
            if(data.frame_Counter > 100) data.frame_Counter = 0;

            if(!data.death_Animation_Playing) UpdateModelAnimation(data.father, data.animations[0], data.animation_Frame_Count);

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

                    if(can_Update) {
                        data.wake_Animation_Tick += 1.f * GetFrameTime();
                    }

                    if(data.wake_Animation_Tick >= data.wake_Animation.size() - 1) {
                        data.wake_Animation_Finished = true;
                    }
                    
                    data.camera.position = current_Position;
                    data.camera_Target = current_Target;
                }
            }

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

                if(can_Update) {
                    data.death_Animation_Tick += 1.f * GetFrameTime();
                }
            }

            /* DOORS */ {
                bool door_Opened = false;
                for(Game_Data::Door_Data &door_Data : data.doors) {
                    // DrawModelEx(door, door_Data.position, {0.f, 1.f, 0.f}, door_Data.rotation, door_Data.scale, WHITE);
                    // DrawLine3D(data.camera.position, door_Data.position, RED);

                    Matrix matrix = MatrixIdentity();
                    matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));

                    matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));
                    matrix = MatrixMultiply(matrix, MatrixRotateY(door_Data.rotation * DEG2RAD));
                    matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

                    matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

                    DrawMesh(data.door.meshes[0], *door_Data.material, matrix);
                    
                    Matrix matrix_Door_Handle = MatrixIdentity();

                    // matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(0.75f, 0.75f, 0.75f));
                    float size = Remap(door_Data.scale.y, 0.f, 5.f, 0.35f, 0.75f);
                    matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixScale(size, size, size));

                    matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixTranslate(door_Data.scale.x + door_Data.scale.x / 2.f, door_Data.scale.y / 4.f, -door_Data.scale.z));
                    matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixRotateY(door_Data.rotation * DEG2RAD));
                    matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

                    matrix_Door_Handle = MatrixMultiply(matrix_Door_Handle, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));
                    
                    DrawMesh(data.door_Handle, *door_Data.material, matrix_Door_Handle);
                    
                    if(door_Data.type != 0) {
                        if(door_Data.start_Rotation_Range < door_Data.end_Rotation_Range) {
                            if(door_Data.opening) {
                                if(door_Data.rotation < door_Data.end_Rotation_Range) {
                                    door_Data.rotation += 100.f * GetFrameTime();
                                }
                            } else {
                                if(door_Data.rotation > door_Data.start_Rotation_Range) {
                                    door_Data.rotation -= 100.f * GetFrameTime();
                                }
                            }
                        } else {
                            if(door_Data.opening) {
                                if(door_Data.rotation > door_Data.end_Rotation_Range) {
                                    door_Data.rotation -= 100.f * GetFrameTime();
                                }
                            } else {
                                if(door_Data.rotation < door_Data.start_Rotation_Range) {
                                    door_Data.rotation += 100.f * GetFrameTime();
                                }
                            }
                        }
                    } else {
                        bool door_Updated = false;
                        if(Vector3Distance(door_Data.position, data.camera.position) < 5.f) {
                            Matrix matrix = MatrixIdentity();
                            matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));
                            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));
                            matrix = MatrixMultiply(matrix, MatrixRotateY(door_Data.rotation * DEG2RAD));
                            matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));
                            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

                            Vector3 target = Vector3Transform({0.f, 0.f, 0.f}, matrix);
                            Ray raycast = {data.camera.position, Vector3Divide(Vector3Subtract(target, data.camera.position), {10.f, 10.f, 10.f})};

                            RayCollision collision = GetRayCollisionMesh(raycast, data.door.meshes[0], matrix);

                            if(collision.hit) {
                                Vector3 diff = Vector3Subtract(collision.point, Vector3Add(collision.point, collision.normal));
                                float angle = fmod((atan2(diff.x, diff.z) * RAD2DEG) + 180.f, 360.f);
                                float normalized_Rotation = Wrap(door_Data.rotation, 0.f, 360.f);
                                Vector2 distances = {fabs(normalized_Rotation - angle), fabs(normalized_Rotation - fmod(angle + 180.f, 360.f))};
                                
                                if(distances.x < distances.y) {
                                    door_Data.rotation += (IsKeyDown(KEY_LEFT_SHIFT) ? 150.f : 100.f) * GetFrameTime();
                                } else {
                                    door_Data.rotation -= (IsKeyDown(KEY_LEFT_SHIFT) ? 150.f : 100.f) * GetFrameTime();
                                }

                                door_Updated = true;
                            }
                        }

                        Vector3 father_Position = Vector3Add(data.father_Position, {0.f, 6.5f, 0.f});
                        if(Vector3Distance(door_Data.position, father_Position) < 5.f) {
                            Matrix matrix = MatrixIdentity();
                            matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));
                            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));
                            matrix = MatrixMultiply(matrix, MatrixRotateY(door_Data.rotation * DEG2RAD));
                            matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));
                            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

                            Vector3 target = Vector3Transform({0.f, 0.f, 0.f}, matrix);
                            Ray raycast = {father_Position, Vector3Divide(Vector3Subtract(target, father_Position), {10.f, 10.f, 10.f})};

                            RayCollision collision = GetRayCollisionMesh(raycast, data.door.meshes[0], matrix);

                            if(collision.hit) {
                                Vector3 diff = Vector3Subtract(collision.point, Vector3Add(collision.point, collision.normal));
                                float angle = fmod((atan2(diff.x, diff.z) * RAD2DEG) + 180.f, 360.f);
                                float normalized_Rotation = Wrap(door_Data.rotation, 0.f, 360.f);
                                Vector2 distances = {fabs(normalized_Rotation - angle), fabs(normalized_Rotation - fmod(angle + 180.f, 360.f))};
                                
                                if(distances.x < distances.y) {
                                    door_Data.rotation += 100.f * GetFrameTime();
                                } else {
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

            DrawModel(Shared::data.house, {0.f, 0.f, 0.f}, 1.f, WHITE);

            // ------------------

            /* ITEMS */ {
                Ray player_Ray = GetMouseRay({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, data.camera);
                
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

            float nearest_Hit = 10000.f;
            int nearest_Hit_Id = -1;

            int index = 0;
            for(Game_Data::Drawer_Data &drawer : data.drawers) {
                DrawModel(data.drawer, drawer.position, 1.f, WHITE);
                if(drawer.has_Lock) DrawModel(data.lock, Vector3Add(drawer.position, {1.95f, 1.f, 0.f}), 1.f, WHITE);
                if(drawer.opening) {
                    drawer.position.x += 10.f * GetFrameTime();
                    for(Game_Data::Item_Data* child : drawer.childs)
                        if(child != nullptr)
                            child->position.x += 10.f * GetFrameTime();
                    if(drawer.position.x > drawer.original_Position.x + 2.f) {
                        drawer.position.x -= 10.f * GetFrameTime();
                        for(Game_Data::Item_Data* child : drawer.childs)
                            if(child != nullptr)
                                child->position.x -= 10.f * GetFrameTime();
                    }
                } else {
                    drawer.position.x -= 10.f * GetFrameTime();
                    for(Game_Data::Item_Data* child : drawer.childs)
                        if(child != nullptr) {
                            child->position.x -= 10.f * GetFrameTime();
                        }
            
                    if(drawer.position.x < drawer.original_Position.x) {
                        drawer.position.x += 10.f * GetFrameTime();
                        for(Game_Data::Item_Data* child : drawer.childs)
                            if(child != nullptr)
                                child->position.x += 10.f * GetFrameTime();
                    }
                }

                index++;
            }

            // ----------------------

            if(data.debug) data.fog_Density += GetMouseWheelMove() / 100.f;
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
                data.fuse_Box.lever_Tick += GetFrameTime() * 80.f;
            } else {
                data.fuse_Box.lever_Tick -= GetFrameTime() * 80.f;
            }

            data.fuse_Box.lever_Tick = Clamp(data.fuse_Box.lever_Tick, 0.f, data.fuse_Box.animations[0].frameCount - 1.f);
            

            /* FATHER */ {
                /* {
                    if(IsKeyPressed(KEY_SPACE)) {
                        data.father_Points.push_back(Game_Data::Father_Point(data.camera.position, Get_Door_States()));
                    } else if(IsKeyPressed(KEY_BACKSPACE)) {
                        data.father_Points.back() = Game_Data::Father_Point(data.camera.position, Get_Door_States());
                    } else if(IsKeyPressed(KEY_ENTER)) {
                        for(Game_Data::Father_Point point : data.father_Points) {
                            std::string doors = "";
                            for(bool state : point.door_States) {
                                doors += " " + std::to_string(state);
                            }
                
                            std::cout << point.position.x << " " << point.position.y << " " << point.position.z << doors << std::endl;
                        }
                    }
                } */

                if(data.debug) {
                    Vector3 previous_Point = {0.f, 0.f, 0.f};
                    int index = 0;
                    for(Game_Data::Father_Point point : data.father_Points) {
                        if(index > 0) {
                            DrawLine3D(previous_Point, point.position, RED);
                        }
                        bool invalid = Get_Collision_Cylinder(point.position, 1.5f);
                        DrawSphere(point.position, .5f, invalid ? RED : BLUE);
                        previous_Point = point.position;
                        index++;
                    }
                }

                if(!data.death_Animation_Playing) {
                    switch(data.father_State) {
                        case Game_Data::INSPECT: {
                            Vector3 old_Position = data.father_Position;
                            data.father_Position = Vector3MoveTowards(data.father_Position, data.camera.position, GetFrameTime() * 10.f);

                            bool collide_X = Get_Collision_Cylinder({data.father_Position.x, data.father_Position.y + 6.5f, old_Position.z}, 1.5f);
                            if(collide_X) data.father_Position.x = old_Position.x;

                            bool collide_Z = Get_Collision_Cylinder({old_Position.x, data.father_Position.y + 6.5f, data.father_Position.z}, 1.5f);
                            if(collide_Z) data.father_Position.z = old_Position.z;

                            Vector3 diff = Vector3Subtract(data.camera.position, data.father_Position);
                            float angle = -atan2(diff.z, diff.x) * RAD2DEG;

                            float addition = (((((int)angle - (int)data.father_Rotation) % 360) + 540) % 360) - 180;
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
                            data.father_Rotation += addition / 20.f;

                            data.return_Tick += 1.f * GetFrameTime();
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
                            data.keyframe_Tick += 1.f * GetFrameTime();

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

                Ray ray = {{data.father_Position.x, data.father_Position.y + 3.5f, data.father_Position.z}, {0.f, -0.1f, 0.f}};
                float y = Get_Collision_Ray(ray).point.y;

                data.father_Position.y = y;

                DrawModelEx(data.father, data.father_Position, {0.f, 1.f, 0.f}, data.father_Rotation + 90.f, Vector3 {12.f, 12.f, 12.f}, WHITE);
            }

            if(Player_Visible()) {
                See_Player();
            }
        } EndMode3D();

        Mod_Callback("Update_Game_2D", (void*)&data, false);

        data.win.Update();

        const float crosshair_size = 10.f;
        DrawLineEx({GetScreenWidth() / 2.f - crosshair_size, GetScreenHeight() / 2.f}, {GetScreenWidth() / 2.f + crosshair_size, GetScreenHeight() / 2.f}, 2.f, Fade(WHITE, 0.2f));
        DrawLineEx({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f - crosshair_size}, {GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + crosshair_size}, 2.f, Fade(WHITE, 0.2f));

        if(Shared::data.mobile_Mode.ticked) {
            data.camera.target = Vector3Add(data.camera.position, data.camera_Target);
            data.camera_Target = Vector3RotateByQuaternion({0.f, 0.f, 10.f}, QuaternionFromEuler(data.camera_Rotation.x * DEG2RAD, data.camera_Rotation.y * DEG2RAD, data.camera_Rotation.z * DEG2RAD));

            if(data.wake_Animation_Finished && !data.death_Animation_Playing) {
                data.movement.Render();
                bool rotation_Updated = false;

                if(data.movement.draggin_Id >= GetTouchPointCount())
                    data.movement.dragging = false;

                float size = GetScreenHeight() / 1300.f;
                float margin = GetScreenHeight() / 30.f;

                Texture texture = data.crouching ? data.un_Crouch : data.crouch;
                DrawTextureEx(texture, {(float)GetScreenWidth() - texture.width * size - margin, margin}, 0.f, size, WHITE);
                Rectangle crouch_Rectangle = {(float)GetScreenWidth() - texture.width * size - margin, margin, texture.width * size, texture.height * size};

                crouch_Rectangle.x -= crouch_Rectangle.width / 2.f;
                crouch_Rectangle.y -= crouch_Rectangle.height / 2.f;
                crouch_Rectangle.width *= 2.f;
                crouch_Rectangle.height *= 2.f;

                if(data.holding_Crouch >= GetTouchPointCount() || !CheckCollisionPointRec(GetTouchPosition(data.holding_Crouch), crouch_Rectangle)) {
                    data.holding_Crouch = -1;
                }

                bool camera_Rotated = false;
                for(int index = 0; index < GetTouchPointCount(); index++) {
                    int id = GetTouchPointId(index);

                    bool can_Update = data.movement.Can_Update(id);
                    Game_Data::Joystick_Data joystick = data.movement.Update(id);

                    if(CheckCollisionPointRec(GetTouchPosition(index), crouch_Rectangle)) {
                        if(data.holding_Crouch != id) {
                            data.crouching = !data.crouching;
                            data.holding_Crouch = id;
                        }
                    } else {
                        if (can_Update) {
                            rotation_Updated = true;
                            if (data.wake_Animation_Finished && !data.death_Animation_Playing && !data.win.playing) {
                                Update_Camera_Android(id, data.previous_Rotated);
                                camera_Rotated = true;
                            }
                        } else {
                            if (joystick.moving && data.wake_Animation_Finished && !data.death_Animation_Playing && !data.win.playing) {
                                float speed = 1.f;
                                if (data.crouching) speed /= 1.5f;
                                if (!data.crouching && IsKeyDown(KEY_LEFT_SHIFT)) speed *= 1.5f;

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

            if(data.wake_Animation_Finished && !data.death_Animation_Playing && !data.win.playing) {
                float speed = 1.f;
                if(data.crouching) speed /= 1.5f;
                if(!data.crouching && IsKeyDown(KEY_LEFT_SHIFT)) speed *= 1.5f;

                Update_Camera_Desktop(speed);
            }
        }

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

        if(!data.guide_Finished) {
            if(data.guide_Texts[data.guide_Index].Update()) {
                data.guide_Index++;
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

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rectangle)) {
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
                if(IsKeyPressed(KEY_E)) {
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

        if(data.guide_Finished) {
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
                    for(int angle = 0; angle < 360; angle += 20) {
                        Vector2 offset = {cos(angle * DEG2RAD) * 3.f, sin(angle * DEG2RAD) * 3.f};

                        Shared::DrawTextExC(Shared::data.bold_Font, u8"Prohrál jsi", Vector2Add({GetScreenWidth() / 2.f, GetScreenHeight() / 4.f}, offset), font_Size, 1.f, BLACK);
                    }
                    Shared::DrawTextExC(Shared::data.bold_Font, u8"Prohrál jsi", {GetScreenWidth() / 2.f, GetScreenHeight() / 4.f}, font_Size, 1.f, WHITE);
                    
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
                
                    if(data.play_Again_Button.Update()) { On_Switch(); Shared::data.coins += coins; } // resetuje všechen progress
                    else if(data.menu_Button.Update()) { Switch_To_Scene(MENU); Shared::data.coins += coins; }

                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorTint(color, BLACK));
                } else if((data.death_Animation_Tick + 1.f * GetFrameTime()) - 2.f > (float)data.death_Animation.size() - 1.f) {
                    EnableCursor();
                    HideCursor();

                    Show_Interstitial_Ad();
                }
            }
        }

        if(Vector3Distance(Vector3Add(data.father_Position, {0.f, 6.5f, 0.f}), data.camera.position) < 2.5f) {
            data.death_Animation_Playing = true;
            data.holding_Item = Game_Data::Item::NONE;
            data.camera_Start_Position = data.camera.position;
            data.camera_Start_Target = data.camera_Target;
            UpdateModelAnimation(data.father, data.animations[0], 16);
        }

        if(data.hear_Cooldown <= 0.f) {
            data.hear_Cooldown = rand() % 10 + 15;

            bool death_Screen = data.death_Animation_Tick - 2.f > (float)data.death_Animation.size() - 1.f;

            if(!data.hear.Playing() && !death_Screen & !data.win.playing) {
                float volume = Remap(Vector3Distance(data.camera.position, data.father_Position), 0.f, 50.f, 1.f, 0.f);
                volume = Clamp(volume, 0.f, 1.f);
                data.hear.Play(volume);
            }
        } else {
            data.hear_Cooldown -= GetFrameTime();
        }

        /*
        if(IsKeyPressed(KEY_P)) {
            data.item_Data[Game_Data::Item::SPOON].position = Vector3Add(data.players_Room_Table.min, {0.f, 1.f, 0.f});
            data.item_Data[Game_Data::Item::PRIBINACEK].position = Vector3Add(data.players_Room_Table.min, {0.f, 1.f, 0.f});
        }
        */

        Mod_Callback("Update_Game_2D", (void*)&data, true);

        if(Shared::data.show_Fps.ticked) {
            const char* text = TextFormat("FPS: %d", GetFPS());
            float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 40.f;

            Vector2 size = MeasureTextEx(Shared::data.medium_Font, text, font_Size, 0.f);
            Shared::DrawTextExOutline(Shared::data.medium_Font, text, {GetScreenWidth() - size.x / 2.f - font_Size, GetScreenHeight() - size.y / 2.f - font_Size}, font_Size, 0.f, WHITE);
        }
    }
};

#endif // GAME_CXX
