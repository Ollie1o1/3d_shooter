#pragma once
// =============================================================================
// LevelPlain.h — Chapter 4: THE BLOOD MOON PLAIN (docs/chapter-4-design.md).
//
// One open space, 1.4 km across, on the hard ground at Y 0 (no basin): cracked
// basalt with glowing fissures, meteor craters ringed with rubble, heaps of
// rejected units where the sky has been dumping them, and a few meteors big
// enough to read as landmarks from across the plain. Far beyond the edge,
// spires and the ribs of something enormous give it scale. The sky (the blood
// moon, burning cloud) is skybox.frag's uPlain; the falling meteors are live
// (PlainMeteors.h, Gameplay_Plain.h).
//
// Every random draw is its own statement (argument evaluation order differs
// between compilers; see the lab dressing notes).
// =============================================================================
#include "world/Level.h"

namespace plain {
constexpr float HALF = 700.f;                 // the playable square: -HALF..HALF in X and Z
constexpr float FAR_GROUND = 3200.f;          // the ground keeps going this far (visual only)
constexpr float FAR_PLANE = 4500.f;           // the camera sees this far here (Gameplay_Plain.h)
inline glm::vec3 start() { return {0.f, 0.f, 560.f}; }
// The blood moon, low in the north: where the Gleaner's back will rise
inline glm::vec3 moonDir() { return glm::normalize(glm::vec3{0.1f, 0.42f, -1.f}); }

inline float hash(float n, float k) { float v = std::sin(n * 12.9898f + k * 78.233f) * 43758.5453f; return v - std::floor(v); }

inline Theme theme() {
    Theme t{};
    t.zenith = {0.09f, 0.008f, 0.012f};
    t.horizon = {0.62f, 0.09f, 0.035f};
    t.ground = {0.05f, 0.01f, 0.008f};
    t.sunDir = moonDir();
    t.sunColor = {1.7f, 0.2f, 0.08f};
    t.sunSize = 0.16f;
    t.sunStripes = 0.f;
    t.mountain = {0.035f, 0.006f, 0.006f};
    t.stars = 0.25f;
    t.lightDir = glm::normalize(glm::vec3{-0.12f, -0.42f, 0.9f});   // the moon's light, travelling south and down
    t.lightColor = {1.05f, 0.32f, 0.2f};
    t.skyAmb = {0.36f, 0.09f, 0.07f};
    t.groundAmb = {0.2f, 0.06f, 0.035f};
    t.fogColor = {0.24f, 0.04f, 0.028f};
    t.fogDensity = 0.0008f;   // thin enough that the Gleaner reads at a kilometre
    return t;
}

// Where the craters, heaps and landmark meteors sit (kept off the start and
// off each other). Shared with the tests and the game's own code.
struct Feature { glm::vec3 pos; float radius; int kind; };   // kind: 0 crater, 1 heap, 2 landmark meteor
inline std::vector<Feature> features() {
    std::vector<Feature> f;
    auto clear = [&](glm::vec3 p, float r) {
        if (glm::length(glm::vec2{p.x - start().x, p.z - start().z}) < r + 45.f) return false;   // the arrival stays open
        for (const auto& o : f) if (glm::length(glm::vec2{p.x - o.pos.x, p.z - o.pos.z}) < r + o.radius + 25.f) return false;
        return std::abs(p.x) < HALF - r - 10.f && std::abs(p.z) < HALF - r - 10.f;
    };
    // Landmarks first: big, few, spread out (one straight ahead of the arrival)
    const glm::vec3 marks[] = {{-250.f, 0.f, 360.f}, {310.f, 0.f, 120.f}, {-380.f, 0.f, -40.f}, {120.f, 0.f, -260.f},
                               {-250.f, 0.f, -470.f}, {470.f, 0.f, -420.f}, {-520.f, 0.f, 420.f}, {520.f, 0.f, 460.f}};
    for (int k = 0; k < 8; ++k) f.push_back({marks[k], 26.f + 10.f * hash(k, 1.f), 2});
    int tries = 0;
    for (int k = 0; k < 34 && tries < 4000; ++tries) {   // craters
        float x = (hash(tries, 2.f) * 2.f - 1.f) * HALF;
        float z = (hash(tries, 3.f) * 2.f - 1.f) * HALF;
        float r = 9.f + 22.f * hash(tries, 4.f);
        if (!clear({x, 0.f, z}, r)) continue;
        f.push_back({{x, 0.f, z}, r, 0}); ++k;
    }
    for (int k = 0; k < 22 && tries < 8000; ++tries) {   // heaps of the rejected
        float x = (hash(tries, 5.f) * 2.f - 1.f) * HALF;
        float z = (hash(tries, 6.f) * 2.f - 1.f) * HALF;
        float r = 6.f + 7.f * hash(tries, 7.f);
        if (!clear({x, 0.f, z}, r)) continue;
        f.push_back({{x, 0.f, z}, r, 1}); ++k;
    }
    // Two heaps close to the arrival, so the first thing you walk to is one of you
    // (nudged round until they are clear of everything else)
    const glm::vec3 near[2] = {{-38.f, 0.f, -70.f}, {46.f, 0.f, -120.f}};
    for (int k = 0; k < 2; ++k) {
        float r = 7.f + 2.f * k;
        for (int t = 0; t < 64; ++t) {
            glm::vec3 p = start() + near[k] + glm::vec3{std::cos(t * 0.9f), 0.f, std::sin(t * 0.9f)} * (float)(t * 3);
            if (clear(p, r)) { f.push_back({p, r, 1}); break; }
        }
    }
    return f;
}
}

