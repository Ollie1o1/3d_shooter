#pragma once
// =============================================================================
// Gameplay_Leviathan.h — GameplayState: THE LEVIATHAN's fight in the Maw. Its
// events turned into hazards and damage (LeviathanHazards.h), the flood in its
// last phase, the swallow's pull and bite, where a shot lands on it (plates,
// head, eye, throat), parried orbs steering into its eye, its banners and
// drawing what it does to the ring. Its body is a rig (EnemyModel.h).
// Included at the end of GameplayState.h.
// =============================================================================

inline Enemy* GameplayState::leviathan() {
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::LEVIATHAN) return &e;
    return nullptr;
}

inline void GameplayState::onLeviathanEvents(Enemy& e, const EnemyEvents& ev) {
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    const float floorY = e.levFloor;
    const glm::vec3 red{1.3f, 0.2f, 0.1f}, water{0.4f, 0.12f, 0.12f};
    if (ev.lvRise) {
        glm::vec3 c{e.levRoot.x, floorY, e.levRoot.z};
        shake(1.0f, 0.06f); audio.duck(6.f, 1.2f);
        audio.playAt("explosion", c, 128, SoundGroup::WORLD, true, 0.5f);
        fx.spawnBurst(c + glm::vec3{0.f, 1.f, 0.f}, water, 80, 14.f, 1.2f, 9.f);
        fx.spawnShockwave(c, Enemy::LV_POOL + 2.f, red);
    }
    if (ev.lvPhase == 2) {
        pushBanner("IT GOES UNDER", "WATCH THE WELLS", {1.f, 0.35f, 0.2f}, 3.f);
        shake(0.5f, 0.06f); audio.duck(6.f, 0.6f);   // (its roar: the voice director, on ev.enraged)
        lev.dropMarks();
    }
    if (ev.lvPhase == 3) {
        pushBanner("THE MAW FLOODS", "GET HIGH - SHOOT THE EYE", {1.f, 0.3f, 0.15f}, 3.f);
        shake(0.6f, 0.07f); audio.duck(6.f, 0.6f);
        shifts.floodTo(level, director.arena, floorY + FLOOD_DEPTH, 4.f);
        lev.dropMarks();
    }
    if (ev.lvCrashMark) lev.markCrash(ev.lvFrom, ev.lvTo);
    if (ev.lvCrash) {
        lev.landCrash();
        glm::vec3 at = ev.lvTo;
        shake(0.9f, 0.09f);
        audio.playAt("slam", at, 128, SoundGroup::ENEMY, true, 0.6f);
        audio.playAt("explosion", at, 110, SoundGroup::WORLD);
        fx.spawnShockwave(at, 7.f, red);
        fx.spawnBurst(at + glm::vec3{0.f, 0.5f, 0.f}, {0.4f, 0.35f, 0.3f}, 40, 10.f, 0.9f, 14.f);   // the ring breaking
        if (LeviathanHazards::inStrip(ev.lvFrom, ev.lvTo, player.position, floorY) &&
            damagePlayer(LeviathanHazards::CRASH_DAMAGE * scale, at, 0.4f, 0.09f)) {
            glm::vec2 d{ev.lvTo.x - ev.lvFrom.x, ev.lvTo.z - ev.lvFrom.z};
            glm::vec2 r{player.position.x - ev.lvFrom.x, player.position.z - ev.lvFrom.z};
            glm::vec2 side{-d.y, d.x};
            side = glm::length(side) > 1e-3f ? glm::normalize(side) * (glm::dot(side, r) >= 0.f ? 1.f : -1.f) : glm::vec2{1.f, 0.f};
            player.velocity += glm::vec3{side.x, 0.f, side.y} * 12.f + glm::vec3{0.f, 6.f, 0.f};   // thrown out of the strip
        }
        if (!beachHinted) { beachHinted = true; ui.toast("BEACHED", "THE EYE'S OPEN - THREE SECONDS", {1.f, 0.75f, 0.3f}, 2.f); }
    }
    if (ev.lvTide) {
        lev.addTide(ev.lvTideAt, e.levAtPool() ? Enemy::LV_POOL : 3.5f, ev.lvTideR);
        audio.playAt("slam", ev.lvTideAt, 120, SoundGroup::ENEMY, true);
        audio.playAt("skim", ev.lvTideAt, 110, SoundGroup::WORLD);
        fx.spawnBurst(ev.lvTideAt + glm::vec3{0.f, 1.f, 0.f}, water, 40, 10.f, 0.8f, 9.f);
    }
    if (ev.lvSpit) {
        glm::vec3 at = ev.lvSpitAt; at.y = groundHeightAt(at.x, at.z, at.y + 1.f);
        lev.addSpit(at);
    }
    if (ev.lvBreachTell) {
        lev.addBoil(ev.lvSite);
        audio.playAt("barrier", ev.lvSite, 120, SoundGroup::ENEMY, true, 0.4f);   // a rumble under the well
    }
    if (ev.lvBreach) {
        glm::vec3 at = ev.lvSite;
        shake(0.8f, 0.08f);
        audio.playAt("explosion", at, 128, SoundGroup::WORLD, true, 0.5f);
        fx.spawnBurst(at + glm::vec3{0.f, 1.f, 0.f}, water, 60, 16.f, 1.f, 9.f);
        fx.spawnShockwave(at, LeviathanHazards::BREACH_RADIUS, red);
        if (LeviathanHazards::breachHits(at, player.position) && damagePlayer(LeviathanHazards::BREACH_DAMAGE * scale, at, 0.4f, 0.09f)) {
            glm::vec3 away = player.position - at; away.y = 0.f;
            if (glm::length(away) > 0.1f) away = glm::normalize(away); else away = {1.f, 0.f, 0.f};
            player.velocity += away * 8.f + glm::vec3{0.f, 13.f, 0.f};   // thrown up out of it
        }
    }
    if (ev.lvInhale) audio.playAt("skim", e.levMouth(), 128, SoundGroup::ENEMY, true);
}

