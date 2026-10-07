#pragma once
// =============================================================================
// Gameplay_Tick.h — GameplayState: The fixed 60 Hz simulation tick, and keeping the player inside the
// arena they're fighting in (zones, ceilings, checkpoints, grapple targets).
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::physicsTick(float dt, const Uint8* keys, bool parryKey) {
    spawnSoundThisTick = false;
    if (director.arena != spaceArena && director.arena >= 0 && director.arena < (int)level.arenas.size()) {
        spaceArena = director.arena;   // a new place: its reverb
        audio.setSpace(level.arenas[spaceArena].space);
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
    if (grapple.active && grapple.moverWall >= 0) grapple.follow(level.walls[grapple.moverWall].box);

    // --- Dash ---
    bool pinned = anchoredAt(player.position);   // an ANCHOR's field: no dash, no grapple
    if (pinnedCueCd > 0.f) pinnedCueCd -= dt;
    bool dashKey = keys[SDL_SCANCODE_LSHIFT] != 0;
    if (dashKey && !prevDashKey && pinned) pinnedCue();
    if (dashKey && !prevDashKey && dashCharges > 0 && !pinned) {
        // Full 3D dash in the direction the camera faces
        glm::vec3 dashDir = player.camera.forward();
        player.velocity = dashDir * 28.f;
        dashMomentumTimer = 0.30f;
        --dashCharges;
        dashCooldown = 1.2f;
        fovKick = 13.f;
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
        }
    }

    // --- Double jump ---
    bool jumpKey = keys[SDL_SCANCODE_SPACE] != 0;
    bool justLanded = player.onGround && !prevOnGround;
    if (justLanded) {
        jumpsRemaining = 1;
        if (!slamming && peakFallSpeed > 4.f) {
            landSquash = glm::clamp(peakFallSpeed / 22.f, 0.f, 1.f) * 0.22f;
            viewModel.land(peakFallSpeed);   // the gun dips too, more after a bigger fall
        }
        peakFallSpeed = 0.f;
        if (slamming) {
            for (auto& e : enemies) {
                if (!e.targetable()) continue;
                if (glm::length(e.position - player.position) < 4.5f)
                    hurtEnemy(e, 30.f, e.position + glm::vec3{0, e.height() * 0.5f, 0}, 15.f, 2.f, StyleSource::SLAM);
            }
            fx.spawnShockwave(player.position, 4.5f, {1.f, 0.8f, 0.4f});
            shake(0.3f, 0.08f);
            audio.play("slam");
            slamming = false;
        }
    }
    prevOnGround = player.onGround;

    if (jumpKey && !prevJumpKey) {
        if (grapple.active) {
            // Slingshot: release grapple mid-swing and keep all momentum + upward kick.
            grapple.release();
            player.velocity.y += 6.f;
            styleSystem.addStyle(10.f);
            audio.play("jump");
        } else if (jumpsRemaining > 0 && !player.onGround && player.coyoteTimer <= 0.f) {
            player.velocity.y = player.jumpForce;
            --jumpsRemaining;
            styleSystem.addStyle(3.f);
            audio.play("jump");
        }
    }
    prevJumpKey = jumpKey;

    // --- Slam ---
    bool crouchKey = (keys[SDL_SCANCODE_LCTRL] != 0) || (keys[SDL_SCANCODE_C] != 0);
    if (crouchKey && !player.onGround && !slamming && player.velocity.y < 0.f) {
        player.velocity.y = -40.f;
        slamming = true;
    }

    // --- Grapple fire / release ---
    glm::vec3 gPoint; int gWall = -1; bool gMover = false;
    bool canHook = findGrappleTarget(gPoint, gWall, gMover);
    grappleTargetInSight = canHook && gMover;
    if (pendingGrapple && modOn(DailyMod::GROUNDED)) pendingGrapple = false;   // DAILY: no grapple today
    if (pendingGrapple && pinned && !grapple.active) { pendingGrapple = false; pinnedCue(); }
    if (pinned && grapple.active) grapple.release();
    if (pendingGrapple) {
        if (!grapple.active) {
            if (canHook) {
                glm::vec3 impulse{0.f};
                grapple.attach(player.camera.position, gPoint, impulse, gWall,
                               gMover ? &level.walls[gWall].box : nullptr);
                player.velocity = impulse;
                viewModel.triggerGrapple();
                audio.play("grapple_fire");
                if (gMover) styleSystem.addStyle(4.f);
            }
        } else {
            grapple.release();
        }
        pendingGrapple = false;
    }
    grapple.update(dt, player.camera.position, player.velocity);

    // --- Aiming (rifles, RMB held) ---
    {
        const WeaponDef& d = weaponDef((WeaponId)activeWeapon);
        const WeaponState& ws = weapons[activeWeapon];
        bool rmb = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || gamepad::state().aim;
        bool want = d.canAim && rmb && pendingWeapon < 0 && !ws.reloading;
        float before = aim;
        if (g_devAim) want = d.canAim;
        if (want) aim = std::min(1.f, aim + dt / d.aimTime);
        else      aim = std::max(0.f, aim - dt / (d.canAim ? d.aimTime * 0.6f : 0.05f));
        if (before < 0.02f && aim >= 0.02f && d.scope) audio.play("scope", 70);
        if (before < 0.98f && aim >= 0.98f) aimFullAt = gameClock;
        player.horizontalSpeed = 7.f * glm::mix(1.f, d.aimMoveMult, aim);
    }

    // --- Player physics ---
    player.dynWalls = level.moverWalls.data();
    player.dynCount = (int)level.moverWalls.size();
    int boost = level.boosterAt(player.position);
    applyWater(player, level);
    if (!(g_devCam && g_devNoMouse)) {   // screenshot runs: the camera stays exactly where it was put
        player.update(dt, keys, level.walls.data(), (int)level.walls.size(),
                      grapple.active || dashMomentumTimer > 0.f || boost >= 0, &spatialGrid);
        if (boost >= 0) applyBooster(level.boosters[boost], player, dt);
        keepPlayerInZone(dt);
    }
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
    if (player.onGround && playerXZSpeed > 1.5f && !player.sliding) {
        footstepTimer -= dt;
        if (footstepTimer <= 0.f) {
            footstepTimer = glm::clamp(0.55f / (playerXZSpeed / 5.f), 0.2f, 0.5f);
            static const char* STEPS[] = {"step1", "step2", "step3", "step4"};
            if (player.wadeDepth >= 0.1f) audio.play("wade", 60);
            else audio.play(STEPS[rand() % 4], 55);
        }
    } else {
        footstepTimer = 0.f;
    }
    if (justLanded && !slamming) audio.play("land", 70);
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

    bool switchBlocked = (pendingWeapon >= 0);
    if (pendingFire && ws.ready() && !switchBlocked) fireWeapon(activeWeapon);
    pendingFire = false;
    if (pendingGrenade && grenadeCount > 0 && grenadeTimer <= 0.f) throwGrenade();
    pendingGrenade = false;

    if (recoilPitch > 0.f) {
        float applied = std::min(14.f * dt, recoilPitch);
        player.camera.pitch -= applied;
        recoilPitch -= applied;
        player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);
    }

    // --- Waves ---
    int alive = 0;
    for (auto& e : enemies) if (e.alive) ++alive;
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
    if (!act2Falling) director.update(dt, alive, player.position, spawns);   // ACT II: the fight waits for you to land
    for (auto& s : spawns) spawnEnemy(s.type, s.pos, s.hollow);

    // --- Enemies ---
    updateEnemies(dt);
    updateSanctumPhase(dt);
    updateSovereign(dt);
    updatePenitent(dt);
    if (fast() && countdown <= 0.f && !victory) ghostRec.record(elapsedTime, player.position, player.camera.yaw);

    // --- Projectiles ---
    projSystem.floorY = level.lowestFloor();
    auto result = projSystem.update(dt, level.walls.data(), (int)level.walls.size(),
                                    enemies, player.camera.position, &spatialGrid);

    bool parryPressed = parryKey && !parryPrev;
    parryPrev = parryKey;
    punchCooldown = std::max(0.f, punchCooldown - dt);
    if (parryPressed && punchCooldown <= 0.f) punch(result.boostableIndex);

    for (auto& [pi, ei] : result.enemyHits) {
        auto& e = enemies[ei];
        const Projectile& pr = projSystem.pool[pi];
        if (pr.parried && pr.heavy) {
            fx.spawnExplosionParticles(pr.position, 3.f);
            audio.playAt("explosion", pr.position, 100, SoundGroup::WORLD);
            shake(0.3f, 0.07f);
        }
        hurtEnemy(e, pr.damage, pr.position, pr.parried ? 25.f : 10.f, 2.f,
                  pr.parried ? StyleSource::PARRY : StyleSource::EXPLOSIVE, false, pr.parried);
    }
    // A Juggernaut's shell into one of its own
    for (auto& [pi, ei] : result.friendlyHits) {
        const Projectile& pr = projSystem.pool[pi];
        fx.spawnExplosionParticles(pr.position, 2.f);
        audio.playAt("explosion", pr.position, 80, SoundGroup::WORLD);
        hurtEnemy(enemies[ei], FRIENDLY_SHELL_DAMAGE, pr.position, 0.f, 0.f, StyleSource::FRIENDLY);
    }
    for (auto& exp : result.explosions)
        pendingBlasts.push_back({exp.pos, exp.radius, exp.damage, exp.radius * 0.5f, 20.f});
    processBlasts();

    if (result.hitPlayer) damagePlayer(result.playerDamage, result.playerHitFrom, 0.2f, 0.04f);

    updatePickups(dt);

    // --- FAST: the finish beacon ---
    if (finishOpen && !victory) {
        glm::vec3 d = player.position - level.finishPos;
        if (glm::length(glm::vec2(d.x, d.z)) < 3.f && std::fabs(d.y) < 4.f) {
            finishOpen = false;
            fx.spawnBurst(level.finishPos + glm::vec3{0, 1.f, 0}, {1.f, 0.6f, 0.2f}, 60, 10.f, 1.2f, -2.f);
            audio.play("wave", 128, SoundGroup::UI); audio.play("split", 128, SoundGroup::UI);
            finishRun();
        }
    }

    if (shakeTimer > 0.f) shakeTimer -= dt;
    if (ceilingFxTimer > 0.f) ceilingFxTimer -= dt;
    nearInteractable = findInteractTarget() >= 0;
    fx.tickMarks(dt);

    // Clear out dead enemies once nothing references them by index
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                  [](const Enemy& e){ return !e.alive; }), enemies.end());
    for (auto& t : pendingTwins) { enemies.push_back(t); enemies.back().uid = nextEnemyUid++; }
    pendingTwins.clear();
    linkConductors(enemies, player.position);   // tethers for the next tick (and this frame's beams)
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
    if (fast()) {
        for (int i = 0; i <= director.arena; ++i) addArena(i);
    } else {
        addArena(director.arena);
        if (director.phase == WaveDirector::Phase::CLEARED) {
            zones.push_back(&level.corridors[director.arena]);
            addArena(director.arena + 1);
        }
    }
    glm::vec3& p = player.position;
    float bestD = 1e9f; glm::vec2 best{p.x, p.z};
    const AABB* in = zones[0];
    for (auto* z : zones) {
        glm::vec2 c{glm::clamp(p.x, z->min.x, z->max.x), glm::clamp(p.z, z->min.z, z->max.z)};
        float d = glm::length(c - glm::vec2{p.x, p.z});
        // Inside several (a doorway where a tube meets a room): the higher ceiling wins
        if (d < bestD - 1e-4f || (d < 1e-4f && bestD < 1e-4f && z->max.y > in->max.y)) { bestD = d; best = c; in = z; }
    }
    if (bestD > 0.f) {
        if (best.x != p.x) player.velocity.x = 0.f;
        if (best.y != p.z) player.velocity.z = 0.f;
        p.x = best.x; p.z = best.y;
    }
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
    glm::vec3 origin = player.camera.position;
    glm::vec3 dir    = player.camera.forward();
    int   best    = -1;
    float bestT   = Interactable::INTERACT_RANGE;
    for (int i = 0; i < (int)interactables.size(); ++i) {
        if (!interactables[i].active) continue;
        float t = rayBoxHit(origin, dir, interactables[i].box);
        if (t > 0.f && t < bestT) { bestT = t; best = i; }
    }
    return best;
}
