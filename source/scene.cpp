#ifndef SCENE_CXX
#define SCENE_CXX

#include <map>
#include <functional>

enum Scene {INTRO, MENU, GAME, CREDITS};
Scene scene;

class SceneInfo {
public:
    std::function<void(void)> init;
    std::function<void(void)> on_Switch;
    std::function<void(void)> update;
};

std::map<Scene, SceneInfo> scenes;

void Set_Scene_Data(std::map<Scene, SceneInfo> data) {
    scenes = data;
}


void Init_Scenes() {
    for(auto const& [key, info] : scenes)  {
        info.init();
    }
}

void Update_Scene() {
    scenes[scene].update();
}


void Switch_To_Scene(Scene next_Scene) {
    scene = next_Scene;
    scenes[scene].on_Switch();
}

Scene &Get_Scene() {
    return scene;
}

class Base_Data {};

#endif // SCENE_CXX