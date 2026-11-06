#pragma once
// =============================================================================
// BoxRenderer.h — draws any number of unit cubes in ONE instanced draw call.
//
// Enemies (built from box rigs in EnemyModel.h), death debris, doors, jump
// pads, pickups, spawn beams, shockwaves and the reactor are all just lists
// of BoxInstance, so everything that moves in the world costs one draw call.
// GameplayState sets the lighting/fog uniforms on `shader` before draw().
// A part that isn't a box (BoxInstance::mesh > 0) is a mesh from MeshLib.h:
// each mesh is one more instanced call, sharing the same instance buffer.
// =============================================================================
#include "core/gl.h"
#include "core/Mesh.h"
#include "core/ShaderProgram.h"
#include "core/Frustum.h"
#include "render/EnemyModel.h"   // BoxInstance
#include "render/MeshLib.h"
#include <vector>
#include <cstddef>
#include <algorithm>

class BoxRenderer {
public:
    ShaderProgram shader;
    GLuint  vao = 0, vbo = 0, ebo = 0, instanceVBO = 0;
    GLsizei indexCount = 0;
    static constexpr size_t INITIAL_INSTANCES = 16384;

    BoxRenderer(const char* vert = "src/shaders/box_inst.vert", const char* frag = "src/shaders/box_inst.frag", const char* defines = "", size_t capacity = INITIAL_INSTANCES)
        : instanceCapacity(std::max(size_t{1}, capacity)) {
        shader.loadFiles(vert, frag, defines);

        // Unit cube centred on the origin, 0..1 UVs per face (for edge shading)
        std::vector<Vertex> verts;
        std::vector<unsigned int> idx;
        auto face = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n) {
            unsigned int base = (unsigned int)verts.size();
            verts.push_back({a,{0,0},n}); verts.push_back({b,{1,0},n});
            verts.push_back({c,{1,1},n}); verts.push_back({d,{0,1},n});
            idx.insert(idx.end(), {base,base+1,base+2, base,base+2,base+3});
        };
        float h = 0.5f;
        face({-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h},{ 0, 0, 1});
        face({ h,-h,-h},{-h,-h,-h},{-h, h,-h},{ h, h,-h},{ 0, 0,-1});
        face({ h,-h, h},{ h,-h,-h},{ h, h,-h},{ h, h, h},{ 1, 0, 0});
        face({-h,-h,-h},{-h,-h, h},{-h, h, h},{-h, h,-h},{-1, 0, 0});
        face({-h, h, h},{ h, h, h},{ h, h,-h},{-h, h,-h},{ 0, 1, 0});
        face({-h,-h,-h},{ h,-h,-h},{ h,-h, h},{-h,-h, h},{ 0,-1, 0});
        indexCount = (GLsizei)idx.size();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &instanceVBO);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size()*sizeof(Vertex)), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idx.size()*sizeof(unsigned int)), idx.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));

        // Per-instance attributes: mat4 at 4-7, colour at 8, emissive at 9
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceCapacity * sizeof(BoxInstance), nullptr, GL_DYNAMIC_DRAW);
        instanceAttribs(0);
        glBindVertexArray(0);
    }

    // The instance attributes of the bound VAO, reading the instance buffer from instance `first` on
    void instanceAttribs(size_t first) {
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        size_t base = first * sizeof(BoxInstance);
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(4 + i);
            glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(BoxInstance),
                                  (void*)(base + offsetof(BoxInstance, model) + i * 16));
            glVertexAttribDivisor(4 + i, 1);
        }
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(BoxInstance), (void*)(base + offsetof(BoxInstance, color)));
        glVertexAttribDivisor(8, 1);
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(BoxInstance), (void*)(base + offsetof(BoxInstance, emissive)));
        glVertexAttribDivisor(9, 1);
        glEnableVertexAttribArray(10);
        glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, sizeof(BoxInstance), (void*)(base + offsetof(BoxInstance, surface)));
        glVertexAttribDivisor(10, 1);
    }

    BoxRenderer(const BoxRenderer&) = delete;
    BoxRenderer& operator=(const BoxRenderer&) = delete;

    ~BoxRenderer() {
        for (auto& g : gpu) { if (g.vao) glDeleteVertexArrays(1, &g.vao); if (g.vbo) glDeleteBuffers(1, &g.vbo); if (g.ebo) glDeleteBuffers(1, &g.ebo); }
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (ebo) glDeleteBuffers(1, &ebo);
        if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    }

    // Keep simulation and rig generation intact; only skip GPU work for parts
    // wholly outside the camera. Bounds come from the actual mesh vertices.
    size_t drawVisible(const std::vector<BoxInstance>& boxes, const glm::mat4& viewProj) {
        const ViewFrustum frustum(viewProj);
        while (meshBounds.size() < (size_t)meshlib::count()) {
            const auto& md = *meshlib::get((int)meshBounds.size() + 1);
            AABB b{glm::vec3{0.f}, glm::vec3{0.f}};
            if (!md.pos.empty()) {
                b.min = b.max = md.pos.front();
                for (const auto& p : md.pos) { b.min = glm::min(b.min, p); b.max = glm::max(b.max, p); }
            }
            meshBounds.push_back(b);
        }
        visible.clear();
        const AABB cube{glm::vec3{-.5f}, glm::vec3{.5f}};
        const int shapes = meshlib::count() + 1;
        for (const auto& b : boxes) {
            const int shape = shapeOf(b, shapes);
            if (frustum.sees(shape ? meshBounds[(size_t)shape - 1] : cube, b.model)) visible.push_back(b);
        }
        draw(visible);
        return visible.size();
    }

    // Ordered batches preserve depth ties in the hands/weapons while joining
    // adjacent parts of the same shape. All instance data is uploaded once.
    void draw(const std::vector<BoxInstance>& boxes, bool ordered = false) {
        lastDrawCalls = 0;
        int count = (int)boxes.size();
        if (count == 0) return;
        // Story dressing can exceed the initial allocation. Never discard the
        // tail: enemies, held weapons and pickups are gathered after the bodies.
        if(boxes.size()>instanceCapacity) {
            while(instanceCapacity<boxes.size())instanceCapacity*=2;
            glBindBuffer(GL_ARRAY_BUFFER,instanceVBO);
            glBufferData(GL_ARRAY_BUFFER,instanceCapacity*sizeof(BoxInstance),nullptr,GL_DYNAMIC_DRAW);
        }
        bool meshes = false;
        for (int i = 0; i < count && !meshes; ++i) meshes = boxes[i].mesh > 0.5f;
        shader.use();
        if (!meshes) {   // (the usual case: all boxes, one call)
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(BoxInstance), boxes.data());
            glBindVertexArray(vao);
            instanceAttribs(0);
            glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, count);
            ++lastDrawCalls;
            glBindVertexArray(0);
            return;
        }
        if (ordered) {
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(BoxInstance), boxes.data());
            const int shapes = meshlib::count() + 1;
            for (int first = 0; first < count;) {
                const int shape = shapeOf(boxes[first], shapes);
                int end = first + 1;
                while (end < count && shapeOf(boxes[end], shapes) == shape) ++end;
                GLuint v = vao; GLsizei ic = indexCount;
                if (shape > 0) { const auto& g = upload(shape); v = g.vao; ic = g.count; }
                glBindVertexArray(v);
                instanceAttribs((size_t)first);
                glDrawElementsInstanced(GL_TRIANGLES, ic, GL_UNSIGNED_INT, nullptr, end - first);
                ++lastDrawCalls;
                first = end;
            }
            glBindVertexArray(0);
            return;
        }
        // Grouped by shape (a counting sort), uploaded once, then one call per shape
        int shapes = meshlib::count() + 1;
        groupStart.assign((size_t)shapes + 1, 0);
        for (int i = 0; i < count; ++i) ++groupStart[(size_t)shapeOf(boxes[i], shapes) + 1];
        for (int k = 0; k < shapes; ++k) groupStart[(size_t)k + 1] += groupStart[(size_t)k];
        sorted.resize((size_t)count);
        fill.assign(groupStart.begin(), groupStart.end() - 1);
        for (int i = 0; i < count; ++i) sorted[(size_t)fill[(size_t)shapeOf(boxes[i], shapes)]++] = boxes[i];
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(BoxInstance), sorted.data());
        for (int k = 0; k < shapes; ++k) {
            int from = groupStart[(size_t)k], n = groupStart[(size_t)k + 1] - from;
            if (n == 0) continue;
            GLuint v = vao; GLsizei ic = indexCount;
            if (k > 0) { const Gpu& g = upload(k); v = g.vao; ic = g.count; }
            glBindVertexArray(v);
            instanceAttribs((size_t)from);
            glDrawElementsInstanced(GL_TRIANGLES, ic, GL_UNSIGNED_INT, nullptr, n);
            ++lastDrawCalls;
        }
        glBindVertexArray(vao);
        instanceAttribs(0);
        glBindVertexArray(0);
    }

    int lastDrawCalls = 0;

