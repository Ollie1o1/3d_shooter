#pragma once
// =============================================================================
// Gameplay_Flow.h — GameplayState: The run: building the state, entering and retrying arenas, director
// events and title cards, splits, finishing a run, the Sanctum's second
// phase, the per-frame update and the soundtrack that follows the fight.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::applyTextureQuality() {
    int q = settings ? settings->quality : GameSettings::QUALITY_DEFAULT;
    if (q == textureQuality) return;
    textureQuality = q;
    for (GLuint t : worldTex) if (t) TextureGen::setAnisotropy(t, GameSettings::qualityAnisotropy(q));
}

inline GameplayState::GameplayState(AudioSystem& aud, GameSettings* s, GameMode m)
    : settings(s), mode(m), audio(aud) {
    worldShader.loadFiles("src/shader.vert","src/shader.frag");
    skyboxShader.loadFiles("src/skybox.vert","src/skybox.frag");
    tracerShader.loadFiles("src/tracer.vert","src/tracer.frag");
    particleShader.loadFiles("src/particle.vert","src/particle.frag");

    // Effects::Tracer VBO: 4 floats per vertex (xyz + alpha), dynamic
    glGenVertexArrays(1, &tracerVAO);
    glGenBuffers(1, &tracerVBO);
    glBindVertexArray(tracerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
    glBufferData(GL_ARRAY_BUFFER, Effects::MAX_TRACERS * 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(3*sizeof(float)));
    glBindVertexArray(0);

    // Effects::Particle VBO: [x, y, z, r, g, b, alpha] per point
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, Effects::MAX_PARTICLES * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(6*sizeof(float)));
    glBindVertexArray(0);

    // Effects::Decal VBO: pos(3)+uv(2)+normal(3)+color(3) = 11 floats, matches worldShader
    glGenVertexArrays(1, &decalVAO);
    glGenBuffers(1, &decalVBO);
    glBindVertexArray(decalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
    glBufferData(GL_ARRAY_BUFFER, Effects::MAX_DECALS * 6 * 11 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(5*sizeof(float)));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(8*sizeof(float)));
    glBindVertexArray(0);

    whiteTex = makeGreyTexture();
    worldTex[TEX_GRID]     = TextureGen::generateGridFloor(128);
    worldTex[TEX_BRICK]    = TextureGen::generateBrickWall(128);
    worldTex[TEX_METAL]    = TextureGen::generateMetalCeiling(128);
    worldTex[TEX_PANEL]    = TextureGen::generatePanel(128);
    worldTex[TEX_ROCK]     = TextureGen::generateRock(128);
    worldTex[TEX_CONCRETE] = TextureGen::generateConcrete(128);

    level = fast() ? buildGauntlet() : buildLevel();
    spatialGrid.build(level.walls);
    world = buildWorldMeshes(level);
    shifts.capture(level);
    director.level = &level;
    director.fast  = fast();
    records.load();

    settingsMenu.s = settings;
    settingsMenu.onBack = [this]() { pauseSettings = false; };

    for (int i=0;i<MAX_POINT_LIGHTS;++i) {
        pointLightPos[i]   = {0,0,0};
        pointLightColor[i] = {0,0,0};
    }

    if (endless()) setupEndless();
    int start = endless() ? endlessArena : glm::clamp(g_startArena, 0, (int)level.arenas.size() - 1);
    ranked = (start == 0 || endless()) && g_startWave <= 0 && !g_godMode && !g_practice && !g_devCam;
    enterArena(start);
    director.wave = glm::clamp(g_startWave, 0, director.waveCount() - 1);
    if (fast()) ghost.load();
    if (fast() && start == 0) { countdown = 3.f; pushBanner("THE GAUNTLET", "SEVEN ROOMS - THEN REACH THE BEACON ON THE TOWER", {1.f, 0.6f, 0.2f}, 3.f); }
    if (g_devCam) countdown = 0.f;
    if (g_devWeapon >= 0) activeWeapon = g_devWeapon % WEAPON_COUNT;
    if (g_devAim) aim = 1.f;
    if (g_devOverlay == "armory") { prog.points = 3; prog.up[2].tier[0] = 2; prog.up[3].mod = true; armoryOpen = true; armoryW = 2; }
    if (g_devOverlay == "pause") paused = true;
    if (g_devOverlay == "victory") {   // the victory screen mid name entry (screenshots)
        elapsedTime = 754.3f; victory = true; ranked = true;
        bankedStyle = 6240.f; bankedDamage = 410.f; wavesCleared = endless() ? 14 : 0; totalKills = 187;
        board.load(); nameEntry = true; nameBuf = "OLLIE";
    }
    for (int k = 0; k < (int)g_devSpawns.size(); ++k) {
        float a = (k - (g_devSpawns.size() - 1) * 0.5f) * 0.4f, dist = 9.f + (k % 2) * 3.f;
        glm::vec3 f = player.camera.flatForward(), r{-f.z, 0.f, f.x};
        glm::vec3 at = player.position * glm::vec3{1, 0, 1} + (f * std::cos(a) + r * std::sin(a)) * dist;
        spawnEnemy((EnemyType)g_devSpawns[k], at + glm::vec3{0, groundHeightAt(at.x, at.z, player.position.y + 1.f), 0});
    }
    if (g_devOverlay == "settings") { paused = true; pauseSettings = true; }

    prevTicks = SDL_GetPerformanceCounter();
    freq      = SDL_GetPerformanceFrequency();
    prevCamPos = player.camera.position;

    captureMouse(true);
}

