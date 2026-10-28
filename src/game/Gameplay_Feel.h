#pragma once
// =============================================================================
// Gameplay_Feel.h — GameplayState: how a fight feels and flows.
//
//   HITS       every hit shoves and flinches what it lands in (HitReact.h);
//              a thump at the body, heavier on the big ones; enough damage
//              close together STUNS a regular enemy and loses its attack.
//   TOKENS     only a few regular enemies wind up at once; the nearest you can
//              see go first, the ones behind you are slower to get to it.
//   FINISHERS  an enemy low enough (or stunned) glows: F on it finishes it,
//              a lunge and a crunch, health back and the gun full.
//   OVERDRIVE  a full style meter: harder, faster shots that never need a
//              reload, every hit staggers, finishers on anything under half;
//              kills keep it burning a little longer.
//   RUN LOG    what hurt you, what killed you, each gun's numbers (RunLog.h).
// Included at the end of GameplayState.h.
// =============================================================================

// A hit's thump, kept for the end of the tick: the hardest of them, at the body it hit
inline void GameplayState::bodyHit(const Enemy& e, glm::vec3 at, float push, bool stunned) {
    if (!e.reacts()) return;
    float mass = hitreact::bodyOf(e.type).mass * e.scale * e.scale;
    if (push > bodyHitPush) { bodyHitAt = at; bodyHitMass = mass; }
    bodyHitPush += push;   // (ten pellets in one body: one big thump)
    bodyHitStun |= stunned;
}

// Its footing breaks: the attack it was winding up is gone
inline void GameplayState::onStunned(Enemy& e, glm::vec3 at) {
    ++runLog.stuns;
    teach(TEACH_STUN);
    const bool heavy = hitreact::bodyOf(e.type).heavy;
    fx.spawnBurst(at, {1.f, 0.85f, 0.45f}, heavy ? 26 : 12, heavy ? 8.f : 5.f, 0.3f, 12.f);
    fx.spawnBurst(e.position + glm::vec3{0.f, e.height() + 0.2f, 0.f}, e.stats().glow, 8, 2.5f, 0.45f, -3.f);   // its eyes sparking out
    styleSystem.addStyle(heavy ? 12.f : 5.f);
    if (heavy) {   // a big one buckling: the world catches for it
        hitStopFrames = std::max(hitStopFrames, 3);
        shake(0.15f, 0.03f);
        fx.spawnShockwave(e.position + glm::vec3{0.f, 0.08f, 0.f}, 2.2f + e.radius(), {1.f, 0.75f, 0.35f});
    }
}

// ---- attack tokens -----------------------------------------------------------
inline int GameplayState::attackersAllowed() const {
    static const int K[DIFFICULTY_LEVELS] = {2, 3, 4, 5};   // LENIENT .. BRUTAL
    int d = settings ? std::clamp(settings->difficulty, 0, DIFFICULTY_LEVELS - 1) : DIFFICULTY_DEFAULT;
    int crowd = 0;
    for (const auto& e : enemies) if (e.targetable() && e.reacts()) ++crowd;
    return K[d] + (crowd > 9 ? 1 : 0);
}

inline void GameplayState::assignAttackers() {
    struct Cand { int i; float score; };
    static std::vector<Cand> cands;
    cands.clear();
    int busy = 0;
    glm::vec3 eye = player.camera.position, fwd = player.camera.flatForward();
    for (int i = 0; i < (int)enemies.size(); ++i) {
        Enemy& e = enemies[i];
        e.mayAttack = true; e.attackPace = 1.f;
        if (!e.alive || !e.reacts() || e.type == EnemyType::MITE || e.type == EnemyType::CONDUCTOR) continue;   // (mites just run in; conductors never attack)
        if (e.telegraphTimer > 0.f || e.burstLeft > 0 || e.diveTimer > 0.f || e.beamTimer > 0.f) { ++busy; continue; }
        glm::vec3 to = e.position - eye; to.y = 0.f;
        float d = glm::length(to);
        bool unseen = d > 3.f && glm::dot(fwd, to / std::max(d, 1e-3f)) < 0.5f;   // outside ~60 degrees of where you look
        if (unseen) e.attackPace = 0.6f;
        cands.push_back({i, d + (unseen ? 25.f : 0.f)});
    }
    int free = std::max(0, attackersAllowed() - busy);
    std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score < b.score; });
    for (int k = free; k < (int)cands.size(); ++k) enemies[cands[k].i].mayAttack = false;
}

