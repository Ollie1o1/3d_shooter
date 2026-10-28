#pragma once
#include "core/GameState.h"
#include "core/MouseCapture.h"
#include "core/Balance.h"
#include "player/Player.h"
#include "core/Camera.h"
#include "core/FrameTimer.h"
#include "core/Mesh.h"
#include "core/ShaderProgram.h"
#include "player/StyleSystem.h"
#include "ui/UIRenderer.h"
#include "enemies/Enemy.h"
#include "audio/EnemyVoice.h"
#include "bosses/warden/WardenHazards.h"
#include "bosses/leviathan/LeviathanHazards.h"
#include "bosses/absolute/AbsoluteHazards.h"
#include "enemies/RelicHazards.h"
#include "render/EnemyModel.h"
#include "render/BoxRenderer.h"
#include "player/Projectile.h"
#include "player/GrappleHook.h"
#include "world/Level.h"
#include "world/LevelGauntlet.h"
#include "world/LevelAct2.h"
#include "game/WaveDirector.h"
#include "render/PostProcess.h"
#include "audio/AudioSystem.h"
#include "render/ViewModel.h"
#include "render/CameraFeel.h"
#include "world/Interactable.h"
#include "world/Stuck.h"
#include "core/Settings.h"
#include "ui/SettingsMenu.h"
#include "render/TextureGen.h"
#include "player/Weapons.h"
#include "game/Progression.h"
#include "game/DevFlags.h"          // the command line's and the dev tools' globals (g_*)
#include "ui/LeaderboardView.h"
#include "game/Ghost.h"
#include "core/Gamepad.h"
#include "core/MouseFilter.h"
#include "render/WorldMesh.h"
#include "render/NeonLights.h"
#include "render/Effects.h"
#include "render/PropDraw.h"
#include "render/LightBake.h"
#include "world/ArenaShifts.h"
#include "game/Score.h"
#include "game/EndlessWaves.h"
#include "game/RunLog.h"
#include "bosses/sovereign/SovereignHazards.h"
#include "bosses/penitent/PenitentHazards.h"
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include "core/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <deque>
#include <algorithm>
#include <array>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <string>
#include <cstdio>

// =============================================================================
// GameplayState — the live game: physics, combat, rendering, progression.
//
// FLOW: WaveDirector (WaveDirector.h) decides when waves start and which
// enemies to spawn where; this class owns the enemies, turns their per-tick
// events into projectiles/damage/effects, opens and closes doors, and keeps
// the player inside the arena they're fighting in (and under its ceiling).
//
// MODES (GameMode, GameState.h): ARENA and ENDLESS play buildLevel() (Level.h:
// four arenas, the Sanctum); ACT II plays LevelAct2.h; FAST plays the
// Gauntlet (LevelGauntlet.h: a countdown, splits, a finish beacon); BOSS runs
// drop straight into one boss's fight (startBossRun, Progression.h's table).
//
// The member functions are split by topic into the Gameplay_*.h files listed
// at the bottom of this one: the run (Flow), the 60 Hz tick (Tick), damage
// both ways (Combat), input and menus (Menus), drawing (Render, HUD), dev
// tools (Dev), and one file per boss fight.
//
// KEY EXTENSION POINTS:
//   ADD A WEAPON:  a row in Weapons.h, a model in GunKit.h / ViewModel.h, a
//                  sound name in fireWeapon(). Damage goes through hurtEnemy().
//   ADD AN ENEMY:  a row in EnemyTypes.h (stats), a think*() in Enemy.h, a box
//                  rig in EnemyModel.h, then list it in a wave (Level.h).
//   ADD A BOSS:    <Boss>State.h (its state, constants, events: inherited by
//                  Enemy / EnemyEvents), Enemy<Boss>.h (its mind), and
//                  Gameplay_<Boss>.h (its fight: hazards, events, drawing).
//   CHANGE A MAP:  Level.h / LevelAct2.h / LevelGauntlet.h. Walls render and
//                  collide automatically; movers are walls that LevelData moves.
// =============================================================================
static constexpr int   SCREEN_W   = 1280;
static constexpr int   SCREEN_H   = 720;
static constexpr float PHYSICS_HZ = 60.f;
static constexpr float PHYSICS_DT = 1.f / PHYSICS_HZ;

class GameplayState : public GameState {
public:
    std::function<void()> onReturnToMenu;
    std::function<void()> onQuit;

    GameSettings* settings = nullptr;   // injected by main — may be null (safe)
    GameMode      mode = GameMode::ARENA;
    bool fast() const { return mode == GameMode::FAST; }
    bool roomAttack() const { return fast() && g_roomAttack >= 0; }   // FAST's ROOM ATTACK: one room, for its medal
    bool act2() const { return mode == GameMode::ACT2; }
    bool arenaSolo() const { return (mode == GameMode::ARENA || act2()) && g_arenaSolo >= 0; }   // the ARENA page: one arena on its own
    bool endless() const { return mode == GameMode::ENDLESS; }
    Board boardFor() const { return fast() ? Board::FAST : endless() ? Board::ENDLESS : Board::ARENA; }

    // ---- ENDLESS (Gameplay_Flow.h) ----
    EndlessWaves gen;
    int   endlessArena = 3;        // ENDLESS runs in the Core
    int   endlessBaseAlive = 11;
    int   wavesCleared = 0;
    void  setupEndless();          // fresh waves for a new run
    void  feedEndless();           // keep a wave queued ahead of the director
    float endlessToughness() const;

    // ---- the run's score (Score.h) ----
    // Retrying an arena rebuilds the style system, so what it had counted goes here first
    float bankedStyle = 0.f, bankedDamage = 0.f;
    RunScore runScore() const;
    Leaderboard::Entry runEntry() const;
    const DifficultyTuning& tune() const { return difficulty(settings ? settings->difficulty : DIFFICULTY_DEFAULT); }

