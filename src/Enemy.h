#pragma once
// =============================================================================
// Enemy.h — AI types, per-enemy state machine, and batch renderer.
//
// ENEMY TYPES (add new ones by extending EnemyType and the switch blocks):
//   GRUNT   — 50 hp, charges the player, melee threat
//   SHOOTER — 35 hp, keeps distance and fires slow projectiles (parryable)
//   STALKER — 25 hp, fast, strafes sideways, rushes when close
//
// TO ADD A NEW ENEMY TYPE:
//   1. Add value to EnemyType enum.
//   2. Set its health in the Enemy constructor switch (health = ...).
//   3. Add a case in Enemy::update() — implement movement + attack logic.
//      Return true from update() when the enemy fires a projectile.
//      The caller (GameplayState) will create the projectile at firePos.
//   4. Optionally set a distinct emissiveColor / baseColor in EnemyRenderer::draw().
//
// DAMAGE / HEALING:
//   Call enemy.takeDamage(amount).  It auto-sets state to CHASE and marks the
//   enemy dead when hp reaches 0.  Lifesteal and style rewards are applied by
//   GameplayState after checking the return value of the projectile update.
//
// COLLISION:
//   Enemies reuse the same AABB push-out logic as the player (resolveAABB).
//   getAABB() returns the current bounding box — used by ray tests and
//   ProjectileSystem for hit detection.
// =============================================================================
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Player.h"
#include "Mesh.h"
#include "ShaderProgram.h"
#include <vector>
#include <cmath>
#include <cstdlib>

enum class EnemyType  { GRUNT, SHOOTER, STALKER };
enum class EnemyState { IDLE, CHASE, ATTACK, DEAD };

struct Enemy {
    EnemyType  type;
    EnemyState state = EnemyState::IDLE;
    glm::vec3  position;
    glm::vec3  velocity{0.f};
    float      health;
    float      maxHealth;
    bool       alive = true;
    float      attackTimer  = 0.f;
    float      strafeTimer  = 0.f;
    float      strafeDir    = 1.f;
    int        invincFrames = 0;
    float      hitFlashTimer = 0.f;   // > 0 while flashing white after taking damage

    static constexpr float FLOOR_Y = 0.f;
    static constexpr float RADIUS  = 0.5f;
    static constexpr float HEIGHT  = 1.8f;

    Enemy(EnemyType t, glm::vec3 pos) : type(t), position(pos) {
        switch (t) {
            case EnemyType::GRUNT:   health = maxHealth = 50.f; break;
            case EnemyType::SHOOTER: health = maxHealth = 35.f; break;
            case EnemyType::STALKER: health = maxHealth = 25.f; break;
        }
    }

    void takeDamage(float dmg) {
        if (!alive || invincFrames > 0) return;
        health -= dmg;
        hitFlashTimer = 0.12f;
        if (health <= 0.f) {
            health = 0.f;
            alive  = false;
            state  = EnemyState::DEAD;
        }
        state = EnemyState::CHASE;
    }

