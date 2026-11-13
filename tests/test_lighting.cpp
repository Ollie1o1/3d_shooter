// Validate the shipped lighting against the geometry built on this target.
// Run as both native code and WebAssembly: compiler rounding must not change
// procedural scenery enough to silently discard a level's lighting.
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "world/LevelLab.h"
#include "world/LevelGauntlet.h"
#include "world/LevelAct2.h"
#include "world/LevelTraining.h"
#include "world/LevelRing.h"
#include "render/LightBake.h"
#include <cstdio>

int main() {
    struct Entry {const char* name; LevelData (*build)();};
    int failures=0;
    for (auto entry : {Entry{"act1",buildLevel}, Entry{"act2",buildAct2Level},
            Entry{"fast",buildGauntlet}, Entry{"story1",buildCradle},
            Entry{"story2",buildWasteland}, Entry{"story3",buildLab},
            Entry{"training",buildTraining}, Entry{"story6",buildRing}}) {
        auto level=entry.build();
        std::string error;
        bool valid=!lightbake::load(entry.name,level,error).empty();
        std::printf("%s: %s lighting %s\n",valid?"ok":"FAIL",entry.name,error.c_str());
        failures+=!valid;
    }
    return failures?1:0;
}
