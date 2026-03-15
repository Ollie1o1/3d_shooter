#pragma once
// =============================================================================
// Level.h — Arena geometry (walls/platforms) + room management
// =============================================================================
//
// CORE CONCEPT: every Wall is an AABB (axis-aligned bounding box) used for
// both collision AND rendering. BuildWorldMesh() in GameplayState.h reads all
// walls and draws each face as a quad automatically.
//
// HOW TO ADD A WALL / PLATFORM / BOX:
//   r.walls.push_back({ AABB{{minX, minY, minZ}, {maxX, maxY, maxZ}} });
//   minY = bottom, maxY = top. Floor is always Y=0.
//
// HOW TO ADD AN ENEMY SPAWN:
//   r.enemySpawns.push_back({worldX, 0.f, worldZ});
//   Types cycle GRUNT → SHOOTER → STALKER; override in
//   GameplayState::spawnEnemiesForRoom() for a specific type.
//
// JUMP HEIGHTS (approximate, use these to design accessible platforms):
//   Single jump : ~1.5 m  (anything up to Y ≈ 1.5 reachable from the floor)
//   Double jump : ~3.0 m  (up to Y ≈ 3.0)
//   Grapple     : unlimited — tall pillars and ledges reward grapple use
//
// CEILING: set to Y = 7 in buildWorldMesh(); keep all geometry below Y = 6.
// =============================================================================

#include "Player.h"
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Room — a self-contained section of the arena.
// cleared: set true when all enemies are dead (triggers door animation).
// doorWallIndex: index into walls[] of the wall that animates open on clear.
//                Set to -1 if the room has no exit door.
// ---------------------------------------------------------------------------
struct Room {
    std::vector<Wall>      walls;
    std::vector<glm::vec3> enemySpawns;
    bool  cleared        = false;
    bool  unlocked       = false;
    int   doorWallIndex  = -1;   // -1 = no door
};

struct LevelData {
    std::vector<Room> rooms;
    int currentRoom = 0;

    // Call every frame — slides open any door whose room was just cleared.
    void update(float dt) {
        for (auto& room : rooms) {
            if (room.doorWallIndex >= 0 && room.cleared) {
                float& botY = room.walls[room.doorWallIndex].box.min.y;
                float& topY = room.walls[room.doorWallIndex].box.max.y;
                botY = std::max(botY - 5.f * dt, -6.f);
                topY = std::max(topY - 5.f * dt,  0.f);
            }
        }
    }

    // Flat list of all walls across all rooms — passed to collision and renderer.
    std::vector<Wall> getAllWalls() const {
        std::vector<Wall> out;
        for (auto& r : rooms)
            for (auto& w : r.walls)
                out.push_back(w);
        return out;
    }
};