// ---- finishers -----------------------------------------------------------------
inline int GameplayState::findFinisher() const {
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();
    int best = -1; float bestScore = 1e9f;
    for (int i = 0; i < (int)enemies.size(); ++i) {
        const Enemy& e = enemies[i];
        if (!e.finishable(overdriveOn())) continue;
        glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.2f), 0};
        glm::vec3 d = c - eye;
        float len = glm::length(d);
        if (len < 1e-3f) continue;
        float reach = len - e.radius(), facing = glm::dot(fwd, d / len);
        if (reach > FINISH_REACH || facing < 0.55f) continue;
        float score = (1.f - facing) * 12.f + reach * 0.4f;   // the one under the crosshair, then the nearest
        if (score < bestScore) { bestScore = score; best = i; }
    }
    return best;
}

inline bool GameplayState::tryFinisher() {
    int i = findFinisher();
    if (i < 0) return false;
    Enemy& e = enemies[i];
    glm::vec3 eye = player.camera.position;
    glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.2f), 0};
    glm::vec3 to = c - eye;
    float len = std::max(glm::length(to), 1e-3f);
    glm::vec3 dir = to / len;
    // The lunge: into it, the way the sword's thrust carries you
    glm::vec3 flat{dir.x, 0.f, dir.z};
    float reach = len - e.radius();
    if (glm::length(flat) > 1e-3f && reach > 1.f) {
        flat = glm::normalize(flat);
        float v = std::min(24.f, reach * 7.5f);
        player.velocity.x = flat.x * v; player.velocity.z = flat.z * v;
        dashMomentumTimer = std::max(dashMomentumTimer, 0.14f);
        fovKick = std::max(fovKick, 7.f);
    }
    const bool heavy = hitreact::bodyOf(e.type).heavy;
    HitPush hp{glm::normalize(dir + glm::vec3{0.f, 0.25f, 0.f}), heavy ? 60.f : 26.f, 0.f};
    float hpBefore = styleSystem.health;
    hurtEnemy(e, e.health + 1.f, c, 0.f, 0.f, StyleSource::PUNCH, false, true, &hp);
    styleSystem.heal((heavy ? 35.f : 25.f) * tune().heal);
    float healed = styleSystem.health - hpBefore;
    styleSystem.addStyle(heavy ? 60.f : 40.f, StyleSource::PUNCH);
    {   // the gun in hand comes back full (a reload under way is done)
        WeaponState& ws = weapons[activeWeapon];
        int mag = weaponMag((WeaponId)activeWeapon, prog.up[activeWeapon]);
        if (ws.reloading || ws.ammo < mag) { ws.reloading = false; ws.reloadTimer = 0.f; ws.ammo = mag; viewModel.cancelReload(); }
    }
    ++runLog.finishers; runLog.finisherHeal += healed;
    invincFrames  = std::max(invincFrames, 0.4f);   // nothing lands on you mid-finisher
    hitStopFrames = std::max(hitStopFrames, heavy ? 8 : 6);
    punchCooldown = 0.18f;                          // chain the next one
    killFx = 1.f;
    viewModel.triggerParry(true);
    camFeel.punch(58.f, frand(-1.f, 1.f) * 20.f, -40.f);
    shake(heavy ? 0.35f : 0.22f, heavy ? 0.07f : 0.045f);
    fx.spawnSpray(c, dir, {1.f, 0.7f, 0.3f}, heavy ? 46 : 30, 15.f, 0.4f, 0.45f);
    fx.spawnBurst(c, {1.f, 0.95f, 0.8f}, 14, 6.f, 0.18f, 0.f);
    fx.spawnShockwave(e.position + glm::vec3{0.f, 0.08f, 0.f}, heavy ? 4.f : 2.6f, {1.f, 0.6f, 0.2f});
    explosionFlashTimer = 0.16f; explosionFlashPos = c;
    audio.play("finisher", heavy ? 128 : 115, SoundGroup::PLAYER);
    if (heavy) audio.duck(4.f, 0.25f);
    char buf[48];
    if (healed >= 1.f) std::snprintf(buf, sizeof(buf), "FINISHER  +%d HP", (int)std::round(healed));
    else std::snprintf(buf, sizeof(buf), "FINISHER");
    ui.feed(buf, {1.f, 0.65f, 0.25f});
    return true;
}

