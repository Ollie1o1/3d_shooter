#pragma once
// =============================================================================
// Effects.h — the short-lived things a fight leaves behind: particles (sparks,
// blasts, shell casings, ambient dust and embers), enemies breaking into their
// boxes, scorch decals, hitscan tracers, shockwave rings and the Sovereign's
// sword strokes. No OpenGL: GameplayState draws them (Gameplay_Render.h).
// =============================================================================
#include "Level.h"
#include "Enemy.h"
#include "EnemyModel.h"
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

class Effects {
public:
    // Hitscan tracer — thin billboard quad, additive blending, fades fast
    struct Tracer {
        glm::vec3 start{0.f}, end{0.f};
        float life = 0.f, maxLife = 0.13f;
        float width = 0.055f;
        bool  alive = false;
    };
    static constexpr int MAX_TRACERS = 32;
    Tracer tracers[MAX_TRACERS];

    struct Decal {
        glm::vec3 pos{0.f};
        float life = 0.f, maxLife = 10.f;
        bool  alive = false;
    };
    static constexpr int MAX_DECALS = 64;
    Decal decals[MAX_DECALS];

    struct Particle {
        glm::vec3 pos{0.f}, vel{0.f};
        glm::vec3 color{1.f, 0.5f, 0.05f};
        float life = 0.f, maxLife = 0.f;
        float gravity = 18.f;     // negative = floats upward (embers)
        bool  alive = false;
    };
    static constexpr int MAX_PARTICLES = 1200;
    Particle particles[MAX_PARTICLES];

    // Enemies come apart into their boxes when they die
    struct Debris {
        glm::mat3 shape;          // rotation * scale of the original part
        glm::vec3 pos, vel, axis, color, emissive;
        float angle = 0.f, spin = 0.f, life = 0.f, maxLife = 1.f, floorY = 0.f;
    };
    std::vector<Debris> debris;

    struct Shockwave { glm::vec3 pos; float radius, t; glm::vec3 color; };
    std::vector<Shockwave> shockwaves;
    // A SOVEREIGN sword stroke: an arc of light that sweeps out and fades
    // (kind as EnemyEvents::slash: 0/1 sweeps, 2 the overhead cleave, 3 a dash's cut)
    struct Slash { glm::vec3 pos; float yaw; int kind; float t; };
    static constexpr float SLASH_LIFE = 0.3f;
    std::vector<Slash> slashes;

    // A fresh arena: no marks, sparks, pieces or rings left from the last one
    void clear() {
        for (auto& d : decals)    d.alive = false;
        for (auto& p : particles) p.alive = false;
        debris.clear(); shockwaves.clear();
    }

    // Round-robin search from the last allocation: O(1) on average instead of
    // rescanning the whole pool for every particle of an explosion.
    Particle* freeParticle() {
        for (int k = 0; k < MAX_PARTICLES; ++k) {
            int i = (particleCursor + k) % MAX_PARTICLES;
            if (!particles[i].alive) { particleCursor = (i + 1) % MAX_PARTICLES; return &particles[i]; }
        }
        return nullptr;
    }

    // Generic radial burst. gravity < 0 floats upward.
    void spawnBurst(glm::vec3 c, glm::vec3 color, int n, float speed, float life, float gravity) {
        for (int i = 0; i < n; ++i) {
            Particle* p = freeParticle();
            if (!p) return;
            glm::vec3 d{frand(-1.f,1.f), frand(0.f,1.f), frand(-1.f,1.f)};
            if (glm::length(d) < 1e-3f) d = {0,1,0};
            p->pos = c; p->vel = glm::normalize(d) * speed * frand(0.4f, 1.f);
            p->maxLife = p->life = life * frand(0.6f, 1.f);
            p->color = color * frand(0.8f, 1.2f);
            p->gravity = gravity;
            p->alive = true;
        }
    }

    void spawnDeathParticles(glm::vec3 center, glm::vec3 color) { spawnBurst(center, color, 22, 12.f, 1.0f, 18.f); }
    void spawnHitSparks(glm::vec3 pos, glm::vec3 color)         { spawnBurst(pos, color, 5, 5.f, 0.25f, 18.f); }

    void spawnShellCasing(glm::vec3 origin, glm::vec3 right) {
        Particle* p = freeParticle();
        if (!p) return;
        p->pos     = origin + right * 0.15f;
        p->vel     = right * 2.5f + glm::vec3{0, 3.f, 0}
                   + glm::vec3{((rand()%100)-50)/100.f, 0, ((rand()%100)-50)/100.f};
        p->maxLife = p->life = 0.7f;
        p->color   = {0.85f, 0.7f, 0.15f};
        p->gravity = 18.f;
        p->alive   = true;
    }

