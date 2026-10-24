#pragma once
// =============================================================================
// Gameplay_Sword.h — GameplayState: OATHBREAKER in the game (Sword.h). Key 5
// draws it (only in the Zenith, or anywhere with --sword); LMB strokes and the
// thrust, RMB the parry and the block, a slam with it out a plunge. Strokes
// hit what's in their arc and heal 6 a hit (a gun hit heals 2).
// Included at the end of GameplayState.h.
// =============================================================================

inline bool GameplayState::swordAllowed() const {
    return swordKeyAllowed(g_devSword, act2(), director.arena, ZENITH_ARENA);
}
inline void GameplayState::drawSword() {
    if (!swordAllowed() || swordOut) return;
    swordOut = true; sword.drawn(); aim = 0.f;
    pendingWeapon = -1;
    lmbPrev = rmbPrev = true;   // the click or key that drew it isn't a stroke
    audio.play("sword_draw", 110);
}
inline void GameplayState::holsterSword() {
    if (!swordOut) return;
    swordOut = false; sword.holster(); swordRoll = 0.f;
    player.horizontalSpeed = 7.f;
}

inline void GameplayState::updateSword(float dt) {
    if (!swordOut) return;
    if (!swordAllowed()) { holsterSword(); return; }
    Uint32 mb = g_devNoMouse ? 0 : SDL_GetMouseState(nullptr, nullptr);
    bool lmb = (mb & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0 || gamepad::state().fire;
    bool rmb = (mb & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || gamepad::state().aim;
    if (paused || playerDead || victory) lmb = rmb = false;
    SwordInput in{lmb && !lmbPrev, lmb, rmb && !rmbPrev, rmb};
    lmbPrev = lmb; rmbPrev = rmb;
    if (in.guardPressed && !sword.broken()) audio.play("sword_guard", 80);
    if (in.guardPressed && act2() && director.arena == ZENITH_ARENA) {   // timing: a press just after a white blow hit you was late
        abLastPressT = gameClock;
        if (gameClock - abWhiteHitT < 0.25f) { ui.feed("LATE", {1.f, 1.f, 1.f}); audio.play("ab_tsk", 110, SoundGroup::UI); }
    }
    swordRoll *= std::exp(-dt * 9.f);
    Sword::Out o = sword.update(dt, in);
    viewModel.tickSword(dt, swordShownFor(sword));
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();

    // A projectile met in the parry window goes back where you look (the punch's rules)
    if (sword.parryOpen()) {
        int pi = findParryTarget();
        if (pi >= 0 && sword.tryParry()) { reflectProjectile(pi, fwd); styleSystem.heal(Sword::HEAL_PARRY * tune().heal); }
    }
    if (o.stroke >= 0) {
        audio.play("sword_swing", 110);
        const Enemy* near = nullptr; float nd = 1e9f;
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            float d = glm::length(e.position - player.position);
            if (d < nd) { nd = d; near = &e; }
        }
        if (near) {   // the lock-on step: carried toward him over the stroke
            glm::vec3 st = Sword::lockStep(player.position, fwd, near->position, near->radius());
            player.velocity.x += st.x / Sword::SWING_TIME; player.velocity.z += st.z / Sword::SWING_TIME;
        }
        fx.slashes.push_back({player.position, std::atan2(fwd.x, fwd.z), 10 + o.stroke, 0.f});   // kinds 10-12: the player's
        swordRoll = o.stroke == 0 ? 2.2f : o.stroke == 1 ? -2.2f : 0.f;   // the head leans into the cut
        if (o.stroke == 2) fovKick = std::max(fovKick, 3.f);
        bool landed = false;
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.3f), 0};
            if (!Sword::inArc(eye, fwd, c, e.radius())) continue;
            hurtEnemy(e, sword.strokeDamage(o.stroke), c, 12.f, Sword::HEAL_HIT, StyleSource::MELEE);
            fx.spawnHitSparks(c, gunkit::SWORD_GLOW * 1.6f);
            landed = true;
        }
        for (int m = (int)abh.mirages.size() - 1; m >= 0; --m)   // a stroke through a mirage ends it
            if (Sword::inArc(eye, fwd, abh.mirages[m].at + glm::vec3{0.f, 1.3f, 0.f}, 0.6f)) { abh.killMirage(m); landed = true; }
        if (landed) { audio.play("sword_hit", 120); hitStopFrames = std::max(hitStopFrames, 3); shake(0.1f, 0.02f); }
    }
    if (o.thrust) {
        glm::vec3 flat{fwd.x, 0.f, fwd.z};
        flat = glm::length(flat) > 1e-3f ? glm::normalize(flat) : glm::vec3{0.f, 0.f, -1.f};
        thrustFrom = player.position;
        player.velocity.x = flat.x * (Sword::THRUST_DIST / Sword::THRUST_TIME);
        player.velocity.z = flat.z * (Sword::THRUST_DIST / Sword::THRUST_TIME);
        dashMomentumTimer = std::max(dashMomentumTimer, Sword::THRUST_TIME);
        thrustHit = false;
        audio.play("sword_thrust", 120);
        fovKick = std::max(fovKick, 8.f);
    }
    if (sword.thrustT > 0.f && !thrustHit) {
        for (auto& e : enemies) {
            if (!e.targetable() || !Sword::thrustHits(thrustFrom, player.position, e.position + glm::vec3{0, 1.f, 0}, e.radius())) continue;
            hurtEnemy(e, sword.thrustDamage(), e.position + glm::vec3{0, 1.2f, 0}, 25.f, Sword::HEAL_HIT, StyleSource::MELEE);
            audio.play("sword_hit", 128); hitStopFrames = std::max(hitStopFrames, 5); shake(0.2f, 0.04f);
            thrustHit = true;
            break;
        }
    }
    player.horizontalSpeed = 7.f * sword.moveMult();
}

