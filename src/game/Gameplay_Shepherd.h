#pragma once
// =============================================================================
// Gameplay_Shepherd.h — THE SHEPHERD (docs/chapter-6-design.md section 6), the
// chapter's boss. Its level is world/LevelShepherd.h, loaded in place at the
// end of the Shore (loadShepherd). `--shepherd [--arena N]` starts on tine N+1.
//
// One health pool across the five phases, drawn as five segments. Built so far:
//
// PHASE 1 · THE COMB. Three tines, one sheepdog each, back hurt from the
// crossing and with their full kit. The Shepherd's aimed sweep fires down the
// deck from the tine's root horn every ~15 s: a narrow cone at where you stood
// when its lamps flared. Get behind a fin (its post lights) or out of the cone;
// the cone is drawn on the deck in red for the whole countdown.
//
//   THE CROOK       hooks you out of shelter during the countdown. Parry the hook
//                   (white) and it is stuck in the open: if the wave catches it
//                   staggered, the wave takes a fifth of it. Below half it cuts
//                   HOLDFAST lines (gold: let go, or be cut and hauled in).
//   THE DROVER      hangs off the deck's edge and slides its herd (three blocks of
//                   ice: moving shelter) across the deck. It shrugs off most damage
//                   unless its formation breaks: spike a herd block with HOLDFAST
//                   (6 s open), or after it sends a block across you (jump it).
//   THE BELLWETHER  its bell sends rings along the deck (jump them) on its own
//                   clock, under the Shepherd's. Hit the bell while it swings and
//                   the clapper cracks (staggered). Below half it rings twice.
//
// Kill the sheepdog and the tine breaks: it bucks everything else off, its horn
// goes dark, and a spar of it falls across to the next tine. The third breaking
// ends the Comb. (THE TURN, THE RESONATOR, THE BREAK and THE FALL: next.)
// =============================================================================
#include "world/LevelShepherd.h"

namespace shepfight {
constexpr float SWEEP_WARN = 4.f, SWEEP_FLASH = 1.2f;   // the aimed sweep: its countdown, and its lamps before it
constexpr float SWEEP_GAP_MIN = 11.f, SWEEP_GAP_MAX = 15.f;   // after one ends, the next
constexpr float CONE_HALF_WIDTH = 12.f;   // metres either side of the cone's line where it reaches you
constexpr float DOG_HURT = 0.72f;          // what a sheepdog has left from the crossing
constexpr float SWEPT_DOG = 0.2f;          // a staggered sheepdog caught by the wave loses this much of its health
constexpr float CUT_TELL = 1.1f, CUT_COOLDOWN = 7.f;
constexpr float HERD_RATE = 0.28f, PIN_OPEN = 6.f, CHARGE_OPEN = 2.5f;
const glm::vec3 CHARGE_HALF{2.2f, 0.65f, 1.8f};   // the Drover's block across the deck: low enough to jump at full weight
constexpr float BREAK_LEN = 2.2f, SPAR_FALL = 1.4f;
constexpr float PHASES = 5.f;
// THE TURN
constexpr float TURN_LEN = 16.f, TURN_ANGLE = 2.5f;          // the sky rolling round it (radians, about the line to it)
constexpr float RIDGE_WARN = 6.f, RIDGE_FLASH = 1.5f;        // 6 s: a RECALL statue (8 s) planted on the flash covers it
constexpr float VENT_OPEN = 8.f, VENT_HP = 420.f, VENT_REACH = 24.f;   // open after each wave; only hurt from close in
// THE RESONATOR
constexpr float HALL_WARN = 5.f, HALL_FLASH = 1.f, HALL_GAP = 9.f;   // its blasts down the hall (the gap grows ragged as forks go)
constexpr float FORK_RING = 4.5f;                                    // after a blast the forks ring this long: spike one now
constexpr float WELL_WARN = 5.f, WELL_SPEED = 30.f, WELL_GAP = 7.f;  // the blasts up the well
}

inline bool GameplayState::shepherd() const { return ring() && level.story.part == 1; }

inline Enemy* GameplayState::shepDog() {
    Enemy* d = shepDogUid >= 0 ? enemyByUid(shepDogUid) : nullptr;
    return d && d->alive ? d : nullptr;
}

// All five phases on one bar: the Comb is the first fifth (each tine a third of it, the sheepdog's
// health the way there)
inline float GameplayState::shepherdHealth01() {
    float comb = (float)shepTines;
    if (shepTines < shep::TINES && shepBreakT < 0.f)
        if (Enemy* d = shepDog(); d && shepDogMax > 0.f) comb += glm::clamp(1.f - d->health / shepDogMax, 0.f, 1.f) * 0.8f;
    float done = comb / (float)shep::TINES;   // phases done, in fifths
    for (int v = 0; v < 3; ++v) done += glm::clamp(1.f - shepVentHp[v] / shepfight::VENT_HP, 0.f, 1.f) / 3.f;   // THE TURN: its vents
    for (int f = 0; f < shep::FORKS; ++f) done += shepForkBroken[f] ? 0.6f / shep::FORKS : 0.f;              // THE RESONATOR: its forks,
    done += 0.4f * glm::clamp(shepWellMaxY / shep::W_OUT, 0.f, 1.f);                                         // and the climb
    return glm::clamp(1.f - done / shepfight::PHASES, 0.f, 1.f);
}

// In place, from the Shore's end (the run goes on: its clock, kills and style are kept)
inline void GameplayState::loadShepherd(int arena) {
    g_shepherd = true; g_startArena = arena; g_devOverlay.clear();
    world.contact.release(); movingShadows.release();
    level = buildShepherd(); world = buildWorldMeshes(level); spatialGrid.build(level.walls);
    bake = &sharedBake("shepherd", level); applyBake(); neonLights = buildNeonLights(level);
    shifts = ArenaShifts{}; shifts.capture(level);
    anchorsBuilt = level.anchors; anchorBoxes.clear();
    storyGlassBoxes.clear(); storyTileBoxes.clear();
    director = WaveDirector{}; director.level = &level; director.fast = true;
    storySeenSection = storyCheckpoint = arena; storyEndT = -1; storyLog = storyTerminal = -1;
    enemies.clear(); spaceArena = -1; warmupFrames = 2;
    shepVentsArmed = shepTurnSeen = shepForksArmed = false;
    enterArena(arena); captureMouse(true); saveStoryProgress();
}

inline void GameplayState::beginShepherd() {
    ringTextures();
    storyOwned |= 15; storyArmed = storyGrapple = true; holdfastFound = true;
    wakeT = -1; clawT = -1; beltBodies.clear(); player.eyeHeight = 1.7f;
    player.camera.farPlane = ring::FAR_PLANE;
    holdfast.reset(); armSweep();
    resetGap(); resetWake(); crookDragT = 0.f; crookDragUid = -1; crookSeen = crookLeft = true;
    shepTines = std::clamp(director.arena, 0, shep::TINES);
    shepBreakT = -1.f; shepSweepT = 7.f; shepDogUid = -1; shepDogMax = 1.f; shepStrayT = 16.f;
    shepCutT = -1.f; shepCutCd = 0.f; shepHerdT = 0.f; shepHerdPinned = -1; shepHerdPinT = 0.f; shepHerdHeld = false;
    shepBellT = -1.f; shepBellHp = -1.f; shepBellHint = false; shepCharge.on = false; shepWasStaggered = false;
    for (int k = 0; k < 2; ++k) shepBridgeT[k] = k < shepTines ? 99.f : -1.f;
    // THE TURN: the vents keep their damage through a retry (one beat replayed, never the phase)
    if (!shepVentsArmed) { shepVentsArmed = true; for (float& h : shepVentHp) h = shepfight::VENT_HP; }
    shepVents = 0; for (float h : shepVentHp) shepVents += h <= 0.f;
    shepVentOpenT = 0.f; shepRidgeSweepT = 4.f; shepAddT = 6.f; shepSweepPassed = false; shepInT = -1.f; shepVentMsgT = 0.f;
    shepTurnT = -1.f; shepTurnAng = director.arena >= 3 ? shepfight::TURN_ANGLE : 0.f;
    if (director.arena == 3 && !shepTurnSeen) { shepTurnT = 0.f; shepTurnAng = 0.f; }
    if (director.arena == 3) { storySeenSection = 3; shepTines = shep::TINES; }
    if (director.arena >= 4) {   // inside: the ridge behind you (its vents broken, the sky turned)
        storySeenSection = std::max(storySeenSection, director.arena); shepTines = shep::TINES;
        for (float& h : shepVentHp) h = 0.f;
        shepVents = 3; shepTurnAng = shepfight::TURN_ANGLE; shepTurnSeen = true;
    }
    if (!shepForksArmed) { shepForksArmed = true; for (bool& f : shepForkBroken) f = false; shepWellMaxY = 0.f; }
    if (director.arena >= 5) for (bool& f : shepForkBroken) f = true;
    shepForks = 0; for (bool f : shepForkBroken) shepForks += f;
    shepForkRingT = 0.f; shepHallSweepT = 5.f; shepWellT = -1.f; shepWellNext = 4.f; shepWellCount = -1.f; shepUpT = -1.f; shepMembraneT = shepForks >= shep::FORKS ? 99.f : -1.f;
    if (director.arena == 4 && shepForks == 0) for (int k = 0; k < 3; ++k) spawnEnemy(EnemyType::STRAY, {shep::HX + (k - 1) * 8.f, 0.f, shep::HZ_N + 14.f});
    if (director.arena == 0 && !shepSeen) pushBanner("THE SHEPHERD", "UNIT ?  /  PURPOSE: KEEP", SWEEP_ICE * 0.7f, 4.5f);
    shepSeen = true;
    ringLevelSeen = 99; ringScriptT = -1.f; ringAir = -1; ringLeeHint = true;   // (its own hint: ITS AIM)
    player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0}; prevCamPos = player.camera.position;
}

