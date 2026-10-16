#pragma once
// =============================================================================
// Gameplay_Morning.h — GameplayState: MORNINGSTAR, the Absolute's second form.
// The swallow (the eclipse taking the Zenith), his moves turned into hazards
// (AbsoluteHazards.h), the swallowed sky's themes, and what's drawn on the
// floor: the ring of fire round the edge, the seams of light in the black
// glass, and the floor tells of his gold and red moves (on his body only white
// shows). Included at the end of Gameplay_Absolute.h.
// =============================================================================

// Inside the eclipse: black overhead, the ring of fire low all round, the floor lit from its edge
inline Theme GameplayState::swallowedOf(const Theme& t) {
    Theme o = t;
    o.zenith = {0.f, 0.f, 0.f}; o.horizon = {0.06f, 0.012f, 0.005f}; o.ground = {0.f, 0.f, 0.f};
    o.sunColor = {1.9f, 0.7f, 0.3f}; o.mountain = {0.f, 0.f, 0.f}; o.stars = 0.f;
    o.lightDir = glm::normalize(glm::vec3{0.35f, -0.25f, 0.5f}); o.lightColor = {0.95f, 0.5f, 0.28f};
    o.skyAmb = {0.05f, 0.03f, 0.03f}; o.groundAmb = {0.16f, 0.06f, 0.03f};
    o.fogColor = {0.03f, 0.008f, 0.004f}; o.fogDensity = t.fogDensity * 0.6f;
    return o;
}
// TOTALITY: the light goes out of everything but him
inline Theme GameplayState::totalityOf(const Theme& t) {
    Theme o = t;
    o.horizon *= 0.25f; o.sunColor *= 0.35f;
    o.lightColor *= 0.22f; o.skyAmb *= 0.3f; o.groundAmb *= 0.3f; o.fogColor *= 0.2f;
    return o;
}

// His events (the swallow, his form, his moves landing) into hazards, sound and the sky
inline void GameplayState::onMorningEvents(Enemy& e, const EnemyEvents& ev) {
    const glm::vec3 C = level.zenith.centre;
    const glm::vec3 fire{2.2f, 0.7f, 0.2f}, white{1.8f, 1.75f, 1.7f};
    abh.morningEvents(ev, C, player.position, e.yaw);   // the hazards (AbsoluteHazards.h); the rest here is how it looks and sounds
    if (ev.msSwallow) {   // he kneels; the eclipse comes down over everything
        audio.stop(abWindH); audio.stop(abFlightH); audio.stop(abBeamH); abWindH = abFlightH = abBeamH = 0;
        abSkyMarkT = -1.f;
        msSwallowT = 0.f; eclipseFlare = 1.f;
        abLineShown = AB_LINE_NOT_YET; abLineT = 2.6f;
        audio.playAt("v_absolute_line", e.position, 128, SoundGroup::ENEMY, true, 0.7f);
        audio.play("ab_rumble", 128, SoundGroup::WORLD, true);
        audio.duck(8.f, 3.f);
        fx.spawnShockwave(e.position, 16.f, white);
        shake(0.6f, 0.05f);
    }
    if (ev.msForm) {   // out of the black: MORNINGSTAR (from here a death retries from him)
        msCheckpoint = true;
        msSwallowT = std::max(msSwallowT, Enemy::MS_SWALLOW);
        eclipseFlare = 1.f;
        pushBanner("MORNINGSTAR", "WHITE: PARRY HIM   THE FLOOR SHOWS THE REST", {1.f, 0.55f, 0.25f}, 4.5f);
        audio.play("wave", 128, SoundGroup::UI); audio.play("explosion", 80, SoundGroup::UI);
    }
    if (ev.abPhase == 5) { pushBanner("RISING", "EVERY SLAM COMES BACK", {1.f, 0.6f, 0.3f}, 3.f); shake(0.5f, 0.06f); audio.duck(6.f, 0.6f); }
    if (ev.abPhase == 6) { pushBanner("TOTALITY", "JUMP THE RING", {1.f, 0.95f, 0.9f}, 3.f); shake(0.6f, 0.06f); audio.duck(8.f, 0.8f); }
    auto quake = [&](glm::vec3 at, float r, float k) {
        fx.spawnShockwave(at + glm::vec3{0.f, 0.1f, 0.f}, r, fire);
        fx.spawnBurst(at + glm::vec3{0.f, 0.4f, 0.f}, glm::vec3{2.f, 0.9f, 0.4f}, 40, 10.f, 0.6f, 6.f);
        float d = glm::length(player.position - at);
        shake(0.25f + 0.35f * k * glm::clamp(1.f - d / 40.f, 0.f, 1.f), 0.06f + 0.04f * k);
    };
    if (ev.msSunder) quake(ev.abFrom, 6.f, 0.8f);
    if (ev.msFissure) quake(ev.abFrom, 3.f, 0.5f);
    if (ev.msDrop) {   // the crater (RISING and his entrance: a ring after it)
        quake(ev.abTo, Enemy::MS_DROP_R + 2.f, 1.f);
        fx.spawnBurst(ev.abTo + glm::vec3{0.f, 0.3f, 0.f}, {1.4f, 0.5f, 0.15f}, 40, 13.f, 0.8f, 12.f);   // embers thrown up
        eclipseFlare = std::max(eclipseFlare, 0.6f);
    }
    if (ev.msGrasp) quake(ev.abTo, 4.f, 0.6f);
    if (ev.msFlares) {
        eclipseFlare = std::max(eclipseFlare, 0.5f);
        audio.play("ab_rumble", 110, SoundGroup::WORLD, true);
    }
    if (ev.msCollapse) { msTotalityT = 0.f; eclipseFlare = 1.f; shake(0.8f, 0.05f); }
}

