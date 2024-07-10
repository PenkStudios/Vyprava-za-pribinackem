all: mod install

ROOT=..

mod:
	x86_64-w64-mingw32-g++ -std=c++17 -static -shared -fPIC -L$(ROOT)/build -l:raylib.dll -I$(ROOT)/source/scenes mod.cpp -o timer.dll
	g++ -std=c++17 -shared -fPIC -L$(ROOT)/build -l:libraylib.so.500 -I$(ROOT)/source/scenes mod.cpp -o timer.so

install:
	cp timer.so $(ROOT)/build/assets/mods
	cp timer.dll $(ROOT)/build/assets/mods
	cp -r timer $(ROOT)/build/assets/mods
