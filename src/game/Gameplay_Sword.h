#pragma once
// =============================================================================
// Gameplay_Sword.h — GameplayState: OATHBREAKER in the game (Sword.h). Key 5
// draws it (only in the Zenith, or anywhere with --sword); LMB strokes and the
// thrust, RMB the parry and the block, a slam with it out a plunge. Strokes
// hit what's in their arc and heal 6 a hit (a gun hit heals 2).
// Included at the end of GameplayState.h.
// =============================================================================

inline bool GameplayState::swordAllowed() const {
    if (laboratory() && storySword && !labEnding.active()) return true;   // (Story: pulled from its statue in the Successor's arena)
    return swordKeyAllowed(g_devSword, act2(), director.arena, ZENITH_ARENA);
}

// ---- OATHBREAKER's statue (Chapter 3, the Successor's arena: LevelSuccessor.h) ----
inline glm::vec3 GameplayState::swordStatueAt() const { return lab::swordStatue(); }

// The seal breaks during the Successor's second phase.
inline void GameplayState::openSwordStatue() {
    if (swordStatueOpen || storySword) return;
    swordStatueOpen = true; swordStatueFlash = 1.f;
    if (swordStatueIt >= 0 && swordStatueIt < (int)interactables.size()) interactables[swordStatueIt].label = "PULL OATHBREAKER";
    audio.playAt("sword_break", lab::swordStatueGrip(), 128, SoundGroup::WORLD, true);
    audio.playAt("anvil", lab::swordStatueGrip(), 120, SoundGroup::WORLD, true);
    ui.toast("THE STATUE'S SEAL BREAKS", "OATHBREAKER CAN BE PULLED", {1.f, 0.35f, 0.5f}, 3.f);
    ui.feed("OATHBREAKER / BACK LEFT OF THE FLOOR", {1.f, 0.4f, 0.55f});
}

inline void GameplayState::pullStatueSword() {
    if (storySword) return;
    if (!swordStatueOpen) {
        ui.feed("SEALED / IT WILL NOT MOVE YET", {1.f, 0.35f, 0.4f});
        audio.play("dry", 110, SoundGroup::UI, true);
        return;
    }
    storySword = true; swordStatueFlash = 1.f;
    if (swordStatueIt >= 0 && swordStatueIt < (int)interactables.size()) interactables[swordStatueIt].active = false;
    audio.playAt("sword_break", lab::swordStatueGrip(), 110, SoundGroup::WORLD, true);
    fx.spawnBurst(lab::swordStatueGrip(), gunkit::SWORD_GLOW * 1.6f, 50, 8.f, 0.7f, 6.f);
    fx.spawnShockwave(lab::swordStatue() + glm::vec3{0.f, 0.45f, 0.f}, 6.f, gunkit::SWORD_GLOW);
    shake(0.35f, 0.03f); hitStopFrames = std::max(hitStopFrames, 4);
    drawSword(); saveStoryProgress();   // (kept through a death and a continue)
    pushBanner("OATHBREAKER", "5 - EQUIP", {1.f, 0.35f, 0.5f}, 4.f);
}

