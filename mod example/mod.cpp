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

/*
MOD_API void Init(Intro::Intro_Data* context) {}

MOD_API void Init_Intro(Intro::Intro_Data* context) {}
MOD_API void Update_Intro(Intro::Intro_Data* context, bool in_Front) {}

MOD_API void Init_Menu(Menu::Menu_Data* context) {}
MOD_API void Update_Menu(Menu::Menu_Data* context, bool in_Front) {}

MOD_API void Init_Game(Game::Game_Data* context) {}
MOD_API void Update_Game(Game::Game_Data* context) {}
MOD_API void Update_Game_2D(Game::Game_Data* context, bool in_Front) {}
*/

}