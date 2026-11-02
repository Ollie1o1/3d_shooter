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

// The prologue's set: where the Successor fell, a broken white floor in the void, far under the plain
// (nothing up there can see it, and its fog hides the plain from it)
inline glm::vec3 prologueCentre() { return {0.f, -2000.f, 0.f}; }
inline Theme voidTheme() {
    Theme t{};
    t.zenith = {0.05f, 0.025f, 0.16f}; t.horizon = {0.22f, 0.06f, 0.25f}; t.ground = {0.03f, 0.02f, 0.06f};
    t.sunDir = glm::normalize(glm::vec3{-0.45f, 0.7f, -0.55f}); t.sunColor = {0.8f, 0.7f, 1.f}; t.sunSize = 0.02f;
    t.mountain = {0.02f, 0.01f, 0.04f}; t.stars = 0.9f;
    t.lightDir = glm::normalize(glm::vec3{0.35f, -0.8f, 0.3f}); t.lightColor = {0.9f, 0.85f, 1.05f};
    t.skyAmb = {0.34f, 0.3f, 0.45f}; t.groundAmb = {0.16f, 0.13f, 0.2f};
    t.fogColor = {0.09f, 0.07f, 0.19f}; t.fogDensity = 0.004f;
    return t;
}
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

    // ---- the prologue's set: the broken assessment floor where the Successor fell, the portal at its end ----
    {
        const vec3 C = plain::prologueCentre(), tile{0.71f, 0.75f, 0.72f}, grout{0.3f, 0.34f, 0.35f}, steel{0.19f, 0.29f, 0.33f};
        b.mat = Mat::CLINIC;
        for (int i = -4; i <= 4; ++i)
            for (int j = -3; j <= 11; ++j) {   // 2 m tiles, a few missing and a few tilted at the broken edges
                float h = plain::hash(i * 31.f + j, 301.f);
                bool edge = std::abs(i) == 4 || j == -3 || j == 11;
                if (edge && h < 0.45f) continue;
                vec3 p = C + vec3{i * 2.f, 0.f, j * 2.f};
                float drop = edge ? -0.4f * h : 0.f;
                b.prop(p.x - 0.97f, p.y - 0.5f + drop, p.z - 0.97f, p.x + 0.97f, p.y + drop, p.z + 0.97f, glm::mix(tile, tile * 0.85f, h));
            }
        b.prop(C.x - 9.f, C.y - 1.4f, C.z - 7.f, C.x + 9.f, C.y - 0.5f, C.z + 23.f, grout);   // the slab under the tiles
        // drifting fragments of the ward around it
        for (int k = 0; k < 16; ++k) {
            float a = k * 0.39f, d = 16.f + 14.f * plain::hash(k, 302.f);
            vec3 p = C + vec3{std::cos(a) * d, -3.f + 9.f * plain::hash(k, 303.f), 8.f + std::sin(a) * d};
            b.kit().box(p, {1.f + 3.f * plain::hash(k, 304.f), 0.4f, 1.f + 2.f * plain::hash(k, 305.f)}, tile * 0.9f, a, 0.3f * plain::hash(k, 306.f), 0.2f);
        }
        // the portal's frame: two cabled pylons and a ring
        b.mat = Mat::METAL;
        for (float s : {-1.f, 1.f}) b.prop(C.x + s * 5.6f - 0.4f, C.y, C.z + 17.1f, C.x + s * 5.6f + 0.4f, C.y + 10.f, C.z + 17.9f, steel);
        for (int k = 0; k < 28; ++k) {   // the ring, standing upright facing the floor
            float a = k * 0.2244f;
            b.kit().box(C + vec3{std::cos(a) * 4.9f, 4.6f + std::sin(a) * 4.9f, 17.5f}, {0.55f, 1.18f, 0.7f}, steel, 0.f, 0.f, a);
        }
    }

    // ---- the arena ----
    Arena a;
    a.name = "THE BLOOD MOON PLAIN"; a.subtitle = "SOMETHING IS WATCHING";
    a.zone = LevelBuilder::aabb(-plain::HALF, -60.f, -plain::HALF, plain::HALF, 600.f, plain::HALF);
    a.bounds = a.zone; a.bounds.min.y = 0.f;
    a.playerStart = plain::start(); a.startYaw = -90.f;
    a.voidY = -50.f; a.hasRespawn = true; a.respawn = plain::start();
    a.hasTrigger = false; a.scripted = true;
    a.maxAlive = 8; a.damageScale = 1.f;
    a.theme = plain::theme();
    a.ambient = Ambient::EMBERS; a.space = ReverbSpace::OPEN; a.tone = RoomTone::YARD;
    l.arenas.push_back(a);
    return l;
}
