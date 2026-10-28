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
    worldShader.loadFiles("src/shaders/shader.vert","src/shaders/shader.frag");
    contactShader.loadFiles("src/shaders/contact.vert","src/shaders/contact.frag");
    skyboxShader.loadFiles("src/shaders/skybox.vert","src/shaders/skybox.frag");
    tracerShader.loadFiles("src/shaders/tracer.vert","src/shaders/tracer.frag");
    particleShader.loadFiles("src/shaders/particle.vert","src/shaders/particle.frag");
    waterShader.loadFiles("src/shaders/water.vert","src/shaders/water.frag");

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

    // Water VBO: a quad (two triangles, xyz) per volume, rebuilt each frame
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_WATER * 6 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
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
    worldTex[TEX_GRID]          = TextureGen::uploadMaterial(MaterialTex::gridFloor());
    worldTex[TEX_BRICK]         = TextureGen::uploadMaterial(MaterialTex::brick());
    worldTex[TEX_METAL]         = TextureGen::uploadMaterial(MaterialTex::metal());
    worldTex[TEX_PANEL]         = TextureGen::uploadMaterial(MaterialTex::panel());
    worldTex[TEX_ROCK]          = TextureGen::uploadMaterial(MaterialTex::rock());
    worldTex[TEX_CONCRETE]      = TextureGen::uploadMaterial(MaterialTex::concrete());
    worldTex[TEX_LIMESTONE]     = TextureGen::uploadMaterial(MaterialTex::limestone());
    worldTex[TEX_BRASS]         = TextureGen::uploadMaterial(MaterialTex::brass());
    worldTex[TEX_ADOBE]         = TextureGen::uploadMaterial(MaterialTex::adobe());
    worldTex[TEX_TERRACOTTA]    = TextureGen::uploadMaterial(MaterialTex::terracotta());
    worldTex[TEX_WOOD]          = TextureGen::uploadMaterial(MaterialTex::wood());
    worldTex[TEX_PLATE]         = TextureGen::uploadMaterial(MaterialTex::plate());
    worldTex[TEX_TREAD]         = TextureGen::uploadMaterial(MaterialTex::tread());
    worldTex[TEX_MARBLE]        = TextureGen::uploadMaterial(MaterialTex::marble());
    worldTex[TEX_FLAG]          = TextureGen::uploadMaterial(MaterialTex::flagstone());
    worldTex[TEX_CIRCUIT]       = TextureGen::uploadMaterial(MaterialTex::circuit());
    worldTex[TEX_CIRCUIT_FLOOR] = TextureGen::uploadMaterial(MaterialTex::circuitFloor());
    worldTex[TEX_BASALT]        = TextureGen::uploadMaterial(MaterialTex::basalt());

    level = fast() ? buildGauntlet() : act2() ? buildAct2Level() : buildLevel();
    spatialGrid.build(level.walls);
    world = buildWorldMeshes(level);
    neonLights = buildNeonLights(level);
    shifts.capture(level);
    anchorsBuilt = level.anchors;   // the Penitent's anchors as built: a retry restores them
    anchorBoxes.clear();
    for (auto& a : level.anchors) anchorBoxes.push_back(level.walls[a.wall].box);
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
    // ACT II is a preview: unranked until the whole act exists
    ranked = (start == 0 || endless()) && g_startWave <= 0 && !g_godMode && !g_practice && !g_devCam && !act2();
    act2FromStart = act2() && start == 0 && g_startWave <= 0 && !g_devCam;
    if (g_bossRun >= 0) { start = bossEntry(g_bossRun).arena; ranked = act2FromStart = false; }
    if (roomAttack()) { start = glm::clamp(g_roomAttack, 0, (int)level.arenas.size() - 1); ranked = false; }
    if (g_bossRun >= 0) msCheckpoint = bossEntry(g_bossRun).morning;   // (MORNINGSTAR: from inside the eclipse)
    enterArena(start);
    if (g_bossRun < 0) director.wave = glm::clamp(g_startWave, 0, director.waveCount() - 1);
    if (fast()) ghost.load();
    if (fast() && start == 0 && !roomAttack()) { countdown = 3.f; pushBanner("THE GAUNTLET", "SEVEN ROOMS - THEN REACH THE BEACON ON THE TOWER", {1.f, 0.6f, 0.2f}, 3.f); }
    if (roomAttack()) startRoomAttack();
    if (g_devCam) countdown = 0.f;
    if (g_devWeapon >= 0 && g_devWeapon < WEAPON_COUNT) activeWeapon = g_devWeapon;
    if (g_devWeapon == WEAPON_COUNT) drawSword();   // --weapon 5: the sword in hand (where it's allowed)
    if (g_devOverlay == "guard" && swordOut) { sword.raised = true; sword.raisedFor = 1.f; sword.guard = 55.f; }   // (screenshots)
    if (g_devAim) aim = 1.f;
    if (g_devOverlay == "armory") { prog.points = 3; prog.up[2].tier[0] = 2; prog.up[3].mod = true; armoryOpen = true; armoryW = 2; }
    if (g_devOverlay == "pause") paused = true;
    if (g_devOverlay == "ascend" && act2()) ascendT = ASCEND_SINK + 0.5f;   // the act's card, then the rise into the Zenith (screenshots)
    if (g_devOverlay == "victory") {   // the victory screen mid name entry (screenshots)
        elapsedTime = 754.3f; victory = true; ranked = true;
        bankedStyle = 6240.f; bankedDamage = 410.f; wavesCleared = endless() ? 14 : 0; totalKills = 187;
        board.load(); nameEntry = true; nameBuf = "OLLIE";
    }
    for (int k = 0; k < (int)g_devSpawns.size(); ++k) {
        float a = (k - (g_devSpawns.size() - 1) * 0.5f) * 0.4f, dist = 9.f + (k % 2) * 3.f;
        glm::vec3 f = player.camera.flatForward(), r{-f.z, 0.f, f.x};
        glm::vec3 at = player.position * glm::vec3{1, 0, 1} + (f * std::cos(a) + r * std::sin(a)) * dist;
        spawnEnemy((EnemyType)g_devSpawns[k], at + glm::vec3{0, groundHeightAt(at.x, at.z, player.position.y + 1.f), 0}, (Hollow)g_devHollow);
    }
    if (g_devOverlay == "settings") { paused = true; pauseSettings = true; }
    if (g_devOverlay == "binds") { paused = true; pauseSettings = true; settingsMenu.openBinds(); settingsMenu.bindSel = 6; settingsMenu.capturing = true; }   // (screenshots)

    prevTicks = SDL_GetPerformanceCounter();
    freq      = SDL_GetPerformanceFrequency();
    prevCamPos = player.camera.position;

    captureMouse(true);
    if (act2()) beginAct2();   // after the capture: the armory frees the mouse to shop with
    if (g_bossRun >= 0) startBossRun();
}

