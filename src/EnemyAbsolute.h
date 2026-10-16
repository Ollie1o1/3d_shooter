#pragma once
// =============================================================================
// EnemyAbsolute.h — THE ABSOLUTE's mind (Enemy::thinkAbsolute), included at
// the end of Enemy.h. He fights in STRINGS: named combos of moves, each move
// with one tell colour (white: parry it; gold: dodge or block it; red: get out
// of where it lands). It only decides and emits events (EnemyEvents::ab*);
// AbsoluteHazards.h and Gameplay_Absolute.h turn them into what hurts you.
//
//   JUDGMENT  (100-60 %)  TOO SLOW, KNEEL, BE JUDGED; SERPENT at range.
//   ASCENSION (60-25 %)   a 360 sweep at chest height (slide), four pillars
//                         shatter; every string a hit longer and 25 % quicker;
//                         MIRAGE, SEAMS, the ZENITH ray for shard campers;
//                         delayed strikes (the window is on the hit).
//   ABSOLUTE  (< 25 %)    the blade: SWEEPS, LUNGE, CRESCENTS; a sword parry
//                         shatters it; at 8 % the LAST JUDGMENT (stand by him).
// Parry a white strike: he reels 0.8 s (x1.5) and the string goes on; twice
// in one string: broken 3 s (x2). Camp 3 s (a shard, 30 m off, one pillar):
// he breaks off and comes for you.
//
// MORNINGSTAR: his lethal blow doesn't kill him. He kneels, the eclipse
// swallows the Zenith, he rises into the black and comes down on you as
// MORNINGSTAR (1.65x, blood orange, white eyes), a fresh health bar:
//   MORNINGSTAR (100-55 %)  SOLAR STRING, COLOSSUS, SUNDER, FISSURE, GRASP,
//                           ECLIPSE (his drop), CORONA (the ring's flares).
//   RISING      (55-12 %)   every slam leaves an aftershock; 18 % quicker;
//                           the ring fires under every third string.
//   TOTALITY    (< 12 %)    the ring collapses on you (jump it); then the
//                           light goes out of him: 28 % quicker.
// On his body only white shows (the whole of him, white-hot: parry it); his
// gold and red moves are read from the floor.
// =============================================================================

#include <cstdint>
#include <iterator>

// (AbTell itself is in Enemy.h: the cue needs it)
inline AbTell abTellOf(AttackKind k) {
    switch (k) {
        case AttackKind::AJAB: case AttackKind::AHOOK: case AttackKind::ASERPENT:
        case AttackKind::ASWEEP: case AttackKind::ACRESCENT: case AttackKind::AMETEOR:
        case AttackKind::ACROSS: case AttackKind::ABACKFIST: case AttackKind::AELBOW: case AttackKind::AROUNDHOUSE:
        case AttackKind::MJAB: case AttackKind::MBACKHAND: return AbTell::WHITE;
        case AttackKind::MGRASP: case AttackKind::MDROP: return AbTell::GOLD;
        case AttackKind::MSUNDER: case AttackKind::MFISSURE: case AttackKind::MFLARES: case AttackKind::MCOLLAPSE: return AbTell::RED;
        case AttackKind::AUPPER: case AttackKind::ADROP: case AttackKind::ALUNGE: case AttackKind::ASKYFALL:
        case AttackKind::AHAYMAKER: case AttackKind::ACHARGE: case AttackKind::AAXE: return AbTell::GOLD;
        case AttackKind::ARAY: case AttackKind::ASTOMP: case AttackKind::AHALO: case AttackKind::ASEAMS:
        case AttackKind::AZENITH: case AttackKind::AJUDGE: case AttackKind::ACLAP: case AttackKind::ATHUNDER: return AbTell::RED;
        default: return AbTell::NONE;
    }
}

// One move of a string: its kind, its tell (s), its damage (a beam's: a
// second of it), the first phase it's in, how much later than its flash a
// delayed strike lands
struct AbStep { AttackKind k; float tell, damage; int minPhase; float late; bool air = false; };   // air: a move made aloft
struct AbString { const char* name; int line; AbStep steps[6]; int n; };
enum { AB_TOO_SLOW, AB_KNEEL, AB_JUDGED, AB_SERPENT, AB_SEAMS, AB_ZENITH, AB_SWEEPS, AB_LUNGE, AB_CRESCENTS, AB_SKYFALL, AB_METEOR, AB_BLADEDIVE,
       AB_FLURRY, AB_BREAKER, AB_CROSSUP, AB_BULLRUSH, AB_HEEL, AB_THUNDER,
       AB_M_ENTRY, AB_M_SOLAR, AB_M_COLOSSUS, AB_M_SUNDER, AB_M_FISSURE, AB_M_GRASP, AB_M_ECLIPSE, AB_M_CORONA,   // MORNINGSTAR's
       AB_M_DAYBREAK, AB_M_WEIGHT, AB_M_COLLAPSE, AB_STRINGS };

