#pragma once
// =============================================================================
// Shapes.h — level geometry that isn't an axis-aligned box: round and tapered
// columns, towers and shafts, boxes turned to any angle, curved walls and
// bands, arches, domes, helixes and jagged rock. Baked into the static world
// mesh with the walls (WorldMesh.h). Purely visual: where one has to be
// solid, the level adds hidden collision boxes under it (LevelBuilder::solid).
//
// Two primitives do all of it:
//   PRISM — an n-sided column of radius 1 from y 0 to 1 (top scaled by
//           topScale, each vertex optionally jittered for rock), then `xf`
//   BOX   — the unit cube centred on the origin, then `xf`
// Everything else (ShapeKit) is built out of those.
// =============================================================================
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

struct Shape {
    enum class Kind { PRISM, BOX };
    Kind      kind = Kind::BOX;
    glm::mat4 xf{1.f};
    int       sides = 8;
    float     topScale = 1.f;
    float     jitter = 0.f;      // PRISM: radial wobble per vertex, 0..~0.4 (rock)
    uint32_t  seed = 1;
    bool      inward = false;    // PRISM: seen from inside (a shaft): faces turned in, no caps
    bool      caps = true;
    glm::vec3 color{0.4f};
    int       mat = 0;           // a Mat, as int (WorldMesh picks the texture)
    bool      neon = false;      // self-lit, untextured

    // Every triangle: f(a, b, c, normal), normals facing out (or in, if inward)
    template <class F> void forEachTri(F f) const {
        auto X = [&](glm::vec3 p) { return glm::vec3(xf * glm::vec4(p, 1.f)); };
        auto tri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
            a = X(a); b = X(b); c = X(c);
            glm::vec3 n = glm::cross(b - a, c - a);
            float l = glm::length(n);
            if (l < 1e-8f) return;
            f(a, b, c, n / l);
        };
        auto quad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) { tri(a, b, c); tri(a, c, d); };
        if (kind == Kind::BOX) {
            const float h = 0.5f;
            glm::vec3 p[8] = {{-h,-h,-h},{h,-h,-h},{h,h,-h},{-h,h,-h},{-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}};
            quad(p[1], p[2], p[6], p[5]);   // +x
            quad(p[4], p[7], p[3], p[0]);   // -x
            quad(p[3], p[7], p[6], p[2]);   // +y
            quad(p[0], p[1], p[5], p[4]);   // -y
            quad(p[5], p[6], p[7], p[4]);   // +z
            quad(p[0], p[3], p[2], p[1]);   // -z
            return;
        }
        int n = std::max(3, sides);
        std::vector<glm::vec3> lo(n), hi(n);
        uint32_t s = seed * 2654435761u + 12345u;
        auto rnd = [&]() { s = s * 1664525u + 1013904223u; return (float)((s >> 8) & 0xFFFF) / 65535.f; };
        for (int i = 0; i < n; ++i) {
            float a = (float)i / n * 6.2831853f;
            glm::vec3 d{std::cos(a), 0.f, -std::sin(a)};   // counter-clockwise seen from above
            float jl = 1.f + jitter * (rnd() * 2.f - 1.f), jh = 1.f + jitter * (rnd() * 2.f - 1.f);
            lo[i] = d * jl;
            hi[i] = d * (topScale * jh) + glm::vec3{0.f, 1.f, 0.f};
        }
        for (int i = 0; i < n; ++i) {
            int j = (i + 1) % n;
            if (inward) quad(lo[i], hi[i], hi[j], lo[j]);
            else        quad(lo[i], lo[j], hi[j], hi[i]);
        }
        if (inward || !caps) return;
        glm::vec3 top{0.f, 1.f, 0.f}, bot{0.f};
        for (int i = 0; i < n; ++i) {
            int j = (i + 1) % n;
            if (topScale > 0.001f) tri(top, hi[i], hi[j]);
            tri(bot, lo[j], lo[i]);
        }
    }
};

// Builders. Angles in radians; yaw turns about +Y (0: along +X).
struct ShapeKit {
    std::vector<Shape>& out;
    int  mat = 0;
    bool neon = false;

    static glm::mat4 T(glm::vec3 p) { return glm::translate(glm::mat4(1.f), p); }
    static glm::mat4 R(float a, glm::vec3 ax) { return glm::rotate(glm::mat4(1.f), a, ax); }
    static glm::mat4 Sc(glm::vec3 s) { return glm::scale(glm::mat4(1.f), s); }

    Shape& add(Shape s) { s.mat = mat; s.neon = s.neon || neon; out.push_back(s); return out.back(); }