    Player           player{{0.f,0.f,24.f}};
    StyleSystem      styleSystem;
    UIRenderer       ui{SCREEN_W,SCREEN_H};
    ProjectileSystem projSystem;
    GrappleHook      grapple;
    LevelData        level;
    WaveDirector     director;
    AudioSystem&     audio;
    PostProcess      postProcess{display::renderW(), display::renderH()};
    SettingsMenu     settingsMenu{SCREEN_W, SCREEN_H};
    Progression      prog;
    Records          records;
    Leaderboard      board;
    Leaderboard      worldBoard;     // web: the shared board, when the site's API answers
    bool             worldLoaded = false;
    float            worldPoll = 0.f;
    GhostRun         ghost;      // FAST: the best run, played back
    GhostRun         ghostRec;   // FAST: this run, being recorded
    MouseFilter      mouseFilter;

    ShaderProgram  worldShader;
    ShaderProgram  skyboxShader;
    BoxRenderer    boxRenderer;

    GLuint      whiteTex = 0;
    GLuint      worldTex[TEX_COUNT] = {};
    static GLuint* sharedWorldTex();   // made once, kept for every run (Gameplay_Flow.h)
    // The level's baked light (LightBake.h): loaded and uploaded once a level, kept (Gameplay_Flow.h)
    struct SharedBake { bool tried = false; GLuint tex = 0; lightbake::Bake bake; };
    static SharedBake& sharedBake(const std::string& level, const LevelData& L);
    const SharedBake* bake = nullptr;
    void applyBake();   // its regions to the world shader (once: they stay set)
    int         textureQuality = -1;   // graphics quality the world textures are filtered for
    void applyTextureQuality();
    WorldMeshes world;
    // The world's light from its neon and lava (NeonLights.h), and the soft
    // shadows on the ground under things (contact.frag; enemies' each frame)
    ShaderProgram          contactShader;
    ContactMesh            movingShadows;
    std::vector<NeonLight> neonLights;
    NeonSet                neonSet;
    void applyNeon(ShaderProgram& sh, bool on);
    void drawContactShadows(const glm::mat4& view, const glm::mat4& proj, const Theme& th, glm::vec3 camPos);

    std::vector<Enemy> enemies;
    SpatialGrid        spatialGrid;   // built once; door boxes move only in Y, movers are tested separately

    // ---- Weapons (see Weapons.h) ----
    std::array<WeaponState, WEAPON_COUNT> weapons;
    int   activeWeapon   = 0;
    int   pendingWeapon  = -1;
    float weaponSwitchTimer = 0.f;
    float aim         = 0.f;    // 0 hip .. 1 aimed (rifles)
    float aimFullAt   = -1.f;   // gameClock when aim last reached full (quickscope window)
    // G key: Grenades — max 2, one returned per 2 kills
    int   grenadeCount   = 2;
    int   grenadeMax     = 2;
    float grenadeTimer   = 0.f;
    int   killsThisCycle = 0;

    float recoilPitch = 0.f;

    // Run stats
    int   totalKills  = 0;
    int   totalShots  = 0;
    int   totalHits   = 0;
    int   deaths      = 0;
    float elapsedTime = 0.f;
    float peakStyle   = 0.f;
    bool  victory      = false;
    float victoryDelay = -1.f;   // counts down after the boss dies, then shows the screen
    bool  newRecord    = false;
    // Only a full run counts for records and the leaderboard: started at the
    // first arena, not in god mode, not from the dev level select
    bool  ranked       = true;
    bool  soloDone = false;        // the ARENA page's one arena: cleared, its time kept (arenaSoloDone)
    bool  act2FromStart = false;   // an ACT II run from its first arena: its times count (act2RunRecords)
    bool  nameEntry    = false;  // victory screen: typing a name for the leaderboard
    std::string nameBuf;
    int   boardPlace   = -1;     // where the saved run landed (highlighted)

    // FAST mode
    float countdown  = 0.f;      // 3-2-1 before the clock starts
    bool  act2Falling = false;   // ACT II: still dropping down the shaft (the fight waits)
    bool  finishOpen = false;
    std::vector<float> splits;   // clock at each section clear

    // Title cards, shown one after another
    struct Banner { std::string title, subtitle; glm::vec3 color; float time, duration; };
    std::deque<Banner> banners;


    int   dashCharges       = 2;
    float dashCooldown      = 0.f;
    float dashMomentumTimer = 0.f;
    static constexpr float GRAPPLE_COOLDOWN = 1.5f;   // from the moment the rope lets go (arrival, release, or broken); arenas only, never FAST
    float grappleCooldown   = 0.f;
    bool  wasGrappling      = false;
    int   jumpsRemaining = 2;
    bool  prevOnGround   = false;
    bool  slamming       = false;
    float invincFrames   = 0.f;

    camfeel::CameraFeel camFeel;   // shake, punches, the landing dip, strafe lean, the stride (CameraFeel.h)
    // Hit sounds this tick, played once at its end (ten pellets: one sound, not ten)
    int  hitSoundCount = 0;
    bool hitSoundCrit  = false;
    bool hitSoundArmor = false;
    static constexpr float HEAVY_LAND = 15.f;   // fall speed (m/s) for the heavy landing
    float landFall   = 0.f;   // the fall speed of the last landing
    bool  slidePrev  = false;
    float slideDustT = 0.f;
    // Screen effects (bloom_composite.frag): a dash's rush, low health's drain and heartbeat
    float dashFx  = 0.f;   // 1 on a dash, gone in 0.35 s
    float speedFx = 0.f;   // what the shader gets: the dash, or just going very fast (a grapple, a tube)
    float lowFx   = 0.f;   // 0 above 35 % health .. 1 at 8 %, eased
    float heartT  = 0.f;   // seconds since the last heartbeat
    float heartPulse = 0.f;
    float windFx  = 0.f;   // 0..1 the air rushing past (AudioSystem::setWind)
    float embeddedT = 0.f;          // how long the body has been inside something solid
    glm::vec3 safeGround{0.f};      // the last firm ground stood on (keepPlayerInZone: unstuck)
    bool  hasSafeGround = false;
    static constexpr float SLOW_TIME = 0.65f;   // a wave's last kill: real seconds of slow motion
    float slowT = 0.f;                          // counting down; time runs at timeScale() meanwhile
    float slowLen = SLOW_TIME, slowLow = 0.25f; // this slow-down's length and how slow it starts (OVERDRIVE's is shorter)
    float timeScale() const {                   // a quarter speed, easing back to full
        if (slowT <= 0.f) return 1.f;
        float u = 1.f - slowT / slowLen;
        return slowLow + (1.f - slowLow) * u * u * (3.f - 2.f * u);
    }
    void slowMo(float len, float low) { if (slowT > 0.f && slowLow <= low) return; slowT = slowLen = len; slowLow = low; }
    float ropeFx = 0.f;                         // 0..1 the grapple's rope singing (AudioSystem::setRope)
    // Hanging off the grapple (GrappleHook::latched): how long, and where you're held
    float hangT = 0.f; glm::vec3 hangPos{0.f}; bool hangPrev = false;
    glm::vec3 hangNormal() const;               // the face of the surface you're hanging on
    glm::vec2 lookDelta{0.f};   // degrees of mouse / stick look this frame (yaw, pitch): the gun's sway

