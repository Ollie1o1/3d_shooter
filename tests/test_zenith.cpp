// Headless tests for the encore: the sword (Sword.h), the Zenith, THE
// ABSOLUTE's mind and hazards, and his fight against scripted players.
// Built and run by `make test` after test_game.
#include "player/Sword.h"
#include "core/Balance.h"
#include "render/GunKit.h"
#include "render/ArmFile.h"
#include "world/LevelAct2.h"
#include "audio/MusicSynth.h"
#include "game/Progression.h"
#include "game/WaveDirector.h"
#include "bosses/absolute/AbsoluteHazards.h"
#include "render/EnemyModel.h"
#include "player/Projectile.h"
#include "world/ArenaShifts.h"
#include "bosses/sovereign/SovereignHazards.h"
#include "bosses/warden/WardenHazards.h"
#include "bosses/leviathan/LeviathanHazards.h"
#include "BossSim.h"
#include "audio/MixTable.h"
#include "audio/VoiceTable.h"
#include "ui/PixelFont.h"
#include "bosses/absolute/AbsoluteFeel.h"
#include <set>
#include <algorithm>
#include <fstream>
#include <string>
#include <cstdio>
#include <cmath>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)
static constexpr float DT = 1.f / 60.f;
static constexpr float AB_ORBIT_TEST_LO = 13.f;   // (a metre under the 14 m he circles at)

// Hold an input for t seconds (a press only on the first tick)
static Sword::Out run(Sword& s, SwordInput in, float t) {
    Sword::Out last; bool first = true;
    for (float u = 0.f; u < t - 1e-5f; u += DT) {
        Sword::Out o = s.update(DT, in);
        if (o.stroke >= 0) last.stroke = o.stroke;
        if (o.thrust) last.thrust = true;
        if (first) { in.swingPressed = in.guardPressed = false; first = false; }
    }
    return last;
}