inline bool GameplayState::finishKey() {
    if (playerDead || victory || paused || hitStopFrames > 0 || punchCooldown > 0.f) return false;
    return tryFinisher();
}

// ---- OVERDRIVE -------------------------------------------------------------------
inline void GameplayState::enterOverdrive() {
    ++runLog.overdrives;
    teach(TEACH_OVERDRIVE);
    ui.onOverdrive();
    audio.play("overdrive_on", 128, SoundGroup::UI);
    audio.duck(6.f, 0.5f);
    slowMo(0.5f, 0.35f);
    hitStopFrames = std::max(hitStopFrames, 3);
    camFeel.punch(-42.f, 0.f, frand(-1.f, 1.f) * 18.f);
    shake(0.3f, 0.05f);
    fovKick = std::max(fovKick, 10.f);
    fx.spawnShockwave(player.position + glm::vec3{0.f, 0.1f, 0.f}, 9.f, {1.f, 0.65f, 0.15f});
    fx.spawnBurst(player.camera.position, {1.f, 0.7f, 0.2f}, 40, 9.f, 0.6f, -2.f);
    for (int w = 0; w < WEAPON_COUNT; ++w) {   // every gun full: it won't need a reload until it's over
        weapons[w].reloading = false; weapons[w].reloadTimer = 0.f;
        weapons[w].ammo = weaponMag((WeaponId)w, prog.up[w]);
    }
    viewModel.cancelReload();
    dashCharges = 2;
}

inline void GameplayState::leaveOverdrive() {
    audio.play("overdrive_off", 110, SoundGroup::UI);
}

// Per frame: OVERDRIVE starting and ending, the screen's gold and kick, the finisher hint
inline void GameplayState::updateFeel(float dt) {
    if (styleSystem.overdriveBegan) { styleSystem.overdriveBegan = false; if (!playerDead && !victory) enterOverdrive(); }
    if (styleSystem.overdriveEnded) { styleSystem.overdriveEnded = false; if (!playerDead) leaveOverdrive(); }
    float want = overdriveOn() && !playerDead ? 1.f : 0.f;
    overdriveFx += (want - overdriveFx) * (1.f - std::exp(-dt * (want > overdriveFx ? 9.f : 2.5f)));
    if (overdriveOn()) runLog.overdriveTime += dt;
    // OVERDRIVE's cues: three ticks as it burns out (1.5, 1 and 0.5 s left, each higher), and a chime
    // when its cooldown runs out and it can burn again (not if it starts at once: the meter was full)
    if (overdriveOn() && !playerDead) {
        const float left = styleSystem.overdriveTimer;
        for (int k = 0; k < 3; ++k) {
            const float at = 1.5f - 0.5f * (float)k;
            if (driveLeftPrev > at && left <= at) {
                SfxMixer::Opts o; o.volume = 0.9f; o.group = SoundGroup::UI; o.pitch = 1.f + 0.12f * (float)k;
                audio.playOpts("drive_warn", o);
            }
        }
        driveLeftPrev = left;
    } else driveLeftPrev = 0.f;
    if (driveCoolPrev > 0.f && driveCoolPrev < 0.5f && styleSystem.driveCool <= 0.f && !overdriveOn() && !playerDead && !victory) {
        audio.play("drive_ready", 110, SoundGroup::UI);
        ui.feed("OVERDRIVE RECHARGED", {1.f, 0.7f, 0.2f});
    }
    driveCoolPrev = styleSystem.driveCool;
    killFx = std::max(0.f, killFx - dt / 0.2f);
    int fi = !playerDead && punchCooldown <= 0.f ? findFinisher() : -1;
    float hint = fi >= 0 ? 1.f : 0.f;
    finishHint += (hint - finishHint) * (1.f - std::exp(-dt * 18.f));
    finishCueCd = std::max(0.f, finishCueCd - dt);
    if (fi >= 0) teach(TEACH_FINISH);
    static bool devTaught = false;
    if (g_devOverlay.rfind("teach", 0) == 0 && elapsedTime > 1.5f && !devTaught) {
        devTaught = true;
        if (g_devOverlay == "teach2") { teach(TEACH_HOOK); teach(TEACH_ALT); } else { teach(TEACH_FINISH); teach(TEACH_OVERDRIVE); }
    }
    if (fi >= 0 && enemies[fi].uid != finishReadyUid && finishCueCd <= 0.f) {   // a new one in reach: a soft, low cue
        finishReadyUid = enemies[fi].uid; finishCueCd = 0.5f;
        audio.playAt("finish_ready", enemies[fi].position + glm::vec3{0.f, enemies[fi].height() * 0.6f, 0.f}, 100, SoundGroup::UI, true);
    }
    if (fi < 0 && finishCueCd <= 0.f) finishReadyUid = -1;
}

