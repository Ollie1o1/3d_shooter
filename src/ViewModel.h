#pragma once
// =============================================================================
// ViewModel.h — First-person weapon model + animation system.
//
// Draws a simple geometric revolver or grenade in the lower-right of screen.
// Uses its own projection (FOV 65°, near 0.03) so it never clips into walls.
// Depth is cleared before drawing so it always renders on top of the world.
//
// ANIMATIONS:
//   triggerFire()    — quick upward kick and settle
//   triggerReload()  — gun swings down and back up
//   triggerGrenade() — arm swings forward and back
//   triggerGrapple() — short forward push
//
// CAMERA BOB:
//   getBobOffset() returns a world-space position delta based on player speed.
//   Add this to renderCam.position in GameplayState::render().
//
// HOW TO ADD A NEW WEAPON:
//   1. Add a draw___() method with box calls defining the shape.
//   2. Call it from draw() based on activeWeapon index.
// =============================================================================
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "ShaderProgram.h"
#include "Mesh.h"
#include <vector>
#include <cmath>
#include <algorithm>

enum class ViewAnim { IDLE, FIRE, RELOAD, GRENADE_THROW, GRAPPLE_FIRE, PUMP, WEAPON_SWITCH };

class ViewModel {
public:
    ViewAnim anim      = ViewAnim::IDLE;
    float    animTimer = 0.f;
    float    animMax   = 0.001f;  // prevent div-by-zero
    float    bobTimer  = 0.f;

    Mesh cubeMesh;  // unit cube (-0.5..0.5), reused for every box in the gun

    // Base offset of gun in camera local space (right, up, forward)
    static constexpr float GUN_R =  0.21f;
    static constexpr float GUN_U = -0.18f;
    static constexpr float GUN_F =  0.32f;

    ViewModel() { buildCube(); }

    // --- Animation triggers --------------------------------------------------
    void triggerFire()    { anim = ViewAnim::FIRE;          animTimer = animMax = 0.14f; }
    void triggerReload()  { anim = ViewAnim::RELOAD;        animTimer = animMax = 0.60f; }
    void triggerGrenade() { anim = ViewAnim::GRENADE_THROW; animTimer = animMax = 0.50f; }
    void triggerGrapple() { anim = ViewAnim::GRAPPLE_FIRE;  animTimer = animMax = 0.28f; }
    void triggerPump()    { anim = ViewAnim::PUMP;          animTimer = animMax = 0.45f; }
    void triggerSwitch()  { anim = ViewAnim::WEAPON_SWITCH; animTimer = animMax = 0.30f; }

    // -------------------------------------------------------------------------
    void update(float dt, float xzSpeed, bool /*onGround*/) {
        animTimer = std::max(0.f, animTimer - dt);
        if (animTimer <= 0.f) anim = ViewAnim::IDLE;

        // Bob advances continuously — speed controls the stride frequency
        bobTimer += dt * (xzSpeed > 0.5f ? xzSpeed * 0.38f : 0.8f);
    }

    // Returns a world-space positional nudge for camera head-bob.
    // Add this to renderCam.position before building the view matrix.
    glm::vec3 getBobOffset(float xzSpeed, bool onGround,
                           const glm::vec3& camRight) const {
        if (!onGround || xzSpeed < 0.3f) {
            // Gentle idle sway
            return camRight * (sinf(bobTimer * 1.1f) * 0.003f)
                 + glm::vec3{0, sinf(bobTimer * 0.65f) * 0.002f, 0};
        }
        float amp = std::min(xzSpeed / 9.f, 1.f) * 0.016f;
        return camRight * (sinf(bobTimer * 2.f) * amp * 0.4f)
             + glm::vec3{0, fabsf(sinf(bobTimer)) * amp, 0};
    }