inline bool abClosesIn(AttackKind k) {   // a blow he walks you down for (the last 30 % of its tell is committed)
    using A = AttackKind;
    return k == A::AJAB || k == A::AHOOK || k == A::AUPPER || k == A::ASWEEP || k == A::ACROSS || k == A::ABACKFIST ||
           k == A::AELBOW || k == A::AROUNDHOUSE || k == A::AHAYMAKER || k == A::AAXE || k == A::ACLAP ||
           k == A::MJAB || k == A::MBACKHAND || k == A::MSUNDER;
}
inline const AbString& abString(int id) {
    using A = AttackKind;
    static const AbString S[AB_STRINGS] = {
        {"TOO SLOW", 1, {{A::ATELEPORT, 0.25f, 0.f, 1, 0.f}, {A::AJAB, 0.45f, 18.f, 1, 0.f}, {A::AJAB, 0.30f, 18.f, 1, 0.f},
                         {A::AJAB, 0.30f, 18.f, 2, 0.2f}, {A::AHOOK, 0.40f, 28.f, 1, 0.f}}, 5},
        {"KNEEL", 0, {{A::ADASH, 0.35f, 0.f, 1, 0.f}, {A::AUPPER, 0.50f, 26.f, 1, 0.f}, {A::ADROP, 0.45f, 30.f, 1, 0.f},
                      {A::ADROP, 0.40f, 30.f, 2, 0.f}}, 4},
        {"BE JUDGED", -1, {{A::AHOP, 0.20f, 0.f, 1, 0.f}, {A::ARAY, 0.60f, 25.f, 1, 0.f}, {A::ARAY, 0.50f, 25.f, 2, 0.f},
                          {A::ASTOMP, 0.50f, 20.f, 1, 0.f}}, 4},
        {"SERPENT", -1, {{A::ASERPENT, 0.50f, 20.f, 1, 0.f}}, 1},
        {"SEAMS", 3, {{A::ASEAMS, 0.90f, 25.f, 2, 0.f}}, 1},
        {"ZENITH", -1, {{A::AZENITH, 0.60f, 20.f, 2, 0.f}}, 1},
        {"SWEEPS", 2, {{A::ADASH, 0.35f, 0.f, 3, 0.f}, {A::ASWEEP, 0.40f, 22.f, 3, 0.f}, {A::ASWEEP, 0.30f, 22.f, 3, 0.2f},
                       {A::ASWEEP, 0.30f, 22.f, 3, 0.f}}, 4},
        {"LUNGE", -1, {{A::ALUNGE, 0.50f, 30.f, 3, 0.f}}, 1},
        {"CRESCENTS", -1, {{A::ACRESCENT, 0.45f, 18.f, 3, 0.f}, {A::ACRESCENT, 0.35f, 18.f, 3, 0.f}}, 2},
        {"SKYFALL", -1, {{A::ATAKEOFF, 0.45f, 0.f, 1, 0.f, true}, {A::ARAY, 0.60f, 25.f, 1, 0.f, true},
                         {A::ASKYFALL, Enemy::AB_MARK_T, 28.f, 1, 0.f, true}}, 3},
        {"METEOR", -1, {{A::ATAKEOFF, 0.45f, 0.f, 1, 0.f, true}, {A::ASERPENT, 0.50f, 20.f, 1, 0.f, true},
                        {A::AMETEOR, 0.50f, 24.f, 1, 0.f, true}}, 3},
        {"BLADE DIVE", -1, {{A::ATAKEOFF, 0.45f, 0.f, 3, 0.f, true}, {A::ACRESCENT, 0.45f, 18.f, 3, 0.f, true},
                            {A::ALUNGE, 0.50f, 30.f, 3, 0.f, true}}, 3},
        // the fighter's strings (each phase 1 string costs at most 70 taken whole)
        {"FLURRY", 1, {{A::AJAB, 0.40f, 14.f, 1, 0.f}, {A::AJAB, 0.24f, 14.f, 1, 0.f}, {A::AJAB, 0.24f, 14.f, 1, 0.f},
                       {A::AJAB, 0.24f, 14.f, 2, 0.f}, {A::ACROSS, 0.30f, 24.f, 1, 0.f}}, 5},                                   // 66
        {"BREAKER", 0, {{A::AJAB, 0.40f, 18.f, 1, 0.f}, {A::ACROSS, 0.30f, 20.f, 1, 0.f}, {A::AHAYMAKER, 0.55f, 30.f, 1, 0.f}}, 3},   // 68
        {"CROSS-UP", -1, {{A::ATELEPORT, 0.25f, 0.f, 1, 0.f}, {A::ABACKFIST, 0.40f, 20.f, 1, 0.f}, {A::AROUNDHOUSE, 0.45f, 26.f, 1, 0.f}}, 3},
        {"BULLRUSH", -1, {{A::ACHARGE, 0.50f, 24.f, 1, 0.f}, {A::AELBOW, 0.28f, 22.f, 1, 0.f}, {A::ACLAP, 0.45f, 20.f, 1, 0.f}}, 3},   // 66
        {"HEEL OF HEAVEN", -1, {{A::AHOP, 0.20f, 0.f, 1, 0.f}, {A::AAXE, 0.50f, 26.f, 1, 0.f}, {A::ASTOMP, 0.50f, 20.f, 1, 0.f}}, 3},
        // he rises (a zoop), hangs, claps: a fan of fire rolls out along the floor at you. Jump a flame, or stand in a gap
        {"THUNDERCLAP", -1, {{A::ATHUNDER, 1.15f, 24.f, 1, 0.f}}, 1},
        // ---- MORNINGSTAR (phases 4-6; a step's minPhase 5 is RISING's addition) ----
        // his entrance: down out of the black onto you, a long first tell
        {"MORNINGSTAR", -1, {{A::MDROP, Enemy::MS_ENTRY, 36.f, 4, 0.f, true}}, 1},
        {"SOLAR STRING", -1, {{A::ADASH, 0.35f, 0.f, 4, 0.f}, {A::MJAB, 0.45f, 20.f, 4, 0.f}, {A::MJAB, 0.30f, 20.f, 4, 0.f},
                              {A::MJAB, 0.28f, 20.f, 5, 0.f}, {A::MBACKHAND, 0.40f, 34.f, 4, 0.f}}, 5},
        {"COLOSSUS", -1, {{A::ADASH, 0.35f, 0.f, 4, 0.f}, {A::MBACKHAND, 0.65f, 34.f, 4, 0.f}, {A::MSUNDER, 0.55f, 28.f, 4, 0.f}}, 3},
        {"SUNDER", -1, {{A::MSUNDER, 0.75f, 28.f, 4, 0.f}, {A::MSUNDER, 0.50f, 28.f, 5, 0.f}}, 2},
        {"FISSURE", -1, {{A::MFISSURE, 0.70f, 30.f, 4, 0.f}, {A::MFISSURE, 0.50f, 30.f, 5, 0.f}}, 2},
        {"GRASP", -1, {{A::MGRASP, 0.85f, 45.f, 4, 0.f}}, 1},
        {"ECLIPSE", -1, {{A::MDROP, 1.5f, 36.f, 4, 0.f, true}}, 1},
        {"CORONA", -1, {{A::MFLARES, 0.6f, 26.f, 4, 0.f}}, 1},
        {"DAYBREAK", -1, {{A::ADASH, 0.35f, 0.f, 5, 0.f}, {A::MJAB, 0.40f, 20.f, 5, 0.f}, {A::MJAB, 0.28f, 20.f, 5, 0.f},
                          {A::MBACKHAND, 0.38f, 34.f, 5, 0.f}, {A::MFISSURE, 0.45f, 30.f, 5, 0.f}}, 5},
        {"HEAVEN'S WEIGHT", -1, {{A::MSUNDER, 0.60f, 28.f, 5, 0.f}, {A::MDROP, 1.2f, 36.f, 5, 0.f, true}}, 2},
        {"TOTALITY", -1, {{A::MCOLLAPSE, 2.2f, 40.f, 6, 0.f}}, 1},
    };
    return S[std::clamp(id, 0, AB_STRINGS - 1)];
}
// SKYFALL's circle is on the floor only while his SKYFALL winds up (shot down, a phase, his death: it's gone)
inline bool abSkyLive(const Enemy* e) {
    return e && e->alive && e->attack == AttackKind::ASKYFALL && e->telegraphTimer > 0.f;
}
// A string with a blow in it (mirages open these): a white strike or a gold one
inline bool abDamaging(int id) {
    const AbString& s = abString(id);
    for (int k = 0; k < s.n; ++k) if (Enemy::abMeleeWhite(s.steps[k].k) || abTellOf(s.steps[k].k) == AbTell::GOLD) return true;
    return false;
}
// His lines (shown as subtitles, a choir phrase under each)
enum { AB_LINE_WORTHY = 3, AB_LINE_NOT_YET = 4 };
inline const char* abLine(int i) {
    static const char* L[] = {"KNEEL.", "TOO SLOW.", "AGAIN.", "...WORTHY.", "...NOT YET."};
    return i >= 0 && i < 5 ? L[i] : "";
}

