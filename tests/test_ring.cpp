// THE KEPT RING (Chapter 6): the sweep and HOLDFAST as numbers. The wall, its
// countdown, the shadow rule, breakers, the double sweep, the Gather, the
// boss's cone, when the next one may come, and the spike's rope and pin.
// No OpenGL context required. `make test-ring`.
#include "world/RingSweep.h"
#include "player/Holdfast.h"
#include "world/LevelRing.h"
#include "world/LevelCold.h"
#include "game/StorySave.h"
#include <cstdio>
#include <string>
static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { ++failures; std::printf("FAIL: %s\n", m); } else std::printf("ok: %s\n", m); } while (0)
constexpr float DT = 1.f / 60;

// Run a sweep to its end with something standing at `feet`. Returns when it was struck (-1: never)
struct Run { float flashAt = -1, begunAt = -1, struckAt = -1, clearedAt = -1, endedAt = -1; int strikes = 0, clears = 0; float lastCount = -1; bool countMonotone = true; float countAtBegin = -1; float frontErrAtZero = 1e9f; };
static Run run(RingSweep& s, std::vector<SweepShelter>& sh, glm::vec3 feet, glm::vec3 vel = glm::vec3{0.f}) {
    Run r; float clock = 0.f, prevCount = 1e9f;
    for (int i = 0; i < 60 * 120 && s.active(); ++i) {
        feet += vel * DT;
        auto ev = s.update(DT, feet, sh); clock += DT;
        if (ev.flash) r.flashAt = clock;
        if (ev.begun) { r.begunAt = clock; r.countAtBegin = s.countdown(feet); }
        if (ev.struck) { if (r.struckAt < 0) r.struckAt = clock; ++r.strikes; r.frontErrAtZero = std::fabs(s.passCoord[s.passed - 1] - s.coord(feet)); }
        if (ev.cleared) { if (r.clearedAt < 0) r.clearedAt = clock; ++r.clears; }
        if (ev.ended) r.endedAt = clock;
        float c = s.countdown(feet);
        if (c >= 0.f) { if (s.passed == 0 && c > prevCount + 1e-3f) r.countMonotone = false; if (s.passed == 0) prevCount = c; r.lastCount = c; }
    }
    return r;
}

