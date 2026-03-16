#pragma once
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <vector>
#include "Camera.h"

// =============================================================================
// Collision primitives
// =============================================================================
// AABB = Axis-Aligned Bounding Box. Defined by two corner points.
// All world obstacles are currently represented as AABBs — simple and fast.
// To add a new wall/platform in main.cpp, just add a Wall with an AABB.
struct AABB {
    glm::vec3 min, max; // min = bottom-left-front, max = top-right-back
};

struct Wall {
    AABB      box;
    glm::vec3 color{0.28f, 0.28f, 0.32f};
};

// =============================================================================
// SpatialGrid — uniform 2D grid over the XZ plane for fast wall queries.
//
// Walls are static (except sliding doors). Call build() whenever allWalls
// changes (room cleared, door opened). Query with a bounding box to get
// candidate wall indices — then still test actual AABB overlap yourself.
//
// Grid covers the full two-room level:
//   X: -100 .. 100  (200 m → 17 cells of 12 m)
//   Z: -240 .. 100  (340 m → 29 cells of 12 m)
// =============================================================================
struct SpatialGrid {
    static constexpr float CELL = 12.f;
    static constexpr float X0 = -100.f, Z0 = -240.f;
    static constexpr int   NX = 17, NZ = 29;

    std::vector<int> cells[NX * NZ];

    void build(const std::vector<Wall>& walls) {
        for (auto& c : cells) c.clear();
        for (int i = 0; i < (int)walls.size(); ++i)
            insertWall(i, walls[i].box);
    }

    // Returns candidate wall indices for a query box.
    // May include false positives — caller must still check intersection.
    void query(const AABB& box, std::vector<int>& out) const {
        out.clear();
        int x0 = cx(box.min.x), x1 = cx(box.max.x);
        int z0 = cz(box.min.z), z1 = cz(box.max.z);
        for (int xi = x0; xi <= x1; ++xi)
            for (int zi = z0; zi <= z1; ++zi)
                for (int idx : cell(xi, zi))
                    out.push_back(idx);
    }

private:
    int cx(float x) const { return glm::clamp((int)((x - X0) / CELL), 0, NX-1); }
    int cz(float z) const { return glm::clamp((int)((z - Z0) / CELL), 0, NZ-1); }
    const std::vector<int>& cell(int x, int z) const { return cells[z * NX + x]; }
          std::vector<int>& cell(int x, int z)       { return cells[z * NX + x]; }

    void insertWall(int idx, const AABB& b) {
        for (int xi = cx(b.min.x); xi <= cx(b.max.x); ++xi)
            for (int zi = cz(b.min.z); zi <= cz(b.max.z); ++zi)
                cell(xi, zi).push_back(idx);
    }
};

// =============================================================================
// Player
// =============================================================================
// The player is a kinematic character controller — meaning we manually compute
// velocity, gravity, and collision response rather than relying on a physics
// engine. This gives us tight, predictable feel (similar to Quake/ULTRAKILL).
//
// Coordinate system: Y is up, floor is at Y = 0.
// The player's position is the feet. The camera sits at position + eyeHeight.
//
// Physics runs at a FIXED RATE (60Hz) driven by the accumulator in main.cpp.
// Call update() once per fixed tick. The camera position is then interpolated
// in main.cpp for smooth rendering at any display framerate.
//
// HOW TO EXTEND:
//   - Crouching: lerp eyeHeight and height to smaller values, shrink radius
//   - Dashing: add a dashVelocity vec3, apply it for a short duration in
//     handleMovement, then decay it with its own friction factor
//   - Double jump: track jumpsRemaining (reset to 2 on landing), decrement on jump
//   - Slide: on crouch while sprinting, add a burst in flatForward direction
// =============================================================================

class Player {
public:
    glm::vec3 position;  // feet position in world space
    glm::vec3 velocity;  // meters per second, all three axes

