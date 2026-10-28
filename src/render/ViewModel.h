#pragma once
#include <memory>
#include "render/MeshLib.h"
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
// It also draws your arms: on the sword, and the gauntlet that punches (their
// file, ArmFile.h; GunKit's recipes if there's none). The camera's own
// bob, shake and kicks live in CameraFeel.h.
// =============================================================================
#include "core/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Camera.h"
#include "core/ShaderProgram.h"
#include "core/Mesh.h"
#include "render/GunKit.h"
#include "render/ArmFile.h"
#include "player/Sword.h"
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
    void cancelReload() { motion.stopReload(); }
    void reset(int gun) { motion.reset(gun); parryTimer = grenadeT = grappleT = 0.f; }
    void triggerBolt(float t)     { motion.cycle(std::max(t, 0.2f)); }
    void triggerPump()            { motion.cycle(0.45f); }
    void triggerSwitch(int toGun) { motion.switchTo(toGun); }
    void triggerInspect()         { motion.inspect(); }
    void triggerGrenade()         { grenadeT = GRENADE_TIME; }
    void triggerGrapple()         { grappleT = GRAPPLE_TIME; }
    // Punch / parry: the gauntlet drives forward from the lower left, alongside
    // whatever the gun is doing
    // FAN (the revolver's alt fire): the left hand comes over the gun and slaps the hammer back, once a shot
    void triggerFan(int shots) { fanT = 0.f; fanLeft = std::max(0, shots); fanStruck = 0; fanAge = 0.f;
                                 handFan.cur = armfile::handRelaxed(); handFan.vel = {}; handFan.primed = true; }
    void fanStrike() { if (fanT < 0.f) triggerFan(1); fanAge = 0.f; ++fanStruck; fanLeft = std::max(0, fanLeft - 1); handFan.jolt(2.5f); }
    // Dev: hold the fanning hand p (0..1) of the way round between two slaps
    // stage 0: round the slap (p 0..1), 1: coming in (p of the way), 2: put away after the last (p of the way)
    void devFan(float p, int stage = 0) {
        if (fanT < 0.f) triggerFan(6);
        if (stage == 1) { fanStruck = 0; fanLeft = 6; fanT = fanAge = p * FAN_ENTER; return; }
        if (stage == 2) { fanT = 1.f; fanStruck = 6; fanLeft = 0; fanAge = FAN_PERIOD * 0.45f + p * FAN_EXIT; return; }
        fanT = 1.f; fanStruck = 2; fanLeft = 3; fanAge = p * FAN_PERIOD;
    }
    static constexpr glm::vec3 FAN_SHOULDER{-0.34f, -0.64f, 0.06f};   // (low on the left: the forearm comes in from the corner, under the hand)
    static constexpr float FAN_PERIOD = 0.075f, FAN_ENTER = 0.1f, FAN_EXIT = 0.22f;
    void triggerParry(bool hit)   { parryTimer = PARRY_TIME; parryHit = hit; parryJolted = false; handParry.cur = armfile::handRelaxed(); handParry.vel = {}; handParry.primed = true; }

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
        // the cut lands where the blade crosses the middle (a hit's already counted on the click): the blow jars it
        if (s.swinging() && swordU < 0.38f && u >= 0.38f && swordLanded) { swordLanded = false; jar(0.8f); }
        if (!s.swinging()) swordLanded = false;
        swordStroke = s.swinging() ? s.lastStroke : -1; swordU = u;
        // The wrist: the blade chases the hand's pose on a loose spring, so it trails into a cut, whips
        // through and overshoots a hair at the end (a guard snaps in stiffer)
        {
            const float w = s.blocking() ? 62.f : s.swinging() ? 40.f : 30.f, Z = s.blocking() ? 0.8f : 0.5f, H = 1.f / 240.f;
            if (!whipPrimed) { bladeW = swordShown.blade; edgeW = swordShown.edge; whipPrimed = true; }
            for (float left = std::min(dt, 0.1f); left > 1e-6f; left -= H) {
                float h = std::min(left, H);
                bladeV += (w * w * (swordShown.blade - bladeW) - 2.f * Z * w * bladeV) * h; bladeW += bladeV * h;
                edgeV  += (w * w * (swordShown.edge - edgeW) - 2.f * Z * w * edgeV) * h;  edgeW += edgeV * h;
            }
            if (glm::length(bladeW) > 1e-4f) bladeW = glm::normalize(bladeW);
            if (glm::length(edgeW) > 1e-4f) edgeW = glm::normalize(edgeW);
            jarT = std::max(0.f, jarT - dt);
        }
        // The hands: the sword hand closes harder through a cut (and eases off after); the off hand
        // takes the grip under it, lies flat on the blade in a guard, or hangs loose out of sight
        {
            armfile::HandPose r = armfile::handGrip();
            float sq = s.swinging() ? std::sin(3.14159f * std::clamp((u - 0.05f) / 0.7f, 0.f, 1.f)) : 0.f;
            sq = std::max(sq, s.blocking() ? 0.6f : 0.f);
            for (int f = 1; f < armfile::FINGERS; ++f) { r.curl[f][0] += 6.f * sq; r.curl[f][1] += 9.f * sq; r.curl[f][2] += 7.f * sq; }
            r.curl[0][1] += 8.f * sq; r.curl[0][2] += 10.f * sq;
            if (!s.swinging() && !s.blocking()) for (int f = 3; f < armfile::FINGERS; ++f) r.curl[f][1] -= 6.f;   // the little fingers loosen at rest
            handR.step(r, dt, s.swinging() ? 1.8f : 1.f);
            const SwordPose& Q = swordShown;
            float off = std::max(Q.twoHand, Q.brace);
            armfile::HandPose l = off < 0.35f ? armfile::handRelaxed() : Q.brace > Q.twoHand ? armfile::handFlat() : armfile::handGrip();
            if (off >= 0.35f && Q.brace <= Q.twoHand) for (int f = 1; f < armfile::FINGERS; ++f) l.curl[f][1] += 6.f * sq;
            handL.step(l, dt, 1.3f);
        }
    }
    // A sword blow landed (on the click: it's shown as the blade crosses the middle) or was blocked / a thrust struck (now)
    void swordImpact(bool now) { if (now) jar(1.f); else swordLanded = true; }
    void jar(float k) { jarT = 0.14f; jarK = k; handR.jolt(4.f * k); handL.jolt(3.f * k); }
    void drawSword(ShaderProgram& shader, const Camera& cam, const Sword& s) {
        SwordPose P = swordShown;
        P.blade = bladeW; P.edge = edgeW;   // (the wrist's whip)
        if (jarT > 0.f) {   // a blow jars it: a hard shudder through the blade, dying fast
            float j = jarT / 0.14f, wob = std::sin(jarT * 190.f) * j * j * jarK;
            P.blade = glm::normalize(P.blade + glm::vec3{0.07f, 0.05f, 0.f} * wob);
            P.hand += glm::vec3{0.006f, -0.008f, -0.01f} * (j * jarK) + glm::vec3{0.003f, 0.003f, 0.f} * wob;
        }
        float calm = swordStroke < 0 && !s.blocking() ? 1.f : 0.f;   // a slow breath on it at rest
        P.hand += calm * glm::vec3{std::sin(bobTimer * 1.3f) * 0.004f, std::sin(bobTimer * 0.9f) * 0.006f, 0.f};
        { glm::vec3 o{0.f}; float pi = 0.f, ya = 0.f, ro = 0.f; applySway(o, pi, ya, ro, 0.f); P.hand += o * 1.4f; }   // (the blade sways with the hand)
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
        for (const auto& p : parts) drawPart(shader, base * frame * S * p.xf, p.color, p.emissive, p.mesh);
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
        armfile::ArmLook arm; arm.seam = glow;
        arm.hand = handR.shown(swordStroke < 0 && !s.blocking() ? 0.7f : 0.15f);
        gunkit::drawArm(b, glm::vec3{0.34f, -0.62f, 0.f} + twist, fist, 1.f, arm);
        float off = std::max(P.twoHand, P.brace);
        if (off > 0.02f) {
            glm::vec3 z{frame[2]}, y{frame[1]}, root{frame[3]};
            glm::vec3 onGrip = root + z * (gunkit::SWORD_OFFHAND_Z * gunkit::SWORD_SCALE);
            glm::vec3 onFlat = root + z * 0.3f - y * 0.008f;
            bool flat = P.brace > P.twoHand;
            glm::vec3 at = flat ? onFlat : onGrip;
            glm::vec3 away{-0.3f, -0.6f, 0.12f};   // below the screen
            // the left hand on the grip: its knuckles to the same edge as the right's (the frame turned over);
            // flat on the blade: the palm on the flat, the fingers toward the point
            glm::mat4 lf = frame * (flat ? glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3{0.f, 1.f, 0.f})
                                         : glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3{0.f, 0.f, 1.f}));
            lf[3] = glm::vec4(glm::mix(away, at, gunkit::smooth(std::min(off, 1.f))), 1.f);
            arm.open = flat;
            arm.hand = handL.shown(0.3f);
            gunkit::drawArm(b, glm::vec3{-0.34f, -0.62f, 0.f} + twist * 0.6f, lf, -1.f, arm);
        }
        for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive, p.mesh);
        if (parryTimer > 0.f) drawFist(shader, cam, gunkit::SWORD_GLOW);   // F still punches
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        lastEmissive = glm::vec3{-1.f};
        glEnable(GL_CULL_FACE);
    }
    // Dev (--overlay handPV): one right hand up close in pose P (0 grip 1 fist 2 flat 3 relaxed 4 spread 5 fan;
    // +6 in the gauntlet), seen from side V (0 its back, 1 its palm, 2 its knuckles, 3 its thumb's side)
    int devHand = -1, devHandView = 0;
    void drawDevHand(ShaderProgram& shader, const Camera& cam) {
        glm::vec3 fwd = cam.forward(), right = cam.right(), up = glm::normalize(glm::cross(right, fwd));
        glm::mat4 base(1.f);
        base[0] = glm::vec4(right, 0.f); base[1] = glm::vec4(up, 0.f); base[2] = glm::vec4(fwd, 0.f);
        base[3] = glm::vec4(cam.position, 1.f);
        const glm::vec3 at{0.f, -0.03f, 0.34f};
        glm::vec3 f{0.f, 1.f, 0.f}, bk{0.f, 0.f, -1.f};
        if (devHandView == 1) bk = {0.f, 0.f, 1.f};
        if (devHandView == 2) { f = {0.f, 0.f, -1.f}; bk = {0.f, 1.f, 0.f}; }
        if (devHandView == 3) bk = {1.f, 0.f, 0.f};
        glm::mat4 hf = armfile::handFrame(at, f, bk, 1.f);
        armfile::ArmLook L; L.gauntlet = devHand >= 6; L.glow = gunkit::SWORD_GLOW;
        const armfile::HandPose P[6] = {armfile::handGrip(), armfile::handFist(), armfile::handFlat(), armfile::handRelaxed(),
                                        armfile::handSpread(), armfile::handFan()};
        L.hand = P[devHand % 6];
        glClear(GL_DEPTH_BUFFER_BIT);
        parts.clear();
        gunkit::Builder b{parts, gunkit::SWORD_GLOW};
        gunkit::drawArm(b, at + glm::vec3{0.f, -0.5f, -0.25f}, hf, 1.f, L);
        for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive, p.mesh);
    }
    const SwordPose& swordPoseShown() const { return swordShown; }
    static constexpr float PARRY_TIME = 0.56f;   // (how long the punch is drawn; the parry window itself is the game's)
    float parryTimer = 0.f;
    bool  parryHit = false, parryJolted = false;
    int   reloadShells = 1;
    void setMove(bool dashing, bool sliding) { motion.setMove(dashing, sliding); }
    void land(float fallSpeed) { motion.land(fallSpeed); }
    std::vector<GunCue> takeCues() { std::vector<GunCue> c; c.swap(motion.cues); return c; }

    // ---- Sway (every frame, GameplayState::update): the gun is left behind by a turn
    // and swings past a little when you stop, leans into a strafe, pulls in at a
    // sprint, and bobs with the same stride as the camera (CameraFeel)
    glm::vec2 swayX{0.f}, swayV{0.f};      // yaw, pitch lag (degrees) and their speed
    float leanRoll = 0.f, leanX = 0.f, pullZ = 0.f;
    float stridePh = 0.f, strideAmt = 0.f;
    // Down the sights: a small tip into a strafe (visual only: the shot goes where you aim)
    float leanSide = 0.f;                       // -1 strafing left .. 1 right, eased
    static constexpr float ADS_LEAN = 3.5f;     // degrees at a full strafe
    static constexpr float SWAY_PER_DPS = 0.011f, SWAY_MAX = 5.5f;   // degrees of lag per degree/s of turn, capped
    void feel(float dt, glm::vec2 lookDeg, float lateral, float forward, float stridePhase, float bobAmt) {
        if (dt <= 0.f) return;
        glm::vec2 rate = lookDeg / dt;
        glm::vec2 target = glm::clamp(-rate * SWAY_PER_DPS, glm::vec2(-SWAY_MAX), glm::vec2(SWAY_MAX));
        const float W = 13.f, Z = 0.62f, H = 1.f / 480.f;   // a little under critical: it swings past, once
        for (float t = dt; t > 1e-6f; t -= H) {
            float h = std::min(t, H);
            swayV += (W * W * (target - swayX) - 2.f * Z * W * swayV) * h;
            swayX += swayV * h;
        }
        float k = 1.f - std::exp(-dt * 10.f);
        float side = glm::clamp(lateral / 7.f, -1.5f, 1.5f);
        leanRoll += (-side * 2.6f - leanRoll) * k;   // the top of the gun tips into the strafe
        leanX    += (-side * 0.009f - leanX) * k;
        pullZ    += (-glm::clamp((forward - 7.f) / 15.f, 0.f, 1.f) * 0.025f - pullZ) * k;
        stridePh = stridePhase; strideAmt = bobAmt;
        leanSide += (glm::clamp(lateral / 7.f, -1.f, 1.f) - leanSide) * (1.f - std::exp(-dt * 8.f));
    }
    // What the sway adds to a pose (aim: 0 hip .. 1 aimed: almost none down the sights)
    void applySway(glm::vec3& offset, float& pitch, float& yaw, float& roll, float aim) const {
        float h = 1.f - 0.85f * aim;
        // Down the sights: the gun tips into a strafe (its own lean: the hip lean above fades out when aimed)
        float a = aim * aim * (3.f - 2.f * aim);
        roll     -= leanSide * ADS_LEAN * a;   // (- roll tips the top of the gun to the right)
        offset.x += leanSide * 0.0025f * a;
        yaw   += swayX.x * h;
        pitch += -swayX.y * h;
        roll  += (leanRoll + swayX.x * 0.6f + std::sin(stridePh) * 0.9f * strideAmt) * h;
        offset += glm::vec3{leanX - swayX.x * 0.0018f + std::sin(stridePh) * 0.007f * strideAmt,
                            swayX.y * 0.0015f - (1.f - std::fabs(std::sin(stridePh))) * 0.009f * strideAmt,
                            pullZ} * h;
    }

    void update(float dt, float xzSpeed, bool /*onGround*/) {
        motion.update(dt);
        parryTimer = std::max(0.f, parryTimer - dt);
        if (fanT >= 0.f) {
            fanT += dt; fanAge += dt;
            const float idle = fanStruck ? fanAge : fanT;
            const bool done = fanLeft == 0 || idle > FAN_PERIOD * 2.f;
            handFan.step(done && idle > FAN_PERIOD * 0.6f ? armfile::handRelaxed() : armfile::handFan(), dt, 2.2f);
            if (done && idle > FAN_PERIOD + FAN_EXIT) fanT = -1.f;
        }
        if (parryTimer > 0.f) {   // the parrying hand: loose, splayed as it cocks, balled tight for the blow, loosening as it drops
            float u = parryU();
            armfile::HandPose want = u < WHACK_AT * 0.4f ? armfile::handRelaxed()
                                   : u < WHACK_AT + 0.02f ? armfile::handSpread()
                                   : u < WHACK_HOLD + 0.08f ? armfile::handFist() : armfile::handRelaxed();
            handParry.step(want, dt, u >= WHACK_AT && u < WHACK_HOLD ? 3.f : 1.3f);
            if (parryHit && !parryJolted && u >= WHACK_CONTACT + 0.03f) { parryJolted = true; handParry.jolt(5.f); }   // the blow comes back up the arm
        }
        grenadeT = std::max(0.f, grenadeT - dt);
        grappleT = std::max(0.f, grappleT - dt);
        heat     = std::max(0.f, heat - dt * 0.7f);
        breathPh += dt * (0.3f + 1.4f * heat);   // a slow breath at rest (~3 s), panting as you fan it
        breathPh -= std::floor(breathPh);
        cylSpin *= std::exp(-dt * 22.f);          // the cylinder clicks round to the next chamber
        bobTimer += dt * (xzSpeed > 0.5f ? xzSpeed * 0.38f : 0.8f);
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
        if (fanT >= 0.f && g == 0) {   // fanning from the hip: the gun drops lower and out, canted toward the slapping hand
            float fp, fin; fanPhase(fp, fin);
            float k = fin * fin * (3.f - 2.f * fin);
            P.offset += glm::vec3{-0.055f, -0.035f, 0.14f} * k; P.roll += 16.f * k; P.pitch -= 3.f * k;
        }
        applySway(P.offset, P.pitch, P.yaw, P.roll, aim);

        GunLook L;
        L.ammo = ammo; L.mag = magSeen; L.lit = motion.litCells(ammo, magSeen);
        L.flash = flash; L.heat = heat; L.cylSpin = cylSpin; L.refill = reloadShells; L.aim = aim;
        L.breath = 0.5f + 0.5f * std::sin(breathPh * 6.2831853f);

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
            for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive, p.mesh);
        }
        if (fanT >= 0.f && g == 0 && !P.hidden) {
            glm::mat4 camBase(1.f);
            camBase[0] = glm::vec4(right, 0.f); camBase[1] = glm::vec4(up, 0.f); camBase[2] = glm::vec4(fwd, 0.f);
            camBase[3] = glm::vec4(cam.position, 1.f);
            drawFan(shader, camBase, glm::inverse(camBase) * base);
        } else if (g != 0) fanT = -1.f;
        if (parryTimer > 0.f) drawFist(shader, cam, gunkit::glowOf(g));
        if (devHand >= 0) drawDevHand(shader, cam);
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        lastEmissive = glm::vec3{-1.f};
        glEnable(GL_CULL_FACE);
    }