int main() {
    const glm::vec3 dir{0.f, 0.f, 1.f};   // the wall travels toward +Z: the Shepherd is at -Z
    auto block = [](float x, float z, float w = 6.f, float h = 5.f) {
        SweepShelter s; s.box = {{x - w * 0.5f, 0.f, z - 1.5f}, {x + w * 0.5f, h, z + 1.5f}}; s.post = {x, 0.f, z + 2.f}; return s;
    };

    {   // ---- one clean wave ----
        std::vector<SweepShelter> sh{block(0, 0)};
        RingSweep s; s.start(sweeptier::far(dir).spec);
        CHECK(s.phase == RingSweep::Phase::FLASH && s.countdown({20, 0, 0}) < 0.f, "the flash comes first, with no number");
        Run r = run(s, sh, {20, 0, 0});
        CHECK(std::fabs(r.begunAt - 4.f) < 0.05f, "the countdown starts when the flash lead is up");
        CHECK(std::fabs(r.countAtBegin - 20.f) < 0.05f, "it starts at the warning");
        CHECK(r.strikes == 1 && std::fabs(r.struckAt - 24.f) < 0.05f, "in the open, the wave arrives when the count reaches zero");
        CHECK(r.frontErrAtZero < 1.5f, "the front is where you stand at zero");
        CHECK(r.countMonotone, "the count only goes down");
        CHECK(r.endedAt > r.struckAt, "it runs on past you, then ends");
        CHECK(!s.active(), "and is idle again");
    }
    {   // ---- the shadow rule ----
        std::vector<SweepShelter> sh{block(0, 0)};
        RingSweep s; s.start(sweeptier::far(dir).spec);
        Run r = run(s, sh, {0, 0, 4});
        CHECK(r.strikes == 0 && r.clears == 1, "behind the shelter, it passes over");
        s.start(sweeptier::far(dir).spec); r = run(s, sh, {0, 0, -4});
        CHECK(r.strikes == 1, "in front of the shelter (its exposed face) is the open");
        s.start(sweeptier::far(dir).spec); r = run(s, sh, {0, 0, 1.5f + RingSweep::SHADOW + 3.f});
        CHECK(r.strikes == 1, "the shadow has an end");
        s.start(sweeptier::far(dir).spec); r = run(s, sh, {5.f, 0, 4});
        CHECK(r.strikes == 1, "beside it is the open");
        s.start(sweeptier::far(dir).spec); r = run(s, sh, {0, 6.f, 4});
        CHECK(r.strikes == 1, "above its top is the open: a jump over the shelter doesn't count");
        s.start(sweeptier::far(dir).spec); r = run(s, sh, {0, 2.f, 4});
        CHECK(r.strikes == 0, "in the air behind it is still behind it");
    }
    {   // ---- moving while it comes ----
        std::vector<SweepShelter> sh;
        RingSweep s; s.start(sweeptier::far(dir).spec);
        Run r = run(s, sh, {0, 0, 0}, {0, 0, -9.f});   // running at the wall
        CHECK(r.strikes == 1 && r.struckAt < 24.f && r.struckAt > 20.f, "running at the wall shortens it a little, never by much");
        CHECK(r.frontErrAtZero < 1.5f, "and it still lands where the front is");
        s.start(sweeptier::far(dir).spec); r = run(s, sh, {0, 0, 0}, {0, 0, 9.f});
        CHECK(r.strikes == 1 && r.struckAt > 24.f && r.struckAt < 29.f, "running from it buys seconds, not safety");
    }
    {   // ---- the double sweep ----
        std::vector<SweepShelter> sh{block(0, 0)};
        auto spec = sweeptier::mid(dir).spec;
        RingSweep s; s.start(spec);
        Run r = run(s, sh, {0, 0, 4});
        CHECK(r.clears == 2 && r.strikes == 0, "two waves pass over a sheltered player");
        s.start(spec);
        bool left = false; int strikes = 0, clears = 0; glm::vec3 feet{0, 0, 4}; int pipsBefore = -1, pipsBetween = -1; float countBetween = -1;
        while (s.active()) {
            auto ev = s.update(DT, feet, sh);
            if (s.phase == RingSweep::Phase::INBOUND && s.passed == 0) pipsBefore = s.wavesLeft();
            if (ev.cleared && !left) { left = true; feet = {12, 0, 4}; pipsBetween = s.wavesLeft(); countBetween = s.countdown(feet); }   // stepped out after the first
            strikes += ev.struck; clears += ev.cleared;
        }
        CHECK(clears == 1 && strikes == 1, "leaving shelter after the first wave: the second takes you");
        CHECK(pipsBefore == 2 && pipsBetween == 1, "the pips count the waves left");
        CHECK(countBetween > spec.waveGap - 0.3f && countBetween <= spec.waveGap + 0.05f, "the number restarts for the second wave");
    }
    {   // ---- breakers ----
        std::vector<SweepShelter> sh{block(0, 0), block(30, 0)};
        sh[1].small = true;
        auto spec = sweeptier::near(dir).spec;
        RingSweep s; s.start(spec);
        std::vector<SweepShelter> a = sh;
        bool litEarly = false, darkLate = false; int strikes = 0; std::vector<int> broke;
        while (s.active()) {
            auto ev = s.update(DT, {30, 0, 4}, a);
            if (s.phase == RingSweep::Phase::FLASH) litEarly = s.shelterLit(a[1]);
            if (s.phase == RingSweep::Phase::INBOUND && s.t > 1.2f && a[1].alive) darkLate = !s.shelterLit(a[1]) && s.shelterLit(a[0]);
            strikes += ev.struck; for (int i : ev.shattered) broke.push_back(i);
        }
        CHECK(litEarly && darkLate, "a small shelter's post is lit on the flash and dark a second into the count");
        CHECK(strikes == 1, "a small shelter does not save you from a breaker");
        CHECK(broke.size() == 1 && broke[0] == 1 && !a[1].alive && a[0].alive, "the breaker shatters the small one and only it");
        a = sh; s.start(spec); Run r = run(s, a, {0, 0, 4});
        CHECK(r.strikes == 0, "the big one still holds");
        a = sh; s.start(sweeptier::far(dir).spec); r = run(s, a, {30, 0, 4});
        CHECK(r.strikes == 0 && a[1].alive, "without breakers a small shelter is a shelter");
    }
    {   // ---- enemies and loose things ----
        std::vector<SweepShelter> sh{block(0, 0)};
        RingSweep s; s.start(sweeptier::far(dir).spec);
        int open = 0, hidden = 0, ahead = 0;
        while (s.active()) {
            s.update(DT, {0, 0, 4}, sh);
            open += s.sweptNow({15, 0, -30}, sh); hidden += s.sweptNow({0.5f, 0, 5}, sh); ahead += s.sweptNow({15, 0, 60}, sh);
        }
        CHECK(open == 1, "an enemy in the open is swept exactly once, when the front crosses it");
        CHECK(hidden == 0, "an enemy in shelter is not");
        CHECK(ahead == 1, "the front carries on past the player and takes what is beyond");
    }
    {   // ---- the Gather ----
        std::vector<SweepShelter> sh;
        RingSweep s; s.start(sweeptier::gather(-dir));
        CHECK(!s.lethal(), "the Gather throws; it does not kill");
        Run r = run(s, sh, {0, 0, 0});
        CHECK(r.strikes == 1, "it still reaches you in the open");
        CHECK(glm::dot(s.spec.dir, -dir) > 0.99f, "and travels toward the Shepherd");
    }
    {   // ---- the aimed sweep ----
        std::vector<SweepShelter> sh{block(0, -10)};
        const glm::vec3 origin{0, 0, -40};
        RingSweep s; s.start(sweeptier::aimed(origin, {0, 0, 0}, 4.f));
        Run r = run(s, sh, {12, 0, 0}, {0, 0, 0});
        CHECK(r.begunAt >= 0 && std::fabs(r.countAtBegin - 4.f) < 0.05f, "the cone's count starts at once, at its warning");
        s.start(sweeptier::aimed(origin, {0, 0, 0}, 4.f)); r = run(s, sh, {12, 0, 0});
        CHECK(r.strikes == 1, "inside the cone, in the open: struck");
        s.start(sweeptier::aimed(origin, {0, 0, 0}, 4.f)); r = run(s, sh, {30, 0, 0});
        CHECK(r.strikes == 0 && r.clears == 1, "outside the cone: it goes by");
        s.start(sweeptier::aimed(origin, {0, 0, 0}, 4.f)); r = run(s, sh, {0, 0, -5});
        CHECK(r.strikes == 0, "behind a fin, toward its origin: sheltered");
        // On the beat: however you move, a clocked sweep lands at its moment
        s.start(sweeptier::aimed(origin, {0, 0, 0}, 4.f)); r = run(s, sh, {12, 0, 0}, {0, 0, -3.f});   // (not so far that you leave the cone)
        CHECK(r.strikes == 1 && std::fabs(r.struckAt - 4.f) < 0.05f, "a clocked sweep lands on its moment even if you run at it");
        s.start(sweeptier::aimed(origin, {0, 0, 0}, 4.f)); r = run(s, sh, {12, 0, 0}, {0, 0, 9.f});
        CHECK(r.strikes == 1 && std::fabs(r.struckAt - 4.f) < 0.05f, "...or away from it");
    }
    {   // ---- when the next one comes ----
        SweepSchedule a; a.arm(90.f, 150.f, 7u);
        float clock = 0; std::vector<float> fired;
        for (int i = 0; i < 60 * 3600; ++i) { clock += DT; if (a.tick(DT, false, false)) fired.push_back(clock); }
        bool inWindow = fired.size() > 10;
        for (size_t i = 1; i < fired.size(); ++i) { float g = fired[i] - fired[i - 1]; if (g < 90.f - 0.1f || g > 150.f + 0.1f) inWindow = false; }
        CHECK(inWindow, "gaps stay inside the window");
        bool varied = false; for (size_t i = 2; i < fired.size(); ++i) if (std::fabs((fired[i] - fired[i - 1]) - (fired[1] - fired[0])) > 2.f) varied = true;
        CHECK(varied, "and are not all the same");
        SweepSchedule b; b.arm(90.f, 150.f, 7u); b.timer = 1.f; b.hold = 0.f;
        b.checkpoint(); clock = 0; float at = -1;
        for (int i = 0; i < 60 * 60 && at < 0; ++i) { clock += DT; if (b.tick(DT, false, false)) at = clock; }
        CHECK(at >= SweepSchedule::GRACE - 0.05f, "never within ten seconds of a checkpoint");
        SweepSchedule c; c.arm(90.f, 150.f, 7u); c.timer = 1.f; c.hold = 0.f; clock = 0; at = -1;
        for (int i = 0; i < 60 * 60 && at < 0; ++i) { clock += DT; if (c.tick(DT, false, clock < 5.f)) at = clock; }
        CHECK(at >= 5.f + SweepSchedule::GRACE - 0.05f, "never while blocked, and not the moment the block lifts");
        SweepSchedule d; d.arm(90.f, 150.f, 7u); d.timer = 1.f; d.hold = 0.f; bool during = false;
        for (int i = 0; i < 60 * 30; ++i) during |= d.tick(DT, true, false);
        CHECK(!during, "never while one is under way");
        SweepSchedule e1, e2; e1.arm(100.f, 100.f, 3u); e2.arm(100.f, 100.f, 3u); e1.hold = e2.hold = 0; float t1 = -1, t2 = -1; clock = 0;
        for (int i = 0; i < 60 * 200; ++i) { clock += DT; if (t1 < 0 && e1.tick(DT, false, false, 1.f)) t1 = clock; if (t2 < 0 && e2.tick(DT, false, false, 2.f)) t2 = clock; }
        CHECK(std::fabs(t1 - 100.f) < 0.1f && std::fabs(t2 - 50.f) < 0.1f, "a Tally halves the gap");
    }
    {   // ---- HOLDFAST ----
        Holdfast h;
        CHECK(h.press() == Holdfast::Press::FIRE, "ready: the button fires");
        h.latch({0, 5, 10}, {0, 1, 0}, 3, -1);
        CHECK(h.held() && std::fabs(h.length - glm::length(glm::vec3{0, 4, 10})) < 1e-3f, "held, on a line as long as the shot");
        glm::vec3 p{0, 1, 0}, v{0, 0, -8};
        for (int i = 0; i < 120; ++i) { p += v * DT; h.constrain(p, v); }
        CHECK(glm::length(p - h.point) <= h.length + 1e-3f, "the line holds you inside its length");
        CHECK(glm::dot(v, glm::normalize(p - h.point)) < 1e-3f, "and takes the speed that was leaving");
        glm::vec3 q{0, 3, 6}, qv{2, 0, 0}; CHECK(!h.constrain(q, qv) && qv.x == 2.f, "inside the line you are free");
        h.carry({5, 0, 0}); glm::vec3 r0 = p; h.constrain(p, v);
        CHECK(p.x > r0.x, "the chunk drifts: the line brings you with it");
        CHECK(h.press() == Holdfast::Press::RELEASED && !h.held(), "press again: let go");
        CHECK(h.press() == Holdfast::Press::NONE, "a short cooldown");
        for (int i = 0; i < 60; ++i) h.tick(DT);
        CHECK(h.press() == Holdfast::Press::FIRE, "then ready again");
        h.latch({0, 1, 0.5f}, {0, 1, 0}, 3, -1);
        CHECK(h.length == Holdfast::MIN_LINE, "a point-blank spike still leaves room to move");
        h.cut(); CHECK(!h.held(), "a cut line lets go");
        Holdfast k; k.press(); k.pin(42);
        CHECK(k.pinning() && k.press() == Holdfast::Press::NONE, "one spike: while it pins, it can't hold");
        bool ended = false; float clock = 0;
        for (int i = 0; i < 60 * 10 && !ended; ++i) { clock += DT; ended = k.tick(DT); }
        CHECK(ended && std::fabs(clock - Holdfast::PIN_TIME) < 0.05f, "the pin lasts its time");
        CHECK(!k.ready(), "then cools down");
    }
    {   // ---- the test ground: shelter is always in reach ----
        LevelData l = buildRingTest();
        CHECK(l.story.built && l.story.chapter == 6, "the test ground builds");
        auto sh = ring::testShelters();
        CHECK(sh.size() >= 6, "the test ground has shelters");
        bool postsBehind = true;
        RingSweep probe; probe.spec.dir = ring::sweepDir();
        for (auto& s : sh) if (!probe.sheltered(s.post, sh)) postsBehind = false;
        CHECK(postsBehind, "every mooring post stands in its own shelter's shadow");
        bool solid = true;
        for (auto& s : sh) {
            bool found = false;
            for (auto& w : l.walls) if (glm::length(w.box.min - s.box.min) < 0.01f && glm::length(w.box.max - s.box.max) < 0.01f) found = true;
            solid &= found;
        }
        CHECK(solid, "every shelter is a wall you can stand behind");
        const AABB g = ring::testGround();
        float worstFar = 0, worstNear = 0; const float walk = 8.f;
        for (float x = g.min.x + 2; x < g.max.x - 2; x += 4.f)
            for (float z = g.min.z + 2; z < g.max.z - 2; z += 4.f) {
                worstFar = std::max(worstFar, sweepreach::secondsToShelter({x, 0, z}, ring::sweepDir(), sh, walk));
                worstNear = std::max(worstNear, sweepreach::secondsToShelter({x, 0, z}, ring::sweepDir(), sh, walk, true));
            }
        std::printf("   worst walk to shelter: %.1f s (any), %.1f s (breakers)\n", worstFar, worstNear);
        CHECK(worstFar < sweeptier::mid(ring::sweepDir()).spec.warning * 0.8f, "from anywhere, shelter is inside the 10 s warning with a fifth to spare");
        CHECK(worstNear < sweeptier::far(ring::sweepDir()).spec.warning * 0.8f, "with the small ones gone, still inside the 20 s warning");
    }
    {   // ---- the chapter: the Landing, the Drift, the Kept ----
        LevelData l = buildRing();
        CHECK(l.story.built && l.story.chapter == 6 && l.arenas.size() == 5, "the chapter builds: five sections over three places");
        auto sh = ring::shelters();
        RingSweep probe; probe.spec.dir = ring::sweepDir();
        bool postsBehind = true;
        for (auto& s : sh) if (!probe.sheltered(s.post, sh)) { postsBehind = false; std::printf("   post not in shadow at %.0f %.0f\n", s.post.x, s.post.z); }
        CHECK(postsBehind, "every mooring post stands in its own shelter's shadow");
        bool solid = true;
        for (auto& s : sh) {
            bool covered = false;   // (a shelter's box sits inside the collision built for it)
            for (auto& w : l.walls)
                if (w.box.min.x <= s.box.min.x + 0.05f && w.box.min.z <= s.box.min.z + 0.05f && w.box.max.x >= s.box.max.x - 0.05f &&
                    w.box.max.z >= s.box.max.z - 0.05f && w.box.max.y >= s.box.max.y - 0.05f) covered = true;
            if (!covered) std::printf("   shelter with no wall at %.0f %.0f\n", s.box.min.x, s.box.min.z);
            solid &= covered;
        }
        CHECK(solid, "every shelter is solid");
        // On the ground (the Landing and the Kept): a fixed shelter's lee inside the far tier's 20 s, walking
        auto worstIn = [&](float zLo, float zHi) {
            float worst = 0.f;
            for (float x = -ring::HALF_X + 4.f; x < ring::HALF_X - 4.f; x += 6.f)
                for (float z = zLo; z < zHi; z += 6.f) {
                    bool inside = false;   // (not inside a shelter or the reef's walls)
                    for (auto& w : l.walls) if (!w.dynamic && x > w.box.min.x && x < w.box.max.x && z > w.box.min.z && z < w.box.max.z && w.box.min.y < 1.f) inside = true;
                    if (!inside) worst = std::max(worst, sweepreach::secondsToShelter({x, 0, z}, ring::sweepDir(), sh, 7.f));
                }
            return worst;
        };
        const float land = worstIn(ring::LAND_N + 4.f, ring::LAND_S - 4.f), kept = worstIn(ring::KEPT_N + 4.f, ring::DRIFT_N - 4.f);
        std::printf("   worst walk to shelter: Landing %.1f s, Kept %.1f s\n", land, kept);
        CHECK(land < 20.f * 0.8f && kept < 20.f * 0.8f, "from anywhere on the ground, fixed shelter is inside the 20 s warning with a fifth to spare");
        // The Drift: each lane a jump from the last (gaps a low-gravity running jump clears), and none too high to climb
        // Everything you stand on crossing the Drift, south to north: the pier, the lanes, the islands, the far shore
        struct Span { float south, north, top; bool grapple; };
        std::vector<Span> spans = {{ring::LAND_N, -170.f, 0.f, false}};
        for (auto& ln : ring::lanes()) spans.push_back({ln.z + (ln.depth - 1.5f) * 0.5f, ln.z - (ln.depth - 1.5f) * 0.5f, ln.top, false});
        for (AABB i : {ring::islandI2(), ring::islandI3()}) spans.push_back({i.max.z, i.min.z, i.max.y, false});
        spans.push_back({ring::DRIFT_N + 2.f, ring::DRIFT_N - 50.f, 0.f, true});   // (the shore: its lip is ragged; the last gap is the grapple's)
        std::sort(spans.begin(), spans.end(), [](const Span& x, const Span& y) { return x.south > y.south; });
        float worstGap = 0.f, worstRise = 0.f;
        for (size_t i = 1; i < spans.size(); ++i) {
            if (spans[i].grapple) continue;
            worstGap = std::max(worstGap, spans[i - 1].north - spans[i].south);
            worstRise = std::max(worstRise, spans[i].top - spans[i - 1].top);
        }
        const float shoreGap = spans[spans.size() - 2].north - spans.back().south;
        std::printf("   the grapple gap to the shore: %.1f m\n", shoreGap);
        CHECK(shoreGap < 30.f, "the last gap is inside the grapple's reach");
        std::printf("   worst lane gap %.1f m, worst climb %.1f m\n", worstGap, worstRise);
        const float g = 24.f * ring::GRAVITY_SCALE, v = 9.1f, air = v / g + std::sqrt(2.f * (v * v / (2.f * g)) / (g * 1.35f));
        CHECK(worstGap < 7.f * air * 0.7f, "every lane gap is inside a running jump, with room to spare");
        CHECK(worstRise < v * v / (2.f * g) * 0.8f, "and no chunk is higher than a jump reaches");
        CHECK(!l.movers.empty() && std::count_if(l.movers.begin(), l.movers.end(), [](const Mover& m) { return m.look == 1; }) > 40, "the lanes are full of drifting chunks");
        int loose = 0; for (auto& s : l.sweepShelters) if (s.mover >= 0) ++loose;
        CHECK(loose >= 4, "there is loose shelter for HERDERS to shove");
        bool spawnsClear = true;
        for (auto& a : l.arenas) for (auto& p : a.groundSpawns)
            for (auto& w : l.walls) if (!w.dynamic && p.x > w.box.min.x && p.x < w.box.max.x && p.z > w.box.min.z && p.z < w.box.max.z && p.y + 0.5f > w.box.min.y && p.y + 0.5f < w.box.max.y) spawnsClear = false;
        CHECK(spawnsClear, "no enemy spawns inside a wall");
        bool tagsOk = l.knockWalls.size() > 50 && l.creakWalls.size() == 6;
        for (int w : l.knockWalls) tagsOk &= w >= 0 && w < (int)l.walls.size() && !l.walls[w].dynamic;
        CHECK(tagsOk, "what you touch: the Landing's capsules and the stacks knock, the reef's six walls creak");
    }
    {   // ---- the menu and the save (step 4): chapter 6 is a chapter ----
        StorySave a; a.chapter = 6; a.checkpoint = 4; a.owned = 15; a.grapple = true;
        StorySave b; const bool ok = b.parse(a.text());
        CHECK(ok && b.chapter == 6 && b.checkpoint == 4, "a chapter 6 save, at the hull, loads");
        StorySave c = a; c.checkpoint = 5; StorySave d;
        CHECK(!d.parse(c.text()), "a chapter 6 save past its last checkpoint does not");
        CHECK((int)buildRing().arenas.size() == 5, "chapter 6 has five checkpoints (0..4), as the save allows");
        CHECK(buildCold().story.nextChapter == 6, "the Cold Store's end card goes on to the ring");
    }
    {   // ---- the Shepherd's count (13.2) ----
        char line[48];
        ring::countLine(line, sizeof(line), ring::DISPATCHED + 1, 0);
        CHECK(std::string(line) == "COUNT: 40,112. ONE EXTRA.", "the count: you are the one extra");
        ring::countLine(line, sizeof(line), ring::DISPATCHED + 1 - 4, 4);
        CHECK(std::string(line) == "COUNT: 40,108. FOUR SHORT.", "the count: four swept or killed, four short");
        ring::countLine(line, sizeof(line), ring::DISPATCHED - 20, 21);
        CHECK(std::string(line) == "COUNT: 40,091. 21 SHORT.", "the count: past ten, in digits");
    }
    {   // ---- HOLDFAST's bar: it fills over whatever cooldown it is in ----
        Holdfast h;
        CHECK(h.cooldown01() == 1.f, "the bar: full when ready");
        h.miss(); h.tick(Holdfast::MISS_COOLDOWN * 0.5f);
        CHECK(std::fabs(h.cooldown01() - 0.5f) < 0.02f, "the bar: half full halfway through a miss's short cooldown");
        h.tick(1.f); h.pin(7); while (!h.tick(DT)) {}
        h.tick(Holdfast::PIN_COOLDOWN * 0.25f);
        CHECK(std::fabs(h.cooldown01() - 0.25f) < 0.02f, "the bar: a quarter full a quarter into a pin's cooldown");
    }
    {   // ---- a breaker shatters a loose shelter, and it is gone ----
        LevelData l = buildRing();
        int si = -1;
        for (int i = 0; i < (int)l.sweepShelters.size(); ++i) if (l.sweepShelters[i].small && l.sweepShelters[i].mover >= 0) { si = i; break; }
        CHECK(si >= 0, "the Kept has loose (small) shelters");
        if (si >= 0) {
            auto sh = l.sweepShelters;
            const glm::vec3 c = (sh[si].box.min + sh[si].box.max) * 0.5f;
            RingSweep s; s.start(sweeptier::near(ring::sweepDir()).spec);
            bool broke = false;
            for (int i = 0; i < 60 * 30 && s.active() && !broke; ++i) {
                auto ev = s.update(DT, c + ring::sweepDir() * 30.f, sh);
                for (int k : ev.shattered) if (k == si) broke = true;
            }
            CHECK(broke, "a breaker shatters it as its front arrives");
            Mover& m = l.movers[sh[si].mover];
            m.gone = true; l.updateMovers(1.f);
            const AABB& b = l.walls[m.wall].box;
            CHECK(b.max.y < -1000.f && m.delta == glm::vec3{0.f}, "a shattered mover leaves: no box where it stood, and nothing riding it is dragged");
            m.gone = false; l.updateMovers(1.f);
            CHECK(std::fabs(l.walls[m.wall].box.min.y - sh[si].box.min.y) < 0.01f, "a retry puts it back");
        }
    }
    std::printf(failures ? "\n%d FAILED\n" : "\nall passed\n", failures);
    return failures ? 1 : 0;
}
