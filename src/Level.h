#pragma once
// =============================================================================
// Level.h — Multi-room arena geometry + room/door management
// =============================================================================
//
// LAYOUT OVERVIEW (two massive rooms + corridor)
//
//  Room 0 (Starting Arena): X=-90..90  Z=-20..82   Y=0..16   (180x102m)
//  Corridor:                 X=-7..7   Z=-70..-20  Y=0..10   (14x50m)
//  Room 1 (Deep Compound):   X=-90..90 Z=-70..-230 Y=0..16   (180x160m)
//
//  Door (Room 0 wall, slides down when Room 0 cleared):
//    X=-7..7, Z=-71..-70, Y=0..14
//
//  Player spawns at {0, 0, 60} facing -Z (north).
//
// WALL COLORS: stored per-wall so buildWorldMesh can read wall.color directly.
// =============================================================================

#include "Player.h"
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Arena color palette — referenced when constructing walls in buildLevel()
// ---------------------------------------------------------------------------
namespace ArenaColor {
    constexpr glm::vec3 Floor    = {0.18f, 0.16f, 0.14f};
    constexpr glm::vec3 Ceiling  = {0.10f, 0.10f, 0.16f};
    constexpr glm::vec3 Perim    = {0.26f, 0.26f, 0.34f};
    constexpr glm::vec3 Hub      = {0.38f, 0.22f, 0.06f};
    constexpr glm::vec3 Riser    = {0.22f, 0.27f, 0.22f};
    constexpr glm::vec3 Corner   = {0.16f, 0.20f, 0.30f};
    constexpr glm::vec3 Ledge    = {0.24f, 0.24f, 0.30f};
    constexpr glm::vec3 Cover    = {0.34f, 0.26f, 0.16f};
    constexpr glm::vec3 Pillar   = {0.48f, 0.32f, 0.04f};
    constexpr glm::vec3 Catwalk  = {0.18f, 0.18f, 0.24f};
    constexpr glm::vec3 Tower    = {0.20f, 0.16f, 0.30f};
    constexpr glm::vec3 SkyPad   = {0.08f, 0.45f, 0.65f};
    constexpr glm::vec3 Spire    = {0.50f, 0.08f, 0.72f};
    constexpr glm::vec3 FarFort  = {0.32f, 0.10f, 0.10f};
    constexpr glm::vec3 Door     = {0.55f, 0.50f, 0.10f};
}

