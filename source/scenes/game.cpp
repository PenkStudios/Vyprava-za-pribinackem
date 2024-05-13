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

#define MINIAUDIO_IMPLEMENTATION
#include "../miniaudio.h"

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

            float rotation; // Rotation caused by player
            float rotation_Father; // Rotation caused by daddy

            // -||-
            bool opening;
            bool opening_Father;
            Door_Data(Vector3 door_Position, Vector3 door_Scale, float door_Default_Rotation, float door_Opened_Rotation) :
                position(door_Position), scale(door_Scale), default_Rotation(door_Default_Rotation),
                opened_Rotation(door_Opened_Rotation), rotation(door_Default_Rotation),
                rotation_Father(door_Default_Rotation), opening(false) {}
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
        int animation_Frame_Count = 0;

        std::vector<Father_Point> father_Points = {};
        float keyframe_Tick = 0.f;
        int keyframe = 0;

        class Wake_Keyframe {
        public:
            Vector3 position;
            Vector3 target;

            Wake_Keyframe(Vector3 position, Vector3 target) : position(position), target(target) {}
        };
        std::vector<Wake_Keyframe> wake_Animation = {};
        float wake_Animation_Tick = 0.f;
        bool wake_Animation_Finished = false;

        Texture joystick_Base;
        Texture joystick_Pointer;

        Texture crouch;
        Texture un_Crouch;

        Vector3 camera_Rotation = {0.f, 0.f, 0.f};
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

        ma_device_config deviceConfig;
        ma_device device;

        ma_context context;

        ma_device_info *pCaptureInfos;
        ma_uint32 captureCount;

        int microphone_Id = 0;
        int microphone_Change_Cooldown;

        float max_Audio = 0.f;
        bool searching_Microphone = true;
    } data;

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

    }

    void Audio_Data_Callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        for(int frame = 0; frame < frameCount; frame++) {
            float audio = ((float*)pInput)[frame];

            if(audio > data.max_Audio) {
                data.max_Audio = audio;
            }
            
            if(audio > 0.999f) {
                See_Player();
            }
        }

        (void)pOutput;
    }

    void Init() {
        data.camera.position = {-10.f, 20.f, 23.5f}; // {0.f, 7.5f, 0.f};
        data.camera.up = {0.f, 1.f, 0.f};
        data.camera.target = {0.f, 20.f, 23.5f};
        data.camera.fovy = 90.f;
        data.camera.projection = CAMERA_PERSPECTIVE;

        if (ma_context_init(NULL, 0, NULL, &data.context) != MA_SUCCESS) {
            LOG("Failed to initialize context");
        }

        ma_device_info *pPlaybackInfos;
        ma_uint32 playbackCount;
        ma_device_uninit(&data.device);
        if (ma_context_get_devices(&data.context, &pPlaybackInfos, &playbackCount, &data.pCaptureInfos, &data.captureCount) != MA_SUCCESS)
        {
            LOG("Cannot list devices\n");
        }

        for(int index = 0; index < data.captureCount; index++)
            std::cout << data.pCaptureInfos[index].name << std::endl;

        ma_result result;

        data.deviceConfig = ma_device_config_init(ma_device_type_capture);
        data.deviceConfig.capture.pDeviceID = &data.pCaptureInfos[0].id;
        data.deviceConfig.capture.format    = ma_format_f32;
        data.deviceConfig.capture.channels  = 2;
        data.deviceConfig.sampleRate        = 0;
        data.deviceConfig.dataCallback      = Audio_Data_Callback;
        data.deviceConfig.pUserData         = nullptr;

        result = ma_device_init(NULL, &data.deviceConfig, &data.device);
        if (result != MA_SUCCESS) {
            LOG("Failed to initialize capture device.\n");
        }

        result = ma_device_start(&data.device);
        if (result != MA_SUCCESS) {
            ma_device_uninit(&data.device);
            LOG("Failed to start device.\n");
        }

        // ma_device_uninit(&data.device);

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

        while(doors_File >> position_X >> position_Y >> position_Z >>
                            scale_X >> scale_Y >> scale_Z >>
                            rotation_Start >> rotation_End) {
            data.doors.push_back(Game_Data::Door_Data(Vector3 {position_X, position_Y, position_Z},
                                      Vector3 {scale_X, scale_Y, scale_Z},
                                              rotation_Start, rotation_End));
        }

        std::istringstream animation_File(LoadFileText("spawn_animation.txt"));
        float keyframe_X, keyframe_Y, keyframe_Z,
              target_X, target_Y, target_Z;


        while(animation_File >> keyframe_X >> keyframe_Y >> keyframe_Z >>
                                target_X >> target_Y >> target_Z) {
            data.wake_Animation.push_back(Game_Data::Wake_Keyframe(Vector3 {keyframe_X, keyframe_Y, keyframe_Z},
                                                                   Vector3 {keyframe_X + target_X, keyframe_Y + target_Y, keyframe_Z + target_Z}));
        }

        data.door = LoadModel("models/door.glb");
        for(int material = 0; material < data.door.materialCount; material++)
            data.door.materials[material].shader = data.lighting;

        Mod_Callback("Init_Game", (void*)&data);
    }

    // Get map collision with ray (map + doors)
    RayCollision Get_Collision_Ray(Ray ray) {
        RayCollision collision = { 0 };
        collision.distance = 1000000.f;
        for (int m = 0; m < data.house.meshCount; m++)
        {
            // NOTE: We consider the model.transform for the collision check but 
            // it can be checked against any transform Matrix, used when checking against same
            // model drawn multiple times with multiple transforms
            RayCollision houseCollision = GetRayCollisionMesh(ray, data.house.meshes[m], data.house.transform);
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

        return collision;
    }

    bool Ray_Side_Collision(Ray ray, Vector3 old_Position) {
        RayCollision collision = Get_Collision_Ray(ray);
        if(collision.hit && collision.distance < 1.f) {
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
        Mod_Callback("Switch_Game", (void*)&data);
    }

    void Update_Camera_Android(int touch_Id, bool update_Camera) {
        Vector3 target = Vector3RotateByQuaternion({0.f, 0.f, 10.f}, QuaternionFromEuler(data.camera_Rotation.x * DEG2RAD, data.camera_Rotation.y * DEG2RAD, data.camera_Rotation.z * DEG2RAD));
        data.camera.target = Vector3Add(data.camera.position, target);

        Vector2 delta = Vector2Subtract(data.old_Mouse_Position, GetTouchPosition(touch_Id));

        data.old_Mouse_Position = GetTouchPosition(touch_Id);

        if(!update_Camera)
            return;

        data.camera_Rotation = Vector3Add(data.camera_Rotation, {-delta.y * GetFrameTime() * 10.f, delta.x * GetFrameTime() * 10.f, 0.f});
        data.camera_Rotation.x = Clamp(data.camera_Rotation.x, -75.f, 75.f);
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

        BeginMode3D(data.camera); {
            Mod_Callback("Update_Game_3D", (void*)&data);

            data.frame_Counter++;
            if(data.frame_Counter > 100) data.frame_Counter = 0;

	        UpdateModelAnimation(data.father, data.animations[0], data.animation_Frame_Count);


            data.animation_Frame_Count++;
            if(data.animation_Frame_Count >= data.animations[0].frameCount)
                data.animation_Frame_Count = 0;


            if(data.debug) {
                for(int index = 0; index < data.wake_Animation.size(); index++) {
                    DrawCube(data.wake_Animation[index].position, 1.f, 1.f, 1.f, RED);
                    DrawLine3D(data.wake_Animation[index].position, data.wake_Animation[index].target, WHITE);
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

                data.wake_Animation_Tick += 1.f * GetFrameTime();
                if(data.wake_Animation_Tick >= data.wake_Animation.size() - 1) {
                    data.wake_Animation_Finished = true;
                }
                
                data.camera.position = current_Position;
                data.camera.target = current_Target;
            }


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

                DrawMesh(data.door.meshes[0], data.house.materials[16], matrix);
                
                Matrix matrixDoorHandle = MatrixIdentity();
                matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixScale(0.75f, 0.75f, 0.75f));

                matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.scale.x + door_Data.scale.x / 2.f, door_Data.scale.y / 4.f, -door_Data.scale.z));
                matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixRotateY(rotation * DEG2RAD));
                matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(-door_Data.scale.x, 0.f, 0.f));

                matrixDoorHandle = MatrixMultiply(matrixDoorHandle, MatrixTranslate(door_Data.position.x, door_Data.position.y, door_Data.position.z));
                DrawMesh(data.doorHandle, data.house.materials[16], matrixDoorHandle);

                Ray ray = GetMouseRay({(float)GetScreenWidth() / 2.f, (float)GetScreenHeight() / 2.f}, data.camera);
                RayCollision collision = GetRayCollisionMesh(ray, data.door.meshes[0], matrix);


                if(collision.hit && collision.distance < 10.f && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !door_Opened) {
                    door_Data.opening = !door_Data.opening;
                    door_Opened = true;
                }
                
                if(door_Data.default_Rotation < door_Data.opened_Rotation) {
                    if(door_Data.opening) {
                        if(door_Data.rotation < door_Data.opened_Rotation) {
                            door_Data.rotation += 1.f;
                        }
                    } else {
                        if(door_Data.rotation > door_Data.default_Rotation) {
                            door_Data.rotation -= 1.f;
                        }
                    }

                    if(door_Data.opening_Father) {
                        if(door_Data.rotation_Father < door_Data.opened_Rotation) {
                            door_Data.rotation_Father += 1.f;
                        }
                    } else {
                        if(door_Data.rotation_Father > door_Data.default_Rotation) {
                            door_Data.rotation_Father -= 1.f;
                        }
                    }
                } else {
                    if(door_Data.opening) {
                        if(door_Data.rotation > door_Data.opened_Rotation) {
                            door_Data.rotation -= 1.f;
                        }
                    } else {
                        if(door_Data.rotation < door_Data.default_Rotation) {
                            door_Data.rotation += 1.f;
                        }
                    }

                    if(door_Data.opening_Father) {
                        if(door_Data.rotation_Father > door_Data.opened_Rotation) {
                            door_Data.rotation_Father -= 1.f;
                        }
                    } else {
                        if(door_Data.rotation_Father < door_Data.default_Rotation) {
                            door_Data.rotation_Father += 1.f;
                        }
                    }
                }
            }


            DrawModel(data.house, {0.f, 0.f, 0.f}, 1.f, WHITE);


            if(data.debug) data.fog_Density += GetMouseWheelMove() / 100.f;
            if(IsKeyPressed(KEY_LEFT_SHIFT)) data.crouching = !data.crouching;

            /* {
                if(IsKeyPressed(KEY_SPACE)) {
                    father_Points.push_back(Father_Point(camera.position, Get_Door_States()));
                } else if(IsKeyPressed(KEY_BACKSPACE)) {
                    father_Points.back() = Father_Point(camera.position, Get_Door_States());
                } else if(IsKeyPressed(KEY_ENTER)) {
                    for(Father_Point point : father_Points) {
                        std::string doors = "";
                        for(bool state : point.door_States) {
                            doors += " " + std::to_string(state);
                        }
               
                        std::cout << point.position.x << " " << point.position.y << " " << point.position.z << doors << std::endl;
                    }
                }

                Vector3 previous_Point = {0.f, 0.f, 0.f};
                int index = 0;
                for(Father_Point point : father_Points) {
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
        } EndMode3D();

        Mod_Callback("Update_Game_2D", (void*)&data, false);

#ifdef ANDROID_UI
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
                } else {
                    Update_Camera_Android(id, data.previous_Rotated);
                }
                rotation_Updated = true;
            } else {
                if(joystick.moving) {
                    Vector3 diff = Vector3Subtract(data.camera.target, data.camera.position);
                    float camera_Angle = atan2(diff.z, diff.x) + 90.f * DEG2RAD;

                    Vector3 offset = {
                            cos(joystick.rotation * DEG2RAD + camera_Angle) * GetFrameTime() * 5.f, 0.f,
                            sin(joystick.rotation * DEG2RAD + camera_Angle) * GetFrameTime() * 5.f};
                    data.camera.position = Vector3Add(data.camera.position, offset);
                    data.camera.target = Vector3Add(data.camera.target, offset);
                }
            }
        }

        data.previous_Rotated = rotation_Updated;
#else
        // Basically, a very complex and buggy way to slow down the player if he is crouching
        if ((data.crouching && data.frame_Counter % 2 == 0) || !data.crouching) {
            UpdateCamera(&data.camera, CAMERA_FIRST_PERSON);
        } else {
            UpdateCamera(&data.camera, CAMERA_FIRST_PERSON);
        }
#endif

        if(Ray_Sides_Collision({old_Position.x, data.camera.position.y, data.camera.position.z}, old_Position))
            data.camera.position.z = old_Position.z;

        // (if debug) we dont want the spheres to be rendered twice
        bool is_Debug = data.debug;
        data.debug = false;
        if(Ray_Sides_Collision({data.camera.position.x, data.camera.position.y, old_Position.z}, old_Position))
            data.camera.position.x = old_Position.x;
        data.debug = is_Debug;

        RayCollision collision_Legs = {0};
        if(data.wake_Animation_Finished) {
            Ray bottom = {Vector3Add(data.camera.position, {0.f, -1.75f, 0.f}), {0.f, -0.1f, 0.f}};
            collision_Legs = Get_Collision_Ray(bottom);
            if(collision_Legs.hit) {
                Vector3 target = Vector3Add(collision_Legs.point, {0.f, data.crouching ? 5.0f : 6.5f, 0.f});
                if(data.camera.position.y < target.y) {
                    data.camera.position = target;
                    data.fall_Acceleration = 0.1f;
                } else if(data.camera.position.y < target.y + 0.1f && data.camera.position.y > target.y - 0.1f) {
                    // Do nothing there
                    data.fall_Acceleration = 0.1f;
                } else {
                    data.camera.position.y -= data.fall_Acceleration;
                    data.fall_Acceleration += 0.01f;
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
        }

        DrawFPS(5, 5);

        Ray enemy_Ray = {enemy_Position, Vector3Divide(Vector3Normalize(Vector3Subtract(data.camera.position, enemy_Position)), {10.f, 10.f, 10.f})};
        float player_Distance = Vector3DistanceSqr(enemy_Position, data.camera.position);
        RayCollision scene_Collision = Get_Collision_Ray(enemy_Ray);

        bool player_Visible = player_Distance < scene_Collision.distance;

        if(player_Visible) {
            See_Player();
        }

        bool trigger_Chase = player_Visible;

        Mod_Callback("Update_Game_2D", (void*)&data, true);
    }
};

#endif // GAME_CXX
