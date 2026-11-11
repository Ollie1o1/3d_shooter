#pragma once
// =============================================================================
// Marks.h — what a fight leaves on the room: the pit a round knocks out of a
// wall, the soot a blast burns into the floor, the stain where something died,
// and the spent brass lying where it fell. They stay for a while, so a room
// you have fought in looks fought in, then fade.
//
// Everything is capped (the AMBIENT DETAIL setting sets the caps; the oldest
// goes when a new one needs its place) and everything fades before it leaves,
// so nothing pops. No OpenGL: the game draws the marks as quads laid on their
// surface (mark.vert/frag, a multiply blend, one draw) and the casings as boxes.
// Tests: tests/test_polish.cpp.
// =============================================================================
#include "core/Collision.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace marks {

enum class Kind : uint8_t { PIT, SCORCH, STAIN, COUNT };

// How long each stays (seconds), and the share of that it spends fading out
inline float lifeOf(Kind k) { static const float L[] = {45.f, 90.f, 14.f}; return L[(int)k]; }
constexpr float FADE_SHARE = 0.3f;
constexpr float FADE_IN = 0.06f;
// What it does to the surface under it at full strength (the surface's colour times this)
inline glm::vec3 tintOf(Kind k) {
    static const glm::vec3 T[] = {{0.20f, 0.19f, 0.18f}, {0.16f, 0.14f, 0.13f}, {0.50f, 0.10f, 0.08f}};
    return T[(int)k];
}

struct Mark {
    glm::vec3 pos{0.f}, normal{0.f, 1.f, 0.f};
    float size = 0.1f;        // its radius (m)
    float seed = 0.f;         // 0..1: its own ragged outline
    float age = 0.f, life = 1.f;
    Kind  kind = Kind::PIT;
    bool  alive = false;
};

// A spent case on the floor. It isn't there until `delay` runs out (the time it took to fall)
struct Casing {
    glm::vec3 pos{0.f};
    glm::vec3 color{0.7f, 0.52f, 0.24f};
    float yaw = 0.f, length = 0.05f, age = 0.f, delay = 0.f;
    bool  alive = false;
};
constexpr float CASING_LIFE = 30.f, CASING_FADE = 3.f;

struct Vert { glm::vec3 pos; glm::vec4 uvSeedStrength; glm::vec4 tintKind; };

// The side of a box a point on its surface lies on
inline glm::vec3 faceNormal(const AABB& b, glm::vec3 p) {
    float best = 1e9f; glm::vec3 n{0.f, 1.f, 0.f};
    for (int a = 0; a < 3; ++a) {
        const float lo = std::fabs(p[a] - b.min[a]), hi = std::fabs(p[a] - b.max[a]);
        if (lo < best) { best = lo; n = glm::vec3{0.f}; n[a] = -1.f; }
        if (hi < best) { best = hi; n = glm::vec3{0.f}; n[a] = 1.f; }
    }
    return n;
}

class Field {
public:
    static constexpr int MAX_MARKS = 192, MAX_CASINGS = 64;
    Mark   marks[MAX_MARKS];
    Casing casings[MAX_CASINGS];

    // AMBIENT DETAIL (0 OFF .. 3): how many of each may lie about. OFF keeps only the stains
    // (they were always drawn) and no brass.
    static int markCap(int detail)   { static const int N[] = {32, 64, 128, MAX_MARKS};  return N[std::clamp(detail, 0, 3)]; }
    static int casingCap(int detail) { static const int N[] = {0, 16, 32, MAX_CASINGS};  return N[std::clamp(detail, 0, 3)]; }
    int detail = 3;

    void clear() {
        for (auto& m : marks) m.alive = false;
        for (auto& c : casings) c.alive = false;
    }
    int markCount() const { int n = 0; for (auto& m : marks) n += m.alive; return n; }
    int casingCount() const { int n = 0; for (auto& c : casings) n += c.alive; return n; }

    // A lower setting takes effect at once: the oldest go first
    void setDetail(int d) {
        detail = std::clamp(d, 0, 3);
        while (markCount() > markCap(detail)) oldestMark()->alive = false;
        while (casingCount() > casingCap(detail)) oldestCasing()->alive = false;
        if (detail == 0) for (auto& m : marks) if (m.kind != Kind::STAIN) m.alive = false;
    }