// ---- the run log -------------------------------------------------------------------
inline void GameplayState::logRun(const char* how) {
    const bool force = std::getenv("OVERDRIVE_RUNLOG") != nullptr;   // dev: log screenshot / dump runs too
    if (runLog.any() && ((!g_devNoMouse && !g_godMode) || force)) {
        static const char* MODE[] = {"ARENA", "FAST", "ENDLESS", "ACT2"};
        int a = std::clamp(director.arena, 0, std::max(0, (int)level.arenas.size() - 1));
        const char* arena = level.arenas.empty() ? "" : level.arenas[a].name;
        RunLog::append(runLog.json(how, g_bossRun >= 0 ? "BOSS" : MODE[(int)mode], settings ? settings->difficulty : DIFFICULTY_DEFAULT,
                                   arena, director.wave + 1, elapsedTime));
    }
    runLog.reset(elapsedTime);
}

// ---- movement into combat --------------------------------------------------------
// The enemy the grapple would hook: under the crosshair (a little give at range),
// in reach, with nothing solid in the way. Bosses have their own fights
inline int GameplayState::findEnemyHook() const {
    glm::vec3 o = player.camera.position, d = player.camera.forward();
    int best = -1; float bestT = HOOK_RANGE;
    for (int i = 0; i < (int)enemies.size(); ++i) {
        const Enemy& e = enemies[i];
        if (!e.targetable() || (isBoss(e.type) && e.type != EnemyType::PACEMAKER)) continue;   // (THE PACEMAKER: you zip to it)
        glm::vec3 c = e.position + glm::vec3{0.f, e.height() * 0.5f, 0.f};
        float t = glm::dot(c - o, d);
        if (t < 2.f || t > bestT) continue;
        float miss = glm::length(c - (o + d * t));
        if (miss > std::max(e.radius() + 0.45f, std::min(e.height() * 0.5f, 1.4f)) + t * 0.025f) continue;   // within its body, plus a little give at range
        bool clear = true;
        for (const auto& w : level.walls) { float wt = rayBoxHit(o, (c - o) / glm::length(c - o), w.box); if (wt > 0.f && wt < glm::length(c - o) - e.radius()) { clear = false; break; } }
        if (!clear) continue;
        bestT = t; best = i;
    }
    return best;
}

inline void GameplayState::hookEnemy(int i) {
    Enemy& e = enemies[i];
    glm::vec3 eye = player.camera.position;
    glm::vec3 c = e.position + glm::vec3{0.f, std::min(e.height() * 0.5f, 1.2f), 0.f};
    glm::vec3 toMe = eye - c;
    float dist = glm::length(toMe);
    glm::vec3 dir = toMe / std::max(dist, 1e-3f);
    float mass = hitreact::bodyOf(e.type).mass * e.scale * e.scale;
    viewModel.triggerGrapple();
    audio.play("grapple_fire");
    styleSystem.addStyle(6.f);
    if (e.reacts() && mass <= HOOK_PULL_MASS) {   // light: dragged to your feet, stunned on arrival
        glm::vec3 to = c + dir * std::max(0.f, dist - 2.6f);                  // its centre, 2.6 m in front of you
        if (e.stats().flying) to -= glm::vec3{0.f, std::min(e.height() * 0.5f, 1.2f), 0.f};   // (a flyer: out of the sky to eye level)
        else to.y = e.position.y;                                             // (on foot: along the ground)
        float t = e.yank(to);
        e.takeHit(-dir, 3.f, 0.f, 0.f, false, gameClock);   // the jerk of the hook: a flinch toward you
        yankUid = e.uid; yankT = t + 0.08f;
        grappleCooldown = 0.8f;
        audio.playAt("hook_yank", c, 120, SoundGroup::PLAYER, true);
        fx.spawnBurst(c, {0.5f, 1.f, 1.f}, 12, 5.f, 0.25f, 0.f);
        camFeel.punch(-26.f, 0.f, frand(-1.f, 1.f) * 12.f);   // you haul on it
    } else {                                      // heavy: the rope takes you to it
        glm::vec3 impulse{0.f};
        grapple.attach(eye, c, impulse);
        grapple.enemyUid = e.uid;
        impulse.y = std::min(impulse.y, 8.f);     // (a flat zip, not the wall hook's lob)
        player.velocity = impulse;
        player.launched();
        invincFrames = std::max(invincFrames, 0.2f);
        fovKick = std::max(fovKick, 9.f);
        audio.playAt("hook_yank", c, 100, SoundGroup::PLAYER, true);
        camFeel.punch(-34.f);
    }
}

