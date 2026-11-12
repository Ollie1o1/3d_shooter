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
//   CROOK   the Shepherd's first sheepdog, met in the Kept (design sections 3
//           and 6). One attack: it throws its hook at where you stand (white:
//           parry it as it arrives) and, if it catches, drags you to it, out of
//           whatever shelter you were in. Held by HOLDFAST, the line holds. A
//           third of its health gone and it breaks off toward the Shepherd: it
//           can't be killed here (the tines are where it comes back).
//   DROVER  the second sheepdog, over the Gap: it hangs beside your raft with its
//           herd and, every few seconds, sends a chunk sliding across it (gold:
//           jump it, or step off its line). A third gone and it breaks off too.
//   BELLWETHER  the third, on the Wake's summit. It paces with its flock and rings
//           its bell: a ring runs out along the ground from it (gold: jump it),
//           hurts and throws but doesn't kill, on its own rhythm beside the
//           Shepherd's count. A third gone, it breaks off.
// =============================================================================

namespace ringfoe {
constexpr float HERD_HOVER = 2.1f;     // a HERDER's height over the floor
constexpr float RAM_SPEED = 17.f;
constexpr float RAM_TIME = 0.55f;
constexpr float RAM_DAMAGE = 14.f;
constexpr float PUSH_REACH = 2.6f;     // how close to its push spot a HERDER must be to shove
constexpr float CROOK_REACH = 18.f;    // how far THE CROOK throws its hook
constexpr float HOOK_LOCK = 0.35f;     // the last this-long of the throw, the aim is fixed: step out of it
constexpr float HOOK_CATCH = 2.2f;     // where it lands: within this, it has you
constexpr float CROOK_GONE = 3.2f;     // how long it takes to leave
}

inline void Enemy::thinkStray(float dt, const EnemyWorld& w, bool resolve) {
    // One in a few doesn't run at all: it stops where it is and turns to face the flash
    if (ringFaceSet && telegraphTimer <= 0.f && dashTimer <= 0.f && glm::length(flatTo(w.playerFeet)) > 3.6f) {
        velocity.x = velocity.z = 0.f;
        turnToward(ringFace, dt, 2.f);
        attackTimer = std::max(attackTimer, 0.3f);
        return;
    }
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

inline void Enemy::thinkCrook(float dt, const EnemyWorld& w, bool resolve) {
    using namespace ringfoe;
    const glm::vec3 to = flatTo(w.playerFeet);
    const float distance = glm::length(to);
    animPhase += dt * (0.8f + 2.6f * std::min(1.f, moveSpeed / std::max(0.1f, stats().speed)));
    if (telegraphTimer > 0.f) {   // the throw: it plants and swings the pole over; the aim follows you until the last moment
        velocity.x = velocity.z = 0.f;
        turnToward(to, dt, 4.f);
        if (telegraphTimer > HOOK_LOCK) crookAim = w.playerFeet;
        return;
    }
    if (resolve && attack == AttackKind::RING_HOOK) {
        attack = AttackKind::NONE;
        const glm::vec3 d = w.playerFeet - crookAim;
        if (glm::length(glm::vec2{d.x, d.z}) < HOOK_CATCH && std::fabs(d.y) < 2.5f) ev.ringHooked = true;
        recover(0.9f);
        return;
    }
    if (crookDrag >= 0) {   // reeling a stray in (the game pulls it): it stands and hauls
        velocity.x = velocity.z = 0.f;
        attackTimer = std::max(attackTimer, 0.6f);
        return;
    }
    turnToward(to, dt, 3.f);
    rangedMove(dt, w, 8.f, 14.f, stats().speed);
    if (recoverTimer <= 0.f && attackReady(dt)) {
        if (distance < CROOK_REACH && lineOfSight(eyePos(), w)) { crookAim = w.playerFeet; startAttack(AttackKind::RING_HOOK, stats().telegraph); }
        else blockedShot();
    }
}

// Breaking off: a crouch, then long bounds north toward the Shepherd, up and over the reef. No
// walls and no arena: it is leaving them. Gone after CROOK_GONE (not killed: it comes back).
inline void Enemy::crookLeaveTick(float dt) {
    using namespace ringfoe;
    crookLeave += dt;
    yaw = 3.14159265f;   // facing -Z: the Shepherd
    velocity = glm::vec3{0.f};
    if (crookLeave > 0.5f) {
        const float t = crookLeave - 0.5f;
        position += glm::vec3{0.f, 14.f - 7.f * t, -15.f} * dt;
        animPhase += dt * 5.f;
    }
    if (crookLeave > CROOK_GONE) { alive = false; state = EnemyState::DEAD; }
}

inline void Enemy::thinkDrover(float dt, const EnemyWorld& w, bool resolve) {
    const glm::vec3 to = flatTo(w.playerFeet);
    animPhase += dt * 2.f;
    turnToward(to, dt, 2.5f);
    // It hangs where the game puts it (beside the raft), drifting there smoothly
    glm::vec3 d = droverPost - position;
    const float n = glm::length(d);
    velocity = n > 0.2f ? d / n * std::min(stats().speed * 1.6f, n * 1.5f) : glm::vec3{0.f};
    if (telegraphTimer > 0.f) { velocity *= 0.3f; return; }
    if (resolve && attack == AttackKind::RING_CHARGE) { attack = AttackKind::NONE; ev.ringCharge = true; recover(1.4f); return; }
    if (recoverTimer <= 0.f && attackReady(dt) && glm::length(to) < 60.f) startAttack(AttackKind::RING_CHARGE, stats().telegraph);
}

inline void Enemy::thinkBellwether(float dt, const EnemyWorld& w, bool resolve) {
    const glm::vec3 to = flatTo(w.playerFeet);
    animPhase += dt * (0.8f + 2.f * std::min(1.f, moveSpeed / std::max(0.1f, stats().speed)));
    if (telegraphTimer > 0.f) { velocity.x = velocity.z = 0.f; turnToward(to, dt, 3.f); return; }
    if (resolve && attack == AttackKind::RING_BELL) { attack = AttackKind::NONE; ev.ringBell = true; recover(0.6f); return; }
    turnToward(to, dt, 2.f);
    rangedMove(dt, w, 9.f, 16.f, stats().speed);
    if (recoverTimer <= 0.f && attackReady(dt) && glm::length(to) < 40.f) startAttack(AttackKind::RING_BELL, stats().telegraph);
}