// Helper: make a Wall with colour
inline Wall W(AABB box, glm::vec3 col) { return {box, col}; }

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
                if (topY > 0.f) {
                    botY = std::max(botY - 5.f * dt, -6.f);
                    topY = std::max(topY - 5.f * dt,  0.f);
                }
            }
        }
    }

    bool anyDoorSliding() const {
        for (auto& room : rooms) {
            if (room.doorWallIndex >= 0 && room.cleared &&
                room.walls[room.doorWallIndex].box.max.y > 0.01f)
                return true;
        }
        return false;
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
// buildLevel() — constructs the full two-room layout.
// =============================================================================
inline LevelData buildLevel() {
    LevelData level;

    // =========================================================================
    // ROOM 0 — Starting Arena
    // Bounds: X=-90..90, Z=-20..82, Y=0..16
    // Player spawns at (0, 0, 60). All enemies here are spawned first.
    // =========================================================================
    {
        Room r;
        r.unlocked = true;

        // --- Perimeter walls -------------------------------------------------
        r.walls.push_back(W({{-90,0, 82},{ 90,16, 83}}, ArenaColor::Perim)); // South
        r.walls.push_back(W({{ 90,0,-20},{ 91,16, 83}}, ArenaColor::Perim)); // East
        r.walls.push_back(W({{-91,0,-20},{-90,16, 83}}, ArenaColor::Perim)); // West
        // North wall — split left/right to leave door gap at X=-7..7
        r.walls.push_back(W({{-90,0,-21},{-7, 16,-20}}, ArenaColor::Perim)); // North-left
        r.walls.push_back(W({{  7,0,-21},{ 90,16,-20}}, ArenaColor::Perim)); // North-right

        // --- Central Hub (raised 4m platform) --------------------------------
        r.walls.push_back(W({{-12,0, 0},{ 12,4, 20}}, ArenaColor::Hub));
        r.walls.push_back(W({{ -6,4, 4},{  6,7, 16}}, ArenaColor::Hub)); // upper tier

        // --- North Fortress (6m slab + two towers) ---------------------------
        r.walls.push_back(W({{-26,0,-14},{ 26,6,  2}}, ArenaColor::Corner)); // fortress floor
        r.walls.push_back(W({{-11,0,  2},{ 11,3,  8}}, ArenaColor::Riser));  // access riser
        r.walls.push_back(W({{-26,0,-14},{-21,12,-9}}, ArenaColor::Pillar)); // NW tower
        r.walls.push_back(W({{ 21,0,-14},{ 26,12,-9}}, ArenaColor::Pillar)); // NE tower

        // --- East Elevated Walkway (4m high) ---------------------------------
        r.walls.push_back(W({{ 28,0,-8},{ 82,4, 12}}, ArenaColor::Ledge));
        r.walls.push_back(W({{ 20,0,-5},{ 28,2,  9}}, ArenaColor::Riser)); // access step
        r.walls.push_back(W({{ 76,4,-8},{ 82,10,-4}}, ArenaColor::Pillar)); // E support N
        r.walls.push_back(W({{ 76,4, 8},{ 82,10, 12}}, ArenaColor::Pillar)); // E support S

        // --- West Raised Platforms -------------------------------------------
        r.walls.push_back(W({{-82,0,-10},{-28,4, 10}}, ArenaColor::Corner));
        r.walls.push_back(W({{-28,0, -6},{-20,2,  8}}, ArenaColor::Riser));
        r.walls.push_back(W({{-82,4,-10},{-76,10,-4}}, ArenaColor::Pillar)); // W pillar N
        r.walls.push_back(W({{-82,4,  8},{-76,10, 12}}, ArenaColor::Pillar)); // W pillar S

        // --- SE Compound (5m platform + tower) -------------------------------
        r.walls.push_back(W({{ 38,0, 38},{ 82,5, 78}}, ArenaColor::Corner));
        r.walls.push_back(W({{ 74,5, 38},{ 82,13,52}}, ArenaColor::Pillar));

        // --- SW Area (4m platform + tower) -----------------------------------
        r.walls.push_back(W({{-82,0, 40},{-38,4, 78}}, ArenaColor::Corner));
        r.walls.push_back(W({{-82,4, 40},{-74,12,54}}, ArenaColor::Tower));

        // --- Far NE / NW mega towers (touch perimeter) -----------------------
        r.walls.push_back(W({{ 82,0,-20},{ 90,14,-12}}, ArenaColor::Pillar));
        r.walls.push_back(W({{-90,0,-20},{-82,14,-12}}, ArenaColor::Pillar));

        // --- Mid-arena pillars (8 tall) --------------------------------------
        r.walls.push_back(W({{ -2,0,-15},{  2, 9,-13}}, ArenaColor::Pillar)); // N
        r.walls.push_back(W({{ -2,0, 22},{  2, 9, 24}}, ArenaColor::Pillar)); // S
        r.walls.push_back(W({{ 19,0,  2},{ 21, 9,  4}}, ArenaColor::Pillar)); // E
        r.walls.push_back(W({{-21,0,  2},{-19, 9,  4}}, ArenaColor::Pillar)); // W
        r.walls.push_back(W({{ 15,0,-11},{ 17, 7, -9}}, ArenaColor::Pillar)); // NE
        r.walls.push_back(W({{-17,0,-11},{-15, 7, -9}}, ArenaColor::Pillar)); // NW
        r.walls.push_back(W({{ 15,0, 21},{ 17, 7, 23}}, ArenaColor::Pillar)); // SE diag
        r.walls.push_back(W({{-17,0, 21},{-15, 7, 23}}, ArenaColor::Pillar)); // SW diag

        // --- Floating catwalks (Y=7) -----------------------------------------
        r.walls.push_back(W({{-28,7,-13},{ 28,7.4f,-11}}, ArenaColor::Catwalk)); // N catwalk
        r.walls.push_back(W({{ 12,7, -3},{ 30,7.4f,  1}}, ArenaColor::Catwalk)); // E catwalk
        r.walls.push_back(W({{-30,7, -3},{-12,7.4f,  1}}, ArenaColor::Catwalk)); // W catwalk

        // --- Central sky pad + bridges (Y=11) --------------------------------
        r.walls.push_back(W({{ -8,11,  2},{  8,11.4f,16}}, ArenaColor::SkyPad));
        r.walls.push_back(W({{ -2,8.5f,-12},{  2, 8.9f, 2}}, ArenaColor::SkyPad)); // N bridge
        r.walls.push_back(W({{ -2,8.5f, 16},{  2, 8.9f,22}}, ArenaColor::SkyPad)); // S bridge

        // --- Cover walls (waist-high) ----------------------------------------
        r.walls.push_back(W({{ -8,0, 30},{  8,1.5f,32}}, ArenaColor::Cover));
        r.walls.push_back(W({{-16,0, 28},{-12,1.5f,31}}, ArenaColor::Cover));
        r.walls.push_back(W({{ 12,0, 28},{ 16,1.5f,31}}, ArenaColor::Cover));
        r.walls.push_back(W({{-18,0,  8},{-15,1.4f,12}}, ArenaColor::Cover));
        r.walls.push_back(W({{ 15,0,  8},{ 18,1.4f,12}}, ArenaColor::Cover));
        r.walls.push_back(W({{ -6,0, 46},{  6,1.5f,48}}, ArenaColor::Cover));
        r.walls.push_back(W({{-40,0, 25},{-36,1.5f,29}}, ArenaColor::Cover));
        r.walls.push_back(W({{ 36,0, 25},{ 40,1.5f,29}}, ArenaColor::Cover));

        // --- Corridor side walls and ceiling ---------------------------------
        r.walls.push_back(W({{ -7.5f,0,-70},{ -7, 10,-20}}, ArenaColor::Perim)); // W corridor
        r.walls.push_back(W({{  7,   0,-70},{7.5f,10,-20}}, ArenaColor::Perim)); // E corridor
        r.walls.push_back(W({{ -7.5f,10,-70},{7.5f,10.3f,-20}}, ArenaColor::Ceiling)); // ceiling

        // --- DOOR (doorWallIndex = last wall added to Room 0) ----------------
        // Blocks corridor exit into Room 1 until all Room 0 enemies are dead.
        r.walls.push_back(W({{ -7,0,-71},{  7,14,-70}}, ArenaColor::Door));
        r.doorWallIndex = (int)r.walls.size() - 1;

        // --- Enemy spawn points ----------------------------------------------
        // Y <= 5 → ground enemy, Y > 5 → FLYER
        r.enemySpawns = {
            // Open south area
            {  0, 0.1f, 50 }, { -22, 0.1f, 45 }, { 22, 0.1f, 45 },
            {  0, 0.1f, 35 }, { -30, 0.1f, 30 }, { 30, 0.1f, 30 },
            // West/east platforms
            { -50, 2.1f,  0 }, { 50, 2.1f,  0 },
            // North fortress (on 6m slab)
            {  0, 6.1f,-8 }, { -16, 6.1f,-8 }, { 16, 6.1f,-8 },
            // SE/SW compounds
            { 58, 3.1f, 55 }, { -58, 3.1f, 58 },
            // Mid arena ground
            { -22, 0.1f, 12 }, { 22, 0.1f, 12 },
            // Flyiers on high platforms
            {  0, 11.3f, 8 },        // central sky pad
            { 28, 7.5f, -12 },       // N catwalk height
            {-28, 7.5f, -12 },       // N catwalk height
            { 60, 5.2f, 57 },        // SE compound top
        };

        level.rooms.push_back(std::move(r));
    }

    // =========================================================================
    // ROOM 1 — Deep Compound
    // Bounds: X=-90..90, Z=-70..-230, Y=0..16
    // Accessed after Room 0 door opens. Much larger, more vertical.
    // =========================================================================
    {
        Room r;
        r.unlocked = false;

        // --- Perimeter walls -------------------------------------------------
        // South wall has a gap at X=-7..7 for the corridor opening
        r.walls.push_back(W({{-90,0,-71},{-7.5f,16,-70}}, ArenaColor::Perim)); // South-left
        r.walls.push_back(W({{ 7.5f,0,-71},{ 90,16,-70}}, ArenaColor::Perim)); // South-right
        r.walls.push_back(W({{-90,0,-231},{ 90,16,-230}}, ArenaColor::Perim)); // North
        r.walls.push_back(W({{ 90,0,-231},{ 91,16,-70}}, ArenaColor::Perim)); // East
        r.walls.push_back(W({{-91,0,-231},{-90,16,-70}}, ArenaColor::Perim)); // West

        // --- Central Fortress (multi-tiered) ---------------------------------
        r.walls.push_back(W({{-20,0,-152},{ 20,7,-118}}, ArenaColor::FarFort)); // base
        r.walls.push_back(W({{-13,7,-150},{ 13,11,-120}}, ArenaColor::Corner)); // upper level
        r.walls.push_back(W({{-20,7,-152},{-16,14,-148}}, ArenaColor::Pillar)); // NW battlement
        r.walls.push_back(W({{ 16,7,-152},{ 20,14,-148}}, ArenaColor::Pillar)); // NE battlement
        r.walls.push_back(W({{-20,7,-122},{-16,14,-118}}, ArenaColor::Pillar)); // SW battlement
        r.walls.push_back(W({{ 16,7,-122},{ 20,14,-118}}, ArenaColor::Pillar)); // SE battlement

        // --- Fortress approach riser ----------------------------------------
        r.walls.push_back(W({{-10,0,-118},{ 10,3,-110}}, ArenaColor::Riser));

        // --- East Compound ---------------------------------------------------
        r.walls.push_back(W({{ 32,0,-202},{ 82,5,-153}}, ArenaColor::Corner));
        r.walls.push_back(W({{ 76,5,-202},{ 82,14,-192}}, ArenaColor::Pillar)); // tower

        // --- West Compound ---------------------------------------------------
        r.walls.push_back(W({{-82,0,-202},{-32,5,-153}}, ArenaColor::Corner));
        r.walls.push_back(W({{-82,5,-202},{-76,14,-192}}, ArenaColor::Pillar)); // tower

        // --- North Bunker ----------------------------------------------------
        r.walls.push_back(W({{-24,0,-227},{ 24,8,-210}}, ArenaColor::FarFort));
        r.walls.push_back(W({{-14,0,-210},{ 14,4,-204}}, ArenaColor::Riser)); // access

        // --- Far corner mega towers ------------------------------------------
        r.walls.push_back(W({{ 80,0,-231},{ 90,14,-220}}, ArenaColor::Pillar)); // NE
        r.walls.push_back(W({{-90,0,-231},{-80,14,-220}}, ArenaColor::Pillar)); // NW
        r.walls.push_back(W({{ 80,0,-81},{  90,12,-71}}, ArenaColor::Pillar)); // SE (near door)
        r.walls.push_back(W({{-90,0,-81},{ -80,12,-71}}, ArenaColor::Pillar)); // SW (near door)

        // --- Mid-arena pillars (6) -------------------------------------------
        r.walls.push_back(W({{ -2,0,-162},{  2,10,-160}}, ArenaColor::Pillar));
        r.walls.push_back(W({{ -2,0,-108},{  2,10,-106}}, ArenaColor::Pillar));
        r.walls.push_back(W({{ 26,0,-136},{ 28,10,-134}}, ArenaColor::Pillar));
        r.walls.push_back(W({{-28,0,-136},{-26,10,-134}}, ArenaColor::Pillar));
        r.walls.push_back(W({{ 26,0,-172},{ 28, 8,-170}}, ArenaColor::Pillar));
        r.walls.push_back(W({{-28,0,-172},{-26, 8,-170}}, ArenaColor::Pillar));

        // --- Elevated catwalks -----------------------------------------------
        r.walls.push_back(W({{-28,8,-157},{ 28,8.4f,-155}}, ArenaColor::Catwalk)); // N fortress catwalk
        r.walls.push_back(W({{ 22,7,-142},{ 40,7.4f,-138}}, ArenaColor::Catwalk)); // E mid
        r.walls.push_back(W({{-40,7,-142},{-22,7.4f,-138}}, ArenaColor::Catwalk)); // W mid
        r.walls.push_back(W({{-28,7,-117},{ 28,7.4f,-115}}, ArenaColor::Catwalk)); // S fortress

        // --- Sky platforms ---------------------------------------------------
        r.walls.push_back(W({{ -9,11,-152},{  9,11.4f,-118}}, ArenaColor::SkyPad)); // above fortress
        r.walls.push_back(W({{-32,9,-183},{ 32,9.4f,-165}}, ArenaColor::SkyPad));   // mid sky

        // --- Cover walls -----------------------------------------------------
        r.walls.push_back(W({{ -8,0,-172},{  8,1.5f,-170}}, ArenaColor::Cover));
        r.walls.push_back(W({{-18,0,-170},{-14,1.5f,-167}}, ArenaColor::Cover));
        r.walls.push_back(W({{ 14,0,-170},{ 18,1.5f,-167}}, ArenaColor::Cover));
        r.walls.push_back(W({{ -8,0,-100},{  8,1.5f,-98}}, ArenaColor::Cover));
        r.walls.push_back(W({{-18,0,-100},{-14,1.5f,-97}}, ArenaColor::Cover));
        r.walls.push_back(W({{ 14,0,-100},{ 18,1.5f,-97}}, ArenaColor::Cover));
        r.walls.push_back(W({{ -6,0,-215},{  6,1.5f,-213}}, ArenaColor::Cover));
        r.walls.push_back(W({{ 37,0,-177},{ 40,1.5f,-174}}, ArenaColor::Cover));
        r.walls.push_back(W({{-40,0,-177},{-37,1.5f,-174}}, ArenaColor::Cover));

        // --- Enemy spawns ----------------------------------------------------
        r.enemySpawns = {
            // Entry zone (just inside room 1)
            {  0, 0.1f, -85 }, { -25, 0.1f, -90 }, { 25, 0.1f, -90 },
            // Mid zone around fortress
            {  0, 7.1f,-135 }, { -14, 7.1f,-140 }, { 14, 7.1f,-140 }, // on fortress
            {  0,11.3f,-135 },                                          // fortress sky pad (FLYER)
            { -40, 0.1f,-130 }, { 40, 0.1f,-130 },                     // flanks
            // East/West compounds
            { 55, 3.1f,-175 }, { 55, 3.1f,-185 },
            {-55, 3.1f,-175 }, {-55, 3.1f,-185 },
            // North bunker
            {  0, 5.1f,-218 }, { -14, 5.1f,-215 }, { 14, 5.1f,-215 },
            // Flyiers
            { 25, 9.5f,-174 }, {-25, 9.5f,-174 },                     // mid sky
            {  0, 7.5f,-200 },                                         // far zone
        };

        level.rooms.push_back(std::move(r));
    }

    return level;
}
