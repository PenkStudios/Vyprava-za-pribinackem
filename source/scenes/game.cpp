#ifndef GAME_CXX
#define GAME_CXX

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#define LOG(message, ...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "PRIBINACEK", "[%s %s] " message, __FILE_NAME__, __func__, ##__VA_ARGS__))
#else
#define LOG(message, ...) ((void)printf("PRIBINACEK: " message "\n", ##__VA_ARGS__))
#endif

#define RLIGHTS_IMPLEMENTATION
#include "../rlights.h"
#include "../reasings.c"

#include "../scene.cpp"
#include "../mod_loader.cpp"

#include "menu.cpp"

// Convert blender coords to raylib coords
#define B2RL(x, y, z) {x, z, -y}

// Map constants. Probably could be in `config.txt`
#define CHAIR_POSITION B2RL(-8.2f, -7.5f, 20)
#define EATING_ROTATION 90.f
#define PRIBINACEK_WIN_ANIMATION {-4.60577, 18.2854, 7.56896}
#define SPOON_ROTATION_WIN_ANIMATION {0.f, 0.f, 0.f}
#define SPOON_WIN_ANIMATION {-4.38755, 18.2854, 8.88417}
#define PLAYERS_ROOM_TABLE {{-6.04131, 17.3548, 3.39275}, {-0.713218, 18.2854, 12.3022}}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Odkomentujte pro android UI
// #define ANDROID_UI

namespace Game {
    class Game_Data {
    public:
        class Door_Data {
        public:
            Vector3 position;
            Vector3 scale;
            float default_Rotation;
            float opened_Rotation;

            Material* material;

            float rotation; // Rotation caused by player
            float rotation_Father; // Rotation caused by daddy

            // -||-
            bool opening;
            bool opening_Father;
            Door_Data(Vector3 door_Position, Vector3 door_Scale, float door_Default_Rotation, float door_Opened_Rotation, Material* material) :
                position(door_Position), scale(door_Scale), default_Rotation(door_Default_Rotation),
                opened_Rotation(door_Opened_Rotation), rotation(door_Default_Rotation),
                rotation_Father(door_Default_Rotation), material(material), opening(false) {}
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

        Shader lighting;
        Model house;
        std::vector<BoundingBox> house_BBoxes;
        
        Model father;
        Camera3D camera;
        Light flashlight;
        Model door;
        ModelAnimation *animations;
        Mesh doorHandle;
        float fall_Acceleration = 0.f;

        bool debug = false;
        float fog_Density = 0.15f;
        bool crouching = false;
        int frame_Counter = 0;
        float animation_Frame_Count = 0;

        std::vector<Father_Point> father_Points = {};
        float keyframe_Tick = 0.f;
        int keyframe = 0;
        float enemy_Direction = 0.f;

        class Directional_Position {
        public:
            Vector3 position;
            Vector3 target;

            Directional_Position(Vector3 position, Vector3 target) : position(position), target(target) {}
        };
        std::vector<Directional_Position> wake_Animation = {};
        float wake_Animation_Tick = 0.f;
        bool wake_Animation_Finished = false;

        Texture joystick_Base;
        Texture joystick_Pointer;

        Texture crouch;
        Texture un_Crouch;

        Vector3 camera_Rotation = {0.f, 180.f, 0.f};
        Vector2 old_Mouse_Position = {0.f, 0.f};
        bool previous_Rotated = false; // If there was a rotate "event" previous frame

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

        int microphone_Id = 0;
        int microphone_Change_Cooldown;

        float max_Audio = 0.f;
        bool searching_Microphone = true;

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

        Vector3 camera_Target = {0.f, 0.f, 1.f}; // Camera rotation from the point {0, 0, 0}

        class Guide_Caption {
        public:
            std::string target_Text;
            bool can_Skip; // tap to skip (for story)

            std::string text; // slowly written
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

            // Returns true if skipping this frame
            bool Update() {
                frame += 20.f * GetFrameTime();
                if(frame > target_Text.size()) {
                    frame = target_Text.size();
                }

                text = _Utf8_Substr(target_Text, 0, (int)frame);

                float margin = GetScreenWidth() / 50.f;
                float height = GetScreenHeight() / 6.f;
                Rectangle rectangle = {margin, margin, GetScreenWidth() - margin * 2.f, height};

                DrawRectangleRounded(rectangle, 0.1f, 20, Color {0, 0, 0, 180});
                DrawRectangleRoundedLinesEx(rectangle, 0.1f, 20, 5.f, Color {0, 0, 0, 255});

                Vector2 text_Size = MeasureTextEx(Menu::data.medium_Font, text.c_str(), 30.f, 0.f);
                DrawTextEx(Menu::data.medium_Font, text.c_str(), {rectangle.x + rectangle.width / 2.f - text_Size.x / 2.f, rectangle.y + rectangle.height / 2.f - text_Size.y / 2.f}, 30.f, 0.f, WHITE);

                if(can_Skip && IsKeyPressed(KEY_SPACE)) {
                    return true;
                }
                return false;
            }