inline GameplayState::~GameplayState() {
    glDeleteTextures(1,&whiteTex);
    glDeleteTextures(TEX_COUNT, worldTex);
    if (tracerVAO)   glDeleteVertexArrays(1, &tracerVAO);
    if (tracerVBO)   glDeleteBuffers(1, &tracerVBO);
    if (particleVAO) glDeleteVertexArrays(1, &particleVAO);
    if (particleVBO) glDeleteBuffers(1, &particleVBO);
    if (decalVAO)    glDeleteVertexArrays(1, &decalVAO);
    if (decalVBO)    glDeleteBuffers(1, &decalVBO);
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

inline void GameplayState::captureMouse(bool on) {
    if (g_devNoMouse) on = false;
    SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE);
    if (on) mouseFilter.onCapture();
}

inline void GameplayState::enterArena(int a) {
    // Every door shut; exits of the arenas already beaten unlocked. The way
    // in locks when the fight starts (ARENA_START).
    for (int d = 0; d < (int)level.doors.size(); ++d) { level.doors[d].locked = false; level.setDoorInstant(d, false); }
    shifts.reset(level);   // the sun back up, the lava back down, the platforms back to speed
    level.updateMovers(moverClock);
    for (int i = 0; i < (int)level.arenas.size(); ++i) {
        const Arena& ar = level.arenas[i];
        if (ar.exitDoor >= 0) level.doors[ar.exitDoor].locked = i >= a;
    }
    resetPlayer(level.arenas[a].playerStart);
    player.camera.yaw = level.arenas[a].startYaw;
    if (g_devCam) {
        player.position = g_devCamPos - glm::vec3{0, player.eyeHeight, 0};
        player.camera.position = g_devCamPos;
        player.camera.yaw = g_devCamYaw; player.camera.pitch = g_devCamPitch;
        prevCamPos = g_devCamPos;
    }
    enemies.clear();
    for (auto& p : projSystem.pool) p.alive = false;
    fx.clear(); pickups.clear(); pendingBlasts.clear();
    sov.clear(); lastStand = false; lastStandT = 0.f;
    banners.clear();
    grapple.release();
    playerDead = false; deadTimer = 0.f;
    victory = false; victoryDelay = -1.f;
    finishOpen = false;
    // Health and XP placed in the level's breathers
    for (auto& pp : level.placedPickups) {
        PickupKind k = pp.kind == 1 ? PickupKind::POTION : pp.kind == 2 ? PickupKind::XP : PickupKind::ORB;
        float fy = groundHeightAt(pp.pos.x, pp.pos.z, pp.pos.y + 0.5f);
        pickups.push_back({pp.pos + glm::vec3{0, 0.6f, 0}, glm::vec3{0.f}, 1e9f, k, fy, pp.pos + glm::vec3{0, 0.6f, 0}});
    }
    // Splits after this section belong to a run we're redoing
    if (fast() && (int)splits.size() > a) splits.resize(a);
    if (fast()) director.approach(a);   // the fight starts at the section's trigger
    else director.startArena(a);
}

