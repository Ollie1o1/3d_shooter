#pragma once
// =============================================================================
// Gameplay_Penitent.h — the Descent's cage and the PENITENT, wired into
// GameplayState: the ride between waves, the chain anchors (shoot them or
// grapple on and rip them out), its hazards turned into damage, drawing and
// sound. Included at the end of GameplayState.h.
// =============================================================================

// The cage: a ride waits for the player to be aboard, the next wave waits for
// the ride, and the fall line follows the cage
inline void GameplayState::updateLift(float dt) {
    (void)dt;
    const Arena& ar = level.arenas[director.arena];
    if (ar.shift != ArenaShift::DESCENT) { director.hold = false; return; }
    auto& lift = level.lift;
    const glm::vec3 C{0.f, lift.y(), -840.f};
    if (g_devCam && lift.pending >= 0 && !lift.riding()) {   // dev camera (screenshots): the cage is simply there
        lift.at = lift.to = lift.pending; lift.pending = -1; lift.t = 0.f; lift.update(0.f, level);
    }
    if (lift.pending >= 0 && !lift.riding()) {
        if (player.onGround && level.onLift(player.groundWall)) {
            lift.start();
            liftRiding = true;
            for (auto& p : projSystem.pool) p.alive = false;   // nothing left over from the last floor
            pickups.clear();
            audio.playAt("door_close", C, 128, SoundGroup::WORLD);
            for (int k = 0; k < 4; ++k)   // the chains take the weight, overhead
                audio.playAt("clank", glm::vec3{k < 2 ? -9.f : 9.f, C.y + 8.f, k % 2 ? -831.f : -849.f}, 70, SoundGroup::WORLD);
        } else if (!boardHinted && director.phase != WaveDirector::Phase::ACTIVE) {
            boardHinted = true;
            pushBanner("BOARD THE CAGE", "IT WON'T GO DOWN WITHOUT YOU", {1.f, 0.7f, 0.3f}, 2.5f);
        }
    }
    if (liftRiding && !lift.riding()) {   // arrived: a jolt, and health waiting on the cage
        liftRiding = false;
        boardHinted = false;
        shake(0.35f, 0.05f);
        audio.playAt("slam", C, 110, SoundGroup::WORLD);
        for (float x : {-4.f, 4.f})
            pickups.push_back({C + glm::vec3{x, 0.6f, 0.f}, glm::vec3{0.f}, 1e9f, PickupKind::ORB, C.y, C + glm::vec3{x, 0.6f, 0.f}});
    }
    // ...and the next fight waits for you to come down to it (no perching on the floor above)
    bool above = !g_devCam && player.position.y > lift.y() + 6.f;
    if (above && !lift.busy() && !downHinted && director.phase != WaveDirector::Phase::ACTIVE) {
        downHinted = true;
        pushBanner("GET DOWN TO THE CAGE", "THE FIGHT IS BELOW YOU", {1.f, 0.7f, 0.3f}, 2.5f);
    }
    if (!above) downHinted = false;
    director.hold = lift.busy() || above;
    // The fall line: 25 m under the cage; a fall puts you back aboard
    Arena& mar = level.arenas[director.arena];
    mar.voidY = lift.y() - 25.f;
    mar.respawn = C;
}

// ---- THE PENITENT ------------------------------------------------------------