// A standing pillar within `pad` of p (XZ), or -1
inline int absPillarAt(const EnemyWorld& w, glm::vec3 p, float pad) {
    for (int i = 0; i < w.zenPillarCount; ++i) {
        const AABB& b = w.zenPillars[i];
        if (p.x > b.min.x - pad && p.x < b.max.x + pad && p.z > b.min.z - pad && p.z < b.max.z + pad) return i;
    }
    return -1;
}
// The floor under p in the Zenith: the Dais, or the floor
inline float absFloorAt(const EnemyWorld& w, glm::vec3 p) {
    float r = glm::length(glm::vec2{p.x - w.zenCentre.x, p.z - w.zenCentre.z});
    return w.zenFloor + (r < Enemy::AB_DAIS_R ? Enemy::AB_DAIS_UP : 0.f);
}
// A teleport's or a dash's landing: on the floor (r <= 42 from the centre),
// clear of every standing pillar (pushed out the way it was going in), never
// past the edge, at the height of what's under it
inline glm::vec3 absSafeXZ(const EnemyWorld& w, glm::vec3 want, glm::vec3 fallback) {
    if (!w.hasZenith) return want;
    const glm::vec3 c = w.zenCentre;
    auto clampR = [&](glm::vec3 p) {
        glm::vec2 d{p.x - c.x, p.z - c.z};
        float r = glm::length(d);
        if (r > Enemy::AB_FLOOR_R) d *= Enemy::AB_FLOOR_R / r;
        return glm::vec3{c.x + d.x, p.y, c.z + d.y};
    };
    glm::vec3 p = clampR(want);
    for (int tries = 0; tries < 8; ++tries) {
        int k = absPillarAt(w, p, 0.8f);
        if (k < 0) break;
        const AABB& b = w.zenPillars[k];
        glm::vec2 bc{(b.min.x + b.max.x) * 0.5f, (b.min.z + b.max.z) * 0.5f};
        glm::vec2 out{p.x - bc.x, p.z - bc.y};
        if (glm::length(out) < 1e-3f) out = {bc.x - c.x, bc.y - c.z};
        out = glm::normalize(out);
        p = clampR(glm::vec3{bc.x + out.x * 3.f, p.y, bc.y + out.y * 3.f});
    }
    if (absPillarAt(w, p, 0.8f) >= 0) p = clampR(fallback);
    if (absPillarAt(w, p, 0.8f) >= 0) p = c;   // (never: the Dais is clear)
    return p;
}
// ...and on whatever floor is there
inline glm::vec3 absSafeSpot(const EnemyWorld& w, glm::vec3 want, glm::vec3 fallback) {
    glm::vec3 p = absSafeXZ(w, want, fallback);
    if (w.hasZenith) p.y = absFloorAt(w, p);
    return p;
}

// Parried: a white strike in its window (the punch, or the sword's parry).
// 0 refused, 1 he reels (and the string goes on), 2 broken, 3 his blade shattered
inline int Enemy::abParried(bool bySword) {
    if (type != EnemyType::ABSOLUTE || staggered() || abParriedThis || !abMeleeWhite(attack)) return 0;
    abParriedThis = true;
    if (attack == AttackKind::AMETEOR) { abDowned(); ev.abShotDown = false; return 4; }   // knocked out of the sky (not shot down)
    if (bySword && abBlade && attack == AttackKind::ASWEEP) {   // blade meets blade: his shatters
        ev.abBladeBroke = true; abBlade = false; abBladeGone = AB_BLADE_BACK;   // (counts once he's back on his feet)
        stagger(AB_BREAK); abReeling = false; abParries = 0;
        return 3;
    }
    if (attack == AttackKind::MBACKHAND || ++abParries >= 2) { stagger(AB_BREAK); abReeling = false; abParries = 0; return 2; }   // (MORNINGSTAR's backhand, met: broken)
    int cur = abCur, next = abStep + 1;
    stagger(AB_REEL);                 // the strike is cancelled...
    abCur = cur; abStep = next;       // ...and after the reel the string goes on
    abReeling = true; abStruck = false;
    return 1;
}

inline glm::vec3 Enemy::abGround(const EnemyWorld& w, glm::vec3 p) const {
    if (w.hasZenith) p.y = absFloorAt(w, p);
    return p;
}

// His height: eased toward where it's going (fast in a dive or a fall), set over
// whatever floor is under him. The one place his Y is decided.
inline void Enemy::abFly(float dt, const EnemyWorld& w) {
    if (staggered()) { abAltWant = 0.f; abDiving = true; }
    float rate = abDiving ? AB_DIVE_V : abAltWant < abAlt ? AB_FALL_V : AB_CLIMB;
    abAlt += glm::clamp(abAltWant - abAlt, -rate * dt, rate * dt);
    if (abAlt > 0.5f) abAirT += dt;
    else { abAirT = 0.f; if (abAltWant <= 0.f) { abFlightHp = -1.f; abDiving = false; } }   // (not on the tick he leaves the floor)
    position.y = (w.hasZenith ? absFloorAt(w, position) : abCentre.y) + abAlt;
}
// Up there: round you at 14-20 m, never past the floor's edge, round a pillar (or over one)
inline void Enemy::abOrbit(float dt, const EnemyWorld& w, glm::vec3 feet) {
    glm::vec3 from = position - feet; from.y = 0.f;
    float r = glm::length(from);
    glm::vec3 dir = r > 0.1f ? from / r : -glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
    glm::vec3 side{-dir.z * abOrbitDir, 0.f, dir.x * abOrbitDir};
    glm::vec3 goal = feet + dir * glm::clamp(r, AB_ORBIT_LO + 0.5f, AB_ORBIT_HI - 0.5f) + side * 5.f;
    glm::vec3 g = goal - position; g.y = 0.f;
    float l = glm::length(g);
    if (l > 0.05f) {
        glm::vec3 p = position + g / l * std::min(l, AB_RUN * 1.4f * dt);
        if (abAlt < AB_OVER) p = absSafeXZ(w, p, position);   // (over the pillars only the edge matters)
        if (w.hasZenith) {
            glm::vec2 d{p.x - abCentre.x, p.z - abCentre.z};
            if (glm::length(d) > AB_FLOOR_R) { d *= AB_FLOOR_R / glm::length(d); p.x = abCentre.x + d.x; p.z = abCentre.z + d.y; abOrbitDir = -abOrbitDir; }
        }
        position.x = p.x; position.z = p.z;
        moveSpeed = AB_RUN * 1.4f;
    }
    turnToward(feet - position, dt, 10.f);
}
// Shot out of the sky: he spins down and lies there, open
inline void Enemy::abDowned() {
    ev.abShotDown = true;
    stagger(AB_DOWNED); abReeling = true;   // (x1.5: the reel's multiplier)
    abAltWant = 0.f; abDiving = true; abFlightHp = -1.f;
}