inline void GameplayState::resetPlayer(glm::vec3 start) {
    player = Player{start};
    player.camera.aspectRatio = (float)SCREEN_W/SCREEN_H;
    player.camera.fov = settings ? settings->fov : 90.f;
    prevCamPos = player.camera.position;
    bankedStyle += styleSystem.earned; bankedDamage += styleSystem.damageTaken;   // the score survives a retry
    styleSystem = StyleSystem{};
    grenadeCount = grenadeMax; killsThisCycle = 0;
    for (int w = 0; w < WEAPON_COUNT; ++w) {
        weapons[w] = WeaponState{};
        weapons[w].ammo = weaponMag((WeaponId)w, prog.up[w]);
    }
    aim = 0.f; aimFullAt = -1.f; boltSoundTimer = -1.f;
    dashCharges = 2; dashCooldown = 0.f; dashMomentumTimer = 0.f;
    jumpsRemaining = 2; slamming = false; invincFrames = 0.f;
    activeWeapon = modOn(DailyMod::MARKSMAN) ? (int)WeaponId::KAR : 0; pendingWeapon = -1; weaponSwitchTimer = 0.f;
    recoilPitch = 0.f; peakFallSpeed = 0.f; landSquash = 0.f; fovKick = 0.f;
    paused = false; pauseSettings = false; pauseSelected = 0; armoryOpen = false;
    ui.clearIndicators();
}

inline void GameplayState::restartHere() {
    enterArena(director.arena + (!fast() && director.phase == WaveDirector::Phase::CLEARED ? 1 : 0));
    captureMouse(true);
}

inline void GameplayState::retryArena() {
    ++deaths;
    if (fast()) { enterArena(director.arena); captureMouse(true); return; }
    // Died on the way out of a cleared arena? Pick up at the next one.
    bool cleared = director.phase == WaveDirector::Phase::CLEARED;
    enterArena(director.arena + (cleared ? 1 : 0));
    captureMouse(true);
}

inline void GameplayState::newRun() {
    director = WaveDirector{};
    director.level = &level;
    director.fast  = fast();
    prog = Progression{};
    totalKills = totalShots = totalHits = deaths = 0;
    elapsedTime = 0.f; peakStyle = 0.f;
    controlHintTimer = 10.f;
    splits.clear();
    newRecord = false;
    ranked = !g_godMode && !g_practice;
    nameEntry = false; boardPlace = -1;
    ghostRec.pts.clear();
    if (fast()) ghost.load();
    if (endless()) setupEndless();
    enterArena(endless() ? endlessArena : 0);
    bankedStyle = bankedDamage = 0.f;   // after enterArena: that banked the last run's
    wavesCleared = 0;
    if (fast()) { countdown = 3.f; pushBanner("THE GAUNTLET", "SEVEN ROOMS - THEN REACH THE BEACON ON THE TOWER", {1.f, 0.6f, 0.2f}, 3.f); }
    captureMouse(true);
}

inline void GameplayState::pushBanner(const std::string& title, const std::string& sub, glm::vec3 col, float dur) {
    banners.push_back({title, sub, col, 0.f, dur});
}

