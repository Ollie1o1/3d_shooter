#pragma once
// =============================================================================
// Gameplay_Warden.h — GameplayState: THE WARDEN's fight. Its conduits (cut
// them all: it reels; they re-attach), its phases' banners, the reactor's
// rings while it feeds, the lance, seekers and steam (WardenHazards.h), the
// meltdown going off, parried orbs steering into its core, and drawing it all.
// Included at the end of GameplayState.h.
// =============================================================================

inline Enemy* GameplayState::warden() {
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::WARDEN) return &e;
    return nullptr;
}

inline void GameplayState::onWardenEvents(Enemy& e, const EnemyEvents& ev) {
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    if (ev.wPhase == 2) {
        pushBanner("THE WARDEN DRINKS THE CORE", "JUMP THE RINGS - PUNISH THE VENTS", {1.f, 0.2f, 0.65f}, 3.f);
        shake(0.5f, 0.06f); audio.duck(6.f, 0.6f);
        for (int i = 0; i < (int)level.anchors.size(); ++i)   // its conduits burn out for good
            if (level.anchors[i].kind == LevelData::ChainAnchor::CONDUIT && level.anchors[i].alive) {
                level.damageAnchor(i, 1e9f);
                fx.spawnBurst(level.anchors[i].pos, {0.3f, 0.9f, 1.f}, 20, 6.f, 0.5f, 4.f);
            }
        conduitClock.reset();
    }
    if (ev.wPhase == 3) {
        pushBanner("MELTDOWN", "40 SECONDS - END IT BEFORE IT BLOWS", {1.f, 0.35f, 0.15f}, 3.f);
        shake(0.6f, 0.07f); audio.duck(6.f, 0.6f);
    }
    if (ev.wLance) ward.addLance(ev.wLanceFrom, ev.wLanceYaw, ev.wLanceSign);
    if (ev.wSeeker) {
        glm::vec3 at = ev.wSeekerAt; at.y = groundHeightAt(at.x, at.z, at.y + 1.f);
        ward.addSeeker(at);
    }
    if (ev.wVent) fx.spawnBurst(e.position + glm::vec3{0.f, e.height() * 0.62f, 0.f}, {0.9f, 0.9f, 0.95f}, 30, 6.f, 0.8f, 3.f);
    if (ev.wDetonate) {
        glm::vec3 c = e.position + glm::vec3{0.f, 2.f, 0.f};
        fx.spawnExplosionParticles(c, 12.f); explosionFlashTimer = 0.4f; explosionFlashPos = c;
        shake(0.9f, 0.09f);
        audio.playAt("explosion", c, 128, SoundGroup::WORLD, true, 0.6f);
        float dmg = WardenHazards::DETONATE_DAMAGE * scale;
        float cap = std::max(styleSystem.health - 1.f, 0.f);   // a setback, never the killing blow
        if (modOn(DailyMod::GLASS_CANNON)) cap *= 0.5f;          // (damagePlayer doubles it)
        dmg = std::min(dmg, cap);
        if (dmg > 0.f) damagePlayer(dmg, c, 0.4f, 0.09f);
        glm::vec3 away = player.position - e.position; away.y = 0.f;
        if (glm::length(away) > 0.1f) player.velocity += glm::normalize(away) * 18.f + glm::vec3{0.f, 7.f, 0.f};
        pushBanner("IT BLEW - AND IT'S STILL STANDING", "ANOTHER 40 SECONDS", {1.f, 0.5f, 0.2f}, 2.f);
    }
}

inline void GameplayState::cutConduit(int i) {
    const glm::vec3 p = level.anchors[i].pos;
    fx.spawnBurst(p, {0.3f, 0.9f, 1.f}, 36, 9.f, 0.6f, 5.f);
    audio.playAt("clank", p, 128, SoundGroup::ENEMY, true, 0.5f);
    styleSystem.addStyle(25.f, StyleSource::EXPLOSIVE);
    ui.feed("CONDUIT CUT", {0.3f, 0.9f, 1.f});
    Enemy* w = warden();
    if (w && w->wardenPhase == 1 && level.anchorsAlive(LevelData::ChainAnchor::CONDUIT) == 0) {
        w->stagger(4.f);
        ui.toast("CUT OFF", "NOTHING FEEDING IT - FULL DAMAGE", {0.3f, 0.9f, 1.f}, 1.6f);
        conduitClock.allCut();
    }
}

