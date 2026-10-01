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
#include "WaveDirector.h"
#include "PostProcess.h"
#include "AudioSystem.h"
#include "ViewModel.h"
#include "Interactable.h"
#include "Settings.h"
#include "TextureGen.h"
#include <SDL2/SDL.h>
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
// the player inside the arena they're fighting in.
//
// KEY EXTENSION POINTS:
//   ADD A WEAPON:  write a fire___() modelled on fireRevolver(); hitscan uses
//                  hitscan(), damage goes through hurtEnemy().
//   ADD AN ENEMY:  Enemy.h (stats + AI), EnemyModel.h (its box rig), then list
//                  it in a wave in Level.h.
//   CHANGE A MAP:  Level.h. Walls render and collide automatically.
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

// Static level geometry, batched by texture: walkable tops + floors (grid),
// vertical sides (brick), undersides (metal) and self-lit neon (no texture).
struct WorldMeshes { Mesh floor, walls, under, neon; };

static WorldMeshes buildWorldMeshes(const LevelData& L) {
    std::vector<Vertex> fV, wV, uV, nV;
    std::vector<unsigned int> fI, wI, uI, nI;
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

    auto pushBox = [&](const AABB& b, glm::vec3 col, bool neon) {
        glm::vec3 mn = b.min, mx = b.max;
        auto& sv = neon ? nV : wV; auto& si = neon ? nI : wI;
        auto& tv = neon ? nV : fV; auto& ti = neon ? nI : fI;
        auto& bv = neon ? nV : uV; auto& bi = neon ? nI : uI;
        bool shade = !neon;
        pushFace(sv, si, {mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{ 1, 0, 0}, col, shade);
        pushFace(sv, si, {mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1, 0, 0}, col, shade);
        pushFace(sv, si, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{ 0, 0, 1}, col, shade);
        pushFace(sv, si, {mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{ 0, 0,-1}, col, shade);
        pushFace(tv, ti, {mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},{ 0, 1, 0}, col, false);
        if (b.min.y > 0.1f || neon)
            pushFace(bv, bi, {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z},{ 0,-1, 0}, col, false);
    };

    for (auto& f : L.floors)
        pushFace(fV, fI, {f.x0,f.y,f.z1},{f.x1,f.y,f.z1},{f.x1,f.y,f.z0},{f.x0,f.y,f.z0},{0,1,0}, f.color, false);
    for (int i = 0; i < (int)L.walls.size(); ++i)
        if (!L.walls[i].hidden && !L.isDoorWall(i)) pushBox(L.walls[i].box, L.walls[i].color, false);
    for (auto& p : L.props) pushBox(p.box, p.color, false);
    for (auto& n : L.neon)  pushBox(n.box, n.color, true);

    WorldMeshes m;
    m.floor.upload(fV, fI);
    m.walls.upload(wV, wI);
    m.under.upload(uV, uI);
    m.neon.upload(nV, nI);
    return m;
}

class GameplayState : public GameState {
public:
    std::function<void()> onReturnToMenu;
    std::function<void()> onQuit;

    GameSettings* settings = nullptr;   // injected by main — may be null (safe)

    Player           player{{0.f,0.f,24.f}};
    StyleSystem      styleSystem;
    UIRenderer       ui{SCREEN_W,SCREEN_H};
    ProjectileSystem projSystem;
    GrappleHook      grapple;
    LevelData        level;
    WaveDirector     director;
    AudioSystem&     audio;
    PostProcess      postProcess{SCREEN_W,SCREEN_H};

    ShaderProgram  worldShader;
    ShaderProgram  skyboxShader;
    BoxRenderer    boxRenderer;

    GLuint      whiteTex = 0, floorTex = 0, wallTex = 0, ceilTex = 0;
    WorldMeshes world;

    std::vector<Enemy> enemies;
    SpatialGrid        spatialGrid;   // built once; door boxes move only in Y

    // Slot 1: Revolver — 8 rounds, manual/auto reload
    int   revolverAmmo    = 8;
    int   revolverAmmoMax = 8;
    float revolverTimer   = 0.f;
    bool  reloading       = false;
    float reloadTimer     = 0.f;
    static constexpr float RELOAD_TIME = 1.2f;
    // Slot 2: Shotgun — 2 shells, pump-action
    int   shotgunAmmo      = 2;
    int   shotgunAmmoMax   = 2;
    float shotgunTimer     = 0.f;
    bool  shotgunReloading = false;
    float shotgunReloadTimer = 0.f;
    static constexpr float SHOTGUN_RELOAD_TIME = 1.4f;
    // G key: Grenades — max 2, one returned per 2 kills
    int   grenadeCount   = 2;
    int   grenadeMax     = 2;
    float grenadeTimer   = 0.f;
    int   killsThisCycle = 0;
    int   activeWeapon   = 0;
    int   pendingWeapon  = -1;
    float weaponSwitchTimer = 0.f;

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

    // Title cards, shown one after another
    struct Banner { std::string title, subtitle; glm::vec3 color; float time, duration; };
    std::deque<Banner> banners;

    float controlHintTimer = 9.f;
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

    // Hitscan tracer — thin billboard quad, additive blending, fades fast
    struct Tracer {
        glm::vec3 start{0.f}, end{0.f};
        float life = 0.f, maxLife = 0.13f;
        bool  alive = false;
    };
    static constexpr int MAX_TRACERS = 24;
    Tracer        tracers[MAX_TRACERS];
    ShaderProgram tracerShader;
    GLuint        tracerVAO = 0, tracerVBO = 0;
    ShaderProgram particleShader;

    ViewModel viewModel;
    std::vector<Interactable> interactables;
    bool  nearInteractable = false;

    float fovKick        = 0.f;
    float playerXZSpeed  = 0.f;
    float peakFallSpeed  = 0.f;
    float landSquash     = 0.f;

    bool  paused        = false;
    int   pauseSelected = 0;
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
    GLuint    particleVAO = 0, particleVBO = 0;
    float     ambientTimer = 0.f;

    // Enemies come apart into their boxes when they die
    struct Debris {
        glm::mat3 shape;          // rotation * scale of the original part
        glm::vec3 pos, vel, axis, color, emissive;
        float angle = 0.f, spin = 0.f, life = 0.f, maxLife = 1.f;
    };
    std::vector<Debris> debris;

    struct Pickup { glm::vec3 pos, vel; float life; };   // health orbs
    std::vector<Pickup> pickups;

    struct Shockwave { glm::vec3 pos; float radius, t; glm::vec3 color; };
    std::vector<Shockwave> shockwaves;

    struct Blast { glm::vec3 pos; float radius, damage; float playerRadius, playerDamage; };
    std::vector<Blast> pendingBlasts;

    float padCooldown  = 0.f;
    float hazardTick   = 0.f;
    float gameClock    = 0.f;     // drives decor animation

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
    bool prevDashKey      = false;
    bool prevJumpKey      = false;
    // Event-driven click flags — set in handleEvent, consumed once in physicsTick.
    bool pendingFire      = false;
    bool pendingShotgun   = false;
    bool pendingGrenade   = false;
    bool pendingGrapple   = false;

    GameplayState(AudioSystem& aud, GameSettings* s = nullptr) : settings(s), audio(aud) {
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
        floorTex = TextureGen::generateGridFloor(128);
        wallTex  = TextureGen::generateBrickWall(128);
        ceilTex  = TextureGen::generateMetalCeiling(128);

        level = buildLevel();
        spatialGrid.build(level.walls);
        world = buildWorldMeshes(level);
        director.level = &level;

        for (int i=0;i<MAX_POINT_LIGHTS;++i) {
            pointLightPos[i]   = {0,0,0};
            pointLightColor[i] = {0,0,0};
        }

        int start = glm::clamp(g_startArena, 0, (int)level.arenas.size() - 1);
        enterArena(start);
        director.wave = glm::clamp(g_startWave, 0, director.waveCount() - 1);

        prevTicks = SDL_GetPerformanceCounter();
        freq      = SDL_GetPerformanceFrequency();
        prevCamPos = player.camera.position;

        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    ~GameplayState() {
        glDeleteTextures(1,&whiteTex);
        glDeleteTextures(1,&floorTex);
        glDeleteTextures(1,&wallTex);
        glDeleteTextures(1,&ceilTex);
        if (tracerVAO)   glDeleteVertexArrays(1, &tracerVAO);
        if (tracerVBO)   glDeleteBuffers(1, &tracerVBO);
        if (particleVAO) glDeleteVertexArrays(1, &particleVAO);
        if (particleVBO) glDeleteBuffers(1, &particleVBO);
        if (decalVAO)    glDeleteVertexArrays(1, &decalVAO);
        if (decalVBO)    glDeleteBuffers(1, &decalVBO);
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    // =========================================================================
    // Run / arena lifecycle
    // =========================================================================

    // Put the player at the start of arena `a` with the doors set as if they
    // had just walked in. Used for a fresh run, a retry after death, and --arena.
    void enterArena(int a) {
        for (int i = 0; i < (int)level.arenas.size(); ++i) {
            const Arena& ar = level.arenas[i];
            if (ar.exitDoor  >= 0) setDoor(ar.exitDoor,  i < a, true);
            if (ar.entryGate >= 0) setDoor(ar.entryGate, i != a, true);
        }
        resetPlayer(level.arenas[a].playerStart);
        enemies.clear();
        for (auto& p : projSystem.pool) p.alive = false;
        for (auto& d : decals)          d.alive = false;
        for (auto& p : particles)       p.alive = false;
        debris.clear(); pickups.clear(); shockwaves.clear(); pendingBlasts.clear();
        banners.clear();
        grapple.release();
        playerDead = false; deadTimer = 0.f;
        victory = false; victoryDelay = -1.f;
        director.startArena(a);
    }

    void resetPlayer(glm::vec3 start) {
        player = Player{start};
        player.camera.aspectRatio = (float)SCREEN_W/SCREEN_H;
        player.camera.fov = settings ? settings->fov : 90.f;
        prevCamPos = player.camera.position;
        styleSystem = StyleSystem{};
        grenadeCount = grenadeMax; killsThisCycle = 0;
        revolverAmmo = revolverAmmoMax; reloading = false; reloadTimer = 0.f; revolverTimer = 0.f;
        shotgunAmmo = shotgunAmmoMax; shotgunReloading = false; shotgunReloadTimer = 0.f; shotgunTimer = 0.f;
        dashCharges = 2; dashCooldown = 0.f; dashMomentumTimer = 0.f;
        jumpsRemaining = 2; slamming = false; invincFrames = 0.f;
        activeWeapon = 0; pendingWeapon = -1; weaponSwitchTimer = 0.f;
        recoilPitch = 0.f; peakFallSpeed = 0.f; landSquash = 0.f; fovKick = 0.f;
        paused = false; pauseSelected = 0;
        for (auto& di : ui.damageIndicators) di.timer = 0.f;
        ui.damageIndicatorCount = 0;
    }

    void retryArena() {
        ++deaths;
        // Died on the way out of a cleared arena? Pick up at the next one.
        bool cleared = director.phase == WaveDirector::Phase::CLEARED;
        enterArena(director.arena + (cleared ? 1 : 0));
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    void newRun() {
        director = WaveDirector{};
        director.level = &level;
        totalKills = totalShots = totalHits = deaths = 0;
        elapsedTime = 0.f; peakStyle = 0.f;
        controlHintTimer = 9.f;
        enterArena(0);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    void setDoor(int d, bool open, bool instant) {
        Door& door = level.doors[d];
        door.open = open;
        if (instant) door.openAmount = open ? 1.f : 0.f;
        applyDoor(door);
    }
    void applyDoor(const Door& door) {
        AABB& b = level.walls[door.wall].box;
        b.max.y = door.height * (1.f - door.openAmount);
        b.min.y = b.max.y - door.height;
    }

    void pushBanner(const std::string& title, const std::string& sub, glm::vec3 col, float dur = 3.f) {
        banners.push_back({title, sub, col, 0.f, dur});
    }

    // Turn director events into title cards, doors and rewards.
    void handleDirectorEvents() {
        for (auto& ev : director.events) {
            const Arena& ar = level.arenas[director.arena];
            char buf[96];
            switch (ev.kind) {
            case DirectorEvent::ARENA_START:
                if (ar.entryGate >= 0) setDoor(ar.entryGate, false, false);
                snprintf(buf, sizeof(buf), "ARENA %d/%d", ev.value + 1, (int)level.arenas.size());
                pushBanner(std::string(buf) + "  " + ar.name, ar.subtitle, {1.f, 0.78f, 0.3f}, 2.6f);
                audio.play("wave");
                break;
            case DirectorEvent::WAVE_START:
                snprintf(buf, sizeof(buf), "WAVE %d/%d", ev.value + 1, director.waveCount());
                pushBanner(buf, "", {1.f, 0.9f, 0.4f}, 1.8f);
                audio.play("wave", 90);
                break;
            case DirectorEvent::BOSS_START:
                pushBanner("THE WARDEN", "DODGE THE VOLLEYS, JUMP THE SLAMS", {1.f, 0.2f, 0.65f}, 3.5f);
                audio.play("wave"); audio.play("explosion", 70);
                shakeTimer = 0.6f; shakeIntensity = 0.06f;
                break;
            case DirectorEvent::NEW_TYPE: {
                EnemyType t = (EnemyType)ev.value;
                if (t == EnemyType::WARDEN) break;
                pushBanner(std::string("NEW: ") + statsOf(t).name, statsOf(t).hint, statsOf(t).glow, 3.4f);
                break;
            }
            case DirectorEvent::WAVE_CLEARED:
                pushBanner("WAVE CLEAR", "", {0.4f, 1.f, 0.6f}, 1.6f);
                styleSystem.heal(10.f);
                break;
            case DirectorEvent::ARENA_CLEARED:
                if (ar.exitDoor >= 0) {
                    setDoor(ar.exitDoor, true, false);
                    pushBanner("ARENA CLEARED", "THE GATE IS OPEN - HEAD NORTH", {0.4f, 1.f, 0.6f}, 3.5f);
                }
                styleSystem.heal(40.f);
                grenadeCount = grenadeMax;
                audio.play("wave");
                break;
            case DirectorEvent::VICTORY:
                victoryDelay = 2.5f;
                break;
            }
        }
        director.events.clear();
    }

    // =========================================================================
    // Input
    // =========================================================================
    void handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_p)) {
            if (playerDead || victory) {
                if (e.key.keysym.sym != SDLK_ESCAPE) return;  // P only pauses
                SDL_SetRelativeMouseMode(SDL_FALSE);
                if (onReturnToMenu) onReturnToMenu();
                return;
            }
            paused = !paused;
            pauseSelected = 0;
            SDL_SetRelativeMouseMode(paused ? SDL_FALSE : SDL_TRUE);
            return;
        }
        if (paused) {
            handlePauseEvent(e);
            return;
        }
        if (e.type == SDL_KEYDOWN && (playerDead || victory)) {
            if (e.key.keysym.sym == SDLK_r && playerDead) { retryArena(); return; }
            if (e.key.keysym.sym == SDLK_RETURN)          { newRun();     return; }
            return;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_e) {
            int idx = findInteractTarget();
            if (idx >= 0 && interactables[idx].onInteract) interactables[idx].onInteract();
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                if (activeWeapon == 0) pendingFire    = true;
                else                   pendingShotgun = true;
            }
            if (e.button.button == SDL_BUTTON_RIGHT) pendingGrapple = true;
        }
        auto trySwitch = [&](int newWeapon) {
            if (newWeapon != activeWeapon && pendingWeapon < 0) {
                pendingWeapon = newWeapon;
                weaponSwitchTimer = 0.15f;
                viewModel.triggerSwitch();
                audio.play("reload", 80);
            }
        };
        if (e.type == SDL_MOUSEWHEEL) trySwitch((activeWeapon + 1) % 2);
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_1) trySwitch(0);
            if (e.key.keysym.sym == SDLK_2) trySwitch(1);
            if (e.key.keysym.sym == SDLK_g) pendingGrenade = true;
        }
        if (e.type == SDL_MOUSEMOTION) {
            float sens = settings ? settings->sensitivity : 0.1f;
            player.applyMouseLook((float)e.motion.xrel, (float)e.motion.yrel, sens);
        }
    }

    // Pause a live run (no-op on death/win screens or if already paused).
    void pause() {
        if (paused || playerDead || victory) return;
        paused = true;
        pauseSelected = 0;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    int pauseButtonY(int i) const { return SCREEN_H/2 - 20 + i * 60; }

    void activatePauseItem(int idx) {
        if (idx == 0) {
            paused = false;
            SDL_SetRelativeMouseMode(SDL_TRUE);
        } else {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            if (onReturnToMenu) onReturnToMenu();
        }
    }

    void handlePauseEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_DOWN:
                    pauseSelected = 1 - pauseSelected; break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    activatePauseItem(pauseSelected); break;
                default: break;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x, my = e.button.y;
            for (int i = 0; i < 2; ++i) {
                int by = pauseButtonY(i);
                if (mx > SCREEN_W/2-130 && mx < SCREEN_W/2+130 && my > by && my < by+46)
                    activatePauseItem(i);
            }
        }
        if (e.type == SDL_MOUSEMOTION) {
            int mx = e.motion.x, my = e.motion.y;
            for (int i = 0; i < 2; ++i) {
                int by = pauseButtonY(i);
                if (mx > SCREEN_W/2-130 && mx < SCREEN_W/2+130 && my > by && my < by+46)
                    pauseSelected = i;
            }
        }
    }

    // =========================================================================
    // Per-frame update
    // =========================================================================
    void update(float dt) override {
        // Banners keep animating on the death/victory screens, not while paused
        if (!banners.empty() && !paused) {
            banners.front().time += dt;
            if (banners.front().time >= banners.front().duration) banners.pop_front();
        }
        if (paused || playerDead || victory) {
            // Keep the physics clock current so resuming doesn't replay the
            // whole paused interval as a burst of catch-up ticks.
            prevTicks   = SDL_GetPerformanceCounter();
            accumulator = 0.0;
            if (playerDead) deadTimer += dt;
            return;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed = (double)(now - prevTicks) / (double)freq;
        prevTicks = now;
        accumulator += elapsed;
        if (accumulator > 0.25) accumulator = 0.25;

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        bool parryKey = keys[SDL_SCANCODE_F] != 0;

        while (accumulator >= PHYSICS_DT) {
            prevCamPos = player.camera.position;
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
        ui.update(floatDt, styleSystem);
        if (controlHintTimer > 0.f) controlHintTimer -= floatDt;

        // Doors slide toward their target
        for (auto& d : level.doors) {
            float target = d.open ? 1.f : 0.f;
            if (d.openAmount != target) {
                float step = floatDt * (d.open ? 0.7f : 1.6f);
                d.openAmount = d.open ? std::min(1.f, d.openAmount + step) : std::max(0.f, d.openAmount - step);
                applyDoor(d);
            }
        }

        viewModel.update(floatDt, playerXZSpeed, player.onGround);
        // Baseline FOV widens with horizontal speed on top of the dash kick
        float speedKick = glm::clamp((playerXZSpeed - player.horizontalSpeed) / 15.f, 0.f, 1.f) * 6.f;
        fovKick = glm::mix(fovKick, speedKick, std::min(1.f, floatDt * 7.f));
        landSquash = glm::mix(landSquash, 0.f, std::min(1.f, floatDt * 10.f));

        if (pendingWeapon >= 0) {
            weaponSwitchTimer -= floatDt;
            if (weaponSwitchTimer <= 0.f) {
                activeWeapon  = pendingWeapon;
                pendingWeapon = -1;
            }
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
            if (victoryDelay <= 0.f) { victory = true; SDL_SetRelativeMouseMode(SDL_FALSE); }
        }
        if (!styleSystem.isAlive() && !playerDead) {
            playerDead = true;
            deadTimer = 0.f;
            grapple.release();
        }
    }

    // =========================================================================
    // Fixed-rate simulation (60 Hz)
    // =========================================================================
    void physicsTick(float dt, const Uint8* keys, bool parryKey) {
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
                shakeTimer = 0.3f;
                shakeIntensity = 0.08f;
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
        if (pendingGrapple) {
            glm::vec3 camPos = player.camera.position;
            glm::vec3 camFwd = player.camera.forward();
            if (!grapple.active) {
                glm::vec3 grappleImpulse{0.f};
                if (grapple.fire(camPos, camFwd, level.walls.data(), (int)level.walls.size(), grappleImpulse)) {
                    player.velocity = grappleImpulse;
                    viewModel.triggerGrapple();
                    audio.play("grapple_fire");
                }
            } else {
                grapple.release();
            }
            pendingGrapple = false;
        }
        grapple.update(dt, player.camera.position, player.velocity);

        // --- Player physics ---
        player.update(dt, keys, level.walls.data(), (int)level.walls.size(),
                      grapple.active || dashMomentumTimer > 0.f, &spatialGrid);
        keepPlayerInZone();
        pushPlayerOutOfEnemies();
        playerXZSpeed = glm::length(glm::vec2(player.velocity.x, player.velocity.z));
        if (!player.onGround) peakFallSpeed = std::max(peakFallSpeed, -player.velocity.y);

        // --- Jump pads ---
        padCooldown = std::max(0.f, padCooldown - dt);
        for (auto& pad : level.pads) {
            glm::vec3 p = player.position;
            if (padCooldown <= 0.f && player.velocity.y <= 0.5f && p.y < pad.centre.y + 0.4f &&
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
            if (p.y < hz.box.max.y + 0.3f && p.x > hz.box.min.x && p.x < hz.box.max.x &&
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
                audio.play("land", 40);
            }
        } else {
            footstepTimer = 0.f;
        }
        if (justLanded && !slamming) audio.play("land", 70);

        // --- Weapons ---
        revolverTimer    = std::max(0.f, revolverTimer    - dt);
        shotgunTimer     = std::max(0.f, shotgunTimer     - dt);
        grenadeTimer     = std::max(0.f, grenadeTimer     - dt);
        invincFrames     = std::max(0.f, invincFrames     - dt);

        if (reloading) {
            reloadTimer -= dt;
            if (reloadTimer <= 0.f) { reloading = false; reloadTimer = 0.f; revolverAmmo = revolverAmmoMax; }
        }
        if (shotgunReloading) {
            shotgunReloadTimer -= dt;
            if (shotgunReloadTimer <= 0.f) { shotgunReloading = false; shotgunReloadTimer = 0.f; shotgunAmmo = shotgunAmmoMax; }
        }
        if (keys[SDL_SCANCODE_R]) {
            if (activeWeapon == 0 && !reloading && revolverAmmo < revolverAmmoMax) startReload();
            else if (activeWeapon == 1 && !shotgunReloading && shotgunAmmo < shotgunAmmoMax) startShotgunReload();
        }

        bool switchBlocked = (pendingWeapon >= 0);
        if (pendingFire && revolverTimer <= 0.f && !reloading && revolverAmmo > 0 && !switchBlocked)
            fireRevolver();
        pendingFire = false;
        if (pendingShotgun && shotgunTimer <= 0.f && !shotgunReloading && shotgunAmmo > 0 && !switchBlocked)
            fireShotgun();
        pendingShotgun = false;
        if (pendingGrenade && grenadeCount > 0 && grenadeTimer <= 0.f) throwGrenade();
        pendingGrenade = false;

        if (recoilPitch > 0.f) {
            float applied = std::min(12.f * dt, recoilPitch);
            player.camera.pitch -= applied;
            recoilPitch -= applied;
            player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);
        }

        // --- Waves ---
        int alive = 0;
        for (auto& e : enemies) if (e.alive) ++alive;
        std::vector<SpawnRequest> spawns;
        director.update(dt, alive, player.position, spawns);
        for (auto& s : spawns) spawnEnemy(s.type, s.pos);

        // --- Enemies ---
        updateEnemies(dt);

        // --- Projectiles ---
        auto result = projSystem.update(dt, level.walls.data(), (int)level.walls.size(),
                                        enemies, player.camera.position, &spatialGrid);

        bool parryPressed = parryKey && !parryPrev;
        parryPrev = parryKey;
        if (parryPressed) {
            if (result.parryableIndex >= 0) {
                // Deflect an enemy projectile back at high speed.
                auto& p = projSystem.pool[result.parryableIndex];
                p.isPlayer      = true;
                p.damage        = 50.f;
                p.velocity      = -p.velocity * 2.f;
                p.emissiveColor = {1.f, 0.9f, 0.3f};
                styleSystem.addStyle(20.f);
                invincFrames  = 0.5f;
                hitStopFrames = glm::max(hitStopFrames, 1);
                audio.play("parry");
            } else if (result.boostableIndex >= 0) {
                // Projectile boost: detonate your own projectile for a massive explosion.
                auto& p = projSystem.pool[result.boostableIndex];
                float r = glm::max(p.blastRadius, 4.f) * 2.f;
                pendingBlasts.push_back({p.position, r, p.damage * 3.f, 0.f, 0.f});
                p.alive = false;
                styleSystem.addStyle(40.f);
                invincFrames  = 0.6f;
                hitStopFrames = glm::max(hitStopFrames, 2);
                audio.play("parry");
            }
        }

        for (auto& [pi, ei] : result.enemyHits) {
            auto& e = enemies[ei];
            hurtEnemy(e, projSystem.pool[pi].damage, projSystem.pool[pi].position, 10.f, 2.f);
        }
        for (auto& exp : result.explosions)
            pendingBlasts.push_back({exp.pos, exp.radius, exp.damage, exp.radius * 0.5f, 20.f});
        processBlasts();

        if (result.hitPlayer) damagePlayer(result.playerDamage, result.playerHitFrom, 0.2f, 0.04f);

        updatePickups(dt);

        if (shakeTimer > 0.f) shakeTimer -= dt;
        nearInteractable = findInteractTarget() >= 0;
        for (auto& t : tracers) if (t.alive) { t.life -= dt; if (t.life <= 0.f) t.alive = false; }
        for (auto& d : decals)  if (d.alive) { d.life -= dt; if (d.life <= 0.f) d.alive = false; }

        // Clear out dead enemies once nothing references them by index
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                      [](const Enemy& e){ return !e.alive; }), enemies.end());
    }

    // The player may only be inside the arena being fought — plus, once it's
    // cleared, the corridor and the next arena. Walls are low enough to stand
    // on, so this (not wall height) is what stops you leaving the map.
    void keepPlayerInZone() {
        std::vector<AABB> zones{ level.arenas[director.arena].zone };
        if (director.phase == WaveDirector::Phase::CLEARED) {
            zones.push_back(level.corridors[director.arena]);
            zones.push_back(level.arenas[director.arena + 1].zone);
        }
        glm::vec3& p = player.position;
        float bestD = 1e9f; glm::vec2 best{p.x, p.z};
        for (auto& z : zones) {
            glm::vec2 c{glm::clamp(p.x, z.min.x, z.max.x), glm::clamp(p.z, z.min.z, z.max.z)};
            float d = glm::length(c - glm::vec2{p.x, p.z});
            if (d < bestD) { bestD = d; best = c; }
        }
        if (bestD > 0.f) {
            if (best.x != p.x) player.velocity.x = 0.f;
            if (best.y != p.z) player.velocity.z = 0.f;
            p.x = best.x; p.z = best.y;
            player.camera.position = p + glm::vec3{0, player.eyeHeight, 0};
        }
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

    void spawnEnemy(EnemyType t, glm::vec3 pos) {
        enemies.push_back(Enemy(t, pos));
        glm::vec3 c = statsOf(t).glow;
        spawnBurst(pos + glm::vec3{0, 0.3f, 0}, c, 14, 3.f, 0.7f, -6.f);
        float d = glm::length(pos - player.position);
        audio.play("spawn", (int)glm::clamp(110.f - d * 2.f, 25.f, 110.f));
    }

    void updateEnemies(float dt) {
        const Arena& ar = level.arenas[director.arena];
        EnemyWorld w;
        w.playerEye  = player.camera.position;
        w.playerFeet = player.position;
        w.walls      = level.walls.data();
        w.wallCount  = (int)level.walls.size();
        w.grid       = &spatialGrid;
        w.bounds     = ar.bounds;

        // Iterate by index: summons push_back into `enemies` mid-loop
        size_t n = enemies.size();
        for (size_t i = 0; i < n; ++i) {
            Enemy& e = enemies[i];
            if (!e.alive) continue;
            e.update(dt, w);
            const EnemyEvents ev = e.ev;   // copy: spawning below may reallocate
            glm::vec3 epos = e.position;
            float dist = glm::length(epos - player.position);

            if (ev.telegraphStarted)
                audio.play("telegraph", (int)glm::clamp(128.f - dist * 3.f, 20.f, 128.f));
            for (int k = 0; k < ev.shots; ++k)
                projSystem.fire(ev.shotOrigin, ev.shotDir[k] * ev.shotSpeed, ev.shotDamage * ar.damageScale, false,
                                enemies[i].stats().shotColor, false, 0.f, ev.shotSize);
            if (ev.meleeHit) {
                if (damagePlayer(ev.meleeDamage * ar.damageScale, epos, 0.25f, 0.06f)) {
                    glm::vec3 away = player.position - epos; away.y = 0.f;
                    if (glm::length(away) > 0.001f)
                        player.velocity += glm::normalize(away) * 8.f + glm::vec3{0, 3.f, 0};
                }
            }
            if (ev.slam) {
                spawnShockwave(epos, ev.slamRadius, statsOf(enemies[i].type).glow);
                shakeTimer = 0.35f; shakeIntensity = 0.07f;
                audio.play("slam");
                glm::vec2 flat{player.position.x - epos.x, player.position.z - epos.z};
                bool grounded = player.position.y < epos.y + 0.9f;   // jump it to dodge
                if (glm::length(flat) < ev.slamRadius && grounded) {
                    if (damagePlayer(ev.slamDamage * ar.damageScale, epos, 0.3f, 0.08f) && glm::length(flat) > 0.01f)
                        player.velocity += glm::vec3{flat.x, 0.f, flat.y} / glm::length(flat) * 10.f + glm::vec3{0, 6.f, 0};
                }
            }
            if (ev.detonated) {
                // A mite that reached you: hurts you AND its friends
                pendingBlasts.push_back({epos + glm::vec3{0, 0.3f, 0}, 4.f, 30.f, 4.f, 24.f * ar.damageScale});
                spawnDebrisFor(enemies[i]);
            }
            if (ev.summonMites + ev.summonRippers > 0) {
                int total = ev.summonMites + ev.summonRippers;
                for (int k = 0; k < total; ++k) {
                    float a = k * 6.2832f / total + gameClock;
                    glm::vec3 p = epos + glm::vec3{std::cos(a) * 3.5f, 0.f, std::sin(a) * 3.5f};
                    p.x = glm::clamp(p.x, ar.bounds.min.x + 1.f, ar.bounds.max.x - 1.f);
                    p.z = glm::clamp(p.z, ar.bounds.min.z + 1.f, ar.bounds.max.z - 1.f);
                    p.y = 0.f;
                    spawnEnemy(k < ev.summonMites ? EnemyType::MITE : EnemyType::RIPPER, p);
                }
            }
            if (ev.enraged) {
                pushBanner("THE WARDEN IS ENRAGED", "", {1.f, 0.15f, 0.25f}, 2.f);
                shakeTimer = 0.5f; shakeIntensity = 0.06f;
                audio.play("wave");
            }
        }

        // Lava burns enemies too: lure them in
        for (auto& e : enemies) {
            if (!e.targetable() || e.stats().flying) continue;
            for (auto& hz : level.hazards)
                if (e.position.y < hz.box.max.y + 0.3f && e.position.x > hz.box.min.x && e.position.x < hz.box.max.x &&
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
                if (!a.stats().flying) d.y = 0.f;
                float len = glm::length(d), minD = a.radius() + b.radius();
                if (len >= minD || len < 1e-4f) continue;
                glm::vec3 n = d / len * (minD - len);
                float wa = b.radius() / minD, wb = a.radius() / minD;   // big ones get pushed less
                a.position -= n * wa; b.position += n * wb;
            }
        }
    }

    // Returns true if damage was applied (not blocked by i-frames).
    bool damagePlayer(float dmg, glm::vec3 from, float shake, float shakeAmt) {
        if (invincFrames > 0.f || playerDead || g_godMode ||
            director.phase == WaveDirector::Phase::VICTORY) return false;
        styleSystem.takeDamage(dmg);
        ui.onDamage();
        showDamageFrom(from);
        shakeTimer = shake; shakeIntensity = shakeAmt;
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
    bool hurtEnemy(Enemy& e, float dmg, glm::vec3 at, float style, float heal) {
        if (!e.targetable()) return false;
        bool killed = e.takeDamage(dmg);
        styleSystem.addStyle(style);
        styleSystem.heal(heal);
        audio.play("hit");
        if (!e.stats().flying) spawnDecal(e.position);
        spawnHitSparks(at, e.stats().color * 1.4f);
        ui.onHit(killed);
        if (killed) {
            onEnemyKilled(e);
            hitStopFrames = glm::max(hitStopFrames, e.type == EnemyType::WARDEN ? 12 : 2);
        }
        return killed;
    }

    void onEnemyKilled(Enemy& e) {
        ++totalKills;
        styleSystem.addStyle(30.f);
        styleSystem.heal(5.f);
        audio.play("enemy_death");
        spawnDeathParticles(e.position + glm::vec3{0, e.height() * 0.5f, 0}, e.stats().color);
        spawnDebrisFor(e);
        if (styleSystem.overdrive) dashCharges = 2;
        if (++killsThisCycle >= 2) {
            killsThisCycle = 0;
            if (grenadeCount < grenadeMax) { grenadeCount++; ui.onGrenadeRefill(); }
        }

        // Health orbs: Brutes always drop a handful, others sometimes
        int orbs = 0;
        switch (e.type) {
            case EnemyType::BRUTE:  orbs = 3; break;
            case EnemyType::MITE:   orbs = (rand() % 10 == 0); break;
            case EnemyType::WARDEN: orbs = 0; break;
            default:                orbs = (rand() % 100 < 28); break;
        }
        for (int i = 0; i < orbs; ++i)
            pickups.push_back({e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.5f), 0},
                               glm::vec3{frand(-3.f, 3.f), frand(3.f, 6.f), frand(-3.f, 3.f)}, 20.f});

        if (e.type == EnemyType::MITE)          // shot mites still pop — but only hurt enemies
            pendingBlasts.push_back({e.position + glm::vec3{0, 0.3f, 0}, 4.f, 30.f, 0.f, 0.f});

        if (e.type == EnemyType::WARDEN) {
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
            shakeTimer = 1.0f; shakeIntensity = 0.12f;
            audio.play("explosion");
            pushBanner("WARDEN DESTROYED", "", {1.f, 0.85f, 0.3f}, 2.5f);
        }
    }

    // Explosions resolve in a loop so a blast can kill a mite whose blast
    // kills another mite… (capped, in case of a giant chain).
    void processBlasts() {
        for (int guard = 0; !pendingBlasts.empty() && guard < 64; ++guard) {
            Blast b = pendingBlasts.back();
            pendingBlasts.pop_back();
            spawnExplosionParticles(b.pos, b.radius);
            shakeTimer = std::max(shakeTimer, 0.3f); shakeIntensity = std::max(shakeIntensity, 0.06f);
            explosionFlashTimer = 0.35f; explosionFlashPos = b.pos;
            audio.play("explosion");
            for (auto& e : enemies) {
                if (!e.targetable()) continue;
                float d = glm::length(e.position + glm::vec3{0, e.height() * 0.5f, 0} - b.pos);
                if (d < b.radius)
                    hurtEnemy(e, b.damage * (1.f - d / b.radius), e.position, 15.f, 3.f);
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
        for (auto& p : pickups) {
            glm::vec3 to = chest - p.pos;
            float d = glm::length(to);
            if (d < 5.f && d > 1e-3f) {
                p.vel = glm::mix(p.vel, to / d * 16.f, std::min(1.f, dt * 8.f));   // magnet
            } else {
                p.vel.y -= 20.f * dt;
                p.vel.x *= std::pow(0.2f, dt); p.vel.z *= std::pow(0.2f, dt);
            }
            p.pos += p.vel * dt;
            if (p.pos.y < 0.4f) { p.pos.y = 0.4f; p.vel.y = std::fabs(p.vel.y) * 0.3f; }
            p.life -= dt;
            if (d < 1.2f) {
                p.life = 0.f;
                styleSystem.heal(12.f);
                audio.play("pickup");
                spawnBurst(p.pos, {0.3f, 1.f, 0.5f}, 8, 2.f, 0.4f, 0.f);
            }
        }
        pickups.erase(std::remove_if(pickups.begin(), pickups.end(),
                      [](const Pickup& p){ return p.life <= 0.f; }), pickups.end());
    }

    // =========================================================================
    // Effects
    // =========================================================================
    Particle* freeParticle() {
        for (auto& p : particles) if (!p.alive) return &p;
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

    // Dust in the yard, embers over the lava, motes around the reactor.
    void spawnAmbientParticles(float dt) {
        int a = level.arenaAt(player.position);
        if (a < 0) return;
        ambientTimer += dt;
        const float every = 1.f / 45.f;
        while (ambientTimer > every) {
            ambientTimer -= every;
            Particle* p = freeParticle();
            if (!p) return;
            p->alive = true;
            if (a == 0) {
                float ang = frand(0.f, 6.28f), r = frand(5.f, 24.f);
                p->pos = player.position + glm::vec3{std::cos(ang) * r, frand(0.3f,6.f), std::sin(ang) * r};
                p->vel = {frand(0.2f,0.8f), frand(-0.05f,0.15f), frand(-0.2f,0.2f)};
                p->color = glm::vec3{1.f, 0.7f, 0.5f} * 0.15f;
                p->gravity = 0.f; p->maxLife = p->life = frand(2.5f, 4.5f);
            } else if (a == 1) {
                const Hazard& hz = level.hazards[rand() % level.hazards.size()];
                p->pos = {frand(hz.box.min.x, hz.box.max.x), 0.1f, frand(hz.box.min.z, hz.box.max.z)};
                p->vel = {frand(-0.4f,0.4f), frand(1.5f,3.5f), frand(-0.4f,0.4f)};
                p->color = glm::vec3{1.f, 0.45f, 0.1f} * frand(0.6f, 1.1f);
                p->gravity = -0.5f; p->maxLife = p->life = frand(1.2f, 2.8f);
            } else {
                float ang = frand(0.f, 6.28f), r = frand(2.5f, 9.f);
                p->pos = {std::cos(ang) * r, frand(0.5f, 4.f), -162.f + std::sin(ang) * r};
                p->vel = {0.f, frand(1.f, 2.5f), 0.f};
                p->color = glm::vec3{0.3f, 0.9f, 1.f} * 0.6f;
                p->gravity = -0.3f; p->maxLife = p->life = frand(2.f, 4.f);
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
            debris.push_back(d);
        }
        if (debris.size() > 900) debris.erase(debris.begin(), debris.begin() + (debris.size() - 900));
    }

    void updateDebris(float dt) {
        for (auto& d : debris) {
            d.vel.y -= 20.f * dt;
            d.pos   += d.vel * dt;
            d.angle += d.spin * dt;
            if (d.pos.y < 0.1f) { d.pos.y = 0.1f; d.vel.y = std::fabs(d.vel.y) * 0.35f; d.vel.x *= 0.6f; d.vel.z *= 0.6f; d.spin *= 0.6f; }
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

    void spawnTracer(glm::vec3 start, glm::vec3 end) {
        for (auto& t : tracers) {
            if (!t.alive) {
                t.start = start; t.end = end;
                t.maxLife = 0.22f; t.life = t.maxLife;
                t.alive = true;
                return;
            }
        }
    }

    // =========================================================================
    // Weapons
    // =========================================================================

    // Nearest enemy along a ray, unless a wall is closer. Returns the enemy
    // index (or -1) and the distance to whatever stopped the ray.
    int hitscan(glm::vec3 origin, glm::vec3 dir, float range, float& hitT) {
        float bestT = 1000.f;
        int   hit   = -1;
        for (int ei = 0; ei < (int)enemies.size(); ++ei) {
            if (!enemies[ei].targetable()) continue;
            float t = rayBoxHit(origin, dir, enemies[ei].getAABB());
            if (t > 0.f && t < bestT) { bestT = t; hit = ei; }
        }
        float wallT = range;
        for (auto& w : level.walls) {
            float t = rayBoxHit(origin, dir, w.box);
            if (t > 0.f && t < wallT) wallT = t;
        }
        if (hit >= 0 && wallT < bestT) hit = -1;
        hitT = hit >= 0 ? bestT : wallT;
        return hit;
    }

    // Headshots: the top fifth of a humanoid takes 1.5x damage.
    float critMultiplier(const Enemy& e, glm::vec3 hitPoint) const {
        bool humanoid = e.type == EnemyType::HUSK || e.type == EnemyType::SENTINEL ||
                        e.type == EnemyType::BRUTE || e.type == EnemyType::WARDEN;
        return humanoid && hitPoint.y > e.position.y + e.height() * 0.8f ? 1.5f : 1.f;
    }

    void startReload() {
        reloading     = true;
        reloadTimer   = RELOAD_TIME;
        viewModel.triggerReload();
        audio.play("reload");
    }

    void fireRevolver() {
        --revolverAmmo;
        revolverTimer = 0.15f;

        glm::vec3 origin = player.camera.position;
        glm::vec3 dir    = player.camera.forward();
        float t;
        int hit = hitscan(origin, dir, 80.f, t);
        spawnTracer(origin + dir * 0.2f, origin + dir * t);
        if (hit >= 0) {
            Enemy& e = enemies[hit];
            glm::vec3 at = origin + dir * t;
            float mult = critMultiplier(e, at);
            if (mult > 1.f) spawnHitSparks(at, {1.f, 0.9f, 0.3f});
            hurtEnemy(e, 35.f * mult, at, 10.f, 2.f);
        }

        recoilPitch = std::min(recoilPitch + 0.7f, 12.f);
        player.camera.pitch = glm::clamp(player.camera.pitch + 0.7f, -89.f, 89.f);

        viewModel.triggerFire();
        ui.onShoot();
        muzzleFlashPos   = origin + dir * 0.6f;
        muzzleFlashTimer = 0.04f;
        shakeTimer       = 0.06f;
        shakeIntensity   = 0.012f;
        audio.play("revolver");
        ++totalShots;
        if (hit >= 0) ++totalHits;
        spawnShellCasing(origin, player.camera.right());
        if (revolverAmmo <= 0) startReload();
    }

    void startShotgunReload() {
        shotgunReloading   = true;
        shotgunReloadTimer = SHOTGUN_RELOAD_TIME;
        audio.play("reload");
    }

    void fireShotgun() {
        --shotgunAmmo;
        shotgunTimer = 0.55f;

        glm::vec3 origin = player.camera.position;
        glm::vec3 fwd    = player.camera.forward();
        glm::vec3 right  = player.camera.right();
        glm::vec3 up     = glm::cross(fwd, right);

        static constexpr int   PELLETS    = 10;
        static constexpr float SPREAD     = 0.18f;
        static constexpr float PELLET_DMG = 9.f;

        bool anyHit = false;
        for (int p = 0; p < PELLETS; ++p) {
            float rx = frand(-1.f, 1.f) * SPREAD;
            float ry = frand(-1.f, 1.f) * SPREAD;
            glm::vec3 dir = glm::normalize(fwd + right * rx + up * ry);
            float t;
            int hit = hitscan(origin, dir, 60.f, t);
            spawnTracer(origin + dir * 0.2f, origin + dir * t);
            if (hit >= 0) {
                anyHit = true;
                hurtEnemy(enemies[hit], PELLET_DMG, origin + dir * t, 3.f, 0.5f);
            }
        }

        recoilPitch = std::min(recoilPitch + 2.2f, 12.f);
        player.camera.pitch = glm::clamp(player.camera.pitch + 2.2f, -89.f, 89.f);

        viewModel.triggerFire();
        ui.onShoot();
        muzzleFlashPos   = origin + fwd * 0.6f;
        muzzleFlashTimer = 0.07f;
        shakeTimer       = 0.12f;
        shakeIntensity   = 0.025f;
        audio.play("shotgun");
        ++totalShots;
        if (anyHit) ++totalHits;
        spawnShellCasing(origin, player.camera.right());
        spawnShellCasing(origin + player.camera.right() * 0.1f, player.camera.right());

        if (shotgunAmmo <= 0) startShotgunReload();
        else viewModel.triggerPump();
    }

    void throwGrenade() {
        --grenadeCount;
        grenadeTimer = 0.6f;
        glm::vec3 origin = player.camera.position;
        glm::vec3 dir    = player.camera.forward();
        projSystem.fire(origin, dir * 14.f + glm::vec3{0, 5.f, 0}, 80.f, true,
                        {0.3f, 0.9f, 0.1f}, /*grenade=*/true, /*blastRadius=*/5.f);
        viewModel.triggerGrenade();
        shakeTimer     = 0.05f;
        shakeIntensity = 0.008f;
        audio.play("jump");
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
        for (int i=0;i<MAX_POINT_LIGHTS;++i) {
            std::string pn = "pointLightPos["+std::to_string(i)+"]";
            std::string cn = "pointLightColor["+std::to_string(i)+"]";
            sh.setVec3(pn.c_str(), pointLightPos[i]);
            sh.setVec3(cn.c_str(), pointLightColor[i]);
        }
    }

    // Everything dynamic that's made of boxes, gathered for one instanced draw.
    void gatherBoxes(std::vector<BoxInstance>& out) {
        using namespace rig;
        float t = gameClock;

        for (auto& e : enemies) if (e.alive) buildEnemy(e, t, out);

        // Spawn beams: a column of light while an enemy materialises
        for (auto& e : enemies) {
            if (!e.alive || e.state != EnemyState::SPAWNING) continue;
            float k = e.spawnTimer / Enemy::SPAWN_TIME;
            glm::vec3 c = e.stats().glow;
            float base = e.stats().flying ? 0.f : e.position.y;
            push(out, T({e.position.x, base + 8.f, e.position.z}) * S({0.25f + 0.6f * k, 16.f, 0.25f + 0.6f * k}),
                 c * 0.2f, c * (1.5f + 2.f * k));
            push(out, T({e.position.x, base + 0.05f, e.position.z}) * RY(t * 3.f) * S({2.2f * k + 0.5f, 0.06f, 2.2f * k + 0.5f}),
                 c * 0.2f, c * 2.f);
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
            glm::mat4 base = T(p.position) * RY(spin) * RX(spin * 0.7f);
            push(out, base * S(glm::vec3{s}), p.emissiveColor * 0.3f, p.emissiveColor * 0.8f);
            push(out, base * RZ(0.785f) * S(glm::vec3{s * 0.75f}), p.emissiveColor, p.emissiveColor * 3.f);
        }

        // Projectiles: a spinning bright core inside a darker shell
        for (auto& p : projSystem.pool) {
            if (!p.alive) continue;
            float s = (p.isGrenade ? 0.22f : 0.26f) * p.size;
            float spin = t * 9.f + p.position.x;
            glm::mat4 base = T(p.position) * RY(spin) * RX(spin * 0.7f);
            push(out, base * S(glm::vec3{s}), p.emissiveColor * 0.3f, p.emissiveColor * 0.8f);
            push(out, base * RZ(0.785f) * S(glm::vec3{s * 0.75f}), p.emissiveColor, p.emissiveColor * 3.f);
        }

        for (auto& p : pickups) {
            float pulse = 1.f + 0.3f * std::sin(t * 8.f);
            push(out, T(p.pos + glm::vec3{0, std::sin(t * 3.f + p.pos.x) * 0.1f, 0}) * RY(t * 3.f) * RX(0.6f) * S(glm::vec3{0.32f}),
                 {0.2f, 0.9f, 0.4f}, glm::vec3{0.3f, 1.6f, 0.6f} * pulse);
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

        // Doors: a dark slab with light bars — red while locked, green once open
        for (int di = 0; di < (int)level.doors.size(); ++di) {
            const Door& d = level.doors[di];
            const AABB& b = level.walls[d.wall].box;
            if (b.max.y <= 0.02f) continue;
            glm::vec3 c = (b.min + b.max) * 0.5f, sz = b.max - b.min;
            push(out, T(c) * S(sz), {0.16f, 0.15f, 0.18f});
            glm::vec3 bar = d.open ? glm::vec3{0.3f, 1.6f, 0.7f} : glm::vec3{1.8f, 0.2f, 0.15f};
            for (int i = 0; i < 6; ++i) {
                float x = b.min.x + sz.x * (i + 0.5f) / 6.f;
                push(out, T({x, c.y, c.z}) * S({0.18f, sz.y * 0.92f, sz.z + 0.08f}), bar * 0.2f, bar);
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

        // Sunset Yard: a diamond spinning above the obelisk
        push(out, T({0.f, 9.f + std::sin(t * 1.5f) * 0.3f, 0.f}) * RY(t) * RX(0.785f) * RZ(0.785f) * S(glm::vec3{1.1f}),
             {1.f, 0.4f, 0.7f}, glm::vec3{1.6f, 0.35f, 0.9f});

        // The Core: the reactor — stacked counter-rotating blocks and orbiting rings
        {
            bool bossRage = false;
            for (auto& e : enemies) if (e.alive && e.type == EnemyType::WARDEN && e.enraged) bossRage = true;
            glm::vec3 base{0.f, 1.5f, -162.f};
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

    void render() override {
        float alpha = (float)(accumulator / PHYSICS_DT);
        glm::vec3 renderCamPos = glm::mix(prevCamPos, player.camera.position, alpha);
        Camera renderCam = player.camera;
        renderCam.position = renderCamPos;

        if (shakeTimer > 0.f) {
            float s = shakeIntensity * (shakeTimer / 0.2f);
            renderCam.position += glm::vec3{frand(-1.f,1.f) * s, frand(-1.f,1.f) * s, 0.f};
        }
        renderCam.position += viewModel.getBobOffset(playerXZSpeed, player.onGround, renderCam.right());
        renderCam.position.y -= landSquash;
        renderCam.fov = player.camera.fov + fovKick;

        glm::mat4 view = renderCam.viewMatrix();
        glm::mat4 proj = renderCam.projectionMatrix();
        Theme th = level.themeAt(player.position);

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
        glBindTexture(GL_TEXTURE_2D, floorTex); world.floor.draw();
        glBindTexture(GL_TEXTURE_2D, wallTex);  world.walls.draw();
        glBindTexture(GL_TEXTURE_2D, ceilTex);  world.under.draw();
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
            gatherBoxes(boxes);
            boxRenderer.shader.use();
            boxRenderer.shader.setMat4("projection", proj);
            boxRenderer.shader.setMat4("view",       view);
            boxRenderer.shader.setInt ("uTexture",   0);
            applyLighting(boxRenderer.shader, th, renderCamPos);
            glBindTexture(GL_TEXTURE_2D, whiteTex);
            boxRenderer.draw(boxes);
        }

        // --- Enemy health bars (only once damaged) ---
        worldShader.use();
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        glDisable(GL_CULL_FACE);
        glm::vec3 camRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
        glm::vec3 camFwdFlat = glm::normalize(glm::cross(camRight, glm::vec3(0,1,0)));
        for (auto& e : enemies) {
            if (!e.targetable() || e.health >= e.maxHealth || e.type == EnemyType::WARDEN) continue;
            float barW = std::max(1.0f, e.radius() * 1.8f), barH = 0.12f;
            float fill = e.health / e.maxHealth;
            glm::vec3 barPos = e.position + glm::vec3{0, e.height() + 0.45f, 0};
            glm::mat4 bg(1.f);
            bg[0] = glm::vec4(camRight * barW, 0); bg[1] = glm::vec4(0, barH, 0, 0);
            bg[2] = glm::vec4(camFwdFlat * 0.01f, 0); bg[3] = glm::vec4(barPos, 1);
            worldShader.setMat4("model", bg);
            worldShader.setVec3("objectColor", {0.1f, 0.1f, 0.1f});
            worldShader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
            viewModel.cubeMesh.draw();
            glm::mat4 fm(1.f);
            fm[0] = glm::vec4(camRight * barW * fill, 0); fm[1] = glm::vec4(0, barH * 0.8f, 0, 0);
            fm[2] = glm::vec4(camFwdFlat * 0.02f, 0);
            fm[3] = glm::vec4(barPos - camRight * (barW * (1.f - fill) * 0.5f), 1);
            worldShader.setMat4("model", fm);
            worldShader.setVec3("objectColor", {0.9f, 0.15f, 0.1f});
            worldShader.setVec3("emissiveColor", {0.6f, 0.08f, 0.04f});
            viewModel.cubeMesh.draw();
        }
        glEnable(GL_CULL_FACE);
        worldShader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        worldShader.setVec3("objectColor", {1.f, 1.f, 1.f});
        worldShader.setMat4("model", glm::mat4(1.f));

        grapple.drawLine(player.position + glm::vec3{0,player.eyeHeight,0}, view, proj);
        renderDecals();

        renderTracers(view, proj);
        renderLasers(view, proj);
        renderParticles(view, proj);

        // --- View model (inside the FBO so it gets bloom; no fog) ---
        {
            float revolverFill = reloading ? (1.f - reloadTimer / RELOAD_TIME)
                                           : (float)revolverAmmo / (float)revolverAmmoMax;
            float shotgunFill  = shotgunReloading ? (1.f - shotgunReloadTimer / SHOTGUN_RELOAD_TIME)
                                                  : (shotgunTimer > 0.f ? (1.f - shotgunTimer / 0.55f) : 1.f);
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
            viewModel.draw(worldShader, renderCam, activeWeapon, revolverFill, shotgunFill);
            worldShader.setMat4("projection", proj);
            worldShader.setMat4("view",       view);
        }

        postProcess.crtEnabled = settings ? settings->crtFilter : false;
        postProcess.endScene();

        renderHUD(view, proj);
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
        float reloadProgress = reloading ? (1.f - reloadTimer / RELOAD_TIME) : 1.f;
        float shotgunReloadProgress = shotgunReloading ? (1.f - shotgunReloadTimer / SHOTGUN_RELOAD_TIME) : 1.f;
        ui.render(styleSystem, activeWeapon,
                  shotgunAmmo, shotgunAmmoMax, shotgunReloading, shotgunReloadProgress,
                  revolverAmmo, revolverAmmoMax, reloading, reloadProgress,
                  nearInteractable);

        const Arena& ar = level.arenas[director.arena];
        int nArenas = (int)level.arenas.size();
        char buf[96];

        if (!playerDead && !victory) {
            int alive = 0;
            for (auto& e : enemies) if (e.alive) ++alive;
            int left = alive + director.queued();
            glm::vec3 accent{1.f, 0.75f, 0.3f};
            const Enemy* boss = nullptr;
            for (auto& e : enemies) if (e.alive && e.type == EnemyType::WARDEN) boss = &e;

            switch (director.phase) {
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

            if (boss) ui.renderBossBar("THE WARDEN", boss->health / boss->maxHealth, boss->enraged);

            // Waypoint to the open gate
            const AABB* gate = ar.exitDoor >= 0 ? &level.walls[level.doors[ar.exitDoor].wall].box : nullptr;
            if (director.phase == WaveDirector::Phase::CLEARED && gate && player.position.z > gate->min.z) {
                const AABB& b = *gate;
                glm::vec3 target{(b.min.x + b.max.x) * 0.5f, 2.f, (b.min.z + b.max.z) * 0.5f};
                float sx, sy;
                bool on = projectToScreen(target, view, proj, sx, sy);
                snprintf(buf, sizeof(buf), "GATE %dM", (int)glm::length(target - player.position));
                ui.renderMarker(sx, sy, on, {0.4f, 1.f, 0.6f}, buf);
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
            ui.renderControlHint(glm::clamp(controlHintTimer / 1.5f, 0.f, 1.f));
        }

        if (!banners.empty()) {
            const Banner& b = banners.front();
            float a = std::min({1.f, b.time * 4.f, (b.duration - b.time) * 2.5f});
            ui.renderBanner(b.title.c_str(), b.subtitle.c_str(), b.color, a);
        }

        if (playerDead) {
            snprintf(buf, sizeof(buf), "ARENA %d/%d %s - WAVE %d/%d", director.arena + 1, nArenas, ar.name,
                     director.wave + 1, director.waveCount());
            ui.renderDeath(buf, totalKills, elapsedTime);
        }
        if (victory) ui.renderVictory(totalKills, totalShots, totalHits, deaths, elapsedTime, peakStyle);
        if (paused) ui.renderPause(pauseSelected);
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

    void renderParticles(const glm::mat4& view, const glm::mat4& proj) {
        struct PVert { float x, y, z, r, g, b, a; };
        static PVert buf[MAX_PARTICLES];
        int count = 0;
        for (auto& p : particles) {
            if (!p.alive) continue;
            float t = p.life / p.maxLife;
            buf[count++] = { p.pos.x, p.pos.y, p.pos.z, p.color.r, p.color.g, p.color.b, t * t };
        }
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

    // Draws thin additive beams. Shared by bullet tracers and Sentinel lasers.
    void drawBeams(const std::vector<std::array<glm::vec4, 2>>& beams, glm::vec3 color, float width,
                   const glm::mat4& view, const glm::mat4& proj) {
        struct TVert { float x,y,z,a; };
        TVert buf[MAX_TRACERS * 6];
        int count = 0;
        for (auto& bm : beams) {
            if (count + 6 > MAX_TRACERS * 6) break;
            glm::vec3 s = glm::vec3(bm[0]), e = glm::vec3(bm[1]);
            glm::vec3 ray = e - s;
            float len = glm::length(ray);
            if (len < 0.001f) continue;
            glm::vec3 dir = ray / len;
            // Side axis perpendicular to the ray (cross with world up; fall back to X)
            glm::vec3 side = glm::cross(dir, glm::vec3(0.f, 1.f, 0.f));
            if (glm::length(side) < 0.01f) side = glm::cross(dir, glm::vec3(1.f, 0.f, 0.f));
            side = glm::normalize(side) * width;
            float aN = bm[0].w, aF = bm[1].w;
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
        std::vector<std::array<glm::vec4, 2>> beams;
        for (auto& t : tracers) {
            if (!t.alive) continue;
            float fade = t.life / t.maxLife;
            beams.push_back({glm::vec4(t.start, fade), glm::vec4(t.end, fade * 0.08f)});
        }
        drawBeams(beams, {0.97f, 0.95f, 0.72f}, 0.055f, view, proj);
    }

    // A Sentinel winding up paints you with a laser; it brightens until it fires.
    void renderLasers(const glm::mat4& view, const glm::mat4& proj) {
        std::vector<std::array<glm::vec4, 2>> beams;
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
            beams.push_back({glm::vec4(eye, a), glm::vec4(eye + d * ((len - 3.f) / len), a * 0.5f)});
        }
        drawBeams(beams, {0.3f, 0.95f, 1.f}, 0.03f, view, proj);
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
