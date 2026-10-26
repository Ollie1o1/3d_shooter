#pragma once
// =============================================================================
// Stuck.h — is the player's body inside something solid? Shrunk a little on
// every side so standing against a wall or on a box top never counts: only
// being pushed into one does (a moving platform, a rising floor, a closing
// door). GameplayState::keepPlayerInZone puts you back on safe ground after a
// moment of it; tests/test_game.cpp drives real movement through every arena
// to show ordinary play never sets it off.
// =============================================================================
#include "core/Collision.h"
#include <vector>

inline bool bodyEmbedded(const std::vector<Wall>& walls, const std::vector<int>& moverWalls, const SpatialGrid& grid,
                         glm::vec3 feet, float radius, float height) {
    static std::vector<int> cands;
    AABB body{feet + glm::vec3{-radius + 0.06f, 0.08f, -radius + 0.06f},
              feet + glm::vec3{ radius - 0.06f, height - 0.08f, radius - 0.06f}};
    grid.query(body, cands);
    for (int i : moverWalls) cands.push_back(i);
    for (int i : cands) {
        const AABB& b = walls[i].box;
        if (body.min.x < b.max.x && body.max.x > b.min.x && body.min.y < b.max.y && body.max.y > b.min.y &&
            body.min.z < b.max.z && body.max.z > b.min.z) return true;
    }
    return false;
}
