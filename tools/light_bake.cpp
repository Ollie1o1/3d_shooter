// light_bake: bake each level's light (ambient occlusion and its arenas' sun shadows) into
// assets/levels/<level>.light (render/LightBake.h). Run from the repo root (make light builds and runs it):
//
//   tools/light_bake [act1 act2 fast story1]
//
// Rebake after changing a level (its code or its layout file): the game leaves out a bake of an
// older level, and the tests say which is stale.
#include "world/Level.h"
#include "world/LevelAct2.h"
#include "world/LevelGauntlet.h"
#include "world/LevelStory.h"
#include "render/LightBake.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>

int main(int argc, char** argv) {
    std::vector<std::string> levels;
    for (int i = 1; i < argc; ++i) levels.push_back(argv[i]);
    if (levels.empty()) levels = {"act1", "act2", "fast", "story1"};
    mkdir("assets/levels", 0755);
    for (const std::string& l : levels) {
        LevelData L = l == "act1" ? buildLevel() : l == "act2" ? buildAct2Level() : l == "fast" ? buildGauntlet() : l == "story1" ? buildCradle() : LevelData{};
        if (L.arenas.empty()) { std::fprintf(stderr, "no level '%s' (act1, act2, fast, story1)\n", l.c_str()); return 2; }
        auto t0 = std::chrono::steady_clock::now();
        std::printf("%s:\n", l.c_str());
        lightbake::Bake B = lightbake::bake(L, 0, [](const char* s) { std::printf("%s\n", s); std::fflush(stdout); });
        std::string bytes = lightbake::serialise(B);
        std::ofstream f(lightbake::pathFor(l), std::ios::binary);
        if (!(f << bytes)) { std::fprintf(stderr, "can't write %s\n", lightbake::pathFor(l).c_str()); return 1; }
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::printf("  -> %s, %.1f KB (%zu cells), %.1f s\n", lightbake::pathFor(l).c_str(), bytes.size() / 1024.0, B.cells.size() / 2, secs);
    }
    return 0;
}
