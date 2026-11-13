#pragma once
// =============================================================================
// Gameplay_Ring.h — GameplayState: Chapter 6, THE KEPT RING (LevelRing.h,
// docs/chapter-6-design.md), and THE SWEEP wherever it runs.
//
// The sweep's rules are world/RingSweep.h. Here: when one starts, what it does
// to you and to enemies, its posts, its front and the corner countdown.
// `--story6` is the test ground; `--sweep [far|mid|near|gather]` runs sweeps
// in any level, with every tall wall as shelter (for trying it with
// --training or another chapter).
// =============================================================================
#include "world/LevelRing.h"

inline const glm::vec3 SWEEP_RED{1.f, 0.1f, 0.06f};          // the countdown: red, never flashing
inline const glm::vec3 POST_LIT{2.4f, 2.6f, 2.1f};           // a mooring post, lit: pale and hard, unlike a capsule's amber
inline const glm::vec3 SWEEP_ICE{1.5f, 1.42f, 1.25f};        // the front: lifted ice in the sun

inline bool GameplayState::sweepOn() const { return ring() || g_devSweep > 0; }

// A fresh start (a new run, a retry): shelters whole, the first sweep soon
inline void GameplayState::armSweep() {
    sweep.reset(); sweepArmed = true; sweepCount = 0; sweepTickAt = -1; sweepSwept = 0;
    ringGone = ringShort = 0; ringCountT = -1.f; sweptBodies.clear();
    resetGap();   // (the raft back at its mooring: armSweep resets every DRIVEN mover)
    resetWake(); resetShore(); lurkerOut.assign(ring::lurkers().size(), 0);
    sweepShelters.clear();
    if (ring()) {
        sweepShelters = level.sweepShelters;
        for (auto& m : level.movers) if (m.path == Mover::Path::DRIVEN) { m.drive = 0.f; m.gone = false; }   // loose shelters back where they were, whole
    } else   // dev, anywhere: anything tall enough to stand behind
        for (const auto& w : level.walls) {
            const glm::vec3 size = w.box.max - w.box.min;
            if (w.dynamic || size.y < 2.4f || std::max(size.x, size.z) > 40.f) continue;
            SweepShelter s; s.box = w.box; s.post = {(w.box.min.x + w.box.max.x) * 0.5f, w.box.min.y, w.box.max.z + 1.5f};
            sweepShelters.push_back(s);
        }
    if (ring() && !g_devRingTest && g_devSweep == 0) {   // the crossing's far tier: one clean wave, a long gap
        // (the far tier's warning, but the crossing's own gaps: about two sweeps in each place, not one)
        sweepSched.arm(55.f, 85.f, 0x52494E47u + (uint32_t)deaths * 7919u);
        // (the random clock runs from the start, so a retry from any checkpoint still gets sweeps;
        //  each place's first is scripted on top of it: updateRing)
    } else {   // the test ground and dev runs come round quickly
        sweepSched.arm(25.f, 40.f, 0x52494E47u + (uint32_t)deaths * 7919u);
        sweepSched.timer = 8.f;
    }
}

// The next sweep. The crossing: far (20 s) for now (the Landing to the Kept are all far, section 4).
// The test ground walks through the tiers, so one visit shows all three
inline SweepSpec GameplayState::nextSweep() const {
    const glm::vec3 dir = ring() ? ring::sweepDir() : glm::vec3{0.f, 0.f, 1.f};
    const int tier = g_devSweep > 0 ? g_devSweep : (ring() && !g_devRingTest) ? (level.arenaAt(player.position) >= 9 ? 3 : level.arenaAt(player.position) >= 5 ? 2 : 1) : 1 + sweepCount % 3;   // (the Shadow Band: the mid tier)
    switch (tier) {
        case 2:  return sweeptier::mid(dir).spec;
        case 3:  return sweeptier::near(dir).spec;
        case 4:  return sweeptier::gather(-dir);
        default: return sweeptier::far(dir).spec;
    }
}

