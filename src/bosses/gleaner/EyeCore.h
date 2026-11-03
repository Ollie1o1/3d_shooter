#pragma once
// =============================================================================
// EyeCore.h — THE EYE, the summit of the Gleaner fight (docs/chapter-4-design.md
// §5, phase 4). No OpenGL: the tests drive it.
//
// Out of its chimney and onto its back, the Gleaner tries to stand one last
// time, and its eye-core (the "moon" of the plain) tears out of its chest and
// rises on its cables to hang over the front of the roof. A red sphere of
// furnace light with a slit pupil, shuttered by six iron lids. The lids open
// only when it looks hard at something: when it fires. The danger is the opening.
//
//   SWEEP  a red line is drawn across the roof (the tell), then its beam burns
//          along it. Jump it, or be behind a chimney.
//   LANCE  it narrows on you, gold (no parry: get out of the line), and fires
//          a straight beam. Chimneys stop it.
//   WEEP   it sheds tears of slag at you: punch one back into it and it reels
//          (STUNNED: lids wide, x2).
//   CALL   it calls the sky onto the roof (three marked meteors).
//
// At two thirds and one third it FALTERS: sinks to the roof's front edge, lids
// wide open (x2.5), for a few seconds, then hauls itself back up angrier.
// At nothing it FALLS onto the roof, dim and pulsing: walk up and tear it out.
// The pupil (lids open, facing you) takes full damage; the shell turns rounds (x0.15).
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

struct EyeEvent {
    enum Kind { RISE, RISEN, SWEEP_TELL, SWEEP, LANCE_AIM, LANCE, CALL, WEEP, STUN, FALTER, RECOVER, FALL, TORN } kind;
    glm::vec3 at{0.f};
};

struct EyeCore {
    enum State { DORMANT, RISING, WATCH, SWEEP_TELL, SWEEP, LANCE_AIM, LANCE, CALL, WEEP, STUNNED, FALTER, RECOVER, FALLEN, TORN };
    static constexpr float MAX_HP = 10000.f;
    static constexpr float R = 6.5f;                   // its radius
    static constexpr float RISE_TIME = 5.5f;
    static constexpr float SWEEP_TIME = 1.5f, LANCE_TIME = 0.4f, FALTER_TIME = 6.f, STUN_TIME = 2.6f, RECOVER_TIME = 1.6f;
    static constexpr float SWEEP_HALF = 2.4f;           // how wide its sweep burns either side of the line
    static constexpr float LANCE_HALF = 1.4f;           // how close to its lance's line hurts
    static constexpr float TEAR_REACH = 4.5f;           // how close (beyond its skin) you must be to tear it out

    State state = DORMANT;
    glm::vec3 pos{0.f}, home{0.f}, from{0.f}, to{0.f};
    glm::vec2 lo{0.f}, hi{0.f};       // the roof (x, z)
    float floorY = 0.f;
    float hp = MAX_HP;
    float t = 0.f, clock = 0.f, cooldown = 2.5f;
    float open = 0.f;                 // 0 lids shut .. 1 wide
    float flash = 0.f;
    int   falters = 0;                // how many of its two falters are behind it
    int   step = 0;                   // through its attack pattern
    glm::vec3 facing{0.f, 0.f, -1.f}; // where it looks (at you)
    glm::vec3 sweepA{0.f}, sweepB{0.f};
    glm::vec3 aim{0.f};               // the lance's mark (locked late in the aim)
    int   tears = 0;
    uint32_t seed = 777u;
    std::vector<EyeEvent> events;

