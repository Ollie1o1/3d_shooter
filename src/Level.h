#pragma once
// =============================================================================
// Level.h — the three arenas, the corridors between them, and their moods.
//
//          +Z (south)
//   ┌─────────────────┐  ARENA 1  SUNSET YARD   X ±30  Z  -30..30   open sky
//   │   player start  │           adobe walls, side platforms, a sun obelisk
//   └──────┐ ┌────────┘
//          │ │          corridor  Z -46..-31
//   ┌──────┘ └────────┐  ARENA 2  THE FOUNDRY   X ±32  Z -110..-46  roofed
//   │  furnace, lava  │           catwalks, lava channels, furnace you can climb
//   └──────┐ ┌────────┘
//          │ │          corridor  Z -126..-111
//   ┌──────┘ └────────┐  ARENA 3  THE CORE      X ±36  Z -198..-126 night sky
//   │ reactor + boss  │           pillar ring, corner perches, the Warden
//   └─────────────────┘
//          -Z (north)
//
// Each arena: three waves. Clearing the last opens the exit door; walking far
// enough into the next arena slams the gate shut behind you and starts it.
//
// Everything here is plain data (no OpenGL), so tests can check that spawn
// points aren't inside walls, pads land on their platforms, and so on.
// =============================================================================
#include "Player.h"
#include "Enemy.h"
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

// Lighting, fog and sky for one arena. Blended across corridors.
struct Theme {
    glm::vec3 zenith, horizon, ground;        // sky gradient
    glm::vec3 sunDir, sunColor;
    float     sunSize, sunStripes;            // radians; 1 = synthwave bands
    glm::vec3 mountain;
    float     stars;
    glm::vec3 lightDir, lightColor;           // key light (direction light travels)
    glm::vec3 skyAmb, groundAmb;              // hemisphere ambient
    glm::vec3 fogColor;
    float     fogDensity;
};

inline Theme lerpTheme(const Theme& a, const Theme& b, float t) {
    auto m = [t](glm::vec3 x, glm::vec3 y) { return glm::mix(x, y, t); };
    auto f = [t](float x, float y) { return x + (y - x) * t; };
    Theme o;
    o.zenith = m(a.zenith, b.zenith);   o.horizon = m(a.horizon, b.horizon); o.ground = m(a.ground, b.ground);
    o.sunDir = glm::normalize(m(a.sunDir, b.sunDir));  o.sunColor = m(a.sunColor, b.sunColor);
    o.sunSize = f(a.sunSize, b.sunSize); o.sunStripes = f(a.sunStripes, b.sunStripes);
    o.mountain = m(a.mountain, b.mountain); o.stars = f(a.stars, b.stars);
    o.lightDir = glm::normalize(m(a.lightDir, b.lightDir)); o.lightColor = m(a.lightColor, b.lightColor);
    o.skyAmb = m(a.skyAmb, b.skyAmb); o.groundAmb = m(a.groundAmb, b.groundAmb);
    o.fogColor = m(a.fogColor, b.fogColor); o.fogDensity = f(a.fogDensity, b.fogDensity);
    return o;
}

struct WaveEntry { EnemyType type; int count; };

// A door is a wall that slides into the floor when opened.
struct Door {
    int       wall;          // index into LevelData::walls
    float     height;        // closed height (top of the door)
    bool      open = false;
    float     openAmount = 0.f;  // 0 closed .. 1 fully sunk; animated by GameplayState
};

struct JumpPad {
    glm::vec3 centre;        // on the floor
    glm::vec2 half;          // XZ half-extents of the trigger
    glm::vec3 launch;        // velocity applied to the player
};

struct Hazard { AABB box; float dps; };

struct FloorPatch { float x0, z0, x1, z1, y; glm::vec3 color; };

struct Arena {
    const char* name;
    const char* subtitle;
    AABB        bounds;          // interior; enemies are clamped inside, max.y caps flyers
    AABB        zone;            // where the player may be (XZ), includes the wall tops
    glm::vec3   playerStart;
    std::vector<glm::vec3> groundSpawns, airSpawns;
    glm::vec3   bossSpawn{0.f};
    std::vector<std::vector<WaveEntry>> waves;
    int         maxAlive = 8;    // concurrent enemies; the rest trickle in as you kill
    float       damageScale = 1.f;  // enemy damage multiplier: the first arena is forgiving
    int         entryGate = -1;  // door behind you once you're in (index into doors)
    int         exitDoor  = -1;
    Theme       theme;
};

