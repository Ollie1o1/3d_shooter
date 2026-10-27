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
BASEFLAGS := -std=c++17 -O3 -ffast-math -Wall -Wextra -Isrc \
             -DGL_SILENCE_DEPRECATION \
             -I/opt/homebrew/include \
             $(shell sdl2-config --cflags)
ifneq ($(wildcard $(LLVM)/bin/clang++),)
CXX      := $(LLVM)/bin/clang++
CXXFLAGS := $(BASEFLAGS) -isysroot $(SDK) -I$(LLVM)/include/c++/v1
STDLIB   := -L$(LLVM)/lib/c++ -Wl,-rpath,$(LLVM)/lib/c++   # clang++ adds -lc++ itself
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
CXXFLAGS := -std=c++17 -O3 -ffast-math -Wall -Wextra -Isrc \
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
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Isrc \
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
# The whole game is one translation unit: main.cpp includes the headers, which
# are grouped by area under src/ (see the README's "Code layout"). Any header
# change rebuilds it, so new headers never need listing here.
SRC     := src/main.cpp
HEADERS := $(shell find src -name '*.h')
TESTS   := tests/test_physics.cpp tests/test_game.cpp tests/test_zenith.cpp tests/BossSim.h

.PHONY: all clean run test web scan

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: all
	./$(TARGET)

# Headless tests (no window or GL context): player physics; the enemy AI, wave
# director, level data, weapons and XP through simulated ARENA, FAST and ACT II
# runs; and the Zenith (the Absolute, the sword, BossSim's kill times)
test: $(TESTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/test_physics.cpp -o tests/test_physics $(STDLIB)
	$(CXX) $(CXXFLAGS) tests/test_game.cpp -o tests/test_game $(STDLIB)
	./tests/test_physics
	./tests/test_game
	$(CXX) $(CXXFLAGS) tests/test_zenith.cpp -o tests/test_zenith $(STDLIB)
	./tests/test_zenith

# Every arena's traps: places you could stand and not get back from by yourself (tests/BoundsScan.h)
scan: tests/scan_bounds.cpp tests/BoundsScan.h $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/scan_bounds.cpp -o tests/scan_bounds $(STDLIB)
	./tests/scan_bounds

# Browser build (WebGL2 + WebAssembly) → web/dist. Needs Emscripten on PATH:
#   source ~/emsdk/emsdk_env.sh && make web && python3 -m http.server -d web/dist
WEB_OUT  := web/dist
WEB_DATA := build/web-data
web: $(SRC) $(HEADERS) web/index.html
	rm -rf $(WEB_DATA) build/web-include && mkdir -p $(WEB_DATA)/src/shaders build/web-include $(WEB_OUT)
	cp src/shaders/*.vert src/shaders/*.frag $(WEB_DATA)/src/shaders/
	cp -R assets $(WEB_DATA)/assets
	rm -rf $(WEB_DATA)/assets/sfx/src   # the frozen recordings gen_arsenal.py reads: the game never loads them
	@command -v oggenc >/dev/null || { echo "make web needs oggenc (brew install vorbis-tools / apt install vorbis-tools)"; exit 1; }
	# Sounds ship as Ogg Vorbis q7: ~1.2 MB instead of ~5.8 MB of WAV (the game prefers .ogg when both exist)
	for f in $(WEB_DATA)/assets/sfx/*.wav; do oggenc -Q -q 7 -o "$${f%.wav}.ogg" "$$f" && rm "$$f" || exit 1; done
	ln -s $$(brew --prefix glm 2>/dev/null || echo /usr)/include/glm build/web-include/glm
	em++ -std=c++17 -O3 -Isrc -Ibuild/web-include $(SRC) -o $(WEB_OUT)/overdrive.js \
	    -sUSE_SDL=2 -sUSE_SDL_MIXER=2 \
	    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
	    -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web \
	    --preload-file $(WEB_DATA)@/
	cp web/index.html $(WEB_OUT)/

clean:
	rm -f shooter shooter.exe tests/test_physics tests/test_game tests/test_zenith
	rm -rf build web/dist
