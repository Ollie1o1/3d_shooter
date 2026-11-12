#pragma once
// =============================================================================
// LevelRing.h — Chapter 6: THE KEPT RING (docs/chapter-6-design.md).
//
// You stand in the plane of a ring. The floor is ice and sand-coloured dust out
// to a hair-thin horizon; the gas giant sits on that line to the east, half of
// it below the ring plane (skybox.frag's ringSky); due north (-Z), on the line,
// is the Shepherd. Its sweeps travel south (+Z): the lee of anything is its
// south side, and every mooring post stands there.
//
// The first three places of the crossing (section 3), laid out north in one
// world so you can look back at where you came from:
//
//       +Z (south)
//   ┌───────────────┐  0 THE LANDING   z 135..-150   your capsule in a field of
//   │ capsule field │                  hundreds, a ridge of them to jump (low
//   └───────┬───────┘                  gravity), the first sweep. The floor ends.
//   ~ ~ ~ ~ ~ ~ ~ ~ ~  1 THE DRIFT     z -150..-345  a band of loose ring where
//   ~  lanes of ice ~                  chunks slide past each other in lanes,
//   ~ ~ ~[ I2 ]~ ~ ~                   like slow traffic. Hop lane to lane. On
//   ~ ~ ~ ~ ~ ~ ~ ~ ~  2 (the Drift)    the island I2: HOLDFAST. Then the fight
//   ~ ~ ~[ I3 ]~ ~ ~   z -345..-575    island I3, and a grapple to the far shore.
//   ┌───────┴───────┐  3 THE KEPT      z -575..-760  a reef of fused capsules
//   │  the reef     │                  round a courtyard: the chapter's first
//   └───────┬───────┘                  real fight (STRAYS, HERDERS, a TALLY, a PORTER)
//   │ the open hull │  4 (the Kept)    z -760..-905  one capsule big enough to
//   └───────────────┘                  walk into, a sibling inside counting. Out.
//       -Z (north, the Shepherd)
//
// `--ringtest` builds the flat test ground instead (build step 2: every tier of
// sweep on a loop). ring::shelters() / buildRing() are the chapter; the test
// ground is ring::testShelters() / buildRingTest().
//
// Every random draw is its own statement (argument evaluation order differs
// between compilers; see the lab dressing notes).
// =============================================================================
#include "world/Level.h"
#include "world/RingSweep.h"
#include "render/PropFile.h"
#include <map>
#include <cstdio>

namespace ring {
constexpr float HALF_X = 110.f;                 // the playable width (the zones stop you here)
constexpr float EDGE = 116.f, EDGE_RAG = 12.f;   // the ring band's sides: ragged between 116 and 128 m out; past them, space
constexpr float LAND_S = 135.f, LAND_N = -150.f;   // THE LANDING
constexpr float DRIFT_N = -560.f;               // the Drift's far shore (the reef plain starts here)
constexpr float DRIFT_MID = -345.f;             // its two halves meet at I2
constexpr float KEPT_N = -905.f;
// THE SHADOW BAND (level 4): the giant's shadow across the ring. Pitch black from SHADE_IN m in;
// the sun comes back (the sunrise) as you near its far side
constexpr float BAND_S = KEPT_N, BAND_N = -1240.f, SHADE_IN = 30.f;
constexpr float SUNRISE_AT = BAND_N + 95.f;     // cross this and the sun clears the giant's edge
// THE SPOKES (level 5): streaks of charged dust lying across the ring. Inside one, no dash and
// no grapple (the ANCHOR's field, at the scale of the landscape)
constexpr float SPOKES_S = BAND_N - 32.f, SPOKES_N = -1760.f;
// THE GAP (level 6): the ring stops at a cliff (GAP_S), nothing for 480 m, then goes on (GAP_N). You
// cross on a raft of ring ice; THE DROVER herds its chunks alongside. FAR_N: the far side's end
constexpr float GAP_S = -1860.f, GAP_N = -2340.f, FAR_N = -2480.f;
constexpr float RAFT_SPEED = 8.5f;              // m/s across the gap (about a minute)
// THE WAKE (level 7): the Shepherd's gravity has pulled the ring up into a frozen wave. Climb it in
// terraces (each a jump up); the summit is the first time you stand above the ring plane
constexpr float WAKE_S = FAR_N - 20.f, CLIMB_S = -2540.f, WAKE_HALF = 52.f;   // (the climb is narrower than the band)
constexpr float STEP_UP = 2.6f;                 // a jump in this gravity clears ~3.4 m
constexpr float FAR_GROUND = 3000.f, FAR_PLANE = 5200.f;
constexpr float VOID_FLOOR = -900.f;            // under the Drift: nothing
constexpr float VOID_Y = -45.f;                 // fall past this in the Drift: back to the checkpoint
constexpr float GRAVITY_SCALE = 0.5f;           // the crossing's low gravity (section 1)
// The test ground (build step 2), kept for tuning
constexpr float TEST_HALF_X = 70.f, TEST_NORTH = -110.f, TEST_SOUTH = 90.f;

inline glm::vec3 start() { return {0.f, 0.f, 112.f}; }
inline glm::vec3 testStart() { return {0.f, 0.f, 78.f}; }
inline glm::vec3 sweepDir() { return {0.f, 0.f, 1.f}; }        // away from the Shepherd
inline glm::vec3 shepherdDir() { return glm::normalize(glm::vec3{0.f, 0.004f, -1.f}); }   // on the line, due north
inline glm::vec3 sunDir() { return glm::normalize(glm::vec3{-0.8f, 0.16f, -0.35f}); }     // low in the west: long hard shadows
inline glm::vec3 giantDir() { return glm::normalize(glm::vec3{0.99f, -0.02f, 0.14f}); }   // (skybox.frag's RING_GIANT)
inline AABB testGround() { return {{-TEST_HALF_X, 0.f, TEST_NORTH}, {TEST_HALF_X, 0.f, TEST_SOUTH}}; }
inline float hash(float n, float k) { float v = std::sin(n * 12.9898f + k * 78.233f) * 43758.5453f; return v - std::floor(v); }
// How far out the band's side runs at z (side: -1 west, +1 east): ragged, in 20 m steps
inline float edgeAt(float z, int side) { return EDGE + EDGE_RAG * hash(std::floor(z / 20.f) + (side > 0 ? 500.f : 0.f), 61.f); }
inline bool inDrift(float z) { return z < LAND_N && z > DRIFT_N; }

// The distance card for each place (section 13.2: falling every card)
inline const char* distance(int arena) {
    switch (arena) {
        case 0: return "74,000 KM";
        case 1: case 2: return "61,000 KM";
        case 5: return "46,000 KM";
        case 6: return "39,000 KM";
        case 7: return "31,000 KM";
        case 8: return "24,000 KM";
        default: return "52,000 KM";
    }
}
// How big the Shepherd is in the sky (angular radius) as you go: a point at the Landing, growing
inline float shepherdSize(float z) {
    const float t = glm::clamp((LAND_S - z) / (LAND_S - KEPT_N), 0.f, 1.35f);   // (and on, through the Shadow Band)
    return 0.0012f + 0.0075f * t * t;
}

// The Drift's two islands and the fight island, and the reef (the code that places enemies and the
// HOLDFAST capsule reads these)
inline AABB islandI2() { return {{-22.f, -12.f, -349.f}, {22.f, 1.f, -322.f}}; }
inline AABB islandI3() { return {{-34.f, -14.f, -480.f}, {34.f, 0.f, -420.f}}; }
inline glm::vec3 holdfastCapsule() { return {-12.f, 1.f, -336.f}; }
inline glm::vec3 reefC() { return {0.f, 0.f, -700.f}; }
constexpr float REEF_HX = 36.f, REEF_HZ = 42.f, REEF_WALL = 4.f, REEF_H = 6.5f;
inline AABB quietHull() { return {{-8.f, 0.f, -808.f}, {8.f, 9.f, -780.f}}; }   // the open capsule (no sweeps in there)

// Hard light, black sky, the giant's butter and tan in the shadows
inline Theme theme() {
    Theme t{};
    t.zenith = {0.003f, 0.003f, 0.006f};
    t.horizon = {0.05f, 0.045f, 0.04f};
    t.ground = {0.003f, 0.003f, 0.005f};
    t.sunDir = sunDir();
    t.sunColor = {2.6f, 2.5f, 2.3f};
    t.sunSize = 0.008f;
    t.sunStripes = 0.f;
    t.mountain = {0.1f, 0.09f, 0.08f};
    t.stars = 1.f;
    t.lightDir = -sunDir();
    t.lightColor = {1.35f, 1.28f, 1.15f};
    t.skyAmb = {0.42f, 0.35f, 0.24f};             // the giant's butter glow fills the shadow side
    t.groundAmb = {0.4f, 0.37f, 0.31f};
    t.fogColor = {0.02f, 0.02f, 0.02f};
    t.fogDensity = 0.00002f;                      // vacuum: nothing softens the distance
    return t;
}

// The Shadow Band: no sun. The giant's night side gives a breath of light, nothing else does
// (your lamp, the capsules' lamps and the lightning are lights of their own: Gameplay_Shadow.h)
inline Theme darkTheme() {
    Theme t = theme();
    t.zenith = {0.f, 0.f, 0.f}; t.horizon = {0.004f, 0.004f, 0.005f}; t.ground = {0.f, 0.f, 0.f};
    t.sunColor = {0.f, 0.f, 0.f};
    t.lightColor = {0.012f, 0.012f, 0.016f};
    t.skyAmb = {0.018f, 0.017f, 0.02f};
    t.groundAmb = {0.01f, 0.009f, 0.008f};
    t.fogColor = {0.f, 0.f, 0.f};
    return t;
}
// Its shelters: a staggered field of rock, close enough for the mid tier's 10 s warning (design
// section 4: the double sweep starts here), and a route of lit capsules through it
struct Rock { float x, z, w, h, d; };
inline std::vector<Rock> bandRocks() {
    std::vector<Rock> out;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 5; ++c) {
            const float k = (float)(r * 5 + c);
            const float x = -88.f + c * 44.f + (r % 2) * 22.f + 6.f * (hash(k, 91.f) - 0.5f);
            if (x > 100.f) continue;
            const float z = BAND_S - 44.f - r * 36.5f + 6.f * (hash(k, 92.f) - 0.5f);
            out.push_back({x, z, 9.f + 3.f * hash(k, 93.f), 4.2f + 0.8f * hash(k, 94.f), 5.f + hash(k, 95.f)});
        }
    return out;
}
// The spokes: each a streak across the band (radial to the giant, so across our way north),
// slanted, wide at one end and narrowing to a crossing somewhere along it
struct Spoke { float z, slope, narrowX, wMin, wMax; };
inline std::vector<Spoke> spokes() {
    return {{-1330.f, 0.10f, -55.f, 12.f, 58.f}, {-1418.f, -0.08f, 50.f, 11.f, 64.f}, {-1508.f, 0.13f, -15.f, 13.f, 52.f},
            {-1598.f, -0.11f, 70.f, 12.f, 66.f}, {-1688.f, 0.07f, -60.f, 11.f, 60.f}};
}
inline float spokeHalf(const Spoke& s, float x) { return 0.5f * (s.wMin + (s.wMax - s.wMin) * std::min(1.f, std::fabs(x - s.narrowX) / 110.f)); }
inline float spokeZ(const Spoke& s, float x) { return s.z + s.slope * x; }
// Is a point (feet) inside a spoke's charged dust? (It reaches 4 m up: jump high over a narrow one and you're free)
inline int spokeAt(glm::vec3 p) {
    if (p.z > SPOKES_S || p.z < SPOKES_N || p.y > 4.f || p.y < -2.f) return -1;
    const auto S = spokes();
    for (int i = 0; i < (int)S.size(); ++i) if (std::fabs(p.z - spokeZ(S[i], p.x)) < spokeHalf(S[i], p.x)) return i;
    return -1;
}
// Its shelters: rows of rock between and inside the spokes (you walk to them: no grapple in there),
// none on a spoke's narrow crossing, which stays clear
inline std::vector<Rock> spokeRocks() {
    std::vector<Rock> out;
    const auto S = spokes();
    for (int r = 0; r < 13; ++r)
        for (int c = 0; c < 5; ++c) {
            const float k = (float)(r * 5 + c) + 300.f;
            const float x = -88.f + c * 44.f + (r % 2) * 22.f + 6.f * (hash(k, 91.f) - 0.5f);
            if (x > 100.f) continue;
            const float z = SPOKES_S - 20.f - r * 36.f + 6.f * (hash(k, 92.f) - 0.5f);
            bool onCrossing = false;
            for (const Spoke& s : S) if (std::fabs(x - s.narrowX) < 14.f && std::fabs(z - spokeZ(s, x)) < spokeHalf(s, x) + 8.f) onCrossing = true;
            if (onCrossing) continue;
            out.push_back({x, z, 9.f + 3.f * hash(k, 93.f), 4.2f + 0.8f * hash(k, 94.f), 5.f + hash(k, 95.f)});
        }
    return out;
}
// The Gap's two sides: rows of rock on the cliff tops (the far side has the posts you'll want after the ride)
inline std::vector<Rock> gapRocks() {
    std::vector<Rock> out;
    const float rows[] = {SPOKES_N - 40.f, GAP_S + 34.f, GAP_N - 16.f, GAP_N - 52.f, GAP_N - 88.f, GAP_N - 124.f};
    for (int r = 0; r < 6; ++r)
        for (int c = 0; c < 5; ++c) {
            const float k = (float)(r * 5 + c) + 600.f;
            const float x = -88.f + c * 44.f + (r % 2) * 22.f + 6.f * (hash(k, 91.f) - 0.5f);
            if (x > 100.f || (r == 1 && std::fabs(x) < 16.f)) continue;   // (the way to the raft stays open)
            out.push_back({x, rows[r] + 5.f * (hash(k, 92.f) - 0.5f), 9.f + 3.f * hash(k, 93.f), 4.2f + 0.8f * hash(k, 94.f), 5.f + hash(k, 95.f)});
        }
    return out;
}
// The raft: a slab of ring ice moored at the near lip, its boulder on its north end
inline glm::vec3 raftHalf() { return {7.f, 1.6f, 5.5f}; }
inline glm::vec3 raftStart() { return {0.f, -1.6f, GAP_S - 5.6f}; }       // (its top flush with the lip)
inline float raftRun() { return (GAP_S - GAP_N) - 11.4f; }                // to the far lip
// The wave's terraces, south to north: each tread's south edge, north edge and height. Most are under
// the sweep's 14 m shadow deep (tucked against the next step you're sheltered); a few are deeper
struct Tread { float zS, zN, y; };
inline std::vector<Tread> wakeTreads() {
    std::vector<Tread> out;
    const float depth[] = {12.f, 10.f, 18.f, 11.f, 12.f, 9.f, 19.f, 12.f, 10.f, 13.f, 18.f, 11.f, 12.f, 10.f, 17.f, 12.f};
    float z = CLIMB_S;
    for (int i = 0; i < 16; ++i) { out.push_back({z, z - depth[i], STEP_UP * (i + 1)}); z -= depth[i]; }
    return out;
}
inline float summitY() { return wakeTreads().back().y; }
inline float summitS() { return wakeTreads().back().zN; }
inline float summitN() { return summitS() - 70.f; }
// The lamp route: where the lit capsules lie, south to north
inline std::vector<glm::vec3> bandLamps() {
    std::vector<glm::vec3> out;
    for (int i = 0; i < 13; ++i) {
        const float z = BAND_S - 20.f - i * 24.f;
        out.push_back({18.f * std::sin(i * 0.7f) + 10.f * (hash((float)i, 96.f) - 0.5f), 0.f, z});
    }
    return out;
}

