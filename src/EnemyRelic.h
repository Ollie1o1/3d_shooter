#pragma once
// =============================================================================
// EnemyRelic.h — the Reliquary's two: THE REVENANT (rakes up close, soul bolts
// at range; when it dies its soul flees - RelicHazards.h) and THE WEAVER
// (keeps its distance, strings wires across your way). Minds only; events out.
// Included at the end of Enemy.h.
// =============================================================================

inline void Enemy::thinkRevenant(float dt, const EnemyWorld& w, bool resolve) {
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    if (telegraphTimer > 0.f) { velocity.x = velocity.z = 0.f; }
    else if (d > 2.4f) { setMove(norm2(to), stats().speed, w); animPhase += dt * 3.f; }
    else velocity.x = velocity.z = 0.f;
    turnToward(to, dt, 6.f);
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        if (k == AttackKind::RAKE) {
            if (d < 3.2f) { ev.meleeHit = true; ev.meleeDamage = 14.f; }
            if (comboLeft > 0) { --comboLeft; startAttack(AttackKind::RAKE, 0.25f, true); return; }   // the second hand
        }
        if (k == AttackKind::SOULBOLT) { fireAt(w.playerEye, 1, 0.f, 14.f, 14.f, 1.2f); ev.shotParry = 60.f; }
    }
    // At range a soul bolt, on its own clock (it closes fast, so it doesn't wait for one)
    if (burstTimer > 0.f) burstTimer -= dt;
    if (telegraphTimer <= 0.f && d > 8.f && burstTimer <= 0.f && lineOfSight(eyePos(), w)) {
        burstTimer = 3.f;
        startAttack(AttackKind::SOULBOLT, 0.6f);
        return;
    }
    if (!attackReady(dt)) return;
    if (d < 3.2f) { comboLeft = 1; startAttack(AttackKind::RAKE, stats().telegraph); }
}

// It hangs back (14-20 m), and every ~6 s strings a wire across where you're
// heading: 10 m long, at chest height, square to your movement
inline void Enemy::thinkWeaver(float dt, const EnemyWorld& w, bool resolve) {
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
    if (telegraphTimer > 0.f) { velocity.x = velocity.z = 0.f; }
    else {
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(2.f, 3.5f); strafeDir = -strafeDir; }
        glm::vec3 mv = d < 14.f ? -dir + side * strafeDir * 0.5f : d > 20.f ? dir : side * strafeDir;
        setMove(mv, stats().speed, w);
        animPhase += dt * 5.f;
    }
    turnToward(to, dt, 4.f);
    if (resolve && attack == AttackKind::STRING) {
        attack = AttackKind::NONE;
        glm::vec3 v{w.playerVel.x, 0.f, w.playerVel.z};
        float sp = glm::length(v);
        glm::vec3 fwd = sp > 0.5f ? v / sp : -dir;                 // across your path (or across the line to it)
        glm::vec3 across{-fwd.z, 0.f, fwd.x};
        glm::vec3 c = w.playerFeet + (sp > 0.5f ? fwd * 4.f : -dir * 4.f);   // ahead of you, or (standing still) between you and it
        c.y = w.playerFeet.y + 1.2f;
        ev.wire = true;
        ev.wireA = c - across * 5.f;
        ev.wireB = c + across * 5.f;
    }
    if (telegraphTimer > 0.f) return;
    if (!attackReady(dt)) return;
    if (lineOfSight(eyePos(), w)) startAttack(AttackKind::STRING, stats().telegraph);
}
