#pragma once
// =============================================================================
// EnemyPacemaker.h — THE PACEMAKER, the Gauntlet's last: the machine that set
// the par times, a clock face in its chest and a blade on each forearm. It
// races you up the Tower, and each phase asks for a lesson a mini-boss taught.
//
//   THE YARD   (100-65 %)  a duellist on the courtyard floor: DASH (a white
//              slash down a line: parry it as it lands; it may chain a THRUST),
//              SWEEP (both blades round it, white), VOLLEY (a fan of orbs to
//              parry). It keeps to 6-12 m and closes to cut.
//   THE CLIMB  (65-30 %)   it LEAPs onto the first balcony, then the second
//              (at 48 %), and fights from up there: CRESCENTs (fast shots)
//              down at you, SLAMs that send a ring along its balcony (jump
//              it), melee if you reach it. Lifts, pads and the hook take you up.
//   THE CROWN  (under 30 %) it leaps to the keep's roof and overclocks: faster,
//              DASHes in pairs, rings, fans. At 10 % it kneels to light the
//              beacon (8 s): put 4 % of its health into it to break the
//              channel. If the beacon lights, it heals 15 % (once more later).
//
// Every tell at least TELL_FLOOR (a follow-up TELL_FLOOR_FOLLOW), every
// recovery RECOVER_MIN; it's never untouchable. Parried (DASH, SWEEP, THRUST)
// it staggers 2.6 s and takes double. The caller integrates it (not mid-leap: it flies its arc).
// Included at the end of Enemy.h.
// =============================================================================

// Where it fights from in each phase (the Tower's balconies and its roof: LevelGauntlet.h)
inline glm::vec3 Enemy::pmPerch(int phase) const {
    const glm::vec3 c = miniHome;   // (set at its spawn: the keep's centre, on the yard floor)
    switch (phase) {
        case 1: return c + glm::vec3{0.f, 12.f, -8.f};   // the first balcony, south face
        case 2: return c + glm::vec3{0.f, 24.f, 8.f};    // the second, north face
        case 3: return c + glm::vec3{0.f, 34.f, 0.f};    // the roof, by the beacon
        default: return c;
    }
}

inline void Enemy::pmLeapTo(glm::vec3 p) {
    const float T = 1.35f, g = 24.f;
    glm::vec3 d = p - position;
    velocity = {d.x / T, (d.y + 0.5f * g * T * T) / T, d.z / T};
    pmLeapT = T + 0.25f;
    ev.leapStarted = true;
    attack = AttackKind::NONE; telegraphTimer = 0.f;
}

