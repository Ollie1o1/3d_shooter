#pragma once
#include "world/Level.h"

// Full body clearance and, on islands over a void, support under the feet.
// A point in empty space can be clear of walls and still be an invalid spawn.
inline bool spawnClearance(const LevelData& level, glm::vec3 p, float radius, float height, bool support) {
    const float r = std::max(.1f, radius - .04f);
    AABB body{p + glm::vec3{-r, .06f, -r}, p + glm::vec3{r, height - .04f, r}};
    for (const auto& w : level.walls) {
        const auto& b = w.box;
        if (body.min.x < b.max.x && body.max.x > b.min.x && body.min.y < b.max.y &&
            body.max.y > b.min.y && body.min.z < b.max.z && body.max.z > b.min.z) return false;
    }
    if (support) {
        const glm::vec2 probes[] = {{0,0},{r,0},{-r,0},{0,r},{0,-r}};
        for (auto d : probes)
            if (std::fabs(level.groundAt(p.x+d.x, p.z+d.y, p.y+.1f)-p.y) > .15f) return false;
    }
    return true;
}