private:
    size_t instanceCapacity = INITIAL_INSTANCES;
    struct Gpu { GLuint vao = 0, vbo = 0, ebo = 0; GLsizei count = 0; };
    std::vector<Gpu> gpu;                 // [id - 1], uploaded the first time each mesh is drawn
    std::vector<BoxInstance> sorted;
    std::vector<BoxInstance> visible;
    std::vector<AABB> meshBounds;
    std::vector<int> groupStart, fill;

    static int shapeOf(const BoxInstance& b, int shapes) {
        int k = (int)(b.mesh + 0.5f);
        return k > 0 && k < shapes ? k : 0;
    }
    const Gpu& upload(int id) {
        if ((int)gpu.size() < id) gpu.resize((size_t)id);
        Gpu& g = gpu[(size_t)id - 1];
        if (g.vao) return g;
        const meshlib::MeshData& md = *meshlib::get(id);
        std::vector<Vertex> verts;
        verts.reserve(md.pos.size());
        // uv 0.5: no seams darkening its edges (those are for boxes)
        for (size_t i = 0; i < md.pos.size(); ++i) verts.push_back({md.pos[i], {0.5f, 0.5f}, md.nrm[i]});
        glGenVertexArrays(1, &g.vao);
        glGenBuffers(1, &g.vbo);
        glGenBuffers(1, &g.ebo);
        glBindVertexArray(g.vao);
        glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(Vertex)), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(md.idx.size() * sizeof(uint32_t)), md.idx.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));
        instanceAttribs(0);
        glBindVertexArray(0);
        g.count = (GLsizei)md.idx.size();
        return g;
    }
public:
};