// ---- drawing ----
inline void GameplayState::gatherMorningBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float sw = swallow01() * (1.f - dawn01());
    const float t = gameClock;
    const glm::vec3 C = level.zenith.centre;
    const float lights = 1.f - 0.75f * totality01();
    const glm::vec3 fire{1.5f, 0.42f, 0.1f}, hot{2.1f, 1.6f, 1.1f}, gold{1.5f, 1.2f, 0.55f}, red{1.6f, 0.2f, 0.1f};
    auto along = [&](glm::vec3 a, glm::vec3 b, float w, float h, glm::vec3 col, glm::vec3 emi) {
        glm::vec3 d = b - a; float l = glm::length(d);
        if (l < 1e-3f) return;
        d /= l;
        push(out, T((a + b) * 0.5f) * RY(std::atan2(d.x, d.z)) * S({w, h, l}), col, emi);
    };
    auto ring = [&](glm::vec3 at, float R, float w, float h, int n, glm::vec3 col, glm::vec3 emi) {
        for (int k = 0; k < n; ++k) {
            float a = k * 6.2831853f / n;
            push(out, T(at + glm::vec3{std::cos(a) * R, h * 0.5f, std::sin(a) * R}) * RY(-a) * S({w, h, R * 6.4f / n}), col, emi);
        }
    };
    // the ring of fire round the floor's edge: tongues of flame, a white-hot base
    if (sw > 0.3f) {
        const float k = glm::smoothstep(0.3f, 1.f, sw) * lights;
        for (int j = 0; j < 120; ++j) {
            float a = j * 6.2831853f / 120.f, fl = 0.6f + 0.4f * std::sin(t * 7.f + j * 2.7f) * std::sin(t * 3.1f + j * 0.9f);
            float h = (2.f + 3.5f * fl) * k;
            glm::vec3 p = C + glm::vec3{std::cos(a) * 45.f, h * 0.5f, std::sin(a) * 45.f};
            push(out, T(p) * RY(-a) * S({0.5f, h, 2.5f}), fire * 0.2f, fire * (0.6f + 0.6f * fl) * k);
        }
        ring(C, 45.f, 0.8f, 0.5f * k, 90, glm::vec3{1.f}, hot * k);
        // seams of light in the black glass: two rings and twelve spokes, faint, brighter near the edge
        for (float R : {16.f, 30.f}) ring(C + glm::vec3{0.f, 0.03f, 0.f}, R, 0.08f, 0.02f, 96, fire * 0.1f, fire * 0.45f * k);
        for (int j = 0; j < 12; ++j) {
            float a = j * 0.5235988f + 0.26f;
            glm::vec3 dir{std::cos(a), 0.f, std::sin(a)};
            along(C + dir * (Enemy::AB_DAIS_R + 0.5f) + glm::vec3{0.f, 0.03f, 0.f}, C + dir * 44.f + glm::vec3{0.f, 0.03f, 0.f}, 0.08f, 0.02f, fire * 0.1f, fire * 0.5f * k);
        }
    }
    // his floor tells (his body only shows white)
    if (Enemy* e = absolute(); e && e->morning() && e->telegraphTimer > 0.f) {
        const float u = e->telegraphProgress();
        const glm::vec3 floorAt{e->position.x, e->position.y - e->abAlt, e->position.z};
        switch (e->attack) {
        case AttackKind::MSUNDER: {   // where his fists will land, a ring opening out from it
            glm::vec3 f{std::sin(e->yaw), 0.f, std::cos(e->yaw)};
            ring(floorAt + f * (1.4f * e->scale) + glm::vec3{0.f, 0.04f, 0.f}, 1.f + 4.f * u, 0.15f, 0.05f, 40, red * 0.2f, red * (0.5f + 1.2f * u));
            break;
        }
        case AttackKind::MFISSURE: {   // the line it will run along, brightening
            glm::vec3 d = e->abAim - floorAt; d.y = 0.f;
            if (glm::length(d) < 0.1f) d = {std::sin(e->yaw), 0.f, std::cos(e->yaw)};
            d = glm::normalize(d);
            along(floorAt + glm::vec3{0.f, 0.04f, 0.f}, floorAt + d * 44.f + glm::vec3{0.f, 0.04f, 0.f}, 0.3f + 1.6f * u, 0.03f, red * 0.2f, red * (0.3f + 0.9f * u * u));
            break;
        }
        case AttackKind::MGRASP: {    // where the hand comes down: a gold circle closing in
            float R = Enemy::MS_GRASP_R * (1.3f - 0.3f * u);
            ring(e->abAim + glm::vec3{0.f, 0.05f, 0.f}, R, 0.16f, 0.06f, 40, gold * 0.3f, gold * (1.2f + 1.6f * u));
            push(out, T(e->abAim + glm::vec3{0.f, 0.03f, 0.f}) * S({R * 1.8f, 0.02f, R * 1.8f}), gold * 0.03f, gold * (0.04f + 0.22f * u * u));
            break;
        }
        case AttackKind::MDROP: {     // his shadow: dark, a gold rim, growing as he falls
            float R = Enemy::MS_DROP_R * (0.7f + 0.3f * u);
            push(out, T(e->abAim + glm::vec3{0.f, 0.03f, 0.f}) * S({R * 1.7f, 0.02f, R * 1.7f}), glm::vec3{0.01f}, glm::vec3{0.f});
            ring(e->abAim + glm::vec3{0.f, 0.06f, 0.f}, R, 0.18f, 0.06f, 48, gold * 0.3f, gold * (1.f + 2.f * u));
            break;
        }
        case AttackKind::MCOLLAPSE:   // the ring of fire gathering itself red
            ring(C, 44.f, 0.6f, 4.f * u, 90, red * 0.2f, red * (0.6f + 1.4f * u));
            break;
        default: break;
        }
    }
    // the hazards
    for (auto& w : abh.waves) {
        if (w.delay > 0.f) {   // an aftershock to come: where it will rise, pulsing
            ring(w.at + glm::vec3{0.f, 0.04f, 0.f}, 1.5f, 0.12f, 0.04f, 24, red * 0.2f, red * (0.6f + 0.5f * std::sin(t * 30.f)));
            continue;
        }
        // a wave of fire: a low white-hot core and a band of orange over it (the COLLAPSE: a red wall, taller).
        // (Never dimmed: in TOTALITY the floor's tells are what you have)
        const float R = std::max(w.r, 0.2f);
        const int n = w.inward ? 120 : std::clamp((int)(R * 3.f), 24, 96);
        ring(w.at, R, 0.3f, w.inward ? 3.f : 1.f, n, glm::vec3{0.05f, 0.01f, 0.f}, w.inward ? red * 1.1f : glm::vec3{1.6f, 0.45f, 0.08f});
        ring(w.at, R, 0.4f, 0.25f, n, glm::vec3{0.1f, 0.05f, 0.02f}, glm::vec3{2.2f, 1.4f, 0.7f});
    }
    for (auto& f : abh.fissures) {
        float tip = std::min(f.len, f.t * AbsoluteHazards::FIS_SPEED);
        float burn = 1.f - glm::clamp((f.t - tip / AbsoluteHazards::FIS_SPEED) / AbsoluteHazards::FIS_BURN, 0.f, 1.f);
        along(f.from + glm::vec3{0.f, 0.05f, 0.f}, f.from + f.dir * tip + glm::vec3{0.f, 0.05f, 0.f}, 0.5f, 0.06f, red * 0.2f, hot * (0.4f + burn));
        if (burn > 0.f) along(f.from + glm::vec3{0.f, 0.6f, 0.f}, f.from + f.dir * tip + glm::vec3{0.f, 0.6f, 0.f}, 1.6f, 1.2f * burn, fire * 0.2f, fire * burn);
        float ta = f.len / AbsoluteHazards::FIS_SPEED + AbsoluteHazards::FIS_AFTER;
        if (f.after) {
            if (f.t < ta) along(f.from + glm::vec3{0.f, 0.07f, 0.f}, f.from + f.dir * f.len + glm::vec3{0.f, 0.07f, 0.f}, 0.3f, 0.04f, red * 0.2f, red * (0.4f + 0.4f * std::sin(t * 30.f)));
            else for (int j = 0; j < 14; ++j) {   // the aftershock: pillars of light all along it
                glm::vec3 p = f.from + f.dir * (f.len * (j + 0.5f) / 14.f);
                float k = 1.f - glm::clamp((f.t - ta) / AbsoluteHazards::FIS_AFTER_LEN, 0.f, 1.f);
                push(out, T(p + glm::vec3{0.f, 1.5f * k, 0.f}) * S({1.4f, 3.f * k, 1.4f}), fire * 0.2f, hot * k);
            }
        }
    }
    for (auto& L : abh.lanes) {
        if (L.t < 0.f) continue;
        const float front = AbsoluteHazards::laneFront(L);
        const float warn = glm::clamp(L.t / AbsoluteHazards::LANE_WARN, 0.f, 1.f);
        for (int k = 0; k < AbsoluteHazards::LANE_N; ++k) {
            float off = L.off[k];
            if (std::fabs(off) > 43.f) continue;
            float half = std::sqrt(43.f * 43.f - off * off);   // the lane inside the floor
            glm::vec3 c0 = C + L.side * off;
            glm::vec3 a = c0 - L.dir * half, b = c0 + L.dir * half;
            if (L.t < AbsoluteHazards::LANE_WARN)   // lit, brightening
                along(a + glm::vec3{0.f, 0.04f, 0.f}, b + glm::vec3{0.f, 0.04f, 0.f}, AbsoluteHazards::LANE_HALF * 2.f, 0.02f, red * 0.15f, red * (0.2f + 0.8f * warn * warn) * lights);
            else if (front > -half && front < half + 2.f) {   // the wall of fire running down it, a scorched trail behind
                glm::vec3 p = c0 + L.dir * std::min(front, half);
                push(out, T(p + glm::vec3{0.f, AbsoluteHazards::LANE_H * 0.5f, 0.f}) * RY(std::atan2(L.dir.x, L.dir.z)) *
                          S({AbsoluteHazards::LANE_HALF * 2.f, AbsoluteHazards::LANE_H * (0.85f + 0.15f * std::sin(t * 23.f + k)), 1.2f}), fire * 0.2f, fire * 1.3f + hot * 0.3f);
                along(a + glm::vec3{0.f, 0.05f, 0.f}, p + glm::vec3{0.f, 0.05f, 0.f}, AbsoluteHazards::LANE_HALF * 1.6f, 0.03f, fire * 0.1f, fire * 0.5f);
            }
        }
    }
}