    // -------------------------------------------------------------------------
    // Tuning knobs — adjust these to change how movement feels
    // -------------------------------------------------------------------------
    float horizontalSpeed = 7.0f;    // max ground speed (m/s)
    float gravity         = -24.0f;  // downward acceleration (m/s^2) — stronger than real for snappier jumps
    float jumpForce       = 8.5f;    // initial vertical velocity on jump
    float acceleration    = 80.0f;   // how fast you reach max speed on the ground
    float airAcceleration = 42.0f;   // responsive air control — lets you steer momentum mid-air
    float friction        = 10.0f;   // how fast horizontal speed bleeds off when grounded

    float eyeHeight = 1.7f;  // camera Y offset above feet (meters)
    float radius    = 0.4f;  // half-width of player AABB (X and Z)
    float height    = 1.8f;  // full height of player AABB

    bool  onGround = false;

    // Slide state — activated by crouching while moving fast on the ground.
    bool  sliding  = false;
    float slideTimer = 0.f;

    // Coyote time: allows jumping for a brief window after walking off a ledge.
    float coyoteTimer = 0.f;

    // Jump buffering: if jump is pressed while airborne, store it so it fires
    // the moment the player lands (up to JUMP_BUFFER_DURATION seconds later).
    float jumpBufferTimer = 0.f;

    static constexpr float FLOOR_Y              = 0.0f;
    static constexpr float SLIDE_DURATION       = 0.55f;
    static constexpr float SLIDE_SPEED          = 14.f;
    static constexpr float SLIDE_FRICTION       = 2.5f;
    static constexpr float NORMAL_EYE_H         = 1.7f;
    static constexpr float SLIDE_EYE_H          = 0.65f;
    static constexpr float NORMAL_HEIGHT        = 1.8f;
    static constexpr float SLIDE_HEIGHT_VAL     = 0.9f;
    static constexpr float COYOTE_DURATION      = 0.10f; // seconds after leaving a ledge
    static constexpr float JUMP_BUFFER_DURATION = 0.10f; // seconds to buffer a jump input

    // The camera is owned by the player. It tracks position + eyeHeight and
    // shares the same yaw/pitch set by mouse input.
    Camera camera;

    Player(glm::vec3 startPos = {0.f, 0.f, 0.f})
        : position(startPos), velocity(0.f),
          camera(startPos + glm::vec3{0, eyeHeight, 0})
    {}

    // -------------------------------------------------------------------------
    // Call this once per fixed physics tick (e.g., every 1/60 seconds).
    // walls[] is the list of solid AABB obstacles in the world.
    // -------------------------------------------------------------------------
    // grappling=true skips ground friction so the grapple can build speed freely.
    // grid (optional): if non-null, used to skip walls that are far away.
    void update(float dt, const Uint8* keys, const Wall* walls, int wallCount,
                bool grappling = false, const SpatialGrid* grid = nullptr) {
        handleMovement(dt, keys, grappling);
        applyGravity(dt);
        integrate(dt);
        resolveCollisions(walls, wallCount, grid);
        camera.position = position + glm::vec3{0, eyeHeight, 0};
    }

    // Call this with raw SDL mouse delta before (or after) update().
    // Mouse look is applied every frame, not on the physics tick, so it's
    // always responsive regardless of physics rate.
    void applyMouseLook(float dx, float dy, float sensitivity = 0.1f) {
        camera.applyMouseDelta(dx, dy, sensitivity);
    }

private:
    bool prevGroundForCoyote = false; // tracks last-tick ground state for coyote detection

    void handleMovement(float dt, const Uint8* keys, bool grappling = false) {
        glm::vec3 flatFwd   = camera.flatForward();
        glm::vec3 flatRight = glm::normalize(glm::cross(flatFwd, {0, 1, 0}));

        // ---- Coyote time ----
        // If we were grounded last tick but aren't now, start the coyote window.
        // This lets the player jump for a brief moment after walking off a ledge.
        if (!onGround) {
            if (prevGroundForCoyote) {
                coyoteTimer = COYOTE_DURATION; // just left solid ground
            } else {
                coyoteTimer = glm::max(0.f, coyoteTimer - dt);
            }
        } else {
            coyoteTimer = 0.f; // grounded — no coyote needed
        }
        prevGroundForCoyote = onGround;

        // ---- Jump buffer ----
        // If jump is pressed while airborne (and coyote window isn't active),
        // store it so the next landing executes it automatically.
        bool jumpKey = keys[SDL_SCANCODE_SPACE] != 0;
        if (jumpKey && !onGround && coyoteTimer <= 0.f) {
            jumpBufferTimer = JUMP_BUFFER_DURATION;
        } else if (jumpBufferTimer > 0.f) {
            jumpBufferTimer = glm::max(0.f, jumpBufferTimer - dt);
        }

        bool crouchKey = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_C];