inline void Enemy::thinkAbsolute(float dt, const EnemyWorld& w, bool resolve) {
    thinkAbsoluteMind(dt, w, resolve);
    abFly(dt, w);
}
inline void Enemy::thinkAbsoluteMind(float dt, const EnemyWorld& w, bool resolve) {
    if (w.hasZenith) abCentre = w.zenCentre;
    else if (abCentre == glm::vec3{0.f}) abCentre = position;   // (dev: spawned somewhere else)
    const glm::vec3 feet = w.playerFeet;
    glm::vec3 to = flatTo(feet);
    float d = glm::length(to);
    moveSpeed = 0.f;

    if (abArrive > 0.f) {   // descending onto the Dais from above
        abArrive -= dt;
        if (abArrive <= 0.f) ev.abLanded = true;
        abAlt = abAltWant = AB_ARRIVE_UP * glm::clamp((abArrive - 0.3f) / (AB_ARRIVE - 0.3f), 0.f, 1.f);
        turnToward(to, dt, 6.f);
        return;
    }
    if (abSwallow > 0.f) {   // the eclipse takes him: on his knees, then up into the black
        if (abSwallowFresh) { abSwallowFresh = false; ev.msSwallow = true; }
        abSwallow -= dt;
        if (abSwallow < MS_RISE) { abAltWant = MS_UP; abDiving = false; }
        if (abSwallow <= 0.f) { abSwallow = 0.f; abBecomeMorning(w, feet); }
        return;
    }
    if (abBladeGone > 0.f) {
        abBladeGone -= dt;
        if (abBladeGone <= 0.f && abPhase == 3) { abBlade = true; ev.abBladeDrawn = true; }
    }
    abPhaseCheck(health / maxHealth);
    if (ev.abJudgeMark) return;   // the last judgment begins: whatever was landing this tick is swept aside
    // shot out of the sky (after the phases: the damage that crosses one ends the flight instead)
    if (abAirborne() && abFlightHp > 0.f && abFlightHp - health >= AB_SHOOTDOWN * maxHealth) { abDowned(); return; }
    const float quick = morning() ? (abPhase >= 6 ? 0.72f : abPhase >= 5 ? 0.82f : 1.f) : abPhase >= 2 ? AB_QUICK : 1.f;
    if (phaseHold > 0.f) { phaseHold -= dt; turnToward(to, dt, 6.f); return; }

    // ---- the anti-camp clock: a shard, far off, or the same pillar ----
    int pillar = absPillarAt(w, feet, 3.f);
    bool far = w.hasZenith && glm::length(glm::vec2{feet.x - abCentre.x, feet.z - abCentre.z}) > AB_FAR;
    bool behind = pillar >= 0 && pillar == abCampPillar;
    abCampPillar = pillar;
    abCamp = (w.onShard || far || behind) ? abCamp + dt : 0.f;

    // ---- a move landing ----
    if (resolve) { abResolve(w, feet, d); to = flatTo(feet); d = glm::length(to); }
    if (telegraphTimer > 0.f) {   // winding up: face you; a strike closes in, a dash covers ground
        const float diveFrom = attack == AttackKind::AMETEOR ? 0.5f : 0.75f;   // (a METEOR dives from further: half its tell)
        if ((attack == AttackKind::ASKYFALL || attack == AttackKind::AMETEOR || (attack == AttackKind::ALUNGE && abAlt > 0.5f))
            && telegraphProgress() > diveFrom) {   // he dives, arriving exactly as it lands
            if (attack == AttackKind::AMETEOR && telegraphTimer > AB_CUE) abAim = feet;   // a METEOR follows you until its cue, then commits: a dash on the ting escapes
            glm::vec3 aim = attack == AttackKind::AMETEOR || attack == AttackKind::ASKYFALL ? abAim : position;   // (a blade dive drops; the lunge runs on)
            float k = std::min(1.f, dt / std::max(telegraphTimer, dt));   // this tick's share of what's left
            glm::vec3 g = aim - position; g.y = 0.f;
            glm::vec3 was = position;
            position.x += g.x * k; position.z += g.z * k;
            glm::vec3 q = absSafeXZ(w, position, was);   // round a pillar, never past the edge (the strike still lands on its mark)
            position.x = q.x; position.z = q.z;
            abAlt -= abAlt * k; abAltWant = abAlt; abDiving = true;
            moveSpeed = std::max(AB_DIVE_V, glm::length(g) / std::max(telegraphTimer, dt));
            return;
        }
        if (attack == AttackKind::MDROP) {   // the Eclipse drop: up there over its shadow, which follows you, then sets; he falls onto it
            const float p = telegraphProgress();
            if (p < 0.5f) abAim = feet;   // (up a pillar too: the crater is where you are; he lands beside it)
            glm::vec3 g = abAim - position; g.y = 0.f;
            float k = p > 0.75f ? std::min(1.f, dt / std::max(telegraphTimer, dt)) : std::min(1.f, dt * 5.f);
            position.x += g.x * k; position.z += g.z * k;
            if (w.hasZenith) {   // (never out past the floor's edge)
                glm::vec2 o{position.x - abCentre.x, position.z - abCentre.z};
                if (glm::length(o) > AB_FLOOR_R) { o *= AB_FLOOR_R / glm::length(o); position.x = abCentre.x + o.x; position.z = abCentre.z + o.y; }
            }
            if (p > 0.75f) { abAlt -= abAlt * std::min(1.f, dt / std::max(telegraphTimer, dt)); abAltWant = abAlt; abDiving = true; }
            moveSpeed = AB_DIVE_V;
            turnToward(to, dt, 8.f);
            return;
        }
        if (attack == AttackKind::MGRASP || attack == AttackKind::MFISSURE) {   // the mark follows you, then sets
            if (telegraphProgress() < (attack == AttackKind::MGRASP ? 0.55f : 0.6f)) {
                glm::vec3 dir = d > 0.1f ? to / d : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
                abAim = attack == AttackKind::MGRASP ? abGround(w, position + dir * std::min(d, MS_GRASP_REACH)) : feet;
                turnToward(to, dt, 10.f);
            }
            return;
        }
        if (attack == AttackKind::ATHUNDER) {   // risen: he hangs there facing you; the fan follows you until its cue, then commits
            if (telegraphTimer > AB_CUE) abAim = feet;
            turnToward(flatTo(abAim), dt, 10.f);
            return;
        }
        if (abAirborne() && !abDiving && attack != AttackKind::ADASH) { abOrbit(dt, w, feet); return; }   // aloft: round you
        if (telegraphProgress() < 0.7f) turnToward(to, dt, 12.f);
        glm::vec3 dir = norm2(to);
        // a strike closes in, then commits for the last 30 % of its tell: a dash then gets you out
        const float reach = morning() ? MS_REACH : AB_REACH, run = morning() ? MS_RUN : AB_RUN;
        if (abClosesIn(attack) && d > reach * 0.7f && telegraphProgress() < 0.7f) {
            float step = std::min(d - reach * 0.6f, run * 1.3f * dt);
            position = absSafeSpot(w, position + dir * step, position);   // (round a pillar, not through it)
            moveSpeed = run * 1.3f;
        }
        if (attack == AttackKind::ADASH) {
            glm::vec3 g = abAim - position; g.y = 0.f;
            float l = glm::length(g);
            if (l > 0.05f) { position = abGround(w, position + g / l * std::min(l, AB_DASH * dt)); moveSpeed = AB_DASH; }
        }
        return;
    }
    if (recoverTimer > 0.f && abCamp < AB_CAMP) {   // a breath between strings: he keeps coming
        if (abAirborne() && !abDiving) { abOrbit(dt, w, feet); return; }
        turnToward(to, dt, 8.f);
        if (d > 7.f) { position = absSafeSpot(w, position + norm2(to) * AB_RUN * dt, position); moveSpeed = AB_RUN; }
        return;
    }

    // ---- the next move: the rest of this string, or a new one ----
    if (abCur >= 0 && abStep < abString(abCur).n && abString(abCur).steps[abStep].air &&
        abAirT + abString(abCur).steps[abStep].tell > AB_AIR_MAX) {   // (the move would end past the limit)
        abCur = -1; abAltWant = 0.f; abDiving = true;   // long enough up there: down, and a string on the ground
    }
    if (abCamp >= AB_CAMP && abCur >= 0 && abCur != abCampAnswer(w)) abCur = -1;   // break off: go for the camper
    if (abCur < 0) {
        abCur = abChoose(d, w); abStep = 0; abParries = 0; abStruck = false;
        if (abString(abCur).line >= 0) ev.abLine = abString(abCur).line;
    }
    const AbString& s = abString(abCur);
    while (abStep < s.n && s.steps[abStep].minPhase > abPhase) ++abStep;
    if (abStep >= s.n) { abCur = -1; abParries = 0; recover(0.8f * quick); return; }
    abBegin(abStep, abStruck, quick, w, feet);
}