    float muzzleFlashTimer = 0.f;
    glm::vec3 muzzleFlashPos{0.f};
    float explosionFlashTimer = 0.f;
    glm::vec3 explosionFlashPos{0.f};
    float ceilingFxTimer = 0.f;

    ArenaShifts   shifts;      // arenas that change as the fight goes on (ArenaShifts.h)
    bool          pulseWarnCued = false;
    Effects       fx;          // particles, debris, decals, tracers, rings (Effects.h)
    ShaderProgram tracerShader;
    GLuint        tracerVAO = 0, tracerVBO = 0;
    ShaderProgram waterShader;
    GLuint        waterVAO = 0, waterVBO = 0;
    static constexpr int MAX_WATER = 64;   // volumes drawn per frame
    float         skimTimer = 0.f;         // spray and hiss while a slide skims water
    ShaderProgram particleShader;

    ViewModel viewModel;
    std::vector<Interactable> interactables;
    bool  nearInteractable = false;
    bool  grappleTargetInSight = false;

    float fovKick        = 0.f;
    float playerXZSpeed  = 0.f;
    float peakFallSpeed  = 0.f;

    bool  paused        = false;
    bool  pauseSettings = false;   // the settings page is open inside the pause menu
    int   pauseSelected = 0;
    bool  armoryOpen    = false;
    int   armoryW = 0, armoryS = 0;
    bool  playerDead = false;
    float deadTimer  = 0.f;

    int hitStopFrames = 0;

    GLuint decalVAO = 0, decalVBO = 0;

    GLuint    particleVAO = 0, particleVBO = 0;

    // Drops: health orbs (common, small heal, magnetic), health potions (rare,
    // big heal, only taken when hurt) and XP shards (rare, bonus XP)
    enum class PickupKind { ORB, POTION, XP };
    struct Pickup { glm::vec3 pos, vel; float life; PickupKind kind; float floorY; glm::vec3 prev{0.f}; };
    // A boss sheds a health potion every fifth of its bar you take off it (four a bar): health earned by hitting it.
    // It lands a few metres from where you last stood firm, so it's always somewhere you can reach
    static constexpr float BOSS_POTION_EVERY = 0.2f;
    static constexpr int   BOSS_POTIONS = 4;
    void shedBossPotions(Enemy& e);
    bool potionSpot(glm::vec3& out);
    std::vector<Pickup> pickups;

    // src: whose blast it is, for style (a Mite that reaches you hurts its friends: FRIENDLY)
    struct Blast { glm::vec3 pos; float radius, damage; float playerRadius, playerDamage;
                   StyleSource src = StyleSource::EXPLOSIVE; EnemyType by = EnemyType::COUNT; };   // by: whose it was, if it can hurt you (the run log)

    // Friendly fire: a Juggernaut's siege shell kills the small fry it hits;
    // a Brute's slam kills Rippers and Mites around it and leaves a Husk on
    // 15; a Juggernaut's smash ring is smaller and weaker
    static constexpr float FRIENDLY_SHELL_DAMAGE    = 120.f;
    static constexpr float FRIENDLY_SLAM_BRUTE      = 45.f;
    static constexpr float FRIENDLY_SLAM_JUGGERNAUT = 35.f;
    std::vector<Blast> pendingBlasts;

    float padCooldown  = 0.f;
    float hazardTick   = 0.f;
    float gameClock    = 0.f;     // drives decor animation
    float moverClock   = 0.f;     // drives moving platforms (fixed-step)

    static constexpr int MAX_POINT_LIGHTS = 4;
    glm::vec3 pointLightPos[MAX_POINT_LIGHTS];
    glm::vec3 pointLightColor[MAX_POINT_LIGHTS];

    Uint64 prevTicks   = 0;
    Uint64 freq        = 0;
    double accumulator = 0.0;
    glm::vec3 prevCamPos{0.f};

    int   fpsFrameCount = 0;
    float fpsTimer      = 0.f;

    bool parryPrev        = false;
    float punchCooldown   = 0.f;
    static constexpr float PARRY_COOLDOWN = 1.0f;   // a punch / parry at most once a second: no spamming it
    bool prevDashKey      = false;
    int  boostPrev        = -1;     // booster the player was in last tick
    int   spaceArena = -1;          // the arena whose reverb is playing
    float shieldClankCd = 0.f;      // throttles the clank of bullets on a shield
    bool prevJumpKey      = false;
    // Event-driven click flags — set in handleEvent, consumed once in physicsTick.
    bool pendingFire      = false;
    bool pendingGrenade   = false;
    bool pendingGrapple   = false;

    GameplayState(AudioSystem& aud, GameSettings* s = nullptr, GameMode m = GameMode::ARENA);

    ~GameplayState();

    void captureMouse(bool on);

    // =========================================================================
    // Run / arena lifecycle
    // =========================================================================

    // Put the player at the start of arena `a` with the doors set as if they
    // had just walked in. Used for a fresh run, a retry after death, and --arena.
    void enterArena(int a);

    void resetPlayer(glm::vec3 start);

    // Restart the room / arena you're in from its checkpoint (pause menu,
    // Backspace). Not a death; the run clock keeps going.
    void restartHere();

    void retryArena();

    void newRun();
    void beginAct2();

    void lockDoor(int d, bool locked) { if (d >= 0) level.doors[d].locked = locked; }

    void pushBanner(const std::string& title, const std::string& sub, glm::vec3 col, float dur = 3.f);

    // Turn director events into title cards, doors and rewards.
    void handleDirectorEvents();

    // FAST mode: note the clock at a section clear and compare with the best run
    int  recordSplit(int section);   // -1 ahead of the best run's split, 1 behind, 0 no best yet

