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
        if (reach > FINISH_REACH || facing < 0.62f) continue;
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
        float v = std::min(20.f, reach * 7.f);
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

// ---- OVERDRIVE -------------------------------------------------------------------
inline void GameplayState::enterOverdrive() {
    ++runLog.overdrives;
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
    killFx = std::max(0.f, killFx - dt / 0.2f);
    float hint = !playerDead && punchCooldown <= 0.f && findFinisher() >= 0 ? 1.f : 0.f;
    finishHint += (hint - finishHint) * (1.f - std::exp(-dt * 18.f));
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