inline GameplayState::~GameplayState() {
    logRun("quit");   // whatever was played since the last death or finish (RunLog.h)
    audio.setMuffle(0.f);   // back to the menu: the world isn't dull any more
    audio.setWind(0.f); audio.setRope(0.f); audio.setOverdrive(false, 0.f);
    audio.setRoom(RoomTone::NONE);   // the menu has no air of its own
    glDeleteTextures(1,&whiteTex);
    glDeleteTextures(TEX_COUNT, worldTex);
    world.contact.release();
    movingShadows.release();
    if (tracerVAO)   glDeleteVertexArrays(1, &tracerVAO);
    if (tracerVBO)   glDeleteBuffers(1, &tracerVBO);
    if (waterVAO)    glDeleteVertexArrays(1, &waterVAO);
    if (waterVBO)    glDeleteBuffers(1, &waterVBO);
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
    shifts.reset(level);   // the sun back up, the lava back down, the platforms back to speed, the cage to the top
    boardHinted = liftRiding = false;
    ward.clear(); conduitClock.reset(); wardenUid = -1; shifts.bossPulseOff();
    relic.clear(); lastChunk = 0; snareTimer = 0.f; driftWasGliding = false;
    lev.clear(); beachHinted = false; levSinkT = 1e9f;   // (shifts.reset drained the Maw's flood)
    pen.clear(); scourgeAnnounced = false; rip = AnchorRip{}; downHinted = false;
    level.anchors = anchorsBuilt;   // every chain whole again
    for (size_t i = 0; i < level.anchors.size() && i < anchorBoxes.size(); ++i) level.walls[level.anchors[i].wall].box = anchorBoxes[i];
    {   // a practice start at a later wave: the flood already at that wave's level
        const Arena& fa = level.arenas[a];
        if (g_startWave > 0 && fa.shift == ArenaShift::FLOOD && !fa.floodLevels.empty())
            for (auto& w : level.water) w.level = fa.floodLevels[std::min(g_startWave, (int)fa.floodLevels.size() - 1)];
    }
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
    enemies.clear(); pendingTwins.clear();
    voices.reset();                                   // and their voices: nobody is speaking now
    for (auto& [uid, h] : voiceLoops) audio.stop(h);
    voiceLoops.clear();
    for (auto& p : projSystem.pool) p.alive = false;
    fx.clear(); pickups.clear(); pendingBlasts.clear();
    sov.clear(); lastStand = false; lastStandT = 0.f;
    banners.clear();
    grapple.release();
    holsterSword(); sword = Sword{};
    abh.clear(); level.restoreZenith(); abGhosts.clear(); audio.stop(abWindH); audio.stop(abFlightH); audio.stop(abBeamH); abWindH = abFlightH = abBeamH = 0;
    abSkyMarkT = abCrackT = -1.f; abLineT = abHintT = 0.f; abLineShown = -1; abDeathT = 1e9f;   // the Zenith whole again
    if (!(act2() && a == ZENITH_ARENA)) msCheckpoint = false;   // MORNINGSTAR's checkpoint holds only for a retry in the Zenith
    msSwallowT = msCheckpoint ? Enemy::MS_SWALLOW : -1.f; msTotalityT = -1.f;   // (retried from him: already inside the eclipse)
    eclipseFlare = 0.f; abShaftT = abDawnT = -1.f; swordRoll = 0.f;
    pillarFallT.fill(-1.f); shardFallT.fill(-1.f); punchedThisTick = false;
    ascendT = -1.f; whiteFade = level.zenith.built && a == ZENITH_ARENA && act2() ? 1.f : 0.f;
    playerDead = false; deadTimer = 0.f;
    victory = false; victoryDelay = -1.f;
    finishOpen = false;
    act2Falling = act2() && !g_devCam;
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
    if (g_bossRun >= 0 && !fast()) { director.wave = bossWaveOf(a); bossStartAt = -1.f; }   // BOSS: straight to its wave (a retry too), its clock afresh
}

inline void GameplayState::resetPlayer(glm::vec3 start) {
    player = Player{start};
    viewModel.reset(activeWeapon);   // whatever the gun was doing (a reload, a switch) is over
    dryCd = 0.f;
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
    aim = 0.f; aimFullAt = -1.f;
    dashCharges = 2; dashCooldown = 0.f; dashMomentumTimer = 0.f;
    grappleCooldown = 0.f; wasGrappling = false;
    jumpsRemaining = 2; slamming = false; invincFrames = 0.f;
    activeWeapon = 0; pendingWeapon = -1; weaponSwitchTimer = 0.f;
    recoilPitch = 0.f; peakFallSpeed = 0.f; fovKick = 0.f; camFeel.reset();
    dashFx = speedFx = lowFx = heartT = heartPulse = windFx = 0.f; audio.setMuffle(0.f); audio.setWind(0.f);
    hasSafeGround = false; embeddedT = 0.f; slowT = 0.f; ropeFx = 0.f; audio.setRope(0.f);
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
    enterArena(retryTarget(director.phase, director.arena));   // (the Zenith, if you died rising into it)
    captureMouse(true);
}

inline void GameplayState::newRun() {
    msCheckpoint = false;
    director = WaveDirector{};
    director.level = &level;
    director.fast  = fast();
    prog = Progression{};
    totalKills = totalShots = totalHits = deaths = 0;
    elapsedTime = 0.f; peakStyle = 0.f;
    splits.clear();
    fastLeftStanding = 0;
    roomTimes.clear(); roomMedals.clear(); roomNewBest = false;
    newRecord = false;
    ranked = !g_godMode && !g_practice && !act2();
    act2FromStart = act2();
    nameEntry = false; boardPlace = -1;
    flareIntroduced = false;
    ghostRec.pts.clear();
    if (fast()) ghost.load();
    if (endless()) setupEndless();
    if (g_bossRun >= 0) { ranked = act2FromStart = false; msCheckpoint = bossEntry(g_bossRun).morning; }
    enterArena(g_bossRun >= 0 ? bossEntry(g_bossRun).arena : endless() ? endlessArena : roomAttack() ? g_roomAttack : 0);
    bankedStyle = bankedDamage = 0.f;   // after enterArena: that banked the last run's
    wavesCleared = 0;
    if (fast() && !roomAttack()) { countdown = 3.f; pushBanner("THE GAUNTLET", "SEVEN ROOMS - THEN REACH THE BEACON ON THE TOWER", {1.f, 0.6f, 0.2f}, 3.f); }
    if (roomAttack()) startRoomAttack();
    captureMouse(true);
    if (act2()) beginAct2();   // after the capture: the armory frees the mouse to shop with
    if (g_bossRun >= 0) startBossRun();
}

