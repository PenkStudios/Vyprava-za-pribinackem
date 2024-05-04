CXX=/bin/g++
WINDOWS-CXX=/bin/x86_64-w64-mingw32-g++

LINK=-std=c++17

all: linux finish

release:
	make linux
	make windows
	make finish

linux: source/main.cpp
	$(CXX) source/main.cpp $(LINK) -L. -l:build/libraylib.so.500 -o build/pribiňáček

windows: source/main.cpp
	$(WINDOWS-CXX) source/main.cpp $(LINK) -L. -l:build/raylib.dll -static -mwindows -lwinmm -o build/pribiňáček.exe

finish:
	@echo "\033[96m"

	@echo " _______________________________________"
	@echo "[                                       ]"
	@echo "[     Build finished successfully       ]"
	@echo "[                                       ]"
	@echo " ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯"

	@echo "\033[0m"