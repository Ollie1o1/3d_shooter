#pragma once
// =============================================================================
// BoxRenderer.h — draws any number of unit cubes in ONE instanced draw call.
//
// Enemies (built from box rigs in EnemyModel.h), death debris, doors, jump
// pads, pickups, spawn beams, shockwaves and the reactor are all just lists
// of BoxInstance, so everything that moves in the world costs one draw call.
// GameplayState sets the lighting/fog uniforms on `shader` before draw().
// =============================================================================
#include "gl.h"
#include "Mesh.h"
#include "ShaderProgram.h"
#include "EnemyModel.h"   // BoxInstance
#include <vector>
#include <cstddef>
#include <algorithm>

class BoxRenderer {
public:
    ShaderProgram shader;
    GLuint  vao = 0, vbo = 0, ebo = 0, instanceVBO = 0;
    GLsizei indexCount = 0;
    static constexpr int MAX_INSTANCES = 4096;

    BoxRenderer() {
        shader.loadFiles("src/box_inst.vert", "src/box_inst.frag");

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
        glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(BoxInstance), nullptr, GL_DYNAMIC_DRAW);
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(4 + i);
            glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(BoxInstance),
                                  (void*)(offsetof(BoxInstance, model) + i * 16));
            glVertexAttribDivisor(4 + i, 1);
        }
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(BoxInstance), (void*)offsetof(BoxInstance, color));
        glVertexAttribDivisor(8, 1);
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(BoxInstance), (void*)offsetof(BoxInstance, emissive));
        glVertexAttribDivisor(9, 1);
        glBindVertexArray(0);
    }

    BoxRenderer(const BoxRenderer&) = delete;
    BoxRenderer& operator=(const BoxRenderer&) = delete;

    ~BoxRenderer() {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (ebo) glDeleteBuffers(1, &ebo);
        if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    }

    void draw(const std::vector<BoxInstance>& boxes) {
        int count = std::min((int)boxes.size(), MAX_INSTANCES);
        if (count == 0) return;
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(BoxInstance), boxes.data());
        shader.use();
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, count);
        glBindVertexArray(0);
    }
};