    void finishRun();
    void recordCue();

    // =========================================================================
    // The bosses. Each one's fight is in its own Gameplay_<Boss>.h (and its
    // mind in Enemy<Boss>.h); what the game keeps for it is here.
    // =========================================================================
    // ---- shared by every boss fight -----------------------------------------------
    // Dusk falls as a boss loses health: the sky, sun and light dim toward night (the tells' own glow
    // stays bright); it eases back once no boss is alive
    float bossDusk = 0.f;
    static constexpr float BOSS_DUSK_MAX = 0.7f;   // how far toward night at 0 hp
    static Theme nightfall(const Theme& t);
    void updateBossDusk(float dt);

    // BOSS runs (g_bossRun): straight into one boss's fight; its fall ends the run
    float bossStartAt = -1.f;   // elapsedTime as the boss's fight began
    float bossKillT = 0.f;      // how long it took
    bool  bossDone = false;
    bool  bossPost = false, bossPosted = false;   // BOSS: the kill may go on the world board (asked for a name); it went
    int   bossWaveOf(int a) const;
    void  startBossRun();
    void  bossRunDone();

    // ---- THE SOVEREIGN (Gameplay_Sovereign.h) -------------------------------------
    // Phase two of the Sovereign fight: the eclipse turns to blood and the
    // platforms orbiting the seal pick up speed
    float rageBlend = 0.f;   // 0..1, eases in once he enrages
    bool  sanctumRaged = false;
    static Theme bloodEclipse(const Theme& t);
    void updateSanctumPhase(float dt);
    // The Sovereign's blades, eruptions and phantoms, and his last stand: under
    // 20% the Sanctum's edges and high ground burn (Gameplay_Sovereign.h)
    SovereignHazards sov;
    bool  lastStand = false;
    float lastStandT = 0.f;      // seconds since it began (the edge warns first)
    int   deflectHints = 0;
    static constexpr float LAST_STAND_WARN = 2.5f;
    void onSovereignEvents(const Enemy& e, const EnemyEvents& ev);
    void onPacemakerEvents(const Enemy& e, const EnemyEvents& ev);   // (Gameplay_Pacemaker.h)
    void updateSovereign(float dt);
    void gatherSovereignBoxes(std::vector<BoxInstance>& out);
    glm::vec3 sanctumCentre() const;
    // --overlay poseN (screenshots): hold a SOVEREIGN in one pose, facing the camera.
    // 0 idle, 1 dash wind-up, 2 dashing, 3 sweep wind-up, 4 mid-sweep,
    // 5 cleave wind-up, 6 mid-cleave, 7 leaping, 8 broken, 9 enraged, 10 shadow step
    // wind-up, 11 judgment, 12 whirlwind wind-up, 13 whirling, 14 thrust wind-up,
    // 15 thrusting, 16 sending phantoms
    void devPose(Enemy& e);

    // ---- THE WARDEN (Gameplay_Warden.h) -------------------------------------------
    // Its lance, seekers and steam; its conduits' clock
    WardenHazards ward;
    ConduitClock  conduitClock;
    int  wardenUid = -1;            // the Warden whose conduits were last made whole
    void onWardenEvents(Enemy& e, const EnemyEvents& ev);
    void updateWarden(float dt);
    void cutConduit(int i);
    void gatherWardenBoxes(std::vector<BoxInstance>& out);
    Enemy* warden();
    bool wardenAlive() { return warden() != nullptr; }
    int  wardenPhase() { Enemy* w = warden(); return w ? w->wardenPhase : 0; }

    // ---- THE PENITENT and the Descent's cage (Gameplay_Penitent.h) ----------------
    // Its hazards, its chains' anchors
    PenitentHazards pen;
    AnchorRip rip;                  // hooking an anchor with the grapple (PenitentHazards.h)
    bool  downHinted = false;       // "BACK TO THE CAGE" shown
    bool  scourgeAnnounced = false;
    std::vector<LevelData::ChainAnchor> anchorsBuilt;   // as built, for a retry
    std::vector<AABB> anchorBoxes;
    void onPenitentEvents(Enemy& e, const EnemyEvents& ev);
    void updatePenitent(float dt);
    void breakAnchor(int i, bool ripped);
    bool hitAnchor(glm::vec3 origin, glm::vec3 dir, float wallT, float dmg);
    float hitChain(glm::vec3 o, glm::vec3 d, float maxT, float dmg);
    void gatherPenitentBoxes(std::vector<BoxInstance>& out);
    // --overlay penitentN: hold THE PENITENT still, facing the camera.
    // 0 kneeling, 1 low sweep wind-up, 2 high sweep wind-up, 3 risen, 4 scourging
    void devPenitentPose(Enemy& e);
    // The Descent's cage
    float dryCd = 0.f;          // the dry click, at most every 0.3 s
    bool boardHinted = false;   // "STEP ONTO THE CAGE" shown for this stop
    bool liftRiding  = false;   // a ride is under way (its arrival drops health on the cage)
    void updateLift(float dt);

    // ---- THE RELIQUARY (Gameplay_Reliquary.h) -------------------------------------
    // Souls and wires, the last relic you
    // stood on, the snare, the drift
    RelicHazards relic;
    int   lastChunk = 0;
    float snareTimer = 0.f;
    bool  driftWasGliding = false;
    void updateReliquary(float dt);
    void onReliquaryEvents(Enemy& e, const EnemyEvents& ev);
    void releaseRevenantSoul(const Enemy& e, bool inVoid);
    bool catchSoulWithPunch();
    void gatherReliquaryBoxes(std::vector<BoxInstance>& out);

