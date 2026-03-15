#pragma once
#include <vector>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>

// =============================================================================
// Vertex layout
// =============================================================================
// Every vertex sent to the GPU has three attributes. The layout matches the
// attribute locations declared in shader.vert (layout(location = N)).
// If you add a new attribute (e.g., vertex color, tangent), add it here AND
// in the shader, then add a new glVertexAttribPointer call in Mesh::upload().
struct Vertex {
    glm::vec3 position; // location 0 in shader
    glm::vec2 uv;       // location 1 — texture coordinates (0..1 range per tile)
    glm::vec3 normal;   // location 2 — used for lighting calculations
    glm::vec3 color = {1.f, 1.f, 1.f}; // location 3 — per-vertex surface tint, default white
};

// =============================================================================
// Mesh
// =============================================================================
// Wraps a VAO (Vertex Array Object) + VBO (vertex buffer) + EBO (index buffer).
//
// VAO: remembers the vertex layout (which buffer, what stride, what offset).
//      Binding a VAO is all you need before drawing — no need to re-describe
//      the layout every frame.
// VBO: the actual vertex data sitting on the GPU.
// EBO: index buffer — lets vertices be shared between triangles to save memory.
//
// PERFORMANCE NOTE:
//   Draw calls are expensive. The goal is to batch as much geometry as possible
//   into a single Mesh so it can be drawn with one glDrawElements call.
//   In main.cpp, the entire world (floor, ceiling, all walls) is one Mesh.
//   When you add enemies or props, batch each "type" into its own Mesh and
//   draw all instances with a single call (or use instancing for many copies).
//
// HOW TO USE:
//   1. Build std::vector<Vertex> and std::vector<unsigned int> (indices)
//   2. Call mesh.upload(vertices, indices) — this sends data to the GPU once
//   3. Call mesh.draw() every frame — cheap, just binds VAO and calls glDrawElements
//
// HOW TO EXTEND:
//   - Dynamic geometry (e.g., animated mesh): use GL_DYNAMIC_DRAW in
//     glBufferData and call glBufferSubData each frame to update a sub-range
//   - Instanced rendering (many identical objects): store per-instance data
//     (transform matrices) in a second VBO and use glDrawElementsInstanced
// =============================================================================

class Mesh {
public:
    GLuint  vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
    bool    hasIndices = false;

    Mesh() = default;
    Mesh(const Mesh&) = delete;             // no copying — GPU resources are unique
    Mesh& operator=(const Mesh&) = delete;

    // Move constructor so we can return Mesh from functions (like buildWorld)
    Mesh(Mesh&& o) noexcept
        : vao(o.vao), vbo(o.vbo), ebo(o.ebo),
          indexCount(o.indexCount), hasIndices(o.hasIndices)
    {
        o.vao = o.vbo = o.ebo = 0; // prevent double-free in destructor
    }

    // Move assignment operator
    Mesh& operator=(Mesh&& o) noexcept {
        if (this != &o) {
            destroy();
            vao = o.vao; vbo = o.vbo; ebo = o.ebo;
            indexCount = o.indexCount; hasIndices = o.hasIndices;
            o.vao = o.vbo = o.ebo = 0;
        }
        return *this;
    }

    ~Mesh() { destroy(); }

    // -------------------------------------------------------------------------
    // Upload geometry to the GPU. Call ONCE after building your vertex arrays.
    // After this, the CPU-side vectors can be freed — the GPU has its own copy.
    // -------------------------------------------------------------------------
    void upload(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices)
    {
        destroy(); // clean up any previous upload
        hasIndices = true;
        indexCount = static_cast<GLsizei>(indices.size());

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao); // start recording layout into this VAO

        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(Vertex),
                     vertices.data(),
                     GL_STATIC_DRAW); // STATIC = uploaded once, drawn many times

        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned int),
                     indices.data(),
                     GL_STATIC_DRAW);

        // Tell the VAO how to interpret the raw bytes in the VBO.
        // Each glVertexAttribPointer call describes one attribute:
        //   (location, count, type, normalize, stride, byte-offset-in-struct)

        // Attribute 0: position (vec3, 12 bytes)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, position));

        // Attribute 1: UV coordinates (vec2, 8 bytes)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, uv));

        // Attribute 2: normal (vec3, 12 bytes)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, normal));

        // Attribute 3: per-vertex color (vec3, 12 bytes)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, color));

        glBindVertexArray(0); // done recording
    }

    // -------------------------------------------------------------------------
    // Issue a single draw call. Bind the VAO, draw all triangles, unbind.
    // This is the only function you call every frame — everything else was
    // set up once in upload().
    // -------------------------------------------------------------------------
    void draw() const {
        glBindVertexArray(vao);
        if (hasIndices)
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        else
            glDrawArrays(GL_TRIANGLES, 0, indexCount);
        glBindVertexArray(0);
    }

private:
    void destroy() {
        if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo) { glDeleteBuffers(1, &vbo);       vbo = 0; }
        if (ebo) { glDeleteBuffers(1, &ebo);       ebo = 0; }
    }
};