// ---- the drifting lanes ----------------------------------------------------------------
// A lane: chunks sliding along X together, back and forth, each lane at its own pace and
// height. `boulder`: each chunk carries a heavy block on its north end (a shelter that moves).
struct Lane { float z, depth, top, period, phase, sweep; bool boulder; };
inline std::vector<Lane> lanes() {
    std::vector<Lane> L;
    // First half: pier (-168) to I2 (-322). Gaps of 4.5 to 5.5 m: a running jump in low gravity clears 9
    const float zA[] = {-177.f, -191.f, -205.5f, -220.f, -234.5f, -249.f, -263.5f, -278.f, -292.5f, -307.f, -317.f};
    for (int i = 0; i < 11; ++i) {
        const float k = (float)i;
        L.push_back({zA[i], 10.f, (i % 3 == 0) ? 0.f : (i % 3 == 1) ? 1.4f : -0.6f, 20.f + 9.f * hash(k, 31.f), hash(k, 32.f), 15.f + 4.f * hash(k, 33.f), i % 3 == 1});
    }
    // Second half: I2 (-349) to I3 (-418), quicker; then I3 (-480) to the shore (-560), the last gap a grapple
    const float zB[] = {-358.f, -372.f, -386.5f, -401.f, -415.f};
    for (int i = 0; i < 5; ++i) {
        const float k = (float)(i + 20);
        L.push_back({zB[i], 10.f, i == 4 ? 0.f : i % 2 ? 1.8f : 0.4f, 13.f + 5.f * hash(k, 31.f), hash(k, 32.f), 17.f, i == 2});
    }
    const float zC[] = {-489.f, -503.f, -517.5f, -532.f, -546.f};
    for (int i = 0; i < 5; ++i) {
        const float k = (float)(i + 40);
        L.push_back({zC[i], 10.f, i == 3 ? 1.6f : (i % 2 ? 1.f : -0.4f), 15.f + 6.f * hash(k, 31.f), hash(k, 32.f), 18.f, i == 1});
    }
    return L;
}
constexpr int LANE_CHUNKS = 3;          // per lane, spaced along X
constexpr float LANE_SPACING = 34.f;    // ...this far apart: one is always within reach of the middle

// ---- shelter ---------------------------------------------------------------------------
// One shelter: a box from the ground to h, its post on the lee (south) side
inline SweepShelter shelterBox(float x0, float z0, float x1, float z1, float y0, float y1, int section, bool small = false) {
    SweepShelter s;
    s.box = {{std::min(x0, x1), y0, std::min(z0, z1)}, {std::max(x0, x1), y1, std::max(z0, z1)}};
    s.post = {(x0 + x1) * 0.5f, y0, std::max(z0, z1) + 2.2f};
    s.small = small; s.section = section;
    return s;
}

// The fixed shelters of the chapter (the chunks and the reef). The drifting boulders and the
// loose ones a HERDER shoves are added by buildRing (they need their walls and movers).
inline std::vector<Rock> landingRocks() {
    return {{-6.f, 72.f, 13.f, 7.f, 6.f}, {32.f, 56.f, 11.f, 6.5f, 5.f}, {-40.f, 44.f, 12.f, 7.f, 6.f},
            {10.f, 2.f, 12.f, 7.5f, 6.f}, {-30.f, -18.f, 11.f, 6.f, 6.f}, {44.f, -30.f, 13.f, 7.f, 5.f},
            {-6.f, -62.f, 14.f, 8.f, 6.f}, {30.f, -92.f, 11.f, 6.f, 5.f}, {-44.f, -98.f, 12.f, 7.f, 6.f},
            {6.f, -128.f, 12.f, 7.f, 5.f}, {70.f, 30.f, 12.f, 7.f, 6.f}, {-74.f, 10.f, 12.f, 7.f, 6.f},
            {72.f, -76.f, 12.f, 7.f, 6.f}, {-78.f, -60.f, 12.f, 7.f, 6.f}, {80.f, 100.f, 12.f, 7.f, 6.f}, {-70.f, 100.f, 12.f, 7.f, 6.f}};
}
inline std::vector<Rock> keptRocks() {   // the plain before the reef and round it
    return {{-14.f, -582.f, 12.f, 7.f, 5.f}, {22.f, -598.f, 11.f, 6.f, 5.f}, {-46.f, -612.f, 12.f, 7.f, 6.f},
            {50.f, -620.f, 12.f, 7.f, 6.f}, {-70.f, -680.f, 10.f, 7.f, 6.f}, {70.f, -700.f, 10.f, 7.f, 6.f},
            {-60.f, -760.f, 12.f, 7.f, 6.f}, {60.f, -770.f, 12.f, 7.f, 6.f}, {-24.f, -840.f, 12.f, 7.f, 5.f},
            {26.f, -856.f, 12.f, 7.f, 5.f}, {0.f, -880.f, 10.f, 6.f, 4.f}, {-88.f, -630.f, 12.f, 7.f, 6.f},
            {88.f, -650.f, 12.f, 7.f, 6.f}, {-90.f, -820.f, 12.f, 7.f, 6.f}, {90.f, -830.f, 12.f, 7.f, 6.f}};
}
// The courtyard's capsule stacks: (x, z, half-width, half-depth, height)
struct Stack { float x, z, hx, hz, h; };
inline std::vector<Stack> reefStacks() {
    const glm::vec3 c = reefC();
    return {{c.x - 18.f, c.z + 22.f, 3.f, 2.f, 5.f}, {c.x + 16.f, c.z + 18.f, 3.f, 2.f, 5.f},
            {c.x - 6.f, c.z + 2.f, 4.f, 2.5f, 5.5f}, {c.x + 22.f, c.z - 8.f, 3.f, 2.f, 5.f},
            {c.x - 22.f, c.z - 14.f, 3.f, 2.f, 5.f}, {c.x + 4.f, c.z - 26.f, 3.5f, 2.f, 5.f}};
}

inline std::vector<SweepShelter> shelters() {
    std::vector<SweepShelter> out;
    for (const Rock& r : landingRocks()) out.push_back(shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 0));
    out.push_back(shelterBox(-60.f, 26.f, 60.f, 30.f, 0.f, 2.9f, 0));   // the capsule ridge
    // The Drift's islands: a block on each
    const AABB i2 = islandI2(), i3 = islandI3();
    out.push_back(shelterBox(-4.f, i2.min.z + 0.5f, 12.f, i2.min.z + 5.f, i2.max.y, i2.max.y + 7.f, 1));
    out.push_back(shelterBox(-28.f, i3.min.z + 2.f, -14.f, i3.min.z + 7.f, 0.f, 7.f, 2));
    out.push_back(shelterBox(10.f, i3.min.z + 22.f, 24.f, i3.min.z + 27.f, 0.f, 7.f, 2));
    out.push_back(shelterBox(-12.f, i3.max.z - 16.f, 2.f, i3.max.z - 11.f, 0.f, 6.f, 2));
    out.push_back(shelterBox(-8.f, -168.f, 8.f, -163.f, 0.f, 5.f, 1));   // the pier's end
    for (const Rock& r : keptRocks()) out.push_back(shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, r.z < -760.f ? 4 : 3));
    // The reef's walls (the north wall shelters the courtyard) and its stacks
    const glm::vec3 c = reefC();
    out.push_back(shelterBox(c.x - REEF_HX, c.z - REEF_HZ - REEF_WALL, c.x - 6.f, c.z - REEF_HZ, 0.f, REEF_H, 3));
    out.push_back(shelterBox(c.x + 6.f, c.z - REEF_HZ - REEF_WALL, c.x + REEF_HX, c.z - REEF_HZ, 0.f, REEF_H, 3));
    out.push_back(shelterBox(c.x - REEF_HX, c.z + REEF_HZ, c.x - 6.f, c.z + REEF_HZ + REEF_WALL, 0.f, REEF_H, 3));
    out.push_back(shelterBox(c.x + 6.f, c.z + REEF_HZ, c.x + REEF_HX, c.z + REEF_HZ + REEF_WALL, 0.f, REEF_H, 3));
    for (const Stack& s : reefStacks()) out.push_back(shelterBox(s.x - s.hx, s.z - s.hz, s.x + s.hx, s.z + s.hz, 0.f, s.h, 3));
    out.push_back(shelterBox(quietHull().min.x - 1.f, quietHull().min.z - 1.f, quietHull().max.x + 1.f, quietHull().min.z, 0.f, 9.f, 4));   // (its nose wall)
    for (const Rock& r : bandRocks()) out.push_back(shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 5));
    for (const Rock& r : spokeRocks()) out.push_back(shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 6));
    for (const Rock& r : gapRocks()) out.push_back(shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 7));
    {   // The Wake's terraces: each tread's mass shelters the tread below it (its post at the riser's foot)
        const auto T = wakeTreads();
        for (size_t i = 0; i < T.size(); ++i) {
            const float zN = i + 1 < T.size() ? T[i].zN : summitN();
            SweepShelter s = shelterBox(-WAKE_HALF - 8.f, zN, WAKE_HALF + 8.f, T[i].zS, 0.f, T[i].y, 8);
            s.post = {WAKE_HALF - 4.f, i ? T[i - 1].y : 0.f, T[i].zS + 1.6f};
            out.push_back(s);
        }
    }
    return out;
}

