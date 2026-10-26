#pragma once
// =============================================================================
// NeonLights.h — the neon (and lava) lights the world: every strip is a box of
// light, and a surface near one picks up its colour (shader.frag / box_inst.frag
// neonLight(): distance to the nearest point of the box, a soft wrap so the
// wall a strip is mounted on glows around it, gone by 7 m).
//
// The shaders take MAX_NEON at a time: each frame the ones nearest the camera.
// Each fades out over the second half of the way to the cut (the distance of the first one left out),
// so a light leaving the set has already faded to nothing — no popping as you
// walk. No OpenGL here: tests check the picking.
// =============================================================================
#include "world/Level.h"
#include <vector>
#include <algorithm>
#include <cmath>

constexpr int MAX_NEON = 8;

struct NeonLight { AABB box; glm::vec3 color; };   // color: the light it gives (not the strip's own)

struct NeonSet {
    int n = 0;
    glm::vec3 mn[MAX_NEON], mx[MAX_NEON], col[MAX_NEON];
};

// How strongly a strip of colour c and this size lights things. A thin strip
// gives a little light; a wide panel (a lava channel) a lot more
inline NeonLight neonLightOf(const AABB& b, glm::vec3 c) {
    glm::vec3 s = b.max - b.min;
    float e[3] = {s.x, s.y, s.z};
    std::sort(e, e + 3);
    float size = std::clamp(std::sqrt(e[1] / 0.2f), 0.7f, 2.5f);   // the middle extent: how wide the strip is
    return {b, c * 1.8f * size};
}

// A band of light wrapped round a solid box (LevelBuilder::ring) is stored as
// one box enclosing it; as a light it's the four thin strips of its sides, or
// the whole top of what it wraps would light up like a panel
inline bool wrapsSomething(const LevelData& L, const AABB& n) {
    for (auto& w : L.walls) {
        const AABB& b = w.box;
        if (b.min.x >= n.min.x - 0.01f && b.max.x <= n.max.x + 0.01f && b.min.z >= n.min.z - 0.01f && b.max.z <= n.max.z + 0.01f &&
            b.min.y <= n.min.y + 0.01f && b.max.y >= n.max.y - 0.01f && (b.max.x - b.min.x) > (n.max.x - n.min.x) * 0.8f)
            return true;
    }
    return false;
}

inline std::vector<NeonLight> buildNeonLights(const LevelData& L) {
    std::vector<NeonLight> out;
    for (auto& n : L.neon) {
        float bright = std::max({n.color.r, n.color.g, n.color.b});
        if (bright < 0.05f) continue;
        const AABB& b = n.box;
        if (wrapsSomething(L, b)) {
            const float t = 0.06f;
            out.push_back(neonLightOf(AABB{b.min, {b.min.x + t, b.max.y, b.max.z}}, n.color));
            out.push_back(neonLightOf(AABB{{b.max.x - t, b.min.y, b.min.z}, b.max}, n.color));
            out.push_back(neonLightOf(AABB{b.min, {b.max.x, b.max.y, b.min.z + t}}, n.color));
            out.push_back(neonLightOf(AABB{{b.min.x, b.min.y, b.max.z - t}, b.max}, n.color));
        } else out.push_back(neonLightOf(b, n.color));
    }
    return out;
}

inline float boxDistance(const AABB& b, glm::vec3 p) {
    glm::vec3 q = glm::clamp(p, b.min, b.max);
    return glm::length(q - p);
}

// The MAX_NEON lights nearest `cam`, each faded by how near it is to the cut.
// Facing `fwd` (if given), a light well behind you counts as further away: it
// can only light what's behind you too. (Smoothly, so turning never pops one.)
inline void pickNeon(const std::vector<NeonLight>& all, glm::vec3 cam, NeonSet& out,
                     glm::vec3 fwd = glm::vec3{0.f}, float maxRange = 45.f) {
    static thread_local std::vector<std::pair<float, int>> d;
    d.clear();
    auto sstep = [](float a, float b, float x) { float t = std::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3.f - 2.f * t); };
    for (int i = 0; i < (int)all.size(); ++i) {
        float dist = boxDistance(all[i].box, cam);
        if (glm::dot(fwd, fwd) > 0.f && dist > 5.f) {
            glm::vec3 to = glm::normalize((all[i].box.min + all[i].box.max) * 0.5f - cam);
            dist += 10.f * sstep(0.1f, -0.5f, glm::dot(to, fwd)) * sstep(5.f, 9.f, dist);
        }
        if (dist < maxRange) d.push_back({dist, i});
    }
    int keep = std::min((int)d.size(), MAX_NEON);
    std::partial_sort(d.begin(), d.begin() + std::min((int)d.size(), MAX_NEON + 1), d.end());
    float cut = (int)d.size() > MAX_NEON ? d[MAX_NEON].first : maxRange;
    out.n = 0;
    for (int i = 0; i < MAX_NEON; ++i) { out.mn[i] = out.mx[i] = glm::vec3{1e6f}; out.col[i] = glm::vec3{0.f}; }   // (the shader runs all MAX_NEON)
    for (int k = 0; k < keep; ++k) {
        float t = std::clamp((d[k].first - cut * 0.5f) / (cut * 0.5f + 1e-4f), 0.f, 1.f);
        float w = 1.f - t * t * (3.f - 2.f * t);
        if (w <= 0.f) continue;
        const NeonLight& l = all[d[k].second];
        out.mn[out.n] = l.box.min; out.mx[out.n] = l.box.max; out.col[out.n] = l.color * w;
        ++out.n;
    }
}