inline void GameplayState::tickSweep(float dt) {
    if (!sweepOn()) return;
    if (!sweepArmed) armSweep();
    for (SweepShelter& q : sweepShelters) {   // drifting and shoved shelters: their box (and post) go where their wall went
        if (q.wall < 0 || q.wall >= (int)level.walls.size()) continue;
        const glm::vec3 d = level.walls[q.wall].box.min - q.box.min;
        q.box = level.walls[q.wall].box; q.post += d;
    }
    const bool blocked = playerDead || victory || storyLocked() || storyEndT >= 0.f || (ring() && ringQuiet());
    float rate = 1.f;   // a TALLY standing: they come twice as often
    for (const auto& e : enemies) if (e.alive && e.type == EnemyType::TALLY) { rate = 2.f; break; }
    if (ringScriptT >= 0.f && !sweep.active() && !blocked && (ringScriptT -= dt) <= 0.f) {   // a level's first: scripted, shelter in sight
        ringScriptT = -1.f; sweep.start(nextSweep()); ++sweepCount;
        sweepSched.timer = 45.f + 15.f * (float)(sweepCount % 3) * 0.5f;   // (the next comes round in under a minute)
    } else if (sweepSched.tick(dt, sweep.active(), blocked, rate)) { sweep.start(nextSweep()); ++sweepCount; }
    if (g_devOverlay == "sweepnow" && !sweep.active() && elapsedTime > 0.5f && sweepCount == 0) {   // dev: one mid-count, for screenshots
        SweepSpec s = nextSweep(); s.flashLead = 0.f; s.warning = 6.f; sweep.start(s); ++sweepCount;
    }
    if (ringCountT >= 0.f && (ringCountT -= dt) < 0.f) ringCountT = -1.f;
    if (ringSweptAgo >= 0.f && (ringSweptAgo += dt) > 12.f) ringSweptAgo = -1.f;
    for (auto& b : sweptBodies) b.t += dt;
    sweptBodies.erase(std::remove_if(sweptBodies.begin(), sweptBodies.end(), [](const SweptBody& b) { return b.t > 5.f; }), sweptBodies.end());
    if (!sweep.active()) return;
    const glm::vec3 dir = sweep.spec.dir;
    auto ev = sweep.update(dt, player.position, sweepShelters);
    if (ev.flash) {   // the ridge lamps: every post lights
        audio.play("telegraph", 128, SoundGroup::WORLD, true);
        shake(0.25f, 0.012f);
        sweepSwept = 0;
        if (ring() && !ringLeeHint) { ringLeeHint = true; ui.toast("THE SWEEP", "THE POSTS ARE LIT: GET ON THE FAR SIDE OF SOMETHING HEAVY, BY ITS POST", POST_LIT * 0.4f, 6.f); }
    }
    if (ev.begun) { sweepTickAt = -1; audio.play("barrier", 90, SoundGroup::WORLD, true); }
    if (const float c = sweep.countdown(player.position); c >= 0.f) {   // a dull tick through the floor each second
        const int whole = (int)std::ceil(c);
        if (whole != sweepTickAt) { sweepTickAt = whole; audio.play("count_tick", c < 3.5f ? 128 : 90, SoundGroup::UI, true); }
        if (std::fmod(gameClock, 0.12f) < dt)   // dust lifting off the ground ahead of it
            fx.spawnBurst(player.position - dir * cosmetic::range(4.f, 18.f) + glm::vec3{cosmetic::range(-8.f, 8.f), 0.1f, 0.f}, SWEEP_ICE * 0.5f, 1, 0.6f, 0.9f, -0.6f);
        if (sweep.spec.kind != SweepSpec::Kind::CONE && !ringQuiet()) {   // ice streaming off its crest, near you
            const glm::vec3 side{-dir.z, 0.f, dir.x};
            for (int w = 0; w < sweep.waves(); ++w) {
                const glm::vec3 crest = sweep.base + dir * sweep.front(w);
                const float ahead = glm::dot(player.position - crest, dir);
                if (ahead < 0.f || ahead > 160.f) continue;
                for (int q = 0; q < 3; ++q) {
                    const glm::vec3 at = crest + side * (glm::dot(player.position - sweep.base, side) + cosmetic::range(-45.f, 45.f)) + glm::vec3{0.f, cosmetic::range(8.f, 18.f), 0.f};
                    fx.spawnSpray(at, glm::normalize(dir * 0.7f + glm::vec3{0.f, 0.7f, 0.f}), SWEEP_ICE * 0.9f * (1.f - 0.8f * ringDark()), 2, 16.f, 1.4f, 0.5f);
                }
            }
        }
    }
    for (int i : ev.shattered) {
        const AABB& b = sweepShelters[i].box; const glm::vec3 c = (b.min + b.max) * 0.5f;
        fx.spawnBurst(c, SWEEP_ICE, 40, 12.f, 0.9f, 6.f);
        audio.playAt("ab_shatter", c, 128, SoundGroup::WORLD, true, 0.8f);
        // ...and it is gone: no box to stand behind, on or in. (Only movers can be small: a fixed
        // wall is baked into the world mesh and the collision grid, so a breakable one is a DRIVEN mover.)
        if (const int m = sweepShelters[i].mover; m >= 0 && m < (int)level.movers.size()) level.movers[m].gone = true;
    }
    // Enemies in the open go with it
    for (auto& e : enemies) {
        if (!e.targetable() || isBoss(e.type) || isSheepdog(e.type) || !sweep.sweptNow(e.position, sweepShelters)) continue;   // (a sheepdog stands in it)
        e.knockVel = dir * 34.f + glm::vec3{0.f, 9.f, 0.f};
        fx.spawnBurst(e.position + glm::vec3{0.f, 1.f, 0.f}, SWEEP_ICE, 14, 10.f, 0.6f, 2.f);
        if (!sweep.lethal()) continue;
        if (e.takeDamage(1e6f)) {
            onEnemyKilled(e, StyleSource::ENVIRONMENT); ++sweepSwept;
            if (sweptBodies.size() < 24)
                sweptBodies.push_back({e.position + glm::vec3{0.f, e.height() * 0.5f, 0.f}, dir, glm::vec3{e.radius() * 1.6f, e.height(), e.radius() * 1.2f}, e.stats().color, 0.f, (float)(e.uid % 7) - 3.f});
        }
    }
    if (ev.struck) {
        const bool heldFast = holdfast.held() && holdfast.shelter >= 0 && sweepShelters[holdfast.shelter].alive && sweepShelters[holdfast.shelter].holds;
        fx.spawnBurst(player.camera.position, SWEEP_ICE, 50, 16.f, 0.7f, 2.f);
        audio.play("slam", 128, SoundGroup::WORLD, true);
        shake(0.5f, 0.06f);
        if (heldFast) ui.feed("HELD FAST", HOLDFAST_STEEL);
        else if (!sweep.lethal()) {   // the Gather: thrown toward the Shepherd (the ride itself is build step 10)
            holdfast.cut(); grapple.release();
            player.velocity = dir * 42.f + glm::vec3{0.f, 13.f, 0.f}; player.launched();
            ui.feed("GATHERED", SWEEP_ICE);
        } else {
            invincFrames = 0.f;   // (a dash does not get you through this)
            if (damagePlayer(1e6f, player.camera.position - dir * 10.f, 0.6f, 0.08f, 0.f, {EnemyType::COUNT, DamageKind::SWEEP})) {
                holdfast.cut();
                player.velocity = dir * 46.f + glm::vec3{0.f, 16.f, 0.f}; player.launched();   // off the ring
                sweptT = 0.f; sweptSkip = 0.f; sweptFrom = player.camera.position; sweptDir = dir;   // (13.6: updateSweptDeath)
                audio.duck(60.f, SWEPT_FALL + 0.3f);   // total silence
            }
        }
    }
    if (ev.cleared && !playerDead) {   // it went over: ice pattering down on the shelter
        fx.spawnBurst(player.position + glm::vec3{0.f, 6.f, 0.f} - dir * 3.f, SWEEP_ICE, 60, 7.f, 1.6f, 3.f);
        audio.duck(30.f, 0.4f);   // the zero (13.1): everything drops away for a beat as it goes over...
        SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.delay = 0.4f;
        audio.playOpts("skim", o);   // ...then ice pattering down on the shelter
        shake(0.3f, 0.02f);
    }
    if (ev.ended && !playerDead) {
        char line[64];
        if (sweepSwept > 0) { snprintf(line, sizeof(line), "SWEPT x%d", sweepSwept); ui.feed(line, SWEEP_ICE); }
        if (sweep.lethal()) ringSweptAgo = 0.f;   // (the swept, burning up across the giant: skybox.frag)
        if (sweep.lethal()) {   // the Shepherd counts (13.2): where the countdown was, for a few seconds
            ring::countLine(ringCountLine, sizeof(ringCountLine), ring::DISPATCHED + 1 - ringGone, ringShort);
            ringCountT = 6.f; ringShort = 0;
        }
    }
}

