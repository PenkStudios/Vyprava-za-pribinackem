CXX=/bin/g++
WINDOWS-CXX=/bin/x86_64-w64-mingw32-g++

LINK=-lraylib -std=c++17 -g

all: linux windows finish

linux: source/main.cpp
	$(CXX) source/main.cpp $(LINK) -o build/pribiňáček

windows: source/main.cpp
	$(WINDOWS-CXX) source/main.cpp $(LINK) -static -mwindows -lwinmm -o build/pribiňáček.exe

finish:
	@echo "\033[96m"

	@echo " _______________________________________"
	@echo "[                                       ]"
	@echo "[     Build finished successfully       ]"
	@echo "[                                       ]"
	@echo " ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯"

	@echo "\033[0m"