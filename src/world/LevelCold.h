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
constexpr float STORE_Z = -2600.f;            // the glacier cliff the Store is cut into (its towers stand south of it, clear of the route)
constexpr float SEA_X0 = 240.f;               // east of here the Void gives way to the frozen sea (rooms 6-7)
constexpr float SEA_Z1 = STORE_Z - 10.f;      // ...which runs north to the cliff
inline glm::vec3 start() { return {0.f, 0.f, -95.f}; }   // the portal's landing: about 250 m of shelf to the Hook Line
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
        float z = STORE_Z + 40.f + hash(k, 3.f) * 140.f;   // (well north of everything you walk on: tests/test_cold.cpp checks)
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
constexpr float STATION_HALF = 2.5f;
constexpr int   STATIONS = 4;                 // gaps between the gantry, three hanging decks and the dock
inline float lipZ() { return edgeZ(HOOK_X); }
inline float stationZ(int k) {   // 0: the gantry's end; 1..3: hanging stations; 4: the tower's dock
    const float z0 = lipZ() - 22.f, z4 = TOWER_FRONT + 10.f - STATION_HALF;
    return z0 + (z4 - z0) * (float)k / STATIONS;
}
inline glm::vec3 hookStart() { return {HOOK_X, 0.f, stationZ(0)}; }
constexpr float INTAKE_Z = -120.f;            // where the Intake belt starts (its shed), running north to the lip
inline float beltEndZ() { return lipZ() + 10.f; }   // the belt stops here, short of the gantry
inline float liftZ() { return beltEndZ() - 1.f; }   // ...where the crane lifts the bodies off it
inline glm::vec3 hookBeacon() { return {HOOK_X + 14.f, 0.f, lipZ() + 4.f}; }   // the tall mast marking the Hook Line
// The hooks you grapple across on: from just past the gantry's end to over the dock
constexpr int   HOOKS = 11;
constexpr float HOOK_Y = 8.4f;                // the hook block's centre (the rail is at RAIL_Y)
constexpr float BODY_RAIL_DX = 8.f;           // the rail the hung units still ride in on, east of the hooks
inline glm::vec3 hookAt(int k) {
    const float z0 = stationZ(0) - 14.f, z1 = TOWER_FRONT + 6.f;   // (the last over the dock: let go and you land on it)
    return {HOOK_X, HOOK_Y, z0 + (z1 - z0) * (float)k / (HOOKS - 1)};
}
// Where you hang under hook k: your feet, clear below it
inline glm::vec3 hangAt(int k) { return hookAt(k) - glm::vec3{0.f, 2.9f, 0.f}; }
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
inline glm::vec3 station() { return {600.f, 0.f, -985.f}; }
inline glm::vec3 stationBeacon() { return station() + glm::vec3{-20.f, 0.f, -12.f}; }   // DISPATCH 2's signal mast
// RECLAIM v1, the first Gleaner, lying under the ice along your crossing (its head toward the station)
inline glm::vec3 reclaimTail() { return {250.f, -24.f, -815.f}; }
inline glm::vec3 reclaimHead() { return {535.f, -24.f, -940.f}; }
// 9-10. THE MANIFEST and THE VAULT: a sinkhole in the sea ice, a round platform hanging in it
inline glm::vec3 vaultC() { return {760.f, 0.f, -1085.f}; }
constexpr float VAULT_R = 30.f;
inline AABB sinkhole() { return {{vaultC().x - 75.f, -1000.f, vaultC().z - 75.f}, {vaultC().x + 75.f, 0.f, vaultC().z + 75.f}}; }
constexpr int DRUMS = 4;   // one weapon in each (phase 3, FACTORY RESET)
inline glm::vec3 drum(int k) { float a = 0.785f + k * 1.5708f; return vaultC() + glm::vec3{std::cos(a) * (VAULT_R - 3.f), 0.f, std::sin(a) * (VAULT_R - 3.f)}; }
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

    // ---- the arrival: where the portal put you down, a square of the portal's grid plates in the snow ----
    {
        const vec3 a = cold::start();
        b.mat = Mat::METAL;
        b.kit().box(a + vec3{0.f, 0.02f, 0.f}, {9.f, 0.04f, 9.f}, steelFrost * 0.7f);
        for (int g = -4; g <= 4; ++g) {   // its grid, faintly lit (the Gleaner's portal showed this floor)
            b.kit(true).box(a + vec3{g * 1.f, 0.05f, 0.f}, {0.04f, 0.02f, 9.f}, vec3{0.25f, 0.35f, 0.45f} * 0.6f);
            b.kit(true).box(a + vec3{0.f, 0.05f, g * 1.f}, {9.f, 0.02f, 0.04f}, vec3{0.25f, 0.35f, 0.45f} * 0.6f);
        }
        for (int k = 0; k < 6; ++k) {   // snow already blowing over its edges
            float ang = k * 1.047f + 0.3f;
            b.kit().box(a + vec3{std::cos(ang) * 4.6f, 0.08f, std::sin(ang) * 4.6f}, {2.4f, 0.16f, 0.9f}, snow, -ang);
        }
    }

    // ---- marker poles: amber lamps from the arrival to the Hook Line's gantry, so the way is plain ----
    {
        const vec3 from = cold::start() + vec3{0.f, 0.f, -12.f}, to{cold::HOOK_X + 10.f, 0.f, cold::liftZ() + 12.f};   // (east of the belt, short of the crane)
        const float len = glm::length(to - from);
        const vec3 u = (to - from) / len, side{-u.z, 0.f, u.x};
        for (float d = 0.f; d < len; d += 24.f) {
            for (int sgn = -1; sgn <= 1; sgn += 2) {
                vec3 p = from + u * d + side * (sgn * 5.f);
                b.mat = Mat::METAL;
                b.kit().rod(p, p + vec3{0.f, 3.2f, 0.f}, 0.08f, steel, 6);
                b.kit().box(p + vec3{0.f, 3.35f, 0.f}, {0.35f, 0.3f, 0.35f}, steel);
                b.kit(true).box(p + vec3{0.f, 3.2f, 0.f}, {0.22f, 0.12f, 0.22f}, amber);
                b.kit().box(p + vec3{0.f, 3.52f, 0.f}, {0.42f, 0.06f, 0.42f}, rime);   // snow on its cap
            }
            vec3 m = from + u * d;
            b.kit(true).box(m + vec3{0.f, 0.05f, 0.f}, {0.5f, 0.02f, 0.5f}, amber * 0.25f);   // a lit stud in the snow, down the middle
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
        const float z0 = cold::INTAKE_Z, z1 = cold::beltEndZ();   // (it stops short of the lip: walk round its end onto the gantry)
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
        for (int k = 0; k < 3; ++k) b.kit().box({HX - 6.f + k * 6.f, 6.9f, z0 + 3.f}, {1.6f, 1.2f, 1.6f}, steel);   // vents on its roof
        stencil("INTAKE 5", {HX - 4.f, 5.4f, z0 + 14.15f}, {1.f, 0.f, 0.f}, 0.18f, paint);   // (on the face you walk up to)
        b.kit(true).box({HX, 2.f, z0 + 14.1f}, {6.f, 0.3f, 0.2f}, amber * 0.6f);
        // Its yard, west of the belt: frozen shipping cases, stacked and drifted over (cover, and something to see)
        b.mat = Mat::METAL;
        for (int k = 0; k < 9; ++k) {
            float x = HX - 14.f - (k % 3) * 8.f - 3.f * cold::hash(k, 211.f), z = z0 - 20.f - (k / 3) * 26.f - 6.f * cold::hash(k, 212.f);
            int high = 1 + (cold::hash(k, 213.f) > 0.5f ? 1 : 0);
            for (int h = 0; h < high; ++h) {
                float y0 = h * 2.6f, yaw = (cold::hash(k * 3 + h, 214.f) - 0.5f) * 0.3f;
                vec3 c = glm::mix(vec3{0.32f, 0.36f, 0.42f}, vec3{0.45f, 0.3f, 0.18f}, cold::hash(k * 3 + h, 215.f) > 0.7f ? 1.f : 0.f);
                b.wall(x - 3.f, y0, z - 1.2f, x + 3.f, y0 + 2.6f, z + 1.2f, c);
                for (int r = -2; r <= 2; ++r) b.kit().box({x + r * 1.4f, y0 + 1.3f, z}, {0.12f, 2.5f, 2.5f}, c * 0.7f, yaw * 0.f);   // its ribs
                b.kit().box({x, y0 + 2.66f, z}, {6.1f, 0.14f, 2.5f}, rime);                                                     // snow on it
                if (h == 0) b.kit().box({x + 2.6f, 0.35f, z + 1.6f}, {2.4f, 0.7f, 1.2f}, snow, 0.4f);                               // drifted against it
            }
            if (k % 4 == 1) stencil("24", {x - 0.6f, 1.8f, z + 1.25f}, {1.f, 0.f, 0.f}, 0.09f, paint * 0.7f);   // (someone's number)
        }
        // Light masts down the belt: floodlights, the snow lit amber under them
        for (float z = z0 - 30.f; z > z1 + 20.f; z -= 60.f) {
            vec3 m{HX - 5.f, 0.f, z};
            b.wall(m.x - 0.3f, 0.f, m.z - 0.3f, m.x + 0.3f, 1.f, m.z + 0.3f, steel);
            b.kit().rod(m, m + vec3{0.f, 13.f, 0.f}, 0.18f, steel, 6);
            b.kit().box(m + vec3{1.2f, 13.f, 0.f}, {2.8f, 0.3f, 0.5f}, steel);
            b.kit(true).box(m + vec3{1.9f, 12.75f, 0.f}, {1.1f, 0.2f, 0.4f}, amber * 1.4f);
            b.kit(true).box(m + vec3{1.6f, 0.02f, 0.f}, {7.f, 0.02f, 7.f}, amber * 0.06f);   // its pool of light on the snow
        }
        // At the belt's end: a crane that lifts the bodies, carries them east and hangs them on the
        // line's second rail, which takes them out to the tower (all drawn live: Gameplay_Cold.h)
        {
            const float cz = cold::liftZ(), xE = HX + cold::BODY_RAIL_DX + 1.f;
            for (float lx : {HX - 4.f, xE}) {
                b.wall(lx - 0.4f, 0.f, cz - 0.4f, lx + 0.4f, 11.f, cz + 0.4f, steel);
                b.kit().box({lx, 5.5f, cz}, {0.9f, 11.f, 0.9f}, steel * 1.4f);
                b.kit().box({lx, 11.1f, cz}, {1.f, 0.1f, 1.f}, rime);
            }
            b.kit().box({(HX - 4.f + xE) * 0.5f, 11.4f, cz}, {xE - HX + 5.f, 0.9f, 1.f}, steel * 1.4f);
            b.kit().box({(HX - 4.f + xE) * 0.5f, 11.9f, cz}, {xE - HX + 5.2f, 0.12f, 1.1f}, rime);
            b.kit(true).box({HX + 2.f, 10.85f, cz + 0.55f}, {0.4f, 0.2f, 0.1f}, amber);
            b.kit(true).box({xE - 1.f, 10.85f, cz + 0.55f}, {0.4f, 0.2f, 0.1f}, amber);
        }
        // The beacon at the Hook Line's gantry: a tall mast you can see from the arrival (its lamp blinks: Gameplay_Cold.h)
        {
            const vec3 m = cold::hookBeacon();   // (east of the crane, at the lamp line's end)
            b.wall(m.x - 0.5f, 0.f, m.z - 0.5f, m.x + 0.5f, 1.5f, m.z + 0.5f, steel);
            b.kit().box(m + vec3{0.f, 12.f, 0.f}, {0.6f, 24.f, 0.6f}, steel);
            for (float y = 3.f; y < 24.f; y += 3.f) b.kit().box(m + vec3{0.f, y, 0.f}, {1.2f, 0.12f, 1.2f}, steel * 1.3f);
            b.kit().box(m + vec3{0.f, 15.f, 0.6f}, {5.4f, 2.2f, 0.2f}, steel);
            stencil("HOOK LINE", m + vec3{-2.4f, 15.6f, 0.75f}, {1.f, 0.f, 0.f}, 0.1f, paint);
            b.kit(true).box(m + vec3{0.f, 14.2f, 0.75f}, {4.8f, 0.12f, 0.1f}, amber * 0.8f);
        }
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
        stencil("GRAPPLE THE HOOKS", {HX - 3.f, 2.5f, z0 + 0.4f}, {1.f, 0.f, 0.f}, 0.07f, paint * 0.8f);
        b.kit().box({HX, 3.f, z0 + 0.5f}, {7.f, 1.2f, 0.2f}, steel);
        // The rail overhead, all the way to the tower
        b.kit().box({HX, cold::RAIL_Y, (tip + cold::TOWER_FRONT) * 0.5f}, {0.9f, 0.9f, tip - cold::TOWER_FRONT + 10.f}, steel);
        for (float z = tip + 4.f; z > cold::TOWER_FRONT; z -= 40.f) b.kit(true).box({HX, cold::RAIL_Y - 0.5f, z}, {0.25f, 0.12f, 0.25f}, amber);
        for (int sg = -1; sg <= 1; sg += 2) b.kit().rod({HX + sg * 2.7f, 0.f, z0 - 1.f}, {HX + sg * 0.6f, cold::RAIL_Y, z0 - 1.f}, 0.22f, steel, 6);   // its posts at the lip (either side of the deck)
        // Stations: decks hung off the rail to rest on (and to come back to if you fall)
        for (int k = 1; k < cold::STATIONS; ++k) {
            float z = cold::stationZ(k), h = cold::STATION_HALF;
            b.wall(HX - h, -0.6f, z - h, HX + h, 0.f, z + h, floorSteel);
            b.kit().box({HX, 0.04f, z}, {2.f * h, 0.06f, 2.f * h}, rime * 0.85f);
            for (int s = -1; s <= 1; s += 2) b.kit().rod({HX + s * (h - 0.2f), 0.f, z}, {HX, cold::RAIL_Y, z}, 0.08f, steel, 4);
            for (int c = 0; c < 3; ++c) b.kit().rod({HX - 1.5f + c * 1.5f, -0.6f, z + h - 0.3f}, {HX - 1.5f + c * 1.5f, -14.f - 8.f * cold::hash((float)k, 200.f + c), z + h - 0.3f}, 0.05f, steel, 4);
            b.kit(true).box({HX + h - 0.3f, 0.6f, z + h - 0.3f}, {0.2f, 1.2f, 0.2f}, amber * 0.8f);
            b.kit().box({HX, -0.8f, z}, {2.f * h - 0.4f, 0.4f, 2.f * h - 0.4f}, steel);
        }
        // The hooks: empty, hung off the rail every 20 m or so from the gantry's end to the dock.
        // Grapple one and you hang under it; from there grapple the next (Gameplay_Cold.h). Each is
        // a solid block (what the grapple bites), a hook under it, and a lamp so the line reads at range.
        l.story.coldHooks = (int)l.walls.size();
        for (int k = 0; k < cold::HOOKS; ++k) {
            const vec3 h = cold::hookAt(k);
            b.wall(h.x - 0.45f, h.y - 0.6f, h.z - 0.45f, h.x + 0.45f, h.y + 0.6f, h.z + 0.45f, steel);
        }
        l.story.coldHookCount = cold::HOOKS;
        for (int k = 0; k < cold::HOOKS; ++k) {
            const vec3 h = cold::hookAt(k);
            b.kit().rod(h + vec3{0.f, 0.6f, 0.f}, {h.x, cold::RAIL_Y - 0.4f, h.z}, 0.07f, steel, 4);   // its hanger
            b.kit().box(h + vec3{0.f, 0.62f, 0.f}, {1.f, 0.08f, 1.f}, rime);                          // frost on it
            b.kit().box(h + vec3{0.f, -0.95f, 0.f}, {0.16f, 0.7f, 0.16f}, steelFrost * 0.6f);           // the hook's shank...
            b.kit().box(h + vec3{0.f, -1.35f, 0.25f}, {0.16f, 0.16f, 0.6f}, steelFrost * 0.6f);         // ...its curve...
            b.kit().box(h + vec3{0.f, -1.15f, 0.5f}, {0.16f, 0.45f, 0.16f}, steelFrost * 0.6f);         // ...and its point
            b.kit(true).box(h + vec3{0.f, 0.f, 0.47f}, {0.22f, 0.22f, 0.05f}, amber * 0.75f);          // its lamp, toward you
            b.kit(true).box(h + vec3{0.f, 0.f, -0.47f}, {0.22f, 0.22f, 0.05f}, amber * 0.35f);
        }
        // The line still runs: a second rail to the east, where hung units ride in to the tower (drawn live)
        b.kit().box({HX + cold::BODY_RAIL_DX, cold::RAIL_Y, (cold::liftZ() + cold::TOWER_FRONT) * 0.5f}, {0.7f, 0.7f, cold::liftZ() - cold::TOWER_FRONT}, steel);
        for (float z = tip + 10.f; z > cold::TOWER_FRONT; z -= 30.f) {   // braced to the main rail
            b.kit().box({HX + cold::BODY_RAIL_DX * 0.5f, cold::RAIL_Y + 0.6f, z}, {cold::BODY_RAIL_DX, 0.3f, 0.3f}, steel);
            b.kit().rod({HX + cold::BODY_RAIL_DX * 0.5f, cold::RAIL_Y + 0.6f, z}, {HX + cold::BODY_RAIL_DX * 0.5f, cold::RAIL_Y + 60.f, z - 30.f}, 0.12f, steel, 4);   // stays up into the dark
        }
        // The dock: a ledge on the tower's face
        float dz = cold::stationZ(cold::STATIONS);
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

    // ---- the tower's rooms, dressed: heat in the Thaw Room, aisles of files in Inventory, the copies'
    // cryo lines in Rack 24, and amber chevrons on every floor pointing at the way on ----
    {
        const vec3 heat{2.4f, 0.75f, 0.2f}, puddle{0.05f, 0.08f, 0.11f};
        b.mat = Mat::METAL;
        auto chevrons = [&](float zFrom, float zTo) {   // painted on the floor down the middle, pointing on (toward -z)
            for (float z = zFrom; z > zTo; z -= 4.f)
                for (int sg = -1; sg <= 1; sg += 2)
                    b.kit(true).box({HX + sg * 0.55f, 0.03f, z + 0.5f}, {1.4f, 0.02f, 0.22f}, amber * 0.22f, sg * 0.6f);
        };
        chevrons(ZF - 4.f, cold::THAW_BACK + 3.f);
        chevrons(cold::THAW_BACK - 3.f, cold::INV_BACK + 3.f);
        chevrons(cold::INV_BACK - 3.f, ZB + 3.f);
        // 3. THE THAW ROOM: heater banks glowing low on both walls, pipes overhead, meltwater on the floor
        for (float z = ZF - 6.f; z > cold::THAW_BACK + 3.f; z -= 14.f)
            for (float x : {X0 + 0.9f, X1 - 0.9f}) {
                b.wall(x - 0.35f, 0.f, z - 2.2f, x + 0.35f, 1.6f, z + 2.2f, steel);
                for (int c = 0; c < 4; ++c) b.kit(true).box({x + (x < HX ? 0.37f : -0.37f), 0.35f + c * 0.35f, z}, {0.04f, 0.1f, 4.f}, heat * (0.5f + 0.15f * c));
                b.kit().box({x, 1.7f, z}, {0.8f, 0.2f, 4.6f}, steelFrost * 0.5f);
            }
        for (float x : {X0 + 8.f, X0 + 10.f, X1 - 8.f}) {
            b.kit().box({x, C - 2.4f, (ZF + cold::INV_BACK) * 0.5f}, {0.6f, 0.6f, ZF - cold::INV_BACK}, steelFrost * 0.5f);
            for (float z = ZF - 10.f; z > cold::INV_BACK; z -= 16.f) b.kit().box({x, C - 2.4f, z}, {0.9f, 0.9f, 0.5f}, steel);   // its flanges
        }
        for (const auto& h : cold::thawHooks()) {   // a puddle under every hook, frozen at its edges
            b.kit().box(h + vec3{0.3f, 0.015f, 0.2f}, {2.2f + cold::hash(h.x, h.z) * 1.5f, 0.02f, 1.6f}, puddle, cold::hash(h.z, h.x) * 3.f);
            b.kit().box(h + vec3{0.3f, 0.01f, 0.2f}, {2.8f + cold::hash(h.x, h.z) * 1.5f, 0.015f, 2.1f}, rime * 0.7f, cold::hash(h.z, h.x) * 3.f);
        }
        for (const auto& p : {vec3{-58.f, 0.f, -624.f}, vec3{-22.f, 0.f, -624.f}, vec3{-51.f, 0.f, -641.f}, vec3{-70.f, 0.f, -642.f}, vec3{-10.f, 0.f, -642.f}}) {
            b.kit().rod(p + vec3{0.f, 2.9f, 0.f}, p + vec3{0.f, C - 2.4f, 0.f}, 0.18f, steel, 6);    // the tank's feed from the pipes
            b.kit(true).box(p + vec3{1.62f, 1.f, 0.f}, {0.05f, 0.5f, 0.8f}, heat * 0.6f);             // its warming coil, lit
        }
        // 4. INVENTORY: two tall stacks of files down the room, three aisles; drawers labelled, a few lit
        for (float sx : {-50.f, -30.f}) {
            b.wall(sx - 1.f, 0.f, cold::INV_BACK + 6.f, sx + 1.f, 6.5f, cold::THAW_BACK - 6.f, steelFrost * 0.35f);
            for (float z = cold::THAW_BACK - 7.f; z > cold::INV_BACK + 6.f; z -= 1.6f)
                for (int row = 0; row < 6; ++row) {
                    float y = 0.6f + row * 1.f;
                    for (int sg = -1; sg <= 1; sg += 2) {
                        b.kit().box({sx + sg * 1.02f, y, z}, {0.05f, 0.82f, 1.4f}, steelFrost * (0.4f + 0.12f * cold::hash(z + row, sx)));
                        if (cold::hash(z * 3.f + row, sx + sg) > 0.86f) b.kit(true).box({sx + sg * 1.06f, y + 0.25f, z}, {0.02f, 0.08f, 0.3f}, amber * 0.8f);
                        else b.kit().box({sx + sg * 1.06f, y + 0.25f, z}, {0.02f, 0.08f, 0.3f}, vec3{0.75f, 0.73f, 0.65f});   // its label
                    }
                }
            b.kit().box({sx, 6.6f, (cold::INV_BACK + cold::THAW_BACK) * 0.5f}, {2.1f, 0.15f, cold::THAW_BACK - cold::INV_BACK - 11.f}, rime);
        }
        for (float x : {-55.5f, -40.f, -24.5f}) b.neon(x - 0.15f, C - 0.4f, cold::INV_BACK + 2.f, x + 0.15f, C - 0.25f, cold::THAW_BACK - 2.f, vec3{0.55f, 0.65f, 0.75f} * 0.4f);
        stencil("INVENTORY", {-47.f, 10.f, cold::THAW_BACK - 0.7f}, {1.f, 0.f, 0.f}, 0.25f, paint);
        // 5. RACK 24: cryo lines from the ceiling to every alcove, a crane over the aisle, a lit trail to 24-A
        for (int k = 0; k < 10; ++k) {
            const vec3 q = cold::rackSlot(k);
            const float bx = k % 2 == 0 ? q.x - 1.3f : q.x + 1.3f;
            b.kit().rod({bx, 8.f, q.z}, {bx, C, q.z}, 0.16f, steelFrost * 0.5f, 6);
            b.kit().rod({bx, C - 0.6f, q.z}, {HX + (k % 2 == 0 ? -6.f : 6.f), C - 0.6f, q.z}, 0.12f, steelFrost * 0.5f, 6);
            b.kit(true).box({bx, 8.1f, q.z}, {0.25f, 0.12f, 0.25f}, vec3{0.5f, 0.7f, 0.9f} * 0.8f);
        }
        for (float x : {HX - 6.f, HX + 6.f}) b.kit().box({x, C - 0.6f, (cold::INV_BACK + ZB) * 0.5f}, {0.5f, 0.5f, cold::INV_BACK - ZB}, steel);   // the lines' mains
        b.kit().box({HX, C - 1.6f, -712.f}, {X1 - X0 - 4.f, 0.8f, 1.2f}, steel);   // a crane's bridge across the room
        b.kit().box({HX + 10.f, C - 2.4f, -712.f}, {2.f, 0.8f, 1.6f}, steel * 1.3f);
        b.kit().rod({HX + 10.f, C - 2.8f, -712.f}, {HX + 10.f, 6.f, -712.f}, 0.04f, steel, 4);
        b.kit().box({HX + 10.f, 5.8f, -712.f}, {0.6f, 0.4f, 0.6f}, steelFrost * 0.6f);
        b.kit().box({HX, 9.5f, cold::INV_BACK - 8.f}, {12.f, 2.2f, 0.25f}, steel);   // a board hung over the aisle
        stencil("COPIES ON FILE: 11", {HX - 5.3f, 10.15f, cold::INV_BACK - 8.15f}, {1.f, 0.f, 0.f}, 0.1f, paint);
        {   // the trail to 24-A: lit studs in the floor, from the door to its alcove
            const vec3 from{HX, 0.f, cold::INV_BACK - 3.f}, to = cold::rackSlot(0) + vec3{2.2f, 0.f, 0.f};
            for (float u = 0.f; u <= 1.f; u += 0.06f) b.kit(true).box(glm::mix(from, to, u) + vec3{0.f, 0.03f, 0.f}, {0.3f, 0.02f, 0.3f}, vec3{0.4f, 0.75f, 1.f} * 0.35f);
        }
    }

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
        {   // its sign, facing you down the bridge (read walking east)
            const float sx = cold::chiller(0).c.x - cold::chiller(0).r - 3.f, sz = (cold::BRIDGE_Z0 + cold::BRIDGE_Z1) * 0.5f;
            for (int sg = -1; sg <= 1; sg += 2) b.kit().box({sx + 0.3f, 3.5f, sz + sg * 5.2f}, {0.3f, 7.f, 0.3f}, steel);
            b.kit().box({sx + 0.25f, 6.1f, sz}, {0.2f, 1.6f, 10.6f}, steel);
            stencil("BLAST CHILLERS", {sx, 6.6f, sz - 4.9f}, {0.f, 0.f, 1.f}, 0.12f, paint);
        }
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
        lump(st + vec3{-20.f, -6.f, -175.f}, 60.f, 34.f, glm::mix(snow, ice, 0.3f), 1501u, 0.f, 0.4f, 3);   // (behind it, north: clear of the duel and the causeway)
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

    // ---- the old dispatch line across the sea: a rail on trestles from the Chillers to DISPATCH 2, its
    // span broken in places, slumped into the ice in others. Follow it and you reach the station. ----
    {
        b.mat = Mat::METAL;
        const vec3 a = cold::seaStart(), e = cold::station() + vec3{-18.f, 0.f, -12.f};
        const float len = glm::length(e - a);
        const vec3 u = (e - a) / len, side{u.z, 0.f, -u.x};   // (side: north of the walk)
        const float yaw = -std::atan2(u.z, u.x);
        const int piers = (int)(len / 24.f);
        for (int k = 0; k <= piers; ++k) {
            const vec3 p = a + u * (k * len / piers) + side * 11.f;
            const bool snapped = cold::hash(k, 401.f) > 0.8f;
            const float lean = snapped ? (cold::hash(k, 402.f) - 0.5f) * 0.6f : 0.f;
            b.wall(p.x - 0.7f, 0.f, p.z - 0.7f, p.x + 0.7f, snapped ? 3.f : 6.f, p.z + 0.7f, steel);
            b.kit().box(p + vec3{0.f, 3.f, 0.f}, {1.2f, 6.f, 1.2f}, steel * 1.3f, yaw, lean);
            b.kit().box(p + vec3{0.f, 0.2f, 0.f}, {2.6f, 0.4f, 2.6f}, rime, yaw);                       // ice frozen up round its foot
            b.kit().box(p + vec3{0.f, 6.1f, 0.f}, {1.4f, 0.12f, 1.4f}, rime);
            if (k == piers) break;
            const vec3 q = a + u * ((k + 1) * len / piers) + side * 11.f;
            const float h = cold::hash(k, 403.f);
            if (h < 0.15f) continue;                                                                    // a span gone
            if (h < 0.3f) {                                                                             // a span fallen, one end in the ice
                b.kit().box((p + q) * 0.5f + vec3{0.f, 3.f, 0.f}, {glm::length(q - p), 0.6f, 0.9f}, steel, yaw, 0.f, 0.22f);
                continue;
            }
            b.kit().box((p + q) * 0.5f + vec3{0.f, 6.3f, 0.f}, {glm::length(q - p), 0.6f, 0.9f}, steel, yaw);
            b.kit().box((p + q) * 0.5f + vec3{0.f, 6.65f, 0.f}, {glm::length(q - p), 0.1f, 1.f}, rime, yaw);
            if (k % 3 == 1) b.kit(true).box((p + q) * 0.5f + vec3{0.f, 5.85f, 0.f}, {0.4f, 0.2f, 0.4f}, amber * 0.8f);   // a lamp under it, still lit
        }
        // Frozen in along the way: a capsule that never left, racks standing up out of the ice, ridges
        auto across = [&](float along, float off) { return a + u * (len * along) + side * off; };
        {
            const vec3 c = across(0.42f, -14.f);
            b.wall(c.x - 2.2f, 0.f, c.z - 2.2f, c.x + 2.2f, 2.6f, c.z + 2.2f, vec3{0.55f, 0.6f, 0.68f});
            b.kit().box(c + vec3{0.f, 1.2f, 0.f}, {4.f, 4.f, 11.f}, vec3{0.55f, 0.6f, 0.68f}, yaw + 0.6f, 0.f, 0.35f);   // the pod, tipped and half sunk
            b.kit().box(c + vec3{0.f, 1.1f, 0.f}, {4.2f, 0.6f, 11.4f}, rime, yaw + 0.6f, 0.f, 0.35f);
            b.kit().box(c + vec3{2.f, 0.1f, 1.f}, {7.f, 0.2f, 5.f}, rime * 0.9f, yaw);
        }
        for (int k = 0; k < 7; ++k) {   // the tops of racks stored upright under the sea, standing out of it
            const vec3 c = across(0.15f + 0.11f * k, (k % 2 ? -1.f : 1.f) * (22.f + 10.f * cold::hash(k, 404.f)));
            const float ry = yaw + (cold::hash(k, 405.f) - 0.5f);
            b.wall(c.x - 1.6f, 0.f, c.z - 1.6f, c.x + 1.6f, 2.f + 3.f * cold::hash(k, 406.f), c.z + 1.6f, steel);
            for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2)
                b.kit().box(c + vec3{sx * 1.4f, 2.f, sz * 0.9f}, {0.15f, 4.f + 6.f * cold::hash(k, 406.f), 0.15f}, steel, ry);
            b.kit().box(c + vec3{0.f, 1.8f, 0.f}, {3.2f, 0.12f, 2.2f}, steel, ry);
            b.kit().box(c + vec3{0.f, 0.1f, 0.f}, {4.f, 0.2f, 3.f}, rime, ry);
        }
        for (int k = 0; k < 10; ++k) {   // pressure ridges: slabs of sea ice shoved up, off the line of the walk
            const vec3 c = across(0.08f + 0.09f * k, (k % 2 ? 1.f : -1.f) * (30.f + 25.f * cold::hash(k, 407.f)));
            for (int g = 0; g < 5; ++g) {
                const vec3 q = c + u * (g * 3.f);
                const float h = 0.8f + 1.8f * cold::hash(k * 7 + g, 408.f);
                b.kit().box(q + vec3{0.f, h * 0.3f, 0.f}, {3.2f, h, 0.9f}, glm::mix(ice, rime, cold::hash(k * 7 + g, 409.f)), yaw, 0.f, (cold::hash(k * 7 + g, 410.f) - 0.5f) * 0.8f);
            }
            b.solid(c.x - 2.f, 0.f, c.z - 2.f, c.x + 2.f, 0.9f, c.z + 2.f);
        }
    }

    // ---- DISPATCH 2, dressed: what was left when the line broke (cover for the duel), and a mast ----
    {
        const vec3 st = cold::station();
        b.mat = Mat::METAL;
        for (const auto& c : {vec3{-10.f, 0.6f, 4.f}, vec3{8.f, 0.6f, 5.f}, vec3{-2.f, 0.6f, -4.f}}) {   // cases on the platform
            b.wall(st.x + c.x - 1.4f, c.y, st.z + c.z - 0.9f, st.x + c.x + 1.4f, c.y + 1.3f, st.z + c.z + 0.9f, vec3{0.45f, 0.3f, 0.18f});
            b.kit().box(st + c + vec3{0.f, 1.36f, 0.f}, {2.9f, 0.1f, 1.9f}, rime);
        }
        for (int k = -1; k <= 1; k += 2) {   // benches under the canopy
            b.wall(st.x + k * 6.f - 2.f, 0.6f, st.z - 7.6f, st.x + k * 6.f + 2.f, 1.1f, st.z - 7.f, steel);
            b.kit().box(st + vec3{k * 6.f, 1.12f, -7.3f}, {4.1f, 0.06f, 0.7f}, rime);
        }
        {   // the capsule off its rail, on its side in the snow beside the platform
            const vec3 c = st + vec3{-6.f, 0.f, -20.f};
            b.wall(c.x - 5.5f, 0.f, c.z - 2.f, c.x + 5.5f, 3.6f, c.z + 2.f, vec3{0.55f, 0.6f, 0.68f});
            b.kit().box(c + vec3{0.f, 1.8f, 0.f}, {11.f, 3.6f, 4.f}, vec3{0.55f, 0.6f, 0.68f});
            b.kit().box(c + vec3{0.f, 3.65f, 0.f}, {10.6f, 0.15f, 3.6f}, rime);
            b.kit().box(c + vec3{5.6f, 1.8f, 0.f}, {0.3f, 2.8f, 2.8f}, vec3{0.1f, 0.12f, 0.15f});   // its hatch, dark
            stencil("DISPATCH", c + vec3{-4.f, 3.f, 2.05f}, {1.f, 0.f, 0.f}, 0.12f, paint * 0.8f);
        }
        b.place("console", st + vec3{12.f, 0.6f, -7.f}, 180.f, 0.6f);
        b.wall(st.x + 11.35f, 0.6f, st.z - 7.4f, st.x + 12.65f, 1.7f, st.z - 6.6f, panel);
        s.terminals.push_back({st + vec3{12.f, 0.6f, -7.f}, "DISPATCH 2", "DISPATCH 2: OUT OF SERVICE. ALL DISPATCH NOW FROM THE VAULT, ON THE RESTORER'S WORD."});
        for (int k = 0; k < 4; ++k) {   // lamp posts round its edge
            const vec3 p = st + vec3{-17.f + (k % 2) * 34.f, 0.6f, k < 2 ? 9.f : -9.5f};
            b.kit().rod(p, p + vec3{0.f, 4.f, 0.f}, 0.1f, steel, 6);
            b.kit(true).box(p + vec3{0.f, 4.f, 0.f}, {0.3f, 0.15f, 0.3f}, amber);
        }
        const vec3 m = cold::stationBeacon();   // its signal mast: seen from the far side of the sea (it blinks: Gameplay_Cold.h)
        b.wall(m.x - 0.6f, 0.f, m.z - 0.6f, m.x + 0.6f, 2.f, m.z + 0.6f, steel);
        b.kit().box(m + vec3{0.f, 15.f, 0.f}, {0.8f, 30.f, 0.8f}, steel);
        for (float y = 3.f; y < 30.f; y += 3.f) b.kit().box(m + vec3{0.f, y, 0.f}, {1.5f, 0.12f, 1.5f}, steel * 1.3f);
    }

    // ---- 9. THE MANIFEST: a causeway from the station, out over the sinkhole, to the Vault ----
    // ---- 10. THE VAULT: a round platform hanging in the sinkhole, the Store's cliff behind it ----
    {
        const vec3 st = cold::station(), vc = cold::vaultC();
        const float VR = cold::VAULT_R;
        // The sinkhole: the sea ice gives way to the Void in a square, its walls ice going down
        const AABB H = cold::sinkhole();
        l.basins.insert(l.basins.begin(), {H, cold::VOID_FLOOR});
        b.mat = Mat::ICE;
        for (int s = 0; s < 4; ++s) {
            bool alongX = s < 2; float at = s == 0 ? H.min.z : s == 1 ? H.max.z : s == 2 ? H.min.x : H.max.x;
            vec3 c = alongX ? vec3{(H.min.x + H.max.x) * 0.5f, -200.f, at} : vec3{at, -200.f, (H.min.z + H.max.z) * 0.5f};
            vec3 sz = alongX ? vec3{H.max.x - H.min.x, 400.f, 3.f} : vec3{3.f, 400.f, H.max.z - H.min.z};
            b.kit().box(c, sz, iceDeep);
            b.kit().box(c + vec3{0.f, 196.f, 0.f}, sz + vec3{0.3f, -392.f, 0.3f}, rime);   // its white lip
        }
        // The causeway: a narrow walk on piers, the station to the Vault's edge
        b.mat = Mat::METAL;
        const vec3 a = st + vec3{18.f, 0.f, -2.f}, e = vc + glm::normalize(st - vc) * (VR - 1.f);
        {
            vec3 d = e - a; d.y = 0.f; float len = glm::length(d); vec3 u = d / len;
            int n = (int)std::ceil(len / 1.6f);
            for (int i = 0; i <= n; ++i) {
                vec3 p = a + u * (len * i / n);
                b.wall(p.x - 1.4f, -0.6f, p.z - 1.4f, p.x + 1.4f, 0.f, p.z + 1.4f, floorSteel);
            }
            b.kit().box((a + e) * 0.5f + vec3{0.f, 0.04f, 0.f}, {2.8f, 0.06f, len}, rime * 0.85f, -std::atan2(u.z, u.x) + 1.5708f);
            for (int i = 0; i < (int)(len / 24.f); ++i) {   // piers down into the dark, a lamp on each
                vec3 p = a + u * (12.f + i * 24.f);
                b.kit().box(p + vec3{0.f, -40.f, 0.f}, {1.2f, 80.f, 1.2f}, steel);
                b.kit(true).box(p + vec3{1.5f, 1.2f, 0.f}, {0.15f, 0.15f, 0.15f}, amber * 0.7f);
            }
        }
        // The Vault: a round platform of the portal's grid, storage drums on shelves round its rim
        b.wall(vc.x - VR * 0.7071f, -0.8f, vc.z - VR * 0.7071f, vc.x + VR * 0.7071f, 0.f, vc.z + VR * 0.7071f, floorSteel);
        b.wall(vc.x - VR + 0.6f, -0.8f, vc.z - VR * 0.42f, vc.x + VR - 0.6f, 0.f, vc.z + VR * 0.42f, floorSteel);
        b.wall(vc.x - VR * 0.42f, -0.8f, vc.z - VR + 0.6f, vc.x + VR * 0.42f, 0.f, vc.z + VR - 0.6f, floorSteel);
        b.wall(vc.x - VR * 0.86f, -0.8f, vc.z - VR * 0.5f, vc.x + VR * 0.86f, 0.f, vc.z + VR * 0.5f, floorSteel);
        b.wall(vc.x - VR * 0.5f, -0.8f, vc.z - VR * 0.86f, vc.x + VR * 0.5f, 0.f, vc.z + VR * 0.86f, floorSteel);
        b.kit().column(vc + vec3{0.f, -0.8f, 0.f}, VR, 0.8f, floorSteel, 40);
        b.kit().column(vc + vec3{0.f, -0.3f, 0.f}, VR + 0.8f, 0.6f, steelFrost * 0.5f, 40).caps = false;
        for (int g = -6; g <= 6; ++g) {   // the grid (the portal's floor)
            float h = std::sqrt(std::max(0.f, VR * VR - (g * 5.f) * (g * 5.f)));
            b.kit(true).box(vc + vec3{g * 5.f, 0.02f, 0.f}, {0.06f, 0.02f, 2.f * h}, vec3{0.25f, 0.35f, 0.45f} * 0.5f);
            b.kit(true).box(vc + vec3{0.f, 0.02f, g * 5.f}, {2.f * h, 0.02f, 0.06f}, vec3{0.25f, 0.35f, 0.45f} * 0.5f);
        }
        // Underneath: it hangs on chains from the Store's cliff, and a stalk falls away into the Void
        b.kit().column(vc + vec3{0.f, -120.f, 0.f}, VR * 0.5f, 119.f, steel * 0.5f, 20, 0.4f);
        for (int k = 0; k < 6; ++k) {
            float ang = 3.1416f + (k - 2.5f) * 0.32f;   // (fanned out toward the cliff, north)
            vec3 rim = vc + vec3{std::cos(ang + 1.5708f) * VR, 0.f, -std::sin(ang + 1.5708f) * VR * 0.2f - VR * 0.8f};
            b.kit().rod(rim + vec3{0.f, 0.5f, 0.f}, vec3{rim.x + (k - 2.5f) * 14.f, 340.f, vc.z - 260.f}, 0.35f, steel, 5);   // (up into the dark, toward the Store)
        }
        // Shelves of storage drums round the rim (phase 3 breaks them open: Gameplay_Restorer.h)
        for (int k = 0; k < cold::DRUMS; ++k) {
            vec3 p = cold::drum(k);
            float ang = std::atan2(p.z - vc.z, p.x - vc.x);
            b.wall(p.x - 1.3f, 0.f, p.z - 1.3f, p.x + 1.3f, 1.4f, p.z + 1.3f, steel);   // the shelf (cover, waist high)
            b.kit().box(p + vec3{0.f, 3.f, 0.f}, {2.6f, 0.2f, 2.6f}, steel, -ang);
            b.kit().box(p + vec3{0.f, 5.6f, 0.f}, {2.6f, 0.2f, 2.6f}, steel, -ang);
            for (int sx = -1; sx <= 1; sx += 2) b.kit().box(p + vec3{sx * 1.2f, 3.f, 0.f}, {0.15f, 6.f, 0.15f}, steel, -ang);
        }
        // Hook rails overhead, for the grapple (and for the tongs to hang you from)
        for (int k = 0; k < 3; ++k) {
            float z = vc.z - 14.f + k * 14.f;
            b.wall(vc.x - VR, 16.f, z - 0.4f, vc.x + VR, 16.8f, z + 0.4f, steel);
            for (int s = -1; s <= 1; s += 2) b.kit().box({vc.x + s * VR, 8.f, z}, {0.8f, 16.f, 0.8f}, steel);
        }
        stencil("VAULT", {vc.x - 4.5f, 22.f, vc.z - VR - 0.2f}, {1.f, 0.f, 0.f}, 0.3f, paint);
        for (int k = 0; k < 24; ++k) {   // posts round its rim, a lamp on every third, and a low chain between (nothing to stop a fall)
            const float ang = k * 0.2618f;
            const vec3 p = vc + vec3{std::cos(ang) * (VR - 0.4f), 0.f, std::sin(ang) * (VR - 0.4f)};
            const vec3 q = vc + vec3{std::cos(ang + 0.2618f) * (VR - 0.4f), 0.f, std::sin(ang + 0.2618f) * (VR - 0.4f)};
            if (std::fabs(std::remainder(ang - std::atan2(cold::station().z - vc.z, cold::station().x - vc.x), 6.2832f)) < 0.2f) continue;   // (the causeway comes in here)
            b.kit().box(p + vec3{0.f, 0.6f, 0.f}, {0.18f, 1.2f, 0.18f}, steel);
            b.kit().box((p + q) * 0.5f + vec3{0.f, 0.9f, 0.f}, {0.06f, 0.06f, glm::length(q - p)}, steel, -ang - 0.1309f + 1.5708f);
            if (k % 3 == 0) b.kit(true).box(p + vec3{0.f, 1.25f, 0.f}, {0.22f, 0.12f, 0.22f}, amber * 0.8f);
        }
        // Its floor for the enemies (ahead of the sinkhole's): the same five boxes its collision is
        for (const AABB& r : {LevelBuilder::aabb(vc.x - VR * 0.7071f, 0.f, vc.z - VR * 0.7071f, vc.x + VR * 0.7071f, 0.f, vc.z + VR * 0.7071f),
                              LevelBuilder::aabb(vc.x - VR + 0.6f, 0.f, vc.z - VR * 0.42f, vc.x + VR - 0.6f, 0.f, vc.z + VR * 0.42f),
                              LevelBuilder::aabb(vc.x - VR * 0.42f, 0.f, vc.z - VR + 0.6f, vc.x + VR * 0.42f, 0.f, vc.z + VR - 0.6f),
                              LevelBuilder::aabb(vc.x - VR * 0.86f, 0.f, vc.z - VR * 0.5f, vc.x + VR * 0.86f, 0.f, vc.z + VR * 0.5f),
                              LevelBuilder::aabb(vc.x - VR * 0.5f, 0.f, vc.z - VR * 0.86f, vc.x + VR * 0.5f, 0.f, vc.z + VR * 0.86f)})
            l.basins.insert(l.basins.begin(), {r, 0.f});
        // The fight's own walls, parked out of the way until it places them: 8 statues, 3 racks
        l.story.coldPool = (int)l.walls.size(); l.story.coldPoolCount = 11;
        for (int k = 0; k < 11; ++k) {
            Wall w{AABB{vc + vec3{-0.1f, -600.f, -0.1f}, vc + vec3{0.1f, -599.f, 0.1f}}, steel, true};
            w.dynamic = true;
            l.walls.push_back(w);
            l.moverWalls.push_back((int)l.walls.size() - 1);
        }
    }
    s.end = LevelBuilder::aabb(cold::vaultC().x - 6.f, 0.f, cold::vaultC().z - cold::VAULT_R, cold::vaultC().x + 6.f, 8.f, cold::vaultC().z - cold::VAULT_R + 8.f);   // the Vault's dispatch rail (north edge)

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
    Arena hook = section("THE HOOK LINE", "GRAPPLE FROM HOOK TO HOOK",
                         LevelBuilder::aabb(HX - 7.f, -1000.f, ZF, HX + 7.f, 60.f, cold::EDGE_Z - 70.f), cold::hookStart(), -90.f);
    hook.zone.max.z = cold::EDGE_Z - 70.f;
    hook.hasRespawn = true; hook.respawn = cold::hookStart();
    hook.hasTrigger = true; hook.trigger = LevelBuilder::aabb(HX - 7.f, -5.f, ZF, HX + 7.f, 20.f, cold::EDGE_Z - 70.f);
    hook.exitDoor = dockDoor; hook.maxAlive = 3;
    hook.airSpawns = {{HX - 30.f, 8.f, cold::stationZ(2)}, {HX + 30.f, 8.f, cold::stationZ(2) - 20.f}, {HX + 25.f, 8.f, cold::stationZ(3)}, {HX - 25.f, 8.f, cold::stationZ(1)}};
    hook.waves = {{W(EnemyType::PICKER, 2)}, {W(EnemyType::PICKER, 2)}};
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
    Arena sea = section("THE FROZEN SEA", "DO NOT LOOK DOWN FOR LONG", LevelBuilder::aabb(cold::SEA_X0, -10.f, -1080.f, cold::station().x - 60.f, 60.f, -760.f), cold::seaStart(), -20.f);
    sea.extraZones.push_back(LevelBuilder::aabb(gateX - 0.4f, -1000.f, cold::BRIDGE_Z0, cold::SEA_X0, 60.f, cold::BRIDGE_Z1));   // the exit catwalk
    sea.hasRespawn = true; sea.respawn = cold::seaStart(); sea.ambient = Ambient::WIND;
    l.arenas.push_back(sea);
    // 8. 24-K: the duel at the broken station
    {
        const vec3 st = cold::station();
        Arena k = section("24-K", "PURPOSE: COMPLY", LevelBuilder::aabb(st.x - 60.f, -10.f, -1080.f, st.x + 70.f, 60.f, -930.f), st + vec3{-40.f, 0.f, 10.f}, 0.f);
        k.hasRespawn = true; k.respawn = st + vec3{-40.f, 0.f, 10.f};
        k.hasTrigger = true; k.trigger = LevelBuilder::aabb(st.x - 30.f, -2.f, st.z - 30.f, st.x + 30.f, 8.f, st.z + 30.f);
        k.waves = {{W(EnemyType::UNIT24K, 0, {st + vec3{14.f, 0.6f, 0.f}})}};
        k.maxAlive = 1; k.ambient = Ambient::WIND;
        l.arenas.push_back(k);
    }
    // 9. the Manifest: the causeway (the interface is the Restorer's from here on)
    {
        const vec3 st = cold::station(), vc = cold::vaultC();
        const vec3 causewayEnd = vc + glm::normalize(st - vc) * (cold::VAULT_R - 1.f);
        Arena m = section("THE MANIFEST", "RESTORE POINT SAVED", LevelBuilder::aabb(st.x + 16.f, -1000.f, causewayEnd.z - 4.f, vc.x - cold::VAULT_R * 0.5f, 60.f, st.z + 12.f), st + vec3{20.f, 0.f, -2.f}, 0.f);
        m.zone.max.z = -930.f;   // (it starts where 24-K's yard ends)
        m.hasRespawn = true; m.respawn = m.playerStart; m.ambient = Ambient::WIND;
        l.arenas.push_back(m);
        // 10. the Vault: THE RESTORER
        Arena v = section("THE VAULT", "THE RESTORER", LevelBuilder::aabb(vc.x - cold::VAULT_R, -1000.f, vc.z - cold::VAULT_R, vc.x + cold::VAULT_R, 40.f, vc.z + cold::VAULT_R - 2.f),
                          vc + glm::normalize(st - vc) * (cold::VAULT_R - 3.f), 0.f);
        v.hasRespawn = true; v.respawn = v.playerStart;
        v.hasTrigger = true; v.trigger = LevelBuilder::aabb(vc.x - 20.f, -2.f, vc.z - 20.f, vc.x + 20.f, 10.f, vc.z + 20.f);
        v.waves = {{W(EnemyType::RESTORER, 0, {vc + vec3{0.f, 0.f, -12.f}})}};
        v.maxAlive = 12; v.space = ReverbSpace::OPEN; v.tone = RoomTone::NONE;
        l.arenas.push_back(v);
        l.story.bossSection = 9;
    }
    return l;
}
