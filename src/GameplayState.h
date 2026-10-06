#pragma once
#include "GameState.h"
#include "Player.h"
#include "Camera.h"
#include "Mesh.h"
#include "ShaderProgram.h"
#include "StyleSystem.h"
#include "UIRenderer.h"
#include "Enemy.h"
#include "EnemyModel.h"
#include "BoxRenderer.h"
#include "Projectile.h"
#include "GrappleHook.h"
#include "Level.h"
#include "LevelGauntlet.h"
#include "WaveDirector.h"
#include "PostProcess.h"
#include "AudioSystem.h"
#include "ViewModel.h"
#include "Interactable.h"
#include "Settings.h"
#include "SettingsMenu.h"
#include "TextureGen.h"
#include "Weapons.h"
#include "Progression.h"
#include "LeaderboardView.h"
#include "Ghost.h"
#include "Gamepad.h"
#include "MouseFilter.h"
#include "WorldMesh.h"
#include "Effects.h"
#include "ArenaShifts.h"
#include "Score.h"
#include "Daily.h"
#include "EndlessWaves.h"
#include "SovereignHazards.h"
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include "gl.h"
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
// MODES: ARENA runs buildLevel() (four arenas, waves, the boss). FAST runs
// buildGauntlet() (LevelGauntlet.h): a countdown, a clock with splits, levels
// whose fights start at a trigger after a breather, and a finish beacon.
//
// KEY EXTENSION POINTS:
//   ADD A WEAPON:  a row in Weapons.h, a model in ViewModel.h, a sound name in
//                  fireWeapon(). Damage goes through hurtEnemy().
//   ADD AN ENEMY:  Enemy.h (stats + AI), EnemyModel.h (its box rig), then list
//                  it in a wave in Level.h.
//   CHANGE A MAP:  Level.h / LevelGauntlet.h. Walls render and collide
//                  automatically; movers are walls that LevelData moves.
// =============================================================================
static constexpr int   SCREEN_W   = 1280;
static constexpr int   SCREEN_H   = 720;
static constexpr float PHYSICS_HZ = 60.f;
static constexpr float PHYSICS_DT = 1.f / PHYSICS_HZ;

// Set from the command line (--arena N) or the web URL (?arena=N) to start a
// run at a later arena — handy for testing and for showing off the boss.
inline int g_startArena = 0;
inline int g_startWave  = 0;   // --wave N / ?wave=N (with --arena): skip to that wave
// --god / ?god: the player takes no damage (for recording footage)
inline bool g_godMode = false;
inline bool g_practice = false;   // dev level select: no records, no leaderboard
// --cam x y z yaw pitch: start the camera somewhere specific (screenshots)
inline bool      g_devCam = false;
inline glm::vec3 g_devCamPos{0.f};
inline float     g_devCamYaw = -90.f, g_devCamPitch = 0.f;
// --weapon N --aim --overlay armory|pause|settings: pose a screenshot
inline int       g_devWeapon = -1;
inline bool      g_devAim = false;
inline std::string g_devOverlay;
inline bool      g_devNoMouse = false;   // screenshot runs: never grab or read the mouse
inline std::vector<int> g_devSpawns;    // --spawn N (repeatable): enemies of type N in an arc 9-12 m in front of the camera
// Footage (--record, see main.cpp): every frame advances a fixed step, the
// camera glides from --cam to --campath, --autoaim turns it onto the nearest
// enemy in sight and fires, --clean hides title cards and control hints
inline double    g_fixedDt = 0.0;
inline bool      g_devCamPath = false;
inline glm::vec3 g_devCamPos2{0.f};
inline float     g_devCamYaw2 = -90.f, g_devCamPitch2 = 0.f;
inline float     g_recProgress = 0.f;    // 0..1 through the recording
inline bool      g_devAutoAim = false;
inline bool      g_devClean = false;
inline bool      g_devKite = false;      // --kite: the camera backs off from whatever gets close

class GameplayState : public GameState {
public:
    std::function<void()> onReturnToMenu;
    std::function<void()> onQuit;

    GameSettings* settings = nullptr;   // injected by main — may be null (safe)
    GameMode      mode = GameMode::ARENA;
    bool fast() const { return mode == GameMode::FAST; }
    bool act2() const { return mode == GameMode::ACT2; }
    bool endless() const { return mode == GameMode::ENDLESS || mode == GameMode::DAILY; }
    bool dailyRun() const { return mode == GameMode::DAILY; }
    Board boardFor() const {
        return fast() ? Board::FAST : mode == GameMode::ENDLESS ? Board::ENDLESS : dailyRun() ? Board::DAILY : Board::ARENA;
    }

    // ---- ENDLESS / DAILY (Gameplay_Flow.h) ----
    DailyInfo    daily = DailyInfo::today();
    EndlessWaves gen;
    int   endlessArena = 3;        // ENDLESS runs in the Core; DAILY wherever today's says
    int   endlessBaseAlive = 11;
    int   wavesCleared = 0;
    bool  modOn(DailyMod m) const { return dailyRun() && daily.mod == m; }
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
    int         textureQuality = -1;   // graphics quality the world textures are filtered for
    void applyTextureQuality();
    WorldMeshes world;