    // ---- THE LEVIATHAN (Gameplay_Leviathan.h) -------------------------------------
    // What it does to the ring, the flood
    LeviathanHazards lev;
    bool  beachHinted = false;      // "BEACHED" shown
    Enemy levCorpse{EnemyType::LEVIATHAN, glm::vec3{0.f}};   // dead: it sinks into its pool (drawn LV_SINK s)
    float levSinkT = 1e9f;
    static constexpr float LV_SINK = 3.f;
    static constexpr float FLOOD_DEPTH = 0.6f;   // the Maw's flood over the ring in its last phase
    Enemy* leviathan();
    void onLeviathanEvents(Enemy& e, const EnemyEvents& ev);
    void updateLeviathan(float dt);
    void throatHit(Enemy& e, float dmg);
    void steerParriedOrbs(float dt);
    void gatherLeviathanBoxes(std::vector<BoxInstance>& out);
    // The ascension: the Leviathan dies, the act's card, a white-out, the Zenith
    float ascendT = -1.f;        // < 0: not rising; else seconds into it
    float whiteFade = 0.f;       // the white-out fading off as the Zenith appears
    static constexpr float ASCEND_SINK = 3.f, ASCEND_CARD = 2.f, ASCEND_WHITE = 1.f;
    void updateAscension(float dt);   // (Gameplay_Flow.h)

    // ---- THE ABSOLUTE and MORNINGSTAR (Gameplay_Absolute.h, Gameplay_Morning.h) ---
    // What he leaves in the Zenith, his afterimages, his lines, his death
    AbsoluteHazards abh;
    struct AbGhost { Enemy pose; float t; };
    std::vector<AbGhost> abGhosts;
    static constexpr float AB_GHOST_LIFE = 0.35f, AB_DEATH = 3.f;
    std::vector<AABB> zenPillarBoxes;               // the standing pillars, for his mind (fillZenithWorld)
    std::array<float, 8> pillarFallT{};             // a shattered pillar's pieces falling (< 0: none)
    glm::vec3 abSkyMark{0.f}; float abSkyMarkT = -1.f;   // SKYFALL's circle on the floor (< 0: none)
    glm::vec3 abCrackAt{0.f}; float abCrackT = -1.f;     // an axe kick's crack
    SoundHandle abWindH = 0, abFlightH = 0, abBeamH = 0;   // his wind-up, his flight's wind, a beam's crackle (stopped early when cut short)
    float abLastPressT = -1e9f, abWhiteHitT = -1e9f;        // the sword's last parry press, the last white blow that hit you (EARLY / LATE)
    std::array<float, 6> shardFallT{};              // a shard dropping away (< 0: none)
    float abLineT = 0.f; int abLineShown = -1;      // his line on screen
    float abHintT = 0.f;                            // the sword's hint as he arrives
    float abDeathT = 1e9f;
    // The sky over the fight (skybox.frag): the eclipse flares at his entrance and his phases; a shaft
    // of light from it as he draws his blade; on his death the eclipse slides off the sun (the dawn)
    float eclipseFlare = 0.f, abShaftT = -1.f, abDawnT = -1.f;
    static constexpr float AB_SHAFT = 1.2f, AB_DAWN_FROM = 1.6f, AB_DAWN_TIME = 6.f;
    float dawn01() const { return abDawnT < 0.f ? 0.f : glm::smoothstep(AB_DAWN_FROM, AB_DAWN_FROM + AB_DAWN_TIME, abDawnT); }
    static Theme dawnOf(const Theme& t);
    // MORNINGSTAR: the swallow (the eclipse grows over the whole sky, then its ring of fire settles on
    // the horizon round the floor), seconds since it began (< 0: not yet); TOTALITY, since the ring
    // collapsed (the light going out); a death to him retries from his form (the checkpoint)
    float msSwallowT = -1.f, msTotalityT = -1.f;
    bool  msCheckpoint = false;
    float swallow01() const { return msSwallowT < 0.f ? 0.f : glm::smoothstep(0.4f, Enemy::MS_SWALLOW - 0.8f, msSwallowT); }
    float totality01() const { return msTotalityT < 0.f ? 0.f : glm::smoothstep(0.5f, 2.f, msTotalityT); }
    static Theme swallowedOf(const Theme& t);
    static Theme totalityOf(const Theme& t);
    void gatherMorningBoxes(std::vector<BoxInstance>& out);
    void onMorningEvents(Enemy& e, const EnemyEvents& ev);
    Enemy abCorpse{EnemyType::ABSOLUTE, glm::vec3{0.f}};
    bool  punchedThisTick = false;                  // F this tick: a serpent within reach goes back
    Enemy* absolute();
    void fillZenithWorld(EnemyWorld& w);
    bool parryAbsolute(Enemy& e, bool bySword);
    // Your key bindings (the defaults when there are no settings: tests, dev runs)
    const Bindings& binds() const { static const Bindings def; return settings ? settings->binds : def; }
    void inspect() { if (pendingWeapon < 0 && !weapons[activeWeapon].reloading) viewModel.triggerInspect(); }
    void swordParryAbsolute(Enemy& e);
    void onAbsoluteEvents(Enemy& e, const EnemyEvents& ev);
    void updateAbsolute(float dt);
    void onAbsoluteKilled(const Enemy& e);
    void updateAbsoluteDeath(float dt);
    void gatherAbsoluteBoxes(std::vector<BoxInstance>& out);
    void renderAbsoluteHUD();
    void devAbsolutePose(Enemy& e);   // --overlay abposeN (Gameplay_Dev.h)

    // ---- OATHBREAKER, the sword (Gameplay_Sword.h): key 5, only in the Zenith -----
    Sword sword;
    bool  swordOut = false, lmbPrev = false, rmbPrev = false;
    glm::vec3 thrustFrom{0.f}; bool thrustHit = false;
    Sword::Guarded lastGuarded = Sword::Guarded::NONE;   // what the sword did with the last strike (damagePlayerStrike)
    float swordRoll = 0.f;                          // the camera leaning into a sword stroke (degrees)
    bool swordAllowed() const;
    void drawSword();
    void holsterSword();
    void updateSword(float dt);
    void swordPlunge(glm::vec3 at);
    bool damagePlayerStrike(float dmg, glm::vec3 from, Sword::Strike k, float dt = 0.f);

    // ---- the enemies' voices (EnemyVoice.h) ---------------------------------------
    // Who speaks, and the held ones by uid
    VoiceDirector voices;
    std::unordered_map<int, SoundHandle> voiceLoops;     // a Seraph's beam, following it
    VoiceIn voiceIn(const Enemy& e, const EnemyEvents* ev) const;
    void playVoice(const VoiceCue& c);

    // ---- arenas that change as the fight goes on (Gameplay_Shifts.h) --------------
    void updateShifts(float dt);
    void gatherShiftBoxes(std::vector<BoxInstance>& out);

