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
constexpr float BREAK_LEN = 2.2f, SPAR_FALL = 1.4f;
constexpr float PHASES = 5.f;
// THE TURN
constexpr float TURN_LEN = 5.f, TURN_ANGLE = 2.5f;          // the sky rolling round it (radians, about the line to it)
constexpr float RIDGE_WARN = 6.f, RIDGE_FLASH = 1.5f;        // 6 s: a RECALL statue (8 s) planted on the flash covers it
constexpr float VENT_OPEN = 8.f, VENT_HP = 360.f;   // open after each wave (any range)
// THE RESONATOR
constexpr float HALL_WARN = 5.f, HALL_FLASH = 1.f, HALL_GAP = 5.f;   // its blasts down the hall (the gap grows ragged as forks go)
constexpr float FORK_RING = 6.5f;                                    // after a blast the forks ring this long: their cores open
constexpr float FORK_HP = 260.f;                                     // a core's worth of rounds (HOLDFAST into a ringing fork: all at once)
constexpr float WELL_WARN = 5.f, WELL_SPEED = 30.f, WELL_GAP = 7.f;  // the blasts up the well
}

#include "game/Gameplay_ShepherdComb.h"
#include "game/Gameplay_ShepherdBreak.h"
#include "game/Gameplay_ShepherdFall.h"

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
    for (int f = 0; f < shep::FORKS; ++f) done += (shepForkBroken[f] ? 1.f : glm::clamp(1.f - shepForkHp[f] / shepfight::FORK_HP, 0.f, 1.f)) * 0.6f / shep::FORKS;              // THE RESONATOR: its forks,
    done += 0.4f * glm::clamp(shepWellMaxY / shep::W_OUT, 0.f, 1.f);                                         // and the climb
    done += (float)shepRides / (float)shep::LEDGES;                                                            // THE BREAK: its rides
    done += (director.arena >= 11 || shepEndingT >= 0.f ? 0.8f : 0.8f * (1.f - shepHeadHp / shephead::HP)) + (shepFinT >= shepfall::FIN_DRAIN || shepEndingT >= 0.f ? 0.2f : 0.f);   // THE FALL: the Shepherd itself, then the finisher
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
    shepVentsArmed = shepTurnSeen = shepForksArmed = shepLampsArmed = shepHeadArmed = false;
    enterArena(arena); captureMouse(true); saveStoryProgress();
}

inline void GameplayState::beginShepherd() {
    shepherdmodel::preload();
    ringTextures();
    storyOwned |= 15; storyArmed = storyGrapple = true; holdfastFound = true;
    wakeT = -1; clawT = -1; beltBodies.clear(); player.eyeHeight = 1.7f;
    player.camera.farPlane = ring::FAR_PLANE;
    holdfast.reset(); armSweep();
    resetGap(); resetWake(); crookDragT = 0.f; crookDragUid = -1; crookSeen = crookLeft = true;
    shepTines = std::clamp(director.arena, 0, shep::TINES);
    shepBreakT = -1.f; shepSweepT = 7.f; shepDogUid = -1; shepDogMax = 1.f; shepStrayT = 12.f;
    shepCombReset();
    // THE TURN: the vents keep their damage through a retry (one beat replayed, never the phase)
    if (!shepVentsArmed) { shepVentsArmed = true; for (float& h : shepVentHp) h = shepfight::VENT_HP; }
    shepVents = 0; for (float h : shepVentHp) shepVents += h <= 0.f;
    for(int k=0;k<3;++k){shepOrbitClock[k]=0.f;shepOrbitPin[k]=0.f;}
    shepOrbitFire=3.f;shepOrbitTell=-1.f;
    shepVentOpenT = 0.f; shepRidgeSweepT = 4.f; shepAddT = 6.f; shepSweepPassed = false; shepInT = -1.f; shepVentMsgT = 0.f;
    shepTurnT = -1.f; shepTurnAng = director.arena >= 3 ? shepfight::TURN_ANGLE : 0.f;
    if (director.arena == 3 && !shepTurnSeen) { shepTurnT = 0.f; shepTurnAng = 0.f; }
    if (director.arena == 3) { storySeenSection = 3; shepTines = shep::TINES; }
    if (director.arena >= 4) {   // inside: the ridge behind you (its vents broken, the sky turned)
        storySeenSection = std::max(storySeenSection, director.arena); shepTines = shep::TINES;
        for (float& h : shepVentHp) h = 0.f;
        shepVents = 3; shepTurnAng = shepfight::TURN_ANGLE; shepTurnSeen = true;
    }
    if (!shepForksArmed) { shepForksArmed = true; for (bool& f : shepForkBroken) f = false; for (float& h : shepForkHp) h = shepfight::FORK_HP; shepWellMaxY = 0.f; }
    if (director.arena >= 5) for (bool& f : shepForkBroken) f = true;
    if (director.arena >= 7) shepWellMaxY = shep::W_OUT;
    shepBreakReset();
    shepFallReset();
    if (director.arena >= 11) shepHeadHp = 0.f;
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
    (void)dt;
    for (int f = 0; f < (int)level.shepForks.size(); ++f) level.movers[level.shepForks[f]].gone = shepForkBroken[f / 2];   // (broken forks stay gone through a retry)
    if (level.shepMembrane >= 0) level.movers[level.shepMembrane].gone = shepMembraneT >= 0.f;
}



