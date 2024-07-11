#include <raylib.h>
#include <iostream>

#ifdef PLATFORM_ANDROID
#define ASSETS_ROOT ""
#else
#define ASSETS_ROOT "assets/"
#endif

#include "intro.cpp"
#include "menu.cpp"
#include "game.cpp"
#include "shared.cpp"

#include <vector>

#ifdef __WIN32__
#define MOD_API __declspec(dllexport)
#else
#define MOD_API
#endif

extern "C" {
Font *font;

MOD_API void Init(Shared::Shared_Data* context) {
	font = &context->medium_Font;
}

bool timer_Shown = false;
bool timer_Playing;
float time_Start;

#define MAX_SPLITS 4 // vzít pribináček, vzít klíč, vzít lžičku, konec
std::pair<std::string, float> splits[MAX_SPLITS];
int split_Pointer = 0;

float last_Time = 0.f;

MOD_API void Switch_Game(Game::Game_Data* context) {
	time_Start = GetTime();
	last_Time = GetTime();
	for(int split = 0; split < MAX_SPLITS; split++) splits[split] = std::make_pair("", -1.f);
	split_Pointer = 0;
	timer_Playing = true;
}

bool Splits_Contain(std::string string) {
	for(int split = 0; split < MAX_SPLITS; split++) {
		if(splits[split].first == string)
			return true;
	}
	return false;
}

void Splits_Append(std::string string) {
	splits[split_Pointer] = std::make_pair(string, GetTime() - last_Time);
	split_Pointer++;
	last_Time = GetTime();
}

MOD_API void Update_Game_2D(Game::Game_Data* context, bool in_Front) {
	if(in_Front) {
		if(IsKeyPressed(KEY_T)) timer_Shown = !timer_Shown;
		if(timer_Shown) {
			float margin = GetScreenWidth() / 50.f;
			float font_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 30.f;
            float font_Size_Smaller = (GetScreenWidth() + GetScreenHeight()) / 2.f / 35.f;
            float line_Size = (GetScreenWidth() + GetScreenHeight()) / 2.f / 300.f;

			Vector2 size = {GetScreenWidth() / 4.5f, GetScreenHeight() / 2.f};
			Rectangle timer_Rectangle = {GetScreenWidth() - size.x, 0.f, size.x, size.y};
			timer_Rectangle.x -= margin;
			timer_Rectangle.y += margin;
			DrawRectangleRounded(timer_Rectangle, 0.1f, 20, Color {20, 20, 20, 255});

			float split_Height = GetScreenHeight() / 12.f / 2.f;

			for(int split = 0; split < MAX_SPLITS; split++) {
				Rectangle rectangle = {timer_Rectangle.x, timer_Rectangle.y + split_Height * (split + 1), timer_Rectangle.width, split_Height};
				DrawRectangleRec(rectangle, split % 2 == 0 ? Color {17, 17, 17, 255} : Color {23, 23, 23, 255});

				if(!FloatEquals(splits[split].second, -1.f)) {
					const char* split_Text = TextFormat("%s %.2f", splits[split].first.c_str(), splits[split].second);
					Vector2 size = MeasureTextEx(*font, split_Text, font_Size_Smaller, 0.f);
					DrawTextEx(*font, split_Text, {rectangle.x + rectangle.width / 2.f - size.x / 2.f, rectangle.y + rectangle.height / 2.f - size.y / 2.f}, font_Size_Smaller, 0.f, WHITE);
				}
			}

			float time;

			if(timer_Playing) {
				time = GetTime() - time_Start;
			} else {
				float sum = 0.f;
				for(int split = 0; split < MAX_SPLITS; split++) { sum += splits[split].second; }
				time = sum;
			}

			Vector2 timer_Text_Size = MeasureTextEx(*font, TextFormat("%f", time), font_Size_Smaller, 0.f);
			Rectangle timer_Region = {timer_Rectangle.x, timer_Rectangle.y + split_Height * (MAX_SPLITS + 1), timer_Rectangle.width, timer_Rectangle.height - split_Height * (MAX_SPLITS + 1)};
			DrawTextEx(*font, TextFormat("%f", time), {timer_Region.x + timer_Region.width / 2.f - timer_Text_Size.x / 2.f, timer_Region.y + timer_Region.height / 2.f - timer_Text_Size.y / 2.f}, font_Size_Smaller, 0.f, timer_Playing ? GREEN : WHITE);
			
			DrawRectangleRounded(Rectangle {timer_Rectangle.x, timer_Rectangle.y, timer_Rectangle.width, timer_Rectangle.height / 12.f}, 0.2f, 20, BLUE);
			Vector2 text_Size = MeasureTextEx(*font, "Speedrun časovač", font_Size_Smaller, 0.f);
			DrawTextEx(*font, "Speedrun časovač", {timer_Rectangle.x + timer_Rectangle.width / 2.f - text_Size.x / 2.f, timer_Rectangle.y + timer_Rectangle.height / 12.f / 2.f - text_Size.y / 2.f}, font_Size_Smaller, 0.f, WHITE);
		}

		if(context->holding_Item == Game::Game_Data::Item::PRIBINACEK) {
			if(!Splits_Contain("Pribináček")) {
				Splits_Append("Pribináček");
			}
		} else if(context->holding_Item == Game::Game_Data::Item::SPOON) {
			if(!Splits_Contain("Lžíce")) {
				Splits_Append("Lžíce");
			}
		} else if(context->holding_Item == Game::Game_Data::Item::KEY) {
			if(!Splits_Contain("Klíč")) {
				Splits_Append("Klíč");
			}
		} else if(context->guide_Texts[7].done) {
			if(!Splits_Contain("Konec")) {
				Splits_Append("Konec");
				timer_Playing = false;
			}
		}
	}
}
}
