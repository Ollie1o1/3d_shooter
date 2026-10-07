#pragma once
// =============================================================================
// ArenaShifts.h — arenas that change as their fight goes on. No OpenGL.
//
//   NIGHTFALL (the Yard)     the sun goes down wave by wave: dusk on wave 2,
//                            night for wave 3
//   LAVA_RISE (the Foundry)  on the last wave the lava channels widen from 3 m
//                            to 9 m, cutting the floor into islands; the
//                            catwalks are the safe way round, and the lava is
//                            a bigger trap to lure enemies into
//   SPEED_UP  (the Spire)    the platforms move faster each wave (x1.3, x1.6)
//   OVERLOAD  (the Core)     during its SURVIVE wave the reactor fires a ring
//                            of energy along the ground every few seconds
//                            (warning first): jump it, or get up on the
//                            walkways. It burns enemies on foot too, and the
//                            lights go to alarm red.
//   SOLAR     (the Orrery)   the sun throws arms of light (24 degree wedges) that
//                            turn about it: warn, burn, rest; the rings speed up
//                            each wave and a second arm joins on the last
//   FLOOD     (the Nave)     the water rises a step each time a wave is
//                            cleared (Arena::floodLevels), over FLOOD_TIME
//   DESCENT   (the Descent)  the cage rides down to the next stop once a wave
//                            is cleared and you're aboard (LevelData::Lift)
//
// It edits the level itself (each arena's theme, its lava hazards, its
// movers' periods), so lighting still blends down the corridors and the
// player and enemy code needs no special cases. reset() puts all of it back
// (a retry, a new run). GameplayState draws the extra lava and the rings
// and applies their damage (Gameplay_Shifts.h).
// =============================================================================
#include "Level.h"
#include <vector>
#include <algorithm>
#include <cmath>

enum class FlarePhase { OFF, WARN, BURN };

class ArenaShifts {
public:
    static constexpr float LAVA_WIDEN   = 3.f;    // m added each side of a channel
    static constexpr float LAVA_TIME    = 6.f;    // s to rise fully
    static constexpr float PULSE_EVERY  = 4.5f;   // s between overload rings
    static constexpr float PULSE_WARN   = 1.0f;   // s of warning before one fires
    static constexpr float PULSE_SPEED  = 11.f;   // m/s outward
    static constexpr float PULSE_RANGE  = 40.f;   // m before it fades
    static constexpr float PULSE_HEIGHT = 1.4f;   // above the floor it sweeps; stand higher and it passes under
    static constexpr float PULSE_DAMAGE = 18.f;   // to the player; enemies on foot take double
    static constexpr float FLOOD_TIME   = 6.f;    // s for one rise of the water
    static constexpr float FLARE_HALF   = 0.2094395f;   // half the wedge: 12 degrees
    static constexpr float FLARE_SPIN   = 0.2617994f;   // 15 degrees a second
    static constexpr float FLARE_WARN   = 1.5f, FLARE_BURN = 5.f, FLARE_REST = 4.f;
    static constexpr float FLARE_CYCLE  = FLARE_WARN + FLARE_BURN + FLARE_REST;

    // Live state, read by the renderer and the HUD
    std::vector<float> night;        // per arena 0..1 (NIGHTFALL)
    std::vector<float> lava;         // per arena 0..1 (LAVA_RISE)
    std::vector<float> speed;        // per arena, current mover speed factor (SPEED_UP)
    float alarm = 0.f;               // 0..1 (OVERLOAD lighting)
    struct Ring { glm::vec3 centre; float radius, prev; };
    std::vector<Ring> rings;         // OVERLOAD rings in flight
    float pulseClock = 0.f;          // counts down to the next ring
    bool  overloading = false;
    bool  pulseFired = false;
    bool  bossDriven = false;        // THE WARDEN is driving the rings (bossPulse)
    float pulseEvery = PULSE_EVERY;
    glm::vec3 bossCentre{0.f};        // this update: a ring went out (for its sound)
    bool  lavaStarted = false;       // this update: the lava began to rise (for its banner)
    bool  floodStarted = false;      // this update: the water began to rise (banner, rumble)
    // SOLAR: the flare's arms (all on one cycle, spread evenly round the sun)
    int   flareArms = 1;
    float flareAngle = 0.f;          // radians, about the sun, from +X toward +Z
    float flareClock = 0.f;          // into the warn/burn/rest cycle

