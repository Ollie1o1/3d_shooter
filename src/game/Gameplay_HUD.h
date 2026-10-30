#pragma once
// =============================================================================
// Gameplay_HUD.h — GameplayState: The HUD: health, weapons, style, waypoints, banners, the objective line.
// Included at the end of GameplayState.h.
// =============================================================================

inline bool GameplayState::projectToScreen(glm::vec3 p, const glm::mat4& view, const glm::mat4& proj, float& sx, float& sy) const {
    const auto marker=projectWorldMarker(p,proj*view,SCREEN_W,SCREEN_H);
    sx=marker.x;sy=marker.y;return marker.onScreen;
}

inline void GameplayState::renderHUD(const glm::mat4& view, const glm::mat4& proj) {
    if (story() && (storyLocked() || storyEndT >= 0.f)) { renderStoryOverlay(proj * view); return; }   // the wake-up, the end: no HUD
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
        h.weapons[w].glow      = gunkit::glowOf(w);
    }
    h.grenades = grenadeCount; h.grenadeMax = grenadeMax;
    h.swordSlot = swordAllowed(); h.swordOut = swordOut; h.swordBroken = sword.broken(); h.swordGuard = sword.guard01();
    h.swordBar = swordOut && (sword.blocking() || sword.broken() || sword.riposteT > 0.f || sword.guard < Sword::GUARD_MAX);
    h.level = prog.level; h.xp = prog.xp; h.xpNext = Progression::xpToNext(prog.level); h.points = prog.points;
    const WeaponDef& wd = weaponDef((WeaponId)activeWeapon);
    h.aim = wd.canAim ? aim : 0.f;
    h.scoped = activeWeapon == (int)WeaponId::LONGSHOT && aim > 0.82f;   // lens fades in, then the model hides
    // Crosshair gap follows the spread (pixels at the current FOV)
    float spread = weaponSpread((WeaponId)activeWeapon, prog.up[activeWeapon], h.aim);
    if (wd.canAim && !player.onGround) spread += 0.03f * (1.f - h.aim);
    h.spread = spread * (SCREEN_H * 0.5f) / std::tan(glm::radians(player.camera.fov) * 0.5f) * 0.7f;
    h.hideCrosshair = (activeWeapon == (int)WeaponId::KAR && aim > 0.6f) || scopedView() || playerDead || victory;
    h.grappleTarget = grappleTargetInSight && !grapple.active && grappleCooldown <= 0.f;
    h.grappleCool = std::max(0.f, grappleCooldown) / GRAPPLE_COOLDOWN;
    h.showTimer = fast() || (settings && settings->showTimer);
    h.speed = fast() ? playerXZSpeed : -1.f;
    h.time = elapsedTime;
    if (settings) settings->crosshairColor(h.crosshairColor.r, h.crosshairColor.g, h.crosshairColor.b);
    h.viewProj = proj * view;
    h.damageNumbers = !settings || settings->damageNumbers;
    h.finishHint = finishHint;
    {   // attacks winding up out of sight (Gameplay_Feel.h): which way, how soon
        h.threatCount = 0;
        glm::vec3 fwd = player.camera.flatForward();
        for (const auto& e : enemies) {
            if (h.threatCount >= 6) break;
            if (!e.targetable() || isBoss(e.type) || !unseenWindup(e, fwd)) continue;
            glm::vec3 to = e.position - player.camera.position;
            float ang = std::atan2(to.x, to.z) - glm::radians(player.camera.yaw + 90.f);
            h.threats[h.threatCount++] = {ang, e.telegraphProgress()};
        }
    }
    h.finishKey  = gamepad::state().pad ? "R3" : binds().label(Act::INTERACT);
    if (story() && !storyArmed) { h.noWeapons = true; h.hideCrosshair = true; }
    if (story()) { for (int w = 0; w < WEAPON_COUNT; ++w) h.weapons[w].owned = storyOwns(w); h.grenades = h.grenadeMax = 0; }
    if (!playerDead && !victory) ui.render(styleSystem, h);
    if (story()) renderStoryOverlay(proj * view);   // a gun's waypoint and prompt, a wreck's log

    const Arena& ar = level.arenas[director.arena];
    const bool zen = act2() && director.arena == ZENITH_ARENA;
    int nArenas = act2() ? ZENITH_ARENA : (int)level.arenas.size();   // ACT II counts its five; the Zenith is the encore
    const char* tag = act2() ? "ACT II" : "ARENA";
    char buf[128];

    if (!playerDead && !victory) {
        int alive = 0;
        for (auto& e : enemies) if (e.alive) ++alive;
        int left = alive + director.queued();
        glm::vec3 accent{1.f, 0.75f, 0.3f};
        const Enemy* boss = nullptr;
        for (auto& e : enemies) if (e.alive && (isBoss(e.type) || isMiniBoss(e.type))) boss = &e;

        if (story()) {   // the room, and what to do in it
            const int here = std::max(0, level.arenaAt(player.position));
            const char* what = "FIND A WAY OUT";
            accent = {0.35f, 0.85f, 1.f};
            if (here == 0 && level.conveyorOfWall(player.groundWall) >= 0 && player.onGround) { what = "GET OFF THE BELT"; accent = {1.f, 0.45f, 0.2f}; }
            else if (director.fighting()) {
                if (director.hasGoal() && director.goal().kind == WaveGoal::HOLD)
                    snprintf(buf,sizeof(buf),"%s   %.0f%%  %s",director.goal().label,director.goalProgress()*100.f,
                             director.zoneContested?"CLEAR THE PAD":director.holding?"CONTACT SEATING":"STAND ON THE SERVICE PAD");
                else if (director.hasGoal() && director.goal().kind == WaveGoal::EXTERNAL)
                    snprintf(buf,sizeof(buf),"RELEASE TURBINE BRAKES   %d/3   HOSTILES %d",3-level.anchorsAlive(LevelData::ChainAnchor::BRAKE),left);
                else snprintf(buf, sizeof(buf), "%s   HOSTILES %d", here == level.story.pitSection ? "THE SCRAP IS AWAKE" : level.arenas[director.arena].name, left);
                what = nullptr; accent = {1.f, 0.45f, 0.2f};
            }
            else if(wasteland()) {
                static const char* goals[]={"FOLLOW THE ROAD / THE TOWER AHEAD","THROUGH THE DEAD CONVOY","RESTORE THE WAY THROUGH THE RELAY","CROSS THE SPAN / SERVICE ROUTE BELOW","FIND THE LONGSHOT / CONTINUE NORTH","THROUGH THE SERVICE CHECKPOINT","BALLAST / KEEP CLEAR OF THE CUT","THE ROAD IS OPEN / THE TOWER AHEAD"};
                what=goals[std::clamp(here,0,7)];
                if(here==6&&storyBossDown)what="THE BARRIER IS OPEN";
            }
            else if (here == level.story.pitSection && !storyOwns(0)) what = "FIND SOMETHING TO FIGHT WITH";
            else if (here == level.story.pitSection) what = "CLIMB OUT";
            else if (here > level.story.pitSection && !storyOwns(2) && director.arena == here && director.phase == WaveDirector::Phase::APPROACH && player.position.z > 40.f) what = "TAKE THE LANCER";
            else if(here==2)what=storyOwns(2)?"BOARD THE SERVICE LIFT / ASSEMBLY ABOVE":"FOLLOW THE DUCTS";
            else if(here==3)what="DESCEND TO THE PRODUCTION FLOOR";
            else if(here==4)what="READ THE RECORDS / CONTINUE WEST";
            else if(here==5)what="THROUGH THE PODS";
            else if(here==6)what="BOARD THE FREIGHT LIFT";
            else if(here==7)what=storyGrapple?"MEET YOUR JUDGE":"STAY BELOW THE SCANNERS / FIND THE GRAPPLE";
            else if(here==8)what=storyBossDown?"THE GATE IS OPEN / WALK OUT":"THE SORTER IS WAITING";
            if (what) snprintf(buf, sizeof(buf), "%s   %s", level.arenas[here].name, what);
        } else if (fast()) {
            if (finishOpen) { snprintf(buf, sizeof(buf), "FINISH OPEN - CLIMB TO THE BEACON"); accent = {1.f, 0.6f, 0.2f}; }
            else if (director.phase == WaveDirector::Phase::APPROACH) {
                snprintf(buf, sizeof(buf), "ROOM %d/%d  %s   ADVANCE", director.arena + 1, nArenas, ar.name);
                accent = {0.4f, 1.f, 0.6f};
            } else snprintf(buf, sizeof(buf), "ROOM %d/%d  %s   HOSTILES %d", director.arena + 1, nArenas, ar.name, left);
        } else switch (director.phase) {
        case WaveDirector::Phase::APPROACH:   // FAST only
        case WaveDirector::Phase::INTRO:
            if (endless()) snprintf(buf, sizeof(buf), "ENDLESS  %s  GET READY", ar.name);
            else if (zen) snprintf(buf, sizeof(buf), "ENCORE  %s  GET READY", ar.name);
            else snprintf(buf, sizeof(buf), "%s %d/%d  %s  GET READY", tag, director.arena + 1, nArenas, ar.name);
            break;
        case WaveDirector::Phase::ACTIVE:
            if (boss && zen) snprintf(buf, sizeof(buf), "ENCORE  %s", ar.name);
            else if (boss) snprintf(buf, sizeof(buf), "%s %d/%d  FINAL WAVE", tag, director.arena + 1, nArenas);
            else if (director.hasGoal()) {
                const WaveGoal& g = director.goal();
                char what[48];
                switch (g.kind) {
                case WaveGoal::HOLD:
                    if (director.goalDone) snprintf(what, sizeof(what), "HELD");
                    else if (director.holding && director.zoneContested) { snprintf(what, sizeof(what), "CONTESTED - CLEAR THE ZONE"); accent = {1.f, 0.3f, 0.25f}; }
                    else if (director.holding) { snprintf(what, sizeof(what), "HOLDING %.0f%%", director.goalProgress() * 100.f); accent = {0.4f, 1.f, 0.6f}; }
                    else snprintf(what, sizeof(what), "%.0f%% - GET IN THE ZONE", director.goalProgress() * 100.f);
                    break;
                case WaveGoal::CONDUITS: snprintf(what, sizeof(what), "%d LEFT", director.conduitsLeft()); break;
                case WaveGoal::SURVIVE:  snprintf(what, sizeof(what), "%s", formatTime(std::max(0.f, director.goalTimer), false).c_str()); break;
                default: what[0] = 0;
                }
                if (endless()) snprintf(buf, sizeof(buf), "WAVE %d   %s   %s", director.wave + 1, g.label, what);
                else snprintf(buf, sizeof(buf), "WAVE %d/%d   %s   %s", director.wave + 1, director.waveCount(), g.label, what);
            }
            else if (endless()) snprintf(buf, sizeof(buf), "%s   WAVE %d   HOSTILES %d", "ENDLESS",
                                         director.wave + 1, left);
            else snprintf(buf, sizeof(buf), "%s %d/%d   WAVE %d/%d   HOSTILES %d",
                          tag, director.arena + 1, nArenas, director.wave + 1, director.waveCount(), left);
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
        // The run's score so far (not FAST: that's a race), under the clock
        if (!fast() && ranked) {
            char sc[32];
            snprintf(sc, sizeof(sc), "SCORE %d", runScore().total);
            ui.begin2D();
            ui.ui.textRight(sc, SCREEN_W - 14, 54, 2, {1.f, 0.85f, 0.4f, 0.9f});
            ui.end2D();
        }

        const bool msBar = boss && boss->type == EnemyType::ABSOLUTE && (boss->morning() || boss->abSwallow > 0.f);   // (the swallow: his new bar filling)
        if (boss) ui.renderBossBar(msBar ? "MORNINGSTAR" : boss->type == EnemyType::ABSOLUTE ? "THE ABSOLUTE" :
                                   boss->type == EnemyType::SORTER ? "THE SORTER" :
                                   boss->type == EnemyType::SOVEREIGN ? "THE SOVEREIGN" :
                                   boss->type == EnemyType::PENITENT ? "THE PENITENT" :
                                   boss->type == EnemyType::LEVIATHAN ? "THE LEVIATHAN" :
                                   boss->type == EnemyType::WARDEN ? "THE WARDEN" : boss->stats().name,
                                   boss->abSwallow > 0.f ? glm::clamp(1.f - (boss->abSwallow - Enemy::MS_RISE) / (Enemy::MS_SWALLOW - Enemy::MS_RISE), 0.f, 1.f)
                                                         : boss->health / boss->maxHealth,
                                   msBar ? boss->abPhase >= 5 : boss->type == EnemyType::ABSOLUTE ? boss->abPhase >= 2 :
                                   boss->type == EnemyType::LEVIATHAN ? boss->levPhase >= 2 :
                                   boss->type == EnemyType::PENITENT ? boss->scourging :
                                   boss->type == EnemyType::WARDEN ? boss->wardenPhase >= 2 : boss->enraged,
                                   boss->type == EnemyType::BALLAST ? (boss->ballPhase>=2?glm::vec3{1.f,.42f,.14f}:glm::vec3{.95f,.69f,.3f}) : glm::vec3{-1.f});
        if (boss && boss->type == EnemyType::WARDEN) {   // its conduits, or the meltdown clock
            char wb[64];
            if (boss->wardenPhase == 1 && boss->conduitsLeft > 0)
                snprintf(wb, sizeof(wb), "CONDUITS %d/4 - SHOOT THE NODES ON THE PILLARS", boss->conduitsLeft);
            else if (boss->wardenPhase == 3)
                snprintf(wb, sizeof(wb), "MELTDOWN  0:%02d", (int)std::ceil(std::max(boss->meltClock, 0.f)));
            else wb[0] = 0;
            if (wb[0]) {
                ui.begin2D();
                ui.ui.textShadow(wb, SCREEN_W / 2, 82, 2, boss->wardenPhase == 3 ? glm::vec4{1.f, 0.35f, 0.2f, 1.f} : glm::vec4{0.3f, 0.9f, 1.f, 0.95f}, true);
                ui.end2D();
            }
        }
        if (boss && boss->type == EnemyType::LEVIATHAN && (boss->levInhale > 0.f || boss->levPhase == 2)) {   // what to do, under the bar
            ui.begin2D();
            if (boss->levInhale > 0.f) ui.ui.textShadow("SHOOT DOWN ITS THROAT", SCREEN_W / 2, 82, 2, {1.f, 0.9f, 0.8f, 1.f}, true);
            else ui.ui.textShadow("IT HUNTS BELOW - WATCH THE WELLS", SCREEN_W / 2, 82, 2, {1.f, 0.45f, 0.3f, 0.95f}, true);
            ui.end2D();
        }
        if (boss && boss->type == EnemyType::PACEMAKER && boss->pmChannel > 0.f) {   // the beacon: how long, how close to breaking it
            char pb[80];
            snprintf(pb, sizeof(pb), "LIGHTING THE BEACON %.0f  -  BREAK IT %.0f%%", std::ceil(boss->pmChannel),
                     100.f * std::min(1.f, boss->pmChannelDmg / (boss->maxHealth * Enemy::PM_BREAK_SHARE)));
            ui.begin2D(); ui.ui.textShadow(pb, SCREEN_W / 2, 82, 2, {1.f, 0.85f, 0.45f, 1.f}, true); ui.end2D();
        }
        if (boss && isMiniBoss(boss->type)) {   // a mini-boss: what to do about it, under the bar
            char mb[64]; mb[0] = 0; glm::vec4 col{1.f, 0.85f, 0.5f, 0.95f};
            switch (boss->type) {
                case EnemyType::FOREMAN:
                    if (boss->linkCount > 0) snprintf(mb, sizeof(mb), "CREW %d - KILL THE TETHERED TO BREAK ITS GUARD", boss->linkCount);
                    else { snprintf(mb, sizeof(mb), "GUARD BROKEN"); col = {1.f, 0.6f, 0.2f, 1.f}; }
                    break;
                case EnemyType::PLUMB:
                    if (boss->miniOpen > 0.f) { snprintf(mb, sizeof(mb), "CORE OPEN - NOW"); col = {1.f, 1.f, 0.85f, 1.f}; }
                    else snprintf(mb, sizeof(mb), "ARMOURED - WAIT FOR THE VOLLEY");
                    break;
                case EnemyType::HARRIER: if (boss->miniPerch > 0.f) { snprintf(mb, sizeof(mb), "IT'S LOW - HOOK IT"); col = {0.5f, 0.9f, 1.f, 1.f}; } break;
                case EnemyType::BULWARK: if (boss->staggered()) { snprintf(mb, sizeof(mb), "GATE DOWN - x2"); col = {0.4f, 1.f, 0.75f, 1.f}; } break;
                default: break;
            }
            if (mb[0]) { ui.begin2D(); ui.ui.textShadow(mb, SCREEN_W / 2, 82, 2, col, true); ui.end2D(); }
        }
        if (boss && boss->type == EnemyType::PENITENT && boss->anchorsLeft > 0) {   // its chains, under the bar
            char cb[48];
            snprintf(cb, sizeof(cb), "CHAINS %d/6 - SHOOT OR RIP THE ANCHORS", boss->anchorsLeft);
            ui.begin2D();
            ui.ui.textShadow(cb, SCREEN_W / 2, 82, 2, {1.f, 0.7f, 0.3f, 0.95f}, true);
            ui.end2D();
        }

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
        if (act2() && director.phase == WaveDirector::Phase::CLEARED && ar.shift == ArenaShift::DRIFT) {
            glm::vec3 target = level.finishPos;   // the Reliquary's rim: the way down to the Maw
            float sx, sy;
            bool on = projectToScreen(target, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), "THE HOLE %dM", (int)glm::length(target - player.position));
            ui.renderMarker(sx, sy, on, {1.f, 0.4f, 0.25f}, buf);
        }
        if (finishOpen) {
            glm::vec3 target = level.finishPos + glm::vec3{0, 2.f, 0};
            float sx, sy;
            bool on = projectToScreen(target, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), "FINISH %dM", (int)glm::length(target - player.position));
            ui.renderMarker(sx, sy, on, {1.f, 0.6f, 0.2f}, buf);
        }
        // Objective markers: every standing conduit, and the zone while you're out of it
        if (story() && director.fighting() && director.goal().kind == WaveGoal::EXTERNAL)
            for(int id:level.story.turbineBrakes) {
                const auto& brake=level.anchors[id];if(!brake.alive)continue;
                float sx,sy;bool on=projectToScreen(brake.pos+glm::vec3{0,1.5f,0},view,proj,sx,sy);
                snprintf(buf,sizeof(buf),"BRAKE %dM",(int)glm::length(brake.pos-player.position));
                ui.renderMarker(sx,sy,on,{1.f,.7f,.2f},buf);
            }
        if (director.fighting() && director.goal().kind == WaveGoal::CONDUITS)
            for (auto& e : enemies) {
                if (!e.alive || e.type != EnemyType::CONDUIT) continue;
                float sx, sy;
                bool on = projectToScreen(e.position + glm::vec3{0, e.height() + 0.8f, 0}, view, proj, sx, sy);
                snprintf(buf, sizeof(buf), "%dM", (int)glm::length(e.position - player.position));
                ui.renderMarker(sx, sy, on, {1.f, 0.3f, 0.5f}, buf);
            }
        if (director.fighting() && director.goal().kind == WaveGoal::HOLD && !director.holding && !director.goalDone) {
            glm::vec3 target = director.goalPos() + glm::vec3{0, 1.5f, 0};
            float sx, sy;
            bool on = projectToScreen(target, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), "HOLD %dM", (int)glm::length(target - player.position));
            ui.renderMarker(sx, sy, on, {0.4f, 1.f, 0.85f}, buf);
        }
        // The last few enemies get markers so you never hunt for a straggler
        if (director.fighting() && !director.hasGoal() && director.queued() == 0 && alive > 0 && alive <= 3) {
            for (auto& e : enemies) {
                if (!e.targetable()) continue;
                glm::vec3 feet=e.hasShown?e.shownPos:e.position;
                glm::vec3 target=feet+glm::vec3{0,e.height()*.55f,0};
                const auto marker=projectWorldMarker(target,proj*view,SCREEN_W,SCREEN_H);
                bool covered=!marker.behind && markerBehindCover(player.camera.position,target,level.walls);
                const char* height=feet.y>player.position.y+4.f?" / ABOVE":feet.y<player.position.y-4.f?" / BELOW":"";
                snprintf(buf,sizeof(buf),"%s %dM%s%s",e.stats().name,(int)glm::length(target-player.camera.position),height,covered?" / BEHIND COVER":"");
                ui.renderMarker(marker.x,marker.y,marker.onScreen,{1.f,.3f,.25f},buf,marker.behind);
            }
        }
    }

    if (!banners.empty() && !armoryOpen && !playerDead) {
        const Banner& b = banners.front();
        float a = std::min({1.f, b.time * 4.f, (b.duration - b.time) * 2.5f});
        ui.renderBanner(b.title.c_str(), b.subtitle.c_str(), b.color, a);
    }
    if (countdown > 0.f) ui.renderCountdown(countdown);
    if (!playerDead && !victory) renderAbsoluteHUD();
    if (ascendT >= ASCEND_SINK || whiteFade > 0.f) {   // the ascension: the act's card, then a white-out
        ui.begin2D();
        float c = ascendT >= ASCEND_SINK ? std::min(1.f, (ascendT - ASCEND_SINK) * 3.f) : 0.f;
        if (c > 0.f) {
            ui.ui.rect(0, 0, SCREEN_W, SCREEN_H, {0.f, 0.02f, 0.05f, 0.6f * c});
            ui.ui.text("ACT II COMPLETE", SCREEN_W / 2, SCREEN_H / 2 - 60, 5, {0.2f, 1.f, 0.6f, c}, true);
            snprintf(buf, sizeof(buf), "THE LEVIATHAN FALLS  -  %s", formatTime(elapsedTime).c_str());
            ui.ui.text(buf, SCREEN_W / 2, SCREEN_H / 2, 2, {0.85f, 0.9f, 0.88f, c}, true);
        }
        float w = ascendT >= ASCEND_SINK + ASCEND_CARD ? std::min(1.f, (ascendT - ASCEND_SINK - ASCEND_CARD) / ASCEND_WHITE) : whiteFade;
        if (w > 0.f) ui.ui.rect(0, 0, SCREEN_W, SCREEN_H, {1.f, 0.98f, 0.95f, w});
        ui.end2D();
    }
    if (g_practice && !victory && !paused) {
        ui.begin2D();
        ui.ui.textShadow("PRACTICE  F5 CLEAR WAVE  F6 HEAL", 12, SCREEN_H - 22, 1, {0.4f, 1.f, 0.85f, 0.8f});
        ui.end2D();
    }

    if (playerDead && !victory) {   // (ENDLESS has no retry: a moment, then the run's score)
        if (story()) snprintf(buf, sizeof(buf), "CHAPTER %d  %s  -  %s", level.story.chapter,level.story.title,ar.name);
        else if (fast()) snprintf(buf, sizeof(buf), "ROOM %d/%d  %s", director.arena + 1, nArenas, ar.name);
        else if (endless()) snprintf(buf, sizeof(buf), "%s - WAVE %d", ar.name, wavesCleared + 1);
        else if (zen) snprintf(buf, sizeof(buf), "ENCORE  %s", ar.name);
        else snprintf(buf, sizeof(buf), "%s %d/%d %s - WAVE %d/%d", tag, director.arena + 1, nArenas, ar.name,
                      director.wave + 1, director.waveCount());
        ui.renderDeath(buf, totalKills, elapsedTime, fast(), !endless(), story() ? "R - TRY AGAIN" : nullptr);
    }
    if (victory && g_bossRun >= 0) {
        const BossEntry& b = bossEntry(g_bossRun);
        std::string shown;
        if (nameEntry) { shown = nameBuf; if (std::fmod(ui.hudTime, 1.f) < 0.55f && (int)nameBuf.size() < Leaderboard::MAX_NAME_LEN) shown += "_"; }
        ui.renderVictoryBoss(b.name, b.where, bossKillT, newRecord ? 0.f : records.bestBoss[g_bossRun], newRecord, deaths, totalShots, totalHits,
                             nameEntry, shown, bossPosted);
    } else if (victory && roomAttack() && !roomTimes.empty()) {   // ROOM ATTACK: the room, its time, its medal
        const int r = glm::clamp(g_roomAttack, 0, (int)level.arenas.size() - 1);
        const float best = r < (int)records.fastRooms.size() ? records.fastRooms[r] : 0.f;
        ui.renderVictoryRoom(level.arenas[r].name, roomTimes.back(), roomMedals.back(), level.arenas[r].medals,
                             newRecord, best, fastLeftStanding, deaths, totalShots, totalHits);
    } else if (victory && arenaSolo()) {   // the ARENA page's one arena: its time against its best
        const int a = glm::clamp(g_arenaSolo, 0, (int)level.arenas.size() - 1);
        char title[64];
        snprintf(title, sizeof(title), "%s  ARENA %d CLEARED", act2() ? "ACT II" : "ACT I", a + 1);
        ui.renderVictoryArena(totalKills, totalShots, totalHits, deaths, elapsedTime, peakStyle, prog.level,
                              newRecord ? 0.f : records.soloBest(act2(), a), newRecord, true, runScore(), title, level.arenas[a].name);
    } else if (victory) {
        if (fast()) {
            const char* rank = elapsedTime < level.parTimes[0] ? "S" : elapsedTime < level.parTimes[1] ? "A"
                             : elapsedTime < level.parTimes[2] ? "B" : elapsedTime < level.parTimes[3] ? "C" : "D";
            // Show the best run as it was before this one (newRecord already replaced it)
            static std::vector<float> none;
            ui.renderVictoryFast(elapsedTime, newRecord ? 0.f : records.bestFast, newRecord, rank, totalKills,
                                 totalShots, totalHits, deaths, splits, newRecord ? none : records.fastSplits, !nameEntry,
                                 &roomTimes, &roomMedals, fastLeftStanding);
        } else if (endless()) {
            std::string title = std::string("ENDLESS  ") + ar.name;
            ui.renderVictoryEndless(title.c_str(), "", runScore(), wavesCleared, totalKills, totalShots, totalHits,
                                    elapsedTime, records.bestEndless, newRecord, !nameEntry);
        } else if (act2()) {   // the act's end, up in the Zenith (no board yet: a local best)
            ui.renderVictoryArena(totalKills, totalShots, totalHits, deaths, elapsedTime, peakStyle,
                                  prog.level, zen ? records.bestZenith : 0.f, zen && newRecord, true, runScore(),
                                  zen ? "!!!!! CLEARED" : "ACT II COMPLETE", zen ? "THE LIGHT FALLS" : "THE ECLIPSE BREAKS");
        } else {
            ui.renderVictoryArena(totalKills, totalShots, totalHits, deaths, elapsedTime, peakStyle,
                                  prog.level, records.bestArena, newRecord, !nameEntry, runScore());
        }
        if (!act2() && !arenaSolo()) renderLeaderboardPanel();   // (one arena on its own: no board)
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
                                      : std::string(act2() ? "ACT II - " : "ARENA - ") + ar.name;
            line += std::string("   ") + tune().name;
            const Bindings& kb = binds();
            auto k = [&](Act a) { return kb.label(a); };
            std::string move = k(Act::FORWARD) + k(Act::LEFT) + k(Act::BACK) + k(Act::RIGHT);
            if (move.size() != 4) move = k(Act::FORWARD) + "/" + k(Act::LEFT) + "/" + k(Act::BACK) + "/" + k(Act::RIGHT);   // (WASD, unless rebound to long names)
            std::string c1 = move + " MOVE  " + k(Act::JUMP) + " JUMP (TWICE)  " + k(Act::DASH) + " DASH  " + k(Act::SLIDE) +
                             " SLIDE / SLAM  LMB FIRE  " + k(Act::PARRY) + " PARRY  " + k(Act::INTERACT) + " FINISH  " + k(Act::GRENADE) + " GRENADE";
            std::string c2 = k(Act::GRAPPLE) + " GRAPPLE  RMB AIM / ALT FIRE  1-4 / SCROLL WEAPONS  " + k(Act::INSPECT) + " INSPECT  TAB ARMORY  " +
                             k(Act::RELOAD) + " RELOAD  BKSP RESTART";
            ui.renderPause(pauseSelected, labels, line.c_str(), c1.c_str(), c2.c_str());
        }
    }
}
