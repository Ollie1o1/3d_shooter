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

static GLuint makeGreyTexture() {
    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    unsigned char grey[4]={200,200,200,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,grey);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    return tex;
}

// Static level geometry, batched by texture. Each wall's material picks the
// texture of its sides and of its top; undersides are always metal, and the
// self-lit neon is drawn untextured.
enum WorldTex { TEX_GRID, TEX_BRICK, TEX_METAL, TEX_PANEL, TEX_ROCK, TEX_CONCRETE, TEX_COUNT };
struct WorldMeshes { Mesh byTex[TEX_COUNT]; Mesh neon; };

static WorldTex sideTex(Mat m) {
    switch (m) {
        case Mat::PANEL: return TEX_PANEL;  case Mat::ROCK: return TEX_ROCK;
        case Mat::METAL: return TEX_METAL;  case Mat::CONCRETE: return TEX_CONCRETE;
        default: return TEX_BRICK;
    }
}
static WorldTex topTex(Mat m) {
    switch (m) {
        case Mat::PANEL: case Mat::METAL: return TEX_METAL;
        case Mat::ROCK: return TEX_ROCK;
        default: return TEX_GRID;
    }
}

static WorldMeshes buildWorldMeshes(const LevelData& L) {
    std::vector<Vertex> V[TEX_COUNT], nV;
    std::vector<unsigned int> I[TEX_COUNT], nI;
    const float texScale = 0.25f;   // one texture repeat per 4 m

    auto pushFace = [&](std::vector<Vertex>& verts, std::vector<unsigned int>& idx,
                        glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
                        glm::vec3 n, glm::vec3 col, bool shade) {
        // Sides darken toward their base: a cheap contact shadow
        float yLo = std::min({a.y, b.y, c.y, d.y});
        float ySpan = std::max({a.y, b.y, c.y, d.y}) - yLo;
        auto gc = [&](float y) -> glm::vec3 {
            if (!shade) return col;
            float t = (ySpan > 0.01f) ? (y - yLo) / ySpan : 1.f;
            // Tall rock and towers shouldn't go black for 30 m: shade the bottom few metres only
            if (ySpan > 12.f) t = glm::clamp((y - yLo) / 12.f + 0.35f, 0.f, 1.f);
            return col * glm::mix(0.55f, 1.0f, t);
        };
        auto worldUV = [&](glm::vec3 p) -> glm::vec2 {
            float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
            if (ay > ax && ay > az) return {p.x * texScale, p.z * texScale};
            else if (ax > az)       return {p.z * texScale, p.y * texScale};
            else                    return {p.x * texScale, p.y * texScale};
        };
        unsigned int base = (unsigned int)verts.size();
        verts.push_back({a, worldUV(a), n, gc(a.y)});
        verts.push_back({b, worldUV(b), n, gc(b.y)});
        verts.push_back({c, worldUV(c), n, gc(c.y)});
        verts.push_back({d, worldUV(d), n, gc(d.y)});
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    auto pushBox = [&](const AABB& b, glm::vec3 col, bool neon, Mat m) {
        glm::vec3 mn = b.min, mx = b.max;
        int st = sideTex(m), tt = topTex(m);
        auto& sv = neon ? nV : V[st];      auto& si = neon ? nI : I[st];
        auto& tv = neon ? nV : V[tt];      auto& ti = neon ? nI : I[tt];
        auto& bv = neon ? nV : V[TEX_METAL]; auto& bi = neon ? nI : I[TEX_METAL];
        bool shade = !neon;
        pushFace(sv, si, {mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{ 1, 0, 0}, col, shade);
        pushFace(sv, si, {mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1, 0, 0}, col, shade);
        pushFace(sv, si, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{ 0, 0, 1}, col, shade);
        pushFace(sv, si, {mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{ 0, 0,-1}, col, shade);
        pushFace(tv, ti, {mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},{ 0, 1, 0}, col, false);
        if (b.min.y > 0.1f || neon)
            pushFace(bv, bi, {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z},{ 0,-1, 0}, col * 0.8f, false);
    };

    for (auto& f : L.floors)
        pushFace(V[TEX_GRID], I[TEX_GRID], {f.x0,f.y,f.z1},{f.x1,f.y,f.z1},{f.x1,f.y,f.z0},{f.x0,f.y,f.z0},{0,1,0}, f.color, false);
    for (int i = 0; i < (int)L.walls.size(); ++i)
        if (!L.walls[i].hidden && !L.isDoorWall(i)) pushBox(L.walls[i].box, L.walls[i].color, false, L.walls[i].mat);
    for (auto& p : L.props) pushBox(p.box, p.color, false, p.mat);
    for (auto& n : L.neon)  pushBox(n.box, n.color, true, Mat::BRICK);

    WorldMeshes m;
    for (int t = 0; t < TEX_COUNT; ++t) m.byTex[t].upload(V[t], I[t]);
    m.neon.upload(nV, nI);
    return m;
}

class GameplayState : public GameState {
public:
    std::function<void()> onReturnToMenu;
    std::function<void()> onQuit;

    GameSettings* settings = nullptr;   // injected by main — may be null (safe)
    GameMode      mode = GameMode::ARENA;
    bool fast() const { return mode == GameMode::FAST; }
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
    void applyTextureQuality() {
        int q = settings ? settings->quality : GameSettings::QUALITY_DEFAULT;
        if (q == textureQuality) return;
        textureQuality = q;
        for (GLuint t : worldTex) if (t) TextureGen::setAnisotropy(t, GameSettings::qualityAnisotropy(q));
    }
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

    // Hitscan tracer — thin billboard quad, additive blending, fades fast
    struct Tracer {
        glm::vec3 start{0.f}, end{0.f};
        float life = 0.f, maxLife = 0.13f;
        float width = 0.055f;
        bool  alive = false;
    };
    static constexpr int MAX_TRACERS = 32;
    Tracer        tracers[MAX_TRACERS];
    ShaderProgram tracerShader;
    GLuint        tracerVAO = 0, tracerVBO = 0;
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

    struct Decal {
        glm::vec3 pos{0.f};
        float life = 0.f, maxLife = 10.f;
        bool  alive = false;
    };
    static constexpr int MAX_DECALS = 64;
    Decal  decals[MAX_DECALS];
    GLuint decalVAO = 0, decalVBO = 0;

    struct Particle {
        glm::vec3 pos{0.f}, vel{0.f};
        glm::vec3 color{1.f, 0.5f, 0.05f};
        float life = 0.f, maxLife = 0.f;
        float gravity = 18.f;     // negative = floats upward (embers)
        bool  alive = false;
    };
    static constexpr int MAX_PARTICLES = 1200;
    Particle  particles[MAX_PARTICLES];
    int       particleCursor = 0;
    GLuint    particleVAO = 0, particleVBO = 0;
    float     ambientTimer = 0.f;

    // Enemies come apart into their boxes when they die
    struct Debris {
        glm::mat3 shape;          // rotation * scale of the original part
        glm::vec3 pos, vel, axis, color, emissive;
        float angle = 0.f, spin = 0.f, life = 0.f, maxLife = 1.f, floorY = 0.f;
    };
    std::vector<Debris> debris;

    // Drops: health orbs (common, small heal, magnetic), health potions (rare,
    // big heal, only taken when hurt) and XP shards (rare, bonus XP)
    enum class PickupKind { ORB, POTION, XP };
    struct Pickup { glm::vec3 pos, vel; float life; PickupKind kind; float floorY; glm::vec3 prev{0.f}; };
    std::vector<Pickup> pickups;

    struct Shockwave { glm::vec3 pos; float radius, t; glm::vec3 color; };
    // A SOVEREIGN sword stroke: an arc of light that sweeps out and fades
    // (kind as EnemyEvents::slash: 0/1 sweeps, 2 the overhead cleave, 3 a dash's cut)
    struct Slash { glm::vec3 pos; float yaw; int kind; float t; };
    static constexpr float SLASH_LIFE = 0.3f;
    std::vector<Slash> slashes;
    std::vector<Shockwave> shockwaves;

    struct Blast { glm::vec3 pos; float radius, damage; float playerRadius, playerDamage; };
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

    GameplayState(AudioSystem& aud, GameSettings* s = nullptr, GameMode m = GameMode::ARENA)
        : settings(s), mode(m), audio(aud) {
        worldShader.loadFiles("src/shader.vert","src/shader.frag");
        skyboxShader.loadFiles("src/skybox.vert","src/skybox.frag");
        tracerShader.loadFiles("src/tracer.vert","src/tracer.frag");
        particleShader.loadFiles("src/particle.vert","src/particle.frag");

        // Tracer VBO: 4 floats per vertex (xyz + alpha), dynamic
        glGenVertexArrays(1, &tracerVAO);
        glGenBuffers(1, &tracerVBO);
        glBindVertexArray(tracerVAO);
        glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_TRACERS * 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(3*sizeof(float)));
        glBindVertexArray(0);

        // Particle VBO: [x, y, z, r, g, b, alpha] per point
        glGenVertexArrays(1, &particleVAO);
        glGenBuffers(1, &particleVBO);
        glBindVertexArray(particleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(6*sizeof(float)));
        glBindVertexArray(0);

        // Decal VBO: pos(3)+uv(2)+normal(3)+color(3) = 11 floats, matches worldShader
        glGenVertexArrays(1, &decalVAO);
        glGenBuffers(1, &decalVBO);
        glBindVertexArray(decalVAO);
        glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_DECALS * 6 * 11 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(5*sizeof(float)));
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(8*sizeof(float)));
        glBindVertexArray(0);

        whiteTex = makeGreyTexture();
        worldTex[TEX_GRID]     = TextureGen::generateGridFloor(128);
        worldTex[TEX_BRICK]    = TextureGen::generateBrickWall(128);
        worldTex[TEX_METAL]    = TextureGen::generateMetalCeiling(128);
        worldTex[TEX_PANEL]    = TextureGen::generatePanel(128);
        worldTex[TEX_ROCK]     = TextureGen::generateRock(128);
        worldTex[TEX_CONCRETE] = TextureGen::generateConcrete(128);

        level = fast() ? buildGauntlet() : buildLevel();
        spatialGrid.build(level.walls);
        world = buildWorldMeshes(level);
        director.level = &level;
        director.fast  = fast();
        records.load();

        settingsMenu.s = settings;
        settingsMenu.onBack = [this]() { pauseSettings = false; };

        for (int i=0;i<MAX_POINT_LIGHTS;++i) {
            pointLightPos[i]   = {0,0,0};
            pointLightColor[i] = {0,0,0};
        }

        int start = glm::clamp(g_startArena, 0, (int)level.arenas.size() - 1);
        ranked = start == 0 && g_startWave <= 0 && !g_godMode && !g_practice && !g_devCam;
        enterArena(start);
        director.wave = glm::clamp(g_startWave, 0, director.waveCount() - 1);
        if (fast()) ghost.load();
        if (fast() && start == 0) { countdown = 3.f; pushBanner("THE GAUNTLET", "SEVEN ROOMS - THEN REACH THE BEACON ON THE TOWER", {1.f, 0.6f, 0.2f}, 3.f); }
        if (g_devCam) countdown = 0.f;
        if (g_devWeapon >= 0) activeWeapon = g_devWeapon % WEAPON_COUNT;
        if (g_devAim) aim = 1.f;
        if (g_devOverlay == "armory") { prog.points = 3; prog.up[2].tier[0] = 2; prog.up[3].mod = true; armoryOpen = true; armoryW = 2; }
        if (g_devOverlay == "pause") paused = true;
        if (g_devOverlay == "victory") {   // the victory screen mid name entry (screenshots)
            elapsedTime = 754.3f; victory = true; ranked = true;
            board.load(); nameEntry = true; nameBuf = "OLLIE";
        }
        for (int k = 0; k < (int)g_devSpawns.size(); ++k) {
            float a = (k - (g_devSpawns.size() - 1) * 0.5f) * 0.4f, dist = 9.f + (k % 2) * 3.f;
            glm::vec3 f = player.camera.flatForward(), r{-f.z, 0.f, f.x};
            glm::vec3 at = player.position * glm::vec3{1, 0, 1} + (f * std::cos(a) + r * std::sin(a)) * dist;
            spawnEnemy((EnemyType)g_devSpawns[k], at + glm::vec3{0, groundHeightAt(at.x, at.z, player.position.y + 1.f), 0});
        }
        if (g_devOverlay == "settings") { paused = true; pauseSettings = true; }

        prevTicks = SDL_GetPerformanceCounter();
        freq      = SDL_GetPerformanceFrequency();
        prevCamPos = player.camera.position;

        captureMouse(true);
    }

    ~GameplayState() {
        glDeleteTextures(1,&whiteTex);
        glDeleteTextures(TEX_COUNT, worldTex);
        if (tracerVAO)   glDeleteVertexArrays(1, &tracerVAO);
        if (tracerVBO)   glDeleteBuffers(1, &tracerVBO);
        if (particleVAO) glDeleteVertexArrays(1, &particleVAO);
        if (particleVBO) glDeleteBuffers(1, &particleVBO);
        if (decalVAO)    glDeleteVertexArrays(1, &decalVAO);
        if (decalVBO)    glDeleteBuffers(1, &decalVBO);
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    void captureMouse(bool on) {
        if (g_devNoMouse) on = false;
        SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE);
        if (on) mouseFilter.onCapture();
    }

    // =========================================================================
    // Run / arena lifecycle
    // =========================================================================

    // Put the player at the start of arena `a` with the doors set as if they
    // had just walked in. Used for a fresh run, a retry after death, and --arena.
    void enterArena(int a) {
        // Every door shut; exits of the arenas already beaten unlocked. The way
        // in locks when the fight starts (ARENA_START).
        for (int d = 0; d < (int)level.doors.size(); ++d) { level.doors[d].locked = false; level.setDoorInstant(d, false); }
        for (int i = 0; i < (int)level.arenas.size(); ++i) {
            const Arena& ar = level.arenas[i];
            if (ar.exitDoor >= 0) level.doors[ar.exitDoor].locked = i >= a;
        }
        resetPlayer(level.arenas[a].playerStart);
        player.camera.yaw = level.arenas[a].startYaw;
        if (g_devCam) {
            player.position = g_devCamPos - glm::vec3{0, player.eyeHeight, 0};
            player.camera.position = g_devCamPos;
            player.camera.yaw = g_devCamYaw; player.camera.pitch = g_devCamPitch;
            prevCamPos = g_devCamPos;
        }
        enemies.clear();
        for (auto& p : projSystem.pool) p.alive = false;
        for (auto& d : decals)          d.alive = false;
        for (auto& p : particles)       p.alive = false;
        debris.clear(); pickups.clear(); shockwaves.clear(); pendingBlasts.clear();
        banners.clear();
        grapple.release();
        playerDead = false; deadTimer = 0.f;
        victory = false; victoryDelay = -1.f;
        finishOpen = false;
        // Health and XP placed in the level's breathers
        for (auto& pp : level.placedPickups) {
            PickupKind k = pp.kind == 1 ? PickupKind::POTION : pp.kind == 2 ? PickupKind::XP : PickupKind::ORB;
            float fy = groundHeightAt(pp.pos.x, pp.pos.z, pp.pos.y + 0.5f);
            pickups.push_back({pp.pos + glm::vec3{0, 0.6f, 0}, glm::vec3{0.f}, 1e9f, k, fy, pp.pos + glm::vec3{0, 0.6f, 0}});
        }
        // Splits after this section belong to a run we're redoing
        if (fast() && (int)splits.size() > a) splits.resize(a);
        if (fast()) director.approach(a);   // the fight starts at the section's trigger
        else director.startArena(a);
    }

    void resetPlayer(glm::vec3 start) {
        player = Player{start};
        player.camera.aspectRatio = (float)SCREEN_W/SCREEN_H;
        player.camera.fov = settings ? settings->fov : 90.f;
        prevCamPos = player.camera.position;
        styleSystem = StyleSystem{};
        grenadeCount = grenadeMax; killsThisCycle = 0;
        for (int w = 0; w < WEAPON_COUNT; ++w) {
            weapons[w] = WeaponState{};
            weapons[w].ammo = weaponMag((WeaponId)w, prog.up[w]);
        }
        aim = 0.f; aimFullAt = -1.f; boltSoundTimer = -1.f;
        dashCharges = 2; dashCooldown = 0.f; dashMomentumTimer = 0.f;
        jumpsRemaining = 2; slamming = false; invincFrames = 0.f;
        activeWeapon = 0; pendingWeapon = -1; weaponSwitchTimer = 0.f;
        recoilPitch = 0.f; peakFallSpeed = 0.f; landSquash = 0.f; fovKick = 0.f;
        paused = false; pauseSettings = false; pauseSelected = 0; armoryOpen = false;
        ui.clearIndicators();
    }

    // Restart the room / arena you're in from its checkpoint (pause menu,
    // Backspace). Not a death; the run clock keeps going.
    void restartHere() {
        enterArena(director.arena + (!fast() && director.phase == WaveDirector::Phase::CLEARED ? 1 : 0));
        captureMouse(true);
    }

    void retryArena() {
        ++deaths;
        if (fast()) { enterArena(director.arena); captureMouse(true); return; }
        // Died on the way out of a cleared arena? Pick up at the next one.
        bool cleared = director.phase == WaveDirector::Phase::CLEARED;
        enterArena(director.arena + (cleared ? 1 : 0));
        captureMouse(true);
    }

    void newRun() {
        director = WaveDirector{};
        director.level = &level;
        director.fast  = fast();
        prog = Progression{};
        totalKills = totalShots = totalHits = deaths = 0;
        elapsedTime = 0.f; peakStyle = 0.f;
        controlHintTimer = 10.f;
        splits.clear();
        newRecord = false;
        ranked = !g_godMode && !g_practice;
        nameEntry = false; boardPlace = -1;
        ghostRec.pts.clear();
        if (fast()) ghost.load();
        enterArena(0);
        if (fast()) { countdown = 3.f; pushBanner("THE GAUNTLET", "SEVEN ROOMS - THEN REACH THE BEACON ON THE TOWER", {1.f, 0.6f, 0.2f}, 3.f); }
        captureMouse(true);
    }

    void lockDoor(int d, bool locked) { if (d >= 0) level.doors[d].locked = locked; }

    void pushBanner(const std::string& title, const std::string& sub, glm::vec3 col, float dur = 3.f) {
        banners.push_back({title, sub, col, 0.f, dur});
    }

    // Turn director events into title cards, doors and rewards.
    void handleDirectorEvents() {
        for (auto& ev : director.events) {
            const Arena& ar = level.arenas[director.arena];
            char buf[128];
            switch (ev.kind) {
            case DirectorEvent::ARENA_START:
                lockDoor(ar.entryGate, true);   // no way back out mid-fight
                if (fast()) {
                    snprintf(buf, sizeof(buf), "ROOM %d/%d  %s", ev.value + 1, (int)level.arenas.size(), ar.name);
                    pushBanner(buf, ar.subtitle, {1.f, 0.7f, 0.3f}, 1.8f);
                    audio.play("wave", 90);
                } else {
                    snprintf(buf, sizeof(buf), "ARENA %d/%d", ev.value + 1, (int)level.arenas.size());
                    pushBanner(std::string(buf) + "  " + ar.name, ar.subtitle, {1.f, 0.78f, 0.3f}, 2.6f);
                    audio.play("wave");
                }
                break;
            case DirectorEvent::WAVE_START:
                if (fast()) { if (ev.value > 0) pushBanner("SECOND WAVE", "", {1.f, 0.5f, 0.3f}, 1.4f); break; }
                snprintf(buf, sizeof(buf), "WAVE %d/%d", ev.value + 1, director.waveCount());
                pushBanner(buf, "", {1.f, 0.9f, 0.4f}, 1.8f);
                audio.play("wave", 90);
                break;
            case DirectorEvent::BOSS_START:
                if (ar.waves[ev.value][0].type == EnemyType::SOVEREIGN)
                    pushBanner("THE SOVEREIGN", "PARRY (F) HIS BLADE AS IT FALLS", {1.f, 0.3f, 0.2f}, 4.f);
                else
                    pushBanner("THE WARDEN", "DODGE THE VOLLEYS, JUMP THE SLAMS", {1.f, 0.2f, 0.65f}, 3.5f);
                audio.play("wave"); audio.play("explosion", 70);
                shake(0.6f, 0.06f);
                break;
            case DirectorEvent::NEW_TYPE: {
                EnemyType t = (EnemyType)ev.value;
                if (isBoss(t)) break;
                if (fast()) ui.feed(std::string("NEW: ") + statsOf(t).name, statsOf(t).glow);
                else pushBanner(std::string("NEW: ") + statsOf(t).name, statsOf(t).hint, statsOf(t).glow, 3.4f);
                break;
            }
            case DirectorEvent::WAVE_CLEARED:
                if (!fast()) pushBanner("WAVE CLEAR", "", {0.4f, 1.f, 0.6f}, 1.6f);
                styleSystem.heal(10.f);
                break;
            case DirectorEvent::ARENA_CLEARED: {
                const Arena& done = level.arenas[ev.value];
                lockDoor(done.exitDoor, false);
                lockDoor(done.entryGate, false);
                if (fast()) {
                    recordSplit(ev.value);
                    styleSystem.heal(25.f);
                    grenadeCount = grenadeMax;
                    audio.play("split");
                    break;
                }
                if (done.exitDoor >= 0)
                    pushBanner("ARENA CLEARED", "THE GATE IS OPEN - HEAD NORTH", {0.4f, 1.f, 0.6f}, 3.5f);
                styleSystem.heal(40.f);
                grenadeCount = grenadeMax;
                audio.play("wave");
                break;
            }
            case DirectorEvent::VICTORY:
                victoryDelay = 2.5f;
                break;
            case DirectorEvent::FINISH_OPEN:
                finishOpen = true;
                pushBanner("FINISH OPEN", "REACH THE BEACON", {1.f, 0.6f, 0.2f}, 2.f);
                audio.play("wave");
                break;
            }
        }
        director.events.clear();
    }

    // FAST mode: note the clock at a section clear and compare with the best run
    void recordSplit(int section) {
        if ((int)splits.size() > section) splits.resize(section);
        splits.push_back(elapsedTime);
        char buf[96];
        if (section < (int)records.fastSplits.size()) {
            float d = elapsedTime - records.fastSplits[section];
            snprintf(buf, sizeof(buf), "ROOM %d  %s  %+.2f", section + 1, formatTime(elapsedTime).c_str(), d);
            ui.showSplit(buf, d <= 0.f ? glm::vec3{0.3f, 1.f, 0.5f} : glm::vec3{1.f, 0.4f, 0.35f});
        } else {
            snprintf(buf, sizeof(buf), "ROOM %d  %s", section + 1, formatTime(elapsedTime).c_str());
            ui.showSplit(buf, {1.f, 0.9f, 0.6f});
        }
    }

    void finishRun() {
        victory = true;
        captureMouse(false);
        newRecord = false;
        if (!ranked) return;
        board.load();
        worldLoaded = worldBoard.loadOnline();
        // Asked for a name when the run makes this browser's board, or (on the
        // web, with the shared board up) that one
        bool places = board.placeFor(fast(), elapsedTime) >= 0 ||
                      (worldLoaded && worldBoard.placeFor(fast(), elapsedTime) >= 0);
        if (places) { nameEntry = true; nameBuf = board.lastName; }
        if (fast()) {
            newRecord = records.bestFast <= 0.f || elapsedTime < records.bestFast;
            if (newRecord) {
                records.bestFast = elapsedTime; records.fastSplits = splits;
                ghostRec.save(); ghost = ghostRec;
            }
        } else {
            newRecord = records.bestArena <= 0.f || elapsedTime < records.bestArena;
            if (newRecord) records.bestArena = elapsedTime;
        }
        if (newRecord) records.save();
    }

    // --overlay poseN (screenshots): hold a SOVEREIGN in one pose, facing the camera.
    // 0 idle, 1 dash wind-up, 2 dashing, 3 sweep wind-up, 4 mid-sweep,
    // 5 cleave wind-up, 6 mid-cleave, 7 leaping, 8 broken, 9 enraged
    void devPose(Enemy& e) {
        int n = std::atoi(g_devOverlay.c_str() + 4);
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        glm::vec3 to = player.position - e.position;
        e.yaw = e.prevYaw = std::atan2(to.x, to.z);
        e.prevPosition = e.position;
        e.attack = AttackKind::NONE; e.telegraphTimer = 0.f; e.telegraphDuration = 1.f;
        e.dashTimer = e.leapTimer = e.swingTimer = e.staggerTimer = 0.f;
        auto windup = [&](AttackKind k) { e.attack = k; e.telegraphTimer = 0.3f; };
        switch (n) {
            case 1: windup(AttackKind::DASH); break;
            case 2: e.dashTimer = 0.3f; e.diveDir = glm::normalize(glm::vec3{to.x, 0.f, to.z}); e.moveSpeed = 34.f; break;
            case 3: windup(AttackKind::SWEEP); break;
            case 4: e.swingTimer = Enemy::SWING_TIME * 0.5f; e.lastSwing = AttackKind::SWEEP; break;
            case 5: windup(AttackKind::CLEAVE); break;
            case 6: e.swingTimer = Enemy::SWING_TIME * 0.4f; e.lastSwing = AttackKind::CLEAVE; break;
            case 7: e.leapTimer = 1.f; break;
            case 8: e.staggerTimer = 1.f; break;
            case 9: e.enraged = true; break;
            default: break;
        }
    }

    // Phase two of the Sovereign fight: the eclipse turns to blood and the
    // platforms orbiting the seal pick up speed
    float rageBlend = 0.f;   // 0..1, eases in once he enrages
    bool  sanctumRaged = false;
    static Theme bloodEclipse(const Theme& t) {
        Theme o = t;
        o.zenith = {0.05f, 0.0f, 0.0f}; o.horizon = {0.55f, 0.03f, 0.02f}; o.ground = {0.06f, 0.0f, 0.0f};
        o.sunColor = {2.2f, 0.45f, 0.25f}; o.mountain = {0.1f, 0.01f, 0.01f};
        o.lightColor = {1.3f, 0.5f, 0.4f}; o.skyAmb = {0.34f, 0.08f, 0.07f};
        o.fogColor = {0.22f, 0.02f, 0.02f}; o.fogDensity = t.fogDensity * 1.6f;
        return o;
    }
    void updateSanctumPhase(float dt) {
        bool raged = false;
        for (auto& e : enemies) if (e.alive && e.type == EnemyType::SOVEREIGN && e.enraged) raged = true;
        rageBlend = glm::clamp(rageBlend + (raged ? dt / 2.f : -dt / 3.f), 0.f, 1.f);
        if (raged == sanctumRaged) return;
        // Speed the orbiting platforms up (or back down after a retry) without
        // a jump: keep each one's current point on its path (offsetAt uses
        // t / period + phase)
        sanctumRaged = raged;
        const Arena& ar = level.arenas[director.arena];
        for (auto& m : level.movers) {
            if (m.path != Mover::Path::ORBIT) continue;
            glm::vec3 c = (m.base.min + m.base.max) * 0.5f;
            if (c.x < ar.zone.min.x || c.x > ar.zone.max.x || c.z < ar.zone.min.z || c.z > ar.zone.max.z) continue;
            float newPeriod = raged ? m.period * 0.55f : m.period / 0.55f;
            m.phase += moverClock / m.period - moverClock / newPeriod;
            m.period = newPeriod;
        }
    }

    // Footage camera (--cam, --campath, --autoaim, --kite; see the globals at the top)
    glm::vec3 devKite{0.f}, devKiteSafe{0.f};
    float devFireCd = 0.f;
    void devCamera(float dt) {
        float u = glm::clamp(g_recProgress, 0.f, 1.f);
        u = u * u * (3.f - 2.f * u);
        glm::vec3 p = g_devCamPath ? glm::mix(g_devCamPos, g_devCamPos2, u) : g_devCamPos;
        if (g_devKite) {
            // Keep 8 m from the nearest enemy, circling a little as a player would
            const Enemy* near = nullptr; float nd = 1e9f;
            for (auto& e : enemies)
                if (e.targetable()) { float d = glm::length(glm::vec2(e.position.x - p.x - devKite.x, e.position.z - p.z - devKite.z)); if (d < nd) { nd = d; near = &e; } }
            if (near && nd < 8.f && nd > 0.01f) {
                glm::vec3 away = p + devKite - near->position; away.y = 0.f; away = glm::normalize(away);
                devKite += (away * 9.f + glm::vec3{-away.z, 0.f, away.x} * 3.f) * dt;
            }
            const AABB& z = level.arenas[director.arena].zone;
            glm::vec3 q = p + devKite;
            q.x = glm::clamp(q.x, z.min.x + 3.f, z.max.x - 3.f); q.z = glm::clamp(q.z, z.min.z + 3.f, z.max.z - 3.f);
            AABB body{q - glm::vec3{0.6f, 1.6f, 0.6f}, q + glm::vec3{0.6f, 0.3f, 0.6f}};
            bool inWall = false;
            for (auto& wl : level.walls)
                if (body.max.x > wl.box.min.x && body.min.x < wl.box.max.x && body.max.y > wl.box.min.y &&
                    body.min.y < wl.box.max.y && body.max.z > wl.box.min.z && body.min.z < wl.box.max.z) { inWall = true; break; }
            if (inWall) q = p + devKiteSafe;   // back to the last spot that was clear
            devKite = devKiteSafe = q - p;
            p = q;
        }
        player.position = p - glm::vec3{0, player.eyeHeight, 0};
        player.camera.position = p;
        prevCamPos = p;
        if (g_devClean) { banners.clear(); controlHintTimer = 0.f; }
        if (!g_devAutoAim) {
            if (g_devCamPath) {
                player.camera.yaw = g_devCamYaw + std::remainder(g_devCamYaw2 - g_devCamYaw, 360.f) * u;
                player.camera.pitch = glm::mix(g_devCamPitch, g_devCamPitch2, u);
            }
            return;
        }
        // Nearest enemy in sight, in front of us first
        const Enemy* best = nullptr; float bestScore = 1e9f;
        glm::vec3 fwd = player.camera.forward();
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            glm::vec3 c = e.position + glm::vec3{0, e.height() * 0.6f, 0};
            glm::vec3 d = c - p;
            float dist = glm::length(d);
            if (dist > 45.f || dist < 0.5f) continue;
            bool clear = true;
            for (auto& wl : level.walls) { float t = rayBoxHit(p, d / dist, wl.box); if (t > 0.f && t < dist - 0.6f) { clear = false; break; } }
            if (!clear) continue;
            float score = dist * (1.6f - glm::dot(fwd, d / dist));
            if (score < bestScore) { bestScore = score; best = &e; }
        }
        if (!best) return;
        glm::vec3 d = best->position + glm::vec3{0, best->height() * 0.6f, 0} - p;
        float wantYaw = glm::degrees(std::atan2(d.z, d.x));
        float wantPitch = glm::degrees(std::asin(glm::clamp(d.y / glm::length(d), -1.f, 1.f)));
        float k = 1.f - std::exp(-7.f * dt);
        float dy = std::remainder(wantYaw - player.camera.yaw, 360.f), dp = wantPitch - player.camera.pitch;
        player.camera.yaw += dy * k; player.camera.pitch += dp * k;
        devFireCd -= dt;
        if (std::fabs(dy) < 3.f && std::fabs(dp) < 3.f && devFireCd <= 0.f) { pendingFire = true; devFireCd = 0.3f; }
    }

    // Practice (dev level select): F5 ends the current wave on the spot
    void devClearWave() {
        director.queue.clear();
        for (auto& e : enemies)
            if (e.alive) {
                e.alive = false; e.state = EnemyState::DEAD; e.health = 0.f;
                spawnDebrisFor(e);
            }
        ui.feed("DEV: WAVE CLEARED", {0.3f, 1.f, 0.8f});
    }

    // Victory screen, right-hand side: the name prompt (when this run makes
    // the board) and the mode's leaderboard
    void renderLeaderboardPanel() {
        float t = gameClock + (float)SDL_GetTicks() * 0.001f;
        float x = 926.f, w = 334.f, y = 110.f;
        ui.begin2D();
        UIBatch& b = ui.ui;
        if (!ranked) {
            b.text("PRACTICE RUN - NOT RANKED", x + w / 2, y + 8, 1, {0.7f, 0.7f, 0.75f, 0.85f}, true);
            y += 30.f;
        } else if (nameEntry) {
            b.rect(x, y, w, 112, {0.08f, 0.05f, 0.02f, 0.9f});
            b.frame(x, y, w, 112, 2, {1.f, 0.75f, 0.2f, 0.9f});
            char buf[64];
            int wp = worldLoaded ? worldBoard.placeFor(fast(), elapsedTime) : -1;
            if (wp >= 0) std::snprintf(buf, sizeof(buf), "YOU MADE THE WORLD BOARD - #%d", wp + 1);
            else         std::snprintf(buf, sizeof(buf), "YOU MADE THE BOARD - #%d", board.placeFor(fast(), elapsedTime) + 1);
            b.text(buf, x + w / 2, y + 12, 2, {1.f, 0.85f, 0.3f, 1.f}, true);
            b.rect(x + 20, y + 40, w - 40, 32, {0.f, 0.f, 0.f, 0.7f});
            std::string shown = nameBuf;
            if (std::fmod(t, 1.f) < 0.55f && (int)nameBuf.size() < Leaderboard::MAX_NAME_LEN) shown += "_";
            b.text(shown.empty() ? " " : shown.c_str(), x + 30, y + 48, 3, {1.f, 1.f, 1.f, 1.f});
            b.text("TYPE YOUR NAME   ENTER - SAVE   ESC - SKIP", x + w / 2, y + 88, 1, {0.8f, 0.75f, 0.65f, 0.9f}, true);
            y += 128.f;
        } else if (boardPlace >= 0) {
            b.text("SAVED TO THE LEADERBOARD", x + w / 2, y + 8, 2, {1.f, 0.85f, 0.3f, 0.95f}, true);
            y += 34.f;
        }
        // The shared board when there is one (re-read every second: the post
        // and the refresh land asynchronously), else this browser's
        if ((worldPoll -= 1.f / 60.f) <= 0.f) { worldPoll = 1.f; worldLoaded = worldBoard.loadOnline(); }
        if (worldLoaded) {
            int hi = -1;
            const auto& l = worldBoard.list(fast());
            for (int i = 0; i < (int)l.size(); ++i)
                if (boardPlace >= 0 && std::fabs(l[i].time - elapsedTime) < 0.02f && l[i].name == board.lastName) hi = i;
            b.text("WORLD", x + 8, y + 14, 1, {0.4f, 0.9f, 1.f, 0.9f});
            drawLeaderboardTable(b, worldBoard, fast(), x, y, w, nameEntry ? 8 : Leaderboard::KEEP, hi, t);
        } else {
            drawLeaderboardTable(b, board, fast(), x, y, w, nameEntry ? 8 : Leaderboard::KEEP, boardPlace < 99 ? boardPlace : -1, t);
        }
        ui.end2D();
    }

    // A controller button press (held buttons are read as keys; see Gamepad.h)
    void padButton(Uint8 b) {
        auto key = [&](SDL_Keycode k) { if (k != SDLK_UNKNOWN) { SDL_Event e = gamepad::keyEvent(k); handleEvent(e); } };
        if (victory && nameEntry) {   // no on-screen keyboard: A saves the name as it stands, B skips
            key(b == SDL_CONTROLLER_BUTTON_A || b == SDL_CONTROLLER_BUTTON_START ? SDLK_RETURN
              : b == SDL_CONTROLLER_BUTTON_B ? SDLK_ESCAPE : SDLK_UNKNOWN);
            return;
        }
        if (playerDead) {             // A retries, X starts over, B to the menu
            key(b == SDL_CONTROLLER_BUTTON_A ? SDLK_r : b == SDL_CONTROLLER_BUTTON_X ? SDLK_RETURN
              : b == SDL_CONTROLLER_BUTTON_B ? SDLK_ESCAPE : SDLK_UNKNOWN);
            return;
        }
        if (victory) { key(b == SDL_CONTROLLER_BUTTON_A ? SDLK_RETURN : b == SDL_CONTROLLER_BUTTON_B ? SDLK_ESCAPE : SDLK_UNKNOWN); return; }
        if (armoryOpen) { key(b == SDL_CONTROLLER_BUTTON_BACK ? SDLK_TAB : gamepad::menuKey(b)); return; }
        if (paused) { key(gamepad::menuKey(b)); return; }
        switch (b) {
            case SDL_CONTROLLER_BUTTON_START:         key(SDLK_ESCAPE); break;
            case SDL_CONTROLLER_BUTTON_BACK:          key(SDLK_TAB); break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  pendingGrapple = true; break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:       pendingGrenade = true; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     key(SDLK_v); break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     trySwitch((activeWeapon + WEAPON_COUNT - 1) % WEAPON_COUNT); break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    trySwitch((activeWeapon + 1) % WEAPON_COUNT); break;
            default: break;
        }
    }

    // Web: post the run to the shared board (web/index.html does the request;
    // the name is already restricted to A-Z 0-9 space - . _)
    void submitWorld(const std::string& name, int diff) {
#ifdef __EMSCRIPTEN__
        char js[160];
        std::snprintf(js, sizeof(js), "window.overdriveBoard&&window.overdriveBoard.submit('%s','%s',%.2f,%d)",
                      fast() ? "fast" : "arena", name.c_str(), elapsedTime, diff);
        emscripten_run_script(js);
#else
        (void)name; (void)diff;
#endif
    }

    // Victory screen: type a name (letters, digits, space - . _), ENTER saves
    // it to the leaderboard, ESC skips
    void handleNameEntry(const SDL_Event& e) {
        if (e.type != SDL_KEYDOWN) return;
        SDL_Keycode k = e.key.keysym.sym;
        if (k == SDLK_BACKSPACE) { if (!nameBuf.empty()) nameBuf.pop_back(); return; }
        if (e.key.repeat) return;
        if (k == SDLK_ESCAPE) { nameEntry = false; return; }
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
            int diff = settings ? settings->difficulty : DIFFICULTY_DEFAULT;
            std::string name = Leaderboard::cleanName(nameBuf);
            boardPlace = board.add(fast(), nameBuf, elapsedTime, diff);
            if (boardPlace >= 0) board.save();
            if (!name.empty()) {
                audio.play("upgrade");
                submitWorld(name, diff);
                if (boardPlace < 0) boardPlace = 99;   // saved to the shared board only
            }
            nameEntry = false;
            return;
        }
        char c = 0;
        if (k >= SDLK_a && k <= SDLK_z) c = (char)('A' + (k - SDLK_a));
        else if (k >= SDLK_0 && k <= SDLK_9) c = (char)('0' + (k - SDLK_0));
        else if (k >= SDLK_KP_1 && k <= SDLK_KP_9) c = (char)('1' + (k - SDLK_KP_1));
        else if (k == SDLK_KP_0) c = '0';
        else if (k == SDLK_SPACE) c = ' ';
        else if (k == SDLK_MINUS) c = '-';
        else if (k == SDLK_PERIOD) c = '.';
        if (c && (int)nameBuf.size() < Leaderboard::MAX_NAME_LEN && !(c == ' ' && nameBuf.empty())) nameBuf += c;
    }

    void shake(float t, float amount) {
        float s = settings ? settings->screenShake : 1.f;
        if (s <= 0.f) return;
        shakeTimer = std::max(shakeTimer, t);
        shakeIntensity = std::max(shakeIntensity, amount * s);
    }

    // =========================================================================
    // Input
    // =========================================================================
    void handleEvent(const SDL_Event& e) override {
        // Gameplay keys act once per press; held-key auto-repeat would toggle
        // the grapple, armory or pause on and off. (Menus below read `e`
        // directly, where repeat is wanted for arrows and sliders.)
        SDL_Keycode key = (e.type == SDL_KEYDOWN && !e.key.repeat) ? e.key.keysym.sym : SDLK_UNKNOWN;

        if (e.type == SDL_CONTROLLERBUTTONDOWN) { padButton(e.cbutton.button); return; }

        // Alt-tabbing away mid-fight pauses (the web build pauses on losing the pointer lock)
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && !g_devNoMouse) { pause(); return; }

        // Armory (TAB) — a paused overlay for spending upgrade points
        if (armoryOpen) {
            if (key == SDLK_TAB || key == SDLK_ESCAPE || key == SDLK_p) { closeArmory(); return; }
            handleArmoryEvent(e);
            return;
        }
        if (paused && pauseSettings) { settingsMenu.handleEvent(e); return; }
        if (victory && nameEntry) { handleNameEntry(e); return; }

        if (key == SDLK_ESCAPE || key == SDLK_p) {
            if (playerDead || victory) {
                if (key != SDLK_ESCAPE) return;  // P only pauses
                captureMouse(false);
                if (onReturnToMenu) onReturnToMenu();
                return;
            }
            paused = !paused;
            pauseSelected = 0;
            captureMouse(!paused);
            return;
        }
        if (paused) { handlePauseEvent(e); return; }
        if (g_practice && !playerDead && !victory) {
            if (key == SDLK_F5) { devClearWave(); return; }    // practice: skip the fight in progress
            if (key == SDLK_F6) { styleSystem.heal(1000.f); ui.feed("HEALTH REFILLED", {0.4f, 1.f, 0.6f}); return; }
        }
        if (key == SDLK_TAB && !playerDead && !victory) { openArmory(); return; }
        // Backspace: straight back to the checkpoint and the start of this fight
        if (key == SDLK_BACKSPACE && !victory) { if (playerDead) retryArena(); else restartHere(); return; }
        if (e.type == SDL_KEYDOWN && (playerDead || victory)) {
            if (key == SDLK_r && playerDead) { retryArena(); return; }
            if (key == SDLK_RETURN)          { newRun();     return; }
            return;
        }
        if (key == SDLK_e) {
            int idx = findInteractTarget();
            if (idx >= 0 && interactables[idx].onInteract) interactables[idx].onInteract();
        }
        // Grapple on its own button (Q by default, see Settings). Right mouse is
        // only ever aim (rifles), so scoping and grappling never fight over it.
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) pendingFire = true;
        {
            GameSettings def;
            const GameSettings& gs = settings ? *settings : def;
            if (key != SDLK_UNKNOWN && gs.isGrappleEvent(0, (int)key)) pendingGrapple = true;
            if (e.type == SDL_MOUSEBUTTONDOWN && gs.isGrappleEvent(1, e.button.button)) pendingGrapple = true;
        }
        if (e.type == SDL_MOUSEWHEEL && e.wheel.y != 0)
            trySwitch((activeWeapon + (e.wheel.y > 0 ? WEAPON_COUNT - 1 : 1)) % WEAPON_COUNT);
        if (key >= SDLK_1 && key <= SDLK_4) trySwitch(key - SDLK_1);
        if (key == SDLK_g) pendingGrenade = true;
        if (key == SDLK_v && pendingWeapon < 0 && !weapons[activeWeapon].reloading) viewModel.triggerInspect();
        if (e.type == SDL_MOUSEMOTION && !g_devNoMouse) {
            float dx = (float)e.motion.xrel, dy = (float)e.motion.yrel;
            mouseFilter.enabled = settings ? settings->mouseFilter : true;
            if (!mouseFilter.accept(dx, dy)) return;
            float sens = (settings ? settings->sensitivity : 0.1f) * aimSensScale();
            if (settings && settings->invertY) dy = -dy;
            player.applyMouseLook(dx, dy, sens);
        }
    }

    // The weapon in hand (or about to be, mid-switch)
    WeaponId heldWeapon() const { return (WeaponId)(pendingWeapon >= 0 ? pendingWeapon : activeWeapon); }

    void trySwitch(int w) {
        if (w != activeWeapon && pendingWeapon < 0) {
            pendingWeapon = w;
            weaponSwitchTimer = 0.15f;
            viewModel.triggerSwitch();
            audio.play("reload", 80);
        }
    }

    // While zoomed, scale mouse look by how much the view narrowed so the
    // crosshair tracks the same distance on screen, then by the user's setting.
    float aimSensScale() const {
        const WeaponDef& d = weaponDef((WeaponId)activeWeapon);
        if (!d.canAim || aim <= 0.f) return 1.f;
        float base = settings ? settings->fov : 90.f;
        float zoomed = base * d.aimFov;
        float ratio = std::tan(glm::radians(zoomed) * 0.5f) / std::tan(glm::radians(base) * 0.5f);
        float k = (settings ? settings->zoomSens : 1.f) * ratio;
        return glm::mix(1.f, k, aim);
    }

    // Pause a live run (no-op on death/win screens, in the armory, or if already paused).
    void pause() {
        if (paused || playerDead || victory || armoryOpen) return;
        paused = true;
        pauseSelected = 0;
        captureMouse(false);
    }

    const char* pauseLabel(int i) const {
        static const char* L[] = {"RESUME", "SETTINGS", "", "", "", "QUIT TO MENU"};
        if (i == 2) return fullscreenOn() ? "EXIT FULLSCREEN" : "FULLSCREEN";
        if (i == 3) return fast() ? "RESTART ROOM" : "RESTART ARENA";
        if (i == 4) return fast() ? "RESTART RUN" : "NEW RUN";
        return L[i];
    }

    // Fullscreen from the pause menu. Desktop: the window (main.cpp applies the
    // setting). Web: the page, through web/index.html, which asks the embedding
    // portfolio page when there is one. In a browser this is the easy way out
    // of fullscreen: Escape belongs to the game there (see index.html).
    bool fullscreenOn() const {
#ifdef __EMSCRIPTEN__
        return EM_ASM_INT({ return window.overdriveFullscreen ? (window.overdriveFullscreen.isOn() ? 1 : 0) : 0; }) != 0;
#else
        return settings && settings->fullscreen;
#endif
    }
    void toggleFullscreen() {
#ifdef __EMSCRIPTEN__
        EM_ASM({ if (window.overdriveFullscreen) window.overdriveFullscreen.toggle(); });
#else
        if (settings) { settings->fullscreen = !settings->fullscreen; settings->save(); }
#endif
    }

    void activatePauseItem(int idx) {
        switch (idx) {
        case 0: paused = false; captureMouse(true); break;
        case 1: pauseSettings = true; settingsMenu.selected = 1; break;
        case 2: toggleFullscreen(); break;
        case 3: restartHere(); break;
        case 4: newRun(); break;
        default:
            captureMouse(false);
            if (onReturnToMenu) onReturnToMenu();
            break;
        }
    }

    int pauseItemAt(int mx, int my) const {
        for (int i = 0; i < UIRenderer::PAUSE_ITEMS; ++i) {
            int by = ui.pauseButtonY(i);
            if (mx > SCREEN_W/2-150 && mx < SCREEN_W/2+150 && my > by && my < by+46) return i;
        }
        return -1;
    }

    void handlePauseEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:   pauseSelected = (pauseSelected + UIRenderer::PAUSE_ITEMS - 1) % UIRenderer::PAUSE_ITEMS; break;
                case SDLK_DOWN: pauseSelected = (pauseSelected + 1) % UIRenderer::PAUSE_ITEMS; break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    activatePauseItem(pauseSelected); break;
                default: break;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int i = pauseItemAt(e.button.x, e.button.y);
            if (i >= 0) activatePauseItem(i);
        }
        if (e.type == SDL_MOUSEMOTION) {
            int i = pauseItemAt(e.motion.x, e.motion.y);
            if (i >= 0) pauseSelected = i;
        }
    }

    void openArmory() {
        armoryOpen = true;
        captureMouse(false);
        // Start on the gun in hand
        armoryW = activeWeapon;
    }
    void closeArmory() {
        armoryOpen = false;
        captureMouse(true);
    }

    void buyUpgrade(int w, int s) {
        WeaponId wid = (WeaponId)w;
        int oldMag = weaponMag(wid, prog.up[w]);
        if (!prog.buy(wid, (UpgradeStat)s)) { audio.play("telegraph", 50); return; }
        // A bigger magazine is topped up straight away
        int newMag = weaponMag(wid, prog.up[w]);
        if (newMag > oldMag && !weapons[w].reloading) weapons[w].ammo += newMag - oldMag;
        audio.play("upgrade");
    }

    void handleArmoryEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:  armoryW = (armoryW + WEAPON_COUNT - 1) % WEAPON_COUNT; break;
                case SDLK_RIGHT: armoryW = (armoryW + 1) % WEAPON_COUNT; break;
                case SDLK_UP:    armoryS = (armoryS + UPGRADE_STATS - 1) % UPGRADE_STATS; break;
                case SDLK_DOWN:  armoryS = (armoryS + 1) % UPGRADE_STATS; break;
                case SDLK_RETURN:
                case SDLK_SPACE: buyUpgrade(armoryW, armoryS); break;
                default: break;
            }
        }
        int w, s;
        if (e.type == SDL_MOUSEMOTION && ui.armoryCellAt(e.motion.x, e.motion.y, w, s)) { armoryW = w; armoryS = s; }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT &&
            ui.armoryCellAt(e.button.x, e.button.y, w, s)) {
            armoryW = w; armoryS = s;
            buyUpgrade(w, s);
        }
    }

    // =========================================================================
    // Per-frame update
    // =========================================================================
    // The soundtrack follows the fight: a track per area, layers by what's
    // happening (MusicSynth.h), muffled under the pause menu
    void updateMusic() {
        auto& m = audio.music;
        audio.setMusicVolume(settings ? settings->musicVolume : 0.6f);
        static const int ARENA_TRACK[] = {0, 1, 2, 3, 4};       // Yard, Foundry, Spire, Core, Sanctum
        static const int FAST_TRACK[]  = {0, 1, 2, 2, 1, 1, 3}; // Canal .. Tower
        int a = director.arena;
        m.setTrack(fast() ? FAST_TRACK[a % 7] : ARENA_TRACK[a % 5]);
        float lv = 0.6f;
        switch (director.phase) {
            case WaveDirector::Phase::ACTIVE:   lv = director.bossWave() ? 1.35f : 1.f; break;
            case WaveDirector::Phase::BREAK:    lv = 0.7f; break;
            case WaveDirector::Phase::CLEARED:  lv = 0.5f; break;
            case WaveDirector::Phase::VICTORY:  lv = fast() ? 1.2f : 0.55f; break;
            default: break;
        }
        if (lv >= 1.f && styleSystem.overdrive) lv = 1.35f;
        if (countdown > 0.f) lv = 0.45f;
        if (victory) lv = 0.55f;
        if (playerDead) lv = 0.1f;
        m.setIntensity(lv);
        m.setMuffle(paused || armoryOpen || playerDead);
    }

    void update(float dt) override {
        if (settings) audio.masterVolume = settings->audioVolume;
        updateMusic();

        // Banners keep animating on the death/victory screens, not while paused
        bool frozen = paused || armoryOpen;
        if (!banners.empty() && !frozen) {
            banners.front().time += dt;
            if (banners.front().time >= banners.front().duration) banners.pop_front();
        }
        if (!frozen) ui.update(dt, styleSystem);
        if (frozen || playerDead || victory || countdown > 0.f) {
            // Keep the physics clock current so resuming doesn't replay the
            // whole paused interval as a burst of catch-up ticks.
            prevTicks   = SDL_GetPerformanceCounter();
            accumulator = 0.0;
            if (playerDead) deadTimer += dt;
            if (countdown > 0.f && !frozen) {
                float before = countdown;
                countdown -= dt;
                if (std::ceil(before) != std::ceil(countdown)) audio.play(countdown <= 0.f ? "wave" : "telegraph", 90);
                if (countdown <= 0.f) countdown = 0.f;
                handleDirectorEvents();
            }
            if (playerDead) gameClock += dt;
            return;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed = g_fixedDt > 0.0 ? g_fixedDt : (double)(now - prevTicks) / (double)freq;
        prevTicks = now;
        if (g_devCam && g_devNoMouse) devCamera((float)elapsed);
        accumulator += elapsed;
        if (accumulator > 0.25) accumulator = 0.25;

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        // A controller holds keys too (Gamepad.h): fold it in
        static Uint8 merged[SDL_NUM_SCANCODES];
        gamepad::poll();
        if (gamepad::state().pad) {
            const gamepad::State& gp = gamepad::state();
            for (int i = 0; i < SDL_NUM_SCANCODES; ++i) merged[i] = keys[i] | gp.held[i];
            keys = merged;
            if (gp.fire && !gp.firePrev) pendingFire = true;
            if (gp.look.x != 0.f || gp.look.y != 0.f) {
                // Full deflection turns 240 degrees a second at the default sensitivity
                float speed = 240.f * (settings ? settings->sensitivity / 0.1f : 1.f) * (float)elapsed;
                float dy = gp.look.y * speed;
                if (settings && settings->invertY) dy = -dy;
                player.applyMouseLook(gp.look.x * speed, dy, aimSensScale());
            }
        }
        static Uint8 noKeys[SDL_NUM_SCANCODES] = {};
        if (g_devNoMouse) keys = noKeys;   // screenshot runs: the real keyboard doesn't reach the game
        bool parryKey = keys[SDL_SCANCODE_F] != 0;

        while (accumulator >= PHYSICS_DT) {
            prevCamPos = player.camera.position;
            snapshotForInterpolation();
            if (hitStopFrames > 0) {
                --hitStopFrames;  // freeze simulation, consume one tick
            } else {
                physicsTick(PHYSICS_DT, keys, parryKey);
            }
            accumulator -= PHYSICS_DT;
        }

        float floatDt = (float)elapsed;
        gameClock += floatDt;
        styleSystem.update(floatDt);
        if (controlHintTimer > 0.f) controlHintTimer -= floatDt;

        // Doors part as you run at them (unless locked) and shut behind you
        level.updateDoors(floatDt, player.position, [this](int di, bool opening) {
            const AABB& c = level.doors[di].closed;
            float d = glm::length((c.min + c.max) * 0.5f - player.camera.position);
            audio.play(opening ? "door" : "door_close", (int)glm::clamp(120.f - d * 3.f, 20.f, 120.f));
        });

        viewModel.update(floatDt, playerXZSpeed, player.onGround);
        // --overlay reloadNN: freeze the gun NN% through its reload (screenshots)
        if (g_devOverlay.rfind("reload", 0) == 0) {
            float u = std::atoi(g_devOverlay.c_str() + 6) / 100.f;
            viewModel.anim = ViewAnim::RELOAD; viewModel.animMax = 1.f; viewModel.animTimer = std::max(0.001f, 1.f - u);
            viewModel.reloadShells = 2;
        }
        if (g_devOverlay.rfind("inspect", 0) == 0) {   // --overlay inspectNN: frozen NN% through it
            float u = std::atoi(g_devOverlay.c_str() + 7) / 100.f;
            viewModel.anim = ViewAnim::INSPECT; viewModel.animMax = 2.2f; viewModel.animTimer = std::max(0.001f, (1.f - u) * 2.2f);
        }
        if (g_devOverlay == "punch") { viewModel.parryTimer = ViewModel::PARRY_TIME * 0.62f; viewModel.parryHit = true; }
        // Baseline FOV widens with horizontal speed on top of the dash kick
        float speedKick = glm::clamp((playerXZSpeed - 7.f) / 15.f, 0.f, 1.f) * 6.f;
        fovKick = glm::mix(fovKick, speedKick, std::min(1.f, floatDt * 7.f));
        landSquash = glm::mix(landSquash, 0.f, std::min(1.f, floatDt * 10.f));

        if (pendingWeapon >= 0) {
            weaponSwitchTimer -= floatDt;
            if (weaponSwitchTimer <= 0.f) {
                activeWeapon  = pendingWeapon;
                pendingWeapon = -1;
            }
        }
        if (boltSoundTimer > 0.f) {
            boltSoundTimer -= floatDt;
            if (boltSoundTimer <= 0.f) audio.play("bolt", 110);
        }

        elapsedTime += floatDt;
        if (styleSystem.style > peakStyle) peakStyle = styleSystem.style;

        updateParticles(floatDt);
        spawnAmbientParticles(floatDt);
        for (auto& p : projSystem.pool) {           // projectile trails
            if (!p.alive) continue;
            Particle* q = freeParticle();
            if (!q) break;
            q->pos = p.position; q->vel = -p.velocity * 0.05f;
            q->color = p.emissiveColor * 0.8f; q->gravity = 0.f;
            q->maxLife = q->life = 0.22f; q->alive = true;
        }
        updateDebris(floatDt);
        for (auto& s : shockwaves) s.t += floatDt;
        for (auto& s : slashes) s.t += floatDt;
        slashes.erase(std::remove_if(slashes.begin(), slashes.end(),
                      [](const Slash& s){ return s.t > SLASH_LIFE; }), slashes.end());
        shockwaves.erase(std::remove_if(shockwaves.begin(), shockwaves.end(),
                         [](const Shockwave& s){ return s.t > 0.5f; }), shockwaves.end());

        // Point light 0: explosion or muzzle flash. 1..3: nearest enemy shots.
        if (explosionFlashTimer > 0.f) {
            explosionFlashTimer -= floatDt;
            float t = glm::clamp(explosionFlashTimer / 0.35f, 0.f, 1.f);
            pointLightPos[0]   = explosionFlashPos;
            pointLightColor[0] = glm::vec3{1.6f, 0.9f, 0.4f} * t;
        } else if (muzzleFlashTimer > 0.f) {
            muzzleFlashTimer -= floatDt;
            pointLightPos[0]   = muzzleFlashPos;
            pointLightColor[0] = {1.f,0.7f,0.2f};
        } else {
            pointLightColor[0] = {0,0,0};
        }
        int lIdx = 1;
        for (auto& p : projSystem.pool) {
            if (!p.alive || p.isPlayer) continue;
            if (lIdx >= MAX_POINT_LIGHTS) break;
            pointLightPos[lIdx]   = p.position;
            pointLightColor[lIdx] = p.emissiveColor * 2.f;
            ++lIdx;
        }
        for (;lIdx<MAX_POINT_LIGHTS;++lIdx) pointLightColor[lIdx]={0,0,0};

        fpsFrameCount++;
        fpsTimer += floatDt;
        if (fpsTimer >= 1.0f) {
            ui.currentFPS   = fpsFrameCount;
            fpsFrameCount   = 0;
            fpsTimer        = 0.f;
        }
        ui.showFPS = settings ? settings->showFPS : false;

        handleDirectorEvents();

        if (victoryDelay > 0.f) {
            victoryDelay -= floatDt;
            if (victoryDelay <= 0.f) finishRun();
        }
        if (!styleSystem.isAlive() && !playerDead) {
            playerDead = true;
            deadTimer = 0.f;
            grapple.release();
        }
    }

    // Everything that moves on the physics tick remembers where it was at the
    // start of it; render() draws it blended toward where it is now, so it
    // glides at 144 or 240 Hz instead of stepping at 60. (Called during
    // hit-stop too, so frozen things stay still rather than flicker.)
    void snapshotForInterpolation() {
        for (auto& e : enemies) { e.prevPosition = e.position; e.prevYaw = e.yaw; }
        for (auto& p : projSystem.pool) if (p.alive) p.prevPosition = p.position;
        for (auto& p : pickups) p.prev = p.pos;
        for (auto& m : level.movers) m.delta = glm::vec3{0.f};
    }
    float renderAlpha = 1.f;   // set by render(): how far between the last two ticks we are
    glm::vec3 lerpPos(glm::vec3 prev, glm::vec3 now) const { return glm::mix(prev, now, renderAlpha); }

    // =========================================================================
    // Fixed-rate simulation (60 Hz)
    // =========================================================================
    void physicsTick(float dt, const Uint8* keys, bool parryKey) {
        spawnSoundThisTick = false;

        // --- Moving platforms: move them, then carry whoever stands on one ---
        int rideMover = level.moverOfWall(player.groundWall);
        moverClock += dt;
        level.updateMovers(moverClock);
        if (rideMover >= 0) {
            player.position += level.movers[rideMover].delta;
            player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
        }
        if (grapple.active && grapple.moverWall >= 0) grapple.follow(level.walls[grapple.moverWall].box);

        // --- Dash ---
        bool dashKey = keys[SDL_SCANCODE_LSHIFT] != 0;
        if (dashKey && !prevDashKey && dashCharges > 0) {
            // Full 3D dash in the direction the camera faces
            glm::vec3 dashDir = player.camera.forward();
            player.velocity = dashDir * 28.f;
            dashMomentumTimer = 0.30f;
            --dashCharges;
            dashCooldown = 1.2f;
            fovKick = 13.f;
            invincFrames = std::max(invincFrames, 0.15f);   // a sliver of i-frames rewards well-timed dodges
            styleSystem.addStyle(8.f);
            audio.play("dash");
        }
        prevDashKey = dashKey;
        if (dashMomentumTimer > 0.f) dashMomentumTimer -= dt;

        if (dashCooldown > 0.f) {
            dashCooldown -= dt;
            if (dashCooldown <= 0.f && dashCharges < 2) {
                ++dashCharges;
                dashCooldown = dashCharges < 2 ? 1.2f : 0.f;
            }
        }

        // --- Double jump ---
        bool jumpKey = keys[SDL_SCANCODE_SPACE] != 0;
        bool justLanded = player.onGround && !prevOnGround;
        if (justLanded) {
            jumpsRemaining = 1;
            if (!slamming && peakFallSpeed > 4.f)
                landSquash = glm::clamp(peakFallSpeed / 22.f, 0.f, 1.f) * 0.22f;
            peakFallSpeed = 0.f;
            if (slamming) {
                for (auto& e : enemies) {
                    if (!e.targetable()) continue;
                    if (glm::length(e.position - player.position) < 4.5f)
                        hurtEnemy(e, 30.f, e.position + glm::vec3{0, e.height() * 0.5f, 0}, 15.f, 2.f);
                }
                spawnShockwave(player.position, 4.5f, {1.f, 0.8f, 0.4f});
                shake(0.3f, 0.08f);
                audio.play("slam");
                slamming = false;
            }
        }
        prevOnGround = player.onGround;

        if (jumpKey && !prevJumpKey) {
            if (grapple.active) {
                // Slingshot: release grapple mid-swing and keep all momentum + upward kick.
                grapple.release();
                player.velocity.y += 6.f;
                styleSystem.addStyle(10.f);
                audio.play("jump");
            } else if (jumpsRemaining > 0 && !player.onGround && player.coyoteTimer <= 0.f) {
                player.velocity.y = player.jumpForce;
                --jumpsRemaining;
                styleSystem.addStyle(3.f);
                audio.play("jump");
            }
        }
        prevJumpKey = jumpKey;

        // --- Slam ---
        bool crouchKey = (keys[SDL_SCANCODE_LCTRL] != 0) || (keys[SDL_SCANCODE_C] != 0);
        if (crouchKey && !player.onGround && !slamming && player.velocity.y < 0.f) {
            player.velocity.y = -40.f;
            slamming = true;
        }

        // --- Grapple fire / release ---
        glm::vec3 gPoint; int gWall = -1; bool gMover = false;
        bool canHook = findGrappleTarget(gPoint, gWall, gMover);
        grappleTargetInSight = canHook && gMover;
        if (pendingGrapple) {
            if (!grapple.active) {
                if (canHook) {
                    glm::vec3 impulse{0.f};
                    grapple.attach(player.camera.position, gPoint, impulse, gWall,
                                   gMover ? &level.walls[gWall].box : nullptr);
                    player.velocity = impulse;
                    viewModel.triggerGrapple();
                    audio.play("grapple_fire");
                    if (gMover) styleSystem.addStyle(4.f);
                }
            } else {
                grapple.release();
            }
            pendingGrapple = false;
        }
        grapple.update(dt, player.camera.position, player.velocity);

        // --- Aiming (rifles, RMB held) ---
        {
            const WeaponDef& d = weaponDef((WeaponId)activeWeapon);
            const WeaponState& ws = weapons[activeWeapon];
            bool rmb = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || gamepad::state().aim;
            bool want = d.canAim && rmb && pendingWeapon < 0 && !ws.reloading;
            float before = aim;
            if (g_devAim) want = d.canAim;
            if (want) aim = std::min(1.f, aim + dt / d.aimTime);
            else      aim = std::max(0.f, aim - dt / (d.canAim ? d.aimTime * 0.6f : 0.05f));
            if (before < 0.02f && aim >= 0.02f && d.scope) audio.play("scope", 70);
            if (before < 0.98f && aim >= 0.98f) aimFullAt = gameClock;
            player.horizontalSpeed = 7.f * glm::mix(1.f, d.aimMoveMult, aim);
        }

        // --- Player physics ---
        player.dynWalls = level.moverWalls.data();
        player.dynCount = (int)level.moverWalls.size();
        int boost = level.boosterAt(player.position);
        if (!(g_devCam && g_devNoMouse)) {   // screenshot runs: the camera stays exactly where it was put
            player.update(dt, keys, level.walls.data(), (int)level.walls.size(),
                          grapple.active || dashMomentumTimer > 0.f || boost >= 0, &spatialGrid);
            if (boost >= 0) applyBooster(level.boosters[boost], player, dt);
            keepPlayerInZone(dt);
        }
        if (boost >= 0 && boostPrev < 0) {
            audio.play("boost", 100);
            fovKick = std::max(fovKick, 10.f);
            styleSystem.addStyle(3.f);
            grapple.release();
        }
        if (boost < 0 && boostPrev >= 0) dashMomentumTimer = std::max(dashMomentumTimer, 0.35f);   // shoot out of the tube
        boostPrev = boost;
        pushPlayerOutOfEnemies();
        playerXZSpeed = glm::length(glm::vec2(player.velocity.x, player.velocity.z));
        if (!player.onGround) peakFallSpeed = std::max(peakFallSpeed, -player.velocity.y);

        // --- Jump pads ---
        padCooldown = std::max(0.f, padCooldown - dt);
        for (auto& pad : level.pads) {
            glm::vec3 p = player.position;
            if (padCooldown <= 0.f && player.velocity.y <= 0.5f && p.y < pad.centre.y + 0.4f && p.y > pad.centre.y - 0.6f &&
                std::fabs(p.x - pad.centre.x) < pad.half.x && std::fabs(p.z - pad.centre.z) < pad.half.y) {
                player.velocity = pad.launch;
                player.onGround = false;
                jumpsRemaining = 1;
                padCooldown = 0.4f;
                grapple.release();
                styleSystem.addStyle(4.f);
                audio.play("dash", 90);
                spawnBurst(pad.centre + glm::vec3{0, 0.2f, 0}, {0.3f, 1.f, 0.8f}, 18, 4.f, 0.5f, -4.f);
            }
        }

        // --- Lava ---
        hazardTick = std::max(0.f, hazardTick - dt);
        for (auto& hz : level.hazards) {
            glm::vec3 p = player.position;
            if (p.y < hz.box.max.y + 0.3f && p.y > hz.box.min.y - 0.5f && p.x > hz.box.min.x && p.x < hz.box.max.x &&
                p.z > hz.box.min.z && p.z < hz.box.max.z) {
                if (!g_godMode && director.phase != WaveDirector::Phase::VICTORY)
                    styleSystem.health = std::max(0.f, styleSystem.health - hz.dps * dt);
                if (hazardTick <= 0.f) {
                    hazardTick = 0.35f;
                    ui.onDamage();
                    audio.play("player_hit", 60);
                    spawnBurst(p + glm::vec3{0, 0.2f, 0}, {1.f, 0.45f, 0.1f}, 8, 3.f, 0.5f, -3.f);
                }
            }
        }

        // --- Footsteps / landing ---
        if (player.onGround && playerXZSpeed > 1.5f && !player.sliding) {
            footstepTimer -= dt;
            if (footstepTimer <= 0.f) {
                footstepTimer = glm::clamp(0.55f / (playerXZSpeed / 5.f), 0.2f, 0.5f);
                static const char* STEPS[] = {"step1", "step2", "step3", "step4"};
                audio.play(STEPS[rand() % 4], 55);
            }
        } else {
            footstepTimer = 0.f;
        }
        if (justLanded && !slamming) audio.play("land", 70);

        // --- Weapons ---
        grenadeTimer = std::max(0.f, grenadeTimer - dt);
        invincFrames = std::max(0.f, invincFrames - dt);
        for (int w = 0; w < WEAPON_COUNT; ++w)
            if (weapons[w].tick(dt, weaponMag((WeaponId)w, prog.up[w])) && w == activeWeapon && w >= 2)
                audio.play("bolt", 90);   // rifles chamber a round when the reload finishes

        reloadSounds();
        WeaponState& ws = weapons[activeWeapon];
        int mag = weaponMag((WeaponId)activeWeapon, prog.up[activeWeapon]);
        if (keys[SDL_SCANCODE_R] && !ws.reloading && ws.ammo < mag && pendingWeapon < 0) startReload(activeWeapon);
        // Empty: reload once the last shot's cycle is done
        if (ws.ammo <= 0 && !ws.reloading && ws.cooldown <= 0.f && pendingWeapon < 0) startReload(activeWeapon);

        bool switchBlocked = (pendingWeapon >= 0);
        if (pendingFire && ws.ready() && !switchBlocked) fireWeapon(activeWeapon);
        pendingFire = false;
        if (pendingGrenade && grenadeCount > 0 && grenadeTimer <= 0.f) throwGrenade();
        pendingGrenade = false;

        if (recoilPitch > 0.f) {
            float applied = std::min(14.f * dt, recoilPitch);
            player.camera.pitch -= applied;
            recoilPitch -= applied;
            player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);
        }

        // --- Waves ---
        int alive = 0;
        for (auto& e : enemies) if (e.alive) ++alive;
        std::vector<SpawnRequest> spawns;
        director.countScale    = tune().waveSize;
        director.maxAliveBonus = tune().maxAliveBonus;
        director.update(dt, alive, player.position, spawns);
        for (auto& s : spawns) spawnEnemy(s.type, s.pos);

        // --- Enemies ---
        updateEnemies(dt);
        updateSanctumPhase(dt);
        if (fast() && countdown <= 0.f && !victory) ghostRec.record(elapsedTime, player.position, player.camera.yaw);

        // --- Projectiles ---
        auto result = projSystem.update(dt, level.walls.data(), (int)level.walls.size(),
                                        enemies, player.camera.position, &spatialGrid);

        bool parryPressed = parryKey && !parryPrev;
        parryPrev = parryKey;
        punchCooldown = std::max(0.f, punchCooldown - dt);
        if (parryPressed && punchCooldown <= 0.f) punch(result.boostableIndex);

        for (auto& [pi, ei] : result.enemyHits) {
            auto& e = enemies[ei];
            const Projectile& pr = projSystem.pool[pi];
            if (pr.parried && pr.heavy) {
                spawnExplosionParticles(pr.position, 3.f);
                audio.play("explosion", 100);
                shake(0.3f, 0.07f);
            }
            hurtEnemy(e, pr.damage, pr.position, pr.parried ? 25.f : 10.f, 2.f, false, pr.parried);
        }
        for (auto& exp : result.explosions)
            pendingBlasts.push_back({exp.pos, exp.radius, exp.damage, exp.radius * 0.5f, 20.f});
        processBlasts();

        if (result.hitPlayer) damagePlayer(result.playerDamage, result.playerHitFrom, 0.2f, 0.04f);

        updatePickups(dt);

        // --- FAST: the finish beacon ---
        if (finishOpen && !victory) {
            glm::vec3 d = player.position - level.finishPos;
            if (glm::length(glm::vec2(d.x, d.z)) < 3.f && std::fabs(d.y) < 4.f) {
                finishOpen = false;
                spawnBurst(level.finishPos + glm::vec3{0, 1.f, 0}, {1.f, 0.6f, 0.2f}, 60, 10.f, 1.2f, -2.f);
                audio.play("wave"); audio.play("split");
                finishRun();
            }
        }

        if (shakeTimer > 0.f) shakeTimer -= dt;
        if (ceilingFxTimer > 0.f) ceilingFxTimer -= dt;
        nearInteractable = findInteractTarget() >= 0;
        for (auto& t : tracers) if (t.alive) { t.life -= dt; if (t.life <= 0.f) t.alive = false; }
        for (auto& d : decals)  if (d.alive) { d.life -= dt; if (d.life <= 0.f) d.alive = false; }

        // Clear out dead enemies once nothing references them by index
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                      [](const Enemy& e){ return !e.alive; }), enemies.end());
    }

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
    int findParryTarget() const {
        glm::vec3 eye = player.camera.position, fwd = player.camera.forward();
        int best = -1; float bestD = 1e9f;
        for (int i = 0; i < ProjectileSystem::POOL_SIZE; ++i) {
            const Projectile& p = projSystem.pool[i];
            if (!p.alive || p.isPlayer) continue;
            glm::vec3 d = p.position - eye;
            float dist = glm::length(d);
            float reach = std::max(2.5f, 1.f + glm::length(p.velocity) * 0.22f) + p.size * 0.3f;
            if (dist < reach && dist > 1e-3f && glm::dot(fwd, d / dist) > 0.25f && dist < bestD) { bestD = dist; best = i; }
        }
        return best;
    }

    void parryFeedback(glm::vec3 at, bool heavy) {
        spawnBurst(at, {1.f, 0.85f, 0.35f}, heavy ? 50 : 28, heavy ? 12.f : 8.f, 0.45f, 4.f);
        spawnBurst(at, {1.f, 1.f, 1.f}, 10, 4.f, 0.2f, 0.f);
        explosionFlashTimer = 0.2f; explosionFlashPos = at;
        audio.play("clank");
        audio.play("parry", 80);
        ui.onParry();
        viewModel.triggerParry(true);
        invincFrames  = std::max(invincFrames, 0.5f);
        hitStopFrames = glm::max(hitStopFrames, heavy ? 9 : 4);
        shake(heavy ? 0.25f : 0.12f, heavy ? 0.06f : 0.03f);
    }

    void punch(int boostable) {
        punchCooldown = 0.3f;
        glm::vec3 eye = player.camera.position, fwd = player.camera.forward();

        int pi = findParryTarget();
        if (pi >= 0) {
            Projectile& p = projSystem.pool[pi];
            float speed = std::max(40.f, glm::length(p.velocity) * 2.f);
            p.isPlayer = true;
            p.parried  = true;
            p.velocity = fwd * speed;                        // it goes where you look
            p.lifetime = 4.f;
            if (p.heavy) {
                p.damage = 400.f; p.size *= 1.3f; p.emissiveColor = {1.6f, 1.1f, 0.3f};
                styleSystem.addStyle(60.f);
                ui.toast("HEAVY PARRY", "", {1.f, 0.8f, 0.2f}, 1.2f);
            } else {
                p.damage = 60.f; p.emissiveColor = {1.f, 0.9f, 0.3f};
                styleSystem.addStyle(25.f);
                ui.feed("PARRY", {1.f, 0.9f, 0.3f});
            }
            parryFeedback(p.position, p.heavy);
            return;
        }
        for (auto& e : enemies) {
            if (!e.targetable() || !e.parryWindow()) continue;
            glm::vec3 d = e.position + glm::vec3{0, e.height() * 0.5f, 0} - eye;
            float dist = glm::length(d);
            if (dist < 5.5f && glm::dot(fwd, d / dist) > 0.2f) {
                e.stagger(e.staggerTime());
                parryFeedback(eye + fwd * 1.2f, true);
                styleSystem.addStyle(70.f);
                gainXp(30);
                if (e.type == EnemyType::SOVEREIGN) ui.toast("PARRIED", "HIS GUARD IS BROKEN - UNLOAD", {1.f, 0.75f, 0.2f}, 1.4f);
                else if (e.type == EnemyType::SHIELDBEARER) ui.toast("SHIELD DOWN", "", {0.4f, 1.f, 0.75f}, 1.2f);
                else ui.toast("BROKEN", "IT TAKES DOUBLE DAMAGE - UNLOAD", {1.f, 0.75f, 0.2f}, 1.8f);
                return;
            }
        }
        if (boostable >= 0) {
            // Projectile boost: detonate your own projectile for a massive explosion.
            auto& p = projSystem.pool[boostable];
            float r = glm::max(p.blastRadius, 4.f) * 2.f;
            pendingBlasts.push_back({p.position, r, p.damage * 3.f, 0.f, 0.f});
            p.alive = false;
            styleSystem.addStyle(40.f);
            invincFrames  = 0.6f;
            hitStopFrames = glm::max(hitStopFrames, 2);
            audio.play("parry");
            viewModel.triggerParry(true);
            ui.feed("PROJECTILE BOOST", {1.f, 0.6f, 0.2f});
            return;
        }
        viewModel.triggerParry(false);
        audio.play("punch", 90);
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.2f), 0};
            glm::vec3 d = c - eye;
            float dist = glm::length(d) - e.radius();
            if (dist < 2.4f && glm::dot(fwd, glm::normalize(d)) > 0.5f) {
                hurtEnemy(e, 25.f, c, 6.f, 1.f);
                glm::vec3 push = glm::normalize(glm::vec3{d.x, 0.f, d.z}) * (e.type == EnemyType::JUGGERNAUT ? 0.3f : 1.4f);
                e.position += push;
                audio.play("clank", 50);
                shake(0.08f, 0.02f);
                break;
            }
        }
    }

    // Where would the grapple hook if fired now? Static walls are hit exactly;
    // moving platforms get a generous 1.2 m of aim assist and pull you to just
    // above their top, so hooking one lands you on it.
    bool findGrappleTarget(glm::vec3& point, int& wall, bool& isMover) const {
        glm::vec3 o = player.camera.position, d = player.camera.forward();
        float best = grapple.maxLength;
        wall = -1; isMover = false;
        for (int i = 0; i < (int)level.walls.size(); ++i) {
            if (level.walls[i].dynamic) continue;
            float t = rayBoxHit(o, d, level.walls[i].box);
            if (t > 0.f && t < best) { best = t; wall = i; }
        }
        float bestMover = best + 0.5f;
        int moverWall = -1;
        for (int w : level.moverWalls) {
            AABB b = level.walls[w].box;
            b.min -= glm::vec3{1.2f}; b.max += glm::vec3{1.2f};
            float t = rayBoxHit(o, d, b);
            if (t > 0.f && t < bestMover) { bestMover = t; moverWall = w; }
        }
        if (moverWall >= 0) {
            const AABB& b = level.walls[moverWall].box;
            point = {(b.min.x + b.max.x) * 0.5f, b.max.y + 1.2f, (b.min.z + b.max.z) * 0.5f};
            wall = moverWall; isMover = true;
            return true;
        }
        if (wall >= 0) { point = o + d * best; return true; }
        return false;
    }

    // The player may only be inside the arena being fought — plus, once it's
    // cleared, the corridor and the next arena (FAST: every section reached so
    // far). Walls are low enough to stand on, so this (not wall height) is
    // what stops you leaving the map. Each zone's max.y is an invisible
    // ceiling, and falling below an arena's voidY returns you to its start.
    void keepPlayerInZone(float dt) {
        static std::vector<const AABB*> zones;
        zones.clear();
        auto addArena = [&](int i) {
            zones.push_back(&level.arenas[i].zone);
            for (auto& z : level.arenas[i].extraZones) zones.push_back(&z);
        };
        if (fast()) {
            for (int i = 0; i <= director.arena; ++i) addArena(i);
        } else {
            addArena(director.arena);
            if (director.phase == WaveDirector::Phase::CLEARED) {
                zones.push_back(&level.corridors[director.arena]);
                addArena(director.arena + 1);
            }
        }
        glm::vec3& p = player.position;
        float bestD = 1e9f; glm::vec2 best{p.x, p.z};
        const AABB* in = zones[0];
        for (auto* z : zones) {
            glm::vec2 c{glm::clamp(p.x, z->min.x, z->max.x), glm::clamp(p.z, z->min.z, z->max.z)};
            float d = glm::length(c - glm::vec2{p.x, p.z});
            // Inside several (a doorway where a tube meets a room): the higher ceiling wins
            if (d < bestD - 1e-4f || (d < 1e-4f && bestD < 1e-4f && z->max.y > in->max.y)) { bestD = d; best = c; in = z; }
        }
        if (bestD > 0.f) {
            if (best.x != p.x) player.velocity.x = 0.f;
            if (best.y != p.z) player.velocity.z = 0.f;
            p.x = best.x; p.z = best.y;
        }
        // Ceiling: a force field you can see when you bump it
        float ceiling = in->max.y;
        if (p.y + player.height > ceiling) {
            p.y = ceiling - player.height;
            if (player.velocity.y > 0.f) player.velocity.y = -1.f;
            if (grapple.active && grapple.target.y > ceiling - 1.f) grapple.release();
            if (ceilingFxTimer <= 0.f) {
                ceilingFxTimer = 0.35f;
                for (int i = 0; i < 26; ++i) {
                    Particle* q = freeParticle();
                    if (!q) break;
                    float a = i * 6.2832f / 26.f;
                    q->pos = glm::vec3{p.x, ceiling, p.z} + glm::vec3{std::cos(a), 0.f, std::sin(a)} * 0.6f;
                    q->vel = glm::vec3{std::cos(a), 0.f, std::sin(a)} * 5.f;
                    q->color = glm::vec3{0.3f, 0.85f, 1.f} * 1.3f;
                    q->gravity = 0.f; q->maxLife = q->life = 0.5f; q->alive = true;
                }
                audio.play("barrier", 70);
            }
        }
        player.camera.position = p + glm::vec3{0, player.eyeHeight, 0};

        // Void: back to the start of the section you fell out of
        int a = level.arenaAt(p);
        if (a >= 0 && p.y < level.arenas[a].voidY) {
            resetToCheckpoint(a);
            if (!g_godMode) styleSystem.takeDamage(15.f);
            ui.onDamage();
            ui.feed("FELL - BACK TO THE LEDGE", {1.f, 0.5f, 0.3f});
            audio.play("player_hit");
        }
        (void)dt;
    }

    void resetToCheckpoint(int a) {
        const Arena& ar = level.arenas[a];
        player.position = ar.hasRespawn ? ar.respawn : ar.playerStart;
        player.velocity = glm::vec3{0.f};
        player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
        prevCamPos = player.camera.position;
        grapple.release();
        slamming = false; peakFallSpeed = 0.f;
    }

    // Ground enemies are solid: you can't walk through a Brute.
    void pushPlayerOutOfEnemies() {
        for (auto& e : enemies) {
            if (!e.targetable() || e.stats().flying) continue;
            glm::vec3 p = player.position;
            if (p.y > e.position.y + e.height() || p.y + player.height < e.position.y) continue;
            glm::vec2 d{p.x - e.position.x, p.z - e.position.z};
            float minD = e.radius() + player.radius;
            float len = glm::length(d);
            if (len < minD && len > 1e-3f) {
                glm::vec2 push = d / len * (minD - len);
                player.position.x += push.x; player.position.z += push.y;
                player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0};
            }
        }
    }

    // Highest walkable surface under (x, z) at or below fromY (0 = the floor)
    float groundHeightAt(float x, float z, float fromY) const {
        static std::vector<int> cands;
        AABB q{{x - 0.05f, -1.f, z - 0.05f}, {x + 0.05f, fromY + 0.5f, z + 0.05f}};
        spatialGrid.query(q, cands);
        float best = 0.f;
        for (int i : cands) {
            const AABB& b = level.walls[i].box;
            if (x >= b.min.x && x <= b.max.x && z >= b.min.z && z <= b.max.z && b.max.y <= fromY + 0.5f)
                best = std::max(best, b.max.y);
        }
        return best;
    }

    void spawnEnemy(EnemyType t, glm::vec3 pos) {
        enemies.push_back(Enemy(t, pos));
        enemies.back().maxHealth *= tune().health;
        enemies.back().health = enemies.back().maxHealth;
        glm::vec3 c = statsOf(t).glow;
        spawnBurst(pos + glm::vec3{0, 0.3f, 0}, c, 14, 3.f, 0.7f, -6.f);
        if (spawnSoundThisTick) return;   // a FAST section spawns a dozen at once: one sound
        spawnSoundThisTick = true;
        float d = glm::length(pos - player.position);
        audio.play("spawn", (int)glm::clamp(110.f - d * 2.f, 25.f, 110.f));
    }

    void updateEnemies(float dt) {
        telegraphSoundCd -= dt;
        shieldClankCd -= dt;
        const Arena& ar = level.arenas[director.arena];
        EnemyWorld w;
        w.playerEye  = player.camera.position;
        w.playerFeet = player.position;
        w.walls      = level.walls.data();
        w.wallCount  = (int)level.walls.size();
        w.grid       = &spatialGrid;
        w.bounds     = ar.bounds;
        w.playerVel  = player.velocity;
        w.tune       = &tune();
        const float dmgScale = ar.damageScale * tune().damage;

        // Iterate by index: summons push_back into `enemies` mid-loop
        size_t n = enemies.size();
        for (size_t i = 0; i < n; ++i) {
            Enemy& e = enemies[i];
            if (!e.alive) continue;
            if (g_devOverlay.rfind("pose", 0) == 0 && e.type == EnemyType::SOVEREIGN) { devPose(e); continue; }
            e.update(dt, w);
            const EnemyEvents ev = e.ev;   // copy: spawning below may reallocate
            glm::vec3 epos = e.position;
            float dist = glm::length(epos - player.position);

            // Wind-up tick: a cue for the ones close enough to matter, at most
            // a few a second however many are aiming at you
            if (ev.telegraphStarted && dist < 30.f && telegraphSoundCd <= 0.f) {
                audio.play("telegraph", (int)glm::clamp(70.f - dist * 2.f, 12.f, 70.f));
                telegraphSoundCd = 0.22f;
            }
            for (int k = 0; k < ev.shots; ++k)
                projSystem.fire(ev.shotOrigin, ev.shotDir[k] * ev.shotSpeed, ev.shotDamage * dmgScale, false,
                                enemies[i].stats().shotColor, false, 0.f, ev.shotSize, ev.shotHeavy);
            if (ev.meleeHit) {
                if (damagePlayer(ev.meleeDamage * dmgScale, epos, 0.25f, 0.06f)) {
                    glm::vec3 away = player.position - epos; away.y = 0.f;
                    if (glm::length(away) > 0.001f)
                        player.velocity += glm::normalize(away) * 8.f + glm::vec3{0, 3.f, 0};
                }
            }
            if (ev.slam) {
                spawnShockwave(epos, ev.slamRadius, statsOf(enemies[i].type).glow);
                shake(0.35f, 0.07f);
                audio.play("slam");
                glm::vec2 flat{player.position.x - epos.x, player.position.z - epos.z};
                bool grounded = player.position.y < epos.y + 0.9f && player.position.y > epos.y - 1.5f;   // jump it to dodge
                if (ev.slamDamage > 0.f && glm::length(flat) < ev.slamRadius && grounded) {
                    if (damagePlayer(ev.slamDamage * dmgScale, epos, 0.3f, 0.08f) && glm::length(flat) > 0.01f)
                        player.velocity += glm::vec3{flat.x, 0.f, flat.y} / glm::length(flat) * 10.f + glm::vec3{0, 6.f, 0};
                }
            }
            if (ev.detonated) {
                // A mite that reached you: hurts you AND its friends
                pendingBlasts.push_back({epos + glm::vec3{0, 0.3f, 0}, 4.f, 30.f, 4.f, 30.f * dmgScale});
                spawnDebrisFor(enemies[i]);
            }
            if (ev.summonMites + ev.summonRippers > 0) {
                int total = ev.summonMites + ev.summonRippers;
                for (int k = 0; k < total; ++k) {
                    float a = k * 6.2832f / total + gameClock;
                    glm::vec3 p = epos + glm::vec3{std::cos(a) * 3.5f, 0.f, std::sin(a) * 3.5f};
                    p.x = glm::clamp(p.x, ar.bounds.min.x + 1.f, ar.bounds.max.x - 1.f);
                    p.z = glm::clamp(p.z, ar.bounds.min.z + 1.f, ar.bounds.max.z - 1.f);
                    p.y = epos.y;
                    spawnEnemy(k < ev.summonMites ? EnemyType::MITE : EnemyType::RIPPER, p);
                }
            }
            if (ev.slash >= 0) {   // a sword stroke: its arc, and a whoosh
                slashes.push_back({epos, enemies[i].yaw, ev.slash, 0.f});
                audio.play("dash", ev.slash == 2 ? 120 : 95);
            }
            if (ev.dashStarted) { audio.play("dash", 128); shake(0.12f, 0.03f); }
            if (ev.leapStarted) { audio.play("jump", 128); spawnShockwave(epos, 3.f, statsOf(enemies[i].type).glow); }
            if (ev.enraged) {
                pushBanner(enemies[i].type == EnemyType::SOVEREIGN ? "THE SOVEREIGN IS ENRAGED" : "THE WARDEN IS ENRAGED",
                           "", {1.f, 0.15f, 0.25f}, 2.f);
                shake(0.5f, 0.06f);
                audio.play("wave");
            }
        }

        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            // Fell into the void: counts as your kill
            if (e.position.y < ar.voidY) {
                e.alive = false; e.state = EnemyState::DEAD; e.health = 0.f;
                ui.feed(std::string(e.stats().name) + " FELL", {1.f, 0.7f, 0.3f});
                styleSystem.addStyle(15.f);
                onEnemyKilled(e);
                continue;
            }
            // Lava burns enemies too: lure them in
            if (e.stats().flying) continue;
            for (auto& hz : level.hazards)
                if (e.position.y < hz.box.max.y + 0.3f && e.position.y > hz.box.min.y - 0.5f &&
                    e.position.x > hz.box.min.x && e.position.x < hz.box.max.x &&
                    e.position.z > hz.box.min.z && e.position.z < hz.box.max.z) {
                    if (e.takeDamage(hz.dps * 0.5f * dt)) onEnemyKilled(e);
                }
        }

        // Soft separation so squads don't stack inside each other
        for (size_t i = 0; i < enemies.size(); ++i) {
            for (size_t j = i + 1; j < enemies.size(); ++j) {
                Enemy& a = enemies[i]; Enemy& b = enemies[j];
                if (!a.alive || !b.alive || a.stats().flying != b.stats().flying) continue;
                glm::vec3 d = b.position - a.position;
                if (!a.stats().flying) { if (std::fabs(d.y) > 1.5f) continue; d.y = 0.f; }
                float len = glm::length(d), minD = a.radius() + b.radius();
                if (len >= minD || len < 1e-4f) continue;
                glm::vec3 n = d / len * (minD - len);
                float wa = b.radius() / minD, wb = a.radius() / minD;   // big ones get pushed less
                a.position -= n * wa; b.position += n * wb;
            }
        }
    }

    // Returns true if damage was applied (not blocked by i-frames).
    bool damagePlayer(float dmg, glm::vec3 from, float shakeT, float shakeAmt) {
        if (invincFrames > 0.f || playerDead || g_godMode || victory ||
            (!fast() && director.phase == WaveDirector::Phase::VICTORY)) return false;
        styleSystem.takeDamage(dmg);
        ui.onDamage();
        showDamageFrom(from);
        shake(shakeT, shakeAmt);
        audio.play("player_hit");
        invincFrames = 0.35f;
        grapple.release();
        return true;
    }

    void showDamageFrom(glm::vec3 source) {
        glm::vec3 toSrc = source - player.camera.position;
        float angle = atan2f(toSrc.x, toSrc.z) - glm::radians(player.camera.yaw + 90.f);
        ui.onDamageFrom(angle);
    }

    // All player damage to enemies goes through here. Returns true on a kill.
    // pierceArmor: parried shots go straight through a Juggernaut's plating.
    bool hurtEnemy(Enemy& e, float dmg, glm::vec3 at, float style, float heal, bool crit = false, bool pierceArmor = false) {
        if (!e.targetable()) return false;
        if (!pierceArmor) dmg *= e.armorMult();
        float before = e.health;
        bool killed = e.takeDamage(dmg);
        styleSystem.addStyle(style);
        styleSystem.heal(heal * tune().heal);
        audio.play("hit");
        if (!e.stats().flying) spawnDecal(e.position);
        spawnHitSparks(at, e.stats().color * 1.4f);
        ui.onHit(killed, crit);
        if (!settings || settings->damageNumbers) ui.spawnDamageNumber(at, std::min(dmg, before), crit);
        if (killed) {
            onEnemyKilled(e);
            hitStopFrames = glm::max(hitStopFrames, isBoss(e.type) ? 12 : 2);
        }
        return killed;
    }

    void onEnemyKilled(Enemy& e) {
        ++totalKills;
        styleSystem.addStyle(30.f);
        styleSystem.heal(5.f * tune().heal);
        audio.play("enemy_death");
        spawnDeathParticles(e.position + glm::vec3{0, e.height() * 0.5f, 0}, e.stats().color);
        spawnDebrisFor(e);
        if (styleSystem.overdrive) dashCharges = 2;
        if (++killsThisCycle >= 2) {
            killsThisCycle = 0;
            if (grenadeCount < grenadeMax) { grenadeCount++; ui.onGrenadeRefill(); }
        }

        // XP, scaled by how stylishly you're playing
        int xp = (int)std::round(xpForKill(e.type) * styleXpMultiplier(styleSystem.getRank()));
        gainXp(xp);
        char buf[64];
        snprintf(buf, sizeof(buf), "%s  +%d XP", e.stats().name, xp);
        ui.feed(buf, {0.8f, 0.85f, 0.9f});

        // Drops. Health orbs are common and small; potions (18%) and XP shards
        // (10%) are the rare ones, and get a loot beam so you notice them.
        glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.5f), 0};
        float floorY = groundHeightAt(e.position.x, e.position.z, e.position.y + 0.5f);
        auto drop = [&](PickupKind k) {
            pickups.push_back({c, glm::vec3{frand(-3.f, 3.f), frand(3.f, 6.f), frand(-3.f, 3.f)},
                               k == PickupKind::ORB ? 20.f : 30.f, k, floorY, c});
        };
        switch (e.type) {
            case EnemyType::BRUTE:  for (int i = 0; i < 3; ++i) drop(PickupKind::ORB);
                                    if (rand() % 100 < 50) drop(PickupKind::POTION); break;
            case EnemyType::MITE:   if (rand() % 10 == 0) drop(PickupKind::ORB); break;
            case EnemyType::WARDEN: case EnemyType::SOVEREIGN: break;
            default:
                if (rand() % 100 < (int)(20 * tune().drops)) drop(PickupKind::ORB);
                if (rand() % 100 < (int)(18 * tune().drops)) drop(PickupKind::POTION);
                if (rand() % 100 < 10) drop(PickupKind::XP);
                break;
        }

        if (e.type == EnemyType::MITE)          // shot mites still pop — but only hurt enemies
            pendingBlasts.push_back({e.position + glm::vec3{0, 0.3f, 0}, 4.f, 30.f, 0.f, 0.f});

        if (isBoss(e.type)) {
            // The boss takes his summons with him
            for (auto& o : enemies)
                if (o.alive && &o != &e) {
                    o.alive = false; o.state = EnemyState::DEAD;
                    spawnDebrisFor(o);
                    spawnDeathParticles(o.position + glm::vec3{0, o.height() * 0.5f, 0}, o.stats().color);
                }
            for (int k = 0; k < 4; ++k)
                spawnExplosionParticles(e.position + glm::vec3{frand(-1.5f,1.5f), frand(1.f,4.f), frand(-1.5f,1.5f)}, 4.f);
            explosionFlashTimer = 0.35f; explosionFlashPos = e.position + glm::vec3{0, 2.f, 0};
            shake(1.0f, 0.12f);
            audio.play("explosion");
            if (e.type == EnemyType::SOVEREIGN) {
                for (int k = 0; k < 6; ++k)
                    spawnBurst(e.position + glm::vec3{frand(-1.f, 1.f), frand(0.5f, 3.5f), frand(-1.f, 1.f)},
                               {1.f, 0.8f, 0.4f}, 30, 10.f, 0.8f, 2.f);
                pushBanner("THE SOVEREIGN HAS FALLEN", "", {1.f, 0.85f, 0.3f}, 3.f);
            } else {
                pushBanner("WARDEN DESTROYED", "", {1.f, 0.85f, 0.3f}, 2.5f);
            }
        }
    }

    void gainXp(int xp) {
        int lv = prog.addXp(xp);
        if (lv > 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "LEVEL %d", prog.level);
            ui.toast(buf, "UPGRADE READY - PRESS TAB", {0.4f, 0.9f, 1.f}, 2.6f);
            audio.play("levelup");
            styleSystem.heal(15.f);
        }
    }

    // Explosions resolve in a loop so a blast can kill a mite whose blast
    // kills another mite… (capped, in case of a giant chain).
    void processBlasts() {
        for (int guard = 0; !pendingBlasts.empty() && guard < 64; ++guard) {
            Blast b = pendingBlasts.back();
            pendingBlasts.pop_back();
            spawnExplosionParticles(b.pos, b.radius);
            shake(0.3f, 0.06f);
            explosionFlashTimer = 0.35f; explosionFlashPos = b.pos;
            audio.play("explosion");
            for (auto& e : enemies) {
                if (!e.targetable()) continue;
                float d = glm::length(e.position + glm::vec3{0, e.height() * 0.5f, 0} - b.pos);
                if (d < b.radius)
                    hurtEnemy(e, b.damage * (1.f - d / b.radius), e.position + glm::vec3{0, e.height() * 0.6f, 0}, 15.f, 3.f);
            }
            if (b.playerDamage > 0.f) {
                float pd = glm::length(player.camera.position - b.pos);
                if (pd < b.playerRadius)
                    damagePlayer(b.playerDamage * (1.f - pd / b.playerRadius) + 2.f, b.pos, 0.3f, 0.07f);
            }
        }
        pendingBlasts.clear();
    }

    void updatePickups(float dt) {
        glm::vec3 chest = player.position + glm::vec3{0, 0.9f, 0};
        bool hurt = styleSystem.health < styleSystem.maxHealth - 0.5f;
        for (auto& p : pickups) {
            glm::vec3 to = chest - p.pos;
            float d = glm::length(to);
            float magnet = p.kind == PickupKind::ORB ? 5.f : p.kind == PickupKind::XP ? 4.f : (hurt ? 2.5f : 0.f);
            if (d < magnet && d > 1e-3f) {
                p.vel = glm::mix(p.vel, to / d * 16.f, std::min(1.f, dt * 8.f));
            } else {
                p.vel.y -= 20.f * dt;
                p.vel.x *= std::pow(0.2f, dt); p.vel.z *= std::pow(0.2f, dt);
            }
            p.pos += p.vel * dt;
            float rest = p.floorY + (p.kind == PickupKind::ORB ? 0.4f : 0.55f);
            if (p.pos.y < rest) { p.pos.y = rest; p.vel.y = std::fabs(p.vel.y) * 0.3f; }
            p.life -= dt;
            if (d < 1.2f) {
                switch (p.kind) {
                case PickupKind::ORB:
                    p.life = 0.f;
                    styleSystem.heal(12.f);
                    audio.play("pickup");
                    spawnBurst(p.pos, {0.3f, 1.f, 0.5f}, 8, 2.f, 0.4f, 0.f);
                    break;
                case PickupKind::POTION:
                    if (!hurt) break;   // saved for when you need it
                    p.life = 0.f;
                    styleSystem.heal(40.f);
                    audio.play("potion");
                    ui.feed("+40 HP  HEALTH POTION", {1.f, 0.35f, 0.4f});
                    spawnBurst(p.pos, {1.f, 0.25f, 0.35f}, 20, 3.f, 0.6f, -2.f);
                    break;
                case PickupKind::XP: {
                    p.life = 0.f;
                    int xp = 40 + 5 * prog.level;
                    gainXp(xp);
                    char buf[32]; snprintf(buf, sizeof(buf), "+%d XP  SHARD", xp);
                    ui.feed(buf, {0.6f, 0.5f, 1.f});
                    audio.play("pickup");
                    spawnBurst(p.pos, {0.6f, 0.4f, 1.f}, 16, 3.f, 0.5f, -2.f);
                    break;
                }
                }
            }
        }
        pickups.erase(std::remove_if(pickups.begin(), pickups.end(),
                      [](const Pickup& p){ return p.life <= 0.f; }), pickups.end());
    }

    // =========================================================================
    // Weapons
    // =========================================================================

    // Every enemy along a ray up to the first wall, nearest first.
    struct RayHit { int enemy; float t; bool head; };
    float hitscanAll(glm::vec3 origin, glm::vec3 dir, float range, std::vector<RayHit>& out) {
        out.clear();
        float wallT = range;
        for (auto& w : level.walls) {
            float t = rayBoxHit(origin, dir, w.box);
            if (t > 0.f && t < wallT) wallT = t;
        }
        for (int ei = 0; ei < (int)enemies.size(); ++ei) {
            if (!enemies[ei].targetable()) continue;
            // A ray through the head box is a headshot, and the head counts
            // even where it pokes out of the body box
            // Test against where it was drawn, not where it has got to since
            const Enemy& en = enemies[ei];
            glm::vec3 off = en.hasShown ? en.shownPos - en.position : glm::vec3{0.f};
            if (glm::dot(off, off) > 9.f) off = glm::vec3{0.f};   // teleported: trust the simulation
            AABB body = en.getAABB();
            body.min += off; body.max += off;
            float t = rayBoxHit(origin, dir, body);
            AABB head;
            bool hasHead = headBox(en, head);
            head.min += off; head.max += off;
            float th = hasHead ? rayBoxHit(origin, dir, head) : -1.f;
            if (th > 0.f && th < wallT) out.push_back({ei, th, true});
            else if (t > 0.f && t < wallT) out.push_back({ei, t, false});
        }
        std::sort(out.begin(), out.end(), [](const RayHit& a, const RayHit& b) { return a.t < b.t; });
        return wallT;
    }

    // Sounds timed to the reload animations (ViewModel.h): the revolver's
    // cylinder out, brass, speedloader, snap shut; each shotgun shell, the rack
    float reloadCueAt = 0.f;   // reload progress already cued
    void reloadSounds() {
        const WeaponState& ws = weapons[activeWeapon];
        if (!ws.reloading || activeWeapon > 1 || pendingWeapon >= 0) return;
        float now = ws.reloadProgress(), before = reloadCueAt;
        reloadCueAt = now;
        auto cue = [&](float at, const char* snd, int vol) { if (before < at && now >= at) audio.play(snd, vol); };
        if (activeWeapon == 0) {
            cue(0.04f, "cyl_open", 110); cue(0.2f, "eject", 100); cue(0.5f, "shell_in", 120); cue(0.78f, "cyl_close", 120);
        } else {
            int n = viewModel.reloadShells;
            for (int i = 0; i < n; ++i) cue(0.13f + (i + 0.55f) * 0.6f / n, "shell_in", 110);
            cue(0.86f, "pump", 120);
        }
    }

    void startReload(int w) {
        WeaponId id = (WeaponId)w;
        weapons[w].startReload(weaponReload(id, prog.up[w]));
        // The revolver and shotgun play their reload over its whole length; the rifles swing
        int mag = weaponMag(id, prog.up[w]);
        if (w == activeWeapon) viewModel.triggerReload(w <= 1 ? weapons[w].reloadTotal : weapons[w].reloadTotal * 0.6f,
                                                       mag - weapons[w].ammo);
        reloadCueAt = 0.f;
        if (w >= 2) audio.play("reload");
    }

    void fireWeapon(int w) {
        WeaponId id = (WeaponId)w;
        const WeaponDef& d = weaponDef(id);
        const WeaponUpgrades& u = prog.up[w];
        WeaponState& ws = weapons[w];
        --ws.ammo;
        ws.cooldown = weaponCooldown(id, u);

        glm::vec3 origin = player.camera.position;
        glm::vec3 fwd    = player.camera.forward();
        glm::vec3 right  = player.camera.right();
        glm::vec3 up     = glm::cross(right, fwd);

        float aimNow = d.canAim ? aim : 0.f;
        bool  quick  = id == WeaponId::LONGSHOT && aimNow >= 0.98f && aimFullAt >= 0.f && gameClock - aimFullAt < 0.4f;
        bool  noscope = d.canAim && aimNow < 0.15f;
        float spread = weaponSpread(id, u, aimNow);
        // Rifles are wild in the air unless aimed
        if (d.canAim && !player.onGround) spread += 0.03f * (1.f - aimNow);
        int   pellets = weaponPellets(id, u);
        int   pierce  = weaponPierce(id, u);
        float dmg     = weaponDamage(id, u);
        bool  sniper  = id == WeaponId::KAR || id == WeaponId::LONGSHOT;

        bool anyHit = false, headKill = false, kills = 0;
        int  killCount = 0;
        static std::vector<RayHit> hits;
        for (int p = 0; p < pellets; ++p) {
            // Uniform within a disc of radius `spread`
            float ang = frand(0.f, 6.2832f), rad = std::sqrt(frand(0.f, 1.f)) * spread;
            glm::vec3 dir = glm::normalize(fwd + right * (std::cos(ang) * rad) + up * (std::sin(ang) * rad));
            float wallT = hitscanAll(origin, dir, d.range, hits);
            int n = std::min((int)hits.size(), pierce + 1);
            float endT = n > 0 && n == pierce + 1 ? hits[n - 1].t : wallT;
            spawnTracer(origin + dir * 0.25f - up * 0.08f, origin + dir * endT, sniper ? 0.09f : 0.055f, sniper ? 0.35f : 0.22f);
            for (int k = 0; k < n; ++k) {
                Enemy& e = enemies[hits[k].enemy];
                glm::vec3 at = origin + dir * hits[k].t;
                bool head = hits[k].head && d.headMult > 1.f;   // the shotgun has no headshot bonus
                // A Shieldbearer's shield stops body shots from the front (not
                // the head over its rim), and the round with it
                if (!hits[k].head && e.blocks(dir)) {
                    spawnHitSparks(at, {0.4f, 1.f, 0.75f});
                    if (shieldClankCd <= 0.f) { audio.play("clank", 70); shieldClankCd = 0.12f; }
                    anyHit = true;
                    break;
                }
                float m = head ? d.headMult : 1.f;
                float falloff = 1.f - 0.15f * k;    // each body it punches through costs a little
                if (head) spawnHitSparks(at, {1.f, 0.9f, 0.3f});
                anyHit = true;
                if (hurtEnemy(e, dmg * m * falloff, at, sniper ? 12.f : (pellets > 1 ? 3.f : 10.f), pellets > 1 ? 0.5f : 2.f, head)) {
                    ++killCount;
                    if (head) headKill = true;
                }
            }
            // Explosive tips: burst where the round stops
            if (id == WeaponId::LONGSHOT && u.mod)
                pendingBlasts.push_back({origin + dir * (n > 0 ? hits[0].t : wallT) - dir * 0.3f, 3.5f, 70.f, 0.f, 0.f});
        }
        kills = killCount > 0;

        // Trick-shot bonuses
        if (kills && id == WeaponId::LONGSHOT && quick) {
            styleSystem.addStyle(40.f); gainXp(25);
            ui.toast("QUICKSCOPE", "+25 XP", {1.f, 0.85f, 0.2f}, 1.4f);
            audio.play("parry", 90);
        } else if (kills && sniper && noscope) {
            styleSystem.addStyle(60.f); gainXp(40);
            ui.toast("NOSCOPE", "+40 XP", {1.f, 0.4f, 0.8f}, 1.6f);
            audio.play("parry", 90);
        }
        if (killCount >= 2) {
            char buf[32]; snprintf(buf, sizeof(buf), "COLLATERAL x%d", killCount);
            styleSystem.addStyle(25.f * (killCount - 1)); gainXp(15 * (killCount - 1));
            ui.toast(buf, "", {1.f, 0.55f, 0.2f}, 1.4f);
        }
        if (headKill && sniper) {
            ui.feed("HEADSHOT", {1.f, 0.85f, 0.25f});
            styleSystem.addStyle(10.f);
            // HEADHUNTER: the round comes back and the bolt is skipped
            if (id == WeaponId::KAR && u.mod) { ++ws.ammo; ws.cooldown = 0.1f; }
        }
        if (killCount > 0 && sniper) hitStopFrames = glm::max(hitStopFrames, 3);

        float recoil = d.recoil * (1.f - 0.4f * aimNow);
        recoilPitch = std::min(recoilPitch + recoil, 14.f);
        player.camera.pitch = glm::clamp(player.camera.pitch + recoil, -89.f, 89.f);

        viewModel.triggerFire();
        ui.onShoot();
        muzzleFlashPos   = origin + fwd * 0.6f;
        muzzleFlashTimer = sniper ? 0.08f : pellets > 1 ? 0.07f : 0.04f;
        shake(sniper ? 0.14f : pellets > 1 ? 0.12f : 0.06f, sniper ? 0.03f : pellets > 1 ? 0.025f : 0.012f);
        static const char* SND[] = {"revolver", "shotgun", "kar", "longshot"};
        audio.play(SND[w]);
        ++totalShots;
        if (anyHit) ++totalHits;
        spawnShellCasing(origin, right);
        if (pellets > 1) spawnShellCasing(origin + right * 0.1f, right);

        if (ws.ammo <= 0) {
            if (!sniper) startReload(w);   // rifles reload after the bolt cycle (physicsTick)
        } else if (sniper && ws.cooldown > 0.2f) {
            viewModel.triggerBolt(ws.cooldown * 0.85f);
            boltSoundTimer = ws.cooldown * 0.2f;
        } else if (pellets > 1) {
            viewModel.triggerPump();
            audio.play("pump", 70);
        }
    }

    void throwGrenade() {
        --grenadeCount;
        grenadeTimer = 0.6f;
        glm::vec3 origin = player.camera.position;
        glm::vec3 dir    = player.camera.forward();
        projSystem.fire(origin, dir * 14.f + glm::vec3{0, 5.f, 0}, 80.f, true,
                        {0.3f, 0.9f, 0.1f}, /*grenade=*/true, /*blastRadius=*/5.f);
        viewModel.triggerGrenade();
        shake(0.05f, 0.008f);
        audio.play("jump");
    }

    // =========================================================================
    // Effects
    // =========================================================================
    // Round-robin search from the last allocation: O(1) on average instead of
    // rescanning the whole pool for every particle of an explosion.
    Particle* freeParticle() {
        for (int k = 0; k < MAX_PARTICLES; ++k) {
            int i = (particleCursor + k) % MAX_PARTICLES;
            if (!particles[i].alive) { particleCursor = (i + 1) % MAX_PARTICLES; return &particles[i]; }
        }
        return nullptr;
    }

    // Generic radial burst. gravity < 0 floats upward.
    void spawnBurst(glm::vec3 c, glm::vec3 color, int n, float speed, float life, float gravity) {
        for (int i = 0; i < n; ++i) {
            Particle* p = freeParticle();
            if (!p) return;
            glm::vec3 d{frand(-1.f,1.f), frand(0.f,1.f), frand(-1.f,1.f)};
            if (glm::length(d) < 1e-3f) d = {0,1,0};
            p->pos = c; p->vel = glm::normalize(d) * speed * frand(0.4f, 1.f);
            p->maxLife = p->life = life * frand(0.6f, 1.f);
            p->color = color * frand(0.8f, 1.2f);
            p->gravity = gravity;
            p->alive = true;
        }
    }

    void spawnDeathParticles(glm::vec3 center, glm::vec3 color) { spawnBurst(center, color, 22, 12.f, 1.0f, 18.f); }
    void spawnHitSparks(glm::vec3 pos, glm::vec3 color)         { spawnBurst(pos, color, 5, 5.f, 0.25f, 18.f); }

    void spawnShellCasing(glm::vec3 origin, glm::vec3 right) {
        Particle* p = freeParticle();
        if (!p) return;
        p->pos     = origin + right * 0.15f;
        p->vel     = right * 2.5f + glm::vec3{0, 3.f, 0}
                   + glm::vec3{((rand()%100)-50)/100.f, 0, ((rand()%100)-50)/100.f};
        p->maxLife = p->life = 0.7f;
        p->color   = {0.85f, 0.7f, 0.15f};
        p->gravity = 18.f;
        p->alive   = true;
    }

    void spawnExplosionParticles(glm::vec3 center, float radius) {
        for (int i = 0; i < 70; ++i) {
            Particle* p = freeParticle();
            if (!p) return;
            glm::vec3 d = glm::normalize(glm::vec3{frand(-1.f,1.f), frand(0.2f,0.8f), frand(-1.f,1.f)});
            p->pos     = center + d * (radius * 0.3f);
            p->vel     = d * (8.f + frand(0.f, 1.f) * radius * 3.f);
            p->maxLife = p->life = 0.5f + frand(0.f, 0.7f);
            float heat = frand(0.f, 1.f);
            p->color   = heat > 0.8f ? glm::vec3{1.f, 0.95f, 0.75f}
                       : heat > 0.4f ? glm::vec3{1.f, 0.55f, 0.10f}
                                     : glm::vec3{0.55f, 0.12f, 0.05f};
            p->gravity = 18.f;
            p->alive   = true;
        }
    }

    void spawnShockwave(glm::vec3 pos, float radius, glm::vec3 color) {
        shockwaves.push_back({pos, radius, 0.f, color});
        for (int i = 0; i < 36; ++i) {
            Particle* p = freeParticle();
            if (!p) return;
            float a = i * 6.2832f / 36.f;
            p->pos = pos + glm::vec3{0, 0.2f, 0};
            p->vel = glm::vec3{std::cos(a), 0.15f, std::sin(a)} * radius * 2.2f;
            p->maxLife = p->life = 0.45f;
            p->color = color;
            p->gravity = 2.f;
            p->alive = true;
        }
    }

    void updateParticles(float dt) {
        for (auto& p : particles) {
            if (!p.alive) continue;
            p.vel.y -= p.gravity * dt;
            p.pos   += p.vel * dt;
            p.life  -= dt;
            if (p.life <= 0.f) p.alive = false;
        }
    }

    // Dust in the yard, embers over lava, motes around the reactor, wind up high.
    void spawnAmbientParticles(float dt) {
        int a = level.arenaAt(player.position);
        if (a < 0) return;
        Ambient kind = level.arenas[a].ambient;
        ambientTimer += dt;
        const float every = 1.f / 45.f;
        while (ambientTimer > every) {
            ambientTimer -= every;
            Particle* p = freeParticle();
            if (!p) return;
            p->alive = true;
            glm::vec3 me = player.position;
            float ang = frand(0.f, 6.28f), r = frand(4.f, 22.f);
            switch (kind) {
            case Ambient::DUST:
                p->pos = me + glm::vec3{std::cos(ang) * r, frand(0.3f,6.f), std::sin(ang) * r};
                p->vel = {frand(0.2f,0.8f), frand(-0.05f,0.15f), frand(-0.2f,0.2f)};
                p->color = glm::vec3{1.f, 0.7f, 0.5f} * 0.15f;
                p->gravity = 0.f; p->maxLife = p->life = frand(2.5f, 4.5f);
                break;
            case Ambient::EMBERS:
                if (!level.hazards.empty() && !fast()) {
                    const Hazard& hz = level.hazards[rand() % level.hazards.size()];
                    p->pos = {frand(hz.box.min.x, hz.box.max.x), hz.box.max.y + 0.05f, frand(hz.box.min.z, hz.box.max.z)};
                } else {
                    p->pos = me + glm::vec3{std::cos(ang) * r, frand(-2.f, 1.f), std::sin(ang) * r};
                }
                p->vel = {frand(-0.4f,0.4f), frand(1.5f,3.5f), frand(-0.4f,0.4f)};
                p->color = glm::vec3{1.f, 0.45f, 0.1f} * frand(0.6f, 1.1f);
                p->gravity = -0.5f; p->maxLife = p->life = frand(1.2f, 2.8f);
                break;
            case Ambient::MOTES: {
                glm::vec3 c = level.hasReactor ? level.reactorPos : me;
                float rr = frand(2.5f, 9.f);
                p->pos = c + glm::vec3{std::cos(ang) * rr, frand(-1.f, 2.5f), std::sin(ang) * rr};
                p->vel = {0.f, frand(1.f, 2.5f), 0.f};
                p->color = glm::vec3{0.3f, 0.9f, 1.f} * 0.6f;
                p->gravity = -0.3f; p->maxLife = p->life = frand(2.f, 4.f);
                break;
            }
            case Ambient::WIND:
                // Fast pale streaks blowing across the heights
                p->pos = me + glm::vec3{std::cos(ang) * r - 10.f, frand(-1.f, 9.f), std::sin(ang) * r};
                p->vel = {frand(9.f, 14.f), frand(-0.3f, 0.3f), frand(1.f, 3.f)};
                p->color = glm::vec3{0.85f, 0.9f, 1.f} * 0.22f;
                p->gravity = 0.f; p->maxLife = p->life = frand(1.f, 2.f);
                break;
            case Ambient::STEAM:
                // Pale puffs drifting up from the floor
                p->pos = me + glm::vec3{std::cos(ang) * r * 0.6f, frand(-1.f, 0.5f), std::sin(ang) * r * 0.6f};
                p->vel = {frand(-0.3f, 0.3f), frand(1.f, 2.2f), frand(-0.3f, 0.3f)};
                p->color = glm::vec3{0.7f, 0.95f, 0.85f} * 0.16f;
                p->gravity = -0.2f; p->maxLife = p->life = frand(2.f, 3.5f);
                break;
            case Ambient::ASH:
                p->pos = me + glm::vec3{std::cos(ang) * r, frand(4.f, 12.f), std::sin(ang) * r};
                p->vel = {frand(0.3f, 1.2f), frand(-1.2f, -0.4f), frand(-0.3f, 0.3f)};
                p->color = glm::vec3{0.6f, 0.55f, 0.55f} * 0.25f;
                p->gravity = 0.f; p->maxLife = p->life = frand(3.f, 5.f);
                break;
            }
        }
    }

    void spawnDebrisFor(const Enemy& e) {
        static std::vector<BoxInstance> parts;
        parts.clear();
        Enemy pose = e;             // the pieces keep the enemy's own colours,
        pose.hitFlashTimer = 0.f;   // not the white flash of the killing blow
        pose.spawnTimer    = 0.f;
        buildEnemy(pose, gameClock, parts);
        glm::vec3 c = e.position + glm::vec3{0, e.height() * 0.5f, 0};
        float floorY = groundHeightAt(e.position.x, e.position.z, e.position.y + 0.5f) + 0.1f;
        for (auto& b : parts) {
            Debris d;
            d.shape = glm::mat3(b.model);
            d.pos   = glm::vec3(b.model[3]);
            glm::vec3 out = d.pos - c; out.y = std::max(out.y, 0.f);
            float ol = glm::length(out);
            d.vel   = (ol > 1e-3f ? out / ol : glm::vec3{0,1,0}) * frand(3.f, 8.f)
                    + glm::vec3{frand(-2.f,2.f), frand(3.f,7.f), frand(-2.f,2.f)} + e.velocity * 0.3f;
            d.axis  = glm::normalize(glm::vec3{frand(-1.f,1.f), frand(-1.f,1.f), frand(-1.f,1.f)} + glm::vec3{0.01f});
            d.spin  = frand(4.f, 12.f);
            d.color = b.color; d.emissive = b.emissive;
            d.maxLife = d.life = frand(1.2f, 1.8f);
            d.floorY = floorY;
            debris.push_back(d);
        }
        if (debris.size() > 900) debris.erase(debris.begin(), debris.begin() + (debris.size() - 900));
    }

    void updateDebris(float dt) {
        for (auto& d : debris) {
            d.vel.y -= 20.f * dt;
            d.pos   += d.vel * dt;
            d.angle += d.spin * dt;
            if (d.pos.y < d.floorY) { d.pos.y = d.floorY; d.vel.y = std::fabs(d.vel.y) * 0.35f; d.vel.x *= 0.6f; d.vel.z *= 0.6f; d.spin *= 0.6f; }
            d.life -= dt;
        }
        debris.erase(std::remove_if(debris.begin(), debris.end(),
                     [](const Debris& d){ return d.life <= 0.f; }), debris.end());
    }

    void spawnDecal(glm::vec3 enemyPos) {
        int slot = -1;
        float minLife = 1e9f;
        for (int i = 0; i < MAX_DECALS; ++i) {
            if (!decals[i].alive) { slot = i; break; }
            if (decals[i].life < minLife) { minLife = decals[i].life; slot = i; }
        }
        auto& d  = decals[slot];
        d.pos    = enemyPos + glm::vec3{0.f, 0.025f, 0.f};
        d.maxLife = 10.f;
        d.life    = d.maxLife;
        d.alive   = true;
    }

    void spawnTracer(glm::vec3 start, glm::vec3 end, float width = 0.055f, float life = 0.22f) {
        Tracer* slot = nullptr;
        for (auto& t : tracers) if (!t.alive) { slot = &t; break; }
        if (!slot) {   // all busy (shotgun spam): reuse the oldest
            slot = &tracers[0];
            for (auto& t : tracers) if (t.life < slot->life) slot = &t;
        }
        slot->start = start; slot->end = end;
        slot->maxLife = life; slot->life = life; slot->width = width;
        slot->alive = true;
    }

    // =========================================================================
    // Rendering
    // =========================================================================
    void applyLighting(ShaderProgram& sh, const Theme& th, glm::vec3 camPos) {
        sh.setVec3("lightDir",    th.lightDir);
        sh.setVec3("lightColor",  th.lightColor);
        sh.setVec3("viewPos",     camPos);
        sh.setVec3("uSkyAmb",     th.skyAmb);
        sh.setVec3("uGroundAmb",  th.groundAmb);
        sh.setVec3("uFogColor",   th.fogColor);
        sh.setFloat("uFogDensity", th.fogDensity);
        sh.setVec3Array("pointLightPos",   pointLightPos,   MAX_POINT_LIGHTS);
        sh.setVec3Array("pointLightColor", pointLightColor, MAX_POINT_LIGHTS);
    }

    // Everything dynamic that's made of boxes, gathered for one instanced draw.
    void gatherBoxes(std::vector<BoxInstance>& out, const glm::mat4& view) {
        using namespace rig;
        float t = gameClock;

        for (auto& e : enemies) {
            if (!e.alive) continue;
            Enemy pose = e;
            pose.position = lerpPos(e.prevPosition, e.position);
            e.shownPos = pose.position; e.hasShown = true;
            pose.yaw = e.prevYaw + std::remainder(e.yaw - e.prevYaw, 6.2831853f) * renderAlpha;
            buildEnemy(pose, t, out);
            // The SOVEREIGN leaves afterimages down the line of a dash
            if (e.type == EnemyType::SOVEREIGN && e.dashTimer > 0.f) {
                for (int k = 1; k <= 3; ++k) {
                    Enemy ghost = pose;
                    ghost.position -= e.diveDir * (1.7f * k);
                    ghost.hitFlashTimer = 0.f;
                    size_t from = out.size();
                    buildEnemy(ghost, t, out);
                    for (size_t j = from; j < out.size(); ++j) {
                        out[j].color = glm::vec3{0.05f};
                        out[j].emissive = glm::vec3{1.2f, 0.3f, 0.15f} * (0.9f / k);
                    }
                }
            }
        }

        // FAST: the ghost of your best run, a glowing figure on its route
        glm::vec3 gFeet; float gYaw;
        if (fast() && (!settings || settings->ghost) && countdown <= 0.f && ghost.at(elapsedTime, gFeet, gYaw) &&
            glm::length(gFeet - player.position) > 1.6f) {
            float r = glm::radians(gYaw);
            glm::mat4 g = T(gFeet) * RY(std::atan2(std::cos(r), std::sin(r)));
            float stride = std::sin(elapsedTime * 11.f) * 0.35f;
            glm::vec3 col{0.03f, 0.06f, 0.08f}, glow{0.25f, 0.9f, 1.2f};
            for (float s : {-1.f, 1.f})
                push(out, g * T({s * 0.14f, 0.85f, 0.f}) * RX(stride * s) * T({0.f, -0.42f, 0.f}) * S({0.16f, 0.84f, 0.18f}), col, glow * 0.6f);
            push(out, g * T({0.f, 1.25f, 0.f}) * S({0.46f, 0.72f, 0.26f}), col, glow * 0.8f);
            push(out, g * T({0.f, 1.78f, 0.f}) * S(glm::vec3{0.26f}), col, glow * 1.2f);
            for (float s : {-1.f, 1.f})
                push(out, g * T({s * 0.31f, 1.5f, 0.f}) * RX(-stride * s) * T({0.f, -0.32f, 0.f}) * S({0.12f, 0.64f, 0.12f}), col, glow * 0.5f);
        }

        // Spawn beams: a column of light while an enemy materialises
        for (auto& e : enemies) {
            if (!e.alive || e.state != EnemyState::SPAWNING) continue;
            float k = e.spawnTimer / Enemy::SPAWN_TIME;
            glm::vec3 c = e.stats().glow;
            glm::vec3 ep = lerpPos(e.prevPosition, e.position);
            float base = ep.y;
            push(out, T({ep.x, base + 8.f, ep.z}) * S({0.25f + 0.6f * k, 16.f, 0.25f + 0.6f * k}),
                 c * 0.2f, c * (1.5f + 2.f * k));
            push(out, T({ep.x, base + 0.05f, ep.z}) * RY(t * 3.f) * S({2.2f * k + 0.5f, 0.06f, 2.2f * k + 0.5f}),
                 c * 0.2f, c * 2.f);
        }

        // Enemy health bars (only once damaged), facing the camera
        glm::vec3 camRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
        glm::vec3 camFwdFlat = glm::normalize(glm::cross(camRight, glm::vec3(0,1,0)));
        for (auto& e : enemies) {
            if (!e.targetable() || e.health >= e.maxHealth || isBoss(e.type)) continue;
            float barW = std::max(1.0f, e.radius() * 1.8f), barH = 0.12f;
            float fill = e.health / e.maxHealth;
            glm::vec3 barPos = lerpPos(e.prevPosition, e.position) + glm::vec3{0, e.height() + 0.45f, 0};
            glm::mat4 bg(1.f);
            bg[0] = glm::vec4(camRight * barW, 0); bg[1] = glm::vec4(0, barH, 0, 0);
            bg[2] = glm::vec4(camFwdFlat * 0.01f, 0); bg[3] = glm::vec4(barPos, 1);
            push(out, bg, {0.05f, 0.05f, 0.05f}, {0.02f, 0.02f, 0.02f});
            glm::mat4 fm(1.f);
            fm[0] = glm::vec4(camRight * barW * fill, 0); fm[1] = glm::vec4(0, barH * 0.8f, 0, 0);
            fm[2] = glm::vec4(camFwdFlat * 0.02f, 0);
            fm[3] = glm::vec4(barPos - camRight * (barW * (1.f - fill) * 0.5f) - camFwdFlat * 0.01f, 1);
            push(out, fm, {0.9f, 0.15f, 0.1f}, {1.2f, 0.15f, 0.08f});
        }

        for (auto& d : debris) {
            float k = glm::clamp(d.life / d.maxLife, 0.f, 1.f);
            glm::mat4 m = T(d.pos) * glm::rotate(glm::mat4(1.f), d.angle, d.axis)
                        * glm::mat4(d.shape) * S(glm::vec3{0.3f + 0.7f * k});
            push(out, m, d.color * (0.4f + 0.6f * k), d.emissive * k);
        }

        // Projectiles: a spinning bright core inside a darker shell
        for (auto& p : projSystem.pool) {
            if (!p.alive) continue;
            float s = (p.isGrenade ? 0.22f : 0.26f) * p.size;
            float spin = t * 9.f + p.position.x;
            glm::mat4 base = T(lerpPos(p.prevPosition, p.position)) * RY(spin) * RX(spin * 0.7f);
            push(out, base * S(glm::vec3{s}), p.emissiveColor * 0.3f, p.emissiveColor * 0.8f);
            push(out, base * RZ(0.785f) * S(glm::vec3{s * 0.75f}), p.emissiveColor, p.emissiveColor * 3.f);
        }

        // Drops
        for (auto& p : pickups) {
            float blink = p.life < 5.f && std::fmod(p.life, 0.4f) < 0.15f ? 0.2f : 1.f;
            glm::vec3 pp = lerpPos(p.prev, p.pos);
            glm::vec3 bob = pp + glm::vec3{0, std::sin(t * 3.f + p.pos.x) * 0.1f, 0};
            switch (p.kind) {
            case PickupKind::ORB: {
                float pulse = 1.f + 0.3f * std::sin(t * 8.f);
                push(out, T(bob) * RY(t * 3.f) * RX(0.6f) * S(glm::vec3{0.32f}),
                     {0.2f, 0.9f, 0.4f}, glm::vec3{0.3f, 1.6f, 0.6f} * pulse * blink);
                break;
            }
            case PickupKind::POTION: {
                // A red flask: body, neck, cork, and a loot beam
                glm::mat4 m = T(bob) * RY(t * 1.5f);
                glm::vec3 red{0.9f, 0.1f, 0.15f}, glow = glm::vec3{1.8f, 0.2f, 0.3f} * (0.7f + 0.3f * std::sin(t * 5.f)) * blink;
                push(out, m * T({0, 0.f, 0}) * S({0.42f, 0.42f, 0.42f}), red, glow);
                push(out, m * T({0, 0.3f, 0}) * S({0.16f, 0.2f, 0.16f}), {0.8f, 0.85f, 0.9f}, glm::vec3{0.4f} * blink);
                push(out, m * T({0, 0.45f, 0}) * S({0.18f, 0.1f, 0.18f}), {0.45f, 0.3f, 0.15f});
                push(out, T(p.pos + glm::vec3{0, 3.f, 0}) * S({0.08f, 6.f, 0.08f}), red * 0.2f, glow * 0.6f);
                break;
            }
            case PickupKind::XP: {
                glm::vec3 c{0.55f, 0.4f, 1.f}, g = glm::vec3{1.0f, 0.7f, 2.f} * (0.8f + 0.4f * std::sin(t * 6.f)) * blink;
                push(out, T(bob) * RY(t * 4.f) * RX(0.785f) * RZ(0.785f) * S(glm::vec3{0.36f}), c, g);
                push(out, T(p.pos + glm::vec3{0, 3.f, 0}) * S({0.06f, 6.f, 0.06f}), c * 0.2f, g * 0.5f);
                break;
            }
            }
        }

        for (auto& sl : slashes) {
            float reveal = std::min(1.f, sl.t / 0.1f), fade = 1.f - sl.t / SLASH_LIFE;
            const int N = 18;
            glm::vec3 hot = glm::vec3{1.9f, 1.4f, 0.8f} * fade * 2.f;
            for (int i = 0; i < N; ++i) {
                float u = (i + 0.5f) / N;
                if (u > reveal) break;
                float w = 1.f - std::fabs(u * 2.f - 1.f);              // thickest mid-arc
                if (sl.kind == 2) {                                    // overhead: a vertical arc down to the floor
                    float R = 3.2f, b = -0.4f + 2.1f * u;
                    glm::mat4 m = T(sl.pos + glm::vec3{0, 0.3f, 0}) * RY(sl.yaw) * T({0.f, R * std::cos(b) * 0.75f, R * std::sin(b)})
                                * RX(b) * S({0.5f * w + 0.1f, 0.08f, R * 2.1f / N * 1.2f});
                    push(out, m, glm::vec3{0.1f}, hot);
                } else {                                               // sweeps: a flat arc at chest height
                    float R = sl.kind == 3 ? 3.4f : 4.4f, span = sl.kind == 3 ? 0.9f : 1.35f;
                    float a = (sl.kind == 1 ? 1.f - u : u) * 2.f * span - span;
                    glm::mat4 m = T(sl.pos + glm::vec3{0, 2.1f, 0}) * RY(sl.yaw + a) * T({0.f, 0.f, R})
                                * S({R * 2.f * span / N * 1.15f, 0.07f, 0.6f * w + 0.12f});
                    push(out, m, glm::vec3{0.1f}, hot);
                }
            }
        }

        for (auto& s : shockwaves) {
            float k = s.t / 0.45f;
            float r = s.radius * std::min(1.f, k);
            float fade = 1.f - std::min(1.f, k);
            for (int i = 0; i < 40; ++i) {
                float a = i * 6.2832f / 40.f;
                push(out, T(s.pos + glm::vec3{std::cos(a) * r, 0.25f, std::sin(a) * r}) * RY(-a) * S({0.25f, 0.5f * fade + 0.05f, r * 0.17f}),
                     s.color * 0.3f, s.color * 2.5f * fade);
            }
        }

        // Doors: two halves that part in the middle. Red while locked, cyan
        // when they'll open for you. The meeting edges glow.
        for (auto& d : level.doors) {
            if (d.openAmount >= 0.999f) continue;
            glm::vec3 lit = d.locked ? glm::vec3{1.8f, 0.18f, 0.12f} : glm::vec3{0.25f, 1.3f, 1.6f};
            if (!d.locked && d.open) lit *= 1.4f;
            bool ax = d.alongX();
            for (int h = 0; h < 2; ++h) {
                const AABB& b = level.walls[h ? d.wall2 : d.wall].box;
                glm::vec3 c = (b.min + b.max) * 0.5f, sz = b.max - b.min;
                if ((ax ? sz.x : sz.z) < 0.02f) continue;
                push(out, T(c) * S(sz), {0.17f, 0.17f, 0.21f});
                float edge = ax ? (h ? b.min.x : b.max.x) : (h ? b.min.z : b.max.z);
                float in = h ? 0.08f : -0.08f;
                glm::vec3 e = c;
                (ax ? e.x : e.z) = edge + in;
                glm::vec3 es = ax ? glm::vec3{0.16f, sz.y, sz.z + 0.06f} : glm::vec3{sz.x + 0.06f, sz.y, 0.16f};
                push(out, T(e) * S(es), lit * 0.2f, lit);
                for (float fy : {0.22f, 0.5f, 0.78f}) {        // light bars across each half
                    glm::vec3 bc = c; bc.y = b.min.y + sz.y * fy;
                    glm::vec3 bs = ax ? glm::vec3{sz.x * 0.75f, 0.09f, sz.z + 0.05f} : glm::vec3{sz.x + 0.05f, 0.09f, sz.z * 0.75f};
                    push(out, T(bc) * S(bs), lit * 0.12f, lit * 0.5f);
                }
            }
        }

        // Boost tubes: chevrons streaming along the floor (or up the shaft)
        for (auto& bo : level.boosters) {
            glm::vec3 dir = bo.dir, mn = bo.box.min, mx = bo.box.max;
            glm::vec3 col = glm::vec3{0.3f, 1.4f, 1.9f};
            if (dir.y > 0.5f) {
                // Lift shaft: rings rising up it
                glm::vec3 c = (mn + mx) * 0.5f, sz = mx - mn;
                for (int k = 0; k < 6; ++k) {
                    float y = mn.y + std::fmod(t * bo.speed * 0.35f + k * sz.y / 6.f, sz.y);
                    float a = 1.f - std::fabs(y - c.y) / (sz.y * 0.5f);
                    glm::vec3 g = col * (0.4f + 0.8f * a);
                    push(out, T({c.x, y, mn.z + 0.05f}) * S({sz.x, 0.08f, 0.08f}), g * 0.2f, g);
                    push(out, T({c.x, y, mx.z - 0.05f}) * S({sz.x, 0.08f, 0.08f}), g * 0.2f, g);
                    push(out, T({mn.x + 0.05f, y, c.z}) * S({0.08f, 0.08f, sz.z}), g * 0.2f, g);
                    push(out, T({mx.x - 0.05f, y, c.z}) * S({0.08f, 0.08f, sz.z}), g * 0.2f, g);
                }
                continue;
            }
            glm::vec3 side{-dir.z, 0.f, dir.x};
            glm::vec3 c = (mn + mx) * 0.5f;
            float len = std::fabs(glm::dot(mx - mn, dir));
            glm::vec3 start = c - dir * (len * 0.5f);
            start.y = mn.y + 0.04f;
            const float gap = 2.4f;
            float ph = std::fmod(t * bo.speed * 0.45f, gap);
            for (float u = ph; u < len; u += gap) {
                glm::vec3 tip = start + dir * u;
                float fade = std::min({1.f, u / 3.f, (len - u) / 3.f});
                glm::vec3 g = col * fade;
                for (float sgn : {1.f, -1.f}) {
                    glm::vec3 arm = glm::normalize(-dir + side * sgn * 1.1f);
                    push(out, T(tip + arm * 0.55f) * RY(std::atan2(arm.x, arm.z)) * S({0.16f, 0.03f, 1.1f}), g * 0.2f, g);
                }
            }
        }

        // Fans: four blades spinning in a housing ring
        for (auto& f : level.fans) {
            glm::mat4 base = T(f.pos) * (f.axis == 0 ? RZ(1.5708f) : f.axis == 2 ? RX(1.5708f) : glm::mat4(1.f));
            float spin = t * 7.f + f.pos.x;
            for (int k = 0; k < 4; ++k)
                push(out, base * RY(spin + k * 0.785f) * S({f.radius * 1.9f, 0.06f, f.radius * 0.28f}), {0.2f, 0.2f, 0.23f});
            push(out, base * S({0.5f, 0.18f, 0.5f}), {0.15f, 0.15f, 0.18f}, f.glow * 0.6f);
            for (int k = 0; k < 12; ++k) {
                float a = k * 0.5236f;
                push(out, base * T({std::cos(a) * f.radius, 0.f, std::sin(a) * f.radius}) * RY(-a) * S({0.16f, 0.16f, f.radius * 0.56f}),
                     f.glow * 0.15f, f.glow * 0.7f);
            }
        }

        // Jump pads: a plate, a glowing core, and a ring that rises off it
        for (auto& pad : level.pads) {
            glm::vec3 c = pad.centre;
            glm::vec2 h = pad.half;
            push(out, T(c + glm::vec3{0, 0.06f, 0}) * S({h.x * 2.f + 0.3f, 0.12f, h.y * 2.f + 0.3f}), {0.15f, 0.16f, 0.2f});
            float pulse = 0.7f + 0.3f * std::sin(t * 6.f);
            push(out, T(c + glm::vec3{0, 0.13f, 0}) * S({h.x * 1.4f, 0.04f, h.y * 1.4f}), {0.2f, 0.8f, 0.7f}, glm::vec3{0.3f, 1.4f, 1.1f} * pulse);
            for (int k = 0; k < 2; ++k) {
                float ph = std::fmod(t * 0.9f + k * 0.5f, 1.f);
                float y = 0.2f + ph * 2.2f, s = 1.f - ph * 0.5f, a = (1.f - ph);
                glm::vec3 e = glm::vec3{0.3f, 1.3f, 1.0f} * a;
                push(out, T(c + glm::vec3{0, y,  h.y * s}) * S({h.x * 2.f * s, 0.05f, 0.08f}), e * 0.2f, e);
                push(out, T(c + glm::vec3{0, y, -h.y * s}) * S({h.x * 2.f * s, 0.05f, 0.08f}), e * 0.2f, e);
                push(out, T(c + glm::vec3{ h.x * s, y, 0}) * S({0.08f, 0.05f, h.y * 2.f * s}), e * 0.2f, e);
                push(out, T(c + glm::vec3{-h.x * s, y, 0}) * S({0.08f, 0.05f, h.y * 2.f * s}), e * 0.2f, e);
            }
        }

        // Moving platforms: a slab with glowing edges and a grapple ring under it
        for (auto& m : level.movers) {
            AABB b = level.walls[m.wall].box;
            b.min -= m.delta * (1.f - renderAlpha); b.max -= m.delta * (1.f - renderAlpha);
            glm::vec3 c = (b.min + b.max) * 0.5f, sz = b.max - b.min;
            float pulse = 0.75f + 0.25f * std::sin(t * 4.f + c.x);
            push(out, T(c) * S(sz), m.color);
            glm::vec3 g = m.glow * pulse;
            float ey = b.max.y - 0.03f;
            push(out, T({c.x, ey, b.min.z}) * S({sz.x + 0.06f, 0.08f, 0.08f}), g * 0.2f, g);
            push(out, T({c.x, ey, b.max.z}) * S({sz.x + 0.06f, 0.08f, 0.08f}), g * 0.2f, g);
            push(out, T({b.min.x, ey, c.z}) * S({0.08f, 0.08f, sz.z + 0.06f}), g * 0.2f, g);
            push(out, T({b.max.x, ey, c.z}) * S({0.08f, 0.08f, sz.z + 0.06f}), g * 0.2f, g);
            float ry = b.min.y - 0.35f, rr = std::min(sz.x, sz.z) * 0.3f;
            for (int k = 0; k < 8; ++k) {
                float a = k * 0.785f + t * 1.5f;
                push(out, T({c.x + std::cos(a) * rr, ry, c.z + std::sin(a) * rr}) * RY(-a) * S({0.1f, 0.1f, rr * 0.8f}), g * 0.2f, g * 1.3f);
            }
        }

        // Lava shimmer over the channels
        for (auto& hz : level.hazards) {
            glm::vec3 c = (hz.box.min + hz.box.max) * 0.5f, sz = hz.box.max - hz.box.min;
            for (int i = 0; i < 8; ++i) {
                float x = hz.box.min.x + sz.x * (i + 0.5f) / 8.f;
                float w = 0.5f + 0.5f * std::sin(t * 2.f + i * 1.7f);
                push(out, T({x, c.y + 0.04f, c.z}) * S({sz.x / 8.f * 0.9f, 0.05f, sz.z * 0.6f}),
                     {1.f, 0.5f, 0.1f}, glm::vec3{1.6f, 0.7f, 0.12f} * (0.6f + 0.6f * w));
            }
        }

        // Spinning gems (the obelisk, the Spire's beacon, the finish)
        for (auto& g : level.gems) {
            float s = g.size;
            glm::vec3 col = g.color;
            push(out, T(g.pos + glm::vec3{0, std::sin(t * 1.5f) * 0.3f, 0}) * RY(t) * RX(0.785f) * RZ(0.785f) * S(glm::vec3{s}),
                 col * 0.6f, col);
            if (g.beam) push(out, T(g.pos + glm::vec3{0, 40.f, 0}) * S({0.35f, 80.f, 0.35f}), col * 0.1f, col * 0.5f);
        }

        // FAST: the finish beacon lights up once the last section is clear
        if (fast()) {
            glm::vec3 f = level.finishPos;
            glm::vec3 col = finishOpen ? glm::vec3{1.8f, 0.7f, 0.2f} : glm::vec3{0.3f, 0.12f, 0.1f};
            push(out, T(f + glm::vec3{0, 0.08f, 0}) * S({5.f, 0.16f, 5.f}), {0.15f, 0.12f, 0.12f});
            for (int k = 0; k < 16; ++k) {
                float a = k * 0.3927f + t * (finishOpen ? 1.2f : 0.2f);
                push(out, T(f + glm::vec3{std::cos(a) * 2.6f, 0.25f, std::sin(a) * 2.6f}) * RY(-a) * S({0.18f, 0.18f, 0.8f}), col * 0.2f, col);
            }
            if (finishOpen) push(out, T(f + glm::vec3{0, 30.f, 0}) * S({1.2f, 60.f, 1.2f}), col * 0.1f, col * 0.8f);
        }

        // The Core: the reactor — stacked counter-rotating blocks and orbiting rings
        if (level.hasReactor) {
            bool bossRage = false;
            for (auto& e : enemies) if (e.alive && e.type == EnemyType::WARDEN && e.enraged) bossRage = true;
            glm::vec3 base = level.reactorPos;
            glm::vec3 hot = bossRage ? glm::vec3{1.8f, 0.2f, 0.3f} : glm::vec3{0.3f, 1.4f, 1.8f};
            glm::vec3 alt = bossRage ? glm::vec3{1.6f, 0.4f, 0.1f} : glm::vec3{1.6f, 0.25f, 1.1f};
            for (int i = 0; i < 6; ++i) {
                float y = 1.0f + i * 1.55f;
                float s = 2.6f - std::fabs(i - 2.5f) * 0.35f;
                float pulse = 0.6f + 0.4f * std::sin(t * 3.f + i);
                push(out, T(base + glm::vec3{0, y, 0}) * RY(t * (i % 2 ? 0.8f : -0.8f) + i) * S({s, 1.2f, s}),
                     {0.1f, 0.12f, 0.15f}, (i % 2 ? hot : alt) * 0.25f * pulse);
                push(out, T(base + glm::vec3{0, y + 0.66f, 0}) * RY(t * (i % 2 ? 0.8f : -0.8f) + i) * S({s + 0.1f, 0.12f, s + 0.1f}),
                     hot * 0.2f, hot * pulse);
            }
            push(out, T(base + glm::vec3{0, 30.f, 0}) * S({0.7f, 40.f, 0.7f}), hot * 0.2f, hot * 0.9f);   // beam to the sky
            for (int ringI = 0; ringI < 3; ++ringI) {
                float r = 3.6f + ringI * 1.6f, y = 3.5f + ringI * 3.f, spd = (ringI % 2 ? -0.6f : 0.5f);
                for (int k = 0; k < 14; ++k) {
                    float a = k * 6.2832f / 14.f + t * spd;
                    push(out, T(base + glm::vec3{std::cos(a) * r, y + std::sin(t * 2.f + k) * 0.15f, std::sin(a) * r})
                              * RY(-a) * S({0.2f, 0.2f, 0.7f}), alt * 0.2f, alt * 0.6f);
                }
            }
        }
    }

    bool scopedView() const { return activeWeapon == (int)WeaponId::LONGSHOT && aim > 0.9f; }

    void render() override {
        applyTextureQuality();
        float alpha = (float)(accumulator / PHYSICS_DT);
        renderAlpha = glm::clamp(alpha, 0.f, 1.f);
        glm::vec3 renderCamPos = glm::mix(prevCamPos, player.camera.position, alpha);
        Camera renderCam = player.camera;
        renderCam.position = renderCamPos;

        if (shakeTimer > 0.f) {
            float s = shakeIntensity * (shakeTimer / 0.2f) * (1.f - 0.7f * aim);
            renderCam.position += glm::vec3{frand(-1.f,1.f) * s, frand(-1.f,1.f) * s, 0.f};
        }
        if (!settings || settings->viewBob)
            renderCam.position += viewModel.getBobOffset(playerXZSpeed, player.onGround, renderCam.right()) * (1.f - aim);
        renderCam.position.y -= landSquash;
        const WeaponDef& wd = weaponDef((WeaponId)activeWeapon);
        float baseFov = settings ? settings->fov : 90.f;
        float a = aim * aim * (3.f - 2.f * aim);
        float zoom = wd.canAim ? glm::mix(1.f, wd.aimFov, a) : 1.f;
        renderCam.fov = baseFov * zoom + fovKick * (1.f - a);

        glm::mat4 view = renderCam.viewMatrix();
        glm::mat4 proj = renderCam.projectionMatrix();
        Theme th = level.themeAt(player.position);
        if (rageBlend > 0.f) th = lerpTheme(th, bloodEclipse(th), rageBlend);

        postProcess.beginScene();
        glClearColor(th.fogColor.r, th.fogColor.g, th.fogColor.b, 1.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // --- Sky ---
        {
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            skyboxShader.use();
            skyboxShader.setMat4("uInvProj", glm::inverse(proj));
            skyboxShader.setMat4("uInvView", glm::inverse(glm::mat4(glm::mat3(view))));
            skyboxShader.setVec3("uZenith",   th.zenith);
            skyboxShader.setVec3("uHorizon",  th.horizon);
            skyboxShader.setVec3("uGround",   th.ground);
            skyboxShader.setVec3("uSunDir",   th.sunDir);
            skyboxShader.setVec3("uSunColor", th.sunColor);
            skyboxShader.setFloat("uSunSize", th.sunSize);
            skyboxShader.setFloat("uSunStripes", th.sunStripes);
            skyboxShader.setVec3("uMountain", th.mountain);
            skyboxShader.setFloat("uStars",   th.stars);
            glBindVertexArray(postProcess.quadVAO);
            glDrawArrays(GL_TRIANGLES,0,6);
            glBindVertexArray(0);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

        // --- Static world ---
        worldShader.use();
        worldShader.setMat4("projection", proj);
        worldShader.setMat4("view",       view);
        worldShader.setMat4("model",      glm::mat4(1.f));
        applyLighting(worldShader, th, renderCamPos);
        worldShader.setVec3 ("emissiveColor", {0.f,0.f,0.f});
        worldShader.setVec3 ("objectColor",   {1.f,1.f,1.f});
        worldShader.setFloat("uPSXStrength",  0.0f);
        worldShader.setFloat("uVertexGlow",   0.0f);
        worldShader.setInt("uTexture",0);
        glActiveTexture(GL_TEXTURE0);
        for (int t = 0; t < TEX_COUNT; ++t) {
            if (world.byTex[t].indexCount == 0) continue;
            glBindTexture(GL_TEXTURE_2D, worldTex[t]);
            world.byTex[t].draw();
        }
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        worldShader.setVec3("objectColor", {0.25f,0.25f,0.25f});
        worldShader.setFloat("uVertexGlow", 1.5f);
        world.neon.draw();
        worldShader.setFloat("uVertexGlow", 0.0f);
        worldShader.setVec3("objectColor", {1.f,1.f,1.f});

        // --- Everything made of boxes: one instanced draw ---
        {
            static std::vector<BoxInstance> boxes;
            boxes.clear();
            gatherBoxes(boxes, view);
            boxRenderer.shader.use();
            boxRenderer.shader.setMat4("projection", proj);
            boxRenderer.shader.setMat4("view",       view);
            boxRenderer.shader.setInt ("uTexture",   0);
            applyLighting(boxRenderer.shader, th, renderCamPos);
            glBindTexture(GL_TEXTURE_2D, whiteTex);
            boxRenderer.draw(boxes);
        }

        worldShader.use();
        worldShader.setMat4("model", glm::mat4(1.f));
        grapple.drawLine(renderCamPos, view, proj);
        renderDecals();

        renderTracers(view, proj);
        renderLasers(view, proj);
        renderParticles(view, proj);
        bool warm = warmupFrames > 0;
        if (warm) warmPipelines(renderCamPos, view, proj);

        // --- View model (inside the FBO so it gets bloom; no fog). Hidden
        // behind the Longshot's scope once it's up. ---
        if (!scopedView()) {
            worldShader.use();
            worldShader.setVec3("lightDir",    glm::normalize(glm::vec3{0.4f,-1.f,0.3f}));
            worldShader.setVec3("lightColor",  {1.f,0.9f,0.8f});
            worldShader.setVec3("uSkyAmb",     {0.32f,0.32f,0.38f});
            worldShader.setVec3("uGroundAmb",  {0.12f,0.11f,0.10f});
            worldShader.setFloat("uFogDensity", 0.f);
            worldShader.setVec3("viewPos",     renderCamPos);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, whiteTex);
            worldShader.setInt("uTexture", 0);
            float flash = glm::clamp(muzzleFlashTimer / 0.06f, 0.f, 1.f);
            viewModel.draw(worldShader, renderCam, activeWeapon, flash, wd.canAim ? aim : 0.f,
                           weapons[activeWeapon].ammo, weaponMag((WeaponId)activeWeapon, prog.up[activeWeapon]));
            worldShader.setMat4("projection", proj);
            worldShader.setMat4("view",       view);
        }

        postProcess.crtEnabled = settings ? settings->crtFilter : false;
        postProcess.endScene();

        renderHUD(view, proj);
        if (warm) { ui.warmScope(); --warmupFrames; }
    }

    // Project a world point to the screen. Off-screen (or behind) points are
    // pushed to the screen edge in the right direction.
    bool projectToScreen(glm::vec3 p, const glm::mat4& view, const glm::mat4& proj, float& sx, float& sy) const {
        glm::vec4 c = proj * view * glm::vec4(p, 1.f);
        bool behind = c.w <= 0.01f;
        glm::vec2 ndc = behind ? -glm::vec2(c) : glm::vec2(c) / c.w;
        bool on = !behind && std::fabs(ndc.x) < 0.95f && std::fabs(ndc.y) < 0.9f;
        if (!on) {
            float m = std::max(std::fabs(ndc.x) / 0.92f, std::fabs(ndc.y) / 0.85f);
            if (m < 1e-4f) { ndc = {0.f, -0.85f}; m = 1.f; }
            ndc /= m;
        }
        sx = (ndc.x * 0.5f + 0.5f) * SCREEN_W;
        sy = (1.f - (ndc.y * 0.5f + 0.5f)) * SCREEN_H;
        return on;
    }

    void renderHUD(const glm::mat4& view, const glm::mat4& proj) {
        // ---- the HUD proper ----
        HudState h;
        h.health = styleSystem.health; h.maxHealth = styleSystem.maxHealth;
        h.activeWeapon = activeWeapon;
        for (int w = 0; w < WEAPON_COUNT; ++w) {
            h.weapons[w].name = weaponDef((WeaponId)w).name;
            h.weapons[w].ammo = weapons[w].ammo;
            h.weapons[w].mag  = weaponMag((WeaponId)w, prog.up[w]);
            h.weapons[w].reloading = weapons[w].reloading;
            h.weapons[w].reload01  = weapons[w].reloadProgress();
        }
        h.grenades = grenadeCount; h.grenadeMax = grenadeMax;
        h.level = prog.level; h.xp = prog.xp; h.xpNext = Progression::xpToNext(prog.level); h.points = prog.points;
        const WeaponDef& wd = weaponDef((WeaponId)activeWeapon);
        h.aim = wd.canAim ? aim : 0.f;
        h.scoped = activeWeapon == (int)WeaponId::LONGSHOT && aim > 0.82f;   // lens fades in, then the model hides
        // Crosshair gap follows the spread (pixels at the current FOV)
        float spread = weaponSpread((WeaponId)activeWeapon, prog.up[activeWeapon], h.aim);
        if (wd.canAim && !player.onGround) spread += 0.03f * (1.f - h.aim);
        h.spread = spread * (SCREEN_H * 0.5f) / std::tan(glm::radians(player.camera.fov) * 0.5f) * 0.7f;
        h.hideCrosshair = (activeWeapon == (int)WeaponId::KAR && aim > 0.6f) || scopedView() || playerDead || victory;
        h.grappleTarget = grappleTargetInSight && !grapple.active;
        h.showTimer = fast() || (settings && settings->showTimer);
        h.speed = fast() ? playerXZSpeed : -1.f;
        h.time = elapsedTime;
        if (settings) settings->crosshairColor(h.crosshairColor.r, h.crosshairColor.g, h.crosshairColor.b);
        h.viewProj = proj * view;
        h.damageNumbers = !settings || settings->damageNumbers;
        if (!playerDead && !victory) ui.render(styleSystem, h);

        const Arena& ar = level.arenas[director.arena];
        int nArenas = (int)level.arenas.size();
        char buf[128];

        if (!playerDead && !victory) {
            int alive = 0;
            for (auto& e : enemies) if (e.alive) ++alive;
            int left = alive + director.queued();
            glm::vec3 accent{1.f, 0.75f, 0.3f};
            const Enemy* boss = nullptr;
            for (auto& e : enemies) if (e.alive && isBoss(e.type)) boss = &e;

            if (fast()) {
                if (finishOpen) { snprintf(buf, sizeof(buf), "FINISH OPEN - CLIMB TO THE BEACON"); accent = {1.f, 0.6f, 0.2f}; }
                else if (director.phase == WaveDirector::Phase::APPROACH) {
                    snprintf(buf, sizeof(buf), "ROOM %d/%d  %s   ADVANCE", director.arena + 1, nArenas, ar.name);
                    accent = {0.4f, 1.f, 0.6f};
                } else snprintf(buf, sizeof(buf), "ROOM %d/%d  %s   HOSTILES %d", director.arena + 1, nArenas, ar.name, left);
            } else switch (director.phase) {
            case WaveDirector::Phase::APPROACH:   // FAST only
            case WaveDirector::Phase::INTRO:
                snprintf(buf, sizeof(buf), "ARENA %d/%d  %s  GET READY", director.arena + 1, nArenas, ar.name); break;
            case WaveDirector::Phase::ACTIVE:
                if (boss) snprintf(buf, sizeof(buf), "ARENA %d/%d  FINAL WAVE", director.arena + 1, nArenas);
                else snprintf(buf, sizeof(buf), "ARENA %d/%d   WAVE %d/%d   HOSTILES %d",
                              director.arena + 1, nArenas, director.wave + 1, director.waveCount(), left);
                break;
            case WaveDirector::Phase::BREAK:
                snprintf(buf, sizeof(buf), "WAVE CLEAR - NEXT WAVE IN %d", (int)std::ceil(director.timer));
                accent = {0.4f, 1.f, 0.6f}; break;
            case WaveDirector::Phase::CLEARED:
                snprintf(buf, sizeof(buf), "ARENA CLEARED - GO THROUGH THE GATE");
                accent = {0.4f, 1.f, 0.6f}; break;
            case WaveDirector::Phase::VICTORY:
                snprintf(buf, sizeof(buf), "VICTORY"); accent = {0.4f, 1.f, 0.6f}; break;
            }
            ui.renderObjective(buf, accent);

            if (boss) ui.renderBossBar(boss->type == EnemyType::SOVEREIGN ? "THE SOVEREIGN" : "THE WARDEN",
                                       boss->health / boss->maxHealth, boss->enraged);

            // Waypoint to the way on: the open gate (ARENA), or the exit of the
            // room you just cleared (FAST) until you're out of it
            int wayDoor = -1;
            if (!fast() && director.phase == WaveDirector::Phase::CLEARED && ar.exitDoor >= 0 &&
                player.position.z > level.doors[ar.exitDoor].closed.min.z) wayDoor = ar.exitDoor;
            if (fast() && director.phase == WaveDirector::Phase::APPROACH && director.arena > 0) {
                const Arena& prev = level.arenas[director.arena - 1];
                const AABB& z = prev.zone;
                if (player.position.x > z.min.x && player.position.x < z.max.x &&
                    player.position.z > z.min.z && player.position.z < z.max.z) wayDoor = prev.exitDoor;
            }
            if (wayDoor >= 0) {
                const Door& dr = level.doors[wayDoor];
                glm::vec3 target = (dr.closed.min + dr.closed.max) * 0.5f;
                target.y = dr.baseY + 2.f;
                float sx, sy;
                bool on = projectToScreen(target, view, proj, sx, sy);
                snprintf(buf, sizeof(buf), fast() ? "EXIT %dM" : "GATE %dM", (int)glm::length(target - player.position));
                ui.renderMarker(sx, sy, on, {0.4f, 1.f, 0.6f}, buf);
            }
            if (finishOpen) {
                glm::vec3 target = level.finishPos + glm::vec3{0, 2.f, 0};
                float sx, sy;
                bool on = projectToScreen(target, view, proj, sx, sy);
                snprintf(buf, sizeof(buf), "FINISH %dM", (int)glm::length(target - player.position));
                ui.renderMarker(sx, sy, on, {1.f, 0.6f, 0.2f}, buf);
            }
            // The last few enemies get markers so you never hunt for a straggler
            if (director.fighting() && director.queued() == 0 && alive > 0 && alive <= 3) {
                for (auto& e : enemies) {
                    if (!e.alive) continue;
                    float sx, sy;
                    bool on = projectToScreen(e.position + glm::vec3{0, e.height() + 0.8f, 0}, view, proj, sx, sy);
                    ui.renderMarker(sx, sy, on, {1.f, 0.3f, 0.25f}, "");
                }
            }
            if (!paused && !armoryOpen)
                ui.renderControlHint(glm::clamp(controlHintTimer / 1.5f, 0.f, 1.f),
                                 GameSettings::grappleLabel(settings ? settings->grappleKey : 0));
        }

        if (!banners.empty() && !armoryOpen) {
            const Banner& b = banners.front();
            float a = std::min({1.f, b.time * 4.f, (b.duration - b.time) * 2.5f});
            ui.renderBanner(b.title.c_str(), b.subtitle.c_str(), b.color, a);
        }
        if (countdown > 0.f) ui.renderCountdown(countdown);
        if (g_practice && !victory && !paused) {
            ui.begin2D();
            ui.ui.textShadow("PRACTICE  F5 CLEAR WAVE  F6 HEAL", 12, SCREEN_H - 22, 1, {0.4f, 1.f, 0.85f, 0.8f});
            ui.end2D();
        }

        if (playerDead) {
            if (fast()) snprintf(buf, sizeof(buf), "ROOM %d/%d  %s", director.arena + 1, nArenas, ar.name);
            else snprintf(buf, sizeof(buf), "ARENA %d/%d %s - WAVE %d/%d", director.arena + 1, nArenas, ar.name,
                          director.wave + 1, director.waveCount());
            ui.renderDeath(buf, totalKills, elapsedTime, fast());
        }
        if (victory) {
            if (fast()) {
                const char* rank = elapsedTime < level.parTimes[0] ? "S" : elapsedTime < level.parTimes[1] ? "A"
                                 : elapsedTime < level.parTimes[2] ? "B" : elapsedTime < level.parTimes[3] ? "C" : "D";
                // Show the best run as it was before this one (newRecord already replaced it)
                static std::vector<float> none;
                ui.renderVictoryFast(elapsedTime, newRecord ? 0.f : records.bestFast, newRecord, rank, totalKills,
                                     totalShots, totalHits, deaths, splits, newRecord ? none : records.fastSplits, !nameEntry);
            } else {
                ui.renderVictoryArena(totalKills, totalShots, totalHits, deaths, elapsedTime, peakStyle,
                                      prog.level, records.bestArena, newRecord, !nameEntry);
            }
            renderLeaderboardPanel();
        }
        if (armoryOpen) ui.renderArmory(prog, armoryW, armoryS);
        if (paused) {
            if (pauseSettings) {
                ui.begin2D();
                ui.ui.rect(0, 0, SCREEN_W, SCREEN_H, {0.f, 0.f, 0.02f, 0.65f});
                settingsMenu.render(ui.ui, gameClock + (float)SDL_GetTicks() * 0.001f);
                ui.end2D();
            } else {
                const char* labels[UIRenderer::PAUSE_ITEMS];
                for (int i = 0; i < UIRenderer::PAUSE_ITEMS; ++i) labels[i] = pauseLabel(i);
                std::string line = fast() ? "FAST - THE GAUNTLET  " + formatTime(elapsedTime)
                                          : std::string("ARENA - ") + ar.name;
                line += std::string("   ") + tune().name;
                ui.renderPause(pauseSelected, labels, line.c_str());
            }
        }
    }

    void renderDecals() {
        struct DVert { float x,y,z, u,v, nx,ny,nz, r,g,b; };
        DVert buf[MAX_DECALS * 6];
        int count = 0;
        for (auto& d : decals) {
            if (!d.alive) continue;
            float fade = d.life / d.maxLife;
            float r = 0.55f * fade, g = 0.03f * fade, b = 0.03f * fade;
            float s = 0.35f;
            float px = d.pos.x, py = d.pos.y, pz = d.pos.z;
            DVert v0{px-s,py,pz-s, 0,0, 0,1,0, r,g,b};
            DVert v1{px+s,py,pz-s, 1,0, 0,1,0, r,g,b};
            DVert v2{px+s,py,pz+s, 1,1, 0,1,0, r,g,b};
            DVert v3{px-s,py,pz+s, 0,1, 0,1,0, r,g,b};
            buf[count++] = v0; buf[count++] = v1; buf[count++] = v2;
            buf[count++] = v0; buf[count++] = v2; buf[count++] = v3;
        }
        if (count == 0) return;
        glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(DVert), buf);
        worldShader.use();
        worldShader.setMat4("model",         glm::mat4(1.f));
        worldShader.setVec3("objectColor",   {1.f,1.f,1.f});
        worldShader.setVec3("emissiveColor", {0.f,0.f,0.f});
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.f, -1.f);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(decalVAO);
        glDrawArrays(GL_TRIANGLES, 0, count);
        glBindVertexArray(0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_CULL_FACE);
    }

    struct PVert { float x, y, z, r, g, b, a; };
    void renderParticles(const glm::mat4& view, const glm::mat4& proj) {
        static PVert buf[MAX_PARTICLES];
        int count = 0;
        for (auto& p : particles) {
            if (!p.alive) continue;
            float t = p.life / p.maxLife;
            buf[count++] = { p.pos.x, p.pos.y, p.pos.z, p.color.r, p.color.g, p.color.b, t * t };
        }
        drawPoints(buf, count, view, proj);
    }
    void drawPoints(const PVert* buf, int count, const glm::mat4& view, const glm::mat4& proj) {
        if (count == 0) return;
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(PVert), buf);

        particleShader.use();
        particleShader.setMat4("projection", proj);
        particleShader.setMat4("view",       view);
        particleShader.setFloat("uPointScale", 60.f);

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);  // always on in GLES/WebGL2
#endif
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, count);
        glBindVertexArray(0);