// The phases: ASCENSION at 60 % (he rises over the Dais and sweeps the arena),
// ABSOLUTE at 25 % (the blade; the shards fall), the last judgment at 8 %
inline void Enemy::abPhaseCheck(float hp) {
    const glm::vec3 dais{abCentre.x, abCentre.y + AB_DAIS_UP, abCentre.z};
    if (morning()) {   // RISING at 55 %; TOTALITY at 12 %: he takes the Dais and pulls the ring of fire in
        if (abPhase == 4 && hp <= MS_PHASE5) {
            abPhase = 5; ev.abPhase = 5; ev.enraged = true;
            phaseHold = PHASE_PAUSE; attack = AttackKind::NONE; telegraphTimer = 0.f; abCur = -1; abRot = 0;
            abAltWant = 0.f; abDiving = true;
        } else if (abPhase == 5 && hp <= MS_TOTALITY) {
            abPhase = 6; ev.abPhase = 6; ev.enraged = true;
            ev.blinked = true; ev.blinkFrom = position;
            position = dais; abAlt = abAltWant = 0.f;
            phaseHold = PHASE_PAUSE; attack = AttackKind::NONE; telegraphTimer = 0.f; abCur = -1; abRot = 0;
            abForce = AB_M_COLLAPSE;
        }
        return;
    }
    if (abPhase == 1 && hp <= AB_PHASE2) {
        abPhase = 2; ev.abPhase = 2; ev.abHalo = true; ev.abLine = 2; ev.enraged = true;
        abAltWant = AB_ASCEND_UP; abDiving = false; abFlightHp = -1.f;   // he rises over the Dais while the sweep turns (no flight: it can't be shot down)
        phaseHold = PHASE_PAUSE + AB_HALO_TIME;   // he hangs over the Dais while it sweeps
        attack = AttackKind::NONE; telegraphTimer = 0.f; abCur = -1; abRot = 0;
        ev.blinked = true; ev.blinkFrom = position;
        position = dais;
    }
    else if (abPhase == 2 && hp <= AB_PHASE3) {   // (one phase a tick: a burst still sees the ascension)
        abPhase = 3; ev.abPhase = 3; ev.abBladeDrawn = true; ev.abShards = true; ev.abLine = 0; ev.enraged = true;
        abAltWant = 0.f; abDiving = true; abFlightHp = -1.f;   // he draws it standing: down at once if he was aloft
        abBlade = true; abBladeGone = 0.f;
        phaseHold = PHASE_PAUSE; attack = AttackKind::NONE; telegraphTimer = 0.f; abCur = -1; abRot = 0;
    }
    else if (abPhase == 3 && !abJudged && hp <= AB_JUDGE_AT) {   // every seam burns but a ring round him
        abJudged = true; ev.abJudgeMark = true; abAlt = abAltWant = 0.f; ev.abTell = (int)AbTell::RED;
        ev.blinked = true; ev.blinkFrom = position;
        position = dais;
        abCur = -1; recoverTimer = 0.f; phaseHold = 0.f;
        attack = AttackKind::AJUDGE; telegraphDuration = telegraphTimer = AB_JUDGE_WARN; tellFollow = false;
        ev.telegraphStarted = true;
    }
}

// Can he reach you on foot? (On the floor or the Dais: not on a shard, past the edge, or up a pillar)
inline bool absReachable(const EnemyWorld& w, glm::vec3 feet) {
    if (!w.hasZenith) return true;
    float r = glm::length(glm::vec2{feet.x - w.zenCentre.x, feet.z - w.zenCentre.z});
    return r <= Enemy::AB_FLOOR_R + 1.5f && feet.y < absFloorAt(w, feet) + 3.f && !w.onShard;
}
// What answers a camper: TOO SLOW (onto you) if he can reach you; else from
// range: the ray (phase 1), the zenith ray from the Halo (from phase 2)
inline int Enemy::abCampAnswer(const EnemyWorld& w) const {
    if (morning()) return absReachable(w, w.playerFeet) ? AB_M_SOLAR : AB_M_ECLIPSE;   // he comes for you, or down on you
    if (w.onShard) return AB_SKYFALL;                        // he flies over and breaks it under you
    if (absReachable(w, w.playerFeet)) return AB_TOO_SLOW;
    return abPhase >= 2 ? AB_ZENITH : AB_SKYFALL;            // up a pillar, past the edge: the Halo burns there, or he comes down on you
}