    void spawnExplosionParticles(glm::vec3 center, float radius) {
        for (int i = 0; i < 70; ++i) {
            Particle* p = freeParticle();
            if (!p) return;
            glm::vec3 d = glm::normalize(glm::vec3{frand(-1.f,1.f), frand(0.2f,0.8f), frand(-1.f,1.f)});
            p->pos     = center + d * (radius * 0.3f);
            p->vel     = d * (8.f + frand(0.f, 1.f) * radius * 3.f);
            p->maxLife = p->life = 0.5f + frand(0.f, 0.7f);
            float heat = frand(0.f, 1.f);
            p->color   = heat > 0.8f ? glm::vec3{1.f, 0.95f, 0.75f}
                       : heat > 0.4f ? glm::vec3{1.f, 0.55f, 0.10f}
                                     : glm::vec3{0.55f, 0.12f, 0.05f};
            p->gravity = 18.f;
            p->alive   = true;
        }
    }

    void spawnShockwave(glm::vec3 pos, float radius, glm::vec3 color) {
        shockwaves.push_back({pos, radius, 0.f, color});
        for (int i = 0; i < 36; ++i) {
            Particle* p = freeParticle();
            if (!p) return;
            float a = i * 6.2832f / 36.f;
            p->pos = pos + glm::vec3{0, 0.2f, 0};
            p->vel = glm::vec3{std::cos(a), 0.15f, std::sin(a)} * radius * 2.2f;
            p->maxLife = p->life = 0.45f;
            p->color = color;
            p->gravity = 2.f;
            p->alive = true;
        }
    }

    void updateParticles(float dt) {
        for (auto& p : particles) {
            if (!p.alive) continue;
            p.vel.y -= p.gravity * dt;
            p.pos   += p.vel * dt;
            p.life  -= dt;
            if (p.life <= 0.f) p.alive = false;
        }
    }

    // Dust in the yard, embers over lava, motes around the reactor, wind up high.
    // fast: the Gauntlet, whose lava strip is too far off for its embers
    void spawnAmbientParticles(const LevelData& level, glm::vec3 me, bool fast, float dt) {
        int a = level.arenaAt(me);
        if (a < 0) return;
        Ambient kind = level.arenas[a].ambient;
        ambientTimer += dt;
        const float every = 1.f / 45.f;
        while (ambientTimer > every) {
            ambientTimer -= every;
            Particle* p = freeParticle();
            if (!p) return;
            p->alive = true;
            float ang = frand(0.f, 6.28f), r = frand(4.f, 22.f);
            switch (kind) {
            case Ambient::DUST:
                p->pos = me + glm::vec3{std::cos(ang) * r, frand(0.3f,6.f), std::sin(ang) * r};
                p->vel = {frand(0.2f,0.8f), frand(-0.05f,0.15f), frand(-0.2f,0.2f)};
                p->color = glm::vec3{1.f, 0.7f, 0.5f} * 0.15f;
                p->gravity = 0.f; p->maxLife = p->life = frand(2.5f, 4.5f);
                break;
            case Ambient::EMBERS:
                if (!level.hazards.empty() && !fast) {
                    const Hazard& hz = level.hazards[rand() % level.hazards.size()];
                    p->pos = {frand(hz.box.min.x, hz.box.max.x), hz.box.max.y + 0.05f, frand(hz.box.min.z, hz.box.max.z)};
                } else {
                    p->pos = me + glm::vec3{std::cos(ang) * r, frand(-2.f, 1.f), std::sin(ang) * r};
                }
                p->vel = {frand(-0.4f,0.4f), frand(1.5f,3.5f), frand(-0.4f,0.4f)};
                p->color = glm::vec3{1.f, 0.45f, 0.1f} * frand(0.6f, 1.1f);
                p->gravity = -0.5f; p->maxLife = p->life = frand(1.2f, 2.8f);
                break;
            case Ambient::MOTES: {
                glm::vec3 c = level.hasReactor ? level.reactorPos : me;
                float rr = frand(2.5f, 9.f);
                p->pos = c + glm::vec3{std::cos(ang) * rr, frand(-1.f, 2.5f), std::sin(ang) * rr};
                p->vel = {0.f, frand(1.f, 2.5f), 0.f};
                p->color = glm::vec3{0.3f, 0.9f, 1.f} * 0.6f;
                p->gravity = -0.3f; p->maxLife = p->life = frand(2.f, 4.f);
                break;
            }
            case Ambient::WIND:
                // Fast pale streaks blowing across the heights
                p->pos = me + glm::vec3{std::cos(ang) * r - 10.f, frand(-1.f, 9.f), std::sin(ang) * r};
                p->vel = {frand(9.f, 14.f), frand(-0.3f, 0.3f), frand(1.f, 3.f)};
                p->color = glm::vec3{0.85f, 0.9f, 1.f} * 0.22f;
                p->gravity = 0.f; p->maxLife = p->life = frand(1.f, 2.f);
                break;
            case Ambient::STEAM:
                // Pale puffs drifting up from the floor
                p->pos = me + glm::vec3{std::cos(ang) * r * 0.6f, frand(-1.f, 0.5f), std::sin(ang) * r * 0.6f};
                p->vel = {frand(-0.3f, 0.3f), frand(1.f, 2.2f), frand(-0.3f, 0.3f)};
                p->color = glm::vec3{0.7f, 0.95f, 0.85f} * 0.16f;
                p->gravity = -0.2f; p->maxLife = p->life = frand(2.f, 3.5f);
                break;
            case Ambient::ASH:
                p->pos = me + glm::vec3{std::cos(ang) * r, frand(4.f, 12.f), std::sin(ang) * r};
                p->vel = {frand(0.3f, 1.2f), frand(-1.2f, -0.4f), frand(-0.3f, 0.3f)};
                p->color = glm::vec3{0.6f, 0.55f, 0.55f} * 0.25f;
                p->gravity = 0.f; p->maxLife = p->life = frand(3.f, 5.f);
                break;
            }
        }
    }