private:
    static constexpr float GRENADE_TIME = 0.5f, GRAPPLE_TIME = 0.28f, SETTLE = 0.22f;
    SwordPose swordShown = gunkit::swordRest(), swordFrom = gunkit::swordRest();
    float swordSince = 1e9f, swordU = -1.f;
    armfile::HandSpring handR, handL, handParry, handFan;   // the hands' fingers (ArmFile.h)
    glm::vec3 bladeW{0.f, 1.f, 0.f}, bladeV{0.f}, edgeW{0.f, 0.f, 1.f}, edgeV{0.f};   // the blade as the wrist carries it
    bool  whipPrimed = false, swordLanded = false;
    float fanT = -1.f, fanAge = 0.f;   // the fan: since it began (-1: not fanning), since the last slap
    int   fanLeft = 0, fanStruck = 0;   // slaps still to come, slaps so far
    float jarT = 0.f, jarK = 1.f;
    int   swordStroke = -1;
    float grenadeT = 0.f, grappleT = 0.f;
    float heat = 0.f;           // the revolver's heat stripe: hotter as you fan it
    float cylSpin = 0.f;        // degrees still to turn to the next chamber
    float breathPh = 0.f;       // the revolver's breath, 0..1 round
    int   magSeen = 8;
    std::vector<GunPart> parts; // this frame's boxes (kept: no allocation after the first)

    // The parry, in camera space: a backhand whack with the left gauntlet. It cocks across the body
    // (fist up by the right cheek, elbow high), then rips out along an arc through the crosshair and
    // wide off the left of the frame, the back of the hand leading; a parry jars it back toward the
    // centre, then it drops away. u: 0..1 through it. lead: the way the back of the hand faces.
    // flex: the wrist's bend (degrees: + curled back toward the palm, cocked for the blow; - snapped past straight)
    struct FistAt { glm::vec3 p; glm::vec3 lead; float out; float flex = 0.f; };
    static constexpr float WHACK_AT = 0.2f, WHACK_END = 0.44f, WHACK_CONTACT = 0.228f, WHACK_HOLD = 0.64f;
    static glm::vec3 whackArc(float t) {   // quadratic: cocked -> through the crosshair -> wide left
        const glm::vec3 a{0.12f, 0.015f, 0.25f}, c{0.f, -0.02f, 0.66f}, e{-0.34f, -0.1f, 0.46f};   // (it ends in frame: the arm stays in view)
        return (1.f - t) * (1.f - t) * a + 2.f * (1.f - t) * t * c + t * t * e;
    }
    static glm::vec3 whackLead(float t) {  // the arc's tangent, tipped a little forward
        const glm::vec3 a{0.12f, 0.015f, 0.25f}, c{0.f, -0.02f, 0.66f}, e{-0.34f, -0.1f, 0.46f};
        return glm::normalize(2.f * (1.f - t) * (c - a) + 2.f * t * (e - c) + glm::vec3{0.f, 0.f, 0.15f});
    }
    static FistAt fistAt(float u, bool hit) {
        const glm::vec3 rest{-0.4f, -0.62f, 0.12f};
        auto sm = [](float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
        if (u < WHACK_AT) {   // cock it: up and across, the last stretch pulling back a hair further
            float k = sm(u / WHACK_AT), pull = std::sin(k * 3.14159f) * 0.03f;
            return {glm::mix(rest, whackArc(0.f) + glm::vec3{0.02f, 0.f, -0.02f}, k) + glm::vec3{pull, pull, 0.f},
                    glm::normalize(glm::mix(glm::vec3{0.f, 1.f, 0.f}, whackLead(0.f), k)), 0.f, 45.f * k};
        }
        if (u < WHACK_END) {  // rip it out: through the mark early, overshoot the end and settle
            float x = (u - WHACK_AT) / (WHACK_END - WHACK_AT) - 1.f, t = 1.f + x * x * (2.6f * x + 1.6f);
            float s = (u - WHACK_AT) / (WHACK_END - WHACK_AT);   // the wrist stays cocked a beat, then cracks through past straight
            float flex = s < 0.25f ? 45.f - 20.f * (s / 0.25f) : 25.f - 45.f * sm((s - 0.25f) / 0.35f);
            return {whackArc(t), whackLead(std::min(t, 1.f)), std::min(t * 1.6f, 1.f), flex};
        }
        const glm::vec3 end = whackArc(1.f), lead = whackLead(1.f);
        if (u < WHACK_HOLD) { // hold the follow-through; a parry jars it back toward the centre
            float h = (u - WHACK_END) / (WHACK_HOLD - WHACK_END);
            float jolt = hit ? std::sin(std::min(h * 2.5f, 1.f) * 3.14159f) : 0.f;
            float buzz = hit ? std::sin(h * 70.f) * 0.006f * (1.f - h) : 0.f;
            return {end + glm::vec3{0.07f * jolt, 0.02f * jolt + buzz, -0.03f * jolt - 0.02f * h}, lead, 1.f, -20.f * (1.f - sm(h)) + 25.f * jolt};
        }
        float k = sm((u - WHACK_HOLD) / (1.f - WHACK_HOLD));
        return {glm::mix(end + glm::vec3{0.f, 0.f, -0.02f}, rest + glm::vec3{0.f, -0.12f, 0.f}, k),
                glm::normalize(glm::mix(lead, glm::vec3{0.f, 1.f, 0.f}, k)), 1.f - k, 20.f * k};   // (the hand drops, limp at the wrist)
    }
    float parryU() const { return 1.f - parryTimer / PARRY_TIME; }
    // the fist's frame in camera space: +Z on along the forearm (from the shoulder), +Y the back of
    // the hand turned to lead
    static glm::mat4 fistFrame(const FistAt& F) {
        // the forearm runs from the shoulder; the hand carries on along it, bent at the wrist toward the palm
        // by `flex` (the back of the hand, leading, is +Y; the left hand: mirrored)
        glm::vec3 arm = glm::normalize(F.p - glm::vec3{-0.3f, -0.56f, -0.1f});
        glm::vec3 back = F.lead - arm * glm::dot(F.lead, arm);
        back = glm::length(back) < 1e-4f ? glm::vec3{0.f, 1.f, 0.f} : glm::normalize(back);
        const float a = glm::radians(F.flex);
        glm::vec3 fingers = arm * std::cos(a) - back * std::sin(a), b2 = back * std::cos(a) + arm * std::sin(a);
        return armfile::handFrame(F.p, fingers, b2, -1.f);
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
        armfile::ArmLook arm; arm.gauntlet = true; arm.seam = 0.8f + 2.5f * flare; arm.glow = glow; arm.flare = flare;
        arm.hand = handParry.shown(0.f);
        gunkit::drawArm(b, glm::vec3{-0.3f, -0.56f, 0.02f}, ff, -1.f, arm);
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
        for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive, p.mesh);
    }

    // Where the fanning hand is round its slap (p) and how far in it is (in: 0 away .. 1 on the gun)
    void fanPhase(float& p, float& in) const {
        auto sm = [](float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
        const float idle = fanStruck ? fanAge : fanT;
        const bool done = fanLeft == 0 || idle > FAN_PERIOD * 2.f;
        in = 1.f;
        if (!fanStruck) { p = 0.f; in = sm(fanT / FAN_ENTER); }                                                 // up into place, cocked
        else if (!done) p = std::min(fanAge / FAN_PERIOD, 0.99f);                                              // round again for the next
        else { p = std::min(fanAge / FAN_PERIOD, 0.5f); in = 1.f - sm((idle - FAN_PERIOD * 0.5f) / FAN_EXIT); }   // follow through, then down and away
    }
    // The fanning hand, as a wrist: the forearm holds still, low on the left, and the hand chops from it.
    // Its swing round one slap (p 0..1, the slap landing at 0), degrees: + cocked forward of the hammer,
    // - swept back past it. The slap is the fast part; the rest is the recovery and a beat held cocked.
    static float fanSwing(float p, float* lift = nullptr) {
        auto sm = [](float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
        p = p - std::floor(p);
        const float COCK = 34.f, PAST = -22.f;
        float a, l = 0.f;
        if (p < 0.14f)      { float t = p / 0.14f; a = PAST * (1.f - (1.f - t) * (1.f - t)); }                 // the follow-through, slowing
        else if (p < 0.72f) { float t = sm((p - 0.14f) / 0.58f); a = PAST + (COCK - PAST) * t; l = std::sin(t * 3.14159f); }   // back up and over, lifted clear
        else if (p < 0.82f) { a = COCK; }                                                                     // held, cocked
        else                { float t = (p - 0.82f) / 0.18f; a = COCK * (1.f - t * t * t); }                   // the slap: accelerating in
        if (lift) *lift = l;
        return a;
    }
    // The hand's frame (camera space): swing a (degrees), lifted by lift (0..1), `in` of the way up onto the gun
    glm::mat4 fanHand(float a, float lift, float in, const glm::mat4& gunCam) const {
        // gun space: the wrist left of the hammer and a little above, the fingers across its top, palm down
        const glm::vec3 wrist{-0.088f, 0.088f + 0.014f * lift, -0.068f};
        glm::mat4 sw = glm::rotate(glm::mat4(1.f), glm::radians(-a), glm::vec3{0.f, 1.f, 0.f});   // (+: the fingers forward)
        glm::vec3 f = glm::vec3(sw * glm::vec4(glm::normalize(glm::vec3{1.f, -0.08f, 0.12f}), 0.f));
        glm::vec3 bk = glm::vec3(sw * glm::vec4(glm::normalize(glm::vec3{-0.15f, 1.f, 0.f}), 0.f));
        // the fingertips tip up as it comes over, and press down into the slap
        glm::vec3 side = glm::normalize(glm::cross(f, bk));
        glm::mat4 tip = glm::rotate(glm::mat4(1.f), glm::radians(12.f * lift + 10.f * std::max(0.f, a / 34.f) - 6.f * std::max(0.f, -a / 22.f)), side);
        f = glm::vec3(tip * glm::vec4(f, 0.f)); bk = glm::vec3(tip * glm::vec4(bk, 0.f));
        glm::mat3 G(gunCam);
        glm::vec3 fC = glm::normalize(G * f), bC = glm::normalize(G * bk), wC = glm::vec3(gunCam * glm::vec4(wrist, 1.f));
        // in and out: up from (and back down out of) the bottom of the screen, the hand dropping limp
        const float d = 1.f - in;
        wC += glm::vec3{-0.05f, -0.22f, -0.03f} * d;
        fC = glm::normalize(fC + glm::vec3{-0.2f, -1.1f, 0.1f} * d);
        glm::mat4 hf = armfile::handFrame(glm::vec3{0.f}, fC, bC, -1.f);
        const glm::vec3 wristLocal{-armfile::WRIST.x, armfile::WRIST.y, armfile::WRIST.z};   // (the mirrored hand's)
        hf[3] = glm::vec4(wC - glm::mat3(hf) * wristLocal, 1.f);
        return hf;
    }
    void drawFan(ShaderProgram& shader, const glm::mat4& camBase, const glm::mat4& gunCam) {
        float p, in;
        fanPhase(p, in);
        if (in <= 0.001f) return;
        const float inE = in * in * (3.f - 2.f * in);
        // before the first slap and after the last the hand waits cocked / settles from its follow-through
        float lift = 0.f, a = fanSwing(p, &lift);
        if (!fanStruck) { a = 34.f; lift = 0.f; }
        glm::mat4 hf = fanHand(a, lift, inE, gunCam);
        parts.clear();
        const glm::vec3 glow = gunkit::glowOf(0);
        gunkit::Builder b{parts, glow};
        armfile::ArmLook arm; arm.glow = glow; arm.hand = handFan.shown(0.f);
        gunkit::drawArm(b, FAN_SHOULDER, hf, -1.f, arm);
        // the slap: a thin streak of the revolver's light where the fingers swept over the hammer
        if (inE > 0.9f && fanStruck > 0 && p < 0.16f) {
            const float fade = 1.f - p / 0.16f;
            const glm::vec4 onFingers{-0.035f, 0.004f, 0.f, 1.f};   // (frame space: out along the fingers, on the palm)
            glm::vec3 prev = glm::vec3(hf * onFingers);
            for (int k = 1; k <= 6; ++k) {
                float q = p - 0.03f * k, l2 = 0.f, a2 = fanSwing(q, &l2);
                glm::vec3 was = glm::vec3(fanHand(a2, l2, 1.f, gunCam) * onFingers);
                float w = (1.f - k / 7.f) * fade;
                b.lit(glow * (1.8f * w));
                gunkit::limb(b, prev, was, 0.008f * w + 0.002f, 0.003f, glm::vec3{0.f, 0.f, 1.f}, glow);
                prev = was;
            }
            b.unlit();
        }
        for (const auto& q : parts) drawPart(shader, camBase * q.xf, q.color, q.emissive, q.mesh);
    }

    void drawPart(ShaderProgram& shader, const glm::mat4& model, glm::vec3 color, glm::vec3 emissive, int mesh = 0) {
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
        if (emissive != lastEmissive) { shader.setVec3("emissiveColor", emissive); lastEmissive = emissive; }   // most parts share it
        drawShape(mesh);
    }
public:
    // A part's shape: a MeshLib mesh (a bevelled plate, a barrel: uploaded the first time it's drawn,
    // centred at its own size) or the unit cube
    void drawShape(int mesh) {
        const meshlib::MeshData* md = mesh > 0 ? meshlib::get(mesh) : nullptr;
        if (!md) { cubeMesh.draw(); return; }
        if ((int)meshes.size() < mesh) meshes.resize((size_t)mesh);
        auto& g = meshes[(size_t)mesh - 1];
        if (!g) {
            std::vector<Vertex> v; v.reserve(md->pos.size());
            for (size_t i = 0; i < md->pos.size(); ++i)   // (uv 0.5: no box-edge darkening on a curved part)
                v.push_back({md->pos[i], {0.5f, 0.5f}, i < md->nrm.size() ? md->nrm[i] : glm::vec3{0.f, 1.f, 0.f}, {1.f, 1.f, 1.f}});
            std::vector<unsigned int> idx(md->idx.begin(), md->idx.end());
            g = std::make_unique<Mesh>();
            g->upload(v, idx);
        }
        g->draw();
    }
    std::vector<std::unique_ptr<Mesh>> meshes;   // [id - 1]
private:
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