struct LevelData {
    std::vector<Wall>       walls;     // collidable (doors included)
    std::vector<Wall>       props;     // visual only
    std::vector<Wall>       neon;      // visual only, self-lit (drawn with vertex glow)
    std::vector<FloorPatch> floors;
    std::vector<Door>       doors;
    std::vector<JumpPad>    pads;
    std::vector<Hazard>     hazards;
    std::vector<Arena>      arenas;
    std::vector<AABB>       corridors; // corridor i joins arena i and i+1 (zone, XZ)

    bool isDoorWall(int w) const {
        for (auto& d : doors) if (d.wall == w) return true;
        return false;
    }

    // Arena whose zone contains p (or -1 if p is in a corridor / outside).
    int arenaAt(glm::vec3 p) const {
        for (int i = 0; i < (int)arenas.size(); ++i) {
            const AABB& z = arenas[i].zone;
            if (p.x >= z.min.x && p.x <= z.max.x && p.z >= z.min.z && p.z <= z.max.z) return i;
        }
        return -1;
    }

    // Lighting for a position: an arena's own theme, or a blend while walking
    // down the corridor between two arenas.
    Theme themeAt(glm::vec3 p) const {
        for (int i = 0; i < (int)corridors.size() && i + 1 < (int)arenas.size(); ++i) {
            const AABB& c = corridors[i];
            if (p.z <= c.max.z && p.z >= c.min.z && p.x >= c.min.x - 2.f && p.x <= c.max.x + 2.f) {
                float t = (c.max.z - p.z) / (c.max.z - c.min.z);
                t = t * t * (3.f - 2.f * t);
                return lerpTheme(arenas[i].theme, arenas[i + 1].theme, t);
            }
        }
        int a = arenaAt(p);
        if (a >= 0) return arenas[a].theme;
        // Outside every zone (shouldn't happen): use the nearest arena by Z
        int best = 0; float bestD = 1e9f;
        for (int i = 0; i < (int)arenas.size(); ++i) {
            float cz = (arenas[i].zone.min.z + arenas[i].zone.max.z) * 0.5f;
            if (std::fabs(cz - p.z) < bestD) { bestD = std::fabs(cz - p.z); best = i; }
        }
        return arenas[best].theme;
    }
};

