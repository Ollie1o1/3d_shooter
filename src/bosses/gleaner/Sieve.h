#pragma once
// =============================================================================
// Sieve.h — THE SIEVE, the second of the Gleaner's brood (docs/chapter-4-design.md
// §5, phase 2), on the saddle over its shoulders. No OpenGL: the tests drive it.
//
// A harvestman of scrap iron: four long stilt legs, knees high over a small hub,
// and slung under the hub on chains a mesh basket full of glowing slag. Its heart
// (a crucible, white-hot) hangs in the middle of the basket, where you can see it
// through the mesh and cannot shoot it:
//
//   SIFT   it strides over you and shakes the basket: a marked circle on the
//          ground, then molten slag rains through the mesh into it for a while.
//   STAKE  it rears one leg (the tip glowing white), drives it into the plate
//          near you, and the plate cracks out from it in lines that glow, then
//          burst. Step off the lines (or jump them). The staked leg pins it:
//          its basket swings out on its chains, mouth toward you, and the
//          heart is open from any side for a moment.
//
// Its heart: x3. The mesh turns rounds aside (x0.2) unless they come steeply,
// from right under it through the grate (or from above, through the mouth), or
// off a SPLINTER (the glass finds the gaps). Enough on the heart in one opening
// spills the basket: it staggers and everything hits harder (x2). Below half
// health it is enraged: quicker, it stakes twice, the rain follows you.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

struct SieveEvent {
    enum Kind { STEP, SHAKE, RAIN_START, RAIN_END, STAKE_TELL, STAKE, CRACKS, ROAR, STAGGER, DIE } kind;
    glm::vec3 at{0.f};
};

struct Sieve {
    enum State { DORMANT, RISING, STALK, SHAKE_AIM, RAIN, STAKE_RAISE, STAKED, RECOVER, STAGGER, DEAD };
    static constexpr float MAX_HP = 3000.f;
    static constexpr float HUB_H = 17.f;                     // its hub above the ground
    static constexpr float BASKET_TOP = 13.f, BASKET_BOT = 6.f, BASKET_R = 3.6f;
    static constexpr float THIGH = 16.f, SHIN = 24.f, LEG_REACH = 13.f, HIP_R = 1.6f;   // (knees high and wide over it)
    static constexpr float RAIN_R = 9.f, RAIN_TIME = 2.6f;
    static constexpr int   CRACKS_N = 5;
    static constexpr float CRACK_LEN = 30.f, CRACK_HALF = 1.4f, CRACK_TELL = 0.75f, CRACK_BURST = 0.45f;
    static constexpr float STEEP = 0.55f;                    // how steep a round must come to thread the grate
    static constexpr float HEART_BREAK = 420.f;              // damage to the heart in one opening that spills it
    static constexpr float RADIUS = 2.4f;                    // its hub and basket: you cannot walk through them... under them you can

    State state = DORMANT;
    glm::vec3 pos{0.f};               // the ground under its hub
    float yaw = 0.f;
    float hp = MAX_HP;
    float t = 0.f, clock = 0.f, cooldown = 2.f;
    float flash = 0.f;
    float heartTaken = 0.f;
    int   stakes = 0;
    int   stakeLeg = 0;
    glm::vec3 stakeAt{0.f};           // where the staked leg goes in
    // The rain: where it falls and how long it has left
    glm::vec3 rainAt{0.f};
    float rainLeft = 0.f;
    // The cracks from the last stake: from its point outward; t since the stake
    glm::vec3 crackFrom{0.f};
    glm::vec3 crackDir[CRACKS_N]{};
    float crackT = -1.f;
    bool  burstDone = false;
    // Its feet: planted, or swinging (diagonal pairs step together)
    glm::vec3 foot[4]{}, footFrom[4]{};
    float footU[4] = {-1.f, -1.f, -1.f, -1.f};   // -1 planted; 0..1 through a step
    glm::vec2 lo{-50.f}, hi{50.f};
    float deckY = 0.f;
    uint32_t seed = 91u;
    std::vector<SieveEvent> events;