    // Footage camera (--cam, --campath, --autoaim, --kite; see the globals at the top)
    glm::vec3 devKite{0.f}, devKiteSafe{0.f};
    float devFireCd = 0.f;
    void devCamera(float dt);

    // Practice (dev level select): F5 ends the current wave on the spot
    void devClearWave();

    // Victory screen, right-hand side: the name prompt (when this run makes
    // the board) and the mode's leaderboard
    void renderLeaderboardPanel();

    // A controller button press (held buttons are read as keys; see Gamepad.h)
    void padButton(Uint8 b);
    void padCycle(int dir);   // D-pad left / right: the previous / next weapon

    // Web: post the run to the shared board (web/index.html does the request;
    // the name is already restricted to A-Z 0-9 space - . _)
    void submitWorld(const std::string& name, int diff);
    void submitBossWorld(const std::string& name, float t);

    // Victory screen: type a name (letters, digits, space - . _), ENTER saves
    // it to the leaderboard, ESC skips
    void handleNameEntry(const SDL_Event& e);

    void shake(float t, float amount);
    void updateScreenFx(float dt);
    void shootInput();
    void whizPast();
    bool onMetal() const;
    void waveEnded();
    void playBoom(const char* name, glm::vec3 at, int volume = 128, SoundGroup g = SoundGroup::WORLD, bool priority = false, float floor = 0.f);
    float wallAlong(glm::vec3 origin, glm::vec3 dir, float range) const;
    void shotEchoes(const char* sound, float loud);
    void flushHitSounds();

    // =========================================================================
    // Input
    // =========================================================================
    void handleEvent(const SDL_Event& e) override;

    // The weapon in hand (or about to be, mid-switch)
    WeaponId heldWeapon() const { return (WeaponId)(pendingWeapon >= 0 ? pendingWeapon : activeWeapon); }

    void trySwitch(int w);

    // While zoomed, scale mouse look by how much the view narrowed so the
    // crosshair tracks the same distance on screen, then by the user's setting.
    float aimSensScale() const;

    // Pause a live run (no-op on death/win screens, in the armory, or if already paused).
    void pause();

    const char* pauseLabel(int i) const;

    // Fullscreen from the pause menu. Desktop: the window (main.cpp applies the
    // setting). Web: the page, through web/index.html, which asks the embedding
    // portfolio page when there is one. In a browser this is the easy way out
    // of fullscreen: Escape belongs to the game there (see index.html).
    bool fullscreenOn() const;
    void toggleFullscreen();

    void activatePauseItem(int idx);

    int pauseItemAt(int mx, int my) const;

    void handlePauseEvent(const SDL_Event& e);

    void openArmory();
    void closeArmory();

    void buyUpgrade(int w, int s);

    void handleArmoryEvent(const SDL_Event& e);

    // =========================================================================
    // Per-frame update
    // =========================================================================
    // The soundtrack follows the fight: a track per area, layers by what's
    // happening (MusicSynth.h), muffled under the pause menu
    void updateMusic();

    void update(float dt) override;

    // Everything that moves on the physics tick remembers where it was at the
    // start of it; render() draws it blended toward where it is now, so it
    // glides at 144 or 240 Hz instead of stepping at 60. (Called during
    // hit-stop too, so frozen things stay still rather than flicker.)
    void snapshotForInterpolation();
    float renderAlpha = 1.f;   // set by render(): how far between the last two ticks we are
    glm::vec3 lerpPos(glm::vec3 prev, glm::vec3 now) const { return glm::mix(prev, now, renderAlpha); }

    // =========================================================================
    // Fixed-rate simulation (60 Hz)
    // =========================================================================
    void physicsTick(float dt, const Uint8* keys, bool parryKey);

    // =========================================================================
    // Punch / parry (F)
    //
    // F always throws a punch. In order of priority it:
    //   1. parries an enemy projectile in front of you (the window grows with
    //      the shot's speed, so sniper rounds are fair). It flies where you
    //      look; a Juggernaut's siege shell comes back as a 400-damage round
    //      that ignores armor.
    //   2. breaks a Juggernaut mid-smash (its last 0.4 s): staggered, it takes
    //      double damage.
    //   3. detonates your own grenade (projectile boost).
    //   4. hits whatever is right in front of you.
    // =========================================================================
    int findParryTarget() const;

    void parryFeedback(glm::vec3 at, bool heavy);
    void onMiniBossKilled(Enemy& e);
    void onPacemakerKilled(Enemy& e);
    static constexpr float MINI_LEFT_COST = 1.f;   // FAST: seconds on the clock for each enemy left standing when a room's mini-boss dies
    int   fastLeftStanding = 0;                    // FAST: how many were left this run (the end screen)
    // FAST: each room's time (its fight starting to its clear) and medal this run; ROOM ATTACK's result
    float roomStartT = 0.f;
    float ghostWhooshCd = 0.f;   // the ghost's whoosh: not more than this often
    std::vector<float> roomTimes;
    std::vector<int>   roomMedals;
    bool  roomNewBest = false;
    bool  keepsRoomTimes() const { return fast() && !g_godMode && !g_practice && !g_devCam && g_startWave <= 0; }
    void  startRoomAttack();
    // ---- what breaks (Gameplay_Destruct.h) ----
    float chipCd = 0.f;   // (a chip's knock, at most this often)
    glm::mat4 destructiblePlacement(const Destructible& d, const char* suffix) const;
    bool hitDestructible(glm::vec3 origin, glm::vec3 dir, float wallT, float dmg);
    void damageDestructible(int i, float dmg, glm::vec3 at, glm::vec3 push);
    void breakDestructible(int i, glm::vec3 push);
    void blastDestructibles(glm::vec3 at, float radius, float dmg);
    void restoreDestructibles();
    void updateDestructibles(float dt);
    void gatherDestructibleBoxes(std::vector<BoxInstance>& out);
    void  startArenaSolo();
    void  arenaSoloDone();
    void breakHalo(Enemy& e);
    bool anchoredAt(glm::vec3 feet) const;   // inside an ANCHOR's field: no dash, no grapple
    void pinnedCue();                        // the dull thunk when one of those is refused
    float pinnedCueCd = 0.f;   // a HALOED enemy's halo shatters: feedback and style

