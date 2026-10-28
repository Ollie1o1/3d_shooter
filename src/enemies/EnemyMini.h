#pragma once
// =============================================================================
// EnemyMini.h — the Gauntlet's mini-bosses (FAST): one to a room, and the room
// is won when it dies (whatever's left standing costs you a second each).
// Each moves on an older enemy's skeleton at a bigger size (rigBase,
// miniScale) with its own body made in Blender. Minds only; events out.
// Included at the end of Enemy.h.
//
//   THE COURIER    (the Canal)     a big hound with the room's key. It runs from
//                  you, dropping Mites; cornered or caught, it lunges. Hook it
//                  (it's heavy: you fly to it) and punch it.
//   THE BULWARK    (the Sluice)    a giant behind a sluice gate for a shield:
//                  nothing gets through the front. Fans of orbs, a slam you
//                  jump, a bash you can parry. Slow to turn: get behind it.
//   THE CANTOR     (the Ascent)    hangs under the rose window: sweeps a beam
//                  down the terraces, rings its bell (rings along the floor you
//                  jump), and fires fans. Climb to it through the gaps.
//   THE HARRIER    (the Span)      a giant raptor: strafes with fans, dives,
//                  and after a dive it skims low and slow - hook it, hit it
//                  (x1.6 while it skims).
//   THE PLUMB LINE (the Well)      a hanging weight that follows you down the
//                  shaft. Armoured (a third gets through) until it fires a
//                  ring of orbs: then its core opens (x2) for a moment.
//   THE FOREMAN    (the Pumpworks) floats along the crane line, tethered to up
//                  to three of its crew: while any tether holds it takes a
//                  third. Lobs heavy shells, calls Mites.
// =============================================================================

// How hard a mini-boss is to hurt right now (Enemy::armorMult)
inline float Enemy::miniArmor() const {
    switch (type) {
        case EnemyType::PLUMB:   return miniOpen > 0.f ? 2.f : 0.34f;
        case EnemyType::HARRIER: return miniPerch > 0.f ? 1.6f : 1.f;
        case EnemyType::FOREMAN: return linkCount > 0 ? 0.33f : 1.f;
        case EnemyType::BULWARK: return staggered() ? 2.f : 1.f;
        default: return 1.f;
    }
}

inline void Enemy::thinkMini(float dt, const EnemyWorld& w, bool resolve) {
    if (!miniHomeSet) { miniHome = position; miniHomeSet = true; }
    if (!enraged && health < maxHealth * 0.5f) { enraged = true; ev.enraged = true; }   // its second wind
    if (miniOpen > 0.f) miniOpen = std::max(0.f, miniOpen - dt);
    switch (type) {
        case EnemyType::COURIER: thinkCourier(dt, w, resolve); break;
        case EnemyType::BULWARK: thinkBulwark(dt, w, resolve); break;
        case EnemyType::CANTOR:  thinkCantor(dt, w, resolve);  break;
        case EnemyType::HARRIER: thinkHarrier(dt, w, resolve); break;
        case EnemyType::PLUMB:   thinkPlumb(dt, w, resolve);   break;
        case EnemyType::FOREMAN: thinkForeman(dt, w, resolve); break;
        default: break;
    }
}