// Inside THE GLEANER (Gameplay_GleanerInside.h): three rooms of its foundry climbing up through it, one
// organ each. They are built far off the plain (the furnace door is a passage, not a place in its body).
namespace gut {
inline glm::vec3 O() { return {5200.f, 0.f, 0.f}; }
// room 0 the Crucible Heart, 1 the Bellows Lungs, 2 the Sorting Core: floor height, half extents, ceiling
inline float floorY(int r) { static const float F[3] = {0.f, 32.f, 62.f}; return F[r]; }
inline glm::vec2 half(int r) { static const glm::vec2 H[3] = {{30.f, 30.f}, {34.f, 26.f}, {32.f, 32.f}}; return H[r]; }
inline float ceilY(int r) { static const float C[3] = {26.f, 58.f, 98.f}; return C[r]; }
constexpr float ISLAND_H = 1.8f, STEP_H = 0.9f;
inline glm::vec3 arrive(int r) {   // where you come in: the furnace door's passage, then each shaft's top
    static const glm::vec3 A[3] = {{0.f, ISLAND_H, 25.f}, {17.f, 32.f, 6.f}, {-24.f, 62.f, 3.f}};   // (clear of the hole the shaft comes up through)
    return O() + A[r];
}
inline float arriveYaw(int r) { static const float Y[3] = {-90.f, 100.f, -8.f}; return Y[r]; }   // (facing its organ)
// The shaft up out of each room (a column of the furnace's breath, once its organ is dead)
inline glm::vec3 shaft(int r) { static const glm::vec3 S[3] = {{25.f, 0.f, 12.f}, {-24.f, 32.f, 14.f}, {0.f, 62.f, 0.f}}; return O() + S[r]; }
constexpr float SHAFT_R = 3.5f;
inline glm::vec3 heart() { return O() + glm::vec3{0.f, 13.f, -2.f}; }
inline glm::vec3 bellows(int k) { return O() + glm::vec3{k ? 16.f : -16.f, 32.f + 6.f, 22.f}; }
inline glm::vec3 blow() { return {0.f, 0.f, -1.f}; }
inline float grateZ() { return O().z - 19.f; }   // the burning grate at the lungs' far end (z below this)
inline glm::vec3 core() { return O() + glm::vec3{0.f, 77.f, 0.f}; }
// The islands in the heart's room (centre, half size), and the pillars that shelter you from the lungs
inline glm::vec3 island(int k) { float a = 0.5236f + k * 1.0472f; return O() + glm::vec3{std::cos(a) * 17.f, 0.f, std::sin(a) * 17.f - 2.f}; }
constexpr int ISLANDS = 6;
constexpr float ISLAND_HALF = 3.4f;
inline glm::vec3 pillar(int k) { static const glm::vec3 P[4] = {{-10.f, 32.f, 4.f}, {10.f, 32.f, 4.f}, {-22.f, 32.f, -6.f}, {22.f, 32.f, -6.f}}; return O() + P[k]; }
constexpr float PILLAR_HALF = 2.2f;
inline glm::vec3 cover(int k) { static const glm::vec3 C[4] = {{-14.f, 62.f, 14.f}, {14.f, 62.f, -14.f}, {16.f, 62.f, 16.f}, {-16.f, 62.f, -16.f}}; return O() + C[k]; }
inline AABB bounds() { return LevelBuilder::aabb(O().x - 40.f, -10.f, O().z - 40.f, O().x + 40.f, 110.f, O().z + 40.f); }
inline bool inside(glm::vec3 p) { AABB b = bounds(); return p.x > b.min.x && p.x < b.max.x && p.y > b.min.y && p.y < b.max.y && p.z > b.min.z && p.z < b.max.z; }
inline int roomAt(glm::vec3 p) { return p.y < floorY(1) - 1.f ? 0 : p.y < floorY(2) - 1.f ? 1 : 2; }
inline Theme theme() {
    Theme t = plain::theme();
    t.zenith = t.horizon = t.ground = {0.05f, 0.012f, 0.008f};
    t.lightDir = glm::normalize(glm::vec3{0.2f, -1.f, 0.15f}); t.lightColor = {0.7f, 0.25f, 0.1f};
    t.skyAmb = {0.3f, 0.1f, 0.06f}; t.groundAmb = {0.45f, 0.16f, 0.05f};   // lit from below: the slag
    t.fogColor = {0.16f, 0.04f, 0.02f}; t.fogDensity = 0.012f;
    t.stars = 0.f; t.sunSize = 0.f;
    return t;
}
}

