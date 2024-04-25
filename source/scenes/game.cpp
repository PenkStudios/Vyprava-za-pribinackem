#ifndef GAME_CXX
#define GAME_CXX

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

#include <iostream>
#include <fstream>

#define RLIGHTS_IMPLEMENTATION
#include "../rlights.h"

#include "../scene.cpp"

namespace Game {
    Shader lighting;
    Model house;
    Model father;
    Camera3D camera;
    Light flashlight;
    ModelAnimation *animations;
    float fall_Acceleration = 0.f;

    bool debug = false;
    float fog_Density = 0.15f;
    bool crouching = false;
    int frameCounter = 0;
    int animation_Frame_Count = 0;

    std::vector<Vector3> father_Points = {};
    float keyframe_Tick = 0.f;
    int keyframe = 0;

    void Init() {
        camera.position = {0.f, 7.5f, 0.f};
        camera.up = {0.f, 1.f, 0.f};
        camera.target = {0.f, 0.f, 10.f};
        camera.fovy = 90.f;
        camera.projection = CAMERA_PERSPECTIVE;

        house = LoadModel("models/house.glb");
        DisableCursor();

        father = LoadModel("models/human.m3d");

        Texture human = LoadTexture("textures/human.png");
        std::cout << father.materialCount << std::endl;
        SetMaterialTexture(&father.materials[1], MATERIAL_MAP_DIFFUSE, human);

        int animation_Count = 0; // (1)
        animations = LoadModelAnimations("models/human.m3d", &animation_Count);

        lighting = LoadShader("shaders/vertex.glsl", "shaders/fragment.glsl");

        lighting.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lighting, "matModel");
        lighting.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lighting, "viewPos");

        int ambientLoc = GetShaderLocation(lighting, "ambient");
        float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
        SetShaderValue(lighting, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

        for(int material = 0; material < father.materialCount; material++)
            father.materials[material].shader = lighting;

        for(int material = 0; material < house.materialCount; material++)
            house.materials[material].shader = lighting;

        flashlight = CreateLight(LIGHT_POINT, camera.position, {0.f, 0.f, 0.f}, WHITE, lighting);
    
        std::ifstream ai_File("ai.txt", std::ios::in);
        float x, y, z;
        while(ai_File >> x >> y >> z) {
            father_Points.push_back(Vector3 {x, y, z});
        }
        ai_File.close();
    }

    RayCollision GetCollisionRay(Ray ray) {
        RayCollision collision = { 0 };
        collision.distance = 1000000.f;
        for (int m = 0; m < house.meshCount; m++)
        {
            // NOTE: We consider the model.transform for the collision check but 
            // it can be checked against any transform Matrix, used when checking against same
            // model drawn multiple times with multiple transforms
            RayCollision houseCollision = GetRayCollisionMesh(ray, house.meshes[m], house.transform);
            if (houseCollision.hit)
            {
                if(houseCollision.distance < collision.distance) {
                    collision = houseCollision;
                }
            }
        }

        return collision;
    }

    bool RaySideCollision(Ray ray, Vector3 old_Position) {
        RayCollision collision = GetCollisionRay(ray);
        if(collision.hit && collision.distance < 1.f) {
            return true;
        }

        if(debug) {
            DrawLine3D(Vector3Add(camera.position, {0.f, -1.75f, 0.f}), collision.point, RED);
            DrawSphere(Vector3Add(camera.position, {0.f, -1.75f, 0.f}), 0.5f, RED);
            DrawSphere(collision.point, 0.5f, BLUE);
        }

        return false;
    }

    bool RaySidesCollision(Vector3 camera_Position, Vector3 old_Position) {
        bool collision_1 = RaySideCollision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.f, 0.f, 0.1f}}, old_Position);
        bool collision_2 = RaySideCollision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.f, 0.f, -0.1f}}, old_Position);
        bool collision_3 = RaySideCollision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {0.1f, 0.f, 0.f}}, old_Position);
        bool collision_4 = RaySideCollision({Vector3Add(camera_Position, {0.f, -0.75f, 0.f}), {-0.1f, 0.f, 0.f}}, old_Position);
    
        return collision_1 || collision_2 || collision_3 || collision_4;
    }

    void Update() {
        ClearBackground(BLACK);
        SetShaderValue(lighting, lighting.locs[SHADER_LOC_VECTOR_VIEW], &camera.position.x, SHADER_UNIFORM_VEC3);

        flashlight.position = camera.position;
        UpdateLightValues(lighting, flashlight);

        Vector3 old_Position = camera.position;
        int fogDensityLoc = GetShaderLocation(lighting, "fogDensity");
        SetShaderValue(lighting, fogDensityLoc, &fog_Density, SHADER_UNIFORM_FLOAT);

        BeginMode3D(camera); {
            frameCounter++;
            if(frameCounter > 100) frameCounter = 0;

            UpdateModelAnimation(father, animations[0], animation_Frame_Count);
            animation_Frame_Count++;
            if(animation_Frame_Count >= animations[0].frameCount)
                animation_Frame_Count = 0;

            // Basically, a very complex and buggy way to slow down the player if he is crouching
            if((crouching && frameCounter % 2 == 0) || !crouching) {
                UpdateCamera(&camera, CAMERA_FIRST_PERSON);
            } else {
                UpdateCamera(&camera, CAMERA_FREE);
                camera.position = old_Position;
            }

            if(RaySidesCollision({old_Position.x, camera.position.y, camera.position.z}, old_Position))
                camera.position.z = old_Position.z;

            if(RaySidesCollision({camera.position.x, camera.position.y, old_Position.z}, old_Position))
                camera.position.x = old_Position.x;

            DrawModel(house, {0.f, 0.f, 0.f}, 1.f, WHITE);

            if(debug) fog_Density += GetMouseWheelMove() / 100.f;
            if(IsKeyPressed(KEY_LEFT_SHIFT)) crouching = !crouching;

            Vector3 source = father_Points[keyframe];
            Vector3 target = father_Points[(keyframe + 1) % father_Points.size()];

            float max = Vector3Distance(source, target) / 4.f;
            keyframe_Tick += 0.01f;

            Ray ray = {{Remap(keyframe_Tick, 0.f, max, source.x, target.x),
                        Remap(keyframe_Tick, 0.f, max, source.y, target.y),
                        Remap(keyframe_Tick, 0.f, max, source.z, target.z)}, {0.f, -0.1f, 0.f}};

            float y = GetCollisionRay(ray).point.y;

            Vector3 current = {Remap(keyframe_Tick, 0.f, max, source.x, target.x),
                               y,
                               Remap(keyframe_Tick, 0.f, max, source.z, target.z)};

            Vector3 difference = Vector3Subtract(target, source);
            float angle = -atan2(difference.z, difference.x) * RAD2DEG;
            float angle_Result = angle;

            float angle_Source = angle;
            if(keyframe > 0) {
                Vector3 difference_Source = Vector3Subtract(source, father_Points[keyframe - 1]);
                angle_Source = -atan2(difference_Source.z, difference_Source.x) * RAD2DEG;

                float lerp = Clamp(keyframe_Tick * 2.f, 0.f, 1.f);
                float addition = (((((int)angle - (int)angle_Source) % 360) + 540) % 360) - 180;

                angle_Result = angle_Source + addition * lerp;
            }
            
            DrawModelEx(father, current, {0.f, 1.f, 0.f}, angle_Result + 90.f, Vector3 {10.f, 10.f, 10.f}, WHITE);

            if(keyframe_Tick > max) {
                keyframe_Tick = 0.f;
                keyframe++;
            }

            if(keyframe > father_Points.size() - 1) {
                keyframe = 0;
            }
        } EndMode3D();
        
        Ray bottom = {Vector3Add(camera.position, {0.f, -1.75f, 0.f}), {0.f, -0.1f, 0.f}};
        RayCollision collision_Legs = GetCollisionRay(bottom);
        if(collision_Legs.hit) {
            Vector3 target = Vector3Add(collision_Legs.point, {0.f, crouching ? 5.0f : 6.5f, 0.f});
            if(camera.position.y < target.y) {
                camera.position = target;
                fall_Acceleration = 0.1f;
            } else if(camera.position.y < target.y + 0.1f && camera.position.y > target.y - 0.1f) {
                // Do nothing there
                fall_Acceleration = 0.1f;
            } else {
                camera.position.y -= fall_Acceleration;
                fall_Acceleration += 0.01f;
            }
        } else
            camera.position = old_Position;

        if(debug)
            DrawText(TextFormat("Legs raycast position: {%f, %f, %f}, angled surface: %d, %f", collision_Legs.point.x, collision_Legs.point.y, collision_Legs.point.z,
                                                                                             collision_Legs.normal.y < 0.99f || collision_Legs.normal.y > 1.01, fog_Density), 5, 5, 15, WHITE);
    }
};

#endif // GAME_CXX