inline int Enemy::abChoose(float d, const EnemyWorld& w) {
    if (abForce >= 0) { int f = abForce; abForce = -1; return f; }
    if (abCamp >= AB_CAMP) { abCamp = 0.f; return abCampAnswer(w); }
    if (morning()) {
        static const int MF[] = {AB_M_FISSURE, AB_M_ECLIPSE, AB_M_CORONA};   // from range
        static const int M4[] = {AB_M_SOLAR, AB_M_SUNDER, AB_M_GRASP, AB_M_COLOSSUS, AB_M_FISSURE, AB_M_ECLIPSE,
                                 AB_M_SOLAR, AB_M_CORONA, AB_M_COLOSSUS, AB_M_GRASP, AB_M_SUNDER, AB_M_ECLIPSE};
        static const int M5[] = {AB_M_DAYBREAK, AB_M_WEIGHT, AB_M_SOLAR, AB_M_CORONA, AB_M_COLOSSUS, AB_M_FISSURE,
                                 AB_M_GRASP, AB_M_DAYBREAK, AB_M_ECLIPSE, AB_M_SUNDER, AB_M_CORONA, AB_M_WEIGHT};
        int id = d > AB_RANGE ? MF[abRot % std::size(MF)] : abPhase == 4 ? M4[abRot % std::size(M4)] : M5[abRot % std::size(M5)];
        if (abPhase >= 5 && abRot % 3 == 2 && id != AB_M_CORONA) ev.msFlaresUnder = true;   // the ring of fire joins in
        ++abRot;
        return id;
    }
    auto bladeless = [&](int id) -> int {   // his blade shattered: fists until it's back
        if (abBlade) return id;
        if (id == AB_BLADEDIVE) return AB_SKYFALL;
        return id == AB_SWEEPS || id == AB_LUNGE || id == AB_CRESCENTS ? ((abRot & 1) ? AB_KNEEL : AB_TOO_SLOW) : id;
    };
    static const int F3[] = {AB_CRESCENTS, AB_LUNGE, AB_BLADEDIVE, AB_THUNDER}, F12[] = {AB_SERPENT, AB_TOO_SLOW, AB_SKYFALL, AB_THUNDER};   // from range
    if (d > AB_RANGE) return bladeless(abPhase == 3 ? F3[abRot++ % std::size(F3)] : F12[abRot++ % std::size(F12)]);
    static const int R1[] = {AB_TOO_SLOW, AB_FLURRY, AB_SKYFALL, AB_KNEEL, AB_BREAKER, AB_METEOR,
                             AB_JUDGED, AB_CROSSUP, AB_SKYFALL, AB_HEEL, AB_BULLRUSH, AB_METEOR, AB_THUNDER};     // a flight in three; the THUNDERCLAP closes the round
    static const int R2[] = {AB_TOO_SLOW, AB_SKYFALL, AB_FLURRY, AB_METEOR, AB_SEAMS, AB_KNEEL, AB_SKYFALL,
                             AB_BREAKER, AB_METEOR, AB_JUDGED, AB_CROSSUP, AB_SKYFALL, AB_BULLRUSH, AB_METEOR, AB_THUNDER};   // about one in two
    static const int R3[] = {AB_SWEEPS, AB_BLADEDIVE, AB_FLURRY, AB_SKYFALL, AB_LUNGE, AB_METEOR,
                             AB_BREAKER, AB_BLADEDIVE, AB_SWEEPS, AB_SKYFALL, AB_CRESCENTS, AB_SEAMS, AB_THUNDER};
    if (abPhase == 1) return R1[abRot++ % std::size(R1)];
    if (abPhase == 2) return R2[abRot++ % std::size(R2)];
    return bladeless(R3[abRot++ % std::size(R3)]);
}

inline void Enemy::abBegin(int step, bool follow, float quick, const EnemyWorld& w, glm::vec3 feet) {
    const AbStep& s = abString(abCur).steps[step];
    abParriedThis = false;
    if (!s.air && abAlt > 0.5f) { abAltWant = 0.f; abDiving = true; }   // a move on the ground: down first (45 m/s; the tell covers it)
    if (!morning() && abPhase >= 2 && step == 0 && !s.air && abDamaging(abCur)) {   // MIRAGE: two copies open any blow string with him
        glm::vec3 to0 = flatTo(feet);
        glm::vec3 dd = glm::length(to0) > 0.1f ? norm2(to0) : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
        ev.abMirages = 2;
        for (int k = 0; k < 2; ++k) ev.phantomPos[k] = absSafeSpot(w, feet + rotY(-dd, k ? 1.2f : -1.2f) * 3.f, feet - dd * 3.f);
    }
    abLate = s.late;
    float tell = s.k == AttackKind::ASKYFALL ? s.tell / std::max(tune_->windup, 0.1f) : (follow ? s.tell * quick : s.tell) + s.late;   // (the circle always gets its 0.7 s)
    startAttack(s.k, tell, follow);
    glm::vec3 to = flatTo(feet);
    glm::vec3 dir = glm::length(to) > 0.1f ? norm2(to) : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
    switch (s.k) {
        case AttackKind::ATELEPORT: {   // behind you (as he sees it), or in front if behind is past the edge
            glm::vec3 behind = feet + dir * 2.5f, front = feet - dir * 2.5f;
            bool off = w.hasZenith && glm::length(glm::vec2{behind.x - abCentre.x, behind.z - abCentre.z}) > AB_FLOOR_R;
            ev.blinked = true; ev.blinkFrom = position;
            position = absSafeSpot(w, off ? front : behind, front);
            glm::vec3 back = flatTo(feet);
            if (glm::length(back) > 0.1f) yaw = std::atan2(back.x, back.z);
            break;
        }
        case AttackKind::ADASH: {
            ev.blinked = true; ev.blinkFrom = position;
            abAim = absSafeSpot(w, feet - dir * 2.2f, position);
            glm::vec3 a = position + glm::vec3{0.f, 1.f, 0.f}, b = abAim + glm::vec3{0.f, 1.f, 0.f};
            for (int i = 0; i < w.zenPillarCount; ++i) {   // a pillar in the way: he blinks past it, never through it
                AABB p = w.zenPillars[i];
                p.min -= glm::vec3{0.6f, 0.f, 0.6f}; p.max += glm::vec3{0.6f, 0.f, 0.6f};
                if (segmentHitsBox(a, b, p)) { position = abAim; break; }
            }
            break;
        }
        case AttackKind::AHOP:
            ev.blinked = true; ev.blinkFrom = position;
            position = absSafeSpot(w, position - dir * 6.f, position);
            break;
        case AttackKind::AZENITH: abAim = feet; break;
        case AttackKind::ASKYFALL:   // the circle locks onto you; he'll come down on it
            abAim = feet; ev.abMark = true; ev.abTo = feet;
            break;
        case AttackKind::AMETEOR: ev.abFlare = true; abAim = feet; break;
        case AttackKind::ACHARGE: abAim = feet; break;
        case AttackKind::ATAKEOFF:
            ev.abTakeoff = true;
            abAltWant = AB_CRUISE_LO + (AB_CRUISE_HI - AB_CRUISE_LO) * std::fmod(age * 0.6180339f, 1.f);
            abDiving = false; abFlightHp = health;
            if (abAlt <= 0.5f) abAirT = 0.f;   // (already up - the ascension: the same flight, the same clock)
            abOrbitDir = (abRot & 1) ? 1 : -1;
            break;
        case AttackKind::ARAY: case AttackKind::ALUNGE:
            abAim = feet;
            ev.abFrom = position + glm::vec3{0.f, 1.6f, 0.f}; ev.abTo = feet + glm::vec3{0.f, 1.f, 0.f};
            break;
        case AttackKind::ADROP: abAim = feet + leadVel_ * tell; break;
        case AttackKind::ATHUNDER: abAim = feet; abAltWant = AB_THUNDER_UP; abDiving = false; break;   // up (zoop)
        case AttackKind::MDROP:    // up into the black (from the floor: a takeoff), its shadow on you
            abAim = feet;
            if (abAlt < 2.f) ev.abTakeoff = true;
            abAltWant = std::max(abAlt, MS_DROP_UP); abDiving = false; abFlightHp = -1.f; abAirT = 0.f;
            break;
        case AttackKind::MGRASP: case AttackKind::MFISSURE: abAim = feet; break;
        default: break;
    }
    ev.abTell = (int)abTellOf(s.k);
}