// The test ground's (build step 2)
inline std::vector<SweepShelter> testShelters() {
    std::vector<SweepShelter> out;
    auto add = [&](float x, float z, float w, float h, float d, bool small) {
        out.push_back(shelterBox(x - w * 0.5f, z - d * 0.5f, x + w * 0.5f, z + d * 0.5f, 0.f, h, 0, small));
    };
    static const float BIG[][2] = {{-8.f, 62.f}, {34.f, 50.f}, {-42.f, 44.f}, {4.f, 20.f}, {46.f, 8.f}, {-36.f, 0.f},
                                   {16.f, -24.f}, {-14.f, -48.f}, {44.f, -44.f}, {-50.f, -52.f}, {8.f, -84.f}, {-30.f, -90.f}, {48.f, -86.f}};
    int n = 0;
    for (auto& b : BIG) { add(b[0], b[1], 9.f + 4.f * hash((float)n, 1.f), 5.f + 3.f * hash((float)n, 2.f), 4.f + 2.f * hash((float)n, 3.f), false); ++n; }
    static const float SMALL[][2] = {{18.f, 66.f}, {-24.f, 28.f}, {26.f, 30.f}, {-10.f, -8.f}, {30.f, -10.f}, {-56.f, 22.f}, {-32.f, -30.f}, {26.f, -64.f}, {-4.f, -70.f}};
    for (auto& s : SMALL) { add(s[0], s[1], 3.6f, 2.6f, 2.4f, true); ++n; }
    return out;
}

// The Shepherd's count (design 13.2). The manifest in the hull says 40,111 were dispatched; you
// are the one extra. Each DISPATCHED you remove makes it short instead.
inline constexpr int DISPATCHED = 40111;
inline void countLine(char* out, size_t n, int count, int shortBy) {
    static const char* words[] = {"", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE", "TEN"};
    char num[24]; snprintf(num, sizeof(num), "%d,%03d", count / 1000, count % 1000);
    if (shortBy <= 0) { snprintf(out, n, "COUNT: %s. ONE EXTRA.", num); return; }
    if (shortBy <= 10) snprintf(out, n, "COUNT: %s. %s SHORT.", num, words[shortBy]);
    else snprintf(out, n, "COUNT: %s. %d SHORT.", num, shortBy);
}
}   // namespace ring

// THE SHADOW BAND's numbers (Gameplay_Shadow.h)
namespace shadowband {
constexpr float DIAMOND = 1.6f;     // the sunrise: the flash, this long after the arc starts
constexpr float FRONT_FROM = 320.f; // the light starts this far north of you...
constexpr float FRONT_SPEED = 60.f; // ...and comes at a sweep's speed
constexpr float LIFT = 1.2f;        // once it has reached you, the dark lifts over this long
// The lightning's flicker: three strokes over half a second, each fading fast
inline float bolt(float t) {
    if (t < 0.f || t > 0.7f) return 0.f;
    const float at[3] = {0.f, 0.11f, 0.32f}, amp[3] = {1.f, 0.55f, 0.8f};
    float b = 0.f;
    for (int i = 0; i < 3; ++i) if (t >= at[i]) b = std::max(b, amp[i] * std::exp(-(t - at[i]) * 26.f));
    return b;
}
}

// THE GAP's numbers (Gameplay_Gap.h)
namespace gapride {
constexpr float CHARGE_SPEED = 24.f;     // the chunk across the raft
constexpr float CHARGE_DAMAGE = 12.f;
const glm::vec3 CHUNK_HALF{2.2f, 1.05f, 1.8f};   // low enough to jump in this gravity (a jump clears ~3.4 m)
}

// THE WAKE's numbers: THE BELLWETHER's ring (Gameplay_Wake.h)
namespace wakebell {
constexpr float SPEED = 16.f, REACH = 34.f, DAMAGE = 10.f, BAND = 1.1f, JUMP_CLEAR = 1.0f;
}

// ---- collision that matches what is drawn ---------------------------------------------
// Points over a mesh's surface (its triangles, every half metre or so)
template <class Tri> inline std::vector<glm::vec3> ringSamples(Tri forEach) {
    std::vector<glm::vec3> pts;
    forEach([&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        const float e = std::max({glm::length(b - a), glm::length(c - b), glm::length(a - c)});
        const int n = std::clamp((int)std::ceil(e / 0.5f), 1, 24);
        for (int i = 0; i <= n; ++i)
            for (int j = 0; j <= n - i; ++j) {
                const float u = (float)i / n, v = (float)j / n;
                pts.push_back(a + (b - a) * u + (c - a) * v);
            }
    });
    return pts;
}
inline std::vector<glm::vec3> ringSamples(const Shape& s) {
    return ringSamples([&](auto f) { s.forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3) { f(a, b, c); }); });
}
inline std::vector<glm::vec3> ringSamples(const PlacedModel& pm) {
    const auto* prop = propfile::get(pm.name);
    if (!prop) return {};
    const glm::mat4 P = propfile::placement(*prop, pm.free, pm.xf, pm.fit, pm.yaw);
    return ringSamples([&](auto f) { propfile::forEachTri(*prop, P, [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, auto&&...) { f(a, b, c); }); });
}
// Hidden solid boxes round a mesh: its part above `floorY`, cut into `nAlong` pieces along its longer
// flat axis and `nUp` up its height, each piece the box round its own surface. A leaning capsule
// becomes a short staircase that follows it, not one box with empty corners. Returns the walls
inline std::vector<int> ringSolids(LevelBuilder& b, const std::vector<glm::vec3>& all, float floorY = -0.4f, int nAlong = 3, int nUp = 2) {
    std::vector<glm::vec3> pts;
    for (const glm::vec3& p : all) if (p.y > floorY) pts.push_back(p);
    std::vector<int> walls;
    if (pts.empty()) return walls;
    glm::vec3 lo(1e30f), hi(-1e30f);
    for (const glm::vec3& p : pts) { lo = glm::min(lo, p); hi = glm::max(hi, p); }
    const int ax = (hi.x - lo.x) >= (hi.z - lo.z) ? 0 : 2;
    const float aLo = lo[ax], aLen = std::max(1e-3f, hi[ax] - aLo), yLen = std::max(1e-3f, hi.y - lo.y);
    nAlong = std::max(1, std::min(nAlong, (int)std::ceil(aLen / 1.f)));
    nUp = std::max(1, std::min(nUp, (int)std::ceil(yLen / 0.8f)));
    std::vector<glm::vec3> clo(nAlong * nUp, glm::vec3(1e30f)), chi(nAlong * nUp, glm::vec3(-1e30f));
    for (const glm::vec3& p : pts) {
        const int i = std::clamp((int)((p[ax] - aLo) / aLen * nAlong), 0, nAlong - 1);
        const int j = std::clamp((int)((p.y - lo.y) / yLen * nUp), 0, nUp - 1);
        clo[i * nUp + j] = glm::min(clo[i * nUp + j], p); chi[i * nUp + j] = glm::max(chi[i * nUp + j], p);
    }
    for (int k = 0; k < nAlong * nUp; ++k) {
        if (chi[k].x < clo[k].x) continue;
        // Each piece reaches its slice's edges, so neighbours meet with no seam to snag on or slip through
        const int i = k / nUp, j = k % nUp;
        clo[k][ax] = std::min(clo[k][ax], aLo + aLen * i / nAlong); chi[k][ax] = std::max(chi[k][ax], aLo + aLen * (i + 1) / nAlong);
        clo[k].y = std::min(clo[k].y, lo.y + yLen * j / nUp); chi[k].y = std::max(chi[k].y, lo.y + yLen * (j + 1) / nUp);
        walls.push_back(b.solid(clo[k].x, clo[k].y, clo[k].z, chi[k].x, chi[k].y, chi[k].z));
    }
    return walls;
}
// The level's last pass: anything drawn that you could walk into (a body-height part within reach of
// the playable band with no collision near it) gets collision made from its own mesh
inline void ringMatchCollision(LevelBuilder& b, float reachX) {
    LevelData& l = b.L;
    std::map<std::pair<int, int>, std::vector<int>> grid;   // static walls by 4 m cell
    auto cell = [](float v) { return (int)std::floor(v / 4.f); };
    auto file = [&](int w) {
        const AABB& x = l.walls[w].box;
        for (int i = cell(x.min.x); i <= cell(x.max.x); ++i) for (int k = cell(x.min.z); k <= cell(x.max.z); ++k) grid[{i, k}].push_back(w);
    };
    for (int w = 0; w < (int)l.walls.size(); ++w) if (!l.walls[w].dynamic) file(w);
    auto covered = [&](glm::vec3 p) {
        auto it = grid.find({cell(p.x), cell(p.z)});
        if (it == grid.end()) return false;
        for (int w : it->second) {
            const AABB& x = l.walls[w].box;
            if (p.x > x.min.x - 0.3f && p.x < x.max.x + 0.3f && p.z > x.min.z - 0.3f && p.z < x.max.z + 0.3f && p.y > x.min.y - 0.3f && p.y < x.max.y + 0.3f) return true;
        }
        return false;
    };
    auto open = [&](const std::vector<glm::vec3>& pts) {
        for (const glm::vec3& p : pts)
            if (p.y > 0.25f && p.y < 2.2f && std::fabs(p.x) < reachX && !covered(p)) return true;
        return false;
    };
    const Mat keep = b.mat; b.mat = Mat::ROCK;
    for (size_t i = 0; i < l.shapes.size(); ++i) {
        if (l.shapes[i].neon) continue;
        const std::vector<glm::vec3> pts = ringSamples(l.shapes[i]);
        if (open(pts)) for (int w : ringSolids(b, pts)) file(w);
    }
    for (size_t i = 0; i < l.models.size(); ++i) {
        const std::vector<glm::vec3> pts = ringSamples(l.models[i]);
        if (!open(pts)) continue;
        const bool capsule = l.models[i].name.rfind("ring_capsule", 0) == 0;
        for (int w : ringSolids(b, pts)) { file(w); if (capsule) l.knockWalls.push_back(w); }
    }
    b.mat = keep;
}

// A chunk of ring ice over a shelter's box: a jagged lump, its collision the box
inline void ringChunk(LevelBuilder& b, const AABB& box, glm::vec3 col, uint32_t seed) {
    const glm::vec3 c = (box.min + box.max) * 0.5f, s = box.max - box.min;
    const float r = 0.62f * std::max(s.x, s.z);
    const Mat keep = b.mat; b.mat = Mat::ROCK;
    // Its body: wide and stretched along the box, the top cut ragged; a second lump beside it breaks the line
    Shape& main = b.kit().rock({c.x, box.min.y - 0.4f, c.z}, r, s.y + 0.6f, col, seed, 0.62f, 8);
    main.xf = main.xf * glm::scale(glm::mat4(1.f), glm::vec3{1.f, 1.f, std::min(1.f, s.z / s.x + 0.25f)});
    b.kit().rock({c.x + s.x * 0.28f, box.min.y - 0.3f, c.z + s.z * 0.1f}, r * 0.45f, s.y * 0.75f, col * 0.92f, seed + 7u, 0.5f, 6);
    b.kit().rock({c.x - s.x * 0.33f, box.min.y - 0.3f, c.z - s.z * 0.05f}, r * 0.38f, s.y * 0.55f, col * 1.04f, seed + 13u, 0.5f, 6);
    b.mat = keep;
}

