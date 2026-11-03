#pragma once
// =============================================================================
// EnemyCold.h — Chapter 5's enemies (docs/chapter-5-design.md section 5).
//
//   THAWLING  a stored unit fallen off its hook. It crawls for its first
//             moments, then stands and comes at you like a Living Ward
//             patient, getting quicker as it warms (pace). It only lunges
//             once it is half warm. Kill it early.
//   PICKER    a claw drone off the hook rails. It circles above you, its claws
//             flash white (parry), then it drops and snatches: caught, you are
//             lifted and held (Gameplay_ColdEnemies.h) until you shoot it off
//             you or punch free.
//   CLERK     a floating clipboard drone. It never attacks; it keeps its
//             distance, and while it is up what you kill is restored
//             (Gameplay_ColdEnemies.h). Kill it first.
// =============================================================================

namespace coldfoe {
constexpr float THAW_RISE = 2.2f;      // seconds crawling before it stands
constexpr float THAW_WARM = 10.f;      // seconds more until it is at full pace
constexpr float PICK_HOVER = 4.6f;     // a PICKER's height over the floor
constexpr float PICK_REACH = 2.8f;     // how close its drop must come to take you
constexpr float PICK_HOLD = 3.f;       // seconds it holds you if nothing stops it
constexpr float CLERK_HOVER = 3.4f;
inline float thawWarm(float age) { return glm::clamp((age - THAW_RISE) / THAW_WARM, 0.f, 1.f); }
}

inline void Enemy::thinkThawling(float dt, const EnemyWorld& w, bool resolve) {
    const glm::vec3 to = flatTo(w.playerFeet);
    if (age < coldfoe::THAW_RISE) {   // still crawling off the floor it fell to: slow, no attacks
        pace = 0.35f;
        turnToward(to, dt, 3.f);
        setMove(norm2(to), stats().speed * 0.5f, w);
        attackTimer = 0.f;
        return;
    }
    const float warm = coldfoe::thawWarm(age);
    pace = 0.45f + 0.65f * warm;
    attackPace = 0.7f + 0.5f * warm;
    thinkLab(dt, w, resolve);
    // Too stiff to throw itself at you yet: a cold one only claws
    if (ev.telegraphStarted && attack == AttackKind::LAB_LUNGE && warm < 0.5f) {
        attack = AttackKind::NONE; telegraphTimer = 0.f; ev.telegraphStarted = false;
    }
}

inline void Enemy::thinkPicker(float dt, const EnemyWorld& w, bool resolve) {
    const glm::vec3 to = flatTo(w.playerFeet);
    const float distance = glm::length(to);
    animPhase += dt * 3.f;
    if (pickT > 0.f) {   // holding you: up, and still (the game hangs you under it)
        pickT -= dt;
        velocity = {0.f, glm::clamp(floorY + coldfoe::PICK_HOVER + 2.5f - position.y, -2.f, 2.f), 0.f};
        if (pickT <= 0.f) recover(1.6f);
        return;
    }
    if (dashTimer > 0.f) {   // the drop: straight at where it marked
        dashTimer = std::max(0.f, dashTimer - dt);
        glm::vec3 d = pickAim + glm::vec3{0.f, 2.2f, 0.f} - position;
        float l = glm::length(d);
        velocity = l > 0.05f ? d / l * std::min(26.f, l / std::max(dt, 1e-3f)) : glm::vec3{0.f};
        glm::vec3 me = position - glm::vec3{0.f, 2.2f, 0.f};
        if (!dashHit && glm::length(glm::vec2{me.x - w.playerFeet.x, me.z - w.playerFeet.z}) < coldfoe::PICK_REACH
            && std::fabs(me.y - w.playerFeet.y) < 2.6f) {
            dashHit = true; dashTimer = 0.f; pickT = coldfoe::PICK_HOLD;
            ev.sortGrab = true; ev.meleeDamage = 8.f;   // (the game takes you: Gameplay_ColdEnemies.h)
        }
        if (dashTimer <= 0.f && !dashHit) recover(1.4f);   // missed: it climbs back up, open
        return;
    }
    if (telegraphTimer > 0.f) {   // claws open over you, tracking until the last moment
        if (telegraphTimer > 0.3f) pickAim = w.playerFeet + leadVel_ * 0.15f;
        glm::vec3 over = pickAim + glm::vec3{0.f, coldfoe::PICK_HOVER, 0.f} - position;
        velocity = over * 2.f;
        if (glm::length(velocity) > 9.f) velocity = glm::normalize(velocity) * 9.f;
        turnToward(to, dt, 6.f);
        return;
    }
    if (resolve && attack == AttackKind::SORT_GRAB) {
        attack = AttackKind::NONE; dashTimer = 0.55f; dashHit = false; ev.dashStarted = true;
        return;
    }
    turnToward(to, dt, 4.f);
    rangedMove(dt, w, 5.f, 11.f, stats().speed * (recoverTimer > 0.f ? 0.6f : 1.f));
    velocity.y = glm::clamp(floorY + coldfoe::PICK_HOVER + 0.4f * std::sin(age * 1.3f + uid) - position.y, -3.f, 3.f);
    if (recoverTimer <= 0.f && attackReady(dt)) {
        if (distance < 11.f && lineOfSight(eyePos(), w)) { pickAim = w.playerFeet; startAttack(AttackKind::SORT_GRAB, 0.85f); }
        else blockedShot();
    }
}

inline void Enemy::thinkClerk(float dt, const EnemyWorld& w) {
    animPhase += dt * 2.f;
    turnToward(flatTo(w.playerFeet), dt, 3.f);
    rangedMove(dt, w, 12.f, 20.f, stats().speed);
    velocity.y = glm::clamp(floorY + coldfoe::CLERK_HOVER + 0.3f * std::sin(age * 2.1f + uid) - position.y, -2.f, 2.f);
    attackTimer = 0.f;
}
