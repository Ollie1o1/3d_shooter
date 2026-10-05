#pragma once
// =============================================================================
// ViewModel.h — First-person weapon model + animation system.
//
// Draws the held gun (revolver, shotgun, Kar98, Longshot) from boxes.
// Uses its own projection (FOV 65°, near 0.03) so it never clips into walls.
// Depth is cleared before drawing so it always renders on top of the world.
//
// ANIMATIONS:
//   triggerFire()    — quick upward kick and settle
//   triggerReload()  — revolver: cylinder out, brass out, speedloader in,
//                      flick shut. Shotgun: roll it over, thumb in each
//                      missing shell, rack the pump. Kar98: tip it over,
//                      bolt up and back (the empty flies), a stripper clip
//                      into the guide, thumb the rounds down, flick the
//                      clip away, bolt home. Longshot: swing down and up.
//   triggerGrenade() — arm swings forward and back
//   triggerGrapple() — short forward push
//   triggerBolt()    — rifles: bolt up, back, forward, down
//
// AIMING: draw() takes an aim amount 0..1. The rifles slide from the hip to
// the centre of the screen: the Kar98 lines its rear notch and front post up
// on the crosshair; the Longshot brings its scope to your eye (the HUD then
// draws the scope view and the model is hidden).
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

enum class ViewAnim { IDLE, FIRE, RELOAD, GRENADE_THROW, GRAPPLE_FIRE, PUMP, WEAPON_SWITCH, BOLT, INSPECT };

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
    void triggerFire()    { anim = ViewAnim::FIRE; animTimer = animMax = 0.14f; heat = std::min(1.f, heat + 0.22f); cylSpin = 45.f; }
    // Reloads play over the whole reload time; shells: how many the shotgun loads
    void triggerReload(float t = 0.6f, int shells = 1) {
        anim = ViewAnim::RELOAD; animTimer = animMax = std::min(t, 2.f); reloadShells = std::max(1, std::min(shells, 5));
    }
    void triggerBolt(float t)          { anim = ViewAnim::BOLT;   animTimer = animMax = std::max(t, 0.2f); }
    // Punch / parry: a fist drives forward from the lower left. It runs
    // alongside the gun's own animation, so you can punch mid-reload.
    void triggerParry(bool hit) { parryTimer = PARRY_TIME; parryHit = hit; }
    static constexpr float PARRY_TIME = 0.3f;
    float parryTimer = 0.f;
    bool  parryHit = false;
    int   reloadShells = 1;     // shotgun: shells to feed this reload; Kar98: rounds in the clip
    void triggerGrenade() { anim = ViewAnim::GRENADE_THROW; animTimer = animMax = 0.50f; }
    void triggerGrapple() { anim = ViewAnim::GRAPPLE_FIRE;  animTimer = animMax = 0.28f; }
    void triggerPump()    { anim = ViewAnim::PUMP;          animTimer = animMax = 0.45f; }
    void triggerSwitch()  { anim = ViewAnim::WEAPON_SWITCH; animTimer = animMax = 0.30f; }
    // V: turn the gun over to admire it (only from idle)
    void triggerInspect() { if (anim == ViewAnim::IDLE) { anim = ViewAnim::INSPECT; animTimer = animMax = 2.2f; } }

    // -------------------------------------------------------------------------
    void update(float dt, float xzSpeed, bool /*onGround*/) {
        animTimer = std::max(0.f, animTimer - dt);
        parryTimer = std::max(0.f, parryTimer - dt);
        heat    = std::max(0.f, heat - dt * 0.7f);
        cylSpin *= std::exp(-dt * 22.f);          // the cylinder clicks round to the next chamber
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
    // flash — muzzle flash 0..1 (1 the instant a shot leaves)
    // aim   — 0 at the hip .. 1 fully aimed (rifles only)
    // -------------------------------------------------------------------------
    // ammo / mag: the revolver's chambers light up for the rounds left
    void draw(ShaderProgram& shader, const Camera& cam, int activeWeapon, float flash, float aim = 0.f,
              int ammo = 8, int mag = 8) {
        ammoNow = ammo; magNow = std::max(1, mag);

        // Compute animation offsets in gun-local space
        float p = (animMax > 0.f) ? (animTimer / animMax) : 0.f;
        float kickZ = 0.f, kickY = 0.f, kickX = 0.f, roll = 0.f, pitchUp = 0.f, reloadYaw = 0.f;
        boltLift = 0.f; boltPull = 0.f;
        reloadU = anim == ViewAnim::RELOAD ? 1.f - p : -1.f;

        switch (anim) {
            case ViewAnim::FIRE:
                // p=1 at peak (just fired), decays to 0
                kickZ = -p * (activeWeapon >= 2 ? 0.09f : 0.045f);
                kickY =  p * 0.028f;
                pitchUp = p * (activeWeapon == 3 ? 9.f : activeWeapon == 2 ? 6.f : 0.f);
                break;
            case ViewAnim::RELOAD: {
                float u = 1.f - p;
                if (activeWeapon == 0) {
                    // Tip the revolver over to the left and up for the swing-out,
                    // muzzle up while the brass drops, back down for the loader,
                    // then a flick back to the right as the cylinder snaps home
                    float tilt = smooth(seg(u, 0.f, 0.12f)) - smooth(seg(u, 0.78f, 0.95f));
                    float eject = std::sin(seg(u, 0.12f, 0.34f) * 3.14159f);
                    float flick = std::sin(seg(u, 0.74f, 0.9f) * 3.14159f);
                    roll    = -tilt * 32.f + flick * 14.f;
                    pitchUp = eject * 28.f + tilt * 6.f;
                    kickX   = -tilt * 0.07f;
                    kickY   =  tilt * 0.035f - eject * 0.02f;
                    kickZ   = -tilt * 0.02f;
                } else if (activeWeapon == 1) {
                    // Roll the shotgun onto its side to bare the loading port,
                    // feed the shells, roll back and rack it
                    float tilt = smooth(seg(u, 0.f, 0.12f)) - smooth(seg(u, 0.72f, 0.84f));
                    roll  = -tilt * 55.f;
                    kickY = tilt * 0.06f;
                    kickX = -tilt * 0.07f;
                    pitchUp = tilt * 10.f;
                    float rack = std::sin(seg(u, 0.84f, 1.f) * 3.14159f);
                    kickZ = -rack * 0.02f;
                } else if (activeWeapon == 2) {
                    // Kar98: tip it so the action faces you (right side up),
                    // dip as the thumb strips the rounds down, settle back
                    float tilt = smooth(seg(u, 0.f, 0.1f)) - smooth(seg(u, 0.86f, 1.f));
                    float press = std::sin(seg(u, KAR_PRESS0, KAR_PRESS1) * 3.14159f);
                    float seat  = std::sin(seg(u, KAR_CLIP0, KAR_CLIP1) * 3.14159f);
                    // and bring it in toward the middle, muzzle turned
                    // inward, so the open action sits clear of the HUD
                    roll    = tilt * 12.f;
                    pitchUp = tilt * 8.f - press * 3.f;
                    reloadYaw = -tilt * 16.f;
                    kickX   = -tilt * 0.09f;
                    kickY   = tilt * 0.045f - press * 0.018f - seat * 0.006f;
                    kickZ   = tilt * 0.07f;
                    // The bolt: lifted and drawn back early, home again at the end
                    boltLift = seg(u, 0.1f, 0.15f) - seg(u, 0.8f, 0.86f);
                    boltPull = seg(u, 0.15f, 0.23f) - seg(u, 0.72f, 0.8f);
                } else {
                    // Longshot: swing down and roll, then back up
                    float t = p > 0.5f ? (1.f - p) * 2.f : p * 2.f;
                    t = std::min(1.f, t * 1.6f);
                    kickY = -t * 0.13f;
                    roll  =  t * 14.f;
                }
                break;
            }
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
            case ViewAnim::BOLT: {
                // 1→0: lift the handle, pull back, push forward, lower it.
                // The gun dips and rolls a little toward the working hand.
                float u = 1.f - p;
                auto seg = [](float u, float a, float b) { return std::clamp((u - a) / (b - a), 0.f, 1.f); };
                boltLift = seg(u, 0.f, 0.2f) - seg(u, 0.8f, 1.f);
                boltPull = seg(u, 0.2f, 0.45f) - seg(u, 0.55f, 0.8f);
                float w = std::sin(u * 3.14159f);
                roll  = w * 9.f;
                kickY = -w * 0.02f;
                break;
            }
            default: break;
        }
        // Inspect: bring the gun in and turn its side to you, tilt it, put it back
        float inspectYaw = 0.f;
        if (anim == ViewAnim::INSPECT) {
            float u = 1.f - p;
            float k = smooth(seg(u, 0.f, 0.22f)) * (1.f - smooth(seg(u, 0.8f, 1.f)));
            float tilt = std::sin(seg(u, 0.3f, 0.75f) * 3.14159f);
            inspectYaw = k * 62.f;
            roll = -tilt * 25.f * k;
            pitchUp = k * 6.f + tilt * 10.f;
            kickX = -k * 0.11f;
            kickY = k * 0.06f;
            kickZ = k * 0.05f;
        }

        // Build camera basis vectors
        glm::vec3 fwd   = cam.forward();
        glm::vec3 right = cam.right();
        glm::vec3 up    = glm::normalize(glm::cross(right, fwd));

        // Hip position, blended toward the aimed position for the rifles
        float a = aim * aim * (3.f - 2.f * aim);
        glm::vec3 hip{GUN_R, GUN_U, GUN_F};
        if (activeWeapon == 0) hip = {0.15f, -0.125f, 0.27f};   // the revolver sits higher, cylinder in view
        glm::vec3 aimed = activeWeapon == 2 ? glm::vec3{0.f, -KAR_SIGHT_Y, 0.30f}
                        : activeWeapon == 3 ? glm::vec3{0.f, -LONG_SCOPE_Y, 0.05f}
                        : hip;
        glm::vec3 off = glm::mix(hip, aimed, a);
        float kickScale = 1.f - 0.6f * a;
        glm::vec3 gunPos = cam.position
                         + right * (off.x + kickX * kickScale)
                         + up    * (off.y + kickY * kickScale)
                         + fwd   * (off.z + kickZ * kickScale);

        // Gun-to-world matrix: local +X=right, +Y=up, +Z=fwd (barrel forward)
        glm::mat4 gunBase(1.f);
        gunBase[0] = glm::vec4(right, 0.f);
        gunBase[1] = glm::vec4(up,    0.f);
        gunBase[2] = glm::vec4(fwd,   0.f);
        gunBase[3] = glm::vec4(gunPos, 1.f);

        // At the hip the rifles cant inward a touch; aimed they sit level
        float yawIn = (activeWeapon >= 2) ? (1.f - a) * 4.f : activeWeapon == 0 ? -9.f : 0.f;
        yawIn += inspectYaw + reloadYaw;
        if (yawIn != 0.f) gunBase = gunBase * glm::rotate(glm::mat4(1.f), glm::radians(yawIn), glm::vec3{0.f, 1.f, 0.f});
        if (pitchUp != 0.f) gunBase = gunBase * glm::rotate(glm::mat4(1.f), glm::radians(pitchUp * kickScale), glm::vec3{-1.f, 0.f, 0.f});
        if (roll != 0.f) {
            glm::mat4 rollMat = glm::rotate(glm::mat4(1.f),
                                            glm::radians(roll * (1.f - 0.5f * a)),
                                            glm::vec3{0.f, 0.f, 1.f});
            gunBase = gunBase * rollMat;
        }

        // Narrow FOV projection (prevents gun from clipping into walls)
        float vmFov = 65.f;
        glm::mat4 vmProj = glm::perspective(
            glm::radians(vmFov), cam.aspectRatio, 0.03f, 10.f);

        // Clear depth — gun always draws on top
        glClear(GL_DEPTH_BUFFER_BIT);

        shader.setMat4("projection", vmProj);
        shader.setMat4("view",       cam.viewMatrix());
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});

        glDisable(GL_CULL_FACE);

        switch (activeWeapon) {
            case 0: drawRevolver(shader, gunBase, flash); break;
            case 1: drawShotgun(shader, gunBase, flash);  break;
            case 2: drawKar(shader, gunBase, flash);      break;
            case 3: drawLongshot(shader, gunBase, flash); break;
            default: break;
        }

        if (parryTimer > 0.f) drawFist(shader, cam);
        glEnable(GL_CULL_FACE);
    }

    // The punching arm, in camera space: wound back low-left, snapping out to
    // just under the crosshair, holding a beat, then pulling back.
    void drawFist(ShaderProgram& shader, const Camera& cam) {
        float u = 1.f - parryTimer / PARRY_TIME;          // 0 → 1 over the punch
        float out = u < 0.3f ? u / 0.3f : u < 0.55f ? 1.f : 1.f - (u - 0.55f) / 0.45f;
        out = out * out * (3.f - 2.f * out);
        glm::vec3 fwd = cam.forward(), right = cam.right();
        glm::vec3 up = glm::normalize(glm::cross(right, fwd));
        glm::vec3 from{-0.32f, -0.36f, 0.2f}, to{-0.08f, -0.13f, 0.58f};
        glm::vec3 o = glm::mix(from, to, out);
        glm::mat4 base(1.f);
        base[0] = glm::vec4(right, 0.f); base[1] = glm::vec4(up, 0.f); base[2] = glm::vec4(fwd, 0.f);
        base[3] = glm::vec4(cam.position + right * o.x + up * o.y + fwd * o.z, 1.f);
        base = base * glm::rotate(glm::mat4(1.f), glm::radians(-12.f + 20.f * out), glm::vec3{0.f, 1.f, 0.f})
                    * glm::rotate(glm::mat4(1.f), glm::radians(10.f), glm::vec3{1.f, 0.f, 0.f});
        glm::vec3 glove{0.13f, 0.13f, 0.15f}, plate{0.42f, 0.4f, 0.38f}, sleeve{0.22f, 0.12f, 0.1f};
        drawBox(shader, base, {0.f, -0.008f, -0.15f}, {0.058f, 0.058f, 0.17f}, sleeve);   // forearm
        drawBox(shader, base, {0.f, 0.f, -0.065f}, {0.072f, 0.05f, 0.04f}, plate);        // cuff
        float glow = parryHit ? std::max(0.f, 1.f - u * 1.6f) : 0.f;
        shader.setVec3("emissiveColor", glm::vec3{1.f, 0.75f, 0.25f} * glow * 2.f);
        drawBox(shader, base, {0.f, -0.005f, 0.f}, {0.08f, 0.07f, 0.075f}, glove);       // fist
        for (int k = 0; k < 4; ++k)                                                      // knuckles
            drawBox(shader, base, {-0.03f + 0.02f * k, 0.022f, 0.042f}, {0.017f, 0.022f, 0.016f}, plate);
        drawBox(shader, base, {0.045f, -0.012f, 0.012f}, {0.02f, 0.03f, 0.045f}, glove * 1.3f);   // thumb
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
    }

    // The Kar98 reload's timeline (0..1 through it): the clip comes in and
    // seats, the rounds are pressed down, the clip flicks out. GameplayState
    // times the sounds to these too.
    static constexpr float KAR_CLIP0 = 0.27f, KAR_CLIP1 = 0.42f;
    static constexpr float KAR_PRESS0 = 0.45f, KAR_PRESS1 = 0.66f;
    static constexpr float KAR_FLICK0 = 0.66f, KAR_FLICK1 = 0.8f;

    // Height of the Kar's sight line and the Longshot's scope axis above the
    // gun origin: aiming moves the gun down by this so they sit on screen centre.
    static constexpr float KAR_SIGHT_Y  = 0.052f;
    static constexpr float LONG_SCOPE_Y = 0.098f;

