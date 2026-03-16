#pragma once
// =============================================================================
// Level.h — Arena geometry (walls/platforms) + room management
// =============================================================================
//
// ARENA LAYOUT (130 × 130 units, X/Z: −65 … 65)
//
//   Zone 0  — Perimeter walls            : X/Z ±65, Y=14
//   Zone 1  — Central Hub               : X[−7,7]    Z[−7,7]    Y=3.5
//   Zone 2  — North Fortress            : X[−22,22]  Z[−38,−20] Y=4
//   Zone 3  — East Elevated Walkway     : X[18,40]   Z[−15,15]  Y=3
//   Zone 4  — West Raised Terraces      : X[−40,−18] Z[−15,5]   Y=2/4
//   Zone 5  — Cover Walls               : waist-height scatter
//   Zone 6  — Mid-Arena Pillars         : 8 pillars ringing hub, Y=5–7
//   Zone 7  — Corner Towers             : SW + SE, Y=7
//   Zone 8  — Perimeter Catwalks        : all 4 walls, Y=5
//   Zone 9  — Inner Floating Platforms  : Y=5.5 near hub
//   Zone 10 — Central Sky Platform      : X[−9,9]    Z[−9,9]    Y=10
//   Zone 11 — Sky Bridges               : thin pads at Y=8 in 4 directions
//   Zone 12 — Corner Sky Pads           : 4 large pads at Y=9, ±30 diagonal
//   Zone 13 — Far North Expansion       : bastion at Z−47..−60, mega towers
//   Zone 14 — Far South Platform        : Z[47,62]  Y=4, sky shelf above
//   Zone 15 — Far East/West Sky Shelves : X±50..63  Y=7
//   Zone 16 — Cardinal Spires           : 4 tall thin pillars at Y=12
//
// JUMP HEIGHTS: single ~1.5 m, double ~3.0 m, grapple unlimited
// CEILING: Y=14. Keep all geometry below Y=13.
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