    void punch(int boostable);
    void reflectProjectile(int pi, glm::vec3 fwd);

    // Where would the grapple hook if fired now? Static walls are hit exactly;
    // moving platforms get a generous 1.2 m of aim assist and pull you to just
    // above their top, so hooking one lands you on it.
    bool findGrappleTarget(glm::vec3& point, int& wall, bool& isMover) const;

    // The player may only be inside the arena being fought — plus, once it's
    // cleared, the corridor and the next arena (FAST: every section reached so
    // far). Walls are low enough to stand on, so this (not wall height) is
    // what stops you leaving the map. Each zone's max.y is an invisible
    // ceiling, and falling below an arena's voidY returns you to its start.
    void keepPlayerInZone(float dt);

    void resetToCheckpoint(int a);

    // Ground enemies are solid: you can't walk through a Brute.
    void pushPlayerOutOfEnemies();

    // Highest walkable surface under (x, z) at or below fromY (0 = the floor)
    float groundHeightAt(float x, float z, float fromY, bool movers = false) const;   // movers: moving platforms count

    void spawnEnemy(EnemyType t, glm::vec3 pos, Hollow h = Hollow::NONE);
    int  nextEnemyUid = 1;
    float beamTickCd = 0.f, beamHissCd = 0.f;   // SERAPH beams: damage in 0.2 s ticks, a hiss now and then
    std::vector<Enemy> pendingTwins;   // TWINNED copies, spawned once the tick's enemy loop is done

    void updateEnemies(float dt);

    // Returns true if damage was applied (not blocked by i-frames).
    // iframes: how long you're untouchable after (a SERAPH beam's ticks give none)
    bool damagePlayer(float dmg, glm::vec3 from, float shakeT, float shakeAmt, float iframes = 0.35f);

    void showDamageFrom(glm::vec3 source);

    // All player damage to enemies goes through here. Returns true on a kill.
    // src: what did it, for style freshness (FRIENDLY: another enemy did).
    // pierceArmor: parried shots go straight through a Juggernaut's plating.
    // push: which way the hit shoves it and how hard (HitReact.h); none: away from you, by its damage
    struct HitPush { glm::vec3 dir{0.f}; float push = 0.f; float poise = 1.f; };
    bool hurtEnemy(Enemy& e, float dmg, glm::vec3 at, float style, float heal, StyleSource src,
                   bool crit = false, bool pierceArmor = false, const HitPush* push = nullptr);

    // ---- how a fight feels (Gameplay_Feel.h) ------------------------------------
    // A hit's thump at the body it landed in, once a tick: the hardest shove of the tick
    glm::vec3 bodyHitAt{0.f}; float bodyHitPush = 0.f, bodyHitMass = 1.f; bool bodyHitStun = false;
    void bodyHit(const Enemy& e, glm::vec3 at, float push, bool stunned);
    void onStunned(Enemy& e, glm::vec3 at);
    float killFx = 0.f;                 // 0..1 the screen's jolt on a kill (bloom_composite.frag uKick)
    // ATTACK TOKENS: only so many regular enemies wind up at once (more on harder
    // difficulties); the nearest you can see go first, the ones behind you wait longer
    int  attackersAllowed() const;
    void assignAttackers();
    // FINISHERS: F on an enemy low enough (it glows) finishes it: health back, the gun full
    int  findFinisher() const;
    bool tryFinisher();
    bool finishKey();                   // the FINISH key (E / R3): a finisher if one's in reach; never a parry
    static constexpr float FINISH_REACH = 6.f;
    float finishHint = 0.f;             // 0..1 the crosshair says a finisher is in reach
    float finishCueCd = 0.f;            // the "ready" cue: not more than this often
    int   finishReadyUid = -1;          // the last one it cued for
    // OVERDRIVE: the meter full is a power state: harder, faster shots that never need
    // reloading, every hit staggers, a punch finishes anything under half
    bool  overdriveOn() const { return styleSystem.overdrive; }
    float overdriveFx = 0.f;            // 0..1 the screen's gold (uDrive)
    float driveLeftPrev = 0.f;          // OVERDRIVE's time left last frame (its burn-out ticks)
    float driveCoolPrev = 0.f;          // its cooldown last frame (the chime when it can burn again)
    void  enterOverdrive();
    void  leaveOverdrive();
    void  updateFeel(float dt);
    // ALT FIRE (RMB on the guns that don't aim): the revolver FANS the rest of its cylinder in a
    // fast, loose, staggering burst; the shotgun lets BOTH BARRELS go at once: twice the pellets,
    // a huge shove, and a kick that throws you back (aimed at the floor: a shotgun jump)
    bool  pendingAlt = false, altPrev = false;
    int   fanLeft = 0; float fanTimer = 0.f;
    bool  fanning = false, doubleShot = false;   // set around fireWeapon for the alt shots
    void  altFire();
    // TEACHING: each fighting move gets one small card (the enemy cards' kind), the first time
    // it comes up, ever; then never again (GameSettings::taught)
    enum Teach { TEACH_FINISH = 1, TEACH_STUN = 2, TEACH_OVERDRIVE = 4, TEACH_HOOK = 8, TEACH_ALT = 16 };
    void  teach(int bit);
    int   killsThisSession = 0;
    // UNSEEN WIND-UPS: a regular enemy out of view (or behind cover you can't see round) starting an
    // attack gets a chevron at the screen's edge and a tick from where it is
    bool  unseenWindup(const Enemy& e, glm::vec3 flatFwd) const;
    float threatTickCd = 0.f;
    // MOVEMENT INTO COMBAT: the grapple hooks enemies (light ones are yanked to you,
    // stunned; you zip to heavy ones), a slide kicks what it runs into, a slam
    // hits harder the further it fell, kills in the air score more
    static constexpr float HOOK_RANGE = 36.f;
    static constexpr float HOOK_PULL_MASS = 1.65f;   // up to this: yanked to you; heavier: you go to it
    int   findEnemyHook() const;
    void  hookEnemy(int i);
    void  updateEnemyZip();          // hooked into a heavy one: the rope follows it, and lets go as you arrive
    float yankT = 0.f; int yankUid = -1;   // a light one on the end of the rope (its rope is drawn while yankT > 0)
    float airT = 0.f;                // how long you've been off the ground
    float slamFromY = 0.f;           // where the slam began: the drop sets its strength
    std::vector<int> slideKicked;    // already kicked this slide (uids)
    void  slideKicks();
    void  slamLands();
    // The run log (RunLog.h): what hurt you, what killed you, each gun's numbers
    RunLog runLog;
    EnemyType hurtBy = EnemyType::COUNT;   // set around enemy events and shots: who's hitting you now
    bool  loggedDeath = false;
    void  logRun(const char* how);

