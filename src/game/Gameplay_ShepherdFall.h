#pragma once
#include "game/Gameplay_ShepherdHead.h"
// =============================================================================
// Gameplay_ShepherdFall.h — THE SHEPHERD, phase 5: THE FALL, the finisher, its
// death and the end (design section 6). On its crown as it falls into the giant
// (LevelShepherd.h, arena 10; the end is arena 11).
//
//   its lamps     six great lamps along its ridge. One at a time each fires the
//                 aimed sweep at you (the red cone on the hull, 3.5 s): leave the
//                 cone, or get behind a fin by its lit post. After it fires, that
//                 lamp stands open (white) for 5 s: shoot it out. The sky goes
//                 from black to re-entry fire as they go; gravity rises.
//   its fleece    sheets of capsules peel off its ridge and come down burning
//                 (gold rings), and strays climb the hull after each shot.
//   the last count  all six out: its core lamp opens and the corner number
//                 starts one last time. Drive HOLDFAST into the core before 0
//                 and you pin it shut while it fires: the number reaches 0 and
//                 nothing comes out. The sweep goes off inside it. Miss, and it
//                 fires everything.
//   the end       you are thrown clear onto a drifting capsule: yours, UNIT 24's,
//                 never sent. Everything it kept drifts loose. COUNT: -
// =============================================================================

namespace shepfall {
constexpr float LAMP_HP = 230.f, LAMP_OPEN = 5.f, WARN = 3.5f, FLASH = 1.f;
constexpr float LAST_COUNT = 24.f, FIN_DRAIN = shep::FIN_DRAIN, FIN_END = 9.f, END_LEN = 18.f;
}

inline void GameplayState::shepFallReset() {
    using namespace shepfall;
    if (!shepLampsArmed) { shepLampsArmed = true; for (float& h : shepLampHp) h = LAMP_HP; }
    for (float& o : shepLampOpen) o = 0.f;
    shepFallSweepT = 5.f; shepFallPassed = false; shepFireLamp = -1; shepLampNext = 0;
    shepLastT = -1.f; shepFinT = -1.f; shepEndingT = -1.f; shepFinCount = 0.f;
    shepHeadReset();
    if (director.arena == 10 && shepHeadHp <= 0.f) shepLastT = 0.f;   // (a retry on the last count: it counts again)
    if (director.arena == 11) shepEndingT = 0.f;
}

inline int GameplayState::shepLampsAlive() const { int n = 0; for (float h : shepLampHp) n += h > 0.f; return n; }