inline void GameplayState::handleDirectorEvents() {
    for (auto& ev : director.events) {
        const Arena& ar = level.arenas[director.arena];
        char buf[128];
        switch (ev.kind) {
        case DirectorEvent::ARENA_START:
            lockDoor(ar.entryGate, true);   // no way back out mid-fight
            if (fast()) {
                snprintf(buf, sizeof(buf), "ROOM %d/%d  %s", ev.value + 1, (int)level.arenas.size(), ar.name);
                pushBanner(buf, ar.subtitle, {1.f, 0.7f, 0.3f}, 1.8f);
                audio.play("wave", 90);
            } else if (endless()) {
                if (dailyRun()) pushBanner("DAILY  " + daily.label(), std::string(daily.modName()) + " - " + daily.modHint(),
                                           {0.4f, 0.8f, 1.f}, 3.4f);
                else pushBanner(std::string("ENDLESS  ") + ar.name, "HOW LONG CAN YOU LAST", {1.f, 0.3f, 0.45f}, 3.f);
                audio.play("wave");
            } else {
                snprintf(buf, sizeof(buf), "ARENA %d/%d", ev.value + 1, (int)level.arenas.size());
                pushBanner(std::string(buf) + "  " + ar.name, ar.subtitle, {1.f, 0.78f, 0.3f}, 2.6f);
                audio.play("wave");
            }
            break;
        case DirectorEvent::WAVE_START:
            if (endless()) shifts.onWave(level, director.arena, ev.value % 3, director.goal(), moverClock, 3);
            else shifts.onWave(level, director.arena, ev.value, director.goal(), moverClock);
            if (ar.shift == ArenaShift::SPEED_UP && ev.value > 0) ui.feed("THE PLATFORMS SPEED UP", {0.4f, 0.9f, 1.f});
            if (fast()) { if (ev.value > 0) pushBanner("SECOND WAVE", "", {1.f, 0.5f, 0.3f}, 1.4f); break; }
            if (endless()) {
                level.arenas[director.arena].maxAlive = endlessBaseAlive + std::min(6, ev.value / 3);   // more at once as it goes on
                snprintf(buf, sizeof(buf), "WAVE %d", ev.value + 1);
            } else snprintf(buf, sizeof(buf), "WAVE %d/%d", ev.value + 1, director.waveCount());
            pushBanner(buf, director.goal().label, {1.f, 0.9f, 0.4f}, director.hasGoal() ? 2.6f : 1.8f);
            audio.play("wave", 90);
            break;
        case DirectorEvent::BOSS_START:
            if (ar.waves[ev.value][0].type == EnemyType::SOVEREIGN)
                pushBanner("THE SOVEREIGN", "PARRY (F) HIS BLADE AS IT FALLS", {1.f, 0.3f, 0.2f}, 4.f);
            else
                pushBanner("THE WARDEN", "DODGE THE VOLLEYS, JUMP THE SLAMS", {1.f, 0.2f, 0.65f}, 3.5f);
            audio.play("wave"); audio.play("explosion", 70);
            shake(0.6f, 0.06f);
            break;
        case DirectorEvent::NEW_TYPE: {
            EnemyType t = (EnemyType)ev.value;
            if (isBoss(t)) break;
            if (fast()) ui.feed(std::string("NEW: ") + statsOf(t).name, statsOf(t).glow);
            else pushBanner(std::string("NEW: ") + statsOf(t).name, statsOf(t).hint, statsOf(t).glow, 3.4f);
            break;
        }
        case DirectorEvent::WAVE_CLEARED:
            if (endless()) ++wavesCleared;
            shifts.onWaveOver();
            if (!fast()) pushBanner("WAVE CLEAR", "", {0.4f, 1.f, 0.6f}, 1.6f);
            styleSystem.heal(10.f);
            break;
        case DirectorEvent::ARENA_CLEARED: {
            const Arena& done = level.arenas[ev.value];
            lockDoor(done.exitDoor, false);
            lockDoor(done.entryGate, false);
            if (fast()) {
                recordSplit(ev.value);
                styleSystem.heal(25.f);
                grenadeCount = grenadeMax;
                audio.play("split");
                break;
            }
            if (done.exitDoor >= 0)
                pushBanner("ARENA CLEARED", "THE GATE IS OPEN - HEAD NORTH", {0.4f, 1.f, 0.6f}, 3.5f);
            styleSystem.heal(40.f);
            grenadeCount = grenadeMax;
            audio.play("wave");
            break;
        }
        case DirectorEvent::GOAL_DONE:
            shifts.onWaveOver();
            // The objective's met: whatever's left of the wave falls apart
            for (auto& e : enemies)
                if (e.alive) {
                    e.alive = false; e.state = EnemyState::DEAD;
                    spawnDebrisFor(e);
                    fx.spawnDeathParticles(e.position + glm::vec3{0, e.height() * 0.5f, 0}, e.stats().color);
                }
            pushBanner("OBJECTIVE COMPLETE", "", {0.4f, 1.f, 0.6f}, 1.8f);
            styleSystem.addStyle(30.f);
            gainXp(40);
            ui.feed("OBJECTIVE  +40 XP", {0.4f, 1.f, 0.6f});
            audio.play("wave");
            shake(0.3f, 0.05f);
            break;
        case DirectorEvent::VICTORY:
            victoryDelay = 2.5f;
            break;
        case DirectorEvent::FINISH_OPEN:
            finishOpen = true;
            pushBanner("FINISH OPEN", "REACH THE BEACON", {1.f, 0.6f, 0.2f}, 2.f);
            audio.play("wave");
            break;
        }
    }
    director.events.clear();
}