    void onEnemyKilled(Enemy& e, StyleSource src);

    // The style source for a gun
    static StyleSource weaponSource(WeaponId id) { return (StyleSource)((int)StyleSource::REVOLVER + (int)id); }
    // The kill confirm for what made the kill (nullptr: none, e.g. the void or friendly fire)
    static const char* killTag(StyleSource s) {
        switch (s) {
            case StyleSource::REVOLVER: return "kill_light";
            case StyleSource::SHOTGUN: case StyleSource::EXPLOSIVE: case StyleSource::SLAM: case StyleSource::PUNCH: return "kill_heavy";
            case StyleSource::KAR: case StyleSource::LONGSHOT: return "kill_rifle";
            case StyleSource::MELEE: case StyleSource::PARRY: return "kill_blade";
            default: return nullptr;
        }
    }

    // Brute slams and Juggernaut smashes hurt the enemies around them too
    void friendlySlam(const Enemy& slammer, float radius);

    void gainXp(int xp);

    // Explosions resolve in a loop so a blast can kill a mite whose blast
    // kills another mite… (capped, in case of a giant chain).
    void processBlasts();

    void updatePickups(float dt);

    // =========================================================================
    // Weapons
    // =========================================================================

    // Every enemy along a ray up to the first wall, nearest first.
    struct RayHit { int enemy; float t; bool head; bool wound = false; LevZone zone = LevZone::NONE; };   // zone: where it found the LEVIATHAN   // wound: the PENITENT's back (x3)
    float hitscanAll(glm::vec3 origin, glm::vec3 dir, float range, std::vector<RayHit>& out);

    // Sounds timed to the reload animations (ViewModel.h): the revolver's
    // cylinder out, brass, speedloader, snap shut; each shotgun shell, the rack

    void startReload(int w);

    void fireWeapon(int w);

    void throwGrenade();

    // =========================================================================
    // Effects (Effects.h)
    // =========================================================================
    // Break an enemy into its boxes, bouncing on the ground under it
    void spawnDebrisFor(const Enemy& e) {
        fx.spawnDebrisFor(e, gameClock, groundHeightAt(e.position.x, e.position.z, e.position.y + 0.5f) + 0.1f);
    }

    // =========================================================================
    // Rendering
    // =========================================================================
    void applyLighting(ShaderProgram& sh, const Theme& th, glm::vec3 camPos);

    // Everything dynamic that's made of boxes, gathered for one instanced draw.
    void gatherBoxes(std::vector<BoxInstance>& out, const glm::mat4& view);

    bool scopedView() const { return activeWeapon == (int)WeaponId::LONGSHOT && aim > 0.9f; }

    void render() override;

    // Project a world point to the screen. Off-screen (or behind) points are
    // pushed to the screen edge in the right direction.
    bool projectToScreen(glm::vec3 p, const glm::mat4& view, const glm::mat4& proj, float& sx, float& sy) const;

    void renderHUD(const glm::mat4& view, const glm::mat4& proj);

    void renderDecals();

    struct PVert { float x, y, z, r, g, b, a; };
    void renderParticles(const glm::mat4& view, const glm::mat4& proj);
    void renderWater(const glm::mat4& view, const glm::mat4& proj, const Theme& th, glm::vec3 camPos);
    void drawPoints(const PVert* buf, int count, const glm::mat4& view, const glm::mat4& proj);

    // The first time OpenGL (on Metal, or WebGL through ANGLE) sees a new
    // shader + blend combination it compiles a pipeline on the spot, which
    // froze the game for ~200 ms the first time you fired, a Sentinel aimed
    // or you scoped in. The first frames of a level draw one invisible
    // instance of each effect so that happens behind the loading screen.
    int warmupFrames = 2;
    void warmPipelines(glm::vec3 camPos, const glm::mat4& view, const glm::mat4& proj);

    // Draws thin additive beams. Shared by bullet tracers and Sentinel lasers.
    // Each beam: start (xyz, alpha), end (xyz, alpha), width.
    struct Beam { glm::vec4 a, b; float width; };
    void drawBeams(const std::vector<Beam>& beams, glm::vec3 color, const glm::mat4& view, const glm::mat4& proj);

    void renderTracers(const glm::mat4& view, const glm::mat4& proj);

    // CONDUCTOR tethers (linkConductors, Enemy.h)
    void renderTethers(const glm::mat4& view, const glm::mat4& proj);

    // A Sentinel winding up paints you with a laser; it brightens until it fires.
    void renderLasers(const glm::mat4& view, const glm::mat4& proj);
    void renderFlare(const glm::mat4& view, const glm::mat4& proj);   // SOLAR: the sun's arms of light
    float flareTickCd = 0.f;      // SOLAR: damage in 0.2 s ticks
    bool  flareIntroduced = false;
    FlarePhase flarePrev = FlarePhase::OFF;

    int findInteractTarget() const;
};

// The member functions, by topic
#include "game/Gameplay_Flow.h"
#include "game/Gameplay_Tick.h"
#include "game/Gameplay_Combat.h"
#include "game/Gameplay_Feel.h"
#include "game/Gameplay_Menus.h"
#include "game/Gameplay_Render.h"
#include "game/Gameplay_HUD.h"
#include "game/Gameplay_Dev.h"
#include "game/Gameplay_Shifts.h"
#include "bosses/sovereign/Gameplay_Sovereign.h"
#include "bosses/pacemaker/Gameplay_Pacemaker.h"
#include "bosses/penitent/Gameplay_Penitent.h"
#include "bosses/warden/Gameplay_Warden.h"
#include "game/Gameplay_Reliquary.h"
#include "bosses/leviathan/Gameplay_Leviathan.h"
#include "game/Gameplay_Sword.h"
#include "game/Gameplay_Destruct.h"
#include "bosses/absolute/Gameplay_Absolute.h"
