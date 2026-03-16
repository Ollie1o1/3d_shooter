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
// JUMP HEIGHTS (approximate):
//   Single jump : ~1.5 m  (floor → platform edge at Y ≤ 1.3 comfortable)
//   Double jump : ~3.0 m  (from ground, reaches Y ≈ 3.0)
//   From riser  : riser_Y + 1.5 (single) / riser_Y + 3.0 (double)
//   Grapple     : unlimited — tall pillars and catwalks reward grapple use
//
// CEILING: Y = 14. Keep all geometry below Y = 12.
//
// ARENA LAYOUT (80 × 80 units, X/Z: −40 … 40):
//
//   Zone 1 — Central Hub        : X[−7,7]    Z[−7,7]    Y=3.5  orange tower
//   Zone 2 — North Fortress     : X[−22,22]  Z[−38,−20] Y=4    blue-grey slab
//              NW / NE Towers   : Y=9 corner pylons, great grapple anchors
//   Zone 3 — East Elevated Walk : X[18,40]   Z[−15,15]  Y=3    raised highway
//   Zone 4 — West Raised Slabs  : X[−40,−18] Z[−15,3]   Y=2/4  ascending terraces
//   Zone 5 — South Open Zone    : Z[10,38]               Y=0    player start + cover
//   Zone 6 — Mid-Arena Pillars  : 8 tall pillars ringing the hub at ~Y=7
//   Zone 7 — Corner Towers      : SW + SE corners, Y=7
//   Zone 8 — Perimeter Catwalks : all 4 walls at Y=5
//   Zone 9 — Floating Platforms : 2 hover pads north + south of hub at Y=5.5
// =============================================================================

#include "Player.h"
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

struct Room {
    std::vector<Wall>      walls;
    std::vector<glm::vec3> enemySpawns;
    bool  cleared       = false;
    bool  unlocked      = false;
    int   doorWallIndex = -1;
};

struct LevelData {
    std::vector<Room> rooms;
    int currentRoom = 0;

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

    std::vector<Wall> getAllWalls() const {
        std::vector<Wall> out;
        for (auto& r : rooms)
            for (auto& w : r.walls)
                out.push_back(w);
        return out;
    }
};

