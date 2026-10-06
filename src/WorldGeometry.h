#pragma once
// =============================================================================
// WorldGeometry.h — the static level geometry (walls, props, neon, floors,
// shapes) as vertex and index lists batched by texture. No OpenGL, so the
// tests can check it: every surface you can see has a face toward you.
// =============================================================================
#include "Level.h"
#include "Vertex.h"
#include <vector>
#include <algorithm>
#include <cmath>

// Static level geometry, batched by texture. Each wall's material picks the
// texture of its sides and of its top; undersides are always metal, and the
// self-lit neon is drawn untextured.
enum WorldTex { TEX_GRID, TEX_BRICK, TEX_METAL, TEX_PANEL, TEX_ROCK, TEX_CONCRETE, TEX_COUNT };

inline WorldTex sideTex(Mat m) {
    switch (m) {
        case Mat::PANEL: return TEX_PANEL;  case Mat::ROCK: return TEX_ROCK;
        case Mat::METAL: return TEX_METAL;  case Mat::CONCRETE: return TEX_CONCRETE;
        default: return TEX_BRICK;
    }
}
inline WorldTex topTex(Mat m) {
    switch (m) {
        case Mat::PANEL: case Mat::METAL: return TEX_METAL;
        case Mat::ROCK: return TEX_ROCK;
        default: return TEX_GRID;
    }
}

// The level's static geometry on the CPU, batched by texture (no OpenGL:
// tests check it). WorldMesh.h uploads it.
struct WorldGeometry {
    std::vector<Vertex> V[TEX_COUNT], nV;
    std::vector<unsigned int> I[TEX_COUNT], nI;
};

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

    auto pushBox = [&](const AABB& b, glm::vec3 col, bool neon, Mat m) {
        glm::vec3 mn = b.min, mx = b.max;
        int st = sideTex(m), tt = topTex(m);
        auto& sv = neon ? nV : V[st];      auto& si = neon ? nI : I[st];
        auto& tv = neon ? nV : V[tt];      auto& ti = neon ? nI : I[tt];
        auto& bv = neon ? nV : V[TEX_METAL]; auto& bi = neon ? nI : I[TEX_METAL];
        bool shade = !neon;
        pushFace(sv, si, {mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{ 1, 0, 0}, col, shade);
        pushFace(sv, si, {mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1, 0, 0}, col, shade);
        pushFace(sv, si, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{ 0, 0, 1}, col, shade);
        pushFace(sv, si, {mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{ 0, 0,-1}, col, shade);
        pushFace(tv, ti, {mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},{ 0, 1, 0}, col, false);
        // The underside, unless it sits on the ground under it (Act I's at Y 0, Act II's far lower)
        if (b.min.y > L.baseFloor((mn.x + mx.x) * 0.5f, (mn.z + mx.z) * 0.5f) + 0.1f || neon)
            pushFace(bv, bi, {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z},{ 0,-1, 0}, col * 0.8f, false);
    };

    for (auto& f : L.floors)
        pushFace(V[TEX_GRID], I[TEX_GRID], {f.x0,f.y,f.z1},{f.x1,f.y,f.z1},{f.x1,f.y,f.z0},{f.x0,f.y,f.z0},{0,1,0}, f.color, false);
    for (int i = 0; i < (int)L.walls.size(); ++i)
        if (!L.walls[i].hidden && !L.isDoorWall(i)) pushBox(L.walls[i].box, L.walls[i].color, false, L.walls[i].mat);
    for (auto& p : L.props) pushBox(p.box, p.color, false, p.mat);
    for (auto& n : L.neon)  pushBox(n.box, n.color, true, Mat::BRICK);
    for (auto& s : L.shapes) pushShape(s);

    return G;
}
