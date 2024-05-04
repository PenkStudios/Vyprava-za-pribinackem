#ifndef MOD_LOADER_CXX
#define MOD_LOADER_CXX

// Ordered - for 2d function where the order of drawing matters (extra bool argument "in_Front")
#define FUNCTION_LIST_ORDERED {{"Update_Intro_2D", nullptr}, {"Update_Menu_2D", nullptr}, {"Update_Game_2D", nullptr}}
#define FUNCTION_LIST {{"Init", nullptr}, {"Init_Intro", nullptr}, {"Switch_Intro", nullptr}, {"Init_Menu", nullptr}, {"Switch_Menu", nullptr}, {"Update_Menu_3D", nullptr}, {"Init_Game", nullptr}, {"Switch_Game", nullptr}, {"Update_Game_3D", nullptr}}

#if defined(__WIN32__) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // WINDOWS

#include <vector>
#include <map>
#include <iostream>
#include <cstring>
#include <libloaderapi.h>
#include <errhandlingapi.h>
#include <filesystem>

typedef void (*callback_Function)(void*);
typedef void (*callback_Ordered_Function)(void*, bool);

struct pmMod {
	HMODULE handle;

	std::map<std::string, callback_Ordered_Function> functions_Ordered = FUNCTION_LIST_ORDERED;
	std::map<std::string, callback_Function> functions = FUNCTION_LIST;
};

std::vector<pmMod> mods;

bool Mod_Load_Path(std::string path) {
	mods.push_back(pmMod {LoadLibrary(path.c_str())});
	if(mods.back().handle == NULL) {
		mods.pop_back();
        std::cerr << "LoadLibrary: " << GetLastError() << std::endl;
		std::cerr << "[ Mod loader ] LoadLibrary failed" << std::endl;
		return false;
	}

    for(auto &iterator : mods.back().functions) {
        iterator.second = reinterpret_cast<callback_Function>(GetProcAddress(mods.back().handle, iterator.first.c_str()));
    }

	for(auto &iterator : mods.back().functions_Ordered) {
        iterator.second = reinterpret_cast<callback_Ordered_Function>(GetProcAddress(mods.back().handle, iterator.first.c_str()));
    }

	return true;
}

void Mod_Load_Directory(std::string directory) {
	for(auto path : std::filesystem::directory_iterator(directory)) {
		if(path.path().extension().string() == ".dll") {
			std::cout << "[ Mod loader ] Loading " << path.path().string() << std::endl;
			Mod_Load_Path(path.path().string());
		}
    }
}

void Mod_Callback(std::string function, void* context, bool in_Front = true) {
	for(pmMod &mod : mods) {
		if(mod.functions.count(function) && mod.functions[function] != nullptr)
			mod.functions[function](context);

		if(mod.functions_Ordered.count(function) && mod.functions_Ordered[function] != nullptr)
			mod.functions_Ordered[function](context, in_Front);
	}
}

#else // LINUX || MACOS

#include <map>
#include <vector>
#include <iostream>
#include <dlfcn.h>
#include <cstring>
#include <filesystem>

typedef void (*callback_Function)(void*);
typedef void (*callback_Ordered_Function)(void*, bool);

struct pmMod {
	void *handle;

	std::map<std::string, callback_Ordered_Function> functions_Ordered = FUNCTION_LIST_ORDERED;
	std::map<std::string, callback_Function> functions = FUNCTION_LIST;
};

std::vector<pmMod> mods;

bool Mod_Load_Path(std::string path) {
	mods.push_back(pmMod {dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL)});
	if(mods.back().handle == NULL) {
		mods.pop_back();
        std::cerr << "dlopen: " << dlerror() << std::endl;
		std::cerr << "[ Mod loader ] dlopen failed" << std::endl;
		return false;
	}

    for(auto &iterator : mods.back().functions) {
        iterator.second = reinterpret_cast<callback_Function>(dlsym(mods.back().handle, iterator.first.c_str()));
    }

	for(auto &iterator : mods.back().functions_Ordered) {
        iterator.second = reinterpret_cast<callback_Ordered_Function>(dlsym(mods.back().handle, iterator.first.c_str()));
    }

	return true;
}

void Mod_Load_Directory(std::string directory) {
	for(auto path : std::filesystem::directory_iterator(directory)) {
        if(path.path().extension().string() == ".so") {
			std::cout << "[ Mod loader ] Loading " << path.path().string() << std::endl;
			Mod_Load_Path(path.path().string());
		}
    }
}

void Mod_Callback(std::string function, void* context, bool in_Front = true) {
	for(pmMod &mod : mods) {
		if(mod.functions.count(function) && mod.functions[function] != nullptr)
			mod.functions[function](context);

		if(mod.functions_Ordered.count(function) && mod.functions_Ordered[function] != nullptr)
			mod.functions_Ordered[function](context, in_Front);
	}
}

#endif


#endif // MOD_LOADER_CXX