// The slam's landing with the sword out (Gameplay_Tick.h)
inline void GameplayState::swordPlunge(glm::vec3 at) {
    fx.spawnShockwave(at, Sword::PLUNGE_RADIUS, gunkit::SWORD_GLOW);
    audio.play("sword_hit", 128);
    for (auto& e : enemies)
        if (e.targetable() && glm::length(glm::vec2{e.position.x - at.x, e.position.z - at.z}) < Sword::PLUNGE_RADIUS + e.radius() &&
            std::fabs(e.position.y - at.y) < 2.5f)
            hurtEnemy(e, Sword::PLUNGE_DAMAGE, e.position + glm::vec3{0, 1.f, 0}, 20.f, Sword::HEAL_HIT, StyleSource::SLAM);
}

// Damage a raised sword can meet: parried (white, in the window), blocked from
// the front (25 %), or breaking the guard. `dt` for a beam's drain.
inline bool GameplayState::damagePlayerStrike(float dmg, glm::vec3 from, Sword::Strike k, float dt) {
    lastGuarded = Sword::Guarded::NONE;
    if (invincFrames > 0.f || playerDead || g_godMode || victory || ascendT >= 0.f) return false;   // (nothing lands: the guard isn't spent)
    if (swordOut) {
        glm::vec3 to = from - player.position; to.y = 0.f;
        glm::vec3 f = player.camera.forward(); f.y = 0.f;
        bool front = glm::length(to) < 0.3f || glm::length(f) < 1e-3f || glm::dot(glm::normalize(to), glm::normalize(f)) > 0.2f;
        const bool tryingParry = sword.parryOpen();
        lastGuarded = sword.receive(k, front, dt);
        if (lastGuarded == Sword::Guarded::PARRIED) {
            viewModel.triggerParry(true);
            parryFeedback(player.camera.position + player.camera.forward() * 1.2f, true);
            styleSystem.heal(Sword::HEAL_PARRY * tune().heal);
            return false;
        }
        if (k == Sword::Strike::GOLD && tryingParry) {   // the rule teaches itself
            audio.play("ab_wrong", 120, SoundGroup::UI, true);
            ui.feed("UNPARRYABLE", abGoldTell({1.f, 0.85f, 0.4f}));
        }
        if (lastGuarded != Sword::Guarded::NONE) {
            if (k != Sword::Strike::BEAM || std::fmod(gameClock, 0.15f) < dt)
                fx.spawnHitSparks(player.camera.position + player.camera.forward() * 0.8f, gunkit::SWORD_GLOW * 1.4f);
            if (k != Sword::Strike::BEAM || lastGuarded == Sword::Guarded::BROKE) {
                if (lastGuarded == Sword::Guarded::BROKE) audio.play("sword_break", 128);
                else {
                    SfxMixer::Opts o; o.volume = 120.f / 128.f; o.group = SoundGroup::PLAYER; o.pitch = 0.8f + 0.3f * sword.guard01();   // lower as it drains
                    audio.playOpts("sword_block", o);
                    if (sword.guard01() < 0.3f) audio.play("ab_creak", 120, SoundGroup::PLAYER, true);   // near empty: it creaks
                }
            }
            if (lastGuarded == Sword::Guarded::BROKE) ui.toast("GUARD BROKEN", "", {1.f, 0.3f, 0.4f}, 1.f);
            dmg *= Sword::BLOCK_TAKEN;
        }
    }
    if (k == Sword::Strike::WHITE && act2() && director.arena == ZENITH_ARENA) {   // timing: pressed, but too early
        if (gameClock - abLastPressT > Sword::PARRY_WINDOW && gameClock - abLastPressT < Sword::PARRY_WINDOW + 0.25f) {
            ui.feed("EARLY", {1.f, 1.f, 1.f}); audio.play("ab_tsk", 110, SoundGroup::UI);
        }
        if (lastGuarded == Sword::Guarded::NONE) abWhiteHitT = gameClock;   // a blocked blow didn't hit you: no LATE for it
    }
    return damagePlayer(dmg, from, k == Sword::Strike::BEAM ? 0.f : 0.25f, k == Sword::Strike::BEAM ? 0.f : 0.05f,
                        k == Sword::Strike::BEAM ? 0.f : 0.15f);
}
