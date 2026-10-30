#pragma once
// =============================================================================
// Gameplay_Tick.h — GameplayState: The fixed 60 Hz simulation tick, and keeping the player inside the
// arena they're fighting in (zones, ceilings, checkpoints, grapple targets).
// Included at the end of GameplayState.h.
// =============================================================================

// The trigger and the grenade, read every frame (not on the 60 Hz tick): the shot
// goes off in the frame you click, up to 16 ms sooner
inline void GameplayState::shootInput() {
    WeaponState& ws = weapons[activeWeapon];
    bool switchBlocked = (pendingWeapon >= 0);
    if (swordOut) pendingFire = false;   // the sword has the button
    if (pendingFire && ws.ready() && !switchBlocked) fireWeapon(activeWeapon);
    else if (pendingFire && !switchBlocked && (ws.ammo <= 0 || ws.reloading) && dryCd <= 0.f) {
        audio.play("dry", 100, SoundGroup::PLAYER);   // empty: the hammer falls on nothing
        dryCd = 0.3f;
    }
    pendingFire = false;
    if (pendingAlt) { altFire(); pendingAlt = false; }
    if (pendingGrenade && grenadeCount > 0 && grenadeTimer <= 0.f) throwGrenade();
    pendingGrenade = false;
    flushHitSounds();
}

// An enemy shot that just went past your head: a whizz from where it is, louder the closer it came
inline void GameplayState::whizPast() {
    glm::vec3 ear = player.camera.position;
    for (auto& p : projSystem.pool) {
        if (!p.alive || p.isPlayer || p.whizzed || p.isGrenade) continue;
        glm::vec3 to = ear - p.position;
        float d = glm::length(to);
        if (d > 3.f || glm::dot(p.velocity, to) > 0.f) continue;   // too far, or still coming
        p.whizzed = true;
        if (d < 0.6f) continue;   // (that one hit you)
        SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = p.position;
        o.volume = 0.55f + 0.45f * (1.f - d / 3.f);
        o.pitch = glm::clamp(glm::length(p.velocity) / 28.f, 0.7f, 1.4f) * (p.size > 1.5f ? 0.7f : 1.f);   // big orbs: a lower rush
        o.floor = 0.5f;
        audio.playOpts("whiz", o);
    }
}

// The last of a wave falls: a quarter speed easing back over SLOW_TIME, a deep boom, the music ducked
inline void GameplayState::waveEnded() {
    slowMo(SLOW_TIME, 0.25f);
    audio.play("lastkill", 110, SoundGroup::UI);
    audio.music.stinger();   // the track rings its chord over it
    audio.duck(4.f, 0.7f);
    camFeel.punch(-30.f, 0.f, frand(-1.f, 1.f) * 25.f);
    if (g_devOverlay == "lastkill") std::fprintf(stderr, "last kill at %.2f s\n", elapsedTime);
}

// What you're standing on rings? (a steel deck, a panel floor)
inline bool GameplayState::onMetal() const {
    if (player.groundWall < 0 || player.groundWall >= (int)level.walls.size()) return false;
    Mat m = level.walls[player.groundWall].mat;
    return m == Mat::METAL || m == Mat::PANEL || m == Mat::PLATE;
}

// Something big and far off: heard as late as sound takes to get here (a blast 60 m away lands 0.15 s after its flash)
inline void GameplayState::playBoom(const char* name, glm::vec3 at, int volume, SoundGroup g, bool priority, float floor) {
    SfxMixer::Opts o; o.volume = volume / 128.f; o.group = g; o.priority = priority;
    o.positional = true; o.pos = at; o.floor = floor;
    o.delay = std::max(0.f, glm::length(at - player.camera.position) - 8.f) / 343.f;
    audio.playOpts(name, o);
}

// Every hit since the last call as one sound: fuller for more, a ping on a crit
inline void GameplayState::flushHitSounds() {
    if (bodyHitPush > 0.f) {   // the round landing in the body: a thump from where it hit, deeper and duller on the big ones
        SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = bodyHitAt; o.priority = true;
        o.volume = 0.5f + 0.5f * std::min(1.f, bodyHitPush / 10.f);
        o.pitch  = glm::clamp(1.18f - 0.07f * bodyHitMass, 0.7f, 1.15f) * (0.96f + 0.08f * frand(0.f, 1.f));
        o.floor  = 0.55f;   // heard over the fight, whatever's further off
        audio.playOpts("hit_body", o);
        if (bodyHitStun) audio.playAt("stun", bodyHitAt, 115, SoundGroup::ENEMY, true, 0.6f);
        bodyHitPush = 0.f; bodyHitStun = false;
    }
    if (hitSoundCount <= 0) return;
    audio.play("hit", 92 + std::min(36, (hitSoundCount - 1) * 9), SoundGroup::PLAYER);
    if (hitSoundArmor) audio.play("hit_armor", 115, SoundGroup::PLAYER);   // armour: you hear the metal turn it
    if (hitSoundCrit) audio.play("hit_head", 110, SoundGroup::PLAYER);
    hitSoundCount = 0; hitSoundCrit = false; hitSoundArmor = false;
}