    // An upright n-sided column: base centre, radius, height (top radius = r * taper)
    Shape& column(glm::vec3 base, float r, float h, glm::vec3 col, int sides = 10, float taper = 1.f) {
        Shape s; s.kind = Shape::Kind::PRISM; s.sides = sides; s.topScale = taper; s.color = col;
        s.xf = T(base) * Sc({r, h, r});
        return add(s);
    }
    // A round shaft seen from inside (walls facing in)
    Shape& shaft(glm::vec3 base, float r, float h, glm::vec3 col, int sides = 16) {
        Shape& s = column(base, r, h, col, sides);
        s.inward = true;
        return s;
    }
    // A column lying along an axis from a to b (pipes, logs, beams)
    Shape& rod(glm::vec3 a, glm::vec3 b, float r, glm::vec3 col, int sides = 8) {
        glm::vec3 d = b - a;
        float len = glm::length(d);
        Shape s; s.kind = Shape::Kind::PRISM; s.sides = sides; s.color = col;
        glm::vec3 up{0.f, 1.f, 0.f}, dir = d / std::max(len, 1e-4f);
        glm::vec3 axis = glm::cross(up, dir);
        float ang = std::acos(glm::clamp(glm::dot(up, dir), -1.f, 1.f));
        glm::mat4 rot = glm::length(axis) > 1e-5f ? R(ang, glm::normalize(axis)) : (dir.y < 0.f ? R(3.14159265f, {1, 0, 0}) : glm::mat4(1.f));
        s.xf = T(a) * rot * Sc({r, len, r});
        return add(s);
    }
    // A box of `size`, centred at c, turned by yaw (about Y), then pitch
    // (about its own X) and roll (about its own Z)
    Shape& box(glm::vec3 c, glm::vec3 size, glm::vec3 col, float yaw = 0.f, float pitch = 0.f, float roll = 0.f) {
        Shape s; s.kind = Shape::Kind::BOX; s.color = col;
        s.xf = T(c) * R(yaw, {0, 1, 0}) * R(pitch, {1, 0, 0}) * R(roll, {0, 0, 1}) * Sc(size);
        return add(s);
    }
    // A slab of rock: a jagged, tapering lump
    Shape& rock(glm::vec3 base, float r, float h, glm::vec3 col, uint32_t seed, float taper = 0.55f, int sides = 7) {
        Shape& s = column(base, r, h, col, sides, taper);
        s.jitter = 0.32f; s.seed = seed;
        s.xf = s.xf * R((float)(seed % 628) * 0.01f, {0, 1, 0});
        return s;
    }
    // A curved wall: the part of a ring (centre, radius) from angle a0 to a1,
    // `thick` through, y0..y1 high, in `seg` straight pieces. Angles are
    // measured from +X toward +Z.
    void curve(glm::vec3 centre, float radius, float a0, float a1, float y0, float y1, float thick, glm::vec3 col, int seg = 16) {
        float da = (a1 - a0) / seg;
        float chord = 2.f * radius * std::sin(std::fabs(da) * 0.5f) + 0.06f;
        for (int i = 0; i < seg; ++i) {
            float a = a0 + da * (i + 0.5f);
            glm::vec3 p = centre + glm::vec3{std::cos(a) * radius, (y0 + y1) * 0.5f, std::sin(a) * radius};
            box(p, {thick, y1 - y0, chord}, col, -a);
        }
    }
    // An arch standing in a vertical plane: feet `span` apart, rising `rise`
    // above `base` (half an ellipse; rise = span/2 for a round arch), `thick`
    // across the curve and `depth` through. yaw: the plane's direction (0: span along X).
    void arch(glm::vec3 base, float span, float rise, float thick, float depth, glm::vec3 col, float yaw = 0.f, int seg = 14) {
        float a = span * 0.5f;
        glm::mat4 W = T(base) * R(yaw, {0, 1, 0});
        for (int i = 0; i < seg; ++i) {
            float t0 = 3.14159265f * i / seg, t1 = 3.14159265f * (i + 1) / seg;
            glm::vec3 p0{std::cos(t0) * a, std::sin(t0) * rise, 0.f}, p1{std::cos(t1) * a, std::sin(t1) * rise, 0.f};
            glm::vec3 m = (p0 + p1) * 0.5f, d = p1 - p0;
            float len = glm::length(d) + 0.04f;
            float ang = std::atan2(d.y, d.x);
            Shape s; s.kind = Shape::Kind::BOX; s.color = col;
            s.xf = W * T(m) * R(ang, {0, 0, 1}) * Sc({len, thick, depth});
            add(s);
        }
    }
    // A dome: a stack of tapering rings
    void dome(glm::vec3 centre, float r, glm::vec3 col, int rings = 5, int sides = 14) {
        for (int i = 0; i < rings; ++i) {
            float a0 = 1.5708f * i / rings, a1 = 1.5708f * (i + 1) / rings;
            float r0 = std::cos(a0) * r, r1 = std::cos(a1) * r;
            float y0 = std::sin(a0) * r, y1 = std::sin(a1) * r;
            Shape& s = column(centre + glm::vec3{0, y0, 0}, r0, y1 - y0, col, sides, r1 / r0);
            s.caps = i == rings - 1;
        }
    }
    // Small boxes along a helix (stairs of light round a column)
    void helix(glm::vec3 base, float r, float h, float turns, glm::vec3 size, glm::vec3 col, int steps = 48) {
        for (int i = 0; i < steps; ++i) {
            float t = (float)i / (steps - 1);
            float a = t * turns * 6.2831853f;
            box(base + glm::vec3{std::cos(a) * r, t * h, std::sin(a) * r}, size, col, -a);
        }
    }
};