// =============================================================================
// buildLevel() — defines the arena.
//
// Current layout: one large room (OVERDRIVE ARENA).
//   • Perimeter walls form a 40 × 36 arena.
//   • Central hub:  elevated 12 × 12 platform, 2.5 m high.
//   • Approach risers: lower steps leading to the hub (single-jump to riser,
//     double-jump from riser to hub).
//   • Corner platforms: 1.5 m high — single-jumpable from the floor.
//   • Cover walls: waist-height L-shapes for tactical play.
//   • Tall pillars: 4 m high — grapple anchor points.
//   • Wall catwalks: 3 m high ledges around the perimeter (grapple access).
//
// To add a second room: copy the Room{} block, give it walls and spawns,
// set room 0's doorWallIndex to the connecting wall, and push_back both rooms.
// =============================================================================
inline LevelData buildLevel() {
    LevelData level;

    // -------------------------------------------------------------------------
    // Room 0 — OVERDRIVE ARENA
    // -------------------------------------------------------------------------
    {
        Room r;
        r.unlocked = true;

        // ---- Perimeter walls ------------------------------------------------
        // Format: {{minX, minY, minZ}, {maxX, maxY, maxZ}}
        // Walls are 10 units tall for a spacious arena feel.
        r.walls = {
            // North wall
            { AABB{{-24.f, 0.f, -22.f}, { 24.f, 10.f, -21.f}} },
            // South wall
            { AABB{{-24.f, 0.f,  21.f}, { 24.f, 10.f,  22.f}} },
            // West wall
            { AABB{{-24.f, 0.f, -21.f}, {-23.f, 10.f,  21.f}} },
            // East wall
            { AABB{{ 23.f, 0.f, -21.f}, { 24.f, 10.f,  21.f}} },

            // ---- Central hub -----------------------------------------------
            // 2.5 m tall — reach it by jumping onto a riser first (see below),
            // then double-jumping up. Or grapple directly from mid-range.
            { AABB{{ -6.f, 0.f,  -6.f}, {  6.f, 2.5f,  6.f}} },

            // ---- Hub approach risers (one per cardinal direction) -----------
            // Each riser is 0.9 m tall — single-jump from floor, then
            // double-jump to the hub surface at 2.5 m.
            { AABB{{ -3.5f, 0.f,  6.f}, {  3.5f, 0.9f,  8.5f}} }, // south riser
            { AABB{{ -3.5f, 0.f, -8.5f},{ 3.5f, 0.9f, -6.f }} }, // north riser
            { AABB{{  6.f,  0.f, -3.5f},{ 8.5f, 0.9f,  3.5f}} }, // east riser
            { AABB{{ -8.5f, 0.f, -3.5f},{-6.f,  0.9f,  3.5f}} }, // west riser

            // ---- Corner platforms (1.5 m — single-jump) --------------------
            { AABB{{-19.f, 0.f, -17.f}, {-13.f, 1.5f, -11.f}} }, // NW corner
            { AABB{{ 13.f, 0.f, -17.f}, { 19.f, 1.5f, -11.f}} }, // NE corner
            { AABB{{-19.f, 0.f,  11.f}, {-13.f, 1.5f,  17.f}} }, // SW corner
            { AABB{{ 13.f, 0.f,  11.f}, { 19.f, 1.5f,  17.f}} }, // SE corner

            // ---- Mid-lane side ledges (1.8 m — borderline double-jump) -----
            { AABB{{-19.f, 0.f,  -3.f}, {-14.f, 1.8f,   3.f}} }, // west ledge
            { AABB{{ 14.f, 0.f,  -3.f}, { 19.f, 1.8f,   3.f}} }, // east ledge

            // ---- Cover walls (1.3 m — crouch or strafe behind) -------------
            // North pair
            { AABB{{ -11.f, 0.f, -11.f}, { -8.f, 1.3f,  -9.f}} },
            { AABB{{   8.f, 0.f, -11.f}, { 11.f, 1.3f,  -9.f}} },
            // South pair
            { AABB{{ -11.f, 0.f,   9.f}, { -8.f, 1.3f,  11.f}} },
            { AABB{{   8.f, 0.f,   9.f}, { 11.f, 1.3f,  11.f}} },
            // Diagonal fillers (make L-shapes)
            { AABB{{ -11.f, 0.f, -11.f}, { -9.f, 1.3f,  -8.f}} },
            { AABB{{   9.f, 0.f, -11.f}, { 11.f, 1.3f,  -8.f}} },
            { AABB{{ -11.f, 0.f,   8.f}, { -9.f, 1.3f,  11.f}} },
            { AABB{{   9.f, 0.f,   8.f}, { 11.f, 1.3f,  11.f}} },

            // ---- Tall pillars (4 m — grapple anchors) ----------------------
            // Positioned at mid-arena, 45° from centre.
            { AABB{{ -1.f, 0.f, -15.f}, {  1.f, 4.f, -13.f}} }, // far north
            { AABB{{ -1.f, 0.f,  13.f}, {  1.f, 4.f,  15.f}} }, // far south
            { AABB{{-15.f, 0.f,  -1.f}, {-13.f, 4.f,   1.f}} }, // far west
            { AABB{{ 13.f, 0.f,  -1.f}, { 15.f, 4.f,   1.f}} }, // far east

            // ---- Perimeter catwalks (4 m high shelf, grapple up to them) ---
            { AABB{{-24.f, 4.f, -21.f}, {-20.f, 4.3f, 21.f}} }, // west catwalk
            { AABB{{ 20.f, 4.f, -21.f}, { 24.f, 4.3f, 21.f}} }, // east catwalk
            { AABB{{-20.f, 4.f, -22.f}, { 20.f, 4.3f,-18.f}} }, // north catwalk
            { AABB{{-20.f, 4.f,  18.f}, { 20.f, 4.3f, 22.f}} }, // south catwalk
        };

        // No door (single-room arena for now; doorWallIndex stays -1).

        // ---- Enemy spawn points -------------------------------------------
        // Spread across different heights and zones so enemies use the terrain.
        // Add more spawns here — or reduce count — to tune difficulty.
        // Player spawns at {0, 0, 14} — keep all enemies well clear of that point.
        r.enemySpawns = {
            {-14.f, 0.f, -14.f},  // NW
            { 14.f, 0.f, -14.f},  // NE
            {-14.f, 0.f,   6.f},  // SW (away from player spawn)
            { 14.f, 0.f,   6.f},  // SE
            {  0.f, 0.f, -18.f},  // far north
            {-20.f, 0.f,  -6.f},  // far west
            { 20.f, 0.f,  -6.f},  // far east
            {  8.f, 0.f,  18.f},  // south-east, away from player
        };

        level.rooms.push_back(std::move(r));
    }

    return level;
}