// BOSS: the wave its boss is in
inline int GameplayState::bossWaveOf(int a) const {
    const Arena& ar = level.arenas[a];
    for (int w = 0; w < (int)ar.waves.size(); ++w)
        for (auto& g : ar.waves[w]) if (isBoss(g.type)) return w;
    return 0;
}
// BOSS: the upgrades a run would have by this boss, spent in the armory before the fight
inline void GameplayState::startBossRun() {
    const BossEntry& b = bossEntry(g_bossRun);
    prog = Progression{};
    prog.level = b.level; prog.points = b.points;
    bossStartAt = -1.f; bossKillT = 0.f; bossDone = false; bossPost = bossPosted = false;
    banners.clear();
    pushBanner(b.name, b.where, {1.f, 0.6f, 0.25f}, 3.f);
    if (!g_devCam && g_devOverlay.empty() && !armoryOpen) openArmory();
}
// FAST's ROOM ATTACK: the room on its own, with the level a whole run would have reached by it, the
// armory open to spend it, and the medals to beat on its card
inline void GameplayState::startRoomAttack() {
    const int r = glm::clamp(g_roomAttack, 0, (int)level.arenas.size() - 1);
    const Arena& a = level.arenas[r];
    prog = Progression{};
    prog.level = 1 + r; prog.points = r;
    countdown = 3.f;
    char sub[96];
    snprintf(sub, sizeof(sub), "GOLD %s   SILVER %s   BRONZE %s", formatTime(a.medals[0], false).c_str(),
             formatTime(a.medals[1], false).c_str(), formatTime(a.medals[2], false).c_str());
    banners.clear();
    pushBanner(std::string("ROOM ATTACK  ") + a.name, sub, {1.f, 0.6f, 0.2f}, 3.f);
    if (r > 0 && !g_devCam && g_devOverlay.empty() && !armoryOpen) openArmory();
}

// BOSS: it's down (for THE ABSOLUTE alone: as the eclipse takes him). The run ends; a best time is kept
inline void GameplayState::bossRunDone() {
    if (g_bossRun < 0 || bossDone) return;
    bossDone = true;
    bossKillT = bossStartAt >= 0.f ? elapsedTime - bossStartAt : elapsedTime;
    newRecord = !g_godMode && !g_practice && recordBoss(records, g_bossRun, bossKillT);
    if (newRecord) records.save();
    bossPost = (WORLD_BOARD || g_devOverlay.find("worldboard") != std::string::npos) && !g_godMode && !g_practice;   // (the web: the victory screen asks for a name to post it under)
    victoryDelay = 3.f;
}