// =============================================================================
// buildLevel()
// =============================================================================
inline LevelData buildLevel() {
    using glm::vec3;
    LevelData L;

    auto aabb = [](float x0, float y0, float z0, float x1, float y1, float z1) {
        return AABB{ {std::min(x0,x1), std::min(y0,y1), std::min(z0,z1)},
                     {std::max(x0,x1), std::max(y0,y1), std::max(z0,z1)} };
    };
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) {
        L.walls.push_back(Wall{aabb(x0,y0,z0,x1,y1,z1), c});
        return (int)L.walls.size() - 1;
    };
    auto prop = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) {
        L.props.push_back(Wall{aabb(x0,y0,z0,x1,y1,z1), c});
    };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) {
        L.neon.push_back(Wall{aabb(x0,y0,z0,x1,y1,z1), c});
    };
    auto door = [&](float x0, float z0, float x1, float z1, float h, vec3 c, bool open) {
        int w = wall(x0, 0.f, z0, x1, h, z1, c);
        Door d; d.wall = w; d.height = h; d.open = open; d.openAmount = open ? 1.f : 0.f;
        if (open) { L.walls[w].box.max.y = 0.f; L.walls[w].box.min.y = -h; }
        L.doors.push_back(d);
        return (int)L.doors.size() - 1;
    };
    // A band of light wrapped around a box (slightly larger, so only its sides show)
    auto ring = [&](float x0, float z0, float x1, float z1, float y0, float y1, vec3 c) {
        neon(x0 - 0.06f, y0, z0 - 0.06f, x1 + 0.06f, y1, z1 + 0.06f, c);
    };
    // A blocky synthwave palm: a leaning trunk and drooping fronds
    auto palm = [&](float x, float z, float h, float lean) {
        vec3 trunk{0.20f, 0.09f, 0.13f}, leaf{0.09f, 0.04f, 0.10f};
        int segs = (int)(h / 1.5f);
        float tx = x;
        for (int i = 0; i < segs; ++i) {
            prop(tx - 0.3f, i * 1.5f, z - 0.3f, tx + 0.3f, (i + 1) * 1.5f, z + 0.3f, trunk);
            tx += lean * (0.1f + 0.05f * i);
        }
        float top = segs * 1.5f;
        prop(tx - 0.6f, top - 0.2f, z - 0.6f, tx + 0.6f, top + 0.4f, z + 0.6f, leaf);
        for (int s = -1; s <= 1; s += 2) {
            prop(tx, top, z - 0.35f, tx + s * 2.6f, top + 0.2f, z + 0.35f, leaf);
            prop(tx + s * 2.4f, top - 1.0f, z - 0.3f, tx + s * 3.6f, top, z + 0.3f, leaf);
            prop(tx - 0.35f, top, z, tx + 0.35f, top + 0.2f, z + s * 2.6f, leaf);
            prop(tx - 0.3f, top - 1.0f, z + s * 2.4f, tx + 0.3f, top, z + s * 3.6f, leaf);
        }
    };

    // Ground outside the arenas (seen from wall tops), just under the arena floors
    L.floors.push_back({-100.f, -240.f, 100.f, 100.f, -0.3f, {0.12f, 0.07f, 0.08f}});

    // =========================================================================
    // ARENA 1 — SUNSET YARD
    // =========================================================================
    {
        vec3 adobe{0.66f,0.40f,0.30f}, adobeDark{0.50f,0.28f,0.22f}, stone{0.56f,0.48f,0.43f},
             crate{0.47f,0.31f,0.21f}, pink{1.0f,0.25f,0.62f}, cyan{0.2f,0.9f,1.0f};
        Arena a;
        a.name = "SUNSET YARD";
        a.subtitle = "SURVIVE 3 WAVES";
        a.bounds = aabb(-30, 0, -30, 30, 12, 30);
        a.zone   = aabb(-31.5f, 0, -31.5f, 31.5f, 40, 31.5f);
        a.playerStart = {0.f, 0.f, 24.f};
        L.floors.push_back({-31.f, -31.f, 31.f, 31.f, 0.f, {0.62f, 0.46f, 0.34f}});

        // Perimeter (low, so the sun shows over it)
        wall(-31,0, 30,  31,5, 31, adobe);
        wall( 30,0,-31,  31,5, 31, adobe);
        wall(-31,0,-31, -30,5, 31, adobe);
        wall(-31,0,-31,  -4,5,-30, adobe);
        wall(  4,0,-31,  31,5,-30, adobe);
        a.exitDoor = door(-4, -30.9f, 4, -30.1f, 5.f, {0.25f,0.22f,0.26f}, false);
        // Gate frame
        wall(-5.5f,0,-31.5f, -4,7.5f,-29.5f, adobeDark);
        wall(  4,0,-31.5f, 5.5f,7.5f,-29.5f, adobeDark);
        wall(-5.5f,5,-31.5f, 5.5f,7.5f,-29.5f, adobeDark);
        neon(-4.9f,0.2f,-29.5f, -4.6f,7.3f,-29.38f, cyan);
        neon( 4.6f,0.2f,-29.5f,  4.9f,7.3f,-29.38f, cyan);
        neon(-5.5f,6.0f,-29.5f,  5.5f,6.3f,-29.38f, cyan);
        // Neon band around the inside of the perimeter
        neon(-30,4.3f, 29.86f,  30,4.5f, 29.98f, pink);
        neon( 29.86f,4.3f,-30,  29.98f,4.5f, 30, pink);
        neon(-29.98f,4.3f,-30, -29.86f,4.5f, 30, pink);
        neon(-30,4.3f,-29.98f, -5.5f,4.5f,-29.86f, pink);
        neon(5.5f,4.3f,-29.98f, 30,4.5f,-29.86f, pink);

        // Central dais + sun obelisk
        wall(-4,0,-4, 4,1,4, stone);
        ring(-4,-4, 4,4, 0.82f, 0.94f, cyan);
        wall(-1,1,-1, 1,7,1, adobeDark);
        ring(-1,-1, 1,1, 3.0f, 3.2f, cyan);
        ring(-1,-1, 1,1, 5.0f, 5.2f, cyan);
        neon(-0.6f,7,-0.6f, 0.6f,7.6f,0.6f, pink);

        // L-shaped waist-high cover around the middle
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            float x0 = sx * 9.f, x1 = sx * 14.f, zA = sz * 12.f, zB = sz * 13.f;
            wall(x0,0,zA, x1,1.3f,zB, stone);
            wall(sx * 13.f,0,sz * 8.f, x1,1.3f,zB, stone);
        }

        // Side platforms with steps and jump pads
        for (int s : {-1, 1}) {
            float in = s * 22.f, out = s * 30.f;
            wall(in,0,-12, out,3.5f,12, adobeDark);
            neon(in - s * 0.12f,3.15f,-12, in - s * 0.02f,3.35f,12, pink);
            wall(s * 18.f,0,8, s * 20.f,1.2f,12, stone);
            wall(s * 20.f,0,8, s * 22.f,2.4f,12, stone);
            wall(s * 25.f,3.5f,-7, s * 27.f,4.7f,-5, crate);
            wall(s * 25.f,3.5f, 3, s * 27.f,4.7f, 5, crate);
            L.pads.push_back({{s * 19.5f, 0.f, -2.f}, {1.4f, 1.4f}, {s * 5.5f, 15.5f, 0.f}});
        }

        // Corner towers with glowing caps
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            wall(sx * 26.f,0,sz * 26.f, sx * 30.f,8,sz * 30.f, adobe);
            neon(sx * 25.95f,8,sz * 25.95f, sx * 30.05f,8.3f,sz * 30.05f, pink);
        }

        // Crates
        wall(-6,0,18, -4,1.2f,20, crate);
        wall( 5,0,20,  7,1.2f,22, crate);
        wall(-20,0,20, -17.5f,1.6f,22.5f, crate);
        wall(17.5f,0,-22.5f, 20,1.6f,-20, crate);
        wall(-19,0,-22, -17,1.2f,-20, crate);
        wall(2,0,-20, 4,1.2f,-18, crate);

        // Palms outside the walls, silhouetted against the sunset
        palm(-37, -18, 10.f,  0.4f); palm(-38, 4, 8.f, -0.3f); palm(-36, 22, 11.f, 0.2f);
        palm( 37, -16, 11.f, -0.4f); palm( 38, 6, 9.f,  0.3f); palm( 36, 24, 8.f, -0.2f);
        palm(-16, -38, 9.f, 0.3f);   palm(14, -37, 11.f, -0.3f); palm(-28, -40, 12.f, 0.2f);
        palm(26, -41, 9.f, 0.2f);    palm(-12, 38, 9.f, 0.3f);   palm(14, 37, 10.f, -0.2f);

        a.groundSpawns = {{-16,0,-24},{16,0,-24},{0,0,-22},{-24,0,-20},{24,0,-20},
                          {-26,3.55f,0},{26,3.55f,0},{-24,0,20},{24,0,20},{-9,0,25},{9,0,25},{0,1.05f,-3}};
        a.airSpawns    = {{-15,8,-15},{15,8,-15},{0,9,-20},{-18,8,12},{18,8,12},{0,9,12}};
        a.waves = {
            {{EnemyType::HUSK, 4}},
            {{EnemyType::HUSK, 4}, {EnemyType::RIPPER, 3}},
            {{EnemyType::HUSK, 3}, {EnemyType::RIPPER, 3}, {EnemyType::RAPTOR, 3}},
        };
        a.maxAlive = 6;
        a.damageScale = 0.7f;
        a.theme = Theme{
            {0.12f,0.05f,0.22f}, {1.0f,0.48f,0.32f}, {0.16f,0.07f,0.10f},
            glm::normalize(vec3{0.f, 0.3f, -1.f}), {1.5f,0.62f,0.18f}, 0.2f, 1.f,
            {0.16f,0.06f,0.16f}, 0.35f,
            glm::normalize(vec3{0.25f,-0.45f,0.85f}), {1.15f,0.72f,0.5f},
            {0.34f,0.22f,0.36f}, {0.16f,0.09f,0.08f},
            {0.58f,0.30f,0.30f}, 0.006f };
        L.arenas.push_back(std::move(a));
    }

    // ---- Corridor 1 → 2 ------------------------------------------------------
    {
        vec3 c{0.22f,0.20f,0.24f}, cyan{0.2f,0.9f,1.0f};
        wall(-5,0,-46, -4,5,-31, c);
        wall( 4,0,-46,  5,5,-31, c);
        wall(-5,5,-46,  5,5.5f,-31, c);
        L.floors.push_back({-4.f, -46.f, 4.f, -31.f, 0.f, {0.15f,0.15f,0.18f}});
        neon(-3.9f,0,-45, -3.7f,0.05f,-32, cyan);
        neon( 3.7f,0,-45,  3.9f,0.05f,-32, cyan);
        neon(-0.3f,4.88f,-45.5f, 0.3f,4.98f,-31.5f, {0.45f,0.5f,0.6f});
        L.corridors.push_back(aabb(-5, 0, -46.5f, 5, 40, -29.5f));
    }

    // =========================================================================
    // ARENA 2 — THE FOUNDRY
    // =========================================================================
    {
        vec3 iron{0.32f,0.29f,0.28f}, rust{0.48f,0.25f,0.15f}, dark{0.18f,0.17f,0.17f},
             orange{1.0f,0.30f,0.04f}, lava{1.0f,0.24f,0.02f};
        Arena a;
        a.name = "THE FOUNDRY";
        a.subtitle = "SURVIVE 3 WAVES";
        a.bounds = aabb(-32, 0, -110, 32, 13, -46);
        a.zone   = aabb(-33.5f, 0, -111.5f, 33.5f, 40, -44.5f);
        a.playerStart = {0.f, 0.f, -50.f};
        L.floors.push_back({-33.f, -111.f, 33.f, -45.f, 0.f, {0.25f,0.23f,0.22f}});

        wall(-33,0,-46, -4,14,-45, iron);
        wall(  4,0,-46, 33,14,-45, iron);
        wall( -4,5,-46,  4,14,-45, iron);
        a.entryGate = door(-4, -45.9f, 4, -45.1f, 5.f, {0.25f,0.22f,0.22f}, true);
        wall( 32,0,-111, 33,14,-45, iron);
        wall(-33,0,-111,-32,14,-45, iron);
        wall(-33,0,-111, -4,14,-110, iron);
        wall(  4,0,-111, 33,14,-110, iron);
        wall( -4,5,-111,  4,14,-110, iron);
        a.exitDoor = door(-4, -110.9f, 4, -110.1f, 5.f, {0.25f,0.22f,0.22f}, false);
        wall(-33,14,-111, 33,15,-45, dark);                                    // roof
        for (float zf : {-46.12f, -109.88f}) {                                // door frames
            float zb = zf < -100.f ? zf - 0.1f : zf + 0.1f;
            neon(-4.6f,0,zf, -4.3f,5.3f,zb, orange);
            neon( 4.3f,0,zf,  4.6f,5.3f,zb, orange);
            neon(-4.6f,5.0f,zf, 4.6f,5.3f,zb, orange);
        }
        // Wall stripes at shoulder height, like hazard paint
        neon(-31.98f,2.0f,-110, -31.88f,2.2f,-46, orange);
        neon( 31.88f,2.0f,-110,  31.98f,2.2f,-46, orange);

        // Central furnace (climb it from the jump pad) with a chimney to the roof
        wall(-5,0,-83, 5,6,-73, rust);
        ring(-5,-83, 5,-73, 1.4f, 1.7f, orange);
        ring(-5,-83, 5,-73, 4.0f, 4.3f, orange);
        neon(-2.f,0.3f,-72.94f, 2.f,2.5f,-72.84f, lava);                      // furnace mouth
        wall(-2,6,-80, 2,14,-76, dark);
        ring(-2,-80, 2,-76, 9.0f, 9.3f, orange);
        ring(-2,-80, 2,-76, 12.0f, 12.3f, orange);
        L.pads.push_back({{0.f, 0.f, -68.5f}, {1.4f, 1.4f}, {0.f, 18.f, -5.f}});

        // Lava channels: jump them, or lure enemies through
        for (float z0 : {-62.f, -97.f}) {
            AABB lv = aabb(-26, 0, z0, 26, 0.06f, z0 + 3.f);
            L.hazards.push_back({lv, 30.f});
            neon(lv.min.x, lv.min.y, lv.min.z, lv.max.x, lv.max.y, lv.max.z, lava);
            prop(-26.4f,0,z0 - 0.4f, 26.4f,0.12f,z0, dark);                  // curbs
            prop(-26.4f,0,z0 + 3.f, 26.4f,0.12f,z0 + 3.4f, dark);
        }

        // Catwalks along the side walls, reached by jump pads
        for (int s : {-1, 1}) {
            float in = s * 24.f, out = s * 32.f;
            wall(in,4.6f,-100, out,5,-56, dark);
            neon(in - s * 0.12f,4.6f,-100, in - s * 0.02f,5.0f,-56, orange);
            for (float z : {-100.f, -78.5f, -57.f})
                wall(in,0,z, in + s * 1.f,4.6f,z + 1.f, iron);
            L.pads.push_back({{s * 20.5f, 0.f, -78.f}, {1.4f, 1.4f}, {s * 6.5f, 16.5f, 0.f}});
        }

        // Full-height pillars for cover
        for (int sx : {-1, 1}) for (float z : {-70.f, -90.f}) {
            wall(sx * 13.f,0,z, sx * 15.f,14,z + 2.f, iron);
            ring(std::min(sx * 13.f, sx * 15.f), z, std::max(sx * 13.f, sx * 15.f), z + 2.f, 3.0f, 3.25f, orange);
        }
        // Low cover
        wall(-10,0,-104, -4,1.2f,-102, rust);
        wall(  4,0,-104, 10,1.2f,-102, rust);
        wall(  4,0,-55,  10,1.2f,-53,  rust);
        wall(-10,0,-55,  -4,1.2f,-53,  rust);
        wall(-22,0,-84, -18,1.4f,-80,  iron);
        wall( 18,0,-84,  22,1.4f,-80,  iron);

        // Hanging chains from the roof (visual)
        for (float x : {-18.f, 18.f, -8.f, 8.f})
            for (float z : {-60.f, -100.f})
                prop(x - 0.08f, 9.f + std::fabs(x) * 0.05f, z - 0.08f, x + 0.08f, 14.f, z + 0.08f, dark);

        a.groundSpawns = {{-26,0,-104},{26,0,-104},{0,0,-104},{-20,0,-88},{20,0,-88},
                          {-28,5.05f,-90},{28,5.05f,-90},{-28,5.05f,-66},{28,5.05f,-66},
                          {-20,0,-68},{20,0,-68},{-26,0,-52},{26,0,-52},{0,6.05f,-82}};
        a.airSpawns    = {{-12,9,-78},{12,9,-78},{0,10,-95},{0,10,-60},{-20,10,-100},{20,10,-56}};
        a.waves = {
            {{EnemyType::HUSK, 4}, {EnemyType::MITE, 4}, {EnemyType::SENTINEL, 1}},
            {{EnemyType::BRUTE, 1}, {EnemyType::RIPPER, 4}, {EnemyType::SENTINEL, 2}, {EnemyType::RAPTOR, 2}},
            {{EnemyType::BRUTE, 2}, {EnemyType::MITE, 6}, {EnemyType::HUSK, 3}, {EnemyType::RAPTOR, 2}},
        };
        a.maxAlive = 8;
        a.theme = Theme{
            {0.04f,0.02f,0.02f}, {0.30f,0.10f,0.04f}, {0.05f,0.02f,0.01f},
            glm::normalize(vec3{0.f, 0.3f, -1.f}), {0.f,0.f,0.f}, 0.01f, 0.f,
            {0.06f,0.02f,0.01f}, 0.f,
            glm::normalize(vec3{0.2f,-1.f,0.15f}), {0.55f,0.38f,0.28f},
            {0.10f,0.08f,0.08f}, {0.42f,0.16f,0.05f},
            {0.16f,0.06f,0.03f}, 0.016f };
        L.arenas.push_back(std::move(a));
    }

    // ---- Corridor 2 → 3 ------------------------------------------------------
    {
        vec3 c{0.20f,0.20f,0.24f}, cyan{0.2f,0.9f,1.0f};
        wall(-5,0,-126, -4,5,-111, c);
        wall( 4,0,-126,  5,5,-111, c);
        wall(-5,5,-126,  5,5.5f,-111, c);
        L.floors.push_back({-4.f, -126.f, 4.f, -111.f, 0.f, {0.15f,0.15f,0.18f}});
        neon(-3.9f,0,-125, -3.7f,0.05f,-112, cyan);
        neon( 3.7f,0,-125,  3.9f,0.05f,-112, cyan);
        neon(-0.3f,4.88f,-125.5f, 0.3f,4.98f,-111.5f, {0.45f,0.5f,0.6f});
        L.corridors.push_back(aabb(-5, 0, -126.5f, 5, 40, -109.5f));
    }

    // =========================================================================
    // ARENA 3 — THE CORE
    // =========================================================================
    {
        vec3 slate{0.24f,0.28f,0.34f}, slateDark{0.15f,0.18f,0.22f},
             cyan{0.15f,0.95f,1.0f}, magenta{1.0f,0.2f,0.7f};
        Arena a;
        a.name = "THE CORE";
        a.subtitle = "SURVIVE 2 WAVES - THEN THE WARDEN";
        a.bounds = aabb(-36, 0, -198, 36, 14, -126);
        a.zone   = aabb(-37.5f, 0, -199.5f, 37.5f, 40, -124.5f);
        a.playerStart = {0.f, 0.f, -130.f};
        a.bossSpawn   = {20.f, 0.f, -162.f};   // mirrored to whichever side is farther from you
        L.floors.push_back({-37.f, -199.f, 37.f, -125.f, 0.f, {0.20f,0.24f,0.28f}});

        wall(-37,0,-126, -4,6,-125, slate);
        wall(  4,0,-126, 37,6,-125, slate);
        a.entryGate = door(-4, -125.9f, 4, -125.1f, 6.f, {0.2f,0.22f,0.26f}, true);
        wall( 36,0,-199, 37,6,-125, slate);
        wall(-37,0,-199,-36,6,-125, slate);
        wall(-37,0,-199, 37,6,-198, slate);
        neon(-36,5.3f,-198.0f+0.02f, 36,5.5f,-197.88f, cyan);
        neon( 35.88f,5.3f,-198, 35.98f,5.5f,-126, cyan);
        neon(-35.98f,5.3f,-198, -35.88f,5.5f,-126, cyan);
        neon(-36,5.3f,-126.12f, -4.6f,5.5f,-126.02f, cyan);
        neon(4.6f,5.3f,-126.12f, 36,5.5f,-126.02f, cyan);

        // Reactor pedestal; the core itself is animated in GameplayState
        wall(-5,0,-167, 5,1.5f,-157, slate);
        ring(-5,-167, 5,-157, 1.2f, 1.4f, magenta);
        L.walls.push_back(Wall{aabb(-2,1.5f,-164, 2,11,-160), {0.1f,0.1f,0.1f}, true});

        // Ring of eight pillars
        for (int k = 0; k < 8; ++k) {
            float ang = glm::radians(22.5f + 45.f * k);
            float cx = std::cos(ang) * 18.f, cz = -162.f + std::sin(ang) * 18.f;
            wall(cx - 1,0,cz - 1, cx + 1,7,cz + 1, slate);
            neon(cx - 1.05f,7,cz - 1.05f, cx + 1.05f,7.25f,cz + 1.05f, cyan);
            ring(cx - 1,cz - 1, cx + 1,cz + 1, 2.0f, 2.15f, cyan);
        }

        // Corner perches with jump pads
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            float zIn = sz < 0 ? -186.f : -138.f, zOut = sz < 0 ? -198.f : -126.f;
            wall(sx * 24.f,0,zIn, sx * 36.f,4,zOut, slateDark);
            neon(sx * 23.88f,3.6f,zIn, sx * 23.98f,3.85f,zOut, magenta);
            float zFace = zIn + (sz < 0 ? 0.12f : -0.12f);
            neon(sx * 24.f,3.6f,zFace, sx * 36.f,3.85f,zIn + (sz < 0 ? 0.02f : -0.02f), magenta);
            float pz = sz < 0 ? -183.f : -141.f;
            L.pads.push_back({{sx * 21.f, 0.f, pz}, {1.4f, 1.4f}, {sx * 6.f, 15.5f, sz < 0 ? -6.f : 6.f}});
        }

        // Low cover
        wall(-12,0,-140, -8,1.3f,-138, slateDark);
        wall(  8,0,-140, 12,1.3f,-138, slateDark);
        wall(-12,0,-186, -8,1.3f,-184, slateDark);
        wall(  8,0,-186, 12,1.3f,-184, slateDark);
        wall(-27,0,-164, -25,1.3f,-160, slateDark);
        wall( 25,0,-164,  27,1.3f,-160, slateDark);

        // A dead city skyline beyond the walls
        const float towers[][4] = {{-52,-150,8,34},{-58,-178,10,26},{-48,-204,7,40},{52,-140,9,30},
                                   {60,-170,12,22},{50,-210,8,44},{-20,-218,10,30},{22,-224,9,36},{0,-232,14,20}};
        for (auto& t : towers) {
            float x = t[0], z = t[1], w = t[2] * 0.5f, h = t[3];
            prop(x - w,0,z - w, x + w,h,z + w, {0.05f,0.06f,0.08f});
            for (float y = 4.f; y < h - 2.f; y += 5.f)
                neon(x - w - 0.05f,y,z - w - 0.05f, x + w + 0.05f,y + 0.25f,z + w + 0.05f,
                     (int)(y + x) % 2 ? cyan * 0.5f : magenta * 0.4f);
        }

        a.groundSpawns = {{30,4.05f,-192},{-30,4.05f,-192},{30,4.05f,-132},{-30,4.05f,-132},
                          {0,0,-192},{-18,0,-192},{18,0,-192},{-31,0,-162},{31,0,-162},
                          {0,0,-150},{-14,0,-175},{14,0,-175}};
        a.airSpawns    = {{-18,10,-180},{18,10,-180},{0,12,-190},{-20,10,-140},{20,10,-140},{0,12,-145}};
        a.waves = {
            {{EnemyType::BRUTE, 2}, {EnemyType::SENTINEL, 3}, {EnemyType::RAPTOR, 3}, {EnemyType::RIPPER, 4}},
            {{EnemyType::BRUTE, 2}, {EnemyType::MITE, 8}, {EnemyType::HUSK, 4}, {EnemyType::RAPTOR, 3}, {EnemyType::SENTINEL, 2}},
            {{EnemyType::WARDEN, 1}},
        };
        a.maxAlive = 10;
        a.damageScale = 1.1f;
        a.theme = Theme{
            {0.005f,0.012f,0.04f}, {0.05f,0.22f,0.30f}, {0.01f,0.03f,0.04f},
            glm::normalize(vec3{0.45f, 0.32f, -1.f}), {0.8f,1.1f,1.3f}, 0.09f, 0.f,
            {0.02f,0.05f,0.08f}, 1.f,
            glm::normalize(vec3{-0.35f,-0.6f,0.7f}), {0.55f,0.75f,0.95f},
            {0.14f,0.22f,0.32f}, {0.04f,0.06f,0.08f},
            {0.04f,0.12f,0.16f}, 0.012f };
        L.arenas.push_back(std::move(a));
    }

    return L;
}