// ---- THE COURIER -------------------------------------------------------------
inline void Enemy::thinkCourier(float dt, const EnemyWorld& w, bool resolve) {
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
    const float spd = stats().speed * (enraged ? 1.2f : 1.f);
    strafeTimer -= dt;
    if (strafeTimer <= 0.f) { strafeTimer = frand(1.1f, 2.f); strafeDir = -strafeDir; }
    if (attack == AttackKind::LUNGE) {   // crouched, then a long spring at you
        float s = telegraphTimer < 0.12f ? 19.f : 0.6f;
        velocity.x = dir.x * s; velocity.z = dir.z * s;
        if (resolve) {
            attack = AttackKind::NONE;
            if (d < 3.6f && std::fabs(w.playerFeet.y - position.y) < 2.6f) { ev.meleeHit = true; ev.meleeDamage = 24.f; }
            recoverTimer = 0.7f;
        }
        turnToward(to, dt, 10.f);
        return;
    }
    if (attack == AttackKind::SUMMON) {   // drops its Mites as it pauses
        velocity.x = velocity.z = 0.f;
        if (resolve) { ev.summonMites = enraged ? 3 : 2; attack = AttackKind::NONE; }
        turnToward(to, dt, 6.f);
        return;
    }
    // Keep away: run from you, slipping side to side; far off it idles round you
    glm::vec3 away = -dir;
    glm::vec3 mv = d < 17.f ? away + side * strafeDir * 0.85f : d > 26.f ? dir * 0.4f + side * strafeDir : side * strafeDir;
    if (recoverTimer > 0.f) mv = away + side * strafeDir;
    const bool cornered = !canStepTo(position + norm2(away) * (radius() + 1.6f), w);
    if (cornered) mv = side * strafeDir + dir * 0.2f;   // its back to a wall: it breaks along it
    setMove(mv, spd, w);
    turnToward(glm::vec3{velocity.x, 0.f, velocity.z}, dt, 9.f);
    attackTimer += dt * tune_->attackRate;
    if (recoverTimer <= 0.f && d < 5.f && (cornered || d < 3.2f || attackTimer > 1.4f)) {
        attackTimer = 0.f;
        startAttack(AttackKind::LUNGE, stats().telegraph);
    } else if (attackTimer >= stats().attackEvery * (enraged ? 0.75f : 1.f) && d < 32.f) {
        attackTimer = 0.f;
        startAttack(AttackKind::SUMMON, 0.5f);
    }
}

// ---- THE BULWARK -------------------------------------------------------------
inline void Enemy::thinkBulwark(float dt, const EnemyWorld& w, bool resolve) {
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
    if (telegraphTimer > 0.f) {
        float s = attack == AttackKind::BASH && d > 2.6f ? 3.f : 0.f;   // it shoulders in behind the gate
        velocity.x = dir.x * s; velocity.z = dir.z * s;
    } else {
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(2.f, 3.5f); strafeDir = -strafeDir; }
        glm::vec3 mv = d > 4.f ? dir + side * strafeDir * 0.15f : side * strafeDir * 0.5f;
        setMove(mv, stats().speed, w);
    }
    turnToward(to, dt, enraged ? 1.5f : 1.15f);   // slow to turn: that's the way in
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        if (k == AttackKind::BASH) {
            glm::vec3 fwd{std::sin(yaw), 0.f, std::cos(yaw)};
            if (d < 5.2f && std::fabs(w.playerFeet.y - position.y) < 2.4f && (d < 1.5f || glm::dot(fwd, to / d) > 0.4f)) {
                ev.meleeHit = true; ev.meleeDamage = 30.f;
            }
            ev.slam = true; ev.slamRadius = 4.f; ev.slamDamage = 0.f;   // the dust as the gate lands
            recoverTimer = 0.6f;
        } else if (k == AttackKind::SLAM) {   // the gate driven into the floor: a shockwave to jump
            ev.slam = true; ev.slamRadius = 11.f; ev.slamDamage = 24.f;
            recoverTimer = 0.5f;
        } else if (k == AttackKind::SHOT) {
            fireAt(w.playerEye, enraged ? 7 : 5, 0.55f, 16.f, 10.f, 1.2f);
            ev.shotParry = 60.f;
        }
    }
    if (recoverTimer > 0.f) return;
    if (attackReady(dt)) {
        ++attackCount;
        if (d < 4.6f) startAttack(AttackKind::BASH, 0.65f);
        else if (attackCount % 3 == 0 && d < 15.f) startAttack(AttackKind::SLAM, 0.95f);
        else if (d < 26.f && lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
        else attackTimer = stats().attackEvery * 0.6f;
    }
    if (d < 4.f && telegraphTimer <= 0.f && attackTimer > stats().attackEvery * 0.45f) startAttack(AttackKind::BASH, 0.65f);
}

