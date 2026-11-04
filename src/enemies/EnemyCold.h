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

// UNIT 24-K: you, restored to factory: revolver, Lancer, dash and punch, crisp and
// without the hesitation. Nothing the journey gave you (no grapple, no splinter, no
// recall): get above it and it can only shoot. Half down, it stops holding back.
//   BURST  the revolver: three quick rounds
//   SHOT   the Lancer: a long gold sight line, one heavy round (it does not track you
//          in its last moment: step out of the line)
//   DASH   a rush and a punch up close; white: parry it
inline void Enemy::thinkUnit24K(float dt, const EnemyWorld& w, bool resolve) {
    const glm::vec3 to = flatTo(w.playerFeet);
    const float distance = glm::length(to);
    const float hot = enraged ? 1.35f : 1.f;
    attackPace = hot;
    kSideCd -= dt;
    if (kSideT > 0.f) {   // a sidestep dash: quick, then straight back to it
        kSideT -= dt;
        velocity.x = kSideDir.x * 17.f; velocity.z = kSideDir.z * 17.f;
        turnToward(to, dt, 12.f);
        return;
    }
    if (dashTimer > 0.f) {   // the rush
        auto from = position, next = from + labDir * (20.f * dt);
        if (!canStepTo(next, w)) { dashTimer = 0.f; velocity = {0.f, 0.f, 0.f}; recover(0.8f); return; }
        dashTimer = std::max(0.f, dashTimer - dt); velocity = labDir * 20.f;
        AABB pl{w.playerFeet - glm::vec3{.5f, 0, .5f}, w.playerFeet + glm::vec3{.5f, 1.8f, .5f}};
        if (!dashHit && segmentHitsBox(from + glm::vec3{0, 1, 0}, next + glm::vec3{0, 1, 0}, pl)) { ev.meleeHit = true; ev.meleeDamage = 22.f; dashHit = true; }
        if (dashTimer <= 0.f) { velocity = {0.f, 0.f, 0.f}; recover(enraged ? 0.55f : 0.8f); }
        return;
    }
    if (burstLeft > 0) {   // the revolver's rounds
        burstTimer -= dt;
        turnToward(to, dt, 12.f);
        if (burstTimer <= 0.f) { fireAt(w.playerEye, 1, 0.012f, 46.f, 8.f, 0.7f); --burstLeft; burstTimer = 0.15f; }
        setMove(glm::vec3{-to.z, 0.f, to.x}, stats().speed * 0.35f, w);
        return;
    }
    if (telegraphTimer > 0.f) {
        velocity.x = velocity.z = 0.f;
        if (attack == AttackKind::SHOT && telegraphTimer > 0.3f) labAim = w.playerEye + leadVel_ * 0.1f;   // the sight line settles, then holds
        if (attack == AttackKind::DASH) labAim = w.playerFeet;
        turnToward(attack == AttackKind::SHOT ? flatTo(labAim) : to, dt, 9.f);
        return;
    }
    if (resolve) {
        auto k = attack; attack = AttackKind::NONE;
        if (k == AttackKind::BURST) { if (lineOfSight(eyePos(), w)) { burstLeft = enraged ? 4 : 3; burstTimer = 0.f; } else blockedShot(); return; }
        if (k == AttackKind::SHOT) {   // the Lancer: down the line it drew, not where you are now
            fireAt(labAim, 1, 0.f, 80.f, 26.f, 1.3f);
            ev.shotHeavy = true; ev.shotParry = 260.f;
            recover(enraged ? 0.35f : 0.6f);
            if (enraged) { burstLeft = 2; burstTimer = 0.25f; }   // and straight on with the revolver
            return;
        }
        if (k == AttackKind::DASH) { dashTimer = 0.32f; labDir = norm2(labAim - position); dashHit = false; ev.dashStarted = true; return; }
    }
    turnToward(to, dt, 8.f);
    // Keep a duelling distance, circling; now and then a quick sidestep (twice in a row, angry)
    if (kSideCd <= 0.f && recoverTimer <= 0.f && distance < 20.f) {
        kSideCd = enraged ? 1.2f : 2.1f;
        glm::vec3 side{-to.z, 0.f, to.x};
        float l = glm::length(side);
        kSideDir = l > 1e-3f ? side / l * ((rand() & 1) ? 1.f : -1.f) : glm::vec3{1.f, 0.f, 0.f};
        if (canStepTo(position + kSideDir * 4.f, w)) { kSideT = 0.22f; ev.dodged = true; return; }
    }
    rangedMove(dt, w, 6.f, 14.f, stats().speed * (recoverTimer > 0.f ? 0.6f : 1.f));
    if (recoverTimer <= 0.f && attackReady(dt)) {
        const bool level = std::fabs(w.playerFeet.y - position.y) < 2.f;
        if (distance < 6.5f && level) startAttack(AttackKind::DASH, enraged ? 0.45f : 0.55f);
        else if (!lineOfSight(eyePos(), w)) blockedShot();
        else if (kLast == 0 && distance > 8.f) { kLast = 1; labAim = w.playerEye; startAttack(AttackKind::SHOT, enraged ? 0.75f : 0.95f); }
        else { kLast = 0; startAttack(AttackKind::BURST, enraged ? 0.35f : 0.45f); }
    }
}
