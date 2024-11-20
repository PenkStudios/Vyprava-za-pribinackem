all: mod install

mod:
	x86_64-w64-mingw32-g++ -std=c++17 -static -shared -fPIC -L. -l:../build/raylib.dll mod.cpp -o mod.dll
	g++ -std=c++17 -shared -fPIC -lraylibdll mod.cpp -o mod.so

install:
	mv mod.dll ../build/assets/mods
	mv mod.so ../build/assets/mods