// The game's hold on what moves: the spars (down once their tine is broken) and the herd
inline void GameplayState::shepMovers(float dt) {
    for (int k = 0; k < (int)level.shepBridges.size(); ++k) {
        Mover& m = level.movers[level.shepBridges[k]];
        if (shepBridgeT[k] < 0.f) { m.gone = true; m.drive = 0.f; continue; }
        const float before = shepBridgeT[k];
        shepBridgeT[k] += dt;
        const float u = glm::clamp(shepBridgeT[k] / shepfight::SPAR_FALL, 0.f, 1.f);
        m.gone = false; m.drive = u * u;   // (it falls: slow, then fast)
        if (before < shepfight::SPAR_FALL && shepBridgeT[k] >= shepfight::SPAR_FALL) {   // it lands
            const AABB br = shep::bridge(k);
            for (float x = br.min.x; x < br.max.x; x += 5.f) fx.spawnBurst({x, 0.3f, shep::BRIDGE_Z}, SWEEP_ICE * 0.7f, 6, 6.f, 1.2f, 2.f);
            shake(0.9f, 0.05f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.55f; o.volume = 1.f;
            audio.playOpts("slam", o);
            ui.toast("THE SPAR", "IT FELL ACROSS. CROSS TO THE NEXT TINE", SWEEP_ICE * 0.6f, 4.f);
        }
    }
    for (int f = 0; f < (int)level.shepForks.size(); ++f) level.movers[level.shepForks[f]].gone = shepForkBroken[f / 2];   // (stilled forks stay gone through a retry)
    if (level.shepMembrane >= 0) level.movers[level.shepMembrane].gone = shepMembraneT >= 0.f;
    Enemy* d = shepDog();
    const bool drover = d && d->type == EnemyType::DROVER;
    if (drover) shepHerdT += dt;
    if (shepHerdPinT > 0.f && (shepHerdPinT -= dt) <= 0.f) shepHerdPinned = -1;
    for (int i = 0; i < (int)level.shepHerd.size(); ++i) {
        Mover& m = level.movers[level.shepHerd[i]];
        if (!drover || i == shepHerdPinned) continue;   // (pinned, or nobody driving: it stays where it is)
        const float want = 0.5f + 0.5f * std::sin(shepHerdT * shepfight::HERD_RATE + 2.1f * (float)i);
        m.drive += glm::clamp(want - m.drive, -dt * 0.4f, dt * 0.4f);
    }
}

inline void GameplayState::shepLaunch(const Enemy& d) {
    shepCharge.on = true; shepCharge.hit = false;
    shepCharge.dir = d.position.x < player.position.x ? 1.f : -1.f;
    shepCharge.from = d.position.x;
    shepCharge.pos = {d.position.x, shepfight::CHARGE_HALF.y, glm::clamp(player.position.z, shep::Z_N + 4.f, shep::Z_S - 4.f)};
    audio.playAt("ab_rumble", shepCharge.pos, 120, SoundGroup::ENEMY, true, 0.7f);
    if (Enemy* dd = shepDog()) dd->shepOpen = std::max(dd->shepOpen, shepfight::CHARGE_OPEN + 1.5f);   // (its herd is a block short while it crosses)
}

inline void GameplayState::shepDogKit(Enemy& d, float dt) {
    using namespace shepfight;
    const float cx = shep::tineX(std::clamp(shepTines, 0, 2));
    d.shepOpen = std::max(0.f, d.shepOpen - dt);
    // Newly staggered (a parried hook, a cracked clapper): it stays down long enough for a wave to find it
    const bool stag = d.staggered();
    if (stag && !shepWasStaggered) {
        d.staggerTimer = std::max(d.staggerTimer, 3.5f);
        if (sweep.active() && sweep.lethal()) ui.toast("IT'S IN THE OPEN", "STAGGERED, THE WAVE WILL TAKE IT", POST_LIT * 0.4f, 2.5f);
    }
    shepWasStaggered = stag;
    // Caught in the wave while it is down
    if (stag && sweep.sweptNow(d.position, sweepShelters)) {
        d.takeDamage(d.maxHealth * SWEPT_DOG);
        d.knockVel = sweep.toSource(d.position) * -18.f + glm::vec3{0.f, 6.f, 0.f};
        fx.spawnBurst(d.position + glm::vec3{0.f, 1.5f, 0.f}, SWEEP_ICE, 30, 12.f, 0.8f, 3.f);
        ui.feed("IT TOOK THE WAVE", SWEEP_ICE);
        styleSystem.addStyle(120.f, StyleSource::ENVIRONMENT);
    }
    if (d.type == EnemyType::CROOK) {   // below half: it cuts the line you hold by
        shepCutCd = std::max(0.f, shepCutCd - dt);
        if (shepCutT < 0.f && shepCutCd <= 0.f && holdfast.held() && d.targetable() && !stag && d.health < shepDogMax * 0.55f && d.telegraphTimer <= 0.f) {
            shepCutT = CUT_TELL;
            audio.playAt("telegraph", d.position, 120, SoundGroup::ENEMY, true, 0.8f);
            if (!shepCutHint) { shepCutHint = true; ui.toast("GOLD: IT CUTS YOUR LINE", "LET GO (" + binds().label(Act::HOLDFAST) + ") BEFORE IT LANDS", abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f}) * 0.6f, 3.f); }
        }
        if (shepCutT >= 0.f) {
            d.velocity.x = d.velocity.z = 0.f; d.attackTimer = std::max(d.attackTimer, 0.5f);
            const glm::vec3 hand = d.position + glm::vec3{0.f, 1.6f, 0.f};
            const float u = 1.f - shepCutT / CUT_TELL;
            if (holdfast.held()) fx.spawnTracer(hand, glm::mix(hand, holdfast.point, u), 0.05f, dt * 1.5f, abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f}) * (0.6f + 1.2f * u));
            if ((shepCutT -= dt) < 0.f || stag) {
                shepCutT = -1.f; shepCutCd = CUT_COOLDOWN;
                if (holdfast.held() && !stag && !playerDead) {
                    holdfast.cut();
                    damagePlayer(6.f, d.position + glm::vec3{0.f, 1.5f, 0.f}, 0.2f, 0.03f, 0.f, {EnemyType::CROOK, DamageKind::MELEE});
                    crookDragT = 0.7f; crookDragUid = d.uid; grapple.release();
                    ui.feed("LINE CUT", abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f}));
                    audio.play("hook_yank", 120, SoundGroup::PLAYER, true);
                }
            }
        }
    }
    if (d.type == EnemyType::DROVER) {   // it hangs off the deck's edge, changing sides
        if ((shepSideT += dt) > 12.f) { shepSideT = 0.f; shepSide = -shepSide; }
        d.droverPost = glm::vec3{cx + shepSide * (shep::HALF + 9.f), 8.f, glm::clamp(player.position.z - 6.f, shep::Z_N + 10.f, shep::Z_S - 10.f)};
        // Spike one of its herd and the formation breaks around it
        int herd = -1;
        if (holdfast.held())
            for (int i = 0; i < (int)level.shepHerd.size(); ++i)
                if (holdfast.body == level.movers[level.shepHerd[i]].wall) herd = i;
        if (herd >= 0 && !shepHerdHeld) {
            shepHerdPinned = herd; shepHerdPinT = PIN_OPEN; d.shepOpen = std::max(d.shepOpen, PIN_OPEN);
            pushBanner("FORMATION BROKEN", "THE DROVER IS OPEN", HOLDFAST_STEEL, 2.f);
            audio.play("drive_ready", 110, SoundGroup::UI, true);
        }
        shepHerdHeld = herd >= 0;
        if (d.shepOpen > 0.f && std::fmod(gameClock, 0.15f) < dt) fx.spawnBurst(d.position + glm::vec3{0.f, 1.f, 0.f}, glm::vec3{1.8f, 1.2f, 0.4f}, 2, 2.f, 0.4f, 0.f);
    }
    if (d.type == EnemyType::BELLWETHER) {   // hit the bell as it swings: the clapper cracks
        if (d.attack == AttackKind::RING_BELL && d.telegraphTimer > 0.f) {
            if (shepBellHp < 0.f) {
                shepBellHp = d.health;
                if (!shepBellHint) { shepBellHint = true; ui.toast("ITS BELL SWINGS", "HIT IT NOW: CRACK THE CLAPPER", glm::vec3{1.9f, 1.4f, 0.4f} * 0.6f, 3.f); }
            } else if (d.health < shepBellHp - 25.f) {
                d.attack = AttackKind::NONE; d.telegraphTimer = 0.f;
                d.stagger(3.f); d.staggerTimer = std::max(d.staggerTimer, 3.f); shepBellHp = -1.f;
                pushBanner("CLAPPER CRACKED", "THE FLOCK SCATTERS", glm::vec3{1.9f, 1.4f, 0.4f} * 0.6f, 2.f);
                audio.playAt("ab_shatter", d.position + glm::vec3{0.f, 2.5f, 0.f}, 128, SoundGroup::ENEMY, true, 1.3f);
                fx.spawnBurst(d.position + glm::vec3{0.f, 2.5f, 0.f}, glm::vec3{1.9f, 1.4f, 0.4f}, 30, 9.f, 0.7f, 2.f);
            }
        } else shepBellHp = -1.f;
        if (shepBellT >= 0.f && (shepBellT -= dt) < 0.f && !playerDead) { shepBellT = -1.f; wakeRing(d); }
    }
}

