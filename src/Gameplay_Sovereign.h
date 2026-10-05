#pragma once
// =============================================================================
// Gameplay_Sovereign.h — GameplayState: the Sovereign's reach beyond his
// sword (SovereignHazards.h): the blades he calls down, the eruptions he
// drives through the floor, his phantoms, his shadow step, and his last
// stand, when the Sanctum's edges and high ground start to burn. Their
// damage, sounds and sparks, and drawing them.
// Included at the end of GameplayState.h.
// =============================================================================

inline glm::vec3 GameplayState::sanctumCentre() const {
    const AABB& z = level.arenas.back().zone;
    return {(z.min.x + z.max.x) * 0.5f, 0.f, (z.min.z + z.max.z) * 0.5f};
}

inline void GameplayState::onSovereignEvents(const Enemy& e, const EnemyEvents& ev) {
    for (int k = 0; k < ev.strikes; ++k) {
        glm::vec3 p = ev.strikePos[k];
        p.y = groundHeightAt(p.x, p.z, p.y + 1.f);   // on whatever's under the mark
        sov.addStrike(p, ev.strikeKind[k], ev.strikeDelay[k]);
    }
    if (ev.strikes > 0) audio.play("telegraph", 110);
    if (ev.blinked) {
        glm::vec3 smoke{0.45f, 0.04f, 0.08f};
        fx.spawnBurst(ev.blinkFrom + glm::vec3{0, 1.6f, 0}, smoke, 40, 7.f, 0.7f, -3.f);
        fx.spawnBurst(e.position + glm::vec3{0, 1.6f, 0}, smoke, 40, 7.f, 0.7f, -3.f);
        fx.spawnShockwave(e.position, 3.f, {1.f, 0.2f, 0.15f});
        audio.play("dash", 128);
        shake(0.15f, 0.04f);
    }
    for (int k = 0; k < ev.phantoms; ++k) sov.addPhantom(ev.phantomPos[k], ev.phantomDir[k]);
    if (ev.phantoms > 0) audio.play("telegraph", 120);
}

inline void GameplayState::updateSovereign(float dt) {
    const Arena& ar = level.arenas[director.arena];
    const float dmgScale = ar.damageScale * tune().damage;
    for (const auto& h : sov.update(dt, player.position)) {
        if (!damagePlayer(h.damage * dmgScale, h.from, 0.3f, 0.07f)) continue;
        if (h.kind == 1) player.velocity.y = std::max(player.velocity.y, 7.f);   // thrown up by the eruption
        else {
            glm::vec3 away = player.position - h.from; away.y = 0.f;
            if (glm::length(away) > 0.01f) player.velocity += glm::normalize(away) * 7.f + glm::vec3{0, 3.f, 0};
        }
    }
    // --overlay strikes (screenshots): a ring of blades and a line of
    // eruptions in front of the camera, over and over
    if (g_devOverlay == "strikes" && sov.strikes.empty()) {
        glm::vec3 f = player.camera.forward(); f.y = 0.f; f = glm::normalize(f);
        glm::vec3 c = player.position + f * 9.f;
        for (int i = 0; i < 7; ++i)
            sov.addStrike(i == 0 ? c : c + glm::vec3{std::cos(i * 1.047f) * 4.4f, 0.f, std::sin(i * 1.047f) * 4.4f}, 0, 1.2f + 0.05f * i);
        for (int i = 0; i < 8; ++i) sov.addStrike(player.position + glm::vec3{-f.z, 0.f, f.x} * 4.f + f * (3.f + 2.2f * i), 1, 0.5f + 0.07f * i);
    }
    bool bladeSound = false, eruptSound = false;
    for (const auto& s : sov.takeLanded()) {
        if (s.kind == 0) {
            fx.spawnShockwave(s.pos, SovereignHazards::BLADE_RADIUS + 0.6f, {1.f, 0.25f, 0.15f});
            fx.spawnBurst(s.pos + glm::vec3{0, 0.3f, 0}, {1.f, 0.45f, 0.25f}, 14, 7.f, 0.5f, 12.f);
            if (!bladeSound) { audio.play("slam", 90); bladeSound = true; }
        } else {
            fx.spawnBurst(s.pos + glm::vec3{0, 0.2f, 0}, {1.f, 0.4f, 0.12f}, 10, 9.f, 0.6f, 14.f);
            if (!eruptSound) { audio.play("slam", 60); eruptSound = true; }
        }
    }

    // The last stand: under a fifth of his health, the edges and the high
    // ground burn - the end is a duel in the middle
    Enemy* boss = nullptr;
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::SOVEREIGN) boss = &e;
    if (!boss) { lastStand = false; return; }
    if (g_devOverlay == "laststand" && boss->targetable() && !lastStand)   // screenshots: straight to it
        boss->health = std::min(boss->health, boss->maxHealth * 0.19f);
    if (!lastStand && boss->health < boss->maxHealth * 0.2f) {
        lastStand = true; lastStandT = 0.f;
        pushBanner("THE SANCTUM BURNS", "GET TO THE MIDDLE - FINISH IT", {1.f, 0.3f, 0.15f}, 3.f);
        audio.play("wave");
        shake(0.6f, 0.07f);
    }
    if (!lastStand) return;
    lastStandT += dt;
    glm::vec3 c = sanctumCentre();
    // Embers rising off the burning band
    for (int k = 0; k < 3; ++k) {
        float x = frand(ar.bounds.min.x, ar.bounds.max.x), z = frand(ar.bounds.min.z, ar.bounds.max.z);
        if (!SovereignHazards::burns({x, 0.f, z}, c)) continue;
        fx.spawnBurst({x, 0.2f, z}, {1.f, 0.35f, 0.1f}, 1, 2.f, 1.4f, -3.f);
    }
    if (lastStandT < LAST_STAND_WARN || playerDead || g_godMode) return;
    if (!SovereignHazards::burns(player.position, c)) return;
    float burn = std::min(SovereignHazards::BURN_DPS * dt, styleSystem.health);
    styleSystem.health -= burn;
    styleSystem.damageTaken += burn;
    if (hazardTick <= 0.f) {
        hazardTick = 0.35f;
        ui.onDamage();
        audio.play("player_hit", 60);
        fx.spawnBurst(player.position + glm::vec3{0, 0.2f, 0}, {1.f, 0.45f, 0.1f}, 8, 3.f, 0.5f, -3.f);
    }
}