// ACT II: the head start (what an Act I run has by the Sanctum), spent in the
// armory before the fall
inline void GameplayState::beginAct2() {
    prog = Progression{};
    prog.level = 6; prog.points = 5;
    if (g_bossRun >= 0) return;   // (BOSS: startBossRun sets the run up)
    pushBanner("ACT II", "BENEATH THE ECLIPSE", {0.35f, 0.95f, 0.9f}, 3.5f);
    if (!g_devCam && g_devOverlay.empty()) openArmory();
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
            if (fast()) roomStartT = elapsedTime;   // (the room's own clock: its medal)
            if (fast()) {
                snprintf(buf, sizeof(buf), "ROOM %d/%d  %s", ev.value + 1, (int)level.arenas.size(), ar.name);
                pushBanner(buf, ar.subtitle, {1.f, 0.7f, 0.3f}, 1.8f);
                audio.play("wave", 90, SoundGroup::UI);
            } else if (endless()) {
                pushBanner(std::string("ENDLESS  ") + ar.name, "NO EXIT - JUST THE NEXT WAVE", {1.f, 0.3f, 0.45f}, 3.f);
                audio.play("wave", 128, SoundGroup::UI);
            } else if (act2() && ar.shift == ArenaShift::DESCENT) {
                level.lift.request(ArenaShifts::descentStop(director.wave));   // down to the first fight
                boardHinted = true;
                pushBanner(std::string("ACT II  ") + ar.name, "STEP ONTO THE CAGE", {1.f, 0.7f, 0.3f}, 3.f);
                audio.play("wave", 128, SoundGroup::UI);
            } else if (act2()) {
                pushBanner(std::string("ACT II  ") + ar.name, ar.subtitle, {0.35f, 0.95f, 0.9f}, 2.6f);
                audio.play("wave", 128, SoundGroup::UI);
            } else {
                snprintf(buf, sizeof(buf), "ARENA %d/%d", ev.value + 1, (int)level.arenas.size());
                pushBanner(std::string(buf) + "  " + ar.name, ar.subtitle, {1.f, 0.78f, 0.3f}, 2.6f);
                audio.play("wave", 128, SoundGroup::UI);
            }
            break;
        case DirectorEvent::WAVE_START:
            if (endless()) shifts.onWave(level, director.arena, ev.value % 3, director.goal(), moverClock, 3);
            else shifts.onWave(level, director.arena, ev.value, director.goal(), moverClock);
            if (ar.shift == ArenaShift::SPEED_UP && ev.value > 0) ui.feed("THE PLATFORMS SPEED UP", {0.4f, 0.9f, 1.f});
            if (ar.shift == ArenaShift::SOLAR && ev.value > 0)
                ui.feed(ev.value >= 2 ? "THE RINGS QUICKEN - A SECOND FLARE" : "THE RINGS QUICKEN", {1.f, 0.75f, 0.3f});
            if (fast()) {
                const EnemyType* mini = nullptr;   // the room's mini-boss arriving: its name and how to beat it
                for (auto& we : ar.waves[ev.value]) if (isMiniBoss(we.type)) mini = &we.type;
                if (mini) {
                    pushBanner(statsOf(*mini).name, statsOf(*mini).hint, statsOf(*mini).glow, 3.f);
                    audio.play("mini_sting", 128, SoundGroup::UI, true);
                    audio.duck(5.f, 0.6f);
                } else if (ev.value > 0) pushBanner("SECOND WAVE", "", {1.f, 0.5f, 0.3f}, 1.4f);
                break;
            }
            if (endless()) {
                level.arenas[director.arena].maxAlive = endlessBaseAlive + std::min(6, ev.value / 3);   // more at once as it goes on
                snprintf(buf, sizeof(buf), "WAVE %d", ev.value + 1);
            } else snprintf(buf, sizeof(buf), "WAVE %d/%d", ev.value + 1, director.waveCount());
            pushBanner(buf, director.goal().label, {1.f, 0.9f, 0.4f}, director.hasGoal() ? 2.6f : 1.8f);
            audio.play("wave", 90, SoundGroup::UI); audio.duck(4.f, 0.4f);
            break;
        case DirectorEvent::BOSS_START:
            if (bossStartAt < 0.f) bossStartAt = elapsedTime;   // BOSS: its clock
            if (ar.waves[ev.value][0].type == EnemyType::SOVEREIGN)
                pushBanner("THE SOVEREIGN", "PARRY (F) THE BLADE AS IT LANDS", {1.f, 0.3f, 0.2f}, 4.f);
            else if (ar.waves[ev.value][0].type == EnemyType::PENITENT)
                pushBanner("THE PENITENT", "BREAK ITS CHAINS - JUMP LOW SWEEPS, SLIDE UNDER HIGH ONES", {1.f, 0.5f, 0.2f}, 4.5f);
            else if (ar.waves[ev.value][0].type == EnemyType::ABSOLUTE && msCheckpoint) { abHintT = 6.f; }   // (MORNINGSTAR announces himself)
            else if (ar.waves[ev.value][0].type == EnemyType::ABSOLUTE) {
                pushBanner("THE ABSOLUTE", g_tellsColourblind ? "WHITE: PARRY   BLUE: DODGE OR BLOCK   RED: GET OUT" : "WHITE: PARRY   GOLD: DODGE OR BLOCK   RED: GET OUT", {1.f, 0.97f, 0.9f}, 4.5f);
                abHintT = 9.f;
            }
            else if (ar.waves[ev.value][0].type == EnemyType::LEVIATHAN)
                pushBanner("THE LEVIATHAN", "DASH THE CRASH - SHOOT THE EYE WHILE IT'S DOWN", {1.f, 0.35f, 0.2f}, 4.5f);
            else if (ar.waves[ev.value][0].type == EnemyType::PACEMAKER)
                pushBanner("THE PACEMAKER", "PARRY THE WHITE - IT RACES YOU TO THE BEACON", {1.f, 0.6f, 0.15f}, 4.f);
            else
                pushBanner("THE WARDEN", "CUT ITS CONDUITS - HIT THE CORE WHEN IT VENTS", {1.f, 0.2f, 0.65f}, 3.5f);
            audio.play("wave", 128, SoundGroup::UI); audio.play("explosion", 70, SoundGroup::UI); audio.duck(4.f, 0.4f);
            shake(0.6f, 0.06f);
            break;
        case DirectorEvent::NEW_TYPE: {
            EnemyType t = (EnemyType)(ev.value & 255);
            Hollow h = (Hollow)(ev.value >> 8);
            if (isBoss(t) || isMiniBoss(t)) break;   // (they get a title card of their own)
            std::string name = h == Hollow::NONE ? std::string(statsOf(t).name)
                                                 : std::string(hollowName(h)) + " " + statsOf(t).name;
            const char* hint = h == Hollow::NONE ? (t == EnemyType::ABSOLUTE ? abTellLegend() : statsOf(t).hint) : hollowHint(h);
            ui.introduce(name, hint, statsOf(t).glow);   // a small card at the left edge, not a title card
            break;
        }
        case DirectorEvent::WAVE_CLEARED:
            if (endless()) ++wavesCleared;
            shifts.onWaveOver();
            shifts.onWaveCleared(level, director.arena, ev.value + 1);
            if (!fast()) pushBanner("WAVE CLEAR", "", {0.4f, 1.f, 0.6f}, 1.6f);
            styleSystem.heal(10.f);
            break;
        case DirectorEvent::ARENA_CLEARED: {
            if (g_bossRun >= 0) { bossRunDone(); break; }   // BOSS: that was the fight
            const Arena& done = level.arenas[ev.value];
            lockDoor(done.exitDoor, false);
            lockDoor(done.entryGate, false);
            if (fast()) {
                int vs = recordSplit(ev.value);
                styleSystem.heal(25.f);
                grenadeCount = grenadeMax;
                audio.play("anvil", 128, SoundGroup::UI, true);   // the room's done: struck on the anvil
                if (vs != 0) {   // ahead of the best run: two rising blips; behind: two falling
                    SfxMixer::Opts o; o.volume = 1.f; o.group = SoundGroup::UI; o.delay = 0.18f;
                    audio.playOpts(vs < 0 ? "split" : "split_behind", o);
                }
                if (!roomMedals.empty() && roomMedals.back() > 0) {   // a medal: bells, higher for a better one
                    SfxMixer::Opts o; o.volume = 1.f; o.group = SoundGroup::UI; o.delay = 0.5f; o.priority = true;
                    o.pitch = 0.8f + 0.12f * (float)roomMedals.back();
                    audio.playOpts("medal", o);
                }
                if (roomAttack()) {   // ROOM ATTACK: that was the room
                    victory = true;
                    captureMouse(false);
                    newRecord = roomNewBest;
                    if (newRecord) recordCue();
                    logRun("end");
                }
                break;
            }
            if (done.exitDoor >= 0 && !act2())
                pushBanner("ARENA CLEARED", "THE GATE IS OPEN - HEAD NORTH", {0.4f, 1.f, 0.6f}, 3.5f);
            else if (act2() && done.shift == ArenaShift::DRIFT)
                pushBanner("THE HOLE IS OPEN", "JUMP IN", {1.f, 0.4f, 0.25f}, 3.5f);
            else if (done.exitDoor >= 0)
                pushBanner("THE WAY DOWN IS OPEN", ev.value == 0 ? "BEHIND THE ORGAN" : "THROUGH THE NORTH ARCH", {0.35f, 0.95f, 0.9f}, 3.5f);
            styleSystem.heal(40.f);
            grenadeCount = grenadeMax;
            audio.play("anvil", 128, SoundGroup::UI, true);   // an arena won: struck on the anvil
            if (zenithHandoff(act2(), ev.value, (int)level.arenas.size())) {   // the Leviathan is dead: the act is done
                if (act2RunRecords(act2FromStart, g_godMode, g_practice)) { recordAct2Clear(records, elapsedTime); records.save(); }
                ascendT = 0.f;   // it sinks, ACT II COMPLETE, a white-out, then the Zenith (updateAscension)
            }
            break;
        }
        case DirectorEvent::GOAL_DONE:
            shifts.onWaveOver();
            // The objective's met: whatever's left of the wave falls apart
            for (auto& e : enemies)
                if (e.alive) {
                    e.alive = false; e.state = EnemyState::DEAD;
                    for (const VoiceCue& c : voices.forget(e.uid)) playVoice(c);
                    spawnDebrisFor(e);
                    fx.spawnDeathParticles(e.position + glm::vec3{0, e.height() * 0.5f, 0}, e.stats().color);
                }
            pushBanner("OBJECTIVE COMPLETE", "", {0.4f, 1.f, 0.6f}, 1.8f);
            styleSystem.addStyle(30.f);
            gainXp(40);
            ui.feed("OBJECTIVE  +40 XP", {0.4f, 1.f, 0.6f});
            audio.play("wave", 128, SoundGroup::UI);
            shake(0.3f, 0.05f);
            break;
        case DirectorEvent::VICTORY:
            if (g_bossRun >= 0) { bossRunDone(); break; }
            victoryDelay = act2() ? 3.f : 2.5f;   // ACT II: the Absolute kneels and dissolves first
            break;
        case DirectorEvent::FINISH_OPEN:
            finishOpen = true;
            pushBanner("FINISH OPEN", "REACH THE BEACON", {1.f, 0.6f, 0.2f}, 2.f);
            audio.play("wave", 128, SoundGroup::UI);
            break;
        }
    }
    director.events.clear();
}