inline void GameplayState::updateShepherd(float dt) {
    using namespace shepfight;
    const glm::vec3 p = player.position;
    const int at = level.arenaAt(p);
    if (at > director.arena && !director.fighting() && director.phase != WaveDirector::Phase::INTRO && at <= shepTines) director.approach(at);
    if (at >= 0 && at > storySeenSection && !playerDead) {
        storySeenSection = at;
        pushBanner(level.arenas[at].name, level.arenas[at].subtitle, SWEEP_ICE * 0.6f, 3.f);
        audio.play("wave", 64, SoundGroup::UI, true);
    }
    if (at >= 0 && at > storyCheckpoint && !director.fighting()) { storyCheckpoint = at; sweepSched.checkpoint(); ui.feed("CHECKPOINT", SWEEP_ICE * 0.6f); saveStoryProgress(); }
    shepMovers(dt);
    // Its sweeps are its own (the random clock is held off)
    sweepSched.timer = std::max(sweepSched.timer, 60.f);
    const int k = std::clamp(shepTines, 0, shep::TINES - 1);   // the tine in play (the director moves on when its wave clears)
    // The sheepdog: found as it arrives (hurt from the crossing; its full kit from here)
    if (!shepDog() && shepBreakT < 0.f && shepTines < shep::TINES && (shepDogUid >= 0 || at == shepTines)) {
        if (shepDogUid >= 0) { shepBreakT = 0.f; shepDogUid = -1; }   // it died: the tine breaks
        else for (auto& e : enemies)
            if (e.alive && isSheepdog(e.type) && e.state == EnemyState::ACTIVE) {
                shepDogUid = e.uid; e.tineForm = true; e.health = e.maxHealth * DOG_HURT; shepDogMax = e.health;
                shepSideT = 0.f; shepSide = -1.f; shepWasStaggered = false;
                static const char* lines[3] = {"IT CAME BACK FOR YOU", "IT BROUGHT THE HERD", "IT RINGS FOR THE SHEPHERD"};
                pushBanner(e.stats().name, lines[k], HOLDFAST_STEEL, 3.f);
                audio.playAt("hook_yank", e.position, 110, SoundGroup::ENEMY, true, 0.5f);
                shepSweepT = std::max(shepSweepT, 6.f);
                break;
            }
    }
    Enemy* dog = shepDog();
    if (dog && g_devOverlay == "shepkill" && dog->age > 2.5f && dog->takeDamage(1e6f)) { onEnemyKilled(*dog, StyleSource::ENVIRONMENT); dog = nullptr; }   // dev: the tine breaking
    if (dog) shepDogKit(*dog, dt);
    // The aimed sweep, down the deck from the horn, while its tine's fight is on
    const bool fight = dog != nullptr && shepBreakT < 0.f && !playerDead && at == shepTines;
    if (fight && !sweep.active() && (shepSweepT -= dt) <= 0.f) {
        const glm::vec3 o = shep::horn(k);
        SweepSpec s = sweeptier::aimed(o, p, SWEEP_WARN);
        s.flashLead = SWEEP_FLASH; s.range = 40.f;
        const float dist = glm::length(glm::vec2{p.x - o.x, p.z - o.z});
        s.coneHalf = glm::clamp(std::atan(CONE_HALF_WIDTH / std::max(dist, 1.f)), 0.05f, 0.5f);
        sweep.start(s); ++sweepCount;
        shepSweepT = cosmetic::range(SWEEP_GAP_MIN, SWEEP_GAP_MAX) + SWEEP_FLASH + SWEEP_WARN;
        if (sweepCount == 1 || !shepConeHint) { shepConeHint = true; ui.toast("ITS AIM", "THE RED CONE: GET OUT OF IT, OR BEHIND A FIN BY ITS LIT POST", SWEEP_RED * 0.8f, 4.5f); }
    }
    if (!fight && !sweep.active()) shepSweepT = std::max(shepSweepT, 5.f);
    // Pairs of strays climb up over the rails just after each wave has gone through (it sends the flock
    // back in behind its sweep), never in the middle of a countdown
    const bool waveDone = shepSweepWas && !sweep.active();
    shepSweepWas = sweep.active();
    if (fight && waveDone) {
        int strays = 0; for (const auto& e : enemies) strays += e.alive && e.type == EnemyType::STRAY;
        if (strays < 3)
            for (int sd = -1; sd <= 1; sd += 2) {
                const glm::vec3 q{shep::tineX(k) + sd * (shep::HALF - 2.f), 0.f, glm::clamp(p.z + cosmetic::range(-18.f, 18.f), shep::Z_N + 8.f, shep::Z_S - 8.f)};
                spawnEnemy(EnemyType::STRAY, q);
                fx.spawnBurst(q + glm::vec3{0.f, 0.5f, 0.f}, SWEEP_ICE * 0.6f, 10, 4.f, 0.8f, 1.f);
            }
    }
    // The Drover's block across the deck
    if (shepCharge.on) {
        const glm::vec3 h = CHARGE_HALF;
        shepCharge.pos.x += shepCharge.dir * gapride::CHARGE_SPEED * dt;
        if (!shepCharge.hit && !playerDead && std::fabs(p.x - shepCharge.pos.x) < h.x + 0.4f && std::fabs(p.z - shepCharge.pos.z) < h.z + 0.4f && p.y < shepCharge.pos.y + h.y) {
            shepCharge.hit = true;
            damagePlayer(gapride::CHARGE_DAMAGE, shepCharge.pos, 0.3f, 0.05f, 0.f, {EnemyType::DROVER, DamageKind::MELEE});
            holdfast.cut();
            player.velocity += glm::vec3{shepCharge.dir * 9.f, 5.f, 0.f}; player.launched();
            ui.feed("RAMMED", HOLDFAST_STEEL);
        }
        if (std::fabs(shepCharge.pos.x - shepCharge.from) > 2.f * shep::HALF + 24.f) shepCharge.on = false;
    }
    // The bow waves: ice thrown up off each tine's west face (near you)
    for (int t = 0; t < shep::TINES; ++t)
        if (std::fabs(p.x - shep::tineX(t)) < 160.f && std::fmod(gameClock, 0.05f) < dt) {
            const glm::vec3 at2{shep::tineX(t) - shep::HALF - 3.f, shep::PLAIN_Y + 1.f, cosmetic::range(shep::Z_N, shep::Z_S)};
            fx.spawnSpray(at2, glm::normalize(glm::vec3{-0.7f, 1.f, cosmetic::range(-0.2f, 0.2f)}), SWEEP_ICE * 0.7f, 2, 12.f, 1.6f, 0.6f);
        }
    // The tine breaking
    if (shepBreakT >= 0.f) {
        const float before = shepBreakT;
        shepBreakT += dt;
        const float cx = shep::tineX(k);
        if (before == 0.f) {
            pushBanner("THE TINE BREAKS", k == 2 ? "THE COMB IS BROKEN" : "IT FELT THAT", SWEEP_ICE * 0.8f, 3.5f);
            shake(1.4f, 0.06f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.4f; o.volume = 1.f;
            audio.playOpts("slam", o); o.delay = 0.7f; o.pitch = 0.33f; audio.playOpts("slam", o);
            sweep.reset();
            for (auto& e : enemies)   // the tine bucks: everything else on it goes
                if (e.targetable() && !isSheepdog(e.type) && e.takeDamage(1e6f)) {
                    onEnemyKilled(e, StyleSource::ENVIRONMENT);
                    if (sweptBodies.size() < 24) sweptBodies.push_back({e.position + glm::vec3{0.f, e.height() * 0.5f, 0.f}, glm::vec3{-1.f, 0.f, 0.f}, glm::vec3{e.radius() * 1.6f, e.height(), e.radius() * 1.2f}, e.stats().color, 0.f, (float)(e.uid % 7) - 3.f});
                }
            shepCharge.on = false; shepherdDamageT = 1.f;
        }
        if (shepBreakT < 1.6f) {   // cracking down its length, root to tip
            const float z = glm::mix(shep::Z_N, shep::Z_S, shepBreakT / 1.6f);
            if (std::fmod(gameClock, 0.06f) < dt) {
                fx.spawnBurst({cx + cosmetic::range(-14.f, 14.f), 0.4f, z}, glm::vec3{2.f, 1.8f, 1.5f}, 12, 9.f, 0.6f, 3.f);
                fx.spawnShockwave({cx, 0.2f, z}, 4.f, SWEEP_ICE * 0.6f);
            }
            shake(0.12f, 0.02f);
        }
        if (k < shep::TINES - 1 && before < BREAK_LEN && shepBreakT >= BREAK_LEN) shepBridgeT[k] = 0.f;
        if (before < BREAK_LEN && shepBreakT >= BREAK_LEN) { shepTines = std::max(shepTines, k + 1); shepBreakT = -1.f; if (k == shep::TINES - 1) shepEndT = 0.f; }
    }
    // The Comb is broken: it turns (THE TURN is the next phase; for now the run ends here)
    if (shepEndT >= 0.f) {
        const float before = shepEndT;
        shepEndT += dt;
        if (before < 3.f && shepEndT >= 3.f) {
            pushBanner("STAY.", "EVERYTHING HERE STAYS.", glm::vec3{0.9f, 0.9f, 0.82f}, 4.5f);
            shake(1.6f, 0.04f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.4f; o.volume = 0.9f;
            audio.playOpts("slam", o);
        }
        if (shepEndT > 3.f && shepEndT < 6.f) shake(0.1f, 0.015f);   // it begins to roll
        if (before < 6.f && shepEndT >= 6.f) {   // and the tine goes from under you: on to its ridge (THE TURN)
            shepEndT = -1.f; sweep.reset();
            fx.spawnBurst(player.camera.position + glm::vec3{0.f, 0.f, -2.f}, SWEEP_ICE, 60, 14.f, 0.8f, 2.f);
            storySeenSection = storyCheckpoint = 3;
            enterArena(3); saveStoryProgress();
            return;
        }
    }
    shepRidge(dt);
    shepInside(dt);
    if (shepherdDamageT > 0.f) shepherdDamageT = std::max(0.f, shepherdDamageT - dt);
    if (storyEndT >= 0.f) { storyEndT += dt; player.velocity = {0.f, 0.f, 0.f}; if (storyEndT > 3.f) captureMouse(false); }
}

// =============================================================================
// PHASE 2 · THE TURN. It rolls over to face you (the sky swings round it), then its ridge
// fires: waves straight down the ice from the north, 6 s, and after each one its three vents
// stand open for 8 s. Get to one, break it from close in, and get back behind a block
// before the next (or plant RECALL in shelter on the flash and snap back on 1).
// =============================================================================
inline void GameplayState::shepRidge(float dt) {
    using namespace shepfight;
    if (shepTines < shep::TINES || level.arenaAt(player.position) != 3) return;
    const glm::vec3 p = player.position;
    shepVentMsgT = std::max(0.f, shepVentMsgT - dt);
    if (shepTurnT >= 0.f) {   // the turn: slow, the sky rolling round it, the music down to its pulse
        const float before = shepTurnT;
        shepTurnT += dt;
        float u = glm::clamp(shepTurnT / TURN_LEN, 0.f, 1.f); u = u * u * (3.f - 2.f * u);
        shepTurnAng = TURN_ANGLE * u;
        if (before == 0.f) pushBanner("THE TURN", "IT ROLLS OVER TO FACE YOU", SWEEP_ICE * 0.7f, 4.f);
        shake(0.1f, 0.006f + 0.01f * std::sin(3.14159f * u));
        if (std::floor(before / 2.6f) != std::floor(shepTurnT / 2.6f)) {   // its mass grinding through the ring, through your boots
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.3f + 0.05f * u; o.volume = 0.7f;
            audio.playOpts("slam", o);
        }
        if (before < TURN_LEN * 0.55f && shepTurnT >= TURN_LEN * 0.55f) pushBanner("ITS FLEECE", "THE ONES IT KEPT CLOSEST", glm::vec3{2.4f, 1.3f, 0.35f} * 0.5f, 3.5f);
        if (shepTurnT >= TURN_LEN) {
            shepTurnT = -1.f; shepTurnSeen = true; shepTurnAng = TURN_ANGLE;
            pushBanner("ITS RIDGE", "AFTER EACH WAVE ITS VENTS OPEN: BREAK ALL THREE", SWEEP_ICE * 0.7f, 4.f);
            ui.toast("RECALL", "PLANT IT (" + binds().label(Act::RECALL) + ") BEHIND A BLOCK ON THE FLASH, RUN IN, SNAP BACK", HOLDFAST_STEEL, 6.f);
            shepRidgeSweepT = 3.f;
        }
        for (auto& e : enemies) if (e.alive) e.attackTimer = std::max(e.attackTimer, 1.f);
        return;
    }
    // In: all three broken
    if (shepInT >= 0.f) {
        const float before = shepInT;
        shepInT += dt;
        if (before == 0.f) {
            pushBanner("IT'S OPEN", "THE WAY IN", SWEEP_ICE * 0.8f, 4.f);
            sweep.reset();
            for (auto& e : enemies) if (e.targetable() && e.takeDamage(1e6f)) onEnemyKilled(e, StyleSource::ENVIRONMENT);
        }
        if (before < 4.f && shepInT >= 4.f) {   // in, through the broken vent
            shepInT = -1.f;
            fx.spawnBurst(player.camera.position + glm::vec3{0.f, 0.f, -2.f}, glm::vec3{2.4f, 1.4f, 0.5f}, 60, 14.f, 0.8f, 2.f);
            storySeenSection = storyCheckpoint = 4;
            enterArena(4); saveStoryProgress();
        }
        return;
    }
    if (playerDead) return;
    // Its waves: straight down the ice, on its own clock
    if (sweep.active() && sweep.passed > 0 && !shepSweepPassed) {   // gone over: the vents open
        shepSweepPassed = true; shepVentOpenT = VENT_OPEN; shepRidgeSweepT = VENT_OPEN + 0.5f;
        ui.feed("VENTS OPEN", glm::vec3{2.4f, 1.3f, 0.35f} * 0.6f);
        // ...and it drops its flock off the fleece in front of them: guards, between you and the vents
        int alive = 0; for (const auto& e : enemies) alive += e.alive && e.targetable();
        ++shepGuardWave;
        for (int v = 0; v < 3 && alive < 7; ++v) {
            if (shepVentHp[v] <= 0.f) continue;
            const glm::vec3 c = shep::vent(v);
            for (int sd = -1; sd <= 1 && alive < 7; sd += 2, ++alive) {
                const glm::vec3 q{c.x + sd * 4.5f, 0.f, shep::RIDGE_Z + 7.f};
                spawnEnemy(EnemyType::STRAY, q);
                fx.spawnBurst(q + glm::vec3{0.f, 8.f, 0.f}, glm::vec3{0.7f}, 12, 5.f, 0.8f, 4.f);
            }
        }
        if (shepGuardWave % 2 == 0 && alive < 8) {   // every second wave a Herder too
            const glm::vec3 q{(shep::RX0 + shep::RX1) * 0.5f, 2.f, shep::RIDGE_Z + 12.f};
            spawnEnemy(EnemyType::HERDER, q);
        }
        for (int v = 0; v < 3; ++v) if (shepVentHp[v] > 0.f) audio.playAt("barrier", shep::vent(v), 110, SoundGroup::WORLD, true, 0.6f);
    }
    if (!sweep.active()) shepSweepPassed = false;
    shepRidgeSweepT -= dt;
    if (!sweep.active() && shepRidgeSweepT <= 0.f) {
        SweepSpec s; s.kind = SweepSpec::Kind::OUT; s.dir = {0.f, 0.f, 1.f};
        s.warning = RIDGE_WARN; s.flashLead = RIDGE_FLASH; s.clocked = true; s.range = 40.f;
        sweep.start(s); ++sweepCount;
        shepRidgeSweepT = 30.f;   // (set again when it goes over)
    }
    if (g_devOverlay == "ventkill" && shepVentOpenT > 0.f)   // dev: a round down each open vent from 10 m (the hit test, the breaking)
        for (int v = 0; v < 3; ++v) { const glm::vec3 from = shep::vent(v) + glm::vec3{0.f, 0.f, 10.f}; float d = 100.f; shootVent(from, {0.f, 0.f, -1.f}, d, 1000.f); }
    if (shepVentOpenT > 0.f) {
        const float before = shepVentOpenT;
        shepVentOpenT = std::max(0.f, shepVentOpenT - dt);
        if (before > 0.f && shepVentOpenT <= 0.f) ui.feed("VENTS SHUT", glm::vec3{0.7f});
    }
    (void)p;
}

// =============================================================================
// PHASE 3 · THE RESONATOR (inside: air again, the hum you only felt is a chord now)
//   THE TUNING HALL  its blasts come down the hall from the core every ~9 s (5 s count): get into a
//                    side room. After each, its four forks ring for 4.5 s: spike a ringing fork with
//                    HOLDFAST and it is stilled. Each stilled fork makes the blasts come less evenly,
//                    and the corner number waver. All four: the core's membrane parts.
//   THE LEDGER       quiet. Its terminals. Every lamp on its walls a capsule.
//   THE LAMP WELL    grapple up from alcove to alcove; blasts come up the shaft (5 s): be in an alcove
//                    when it reaches your height. Out at the top: its surface.
// =============================================================================
inline bool GameplayState::shepInAlcove(glm::vec3 feet) const {
    if (feet.z > shep::WZ_S - 0.5f && feet.y < 5.f) return true;   // (the way in, at the foot: under its lintel)
    for (int i = 0; i < shep::WELL_ALCOVES; ++i) {
        const AABB a = shep::wellAlcove(i);
        if (feet.x > a.min.x - 0.3f && feet.x < a.max.x + 0.3f && feet.z > a.min.z && feet.z < a.max.z && feet.y > a.min.y - 0.6f && feet.y < a.max.y) return true;
    }
    return false;
}

inline void GameplayState::shepInside(float dt) {
    using namespace shepfight;
    const int at = level.arenaAt(player.position);
    if (at < 4 || playerDead || storyEndT >= 0.f) { shepWellCount = -1.f; return; }
    const glm::vec3 p = player.position;
    // ---- THE TUNING HALL ----
    if (at == 4 && shepForks < shep::FORKS) {
        if (sweep.active() && sweep.passed > 0 && !shepSweepPassed) {   // it went over: the forks ring
            shepSweepPassed = true; shepForkRingT = FORK_RING;
            ui.feed("THE FORKS RING", POST_LIT * 0.5f);
            for (int f = 0; f < shep::FORKS; ++f) if (!shepForkBroken[f]) audio.playAt("ab_ting", shep::forkNode(f), 110, SoundGroup::WORLD, true, 0.55f + 0.08f * f);
            int alive = 0; for (const auto& e : enemies) alive += e.alive && e.targetable();
            for (int k = 0; k < 2 && alive < 4; ++k, ++alive) {   // and its flock comes back down the hall behind it
                const glm::vec3 q{shep::HX + (k ? 6.f : -6.f), 0.f, shep::HZ_N + 6.f};
                spawnEnemy(EnemyType::STRAY, q);
            }
        }
        if (!sweep.active()) shepSweepPassed = false;
        shepForkRingT = std::max(0.f, shepForkRingT - dt);
        shepHallSweepT -= dt;
        if (!sweep.active() && shepHallSweepT <= 0.f) {
            SweepSpec s; s.kind = SweepSpec::Kind::OUT; s.dir = {0.f, 0.f, 1.f};
            s.warning = HALL_WARN; s.flashLead = HALL_FLASH; s.clocked = true; s.range = 30.f;
            sweep.start(s); ++sweepCount;
            // detuned: the rhythm goes (each warning stays whole and honest)
            shepHallSweepT = HALL_GAP + HALL_FLASH + HALL_WARN + cosmetic::range(-1.f, 1.f) * 2.2f * (float)shepForks;
            if (!shepHallHint) { shepHallHint = true; ui.toast("THE TUNING HALL", "BLASTS COME DOWN THE HALL: GET INTO A SIDE ROOM. THEN SPIKE A RINGING FORK (" + binds().label(Act::HOLDFAST) + ")", POST_LIT * 0.4f, 6.f); }
        }
        if (g_devOverlay == "forkkill" && shepForkRingT > 0.f && !holdfast.held())   // dev: the spike, into the first ringing fork
            for (int f = 0; f < shep::FORKS; ++f) if (!shepForkBroken[f]) { const int w = level.movers[level.shepForks[2 * f]].wall; holdfast.latch(shep::forkNode(f), player.camera.position, w, -1); break; }
        // A fork spiked while it rings: stilled
        if (holdfast.held())
            for (int f = 0; f < shep::FORKS; ++f) {
                if (shepForkBroken[f]) continue;
                const bool on = holdfast.body == level.movers[level.shepForks[2 * f]].wall || holdfast.body == level.movers[level.shepForks[2 * f + 1]].wall;
                if (!on) continue;
                if (shepForkRingT <= 0.f) {
                    if (shepVentMsgT <= 0.f) { shepVentMsgT = 2.5f; ui.feed("IT ISN'T RINGING: AFTER A BLAST", glm::vec3{0.75f}); }
                    continue;
                }
                shepForkBroken[f] = true; ++shepForks; holdfast.cut();
                const glm::vec3 n = shep::forkNode(f);
                fx.spawnBurst(n + glm::vec3{0.f, 6.f, 0.f}, POST_LIT, 70, 14.f, 1.f, 3.f);
                fx.spawnShockwave(n, 10.f, POST_LIT * 0.6f);
                shake(1.f, 0.05f); shepherdDamageT = 1.f;
                audio.playAt("ab_shatter", n, 128, SoundGroup::WORLD, true, 0.6f);
                SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.45f; o.volume = 0.9f;
                audio.playOpts("slam", o);
                styleSystem.addStyle(150.f, StyleSource::ENVIRONMENT);
                static const char* left[4] = {"THREE LEFT. IT SOUNDS WRONG", "TWO LEFT", "ONE LEFT", "DETUNED"};
                pushBanner("FORK STILLED", left[std::clamp(shepForks - 1, 0, 3)], POST_LIT * 0.6f, 2.5f);
                if (shepForks >= shep::FORKS) {
                    shepMembraneT = 0.f; sweep.reset();
                    pushBanner("THE CORE OPENS", "ON, THROUGH IT", POST_LIT * 0.6f, 3.5f);
                    fx.spawnBurst({shep::HX, 10.f, shep::HZ_N + 2.f}, glm::vec3{2.4f, 1.3f, 0.35f}, 80, 16.f, 1.2f, 2.f);
                }
                break;
            }
    }
    if (shepMembraneT >= 0.f) shepMembraneT += dt;
    // ---- THE LEDGER: nothing comes here ----
    if (at == 5 && !shepLedgerSeen) { shepLedgerSeen = true; ui.toast("THE LEDGER", "READ IT IF YOU LIKE. THE WELL IS ON, NORTH", POST_LIT * 0.4f, 4.f); }
    if (storyTerminal >= 0 && glm::length(level.story.terminals[storyTerminal].pos - p) > 6.f) storyTerminal = -1;
    // ---- THE LAMP WELL ----
    shepWellCount = -1.f;
    if (at == 6) {
        shepWellMaxY = std::max(shepWellMaxY, p.y);
        if (shepUpT >= 0.f) {   // out: its surface
            const float before = shepUpT; shepUpT += dt;
            if (before < 5.f && shepUpT >= 5.f && storyEndT < 0.f) { storyEndT = 0.f; logRun("end"); audio.play("wave", 90, SoundGroup::UI, true); }   // (THE BREAK: next)
            return;
        }
        if (p.y > shep::W_OUT) {
            shepUpT = 0.f; shepWellT = -1.f;
            pushBanner("ITS SURFACE", "IT IS LOSING ITS GRIP", SWEEP_ICE * 0.8f, 4.f);
            shake(1.2f, 0.04f);
            return;
        }
        if (!shepWellHint) { shepWellHint = true; ui.toast("THE LAMP WELL", "GRAPPLE UP FROM ALCOVE TO ALCOVE. WHEN A BLAST COMES UP THE SHAFT, BE IN ONE", POST_LIT * 0.4f, 6.f); }
        if (shepWellT < 0.f && p.z < shep::WZ_S && (shepWellNext -= dt) <= 0.f) {   // (once you are in the shaft)   // a blast: the lamps below flare, then it climbs
            shepWellT = 0.f;
            audio.play("telegraph", 128, SoundGroup::WORLD, true);
            shake(0.3f, 0.012f);
        }
        if (shepWellT >= 0.f) {
            const float before = shepWellT;
            shepWellT += dt;
            const float due = HALL_FLASH + WELL_WARN, left = due - shepWellT;
            if (shepWellT >= HALL_FLASH && left > 0.f) {
                shepWellCount = left;
                if (std::ceil(left) != std::ceil(left + dt)) audio.play("count_tick", left < 3.5f ? 128 : 90, SoundGroup::UI, true);
            }
            shepWellFront = p.y - left * WELL_SPEED;   // (clocked: it reaches your height at zero, wherever you climb to)
            if (before < due && shepWellT >= due) {
                if (shepInAlcove(p)) {
                    ui.feed("IT WENT PAST", SWEEP_ICE * 0.7f);
                    audio.duck(30.f, 0.4f); shake(0.4f, 0.03f);
                    SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.delay = 0.3f; audio.playOpts("skim", o);
                } else {
                    invincFrames = 0.f;
                    if (damagePlayer(1e6f, p - glm::vec3{0.f, 4.f, 0.f}, 0.6f, 0.08f, 0.f, {EnemyType::COUNT, DamageKind::SWEEP})) {
                        holdfast.cut();
                        player.velocity = glm::vec3{0.f, 40.f, 0.f}; player.launched();
                        sweptT = 0.f; sweptSkip = 0.f; sweptFrom = player.camera.position; sweptDir = glm::vec3{0.f, 0.f, 1.f};
                        audio.duck(60.f, SWEPT_FALL + 0.3f);
                    }
                }
            }
            if (shepWellT > due + 3.f) { shepWellT = -1.f; shepWellNext = WELL_GAP; }
        }
    }
}

// A round down a vent's throat (Gameplay_Combat.h's hitscan): only while it is open, and only from close in
inline bool GameplayState::shootVent(glm::vec3 from, glm::vec3 dir, float& distance, float dmg) {
    using namespace shepfight;
    if (!shepherd() || shepTines < shep::TINES || shepTurnT >= 0.f) return false;
    for (int v = 0; v < 3; ++v) {
        if (shepVentHp[v] <= 0.f) continue;
        const glm::vec3 c = shep::vent(v);
        const AABB box{c - glm::vec3{shep::VENT_HX, shep::VENT_HY, 0.4f}, c + glm::vec3{shep::VENT_HX, shep::VENT_HY, 0.4f}};
        const float t = rayBoxHit(from, dir, box);
        if (t <= 0.f || t >= distance) continue;
        distance = t;
        const glm::vec3 at = from + dir * t;
        if (shepVentOpenT <= 0.f) {
            fx.spawnHitSparks(at, glm::vec3{0.8f});
            if (shepVentMsgT <= 0.f) { shepVentMsgT = 2.f; ui.feed("SHUT: IT OPENS AFTER A WAVE", glm::vec3{0.75f}); }
            return true;
        }
        if (t > VENT_REACH) {
            fx.spawnHitSparks(at, glm::vec3{0.8f, 0.6f, 0.3f});
            if (shepVentMsgT <= 0.f) { shepVentMsgT = 2.f; ui.feed("IT GLANCES OFF: GET IN CLOSE", glm::vec3{2.4f, 1.3f, 0.35f} * 0.5f); }
            return true;
        }
        shepVentHp[v] -= dmg; shepherdDamageT = 0.3f;
        fx.spawnHitSparks(at, glm::vec3{2.4f, 1.3f, 0.35f});
        styleSystem.addStyle(4.f, StyleSource::ENVIRONMENT);
        if (shepVentHp[v] <= 0.f) {
            shepVentHp[v] = 0.f; ++shepVents;
            fx.spawnBurst(c, glm::vec3{2.4f, 1.6f, 0.6f}, 60, 16.f, 1.f, 3.f);
            fx.spawnShockwave(c, 8.f, glm::vec3{2.4f, 1.3f, 0.35f});
            shake(1.f, 0.05f);
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.5f; o.volume = 1.f;
            audio.playOpts("slam", o); audio.playAt("ab_shatter", c, 128, SoundGroup::WORLD, true, 0.7f);
            styleSystem.addStyle(150.f, StyleSource::ENVIRONMENT);
            static const char* left[3] = {"TWO LEFT", "ONE LEFT", "ALL THREE"};
            pushBanner("VENT BROKEN", left[std::clamp(shepVents - 1, 0, 2)], glm::vec3{2.4f, 1.3f, 0.35f} * 0.6f, 2.5f);
            if (shepVents >= 3) shepInT = 0.f;
        }
        return true;
    }
    return false;
}

// The cone on the deck, its horn's lamps (dark once its tine is broken), the Drover's block
inline void GameplayState::gatherShepherd(std::vector<BoxInstance>& out) {
    using namespace rig;
    if (!shepherd()) return;
    if (sweep.active() && sweep.spec.kind == SweepSpec::Kind::CONE && sweep.lethal()) {
        const glm::vec3 o = sweep.spec.origin, ax = sweep.spec.dir;
        const float lit = sweep.phase == RingSweep::Phase::FLASH ? 0.5f + 0.5f * std::min(1.f, sweep.t / 0.3f) : 1.f;
        for (int sd = -1; sd <= 1; sd += 2) {   // its two edges, red, all the way down the deck
            const float a = sd * sweep.spec.coneHalf;
            const glm::vec3 d{ax.x * std::cos(a) - ax.z * std::sin(a), 0.f, ax.x * std::sin(a) + ax.z * std::cos(a)};
            for (float r = 14.f; r < 240.f; r += 6.f) {
                const glm::vec3 c = glm::vec3{o.x, 0.06f, o.z} + d * (r + 2.5f);
                push(out, T(c) * RY(std::atan2(d.x, d.z)) * S(glm::vec3{0.35f, 0.05f, 4.6f}), glm::vec3{0.f}, SWEEP_RED * 2.2f * lit);
            }
        }
        // ...and red chevrons down its middle, toward you
        for (float r = 20.f; r < 240.f; r += 12.f) {
            const glm::vec3 c = glm::vec3{o.x, 0.05f, o.z} + ax * r;
            push(out, T(c) * RY(std::atan2(ax.x, ax.z)) * S(glm::vec3{std::tan(sweep.spec.coneHalf) * r * 2.f, 0.03f, 0.5f}), glm::vec3{0.f}, SWEEP_RED * 0.5f * lit);
        }
        if (sweep.phase != RingSweep::Phase::FLASH && sweep.passed < sweep.waves()) {   // the front: an arc of lifted ice racing down
            const float f = sweep.front(0);
            if (f > 12.f)
                for (int i = -6; i <= 6; ++i) {
                    const float a = sweep.spec.coneHalf * i / 6.f;
                    const glm::vec3 d{ax.x * std::cos(a) - ax.z * std::sin(a), 0.f, ax.x * std::sin(a) + ax.z * std::cos(a)};
                    const glm::vec3 c = glm::vec3{o.x, 1.6f, o.z} + d * f;
                    push(out, T(c) * RY(std::atan2(d.x, d.z)) * S(glm::vec3{f * sweep.spec.coneHalf / 3.f + 0.5f, 3.2f, 0.6f}), SWEEP_ICE * 0.4f, SWEEP_ICE * 1.4f);
                }
        }
    }
    for (int t = 0; t < shep::TINES; ++t) {   // a broken tine's horn: its lamps out
        if (t >= shepTines) {
            if (t == shepTines && ringFlash() > 0.f) {   // the live horn flaring
                const glm::vec3 h = shep::horn(t);
                push(out, T(h + glm::vec3{0.f, 0.f, 2.f}) * S(glm::vec3{7.f, 5.f, 0.3f}), glm::vec3{0.f}, POST_LIT * 1.6f * ringFlash());
            }
            continue;
        }
        const float x0 = shep::tineX(t) - shep::HALF;
        push(out, T(glm::vec3{shep::tineX(t), 8.f, shep::Z_N + 0.35f}) * S(glm::vec3{2.f * shep::HALF - 4.f, 10.f, 0.3f}), glm::vec3{0.02f});
        (void)x0;
    }
    if (shepTines >= shep::TINES) {   // THE TURN: its count, in six great lamps across the ridge's face. All lit on the
        // flash; one goes out with each second of the countdown; orange while its vents stand open
        const float c = sweep.countdown(player.position);
        for (int i = 0; i < 6; ++i) {
            const glm::vec3 at{shep::RX0 + 12.f + (shep::RX1 - shep::RX0 - 24.f) * i / 5.f, 21.f, shep::RIDGE_Z + 0.4f};
            glm::vec3 glow{0.05f};
            if (sweep.active() && sweep.phase == RingSweep::Phase::FLASH) glow = POST_LIT * 2.2f;
            else if (c >= 0.f) glow = (float)i < std::ceil(c) ? SWEEP_RED * 3.f : glm::vec3{0.08f, 0.01f, 0.01f};
            else if (shepVentOpenT > 0.f) glow = glm::vec3{2.4f, 1.3f, 0.35f} * (0.8f + 0.4f * (float)((int)(shepVentOpenT * 2.f) % 2));
            else glow = POST_LIT * 0.25f;
            push(out, T(at) * S(glm::vec3{6.f, 3.f, 0.6f}), glm::vec3{0.05f}, glow);
        }
    }
    for (int v = 0; v < 3 && shepTines >= shep::TINES; ++v) {   // THE TURN: its vents (shut, open, broken)
        const glm::vec3 c = shep::vent(v);
        const glm::vec3 hs{shep::VENT_HX, shep::VENT_HY, 0.f};
        if (shepVentHp[v] <= 0.f) {   // broken: a black hole, smoking
            push(out, T(c + glm::vec3{0.f, 0.f, 0.05f}) * S(glm::vec3{hs.x * 2.f, hs.y * 2.f, 0.1f}), glm::vec3{0.01f});
            if (std::fmod(gameClock + v * 0.13f, 0.2f) < 0.017f) fx.spawnBurst(c + glm::vec3{cosmetic::range(-2.f, 2.f), 1.f, 1.f}, glm::vec3{0.15f}, 1, 1.5f, 1.6f, -1.f);
            continue;
        }
        const float open = glm::clamp(shepVentOpenT / 0.6f, 0.f, 1.f) * glm::clamp((shepfight::VENT_OPEN - (shepfight::VENT_OPEN - shepVentOpenT)) / 0.6f, 0.f, 1.f);
        const float hurt = 1.f - shepVentHp[v] / shepfight::VENT_HP, pulse = 0.75f + 0.25f * std::sin(gameClock * 7.f + v);
        push(out, T(c) * S(glm::vec3{hs.x * 2.f, hs.y * 2.f, 0.2f}), glm::vec3{0.02f}, glm::vec3{2.4f, 1.3f, 0.35f} * (0.08f + open * (1.4f + hurt) * pulse));   // its throat
        for (int i = 0; i < 5; ++i) {   // louvers: shut flat across it, open turned edge-on
            const float y = c.y - hs.y + 0.45f + 0.85f * i;
            push(out, T(glm::vec3{c.x, y, c.z + 0.45f}) * RX(open * 1.35f) * S(glm::vec3{hs.x * 2.f, 0.78f, 0.12f}), glm::vec3{0.2f, 0.19f, 0.18f});
        }
        if (open > 0.5f && std::fmod(gameClock + v * 0.11f, 0.1f) < 0.017f)   // its heat pouring out
            fx.spawnBurst(c + glm::vec3{cosmetic::range(-2.5f, 2.5f), cosmetic::range(-1.5f, 1.5f), 1.f}, glm::vec3{2.4f, 1.4f, 0.5f}, 1, 3.f, 0.5f, 0.5f);
    }
    // THE RESONATOR: its forks (ringing white after a blast), the core's membrane, the blast in the well
    for (int f = 0; f < (int)level.shepForks.size(); ++f) {
        const Mover& m = level.movers[level.shepForks[f]];
        if (m.gone) continue;
        const AABB& bx = level.walls[m.wall].box;
        const float ring = shepForkRingT / shepfight::FORK_RING, shiver = ring > 0.f ? 0.05f * std::sin(gameClock * 90.f + f) : 0.f;
        push(out, T((bx.min + bx.max) * 0.5f + glm::vec3{shiver, 0.f, 0.f}) * S(bx.max - bx.min), m.color, POST_LIT * (0.05f + 1.6f * ring * (0.8f + 0.2f * std::sin(gameClock * 30.f))));
        if (f % 2 == 0) push(out, T(shep::forkNode(f / 2)) * S(glm::vec3{1.6f}), glm::vec3{0.1f}, POST_LIT * (0.3f + 2.f * ring));
    }
    if (level.shepMembrane >= 0 && shepMembraneT < 1.5f) {
        const AABB& bx = level.walls[level.movers[level.shepMembrane].wall].box;
        const float part = shepMembraneT < 0.f ? 0.f : shepMembraneT / 1.5f;
        const glm::vec3 c = (bx.min + bx.max) * 0.5f, sz = bx.max - bx.min;
        if (shepMembraneT >= 0.f || !level.movers[level.shepMembrane].gone)
            for (int sd = -1; sd <= 1; sd += 2)   // two halves, drawing apart
                push(out, T(c + glm::vec3{sd * (sz.x * 0.25f + part * sz.x * 0.5f), 0.f, 0.f}) * S(glm::vec3{sz.x * 0.5f, sz.y, sz.z}), glm::vec3{0.2f, 0.08f, 0.04f},
                     glm::vec3{2.4f, 1.3f, 0.35f} * (0.25f + 0.15f * std::sin(gameClock * 3.f) + (sweep.active() ? 0.8f * ringFlash() : 0.f)) * (1.f - part));
    }
    if (shepWellT >= 0.f && shepWellFront > -2.f && shepWellFront < shep::W_TOP) {
        const float y = shepWellFront;
        push(out, T(glm::vec3{(shep::WX0 + shep::WX1) * 0.5f, y, (shep::WZ_S + shep::WZ_N) * 0.5f}) * S(glm::vec3{shep::WX1 - shep::WX0, 0.6f, shep::WZ_S - shep::WZ_N}), SWEEP_ICE * 0.3f, SWEEP_ICE * 1.6f);
        if (std::fmod(gameClock, 0.05f) < 0.017f) fx.spawnBurst({cosmetic::range(shep::WX0, shep::WX1), y, cosmetic::range(shep::WZ_N, shep::WZ_S)}, SWEEP_ICE, 3, 6.f, 0.5f, -2.f);
    }
    if (shepCharge.on) {
        const glm::vec3 h = shepfight::CHARGE_HALF;
        push(out, T(shepCharge.pos) * RY(shepCharge.from) * S(h * 2.f), glm::vec3{0.6f, 0.58f, 0.53f}, abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f}) * 0.6f);
    }
    if (Enemy* d = shepDog(); d && d->type == EnemyType::DROVER && d->attack == AttackKind::RING_CHARGE && d->telegraphTimer > 0.f) {
        const glm::vec3 fwd{std::sin(d->yaw), 0.f, std::cos(d->yaw)};
        push(out, T(d->position + fwd * 2.5f - glm::vec3{0.f, 1.2f, 0.f}) * S(shepfight::CHARGE_HALF * 1.8f), glm::vec3{0.6f, 0.58f, 0.53f}, abGoldTell(glm::vec3{1.9f, 1.15f, 0.15f}) * (0.6f + 1.4f * d->telegraphProgress()));
    }
}