        // ---- Slide state machine ----
        glm::vec3 hVelFlat{velocity.x, 0.f, velocity.z};
        float flatSpeed = glm::length(hVelFlat);

        if (!sliding) {
            // Activate slide: crouch while grounded and moving fast enough.
            if (crouchKey && onGround && flatSpeed > horizontalSpeed * 0.6f) {
                sliding    = true;
                slideTimer = SLIDE_DURATION;
                // Boost in current travel direction; at least SLIDE_SPEED.
                glm::vec3 slideDir = glm::normalize(hVelFlat);
                float boostSpd     = glm::max(flatSpeed, SLIDE_SPEED);
                velocity.x = slideDir.x * boostSpd;
                velocity.z = slideDir.z * boostSpd;
                eyeHeight  = SLIDE_EYE_H;
                height     = SLIDE_HEIGHT_VAL;
            }
        } else {
            slideTimer -= dt;
            // End slide when: timer runs out, crouch released, or left the ground.
            if (slideTimer <= 0.f || !crouchKey || !onGround) {
                sliding   = false;
                eyeHeight = NORMAL_EYE_H;
                height    = NORMAL_HEIGHT;
            }
        }

        // WASD acceleration — suppressed while sliding so momentum carries.
        if (!sliding) {
            glm::vec3 wishDir{0.f};
            if (keys[SDL_SCANCODE_W]) wishDir += flatFwd;
            if (keys[SDL_SCANCODE_S]) wishDir -= flatFwd;
            if (keys[SDL_SCANCODE_D]) wishDir += flatRight;
            if (keys[SDL_SCANCODE_A]) wishDir -= flatRight;

            float accel = onGround ? acceleration : airAcceleration;

            if (glm::length(wishDir) > 0.001f) {
                wishDir = glm::normalize(wishDir);
                glm::vec3 hVel{velocity.x, 0.f, velocity.z};
                float currentSpeed = glm::dot(hVel, wishDir);
                float addSpeed = glm::clamp(horizontalSpeed - currentSpeed, 0.f, accel * dt);
                velocity.x += wishDir.x * addSpeed;
                velocity.z += wishDir.z * addSpeed;
            }
        }

        // Friction: use low slide friction during a slide, normal otherwise.
        // Skipped while grappling so the hook can build speed freely.
        if (onGround && !grappling) {
            float fr = sliding ? SLIDE_FRICTION : friction;
            glm::vec3 hVel{velocity.x, 0.f, velocity.z};
            float speed = glm::length(hVel);
            if (speed > 0.001f) {
                float drop     = speed * fr * dt;
                float newSpeed = glm::max(speed - drop, 0.f);
                float scale    = newSpeed / speed;
                velocity.x *= scale;
                velocity.z *= scale;
            }
        }

        // Jump: ground jump, coyote jump, or buffered jump.
        // All three preserve horizontal velocity (slide-jump works naturally).
        bool canGroundJump = onGround || coyoteTimer > 0.f;