inline int GameplayState::recordSplit(int section) {
    if ((int)splits.size() > section) splits.resize(section);
    splits.push_back(elapsedTime);
    // The room on its own: its time, its medal, kept if it's its best
    const float rt = elapsedTime - roomStartT;
    const int medal = medalFor(level.arenas[section].medals, rt);
    if ((int)roomTimes.size() > section) { roomTimes.resize(section); roomMedals.resize(section); }
    roomTimes.push_back(rt); roomMedals.push_back(medal);
    roomNewBest = false;
    if (keepsRoomTimes()) { roomNewBest = records.roomTime(section, rt, medal); records.save(); }
    char buf[96];
    if (roomAttack()) {
        snprintf(buf, sizeof(buf), "%s  %s  %s", level.arenas[section].name, formatTime(rt).c_str(), medal ? medalName(medal) : "NO MEDAL");
        ui.showSplit(buf, medalColor(medal));
        return 0;
    }
    if (section < (int)records.fastSplits.size()) {
        float d = elapsedTime - records.fastSplits[section];
        snprintf(buf, sizeof(buf), "ROOM %d  %s  %+.2f%s%s", section + 1, formatTime(elapsedTime).c_str(), d, medal ? "  " : "", medalName(medal));
        ui.showSplit(buf, d <= 0.f ? glm::vec3{0.3f, 1.f, 0.5f} : glm::vec3{1.f, 0.4f, 0.35f});
        return d <= 0.f ? -1 : 1;
    }
    snprintf(buf, sizeof(buf), "ROOM %d  %s%s%s", section + 1, formatTime(elapsedTime).c_str(), medal ? "  " : "", medalName(medal));
    ui.showSplit(buf, {1.f, 0.9f, 0.6f});
    return 0;
}

inline void GameplayState::finishRun() {
    if (!victory) logRun("end");
    victory = true;
    captureMouse(false);
    if (g_bossRun >= 0) {   // BOSS: its time is kept already (bossRunDone); a name for the world board
        if (bossPost && !bossPosted) { board.load(); nameEntry = true; nameBuf = board.lastName; }
        return;
    }
    newRecord = false;
    if (act2() && director.arena == ZENITH_ARENA && act2RunRecords(act2FromStart, g_godMode, g_practice)) {   // (ACT II is never ranked: its own local best)
        newRecord = recordZenith(records, elapsedTime); records.save();
        if (newRecord) recordCue();
    }
    if (!ranked) return;
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
        newRecord = me.score > records.bestEndless;
        if (newRecord) records.bestEndless = me.score;
    } else {
        newRecord = records.bestArena <= 0.f || elapsedTime < records.bestArena;
        if (newRecord) records.bestArena = elapsedTime;
        int sc = me.score;
        if (sc > records.bestArenaScore) { records.bestArenaScore = sc; newRecord = true; }
    }
    if (newRecord) { records.save(); recordCue(); }
}