inline void GameplayState::updateEnemyZip() {
    if (!grapple.active || grapple.enemyUid < 0) return;
    const Enemy* z = nullptr;
    for (const auto& e : enemies) if (e.uid == grapple.enemyUid) { z = &e; break; }
    if (!z || !z->targetable()) { grapple.release(); return; }
    grapple.target = z->position + glm::vec3{0.f, std::min(z->height() * 0.5f, 1.2f), 0.f};
    glm::vec3 d = grapple.target - player.camera.position;
    if (glm::length(glm::vec2{d.x, d.z}) < z->radius() + 2.3f) {   // arrived: let go in its face, ready to hit it
        grapple.release();
        glm::vec3 v = player.velocity;
        player.velocity = glm::vec3{v.x * 0.15f, std::max(v.y * 0.3f, 3.5f), v.z * 0.15f};
        invincFrames = std::max(invincFrames, 0.3f);
        camFeel.punch(30.f);
        shake(0.1f, 0.02f);
        styleSystem.addStyle(6.f);
    }
}

// Sliding into one: a kick that shoves it on and breaks a light one's footing; a heavy one stops you dead
inline void GameplayState::slideKicks() {
    if (!player.sliding) { slideKicked.clear(); return; }
    float spd = glm::length(glm::vec2{player.velocity.x, player.velocity.z});
    if (!player.onGround || spd < 7.5f) return;
    glm::vec3 fwd = glm::normalize(glm::vec3{player.velocity.x, 0.f, player.velocity.z});
    for (auto& e : enemies) {
        if (!e.targetable() || !e.reacts() || e.stats().flying) continue;
        if (std::find(slideKicked.begin(), slideKicked.end(), e.uid) != slideKicked.end()) continue;
        glm::vec3 d = e.position - player.position;
        if (std::fabs(d.y) > 1.6f || glm::length(glm::vec2{d.x, d.z}) > e.radius() + 0.9f) continue;
        slideKicked.push_back(e.uid);
        const bool heavy = hitreact::bodyOf(e.type).heavy;
        glm::vec3 at = e.position + glm::vec3{0.f, std::min(e.height() * 0.35f, 0.9f), 0.f};
        HitPush hp{glm::normalize(fwd + glm::vec3{0.f, 0.35f, 0.f}), 12.f + spd * 0.4f, 3.f};
        hurtEnemy(e, 20.f, at, 10.f, 1.f, StyleSource::SLAM, false, false, &hp);
        audio.playAt("punch", at, 110, SoundGroup::PLAYER, true);
        camFeel.punch(heavy ? 40.f : 22.f, 0.f, frand(-1.f, 1.f) * 16.f);
        shake(heavy ? 0.18f : 0.1f, heavy ? 0.04f : 0.02f);
        hitStopFrames = std::max(hitStopFrames, heavy ? 3 : 2);
        fx.spawnSpray(at, fwd, {1.f, 0.85f, 0.6f}, 12, 7.f, 0.3f);
        if (heavy) { player.velocity.x *= 0.3f; player.velocity.z *= 0.3f; }   // like kicking a wall
    }
}

