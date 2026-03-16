#pragma once
#include <glm/glm.hpp>
#include <SDL2/SDL.h>
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

    bool onGround = false;

    // Floor is a hard-coded infinite plane at Y=0.
    // To make a different floor height, change this or add a floor Wall.
    static constexpr float FLOOR_Y = 0.0f;

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
    void update(float dt, const Uint8* keys, const Wall* walls, int wallCount,
                bool grappling = false) {
        handleMovement(dt, keys, grappling);
        applyGravity(dt);
        integrate(dt);
        resolveCollisions(walls, wallCount);
        camera.position = position + glm::vec3{0, eyeHeight, 0};
    }

    // Call this with raw SDL mouse delta before (or after) update().
    // Mouse look is applied every frame, not on the physics tick, so it's
    // always responsive regardless of physics rate.
    void applyMouseLook(float dx, float dy, float sensitivity = 0.1f) {
        camera.applyMouseDelta(dx, dy, sensitivity);
    }

private:
    // -------------------------------------------------------------------------
    // Quake-style movement acceleration.
    //
    // Instead of directly setting velocity = wishDir * speed, we project the
    // current velocity onto wishDir and only add enough to reach max speed.
    // This means:
    //   - Strafing mid-air doesn't kill your existing momentum
    //   - You can gain a tiny bit of extra speed by timing direction changes
    //     (strafe-jumping, same mechanic as Quake/CS/ULTRAKILL)
    //
    // Formula: addSpeed = clamp(maxSpeed - dot(velocity, wishDir), 0, accel*dt)
    //          velocity += wishDir * addSpeed
    // -------------------------------------------------------------------------
    void handleMovement(float dt, const Uint8* keys, bool grappling = false) {
        // Build the desired movement direction from WASD in camera-relative space.
        // We use flatForward (no pitch) so looking up doesn't make you fly.
        glm::vec3 flatFwd   = camera.flatForward();
        glm::vec3 flatRight = glm::normalize(glm::cross(flatFwd, {0, 1, 0}));

        glm::vec3 wishDir{0.f};
        if (keys[SDL_SCANCODE_W]) wishDir += flatFwd;
        if (keys[SDL_SCANCODE_S]) wishDir -= flatFwd;
        if (keys[SDL_SCANCODE_D]) wishDir += flatRight;
        if (keys[SDL_SCANCODE_A]) wishDir -= flatRight;

        // Ground gives full acceleration; air gives much less (you commit to a
        // jump direction but can still steer slightly).
        float accel = onGround ? acceleration : airAcceleration;

        if (glm::length(wishDir) > 0.001f) {
            wishDir = glm::normalize(wishDir);

            // Project horizontal velocity onto wishDir to find how much speed
            // we already have in that direction.
            glm::vec3 hVel{velocity.x, 0.f, velocity.z};
            float currentSpeed = glm::dot(hVel, wishDir);

            // Only add speed up to the maximum — never overshoot it from this call.
            float addSpeed = glm::clamp(horizontalSpeed - currentSpeed, 0.f, accel * dt);
            velocity.x += wishDir.x * addSpeed;
            velocity.z += wishDir.z * addSpeed;
        }

        // Friction: bleed off horizontal speed when on the ground.
        // Skipped while grappling so the hook can build speed freely.
        if (onGround && !grappling) {
            glm::vec3 hVel{velocity.x, 0.f, velocity.z};
            float speed = glm::length(hVel);
            if (speed > 0.001f) {
                float drop     = speed * friction * dt;
                float newSpeed = glm::max(speed - drop, 0.f);
                float scale    = newSpeed / speed;
                velocity.x *= scale;
                velocity.z *= scale;
            }
        }

        // Jump: instant vertical velocity. onGround prevents double-jumping.
        // To add double-jump, track a jumpsRemaining counter here.
        if (keys[SDL_SCANCODE_SPACE] && onGround) {
            velocity.y = jumpForce;
            onGround   = false;
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
    void resolveCollisions(const Wall* walls, int wallCount) {
        // Infinite floor plane at Y = FLOOR_Y
        if (position.y < FLOOR_Y) {
            position.y = FLOOR_Y;
            velocity.y = 0.f;
            onGround   = true;
        } else {
            onGround = false; // re-determined by AABB checks below
        }

        // AABB push-out for each wall. Resolves on the axis of least penetration
        // to avoid getting stuck in corners.
        for (int i = 0; i < wallCount; ++i) {
            resolveAABB(walls[i].box);
        }

        // After wall resolution the player may have landed on top of a box.
        if (position.y <= FLOOR_Y + 0.001f) {
            onGround = true;
        }
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
