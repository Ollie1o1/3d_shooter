#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Collision.h"
#include <array>
#include <vector>

// Forward declaration
struct Enemy;

struct Projectile {
    glm::vec3 position{0.f};
    glm::vec3 prevPosition{0.f};   // start of the last physics tick (render interpolation)
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
    int       owner      = -1;     // uid of the enemy that fired it
    int       ownerType  = -1;     // its EnemyType (the run log: who hurt you, even once it's dead)
    float     parryDamage = 0.f;   // > 0: what it does when parried back (else 60, or 400 heavy)
    int       homeOn     = -1;     // uid: a parried orb steering into the WARDEN's core
    bool      whizzed    = false;  // its near-miss whizz has played (GameplayState::whizPast)
    float     age        = 0.f;    // seconds since it was fired (drawn: it pops into being)
};

class ProjectileSystem {
public:
    static constexpr int POOL_SIZE = 160;  // a boss volley is 11 shots
    static constexpr float FRIENDLY_ARM = 0.35f;   // s before an enemy's shell can hit another enemy
    std::array<Projectile, POOL_SIZE> pool;
    float floorY = 0.f;   // shots that sink below this hit the ground
    const int* dynWalls = nullptr;
    int dynCount = 0;

    // The shot fired (its owner can be set), or nullptr if the pool is full
    Projectile* fire(glm::vec3 pos, glm::vec3 vel, float dmg, bool player,
              glm::vec3 color = {1,0.8f,0.2f},
              bool grenade = false, float blastR = 0.f, float size = 1.f, bool heavy = false) {
        for (auto& p : pool) {
            if (!p.alive) {
                p.position    = pos;
                p.prevPosition = pos;
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
                p.owner       = -1;
                p.ownerType   = -1;
                p.parryDamage = 0.f;
                p.homeOn      = -1;
                p.whizzed     = false;
                p.age         = 0.f;
                return &p;
            }
        }
        return nullptr;
    }

    struct ExplosionEvent {
        glm::vec3 pos;
        float     radius;
        float     damage;
    };

    struct HitResult {
        std::vector<std::pair<int,int>> enemyHits; // proj idx, enemy idx
        std::vector<std::pair<int,int>> friendlyHits; // an enemy's siege shell into another enemy
        std::vector<ExplosionEvent>     explosions;
        std::vector<int>                wallHits;     // proj idx: a shot that broke on a wall or the floor (no blast)
        bool  hitPlayer    = false;
        float playerDamage = 0.f;
        glm::vec3 playerHitFrom{0.f};  // world point the hitting shot came from
        int   playerHitOwner = -1;     // the EnemyType of whoever fired it (the run log); -1 unknown
        int   parryableIndex  = -1;  // closest enemy projectile within parry range
        int   boostableIndex  = -1;  // closest PLAYER projectile within parry range
    };

    // Result storage is retained until the next update; callers may copy it
    // when they need a longer-lived snapshot.
    const HitResult& update(float dt, const Wall* walls, int wallCount,
                     std::vector<Enemy>& enemies,
                     const glm::vec3& playerPos,
                     const SpatialGrid* grid = nullptr);
private:
    HitResult result;
    struct Target { int index, uid; AABB box; bool boss; };
    std::vector<Target> targets;
    std::vector<int> candidates;
};

// Include Enemy after forward declaration is satisfied
#include "enemies/Enemy.h"