// Where to go: the sheepdog on its tine (phase 1), the vents (phase 2) with how long they stay open
inline void GameplayState::shepherdMarkers(const glm::mat4& view, const glm::mat4& proj) {
    if (!shepherd() || storyEndT >= 0.f || playerDead) return;
    char buf[64]; float sx, sy;
    if (shepTines < shep::TINES && shepBreakT < 0.f) {
        if (Enemy* d = shepDog(); d && d->state == EnemyState::ACTIVE) {
            const glm::vec3 at = d->position + glm::vec3{0.f, d->height() + 0.8f, 0.f};
            const bool on = projectToScreen(at, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), "%s %dM", d->stats().name, (int)glm::length(at - player.position));
            ui.renderMarker(sx, sy, on, d->shepOpen > 0.f || d->staggered() ? glm::vec3{1.f, 0.75f, 0.2f} : glm::vec3{0.95f, 0.92f, 0.8f}, buf);
        }
        return;
    }
    const int here = level.arenaAt(player.position);
    if (here == 4) {   // the forks, or the way on
        if (shepForks >= shep::FORKS) {
            const glm::vec3 at{shep::HX, 3.f, shep::HZ_N};
            const bool on = projectToScreen(at, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), "ON %dM", (int)glm::length(at - player.position));
            ui.renderMarker(sx, sy, on, glm::vec3{0.4f, 1.f, 0.6f}, buf);
            return;
        }
        for (int f = 0; f < shep::FORKS; ++f) {
            if (shepForkBroken[f]) continue;
            const glm::vec3 at = shep::forkNode(f) + glm::vec3{0.f, 15.f, 0.f};
            const bool on = projectToScreen(at, view, proj, sx, sy);
            if (shepForkRingT > 0.f) snprintf(buf, sizeof(buf), "RINGING %dM", (int)glm::length(at - player.position));
            else snprintf(buf, sizeof(buf), "FORK");
            ui.renderMarker(sx, sy, on, shepForkRingT > 0.f ? glm::vec3{1.f} : glm::vec3{0.6f}, buf, false, shepForkRingT <= 0.f);
        }
        return;
    }
    if (here == 5) {
        const glm::vec3 at{shep::HX, 2.5f, shep::LZ_N};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        snprintf(buf, sizeof(buf), "THE WELL %dM", (int)glm::length(at - player.position));
        ui.renderMarker(sx, sy, on, glm::vec3{0.4f, 1.f, 0.6f}, buf);
        return;
    }
    if (here == 6 && shepUpT < 0.f) {   // the next alcove up, and the top
        int next = -1;
        for (int i = 0; i < shep::WELL_ALCOVES; ++i) if (shep::wellAlcoveY(i) > player.position.y + 1.f) { next = i; break; }
        if (next >= 0) {
            const AABB a = shep::wellAlcove(next);
            const glm::vec3 at = (a.min + a.max) * 0.5f;
            const bool on = projectToScreen(at, view, proj, sx, sy);
            snprintf(buf, sizeof(buf), "ALCOVE %dM", (int)glm::length(at - player.position));
            ui.renderMarker(sx, sy, on, shepWellT >= 0.f ? glm::vec3{1.f, 0.3f, 0.2f} : glm::vec3{1.f, 0.75f, 0.3f}, buf);
        } else {
            const glm::vec3 at{shep::HX, shep::W_TOP, (shep::WZ_S + shep::WZ_N) * 0.5f};
            const bool on = projectToScreen(at, view, proj, sx, sy);
            ui.renderMarker(sx, sy, on, glm::vec3{0.4f, 1.f, 0.6f}, "OUT");
        }
        return;
    }
    if (here != 3 || shepTurnT >= 0.f || shepInT >= 0.f) return;
    for (int v = 0; v < 3; ++v) {
        if (shepVentHp[v] <= 0.f) continue;
        const glm::vec3 at = shep::vent(v) + glm::vec3{0.f, shep::VENT_HY + 1.f, 1.f};
        const bool on = projectToScreen(at, view, proj, sx, sy);
        const int dist = (int)glm::length(at - player.position), pct = (int)std::ceil(100.f * shepVentHp[v] / shepfight::VENT_HP);
        // (the nearest gets the full label; the others just their number, so three don't run together far off)
        int nearest = -1; float nd = 1e9f;
        for (int w = 0; w < 3; ++w) if (shepVentHp[w] > 0.f) { const float d = glm::length(shep::vent(w) - player.position); if (d < nd) { nd = d; nearest = w; } }
        if (v != nearest) snprintf(buf, sizeof(buf), "%d%%", pct);
        else if (shepVentOpenT > 0.f) snprintf(buf, sizeof(buf), "VENT OPEN %dS  %dM  %d%%", (int)std::ceil(shepVentOpenT), dist, pct);
        else snprintf(buf, sizeof(buf), "VENT %dM  %d%%", dist, pct);
        ui.renderMarker(sx, sy, on, shepVentOpenT > 0.f ? glm::vec3{1.f, 0.6f, 0.15f} : glm::vec3{0.6f, 0.6f, 0.6f}, buf, false, v != nearest);
    }
}

// The Shepherd's bar (five segments: one a phase) and the sheepdog's under it
inline void GameplayState::renderShepherdBars() {
    if (!shepherd() || storyEndT >= 0.f) return;
    ui.renderBossBar("THE SHEPHERD", shepherdHealth01(), shepherdDamageT > 0.f, glm::vec3{0.92f, 0.9f, 0.82f}, 62, (int)shepfight::PHASES);
    if (Enemy* d = shepDog(); d && d->state == EnemyState::ACTIVE)
        ui.renderBossBar(d->stats().name, glm::clamp(d->health / std::max(1.f, d->maxHealth), 0.f, 1.f), d->shepOpen > 0.f || d->staggered(), glm::vec3{0.85f, 0.82f, 0.7f}, 102, 1);
}
