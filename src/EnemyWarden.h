#pragma once
// =============================================================================
// EnemyWarden.h — THE WARDEN's mind (Enemy::thinkWarden). Included at the end
// of Enemy.h. It only decides and emits events (EnemyEvents::w*);
// WardenHazards.h and Gameplay_Warden.h turn them into what hurts you.
//
//   CHARGING (100-60 %)  fed by its conduits (half damage until they're cut);
//                        volleys you can parry back, slams, Mites; every third
//                        attack it VENTS: the core open 2.5 s (x3).
//   OVERLOAD (60-25 %)   walks to the reactor and feeds; the reactor rings;
//                        volleys and the LANCE (a sweeping beam ending in a
//                        vent); hide or keep away 4 s and a SEEKER finds you.
//   MELTDOWN (< 25 %)    tears free, the core open (x2); everything quicker;
//                        lunges you can punch; 40 s until it goes off (it
//                        hurts you and heals to 25 %).
// =============================================================================

inline void Enemy::thinkWarden(float dt, const EnemyWorld& w, bool resolve) {
    const float hp = health / maxHealth;
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    // ---- phases ----
    if (wardenPhase == 1 && hp <= W_PHASE2) {
        wardenPhase = 2; ev.wPhase = 2; ev.enraged = true; phaseHold = PHASE_PAUSE;
        attack = AttackKind::NONE; telegraphTimer = 0.f; ventTimer = 0.f; sinceVent = 0; farTimer = 0.f;
        if (w.hasReactor) {   // the reactor's edge, on your side of it
            glm::vec3 out{w.playerFeet.x - w.reactor.x, 0.f, w.playerFeet.z - w.reactor.z};
            float l = glm::length(out);
            out = l > 0.1f ? out / l : glm::vec3{1.f, 0.f, 0.f};
            reactorSpot = glm::vec3{w.reactor.x, position.y, w.reactor.z} + out * 9.f;   // clear of the pedestal (a 10 m square: 7.1 m at its corners) with its 1.6 m girth
        } else { reactorSpot = position; }
        atReactor = false; spotBest = 1e9f; spotAt = age;
    }
    if (wardenPhase == 2 && hp <= W_PHASE3) {
        wardenPhase = 3; ev.wPhase = 3; ev.enraged = true; phaseHold = PHASE_PAUSE;
        attack = AttackKind::NONE; telegraphTimer = 0.f; lanceTimer = 0.f; ventTimer = 0.f; sinceVent = 0;
        meltClock = W_MELT_TIME; atReactor = false;
    }
    const float quick = wardenPhase == 3 ? 0.7f : 1.f;
    if (ventTimer > 0.f) ventTimer -= dt;
    if (phaseHold > 0.f) phaseHold -= dt;

    // ---- the meltdown clock: in its last 1.5 s it winds up to go off ----
    if (wardenPhase == 3) {
        meltClock -= dt;
        if (meltClock <= 1.5f && attack != AttackKind::DETONATE && !resolve) {   // (an attack landing this tick lands first)
            dashTimer = 0.f;
            startAttack(AttackKind::DETONATE, std::max(meltClock, 0.f));
        }
    }

    // ---- moving ----
    if (dashTimer > 0.f) {   // the lunge: dragged straight at you
        dashTimer -= dt;
        velocity.x = diveDir.x * W_LUNGE_SPEED; velocity.z = diveDir.z * W_LUNGE_SPEED;
        if (!dashHit && d < 2.8f) { dashHit = true; ev.meleeHit = true; ev.meleeDamage = 30.f; dashTimer = std::min(dashTimer, 0.05f); }
        if (dashTimer <= 0.f) { velocity.x = velocity.z = 0.f; }
        return;
    }
    if (lanceTimer > 0.f) {   // planted while the beam sweeps
        lanceTimer -= dt;
        velocity.x = velocity.z = 0.f;
        if (lanceTimer <= 0.f) { lanceTimer = 0.f; ventTimer = W_LANCE_VENT; ev.wVent = true; }
        return;
    }
    if (telegraphTimer > 0.f) { velocity.x = velocity.z = 0.f; }
    else if (wardenPhase == 2) {
        glm::vec3 go = flatTo(reactorSpot);
        if (!atReactor && glm::length(go) > 0.8f) {
            setMove(norm2(go), stats().speed * 1.3f, w); animPhase += dt * 3.f;
            // Something in the way: plant itself where it got to
            if (glm::length(go) < spotBest - 0.3f) { spotBest = glm::length(go); spotAt = age; }
            else if (age - spotAt > 1.5f) { atReactor = true; reactorSpot = position; }
        }
        else { atReactor = true; velocity.x = velocity.z = 0.f; }
    } else {
        float speed = stats().speed * (wardenPhase == 3 ? 1.4f : 1.f);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(3.f, 5.f); strafeDir = -strafeDir; }
        glm::vec3 mv = d > 11.f ? dir + side * strafeDir * 0.4f : side * strafeDir;
        setMove(mv, speed, w);
        animPhase += dt * 3.f;
    }
    turnToward(to, dt, wardenPhase == 3 ? 3.f : 2.f);

    // ---- what it's winding up lands ----
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        switch (k) {
            case AttackKind::SLAM:
                ev.slam = true; ev.slamRadius = 11.f; ev.slamDamage = 30.f; break;
            case AttackKind::VOLLEY:
                fireAt(w.playerEye, tune_->windup < 1.f ? 9 : 7, 0.9f, 18.f, 12.f, 1.6f);
                ev.shotParry = 150.f;   // parried back, it homes into the core (Gameplay_Warden.h)
                break;
            case AttackKind::SUMMON:
                ev.summonMites = 3; break;
            case AttackKind::WVENT:
                ventTimer = W_VENT_TIME; ev.wVent = true; break;
            case AttackKind::LANCE: {
                lanceSign = -lanceSign;
                float toYaw = std::atan2(to.x, to.z);
                ev.wLance = true; ev.wLanceSign = lanceSign;
                ev.wLanceYaw = toYaw - lanceSign * 1.0471976f;   // starts 60 degrees to one side, sweeps through you
                ev.wLanceFrom = position + glm::vec3{0.f, height() * 0.62f, 0.f};
                lanceTimer = W_LANCE_TIME;
                break;
            }
            case AttackKind::SEEKER: break;   // its marker went down when it began (WardenHazards bursts it)
            case AttackKind::WLUNGE: {
                diveDir = d > 0.1f ? to / d : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
                dashTimer = glm::clamp((d + 2.f) / W_LUNGE_SPEED, 0.2f, 0.9f); dashHit = false;
                ev.dashStarted = true;
                break;
            }
            case AttackKind::DETONATE:
                ev.wDetonate = true;
                health = maxHealth * W_PHASE3;
                meltClock = W_MELT_TIME;
                break;
            default: break;
        }
    }

    // ---- the seeker: hiding behind a pillar or keeping away doesn't last ----
    // (fed or feeding: phases 1 and 2; in its meltdown it comes for you itself)
    bool away = d > W_SEEK_FAR || !lineOfSight(eyePos(), w);
    farTimer = wardenPhase <= 2 && away ? farTimer + dt : 0.f;
    if (telegraphTimer > 0.f || phaseHold > 0.f) return;
    if (wardenPhase <= 2 && farTimer >= W_SEEK_AFTER) {   // even on its way to the reactor
        farTimer = 0.f;
        ev.wSeeker = true; ev.wSeekerAt = w.playerFeet + w.playerVel * 1.f;   // where you'll be when it lands (keep changing direction)
        ev.wSeekerAt.y = w.playerFeet.y;
        startAttack(AttackKind::SEEKER, 1.f);
        return;
    }

    // ---- what it starts next (overloaded: once it's at the reactor) ----
    if (wardenPhase == 2 && !atReactor) return;
    if (!attackReady(dt / quick)) return;
    ++attackCount;
    if (sinceVent >= 3 && wardenPhase != 2) { sinceVent = 0; startAttack(AttackKind::WVENT, 0.6f); return; }
    ++sinceVent; ++wardenAttacks;
    switch (wardenPhase) {
        case 1:
            if (d < 9.f)                   startAttack(AttackKind::SLAM, 1.0f);
            else if (wardenAttacks % 4 == 0) startAttack(AttackKind::SUMMON, 1.2f);
            else                           startAttack(AttackKind::VOLLEY, stats().telegraph);
            break;
        case 2:
            if (attackCount % 2 == 0) startAttack(AttackKind::LANCE, 1.0f);
            else                      startAttack(AttackKind::VOLLEY, stats().telegraph);
            break;
        default:
            if (d < 8.f)                   startAttack(AttackKind::SLAM, 1.0f * quick);
            else if (attackCount % 2 == 1) startAttack(AttackKind::WLUNGE, 0.8f * quick);
            else                           startAttack(AttackKind::VOLLEY, stats().telegraph * quick);
            break;
    }
}
