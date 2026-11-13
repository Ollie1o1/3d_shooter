#pragma once
// =============================================================================
// WorldMesh.h — the static level geometry, batched into one mesh per texture,
// and the 1x1 grey texture untextured things are drawn with.
// =============================================================================
#include "render/WorldGeometry.h"
#include "core/Mesh.h"
#include "core/Frustum.h"
#include "core/gl.h"
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstddef>

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

// Contact shadows (contact.vert): the static ones built once, and a second
// buffer refilled each frame for whatever moves (enemies)
struct ContactMesh {
    GLuint vao = 0, vbo = 0;
    int count = 0, capacity = 0;
    // Leaves GL_ARRAY_BUFFER as it found it: other passes fill whatever is bound
    void upload(const std::vector<ContactVert>& v, bool dynamic) {
        GLint prevBuf = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevBuf);
        if (!vao) {
            glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            const GLsizei st = sizeof(ContactVert);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, st, (void*)offsetof(ContactVert, pos));
            glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, st, (void*)offsetof(ContactVert, rect));
            glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, st, (void*)offsetof(ContactVert, strengthReach));
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if ((int)v.size() > capacity || !dynamic) {
            capacity = std::max((int)v.size(), dynamic ? 1536 : 0);
            glBufferData(GL_ARRAY_BUFFER, capacity * sizeof(ContactVert), nullptr, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        if (!v.empty()) glBufferSubData(GL_ARRAY_BUFFER, 0, v.size() * sizeof(ContactVert), v.data());
        count = (int)v.size();
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, (GLuint)prevBuf);
    }
    void draw() const { if (count > 0) { glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES, 0, count); glBindVertexArray(0); } }
    void release() { if (vbo) glDeleteBuffers(1, &vbo); if (vao) glDeleteVertexArrays(1, &vao); vao = vbo = 0; count = capacity = 0; }
};

// The static world, cut into chunks (CHUNK metres a side) so a frame only draws what's in view: each
// texture's triangles are sorted chunk by chunk, so a chunk is one run of its index buffer. The chunks in
// view are drawn nearest first (what's near hides what's behind it before that is shaded), and runs that
// follow on in the buffer are drawn as one.
struct WorldChunked {
    Mesh mesh;
    struct Run { int chunk; GLsizei first, count; };
    std::vector<Run> runs;
    std::vector<int> runOf;   // each chunk's run (-1: none of this texture there)
    bool empty() const { return mesh.indexCount == 0; }
};
struct WorldMeshes {
    WorldChunked byTex[TEX_COUNT];
    WorldChunked neon;
    std::vector<AABB> chunkBox;   // every chunk's bounds (its triangles'), for the view test
    std::vector<uint8_t> chunkLod; // what the chunk holds (WorldGeometry::lod): 0 always drawn, 1 near, 2 far
    ContactMesh contact;
};

