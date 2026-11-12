#pragma once
// =============================================================================
// EnemyRing.h — Chapter 6's crossing enemies, THE DISPATCHED
// (docs/chapter-6-design.md section 7; models: tools/blender/ring_foes_build.py).
//
//   STRAY   a unit that climbed out of its capsule. It fights like a Living
//           Ward patient (claw, and a lunge you parry on white), and bounds in
//           low gravity. When the posts light it runs for the nearest lee and
//           fights you for it: the game hands it the spot (ringSeek).
//   HERDER  a pusher drone. It hangs off you and rams with its plough (gold:
//           dodge). During a countdown the game may give it a loose shelter to
//           shove out of line (ringPush): it flies round behind it and pushes.
//   TALLY   a counting mast. It does nothing to you; while one stands the
//           sweeps come twice as often (Gameplay_RingFoes.h).
//   PORTER  a hauler with a capsule on its back: the BRUTE's slam and lob
//           (thinkBrute), heavier and slower; its lobs arc over cover.
// =============================================================================

namespace ringfoe {
constexpr float HERD_HOVER = 2.1f;     // a HERDER's height over the floor
constexpr float RAM_SPEED = 17.f;
constexpr float RAM_TIME = 0.55f;
constexpr float RAM_DAMAGE = 14.f;
constexpr float PUSH_REACH = 2.6f;     // how close to its push spot a HERDER must be to shove
}

inline void Enemy::thinkStray(float dt, const EnemyWorld& w, bool resolve) {
    // Posts lit: it runs for shelter, unless you are right there to be fought
    if (ringSeekSet && telegraphTimer <= 0.f && dashTimer <= 0.f) {
        glm::vec3 to = flatTo(ringSeek);
        const float d = glm::length(to), you = glm::length(flatTo(w.playerFeet));
        if (d > 1.2f && you > 3.6f) {
            animPhase += dt * 9.f;
            turnToward(to, dt, 8.f);
            setMove(norm2(to), stats().speed * 1.25f, w);
            attackTimer = std::max(attackTimer, 0.3f);
            return;
        }
        if (d <= 1.2f && you > 3.6f) {   // in: it crouches there, facing where the wall comes from
            velocity.x = velocity.z = 0.f;
            turnToward(flatTo(w.playerFeet), dt, 3.f);
            return;
        }
    }
    thinkLab(dt, w, resolve);
}

inline void Enemy::thinkHerder(float dt, const EnemyWorld& w, bool resolve) {
    using namespace ringfoe;
    const glm::vec3 to = flatTo(w.playerFeet);
    const float distance = glm::length(to);
    animPhase += dt * 3.f;
    auto hover = [&](float h) { velocity.y = glm::clamp(floorY + h + 0.2f * std::sin(age * 1.7f + uid) - position.y, -3.f, 3.f); };
    if (dashTimer > 0.f) {   // the ram: straight down the line it marked
        const glm::vec3 from = position, next = from + labDir * (RAM_SPEED * dt);
        dashTimer = std::max(0.f, dashTimer - dt);
        if (!canStepTo(next, w)) { dashTimer = 0.f; velocity = {0.f, 0.f, 0.f}; recover(1.2f); return; }
        velocity = labDir * RAM_SPEED; velocity.y = 0.f;
        AABB player{w.playerFeet - glm::vec3{0.5f, 0.f, 0.5f}, w.playerFeet + glm::vec3{0.5f, 1.8f, 0.5f}};
        if (!dashHit && segmentHitsBox(from, next, player)) { ev.meleeHit = true; ev.meleeDamage = RAM_DAMAGE; dashHit = true; }
        if (dashTimer <= 0.f) { velocity = {0.f, 0.f, 0.f}; recover(1.f); }
        return;
    }
    if (telegraphTimer > 0.f) {   // backs off a touch, nose on you, then goes
        turnToward(to, dt, 5.f);
        velocity = -norm2(to) * 1.2f; hover(HERD_HOVER);
        return;
    }
    if (resolve && attack == AttackKind::RING_RAM) {
        attack = AttackKind::NONE;
        labDir = norm2(labAim - position); dashTimer = RAM_TIME; dashHit = false; ev.dashStarted = true;
        return;
    }
    if (ringPush >= 0) {   // a loose shelter to shove: round behind it, then lean on it
        const glm::vec3 go = ringPushAt - position;
        const glm::vec3 flat{go.x, 0.f, go.z};
        if (glm::length(flat) > PUSH_REACH * 0.5f) {
            turnToward(flat, dt, 4.f);
            velocity = norm2(flat) * stats().speed * 1.15f;
        } else velocity = {0.f, 0.f, 0.f};
        hover(1.2f);   // low, down at the chunk
        attackTimer = std::max(attackTimer, 0.5f);
        return;
    }
    turnToward(to, dt, 3.f);
    rangedMove(dt, w, 7.f, 13.f, stats().speed);
    hover(HERD_HOVER);
    if (recoverTimer <= 0.f && attackReady(dt)) {
        if (distance < 16.f && lineOfSight(eyePos(), w)) { labAim = w.playerFeet; startAttack(AttackKind::RING_RAM, 0.8f); }
        else blockedShot();
    }
}
