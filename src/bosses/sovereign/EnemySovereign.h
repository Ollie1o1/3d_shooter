#pragma once
// =============================================================================
// EnemySovereign.h — THE SOVEREIGN's mind (Enemy::thinkSovereign). Included at
// the end of Enemy.h; his state is in SovereignState.h. It only decides and
// emits events; Gameplay_Sovereign.h and SovereignHazards.h turn them into
// what hurts you.
//
// A duelist. Every stroke is telegraphed (blade raised, eyes flaring) and
// every one can be dodged; what makes him hard is that they come in
// strings. Out of a dash he dashes again or goes straight into a combo;
// a combo is two sweeps and an overhead cleave whose shockwave you jump.
// Standing on a platform doesn't save you: he leaps up to you. Enraged
// (below half health) he winds up faster, chains longer and throws two
// crescents at a time.
// =============================================================================

inline void Enemy::thinkSovereign(float dt, const EnemyWorld& w, bool resolve) {
    const float rage = enraged ? 1.f : 0.f;
    const bool  last = health < maxHealth * 0.2f;     // the last stand: everything comes quicker
    const float quick = last ? 0.8f : 1.f;            // wind-up multiplier
    glm::vec3 to  = flatTo(w.playerFeet);
    float d       = glm::length(to);
    glm::vec3 dir = norm2(to);
    float dy      = w.playerFeet.y - position.y;
    if (swingTimer > 0.f) swingTimer -= dt;
    if (riposteCd > 0.f) riposteCd -= dt;
    if (blinkFlash < 5.f) blinkFlash += dt;
    // A phase change (enraged at half, the last stand at a fifth): a breath before anything new
    if (enraged && !ragePaused) { ragePaused = true; phaseHold = PHASE_PAUSE; }
    if (last && !lastPaused)    { lastPaused = true; phaseHold = PHASE_PAUSE; }
    if (phaseHold > 0.f) phaseHold -= dt;

    // Keeping your distance (or the high ground) doesn't last: the
    // longer you do it, the sooner he comes for you
    bool away = d > PURSUE_RANGE || dy > 3.f;
    farTimer = away ? farTimer + dt : std::max(0.f, farTimer - 2.f * dt);

    // ---- in the air (LEAP): gravity does the work; land with a slam ----
    if (leapTimer > 0.f) {
        leapTimer -= dt;
        turnToward(to, dt, 4.f);
        bool landed = grounded && velocity.y <= 0.f && leapTimer < 1.2f;
        if (landed || leapTimer <= 0.f) {
            leapTimer = 0.f;
            velocity.x = velocity.z = 0.f;
            ev.slam = true; ev.slamRadius = 8.f; ev.slamDamage = 24.f;
            ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
            recover(0.65f - 0.25f * rage);
        }
        return;
    }

    // ---- dashing: a straight line at 34 m/s, cutting at the end --------
    if (dashTimer > 0.f) {
        // How far the last tick actually carried him (walls stop a dash)
        float made = dashAge > 0.f ? glm::length(glm::vec2(position.x - dashFrom.x, position.z - dashFrom.z)) / dt : DASH_SPEED;
        dashFrom = position;
        dashTimer -= dt; dashAge += dt;
        velocity.x = diveDir.x * DASH_SPEED;
        velocity.z = diveDir.z * DASH_SPEED;
        turnToward(diveDir, dt, 20.f);
        if (!dashHit && d < 2.6f && std::fabs(dy) < 2.6f) {
            dashHit = true; ev.meleeHit = true; ev.meleeDamage = 22.f;
            // Caught you: pull up just past instead of carrying on through you
            dashTimer = std::min(dashTimer, 0.06f);
        }
        // Stopped by a wall (barely moved last tick) or out of distance
        bool stalled = dashAge > 0.1f && made < 6.f;
        if (dashTimer <= 0.f || stalled) {
            dashTimer = 0.f;
            velocity.x = velocity.z = 0.f;
            ev.slash = 3; lastSwing = AttackKind::DASH; swingTimer = SWING_TIME;
            if (!dashHit && inArc(w, 4.4f, 1.2f)) { ev.meleeHit = true; ev.meleeDamage = 18.f; }
            attack = AttackKind::NONE;
            if (chainLeft > 0) {                       // and again
                --chainLeft;
                // Enraged, the last link of a chain can be a shadow step behind you
                if (enraged && chainLeft == 0 && attackCount % 2 == 0) startAttack(AttackKind::BLINK, 0.3f * quick, true);
                else startAttack(AttackKind::DASH, (0.34f - 0.08f * rage) * quick, true);
            } else if (d < SWEEP_REACH + 0.8f) {       // straight into a combo
                comboStep = 0;
                startAttack(AttackKind::SWEEP, (0.36f - 0.06f * rage) * quick, true);
            } else {
                recover(0.55f - 0.2f * rage);    // the opening: punish it
            }
        }
        return;
    }

    // ---- thrusting: a short, straight lunge, sword first ---------------
    if (thrustTimer > 0.f) {
        thrustTimer -= dt;
        velocity.x = diveDir.x * THRUST_SPEED;
        velocity.z = diveDir.z * THRUST_SPEED;
        if (!thrustHit && d < 2.9f && std::fabs(dy) < 2.4f) {
            thrustHit = true; ev.meleeHit = true; ev.meleeDamage = 24.f;
            thrustTimer = std::min(thrustTimer, 0.04f);
        }
        if (thrustTimer <= 0.f) {
            thrustTimer = 0.f;
            velocity.x = velocity.z = 0.f;
            ev.slash = 3; lastSwing = AttackKind::THRUST; swingTimer = SWING_TIME;
            recover(0.6f - 0.2f * rage);
        }
        return;
    }

    // ---- the whirlwind: spinning in on you, blade out ------------------
    // Back off, or jump it: the blade passes at waist height
    if (whirlTimer > 0.f) {
        whirlTimer -= dt; whirlHitCd -= dt;
        yaw += dt * 13.f;
        setMove(to, d > 1.5f ? 6.5f + 2.f * rage : 0.f, w);
        if (whirlHitCd <= 0.f) {
            whirlHitCd = 0.3f;
            ev.slash = whirlStep % 2; lastSwing = AttackKind::SWEEP; lastSwingStep = whirlStep; swingTimer = SWING_TIME;
            ++whirlStep;
            if (d < 4.4f && dy > -1.f && dy < 1.2f) { ev.meleeHit = true; ev.meleeDamage = 12.f; }
        }
        if (whirlTimer <= 0.f) {
            whirlTimer = 0.f;
            velocity.x = velocity.z = 0.f;
            recover(0.9f - 0.3f * rage);        // dizzy: the opening
        }
        return;
    }

    // ---- on foot ----
    if (telegraphTimer > 0.f) {
        // Planted for most wind-ups; creeps in behind a sweep or cleave
        bool creep = attack == AttackKind::SWEEP || attack == AttackKind::CLEAVE;
        float spd = creep && d > 2.5f ? 2.2f : 0.f;
        velocity.x = dir.x * spd; velocity.z = dir.z * spd;
        turnToward(to, dt, attack == AttackKind::DASH || attack == AttackKind::THRUST ? 7.f : creep ? 3.5f : 2.5f);
    } else if (recoverTimer > 0.f) {
        velocity.x = velocity.z = 0.f;
        turnToward(to, dt, 1.5f);
    } else {
        glm::vec3 side{-dir.z, 0.f, dir.x};
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(1.2f, 2.4f); strafeDir = -strafeDir; }
        glm::vec3 mv = d > 7.f ? dir + side * strafeDir * 0.35f : side * strafeDir + dir * 0.25f;
        setMove(mv, stats().speed * (1.f + 0.25f * rage), w);
        turnToward(to, dt, 5.f);
        // A shot turned aside comes straight back as a crescent
        if (riposte) {
            riposte = false;
            ev.slash = 0; lastSwing = AttackKind::SWEEP; lastSwingStep = 0; swingTimer = SWING_TIME;
            fireAt(w.playerEye, 3, 0.25f, 34.f, 12.f, 1.3f);
        }
    }

    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        switch (k) {
        case AttackKind::DASH: {
            // Aim where you're going to be, and carry on a few metres past
            glm::vec3 aim = w.playerFeet + leadVel_ * 0.35f;
            glm::vec3 a = flatTo(aim);
            float len = glm::length(a);
            diveDir = len > 0.1f ? a / len : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            float dist = glm::clamp(len + 4.f, 7.f, 24.f);
            dashTimer = dist / DASH_SPEED;
            dashAge = 0.f; dashHit = false;
            ev.dashStarted = true;
            break;
        }
        case AttackKind::SWEEP: {
            ev.slash = comboStep % 2; lastSwing = AttackKind::SWEEP; lastSwingStep = comboStep;
            swingTimer = SWING_TIME;
            if (inArc(w, SWEEP_REACH, 1.35f)) { ev.meleeHit = true; ev.meleeDamage = 18.f; }
            ++comboStep;
            if (comboStep < 2) startAttack(AttackKind::SWEEP, (0.3f - 0.06f * rage) * quick, true);
            else               startAttack(AttackKind::CLEAVE, (0.48f - 0.08f * rage) * quick, true);
            break;
        }
        case AttackKind::CLEAVE: {
            ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
            if (inArc(w, CLEAVE_REACH, 0.6f)) { ev.meleeHit = true; ev.meleeDamage = 28.f; }
            ev.slam = true; ev.slamRadius = 7.f; ev.slamDamage = 16.f;   // jump the shockwave
            comboStep = 0;
            if (rage > 0.f && d > 4.f) { chainLeft = 0; startAttack(AttackKind::DASH, 0.38f * quick, true); }
            else recover(0.85f - 0.3f * rage);
            break;
        }
        case AttackKind::LEAP: {
            // A ballistic arc onto where you are (gravity 24, as in integrate)
            glm::vec3 target = w.playerFeet + leadVel_ * 0.25f;
            glm::vec3 flat = flatTo(target);
            float T  = glm::clamp(0.8f + glm::length(flat) / 40.f, 0.85f, 1.6f);
            float up = target.y - position.y;
            velocity.x = flat.x / T; velocity.z = flat.z / T;
            velocity.y = up / T + 0.5f * 24.f * T;
            leapTimer = T + 0.8f;
            grounded = false;
            ev.leapStarted = true;
            break;
        }
        case AttackKind::CRESCENT: {
            ev.slash = 0; lastSwing = AttackKind::SWEEP; lastSwingStep = 0; swingTimer = SWING_TIME;
            fireAt(w.playerEye, enraged ? 7 : 5, enraged ? 1.2f : 0.9f, 26.f, 10.f, 1.5f);
            if (crescentLeft > 0) { --crescentLeft; startAttack(AttackKind::CRESCENT, 0.32f, true); }
            else recover(0.4f);
            break;
        }
        case AttackKind::BLINK: {
            // Gone in smoke, and out again just past you - on the ground
            // or up on whatever you're standing on - already raising the
            // blade. Prefer a spot with a floor under it.
            glm::vec3 beyond = d > 0.1f ? dir : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            glm::vec3 dest{0.f}; bool found = false;
            static const float ANG[] = {0.f, 1.2f, -1.2f, 2.4f, -2.4f, 3.1416f};
            for (int pass = 0; pass < 2 && !found; ++pass)
                for (float a : ANG) {
                    glm::vec3 p = w.playerFeet + rotY(beyond, a) * 3.2f;
                    p.y = w.playerFeet.y;
                    if (!insideBounds(p, w, radius() + 0.5f) || blockedAt(p, w)) continue;
                    if (pass == 0 && !supportedAt(p, w)) continue;
                    dest = p; found = true; break;
                }
            if (!found) { recover(0.3f); break; }
            ev.blinked = true; ev.blinkFrom = position;
            position = prevPosition = dest;
            velocity = glm::vec3{0.f};
            glm::vec3 back = flatTo(w.playerFeet);
            if (glm::length(back) > 0.01f) yaw = prevYaw = std::atan2(back.x, back.z);
            blinkFlash = 0.f; farTimer = 0.f;
            comboStep = 0;
            startAttack(AttackKind::CLEAVE, (0.46f - 0.08f * rage) * quick, true);   // parry it as it falls
            break;
        }
        case AttackKind::JUDGMENT: {
            // Blades out of the sky: one where you stand (and where you're
            // heading), a ring round it, and enraged a wider ring after
            glm::vec3 c = w.playerFeet + leadVel_ * 0.5f;
            addStrike(w.playerFeet, 0, 0.95f);
            if (glm::length(leadVel_) > 1.f) addStrike(c, 0, 1.0f);
            int ring = enraged ? 8 : 6;
            for (int i = 0; i < ring; ++i) {
                float a = i * 6.2832f / ring + age;
                addStrike(c + glm::vec3{std::cos(a) * 4.4f, 0.f, std::sin(a) * 4.4f}, 0, 1.05f + 0.05f * i);
            }
            if (enraged || last)
                for (int i = 0; i < 10; ++i) {
                    float a = i * 0.6283f + age * 0.5f;
                    addStrike(c + glm::vec3{std::cos(a) * 8.5f, 0.f, std::sin(a) * 8.5f}, 0, 1.6f + 0.04f * i);
                }
            ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
            recover(0.45f);
            break;
        }
        case AttackKind::WHIRL:
            whirlTimer = 1.7f + 0.6f * rage; whirlHitCd = 0.f;
            ev.dashStarted = true;
            break;
        case AttackKind::THRUST: {
            glm::vec3 a = flatTo(w.playerFeet + leadVel_ * 0.2f);
            float len = glm::length(a);
            diveDir = len > 0.1f ? a / len : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            thrustTimer = glm::clamp((len + 1.5f) / THRUST_SPEED, 0.12f, 0.32f);
            thrustHit = false;
            ev.dashStarted = true;
            break;
        }
        case AttackKind::RUPTURE: {
            // The blade driven into the floor: eruptions run out along it
            // toward you (enraged, three lines fanned out). Sidestep or jump.
            ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
            ev.slam = true; ev.slamRadius = 3.5f; ev.slamDamage = 14.f;
            glm::vec3 fwd = d > 0.1f ? dir : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
            int lines = enraged ? 3 : 1, per = enraged ? 9 : 12;
            for (int l = 0; l < lines; ++l) {
                glm::vec3 ld = rotY(fwd, (l - (lines - 1) * 0.5f) * 0.42f);
                for (int i = 1; i <= per; ++i) {
                    glm::vec3 p = position + ld * (1.2f + 2.2f * i);
                    if (!insideBounds(p, w, 0.5f)) break;
                    addStrike(p, 1, 0.12f + 0.065f * i);
                }
            }
            recover(0.7f - 0.2f * rage);
            break;
        }
        case AttackKind::PHANTOMS: {
            // Two of him, from either side of you, dashing in at once;
            // he follows them in himself
            for (int s = 0; s < 2; ++s) {
                glm::vec3 side = rotY(d > 0.1f ? dir : glm::vec3{0, 0, 1}, s == 0 ? 1.5708f : -1.5708f);
                glm::vec3 p = w.playerFeet - side * 14.f;
                p.x = glm::clamp(p.x, w.bounds.min.x + 2.f, w.bounds.max.x - 2.f);
                p.z = glm::clamp(p.z, w.bounds.min.z + 2.f, w.bounds.max.z - 2.f);
                glm::vec3 pd = w.playerFeet - p; pd.y = 0.f;
                float l = glm::length(pd);
                ev.phantomPos[s] = p;
                ev.phantomDir[s] = l > 0.1f ? pd / l : side;
            }
            ev.phantoms = 2;
            chainLeft = 0;
            startAttack(AttackKind::DASH, 0.5f, true);
            break;
        }
        default: break;
        }
    }

    if (recoverTimer <= 0.f && phaseHold <= 0.f && attackReady(dt)) {
        ++attackCount;
        float pursueAfter = last ? 1.2f : enraged ? 1.8f : 2.6f;
        if (farTimer > pursueAfter) {
            // Pursuit: a shadow step to you, blades on you, or (if you're
            // up high) a leap from wherever he is
            farTimer = 0.f;
            int pick = pursuitCount++ % 3;
            if (pick == 1)                 startAttack(AttackKind::JUDGMENT, 0.65f * quick);
            else if (pick == 2 && dy > 2.f) startAttack(AttackKind::LEAP, (0.55f - 0.1f * rage) * quick);
            else                           startAttack(AttackKind::BLINK, 0.5f * quick);
        }
        else if (dy > 3.f && d < 30.f)    startAttack(AttackKind::LEAP, (0.55f - 0.1f * rage) * quick);
        else if (d < SWEEP_REACH) {
            // The sweep-sweep-cleave combo most often; a whirlwind, a
            // thrust, a rupture under your feet (enraged, every other
            // time the phantoms instead) to keep you guessing
            int v = attackCount % (enraged ? 5 : 6);
            if (v == 1)      startAttack(AttackKind::WHIRL, (0.5f - 0.1f * rage) * quick);
            else if (v == 2) startAttack(enraged && (attackCount / 5) % 2 ? AttackKind::PHANTOMS : AttackKind::RUPTURE,
                                         (0.6f - 0.1f * rage) * quick);
            else if (v == 3) startAttack(AttackKind::THRUST, (0.45f - 0.05f * rage) * quick);
            else { comboStep = 0; startAttack(AttackKind::SWEEP, (0.42f - 0.08f * rage) * quick); }
        }
        else if (enraged && attackCount % 5 == 0 && d < 30.f) startAttack(AttackKind::PHANTOMS, 0.6f * quick);
        else if (d < 12.f && attackCount % 3 == 0)            startAttack(AttackKind::RUPTURE, (0.6f - 0.1f * rage) * quick);
        else if (d < 11.f && attackCount % 3 == 1)            startAttack(AttackKind::THRUST, (0.5f - 0.08f * rage) * quick);
        else if (d < 18.f || attackCount % 2 == 0) {
            chainLeft = rage > 0.f ? 2 : (attackCount % 3 == 0 ? 0 : 1);
            startAttack(AttackKind::DASH, (0.55f - 0.12f * rage) * quick);
        } else {
            crescentLeft = rage > 0.f ? 1 : 0;
            startAttack(AttackKind::CRESCENT, (0.6f - 0.1f * rage) * quick);
        }
    }
}
