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

inline void Enemy::thinkWeaver(float dt, const EnemyWorld& w, bool resolve) {
    (void)resolve; velocity.x = velocity.z = 0.f;   // Task 4 gives it its wires
    (void)dt; (void)w;
}