inline void GameplayState::recordSplit(int section) {
    if ((int)splits.size() > section) splits.resize(section);
    splits.push_back(elapsedTime);
    char buf[96];
    if (section < (int)records.fastSplits.size()) {
        float d = elapsedTime - records.fastSplits[section];
        snprintf(buf, sizeof(buf), "ROOM %d  %s  %+.2f", section + 1, formatTime(elapsedTime).c_str(), d);
        ui.showSplit(buf, d <= 0.f ? glm::vec3{0.3f, 1.f, 0.5f} : glm::vec3{1.f, 0.4f, 0.35f});
    } else {
        snprintf(buf, sizeof(buf), "ROOM %d  %s", section + 1, formatTime(elapsedTime).c_str());
        ui.showSplit(buf, {1.f, 0.9f, 0.6f});
    }
}

inline void GameplayState::finishRun() {
    victory = true;
    captureMouse(false);
    newRecord = false;
    if (!ranked) return;
    board.daily = worldBoard.daily = daily.key();
    board.load();
    worldLoaded = worldBoard.loadOnline();
    // Asked for a name when the run makes this browser's board, or (on the
    // web, with the shared board up) that one
    Board bd = boardFor();
    Leaderboard::Entry me = runEntry();
    bool places = board.placeFor(bd, me) >= 0 || (worldLoaded && worldBoard.placeFor(bd, me) >= 0);
    if (places) { nameEntry = true; nameBuf = board.lastName; }
    if (fast()) {
        newRecord = records.bestFast <= 0.f || elapsedTime < records.bestFast;
        if (newRecord) {
            records.bestFast = elapsedTime; records.fastSplits = splits;
            ghostRec.save(); ghost = ghostRec;
        }
    } else if (endless()) {
        int& best = dailyRun() ? records.bestDaily : records.bestEndless;
        if (dailyRun() && records.dailyDate != daily.date) { records.dailyDate = daily.date; best = 0; }
        newRecord = me.score > best;
        if (newRecord) best = me.score;
        if (dailyRun()) records.save();   // today's date, best or not
    } else {
        newRecord = records.bestArena <= 0.f || elapsedTime < records.bestArena;
        if (newRecord) records.bestArena = elapsedTime;
        int sc = me.score;
        if (sc > records.bestArenaScore) { records.bestArenaScore = sc; newRecord = true; }
    }
    if (newRecord) records.save();
}

inline RunScore GameplayState::runScore() const {
    int diff = settings ? settings->difficulty : DIFFICULTY_DEFAULT;
    float style = bankedStyle + styleSystem.earned;
    if (endless()) return endlessScore(style, wavesCleared, diff);
    return arenaScore(style, elapsedTime, bankedDamage + styleSystem.damageTaken, diff);
}

inline Leaderboard::Entry GameplayState::runEntry() const {
    Leaderboard::Entry e;
    e.time = elapsedTime;
    e.difficulty = settings ? settings->difficulty : DIFFICULTY_DEFAULT;
    e.score = fast() ? 0 : runScore().total;
    e.wave = endless() ? wavesCleared : 0;
    return e;
}

// ---- ENDLESS / DAILY --------------------------------------------------------
// The run's arena gets generated waves instead of its own: one is always
// queued ahead of the director, so it never runs out and never "clears" the
// arena (its exit stays shut). The arena's own objectives come back as every
// 4th wave (EndlessWaves.h).
inline void GameplayState::setupEndless() {
    daily = DailyInfo::today();
    endlessArena = dailyRun() ? daily.arena : 3;
    static LevelData built = buildLevel();   // the arenas' own objectives, before any run touched them
    Arena& a = level.arenas[endlessArena];
    endlessBaseAlive = built.arenas[endlessArena].maxAlive;
    uint32_t seed = dailyRun() ? daily.seed : (uint32_t)SDL_GetPerformanceCounter();
    gen.budgetScale = modOn(DailyMod::SWARM) ? 1.5f : 1.f;
    gen.begin(seed, built.arenas[endlessArena].goals, endlessArena == 3);
    a.waves.clear(); a.goals.clear();
    feedEndless();
    board.daily = worldBoard.daily = daily.key();
}