// ---- Chapter 6 ----
inline float GameplayState::ringShepherdSize() const { return g_devRingTest ? 0.004f : ring::shepherdSize(player.position.z); }
inline float GameplayState::ringFlash() const {   // the ridge lamps: full on the flash, then held while the wave comes
    if (!sweep.active()) return 0.f;
    if (sweep.phase == RingSweep::Phase::FLASH) return 1.f;
    return sweep.phase == RingSweep::Phase::INBOUND ? 0.55f + 0.1f * std::sin(gameClock * 2.f) : 0.2f;
}
inline float GameplayState::ringNight() const { return ringDark(); }   // (the Shadow Band: Gameplay_Shadow.h)
inline bool GameplayState::ringQuiet() const {   // inside the open hull: air, and no sweeps
    const AABB H = ring::quietHull(); const glm::vec3 p = player.position;
    return !g_devRingTest && p.x > H.min.x && p.x < H.max.x && p.z > H.min.z && p.z < H.max.z + 1.f && p.y < H.max.y;
}

// Chapter 6's painted-once textures (render/RingDetail.h): the ground's and rock's patterns, the giant's
// cloud tops, the galaxy. ~0.3 s (0.6 s in a browser) the first time: done at the level's start, kept after
inline void GameplayState::ringTextures() {
    static GLuint detail[2] = {}, sky[3] = {};   // (shared by every run, like the world's: a retry doesn't paint them again)
    if (!detail[0]) {
        detail[0] = TextureGen::uploadTexture(ringdetail::ground(), ringdetail::SIZE, ringdetail::SIZE);
        detail[1] = TextureGen::uploadTexture(ringdetail::rock(), ringdetail::SIZE, ringdetail::SIZE);
        for (GLuint t : detail) TextureGen::setAnisotropy(t, GameSettings::qualityAnisotropy(settings ? settings->quality : GameSettings::QUALITY_DEFAULT));
    }
    if (!sky[0]) {
        sky[0] = TextureGen::uploadTexture(ringdetail::giant(), ringdetail::GIANT_W, ringdetail::GIANT_H);
        sky[1] = TextureGen::uploadTexture(ringdetail::galaxy(), ringdetail::GALAXY_W, ringdetail::GALAXY_H);
        sky[2] = TextureGen::uploadTexture(ringdetail::nebula(), ringdetail::NEBULA_SIZE, ringdetail::NEBULA_SIZE);
        for (GLuint t : sky) { glBindTexture(GL_TEXTURE_2D, t); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); }
    }
    ringDetailTex[0] = detail[0]; ringDetailTex[1] = detail[1]; ringSkyTex[0] = sky[0]; ringSkyTex[1] = sky[1]; ringSkyTex[2] = sky[2];
}