inline void Enemy::abResolve(const EnemyWorld& w, glm::vec3 feet, float d) {
    if (attack == AttackKind::AJUDGE) { abResolveLate(w, feet, d, 0.f); ev.abImpact = true; ev.abImpactKind = attack; lastAbAttack = attack; attack = AttackKind::NONE; return; }
    if (abCur < 0) { attack = AttackKind::NONE; return; }
    const AbString& str = abString(abCur);
    const AbStep& s = str.steps[std::min(abStep, str.n - 1)];
    const float vert = feet.y - position.y;
    auto melee = [&](float reach) {
        if (d < reach && vert > -2.2f && vert < 2.6f) { ev.meleeHit = true; ev.meleeDamage = s.damage; }
    };
    switch (attack) {
        case AttackKind::AJAB: case AttackKind::AHOOK: melee(AB_REACH); break;
        case AttackKind::AUPPER: melee(AB_REACH); if (ev.meleeHit) ev.abLaunch = true; break;
        case AttackKind::ACROSS: case AttackKind::ABACKFIST: case AttackKind::AELBOW: melee(AB_REACH); break;
        case AttackKind::AROUNDHOUSE: melee(AB_KICK_REACH); break;
        case AttackKind::AHAYMAKER: melee(AB_REACH + 0.4f); if (ev.meleeHit) ev.abHeavy = true; break;
        case AttackKind::AAXE: {
            melee(AB_REACH);
            glm::vec3 t0 = flatTo(feet);
            ev.abCrack = true; ev.abTo = position + (glm::length(t0) > 0.1f ? norm2(t0) : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)}) * 1.6f;
            break;
        }
        case AttackKind::ACLAP: {   // a cone of air in front of him
            glm::vec3 f{std::sin(yaw), 0.f, std::cos(yaw)}, to2 = flatTo(feet);
            float l = glm::length(to2);
            if (l < AB_CLAP_R && (l < 0.5f || glm::dot(to2 / l, f) > AB_CLAP_COS) && feet.y - position.y < 2.5f) { ev.meleeHit = true; ev.meleeDamage = s.damage; }
            break;
        }
        case AttackKind::ACHARGE: {   // head down, 14 m straight through where you stood
            glm::vec3 from = position, dir = norm2(abAim - position);
            if (glm::length(flatTo(abAim)) < 0.1f) dir = glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            glm::vec3 end = absSafeSpot(w, position + dir * AB_CHARGE_LEN, position);
            for (int i = 0; i < w.zenPillarCount; ++i) {   // a pillar stops him short
                AABB pb = w.zenPillars[i]; pb.min -= glm::vec3{0.8f, 0.f, 0.8f}; pb.max += glm::vec3{0.8f, 0.f, 0.8f};
                float hit = rayBoxHit(from + glm::vec3{0.f, 1.f, 0.f}, dir, pb);
                if (hit >= 0.f && hit < glm::length(end - from)) end = absSafeSpot(w, from + dir * std::max(0.f, hit - 1.f), from);
            }
            ev.blinked = true; ev.blinkFrom = from;
            glm::vec3 ab = end - from;
            float t = glm::clamp(glm::dot(feet - from, ab) / std::max(glm::dot(ab, ab), 1e-4f), 0.f, 1.f);
            glm::vec3 q = from + ab * t;
            if (glm::length(glm::vec2{feet.x - q.x, feet.z - q.z}) < 1.4f) { ev.meleeHit = true; ev.meleeDamage = s.damage; }
            position = end;
            break;
        }
        case AttackKind::ADROP: {   // a leap onto where you were going
            ev.blinked = true; ev.blinkFrom = position;
            position = absSafeSpot(w, abAim, position);
            float dd = glm::length(glm::vec2{feet.x - position.x, feet.z - position.z});
            if (dd < 2.6f) { ev.meleeHit = true; ev.meleeDamage = s.damage; }
            break;
        }
        case AttackKind::ARAY:
            ev.abRay = true; ev.abFrom = position + glm::vec3{0.f, 1.6f, 0.f}; ev.abTo = abAim + glm::vec3{0.f, 1.f, 0.f};   // along its red line
            recoverTimer = 0.6f;   // he holds the beam: the next move comes as it ends
            break;
        case AttackKind::ASTOMP:   ev.abStomp = true; ev.abFrom = position; break;
        case AttackKind::ATHUNDER:   // the clap: the fan leaves the floor under him, and he drops back down
            ev.abFan = true; ev.abFrom = abGround(w, position); ev.abTo = abAim;
            abAltWant = 0.f;
            break;
        case AttackKind::ASKYFALL: {   // the crater: 3.5 m round where the circle was
            ev.abCrater = true; ev.abTo = abAim;
            float dd = glm::length(glm::vec2{feet.x - abAim.x, feet.z - abAim.z});
            if (dd < AB_SKY_R && feet.y - abAim.y < 3.f) { ev.meleeHit = true; ev.meleeDamage = s.damage; }
            if (w.hasZenith && glm::length(glm::vec2{abAim.x - abCentre.x, abAim.z - abCentre.z}) > AB_FLOOR_R + 1.f) ev.abShardBreak = true;
            ev.blinked = true; ev.blinkFrom = position;
            position = absSafeSpot(w, abAim, position);   // (off a shard: back onto the floor's edge)
            abAlt = abAltWant = 0.f;
            break;
        }
        case AttackKind::AMETEOR: {
            glm::vec3 fist = position + glm::vec3{0.f, 1.3f, 0.f}, chest = feet + glm::vec3{0.f, 1.1f, 0.f};
            if (glm::length(fist - chest) < 2.8f) { ev.meleeHit = true; ev.meleeDamage = s.damage; }
            glm::vec3 to2 = flatTo(feet);
            glm::vec3 back = glm::length(to2) > 0.1f ? norm2(to2) : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            position = absSafeSpot(w, feet - back * 1.5f, position);
            abAlt = abAltWant = 0.f;
            break;
        }
        case AttackKind::ASERPENT: ev.abSerpent = true; ev.abFrom = position + glm::vec3{0.f, 1.8f, 0.f}; break;
        // ---- MORNINGSTAR ----
        case AttackKind::MJAB: melee(MS_REACH); break;
        case AttackKind::MBACKHAND: {   // wide: anything in front of him within its sweep
            glm::vec3 f{std::sin(yaw), 0.f, std::cos(yaw)}, to2 = flatTo(feet);
            if (d < MS_BACK_REACH && (d < 0.5f || glm::dot(to2 / d, f) > -0.2f) && vert > -2.2f && vert < 3.5f) {
                ev.meleeHit = true; ev.meleeDamage = s.damage; ev.abHeavy = true;
            }
            break;
        }
        case AttackKind::MSUNDER: {   // both fists into the floor in front of him: a ring rolls out from them
            glm::vec3 f{std::sin(yaw), 0.f, std::cos(yaw)};
            ev.msSunder = true; ev.abFrom = abGround(w, position + f * (1.4f * scale)); ev.msAfter = abPhase >= 5;
            break;
        }
        case AttackKind::MFISSURE: {   // a crack of light rips along the floor at where you stood
            glm::vec3 dir = norm2(abAim - position);
            if (glm::length(dir) < 0.5f) dir = glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            ev.msFissure = true; ev.abFrom = abGround(w, position + dir * (1.4f * scale)); ev.abTo = abAim; ev.msAfter = abPhase >= 5;
            break;
        }
        case AttackKind::MGRASP: {   // the hand closes where it came down: caught, you're lifted and thrown
            ev.msGrasp = true; ev.abTo = abAim; ev.msAfter = abPhase >= 5;
            float dd = glm::length(glm::vec2{feet.x - abAim.x, feet.z - abAim.z});
            if (dd < MS_GRASP_R && feet.y - abAim.y < 2.5f) { ev.meleeHit = true; ev.meleeDamage = s.damage; ev.abLaunch = true; }
            else recover(1.4f);   // a miss leaves him bent over the floor: the opening
            break;
        }
        case AttackKind::MDROP: {   // the crater; he stands up out of it
            ev.msDrop = true; ev.abTo = abAim; ev.msAfter = abPhase >= 5 || (abStep == 0 && abCur == AB_M_ENTRY);
            float dd = glm::length(glm::vec2{feet.x - abAim.x, feet.z - abAim.z});
            if (dd < MS_DROP_R && feet.y - abAim.y < 3.f) { ev.meleeHit = true; ev.meleeDamage = s.damage; }
            position = absSafeSpot(w, abAim, position);
            abAlt = abAltWant = 0.f;
            recover(1.f);
            break;
        }
        case AttackKind::MFLARES: ev.msFlares = true; ev.abFrom = abCentre; ev.abTo = feet; ev.msAfter = abPhase >= 5; break;
        case AttackKind::MCOLLAPSE: ev.msCollapse = true; ev.abFrom = abCentre; break;
        default: abResolveLate(w, feet, d, s.damage); break;
    }
    if (abTellOf(attack) != AbTell::NONE) abStruck = true;
    ev.abImpact = true; ev.abImpactKind = attack;   // the boom, hit or miss (AbsoluteFeel.h decides what it sounds like)
    lastAbAttack = attack;
    attack = AttackKind::NONE;
    ++abStep;
}