inline void GameplayState::feedEndless() {
    Arena& a = level.arenas[endlessArena];
    while ((int)a.waves.size() < director.wave + 2) {
        auto w = gen.wave((int)a.waves.size());
        a.waves.push_back(w.first);
        a.goals.push_back(w.second);
    }
}

inline float GameplayState::endlessToughness() const {
    if (!endless()) return 1.f;
    float t = 1.f + 0.03f * director.wave;   // each wave a little tougher
    return modOn(DailyMod::SWARM) ? t * 0.6f : t;
}

inline Theme GameplayState::bloodEclipse(const Theme& t) {
    Theme o = t;
    o.zenith = {0.05f, 0.0f, 0.0f}; o.horizon = {0.55f, 0.03f, 0.02f}; o.ground = {0.06f, 0.0f, 0.0f};
    o.sunColor = {2.2f, 0.45f, 0.25f}; o.mountain = {0.1f, 0.01f, 0.01f};
    o.lightColor = {1.3f, 0.5f, 0.4f}; o.skyAmb = {0.34f, 0.08f, 0.07f};
    o.fogColor = {0.22f, 0.02f, 0.02f}; o.fogDensity = t.fogDensity * 1.6f;
    return o;
}

inline void GameplayState::updateSanctumPhase(float dt) {
    bool raged = false;
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::SOVEREIGN && e.enraged) raged = true;
    rageBlend = glm::clamp(rageBlend + (raged ? dt / 2.f : -dt / 3.f), 0.f, 1.f);
    if (raged == sanctumRaged) return;
    // Speed the orbiting platforms up (or back down after a retry) without
    // a jump: keep each one's current point on its path (offsetAt uses
    // t / period + phase)
    sanctumRaged = raged;
    const Arena& ar = level.arenas[director.arena];
    for (auto& m : level.movers) {
        if (m.path != Mover::Path::ORBIT) continue;
        glm::vec3 c = (m.base.min + m.base.max) * 0.5f;
        if (c.x < ar.zone.min.x || c.x > ar.zone.max.x || c.z < ar.zone.min.z || c.z > ar.zone.max.z) continue;
        float newPeriod = raged ? m.period * 0.55f : m.period / 0.55f;
        m.phase += moverClock / m.period - moverClock / newPeriod;
        m.period = newPeriod;
    }
}

inline void GameplayState::shake(float t, float amount) {
    float s = settings ? settings->screenShake : 1.f;
    if (s <= 0.f) return;
    shakeTimer = std::max(shakeTimer, t);
    shakeIntensity = std::max(shakeIntensity, amount * s);
}