// ---- a beam swept toward you (THE CANTOR's; as the SERAPH's) ------------------
inline bool Enemy::miniBeam(float dt, const EnemyWorld& w, float speed) {
    if (beamTimer <= 0.f) return false;
    beamTimer -= dt;
    glm::vec3 feet = w.playerFeet;
    glm::vec3 gap = feet - beamPoint;
    float l = glm::length(gap), step = speed * dt;
    beamPoint = l <= step ? feet : beamPoint + gap / l * step;
    glm::vec3 from = eyePos(), seg = beamPoint - from;
    float len = glm::length(seg);
    beamEnd = beamPoint;
    if (w.walls && len > 1e-3f) {
        glm::vec3 u = seg / len;
        AABB q{glm::min(from, beamPoint) - glm::vec3{0.5f}, glm::max(from, beamPoint) + glm::vec3{0.5f}};
        static std::vector<int> cands;
        if (w.grid) w.grid->query(q, cands);
        else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
        float best = len;
        for (int i : cands) { float t = rayBoxHit(from, u, w.walls[i].box); if (t > 0.f && t < best) best = t; }
        beamEnd = from + u * best;
    }
    ev.beamOn = true; ev.beamFrom = from; ev.beamTo = beamEnd;
    if (beamTimer <= 0.f) { beamTimer = 0.f; attackTimer = 0.f; }
    return true;
}

// ---- THE CANTOR --------------------------------------------------------------
inline void Enemy::thinkCantor(float dt, const EnemyWorld& w, bool resolve) {
    animPhase += dt * 3.f;
    glm::vec3 to = flatTo(w.playerFeet);
    glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
    // It keeps to its window: drifting a few metres either side of where it hung
    glm::vec3 target = miniHome + side * (std::sin(age * 0.45f) * 5.f);
    glm::vec3 go = target - position;
    bool busy = telegraphTimer > 0.f || beamTimer > 0.f;
    float s = busy ? 0.6f : 1.f;
    velocity.x = glm::clamp(go.x * 1.2f, -stats().speed, stats().speed) * s;
    velocity.z = glm::clamp(go.z * 1.2f, -stats().speed, stats().speed) * s;
    velocity.y = glm::clamp(go.y * 2.f + std::sin(age * 1.3f) * 0.3f, -4.f, 4.f);
    turnToward(to, dt, 3.f);
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        if (k == AttackKind::BEAM) beamTimer = 3.2f;
        else if (k == AttackKind::VOLLEY) {   // the bell: a ring along the floor you're on
            ev.ring = true; ev.ringAt = {position.x, w.playerFeet.y, position.z};
            if (comboLeft > 0) { --comboLeft; startAttack(AttackKind::VOLLEY, 0.55f, true); }
        } else if (k == AttackKind::SHOT) {
            fireAt(w.playerEye, 5, 0.45f, 17.f, 11.f, 1.2f);
            ev.shotParry = 60.f;
        }
    }
    if (miniBeam(dt, w, enraged ? 11.f : 9.f)) return;
    if (attackReady(dt)) {
        ++attackCount;
        const int pick = attackCount % 3;
        if (pick == 1 && lineOfSight(eyePos(), w)) {
            glm::vec2 v{w.playerVel.x, w.playerVel.z};
            glm::vec3 off = glm::length(v) > 1.f ? -glm::vec3{v.x, 0.f, v.y} / glm::length(v) : side;
            beamPoint = w.playerFeet + off * 6.f;
            startAttack(AttackKind::BEAM, 1.f);
        } else if (pick == 2) {
            comboLeft = enraged ? 1 : 0;
            startAttack(AttackKind::VOLLEY, 0.9f);
        } else if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
        else attackTimer = stats().attackEvery * 0.6f;
    }
}