    // Leave a mark on a surface. A pit on top of another (a burst into one spot) renews
    // that one instead of stacking, and so does a stain. Returns the mark, or null if this setting leaves none.
    Mark* add(Kind kind, glm::vec3 at, glm::vec3 normal, float size) {
        if (detail == 0 && kind != Kind::STAIN) return nullptr;
        const float merge = kind == Kind::PIT ? 0.03f : kind == Kind::STAIN ? 0.3f : 0.f;   // (blasts overlap: each its own)
        if (merge > 0.f)
            for (auto& m : marks)
                if (m.alive && m.kind == kind && glm::dot(m.normal, normal) > 0.9f) {
                    const glm::vec3 d = m.pos - at;
                    if (glm::dot(d, d) < merge * merge) { m.age = std::min(m.age, FADE_IN); m.size = std::max(m.size, size); return &m; }
                }
        Mark* slot = nullptr;
        if (markCount() >= markCap(detail)) slot = oldestMark();
        else for (auto& m : marks) if (!m.alive) { slot = &m; break; }
        if (!slot) return nullptr;
        *slot = Mark{};
        slot->pos = at + normal * 0.012f;   // (just off the surface; the draw adds a depth offset too)
        slot->normal = normal; slot->size = size; slot->kind = kind;
        slot->life = lifeOf(kind);
        slot->seed = nextSeed();
        slot->alive = true;
        return slot;
    }

    void addCasing(glm::vec3 restAt, float delay, glm::vec3 color, float length) {
        if (casingCap(detail) <= 0) return;
        Casing* slot = nullptr;
        if (casingCount() >= casingCap(detail)) slot = oldestCasing();
        else for (auto& c : casings) if (!c.alive) { slot = &c; break; }
        if (!slot) return;
        *slot = Casing{};
        slot->pos = restAt; slot->delay = delay; slot->color = color; slot->length = length;
        slot->yaw = nextSeed() * 6.2831853f;
        slot->alive = true;
    }

    void tick(float dt) {
        for (auto& m : marks) if (m.alive) { m.age += dt; if (m.age >= m.life) m.alive = false; }
        for (auto& c : casings) if (c.alive) {
            if (c.delay > 0.f) { c.delay -= dt; continue; }
            c.age += dt; if (c.age >= CASING_LIFE) c.alive = false;
        }
    }

    // 0..1: in quickly, whole for most of its life, out slowly over the last of it
    static float strength(const Mark& m) {
        if (!m.alive) return 0.f;
        const float in = std::min(1.f, m.age / FADE_IN);
        const float out = std::clamp((m.life - m.age) / (m.life * FADE_SHARE), 0.f, 1.f);
        return in * out * out * (3.f - 2.f * out);
    }
    // 0..1: a casing's size (0 while it is still falling; it shrinks away at the end)
    static float shown(const Casing& c) {
        if (!c.alive || c.delay > 0.f) return 0.f;
        return std::clamp((CASING_LIFE - c.age) / CASING_FADE, 0.f, 1.f);
    }

    // Every mark within `range` of the camera as two triangles laid on its surface
    int build(std::vector<Vert>& out, glm::vec3 cam, float range = 70.f) const {
        out.clear();
        for (auto& m : marks) {
            const float s = strength(m);
            if (s <= 0.f) continue;
            const glm::vec3 d = m.pos - cam;
            if (glm::dot(d, d) > range * range) continue;
            const glm::vec3 ref = std::fabs(m.normal.y) < 0.9f ? glm::vec3{0.f, 1.f, 0.f} : glm::vec3{1.f, 0.f, 0.f};
            const glm::vec3 t = glm::normalize(glm::cross(ref, m.normal)) * m.size, b = glm::cross(m.normal, t);
            const glm::vec4 tk{tintOf(m.kind), (float)(int)m.kind};
            const Vert q[4] = {{m.pos - t - b, {-1.f, -1.f, m.seed, s}, tk}, {m.pos + t - b, {1.f, -1.f, m.seed, s}, tk},
                               {m.pos + t + b, {1.f, 1.f, m.seed, s}, tk},   {m.pos - t + b, {-1.f, 1.f, m.seed, s}, tk}};
            out.insert(out.end(), {q[0], q[1], q[2], q[0], q[2], q[3]});
        }
        return (int)out.size();
    }

private:
    uint32_t rng = 0x9e3779b9u;
    float nextSeed() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(rng & 0xffffu) / 65535.f; }
    // The one nearest its end (by the share of its life it has used)
    Mark* oldestMark() {
        Mark* o = nullptr;
        for (auto& m : marks) if (m.alive && (!o || m.age / m.life > o->age / o->life)) o = &m;
        return o;
    }
    Casing* oldestCasing() {
        Casing* o = nullptr;
        for (auto& c : casings) if (c.alive && (!o || c.age - c.delay > o->age - o->delay)) o = &c;
        return o;
    }
};

}  // namespace marks
