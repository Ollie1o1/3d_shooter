LLVM     := /opt/homebrew/opt/llvm
SDK      := /Library/Developer/CommandLineTools/SDKs/MacOSX15.5.sdk
CXX      := $(LLVM)/bin/clang++
CXXFLAGS := -std=c++17 -O3 -march=native -ffast-math -Wall -Wextra \
            -DGL_SILENCE_DEPRECATION \
            -isysroot $(SDK) \
            -I/opt/homebrew/include \
            -I$(LLVM)/lib/c++/v1 \
            $(shell sdl2-config --cflags)
LDFLAGS  := $(shell sdl2-config --libs) \
            -L$(LLVM)/lib/c++ -lc++ \
            -framework OpenGL \
            -lSDL2_mixer

SRC      := src/main.cpp
TARGET   := shooter

HEADERS  := src/Camera.h src/Player.h src/Mesh.h src/ShaderProgram.h \
            src/GameState.h src/MenuState.h src/GameplayState.h \
            src/Enemy.h src/Projectile.h src/GrappleHook.h \
            src/StyleSystem.h src/UIRenderer.h src/PostProcess.h \
            src/Level.h src/AudioSystem.h

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