int main() {
    balanceInit();   // the numbers as assets/balance.txt ships them (core/Balance.h)
    // ---------------------------------------------------------------- the sword: strokes
    {
        Sword s;
        SwordInput click{true, true, false, false};
        Sword::Out o = s.update(DT, click);
        CHECK(o.stroke == 0 && s.swinging(), "a click starts the first stroke at once");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f) < 1e-4f && std::fabs(s.strokeDamage(2) - 120.f) < 1e-4f,
              "strokes do 70, 70 and 120");
        run(s, {false, false, false, false}, Sword::SWING_TIME);
        o = s.update(DT, click);
        CHECK(o.stroke == 1, "a second click inside the combo window is the second stroke");
        run(s, {false, false, false, false}, Sword::SWING_TIME + Sword::COMBO_RESET + 0.05f);
        o = s.update(DT, click);
        CHECK(o.stroke == 0, "after 0.6 s without a swing the combo starts over");
    }
    {
        Sword s;
        s.update(DT, {true, true, false, false});
        Sword::Out o = s.update(DT, {true, true, false, false});   // clicked again mid-stroke
        CHECK(o.stroke < 0, "a click mid-stroke doesn't cut the stroke short");
        Sword::Out later = run(s, {false, false, false, false}, Sword::SWING_TIME);
        CHECK(later.stroke == 1, "...it's buffered: the next stroke follows when this one ends");
    }
    // ---------------------------------------------------------------- the sword: thrust
    {
        Sword s;
        run(s, {true, true, false, false}, Sword::CHARGE_TIME + 0.05f);
        Sword::Out o = s.update(DT, {false, false, false, false});
        CHECK(o.thrust && std::fabs(s.thrustDamage() - 220.f) < 1e-4f, "hold 0.5 s and release: a thrust for 220");
        Sword q;
        run(q, {true, true, false, false}, 0.3f);
        o = q.update(DT, {false, false, false, false});
        CHECK(!o.thrust, "released before 0.5 s: no thrust");
        Sword h;
        run(h, {true, true, false, false}, Sword::CHARGE_TIME + 0.05f);
        h.holster();
        o = h.update(DT, {false, false, false, false});
        CHECK(!o.thrust, "holstering mid-charge never fires a thrust");
    }
    // ---------------------------------------------------------------- the sword: parry, spam lock, riposte
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        CHECK(s.parryOpen(), "raising the guard opens a parry window");
        CHECK(s.receive(Sword::Strike::WHITE, true) == Sword::Guarded::PARRIED, "a white strike in the window is parried");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f * Sword::RIPOSTE_MULT) < 1e-3f, "the next stroke is a riposte at x2.5");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f) < 1e-3f, "...and only that one");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.12f);   // 8 more ticks: 0.133 s raised
        CHECK(s.parryOpen(), "the window is still open at 0.13 s");
        run(s, {false, false, false, true}, 0.03f);   // 0.167 s
        CHECK(!s.parryOpen() && s.blocking(), "closed by 0.17 s: still blocking");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.2f);          // missed
        run(s, {false, false, false, false}, 0.1f);         // lowered
        s.update(DT, {false, false, true, true});           // raised again 0.1 s after the miss
        CHECK(!s.parryOpen() && s.blocking(), "a raise within 0.4 s of a missed parry blocks but can't parry");
        Sword t;
        t.update(DT, {false, false, true, true});
        run(t, {false, false, false, true}, 0.2f);
        run(t, {false, false, false, false}, 0.45f);
        t.update(DT, {false, false, true, true});
        CHECK(t.parryOpen(), "after 0.4 s the window opens again");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        CHECK(s.receive(Sword::Strike::GOLD, true) == Sword::Guarded::BLOCKED, "a gold strike can't be parried: blocked");
        CHECK(s.receive(Sword::Strike::WHITE, false) == Sword::Guarded::NONE, "a strike from behind isn't guarded");
        Sword p;
        p.update(DT, {false, false, true, true});
        CHECK(p.tryParry() && !p.tryParry(), "one window parries one thing (a projectile or a ray)");
    }
    // ---------------------------------------------------------------- the sword: block and guard break
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.2f);
        s.receive(Sword::Strike::WHITE, true);
        CHECK(std::fabs(s.guard - 75.f) < 1e-3f, "a blocked white strike drains 25");
        s.receive(Sword::Strike::GOLD, true);
        CHECK(std::fabs(s.guard - 15.f) < 1e-3f, "a blocked gold strike drains 60");
        CHECK(s.receive(Sword::Strike::WHITE, true) == Sword::Guarded::BROKE && s.broken() && !s.blocking(),
              "the meter empties: guard break");
        run(s, {false, false, false, true}, 0.5f);
        CHECK(!s.blocking(), "broken: the guard can't come up for a second");
        run(s, {false, false, false, false}, Sword::BREAK_TIME);
        CHECK(!s.broken(), "...and then it can");
        float g = s.guard;
        run(s, {false, false, false, false}, 1.0f + 1.0f);
        CHECK(s.guard > g + 25.f, "the guard refills (30/s after a second down)");
        CHECK(std::fabs(Sword::BLOCK_TAKEN - 0.25f) < 1e-6f && std::fabs(s.moveMult() - 1.f) < 1e-6f,
              "a block takes 25 %; lowered, you move at full speed");
        Sword b; b.update(DT, {false, false, true, true});
        b.receive(Sword::Strike::BEAM, true, 1.f);
        CHECK(std::fabs(b.guard - 85.f) < 1e-3f && std::fabs(b.moveMult() - Sword::BLOCK_SPEED) < 1e-6f,
              "a beam drains 15 a second; blocking you move at 60 %");
        Sword n; n.update(DT, {false, false, true, true});
        Sword::Out o = n.update(DT, {true, true, false, true});
        CHECK(o.stroke < 0, "no swings while blocking");
    }
    // ---------------------------------------------------------------- the sword: reach, arc, thrust path, lock-on
    {
        glm::vec3 eye{0, 1.7f, 0}, fwd{0, 0, -1};
        CHECK(Sword::inArc(eye, fwd, {0, 1.2f, -3.f}, 0.5f), "a target 3 m ahead is in reach");
        CHECK(!Sword::inArc(eye, fwd, {0, 1.2f, -4.5f}, 0.5f), "4.5 m ahead is out of reach");
        CHECK(Sword::inArc(eye, fwd, {1.8f, 1.2f, -1.8f}, 0.5f), "45 degrees off the crosshair is in the arc");
        CHECK(!Sword::inArc(eye, fwd, {2.5f, 1.2f, 0.5f}, 0.5f), "beside and behind is not");
        CHECK(Sword::thrustHits({0, 1, 0}, {0, 1, -8}, {0.8f, 1, -5}, 0.5f), "a thrust hits what's along its path");
        CHECK(!Sword::thrustHits({0, 1, 0}, {0, 1, -8}, {3.f, 1, -5}, 0.5f), "...not what's 3 m to the side");
        glm::vec3 st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, -5}, 0.8f);
        CHECK(std::fabs(glm::length(st) - Sword::STEP_DIST) < 1e-3f && st.z < 0.f, "lock-on: a stroke carries you 2 m toward him");
        st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, -2.f}, 0.8f);
        CHECK(glm::length(st) < 0.25f, "...but never into him");
        st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, 5}, 0.8f);
        CHECK(glm::length(st) < 1e-4f, "...and not toward something behind you");
    }

    // ---------------------------------------------------------------- the sword: its look and motion
    {
        std::vector<GunPart> parts;
        auto lit = [&](float g, bool broken) {
            parts.clear(); SwordLook L; L.guard01 = g; L.broken = broken;
            gunkit::buildSword(L, parts);
            int n = 0; for (auto& p : parts) n += p.tag == 1; return n;
        };
        CHECK(lit(1.f, false) == 8 && lit(0.5f, false) == 4 && lit(0.01f, false) == 1, "the fuller's 8 cells read the guard meter");
        CHECK(lit(1.f, true) == 0, "a broken guard: the fuller goes dark");
        parts.clear(); gunkit::buildSword(SwordLook{}, parts);
        glm::vec3 lo{1e9f}, hi{-1e9f};
        for (auto& p : parts) { glm::vec3 c{p.xf[3]}; lo = glm::min(lo, c); hi = glm::max(hi, c); }
        CHECK(hi.z > 0.85f && hi.z < 1.f && lo.z > -0.3f && hi.x - lo.x < 0.25f, "a blade about 0.9 m long past the guard, a hand-and-a-half grip, slim");
        bool glows = false;
        for (auto& p : parts) if (p.tag == 1 && glm::length(p.emissive - gunkit::SWORD_GLOW * 1.6f) < 0.05f) glows = true;
        CHECK(glows, "its cells glow rose-red");
        SwordPose a = gunkit::swordPose(0, 0.f, false, 0.f, 0.f), b = gunkit::swordPose(0, 1.f, false, 0.f, 0.f);
        CHECK(a.hand.x > 0.1f && b.hand.x < -0.1f, "stroke 0 sweeps from right to left");
        a = gunkit::swordPose(1, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(1, 1.f, false, 0.f, 0.f);
        CHECK(a.hand.x < -0.1f && b.hand.x > 0.1f, "stroke 1 sweeps from left to right");
        a = gunkit::swordPose(2, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(2, 1.f, false, 0.f, 0.f);
        CHECK(a.hand.y > b.hand.y + 0.2f, "stroke 2 comes down overhead");
        CHECK(a.twoHand > 0.99f && b.twoHand > 0.99f && gunkit::swordPose(0, 0.5f, false, 0.f, 0.f).twoHand < 0.01f,
              "the overhead is two-handed; the side cuts one");
        SwordPose g = gunkit::swordPose(-1, -1.f, true, 0.f, 0.f), r = gunkit::swordPose(-1, -1.f, false, 0.f, 0.f);
        CHECK(std::fabs(g.hand.x) < std::fabs(r.hand.x) && std::fabs(g.blade.x) > 0.9f && g.brace > 0.99f,
              "raised: the blade across the middle of the screen, the off hand braced on it");
        // Where the blade's point is on screen (camera space: x right, y up, z forward)
        auto tip = [](int stroke, float u) { return gunkit::swordTip(gunkit::swordPose(stroke, u, false, 0.f, 0.f)); };
        CHECK(tip(2, 0.f).y > tip(2, 1.f).y + 0.6f && tip(2, 1.f).z > 0.2f,
              "the overhead chops down: the point starts high and ends low in front (as its trail does)");
        bool down = true;
        for (int k = 1; k <= 20; ++k) down &= tip(2, k / 20.f).y <= tip(2, (k - 1) / 20.f).y + 1e-4f;
        CHECK(down, "...and never flicks back up on the way");
        CHECK(tip(0, 1.f).x < tip(0, 0.f).x - 0.4f, "the first stroke's point sweeps right to left");
        CHECK(tip(1, 1.f).x > tip(1, 0.f).x + 0.4f, "the second's sweeps left to right");
        bool across = true;   // through the cut the blade shows its length: never pointing straight into the screen for long
        for (int st = 0; st < 2; ++st) {
            int endOn = 0;
            for (int k = 0; k <= 40; ++k) {
                SwordPose P = gunkit::swordPose(st, k / 40.f, false, 0.f, 0.f);
                if (glm::length(glm::vec2{P.blade.x, P.blade.y}) < 0.35f) ++endOn;
            }
            across &= endOn <= 4;   // (a tenth of the stroke at most)
        }
        CHECK(across, "the side cuts sweep the blade across the screen, not end-on");
        bool smooth = true;   // no jumps between keyframes: the hand moves at most 6 cm in a 60th of the stroke
        for (int st = 0; st < 3; ++st)
            for (int k = 1; k <= 60; ++k)
                smooth &= glm::length(gunkit::swordPose(st, k / 60.f, false, 0.f, 0.f).hand - gunkit::swordPose(st, (k - 1) / 60.f, false, 0.f, 0.f).hand) < 0.06f;
        CHECK(smooth, "every stroke moves the hand continuously");
        bool square = true;   // the sword's frame stays a rotation (the edge kept square to the blade)
        for (int st = 0; st < 3; ++st)
            for (int k = 0; k <= 20; ++k) {
                glm::mat3 m{gunkit::swordFrame(gunkit::swordPose(st, k / 20.f, false, 0.f, 0.f))};
                square &= std::fabs(glm::determinant(m) - 1.f) < 1e-3f && std::fabs(glm::dot(m[1], m[2])) < 1e-3f;
            }
        CHECK(square, "the sword's frame is a true rotation all through every stroke");
        // the hand's frame is the sword's: its knuckles (+X) face the edge, and the side cuts lead with it
        bool leads = true;
        for (int st = 0; st < 2; ++st)
            for (int k = 30; k <= 50; k += 5) {   // (through the cut itself)
                float u = k / 100.f;
                glm::vec3 v = gunkit::swordTip(gunkit::swordPose(st, u + 0.01f, false, 0.f, 0.f)) - gunkit::swordTip(gunkit::swordPose(st, u - 0.01f, false, 0.f, 0.f));
                glm::vec3 x{gunkit::swordFrame(gunkit::swordPose(st, u, false, 0.f, 0.f))[0]};
                leads &= glm::dot(glm::normalize(v), x) > 0.5f;
            }
        CHECK(leads, "the side cuts lead with the edge, not the flat");
        // a wrist: the knuckles never face far from where the forearm points, and the roll it
        // makes the blade take stays smooth through every stroke
        {
            bool reach = true, smoothRoll = true;
            for (int st = 0; st < 3; ++st) {
                glm::vec3 prev = armfile::swordHeld(gunkit::swordPose(st, 0.f, false, 0.f, 0.f)).edge;
                for (int k = 0; k <= 100; ++k) {
                    SwordPose P = armfile::swordHeld(gunkit::swordPose(st, k / 100.f, false, 0.f, 0.f), &prev);
                    smoothRoll &= st == 2 || glm::dot(P.edge, prev) > std::cos(glm::radians(12.f));   // (the overhead's rolls through its cock: the wrist spring takes it)
                    prev = P.edge;
                    if (P.twoHand > 0.5f) continue;
                    glm::mat4 fist = gunkit::swordFrame(P); fist[3] = glm::vec4(P.hand, 1.f);
                    glm::vec3 el, wr; armfile::chain(armfile::swordShoulder(P, 1.f), fist, 1.f, el, wr, glm::vec3{0.f}, true);
                    glm::vec3 d = glm::inverse(glm::mat3(fist)) * glm::normalize(el - wr);   // the forearm, in hand space
                    reach &= d.x < -std::cos(glm::radians(armfile::FLEX + 1.f));   // (straight out of the back of the hand: -X)
                }
            }
            CHECK(reach, "the sword hand's forearm always runs from the wrist within a wrist's bend");
            CHECK(smoothRoll, "the side cuts roll the blade smoothly");
            glm::vec3 w = armfile::wristReach({1.f, 0.f, 0.f});
            CHECK(w.x < 0.f, "a forearm asked to come from the fingers' side is brought round behind the wrist");
        }
        // the arm reaches the hand: its parts run from below the screen to the grip
        {
            std::vector<GunPart> arm; gunkit::Builder ab{arm, gunkit::SWORD_GLOW};
            SwordPose P = gunkit::swordPose(0, 0.6f, false, 0.f, 0.f);
            glm::mat4 fist = gunkit::swordFrame(P); fist[3] = glm::vec4(P.hand, 1.f);
            gunkit::buildArm(ab, {0.34f, -0.62f, 0.f}, fist, 1.f, false, 1.f);
            float lowest = 1e9f, nearHand = 1e9f;
            for (auto& q : arm) { glm::vec3 c{q.xf[3]}; lowest = std::min(lowest, c.y); nearHand = std::min(nearHand, glm::length(c - P.hand)); }
            CHECK(lowest < -0.35f && nearHand < 0.02f, "the arm runs from below the screen to the hand on the grip");
        }
    }

    // ---------------------------------------------------------------- the sword: where it can be drawn
    {
        CHECK(swordKeyAllowed(false, true, 5, 5), "key 5 in the Zenith draws the sword");
        CHECK(!swordKeyAllowed(false, true, 4, 5) && !swordKeyAllowed(false, false, 5, 5),
              "key 5 outside the Zenith keeps the gun");
        CHECK(swordKeyAllowed(true, false, 0, 5), "--sword allows it anywhere (dev)");
    }

    // ---------------------------------------------------------------- the Zenith
    LevelData N = buildAct2Level();
    SpatialGrid g; g.build(N.walls);
    const glm::vec3 ZC = zenithCentre();
    {
        CHECK(std::string(ZENITH_NAME) == "!!!!!" && PIXEL_FONT['!' - ' '][0] != 0, "the encore's name is !!!!! (and the pixel font can draw it)");
        bool gone = true;
        for (int i = 0; i < 6; ++i) gone &= std::string(abLine(i)) != "BE JUDGED.";
        CHECK(gone && abString(AB_JUDGED).line == -1, "\"BE JUDGED.\" is gone");
        CHECK(std::string(abLine(0)) == "KNEEL." && std::string(abLine(1)) == "TOO SLOW." && std::string(abLine(2)) == "AGAIN." &&
              std::string(abLine(AB_LINE_WORTHY)) == "...WORTHY.", "...and the other four lines stay");
    }
    {
        CHECK(N.arenas.size() == 6 && std::string(N.arenas[ZENITH_ARENA].name) == "!!!!!", "a sixth Act II arena: the encore, called !!!!!");
        const Arena& Z = N.arenas[ZENITH_ARENA];
        CHECK(Z.waves.size() == 1 && Z.waves[0][0].type == EnemyType::ABSOLUTE && Z.maxAlive == 1 && Z.space == ReverbSpace::OPEN,
              "one wave, THE ABSOLUTE, alone, under an open sky");
        CHECK(std::fabs(Z.voidY - (ZC.y - 40.f)) < 1e-3f && Z.hasRespawn, "the void below, a respawn on the floor");
        CHECK(N.arenaAt(Z.playerStart) == ZENITH_ARENA && N.arenaAt(zenithShard(3)) == ZENITH_ARENA,
              "the Zenith's start and shards belong to the Zenith (arenaAt is XZ-only: it must not overlap the Maw)");
        auto floorAt = [&](glm::vec3 p) { return N.groundAt(p.x, p.z, p.y + 3.f); };
        bool floorOk = true;
        for (int k = 0; k < 16; ++k) for (float r : {12.f, 25.f, 40.f}) {
            float a = k * 0.3926991f;
            glm::vec3 p = ZC + glm::vec3{std::cos(a) * r, 0.5f, std::sin(a) * r};
            if (N.zenithPillarAt(p, 0.5f) >= 0) continue;
            if (std::fabs(floorAt(p) - ZC.y) > 0.05f) floorOk = false;
        }
        CHECK(floorOk, "the floor is at the centre's height from r 9 to 44, all the way round");
        CHECK(std::fabs(floorAt(ZC + glm::vec3{0.f, 2.f, 3.f}) - (ZC.y + ZEN_DAIS_UP)) < 0.05f, "the Dais is raised a hand's breadth (0.15 m)");
        CHECK(floorAt(ZC + glm::vec3{48.f, 2.f, 0.f}) < ZC.y - 30.f, "a gap past the floor's edge (the void)");
        CHECK(N.zenith.pillarWalls.size() == 8 && N.zenith.shardWalls.size() == 6, "eight pillars, six shards");
        {
            const Theme& th = N.arenas[ZENITH_ARENA].theme;
            auto lum = [](glm::vec3 c) { return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z; };
            CHECK(lum(th.zenith) < 0.08f && th.zenith.z > th.zenith.x, "a near-black indigo sky overhead");
            CHECK(th.horizon.x > th.horizon.y * 2.f && th.horizon.x > th.horizon.z * 4.f, "a crimson-orange horizon");
            CHECK(th.sunDir.y > 0.99f && th.eclipse > 0.99f, "the eclipse straight overhead, drawn by the sky (out of reach)");
            CHECK(th.lightColor.x > th.lightColor.z * 2.5f && th.lightDir.y > -0.45f, "a warm, low key light (long shadows)");
            CHECK(lum(th.skyAmb) < 0.25f && th.skyAmb.z > th.skyAmb.y, "dim violet fill");
            CHECK(th.stars > 0.f, "a few stars");
        }
        bool shardsOk = true;
        for (int k = 0; k < 6; ++k) { glm::vec3 s = zenithShard(k); if (std::fabs(floorAt(s + glm::vec3{0, 3, 0}) - s.y) > 0.05f || !N.onZenithShard(s)) shardsOk = false; }
        CHECK(shardsOk, "each shard is a floor you can stand on");
        N.parkZenithPiece(false, 0);
        CHECK(N.zenithPillarAt(zenithPillar(0), 0.5f) < 0 && !N.zenith.pillarUp[0], "a shattered pillar is gone: no collision");
        N.parkZenithPiece(true, 2);
        CHECK(!N.onZenithShard(zenithShard(2)), "a fallen shard is gone");
        N.restoreZenith();
        CHECK(N.zenithPillarAt(zenithPillar(0), 0.5f) == 0 && N.onZenithShard(zenithShard(2)), "a retry puts them back");
        CHECK(MUSIC_TRACKS == 12 && std::string(musicTrack(10).name) == "ZENITH" && musicTrack(10).bpm >= 165.f, "the Zenith's own fast track");
        CHECK(std::string(musicTrack(11).name) == "MORNINGSTAR" && musicTrack(11).bpm > musicTrack(10).bpm && musicTrack(11).techno == 2,
              "MORNINGSTAR's: faster than the Zenith's, the harder voicing");
    }

    // ---------------------------------------------------------------- BOSS: every boss on its own
    {
        const char* want[] = {"THE WARDEN", "THE SOVEREIGN", "THE PENITENT", "THE LEVIATHAN", "THE ABSOLUTE", "MORNINGSTAR"};
        bool order = BOSS_COUNT == 6;
        for (int i = 0; i < BOSS_COUNT; ++i) order &= std::string(bossEntry(i).name) == want[i];
        CHECK(order, "BOSS lists every boss in the order you meet them, the Absolute's two forms last");
        LevelData A = buildLevel(), A2 = buildAct2Level();
        bool where = true;
        const EnemyType types[] = {EnemyType::WARDEN, EnemyType::SOVEREIGN, EnemyType::PENITENT, EnemyType::LEVIATHAN, EnemyType::ABSOLUTE, EnemyType::ABSOLUTE};
        for (int i = 0; i < BOSS_COUNT; ++i) {
            const BossEntry& b = bossEntry(i);
            const Arena& ar = (b.act2 ? A2 : A).arenas[b.arena];
            bool found = false;
            for (auto& w : ar.waves) for (auto& g : w) found |= g.type == types[i];
            if (!found) { where = false; std::printf("      boss %d: not in %s\n", i, ar.name); }
        }
        CHECK(where, "...and each one's arena is the one its boss fights in");
        CHECK(bossEntry(5).morning && !bossEntry(4).morning, "...MORNINGSTAR starts inside the eclipse; the Absolute's entry is his first form");
        Records r;
        bool allOpen = true;
        for (int i = 0; i < BOSS_COUNT; ++i) allOpen &= bossOpen(r, i);
        CHECK(allOpen, "every boss is open from the start, on a fresh save");
        CHECK(recordBoss(r, 2, 95.f) && !recordBoss(r, 2, 100.f) && recordBoss(r, 2, 90.f) && r.bestBoss[2] == 90.f, "a boss's best kill is its fastest");
        Records back; back.parse(r.serialise());
        CHECK(back.bestBoss[2] == 90.f && back.bestBoss[0] == 0.f, "...and it's saved");
        // The world record, as web/index.html caches a boss's board ("TIME DIFFICULTY NAME", fastest first)
        WorldBest w = parseWorldBest("84.31 2 OLLIE R\n90.5 1 SAM\n");
        CHECK(w.time > 84.3f && w.time < 84.32f && w.difficulty == 2 && w.name == "OLLIE R" && bossBoardKey(1) == "boss1",
              "a boss's world record is read from the shared board's cache (the fastest, with who holds it)");
        CHECK(parseWorldBest("").time == 0.f && parseWorldBest("junk\n").time == 0.f, "...and with no board (or a bad one) there's none to show");
    }

    // ---------------------------------------------------------------- the hand-off and the records
    {
        Records r;
        recordAct2Clear(r, 1500.f);
        CHECK(r.act2Cleared && std::fabs(r.bestAct2 - 1500.f) < 1e-3f, "the Leviathan's death saves the Act II clear and its time");
        recordAct2Clear(r, 1600.f);
        CHECK(std::fabs(r.bestAct2 - 1500.f) < 1e-3f, "a slower clear doesn't replace the best");
        CHECK(recordZenith(r, 1700.f) && !recordZenith(r, 1800.f) && std::fabs(r.bestZenith - 1700.f) < 1e-3f, "ACT II + ZENITH keeps its own best");
        CHECK(zenithHandoff(true, 4, 6) && !zenithHandoff(true, 3, 6) && !zenithHandoff(false, 4, 6) && !zenithHandoff(true, 4, 5),
              "only the Maw's clear in Act II hands off to the Zenith");
        CHECK(retryTarget(WaveDirector::Phase::CLEARED, 4) == ZENITH_ARENA && retryTarget(WaveDirector::Phase::ACTIVE, ZENITH_ARENA) == ZENITH_ARENA,
              "a death during the ascension retries the Zenith; a death in it, the Zenith again");
        Records w; w.act2Cleared = true; w.bestAct2 = 1234.5f; w.bestZenith = 1500.25f;
        std::string text = w.serialise();
        Records back; back.parse(text);
        CHECK(back.act2Cleared && std::fabs(back.bestAct2 - 1234.5f) < 0.01f && std::fabs(back.bestZenith - 1500.25f) < 0.01f,
              "the clear and both times survive a save and a load");
        // FAST times set on an older Gauntlet are dropped as they load; the rest of the save is kept
        Records old; old.parse("bestFast 400\nfastRooms 2 40 50\nfastMedals 2 3 4\nfastSplits 1 40\nbestAct2 900\n");
        Records cur; cur.bestFast = 500.f; cur.fastMedals = {3}; Records cur2; cur2.parse(cur.serialise());
        CHECK(old.bestFast == 0.f && old.fastRooms.empty() && old.fastMedals.empty() && old.fastSplits.empty() &&
              std::fabs(old.bestAct2 - 900.f) < 1e-3f && cur2.bestFast == 500.f && cur2.fastMedals.size() == 1,
              "FAST bests from an older Gauntlet are dropped (the rest kept); this one's survive a save and a load");
    }

    // ---------------------------------------------------------------- THE ABSOLUTE: phase 1
    const Arena& ZA = N.arenas[ZENITH_ARENA];
    std::vector<AABB> standing;
    auto zworld = [&](glm::vec3 feet) {
        standing.clear();
        for (size_t i = 0; i < N.zenith.pillarWalls.size(); ++i) if (N.zenith.pillarUp[i]) standing.push_back(N.walls[N.zenith.pillarWalls[i]].box);
        EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
        w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = ZA.bounds;
        w.hasZenith = true; w.zenCentre = ZC; w.zenFloor = ZC.y;
        w.zenPillars = standing.data(); w.zenPillarCount = (int)standing.size();
        return w;
    };
    auto freshAbs = [&]() { Enemy e(EnemyType::ABSOLUTE, ZA.bossSpawn); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE; return e; };
    {   // his entrance, from the spawn the arena gives him (not freshAbs: the real one)
        Enemy e(EnemyType::ABSOLUTE, ZA.bossSpawn);
        EnemyWorld w = zworld(ZC + glm::vec3{0.f, 0.f, 20.f});
        float first = -1e9f, prev = 1e9f; bool neverRises = true; int landed = 0; float landedAt = -1.f;
        for (int i = 0; i < 4 * 60; ++i) {
            e.update(DT, w);
            if (e.ev.abLanded) { ++landed; landedAt = e.position.y - absFloorAt(w, e.position); }
            if (i == 0) first = e.position.y;
            if (e.position.y > prev + 1e-4f) neverRises = false;
            prev = e.position.y;
        }
        CHECK(first > ZC.y + 10.f, "he enters from high above the Dais, from the first tick");
        CHECK(neverRises, "...and only comes down: never up again after he appears");
        CHECK(landed == 1 && std::fabs(landedAt) < 0.05f, "...and lands once, on the Dais (the landing's ring and the eclipse's flare)");
        CHECK((e.abArrive <= 0.f && std::fabs(e.position.y - absFloorAt(w, e.position)) < 0.6f) || e.abAirborne(),
              "...landing on the Dais");
    }
    {
        bool rule = true;
        for (int id = 0; id < AB_STRINGS; ++id) {
            const AbString& s = abString(id);
            for (int k = 0; k < s.n; ++k)
                if (s.steps[k].damage > 0.f && abTellOf(s.steps[k].k) == AbTell::NONE) rule = false;
        }
        CHECK(rule, "every damaging move in every string has a tell colour");
        CHECK(abTellOf(AttackKind::AJAB) == AbTell::WHITE && abTellOf(AttackKind::AHOOK) == AbTell::WHITE &&
              abTellOf(AttackKind::AUPPER) == AbTell::GOLD && abTellOf(AttackKind::ADROP) == AbTell::GOLD &&
              abTellOf(AttackKind::ARAY) == AbTell::RED && abTellOf(AttackKind::ASTOMP) == AbTell::RED &&
              abTellOf(AttackKind::ASERPENT) == AbTell::WHITE && abTellOf(AttackKind::ASWEEP) == AbTell::WHITE &&
              abTellOf(AttackKind::ALUNGE) == AbTell::GOLD && abTellOf(AttackKind::ACRESCENT) == AbTell::WHITE,
              "white: jab, hook, serpent, sweep, crescent; gold: uppercut, drop-kick, lunge; red: ray, stomp");
        float longest = 0.f; bool capped = true, singles = true;
        for (int id = 0; id < AB_STRINGS; ++id) {
            float sum = 0.f; const AbString& st = abString(id);
            for (int k = 0; k < st.n; ++k) {
                if (st.steps[k].minPhase > 1) continue;
                sum += st.steps[k].damage;
                if (st.steps[k].damage > 0.f && (st.steps[k].damage < 14.f || st.steps[k].damage > 30.f)) singles = false;
            }
            if (sum > 70.f) { capped = false; std::printf("      %s costs %.0f\n", st.name, sum); }
            longest = std::max(longest, sum);
        }
        CHECK(capped && longest >= 60.f, "no phase 1 string costs more than 70; the longest 60-70");
        CHECK(singles, "single hits 14-30");
    }
    {
        using A = AttackKind;
        CHECK(abTellOf(A::ACROSS) == AbTell::WHITE && abTellOf(A::ABACKFIST) == AbTell::WHITE && abTellOf(A::AELBOW) == AbTell::WHITE &&
              abTellOf(A::AROUNDHOUSE) == AbTell::WHITE && abTellOf(A::AHAYMAKER) == AbTell::GOLD && abTellOf(A::ACHARGE) == AbTell::GOLD &&
              abTellOf(A::AAXE) == AbTell::GOLD && abTellOf(A::ACLAP) == AbTell::RED,
              "white: cross, backfist, elbow, roundhouse; gold: haymaker, shoulder charge, axe kick; red: the clap");
        bool parry = true;
        for (A k : {A::ACROSS, A::ABACKFIST, A::AELBOW, A::AROUNDHOUSE}) parry &= Enemy::abMeleeWhite(k);
        for (A k : {A::AHAYMAKER, A::ACHARGE, A::AAXE, A::ACLAP}) parry &= !Enemy::abMeleeWhite(k);
        CHECK(parry, "...the whites parry, the rest never do");
        auto kinds = [](int id) { std::vector<A> v; const AbString& s = abString(id); for (int k = 0; k < s.n; ++k) v.push_back(s.steps[k].k); return v; };
        CHECK(kinds(AB_FLURRY) == std::vector<A>({A::AJAB, A::AJAB, A::AJAB, A::AJAB, A::ACROSS}) && abString(AB_FLURRY).steps[3].minPhase == 2,
              "FLURRY: jab-jab-jab(-jab)-cross");
        CHECK(kinds(AB_BREAKER) == std::vector<A>({A::AJAB, A::ACROSS, A::AHAYMAKER}), "BREAKER: jab, cross, then a gold haymaker");
        CHECK(kinds(AB_CROSSUP) == std::vector<A>({A::ATELEPORT, A::ABACKFIST, A::AROUNDHOUSE}), "CROSS-UP: teleport, backfist, roundhouse");
        CHECK(kinds(AB_BULLRUSH) == std::vector<A>({A::ACHARGE, A::AELBOW, A::ACLAP}), "BULLRUSH: charge, elbow, clap - all three colours");
        CHECK(kinds(AB_HEEL) == std::vector<A>({A::AHOP, A::AAXE, A::ASTOMP}), "HEEL OF HEAVEN: hop, axe kick, the stomp ring");
        {   // THUNDERCLAP: he rises, hangs, claps; the fan leaves the floor under him at you, and he comes back down
            CHECK(kinds(AB_THUNDER) == std::vector<A>({A::ATHUNDER}) && abTellOf(A::ATHUNDER) == AbTell::RED && !Enemy::abMeleeWhite(A::ATHUNDER),
                  "THUNDERCLAP: one red move (get out of the flames' way), never parried");
            glm::vec3 me = ZC + glm::vec3{0, 0, 18};
            Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_THUNDER;
            e.position = me + glm::vec3{0, 0, -8}; e.position.y = ZC.y;
            float peak = 0.f; bool fan = false, cued = false; glm::vec3 from{0.f}, to{0.f}; float after = -1.f, altAfter = 99.f;
            for (int i = 0; i < 60 * 4; ++i) {
                e.update(DT, zworld(me));
                if (e.attack == A::ATHUNDER) peak = std::max(peak, e.abAlt);
                cued |= e.ev.abCue && e.attack == A::ATHUNDER;
                if (e.ev.abFan) { fan = true; from = e.ev.abFrom; to = e.ev.abTo; after = 0.f; }
                else if (after >= 0.f && (after += DT) > 0.8f && altAfter > 50.f) altAfter = e.abAlt;
            }
            CHECK(peak > Enemy::AB_THUNDER_UP - 0.5f, "...he rises before he claps");
            CHECK(cued && fan && std::fabs(from.y - ZC.y) < 1.3f && glm::length(glm::vec2{to.x - me.x, to.z - me.z}) < 0.5f,
                  "...the pulse cues it, then the fan leaves the floor under him, aimed where you stood");
            CHECK(altAfter < 1.f, "...and he's back on the floor soon after");
        }
        auto lands = [&](int id, glm::vec3 me) {
            Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = id;
            e.position = me + glm::vec3{0, 0, -6}; e.position.y = ZC.y;
            int hits = 0;
            for (int i = 0; i < 60 * 6; ++i) {
                e.update(DT, zworld(me));
                if (e.abCur >= 0 && e.abCur != id) break;   // (only this string's blows)
                hits += e.ev.meleeHit;
            }
            return hits;
        };
        glm::vec3 me = ZC + glm::vec3{0, 0, 18};
        int br = lands(AB_BULLRUSH, me), fl = lands(AB_FLURRY, me);
        std::printf("      bullrush lands %d, flurry %d\n", br, fl);
        CHECK(br >= 2, "BULLRUSH lands on a player standing still");
        CHECK(fl >= 4, "FLURRY lands every punch on one who doesn't answer");
        Enemy c = freshAbs(); c.abArrive = 0.f; c.attack = A::ACLAP; c.abCur = AB_BULLRUSH; c.abStep = 2; c.telegraphDuration = 0.4f; c.telegraphTimer = DT * 0.5f;
        c.position = me + glm::vec3{0, 0, -4}; c.position.y = ZC.y; c.yaw = 0.f;   // facing +Z, at you
        Enemy c2 = c;
        c.update(DT, zworld(me));
        c2.update(DT, zworld(me + glm::vec3{5.f, 0, -4.f}));   // beside him
        CHECK(c.ev.meleeHit && !c2.ev.meleeHit, "the clap: a 6 m cone in front of him (step aside or behind)");
        CHECK(statsOf(EnemyType::ABSOLUTE).health == 10800.f && isBoss(EnemyType::ABSOLUTE), "10800 hp (he flies now: retuned), a boss");
        CHECK(ZEN_DAIS == Enemy::AB_DAIS_R && ZEN_DAIS_UP == Enemy::AB_DAIS_UP, "the mind and the map agree on the Dais");
    }
    {
        Enemy e = freshAbs();
        glm::vec3 me = ZC + glm::vec3{0, 0, 20};
        e.update(DT, zworld(me));
        CHECK(!e.targetable(), "arriving: untargetable");
        for (int i = 0; i < 125; ++i) e.update(DT, zworld(me));
        CHECK(e.targetable(), "after 2 s he's in the fight");
        bool started = false;
        for (int i = 0; i < 120 && !started; ++i) { e.update(DT, zworld(me)); started = e.abCur >= 0; }
        CHECK(started, "...and opens a string within 2 s");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_TOO_SLOW;
        glm::vec3 me = ZC + glm::vec3{0, 0, 20};
        bool blinked = false; int whites = 0, parried = 0; float reeled = 0.f;
        bool firstOk = false, secondOk = false;
        for (int i = 0; i < 600 && parried < 2; ++i) {
            e.update(DT, zworld(me));
            if (e.ev.blinked) blinked = true;
            if (e.ev.abTell == (int)AbTell::WHITE) ++whites;
            if (e.parryWindow()) {
                int r = e.abParried(false);
                ++parried;
                if (parried == 1) { firstOk = r == 1 && e.staggered() && e.abReeling && std::fabs(e.armorMult() - 1.5f) < 1e-4f; reeled = e.staggerTimer; }
                if (parried == 2) secondOk = r == 2 && e.staggered() && !e.abReeling && std::fabs(e.armorMult() - 2.f) < 1e-4f && std::fabs(e.staggerTimer - 3.f) < 0.05f;
            }
        }
        CHECK(blinked, "TOO SLOW opens with a teleport (an afterimage)");
        CHECK(firstOk && whites >= 2 && std::fabs(reeled - Enemy::AB_REEL) < 0.05f, "the first parry breaks his stance: 1.25 s (x1.5), and the string goes on");
        CHECK(secondOk, "the second in one string breaks him: 3 s at x2");
        CHECK(e.abParried(false) == 0, "a broken Absolute can't be parried again");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_KNEEL;
        glm::vec3 me = ZC + glm::vec3{0, 0, 12};
        bool goldWindow = false, launched = false;
        for (int i = 0; i < 400; ++i) {
            e.update(DT, zworld(me));
            if (abTellOf(e.attack) == AbTell::GOLD && e.parryWindow()) goldWindow = true;
            if (e.ev.abLaunch) launched = true;
        }
        CHECK(!goldWindow, "no parry window ever opens on a gold strike");
        CHECK(launched, "KNEEL's uppercut launches you when it lands");
    }
    {
        float least = 1e9f, leastFollow = 1e9f;
        for (int d = 0; d < DIFFICULTY_LEVELS; ++d)
            for (int id = 0; id < AB_STRINGS; ++id)
                for (int ph = 1; ph <= 3; ++ph) {
                    Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = id; e.abPhase = ph; e.abBlade = ph == 3;
                    EnemyWorld w = zworld(ZC + glm::vec3{0, 0, 14}); w.tune = &difficulty(d);
                    for (int i = 0; i < 400; ++i) {
                        e.update(DT, w);
                        if (e.ev.telegraphStarted && abTellOf(e.attack) != AbTell::NONE) {
                            if (e.tellFollow) leastFollow = std::min(leastFollow, e.telegraphDuration);
                            else least = std::min(least, e.telegraphDuration);
                        }
                    }
                }
        CHECK(least >= Enemy::TELL_FLOOR - 1e-4f && leastFollow >= Enemy::AB_FOLLOW_FLOOR - 1e-4f,
              "every Absolute tell is at least 0.35 s (0.22 s within a string), every difficulty and phase");
    }
    {
        // He starts beside the camper, mid-rotation, in a long string: only the
        // camp clock can bring TOO SLOW (a teleport onto you) before 5 s
        auto campTest = [&](glm::vec3 me, bool shard) {
            Enemy e = freshAbs(); e.abArrive = 0.f; e.abRot = 1; e.abForce = AB_JUDGED;
            glm::vec3 in = glm::normalize(glm::vec3{ZC.x - me.x, 0.f, ZC.z - me.z});
            e.position = me + in * 8.f; e.position.y = ZC.y;
            for (int i = 0; i < 5 * 60; ++i) {
                EnemyWorld w = zworld(me); w.onShard = shard;
                e.update(DT, w);
                if (e.ev.blinked && e.abCur == AB_TOO_SLOW && glm::length(glm::vec2{e.position.x - me.x, e.position.z - me.z}) < 6.f) return i * DT;
            }
            return 99.f;
        };
        float far = campTest(ZC + glm::vec3{0, 0, 42.f}, false);
        glm::vec3 pil = zenithPillar(0), out = glm::normalize(glm::vec3{pil.x - ZC.x, 0, pil.z - ZC.z});
        float behind = campTest(pil + out * 2.f, false);
        std::printf("      anti-camp: far %.1f s, behind a pillar %.1f s\n", far, behind);
        CHECK(far < 5.f && behind < 5.f, "far away or behind a pillar: within 5 s he's on you (TOO SLOW)");
    }
    {
        srand(77);
        int bad = 0;
        EnemyWorld w = zworld(ZC);
        for (int i = 0; i < 500; ++i) {
            float a = frand(0.f, 6.283f), r = frand(0.f, 48.f);
            glm::vec3 want = ZC + glm::vec3{std::cos(a) * r, 0.f, std::sin(a) * r};
            glm::vec3 p = absSafeSpot(w, want, ZC);
            float pr = glm::length(glm::vec2{p.x - ZC.x, p.z - ZC.z});
            if (pr > Enemy::AB_FLOOR_R + 0.01f || N.zenithPillarAt(p, 0.7f) >= 0) ++bad;
            if (pr < Enemy::AB_DAIS_R - 0.5f && std::fabs(p.y - (ZC.y + Enemy::AB_DAIS_UP)) > 1e-3f) ++bad;
        }
        CHECK(bad == 0, "500 teleports, at the edge, past it or into pillars, all land on open floor (on the Dais at its height)");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.yaw = 0.f;
        CHECK(e.deflects({0, 0, -1}, 20.f) && !e.deflects({0, 0, -1}, 10.f), "beyond 14 m his guard turns shots aside; closer they land");
        e.stagger(1.f);
        CHECK(!e.deflects({0, 0, -1}, 20.f), "staggered, he's open from anywhere");
    }

    // ---------------------------------------------------------------- THE ABSOLUTE: flight
    {
        auto floorUnder = [&](const Enemy& e) {
            float r = glm::length(glm::vec2{e.position.x - ZC.x, e.position.z - ZC.z});
            return ZC.y + (r < ZEN_DAIS ? ZEN_DAIS_UP : 0.f);
        };
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_SKYFALL;
        float top = 0.f, airT = 0.f, maxAir = 0.f; bool took = false, below = false, inside = false, orbit = true;
        for (int i = 0; i < 60 * 8; ++i) {
            e.update(DT, zworld(me));
            took |= e.ev.abTakeoff;
            top = std::max(top, e.position.y - floorUnder(e));
            below |= e.position.y < floorUnder(e) - 0.01f;
            airT = e.abAirborne() ? airT + DT : 0.f; maxAir = std::max(maxAir, airT);
            for (int k = 0; k < 8; ++k) {
                const AABB& b = N.zenith.pillarHome[k];
                if (e.position.x > b.min.x && e.position.x < b.max.x && e.position.z > b.min.z && e.position.z < b.max.z && e.position.y < b.max.y) inside = true;
            }
            float dxz = glm::length(glm::vec2{e.position.x - me.x, e.position.z - me.z});
            if (e.abAirT > 1.5f && e.abAlt > 8.f && !e.abDiving && e.telegraphTimer > 0.f && (dxz < AB_ORBIT_TEST_LO || dxz > 21.f)) orbit = false;   // (once he's settled up there)
        }
        std::printf("      flight: top %.1f m, longest %.1f s aloft\n", top, maxAir);
        CHECK(took, "he takes off (a takeoff event: the roar, the dust)");
        CHECK(top >= 9.f && top <= 14.5f, "he climbs to 9-14 m");
        CHECK(orbit, "up there he circles 14-20 m from you");
        CHECK(maxAir <= 6.f, "never airborne longer than 6 s");
        CHECK(!below && !inside, "never under the floor, never inside a pillar");
    }
    {
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_SKYFALL;
        for (int i = 0; i < 120 && !e.abAirborne(); ++i) e.update(DT, zworld(me));
        CHECK(e.abAirborne() && !e.deflects({0, 0, -1}, 30.f), "in the air his guard is down: gunfire lands from any range");
        e.health -= e.maxHealth * (Enemy::AB_SHOOTDOWN + 0.005f);
        e.update(DT, zworld(me));
        CHECK(e.ev.abShotDown && e.staggered() && std::fabs(e.staggerTimer - Enemy::AB_DOWNED) < 0.05f &&
              std::fabs(e.armorMult() - 1.5f) < 1e-4f, "6 % of his health in one flight shoots him down: 1.5 s at x1.5");
        float fallT = 0.f;
        while (e.abAlt > 0.01f && fallT < 2.f) { e.update(DT, zworld(me)); fallT += DT; }
        CHECK(fallT < 1.f, "...and he crashes to the floor");
        Enemy f = freshAbs(); f.abArrive = 0.f; f.abForce = AB_SKYFALL;
        for (int i = 0; i < 120 && !f.abAirborne(); ++i) f.update(DT, zworld(me));
        f.health -= f.maxHealth * 0.05f;
        f.update(DT, zworld(me));
        CHECK(!f.ev.abShotDown && f.abAirborne(), "5 % isn't enough");
    }
    {
        // Review Focus 1: interrupted in the air, he always comes down
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        bool down = true;
        for (int how : {0, 2}) {   // (the ascension lifts him over the Dais on purpose: checked below)
            Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_SKYFALL;
            for (int i = 0; i < 120 && !e.abAirborne(); ++i) e.update(DT, zworld(me));
            if (how == 0) e.stagger(1.f);
            if (how == 2) { e.abPhase = 3; e.abBlade = true; e.health = e.maxHealth * 0.07f; }   // the last judgment
            float low = e.abAlt;
            for (float t = 0.f; t < 1.2f; t += DT) {   // (he may take off again afterwards: what matters is that he came down)
                e.update(DT, zworld(me));
                low = std::min(low, e.abAlt);
                down &= e.position.y >= ZC.y - 0.01f;
            }
            down &= low < 0.1f;
        }
        CHECK(down, "a stagger or the last judgment in the air brings him down within 1.2 s");
        Enemy a = freshAbs(); a.abArrive = 0.f; a.health = a.maxHealth * 0.59f;
        float hi = 0.f;
        for (int i = 0; i < 60 * 3; ++i) { a.update(DT, zworld(me)); hi = std::max(hi, a.position.y - (ZC.y + ZEN_DAIS_UP)); }
        CHECK(hi > 10.f, "ASCENSION really lifts him ~12 m over the Dais for the sweep");
        Enemy arr = freshAbs();
        arr.update(DT, zworld(me));
        CHECK(arr.position.y > ZC.y + ZEN_DAIS_UP + 8.f, "he arrives from above...");
        for (int i = 0; i < 125; ++i) arr.update(DT, zworld(me));
        CHECK(arr.abAlt < 0.1f, "...and stands on the Dais when the fight begins");
    }
    {
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_SKYFALL;
        bool marked = false, gold = false; float markT = -1.f, landT = -1.f; glm::vec3 mark{0.f}; float t = 0.f;
        for (; t < 6.f && landT < 0.f; t += DT) {
            e.update(DT, zworld(me));
            if (e.ev.abMark) { marked = true; mark = e.ev.abTo; markT = t; gold = abTellOf(e.attack) == AbTell::GOLD; }
            if (e.ev.abCrater) landT = t;
        }
        CHECK(marked && gold && glm::length(glm::vec2{mark.x - me.x, mark.z - me.z}) < 0.5f, "SKYFALL: a gold circle locks onto where you stand");
        CHECK(landT - markT >= 0.69f && e.abAlt < 0.1f, "...0.7 s, then he crashes down onto it");
        CHECK(e.ev.meleeHit && std::fabs(e.ev.meleeDamage - 28.f) < 1e-3f, "standing in it: 28");
        Enemy d = freshAbs(); d.abArrive = 0.f; d.abForce = AB_SKYFALL;
        glm::vec3 p = me; bool hitD = false, moved = false;
        for (float u = 0.f; u < 6.f; u += DT) {
            d.update(DT, zworld(p));
            if (d.ev.abMark && !moved) { p += glm::vec3{5.f, 0, 0}; moved = true; }   // a dash out of the circle
            hitD |= d.ev.abCrater && d.ev.meleeHit;   // (the strings after it are another matter)
        }
        CHECK(moved && !hitD, "...a dash out of the circle and it misses");
    }
    {
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_METEOR;
        bool flare = false, window = false; int r = 0;
        for (int i = 0; i < 60 * 6 && !r; ++i) {
            e.update(DT, zworld(me));
            flare |= e.ev.abFlare;
            if (e.attack == AttackKind::AMETEOR && e.parryWindow()) { window = true; r = e.abParried(true); }
        }
        CHECK(flare && window && abTellOf(AttackKind::AMETEOR) == AbTell::WHITE, "METEOR: a flare, a white dive at you, parryable");
        CHECK(r == 4 && e.staggered() && std::fabs(e.staggerTimer - Enemy::AB_DOWNED) < 0.05f && e.abReeling,
              "...parried: knocked out of the sky, 1.5 s at x1.5");
        float fall = 0.f; while (e.abAlt > 0.01f && fall < 2.f) { e.update(DT, zworld(me)); fall += DT; }
        CHECK(fall < 1.f, "...down on the floor");
        Enemy u = freshAbs(); u.abArrive = 0.f; u.abForce = AB_METEOR;
        bool hit = false; float dmg = 0.f;
        for (int i = 0; i < 60 * 6 && !hit; ++i) { u.update(DT, zworld(me)); if (u.ev.meleeHit) { hit = true; dmg = u.ev.meleeDamage; } }
        CHECK(hit && std::fabs(dmg - 24.f) < 1e-3f, "...unparried, it lands on you: 24");
    }
    {
        // a shard camper: he flies over and SKYFALLs the shard (every phase)
        bool all = true;
        for (int ph = 1; ph <= 3; ++ph) {
            glm::vec3 me = zenithShard(0); me.y = N.zenith.shardHome[0].max.y;
            Enemy e = freshAbs(); e.abArrive = 0.f; e.abPhase = ph; e.abBlade = ph == 3; e.abRot = 1; e.abForce = AB_JUDGED;
            e.health = e.maxHealth * (ph == 1 ? 1.f : ph == 2 ? 0.55f : 0.2f);
            bool broke = false;
            for (int i = 0; i < 60 * 8 && !broke; ++i) {
                EnemyWorld w = zworld(me); w.onShard = true;
                e.update(DT, w);
                broke = e.ev.abShardBreak && N.zenithShardAt(e.ev.abTo) == 0;
            }
            if (!broke) std::printf("      phase %d: no shard break\n", ph);
            all &= broke && glm::length(glm::vec2{e.position.x - ZC.x, e.position.z - ZC.z}) <= Enemy::AB_FLOOR_R + 0.01f;
        }
        CHECK(all, "camping a shard: SKYFALL breaks it under you, in every phase, and he lands back on the floor");
        LevelData L2 = N;
        L2.parkZenithPiece(true, 0);
        L2.restoreZenith();
        CHECK(L2.zenith.shardUp[0] && L2.onZenithShard(glm::vec3{zenithShard(0).x, N.zenith.shardHome[0].max.y, zenithShard(0).z}),
              "a retry brings a broken shard back");
    }
    {
        // Review Focus 5: a dive beside a pillar or the edge never lands in it
        int bad = 0;
        for (int k = 0; k < 8; ++k)
            for (int which : {AB_SKYFALL, AB_METEOR}) {
                glm::vec3 pil = zenithPillar(k), out = glm::normalize(glm::vec3{pil.x - ZC.x, 0, pil.z - ZC.z});
                glm::vec3 me = pil + out * 1.8f; me.y = ZC.y;
                Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = which;
                for (int i = 0; i < 60 * 6; ++i) {
                    e.update(DT, zworld(me));
                    const AABB& b = N.zenith.pillarHome[k];
                    if (e.position.x > b.min.x && e.position.x < b.max.x && e.position.z > b.min.z && e.position.z < b.max.z && e.position.y < b.max.y) ++bad;
                    if (glm::length(glm::vec2{e.position.x - ZC.x, e.position.z - ZC.z}) > Enemy::AB_FLOOR_R + 0.01f) ++bad;
                }
            }
        CHECK(bad == 0, "a SKYFALL or METEOR beside a pillar never ends inside it, or past the edge");
    }
    {
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abPhase = 3; e.abBlade = true; e.health = e.maxHealth * 0.2f; e.abForce = AB_BLADEDIVE;
        bool lungeAir = false;
        for (int i = 0; i < 60 * 6; ++i) { e.update(DT, zworld(me)); if (e.attack == AttackKind::ALUNGE && e.abAirborne()) lungeAir = true; }
        CHECK(lungeAir && abTellOf(AttackKind::ALUNGE) == AbTell::GOLD, "phase 3: the blade dive, a gold LUNGE from the air");
    }
    // ---------------------------------------------------------------- review fixes (the overhaul)
    {
        // shot down while SKYFALL's circle is on the floor: the circle goes with him
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_SKYFALL;
        bool marked = false;
        for (int i = 0; i < 60 * 6 && !marked; ++i) { e.update(DT, zworld(me)); marked = e.ev.abMark; }
        bool liveThen = abSkyLive(&e);
        e.health -= e.maxHealth * (Enemy::AB_SHOOTDOWN + 0.01f);
        e.update(DT, zworld(me));
        CHECK(marked && liveThen && e.ev.abShotDown && !abSkyLive(&e) && !abSkyLive(nullptr),
              "shot down mid-SKYFALL, the gold circle is gone (it shows only while his SKYFALL winds up)");
    }
    {
        // METEOR commits at its cue: a dash on the ting gets you out (the rule every blow keeps)
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_METEOR;
        glm::vec3 p = me; bool dashed = false, hit = false, landed = false;
        for (int i = 0; i < 60 * 6 && !landed; ++i) {
            e.update(DT, zworld(p));
            if (e.attack == AttackKind::AMETEOR && e.ev.abCue && !dashed) { p += glm::vec3{5.f, 0, 0}; dashed = true; }
            if (e.ev.abImpact && e.ev.abImpactKind == AttackKind::AMETEOR) { landed = true; hit = e.ev.meleeHit; }
        }
        CHECK(dashed && landed && !hit, "a 5 m dash on METEOR's cue gets you out of it");
    }
    {
        // a phase change in the air: never shot out of the ascension's hold, down at once for the blade
        glm::vec3 me = ZC + glm::vec3{0, 0, 16};
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_SKYFALL;
        for (int i = 0; i < 120 && !e.abAirborne(); ++i) e.update(DT, zworld(me));
        e.health = e.maxHealth * 0.59f;
        e.update(DT, zworld(me));
        e.health -= e.maxHealth * 0.055f;
        bool shot = false;
        for (int i = 0; i < 60; ++i) { e.update(DT, zworld(me)); shot |= e.ev.abShotDown; }
        CHECK(e.abPhase == 2 && !shot, "the ascension's hold is no flight: damage through it can't shoot him down");
        Enemy b = freshAbs(); b.abArrive = 0.f; b.abForce = AB_SKYFALL; b.abPhase = 2; b.health = b.maxHealth * 0.3f;
        for (int i = 0; i < 120 && !b.abAirborne(); ++i) b.update(DT, zworld(me));
        b.health = b.maxHealth * 0.24f;
        float t = 0.f;
        for (; t < 1.f && b.abAlt > 0.1f; t += DT) b.update(DT, zworld(me));
        CHECK(b.abPhase == 3 && t < 0.5f, "drawing the blade in the air, he comes straight down (no hanging there)");
    }
    {
        // the ascension mid-flight, then a camper on a shard: still never more than 6 s aloft
        glm::vec3 me = zenithShard(0); me.y = N.zenith.shardHome[0].max.y;
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_SKYFALL;
        float air = 0.f, most = 0.f;
        for (int i = 0; i < 60 * 14; ++i) {
            if (i == 90) { e.health = e.maxHealth * 0.59f; e.abForce = AB_SKYFALL; }   // (already up; another flight straight after the hold)
            EnemyWorld w = zworld(me); w.onShard = true;
            e.update(DT, w);
            air = e.abAirborne() ? air + DT : 0.f; most = std::max(most, air);
        }
        std::printf("      longest aloft through an ascension: %.1f s\n", most);
        CHECK(most <= 6.f, "a second takeoff while still aloft doesn't restart the flight's clock");
    }

    // ---------------------------------------------------------------- THE ABSOLUTE: by ear
    {
        bool every = true;
        for (AttackKind k : abAllAttacks()) {
            const AbFeel& f = abFeel(k);
            AbTell t = abTellOf(k);
            if (t != AbTell::NONE && !f.wind) { every = false; std::printf("      no wind-up for %s\n", attackKey(k)); }
            bool blow = Enemy::abMeleeWhite(k) || (t == AbTell::GOLD && k != AttackKind::ALUNGE) || k == AttackKind::ACLAP || k == AttackKind::ASTOMP;
            if (blow && !f.boom) { every = false; std::printf("      no boom for %s\n", attackKey(k)); }
        }
        CHECK(every, "every telegraphed move has a wind-up; every blow a boom");
        CHECK(std::string(abCueSound(AbTell::WHITE)) == "ab_ting" && std::string(abCueSound(AbTell::GOLD)) == "ab_doom" &&
              std::string(abCueSound(AbTell::RED)) == "ab_pulse", "the cues: TING parry, DOOM dodge or block, a double pulse get out");
        bool goldNoTing = true;
        for (AttackKind k : abAllAttacks())
            if (abTellOf(k) == AbTell::GOLD && abFeel(k).wind && std::string(abFeel(k).wind) == "ab_ting") goldNoTing = false;
        CHECK(goldNoTing, "no gold move ever plays the ting");
        std::set<std::string> booms;
        for (AttackKind k : {AttackKind::AJAB, AttackKind::ACROSS, AttackKind::AHOOK, AttackKind::AHAYMAKER, AttackKind::AROUNDHOUSE, AttackKind::AAXE})
            booms.insert(abFeel(k).boom);
        CHECK(booms.size() == 6, "jab, cross, hook, haymaker, kick and axe kick each boom their own way");
        bool listed = true;
        for (AttackKind k : attacksOf(EnemyType::ABSOLUTE)) listed &= std::find(abAllAttacks().begin(), abAllAttacks().end(), k) != abAllAttacks().end();
        CHECK(listed && abAllAttacks().size() == attacksOf(EnemyType::ABSOLUTE).size() + 8, "the feel table knows every move he has (MORNINGSTAR's eight too)");
    }
    {
        // Review Focus 4: one cue per move, 0.22 s before it lands, every difficulty and phase (delayed strikes too)
        bool once = true, lead = true; int seen = 0;
        for (int dl = 0; dl < DIFFICULTY_LEVELS; ++dl)
            for (int ph = 1; ph <= 3; ++ph) {
                Enemy e = freshAbs(); e.abArrive = 0.f; e.abPhase = ph; e.abBlade = ph == 3;
                e.health = e.maxHealth * (ph == 1 ? 1.f : ph == 2 ? 0.5f : 0.2f);
                glm::vec3 me = ZC + glm::vec3{0, 0, 10};
                int cues = 0; AttackKind cur = AttackKind::NONE;   // (no parries here: no move is cut short)
                for (int i = 0; i < 60 * 30; ++i) {
                    EnemyWorld w = zworld(me); w.tune = &difficulty(dl);
                    e.update(DT, w);
                    if (e.ev.telegraphStarted) {
                        if (cur != AttackKind::NONE && abTellOf(cur) != AbTell::NONE && cues != 1) { once = false; std::printf("      %s cued %d times\n", attackKey(cur), cues); }
                        cur = e.attack; cues = 0;
                    }
                    if (e.ev.abCue) {
                        ++cues; ++seen;
                        if (e.telegraphDuration > Enemy::AB_CUE + DT && std::fabs(e.telegraphTimer - Enemy::AB_CUE) > DT + 1e-4f) lead = false;
                    }
                }
            }
        std::printf("      cues heard: %d\n", seen);
        CHECK(once && seen > 50, "every telegraphed move cues exactly once");
        CHECK(lead, "...0.22 s before it lands, whatever sped it up");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_FLURRY;
        glm::vec3 me = ZC + glm::vec3{0, 0, 30};   // well out of reach: every punch misses
        int booms = 0;
        for (int i = 0; i < 60 * 6; ++i) {
            e.update(DT, zworld(me));
            if (e.abCur >= 0 && e.abCur != AB_FLURRY) break;
            booms += e.ev.abImpact && abFeel(e.ev.abImpactKind).boom != nullptr && !e.ev.meleeHit;
        }
        CHECK(booms >= 4, "a punch booms whether or not it lands (FLURRY: boom boom boom boom)");
    }

    // ---------------------------------------------------------------- the Absolute's hazards
    {
        using AH = AbsoluteHazards;
        const glm::vec3 c = ZC;
        auto total = [](const std::vector<AH::Hit>& hs) { float s = 0; for (auto& h : hs) s += h.damage; return s; };
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 12};
          h.addRay(c + glm::vec3{0, 1.6f, 0}, me + glm::vec3{0, 1, 0});
          float got = 0; for (float t = 0; t < 1.2f; t += DT) got += total(h.update(DT, me, 1.8f, true, &N, c));
          CHECK(got > 20.f && got < 27.f, "standing in a ray: about 25 over its second");
          AH r; glm::vec3 p = me + glm::vec3{7.f * 0.6f, 0, 0}; float got2 = 0;   // strafed through its 0.6 s tell
          r.addRay(c + glm::vec3{0, 1.6f, 0}, me + glm::vec3{0, 1, 0});
          for (float t = 0; t < 1.2f; t += DT) { p.x += 7.f * DT; got2 += total(r.update(DT, p, 1.8f, true, &N, c)); }
          CHECK(got2 == 0.f, "strafing at a run outpaces it: it fires where its red line was, then chases at 5.5 m/s");
          AH q; glm::vec3 pil = zenithPillar(0), out = glm::normalize(glm::vec3{pil.x - c.x, 0, pil.z - c.z});
          glm::vec3 hide = pil + out * 2.f;
          q.addRay(c + glm::vec3{0, 1.6f, 0}, hide + glm::vec3{0, 1, 0});
          float got3 = 0; for (float t = 0; t < 1.2f; t += DT) got3 += total(q.update(DT, hide, 1.8f, true, &N, c));
          CHECK(got3 < 1e-3f, "a pillar between you stops it");
          AH pr; pr.addRay(c, me); pr.update(0.1f, me, 1.8f, true, &N, c);
          CHECK(pr.rayParry(0) && pr.rays.empty(), "a ray caught in its first 0.15 s is parried and ends");
          AH late; late.addRay(c, me); late.update(0.2f, me, 1.8f, true, &N, c);
          CHECK(!late.rayParry(0), "...not after"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 9};
          AH h; h.addStomp(c);
          float grounded = 0, jumped = 0;
          for (float t = 0; t < 1.f; t += DT) grounded += total(h.update(DT, me, 1.8f, true, &N, c));
          AH j; j.addStomp(c);
          for (float t = 0; t < 1.f; t += DT) jumped += total(j.update(DT, me + glm::vec3{0, 1.4f, 0}, 1.8f, false, &N, c));
          CHECK(std::fabs(grounded - AH::STOMP_DAMAGE) < 1e-3f && jumped == 0.f, "the stomp ring hits once on the ground; jump it"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 12};   // THUNDERCLAP's fan, aimed at you
          auto run = [&](glm::vec3 at, glm::vec3 feet, bool grounded) {
              AH h; h.addFan(c, at); float got = 0;
              for (float t = 0; t < 3.f; t += DT) got += total(h.update(DT, feet, 1.8f, grounded, &N, c));
              return got;
          };
          float stand = run(me, me, true), jump = run(me, me + glm::vec3{0, 1.4f, 0}, false);
          glm::vec3 gap = c + glm::vec3{std::sin(0.15f), 0.f, std::cos(0.15f)} * 20.f;   // between the middle lane and the next, 20 m out
          float inGap = run(c + glm::vec3{0, 0, 20}, gap, true);
          glm::vec3 pil = zenithPillar(0), toP = glm::normalize(glm::vec3{pil.x - c.x, 0.f, pil.z - c.z});
          float hidden = run(pil, pil + toP * 4.f, true);
          CHECK(std::fabs(stand - AH::FAN_DAMAGE) < 1e-3f, "THUNDERCLAP's fan hits a standing player once");
          CHECK(jump == 0.f, "...jump the flame and it passes under you");
          CHECK(inGap == 0.f, "...further out the lanes open: stand in a gap");
          CHECK(hidden == 0.f, "...a pillar stops the flame that would reach you");
          AH h; h.addFan(c, me); h.update(DT, me, 1.8f, true, &N, c); h.clear();
          CHECK(h.fans.empty(), "a retry clears the fan"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 15}, p = me;
          AH h; h.addSerpent(c + glm::vec3{0, 1.8f, 0}, {0, 0, 1});
          float got = 0;
          for (float t = 0; t < AH::SERPENT_LIFE + 0.5f; t += DT) { p.x += 7.f * DT; got += total(h.update(DT, p, 1.8f, true, &N, c)); }
          CHECK(got == 0.f, "a running player outruns a serpent");
          AH s; s.addSerpent(c + glm::vec3{0, 1.8f, 0}, {0, 0, 1});
          float hit = 0; for (float t = 0; t < 4.f; t += DT) hit += total(s.update(DT, me, 1.8f, true, &N, c));
          CHECK(std::fabs(hit - AH::SERPENT_DAMAGE) < 1e-3f, "standing still, it reaches you once");
          AH r; r.addSerpent(me + glm::vec3{0, 1.5f, -1.f}, {0, 0, 1});
          int k = r.serpentNear(me + glm::vec3{0, 1.5f, 0}, 2.f);
          CHECK(k == 0, "a serpent within reach can be found to parry");
          r.reflectSerpent(k);
          int back = 0; for (float t = 0; t < 4.f && !back; t += DT) { r.update(DT, me, 1.8f, true, &N, c); back = r.reflectedHits(c + glm::vec3{0, 1.3f, 0}, 1.4f); }
          CHECK(back == 1, "parried, it flies back into him"); }
        { glm::vec3 a = c + glm::vec3{3, 0, 3}, b = c + glm::vec3{9, 0, 3};
          CHECK(AH::seamParity(c, a) != AH::seamParity(c, b), "neighbouring cells alternate");
          glm::vec3 in0 = AH::seamParity(c, a) == 0 ? a : b, in1 = AH::seamParity(c, a) == 0 ? b : a;
          AH h0; h0.addSeams(c, 0); float d0 = 0; for (float t = 0; t < AH::SEAM_WARN + 0.05f; t += DT) d0 += total(h0.update(DT, in0, 1.8f, true, &N, c));
          AH h1; h1.addSeams(c, 0); float d1 = 0; for (float t = 0; t < AH::SEAM_WARN + 0.05f; t += DT) d1 += total(h1.update(DT, in1, 1.8f, true, &N, c));
          CHECK(d0 == AH::SEAM_DAMAGE && d1 == 0.f, "the first wave burns one parity; the dark cells are safe");
          AH h2; h2.addSeams(c, 0); float d2 = 0; for (float t = 0; t < AH::SEAM_WARN + AH::SEAM_GAP + 0.05f; t += DT) d2 += total(h2.update(DT, in1, 1.8f, true, &N, c));
          CHECK(d2 == AH::SEAM_DAMAGE, "...then the other, 0.7 s later: step across"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 20}, p = me + glm::vec3{7.f * 0.6f, 0, 0};
          AH h; h.addZenith(c + glm::vec3{0, 40, 0}, me);
          float got = 0; for (float t = 0; t < AH::ZRAY_TIME; t += DT) { p.x += 7.f * DT; got += total(h.update(DT, p, 1.8f, true, &N, c)); }
          AH s; s.addZenith(c + glm::vec3{0, 40, 0}, me);
          float still = 0; for (float t = 0; t < AH::ZRAY_TIME; t += DT) still += total(s.update(DT, me, 1.8f, true, &N, c));
          CHECK(got == 0.f && still > 40.f, "the zenith ray burns where you stand, and turns slower than you run"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 20};
          AH h; h.addHalo(c, c.y);
          float stand = 0; for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) stand += total(h.update(DT, me, 1.8f, true, &N, c));
          AH s; s.addHalo(c, c.y);
          float slide = 0; for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) slide += total(s.update(DT, me, 0.9f, true, &N, c));
          CHECK(stand == AH::HALO_DAMAGE && slide == 0.f, "the 360 sweep hits you standing; slide under it");
          AH m; m.addHalo(c, c.y); int mask = 0;
          for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) { m.update(DT, me, 1.8f, true, &N, c); mask |= m.haloShatterMask(); }
          CHECK(mask == 0x55, "it shatters four pillars (every other one)"); }
        { AH h; h.addJudge(c, c); AH f; f.addJudge(c, c);
          float near = 0, far = 0;
          for (float t = 0; t < AH::JUDGE_WARN + 0.1f; t += DT) {
              near += total(h.update(DT, c + glm::vec3{0, 0, 4}, 1.8f, true, &N, c));
              far += total(f.update(DT, c + glm::vec3{0, 6, 20}, 1.8f, false, &N, c));
          }
          CHECK(near == 0.f && far == AH::JUDGE_DAMAGE, "the last judgment: beside him you're safe; anywhere else (even in the air) it's 60"); }
        { glm::vec3 me = c + glm::vec3{0, 0, 10};
          AH h; h.addMirage(me + glm::vec3{2, 0, 0});
          CHECK(h.mirageAlong(me + glm::vec3{0, 1.5f, 0}, glm::vec3{1, 0, 0}, 10.f) == 0, "a mirage can be shot");
          h.killMirage(0);
          float got = 0; for (float t = 0; t < 1.f; t += DT) got += total(h.update(DT, me, 1.8f, true, &N, c));
          CHECK(h.mirages.empty() && got == 0.f, "one hit and it's gone");
          AH k; k.addMirage(me + glm::vec3{2, 0, 0}); float hit = 0;
          for (float t = 0; t < 1.f; t += DT) hit += total(k.update(DT, me, 1.8f, true, &N, c));
          CHECK(hit == AH::MIRAGE_DAMAGE, "left alone its jab lands for half (8)"); }
        { AH h; h.addRay(c, c); h.addStomp(c); h.addSerpent(c, {0, 0, 1}); h.addSeams(c, 0); h.addZenith(c, c); h.addHalo(c, c.y); h.addMirage(c); h.addJudge(c, c);
          h.clear();
          CHECK(h.rays.empty() && h.stomps.empty() && h.serpents.empty() && h.seams.empty() && h.zrays.empty() && h.halos.empty() && h.mirages.empty() && h.judges.empty(),
                "clear() empties every hazard (a retry)"); }
    }

    // ---------------------------------------------------------------- THE ABSOLUTE: phases 2 and 3
    {
        Enemy e = freshAbs(); e.abArrive = 0.f;
        glm::vec3 me = ZC + glm::vec3{0, 0, 14};
        e.health = e.maxHealth * 0.59f;
        e.update(DT, zworld(me));
        CHECK(e.abPhase == 2 && e.ev.abPhase == 2 && e.ev.abHalo && e.phaseHold > 0.f, "at 60 %: ASCENSION, the halo sweep, a pause");
        Enemy a = freshAbs(); a.abArrive = 0.f; a.abForce = AB_TOO_SLOW;
        Enemy b = freshAbs(); b.abArrive = 0.f; b.abForce = AB_TOO_SLOW; b.abPhase = 2;
        int ja = 0, jb = 0; float ta = 0, tb = 0;
        for (int i = 0; i < 400; ++i) {
            a.update(DT, zworld(me)); b.update(DT, zworld(me));
            if (a.ev.telegraphStarted && a.attack == AttackKind::AJAB && a.abCur == AB_TOO_SLOW) { ++ja; if (a.tellFollow) ta = a.telegraphDuration; }
            if (b.ev.telegraphStarted && b.attack == AttackKind::AJAB && b.abCur == AB_TOO_SLOW) { ++jb; if (b.tellFollow && b.abLate == 0.f) tb = b.telegraphDuration; }
            if (a.abCur != AB_TOO_SLOW && ja > 0 && b.abCur != AB_TOO_SLOW && jb > 0) break;
        }
        CHECK(jb == ja + 1 && tb > 0.f && tb < ta, "phase 2: one more jab, follow-ups 25 % quicker");
        Enemy dl = freshAbs(); dl.abArrive = 0.f; dl.abForce = AB_TOO_SLOW; dl.abPhase = 2;
        bool lateSeen = false, lateWindowOk = true;
        for (int i = 0; i < 300; ++i) {
            dl.update(DT, zworld(me));
            if (dl.abLate > 0.f && dl.attack == AttackKind::AJAB && dl.telegraphTimer > 0.f && dl.telegraphTimer < 0.25f) { lateSeen = true; lateWindowOk &= dl.parryWindow(); }
            if (dl.abLate > 0.f && dl.attack == AttackKind::AJAB && dl.telegraphTimer > 0.3f && dl.parryWindow()) lateWindowOk = false;
        }
        CHECK(lateSeen && lateWindowOk, "a delayed strike's parry window is its last 0.25 s, not its flash");
        Enemy r = freshAbs(); r.abArrive = 0.f; r.abPhase = 2;
        bool seams = false, mirages = false, zray = false;
        for (int i = 0; i < 60 * 40; ++i) { r.update(DT, zworld(me)); seams |= r.ev.abSeams; mirages |= r.ev.abMirages > 0; }
        Enemy sh = freshAbs(); sh.abArrive = 0.f; sh.abPhase = 2;
        for (int i = 0; i < 60 * 8 && !zray; ++i) { EnemyWorld sw = zworld(zenithShard(0)); sw.onShard = true; sh.update(DT, sw); zray |= sh.ev.abMark; }
        CHECK(seams && mirages, "phase 2 brings the seams and mirages");
        CHECK(zray, "camping on a shard in phase 2 brings SKYFALL");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abPhase = 2;
        glm::vec3 me = ZC + glm::vec3{0, 0, 10};
        e.health = e.maxHealth * 0.24f;
        e.update(DT, zworld(me));
        CHECK(e.abPhase == 3 && e.ev.abPhase == 3 && e.ev.abBladeDrawn && e.ev.abShards && e.abBlade, "at 25 %: the blade, the shards fall");
        e.phaseHold = 0.f; e.abForce = AB_SWEEPS;
        int res = 0; bool broke = false;
        for (int i = 0; i < 400 && !res; ++i) { e.update(DT, zworld(me)); if (e.parryWindow()) { res = e.abParried(true); broke = e.ev.abBladeBroke; } }
        CHECK(res == 3 && !e.abBlade && e.staggered() && std::fabs(e.armorMult() - 2.f) < 1e-4f && broke,
              "a sword parry of a sweep shatters the blade: broken 3 s at x2");
        bool redrawn = false;
        for (int i = 0; i < (int)((Enemy::AB_BREAK + Enemy::AB_BLADE_BACK + 0.3f) * 60); ++i) { e.update(DT, zworld(me)); redrawn |= e.ev.abBladeDrawn; }
        CHECK(e.abBlade && redrawn, "...and he draws it again");
        Enemy f = freshAbs(); f.abArrive = 0.f; f.abPhase = 3; f.abBlade = true; f.abForce = AB_SWEEPS;
        int rf = 0;
        for (int i = 0; i < 400 && !rf; ++i) { f.update(DT, zworld(me)); if (f.parryWindow()) rf = f.abParried(false); }
        CHECK(rf == 1 && f.abBlade, "an F parry only interrupts it");
        Enemy c = freshAbs(); c.abArrive = 0.f; c.abPhase = 3; c.abBlade = true; c.abForce = AB_CRESCENTS;
        int shots = 0; float parryVal = 0;
        for (int i = 0; i < 120; ++i) { c.update(DT, zworld(ZC + glm::vec3{0, 0, 25})); shots += c.ev.shots; if (c.ev.shots) parryVal = c.ev.shotParry; }
        CHECK(shots >= 2 && std::fabs(parryVal - 200.f) < 1e-3f, "crescents: thrown arcs that parry back for 200");
        Enemy l = freshAbs(); l.abArrive = 0.f; l.abPhase = 3; l.abBlade = true; l.abForce = AB_LUNGE;
        bool lunged = false; glm::vec3 lme = ZC + glm::vec3{0, 0, 16};
        l.position = ZC + glm::vec3{0, Enemy::AB_DAIS_UP, 0};
        for (int i = 0; i < 120 && !lunged; ++i) { l.update(DT, zworld(lme)); lunged = l.ev.meleeHit; }
        CHECK(lunged && abTellOf(l.lastAbAttack) == AbTell::GOLD, "the lunge crosses 16 m and runs you through (gold)");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abPhase = 3; e.abBlade = true;
        glm::vec3 me = ZC + glm::vec3{0, 0, 10};
        e.health = e.maxHealth * 0.079f;
        bool marked = false, landed = false; float spentAt = 0;
        for (int i = 0; i < 60 * 4; ++i) {
            e.update(DT, zworld(me));
            marked |= e.ev.abJudgeMark;
            if (e.ev.abJudge) { landed = true; spentAt = e.staggerTimer; }
        }
        CHECK(marked && landed && e.abJudged && std::fabs(spentAt - Enemy::AB_SPENT) < 0.05f && !e.abReeling,
              "at 8 %: the last judgment, then he's spent 4 s (x2)");
        e.health = e.maxHealth * 0.05f;
        bool again = false; for (int i = 0; i < 60 * 10; ++i) { e.update(DT, zworld(me)); again |= e.ev.abJudgeMark; }
        CHECK(!again, "only once");
    }

    // ---------------------------------------------------------------- his rig: the most detailed in the game
    {
        auto parts = [](Enemy e) { std::vector<BoxInstance> out; buildEnemy(e, 0.f, out); return out; };
        Enemy a(EnemyType::ABSOLUTE, {0, 0, 0}); a.spawnTimer = 0.f; a.state = EnemyState::ACTIVE; a.abArrive = 0.f;
        size_t mine = parts(a).size(), most = 0;
        for (EnemyType t : {EnemyType::WARDEN, EnemyType::SOVEREIGN, EnemyType::PENITENT, EnemyType::LEVIATHAN}) {
            Enemy b(t, {0, 0, 0}); b.spawnTimer = 0.f; b.state = EnemyState::ACTIVE;
            if (t == EnemyType::LEVIATHAN) { b.levInit = true; b.levStage = Enemy::LevStage::FIGHT; b.position = {0.f, 12.f, 6.f}; }
            most = std::max(most, parts(b).size());
        }
        std::printf("      the Absolute: %d parts (the most of any other boss: %d)\n", (int)mine, (int)most);
        CHECK(mine >= 150 && mine > most, "his rig has more parts than any other boss's: fingers, ribs, plates, wraps, a broken halo");
        Enemy bl = a; bl.abBlade = true;
        CHECK(parts(bl).size() > mine, "phase 3: his blade of light in his hand");
        glm::vec3 lo{1e9f}, hi{-1e9f};
        for (auto& p : parts(a)) { glm::vec3 c{p.model[3]}; lo = glm::min(lo, c); hi = glm::max(hi, c); }
        CHECK(hi.y > 2.3f && hi.y < 3.3f && lo.y > -0.1f, "about 2.6 m tall, standing on the floor");
        auto tint = [&](AttackKind k) {
            Enemy e = a; e.attack = k; e.telegraphDuration = 0.5f; e.telegraphTimer = 0.05f;   // the end of its tell: blazing
            glm::vec3 best{0.f}; float bm = 0.f;
            for (auto& p : parts(e)) { float m = p.emissive.x + p.emissive.y + p.emissive.z; if (m > bm) { bm = m; best = p.emissive; } }
            return best / std::max(bm, 1e-4f);
        };
        glm::vec3 w = tint(AttackKind::AJAB), gld = tint(AttackKind::AUPPER), rd = tint(AttackKind::ARAY);
        CHECK(std::fabs(w.x - w.z) < 0.08f, "a white tell flashes white");
        CHECK(gld.x > gld.z * 2.f && gld.y > gld.z * 1.4f, "a gold tell flashes gold");
        CHECK(rd.x > rd.y * 3.f, "a red tell flashes red");
        // red eyes: two crimson slits in his face, and nothing gold there
        int eyes = 0, goldFace = 0;
        AABB face; headBox(a, face);
        const float faceZ = 0.5f * (face.min.z + face.max.z);
        for (auto& p : parts(a)) {
            glm::vec3 c{p.model[3]}, m = p.emissive;
            if (glm::any(glm::lessThan(c, face.min - 0.03f)) || glm::any(glm::greaterThan(c, face.max + 0.03f)) || c.z < faceZ) continue;   // his face (the front of his head: he faces +Z at yaw 0)
            if (m.x > 1.5f && m.x > m.y * 6.f) ++eyes;
            if (m.x > 1.f && m.y > m.x * 0.55f && m.z < m.x * 0.5f) ++goldFace;
        }
        CHECK(eyes >= 2 && goldFace == 0, "two glowing red eyes (the gold slit is gone)");
        int runes = 0;
        for (auto& p : parts(a)) { glm::vec3 m = p.emissive; if (m.z > 0.9f && m.z > m.x * 1.8f && m.z > m.y * 1.6f) ++runes; }
        CHECK(runes >= 8, "his gauntlets: rune bands of pale violet-blue at knuckles and wrists");
        // his guard: an MMA stance - bladed, lead (left) side forward, sat in his knees, hunched, the lead fist
        // out at you and the rear one at his cheek, standing or on the move
        rig::AbsPose gp = rig::absolutePose(a, 0.f);
        Enemy run = a; run.moveSpeed = Enemy::AB_RUN;
        rig::AbsPose gr = rig::absolutePose(run, 0.3f);
        // (an arm hangs down; a shoulder's negative swing takes it forward, the elbow folds the forearm up past it)
        auto guard = [](float sh, float el) { return sh < -0.4f && sh > -1.1f && sh - el < -2.f; };   // forearm up and forward
        CHECK(guard(gp.shR, gp.elR) && guard(gp.shL, gp.elL) && gp.shL < gp.shR && gp.elL < gp.elR,
              "idle: elbows down, forearms up, the lead fist further out, the rear one folded back to his cheek");
        CHECK(gp.hipYaw + gp.torsoYaw < -0.2f && gp.torsoPitch > 0.25f && gp.crouch > 0.08f && gp.kneeR > 0.4f && gp.kneeL > 0.4f,
              "...bladed (lead shoulder forward), hunched over, sat low in bent knees");
        CHECK(guard(gr.shR, gr.elR) && guard(gr.shL, gr.elL), "...and he keeps it up as he closes in");
        {   // his feet on the floor however his knees bend (the hips sit on the legs)
            float lowest = 1e9f;
            for (auto& p : parts(a)) lowest = std::min(lowest, p.model[3].y);
            CHECK(lowest > -0.08f && lowest < 0.06f, "...his feet on the floor in his crouch, not in it or over it");
        }
        {   // he never stands still: his guard lives (weight, head, the lead hand pawing)
            float most = 0.f;
            for (float t = 0.f; t < 6.f; t += 0.05f) {
                rig::AbsPose q = rig::absolutePose(a, t);
                most = std::max({most, std::fabs(q.torsoRoll - gp.torsoRoll), std::fabs(q.shL - gp.shL)});
            }
            CHECK(most > 0.1f, "...his guard lives: his head slips, his lead hand paws");
        }
        {   // in motion: thirty seconds of strings at 120 Hz, his drawn body springing after his pose
            glm::vec3 me = ZC + glm::vec3{0, 0, 14};
            Enemy f = freshAbs(); f.abArrive = 0.f; f.position = me + glm::vec3{0, 0, -5}; f.position.y = ZC.y;
            const float H = 1.f / 120.f;
            float worst = 0.f; int blows = 0, home = 0, follows = 0, back = 0;
            rig::AbsPose prev{}; bool have = false;
            srand(77);
            for (int i = 0; i < 120 * 30; ++i) {
                f.update(H, zworld(me));
                rig::tickAbsoluteBody(f, H);
                rig::AbsPose L = rig::absoluteLivePose(f, f.age);
                if (have && !f.staggered())
                    for (int k = 0; k < rig::AB_NCH; ++k) worst = std::max(worst, std::fabs(L.*rig::AB_CH[k] - prev.*rig::AB_CH[k]));
                prev = L; have = true;
                // a straight punch, as it lands: the arm out straight (the drawn arm, not just the goal)
                if (f.abBodySince == 0.f && (f.abBodyLanded == AttackKind::AJAB || f.abBodyLanded == AttackKind::ACROSS)) {
                    ++blows;
                    float el = f.abBodyLanded == AttackKind::AJAB ? L.elL : L.elR;
                    home += el < 0.6f;
                }
                if (f.abBodySince < 0.5f && f.abBodySince + H >= 0.5f && f.telegraphTimer <= 0.f && !f.staggered() && !f.abAirborne()) {
                    ++follows;   // half a second after a blow, with nothing new begun: back in his guard
                    rig::AbsPose G = rig::absoluteGuard(f, f.age);
                    back += std::fabs(L.elR - G.elR) < 0.5f && std::fabs(L.elL - G.elL) < 0.5f && std::fabs(L.torsoYaw - G.torsoYaw) < 0.4f;
                }
            }
            std::printf("      his body: worst joint step %.3f rad/tick; straights landing extended %d/%d; back to guard %d/%d\n",
                        worst, home, blows, back, follows);
            CHECK(worst < 0.4f, "...nothing he does snaps: no joint past 0.4 rad in a 1/120 s tick (a punch's elbow whips at ~0.3)");
            CHECK(blows >= 3 && home == blows, "...a straight punch is at full stretch as it lands (the blow you see is the blow that hits)");
            CHECK(follows == 0 || back * 10 >= follows * 8, "...and after a blow he gathers himself back into his guard");
        }
        {   // the blade is held like a sword: its direction (in his own frame: +Z toward you, -X his right)
            auto bladeDir = [](AttackKind k, bool blow) {
                Enemy b(EnemyType::ABSOLUTE, glm::vec3{0.f});
                b.abBlade = true; b.spawnTimer = 0.f; b.state = EnemyState::ACTIVE;
                if (k != AttackKind::NONE) { b.attack = k; b.telegraphDuration = 1.f; b.telegraphTimer = blow ? 0.001f : 0.3f; }
                for (int i = 0; i < 400; ++i) rig::tickAbsoluteBody(b, 1.f / 120.f);
                std::vector<BoxInstance> out; rig::Rig r{out};
                rig::buildAbsolute(b, 1.f, r, glm::mat4(1.f));
                return glm::normalize(glm::vec3(out.back().model[1]) * -1.f);   // (the edge: the last box, along its -Y)
            };
            glm::vec3 g = bladeDir(AttackKind::NONE, false), sw = bladeDir(AttackKind::ASWEEP, true), bh = bladeDir(AttackKind::ACROSS, true);
            glm::vec3 lu = bladeDir(AttackKind::ALUNGE, true), cr = bladeDir(AttackKind::ACRESCENT, true);
            CHECK(g.y > 0.6f && g.z > 0.2f, "his sword guard: the blade up in front of him, point toward you");
            CHECK(std::fabs(sw.y) < 0.35f && sw.x > 0.35f, "the sweep lands flat, the blade across to his left (a cut you see, not a poke)");
            CHECK(std::fabs(bh.y) < 0.35f && bh.x < -0.5f, "the backhand (his cross, with the sword) lands flat out to his right");
            CHECK(lu.z > 0.9f, "the lunge lands point first");
            CHECK(cr.y < -0.3f && cr.z > 0.5f, "the overhead cut lands down and forward");
        }
        glm::vec3 gt = tint(AttackKind::AUPPER);
        CHECK(gt.y > gt.x * 0.72f, "the gold tell is a pale gold (it doesn't sink into the orange sky)");
        AABB hb;
        CHECK(headBox(a, hb) && hb.min.y > 1.75f && hb.max.y < 2.7f, "his head can be shot (a head box where his head is)");
    }

    // ---------------------------------------------------------------- THE ABSOLUTE against scripted players
    {
        auto run = [&](BossSim::Policy p, float limit) {
            srand(4242);
            BossSim s(N, g, ZA, EnemyType::ABSOLUTE, p);
            while (s.t < limit && s.bossAlive()) s.step(DT);
            return s;
        };
        BossSim solid = run(BossSim::SOLID, 700.f);
        const float form1 = solid.formAt, form2 = solid.formAt >= 0.f ? solid.t - solid.formAt : -1.f;
        std::printf("      absolute: solid fells him in %.0f s, then MORNINGSTAR in %.0f s more (alive %d), taking %.0f (%.0f of it to MORNINGSTAR); parries %d\n",
                    form1, form2, (int)solid.bossAlive(), solid.damageTaken, solid.taken2, solid.parries);
        std::printf("      lowest health against MORNINGSTAR: %.0f; hurt by his body %.0f, rings %.0f, fissures %.0f, lanes %.0f, the collapse %.0f\n",
                    solid.lowest2, solid.msBy[0], solid.msBy[1], solid.msBy[2], solid.msBy[3], solid.msBy[4]);
        std::printf("      solid hurt by: melee %.0f, beams %.0f, ground %.0f, serpents/mirages %.0f, crescents %.0f\n", solid.dmgMelee, solid.dmgBeam, solid.dmgGround, solid.dmgWhite, solid.dmgShot);
        std::printf("      solid with the sword's healing: lowest health %.0f\n", solid.lowest);
        // One fight is a knife edge: the sim is chaotic, so a compiler's rounding (Linux vs macOS) or a
        // pose tweak can turn a narrow win into a narrow loss. So the fight is judged over 25: the same
        // player starting up to 3 m from the usual spot. MORNINGSTAR is judged from his checkpoint (full
        // health), as a death to him retries there in the game
        std::vector<float> f1s, f2s;
        int fights = 0, live1 = 0, live2 = 0, unfinished = 0;
        for (int r = 0; r <= 3; ++r)
            for (int k = 0; k < (r ? 8 : 1); ++k) {
                srand(4242);
                BossSim s(N, g, ZA, EnemyType::ABSOLUTE, BossSim::SOLID);
                float a = k * 0.785398f + r * 0.3f;
                s.feet += glm::vec3{std::cos(a), 0.f, std::sin(a)} * (float)r;
                while (s.t < 700.f && s.bossAlive() && s.formAt < 0.f) s.step(DT);
                ++fights;
                if (s.formAt < 0.f) { ++unfinished; continue; }
                f1s.push_back(s.formAt);
                live1 += s.lowest > 0.f;
                s.health = 100.f; s.lowest2 = 100.f;   // MORNINGSTAR's checkpoint
                const float from = s.t;
                while (s.t < from + 700.f && s.bossAlive()) s.step(DT);
                if (s.bossAlive()) { ++unfinished; continue; }
                f2s.push_back(s.t - from);
                live2 += s.lowest2 > 0.f;
            }
        std::sort(f1s.begin(), f1s.end()); std::sort(f2s.begin(), f2s.end());
        const float med1 = f1s.empty() ? -1.f : f1s[f1s.size() / 2], med2 = f2s.empty() ? -1.f : f2s[f2s.size() / 2];
        std::printf("      over %d fights: the Absolute falls in %.0f s (median; %.0f-%.0f), MORNINGSTAR in %.0f s more (%.0f-%.0f);"
                    " survived the Absolute in %d, MORNINGSTAR (from his checkpoint) in %d (%d unfinished)\n", fights, med1, f1s.empty() ? 0.f : f1s.front(), f1s.empty() ? 0.f : f1s.back(),
                    med2, f2s.empty() ? 0.f : f2s.front(), f2s.empty() ? 0.f : f2s.back(), live1, live2, unfinished);
        CHECK(unfinished == 0, "a solid player (sword, half the parries) always gets through both forms");
        CHECK(live1 * 2 > fights && live2 * 2 > fights,
              "...and survives most fights against each form, healing by the sword as the game does: hard, never impossible");
        CHECK(med1 >= 150.f && med1 <= 180.f, "a solid player fells the Absolute in 150-180 s (the median fight)");
        CHECK(med2 >= med1 && med2 <= 240.f, "...and MORNINGSTAR takes as long again or longer (up to 240 s)");
        BossSim dodger = run(BossSim::DODGER, 2000.f);
        std::printf("      absolute: guns-only dodger kills in %.0f s (alive %d), taking %.0f\n", dodger.t, (int)dodger.bossAlive(), dodger.damageTaken);
        std::printf("      dodger hurt by: melee %.0f, beams %.0f, ground %.0f, serpents/mirages %.0f, crescents %.0f\n", dodger.dmgMelee, dodger.dmgBeam, dodger.dmgGround, dodger.dmgWhite, dodger.dmgShot);
        std::printf("      dodger: MORNINGSTAR from %.0f s\n", dodger.formAt);
        CHECK(!dodger.bossAlive() && dodger.t > solid.t, "a dodge-only, guns-only player can still win, more slowly");
        BossSim still = run(BossSim::STILL, 30.f);
        std::printf("      absolute: a player standing still takes %.0f in 30 s\n", still.damageTaken);
        CHECK(still.damageTaken >= 100.f, "a player who stands still is dead inside 30 s");
        bool punished = true;
        for (auto pol : {BossSim::CORNER, BossSim::PERCH, BossSim::KITE, BossSim::RANGED})
            for (int ph = 1; ph <= 3; ++ph) {
                srand(77 + ph);
                BossSim c(N, g, ZA, EnemyType::ABSOLUTE, pol);
                c.boss.abArrive = 0.f; c.boss.abPhase = ph; c.boss.abBlade = ph == 3;
                c.boss.health = c.boss.maxHealth * (ph == 1 ? 1.f : ph == 2 ? 0.55f : 0.2f);
                float first = -1.f;
                while (c.t < 8.f && first < 0.f) { c.step(DT); if (c.damageTaken > 0.f) first = c.t; }
                std::printf("      cheese %d phase %d: first hurt at %.1f s\n", (int)pol, ph, first);
                if (first < 0.f || first > 5.f) punished = false;
            }
        CHECK(punished, "every cheese (the rim, a pillar top, edge-kiting, ranged-only) is punished within 5 s, in every phase");
        bool msPunished = true;
        for (auto pol : {BossSim::CORNER, BossSim::PERCH, BossSim::KITE, BossSim::RANGED, BossSim::STILL})
            for (int ph = 4; ph <= 6; ++ph) {
                srand(91 + ph);
                BossSim c(N, g, ZA, EnemyType::ABSOLUTE, pol);
                c.boss.abArrive = 0.f; c.boss.abSwallow = 0.01f; c.boss.abAlt = c.boss.abAltWant = Enemy::MS_UP;
                float first = -1.f, hpAt = ph == 4 ? 1.f : ph == 5 ? 0.5f : 0.11f, from = -1.f;
                while (c.t < 20.f && first < 0.f) {
                    c.step(DT);
                    if (from < 0.f && c.boss.morning() && c.boss.abCur != AB_M_ENTRY) {   // his entrance done: from here
                        from = c.t; c.damageTaken = 0.f;
                        if (c.boss.health > c.boss.maxHealth * hpAt) c.boss.health = c.boss.maxHealth * hpAt;
                    }
                    if (from >= 0.f && c.damageTaken > 0.f) first = c.t - from;
                }
                std::printf("      MORNINGSTAR cheese %d phase %d: first hurt %.1f s after his entrance\n", (int)pol, ph, first);
                if (first < 0.f || first > 7.f) msPunished = false;
            }
        CHECK(msPunished, "...and against MORNINGSTAR (his entrance included), in every one of his phases");
    }

    // ---------------------------------------------------------------- MORNINGSTAR: the blow that should kill him doesn't
    {
        srand(5);
        BossSim s(N, g, ZA, EnemyType::ABSOLUTE, BossSim::STILL);
        s.boss.abArrive = 0.f; s.boss.abPhase = 3; s.boss.abJudged = true; s.boss.health = 10.f;
        const float firstMax = s.boss.maxHealth;
        bool killed = s.boss.takeDamage(500.f);
        CHECK(!killed && s.boss.alive && !s.boss.targetable() && s.boss.abSwallow > 0.f,
              "the Absolute's lethal blow doesn't kill him: he's alive, untouchable, and the swallow begins");
        bool swallowEv = false, formEv = false; float formT = -1.f;
        while (s.t < Enemy::MS_SWALLOW + 4.f) {
            s.step(DT);
            swallowEv |= s.boss.ev.msSwallow;
            if (s.boss.ev.msForm) { formEv = true; formT = s.t; }
        }
        std::printf("      MORNINGSTAR arrives %.2f s after the swallow begins\n", formT);
        CHECK(swallowEv && formEv && std::fabs(formT - Enemy::MS_SWALLOW) < 0.1f, "...it runs MS_SWALLOW, then he's MORNINGSTAR (one event each)");
        CHECK(s.boss.morning() && s.boss.scale == Enemy::MS_SCALE && s.boss.abPhase == 4 &&
              std::fabs(s.boss.maxHealth - firstMax * Enemy::MS_HP / statsOf(EnemyType::ABSOLUTE).health) < 1.f,
              "...1.65 times the size, phase 4, a fresh bar of MS_HP (the difficulty's share carried over)");
        CHECK(s.damageTaken > 0.f, "...and his entrance comes down on a player who stands there");
        while (!s.boss.targetable() && s.t < 30.f) s.step(DT);
        s.boss.health = 1.f;
        CHECK(s.boss.takeDamage(5.f) && !s.boss.alive, "MORNINGSTAR's lethal blow is lethal");
    }

    // ---------------------------------------------------------------- the encore's sounds
    {
        bool all = true;
        for (const char* n : {"sword_draw", "sword_swing", "sword_hit", "sword_thrust", "sword_guard", "sword_block", "sword_break", "ab_ting", "ab_thrum"}) {
            std::ifstream f(std::string("assets/sfx/") + n + ".wav", std::ios::binary);
            if (!mixEntry(n) || !f.good()) { all = false; std::printf("      missing sound: %s\n", n); }
        }
        CHECK(all, "every sound the sword and his tells play is built and in the mix");
    }
    {
        bool all = true;
        for (const std::string& n : abAllSounds()) {
            std::ifstream f("assets/sfx/" + n + ".wav", std::ios::binary);
            if (!mixEntry(n) || !f.good()) { all = false; std::printf("      missing sound: %s\n", n.c_str()); }
        }
        CHECK(all, "every sound THE ABSOLUTE's fight plays is built and in the mix");
        auto seconds = [](const std::string& n) {
            std::ifstream f("assets/sfx/" + n + ".wav", std::ios::binary | std::ios::ate);
            return f.good() ? (float)((long)f.tellg() - 44) / 2.f / 44100.f : 0.f;
        };
        CHECK(std::fabs(seconds("ab_ray_beam") - AbsoluteHazards::RAY_TIME) < 0.02f &&
              std::fabs(seconds("ab_zray_beam") - AbsoluteHazards::ZRAY_TIME) < 0.02f, "each beam sounds exactly as long as it burns");
        bool lens = true;
        for (AttackKind k : abAllAttacks()) { const AbFeel& f = abFeel(k); if (f.wind && std::fabs(seconds(f.wind) - f.windLen) > 0.02f) { lens = false; std::printf("      %s is %.2f s\n", f.wind, seconds(f.wind)); } }
        CHECK(lens, "the feel table knows each wind-up's real length (it stretches them to the tell)");
        std::ifstream v1("assets/sfx/ab_boom_jab_1.wav"), v2("assets/sfx/ab_boom_jab_2.wav");
        CHECK(v1.good() && v2.good(), "the jab's boom has variants: a FLURRY drums");
        int lines = 0; for (auto& v : voiceBank()) lines += std::string(v.name) == "v_absolute_line";
        CHECK(lines == 1, "his lines have a choir phrase in the voice bank");
    }

    // ---------------------------------------------------------------- review fixes
    {
        Sword s; s.update(DT, {false, false, true, true});
        CHECK(s.receive(Sword::Strike::AREA, true) == Sword::Guarded::NONE && std::fabs(s.guard - 100.f) < 1e-3f,
              "red means get out: an area can't be blocked (or parried)");
        Sword d; d.drawn();
        run(d, {false, true, false, false}, Sword::CHARGE_TIME + 0.1f);   // LMB still held from before the draw
        Sword::Out o = d.update(DT, {false, false, false, false});
        CHECK(!o.thrust, "drawing with the mouse held never fires a thrust nobody pressed for");
        CHECK(act2RunRecords(true, false, false) && !act2RunRecords(false, false, false) && !act2RunRecords(true, true, false) &&
              !act2RunRecords(true, false, true), "ACT II and the Zenith keep times only for a run from the act's start (no god, no practice)");
        Enemy e = freshAbs(); e.abArrive = 0.f;
        e.health = e.maxHealth * 0.2f; e.staggerTimer = 0.f;
        e.update(DT, zworld(ZC + glm::vec3{0, 0, 10}));
        CHECK(e.abPhase == 2 && e.ev.abPhase == 2 && e.ev.abHalo, "a burst from 60 % to under 25 % still takes the ascension first");
        for (int i = 0; i < 60 * 6; ++i) e.update(DT, zworld(ZC + glm::vec3{0, 0, 10}));
        CHECK(e.abPhase == 3, "...and the blade after it");
        Enemy j = freshAbs(); j.abArrive = 0.f; j.abPhase = 3; j.abBlade = true;
        j.attack = AttackKind::AJAB; j.telegraphTimer = DT * 0.5f; j.telegraphDuration = 0.3f; j.abCur = AB_TOO_SLOW; j.abStep = 1;
        j.health = j.maxHealth * 0.05f;
        j.update(DT, zworld(ZC + glm::vec3{0, 0, 10}));
        CHECK(j.ev.abJudgeMark && !j.ev.abJudge && !j.staggered(), "the last judgment never lands on the tick it begins");
        CHECK(j.ev.abLine == -1, "the last judgment says nothing now");
        AbsoluteHazards two; two.addRay(ZC, ZC); two.update(0.5f, ZC + glm::vec3{0, 0, 9}, 1.8f, true, &N, ZC); two.addRay(ZC, ZC);
        CHECK(two.freshRay() == 1, "a second ray, fired while the first still burns, can be parried");
    }

    // ---------------------------------------------------------------- the deferred minors
    {
        bool onDais = true, offDais = true;
        for (int k = 0; k < 72; ++k) {
            float a = k * 0.0872665f;
            glm::vec3 in = ZC + glm::vec3{std::cos(a) * 8.6f, 3.f, std::sin(a) * 8.6f};
            glm::vec3 out = ZC + glm::vec3{std::cos(a) * 9.4f, 3.f, std::sin(a) * 9.4f};
            if (std::fabs(N.groundAt(in.x, in.z, in.y) - (ZC.y + ZEN_DAIS_UP)) > 0.01f) onDais = false;
            float fo = N.groundAt(out.x, out.z, out.y);
            bool step = std::fabs(fo - (ZC.y + 0.6f)) < 0.01f;   // the four steps up onto it
            if (!step && std::fabs(fo - ZC.y) > 0.01f) offDais = false;
        }
        CHECK(onDais && offDais, "the Dais you stand on is the Dais you see: solid to r 8.6 all round, gone by r 9.4");
        int inside = 0;
        for (int k = 0; k < 8; ++k) {
            glm::vec3 pil = zenithPillar(k), out = glm::normalize(glm::vec3{pil.x - ZC.x, 0, pil.z - ZC.z});
            Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_KNEEL;
            e.position = pil - out * 5.f; e.position.y = ZC.y;
            glm::vec3 me = pil + out * 4.f; me.y = ZC.y;
            for (int i = 0; i < 40; ++i) {
                e.update(DT, zworld(me));
                if (N.zenithPillarAt(e.position, 0.f) >= 0) ++inside;
            }
        }
        CHECK(inside == 0, "a dash with a pillar in the way blinks past it, never through it");
        Records r;
        CHECK(act2Subtitle(r, false) == "DEFEAT THE SOVEREIGN TO UNLOCK" && act2Subtitle(r, true) == "5 ARENAS - BENEATH THE ECLIPSE",
              "the ACT II row: locked, then the act");
        r.bestAct2 = 1534.2f;
        CHECK(act2Subtitle(r, true) == "5 ARENAS - BENEATH THE ECLIPSE   BEST " + formatTime(1534.2f), "...with the act's best time once you have one");
        r.bestZenith = 1712.f;
        CHECK(act2Subtitle(r, true) == "5 ARENAS - AND WHAT WAITS ABOVE   BEST " + formatTime(1534.2f) + "   !!!!! " + formatTime(1712.f),
              "...and the encore's (!!!!!) once it's cleared");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
