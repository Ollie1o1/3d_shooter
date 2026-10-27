#pragma once
// =============================================================================
// MeshLib.h — the shapes that aren't boxes: meshes read from a model file
// (a bevelled plate, a cannon's barrel), kept on the CPU, numbered from 1.
// Mesh 0 is the unit cube every box is drawn with. BoxRenderer uploads each
// mesh the first time it's drawn; a model reloaded in the viewer adds new
// ones (the old stay, unused: a few KB). No OpenGL here.
//
// A mesh is centred on its bounds, so a part's transform puts its centre
// (debris spins about it) and the mesh's own size is its size.
// =============================================================================
#include <glm/glm.hpp>
#include <cstdint>
#include <deque>
#include <vector>

namespace meshlib {

struct MeshData {
    std::vector<glm::vec3> pos, nrm;   // one normal per vertex (flat faces: split vertices)
    std::vector<uint32_t>  idx;        // triangles
    glm::vec3 size{0.f};               // its bounds
};

inline std::deque<MeshData>& all() { static std::deque<MeshData> M; return M; }   // [0] is mesh 1
inline int add(MeshData m) { all().push_back(std::move(m)); return (int)all().size(); }
inline const MeshData* get(int id) { return id >= 1 && id <= (int)all().size() ? &all()[(size_t)id - 1] : nullptr; }
inline int count() { return (int)all().size(); }

// Recentre on its bounds; returns the centre it had
inline glm::vec3 centre(MeshData& m) {
    if (m.pos.empty()) return glm::vec3(0.f);
    glm::vec3 lo{1e30f}, hi{-1e30f};
    for (auto& p : m.pos) { lo = glm::min(lo, p); hi = glm::max(hi, p); }
    glm::vec3 c = (lo + hi) * 0.5f;
    for (auto& p : m.pos) p -= c;
    m.size = hi - lo;
    return c;
}
// Mirrored in X (a negative scale in the file): x flipped, each triangle wound back
inline MeshData mirrorX(MeshData m) {
    for (auto& p : m.pos) p.x = -p.x;
    for (auto& n : m.nrm) n.x = -n.x;
    for (size_t i = 0; i + 2 < m.idx.size(); i += 3) std::swap(m.idx[i + 1], m.idx[i + 2]);
    return m;
}
// Flat normals for a mesh that came without any
inline void flatNormals(MeshData& m) {
    MeshData f; f.size = m.size;
    for (size_t i = 0; i + 2 < m.idx.size(); i += 3) {
        glm::vec3 a = m.pos[m.idx[i]], b = m.pos[m.idx[i + 1]], c = m.pos[m.idx[i + 2]];
        glm::vec3 n = glm::cross(b - a, c - a);
        float l = glm::length(n);
        n = l > 1e-12f ? n / l : glm::vec3(0.f, 1.f, 0.f);
        for (glm::vec3 p : {a, b, c}) { f.idx.push_back((uint32_t)f.pos.size()); f.pos.push_back(p); f.nrm.push_back(n); }
    }
    m = std::move(f);
}

} // namespace meshlib