            Guide_Caption(std::string text) : target_Text(text), can_Skip(true), text("") {}
            Guide_Caption(std::string text, bool can_Skip) : target_Text(text), can_Skip(can_Skip), text("") {}
        };

        int guide_Index = 0;
        std::vector<Guide_Caption> guide_Texts = {};

        bool guide_Finished = false;
        std::vector<Item_Data> key_Spawns;

        const BoundingBox players_Room_Table = PLAYERS_ROOM_TABLE;
        
        Model drawer;
        BoundingBox drawer_BBox;

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
            int tick = 0;

            int walk_Finish;
            int fetch_Finish; // fetching of pribináček + spoon
            int open_Finish; // opening pribinacek
            int eat_Finish; // eating it

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
    } data;

    void Game_Data::Win_Animation::Play() {
        // Once got into the room with pribináček & spoon
        walk_Finish = Vector3Distance(data.camera.position, CHAIR_POSITION) * 10;
        pribinacek_Fetch = Vector3DistanceSqr(data.item_Data[Game_Data::PRIBINACEK].position, PRIBINACEK_WIN_ANIMATION) * 0.1;
        spoon_Fetch = Vector3DistanceSqr(data.item_Data[Game_Data::SPOON].position, SPOON_WIN_ANIMATION) * 0.1;
        fetch_Finish = MAX(pribinacek_Fetch, spoon_Fetch) + walk_Finish;
        open_Finish = fetch_Finish + Menu::data.animations[0].frameCount - 1;
        eat_Finish = open_Finish + GetFPS() * 2.5f;

        from_Rotation = data.camera_Rotation;
        from_Position = data.camera.position;

        from_Pribinacek_Position = data.item_Data[Game_Data::PRIBINACEK].position;
        from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
        from_Spoon_Rotation = data.item_Data[Game_Data::SPOON].rotation;

        playing = true;
    }

    Vector3 LerpVector3XYZ(Vector3 source, Vector3 target, float amount) {
        Vector3 addition = {(float)(((((int)target.x - (int)source.x) % 360) + 540) % 360) - 180,
                            (float)(((((int)target.y - (int)source.y) % 360) + 540) % 360) - 180,
                            (float)(((((int)target.z - (int)source.z) % 360) + 540) % 360) - 180};
        return Vector3Add(source, Vector3Multiply(addition, {amount, amount, amount}));
    }

