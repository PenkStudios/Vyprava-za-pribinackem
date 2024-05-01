all: mod install

mod:
	x86_64-w64-mingw32-g++ -static -shared -fPIC -L. -l:../build/raylib.dll mod.cpp -o mod.dll
	g++ -shared -fPIC -L. -l:../build/libraylib.so.500 mod.cpp -o mod.so

install:
	mv mod.dll ../build/assets/mods
	mv mod.so ../build/assets/mods