// The sword in the block (until you pull it), its seal of red bands, and the light that calls you once it breaks
inline void GameplayState::gatherSwordStatue(std::vector<BoxInstance>& out) {
    using namespace rig; using glm::vec3;
    const vec3 B = lab::swordStatue(), G = lab::swordStatueGrip();
    auto S3 = [](float x, float y = -1.f, float z = -1.f) { if (y < 0.f) y = z = x; return rig::S(glm::vec3{x, y, z}); };
    if (glm::length(player.position - B) > 160.f) return;
    const float f = swordStatueFlash;
    if (!storySword) {
        const vec3 glow = gunkit::SWORD_GLOW * (swordStatueOpen ? 1.6f + 0.6f * std::sin(gameClock * 4.f) : 0.7f) + vec3{f * 3.f};
        const float top = B.y + 1.3f;   // (the block's top)
        push(out, T(vec3{G.x, (top + G.y - 0.2f) * 0.5f, G.z}) * S3(0.1f, G.y - 0.2f - top, 0.035f), vec3{0.32f, 0.33f, 0.36f}, glow * 0.25f);   // the blade, out of the block
        push(out, T(vec3{G.x, (top + G.y - 0.2f) * 0.5f, G.z + 0.02f}) * S3(0.018f, G.y - 0.2f - top, 0.01f), vec3{0.f}, glow);         // its fuller, lit
        push(out, T(G + vec3{0.f, -0.18f, 0.f}) * S3(0.55f, 0.07f, 0.1f), vec3{0.08f, 0.08f, 0.1f}, glow * 0.4f);                        // the guard
        push(out, T(G + vec3{0.f, 0.f, 0.f}) * S3(0.065f, 0.32f, 0.065f), vec3{0.12f, 0.05f, 0.06f}, vec3{0.f});                           // the grip
        push(out, T(G + vec3{0.f, 0.2f, 0.f}) * RY(0.785f) * S3(0.12f, 0.11f, 0.12f), vec3{0.1f}, glow * 0.8f);                           // the pommel
        if (!swordStatueOpen) {   // the seal: bands of red turning round the blade, and chains to the obelisks
            for (int r = 0; r < 3; ++r) {
                const float y = top + 0.2f + r * 0.28f, rad = 0.32f - r * 0.04f, spin = gameClock * (r % 2 ? -0.9f : 0.7f);
                for (int k = 0; k < 10; ++k) {
                    if ((k + r) % 4 == 0) continue;
                    float a = spin + k * 0.628f;
                    push(out, T(vec3{G.x + std::cos(a) * rad, y, G.z + std::sin(a) * rad}) * RY(-a) * S3(0.03f, 0.05f, 0.17f), vec3{0.1f, 0.f, 0.f}, vec3{1.8f, 0.08f, 0.2f});
                }
            }
            for (float sx : {-1.f, 1.f}) {
                const vec3 a = G + vec3{0.f, -0.3f, 0.f}, b = B + vec3{sx * 1.36f, 2.5f, 1.6f};   // (to its hands: the giant holds it down)
                const int links = 9;
                for (int k = 0; k < links; ++k) {
                    float u = (k + 0.5f) / links;
                    vec3 p = glm::mix(a, b, u) - vec3{0.f, 0.35f * std::sin(u * 3.14159f), 0.f};
                    vec3 d = b - a;
                    push(out, T(p) * RY(std::atan2(d.x, d.z) + (k % 2) * 1.5708f) * S3(0.05f, 0.12f, 0.22f), vec3{0.12f, 0.03f, 0.04f}, vec3{0.9f, 0.04f, 0.1f});
                }
            }
        } else {   // unsealed: a shaft of light up out of it, seen across the floor
            const float pulse = 0.7f + 0.3f * std::sin(gameClock * 3.f);
            push(out, T(vec3{G.x, G.y + 15.f, G.z}) * S3(0.08f, 30.f, 0.08f), vec3{0.f}, gunkit::SWORD_GLOW * 2.2f * pulse);
            push(out, T(vec3{G.x, G.y + 15.f, G.z}) * RY(gameClock * 0.5f) * S3(0.35f, 30.f, 0.02f), vec3{0.f}, gunkit::SWORD_GLOW * 0.5f * pulse);
            for (int k = 0; k < 8; ++k) {   // motes rising up it
                float u = std::fmod(gameClock * 0.35f + k * 0.125f, 1.f);
                push(out, T(G + vec3{std::cos(k * 2.4f) * 0.3f, u * 6.f, std::sin(k * 2.4f) * 0.3f}) * S3(0.05f), vec3{0.f}, gunkit::SWORD_GLOW * 2.f * (1.f - u));
            }
        }
    } else {   // its slot in the block, still glowing faintly
        push(out, T(vec3{G.x, B.y + 1.31f, G.z}) * S3(0.14f, 0.02f, 0.06f), vec3{0.f}, gunkit::SWORD_GLOW * 0.8f);
    }
    if (f > 0.01f) push(out, T(G) * S3(0.6f + 2.f * (1.f - f)), vec3{0.f}, vec3{2.5f, 1.4f, 1.8f} * f * f);
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
        if (landed) { viewModel.swordImpact(false); audio.play("sword_hit", 120); hitStopFrames = std::max(hitStopFrames, 3); shake(0.1f, 0.02f); }
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
            viewModel.swordImpact(true);
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
                viewModel.swordImpact(true);   // the blow jars the blade in your hands
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
