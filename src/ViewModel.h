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
#include "Sword.h"
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

    // OATHBREAKER instead of the gun. tickSword moves it (every frame it's out:
    // a new stroke takes over from wherever the blade was, anything else eases
    // there), drawSword draws it and the arms holding it.
    void tickSword(float dt, const Sword& s) {
        float u = s.swinging() ? 1.f - s.swingT / Sword::SWING_TIME : -1.f;
        float charge = s.heldFor > 0.12f ? std::min(1.f, s.heldFor / Sword::CHARGE_TIME) : 0.f;
        SwordPose target = gunkit::swordPose(s.lastStroke, u, s.blocking(), charge, s.thrustT > 0.f ? 1.f : 0.f);
        if (!s.swinging() && !s.blocking() && s.thrustT <= 0.f && charge <= 0.f && s.sinceSwing < Sword::SWING_TIME + SETTLE) {
            // just after a stroke: settle from its end back to rest (a quick stroke chained on starts from here)
            float k = gunkit::smooth(std::clamp((s.sinceSwing - Sword::SWING_TIME) / SETTLE, 0.f, 1.f));
            target = gunkit::mixPose(gunkit::swordPose(s.lastStroke, 1.f, false, 0.f, 0.f), target, k);
        }
        if (s.swinging() && s.sinceSwing < swordSince) swordFrom = swordShown;   // a new stroke: from here
        swordSince = s.sinceSwing;
        if (s.swinging()) swordShown = gunkit::mixPose(swordFrom, target, gunkit::smooth(std::clamp(u / 0.1f, 0.f, 1.f)));
        else              swordShown = gunkit::mixPose(swordShown, target, 1.f - std::exp(-dt * (s.blocking() ? 30.f : 18.f)));
        swordStroke = s.swinging() ? s.lastStroke : -1; swordU = u;
    }
    void drawSword(ShaderProgram& shader, const Camera& cam, const Sword& s) {
        SwordPose P = swordShown;
        float calm = swordStroke < 0 && !s.blocking() ? 1.f : 0.f;   // a slow breath on it at rest
        P.hand += calm * glm::vec3{std::sin(bobTimer * 1.3f) * 0.004f, std::sin(bobTimer * 0.9f) * 0.006f, 0.f};
        glm::vec3 fwd = cam.forward(), right = cam.right(), up = glm::normalize(glm::cross(right, fwd));
        glm::mat4 base(1.f);
        base[0] = glm::vec4(right, 0.f); base[1] = glm::vec4(up, 0.f); base[2] = glm::vec4(fwd, 0.f);
        base[3] = glm::vec4(cam.position, 1.f);
        glm::mat4 vmProj = glm::perspective(glm::radians(65.f), cam.aspectRatio, 0.03f, 10.f);
        glClear(GL_DEPTH_BUFFER_BIT);   // the sword always draws on top
        lastEmissive = glm::vec3{-1.f};
        shader.setMat4("projection", vmProj);
        shader.setMat4("view", cam.viewMatrix());
        glDisable(GL_CULL_FACE);
        const glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3{gunkit::SWORD_SCALE});
        const glm::mat4 frame = gunkit::swordFrame(P);
        SwordLook L; L.guard01 = s.guard01(); L.broken = s.broken(); L.charge = s.charged ? 1.f : 0.f;
        L.riposte = s.riposteT > 0.f ? 1.f : 0.f;
        parts.clear();
        gunkit::buildSword(L, parts);
        for (const auto& p : parts) drawPart(shader, base * frame * S * p.xf, p.color, p.emissive);
        // the cut's smear: the band the outer blade swept through, fading behind it
        if (swordStroke >= 0 && swordU > 0.08f && swordU < 0.85f) {
            glm::vec3 g = gunkit::SWORD_GLOW * (1.f + 1.2f * L.riposte);
            const int N = 8; const float STEP = 0.022f;
            glm::mat4 prev = frame;
            for (int k = 1; k <= N; ++k) {
                glm::mat4 f = gunkit::swordFrame(gunkit::swordPose(swordStroke, std::max(0.f, swordU - k * STEP), false, 0.f, 0.f));
                float fade = 1.f - (k - 1) / (float)N;
                // a panel along the blade's outer part, reaching from this ghost to the one after it
                glm::vec3 tipA{prev * glm::vec4(0.f, 0.f, 0.74f * gunkit::SWORD_SCALE, 1.f)}, tipB{f * glm::vec4(0.f, 0.f, 0.74f * gunkit::SWORD_SCALE, 1.f)};
                glm::vec3 along = glm::normalize(glm::vec3{f[2]} + glm::vec3{prev[2]});
                glm::vec3 across = tipA - tipB;
                float gap = glm::length(across);
                if (gap > 1e-4f) {
                    glm::vec3 x = glm::normalize(glm::cross(across, along)), y = glm::normalize(glm::cross(along, x));
                    glm::mat4 m(1.f);
                    m[0] = glm::vec4(x * 0.002f, 0.f); m[1] = glm::vec4(y * gap, 0.f); m[2] = glm::vec4(along * (0.4f * gunkit::SWORD_SCALE), 0.f);
                    m[3] = glm::vec4((tipA + tipB) * 0.5f, 1.f);
                    drawPart(shader, base * m, g * 0.04f, g * (1.1f * fade * fade));
                }
                // its leading line: the point's path, brightest
                glm::vec3 pA{prev * glm::vec4(0.f, 0.f, 0.94f * gunkit::SWORD_SCALE, 1.f)}, pB{f * glm::vec4(0.f, 0.f, 0.94f * gunkit::SWORD_SCALE, 1.f)};
                if (glm::length(pA - pB) > 1e-4f) {
                    glm::vec3 z = glm::normalize(pA - pB), x = glm::normalize(glm::cross(along, z)), y = glm::cross(z, x);
                    glm::mat4 m(1.f);
                    m[0] = glm::vec4(x * 0.006f * fade, 0.f); m[1] = glm::vec4(y * 0.006f * fade, 0.f); m[2] = glm::vec4(z * glm::length(pA - pB) * 1.1f, 0.f);
                    m[3] = glm::vec4((pA + pB) * 0.5f, 1.f);
                    drawPart(shader, base * m, glm::vec3{1.f}, glm::mix(g, glm::vec3{1.6f}, 0.5f) * 2.f * fade);
                }
                prev = f;
            }
        }
        // the arms: the right on the grip; the left on the grip under it, or flat on the blade in a guard
        parts.clear();
        gunkit::Builder b{parts, gunkit::SWORD_GLOW};
        const float glow = s.broken() ? 0.15f : 0.7f + 1.2f * L.riposte + 0.6f * L.charge;
        glm::vec3 twist{0.3f * (P.hand.x - 0.2f), 0.12f * (P.hand.y + 0.22f), 0.1f * (P.hand.z - 0.36f)};   // the body turns into the cut
        glm::mat4 fist = frame; fist[3] = glm::vec4(P.hand, 1.f);
        gunkit::buildArm(b, glm::vec3{0.34f, -0.62f, 0.f} + twist, fist, 1.f, false, glow);
        float off = std::max(P.twoHand, P.brace);
        if (off > 0.02f) {
            glm::vec3 z{frame[2]}, y{frame[1]}, root{frame[3]};
            glm::vec3 onGrip = root + z * (gunkit::SWORD_OFFHAND_Z * gunkit::SWORD_SCALE);
            glm::vec3 onFlat = root + z * 0.3f + y * 0.03f;
            glm::vec3 at = P.brace > P.twoHand ? onFlat : onGrip;
            glm::vec3 away{-0.3f, -0.6f, 0.12f};   // below the screen
            glm::mat4 lf = frame; lf[3] = glm::vec4(glm::mix(away, at, gunkit::smooth(std::min(off, 1.f))), 1.f);
            gunkit::buildArm(b, glm::vec3{-0.34f, -0.62f, 0.f} + twist * 0.6f, lf, -1.f, P.brace > P.twoHand, glow);
        }
        for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive);
        if (parryTimer > 0.f) drawFist(shader, cam, gunkit::SWORD_GLOW);   // F still punches
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        lastEmissive = glm::vec3{-1.f};
        glEnable(GL_CULL_FACE);
    }
    const SwordPose& swordPoseShown() const { return swordShown; }
    static constexpr float PARRY_TIME = 0.56f;   // (how long the punch is drawn; the parry window itself is the game's)
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
        if (parryTimer > 0.f) {   // the gun hand pulls back as the other one goes out
            float o = fistAt(parryU(), parryHit).out;
            P.offset += glm::vec3{0.03f, -0.045f, -0.04f} * o; P.roll += 7.f * o; P.yaw += 4.f * o;
        }

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
    static constexpr float GRENADE_TIME = 0.5f, GRAPPLE_TIME = 0.28f, SETTLE = 0.22f;
    SwordPose swordShown = gunkit::swordRest(), swordFrom = gunkit::swordRest();
    float swordSince = 1e9f, swordU = -1.f;
    int   swordStroke = -1;
    float grenadeT = 0.f, grappleT = 0.f;
    float heat = 0.f;           // the revolver's heat stripe: hotter as you fan it
    float cylSpin = 0.f;        // degrees still to turn to the next chamber
    int   magSeen = 8;
    std::vector<GunPart> parts; // this frame's boxes (kept: no allocation after the first)

    // The parry, in camera space: a backhand whack with the left gauntlet. It cocks across the body
    // (fist up by the right cheek, elbow high), then rips out along an arc through the crosshair and
    // wide off the left of the frame, the back of the hand leading; a parry jars it back toward the
    // centre, then it drops away. u: 0..1 through it. lead: the way the back of the hand faces.
    struct FistAt { glm::vec3 p; glm::vec3 lead; float out; };
    static constexpr float WHACK_AT = 0.2f, WHACK_END = 0.44f, WHACK_CONTACT = 0.228f, WHACK_HOLD = 0.64f;
    static glm::vec3 whackArc(float t) {   // quadratic: cocked -> through the crosshair -> wide left
        const glm::vec3 a{0.17f, -0.1f, 0.24f}, c{0.f, -0.02f, 0.66f}, e{-0.34f, -0.1f, 0.46f};   // (it ends in frame: the arm stays in view)
        return (1.f - t) * (1.f - t) * a + 2.f * (1.f - t) * t * c + t * t * e;
    }
    static glm::vec3 whackLead(float t) {  // the arc's tangent, tipped a little forward
        const glm::vec3 a{0.17f, -0.1f, 0.24f}, c{0.f, -0.02f, 0.66f}, e{-0.34f, -0.1f, 0.46f};
        return glm::normalize(2.f * (1.f - t) * (c - a) + 2.f * t * (e - c) + glm::vec3{0.f, 0.f, 0.15f});
    }
    static FistAt fistAt(float u, bool hit) {
        const glm::vec3 rest{-0.4f, -0.62f, 0.12f};
        auto sm = [](float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
        if (u < WHACK_AT) {   // cock it: up and across, the last stretch pulling back a hair further
            float k = sm(u / WHACK_AT), pull = std::sin(k * 3.14159f) * 0.03f;
            return {glm::mix(rest, whackArc(0.f) + glm::vec3{0.02f, 0.f, -0.02f}, k) + glm::vec3{pull, pull, 0.f},
                    glm::normalize(glm::mix(glm::vec3{0.f, 1.f, 0.f}, whackLead(0.f), k)), 0.f};
        }
        if (u < WHACK_END) {  // rip it out: through the mark early, overshoot the end and settle
            float x = (u - WHACK_AT) / (WHACK_END - WHACK_AT) - 1.f, t = 1.f + x * x * (2.6f * x + 1.6f);
            return {whackArc(t), whackLead(std::min(t, 1.f)), std::min(t * 1.6f, 1.f)};
        }
        const glm::vec3 end = whackArc(1.f), lead = whackLead(1.f);
        if (u < WHACK_HOLD) { // hold the follow-through; a parry jars it back toward the centre
            float h = (u - WHACK_END) / (WHACK_HOLD - WHACK_END);
            float jolt = hit ? std::sin(std::min(h * 2.5f, 1.f) * 3.14159f) : 0.f;
            float buzz = hit ? std::sin(h * 70.f) * 0.006f * (1.f - h) : 0.f;
            return {end + glm::vec3{0.07f * jolt, 0.02f * jolt + buzz, -0.03f * jolt - 0.02f * h}, lead, 1.f};
        }
        float k = sm((u - WHACK_HOLD) / (1.f - WHACK_HOLD));
        return {glm::mix(end + glm::vec3{0.f, 0.f, -0.02f}, rest + glm::vec3{0.f, -0.12f, 0.f}, k),
                glm::normalize(glm::mix(lead, glm::vec3{0.f, 1.f, 0.f}, k)), 1.f - k};
    }
    float parryU() const { return 1.f - parryTimer / PARRY_TIME; }
    // the fist's frame in camera space: +Z on along the forearm (from the shoulder), +Y the back of
    // the hand turned to lead
    static glm::mat4 fistFrame(const FistAt& F) {
        glm::vec3 z = glm::normalize(F.p - glm::vec3{-0.3f, -0.56f, -0.1f});
        glm::vec3 y = F.lead - z * glm::dot(F.lead, z);
        y = glm::length(y) < 1e-4f ? glm::vec3{0.f, 1.f, 0.f} : glm::normalize(y);
        glm::vec3 x = glm::cross(y, z);
        glm::mat4 m(1.f);
        m[0] = glm::vec4(x, 0.f); m[1] = glm::vec4(y, 0.f); m[2] = glm::vec4(z, 0.f); m[3] = glm::vec4(F.p, 1.f);
        return m;
    }

    // The arm and gauntlet, then what the whack leaves in the air: a crescent of light along the arc;
    // on a parry a white-hot burst where it crossed the crosshair, a ring of
    // shards and sparks flung off the way the hand was going
    void drawFist(ShaderProgram& shader, const Camera& cam, glm::vec3 glow) {
        const float u = parryU();
        const FistAt F = fistAt(u, parryHit);
        glm::vec3 fwd = cam.forward(), right = cam.right();
        glm::vec3 up = glm::normalize(glm::cross(right, fwd));
        glm::mat4 base(1.f);
        base[0] = glm::vec4(right, 0.f); base[1] = glm::vec4(up, 0.f); base[2] = glm::vec4(fwd, 0.f);
        base[3] = glm::vec4(cam.position, 1.f);
        const float hu = (u - WHACK_CONTACT) / 0.38f;                         // the parry's burst, 0..1
        const float flare = parryHit ? std::clamp(1.f - std::fabs(hu - 0.05f) * 2.2f, 0.f, 1.f) : 0.f;
        const glm::mat4 ff = fistFrame(F);
        parts.clear();
        gunkit::Builder b{parts, glow};
        gunkit::buildArm(b, glm::vec3{-0.3f, -0.56f, 0.02f}, ff, -1.f, false, 0.8f + 2.5f * flare);
        gunkit::buildGauntlet(b, ff, glow, flare);
        if (u > WHACK_AT && u < WHACK_END + 0.1f) {   // the smear: a soft band of the gauntlet's light along the arc it swept
            const float fade = 1.f - std::clamp((u - WHACK_END) / 0.1f, 0.f, 1.f);
            const glm::vec3 view{0.f, 0.f, 1.f};
            glm::vec3 prev = F.p;
            for (int k = 1; k <= 7; ++k) {
                glm::vec3 was = fistAt(std::max(WHACK_AT, u - 0.014f * k), parryHit).p;
                if (glm::length(was - prev) < 0.004f) break;
                float w = (1.f - k / 8.f) * fade;
                b.lit(glow * (1.1f * w));
                gunkit::limb(b, prev, was, 0.032f * w + 0.006f, 0.004f, view, glm::vec3{0.f});   // flat light, fading: motion, not an edge
                prev = was;
            }
            b.unlit();
        }
        if (parryHit && hu > 0.f && hu < 1.f) {
            const glm::vec3 c = whackArc(0.42f) + glm::vec3{0.f, 0.02f, 0.06f};   // where it crossed the crosshair
            const glm::vec3 z{0.f, 0.f, 1.f}, x{1.f, 0.f, 0.f}, y{0.f, 1.f, 0.f};
            const glm::vec3 fling = whackLead(0.42f);                              // the way it was going
            const float g = 1.f - hu, e = 1.f - g * g * g;
            const glm::vec3 hot{2.8f, 2.5f, 2.f};
            b.lit(hot * (4.f * g * g));                                            // the flash
            b.box(glm::translate(glm::mat4(1.f), c), {0.f, 0.f, 0.f}, glm::vec3{0.06f * g * g + 0.012f}, glm::vec3{1.f});
            for (int k = 0; k < 14; ++k) {                                         // the ring of shards
                float a = k * 0.4487989f + 0.2f * std::sin(k * 3.7f);
                glm::vec3 d = x * std::cos(a) + y * std::sin(a);
                glm::vec3 p0 = c + d * (0.03f + 0.26f * e) + fling * (0.12f * e), p1 = p0 + d * (0.05f * g + 0.01f);
                b.lit(glm::mix(hot, glow * 2.f, hu) * (2.5f * g));
                gunkit::limb(b, p0, p1, 0.01f * g + 0.002f, 0.005f * g + 0.0015f, z, glow);
            }
            for (int k = 0; k < 12; ++k) {                                         // sparks, flung along the swing and falling
                float a = k * 2.399963f, sp = 0.35f + 0.3f * std::fmod(k * 0.618f, 1.f);
                glm::vec3 d = glm::normalize(x * std::cos(a) * 0.6f + y * std::sin(a) * 0.6f + fling * 1.1f + z * 0.3f);
                glm::vec3 p = c + d * (sp * e) - glm::vec3{0.f, 0.25f * hu * hu, 0.f};
                b.lit(hot * (3.f * g));
                b.box(glm::translate(glm::mat4(1.f), p), {0.f, 0.f, 0.f}, glm::vec3{0.014f * g + 0.003f}, glm::vec3{1.f});
            }
            b.unlit();
        }
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