// =============================================================================
// buildLevel() — 80 × 80 arena with multiple distinct combat zones.
// Wall order must stay in sync with wallColors[] in GameplayState.h.
// =============================================================================
inline LevelData buildLevel() {
    LevelData level;
    Room r;
    r.unlocked = true;

    r.walls = {

        // =====================================================================
        // ZONE 0 — PERIMETER WALLS (indices 0-3)
        // 12 m tall boundary. Floor at Y=0, ceiling at Y=14.
        // =====================================================================
        { AABB{{-40.f, 0.f,-41.f},{ 40.f,12.f,-40.f}} }, // 0  North wall
        { AABB{{-40.f, 0.f, 40.f},{ 40.f,12.f, 41.f}} }, // 1  South wall
        { AABB{{-41.f, 0.f,-40.f},{-40.f,12.f, 40.f}} }, // 2  West wall
        { AABB{{ 40.f, 0.f,-40.f},{ 41.f,12.f, 40.f}} }, // 3  East wall

        // =====================================================================
        // ZONE 1 — CENTRAL HUB (index 4)
        // 14 × 14 tower, 3.5 m tall.
        // Approach via the 4 risers below (single-jump riser → single-jump hub).
        // =====================================================================
        { AABB{{ -7.f, 0.f, -7.f},{  7.f, 3.5f,  7.f}} }, // 4  Hub

        // Hub approach risers — Y=1.0, step up from floor then jump to hub top.
        // From floor (0) → riser (1.0) : easy single jump
        // From riser (1.0) → hub (3.5) : 2.5 m gap, clear with double jump
        { AABB{{ -4.f, 0.f,  7.f},{  4.f, 1.0f, 10.f}} }, // 5  South riser
        { AABB{{ -4.f, 0.f,-10.f},{  4.f, 1.0f, -7.f}} }, // 6  North riser
        { AABB{{  7.f, 0.f, -4.f},{ 10.f, 1.0f,  4.f}} }, // 7  East riser
        { AABB{{-10.f, 0.f, -4.f},{ -7.f, 1.0f,  4.f}} }, // 8  West riser

        // =====================================================================
        // ZONE 2 — NORTH FORTRESS (indices 9-12)
        // Massive raised slab, 4 m tall. Two huge corner towers (9 m).
        // Approach from south: access riser at Y=2, then single-jump to top (4 m).
        // =====================================================================
        { AABB{{-22.f, 0.f,-38.f},{ 22.f, 4.f,-20.f}} }, // 9  Fortress floor
        { AABB{{-10.f, 0.f,-20.f},{ 10.f, 2.f,-16.f}} }, // 10 South approach riser
                                                           //    floor→riser(2): double jump
                                                           //    riser(2)→fortress(4): single jump
        { AABB{{-22.f, 0.f,-38.f},{-18.f, 9.f,-34.f}} }, // 11 NW tower (grapple anchor)
        { AABB{{ 18.f, 0.f,-38.f},{  22.f, 9.f,-34.f}} }, // 12 NE tower (grapple anchor)

        // =====================================================================
        // ZONE 3 — EAST ELEVATED WALKWAY (indices 13-16)
        // Long raised highway at Y=3 along the east wall.
        // Access step at Y=1.2 → single-jump reaches Y=2.7 (walkway at Y=3 with run-up).
        // Two tall support columns serve as grapple anchors.
        // =====================================================================
        { AABB{{ 18.f, 0.f,-15.f},{ 40.f, 3.f, 15.f}} }, // 13 East walkway
        { AABB{{ 14.f, 0.f, -6.f},{ 18.f, 1.5f, 6.f}} }, // 14 Access step
        { AABB{{ 37.f, 0.f,-15.f},{ 40.f, 6.f,-11.f}} }, // 15 North support column
        { AABB{{ 37.f, 0.f, 11.f},{ 40.f, 6.f, 15.f}} }, // 16 South support column

        // =====================================================================
        // ZONE 4 — WEST RAISED TERRACES (indices 17-19)
        // Two ascending platforms + a tall tower. Provides vertical variety on
        // the west side. Platform A (Y=2) reachable by double-jump from floor.
        // Platform B (Y=4) reachable from A via double-jump.
        // =====================================================================
        { AABB{{-40.f, 0.f, -5.f},{-18.f, 2.f,  5.f}} }, // 17 West platform A
        { AABB{{-40.f, 0.f,-15.f},{-28.f, 4.f, -7.f}} }, // 18 West platform B
        { AABB{{-40.f, 0.f,-24.f},{-34.f, 8.f,-18.f}} }, // 19 West tall tower

        // =====================================================================
        // ZONE 5 — COVER WALLS (indices 20-29)
        // Waist-height (1.4 m) L-shaped cover. Paired north and south of center,
        // plus two long central covers in the south open zone where the player
        // spawns. Each L-shape is two wall segments.
        // =====================================================================
        // North zone — NW L-shape
        { AABB{{-13.f, 0.f,-18.f},{ -9.f, 1.4f,-15.f}} }, // 20
        { AABB{{-13.f, 0.f,-15.f},{-10.f, 1.4f,-12.f}} }, // 21
        // North zone — NE L-shape
        { AABB{{  9.f, 0.f,-18.f},{ 13.f, 1.4f,-15.f}} }, // 22
        { AABB{{  9.f, 0.f,-15.f},{ 12.f, 1.4f,-12.f}} }, // 23
        // South zone — SW L-shape
        { AABB{{-18.f, 0.f, 20.f},{-14.f, 1.4f, 24.f}} }, // 24
        { AABB{{-18.f, 0.f, 24.f},{-15.f, 1.4f, 27.f}} }, // 25
        // South zone — SE L-shape
        { AABB{{ 14.f, 0.f, 20.f},{ 18.f, 1.4f, 24.f}} }, // 26
        { AABB{{ 14.f, 0.f, 24.f},{ 15.f, 1.4f, 27.f}} }, // 27
        // Center-south long covers (flanking the player spawn lane)
        { AABB{{ -5.f, 0.f, 25.f},{  5.f, 1.4f, 28.f}} }, // 28
        { AABB{{ -7.f, 0.f, 31.f},{  7.f, 1.4f, 33.f}} }, // 29

        // =====================================================================
        // ZONE 6 — MID-ARENA PILLARS (indices 30-37)
        // 8 tall pillars ringing the central hub at ~17 m radius.
        // The 4 cardinal pillars are 7 m tall (major grapple anchors).
        // The 4 diagonal pillars are 5 m tall (secondary anchors).
        // =====================================================================
        { AABB{{ -1.f, 0.f,-17.f},{  1.f, 7.f,-15.f}} }, // 30 N pillar
        { AABB{{ -1.f, 0.f, 15.f},{  1.f, 7.f, 17.f}} }, // 31 S pillar
        { AABB{{-17.f, 0.f, -1.f},{-15.f, 7.f,  1.f}} }, // 32 W pillar
        { AABB{{ 15.f, 0.f, -1.f},{ 17.f, 7.f,  1.f}} }, // 33 E pillar
        { AABB{{-17.f, 0.f,-17.f},{-14.f, 5.f,-14.f}} }, // 34 NW diagonal pillar
        { AABB{{ 14.f, 0.f,-17.f},{ 17.f, 5.f,-14.f}} }, // 35 NE diagonal pillar
        { AABB{{-17.f, 0.f, 14.f},{-14.f, 5.f, 17.f}} }, // 36 SW diagonal pillar
        { AABB{{ 14.f, 0.f, 14.f},{ 17.f, 5.f, 17.f}} }, // 37 SE diagonal pillar

        // =====================================================================
        // ZONE 7 — CORNER TOWERS (indices 38-39)
        // Two 7 m towers in the south corners. Distinct purple-grey colour.
        // Good high ground for the player or enemies. Grapple to reach.
        // =====================================================================
        { AABB{{-40.f, 0.f, 36.f},{-36.f, 7.f, 40.f}} }, // 38 SW corner tower
        { AABB{{ 36.f, 0.f, 36.f},{ 40.f, 7.f, 40.f}} }, // 39 SE corner tower

        // =====================================================================
        // ZONE 8 — PERIMETER CATWALKS (indices 40-43)
        // Thin shelf (0.3 m tall) at Y=5 hugging all 4 walls.
        // Grapple to the pillars or towers then hop across.
        // =====================================================================
        { AABB{{-40.f, 5.f,-40.f},{-37.f, 5.3f, 40.f}} }, // 40 West catwalk
        { AABB{{ 37.f, 5.f,-40.f},{ 40.f, 5.3f, 40.f}} }, // 41 East catwalk
        { AABB{{-37.f, 5.f,-40.f},{ 37.f, 5.3f,-37.f}} }, // 42 North catwalk
        { AABB{{-37.f, 5.f, 37.f},{ 37.f, 5.3f, 40.f}} }, // 43 South catwalk

        // =====================================================================
        // ZONE 9 — FLOATING PLATFORMS (indices 44-45)
        // Thin hover pads suspended at Y=5.5, north and south of the hub.
        // Only reachable via grapple — reward skilled movement.
        // =====================================================================
        { AABB{{ -4.f, 5.5f,-19.f},{  4.f, 5.8f,-16.f}} }, // 44 North hover pad
        { AABB{{ -4.f, 5.5f, 16.f},{  4.f, 5.8f, 19.f}} }, // 45 South hover pad
    };

    // -------------------------------------------------------------------------
    // Enemy spawns — player starts at {0, 0, 32} (south open zone).
    // Enemies are spread across every zone so combat flows through the whole map.
    // -------------------------------------------------------------------------
    r.enemySpawns = {
        // North fortress (on top of 4 m slab)
        {  0.f, 4.1f,-30.f },
        {-12.f, 4.1f,-28.f },
        { 12.f, 4.1f,-28.f },
        {  0.f, 4.1f,-36.f },  // deep on fortress, near NW/NE towers
        // East walkway
        { 28.f, 3.1f,  0.f },
        { 28.f, 3.1f, -8.f },
        // West terraces
        {-30.f, 2.1f,  0.f },
        {-34.f, 4.1f,-11.f },
        // Mid-arena (ground, between pillars)
        {-14.f, 0.f, -14.f },
        { 14.f, 0.f, -14.f },
        {  0.f, 0.f, -14.f },
        { 14.f, 0.f,   0.f },
        // South open zone (player has to push through cover to engage)
        {-18.f, 0.f,  18.f },
        { 18.f, 0.f,  18.f },
        {  0.f, 0.f,  20.f },
    };

    level.rooms.push_back(std::move(r));
    return level;
}
