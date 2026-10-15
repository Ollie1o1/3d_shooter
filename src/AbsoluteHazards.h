#pragma once
// =============================================================================
// AbsoluteHazards.h — what THE ABSOLUTE leaves in the Zenith besides his
// fists and blade:
//   RAYS      red. They fire along their red line (where you stood as it
//             began), then chase you slower than you strafe; a pillar between
//             you stops them; a sword parry in the first 0.15 s sends one back.
//   STOMPS    a ring rolling out along the floor: jump it.
//   SERPENTS  white coils of light that home in, slower than you run; parry
//             one (F or the sword) and it flies back into him for 300.
//   SEAMS     two waves of burning floor cells, a checkerboard 0.7 s apart:
//             stand in the dark ones, then step across.
//   ZENITH    a ray from the Halo burning where you stand, chasing slower than
//             you run (the price of camping on a shard).
//   HALO      the 360 sweep at chest height: slide under it. It shatters every
//             other pillar.
//   MIRAGES   afterimages that jab once for half; one hit kills them.
//   JUDGMENT  every seam burns but a ring round him: be beside him.
// No OpenGL or audio: Gameplay_Absolute.h draws and sounds them.
// =============================================================================
#include "Level.h"
#include "Sword.h"
#include <vector>
#include <algorithm>

struct AbsoluteHazards {
    static constexpr float RAY_TIME = 1.f, RAY_CHASE = 5.5f, RAY_DPS = 25.f, RAY_HALF = 0.6f, RAY_PARRY = 0.15f, RAY_REFLECT = 250.f,
                           STOMP_SPEED = 18.f, STOMP_H = 1.f, STOMP_DAMAGE = 20.f, STOMP_MAX = 46.f,
                           SERPENT_SPEED = 6.5f, SERPENT_TURN = 1.6f, SERPENT_LIFE = 6.f, SERPENT_DAMAGE = 20.f, SERPENT_PARRY = 300.f,
                           SEAM_WARN = 0.9f, SEAM_GAP = 0.7f, SEAM_DAMAGE = 25.f, SEAM_CELL = 6.f,
                           ZRAY_TIME = 2.5f, ZRAY_CHASE = 5.f, ZRAY_R = 1.6f, ZRAY_DPS = 20.f,
                           HALO_TIME = Enemy::AB_HALO_TIME, HALO_LO = 1.0f, HALO_HI = 1.6f, HALO_DAMAGE = 30.f, HALO_REACH = 46.f,
                           MIRAGE_TELL = 0.5f, MIRAGE_DAMAGE = 8.f, MIRAGE_R = 0.6f,
                           JUDGE_WARN = Enemy::AB_JUDGE_WARN, JUDGE_SAFE = 6.f, JUDGE_DAMAGE = 60.f;
    // A hit on the player: `tick` damage is already per tick (a beam)
    struct Hit { float damage; glm::vec3 from; Sword::Strike kind; bool tick; };
    struct Ray     { glm::vec3 from, at; float t = 0.f; };
    struct Stomp   { glm::vec3 at; float r = 0.f; bool hit = false; };
    struct Serpent { glm::vec3 pos, dir; float t = 0.f; bool back = false; };
    struct Seams   { glm::vec3 centre; int first; float t = 0.f; bool done0 = false, done1 = false; };
    struct ZRay    { glm::vec3 halo, at; float t = 0.f; };
    struct Halo    { glm::vec3 centre; float floorY, t = 0.f, prevA = 0.f; bool hit = false; };
    struct Mirage  { glm::vec3 at; float t = 0.f; bool struck = false; };
    struct Judge   { glm::vec3 centre, boss; float t = 0.f; bool done = false; };
    std::vector<Ray> rays; std::vector<Stomp> stomps; std::vector<Serpent> serpents; std::vector<Seams> seams;
    std::vector<ZRay> zrays; std::vector<Halo> halos; std::vector<Mirage> mirages; std::vector<Judge> judges;
    int shatterThisTick = 0;   // the pillars the halo sweep passed this tick (a mask)

    void clear() {
        rays.clear(); stomps.clear(); serpents.clear(); seams.clear();
        zrays.clear(); halos.clear(); mirages.clear(); judges.clear(); shatterThisTick = 0;
    }
    void addRay(glm::vec3 from, glm::vec3 at)       { rays.push_back({from, at}); }
    void addStomp(glm::vec3 at)                     { stomps.push_back({at}); }
    void addSerpent(glm::vec3 at, glm::vec3 dir)    { float l = glm::length(dir); serpents.push_back({at, l > 1e-4f ? dir / l : glm::vec3{0, 0, 1}}); }
    void addSeams(glm::vec3 centre, int first)      { seams.push_back({centre, first & 1}); }
    void addZenith(glm::vec3 halo, glm::vec3 at)    { zrays.push_back({halo, at}); }
    void addHalo(glm::vec3 centre, float floorY)    { halos.push_back({centre, floorY}); }
    void addMirage(glm::vec3 at)                    { mirages.push_back({at}); }
    void addJudge(glm::vec3 centre, glm::vec3 boss) { judges.push_back({centre, boss}); }