inline void GameplayState::onPenitentEvents(Enemy& e, const EnemyEvents& ev) {
    const Arena& ar = level.arenas[director.arena];
    const float scale = ar.damageScale * tune().damage;
    const float floorY = e.floorY;
    glm::vec3 at = e.position + glm::vec3{0.f, 3.f, 0.f};
    // Tells: a distinct sound for each, from where it kneels; big ones duck the rest
    if (ev.telegraphStarted) {
        switch (e.attack) {
            case AttackKind::CENSER_LOW:  audio.playAt("telegraph", at, 120, SoundGroup::ENEMY, true); break;   // grinding whine
            case AttackKind::CENSER_HIGH: audio.playAt("barrier", at, 128, SoundGroup::ENEMY, true); break;     // a bell tone
            case AttackKind::PSLAM: case AttackKind::PSTOMP: case AttackKind::SCOURGE:
                audio.playAt("telegraph", at, 128, SoundGroup::ENEMY, true); audio.duck(6.f, 0.5f); break;
            default: break;
        }
    }
    if (ev.penSweep >= 0) {
        pen.addSweep(e.position, e.yaw, e.sweepReach(), ev.penSweep);
        audio.playAt("dash", at, 128, SoundGroup::ENEMY, true);
        if (PenitentHazards::sweepHits(ev.penSweep, e.position, e.yaw, e.sweepReach(), player.position, player.height, floorY))
            damagePlayer(PenitentHazards::SWEEP_DAMAGE * scale, e.position, 0.3f, 0.07f);
    }
    if (ev.penSlam)  { pen.addRing(e.position, PenitentHazards::SLAM_RADIUS, PenitentHazards::SLAM_DAMAGE, 0);  audio.playAt("slam", e.position, 128, SoundGroup::ENEMY, true); shake(0.5f, 0.07f); }
    if (ev.penStomp) { pen.addRing(e.position, PenitentHazards::STOMP_RADIUS, PenitentHazards::SLAM_DAMAGE, 0); audio.playAt("slam", e.position, 110, SoundGroup::ENEMY, true); shake(0.3f, 0.05f); }
    if (ev.penEmbers) { pen.addRing(e.position, PenitentHazards::EMBER_RANGE, PenitentHazards::EMBER_DAMAGE, 1); audio.playAt("explosion", at, 100, SoundGroup::ENEMY, true); }
    if (ev.penLash) { pen.addLash(ev.penLashFrom, ev.penLashDir, e.risen); audio.playAt("clank", at, 128, SoundGroup::ENEMY, true); audio.duck(6.f, 0.5f); }
    for (int k = 0; k < ev.penIncense; ++k) pen.addPool(ev.penIncensePos[k]);
    if (ev.penIncense > 0) audio.playAt("skim", player.position, 80, SoundGroup::ENEMY);
    if (ev.penRose) {
        pushBanner("THE PENITENT RISES", "IT WALKS - KEEP MOVING, READ EVERY SWING", {1.f, 0.4f, 0.2f}, 3.f);
        audio.play("wave", 128, SoundGroup::UI); audio.duck(6.f, 0.8f); shake(0.8f, 0.08f);
    }
    const glm::vec3 base = e.position;   // spawning reallocates the enemy list: e is gone after the first
    for (int k = 0; k < ev.penSummon; ++k) {   // Hollowed Husks, round it
        float a = k * 1.5707963f + gameClock;
        glm::vec3 p = base + glm::vec3{std::cos(a) * 8.f, 0.f, std::sin(a) * 8.f};
        p.y = groundHeightAt(p.x, p.z, base.y + 2.f, true);
        spawnEnemy(EnemyType::HUSK, p, (Hollow)(1 + k % 3));
    }
}

inline void GameplayState::breakAnchor(int i, bool ripped) {
    const glm::vec3 p = level.anchors[i].pos;
    fx.spawnBurst(p, {1.2f, 0.6f, 0.25f}, 40, 9.f, 0.7f, 6.f);
    audio.playAt("clank", p, 128, SoundGroup::ENEMY, true, 0.5f);
    audio.playAt("explosion", p, 90, SoundGroup::WORLD);
    styleSystem.addStyle(ripped ? 40.f : 25.f, ripped ? StyleSource::PARRY : StyleSource::EXPLOSIVE);
    ui.feed(ripped ? "RIPPED" : "CHAIN BROKEN", {1.f, 0.7f, 0.3f});
    if (ripped) grapple.release();
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::PENITENT && !e.risen) { e.stagger(1.f); break; }
}

inline bool GameplayState::hitAnchor(glm::vec3 origin, glm::vec3 dir, float wallT, float dmg) {
    int i = level.anchorAlong(origin, dir, wallT);
    if (i < 0) return false;
    fx.spawnHitSparks(origin + dir * wallT, {1.2f, 0.7f, 0.3f});
    if (level.damageAnchor(i, dmg)) breakAnchor(i, false);
    return true;
}

