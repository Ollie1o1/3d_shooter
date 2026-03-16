#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Player.h"
#include "Mesh.h"
#include "ShaderProgram.h"
#include <array>
#include <vector>

// Forward declaration
struct Enemy;

struct Projectile {
    glm::vec3 position{0.f};
    glm::vec3 velocity{0.f};
    glm::vec3 emissiveColor{1.f, 0.8f, 0.2f};
    float     damage     = 8.f;
    float     lifetime   = 5.f;
    bool      alive      = false;
    bool      isPlayer   = true;
    bool      isGrenade  = false;  // if true: gravity applied, explodes on contact
    bool      hasGravity = false;  // arc trajectory
    float     blastRadius = 0.f;   // > 0 triggers AoE explosion
};

class ProjectileSystem {
public:
    static constexpr int POOL_SIZE = 64;
    std::array<Projectile, POOL_SIZE> pool;

    GLuint vao = 0, vbo = 0;

    ProjectileSystem() {
        float s = 0.15f;
        float verts[] = {
            -s,-s,0, 0,0, 0,0,1,
             s,-s,0, 1,0, 0,0,1,
             s, s,0, 1,1, 0,0,1,
            -s,-s,0, 0,0, 0,0,1,
             s, s,0, 1,1, 0,0,1,
            -s, s,0, 0,1, 0,0,1,
        };
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(5*sizeof(float)));
        glBindVertexArray(0);
    }

    ~ProjectileSystem() {
        if (vao) glDeleteVertexArrays(1,&vao);
        if (vbo) glDeleteBuffers(1,&vbo);
    }

    void fire(glm::vec3 pos, glm::vec3 vel, float dmg, bool player,
              glm::vec3 color = {1,0.8f,0.2f},
              bool grenade = false, float blastR = 0.f) {
        for (auto& p : pool) {
            if (!p.alive) {
                p.position    = pos;
                p.velocity    = vel;
                p.damage      = dmg;
                p.isPlayer    = player;
                p.emissiveColor = color;
                p.alive       = true;
                p.lifetime    = grenade ? 6.f : 5.f;
                p.isGrenade   = grenade;
                p.hasGravity  = grenade;
                p.blastRadius = blastR;
                return;
            }
        }
    }

    struct ExplosionEvent {
        glm::vec3 pos;
        float     radius;
        float     damage;
    };

    struct HitResult {
        std::vector<std::pair<int,int>> enemyHits; // proj idx, enemy idx
        std::vector<ExplosionEvent>     explosions;
        bool  hitPlayer    = false;
        float playerDamage = 0.f;
        int   parryableIndex  = -1;  // closest enemy projectile within parry range
        int   boostableIndex  = -1;  // closest PLAYER projectile within parry range
    };

    HitResult update(float dt, const Wall* walls, int wallCount,
                     std::vector<Enemy>& enemies,
                     const glm::vec3& playerPos,
                     const SpatialGrid* grid = nullptr);

    void draw(ShaderProgram& shader, const glm::mat4& view, const glm::mat4& /*proj*/) {
        glDisable(GL_CULL_FACE);
        glBindVertexArray(vao);
        for (auto& p : pool) {
            if (!p.alive) continue;
            glm::mat4 m = glm::translate(glm::mat4(1.f), p.position);
            glm::mat4 billboard = m;
            billboard[0] = glm::vec4(glm::normalize(glm::vec3(view[0][0],view[1][0],view[2][0])), 0);
            billboard[1] = glm::vec4(glm::normalize(glm::vec3(view[0][1],view[1][1],view[2][1])), 0);
            billboard[2] = glm::vec4(glm::normalize(glm::vec3(view[0][2],view[1][2],view[2][2])), 0);
            billboard[3] = m[3];
            shader.setMat4("model", billboard);
            shader.setVec3("emissiveColor", p.emissiveColor);
            shader.setVec3("objectColor", p.emissiveColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
    }
};

// Include Enemy after forward declaration is satisfied
#include "Enemy.h"

inline ProjectileSystem::HitResult ProjectileSystem::update(
    float dt, const Wall* walls, int wallCount,
    std::vector<Enemy>& enemies,
    const glm::vec3& playerPos,
    const SpatialGrid* grid)
{
    HitResult result;
    float closestParry = 2.5f;
    float closestBoost = 2.5f;

    // Reusable candidate list for grid queries (avoids per-projectile allocation).
    static std::vector<int> candidates;

    for (int i = 0; i < POOL_SIZE; ++i) {
        auto& p = pool[i];
        if (!p.alive) continue;

        p.lifetime -= dt;
        if (p.lifetime <= 0.f) { p.alive = false; continue; }

        if (p.hasGravity) p.velocity.y -= 20.f * dt;
        p.position += p.velocity * dt;

        // Wall + floor/ceiling collision
        bool hitSolid = false;
        if (grid) {
            AABB pb{ p.position - glm::vec3{0.1f}, p.position + glm::vec3{0.1f} };
            grid->query(pb, candidates);
            for (int w : candidates) {
                const AABB& b = walls[w].box;
                if (p.position.x > b.min.x && p.position.x < b.max.x &&
                    p.position.y > b.min.y && p.position.y < b.max.y &&
                    p.position.z > b.min.z && p.position.z < b.max.z) {
                    hitSolid = true; break;
                }
            }
        } else {
            for (int w = 0; w < wallCount && !hitSolid; ++w) {
                const AABB& b = walls[w].box;
                if (p.position.x > b.min.x && p.position.x < b.max.x &&
                    p.position.y > b.min.y && p.position.y < b.max.y &&
                    p.position.z > b.min.z && p.position.z < b.max.z) {
                    hitSolid = true;
                }
            }
        }
        if (!hitSolid && (p.position.y < 0.f || p.position.y > 14.f)) hitSolid = true;

        if (hitSolid) {
            if (p.isGrenade && p.blastRadius > 0.f) {
                result.explosions.push_back({p.position, p.blastRadius, p.damage});
            }
            p.alive = false;
            continue;
        }

        if (p.isPlayer) {
            // Track proximity to player for projectile boost (parry own shot).
            float selfDist = glm::length(p.position - playerPos);
            if (selfDist < closestBoost) {
                closestBoost = selfDist;
                result.boostableIndex = i;
            }

            for (int ei = 0; ei < (int)enemies.size(); ++ei) {
                auto& e = enemies[ei];
                if (!e.alive) continue;
                AABB box = e.getAABB();
                if (p.position.x > box.min.x && p.position.x < box.max.x &&
                    p.position.y > box.min.y && p.position.y < box.max.y &&
                    p.position.z > box.min.z && p.position.z < box.max.z) {
                    if (p.isGrenade && p.blastRadius > 0.f) {
                        result.explosions.push_back({p.position, p.blastRadius, p.damage});
                        p.alive = false;
                        break;
                    }
                    result.enemyHits.push_back({i, ei});
                    p.alive = false;
                    break;
                }
            }
        } else {
            glm::vec3 diff = p.position - playerPos;
            float dist = glm::length(diff);
            if (dist < 0.6f) {
                result.hitPlayer = true;
                result.playerDamage += p.damage;
                p.alive = false;
            } else if (dist < closestParry) {
                closestParry = dist;
                result.parryableIndex = i;
            }
        }
    }
    return result;
}
