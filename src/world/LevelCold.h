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

namespace cold {
constexpr float HALF_X = 260.f;               // the playable shelf: -HALF_X..HALF_X in X
constexpr float SOUTH = 430.f;                //   ...and from its north edge (edgeZ) to here in Z
constexpr float EDGE_Z = -320.f;              // the shelf's north edge, on average (it is ragged)
constexpr float STRIP = 40.f;                 // the edge is built in strips this wide, each its own depth
constexpr float FAR_GROUND = 3400.f;          // the shelf and the land round it keep going this far (visual)
constexpr float FAR_PLANE = 5200.f;           // the camera sees this far here (Gameplay_Cold.h)
constexpr float VOID_FLOOR = -900.f;          // the Void's "floor": far below the respawn line
constexpr float STORE_Z = -1250.f;            // the glacier cliff the Store is cut into
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
        float z = STORE_Z + 120.f + hash(k, 3.f) * 260.f;
        float half = 35.f + 40.f * hash(k, 4.f);
        float h = 1250.f + 900.f * hash(k, 5.f);
        if (std::abs(x) < 260.f) h += 500.f;   // the tallest straight ahead of the shelf
        t.push_back({{x, VOID_FLOOR, z}, half, h});
    }
    return t;
}
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
        // The Void beyond its lip: no floor, a long way down
        l.basins.push_back({LevelBuilder::aabb(x, 0.f, -cold::FAR_GROUND, x1, 0.f, ez), cold::VOID_FLOOR});
        // The lip's face: ice going down into the dark, banded deeper blue as it goes
        b.mat = Mat::ICE;
        b.kit().box({x + cold::STRIP * 0.5f, -30.f, ez - 2.f}, {cold::STRIP + 0.4f, 60.f, 4.f}, ice);
        b.kit().box({x + cold::STRIP * 0.5f, -140.f, ez - 6.f + 4.f * cold::hash(n, 13.f)}, {cold::STRIP + 0.4f, 160.f, 4.f}, iceDeep);
        b.kit().box({x + cold::STRIP * 0.5f, -360.f, ez - 10.f}, {cold::STRIP + 0.4f, 280.f, 4.f}, iceDeep * 0.5f);
        // ...and the side where the next strip's lip is further out
        float nz = cold::edgeZ(x1);
        if (x1 < cold::FAR_GROUND && std::abs(nz - ez) > 0.5f) {
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
    const float gantryX[] = {-150.f, -40.f, 95.f, 210.f, -520.f, 640.f, -980.f, 1150.f};
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

    // ---- the arena: the shelf (it runs a little past the lip, so you can walk off it) ----
    Arena a;
    a.name = "THE ICE SHELF"; a.subtitle = "NOTHING HERE IS THROWN AWAY";
    a.zone = LevelBuilder::aabb(-cold::HALF_X, -1000.f, cold::EDGE_Z - 70.f, cold::HALF_X, 800.f, cold::SOUTH);
    a.bounds = a.zone; a.bounds.min.y = 0.f;
    a.playerStart = cold::start(); a.startYaw = -90.f;
    a.voidY = -60.f; a.hasRespawn = true; a.respawn = cold::start();
    a.hasTrigger = false; a.scripted = true;
    a.maxAlive = 8; a.damageScale = 1.f;
    a.theme = cold::theme();
    a.ambient = Ambient::WIND; a.space = ReverbSpace::OPEN; a.tone = RoomTone::YARD;
    l.arenas.push_back(a);
    return l;
}
