#pragma once
// =============================================================================
// ViewModel.h — the gun in your hands, drawn over the world.
//
// It frames the gun (one hip position for every gun, the rifles sliding to
// their sights or scope as you aim), asks GunMotion where it is this frame
// (kick, cycling, switch, the three reload beats, sway, inspect) and asks
// GunKit what it's made of (one body, a glow per gun, the ammo cells), then
// draws those boxes with its own narrow projection, depth cleared, so the gun
// never clips into walls. The sounds come out of GunMotion as timed cues; the
// game plays them (takeCues).
//
// It also draws the gauntlet that punches (GunKit's fist), and keeps the
// camera's head bob.
// =============================================================================
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "ShaderProgram.h"
#include "Mesh.h"
#include "GunKit.h"
#include <vector>
#include <cmath>
#include <algorithm>

class ViewModel {
public:
    float bobTimer = 0.f;
    Mesh cubeMesh;          // unit cube (-0.5..0.5), reused for every box
    GunMotion motion;       // how the gun moves (GunMotion.h)

    // Aimed: the gun moves down by these so the sights / scope sit on screen centre
    static constexpr float KAR_SIGHT_Y  = gunkit::LANCER_SIGHT_Y;
    static constexpr float LONG_SCOPE_Y = gunkit::LONG_SCOPE_Y;

    ViewModel() { buildCube(); }

    // --- Triggers --------------------------------------------------------------
    void triggerFire() { motion.fire(); heat = std::min(1.f, heat + 0.22f); cylSpin = 45.f; }
    // A reload over its whole time; shells: rounds it puts back (the shotgun's shells, the Lancer's clip)
    // ammo / mag: the gun's real state as the reload starts; startU: pick it up part-way
    void triggerReload(float t, int shells, int ammo, int mag, float startU = 0.f) {
        reloadShells = std::max(1, shells);
        motion.reload(t, ammo, std::max(1, mag), reloadShells, startU);
    }
    void reset(int gun) { motion.reset(gun); parryTimer = grenadeT = grappleT = 0.f; }
    void triggerBolt(float t)     { motion.cycle(std::max(t, 0.2f)); }
    void triggerPump()            { motion.cycle(0.45f); }
    void triggerSwitch(int toGun) { motion.switchTo(toGun); }
    void triggerInspect()         { motion.inspect(); }
    void triggerGrenade()         { grenadeT = GRENADE_TIME; }
    void triggerGrapple()         { grappleT = GRAPPLE_TIME; }
    // Punch / parry: the gauntlet drives forward from the lower left, alongside
    // whatever the gun is doing
    void triggerParry(bool hit)   { parryTimer = PARRY_TIME; parryHit = hit; }
    static constexpr float PARRY_TIME = 0.3f;
    float parryTimer = 0.f;
    bool  parryHit = false;
    int   reloadShells = 1;
    void setMove(bool dashing, bool sliding) { motion.setMove(dashing, sliding); }
    void land(float fallSpeed) { motion.land(fallSpeed); }
    std::vector<GunCue> takeCues() { std::vector<GunCue> c; c.swap(motion.cues); return c; }

    void update(float dt, float xzSpeed, bool /*onGround*/) {
        motion.update(dt);
        parryTimer = std::max(0.f, parryTimer - dt);
        grenadeT = std::max(0.f, grenadeT - dt);
        grappleT = std::max(0.f, grappleT - dt);
        heat     = std::max(0.f, heat - dt * 0.7f);
        cylSpin *= std::exp(-dt * 22.f);          // the cylinder clicks round to the next chamber
        bobTimer += dt * (xzSpeed > 0.5f ? xzSpeed * 0.38f : 0.8f);
    }

    // A world-space nudge for the camera's head bob
    glm::vec3 getBobOffset(float xzSpeed, bool onGround, const glm::vec3& camRight) const {
        if (!onGround || xzSpeed < 0.3f)
            return camRight * (sinf(bobTimer * 1.1f) * 0.003f) + glm::vec3{0, sinf(bobTimer * 0.65f) * 0.002f, 0};
        float amp = std::min(xzSpeed / 9.f, 1.f) * 0.016f;
        return camRight * (sinf(bobTimer * 2.f) * amp * 0.4f) + glm::vec3{0, fabsf(sinf(bobTimer)) * amp, 0};
    }

