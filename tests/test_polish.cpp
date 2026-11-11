// The polish pass (headless): what a fight leaves on the room (render/Marks.h), the room
// answering a blast (world/Reactions.h) and the places heard from where they are
// (audio/WorldEmitters.h, world/WorldSound.h).
#include "render/Marks.h"
#include "world/Reactions.h"
#include "world/WorldSound.h"
#include "world/LevelAct2.h"
#include "world/LevelGauntlet.h"
#include <cstdio>

static int failures = 0;
#define CHECK(c, msg) do { bool ok = (c); std::printf("%s %s\n", ok ? "ok:" : "FAIL:", msg); failures += !ok; } while (0)

int main() {
    using marks::Field; using marks::Mark; using marks::Vert; using marks::lifeOf; using marks::faceNormal;
    using marks::FADE_IN; using marks::CASING_LIFE; using marks::CASING_FADE;
    const glm::vec3 UP{0.f, 1.f, 0.f};

    // ---- Marks: caps, the oldest goes first, fades, merging, the detail setting ----
    {
        Field f;
        for (int i = 0; i < 1000; ++i) { f.add(marks::Kind::PIT, {(float)i, 0.f, 0.f}, UP, 0.08f); f.tick(0.01f); }
        CHECK(f.markCount() == Field::MAX_MARKS, "a thousand rounds leave no more marks than the cap");
        bool newestKept = false, oldestGone = true;
        for (auto& m : f.marks) if (m.alive) { newestKept |= std::fabs(m.pos.x - 999.f) < 0.01f; oldestGone &= m.pos.x > 500.f; }
        CHECK(newestKept && oldestGone, "at the cap the oldest mark makes room for the newest");
        for (int i = 0; i < 300; ++i) f.addCasing({(float)i, 0.f, 0.f}, 0.f, {1.f, 1.f, 1.f}, 0.05f);
        CHECK(f.casingCount() == Field::MAX_CASINGS, "spent brass is capped too");
    }
    {
        Field f;
        Mark* a = f.add(marks::Kind::PIT, {1.f, 0.f, 1.f}, UP, 0.08f);
        f.tick(20.f);
        Mark* b = f.add(marks::Kind::PIT, {1.01f, 0.f, 1.01f}, UP, 0.11f);
        CHECK(a == b && f.markCount() == 1 && a->age <= FADE_IN && a->size == 0.11f, "a round into an old pit renews it instead of stacking");
        f.add(marks::Kind::PIT, {1.01f, 0.f, 1.01f}, {1.f, 0.f, 0.f}, 0.08f);
        CHECK(f.markCount() == 2, "the same spot on another face is its own mark");
        f.add(marks::Kind::SCORCH, {1.f, 0.f, 1.f}, UP, 1.5f); f.add(marks::Kind::SCORCH, {1.f, 0.f, 1.f}, UP, 1.5f);
        CHECK(f.markCount() == 4, "blasts overlap: each leaves its own soot");
    }
    {
        Field f;
        Mark* m = f.add(marks::Kind::SCORCH, {0.f, 0.f, 0.f}, UP, 1.5f);
        CHECK(Field::strength(*m) == 0.f, "a mark starts from nothing (no pop in)");
        float last = 0.f, peak = 0.f; bool smooth = true, wholeMidway = false;
        const float dt = 1.f / 60.f;
        for (float t = 0.f; t < lifeOf(marks::Kind::SCORCH) + 1.f; t += dt) {
            f.tick(dt);
            const float s = Field::strength(*m);
            smooth &= std::fabs(s - last) < 0.3f; peak = std::max(peak, s);
            if (std::fabs(t - lifeOf(marks::Kind::SCORCH) * 0.5f) < dt) wholeMidway = s > 0.999f;
            last = s;
        }
        CHECK(peak > 0.999f && wholeMidway && smooth && last == 0.f && !m->alive, "a mark holds, fades out smoothly over the last of its life, and is gone");
        CHECK(f.markCount() == 0, "nothing is left once every mark has lived its life");
    }
    {
        Field f;
        for (int i = 0; i < 150; ++i) { f.add(i % 3 ? marks::Kind::PIT : marks::Kind::STAIN, {(float)i, 0.f, 0.f}, UP, 0.1f); f.tick(0.01f); }
        for (int i = 0; i < 40; ++i) f.addCasing({(float)i, 0.f, 0.f}, 0.f, {1.f, 1.f, 1.f}, 0.05f);
        f.setDetail(1);
        CHECK(f.markCount() == Field::markCap(1) && f.casingCount() == Field::casingCap(1), "a lower AMBIENT DETAIL trims what lies about at once");
        f.setDetail(0);
        bool onlyStains = true; for (auto& m : f.marks) if (m.alive) onlyStains &= m.kind == marks::Kind::STAIN;
        CHECK(onlyStains && f.casingCount() == 0 && f.markCount() <= Field::markCap(0), "AMBIENT DETAIL OFF keeps only the stains it always drew");
        CHECK(f.add(marks::Kind::PIT, {0.f, 5.f, 0.f}, UP, 0.1f) == nullptr && f.add(marks::Kind::STAIN, {0.f, 5.f, 0.f}, UP, 0.4f) != nullptr, "...and leaves no new pits, only stains");
        f.addCasing({0.f, 0.f, 0.f}, 0.f, {1.f, 1.f, 1.f}, 0.05f);
        CHECK(f.casingCount() == 0, "...and no brass");
    }
    {
        Field f;
        f.addCasing({0.f, 0.f, 0.f}, 0.4f, {1.f, 1.f, 1.f}, 0.05f);
        CHECK(Field::shown(f.casings[0]) == 0.f, "a casing isn't on the floor until it has had time to fall");
        f.tick(0.5f); f.tick(0.1f);
        CHECK(Field::shown(f.casings[0]) == 1.f, "then it lies there");
        f.tick(CASING_LIFE - CASING_FADE * 0.5f - 0.1f);
        const float k = Field::shown(f.casings[0]);
        CHECK(k > 0.3f && k < 0.7f, "it shrinks away at the end rather than vanishing");
        f.tick(CASING_FADE);
        CHECK(f.casingCount() == 0, "and is gone after its time");
    }
    {
        Field f; std::vector<Vert> v;
        f.add(marks::Kind::PIT, {0.f, 0.f, 0.f}, UP, 0.1f); f.add(marks::Kind::PIT, {0.f, 2.f, 5.f}, {0.f, 0.f, -1.f}, 0.1f); f.add(marks::Kind::PIT, {500.f, 0.f, 0.f}, UP, 0.1f);
        f.tick(0.5f);
        CHECK(f.build(v, {0.f, 1.7f, 0.f}) == 12, "marks out of sight range aren't drawn");
        bool flat = true, onWall = true;
        for (int i = 0; i < 6; ++i) flat &= std::fabs(v[i].pos.y - v[0].pos.y) < 1e-5f;
        for (int i = 6; i < 12; ++i) onWall &= std::fabs(v[i].pos.z - v[6].pos.z) < 1e-5f;
        CHECK(flat && onWall, "a mark lies flat on the face it is on (floor or wall)");
        const AABB box{{0.f, 0.f, 0.f}, {2.f, 3.f, 4.f}};
        CHECK(faceNormal(box, {2.f, 1.f, 1.f}) == glm::vec3(1.f, 0.f, 0.f) && faceNormal(box, {1.f, 3.f, 2.f}) == UP && faceNormal(box, {1.f, 1.f, 0.f}) == glm::vec3(0.f, 0.f, -1.f),
              "a point on a box knows which face it is on");
    }

    // ---- Reactions: the neon answering a blast ----
    {
        react::Field r;
        const AABB strip{{4.f, 3.f, -1.f}, {4.2f, 3.2f, 1.f}}, far{{80.f, 3.f, 0.f}, {80.2f, 3.2f, 2.f}};
        CHECK(r.lightScale(strip, 0, 1.f, false) == 1.f, "a strip nothing has shaken is steady");
        r.add({0.f, 1.f, 0.f}, 1.f);
        float lo = 9.f, hi = 0.f; bool farSteady = true;
        for (int i = 0; i < 40; ++i) {
            const float s = r.lightScale(strip, 3, 10.f + (float)i / 60.f, false);
            lo = std::min(lo, s); hi = std::max(hi, s);
            farSteady &= r.lightScale(far, 4, 10.f + (float)i / 60.f, false) == 1.f;
        }
        CHECK(lo < 0.6f && hi > 0.9f && lo >= 0.f && hi <= 1.2f, "a blast makes the neon near it stutter, between out and a little over");
        CHECK(farSteady, "a strip out of the blast's reach doesn't notice");
        float calmLo = 9.f, calmHi = 0.f;
        for (int i = 0; i < 40; ++i) { const float s = r.lightScale(strip, 3, 10.f + (float)i / 60.f, true); calmLo = std::min(calmLo, s); calmHi = std::max(calmHi, s); }
        CHECK(calmHi - calmLo < 1e-6f && calmLo >= 0.7f && calmLo < 1.f, "REDUCE FLASHING: the strip dips once, it doesn't stutter");
        // The same at any frame rate: the pattern is a function of the clock, the fade of the jolt's age
        react::Field a, b; a.add({0.f, 1.f, 0.f}, 1.f); b.add({0.f, 1.f, 0.f}, 1.f);
        for (int i = 0; i < 12; ++i) a.tick(1.f / 30.f);
        for (int i = 0; i < 96; ++i) b.tick(1.f / 240.f);
        CHECK(std::fabs(a.lightScale(strip, 3, 5.4f, false) - b.lightScale(strip, 3, 5.4f, false)) < 0.02f, "the stutter is the same at 30 and 240 Hz");
        for (int i = 0; i < 60; ++i) r.tick(1.f / 60.f);
        CHECK(r.jolts.empty() && r.lightScale(strip, 3, 11.f, false) == 1.f, "a second later the room is steady again");
        for (int i = 0; i < 50; ++i) r.add({(float)i, 0.f, 0.f}, 1.f);
        CHECK((int)r.jolts.size() == react::MAX_JOLTS, "jolts are capped");
    }

    // ---- World emitters ----
    {
        using namespace worldsound;
        bool level = true, finite = true, same = true, edges = true, differ = true;
        for (int k = 0; k < (int)Kind::COUNT; ++k)
            for (int v = 0; v < VARIANTS; ++v) {
                const auto a = build((Kind)k, v), b = build((Kind)k, v);
                same &= a == b;
                for (float s : a) finite &= std::isfinite(s) && std::fabs(s) <= 0.98f;
                level &= std::fabs(shortTermDb(a.data(), a.size(), RATE) - levelDb((Kind)k)) < 1.5f;
                edges &= std::fabs(a.front()) < 0.002f && std::fabs(a.back()) < 0.002f;
                if (v > 0) differ &= a != build((Kind)k, 0);
            }
        CHECK(same && finite, "every emitter grain is finite, unclipped and the same each time it is built");
        CHECK(level, "each grain sits at its kind's level (the room, under the fight)");
        CHECK(edges, "grains start and end at silence (they overlap: no clicks)");
        CHECK(differ, "a kind's variants differ");

        Director d;
        for (int i = 0; i < 12; ++i) d.add({(float)i * 4.f, 0.f, 0.f}, Kind::LAVA);
        d.add({500.f, 0.f, 0.f}, Kind::HUM);
        int cues = 0; bool nearOnly = true, placed = true;
        for (int i = 0; i < 60 * 20; ++i)
            for (const Cue& c : d.tick(1.f / 60.f, {0.f, 1.7f, 0.f})) { ++cues; nearOnly &= c.pos.x <= 8.01f; placed &= c.kind == Kind::LAVA && c.volume > 0.f; }
        CHECK(cues > 15 && cues <= (int)(20.f * Director::MAX_PER_SECOND) + 2, "emitters near you sound, never more than the budget of grains a second");
        CHECK(nearOnly && placed, "only the nearest three sound, each from where it is");
        Director off; off.add({1.f, 0.f, 0.f}, Kind::FAN);
        int offCues = 0; for (int i = 0; i < 600; ++i) offCues += (int)off.tick(1.f / 60.f, {0.f, 0.f, 0.f}, 0.f).size();
        CHECK(offCues == 0, "AMBIENT DETAIL OFF: the emitters are silent");
        Director farOnly; farOnly.add({100.f, 0.f, 0.f}, Kind::WATER);
        int farCues = 0; for (int i = 0; i < 600; ++i) farCues += (int)farOnly.tick(1.f / 60.f, {0.f, 0.f, 0.f}).size();
        CHECK(farCues == 0, "an emitter out of range is silent");
        // The same count at 30 and 240 Hz (within one grain)
        auto run = [](float dt) { Director x; for (int i = 0; i < 3; ++i) x.add({(float)i, 0.f, 0.f}, Kind::FAN); int n = 0; for (float t = 0.f; t < 30.f; t += dt) n += (int)x.tick(dt, {0.f, 0.f, 0.f}).size(); return n; };
        const int n30 = run(1.f / 30.f), n240 = run(1.f / 240.f);
        CHECK(std::abs(n30 - n240) <= 3, "the emitters keep the same pace at 30 and 240 Hz");
    }
    {
        // Read off the levels: the Foundry's lava and the Core's reactor are heard; nothing sits outside the level
        struct Named { const char* name; LevelData L; };
        Named levels[] = {{"ACT I", buildLevel()}, {"ACT II", buildAct2Level()}, {"FAST", buildGauntlet()}};
        for (auto& n : levels) {
            worldsound::Director d; gatherWorldEmitters(n.L, d);
            int lava = 0, hum = 0; bool sane = true;
            for (auto& e : d.emitters) { lava += e.kind == worldsound::Kind::LAVA; hum += e.kind == worldsound::Kind::HUM; sane &= std::isfinite(e.pos.x + e.pos.y + e.pos.z) && e.size >= 0.4f && e.size <= 1.6f; }
            char msg[160];
            std::snprintf(msg, sizeof(msg), "%s: %d emitters (%d lava, %d hum), all sane and under the cap", n.name, (int)d.emitters.size(), lava, hum);
            CHECK(sane && d.emitters.size() <= 97 && (n.L.hazards.empty() || lava > 0) && (!n.L.hasReactor || hum > 0), msg);
        }
    }

    std::printf("\n%d polish failures\n", failures);
    return failures ? 1 : 0;
}
