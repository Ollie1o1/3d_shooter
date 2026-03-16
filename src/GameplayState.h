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

// Colors for different surface types — edit these to restyle the arena.
// Each color is a linear RGB value applied as a per-vertex tint.
namespace ArenaColor {
    constexpr glm::vec3 Floor    = {0.18f, 0.16f, 0.14f}; // dark warm ground
    constexpr glm::vec3 Ceiling  = {0.10f, 0.10f, 0.16f}; // very dark cool sky
    constexpr glm::vec3 Perim    = {0.26f, 0.26f, 0.34f}; // outer walls — slate
    constexpr glm::vec3 Hub      = {0.38f, 0.22f, 0.06f}; // central tower — burnt orange
    constexpr glm::vec3 Riser    = {0.22f, 0.27f, 0.22f}; // approach steps — olive grey
    constexpr glm::vec3 Corner   = {0.16f, 0.20f, 0.30f}; // fortress / raised sections — steel blue
    constexpr glm::vec3 Ledge    = {0.24f, 0.24f, 0.30f}; // elevated walkways
    constexpr glm::vec3 Cover    = {0.34f, 0.26f, 0.16f}; // cover blocks — sandy
    constexpr glm::vec3 Pillar   = {0.48f, 0.32f, 0.04f}; // tall pillars — bright orange
    constexpr glm::vec3 Catwalk  = {0.18f, 0.18f, 0.24f}; // perimeter catwalks
    constexpr glm::vec3 Tower    = {0.20f, 0.16f, 0.30f}; // corner towers — dark purple
    constexpr glm::vec3 SkyPad   = {0.08f, 0.45f, 0.65f}; // sky-blue floating platforms
    constexpr glm::vec3 Spire    = {0.50f, 0.08f, 0.72f}; // tall grapple spires — vivid purple
    constexpr glm::vec3 FarFort  = {0.32f, 0.10f, 0.10f}; // far north bastion — dark red
}

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
        unsigned int base = (unsigned int)verts.size();
        verts.push_back({a, {0,0}, n, gc(a.y)});
        verts.push_back({b, {1,0}, n, gc(b.y)});
        verts.push_back({c, {1,1}, n, gc(c.y)});
        verts.push_back({d, {0,1}, n, gc(d.y)});
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    // Push the 5 visible faces of an AABB box with a given colour.
    // Vertex ordering is CCW when viewed from OUTSIDE (the direction the normal points),
    // which is what OpenGL requires for front-face culling. All cross-products verified.
    // Bottom face (-Y) is omitted — it sits on the floor and Z-fights with the floor quad.
    auto pushBox = [&](const AABB& b, glm::vec3 col) {
        glm::vec3 mn = b.min, mx = b.max;
        // +X: right face — CCW from +X side  cross((b-a)×(c-a)) = +X
        pushFace({mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z},{1,0,0}, col);
        // -X: left face — CCW from -X side
        pushFace({mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z},{mn.x,mn.y,mn.z},{-1,0,0}, col);
        // +Z: front face — CCW from +Z side (unchanged, already correct)
        pushFace({mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z},{0,0,1}, col);
        // -Z: back face — CCW from -Z side (unchanged, already correct)
        pushFace({mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{0,0,-1}, col);
        // +Y: top face — CCW from above
        pushFace({mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z},{0,1,0}, col);
    };

    // Floor — one large quad, +Y normal, CCW from above
    float F = 65.f;
    pushFace({-F,0,F},{F,0,F},{F,0,-F},{-F,0,-F},{0,1,0}, ArenaColor::Floor);
    // Ceiling — -Y normal, CCW from below
    pushFace({-F,14.f,-F},{F,14.f,-F},{F,14.f,F},{-F,14.f,F},{0,-1,0}, ArenaColor::Ceiling);

    // Assign a color to each wall based on its role in the level.
    // Order must exactly match buildLevel(). Anything beyond the array uses Perim.
    const glm::vec3 wallColors[] = {
        // 0-3: Perimeter
        ArenaColor::Perim, ArenaColor::Perim, ArenaColor::Perim, ArenaColor::Perim,
        // 4: Central hub
        ArenaColor::Hub,
        // 5-8: Hub approach risers
        ArenaColor::Riser, ArenaColor::Riser, ArenaColor::Riser, ArenaColor::Riser,
        // 9: North fortress platform
        ArenaColor::Corner,
        // 10: North fortress access riser
        ArenaColor::Riser,
        // 11-12: North NW / NE towers
        ArenaColor::Pillar, ArenaColor::Pillar,
        // 13: East elevated walkway
        ArenaColor::Ledge,
        // 14: East access step
        ArenaColor::Riser,
        // 15-16: East tall support columns
        ArenaColor::Pillar, ArenaColor::Pillar,
        // 17-18: West raised platforms A / B
        ArenaColor::Corner, ArenaColor::Corner,
        // 19: West tall tower
        ArenaColor::Pillar,
        // 20-29: Cover walls (10)
        ArenaColor::Cover, ArenaColor::Cover, ArenaColor::Cover, ArenaColor::Cover,
        ArenaColor::Cover, ArenaColor::Cover, ArenaColor::Cover, ArenaColor::Cover,
        ArenaColor::Cover, ArenaColor::Cover,
        // 30-37: Mid-arena pillars (8)
        ArenaColor::Pillar, ArenaColor::Pillar, ArenaColor::Pillar, ArenaColor::Pillar,
        ArenaColor::Pillar, ArenaColor::Pillar, ArenaColor::Pillar, ArenaColor::Pillar,
        // 38-39: SW / SE corner towers
        ArenaColor::Tower, ArenaColor::Tower,
        // 40-43: Perimeter catwalks
        ArenaColor::Catwalk, ArenaColor::Catwalk, ArenaColor::Catwalk, ArenaColor::Catwalk,
        // 44-45: Inner floating platforms
        ArenaColor::Ledge, ArenaColor::Ledge,
        // 46: Central sky pad
        ArenaColor::SkyPad,
        // 47-50: Sky bridges (N/S/E/W)
        ArenaColor::SkyPad, ArenaColor::SkyPad, ArenaColor::SkyPad, ArenaColor::SkyPad,
        // 51-54: Corner sky pads (NE/NW/SE/SW)
        ArenaColor::SkyPad, ArenaColor::SkyPad, ArenaColor::SkyPad, ArenaColor::SkyPad,
        // 55-57: Far north expansion
        ArenaColor::FarFort, ArenaColor::Pillar, ArenaColor::Pillar,
        // 58-59: Far south platform + sky shelf
        ArenaColor::Corner, ArenaColor::SkyPad,
        // 60-61: Far east/west sky shelves
        ArenaColor::Ledge, ArenaColor::Ledge,
        // 62-65: Cardinal spires
        ArenaColor::Spire, ArenaColor::Spire, ArenaColor::Spire, ArenaColor::Spire,
    };
    const int numColors = (int)(sizeof(wallColors)/sizeof(wallColors[0]));

    for (int i = 0; i < (int)walls.size(); ++i) {
        glm::vec3 col = (i < numColors) ? wallColors[i] : ArenaColor::Perim;
        pushBox(walls[i].box, col);
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

class GameplayState : public GameState {
public:
    std::function<void()> onReturnToMenu;
    std::function<void()> onQuit;

    Player         player{{0.f,0.f,32.f}};
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
    Mesh     worldMesh;

    std::vector<Enemy>   enemies;
    std::vector<Wall>    allWalls;

    // Slot 1: Revolver — 8 rounds, manual/auto reload
    int   revolverAmmo    = 8;
    int   revolverAmmoMax = 8;
    float revolverTimer   = 0.f;   // between-shot cooldown (rate of fire)
    bool  reloading       = false;
    float reloadTimer     = 0.f;
    static constexpr float RELOAD_TIME = 1.2f;
    // Slot 2: Grenades — max 2, one returned per 2 kills
    int   grenadeCount   = 2;
    int   grenadeMax     = 2;
    float grenadeTimer   = 0.f;   // cooldown between throws
    int   killsThisCycle = 0;     // counts toward next grenade refill (every 2 kills)
    // Slot selection: 0 = revolver, 1 = grenades
    int   activeWeapon   = 0;

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

    // Explosion particles — spawned by grenade blasts, rendered as GL_POINTS
    struct Particle {
        glm::vec3 pos{0.f}, vel{0.f};
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
    static constexpr int   MIN_ALIVE        = 3;    // respawn if below this

    static constexpr int MAX_POINT_LIGHTS = 4;
    glm::vec3 pointLightPos[MAX_POINT_LIGHTS];
    glm::vec3 pointLightColor[MAX_POINT_LIGHTS];

    Uint64 prevTicks   = 0;
    Uint64 freq        = 0;
    double accumulator = 0.0;
    glm::vec3 prevCamPos{0.f};

    bool rightMousePrev   = false;
    bool leftMousePrev    = false;
    bool parryPrev        = false;
    bool prevDashKey      = false;
    bool prevJumpKey      = false;
    // Event-driven click flags — set in handleEvent, consumed once in physicsTick.
    // More reliable than polling SDL_GetMouseState on trackpads.
    bool pendingFire      = false;
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

        whiteTex = makeGreyTexture();
        player.camera.aspectRatio = (float)SCREEN_W/SCREEN_H;
        player.camera.fov = 90.f;

        level = buildLevel();
        allWalls = level.getAllWalls();
        worldMesh = buildWorldMesh(allWalls);
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
        if (tracerVAO)    glDeleteVertexArrays(1, &tracerVAO);
        if (tracerVBO)    glDeleteBuffers(1, &tracerVBO);
        if (particleVAO)  glDeleteVertexArrays(1, &particleVAO);
        if (particleVBO)  glDeleteBuffers(1, &particleVBO);
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    void spawnEnemiesForRoom(int roomIdx) {
        if (roomIdx >= (int)level.rooms.size()) return;
        auto& room = level.rooms[roomIdx];
        int groundIdx = 0;
        for (auto& sp : room.enemySpawns) {
            EnemyType t;
            if (sp.y > 5.f) {
                t = EnemyType::FLYER;
            } else {
                int tidx = groundIdx % 3;
                t = (tidx == 0) ? EnemyType::GRUNT
                  : (tidx == 1) ? EnemyType::SHOOTER
                  :               EnemyType::STALKER;
                ++groundIdx;
            }
            enemies.push_back(Enemy(t, sp));
        }
        room.unlocked = true;
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
                else                   pendingGrenade = true;
            }
            if (e.button.button == SDL_BUTTON_RIGHT) {
                pendingGrapple = true;
            }
        }
        if (e.type == SDL_MOUSEWHEEL) {
            activeWeapon = (activeWeapon + 1) % 2;
        }
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_1) activeWeapon = 0;
            if (e.key.keysym.sym == SDLK_2) activeWeapon = 1;
        }
        if (e.type == SDL_MOUSEMOTION) {
            player.applyMouseLook((float)e.motion.xrel, (float)e.motion.yrel);
        }
    }

    void update(float dt) override {
        (void)dt;
        if (paused || playerDead) {
            if (playerDead) {
                deadTimer += dt;
                if (deadTimer > 3.f && onReturnToMenu) onReturnToMenu();
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
            physicsTick(PHYSICS_DT, keys, leftMouse, rightMouse, parryKey);
            accumulator -= PHYSICS_DT;
        }

        float floatDt = (float)elapsed;
        styleSystem.update(floatDt);
        ui.update(floatDt, styleSystem);
        level.update(floatDt);
        viewModel.update(floatDt, playerXZSpeed, player.onGround);
        fovKick = glm::mix(fovKick, 0.f, std::min(1.f, floatDt * 7.f));

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
            } else if (jumpsRemaining > 0 && !player.onGround) {
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
                allWalls = level.getAllWalls();
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
        allWalls = level.getAllWalls();
        player.update(dt, keys, allWalls.data(), (int)allWalls.size(),
                      grapple.active || dashMomentumTimer > 0.f);
        playerXZSpeed = glm::length(glm::vec2(player.velocity.x, player.velocity.z));

        // --- Shooting ---
        revolverTimer = std::max(0.f, revolverTimer - dt);
        grenadeTimer  = std::max(0.f, grenadeTimer  - dt);
        invincFrames  = std::max(0.f, invincFrames  - dt);

        // Reload tick
        if (reloading) {
            reloadTimer -= dt;
            if (reloadTimer <= 0.f) {
                reloading     = false;
                reloadTimer   = 0.f;
                revolverAmmo  = revolverAmmoMax;
            }
        }

        // R key — manual reload (only if not full and not already reloading)
        if (keys[SDL_SCANCODE_R] && !reloading && revolverAmmo < revolverAmmoMax) {
            startReload();
        }

        // Consume event-driven clicks — fire once per click event (reliable on trackpads)
        if (pendingFire && revolverTimer <= 0.f && !reloading && revolverAmmo > 0) {
            fireRevolver();
        }
        pendingFire = false;

        if (pendingGrenade && grenadeCount > 0 && grenadeTimer <= 0.f) {
            throwGrenade();
        }
        pendingGrenade = false;

        // --- Enemy & projectile update ---
        for (auto& e : enemies) {
            if (!e.alive) continue;
            bool fired = e.update(dt, player.camera.position, allWalls.data(), (int)allWalls.size());
            if (fired) {
                glm::vec3 firePos = e.position + glm::vec3{0,1.2f,0};
                glm::vec3 fireDir = e.getFireDir(player.camera.position);
                projSystem.fire(firePos, fireDir * 16.f, 10.f, false, {0.9f,0.2f,0.1f});
            }
        }

        auto result = projSystem.update(dt, allWalls.data(), (int)allWalls.size(),
                                        enemies, player.camera.position);

        // --- Parry ---
        bool parryPressed = parryKey && !parryPrev;
        parryPrev = parryKey;

        if (parryPressed && result.parryableIndex >= 0) {
            auto& p = projSystem.pool[result.parryableIndex];
            p.isPlayer = true;
            p.damage   = 50.f;
            p.velocity  = -p.velocity * 2.f;
            p.emissiveColor = {1.f,0.9f,0.3f};
            styleSystem.addStyle(20.f);
            invincFrames = 0.5f;
            audio.play("parry");
        }

        for (auto& [pi,ei] : result.enemyHits) {
            auto& e = enemies[ei];
            float dmg = projSystem.pool[pi].damage;
            e.takeDamage(dmg);
            styleSystem.addStyle(10.f);
            styleSystem.heal(2.f);
            audio.play("hit");
            bool killed = !e.alive;
            ui.onHit(killed);
            if (killed) onEnemyKilled();
        }

        // Grenade explosions — AoE damage to all enemies in radius
        for (auto& exp : result.explosions) {
            spawnExplosionParticles(exp.pos, exp.radius);
            shakeTimer = 0.35f; shakeIntensity = 0.07f;
            for (auto& e : enemies) {
                if (!e.alive) continue;
                float d = glm::length(e.position - exp.pos);
                if (d < exp.radius) {
                    float falloff = 1.f - (d / exp.radius);
                    e.takeDamage(exp.damage * falloff);
                    styleSystem.addStyle(15.f);
                    styleSystem.heal(3.f);
                    ui.onHit(!e.alive);
                    if (!e.alive) onEnemyKilled();
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
        }

        if (shakeTimer > 0.f) shakeTimer -= dt;

        // --- Interactable scan ---
        nearInteractable = findInteractTarget() >= 0;

        // Age tracers
        for (auto& t : tracers) {
            if (t.alive) { t.life -= dt; if (t.life <= 0.f) t.alive = false; }
        }
    }

    void checkRoomClear() {
        int roomIdx = level.currentRoom;
        if (roomIdx >= (int)level.rooms.size()) return;
        auto& room = level.rooms[roomIdx];
        if (room.cleared) return;

        int enemyStart = 0;
        for (int r = 0; r < roomIdx; ++r)
            enemyStart += (int)level.rooms[r].enemySpawns.size();

        bool allDead = true;
        for (int i = enemyStart; i < enemyStart+(int)room.enemySpawns.size() && i < (int)enemies.size(); ++i) {
            if (enemies[i].alive) { allDead = false; break; }
        }

        if (allDead) {
            room.cleared = true;
            if (roomIdx + 1 < (int)level.rooms.size()) {
                level.currentRoom++;
                spawnEnemiesForRoom(level.currentRoom);
                allWalls = level.getAllWalls();
                worldMesh = buildWorldMesh(allWalls);
            }
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

    void onEnemyKilled() {
        styleSystem.addStyle(30.f);
        styleSystem.heal(5.f);
        audio.play("enemy_death");
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
            bool killed = !enemies[hitEnemy].alive;
            ui.onHit(killed);
            if (killed) onEnemyKilled();
        }

        // Camera kick
        player.camera.pitch += 0.7f;
        player.camera.pitch = glm::clamp(player.camera.pitch, -89.f, 89.f);

        viewModel.triggerFire();
        ui.onShoot();
        muzzleFlashPos   = origin + dir * 0.6f;
        muzzleFlashTimer = 0.04f;
        shakeTimer       = 0.06f;
        shakeIntensity   = 0.012f;
        audio.play("revolver");

        // Auto-reload when empty
        if (revolverAmmo <= 0) startReload();
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
        player = Player{{0.f,0.f,32.f}};
        player.camera.aspectRatio = (float)SCREEN_W/SCREEN_H;
        styleSystem = StyleSystem{};
        enemies.clear();
        level = buildLevel();
        allWalls = level.getAllWalls();
        worldMesh = buildWorldMesh(allWalls);
        spawnEnemiesForRoom(0);
        grenadeCount   = grenadeMax;
        killsThisCycle = 0;
        revolverAmmo   = revolverAmmoMax;
        reloading      = false;
        reloadTimer    = 0.f;
        revolverTimer  = 0.f;
        dashCharges    = 2;
        jumpsRemaining = 2;
        playerDead = false;
        deadTimer = 0.f;
        paused = false;
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
        worldShader.setVec3("emissiveColor",{0.f,0.f,0.f});
        worldShader.setVec3("objectColor",{1.f,1.f,1.f});
        for (int i=0;i<MAX_POINT_LIGHTS;++i) {
            std::string pn = "pointLightPos["+std::to_string(i)+"]";
            std::string cn = "pointLightColor["+std::to_string(i)+"]";
            worldShader.setVec3(pn.c_str(),   pointLightPos[i]);
            worldShader.setVec3(cn.c_str(),   pointLightColor[i]);
        }
        worldShader.setInt("uTexture",0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,whiteTex);
        worldMesh.draw();

        // Spawn pads — pulsing green emissive quads on the floor
        {
            float glow = 0.4f + 0.35f * std::sin(spawnPadPulse);
            worldShader.setVec3("emissiveColor", {0.f, glow, glow * 0.4f});
            worldShader.setVec3("objectColor",   {0.f, 0.4f, 0.1f});
            spawnPadMesh.draw();
            worldShader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
            worldShader.setVec3("objectColor",   {1.f, 1.f, 1.f});
        }

        worldShader.setMat4("projection",proj);
        worldShader.setMat4("view",view);
        enemyRenderer.draw(worldShader, enemies);

        grapple.drawLine(worldShader, player.position + glm::vec3{0,player.eyeHeight,0}, view, proj);

        projSystem.draw(worldShader, view, proj);

        // Tracers drawn with additive blending — must be after opaque geometry
        renderTracers(view, proj, renderCamPos);

        // Explosion particles — additive GL_POINTS
        renderParticles(view, proj);

        // View model — drawn last inside FBO so it receives bloom; depth cleared inside
        {
            float revolverFill = reloading ? (1.f - reloadTimer / RELOAD_TIME)
                                           : (float)revolverAmmo / (float)revolverAmmoMax;
            worldShader.use();
            worldShader.setVec3("lightDir",    glm::normalize(glm::vec3{0.4f,-1.f,0.3f}));
            worldShader.setVec3("lightColor",  {1.f,0.9f,0.8f});
            worldShader.setVec3("ambientColor",{0.25f,0.25f,0.30f});
            worldShader.setVec3("viewPos",     renderCamPos);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, whiteTex);
            worldShader.setInt("uTexture", 0);
            viewModel.draw(worldShader, renderCam, activeWeapon, revolverFill);
            // Restore world projection/view after viewmodel (postProcess.endScene reads no uniforms)
            worldShader.setMat4("projection", proj);
            worldShader.setMat4("view",       view);
        }

        postProcess.endScene();

        float reloadProgress = reloading ? (1.f - reloadTimer / RELOAD_TIME) : 1.f;
        ui.render(styleSystem, activeWeapon, grenadeCount, grenadeMax,
                  revolverAmmo, revolverAmmoMax,
                  reloading, reloadProgress, nearInteractable);
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