inline void GameplayState::beginRing() {
    ringTextures();
    storyOwned |= 15; storyArmed = storyGrapple = true;
    wakeT = -1; clawT = -1; beltBodies.clear(); player.eyeHeight = 1.7f;
    player.camera.farPlane = ring::FAR_PLANE;
    if (!wakeSeen && director.arena == 0) pushBanner(level.story.title, "CHAPTER 6", SWEEP_ICE * 0.6f, 4.f);
    wakeSeen = true;
    if (director.arena >= 2) holdfastFound = true;   // (a checkpoint past I2: it is yours)
    holdfast.reset(); armSweep();
    ringLevelSeen = -1; ringScriptT = -1.f; ringAir = -1;
    ringBolt = 0.f; ringBoltT = -1.f; ringBoltNext = 3.f; ringSunT = ringSunHit = -1.f; ringShadeHint = false; spokeHint = false; resetGap(); resetWake(); ringKnockOn = -1; ringKnockT = ringCreakT = 0.f; crookDragT = 0.f; crookDragUid = -1; crookSeen = crookLeft = false;
    if (g_devRingTest) ringLevelSeen = 99;
    player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0}; prevCamPos = player.camera.position;
}

inline void GameplayState::updateRing(float dt) {
    player.camera.farPlane = ring::FAR_PLANE;                 // (a retry makes a fresh camera and player)
    player.gravity = -24.f * (g_devRingTest ? ring::GRAVITY_SCALE : ring::gravityAt(player.position.z));   // the crossing's low gravity (full again on the Shore); fast-fall and the slam are untouched
    if (g_devCam) {   // dev: --cam X Y Z YAW PITCH holds the eye there (screenshots)
        player.position = g_devCamPos - glm::vec3{0.f, player.eyeHeight, 0.f}; player.velocity = glm::vec3{0.f};
        player.camera.yaw = g_devCamYaw; player.camera.pitch = g_devCamPitch;
        player.camera.position = g_devCamPos; prevCamPos = g_devCamPos;
    }
    {   // The sound (design 9, 13.1): vacuum outside, air and a room's echo inside the hull
        const int air = ringQuiet() ? 1 : 0;
        if (air != ringAir) {
            if (air && ringAir == 0) audio.play("barrier", 60, SoundGroup::UI, true);   // the air coming back
            ringAir = air;
            audio.setVacuum(air ? 0.f : 1.f);
            audio.setSpace(air ? ReverbSpace::METAL : ReverbSpace::OPEN);
        }
    }
    if (shepherd()) { updateShepherd(dt); ringFoes(dt); return; }   // its own level (Gameplay_Shepherd.h)
    ringTouch(dt);
    tickShadow(dt);
    tickSpokes(dt);
    tickGap(dt);
    tickWake(dt);
    tickShore(dt);
    const glm::vec3 p = player.position;
    const int at = level.arenaAt(p);
    if (at > director.arena && !director.fighting() && director.phase != WaveDirector::Phase::INTRO) director.approach(at);
    if (at >= 0 && at > storySeenSection && !playerDead) {
        storySeenSection = at;
        // A place's card once (the Drift's two halves, the Kept's two, are one place each)
        const bool fresh = ringLevelSeen < 0 || std::string(level.arenas[at].name) != level.arenas[std::max(0, ringLevelSeen)].name;
        if (fresh && !g_devRingTest) {
            ringLevelSeen = at;
            pushBanner(level.arenas[at].name, level.arenas[at].subtitle, SWEEP_ICE * 0.6f, 3.f);
            audio.play("wave", 64, SoundGroup::UI, true);
            // Each place's first sweep is scripted, with shelter in sight (the Drift's waits for HOLDFAST)
            if (at == 0) ringScriptT = -1.f;   // (the Landing's comes at the ridge, below)
            if (at == 3) ringScriptT = 14.f;
            if (at == 5) ringScriptT = 22.f;   // (the Shadow Band's first: in the dark, among the rocks)
            if (at == 6) ringScriptT = 18.f;   // (the Spokes': you walk to shelter)
            if (at == 7) ringScriptT = -1.f;   // (the Gap's comes mid-crossing: Gameplay_Gap.h)
            if (at == 8) ringScriptT = -1.f;   // (the Wake's comes on the climb: Gameplay_Wake.h)
            if (at == 9) ringScriptT = -1.f;   // (the Shore's come once you're down the slope: Gameplay_Shore.h)
        }
    }
    if (at >= 0 && at > storyCheckpoint && !director.fighting()) { storyCheckpoint = at; sweepSched.checkpoint(); ui.feed("CHECKPOINT", SWEEP_ICE * 0.6f); }
    if (g_devRingTest) {
        if (elapsedTime > 2.f && holdfastHints == 0 && ringHintT < 0.f) {
            ringHintT = 0.f;
            ui.toast("HOLDFAST", "PRESS " + binds().label(Act::HOLDFAST) + ": SPIKE A SURFACE TO HOLD ON, OR A SMALL ENEMY TO PIN IT", HOLDFAST_STEEL, 5.f);
        }
        return;
    }
    // The Landing: low gravity, taught by the ridge; the first sweep comes once you're over it
    if (!ringRidgeHint && p.z < 44.f && p.z > 30.f) {
        ringRidgeHint = true;
        ui.toast("LOW GRAVITY", "JUMP: YOU GO HIGHER AND COME DOWN SLOWER OUT HERE", SWEEP_ICE * 0.6f, 4.f);
    }
    if (sweepCount == 0 && ringScriptT < 0.f && !sweep.active() && p.z < 20.f && at == 0) ringScriptT = 2.f;
    // The Drift: falling between the chunks
    if (!ringDriftHint && at == 1 && p.z < ring::LAND_N - 6.f) {
        ringDriftHint = true;
        ui.toast("THE DRIFT", "THE RING IS LOOSE HERE. JUMP FROM CHUNK TO CHUNK; DON'T FALL THROUGH", SWEEP_ICE * 0.6f, 4.5f);
    }
    // HOLDFAST: in the capsule on I2, the spike its sibling held on with
    if (!holdfastFound && glm::length(p - ring::holdfastCapsule()) < 4.5f) {
        holdfastFound = true;
        pushBanner("HOLDFAST", "A SIBLING HELD ON WITH THIS", HOLDFAST_STEEL, 3.f);
        ui.toast("HOLDFAST", "PRESS " + binds().label(Act::HOLDFAST) + ": SPIKE A SURFACE AND BE HELD TO IT. ON A DRIFTING CHUNK'S FAR SIDE, IT CARRIES YOU THROUGH. SPIKE A SMALL ENEMY TO PIN IT", HOLDFAST_STEEL, 7.f);
        audio.play("drive_ready", 110, SoundGroup::UI, true);
        ringScriptT = 18.f;   // the Drift's first sweep: soon, on the lanes
        saveStoryProgress();
    }
    // The way on
    const AABB& E = level.story.end;
    if (storyEndT < 0.f && shepEnterT < 0.f && !playerDead && p.x > E.min.x && p.x < E.max.x && p.z > E.min.z && p.z < E.max.z) {
        shepEnterT = 0.f; audio.play("wave", 90, SoundGroup::UI, true);   // the Shepherd's foot: on, into its level (Gameplay_Shepherd.h)
    }
    if (shepEnterT >= 0.f) {
        shepEnterT += dt; player.velocity.x *= 0.9f; player.velocity.z *= 0.9f;
        if (shepEnterT > 1.4f) { shepEnterT = -1.f; loadShepherd(0); return; }
    }
    if (storyEndT >= 0.f) { storyEndT += dt; player.velocity = {0.f, 0.f, 0.f}; if (storyEndT > 3.f) captureMouse(false); }
    if (storyTerminal >= 0 && glm::length(level.story.terminals[storyTerminal].pos - p) > 6.f) storyTerminal = -1;
    if (storyLog >= 0) { storyLogT += dt; if (glm::length(level.story.wrecks[storyLog].pos - p) > 6.f) storyLog = -1; }   // (a unit's last log: walk off and it closes)
    ringFoes(dt);
}

