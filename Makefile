CXX=/bin/g++
WINDOWS-CXX=/bin/x86_64-w64-mingw32-g++

LINK=-std=c++17 -g

all: release

release:
	rm -f build/assets/player.txt
	make linux
	make windows
	make finish

linux: source/main.cpp
	$(CXX) source/main.cpp $(LINK) -L. -l:build/libraylib.so.500 -o build/pribináček

windows: source/main.cpp
	$(WINDOWS-CXX) source/main.cpp $(LINK) -L. -l:build/raylib.dll -static -mwindows -lwinmm -o build/pribináček.exe

git:
	./github.py

finish:
	@echo "\033[96m"

	@echo " _______________________________________"
	@echo "[                                       ]"
	@echo "[     Build finished successfully       ]"
	@echo "[                                       ]"
	@echo " ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯"

	@echo "\033[0m"