private:
    // ---- Gun geometry -------------------------------------------------------
    // All positions in gun-local space where +Z = barrel direction (world fwd).

    // ---- Animation helpers ----
    static float seg(float u, float a, float b) { return std::clamp((u - a) / (b - a), 0.f, 1.f); }
    static float smooth(float t) { return t * t * (3.f - 2.f * t); }
    static glm::mat4 T(glm::vec3 v) { return glm::translate(glm::mat4(1.f), v); }
    static glm::mat4 R(float deg, glm::vec3 axis) { return glm::rotate(glm::mat4(1.f), glm::radians(deg), axis); }

    float heat = 0.f;           // revolver barrel stripe: glows hotter as you fan it
    float cylSpin = 0.f;        // degrees still to turn to the next chamber
    float reloadU = -1.f;       // 0..1 through the current reload (-1: not reloading)
    int   ammoNow = 8, magNow = 8;

    // THE REVOLVER — OVERDRIVE's sidearm. A heavy blackened-steel magnum: a
    // chrome vent rib over a full-length underlug, a ported compensator, an
    // eight-shot cylinder whose chambers glow orange while loaded (it's your
    // ammo counter), a heat stripe that brightens as you fan it, a glowing
    // front sight, and an oxblood grip with a brass medallion.
    void drawRevolver(ShaderProgram& shader, const glm::mat4& base, float flash) {
        const glm::vec3 black  {0.10f, 0.10f, 0.11f};
        const glm::vec3 steel  {0.20f, 0.20f, 0.22f};
        const glm::vec3 chrome {0.72f, 0.72f, 0.76f};
        const glm::vec3 grip   {0.36f, 0.09f, 0.07f};
        const glm::vec3 brass  {0.85f, 0.62f, 0.22f};
        const glm::vec3 orange {1.0f, 0.55f, 0.1f};
        const float CY = 0.018f, CZ = 0.036f;              // cylinder axis

        // ---- reload state ----
        float u = reloadU;
        bool  rl = u >= 0.f;
        float swing = rl ? smooth(seg(u, 0.03f, 0.13f)) * (1.f - smooth(seg(u, 0.76f, 0.84f))) : 0.f;
        float spin  = rl ? smooth(seg(u, 0.76f, 0.95f)) * 540.f : 0.f;      // spun shut
        int   lit;                                                         // chambers glowing
        int   shown = (int)std::ceil(8.f * ammoNow / magNow);
        if (!rl)              lit = shown;
        else if (u < 0.22f)   lit = shown;
        else if (u < 0.55f)   lit = 0;
        else                  lit = (int)std::round(8.f * seg(u, 0.55f, 0.72f));

        // ---- frame ----
        drawBox(shader, base, {0.f, 0.006f, -0.036f}, {0.050f, 0.082f, 0.062f}, black);   // rear of the frame
        drawBox(shader, base, {0.f, -0.026f, CZ},     {0.044f, 0.018f, 0.078f}, black);   // bridge under the cylinder
        drawBox(shader, base, {0.f, 0.050f, 0.030f},  {0.040f, 0.014f, 0.13f},  steel);   // top strap
        drawBox(shader, base, {0.f, 0.020f, 0.080f},  {0.044f, 0.064f, 0.012f}, black);   // barrel seat
        // Side plates with an orange inlay line
        for (float sx : {-1.f, 1.f}) {
            drawBox(shader, base, {sx * 0.0255f, 0.004f, -0.036f}, {0.002f, 0.06f, 0.05f}, steel);
            glow(shader, orange * (0.35f + heat * 0.8f));
            drawBox(shader, base, {sx * 0.0268f, 0.03f, -0.036f}, {0.001f, 0.004f, 0.048f}, orange);
            glow(shader, {});
        }
        // ---- barrel: tube, underlug, vent rib, compensator, sights ----
        drawBox(shader, base, {0.f, 0.028f, 0.195f}, {0.030f, 0.030f, 0.23f}, steel);
        drawBox(shader, base, {0.f, 0.000f, 0.185f}, {0.034f, 0.032f, 0.21f}, black);      // underlug
        drawBox(shader, base, {0.f, 0.048f, 0.195f}, {0.014f, 0.010f, 0.23f}, chrome);     // vent rib
        for (int k = 0; k < 5; ++k)                                                          // vents
            drawBox(shader, base, {0.f, 0.0535f, 0.105f + k * 0.045f}, {0.015f, 0.002f, 0.018f}, black);
        for (float sx : {-1.f, 1.f}) {                                                       // heat stripes
            glow(shader, orange * (0.25f + heat * 1.6f));
            drawBox(shader, base, {sx * 0.0172f, 0.0f, 0.185f}, {0.001f, 0.005f, 0.19f}, orange);
            glow(shader, {});
            drawBox(shader, base, {sx * 0.0158f, 0.028f, 0.195f}, {0.001f, 0.026f, 0.22f}, chrome);   // polished flats
        }
        drawBox(shader, base, {0.f, 0.020f, 0.326f}, {0.040f, 0.058f, 0.036f}, black);     // compensator
        for (int k = 0; k < 2; ++k)                                                          // its ports
            drawBox(shader, base, {0.f, 0.0495f, 0.316f + k * 0.017f}, {0.026f, 0.002f, 0.008f}, steel * 0.5f);
        glow(shader, orange * 1.4f);
        drawBox(shader, base, {0.f, 0.062f, 0.322f}, {0.004f, 0.012f, 0.014f}, orange);     // front sight blade
        glow(shader, {});
        drawBox(shader, base, {-0.009f, 0.062f, -0.052f}, {0.006f, 0.012f, 0.008f}, black);// rear sight ears
        drawBox(shader, base, { 0.009f, 0.062f, -0.052f}, {0.006f, 0.012f, 0.008f}, black);

        // ---- hammer: falls when you fire, cocks back after ----
        float fall = anim == ViewAnim::FIRE ? (animTimer / animMax) : 0.f;
        glm::mat4 hm = base * T({0.f, 0.040f, -0.064f}) * R(-28.f + fall * 28.f, {1.f, 0.f, 0.f});
        drawBox(shader, hm, {0.f, 0.012f, -0.004f}, {0.011f, 0.024f, 0.010f}, black);
        drawBox(shader, hm, {0.f, 0.024f, -0.012f}, {0.012f, 0.006f, 0.014f}, steel);

        // ---- the cylinder: swings out on its crane for a reload ----
        glm::vec3 pivot{-0.024f, -0.022f, CZ};
        glm::mat4 crane = base * T(pivot) * R(swing * 82.f, {0.f, 0.f, 1.f}) * T(-pivot);
        drawBox(shader, crane, {-0.012f, -0.022f, CZ + 0.044f}, {0.012f, 0.008f, 0.02f}, steel);   // crane arm
        float cyl = cylSpin + spin;
        glm::mat4 cm = crane * T({0.f, CY, CZ}) * R(cyl, {0.f, 0.f, 1.f});
        for (int k = 0; k < 4; ++k)                                                  // octagonal prism: four
            drawBox(shader, cm * R(k * 45.f, {0.f, 0.f, 1.f}), {}, {0.0257f, 0.062f, 0.070f}, steel);   // crossed slabs
        // Each chamber shows through a slot in the cylinder's side, glowing
        // while it's loaded: the ammo counter you see from where you hold it
        for (int k = 0; k < 8; ++k) {
            glm::mat4 f = cm * R(k * 45.f, {0.f, 0.f, 1.f});
            bool loaded = k < lit;
            glow(shader, loaded ? orange * 1.8f : glm::vec3{});
            drawBox(shader, f, {0.f, 0.0313f, -0.006f}, {0.011f, 0.002f, 0.044f}, loaded ? orange : black * 0.6f);
        }
        glow(shader, {});
        glow(shader, orange * (0.4f + heat));
        for (int k = 0; k < 4; ++k)                                                  // the band round its middle
            drawBox(shader, cm * R(k * 45.f, {0.f, 0.f, 1.f}), {0.f, 0.f, 0.03f}, {0.027f, 0.0645f, 0.003f}, orange);
        glow(shader, {});
        // Chambers, front and back: the next one to fire sits at the top
        for (int k = 0; k < 8; ++k) {
            float a = glm::radians(90.f + k * 45.f);
            glm::vec3 c{std::cos(a) * 0.021f, std::sin(a) * 0.021f, 0.f};
            bool loaded = k < lit;
            glow(shader, loaded ? orange * 1.6f : glm::vec3{});
            for (float z : {0.0355f, -0.0355f})
                drawBox(shader, cm, c + glm::vec3{0.f, 0.f, z}, {0.011f, 0.011f, 0.003f}, loaded ? orange : black * 0.5f);
        }
        glow(shader, {});
        drawBox(shader, crane, {0.f, CY, CZ + 0.05f}, {0.006f, 0.006f, 0.03f}, chrome);   // ejector rod
        drawBox(shader, crane, {0.f, CY, CZ}, {0.012f, 0.012f, 0.076f}, chrome);          // its star

        // ---- the brass: eight spent cases tumble out when you eject ----
        if (rl && u > 0.16f && u < 0.42f) {
            float t = seg(u, 0.16f, 0.42f);
            for (int k = 0; k < 8; ++k) {
                float a = glm::radians(90.f + k * 45.f);
                glm::vec3 from{std::cos(a) * 0.021f, CY + std::sin(a) * 0.021f, CZ - 0.04f};
                float tk = std::max(0.f, t * 1.3f - k * 0.03f);
                glm::vec3 at = from + glm::vec3{-0.03f * tk + std::cos(a) * 0.02f * tk, -0.35f * tk * tk, -0.12f * tk};
                glm::mat4 m = crane * T(at) * R(tk * 400.f + k * 30.f, {1.f, 0.3f, 0.f});
                drawBox(shader, m, {}, {0.009f, 0.009f, 0.030f}, brass * 0.8f);
            }
        }
        // ---- the speedloader: rises from below, seats eight rounds, drops away ----
        if (rl && u > 0.34f && u < 0.78f) {
            float in  = smooth(seg(u, 0.34f, 0.55f));
            float out = smooth(seg(u, 0.64f, 0.78f));
            glm::vec3 seat{0.f, CY, CZ - 0.06f};
            glm::vec3 at = seat + glm::vec3{-0.02f, -0.22f, -0.10f} * (1.f - in) + glm::vec3{-0.06f, -0.25f, -0.05f} * out;
            glm::mat4 lm = crane * T(at);
            drawBox(shader, lm, {0.f, 0.f, -0.012f}, {0.05f, 0.05f, 0.016f}, black);            // the loader body
            drawBox(shader, lm, {0.f, 0.f, -0.026f}, {0.018f, 0.018f, 0.014f}, chrome);          // its knob
            if (u < 0.6f)                                                                         // rounds until seated
                for (int k = 0; k < 8; ++k) {
                    float a = glm::radians(90.f + k * 45.f);
                    glm::vec3 c{std::cos(a) * 0.021f, std::sin(a) * 0.021f, 0.012f};
                    drawBox(shader, lm, c, {0.009f, 0.009f, 0.024f}, brass);
                    glow(shader, orange * 0.6f);
                    drawBox(shader, lm, c + glm::vec3{0.f, 0.f, 0.013f}, {0.006f, 0.006f, 0.004f}, orange);
                    glow(shader, {});
                }
        }

        // ---- trigger and guard ----
        drawBox(shader, base, {0.f, -0.052f, 0.000f}, {0.016f, 0.008f, 0.064f}, black);
        drawBox(shader, base, {0.f, -0.040f, 0.030f}, {0.016f, 0.030f, 0.008f}, black);
        drawBox(shader, base, {0.f, -0.034f, -0.004f + fall * 0.004f}, {0.007f, 0.026f, 0.008f}, chrome);

        // ---- grip: raked back, oxblood panels, a brass medallion, a steel butt cap ----
        glm::mat4 gm = base * T({0.f, -0.034f, -0.050f}) * R(-17.f, {1.f, 0.f, 0.f});
        drawBox(shader, gm, {0.f, -0.055f, -0.004f}, {0.040f, 0.112f, 0.050f}, black);
        for (float sx : {-1.f, 1.f}) {
            drawBox(shader, gm, {sx * 0.0215f, -0.058f, -0.004f}, {0.004f, 0.098f, 0.046f}, grip);
            for (int r = 0; r < 4; ++r)                                                    // checkering
                drawBox(shader, gm, {sx * 0.0237f, -0.085f + r * 0.012f, -0.004f}, {0.001f, 0.004f, 0.04f}, grip * 0.6f);
            glow(shader, brass * 0.25f);
            drawBox(shader, gm, {sx * 0.0238f, -0.032f, -0.004f}, {0.002f, 0.016f, 0.016f}, brass);
            glow(shader, {});
        }
        drawBox(shader, gm, {0.f, -0.114f, -0.004f}, {0.044f, 0.012f, 0.054f}, steel);   // butt cap
        drawBox(shader, gm, {0.f, -0.114f, -0.032f}, {0.012f, 0.008f, 0.006f}, chrome);  // lanyard ring

        muzzleFlash(shader, base, {0.f, 0.020f, 0.37f}, 0.042f, flash);
        if (flash > 0.f) {                                                                // compensator jets
            glow(shader, glm::vec3{1.f, 0.7f, 0.25f} * flash * 1.5f);
            for (float sx : {-0.5f, 0.5f})
                drawBox(shader, base, {sx * 0.02f, 0.062f + flash * 0.02f, 0.325f}, {0.008f, 0.03f * flash, 0.012f}, {1.f, 0.85f, 0.5f});
            glow(shader, {});
        }
    }

    void glow(ShaderProgram& shader, glm::vec3 e) { shader.setVec3("emissiveColor", e); }

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

    void drawShotgun(ShaderProgram& shader, const glm::mat4& base, float flash) {
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
        // Reload: thumb each missing shell into the loading port under the
        // receiver, then rack the pump
        if (reloadU >= 0.f) {
            float u = reloadU;
            float rack = seg(u, 0.84f, 1.f);
            pumpSlide = rack < 0.5f ? -rack * 2.f * 0.08f : -(1.f - rack) * 2.f * 0.08f;
            int n = reloadShells;
            float span = 0.6f / n;
            for (int i = 0; i < n; ++i) {
                float t = seg(u, 0.13f + i * span, 0.13f + (i + 0.85f) * span);
                if (t <= 0.f || t >= 1.f) continue;
                // up from below-left to the port, then pushed forward into the tube
                float up = smooth(std::min(1.f, t * 1.7f));
                float push = smooth(seg(t, 0.55f, 1.f));
                glm::vec3 port{0.f, -0.046f, -0.02f};
                glm::vec3 at = port + glm::vec3{-0.13f, -0.05f, -0.03f} * (1.f - up) + glm::vec3{0.f, 0.02f, 0.07f} * push;
                glm::mat4 sm = base * T(at) * R((1.f - up) * 40.f, {1.f, 0.f, 0.f});
                drawBox(shader, sm, {0.f, 0.f, 0.f},      {0.022f, 0.022f, 0.050f}, {0.70f, 0.08f, 0.06f});   // hull
                drawBox(shader, sm, {0.f, 0.f, -0.029f},  {0.024f, 0.024f, 0.010f}, {0.85f, 0.62f, 0.22f});   // brass head
                drawBox(shader, sm, {-0.016f, -0.012f, -0.04f + push * 0.01f}, {0.022f, 0.026f, 0.03f}, {0.13f, 0.13f, 0.15f}); // thumb
            }
        }
        drawBox(shader, base, {0.f, -0.005f, 0.08f + pumpSlide}, {0.050f, 0.044f, 0.10f}, wood);
        for (int k = 0; k < 4; ++k) {                                   // pump grip ridges
            drawBox(shader, base, {0.f, -0.005f, 0.044f + k * 0.024f + pumpSlide}, {0.052f, 0.046f, 0.006f}, wood * 0.7f);
        }
        // Side saddle: four spare shells on the left of the receiver
        drawBox(shader, base, {-0.036f, 0.004f, -0.04f}, {0.008f, 0.05f, 0.11f}, dark);
        for (int k = 0; k < 4; ++k) {
            drawBox(shader, base, {-0.044f, 0.004f, -0.08f + k * 0.026f}, {0.012f, 0.044f, 0.02f}, {0.70f, 0.08f, 0.06f});
            drawBox(shader, base, {-0.044f, -0.022f, -0.08f + k * 0.026f}, {0.013f, 0.009f, 0.021f}, {0.85f, 0.62f, 0.22f});
        }
        drawBox(shader, base, {0.f, -0.046f, -0.02f}, {0.026f, 0.004f, 0.05f}, {0.05f, 0.05f, 0.05f});   // loading port
        // Stock
        drawBox(shader, base, {0.f, -0.035f, -0.16f}, {0.048f, 0.065f, 0.10f}, wood);
        drawBox(shader, base, {0.f, -0.060f, -0.22f}, {0.040f, 0.055f, 0.06f}, wood);
        // Trigger guard
        drawBox(shader, base, {0.f, -0.050f, -0.04f}, {0.020f, 0.022f, 0.070f}, dark);
        // Grip
        drawBox(shader, base, {0.f, -0.120f, -0.08f}, {0.044f, 0.100f, 0.055f}, wood);

        // Muzzle flash — wider than revolver
        if (flash > 0.f) {
            float glow = flash;
            shader.setVec3("emissiveColor", glm::vec3{1.f, 0.7f, 0.2f} * glow * 1.2f);
            drawBox(shader, base, {0.f, 0.028f, 0.30f}, {0.035f, 0.035f, 0.04f},
                    glm::mix(metal, glm::vec3{1.f, 0.85f, 0.4f}, glow));
            shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        }
    }

    // Bolt handle state for the rifles' BOLT animation (set in draw())
    float boltLift = 0.f, boltPull = 0.f;

    // Bolt handle on the right of the receiver: rotates up, then slides back
    void drawBoltHandle(ShaderProgram& shader, const glm::mat4& base, glm::vec3 pivot, float len,
                        glm::vec3 metal, glm::vec3 knob) {
        glm::mat4 m = base * glm::translate(glm::mat4(1.f), pivot + glm::vec3{0.f, 0.f, -boltPull * 0.07f})
                           * glm::rotate(glm::mat4(1.f), boltLift * 1.4f, glm::vec3{0.f, 0.f, 1.f});
        drawBox(shader, m, {len * 0.5f, 0.f, 0.f}, {len, 0.011f, 0.011f}, metal);
        drawBox(shader, m, {len, -0.004f, 0.f}, {0.016f, 0.016f, 0.016f}, knob);
    }

    void muzzleFlash(ShaderProgram& shader, const glm::mat4& base, glm::vec3 at, float size, float flash) {
        if (flash <= 0.f) return;
        shader.setVec3("emissiveColor", glm::vec3{1.f, 0.75f, 0.3f} * flash * 1.6f);
        drawBox(shader, base, at, glm::vec3{size, size, size * 1.4f} * (0.6f + 0.4f * flash), {1.f, 0.85f, 0.5f});
        drawBox(shader, base, at + glm::vec3{0.f, 0.f, size * 0.6f},
                glm::vec3{size * 0.45f, size * 0.45f, size * 1.6f} * flash, {1.f, 0.9f, 0.6f});
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
    }

    // Kar98: a long wooden-stocked bolt rifle. Rear notch at z 0.11 and the
    // front post at z 0.62 both peak at y = KAR_SIGHT_Y, so aiming puts the
    // post in the notch exactly on screen centre.
    void drawKar(ShaderProgram& shader, const glm::mat4& base, float flash) {
        glm::vec3 wood  = {0.42f, 0.24f, 0.11f};
        glm::vec3 woodD = {0.30f, 0.17f, 0.08f};
        glm::vec3 metal = {0.22f, 0.22f, 0.24f};
        glm::vec3 steel = {0.45f, 0.45f, 0.48f};
        // Stock, wrist and butt
        drawBox(shader, base, {0.f, -0.040f, -0.150f}, {0.050f, 0.066f, 0.30f}, wood);
        drawBox(shader, base, {0.f, -0.080f, -0.270f}, {0.052f, 0.100f, 0.07f}, wood);
        drawBox(shader, base, {0.f, -0.082f, -0.308f}, {0.054f, 0.104f, 0.012f}, woodD);   // butt plate
        drawBox(shader, base, {0.f, -0.068f, -0.040f}, {0.040f, 0.070f, 0.07f}, woodD);    // wrist
        // Receiver and magazine floorplate
        drawBox(shader, base, {0.f,  0.014f,  0.020f}, {0.040f, 0.044f, 0.17f}, metal);
        drawBox(shader, base, {0.f, -0.034f,  0.050f}, {0.034f, 0.026f, 0.08f}, metal);
        drawBox(shader, base, {0.f, -0.056f, -0.005f}, {0.016f, 0.020f, 0.05f}, metal);    // trigger guard
        // Forend and barrel bands
        drawBox(shader, base, {0.f, -0.004f,  0.250f}, {0.044f, 0.040f, 0.32f}, wood);
        drawBox(shader, base, {0.f,  0.004f,  0.200f}, {0.048f, 0.050f, 0.012f}, steel);
        drawBox(shader, base, {0.f,  0.004f,  0.380f}, {0.048f, 0.050f, 0.012f}, steel);
        // Barrel
        drawBox(shader, base, {0.f,  0.028f,  0.360f}, {0.020f, 0.020f, 0.58f}, metal);
        // Rear sight: two ears with a notch between them
        drawBox(shader, base, {-0.010f, 0.044f, 0.110f}, {0.008f, 0.024f, 0.012f}, steel);
        drawBox(shader, base, { 0.010f, 0.044f, 0.110f}, {0.008f, 0.024f, 0.012f}, steel);
        drawBox(shader, base, { 0.f,    0.036f, 0.110f}, {0.028f, 0.008f, 0.016f}, steel);
        // Front post inside a hood
        drawBox(shader, base, { 0.f,    0.044f, 0.630f}, {0.004f, 0.016f, 0.008f}, {0.85f, 0.85f, 0.8f});
        drawBox(shader, base, {-0.011f, 0.046f, 0.630f}, {0.004f, 0.024f, 0.014f}, metal);
        drawBox(shader, base, { 0.011f, 0.046f, 0.630f}, {0.004f, 0.024f, 0.014f}, metal);
        drawBoltHandle(shader, base, {0.020f, 0.022f, -0.035f}, 0.036f, steel, steel * 1.2f);
        if (reloadU >= 0.f) drawKarReload(shader, base, reloadU);
        muzzleFlash(shader, base, {0.f, 0.028f, 0.68f}, 0.05f, flash);
    }

    // The Kar98's stripper-clip reload, in gun space. The action is open from
    // the bolt pull to the bolt push; the clip guide sits over its rear.
    void drawKarReload(ShaderProgram& shader, const glm::mat4& base, float u) {
        glm::vec3 brass{0.85f, 0.62f, 0.22f}, copper{0.72f, 0.38f, 0.18f}, clipSteel{0.3f, 0.3f, 0.33f},
                  glove{0.13f, 0.13f, 0.15f}, sleeve{0.22f, 0.12f, 0.1f};
        // The empty, kicked up and out to the right as the bolt comes back
        float ej = seg(u, 0.2f, 0.36f);
        if (ej > 0.f && ej < 1.f) {
            glm::vec3 at = glm::vec3{0.02f, 0.04f, 0.03f} + glm::vec3{0.16f, 0.f, -0.05f} * ej
                         + glm::vec3{0.f, 0.11f * std::sin(ej * 3.14159f) - 0.05f * ej * ej, 0.f};
            glm::mat4 cm = base * T(at) * R(ej * 540.f, {0.3f, 0.2f, 1.f});
            drawBox(shader, cm, {0.f, 0.f, 0.f}, {0.01f, 0.01f, 0.04f}, brass);
        }
        // The clip: in from above-right, seated in the guide, pressed empty, flicked away
        float in = smooth(seg(u, KAR_CLIP0, KAR_CLIP1));
        float press = smooth(seg(u, KAR_PRESS0, KAR_PRESS1));
        float flick = seg(u, KAR_FLICK0, KAR_FLICK1);
        if (u < KAR_CLIP0 || flick >= 1.f) return;
        glm::vec3 seat{0.f, 0.07f, -0.005f};
        glm::vec3 clipAt = seat + glm::vec3{0.09f, 0.1f, -0.04f} * (1.f - in)
                         + glm::vec3{0.12f * flick, 0.09f * std::sin(flick * 3.14159f) + 0.02f * flick, -0.03f * flick};
        glm::mat4 cm = base * T(clipAt) * R((1.f - in) * -35.f + flick * 300.f, {0.f, 0.f, 1.f});
        drawBox(shader, cm, {0.f, 0.f, -0.009f}, {0.016f, 0.066f, 0.004f}, clipSteel);   // the clip's spine
        drawBox(shader, cm, {0.f, 0.032f, 0.f},  {0.016f, 0.004f, 0.02f}, clipSteel);
        drawBox(shader, cm, {0.f, -0.032f, 0.f}, {0.016f, 0.004f, 0.02f}, clipSteel);
        // The rounds stacked in it, stripped down into the magazine by the thumb
        int n = std::max(1, std::min(reloadShells, 5));
        if (flick <= 0.f)
            for (int i = 0; i < n; ++i) {
                float y = 0.024f - i * 0.012f - press * 0.075f;
                glm::mat4 rm = base * T(clipAt + glm::vec3{0.f, y, 0.022f});
                drawBox(shader, rm, {0.f, 0.f, 0.f},     {0.0095f, 0.0095f, 0.042f}, brass);
                drawBox(shader, rm, {0.f, 0.f, 0.029f},  {0.007f, 0.007f, 0.018f}, copper);
            }
        // Left thumb and hand: brings the clip in, presses the stack, flicks the clip
        float handIn = smooth(seg(u, KAR_CLIP0 - 0.04f, KAR_CLIP0 + 0.06f)) * (1.f - smooth(seg(u, KAR_FLICK0 + 0.02f, KAR_FLICK1)));
        if (handIn > 0.01f) {
            // The hand comes in from low on the left, its thumb over the stack
            glm::vec3 thumb = clipAt + glm::vec3{0.f, 0.04f - press * 0.06f, 0.012f};
            glm::vec3 hand = thumb + glm::vec3{-0.03f, -0.005f, -0.004f} + glm::vec3{-0.12f, -0.1f, -0.04f} * (1.f - handIn);
            glm::mat4 hm = base * T(hand) * R(20.f, {0.f, 0.f, 1.f});
            drawBox(shader, hm, {0.f, 0.f, 0.f},          {0.028f, 0.034f, 0.03f}, glove);
            drawBox(shader, hm, {-0.03f, -0.03f, -0.02f}, {0.026f, 0.026f, 0.08f}, sleeve);
            drawBox(shader, base * T(thumb), {0.f, 0.f, 0.f}, {0.013f, 0.012f, 0.022f}, glove * 1.3f);
        }
    }

    // Longshot: a heavy .50 with a big scope, muzzle brake and folded bipod.
    void drawLongshot(ShaderProgram& shader, const glm::mat4& base, float flash) {
        glm::vec3 poly  = {0.14f, 0.15f, 0.14f};
        glm::vec3 olive = {0.28f, 0.30f, 0.20f};
        glm::vec3 metal = {0.20f, 0.20f, 0.22f};
        glm::vec3 steel = {0.42f, 0.42f, 0.46f};
        glm::vec3 lens  = {0.15f, 0.35f, 0.5f};
        // Stock: skeleton with cheek riser and a thick butt pad
        drawBox(shader, base, {0.f, -0.030f, -0.200f}, {0.050f, 0.050f, 0.20f}, olive);
        drawBox(shader, base, {0.f, -0.090f, -0.250f}, {0.046f, 0.030f, 0.14f}, olive);
        drawBox(shader, base, {0.f,  0.020f, -0.210f}, {0.044f, 0.030f, 0.14f}, olive);
        drawBox(shader, base, {0.f, -0.040f, -0.315f}, {0.056f, 0.140f, 0.030f}, poly);
        // Receiver, grip, magazine
        drawBox(shader, base, {0.f,  0.012f,  0.020f}, {0.066f, 0.066f, 0.25f}, metal);
        drawBox(shader, base, {0.f, -0.085f, -0.060f}, {0.040f, 0.110f, 0.050f}, poly);
        drawBox(shader, base, {0.f, -0.070f,  0.060f}, {0.046f, 0.090f, 0.070f}, poly);
        // Fluted heavy barrel and muzzle brake
        drawBox(shader, base, {0.f,  0.026f,  0.440f}, {0.034f, 0.034f, 0.62f}, metal);
        drawBox(shader, base, {0.f,  0.026f,  0.300f}, {0.040f, 0.012f, 0.30f}, steel);
        drawBox(shader, base, {0.f,  0.026f,  0.770f}, {0.062f, 0.046f, 0.075f}, poly);
        drawBox(shader, base, {0.f,  0.026f,  0.770f}, {0.070f, 0.016f, 0.030f}, metal);
        // Folded bipod under the barrel
        drawBox(shader, base, {-0.016f, -0.012f, 0.380f}, {0.010f, 0.010f, 0.24f}, steel);
        drawBox(shader, base, { 0.016f, -0.012f, 0.380f}, {0.010f, 0.010f, 0.24f}, steel);
        drawBox(shader, base, { 0.f,     0.000f, 0.260f}, {0.050f, 0.018f, 0.030f}, metal);
        // Scope: rings, tube, bells, turrets, lenses
        drawBox(shader, base, {0.f, 0.058f, -0.020f}, {0.030f, 0.030f, 0.022f}, metal);
        drawBox(shader, base, {0.f, 0.058f,  0.110f}, {0.030f, 0.030f, 0.022f}, metal);
        drawBox(shader, base, {0.f, LONG_SCOPE_Y, 0.040f}, {0.044f, 0.044f, 0.30f}, poly);
        drawBox(shader, base, {0.f, LONG_SCOPE_Y, 0.215f}, {0.066f, 0.066f, 0.070f}, poly);
        drawBox(shader, base, {0.f, LONG_SCOPE_Y, -0.135f}, {0.056f, 0.056f, 0.060f}, poly);
        drawBox(shader, base, {0.f, LONG_SCOPE_Y + 0.030f, 0.045f}, {0.022f, 0.022f, 0.026f}, steel);
        drawBox(shader, base, {0.030f, LONG_SCOPE_Y, 0.045f}, {0.022f, 0.022f, 0.026f}, steel);
        shader.setVec3("emissiveColor", lens * 0.35f);
        drawBox(shader, base, {0.f, LONG_SCOPE_Y, 0.251f}, {0.054f, 0.054f, 0.004f}, lens);
        drawBox(shader, base, {0.f, LONG_SCOPE_Y, -0.166f}, {0.044f, 0.044f, 0.004f}, lens);
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        drawBoltHandle(shader, base, {0.034f, 0.025f, -0.060f}, 0.055f, steel, poly);
        muzzleFlash(shader, base, {0.f, 0.026f, 0.84f}, 0.07f, flash);
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