inline void GameplayState::update(float dt) {
    if (settings) audio.masterVolume = settings->audioVolume;
    updateMusic();

    // Banners keep animating on the death/victory screens, not while paused
    bool frozen = paused || armoryOpen;
    if (!banners.empty() && !frozen) {
        banners.front().time += dt;
        if (banners.front().time >= banners.front().duration) banners.pop_front();
    }
    if (!frozen) ui.update(dt, styleSystem);
    if (frozen || playerDead || victory || countdown > 0.f) {
        // Keep the physics clock current so resuming doesn't replay the
        // whole paused interval as a burst of catch-up ticks.
        prevTicks   = SDL_GetPerformanceCounter();
        accumulator = 0.0;
        if (playerDead) deadTimer += dt;
        if (countdown > 0.f && !frozen) {
            float before = countdown;
            countdown -= dt;
            if (std::ceil(before) != std::ceil(countdown)) audio.play(countdown <= 0.f ? "wave" : "telegraph", 90);
            if (countdown <= 0.f) countdown = 0.f;
            handleDirectorEvents();
        }
        if (playerDead) gameClock += dt;
        // ENDLESS: the death screen hands over to the run's score
        if (playerDead && victoryDelay > 0.f && !frozen) {
            victoryDelay -= dt;
            if (victoryDelay <= 0.f) finishRun();
        }
        return;
    }

    Uint64 now = SDL_GetPerformanceCounter();
    double elapsed = g_fixedDt > 0.0 ? g_fixedDt : (double)(now - prevTicks) / (double)freq;
    prevTicks = now;
    if (g_devCam && g_devNoMouse) devCamera((float)elapsed);
    accumulator += elapsed;
    if (accumulator > 0.25) accumulator = 0.25;

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    // A controller holds keys too (Gamepad.h): fold it in
    static Uint8 merged[SDL_NUM_SCANCODES];
    gamepad::poll();
    if (gamepad::state().pad) {
        const gamepad::State& gp = gamepad::state();
        for (int i = 0; i < SDL_NUM_SCANCODES; ++i) merged[i] = keys[i] | gp.held[i];
        keys = merged;
        if (gp.fire && !gp.firePrev) pendingFire = true;
        if (gp.look.x != 0.f || gp.look.y != 0.f) {
            // Full deflection turns 240 degrees a second at the default sensitivity
            float speed = 240.f * (settings ? settings->sensitivity / 0.1f : 1.f) * (float)elapsed;
            float dy = gp.look.y * speed;
            if (settings && settings->invertY) dy = -dy;
            player.applyMouseLook(gp.look.x * speed, dy, aimSensScale());
        }
    }
    static Uint8 noKeys[SDL_NUM_SCANCODES] = {};
    if (g_devNoMouse) keys = noKeys;   // screenshot runs: the real keyboard doesn't reach the game
    bool parryKey = keys[SDL_SCANCODE_F] != 0;

    while (accumulator >= PHYSICS_DT) {
        prevCamPos = player.camera.position;
        snapshotForInterpolation();
        if (hitStopFrames > 0) {
            --hitStopFrames;  // freeze simulation, consume one tick
        } else {
            physicsTick(PHYSICS_DT, keys, parryKey);
        }
        accumulator -= PHYSICS_DT;
    }

    float floatDt = (float)elapsed;
    gameClock += floatDt;
    styleSystem.update(floatDt);
    if (controlHintTimer > 0.f) controlHintTimer -= floatDt;

    // Doors part as you run at them (unless locked) and shut behind you
    level.updateDoors(floatDt, player.position, [this](int di, bool opening) {
        const AABB& c = level.doors[di].closed;
        float d = glm::length((c.min + c.max) * 0.5f - player.camera.position);
        audio.play(opening ? "door" : "door_close", (int)glm::clamp(120.f - d * 3.f, 20.f, 120.f));
    });

    viewModel.update(floatDt, playerXZSpeed, player.onGround);
    // --overlay reloadNN: freeze the gun NN% through its reload (screenshots)
    if (g_devOverlay.rfind("reload", 0) == 0) {
        float u = std::atoi(g_devOverlay.c_str() + 6) / 100.f;
        viewModel.anim = ViewAnim::RELOAD; viewModel.animMax = 1.f; viewModel.animTimer = std::max(0.001f, 1.f - u);
        viewModel.reloadShells = 2;
    }
    if (g_devOverlay.rfind("inspect", 0) == 0) {   // --overlay inspectNN: frozen NN% through it
        float u = std::atoi(g_devOverlay.c_str() + 7) / 100.f;
        viewModel.anim = ViewAnim::INSPECT; viewModel.animMax = 2.2f; viewModel.animTimer = std::max(0.001f, (1.f - u) * 2.2f);
    }
    if (g_devOverlay == "punch") { viewModel.parryTimer = ViewModel::PARRY_TIME * 0.62f; viewModel.parryHit = true; }
    // Baseline FOV widens with horizontal speed on top of the dash kick
    float speedKick = glm::clamp((playerXZSpeed - 7.f) / 15.f, 0.f, 1.f) * 6.f;
    fovKick = glm::mix(fovKick, speedKick, std::min(1.f, floatDt * 7.f));
    landSquash = glm::mix(landSquash, 0.f, std::min(1.f, floatDt * 10.f));

    if (pendingWeapon >= 0) {
        weaponSwitchTimer -= floatDt;
        if (weaponSwitchTimer <= 0.f) {
            activeWeapon  = pendingWeapon;
            pendingWeapon = -1;
        }
    }
    if (boltSoundTimer > 0.f) {
        boltSoundTimer -= floatDt;
        if (boltSoundTimer <= 0.f) audio.play("bolt", 110);
    }

    elapsedTime += floatDt;
    if (styleSystem.style > peakStyle) peakStyle = styleSystem.style;

    fx.updateParticles(floatDt);
    fx.spawnAmbientParticles(level, player.position, fast(), floatDt);
    for (auto& p : projSystem.pool) {           // projectile trails
        if (!p.alive) continue;
        Effects::Particle* q = fx.freeParticle();
        if (!q) break;
        q->pos = p.position; q->vel = -p.velocity * 0.05f;
        q->color = p.emissiveColor * 0.8f; q->gravity = 0.f;
        q->maxLife = q->life = 0.22f; q->alive = true;
    }
    fx.updateDebris(floatDt);
    fx.updateRings(floatDt);

    // Point light 0: explosion or muzzle flash. 1..3: nearest enemy shots.
    if (explosionFlashTimer > 0.f) {
        explosionFlashTimer -= floatDt;
        float t = glm::clamp(explosionFlashTimer / 0.35f, 0.f, 1.f);
        pointLightPos[0]   = explosionFlashPos;
        pointLightColor[0] = glm::vec3{1.6f, 0.9f, 0.4f} * t;
    } else if (muzzleFlashTimer > 0.f) {
        muzzleFlashTimer -= floatDt;
        pointLightPos[0]   = muzzleFlashPos;
        pointLightColor[0] = {1.f,0.7f,0.2f};
    } else {
        pointLightColor[0] = {0,0,0};
    }
    int lIdx = 1;
    for (auto& p : projSystem.pool) {
        if (!p.alive || p.isPlayer) continue;
        if (lIdx >= MAX_POINT_LIGHTS) break;
        pointLightPos[lIdx]   = p.position;
        pointLightColor[lIdx] = p.emissiveColor * 2.f;
        ++lIdx;
    }
    for (;lIdx<MAX_POINT_LIGHTS;++lIdx) pointLightColor[lIdx]={0,0,0};

    fpsFrameCount++;
    fpsTimer += floatDt;
    if (fpsTimer >= 1.0f) {
        ui.currentFPS   = fpsFrameCount;
        fpsFrameCount   = 0;
        fpsTimer        = 0.f;
    }
    ui.showFPS = settings ? settings->showFPS : false;

    handleDirectorEvents();

    if (victoryDelay > 0.f) {
        victoryDelay -= floatDt;
        if (victoryDelay <= 0.f) finishRun();
    }
    if (!styleSystem.isAlive() && !playerDead) {
        playerDead = true;
        deadTimer = 0.f;
        grapple.release();
        if (endless()) victoryDelay = 2.4f;   // no retries: YOU DIED, then on to the run's score
    }
}