// A new best: bells over a bright strike, after the finish has rung out a moment
inline void GameplayState::recordCue() {
    SfxMixer::Opts o; o.volume = 1.f; o.group = SoundGroup::UI; o.priority = true; o.delay = 1.3f;
    audio.playOpts("record", o);
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

// ---- ENDLESS ----------------------------------------------------------------
// The run's arena gets generated waves instead of its own: one is always
// queued ahead of the director, so it never runs out and never "clears" the
// arena (its exit stays shut). The arena's own objectives come back as every
// 4th wave (EndlessWaves.h).
inline void GameplayState::setupEndless() {
    endlessArena = 3;   // the Core
    static LevelData built = buildLevel();   // the arenas' own objectives, before any run touched them
    Arena& a = level.arenas[endlessArena];
    endlessBaseAlive = built.arenas[endlessArena].maxAlive;
    uint32_t seed = (uint32_t)SDL_GetPerformanceCounter();
    gen.begin(seed, built.arenas[endlessArena].goals, endlessArena == 3);
    a.waves.clear(); a.goals.clear();
    feedEndless();
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
    return 1.f + 0.03f * director.wave;   // each wave a little tougher
}

inline Theme GameplayState::bloodEclipse(const Theme& t) {
    Theme o = t;
    o.zenith = {0.05f, 0.0f, 0.0f}; o.horizon = {0.55f, 0.03f, 0.02f}; o.ground = {0.06f, 0.0f, 0.0f};
    o.sunColor = {2.2f, 0.45f, 0.25f}; o.mountain = {0.1f, 0.01f, 0.01f};
    o.lightColor = {1.3f, 0.5f, 0.4f}; o.skyAmb = {0.34f, 0.08f, 0.07f};
    o.fogColor = {0.22f, 0.02f, 0.02f}; o.fogDensity = t.fogDensity * 1.6f;
    return o;
}

inline Theme GameplayState::nightfall(const Theme& t) {
    Theme o = t;
    o.zenith = t.zenith * 0.08f; o.horizon = t.horizon * 0.18f; o.ground = t.ground * 0.15f;
    o.sunColor = t.sunColor * 0.3f; o.mountain = t.mountain * 0.15f; o.stars = std::max(t.stars, 1.f);
    o.lightColor = t.lightColor * 0.22f; o.skyAmb = t.skyAmb * 0.25f; o.groundAmb = t.groundAmb * 0.25f;
    o.fogColor = t.fogColor * 0.15f;
    return o;
}

// The Zenith after the Absolute: the eclipse gone, a gold sunrise over the cloud
inline Theme GameplayState::dawnOf(const Theme& t) {
    Theme o = t;
    o.zenith = {0.05f, 0.07f, 0.2f}; o.horizon = {0.9f, 0.42f, 0.14f}; o.ground = {0.28f, 0.13f, 0.08f};
    o.sunColor = {1.5f, 0.75f, 0.3f};
    o.lightDir = glm::normalize(glm::vec3{0.1f, -0.85f, 0.4f}); o.lightColor = {1.7f, 1.35f, 0.95f};
    o.skyAmb = {0.24f, 0.22f, 0.32f}; o.groundAmb = {0.36f, 0.2f, 0.1f};
    o.fogColor = {0.5f, 0.3f, 0.16f}; o.fogDensity = t.fogDensity * 0.7f;
    return o;
}

inline void GameplayState::updateBossDusk(float dt) {
    float want = 0.f;
    for (auto& e : enemies)
        if (e.alive && isBoss(e.type) && e.maxHealth > 0.f) want = std::max(want, 1.f - glm::clamp(e.health / e.maxHealth, 0.f, 1.f));
    bossDusk += glm::clamp(want - bossDusk, -dt / 3.f, dt * 0.5f);   // falls with his health; dawn comes back slower
    if (g_devOverlay.rfind("dusk", 0) == 0) bossDusk = std::atoi(g_devOverlay.c_str() + 4) / 100.f;   // dev: --overlay duskNN (screenshots)
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
    camFeel.shake(t, amount, settings ? settings->screenShake : 1.f);
}

// The dash's rush and low health's drain (drawn in bloom_composite.frag), the heartbeat and the muffle
inline void GameplayState::updateScreenFx(float dt) {
    float motion = settings ? settings->screenShake : 1.f;   // the rush is motion: SCREEN SHAKE scales it
    dashFx = std::max(0.f, dashFx - dt / 0.35f);
    float fast = glm::clamp((glm::length(player.velocity) - 18.f) / 22.f, 0.f, 1.f) * 0.6f;
    float want = std::max(dashFx * dashFx * (3.f - 2.f * dashFx), fast) * motion;
    speedFx = want > speedFx ? want : speedFx + (want - speedFx) * (1.f - std::exp(-dt * 8.f));   // in at once, out eased

    float hp = styleSystem.maxHealth > 0.f ? styleSystem.health / styleSystem.maxHealth : 1.f;
    float low = glm::clamp((0.35f - hp) / 0.27f, 0.f, 1.f);
    lowFx += (low - lowFx) * (1.f - std::exp(-dt * 3.f));
    if (lowFx > 0.05f) {   // lub-dub, quicker the closer you are to dying
        heartT += dt;
        if (heartT >= glm::mix(0.95f, 0.5f, lowFx)) { heartT = 0.f; audio.play("heartbeat", 55 + (int)(70.f * lowFx), SoundGroup::UI); }
    } else heartT = 0.f;
    float dub = heartT > 0.17f ? std::exp(-(heartT - 0.17f) * 10.f) * 0.6f : 0.f;
    heartPulse = lowFx > 0.05f ? std::min(1.f, std::exp(-heartT * 10.f) + dub) : 0.f;
    audio.setMuffle(lowFx * 0.75f);   // the world goes dull and far away
    // The air going past: nothing at a run (7 m/s), building through a slide and a fall, a roar at grapple speed
    float spd = glm::length(player.velocity);
    float wantWind = glm::clamp((spd - 9.f) / 32.f, 0.f, 1.f);
    windFx += (wantWind - windFx) * (1.f - std::exp(-dt * (wantWind > windFx ? 6.f : 3.f)));
    audio.setWind(playerDead ? 0.f : windFx);
    // The rope singing while the grapple pulls: higher the faster it drags you
    float wantRope = grapple.active ? glm::clamp(spd / 38.f, 0.15f, 1.f) : 0.f;
    ropeFx += (wantRope - ropeFx) * (1.f - std::exp(-dt * (wantRope > ropeFx ? 14.f : 9.f)));
    audio.setRope(playerDead ? 0.f : ropeFx);
}

inline void GameplayState::update(float dt) {
    if (settings) audio.masterVolume = settings->audioVolume;
    updateMusic();
    // The place's own air (RoomTone.h): full between fights, under a fight,
    // low while paused or dead; the Yard's night comes with its nightfall
    audio.setRoomLevel(paused || armoryOpen || playerDead ? 0.35f : director.fighting() ? 0.7f : 1.f);
    {
        int a = director.arena;
        bool dusk = a >= 0 && a < (int)level.arenas.size() && level.arenas[a].shift == ArenaShift::NIGHTFALL;
        audio.setRoomNight(dusk ? shifts.night[a] : 0.f);
    }
    updateBossDusk(dt);

    // Banners keep animating on the death/victory screens, not while paused
    bool frozen = paused || armoryOpen || g_balanceOpen;   // (F1: the balance panel)
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
        if (victory) audio.setMuffle(0.f);
        audio.setWind(0.f); audio.setRope(0.f);   // paused, dead or won: no air moving
        if (countdown > 0.f && !frozen) {
            float before = countdown;
            countdown -= dt;
            if (std::ceil(before) != std::ceil(countdown) || before >= 3.f)   // 3, 2, 1: beeps (the 3 as it appears); GO: the starting gun
                audio.play(countdown <= 0.f ? "count_go" : "count_tick", 128, SoundGroup::UI, true);
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
    accumulator += elapsed * timeScale();   // (a wave's last kill: slow motion)
    if (slowT > 0.f) slowT -= (float)elapsed;
    if (accumulator > 0.25) accumulator = 0.25;

    // Your bindings, read as the default keys the game knows (Bindings.h)
    static Uint8 bound[SDL_NUM_SCANCODES];
    binds().remap(SDL_GetKeyboardState(nullptr), g_devNoMouse ? 0 : SDL_GetMouseState(nullptr, nullptr), bound);
    const Uint8* keys = bound;
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
            lookDelta += glm::vec2{gp.look.x * speed, -dy} * aimSensScale();
        }
    }
    static Uint8 noKeys[SDL_NUM_SCANCODES] = {};
    if (g_devNoMouse) keys = noKeys;   // screenshot runs: the real keyboard doesn't reach the game
    bool parryKey = keys[SDL_SCANCODE_F] != 0;

    if (hitStopFrames <= 0) shootInput();   // the click's shot in this frame, not the next tick's

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
    // Per frame, not per tick: the aim settling after a shot and the sword's lean stay smooth above 60 fps
    if (recoilPitch > 0.f) {
        // Settle back quickly at first, then ease in (was a flat 14 degrees a second)
        float applied = std::min(recoilPitch, std::max(recoilPitch * (1.f - std::exp(-floatDt * 11.f)), 1.5f * floatDt));
        player.camera.pitch -= applied;
        recoilPitch -= applied;
        player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);
    }
    swordRoll *= std::exp(-floatDt * 9.f);
    styleSystem.update(floatDt);

    // Doors part as you run at them (unless locked) and shut behind you
    level.updateDoors(floatDt, player.position, [this](int di, bool opening) {
        const AABB& c = level.doors[di].closed;
        audio.playAt(opening ? "door" : "door_close", (c.min + c.max) * 0.5f, 120, SoundGroup::WORLD);
    });

    viewModel.setMove(dashMomentumTimer > 0.f, player.sliding);
    viewModel.update(floatDt, playerXZSpeed, player.onGround);
    {   // the gun's sway: the look this frame, the strafe, the stride (same phase as the camera's bob)
        bool bob = !settings || settings->viewBob;
        viewModel.feel(floatDt, lookDelta, glm::dot(player.velocity, player.camera.right()),
                       glm::dot(player.velocity, player.camera.flatForward()), camFeel.strideAt(renderAlpha), bob ? camFeel.bobAmt : 0.f);
        lookDelta = glm::vec2{0.f};
    }
    // --overlay swayL / swayR: the gun as if turning hard left / right (screenshots)
    if (g_devOverlay == "swayR" || g_devOverlay == "swayL") { viewModel.swayX = {g_devOverlay == "swayR" ? -5.5f : 5.5f, 0.f}; viewModel.swayV = glm::vec2{0.f}; }
    // --overlay leanL / leanR: strafing hard left / right (with --aim: the sights' tip)
    if (g_devOverlay == "leanR" || g_devOverlay == "leanL") {
        float sd = g_devOverlay == "leanR" ? 1.f : -1.f;
        viewModel.leanSide = sd; viewModel.leanRoll = -sd * 2.6f; viewModel.leanX = -sd * 0.009f;
    }
    for (const auto& c : viewModel.takeCues()) audio.play(c.name, c.volume, SoundGroup::PLAYER);   // on the motion's beats
    // --overlay reloadNN / inspectNN / switchNN: hold the gun NN% through it (screenshots)
    if (g_devOverlay.rfind("reload", 0) == 0) {
        float u = std::atoi(g_devOverlay.c_str() + 6) / 100.f;
        int m = weaponMag((WeaponId)activeWeapon, prog.up[activeWeapon]);
        viewModel.reloadShells = activeWeapon == 1 ? m : activeWeapon == 2 ? std::min(5, m) : m;
        viewModel.motion.devReload(u, 0, m, viewModel.reloadShells);
    }
    if (g_devOverlay.rfind("inspect", 0) == 0) viewModel.motion.devInspect(std::atoi(g_devOverlay.c_str() + 7) / 100.f);
    if (g_devOverlay.rfind("switch", 0) == 0) viewModel.motion.devSwitch(std::atoi(g_devOverlay.c_str() + 6) / 100.f, activeWeapon);
    if (g_devOverlay.rfind("hand", 0) == 0 && g_devOverlay.size() >= 6) {   // --overlay handPV: a hand up close (ViewModel::drawDevHand)
        viewModel.devHand = std::atoi(g_devOverlay.substr(4, g_devOverlay.size() - 5).c_str()); viewModel.devHandView = g_devOverlay.back() - '0';
    }
    // --overlay fanNN / faninNN / fanoutNN: the fanning hand NN% round a slap / of the way in / of the way put away
    if (g_devOverlay.rfind("fan", 0) == 0 && g_devOverlay.size() > 3) {
        const bool in = g_devOverlay.rfind("fanin", 0) == 0, out = g_devOverlay.rfind("fanout", 0) == 0;
        viewModel.devFan(std::atoi(g_devOverlay.c_str() + (in ? 5 : out ? 6 : 3)) / 100.f, in ? 1 : out ? 2 : 0);
    }
    if (g_devOverlay == "punch") { viewModel.parryTimer = ViewModel::PARRY_TIME * 0.62f; viewModel.parryHit = true; }
    // --overlay volley: orbs fired across your view every 0.2 s, a heavy shell every second (screenshots: their tumble,
    // trail and stretch), into the ground ahead (their shatter)
    if (g_devOverlay == "volley") {
        static float volleyT = 0.f; static int volleyN = 0;
        volleyT += dt;
        if (volleyT >= 0.2f) {
            volleyT = 0.f; ++volleyN;
            glm::vec3 f = player.camera.flatForward(), r{-f.z, 0.f, f.x};
            glm::vec3 from = player.camera.position + f * 7.f + r * 9.f + glm::vec3{0.f, 0.6f + 0.5f * (float)(volleyN % 3), 0.f};
            glm::vec3 vel = -r * 22.f + f * 4.f - glm::vec3{0.f, 1.2f, 0.f};
            const bool heavy = volleyN % 5 == 0;
            if (Projectile* q = projSystem.fire(from, vel, 0.f, false, heavy ? glm::vec3{1.f, 0.72f, 0.12f} : glm::vec3{1.f, 0.3f, 0.6f}, false, 0.f, heavy ? 1.6f : 1.f, heavy))
                q->lifetime = 1.2f;
        }
    }
    // --overlay showcase: one of each pickup and shot hanging in a row in front of you, the hook beside them
    if (g_devOverlay == "showcase") {
        glm::vec3 f = player.camera.flatForward(), r{-f.z, 0.f, f.x};
        glm::vec3 c = player.camera.position + f * 3.2f - glm::vec3{0.f, 0.4f, 0.f};
        const PickupKind kinds[3] = {PickupKind::ORB, PickupKind::POTION, PickupKind::XP};
        for (int k = 0; k < 3; ++k) { glm::vec3 p = c + f * 5.f + r * (-1.6f + 1.2f * k) + glm::vec3{0.f, 1.4f, 0.f}; pickups.push_back({p, glm::vec3{0.f}, 1e9f, kinds[k], p.y - 5.f, p}); }
        for (int k = 0; k < 3; ++k) {
            glm::vec3 p = c + r * (0.3f + 0.6f * k) + glm::vec3{0.f, 0.3f, 0.f};
            if (Projectile* q = projSystem.fire(p, r * 0.001f, 0.f, false, k == 1 ? glm::vec3{1.f, 0.72f, 0.12f} : glm::vec3{1.f, 0.3f, 0.6f}, k == 2, 0.f, 1.f, k == 1))
                { q->lifetime = 1e9f; q->hasGravity = false; }
        }
        g_devOverlay = "showcased";
    }
    if (g_devOverlay == "showcased") {   // (the hook, held out there)
        glm::vec3 f = player.camera.flatForward(), r{-f.z, 0.f, f.x};
        grapple.active = true; grapple.target = player.camera.position + f * 2.4f + r * 0.9f - glm::vec3{0.f, 0.2f, 0.f};
    }
    // --overlay dieT: at T s every enemy dies of a hard blow from you (the pieces, on film); "dieTh" a headshot
    if (g_devOverlay.rfind("die", 0) == 0 && g_devOverlay.size() > 3 && elapsedTime > (float)std::atof(g_devOverlay.c_str() + 3)) {
        for (auto& e : enemies)
            if (e.alive) {
                glm::vec3 d = e.position - player.position; d.y = 0.f;
                e.hitDir = glm::length(d) > 1e-3f ? glm::normalize(d) : glm::vec3{0.f, 0.f, 1.f};
                e.hitPush = 40.f; e.hitHead = g_devOverlay.back() == 'h';
                e.alive = false; e.state = EnemyState::DEAD; e.health = 0.f;
                spawnDebrisFor(e);
            }
        g_devOverlay.clear();
    }
    // --overlay punchNN / whiffNN: the parry's punch held NN% through it, landing a parry or not
    if ((g_devOverlay.rfind("punch", 0) == 0 || g_devOverlay.rfind("whiff", 0) == 0) && g_devOverlay.size() > 5) {
        viewModel.parryTimer = ViewModel::PARRY_TIME * (1.f - std::atoi(g_devOverlay.c_str() + 5) / 100.f);
        viewModel.parryHit = g_devOverlay[0] == 'p';
    }
    // Baseline FOV widens with horizontal speed on top of the dash kick
    float speedKick = glm::clamp((playerXZSpeed - 7.f) / 15.f, 0.f, 1.f) * 6.f;
    fovKick = glm::mix(fovKick, speedKick, std::min(1.f, floatDt * 7.f));
    camFeel.kickScale = settings ? settings->screenShake : 1.f;
    updateScreenFx(floatDt);
    updateFeel(floatDt);   // OVERDRIVE in and out, the kill kick, the finisher hint (Gameplay_Feel.h)
    camFeel.update(floatDt, glm::dot(player.velocity, player.camera.right()), playerXZSpeed, player.onGround);

    if (pendingWeapon >= 0) {
        weaponSwitchTimer -= floatDt;
        if (weaponSwitchTimer <= 0.f) {
            activeWeapon  = pendingWeapon;
            pendingWeapon = -1;
            const WeaponState& ws = weapons[activeWeapon];
            if (ws.reloading) {   // back to a gun still reloading: pick its reload up where it is
                int m = weaponMag((WeaponId)activeWeapon, prog.up[activeWeapon]);
                viewModel.triggerReload(ws.reloadTotal, m - ws.ammo, ws.ammo, m, ws.reloadProgress());
            }
        }
    }

    if (!act2Falling) elapsedTime += floatDt;   // ACT II's clock starts when you land
    if (styleSystem.style > peakStyle) peakStyle = styleSystem.style;

    frametimer::Lap ftFx;
    fx.updateParticles(floatDt * timeScale());
    fx.spawnAmbientParticles(level, player.position, fast(), floatDt);
    for (auto& p : projSystem.pool) {           // projectile trails
        if (!p.alive) continue;
        Effects::Particle* q = fx.freeParticle();
        if (!q) break;
        q->pos = p.position; q->vel = -p.velocity * 0.05f;
        q->color = p.emissiveColor * 0.8f; q->gravity = 0.f;
        q->maxLife = q->life = 0.22f; q->alive = true;
    }
    fx.updateDebris(floatDt * timeScale());
    fx.updateRings(floatDt * timeScale());
    ftFx.to(frametimer::EFFECTS);

    // Point light 0: explosion or muzzle flash. 1..3: nearest enemy shots.
    if (explosionFlashTimer > 0.f) {
        explosionFlashTimer -= floatDt;
        float t = glm::clamp(explosionFlashTimer / 0.35f, 0.f, 1.f);
        pointLightPos[0]   = explosionFlashPos;
        pointLightColor[0] = glm::vec3{1.6f, 0.9f, 0.4f} * t * (g_reduceFlashing ? 0.5f : 1.f);
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
    updateAscension(floatDt);
    whiteFade = std::max(0.f, whiteFade - floatDt);
    if (!styleSystem.isAlive() && !playerDead) {
        playerDead = true;
        deadTimer = 0.f;
        logRun("death");
        grapple.release();
        if (endless()) victoryDelay = 2.4f;   // no retries: YOU DIED, then on to the run's score
    }
}