inline void GameplayState::updatePenitent(float dt) {
    Enemy* boss = nullptr;
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::PENITENT) boss = &e;
    if (boss) boss->anchorsLeft = level.anchorsAlive();
    // Rip: hook an anchor and hang on half a second (reaching it counts)
    int hooked = -1;
    if (grapple.active)
        for (int i = 0; i < (int)level.anchors.size(); ++i)
            if (level.anchors[i].alive && grapple.hookedWall == level.anchors[i].wall) hooked = i;
    float rd = rip.anchor >= 0 ? glm::length(player.camera.position - level.anchors[rip.anchor].pos) : 1e9f;
    int ripped = rip.update(dt, hooked, grapple.active, rd);
    if (ripped >= 0 && level.damageAnchor(ripped, 1e9f)) breakAnchor(ripped, true);
    if (boss && boss->staggered()) pen.lashes.clear();   // reeling, its marked lash falls slack
    // Its hazards
    if (!boss && pen.rings.empty() && pen.lashes.empty() && pen.pools.empty()) return;
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    float floorY = boss ? boss->floorY : level.lift.y();
    for (const auto& h : pen.update(dt, player.position, floorY)) {
        if (!damagePlayer(h.damage * scale, h.from, 0.25f, 0.05f, h.damage < 10.f ? 0.f : 0.25f)) continue;
        if (h.yank) {
            glm::vec3 in = h.from - player.position; in.y = 0.f;
            if (glm::length(in) > 0.1f) player.velocity += glm::normalize(in) * 16.f + glm::vec3{0.f, 4.f, 0.f};
        }
    }
    // Phase 3's banner, once
    if (boss && boss->scourging && !scourgeAnnounced) {
        scourgeAnnounced = true;
        pushBanner("IT SCOURGES ITSELF", "STRIKE THE WOUND ON ITS BACK", {1.f, 0.25f, 0.15f}, 3.f);
        audio.duck(6.f, 0.6f);
    }
}