    // Which of the seams' two waves a point's cell burns in
    static int seamParity(glm::vec3 c, glm::vec3 p) {
        return ((int)std::floor((p.x - c.x) / SEAM_CELL) + (int)std::floor((p.z - c.z) / SEAM_CELL)) & 1;
    }
    // A standing pillar between a and b
    static bool blocked(const LevelData* lv, glm::vec3 a, glm::vec3 b) {
        if (!lv) return false;
        for (size_t i = 0; i < lv->zenith.pillarWalls.size(); ++i)
            if (lv->zenith.pillarUp[i] && b.y < lv->walls[lv->zenith.pillarWalls[i]].box.max.y &&   // (not the pillar you stand on)
                segmentHitsBox(a, b, lv->walls[lv->zenith.pillarWalls[i]].box)) return true;
        return false;
    }
    static glm::vec3 chase(glm::vec3 at, glm::vec3 target, float speed, float dt) {
        glm::vec3 d = target - at; d.y = 0.f;
        float l = glm::length(d);
        return l < 1e-4f ? at : at + d / l * std::min(l, speed * dt);
    }

    std::vector<Hit> update(float dt, glm::vec3 feet, float height, bool grounded, const LevelData* lv, glm::vec3 boss) {
        std::vector<Hit> out;
        const glm::vec3 chest = feet + glm::vec3{0.f, height * 0.6f, 0.f};
        shatterThisTick = 0;
        for (auto& r : rays) {
            r.t += dt;
            r.at = chase(r.at, chest, RAY_CHASE, dt); r.at.y = chest.y;
            glm::vec3 dir = r.at - r.from;
            float l = glm::length(dir);
            if (l < 1e-3f) continue;
            dir /= l;
            float along = glm::dot(chest - r.from, dir);
            glm::vec3 q = r.from + dir * std::max(along, 0.f);
            if (along > 0.f && glm::length(chest - q) < RAY_HALF + 0.4f && !blocked(lv, r.from, chest))
                out.push_back({RAY_DPS * dt, r.from, Sword::Strike::BEAM, true});
        }
        rays.erase(std::remove_if(rays.begin(), rays.end(), [](const Ray& r) { return r.t >= RAY_TIME; }), rays.end());
        for (auto& s : stomps) {
            float was = s.r; s.r += STOMP_SPEED * dt;
            float d = glm::length(glm::vec2{feet.x - s.at.x, feet.z - s.at.z});
            if (!s.hit && d >= was - 0.5f && d < s.r + 0.5f && grounded && feet.y < s.at.y + STOMP_H) {
                s.hit = true; out.push_back({STOMP_DAMAGE, s.at, Sword::Strike::GOLD, false});
            }
        }
        stomps.erase(std::remove_if(stomps.begin(), stomps.end(), [](const Stomp& s) { return s.r > STOMP_MAX; }), stomps.end());
        for (auto& s : serpents) {
            s.t += dt;
            glm::vec3 target = s.back ? boss + glm::vec3{0.f, 1.3f, 0.f} : chest;
            glm::vec3 want = target - s.pos;
            float wl = glm::length(want);
            if (wl > 1e-3f) {
                want /= wl;
                float ang = std::acos(glm::clamp(glm::dot(s.dir, want), -1.f, 1.f)), turn = (s.back ? 8.f : SERPENT_TURN) * dt;
                if (ang <= turn) s.dir = want;
                else {
                    glm::vec3 axis = glm::cross(s.dir, want);
                    if (glm::length(axis) < 1e-5f) axis = {0.f, 1.f, 0.f};
                    s.dir = glm::normalize(s.dir * std::cos(turn) + glm::cross(glm::normalize(axis), s.dir) * std::sin(turn));
                }
            }
            s.pos += s.dir * (s.back ? 30.f : SERPENT_SPEED) * dt;
            if (!s.back && glm::length(s.pos - chest) < 0.9f) { out.push_back({SERPENT_DAMAGE, s.pos, Sword::Strike::WHITE, false}); s.t = 1e9f; }
        }
        serpents.erase(std::remove_if(serpents.begin(), serpents.end(), [](const Serpent& s) { return s.t >= SERPENT_LIFE; }), serpents.end());
        for (auto& s : seams) {
            s.t += dt;
            auto burn = [&](int parity, bool& done, float at) {
                if (done || s.t < at) return;
                done = true;
                if (grounded && feet.y < s.centre.y + 0.6f && seamParity(s.centre, feet) == parity &&
                    glm::length(glm::vec2{feet.x - s.centre.x, feet.z - s.centre.z}) < 44.f)
                    out.push_back({SEAM_DAMAGE, feet, Sword::Strike::GOLD, false});
            };
            burn(s.first, s.done0, SEAM_WARN);
            burn(s.first ^ 1, s.done1, SEAM_WARN + SEAM_GAP);
        }
        seams.erase(std::remove_if(seams.begin(), seams.end(), [](const Seams& s) { return s.done1; }), seams.end());
        for (auto& z : zrays) {
            z.t += dt;
            z.at = chase(z.at, feet, ZRAY_CHASE, dt);
            if (glm::length(glm::vec2{feet.x - z.at.x, feet.z - z.at.z}) < ZRAY_R) out.push_back({ZRAY_DPS * dt, z.halo, Sword::Strike::BEAM, true});
        }
        zrays.erase(std::remove_if(zrays.begin(), zrays.end(), [](const ZRay& z) { return z.t >= ZRAY_TIME; }), zrays.end());
        for (auto& h : halos) {   // a beam from the centre turning a full circle in HALO_TIME, at chest height
            h.t += dt;
            float a = 6.2831853f * std::min(1.f, h.t / HALO_TIME), was = h.prevA;
            h.prevA = a;
            auto swept = [&](float x, float z) {
                float pa = std::atan2(z - h.centre.z, x - h.centre.x);
                if (pa < 0.f) pa += 6.2831853f;
                return pa >= was && pa < a;
            };
            if (!h.hit && swept(feet.x, feet.z) && glm::length(glm::vec2{feet.x - h.centre.x, feet.z - h.centre.z}) < HALO_REACH &&
                feet.y + height > h.floorY + HALO_LO && feet.y < h.floorY + HALO_HI) {
                h.hit = true; out.push_back({HALO_DAMAGE, h.centre, Sword::Strike::BEAM, false});
            }
            if (lv) for (size_t i = 0; i < lv->zenith.pillarHome.size(); i += 2) {   // every other pillar shatters
                const AABB& b = lv->zenith.pillarHome[i];
                if (lv->zenith.pillarUp[i] && swept((b.min.x + b.max.x) * 0.5f, (b.min.z + b.max.z) * 0.5f)) shatterThisTick |= 1 << i;
            }
        }
        halos.erase(std::remove_if(halos.begin(), halos.end(), [](const Halo& h) { return h.t >= HALO_TIME; }), halos.end());
        for (auto& m : mirages) {   // each jabs once, MIRAGE_TELL after it appears
            m.t += dt;
            if (!m.struck && m.t >= MIRAGE_TELL) {
                m.struck = true;
                if (glm::length(glm::vec2{feet.x - m.at.x, feet.z - m.at.z}) < 3.2f && std::fabs(feet.y - m.at.y) < 2.f)
                    out.push_back({MIRAGE_DAMAGE, m.at, Sword::Strike::WHITE, false});
            }
        }
        mirages.erase(std::remove_if(mirages.begin(), mirages.end(), [](const Mirage& m) { return m.t > MIRAGE_TELL + 0.3f; }), mirages.end());
        for (auto& j : judges) {
            j.t += dt;
            if (!j.done && j.t >= JUDGE_WARN) {
                j.done = true;
                if (glm::length(glm::vec2{feet.x - j.boss.x, feet.z - j.boss.z}) > JUDGE_SAFE) out.push_back({JUDGE_DAMAGE, j.boss, Sword::Strike::GOLD, false});
            }
        }
        judges.erase(std::remove_if(judges.begin(), judges.end(), [](const Judge& j) { return j.done; }), judges.end());
        return out;
    }