    // Returns true if this enemy should fire a projectile this tick.
    // Velocity is in m/s; position integrates with dt — same convention as Player.
    bool update(float dt, const glm::vec3& playerPos, const Wall* walls, int wallCount) {
        if (!alive) return false;
        if (invincFrames > 0) --invincFrames;
        if (hitFlashTimer > 0.f) hitFlashTimer -= dt;

        bool fireProjectile = false;
        glm::vec3 toPlayer = playerPos - position;
        float dist = glm::length(toPlayer);
        glm::vec3 dirToPlayer = dist > 0.001f ? toPlayer / dist : glm::vec3{1,0,0};

        // Velocity is SET directly each tick (not accumulated) to prevent speed runaway.
        // Only gravity and knockback are accumulated. Horizontal speed caps are stable.
        // Perpendicular strafe axis used by GRUNT and STALKER
        glm::vec3 strafeAxis = glm::normalize(glm::vec3{-dirToPlayer.z, 0.f, dirToPlayer.x});

        switch (type) {
            case EnemyType::GRUNT: {
                // Patrol-shooter: wanders with slow strafe movement and fires
                // when the player is in range. Keeps 14-22 m distance — never rushes.
                if (dist < 22.f) state = EnemyState::CHASE;
                if (dist > 30.f) state = EnemyState::IDLE;
                if (state == EnemyState::CHASE) {
                    strafeTimer -= dt;
                    if (strafeTimer <= 0.f) {
                        strafeTimer = 2.0f + (float)(rand() % 150) / 60.f;
                        strafeDir  *= -1.f;
                    }
                    if (dist < 14.f) {
                        // Too close — back away while strafing sideways
                        glm::vec3 mv = glm::normalize(-dirToPlayer + strafeAxis * 0.7f * strafeDir);
                        velocity.x = mv.x * 3.0f;
                        velocity.z = mv.z * 3.0f;
                    } else if (dist > 22.f) {
                        // Too far — close in just enough to be in range
                        velocity.x = dirToPlayer.x * 2.5f;
                        velocity.z = dirToPlayer.z * 2.5f;
                    } else {
                        // In preferred range — strafe sideways only, no approach
                        velocity.x = strafeAxis.x * strafeDir * 2.5f;
                        velocity.z = strafeAxis.z * strafeDir * 2.5f;
                    }
                    attackTimer += dt;
                    if (attackTimer >= 2.2f) {
                        attackTimer = 0.f;
                        fireProjectile = true;
                    }
                }
                break;
            }
            case EnemyType::SHOOTER: {
                // Keeps 10-16 m distance, fires every 2.5 s, backs off if too close.
                if (dist < 18.f) state = EnemyState::CHASE;
                if (dist > 24.f) state = EnemyState::IDLE;
                if (state == EnemyState::CHASE) {
                    if (dist < 10.f) {
                        velocity.x = -dirToPlayer.x * 3.5f;  // back off
                        velocity.z = -dirToPlayer.z * 3.5f;
                    } else if (dist > 16.f) {
                        velocity.x = dirToPlayer.x * 2.5f;   // close in slowly
                        velocity.z = dirToPlayer.z * 2.5f;
                    } else {
                        velocity.x *= std::pow(0.01f, dt);    // hold position
                        velocity.z *= std::pow(0.01f, dt);
                    }
                    attackTimer += dt;
                    if (attackTimer >= 2.5f) {
                        attackTimer = 0.f;
                        fireProjectile = true;
                    }
                }
                break;
            }
            case EnemyType::STALKER: {
                // Fast and aggressive: strafes in wide arcs, charges at close range.
                if (dist < 18.f) state = EnemyState::CHASE;
                if (dist > 24.f) state = EnemyState::IDLE;
                if (state == EnemyState::CHASE) {
                    strafeTimer -= dt;
                    if (strafeTimer <= 0.f) {
                        strafeTimer = 1.2f + (float)(rand() % 100) / 60.f;
                        strafeDir  *= -1.f;
                    }
                    float speed = (dist < 5.f) ? 6.5f : 5.f;
                    glm::vec3 moveVec = dirToPlayer + strafeAxis * 0.7f * strafeDir;
                    float mvLen = glm::length(moveVec);
                    if (mvLen > 0.001f) {
                        velocity.x = (moveVec.x / mvLen) * speed;
                        velocity.z = (moveVec.z / mvLen) * speed;
                    }
                }
                break;
            }
        }

        // Bleed off horizontal velocity quickly when idle (e.g. after knockback)
        if (state == EnemyState::IDLE) {
            velocity.x *= std::pow(0.01f, dt);
            velocity.z *= std::pow(0.01f, dt);
        }

        // Apply gravity (only when airborne)
        if (position.y > FLOOR_Y) velocity.y += -24.f * dt;

        // Integrate — velocity in m/s, matches Player convention
        position += velocity * dt;
        if (position.y < FLOOR_Y) { position.y = FLOOR_Y; velocity.y = 0.f; }

        // Wall collision
        for (int i = 0; i < wallCount; ++i) resolveAABB(walls[i].box);

        return fireProjectile;
    }

    glm::vec3 getFireDir(const glm::vec3& playerPos) const {
        glm::vec3 d = playerPos - position;
        float len = glm::length(d);
        return len > 0.001f ? d / len : glm::vec3{0,0,1};
    }