// You hear what you touch (13.1): out here nothing else carries. Stand on a capsule and, now and
// then, something inside knocks twice; lean on the reef and it creaks.
inline void GameplayState::ringTouch(float dt) {
    if (playerDead || ringQuiet()) return;
    const int on = player.onGround ? player.groundWall : -1;
    const bool capsule = on >= 0 && std::find(level.knockWalls.begin(), level.knockWalls.end(), on) != level.knockWalls.end();
    if (!capsule) ringKnockOn = -1;
    else {
        if (on != ringKnockOn) { ringKnockOn = on; ringKnockT = cosmetic::range(0.7f, 1.4f); }   // (a moment after you land on it)
        if ((ringKnockT -= dt) <= 0.f) {
            ringKnockT = cosmetic::range(3.f, 6.f);
            const glm::vec3 at = player.position - glm::vec3{0.f, 0.6f, 0.f};
            for (int k = 0; k < 2; ++k) {
                SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.positional = true; o.pos = at; o.volume = 0.5f;
                o.pitch = 0.55f; o.delay = k * 0.22f; o.role = SoundRole::ACTION;
                audio.playOpts("impact_metal", o);
            }
        }
    }
    // Leaning on the reef: pressed against one of its walls, not just passing
    if ((ringCreakT -= dt) > 0.f) return;
    const glm::vec3 p = player.position;
    for (int w : level.creakWalls) {
        const AABB& b = level.walls[w].box;
        const float dx = std::max({b.min.x - p.x, 0.f, p.x - b.max.x}), dz = std::max({b.min.z - p.z, 0.f, p.z - b.max.z});
        if (dx < 0.7f && dz < 0.7f && p.y < b.max.y && p.y + 1.8f > b.min.y && glm::length(glm::vec2{player.velocity.x, player.velocity.z}) < 2.f) {
            ringCreakT = cosmetic::range(4.f, 7.f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.positional = true; o.pos = p + glm::vec3{0.f, 1.f, 0.f}; o.volume = 0.35f; o.pitch = 0.8f;
            audio.playOpts("ab_creak", o);
            return;
        }
    }
    ringCreakT = 0.5f;   // (look again in a moment)
}

// The posts (lit from the flash until it has passed) and the front
inline void GameplayState::gatherSweep(std::vector<BoxInstance>& out) {
    using namespace rig;
    if (!sweepOn()) return;
    const glm::vec3 eye = player.camera.position;
    for (const SweepShelter& s : sweepShelters) {
        if (!s.alive || glm::length(s.post - eye) > 400.f) continue;
        const bool lit = sweep.shelterLit(s);
        push(out, T(s.post + glm::vec3{0.f, 3.1f, 0.f}) * S(glm::vec3{0.34f, 0.34f, 0.34f}), glm::vec3{0.05f}, lit ? POST_LIT : glm::vec3{0.f});
        if (lit) push(out, T(s.post + glm::vec3{0.f, 23.f, 0.f}) * S(glm::vec3{0.07f, 40.f, 0.07f}), glm::vec3{0.f}, POST_LIT * 0.6f);   // a line of light you can find from anywhere
        if (!ring() || s.wall >= 0)   // (a post where the level baked none: dev runs, and on what drifts)
            push(out, T(s.post + glm::vec3{0.f, 1.5f, 0.f}) * S(glm::vec3{0.14f, 3.f, 0.14f}), glm::vec3{0.06f}, glm::vec3{0.f});
    }
    // The swept: tumbling away down the sweep and up, faster and faster, to a glint
    for (const SweptBody& b : sweptBodies) {
        const float d = 24.f * (std::exp(1.2f * b.t) - 1.f);   // ~250 m at 2 s, ~1 km by the end
        const glm::vec3 at = b.from + b.dir * d + glm::vec3{0.f, 4.f * b.t + d * 0.35f, 0.f};
        const float a = b.t * (2.f + b.spin);
        push(out, T(at) * RY(a) * RX(a * 0.7f) * S(b.size), b.color * 0.6f);
        push(out, T(at) * S(glm::vec3{0.3f + d * 0.01f}), glm::vec3{0.f}, SWEEP_ICE * std::min(3.f, 0.5f + b.t));   // (far off, the glint is what you see: it keeps ~0.6 degrees)
    }
    if (sweep.phase != RingSweep::Phase::INBOUND && sweep.phase != RingSweep::Phase::PASSING) return;
    // The front: a wall of lifted ice. Three layers (a low bright crest racing ahead, the wall, a taller
    // dimmer wall of what it has already lifted behind), leaning forward and churning; chunks tumbling over
    // its top; a hard bright edge where it meets the ice; and its shadow lying on the ground ahead of it
    // (13.4: the ground goes dark a moment before the wave). The boss's cone is drawn plain
    const glm::vec3 dir = sweep.spec.dir, side{-dir.z, 0.f, dir.x};
    const bool cone = sweep.spec.kind == SweepSpec::Kind::CONE;
    const float seen = 1.f - 0.88f * ringDark();   // (in the Shadow Band you barely see it)
    const float yawSide = std::atan2(side.z, -side.x);
    const float ground = std::fabs(player.position.y) < 1.5f ? 0.f : player.position.y;   // (on the Wake's treads it runs over the step you're on)
    for (int k = 0; k < sweep.waves(); ++k) {
        const float f = sweep.front(k);
        if (cone && f <= 0.5f) continue;
        for (int i = -40; i <= 40; ++i) {
            const float n = (float)i * 3.7f + (float)k * 17.f;
            const float h = 9.f + 9.f * ring::hash(n, 1.f) + 2.f * std::sin(gameClock * 5.f + n);
            glm::vec3 at;
            if (cone) {
                const float a = sweep.spec.coneHalf * (float)i / 40.f;
                at = sweep.spec.origin + (dir * std::cos(a) + side * std::sin(a)) * f;
            } else {
                const float across = glm::dot(player.position - sweep.base, side) + (float)i * 4.f;
                at = sweep.base + dir * f + side * across;
            }
            at.y = ground;
            const float w = cone ? std::max(0.6f, f * sweep.spec.coneHalf / 40.f * 1.1f) : 4.2f;
            if (cone) {
                push(out, T(at + glm::vec3{0.f, h * 0.5f, 0.f}) * RY(yawSide) * S(glm::vec3{w, h, 0.9f}), SWEEP_ICE * 0.35f * seen, SWEEP_ICE * (0.55f + 0.35f * ring::hash(n, 2.f)) * seen);
                continue;
            }
            // (the wall itself is drawn by renderSweepWall: a curtain, not boxes)
            (void)w; (void)yawSide;
            // a chunk it has lifted, tumbling over the top
            if (i % 3 == 0) {
                const float t = gameClock * (1.5f + ring::hash(n, 3.f)) + n;
                const glm::vec3 c = at - dir * 2.f + glm::vec3{0.f, h + 2.f + 3.f * std::sin(t * 0.7f), 0.f} + side * (1.5f * std::sin(t));
                const float sz = 0.6f + 1.4f * ring::hash(n, 4.f);
                push(out, T(c) * RY(t) * RX(t * 1.3f) * S(glm::vec3{sz, sz * 0.8f, sz * 1.1f}), glm::vec3{0.55f, 0.52f, 0.48f} * seen, SWEEP_ICE * 0.15f * seen);
            }
        }
        if (cone) continue;
        // where it meets the ice: a hard bright edge, then its shadow on the ground ahead of it
        const glm::vec3 mid = sweep.base + dir * f + side * glm::dot(player.position - sweep.base, side);
        push(out, T(mid + dir * 1.6f + glm::vec3{0.f, ground + 0.06f, 0.f}) * RY(yawSide) * S(glm::vec3{340.f, 0.12f, 0.7f}), glm::vec3{0.f}, SWEEP_ICE * 2.2f * seen);
        const float shadowLen = 28.f;
        push(out, T(mid + dir * (2.f + shadowLen * 0.5f) + glm::vec3{0.f, ground + 0.025f, 0.f}) * RY(yawSide) * S(glm::vec3{340.f, 0.02f, shadowLen}), glm::vec3{0.018f, 0.016f, 0.015f}, glm::vec3{0.f});
    }
}

// The wall itself (13.4): a translucent curtain of lifted ice, two layers (the wall and what it has lifted
// behind it), drawn after the sky (it writes no depth: what it passes in front of shows through)
inline void GameplayState::renderSweepWall(const glm::mat4& view, const glm::mat4& proj) {
    if (!sweepOn() || (sweep.phase != RingSweep::Phase::INBOUND && sweep.phase != RingSweep::Phase::PASSING)) return;
    if (sweep.spec.kind == SweepSpec::Kind::CONE || devSkip("sweepwall")) return;
    using glm::vec3;
    const vec3 dir = sweep.spec.dir, side{-dir.z, 0.f, dir.x};
    const float ground = std::fabs(player.position.y) < 1.5f ? 0.f : player.position.y;
    const float seen = 1.f - 0.88f * ringDark();
    sweepWallShader.use();
    sweepWallShader.setMat4("projection", proj); sweepWallShader.setMat4("view", view);
    sweepWallShader.setFloat("uTime", gameClock); sweepWallShader.setFloat("uSeen", seen);
    sweepWallShader.setFloat("uGround", ground); sweepWallShader.setVec3("uSide", side); sweepWallShader.setVec3("uViewPos", player.camera.position);
    glDisable(GL_CULL_FACE); glDepthMask(GL_FALSE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(waterVAO); glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    vec3 v[MAX_WATER * 6]; int quads = 0;
    struct Layer { vec3 origin; float height, layer; } layers[8];
    for (int k = 0; k < sweep.waves() && quads + 2 <= 8; ++k)
        for (int l = 1; l >= 0; --l) {   // (the far layer first: drawn back to front)
            const vec3 mid = sweep.base + dir * (sweep.front(k) - l * 6.f) + side * glm::dot(player.position - sweep.base, side);
            const float H = l ? 46.f : 32.f;
            const vec3 a = mid - side * 220.f + vec3{0.f, ground, 0.f} - vec3{0.f, mid.y, 0.f}, b = a + side * 440.f;
            const vec3 e{0.f, H, 0.f};
            const vec3 q[] = {a, b, b + e, a, b + e, a + e};
            std::copy(q, q + 6, v + quads * 6);
            layers[quads++] = {a, H, (float)l};
        }
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, quads * 6 * sizeof(vec3), v);
    for (int i = 0; i < quads; ++i) {
        sweepWallShader.setVec3("uOrigin", layers[i].origin); sweepWallShader.setFloat("uHeight", layers[i].height); sweepWallShader.setFloat("uLayer", layers[i].layer);
        glDrawArrays(GL_TRIANGLES, i * 6, 6);
    }
    glBindVertexArray(0); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_CULL_FACE);
}