    FlarePhase flarePhase() const {
        if (flareClock < FLARE_WARN) return FlarePhase::WARN;
        if (flareClock < FLARE_WARN + FLARE_BURN) return FlarePhase::BURN;
        return FlarePhase::OFF;
    }
    // Is p inside one of the arms' wedges (whatever the phase)?
    bool inFlareWedge(const Arena& ar, glm::vec3 p) const {
        glm::vec2 d{p.x - ar.sunPos.x, p.z - ar.sunPos.z};
        if (glm::dot(d, d) < 1e-4f) return false;
        float ang = std::atan2(d.y, d.x);
        for (int k = 0; k < flareArms; ++k) {
            float arm = flareAngle + k * 6.2831853f / flareArms;
            if (std::fabs(std::remainder(ang - arm, 6.2831853f)) <= FLARE_HALF) return true;
        }
        return false;
    }
    // Does the burning flare reach p (feet)? In a wedge, and nothing solid
    // between the sun's axis and p's chest: pillars and walls (not the rings,
    // which turn through that line and would shade the whole terrace)
    bool flareHits(const LevelData& L, int a, glm::vec3 p) const {
        const Arena& ar = L.arenas[a];
        if (ar.shift != ArenaShift::SOLAR || flarePhase() != FlarePhase::BURN || !inFlareWedge(ar, p)) return false;
        glm::vec3 chest = p + glm::vec3{0.f, 1.2f, 0.f};
        glm::vec3 from{ar.sunPos.x, chest.y, ar.sunPos.z};
        glm::vec3 d = chest - from;
        float len = glm::length(d);
        if (len < 1e-3f) return true;
        d /= len;
        for (auto& w : L.walls) {
            if (w.dynamic) continue;
            float t = rayBoxHit(from, d, w.box);
            if (t > 0.f && t < len - 0.3f) return false;
        }
        return true;
    }

    // Remember the level as built
    void capture(const LevelData& L) {
        int n = (int)L.arenas.size();
        baseTheme.clear(); for (auto& a : L.arenas) baseTheme.push_back(a.theme);
        baseHazard.clear(); for (auto& h : L.hazards) baseHazard.push_back(h.box);
        basePeriod.clear(); for (auto& m : L.movers) basePeriod.push_back({m.period, m.phase});
        night.assign(n, 0.f); lava.assign(n, 0.f); speed.assign(n, 1.f);
        nightTarget.assign(n, 0.f); lavaTarget.assign(n, 0.f);
        baseWater.clear(); for (auto& w : L.water) baseWater.push_back(w.level);
        waterTarget = baseWater; waterRate.assign(baseWater.size(), 0.f); rising.assign(baseWater.size(), false);
    }

    // Back to the level as built (a retry or a new run)
    void reset(LevelData& L) {
        L.lift.reset(); L.lift.update(0.f, L);   // the Descent's cage back at the top
        for (int a = 0; a < (int)L.arenas.size(); ++a) {
            L.arenas[a].theme = baseTheme[a];
            night[a] = nightTarget[a] = lava[a] = lavaTarget[a] = 0.f;
            if (L.arenas[a].shift == ArenaShift::SPEED_UP || L.arenas[a].shift == ArenaShift::SOLAR) {
                for (int i = 0; i < (int)L.movers.size(); ++i)
                    if (moverIn(L, i, a)) { L.movers[i].period = basePeriod[i].first; L.movers[i].phase = basePeriod[i].second; }
                speed[a] = 1.f;
            }
        }
        for (int i = 0; i < (int)L.hazards.size(); ++i) L.hazards[i].box = baseHazard[i];
        alarm = 0.f; rings.clear(); overloading = false; bossPulseOff();
        flareArms = 1; flareAngle = 0.f; flareClock = 0.f;
        for (int i = 0; i < (int)L.water.size() && i < (int)baseWater.size(); ++i) {
            L.water[i].level = waterTarget[i] = baseWater[i];
            waterRate[i] = 0.f; rising[i] = false;
        }
    }

    // A wave was cleared in arena a: a FLOOD arena's water heads for the
    // next wave's level
    // DESCENT: the stop each wave is fought at (the top is stop 0)
    static int descentStop(int wave) { return wave + 1; }
    void onWaveCleared(LevelData& L, int a, int nextWave) {
        if (a >= 0 && a < (int)L.arenas.size() && L.arenas[a].shift == ArenaShift::DESCENT) L.lift.request(descentStop(nextWave));
        const Arena& ar = L.arenas[a];
        if (ar.shift != ArenaShift::FLOOD || nextWave >= (int)ar.floodLevels.size()) return;
        for (int i = 0; i < (int)L.water.size(); ++i)
            if (inArena(ar, L.water[i].box)) {
                waterTarget[i] = ar.floodLevels[nextWave];
                waterRate[i] = std::fabs(waterTarget[i] - L.water[i].level) / FLOOD_TIME;
                rising[i] = false;
            }
    }