inline void Enemy::thinkPacemaker(float dt, const EnemyWorld& w, bool resolve) {
    if (!miniHomeSet) { miniHome = pmKeep; miniHomeSet = true; }   // (the game says where the keep is: pmKeep)
    const float hp = health / maxHealth;
    // ---- the phases: each one a leap up the Tower ----
    int want = hp > 0.65f ? 0 : hp > 0.48f ? 1 : hp > 0.30f ? 2 : 3;
    if (want > pmPhase && pmLeapT <= 0.f && telegraphTimer <= 0.f && dashTimer <= 0.f) {
        pmPhase = want;
        if (pmPhase == 3) enraged = true;   // the overclock
        ev.pmPhaseStarted = pmPhase;
        pmLeapTo(pmPerch(pmPhase));
        phaseHold = PHASE_PAUSE;
        return;
    }
    if (pmLeapT > 0.f) {   // in the air: ballistic, landing on its perch
        pmLeapT -= dt;
        velocity.y -= 24.f * dt;
        position += velocity * dt;
        moveSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
        if (pmLeapT <= 0.f) {
            position = pmPerch(pmPhase);   // (lands exactly: the arc is planned to)
            velocity = glm::vec3{0.f};
            ev.slam = true; ev.slamRadius = 4.f; ev.slamDamage = 0.f;   // the dust as it lands
            recover(0.9f);
        }
        return;
    }
    if (phaseHold > 0.f) { phaseHold -= dt; velocity.x = velocity.z = 0.f; turnToward(flatTo(w.playerFeet), dt, 6.f); return; }

    // ---- the beacon (under 10 %, up on the roof) ----
    if (pmChannel > 0.f) {
        velocity.x = velocity.z = 0.f;
        pmChannel -= dt;
        if (pmChannelDmg >= maxHealth * PM_BREAK_SHARE) {   // broken off: it reels
            pmChannel = 0.f; pmChannelDmg = 0.f; ev.pmChannel = -1;
            stagger(2.6f);
            pmBeaconCd = 14.f;
        } else if (pmChannel <= 0.f) {   // lit: it drinks it in
            pmChannel = 0.f;
            health = std::min(maxHealth, health + maxHealth * 0.15f);
            ev.pmChannel = 2;
            pmBeaconCd = 20.f;
            recover(1.f);
        }
        return;
    }
    if (pmBeaconCd > 0.f) pmBeaconCd -= dt;
    if (pmPhase == 3 && hp < 0.10f && pmBeaconCd <= 0.f && pmBeacons < 2 && telegraphTimer <= 0.f && dashTimer <= 0.f) {
        ++pmBeacons;
        pmChannel = PM_CHANNEL; pmChannelDmg = 0.f; ev.pmChannel = 1;
        return;
    }

    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
    const float dy = w.playerFeet.y - position.y;
    const bool level = std::fabs(dy) < 2.6f;           // you're on its floor
    const float quick = enraged ? 0.8f : 1.f;           // the overclock: every tell a fifth shorter (never past the floor)

    // ---- a dash in flight: down the line, cutting what it meets ----
    if (dashTimer > 0.f) {
        dashTimer -= dt;
        glm::vec3 fwd{std::sin(yaw), 0.f, std::cos(yaw)};
        velocity.x = fwd.x * 20.f; velocity.z = fwd.z * 20.f;
        if (!dashHit && level && d < 2.4f) { dashHit = true; ev.meleeHit = true; ev.meleeDamage = 24.f; }
        if (dashTimer <= 0.f) {
            dashTimer = 0.f;
            velocity.x = velocity.z = 0.f;
            if (comboLeft > 0) { --comboLeft; startAttack(AttackKind::THRUST, 0.32f * quick, true); }
            else recover(0.8f);
        }
        return;
    }

    // ---- moving: in the yard it duels; up top it holds its perch and moves along it ----
    if (telegraphTimer > 0.f) {
        velocity.x = velocity.z = 0.f;
        if (attack == AttackKind::DASH) turnToward(to, dt, 5.f);   // it tracks you through the wind-up, then commits
        else turnToward(to, dt, 3.f);
    } else if (recoverTimer > 0.f) {
        velocity.x = velocity.z = 0.f;
    } else if ((pmPhase == 0 || level) && evadeTick(dt, w)) {   // in a duel, hurt hard and fast: it breaks away across your aim
        turnToward(to, dt, 6.f);
    } else {
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(1.2f, 2.2f); strafeDir = -strafeDir; }
        glm::vec3 mv;
        if (pmPhase == 0 || level) mv = d > 12.f ? dir + side * strafeDir * 0.3f : d < 6.f ? side * strafeDir - dir * 0.3f : side * strafeDir;
        else {   // you're below (or above): keep to its perch, pacing
            glm::vec3 home = flatTo(pmPerch(pmPhase));
            mv = glm::length(home) > 3.f ? norm2(home) : side * strafeDir * 0.5f;
        }
        setMove(mv, stats().speed * (enraged ? 1.25f : 1.f), w);
        turnToward(to, dt, 6.f);
    }

    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        switch (k) {
            case AttackKind::DASH: case AttackKind::THRUST:
                dashTimer = k == AttackKind::THRUST ? 0.3f : 0.45f; dashHit = false;
                ev.dashStarted = true; ev.slash = 3;
                break;
            case AttackKind::SWEEP:
                if (level && d < 4.8f) { ev.meleeHit = true; ev.meleeDamage = 22.f; }
                ev.slash = (attackCount & 1) ? 0 : 1;
                if (comboLeft > 0) { --comboLeft; startAttack(AttackKind::SWEEP, 0.3f * quick, true); }
                else recover(0.8f);
                break;
            case AttackKind::VOLLEY:
                fireAt(w.playerEye, enraged ? 9 : 7, 0.6f, 15.f, 9.f, 1.1f);
                ev.shotParry = 50.f;
                recover(0.8f);
                break;
            case AttackKind::CRESCENT:
                fireAt(w.playerEye, 3, 0.12f, 26.f, 12.f, 0.9f);
                if (comboLeft > 0) { --comboLeft; startAttack(AttackKind::CRESCENT, 0.32f, true); }
                else recover(0.8f);
                break;
            case AttackKind::SLAM:   // a ring along the floor it stands on
                ev.ring = true; ev.ringAt = position;
                ev.slam = true; ev.slamRadius = 3.5f; ev.slamDamage = level && d < 3.5f ? 20.f : 0.f;
                recover(0.8f);
                break;
            default: break;
        }
        return;
    }

    if (recoverTimer > 0.f || !attackReady(dt)) return;
    ++attackCount;
    const bool see = lineOfSight(eyePos(), w);
    if (level && d < 4.2f) {   // in reach: both blades round, or a short dash through you
        comboLeft = enraged ? 1 : 0;
        startAttack(attackCount % 2 ? AttackKind::SWEEP : AttackKind::DASH, 0.5f * quick);
    } else if (level && d < 13.f && see && attackCount % 3 != 0) {
        comboLeft = enraged ? 1 : (attackCount % 2);   // a dash, sometimes followed by a thrust
        startAttack(AttackKind::DASH, 0.6f * quick);
    } else if (pmPhase >= 1 && !level && see) {   // you're below: it shoots down at you, or rings its floor
        if (attackCount % 3 == 0) startAttack(AttackKind::SLAM, 0.7f * quick);
        else { comboLeft = 1 + (enraged ? 1 : 0); startAttack(AttackKind::CRESCENT, 0.55f * quick); }
    } else if (pmPhase >= 1 && attackCount % 4 == 0) startAttack(AttackKind::SLAM, 0.7f * quick);
    else if (see) startAttack(AttackKind::VOLLEY, stats().telegraph * quick);
    else attackTimer = stats().attackEvery * 0.6f;
}