inline void dressGut(LevelBuilder& b);
// The rooms' walls (iron), the heart's islands, the lungs' pillars and grate, the core's cover
inline void buildGut(LevelBuilder& b) {
    using glm::vec3;
    const vec3 O = gut::O(), iron{0.13f, 0.1f, 0.09f}, rust{0.3f, 0.14f, 0.07f}, slagLit{1.6f, 0.45f, 0.1f};
    b.mat = Mat::METAL;
    for (int r = 0; r < 3; ++r) {
        const float y0 = gut::floorY(r), y1 = gut::ceilY(r);
        const glm::vec2 h = gut::half(r);
        const float x0 = O.x - h.x, x1 = O.x + h.x, z0 = O.z - h.y, z1 = O.z + h.y;
        // the floor, with the shaft from below cut out of it (a ring of plates round it)
        const vec3 under = r > 0 ? gut::shaft(r - 1) : vec3{0.f, -1000.f, 0.f};
        const float hole = gut::SHAFT_R + 0.5f;
        if (r == 0) b.wall(x0, y0 - 2.f, z0, x1, y0, z1, iron);
        else {
            b.wall(x0, y0 - 2.f, z0, under.x - hole, y0, z1, iron);
            b.wall(under.x + hole, y0 - 2.f, z0, x1, y0, z1, iron);
            b.wall(under.x - hole, y0 - 2.f, z0, under.x + hole, y0, under.z - hole, iron);
            b.wall(under.x - hole, y0 - 2.f, under.z + hole, under.x + hole, y0, z1, iron);
        }
        // walls and the roof (its shaft's mouth cut out of the roof)
        b.wall(x0 - 2.f, y0, z0 - 2.f, x1 + 2.f, y1, z0, iron);
        b.wall(x0 - 2.f, y0, z1, x1 + 2.f, y1, z1 + 2.f, iron);
        b.wall(x0 - 2.f, y0, z0, x0, y1, z1, iron);
        b.wall(x1, y0, z0, x1 + 2.f, y1, z1, iron);
        if (r < 2) {
            const vec3 sh = gut::shaft(r);
            b.wall(x0, y1, z0, sh.x - hole, y1 + 2.f, z1, rust);
            b.wall(sh.x + hole, y1, z0, x1, y1 + 2.f, z1, rust);
            b.wall(sh.x - hole, y1, z0, sh.x + hole, y1 + 2.f, sh.z - hole, rust);
            b.wall(sh.x - hole, y1, sh.z + hole, sh.x + hole, y1 + 2.f, z1, rust);
        } else b.wall(x0, y1, z0, x1, y1 + 2.f, z1, rust);   // (the chimney: Gameplay_GleanerInside.h throws you up out of it)
        // glowing seams in the walls: the furnace's light
        for (int k = 0; k < 6; ++k) {
            float u = x0 + (k + 0.5f) * (x1 - x0) / 6.f;
            b.neon(u - 0.15f, y0 + 2.f, z0 + 0.02f, u + 0.15f, y1 - 2.f, z0 + 0.08f, slagLit * 0.35f);
            b.neon(u - 0.15f, y0 + 2.f, z1 - 0.08f, u + 0.15f, y1 - 2.f, z1 - 0.02f, slagLit * 0.35f);
        }
    }
    // The heart's room: the entry ledge, six islands round the floor it floods, each with a step up
    b.wall(O.x - 7.f, 0.f, O.z + 21.f, O.x + 7.f, gut::ISLAND_H, O.z + 30.f, rust);
    b.wall(O.x - 3.f, 0.f, O.z + 19.f, O.x + 3.f, gut::STEP_H, O.z + 21.f, rust);
    for (int k = 0; k < gut::ISLANDS; ++k) {
        vec3 c = gut::island(k);
        const float hh = gut::ISLAND_HALF;
        b.wall(c.x - hh, 0.f, c.z - hh, c.x + hh, gut::ISLAND_H, c.z + hh, rust);
        vec3 toward = glm::normalize(vec3{O.x - c.x, 0.f, O.z - 2.f - c.z});
        vec3 st = c + toward * (hh + 1.f);
        b.wall(st.x - 1.2f, 0.f, st.z - 1.2f, st.x + 1.2f, gut::STEP_H, st.z + 1.2f, iron);
    }
    // The shaft's foot in the heart's room: a ring of grating round it
    // The lungs' room: pillars to stand behind, the grate burning at its far end
    for (int k = 0; k < 4; ++k) {
        vec3 p = gut::pillar(k);
        const float ph = gut::PILLAR_HALF;
        b.wall(p.x - ph, p.y, p.z - ph, p.x + ph, gut::ceilY(1), p.z + ph, iron);
    }
    b.neon(O.x - 33.f, 32.02f, O.z - 26.f, O.x + 33.f, 32.1f, gut::grateZ(), slagLit * 0.9f);
    // The core's room: low walls of sorted bodies' crates to duck behind
    for (int k = 0; k < 4; ++k) {
        vec3 c = gut::cover(k);
        b.wall(c.x - 3.f, c.y, c.z - 1.f, c.x + 3.f, c.y + 1.6f, c.z + 1.f, rust);
    }
    dressGut(b);
}

