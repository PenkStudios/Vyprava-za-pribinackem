#include <raylib.h>
#include <iostream>

#include "../source/scenes/intro.cpp"
#include "../source/scenes/menu.cpp"
#include "../source/scenes/game.cpp"

#ifdef __WIN32__
#define MOD_API __declspec(dllexport)
#else
#define MOD_API
#endif

extern "C" {

MOD_API void Init(void*) {}

MOD_API void Update_Intro(Intro::Intro_Data* context) {}

MOD_API void Update_Menu(Menu::Menu_Data* context) {}

MOD_API void Update_Game(Game::Game_Data* context) { }
MOD_API void Update_Game_UI(Game::Game_Data* context) {}

}