inline void GameplayState::updateShepherd(float dt) {
    using namespace shepfight;
    const glm::vec3 p = player.position;
    const int at = level.arenaAt(p);
    if (at > director.arena && !director.fighting() && director.phase != WaveDirector::Phase::INTRO && (at <= shepTines || at >= 7)) director.approach(at);
    if (at >= 0 && at > storySeenSection && !playerDead) {
        storySeenSection = at;
        pushBanner(level.arenas[at].name, level.arenas[at].subtitle, SWEEP_ICE * 0.6f, 3.f);
        audio.play("wave", 64, SoundGroup::UI, true);
    }
    if (at >= 0 && at > storyCheckpoint && !director.fighting()) { storyCheckpoint = at; sweepSched.checkpoint(); ui.feed("CHECKPOINT", SWEEP_ICE * 0.6f); saveStoryProgress(); }
    shepMovers(dt);
    sweepSched.timer = std::max(sweepSched.timer, 60.f);   // (its sweeps are its own: the random clock is held off)
    shepComb(dt);   // THE COMB (Gameplay_ShepherdComb.h)
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
    shepBreak(dt);   // THE BREAK (Gameplay_ShepherdBreak.h)
    shepFall(dt);    // THE FALL, the finisher, the end (Gameplay_ShepherdFall.h)
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
    if(shepTines<shep::TINES||level.arenaAt(player.position)!=3||playerDead)return;
    shepVentMsgT=std::max(0.f,shepVentMsgT-dt);
    if(shepTurnT>=0.f){
        if(shepTurnT==0.f){pushBanner("THE ORRERY","THREE MOONS. ONE CAPTIVE ORBIT",POST_LIT*.6f,4.f);audio.play("slam",128,SoundGroup::WORLD,true);}
        shepTurnT+=dt;shepTurnAng=TURN_ANGLE*shephead::smooth(shepTurnT/TURN_LEN);shake(.1f,.015f);
        if(shepTurnT>=TURN_LEN){shepTurnT=-1.f;shepTurnSeen=true;shepRidgeSweepT=5.f;
            ui.toast("BREAK ITS ORBIT","SHOOT THE THREE MOVING MOONS. HOLDFAST PINS ONE IN PLACE. JUMP THE GOLD RINGS",HOLDFAST_STEEL,7.f);
            if(shepVents<3){spawnEnemy(EnemyType::HERDER,{610,0,-3375});spawnEnemy(EnemyType::PORTER,{650,0,-3375});}
        }
        return;
    }
    if(shepInT>=0.f){
        if(shepInT==0.f){sweep.reset();wakeRings.clear();shepOrbitTell=-1.f;pushBanner("THE ORBIT BREAKS","THROUGH ITS EXPOSED HEART",POST_LIT*.6f,3.f);
            for(auto& e:enemies)if(e.targetable()&&e.takeDamage(1e6f))onEnemyKilled(e,StyleSource::ENVIRONMENT);}
        shepInT+=dt;if(shepInT>=2.5f){shepInT=-1.f;storySeenSection=storyCheckpoint=4;enterArena(4);saveStoryProgress();}return;
    }
    for(int k=0;k<3;++k){if(shepOrbitPin[k]>0.f)shepOrbitPin[k]=std::max(0.f,shepOrbitPin[k]-dt);else shepOrbitClock[k]+=dt*(1.f+.25f*shepVents);}
    // A locked ground strike at the player's old position; one warning, one impact.
    if(shepOrbitTell>=0.f){shepOrbitTell+=dt;if(shepOrbitTell>=1.65f){
        shepOrbitTell=-1.f;const auto q=shepOrbitAim;wakeRings.push_back({q,.6f,false});fx.spawnShockwave(q,12.f,POST_LIT);shake(.6f,.04f);audio.play("slam",120,SoundGroup::WORLD,true);
        if(glm::length(glm::vec2{player.position.x-q.x,player.position.z-q.z})<5.f&&player.position.y<q.y+6.f&&invincFrames<=0.f)damagePlayer(24.f,q,.3f,.04f,0.f,{EnemyType::COUNT,DamageKind::SHOCKWAVE});
        for(auto& e:enemies)if(e.targetable()&&glm::length(e.position-q)<7.f&&e.takeDamage(150.f))onEnemyKilled(e,StyleSource::ENVIRONMENT);
    }}else if(!sweep.active()&&(shepOrbitFire-=dt)<=0.f){shepOrbitFire=6.f;shepOrbitTell=0.f;shepOrbitAim=player.position;shepOrbitAim.y=0.f;audio.play("telegraph",128,SoundGroup::WORLD,true);}
    if(sweep.active()&&sweep.passed>0&&!shepSweepPassed){shepSweepPassed=true;shepRidgeSweepT=7.f;}
    if(!sweep.active())shepSweepPassed=false;
    // Finish a local strike before starting a lethal countdown.
    shepRidgeSweepT-=dt;
    if(!sweep.active()&&shepOrbitTell<0.f&&shepRidgeSweepT<=0.f){SweepSpec w;w.kind=SweepSpec::Kind::OUT;w.dir={0,0,1};w.warning=RIDGE_WARN;w.flashLead=RIDGE_FLASH;w.clocked=true;w.range=40;sweep.start(w);++sweepCount;shepRidgeSweepT=30;}
    if(g_devOverlay=="ventkill")for(int k=0;k<3;++k){auto c=shepMoon(k);float range=20;shootVent(c+glm::vec3{0,0,10},{0,0,-1},range,1000);}
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
            ui.feed("THE FORKS RING: SHOOT THEIR CORES", POST_LIT * 0.6f);
            if (!shepForkHint) { shepForkHint = true; pushBanner("THEIR CORES ARE OPEN", "SHOOT THE WHITE CORES. QUICK", POST_LIT * 0.6f, 2.5f); }
            for (int f = 0; f < shep::FORKS; ++f) if (!shepForkBroken[f]) audio.playAt("ab_ting", shep::forkNode(f), 110, SoundGroup::WORLD, true, 0.55f + 0.08f * f);
            int alive = 0; for (const auto& e : enemies) alive += e.alive && e.targetable();
            for (int k = 0; k < 2 && alive < 4; ++k, ++alive) {   // and its flock comes back down the hall behind it
                const glm::vec3 q{shep::HX + (k ? 6.f : -6.f), 0.f, shep::HZ_N + 6.f};
                spawnEnemy(k == 0 ? EnemyType::HERDER : EnemyType::STRAY, q);
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
            if (!shepHallHint) { shepHallHint = true; ui.toast("THE TUNING HALL", "BLASTS COME DOWN THE HALL: GET INTO A SIDE ROOM. AFTER EACH ONE THE FORKS RING AND THEIR CORES OPEN: SHOOT THEM", POST_LIT * 0.4f, 7.f); }
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
                holdfast.cut(); shepStillFork(f);
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
            if (before < 4.f && shepUpT >= 4.f) {   // out onto its surface: THE BREAK
                shepUpT = -1.f;
                storySeenSection = storyCheckpoint = 7;
                enterArena(7); saveStoryProgress();
            }
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

// A fork broken (its core shot out while it rang, or HOLDFAST driven into it)
inline void GameplayState::shepStillFork(int f) {
    using namespace shepfight;
    if(f<0||f>=shep::FORKS||shepForkBroken[f])return;
    shepForkBroken[f] = true;
    shepForkRingT = shepfight::FORK_RING; shepHallSweepT = std::max(shepHallSweepT, 7.f); ++shepForks;
    const glm::vec3 n = shep::forkNode(f);
    fx.spawnBurst(n + glm::vec3{0.f, 6.f, 0.f}, POST_LIT, 70, 14.f, 1.f, 3.f);
    fx.spawnShockwave(n, 10.f, POST_LIT * 0.6f);
    shake(1.f, 0.05f); shepherdDamageT = 1.f;
    audio.playAt("ab_shatter", n, 128, SoundGroup::WORLD, true, 0.6f);
    SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.priority = true; o.unducked = true; o.pitch = 0.45f; o.volume = 0.9f;
    audio.playOpts("slam", o);
    styleSystem.addStyle(150.f, StyleSource::ENVIRONMENT);
    static const char* left[4] = {"THREE LEFT. IT SOUNDS WRONG", "TWO LEFT", "ONE LEFT", "DETUNED"};
    pushBanner("FORK BROKEN", left[std::clamp(shepForks - 1, 0, 3)], POST_LIT * 0.6f, 2.5f);
    if (shepForks >= shep::FORKS) {
        shepMembraneT = 0.f; sweep.reset();
        pushBanner("THE CORE OPENS", "ON, THROUGH IT", POST_LIT * 0.6f, 3.5f);
        fx.spawnBurst({shep::HX, 10.f, shep::HZ_N + 2.f}, glm::vec3{2.4f, 1.3f, 0.35f}, 80, 16.f, 1.2f, 2.f);
    }
}

// A round down a vent's throat (Gameplay_Combat.h's hitscan): only while it is open, and only from close in
inline bool GameplayState::shootVent(glm::vec3 from, glm::vec3 dir, float& distance, float dmg) {
    using namespace shepfight;
    if (shepherd() && shepTines < shep::TINES) return shootComb(from, dir, distance, dmg);   // THE COMB's targets
    if (shepherd() && shootFall(from, dir, distance, dmg)) return true;                      // THE FALL's lamps
    if (!shepherd() || shepTines < shep::TINES || shepTurnT >= 0.f) return false;
    if (level.arenaAt(player.position) == 4)   // THE TUNING HALL: a fork's core, glowing white while it rings
        for (int f = 0; f < shep::FORKS; ++f) {
            if (shepForkBroken[f]) continue;
            const glm::vec3 c = shep::forkNode(f);
            const AABB box{c - glm::vec3{1.5f}, c + glm::vec3{1.5f}};
            const float t = rayBoxHit(from, dir, box);
            if (t <= 0.f || t >= distance) continue;
            distance = t;
            const glm::vec3 at = from + dir * t;
            if (shepForkRingT <= 0.f) {
                fx.spawnHitSparks(at, glm::vec3{0.8f});
                if (shepVentMsgT <= 0.f) { shepVentMsgT = 2.f; ui.feed("SHUT: IT OPENS WHEN THE FORK RINGS, AFTER A BLAST", glm::vec3{0.75f}); }
                return true;
            }
            shepForkHp[f] -= dmg; shepherdDamageT = 0.3f;
            fx.spawnHitSparks(at, POST_LIT);
            styleSystem.addStyle(4.f, StyleSource::ENVIRONMENT);
            if (shepForkHp[f] <= 0.f) shepStillFork(f);
            return true;
        }
    if(level.arenaAt(player.position)!=3||shepInT>=0.f)return false;
    int hit=-1;float best=distance;
    for(int k=0;k<3;++k)if(shepVentHp[k]>0.f){auto c=shepMoon(k);float r=shephead::MOON_RADIUS;
        float t=rayBoxHit(from,dir,{c-glm::vec3{r},c+glm::vec3{r}});if(t>0.f&&t<best){best=t;hit=k;}}
    if(hit<0)return false;
    distance=best;auto c=shepMoon(hit);shepVentHp[hit]=std::max(0.f,shepVentHp[hit]-dmg*(shepOrbitPin[hit]>0.f?1.5f:1.f));
    shepherdDamageT=.3f;fx.spawnHitSparks(from+dir*best,POST_LIT);
    if(shepVentHp[hit]<=0.f){++shepVents;fx.spawnBurst(c,POST_LIT,70,17.f,1.2f,3.f);fx.spawnShockwave(c,15.f,POST_LIT);shake(.8f,.05f);
        audio.play("ab_shatter",128,SoundGroup::WORLD,true);styleSystem.addStyle(180.f,StyleSource::ENVIRONMENT);
        pushBanner("MOON SHATTERED",shepVents==3?"NOTHING HOLDS THE HEART":"THE ORBIT ACCELERATES",POST_LIT*.6f,2.f);
        if(shepVents>=3)shepInT=0.f;
        else {int alive=0;for(const auto& e:enemies)alive+=e.targetable();if(alive<4)spawnEnemy(shepVents==1?EnemyType::TALLY:EnemyType::HERDER,{630,0,-3360});}
    }
    return true;
}

// The cone on the deck, its horn's lamps (dark once its tine is broken), the Drover's block
inline void GameplayState::gatherShepherd(std::vector<BoxInstance>& out) {
    using namespace rig;
    if (!shepherd()) return;
    if (sweep.active() && sweep.spec.kind == SweepSpec::Kind::CONE && sweep.lethal()) {
        const glm::vec3 o = sweep.spec.origin, ax = sweep.spec.dir;
        const float ground = level.arenaAt(player.position)==10 ? shep::FALL_Y : 0.f;
        const float lit = sweep.phase == RingSweep::Phase::FLASH ? 0.5f + 0.5f * std::min(1.f, sweep.t / 0.3f) : 1.f;
        for (int sd = -1; sd <= 1; sd += 2) {   // its two edges, red, all the way down the deck
            const float a = sd * sweep.spec.coneHalf;
            const glm::vec3 d{ax.x * std::cos(a) - ax.z * std::sin(a), 0.f, ax.x * std::sin(a) + ax.z * std::cos(a)};
            for (float r = 14.f; r < 240.f; r += 6.f) {
                const glm::vec3 c = glm::vec3{o.x, ground + 0.06f, o.z} + d * (r + 2.5f);
                push(out, T(c) * RY(std::atan2(d.x, d.z)) * S(glm::vec3{0.35f, 0.05f, 4.6f}), glm::vec3{0.f}, SWEEP_RED * 2.2f * lit);
            }
        }
        // ...and red chevrons down its middle, toward you
        for (float r = 20.f; r < 240.f; r += 12.f) {
            const glm::vec3 c = glm::vec3{o.x, ground + 0.05f, o.z} + ax * r;
            push(out, T(c) * RY(std::atan2(ax.x, ax.z)) * S(glm::vec3{std::tan(sweep.spec.coneHalf) * r * 2.f, 0.03f, 0.5f}), glm::vec3{0.f}, SWEEP_RED * 0.5f * lit);
        }
        if (sweep.phase != RingSweep::Phase::FLASH && sweep.passed < sweep.waves()) {   // the front: an arc of lifted ice racing down
            const float f = sweep.front(0);
            if (f > 12.f)
                for (int i = -6; i <= 6; ++i) {
                    const float a = sweep.spec.coneHalf * i / 6.f;
                    const glm::vec3 d{ax.x * std::cos(a) - ax.z * std::sin(a), 0.f, ax.x * std::sin(a) + ax.z * std::cos(a)};
                    const glm::vec3 c = glm::vec3{o.x, ground + 1.6f, o.z} + d * f;
                    push(out, T(c) * RY(std::atan2(d.x, d.z)) * S(glm::vec3{f * sweep.spec.coneHalf / 3.f + 0.5f, 3.2f, 0.6f}), SWEEP_ICE * 0.4f, SWEEP_ICE * 1.4f);
                }
        }
    }
    gatherComb(out);   // THE COMB
    gatherBreak(out);  // THE BREAK
    gatherFall(out);   // THE FALL
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
    if(level.arenaAt(player.position)==3){
        for(int j=0;j<120;++j){float a=6.2831853f*j/120.f;push(out,T(glm::vec3{630.f+36.f*std::cos(a),.08f,-3398.f+30.f*std::sin(a)})*RY(-a)*S(glm::vec3{.16f,.08f,1.8f}),glm::vec3{.05f},POST_LIT*.35f);}
        for(int k=0;k<3;++k)if(shepVentHp[k]>0.f){auto c=shepMoon(k);const auto glow=shepOrbitPin[k]>0.f?HOLDFAST_STEEL:POST_LIT;
            BoxInstance m;m.mesh=(float)headmesh::orb();m.model=T(c)*S(glm::vec3{6.8f});m.color={.12f,.18f,.24f};m.emissive=glow*.5f;out.push_back(m);
            for(int j=0;j<40;++j){float a=6.2831853f*j/40.f;push(out,T(c)*RZ(.5f+k)*T(glm::vec3{std::cos(a)*4.8f,std::sin(a)*4.8f,0})*RZ(a)*S(glm::vec3{.2f,.77f,.2f}),glm::vec3{.1f},glow);}
            if(shepOrbitPin[k]>0.f)push(out,T(glm::vec3{c.x,c.y*.5f,c.z})*S(glm::vec3{.2f,c.y,.2f}),HOLDFAST_STEEL,HOLDFAST_STEEL);
        }
        if(shepOrbitTell>=0.f){for(int j=0;j<40;++j){float a=6.2831853f*j/40.f;push(out,T(shepOrbitAim+glm::vec3{std::cos(a)*5.f,.1f,std::sin(a)*5.f})*RY(-a)*S(glm::vec3{.25f,.1f,.8f}),glm::vec3{.1f},SWEEP_RED*2.f);}
            push(out,T(shepOrbitAim+glm::vec3{0,18.f*(1.f-shepOrbitTell/1.65f),0})*S(glm::vec3{4.f}),glm::vec3{.1f},POST_LIT);}
    }
    // THE RESONATOR: its forks (ringing white after a blast), the core's membrane, the blast in the well
    for (int f = 0; f < (int)level.shepForks.size(); ++f) {
        const Mover& m = level.movers[level.shepForks[f]];
        if (m.gone) continue;
        const AABB& bx = level.walls[m.wall].box;
        const float ring = shepForkRingT / shepfight::FORK_RING, shiver = ring > 0.f ? 0.05f * std::sin(gameClock * 90.f + f) : 0.f;
        push(out, T((bx.min + bx.max) * 0.5f + glm::vec3{shiver, 0.f, 0.f}) * S(bx.max - bx.min), m.color, POST_LIT * (0.05f + 1.6f * ring * (0.8f + 0.2f * std::sin(gameClock * 30.f))));
        if (f % 2 == 0) push(out, T(shep::forkNode(f / 2)) * RY(gameClock * (0.5f + 4.f * ring)) * S(glm::vec3{1.6f + 1.2f * ring}), glm::vec3{0.1f}, POST_LIT * (0.15f + 3.f * ring * (0.75f + 0.25f * std::sin(gameClock * 12.f))));
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
}

// Where to go: the sheepdog on its tine (phase 1), the vents (phase 2) with how long they stay open
inline void GameplayState::shepherdMarkers(const glm::mat4& view, const glm::mat4& proj) {
    if (!shepherd() || storyEndT >= 0.f || playerDead) return;
    char buf[64]; float sx, sy;
    if (shepTines < shep::TINES) { combMarkers(view, proj); return; }
    if (shepLedge() >= 0) { breakMarkers(view, proj); return; }   // THE BREAK
    if (level.arenaAt(player.position) >= 10) { fallMarkers(view, proj); return; }   // THE FALL
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
            const glm::vec3 at = shep::forkNode(f) + glm::vec3{0.f, 2.2f, 0.f};
            const bool on = projectToScreen(at, view, proj, sx, sy);
            const int pct = (int)std::ceil(100.f * shepForkHp[f] / shepfight::FORK_HP);
            int nearest = -1; float nd = 1e9f;
            for (int g = 0; g < shep::FORKS; ++g) if (!shepForkBroken[g]) { const float d = glm::length(shep::forkNode(g) - player.position); if (d < nd) { nd = d; nearest = g; } }
            if (f != nearest) snprintf(buf, sizeof(buf), "%d%%", pct);
            else if (shepForkRingT > 0.f) snprintf(buf, sizeof(buf), "SHOOT ITS CORE %dS  %d%%", (int)std::ceil(shepForkRingT), pct);
            else snprintf(buf, sizeof(buf), "FORK %d%%", pct);
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
    for(int k=0;k<3;++k)if(shepVentHp[k]>0.f){auto at=shepMoon(k)+glm::vec3{0,5,0};bool on=projectToScreen(at,view,proj,sx,sy);
        snprintf(buf,sizeof(buf),"MOON %d  %d%%%s",k+1,(int)std::ceil(100.f*shepVentHp[k]/shepfight::VENT_HP),shepOrbitPin[k]>0.f?"  PINNED":"");
        ui.renderMarker(sx,sy,on,shepOrbitPin[k]>0.f?HOLDFAST_STEEL:POST_LIT*.6f,buf);}

}

// The Shepherd's bar (five segments: one a phase) and the sheepdog's under it
inline void GameplayState::renderShepherdBars() {
    if (!shepherd() || storyEndT >= 0.f || shepEndingT >= 0.f) return;
    if(level.arenaAt(player.position)==10){ui.renderBossBar("THE SHEPHERD",shepHeadThird(),shepherdDamageT>0.f,HOLDFAST_STEEL,62,3);return;}
    ui.renderBossBar("THE SHEPHERD", shepherdHealth01(), shepherdDamageT > 0.f, glm::vec3{0.92f, 0.9f, 0.82f}, 62, (int)shepfight::PHASES);
    if (Enemy* d = shepDog(); d && d->state == EnemyState::ACTIVE)
        ui.renderBossBar(d->stats().name, glm::clamp(d->health / std::max(1.f, d->maxHealth), 0.f, 1.f), d->shepOpen > 0.f || d->staggered(), glm::vec3{0.85f, 0.82f, 0.7f}, 102, 1);
}