inline void GameplayState::updateWarden(float dt) {
    Enemy* w = warden();
    if (w && w->uid != wardenUid) {   // it has arrived: every conduit whole, whatever happened before
        wardenUid = w->uid;
        for (int i = 0; i < (int)level.anchors.size(); ++i)
            if (level.anchors[i].kind == LevelData::ChainAnchor::CONDUIT) level.restoreAnchor(i);
        conduitClock.reset();
    }
    if (w) w->conduitsLeft = w->wardenPhase == 1 ? level.anchorsAlive(LevelData::ChainAnchor::CONDUIT) : 0;
    // Re-attaching, one at a time
    if (w && w->wardenPhase == 1 && conduitClock.running()) {
        int want = conduitClock.update(dt), have = level.anchorsAlive(LevelData::ChainAnchor::CONDUIT);
        for (int i = 0; i < (int)level.anchors.size() && have < want; ++i)
            if (level.anchors[i].kind == LevelData::ChainAnchor::CONDUIT && !level.anchors[i].alive) {
                level.restoreAnchor(i); ++have;
                fx.spawnBurst(level.anchors[i].pos, {0.3f, 0.9f, 1.f}, 24, 4.f, 1.f, 0.f);   // the arc crawling back
                audio.playAt("barrier", level.anchors[i].pos, 110, SoundGroup::ENEMY, true, 0.4f);
                ui.feed("A CONDUIT RECONNECTS", {0.3f, 0.9f, 1.f});
            }
        if (have >= 4) conduitClock.reset();
    }
    // The reactor's rings while it feeds; none once it's gone
    if (w && w->wardenPhase >= 2) {
        glm::vec3 c = level.hasReactor ? level.reactorPos : w->position;
        c.y = w->floorY;
        shifts.bossPulse(w->wardenPhase == 2 ? 3.5f : 5.f, c);
    } else if (shifts.bossDriven) shifts.bossPulseOff();
    // Parried orbs into its core
    for (auto& p : projSystem.pool) {
        if (!p.alive || p.homeOn < 0) continue;
        if (!w || w->uid != p.homeOn) { p.homeOn = -1; continue; }
        AABB cb; glm::vec3 target = coreBox(*w, cb) ? (cb.min + cb.max) * 0.5f : w->position + glm::vec3{0.f, w->height() * 0.62f, 0.f};
        p.velocity = steerParried(p.position, p.velocity, target, dt);
    }
    // Its hazards
    if (!w && ward.lances.empty() && ward.seekers.empty()) return;
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    glm::vec3 at = w ? w->position : glm::vec3{0.f};
    for (const auto& h : ward.update(dt, player.position, player.height, level.walls.data(), (int)level.walls.size(), at, w && w->ventTimer > 0.f))
        damagePlayer(h.damage * scale, h.from, 0.1f, 0.03f, h.damage < 10.f ? 0.f : 0.25f);
}

inline void GameplayState::gatherWardenBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float t = gameClock;
    const glm::vec3 cyan{0.3f, 1.2f, 1.4f}, red{1.6f, 0.25f, 0.2f}, white{1.7f, 1.6f, 1.5f};
    Enemy* w = warden();
    // Conduit nodes on the pillars, and their cables to its back
    for (const auto& a : level.anchors) {
        if (a.kind != LevelData::ChainAnchor::CONDUIT || !a.alive) continue;
        float pulse = 0.7f + 0.3f * std::sin(t * 4.f + a.pos.x);
        push(out, T(a.pos) * S(glm::vec3{1.1f}), {0.08f, 0.1f, 0.12f}, cyan * (0.9f * pulse));
        if (!w || w->wardenPhase != 1) continue;
        glm::vec3 from = w->position + glm::vec3{0.f, w->height() * 0.7f, 0.f} - glm::vec3{std::sin(w->yaw), 0.f, std::cos(w->yaw)} * 0.8f;
        glm::vec3 d = a.pos - from; float len = glm::length(d);
        int segs = std::max(1, (int)(len / 1.2f));
        float yaw = std::atan2(d.x, d.z), pitch = -std::asin(d.y / std::max(len, 1e-3f));
        for (int k = 0; k < segs; ++k) {
            glm::vec3 p = from + d * ((k + 0.5f) / segs) - glm::vec3{0.f, std::sin((k + 0.5f) / segs * 3.1416f) * len * 0.05f, 0.f};
            float flow = 0.5f + 0.5f * std::sin(t * 9.f - k * 0.8f);
            push(out, T(p) * RY(yaw) * RX(pitch) * S({0.12f, 0.12f, len / segs}), {0.05f, 0.06f, 0.07f}, cyan * (0.6f + 1.2f * flow));
        }
    }
    // The lance: a beam out to its reach
    for (const auto& l : ward.lances) {
        float yaw = l.yaw();
        glm::vec3 dir{std::sin(yaw), 0.f, std::cos(yaw)};
        glm::vec3 mid = l.from + dir * (WardenHazards::LANCE_REACH * 0.5f);
        push(out, T(mid) * RY(yaw) * S({0.35f, 0.35f, WardenHazards::LANCE_REACH}), red, white * 1.4f);
    }
    // Seekers: a red disc growing on the floor until it bursts
    for (const auto& s : ward.seekers) {
        float k = std::min(1.f, s.t / WardenHazards::SEEK_WARN);
        push(out, T(s.at + glm::vec3{0.f, 0.05f, 0.f}) * S({WardenHazards::SEEK_RADIUS * 2.f * k, 0.06f, WardenHazards::SEEK_RADIUS * 2.f * k}), red * 0.3f, red * (0.6f + 0.8f * k));
    }
}
