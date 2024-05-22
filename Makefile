CXX=/bin/g++
WINDOWS-CXX=/bin/x86_64-w64-mingw32-g++

LINK=-std=c++17 -g

all: linux windows finish

release:
	make linux
	make windows
	make finish

linux: source/main.cpp
	$(CXX) source/main.cpp $(LINK) -L. -l:build/libraylib.so.500 -o build/pribináček

windows: source/main.cpp
	$(WINDOWS-CXX) source/main.cpp $(LINK) -L. -l:build/raylib.dll -static -mwindows -lwinmm -o build/pribináček.exe

finish:
	@echo "\033[96m"

	@echo " _______________________________________"
	@echo "[                                       ]"
	@echo "[     Build finished successfully       ]"
	@echo "[                                       ]"
	@echo " ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯"

	@echo "\033[0m"