// The swept death (13.6). First person: the ring drops away, tumbling, and in two seconds it is a
// line of light across the giant. Silent. Hold Jump to go straight to the death screen. Runs on the
// frame (the dead don't tick), so it is written as a function of time, not a simulation.
inline void GameplayState::updateSweptDeath(float dt) {
    if (!sweptFalling()) return;
    static Uint8 bound[SDL_NUM_SCANCODES];
    binds().remap(SDL_GetKeyboardState(nullptr), g_devNoMouse ? 0 : SDL_GetMouseState(nullptr, nullptr), bound);
    const Uint8* keys = g_devNoMouse ? g_devKeys : bound;   // (screenshot runs: the real keyboard doesn't reach the game)
    sweptSkip = keys[SDL_SCANCODE_SPACE] ? sweptSkip + dt : 0.f;
    sweptT += dt;
    if (sweptSkip > 0.25f) sweptT = SWEPT_FALL;
    const float t = std::min(sweptT, SWEPT_FALL);
    const float d = 40.f * (std::exp(2.1f * t) - 1.f);   // ~4 km in 2.2 s (inside the far plane): fast, and faster
    player.camera.position = sweptFrom + sweptDir * d * 0.75f + glm::vec3{0.f, d * 0.55f + 6.f * t, 0.f};
    prevCamPos = player.camera.position;
    // Facing back at the ring, turning over: the yaw wheels, the pitch rocks down at it
    const float back = glm::degrees(std::atan2(-sweptDir.z, -sweptDir.x));
    player.camera.yaw = back + 150.f * t + 30.f * std::sin(2.7f * t);
    player.camera.pitch = glm::clamp(-20.f - 40.f * t / SWEPT_FALL + 28.f * std::sin(3.1f * t), -85.f, 85.f);
    if (sweptT >= SWEPT_FALL) deadTimer = 0.f;   // the death screen starts its own clock from here
}

