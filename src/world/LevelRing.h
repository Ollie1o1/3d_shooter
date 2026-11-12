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
#include <cstdio>

namespace ring {
constexpr float HALF_X = 110.f;                 // the playable width
constexpr float LAND_S = 135.f, LAND_N = -150.f;   // THE LANDING
constexpr float DRIFT_N = -560.f;               // the Drift's far shore (the reef plain starts here)
constexpr float DRIFT_MID = -345.f;             // its two halves meet at I2
constexpr float KEPT_N = -905.f;
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
inline bool inDrift(float z) { return z < LAND_N && z > DRIFT_N; }

// The distance card for each place (section 13.2: falling every card)
inline const char* distance(int arena) {
    switch (arena) {
        case 0: return "74,000 KM";
        case 1: case 2: return "61,000 KM";
        default: return "52,000 KM";
    }
}
// How big the Shepherd is in the sky (angular radius) as you go: a point at the Landing, growing
inline float shepherdSize(float z) {
    const float t = glm::clamp((LAND_S - z) / (LAND_S - KEPT_N), 0.f, 1.f);
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
struct Rock { float x, z, w, h, d; };
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
    out.push_back(shelterBox(quietHull().min.x, quietHull().min.z, quietHull().max.x, quietHull().min.z + 1.f, 0.f, 9.f, 4));
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
    if (up) b.solid(at.x - 1.f, at.y, at.z - 1.f, at.x + 1.f, at.y + 3.4f, at.z + 1.f);
    else if (quarter % 2 == 0) b.solid(at.x - 0.95f, at.y - 0.9f, at.z - 2.3f, at.x + 0.95f, at.y + 0.95f, at.z + 2.3f);
    else b.solid(at.x - 2.3f, at.y - 0.9f, at.z - 0.95f, at.x + 2.3f, at.y + 0.95f, at.z + 0.95f);
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
    auto plane = [&](float zLo, float zHi) {   // z from zLo to zHi (zLo < zHi)
        for (float x = -ring::FAR_GROUND; x < ring::FAR_GROUND; ) {
            const bool nearX = std::abs(x) < 240.f;
            const float x1 = x + (nearX ? 40.f : 400.f);
            for (float z = zLo; z < zHi; ) {
                const float step = (nearX && z > -950.f && z < 200.f) ? 40.f : 400.f;
                const float z1 = std::min(zHi, z + step);
                patch(x, z, x1, z1, x * 0.37f + z * 0.11f);
                z = z1;
            }
            x = x1;
        }
    };
    plane(ring::LAND_N, ring::FAR_GROUND);          // the Landing and everything south
    plane(-ring::FAR_GROUND, ring::DRIFT_N);         // the reef plain and everything north
    // Under the Drift: nothing. Its band runs across the whole ring
    l.basins.push_back({LevelBuilder::aabb(-ring::FAR_GROUND, 0.f, ring::DRIFT_N, ring::FAR_GROUND, 0.f, ring::LAND_N), ring::VOID_FLOOR});

    // The ragged lips where the floor ends: ice slabs leaning out over the band
    for (float x = -400.f; x <= 400.f; x += 9.f) {
        for (int side = 0; side < 2; ++side) {
            const float k = x * 0.1f + side * 50.f;
            const float z = side == 0 ? ring::LAND_N + 1.f - 3.f * ring::hash(k, 1.f) : ring::DRIFT_N - 1.f + 3.f * ring::hash(k, 1.f);
            b.mat = Mat::ROCK;
            b.kit().rock({x + 4.f * ring::hash(k, 2.f), -6.f, z}, 4.5f + 2.f * ring::hash(k, 3.f), 6.4f + 1.2f * ring::hash(k, 4.f), glm::mix(iceLit, iceDark, 0.5f), (uint32_t)(k * 13.f + 7.f), 0.8f, 7);
        }
    }

    int seed = 1;
    auto rockShelter = [&](const SweepShelter& sh) {
        b.solid(sh.box.min.x, sh.box.min.y, sh.box.min.z, sh.box.max.x, sh.box.max.y, sh.box.max.z);
        ringChunk(b, sh.box, glm::mix(iceLit, iceDark, 0.15f + 0.4f * ring::hash((float)seed, 5.f)), (uint32_t)(seed * 97 + 3));
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
        for (float x = -400.f; x <= 400.f; x += 9.f)
            for (float z = ring::LAND_N + 6.f; z <= 400.f; z += 9.f) {
                const float k = x * 0.71f + z * 0.13f;
                const float fx = x + 6.f * (ring::hash(k, 1.f) - 0.5f), fz = z + 6.f * (ring::hash(k, 2.f) - 0.5f);
                const bool reach = std::abs(fx) < ring::HALF_X + 4.f && fz < ring::LAND_S + 6.f;
                const float keep = reach ? 0.55f : std::abs(fx) > 250.f || fz > 260.f ? 0.18f : 0.32f;
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
                    l.movers[bm].look = 1;
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
            b.solid(x0, 0.f, z - ring::REEF_WALL * 0.5f, x1, ring::REEF_H, z + ring::REEF_WALL * 0.5f);
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
            b.solid(x - ring::REEF_WALL * 0.5f, 0.f, z0, x + ring::REEF_WALL * 0.5f, ring::REEF_H, z1);
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
            b.solid(st.x - st.hx, 0.f, st.z - st.hz, st.x + st.hx, st.h, st.z + st.hz);
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
        // The way on: north, toward the Shepherd
        s.end = LevelBuilder::aabb(-20.f, -1.f, ring::KEPT_N + 5.f, 20.f, 10.f, ring::KEPT_N + 25.f);
        b.mat = Mat::METAL;
        for (int k = 0; k < 6; ++k) post({-6.f + (k % 2) * 12.f, 0.f, ring::KEPT_N + 30.f + (k / 2) * 14.f});
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
        l.movers[m].look = 1; l.movers[m].color = glm::mix(iceLit, iceDark, 0.25f);
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
        k.waves = {{W(EnemyType::STRAY, 5)},
                   {W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 2), W(EnemyType::TALLY, 0, {tally})},
                   {W(EnemyType::PORTER, 0, {c + vec3{0.f, 0.f, -36.f}}), W(EnemyType::STRAY, 4), W(EnemyType::HERDER, 1)}};
        k.waveBreak = 3.f; k.maxAlive = 7;
        l.arenas.push_back(k);
    }
    // 4. The open hull, and out north
    Arena h = section("THE KEPT", "", ring::KEPT_N, -760.f, {0.f, 0.f, -770.f});
    l.arenas.push_back(h);
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
