#pragma once
// =============================================================================
// PropDraw.h — the small things drawn live from their files (assets/props,
// PropFile.h): pickups, enemy shots, the grapple's claw. Each part into the
// instanced boxes; parts made of "glow*s" take the tint's colour (x s), parts
// glowing on "glow" glow in the tint (x k), on "const" in k.
// =============================================================================
#include "render/PropFile.h"
#include "render/EnemyModel.h"   // (BoxInstance)
#include <vector>

namespace propdraw {
// A prop's parts at m into out; false if there's no file (draw the old boxes then)
inline bool push(std::vector<BoxInstance>& out, const char* name, const glm::mat4& m, glm::vec3 tint, float glow = 1.f) {
    const propfile::Prop* p = propfile::get(name);
    if (!p) return false;
    for (const auto& part : p->model.parts) {
        BoxInstance b;
        b.model = m * part.rel;
        b.color = part.glowScale >= 0.f ? tint * part.glowScale : part.color;
        b.emissive = part.chan == 1 ? tint * part.k * glow : part.chan == 0 ? part.k * glow : glm::vec3{0.f};
        b.mesh = (float)part.mesh;
        out.push_back(b);
    }
    return true;
}
// A frame at p with +Z along dir (a shell's flight, the rope)
inline glm::mat4 along(glm::vec3 p, glm::vec3 dir) {
    float l = glm::length(dir);
    glm::vec3 z = l > 1e-5f ? dir / l : glm::vec3{0.f, 0.f, 1.f};
    glm::vec3 up = std::fabs(z.y) > 0.95f ? glm::vec3{1.f, 0.f, 0.f} : glm::vec3{0.f, 1.f, 0.f};
    glm::vec3 x = glm::normalize(glm::cross(up, z)), y = glm::cross(z, x);
    glm::mat4 m(1.f);
    m[0] = glm::vec4(x, 0.f); m[1] = glm::vec4(y, 0.f); m[2] = glm::vec4(z, 0.f); m[3] = glm::vec4(p, 1.f);
    return m;
}
} // namespace propdraw
