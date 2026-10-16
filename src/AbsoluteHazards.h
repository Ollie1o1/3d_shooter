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
//   FAN       THUNDERCLAP: seven flames roll out along the floor from under
//             him in a fan at you. Up close they're shoulder to shoulder (jump
//             one); further out the gaps open (stand in one). A pillar stops a
//             flame.
// MORNINGSTAR's (his second form):
//   WAVES     SUNDER's ring of light rolling out along the floor (jump it); an
//             aftershock is the same ring again, later, from the same place;
//             the COLLAPSE is the ring of fire closing in from the edge.
//   FISSURES  a crack ripping along the floor at you (step off its line); its
//             aftershock: the whole line erupts again.
//   LANES     the ring of fire's flares: seven lanes lit across the floor, one
//             on you, then walls of fire run down them (stand between).
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
                           JUDGE_WARN = Enemy::AB_JUDGE_WARN, JUDGE_SAFE = 6.f, JUDGE_DAMAGE = 60.f,
                           FAN_SPEED = 19.f, FAN_HALF = 0.9f, FAN_R = 1.1f, FAN_H = 1.3f, FAN_MAX = 46.f, FAN_DAMAGE = 24.f;
    static constexpr int FAN_N = 7;
    static constexpr float WAVE_H = 1.2f, RING_SPEED = 20.f, RING_MAX = 46.f, RING_DAMAGE = 28.f, AFTER_DELAY = 0.6f, AFTER_DAMAGE = 22.f,
                           FIS_LEN = 44.f, FIS_SPEED = 48.f, FIS_HALF = 1.1f, FIS_BURN = 0.45f, FIS_DAMAGE = 30.f, FIS_AFTER = 0.35f, FIS_AFTER_LEN = 0.35f,
                           LANE_GAP = 9.f, LANE_HALF = 1.6f, LANE_WARN = 1.f, LANE_SPEED = 36.f, LANE_R = 46.f, LANE_DAMAGE = 26.f, LANE_H = 3.f,
                           COLLAPSE_R = 44.f, COLLAPSE_SPEED = 14.f, COLLAPSE_DAMAGE = 40.f;
    static constexpr int LANE_N = 7;
    // A hit on the player: `tick` damage is already per tick (a beam)
    struct Hit { float damage; glm::vec3 from; Sword::Strike kind; bool tick; int src = 0; };   // src (tuning): MORNINGSTAR's 1 ring, 2 fissure, 3 lane, 4 collapse
    struct Ray     { glm::vec3 from, at; float t = 0.f; };
    struct Stomp   { glm::vec3 at; float r = 0.f; bool hit = false; };
    struct Serpent { glm::vec3 pos, dir; float t = 0.f; bool back = false; };
    struct Seams   { glm::vec3 centre; int first; float t = 0.f; bool done0 = false, done1 = false; };
    struct ZRay    { glm::vec3 halo, at; float t = 0.f; };
    struct Halo    { glm::vec3 centre; float floorY, t = 0.f, prevA = 0.f; bool hit = false; };
    struct Mirage  { glm::vec3 at; float t = 0.f; bool struck = false; };
    struct Judge   { glm::vec3 centre, boss; float t = 0.f; bool done = false; };
    struct Fan     { glm::vec3 at; glm::vec3 dir[FAN_N]; bool out[FAN_N] = {}; float r = 0.f; bool hit = false; };   // out: a pillar stopped it
    struct Wave    { glm::vec3 at; float r, speed, maxR, damage, delay; bool inward = false, hit = false; };   // (inward: from maxR to 0)
    struct Fissure { glm::vec3 from, dir; float len, t = 0.f; bool after = false, hit = false, afterHit = false; };
    struct Lanes   { glm::vec3 centre, dir, side; float off[LANE_N]; float t = 0.f; bool hit = false; };   // t < 0: not lit yet
    std::vector<Ray> rays; std::vector<Stomp> stomps; std::vector<Serpent> serpents; std::vector<Seams> seams;
    std::vector<ZRay> zrays; std::vector<Halo> halos; std::vector<Mirage> mirages; std::vector<Judge> judges;
    std::vector<Fan> fans;
    std::vector<Wave> waves; std::vector<Fissure> fissures; std::vector<Lanes> lanes;
    int shatterThisTick = 0;   // the pillars the halo sweep passed this tick (a mask)

    void clear() {
        rays.clear(); stomps.clear(); serpents.clear(); seams.clear();
        zrays.clear(); halos.clear(); mirages.clear(); judges.clear(); fans.clear(); shatterThisTick = 0;
        waves.clear(); fissures.clear(); lanes.clear();
    }
    void addRay(glm::vec3 from, glm::vec3 at)       { rays.push_back({from, at}); }
    void addStomp(glm::vec3 at)                     { stomps.push_back({at}); }
    void addSerpent(glm::vec3 at, glm::vec3 dir)    { float l = glm::length(dir); serpents.push_back({at, l > 1e-4f ? dir / l : glm::vec3{0, 0, 1}}); }
    void addSeams(glm::vec3 centre, int first)      { seams.push_back({centre, first & 1}); }
    void addZenith(glm::vec3 halo, glm::vec3 at)    { zrays.push_back({halo, at}); }
    void addHalo(glm::vec3 centre, float floorY)    { halos.push_back({centre, floorY}); }
    void addMirage(glm::vec3 at)                    { mirages.push_back({at}); }
    void addJudge(glm::vec3 centre, glm::vec3 boss) { judges.push_back({centre, boss}); }
    // A fan from `at` (the floor under him) centred on `toward`
    void addFan(glm::vec3 at, glm::vec3 toward) {
        Fan f; f.at = at;
        glm::vec3 d = toward - at; d.y = 0.f;
        float base = glm::length(d) > 0.1f ? std::atan2(d.x, d.z) : 0.f;
        for (int k = 0; k < FAN_N; ++k) {
            float a = base + FAN_HALF * (2.f * k / (FAN_N - 1) - 1.f);
            f.dir[k] = {std::sin(a), 0.f, std::cos(a)};
        }
        fans.push_back(f);
    }
    // MORNINGSTAR's: a ring rolling out from `at` (after `delay`); the ring of fire closing in on `centre`
    void addRing(glm::vec3 at, float delay = 0.f, float damage = RING_DAMAGE) { waves.push_back({at, 0.f, RING_SPEED, RING_MAX, damage, delay}); }
    void addCollapse(glm::vec3 centre) { Wave w{centre, COLLAPSE_R, COLLAPSE_SPEED, COLLAPSE_R, COLLAPSE_DAMAGE, 0.f}; w.inward = true; waves.push_back(w); }
    // A fissure from `from` toward `toward`, as far as the floor's edge (radius 43 round `centre`)
    void addFissure(glm::vec3 from, glm::vec3 toward, glm::vec3 centre, bool after) {
        glm::vec3 d = toward - from; d.y = 0.f;
        float l = glm::length(d);
        d = l > 0.1f ? d / l : glm::vec3{0.f, 0.f, 1.f};
        glm::vec2 o{from.x - centre.x, from.z - centre.z}, v{d.x, d.z};   // |o + v s| = 43
        float b = glm::dot(o, v), c = glm::dot(o, o) - 43.f * 43.f, disc = b * b - c;
        float len = disc > 0.f ? std::min(FIS_LEN, -b + std::sqrt(disc)) : FIS_LEN;
        Fissure f{from, d, std::max(len, 4.f)}; f.after = after;
        fissures.push_back(f);
    }
    // The flares: lanes across the floor along `dir` (from the far side of the ring), one centred on `on`
    void addLanes(glm::vec3 centre, glm::vec3 on, glm::vec3 dir, float delay = 0.f) {
        dir.y = 0.f; dir = glm::length(dir) > 0.1f ? glm::normalize(dir) : glm::vec3{0.f, 0.f, 1.f};
        Lanes L; L.centre = centre; L.dir = dir; L.side = {-dir.z, 0.f, dir.x}; L.t = -delay;
        float mine = glm::dot(on - centre, L.side);
        for (int k = 0; k < LANE_N; ++k) L.off[k] = mine + (k - LANE_N / 2) * LANE_GAP;
        lanes.push_back(L);
    }
    // MORNINGSTAR's moves landing, as hazards (the game and the boss sim both call this): his events,
    // the floor's centre, where you are, which way he faces
    static glm::vec3 flaresDir(glm::vec3 centre, glm::vec3 you, float yaw) {   // lanes running past you (across your line to the centre)
        glm::vec3 r = you - centre; r.y = 0.f;
        if (glm::length(r) < 2.f) r = glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
        r = glm::normalize(r);
        return glm::vec3{-r.z, 0.f, r.x};
    }
    void morningEvents(const EnemyEvents& ev, glm::vec3 centre, glm::vec3 you, float yaw) {
        if (ev.msSwallow) clear();
        if (ev.msSunder) { addRing(ev.abFrom); if (ev.msAfter) addRing(ev.abFrom, AFTER_DELAY, AFTER_DAMAGE); }
        if (ev.msFissure) addFissure(ev.abFrom, ev.abTo, centre, ev.msAfter);
        if (ev.msDrop && ev.msAfter) addRing(ev.abTo, 0.35f, AFTER_DAMAGE);
        if (ev.msGrasp && ev.msAfter) addRing(ev.abTo, 0.5f, AFTER_DAMAGE);
        if (ev.msFlares) {
            glm::vec3 d = flaresDir(centre, you, yaw);
            addLanes(centre, you, d);
            if (ev.msAfter) addLanes(centre, you, glm::vec3{-d.z, 0.f, d.x}, 0.9f);   // RISING: a second set across the first
        }
        if (ev.msFlaresUnder) addLanes(centre, you, flaresDir(centre, you, yaw), 0.4f);
        if (ev.msCollapse) addCollapse(centre);
    }
    // Where a lane's wall of fire is along it (from -LANE_R), and is it running?
    static float laneFront(const Lanes& L) { return -LANE_R + std::max(0.f, L.t - LANE_WARN) * LANE_SPEED; }

    // Where flame k of a fan is, on whatever floor is under it (the Dais is higher)
    static glm::vec3 fanFlame(const Fan& f, int k, const LevelData* lv) {
        glm::vec3 p = f.at + f.dir[k] * f.r;
        if (lv && lv->zenith.built) {
            const glm::vec3 c = lv->zenith.centre;
            p.y = c.y + (glm::length(glm::vec2{p.x - c.x, p.z - c.z}) < Enemy::AB_DAIS_R ? Enemy::AB_DAIS_UP : 0.f);
        }
        return p;
    }

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
                s.hit = true; out.push_back({STOMP_DAMAGE, s.at, Sword::Strike::AREA, false});
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
                    out.push_back({SEAM_DAMAGE, feet, Sword::Strike::AREA, false});
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
                h.hit = true; out.push_back({HALO_DAMAGE, h.centre, Sword::Strike::AREA, false});
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
                if (glm::length(glm::vec2{feet.x - j.boss.x, feet.z - j.boss.z}) > JUDGE_SAFE) out.push_back({JUDGE_DAMAGE, j.boss, Sword::Strike::AREA, false});
            }
        }
        judges.erase(std::remove_if(judges.begin(), judges.end(), [](const Judge& j) { return j.done; }), judges.end());
        for (auto& f : fans) {
            f.r += FAN_SPEED * dt;
            for (int k = 0; k < FAN_N; ++k) {
                if (f.out[k]) continue;
                glm::vec3 p = fanFlame(f, k, lv);
                if (blocked(lv, f.at + glm::vec3{0.f, 0.5f, 0.f}, p + glm::vec3{0.f, 0.5f, 0.f})) { f.out[k] = true; continue; }
                if (!f.hit && glm::length(glm::vec2{feet.x - p.x, feet.z - p.z}) < FAN_R + 0.4f && feet.y < p.y + FAN_H) {
                    f.hit = true; out.push_back({FAN_DAMAGE, p, Sword::Strike::AREA, false});
                }
            }
        }
        fans.erase(std::remove_if(fans.begin(), fans.end(), [](const Fan& f) { return f.r > FAN_MAX; }), fans.end());
        // ---- MORNINGSTAR's (heights from the floor under you: the Dais is higher) ----
        float under = feet.y;
        if (lv && lv->zenith.built) {
            const glm::vec3 c = lv->zenith.centre;
            under = c.y + (glm::length(glm::vec2{feet.x - c.x, feet.z - c.z}) < Enemy::AB_DAIS_R ? Enemy::AB_DAIS_UP : 0.f);
        }
        const float up = feet.y - under;   // how high you are off it
        for (auto& w : waves) {
            if (w.delay > 0.f) { w.delay -= dt; continue; }
            float was = w.r; w.r += (w.inward ? -w.speed : w.speed) * dt;
            float d = glm::length(glm::vec2{feet.x - w.at.x, feet.z - w.at.z});
            float lo = std::min(was, w.r) - 0.5f, hi = std::max(was, w.r) + 0.5f;
            if (!w.hit && d >= lo && d < hi && up < WAVE_H) { w.hit = true; out.push_back({w.damage, w.at, Sword::Strike::AREA, false, w.inward ? 4 : 1}); }
        }
        waves.erase(std::remove_if(waves.begin(), waves.end(), [](const Wave& w) { return w.inward ? w.r < 0.f : w.r > w.maxR; }), waves.end());
        for (auto& f : fissures) {
            f.t += dt;
            glm::vec3 rel = feet - f.from;
            float along = glm::dot(rel, f.dir), perp = glm::length(glm::vec2{rel.x - f.dir.x * along, rel.z - f.dir.z * along});
            bool on = along > -0.5f && along < f.len && perp < FIS_HALF && up < 1.5f;
            float since = f.t - std::max(along, 0.f) / FIS_SPEED;   // how long ago the crack passed here
            if (!f.hit && on && since >= 0.f && since < FIS_BURN) { f.hit = true; out.push_back({FIS_DAMAGE, f.from, Sword::Strike::AREA, false, 2}); }
            float ta = f.len / FIS_SPEED + FIS_AFTER;              // the aftershock: the whole line at once
            if (f.after && !f.afterHit && on && f.t >= ta && f.t < ta + FIS_AFTER_LEN) { f.afterHit = true; out.push_back({AFTER_DAMAGE, f.from, Sword::Strike::AREA, false, 2}); }
        }
        fissures.erase(std::remove_if(fissures.begin(), fissures.end(), [](const Fissure& f) {
            return f.t > f.len / FIS_SPEED + (f.after ? FIS_AFTER + FIS_AFTER_LEN : FIS_BURN); }), fissures.end());
        for (auto& L : lanes) {
            float was = laneFront(L);
            L.t += dt;
            if (L.t < LANE_WARN) continue;
            float now = laneFront(L);
            glm::vec3 rel = feet - L.centre;
            float along = glm::dot(rel, L.dir), across = glm::dot(rel, L.side);
            bool inLane = false;
            for (int k = 0; k < LANE_N; ++k) if (std::fabs(across - L.off[k]) < LANE_HALF + 0.3f) inLane = true;
            if (!L.hit && inLane && along >= was - 1.f && along < now + 1.f && up < LANE_H &&
                glm::length(glm::vec2{rel.x, rel.z}) < 45.f) { L.hit = true; out.push_back({LANE_DAMAGE, feet, Sword::Strike::AREA, false, 3}); }
        }
        lanes.erase(std::remove_if(lanes.begin(), lanes.end(), [](const Lanes& L) { return laneFront(L) > LANE_R; }), lanes.end());
        return out;
    }

    int  haloShatterMask() const { return shatterThisTick; }
    // A ray still in its first 0.15 s (a sword parry sends it back), or -1
    int  freshRay() const { for (int i = 0; i < (int)rays.size(); ++i) if (rays[i].t <= RAY_PARRY) return i; return -1; }
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