    // -------------------------------------------------------------------------
    // Draw the view model.  Call this after clearing depth (inside the scene FBO)
    // so the gun renders in front of all world geometry.
    //
    // revolverFill  — 0 = just fired / cooldown, 1 = fully ready
    // -------------------------------------------------------------------------
    void draw(ShaderProgram& shader, const Camera& cam,
              int activeWeapon, float revolverFill, float shotgunFill = 1.f) {

        // Compute animation offsets in gun-local space
        float p = (animMax > 0.f) ? (animTimer / animMax) : 0.f;
        float kickZ = 0.f, kickY = 0.f, kickX = 0.f, roll = 0.f;

        switch (anim) {
            case ViewAnim::FIRE:
                // p=1 at peak (just fired), decays to 0
                kickZ = -p * 0.045f;
                kickY =  p * 0.028f;
                break;
            case ViewAnim::RELOAD:
                // First half: swing down; second half: swing up
                if (p > 0.5f) {
                    float t = (p - 0.5f) * 2.f;
                    kickY    = -t * 0.13f;
                    roll     =  t * 14.f;
                } else {
                    float t = p * 2.f;
                    kickY    = -t * 0.13f;
                    roll     =  t * 14.f;
                }
                break;
            case ViewAnim::GRENADE_THROW:
                kickZ =  p * 0.07f;
                kickX = -p * 0.05f;
                kickY =  sinf(p * 3.14159f) * 0.045f;
                break;
            case ViewAnim::GRAPPLE_FIRE:
                kickZ = p * 0.06f;
                break;
            case ViewAnim::PUMP:
                // p goes 1→0; first half: pump slides back, second half: slides forward
                if (p > 0.5f) {
                    float t = (p - 0.5f) * 2.f;
                    kickZ = -t * 0.04f;
                    kickY = -t * 0.015f;
                } else {
                    float t = p * 2.f;
                    kickZ =  t * 0.02f;
                    kickY = -t * 0.01f;
                }
                break;
            case ViewAnim::WEAPON_SWITCH:
                // p goes 1→0; first half: weapon drops down, second half: rises up
                if (p > 0.5f) {
                    float t = (p - 0.5f) * 2.f;
                    kickY = -t * 0.18f;
                } else {
                    float t = p * 2.f;
                    kickY = -(1.f - t) * 0.18f;
                }
                break;
            default: break;
        }

        // Build camera basis vectors
        glm::vec3 fwd   = cam.forward();
        glm::vec3 right = cam.right();
        glm::vec3 up    = glm::normalize(glm::cross(right, fwd));

        // Gun base position with animation offset applied
        glm::vec3 gunPos = cam.position
                         + right * (GUN_R + kickX)
                         + up    * (GUN_U + kickY)
                         + fwd   * (GUN_F + kickZ);

        // Gun-to-world matrix: local +X=right, +Y=up, +Z=fwd (barrel forward)
        glm::mat4 gunBase(1.f);
        gunBase[0] = glm::vec4(right, 0.f);
        gunBase[1] = glm::vec4(up,    0.f);
        gunBase[2] = glm::vec4(fwd,   0.f);
        gunBase[3] = glm::vec4(gunPos, 1.f);

        // Optional roll for reload animation
        if (roll != 0.f) {
            glm::mat4 rollMat = glm::rotate(glm::mat4(1.f),
                                            glm::radians(roll),
                                            glm::vec3{0.f, 0.f, 1.f});
            gunBase = gunBase * rollMat;
        }

        // Narrow FOV projection (prevents gun from clipping into walls)
        glm::mat4 vmProj = glm::perspective(
            glm::radians(65.f), cam.aspectRatio, 0.03f, 10.f);

        // Clear depth — gun always draws on top
        glClear(GL_DEPTH_BUFFER_BIT);

        shader.setMat4("projection", vmProj);
        shader.setMat4("view",       cam.viewMatrix());
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});

        glDisable(GL_CULL_FACE);

        if (activeWeapon == 0)
            drawRevolver(shader, gunBase, revolverFill);
        else if (activeWeapon == 1)
            drawShotgun(shader, gunBase, shotgunFill);
        // grenade only shown during GRENADE_THROW animation (handled by triggerGrenade)

        glEnable(GL_CULL_FACE);
    }