inline void GameplayState::snapshotForInterpolation() {
    for (auto& e : enemies) { e.prevPosition = e.position; e.prevYaw = e.yaw; }
    for (auto& p : projSystem.pool) if (p.alive) p.prevPosition = p.position;
    for (auto& p : pickups) p.prev = p.pos;
    for (auto& m : level.movers) m.delta = glm::vec3{0.f};
}

inline void GameplayState::updateMusic() {
    auto& m = audio.music;
    audio.setMusicVolume(settings ? settings->musicVolume : 0.6f);
    static const int ARENA_TRACK[] = {0, 1, 2, 3, 4};       // Yard, Foundry, Spire, Core, Sanctum
    static const int FAST_TRACK[]  = {0, 1, 2, 2, 1, 1, 3}; // Canal .. Tower
    int a = director.arena;
    m.setTrack(fast() ? FAST_TRACK[a % 7] : ARENA_TRACK[a % 5]);
    float lv = 0.6f;
    switch (director.phase) {
        case WaveDirector::Phase::ACTIVE:   lv = director.bossWave() ? 1.35f : 1.f; break;
        case WaveDirector::Phase::BREAK:    lv = 0.7f; break;
        case WaveDirector::Phase::CLEARED:  lv = 0.5f; break;
        case WaveDirector::Phase::VICTORY:  lv = fast() ? 1.2f : 0.55f; break;
        default: break;
    }
    if (lv >= 1.f && styleSystem.overdrive) lv = 1.35f;
    if (countdown > 0.f) lv = 0.45f;
    if (victory) lv = 0.55f;
    if (playerDead) lv = 0.1f;
    m.setIntensity(lv);
    m.setMuffle(paused || armoryOpen || playerDead);
}
