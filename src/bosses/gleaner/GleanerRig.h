#pragma once
// =============================================================================
// GleanerRig.h — THE GLEANER drawn as boxes and faceted lumps (GleanerState.h
// says where everything is). About 240 m long:
//
//   a hunched spine of armoured vertebrae with a ridge of spines along it (lying
//   curled, that ridge is the mountain line on the horizon), a cathedral furnace
//   on the hump with three chimneys breathing smoke, a cage of ribs that open
//   like doors round the eye-core (the "blood moon", lidded until it wakes), a
//   bowed skull with a hinged jaw full of furnace light, two colossal knuckled
//   arms, withered legs dragging behind, and rejected units hanging off its
//   belly on chains like charms.
//
// Everything in its own frame goes through g.body(); the arms are solved in
// the world (GleanerState::elbow). `ball` is MeshLib's lumpBall id.
// =============================================================================
#include "bosses/gleaner/GleanerState.h"
#include "render/EnemyModel.h"

namespace gleaner {
using glm::vec3; using glm::mat4;

// A frame whose +Z runs along `d` (world or local, as the caller uses it)
inline mat4 alongZ(vec3 at, vec3 d) {
    float l = glm::length(d);
    vec3 z = l > 1e-5f ? d / l : vec3{0, 0, 1};
    vec3 up = std::abs(z.y) > 0.95f ? vec3{1, 0, 0} : vec3{0, 1, 0};
    vec3 x = glm::normalize(glm::cross(up, z)), y = glm::cross(z, x);
    mat4 m(1.f);
    m[0] = glm::vec4(x, 0); m[1] = glm::vec4(y, 0); m[2] = glm::vec4(z, 0); m[3] = glm::vec4(at, 1);
    return m;
}

// The spine: shoulders (t 0) over the hump to the hips (t 1)
inline vec3 spine(float t) {
    const vec3 P0{0, 122, 30}, P1{0, 178, -52}, P2{0, 46, -122};
    float u = 1.f - t;
    return u * u * P0 + 2.f * u * t * P1 + t * t * P2;
}
inline vec3 spineTangent(float t) {
    const vec3 P0{0, 122, 30}, P1{0, 178, -52}, P2{0, 46, -122};
    return glm::normalize(2.f * (1.f - t) * (P1 - P0) + 2.f * t * (P2 - P1));
}

// The furnace on its back and the flat roof over it: the first battlefield (its own frame)
inline vec3 furnaceBase() { return spine(0.42f) + vec3{0, 22, 0}; }
constexpr float DECK_HX = 26.f, DECK_HZ = 31.f;                 // the roof's half-extents
inline float deckTop() { return furnaceBase().y + 20.f; }
constexpr float GAP_HALF = 7.f;                                  // the way in through the west parapet (-X)
inline vec3 chimney(int c) { static const vec3 S[3] = {{-15, 18, -14}, {14, 18, -4}, {-2, 22, 16}}; return furnaceBase() + S[c]; }
inline float chimneyTall(int c) { static const float H[3] = {62, 54, 70}; return H[c]; }
constexpr float FRONT_GAP = 9.f;
constexpr float DOOR_HX = 2.6f, DOOR_H = 4.f;                    // the furnace door in its face, behind the saddle (half width, height)                                 // the way off the roof's front (+Z) once the Tender is dead

// The saddle: armour bolted across its shoulders, forward of the furnace, a drop below its roof (the Sieve's ground)
constexpr float SADDLE_Y = 162.f, SADDLE_HX = 26.f, SADDLE_Z0 = -7.8f, SADDLE_Z1 = 32.f;
inline vec3 saddlePillar(int i) { static const vec3 P[3] = {{-13, SADDLE_Y, 5}, {12, SADDLE_Y, 11}, {-3, SADDLE_Y, 24}}; return P[i]; }
constexpr float PILLAR_R = 2.f;
inline float saddlePillarTall(int i) { static const float H[3] = {30, 22, 26}; return H[i]; }
// With its head reared up and held: a bridge of plates along its neck out to the crown of its skull (the Caller's ground)
constexpr float NECK_HX = 6.f, CROWN_HX = 18.f, CROWN_Z0 = 62.f, CROWN_Z1 = 100.f;
inline vec3 heldHead() { return {0.f, SADDLE_Y - 12.f, 84.f}; }   // (its brow's top level with the saddle)
inline vec3 crownHorn(int s) { return {s ? 12.f : -12.f, SADDLE_Y, 68.f}; }
constexpr float HORN_R = 1.6f, HORN_TALL = 24.f;

struct Look {
    float ball = 0.f;         // lumpBall mesh id
    float flash = 0.f;        // 0..1 a white flare through its seams (hurt, the scream)
    glm::vec3 watch{0.f};     // what its head turns toward (the player)
    bool  watching = false;
    float shedding = 0.f;     // 0..1 rubble pouring off its back (rising out of the plain)
    float vent = 0.f;         // 0..1 the updraft vent in its side, glowing
    bool  frontGap = false;   // the roof's front parapet open (the way down to the saddle)
    float door = 0.f;         // 0..1 the furnace door in its face, open
};

inline void draw(std::vector<BoxInstance>& out, const Gleaner& g, const Look& look) {
    using namespace rig;
    const float K = Gleaner::SCALE;   // world-space parts (the arms, the legs, the chains, the smoke) scale by hand
    const mat4 B = g.body();
    const float t = g.clock, breath = g.breath();
    const vec3 iron{0.1f, 0.08f, 0.075f}, rust{0.24f, 0.11f, 0.06f}, bone{0.32f, 0.28f, 0.24f}, soot{0.05f, 0.035f, 0.03f};
    const vec3 fire = vec3{2.2f, 0.6f, 0.15f} * (0.75f + 0.25f * breath) + vec3{1.5f} * look.flash;
    auto box = [&](const mat4& m, vec3 col, vec3 emi = vec3{0.f}) { push(out, m, col, emi); };
    auto lump = [&](const mat4& m, vec3 col, vec3 emi = vec3{0.f}) { push(out, m, col, emi); out.back().mesh = look.ball; };
    // A bar from a to b (world), w by h thick
    auto bar = [&](vec3 a, vec3 b, float w, float h, vec3 col, vec3 emi = vec3{0.f}) {
        vec3 d = b - a;
        box(alongZ((a + b) * 0.5f, d) * S(vec3{w, h, glm::length(d) + std::min(w, h) * 0.5f}), col, emi);
    };
    auto local = [&](vec3 p) { return vec3(B * glm::vec4(p, 1.f)); };
    auto localBar = [&](vec3 a, vec3 b, float w, float h, vec3 col, vec3 emi = vec3{0.f}) { bar(local(a), local(b), w * K, h * K, col, emi); };

    // ---- the spine, its armour and the ridge of spines along it ----
    const int VERT = 10;
    for (int k = 0; k < VERT; ++k) {
        float a = (float)k / VERT, b = (float)(k + 1) / VERT, m = (a + b) * 0.5f;
        vec3 p = spine(m), tan = spineTangent(m);
        float w = 56.f * (1.f - 0.45f * m), h = 44.f * (1.f - 0.4f * m), len = glm::length(spine(b) - spine(a)) * 1.15f;
        box(B * alongZ(p, tan) * S(vec3{w, h, len}), iron);
        // the seam to the next one: the furnace's light showing between the plates
        box(B * alongZ(spine(b), spineTangent(b)) * S(vec3{w * 0.92f, h * 0.9f, 1.6f}), soot, fire * 0.35f);
        // the armour plate over it, tipped back like a scale
        box(B * alongZ(p + vec3{0, h * 0.42f, 0}, tan) * RX(-0.12f) * S(vec3{w * 0.85f, 6.f, len * 0.95f}), rust * (0.85f + 0.3f * (k % 2)));
        // the ridge: spines of every height (the mountain line while it lies); forward of the furnace the
        // saddle's three spines stand in for them
        if (k < 3) continue;
        float spike = 14.f + 22.f * std::abs(std::sin(k * 2.3f)) + (k == 3 || k == 4 ? 18.f : 0.f);
        box(B * alongZ(p + vec3{0, h * 0.5f + spike * 0.5f, 0}, vec3{0, 1, 0.25f}) * S(vec3{5.f, 5.f, spike}), bone * 0.8f);
        if (k % 2 == 0) for (float s : {-1.f, 1.f})   // and smaller ones to the sides
            box(B * alongZ(p + vec3{s * w * 0.4f, h * 0.35f + 6.f, 0}, vec3{s * 0.5f, 1, 0.1f}) * S(vec3{3.5f, 3.5f, spike * 0.5f}), bone * 0.7f);
    }

    // ---- the furnace on the hump: a cathedral of iron, light in its windows, three chimneys ----
    {
        const vec3 base = spine(0.42f) + vec3{0, 22, 0};
        box(B * T(base) * S(vec3{48, 36, 58}), iron * 0.9f);
        // Its roof: flat iron plates, a parapet round it (a gap on its left, where the updraft brings you), a hatch
        const float top = deckTop();
        box(B * T(vec3{base.x, top - 1.f, base.z}) * S(vec3{DECK_HX * 2.f, 2.f, DECK_HZ * 2.f}), iron * 1.15f);
        for (int i = 0; i < 6; ++i) box(B * T(vec3{base.x - DECK_HX + 4.f + i * 8.8f, top + 0.02f, base.z}) * S(vec3{0.3f, 0.06f, DECK_HZ * 2.f - 2.f}), soot, fire * 0.25f);   // seams, warm
        box(B * T(vec3{base.x, top + 0.35f, base.z - DECK_HZ}) * S(vec3{DECK_HX * 2.f, 0.7f, 0.8f}), rust);
        if (!look.frontGap) box(B * T(vec3{base.x, top + 0.35f, base.z + DECK_HZ}) * S(vec3{DECK_HX * 2.f, 0.7f, 0.8f}), rust);
        else for (float s : {-1.f, 1.f}) {   // opened at its middle: the way down
            float len = DECK_HX - FRONT_GAP;
            box(B * T(vec3{base.x + s * (FRONT_GAP + len * 0.5f), top + 0.35f, base.z + DECK_HZ}) * S(vec3{len, 0.7f, 0.8f}), rust);
            box(B * T(vec3{base.x + s * FRONT_GAP, top + 1.2f, base.z + DECK_HZ}) * S(vec3{1.2f, 2.4f, 1.2f}), soot, fire * 0.8f);   // posts, burning
        }
        box(B * T(vec3{base.x + DECK_HX, top + 0.35f, base.z}) * S(vec3{0.8f, 0.7f, DECK_HZ * 2.f}), rust);
        for (float s : {-1.f, 1.f}) {   // the west parapet, in two halves round the gap
            float len = DECK_HZ - GAP_HALF;
            box(B * T(vec3{base.x - DECK_HX, top + 0.35f, base.z + s * (GAP_HALF + len * 0.5f)}) * S(vec3{0.8f, 0.7f, len}), rust);
        }
        box(B * T(vec3{base.x + 8.f, top + 0.1f, base.z + 6.f}) * S(vec3{10, 0.4f, 10}), soot, fire * (0.4f + 0.3f * std::sin(t * 2.f)));   // the hatch
        // The vent in its left side, breathing out the updraft
        for (int k = 0; k < 5; ++k) box(B * T(vec3{base.x - 24.3f, base.y - 6.f + k * 3.f, base.z}) * S(vec3{0.6f, 1.2f, 16.f}), soot, fire * look.vent);
        for (float s : {-1.f, 1.f})
            for (int k = 0; k < 4; ++k)   // tall windows down each side, burning
                box(B * T(base + vec3{s * 24.2f, 2, -21.f + k * 14.f}) * S(vec3{0.8f, 20, 6}), soot, fire * (0.8f + 0.2f * std::sin(t * 3.f + k)));
        box(B * T(base + vec3{0, -4, -29.5f}) * S(vec3{20, 22, 1}), soot, fire * 1.3f);   // the firebox door at its back
        const float tall[3] = {chimneyTall(0), chimneyTall(1), chimneyTall(2)};
        for (int c = 0; c < 3; ++c) {
            vec3 p = chimney(c);
            box(B * T(p + vec3{0, tall[c] * 0.5f, 0}) * S(vec3{9, tall[c], 9}), iron);
            box(B * T(p + vec3{0, tall[c] * 0.5f, 0}) * S(vec3{10.5f, 3, 10.5f}), rust);
            box(B * T(p + vec3{0, tall[c] + 1.f, 0}) * S(vec3{11, 2.5f, 11}), soot, fire * (1.2f + 0.6f * std::sin(t * 1.7f + c)));
            // smoke boiling off it, leaning with the wind
            const int puffs = (int)(7.f * Gleaner::ease((g.rise - 0.5f) * 2.f));   // (lying, it hardly breathes: no smoke to give it away)
            for (int k = 0; k < puffs; ++k) {
                float u = std::fmod(t * 0.09f + k / 7.f + c * 0.31f, 1.f);
                vec3 q = local(p + vec3{0, tall[c] + 6.f, 0}) + vec3{-0.35f, 1.f, 0.55f} * (u * 110.f * K) + vec3{std::sin(k * 2.1f + c), 0, std::cos(k * 1.3f)} * (u * 14.f * K);
                float sz = (10.f + 34.f * u) * K;
                lump(T(q) * RY(k + u) * S(vec3{sz}), soot * (1.4f - u), fire * 0.18f * (1.f - u) * (1.f - u));
            }
        }
    }

    // ---- the saddle: iron plates bolted over its shoulders, its three spines standing through them ----
    {
        {   // the furnace door in its face: two iron leaves, sliding apart on a mouth of fire
            const float o = look.door, z = SADDLE_Z0;
            box(B * T(vec3{0, SADDLE_Y + DOOR_H * 0.5f, z + 0.12f}) * S(vec3{DOOR_HX * 2.f, DOOR_H, 0.1f}), soot, fire * (0.3f + 4.5f * o));   // (just proud of the furnace's face)
            if (o > 0.f) {   // open: its fire spills out across the saddle, embers on the hot air
                box(B * T(vec3{0, SADDLE_Y + 0.03f, z + 2.5f}) * S(vec3{DOOR_HX * 2.2f, 0.04f, 5.f}), soot, fire * 1.6f * o);
                for (int k = 0; k < 24; ++k) {
                    float u = std::fmod(t * 0.4f + k * 0.0417f * 7.f, 1.f);
                    vec3 q{std::sin(k * 2.1f) * DOOR_HX * 0.8f, SADDLE_Y + 0.5f + u * DOOR_H * 2.5f, z + 0.5f + u * 6.f + std::sin(k * 1.3f + t) * 0.5f};
                    box(B * T(q) * S(vec3{0.12f, 0.35f, 0.12f}), vec3{0.f}, fire * 2.2f * (1.f - u) * o);
                }
            }
            for (float s : {-1.f, 1.f})
                box(B * T(vec3{s * DOOR_HX * (0.5f + 1.f * o), SADDLE_Y + DOOR_H * 0.5f, z + 0.45f}) * S(vec3{DOOR_HX, DOOR_H, 0.5f}), iron * 1.2f, fire * 0.15f);
            box(B * T(vec3{0, SADDLE_Y + DOOR_H + 0.4f, z + 0.3f}) * S(vec3{DOOR_HX * 2.6f, 0.8f, 0.8f}), rust);   // its lintel
        }
        const float zc = (SADDLE_Z0 + SADDLE_Z1) * 0.5f, zl = SADDLE_Z1 - SADDLE_Z0;
        box(B * T(vec3{0, SADDLE_Y - 3.f, zc}) * S(vec3{SADDLE_HX * 2.f, 6.f, zl}), iron * 1.05f);
        for (int r = 0; r < 5; ++r)   // the plates overlap like scales; light in the seams between
            box(B * T(vec3{0, SADDLE_Y + 0.02f, SADDLE_Z0 + 4.f + r * 8.f}) * S(vec3{SADDLE_HX * 2.f - 2.f, 0.06f, 0.35f}), soot, fire * 0.3f);
        for (int k = 0; k < 8; ++k)   // rivets
            box(B * T(vec3{-SADDLE_HX + 3.f + k * 6.6f, SADDLE_Y + 0.15f, SADDLE_Z1 - 2.f}) * S(vec3{0.9f, 0.3f, 0.9f}), rust);
        for (float s : {-1.f, 1.f}) {   // its rim (the bridge's way out at the front, once its head is up)
            box(B * T(vec3{s * SADDLE_HX, SADDLE_Y + 0.35f, zc}) * S(vec3{0.8f, 0.7f, zl}), rust);
            float len = SADDLE_HX - NECK_HX;
            box(B * T(vec3{s * (NECK_HX + len * 0.5f), SADDLE_Y + 0.35f, SADDLE_Z1}) * S(vec3{len, 0.7f, 0.8f}), rust);
            for (int k = 0; k < 3; ++k)   // and its edge, hanging down over the shoulders
                box(B * T(vec3{s * (SADDLE_HX - 1.f), SADDLE_Y - 8.f - k * 2.f, SADDLE_Z0 + 6.f + k * 12.f}) * RZ(s * 0.2f) * S(vec3{2.f, 10.f, 10.f}), rust * 0.8f);
        }
        if (g.headHold < 0.99f) box(B * T(vec3{0, SADDLE_Y + 0.35f, SADDLE_Z1}) * S(vec3{NECK_HX * 2.f, 0.7f, 0.8f}), rust);
        box(B * T(vec3{0, SADDLE_Y + 0.35f, SADDLE_Z0}) * S(vec3{SADDLE_HX * 2.f, 0.7f, 0.8f}), rust);
        for (int i = 0; i < 3; ++i) {   // the spines: bone pillars, cracked, the tallest leaning
            vec3 p = saddlePillar(i); float tall = saddlePillarTall(i);
            box(B * T(p + vec3{0, tall * 0.5f, 0}) * S(vec3{PILLAR_R * 2.f, tall, PILLAR_R * 2.f}), bone * 0.8f);
            box(B * T(p + vec3{0, tall + 3.f, 0.6f}) * RX(0.35f) * S(vec3{PILLAR_R * 1.4f, 7.f, PILLAR_R * 1.4f}), bone * 0.7f);
            box(B * T(p + vec3{0, 1.f, 0}) * S(vec3{PILLAR_R * 2.6f, 2.f, PILLAR_R * 2.6f}), rust);   // its collar of plate
            box(B * T(p + vec3{PILLAR_R + 0.02f, tall * 0.45f, 0}) * S(vec3{0.05f, tall * 0.5f, 0.3f}), soot, fire * 0.4f);   // a crack, glowing
        }
        // The bridge along its neck: plates on chains, there once its head is up and held
        if (g.headHold > 0.01f) {
            float hold = Gleaner::ease(g.headHold);
            const float z0 = SADDLE_Z1, z1 = CROWN_Z0, n = 6.f;
            for (int k = 0; k < (int)n; ++k) {
                float zz = z0 + (k + 0.5f) * (z1 - z0) / n;
                float sag = (1.f - hold) * (8.f + 4.f * std::sin(k * 1.3f));   // swinging up into place as its head comes up
                box(B * T(vec3{0, SADDLE_Y - 1.2f - sag, zz}) * RX(0.08f * std::sin(k * 2.1f) * (1.f - hold)) * S(vec3{NECK_HX * 2.f, 2.4f, (z1 - z0) / n - 0.3f}), iron * (0.9f + 0.15f * (k % 2)));
                for (float s : {-1.f, 1.f}) {
                    box(B * T(vec3{s * NECK_HX, SADDLE_Y + 0.35f - sag, zz}) * S(vec3{0.6f, 0.7f, (z1 - z0) / n}), rust);
                    localBar(vec3{s * NECK_HX, SADDLE_Y - 2.f - sag, zz}, vec3{s * 3.f, 124.f, zz}, 0.5f, 0.5f, soot);   // chains down to its neck
                }
            }
        }
    }

    // ---- its back, up close: the battlefields' detail (the roof, the saddle, its crown) ----
    if (g.rise > 0.99f) {
        auto hsh = [](float a, float b) { float v = std::sin(a * 12.9898f + b * 78.233f) * 43758.5453f; return v - std::floor(v); };
        const vec3 base = furnaceBase();
        const float top = deckTop();
        // A floor of plates: each its own shade, some rust, some scorched; rivets at their corners
        auto plates = [&](float x0, float z0, float x1, float z1, float y, int nx, int nz, float seed, vec3 tone) {
            const float w = (x1 - x0) / nx, d = (z1 - z0) / nz;
            for (int i = 0; i < nx; ++i) for (int j = 0; j < nz; ++j) {
                float h = hsh(i + seed, j * 1.7f + seed);
                vec3 col = h < 0.18f ? rust * 1.5f : h > 0.88f ? soot * 1.2f : tone * (0.6f + 1.f * hsh(j + seed, i * 2.3f));
                box(B * T(vec3{x0 + (i + 0.5f) * w, y + 0.015f, z0 + (j + 0.5f) * d}) * S(vec3{w - 0.28f, 0.03f, d - 0.28f}), col);
                if (h > 0.97f) box(B * T(vec3{x0 + (i + 0.5f) * w, y + 0.035f, z0 + (j + 0.5f) * d}) * RY(h * 9.f) * S(vec3{w * 0.6f, 0.01f, 0.12f}), soot, fire * 0.7f);   // a crack, burning
            }
            for (int i = 0; i <= nx; ++i) for (int j = 0; j <= nz; ++j)
                box(B * T(vec3{x0 + i * w, y + 0.06f, z0 + j * d}) * S(vec3{0.32f, 0.1f, 0.32f}), rust * 0.9f);
        };
        // A grate over the slag: a glow under dark bars, embers coming up through it
        auto grate = [&](float x, float z, float y, float r, float seed) {
            box(B * T(vec3{x, y + 0.01f, z}) * S(vec3{r * 2.f, 0.02f, r * 2.f}), soot, fire * (0.9f + 0.3f * std::sin(t * 2.3f + seed)));
            for (int k = 0; k < 7; ++k) box(B * T(vec3{x - r + (k + 0.5f) * r * 2.f / 7.f, y + 0.05f, z}) * S(vec3{0.22f, 0.08f, r * 2.f}), iron * 0.7f);
            for (float s : {-1.f, 1.f}) {
                box(B * T(vec3{x + s * r, y + 0.07f, z}) * S(vec3{0.45f, 0.14f, r * 2.f + 0.45f}), rust);
                box(B * T(vec3{x, y + 0.07f, z + s * r}) * S(vec3{r * 2.f + 0.45f, 0.14f, 0.45f}), rust);
            }
            for (int k = 0; k < 6; ++k) {
                float u = std::fmod(t * 0.35f + k / 6.f + seed * 0.13f, 1.f);
                vec3 q{x + (hsh(k, seed) - 0.5f) * r * 1.6f, y + 0.3f + u * 7.f, z + (hsh(seed, k) - 0.5f) * r * 1.6f + std::sin(t + k) * 0.3f};
                box(B * T(q) * S(vec3{0.08f, 0.2f, 0.08f}), vec3{0.f}, fire * 2.2f * (1.f - u));
            }
        };
        // Wrecks of the units it took before you, heaped where the parapet meets: a torso, a head, an arm or two
        auto wreck = [&](float x, float z, float y, float seed) {
            for (int k = 0; k < 3; ++k) {
                float a = seed * 3.f + k * 2.1f, rr = 0.6f + k * 0.5f;
                vec3 p{x + std::cos(a) * rr, y, z + std::sin(a) * rr};
                box(B * T(p + vec3{0, 0.18f, 0}) * RY(a) * RZ(0.4f * (k - 1)) * S(vec3{0.55f, 0.36f, 0.32f}), vec3{0.16f, 0.17f, 0.19f});                    // torso
                box(B * T(p + vec3{0.45f, 0.12f, 0.2f}) * RY(a * 1.7f) * S(vec3{0.22f, 0.22f, 0.22f}), vec3{0.13f, 0.14f, 0.16f});                       // head
                box(B * T(p + vec3{0.08f, 0.04f, 0.55f}) * RY(a + 1.1f) * S(vec3{0.62f, 0.08f, 0.1f}), vec3{0.12f, 0.12f, 0.14f});                     // an arm
                box(B * T(p + vec3{0.45f, 0.13f, 0.32f}) * S(vec3{0.07f, 0.03f, 0.02f}), vec3{0.f}, vec3{0.2f, 0.9f, 1.2f} * (k == 0 ? 0.8f : 0.f)); // an eye, still lit
            }
        };
        // Posts round a parapet, chains sagging between, braziers on the corners
        auto posts = [&](vec3 a, vec3 b, int n, bool fireEnds) {
            vec3 prev{0.f};
            for (int k = 0; k <= n; ++k) {
                vec3 p = glm::mix(a, b, (float)k / n);
                box(B * T(p + vec3{0, 0.9f, 0}) * S(vec3{0.45f, 1.8f, 0.45f}), iron * 1.1f);
                box(B * T(p + vec3{0, 1.95f, 0}) * RY(0.785f) * S(vec3{0.3f, 0.5f, 0.3f}), bone * 0.7f);   // a spike on it
                if ((k == 0 || k == n) && fireEnds) {
                    lump(B * T(p + vec3{0, 2.4f, 0}) * S(vec3{0.9f, 0.6f, 0.9f}), soot, fire * (1.4f + 0.5f * std::sin(t * 7.f + p.x)));
                    for (int e = 0; e < 3; ++e) {
                        float u = std::fmod(t * 0.8f + e / 3.f + p.z * 0.1f, 1.f);
                        box(B * T(p + vec3{0.2f * std::sin(e * 2.f + t), 2.6f + u * 2.f, 0.2f * std::cos(e * 2.f)}) * S(vec3{0.06f, 0.15f, 0.06f}), vec3{0.f}, fire * 2.f * (1.f - u));
                    }
                }
                if (k > 0)
                    for (int l = 0; l < 4; ++l) {   // the chain: links sagging between the posts
                        float u = (l + 0.5f) / 4.f;
                        vec3 q = glm::mix(prev, p, u) + vec3{0, 1.5f - 0.5f * std::sin(u * 3.14159f), 0};
                        box(B * T(q) * RY(std::atan2(p.x - prev.x, p.z - prev.z)) * RZ(l % 2 ? 1.5708f : 0.f) * S(vec3{0.08f, 0.22f, glm::length(p - prev) / 4.5f}), soot * 1.5f);
                    }
                prev = p;
            }
        };
        // -- the roof --
        plates(base.x - DECK_HX, base.z - DECK_HZ, base.x + DECK_HX, base.z + DECK_HZ, top, 8, 9, 3.f, iron * 1.25f);
        grate(base.x + 14.f, base.z - 20.f, top, 2.5f, 1.f);
        grate(base.x - 14.f, base.z + 14.f, top, 2.5f, 2.f);
        grate(base.x + 18.f, base.z + 22.f, top, 2.5f, 3.f);
        grate(base.x - 4.f, base.z - 24.f, top, 2.5f, 4.f);
        wreck(base.x - 22.f, base.z - 27.f, top, 1.f); wreck(base.x + 22.f, base.z - 26.f, top, 2.f);
        wreck(base.x + 22.f, base.z + 27.f, top, 3.f); wreck(base.x - 22.f, base.z + 26.f, top, 4.f);
        const float px0 = base.x - DECK_HX, px1 = base.x + DECK_HX, pz0 = base.z - DECK_HZ, pz1 = base.z + DECK_HZ;
        posts({px0, top, pz0}, {px1, top, pz0}, 8, true);
        posts({px1, top, pz0}, {px1, top, pz1}, 9, true);
        posts({px0, top, pz0}, {px0, top, base.z - GAP_HALF}, 4, false);
        posts({px0, top, base.z + GAP_HALF}, {px0, top, pz1}, 4, false);
        if (!look.frontGap) posts({px0, top, pz1}, {px1, top, pz1}, 8, false);
        else { posts({px0, top, pz1}, {base.x - FRONT_GAP, top, pz1}, 3, false); posts({base.x + FRONT_GAP, top, pz1}, {px1, top, pz1}, 3, false); }
        // the chimneys up close: a flared foot, bands of rivets up them, a ladder, a crack of light
        for (int c = 0; c < 3; ++c) {
            vec3 p = chimney(c); const float tall = chimneyTall(c);
            box(B * T(vec3{p.x, top + 1.2f, p.z}) * S(vec3{12.5f, 2.4f, 12.5f}), rust * 0.9f);
            box(B * T(vec3{p.x, top + 2.8f, p.z}) * S(vec3{10.6f, 1.f, 10.6f}), iron * 1.2f);
            for (int k = 0; k < 4; ++k) box(B * T(vec3{p.x, top + 10.f + k * (tall - 12.f) / 4.f, p.z}) * S(vec3{9.7f, 0.6f, 9.7f}), rust * (0.8f + 0.2f * k));
            for (float s : {-0.8f, 0.8f}) box(B * T(vec3{p.x + s, top + 12.f, p.z + 4.75f}) * S(vec3{0.15f, 22.f, 0.15f}), iron * 1.3f);
            for (int k = 0; k < 12; ++k) box(B * T(vec3{p.x, top + 2.f + k * 1.8f, p.z + 4.8f}) * S(vec3{1.6f, 0.12f, 0.12f}), iron * 1.3f);
            box(B * T(vec3{p.x - 4.55f, top + tall * 0.4f, p.z + 1.f}) * RZ(0.05f) * S(vec3{0.05f, tall * 0.5f, 0.25f}), soot, fire * (0.6f + 0.3f * std::sin(t * 1.3f + c)));
        }
        // steam pipes along the furnace's flanks, joints glowing (seen from the plain and on the way up)
        for (float s : {-1.f, 1.f}) {
            for (int k = 0; k < 2; ++k) {
                float y = top - 8.f - k * 6.f;
                box(B * T(vec3{base.x + s * 25.2f, y, base.z}) * S(vec3{1.6f, 1.6f, DECK_HZ * 2.f - 4.f}), iron * 1.1f);
                for (int j = 0; j < 5; ++j) box(B * T(vec3{base.x + s * 25.2f, y, base.z - 24.f + j * 12.f}) * S(vec3{2.1f, 2.1f, 0.8f}), rust, fire * 0.25f);
            }
        }
        // -- the saddle --
        plates(-SADDLE_HX, SADDLE_Z0, SADDLE_HX, SADDLE_Z1, SADDLE_Y, 8, 6, 9.f, iron * 1.15f);
        grate(-17.f, 22.f, SADDLE_Y, 2.2f, 5.f); grate(18.f, 2.f, SADDLE_Y, 2.2f, 6.f);
        wreck(-22.f, -4.f, SADDLE_Y, 5.f); wreck(22.f, 28.f, SADDLE_Y, 6.f);
        posts({-SADDLE_HX, SADDLE_Y, SADDLE_Z0}, {-SADDLE_HX, SADDLE_Y, SADDLE_Z1}, 6, true);
        posts({SADDLE_HX, SADDLE_Y, SADDLE_Z0}, {SADDLE_HX, SADDLE_Y, SADDLE_Z1}, 6, true);
        for (float s : {-1.f, 1.f}) for (int k = 0; k < 4; ++k)   // bone spines out along its shoulders' edge
            box(B * T(vec3{s * (SADDLE_HX + 2.f), SADDLE_Y + 1.5f, SADDLE_Z0 + 5.f + k * 9.f}) * RZ(-s * 0.6f) * S(vec3{1.f, 7.f - k, 1.f}), bone * 0.75f);
        // -- its crown (its head held up): plates of skull, cracks of light, a rim of teeth --
        if (g.headHold > 0.99f) {
            plates(-CROWN_HX, CROWN_Z0, CROWN_HX, CROWN_Z1, SADDLE_Y, 5, 5, 13.f, bone * 0.55f);
            for (int k = 0; k < 4; ++k) box(B * T(vec3{-10.f + k * 6.5f, SADDLE_Y + 0.03f, 70.f + 7.f * std::sin(k * 2.f)}) * RY(k * 0.9f) * S(vec3{5.f, 0.02f, 0.18f}), soot, fire * 0.9f);
            for (int k = 0; k < 9; ++k) {
                float z = CROWN_Z0 + 2.f + k * (CROWN_Z1 - CROWN_Z0 - 4.f) / 8.f;
                for (float s : {-1.f, 1.f}) box(B * T(vec3{s * (CROWN_HX + 0.6f), SADDLE_Y + 0.8f, z}) * RZ(-s * 0.35f) * S(vec3{0.9f, 2.2f, 1.2f}), bone * 0.8f);
            }
        }
    }

    // ---- the chest: belly plates, the cavity's glow, the eye-core, the ribs ----
    {
        box(B * T(vec3{0, 46, -12}) * RX(0.18f) * S(vec3{58, 26, 92}), iron * 0.8f);    // belly
        float ribs = g.ribs;
        box(B * T(vec3{0, 62, 8}) * S(vec3{44, 50, 30}), soot, fire * (0.12f + 0.4f * ribs));   // the foundry inside it
        // The eye-core: a dark shell, lids sliding apart, the red iris, a slit pupil
        const vec3 e = Gleaner::eyeLocal();
        float open = g.eyeOpen;
        lump(B * T(e) * S(vec3{50}), iron * 0.7f, vec3{0.5f, 0.05f, 0.02f} * (0.2f + 0.8f * open));
        lump(B * T(e + vec3{0, 0, 5}) * RY(t * 0.1f) * S(vec3{44.f * (0.25f + 0.75f * open), 44.f * (0.1f + 0.9f * open), 44.f}),
             {0.6f, 0.08f, 0.04f}, vec3{3.2f, 0.35f, 0.12f} * open * (0.85f + 0.15f * std::sin(t * 2.f)) + vec3{2.f} * look.flash);
        box(B * T(e + vec3{0, 0, 26.6f}) * S(vec3{4.5f, 32.f * open + 0.1f, 1.f}), soot);   // the pupil
        for (float s : {-1.f, 1.f})   // the lids
            box(B * T(e + vec3{0, s * (9.f + 19.f * open), 18.f}) * RX(-s * 0.35f * open) * S(vec3{52, 20, 16}), rust * 0.7f);
        for (int k = 0; k < 4; ++k) {   // the cables it hangs from
            float a = 0.6f + k * 0.65f;
            localBar(e + vec3{std::cos(a) * 12.f, 12.f, 0}, spine(0.05f + 0.06f * k) + vec3{std::cos(a) * 24.f, -26.f, 0}, 1.6f, 1.6f, soot);
        }
        // Five ribs a side, opening outward like doors
        for (int r = 0; r < 5; ++r) {
            vec3 root = spine(0.03f + 0.065f * r) + vec3{0, -12, 0};
            for (float s : {-1.f, 1.f}) {
                vec3 A = root + vec3{s * 14.f, 0, 0};
                vec3 pts[3] = {root + vec3{s * 52.f, -18, 12}, root + vec3{s * 50.f, -66, 22}, root + vec3{s * 18.f, -92, 26}};
                float ang = s * ribs * 0.85f;
                vec3 prev = A;
                for (auto& p : pts) {   // swing it about the root, outward and up
                    vec3 d = p - A;
                    p = A + vec3{d.x * std::cos(ang) - d.y * std::sin(ang), d.x * std::sin(ang) + d.y * std::cos(ang), d.z};
                    localBar(prev, p, 7.f, 8.f, bone * (0.9f - 0.05f * r));
                    prev = p;
                }
            }
        }
    }

    // ---- the head: thrust out ahead of its shoulders on a short neck, bowed; for the scream it comes up and back ----
    {
        float lift = g.headLift, hold = Gleaner::ease(g.headHold);
        vec3 h = glm::mix(glm::mix(Gleaner::headLocal(), vec3{0, 140, 86}, lift), heldHead(), hold);
        vec3 n0 = spine(0.f) + vec3{0, -14, 8};
        vec3 n1 = glm::mix(n0, h, 0.5f) + vec3{0, 6, 0};
        vec3 n2 = h + vec3{0, 4, -14};
        localBar(n0, n1, 24, 22, iron); localBar(n1, n2, 20, 18, iron);
        box(B * T(n1 + vec3{0, 12, 0}) * S(vec3{16, 8, 20}), rust * 0.8f);   // a plate on the neck
        // the collar: spines of scrap fanned round the neck's root, breaking the shoulders' line
        for (int k = 0; k < 9; ++k) {
            float a = -1.25f + k * 0.3125f;
            vec3 d{std::sin(a), std::cos(a) * 0.9f, 0.35f};
            box(B * alongZ(n0 + d * 16.f + vec3{0, 0, 6}, d) * S(vec3{4.f, 4.f, 22.f + 10.f * ((k * 7) % 3)}), bone * 0.75f);
        }
        float pitch = glm::mix(glm::mix(0.38f, -0.6f, lift), 0.f, hold);    // bowed .. thrown back to scream .. held level
        // It watches you: the head turns (and tips) toward what it is looking at, slowly, within its neck's reach
        float turnTo = 0.f, tip = 0.f;
        if (look.watching) {
            vec3 inv = vec3(glm::inverse(B) * glm::vec4(look.watch, 1.f)) - h;
            turnTo = std::clamp(std::atan2(inv.x, inv.z), -0.6f, 0.6f);
            tip = std::clamp(-std::atan2(inv.y, std::max(1.f, std::hypot(inv.x, inv.z))) * 0.5f, -0.2f, 0.25f);
        }
        turnTo += 0.06f * std::sin(t * 0.21f);   // and never quite still
        turnTo *= 1.f - hold; tip *= 1.f - hold;  // (held, it is ground: it must not move under you)
        mat4 H = B * T(h) * RY(turnTo) * RX(pitch + tip * (1.f - lift));
        // Its crown: a plate of slag-crusted bone over the skull, two horns at its back, a rim round it
        box(H * T(vec3{0, 11.f, CROWN_Z0 + (CROWN_Z1 - CROWN_Z0) * 0.5f - heldHead().z}) * S(vec3{CROWN_HX * 2.f, 2.f, CROWN_Z1 - CROWN_Z0}), bone * 0.62f);
        for (int k = 0; k < 5; ++k)   // cracks across it, its furnace light under them
            box(H * T(vec3{-12.f + k * 6.f, 12.03f, -2.f + std::sin(k * 1.7f) * 6.f}) * RY(0.6f * std::sin(k * 2.3f)) * S(vec3{0.35f, 0.06f, 14.f}), soot, fire * 0.5f);
        for (float s : {-1.f, 1.f}) {
            box(H * T(vec3{s * CROWN_HX, 12.35f, -3.f}) * S(vec3{0.8f, 0.7f, CROWN_Z1 - CROWN_Z0}), bone * 0.5f);
            vec3 hp = crownHorn(s > 0.f) - heldHead();
            box(H * T(hp + vec3{0, HORN_TALL * 0.5f, 0}) * RZ(-s * 0.08f) * S(vec3{HORN_R * 2.f, HORN_TALL, HORN_R * 2.f}), bone * 0.8f);
            box(H * T(hp + vec3{0, HORN_TALL + 2.f, -1.f}) * RX(-0.5f) * S(vec3{HORN_R * 1.3f, 5.f, HORN_R * 1.3f}), bone * 0.7f);
            float len = CROWN_HX - NECK_HX;
            box(H * T(vec3{s * (NECK_HX + len * 0.5f), 12.35f, CROWN_Z0 - heldHead().z}) * S(vec3{len, 0.7f, 0.8f}), bone * 0.5f);
        }
        box(H * T(vec3{0, 12.35f, CROWN_Z1 - heldHead().z}) * S(vec3{CROWN_HX * 2.f, 0.7f, 0.8f}), bone * 0.5f);
        box(H * S(vec3{26, 20, 36}), bone * 0.75f);
        box(H * T(vec3{0, 9, 4}) * S(vec3{30, 6, 26}), rust * 0.7f);                    // brow
        for (float s : {-1.f, 1.f}) box(H * T(vec3{s * 7.f, 3, 18.2f}) * S(vec3{5, 3, 1}), soot, vec3{1.6f, 0.25f, 0.06f} * (0.5f + 0.5f * g.eyeOpen));   // sockets
        box(H * T(vec3{0, -6, 6}) * S(vec3{20, 6, 26}), soot, fire * (0.3f + 2.4f * g.jaw));   // the mouth's light
        mat4 J = H * T(vec3{0, -9, -14}) * RX(0.95f * g.jaw) * T(vec3{0, 0, 16});            // hinged at the back
        box(J * S(vec3{23, 6, 32}), bone * 0.65f);
        for (int k = 0; k < 6; ++k) for (float s : {-1.f, 1.f}) {   // teeth, top and bottom
            box(J * T(vec3{s * (3.f + k * 1.6f), 4.5f, 14.f - k * 4.5f}) * S(vec3{1.6f, 4.f, 1.6f}), bone);
            box(H * T(vec3{s * (3.f + k * 1.6f), -11.f, 16.f - k * 4.5f}) * S(vec3{1.6f, 4.f, 1.6f}), bone);
        }
    }

    // ---- the arms ----
    for (int a = 0; a < 2; ++a) {
        const float s = Gleaner::side(a);
        vec3 sh = g.shoulder(a), el = g.elbow(a), hd = g.hand(a);
        vec3 fist = hd + vec3{0, 14 * K, 0};
        lump(T(sh + vec3{0, 8 * K, 0}) * RY(g.yaw) * S(vec3{40, 28, 40} * K), rust * 0.85f);   // the pauldron
        bar(sh, el, 25 * K, 23 * K, iron);
        box(alongZ((sh + el) * 0.5f + vec3{0, 9 * K, 0}, el - sh) * S(vec3{17 * K, 6 * K, glm::length(el - sh) * 0.75f}), rust);   // plate down the upper arm
        lump(T(el) * S(vec3{31 * K}), iron * 1.1f);
        bar(el, fist, 22 * K, 21 * K, iron);
        for (int k = 0; k < 3; ++k) {   // spines down the back of the forearm
            vec3 p = glm::mix(el, fist, 0.25f + 0.25f * k);
            box(alongZ(p + vec3{0, 11 * K, 0}, vec3{s * 0.2f, 1, -0.4f}) * S(vec3{3.5f * K, 3.5f * K, (16.f - 3.f * k) * K}), bone * 0.8f);
        }
        // The fist: a great lump, four knuckles on the ground ahead of it
        vec3 f = g.fwd();
        lump(T(fist) * RY(g.yaw + s * 0.2f) * S(vec3{38, 30, 36} * K), iron);
        for (int k = 0; k < 4; ++k) {
            vec3 kp = fist + (f * 16.f + g.right() * ((k - 1.5f) * 8.f) + vec3{0, -6.f + (k % 2) * 2.f, 0}) * K;
            box(T(kp) * RY(g.yaw) * S(vec3{7, 12, 10} * K), bone * 0.7f);
        }
        // Light in the seam of the wrist
        box(alongZ(glm::mix(el, fist, 0.92f), fist - el) * S(vec3{23 * K, 2.5f * K, 3 * K}), soot, fire * 0.9f);
    }

    // ---- the legs: withered, dragging behind ----
    for (float s : {-1.f, 1.f}) {
        vec3 hip = local(vec3{s * 24, 46, -118});
        vec3 knee = local(vec3{s * 36, 18, -150}); knee.y = std::max(knee.y, 10.f * K);
        vec3 foot = local(vec3{s * 30, 0, -196}); foot.y = std::max(foot.y, 3.f * K);
        bar(hip, knee, 10 * K, 10 * K, iron * 0.9f); bar(knee, foot, 8 * K, 8 * K, iron * 0.9f);
        lump(T(knee) * S(vec3{14 * K}), bone * 0.7f);
        lump(T(foot) * RY(g.yaw) * S(vec3{16, 7, 20} * K), iron);
    }

    // ---- rising out of the plain: rubble and earth pouring off its back and sides ----
    if (look.shedding > 0.01f) {
        for (int k = 0; k < 40; ++k) {
            float u = std::fmod(t * 0.7f + k * 0.618f, 1.f);
            float along = (float)(k % 10) / 10.f;
            vec3 from = local(spine(along) + vec3{(k % 3 - 1) * 24.f, 20.f, 0.f});
            vec3 p = from + vec3{std::sin(k * 1.9f) * 12.f * K * u, -u * u * 220.f * K, std::cos(k * 2.3f) * 12.f * K * u};
            if (p.y < 0.f) continue;
            float sz = (3.f + 5.f * ((k * 7) % 5) / 4.f) * K * look.shedding;
            lump(T(p) * RY(k + u * 3.f) * RX(u * 2.f) * S(vec3{sz}), vec3{0.2f, 0.12f, 0.09f});
        }
    }

    // ---- the charms: rejected units on chains from its belly, swinging as it moves ----
    for (int k = 0; k < 7; ++k) {
        vec3 hook = local(vec3{-30.f + k * 10.f, 36.f - (k % 3) * 4.f, -70.f + (k % 4) * 22.f});
        float len = (18.f + 9.f * (k % 3)) * K;
        float sw = 0.18f * std::sin(t * 0.8f + k * 1.7f) + 3.f * g.roll();
        vec3 dir{std::sin(sw), -std::cos(sw), 0.08f * std::sin(t * 0.6f + k)};
        vec3 end = hook + dir * len;
        if (end.y < 4.f * K) end.y = 4.f * K;
        bar(hook, end, 0.9f * K, 0.9f * K, soot);
        // the body on the hook: chest, head, arms, legs (twice a unit's size: they are the big ones)
        mat4 root = T(end) * RY(t * 0.2f + k) * S(vec3{2.f * K});
        box(root * T({0, -1.3f, 0}) * S({0.6f, 0.65f, 0.4f}), bone);
        box(root * T({0, -0.75f, 0}) * S({0.4f, 0.4f, 0.4f}), bone);
        box(root * T({0, -2.4f, 0}) * S({0.55f, 1.6f, 0.3f}), bone * 0.8f);
        for (float s : {-1.f, 1.f}) box(root * T({s * 0.45f, -1.4f, 0}) * S({0.16f, 1.2f, 0.18f}), bone * 0.8f);
    }
}
}  // namespace gleaner