inline void GameplayState::physicsTick(float dt, const Uint8* keys, bool parryKey) {
    frametimer::Scope ftSim(frametimer::SIM);
    frametimer::tick();
    frametimer::Lap ftLap;
    int acousticRoom=story()?level.arenaAt(player.position):director.arena;
    if (acousticRoom != spaceArena && acousticRoom >= 0 && acousticRoom < (int)level.arenas.size()) {
        spaceArena = acousticRoom;   // acoustics follow the room occupied, including backtracking
        audio.setSpace(level.arenas[spaceArena].space);
        audio.setRoom(level.arenas[spaceArena].tone);   // and its air
    }

    // --- Moving platforms: move them, then carry whoever stands on one ---
    int rideMover = level.moverOfWall(player.groundWall);
    moverClock += dt;
    updateShifts(dt);
    updateLift(dt);
    level.updateMovers(moverClock);
    if (rideMover >= 0) {
        player.position += level.movers[rideMover].delta;
        player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
    }
    if (int cv = player.onGround ? level.conveyorOfWall(player.groundWall) : -1; cv >= 0) {   // a conveyor carries you along it
        player.position += level.conveyors[cv].dir * (level.conveyors[cv].speed * dt);
        player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
    }
    if (grapple.active && grapple.moverWall >= 0) grapple.follow(level.walls[grapple.moverWall].box);

    // --- Dash ---
    bool pinned = anchoredAt(player.position);   // an ANCHOR's field: no dash, no grapple
    if (pinnedCueCd > 0.f) pinnedCueCd -= dt;
    bool dashKey = keys[SDL_SCANCODE_LSHIFT] != 0;
    if (dashKey && !prevDashKey && pinned) pinnedCue();
    if (dashKey && !prevDashKey && dashCharges > 0 && !pinned && snareTimer <= 0.f) {   // (snared by a wire: no dash)
        // Dash by the keys held, W first: W (or nothing) along the camera in full 3D, else A/D flat to the
        // side, else S straight back along the camera
        bool w = keys[SDL_SCANCODE_W], s = keys[SDL_SCANCODE_S];
        int side = (keys[SDL_SCANCODE_D] ? 1 : 0) - (keys[SDL_SCANCODE_A] ? 1 : 0);
        glm::vec3 dashDir = player.camera.forward();
        if (!w && side != 0) dashDir = player.camera.right() * (float)side;
        else if (!w && s)    dashDir = -dashDir;
        player.velocity = dashDir * 28.f;
        player.launched();
        dashMomentumTimer = 0.30f;
        --dashCharges;
        dashCooldown = 1.2f;
        fovKick = 13.f;
        dashFx = 1.f;
        camFeel.punch(-20.f);   // the head tucks into the rush
        invincFrames = std::max(invincFrames, 0.15f);   // a sliver of i-frames rewards well-timed dodges
        styleSystem.addStyle(8.f);
        audio.play("dash");
    }
    prevDashKey = dashKey;
    if (dashMomentumTimer > 0.f) dashMomentumTimer -= dt;

    if (dashCooldown > 0.f) {
        dashCooldown -= dt;
        if (dashCooldown <= 0.f && dashCharges < 2) {
            ++dashCharges;
            dashCooldown = dashCharges < 2 ? 1.2f : 0.f;
            audio.play("dash_ready", 72, SoundGroup::UI);   // a charge back: a small flick, so you needn't look at the pips
        }
    }

    // --- Double jump ---
    bool jumpKey = keys[SDL_SCANCODE_SPACE] != 0;
    bool justLanded = player.onGround && !prevOnGround;
    if (justLanded) {
        jumpsRemaining = 1;
        if (!slamming && peakFallSpeed > 4.f) {
            // The head sinks with the fall and nods (a spring: it eases down and back), the gun dips too
            float f = std::min(peakFallSpeed, 30.f);
            camFeel.dip(-f * 0.11f);
            camFeel.punch(-f * 2.4f, 0.f, frand(-1.f, 1.f) * f * 0.6f);
            viewModel.land(peakFallSpeed);
            if (peakFallSpeed > HEAVY_LAND) {   // a real drop: the floor answers
                shake(0.3f, 0.025f + 0.025f * std::min(1.f, (peakFallSpeed - HEAVY_LAND) / 14.f));
                audio.play("land_heavy", 70 + (int)std::min(58.f, (peakFallSpeed - HEAVY_LAND) * 5.f));
                fx.spawnShockwave(player.position + glm::vec3{0, 0.05f, 0}, 2.2f, {0.75f, 0.7f, 0.65f});
            }
        }
        landFall = slamming ? 0.f : peakFallSpeed;   // (a slam has its own sound)
        peakFallSpeed = 0.f;
        if (slamming) {
            slamLands();   // harder and wider the further it fell (Gameplay_Feel.h)
            camFeel.dip(-4.f);
            camFeel.punch(-140.f, 0.f, frand(-1.f, 1.f) * 30.f);
            audio.play("slam");
            if (swordOut) swordPlunge(player.position);
            slamming = false;
        }
    }
    prevOnGround = player.onGround;

    if (jumpKey && !prevJumpKey) {
        if (grapple.active && grapple.latched) {
            // Off the wall: pushed out from its face, up, and a little the way you're looking
            glm::vec3 n = hangNormal();
            grapple.release();
            glm::vec3 look = player.camera.flatForward();
            player.velocity = n.y < -0.5f ? look * 6.f : glm::vec3{n.x, 0.f, n.z} * 8.f + look * 4.f + glm::vec3{0.f, 9.f, 0.f};
            player.launched();
            styleSystem.addStyle(10.f);
            audio.play("jump_air", 110);
            camFeel.punch(-30.f);
        } else if (grapple.active) {
            // Slingshot: release grapple mid-swing and keep all momentum + upward kick.
            grapple.release();
            player.velocity.y += 6.f;
            player.launched();
            styleSystem.addStyle(10.f);
            audio.play("jump");
        } else if (jumpsRemaining > 0 && !player.onGround && player.coyoteTimer <= 0.f) {
            player.airJump();
            --jumpsRemaining;
            styleSystem.addStyle(3.f);
            // The air jump has its own sound and a ring kicked off under your feet
            audio.play("jump_air", 110);
            fx.spawnShockwave(player.position + glm::vec3{0, -0.1f, 0}, 1.6f, {0.45f, 0.85f, 1.f});
            fx.spawnBurst(player.position, {0.5f, 0.85f, 1.f}, 10, 4.f, 0.3f, -2.f);
            camFeel.punch(-38.f);
            camFeel.dip(0.5f);
            fovKick = std::max(fovKick, 4.f);
        }
    }
    prevJumpKey = jumpKey;

    // --- Slam ---
    bool crouchKey = (keys[SDL_SCANCODE_LCTRL] != 0) || (keys[SDL_SCANCODE_C] != 0);
    if (crouchKey && !player.onGround && !slamming && player.velocity.y < 0.f) {
        player.velocity.y = -40.f;
        slamming = true;
        slamFromY = player.position.y;
    }

    // --- Grapple fire / release ---
    glm::vec3 gPoint{0.f}; int gWall = -1; bool gMover = false;
    bool canHook = findGrappleTarget(gPoint, gWall, gMover);
    int hookE = grapple.active || pinned ? -1 : findEnemyHook();   // an enemy under the crosshair: the hook takes it first
    grappleTargetInSight = (canHook && gMover) || hookE >= 0;
    if (hookE >= 0 && grappleCooldown <= 0.f) teach(TEACH_HOOK);
    if (pendingGrapple && pinned && !grapple.active) { pendingGrapple = false; pinnedCue(); }
    if (pinned && grapple.active) grapple.release();
    if (grappleCooldown > 0.f) grappleCooldown -= dt;
    if (pendingGrapple) {
        if (!grapple.active) {
            if (grappleCooldown > 0.f) {
                audio.play("dry", 100, SoundGroup::PLAYER);   // still coiling: no spamming the hook
            } else if (hookE >= 0) {
                hookEnemy(hookE);
            } else if (canHook) {
                glm::vec3 impulse{0.f};
                grapple.attach(player.camera.position, gPoint, impulse, gWall,
                               gMover ? &level.walls[gWall].box : nullptr);
                player.velocity = impulse;
                player.launched();
                viewModel.triggerGrapple();
                audio.play("grapple_fire");
                if (gMover) styleSystem.addStyle(4.f);
            }
        } else {
            grapple.release();
        }
        pendingGrapple = false;
    }
    updateEnemyZip();
    if (g_devOverlay == "hang") {   // dev: the grapple fired at whatever's ahead, pressed again 4 s later (the hang, then the let-go)
        static int presses = 0;
        if (presses == 0 && elapsedTime > 1.f) { ++presses; pendingGrapple = true; }
        else if (presses == 1 && elapsedTime > 5.f) { ++presses; pendingGrapple = true; }
    }
    grapple.update(dt, player.camera.position, player.velocity);
    if (grapple.active && grapple.latched) {   // hanging: until you press the grapple again or jump off
        if (!hangPrev) {
            hangT = 0.f; hangPos = player.position; audio.play("land_metal", 70); camFeel.dip(0.25f);
            if (g_devOverlay == "hang") std::fprintf(stderr, "hang: caught at %.2f s, feet (%.1f %.1f %.1f)\n", elapsedTime, hangPos.x, hangPos.y, hangPos.z);
        }
        hangT += dt;
        bool gone = false;   // what it caught on has moved away (a door opening): nothing to hold
        if (grapple.hookedWall >= 0 && grapple.hookedWall < (int)level.walls.size()) {
            const AABB& b = level.walls[grapple.hookedWall].box;
            glm::vec3 q = glm::clamp(grapple.target, b.min, b.max);
            gone = glm::length(q - grapple.target) > 0.5f;
        }
        if (gone) grapple.release();
    }
    if (hangPrev && !(grapple.active && grapple.latched) && g_devOverlay == "hang")
        std::fprintf(stderr, "hang: let go at %.2f s after %.2f s, feet (%.1f %.1f %.1f)\n", elapsedTime, hangT, player.position.x, player.position.y, player.position.z);
    hangPrev = grapple.active && grapple.latched;
    if (yankT > 0.f) yankT -= dt;
    if (grapple.active) player.launched();   // swung: the rope's arc, not a jump's
    if (wasGrappling && !grapple.active && !fast()) grappleCooldown = GRAPPLE_COOLDOWN;   // however it ended (arrival, release, a rip, a pin); FAST runs grapple freely
    wasGrappling = grapple.active;

    // --- Aiming (rifles, RMB held) ---
    {
        const WeaponDef& d = weaponDef((WeaponId)activeWeapon);
        const WeaponState& ws = weapons[activeWeapon];
        bool rmb = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || gamepad::state().aim;
        bool want = d.canAim && rmb && pendingWeapon < 0 && !ws.reloading;
        float before = aim;
        if (g_devAim) want = d.canAim;
        if (swordOut) want = false;
        if (want) aim = std::min(1.f, aim + dt / d.aimTime);
        else      aim = std::max(0.f, aim - dt / (d.canAim ? d.aimTime * 0.6f : 0.05f));
        if (before < 0.02f && aim >= 0.02f && d.scope) audio.play("scope", 70);
        if (before < 0.98f && aim >= 0.98f) aimFullAt = gameClock;
        player.horizontalSpeed = 7.f * glm::mix(1.f, d.aimMoveMult, aim);
        // RMB on a gun that doesn't aim is its alt fire (the revolver fans, the shotgun lets both barrels go)
        if (rmb && !altPrev && !d.canAim && !swordOut && pendingWeapon < 0) pendingAlt = true;
        altPrev = rmb;
    }
    // The revolver fanned: the rest of the cylinder, a round every 75 ms
    if (fanLeft > 0) {
        fanTimer -= dt;
        if (fanTimer <= 0.f) {
            WeaponState& fw = weapons[(int)WeaponId::REVOLVER];
            if (activeWeapon == (int)WeaponId::REVOLVER && fw.ammo > 0 && !fw.reloading && pendingWeapon < 0) {
                fanning = true; fireWeapon((int)WeaponId::REVOLVER); fanning = false;
                if (--fanLeft <= 0) fw.cooldown = std::max(fw.cooldown, 0.35f);
                fanTimer = 0.075f;
            } else fanLeft = 0;
        }
    }
    updateSword(dt);   // (sets the move speed while it's out)

    // --- Player physics ---
    player.dynWalls = level.moverWalls.data();
    player.dynCount = (int)level.moverWalls.size();
    int boost = level.boosterAt(player.position);
    applyWater(player, level);
    if (!(g_devCam && g_devNoMouse)) {   // screenshot runs: the camera stays exactly where it was put
        player.update(dt, keys, level.walls.data(), (int)level.walls.size(),
                      grapple.active || dashMomentumTimer > 0.f || boost >= 0, &spatialGrid);
        if (boost >= 0) { applyBooster(level.boosters[boost], player, dt); player.launched(); }
        if (grapple.active && grapple.latched) { player.position = hangPos; player.velocity = glm::vec3{0.f}; }   // held where it caught
        keepPlayerInZone(dt);
        if (story()) { updateStory(dt); player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0}; }
        if (player.jumpedThisTick) {   // the push-off: a soft scuff and the head lifting off the legs
            audio.play("jump", 90);
            camFeel.dip(0.35f);
            camFeel.punch(-14.f);
        }
        if (player.sliding && !slidePrev) {   // into a slide: a scrape, the head drops and leans in
            audio.play("slide", 110);
            camFeel.punch(-28.f, 0.f, glm::dot(player.velocity, player.camera.right()) > 0.f ? 18.f : -18.f);
            fovKick = std::max(fovKick, 5.f);
        }
        slidePrev = player.sliding;
        slideKicks();
        airT = player.onGround ? 0.f : airT + dt;
    }
    if(story() && g_devCam && g_devNoMouse) updateStory(dt);
    if (act2Falling && player.onGround) act2Falling = false;
    if (boost >= 0 && boostPrev < 0) {
        audio.play("boost", 100);
        fovKick = std::max(fovKick, 10.f);
        styleSystem.addStyle(3.f);
        grapple.release();
    }
    if (boost < 0 && boostPrev >= 0) dashMomentumTimer = std::max(dashMomentumTimer, 0.35f);   // shoot out of the tube
    boostPrev = boost;
    pushPlayerOutOfEnemies();
    playerXZSpeed = glm::length(glm::vec2(player.velocity.x, player.velocity.z));
    if (!player.onGround) peakFallSpeed = std::max(peakFallSpeed, -player.velocity.y);

    // --- Jump pads ---
    padCooldown = std::max(0.f, padCooldown - dt);
    for (auto& pad : level.pads) {
        glm::vec3 p = player.position;
        if (padCooldown <= 0.f && player.velocity.y <= 0.5f && p.y < pad.centre.y + 0.4f && p.y > pad.centre.y - 0.6f &&
            std::fabs(p.x - pad.centre.x) < pad.half.x && std::fabs(p.z - pad.centre.z) < pad.half.y) {
            player.velocity = pad.launch;
            player.launched();
            player.onGround = false;
            jumpsRemaining = 1;
            padCooldown = 0.4f;
            grapple.release();
            styleSystem.addStyle(4.f);
            audio.play("dash", 90);
            fx.spawnBurst(pad.centre + glm::vec3{0, 0.2f, 0}, {0.3f, 1.f, 0.8f}, 18, 4.f, 0.5f, -4.f);
        }
    }

    // --- Lava ---
    hazardTick = std::max(0.f, hazardTick - dt);
    for (auto& hz : level.hazards) {
        glm::vec3 p = player.position;
        if (p.y < hz.box.max.y + 0.3f && p.y > hz.box.min.y - 0.5f && p.x > hz.box.min.x && p.x < hz.box.max.x &&
            p.z > hz.box.min.z && p.z < hz.box.max.z) {
            if (!g_godMode && director.phase != WaveDirector::Phase::VICTORY) {
                float burn = std::min(hz.dps * dt, styleSystem.health);
                styleSystem.health -= burn;
                styleSystem.damageTaken += burn;
            }
            if (hazardTick <= 0.f) {
                hazardTick = 0.35f;
                ui.onDamage();
                audio.play("player_hit", 60);
                fx.spawnBurst(p + glm::vec3{0, 0.2f, 0}, {1.f, 0.45f, 0.1f}, 8, 3.f, 0.5f, -3.f);
            }
        }
    }

    // --- Footsteps / landing ---
    // A step each time a foot comes down in the stride (CameraFeel): the bob's low point
    if (camFeel.stepTick(playerXZSpeed, player.onGround && !player.sliding, dt)) {
        static const char* STEPS[] = {"step1", "step2", "step3", "step4"};
        static int last = -1;
        int k = rand() % 3; if (k >= last) ++k; last = k;   // never the same step twice running
        int vol = 50 + (int)std::min(14.f, playerXZSpeed * 1.5f);
        if (player.wadeDepth >= 0.1f) audio.play("wade", 60);
        else if (onMetal()) audio.play("step_metal", vol);   // a steel deck rings under you
        else audio.play(STEPS[k], vol);
    }
    // The land sound grows with the fall (a hop is a tap, a drop is a thud)
    if (justLanded && landFall > 2.f) audio.play(onMetal() ? "land_metal" : "land", 45 + (int)std::min(83.f, landFall * 5.f));
    // Sliding over dry ground: grit thrown up from under you
    if (player.sliding && player.onGround && player.wadeDepth < 0.1f && playerXZSpeed > 6.f) {
        slideDustT -= dt;
        if (slideDustT <= 0.f) {
            slideDustT = 0.05f;
            glm::vec3 back = -glm::normalize(glm::vec3{player.velocity.x, 0.f, player.velocity.z});
            fx.spawnBurst(player.position + back * 0.3f + glm::vec3{0, 0.05f, 0}, {0.7f, 0.62f, 0.55f}, 3, 2.5f, 0.35f, -6.f);
        }
    }
    // Water: a splash on landing in it, spray and a hiss while a slide skims it
    if (justLanded && player.wadeDepth >= 0.1f) {
        fx.spawnBurst(player.position + glm::vec3{0, 0.1f, 0}, {0.6f, 0.85f, 0.9f}, 20, 6.f, 0.5f, 9.f);
        audio.play("wade", 90);
    }
    bool skimming = player.sliding && player.onGround &&
                    level.waterSurfaceAt(player.position.x, player.position.z) > player.position.y + 0.05f;
    if (skimming) {
        skimTimer -= dt;
        if (skimTimer <= 0.f) {
            skimTimer = 0.18f;
            fx.spawnBurst(player.position + glm::vec3{0, 0.25f, 0}, {0.6f, 0.85f, 0.9f}, 6, 4.f, 0.35f, 9.f);
            static int hiss = 0;
            if (hiss++ % 3 == 0) audio.play("skim", 70);
        }
    } else skimTimer = 0.f;

    // --- Weapons ---
    grenadeTimer = std::max(0.f, grenadeTimer - dt);
    invincFrames = std::max(0.f, invincFrames - dt);
    for (int w = 0; w < WEAPON_COUNT; ++w)
        weapons[w].tick(dt, weaponMag((WeaponId)w, prog.up[w]));   // the rifles' bolt home is the reload's CLOSE cue
    WeaponState& ws = weapons[activeWeapon];
    int mag = weaponMag((WeaponId)activeWeapon, prog.up[activeWeapon]);
    if (keys[SDL_SCANCODE_R] && !ws.reloading && ws.ammo < mag && pendingWeapon < 0) startReload(activeWeapon);
    // Empty: reload once the last shot's cycle is done
    if (ws.ammo <= 0 && !ws.reloading && ws.cooldown <= 0.f && pendingWeapon < 0) startReload(activeWeapon);

    dryCd = std::max(0.f, dryCd - dt);
    shootInput();   // (again here: a click held over from a frame with no tick)


    ftLap.to(frametimer::SIM_PLAYER);
    // --- Waves ---
    int alive = 0;
    for (auto& e : enemies) if (e.alive) ++alive;
    alive += relic.alive();              // a Revenant's soul in flight: not over until it's caught or back
    alive += (int)pendingTwins.size();   // a TWINNED kill this tick: its copies count before they're spawned
    std::vector<SpawnRequest> spawns;
    director.countScale    = tune().waveSize;
    director.maxAliveBonus = tune().maxAliveBonus;
    if (endless()) feedEndless();
    // HOLD: an enemy on foot in the circle stops it filling
    director.zoneContested = false;
    if (director.goal().kind == WaveGoal::HOLD)
        for (auto& e : enemies)
            if (e.targetable() && !e.stats().flying && e.type != EnemyType::CONDUIT && director.inHoldZone(e.position + glm::vec3{0, 0.3f, 0}))
                director.zoneContested = true;
    WaveDirector::Phase phaseBefore = director.phase;
    if (!act2Falling) director.update(dt, alive, player.position, spawns);   // ACT II: the fight waits for you to land
    // The last of a wave falls: time drops to a quarter and eases back (not in FAST: the clock is the point)
    if (phaseBefore == WaveDirector::Phase::ACTIVE && !fast() &&
        (director.phase == WaveDirector::Phase::BREAK || director.phase == WaveDirector::Phase::CLEARED)) waveEnded();
    if (g_devOverlay == "lastkill" && slowT <= 0.f && elapsedTime > 3.f && elapsedTime < 3.1f) waveEnded();   // dev: the moment, on cue
    if (g_devOverlay == "clearwaves" && director.phase == WaveDirector::Phase::ACTIVE && std::fmod(elapsedTime, 4.f) < dt)
        devClearWave();   // dev: every fight won for you, one wave every 4 s (to hear the clears: --audiodump)
    for (auto& s : spawns) spawnEnemy(s.type, s.pos, s.hollow);

    ftLap.to(frametimer::SIM_DIRECTOR);
    // --- Enemies ---
    updateEnemies(dt);
    ftLap.to(frametimer::SIM_ENEMIES);
    hurtBy = EnemyType::COUNT;   // a boss fight's hazards: put down to the boss
    for (const auto& o : enemies) if (o.alive && isBoss(o.type)) { hurtBy = o.type; break; }
    updateSanctumPhase(dt);
    updateSovereign(dt);
    updatePenitent(dt);
    updateWarden(dt);
    updateReliquary(dt);
    updateLeviathan(dt);
    updateAbsolute(dt);
    steerParriedOrbs(dt);   // parried orbs into the Warden's core, the Leviathan's eye
    if (fast() && countdown <= 0.f && !victory) ghostRec.record(elapsedTime, player.position, player.camera.yaw);
    // FAST: your best run's ghost going by close (it passing you, or you it): a whoosh from where it is
    if (ghostWhooshCd > 0.f) ghostWhooshCd -= dt;
    if (fast() && countdown <= 0.f && !victory && ghostWhooshCd <= 0.f && (!settings || settings->ghost)) {
        glm::vec3 gf; float gy;
        if (ghost.at(elapsedTime, gf, gy)) {
            glm::vec3 d = gf - player.position;
            if (glm::length(glm::vec2(d.x, d.z)) < 2.2f && std::fabs(d.y) < 2.5f && playerXZSpeed > 6.f) {
                audio.playAt("whiz", gf + glm::vec3{0.f, 1.2f, 0.f}, 110, SoundGroup::UI, true);
                ghostWhooshCd = 3.f;
            }
        }
    }

    ftLap.to(frametimer::SIM_BOSSES);
    // --- Projectiles ---
    projSystem.floorY = level.lowestFloor();
    auto result = projSystem.update(dt, level.walls.data(), (int)level.walls.size(),
                                    enemies, player.camera.position, &spatialGrid);

    whizPast();
    bool parryPressed = parryKey && !parryPrev;
    parryPrev = parryKey;
    punchCooldown = std::max(0.f, punchCooldown - dt);
    if (parryPressed && punchCooldown <= 0.f) punch(result.boostableIndex);

    for (auto& [pi, ei] : result.enemyHits) {
        auto& e = enemies[ei];
        const Projectile& pr = projSystem.pool[pi];
        if (pr.parried && pr.heavy) {
            fx.spawnExplosionParticles(pr.position, 3.f);
            playBoom("explosion", pr.position, 100, SoundGroup::WORLD);
            shake(0.3f, 0.07f);
        }
        float spd = glm::length(pr.velocity);
        HitPush hp{spd > 1e-3f ? pr.velocity / spd : player.camera.forward(), std::min(pr.damage * 0.08f, 30.f), 1.5f};   // a parried orb hits like a truck
        hurtEnemy(e, pr.damage, pr.position, pr.parried ? 25.f : 10.f, 2.f,
                  pr.parried ? StyleSource::PARRY : StyleSource::EXPLOSIVE, false, pr.parried, &hp);
    }
    // A Juggernaut's shell into one of its own
    for (auto& [pi, ei] : result.friendlyHits) {
        const Projectile& pr = projSystem.pool[pi];
        fx.spawnExplosionParticles(pr.position, 2.f);
        playBoom("explosion", pr.position, 80, SoundGroup::WORLD);
        hurtEnemy(enemies[ei], FRIENDLY_SHELL_DAMAGE, pr.position, 0.f, 0.f, StyleSource::FRIENDLY);
    }
    for (int pi : result.wallHits) {   // a shot breaks on the wall: its cubes thrown back off it
        const Projectile& pr = projSystem.pool[pi];
        float spd = glm::length(pr.velocity);
        glm::vec3 back = spd > 1e-3f ? -pr.velocity / spd : glm::vec3{0.f, 1.f, 0.f};
        fx.spawnShards(pr.prevPosition, back, pr.emissiveColor, pr.size, groundHeightAt(pr.prevPosition.x, pr.prevPosition.z, pr.prevPosition.y + 0.2f));
    }
    for (auto& exp : result.explosions)
        pendingBlasts.push_back({exp.pos, exp.radius, exp.damage, exp.radius * 0.5f, 20.f});
    processBlasts();
    ftLap.to(frametimer::SIM_SHOTS);

    hurtBy = EnemyType::COUNT;
    if (result.hitPlayer && result.playerHitOwner >= 0) hurtBy = (EnemyType)result.playerHitOwner;
    if (result.hitPlayer) {   // with the sword out, a shot from the front meets the guard
        if (swordOut) damagePlayerStrike(result.playerDamage, result.playerHitFrom, Sword::Strike::WHITE);
        else damagePlayer(result.playerDamage, result.playerHitFrom, 0.2f, 0.04f);
    }

    updatePickups(dt);

    // --- FAST: the finish beacon ---
    if (finishOpen && !victory) {
        glm::vec3 d = player.position - level.finishPos;
        if (glm::length(glm::vec2(d.x, d.z)) < 3.f && std::fabs(d.y) < 4.f) {
            finishOpen = false;
            fx.spawnBurst(level.finishPos + glm::vec3{0, 1.f, 0}, {1.f, 0.6f, 0.2f}, 60, 10.f, 1.2f, -2.f);
            audio.play("anvil_final", 128, SoundGroup::UI, true);   // the smith's last three blows
            finishRun();
        }
    }

    flushHitSounds();
    if (ceilingFxTimer > 0.f) ceilingFxTimer -= dt;
    nearInteractable = findInteractTarget() >= 0;
    fx.tickMarks(dt);

    // Clear out dead enemies once nothing references them by index
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                  [](const Enemy& e){ return !e.alive; }), enemies.end());
    for (auto& t : pendingTwins) { enemies.push_back(t); enemies.back().uid = nextEnemyUid++; }
    pendingTwins.clear();
    linkConductors(enemies, player.position);   // tethers for the next tick (and this frame's beams)
    linkForemen(enemies);                       // THE FOREMAN's crew (EnemyMini.h)
    ftLap.to(frametimer::SIM_REST);
}