    // flash: muzzle flash 0..1; aim: 0 hip .. 1 aimed (rifles); ammo / mag: the cells
    void draw(ShaderProgram& shader, const Camera& cam, int activeWeapon, float flash, float aim = 0.f,
              int ammo = 8, int mag = 8) {
        magSeen = std::max(1, mag);
        if (!motion.switchingNow() && motion.shownGun() != activeWeapon) motion.reset(activeWeapon);   // out of step: start clean
        int g = motion.shownGun();
        GunPose P = motion.pose();
        // The grenade toss and the grapple shove ride on top
        if (grenadeT > 0.f) {
            float p = grenadeT / GRENADE_TIME;
            P.offset += glm::vec3{-p * 0.05f, std::sin(p * 3.14159f) * 0.045f, p * 0.07f};
        }
        if (grappleT > 0.f) P.offset.z += grappleT / GRAPPLE_TIME * 0.06f;

        GunLook L;
        L.ammo = ammo; L.mag = magSeen; L.lit = motion.litCells(ammo, magSeen);
        L.flash = flash; L.heat = heat; L.cylSpin = cylSpin; L.refill = reloadShells; L.aim = aim;

        glm::vec3 fwd = cam.forward(), right = cam.right();
        glm::vec3 up = glm::normalize(glm::cross(right, fwd));
        float a = aim * aim * (3.f - 2.f * aim);
        glm::vec3 hip = gunkit::hipOf(g);
        glm::vec3 aimed = g == 2 ? glm::vec3{0.f, -gunkit::LANCER_SIGHT_Y, 0.30f}
                        : g == 3 ? glm::vec3{0.f, -gunkit::LONG_SCOPE_Y, 0.05f} : hip;
        glm::vec3 off = glm::mix(hip, aimed, a) + P.offset * (1.f - 0.6f * a);
        glm::mat4 base(1.f);
        base[0] = glm::vec4(right, 0.f); base[1] = glm::vec4(up, 0.f); base[2] = glm::vec4(fwd, 0.f);
        base[3] = glm::vec4(cam.position + right * off.x + up * off.y + fwd * off.z, 1.f);
        float yawIn = (g >= 2 ? (1.f - a) * 4.f : g == 0 ? -9.f : -3.f) + P.yaw;   // barrels converge on the crosshair
        base = base * glm::rotate(glm::mat4(1.f), glm::radians(yawIn), glm::vec3{0.f, 1.f, 0.f})
                    * glm::rotate(glm::mat4(1.f), glm::radians(P.pitch * (1.f - 0.6f * a)), glm::vec3{-1.f, 0.f, 0.f})
                    * glm::rotate(glm::mat4(1.f), glm::radians(P.roll * (1.f - 0.5f * a)), glm::vec3{0.f, 0.f, 1.f});

        glm::mat4 vmProj = glm::perspective(glm::radians(65.f), cam.aspectRatio, 0.03f, 10.f);
        glClear(GL_DEPTH_BUFFER_BIT);   // the gun always draws on top
        lastEmissive = glm::vec3{-1.f};
        shader.setMat4("projection", vmProj);
        shader.setMat4("view", cam.viewMatrix());
        glDisable(GL_CULL_FACE);
        if (!P.hidden) {
            parts.clear();
            gunkit::buildGun(g, L, P, parts);
            for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive);
        }
        if (parryTimer > 0.f) drawFist(shader, cam, gunkit::glowOf(g));
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        lastEmissive = glm::vec3{-1.f};
        glEnable(GL_CULL_FACE);
    }

private:
    static constexpr float GRENADE_TIME = 0.5f, GRAPPLE_TIME = 0.28f;
    float grenadeT = 0.f, grappleT = 0.f;
    float heat = 0.f;           // the revolver's heat stripe: hotter as you fan it
    float cylSpin = 0.f;        // degrees still to turn to the next chamber
    int   magSeen = 8;
    std::vector<GunPart> parts; // this frame's boxes (kept: no allocation after the first)

    // The punching arm, in camera space: wound back low-left, snapping out to
    // just under the crosshair, holding a beat, then pulling back
    void drawFist(ShaderProgram& shader, const Camera& cam, glm::vec3 glow) {
        float u = 1.f - parryTimer / PARRY_TIME;
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
        float punch = parryHit ? std::max(0.f, 1.f - u * 1.6f) : 0.f;
        parts.clear();
        gunkit::buildFist(glow, punch, parryHit, parts);
        for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive);
    }

    void drawPart(ShaderProgram& shader, const glm::mat4& model, glm::vec3 color, glm::vec3 emissive) {
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
        if (emissive != lastEmissive) { shader.setVec3("emissiveColor", emissive); lastEmissive = emissive; }   // most parts share it
        cubeMesh.draw();
    }
    glm::vec3 lastEmissive{-1.f};

    // A unit cube (-0.5..0.5) with 0..1 UVs per face so the shader's edge-darkening works
    void buildCube() {
        std::vector<Vertex> verts;
        std::vector<unsigned int> idx;
        auto face = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n) {
            unsigned int base = (unsigned int)verts.size();
            verts.push_back({a, {0.f, 0.f}, n, {1.f, 1.f, 1.f}});
            verts.push_back({b, {1.f, 0.f}, n, {1.f, 1.f, 1.f}});
            verts.push_back({c, {1.f, 1.f}, n, {1.f, 1.f, 1.f}});
            verts.push_back({d, {0.f, 1.f}, n, {1.f, 1.f, 1.f}});
            idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        };
        face({-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}, {0,0,1});
        face({.5f,-.5f,-.5f},{-.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f}, {0,0,-1});
        face({.5f,-.5f,.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{.5f,.5f,.5f}, {1,0,0});
        face({-.5f,-.5f,-.5f},{-.5f,-.5f,.5f},{-.5f,.5f,.5f},{-.5f,.5f,-.5f}, {-1,0,0});
        face({-.5f,.5f,.5f},{.5f,.5f,.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f}, {0,1,0});
        face({-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f},{-.5f,-.5f,.5f}, {0,-1,0});
        cubeMesh.upload(verts, idx);
    }
};
