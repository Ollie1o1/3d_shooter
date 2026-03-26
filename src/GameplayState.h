#pragma once
#include "GameState.h"
#include "Player.h"
#include "Camera.h"
#include "Mesh.h"
#include "ShaderProgram.h"
#include "StyleSystem.h"
#include "UIRenderer.h"
#include "Enemy.h"
#include "Projectile.h"
#include "GrappleHook.h"
#include "Level.h"
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
#include <array>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <string>
#include <cstdio>

// =============================================================================
// GameplayState — the live game: physics, combat, rendering, progression.
//
// KEY EXTENSION POINTS:
//
//   ADD A WEAPON:
//     1. Add ammo/timer members in the "Player weapon state" block below.
//     2. Write a fire___() method modelled on fireRevolver().
//        Hitscan: rayAABBHit() against enemies + allWalls, then spawnTracer().
//        Projectile: projSystem.fire(origin, dir*speed, damage, true, color).
//     3. Handle the key/button in physicsTick() where the shooting block is.
//     4. Add an ammo display case in render() → ui.render(ammo, max).
//
//   ADD AN ENEMY TYPE:
//     1. Add a new EnemyType enum value in Enemy.h.
//     2. Handle it in Enemy::update() (movement/attack logic).
//     3. Set health in the Enemy constructor switch.
//     4. Spawn it by type in spawnEnemiesForRoom() or directly via
//        enemies.push_back(Enemy(EnemyType::YOURTYPE, position)).
//
//   ADD A GAME MECHANIC (dash, slam, etc.):
//     All mechanics live in physicsTick(). Follow the dash pattern:
//     - Check input (SDL_SCANCODE_* or mouse button).
//     - Apply an impulse to player.velocity.
//     - Set a cooldown timer; decrement it each tick.
//
//   CHANGE THE MAP:
//     Edit buildLevel() in Level.h. Every Wall added there is automatically
//     rendered AND used for collision. No other changes needed.
// =============================================================================
static constexpr int   SCREEN_W   = 1280;
static constexpr int   SCREEN_H   = 720;
static constexpr float PHYSICS_HZ = 60.f;
static constexpr float PHYSICS_DT = 1.f / PHYSICS_HZ;

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

// ArenaColor palette is defined in Level.h (included above)

// Thin glowing quads placed at every enemy spawn point.
// Not added to the wall list so they don't affect collision.
static Mesh buildSpawnPadMesh(const LevelData& level) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;
    float r = 1.0f;   // pad half-size (1 m radius → 2x2 m square)
    float y = 0.04f;  // just above floor to avoid z-fighting

    for (auto& room : level.rooms) {
        for (auto& sp : room.enemySpawns) {
            unsigned int base = (unsigned int)verts.size();
            glm::vec3 col{0.f, 0.7f, 0.25f};
            glm::vec3 n{0.f, 1.f, 0.f};
            verts.push_back({{sp.x-r, y, sp.z-r}, {0,0}, n, col});
            verts.push_back({{sp.x+r, y, sp.z-r}, {1,0}, n, col});
            verts.push_back({{sp.x+r, y, sp.z+r}, {1,1}, n, col});
            verts.push_back({{sp.x-r, y, sp.z+r}, {0,1}, n, col});
            idx.insert(idx.end(), {base,base+1,base+2, base,base+2,base+3});
        }
    }
    Mesh m;
    m.upload(verts, idx);
    return m;
}