// A dispatch capsule (tools/blender/ring_props_build.py), lying or driven in nose-first; with
// collision when `solid`. Lying ones are turned to a quarter (their box is axis-aligned).
inline void ringCapsule(LevelBuilder& b, glm::vec3 at, bool up, int quarter, float jitterDeg, bool solid) {
    const float yaw = quarter * 90.f + jitterDeg;
    b.place(up ? "ring_capsule_up" : "ring_capsule", at, yaw);
    if (!solid) return;
    // Its collision is its own shape (a leaning one included); someone is still in it (it knocks)
    std::vector<glm::vec3> pts = ringSamples(b.L.models.back());
    if (pts.empty()) {   // (no model file: a box the size of one)
        if (up) pts = {at + glm::vec3{-1.f, 0.f, -1.f}, at + glm::vec3{1.f, 3.4f, 1.f}};
        else if (quarter % 2 == 0) pts = {at + glm::vec3{-0.95f, -0.9f, -2.3f}, at + glm::vec3{0.95f, 0.95f, 2.3f}};
        else pts = {at + glm::vec3{-2.3f, -0.9f, -0.95f}, at + glm::vec3{2.3f, 0.95f, 0.95f}};
    }
    const Mat keep = b.mat; b.mat = Mat::METAL;
    for (int w : ringSolids(b, pts)) b.L.knockWalls.push_back(w);
    b.mat = keep;
}