inline LevelData buildLevel() {
    LevelData level;
    Room r;
    r.unlocked = true;

    r.walls = {
        // =====================================================================
        // ZONE 0 — PERIMETER WALLS (0-3)  130×130 arena, 14 m tall
        // =====================================================================
        { AABB{{-65.f, 0.f,-66.f},{ 65.f,14.f,-65.f}} }, // 0  North wall
        { AABB{{-65.f, 0.f, 65.f},{ 65.f,14.f, 66.f}} }, // 1  South wall
        { AABB{{-66.f, 0.f,-65.f},{-65.f,14.f, 65.f}} }, // 2  West wall
        { AABB{{ 65.f, 0.f,-65.f},{ 66.f,14.f, 65.f}} }, // 3  East wall

        // =====================================================================
        // ZONE 1 — CENTRAL HUB (4-8)
        // =====================================================================
        { AABB{{ -7.f, 0.f, -7.f},{  7.f, 3.5f,  7.f}} }, // 4  Hub
        { AABB{{ -4.f, 0.f,  7.f},{  4.f, 1.0f, 10.f}} }, // 5  South riser
        { AABB{{ -4.f, 0.f,-10.f},{  4.f, 1.0f, -7.f}} }, // 6  North riser
        { AABB{{  7.f, 0.f, -4.f},{ 10.f, 1.0f,  4.f}} }, // 7  East riser
        { AABB{{-10.f, 0.f, -4.f},{ -7.f, 1.0f,  4.f}} }, // 8  West riser

        // =====================================================================
        // ZONE 2 — NORTH FORTRESS (9-12)
        // =====================================================================
        { AABB{{-22.f, 0.f,-38.f},{ 22.f, 4.f,-20.f}} }, // 9  Fortress floor
        { AABB{{-10.f, 0.f,-20.f},{ 10.f, 2.f,-16.f}} }, // 10 South approach riser
        { AABB{{-22.f, 0.f,-38.f},{-18.f, 9.f,-34.f}} }, // 11 NW tower
        { AABB{{ 18.f, 0.f,-38.f},{  22.f, 9.f,-34.f}} }, // 12 NE tower

        // =====================================================================
        // ZONE 3 — EAST ELEVATED WALKWAY (13-16)
        // =====================================================================
        { AABB{{ 18.f, 0.f,-15.f},{ 40.f, 3.f, 15.f}} }, // 13 East walkway
        { AABB{{ 14.f, 0.f, -6.f},{ 18.f, 1.5f, 6.f}} }, // 14 Access step
        { AABB{{ 37.f, 0.f,-15.f},{ 40.f, 6.f,-11.f}} }, // 15 North support column
        { AABB{{ 37.f, 0.f, 11.f},{ 40.f, 6.f, 15.f}} }, // 16 South support column

        // =====================================================================
        // ZONE 4 — WEST RAISED TERRACES (17-19)
        // =====================================================================
        { AABB{{-40.f, 0.f, -5.f},{-18.f, 2.f,  5.f}} }, // 17 West platform A
        { AABB{{-40.f, 0.f,-15.f},{-28.f, 4.f, -7.f}} }, // 18 West platform B
        { AABB{{-40.f, 0.f,-24.f},{-34.f, 8.f,-18.f}} }, // 19 West tall tower

        // =====================================================================
        // ZONE 5 — COVER WALLS (20-29)
        // =====================================================================
        { AABB{{-13.f, 0.f,-18.f},{ -9.f, 1.4f,-15.f}} }, // 20 NW L-shape A
        { AABB{{-13.f, 0.f,-15.f},{-10.f, 1.4f,-12.f}} }, // 21 NW L-shape B
        { AABB{{  9.f, 0.f,-18.f},{ 13.f, 1.4f,-15.f}} }, // 22 NE L-shape A
        { AABB{{  9.f, 0.f,-15.f},{ 12.f, 1.4f,-12.f}} }, // 23 NE L-shape B
        { AABB{{-18.f, 0.f, 20.f},{-14.f, 1.4f, 24.f}} }, // 24 SW L-shape A
        { AABB{{-18.f, 0.f, 24.f},{-15.f, 1.4f, 27.f}} }, // 25 SW L-shape B
        { AABB{{ 14.f, 0.f, 20.f},{ 18.f, 1.4f, 24.f}} }, // 26 SE L-shape A
        { AABB{{ 14.f, 0.f, 24.f},{ 15.f, 1.4f, 27.f}} }, // 27 SE L-shape B
        { AABB{{ -5.f, 0.f, 25.f},{  5.f, 1.4f, 28.f}} }, // 28 Center cover A
        { AABB{{ -7.f, 0.f, 31.f},{  7.f, 1.4f, 33.f}} }, // 29 Center cover B

        // =====================================================================
        // ZONE 6 — MID-ARENA PILLARS (30-37)
        // =====================================================================
        { AABB{{ -1.f, 0.f,-17.f},{  1.f, 7.f,-15.f}} }, // 30 N pillar
        { AABB{{ -1.f, 0.f, 15.f},{  1.f, 7.f, 17.f}} }, // 31 S pillar
        { AABB{{-17.f, 0.f, -1.f},{-15.f, 7.f,  1.f}} }, // 32 W pillar
        { AABB{{ 15.f, 0.f, -1.f},{ 17.f, 7.f,  1.f}} }, // 33 E pillar
        { AABB{{-17.f, 0.f,-17.f},{-14.f, 5.f,-14.f}} }, // 34 NW diag pillar
        { AABB{{ 14.f, 0.f,-17.f},{ 17.f, 5.f,-14.f}} }, // 35 NE diag pillar
        { AABB{{-17.f, 0.f, 14.f},{-14.f, 5.f, 17.f}} }, // 36 SW diag pillar
        { AABB{{ 14.f, 0.f, 14.f},{ 17.f, 5.f, 17.f}} }, // 37 SE diag pillar

        // =====================================================================
        // ZONE 7 — CORNER TOWERS (38-39)
        // =====================================================================
        { AABB{{-40.f, 0.f, 36.f},{-36.f, 7.f, 40.f}} }, // 38 SW corner tower
        { AABB{{ 36.f, 0.f, 36.f},{ 40.f, 7.f, 40.f}} }, // 39 SE corner tower

        // =====================================================================
        // ZONE 8 — PERIMETER CATWALKS (40-43)
        // =====================================================================
        { AABB{{-40.f, 5.f,-40.f},{-37.f, 5.3f, 40.f}} }, // 40 West catwalk
        { AABB{{ 37.f, 5.f,-40.f},{ 40.f, 5.3f, 40.f}} }, // 41 East catwalk
        { AABB{{-37.f, 5.f,-40.f},{ 37.f, 5.3f,-37.f}} }, // 42 North catwalk
        { AABB{{-37.f, 5.f, 37.f},{ 37.f, 5.3f, 40.f}} }, // 43 South catwalk

        // =====================================================================
        // ZONE 9 — INNER FLOATING PLATFORMS (44-45)
        // =====================================================================
        { AABB{{ -4.f, 5.5f,-19.f},{  4.f, 5.8f,-16.f}} }, // 44 North hover pad
        { AABB{{ -4.f, 5.5f, 16.f},{  4.f, 5.8f, 19.f}} }, // 45 South hover pad

        // =====================================================================
        // ZONE 10 — CENTRAL SKY PLATFORM (46)
        // Large landing zone at Y=10. Only reachable via grapple or sky bridges.
        // =====================================================================
        { AABB{{ -9.f,10.0f, -9.f},{  9.f,10.3f,  9.f}} }, // 46 Central sky pad

        // =====================================================================
        // ZONE 11 — SKY BRIDGES (47-50)
        // Thin platforms at Y=8 radiating from the central sky pad.
        // Grapple to a pillar top, jump to the bridge, walk to the sky pad.
        // =====================================================================
        { AABB{{ -2.f, 8.0f,-20.f},{  2.f, 8.3f, -9.f}} }, // 47 North sky bridge
        { AABB{{ -2.f, 8.0f,  9.f},{  2.f, 8.3f, 20.f}} }, // 48 South sky bridge
        { AABB{{  9.f, 8.0f, -2.f},{ 20.f, 8.3f,  2.f}} }, // 49 East sky bridge
        { AABB{{-20.f, 8.0f, -2.f},{ -9.f, 8.3f,  2.f}} }, // 50 West sky bridge

        // =====================================================================
        // ZONE 12 — CORNER SKY PADS (51-54)
        // Wide floating platforms at Y=9 in the diagonal corners.
        // =====================================================================
        { AABB{{ 22.f, 9.0f,-38.f},{ 38.f, 9.3f,-22.f}} }, // 51 NE sky pad
        { AABB{{-38.f, 9.0f,-38.f},{-22.f, 9.3f,-22.f}} }, // 52 NW sky pad
        { AABB{{ 22.f, 9.0f, 22.f},{ 38.f, 9.3f, 38.f}} }, // 53 SE sky pad
        { AABB{{-38.f, 9.0f, 22.f},{-22.f, 9.3f, 38.f}} }, // 54 SW sky pad

        // =====================================================================
        // ZONE 13 — FAR NORTH EXPANSION (55-57)
        // Extended territory beyond the original north fortress.
        // Two mega-towers (Y=13) mark the far corners.
        // =====================================================================
        { AABB{{-20.f, 0.f,-60.f},{ 20.f, 5.f,-47.f}} }, // 55 Far north bastion
        { AABB{{-63.f, 0.f,-65.f},{-58.f,13.f,-58.f}} }, // 56 Far NW mega tower
        { AABB{{ 58.f, 0.f,-65.f},{ 63.f,13.f,-58.f}} }, // 57 Far NE mega tower

        // =====================================================================
        // ZONE 14 — FAR SOUTH PLATFORM (58-59)
        // Elevated slab at Y=4, with a smaller sky shelf above at Y=8.
        // =====================================================================
        { AABB{{-30.f, 0.f, 47.f},{ 30.f, 4.f, 62.f}} }, // 58 Far south slab
        { AABB{{ -8.f, 8.0f, 52.f},{  8.f, 8.3f, 60.f}} }, // 59 Far south sky shelf

        // =====================================================================
        // ZONE 15 — FAR EAST/WEST SKY SHELVES (60-61)
        // Long thin platforms at Y=7 hugging the far east/west walls.
        // Grapple from the mid-arena pillars or catwalks to reach them.
        // =====================================================================
        { AABB{{ 50.f, 7.0f,-20.f},{ 63.f, 7.3f, 20.f}} }, // 60 Far east sky shelf
        { AABB{{-63.f, 7.0f,-20.f},{-50.f, 7.3f, 20.f}} }, // 61 Far west sky shelf

        // =====================================================================
        // ZONE 16 — CARDINAL SPIRES (62-65)
        // Tall thin pillars at the mid-range cardinal points.
        // Excellent grapple anchors for crossing the expanded arena.
        // =====================================================================
        { AABB{{ -1.5f, 0.f,-52.f},{  1.5f,12.f,-50.f}} }, // 62 Far north spire
        { AABB{{ -1.5f, 0.f, 50.f},{  1.5f,12.f, 52.f}} }, // 63 Far south spire
        { AABB{{ 50.f, 0.f, -1.5f},{ 52.f,12.f,  1.5f}} }, // 64 Far east spire
        { AABB{{-52.f, 0.f, -1.5f},{-50.f,12.f,  1.5f}} }, // 65 Far west spire
    };

    // -------------------------------------------------------------------------
    // Enemy spawns.  Y > 5 → FLYER type (GameplayState detects this).
    // Player spawns at {0, 0, 32} (south open zone).
    // -------------------------------------------------------------------------
    r.enemySpawns = {
        // North fortress (on top of 4 m slab) — ground enemies
        {  0.f, 4.1f,-30.f },
        {-12.f, 4.1f,-28.f },
        { 12.f, 4.1f,-28.f },
        {  0.f, 4.1f,-36.f },
        // East walkway
        { 28.f, 3.1f,  0.f },
        { 28.f, 3.1f, -8.f },
        // West terraces
        {-30.f, 2.1f,  0.f },
        {-34.f, 4.1f,-11.f },
        // Mid-arena (ground)
        {-14.f, 0.f, -14.f },
        { 14.f, 0.f, -14.f },
        {  0.f, 0.f, -14.f },
        { 14.f, 0.f,   0.f },
        // South open zone
        {-18.f, 0.f,  18.f },
        { 18.f, 0.f,  18.f },
        {  0.f, 0.f,  20.f },
        // Far north bastion (ground)
        {  0.f, 5.1f,-53.f },
        {-12.f, 5.1f,-52.f },
        { 12.f, 5.1f,-52.f },
        // Far south slab (ground)
        {  0.f, 4.1f, 55.f },
        {-18.f, 4.1f, 55.f },
        { 18.f, 4.1f, 55.f },
        // Sky platform spawns — Y > 5 → GameplayState spawns FLYER here
        {  0.f, 10.2f,  0.f },   // Central sky pad
        { 30.f,  9.4f,-30.f },   // NE sky pad
        {-30.f,  9.4f,-30.f },   // NW sky pad
        { 30.f,  9.4f, 30.f },   // SE sky pad
        {-30.f,  9.4f, 30.f },   // SW sky pad
        { 56.f,  7.2f,  0.f },   // Far east shelf flyer
        {-56.f,  7.2f,  0.f },   // Far west shelf flyer
    };

    level.rooms.push_back(std::move(r));
    return level;
}