// A shot that found its throat while it inhales: enough and it chokes
inline void GameplayState::throatHit(Enemy& e, float dmg) {
    if (!e.levThroatHit(dmg)) return;
    lev.dropMarks();
    ui.toast("IT CHOKES", "FOUR SECONDS - UNLOAD", {1.f, 0.75f, 0.2f}, 1.8f);
    styleSystem.addStyle(60.f, StyleSource::PARRY);
    audio.playAt("v_leviathan_choke", e.position, 128, SoundGroup::ENEMY, true);
    shake(0.5f, 0.06f);
}

inline void GameplayState::updateLeviathan(float dt) {
    if (levSinkT < LV_SINK) {   // dead: rearing once more, then down into the dark
        levSinkT += dt;
        levCorpse.prevPosition = levCorpse.position; levCorpse.prevYaw = levCorpse.yaw;
        glm::vec3 base{levCorpse.levRoot.x, levCorpse.levFloor, levCorpse.levRoot.z};
        glm::vec3 want = levSinkT < 0.8f ? base + glm::vec3{0.f, 17.f, 0.f} : base - glm::vec3{0.f, 12.f, 0.f};
        levCorpse.position += (want - levCorpse.position) * std::min(1.f, dt * (levSinkT < 0.8f ? 4.f : 1.6f));
        levCorpse.levPitch = levSinkT < 0.8f ? 0.8f : -0.6f; levCorpse.levJaw = levSinkT < 0.8f ? 1.f : 0.2f;
        if (std::fmod(levSinkT, 0.2f) < dt) fx.spawnBurst(base + glm::vec3{frand(-6.f, 6.f), 0.5f, frand(-6.f, 6.f)}, {0.4f, 0.12f, 0.12f}, 6, 8.f, 0.8f, 9.f);
    }
    Enemy* e = leviathan();
    if (e && (e->staggered() || e->attack != AttackKind::CRASH)) lev.dropMarks();   // a crash that never came
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    // The swallow: dragged toward its mouth along the ground, bitten if you get there
    if (e && e->levInhale > 0.f && !playerDead) {
        glm::vec3 mouth = e->levMouth();
        glm::vec3 pull = LeviathanHazards::pull(mouth, player.position);
        player.position += pull * dt;   // the player's own collision holds it against walls
        player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
        if (std::fmod(gameClock, 0.08f) < dt)
            fx.spawnBurst(player.position + glm::vec3{frand(-3.f, 3.f), 1.f, frand(-3.f, 3.f)}, {0.8f, 0.8f, 0.85f}, 2, 3.f, 0.5f, 0.f);
        if (LeviathanHazards::bites(mouth, player.position)) {
            if (damagePlayer(LeviathanHazards::BITE_DAMAGE * scale, mouth, 0.4f, 0.09f)) {
                glm::vec3 away = player.position - mouth; away.y = 0.f;
                if (glm::length(away) > 0.1f) player.velocity += glm::normalize(away) * 18.f + glm::vec3{0.f, 6.f, 0.f};
            }
            audio.playAt("slam", mouth, 128, SoundGroup::ENEMY, true);
            e->levEndInhale();
        }
    }
    // Its hazards (they outlast it a moment: a spit already burning)
    if (!e && lev.strips.empty() && lev.tides.empty() && lev.spits.empty() && lev.boils.empty()) return;
    float floorY = e ? e->levFloor : level.lairFloor;
    for (const auto& h : lev.update(dt, player.position, floorY))
        damagePlayer(h.damage * scale, h.from, h.damage < 10.f ? 0.1f : 0.3f, h.damage < 10.f ? 0.03f : 0.07f, h.damage < 10.f ? 0.f : 0.25f);
    for (auto& b : lev.boils)   // the well boiling
        if (rand() % 2 == 0)
            fx.spawnBurst(b.at + glm::vec3{frand(-3.f, 3.f), 0.2f, frand(-3.f, 3.f)}, {1.2f, 0.25f, 0.12f}, 2, 4.f, 0.5f, 6.f);
}