    // A wave began in arena a: set where its shift is heading. `of`: how many
    // waves make the cycle (ENDLESS passes 3 and the wave mod 3, so the Yard
    // goes day-dusk-night-day and the Foundry's lava rises and falls); -1: the
    // arena's own count
    void onWave(LevelData& L, int a, int wave, const WaveGoal& goal, float moverClock, int of = -1) {
        int waves = of > 0 ? of : (int)L.arenas[a].waves.size();
        switch (L.arenas[a].shift) {
        case ArenaShift::NIGHTFALL: nightTarget[a] = waves > 1 ? (float)wave / (waves - 1) : 0.f; break;
        case ArenaShift::LAVA_RISE: lavaTarget[a] = wave == waves - 1 ? 1.f : 0.f; break;
        case ArenaShift::SPEED_UP:  setSpeed(L, a, wave == 0 ? 1.f : wave == 1 ? 1.3f : 1.6f, moverClock); break;
        case ArenaShift::SOLAR:
            setSpeed(L, a, wave == 0 ? 1.f : wave == 1 ? 1.3f : 1.6f, moverClock);
            flareArms = wave >= 2 ? 2 : 1;
            flareClock = 0.f;
            break;
        case ArenaShift::OVERLOAD:
            overloading = goal.kind == WaveGoal::SURVIVE;
            pulseClock = PULSE_EVERY;
            break;
        default: break;
        }
    }
    // The wave's goal is met (or it was cleared): an overload ends
    void onWaveOver() { overloading = false; }
    // THE WARDEN feeding on the reactor (or, with none, on itself): rings every
    // `every` seconds from `centre` while it lives; bossPulseOff() stops them
    void bossPulse(float every, glm::vec3 centre) {
        if (!bossDriven) pulseClock = every;
        bossDriven = true; pulseEvery = every; bossCentre = centre;
    }
    void bossPulseOff() { bossDriven = false; pulseEvery = PULSE_EVERY; }


    // fighting: the overload only pulses mid-wave
    void update(float dt, LevelData& L, int arena, bool fighting) {
        L.lift.update(dt, L);   // the Descent's cage (rides only when GameplayState starts one)
        pulseFired = lavaStarted = floodStarted = false;
        if (arena >= 0 && arena < (int)L.arenas.size() && L.arenas[arena].shift == ArenaShift::SOLAR && fighting) {
            flareClock = std::fmod(flareClock + dt, FLARE_CYCLE);
            flareAngle = std::fmod(flareAngle + FLARE_SPIN * dt, 6.2831853f);
        }
        for (int a = 0; a < (int)L.arenas.size(); ++a) {
            Arena& ar = L.arenas[a];
            if (ar.shift == ArenaShift::NIGHTFALL && night[a] != nightTarget[a]) {
                night[a] = approach(night[a], nightTarget[a], dt / 5.f);
                ar.theme = lerpTheme(baseTheme[a], nightfall(baseTheme[a]), night[a]);
            }
            if (ar.shift == ArenaShift::LAVA_RISE && lava[a] != lavaTarget[a]) {
                if (lava[a] == 0.f) lavaStarted = true;
                lava[a] = approach(lava[a], lavaTarget[a], dt / LAVA_TIME);
                for (int i = 0; i < (int)L.hazards.size(); ++i)
                    if (inArena(ar, baseHazard[i])) L.hazards[i].box = widened(baseHazard[i], lava[a]);
            }
            if (ar.shift == ArenaShift::OVERLOAD) {
                float target = overloading && a == arena ? 0.75f : 0.f;
                alarm = approach(alarm, target, dt / 1.5f);
                ar.theme = lerpTheme(baseTheme[a], alarmed(baseTheme[a]), alarm);
            }
        }
        // The flood
        for (int i = 0; i < (int)L.water.size(); ++i) {
            if (L.water[i].level == waterTarget[i]) continue;
            if (!rising[i]) { rising[i] = true; floodStarted = true; }
            L.water[i].level = approach(L.water[i].level, waterTarget[i], waterRate[i] * dt);
            if (L.water[i].level == waterTarget[i]) rising[i] = false;
        }
        // Overload rings
        bool live = (overloading && fighting && arena >= 0 && arena < (int)L.arenas.size() &&
                     L.arenas[arena].shift == ArenaShift::OVERLOAD && L.hasReactor) || (bossDriven && fighting);
        if (live) {
            pulseClock -= dt;
            if (pulseClock <= 0.f) {
                pulseClock += bossDriven ? pulseEvery : PULSE_EVERY;
                glm::vec3 c = bossDriven ? bossCentre : L.reactorPos;
                c.y = bossDriven ? bossCentre.y : L.arenas[arena].playerStart.y;
                rings.push_back({c, 1.5f, 1.5f});
                pulseFired = true;
            }
        }
        for (auto& r : rings) { r.prev = r.radius; r.radius += PULSE_SPEED * dt; }
        rings.erase(std::remove_if(rings.begin(), rings.end(), [](const Ring& r) { return r.radius > PULSE_RANGE; }), rings.end());
    }