static Mesh buildWorldMesh(const std::vector<Wall>& walls) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    // Push a quad face. Vertices are automatically tinted by their world Y:
    // bottom verts get 55% brightness, top verts get 100%. This gives every
    // wall a sense of mass and ground shadow for free.
    // Horizontal faces (all verts at same Y) stay flat-coloured.
    auto pushFace = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
                        glm::vec3 n, glm::vec3 col) {
        float yLo = std::min({a.y, b.y, c.y, d.y});
        float yHi = std::max({a.y, b.y, c.y, d.y});
        float ySpan = yHi - yLo;
        auto gc = [&](float y) -> glm::vec3 {
            float t = (ySpan > 0.01f) ? (y - yLo) / ySpan : 1.f;
            return col * glm::mix(0.52f, 1.0f, t);
        };

        // Compute world-space UVs based on face orientation (normal)
        // so textures tile at consistent density (1 repeat per 4 meters)
        float texScale = 0.25f;
        auto worldUV = [&](glm::vec3 p) -> glm::vec2 {
            float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
            if (ay > ax && ay > az)      return {p.x * texScale, p.z * texScale}; // floor/ceiling
            else if (ax > az)            return {p.z * texScale, p.y * texScale}; // X-facing wall
            else                         return {p.x * texScale, p.y * texScale}; // Z-facing wall
        };

        unsigned int base = (unsigned int)verts.size();
        verts.push_back({a, worldUV(a), n, gc(a.y)});
        verts.push_back({b, worldUV(b), n, gc(b.y)});
        verts.push_back({c, worldUV(c), n, gc(c.y)});
        verts.push_back({d, worldUV(d), n, gc(d.y)});
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    // Push all 6 faces of an AABB box with a given colour.
    // Bottom face (-Y) is only rendered for floating objects (min.y > 0.1)
    // so ground-level boxes don't Z-fight with the floor quad.
    auto pushBox = [&](const AABB& b, glm::vec3 col) {
        glm::vec3 mn = b.min, mx = b.max;
        pushFace({mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{ 1, 0, 0}, col);
        pushFace({mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1, 0, 0}, col);
        pushFace({mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{ 0, 0, 1}, col);
        pushFace({mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{ 0, 0,-1}, col);
        pushFace({mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},{ 0, 1, 0}, col);
        // Bottom face — render for all elevated objects so they look solid from below
        if (b.min.y > 0.1f)
            pushFace({mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z},{ 0,-1, 0}, col);
    };

    // Floor — large quad covering both rooms + corridor
    pushFace({-100,0, 90},{100,0, 90},{100,0,-240},{-100,0,-240},{0,1,0}, ArenaColor::Floor);
    // Ceiling — matching extent
    pushFace({-100,16,-240},{100,16,-240},{100,16,90},{-100,16,90},{0,-1,0}, ArenaColor::Ceiling);

    // Each wall carries its own colour (set in Level.h when the wall is created)
    for (int i = 0; i < (int)walls.size(); ++i) {
        pushBox(walls[i].box, walls[i].color);
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

struct SplitWorldMeshes {
    Mesh floor, walls, ceiling;
};

static SplitWorldMeshes buildSplitWorldMesh(const std::vector<Wall>& walls) {
    std::vector<Vertex> floorV, wallV, ceilV;
    std::vector<unsigned int> floorI, wallI, ceilI;

    float texScale = 0.25f;

    auto pushFace = [&](std::vector<Vertex>& verts, std::vector<unsigned int>& idx,
                        glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
                        glm::vec3 n, glm::vec3 col) {
        float yLo = std::min({a.y, b.y, c.y, d.y});
        float yHi = std::max({a.y, b.y, c.y, d.y});
        float ySpan = yHi - yLo;
        auto gc = [&](float y) -> glm::vec3 {
            float t = (ySpan > 0.01f) ? (y - yLo) / ySpan : 1.f;
            return col * glm::mix(0.52f, 1.0f, t);
        };
        auto worldUV = [&](glm::vec3 p) -> glm::vec2 {
            float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
            if (ay > ax && ay > az)      return {p.x * texScale, p.z * texScale};
            else if (ax > az)            return {p.z * texScale, p.y * texScale};
            else                         return {p.x * texScale, p.y * texScale};
        };
        unsigned int base = (unsigned int)verts.size();
        verts.push_back({a, worldUV(a), n, gc(a.y)});
        verts.push_back({b, worldUV(b), n, gc(b.y)});
        verts.push_back({c, worldUV(c), n, gc(c.y)});
        verts.push_back({d, worldUV(d), n, gc(d.y)});
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    auto pushBoxSplit = [&](const AABB& b, glm::vec3 col) {
        glm::vec3 mn = b.min, mx = b.max;
        // Side faces → wall batch
        pushFace(wallV, wallI, {mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{ 1, 0, 0}, col);
        pushFace(wallV, wallI, {mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1, 0, 0}, col);
        pushFace(wallV, wallI, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{ 0, 0, 1}, col);
        pushFace(wallV, wallI, {mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{ 0, 0,-1}, col);
        // Top face → ceiling batch
        pushFace(ceilV, ceilI, {mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},{ 0, 1, 0}, col);
        // Bottom face for elevated objects → floor batch
        if (b.min.y > 0.1f)
            pushFace(floorV, floorI, {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z},{ 0,-1, 0}, col);
    };

    // Floor quad
    pushFace(floorV, floorI, {-100,0, 90},{100,0, 90},{100,0,-240},{-100,0,-240},{0,1,0}, ArenaColor::Floor);
    // Ceiling quad
    pushFace(ceilV, ceilI, {-100,16,-240},{100,16,-240},{100,16,90},{-100,16,90},{0,-1,0}, ArenaColor::Ceiling);

    for (int i = 0; i < (int)walls.size(); ++i) {
        pushBoxSplit(walls[i].box, walls[i].color);
    }

    SplitWorldMeshes result;
    result.floor.upload(floorV, floorI);
    result.walls.upload(wallV, wallI);
    result.ceiling.upload(ceilV, ceilI);
    return result;
}

class GameplayState : public GameState {
public:
    std::function<void()> onReturnToMenu;
    std::function<void()> onQuit;

    GameSettings* settings = nullptr;   // injected by main — may be null (safe)

    Player         player{{0.f,0.f,60.f}};
    StyleSystem    styleSystem;
    UIRenderer     ui{SCREEN_W,SCREEN_H};
    ProjectileSystem projSystem;
    GrappleHook    grapple;
    LevelData      level;
    AudioSystem&   audio;
    PostProcess    postProcess{SCREEN_W,SCREEN_H};

    ShaderProgram  worldShader;
    ShaderProgram  skyboxShader;
    EnemyRenderer  enemyRenderer;

    GLuint   whiteTex = 0;
    GLuint   floorTex = 0;
    GLuint   wallTex  = 0;
    GLuint   ceilTex  = 0;
    Mesh     worldMesh;
    // Split world geometry for per-surface texture binding
    Mesh     floorMesh;
    Mesh     wallMesh;
    Mesh     ceilMesh;

    std::vector<Enemy>   enemies;
    std::vector<Wall>    allWalls;
    SpatialGrid          spatialGrid;  // rebuilt whenever allWalls changes

    // Slot 1: Revolver — 8 rounds, manual/auto reload
    int   revolverAmmo    = 8;
    int   revolverAmmoMax = 8;
    float revolverTimer   = 0.f;   // between-shot cooldown (rate of fire)
    bool  reloading       = false;
    float reloadTimer     = 0.f;
    static constexpr float RELOAD_TIME = 1.2f;
    // Slot 2: Shotgun — 2 shells, pump-action
    int   shotgunAmmo      = 2;
    int   shotgunAmmoMax   = 2;
    float shotgunTimer     = 0.f;   // between-shot cooldown
    bool  shotgunReloading = false;
    float shotgunReloadTimer = 0.f;
    static constexpr float SHOTGUN_RELOAD_TIME = 1.4f;
    // G key: Grenades — max 2, one returned per 2 kills (not a weapon slot)
    int   grenadeCount   = 2;
    int   grenadeMax     = 2;
    float grenadeTimer   = 0.f;   // cooldown between throws
    int   killsThisCycle = 0;     // counts toward next grenade refill (every 2 kills)
    // Slot selection: 0 = revolver, 1 = shotgun
    int   activeWeapon   = 0;
    int   pendingWeapon  = -1;       // weapon switch in progress (-1 = none)
    float weaponSwitchTimer = 0.f;   // time until actual switch happens

    // Recoil recovery — kick accumulates, then smoothly returns to 0
    float recoilPitch = 0.f;

    // Game stats
    int   totalKills  = 0;
    int   totalShots  = 0;
    int   totalHits   = 0;
    float elapsedTime = 0.f;
    float peakStyle   = 0.f;
    bool  arenaCleared = false;
    float arenaTimer   = 0.f;    // time on win screen

    // Wave system
    int   waveNumber      = 1;
    int   wavesPerRoom    = 3;
    float waveBannerTimer = 0.f;
    float wavePauseTimer  = 0.f;
    int   waveEnemyStart  = 0;   // index of first enemy in current wave

    // Footstep system
    float footstepTimer = 0.f;

    int   dashCharges       = 2;
    float dashCooldown      = 0.f;
    float dashMomentumTimer = 0.f;  // skips ground friction while nonzero
    int   jumpsRemaining = 2;
    bool  prevOnGround   = false;
    bool  slamming       = false;
    float invincFrames   = 0.f;

    float shakeTimer     = 0.f;
    float shakeIntensity = 0.f;

    float muzzleFlashTimer = 0.f;
    glm::vec3 muzzleFlashPos{0.f};

    // Hitscan tracer — thin billboard quad, additive blending, fades fast
    struct Tracer {
        glm::vec3 start{0.f}, end{0.f};
        float life = 0.f, maxLife = 0.13f;
        bool  alive = false;
    };
    static constexpr int MAX_TRACERS = 16;
    Tracer        tracers[MAX_TRACERS];
    ShaderProgram tracerShader;
    GLuint        tracerVAO = 0, tracerVBO = 0;

    ViewModel viewModel;
    std::vector<Interactable> interactables;
    bool  nearInteractable = false;   // crosshair is on an interactable this frame

    float fovKick        = 0.f;      // extra FOV added on dash, fades back to 0
    float playerXZSpeed  = 0.f;      // current horizontal speed (for camera bob)

    bool  paused     = false;
    bool  playerDead = false;
    float deadTimer  = 0.f;

    // Hit-stop: number of physics ticks to freeze on high-damage events.
    int hitStopFrames = 0;

    // Impact decals — flat blood/burn quads that appear on the floor at enemy
    // hit positions. Rendered with polygon offset to avoid Z-fighting.
    struct Decal {
        glm::vec3 pos{0.f};
        float life = 0.f, maxLife = 10.f;
        bool  alive = false;
    };
    static constexpr int MAX_DECALS = 64;
    Decal  decals[MAX_DECALS];
    GLuint decalVAO = 0, decalVBO = 0;

    // Explosion particles — spawned by grenade blasts, rendered as GL_POINTS
    struct Particle {
        glm::vec3 pos{0.f}, vel{0.f};
        glm::vec3 color{1.f, 0.5f, 0.05f};  // particle color
        float life = 0.f, maxLife = 0.f;
        bool  alive = false;
    };
    static constexpr int MAX_PARTICLES = 300;
    Particle  particles[MAX_PARTICLES];
    GLuint    particleVAO = 0, particleVBO = 0;

    // Spawn pads — glowing floor markers at each enemy spawn point.
    Mesh  spawnPadMesh;
    float spawnPadPulse  = 0.f;   // drives the emissive glow animation
    float respawnTimer   = 0.f;
    int   nextSpawnIdx   = 0;
    static constexpr float RESPAWN_INTERVAL = 4.f;  // check every 4 s
    static constexpr int   MIN_ALIVE        = 0;    // 0 = disabled; room-clear progression

    static constexpr int MAX_POINT_LIGHTS = 4;
    glm::vec3 pointLightPos[MAX_POINT_LIGHTS];
    glm::vec3 pointLightColor[MAX_POINT_LIGHTS];

    Uint64 prevTicks   = 0;
    Uint64 freq        = 0;
    double accumulator = 0.0;
    glm::vec3 prevCamPos{0.f};

    // FPS counter
    int   fpsFrameCount = 0;
    float fpsTimer      = 0.f;

    bool rightMousePrev   = false;
    bool leftMousePrev    = false;
    bool parryPrev        = false;
    bool prevDashKey      = false;
    bool prevJumpKey      = false;
    // Event-driven click flags — set in handleEvent, consumed once in physicsTick.
    // More reliable than polling SDL_GetMouseState on trackpads.
    bool pendingFire      = false;
    bool pendingShotgun   = false;
    bool pendingGrenade   = false;
    bool pendingGrapple   = false;   // right-click: fire or release grapple

    GameplayState(AudioSystem& aud) : audio(aud) {
        worldShader.loadFiles("src/shader.vert","src/shader.frag");
        skyboxShader.loadFiles("src/skybox.vert","src/skybox.frag");
        tracerShader.loadFiles("src/tracer.vert","src/tracer.frag");

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

        // Particle VBO: [x, y, z, alpha] per point — reuses tracer shader
        glGenVertexArrays(1, &particleVAO);
        glGenBuffers(1, &particleVBO);
        glBindVertexArray(particleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(3*sizeof(float)));
        glBindVertexArray(0);

        // Decal VBO: dynamic flat quads, each vertex = pos(3)+uv(2)+normal(3)+color(3)=11 floats
        // Matches the Vertex layout expected by worldShader.
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
        player.camera.aspectRatio = (float)SCREEN_W/SCREEN_H;
        player.camera.fov = settings ? settings->fov : 90.f;

        level = loadLevelFromFile("assets/level.txt");
        if (level.rooms.empty()) level = buildLevel();  // fallback if file missing
        allWalls = level.getAllWalls();
        spatialGrid.build(allWalls);
        worldMesh = buildWorldMesh(allWalls);
        {
            auto split = buildSplitWorldMesh(allWalls);
            floorMesh = std::move(split.floor);
            wallMesh  = std::move(split.walls);
            ceilMesh  = std::move(split.ceiling);
        }
        spawnPadMesh = buildSpawnPadMesh(level);

        spawnEnemiesForRoom(0);

        prevTicks = SDL_GetPerformanceCounter();
        freq      = SDL_GetPerformanceFrequency();
        prevCamPos = player.camera.position;

        for (int i=0;i<MAX_POINT_LIGHTS;++i) {
            pointLightPos[i]   = {0,0,0};
            pointLightColor[i] = {0,0,0};
        }

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

    void spawnEnemiesForRoom(int roomIdx) {
        if (roomIdx >= (int)level.rooms.size()) return;
        auto& room = level.rooms[roomIdx];
        waveNumber = 1;
        waveBannerTimer = 3.0f;
        spawnWaveEnemies(roomIdx, 0);
        room.unlocked = true;
    }

    void spawnWaveEnemies(int roomIdx, int wave) {
        if (roomIdx >= (int)level.rooms.size()) return;
        auto& room = level.rooms[roomIdx];
        int totalSpawns = (int)room.enemySpawns.size();
        int perWave = totalSpawns / wavesPerRoom;
        int start = wave * perWave;
        int end   = (wave == wavesPerRoom - 1) ? totalSpawns : start + perWave;

        waveEnemyStart = (int)enemies.size();

        int groundIdx = 0;
        for (int i = start; i < end; ++i) {
            auto& sp = room.enemySpawns[i];
            EnemyType t;
            if (sp.y > 5.f) {
                t = EnemyType::FLYER;
            } else {
                // Later waves get harder enemy mixes
                if (wave == 0) {
                    // Wave 1: mostly grunts
                    t = (groundIdx % 3 == 2) ? EnemyType::SHOOTER : EnemyType::GRUNT;
                } else if (wave == 1) {
                    // Wave 2: mixed
                    int tidx = groundIdx % 3;
                    t = (tidx == 0) ? EnemyType::GRUNT
                      : (tidx == 1) ? EnemyType::SHOOTER
                      :               EnemyType::STALKER;
                } else {
                    // Wave 3: harder mix with more stalkers
                    int tidx = groundIdx % 4;
                    t = (tidx == 0) ? EnemyType::GRUNT
                      : (tidx == 1) ? EnemyType::STALKER
                      : (tidx == 2) ? EnemyType::SHOOTER
                      :               EnemyType::STALKER;
                }
                ++groundIdx;
            }
            enemies.push_back(Enemy(t, sp));
        }
    }

    void handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                if (onReturnToMenu) onReturnToMenu();
                return;
            }
            if (e.key.keysym.sym == SDLK_e) {
                int idx = findInteractTarget();
                if (idx >= 0 && interactables[idx].onInteract)
                    interactables[idx].onInteract();
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                if (activeWeapon == 0) pendingFire    = true;
                else                   pendingShotgun  = true;
            }
            if (e.button.button == SDL_BUTTON_RIGHT) {
                pendingGrapple = true;
            }
        }
        if (e.type == SDL_MOUSEWHEEL) {
            int newWeapon = (activeWeapon + 1) % 2;
            if (newWeapon != activeWeapon && pendingWeapon < 0) {
                pendingWeapon = newWeapon;
                weaponSwitchTimer = 0.15f;
                viewModel.triggerSwitch();
                audio.play("reload", 80);
            }
        }
        if (e.type == SDL_KEYDOWN) {
            auto trySwitch = [&](int newWeapon) {
                if (newWeapon != activeWeapon && pendingWeapon < 0) {
                    pendingWeapon = newWeapon;
                    weaponSwitchTimer = 0.15f;
                    viewModel.triggerSwitch();
                    audio.play("reload", 80);
                }
            };
            if (e.key.keysym.sym == SDLK_1) trySwitch(0);
            if (e.key.keysym.sym == SDLK_2) trySwitch(1);
            if (e.key.keysym.sym == SDLK_g) pendingGrenade = true;
            // Restart on R during death or win screen
            if (e.key.keysym.sym == SDLK_r && (playerDead || arenaCleared)) {
                restartGame();
                return;
            }
            // Enter to restart on win/death
            if (e.key.keysym.sym == SDLK_RETURN && (playerDead || arenaCleared)) {
                restartGame();
                return;
            }
        }
        if (e.type == SDL_MOUSEMOTION) {
            float sens = settings ? settings->sensitivity : 0.1f;
            player.applyMouseLook((float)e.motion.xrel, (float)e.motion.yrel, sens);
        }
    }

    void update(float dt) override {
        (void)dt;
        if (paused || playerDead || arenaCleared) {
            if (playerDead) {
                deadTimer += dt;
                // No longer auto-return to menu; player presses R or Enter
            }
            if (arenaCleared) {
                arenaTimer += dt;
            }
            return;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed = (double)(now - prevTicks) / (double)freq;
        prevTicks = now;
        accumulator += elapsed;
        if (accumulator > 0.25) accumulator = 0.25;

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        int mouseButtons = SDL_GetMouseState(nullptr,nullptr);
        bool leftMouse  = (mouseButtons & SDL_BUTTON(1)) != 0;
        bool rightMouse = (mouseButtons & SDL_BUTTON(3)) != 0;
        bool parryKey   = keys[SDL_SCANCODE_F] != 0;

        while (accumulator >= PHYSICS_DT) {
            prevCamPos = player.camera.position;
            if (hitStopFrames > 0) {
                --hitStopFrames;  // freeze simulation, consume one tick
            } else {
                physicsTick(PHYSICS_DT, keys, leftMouse, rightMouse, parryKey);
            }
            accumulator -= PHYSICS_DT;
        }

        float floatDt = (float)elapsed;
        styleSystem.update(floatDt);
        ui.update(floatDt, styleSystem);
        level.update(floatDt);

        // Rebuild world mesh while a door is sliding open
        if (level.anyDoorSliding()) {
            allWalls  = level.getAllWalls();
            spatialGrid.build(allWalls);
            worldMesh = buildWorldMesh(allWalls);
            auto split = buildSplitWorldMesh(allWalls);
            floorMesh = std::move(split.floor);
            wallMesh  = std::move(split.walls);
            ceilMesh  = std::move(split.ceiling);
        }

        viewModel.update(floatDt, playerXZSpeed, player.onGround);
        fovKick = glm::mix(fovKick, 0.f, std::min(1.f, floatDt * 7.f));

        // Weapon switch timer — swap at midpoint of animation
        if (pendingWeapon >= 0) {
            weaponSwitchTimer -= floatDt;
            if (weaponSwitchTimer <= 0.f) {
                activeWeapon  = pendingWeapon;
                pendingWeapon = -1;
            }
        }

        // Track gameplay stats
        elapsedTime += floatDt;
        if (styleSystem.style > peakStyle) peakStyle = styleSystem.style;

        // Wave pause timer — spawn next wave when pause ends
        if (wavePauseTimer > 0.f) {
            wavePauseTimer -= floatDt;
            if (wavePauseTimer <= 0.f) {
                wavePauseTimer = 0.f;
                spawnWaveEnemies(level.currentRoom, waveNumber - 1);
            }
        }

        // Wave banner timer
        if (waveBannerTimer > 0.f) waveBannerTimer -= floatDt;

        // Update explosion particles
        for (auto& p : particles) {
            if (!p.alive) continue;
            p.vel.y -= 18.f * floatDt;   // lighter gravity for visual particles
            p.pos   += p.vel * floatDt;
            p.life  -= floatDt;
            if (p.life <= 0.f) p.alive = false;
        }

        // Spawn pad pulse + enemy respawn
        spawnPadPulse += floatDt * 2.5f;
        respawnTimer  += floatDt;
        if (respawnTimer >= RESPAWN_INTERVAL) {
            respawnTimer = 0.f;
            int alive = 0;
            for (auto& e : enemies) if (e.alive) ++alive;
            if (alive < MIN_ALIVE && !level.rooms.empty()) {
                auto& spawns = level.rooms[0].enemySpawns;
                if (!spawns.empty()) {
                    glm::vec3 sp = spawns[nextSpawnIdx % (int)spawns.size()];
                    nextSpawnIdx++;
                    EnemyType t = (EnemyType)(nextSpawnIdx % 3);
                    enemies.push_back(Enemy(t, sp));
                }
            }
        }

        if (muzzleFlashTimer > 0.f) {
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

        leftMousePrev  = leftMouse;
        rightMousePrev = rightMouse;
        parryPrev      = parryKey;

        // FPS tracking
        fpsFrameCount++;
        fpsTimer += floatDt;
        if (fpsTimer >= 1.0f) {
            ui.currentFPS   = fpsFrameCount;
            fpsFrameCount   = 0;
            fpsTimer        = 0.f;
        }
        ui.showFPS = settings ? settings->showFPS : false;

        checkRoomClear();

        if (!styleSystem.isAlive()) {
            playerDead = true;
            deadTimer = 0.f;
        }
    }

    void physicsTick(float dt, const Uint8* keys, bool leftMouse, bool rightMouse, bool parryKey) {
        // --- Dash ---
        bool dashKey = keys[SDL_SCANCODE_LSHIFT] != 0;
        if (dashKey && !prevDashKey && dashCharges > 0) {
            // Full 3D dash in the direction the camera faces — diagonal, upward, etc.
            glm::vec3 dashDir = player.camera.forward();
            player.velocity = dashDir * 28.f;
            dashMomentumTimer = 0.30f;  // protect dash velocity from ground friction
            --dashCharges;
            dashCooldown = 1.2f;
            fovKick = 13.f;
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
            jumpsRemaining = 1;  // 1 air jump available after landing (ground jump handled by Player.h)
            if (slamming) {
                for (auto& e : enemies) {
                    if (!e.alive) continue;
                    float dist = glm::length(e.position - player.position);
                    if (dist < 4.f) {
                        e.takeDamage(30.f);
                        styleSystem.addStyle(15.f);
                        styleSystem.heal(2.f);
                    }
                }
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

        // --- Grapple fire / release (event-driven, reliable on all hardware) ---
        if (pendingGrapple) {
            glm::vec3 camPos = player.camera.position;
            glm::vec3 camFwd = player.camera.forward();
            if (!grapple.active) {
                // allWalls is already current (no door slides mid-tick)
                glm::vec3 grappleImpulse{0.f};
                if (grapple.fire(camPos, camFwd, allWalls.data(), (int)allWalls.size(), grappleImpulse)) {
                    player.velocity = grappleImpulse;
                    viewModel.triggerGrapple();
                    audio.play("grapple_fire");
                }
            } else {
                grapple.release();
            }
            pendingGrapple = false;
        }

        // Apply grapple force BEFORE player physics so it integrates this tick.
        grapple.update(dt, player.camera.position, player.velocity);

        // --- Update player physics ---
        player.update(dt, keys, allWalls.data(), (int)allWalls.size(),
                      grapple.active || dashMomentumTimer > 0.f, &spatialGrid);
        playerXZSpeed = glm::length(glm::vec2(player.velocity.x, player.velocity.z));

        // --- Footsteps ---
        if (player.onGround && playerXZSpeed > 1.5f && !player.sliding) {
            footstepTimer -= dt;
            if (footstepTimer <= 0.f) {
                float interval = glm::clamp(0.55f / (playerXZSpeed / 5.f), 0.2f, 0.5f);
                footstepTimer = interval;
                audio.play("land", 40);  // reuse land sound at lower volume
            }
        } else {
            footstepTimer = 0.f;
        }

        // Landing sound
        if (justLanded && !slamming) {
            audio.play("land", 70);
        }

        // --- Shooting ---
        revolverTimer    = std::max(0.f, revolverTimer    - dt);
        shotgunTimer     = std::max(0.f, shotgunTimer     - dt);
        grenadeTimer     = std::max(0.f, grenadeTimer     - dt);
        invincFrames     = std::max(0.f, invincFrames     - dt);

        // Revolver reload tick
        if (reloading) {
            reloadTimer -= dt;
            if (reloadTimer <= 0.f) {
                reloading    = false;
                reloadTimer  = 0.f;
                revolverAmmo = revolverAmmoMax;
            }
        }

        // Shotgun reload tick
        if (shotgunReloading) {
            shotgunReloadTimer -= dt;
            if (shotgunReloadTimer <= 0.f) {
                shotgunReloading   = false;
                shotgunReloadTimer = 0.f;
                shotgunAmmo        = shotgunAmmoMax;
            }
        }

        // R key — manual reload
        if (keys[SDL_SCANCODE_R]) {
            if (activeWeapon == 0 && !reloading && revolverAmmo < revolverAmmoMax)
                startReload();
            else if (activeWeapon == 1 && !shotgunReloading && shotgunAmmo < shotgunAmmoMax)
                startShotgunReload();
        }

        // Consume event-driven clicks — fire once per click event (reliable on trackpads)
        // Block firing during weapon switch animation
        bool switchBlocked = (pendingWeapon >= 0);
        if (pendingFire && revolverTimer <= 0.f && !reloading && revolverAmmo > 0 && !switchBlocked) {
            fireRevolver();
        }
        pendingFire = false;

        if (pendingShotgun && shotgunTimer <= 0.f && !shotgunReloading && shotgunAmmo > 0 && !switchBlocked) {
            fireShotgun();
        }
        pendingShotgun = false;

        if (pendingGrenade && grenadeCount > 0 && grenadeTimer <= 0.f) {
            throwGrenade();
        }
        pendingGrenade = false;

        // --- Recoil recovery ---
        if (recoilPitch > 0.f) {
            float recovery = 12.f * dt;  // degrees/sec recovery rate
            float applied  = std::min(recovery, recoilPitch);
            player.camera.pitch -= applied;
            recoilPitch -= applied;
            player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);
        }

        // --- Enemy & projectile update ---
        for (auto& e : enemies) {
            if (!e.alive) continue;
            bool fired = e.update(dt, player.camera.position, allWalls.data(), (int)allWalls.size(), &spatialGrid);
            // Telegraph audio: plays once at the start of each enemy wind-up
            if (e.telegraphJustStarted) audio.play("telegraph");
            if (fired) {
                glm::vec3 firePos = e.position + glm::vec3{0,1.2f,0};
                glm::vec3 fireDir = e.getFireDir(player.camera.position);
                projSystem.fire(firePos, fireDir * 16.f, 10.f, false, {0.9f,0.2f,0.1f});
            }
        }

        auto result = projSystem.update(dt, allWalls.data(), (int)allWalls.size(),
                                        enemies, player.camera.position, &spatialGrid);

        // --- Parry ---
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
                glm::vec3 boomPos    = p.position;
                float     boomRadius = glm::max(p.blastRadius, 4.f) * 2.f;
                float     boomDmg    = p.damage * 3.f;
                p.alive = false;

                spawnExplosionParticles(boomPos, boomRadius);
                shakeTimer = 0.5f; shakeIntensity = 0.12f;
                audio.play("explosion");

                for (auto& e : enemies) {
                    if (!e.alive) continue;
                    float d = glm::length(e.position - boomPos);
                    if (d < boomRadius) {
                        float falloff = 1.f - d / boomRadius;
                        e.takeDamage(boomDmg * falloff);
                        styleSystem.addStyle(25.f);
                        styleSystem.heal(5.f);
                        ui.onHit(!e.alive);
                        if (!e.alive) {
                            onEnemyKilled(e.position, getEnemyColor(e.type));
                            hitStopFrames = glm::max(hitStopFrames, 3);
                        }
                    }
                }
                styleSystem.addStyle(40.f);
                invincFrames  = 0.6f;
                hitStopFrames = glm::max(hitStopFrames, 2);
                audio.play("parry");
            }
        }

        for (auto& [pi,ei] : result.enemyHits) {
            auto& e = enemies[ei];
            float dmg = projSystem.pool[pi].damage;
            e.takeDamage(dmg);
            styleSystem.addStyle(10.f);
            styleSystem.heal(2.f);
            audio.play("hit");
            spawnDecal(e.position);
            bool killed = !e.alive;
            ui.onHit(killed);
            if (killed) {
                onEnemyKilled(e.position, getEnemyColor(e.type));
                hitStopFrames = glm::max(hitStopFrames, 2);  // 2-frame freeze on kill
            }
        }

        // Grenade explosions — AoE damage to all enemies in radius
        for (auto& exp : result.explosions) {
            spawnExplosionParticles(exp.pos, exp.radius);
            shakeTimer = 0.35f; shakeIntensity = 0.07f;
            audio.play("explosion");
            for (auto& e : enemies) {
                if (!e.alive) continue;
                float d = glm::length(e.position - exp.pos);
                if (d < exp.radius) {
                    float falloff = 1.f - (d / exp.radius);
                    e.takeDamage(exp.damage * falloff);
                    styleSystem.addStyle(15.f);
                    styleSystem.heal(3.f);
                    ui.onHit(!e.alive);
                    if (!e.alive) onEnemyKilled(e.position, getEnemyColor(e.type));
                }
            }
            // Check player self-damage (if too close)
            float pd = glm::length(player.camera.position - exp.pos);
            if (pd < exp.radius * 0.5f && invincFrames <= 0.f) {
                float falloff = 1.f - (pd / (exp.radius * 0.5f));
                styleSystem.takeDamage(20.f * falloff);
                ui.onDamage();
            }
        }

        if (result.hitPlayer && invincFrames <= 0.f) {
            styleSystem.takeDamage(result.playerDamage);
            ui.onDamage();
            shakeTimer = 0.2f;
            shakeIntensity = 0.04f;
            audio.play("player_hit");
            invincFrames = 0.3f;
            grapple.release();
            // Damage direction indicator: find the source projectile
            for (auto& p : projSystem.pool) {
                if (!p.isPlayer && !p.alive) {
                    glm::vec3 toProj = p.position - player.camera.position;
                    float angle = atan2f(toProj.x, toProj.z) - glm::radians(player.camera.yaw + 90.f);
                    ui.onDamageFrom(angle);
                    break;
                }
            }
        }

        if (shakeTimer > 0.f) shakeTimer -= dt;

        // --- Interactable scan ---
        nearInteractable = findInteractTarget() >= 0;

        // Age tracers
        for (auto& t : tracers) {
            if (t.alive) { t.life -= dt; if (t.life <= 0.f) t.alive = false; }
        }

        // Age decals
        for (auto& d : decals) {
            if (d.alive) { d.life -= dt; if (d.life <= 0.f) d.alive = false; }
        }
    }

    void checkRoomClear() {
        if (wavePauseTimer > 0.f) return;  // between waves

        int roomIdx = level.currentRoom;
        if (roomIdx >= (int)level.rooms.size()) return;
        auto& room = level.rooms[roomIdx];
        if (room.cleared) return;

        // Check if all enemies from current wave are dead
        bool allDead = true;
        for (int i = waveEnemyStart; i < (int)enemies.size(); ++i) {
            if (enemies[i].alive) { allDead = false; break; }
        }

        if (allDead) {
            int currentWave = waveNumber - 1;  // 0-based
            if (currentWave + 1 < wavesPerRoom) {
                // More waves in this room
                waveNumber++;
                wavePauseTimer  = 3.0f;
                waveBannerTimer = 3.0f;
            } else {
                // Room cleared — all waves done
                room.cleared = true;
                if (roomIdx + 1 < (int)level.rooms.size()) {
                    level.currentRoom++;
                    spawnEnemiesForRoom(level.currentRoom);
                    allWalls = level.getAllWalls();
                    spatialGrid.build(allWalls);
                    worldMesh = buildWorldMesh(allWalls);
                    auto split = buildSplitWorldMesh(allWalls);
                    floorMesh = std::move(split.floor);
                    wallMesh  = std::move(split.walls);
                    ceilMesh  = std::move(split.ceiling);
                } else {
                    // All rooms cleared — arena victory!
                    arenaCleared = true;
                    arenaTimer   = 0.f;
                }
            }
        }
    }

    void spawnDeathParticles(glm::vec3 center, glm::vec3 enemyColor) {
        int spawned = 0;
        for (auto& p : particles) {
            if (p.alive) continue;
            if (spawned >= 8) break;
            float rx = ((rand() % 2001) - 1000) / 1000.f;
            float ry = ((rand() % 1000)) / 1000.f * 0.8f + 0.3f;
            float rz = ((rand() % 2001) - 1000) / 1000.f;
            float len = sqrtf(rx*rx + ry*ry + rz*rz);
            if (len < 0.001f) { rx=0; ry=1; rz=0; len=1; }
            float speed = 4.f + (rand() % 1000) / 1000.f * 6.f;
            p.pos     = center + glm::vec3{0, 0.9f, 0};
            p.vel     = glm::vec3{rx,ry,rz} / len * speed;
            p.maxLife = 0.6f + (rand() % 1000) / 1000.f * 0.5f;
            p.life    = p.maxLife;
            p.color   = enemyColor;
            p.alive   = true;
            ++spawned;
        }
    }

    void spawnShellCasing(glm::vec3 origin, glm::vec3 right) {
        for (auto& p : particles) {
            if (p.alive) continue;
            p.pos     = origin + right * 0.15f;
            p.vel     = right * 2.5f + glm::vec3{0, 3.f, 0}
                      + glm::vec3{((rand()%100)-50)/100.f, 0, ((rand()%100)-50)/100.f};
            p.maxLife = 0.7f;
            p.life    = p.maxLife;
            p.color   = {0.85f, 0.7f, 0.15f}; // golden brass
            p.alive   = true;
            return;
        }
    }

    void spawnExplosionParticles(glm::vec3 center, float radius) {
        int spawned = 0;
        for (auto& p : particles) {
            if (p.alive) continue;
            if (spawned >= 50) break;
            // Random outward direction with upward bias
            float rx = ((rand() % 2001) - 1000) / 1000.f;
            float ry = ((rand() % 1000)) / 1000.f * 0.6f + 0.2f; // bias upward
            float rz = ((rand() % 2001) - 1000) / 1000.f;
            float len = sqrtf(rx*rx + ry*ry + rz*rz);
            if (len < 0.001f) { rx=0; ry=1; rz=0; len=1; }
            float speed = 8.f + (rand() % 1000) / 1000.f * (radius * 3.f);
            p.pos     = center + glm::vec3{rx,ry,rz} / len * (radius * 0.3f);
            p.vel     = glm::vec3{rx,ry,rz} / len * speed;
            p.maxLife = 0.5f + (rand() % 1000) / 1000.f * 0.7f;
            p.life    = p.maxLife;
            p.alive   = true;
            ++spawned;
        }
    }

    void spawnDecal(glm::vec3 enemyPos) {
        // Find a free slot; if full, overwrite the one closest to expiry.
        int slot = -1;
        float minLife = 1e9f;
        for (int i = 0; i < MAX_DECALS; ++i) {
            if (!decals[i].alive) { slot = i; break; }
            if (decals[i].life < minLife) { minLife = decals[i].life; slot = i; }
        }
        auto& d  = decals[slot];
        d.pos    = enemyPos + glm::vec3{0.f, 0.025f, 0.f}; // hover 2.5 cm above floor
        d.maxLife = 10.f;
        d.life    = d.maxLife;
        d.alive   = true;
    }

    void renderDecals() {
        // Each decal is a flat XZ quad (faces up). Vertices use the same layout
        // as worldShader so we can render them with the already-bound shader.
        struct DVert { float x,y,z, u,v, nx,ny,nz, r,g,b; };
        DVert buf[MAX_DECALS * 6];
        int count = 0;

        for (auto& d : decals) {
            if (!d.alive) continue;
            float fade = d.life / d.maxLife;
            // Blood red, dims as the decal ages
            float r = 0.55f * fade, g = 0.03f * fade, b = 0.03f * fade;
            float s = 0.35f; // half-size in metres
            float px = d.pos.x, py = d.pos.y, pz = d.pos.z;

            // Two CCW triangles forming a horizontal quad
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

        // Polygon offset pushes decal surfaces slightly toward the camera in
        // depth so they don't Z-fight with the floor geometry beneath them.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.f, -1.f);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(decalVAO);
        glDrawArrays(GL_TRIANGLES, 0, count);
        glBindVertexArray(0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_CULL_FACE);
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

    static glm::vec3 getEnemyColor(EnemyType t) {
        switch (t) {
            case EnemyType::GRUNT:   return {0.8f,0.2f,0.2f};
            case EnemyType::SHOOTER: return {0.2f,0.3f,0.9f};
            case EnemyType::STALKER: return {0.8f,0.6f,0.0f};
            case EnemyType::FLYER:   return {0.7f,0.1f,0.9f};
        }
        return {1,1,1};
    }

    void onEnemyKilled(glm::vec3 deathPos = {0,0,0}, glm::vec3 deathColor = {0.8f,0.2f,0.2f}) {
        ++totalKills;
        styleSystem.addStyle(30.f);
        styleSystem.heal(5.f);
        audio.play("enemy_death");
        spawnDeathParticles(deathPos, deathColor);
        if (styleSystem.overdrive) dashCharges = 2;
        // Grenade refill: every 2 kills grants 1 grenade (up to max)
        killsThisCycle++;
        if (killsThisCycle >= 2) {
            killsThisCycle = 0;
            if (grenadeCount < grenadeMax) {
                grenadeCount++;
                ui.onGrenadeRefill();
            }
        }
    }

    void startReload() {
        reloading     = true;
        reloadTimer   = RELOAD_TIME;
        viewModel.triggerReload();
        audio.play("reload");
    }

    void fireRevolver() {
        --revolverAmmo;
        revolverTimer = 0.15f;  // between-shot rate of fire

        glm::vec3 origin = player.camera.position;
        glm::vec3 dir    = player.camera.forward();

        // Find closest enemy hit
        float bestT    = 1000.f;
        int   hitEnemy = -1;
        for (int ei = 0; ei < (int)enemies.size(); ++ei) {
            auto& e = enemies[ei];
            if (!e.alive) continue;
            float t = rayAABBHit(origin, dir, e.getAABB());
            if (t > 0.f && t < bestT) { bestT = t; hitEnemy = ei; }
        }

        // Find closest wall hit (for tracer endpoint)
        float wallT = 80.f;
        for (auto& w : allWalls) {
            float t = rayAABBHit(origin, dir, w.box);
            if (t > 0.f && t < wallT) wallT = t;
        }

        // Tracer ends at enemy hit, wall hit, or max range
        float tracerDist = (hitEnemy >= 0) ? std::min(bestT, wallT) : wallT;
        spawnTracer(origin + dir * 0.2f, origin + dir * tracerDist);

        if (hitEnemy >= 0) {
            enemies[hitEnemy].takeDamage(35.f);
            styleSystem.addStyle(10.f);
            styleSystem.heal(2.f);
            audio.play("hit");
            spawnDecal(enemies[hitEnemy].position);
            bool killed = !enemies[hitEnemy].alive;
            ui.onHit(killed);
            if (killed) {
                glm::vec3 ec = getEnemyColor(enemies[hitEnemy].type);
                onEnemyKilled(enemies[hitEnemy].position, ec);
                hitStopFrames = glm::max(hitStopFrames, 2);
            }
        }

        // Camera kick — accumulate recoil (recovered smoothly in physicsTick)
        recoilPitch += 0.7f;
        recoilPitch = std::min(recoilPitch, 12.f);
        player.camera.pitch += 0.7f;
        player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);

        viewModel.triggerFire();
        ui.onShoot();
        muzzleFlashPos   = origin + dir * 0.6f;
        muzzleFlashTimer = 0.04f;
        shakeTimer       = 0.06f;
        shakeIntensity   = 0.012f;
        audio.play("revolver");
        ++totalShots;
        if (hitEnemy >= 0) ++totalHits;
        spawnShellCasing(origin, player.camera.right());

        // Auto-reload when empty
        if (revolverAmmo <= 0) startReload();
    }

    void startShotgunReload() {
        shotgunReloading   = true;
        shotgunReloadTimer = SHOTGUN_RELOAD_TIME;
        audio.play("reload");
    }

    void fireShotgun() {
        --shotgunAmmo;
        shotgunTimer = 0.55f;  // pump cycle

        glm::vec3 origin = player.camera.position;
        glm::vec3 fwd    = player.camera.forward();
        glm::vec3 right  = player.camera.right();
        glm::vec3 up     = glm::cross(fwd, right); // camera-aligned up

        static constexpr int   PELLETS      = 10;
        static constexpr float SPREAD       = 0.18f;  // ±~10° half-angle in radians
        static constexpr float PELLET_DMG   = 9.f;    // per pellet; 10 = 90 max

        for (int p = 0; p < PELLETS; ++p) {
            // Random spread within a square cone, then normalise
            float rx = ((rand() % 2001) - 1000) / 1000.f * SPREAD;
            float ry = ((rand() % 2001) - 1000) / 1000.f * SPREAD;
            glm::vec3 dir = glm::normalize(fwd + right * rx + up * ry);

            // Find closest enemy hit
            float bestT    = 1000.f;
            int   hitEnemy = -1;
            for (int ei = 0; ei < (int)enemies.size(); ++ei) {
                auto& e = enemies[ei];
                if (!e.alive) continue;
                float t = rayAABBHit(origin, dir, e.getAABB());
                if (t > 0.f && t < bestT) { bestT = t; hitEnemy = ei; }
            }

            // Find closest wall hit
            float wallT = 60.f;
            for (auto& w : allWalls) {
                float t = rayAABBHit(origin, dir, w.box);
                if (t > 0.f && t < wallT) wallT = t;
            }

            float tracerDist = (hitEnemy >= 0) ? std::min(bestT, wallT) : wallT;
            spawnTracer(origin + dir * 0.2f, origin + dir * tracerDist);

            if (hitEnemy >= 0) {
                enemies[hitEnemy].takeDamage(PELLET_DMG);
                styleSystem.addStyle(3.f);
                styleSystem.heal(0.5f);
                spawnDecal(enemies[hitEnemy].position);
                bool killed = !enemies[hitEnemy].alive;
                ui.onHit(killed);
                if (killed) {
                    glm::vec3 ec = getEnemyColor(enemies[hitEnemy].type);
                    onEnemyKilled(enemies[hitEnemy].position, ec);
                    hitStopFrames = glm::max(hitStopFrames, 2);
                }
            }
        }

        // Camera kick — stronger than revolver, with recoil recovery
        recoilPitch += 2.2f;
        recoilPitch = std::min(recoilPitch, 12.f);
        player.camera.pitch += 2.2f;
        player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);

        viewModel.triggerFire();
        ui.onShoot();
        muzzleFlashPos   = origin + fwd * 0.6f;
        muzzleFlashTimer = 0.07f;
        shakeTimer       = 0.12f;
        shakeIntensity   = 0.025f;
        audio.play("shotgun");
        ++totalShots;
        // Eject 2 shell casings for shotgun
        spawnShellCasing(origin, player.camera.right());
        spawnShellCasing(origin + player.camera.right() * 0.1f, player.camera.right());

        if (shotgunAmmo <= 0) startShotgunReload();
        else viewModel.triggerPump(); // pump animation after each shot
    }

    void throwGrenade() {
        --grenadeCount;
        grenadeTimer = 0.6f;  // brief cooldown between throws

        glm::vec3 origin = player.camera.position;
        glm::vec3 dir    = player.camera.forward();

        // Throw at a slight upward angle so it arcs
        glm::vec3 throwVel = dir * 14.f + glm::vec3{0, 5.f, 0};

        projSystem.fire(origin, throwVel, 80.f, true,
                        {0.3f, 0.9f, 0.1f},  // green glow
                        /*grenade=*/true, /*blastRadius=*/5.f);

        viewModel.triggerGrenade();
        shakeTimer     = 0.05f;
        shakeIntensity = 0.008f;
        audio.play("jump");  // placeholder sound
    }

    void restartGame() {
        player = Player{{0.f,0.f,60.f}};
        player.camera.aspectRatio = (float)SCREEN_W/SCREEN_H;
        player.camera.fov = settings ? settings->fov : 90.f;
        styleSystem = StyleSystem{};
        enemies.clear();
        level = loadLevelFromFile("assets/level.txt");
        if (level.rooms.empty()) level = buildLevel();
        allWalls = level.getAllWalls();
        spatialGrid.build(allWalls);
        worldMesh = buildWorldMesh(allWalls);
        {
            auto split = buildSplitWorldMesh(allWalls);
            floorMesh = std::move(split.floor);
            wallMesh  = std::move(split.walls);
            ceilMesh  = std::move(split.ceiling);
        }
        spawnEnemiesForRoom(0);
        grenadeCount       = grenadeMax;
        killsThisCycle     = 0;
        revolverAmmo       = revolverAmmoMax;
        reloading          = false;
        reloadTimer        = 0.f;
        revolverTimer      = 0.f;
        shotgunAmmo        = shotgunAmmoMax;
        shotgunReloading   = false;
        shotgunReloadTimer = 0.f;
        shotgunTimer       = 0.f;
        dashCharges    = 2;
        jumpsRemaining = 2;
        playerDead = false;
        deadTimer = 0.f;
        paused = false;
        activeWeapon   = 0;
        pendingWeapon  = -1;
        weaponSwitchTimer = 0.f;
        recoilPitch    = 0.f;
        totalKills     = 0;
        totalShots     = 0;
        totalHits      = 0;
        elapsedTime    = 0.f;
        peakStyle      = 0.f;
        arenaCleared   = false;
        arenaTimer     = 0.f;
        waveNumber     = 1;
        waveBannerTimer = 0.f;
        wavePauseTimer  = 0.f;
        waveEnemyStart  = 0;
        for (auto& di : ui.damageIndicators) di.timer = 0.f;
        ui.damageIndicatorCount = 0;
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    void render() override {
        float alpha = (float)(accumulator / PHYSICS_DT);
        glm::vec3 renderCamPos = glm::mix(prevCamPos, player.camera.position, alpha);
        Camera renderCam = player.camera;
        renderCam.position = renderCamPos;

        if (shakeTimer > 0.f) {
            float s = shakeIntensity * (shakeTimer / 0.2f);
            renderCam.position += glm::vec3{
                ((float)(rand()%200)-100)/100.f * s,
                ((float)(rand()%200)-100)/100.f * s,
                0.f
            };
        }

        // Camera bob — subtle positional sway tied to movement
        renderCam.position += viewModel.getBobOffset(playerXZSpeed, player.onGround,
                                                     renderCam.right());

        // FOV kick from dash
        renderCam.fov = player.camera.fov + fovKick;

        glm::mat4 view = renderCam.viewMatrix();
        glm::mat4 proj = renderCam.projectionMatrix();

        postProcess.beginScene();
        glClearColor(0.05f,0.05f,0.1f,1.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // Draw skybox
        {
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            glm::mat4 skyView = glm::mat4(glm::mat3(view));
            skyboxShader.use();
            skyboxShader.setMat4("view",       skyView);
            skyboxShader.setMat4("projection", proj);
            glBindVertexArray(postProcess.quadVAO);
            glDrawArrays(GL_TRIANGLES,0,6);
            glBindVertexArray(0);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

        // Draw world
        worldShader.use();
        worldShader.setMat4("projection", proj);
        worldShader.setMat4("view",       view);
        worldShader.setMat4("model",      glm::mat4(1.f));
        worldShader.setVec3("lightDir",   glm::normalize(glm::vec3{0.4f,-1.f,0.3f}));
        worldShader.setVec3("lightColor", {1.f,0.9f,0.8f});
        worldShader.setVec3("ambientColor",{0.15f,0.15f,0.2f});
        worldShader.setVec3("viewPos",    renderCamPos);
        worldShader.setVec3 ("emissiveColor",  {0.f,0.f,0.f});
        worldShader.setVec3 ("objectColor",    {1.f,1.f,1.f});
        worldShader.setFloat("uPSXStrength",   0.0f);  // set > 0 to enable vertex jitter
        for (int i=0;i<MAX_POINT_LIGHTS;++i) {
            std::string pn = "pointLightPos["+std::to_string(i)+"]";
            std::string cn = "pointLightColor["+std::to_string(i)+"]";
            worldShader.setVec3(pn.c_str(),   pointLightPos[i]);
            worldShader.setVec3(cn.c_str(),   pointLightColor[i]);
        }
        worldShader.setInt("uTexture",0);
        glActiveTexture(GL_TEXTURE0);
        // Draw floor with grid texture
        glBindTexture(GL_TEXTURE_2D, floorTex);
        floorMesh.draw();
        // Draw walls with brick texture
        glBindTexture(GL_TEXTURE_2D, wallTex);
        wallMesh.draw();
        // Draw ceiling with metal texture
        glBindTexture(GL_TEXTURE_2D, ceilTex);
        ceilMesh.draw();

        // Spawn pads — pulsing green emissive quads on the floor
        {
            float glow = 0.4f + 0.35f * std::sin(spawnPadPulse);
            // Pads pulse more when wave is incoming
            if (wavePauseTimer > 0.f) glow += 0.3f;
            worldShader.setVec3("emissiveColor", {0.f, glow, glow * 0.4f});
            worldShader.setVec3("objectColor",   {0.f, 0.4f, 0.1f});
            spawnPadMesh.draw();
            worldShader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
            worldShader.setVec3("objectColor",   {1.f, 1.f, 1.f});
        }

        // Set lighting uniforms on the instanced enemy shader, then draw.
        enemyRenderer.shader.use();
        enemyRenderer.shader.setMat4("projection", proj);
        enemyRenderer.shader.setMat4("view",       view);
        enemyRenderer.shader.setVec3("viewPos",    renderCamPos);
        enemyRenderer.shader.setVec3("lightDir",   glm::normalize(glm::vec3{0.4f,-1.f,0.3f}));
        enemyRenderer.shader.setVec3("lightColor", {1.f,0.9f,0.8f});
        enemyRenderer.shader.setVec3("ambientColor",{0.15f,0.15f,0.2f});
        enemyRenderer.shader.setInt ("uTexture",   0);
        for (int i=0;i<MAX_POINT_LIGHTS;++i) {
            std::string pn = "pointLightPos["  + std::to_string(i) + "]";
            std::string cn = "pointLightColor[" + std::to_string(i) + "]";
            enemyRenderer.shader.setVec3(pn.c_str(), pointLightPos[i]);
            enemyRenderer.shader.setVec3(cn.c_str(), pointLightColor[i]);
        }
        enemyRenderer.draw(enemies, elapsedTime);

        // Enemy health bars — billboard quads above damaged enemies
        worldShader.use();
        worldShader.setMat4("projection", proj);
        worldShader.setMat4("view",       view);
        glBindTexture(GL_TEXTURE_2D, whiteTex);
        glDisable(GL_CULL_FACE);
        for (auto& e : enemies) {
            if (!e.alive) continue;
            if (e.hitFlashTimer <= 0.f && e.health >= e.maxHealth) continue;
            // Show health bar for 2s after damage
            if (e.health >= e.maxHealth) continue;

            float barW = 1.2f, barH = 0.12f;
            float fill = e.health / e.maxHealth;
            glm::vec3 barPos = e.position + glm::vec3{0, 2.2f, 0};

            // Billboard: face camera
            glm::vec3 camRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));

            // Background (dark)
            glm::mat4 bgModel = glm::translate(glm::mat4(1.f), barPos);
            bgModel = bgModel * glm::scale(glm::mat4(1.f), {barW, barH, 0.01f});
            bgModel[0] = glm::vec4(camRight * barW, 0);
            bgModel[1] = glm::vec4(0, barH, 0, 0);
            bgModel[2] = glm::vec4(glm::normalize(glm::cross(camRight, glm::vec3(0,1,0))) * 0.01f, 0);
            bgModel[3] = glm::vec4(barPos, 1);
            worldShader.setMat4("model", bgModel);
            worldShader.setVec3("objectColor", {0.1f, 0.1f, 0.1f});
            worldShader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
            viewModel.cubeMesh.draw();

            // Red fill
            glm::vec3 fillPos = barPos - camRight * (barW * (1.f - fill) * 0.5f);
            glm::mat4 fillModel(1.f);
            fillModel[0] = glm::vec4(camRight * barW * fill, 0);
            fillModel[1] = glm::vec4(0, barH * 0.8f, 0, 0);
            fillModel[2] = glm::vec4(glm::normalize(glm::cross(camRight, glm::vec3(0,1,0))) * 0.02f, 0);
            fillModel[3] = glm::vec4(fillPos, 1);
            worldShader.setMat4("model", fillModel);
            worldShader.setVec3("objectColor", {0.9f, 0.15f, 0.1f});
            worldShader.setVec3("emissiveColor", {0.3f, 0.05f, 0.02f});
            viewModel.cubeMesh.draw();
        }
        glEnable(GL_CULL_FACE);
        worldShader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        worldShader.setVec3("objectColor", {1.f, 1.f, 1.f});
        worldShader.setMat4("model", glm::mat4(1.f));

        grapple.drawLine(player.position + glm::vec3{0,player.eyeHeight,0}, view, proj);

        renderDecals();

        projSystem.draw(worldShader, view, proj);

        // Tracers drawn with additive blending — must be after opaque geometry
        renderTracers(view, proj, renderCamPos);

        // Explosion particles — additive GL_POINTS
        renderParticles(view, proj);

        // View model — drawn last inside FBO so it receives bloom; depth cleared inside
        {
            float revolverFill = reloading ? (1.f - reloadTimer / RELOAD_TIME)
                                           : (float)revolverAmmo / (float)revolverAmmoMax;
            float shotgunFill  = shotgunReloading ? (1.f - shotgunReloadTimer / SHOTGUN_RELOAD_TIME)
                                                  : (shotgunTimer > 0.f ? (1.f - shotgunTimer / 0.55f) : 1.f);
            worldShader.use();
            worldShader.setVec3("lightDir",    glm::normalize(glm::vec3{0.4f,-1.f,0.3f}));
            worldShader.setVec3("lightColor",  {1.f,0.9f,0.8f});
            worldShader.setVec3("ambientColor",{0.25f,0.25f,0.30f});
            worldShader.setVec3("viewPos",     renderCamPos);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, whiteTex);
            worldShader.setInt("uTexture", 0);
            viewModel.draw(worldShader, renderCam, activeWeapon, revolverFill, shotgunFill);
            // Restore world projection/view after viewmodel (postProcess.endScene reads no uniforms)
            worldShader.setMat4("projection", proj);
            worldShader.setMat4("view",       view);
        }

        postProcess.crtEnabled = settings ? settings->crtFilter : false;
        postProcess.endScene();

        float reloadProgress = reloading ? (1.f - reloadTimer / RELOAD_TIME) : 1.f;
        float shotgunReloadProgress = shotgunReloading
                                    ? (1.f - shotgunReloadTimer / SHOTGUN_RELOAD_TIME) : 1.f;
        ui.render(styleSystem, activeWeapon,
                  shotgunAmmo, shotgunAmmoMax, shotgunReloading, shotgunReloadProgress,
                  revolverAmmo, revolverAmmoMax, reloading, reloadProgress,
                  nearInteractable,
                  arenaCleared, playerDead,
                  totalKills, totalShots, totalHits,
                  elapsedTime, peakStyle,
                  waveNumber, waveBannerTimer);
    }

    void renderParticles(const glm::mat4& view, const glm::mat4& proj) {
        struct PVert { float x, y, z, a; };
        PVert buf[MAX_PARTICLES];
        int count = 0;
        for (auto& p : particles) {
            if (!p.alive) continue;
            float t = p.life / p.maxLife;
            buf[count++] = { p.pos.x, p.pos.y, p.pos.z, t * t };
        }
        if (count == 0) return;

        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(PVert), buf);

        tracerShader.use();
        tracerShader.setMat4("projection", proj);
        tracerShader.setMat4("view",       view);

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive — particles glow

        glBindVertexArray(particleVAO);

        // Two passes: bright orange core + darker red outer particles
        tracerShader.setVec3("uColor", {1.f, 0.55f, 0.05f});
        glPointSize(5.f);
        glDrawArrays(GL_POINTS, 0, count);

        tracerShader.setVec3("uColor", {1.f, 0.20f, 0.02f});
        glPointSize(3.f);
        glDrawArrays(GL_POINTS, 0, count);

        glBindVertexArray(0);
        glPointSize(1.f);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    void renderTracers(const glm::mat4& view, const glm::mat4& proj,
                       const glm::vec3& camPos) {
        struct TVert { float x,y,z,a; };
        TVert buf[MAX_TRACERS * 6];
        int   count = 0;

        for (auto& t : tracers) {
            if (!t.alive) continue;
            float fade = t.life / t.maxLife;

            glm::vec3 ray = t.end - t.start;
            float len = glm::length(ray);
            if (len < 0.001f) continue;
            glm::vec3 dir = ray / len;

            // Billboard side axis — must be perpendicular to the ray.
            // We CANNOT use (midpoint - camPos) because for a hitscan shot
            // that axis is nearly parallel to dir, making cross(dir, dir) ≈ 0.
            // Instead cross with world-up; fall back to world-right if the ray
            // points straight up or down.
            glm::vec3 side = glm::cross(dir, glm::vec3(0.f, 1.f, 0.f));
            if (glm::length(side) < 0.01f)
                side = glm::cross(dir, glm::vec3(1.f, 0.f, 0.f));
            side = glm::normalize(side);

            float w = 0.055f; // tracer half-width in world units

            // Quad: full brightness at gun end, nearly invisible at impact end.
            // Gives a sense of the shot landing rather than a static beam.
            float aN = fade;          // near alpha
            float aF = fade * 0.08f;  // far alpha

            glm::vec3 s0 = t.start - side * w, s1 = t.start + side * w;
            glm::vec3 e0 = t.end   - side * w, e1 = t.end   + side * w;

            buf[count++] = {s0.x,s0.y,s0.z, aN};
            buf[count++] = {s1.x,s1.y,s1.z, aN};
            buf[count++] = {e1.x,e1.y,e1.z, aF};
            buf[count++] = {s0.x,s0.y,s0.z, aN};
            buf[count++] = {e1.x,e1.y,e1.z, aF};
            buf[count++] = {e0.x,e0.y,e0.z, aF};
        }
        if (count == 0) return;

        glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(TVert), buf);

        tracerShader.use();
        tracerShader.setMat4("projection", proj);
        tracerShader.setMat4("view",       view);
        tracerShader.setVec3("uColor",     {0.97f, 0.95f, 0.72f}); // warm white-yellow

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive — tracer glows
        glBindVertexArray(tracerVAO);
        glDrawArrays(GL_TRIANGLES, 0, count);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    // Returns index into interactables[] of the one the player is looking at,
    // or -1 if none. Uses the same ray-AABB test as the revolver hitscan.
    int findInteractTarget() const {
        glm::vec3 origin = player.camera.position;
        glm::vec3 dir    = player.camera.forward();
        int   best    = -1;
        float bestT   = Interactable::INTERACT_RANGE;
        for (int i = 0; i < (int)interactables.size(); ++i) {
            if (!interactables[i].active) continue;
            float t = rayAABBHit(origin, dir, interactables[i].box);
            if (t > 0.f && t < bestT) { bestT = t; best = i; }
        }
        return best;
    }

private:
    static float rayAABBHit(glm::vec3 o, glm::vec3 d, const AABB& b) {
        glm::vec3 invD{1.f/(d.x+1e-9f),1.f/(d.y+1e-9f),1.f/(d.z+1e-9f)};
        glm::vec3 t0=(b.min-o)*invD, t1=(b.max-o)*invD;
        glm::vec3 tMin=glm::min(t0,t1), tMax=glm::max(t0,t1);
        float tEnter=std::max({tMin.x,tMin.y,tMin.z});
        float tExit =std::min({tMax.x,tMax.y,tMax.z});
        if(tEnter>tExit||tExit<0.f) return -1.f;
        return tEnter>0.f?tEnter:tExit;
    }
};
