#pragma once
// =============================================================================
// WorldMesh.h — the static level geometry, batched into one mesh per texture,
// and the 1x1 grey texture untextured things are drawn with.
// =============================================================================
#include "Level.h"
#include "Mesh.h"
#include "gl.h"
#include <vector>
#include <algorithm>
#include <cmath>

static GLuint makeGreyTexture() {
    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    unsigned char grey[4]={200,200,200,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,grey);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    return tex;
}

// Static level geometry, batched by texture. Each wall's material picks the
// texture of its sides and of its top; undersides are always metal, and the
// self-lit neon is drawn untextured.
enum WorldTex { TEX_GRID, TEX_BRICK, TEX_METAL, TEX_PANEL, TEX_ROCK, TEX_CONCRETE, TEX_COUNT };
struct WorldMeshes { Mesh byTex[TEX_COUNT]; Mesh neon; };

static WorldTex sideTex(Mat m) {
    switch (m) {
        case Mat::PANEL: return TEX_PANEL;  case Mat::ROCK: return TEX_ROCK;
        case Mat::METAL: return TEX_METAL;  case Mat::CONCRETE: return TEX_CONCRETE;
        default: return TEX_BRICK;
    }
}
static WorldTex topTex(Mat m) {
    switch (m) {
        case Mat::PANEL: case Mat::METAL: return TEX_METAL;
        case Mat::ROCK: return TEX_ROCK;
        default: return TEX_GRID;
    }
}

static WorldMeshes buildWorldMeshes(const LevelData& L) {
    std::vector<Vertex> V[TEX_COUNT], nV;
    std::vector<unsigned int> I[TEX_COUNT], nI;
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
        if (b.min.y > 0.1f || neon)
            pushFace(bv, bi, {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z},{ 0,-1, 0}, col * 0.8f, false);
    };

    for (auto& f : L.floors)
        pushFace(V[TEX_GRID], I[TEX_GRID], {f.x0,f.y,f.z1},{f.x1,f.y,f.z1},{f.x1,f.y,f.z0},{f.x0,f.y,f.z0},{0,1,0}, f.color, false);
    for (int i = 0; i < (int)L.walls.size(); ++i)
        if (!L.walls[i].hidden && !L.isDoorWall(i)) pushBox(L.walls[i].box, L.walls[i].color, false, L.walls[i].mat);
    for (auto& p : L.props) pushBox(p.box, p.color, false, p.mat);
    for (auto& n : L.neon)  pushBox(n.box, n.color, true, Mat::BRICK);

    WorldMeshes m;
    for (int t = 0; t < TEX_COUNT; ++t) m.byTex[t].upload(V[t], I[t]);
    m.neon.upload(nV, nI);
    return m;
}
