// Headless tests for the encore: the sword (Sword.h), the Zenith, THE
// ABSOLUTE's mind and hazards, and his fight against scripted players.
// Built and run by `make test` after test_game.
#include "../src/Sword.h"
#include "../src/GunKit.h"
#include <cstdio>
#include <cmath>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)
static constexpr float DT = 1.f / 60.f;

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
        CHECK(hi.z - lo.z > 0.7f && hi.z - lo.z < 1.1f && hi.x - lo.x < 0.25f, "a blade about 0.9 m long and slim");
        bool glows = false;
        for (auto& p : parts) if (p.tag == 1 && glm::length(p.emissive - gunkit::SWORD_GLOW * 1.6f) < 0.05f) glows = true;
        CHECK(glows, "its cells glow rose-red");
        SwordPose a = gunkit::swordPose(0, 0.f, false, 0.f, 0.f), b = gunkit::swordPose(0, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.x > 0.1f && b.offset.x < -0.1f, "stroke 0 sweeps from right to left");
        a = gunkit::swordPose(1, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(1, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.x < -0.1f && b.offset.x > 0.1f, "stroke 1 sweeps from left to right");
        a = gunkit::swordPose(2, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(2, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.y > b.offset.y + 0.2f, "stroke 2 comes down overhead");
        SwordPose g = gunkit::swordPose(-1, -1.f, true, 0.f, 0.f), r = gunkit::swordPose(-1, -1.f, false, 0.f, 0.f);
        CHECK(std::fabs(g.offset.x) < std::fabs(r.offset.x) && std::fabs(g.roll) > 60.f, "raised: the blade across the middle of the screen");
    }

    // ---------------------------------------------------------------- the sword: where it can be drawn
    {
        CHECK(swordKeyAllowed(false, true, 5, 5), "key 5 in the Zenith draws the sword");
        CHECK(!swordKeyAllowed(false, true, 4, 5) && !swordKeyAllowed(false, false, 5, 5),
              "key 5 outside the Zenith keeps the gun");
        CHECK(swordKeyAllowed(true, false, 0, 5), "--sword allows it anywhere (dev)");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