// What makes the rooms feel like the inside of a living foundry (all of it against the walls and
// overhead, never in the way): iron ribs arching over each room like a ribcage, buttresses between
// them, pipes and sinew-cables, chains, slag running in channels along the walls; then each room's own
inline void dressGut(LevelBuilder& b) {
    using glm::vec3;
    const vec3 O = gut::O(), iron{0.13f, 0.1f, 0.09f}, dark{0.06f, 0.045f, 0.04f}, rust{0.3f, 0.14f, 0.07f}, bone{0.36f, 0.3f, 0.25f},
               slag{1.8f, 0.5f, 0.12f}, sinew{0.28f, 0.07f, 0.05f};
    b.mat = Mat::METAL;
    auto k = b.kit();
    for (int r = 0; r < 3; ++r) {
        const float y0 = gut::floorY(r), y1 = gut::ceilY(r), hgt = y1 - y0;
        const glm::vec2 h = gut::half(r);
        const float x0 = O.x - h.x, x1 = O.x + h.x, z0 = O.z - h.y, z1 = O.z + h.y;
        // the ribs: arches across the room, from wall to wall, every few metres
        const int ribs = 5;
        for (int i = 0; i < ribs; ++i) {
            float z = z0 + (i + 0.5f) * (z1 - z0) / ribs;
            k.arch(vec3{O.x, y0 + hgt * 0.35f, z}, h.x * 2.f - 0.4f, hgt * 0.6f, 1.4f, 1.6f, i % 2 ? iron * 1.2f : bone * 0.7f, 1.5708f, 16);
            for (float s : {-1.f, 1.f}) {   // its foot: a buttress up the wall to the arch's spring
                float x = s < 0.f ? x0 + 0.7f : x1 - 0.7f;
                b.prop(x - 0.7f, y0, z - 0.9f, x + 0.7f, y0 + hgt * 0.35f, z + 0.9f, iron * 1.1f);
                b.prop(x - 0.9f, y0, z - 1.1f, x + 0.9f, y0 + 1.2f, z + 1.1f, rust);
                b.neon(x - 0.75f, y0 + 1.6f, z - 0.05f, x + 0.75f, y0 + hgt * 0.33f, z + 0.05f, slag * 0.25f);
            }
        }
        // pipes along the walls, two heights, with glowing joints; a slag channel along the foot of each long wall
        for (float s : {-1.f, 1.f}) {
            float x = s < 0.f ? x0 + 0.5f : x1 - 0.5f;
            for (int p = 0; p < 2; ++p) {
                float y = y0 + 3.f + p * 4.5f;
                k.rod(vec3{x, y, z0 + 0.6f}, vec3{x, y, z1 - 0.6f}, 0.35f + 0.15f * p, p ? rust : iron * 1.2f, 8);
                for (int j = 0; j < 6; ++j) {
                    float z = z0 + (j + 0.5f) * (z1 - z0) / 6.f;
                    b.neon(x - 0.55f, y - 0.55f, z - 0.2f, x + 0.55f, y + 0.55f, z + 0.2f, slag * 0.4f);
                }
            }
            b.neon(x - 0.45f, y0 + 0.01f, z0 + 1.f, x + 0.45f, y0 + 0.06f, z1 - 1.f, slag * 0.8f);
        }
        // chains and sinew hanging from the ribs, in the corners and over the edges (never across the middle)
        for (int c = 0; c < 8; ++c) {
            float a = c * 0.785f + 0.39f;
            float x = O.x + std::cos(a) * h.x * 0.82f, z = O.z + std::sin(a) * h.y * 0.82f;
            float len = hgt * (0.3f + 0.25f * std::abs(std::sin(c * 1.7f + r)));
            if (c % 2) k.rod(vec3{x, y1, z}, vec3{x, y1 - len, z}, 0.12f, dark, 4);
            else { k.rod(vec3{x, y1, z}, vec3{x + 0.6f, y1 - len, z + 0.4f}, 0.25f, sinew, 6); k.column(vec3{x + 0.6f, y1 - len - 0.6f, z + 0.4f}, 0.45f, 0.8f, sinew * 1.2f, 6, 0.4f); }
        }
        // cables strung between the ribs overhead, drooping
        for (int c = 0; c < 6; ++c) {
            float x = x0 + 4.f + c * (h.x * 2.f - 8.f) / 5.f;
            vec3 a{x, y1 - 0.5f, z0 + 1.f}, bb{x, y1 - 0.5f, z1 - 1.f}, m{x, y1 - 4.f - 2.f * (c % 3), O.z};
            k.rod(a, m, 0.15f, sinew, 4); k.rod(m, bb, 0.15f, sinew, 4);
        }
    }
    // The heart's room: a rim of melted slag round the flooding floor, bone teeth up its walls
    for (int i = 0; i < 16; ++i) {
        float a = i * 0.3927f;
        vec3 p = O + vec3{std::cos(a) * 27.f, 0.f, std::sin(a) * 27.f};
        k.rock(p, 1.2f + 0.5f * std::sin(i * 2.3f), 2.5f + 2.f * std::abs(std::sin(i * 1.3f)), bone * 0.6f, 40 + i, 0.3f, 5);
    }
    // The lungs' room: the bellows' leather pleats down its far wall, the grate's glow licking up it
    for (int i = 0; i < 12; ++i) {
        float y = 34.f + i * 1.9f;
        k.box(vec3{O.x, y, O.z - 25.4f}, {60.f, 0.7f, 0.9f}, i % 2 ? sinew : dark, 0.f, i % 2 ? 0.3f : -0.3f);
    }
    b.neon(O.x - 30.f, 32.2f, O.z - 25.9f, O.x + 30.f, 40.f, O.z - 25.8f, slag * 0.35f);
    // The core's room: racks of sorted bodies up its walls (what it has kept), conveyor rails overhead
    for (int side = 0; side < 4; ++side) {
        for (int i = 0; i < 7; ++i) {
            float u = -24.f + i * 8.f;
            vec3 p = side == 0 ? vec3{O.x + u, 66.f, O.z - 31.2f} : side == 1 ? vec3{O.x + u, 66.f, O.z + 31.2f} : side == 2 ? vec3{O.x - 31.2f, 66.f, O.z + u} : vec3{O.x + 31.2f, 66.f, O.z + u};
            const bool alongX = side < 2;
            for (int t = 0; t < 3; ++t) {   // three shelves of them
                vec3 q = p + vec3{0.f, t * 3.2f, 0.f};
                k.box(q + vec3{0.f, -0.2f, 0.f}, alongX ? vec3{6.f, 0.25f, 1.2f} : vec3{1.2f, 0.25f, 6.f}, rust);
                for (int u2 = -1; u2 <= 1; ++u2) {   // a body hung on each: torso, head, a lit eye
                    vec3 o = q + (alongX ? vec3{u2 * 1.8f, 0.9f, 0.f} : vec3{0.f, 0.9f, u2 * 1.8f});
                    k.box(o, {0.6f, 1.1f, 0.4f}, vec3{0.15f, 0.16f, 0.18f});
                    k.box(o + vec3{0.f, 0.8f, 0.f}, {0.35f, 0.35f, 0.35f}, vec3{0.13f, 0.14f, 0.16f});
                    if ((i + t + u2 + side) % 5 == 0) k.box(o + vec3{0.f, 0.82f, 0.f}, {0.37f, 0.06f, 0.37f}, vec3{0.3f, 1.2f, 1.6f}).neon = true;
                }
            }
        }
    }
    for (float s : {-1.f, 1.f}) k.box(vec3{O.x + s * 9.f, 94.f, O.z}, {1.f, 1.2f, 62.f}, iron * 1.3f);
    for (int i = 0; i < 4; ++i) k.box(vec3{O.x, 94.f, O.z - 24.f + i * 16.f}, {20.f, 0.8f, 1.f}, rust);
}