    void Game_Data::Win_Animation::Update() {
        if(!playing) return;
        if(IsKeyDown(KEY_T)) tick++;

        if(tick < walk_Finish) {
            int addition = (((((int)EATING_ROTATION - (int)from_Rotation.y) % 360) + 540) % 360) - 180;
            data.camera_Rotation = Vector3Lerp(from_Rotation, {0.f, from_Rotation.y + addition, 0.f}, (float)tick / (float)walk_Finish);
            data.camera.position = Vector3Lerp(from_Position, CHAIR_POSITION, (float)tick / (float)walk_Finish);
        } else if(tick < fetch_Finish) {
            data.item_Data[Game_Data::PRIBINACEK].position = Vector3Lerp(from_Pribinacek_Position, PRIBINACEK_WIN_ANIMATION, Clamp((float)(tick - walk_Finish) / (float)pribinacek_Fetch, 0.f, 1.f));
            data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, SPOON_WIN_ANIMATION, Clamp((float)(tick - walk_Finish) / (float)spoon_Fetch, 0.f, 1.f));
            data.item_Data[Game_Data::SPOON].rotation = Vector3Lerp(from_Spoon_Rotation, SPOON_ROTATION_WIN_ANIMATION, Clamp((float)(tick - walk_Finish) / (float)spoon_Fetch, 0.f, 1.f));
            if(tick + 1 >= fetch_Finish) {
                from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                from_Spoon_Rotation = data.item_Data[Game_Data::SPOON].rotation;
            }
        } else if(tick < open_Finish) {
            Menu::data.animation_Frame_Counter++;
            UpdateModelAnimation(Menu::data.pribinacek, Menu::data.animations[0], Menu::data.animation_Frame_Counter);
        } else if(tick < eat_Finish) {
            float stage_Tick = Clamp((float)(tick - open_Finish) / (float)(eat_Finish - open_Finish), 0.f, 1.f);
            float next_Stage_Tick = Clamp((float)(tick + 1 - open_Finish) / (float)(eat_Finish - open_Finish), 0.f, 1.f);
            if(stage_Tick < 0.33f) {
                // 0 - 0.33
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                target.x += 0.2f;
                target.y += 2.f;
                target.z += 0.4f;
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, stage_Tick / 0.33f);
                data.item_Data[Game_Data::SPOON].rotation = LerpVector3XYZ(from_Spoon_Rotation, {-90.f, 0.f, 0.f}, stage_Tick / 0.33f);
                if(next_Stage_Tick < 0.33f != true) {
                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                    from_Spoon_Rotation = data.item_Data[Game_Data::SPOON].rotation;
                }
            } else if(stage_Tick < 0.66f) {
                // 0.33 - 0.66
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                target.x += 0.2f;
                target.z += 0.4f;
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.33f) / 0.33f);
                if(next_Stage_Tick < 0.66f != true) {
                    from_Spoon_Position = data.item_Data[Game_Data::SPOON].position;
                }
            } else {
                Vector3 target = PRIBINACEK_WIN_ANIMATION;
                target.x += 0.2f;
                target.y += 2.f;
                target.z += 0.4f;
                data.item_Data[Game_Data::SPOON].rotation = LerpVector3XYZ(from_Spoon_Rotation, {180.f, -90.f, 0.f}, stage_Tick / 0.33f);
                data.item_Data[Game_Data::SPOON].position = Vector3Lerp(from_Spoon_Position, target, (stage_Tick - 0.66f) / 0.33f);
                // 0.66 - 1
            }
        }
    }

    Game_Data::Drawer_Data::Drawer_Data(Vector3 position, bool has_Lock) : has_Lock(has_Lock), position(position), original_Position(position), opening(false) {
        for(int index = 0; index < MAX_ITEMS; index++)
            childs.push_back(nullptr);
    }

    // Get map collision with ray (map + doors + drawers)
    RayCollision Get_Collision_Ray(Ray ray, Game_Data::Item_Data *item = nullptr) {
        RayCollision collision = { 0 };
        collision.distance = 1000000.f;
        for (int m = 0; m < data.house.meshCount; m++)
        {
            // NOTE: We consider the model.transform for the collision check but 
            // it can be checked against any transform Matrix, used when checking against same
            // model drawn multiple times with multiple transforms
            RayCollision houseCollision = GetRayCollisionBox(ray, data.house_BBoxes[m]);
            if(data.debug) DrawBoundingBox(data.house_BBoxes[m], ColorFromHSV((m * 70) % 360, 1.f, 1.f));

            if (houseCollision.hit)
            {
                if(houseCollision.distance < collision.distance) {
                    collision = houseCollision;
                }
            }
        }

        for(Game_Data::Door_Data &door_Data : data.doors) {
            // Fun fact: Matrix multiplication order matters
            // bruh it literaly says that in the MatrixMultiply documentation ._.

            Matrix matrix = MatrixIdentity();
            matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));
            
            matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));

            float rotation_Player = door_Data.rotation;
            float rotation_Father = door_Data.rotation_Father;

            float rotation = 0.f;
            if(door_Data.default_Rotation < door_Data.opened_Rotation) {
                rotation = rotation_Player > rotation_Father ? rotation_Player : rotation_Father;
            } else {
                rotation = rotation_Player < rotation_Father ? rotation_Player : rotation_Father;
            }
            matrix = MatrixMultiply(matrix, MatrixRotateY(rotation * DEG2RAD));
            
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
            Matrix matrix = MatrixIdentity();
            matrix = MatrixMultiply(matrix, MatrixTranslate(drawer_Data.position.x, drawer_Data.position.y, drawer_Data.position.z));

            RayCollision doorCollision = GetRayCollisionMesh(ray, data.drawer.meshes[0], matrix);
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
        int target_Breakpoint = -1;
        float target_Breakpoint_Distance = 10000.f;

        int index = 0;
        for(Game_Data::Father_Point point : data.father_Points) {
            float distance = Vector3DistanceSqr(point.position, data.camera.position);
            if(distance < target_Breakpoint_Distance) {
                target_Breakpoint = index;
                target_Breakpoint_Distance = distance;
            }
            index++;
        }

        data.enemy_Direction = 1.f * GetFrameTime();
        if(target_Breakpoint < data.keyframe) {
            data.enemy_Direction *= -1.f;
        }
    }

    void Init() {
        data.camera.position = {-10.f, 20.f, 23.5f}; // {0.f, 7.5f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.target = {0.f, 20.f, 23.5f};
        data.camera.fovy = 90.f;
        data.camera.projection = CAMERA_PERSPECTIVE;

#ifdef ANDROID_UI
        data.joystick_Base = LoadTexture("textures/joystick_base.png");
        SetTextureFilter(data.joystick_Base, TEXTURE_FILTER_BILINEAR);

        data.joystick_Pointer = LoadTexture("textures/joystick.png");
        SetTextureFilter(data.joystick_Pointer, TEXTURE_FILTER_BILINEAR);

        float margin = GetScreenHeight() / 30.f;
        data.movement = Game_Data::Joystick({GetScreenHeight() / 5.f + margin, GetScreenHeight() - GetScreenHeight() / 5.f - margin}, GetScreenHeight() / 5.f);

        data.crouch = LoadTexture("textures/crouch.png");
        SetTextureFilter(data.crouch, TEXTURE_FILTER_BILINEAR);

        data.un_Crouch = LoadTexture("textures/uncrouch.png");
        SetTextureFilter(data.un_Crouch, TEXTURE_FILTER_BILINEAR);
#endif

        data.house = LoadModel("models/house.glb");
        for(int material = 0; material < data.house.materialCount; material++) {
            SetTextureFilter(data.house.materials[material].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_BILINEAR);
        }

        for(int mesh = 0; mesh < data.house.meshCount; mesh++) {
            data.house_BBoxes.push_back(GetMeshBoundingBox(data.house.meshes[mesh]));
        }

        data.father = LoadModel("models/human.iqm");

        Texture human = LoadTexture("textures/human.png");
        SetMaterialTexture(&data.father.materials[0], MATERIAL_MAP_DIFFUSE, human);

        int animation_Count = 0; // (1)
        data.animations = LoadModelAnimations("models/human.iqm", &animation_Count);

#ifdef PLATFORM_ANDROID
        data.lighting = LoadShader("shaders/vertex100.glsl", "shaders/fragment100.glsl");
#else
        data.lighting = LoadShader("shaders/vertex330.glsl", "shaders/fragment330.glsl");
#endif

        data.lighting.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(data.lighting, "matModel");
        data.lighting.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(data.lighting, "viewPos");

        int ambientLoc = GetShaderLocation(data.lighting, "ambient");
        float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
        SetShaderValue(data.lighting, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

        for(int material = 0; material < data.father.materialCount; material++)
            data.father.materials[material].shader = data.lighting;

        for(int material = 0; material < data.house.materialCount; material++)
            data.house.materials[material].shader = data.lighting;

        data.doorHandle = GenMeshSphere(0.5f, 15, 15);

        data.flashlight = CreateLight(LIGHT_POINT, data.camera.position, {0.f, 0.f, 0.f}, WHITE, data.lighting);

        std::istringstream ai_File(LoadFileText("ai.txt"));

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

        std::istringstream doors_File(LoadFileText("doors.txt"));
        float position_X, position_Y, position_Z,
              scale_X, scale_Y, scale_Z,
              rotation_Start, rotation_End;

        int material_Id;
        Material *material;

        while(doors_File >> position_X >> position_Y >> position_Z >>
                            scale_X >> scale_Y >> scale_Z >>
                            rotation_Start >> rotation_End >> material_Id) {
            switch(material_Id) {
                case 0: {
                    material = &data.house.materials[14];
                    break;
                }
                case 1: {
                    material = &data.house.materials[0];
                    break;
                }
            }
            data.doors.push_back(Game_Data::Door_Data(Vector3 {position_X, position_Y, position_Z},
                                                      Vector3 {scale_X, scale_Y, scale_Z},
                                                      rotation_Start, rotation_End, material));
        }

        std::istringstream animation_File(LoadFileText("spawn_animation.txt"));
        float keyframe_X, keyframe_Y, keyframe_Z,
              target_X, target_Y, target_Z;


        while(animation_File >> keyframe_X >> keyframe_Y >> keyframe_Z >>
                                target_X >> target_Y >> target_Z) {
            data.wake_Animation.push_back(Game_Data::Directional_Position(Vector3 {keyframe_X, keyframe_Y, keyframe_Z},
                                                                   Vector3 {target_X, target_Y, target_Z}));
        }

        data.door = LoadModel("models/door.glb");
        for(int material = 0; material < data.door.materialCount; material++)
            data.door.materials[material].shader = data.lighting;

        data.spoon = LoadModel("models/spoon.glb");
        for(int material = 0; material < data.spoon.materialCount; material++)
            data.spoon.materials[material].shader = data.lighting;

        data.key = LoadModel("models/key.glb");
        for(int material = 0; material < data.key.materialCount; material++)
            data.key.materials[material].shader = data.lighting;

        data.drawer = LoadModel("models/drawer.glb");
        for(int material = 0; material < data.drawer.materialCount; material++)
            data.drawer.materials[material].shader = data.lighting;
        
        data.lock = LoadModel("models/lock.glb");
        for(int material = 0; material < data.lock.materialCount; material++)
            data.lock.materials[material].shader = data.lighting;

        data.drawer_BBox = GetModelBoundingBox(data.drawer);

        std::istringstream config_File(LoadFileText("config.txt"));

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
                } else {
                    LOG("Unknown variable or bad value %s", strings[0].c_str());
                }
            }
        }

        data.item_Data.push_back(Game_Data::Item_Data(0, {0.f, 0.f, 0.f})); //              NONE
        data.item_Data.push_back(Game_Data::Item_Data(1, B2RL(26.5f, -41.f, 8.f))); //      PRIBINACEK
        data.item_Data.push_back(Game_Data::Item_Data(2, B2RL(-26.5f, 10.f, 4.4f))); //     SPOON
        data.item_Data.push_back(Game_Data::Item_Data(3, {0.f, 0.f, 0.f})); //              KEY

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
        data.enemy_Direction = 1.f * GetFrameTime();

        for(int material = 0; material < Menu::data.pribinacek.materialCount; material++)
            Menu::data.pribinacek.materials[material].shader = data.lighting;

        int spoon_Position_Index = rand() % data.key_Spawns.size();
        data.item_Data[3] = Game_Data::Item_Data(3, data.key_Spawns[spoon_Position_Index].position, data.key_Spawns[spoon_Position_Index].rotation);

        for(Game_Data::Item_Data &item : data.item_Data) {
            item.fall_Acceleration = 0.1f;
            item.Calculate_Collision();
            item.falling = true;
        }

        data.guide_Texts.clear();
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Probudíš se uprostřed noci a máš nepžekonatelnou chuť na pribiňáčka"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Tvým cílem je jít pro pribiňáčka a sníst si ho tady v pokoji"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("Jo a táta vždycky na noc vypíná pojistky, aby jsem nemohl být na mobilu"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("...je trochu přehnaňe starostlivý"));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("1. najdi pribináček", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("2. najdi lžičku", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("3. vem pribináček a\nlžíci do pokoje\n na stůl", false));
        data.guide_Texts.push_back(Game_Data::Guide_Caption("4. dej si pribináček", false));

        data.guide_Finished = false;

        Mod_Callback("Switch_Game", (void*)&data);
    }

    void Update_Camera_Android(int touch_Id, bool update_Camera) {
        data.camera_Target = Vector3RotateByQuaternion({0.f, 0.f, 10.f}, QuaternionFromEuler(data.camera_Rotation.x * DEG2RAD, data.camera_Rotation.y * DEG2RAD, data.camera_Rotation.z * DEG2RAD));

        Vector2 delta = Vector2Subtract(data.old_Mouse_Position, GetTouchPosition(touch_Id));

        data.old_Mouse_Position = GetTouchPosition(touch_Id);

        if(!update_Camera)
            return;

        data.camera_Rotation = Vector3Add(data.camera_Rotation, {-delta.y * GetFrameTime() * 5.f, delta.x * GetFrameTime() * 5.f, 0.f});
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
                0.f, 0.f, 0.f                                                            // Rotation: roll
            },
            0.f);

        data.camera_Target = Vector3RotateByQuaternion({0.f, 0.f, 10.f}, QuaternionFromEuler(data.camera_Rotation.x * DEG2RAD, data.camera_Rotation.y * DEG2RAD, data.camera_Rotation.z * DEG2RAD));

        Vector2 delta = GetMouseDelta();
        data.camera_Rotation = Vector3Add(data.camera_Rotation, {delta.y * GetFrameTime() * 5.f, -delta.x * GetFrameTime() * 5.f, 0.f});
        data.camera_Rotation.x = Clamp(data.camera_Rotation.x, -85.f, 85.f);
    }

    void Update() {
        ClearBackground(BLACK);
        SetShaderValue(data.lighting, data.lighting.locs[SHADER_LOC_VECTOR_VIEW], &data.camera.position.x, SHADER_UNIFORM_VEC3);

        data.flashlight.position = data.camera.position;
        UpdateLightValues(data.lighting, data.flashlight);

        Vector3 old_Position = data.camera.position;
        int fogDensityLoc = GetShaderLocation(data.lighting, "fogDensity");
        SetShaderValue(data.lighting, fogDensityLoc, &data.fog_Density, SHADER_UNIFORM_FLOAT);

        Vector3 enemy_Position;
        bool action_Used = false; // If any action was used this frame (preventing click-through)

        BeginMode3D(data.camera); {
            Mod_Callback("Update_Game_3D", (void*)&data);

            data.frame_Counter++;
            if(data.frame_Counter > 100) data.frame_Counter = 0;

	        UpdateModelAnimation(data.father, data.animations[0], data.animation_Frame_Count);

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

            /* DOORS */ {
                bool door_Opened = false;
                for(Game_Data::Door_Data &door_Data : data.doors) {
                    // DrawModelEx(door, door_Data.position, {0.f, 1.f, 0.f}, door_Data.rotation, door_Data.scale, WHITE);
                    // DrawLine3D(data.camera.position, door_Data.position, RED);

                    float rotation_Player = door_Data.rotation;
                    float rotation_Father = door_Data.rotation_Father;

                    float rotation = 0.f;
                    if(door_Data.default_Rotation < door_Data.opened_Rotation) {
                        rotation = rotation_Player > rotation_Father ? rotation_Player : rotation_Father;
                    } else {
                        rotation = rotation_Player < rotation_Father ? rotation_Player : rotation_Father;
                    }

                    Matrix matrix = MatrixIdentity();
                    matrix = MatrixMultiply(matrix, MatrixScale(door_Data.scale.x, door_Data.scale.y, door_Data.scale.z));

                    matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.scale.x, 0.f, 0.f));
                    matrix = MatrixMultiply(matrix, MatrixRotateY(rotation * DEG2RAD));
                    matrix = MatrixMultiply(matrix, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

                    matrix = MatrixMultiply(matrix, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));

                    DrawMesh(data.door.meshes[0], *door_Data.material, matrix);
                    
                    Matrix matrixDoorHandle = MatrixIdentity();
                    matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(0.75f, 0.75f, 0.75f));

                    matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.scale.x + door_Data.scale.x / 2.f, door_Data.scale.y / 4.f, -door_Data.scale.z));
                    matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixRotateY(rotation * DEG2RAD));
                    matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

                    matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));
                    DrawMesh(data.doorHandle, *door_Data.material, matrixDoorHandle);

                    Ray ray = GetMouseRay({(float)GetScreenWidth() / 2.f, (float)GetScreenHeight() / 2.f}, data.camera);
                    RayCollision collision = GetRayCollisionMesh(ray, data.door.meshes[0], matrix);

                    if(collision.hit && collision.distance < 10.f && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !door_Opened && !action_Used) {
                        door_Data.opening = !door_Data.opening;
                        door_Opened = true;
                        action_Used = true;
                    }
                    
                    if(door_Data.default_Rotation < door_Data.opened_Rotation) {
                        if(door_Data.opening) {
                            if(door_Data.rotation < door_Data.opened_Rotation) {
                                door_Data.rotation += 100.f * GetFrameTime();
                            }
                        } else {
                            if(door_Data.rotation > door_Data.default_Rotation) {
                                door_Data.rotation -= 100.f * GetFrameTime();
                            }
                        }

                        if(door_Data.opening_Father) {
                            if(door_Data.rotation_Father < door_Data.opened_Rotation) {
                                door_Data.rotation_Father += 100.f * GetFrameTime();
                            }
                        } else {
                            if(door_Data.rotation_Father > door_Data.default_Rotation) {
                                door_Data.rotation_Father -= 100.f * GetFrameTime();
                            }
                        }
                    } else {
                        if(door_Data.opening) {
                            if(door_Data.rotation > door_Data.opened_Rotation) {
                                door_Data.rotation -= 100.f * GetFrameTime();
                            }
                        } else {
                            if(door_Data.rotation < door_Data.default_Rotation) {
                                door_Data.rotation += 100.f * GetFrameTime();
                            }
                        }

                        if(door_Data.opening_Father) {
                            if(door_Data.rotation_Father > door_Data.opened_Rotation) {
                                door_Data.rotation_Father -= 100.f * GetFrameTime();
                            }
                        } else {
                            if(door_Data.rotation_Father < door_Data.default_Rotation) {
                                door_Data.rotation_Father += 100.f * GetFrameTime();
                            }
                        }
                    }
                }
            }

            DrawModel(data.house, {0.f, 0.f, 0.f}, 1.f, WHITE);

            /* ITEMS */ {
                Ray player_Ray = GetMouseRay({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, data.camera);
                Mesh *bbox_Mesh = nullptr;
                
                for(int index = 0; index < data.item_Data.size(); index++) {
                    switch(index) {
                        case Game_Data::PRIBINACEK: {
                            Menu::data.pribinacek.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                                data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                                data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(Menu::data.pribinacek, data.item_Data[index].position, 1.f, WHITE);
                            bbox_Mesh = &Menu::data.pribinacek.meshes[1];
                            break;
                        }

                        case Game_Data::SPOON: {
                            data.spoon.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                     data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                     data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.spoon, data.item_Data[index].position, 1.f, WHITE);
                            bbox_Mesh = &data.spoon.meshes[0];
                            break;
                        }

                        case Game_Data::KEY: {
                            data.key.transform = MatrixMultiply(MatrixIdentity(), MatrixRotateXYZ({data.item_Data[index].rotation.x * DEG2RAD,
                                                                                                   data.item_Data[index].rotation.y * DEG2RAD,
                                                                                                   data.item_Data[index].rotation.z * DEG2RAD}));
                            DrawModel(data.key, data.item_Data[index].position, 1.f, WHITE);
                            bbox_Mesh = &data.key.meshes[0];
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

                        if(bbox_Mesh != nullptr) {
                            BoundingBox bbox = {{-1.f, -1.f, -1.f}, {1.f, 1.f, 1.f}};
                            bbox.min = Vector3Add(bbox.min, data.item_Data[index].position);
                            bbox.max = Vector3Add(bbox.max, data.item_Data[index].position);
                            
                            if(data.debug) DrawBoundingBox(bbox, RED);

                            RayCollision player_Item_Collision = GetRayCollisionBox(player_Ray, bbox);

                            bool item_Visible = player_Item_Collision.hit && player_Item_Collision.distance < 5.f;

                            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_Q)) {
                                RayCollision player_Map_Collision = Get_Collision_Ray(player_Ray);

                                if(data.holding_Item == (Game_Data::Item)index && !action_Used && IsKeyPressed(KEY_Q)) {
                                    data.item_Data[index].fall_Acceleration = 0.1f;
                                    data.item_Data[index].Calculate_Collision();
                                    data.item_Data[index].falling = true;
                                    data.holding_Item = Game_Data::NONE;
                                    action_Used = true;
                                } else if(item_Visible && !action_Used && data.holding_Item == Game_Data::NONE &&
                                          player_Map_Collision.distance > player_Item_Collision.distance && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                    data.holding_Item = (Game_Data::Item)index;
                                    action_Used = true;

                                    if((Game_Data::Item)index == Game_Data::PRIBINACEK && data.guide_Index == 4)
                                        data.guide_Index++;

                                    if((Game_Data::Item)index == Game_Data::SPOON && data.guide_Index == 5)
                                        data.guide_Index++;
                                }
                            }
                        }
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

                Ray ray = GetMouseRay({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f}, data.camera);
                BoundingBox bbox = data.drawer_BBox;
                bbox.min = Vector3Add(bbox.min, drawer.position);
                bbox.max = Vector3Add(bbox.max, drawer.position);
                RayCollision collision = GetRayCollisionBox(ray, bbox);

                if(nearest_Hit > collision.distance && collision.hit) {
                    nearest_Hit = collision.distance;
                    nearest_Hit_Id = index;
                }

                index++;
            }

            if(nearest_Hit < 5.f && nearest_Hit_Id != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !action_Used) {
                if(data.drawers[nearest_Hit_Id].has_Lock) {
                    if(data.holding_Item == Game_Data::KEY) {
                        action_Used = true;
                        data.drawers[nearest_Hit_Id].opening = !data.drawers[nearest_Hit_Id].opening;
                    }
                } else {
                    action_Used = true;
                    data.drawers[nearest_Hit_Id].opening = !data.drawers[nearest_Hit_Id].opening;
                }
            }

            if(data.debug) data.fog_Density += GetMouseWheelMove() / 100.f;
            if(IsKeyPressed(KEY_LEFT_CONTROL)) data.crouching = !data.crouching;

            if(CheckCollisionBoxSphere(data.players_Room_Table, data.item_Data[Game_Data::PRIBINACEK].position, 0.1f) &&
               CheckCollisionBoxSphere(data.players_Room_Table, data.item_Data[Game_Data::SPOON].position, 0.1f) && data.guide_Index == 6 &&
               data.holding_Item != Game_Data::PRIBINACEK && data.holding_Item != Game_Data::SPOON) {
                data.guide_Index++;
            }

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

                    Vector3 previous_Point = {0.f, 0.f, 0.f};
                    int index = 0;
                    for(Game_Data::Father_Point point : data.father_Points) {
                        if(index > 0) {
                            DrawLine3D(previous_Point, point.position, RED);
                        }
                        DrawSphere(point.position, .5f, BLUE);
                        previous_Point = point.position;
                        index++;
                    }
                } */

                Vector3 source = data.father_Points[data.keyframe].position;
                Vector3 target = data.father_Points[(data.keyframe + 1) % data.father_Points.size()].position;

                int index = 0;
                bool changing_Door_State = false;
                for(bool state : data.father_Points[(data.keyframe + 1) % data.father_Points.size()].door_States) {
                    if(data.doors[index].opening_Father != state)
                        changing_Door_State = true;
                    data.doors[index].opening_Father = state;
                    index++;
                }

                float max = Vector3Distance(source, target) / 4.f;
                data.keyframe_Tick += 1.f * GetFrameTime();

                Ray ray = {{Remap(data.keyframe_Tick, 0.f, max, source.x, target.x),
                            Remap(data.keyframe_Tick, 0.f, max, source.y, target.y),
                            Remap(data.keyframe_Tick, 0.f, max, source.z, target.z)}, {0.f, -0.1f, 0.f}};

                float y = Get_Collision_Ray(ray).point.y;

                enemy_Position = {Remap(data.keyframe_Tick, 0.f, max, source.x, target.x),
                                y,
                                Remap(data.keyframe_Tick, 0.f, max, source.z, target.z)};

                Vector3 difference = Vector3Subtract(target, source);
                float angle = -atan2(difference.z, difference.x) * RAD2DEG;
                float angle_Result = angle;

                float angle_Source = angle;
                if(data.keyframe > 0) {
                    Vector3 difference_Source = Vector3Subtract(source, data.father_Points[data.keyframe - 1].position);
                    angle_Source = -atan2(difference_Source.z, difference_Source.x) * RAD2DEG;

                    float lerp = Clamp(data.keyframe_Tick * 2.f, 0.f, 1.f);
                    float addition = (((((int)angle - (int)angle_Source) % 360) + 540) % 360) - 180;

                    angle_Result = angle_Source + addition * lerp;
                }
                
                DrawModelEx(data.father, enemy_Position, {0.f, 1.f, 0.f}, angle_Result + 90.f, Vector3 {12.f, 12.f, 12.f}, WHITE);

                if(data.keyframe_Tick > max) {
                    data.keyframe_Tick = 0.f;
                    data.keyframe++;
                }

                if(data.keyframe > data.father_Points.size() - 1) {
                    data.keyframe = 0;
                }
            }
        } EndMode3D();

        Mod_Callback("Update_Game_2D", (void*)&data, false);

        if(IsKeyPressed(KEY_P)) data.win.Play();
        data.win.Update();

        const float crosshair_size = 10.f;
        DrawLineEx({GetScreenWidth() / 2.f - crosshair_size, GetScreenHeight() / 2.f}, {GetScreenWidth() / 2.f + crosshair_size, GetScreenHeight() / 2.f}, 2.f, Fade(WHITE, 0.2f));
        DrawLineEx({GetScreenWidth() / 2.f, GetScreenHeight() / 2.f - crosshair_size}, {GetScreenWidth() / 2.f, GetScreenHeight() / 2.f + crosshair_size}, 2.f, Fade(WHITE, 0.2f));

