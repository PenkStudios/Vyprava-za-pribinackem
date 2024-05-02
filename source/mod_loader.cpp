#ifndef MOD_LOADER_CXX
#define MOD_LOADER_CXX

#define FUNCTION_LIST {{"Init", nullptr}, {"Update_Intro", nullptr}, {"Update_Menu", nullptr}, {"Update_Game", nullptr}, {"Update_Game_UI", nullptr}}

#if defined(__WIN32__) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // WINDOWS

#include <vector>
#include <map>
#include <iostream>
#include <cstring>
#include <libloaderapi.h>
#include <errhandlingapi.h>
#include <filesystem>

typedef void (*callback_Function)(void*);

struct pmMod {
	HMODULE handle;

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
        if(iterator.second == NULL) {
            mods.pop_back();
            std::cerr << "GetProcAdress: " << GetLastError() << std::endl;
            std::cerr << "[ Mod loader ] GetProcAdress failed" << std::endl;
            return false;
        }
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

void Mod_Callback(std::string function, void* context) {
	for(pmMod &mod : mods) {
		mod.functions[function](context);
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

struct pmMod {
	void *handle;

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
        if(iterator.second == NULL) {
            mods.pop_back();
            std::cerr << "dlsym: " << dlerror() << std::endl;
            std::cerr << "[ Mod loader ] dlsym failed" << std::endl;
            return false;
        }
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

void Mod_Callback(std::string function, void* context) {
	for(pmMod &mod : mods) {
		mod.functions[function](context);
	}
}

#endif


#endif // MOD_LOADER_CXX