// The later phases' moves landing
inline void Enemy::abResolveLate(const EnemyWorld& w, glm::vec3 feet, float d, float damage) {
    switch (attack) {
        case AttackKind::ASEAMS: ev.abSeams = true; ev.abSeamFirst = (int)(age * 7.f) & 1; break;
        case AttackKind::AZENITH:   // from the Halo, onto where you stood as it began
            ev.abZenith = true; ev.abTo = abAim; ev.abFrom = abAim + glm::vec3{0.f, AB_HALO_UP, 0.f};
            break;
        case AttackKind::ASWEEP:
            if (d < AB_REACH + 0.8f && feet.y - position.y > -2.2f && feet.y - position.y < 2.6f) { ev.meleeHit = true; ev.meleeDamage = damage; }
            ev.slash = abStep & 1;   // the blade's arc (Effects, as the Sovereign's)
            break;
        case AttackKind::ALUNGE: {   // through where you stood, 20 m at most
            glm::vec3 from = position, dir = norm2(abAim - position);
            glm::vec3 end = absSafeSpot(w, position + dir * std::min(20.f, glm::length(flatTo(abAim)) + 3.f), position);
            ev.blinked = true; ev.blinkFrom = from;
            glm::vec3 ab = end - from;
            float t = glm::clamp(glm::dot(feet - from, ab) / std::max(glm::dot(ab, ab), 1e-4f), 0.f, 1.f);
            glm::vec3 q = from + ab * t;
            if (glm::length(glm::vec2{feet.x - q.x, feet.z - q.z}) < 1.6f) { ev.meleeHit = true; ev.meleeDamage = damage; }
            position = end;
            ev.slash = 3;
            abAlt = abAltWant = 0.f;   // (a blade dive lands where its thrust ends)
            break;
        }
        case AttackKind::ACRESCENT: {
            ev.shotOrigin = position + glm::vec3{0.f, 1.6f, 0.f};
            glm::vec3 aim = w.playerEye + leadVel_ * 0.4f - ev.shotOrigin;
            float l = glm::length(aim);
            aim = l > 1e-3f ? aim / l : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            ev.shots = 2; ev.shotDir[0] = rotY(aim, 0.06f); ev.shotDir[1] = rotY(aim, -0.06f);
            ev.shotSpeed = 22.f; ev.shotDamage = damage; ev.shotSize = 1.6f; ev.shotParry = 200.f;
            ev.slash = abStep & 1;
            break;
        }
        case AttackKind::AJUDGE: ev.abJudge = true; stagger(AB_SPENT); abReeling = false; break;
        default: break;
    }
}

// ---- MORNINGSTAR ----
// His lethal blow (Enemy::takeDamage): he doesn't die. Everything he was doing stops; he kneels
inline void Enemy::abBeginSwallow() {
    abSwallow = MS_SWALLOW; abSwallowFresh = true;
    attack = AttackKind::NONE; telegraphTimer = 0.f; recoverTimer = 0.f; staggerTimer = 0.f; phaseHold = 0.f;
    abCur = -1; abStep = 0; abParries = 0; abReeling = false; abForce = -1;
    abBlade = false; abBladeGone = 0.f; abAltWant = 0.f; abDiving = true; abFlightHp = -1.f;
}
// ...and out of the black he comes down on you as MORNINGSTAR: bigger, a fresh health bar, phase 4
inline void Enemy::abBecomeMorning(const EnemyWorld& w, glm::vec3 feet) {
    abForm = 1; scale = MS_SCALE;
    maxHealth *= MS_HP / statsOf(EnemyType::ABSOLUTE).health;   // (the difficulty's share carried over)
    health = maxHealth;
    abPhase = 4; ev.abPhase = 4; ev.msForm = true; enraged = false;
    abJudged = true; abBlade = false; abRot = 0; abCamp = 0.f; abAirT = 0.f; abParries = 0;
    glm::vec3 over = absSafeXZ(w, feet, abCentre);
    position.x = over.x; position.z = over.z;
    abCur = AB_M_ENTRY; abStep = 0; abStruck = false;
    abBegin(0, false, 1.f, w, feet);
}
