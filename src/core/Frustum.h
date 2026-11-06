#pragma once
#include "core/Collision.h"
#include <glm/glm.hpp>

// Clip planes work without normalization: only the sign of the distance matters.
struct ViewFrustum {
    glm::vec4 p[6];
    explicit ViewFrustum(const glm::mat4& m) {
        const glm::vec4 r0{m[0][0], m[1][0], m[2][0], m[3][0]}, r1{m[0][1], m[1][1], m[2][1], m[3][1]},
                        r2{m[0][2], m[1][2], m[2][2], m[3][2]}, r3{m[0][3], m[1][3], m[2][3], m[3][3]};
        p[0] = r3 + r0; p[1] = r3 - r0; p[2] = r3 + r1; p[3] = r3 - r1; p[4] = r3 + r2; p[5] = r3 - r2;
    }
    bool sees(const AABB& b) const {
        for (const glm::vec4& q : p) {
            const glm::vec3 far{q.x >= 0.f ? b.max.x : b.min.x, q.y >= 0.f ? b.max.y : b.min.y, q.z >= 0.f ? b.max.z : b.min.z};
            if (q.x * far.x + q.y * far.y + q.z * far.z + q.w < 0.f) return false;
        }
        return true;
    }
    bool sees(const AABB& local, const glm::mat4& model) const {
        const glm::vec3 centre = glm::vec3(model * glm::vec4((local.min + local.max) * .5f, 1.f));
        const glm::vec3 half = (local.max - local.min) * .5f;
        // Absolute columns enclose rotation, non-uniform/negative scale and shear.
        // A small margin keeps parts on the clip planes despite rounding.
        const glm::vec3 extent = glm::abs(glm::vec3(model[0])) * half.x + glm::abs(glm::vec3(model[1])) * half.y
                               + glm::abs(glm::vec3(model[2])) * half.z + glm::vec3{.01f};
        return sees({centre - extent, centre + extent});
    }
};
