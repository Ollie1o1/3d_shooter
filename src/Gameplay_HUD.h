#pragma once
// =============================================================================
// Gameplay_HUD.h — GameplayState: The HUD: health, weapons, style, waypoints, banners, the objective line.
// Included at the end of GameplayState.h.
// =============================================================================

inline bool GameplayState::projectToScreen(glm::vec3 p, const glm::mat4& view, const glm::mat4& proj, float& sx, float& sy) const {
    glm::vec4 c = proj * view * glm::vec4(p, 1.f);
    bool behind = c.w <= 0.01f;
    glm::vec2 ndc = behind ? -glm::vec2(c) : glm::vec2(c) / c.w;
    bool on = !behind && std::fabs(ndc.x) < 0.95f && std::fabs(ndc.y) < 0.9f;
    if (!on) {
        float m = std::max(std::fabs(ndc.x) / 0.92f, std::fabs(ndc.y) / 0.85f);
        if (m < 1e-4f) { ndc = {0.f, -0.85f}; m = 1.f; }
        ndc /= m;
    }
    sx = (ndc.x * 0.5f + 0.5f) * SCREEN_W;
    sy = (1.f - (ndc.y * 0.5f + 0.5f)) * SCREEN_H;
    return on;
}

inline void GameplayState::renderHUD(const glm::mat4& view, const glm::mat4& proj) {
    // ---- the HUD proper ----
    HudState h;
    h.health = styleSystem.health; h.maxHealth = styleSystem.maxHealth;
    h.activeWeapon = activeWeapon;
    h.heldSource = weaponSource(heldWeapon());
    for (int w = 0; w < WEAPON_COUNT; ++w) {
        h.weapons[w].name = weaponDef((WeaponId)w).name;
        h.weapons[w].ammo = weapons[w].ammo;
        h.weapons[w].mag  = weaponMag((WeaponId)w, prog.up[w]);
        h.weapons[w].reloading = weapons[w].reloading;
        h.weapons[w].reload01  = weapons[w].reloadProgress();
    }
    h.grenades = grenadeCount; h.grenadeMax = grenadeMax;
    h.level = prog.level; h.xp = prog.xp; h.xpNext = Progression::xpToNext(prog.level); h.points = prog.points;
    const WeaponDef& wd = weaponDef((WeaponId)activeWeapon);
    h.aim = wd.canAim ? aim : 0.f;
    h.scoped = activeWeapon == (int)WeaponId::LONGSHOT && aim > 0.82f;   // lens fades in, then the model hides
    // Crosshair gap follows the spread (pixels at the current FOV)
    float spread = weaponSpread((WeaponId)activeWeapon, prog.up[activeWeapon], h.aim);
    if (wd.canAim && !player.onGround) spread += 0.03f * (1.f - h.aim);
    h.spread = spread * (SCREEN_H * 0.5f) / std::tan(glm::radians(player.camera.fov) * 0.5f) * 0.7f;
    h.hideCrosshair = (activeWeapon == (int)WeaponId::KAR && aim > 0.6f) || scopedView() || playerDead || victory;
    h.grappleTarget = grappleTargetInSight && !grapple.active;
    h.showTimer = fast() || (settings && settings->showTimer);
    h.speed = fast() ? playerXZSpeed : -1.f;
    h.time = elapsedTime;
    if (settings) settings->crosshairColor(h.crosshairColor.r, h.crosshairColor.g, h.crosshairColor.b);
    h.viewProj = proj * view;
    h.damageNumbers = !settings || settings->damageNumbers;
    if (!playerDead && !victory) ui.render(styleSystem, h);

    const Arena& ar = level.arenas[director.arena];
    int nArenas = (int)level.arenas.size();
    char buf[128];

    if (!playerDead && !victory) {
        int alive = 0;
        for (auto& e : enemies) if (e.alive) ++alive;
        int left = alive + director.queued();
        glm::vec3 accent{1.f, 0.75f, 0.3f};
        const Enemy* boss = nullptr;
        for (auto& e : enemies) if (e.alive && isBoss(e.type)) boss = &e;

        if (fast()) {
            if (finishOpen) { snprintf(buf, sizeof(buf), "FINISH OPEN - CLIMB TO THE BEACON"); accent = {1.f, 0.6f, 0.2f}; }
            else if (director.phase == WaveDirector::Phase::APPROACH) {
                snprintf(buf, sizeof(buf), "ROOM %d/%d  %s   ADVANCE", director.arena + 1, nArenas, ar.name);
                accent = {0.4f, 1.f, 0.6f};
            } else snprintf(buf, sizeof(buf), "ROOM %d/%d  %s   HOSTILES %d", director.arena + 1, nArenas, ar.name, left);
        } else switch (director.phase) {
        case WaveDirector::Phase::APPROACH:   // FAST only
        case WaveDirector::Phase::INTRO:
            snprintf(buf, sizeof(buf), "ARENA %d/%d  %s  GET READY", director.arena + 1, nArenas, ar.name); break;
        case WaveDirector::Phase::ACTIVE:
            if (boss) snprintf(buf, sizeof(buf), "ARENA %d/%d  FINAL WAVE", director.arena + 1, nArenas);
            else snprintf(buf, sizeof(buf), "ARENA %d/%d   WAVE %d/%d   HOSTILES %d",
                          director.arena + 1, nArenas, director.wave + 1, director.waveCount(), left);
            break;
        case WaveDirector::Phase::BREAK:
            snprintf(buf, sizeof(buf), "WAVE CLEAR - NEXT WAVE IN %d", (int)std::ceil(director.timer));
            accent = {0.4f, 1.f, 0.6f}; break;
        case WaveDirector::Phase::CLEARED:
            snprintf(buf, sizeof(buf), "ARENA CLEARED - GO THROUGH THE GATE");
            accent = {0.4f, 1.f, 0.6f}; break;
        case WaveDirector::Phase::VICTORY:
            snprintf(buf, sizeof(buf), "VICTORY"); accent = {0.4f, 1.f, 0.6f}; break;
        }
        ui.renderObjective(buf, accent);

        if (boss) ui.renderBossBar(boss->type == EnemyType::SOVEREIGN ? "THE SOVEREIGN" : "THE WARDEN",
                                   boss->health / boss->maxHealth, boss->enraged);

        // Waypoint to the way on: the open gate (ARENA), or the exit of the
        // room you just cleared (FAST) until you're out of it
        int wayDoor = -1;
        if (!fast() && director.phase == WaveDirector::Phase::CLEARED && ar.exitDoor >= 0 &&
            player.position.z > level.doors[ar.exitDoor].closed.min.z) wayDoor = ar.exitDoor;
        if (fast() && director.phase == WaveDirector::Phase::APPROACH && director.arena > 0) {
            const Arena& prev = level.arenas[director.arena - 1];
            const AABB& z = prev.zone;
            if (player.position.x > z.min.x && player.position.x < z.max.x &&
                player.position.z > z.min.z && player.position.z < z.max.z) wayDoor = prev.exitDoor;
        }
        if (wayDoor >= 0) {
            const Door& dr = level.doors[wayDoor];
            glm::vec3 target = (dr.closed.min + dr.closed.max) * 0.5f;
            target.y = dr.baseY + 2.f;
            float sx, sy;
            bool on = projectToScreen(target, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), fast() ? "EXIT %dM" : "GATE %dM", (int)glm::length(target - player.position));
            ui.renderMarker(sx, sy, on, {0.4f, 1.f, 0.6f}, buf);
        }
        if (finishOpen) {
            glm::vec3 target = level.finishPos + glm::vec3{0, 2.f, 0};
            float sx, sy;
            bool on = projectToScreen(target, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), "FINISH %dM", (int)glm::length(target - player.position));
            ui.renderMarker(sx, sy, on, {1.f, 0.6f, 0.2f}, buf);
        }
        // The last few enemies get markers so you never hunt for a straggler
        if (director.fighting() && director.queued() == 0 && alive > 0 && alive <= 3) {
            for (auto& e : enemies) {
                if (!e.alive) continue;
                float sx, sy;
                bool on = projectToScreen(e.position + glm::vec3{0, e.height() + 0.8f, 0}, view, proj, sx, sy);
                ui.renderMarker(sx, sy, on, {1.f, 0.3f, 0.25f}, "");
            }
        }
        if (!paused && !armoryOpen)
            ui.renderControlHint(glm::clamp(controlHintTimer / 1.5f, 0.f, 1.f),
                             GameSettings::grappleLabel(settings ? settings->grappleKey : 0));
    }

    if (!banners.empty() && !armoryOpen) {
        const Banner& b = banners.front();
        float a = std::min({1.f, b.time * 4.f, (b.duration - b.time) * 2.5f});
        ui.renderBanner(b.title.c_str(), b.subtitle.c_str(), b.color, a);
    }
    if (countdown > 0.f) ui.renderCountdown(countdown);
    if (g_practice && !victory && !paused) {
        ui.begin2D();
        ui.ui.textShadow("PRACTICE  F5 CLEAR WAVE  F6 HEAL", 12, SCREEN_H - 22, 1, {0.4f, 1.f, 0.85f, 0.8f});
        ui.end2D();
    }

    if (playerDead) {
        if (fast()) snprintf(buf, sizeof(buf), "ROOM %d/%d  %s", director.arena + 1, nArenas, ar.name);
        else snprintf(buf, sizeof(buf), "ARENA %d/%d %s - WAVE %d/%d", director.arena + 1, nArenas, ar.name,
                      director.wave + 1, director.waveCount());
        ui.renderDeath(buf, totalKills, elapsedTime, fast());
    }
    if (victory) {
        if (fast()) {
            const char* rank = elapsedTime < level.parTimes[0] ? "S" : elapsedTime < level.parTimes[1] ? "A"
                             : elapsedTime < level.parTimes[2] ? "B" : elapsedTime < level.parTimes[3] ? "C" : "D";
            // Show the best run as it was before this one (newRecord already replaced it)
            static std::vector<float> none;
            ui.renderVictoryFast(elapsedTime, newRecord ? 0.f : records.bestFast, newRecord, rank, totalKills,
                                 totalShots, totalHits, deaths, splits, newRecord ? none : records.fastSplits, !nameEntry);
        } else {
            ui.renderVictoryArena(totalKills, totalShots, totalHits, deaths, elapsedTime, peakStyle,
                                  prog.level, records.bestArena, newRecord, !nameEntry);
        }
        renderLeaderboardPanel();
    }
    if (armoryOpen) ui.renderArmory(prog, armoryW, armoryS);
    if (paused) {
        if (pauseSettings) {
            ui.begin2D();
            ui.ui.rect(0, 0, SCREEN_W, SCREEN_H, {0.f, 0.f, 0.02f, 0.65f});
            settingsMenu.render(ui.ui, gameClock + (float)SDL_GetTicks() * 0.001f);
            ui.end2D();
        } else {
            const char* labels[UIRenderer::PAUSE_ITEMS];
            for (int i = 0; i < UIRenderer::PAUSE_ITEMS; ++i) labels[i] = pauseLabel(i);
            std::string line = fast() ? "FAST - THE GAUNTLET  " + formatTime(elapsedTime)
                                      : std::string("ARENA - ") + ar.name;
            line += std::string("   ") + tune().name;
            ui.renderPause(pauseSelected, labels, line.c_str());
        }
    }
}