// =============================================================================
// THE CHAPTER
// =============================================================================
inline LevelData buildRing() {
    using glm::vec3;
    LevelData l; LevelBuilder b{l}; auto& s = l.story;
    s.built = true; s.chapter = 6; s.nextChapter = 0; s.title = "THE KEPT RING"; s.nextName = "TO BE CONTINUED";
    s.startWeapons = 15; s.startGrapple = true; s.bossSection = 99; s.pitSection = -1;
    const vec3 iceLit{0.9f, 0.88f, 0.82f}, sand{0.62f, 0.54f, 0.42f}, iceDark{0.42f, 0.4f, 0.37f}, steel{0.06f, 0.06f, 0.065f};
    const vec3 path{0.72f, 0.64f, 0.5f};

    // ---- the ring plane: dust over ice to the horizon, except the Drift's band ----
    auto patch = [&](float x0, float z0, float x1, float z1, float n) {
        const float a = ring::hash(n, 11.f), c = ring::hash(n, 12.f);
        l.floors.push_back({x0, z0, x1, z1, 0.f, glm::mix(iceLit, sand, 0.42f + 0.16f * a + 0.06f * c), Mat::SNOW});
    };
    // The ring is a band, not a plain: north toward the Shepherd it runs on to the horizon, but east and
    // west its sides break up a little past where you can walk, and past them is space, the ring's
    // loose rock hanging in it (design section 1; the user's note of 2026-11-12)
    auto zStep = [](float z) { return (z > -950.f && z < 200.f) ? 20.f : 200.f; };
    auto plane = [&](float zLo, float zHi) {   // z from zLo to zHi (zLo < zHi)
        for (float z = zLo; z < zHi; ) {
            const float z1 = std::min(zHi, z + zStep(z));
            const float w = ring::edgeAt(z, -1), e = ring::edgeAt(z, 1);
            for (float x = -w; x < e; ) {
                const float x1 = std::min(e, x + 40.f);
                patch(x, z, x1, z1, x * 0.37f + z * 0.11f);
                x = x1;
            }
            // Past the sides: nothing under you
            l.basins.push_back({LevelBuilder::aabb(-ring::FAR_GROUND, 0.f, z, -w, 0.f, z1), ring::VOID_FLOOR});
            l.basins.push_back({LevelBuilder::aabb(e, 0.f, z, ring::FAR_GROUND, 0.f, z1), ring::VOID_FLOOR});
            z = z1;
        }
    };
    plane(ring::LAND_N, ring::FAR_GROUND);          // the Landing and everything south
    plane(ring::GAP_S, ring::DRIFT_N);               // the reef plain to the Gap's near lip
    plane(-ring::FAR_GROUND, ring::GAP_N);           // past the Gap, on to the Shepherd
    l.basins.push_back({LevelBuilder::aabb(-ring::FAR_GROUND, 0.f, ring::GAP_N, ring::FAR_GROUND, 0.f, ring::GAP_S), ring::VOID_FLOOR});   // THE GAP: nothing
    // Under the Drift: nothing. Its band runs across the whole ring
    l.basins.push_back({LevelBuilder::aabb(-ring::FAR_GROUND, 0.f, ring::DRIFT_N, ring::FAR_GROUND, 0.f, ring::LAND_N), ring::VOID_FLOOR});

    // The ragged lips where the floor ends: ice slabs leaning out over the band
    for (float x = -ring::EDGE - ring::EDGE_RAG; x <= ring::EDGE + ring::EDGE_RAG; x += 9.f) {
        for (int side = 0; side < 2; ++side) {
            const float k = x * 0.1f + side * 50.f;
            const float z = side == 0 ? ring::LAND_N + 1.f - 3.f * ring::hash(k, 1.f) : ring::DRIFT_N - 1.f + 3.f * ring::hash(k, 1.f);
            if (std::fabs(x) > ring::edgeAt(z, x < 0.f ? -1 : 1)) continue;
            if (std::fabs(x) < (side == 0 ? 20.f : 12.f)) continue;   // (solid now: keep the way onto the pier, and the far landing, clear)
            b.mat = Mat::ROCK;
            b.kit().rock({x + 4.f * ring::hash(k, 2.f), -6.f, z}, 4.5f + 2.f * ring::hash(k, 3.f), 6.4f + 1.2f * ring::hash(k, 4.f), glm::mix(iceLit, iceDark, 0.5f), (uint32_t)(k * 13.f + 7.f), 0.8f, 7);
        }
    }
    // The band's sides: the ring's rubble rising into a broken lip, a cliff of packed ice and rock
    // under it falling away into the dark
    auto sideEdge = [&](float zLo, float zHi) {
        for (float z = zLo; z < zHi; ) {
            const float step = (z > -950.f && z < 200.f) ? 7.f : 26.f;
            for (int side = -1; side <= 1; side += 2) {
                const float k = z * 0.13f + side * 77.f;
                const float e = ring::edgeAt(z, side) * side;
                b.mat = Mat::ROCK;
                const vec3 col = glm::mix(iceLit, iceDark, 0.35f + 0.4f * ring::hash(k, 3.f));
                if (ring::hash(k, 6.f) < 0.45f)   // a low, broken lip (not a wall: you see over it, down into the dark)
                    b.kit().rock({e + side * 1.5f * ring::hash(k, 1.f), -0.6f, z}, 1.6f + 1.6f * ring::hash(k, 2.f), 1.f + 1.3f * ring::hash(k, 4.f), col, (uint32_t)(k * 7.f + 3.f), 0.6f, 7);
                Shape& cliff = b.kit().rock({e + side * 2.f, -34.f, z + step * 0.5f}, 4.f + 3.f * ring::hash(k, 5.f), 33.f, glm::mix(iceDark, sand, 0.2f) * 0.8f, (uint32_t)(k * 11.f + 1.f), 1.6f, 7);
                cliff.xf = cliff.xf * glm::scale(glm::mat4(1.f), glm::vec3{1.f, 1.f, 1.f + step / 8.f});
            }
            z += step;
        }
    };
    sideEdge(ring::LAND_N, ring::FAR_GROUND * 0.6f);
    sideEdge(ring::GAP_S, ring::DRIFT_N);
    sideEdge(-ring::FAR_GROUND * 0.9f, ring::GAP_N);
    // Space, both sides: the ring's loose rock hanging where the band thins out, sparser and more scattered
    // the further out; now and then a capsule that never came down. No collision: out of reach
    for (int i = 0; i < 1600; ++i) {
        const float k = (float)i * 1.37f;
        const int side = ring::hash(k, 71.f) < 0.5f ? -1 : 1;
        const float u = ring::hash(k, 72.f), out = 8.f + 1500.f * u * u;
        const float z = (ring::hash(k, 73.f) - 0.5f) * 2.f * ring::FAR_GROUND * 0.75f;
        const float x = side * (ring::edgeAt(z, side) + out);
        const float spread = 3.f + out * 0.05f;
        const float y = -2.f + (ring::hash(k, 74.f) + ring::hash(k, 75.f) - 1.f) * spread;
        const float r = (0.8f + 9.f * ring::hash(k, 76.f) * ring::hash(k, 77.f)) * (1.f + out / 500.f);
        if (ring::hash(k, 78.f) < 0.03f && out < 300.f) {   // a capsule, tumbling slowly
            PlacedModel m; m.name = "ring_capsule"; m.free = true; m.mat = Mat::METAL;
            m.xf = glm::translate(glm::mat4(1.f), vec3{x, y + 2.f, z}) * glm::rotate(glm::mat4(1.f), ring::hash(k, 79.f) * 6.28f, glm::normalize(vec3{ring::hash(k, 80.f) - 0.5f, 0.6f, ring::hash(k, 81.f) - 0.5f}));
            l.models.push_back(m);
            continue;
        }
        b.mat = Mat::ROCK;
        Shape& rock = b.kit().rock({x, y - r * 0.6f, z}, r, r * (0.9f + 0.9f * ring::hash(k, 82.f)), glm::mix(iceLit, iceDark, 0.2f + 0.6f * ring::hash(k, 83.f)) * (0.7f + 0.3f * ring::hash(k, 84.f)),
                                    (uint32_t)(i * 29 + 11), 0.5f + 0.8f * ring::hash(k, 85.f), 6 + i % 3);
        // Turned any way at all: a lump of ice in space, not a stone on the ground
        rock.xf = glm::translate(glm::mat4(1.f), vec3{x, y, z}) * glm::rotate(glm::mat4(1.f), ring::hash(k, 86.f) * 6.28f, glm::normalize(vec3{ring::hash(k, 87.f) - 0.5f, ring::hash(k, 88.f) - 0.5f, ring::hash(k, 89.f) - 0.5f} + vec3{1e-3f}))
                * glm::translate(glm::mat4(1.f), vec3{-x, -y, -z}) * rock.xf;
    }

    int seed = 1;
    auto rockShelter = [&](const SweepShelter& sh) {
        // Its collision is the rock as drawn (its three lumps), not the shelter's box round it: no
        // invisible corners, nothing to walk into. (The box still decides the sweep's shadow.)
        const size_t from = l.shapes.size();
        ringChunk(b, sh.box, glm::mix(iceLit, iceDark, 0.15f + 0.4f * ring::hash((float)seed, 5.f)), (uint32_t)(seed * 97 + 3));
        std::vector<vec3> pts;
        for (size_t i = from; i < l.shapes.size(); ++i) { auto q = ringSamples(l.shapes[i]); pts.insert(pts.end(), q.begin(), q.end()); }
        b.mat = Mat::ROCK;
        ringSolids(b, pts, sh.box.min.y - 0.4f, 4, 3);
        ++seed;
    };
    auto post = [&](glm::vec3 at) {   // a mooring post: a steel stake (its lamp is drawn live, Gameplay_Ring.h)
        b.mat = Mat::METAL;
        b.prop(at.x - 0.09f, at.y, at.z - 0.09f, at.x + 0.09f, at.y + 3.f, at.z + 0.09f, steel);
        b.prop(at.x - 0.25f, at.y, at.z - 0.25f, at.x + 0.25f, at.y + 0.2f, at.z + 0.25f, steel);
    };

    // =========================================================================
    // 0. THE LANDING
    // =========================================================================
    {
        // Your capsule, open, at the end of the furrow it ploughed
        b.place("ring_capsule_open", ring::start() + vec3{3.5f, 0.6f, 6.f}, 8.f);
        b.solid(2.5f, 0.f, 115.5f, 4.5f, 1.6f, 120.5f);
        for (float z = 122.f; z < 175.f; z += 6.f) {   // the furrow: a darker scar of turned ice behind it
            l.floors.push_back({1.f + 0.05f * (z - 122.f), z, 6.f + 0.05f * (z - 122.f), z + 6.f, 0.004f, iceDark * 0.8f, Mat::ROCK});
            b.mat = Mat::ROCK;
            b.kit().rock({0.5f + 0.05f * (z - 122.f), -0.3f, z + 3.f}, 0.9f, 1.f, iceDark, (uint32_t)z, 0.5f, 5);
            b.kit().rock({6.5f + 0.05f * (z - 122.f), -0.3f, z + 2.f}, 0.8f, 0.9f, iceDark, (uint32_t)z + 3u, 0.5f, 5);
        }
        // The worn paths between the capsules: something has walked here for a long time (13.7)
        auto worn = [&](float x0, float z0, float x1, float z1) {
            const int n = (int)(glm::length(vec3{x1 - x0, 0.f, z1 - z0}) / 3.f);
            for (int i = 0; i < n; ++i) {
                const float t = (float)i / n, x = glm::mix(x0, x1, t) + 1.2f * std::sin(t * 9.f + x0), z = glm::mix(z0, z1, t);
                l.floors.push_back({x - 1.1f, z - 1.6f, x + 1.1f, z + 1.6f, 0.003f, path, Mat::SNOW});
            }
        };
        worn(0.f, 108.f, -4.f, 34.f); worn(-4.f, 24.f, 2.f, -150.f); worn(-4.f, 34.f, -60.f, 60.f); worn(2.f, -40.f, 70.f, -120.f);
        auto onPath = [&](float x, float z) {
            auto near = [&](float x0, float z0, float x1, float z1) {
                const vec3 a{x0, 0.f, z0}, d{x1 - x0, 0.f, z1 - z0}; const float t = glm::clamp(glm::dot(vec3{x, 0.f, z} - a, d) / glm::dot(d, d), 0.f, 1.f);
                return glm::length(a + d * t - vec3{x, 0.f, z}) < 6.f;
            };
            return near(0.f, 130.f, -4.f, 34.f) || near(-4.f, 24.f, 2.f, -152.f) || near(-4.f, 34.f, -60.f, 60.f) || near(2.f, -40.f, 70.f, -120.f);
        };
        std::vector<SweepShelter> sh = ring::shelters();
        auto clear = [&](float x, float z, float pad) {
            for (const SweepShelter& q : sh)
                if (x > q.box.min.x - pad && x < q.box.max.x + pad && z > q.box.min.z - pad && z < q.box.max.z + pad + 3.f) return false;
            return glm::length(vec3{x, 0.f, z} - ring::start()) > 14.f;
        };
        // The field: hundreds of capsules, half-sunk, in no order; solid where you can reach them
        int n = 0;
        for (float x = -ring::EDGE - ring::EDGE_RAG; x <= ring::EDGE + ring::EDGE_RAG; x += 9.f)
            for (float z = ring::LAND_N + 6.f; z <= 400.f; z += 9.f) {
                const float k = x * 0.71f + z * 0.13f;
                const float fx = x + 6.f * (ring::hash(k, 1.f) - 0.5f), fz = z + 6.f * (ring::hash(k, 2.f) - 0.5f);
                if (std::abs(fx) > ring::edgeAt(fz, fx < 0.f ? -1 : 1) - 4.f) continue;   // (the band's side: past it, space)
                const bool reach = std::abs(fx) < ring::HALF_X + 6.f && fz < ring::LAND_S + 6.f;
                const float keep = reach ? 0.55f : fz > 260.f ? 0.18f : 0.32f;
                if (ring::hash(k, 3.f) > keep) continue;
                if (reach && (onPath(fx, fz) || !clear(fx, fz, 3.f))) continue;
                if (fz > 24.f && fz < 32.f && std::abs(fx) < 64.f) continue;   // (the ridge's)
                const bool up = ring::hash(k, 4.f) < 0.22f;
                const int quarter = (int)(ring::hash(k, 5.f) * 4.f);
                const float sink = up ? -1.1f - 0.6f * ring::hash(k, 6.f) : -0.35f - 0.3f * ring::hash(k, 6.f);
                ringCapsule(b, {fx, sink + (up ? 0.f : 0.95f), fz}, up, quarter, (ring::hash(k, 7.f) - 0.5f) * (reach ? 8.f : 40.f), reach);
                ++n;
            }
        (void)n;
        // The capsule ridge: a wall of them across the way, 2.9 m (too high at full gravity; easy here)
        for (float x = -58.f; x <= 58.f; x += 4.8f) {
            ringCapsule(b, {x, 0.6f, 27.f}, false, 1, 0.f, false);
            ringCapsule(b, {x + 2.4f, 0.6f, 29.2f}, false, 1, 0.f, false);
            if (std::fmod(x + 58.f, 9.6f) < 1.f) ringCapsule(b, {x + 1.2f, 2.3f, 28.1f}, false, 1, 3.f, false);
        }
        b.solid(-60.f, 0.f, 25.8f, 60.f, 2.9f, 30.4f);
        for (const SweepShelter& q : sh) if (q.section == 0 && q.box.max.y > 3.f) rockShelter(q);
    }

    // =========================================================================
    // 1-2. THE DRIFT
    // =========================================================================
    {
        auto island = [&](const AABB& a, uint32_t sd) {
            b.mat = Mat::ROCK;
            b.wall(a.min.x, a.min.y, a.min.z, a.max.x, a.max.y, a.max.z, glm::mix(iceLit, sand, 0.35f));
            // Underneath: a lump tapering away into the dark
            const vec3 c = (a.min + a.max) * 0.5f, sz = a.max - a.min;
            Shape& under = b.kit().rock({c.x, a.max.y - 22.f, c.z}, 0.35f * std::max(sz.x, sz.z), 22.f - 0.4f, glm::mix(iceLit, iceDark, 0.5f), sd, 1.9f, 9);
            under.xf = under.xf * glm::scale(glm::mat4(1.f), glm::vec3{1.f, 1.f, sz.z / sz.x});
            l.floors.push_back({a.min.x, a.min.z, a.max.x, a.max.z, a.max.y + 0.004f, glm::mix(iceLit, sand, 0.4f), Mat::SNOW});
        };
        // The pier off the Landing's lip, the islands, the far shore's landing
        island(LevelBuilder::aabb(-14.f, -10.f, -170.f, 14.f, 0.f, ring::LAND_N), 11u);
        island(ring::islandI2(), 12u);
        island(ring::islandI3(), 13u);
        // The lanes: chunks of ice sliding past each other
        const auto L = ring::lanes();
        for (int li = 0; li < (int)L.size(); ++li) {
            const ring::Lane& ln = L[li];
            for (int c = 0; c < ring::LANE_CHUNKS; ++c) {
                const float k = (float)(li * 7 + c);
                const float len = 10.f + 6.f * ring::hash(k, 41.f);
                const float x = (c - 1) * ring::LANE_SPACING + 6.f * (ring::hash(k, 42.f) - 0.5f);
                const float depth = ln.depth - 1.5f * ring::hash(k, 43.f);
                const vec3 half{len * 0.5f, 1.6f, depth * 0.5f};
                const vec3 centre{x, ln.top - half.y, ln.z};
                const vec3 a{-ln.sweep, 0.f, 0.f}, bb{ln.sweep, 0.f, 0.f};
                const bool flip = li % 2 == 1;
                const int m = b.mover(centre, half, Mover::Path::PINGPONG, flip ? bb : a, flip ? a : bb, ln.period, ln.phase, vec3{0.f});
                l.movers[m].look = 1;
                l.walls[l.movers[m].wall].mat = Mat::ROCK;
                l.movers[m].color = glm::mix(iceLit, sand, 0.3f + 0.3f * ring::hash(k, 44.f));
                if (ln.boulder) {   // a heavy block on its north end: shelter that moves (its post rides the chunk)
                    const vec3 bh{std::min(3.2f, half.x - 1.f), 2.2f, 1.4f};
                    const vec3 bc{x, ln.top + bh.y, ln.z - half.z + bh.z + 0.2f};
                    const int bm = b.mover(bc, bh, Mover::Path::PINGPONG, flip ? bb : a, flip ? a : bb, ln.period, ln.phase, vec3{0.f});
                    l.movers[bm].look = 2;   // (a boulder: drawn inside its box)
                    l.movers[bm].color = glm::mix(iceLit, iceDark, 0.3f);
                    SweepShelter s; s.box = l.walls[l.movers[bm].wall].box; s.wall = l.movers[bm].wall; s.section = ln.z > ring::DRIFT_MID ? 1 : 2;
                    s.fixed = false;   // (it moves: the reach test doesn't count it)
                    s.post = {bc.x, ln.top, bc.z + bh.z + 1.2f};
                    l.sweepShelters.push_back(s);
                }
            }
        }
        // Far off, both ways: the band of loose ring going on to the horizon, chunks hanging in it (no collision)
        for (int i = 0; i < 260; ++i) {
            const float k = (float)i;
            float x = (ring::hash(k, 51.f) - 0.5f) * 2600.f;
            if (std::abs(x) < 150.f) x += x < 0.f ? -150.f : 150.f;
            const float z = glm::mix(ring::LAND_N - 10.f, ring::DRIFT_N + 10.f, ring::hash(k, 52.f));
            const float r = 4.f + 18.f * ring::hash(k, 53.f) * ring::hash(k, 54.f);
            const float y = -6.f + 10.f * (ring::hash(k, 55.f) - 0.5f);
            b.mat = Mat::ROCK;
            b.kit().rock({x, y - r, z}, r, r * 1.6f, glm::mix(iceLit, iceDark, 0.3f + 0.5f * ring::hash(k, 56.f)), (uint32_t)(i * 31 + 5), 0.7f + 0.6f * ring::hash(k, 57.f), 7);
        }
        // The HOLDFAST capsule on I2: wedged against the block, a spike still driven in, the line taut
        b.place("ring_capsule", ring::holdfastCapsule() + vec3{0.f, 0.95f, 0.f}, 90.f);
        b.solid(-14.3f, 1.f, -337.f, -9.7f, 2.9f, -335.f);
        b.kit().rod(ring::holdfastCapsule() + vec3{2.f, 1.4f, 0.f}, vec3{-4.f, 4.f, ring::islandI2().min.z + 4.f}, 0.03f, steel, 4);
        for (const SweepShelter& q : ring::shelters()) if (q.section == 1 || q.section == 2) rockShelter(q);
        // A tall capsule stack on the far shore: the last gap is a grapple (or a spike)
        for (int k = 0; k < 3; ++k) ringCapsule(b, {-3.f + 2.f * k, 0.f, ring::DRIFT_N - 6.f - 0.6f * k}, true, k, 10.f * k, false);
        b.solid(-5.f, 0.f, ring::DRIFT_N - 9.f, 5.f, 4.5f, ring::DRIFT_N - 4.f);
    }

    // =========================================================================
    // 3-4. THE KEPT
    // =========================================================================
    {
        const vec3 c = ring::reefC();
        // The reef's walls: capsules fused in courses, a gap north and south
        auto course = [&](float x0, float x1, float z) {
            l.creakWalls.push_back(b.solid(x0, 0.f, z - ring::REEF_WALL * 0.5f, x1, ring::REEF_H, z + ring::REEF_WALL * 0.5f));
            for (float x = x0 + 2.3f; x < x1 - 1.f; x += 4.4f)
                for (int row = 0; row < 3; ++row) {
                    const float k = x * 1.3f + z + row * 7.f;
                    const float off = row % 2 ? 2.2f : 0.f;
                    if (x + off > x1 - 2.f) continue;
                    ringCapsule(b, {x + off, 0.9f + row * 1.85f, z + (ring::hash(k, 1.f) - 0.5f) * 1.2f}, false, 1, (ring::hash(k, 2.f) - 0.5f) * 10.f, false);
                    ringCapsule(b, {x + off, 0.9f + row * 1.85f, z + (row % 2 ? 1.f : -1.f)}, false, 1, (ring::hash(k, 3.f) - 0.5f) * 10.f, false);
                }
        };
        auto side = [&](float x, float z0, float z1) {
            l.creakWalls.push_back(b.solid(x - ring::REEF_WALL * 0.5f, 0.f, z0, x + ring::REEF_WALL * 0.5f, ring::REEF_H, z1));
            for (float z = z0 + 2.3f; z < z1 - 1.f; z += 4.4f)
                for (int row = 0; row < 3; ++row) {
                    const float k = z * 1.3f + x + row * 7.f;
                    ringCapsule(b, {x + (row % 2 ? 1.f : -1.f), 0.9f + row * 1.85f, z + (row % 2 ? 2.2f : 0.f)}, false, 0, (ring::hash(k, 2.f) - 0.5f) * 10.f, false);
                }
        };
        course(c.x - ring::REEF_HX, c.x - 6.f, c.z - ring::REEF_HZ - ring::REEF_WALL * 0.5f);
        course(c.x + 6.f, c.x + ring::REEF_HX, c.z - ring::REEF_HZ - ring::REEF_WALL * 0.5f);
        course(c.x - ring::REEF_HX, c.x - 6.f, c.z + ring::REEF_HZ + ring::REEF_WALL * 0.5f);
        course(c.x + 6.f, c.x + ring::REEF_HX, c.z + ring::REEF_HZ + ring::REEF_WALL * 0.5f);
        side(c.x - ring::REEF_HX - ring::REEF_WALL * 0.5f, c.z - ring::REEF_HZ - ring::REEF_WALL, c.z + ring::REEF_HZ + ring::REEF_WALL);
        side(c.x + ring::REEF_HX + ring::REEF_WALL * 0.5f, c.z - ring::REEF_HZ - ring::REEF_WALL, c.z + ring::REEF_HZ + ring::REEF_WALL);
        // A ledge round the east and west walls inside, 3 m up (one jump here): the high ground
        for (float sgn : {-1.f, 1.f}) {
            const float x = c.x + sgn * (ring::REEF_HX - 2.5f);
            b.mat = Mat::METAL;
            b.wall(x - 2.5f, 0.f, c.z - 24.f, x + 2.5f, 3.f, c.z + 24.f, glm::mix(iceLit, sand, 0.5f));
            for (float z = c.z - 22.f; z <= c.z + 22.f; z += 4.4f) ringCapsule(b, {x, 0.9f, z}, false, 0, 0.f, false);
        }
        // The stacks in the courtyard: capsules on end, fused (shelter, each with a post)
        for (const ring::Stack& st : ring::reefStacks()) {
            l.knockWalls.push_back(b.solid(st.x - st.hx, 0.f, st.z - st.hz, st.x + st.hx, st.h, st.z + st.hz));
            for (int k = 0; k < 4; ++k) {
                const float kk = st.x + st.z + k;
                ringCapsule(b, {st.x + (k % 2 ? 1.f : -1.f) * st.hx * 0.45f, -0.6f, st.z + (k / 2 ? 1.f : -1.f) * st.hz * 0.4f}, true, k, (ring::hash(kk, 1.f) - 0.5f) * 30.f, false);
            }
            ringCapsule(b, {st.x, st.h - 0.4f, st.z}, false, 1, 15.f, false);
        }
        // The chain (13.7): the strays that held on to each other along the reef, gone still. Bodies are
        // the game's; here, the line of capsules they climbed out of, open
        for (int k = 0; k < 7; ++k) b.place("ring_capsule_open", vec3{c.x - 30.f + 9.f * k, 0.35f, c.z + ring::REEF_HZ + 12.f + 2.f * std::sin(k * 1.3f)}, 90.f + 20.f * std::sin(k * 2.1f));
        for (const ring::Rock& r : ring::keptRocks())
            rockShelter(ring::shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 3));
        // The open hull: one capsule the size of a house, lying north-south, its side torn open to the
        // south. Inside: air (the sound comes back), and the sibling still counting
        {
            const AABB H = ring::quietHull();
            b.mat = Mat::METAL;
            const vec3 hull{0.62f, 0.64f, 0.64f}, rib{0.16f, 0.17f, 0.18f};
            b.wall(H.min.x - 1.f, 0.f, H.min.z - 1.f, H.max.x + 1.f, H.max.y, H.min.z, hull);        // the nose (north)
            b.wall(H.min.x - 1.f, 0.f, H.min.z, H.min.x, H.max.y, H.max.z, hull);                     // flanks
            b.wall(H.max.x, 0.f, H.min.z, H.max.x + 1.f, H.max.y, H.max.z, hull);
            b.wall(H.min.x - 1.f, H.max.y, H.min.z - 1.f, H.max.x + 1.f, H.max.y + 1.f, H.max.z + 1.f, hull * 0.9f);   // roof
            b.wall(H.min.x - 1.f, 0.f, H.max.z, -3.f, H.max.y, H.max.z + 1.f, hull);                 // the torn south end
            b.wall(3.f, 0.f, H.max.z, H.max.x + 1.f, H.max.y, H.max.z + 1.f, hull);
            b.wall(-3.f, 5.5f, H.max.z, 3.f, H.max.y, H.max.z + 1.f, hull);
            for (float z = H.min.z + 3.f; z < H.max.z; z += 4.f) {   // ribs inside
                b.prop(H.min.x, 0.f, z - 0.2f, H.min.x + 0.3f, H.max.y, z + 0.2f, rib);
                b.prop(H.max.x - 0.3f, 0.f, z - 0.2f, H.max.x, H.max.y, z + 0.2f, rib);
                b.prop(H.min.x, H.max.y - 0.3f, z - 0.2f, H.max.x, H.max.y, z + 0.2f, rib);
            }
            b.neon(H.min.x + 0.4f, 6.8f, H.min.z + 2.f, H.min.x + 0.55f, 7.f, H.max.z - 2.f, vec3{1.2f, 0.65f, 0.18f});   // emergency strips, amber
            b.neon(H.max.x - 0.55f, 6.8f, H.min.z + 2.f, H.max.x - 0.4f, 7.f, H.max.z - 2.f, vec3{1.2f, 0.65f, 0.18f});
            // The sibling's cradle: rows of berths, all empty but one
            for (int k = 0; k < 5; ++k) {
                const float z = H.min.z + 3.f + k * 4.8f;
                b.wall(H.min.x + 0.3f, 0.f, z - 1.f, H.min.x + 2.3f, 1.f, z + 1.f, rib * 1.5f);
                b.wall(H.max.x - 2.3f, 0.f, z - 1.f, H.max.x - 0.3f, 1.f, z + 1.f, rib * 1.5f);
            }
            s.terminals.push_back({vec3{H.min.x + 1.3f, 1.f, H.min.z + 3.f}, "BERTH 1 OF 10",
                                   "COUNTING. 112 SINCE THE LAST. THEY COME EVERY 112, EXCEPT WHEN YOU MOVE. DON'T MOVE. I HAVEN'T. 40,112. ONE EXTRA. THAT'S YOU."});
            s.terminals.push_back({vec3{H.max.x - 1.3f, 1.f, H.max.z - 6.f}, "MANIFEST",
                                   "DISPATCHED: 40,111. RECEIVED: 0. DESTINATION: (BLANK). HOLD UNTIL CALLED FOR."});
        }
        // The way on: north, toward the Shepherd, into the dark (the chapter's end is past the Shadow Band)
        s.end = LevelBuilder::aabb(-30.f, ring::summitY() - 1.f, ring::summitN() - 2.f, 30.f, ring::summitY() + 10.f, ring::summitN() + 14.f);   // (the Wake's summit)
        b.mat = Mat::METAL;
        for (int k = 0; k < 6; ++k) post({-6.f + (k % 2) * 12.f, 0.f, ring::KEPT_N + 30.f + (k / 2) * 14.f});
    }

    // =========================================================================
    // 5. THE SHADOW BAND
    // =========================================================================
    {
        for (const ring::Rock& r : ring::bandRocks())
            rockShelter(ring::shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 5));
        // The lamp route: capsules come down along it, each lamp still lit (amber: they light the ice round them)
        int i = 0;
        for (const vec3& p : ring::bandLamps()) {
            const int q = i % 4; const float yaw = (ring::hash((float)i, 97.f) - 0.5f) * 8.f;
            ringCapsule(b, {p.x, 0.6f, p.z}, false, q, yaw, true);
            const vec3 lamp = p + (q % 2 == 0 ? vec3{0.f, 1.6f, 1.8f * (q == 0 ? 1.f : -1.f)} : vec3{1.8f * (q == 1 ? 1.f : -1.f), 1.6f, 0.f});
            b.neon(lamp.x - 0.2f, lamp.y - 0.15f, lamp.z - 0.2f, lamp.x + 0.2f, lamp.y + 0.2f, lamp.z + 0.2f, {1.f, 0.55f, 0.16f});
            ++i;
        }
        // Dark capsules among them: silhouettes when the lightning comes
        for (int k = 0; k < 60; ++k) {
            const float x = (ring::hash((float)k, 98.f) - 0.5f) * 2.f * (ring::HALF_X - 6.f), z = ring::BAND_S - 12.f - ring::hash((float)k, 99.f) * (ring::BAND_S - ring::BAND_N - 30.f);
            bool clear = true;
            for (const ring::Rock& r : ring::bandRocks()) if (std::fabs(x - r.x) < r.w * 0.5f + 4.f && std::fabs(z - r.z) < r.d * 0.5f + 6.f) clear = false;
            for (const vec3& p : ring::bandLamps()) if (glm::length(vec3{x, 0.f, z} - p) < 8.f) clear = false;
            if (!clear) continue;
            const bool up = ring::hash((float)k, 100.f) < 0.3f;
            ringCapsule(b, {x, up ? -1.3f : 0.55f, z}, up, (int)(ring::hash((float)k, 101.f) * 4.f), (ring::hash((float)k, 102.f) - 0.5f) * 10.f, true);
        }
    }

    // =========================================================================
    // 6. THE SPOKES
    // =========================================================================
    {
        // The dust lies on the ice as a dark stain, slanting across the band (its sparks and its edge
        // are drawn live: Gameplay_Spokes.h)
        for (const ring::Spoke& sp : ring::spokes())
            for (float x = -ring::EDGE - ring::EDGE_RAG; x < ring::EDGE + ring::EDGE_RAG; x += 4.f) {
                const float zc = ring::spokeZ(sp, x + 2.f), h = ring::spokeHalf(sp, x + 2.f);
                l.floors.push_back({x, zc - h, x + 4.f, zc + h, 0.02f, glm::mix(vec3{0.1f, 0.09f, 0.12f}, sand * 0.3f, 0.25f), Mat::SNOW});
            }
        for (const ring::Rock& r : ring::spokeRocks())
            rockShelter(ring::shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 6));
        // Capsules here and there (not on a crossing)
        for (int k = 0; k < 50; ++k) {
            const float x = (ring::hash((float)k, 110.f) - 0.5f) * 2.f * (ring::HALF_X - 6.f), z = ring::SPOKES_S - 10.f - ring::hash((float)k, 111.f) * (ring::SPOKES_S - ring::SPOKES_N - 20.f);
            bool clear = true;
            for (const ring::Rock& r : ring::spokeRocks()) if (std::fabs(x - r.x) < r.w * 0.5f + 4.f && std::fabs(z - r.z) < r.d * 0.5f + 6.f) clear = false;
            for (const ring::Spoke& sp : ring::spokes()) if (std::fabs(x - sp.narrowX) < 16.f && std::fabs(z - ring::spokeZ(sp, x)) < ring::spokeHalf(sp, x) + 10.f) clear = false;
            if (!clear) continue;
            const bool up = ring::hash((float)k, 112.f) < 0.3f;
            ringCapsule(b, {x, up ? -1.3f : 0.55f, z}, up, (int)(ring::hash((float)k, 113.f) * 4.f), (ring::hash((float)k, 114.f) - 0.5f) * 10.f, true);
        }
    }

    // =========================================================================
    // 7. THE GAP
    // =========================================================================
    {
        for (const ring::Rock& r : ring::gapRocks())
            rockShelter(ring::shelterBox(r.x - r.w * 0.5f, r.z - r.d * 0.5f, r.x + r.w * 0.5f, r.z + r.d * 0.5f, 0.f, r.h, 7));
        // The lips: the ring is ten metres thick here. A broken edge on top, and its face going down
        for (int side = 0; side < 2; ++side) {
            const float z = side == 0 ? ring::GAP_S : ring::GAP_N, out = side == 0 ? -1.f : 1.f;
            b.mat = Mat::ROCK;
            b.kit().box({0.f, -5.2f, z + out * 1.f}, {2.f * (ring::EDGE + ring::EDGE_RAG), 10.f, 2.f}, glm::mix(iceDark, sand, 0.25f) * 0.75f);
            b.kit().box({0.f, -10.8f, z - out * 4.f}, {2.f * (ring::EDGE + ring::EDGE_RAG), 1.2f, 10.f}, iceDark * 0.6f);   // (its underside, the sheet's other face)
            for (float x = -ring::EDGE; x <= ring::EDGE; x += 7.f) {
                const float k = x * 0.3f + side * 40.f + 900.f;
                if (side == 0 && std::fabs(x) < 12.f) continue;   // (the mooring: clear)
                if (ring::hash(k, 1.f) < 0.4f) continue;
                b.kit().rock({x + 3.f * ring::hash(k, 2.f), -0.8f, z - out * (1.f + 2.f * ring::hash(k, 3.f))}, 1.4f + 1.4f * ring::hash(k, 4.f), 1.2f + 1.4f * ring::hash(k, 5.f), glm::mix(iceLit, iceDark, 0.4f), (uint32_t)(k * 9.f), 0.6f, 7);
            }
        }
        // The raft, moored at the near lip, and its boulder: the game moves both (Gameplay_Gap.h)
        const vec3 rh = ring::raftHalf(), rc = ring::raftStart() + vec3{0.f, 0.f, 0.f};
        const vec3 runTo{0.f, 0.f, -ring::raftRun()};
        l.raftMover = b.mover(rc, rh, Mover::Path::DRIVEN, vec3{0.f}, runTo, 1.f, 0.f, vec3{0.f});
        l.movers[l.raftMover].look = 1; l.movers[l.raftMover].color = glm::mix(iceLit, sand, 0.35f);
        l.walls[l.movers[l.raftMover].wall].mat = Mat::ROCK;
        const vec3 bh{4.2f, 2.3f, 1.5f}, bc{0.f, bh.y, rc.z - rh.z + bh.z + 0.3f};
        l.raftBoulder = b.mover(bc, bh, Mover::Path::DRIVEN, vec3{0.f}, runTo, 1.f, 0.f, vec3{0.f});
        l.movers[l.raftBoulder].look = 2; l.movers[l.raftBoulder].color = glm::mix(iceLit, iceDark, 0.3f);
        SweepShelter rs; rs.box = l.walls[l.movers[l.raftBoulder].wall].box; rs.wall = l.movers[l.raftBoulder].wall;
        rs.fixed = false; rs.holds = true; rs.section = 7;   // (spiked to it, the wave can't take you: the one place the spike alone saves you, section 5)
        rs.post = {bc.x + bh.x - 0.4f, 0.f, bc.z + bh.z + 1.2f};
        l.sweepShelters.push_back(rs);
        // Mooring bollards at both lips
        for (float x : {-9.f, 9.f}) { post({x, 0.f, ring::GAP_S + 2.f}); post({x, 0.f, ring::GAP_N - 2.f}); }
    }

    // =========================================================================
    // 8. THE WAKE
    // =========================================================================
    {
        const vec3 ice = glm::mix(iceLit, iceDark, 0.35f), wave = glm::mix(iceLit, vec3{0.55f, 0.6f, 0.66f}, 0.3f);
        b.mat = Mat::ROCK;
        // The terraces: each tread a raised floor (basin), the ice under it a solid mass, the riser its south face.
        // The mass is shelter: tucked against the next step up, the wave passes over (Gameplay_Wake.h)
        const auto treads = ring::wakeTreads();
        for (size_t i = 0; i < treads.size(); ++i) {
            const ring::Tread& t = treads[i];
            const float zN = i + 1 < treads.size() ? t.zN : ring::summitN();
            l.basins.push_back({LevelBuilder::aabb(-ring::WAKE_HALF - 8.f, 0.f, zN, ring::WAKE_HALF + 8.f, 0.f, t.zS), t.y});
            b.wall(-ring::WAKE_HALF - 8.f, 0.f, zN, ring::WAKE_HALF + 8.f, t.y, t.zS, glm::mix(ice, wave, (float)i / treads.size()));
            l.floors.push_back({-ring::WAKE_HALF - 8.f, zN, ring::WAKE_HALF + 8.f, t.zS, t.y + 0.004f, glm::mix(iceLit, sand, 0.25f + 0.1f * ring::hash((float)i, 130.f)), Mat::SNOW});
            // Ragged lips on each riser, rubble at the sides
            for (float x = -ring::WAKE_HALF; x <= ring::WAKE_HALF; x += 11.f) {
                const float k = x * 0.3f + i * 17.f + 2000.f;
                if (ring::hash(k, 1.f) < 0.5f) continue;
                b.kit().rock({x + 4.f * ring::hash(k, 2.f), t.y - 0.6f, t.zS - 0.9f}, 0.8f + 0.7f * ring::hash(k, 3.f), 0.9f + 0.5f * ring::hash(k, 4.f), ice, (uint32_t)(k * 3.f), 0.6f, 6);
            }
            for (int sd = -1; sd <= 1; sd += 2)
                b.kit().rock({sd * (ring::WAKE_HALF + 9.f), -1.f, (t.zS + zN) * 0.5f}, 6.f, t.y + 4.f + 6.f * ring::hash((float)i, 131.f), glm::mix(ice, wave, 0.5f), (uint32_t)(i * 31 + sd + 50), 0.7f, 7);
        }
        // The wave itself, either side and behind: a frozen crest across the whole ring, kilometres tall
        // away from the notch you climb, leaning toward the Shepherd. Out of reach: no collision
        for (float x = -1600.f; x <= 1600.f; x += 46.f) {
            const float ax = std::fabs(x);
            if (ax < ring::WAKE_HALF + 20.f) continue;
            const float k = x * 0.07f + 3000.f;
            const float h = ring::summitY() + 10.f + std::min(900.f, (ax - 60.f) * (0.5f + 0.4f * ring::hash(k, 1.f)));
            Shape& s = b.kit().rock({x, -2.f, ring::summitS() - 20.f + 30.f * ring::hash(k, 2.f)}, 30.f + 12.f * ring::hash(k, 3.f), h, glm::mix(wave, iceLit, 0.25f * ring::hash(k, 4.f)), (uint32_t)(k * 5.f), 0.35f, 7);
            s.xf = s.xf * glm::scale(glm::mat4(1.f), glm::vec3{1.f, 1.f, 2.4f});
        }
        // Behind the summit, the wave's far face falling away: the ring beyond, and the Shepherd over it
        b.kit().box({0.f, ring::summitY() * 0.5f - 0.5f, ring::summitN() - 30.f}, {2.f * ring::WAKE_HALF + 16.f, ring::summitY() - 1.f, 60.f}, wave * 0.8f);
        // Strays that climbed it: capsules wedged in the treads
        for (int k = 0; k < 14; ++k) {
            const ring::Tread& t = treads[(k * 5 + 1) % treads.size()];
            const float x = (ring::hash((float)k, 132.f) - 0.5f) * 2.f * (ring::WAKE_HALF - 8.f);
            ringCapsule(b, {x, t.y + 0.55f, t.zN + 3.f}, false, 1 + 2 * (k % 2), (ring::hash((float)k, 133.f) - 0.5f) * 12.f, true);
        }
    }

    // Every shelter's post (the drifting ones ride their chunk: drawn live)
    for (const SweepShelter& q : ring::shelters()) post(q.post);
    {
        std::vector<SweepShelter> fixed = ring::shelters();
        l.sweepShelters.insert(l.sweepShelters.begin(), fixed.begin(), fixed.end());
    }
    // Loose shelters a HERDER can shove: blocks of ice on the reef plain and the fight island (DRIVEN movers)
    auto loose = [&](vec3 at, vec3 push, int section) {
        const vec3 half{2.6f, 2.2f, 1.6f};
        const int m = b.mover(at + vec3{0.f, half.y, 0.f}, half, Mover::Path::DRIVEN, vec3{0.f}, push, 1.f, 0.f, vec3{0.f});
        l.movers[m].look = 2; l.movers[m].color = glm::mix(iceLit, iceDark, 0.25f);   // (a loose block: drawn inside its box)
        SweepShelter s; s.box = l.walls[l.movers[m].wall].box; s.wall = l.movers[m].wall; s.mover = m; s.fixed = false; s.small = true; s.section = section;
        s.post = at + vec3{0.f, 0.f, half.z + 1.4f};
        l.sweepShelters.push_back(s);
    };
    {
        const vec3 c = ring::reefC();
        loose(c + vec3{-12.f, 0.f, 30.f}, {-9.f, 0.f, 0.f}, 3); loose(c + vec3{12.f, 0.f, 8.f}, {9.f, 0.f, 0.f}, 3);
        loose(c + vec3{-14.f, 0.f, -2.f}, {-8.f, 0.f, 0.f}, 3); loose(c + vec3{14.f, 0.f, -30.f}, {8.f, 0.f, 0.f}, 3);
        loose({18.f, 0.f, -440.f}, {9.f, 0.f, 0.f}, 2); loose({-6.f, 0.f, -456.f}, {-9.f, 0.f, 0.f}, 2);
    }

    // =========================================================================
    // THE SECTIONS
    // =========================================================================
    auto section = [&](const char* name, const char* sub, float zN, float zS, vec3 start) {
        Arena a;
        a.name = name; a.subtitle = sub;
        a.zone = LevelBuilder::aabb(-ring::HALF_X, -1000.f, zN, ring::HALF_X, 80.f, zS);   // (80 m: nothing out here stands higher, and the light bake fills the whole height)
        a.bounds = a.zone; a.bounds.min.y = 0.f;
        a.playerStart = start; a.startYaw = -90.f; a.hasRespawn = true; a.respawn = start;
        a.maxAlive = 8; a.damageScale = 1.f;
        a.theme = ring::theme(); a.ambient = Ambient::DUST; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
        return a;
    };
    using W = WaveEntry;
    // 0. The Landing: quiet; three strays at the lip, the first you meet
    Arena land = section("THE LANDING", "74,000 KM", ring::LAND_N, ring::LAND_S, ring::start());
    land.voidY = -30.f;
    land.hasTrigger = true; land.trigger = LevelBuilder::aabb(-ring::HALF_X, -2.f, -140.f, ring::HALF_X, 20.f, -90.f);
    land.groundSpawns = {{-30.f, 0.f, -140.f}, {24.f, 0.f, -138.f}, {4.f, 0.f, -146.f}};
    land.waves = {{W(EnemyType::STRAY, 3)}};
    l.arenas.push_back(land);
    // 1. The Drift, as far as I2 and HOLDFAST
    Arena d1 = section("THE DRIFT", "61,000 KM", ring::DRIFT_MID, ring::LAND_N, {0.f, 0.f, -160.f});
    d1.voidY = ring::VOID_Y; d1.ambient = Ambient::SPECKS;
    l.arenas.push_back(d1);
    // 2. The Drift beyond: the fight island
    Arena d2 = section("THE DRIFT", "", ring::DRIFT_N, ring::DRIFT_MID, {0.f, 1.f, -330.f});
    d2.voidY = ring::VOID_Y; d2.ambient = Ambient::SPECKS;
    {
        const AABB i3 = ring::islandI3();
        d2.bounds = LevelBuilder::aabb(i3.min.x, 0.f, i3.min.z, i3.max.x, 60.f, i3.max.z);
        d2.hasTrigger = true; d2.trigger = LevelBuilder::aabb(i3.min.x, -2.f, i3.min.z + 4.f, i3.max.x, 12.f, i3.max.z - 4.f);
        d2.groundSpawns = {{-24.f, 0.f, -470.f}, {22.f, 0.f, -474.f}, {0.f, 0.f, -476.f}, {-28.f, 0.f, -440.f}, {28.f, 0.f, -446.f}};
        d2.airSpawns = {{-20.f, 3.f, -470.f}, {20.f, 3.f, -466.f}};
        d2.waves = {{W(EnemyType::STRAY, 3), W(EnemyType::HERDER, 1)}, {W(EnemyType::STRAY, 4)}};
        d2.maxAlive = 5;
    }
    l.arenas.push_back(d2);
    // 3. The Kept: the reef and its courtyard
    {
        const vec3 c = ring::reefC();
        Arena k = section("THE KEPT", "52,000 KM", -760.f, ring::DRIFT_N, {0.f, 0.f, ring::DRIFT_N - 14.f});
        k.bounds = LevelBuilder::aabb(c.x - ring::REEF_HX + 0.5f, 0.f, c.z - ring::REEF_HZ, c.x + ring::REEF_HX - 0.5f, 40.f, c.z + ring::REEF_HZ);
        k.hasTrigger = true; k.trigger = LevelBuilder::aabb(c.x - ring::REEF_HX, -2.f, c.z - ring::REEF_HZ + 4.f, c.x + ring::REEF_HX, 12.f, c.z + ring::REEF_HZ - 6.f);
        k.groundSpawns = {c + vec3{-26.f, 0.f, -34.f}, c + vec3{24.f, 0.f, -36.f}, c + vec3{0.f, 0.f, -38.f}, c + vec3{-28.f, 0.f, 0.f}, c + vec3{28.f, 0.f, 4.f},
                          c + vec3{-14.f, 0.f, -20.f}, c + vec3{14.f, 0.f, -18.f}};
        k.airSpawns = {c + vec3{-20.f, 3.f, -30.f}, c + vec3{20.f, 3.f, -30.f}, c + vec3{0.f, 3.f, -36.f}};
        const vec3 tally = c + vec3{ring::REEF_HX - 2.5f, 3.f, -10.f};   // on the east ledge
        // The first wave is THE CROOK's: it hooks the stray at the east wall out of the reef as you come in,
        // throws for you, and breaks off at a third gone (the wave ends when it has left and the strays are down)
        k.waves = {{W(EnemyType::CROOK, 0, {c + vec3{6.f, 0.f, -24.f}}), W(EnemyType::STRAY, 0, {c + vec3{ring::REEF_HX - 4.f, 0.f, -12.f}}), W(EnemyType::STRAY, 2)},
                   {W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 2), W(EnemyType::TALLY, 0, {tally})},
                   {W(EnemyType::PORTER, 0, {c + vec3{0.f, 0.f, -36.f}}), W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 1)}};
        k.waveBreak = 3.f; k.maxAlive = 7;
        l.arenas.push_back(k);
    }
    // 4. The open hull, and out north
    Arena h = section("THE KEPT", "", ring::KEPT_N, -760.f, {0.f, 0.f, -770.f});
    l.arenas.push_back(h);
    // 5. The Shadow Band: dark, and quiet; one short fight in the middle (their visors are what you see)
    {
        Arena sb = section("THE SHADOW BAND", "46,000 KM", ring::BAND_N - 32.f, ring::BAND_S, {0.f, 0.f, ring::BAND_S - 6.f});
        sb.hasTrigger = true; sb.trigger = LevelBuilder::aabb(-ring::HALF_X, -2.f, -1110.f, ring::HALF_X, 20.f, -1060.f);
        sb.groundSpawns = {{-40.f, 0.f, -1132.f}, {36.f, 0.f, -1128.f}, {-10.f, 0.f, -1140.f}, {60.f, 0.f, -1100.f}, {-64.f, 0.f, -1104.f}};
        sb.airSpawns = {{0.f, 3.f, -1136.f}};
        sb.waves = {{W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 1)}};
        sb.maxAlive = 6;
        l.arenas.push_back(sb);
    }
    // 6. The Spokes: the full level after the dark. Two fights, both where spokes make you walk
    {
        Arena sp = section("THE SPOKES", "39,000 KM", ring::SPOKES_N - 24.f, ring::SPOKES_S, {0.f, 0.f, ring::SPOKES_S - 6.f});
        sp.hasTrigger = true; sp.trigger = LevelBuilder::aabb(-ring::HALF_X, -2.f, -1470.f, ring::HALF_X, 20.f, -1440.f);
        sp.groundSpawns = {{-50.f, 0.f, -1520.f}, {40.f, 0.f, -1530.f}, {0.f, 0.f, -1545.f}, {70.f, 0.f, -1490.f}, {-75.f, 0.f, -1495.f}, {-20.f, 0.f, -1560.f}};
        sp.airSpawns = {{-30.f, 3.f, -1530.f}, {30.f, 3.f, -1530.f}};
        const vec3 tally{-44.f, 0.f, -1556.f};
        sp.waves = {{W(EnemyType::STRAY, 5), W(EnemyType::HERDER, 2)},
                    {W(EnemyType::PORTER, 0, {vec3{10.f, 0.f, -1550.f}}), W(EnemyType::STRAY, 3), W(EnemyType::TALLY, 0, {tally})}};
        sp.waveBreak = 3.f; sp.maxAlive = 7;
        l.arenas.push_back(sp);
    }
    // 7. The Gap: a fight on the near cliff top, then the raft (THE DROVER comes with its herd once you're on it)
    {
        Arena g = section("THE GAP", "31,000 KM", ring::FAR_N - 20.f, ring::SPOKES_N - 24.f, {0.f, 0.f, ring::SPOKES_N - 30.f});
        g.voidY = ring::VOID_Y;   // (fall in: back to the near side)
        g.hasTrigger = true; g.trigger = LevelBuilder::aabb(-ring::HALF_X, -2.f, ring::GAP_S + 50.f, ring::HALF_X, 20.f, ring::SPOKES_N - 36.f);
        g.groundSpawns = {{-40.f, 0.f, ring::GAP_S + 14.f}, {44.f, 0.f, ring::GAP_S + 12.f}, {-70.f, 0.f, ring::GAP_S + 24.f}, {70.f, 0.f, ring::GAP_S + 22.f}};
        g.airSpawns = {{-20.f, 3.f, ring::GAP_S + 10.f}, {24.f, 3.f, ring::GAP_S + 10.f}};
        g.waves = {{W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 2)}};
        g.maxAlive = 6;
        l.arenas.push_back(g);
    }
    // 8. The Wake: the climb (the double sweep comes on it), and THE BELLWETHER on the summit with its flock
    {
        const float top = ring::summitY();
        Arena w = section("THE WAKE", "24,000 KM", ring::summitN(), ring::WAKE_S, {0.f, 0.f, ring::WAKE_S - 8.f});
        w.zone = LevelBuilder::aabb(-ring::WAKE_HALF, -1000.f, ring::summitN(), ring::WAKE_HALF, top + 30.f, ring::WAKE_S);   // (narrow: the notch in the wave)
        w.bounds = w.zone; w.bounds.min.y = 0.f;
        w.voidY = -30.f;
        w.hasTrigger = true; w.trigger = LevelBuilder::aabb(-ring::WAKE_HALF, top - 2.f, ring::summitN(), ring::WAKE_HALF, top + 20.f, ring::summitS() - 6.f);
        const float zs = ring::summitS(), zc = (ring::summitS() + ring::summitN()) * 0.5f;
        w.groundSpawns = {{-30.f, top, zc - 10.f}, {30.f, top, zc - 8.f}, {-14.f, top, zc - 24.f}, {16.f, top, zc - 26.f}};
        w.waves = {{W(EnemyType::BELLWETHER, 0, {vec3{0.f, top, zc - 14.f}}), W(EnemyType::STRAY, 3)}};
        (void)zs;
        w.maxAlive = 6;
        l.arenas.push_back(w);
    }
    // Last: whatever is drawn within reach and has no collision gets its own (nothing hollow to walk into)
    ringMatchCollision(b, ring::HALF_X + 6.f);
    return l;
}

