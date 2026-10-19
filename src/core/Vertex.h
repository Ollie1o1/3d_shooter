#pragma once
// One vertex of the static world mesh (and of Mesh in general). No OpenGL here,
// so the CPU side of the world geometry can be tested headlessly.
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position; // location 0 in shader
    glm::vec2 uv;       // location 1 — texture coordinates (0..1 range per tile)
    glm::vec3 normal;   // location 2 — used for lighting calculations
    glm::vec3 color = {1.f, 1.f, 1.f}; // location 3 — per-vertex surface tint, default white
};
