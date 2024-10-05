CXX=/bin/g++
WINDOWS-CXX=/bin/x86_64-w64-mingw32-g++
WINDOWS-WINDRES=/bin/x86_64-w64-mingw32-windres

LINK=-std=c++17

all: release

release:
	rm -f build/assets/player.txt
	rm -f build/assets/missions.txt
	make linux
	make windows
	make finish

linux: source/main.cpp
	$(CXX) source/main.cpp $(LINK) -L. -l:build/libraylib.so.500 -lenet -o build/pribináček

windows: source/main.cpp
	$(WINDOWS-WINDRES) icon.rc -O coff -o icon.res
	$(WINDOWS-CXX) source/main.cpp icon.res $(LINK) -L. -l:build/raylib.dll -static -mwindows -lenet -lwinmm -lws2_32  -o build/pribináček.exe

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