    // Break an enemy into the boxes of its rig, posed as it was at `clock`;
    // the pieces bounce on the ground at floorY
    void spawnDebrisFor(const Enemy& e, float clock, float floorY) {
        static std::vector<BoxInstance> parts;
        parts.clear();
        Enemy pose = e;             // the pieces keep the enemy's own colours,
        pose.hitFlashTimer = 0.f;   // not the white flash of the killing blow
        pose.spawnTimer    = 0.f;
        buildEnemy(pose, clock, parts);
        glm::vec3 c = e.position + glm::vec3{0, e.height() * 0.5f, 0};
        for (auto& b : parts) {
            Debris d;
            d.shape = glm::mat3(b.model);
            d.pos   = glm::vec3(b.model[3]);
            glm::vec3 out = d.pos - c; out.y = std::max(out.y, 0.f);
            float ol = glm::length(out);
            d.vel   = (ol > 1e-3f ? out / ol : glm::vec3{0,1,0}) * frand(3.f, 8.f)
                    + glm::vec3{frand(-2.f,2.f), frand(3.f,7.f), frand(-2.f,2.f)} + e.velocity * 0.3f;
            d.axis  = glm::normalize(glm::vec3{frand(-1.f,1.f), frand(-1.f,1.f), frand(-1.f,1.f)} + glm::vec3{0.01f});
            d.spin  = frand(4.f, 12.f);
            d.color = b.color; d.emissive = b.emissive;
            d.maxLife = d.life = frand(1.2f, 1.8f);
            d.floorY = floorY;
            debris.push_back(d);
        }
        if (debris.size() > 900) debris.erase(debris.begin(), debris.begin() + (debris.size() - 900));
    }

    void updateDebris(float dt) {
        for (auto& d : debris) {
            d.vel.y -= 20.f * dt;
            d.pos   += d.vel * dt;
            d.angle += d.spin * dt;
            if (d.pos.y < d.floorY) { d.pos.y = d.floorY; d.vel.y = std::fabs(d.vel.y) * 0.35f; d.vel.x *= 0.6f; d.vel.z *= 0.6f; d.spin *= 0.6f; }
            d.life -= dt;
        }
        debris.erase(std::remove_if(debris.begin(), debris.end(),
                     [](const Debris& d){ return d.life <= 0.f; }), debris.end());
    }

    // Shockwave rings and sword strokes grow, fade and go
    void updateRings(float dt) {
        for (auto& s : shockwaves) s.t += dt;
        for (auto& s : slashes) s.t += dt;
        slashes.erase(std::remove_if(slashes.begin(), slashes.end(),
                      [](const Slash& s){ return s.t > SLASH_LIFE; }), slashes.end());
        shockwaves.erase(std::remove_if(shockwaves.begin(), shockwaves.end(),
                         [](const Shockwave& s){ return s.t > 0.5f; }), shockwaves.end());
    }

    void spawnDecal(glm::vec3 enemyPos) {
        int slot = -1;
        float minLife = 1e9f;
        for (int i = 0; i < MAX_DECALS; ++i) {
            if (!decals[i].alive) { slot = i; break; }
            if (decals[i].life < minLife) { minLife = decals[i].life; slot = i; }
        }
        auto& d  = decals[slot];
        d.pos    = enemyPos + glm::vec3{0.f, 0.025f, 0.f};
        d.maxLife = 10.f;
        d.life    = d.maxLife;
        d.alive   = true;
    }

    void spawnTracer(glm::vec3 start, glm::vec3 end, float width = 0.055f, float life = 0.22f) {
        Tracer* slot = nullptr;
        for (auto& t : tracers) if (!t.alive) { slot = &t; break; }
        if (!slot) {   // all busy (shotgun spam): reuse the oldest
            slot = &tracers[0];
            for (auto& t : tracers) if (t.life < slot->life) slot = &t;
        }
        slot->start = start; slot->end = end;
        slot->maxLife = life; slot->life = life; slot->width = width;
        slot->alive = true;
    }

    // Tracers and decals age on the physics tick
    void tickMarks(float dt) {
        for (auto& t : tracers) if (t.alive) { t.life -= dt; if (t.life <= 0.f) t.alive = false; }
        for (auto& d : decals)  if (d.alive) { d.life -= dt; if (d.life <= 0.f) d.alive = false; }
    }

private:
    int   particleCursor = 0;
    float ambientTimer   = 0.f;
};
