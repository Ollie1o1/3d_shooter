#pragma once
// =============================================================================
// Effects.h — the short-lived things a fight leaves behind: particles (sparks,
// blasts, shell casings, ambient dust and embers), enemies breaking into their
// boxes, scorch decals, hitscan tracers, shockwave rings and the Sovereign's
// sword strokes. No OpenGL: GameplayState draws them (Gameplay_Render.h).
// =============================================================================
#include "world/Level.h"
#include "enemies/Enemy.h"
#include "render/EnemyModel.h"
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
        glm::vec3 color{0.97f, 0.95f, 0.72f};   // the gun's glow, whitened
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
        float mesh = 0.f;         // its shape (BoxInstance::mesh)
        glm::vec3 pos, vel, axis, color, emissive;
        float angle = 0.f, spin = 0.f, life = 0.f, maxLife = 1.f, floorY = 0.f;
        float hot = 0.f;          // seconds left glowing white-hot from the killing blow
    };
    std::vector<Debris> debris;
    // A body made in Blender breaks at its joints: each limb (every part on one joint) flies as one
    // rigid piece, holding its shape, and shrinks away
    struct Piece { glm::mat4 local; glm::vec3 color, emissive; float mesh; };   // about the chunk's centre
    struct Chunk {
        std::vector<Piece> pieces;
        glm::vec3 pos, vel, axis;
        float angle = 0.f, spin = 0.f, life = 0.f, maxLife = 1.f, floorY = 0.f, hot = 0.f;
        float low = 0.f;          // how far below its centre it reaches (it bounces there)
    };
    std::vector<Chunk> chunks;
    static constexpr size_t MAX_CHUNK_PIECES = 2400;

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
        debris.clear(); chunks.clear(); shockwaves.clear();
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
    // A cone of sparks along `dir` (a hit's spray: back at you off the armour, or out through it)
    void spawnSpray(glm::vec3 pos, glm::vec3 dir, glm::vec3 color, int n, float speed, float life, float spread = 0.6f) {
        for (int i = 0; i < n; ++i) {
            Particle* p = freeParticle();
            if (!p) return;
            glm::vec3 v = dir + glm::vec3{frand(-spread, spread), frand(-spread, spread) + 0.15f, frand(-spread, spread)};
            float l = glm::length(v);
            p->pos = pos; p->vel = (l > 1e-3f ? v / l : glm::vec3{0, 1, 0}) * speed * frand(0.5f, 1.f);
            p->maxLife = p->life = life * frand(0.6f, 1.f);
            p->color = color * frand(0.8f, 1.3f);
            p->gravity = 16.f;
            p->alive = true;
        }
    }

    void spawnShellCasing(glm::vec3 origin, glm::vec3 right, glm::vec3 glow = {0.85f, 0.7f, 0.15f}) {
        Particle* p = freeParticle();
        if (!p) return;
        p->pos     = origin + right * 0.15f;
        p->vel     = right * 2.5f + glm::vec3{0, 3.f, 0}
                   + glm::vec3{((rand()%100)-50)/100.f, 0, ((rand()%100)-50)/100.f};
        p->maxLife = p->life = 0.7f;
        p->color   = glm::mix(glm::vec3{0.7f, 0.52f, 0.24f}, glow, 0.5f);   // brass, lit by the gun's glow
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

    // A shot breaking on a wall: a spray of small glowing cubes thrown back off it, tumbling, bouncing on floorY
    void spawnShards(glm::vec3 pos, glm::vec3 back, glm::vec3 glow, float size, float floorY) {
        const int n = 5 + (int)std::min(6.f, size * 3.f);
        for (int i = 0; i < n; ++i) {
            Debris d;
            const float s = (0.05f + 0.06f * frand(0.f, 1.f)) * std::max(0.7f, size);
            d.shape = glm::mat3(glm::scale(glm::mat4(1.f), glm::vec3{s}));
            d.pos = pos + glm::vec3{frand(0.f, 1.f) - 0.5f, frand(0.f, 1.f) - 0.5f, frand(0.f, 1.f) - 0.5f} * 0.2f;
            glm::vec3 r{frand(0.f, 1.f) * 2.f - 1.f, frand(0.f, 1.f) * 2.f - 1.f, frand(0.f, 1.f) * 2.f - 1.f};
            d.vel = back * (2.f + 4.f * frand(0.f, 1.f)) + r * 3.f + glm::vec3{0.f, 2.5f, 0.f};
            glm::vec3 ax{frand(0.f, 1.f) - 0.5f, frand(0.f, 1.f) - 0.5f, frand(0.f, 1.f) - 0.5f};
            d.axis = glm::length(ax) > 1e-3f ? glm::normalize(ax) : glm::vec3{0.f, 1.f, 0.f};
            d.spin = 8.f + 14.f * frand(0.f, 1.f);
            d.maxLife = d.life = 0.5f + 0.4f * frand(0.f, 1.f);
            d.color = glow * 0.35f; d.emissive = glow * 1.6f;
            d.floorY = floorY + s;
            debris.push_back(d);
        }
        if (debris.size() > 900) debris.erase(debris.begin(), debris.begin() + (debris.size() - 900));
    }

    // Break an enemy into the boxes of its rig, posed as it was at `clock`;
    // the pieces bounce on the ground at floorY
    void spawnDebrisFor(const Enemy& e, float clock, float floorY) {
        static std::vector<BoxInstance> parts;
        static std::vector<rig::PartJoint> joints;
        parts.clear(); joints.clear();
        Enemy pose = e;             // the pieces keep the enemy's own colours,
        pose.hitFlashTimer = 0.f;   // not the white flash of the killing blow
        pose.spawnTimer    = 0.f;
        rig::jointSink() = &joints;
        buildEnemy(pose, clock, parts);
        rig::jointSink() = nullptr;
        // parts on one joint (three or more) go as one chunk; the rest one by one
        static std::vector<int> group;
        group.assign(parts.size(), -1);
        {
            std::vector<std::pair<int, int>> count;   // (joint, parts)
            for (auto& pj : joints) {
                auto it = std::find_if(count.begin(), count.end(), [&](auto& c) { return c.first == pj.joint; });
                if (it == count.end()) count.push_back({pj.joint, 1}); else ++it->second;
            }
            for (auto& pj : joints)
                for (auto& c : count)
                    if (c.first == pj.joint && c.second >= 3 && pj.index >= 0 && pj.index < (int)parts.size()) group[pj.index] = pj.joint;
        }
        glm::vec3 c = e.position + glm::vec3{0, e.height() * 0.5f, 0};
        // The killing blow carries it: the pieces go the way the shot went, harder the
        // harder it hit (and light ones further than heavies); a headshot takes the head off first
        float mass = std::min(hitreact::bodyOf(e.type).mass, 12.f);
        float carry = std::min(e.hitPush / std::max(mass, 0.5f), 18.f);
        glm::vec3 along = e.hitDir;
        float headY = e.position.y + e.height() * 0.78f;
        // where a piece flies: out from the body's middle, up, the way the killing blow went
        auto launch = [&](glm::vec3 at, float scatter) {
            glm::vec3 out = at - c; out.y = std::max(out.y, 0.f);
            float ol = glm::length(out);
            glm::vec3 v = (ol > 1e-3f ? out / ol : glm::vec3{0,1,0}) * frand(3.f, 8.f) * scatter * (1.f - 0.35f * std::min(1.f, carry / 10.f))
                        + glm::vec3{frand(-2.f,2.f), frand(3.f,7.f), frand(-2.f,2.f)} * scatter + e.velocity * 0.3f + e.knockVel * 0.5f
                        + along * carry * frand(0.5f, 1.25f);
            if (carry > 2.f) {   // a hard blow: nothing much comes back at the one who struck it
                glm::vec3 flat{along.x, 0.f, along.z};
                float fl = glm::length(flat), back = fl > 1e-3f ? glm::dot(v, flat / fl) : 0.f;
                if (back < 0.f) v -= flat / fl * back * 0.85f;
            }
            return v;
        };
        {   // the chunks: one per joint
            std::vector<int> done;
            for (size_t i = 0; i < parts.size(); ++i) {
                int g = group[i];
                if (g < 0 || std::find(done.begin(), done.end(), g) != done.end()) continue;
                done.push_back(g);
                Chunk ch;
                glm::vec3 lo{1e9f}, hi{-1e9f};
                for (size_t j = i; j < parts.size(); ++j)
                    if (group[j] == g) { glm::vec3 p{parts[j].model[3]}; lo = glm::min(lo, p); hi = glm::max(hi, p); }
                ch.pos = (lo + hi) * 0.5f;
                for (size_t j = i; j < parts.size(); ++j)
                    if (group[j] == g) {
                        glm::mat4 local = parts[j].model; local[3] -= glm::vec4(ch.pos, 0.f);
                        ch.pieces.push_back({local, parts[j].color, parts[j].emissive, parts[j].mesh});
                    }
                float size = glm::length(hi - lo);
                ch.low = std::max(0.05f, 0.35f * size);
                ch.vel = launch(ch.pos, 0.8f);
                if (e.hitHead && (g == enemyfile::E_HEAD || g == enemyfile::E_CROWN)) ch.vel += glm::vec3{0.f, frand(5.f, 8.f), 0.f} + along * frand(3.f, 6.f);   // the head goes first
                ch.hot = carry > 2.f || e.hitHead ? frand(0.12f, 0.22f) : 0.08f;
                ch.axis = glm::normalize(glm::vec3{frand(-1.f,1.f), frand(-1.f,1.f), frand(-1.f,1.f)} + glm::vec3{0.01f});
                ch.spin = frand(3.f, 8.f) / std::max(1.f, std::sqrt(size));   // big pieces turn slower
                ch.maxLife = ch.life = frand(1.6f, 2.3f);
                ch.floorY = floorY;
                chunks.push_back(std::move(ch));
            }
            size_t total = 0;
            for (auto& ch : chunks) total += ch.pieces.size();
            while (total > MAX_CHUNK_PIECES && !chunks.empty()) { total -= chunks.front().pieces.size(); chunks.erase(chunks.begin()); }
        }
        for (size_t i = 0; i < parts.size(); ++i) {
            if (group[i] >= 0) continue;   // (in a chunk)
            const BoxInstance& b = parts[i];
            Debris d;
            d.shape = glm::mat3(b.model);
            d.mesh  = b.mesh;
            d.pos   = glm::vec3(b.model[3]);
            d.vel   = launch(d.pos, 1.f);
            if (e.hitHead && d.pos.y > headY) d.vel += glm::vec3{0.f, frand(5.f, 8.f), 0.f} + along * frand(3.f, 6.f);   // the head goes first
            d.hot = carry > 2.f || e.hitHead ? frand(0.12f, 0.22f) : 0.08f;
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
            if (d.hot > 0.f) d.hot -= dt;
        }
        debris.erase(std::remove_if(debris.begin(), debris.end(),
                     [](const Debris& d){ return d.life <= 0.f; }), debris.end());
        for (auto& ch : chunks) {
            ch.vel.y -= 20.f * dt;
            ch.pos   += ch.vel * dt;
            ch.angle += ch.spin * dt;
            if (ch.pos.y - ch.low < ch.floorY) {   // it lands: bounces low, skids, its spin dying
                ch.pos.y = ch.floorY + ch.low;
                ch.vel.y = std::fabs(ch.vel.y) * 0.3f; ch.vel.x *= 0.55f; ch.vel.z *= 0.55f; ch.spin *= 0.5f;
            }
            ch.life -= dt;
            if (ch.hot > 0.f) ch.hot -= dt;
        }
        chunks.erase(std::remove_if(chunks.begin(), chunks.end(), [](const Chunk& c){ return c.life <= 0.f; }), chunks.end());
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

    void spawnTracer(glm::vec3 start, glm::vec3 end, float width = 0.055f, float life = 0.22f,
                     glm::vec3 color = {0.97f, 0.95f, 0.72f}) {
        Tracer* slot = nullptr;
        for (auto& t : tracers) if (!t.alive) { slot = &t; break; }
        if (!slot) {   // all busy (shotgun spam): reuse the oldest
            slot = &tracers[0];
            for (auto& t : tracers) if (t.life < slot->life) slot = &t;
        }
        slot->start = start; slot->end = end;
        slot->maxLife = life; slot->life = life; slot->width = width; slot->color = color;
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