    // 0..1: how close the next ring is (the reactor flares as it nears 1)
    float warning() const {
        if (!overloading && !bossDriven) return 0.f;
        return std::clamp(1.f - pulseClock / PULSE_WARN, 0.f, 1.f);
    }

    // Did a ring sweep over feet at p this update? Once per ring: the update
    // its edge crosses them. (Only near the floor: from a walkway or mid-jump
    // it passes underneath.)
    bool ringHits(glm::vec3 p) const {
        for (auto& r : rings) {
            if (p.y > r.centre.y + PULSE_HEIGHT - 0.2f || p.y < r.centre.y - 0.5f) continue;
            float d = glm::length(glm::vec2{p.x - r.centre.x, p.z - r.centre.z});
            if (d > r.prev && d <= r.radius) return true;
        }
        return false;
    }

    // The lava box at rise t (0 as built .. 1 widened)
    static AABB widened(const AABB& b, float t) {
        AABB o = b;
        float w = LAVA_WIDEN * smooth(t);
        bool alongX = (b.max.x - b.min.x) > (b.max.z - b.min.z);
        if (alongX) { o.min.z -= w; o.max.z += w; } else { o.min.x -= w; o.max.x += w; }
        o.max.y += 0.2f * smooth(t);   // and it rises a little
        return o;
    }

    static Theme nightfall(const Theme& t) {
        Theme o = t;
        o.zenith = {0.005f, 0.008f, 0.03f}; o.horizon = {0.14f, 0.05f, 0.13f}; o.ground = {0.02f, 0.01f, 0.03f};
        o.sunDir = glm::normalize(glm::vec3{0.f, -0.06f, -1.f});   // just under the horizon: an afterglow
        o.sunColor = {0.6f, 0.12f, 0.1f}; o.mountain = {0.02f, 0.01f, 0.04f}; o.stars = 1.f;
        o.lightDir = glm::normalize(glm::vec3{0.25f, -0.3f, 0.85f}); o.lightColor = {0.32f, 0.26f, 0.5f};
        o.skyAmb = {0.11f, 0.09f, 0.2f}; o.groundAmb = {0.04f, 0.03f, 0.05f};
        o.fogColor = {0.06f, 0.03f, 0.08f}; o.fogDensity = t.fogDensity * 1.3f;
        return o;
    }
    static Theme alarmed(const Theme& t) {
        Theme o = t;
        o.zenith = {0.05f, 0.f, 0.01f}; o.horizon = {0.36f, 0.05f, 0.04f}; o.ground = {0.04f, 0.f, 0.f};
        o.sunColor = {1.4f, 0.4f, 0.3f}; o.lightColor = {0.95f, 0.38f, 0.32f};
        o.skyAmb = {0.3f, 0.08f, 0.08f}; o.groundAmb = {0.08f, 0.02f, 0.02f};
        o.fogColor = {0.18f, 0.03f, 0.03f};
        return o;
    }

private:
    std::vector<Theme> baseTheme;
    std::vector<AABB>  baseHazard;
    std::vector<std::pair<float, float>> basePeriod;   // (period, phase) as built
    std::vector<float> nightTarget, lavaTarget;
    std::vector<float> baseWater, waterTarget, waterRate;   // per water volume
    std::vector<bool>  rising;

    static float smooth(float t) { return t * t * (3.f - 2.f * t); }
    static float approach(float v, float target, float step) {
        return v < target ? std::min(target, v + step) : std::max(target, v - step);
    }
    static bool inArena(const Arena& a, const AABB& b) {
        glm::vec3 c = (b.min + b.max) * 0.5f;
        return c.x >= a.zone.min.x && c.x <= a.zone.max.x && c.z >= a.zone.min.z && c.z <= a.zone.max.z;
    }
    static bool moverIn(const LevelData& L, int i, int a) {
        return inArena(L.arenas[a], L.movers[i].base);
    }
    // Change arena a's platforms to `factor` x their built speed without a
    // jump: each keeps its current point on its path (offsetAt uses t / period + phase)
    void setSpeed(LevelData& L, int a, float factor, float t) {
        if (factor == speed[a]) return;
        for (int i = 0; i < (int)L.movers.size(); ++i) {
            if (!moverIn(L, i, a)) continue;
            Mover& m = L.movers[i];
            float newPeriod = basePeriod[i].first / factor;
            m.phase += t / m.period - t / newPeriod;
            m.period = newPeriod;
        }
        speed[a] = factor;
    }
};