    float rnd() { seed = seed * 1664525u + 1013904223u; return (float)(seed >> 8) / 16777216.f; }
    bool alive() const { return state != DEAD && state != DORMANT; }
    bool fighting() const { return alive() && state != RISING; }
    bool enraged() const { return hp < MAX_HP * 0.5f; }
    float health01() const { return std::max(0.f, hp / MAX_HP); }
    static float ease(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
    float pace() const { return enraged() ? 1.3f : 1.f; }

    // Out of a meteor's crater on the saddle: it unfolds
    void wake(glm::vec3 at, float facing, glm::vec2 deckLo, glm::vec2 deckHi, float y) {
        *this = Sieve{};
        pos = at; pos.y = y; yaw = facing; lo = deckLo; hi = deckHi; deckY = y;
        state = RISING; t = 0.f;
        for (int i = 0; i < 4; ++i) { foot[i] = rest(i); footFrom[i] = foot[i]; }
    }
    float unfold() const { return state == RISING ? ease(t / 3.f) : state == DEAD ? 1.f - ease(t / 2.2f) * 0.7f : 1.f; }

    // Its frame
    glm::vec3 hipDir(int i) const { float a = yaw + 0.785398f + i * 1.570796f; return {std::sin(a), 0.f, std::cos(a)}; }
    glm::vec3 rest(int i) const { glm::vec3 p = pos + hipDir(i) * LEG_REACH; p.y = deckY; return p; }
    glm::vec3 hub() const { return pos + glm::vec3{0.f, glm::mix(3.f, HUB_H, unfold()) + 0.4f * std::sin(clock * 1.3f), 0.f}; }
    glm::vec3 hip(int i) const { return hub() + hipDir(i) * HIP_R; }
    // How far the basket is swung out on its chains (toward the staked leg, mouth toward you) and shaken
    float swingOut() const {
        if (state == STAKED) return ease(t / 0.5f) * (1.f - ease((t - stakedTime() + 0.4f) / 0.4f));
        if (state == STAGGER) return 0.f;
        return 0.f;
    }
    float shake() const { return state == SHAKE_AIM ? ease((t - 0.5f) / 0.6f) : state == RAIN ? 1.f : 0.f; }
    float drop() const { return state == STAGGER ? ease(t / 0.4f) * (1.f - ease((t - 2.2f) / 0.4f)) : 0.f; }   // spilled: down on the plate
    glm::vec3 swingDir() const { glm::vec3 d = stakeAt - pos; d.y = 0.f; float l = glm::length(d); return l > 0.1f ? d / l : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)}; }
    glm::vec3 heart() const {
        glm::vec3 h = hub() - glm::vec3{0.f, HUB_H - (BASKET_TOP + BASKET_BOT) * 0.5f, 0.f} * (state == RISING ? unfold() : 1.f);
        h += swingDir() * (5.f * swingOut()) + glm::vec3{0.f, 1.5f * swingOut(), 0.f};
        h += glm::vec3{std::sin(clock * 23.f), 0.f, std::cos(clock * 19.f)} * (0.35f * shake());
        h.y = glm::mix(h.y, deckY + 2.2f, drop());
        return h;
    }
    // The basket's tilt (radians, about the axis across swingDir): its mouth turned toward you
    float basketTilt() const { return 1.15f * swingOut(); }
    bool heartOpen() const { return (state == STAKED && swingOut() > 0.35f) || state == STAGGER; }
    float stakedTime() const { return enraged() ? 2.2f : 2.8f; }

    // Its body for the guns
    struct Box { glm::vec3 lo, hi; };
    Box hubBox() const { glm::vec3 h = hub(); return {h - glm::vec3{2.2f, 1.4f, 2.2f}, h + glm::vec3{2.2f, 2.2f, 2.2f}}; }
    Box basketBox() const { glm::vec3 c = heart(); float hh = (BASKET_TOP - BASKET_BOT) * 0.5f; return {c - glm::vec3{BASKET_R, hh, BASKET_R}, c + glm::vec3{BASKET_R, hh, BASKET_R}}; }
    Box heartBox() const { glm::vec3 c = heart(); return {c - glm::vec3{1.5f}, c + glm::vec3{1.5f}}; }
    static float rayBox(glm::vec3 o, glm::vec3 d, const Box& b) {
        float t0 = 0.f, t1 = 1e9f;
        for (int a = 0; a < 3; ++a) {
            if (std::abs(d[a]) < 1e-8f) { if (o[a] < b.lo[a] || o[a] > b.hi[a]) return -1.f; continue; }
            float i0 = (b.lo[a] - o[a]) / d[a], i1 = (b.hi[a] - o[a]) / d[a];
            if (i0 > i1) std::swap(i0, i1);
            t0 = std::max(t0, i0); t1 = std::min(t1, i1);
            if (t0 > t1) return -1.f;
        }
        return t0;
    }
    // Does a round on this line get through the mesh to the heart?
    bool threads(glm::vec3 d, bool splinter) const { return splinter || heartOpen() || std::abs(d.y) > STEEP; }
    // Where a round meets it first: zone 0 the mesh, 1 its hub, 2 the heart; -1 a miss
    int ray(glm::vec3 o, glm::vec3 d, float maxT, float& tOut, bool splinter = false) const {
        if (!alive()) return -1;
        int zone = -1; float best = maxT;
        float th = rayBox(o, d, hubBox()), tb = rayBox(o, d, basketBox());
        if (th >= 0.f && th < best) { best = th; zone = 1; }
        if (tb >= 0.f && tb < best) {
            float tc = threads(d, splinter) ? rayBox(o, d, heartBox()) : -1.f;
            if (tc >= 0.f) { best = tc; zone = 2; } else { best = tb; zone = 0; }
        }
        tOut = best;
        return zone;
    }
    static float zoneMult(int zone) { return zone == 2 ? 3.f : zone == 1 ? 1.f : 0.2f; }

    // Damage (already multiplied for its zone). True if this killed it
    bool hurt(float dmg, int zone) {
        if (!fighting()) return false;
        if (state == STAGGER) dmg *= 2.f;
        hp -= dmg;
        flash = 1.f;
        if (zone == 2 && state != STAGGER) {
            heartTaken += dmg;
            if (heartTaken >= HEART_BREAK) stagger();
        }
        if (hp <= 0.f) { hp = 0.f; state = DEAD; t = 0.f; rainLeft = 0.f; events.push_back({SieveEvent::DIE, heart()}); return true; }
        return false;
    }
    void stagger() {
        if (!fighting()) return;
        if (state == RAIN) events.push_back({SieveEvent::RAIN_END, rainAt});
        state = STAGGER; t = 0.f; rainLeft = 0.f; heartTaken = 0.f;
        events.push_back({SieveEvent::STAGGER, heart()});
    }

    // Hazards, for whoever stands at p (feet)
    bool inRain(glm::vec3 p) const {
        if (rainLeft <= 0.f || rainLeft > RAIN_TIME || std::abs(p.y - deckY) > 3.f) return false;
        return glm::length(glm::vec2{p.x - rainAt.x, p.z - rainAt.z}) < RAIN_R;
    }
    float crackGlow() const { return crackT < 0.f ? 0.f : crackT < CRACK_TELL ? crackT / CRACK_TELL : std::max(0.f, 1.f - (crackT - CRACK_TELL) / 1.2f); }
    float crackLen() const { return crackT < 0.f ? 0.f : CRACK_LEN * ease(crackT / (CRACK_TELL * 0.8f)); }
    bool bursting() const { return crackT >= CRACK_TELL && crackT < CRACK_TELL + CRACK_BURST; }
    bool onCrack(glm::vec3 p) const {
        if (crackT < 0.f || p.y - deckY > 1.3f || p.y < deckY - 1.f) return false;
        for (const auto& d : crackDir) {
            glm::vec3 q = p - crackFrom; q.y = 0.f;
            float along = glm::dot(q, d);
            if (along < -1.f || along > crackLen()) continue;
            if (glm::length(q - d * along) < CRACK_HALF) return true;
        }
        return false;
    }

    void stepLegs(float dt) {
        const float sp = 2.6f * pace();
        for (int i = 0; i < 4; ++i) {
            if (footU[i] >= 0.f) {
                footU[i] += dt * sp;
                if (footU[i] >= 1.f) { footU[i] = -1.f; foot[i] = rest(i); events.push_back({SieveEvent::STEP, foot[i]}); }
            }
        }
        // A pair (0+2 or 1+3) steps when either foot has fallen far behind and neither of the other pair is mid-step
        for (int pair = 0; pair < 2; ++pair) {
            int a = pair, b = pair + 2, o1 = 1 - pair, o2 = 3 - pair;
            if (footU[a] >= 0.f || footU[b] >= 0.f || footU[o1] >= 0.f || footU[o2] >= 0.f) continue;
            if (glm::length(foot[a] - rest(a)) > 3.5f || glm::length(foot[b] - rest(b)) > 3.5f) {
                for (int k : {a, b}) { footU[k] = 0.f; footFrom[k] = foot[k]; }
                break;
            }
        }
    }
    glm::vec3 footNow(int i) const {
        if (state == STAKED && i == stakeLeg) return stakeAt;
        if (state == STAKE_RAISE && i == stakeLeg) {   // reared high, pointing down at where it will go
            float u = ease(t / 0.55f);
            glm::vec3 up = glm::mix(foot[i], hip(i) + hipDir(i) * 4.f + glm::vec3{0.f, 6.f, 0.f}, u);
            float strike = ease((t - (raiseTime() - 0.12f)) / 0.12f);
            return glm::mix(up, stakeAt, strike);
        }
        if (footU[i] < 0.f) return foot[i];
        float u = footU[i];
        glm::vec3 p = glm::mix(footFrom[i], rest(i), ease(u));
        p.y += 3.f * std::sin(3.14159f * u);
        return p;
    }
    float raiseTime() const { return enraged() ? 0.75f : 0.95f; }
    // Its knee (two-bone IK), high over the hub
    glm::vec3 knee(int i) const {
        glm::vec3 s = hip(i), f = footNow(i), d = f - s;
        float len = std::clamp(glm::length(d), 1.f, (THIGH + SHIN) * 0.999f);
        glm::vec3 dir = glm::normalize(d);
        float along = (THIGH * THIGH - SHIN * SHIN + len * len) / (2.f * len);
        float h = std::sqrt(std::max(0.f, THIGH * THIGH - along * along));
        glm::vec3 pole = glm::vec3{0.f, 1.f, 0.f} + hipDir(i);
        pole -= dir * glm::dot(pole, dir);
        float pl = glm::length(pole);
        pole = pl > 1e-4f ? pole / pl : glm::vec3{0.f, 1.f, 0.f};
        return s + dir * along + pole * h;
    }

    // (events pile up from tick() and from hurt() between ticks; whoever reads them clears them)
    void tick(float dt, glm::vec3 player) {
        clock += dt; t += dt;
        flash = std::max(0.f, flash - dt * 5.f);
        if (rainLeft > 0.f) {
            float was = rainLeft;
            rainLeft -= dt;
            if (was > 0.f && rainLeft <= 0.f) events.push_back({SieveEvent::RAIN_END, rainAt});
        }
        if (crackT >= 0.f) {
            float was = crackT;
            crackT += dt;
            if (was < CRACK_TELL && crackT >= CRACK_TELL) events.push_back({SieveEvent::CRACKS, crackFrom});
            if (crackT > CRACK_TELL + 1.4f) crackT = -1.f;
        }
        if (state == DEAD || state == DORMANT) return;
        glm::vec3 to = player - pos; to.y = 0.f;
        float dist = glm::length(to);
        glm::vec3 dirTo = dist > 0.1f ? to / dist : glm::vec3{0.f, 0.f, 1.f};
        auto walk = [&](glm::vec3 goal, float speed) {
            glm::vec3 d = goal - pos; d.y = 0.f;
            float l = glm::length(d);
            if (l < 0.3f) return;
            glm::vec3 step = d / l * std::min(l, speed * dt);
            pos.x = std::clamp(pos.x + step.x, lo.x + 4.f, hi.x - 4.f);
            pos.z = std::clamp(pos.z + step.z, lo.y + 4.f, hi.y - 4.f);
        };
        yaw += 0.12f * dt;   // (it turns slowly as it walks: its legs never quite line up the same)
        switch (state) {
        case RISING:
            if (t > 3.f) { state = STALK; t = 0.f; cooldown = 1.5f; events.push_back({SieveEvent::ROAR, heart()}); }
            break;
        case STALK: {
            // It keeps a stride off you, sifting: you must come in under it
            glm::vec3 goal = player - dirTo * 13.f;
            walk(goal, 5.f * pace());
            cooldown -= dt;
            if (cooldown <= 0.f) {
                float r = rnd();
                if (dist < 26.f && r < 0.5f) { state = SHAKE_AIM; rainAt = player; rainAt.y = deckY; events.push_back({SieveEvent::SHAKE, heart()}); }
                else { state = STAKE_RAISE; stakes = 0; aimStake(player, true); }
                t = 0.f;
            }
            break;
        }
        case SHAKE_AIM:   // it strides over you, then shakes
            walk(player, 9.f * pace());
            if (t < 0.8f) { rainAt = player; rainAt.y = deckY; }
            if (t >= 1.3f) { state = RAIN; t = 0.f; rainLeft = RAIN_TIME; heartTaken = 0.f; events.push_back({SieveEvent::RAIN_START, rainAt}); }
            break;
        case RAIN:
            if (enraged()) {   // enraged, the rain follows you (slowly)
                glm::vec3 d = player - rainAt; d.y = 0.f;
                float l = glm::length(d);
                if (l > 0.1f) rainAt += d / l * std::min(l, 2.2f * dt);
                walk(rainAt, 2.2f);
            }
            if (t >= RAIN_TIME) { state = RECOVER; t = 0.f; }
            break;
        case STAKE_RAISE:
            if (t < raiseTime() - 0.3f) aimStake(player, false);   // it aims, then commits
            if (t >= raiseTime()) {
                state = STAKED; t = 0.f; heartTaken = 0.f;
                crackFrom = stakeAt; crackT = 0.f; burstDone = false;
                float base = std::atan2(player.x - stakeAt.x, player.z - stakeAt.z);   // one line runs at you
                if (glm::length(glm::vec2{player.x - stakeAt.x, player.z - stakeAt.z}) < 1.f) base = rnd() * 6.283f;
                for (int k = 0; k < CRACKS_N; ++k) { float a = base + k * 6.283185f / CRACKS_N; crackDir[k] = {std::sin(a), 0.f, std::cos(a)}; }
                events.push_back({SieveEvent::STAKE, stakeAt});
            }
            break;
        case STAKED:
            if (t >= stakedTime()) {
                foot[stakeLeg] = stakeAt;
                ++stakes;
                if (enraged() && stakes < 2) { state = STAKE_RAISE; t = 0.f; aimStake(player, true); }
                else { state = RECOVER; t = 0.f; }
            }
            break;
        case RECOVER:
            walk(player - dirTo * 13.f, 3.f);
            if (t >= 0.9f / pace()) { state = STALK; t = 0.f; cooldown = (enraged() ? 1.f : 1.8f) + rnd() * 0.9f; }
            break;
        case STAGGER:
            if (t >= 2.6f) { state = STALK; t = 0.f; cooldown = 1.2f; }
            break;
        default: break;
        }
        if (state != STAKED && state != STAKE_RAISE) stepLegs(dt);
    }
    // The leg nearest you goes up; it will come down where you are (within its reach)
    void aimStake(glm::vec3 player, bool choose) {
        if (choose) {
            float best = -2.f; int was = stakeLeg;
            glm::vec3 d = player - pos; d.y = 0.f;
            d = glm::length(d) > 0.01f ? glm::normalize(d) : glm::vec3{0.f, 0.f, 1.f};
            for (int i = 0; i < 4; ++i) { float c = glm::dot(d, hipDir(i)) - (state == STAKED && i == was ? 0.5f : 0.f); if (c > best) { best = c; stakeLeg = i; } }
            footU[stakeLeg] = -1.f;
            events.push_back({SieveEvent::STAKE_TELL, foot[stakeLeg]});
        }
        glm::vec3 d = player - pos; d.y = 0.f;
        float l = glm::length(d), reach = LEG_REACH + 9.f;
        stakeAt = l > reach ? pos + d / l * reach : player;
        stakeAt.y = deckY;
    }
};
