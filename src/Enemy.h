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
#include <vector>
#include "Player.h"  // also brings in SpatialGrid, AABB, Wall
#include "Mesh.h"
#include "ShaderProgram.h"
#include <vector>
#include <cmath>
#include <cstdlib>

enum class EnemyType  { GRUNT, SHOOTER, STALKER, FLYER };
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
    float      hoverY       = 0.f;   // target hover altitude for FLYER type

    static constexpr float FLOOR_Y = 0.f;
    static constexpr float RADIUS  = 0.5f;
    static constexpr float HEIGHT  = 1.8f;

    Enemy(EnemyType t, glm::vec3 pos) : type(t), position(pos) {
        switch (t) {
            case EnemyType::GRUNT:   health = maxHealth = 50.f; break;
            case EnemyType::SHOOTER: health = maxHealth = 35.f; break;
            case EnemyType::STALKER: health = maxHealth = 25.f; break;
            case EnemyType::FLYER:
                health = maxHealth = 40.f;
                hoverY = pos.y > 1.f ? pos.y : 10.f;
                break;
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

    bool update(float dt, const glm::vec3& playerPos,
                const Wall* walls, int wallCount,
                const SpatialGrid* grid = nullptr) {
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
            case EnemyType::FLYER: {
                // Hovers at fixed altitude, circles the player horizontally,
                // and fires downward-angled shots at a fast rate.
                if (dist < 40.f) state = EnemyState::CHASE;
                if (dist > 55.f) state = EnemyState::IDLE;
                if (state == EnemyState::CHASE) {
                    strafeTimer -= dt;
                    if (strafeTimer <= 0.f) {
                        strafeTimer = 1.5f + (float)(rand() % 100) / 50.f;
                        strafeDir  *= -1.f;
                    }
                    // Horizontal: orbit around the player
                    glm::vec3 orbDir = dirToPlayer + strafeAxis * strafeDir * 0.9f;
                    float orbLen = glm::length(orbDir);
                    if (orbLen > 0.001f) orbDir /= orbLen;
                    velocity.x = orbDir.x * 7.f;
                    velocity.z = orbDir.z * 7.f;
                    // Vertical: servo toward hover altitude
                    float yErr = hoverY - position.y;
                    velocity.y = glm::clamp(yErr * 6.f, -12.f, 12.f);

                    attackTimer += dt;
                    if (attackTimer >= 1.6f) {
                        attackTimer = 0.f;
                        fireProjectile = true;
                    }
                } else {
                    // Idle: drift back toward hover altitude
                    float yErr = hoverY - position.y;
                    velocity.y = glm::clamp(yErr * 4.f, -6.f, 6.f);
                    velocity.x *= std::pow(0.01f, dt);
                    velocity.z *= std::pow(0.01f, dt);
                }
                break;
            }
        }

        // Bleed off horizontal velocity quickly when idle (e.g. after knockback)
        if (state == EnemyState::IDLE) {
            velocity.x *= std::pow(0.01f, dt);
            velocity.z *= std::pow(0.01f, dt);
        }

        // Gravity only for ground-based enemies; FLYER manages its own Y velocity.
        if (type != EnemyType::FLYER && position.y > FLOOR_Y)
            velocity.y += -24.f * dt;

        // Integrate
        position += velocity * dt;

        if (type != EnemyType::FLYER) {
            if (position.y < FLOOR_Y) { position.y = FLOOR_Y; velocity.y = 0.f; }
            if (grid) {
                static std::vector<int> cands;
                AABB eb{ position+glm::vec3{-RADIUS-0.1f,-0.1f,-RADIUS-0.1f},
                         position+glm::vec3{ RADIUS+0.1f, HEIGHT+0.1f, RADIUS+0.1f} };
                grid->query(eb, cands);
                for (int idx : cands) resolveAABB(walls[idx].box);
            } else {
                for (int i = 0; i < wallCount; ++i) resolveAABB(walls[i].box);
            }
        } else {
            // Keep flyer above the floor minimum even if something pushes it down
            if (position.y < 1.5f) { position.y = 1.5f; velocity.y = 0.f; }
        }

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

// Instanced renderer — one glDrawElementsInstanced call regardless of enemy count.
// GameplayState sets all lighting uniforms on `shader` before calling draw().
class EnemyRenderer {
public:
    ShaderProgram shader;          // owns enemy_inst.vert / enemy_inst.frag
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint instanceVBO = 0;
    GLsizei indexCount = 0;

    static constexpr int MAX_INSTANCES = 256;

    // Per-instance data uploaded to the GPU each frame.
    struct InstanceData {
        glm::mat4 model;        // locations 4-7
        glm::vec3 color;        // location 8
        float     _pad0 = 0.f;
        glm::vec3 emissive;     // location 9
        float     _pad1 = 0.f;
    };

    EnemyRenderer() {
        shader.loadFiles("src/enemy_inst.vert", "src/enemy_inst.frag");

        // Build the shared cube mesh
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
        indexCount = (GLsizei)idx.size();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glGenBuffers(1, &instanceVBO);

        glBindVertexArray(vao);

        // Static mesh geometry (per-vertex attributes 0-3)
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size()*sizeof(Vertex)), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idx.size()*sizeof(unsigned int)), idx.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));

        // Per-instance buffer (dynamic, updated every frame)
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

        // mat4 instanceModel at locations 4-7 (four consecutive vec4s)
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(4 + i);
            glVertexAttribPointer(4+i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                                  (void*)(offsetof(InstanceData, model) + i * 16));
            glVertexAttribDivisor(4 + i, 1);
        }
        // vec3 instanceColor at location 8
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              (void*)offsetof(InstanceData, color));
        glVertexAttribDivisor(8, 1);
        // vec3 instanceEmissive at location 9
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              (void*)offsetof(InstanceData, emissive));
        glVertexAttribDivisor(9, 1);

        glBindVertexArray(0);
    }

    EnemyRenderer(const EnemyRenderer&) = delete;
    EnemyRenderer& operator=(const EnemyRenderer&) = delete;

    ~EnemyRenderer() {
        if (vao) glDeleteVertexArrays(1,&vao);
        if (vbo) glDeleteBuffers(1,&vbo);
        if (ebo) glDeleteBuffers(1,&ebo);
        if (instanceVBO) glDeleteBuffers(1,&instanceVBO);
    }

    // Call after setting all lighting uniforms on `shader`.
    void draw(const std::vector<Enemy>& enemies) {
        // Build per-instance data on the CPU
        static InstanceData buf[MAX_INSTANCES];
        int count = 0;
        for (auto& e : enemies) {
            if (!e.alive || count >= MAX_INSTANCES) continue;
            auto& inst = buf[count++];
            inst.model = glm::translate(glm::mat4(1.f), e.position);

            switch (e.type) {
                case EnemyType::GRUNT:   inst.color={0.8f,0.2f,0.2f}; inst.emissive={0,0,0}; break;
                case EnemyType::SHOOTER: inst.color={0.2f,0.3f,0.9f}; inst.emissive={0.1f,0.2f,0.8f}; break;
                case EnemyType::STALKER: inst.color={0.8f,0.6f,0.0f}; inst.emissive={0,0,0}; break;
                case EnemyType::FLYER:   inst.color={0.7f,0.1f,0.9f}; inst.emissive={0.4f,0.0f,0.6f}; break;
            }
            // Hit flash — bleach toward white
            if (e.hitFlashTimer > 0.f) {
                float t = e.hitFlashTimer / 0.12f;
                inst.color   = glm::mix(inst.color,   glm::vec3{1.f}, t * 0.85f);
                inst.emissive = glm::mix(inst.emissive, glm::vec3{0.8f}, t * 0.6f);
            }
        }
        if (count == 0) return;

        // Upload instance data and draw
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(InstanceData), buf);

        shader.use();
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, count);
        glBindVertexArray(0);
    }
};
