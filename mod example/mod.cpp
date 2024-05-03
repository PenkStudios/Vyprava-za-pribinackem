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
// Internal initialization
MOD_API void Init(void*) {}


// Intro initialization with the context of intro
MOD_API void Init_Intro(Intro::Intro_Data* context) {}

// Intro updation with the context of intro and the "in_Front" variable
MOD_API void Update_Intro(Intro::Intro_Data* context, bool in_Front) {}


// Menu initialization with the context of menu
MOD_API void Init_Menu(Menu::Menu_Data* context) {}

// Menu updation with the context of menu and the "in_Front" variable
MOD_API void Update_Menu(Menu::Menu_Data* context, bool in_Front) {}


// Game initialization with the context of game
MOD_API void Init_Game(Game::Game_Data* context) {}

// Game 3d updation with the context of game
MOD_API void Update_Game(Game::Game_Data* context) {}

// Game ui updation with the context of game and the "in_Front" variable
MOD_API void Update_Game_UI(Game::Game_Data* context, bool in_Front) {}
*/

}