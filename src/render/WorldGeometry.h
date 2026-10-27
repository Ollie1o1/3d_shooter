#pragma once
// =============================================================================
// WorldGeometry.h — the static level geometry (walls, props, neon, floors,
// shapes) as vertex and index lists batched by texture. No OpenGL, so the
// tests can check it: every surface you can see has a face toward you.
// =============================================================================
#include "world/Level.h"
#include "core/Vertex.h"
#include "render/PropFile.h"
#include <vector>
#include <algorithm>
#include <cmath>

// Static level geometry, batched by texture. Each wall's material picks the
// texture of its sides and of its top; undersides are always metal, and the
// self-lit neon is drawn untextured.
enum WorldTex { TEX_GRID, TEX_BRICK, TEX_METAL, TEX_PANEL, TEX_ROCK, TEX_CONCRETE,
                // Act I's own (MaterialTex.h)
                TEX_ADOBE, TEX_TERRACOTTA, TEX_WOOD, TEX_PLATE, TEX_TREAD, TEX_MARBLE, TEX_FLAG,
                TEX_CIRCUIT, TEX_CIRCUIT_FLOOR, TEX_BASALT,
                // ACT II and FAST's own
                TEX_LIMESTONE, TEX_BRASS,
                TEX_COUNT };

inline WorldTex sideTex(Mat m) {
    switch (m) {
        case Mat::PANEL: return TEX_PANEL;  case Mat::ROCK: return TEX_ROCK;
        case Mat::METAL: return TEX_METAL;  case Mat::CONCRETE: return TEX_CONCRETE;
        case Mat::ADOBE: return TEX_ADOBE;  case Mat::PLATE: return TEX_PLATE;
        case Mat::MARBLE: return TEX_MARBLE; case Mat::CIRCUIT: return TEX_CIRCUIT;
        case Mat::BASALT: return TEX_BASALT; case Mat::WOOD: return TEX_WOOD;
        case Mat::LIMESTONE: return TEX_LIMESTONE; case Mat::BRASS: return TEX_BRASS;
        default: return TEX_BRICK;
    }
}
inline WorldTex topTex(Mat m) {
    switch (m) {
        case Mat::PANEL: case Mat::METAL: return TEX_METAL;
        case Mat::ROCK: return TEX_ROCK;
        case Mat::ADOBE: return TEX_TERRACOTTA;
        case Mat::PLATE: return TEX_TREAD;
        case Mat::MARBLE: case Mat::BASALT: return TEX_FLAG;
        case Mat::CIRCUIT: return TEX_CIRCUIT_FLOOR;
        case Mat::WOOD: return TEX_WOOD;
        case Mat::LIMESTONE: return TEX_LIMESTONE;
        case Mat::BRASS: return TEX_BRASS;
        default: return TEX_GRID;
    }
}

// The level's static geometry on the CPU, batched by texture (no OpenGL:
// tests check it). WorldMesh.h uploads it.
// A soft contact shadow on the ground around something standing on it: a
// quad over the footprint grown by `reach`, darkest at the footprint's edge
// (contact.frag measures the distance to `rect`: x0 z0 x1 z1)
struct ContactVert { glm::vec3 pos; glm::vec4 rect; glm::vec2 strengthReach; };

struct WorldGeometry {
    std::vector<Vertex> V[TEX_COUNT], nV;
    std::vector<unsigned int> I[TEX_COUNT], nI;
    std::vector<ContactVert> contact;   // triangles
};

// What a box with this bottom rests on: the surface's height and its extent
// in X and Z (the shadow stays on it). False: it's floating (a canopy, a bridge)
struct Support { float y; glm::vec4 rect; };
inline bool findSupport(const LevelData& L, const AABB& b, const Wall* self, Support& out) {
    const float EPS = 0.06f;
    float best = 0.f;
    auto overlap = [&](float x0, float z0, float x1, float z1) {
        float w = std::min(x1, b.max.x) - std::max(x0, b.min.x), d = std::min(z1, b.max.z) - std::max(z0, b.min.z);
        return w > 0.f && d > 0.f ? w * d : 0.f;
    };
    auto consider = [&](float y, float x0, float z0, float x1, float z1) {
        if (std::fabs(y - b.min.y) > EPS) return;
        float a = overlap(x0, z0, x1, z1);
        if (a > best) { best = a; out = {y, {x0, z0, x1, z1}}; }
    };
    for (const FloorPatch& f : L.floors)
        consider(f.y, std::min(f.x0, f.x1), std::min(f.z0, f.z1), std::max(f.x0, f.x1), std::max(f.z0, f.z1));
    for (const Wall& w : L.walls) {
        if (&w == self || w.dynamic || w.hidden) continue;
        consider(w.box.max.y, w.box.min.x, w.box.min.z, w.box.max.x, w.box.max.z);
    }
    if (best <= 0.f) {   // the hard ground under it, with no floor drawn there
        float cx = (b.min.x + b.max.x) * 0.5f, cz = (b.min.z + b.max.z) * 0.5f;
        if (std::fabs(L.baseFloor(cx, cz) - b.min.y) > EPS) return false;
        out = {b.min.y, {-1e5f, -1e5f, 1e5f, 1e5f}};
    }
    return true;
}