namespace worldchunk {
constexpr float CHUNK = 48.f;

// Group a batch's triangles by chunk (by their middle), in chunk order; grow each chunk's box over them
inline void sortBatch(const std::vector<Vertex>& V, std::vector<unsigned int>& I, std::vector<WorldChunked::Run>& runs,
                      std::map<long long, int>& ids, std::vector<AABB>& boxes, const std::vector<uint8_t>& lod, std::vector<uint8_t>& boxLod) {
    const size_t tris = I.size() / 3;
    std::vector<std::pair<int, unsigned int>> order(tris);   // (chunk, triangle)
    for (size_t t = 0; t < tris; ++t) {
        const glm::vec3 a = V[I[t * 3]].position, b = V[I[t * 3 + 1]].position, c = V[I[t * 3 + 2]].position;
        const glm::vec3 mid = (a + b + c) / 3.f;
        const long long cx = (long long)std::floor(mid.x / CHUNK), cy = (long long)std::floor(mid.y / CHUNK), cz = (long long)std::floor(mid.z / CHUNK);
        const uint8_t l = t < lod.size() ? lod[t] : 0;
        const long long key = (((cz + 100000) * 200000LL + (cx + 100000)) * 4096LL + (cy + 2048)) * 4 + l;   // (row by row in Z, then X: neighbours sit together; a model's near and far versions apart)
        auto it = ids.find(key);
        if (it == ids.end()) { it = ids.emplace(key, (int)boxes.size()).first; boxes.push_back({glm::vec3{1e9f}, glm::vec3{-1e9f}}); boxLod.push_back(l); }
        AABB& box = boxes[it->second];
        box.min = glm::min(box.min, glm::min(a, glm::min(b, c)));
        box.max = glm::max(box.max, glm::max(a, glm::max(b, c)));
        order[t] = {it->second, (unsigned int)t};
    }
    std::stable_sort(order.begin(), order.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
    std::vector<unsigned int> sorted;
    sorted.reserve(I.size());
    for (const auto& [chunk, t] : order) {
        if (runs.empty() || runs.back().chunk != chunk) runs.push_back({chunk, (GLsizei)sorted.size(), 0});
        sorted.insert(sorted.end(), {I[t * 3], I[t * 3 + 1], I[t * 3 + 2]});
        runs.back().count += 3;
    }
    I.swap(sorted);
}

// The six planes of the view (each: normal xyz, offset w; inside is >= 0)
using Frustum = ViewFrustum;

// How near a model's chunk must be to draw its full mesh (past it, its stand-in: WorldGeometry farVersion)
inline float& lodDistance() { static float d = 120.f; return d; }

// The chunks in view, nearest first; of a model's two versions, the one for how far its chunk is
inline void inViewOrder(const std::vector<AABB>& boxes, const glm::mat4& viewProj, glm::vec3 eye, bool all, std::vector<int>& out,
                        const std::vector<uint8_t>* lods = nullptr) {
    static std::vector<std::pair<float, int>> near;
    near.clear();
    const Frustum fr(viewProj);
    const float far2 = lodDistance() * lodDistance();
    for (int c = 0; c < (int)boxes.size(); ++c) {
        if (!all && !fr.sees(boxes[c])) continue;
        const glm::vec3 d = glm::max(glm::max(boxes[c].min - eye, eye - boxes[c].max), glm::vec3{0.f});   // (to its nearest point)
        const float d2 = glm::dot(d, d);
        if (lods && c < (int)lods->size()) {
            const uint8_t l = (*lods)[c];
            if ((l == 1 && d2 > far2) || (l == 2 && d2 <= far2)) continue;
        }
        near.push_back({d2, c});
    }
    std::sort(near.begin(), near.end());
    out.clear();
    for (const auto& n : near) out.push_back(n.second);
}

// Draw these chunks' runs, in this order (one that follows on from the last in the buffer: the same draw)
inline void draw(const WorldChunked& w, const std::vector<int>& order) {
    if (w.empty()) return;
    w.mesh.bind();
    GLsizei first = 0, count = 0;
    for (int c : order) {
        const int ri = w.runOf[c];
        if (ri < 0) continue;
        const WorldChunked::Run& r = w.runs[ri];
        if (count > 0 && first + count == r.first) { count += r.count; continue; }
        if (count > 0) w.mesh.drawRange(first, count);
        first = r.first; count = r.count;
    }
    if (count > 0) w.mesh.drawRange(first, count);
    glBindVertexArray(0);
}
}  // namespace worldchunk

static WorldMeshes buildWorldMeshes(const LevelData& L) {
    WorldGeometry G = buildWorldGeometry(L);
    WorldMeshes m;
    std::map<long long, int> ids;
    for (int t = 0; t < TEX_COUNT; ++t) {
        worldchunk::sortBatch(G.V[t], G.I[t], m.byTex[t].runs, ids, m.chunkBox, G.lod[t], m.chunkLod);
        m.byTex[t].mesh.upload(G.V[t], G.I[t]);
    }
    worldchunk::sortBatch(G.nV, G.nI, m.neon.runs, ids, m.chunkBox, G.nLod, m.chunkLod);
    m.neon.mesh.upload(G.nV, G.nI);
    auto index = [&](WorldChunked& w) {
        w.runOf.assign(m.chunkBox.size(), -1);
        for (int i = 0; i < (int)w.runs.size(); ++i) w.runOf[w.runs[i].chunk] = i;
    };
    for (auto& w : m.byTex) index(w);
    index(m.neon);
    m.contact.upload(G.contact, false);
    return m;
}