inline void GameplayState::gatherPenitentBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    float t = gameClock;
    const glm::vec3 ember{1.5f, 0.55f, 0.15f}, blood{1.6f, 0.15f, 0.08f}, amber{1.4f, 0.85f, 0.35f}, white{1.6f, 1.6f, 1.5f};
    Enemy* boss = nullptr;
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::PENITENT) boss = &e;
    // Anchors: a glowing sigil on an iron block; chains from its collar to each
    for (const auto& a : level.anchors) {
        if (!a.alive) continue;
        float pulse = 0.7f + 0.3f * std::sin(t * 3.f + a.pos.x);
        push(out, T(a.pos) * S(glm::vec3{1.8f}), {0.1f, 0.09f, 0.1f}, amber * 0.2f);
        push(out, T(a.pos) * S(glm::vec3{1.0f, 1.0f, 1.9f}), {0.2f, 0.1f, 0.05f}, amber * (1.2f * pulse));
        if (!boss || boss->risen) continue;
        glm::vec3 from = boss->position + glm::vec3{0.f, boss->height() * 0.75f, 0.f};
        glm::vec3 d = a.pos - from; float len = glm::length(d);
        int links = (int)(len / 0.9f);
        float yaw = std::atan2(d.x, d.z), pitch = -std::asin(d.y / std::max(len, 1e-3f));
        for (int k = 0; k < links; ++k) {
            glm::vec3 p = from + d * ((k + 0.5f) / links) - glm::vec3{0.f, std::sin((k + 0.5f) / links * 3.1416f) * len * 0.06f, 0.f};
            push(out, T(p) * RY(yaw) * RX(pitch) * RZ(k % 2 ? 1.5708f : 0.f) * S({0.22f, 0.5f, 0.75f}), {0.16f, 0.15f, 0.16f});
        }
    }
    // Sweep trails: a fan of fading slabs along the arc at its height
    for (const auto& a : pen.arcs) {
        float fade = 1.f - a.t / 0.4f, h = a.kind == 0 ? 0.5f : 2.2f;
        for (int k = -8; k <= 8; ++k) {
            float ang = a.yaw + k * PenitentHazards::HALF_ARC / 8.f;
            glm::vec3 p = a.centre + glm::vec3{std::sin(ang) * a.reach * 0.8f, h, std::cos(ang) * a.reach * 0.8f};
            push(out, T(p) * RY(ang) * S({a.reach * 0.2f, 0.12f, 0.25f}), {0.1f, 0.05f, 0.02f},
                 (a.kind == 0 ? amber : white) * 1.6f * fade);
        }
    }
    // Rings: segments round the expanding circle (embers redder)
    for (const auto& r : pen.rings) {
        const int SEG = 40; float segLen = 6.2832f * std::max(r.radius, 0.5f) / SEG;
        for (int k = 0; k < SEG; ++k) {
            float a = (k + 0.5f) / SEG * 6.2832f;
            push(out, T(r.centre + glm::vec3{std::cos(a) * r.radius, 0.35f, std::sin(a) * r.radius}) * RY(-a) * S({0.2f, 0.7f, segLen * 0.85f}),
                 {0.1f, 0.04f, 0.02f}, (r.kind == 1 ? blood : ember) * 1.5f);
        }
    }
    // The lash: a thin red line from its shoulder to where you stood while it's
    // warned, the chain itself (thick, blazing) when it strikes
    for (const auto& l : pen.lashes) {
        bool struck = l.t >= PenitentHazards::LASH_WARN;
        float yaw = std::atan2(l.dir.x, l.dir.z), pitch = -std::asin(glm::clamp(l.dir.y, -1.f, 1.f));
        glm::vec3 mid = l.from + l.dir * (PenitentHazards::LASH_REACH * 0.5f);
        float pulse = 0.5f + 0.5f * std::sin(t * 30.f);
        float thick = struck ? 0.55f : 0.12f;
        push(out, T(mid) * RY(yaw) * RX(pitch) * S({thick, thick, PenitentHazards::LASH_REACH}),
             {0.1f, 0.02f, 0.02f}, blood * (struck ? 2.5f : 0.6f + 1.2f * pulse));
    }
    // Incense: a burning disc (a hint while it lands, then full)
    for (const auto& p : pen.pools) {
        float on = p.t < PenitentHazards::POOL_WARN ? 0.3f : 1.f, fade = std::min(1.f, (PenitentHazards::POOL_TIME - p.t) * 2.f);
        for (int k = 0; k < 6; ++k)
            push(out, T(p.pos + glm::vec3{0.f, 0.04f, 0.f}) * RY(k * 0.5236f) * S({PenitentHazards::POOL_RADIUS * 2.f, 0.05f, 0.9f}),
                 {0.1f, 0.05f, 0.02f}, ember * on * fade * (0.8f + 0.2f * std::sin(t * 6.f + k)));
    }
    // The cage's dressing rides with it: a railing and four chains up into the dark
    if (!level.lift.movers.empty()) {
        float y = level.lift.y();
        const glm::vec3 C{0.f, y, -840.f};
        // Round the cage's outline (the corners of its four boxes): 16 corners,
        // a rail on the 12 short edges; the 4 straight edges across the axes
        // stay open, where the bridges meet the rim
        static const glm::vec2 RIM[16] = {{12.f, 5.f}, {10.5f, 8.f}, {8.f, 10.5f}, {5.f, 12.f}, {-5.f, 12.f}, {-8.f, 10.5f},
                                          {-10.5f, 8.f}, {-12.f, 5.f}, {-12.f, -5.f}, {-10.5f, -8.f}, {-8.f, -10.5f}, {-5.f, -12.f},
                                          {5.f, -12.f}, {8.f, -10.5f}, {10.5f, -8.f}, {12.f, -5.f}};
        for (int k = 0; k < 16; ++k) {
            glm::vec2 a = RIM[k], b = RIM[(k + 1) % 16], d = b - a;
            if (std::fabs(d.x) < 1e-3f || std::fabs(d.y) < 1e-3f) continue;   // an axis edge: a bridge
            glm::vec2 m = (a + b) * 0.5f;
            push(out, T(C + glm::vec3{m.x, 0.6f, m.y}) * RY(std::atan2(d.x, d.y)) * S({0.12f, 1.2f, glm::length(d)}), {0.12f, 0.11f, 0.12f});
        }
        for (int k = 0; k < 4; ++k) {
            float a = k * 1.5708f + 0.7854f;
            glm::vec3 base = C + glm::vec3{std::cos(a) * 12.f, 0.f, std::sin(a) * 12.f};
            push(out, T(base + glm::vec3{0.f, 30.f, 0.f}) * S({0.3f, 60.f, 0.3f}), {0.1f, 0.1f, 0.11f});
        }
    }
}