// A parried orb in flight steers into what it's homing on: the Warden's core,
// the Leviathan's eye. A target gone (dead, hidden) and it flies straight on
inline void GameplayState::steerParriedOrbs(float dt) {
    for (auto& p : projSystem.pool) {
        if (!p.alive || p.homeOn < 0) continue;
        Enemy* t = nullptr;
        for (auto& o : enemies) if (o.alive && o.uid == p.homeOn) t = &o;
        if (!t || !t->targetable()) { p.homeOn = -1; continue; }
        glm::vec3 target;
        AABB cb;
        if (t->type == EnemyType::LEVIATHAN) { AABB eb = leviathanEye(*t); target = (eb.min + eb.max) * 0.5f; }
        else target = coreBox(*t, cb) ? (cb.min + cb.max) * 0.5f : t->position + glm::vec3{0.f, t->height() * 0.62f, 0.f};
        p.velocity = steerParried(p.position, p.velocity, target, dt);
    }
}

inline void GameplayState::gatherLeviathanBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float t = gameClock;
    const glm::vec3 red{1.6f, 0.2f, 0.1f}, acid{0.4f, 1.2f, 0.8f}, white{1.6f, 1.5f, 1.4f};
    if (levSinkT < LV_SINK) {   // its corpse, going under (the eye cracked: dark)
        Enemy pose = levCorpse;
        pose.position = glm::mix(levCorpse.prevPosition, levCorpse.position, renderAlpha);
        pose.hitFlashTimer = 0.12f * std::max(0.f, 1.f - levSinkT * 2.f);
        size_t from = out.size();
        buildEnemy(pose, t, out);
        float k = std::max(0.f, 1.f - levSinkT / LV_SINK);
        for (size_t j = from; j < out.size(); ++j) out[j].emissive *= k;
    }
    Enemy* e = leviathan();
    float floorY = e ? e->levFloor : level.lairFloor;
    // The crash's strip: a red band burning brighter until it lands; then the scar, fading
    for (const auto& s : lev.strips) {
        glm::vec3 d = s.to - s.from; d.y = 0.f;
        float len = glm::length(d) + LeviathanHazards::STRIP_PAST;
        if (len < 1e-3f) continue;
        glm::vec3 dir = d / std::max(glm::length(d), 1e-3f);
        float yaw = std::atan2(dir.x, dir.z);
        glm::vec3 mid = glm::vec3{s.from.x, floorY + 0.04f, s.from.z} + dir * (len * 0.5f);
        float k = s.landed ? 1.f - s.t / LeviathanHazards::STRIP_FADE
                           : (e && e->attack == AttackKind::CRASH ? 0.3f + 0.9f * e->telegraphProgress() : 0.3f);
        float pulse = s.landed ? 1.f : 0.75f + 0.25f * std::sin(t * 24.f);
        push(out, T(mid) * RY(yaw) * S({LeviathanHazards::STRIP_HALF * 2.f, 0.05f, len}), red * 0.2f, red * (1.4f * k * pulse));
        for (float sd : {-1.f, 1.f})   // its edges, sharper
            push(out, T(mid + glm::vec3{dir.z, 0.f, -dir.x} * (sd * LeviathanHazards::STRIP_HALF)) * RY(yaw) * S({0.15f, 0.08f, len}),
                 red * 0.2f, red * (2.2f * k));
    }
    // Tides: a wall of black water and red foam rolling outward
    for (const auto& w : lev.tides) {
        const int SEG = 56; float segLen = 6.2832f * std::max(w.radius, 0.5f) / SEG;
        for (int k = 0; k < SEG; ++k) {
            float a = (k + 0.5f) / SEG * 6.2832f;
            glm::vec3 p = w.centre + glm::vec3{std::cos(a) * w.radius, 0.55f, std::sin(a) * w.radius};
            push(out, T(p) * RY(-a) * S({0.6f, 1.1f, segLen * 0.9f}), {0.03f, 0.02f, 0.03f}, red * 0.5f);
            push(out, T(p + glm::vec3{0.f, 0.6f, 0.f}) * RY(-a) * S({0.7f, 0.12f, segLen * 0.9f}), {0.1f, 0.05f, 0.05f}, red * 1.4f);
        }
    }
    // Spits: a marker growing on the ground; then a burning pool of acid
    for (const auto& s : lev.spits) {
        if (!s.burst) {
            float k = std::min(1.f, s.t / LeviathanHazards::SPIT_WARN);
            float r = LeviathanHazards::SPIT_RADIUS * 2.f * k;
            push(out, T(s.at + glm::vec3{0.f, 0.05f, 0.f}) * S({r, 0.06f, r}), red * 0.3f, red * (0.6f + 0.9f * k));
        } else {
            float fade = std::min(1.f, (LeviathanHazards::SPIT_WARN + LeviathanHazards::SPIT_BURN - s.t) * 2.f);
            for (int k = 0; k < 6; ++k)
                push(out, T(s.at + glm::vec3{0.f, 0.05f, 0.f}) * RY(k * 0.5236f) * S({LeviathanHazards::SPIT_RADIUS * 2.f, 0.05f, 1.1f}),
                     {0.05f, 0.1f, 0.06f}, acid * fade * (0.8f + 0.2f * std::sin(t * 7.f + k)));
        }
    }
    // A boiling well: its rim burning, the water churning red
    for (const auto& b : lev.boils) {
        float k = std::min(1.f, b.t / LeviathanHazards::BOIL_TIME);
        for (int i = 0; i < 16; ++i) {
            float a = i * 0.3927f;
            glm::vec3 p = b.at + glm::vec3{std::cos(a) * LeviathanHazards::BREACH_RADIUS, 0.1f, std::sin(a) * LeviathanHazards::BREACH_RADIUS};
            push(out, T(p) * RY(-a) * S({0.2f, 0.15f, 2.4f}), red * 0.2f, red * (0.5f + 1.5f * k));
        }
        push(out, T(b.at + glm::vec3{0.f, 0.06f + 0.3f * std::sin(t * 30.f) * k, 0.f}) * S({7.f, 0.06f, 7.f}), red * 0.2f, red * (0.4f + 1.4f * k));
    }
    if (!e) return;
    // Hidden: a red wake running under the cracks toward where it'll come up
    if (e->levStage == Enemy::LevStage::HIDDEN) {
        float k = 1.f - e->levStageT / Enemy::LV_HIDDEN;
        glm::vec3 a = e->levWakeFrom, b = e->levTarget;
        for (int i = 0; i < 6; ++i) {
            float u = glm::clamp(k - i * 0.06f, 0.f, 1.f);
            glm::vec3 p = glm::mix(a, b, u); p.y = floorY + 0.06f;
            push(out, T(p) * S(glm::vec3{2.4f - 0.3f * i, 0.05f, 2.4f - 0.3f * i}), red * 0.2f, red * (1.5f - 0.2f * i));
        }
    }
    // Inhaling: streaks of air drawn into its mouth
    if (e->levInhale > 0.f) {
        glm::vec3 m = e->levMouth();
        for (int i = 0; i < 10; ++i) {
            float a = i * 0.628f + t * 2.f, r = 3.f + std::fmod(t * 9.f + i * 1.7f, 9.f);
            glm::vec3 p = m + glm::vec3{std::cos(a) * r, -0.5f + 0.3f * std::sin(a * 3.f), std::sin(a) * r};
            glm::vec3 d = m - p;
            push(out, T(p) * RY(std::atan2(d.x, d.z)) * S({0.06f, 0.06f, 1.4f}), white * 0.2f, white * 0.9f);
        }
    }
}
