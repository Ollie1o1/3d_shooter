#pragma once
// =============================================================================
// PropFile.h — level props made in Blender (assets/props/<name>.glb, built by
// tools/blender/props_build.py), the same format as the guns (GunFile.h) with
// no joints: every part rides the root. A part's colour is its material's; one
// that glows ("const", k its colour) is self-lit, baked with the neon; `tex`
// picks a part's surface (else the placement's). Levels place them
// (PlacedModel, Level.h) and WorldGeometry.h bakes them into the static mesh.
// No OpenGL.
// =============================================================================
#include "render/GunFile.h"
#include "core/Collision.h"
#include <map>
#include <string>

namespace propfile {

inline const char* jointName(int) { return "root"; }
inline const char* chanName(int c) { return c == 0 ? "const" : "glow"; }
inline const char* flagName(int) { return "never"; }
inline const gunkit::Names& names() { static const gunkit::Names n{1, 2, 1, jointName, chanName, flagName}; return n; }

struct Prop {
    gunkit::GunModel model;
    glm::vec3 lo{0.f}, hi{0.f};   // its bounds (every part's box)
};

// A prop by name, loaded once (nullptr if there's no such file, or it's broken: the error is printed)
inline const Prop* get(const std::string& name) {
    static std::map<std::string, Prop> cache;
    auto it = cache.find(name);
    if (it == cache.end()) {
        Prop p;
        p.model = gunkit::readGunFile("assets/props/" + name + ".glb", names());
        if (!p.model.error.empty()) std::fprintf(stderr, "prop: %s\n", p.model.error.c_str());
        glm::vec3 lo{1e9f}, hi{-1e9f};
        for (const auto& part : p.model.parts) {
            glm::mat4 b = part.rel;
            if (const meshlib::MeshData* md = meshlib::get(part.mesh)) b = b * glm::scale(glm::mat4(1.f), md->size);
            for (int c = 0; c < 8; ++c) {
                glm::vec3 v = glm::vec3(b * glm::vec4((c & 1) ? 0.5f : -0.5f, (c & 2) ? 0.5f : -0.5f, (c & 4) ? 0.5f : -0.5f, 1.f));
                lo = glm::min(lo, v); hi = glm::max(hi, v);
            }
        }
        p.lo = lo; p.hi = hi;
        it = cache.emplace(name, std::move(p)).first;
    }
    const Prop& p = it->second;
    return p.model.error.empty() && !p.model.parts.empty() ? &p : nullptr;
}

// Where a placed prop's own space goes in the world: fitted into its box (turned by yaw, a
// multiple of 90 degrees: its footprint swaps x and z at 90 / 270), or as placed
inline glm::mat4 placement(const Prop& p, bool free, const glm::mat4& xf, const AABB& fit, float yaw) {
    if (free) return xf;
    const bool quarter = std::fabs(std::sin(yaw)) > 0.5f;
    glm::vec3 size = fit.max - fit.min, own = p.hi - p.lo;
    if (quarter) std::swap(size.x, size.z);
    glm::vec3 s = size / glm::max(own, glm::vec3{1e-4f});
    glm::vec3 base{(fit.min.x + fit.max.x) * 0.5f, fit.min.y, (fit.min.z + fit.max.z) * 0.5f};
    glm::vec3 ownBase{(p.lo.x + p.hi.x) * 0.5f, p.lo.y, (p.lo.z + p.hi.z) * 0.5f};
    return glm::translate(glm::mat4(1.f), base) * glm::rotate(glm::mat4(1.f), yaw, glm::vec3{0.f, 1.f, 0.f})
         * glm::scale(glm::mat4(1.f), s) * glm::translate(glm::mat4(1.f), -ownBase);
}

// Every triangle of a placed prop, in the world, with its corners' normals (a mesh's own, smooth
// where Blender made it smooth; a box's flat): f(a, b, c, na, nb, nc, colour, glows, tex)
template <class F> void forEachTri(const Prop& p, const glm::mat4& world, F f) {
    static const int CUBE[12][3] = {{1,2,6},{1,6,5},{4,7,3},{4,3,0},{3,7,6},{3,6,2},{0,1,5},{0,5,4},{5,6,7},{5,7,4},{0,3,2},{0,2,1}};
    static const glm::vec3 CV[8] = {{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}};
    for (const auto& part : p.model.parts) {
        const glm::mat4 m = world * part.rel;
        const glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(m)));
        const bool flip = glm::determinant(glm::mat3(m)) < 0.f;
        const bool glows = part.chan >= 0;
        const glm::vec3 col = glows ? part.k : part.color;
        auto X = [&](glm::vec3 v) { return glm::vec3(m * glm::vec4(v, 1.f)); };
        auto N = [&](glm::vec3 n) { glm::vec3 w = nm * n; float l = glm::length(w); return l > 1e-9f ? w / l : glm::vec3{0.f, 1.f, 0.f}; };
        auto emit = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 na, glm::vec3 nb, glm::vec3 nc) {
            if (flip) { std::swap(b, c); std::swap(nb, nc); }
            if (glm::length(glm::cross(b - a, c - a)) < 1e-9f) return;
            f(a, b, c, na, nb, nc, col, glows, part.tex);
        };
        if (const meshlib::MeshData* md = meshlib::get(part.mesh)) {
            const bool hasN = md->nrm.size() == md->pos.size();
            for (size_t i = 0; i + 2 < md->idx.size(); i += 3) {
                unsigned i0 = md->idx[i], i1 = md->idx[i + 1], i2 = md->idx[i + 2];
                glm::vec3 a = X(md->pos[i0]), b = X(md->pos[i1]), c = X(md->pos[i2]);
                glm::vec3 fn = glm::normalize(glm::cross(b - a, c - a) + glm::vec3{1e-12f});
                if (flip) fn = -fn;
                emit(a, b, c, hasN ? N(md->nrm[i0]) : fn, hasN ? N(md->nrm[i1]) : fn, hasN ? N(md->nrm[i2]) : fn);
            }
        } else {
            for (auto& t : CUBE) {
                glm::vec3 a = X(CV[t[0]]), b = X(CV[t[1]]), c = X(CV[t[2]]);
                glm::vec3 fn = glm::normalize(glm::cross(b - a, c - a) + glm::vec3{1e-12f});
                if (flip) fn = -fn;
                emit(a, b, c, fn, fn, fn);
            }
        }
    }
}

} // namespace propfile