inline const ProjectileSystem::HitResult& ProjectileSystem::update(
    float dt, const Wall* walls, int wallCount,
    std::vector<Enemy>& enemies,
    const glm::vec3& playerPos,
    const SpatialGrid* grid)
{
    result.enemyHits.clear(); result.friendlyHits.clear(); result.explosions.clear(); result.wallHits.clear();
    result.hitPlayer = false; result.playerDamage = 0.f; result.playerHitFrom = glm::vec3{0.f};
    result.playerHitOwner = result.parryableIndex = result.boostableIndex = -1;
    float closestParry = 2.5f;
    float closestBoost = 2.5f;

    // Build each live target's bounds once per tick, rather than per shot.
    targets.clear();
    const bool needsTargets = std::any_of(pool.begin(), pool.end(), [](const Projectile& p) { return p.alive && (p.isPlayer || p.heavy); });
    for (int ei = 0; needsTargets && ei < (int)enemies.size(); ++ei) {
        const auto& e = enemies[ei];
        if (e.targetable()) targets.push_back({ei, e.uid, e.getAABB(), isBoss(e.type)});
    }

    for (int i = 0; i < POOL_SIZE; ++i) {
        auto& p = pool[i];
        if (!p.alive) continue;

        p.lifetime -= dt;
        p.age += dt;
        if (p.lifetime <= 0.f) { p.alive = false; continue; }

        const glm::vec3 from = p.position;
        if (p.hasGravity) p.velocity.y -= 20.f * dt;
        const glm::vec3 to = from + p.velocity * dt;
        const AABB swept{glm::min(from, to), glm::max(from, to)};
        float first = 1.f;
        enum class Contact { NONE, SOLID, ENEMY, PLAYER };
        Contact contact = Contact::NONE;
        int target = -1;
        auto wall = [&](int index) {
            SweepHit hit;
            if (segmentBoxHit(from, to, walls[index].box, hit) && (contact == Contact::NONE || hit.time < first)) {
                first = hit.time; contact = Contact::SOLID;
            }
        };
        if (grid) {
            grid->query(swept, candidates);
            for (int index : candidates) wall(index);
            for (int k = 0; k < dynCount; ++k) wall(dynWalls[k]);
        } else for (int index = 0; index < wallCount; ++index) wall(index);
        auto plane = [&](float limit, bool below) {
            const bool outside = below ? to.y < limit : to.y > limit;
            if (!outside) return;
            const float travel = to.y - from.y;
            const float time = std::abs(travel) > 1e-8f ? glm::clamp((limit - from.y) / travel, 0.f, 1.f) : 0.f;
            if (contact == Contact::NONE || time < first) { first = time; contact = Contact::SOLID; }
        };
        plane(floorY, true); plane(150.f, false);

        const bool friendly = !p.isPlayer && p.heavy && p.lifetime < 5.f - FRIENDLY_ARM;
        if (p.isPlayer || friendly) for (const auto& candidate : targets) {
            if (friendly && (candidate.boss || (p.owner >= 0 && candidate.uid == p.owner))) continue;
            const AABB& box = candidate.box;
            // Cheap broad phase before the exact segment test.
            if (swept.max.x < box.min.x || swept.min.x > box.max.x || swept.max.y < box.min.y ||
                swept.min.y > box.max.y || swept.max.z < box.min.z || swept.min.z > box.max.z) continue;
            SweepHit hit;
            if (segmentBoxHit(from, to, box, hit) && (contact == Contact::NONE || hit.time < first)) {
                first = hit.time; contact = Contact::ENEMY; target = candidate.index;
            }
        }
        if (!p.isPlayer) {
            float time;
            if (segmentSphereHit(from, to, playerPos, .6f * std::max(1.f, p.size * .8f), time) &&
                (contact == Contact::NONE || time < first)) {
                first = time; contact = Contact::PLAYER;
            }
        }
        p.position = glm::mix(from, to, first);
        if (contact != Contact::NONE) {
            p.alive = false;
            if (contact == Contact::PLAYER) {
                result.hitPlayer = true; result.playerDamage += p.damage; result.playerHitOwner = p.ownerType;
                const float speed = glm::length(p.velocity);
                result.playerHitFrom = speed > .001f ? p.position - p.velocity / speed * 5.f : p.position;
            } else if (contact == Contact::ENEMY && friendly) result.friendlyHits.push_back({i, target});
            else if (p.isGrenade && p.blastRadius > 0.f) result.explosions.push_back({p.position, p.blastRadius, p.damage});
            else if (contact == Contact::ENEMY) result.enemyHits.push_back({i, target});
            else result.wallHits.push_back(i);
            continue;
        }
        const float distance = glm::length(p.position - playerPos);
        if (p.isPlayer && distance < closestBoost) { closestBoost = distance; result.boostableIndex = i; }
        if (!p.isPlayer && distance < closestParry) { closestParry = distance; result.parryableIndex = i; }
    }
    return result;
}
