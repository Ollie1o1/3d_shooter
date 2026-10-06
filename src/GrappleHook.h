#pragma once
#include <glm/glm.hpp>
#include "Player.h"
#include "ShaderProgram.h"
#include "gl.h"
#include <algorithm>

class GrappleHook {
public:
    bool      active      = false;
    glm::vec3 target{0.f};
    float     maxLength   = 50.f;

    GLuint       lineVAO = 0, lineVBO = 0;
    ShaderProgram lineShader; // dedicated dithered-line shader

    GrappleHook() {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
        glBindVertexArray(0);

        lineShader.loadFiles("src/grapple.vert", "src/grapple.frag");
    }

    ~GrappleHook() {
        if (lineVAO) glDeleteVertexArrays(1, &lineVAO);
        if (lineVBO) glDeleteBuffers(1, &lineVBO);
    }

    // Hook onto `point` (GameplayState picks it with a raycast). If the point
    // is on a moving platform, pass its wall index and current box: the anchor
    // then rides along with the platform (see follow()). Writes an immediate
    // launch impulse into outImpulse for the caller to apply to the player.
    void attach(glm::vec3 origin, glm::vec3 point, glm::vec3& outImpulse,
                int wall = -1, const AABB* box = nullptr) {
        target = point;
        active = true;
        hookedWall = wall;
        moverWall = box ? wall : -1;
        if (box) anchorLocal = point - box->min;
        // Strong immediate burst — overrides current velocity so movement is
        // felt the instant you fire. Min Y ensures you always gain height.
        glm::vec3 toTarget = glm::normalize(target - origin);
        outImpulse = toTarget * 32.f;
        outImpulse.y = std::max(outImpulse.y, 12.f);
    }

    // Keep the anchor on a moving platform
    void follow(const AABB& box) { if (active && moverWall >= 0) target = box.min + anchorLocal; }

    int       moverWall = -1;       // wall index of the platform we're hooked to, or -1
    int       hookedWall = -1;      // wall index of whatever we're hooked to (a PENITENT's chain anchor: rip it out)
    glm::vec3 anchorLocal{0.f};

    void release() { active = false; moverWall = -1; hookedWall = -1; }

    // Called every physics tick while active. Strongly pulls toward anchor +
    // cancels gravity entirely so you fly straight at the attachment point.
    void update(float dt, const glm::vec3& playerPos, glm::vec3& playerVel) {
        if (!active) return;
        glm::vec3 delta = target - playerPos;
        float dist = glm::length(delta);
        if (dist < 2.0f) { active = false; return; }
        glm::vec3 dir = delta / dist;

        // Strong continued pull — enough to clearly accelerate toward target.
        playerVel += dir * 55.f * dt;

        // Cancel gravity entirely (gravity = -24 m/s²) so grappling upward works.
        playerVel.y += 24.f * dt;

        // Higher speed cap so long-range grapples build real momentum.
        float spd = glm::length(playerVel);
        if (spd > 45.f) playerVel *= 45.f / spd;
    }

    // Draw the rope and anchor cross using PSX-style dithered transparency.
    // view/proj are needed by the grapple shader; no other shader state is touched.
    void drawLine(const glm::vec3& playerPos,
                  const glm::mat4& view, const glm::mat4& proj) {
        if (!active) return;

        lineShader.use();
        lineShader.setMat4("view",       view);
        lineShader.setMat4("projection", proj);

        glm::vec3 origin = playerPos + glm::vec3{0.f, 1.4f, 0.f};

        // Dithered rope at ~70% opacity (PSX look — every pixel either solid or
        // discarded based on screen-space Bayer matrix in the frag shader).
        lineShader.setVec3 ("uColor", {0.5f, 1.0f, 1.0f});
        lineShader.setFloat("uAlpha", 0.70f);

        // Two parallel strands offset along world-up for visual thickness.
        static const glm::vec3 offsets[] = {
            {0.f,  0.025f, 0.f},
            {0.f, -0.025f, 0.f},
        };
        for (auto& off : offsets) {
            float pts[6] = {
                origin.x+off.x, origin.y+off.y, origin.z+off.z,
                target.x+off.x, target.y+off.y, target.z+off.z
            };
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pts), pts);
            glBindVertexArray(lineVAO);
            glDrawArrays(GL_LINES, 0, 2);
        }

        // Anchor cross — fully solid (alpha 1.0 skips dither entirely).
        lineShader.setVec3 ("uColor", {1.f, 1.f, 0.4f});
        lineShader.setFloat("uAlpha", 1.0f);
        float cr = 0.25f;
        const float crosses[3][6] = {
            { target.x-cr, target.y,    target.z,    target.x+cr, target.y,    target.z    },
            { target.x,    target.y-cr, target.z,    target.x,    target.y+cr, target.z    },
            { target.x,    target.y,    target.z-cr, target.x,    target.y,    target.z+cr },
        };
        for (auto& c : crosses) {
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c), c);
            glBindVertexArray(lineVAO);
            glDrawArrays(GL_LINES, 0, 2);
        }

        glBindVertexArray(0);
    }

};
