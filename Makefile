# =============================================================================
# Cross-platform Makefile — macOS and Windows (MSYS2/ucrt64)
# =============================================================================

# Detect platform
UNAME := $(shell uname -s 2>/dev/null || echo Windows)

# -----------------------------------------------------------------------------
# macOS
# -----------------------------------------------------------------------------
ifeq ($(UNAME), Darwin)
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
TARGET   := shooter

# -----------------------------------------------------------------------------
# Windows (MSYS2 ucrt64)
# -----------------------------------------------------------------------------
else
UCRT64   := /ucrt64
CXX      := $(UCRT64)/bin/g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra \
            -I$(UCRT64)/include \
            -I$(UCRT64)/include/SDL2
LDFLAGS  := -L$(UCRT64)/lib \
            -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer \
            -lglew32 -lopengl32 -lgdi32 \
            -mwindows
TARGET   := shooter.exe
endif

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------
SRC     := src/main.cpp
HEADERS := src/gl.h \
           src/Camera.h src/Player.h src/Mesh.h src/ShaderProgram.h \
           src/GameState.h src/MenuState.h src/GameplayState.h \
           src/Enemy.h src/Projectile.h src/GrappleHook.h \
           src/StyleSystem.h src/UIRenderer.h src/PostProcess.h \
           src/Level.h src/AudioSystem.h src/ViewModel.h src/Interactable.h

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -f shooter shooter.exe
