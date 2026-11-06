#include "player/Sword.h"
#include "player/ActionBuffer.h"
#include "ui/DamageTrail.h"
#include <cstdio>

static int failures = 0;
#define CHECK(c, msg) do { bool ok = (c); std::printf("%s %s\n", ok ? "ok:" : "FAIL:", msg); failures += !ok; } while (0)

int main() {
    for (float dt : {1.f / 30, 1.f / 60, 1.f / 144}) {
        Sword sword;
        auto press = sword.update(dt, {true, true, false, false});
        CHECK(press.stroke == 0 && press.contact < 0, "press begins anticipation without dealing damage");
        int contacts = 0; float first = 0;
        for (float t = dt; t < .5f; t += dt) {
            auto o = sword.update(dt, {});
            if (o.contact >= 0) { ++contacts; first = t; CHECK(o.damage == 70, "contact carries the stroke's actual damage"); }
        }
        CHECK(contacts == 1 && first >= Sword::SWING_TIME * Sword::contactPhase(0) - dt && first < .16f,
              "each stroke emits exactly one contact at the visual crossing across frame rates");
    }
    {
        Sword s; s.update(.01f, {true, true, false, false});
        s.update(.03f, {false, false, true, true});
        int contacts = 0;
        for (int i = 0; i < 50; ++i) contacts += s.update(.01f, {}).contact >= 0;
        CHECK(contacts == 0, "guarding before contact cancels the pending cut");
        s.update(.01f, {true, true, false, false}); s.holster();
        CHECK(s.update(.2f, {}).contact < 0, "holstering cancels pending contact");
    }
    {
        Sword s; s.riposteT = .04f;
        s.update(.01f, {true, true, false, false});
        Sword::Out contact;
        for (int i = 0; i < 25; ++i) { auto o = s.update(.01f, {}); if (o.contact >= 0) contact = o; }
        CHECK(contact.riposte && contact.damage == 70.f * Sword::RIPOSTE_MULT && s.riposteT == 0,
              "a riposte started in its window stays empowered through anticipation");
    }
    {
        Sword s; s.update(.01f, {true, true, false, false});
        s.pauseImpact(.025f); float before = s.swingT;
        s.update(.01f, {}); s.update(.01f, {});
        CHECK(s.swingT == before, "local impact pause holds the sword animation");
        s.update(.01f, {});
        CHECK(s.swingT < before && std::fabs(s.swingT - before + .005f) < .0001f,
              "only the unpaused portion advances motion at the pause boundary");
    }
    {
        Sword s; s.update(.01f, {true, true, false, false});
        s.update(.01f, {true, true, false, false});
        int begun = 1, contacts = 0;
        for (int i = 0; i < 100; ++i) { auto o = s.update(.01f, {}); begun += o.stroke >= 0; contacts += o.contact >= 0; }
        CHECK(begun == 2 && contacts == 2, "a buffered combo preserves one contact per attack");
    }
    {
        ActionBuffer b; b.press(2, 1.f);
        CHECK(b.pending(2, 1.08f) && !b.pending(1, 1.08f) && !b.pending(2, 1.11f),
              "input survives a short readiness gap, expires, and belongs to its requested weapon");
        b.clear(); CHECK(!b.pending(2, 1.05f), "consumption or menu/context changes cancel buffered input");
    }
    {
        DamageTrail trail; trail.observe(1.f); trail.observe(.72f); trail.tick(.1f);
        CHECK(trail.health == .72f && trail.value == 1.f, "health drops immediately while the damage chunk briefly holds");
        trail.tick(.2f);
        CHECK(trail.value > .72f && trail.value < .8f, "damage chunk catches up smoothly after its hold");
        trail.observe(.9f); CHECK(trail.value == .9f && trail.health == .9f, "healing and phase refills reset the trail immediately");
        DamageTrail a, b; a.reset(1); b.reset(1); a.observe(.5f); b.observe(.5f);
        a.tick(.4f); for (int i = 0; i < 40; ++i) b.tick(.01f);
        CHECK(std::fabs(a.value - b.value) < .0001f, "boss damage trail timing is independent of frame rate");
    }
    std::printf("%d feel failures\n", failures);
    return failures ? 1 : 0;
}