inline void GameplayState::gatherSovereignBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    float t = gameClock;
    const glm::vec3 crimson{1.6f, 0.3f, 0.18f};

    // Blades from the sky: a ring on the ground where each will land, the
    // blade hanging high above it, then dropping, then stuck in the floor
    for (const auto& s : sov.strikes) {
        float u = s.landed ? 1.f : 1.f - s.delay / s.total;
        if (s.kind == 0) {
            float fade = s.landed ? 1.f - s.after / SovereignHazards::LINGER : 1.f;
            const int SEG = 18;
            float r = SovereignHazards::BLADE_RADIUS, segLen = 6.2832f * r / SEG;
            float pulse = 0.6f + 0.4f * std::sin(t * 18.f);
            for (int k = 0; k < SEG; ++k) {
                float a = (k + 0.5f) / SEG * 6.2832f + t * 0.8f;
                push(out, T(s.pos + glm::vec3{std::cos(a) * r, 0.05f, std::sin(a) * r}) * RY(-a) * S({0.14f, 0.05f, segLen * 0.7f}),
                     crimson * 0.2f, crimson * (0.5f + 1.5f * u * pulse) * fade);
            }
            float fall = glm::clamp((u - 0.55f) / 0.45f, 0.f, 1.f);
            float h = s.landed ? 0.9f : 1.6f + 22.f * (1.f - fall * fall);
            glm::mat4 b = T(s.pos + glm::vec3{0, h, 0}) * RY(s.pos.x * 0.7f + s.pos.z);
            glm::vec3 glow = glm::vec3{1.3f, 0.45f, 0.3f} * (0.6f + 0.8f * u) * fade;
            push(out, b * T({0, 0.f, 0}) * S({0.12f, 3.2f, 0.42f}), {0.05f, 0.03f, 0.04f}, glow);          // blade
            push(out, b * T({0, 1.7f, 0}) * S({1.2f, 0.16f, 0.24f}), {0.08f, 0.05f, 0.05f}, glow * 1.3f);   // guard
            push(out, b * T({0, 2.15f, 0}) * S({0.14f, 0.8f, 0.14f}), {0.06f, 0.04f, 0.04f}, glow * 0.6f); // grip
            continue;
        }
        // An eruption: the crack glows hotter, then a column of fire
        if (!s.landed) {
            push(out, T(s.pos + glm::vec3{0, 0.04f, 0}) * RY(s.pos.x + s.pos.z) * S({1.5f * u + 0.3f, 0.05f, 0.25f}),
                 {0.2f, 0.05f, 0.03f}, crimson * (0.4f + 1.4f * u));
        } else {
            float a = s.after / SovereignHazards::LINGER;
            float h = 3.4f * (1.f - a * a);
            push(out, T(s.pos + glm::vec3{0, h * 0.5f, 0}) * RY(t * 4.f + s.pos.x) * S({1.1f * (1.f - a) + 0.2f, h, 1.1f * (1.f - a) + 0.2f}),
                 {0.4f, 0.1f, 0.03f}, glm::vec3{2.f, 0.6f, 0.15f} * (1.f - a));
        }
    }

    // Phantoms: a red copy of him - flickering where he'll come from, then
    // dashing through, with a trail
    const Enemy* boss = nullptr;
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::SOVEREIGN) boss = &e;
    if (boss) {
        for (const auto& p : sov.phantoms) {
            Enemy ph = *boss;
            ph.position = SovereignHazards::phantomAt(p);
            ph.yaw = std::atan2(p.dir.x, p.dir.z);
            ph.hitFlashTimer = 0.f; ph.swingTimer = 0.f; ph.leapTimer = 0.f; ph.whirlTimer = 0.f; ph.thrustTimer = 0.f;
            bool waiting = p.t < SovereignHazards::PHANTOM_WAIT;
            if (waiting) { ph.dashTimer = 0.f; ph.attack = AttackKind::DASH; ph.telegraphDuration = SovereignHazards::PHANTOM_WAIT;
                           ph.telegraphTimer = SovereignHazards::PHANTOM_WAIT - p.t; }
            else         { ph.dashTimer = 0.3f; ph.diveDir = p.dir; ph.telegraphTimer = 0.f; }
            float glow = waiting ? 0.5f + 0.5f * std::sin(t * 30.f) : 1.f;
            for (int k = 0; k <= (waiting ? 0 : 3); ++k) {
                Enemy g = ph;
                g.position -= p.dir * (1.7f * k);
                size_t from = out.size();
                buildEnemy(g, t, out);
                for (size_t j = from; j < out.size(); ++j) {
                    out[j].color = glm::vec3{0.04f, 0.01f, 0.02f};
                    out[j].emissive = glm::vec3{1.5f, 0.2f, 0.18f} * glow / (1.f + k);
                }
            }
        }
        // Shadow step wind-up: his mark under your feet
        if (boss->attack == AttackKind::BLINK && boss->telegraphTimer > 0.f) {
            float k = boss->telegraphProgress();
            const int SEG = 16;
            float r = 1.8f - 0.6f * k;
            for (int i = 0; i < SEG; ++i) {
                float a = (i + 0.5f) / SEG * 6.2832f - t * 4.f;
                push(out, T(player.position + glm::vec3{std::cos(a) * r, 0.06f, std::sin(a) * r}) * RY(-a) * S({0.12f, 0.05f, 0.45f}),
                     crimson * 0.2f, crimson * (1.f + 2.f * k));
            }
        }
    }

    // The last stand: the band outside the middle glows, then burns, and the
    // islands and corner tops glow with it
    if (!lastStand || !boss) return;
    const Arena& ar = level.arenas.back();
    glm::vec3 c = sanctumCentre();
    float H = SovereignHazards::SAFE_HALF;
    float warn = lastStandT < LAST_STAND_WARN;
    float pulse = warn ? 0.5f + 0.5f * std::sin(t * 14.f) : 0.85f + 0.15f * std::sin(t * 2.5f);
    glm::vec3 hot = glm::vec3{0.42f, 0.06f, 0.02f} * pulse * (warn ? 0.6f : 1.f);
    auto slab = [&](float x0, float z0, float x1, float z1, float y) {
        push(out, T({(x0 + x1) * 0.5f, y, (z0 + z1) * 0.5f}) * S({x1 - x0, 0.06f, z1 - z0}), {0.14f, 0.02f, 0.01f}, hot);
    };
    // A bright line where the safe ground ends
    glm::vec3 edge = glm::vec3{1.8f, 0.4f, 0.1f} * pulse;
    for (int s = -1; s <= 1; s += 2) {
        push(out, T({c.x, 0.08f, c.z + s * H}) * S({2.f * H, 0.1f, 0.25f}), {0.4f, 0.1f, 0.02f}, edge);
        push(out, T({c.x + s * H, 0.08f, c.z}) * S({0.25f, 0.1f, 2.f * H}), {0.4f, 0.1f, 0.02f}, edge);
    }
    const AABB& b = ar.bounds;
    slab(b.min.x, b.min.z, b.max.x, c.z - H, 0.04f);
    slab(b.min.x, c.z + H, b.max.x, b.max.z, 0.04f);
    slab(b.min.x, c.z - H, c.x - H, c.z + H, 0.04f);
    slab(c.x + H, c.z - H, b.max.x, c.z + H, 0.04f);
    for (int k = 0; k < 4; ++k) {   // the islands (11 m) and the corner tops (5 m)
        float dx = k == 2 ? 1.f : k == 3 ? -1.f : 0.f, dz = k == 0 ? -1.f : k == 1 ? 1.f : 0.f;
        slab(c.x + dx * 24.f - 3.f, c.z + dz * 24.f - 3.f, c.x + dx * 24.f + 3.f, c.z + dz * 24.f + 3.f, 11.04f);
        float sx = k % 2 ? 1.f : -1.f, sz = k / 2 ? 1.f : -1.f;
        slab(c.x + std::min(sx * 34.f, sx * 44.f), c.z + std::min(sz * 34.f, sz * 44.f),
             c.x + std::max(sx * 34.f, sx * 44.f), c.z + std::max(sz * 34.f, sz * 44.f), 5.04f);
    }
}
