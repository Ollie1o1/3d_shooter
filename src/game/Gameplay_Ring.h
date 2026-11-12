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
        const auto t = sweeptier::far(ring::sweepDir());
        sweepSched.arm(t.gapMin, t.gapMax, 0x52494E47u + (uint32_t)deaths * 7919u);
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
    const int tier = g_devSweep > 0 ? g_devSweep : (ring() && !g_devRingTest) ? 1 : 1 + sweepCount % 3;
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
        const auto t = sweeptier::far(ring::sweepDir()); sweepSched.timer = t.gapMin + 10.f;
    } else if (sweepSched.tick(dt, sweep.active(), blocked, rate)) { sweep.start(nextSweep()); ++sweepCount; }
    if (g_devOverlay == "sweepnow" && !sweep.active() && elapsedTime > 0.5f && sweepCount == 0) {   // dev: one mid-count, for screenshots
        SweepSpec s = nextSweep(); s.flashLead = 0.f; s.warning = 6.f; sweep.start(s); ++sweepCount;
    }
    if (ringCountT >= 0.f && (ringCountT -= dt) < 0.f) ringCountT = -1.f;
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
        if (!e.targetable() || isBoss(e.type) || e.type == EnemyType::CROOK || !sweep.sweptNow(e.position, sweepShelters)) continue;   // (a sheepdog stands in it)
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
inline float GameplayState::ringNight() const { return 0.f; }   // (the Shadow Band, when it exists)
inline bool GameplayState::ringQuiet() const {   // inside the open hull: air, and no sweeps
    const AABB H = ring::quietHull(); const glm::vec3 p = player.position;
    return !g_devRingTest && p.x > H.min.x && p.x < H.max.x && p.z > H.min.z && p.z < H.max.z + 1.f && p.y < H.max.y;
}

inline void GameplayState::beginRing() {
    storyOwned |= 15; storyArmed = storyGrapple = true;
    wakeT = -1; clawT = -1; beltBodies.clear(); player.eyeHeight = 1.7f;
    player.camera.farPlane = ring::FAR_PLANE;
    if (!wakeSeen && director.arena == 0) pushBanner(level.story.title, "CHAPTER 6", SWEEP_ICE * 0.6f, 4.f);
    wakeSeen = true;
    if (director.arena >= 2) holdfastFound = true;   // (a checkpoint past I2: it is yours)
    holdfast.reset(); armSweep();
    ringLevelSeen = -1; ringScriptT = -1.f; ringAir = -1; ringKnockOn = -1; ringKnockT = ringCreakT = 0.f; crookDragT = 0.f; crookDragUid = -1; crookSeen = crookLeft = false;
    if (g_devRingTest) ringLevelSeen = 99;
    player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0}; prevCamPos = player.camera.position;
}

inline void GameplayState::updateRing(float dt) {
    player.camera.farPlane = ring::FAR_PLANE;                 // (a retry makes a fresh camera and player)
    player.gravity = -24.f * ring::GRAVITY_SCALE;             // the crossing's low gravity; fast-fall and the slam are untouched
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
    ringTouch(dt);
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
    if (storyEndT < 0.f && !playerDead && p.x > E.min.x && p.x < E.max.x && p.z > E.min.z && p.z < E.max.z) {
        storyEndT = 0.f; saveStoryProgress(); logRun("end"); audio.play("wave", 90, SoundGroup::UI, true);
    }
    if (storyEndT >= 0.f) { storyEndT += dt; player.velocity = {0.f, 0.f, 0.f}; if (storyEndT > 3.f) captureMouse(false); }
    if (storyTerminal >= 0 && glm::length(level.story.terminals[storyTerminal].pos - p) > 6.f) storyTerminal = -1;
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
    // The front: a wall of lifted ice, ragged along its top
    const glm::vec3 dir = sweep.spec.dir, side{-dir.z, 0.f, dir.x};
    const bool cone = sweep.spec.kind == SweepSpec::Kind::CONE;
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
            at.y = player.position.y > 40.f ? 0.f : 0.f;
            const float w = cone ? std::max(0.6f, f * sweep.spec.coneHalf / 40.f * 1.1f) : 4.2f;
            push(out, T(at + glm::vec3{0.f, h * 0.5f, 0.f}) * RY(std::atan2(side.z, -side.x)) * S(glm::vec3{w, h, 0.9f}), SWEEP_ICE * 0.35f, SWEEP_ICE * (0.55f + 0.35f * ring::hash(n, 2.f)));
        }
    }
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