    std::vector<Enemy> enemies;
    SpatialGrid        spatialGrid;   // built once; door boxes move only in Y, movers are tested separately

    // ---- Weapons (see Weapons.h) ----
    std::array<WeaponState, WEAPON_COUNT> weapons;
    int   activeWeapon   = 0;
    int   pendingWeapon  = -1;
    float weaponSwitchTimer = 0.f;
    float aim         = 0.f;    // 0 hip .. 1 aimed (rifles)
    float aimFullAt   = -1.f;   // gameClock when aim last reached full (quickscope window)
    float boltSoundTimer = -1.f;
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

    float controlHintTimer = 10.f;
    float footstepTimer = 0.f;

    int   dashCharges       = 2;
    float dashCooldown      = 0.f;
    float dashMomentumTimer = 0.f;
    int   jumpsRemaining = 2;
    bool  prevOnGround   = false;
    bool  slamming       = false;
    float invincFrames   = 0.f;

    float shakeTimer     = 0.f;
    float shakeIntensity = 0.f;

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
    float landSquash     = 0.f;

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
    std::vector<Pickup> pickups;

    // src: whose blast it is, for style (a Mite that reaches you hurts its friends: FRIENDLY)
    struct Blast { glm::vec3 pos; float radius, damage; float playerRadius, playerDamage;
                   StyleSource src = StyleSource::EXPLOSIVE; };

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
    bool prevDashKey      = false;
    int  boostPrev        = -1;     // booster the player was in last tick
    float telegraphSoundCd = 0.f;   // throttles the enemy wind-up tick
    float shieldClankCd = 0.f;      // throttles the clank of bullets on a shield
    bool prevJumpKey      = false;
    // Event-driven click flags — set in handleEvent, consumed once in physicsTick.
    bool pendingFire      = false;
    bool pendingGrenade   = false;
    bool pendingGrapple   = false;
    bool spawnSoundThisTick = false;

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
    void recordSplit(int section);

    void finishRun();

    // --overlay poseN (screenshots): hold a SOVEREIGN in one pose, facing the camera.
    // 0 idle, 1 dash wind-up, 2 dashing, 3 sweep wind-up, 4 mid-sweep,
    // 5 cleave wind-up, 6 mid-cleave, 7 leaping, 8 broken, 9 enraged, 10 shadow step
    // wind-up, 11 judgment, 12 whirlwind wind-up, 13 whirling, 14 thrust wind-up,
    // 15 thrusting, 16 sending phantoms
    void devPose(Enemy& e);

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
    void updateSovereign(float dt);
    void gatherSovereignBoxes(std::vector<BoxInstance>& out);
    glm::vec3 sanctumCentre() const;

    // Arenas that change as the fight goes on (Gameplay_Shifts.h)
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

    // Web: post the run to the shared board (web/index.html does the request;
    // the name is already restricted to A-Z 0-9 space - . _)
    void submitWorld(const std::string& name, int diff);

    // Victory screen: type a name (letters, digits, space - . _), ENTER saves
    // it to the leaderboard, ESC skips
    void handleNameEntry(const SDL_Event& e);

    void shake(float t, float amount);

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
    void breakHalo(Enemy& e);   // a HALOED enemy's halo shatters: feedback and style

    void punch(int boostable);

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
    float groundHeightAt(float x, float z, float fromY) const;

    void spawnEnemy(EnemyType t, glm::vec3 pos, Hollow h = Hollow::NONE);
    int  nextEnemyUid = 1;

    void updateEnemies(float dt);

    // Returns true if damage was applied (not blocked by i-frames).
    bool damagePlayer(float dmg, glm::vec3 from, float shakeT, float shakeAmt);

    void showDamageFrom(glm::vec3 source);

    // All player damage to enemies goes through here. Returns true on a kill.
    // src: what did it, for style freshness (FRIENDLY: another enemy did).
    // pierceArmor: parried shots go straight through a Juggernaut's plating.
    bool hurtEnemy(Enemy& e, float dmg, glm::vec3 at, float style, float heal, StyleSource src,
                   bool crit = false, bool pierceArmor = false);

    void onEnemyKilled(Enemy& e, StyleSource src);

    // The style source for a gun
    static StyleSource weaponSource(WeaponId id) { return (StyleSource)((int)StyleSource::REVOLVER + (int)id); }

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
    struct RayHit { int enemy; float t; bool head; };
    float hitscanAll(glm::vec3 origin, glm::vec3 dir, float range, std::vector<RayHit>& out);

    // Sounds timed to the reload animations (ViewModel.h): the revolver's
    // cylinder out, brass, speedloader, snap shut; each shotgun shell, the rack
    float reloadCueAt = 0.f;   // reload progress already cued
    void reloadSounds();

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

    int findInteractTarget() const;
};

// The member functions, by topic
#include "Gameplay_Flow.h"
#include "Gameplay_Tick.h"
#include "Gameplay_Combat.h"
#include "Gameplay_Menus.h"
#include "Gameplay_Render.h"
#include "Gameplay_HUD.h"
#include "Gameplay_Dev.h"
#include "Gameplay_Shifts.h"
#include "Gameplay_Sovereign.h"