// Brightness of a side at height h above the base of a box `span` tall. Over
// the whole side a gentle fall toward the base (tall rock and towers: their
// bottom 12 m); grounded, also a contact band, dark where it meets the floor
// and gone by 0.9 m
inline float sideShade(float h, float span, bool grounded) {
    float t = span > 12.f ? std::clamp(h / 12.f + 0.35f, 0.f, 1.f) : span > 0.01f ? std::clamp(h / span, 0.f, 1.f) : 1.f;
    float overall = 0.78f + 0.22f * t;
    if (!grounded) return overall;
    float c = std::clamp(h / 0.9f, 0.f, 1.f);
    c = c * c * (3.f - 2.f * c);
    return overall * (0.45f + 0.55f * c);
}

inline WorldGeometry buildWorldGeometry(const LevelData& L) {
    WorldGeometry G;
    auto& V = G.V; auto& I = G.I; auto& nV = G.nV; auto& nI = G.nI;
    const float texScale = 0.25f;   // one texture repeat per 4 m

    auto pushFace = [&](std::vector<Vertex>& verts, std::vector<unsigned int>& idx,
                        glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
                        glm::vec3 n, glm::vec3 col, bool shade) {
        // Sides darken toward their base: a cheap contact shadow
        float yLo = std::min({a.y, b.y, c.y, d.y});
        float ySpan = std::max({a.y, b.y, c.y, d.y}) - yLo;
        auto gc = [&](float y) -> glm::vec3 {
            if (!shade) return col;
            float t = (ySpan > 0.01f) ? (y - yLo) / ySpan : 1.f;
            // Tall rock and towers shouldn't go black for 30 m: shade the bottom few metres only
            if (ySpan > 12.f) t = glm::clamp((y - yLo) / 12.f + 0.35f, 0.f, 1.f);
            return col * glm::mix(0.55f, 1.0f, t);
        };
        auto worldUV = [&](glm::vec3 p) -> glm::vec2 {
            float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
            if (ay > ax && ay > az) return {p.x * texScale, p.z * texScale};
            else if (ax > az)       return {p.z * texScale, p.y * texScale};
            else                    return {p.x * texScale, p.y * texScale};
        };
        unsigned int base = (unsigned int)verts.size();
        verts.push_back({a, worldUV(a), n, gc(a.y)});
        verts.push_back({b, worldUV(b), n, gc(b.y)});
        verts.push_back({c, worldUV(c), n, gc(c.y)});
        verts.push_back({d, worldUV(d), n, gc(d.y)});
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    auto pushTri = [&](std::vector<Vertex>& verts, std::vector<unsigned int>& idx,
                       glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n, glm::vec3 col, bool shade, float yLo, float ySpan) {
        auto gc = [&](float y) -> glm::vec3 {
            if (!shade || ySpan < 0.01f) return col;
            float t = ySpan > 12.f ? glm::clamp((y - yLo) / 12.f + 0.35f, 0.f, 1.f) : (y - yLo) / ySpan;
            return col * glm::mix(0.55f, 1.0f, glm::clamp(t, 0.f, 1.f));
        };
        float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
        auto uv = [&](glm::vec3 p) -> glm::vec2 {
            if (ay > ax && ay > az) return {p.x * texScale, p.z * texScale};
            if (ax > az)            return {p.z * texScale, p.y * texScale};
            return {p.x * texScale, p.y * texScale};
        };
        unsigned int base = (unsigned int)verts.size();
        verts.push_back({a, uv(a), n, gc(a.y)});
        verts.push_back({b, uv(b), n, gc(b.y)});
        verts.push_back({c, uv(c), n, gc(c.y)});
        idx.insert(idx.end(), {base, base + 1, base + 2});
    };
    // A shape: sides shade toward the shape's base like a wall's; anything
    // facing up gets the top texture
    auto pushShape = [&](const Shape& s) {
        float yLo = 1e9f, yHi = -1e9f;
        s.forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3) {
            yLo = std::min({yLo, a.y, b.y, c.y}); yHi = std::max({yHi, a.y, b.y, c.y});
        });
        Mat m = (Mat)s.mat;
        s.forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
            if (s.neon) {
                pushTri(nV, nI, a, b, c, n, s.color, false, yLo, yHi - yLo);
                if (s.twoSided) pushTri(nV, nI, a, c, b, -n, s.color, false, yLo, yHi - yLo);
                return;
            }
            int t = n.y > 0.7f ? topTex(m) : n.y < -0.7f ? TEX_METAL : sideTex(m);
            pushTri(V[t], I[t], a, b, c, n, n.y < -0.7f ? s.color * 0.8f : s.color, n.y < 0.7f, yLo, yHi - yLo);
            if (s.twoSided) {   // and its back, for an open shape seen from the other side
                int tb = -n.y > 0.7f ? topTex(m) : -n.y < -0.7f ? TEX_METAL : sideTex(m);
                pushTri(V[tb], I[tb], a, c, b, -n, s.color * 0.85f, -n.y < 0.7f, yLo, yHi - yLo);
            }
        });
    };

    // A side in horizontal bands, so the contact shade can follow its curve
    // (vertex colours only interpolate linearly). Its low corners are at yLo
    auto pushSide = [&](std::vector<Vertex>& verts, std::vector<unsigned int>& idx,
                        glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n, glm::vec3 col, bool grounded) {
        float yLo = std::min({a.y, b.y, c.y, d.y}), yHi = std::max({a.y, b.y, c.y, d.y}), span = yHi - yLo;
        float hs[6]; int nCuts = 0;
        hs[nCuts++] = 0.f;
        if (grounded) for (float h : {0.12f, 0.3f, 0.55f, 0.9f}) if (h < span - 0.02f) hs[nCuts++] = h;
        hs[nCuts++] = span;
        auto worldUV = [&](glm::vec3 p) -> glm::vec2 {
            if (std::fabs(n.x) > std::fabs(n.z)) return {p.z * texScale, p.y * texScale};
            return {p.x * texScale, p.y * texScale};
        };
        for (int k = 0; k + 1 < nCuts; ++k) {
            glm::vec3 q[4] = {a, b, c, d};
            for (auto& p : q) p.y = yLo + (p.y < yLo + 1e-4f ? hs[k] : hs[k + 1]);
            unsigned int base = (unsigned int)verts.size();
            for (auto& p : q) verts.push_back({p, worldUV(p), n, col * sideShade(p.y - yLo, span, grounded)});
            idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        }
    };

    auto pushBox = [&](const AABB& b, glm::vec3 col, bool neon, Mat m, const Wall* self) {
        glm::vec3 mn = b.min, mx = b.max;
        int st = sideTex(m), tt = topTex(m);
        auto& sv = neon ? nV : V[st];      auto& si = neon ? nI : I[st];
        auto& tv = neon ? nV : V[tt];      auto& ti = neon ? nI : I[tt];
        auto& bv = neon ? nV : V[TEX_METAL]; auto& bi = neon ? nI : I[TEX_METAL];
        if (neon) {
            pushFace(sv, si, {mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{ 1, 0, 0}, col, false);
            pushFace(sv, si, {mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1, 0, 0}, col, false);
            pushFace(sv, si, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{ 0, 0, 1}, col, false);
            pushFace(sv, si, {mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{ 0, 0,-1}, col, false);
        } else {
            Support sup;
            bool grounded = findSupport(L, b, self, sup);
            pushSide(sv, si, {mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{ 1, 0, 0}, col, grounded);
            pushSide(sv, si, {mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1, 0, 0}, col, grounded);
            pushSide(sv, si, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{ 0, 0, 1}, col, grounded);
            pushSide(sv, si, {mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{ 0, 0,-1}, col, grounded);
            // Its shadow on what it stands on: reaching further for taller
            // things, and kept on that surface (never hanging off its edge)
            float h = mx.y - mn.y;
            if (grounded && h > 0.08f) {
                float reach = std::clamp(0.3f + 0.3f * h, 0.35f, 1.1f);
                float strength = std::clamp(0.30f + 0.12f * h, 0.30f, 0.55f);
                float x0 = std::max(mn.x - reach, sup.rect.x), z0 = std::max(mn.z - reach, sup.rect.y);
                float x1 = std::min(mx.x + reach, sup.rect.z), z1 = std::min(mx.z + reach, sup.rect.w);
                if (x1 > x0 && z1 > z0) {
                    float y = sup.y + 0.02f;
                    glm::vec4 r{mn.x, mn.z, mx.x, mx.z};
                    glm::vec2 sr{strength, reach};
                    ContactVert q[4] = {{{x0, y, z0}, r, sr}, {{x0, y, z1}, r, sr}, {{x1, y, z1}, r, sr}, {{x1, y, z0}, r, sr}};
                    G.contact.insert(G.contact.end(), {q[0], q[1], q[2], q[0], q[2], q[3]});
                }
            }
        }
        pushFace(tv, ti, {mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},{ 0, 1, 0}, col, false);
        // The underside, unless it sits on the ground under it (Act I's at Y 0, Act II's far lower)
        if (b.min.y > L.baseFloor((mn.x + mx.x) * 0.5f, (mn.z + mx.z) * 0.5f) + 0.1f || neon)
            pushFace(bv, bi, {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z},{ 0,-1, 0}, col * 0.8f, false);
    };

    // Props made in Blender (PropFile.h): each triangle with its own normals, shaded toward the prop's
    // base like a shape; self-lit parts with the neon. A box a prop dresses isn't drawn itself.
    std::vector<char> dressedWall(L.walls.size(), 0), dressedProp(L.props.size(), 0);
    for (const PlacedModel& pm : L.models) {
        const propfile::Prop* pr = propfile::get(pm.name);
        if (!pr) continue;   // (no file: the box draws as it always did)
        if (pm.wall >= 0 && pm.wall < (int)L.walls.size()) dressedWall[pm.wall] = 1;
        if (pm.prop >= 0 && pm.prop < (int)L.props.size()) dressedProp[pm.prop] = 1;
        const Mat own = pm.wall >= 0 ? L.walls[pm.wall].mat : pm.prop >= 0 ? L.props[pm.prop].mat : pm.mat;
        const glm::mat4 world = propfile::placement(*pr, pm.free, pm.xf, pm.fit, pm.yaw);
        const float yLo = glm::vec3(world * glm::vec4(0.f, pr->lo.y, 0.f, 1.f)).y;
        const float ySpan = std::max(0.01f, glm::vec3(world * glm::vec4(0.f, pr->hi.y, 0.f, 1.f)).y - yLo);
        propfile::forEachTri(*pr, world, [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 na, glm::vec3 nb, glm::vec3 nc,
                                              glm::vec3 col, bool glows, int tex) {
            glm::vec3 fn = glm::normalize(glm::cross(b - a, c - a));
            int t = 0;
            if (!glows) { Mat m = tex >= 0 ? (Mat)tex : own; t = fn.y > 0.7f ? topTex(m) : fn.y < -0.7f ? TEX_METAL : sideTex(m); }
            std::vector<Vertex>& vv = glows ? nV : V[t];
            std::vector<unsigned int>& ii = glows ? nI : I[t];
            float ax = std::fabs(fn.x), ay = std::fabs(fn.y), az = std::fabs(fn.z);
            auto uv = [&](glm::vec3 p) -> glm::vec2 {
                if (ay > ax && ay > az) return {p.x * texScale, p.z * texScale};
                if (ax > az)            return {p.z * texScale, p.y * texScale};
                return {p.x * texScale, p.y * texScale};
            };
            auto shadeAt = [&](float y) { return glows ? col : col * glm::mix(0.6f, 1.0f, glm::clamp((y - yLo) / std::min(ySpan, 4.f), 0.f, 1.f)); };
            unsigned int base = (unsigned int)vv.size();
            vv.push_back({a, uv(a), na, shadeAt(a.y)});
            vv.push_back({b, uv(b), nb, shadeAt(b.y)});
            vv.push_back({c, uv(c), nc, shadeAt(c.y)});
            ii.insert(ii.end(), {base, base + 1, base + 2});
        });
    }

    for (auto& f : L.floors)
        pushFace(V[topTex(f.mat)], I[topTex(f.mat)], {f.x0,f.y,f.z1},{f.x1,f.y,f.z1},{f.x1,f.y,f.z0},{f.x0,f.y,f.z0},{0,1,0}, f.color, false);
    for (int i = 0; i < (int)L.walls.size(); ++i)
        if (!L.walls[i].hidden && !L.isDoorWall(i) && !dressedWall[i]) pushBox(L.walls[i].box, L.walls[i].color, false, L.walls[i].mat, &L.walls[i]);
    for (size_t i = 0; i < L.props.size(); ++i)
        if (!dressedProp[i]) pushBox(L.props[i].box, L.props[i].color, false, L.props[i].mat, &L.props[i]);
    for (auto& n : L.neon)  pushBox(n.box, n.color, true, Mat::BRICK, nullptr);
    for (auto& s : L.shapes) pushShape(s);

    return G;
}
