# =============================================================================
# Cross-platform Makefile — macOS and Windows (MSYS2/ucrt64)
# =============================================================================

# Detect platform
UNAME := $(shell uname -s 2>/dev/null || echo Windows)

# -----------------------------------------------------------------------------
# macOS
# -----------------------------------------------------------------------------
ifeq ($(UNAME), Darwin)
# Prefer Homebrew LLVM (some Command Line Tools installs ship a clang++ that
# can't find the C++ stdlib headers); fall back to Apple clang if it's absent.
LLVM     := $(shell brew --prefix llvm 2>/dev/null)
SDK      := $(shell xcrun --show-sdk-path 2>/dev/null)
BASEFLAGS := -std=c++17 -O3 -ffast-math -Wall -Wextra \
             -DGL_SILENCE_DEPRECATION \
             -I/opt/homebrew/include \
             $(shell sdl2-config --cflags)
ifneq ($(wildcard $(LLVM)/bin/clang++),)
CXX      := $(LLVM)/bin/clang++
CXXFLAGS := $(BASEFLAGS) -isysroot $(SDK) -I$(LLVM)/include/c++/v1
STDLIB   := -L$(LLVM)/lib/c++ -Wl,-rpath,$(LLVM)/lib/c++ -lc++
else
CXX      := clang++
CXXFLAGS := $(BASEFLAGS)
STDLIB   :=
endif
LDFLAGS  := $(shell sdl2-config --libs) $(STDLIB) \
            -framework OpenGL \
            -lSDL2_mixer
TARGET   := shooter

# -----------------------------------------------------------------------------
# Linux (apt: g++ libsdl2-dev libsdl2-mixer-dev libglew-dev libglm-dev)
# -----------------------------------------------------------------------------
else ifeq ($(UNAME), Linux)
CXX      := g++
CXXFLAGS := -std=c++17 -O3 -ffast-math -Wall -Wextra \
            $(shell sdl2-config --cflags)
LDFLAGS  := $(shell sdl2-config --libs) -lSDL2_mixer -lGLEW -lGL
STDLIB   :=
TARGET   := shooter

# -----------------------------------------------------------------------------
# Windows (MSYS2 ucrt64)
# -----------------------------------------------------------------------------
else
UCRT64   := /c/msys64/ucrt64
export PATH := $(UCRT64)/bin:$(PATH)
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
           src/Gameplay_Flow.h src/Gameplay_Tick.h src/Gameplay_Combat.h \
           src/Gameplay_Menus.h src/Gameplay_Render.h src/Gameplay_HUD.h src/Gameplay_Dev.h src/WorldMesh.h src/WorldGeometry.h src/Vertex.h src/Effects.h src/ArenaShifts.h src/Gameplay_Shifts.h src/Gameplay_Sovereign.h src/SovereignHazards.h src/Shapes.h src/Score.h src/Daily.h src/EndlessWaves.h \
           src/Enemy.h src/EnemyModel.h src/BoxRenderer.h src/WaveDirector.h \
           src/Projectile.h src/GrappleHook.h \
           src/StyleSystem.h src/UIRenderer.h src/PostProcess.h \
           src/Level.h src/LevelGauntlet.h src/LevelAct2.h src/AudioSystem.h src/ViewModel.h src/Interactable.h \
           src/Settings.h src/SettingsMenu.h src/Persist.h src/PixelFont.h src/UIBatch.h \
           src/Weapons.h src/Progression.h src/MouseFilter.h src/Difficulty.h src/MusicSynth.h src/Display.h

.PHONY: all clean run test web

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: all
	./$(TARGET)

# Headless tests — no window or GL context needed: player physics, plus the
# enemy AI, wave director, level data, weapons, XP and the mouse filter,
# driven through simulated ARENA and FAST runs
test: tests/test_physics.cpp tests/test_game.cpp src/Player.h src/Camera.h \
      src/Enemy.h src/EnemyModel.h src/Level.h src/LevelGauntlet.h src/LevelAct2.h src/WaveDirector.h \
      src/Weapons.h src/Progression.h src/MouseFilter.h src/Persist.h src/Difficulty.h src/MusicSynth.h \
      src/StyleSystem.h src/Projectile.h src/ArenaShifts.h src/Score.h src/Daily.h src/EndlessWaves.h
	$(CXX) $(CXXFLAGS) tests/test_physics.cpp -o tests/test_physics $(STDLIB)
	$(CXX) $(CXXFLAGS) tests/test_game.cpp -o tests/test_game $(STDLIB)
	./tests/test_physics
	./tests/test_game

# Browser build (WebGL2 + WebAssembly) → web/dist. Needs Emscripten on PATH:
#   source ~/emsdk/emsdk_env.sh && make web && python3 -m http.server -d web/dist
WEB_OUT  := web/dist
WEB_DATA := build/web-data
web: $(SRC) $(HEADERS) web/index.html
	rm -rf $(WEB_DATA) build/web-include && mkdir -p $(WEB_DATA)/src build/web-include $(WEB_OUT)
	cp src/*.vert src/*.frag $(WEB_DATA)/src/
	cp -R assets $(WEB_DATA)/assets
	ln -s $$(brew --prefix glm 2>/dev/null || echo /usr)/include/glm build/web-include/glm
	em++ -std=c++17 -O3 -Ibuild/web-include $(SRC) -o $(WEB_OUT)/overdrive.js \
	    -sUSE_SDL=2 -sUSE_SDL_MIXER=2 \
	    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
	    -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web \
	    --preload-file $(WEB_DATA)@/
	cp web/index.html $(WEB_OUT)/

clean:
	rm -f shooter shooter.exe tests/test_physics tests/test_game
	rm -rf build web/dist