inline bool GameplayState::findGrappleTarget(glm::vec3& point, int& wall, bool& isMover) const {
    glm::vec3 o = player.camera.position, d = player.camera.forward();
    float best = grapple.maxLength;
    wall = -1; isMover = false;
    for (int i = 0; i < (int)level.walls.size(); ++i) {
        if (level.walls[i].dynamic) continue;
        float t = rayBoxHit(o, d, level.walls[i].box);
        if (t > 0.f && t < best) { best = t; wall = i; }
    }
    float bestMover = best + 0.5f;
    int moverWall = -1;
    for (int w : level.moverWalls) {
        AABB b = level.walls[w].box;
        b.min -= glm::vec3{1.2f}; b.max += glm::vec3{1.2f};
        float t = rayBoxHit(o, d, b);
        if (t > 0.f && t < bestMover) { bestMover = t; moverWall = w; }
    }
    if (moverWall >= 0) {
        const AABB& b = level.walls[moverWall].box;
        point = {(b.min.x + b.max.x) * 0.5f, b.max.y + 1.2f, (b.min.z + b.max.z) * 0.5f};
        wall = moverWall; isMover = true;
        return true;
    }
    if (wall >= 0) { point = o + d * best; return true; }
    return false;
}

inline void GameplayState::keepPlayerInZone(float dt) {
    static std::vector<const AABB*> zones;
    zones.clear();
    auto addArena = [&](int i) {
        zones.push_back(&level.arenas[i].zone);
        for (auto& z : level.arenas[i].extraZones) zones.push_back(&z);
    };
    if (story()) {   // STORY: every section you've reached, and the next one (walked into)
        for (int i = 0; i <= std::min(director.arena + (director.fighting() ? 0 : 1), (int)level.arenas.size() - 1); ++i) addArena(i);
    } else if (fast()) {
        for (int i = 0; i <= director.arena; ++i) addArena(i);
    } else {
        addArena(director.arena);
        if (director.phase == WaveDirector::Phase::CLEARED) {
            if (director.arena < (int)level.corridors.size()) zones.push_back(&level.corridors[director.arena]);   // (the Maw has none: the Zenith is reached by rising)
            addArena(director.arena + 1);
        }
    }
    glm::vec3& p = player.position;
    const AABB* in = constrainToZones(p,player.velocity,player.height,zones,story());
    // Ceiling: a force field you can see when you bump it
    float ceiling = in->max.y;
    if (p.y + player.height > ceiling) {
        p.y = ceiling - player.height;
        if (player.velocity.y > 0.f) player.velocity.y = -1.f;
        if (grapple.active && grapple.target.y > ceiling - 1.f) grapple.release();
        if (ceilingFxTimer <= 0.f) {
            ceilingFxTimer = 0.35f;
            for (int i = 0; i < 26; ++i) {
                Effects::Particle* q = fx.freeParticle();
                if (!q) break;
                float a = i * 6.2832f / 26.f;
                q->pos = glm::vec3{p.x, ceiling, p.z} + glm::vec3{std::cos(a), 0.f, std::sin(a)} * 0.6f;
                q->vel = glm::vec3{std::cos(a), 0.f, std::sin(a)} * 5.f;
                q->color = glm::vec3{0.3f, 0.85f, 1.f} * 1.3f;
                q->gravity = 0.f; q->maxLife = q->life = 0.5f; q->alive = true;
            }
            audio.play("barrier", 70);
        }
    }
    player.camera.position = p + glm::vec3{0, player.eyeHeight, 0};

    // Never stuck: inside something solid for a moment (a platform, a rising floor or a
    // closing door pushed you in) and you're put back on the last safe ground you stood on.
    // (tests/BoundsScan.h checks that the static level itself has nowhere to get stuck.)
    {
        bool embedded = bodyEmbedded(level.walls, level.moverWalls, spatialGrid, p, player.radius, player.height);
        embeddedT = embedded ? embeddedT + dt : 0.f;
        bool firm = player.onGround && (player.groundWall < 0 || level.moverOfWall(player.groundWall) < 0);   // not riding anything
        if (!embedded && firm) { safeGround = p; hasSafeGround = true; }
        if (embeddedT > 0.25f && hasSafeGround) {
            std::fprintf(stderr, "unstuck: (%.1f %.1f %.1f) -> (%.1f %.1f %.1f)\n", player.position.x, player.position.y, player.position.z, safeGround.x, safeGround.y, safeGround.z);
            p = safeGround; player.velocity = glm::vec3{0.f}; embeddedT = 0.f;
            grapple.release();
            prevCamPos = p + glm::vec3{0, player.eyeHeight, 0};
            ui.feed("UNSTUCK", {0.4f, 0.9f, 1.f});
            audio.play("barrier", 90);
        }
    }

    // Void: back to the start of the section you fell out of
    int a = level.arenaAt(p);
    if (a >= 0 && p.y < level.arenas[a].voidY) {
        resetToCheckpoint(a);
        if (!g_godMode) styleSystem.takeDamage(15.f);
        ui.onDamage();
        ui.feed("FELL - BACK TO THE LEDGE", {1.f, 0.5f, 0.3f});
        audio.play("player_hit");
        audio.duck(5.f, 0.15f);
    }
    (void)dt;
}