private:
    // ---- Gun geometry -------------------------------------------------------
    // All positions in gun-local space where +Z = barrel direction (world fwd).

    void drawRevolver(ShaderProgram& shader, const glm::mat4& base, float fill) {
        glm::vec3 metal  = {0.50f, 0.46f, 0.42f};  // gunmetal silver
        glm::vec3 dark   = {0.25f, 0.23f, 0.22f};  // dark frame
        glm::vec3 wood   = {0.40f, 0.25f, 0.10f};  // grip wood

        // Barrel — long thin along +Z
        drawBox(shader, base, {0.f,  0.022f,  0.14f}, {0.036f, 0.036f, 0.28f}, metal);
        // Receiver / frame
        drawBox(shader, base, {0.f,  -0.012f, -0.02f}, {0.062f, 0.080f, 0.16f}, dark);
        // Cylinder (ammo wheel) — sits on the right side of frame
        drawBox(shader, base, {0.040f, 0.008f, 0.042f}, {0.052f, 0.058f, 0.076f}, metal);
        // Grip
        drawBox(shader, base, {0.f, -0.130f, -0.075f}, {0.052f, 0.130f, 0.068f}, wood);
        // Hammer (top rear)
        drawBox(shader, base, {0.f,  0.058f, -0.098f}, {0.020f, 0.036f, 0.026f}, dark);
        // Trigger guard
        drawBox(shader, base, {0.f, -0.052f, -0.030f}, {0.020f, 0.022f, 0.080f}, dark);

        // Muzzle flash — brief glow right after firing (fill near 0)
        if (fill < 0.25f) {
            float glow = (0.25f - fill) / 0.25f;
            shader.setVec3("emissiveColor", glm::vec3{1.f, 0.8f, 0.3f} * glow * 0.9f);
            drawBox(shader, base, {0.f, 0.022f, 0.295f}, {0.022f, 0.022f, 0.028f},
                    glm::mix(metal, glm::vec3{1.f, 0.9f, 0.5f}, glow));
            shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        }
    }

    void drawGrenade(ShaderProgram& shader, const glm::mat4& base) {
        glm::vec3 body  = {0.22f, 0.52f, 0.12f};  // dark OD green
        glm::vec3 band  = {0.35f, 0.30f, 0.20f};  // brass/khaki bands
        glm::vec3 metal = {0.32f, 0.32f, 0.32f};  // pin/lever

        // Main body — roughly oval (stack of boxes)
        drawBox(shader, base, {0.f,  0.00f,  0.04f}, {0.075f, 0.095f, 0.145f}, body);
        drawBox(shader, base, {0.f,  0.00f,  0.04f}, {0.068f, 0.110f, 0.100f}, body);
        // Segmentation bands
        drawBox(shader, base, {0.f,  0.00f,  0.065f}, {0.077f, 0.100f, 0.012f}, band);
        drawBox(shader, base, {0.f,  0.00f,  0.005f}, {0.077f, 0.100f, 0.012f}, band);
        // Cap / fuze top
        drawBox(shader, base, {0.f,  0.00f,  0.175f}, {0.040f, 0.040f, 0.048f}, metal);
        // Lever
        drawBox(shader, base, {0.024f, 0.048f, 0.120f}, {0.010f, 0.030f, 0.090f}, metal);
        // Grip
        drawBox(shader, base, {0.f, -0.110f, -0.042f}, {0.048f, 0.100f, 0.055f}, metal);
    }

    void drawShotgun(ShaderProgram& shader, const glm::mat4& base, float fill) {
        glm::vec3 metal  = {0.35f, 0.33f, 0.30f};  // dark gunmetal
        glm::vec3 dark   = {0.20f, 0.18f, 0.17f};  // dark frame
        glm::vec3 wood   = {0.45f, 0.28f, 0.10f};  // pump grip / stock wood

        // Barrel — thick and long along +Z
        drawBox(shader, base, {0.f,  0.028f,  0.12f}, {0.048f, 0.048f, 0.32f}, metal);
        // Magazine tube — under the barrel
        drawBox(shader, base, {0.f, -0.018f,  0.14f}, {0.032f, 0.032f, 0.26f}, metal);
        // Receiver body
        drawBox(shader, base, {0.f,  0.005f, -0.04f}, {0.065f, 0.075f, 0.14f}, dark);
        // Pump forearm — slides during pump animation
        float pumpSlide = 0.f;
        if (anim == ViewAnim::PUMP) {
            float p = (animMax > 0.f) ? (animTimer / animMax) : 0.f;
            if (p > 0.5f) pumpSlide = -(p - 0.5f) * 2.f * 0.08f;
            else          pumpSlide =  p * 2.f * 0.04f - 0.04f;
        }
        drawBox(shader, base, {0.f, -0.005f, 0.08f + pumpSlide}, {0.050f, 0.044f, 0.10f}, wood);
        // Stock
        drawBox(shader, base, {0.f, -0.035f, -0.16f}, {0.048f, 0.065f, 0.10f}, wood);
        drawBox(shader, base, {0.f, -0.060f, -0.22f}, {0.040f, 0.055f, 0.06f}, wood);
        // Trigger guard
        drawBox(shader, base, {0.f, -0.050f, -0.04f}, {0.020f, 0.022f, 0.070f}, dark);
        // Grip
        drawBox(shader, base, {0.f, -0.120f, -0.08f}, {0.044f, 0.100f, 0.055f}, wood);

        // Muzzle flash — wider than revolver
        if (fill < 0.20f) {
            float glow = (0.20f - fill) / 0.20f;
            shader.setVec3("emissiveColor", glm::vec3{1.f, 0.7f, 0.2f} * glow * 1.2f);
            drawBox(shader, base, {0.f, 0.028f, 0.30f}, {0.035f, 0.035f, 0.04f},
                    glm::mix(metal, glm::vec3{1.f, 0.85f, 0.4f}, glow));
            shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        }
    }

    // Draw a box centered at localCenter with given full extents, in gun space.
    void drawBox(ShaderProgram& shader, const glm::mat4& base,
                 glm::vec3 localCenter, glm::vec3 size, glm::vec3 color) {
        glm::mat4 model = base
            * glm::translate(glm::mat4(1.f), localCenter)
            * glm::scale(glm::mat4(1.f), size);
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
        cubeMesh.draw();
    }

    // Build a unit cube (-0.5..0.5) using the full Vertex layout.
    // Each face gets proper 0..1 UVs so the shader's edge-darkening works.
    void buildCube() {
        std::vector<Vertex>       verts;
        std::vector<unsigned int> idx;

        // Helper: push a quad face with CCW winding from outside (normal direction)
        auto face = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n) {
            unsigned int base = (unsigned int)verts.size();
            verts.push_back({a, {0.f, 0.f}, n, {1.f,1.f,1.f}});
            verts.push_back({b, {1.f, 0.f}, n, {1.f,1.f,1.f}});
            verts.push_back({c, {1.f, 1.f}, n, {1.f,1.f,1.f}});
            verts.push_back({d, {0.f, 1.f}, n, {1.f,1.f,1.f}});
            idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
        };

        // +Z face
        face({-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}, {0,0,1});
        // -Z face
        face({.5f,-.5f,-.5f},{-.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f}, {0,0,-1});
        // +X face
        face({.5f,-.5f,.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{.5f,.5f,.5f}, {1,0,0});
        // -X face
        face({-.5f,-.5f,-.5f},{-.5f,-.5f,.5f},{-.5f,.5f,.5f},{-.5f,.5f,-.5f}, {-1,0,0});
        // +Y face
        face({-.5f,.5f,.5f},{.5f,.5f,.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f}, {0,1,0});
        // -Y face
        face({-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f},{-.5f,-.5f,.5f}, {0,-1,0});

        cubeMesh.upload(verts, idx);
    }
};