inline void GameplayState::shepFall(float dt) {
    using namespace shepfall;
    const glm::vec3 p = player.position;
    const int at = level.arenaAt(p);
    // ---- the end: drifting ----
    if (shepEndingT >= 0.f) {
        const float before = shepEndingT;
        shepEndingT += dt;
        shepFallAmt = 1.f;
        player.gravity = -24.f * ring::GRAVITY_SCALE * 0.5f;
        if (before < 1.5f && shepEndingT >= 1.5f) pushBanner("UNIT 24", "YOUR CAPSULE. NEVER SENT", glm::vec3{2.4f, 1.3f, 0.35f} * 0.5f, 5.f);
        if (before < 9.f && shepEndingT >= 9.f) { snprintf(ringCountLine, sizeof(ringCountLine), "COUNT: -"); ringCountT = 7.f; }   // the count ends (13.8)
        if (before < END_LEN && shepEndingT >= END_LEN && storyEndT < 0.f) {
            storyEndT = 0.f; storyBossDown = true; saveStoryProgress(); logRun("end"); audio.play("wave", 90, SoundGroup::UI, true);
        }
        return;
    }
    // ---- the finisher: pinned shut while it fires ----
    if (shepFinT >= 0.f) {
        const float before = shepFinT;
        shepFinT += dt;
        player.velocity = glm::vec3{0.f}; invincFrames = std::max(invincFrames, 0.3f);
        const glm::vec3 c = shep::fallCore();
        if (holdfast.held()) fx.spawnTracer(player.camera.position - glm::vec3{0.f, 0.4f, 0.f}, c, 0.06f, dt * 1.5f, HOLDFAST_STEEL * 2.f);
        if (shepFinT < FIN_DRAIN) shake(0.1f, 0.01f + 0.03f * shepFinT / FIN_DRAIN);
        if (before < FIN_DRAIN && shepFinT >= FIN_DRAIN) {   // 0: nothing comes out. It goes off inside
            holdfast.cut();
            // updateMusic runs before the simulation. Release here as well so
            // the callback cannot play another frame of combat after detonation.
            audio.setMusicVolume(0.f);audio.music.setDrive(false);audio.setOverdrive(false,0);
            shake(2.5f, 0.12f);
            for (int k = 0; k < 8; ++k) {
                fx.spawnBurst(c + glm::vec3{cosmetic::range(-12.f, 12.f), cosmetic::range(-4.f, 14.f), cosmetic::range(-6.f, 2.f)}, POST_LIT, 50, 20.f, 1.4f, 2.f);
                fx.spawnShockwave(c + glm::vec3{0.f, 2.f * k, 0.f}, 30.f + 8.f * k, POST_LIT * 0.8f);
            }
            shepherdSound("shep_implode",c);
            for (auto& e : enemies) if (e.targetable() && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
            styleSystem.addStyle(500.f, StyleSource::ENVIRONMENT);
            shepherdDamageT = 2.f;
        }
        if (shepFinT > FIN_DRAIN && shepFinT < FIN_END) {
            shake(0.15f, 0.03f);
            if (std::fmod(gameClock, 0.1f) < dt) fx.spawnBurst(c + glm::vec3{cosmetic::range(-20.f, 20.f), cosmetic::range(0.f, 25.f), cosmetic::range(-10.f, 0.f)}, glm::vec3{2.4f, 1.1f, 0.4f}, 12, 14.f, 1.2f, 1.f);
        }
        if (before < FIN_DRAIN + 1.f && shepFinT >= FIN_DRAIN + 1.f) pushBanner("I NEVER KNEW WHERE THEY WERE GOING.", "ONLY THAT THEY STAYED.", glm::vec3{0.9f, 0.9f, 0.82f}, 6.f);
        if (before < FIN_END && shepFinT >= FIN_END) {   // thrown clear as it burns down into the clouds
            shepFinT = -1.f;
            storySeenSection = storyCheckpoint = 11;
            enterArena(11); saveStoryProgress();
        }
        return;
    }
    if (at != 10 || playerDead || storyEndT >= 0.f) return;
    // ---- THE FALL ----
    shepFallT += dt;
    shepFallAmt = glm::clamp(0.15f + shepFallT / 160.f + 0.6f * (1.f - shepHeadHp / shephead::HP), 0.f, 1.f);
    player.gravity = -24.f * (1.f + 0.25f * shepFallAmt);   // full, and rising
    if (!shepFallSeen) {
        shepFallSeen = true;
        pushBanner("THE FALL", "THE LAST ORBIT. INTO THE GIANT", glm::vec3{2.4f, 1.1f, 0.4f} * 0.5f, 4.f);
    }
    if (std::fmod(gameClock, 0.05f) < dt)   // re-entry: fire pouring over its ridge, more as it falls
        for (int k = 0; k < 1 + (int)(4.f * shepFallAmt); ++k)
            fx.spawnSpray({cosmetic::range(shep::FALL_X0, shep::FALL_X1), shep::FALL_Y + 24.f, shep::FALL_ZN - 4.f}, glm::normalize(glm::vec3{cosmetic::range(-0.2f, 0.2f), 0.2f, 1.f}), glm::vec3{2.4f, 1.0f, 0.3f} * (0.5f + shepFallAmt), 2, 18.f, 1.4f, 0.8f);
    if (g_devOverlay == "fallkill") {   // dev: a round into each lamp as it opens; the spike into the core on the last count
        if (shepOpenT > 0.f && shepHeadHp > 0.f) shepHeadHurt(900.f * dt);
        if (shepLastT > 2.f && !holdfast.held()) holdfast.latch(shep::fallCore(), player.camera.position, level.shepCoreWall, -1);
    }
    // The last count
    if (shepLastT >= 0.f) {
        const float before = shepLastT;
        shepLastT += dt;
        if (before == 0.f) {
            sweep.reset();
            pushBanner("ITS CORE IS OPEN", "THE LAST COUNT", SWEEP_RED, 3.f);
            ui.toast("THE LAST COUNT", "DRIVE HOLDFAST (" + binds().label(Act::HOLDFAST) + ") INTO ITS CORE BEFORE 0. PIN IT SHUT", HOLDFAST_STEEL, 6.f);
            audio.play("telegraph", 128, SoundGroup::WORLD, true);
        }
        const float left = LAST_COUNT - shepLastT;
        shepLastCount = std::max(0.f, left);
        if (std::ceil(left) != std::ceil(left + dt) && left > 0.f) audio.play("count_tick", left < 3.5f ? 128 : 100, SoundGroup::UI, true);
        if (holdfast.held() && holdfast.body == level.shepCoreWall) {   // pinned
            shepFinT = 0.f; shepFinCount = shepLastCount; shepLastT = -1.f;
            stopShepherdAudio();shepherdSound("shep_seal",shep::fallCore());
            return;
        }
        if (left <= 0.f) {   // it fires everything
            shepLastT = 0.f;
            invincFrames = 0.f;
            if (damagePlayer(1e6f, shep::fallCore(), 0.6f, 0.1f, 0.f, {EnemyType::COUNT, DamageKind::SWEEP})) {
                holdfast.cut();
                player.velocity = glm::vec3{0.f, 16.f, 46.f}; player.launched();
                sweptT = 0.f; sweptSkip = 0.f; sweptFrom = player.camera.position; sweptDir = glm::vec3{0.f, 0.f, 1.f};
                audio.duck(60.f, SWEPT_FALL + 0.3f);
            }
        }
        return;
    }
    shepHead(dt);   // THE SHEPHERD itself (Gameplay_ShepherdHead.h)
    shepCapsTick(dt);
}

// A round into one of its lamps (only while it stands open)
inline bool GameplayState::shootFall(glm::vec3 from, glm::vec3 dir, float& distance, float dmg) {
    if (level.arenaAt(player.position) != 10 || shepLastT >= 0.f || shepFinT >= 0.f) return false;
    return shootHead(from, dir, distance, dmg);
}

// Its lamps and core, live; capsules drifting loose at the end
inline void GameplayState::gatherFall(std::vector<BoxInstance>& out) {
    using namespace rig;
    using namespace shepfall;
    const int at = level.arenaAt(player.position);
    if(at==10||shepFinT>=0.f)gatherShepherdCourt(out);
    gatherHead(out);   // THE SHEPHERD itself
    if (at == 10 || shepFinT >= 0.f) {
        const glm::vec3 core = shep::fallCore();
        const bool open = shepLastT >= 0.f || shepFinT >= 0.f;
        const float pulse = 0.7f + 0.3f * std::sin(gameClock * (open ? 10.f : 2.f));
        const glm::vec3 coreGlow = shepFinT >= FIN_DRAIN ? POST_LIT * 4.f : open ? POST_LIT * 3.f * pulse : glm::vec3{2.4f, 1.1f, 0.4f} * 0.8f * pulse;
        push(out, T(core + glm::vec3{0.f, 0.f, 1.2f}) * S(glm::vec3{9.f, 9.f, 0.6f}), glm::vec3{0.1f}, coreGlow);
        if (!open) for (int k = 0; k < 4; ++k)   // its shutters, closed until its lamps are out
            push(out, T(core + glm::vec3{0.f, -3.4f + 2.3f * k, 1.6f}) * S(glm::vec3{10.f, 1.6f, 0.4f}), glm::vec3{0.22f, 0.21f, 0.2f});
    }
    if (shepEndingT >= 0.f) {   // everything it kept, drifting loose
        const glm::vec3 d = shep::driftStart();
        for (int k = 0; k < 40; ++k) {
            const float fk = (float)k, out2 = 30.f + 160.f * ring::hash(fk, 91.f) + shepEndingT * (1.f + 2.f * ring::hash(fk, 92.f));
            const float a = 6.2831853f * ring::hash(fk, 93.f);
            const glm::vec3 c = d + glm::vec3{std::cos(a) * out2, (ring::hash(fk, 94.f) - 0.5f) * 80.f, std::sin(a) * out2};
            push(out, T(c) * RY(fk + shepEndingT * 0.05f) * RX(fk * 0.7f + shepEndingT * 0.03f) * S(glm::vec3{2.4f, 2.2f, 4.8f}), glm::vec3{0.6f, 0.62f, 0.62f});
            push(out, T(c + glm::vec3{0.f, 1.2f, 0.f}) * S(glm::vec3{0.5f, 0.15f, 0.5f}), glm::vec3{0.1f}, glm::vec3{2.4f, 1.3f, 0.35f} * (ring::hash(fk, 95.f) < 0.5f ? 0.8f : 0.1f));
        }
    }
}

inline void GameplayState::fallMarkers(const glm::mat4& view, const glm::mat4& proj) {
    if (level.arenaAt(player.position) != 10 || shepFinT >= 0.f) return;
    char buf[64]; float sx, sy;
    if (shepLastT >= 0.f) {
        const glm::vec3 at = shep::fallCore() + glm::vec3{0.f, 5.f, 0.f};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        const int d = (int)glm::length(at - player.position);
        snprintf(buf, sizeof(buf), d <= (int)Holdfast::RANGE ? "HOLDFAST IT NOW  %dM" : "ITS CORE  %dM: GET IN RANGE", d);
        ui.renderMarker(sx, sy, on, HOLDFAST_STEEL, buf);
        return;
    }
    headMarkers(view, proj);
}

inline const char* GameplayState::fallObjective() {
    static char line[110];
    if (shepEndingT >= 0.f) return "";
    if (shepFinT >= 0.f) return "HOLD IT SHUT";
    if (shepLastT >= 0.f) { snprintf(line, sizeof(line), "THE LAST COUNT  /  HOLDFAST (%s) INTO ITS CORE", binds().label(Act::HOLDFAST).c_str()); return line; }
    return headObjective();
}