        // Buffered jump fires on landing if the buffer window is still open.
        if (onGround && jumpBufferTimer > 0.f && !jumpKey) {
            velocity.y      = jumpForce;
            onGround        = false;
            jumpBufferTimer = 0.f;
            coyoteTimer     = 0.f;
            if (sliding) { sliding = false; eyeHeight = NORMAL_EYE_H; height = NORMAL_HEIGHT; }
        }
        // Direct jump (including coyote window).
        else if (jumpKey && canGroundJump) {
            velocity.y      = jumpForce;
            onGround        = false;
            coyoteTimer     = 0.f;
            jumpBufferTimer = 0.f;
            if (sliding) { sliding = false; eyeHeight = NORMAL_EYE_H; height = NORMAL_HEIGHT; }
        }
    }

    // Constant downward acceleration. Not applied when standing on the ground
    // to prevent slowly accumulating negative Y velocity while idle.
    void applyGravity(float dt) {
        if (!onGround) {
            velocity.y += gravity * dt;
        }
    }

    // Euler integration: move position by velocity scaled by the timestep.
    // Works well at fixed 60Hz. If you ever want sub-step accuracy, use
    // Verlet integration here instead.
    void integrate(float dt) {
        position += velocity * dt;
    }

    // -------------------------------------------------------------------------
    // Collision resolution: push the player out of any overlapping geometry.
    // Order: check floor first, then walls (so standing on top of a box works),
    // then re-check floor in case a wall pushed us below it.
    // -------------------------------------------------------------------------
    void resolveCollisions(const Wall* walls, int wallCount,
                           const SpatialGrid* grid = nullptr) {
        if (position.y < FLOOR_Y) {
            position.y = FLOOR_Y;
            velocity.y = 0.f;
            onGround   = true;
        } else {
            onGround = false;
        }

        if (grid) {
            // Grid path: only test walls in nearby cells (~4–9 cells vs all walls).
            static std::vector<int> candidates;
            AABB pb{ position + glm::vec3{-radius-0.1f, -0.1f, -radius-0.1f},
                     position + glm::vec3{ radius+0.1f,  height+0.1f, radius+0.1f} };
            grid->query(pb, candidates);
            for (int idx : candidates) resolveAABB(walls[idx].box);
        } else {
            for (int i = 0; i < wallCount; ++i) resolveAABB(walls[i].box);
        }

        if (position.y <= FLOOR_Y + 0.001f) onGround = true;
    }

    // Push the player out of a single AABB wall.
    // The player is also treated as an AABB (a capsule would be smoother in
    // corners, but AABB is cheaper and plenty good for flat walls).
    void resolveAABB(const AABB& wall) {
        glm::vec3 pMin = position + glm::vec3{-radius, 0.f,    -radius};
        glm::vec3 pMax = position + glm::vec3{ radius, height,  radius};

        if (pMax.x <= wall.min.x || pMin.x >= wall.max.x) return;
        if (pMax.y <= wall.min.y || pMin.y >= wall.max.y) return;
        if (pMax.z <= wall.min.z || pMin.z >= wall.max.z) return;

        float ox = glm::min(pMax.x - wall.min.x, wall.max.x - pMin.x);
        float oy = glm::min(pMax.y - wall.min.y, wall.max.y - pMin.y);
        float oz = glm::min(pMax.z - wall.min.z, wall.max.z - pMin.z);

        if (ox <= oy && ox <= oz) {
            // Horizontal push — only zero the velocity component moving INTO the wall
            // so grapple / dash momentum along the wall face is preserved.
            float pushDir = (position.x < (wall.min.x + wall.max.x) * 0.5f) ? -1.f : 1.f;
            position.x += pushDir * ox;
            if (velocity.x * pushDir < 0.f) velocity.x = 0.f;
        } else if (oy <= ox && oy <= oz) {
            float pushDir = (position.y < (wall.min.y + wall.max.y) * 0.5f) ? -1.f : 1.f;
            position.y += pushDir * oy;
            if (pushDir > 0.f) {
                // Pushed UP  → feet landed on the top surface of a box
                if (velocity.y < 0.f) velocity.y = 0.f;
                onGround = true;
            } else {
                // Pushed DOWN → head hit the underside of a box (ceiling)
                // Only kill upward velocity; do NOT set onGround.
                if (velocity.y > 0.f) velocity.y = 0.f;
            }
        } else {
            float pushDir = (position.z < (wall.min.z + wall.max.z) * 0.5f) ? -1.f : 1.f;
            position.z += pushDir * oz;
            if (velocity.z * pushDir < 0.f) velocity.z = 0.f;
        }
    }
};