// ---- THE HARRIER -------------------------------------------------------------
inline void Enemy::thinkHarrier(float dt, const EnemyWorld& w, bool resolve) {
    glm::vec3 to3 = w.playerEye - position;
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
    if (miniPerch > 0.f) {   // after a dive: skimming low and slow, open
        miniPerch -= dt;
        animPhase += dt * 3.f;
        float ty = std::max(w.playerFeet.y + 0.6f, floorY + 0.5f);
        velocity.x *= std::exp(-dt * 3.f); velocity.z *= std::exp(-dt * 3.f);
        velocity.y = glm::clamp((ty - position.y) * 2.f, -6.f, 3.f);
        pitch = glm::mix(pitch, 0.f, std::min(1.f, dt * 5.f));
        turnToward(to, dt, 2.f);
        if (miniPerch <= 0.f) { miniPerch = 0.f; recoverTimer = 1.f; }
        return;
    }
    if (diveTimer > 0.f) {
        animPhase += dt * 5.f;
        diveTimer -= dt;
        velocity = diveDir * 23.f;
        pitch = glm::mix(pitch, 0.7f, std::min(1.f, dt * 8.f));
        if (glm::length(w.playerEye - position - glm::vec3{0.f, height() * 0.5f, 0.f}) < 2.8f) {
            ev.meleeHit = true; ev.meleeDamage = 26.f;
            diveTimer = 0.f;
        }
        if (position.y < w.playerFeet.y + 0.8f) diveTimer = 0.f;   // pulled out at your height
        if (diveTimer <= 0.f) { diveTimer = 0.f; miniPerch = enraged ? 1.6f : 2.2f; velocity *= 0.3f; }
        turnToward(diveDir, dt, 8.f);
        return;
    }
    animPhase += dt * 7.f;
    pitch = glm::mix(pitch, 0.f, std::min(1.f, dt * 4.f));
    strafeTimer -= dt;
    if (strafeTimer <= 0.f) { strafeTimer = frand(3.f, 5.f); strafeDir = -strafeDir; }
    float radial = glm::clamp((d - 16.f) * 0.25f, -1.f, 1.f);
    glm::vec3 mv = norm2(side * strafeDir + dir * radial);
    velocity.x = mv.x * stats().speed * tune_->moveSpeed;
    velocity.z = mv.z * stats().speed * tune_->moveSpeed;
    float targetY = std::max(floorY + hoverY, w.playerFeet.y + 8.f) + (recoverTimer > 0.f ? 2.f : 0.f);
    velocity.y = glm::clamp((targetY - position.y) * 2.5f, -8.f, 8.f);
    turnToward(glm::vec3{velocity.x, 0.f, velocity.z}, dt, 4.f);
    if (resolve) {
        if (attack == AttackKind::SHOT) { fireAt(w.playerEye, 3, 0.25f, 20.f, 10.f, 1.1f); ev.shotParry = 60.f; }
        else if (attack == AttackKind::DIVE) {
            float l = glm::length(to3);
            diveDir = l > 1e-3f ? to3 / l : glm::vec3{0, -1, 0};
            diveTimer = 1.6f;
        }
        attack = AttackKind::NONE;
    }
    if (recoverTimer <= 0.f && attackReady(dt)) {
        ++attackCount;
        if (attackCount % (enraged ? 2 : 3) == 0 && lineOfSight(eyePos(), w)) startAttack(AttackKind::DIVE, 0.75f);
        else if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
    }
}

// ---- THE PLUMB LINE ----------------------------------------------------------
inline void Enemy::thinkPlumb(float dt, const EnemyWorld& w, bool resolve) {
    animPhase += dt * 2.f;
    // Down the shaft with you (never above where it hung), swaying round its line
    float ty = glm::clamp(w.playerFeet.y - 0.6f, floorY + 0.3f, miniHome.y);
    glm::vec3 sway{std::sin(age * 0.7f) * 1.2f, 0.f, std::cos(age * 0.55f) * 1.2f};
    glm::vec3 go = miniHome + sway - position;
    velocity.x = glm::clamp(go.x * 1.5f, -3.f, 3.f);
    velocity.z = glm::clamp(go.z * 1.5f, -3.f, 3.f);
    velocity.y = glm::clamp((ty - position.y) * 1.6f, -stats().speed, stats().speed);
    turnToward(flatTo(w.playerFeet), dt, 1.5f);
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        if (k == AttackKind::VOLLEY) {   // a ring of orbs out from its core, and the core opens
            const int n = enraged ? 16 : 12;
            ev.shotOrigin = position + glm::vec3{0.f, height() * 0.3f, 0.f};   // (out of its belly: the vents)
            ev.shotSpeed = 12.f * tune_->shotSpeed; ev.shotDamage = 12.f; ev.shotSize = 1.4f; ev.shotParry = 80.f;
            float tilt = glm::clamp((w.playerEye.y - ev.shotOrigin.y) / 12.f, -0.4f, 0.4f);
            for (int i = 0; i < n; ++i) {
                float a = 6.2831853f * (float)i / (float)n + age;
                ev.shotDir[i] = glm::normalize(glm::vec3{std::cos(a), tilt, std::sin(a)});
            }
            ev.shots = n;
            miniOpen = enraged ? 2.2f : 2.8f;
        } else if (k == AttackKind::SHOT) {
            fireAt(w.playerEye, 3, 0.2f, 19.f, 10.f, 1.1f);
            ev.shotParry = 60.f;
        }
    }
    if (attackReady(dt)) {
        ++attackCount;
        if (attackCount % 2 == 0) startAttack(AttackKind::VOLLEY, 1.f);
        else if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
    }
}