inline void GameplayState::renderSweptDeath() {
    if (!sweptFalling()) return;
    ui.begin2D();
    const std::string skip = "HOLD " + binds().label(Act::JUMP) + " TO SKIP";
    ui.ui.text(skip.c_str(), SCREEN_W / 2, SCREEN_H - 24, 1, {0.75f, 0.72f, 0.65f, 0.6f}, true);
    ui.end2D();
}

// Top left, red, never flashing: the seconds until the wall reaches where you stand
inline void GameplayState::renderSweepCountdown() {
    if (!sweepOn() || playerDead || victory) return;
    const float c = sweep.countdown(player.position);
    if (c < 0.f && ringCountT > 0.f) {   // after it: the count, small, on the same plate, fading out
        const float a = std::min(1.f, ringCountT / 1.5f);
        const float w = 20.f + ui.ui.textWidth(ringCountLine, 2);
        ui.begin2D();
        ui.ui.rect(14, 40, w, 30, {0.f, 0.f, 0.f, 0.72f * a});
        ui.ui.text(ringCountLine, 24, 48, 2, {0.86f, 0.82f, 0.72f, a});
        ui.end2D();
        return;
    }
    if (c < 0.f) return;
    char num[16];
    if (c >= 3.f) snprintf(num, sizeof(num), "%d", (int)std::ceil(c));   // whole seconds, then tenths for the last three
    else snprintf(num, sizeof(num), "%.1f", c);
    const glm::vec4 red{SWEEP_RED, 1.f};
    ui.begin2D();
    ui.ui.rect(14, 40, 150, 92, {0.f, 0.f, 0.f, 0.72f});   // a dark plate: red stays readable over the giant, and over fire
    ui.ui.text(sweep.spec.kind == SweepSpec::Kind::GATHER ? "GATHER" : "SWEEP", 24, 46, 2, red);
    ui.ui.text(num, 24, 66, 7, red);
    for (int i = 0; i < sweep.wavesLeft(); ++i) ui.ui.rect(24.f + i * 14.f, 122, 9, 5, red);   // waves still to come
    ui.end2D();
}