#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    // The first time OpenGL (on Metal, or WebGL through ANGLE) sees a new
    // shader + blend combination it compiles a pipeline on the spot, which
    // froze the game for ~200 ms the first time you fired, a Sentinel aimed
    // or you scoped in. The first frames of a level draw one invisible
    // instance of each effect so that happens behind the loading screen.
    int warmupFrames = 2;
    void warmPipelines(glm::vec3 camPos, const glm::mat4& view, const glm::mat4& proj) {
        glm::vec3 f = camPos + player.camera.forward() * 30.f;
        std::vector<Beam> beam{{glm::vec4(f, 0.f), glm::vec4(f + glm::vec3{0, 1, 0}, 0.f), 0.01f}};
        drawBeams(beam, {0.f, 0.f, 0.f}, view, proj);
        PVert pt{f.x, f.y, f.z, 0.f, 0.f, 0.f, 0.f};
        drawPoints(&pt, 1, view, proj);
        if (!grapple.active) {   // a zero-length rope
            glm::vec3 keep = grapple.target;
            grapple.active = true;
            grapple.target = camPos + glm::vec3{0.f, 1.4f, 0.f};
            grapple.drawLine(camPos, view, proj);
            grapple.active = false;
            grapple.target = keep;
        }
    }

    // Draws thin additive beams. Shared by bullet tracers and Sentinel lasers.
    // Each beam: start (xyz, alpha), end (xyz, alpha), width.
    struct Beam { glm::vec4 a, b; float width; };
    void drawBeams(const std::vector<Beam>& beams, glm::vec3 color, const glm::mat4& view, const glm::mat4& proj) {
        struct TVert { float x,y,z,a; };
        TVert buf[MAX_TRACERS * 6];
        int count = 0;
        for (auto& bm : beams) {
            if (count + 6 > MAX_TRACERS * 6) break;
            glm::vec3 s = glm::vec3(bm.a), e = glm::vec3(bm.b);
            glm::vec3 ray = e - s;
            float len = glm::length(ray);
            if (len < 0.001f) continue;
            glm::vec3 dir = ray / len;
            // Side axis perpendicular to the ray (cross with world up; fall back to X)
            glm::vec3 side = glm::cross(dir, glm::vec3(0.f, 1.f, 0.f));
            if (glm::length(side) < 0.01f) side = glm::cross(dir, glm::vec3(1.f, 0.f, 0.f));
            side = glm::normalize(side) * bm.width;
            float aN = bm.a.w, aF = bm.b.w;
            glm::vec3 s0 = s - side, s1 = s + side, e0 = e - side, e1 = e + side;
            buf[count++] = {s0.x,s0.y,s0.z, aN}; buf[count++] = {s1.x,s1.y,s1.z, aN};
            buf[count++] = {e1.x,e1.y,e1.z, aF}; buf[count++] = {s0.x,s0.y,s0.z, aN};
            buf[count++] = {e1.x,e1.y,e1.z, aF}; buf[count++] = {e0.x,e0.y,e0.z, aF};
        }
        if (count == 0) return;
        glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(TVert), buf);
        tracerShader.use();
        tracerShader.setMat4("projection", proj);
        tracerShader.setMat4("view",       view);
        tracerShader.setVec3("uColor",     color);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindVertexArray(tracerVAO);
        glDrawArrays(GL_TRIANGLES, 0, count);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    void renderTracers(const glm::mat4& view, const glm::mat4& proj) {
        static std::vector<Beam> beams;
        beams.clear();
        for (auto& t : tracers) {
            if (!t.alive) continue;
            float fade = t.life / t.maxLife;
            beams.push_back({glm::vec4(t.start, fade), glm::vec4(t.end, fade * 0.08f), t.width});
        }
        drawBeams(beams, {0.97f, 0.95f, 0.72f}, view, proj);
    }

    // A Sentinel winding up paints you with a laser; it brightens until it fires.
    void renderLasers(const glm::mat4& view, const glm::mat4& proj) {
        static std::vector<Beam> beams;
        beams.clear();
        glm::vec3 target = player.camera.position - glm::vec3{0, 0.35f, 0};
        for (auto& e : enemies) {
            if (!e.targetable() || e.type != EnemyType::SENTINEL || e.attack != AttackKind::BURST) continue;
            float k = e.telegraphProgress();
            float a = 0.25f + 0.75f * k;
            if (k > 0.8f) a *= 0.5f + 0.5f * std::sin(gameClock * 60.f);
            glm::vec3 eye = e.position + glm::vec3{0, e.height() * 0.85f, 0};
            // Stop a few metres short so the beam doesn't smear across the screen
            glm::vec3 d = target - eye;
            float len = glm::length(d);
            if (len < 4.f) continue;
            beams.push_back({glm::vec4(eye, a), glm::vec4(eye + d * ((len - 3.f) / len), a * 0.5f), 0.03f});
        }
        drawBeams(beams, {0.3f, 0.95f, 1.f}, view, proj);
    }

    int findInteractTarget() const {
        glm::vec3 origin = player.camera.position;
        glm::vec3 dir    = player.camera.forward();
        int   best    = -1;
        float bestT   = Interactable::INTERACT_RANGE;
        for (int i = 0; i < (int)interactables.size(); ++i) {
            if (!interactables[i].active) continue;
            float t = rayBoxHit(origin, dir, interactables[i].box);
            if (t > 0.f && t < bestT) { bestT = t; best = i; }
        }
        return best;
    }
};