inline LevelData buildPlain() {
    using glm::vec3;
    LevelData l; LevelBuilder b{l}; auto& s = l.story;
    s.built = true; s.chapter = 4; s.nextChapter = 0; s.title = "THE BLOOD MOON PLAIN"; s.nextName = "";
    s.startWeapons = 15; s.startGrapple = true; s.bossSection = 99; s.pitSection = -1;
    const vec3 basalt{0.16f, 0.11f, 0.1f}, scorched{0.09f, 0.06f, 0.055f}, ash{0.3f, 0.26f, 0.24f},
               rock{0.13f, 0.09f, 0.08f}, scrap{0.24f, 0.22f, 0.21f}, rust{0.32f, 0.16f, 0.09f},
               lava{1.6f, 0.42f, 0.1f}, ember{1.2f, 0.25f, 0.06f};

    // A broken lump of rock: three or four stacked, offset, tapering pieces (a single
    // prism reads as a smooth tower), leaning `lean` toward `yaw`
    auto boulder = [&](vec3 base, float r, float h, vec3 col, uint32_t seed, float lean, float yaw, int pieces = 4) {
        static const float PROFILE[5] = {0.9f, 1.f, 0.82f, 0.55f, 0.25f};
        float y = 0.f, rad = r * PROFILE[0];
        for (int k = 0; k < pieces; ++k) {
            float next = r * PROFILE[std::min(k + 1, 4)] * (0.85f + 0.3f * plain::hash((float)seed, 80.f + k));
            float seg = h / pieces * (0.8f + 0.4f * plain::hash((float)seed, 90.f + k));
            vec3 off{(plain::hash((float)seed, 100.f + k) - 0.5f) * r * 0.25f, y, (plain::hash((float)seed, 110.f + k) - 0.5f) * r * 0.25f};
            Shape& p = b.kit().column(base, rad, seg, col * (0.85f + 0.3f * plain::hash((float)seed, 120.f + k)), 8, next / std::max(rad, 0.01f));
            p.jitter = 0.34f; p.seed = seed * 7u + (uint32_t)k;
            p.xf = ShapeKit::T(base) * ShapeKit::R(yaw, {0, 1, 0}) * ShapeKit::R(lean, {1, 0, 0}) * ShapeKit::T(off)
                 * ShapeKit::R(plain::hash((float)seed, 130.f + k) * 6.283f, {0, 1, 0}) * ShapeKit::Sc({rad, seg, rad});
            y += seg * 0.92f; rad = next;
        }
    };

    // ---- the ground: 50 m patches inside, 400 m ones out to the horizon ----
    auto patch = [&](float x0, float z0, float x1, float z1, float n) {
        float a = plain::hash(n, 11.f), c = plain::hash(n, 12.f);
        vec3 col = glm::mix(basalt, scorched, a * 0.8f);
        if (c > 0.85f) col = glm::mix(col, ash, 0.35f);          // a drift of grey ash
        else if (c < 0.1f) col = glm::mix(col, rust, 0.3f);      // a rusted stain
        l.floors.push_back({x0, z0, x1, z1, 0.f, col, Mat::CINDER});
    };
    for (int i = 0; i < 28; ++i)
        for (int j = 0; j < 28; ++j) {
            float x0 = -plain::HALF + i * 50.f, z0 = -plain::HALF + j * 50.f;
            patch(x0, z0, x0 + 50.f, z0 + 50.f, (float)(i * 31 + j));
        }
    for (float x = -plain::FAR_GROUND; x < plain::FAR_GROUND; x += 400.f)
        for (float z = -plain::FAR_GROUND; z < plain::FAR_GROUND; z += 400.f) {
            bool inside = x >= -plain::HALF - 1.f && x + 400.f <= plain::HALF + 1.f && z >= -plain::HALF - 1.f && z + 400.f <= plain::HALF + 1.f;
            if (inside) continue;
            // Clip each outer tile around the inner square
            float x0 = x, x1 = x + 400.f, z0 = z, z1 = z + 400.f;
            auto overlapX = x0 < plain::HALF && x1 > -plain::HALF, overlapZ = z0 < plain::HALF && z1 > -plain::HALF;
            if (overlapX && overlapZ) {
                // Split into the parts outside the inner square
                if (x0 < -plain::HALF) patch(x0, z0, -plain::HALF, z1, x * 0.7f + z);
                if (x1 > plain::HALF) patch(plain::HALF, z0, x1, z1, x * 0.7f + z + 1.f);
                float cx0 = std::max(x0, -plain::HALF), cx1 = std::min(x1, plain::HALF);
                if (z0 < -plain::HALF) patch(cx0, z0, cx1, -plain::HALF, x * 0.7f + z + 2.f);
                if (z1 > plain::HALF) patch(cx0, plain::HALF, cx1, z1, x * 0.7f + z + 3.f);
                continue;
            }
            patch(x0, z0, x1, z1, x * 0.7f + z);
        }

    // ---- fissures: glowing cracks wandering across the ground ----
    {
        ShapeKit glow = b.kit(true);
        for (int k = 0; k < 70; ++k) {
            float x = (plain::hash(k, 21.f) * 2.f - 1.f) * (plain::HALF + 300.f);
            float z = (plain::hash(k, 22.f) * 2.f - 1.f) * (plain::HALF + 300.f);
            float heading = plain::hash(k, 23.f) * 6.283f;
            int segs = 4 + (int)(plain::hash(k, 24.f) * 6.f);
            float width = 0.18f + 0.5f * plain::hash(k, 25.f);
            for (int g = 0; g < segs; ++g) {
                float len = 6.f + 14.f * plain::hash(k * 13 + g, 26.f);
                float turn = (plain::hash(k * 13 + g, 27.f) - 0.5f) * 1.1f;
                heading += turn;
                vec3 d{std::cos(heading), 0.f, std::sin(heading)};
                vec3 mid = vec3{x, 0.05f, z} + d * (len * 0.5f);
                vec3 c = glm::mix(ember, lava, plain::hash(k, 28.f)) * (0.55f + 0.45f * plain::hash(k * 13 + g, 29.f));
                glow.box(mid, {len + width, 0.05f, width * (0.6f + 0.6f * plain::hash(k * 7 + g, 30.f))}, c, -heading);
                x += d.x * len; z += d.z * len;
            }
        }
    }

    // ---- craters, heaps and landmark meteors ----
    const auto feats = plain::features();
    int fi = 0;
    for (const auto& f : feats) {
        ++fi;
        const vec3 c = f.pos;
        const float n = (float)fi;
        if (f.kind == 0) {   // a crater: a rim of thrown rock, a molten floor, the meteor's broken core
            b.mat = Mat::CINDER;
            int lumps = 10 + (int)(f.radius * 0.6f);
            const float gap0 = plain::hash(n, 36.f) * 6.283f;   // three ways in through the rim
            for (int k = 0; k < lumps; ++k) {
                float a = (float)k / lumps * 6.283f + plain::hash(n * 3 + k, 31.f) * 0.3f;
                bool gap = false;
                for (int g = 0; g < 3; ++g) {
                    float da = std::fmod(std::abs(a - (gap0 + g * 2.094f)) + 9.4248f, 6.2832f) - 3.1416f;
                    gap |= std::abs(da) < std::max(0.32f, 4.5f / f.radius);
                }
                if (gap) continue;
                float rr = f.radius * (0.92f + 0.18f * plain::hash(n * 3 + k, 32.f));
                vec3 p = c + vec3{std::cos(a) * rr, 0.f, std::sin(a) * rr};
                float lump = 1.2f + f.radius * 0.09f * (0.6f + plain::hash(n * 5 + k, 33.f));
                float h = 0.8f + f.radius * 0.11f * plain::hash(n * 7 + k, 34.f);
                b.kit().rock(p, lump, h, glm::mix(rock, scorched, plain::hash(n * 9 + k, 35.f)), (uint32_t)(fi * 97 + k), 0.6f, 6);
                if (h > 1.4f) b.solid(p.x - lump * 0.6f, 0.f, p.z - lump * 0.6f, p.x + lump * 0.6f, h * 0.8f, p.z + lump * 0.6f);
            }
            // The molten floor and the scorched ring round it
            b.kit().column(c + vec3{0.f, 0.005f, 0.f}, f.radius * 0.95f, 0.01f, scorched * 0.6f, 18);
            b.kit(true).column(c + vec3{0.f, 0.02f, 0.f}, f.radius * 0.32f, 0.03f, lava * 0.7f, 14);
            // The core: a dark lump split with light
            float core = 1.5f + f.radius * 0.08f;
            b.kit().rock(c, core, core * 1.3f, rock * 0.7f, (uint32_t)(fi * 131), 0.5f, 7);
            b.kit(true).rod(c + vec3{-core * 0.7f, 0.3f, 0.f}, c + vec3{core * 0.6f, core * 1.1f, 0.2f}, 0.1f, lava, 5);
            b.kit(true).rod(c + vec3{0.f, 0.2f, -core * 0.7f}, c + vec3{0.1f, core * 0.9f, core * 0.5f}, 0.07f, ember, 5);
            b.solid(c.x - core * 0.7f, 0.f, c.z - core * 0.7f, c.x + core * 0.7f, core * 1.1f, c.z + core * 0.7f);
        } else if (f.kind == 1) {   // a heap of the rejected: scrap mound, bodies sprawled over it (drawn live)
            b.mat = Mat::METAL;
            const float r = f.radius, top = r * 0.34f;
            for (int ring = 0; ring < 4; ++ring) {   // a low mound: a dome flattened to `top`
                float a0 = 1.5708f * ring / 4.f, a1 = 1.5708f * (ring + 1) / 4.f;
                float r0 = std::cos(a0) * r, r1 = ring == 3 ? 0.f : std::cos(a1) * r;
                float y0 = std::sin(a0) * top, y1 = std::sin(a1) * top;
                Shape& m = b.kit().column(c + vec3{0.f, y0, 0.f}, r0, y1 - y0, scrap * (0.7f + 0.1f * ring), 12, r1 / r0);
                m.jitter = 0.12f; m.seed = (uint32_t)(fi * 59 + ring); m.caps = ring == 3;
            }
            // Walkable tiers under the mound, each under a single jump (about 0.9 m):
            // climb it, stand on it, use it as cover
            const int tiers = std::max(1, (int)std::ceil(top * 0.92f / 0.9f));
            for (int k = 0; k < tiers; ++k) {
                float h = std::min(top * 0.92f, 0.9f * (k + 1));
                float half = r * 0.82f * std::sqrt(std::max(0.05f, 1.f - (h / top) * (h / top)));
                b.solid(c.x - half, 0.f, c.z - half, c.x + half, h, c.z + half);
            }
            // Scrap sticking out of it: plates, beams, a broken belt section
            for (int k = 0; k < 9; ++k) {
                float a = plain::hash(n * 11 + k, 41.f) * 6.283f, d = r * (0.2f + 0.6f * plain::hash(n * 11 + k, 42.f));
                vec3 p = c + vec3{std::cos(a) * d, 0.f, std::sin(a) * d};
                p.y = top * std::sqrt(std::max(0.f, 1.f - (d / r) * (d / r))) - 0.2f;
                vec3 size{0.3f + 2.5f * plain::hash(n * 13 + k, 43.f), 0.1f + 0.3f * plain::hash(n * 13 + k, 44.f), 0.4f + 1.8f * plain::hash(n * 13 + k, 45.f)};
                b.kit().box(p, size, plain::hash(n * 17 + k, 46.f) > 0.6f ? rust : scrap,
                            plain::hash(n * 19 + k, 47.f) * 3.f, plain::hash(n * 23 + k, 48.f) - 0.5f, plain::hash(n * 29 + k, 49.f) - 0.5f);
            }
            // The bodies: on its slopes and spilled round its foot
            int bodies = 7 + (int)(r * 0.8f);
            for (int k = 0; k < bodies; ++k) {
                LevelData::Story::Dumped d;
                float a = plain::hash(n * 31 + k, 51.f) * 6.283f;
                float dist = r * (0.15f + 1.05f * plain::hash(n * 31 + k, 52.f));
                d.pos = c + vec3{std::cos(a) * dist, 0.f, std::sin(a) * dist};
                float u = dist / r;
                d.pos.y = u < 1.f ? top * std::sqrt(std::max(0.f, 1.f - u * u)) - 0.15f : 0.f;
                d.yaw = plain::hash(n * 37 + k, 53.f) * 6.283f;
                d.face = plain::hash(n * 41 + k, 54.f) > 0.55f;
                d.pose = plain::hash(n * 43 + k, 55.f);
                d.tilt = (plain::hash(n * 47 + k, 56.f) - 0.5f) * (u < 1.f ? 0.6f : 0.2f);
                s.dumped.push_back(d);
            }
        } else {   // a landmark meteor, half buried, tilted, split with light; seen from across the plain
            b.mat = Mat::CINDER;
            const float r = f.radius;
            const float lean = (plain::hash(n, 61.f) - 0.5f) * 0.5f, yaw = plain::hash(n, 62.f) * 6.283f;
            boulder(c + vec3{0.f, -r * 0.3f, 0.f}, r * 0.75f, r * 1.9f, rock * 0.8f, (uint32_t)(fi * 173), lean, yaw, 4);
            for (int k = 0; k < 3; ++k) {   // pieces that broke off it, leaning on it and lying round it
                float a = yaw + 1.3f + k * 2.1f;
                vec3 p = c + vec3{std::cos(a), 0.f, std::sin(a)} * (r * (0.7f + 0.25f * k));
                boulder(p + vec3{0.f, -1.f, 0.f}, r * (0.32f - 0.06f * k), r * (0.8f - 0.15f * k), rock * 0.9f, (uint32_t)(fi * 173 + 11 + k),
                        0.35f + 0.2f * k, a + 3.1416f, 3);
            }
            // Cracks of light down its sides
            for (int k = 0; k < 7; ++k) {
                float a = (float)k / 7.f * 6.283f + yaw;
                vec3 out{std::cos(a), 0.f, std::sin(a)};
                vec3 lo = c + out * (r * 0.62f) + vec3{0.f, 0.5f, 0.f}, hi = c + out * (r * 0.3f) + vec3{0.f, r * (0.7f + 0.5f * plain::hash(n * 3 + k, 63.f)), 0.f};
                b.kit(true).rod(lo, hi, 0.12f + 0.25f * plain::hash(n * 5 + k, 64.f), k % 2 ? lava : ember, 5);
            }
            // The ground it shoved up as it hit
            for (int k = 0; k < 14; ++k) {
                float a = (float)k / 14.f * 6.283f;
                float rr = r * (0.85f + 0.4f * plain::hash(n * 7 + k, 65.f));
                vec3 p = c + vec3{std::cos(a) * rr, 0.f, std::sin(a) * rr};
                float h = 1.f + 3.f * plain::hash(n * 9 + k, 66.f);
                b.kit().box(p + vec3{0.f, h * 0.3f, 0.f}, {r * 0.35f, h, r * 0.2f}, scorched, -a, 0.f, 0.35f + 0.3f * plain::hash(n * 11 + k, 67.f));
            }
            b.kit(true).column(c + vec3{0.f, 0.02f, 0.f}, r * 1.05f, 0.02f, ember * 0.35f, 16);
            b.solid(c.x - r * 0.62f, 0.f, c.z - r * 0.62f, c.x + r * 0.62f, r * 1.3f, c.z + r * 0.62f);
        }
    }

    // ---- far beyond the edge: spires, and the ribs of something enormous ----
    b.mat = Mat::CINDER;
    for (int k = 0; k < 36; ++k) {
        float a = (float)k / 36.f * 6.283f + plain::hash(k, 71.f) * 0.15f;
        float d = 1300.f + 1500.f * plain::hash(k, 72.f);
        vec3 p{std::cos(a) * d, -10.f, std::sin(a) * d};
        float h = 70.f + 230.f * plain::hash(k, 73.f);
        boulder(p, 25.f + 45.f * plain::hash(k, 74.f), h, rock * 0.5f, (uint32_t)(900 + k), (plain::hash(k, 75.f) - 0.5f) * 0.4f, a, 4);
    }
    for (int k = 0; k < 5; ++k) {   // a dead colossus's rib cage, lying in the west
        vec3 base{-1650.f + k * 70.f, -20.f, 650.f + k * 40.f};
        b.kit().arch(base, 220.f - k * 18.f, 260.f - k * 25.f, 14.f, 18.f, scrap * 0.4f, 0.55f);
    }

    buildGut(b);   // (inside the Gleaner: far off the plain)

    // ---- the arena ----
    Arena a;
    a.name = "THE BLOOD MOON PLAIN"; a.subtitle = "SOMETHING IS WATCHING";
    a.zone = LevelBuilder::aabb(-plain::HALF, -60.f, -plain::HALF, plain::HALF, 600.f, plain::HALF);
    a.bounds = a.zone; a.bounds.min.y = 0.f;
    a.playerStart = plain::start(); a.startYaw = -90.f;
    a.voidY = -50.f; a.hasRespawn = true; a.respawn = plain::start();
    a.hasTrigger = false; a.scripted = true;
    a.maxAlive = 8; a.damageScale = 1.f;
    a.extraZones.push_back(gut::bounds());
    a.theme = plain::theme();
    a.ambient = Ambient::EMBERS; a.space = ReverbSpace::OPEN; a.tone = RoomTone::YARD;
    l.arenas.push_back(a);
    return l;
}
