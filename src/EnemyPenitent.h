#pragma once
// =============================================================================
// EnemyPenitent.h — THE PENITENT's mind (Enemy::thinkPenitent). Included at
// the end of Enemy.h. It only decides and emits events (EnemyEvents::pen*);
// PenitentHazards.h turns them into what hurts you.
//
//   CHAINED   (anchorsLeft > 0)  kneels, turns, can't move; takes a quarter.
//             Censer sweeps low (jump) or high (slide under), a slam ring,
//             incense pools every 12 s, and the chain lash for anyone who
//             keeps away (22 m) or perches (4 m up) for 3 s.
//   UNCHAINED (all broken)       rises (2 s), stalks you; sweeps come in
//             pairs, low then high or high then low; stomps up close; calls
//             up four Hollowed Husks every 25 s; full damage.
//   SCOURGE   (under 25 %)       lashes its own back every 6 s (a ring of
//             embers), the wound opens; everything 30 % sooner.
// =============================================================================

inline void Enemy::thinkPenitent(float dt, const EnemyWorld& w, bool resolve) {
    if (!risen && anchorsLeft == 0) {   // the last chain broke
        risen = true; riseTimer = PEN_RISE_TIME; ev.penRose = true;
        attack = AttackKind::NONE; telegraphTimer = 0.f; comboLeft = 0;
    }
    if (!scourging && health < maxHealth * 0.25f) { scourging = true; scourgeTimer = 1.f; }
    const float quick = scourging ? 0.7f : 1.f;
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);

    if (riseTimer > 0.f) { riseTimer -= dt; velocity.x = velocity.z = 0.f; animPhase += dt; return; }

    // Moving: chained it only turns; risen it walks at you, stopping to strike
    if (!risen || telegraphTimer > 0.f || d < 6.f) { velocity.x = velocity.z = 0.f; }
    else { setMove(norm2(to), stats().speed, w); animPhase += dt * 1.6f; }
    if (telegraphTimer <= 0.f) turnToward(to, dt, risen ? 1.1f : 0.8f);

    // Keeping your distance (or perching) winds up the lash
    bool away = d > PEN_LASH_FAR || w.playerFeet.y > floorY + PEN_LASH_HIGH;
    farTimer = away ? farTimer + dt : 0.f;

    if (resolve) {
        switch (attack) {
            case AttackKind::CENSER_LOW:  ev.penSweep = 0; break;
            case AttackKind::CENSER_HIGH: ev.penSweep = 1; break;
            case AttackKind::PSLAM:       ev.penSlam = true; break;
            case AttackKind::PSTOMP:      ev.penStomp = true; break;
            case AttackKind::SCOURGE:     ev.penEmbers = true; break;
            default: break;
        }
        bool swept = ev.penSweep >= 0;
        attack = AttackKind::NONE;
        if (swept && comboLeft > 0) {   // the second stroke of a pair, the other height
            --comboLeft;
            nextSweep = 1 - nextSweep;
            startAttack(nextSweep ? AttackKind::CENSER_HIGH : AttackKind::CENSER_LOW, 0.75f * quick);
            return;
        }
    }

    // Incense: three pools round where you stand
    incenseTimer -= dt / quick;
    if (incenseTimer <= 0.f) {
        incenseTimer = PEN_INCENSE_EVERY;
        ev.penIncense = 3;
        float base = frand(0.f, 6.2831853f);
        for (int k = 0; k < 3; ++k) {
            float a = base + k * 2.0943951f;
            float r = k == 0 ? 0.f : 4.5f;
            ev.penIncensePos[k] = glm::vec3{w.playerFeet.x + std::cos(a) * r, floorY, w.playerFeet.z + std::sin(a) * r};
        }
    }
    if (risen) {
        summonTimer -= dt;
        if (summonTimer <= 0.f) { summonTimer = PEN_SUMMON_EVERY; ev.penSummon = 4; }
    }
    if (telegraphTimer > 0.f) return;

    if (scourging) {
        scourgeTimer -= dt;
        if (scourgeTimer <= 0.f) { scourgeTimer = PEN_SCOURGE_EVERY * quick; startAttack(AttackKind::SCOURGE, 0.8f * quick); return; }
    }
    if (farTimer >= PEN_LASH_AFTER) {   // the lash, marked along the floor toward you
        farTimer = 0.f;
        ev.penLash = true;
        ev.penLashFrom = position + glm::vec3{0.f, 0.3f, 0.f};
        ev.penLashDir = norm2(to);
        startAttack(AttackKind::PLASH, PEN_LASH_WARN);
        return;
    }
    if (!attackReady(dt / quick)) return;
    ++attackCount;
    if (risen && d < 6.f) { startAttack(AttackKind::PSTOMP, 0.8f * quick); return; }
    if (d > sweepReach()) return;   // out of reach: chained it waits for the lash, risen it keeps walking
    if (attackCount % 4 == 0) { startAttack(AttackKind::PSLAM, 1.0f * quick); return; }
    nextSweep = rand() % 2;
    comboLeft = risen ? (scourging ? 2 : 1) : 0;
    startAttack(nextSweep ? AttackKind::CENSER_HIGH : AttackKind::CENSER_LOW, 0.9f * quick);
}
