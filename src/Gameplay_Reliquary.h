#pragma once
// =============================================================================
// Gameplay_Reliquary.h — GameplayState: THE RELIQUARY. The relics drift to the
// next wave's arrangement once a wave is cleared (the fight waits for them to
// settle); a fall puts you back on the last relic you stood on; Revenant souls
// flee and re-form; Weaver wires snare and wake the wave; drawing all of it.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::updateReliquary(float dt) {
    const Arena& ar = level.arenas[director.arena];
    if (ar.shift != ArenaShift::DRIFT) { relic.clear(); snareTimer = 0.f; player.speedCap = 0.f; return; }
    auto& fm = level.formation;
    // The drift: started by ArenaShifts on a cleared wave; the fight waits for it
    if (fm.gliding() && !driftWasGliding) {
        relic.clearWires();
        for (auto& p : projSystem.pool) p.alive = false;
        pickups.clear();
        for (int c = 0; c < 4; ++c) audio.playAt("door", fm.top(c), 110, SoundGroup::WORLD);   // stone grinding as each relic lets go
        shake(0.25f, 0.03f);
    }
    if (!fm.gliding() && driftWasGliding) {   // settled: a jolt, and health on two of the relics
        audio.playAt("slam", fm.top(0), 100, SoundGroup::WORLD);
        for (int c : {1, 3}) {
            glm::vec3 p = fm.top(c) + glm::vec3{0.f, 0.6f, 0.f};
            pickups.push_back({p, glm::vec3{0.f}, 1e9f, PickupKind::ORB, p.y - 0.6f, p});
        }
    }
    driftWasGliding = fm.gliding();
    director.hold = fm.gliding();
    // The last relic you stood on: where a fall puts you back
    if (player.onGround) { int c = fm.chunkOfWall(level, player.groundWall); if (c >= 0) lastChunk = c; }
    Arena& mar = level.arenas[director.arena];
    mar.respawn = fm.top(lastChunk) + glm::vec3{0.f, 0.05f, 0.f};
    mar.voidY = fm.lowestTop() - 22.f;
    // Souls: fly; those that get there re-form their Revenant
    for (const Soul& s : relic.arrived(dt)) {
        glm::vec3 at = s.to;
        spawnEnemy(s.type, at, s.hollow);
        Enemy& e = enemies.back();
        e.maxHealth = e.health = s.bodyHealth; e.reforms = s.reforms; e.scale = s.scale;
        audio.playAt("v_revenant_reform", at + glm::vec3{0.f, 1.5f, 0.f}, 128, SoundGroup::ENEMY, true, 0.4f);
        fx.spawnBurst(at + glm::vec3{0.f, 1.f, 0.f}, {0.75f, 0.9f, 1.3f}, 40, 5.f, 0.9f, 6.f);
        ui.feed("IT CAME BACK", {0.75f, 0.9f, 1.3f});
    }
    // Wires: age, snare on touch (once per wire contact)
    relic.age(dt);
    if (snareTimer > 0.f) snareTimer -= dt;
    int wi = relic.touchedWire(player.position, player.height, player.radius);
    if (wi >= 0 && snareTimer <= 0.f) {
        snareTimer = RelicHazards::SNARE_TIME;
        damagePlayer(RelicHazards::SNARE_DAMAGE * level.arenas[director.arena].damageScale * tune().damage, player.position, 0.15f, 0.04f, 0.f);
        audio.playAt("v_weaver_twang", player.position + glm::vec3{0.f, 1.2f, 0.f}, 128, SoundGroup::ENEMY, true, 0.5f);
        ui.feed("SNARED", {0.75f, 0.35f, 1.1f});
        for (auto& e : enemies)   // the whole wave feels it
            if (e.targetable() && glm::length(e.position - player.position) < 30.f) e.attackTimer = e.stats().attackEvery;
        relic.cutWire(wi);   // it snaps
    }
    player.speedCap = snareTimer > 0.f ? 3.f : 0.f;   // snared: a crawl (Player::integrate), and no dash (Gameplay_Tick)
}

inline void GameplayState::onReliquaryEvents(Enemy& e, const EnemyEvents& ev) {
    if (ev.wire) {
        relic.addWire(e.uid, ev.wireA, ev.wireB);
        audio.playAt("v_weaver_twang", (ev.wireA + ev.wireB) * 0.5f, 90, SoundGroup::ENEMY, false, 0.3f);
    }
}

inline void GameplayState::releaseRevenantSoul(const Enemy& e, bool inVoid) {
    if (!revenantSoulEscapes(StyleSource::ENVIRONMENT, inVoid)) return;
    const Arena& ar = level.arenas[director.arena];
    const std::vector<glm::vec3>& spots = director.wave >= 0 && director.wave < (int)ar.waveGround.size() ? ar.waveGround[director.wave] : ar.groundSpawns;
    if (!relic.releaseSoul(e, soulDestination(spots, e.position, 15.f))) return;
    audio.playAt("v_revenant_soul", e.position + glm::vec3{0.f, e.height() * 0.6f, 0.f}, 128, SoundGroup::ENEMY, true, 0.4f);
    ui.feed("ITS SOUL RUNS", {0.75f, 0.9f, 1.3f});
}

// The parry key, aimed at a soul passing within reach: caught for good
inline bool GameplayState::catchSoulWithPunch() {
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();
    int i = relic.soulNear(eye + fwd * 1.5f, 2.5f);
    if (i < 0) return false;
    glm::vec3 at = relic.souls[i].pos;
    relic.removeSoul(i);
    parryFeedback(at, true);
    styleSystem.addStyle(80.f, StyleSource::PARRY);
    ui.toast("SOUL TAKEN", "BARE-HANDED", {0.75f, 0.9f, 1.3f}, 1.4f);
    return true;
}

inline void GameplayState::gatherReliquaryBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float t = gameClock;
    const glm::vec3 violet{0.75f, 0.35f, 1.1f}, soul{0.75f, 0.9f, 1.3f};
    for (const auto& w : relic.wires) {
        glm::vec3 d = w.b - w.a; float len = glm::length(d);
        float yaw = std::atan2(d.x, d.z), hum = 0.7f + 0.3f * std::sin(t * 30.f + w.a.x);
        push(out, T((w.a + w.b) * 0.5f) * RY(yaw) * S({0.04f, 0.04f, len}), violet * 0.3f, violet * (1.4f * hum));
        for (glm::vec3 n : {w.a, w.b}) push(out, T(n) * S(glm::vec3{0.35f}), violet * 0.3f, violet * 2.f);
    }
    for (const auto& s : relic.souls) {
        float pulse = 0.8f + 0.2f * std::sin(t * 12.f);
        push(out, T(s.pos) * RY(t * 3.f) * S(glm::vec3{0.6f * pulse}), soul * 0.5f, soul * 2.5f);
    }
}