inline void GameplayState::snapshotForInterpolation() {
    for (auto& e : enemies) { e.prevPosition = e.position; e.prevYaw = e.yaw; e.prevAnimPhase = e.animPhase; }
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
    bool zenith = act2() && a == ZENITH_ARENA;
    const bool morning = zenith && msSwallowT >= 0.f;   // inside the eclipse: MORNINGSTAR's track (from the next bar line)
    m.setTrack(fast() ? FAST_TRACK[a % 7] : morning ? 11 : zenith ? 10 : act2() ? 5 + std::min(a, 4) : ARENA_TRACK[a % 5]);   // the Nave .. the Maw; !!!!! has its own
    float lv = 0.6f;
    switch (director.phase) {
        case WaveDirector::Phase::ACTIVE:   lv = director.bossWave() ? 1.35f : 1.f; break;
        case WaveDirector::Phase::BREAK:    lv = 0.7f; break;
        case WaveDirector::Phase::CLEARED:  lv = 0.5f; break;
        case WaveDirector::Phase::VICTORY:  lv = fast() ? 1.2f : 0.55f; break;
        default: break;
    }
    if (lv >= 1.f && styleSystem.overdrive) lv = 1.35f;
    {   // OVERDRIVE: the stabs come in, its hum under everything (faster in its last 1.5 s), the world steps back
        const bool on = overdriveOn() && !playerDead && !victory;
        m.setDrive(on);
        audio.setOverdrive(on && !paused && !armoryOpen, std::clamp((1.5f - styleSystem.overdriveTimer) / 1.5f, 0.f, 1.f));
    }
    {   // the last one standing: the drums alone (not a boss's fight, not a wave that refills)
        int left = 0;
        for (const auto& e : enemies) if (e.targetable() && e.type != EnemyType::CONDUIT) ++left;
        bool last = director.fighting() && !director.bossWave() && director.queue.empty() && pendingTwins.empty() &&
                    director.goal().kind == WaveGoal::KILL_ALL && left == 1 && !playerDead;
        m.setSolo(last);
    }
    if (countdown > 0.f) lv = 0.45f;
    if (zenith) lv = std::max(lv, 1.15f);    // !!!!!: the techno never lets up, kick, bass, clap and acid from the first second
    if (morning && victoryDelay < 0.f && !victory) lv = std::max(lv, 1.32f);   // MORNINGSTAR: everything, stabs and all
    if (victory) lv = 0.55f;
    if (playerDead) lv = 0.1f;
    m.setIntensity(lv);
    m.setMuffle(paused || armoryOpen || playerDead);
}

// Rising into the Zenith: the Leviathan sinks (3 s), ACT II COMPLETE (2 s), the
// eclipse breaks into a white-out (1 s), then you stand in the Zenith. A death
// before then (a last spit) retries the Zenith, never the Maw.
inline void GameplayState::updateAscension(float dt) {
    if (ascendT < 0.f || playerDead || paused) return;
    float was = ascendT; ascendT += dt;
    if (was < ASCEND_SINK && ascendT >= ASCEND_SINK) audio.play("wave", 128, SoundGroup::UI);
    if (was < ASCEND_SINK + ASCEND_CARD && ascendT >= ASCEND_SINK + ASCEND_CARD) {
        audio.duck(8.f, 1.5f);
        shake(0.6f, 0.05f);
    }
    if (ascendT >= ASCEND_SINK + ASCEND_CARD + ASCEND_WHITE) {
        enterArena(ZENITH_ARENA);
        pushBanner("THE ECLIPSE BREAKS", "SOMETHING ABOVE IT DOES NOT", {1.f, 0.95f, 0.85f}, 3.5f);
        captureMouse(true);
    }
}
