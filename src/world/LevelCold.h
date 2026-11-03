#pragma once
// =============================================================================
// LevelCold.h — Chapter 5: THE COLD STORE (docs/chapter-5-design.md).
//
// An ice shelf on a frozen planet. You walk out across packed snow toward its
// north edge, where it simply ends: past it is the Void, black all the way
// down, with racks and hook chains hanging off the lip into it. Across the
// chasm, about a kilometre off, the Store is cut into a glacier cliff: dark
// storage towers rising out of the black with rows of amber lamps, hook rails
// strung between them. Far round the horizon, frozen giants (siblings far
// bigger than the Gleaner) stand half buried in glaciers. The sky (a ringed gas
// giant, a small cold sun, aurora) is skybox.frag's uCold; the live parts (the
// dispatch capsules, shards rising out of the Void) are Gameplay_Cold.h.
//
// Every random draw is its own statement (argument evaluation order differs
// between compilers; see the lab dressing notes).
// =============================================================================
#include "world/Level.h"
#include "ui/PixelFont.h"

namespace cold {
constexpr float HALF_X = 260.f;               // the playable shelf: -HALF_X..HALF_X in X
constexpr float SOUTH = 430.f;                //   ...and from its north edge (edgeZ) to here in Z
constexpr float EDGE_Z = -320.f;              // the shelf's north edge, on average (it is ragged)
constexpr float STRIP = 40.f;                 // the edge is built in strips this wide, each its own depth
constexpr float FAR_GROUND = 3400.f;          // the shelf and the land round it keep going this far (visual)
constexpr float FAR_PLANE = 5200.f;           // the camera sees this far here (Gameplay_Cold.h)
constexpr float VOID_FLOOR = -900.f;          // the Void's "floor": far below the respawn line
constexpr float STORE_Z = -1250.f;            // the glacier cliff the Store is cut into
constexpr float SEA_X0 = 240.f;               // east of here the Void gives way to the frozen sea (rooms 6-7)
constexpr float SEA_Z1 = STORE_Z - 10.f;      // ...which runs north to the cliff
inline glm::vec3 start() { return {0.f, 0.f, 360.f}; }
// The sun: small, cold, low in the west-north-west (long blue shadows to the east)
inline glm::vec3 sunDir() { return glm::normalize(glm::vec3{-0.85f, 0.2f, -0.45f}); }
// The gas giant hangs low over the Store (the shader draws it from this too)
inline glm::vec3 giantDir() { return glm::normalize(glm::vec3{0.5f, 0.36f, -1.f}); }

inline float hash(float n, float k) { float v = std::sin(n * 12.9898f + k * 78.233f) * 43758.5453f; return v - std::floor(v); }

// Where the shelf ends at this X (a ragged lip, strip by strip)
inline float edgeZ(float x) {
    float i = std::floor(x / STRIP);
    return EDGE_Z + (hash(i, 1.f) - 0.5f) * 50.f;
}

inline Theme theme() {
    Theme t{};
    t.zenith = {0.006f, 0.009f, 0.02f};
    t.horizon = {0.36f, 0.43f, 0.52f};
    t.ground = {0.004f, 0.005f, 0.01f};          // below the horizon: the Void, black
    t.sunDir = sunDir();
    t.sunColor = {2.2f, 2.3f, 2.5f};
    t.sunSize = 0.018f;
    t.sunStripes = 0.f;
    t.mountain = {0.12f, 0.15f, 0.2f};
    t.stars = 1.f;
    t.lightDir = -sunDir();
    t.lightColor = {0.95f, 0.98f, 1.05f};
    t.skyAmb = {0.3f, 0.38f, 0.5f};               // the blue of the open sky in the shadows
    t.groundAmb = {0.34f, 0.37f, 0.42f};          // snow throws light back up
    t.fogColor = {0.4f, 0.47f, 0.56f};
    t.fogDensity = 0.00028f;                      // clear, cold air: the Store reads at a kilometre and more
    return t;
}

// The frozen giants: where they stand round the horizon, how tall, and which way they face
struct Giant { glm::vec3 pos; float height, yaw; bool armRaised; };
inline std::vector<Giant> giants() {
    return {{{1900.f, 0.f, 250.f}, 1300.f, -1.9f, true},    // east: one arm raised, as if to stop something
            {{-2100.f, 0.f, -100.f}, 1100.f, 1.4f, false},  // west, by the sun
            {{1100.f, 0.f, 2000.f}, 1500.f, 2.8f, false},   // behind you to the south-east
            {{-1300.f, 0.f, 1900.f}, 950.f, 0.4f, false}};  // south-west, smaller
}

// Storage towers rising out of the Void in front of the cliff (base, half width, height)
struct Tower { glm::vec3 base; float half, height; };
inline std::vector<Tower> towers() {
    std::vector<Tower> t;
    for (int k = 0; k < 15; ++k) {
        float x = -1700.f + k * 240.f + (hash(k, 2.f) - 0.5f) * 120.f;
        float z = STORE_Z + 50.f + hash(k, 3.f) * 200.f;   // (north of the Blast Chillers)
        float half = 35.f + 40.f * hash(k, 4.f);
        float h = 1250.f + 900.f * hash(k, 5.f);
        if (std::abs(x) < 260.f) h += 500.f;   // the tallest straight ahead of the shelf
        t.push_back({{x, VOID_FLOOR, z}, half, h});
    }
    return t;
}
}

// ---- the route (rooms 1-5): the Intake belt on the shelf, the Hook Line out over the
// Void, and the Receiving Tower standing in it (the Thaw Room, Inventory, Rack 24) ----
namespace cold {
constexpr float HOOK_X = -40.f;               // the Hook Line runs north along this X
constexpr float RAIL_Y = 10.f;                // its rail, overhead
constexpr float TOWER_X0 = -80.f, TOWER_X1 = 0.f, TOWER_FRONT = -600.f, TOWER_BACK = -760.f;
constexpr float CEIL = 14.f;
constexpr float THAW_BACK = -650.f, INV_BACK = -680.f;   // the Thaw Room is TOWER_FRONT..THAW_BACK; Inventory to INV_BACK; Rack 24 behind
constexpr float DOOR_X0 = HOOK_X - 4.f, DOOR_X1 = HOOK_X + 4.f;
constexpr float STATION_HALF = 2.5f, CARRIER_HALF = 1.75f;
constexpr int   CARRIERS = 4;
inline float lipZ() { return edgeZ(HOOK_X); }
inline float stationZ(int k) {   // 0: the gantry's end; 1..3: hanging stations; 4: the tower's dock
    const float z0 = lipZ() - 22.f, z4 = TOWER_FRONT + 10.f - STATION_HALF;
    return z0 + (z4 - z0) * (float)k / CARRIERS;
}
inline glm::vec3 hookStart() { return {HOOK_X, 0.f, stationZ(0)}; }
// The Thaw Room's ceiling hooks: frozen units hang from them; the first ones are where Thawlings drop
inline std::vector<glm::vec3> thawHooks() {
    std::vector<glm::vec3> h;
    for (int row = 0; row < 3; ++row)
        for (int k = 0; k < 6; ++k)
            h.push_back({-70.f + k * 12.f + (row == 1 ? 6.f : 0.f), 0.f, -612.f - row * 13.f});
    return h;
}
constexpr int THAW_SPAWNS = 13;               // hooks 0..12 drop; the rest stay hung
// Rack 24: ten copies of UNIT 24 in alcoves along the aisle (24-A first, on the west), and 24-K's empty slot
inline glm::vec3 rackSlot(int k) {   // k 0..9: A..J; 10: K
    if (k >= 10) return {HOOK_X, 0.f, TOWER_BACK + 6.f};
    float z = INV_BACK - 14.f - (k / 2) * 12.f;
    return {k % 2 == 0 ? -64.f : -16.f, 0.f, z};
}
inline glm::vec3 recallAt() { return rackSlot(0) + glm::vec3{0.f, 1.3f, 0.f}; }
// 6. THE BLAST CHILLERS: three cooling-tower tops standing in the Void, catwalks between them; the
// transfer bridge to them out of Rack 24's back door
struct Deck { glm::vec3 c; float r; };
inline Deck chiller(int k) { static const Deck D[3] = {{{90.f, 0.f, -830.f}, 22.f}, {{150.f, 0.f, -868.f}, 20.f}, {{210.f, 0.f, -830.f}, 22.f}}; return D[k]; }
constexpr float BRIDGE_Z0 = -834.f, BRIDGE_Z1 = -826.f;   // the bridge's east run, and the exit catwalk, along this band
inline glm::vec3 vent(int k) {   // two per deck: freezing columns on a cycle
    Deck d = chiller(k / 2);
    return d.c + glm::vec3{(k % 2 ? 1.f : -1.f) * 8.f, 0.f, (k % 2 ? -1.f : 1.f) * 7.f};
}
constexpr int VENTS = 6;
constexpr float VENT_R = 3.2f, VENT_PERIOD = 7.f, VENT_TELL = 1.3f, VENT_BLAST = 1.6f;
// 0 idle, 1 telling, 2 blasting: where vent k is in its cycle at time t
inline int ventState(int k, float t) {
    float u = std::fmod(t + k * 2.35f, VENT_PERIOD);
    return u < VENT_PERIOD - VENT_TELL - VENT_BLAST ? 0 : u < VENT_PERIOD - VENT_BLAST ? 1 : 2;
}
// 7. THE FROZEN SEA: from the Chillers' east catwalk across the ice to a broken rail station
inline glm::vec3 seaStart() { return {SEA_X0 + 6.f, 0.f, (BRIDGE_Z0 + BRIDGE_Z1) * 0.5f}; }
inline glm::vec3 station() { return {720.f, 0.f, -1000.f}; }
// RECLAIM v1, the first Gleaner, lying under the ice along your crossing (its head toward the station)
inline glm::vec3 reclaimTail() { return {250.f, -24.f, -820.f}; }
inline glm::vec3 reclaimHead() { return {640.f, -24.f, -975.f}; }
}