#ifdef ANDROID_UI
        if(data.wake_Animation_Finished) {
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

            // TODO: Předělat celej systém dotyků pomocí gestures
            for(int id = 0; id < GetTouchPointCount(); id++) {
                bool can_Update = data.movement.Can_Update(id);
                Game_Data::Joystick_Data joystick = data.movement.Update(id);

                if(CheckCollisionPointRec(GetTouchPosition(id), crouch_Rectangle)) {
                    data.crouching = !data.crouching;
                } else if (can_Update) {
                    // Basically, a very complex and buggy way to slow down the player if he is crouching
                    if ((data.crouching && data.frame_Counter % 2 == 0) || !data.crouching) {
                        Update_Camera_Android(id, data.previous_Rotated);
                    }
                    rotation_Updated = true;
                } else {
                    if(joystick.moving) {
                        float camera_Angle = atan2(data.camera_Target.z, data.camera_Target.x) + 90.f * DEG2RAD;

                        Vector3 offset = {
                                cos(joystick.rotation * DEG2RAD + camera_Angle) * GetFrameTime() * 5.f, 0.f,
                                sin(joystick.rotation * DEG2RAD + camera_Angle) * GetFrameTime() * 5.f};
                        data.camera.position = Vector3Add(data.camera.position, offset);
                    }
                }
            }

            data.previous_Rotated = rotation_Updated;
        }