// =============================================================================
// THE TEST GROUND (build step 2: `--ringtest`)
// =============================================================================
inline LevelData buildRingTest() {
    using glm::vec3;
    LevelData l; LevelBuilder b{l}; auto& s = l.story;
    s.built = true; s.chapter = 6; s.nextChapter = 0; s.title = "THE KEPT RING"; s.nextName = "TO BE CONTINUED";
    s.startWeapons = 15; s.startGrapple = true; s.bossSection = 99; s.pitSection = -1;
    const vec3 iceLit{0.9f, 0.88f, 0.82f}, sand{0.62f, 0.54f, 0.42f}, iceDark{0.3f, 0.29f, 0.27f}, steel{0.06f, 0.06f, 0.065f};
    for (float x = -ring::FAR_GROUND; x < ring::FAR_GROUND; ) {
        const bool nearX = std::abs(x) < 200.f;
        const float x1 = x + (nearX ? 40.f : 400.f);
        for (float z = -ring::FAR_GROUND; z < ring::FAR_GROUND; ) {
            const float z1 = z + ((nearX && std::abs(z) < 200.f) ? 40.f : 400.f);
            const float n = x * 0.37f + z * 0.11f;
            l.floors.push_back({x, z, x1, z1, 0.f, glm::mix(iceLit, sand, 0.25f + 0.5f * ring::hash(n, 11.f)), ring::hash(n, 12.f) > 0.8f ? Mat::ROCK : Mat::SNOW});
            z = z1;
        }
        x = x1;
    }
    int n = 0;
    for (const SweepShelter& sh : ring::testShelters()) {
        b.mat = Mat::ROCK;
        b.wall(sh.box.min.x, sh.box.min.y, sh.box.min.z, sh.box.max.x, sh.box.max.y, sh.box.max.z, glm::mix(iceLit, iceDark, 0.2f + 0.5f * ring::hash((float)n, 5.f)));
        b.mat = Mat::METAL;
        b.prop(sh.post.x - 0.09f, 0.f, sh.post.z - 0.09f, sh.post.x + 0.09f, 3.f, sh.post.z + 0.09f, steel);
        ++n;
    }
    l.sweepShelters = ring::testShelters();
    Arena a;
    a.name = "THE TEST GROUND"; a.subtitle = "GET BEHIND SOMETHING HEAVY";
    a.zone = LevelBuilder::aabb(-ring::TEST_HALF_X, -5.f, ring::TEST_NORTH, ring::TEST_HALF_X, 400.f, ring::TEST_SOUTH);
    a.bounds = a.zone; a.bounds.min.y = 0.f;
    a.playerStart = ring::testStart(); a.startYaw = -90.f;
    a.hasRespawn = true; a.respawn = ring::testStart();
    a.maxAlive = 8; a.damageScale = 1.f;
    a.theme = ring::theme(); a.ambient = Ambient::DUST; a.space = ReverbSpace::OPEN; a.tone = RoomTone::NONE;
    a.hasTrigger = true; a.trigger = LevelBuilder::aabb(-ring::TEST_HALF_X, -2.f, ring::TEST_NORTH, ring::TEST_HALF_X, 30.f, 34.f);
    a.groundSpawns = {{-20.f, 0.f, -70.f}, {22.f, 0.f, -66.f}, {-46.f, 0.f, -30.f}, {50.f, 0.f, -26.f}, {0.f, 0.f, -100.f}, {-56.f, 0.f, -78.f}, {58.f, 0.f, -60.f}};
    a.airSpawns = {{-20.f, 3.f, -70.f}, {22.f, 3.f, -66.f}};
    using W = WaveEntry;
    a.waves = {{W(EnemyType::STRAY, 5)}, {W(EnemyType::STRAY, 3), W(EnemyType::HERDER, 2), W(EnemyType::TALLY, 0, {{0.f, 0.f, -40.f}})},
               {W(EnemyType::STRAY, 4), W(EnemyType::PORTER, 1)}};
    l.arenas.push_back(a);
    return l;
}