// A slam lands: the further it fell, the harder and wider it hits (and it throws them out and up)
inline void GameplayState::slamLands() {
    float h = std::max(0.f, slamFromY - player.position.y);
    float dmg = 30.f + std::min(50.f, h * 4.f), r = 4.5f + std::min(2.5f, h * 0.15f);
    for (auto& e : enemies) {
        if (!e.targetable()) continue;
        glm::vec3 off = e.position - player.position;
        float d = glm::length(off);
        if (d >= r) continue;
        glm::vec3 out = glm::length(glm::vec2{off.x, off.z}) > 1e-3f ? glm::normalize(glm::vec3{off.x, 0.f, off.z}) : glm::vec3{1.f, 0.f, 0.f};
        HitPush hp{glm::normalize(out + glm::vec3{0.f, 0.8f, 0.f}), (10.f + std::min(10.f, h * 0.5f)) * (1.f - 0.5f * d / r), 2.5f};
        hurtEnemy(e, dmg * (1.f - 0.4f * d / r), e.position + glm::vec3{0, e.height() * 0.5f, 0}, 15.f, 2.f, StyleSource::SLAM, false, false, &hp);
    }
    fx.spawnShockwave(player.position, r, {1.f, 0.8f, 0.4f});
    if (h > 6.f) fx.spawnBurst(player.position + glm::vec3{0.f, 0.2f, 0.f}, {1.f, 0.85f, 0.5f}, 30, 9.f, 0.4f, 12.f);
    shake(0.3f + std::min(0.3f, h * 0.02f), 0.08f);
    hitStopFrames = std::max(hitStopFrames, h > 6.f ? 3 : 1);
}

// ---- unseen wind-ups ---------------------------------------------------------------
// A regular enemy within 40 m winding up an attack where you can't see it: outside ~60 degrees of where you look
inline bool GameplayState::unseenWindup(const Enemy& e, glm::vec3 flatFwd) const {
    if (e.telegraphTimer <= 0.f || e.type == EnemyType::MITE) return false;
    glm::vec3 to = e.position - player.camera.position; to.y = 0.f;
    float d = glm::length(to);
    return d > 2.f && d < 40.f && glm::dot(flatFwd, to / d) < 0.5f;
}

// ---- alt fire ------------------------------------------------------------------------
inline void GameplayState::altFire() {
    WeaponState& ws = weapons[activeWeapon];
    if (!ws.ready()) {
        if ((ws.ammo <= 0 || ws.reloading) && dryCd <= 0.f) { audio.play("dry", 100, SoundGroup::PLAYER); dryCd = 0.3f; }
        return;
    }
    if (activeWeapon == (int)WeaponId::REVOLVER) {   // FAN: the rest of the cylinder (six at most), one every 75 ms
        fanLeft = overdriveOn() ? 6 : std::min(ws.ammo, 6);
        fanTimer = ViewModel::FAN_ENTER;   // (the first when the left hand reaches the hammer)
        viewModel.triggerFan(fanLeft);
        audio.play("cyl_close", 80, SoundGroup::PLAYER);   // the palm on the hammer
    } else if (activeWeapon == (int)WeaponId::SHOTGUN) {   // BOTH BARRELS (one shell left: just the one)
        doubleShot = ws.ammo >= 2 || overdriveOn();
        fireWeapon(activeWeapon);
        doubleShot = false;
    }
}

// ---- teaching --------------------------------------------------------------------------
// The first time a move comes up: one small card at the left edge, then never again
inline void GameplayState::teach(int bit) {
    const bool dev = g_devOverlay.rfind("teach", 0) == 0;   // dev: --overlay teach / teach2: the cards, every time (screenshots)
    if (!dev && (!settings || (settings->taught & bit) || g_devNoMouse || fast())) return;
    if (!dev) { settings->taught |= bit; settings->save(); }
    const Bindings& kb = binds();
    const glm::vec3 c{1.f, 0.65f, 0.25f};
    switch (bit) {
        case TEACH_FINISH:    ui.introduce("FINISHER", kb.label(Act::INTERACT) + " ON A GLOWING ENEMY - HEALTH BACK, GUN RELOADED", c, "NEW MOVE"); break;
        case TEACH_STUN:      ui.introduce("STAGGER", "HITS CLOSE TOGETHER BREAK ITS ATTACK", c, "NEW MOVE"); break;
        case TEACH_OVERDRIVE: ui.introduce("OVERDRIVE", "HARDER, FASTER, NO RELOADS - KILLS KEEP IT BURNING", c, "NEW MOVE"); break;
        case TEACH_HOOK:      ui.introduce("GRAPPLE AN ENEMY", kb.label(Act::GRAPPLE) + " - LIGHT ONES COME TO YOU, YOU GO TO HEAVY ONES", c, "NEW MOVE"); break;
        case TEACH_ALT:       ui.introduce("ALT FIRE", "RMB - THE REVOLVER FANS ITS HAMMER, THE SHOTGUN FIRES BOTH BARRELS", c, "NEW MOVE"); break;
        default: break;
    }
}