#else
        data.camera.target = Vector3Add(data.camera.position, data.camera_Target);
        if(data.wake_Animation_Finished) {
            float speed = 1.f;
            if(data.crouching) speed /= 1.5f;
            if(!data.crouching && IsKeyDown(KEY_LEFT_SHIFT)) speed *= 1.5f;

            Update_Camera_Desktop(speed);
        }
#endif

        /* COLLISIONS */ {
            if(Ray_Sides_Collision({old_Position.x, data.camera.position.y, data.camera.position.z}, old_Position))
                data.camera.position.z = old_Position.z;

            // (if debug) we dont want the spheres to be rendered twice
            bool is_Debug = data.debug;
            data.debug = true;
            if(Ray_Sides_Collision({data.camera.position.x, data.camera.position.y, old_Position.z}, old_Position))
                data.camera.position.x = old_Position.x;
            data.debug = is_Debug;

            RayCollision collision_Legs = {0};
            if(data.wake_Animation_Finished) {
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
                        // Do nothing there
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

        DrawFPS(GetScreenWidth() / 500.f, GetScreenWidth() / 500.f);

        Ray enemy_Ray = {enemy_Position, Vector3Divide(Vector3Normalize(Vector3Subtract(data.camera.position, enemy_Position)), {10.f, 10.f, 10.f})};
        float player_Distance = Vector3DistanceSqr(enemy_Position, data.camera.position);
        RayCollision scene_Collision = Get_Collision_Ray(enemy_Ray);

        bool player_Visible = player_Distance < scene_Collision.distance;

        if(player_Visible) {
            See_Player();
        }

        bool trigger_Chase = player_Visible;

        if(!data.guide_Finished && data.guide_Texts[data.guide_Index].Update()) {
            data.guide_Index++;
            if(data.guide_Index > 3) {
                data.guide_Finished = true;
            }
        }

        if(data.guide_Finished) {
            float margin = GetScreenWidth() / 50.f;
            Vector2 rectangle_Size = MeasureTextEx(Menu::data.medium_Font, data.guide_Texts[4].target_Text.c_str(), 30.f, 0.f);
            for(int i = 4 + 1; i < data.guide_Texts.size(); i++) {
                rectangle_Size.y += MeasureTextEx(Menu::data.medium_Font, data.guide_Texts[i].target_Text.c_str(), 30.f, 0.f).y;
            }

            Rectangle rectangle = {margin, margin, rectangle_Size.x, rectangle_Size.y};
            DrawRectangleRounded(rectangle, 0.1f, 20, Color {0, 0, 0, 180});
            DrawRectangleRoundedLinesEx(rectangle, 0.1f, 20, 5.f, BLACK);

            Vector2 text_Box_Size = MeasureTextEx(Menu::data.medium_Font, data.guide_Texts[4].target_Text.c_str(), 25.f, 0.f);
            for(int i = 4 + 1; i < data.guide_Texts.size(); i++) {
                text_Box_Size.y += MeasureTextEx(Menu::data.medium_Font, data.guide_Texts[i].target_Text.c_str(), 25.f, 0.f).y;
            }

            int y = 0;
            for(int i = 0; i < 4; i++) {
                const char* text = data.guide_Texts[4 + i].target_Text.c_str();
                Vector2 line = MeasureTextEx(Menu::data.medium_Font, text, 25.f, 0.f);
                DrawTextEx(Menu::data.medium_Font, text, {rectangle.x + rectangle.width / 2.f - text_Box_Size.x / 2.f, rectangle.y + rectangle.height / 2.f - text_Box_Size.y / 2.f + y}, 25.f, 0.f, (4 + i < data.guide_Index) ? GREEN : WHITE);
                y += line.y;
            }
        }

        Mod_Callback("Update_Game_2D", (void*)&data, true);
    }
};

#endif // GAME_CXX