inline void GameplayState::resetToCheckpoint(int a) {
    const Arena& ar = level.arenas[a];
    player.position = ar.hasRespawn ? ar.respawn : ar.playerStart;
    player.velocity = glm::vec3{0.f};
    player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
    prevCamPos = player.camera.position;
    grapple.release();
    slamming = false; peakFallSpeed = 0.f;
    hasSafeGround = false; embeddedT = 0.f; slowT = 0.f;
    dashFx = speedFx = lowFx = heartT = heartPulse = windFx = 0.f; audio.setMuffle(0.f); audio.setWind(0.f);
}

inline void GameplayState::pushPlayerOutOfEnemies() {
    for (auto& e : enemies) {
        if (!e.targetable() || e.stats().flying) continue;
        glm::vec3 p = player.position;
        if (p.y > e.position.y + e.height() || p.y + player.height < e.position.y) continue;
        glm::vec2 d{p.x - e.position.x, p.z - e.position.z};
        float minD = e.radius() + player.radius;
        float len = glm::length(d);
        if (len < minD && len > 1e-3f) {
            glm::vec2 push = d / len * (minD - len);
            player.position.x += push.x; player.position.z += push.y;
            player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
        }
    }
}

inline float GameplayState::groundHeightAt(float x, float z, float fromY, bool movers) const {
    static std::vector<int> cands;
    float base = level.baseFloor(x, z);
    AABB q{{x - 0.05f, base - 1.f, z - 0.05f}, {x + 0.05f, fromY + 0.5f, z + 0.05f}};
    spatialGrid.query(q, cands);
    float best = base;
    for (int i : cands) {
        const AABB& b = level.walls[i].box;
        if (x >= b.min.x && x <= b.max.x && z >= b.min.z && z <= b.max.z && b.max.y <= fromY + 0.5f)
            best = std::max(best, b.max.y);
    }
    if (movers) for (int wi : level.moverWalls) {   // moving platforms (the Descent's cage) aren't in the grid
        const AABB& b = level.walls[wi].box;
        if (x >= b.min.x && x <= b.max.x && z >= b.min.z && z <= b.max.z && b.max.y <= fromY + 0.5f)
            best = std::max(best, b.max.y);
    }
    return best;
}

inline int GameplayState::findInteractTarget() const {
    return interactTarget(interactables, player.camera.position, player.camera.forward(), level.walls);
}

// The face of what the grapple's hanging on: the side of its box nearest the hook (no box: back toward you)
inline glm::vec3 GameplayState::hangNormal() const {
    glm::vec3 away = player.camera.position - grapple.target; away.y = 0.f;
    glm::vec3 fallback = glm::length(away) > 1e-3f ? glm::normalize(away) : -player.camera.flatForward();
    if (grapple.hookedWall < 0 || grapple.hookedWall >= (int)level.walls.size()) return fallback;
    const AABB& b = level.walls[grapple.hookedWall].box;
    const glm::vec3 p = grapple.target;
    const float d[6] = {p.x - b.min.x, b.max.x - p.x, p.y - b.min.y, b.max.y - p.y, p.z - b.min.z, b.max.z - p.z};
    static const glm::vec3 N[6] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
    int best = 0;
    for (int i = 1; i < 6; ++i) if (std::fabs(d[i]) < std::fabs(d[best])) best = i;
    return N[best];
}