    float rnd() { seed = seed * 1664525u + 1013904223u; return (float)(seed >> 8) / 16777216.f; }
    static float ease(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
    bool alive() const { return state != DORMANT && state != FALLEN && state != TORN; }
    bool fighting() const { return alive() && state != RISING; }
    bool fallen() const { return state == FALLEN; }
    float health01() const { return std::max(0.f, hp / MAX_HP); }
    int stage() const { return hp > MAX_HP * 0.667f ? 0 : hp > MAX_HP * 0.333f ? 1 : 2; }
    bool enraged() const { return stage() == 2; }
    bool vulnerable() const { return state == FALTER || state == STUNNED; }

    // Out of its chest (`chest`) up to hang over the roof's front
    void wake(glm::vec3 chest, glm::vec3 hangAt, glm::vec2 roofLo, glm::vec2 roofHi, float roofY) {
        *this = EyeCore{};
        from = pos = chest; home = hangAt; lo = roofLo; hi = roofHi; floorY = roofY;
        state = RISING; t = 0.f;
        events.push_back({EyeEvent::RISE, pos});
    }
    // Where it drifts while it watches: side to side over the roof's front, bobbing
    glm::vec3 driftPoint() const {
        const float span = (hi.x - lo.x) * 0.32f;
        return home + glm::vec3{std::sin(clock * 0.27f) * span, 2.f * std::sin(clock * 0.8f), 4.f * std::cos(clock * 0.19f)};
    }
    float sweepU() const { return state == SWEEP ? ease(t / SWEEP_TIME) : 0.f; }
    glm::vec3 sweepPoint() const { return glm::mix(sweepA, sweepB, sweepU()); }
    float aimTime() const { return stage() == 2 ? 0.85f : 1.1f; }
    float tellTime() const { return stage() == 2 ? 1.f : 1.3f; }
    bool aimLocked() const { return state == LANCE_AIM && t > aimTime() - 0.3f; }
    glm::vec3 lanceDir() const { glm::vec3 d = aim - pos; float l = glm::length(d); return l > 0.01f ? d / l : facing; }

    void go(State s) { state = s; t = 0.f; }

    // Its next move: a pattern for each third of it, harder as it goes
    void pick(glm::vec3 player) {
        static const State P0[4] = {SWEEP_TELL, WEEP, SWEEP_TELL, CALL};
        static const State P1[5] = {SWEEP_TELL, LANCE_AIM, WEEP, LANCE_AIM, CALL};
        static const State P2[6] = {LANCE_AIM, SWEEP_TELL, LANCE_AIM, WEEP, CALL, SWEEP_TELL};
        const int s = stage();
        State next = s == 0 ? P0[step % 4] : s == 1 ? P1[step % 5] : P2[step % 6];
        ++step;
        go(next);
        if (next == SWEEP_TELL) {   // a line across the roof through you: along X, or (later) along Z
            const bool alongZ = s > 0 && (step % 2 == 0);
            const float y = floorY + 0.1f;
            if (!alongZ) {
                float z = std::clamp(player.z, lo.y + 3.f, hi.y - 3.f);
                bool flip = rnd() < 0.5f;
                sweepA = {flip ? hi.x - 2.f : lo.x + 2.f, y, z}; sweepB = {flip ? lo.x + 2.f : hi.x - 2.f, y, z};
            } else {
                float x = std::clamp(player.x, lo.x + 3.f, hi.x - 3.f);
                sweepA = {x, y, hi.y - 2.f}; sweepB = {x, y, lo.y + 2.f};
            }
            events.push_back({EyeEvent::SWEEP_TELL, (sweepA + sweepB) * 0.5f});
        } else if (next == LANCE_AIM) { aim = player; events.push_back({EyeEvent::LANCE_AIM, pos}); }
        else if (next == WEEP) tears = 0;
        else if (next == CALL) events.push_back({EyeEvent::CALL, player});
    }

    void tick(float dt, glm::vec3 player /* your chest */) {
        clock += dt; t += dt;
        flash = std::max(0.f, flash - dt * 4.f);
        // It watches you
        if (state != FALLEN && state != TORN && !(state == LANCE_AIM && aimLocked()) && state != LANCE) {
            glm::vec3 d = player - pos; float l = glm::length(d);
            if (l > 0.1f) facing = glm::normalize(glm::mix(facing, d / l, std::min(1.f, dt * 4.f)));
        }
        // Its lids: open when it looks hard (firing), wide when it is hurt
        float want = 0.f;
        switch (state) {
            case SWEEP_TELL: want = ease((t - tellTime() + 0.6f) / 0.5f); break;
            case SWEEP: case LANCE: case WEEP: want = 1.f; break;
            case LANCE_AIM: want = ease(t / 0.4f); break;
            case STUNNED: case FALTER: case FALLEN: want = 1.f; break;
            case RISING: want = ease((t - RISE_TIME + 1.5f) / 1.f); break;
            default: want = 0.f;
        }
        open += (want - open) * std::min(1.f, dt * (want > open ? 6.f : 3.f));
        auto hover = [&](float k) { pos += (driftPoint() - pos) * std::min(1.f, dt * k); };
        switch (state) {
        case DORMANT: case TORN: break;
        case RISING: {
            float u = ease(t / RISE_TIME);
            glm::vec3 p = glm::mix(from, home, u);
            p.y += std::sin(u * 3.14159f) * 18.f;   // (up past it, settling down onto its hang)
            pos = p;
            if (t >= RISE_TIME) { go(WATCH); cooldown = 1.5f; events.push_back({EyeEvent::RISEN, pos}); }
            break;
        }
        case WATCH:
            hover(1.2f);
            cooldown -= dt;
            if (cooldown <= 0.f) pick(player);
            break;
        case SWEEP_TELL:
            hover(0.6f);
            if (t >= tellTime()) { go(SWEEP); events.push_back({EyeEvent::SWEEP, sweepA}); }
            break;
        case SWEEP:
            if (t >= SWEEP_TIME) { go(WATCH); cooldown = stage() == 2 ? 0.9f : 1.6f; }
            break;
        case LANCE_AIM:
            hover(0.4f);
            if (!aimLocked()) aim = player;
            if (t >= aimTime()) { go(LANCE); events.push_back({EyeEvent::LANCE, pos}); }
            break;
        case LANCE:
            if (t >= LANCE_TIME) { go(WATCH); cooldown = stage() == 2 ? 0.8f : 1.4f; }
            break;
        case CALL:
            hover(1.f);
            if (t >= 1.2f) { go(WATCH); cooldown = 1.2f; }
            break;
        case WEEP: {
            hover(0.8f);
            const int want2 = stage() == 2 ? 4 : 3;
            while (tears < want2 && t >= 0.3f + tears * 0.38f) { ++tears; events.push_back({EyeEvent::WEEP, pos + facing * (R + 0.5f)}); }
            if (t >= 0.5f + want2 * 0.38f) { go(WATCH); cooldown = 1.3f; }
            break;
        }
        case STUNNED:
            pos += glm::vec3{0.f, -2.f * dt, 0.f} * (t < 0.8f ? 1.f : 0.f);
            if (t >= STUN_TIME) { go(WATCH); cooldown = 0.8f; }
            break;
        case FALTER: {   // down to the roof's front edge, lids wide
            float u = ease(t / 1.2f);
            pos = glm::mix(from, to, u) + glm::vec3{0.f, 0.4f * std::sin(clock * 3.f), 0.f};
            if (t >= FALTER_TIME) { go(RECOVER); from = pos; events.push_back({EyeEvent::RECOVER, pos}); }
            break;
        }
        case RECOVER:
            pos = glm::mix(from, driftPoint(), ease(t / RECOVER_TIME));
            if (t >= RECOVER_TIME) { go(WATCH); cooldown = 0.6f; }
            break;
        case FALLEN: {   // down onto the roof: it rolls to a stop
            float u = ease(t / 1.6f);
            glm::vec3 p = glm::mix(from, to, u);
            p.y += std::sin(u * 3.14159f) * 4.f;
            pos = t < 1.6f ? p : to;
            break;
        }
        }
    }

    // A round on a line: 2 the pupil (its lids open, on the face toward you), 0 the shell; -1 a miss
    int ray(glm::vec3 o, glm::vec3 d, float maxT, float& tOut, bool splinter = false) const {
        if (!fighting()) return -1;
        glm::vec3 oc = o - pos;
        float b = glm::dot(oc, d), c = glm::dot(oc, oc) - R * R, h = b * b - c;
        if (h < 0.f) return -1;
        float th = -b - std::sqrt(h);
        if (th < 0.f || th >= maxT) return -1;
        tOut = th;
        glm::vec3 n = (o + d * th - pos) / R;
        const bool face = glm::dot(n, facing) > 0.55f;
        if ((splinter && open > 0.1f) || (face && open > 0.35f)) return 2;
        return 0;
    }
    static float zoneMult(int zone) { return zone == 2 ? 1.f : 0.15f; }

    bool hurt(float dmg, int zone) {
        if (!fighting()) return false;
        (void)zone;
        if (state == FALTER) dmg *= 2.5f; else if (state == STUNNED) dmg *= 2.f;
        const int before = stage();
        // (faltering, it cannot be driven through its next third: each third ends in a falter of its own)
        const float floorHp = (state == FALTER || state == RECOVER) && before < 2 ? MAX_HP * (before == 0 ? 0.667f : 0.333f) + 1.f : 0.f;
        hp = std::max(hp - dmg, floorHp);
        flash = 1.f;
        if (hp <= 0.f) {   // down onto the roof, just inside its front edge
            hp = 0.f;
            from = pos; to = {std::clamp(pos.x, lo.x + R + 2.f, hi.x - R - 2.f), floorY + R * 0.85f, hi.y - R - 5.f};
            go(FALLEN);
            events.push_back({EyeEvent::FALL, pos});
            return true;
        }
        if (stage() > before && falters < stage()) {   // a third of it gone: it falters, down to the roof's edge
            falters = stage();
            from = pos; to = {std::clamp(pos.x, lo.x + R, hi.x - R), floorY + R + 0.5f, hi.y + R * 0.6f};
            go(FALTER);
            events.push_back({EyeEvent::FALTER, pos});
        }
        return false;
    }
    // A tear of its own slag punched back into it: it reels
    void stun() {
        if (!fighting() || state == FALTER || state == RECOVER) return;
        go(STUNNED);
        events.push_back({EyeEvent::STUN, pos});
    }
    bool canTear(glm::vec3 player) const {
        glm::vec2 d{player.x - pos.x, player.z - pos.z};
        return state == FALLEN && t > 1.6f && glm::length(d) < R + TEAR_REACH;
    }
    void tear() {
        if (state != FALLEN) return;
        go(TORN);
        events.push_back({EyeEvent::TORN, pos});
    }
};