// ---- THE FOREMAN -------------------------------------------------------------
inline void Enemy::thinkForeman(float dt, const EnemyWorld& w, bool resolve) {
    animPhase += dt * 3.f;
    glm::vec3 toP = flatTo(w.playerFeet);
    float dP = glm::length(toP);
    // Along the crane line (X) either side of where it started, backing off if you close in
    glm::vec3 target = miniHome + glm::vec3{std::sin(age * 0.32f) * 17.f, 0.f, 0.f};
    glm::vec3 go = target - position;
    glm::vec3 mv{go.x, 0.f, go.z};
    float ml = glm::length(mv);
    mv = ml > 0.5f ? mv / ml * std::min(1.f, ml / 3.f) : glm::vec3{0.f};
    if (dP < 8.f) mv -= norm2(toP) * (8.f - dP) / 3.f;
    float spd = stats().speed * (linkCount == 0 ? 1.3f : 1.f);
    velocity.x = mv.x * spd; velocity.z = mv.z * spd;
    velocity.y = glm::clamp(go.y * 2.f + std::sin(age * 2.f) * 0.3f, -5.f, 5.f);
    turnToward(toP, dt, 3.f);
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        if (k == AttackKind::LOB) { fireAt(w.playerEye, 1, 0.f, 14.f, 20.f, 2.2f); ev.shotParry = 150.f; }
        else if (k == AttackKind::SHOT) fireAt(w.playerEye, 3, 0.3f, 18.f, 10.f);
        else if (k == AttackKind::SUMMON) ev.summonMites = 2;
    }
    if (attackReady(dt)) {
        ++attackCount;
        if (attackCount % 4 == 0) startAttack(AttackKind::SUMMON, 0.6f);
        else if (!lineOfSight(eyePos(), w)) attackTimer = stats().attackEvery * 0.6f;
        else if (attackCount % 2 == 0) startAttack(AttackKind::LOB, 0.8f);
        else startAttack(AttackKind::SHOT, stats().telegraph);
    }
}

// THE FOREMAN's crew: up to three of the nearest (not shielded by it: it's shielded by them)
inline void linkForemen(std::vector<Enemy>& es) {
    for (auto& c : es) {
        if (c.type != EnemyType::FOREMAN || !c.targetable()) continue;
        std::vector<bool> taken(es.size(), false);
        for (int n = 0; n < 3; ++n) {
            int best = -1; float bestD = 24.f;
            for (int i = 0; i < (int)es.size(); ++i) {
                const Enemy& o = es[i];
                if (taken[i] || &o == &c || !o.targetable() || isBoss(o.type) || isMiniBoss(o.type) ||
                    o.type == EnemyType::CONDUIT || o.type == EnemyType::CONDUCTOR || o.type == EnemyType::MITE) continue;
                float d = glm::length(o.position - c.position);
                if (d < bestD) { bestD = d; best = i; }
            }
            if (best < 0) break;
            taken[best] = true;
            c.links[c.linkCount++] = best;
        }
    }
}
