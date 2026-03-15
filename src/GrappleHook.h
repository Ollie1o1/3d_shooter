#pragma once
#include <glm/glm.hpp>
#include "Player.h"
#include "ShaderProgram.h"
#include <OpenGL/gl3.h>
#include <algorithm>

class GrappleHook {
public:
    bool      active      = false;
    glm::vec3 target{0.f};
    float     stiffness   = 12.f;
    float     damping     = 3.f;
    float     maxLength   = 20.f;

    GLuint lineVAO = 0, lineVBO = 0;

    GrappleHook() {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    ~GrappleHook() {
        if (lineVAO) glDeleteVertexArrays(1, &lineVAO);
        if (lineVBO) glDeleteBuffers(1, &lineVBO);
    }

    bool fire(glm::vec3 origin, glm::vec3 dir, const Wall* walls, int wallCount) {
        if (active) { active = false; return false; }

        float bestT = maxLength;
        bool  hit   = false;
        for (int i = 0; i < wallCount; ++i) {
            float t = rayAABB(origin, dir, walls[i].box);
            if (t > 0.f && t < bestT) { bestT = t; hit = true; }
        }
        if (hit) {
            target = origin + dir * bestT;
            active = true;
        }
        return hit;
    }

    void release() { active = false; }

    void update(float dt, const glm::vec3& playerPos, glm::vec3& playerVel) {
        if (!active) return;
        glm::vec3 delta = target - playerPos;
        float dist = glm::length(delta);
        if (dist < 0.5f) { active = false; return; }
        glm::vec3 dir   = delta / dist;
        glm::vec3 force = dir * stiffness * dist - playerVel * damping;
        playerVel += force * dt;
    }

    void drawLine(ShaderProgram& shader, const glm::vec3& playerPos,
                  const glm::mat4& /*view*/, const glm::mat4& /*proj*/) {
        if (!active) return;
        float pts[6] = {
            playerPos.x, playerPos.y + 1.4f, playerPos.z,
            target.x,    target.y,            target.z
        };
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pts), pts);

        shader.use();
        shader.setMat4("model", glm::mat4(1.f));
        shader.setVec3("emissiveColor", {0.f, 1.f, 0.5f});
        shader.setVec3("objectColor",   {0.f, 1.f, 0.5f});
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, 2);
        glBindVertexArray(0);
    }

private:
    static float rayAABB(glm::vec3 o, glm::vec3 d, const AABB& b) {
        glm::vec3 invD{1.f/(d.x+1e-9f), 1.f/(d.y+1e-9f), 1.f/(d.z+1e-9f)};
        glm::vec3 t0 = (b.min - o) * invD;
        glm::vec3 t1 = (b.max - o) * invD;
        glm::vec3 tMin = glm::min(t0, t1);
        glm::vec3 tMax = glm::max(t0, t1);
        float tEnter = std::max({tMin.x, tMin.y, tMin.z});
        float tExit  = std::min({tMax.x, tMax.y, tMax.z});
        if (tEnter > tExit || tExit < 0.f) return -1.f;
        return tEnter > 0.f ? tEnter : tExit;
    }
};
