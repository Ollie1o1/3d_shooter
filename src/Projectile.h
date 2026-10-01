#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Player.h"
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
    float     size       = 1.f;    // billboard + hit-radius scale (big boss orbs)
    bool      heavy      = false;  // JUGGERNAUT siege shell
    bool      parried    = false;  // sent back by the player: ignores armor
};

class ProjectileSystem {
public:
    static constexpr int POOL_SIZE = 160;  // a boss volley is 11 shots
    std::array<Projectile, POOL_SIZE> pool;

    void fire(glm::vec3 pos, glm::vec3 vel, float dmg, bool player,
              glm::vec3 color = {1,0.8f,0.2f},
              bool grenade = false, float blastR = 0.f, float size = 1.f, bool heavy = false) {
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
                p.size        = size;
                p.heavy       = heavy;
                p.parried     = false;
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
        glm::vec3 playerHitFrom{0.f};  // world point the hitting shot came from
        int   parryableIndex  = -1;  // closest enemy projectile within parry range
        int   boostableIndex  = -1;  // closest PLAYER projectile within parry range
    };

    HitResult update(float dt, const Wall* walls, int wallCount,
                     std::vector<Enemy>& enemies,
                     const glm::vec3& playerPos,
                     const SpatialGrid* grid = nullptr);

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
        if (!hitSolid && (p.position.y < 0.f || p.position.y > 150.f)) hitSolid = true;

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
                if (!e.targetable()) continue;
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
            if (dist < 0.6f * std::max(1.f, p.size * 0.8f)) {
                result.hitPlayer = true;
                result.playerDamage += p.damage;
                float spd = glm::length(p.velocity);
                result.playerHitFrom = spd > 0.001f ? p.position - p.velocity / spd * 5.f
                                                    : p.position;
                p.alive = false;
            } else if (dist < closestParry) {
                closestParry = dist;
                result.parryableIndex = i;
            }
        }
    }
    return result;
}
