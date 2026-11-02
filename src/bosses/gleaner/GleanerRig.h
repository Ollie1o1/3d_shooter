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

struct Look {
    float ball = 0.f;         // lumpBall mesh id
    float flash = 0.f;        // 0..1 a white flare through its seams (hurt, the scream)
    glm::vec3 watch{0.f};     // what its head turns toward (the player)
    bool  watching = false;
    float shedding = 0.f;     // 0..1 rubble pouring off its back (rising out of the plain)
    float vent = 0.f;         // 0..1 the updraft vent in its side, glowing
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
        // the ridge: spines of every height (the mountain line while it lies)
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
        for (float s : {-1.f, 1.f}) box(B * T(vec3{base.x, top + 0.35f, base.z + s * DECK_HZ}) * S(vec3{DECK_HX * 2.f, 0.7f, 0.8f}), rust);
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
        float lift = g.headLift;
        vec3 h = glm::mix(Gleaner::headLocal(), vec3{0, 140, 86}, lift);
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
        float pitch = glm::mix(0.38f, -0.6f, lift);    // bowed .. thrown back to scream
        // It watches you: the head turns (and tips) toward what it is looking at, slowly, within its neck's reach
        float turnTo = 0.f, tip = 0.f;
        if (look.watching) {
            vec3 inv = vec3(glm::inverse(B) * glm::vec4(look.watch, 1.f)) - h;
            turnTo = std::clamp(std::atan2(inv.x, inv.z), -0.6f, 0.6f);
            tip = std::clamp(-std::atan2(inv.y, std::max(1.f, std::hypot(inv.x, inv.z))) * 0.5f, -0.2f, 0.25f);
        }
        turnTo += 0.06f * std::sin(t * 0.21f);   // and never quite still
        mat4 H = B * T(h) * RY(turnTo) * RX(pitch + tip * (1.f - lift));
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