    AABB getAABB() const {
        return { position + glm::vec3{-RADIUS,0,-RADIUS},
                 position + glm::vec3{ RADIUS, HEIGHT, RADIUS} };
    }

private:
    void resolveAABB(const AABB& wall) {
        glm::vec3 pMin = position + glm::vec3{-RADIUS,0,-RADIUS};
        glm::vec3 pMax = position + glm::vec3{ RADIUS,HEIGHT, RADIUS};
        if (pMax.x <= wall.min.x || pMin.x >= wall.max.x) return;
        if (pMax.y <= wall.min.y || pMin.y >= wall.max.y) return;
        if (pMax.z <= wall.min.z || pMin.z >= wall.max.z) return;
        float ox = std::min(pMax.x-wall.min.x, wall.max.x-pMin.x);
        float oy = std::min(pMax.y-wall.min.y, wall.max.y-pMin.y);
        float oz = std::min(pMax.z-wall.min.z, wall.max.z-pMin.z);
        if (ox <= oy && ox <= oz) {
            float dir = (position.x < (wall.min.x+wall.max.x)*0.5f) ? -1.f : 1.f;
            position.x += dir * ox;
        } else if (oy <= ox && oy <= oz) {
            float dir = (position.y < (wall.min.y+wall.max.y)*0.5f) ? -1.f : 1.f;
            position.y += dir * oy;
        } else {
            float dir = (position.z < (wall.min.z+wall.max.z)*0.5f) ? -1.f : 1.f;
            position.z += dir * oz;
        }
    }
};

// Batch-render all enemies as colored cubes
class EnemyRenderer {
public:
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;

    EnemyRenderer() {
        std::vector<Vertex> verts;
        std::vector<unsigned int> idx;
        float h = 0.9f, r = 0.5f;
        auto pushFace = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n) {
            unsigned int base = (unsigned int)verts.size();
            verts.push_back({a,{0,0},n,{1,1,1}}); verts.push_back({b,{1,0},n,{1,1,1}});
            verts.push_back({c,{1,1},n,{1,1,1}}); verts.push_back({d,{0,1},n,{1,1,1}});
            idx.insert(idx.end(),{base,base+1,base+2,base,base+2,base+3});
        };
        pushFace({-r,0,r},{r,0,r},{r,h*2,r},{-r,h*2,r},{0,0,1});
        pushFace({r,0,-r},{-r,0,-r},{-r,h*2,-r},{r,h*2,-r},{0,0,-1});
        pushFace({r,0,r},{r,0,-r},{r,h*2,-r},{r,h*2,r},{1,0,0});
        pushFace({-r,0,-r},{-r,0,r},{-r,h*2,r},{-r,h*2,-r},{-1,0,0});
        pushFace({-r,h*2,r},{r,h*2,r},{r,h*2,-r},{-r,h*2,-r},{0,1,0});
        pushFace({-r,0,-r},{r,0,-r},{r,0,r},{-r,0,r},{0,-1,0});

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size()*sizeof(Vertex)), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idx.size()*sizeof(unsigned int)), idx.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));
        glBindVertexArray(0);
        indexCount = (GLsizei)idx.size();
    }

    ~EnemyRenderer() {
        if (vao) glDeleteVertexArrays(1,&vao);
        if (vbo) glDeleteBuffers(1,&vbo);
        if (ebo) glDeleteBuffers(1,&ebo);
    }

    void draw(ShaderProgram& shader, const std::vector<Enemy>& enemies) {
        glBindVertexArray(vao);
        for (auto& e : enemies) {
            if (!e.alive) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.f), e.position);
            shader.setMat4("model", model);
            glm::vec3 emissive{0,0,0};
            glm::vec3 baseColor{1,1,1};
            switch (e.type) {
                case EnemyType::GRUNT:   baseColor={0.8f,0.2f,0.2f}; break;
                case EnemyType::SHOOTER: baseColor={0.2f,0.3f,0.9f}; emissive={0.1f,0.2f,0.8f}; break;
                case EnemyType::STALKER: baseColor={0.8f,0.6f,0.0f}; break;
            }
            // Hit flash — briefly bleach the enemy white on damage
            if (e.hitFlashTimer > 0.f) {
                float t = e.hitFlashTimer / 0.12f;
                baseColor = glm::mix(baseColor, glm::vec3{1.f,1.f,1.f}, t * 0.85f);
                emissive  = glm::mix(emissive,  glm::vec3{0.8f,0.8f,0.8f}, t * 0.6f);
            }
            shader.setVec3("emissiveColor", emissive);
            shader.setVec3("objectColor",   baseColor);
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glBindVertexArray(0);
    }
};