    int  haloShatterMask() const { return shatterThisTick; }
    // A ray just fired, caught by the sword's parry: it ends (the game sends its damage back)
    bool rayParry(int i) {
        if (i < 0 || i >= (int)rays.size() || rays[i].t > RAY_PARRY) return false;
        rays.erase(rays.begin() + i);
        return true;
    }
    int serpentNear(glm::vec3 p, float r) const {
        for (int i = 0; i < (int)serpents.size(); ++i) if (!serpents[i].back && glm::length(serpents[i].pos - p) < r) return i;
        return -1;
    }
    void reflectSerpent(int i) { if (i >= 0 && i < (int)serpents.size()) { serpents[i].back = true; serpents[i].t = 0.f; } }
    // Parried serpents reaching him this tick (they're spent)
    int reflectedHits(glm::vec3 boss, float r) {
        int n = 0;
        for (auto it = serpents.begin(); it != serpents.end();)
            if (it->back && glm::length(it->pos - boss) < r) { ++n; it = serpents.erase(it); } else ++it;
        return n;
    }
    int mirageAlong(glm::vec3 o, glm::vec3 d, float maxT) const {
        for (int i = 0; i < (int)mirages.size(); ++i) {
            AABB b{mirages[i].at + glm::vec3{-MIRAGE_R, 0.f, -MIRAGE_R}, mirages[i].at + glm::vec3{MIRAGE_R, 2.6f, MIRAGE_R}};
            float t = rayBoxHit(o, d, b);
            if (t >= 0.f && t <= maxT) return i;
        }
        return -1;
    }
    void killMirage(int i) { if (i >= 0 && i < (int)mirages.size()) mirages.erase(mirages.begin() + i); }
};

// A retry in the Zenith: every pillar and shard back, nothing of his left
inline void resetZenithFight(LevelData& lv, AbsoluteHazards& h) { h.clear(); lv.restoreZenith(); }