inline LevelData buildCold() {
    using glm::vec3;
    LevelData l; LevelBuilder b{l}; auto& s = l.story;
    s.built = true; s.chapter = 5; s.nextChapter = 0; s.title = "THE COLD STORE"; s.nextName = "TO BE CONTINUED";
    s.startWeapons = 15; s.startGrapple = true; s.bossSection = 99; s.pitSection = -1;
    const vec3 snow{0.86f, 0.9f, 0.96f}, snowBlue{0.66f, 0.75f, 0.86f}, ice{0.42f, 0.58f, 0.72f}, iceDeep{0.16f, 0.26f, 0.38f},
               steel{0.07f, 0.075f, 0.085f}, steelFrost{0.42f, 0.46f, 0.52f}, rime{0.78f, 0.84f, 0.9f},
               amber{2.2f, 1.1f, 0.25f}, sodium{1.8f, 0.85f, 0.2f};

    // A broken lump (of ice here): stacked, offset, tapering pieces, leaning `lean` toward `yaw`
    auto lump = [&](vec3 base, float r, float h, vec3 col, uint32_t seed, float lean, float yaw, int pieces = 4) {
        static const float PROFILE[5] = {0.95f, 1.f, 0.8f, 0.5f, 0.2f};
        float y = 0.f, rad = r * PROFILE[0];
        for (int k = 0; k < pieces; ++k) {
            float next = r * PROFILE[std::min(k + 1, 4)] * (0.85f + 0.3f * cold::hash((float)seed, 80.f + k));
            float seg = h / pieces * (0.8f + 0.4f * cold::hash((float)seed, 90.f + k));
            vec3 off{(cold::hash((float)seed, 100.f + k) - 0.5f) * r * 0.2f, y, (cold::hash((float)seed, 110.f + k) - 0.5f) * r * 0.2f};
            Shape& p = b.kit().column(base, rad, seg, col * (0.9f + 0.2f * cold::hash((float)seed, 120.f + k)), 7, next / std::max(rad, 0.01f));
            p.jitter = 0.28f; p.seed = seed * 7u + (uint32_t)k;
            p.xf = ShapeKit::T(base) * ShapeKit::R(yaw, {0, 1, 0}) * ShapeKit::R(lean, {1, 0, 0}) * ShapeKit::T(off)
                 * ShapeKit::R(cold::hash((float)seed, 130.f + k) * 6.283f, {0, 1, 0}) * ShapeKit::Sc({rad, seg, rad});
            y += seg * 0.92f; rad = next;
        }
    };

    // ---- the ground: the shelf in 40 m strips, each to its own ragged edge; the Void north of it ----
    auto patch = [&](float x0, float z0, float x1, float z1, float n) {
        float a = cold::hash(n, 11.f), c = cold::hash(n, 12.f);
        vec3 col = glm::mix(snow, snowBlue, a * 0.35f);
        Mat m = Mat::SNOW;
        if (c > 0.9f) { col = ice; m = Mat::ICE; }   // a blown-clear sheet of ice
        l.floors.push_back({x0, z0, x1, z1, 0.f, col, m});
    };
    for (float x = -cold::FAR_GROUND; x < cold::FAR_GROUND; x += cold::STRIP) {
        const float x1 = x + cold::STRIP, ez = cold::edgeZ(x);
        const float n = x * 0.37f;
        // The shelf: near patches small (they carry the colour variation), far ones long
        for (float z = ez; z < cold::FAR_GROUND; ) {
            float step = (std::abs(x) < 400.f && z < 600.f) ? 40.f : 400.f;
            float z1 = std::min(z + step, cold::FAR_GROUND);
            patch(x, z, x1, z1, n + z * 0.11f);
            z = z1;
        }
        // East of SEA_X0 the shelf runs out onto the frozen sea (drawn as clear ice: Gameplay_Cold.h)
        if (x >= cold::SEA_X0 - 0.5f) {
            l.basins.push_back({LevelBuilder::aabb(x, 0.f, cold::SEA_Z1, x1, 0.f, ez), 0.f});
            l.basins.push_back({LevelBuilder::aabb(x, 0.f, -cold::FAR_GROUND, x1, 0.f, cold::SEA_Z1), cold::VOID_FLOOR});
            continue;
        }
        // The Void beyond its lip: no floor, a long way down
        l.basins.push_back({LevelBuilder::aabb(x, 0.f, -cold::FAR_GROUND, x1, 0.f, ez), cold::VOID_FLOOR});
        // The lip's face: ice going down into the dark, banded deeper blue as it goes
        b.mat = Mat::ICE;
        b.kit().box({x + cold::STRIP * 0.5f, -30.f, ez - 2.f}, {cold::STRIP + 0.4f, 60.f, 4.f}, ice);
        b.kit().box({x + cold::STRIP * 0.5f, -140.f, ez - 6.f + 4.f * cold::hash(n, 13.f)}, {cold::STRIP + 0.4f, 160.f, 4.f}, iceDeep);
        b.kit().box({x + cold::STRIP * 0.5f, -360.f, ez - 10.f}, {cold::STRIP + 0.4f, 280.f, 4.f}, iceDeep * 0.5f);
        // ...and the side where the next strip's lip is further out
        float nz = cold::edgeZ(x1);
        if (x1 < cold::SEA_X0 - 0.5f && std::abs(nz - ez) > 0.5f) {
            float zA = std::min(ez, nz), zB = std::max(ez, nz);
            b.kit().box({x1, -60.f, (zA + zB) * 0.5f}, {4.f, 120.f, zB - zA}, ice);
        }
        // Blocks broken off the lip, sitting on it
        b.mat = Mat::ICE;
        for (int k = 0; k < 3; ++k) {
            float bx = x + cold::STRIP * (0.15f + 0.7f * cold::hash(n, 14.f + k));
            float sz = 1.5f + 4.f * cold::hash(n, 17.f + k);
            vec3 c{bx, sz * 0.35f, ez + 2.f + 6.f * cold::hash(n, 20.f + k)};
            b.kit().box(c, {sz, sz * 0.8f, sz * 0.9f}, glm::mix(ice, rime, cold::hash(n, 23.f + k)),
                        cold::hash(n, 26.f + k) * 3.f, (cold::hash(n, 29.f + k) - 0.5f) * 0.5f, (cold::hash(n, 32.f + k) - 0.5f) * 0.5f);
            if (std::abs(bx) < cold::HALF_X && sz > 2.5f) b.solid(c.x - sz * 0.4f, 0.f, c.z - sz * 0.4f, c.x + sz * 0.4f, sz * 0.6f, c.z + sz * 0.4f);
        }
    }

    // ---- on the shelf: pressure ridges, seracs, drifts ----
    {
        b.mat = Mat::ICE;
        for (int k = 0; k < 26; ++k) {   // pressure ridges: long broken walls of shoved-up slabs
            float x = (cold::hash(k, 41.f) * 2.f - 1.f) * (cold::HALF_X + 500.f);
            float z = cold::EDGE_Z + 60.f + cold::hash(k, 42.f) * 1400.f;
            if (glm::length(glm::vec2{x - cold::start().x, z - cold::start().z}) < 60.f) continue;   // the arrival stays open
            float heading = cold::hash(k, 43.f) * 3.1416f;
            int slabs = 6 + (int)(cold::hash(k, 44.f) * 8.f);
            for (int g = 0; g < slabs; ++g) {
                vec3 d{std::cos(heading), 0.f, std::sin(heading)};
                vec3 p = vec3{x, 0.f, z} + d * (g * 3.2f);
                float h = 1.f + 2.6f * cold::hash(k * 17 + g, 45.f);
                vec3 col = glm::mix(ice, rime, cold::hash(k * 17 + g, 46.f));
                b.kit().box(p + vec3{0.f, h * 0.3f, 0.f}, {3.4f, h, 0.9f + cold::hash(k * 17 + g, 47.f)}, col,
                            -heading + (cold::hash(k * 17 + g, 48.f) - 0.5f) * 0.4f, 0.f, (cold::hash(k * 17 + g, 49.f) - 0.5f) * 0.9f);
                if (std::abs(p.x) < cold::HALF_X + 5.f && p.z < cold::SOUTH + 5.f && h > 1.5f)   // cover: each slab its own block, one jump high
                    b.solid(p.x - 1.f, 0.f, p.z - 1.f, p.x + 1.f, 0.9f, p.z + 1.f);
            }
        }
        for (int k = 0; k < 40; ++k) {   // seracs: tilted towers of ice, some big enough to see from anywhere
            float x = (cold::hash(k, 51.f) * 2.f - 1.f) * (cold::HALF_X + 900.f);
            float z = cold::EDGE_Z + 50.f + cold::hash(k, 52.f) * 2400.f;
            if (glm::length(glm::vec2{x - cold::start().x, z - cold::start().z}) < 70.f) continue;
            if (z < cold::edgeZ(x) + 20.f) continue;
            float r = 3.f + 9.f * cold::hash(k, 53.f), h = r * (1.6f + 1.8f * cold::hash(k, 54.f));
            lump({x, -1.f, z}, r, h, glm::mix(ice, rime, 0.3f + 0.5f * cold::hash(k, 55.f)), (uint32_t)(300 + k),
                 (cold::hash(k, 56.f) - 0.5f) * 0.45f, cold::hash(k, 57.f) * 6.283f, 3 + (int)(cold::hash(k, 58.f) * 2.f));
            if (std::abs(x) < cold::HALF_X + r && z < cold::SOUTH + r) b.solid(x - r * 0.6f, 0.f, z - r * 0.6f, x + r * 0.6f, h * 0.75f, z + r * 0.6f);
        }
        b.mat = Mat::SNOW;
        for (int k = 0; k < 30; ++k) {   // drifts: long, low, wind-shaped
            float x = (cold::hash(k, 61.f) * 2.f - 1.f) * cold::HALF_X * 1.6f;
            float z = cold::EDGE_Z + 40.f + cold::hash(k, 62.f) * 900.f;
            if (z < cold::edgeZ(x) + 12.f) continue;
            float len = 14.f + 30.f * cold::hash(k, 63.f);
            Shape& d = b.kit().column({x, -0.6f, z}, 1.f, 1.f, glm::mix(snow, snowBlue, 0.2f), 9, 0.5f);
            d.jitter = 0.15f; d.seed = (uint32_t)(700 + k);
            d.xf = ShapeKit::T({x, -0.6f, z}) * ShapeKit::R(0.35f + 0.2f * cold::hash(k, 64.f), {0, 1, 0}) * ShapeKit::Sc({len * 0.5f, 1.2f + 0.8f * cold::hash(k, 65.f), 4.f});
        }
    }

    // ---- marker poles: a line of amber lamps from the arrival to the edge (where the Intake will be) ----
    for (int k = 0; k < 14; ++k) {
        float z = cold::start().z - 30.f - k * 47.f;
        if (z < cold::edgeZ(-14.f) + 8.f) break;
        for (int side = -1; side <= 1; side += 2) {
            vec3 p{side * 7.f + (cold::hash(k, 71.f + side) - 0.5f) * 2.f, 0.f, z};
            b.mat = Mat::METAL;
            b.kit().rod(p, p + vec3{0.f, 3.2f, 0.f}, 0.08f, steel, 6);
            b.kit().box(p + vec3{0.f, 3.35f, 0.f}, {0.35f, 0.3f, 0.35f}, steel);
            b.kit(true).box(p + vec3{0.f, 3.2f, 0.f}, {0.22f, 0.12f, 0.22f}, amber);
            b.kit().box(p + vec3{0.f, 3.52f, 0.f}, {0.42f, 0.06f, 0.42f}, rime);   // snow on its cap
        }
    }

    // ---- off the lip: gantries over the Void, chains and racks hanging down into it ----
    const float gantryX[] = {-150.f, -95.f, 95.f, 210.f, -520.f, -1350.f, -980.f, -1750.f};   // (the Hook Line's own is at HOOK_X)
    for (int k = 0; k < 8; ++k) {
        float x = gantryX[k], ez = cold::edgeZ(x);
        float reach = 26.f + 20.f * cold::hash(k, 81.f);
        b.mat = Mat::METAL;
        // Two girders cantilevered out over the drop, braced back onto the shelf
        for (int side = -1; side <= 1; side += 2) {
            vec3 root{x + side * 3.f, 6.f, ez + 12.f}, tip{x + side * 3.f, 6.f, ez - reach};
            b.kit().box((root + tip) * 0.5f, {0.6f, 0.9f, glm::length(tip - root)}, steel);
            b.kit().box((root + tip) * 0.5f + vec3{0.f, 0.48f, 0.f}, {0.7f, 0.08f, glm::length(tip - root)}, rime);   // frost on top
            b.kit().rod(root + vec3{0.f, -6.f, 0.f}, root, 0.35f, steel, 6);
            b.kit().rod(root + vec3{0.f, -6.f, 6.f}, root + vec3{0.f, 0.f, -10.f}, 0.18f, steel, 5);
        }
        b.kit().box({x, 6.f, ez - reach}, {7.f, 1.f, 1.2f}, steel);
        b.kit(true).box({x, 5.4f, ez - reach}, {0.5f, 0.2f, 0.5f}, amber);
        // The chains: straight down, some with a frozen unit hung on the hook, some with a rack
        int chains = 3 + (int)(cold::hash(k, 82.f) * 3.f);
        for (int c = 0; c < chains; ++c) {
            float along = reach * (0.3f + 0.65f * (float)c / std::max(1, chains - 1));
            float cx = x + (cold::hash(k * 7 + c, 83.f) - 0.5f) * 5.f;
            vec3 top{cx, 5.5f, ez - along};
            float len = 40.f + 260.f * cold::hash(k * 7 + c, 84.f);
            b.kit().rod(top, top - vec3{0.f, len, 0.f}, 0.09f, steel, 4);
            vec3 end = top - vec3{0.f, len, 0.f};
            float what = cold::hash(k * 7 + c, 85.f);
            if (what < 0.55f) {   // a unit hung by the neck plate: a frosted body shape
                b.kit().box(end + vec3{0.f, -1.f, 0.f}, {0.55f, 1.f, 0.3f}, steelFrost);
                b.kit().box(end + vec3{0.f, -0.32f, 0.f}, {0.3f, 0.3f, 0.3f}, rime);
                b.kit().box(end + vec3{-0.16f, -2.3f, 0.f}, {0.18f, 1.1f, 0.18f}, steelFrost);
                b.kit().box(end + vec3{0.16f, -2.3f, 0.f}, {0.18f, 1.1f, 0.18f}, steelFrost);
            } else {   // a rack: a tall frame of shelves, each with a drum on it
                float h = 12.f + 20.f * cold::hash(k * 7 + c, 86.f);
                vec3 rc = end - vec3{0.f, h * 0.5f, 0.f};
                for (int sx = -1; sx <= 1; sx += 2)
                    for (int sz = -1; sz <= 1; sz += 2) b.kit().box(rc + vec3{sx * 1.6f, 0.f, sz * 1.f}, {0.15f, h, 0.15f}, steel);
                for (int sh = 0; sh < (int)(h / 2.5f); ++sh) {
                    vec3 y = rc + vec3{0.f, -h * 0.5f + 1.f + sh * 2.5f, 0.f};
                    b.kit().box(y, {3.4f, 0.1f, 2.2f}, steel);
                    b.kit().column(y + vec3{-0.8f, 0.05f, 0.f}, 0.45f, 1.2f, steelFrost, 8);
                    if (cold::hash(k * 31 + c * 7 + sh, 87.f) > 0.6f) b.kit(true).box(y + vec3{1.f, 0.4f, 0.9f}, {0.12f, 0.12f, 0.05f}, amber * 0.7f);
                }
            }
        }
    }

    // ---- the Void's bottom, as far as anyone will ever see: black ----
    b.mat = Mat::METAL;
    b.kit().box({0.f, cold::VOID_FLOOR + 60.f, -cold::FAR_GROUND * 0.5f + cold::EDGE_Z * 0.5f},
                {cold::FAR_GROUND * 2.f, 2.f, cold::FAR_GROUND + cold::EDGE_Z}, vec3{0.004f, 0.005f, 0.008f});

    // ---- the Void's east end: an ice cliff where the frozen sea begins ----
    b.mat = Mat::ICE;
    {
        float zS = cold::edgeZ(cold::SEA_X0 - 1.f), zN = cold::SEA_Z1;
        b.kit().box({cold::SEA_X0 - 2.f, -450.f, (zS + zN) * 0.5f}, {4.f, 900.f, zS - zN}, iceDeep);
        b.kit().box({cold::SEA_X0 - 2.f, -30.f, (zS + zN) * 0.5f}, {4.2f, 60.f, zS - zN}, ice);
        // Under the sea ice: the seabed, dark, far down
        b.mat = Mat::ROCK;
        b.kit().box({(cold::SEA_X0 + cold::FAR_GROUND) * 0.5f, -82.f, (zN + cold::EDGE_Z) * 0.5f}, {cold::FAR_GROUND - cold::SEA_X0, 4.f, cold::EDGE_Z - zN + 60.f}, vec3{0.1f, 0.17f, 0.24f});   // (lighter than what lies on it, so their shapes read)
    }

    // ---- across the chasm: the Store, cut into a glacier cliff ----
    b.mat = Mat::ICE;
    for (int k = 0; k < 22; ++k) {   // the cliff: huge slabs of glacier ice, white on top, blue going down
        float x = -2700.f + k * 260.f;
        float top = 520.f + 300.f * cold::hash(k, 91.f);
        float z = cold::STORE_Z - 80.f * cold::hash(k, 92.f);
        b.kit().box({x, (top + cold::VOID_FLOOR) * 0.5f, z - 150.f}, {275.f, top - cold::VOID_FLOOR, 300.f}, glm::mix(ice, iceDeep, 0.35f * cold::hash(k, 93.f)));
        b.kit().box({x, top + 8.f, z - 150.f}, {280.f, 16.f, 304.f}, snow);   // the snow cap
        // Vertical fractures down its face
        for (int f = 0; f < 3; ++f) {
            float fx = x + (cold::hash(k * 3 + f, 94.f) - 0.5f) * 220.f;
            b.kit().box({fx, top * 0.4f, z + 1.f}, {3.f + 6.f * cold::hash(k * 3 + f, 95.f), top * 1.1f, 2.f}, iceDeep * 0.6f);
        }
    }
    // The Store's face: a black steel frame set into the ice, grids of amber windows
    b.mat = Mat::METAL;
    for (int k = 0; k < 9; ++k) {
        float x = -1200.f + k * 300.f + (cold::hash(k, 101.f) - 0.5f) * 80.f;
        float w = 140.f + 80.f * cold::hash(k, 102.f), h = 220.f + 120.f * cold::hash(k, 103.f);
        float y0 = -120.f + 80.f * cold::hash(k, 104.f);
        float z = cold::STORE_Z + 4.f;
        b.kit().box({x, y0 + h * 0.5f, z}, {w, h, 6.f}, steel);
        for (int row = 0; row < (int)(h / 22.f); ++row) {   // rows of windows, as strips
            if (cold::hash(k * 977 + row, 105.f) < 0.3f) continue;
            float sw = w * (0.3f + 0.6f * cold::hash(k * 13 + row, 104.f));
            float sx = x + (cold::hash(k * 17 + row, 103.f) - 0.5f) * (w - sw);
            b.kit(true).box({sx, y0 + 11.f + row * 22.f, z + 3.2f}, {sw, 4.f, 0.5f}, (cold::hash(k * 13 + row, 106.f) > 0.8f ? sodium : amber) * 0.8f);
        }
    }
    // Storage towers rising out of the Void: dark, ribbed, frost-capped, lamps in rows up their faces
    for (const auto& t : cold::towers()) {
        const float n = t.base.x * 0.13f;
        // (in three lengths, darker going down: below the shelf they sink into the Void's black)
        const float y0 = t.base.y, y1 = -420.f, y2 = -120.f, yTop = t.base.y + t.height;
        b.kit().box({t.base.x, (y0 + y1) * 0.5f, t.base.z}, {t.half * 2.f, y1 - y0, t.half * 2.f}, steel * 0.12f);
        b.kit().box({t.base.x, (y1 + y2) * 0.5f, t.base.z}, {t.half * 2.f, y2 - y1, t.half * 2.f}, steel * 0.45f);
        b.kit().box({t.base.x, (y2 + yTop) * 0.5f, t.base.z}, {t.half * 2.f, yTop - y2, t.half * 2.f}, steel);
        b.kit().box(t.base + vec3{0.f, t.height + 6.f, 0.f}, {t.half * 2.1f, 12.f, t.half * 2.1f}, rime);
        for (int r = 0; r < 4; ++r) {   // ribs at the corners
            float sx = (r & 1) ? 1.f : -1.f, sz = (r & 2) ? 1.f : -1.f;
            b.kit().box({t.base.x + sx * t.half, (y1 + yTop) * 0.5f, t.base.z + sz * t.half}, {5.f, yTop - y1 + 4.f, 5.f}, steelFrost * 0.4f);   // (lit only above the dark)
        }
        // Lamps in rows on the face toward the shelf (south)
        // (each floor's lamps as two or three lit strips, not one box a lamp: from a kilometre off
        // they read the same, and thousands of boxes cost frames)
        for (float y = -cold::VOID_FLOOR - 250.f; y < t.height - 20.f; y += 40.f) {   // (from just below the shelf: lower down nobody sees them)
            float u = cold::hash(n + y * 0.1f, 107.f);
            if (u < 0.2f) continue;   // a dark floor
            int strips = 1 + (int)(u * 2.9f);
            for (int q = 0; q < strips; ++q) {
                float w = t.half * (0.25f + 0.35f * cold::hash(n + y + q, 108.f));
                float cx = -t.half * 0.7f + (t.half * 1.4f) * (q + 0.5f) / strips;
                b.kit(true).box(t.base + vec3{cx, y, t.half + 0.6f}, {w, 2.4f, 0.4f}, (cold::hash(n + y + q, 109.f) > 0.85f ? sodium : amber) * 0.9f);
            }
        }
        // A dispatch rail up one side (the capsules ride it; Gameplay_Cold.h)
        b.kit().box(t.base + vec3{t.half + 4.f, t.height * 0.5f + 40.f, 0.f}, {3.f, t.height + 80.f, 3.f}, steel);
    }
    // Hook rails strung between the towers, and a few reaching back across to the shelf's gantries
    {
        const auto T = cold::towers();
        for (size_t k = 0; k + 1 < T.size(); ++k) {
            const auto& a = T[k]; const auto& c = T[k + 1];
            float y = std::min(a.height, c.height) * (0.35f + 0.4f * cold::hash((float)k, 111.f)) + cold::VOID_FLOOR;
            b.kit().rod(vec3{a.base.x, y, a.base.z}, vec3{c.base.x, y + 20.f * (cold::hash((float)k, 112.f) - 0.5f), c.base.z}, 1.2f, steel, 5);
        }
        for (int k = 0; k < 4; ++k) {   // back to the gantries: the Hook Line's rails, sagging slightly
            float x = gantryX[k];
            const auto& t = T[6 + k];
            vec3 from{x, 6.f, cold::edgeZ(x) - 40.f}, to{t.base.x, 60.f, t.base.z + t.half};
            for (int seg = 0; seg < 6; ++seg) {
                float u0 = seg / 6.f, u1 = (seg + 1) / 6.f;
                vec3 p0 = glm::mix(from, to, u0) - vec3{0.f, 30.f * std::sin(u0 * 3.1416f), 0.f};
                vec3 p1 = glm::mix(from, to, u1) - vec3{0.f, 30.f * std::sin(u1 * 3.1416f), 0.f};
                b.kit().rod(p0, p1, 0.45f, steel, 5);
            }
        }
    }

    // ---- round the horizon: ice mountains, glaciers, and the frozen giants in them ----
    b.mat = Mat::ICE;
    for (int k = 0; k < 40; ++k) {
        float a = (float)k / 40.f * 6.283f + cold::hash(k, 121.f) * 0.12f;
        vec3 dir{std::cos(a), 0.f, std::sin(a)};
        if (dir.z < -0.55f) continue;   // (north is the Store and the Void)
        float d = 1700.f + 1500.f * cold::hash(k, 122.f);
        vec3 p = dir * d + vec3{0.f, -20.f, 0.f};
        float h = 180.f + 520.f * cold::hash(k, 123.f);
        lump(p, 90.f + 120.f * cold::hash(k, 124.f), h, glm::mix(snowBlue, ice, cold::hash(k, 125.f) * 0.6f), (uint32_t)(900 + k),
             (cold::hash(k, 126.f) - 0.5f) * 0.3f, a, 4);
    }
    for (const auto& g : cold::giants()) {
        const float H = g.height, n = g.pos.x * 0.01f;
        const vec3 fwd{std::sin(g.yaw), 0.f, std::cos(g.yaw)}, right{fwd.z, 0.f, -fwd.x};
        auto at = [&](float r, float y, float f) { return g.pos + right * r + fwd * f + vec3{0.f, y, 0.f}; };
        const vec3 hull = glm::mix(steel, steelFrost, 0.55f);
        b.mat = Mat::METAL;
        // Legs (mostly under the glacier), hips, torso, shoulders, head
        b.kit().box(at(-H * 0.07f, H * 0.2f, 0.f), {H * 0.09f, H * 0.42f, H * 0.1f}, hull, g.yaw);
        b.kit().box(at(H * 0.07f, H * 0.2f, 0.f), {H * 0.09f, H * 0.42f, H * 0.1f}, hull, g.yaw);
        b.kit().box(at(0.f, H * 0.46f, 0.f), {H * 0.24f, H * 0.12f, H * 0.14f}, hull, g.yaw);
        b.kit().box(at(0.f, H * 0.64f, 0.f), {H * 0.28f, H * 0.26f, H * 0.16f}, hull, g.yaw, 0.08f);
        b.kit().box(at(0.f, H * 0.79f, 0.f), {H * 0.42f, H * 0.07f, H * 0.17f}, hull, g.yaw);
        b.kit().box(at(0.f, H * 0.89f, H * 0.01f), {H * 0.1f, H * 0.11f, H * 0.11f}, hull, g.yaw);
        b.kit(true).box(at(0.f, H * 0.9f, H * 0.065f), {H * 0.06f, H * 0.012f, H * 0.01f}, vec3{0.25f, 0.32f, 0.4f}, g.yaw);   // its eye, long dead: a dull slot
        // Arms: hanging, or one raised
        for (int side = -1; side <= 1; side += 2) {
            vec3 sh = at(side * H * 0.22f, H * 0.78f, 0.f);
            bool up = g.armRaised && side > 0;
            vec3 hand = up ? sh + vec3{0.f, H * 0.38f, 0.f} + fwd * (H * 0.12f) + right * (H * 0.05f)
                           : sh + vec3{0.f, -H * 0.4f, 0.f} + fwd * (H * 0.04f) + right * (side * H * 0.03f);
            b.kit().rod(sh, hand, H * 0.045f, hull, 6);
            b.kit().box(hand + (up ? vec3{0.f, H * 0.04f, 0.f} : vec3{0.f, -H * 0.04f, 0.f}), {H * 0.06f, H * 0.08f, H * 0.04f}, hull, g.yaw);
        }
        // Frost lying on its shoulders and head
        b.kit().box(at(0.f, H * 0.83f, 0.f), {H * 0.4f, H * 0.012f, H * 0.16f}, rime, g.yaw);
        b.kit().box(at(0.f, H * 0.95f, H * 0.01f), {H * 0.1f, H * 0.01f, H * 0.1f}, rime, g.yaw);
        // The glacier it stands in: up past its hips, a slope of ice round it
        b.mat = Mat::ICE;
        lump(g.pos + vec3{0.f, -H * 0.05f, 0.f}, H * 0.45f, H * 0.55f, glm::mix(snow, ice, 0.4f), (uint32_t)(1200 + (int)n), 0.f, g.yaw, 3);
        lump(g.pos + fwd * (H * 0.35f) + vec3{0.f, -H * 0.05f, 0.f}, H * 0.5f, H * 0.3f, snowBlue, (uint32_t)(1300 + (int)n), 0.15f, g.yaw, 3);
    }

    // =========================================================================
    // THE ROUTE (rooms 1-5)
    // =========================================================================
    auto stencil = [&](const char* text, vec3 p, vec3 right, float px, vec3 col) {
        for (int i = 0; text[i]; ++i) {
            unsigned char ch = text[i]; if (ch < 32 || ch > 127) continue;
            for (int y = 0; y < 7; ++y) for (int x = 0; x < 5; ++x) if (PIXEL_FONT[ch - 32][y] & (1 << (4 - x))) {
                vec3 q = p + right * ((i * 6 + x) * px) - vec3{0.f, y * px, 0.f};
                b.prop(q.x, q.y, q.z, q.x + std::max(px * 0.85f, 0.02f), q.y + px * 0.85f, q.z + px * 0.85f, col);
            }
        }
    };
    const vec3 paint{0.86f, 0.62f, 0.2f}, panel{0.2f, 0.22f, 0.25f}, floorSteel{0.24f, 0.26f, 0.29f};
    const float HX = cold::HOOK_X;

    // ---- 1. THE INTAKE: a belt of frozen bodies running to the lip, to be hung (drawn live) ----
    b.mat = Mat::METAL;
    {
        const float z0 = 250.f, z1 = cold::lipZ() + 1.f;
        b.wall(HX - 1.6f, 0.f, z1, HX + 1.6f, 0.75f, z0, steel);                      // the belt bed (stand on it, jump it)
        b.kit().box({HX, 0.78f, (z0 + z1) * 0.5f}, {2.8f, 0.06f, z0 - z1}, vec3{0.1f, 0.1f, 0.11f});   // the belt
        for (float z = z1 + 2.f; z < z0; z += 6.f) {
            b.kit().box({HX - 1.7f, 0.5f, z}, {0.15f, 0.9f, 0.4f}, steelFrost * 0.5f);
            b.kit().box({HX + 1.7f, 0.5f, z}, {0.15f, 0.9f, 0.4f}, steelFrost * 0.5f);
        }
        for (float z = z1 + 10.f; z < z0; z += 30.f) b.kit(true).box({HX + 1.8f, 1.f, z}, {0.12f, 0.12f, 0.3f}, amber);
        // Where it starts: a low receiving shed, half drifted over
        b.wall(HX - 8.f, 0.f, z0, HX + 8.f, 6.f, z0 + 14.f, steelFrost * 0.6f);
        b.kit().box({HX, 6.4f, z0 + 7.f}, {17.f, 0.8f, 15.f}, rime);
        b.kit(true).box({HX, 3.2f, z0 - 0.1f}, {4.f, 2.6f, 0.2f}, amber * 0.6f);
        stencil("INTAKE 5", {HX - 4.f, 5.4f, z0 + 14.15f}, {1.f, 0.f, 0.f}, 0.18f, paint);   // (on the face you walk up to)
        b.kit(true).box({HX, 2.f, z0 + 14.1f}, {6.f, 0.3f, 0.2f}, amber * 0.6f);
    }

    // ---- 2. THE HOOK LINE: a gantry off the lip, hanging stations, carriers shuttling between them ----
    {
        const float z0 = cold::lipZ() + 2.f, tip = cold::stationZ(0);
        b.wall(HX - 2.5f, -0.6f, tip - cold::STATION_HALF, HX + 2.5f, 0.f, z0, floorSteel);   // the gantry deck
        b.kit().box({HX, 0.04f, (tip + z0) * 0.5f}, {5.1f, 0.06f, z0 - tip + cold::STATION_HALF}, rime * 0.9f);
        for (int s = -1; s <= 1; s += 2) {   // its rails (waist high: you can still jump off)
            b.wall(HX + s * 2.45f, 0.f, tip - cold::STATION_HALF, HX + s * 2.55f, 1.1f, z0, steel);
            b.kit().box({HX + s * 2.5f, -1.2f, (tip + z0) * 0.5f}, {0.4f, 2.4f, z0 - tip}, steel);   // girders under it
        }
        stencil("HOOK LINE", {HX - 3.2f, 3.5f, z0 + 0.4f}, {1.f, 0.f, 0.f}, 0.12f, paint);
        b.kit().box({HX, 3.f, z0 + 0.5f}, {7.f, 1.2f, 0.2f}, steel);
        // The rail overhead, all the way to the tower
        b.kit().box({HX, cold::RAIL_Y, (tip + cold::TOWER_FRONT) * 0.5f}, {0.9f, 0.9f, tip - cold::TOWER_FRONT + 10.f}, steel);
        for (float z = tip + 4.f; z > cold::TOWER_FRONT; z -= 40.f) b.kit(true).box({HX, cold::RAIL_Y - 0.5f, z}, {0.25f, 0.12f, 0.25f}, amber);
        b.kit().rod({HX, 0.f, z0 - 1.f}, {HX, cold::RAIL_Y, z0 - 1.f}, 0.3f, steel, 6);   // its post at the lip
        // Stations: racks hung off the rail with a deck to stand on
        for (int k = 1; k < cold::CARRIERS; ++k) {
            float z = cold::stationZ(k), h = cold::STATION_HALF;
            b.wall(HX - h, -0.6f, z - h, HX + h, 0.f, z + h, floorSteel);
            b.kit().box({HX, 0.04f, z}, {2.f * h, 0.06f, 2.f * h}, rime * 0.85f);
            for (int s = -1; s <= 1; s += 2) b.kit().rod({HX + s * (h - 0.2f), 0.f, z}, {HX, cold::RAIL_Y, z}, 0.08f, steel, 4);
            for (int c = 0; c < 3; ++c) b.kit().rod({HX - 1.5f + c * 1.5f, -0.6f, z + h - 0.3f}, {HX - 1.5f + c * 1.5f, -14.f - 8.f * cold::hash((float)k, 200.f + c), z + h - 0.3f}, 0.05f, steel, 4);
            b.kit(true).box({HX + h - 0.3f, 0.6f, z + h - 0.3f}, {0.2f, 1.2f, 0.2f}, amber * 0.8f);
            b.kit().box({HX, -0.8f, z}, {2.f * h - 0.4f, 0.4f, 2.f * h - 0.4f}, steel);
        }
        // The carriers: each shuttles between two stations, lingering at each end to board
        for (int k = 0; k < cold::CARRIERS; ++k) {
            float from = cold::stationZ(k) - cold::STATION_HALF - cold::CARRIER_HALF - 0.15f;
            float to = cold::stationZ(k + 1) + cold::STATION_HALF + cold::CARRIER_HALF + 0.15f;
            b.mover({HX, -0.25f, from}, {cold::CARRIER_HALF, 0.25f, cold::CARRIER_HALF}, Mover::Path::PINGPONG,
                    {0.f, 0.f, 0.f}, {0.f, 0.f, to - from}, 15.f, 0.13f * k, vec3{1.6f, 0.8f, 0.2f});
        }
        // The dock: a ledge on the tower's face
        float dz = cold::stationZ(cold::CARRIERS);
        b.wall(HX - 6.f, -0.6f, cold::TOWER_FRONT, HX + 6.f, 0.f, dz + cold::STATION_HALF, floorSteel);
        b.wall(HX - 6.f, 0.f, cold::TOWER_FRONT, HX - 5.8f, 1.1f, dz + cold::STATION_HALF, steel);
        b.wall(HX + 5.8f, 0.f, cold::TOWER_FRONT, HX + 6.f, 1.1f, dz + cold::STATION_HALF, steel);
    }

    // ---- the Receiving Tower: standing in the Void, rooms 3-5 inside it at the shelf's level ----
    const float X0 = cold::TOWER_X0, X1 = cold::TOWER_X1, ZF = cold::TOWER_FRONT, ZB = cold::TOWER_BACK, C = cold::CEIL;
    l.basins.insert(l.basins.begin(), {LevelBuilder::aabb(X0, 0.f, ZB, X1, 0.f, ZF + 12.f), 0.f});   // (its floor, ahead of the Void's)
    {
        b.mat = Mat::METAL;
        const vec3 wallC{0.13f, 0.14f, 0.16f};
        b.wall(X0, -0.6f, ZB, X1, 0.f, ZF, floorSteel);                       // the floor
        b.wall(X0, C, ZB, X1, C + 0.6f, ZF, wallC);                           // the ceiling
        using Gap = LevelBuilder::Gap;
        const float T = 0.6f;
        // Front (the dock door), the walls between the rooms, the back (shut: the way on is not open yet)
        b.wallX(X0, X1, ZF - T, ZF, 0.f, C, wallC, {Gap{cold::DOOR_X0, cold::DOOR_X1, 0.f, 6.f}});
        b.wallX(X0, X1, cold::THAW_BACK - T, cold::THAW_BACK, 0.f, C, wallC, {Gap{cold::DOOR_X0, cold::DOOR_X1, 0.f, 6.f}});
        b.wallX(X0, X1, cold::INV_BACK - T, cold::INV_BACK, 0.f, C, wallC, {Gap{cold::DOOR_X0, cold::DOOR_X1, 0.f, 6.f}});
        b.wallX(X0, X1, ZB, ZB + T, 0.f, C, wallC, {Gap{cold::DOOR_X0, cold::DOOR_X1, 0.f, 6.f}});
        // Inventory is a narrow room: solid on either side of it
        b.wall(X0, 0.f, cold::INV_BACK, -60.f, C, cold::THAW_BACK - T, wallC);
        b.wall(-20.f, 0.f, cold::INV_BACK, X1, C, cold::THAW_BACK - T, wallC);
        // The long side walls, with tall windows onto the glacier in the Thaw Room and Rack 24
        std::vector<Gap> win;
        for (float z = ZB + 8.f; z < ZF - 6.f; z += 14.f) {
            if (z > cold::INV_BACK - 2.f && z < cold::THAW_BACK + 6.f) continue;   // (Inventory has none)
            win.push_back({z, z + 7.f, 2.f, 11.f});
        }
        b.wallZ(ZB, ZF, X0, X0 + T, 0.f, C, wallC, win);
        b.wallZ(ZB, ZF, X1 - T, X1, 0.f, C, wallC, win);
        // Window bars
        for (const auto& g : win) for (float x : {X0 + 0.3f, X1 - 0.3f})
            for (int k = 1; k < 4; ++k) b.kit().box({x, 6.5f, g.a0 + k * 1.75f}, {0.12f, 9.f, 0.12f}, steel);
        // The tower outside: up into the dark, down into the Void, lamps on its face
        b.kit().box({(X0 + X1) * 0.5f, C + 120.f, (ZF + ZB) * 0.5f}, {X1 - X0 + 1.f, 240.f, ZF - ZB + 1.f}, steel);
        b.kit().box({(X0 + X1) * 0.5f, C + 242.f, (ZF + ZB) * 0.5f}, {X1 - X0 + 3.f, 6.f, ZF - ZB + 3.f}, rime);
        b.kit().box({(X0 + X1) * 0.5f, -300.f, (ZF + ZB) * 0.5f}, {X1 - X0 + 1.f, 599.f, ZF - ZB + 1.f}, steel * 0.3f);
        for (float y = 22.f; y < C + 230.f; y += 24.f) b.kit(true).box({(X0 + X1) * 0.5f, y, ZF + 0.6f}, {(X1 - X0) * 0.7f, 1.6f, 0.3f}, amber * 0.8f);
        stencil("RECEIVING", {HX - 6.5f, 10.f, ZF + 0.7f}, {1.f, 0.f, 0.f}, 0.22f, paint);
        // Light inside: amber strips along the ceiling
        for (float z = ZB + 6.f; z < ZF - 3.f; z += 10.f) b.neon(X0 + 6.f, C - 0.3f, z, X1 - 6.f, C - 0.15f, z + 0.3f, amber * 0.35f);
    }

    // ---- 3. THE THAW ROOM: ceiling rails, hooks, drains in the floor ----
    for (int row = 0; row < 3; ++row) {
        float z = -612.f - row * 13.f;
        b.kit().box({(X0 + X1) * 0.5f, C - 1.2f, z}, {X1 - X0 - 2.f, 0.35f, 0.35f}, steel);   // a rail
    }
    for (const auto& h : cold::thawHooks()) b.kit().rod(h + vec3{0.f, C - 1.2f, 0.f}, h + vec3{0.f, 4.6f, 0.f}, 0.05f, steel, 4);
    for (int k = 0; k < 4; ++k) b.kit().box({-62.f + k * 16.f, 0.02f, -630.f}, {3.f, 0.04f, 1.f}, vec3{0.05f, 0.06f, 0.07f});   // drains
    // Cover: thawing tanks
    for (const auto& p : {vec3{-58.f, 0.f, -624.f}, vec3{-22.f, 0.f, -624.f}, vec3{-51.f, 0.f, -641.f}, vec3{-70.f, 0.f, -642.f}, vec3{-10.f, 0.f, -642.f}}) {
        b.wall(p.x - 1.6f, 0.f, p.z - 1.6f, p.x + 1.6f, 2.4f, p.z + 1.6f, steelFrost * 0.5f);
        b.kit().column(p + vec3{0.f, 2.4f, 0.f}, 1.3f, 0.5f, rime, 10);
        b.kit(true).box(p + vec3{0.f, 1.6f, 1.62f}, {0.8f, 0.3f, 0.05f}, amber * 0.7f);
    }
    stencil("THAW", {-45.f, 11.f, cold::THAW_BACK + 0.1f}, {1.f, 0.f, 0.f}, 0.3f, paint);

    // ---- 4. INVENTORY: terminals, one line each ----
    auto terminal = [&](vec3 p, float yaw, const char* title, const char* line) {
        b.place("console", p, yaw, 0.6f);
        b.wall(p.x - 0.65f, p.y, p.z - 0.4f, p.x + 0.65f, p.y + 1.1f, p.z + 0.4f, panel);
        s.terminals.push_back({p, title, line});
    };
    terminal({-56.f, 0.f, -657.f}, 90.f, "INVENTORY: UNIT 07", "UNIT 07: RESTORED 41 TIMES.");
    terminal({-24.f, 0.f, -662.f}, -90.f, "INVENTORY: UNIT 12", "UNIT 12: RESTORED. DID NOT RESUME.");
    terminal({-56.f, 0.f, -668.f}, 90.f, "INVENTORY: UNIT 19", "UNIT 19: STORED WHOLE. ATTEMPTS TO WAKE: 0. NONE REQUIRED.");
    terminal({-24.f, 0.f, -674.f}, -90.f, "INVENTORY: UNIT 24", "UNIT 24: RECALLED FOR CORRECTION. COPIES ON FILE: 24-A TO 24-K.");
    for (float z = -676.f; z < -652.f; z += 6.f) {   // filing drawers up both walls
        b.kit().box({-59.5f, 5.f, z}, {0.6f, 9.f, 5.6f}, steelFrost * 0.45f);
        b.kit().box({-20.5f, 5.f, z}, {0.6f, 9.f, 5.6f}, steelFrost * 0.45f);
    }

    // ---- 5. RACK 24: an aisle of alcoves (the copies are drawn live), 24-K's slot empty at the end ----
    for (int k = 0; k <= 10; ++k) {
        vec3 p = cold::rackSlot(k);
        bool west = k < 10 && k % 2 == 0, endSlot = k == 10;
        float back = endSlot ? p.z - 1.6f : 0.f;
        if (endSlot) {
            b.kit().box({p.x, 4.f, back}, {3.4f, 8.f, 0.3f}, steelFrost * 0.5f);
            stencil("24-K", {p.x - 1.1f, 6.6f, back + 0.2f}, {1.f, 0.f, 0.f}, 0.1f, paint);
            b.kit(true).box({p.x, 0.05f, p.z}, {2.f, 0.04f, 2.f}, amber * 0.4f);   // its empty plinth, lit
        } else {
            float bx = west ? p.x - 1.6f : p.x + 1.6f;
            b.kit().box({bx, 4.f, p.z}, {0.3f, 8.f, 3.4f}, steelFrost * 0.5f);              // the alcove's back
            b.wall(p.x - 1.f, 0.f, p.z - 1.f, p.x + 1.f, 0.25f, p.z + 1.f, steel);          // its plinth
            char label[8]; std::snprintf(label, sizeof(label), "24-%c", 'A' + k);
            stencil(label, {west ? p.x + 0.9f : p.x - 0.9f, 3.f, west ? p.z + 0.85f : p.z - 0.85f}, {0.f, 0.f, west ? -1.f : 1.f}, 0.08f, paint);   // (read from the aisle)
            b.kit().box({west ? p.x - 1.3f : p.x + 1.3f, 7.8f, p.z}, {0.6f, 0.2f, 3.f}, steel);   // its bracket
            b.kit(true).box({west ? p.x - 1.2f : p.x + 1.2f, 7.6f, p.z}, {0.15f, 0.08f, 2.4f}, vec3{0.6f, 0.75f, 0.9f} * 0.6f);
        }
    }
    stencil("RACK 24", {-48.f, 11.f, cold::INV_BACK - 0.7f}, {1.f, 0.f, 0.f}, 0.3f, paint);
    stencil("TO COLD STORAGE", {-46.f, 8.f, ZB + 0.7f}, {1.f, 0.f, 0.f}, 0.15f, paint);

    // ---- 6. THE BLAST CHILLERS ----
    b.mat = Mat::METAL;
    {
        // The transfer bridge: north out of Rack 24's back door, then east to the first deck
        const float bz0 = cold::BRIDGE_Z0, bz1 = cold::BRIDGE_Z1, dx0 = cold::DOOR_X0, dx1 = cold::DOOR_X1;
        const float deckW = cold::chiller(0).c.x - cold::chiller(0).r;
        b.wall(dx0, -0.6f, bz0, dx1, 0.f, ZB, floorSteel);
        b.wall(dx0, -0.6f, bz0, deckW + 2.f, 0.f, bz1, floorSteel);
        b.wall(dx0 - 0.2f, 0.f, bz1, dx0, 1.1f, ZB, steel);   // rails
        b.wall(dx1, 0.f, bz1, dx1 + 0.2f, 1.1f, ZB, steel);
        b.wall(dx1, 0.f, bz1, deckW + 1.f, 1.1f, bz1 + 0.2f, steel);
        b.wall(dx0 - 0.2f, 0.f, bz0 - 0.2f, deckW + 1.f, 1.1f, bz0, steel);
        for (float x = dx0; x < deckW; x += 24.f) {   // under it: trusses down into the dark
            b.kit().box({x, -6.f, (bz0 + bz1) * 0.5f}, {0.8f, 12.f, 0.8f}, steel);
            b.kit(true).box({x, 1.4f, bz1 + 0.15f}, {0.2f, 0.2f, 0.2f}, amber * 0.8f);
        }
        b.kit().box({cold::HOOK_X, -6.f, (ZB + bz0) * 0.5f}, {0.8f, 12.f, ZB - bz0}, steel);
        // The decks: cooling towers' tops, round, a hyperboloid falling away under each into the Void.
        // (Collision: a cross of boxes inside the round top.)
        for (int k = 0; k < 3; ++k) {
            const auto d = cold::chiller(k);
            const float r = d.r, s = r * 0.7071f, w = r * 0.42f;
            b.wall(d.c.x - s, -0.6f, d.c.z - s, d.c.x + s, 0.f, d.c.z + s, floorSteel);
            b.wall(d.c.x - r + 0.5f, -0.6f, d.c.z - w, d.c.x + r - 0.5f, 0.f, d.c.z + w, floorSteel);
            b.wall(d.c.x - w, -0.6f, d.c.z - r + 0.5f, d.c.x + w, 0.f, d.c.z + r - 0.5f, floorSteel);
            b.kit().column(d.c + vec3{0.f, -0.7f, 0.f}, r, 0.7f, floorSteel, 24);
            b.kit().column(d.c + vec3{0.f, -0.2f, 0.f}, r + 0.6f, 0.9f, steelFrost * 0.5f, 24, 1.f).caps = false;   // its lip
            b.kit().column(d.c + vec3{0.f, -160.f, 0.f}, r * 1.5f, 159.f, steel * 0.6f, 24, 0.66f).caps = false;   // the tower falling away
            b.kit().column(d.c + vec3{0.f, -420.f, 0.f}, r * 1.9f, 260.f, steel * 0.25f, 24, 0.79f).caps = false;
            b.kit().column(d.c + vec3{0.f, 0.02f, 0.f}, r * 0.35f, 0.03f, vec3{0.05f, 0.06f, 0.07f}, 16);   // its fan well
            for (int g = 0; g < 8; ++g) {   // frost drifted against the lip
                float a = g * 0.785f + k;
                b.kit().box(d.c + vec3{std::cos(a) * (r - 1.2f), 0.15f, std::sin(a) * (r - 1.2f)}, {3.f, 0.3f, 1.2f}, rime, -a);
            }
            for (int g = 0; g < 2; ++g) {   // cover: compressor housings
                vec3 p = d.c + vec3{g ? -10.f : 9.f, 0.f, g ? -9.f : 10.f};
                b.wall(p.x - 1.8f, 0.f, p.z - 1.2f, p.x + 1.8f, 2.2f, p.z + 1.2f, steelFrost * 0.45f);
                b.kit(true).box(p + vec3{0.f, 1.6f, 1.22f}, {0.6f, 0.2f, 0.04f}, amber);
            }
        }
        // Catwalks between the decks (A-B, B-C) and the exit east toward the sea
        auto catwalk = [&](vec3 a, vec3 c, float half) {
            vec3 d = c - a; d.y = 0.f; float len = glm::length(d); vec3 u = d / len;
            int n = (int)std::ceil(len / 2.f);
            for (int i = 0; i <= n; ++i) {   // (axis boxes along a diagonal: overlapping steps)
                vec3 p = a + u * (len * i / n);
                b.wall(p.x - half, -0.6f, p.z - half, p.x + half, 0.f, p.z + half, floorSteel);
            }
            b.kit().box((a + c) * 0.5f + vec3{0.f, 0.05f, 0.f}, {2.f * half, 0.06f, len}, rime * 0.9f, -std::atan2(u.z, u.x) + 1.5708f);
        };
        catwalk(cold::chiller(0).c, cold::chiller(1).c, 1.6f);
        catwalk(cold::chiller(1).c, cold::chiller(2).c, 1.6f);
        b.wall(cold::chiller(2).c.x + cold::chiller(2).r - 2.f, -0.6f, cold::BRIDGE_Z0, cold::SEA_X0 + 2.f, 0.f, cold::BRIDGE_Z1, floorSteel);
        // The vents: grates (the freezing columns are live: Gameplay_Cold.h)
        for (int v = 0; v < cold::VENTS; ++v) {
            vec3 p = cold::vent(v);
            b.kit().column(p + vec3{0.f, 0.01f, 0.f}, cold::VENT_R, 0.06f, vec3{0.04f, 0.05f, 0.06f}, 16);
            for (int g = 0; g < 6; ++g) b.kit().box(p + vec3{0.f, 0.08f, -2.5f + g}, {cold::VENT_R * 1.6f, 0.05f, 0.12f}, steel);
        }
        stencil("BLAST CHILLERS", {cold::chiller(0).c.x - 18.f, 6.f, cold::chiller(0).c.z + cold::chiller(0).r + 0.2f}, {1.f, 0.f, 0.f}, 0.2f, paint);
    }
    // Their floor for the enemies (ahead of the Void's): the decks and the catwalks between them
    l.basins.insert(l.basins.begin(), {LevelBuilder::aabb(cold::chiller(0).c.x - 22.f, 0.f, -890.f, cold::chiller(2).c.x + 22.f, 0.f, -806.f), 0.f});

    // ---- 7. THE FROZEN SEA: clear ice, and what is stored under it ----
    {
        // RECLAIM v1: the first Gleaner, 300 m of it, face up under your crossing
        const vec3 tail = cold::reclaimTail(), head = cold::reclaimHead();
        const vec3 along = glm::normalize(head - tail), side = glm::normalize(glm::cross(along, vec3{0.f, 1.f, 0.f}));
        const float len = glm::length(head - tail), yaw = -std::atan2(along.z, along.x);
        const vec3 iron{0.035f, 0.03f, 0.03f}, rust{0.07f, 0.04f, 0.03f};
        b.mat = Mat::METAL;
        for (int k = 0; k < 26; ++k) {   // the spine: plates, swelling toward the chest
            float u = (k + 0.5f) / 26.f;
            float girth = 7.f + 16.f * std::sin(std::min(1.f, u * 1.3f) * 3.1416f * 0.9f);
            vec3 p = tail + (head - tail) * u + vec3{0.f, -girth * 0.3f, 0.f};
            b.kit().box(p, {len / 26.f * 0.9f, girth * 0.7f, girth}, k % 3 ? iron : rust, yaw, 0.f, 0.05f * std::sin(k * 1.3f));
            if (k % 2 == 0 && u > 0.3f) for (int s = -1; s <= 1; s += 2)   // ribs, splayed
                b.kit().rod(p + vec3{0.f, girth * 0.2f, 0.f}, p + side * (s * girth * 1.6f) + vec3{0.f, -girth * 0.4f, 0.f}, 1.2f, iron, 6);
        }
        for (int s = -1; s <= 1; s += 2) {   // its arms, flung out; hands open on the seabed
            vec3 sh = tail + (head - tail) * 0.78f + side * (s * 14.f);
            vec3 el = sh + side * (s * 55.f) - along * 30.f + vec3{0.f, -10.f, 0.f};
            vec3 wr = el + side * (s * 20.f) - along * 60.f + vec3{0.f, -12.f, 0.f};
            b.kit().rod(sh, el, 5.f, rust, 8); b.kit().rod(el, wr, 4.f, iron, 8);
            for (int f = 0; f < 4; ++f) b.kit().rod(wr, wr - along * (12.f + 3.f * f) + side * (s * (f - 1.5f) * 4.f), 1.1f, iron, 5);
        }
        // Its head: a long furnace mouth, its eye a dark slot
        b.kit().box(head + along * 12.f + vec3{0.f, -4.f, 0.f}, {30.f, 14.f, 22.f}, iron, yaw);
        b.kit(true).box(head + along * 26.f, {2.f, 1.6f, 12.f}, vec3{0.18f, 0.05f, 0.03f}, yaw);
        // Other things they stored in the sea: racks lying in rows deep down
        for (int k = 0; k < 40; ++k) {
            vec3 p{cold::SEA_X0 + 40.f + cold::hash(k, 301.f) * 1500.f, -40.f - 30.f * cold::hash(k, 302.f), -360.f - cold::hash(k, 303.f) * 880.f};
            if (glm::length(glm::vec2{p.x - head.x, p.z - head.z}) < 60.f) continue;
            b.kit().box(p, {14.f + 20.f * cold::hash(k, 304.f), 4.f, 4.f}, iron * 1.5f, cold::hash(k, 305.f) * 3.f);
        }
        for (int k = 0; k < 60; ++k) {   // close under the surface along the crossing: stored units, face up, arms out
            float u = cold::hash(k, 311.f);
            vec3 p = cold::seaStart() + (cold::station() - cold::seaStart()) * u + vec3{(cold::hash(k, 312.f) - 0.5f) * 80.f, -2.5f - 5.f * cold::hash(k, 313.f), (cold::hash(k, 314.f) - 0.5f) * 80.f};
            float a = cold::hash(k, 315.f) * 6.283f;
            b.kit().box(p, {0.5f, 0.3f, 1.8f}, steelFrost * 0.6f, a);
            b.kit().box(p + vec3{std::sin(a), 0.f, std::cos(a)} * 1.1f, {0.3f, 0.3f, 0.3f}, steelFrost * 0.7f, a);
            b.kit().box(p, {1.8f, 0.15f, 0.2f}, steelFrost * 0.5f, a);
        }
        // The far shore: a glacier tongue with a broken rail station on it
        const vec3 st = cold::station();
        b.mat = Mat::SNOW;
        lump(st + vec3{60.f, -6.f, -40.f}, 70.f, 30.f, glm::mix(snow, ice, 0.3f), 1501u, 0.f, 0.4f, 3);
        b.mat = Mat::METAL;
        b.wall(st.x - 18.f, -0.6f, st.z - 10.f, st.x + 18.f, 0.6f, st.z + 10.f, floorSteel);   // the platform (a step up)
        b.kit().box(st + vec3{0.f, 0.62f, 0.f}, {36.f, 0.06f, 20.f}, rime * 0.8f);
        for (int k = -2; k <= 2; ++k) b.wall(st.x + k * 8.f - 0.3f, 0.6f, st.z - 9.f, st.x + k * 8.f + 0.3f, 7.f, st.z - 8.4f, steel);   // canopy posts
        b.kit().box(st + vec3{-4.f, 7.4f, -6.f}, {30.f, 0.6f, 8.f}, steel, 0.f, 0.f, 0.06f);   // the canopy, sagging
        b.kit().box(st + vec3{0.f, 1.6f, -12.f}, {60.f, 0.5f, 1.2f}, steel);                    // the rail, running off
        b.kit().box(st + vec3{40.f, -1.f, -12.f}, {20.f, 0.5f, 1.2f}, steel, 0.f, 0.f, -0.2f);  // ...snapped, into the ice
        b.kit(true).box(st + vec3{0.f, 6.6f, -6.f}, {8.f, 0.2f, 0.3f}, amber * 0.7f);
        stencil("DISPATCH 2", {st.x - 6.f, 9.4f, st.z - 8.3f}, {1.f, 0.f, 0.f}, 0.2f, paint);
    }
    for (int k = 1; k < 14; ++k) {   // a line of lamp posts across the ice, so the way is never lost
        vec3 p = cold::seaStart() + (cold::station() - cold::seaStart()) * (k / 14.f) + vec3{(k % 2 ? 6.f : -6.f), 0.f, 0.f};
        b.kit().rod(p, p + vec3{0.f, 3.2f, 0.f}, 0.08f, steel, 6);
        b.kit(true).box(p + vec3{0.f, 3.2f, 0.f}, {0.22f, 0.12f, 0.22f}, amber);
    }
    s.end = LevelBuilder::aabb(cold::station().x - 18.f, 0.f, cold::station().z - 10.f, cold::station().x + 18.f, 8.f, cold::station().z + 10.f);   // the far shore's station: the chapter so far ends

    // =========================================================================
    // THE SECTIONS
    // =========================================================================
    auto section = [&](const char* name, const char* sub, AABB zone, vec3 start, float yaw) {
        Arena a;
        a.name = name; a.subtitle = sub; a.zone = zone; a.bounds = zone; a.bounds.min.y = std::max(zone.min.y, 0.f);
        a.playerStart = start; a.startYaw = yaw; a.maxAlive = 8; a.damageScale = 1.f;
        a.theme = cold::theme(); a.ambient = Ambient::WIND; a.space = ReverbSpace::OPEN; a.tone = RoomTone::YARD;
        a.voidY = -60.f;
        return a;
    };
    using W = WaveEntry;
    const float lip = cold::lipZ();
    // 1. the shelf
    Arena intake = section("THE INTAKE", "NOTHING HERE IS THROWN AWAY",
                           LevelBuilder::aabb(-cold::HALF_X, -1000.f, cold::EDGE_Z - 70.f, cold::HALF_X, 800.f, cold::SOUTH), cold::start(), -90.f);
    intake.hasRespawn = true; intake.respawn = cold::start(); intake.scripted = true;
    l.arenas.push_back(intake);
    // 2. the Hook Line: Pickers come off the rails once you are out past the shelf's reach
    const int dockDoor = b.doorway(true, cold::DOOR_X0, cold::DOOR_X1, ZF - 0.6f, ZF, 0.f, 6.f, amber * 0.6f);
    Arena hook = section("THE HOOK LINE", "RIDE THE CARRIERS ACROSS",
                         LevelBuilder::aabb(HX - 7.f, -1000.f, ZF, HX + 7.f, 60.f, cold::EDGE_Z - 70.f), cold::hookStart(), -90.f);
    hook.zone.max.z = cold::EDGE_Z - 70.f;
    hook.hasRespawn = true; hook.respawn = cold::hookStart();
    hook.hasTrigger = true; hook.trigger = LevelBuilder::aabb(HX - 7.f, -5.f, ZF, HX + 7.f, 20.f, cold::EDGE_Z - 70.f);
    hook.exitDoor = dockDoor; hook.maxAlive = 3;
    hook.airSpawns = {{HX - 30.f, 8.f, cold::stationZ(2)}, {HX + 30.f, 8.f, cold::stationZ(2) - 20.f}, {HX + 25.f, 8.f, cold::stationZ(3)}, {HX - 25.f, 8.f, cold::stationZ(1)}};
    hook.waves = {{W(EnemyType::PICKER, 2)}, {W(EnemyType::PICKER, 3)}};
    l.arenas.push_back(hook);
    (void)lip;
    // 3. the Thaw Room: the hung units drop and wake
    const int thawDoor = b.doorway(true, cold::DOOR_X0, cold::DOOR_X1, cold::THAW_BACK - 0.6f, cold::THAW_BACK, 0.f, 6.f, amber * 0.6f);
    Arena thaw = section("THE THAW ROOM", "THEY ARE WARMING UP", LevelBuilder::aabb(X0 + 1.f, -5.f, cold::THAW_BACK, X1 - 1.f, C, ZF), {HX, 0.f, ZF - 3.f}, -90.f);
    thaw.hasTrigger = true; thaw.trigger = LevelBuilder::aabb(X0 + 1.f, -2.f, cold::THAW_BACK + 2.f, X1 - 1.f, 6.f, ZF - 6.f);
    thaw.entryGate = dockDoor; thaw.exitDoor = thawDoor; thaw.space = ReverbSpace::METAL; thaw.tone = RoomTone::FOUNDRY;
    thaw.ambient = Ambient::MOTES; thaw.maxAlive = 7;
    {
        auto H = cold::thawHooks();
        auto at = [&](std::initializer_list<int> ks) { std::vector<vec3> v; for (int k : ks) v.push_back(H[k]); return v; };
        thaw.waves = {{W(EnemyType::THAWLING, 0, at({0, 2, 4, 7, 9}))},
                      {W(EnemyType::THAWLING, 0, at({1, 3, 5, 12})), W(EnemyType::CLERK, 0, {{-40.f, 3.4f, -646.f}})},
                      {W(EnemyType::THAWLING, 0, at({6, 8, 10, 11})), W(EnemyType::PICKER, 0, {{-60.f, 4.6f, -640.f}}), W(EnemyType::CLERK, 0, {{-14.f, 3.4f, -645.f}})}};
    }
    l.arenas.push_back(thaw);
    // 4. Inventory (quiet)
    Arena inv = section("INVENTORY", "EVERYTHING ON FILE", LevelBuilder::aabb(-60.f, -5.f, cold::INV_BACK, -20.f, C, cold::THAW_BACK), {HX, 0.f, cold::THAW_BACK - 3.f}, -90.f);
    inv.space = ReverbSpace::METAL; inv.tone = RoomTone::NONE; inv.ambient = Ambient::DUST;
    l.arenas.push_back(inv);
    // 5. Rack 24 (quiet: the RECALL module, Gameplay_Cold.h)
    const int backDoor = b.doorway(true, cold::DOOR_X0, cold::DOOR_X1, ZB, ZB + 0.6f, 0.f, 6.f, amber * 0.6f, true);   // (opens when RECALL is taken)
    l.story.coldBackDoor = backDoor;
    Arena rack = section("RACK 24", "COPIES ON FILE", LevelBuilder::aabb(X0 + 1.f, -5.f, ZB + 0.6f, X1 - 1.f, C, cold::INV_BACK), {HX, 0.f, cold::INV_BACK - 3.f}, -90.f);
    rack.space = ReverbSpace::METAL; rack.tone = RoomTone::NONE; rack.ambient = Ambient::MOTES;
    l.arenas.push_back(rack);
    // 6. the Blast Chillers: a fight on the cooling towers, the vents firing all through it
    const float gateX = cold::chiller(2).c.x + cold::chiller(2).r;
    const int chillGate = b.doorway(false, cold::BRIDGE_Z0, cold::BRIDGE_Z1, gateX - 1.f, gateX - 0.4f, 0.f, 4.f, amber * 0.6f);
    Arena chill = section("THE BLAST CHILLERS", "MIND THE VENTS", LevelBuilder::aabb(cold::chiller(0).c.x - 22.f, -1000.f, -890.f, gateX - 0.4f, 60.f, -806.f),
                          {cold::chiller(0).c.x - 18.f, 0.f, (cold::BRIDGE_Z0 + cold::BRIDGE_Z1) * 0.5f}, 0.f);
    chill.extraZones.push_back(LevelBuilder::aabb(cold::DOOR_X0, -1000.f, cold::BRIDGE_Z0, cold::chiller(0).c.x - 20.f, 60.f, ZB + 1.f));   // the transfer bridge (overlapping Rack 24's zone through the door)
    chill.hasRespawn = true; chill.respawn = chill.playerStart;
    chill.hasTrigger = true; chill.trigger = LevelBuilder::aabb(cold::chiller(0).c.x - 12.f, -2.f, -890.f, gateX, 8.f, -806.f);
    chill.exitDoor = chillGate; chill.maxAlive = 7; chill.space = ReverbSpace::OPEN; chill.tone = RoomTone::YARD;
    {
        auto on = [&](int k, float a, float r) { auto d = cold::chiller(k); return d.c + glm::vec3{std::cos(a) * r, 0.f, std::sin(a) * r}; };
        const vec3 up{0.f, 4.6f, 0.f}, upC{0.f, 3.4f, 0.f};
        chill.waves = {{W(EnemyType::THAWLING, 0, {on(1, 0.3f, 12.f), on(1, 2.4f, 12.f), on(2, 1.f, 14.f), on(2, 4.6f, 13.f)}), W(EnemyType::PICKER, 0, {on(1, 1.5f, 6.f) + up})},
                       {W(EnemyType::THAWLING, 0, {on(0, 1.2f, 14.f), on(2, 2.2f, 14.f), on(1, 4.5f, 12.f), on(2, 5.3f, 12.f)}), W(EnemyType::CLERK, 0, {on(2, 0.f, 18.f) + upC}),
                        W(EnemyType::PICKER, 0, {on(0, 3.f, 8.f) + up})},
                       {W(EnemyType::THAWLING, 0, {on(0, 0.5f, 13.f), on(1, 1.6f, 13.f), on(1, 3.6f, 12.f), on(2, 0.6f, 12.f), on(2, 3.3f, 14.f)}),
                        W(EnemyType::PICKER, 0, {on(1, 0.f, 6.f) + up, on(2, 2.f, 6.f) + up}), W(EnemyType::CLERK, 0, {on(1, 4.7f, 17.f) + upC})}};
    }
    l.arenas.push_back(chill);
    // 7. the Frozen Sea: a long walk over clear ice (no fight: what is under it is the point)
    Arena sea = section("THE FROZEN SEA", "DO NOT LOOK DOWN FOR LONG", LevelBuilder::aabb(cold::SEA_X0, -10.f, -1080.f, 820.f, 60.f, -760.f), cold::seaStart(), -20.f);
    sea.extraZones.push_back(LevelBuilder::aabb(gateX - 0.4f, -1000.f, cold::BRIDGE_Z0, cold::SEA_X0, 60.f, cold::BRIDGE_Z1));   // the exit catwalk
    sea.hasRespawn = true; sea.respawn = cold::seaStart(); sea.ambient = Ambient::WIND;
    l.arenas.push_back(sea);
    return l;
}
