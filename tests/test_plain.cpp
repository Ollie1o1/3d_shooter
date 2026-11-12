// THE BLOOD MOON PLAIN (Chapter 4): the level, walking it with the real
// controller, the meteors, the save slot and the ground texture.
// No OpenGL context required. Build + run with `make test-plain`.
#include "core/Balance.h"
#include "world/LevelPlain.h"
#include "world/PlainMeteors.h"
#include "world/ZoneConstraint.h"
#include "game/StorySave.h"
#include "render/MaterialTex.h"
#include "bosses/gleaner/GleanerState.h"
#include "bosses/gleaner/GleanerRig.h"   // (drawn: its Blender pieces and its surfaces; no OpenGL)
#include "bosses/gleaner/Tender.h"
#include "bosses/gleaner/Sieve.h"
#include "bosses/gleaner/Caller.h"
#include "bosses/gleaner/Organs.h"
#include "game/PlainReveal.h"
#include <algorithm>
#include <cstdio>
static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { ++failures; std::printf("FAIL: %s\n", m); } else std::printf("ok: %s\n", m); } while (0)
constexpr float DT = 1.f / 60;

// The player as the game moves it: keys, collision, the zone. Jumps when it
// stops dead against something it is trying to get past (climbing a heap).
struct Walk {
    LevelData& l; SpatialGrid grid; Player p; int jumps = 0; float stall = 0.f;
    Walk(LevelData& level, glm::vec3 start) : l(level), p(start) { grid.build(l.walls); }
    void step(glm::vec3 goal, bool mayJump) {
        Uint8 keys[SDL_NUM_SCANCODES] = {};
        auto d = goal - p.position; d.y = 0; float n = glm::length(d);
        p.camera.yaw = glm::degrees(std::atan2(d.z, d.x)); keys[SDL_SCANCODE_W] = n > .2f;
        float speed = glm::length(glm::vec2{p.velocity.x, p.velocity.z});
        stall = (n > 1.f && speed < 1.f) ? stall + DT : 0.f;
        if (mayJump && stall > 0.15f && p.onGround) { keys[SDL_SCANCODE_SPACE] = 1; ++jumps; stall = 0.f; }
        p.update(DT, keys, l.walls.data(), (int)l.walls.size(), false, &grid);
        std::vector<const AABB*> zones; for (auto& a : l.arenas) zones.push_back(&a.zone);
        constrainToZones(p.position, p.velocity, p.height, zones, true);
    }
    bool to(glm::vec3 goal, bool mayJump = false, int frames = 6000, float tol = 1.f) {
        for (int f = 0; f < frames; ++f) {
            if (glm::length(glm::vec2{p.position.x - goal.x, p.position.z - goal.z}) < tol) return true;
            step(goal, mayJump);
        }
        std::printf("  stuck %.1f %.1f %.1f -> %.1f %.1f %.1f\n", p.position.x, p.position.y, p.position.z, goal.x, goal.y, goal.z);
        return false;
    }
};

int main() {
    balanceInit();
    auto L = buildPlain();
    const auto feats = plain::features();
    std::printf("      plain: %zu walls, %zu shapes, %zu floor patches, %zu bodies, %zu features\n",
                L.walls.size(), L.shapes.size(), L.floors.size(), L.story.dumped.size(), feats.size());

    // ---- the level ----
    CHECK(L.story.chapter == 4 && L.arenas.size() == 1 && L.story.startWeapons == 15 && L.story.startGrapple,
          "Chapter 4 is one open arena with every gun and the grapple carried in");
    CHECK(L.arenas[0].containsXZ(plain::start()) && L.arenaAt(plain::start()) == 0, "the arrival is inside the plain");
    CHECK(L.walls.size() < 1000 && L.shapes.size() < 5000, "the plain stays within a sensible wall and shape budget (every shot tests the walls)");
    bool open = true;
    for (const auto& w : L.walls)
        if (glm::length(glm::vec2{(w.box.min.x + w.box.max.x) * .5f - plain::start().x, (w.box.min.z + w.box.max.z) * .5f - plain::start().z}) < 40.f) open = false;
    CHECK(open, "nothing solid within 40 m of the arrival: you land in the open");
    bool apart = true, inside = true; int kinds[3] = {0, 0, 0};
    for (size_t i = 0; i < feats.size(); ++i) {
        ++kinds[feats[i].kind];
        inside &= std::abs(feats[i].pos.x) + feats[i].radius < plain::HALF && std::abs(feats[i].pos.z) + feats[i].radius < plain::HALF;
        for (size_t j = i + 1; j < feats.size(); ++j)
            apart &= glm::length(glm::vec2{feats[i].pos.x - feats[j].pos.x, feats[i].pos.z - feats[j].pos.z}) > feats[i].radius + feats[j].radius + 5.f;
    }
    CHECK(inside && apart, "craters, heaps and landmarks sit inside the plain with room to move between them");
    CHECK(kinds[0] >= 25 && kinds[1] >= 15 && kinds[2] == 8, "the plain has its craters, heaps of the rejected and eight landmark meteors");
    CHECK(L.story.dumped.size() > 150, "the heaps are covered in rejected units");

    // ---- walking it ----
    {
        Walk w(L, plain::start());
        for (int f = 0; f < 120; ++f) w.step(w.p.position, false);
        CHECK(w.p.onGround && glm::length(w.p.position - plain::start()) < .3f, "the arrival stands on the hard ground");
    }
    {   // a route round the craters and heaps (planned on a 10 m grid), walked with real movement to every corner
        const int N = 136; const float CELL = 10.f, O = -680.f;
        auto blocked = [&](int i, int j) {
            glm::vec2 p{O + i * CELL, O + j * CELL};
            for (const auto& f : feats) if (glm::length(p - glm::vec2{f.pos.x, f.pos.z}) < f.radius + 9.f) return true;
            return false;
        };
        auto path = [&](glm::vec3 from, glm::vec3 to) {
            std::vector<int> prev(N * N, -1); std::vector<int> q;
            int s = (int)((from.x - O) / CELL + .5f) + N * (int)((from.z - O) / CELL + .5f);
            int g = (int)((to.x - O) / CELL + .5f) + N * (int)((to.z - O) / CELL + .5f);
            q.push_back(s); prev[s] = s;
            for (size_t h = 0; h < q.size() && prev[g] < 0; ++h) {
                int c = q[h], ci = c % N, cj = c / N;
                const int di[4] = {1, -1, 0, 0}, dj[4] = {0, 0, 1, -1};
                for (int k = 0; k < 4; ++k) {
                    int ni = ci + di[k], nj = cj + dj[k];
                    if (ni < 0 || nj < 0 || ni >= N || nj >= N) continue;
                    int nc = ni + N * nj;
                    if (prev[nc] >= 0 || blocked(ni, nj)) continue;
                    prev[nc] = c; q.push_back(nc);
                }
            }
            std::vector<glm::vec3> pts;
            if (prev[g] < 0) return pts;
            for (int c = g; c != s; c = prev[c]) pts.push_back({O + (c % N) * CELL, 0.f, O + (c / N) * CELL});
            std::reverse(pts.begin(), pts.end());
            return pts;
        };
        Walk w(L, plain::start());
        bool far = true; int legs = 0;
        for (glm::vec3 goal : {glm::vec3{-640, 0, -640}, glm::vec3{640, 0, -640}, glm::vec3{640, 0, 640}, glm::vec3{-640, 0, 640}}) {
            auto pts = path(w.p.position, goal);
            far &= !pts.empty();
            for (size_t k = 0; k < pts.size() && far; k += 2) far &= w.to(pts[k], false, 1200, 3.f);
            legs += far;
        }
        std::printf("      corners reached: %d of 4\n", legs);
        CHECK(far, "you can walk the whole plain, corner to corner round its craters and heaps, with real movement");
        Walk in(L, plain::start());
        int craters = 0, entered = 0;
        for (const auto& f : feats) if (f.kind == 0 && craters < 6) {
            ++craters;
            Walk c(L, f.pos + glm::vec3{f.radius + 6.f, 0, 0});
            // Round the rim until a way in opens, then into the middle
            bool got = false;
            for (int k = 0; k < 24 && !got; ++k) {
                float a = k * 0.2618f;
                glm::vec3 edge = f.pos + glm::vec3{std::cos(a), 0, std::sin(a)} * (f.radius + 6.f);
                c.to(edge, false, 600, 1.5f);
                got = c.to(f.pos + glm::vec3{std::cos(a), 0, std::sin(a)} * (f.radius * 0.45f), false, 400, 1.5f);
            }
            entered += got;
        }
        CHECK(entered == craters, "every crater has a way in through its rim without jumping");
    }
    {   // climb the heap nearest the arrival
        auto heap = feats[feats.size() - 2];
        Walk w(L, plain::start());
        bool up = w.to(heap.pos + glm::vec3{0, 0, heap.radius + 3.f}) && w.to(heap.pos, true, 1200, 0.8f);
        std::printf("      heap climb: top %.2f m after %d jumps\n", w.p.position.y, w.jumps);
        CHECK(up && w.p.position.y > heap.radius * 0.42f * 0.8f, "a heap of the rejected can be climbed with jumps and stood on");
    }
    {   // the edge: walking out keeps you in
        Walk w(L, {plain::HALF - 30.f, 0, 0});
        for (int f = 0; f < 900; ++f) w.step({plain::HALF + 200.f, 0, 0}, false);
        CHECK(w.p.position.x <= plain::HALF + 0.5f && w.p.onGround, "the plain's edge holds you on the ground inside it");
    }

    // ---- the meteors ----
    {
        MeteorRain rain; rain.reset(7u);
        std::vector<MeteorImpact> hits;
        int impacts = 0, near = 0, big = 0, peak = 0; float closest = 1e9f;
        glm::vec3 player = plain::start();
        for (int f = 0; f < 60 * 60 * 20; ++f) {   // twenty minutes, walking a slow circle
            float t = f * DT;
            player = {std::cos(t * 0.02f) * 400.f, 0.f, std::sin(t * 0.02f) * 400.f};
            rain.tick(DT, player, hits);
            peak = std::max(peak, rain.live());
            for (const auto& h : hits) {
                ++impacts;
                float d = glm::length(h.pos - player);
                closest = std::min(closest, d);
                near += d < 350.f; big += h.size > 8.f;
            }
        }
        std::printf("      meteors in 20 min: %d impacts, %d within 350 m, %d big, closest %.0f m, %d in the air at most\n", impacts, near, big, closest, peak);
        CHECK(impacts > 500 && impacts < 1100, "a meteor comes down every second or two");
        CHECK(closest > MeteorRain::MIN_NEAR - 25.f, "an ambient meteor never lands on the player (walking included)");
        CHECK(near > 30 && big > 30, "now and then one lands near enough to feel, and some are big");
        CHECK(peak < MeteorRain::POOL, "the sky never runs out of meteors to throw");
        MeteorRain a, b; a.reset(3u); b.reset(3u); std::vector<MeteorImpact> ha, hb; bool same = true;
        for (int f = 0; f < 3000; ++f) { a.tick(DT, player, ha); b.tick(DT, player, hb); same &= ha.size() == hb.size(); }
        CHECK(same, "the same seed rains the same way (repeatable runs and tests)");
        MeteorRain one; one.reset(); one.next = 1e9f;
        int i = one.launch({10, 0, -20}, 5.f); std::vector<MeteorImpact> h; bool landed = false;
        for (int f = 0; f < 60 * 20 && !landed; ++f) { one.tick(DT, player, h); if (!h.empty()) landed = glm::length(h[0].pos - glm::vec3{10, 0, -20}) < 0.5f; }
        CHECK(i >= 0 && landed && one.remains[0].alive, "a meteor lands exactly where it was aimed and leaves burning remains");
    }

    // ---- THE GLEANER ----
    {
        const float K = Gleaner::SCALE;
        Gleaner g; g.place({0, 0, -400}, 0.f, 1.f);
        float shoulderY = g.shoulder(0).y;
        Gleaner lying; lying.place({0, 0, -400}, 3.14159f, 0.f);
        std::printf("      gleaner: shoulders %.0f m up standing, %.0f m lying; eye %.0f m up\n", shoulderY, lying.shoulder(0).y, g.eye().y);
        CHECK(shoulderY > 250.f && lying.shoulder(0).y < shoulderY * 0.3f, "it stands hundreds of metres tall and lies sunk in the plain");
        CHECK(std::abs(glm::length(g.elbow(0) - g.shoulder(0)) - Gleaner::UPPER * K) < 1.f &&
              std::abs(glm::length(g.hand(0) - g.elbow(0)) - Gleaner::FORE * K) < 1.f, "its arms keep their length (two-bone IK)");
        {   // Its pieces made in Blender (tools/blender/gleaner_build.py): every one loads, and draws with surfaces
            const char* P[] = {"gleaner_fist", "gleaner_forearm", "gleaner_upperarm", "gleaner_pauldron", "gleaner_skull", "gleaner_jaw", "gleaner_scale", "gleaner_charm"};
            bool all = true; int surfaced = 0, parts = 0;
            for (const char* n : P) {
                std::vector<BoxInstance> out;
                all &= propdraw::push(out, n, glm::mat4(1.f), glm::vec3{2.f, 0.6f, 0.15f}, 1.f, true);
                for (const auto& b : out) { ++parts; surfaced += b.surface > 0.f; }
            }
            std::printf("      gleaner pieces: %d parts, %d with a surface\n", parts, surfaced);
            CHECK(all && parts > 600 && surfaced > parts / 2, "the Gleaner's Blender pieces all load, most parts with a surface");
            Gleaner g; g.place({0, 0, -400}, 0.f, 1.f);
            std::vector<BoxInstance> out; gleaner::Look look; gleaner::draw(out, g, look);
            int plain = 0; for (const auto& b : out) plain += b.surface == 0.f && glm::length(b.emissive) < 0.01f;
            std::printf("      gleaner drawn: %zu instances, %d without a surface\n", out.size(), plain);
            CHECK(plain < (int)out.size() / 10, "drawn whole, nearly every part of it has a surface (the code boxes from their colours)");
        }
        {   // A scream called between ticks (by the fight, by the reveal) reaches the tick's events: the roar's sound
            // is played from them. (tick() used to clear them first: the Gleaner never made a sound when it screamed.)
            Gleaner s; s.place({0, 0, -400}, 0.f, 1.f);
            s.tick(DT); s.scream(); s.tick(DT);
            int screams = 0; for (const auto& e : s.events) screams += e.kind == GleanerEvent::SCREAM;
            s.tick(DT);
            int after = 0; for (const auto& e : s.events) after += e.kind == GleanerEvent::SCREAM;
            CHECK(screams == 1 && after == 0, "a scream called between ticks is in the next tick's events once (its roar plays), then gone");
        }
        // Crawl two minutes
        g.wantSpeed = 3.4f; g.speed = 3.4f;
        int slams[2] = {0, 0}, lastSide = -1; bool alternate = true, reach = true;
        glm::vec3 start = g.pos;
        for (int f = 0; f < 60 * 120; ++f) {
            g.tick(DT);
            for (const auto& e : g.events) if (e.kind == GleanerEvent::SLAM) {
                int sideHit = glm::dot(e.at - g.pos, g.right()) < 0.f ? 0 : 1;
                ++slams[sideHit]; alternate &= sideHit != lastSide; lastSide = sideHit;
            }
            for (int a = 0; a < 2; ++a) reach &= glm::length(g.hand(a) - g.shoulder(a)) < (Gleaner::UPPER + Gleaner::FORE) * K * 1.02f;
        }
        float went = glm::length(g.pos - start);
        std::printf("      gleaner crawl: %.0f m in 120 s, %d + %d fists landed\n", went, slams[0], slams[1]);
        CHECK(went > 300.f && went < 450.f, "it crawls slowly: a few metres a second");
        CHECK(slams[0] >= 12 && std::abs(slams[0] - slams[1]) <= 1 && alternate, "its fists land in turn, left, right, left");
        CHECK(reach, "its fists never leave its reach");
        // Turning round on the spot, fist over fist (the reveal's turn)
        Gleaner t; t.place({0, 0, -400}, 3.14159f, 1.f); t.gaitPeriod = 3.4f;
        int steps = 0;
        for (int f = 0; f < 60 * 8; ++f) {
            float u = std::min(1.f, f / (60.f * 3.6f));
            t.yaw = t.targetYaw = 3.14159f * (1.f - u * u * (3 - 2 * u));
            t.tick(DT);
            for (const auto& e : t.events) steps += e.kind == GleanerEvent::SLAM;
        }
        bool home = glm::length(t.arm[0].plant - t.home(0, 0.f)) < 25.f * K && glm::length(t.arm[1].plant - t.home(1, 0.f)) < 25.f * K;
        std::printf("      gleaner turn: %d fists landed\n", steps);
        CHECK(steps >= 2 && home, "turning round, it steps its fists round with it and ends planted");
        // The scream
        Gleaner sc; sc.place({0, 0, 0}, 0.f, 1.f); sc.scream();
        int screams = (int)sc.events.size(); float jawAt = 0.f;
        for (int f = 0; f < 36; ++f) sc.tick(DT);
        jawAt = sc.jaw;
        for (int f = 0; f < 60 * 6; ++f) sc.tick(DT);
        CHECK(screams == 1 && jawAt > 0.9f && !sc.screaming() && sc.jaw == 0.f, "a scream opens its jaw wide, holds, and closes");
    }

    // ---- THE TENDER ----
    {
        Tender k; k.wake({0, 100, 0}, 0.f, {-60, -60}, {60, 60}, 100.f);
        glm::vec3 you{0, 100, 30};
        for (int f = 0; f < 60 * 3; ++f) { k.tick(DT, you); k.events.clear(); }
        CHECK(k.fighting() && k.state != Tender::RISING, "the Tender climbs out of the hatch and comes for you");
        // Close: it sweeps; the sweep catches you on the deck and misses you in the air
        Tender s = k; s.pos = {0, 100, 0}; s.yaw = 0.f; s.state = Tender::SWEEP_WIND; s.t = 0.f;
        bool swept = false;
        for (int f = 0; f < 120 && !swept; ++f) { s.tick(DT, {0, 100, 6}); for (auto& e : s.events) swept |= e.kind == TenderEvent::SWEEP; s.events.clear(); }
        CHECK(swept && s.inSweep({0, 100, 6}) && !s.inSweep({0, 102.2f, 6}) && !s.inSweep({0, 100, -6}),
              "its sweep follows a visible wind-up, catches you in front of it on the deck, and a jump clears it");
        // The pour: a river ahead of it, its lip open to you; enough on the lip spills it
        Tender p = k; p.pos = {0, 100, 0}; p.yaw = 0.f; p.state = Tender::POUR_TIP; p.t = 0.f; p.lipTaken = 0.f;
        for (int f = 0; f < 60 * 2; ++f) { p.tick(DT, {0, 100, 30}); p.events.clear(); }
        CHECK(p.state == Tender::POUR && p.lipOpen() && p.riverLen > 10.f && p.inRiver(p.riverFrom + p.riverDir * 8.f) && !p.inRiver(p.riverFrom + glm::vec3{9, 0, 8}),
              "it pours a river of metal out ahead of it, its crucible's lip open to you");
        float t = 0.f;
        int zone = p.ray({0, 101.7f, 30}, glm::normalize(p.lipPoint() - glm::vec3{0, 101.7f, 30}), 100.f, t);
        CHECK(zone == 2, "a round at the open lip hits the lip (its weak point)");
        float before = p.hp;
        for (int i = 0; i < 4; ++i) p.hurt(42.f * Tender::zoneMult(2), 2);
        CHECK(p.staggered() && before - p.hp > 42.f * 3.f * 3.9f, "enough damage on the lip spills the pour and staggers it; staggered it takes double");
        // It dies, once
        Tender d = k; int dies = 0;
        for (int i = 0; i < 200 && d.fighting(); ++i) d.hurt(42.f, 0);
        for (auto& e : d.events) dies += e.kind == TenderEvent::DIE;
        CHECK(!d.fighting() && d.state == Tender::DEAD && dies == 1, "shot enough, it dies, once");
        Tender e2 = k; e2.hp = Tender::MAX_HP * 0.4f;
        CHECK(e2.enraged() && e2.windTime() < k.windTime(), "below half health it is enraged: a quicker wind-up");
    }
    // ---- THE SIEVE ----
    {
        const float Y = 100.f;
        Sieve v; v.wake({0, Y, 0}, 0.f, {-60, -60}, {60, 60}, Y);
        glm::vec3 you{0, Y, 30};
        bool rose = false; float lowest = 1e9f;
        for (int f = 0; f < 60 * 4; ++f) { v.tick(DT, you); rose |= v.fighting(); v.events.clear(); for (int i = 0; i < 4; ++i) lowest = std::min(lowest, v.knee(i).y - v.hub().y); }
        CHECK(rose && v.hub().y > Y + 15.f && lowest > 0.f, "the Sieve unfolds out of its crater: its hub high, its knees higher");
        // The mesh: a level shot from the side is turned aside; one from right under it reaches the heart; a splinter threads it
        float t = 0.f;
        glm::vec3 h = v.heart(), side = h + glm::vec3{0, 0, 20};
        int z1 = v.ray(side, glm::normalize(h - side), 100.f, t);
        glm::vec3 under = glm::vec3{h.x + 2.f, Y + 1.7f, h.z + 1.f};
        int z2 = v.ray(under, glm::normalize(h - under), 100.f, t);
        int z3 = v.ray(side, glm::normalize(h - side), 100.f, t, true);
        CHECK(z1 == 0 && z2 == 2 && z3 == 2, "its mesh turns a level shot aside; from under it, or off a splinter, the round reaches its heart");
        CHECK(Sieve::zoneMult(2) >= 3.f && Sieve::zoneMult(0) < 0.5f, "its heart is a x3 weak point; the mesh takes little");
        // Walking: it keeps a stride off you and its feet step (never all at once)
        Sieve w = v; w.state = Sieve::STALK; w.cooldown = 99.f; w.pos = {0, Y, -10};
        int steps = 0; bool allUp = false;
        for (int f = 0; f < 60 * 6; ++f) {
            w.tick(DT, you);
            for (auto& e : w.events) steps += e.kind == SieveEvent::STEP;
            w.events.clear();
            allUp |= w.footU[0] >= 0.f && w.footU[1] >= 0.f && w.footU[2] >= 0.f && w.footU[3] >= 0.f;
        }
        float gap = glm::length(glm::vec2{w.pos.x - you.x, w.pos.z - you.z});
        CHECK(steps >= 4 && !allUp && gap > 9.f && gap < 18.f, "it stalks you a stride off on its stilts, its feet stepping in pairs");
        // The rain: a circle on you, then slag through the mesh into it; out of it is safe
        Sieve r = v; r.pos = {0, Y, 20}; r.state = Sieve::SHAKE_AIM; r.t = 0.f;
        bool rained = false;
        for (int f = 0; f < 60 * 2 && !rained; ++f) { r.tick(DT, you); for (auto& e : r.events) rained |= e.kind == SieveEvent::RAIN_START; r.events.clear(); }
        CHECK(rained && r.inRain(you) && !r.inRain(you + glm::vec3{Sieve::RAIN_R + 1.f, 0, 0}), "it shakes its basket over you: slag rains into a marked circle, not outside it");
        // The stake: a leg rears, comes down near you, the plate cracks in lines (one runs at you) that burst after a tell
        Sieve k = v; k.pos = {0, Y, 10}; k.state = Sieve::STALK; k.cooldown = 0.f; k.seed = 3u;
        for (int tries = 0; tries < 40 && k.state != Sieve::STAKE_RAISE; ++tries) { Sieve c = v; c.pos = {0, Y, 10}; c.state = Sieve::STALK; c.cooldown = 0.f; c.seed = 3u + tries; c.tick(DT, {0, Y, 50}); if (c.state == Sieve::STAKE_RAISE) k = c; }
        bool staked = false, cracked = false, tellFirst = true;
        glm::vec3 you2{0, Y, 50};
        for (int f = 0; f < 60 * 3; ++f) {
            k.tick(DT, you2);
            for (auto& e : k.events) { staked |= e.kind == SieveEvent::STAKE; if (e.kind == SieveEvent::CRACKS) { cracked = true; tellFirst &= k.crackT >= Sieve::CRACK_TELL; } }
            k.events.clear();
            if (cracked) break;
        }
        glm::vec3 onLine = k.crackFrom + k.crackDir[0] * 6.f, offLine = k.crackFrom + (k.crackDir[0] + k.crackDir[1]) * 0.5f * 6.f;
        onLine.y = offLine.y = Y;
        CHECK(staked && cracked && tellFirst && k.onCrack(onLine) && !k.onCrack(offLine) && !k.onCrack(onLine + glm::vec3{0, 2.2f, 0}),
              "its stake cracks the plate in lines: on one you are hit, between them or over them you are not");
        CHECK(k.state == Sieve::STAKED && k.heartOpen(), "staked, its basket swings its mouth at you: the heart is open");
        float before = k.hp;
        for (int i = 0; i < 4; ++i) k.hurt(40.f * Sieve::zoneMult(2), 2);
        CHECK(k.state == Sieve::STAGGER && before - k.hp > 40.f * 3.f * 3.9f, "enough on its heart spills the basket: it staggers, and takes double");
        Sieve d = v; int dies = 0;
        for (int i = 0; i < 400 && d.fighting(); ++i) d.hurt(42.f, 1);
        for (auto& e : d.events) dies += e.kind == SieveEvent::DIE;
        CHECK(!d.fighting() && dies == 1, "shot enough, the Sieve dies, once");
    }

    // ---- THE CALLER ----
    {
        const float Y = 100.f;
        Caller c; c.wake({0, Y, 0}, Y, 0.6f);
        glm::vec3 you{0, Y, 0}, eye = you + glm::vec3{0, 1.7f, 0};
        bool arrived = false, tolled = false;
        for (int f = 0; f < 60 * 6; ++f) { c.tick(DT, you, eye); for (auto& e : c.events) tolled |= e.kind == CallerEvent::TOLL; c.events.clear(); arrived |= c.fighting(); }
        CHECK(arrived && tolled && c.pos.y > Y + Caller::ALT - 6.f, "the Caller comes down out of the sky tolling and circles high over its ground");
        // Its lamp: up into its mouth you see it; from level with it, or from above, its shell turns the round
        Caller u = c; u.state = Caller::CIRCLE; u.axis = {0, 1, 0}; u.pos = {0, Y + 24.f, 0};
        float t = 0.f;
        glm::vec3 below{3, Y + 1.7f, 2}, level = u.pos + glm::vec3{25, 0, 0}, above = u.pos + glm::vec3{5, 20, 0};
        int zb = u.ray(below, glm::normalize(u.lamp() - below), 200.f, t), zl = u.ray(level, glm::normalize(u.lamp() - level), 200.f, t), za = u.ray(above, glm::normalize(u.lamp() - above), 200.f, t);
        int zs = u.ray(level, glm::normalize(u.lamp() - level), 200.f, t, true);
        CHECK(zb == 2 && zl == 0 && za == 0 && zs == 2, "its lamp shows only from below (or to a splinter); its shell takes the rest");
        // The toll calls the sky down on you
        Caller tl = u; tl.state = Caller::TOLL; tl.t = 0.f;
        int calls = 0; glm::vec3 callAt{0.f};
        for (int f = 0; f < 60 * 2; ++f) { tl.tick(DT, you, eye); for (auto& e : tl.events) if (e.kind == CallerEvent::CALL) { ++calls; callAt = e.at; } tl.events.clear(); }
        CHECK(calls == 1 && glm::length(callAt - you) < 0.5f, "a toll calls one meteor down on you");
        // The dive: a tell (its mouth tipped at you, the lamp open from the front), then a straight line through where you stood
        Caller dv = u; dv.state = Caller::DIVE_AIM; dv.t = 0.f;
        bool openAtTell = false, dived = false; float closest = 1e9f;
        for (int f = 0; f < 60 * 3; ++f) {
            dv.tick(DT, you, eye);
            if (dv.state == Caller::DIVE_AIM && dv.t > dv.aimTime() * 0.8f) openAtTell |= dv.lampSeen(glm::normalize(dv.pos - eye), false);
            if (dv.state == Caller::DIVE) { dived = true; closest = std::min(closest, glm::length(dv.pos - eye)); }
            dv.events.clear();
        }
        CHECK(openAtTell && dived && closest < Caller::HIT_R, "it tips its mouth at you (the lamp open), then dives straight through where you stood");
        // Parried as it arrives: it goes down on its side, the lamp open from anywhere, double damage, then it gets up
        Caller pr = u; pr.state = Caller::DIVE_AIM; pr.t = 0.f;
        while (!(pr.state == Caller::DIVE && glm::length(pr.pos - eye) < Caller::PARRY_REACH)) { pr.tick(DT, you, eye); pr.events.clear(); }
        CHECK(pr.parryable(), "its dive can be met with a punch as it arrives");
        pr.parried(pr.pos - eye);
        for (int f = 0; f < 60 * 2; ++f) { pr.tick(DT, you, eye); pr.events.clear(); }
        int zSide = pr.ray(eye, glm::normalize(pr.lamp() - eye), 200.f, t);
        float hp0 = pr.hp; pr.hurt(100.f, 2);
        CHECK(pr.stunned() && pr.onGroundNow() && zSide == 2 && hp0 - pr.hp > 199.f, "punched, it is thrown down on its side: its lamp open from anywhere, double damage");
        for (int f = 0; f < 60 * 4; ++f) { pr.tick(DT, you, eye); pr.events.clear(); }
        CHECK(!pr.stunned() && pr.fighting(), "and it gets back up into the air");
        Caller d = u; int dies = 0;
        for (int i = 0; i < 400 && d.fighting(); ++i) d.hurt(42.f, 2);
        for (auto& e : d.events) dies += e.kind == CallerEvent::DIE;
        for (int f = 0; f < 60 * 4; ++f) { d.tick(DT, you, eye); d.events.clear(); }
        CHECK(!d.fighting() && dies == 1 && d.onGroundNow(), "shot enough, the Caller dies, once, and falls onto its ground");
    }
    // ---- inside it: the organs ----
    {
        CrucibleHeart h; h.wake({0, 13, 0}, 0.f);
        bool swelled = false, flooded = false, valvesShutBefore = true, spat = false;
        float t = 0.f;
        for (int f = 0; f < 60 * 12 && !flooded; ++f) {
            if (h.state != CrucibleHeart::FLOOD) valvesShutBefore &= !h.valvesOpen();
            h.tick(DT); t += DT;
            for (auto& e : h.events) { swelled |= e.kind == organ::Event::SWELL; flooded |= e.kind == organ::Event::FLOOD; spat |= e.kind == organ::Event::SPIT; }
            h.events.clear();
        }
        for (int f = 0; f < 60; ++f) { h.tick(DT); h.events.clear(); }
        CHECK(swelled && flooded && spat && valvesShutBefore && h.valvesOpen(), "the heart beats, spits, swells (the tell), then floods with its valves open");
        CHECK(h.inSlag({0, 0, 5}) && !h.inSlag({0, 1.8f, 5}), "its flood burns on the floor and not on the islands");
        float tt = 0.f; glm::vec3 eye{0, 1.7f, 20};
        int nv = 0; for (int k = 1; k < 3; ++k) if (glm::length(h.valve(k) - eye) < glm::length(h.valve(nv) - eye)) nv = k;
        int zv = h.ray(eye, glm::normalize(h.valve(nv) - eye), 100.f, tt);
        int zb = h.ray(h.pos + glm::vec3{0, 20, 0}, {0, -1, 0}, 100.f, tt);   // (down onto its top, clear of the valves round its waist)
        CHECK(zv == 2 && zb == 0 && CrucibleHeart::zoneMult(2) > 2.9f, "its open valves are its weak point; its body is armoured");
        CrucibleHeart d = h; int dies = 0; for (int i = 0; i < 300 && d.alive(); ++i) d.hurt(40.f); for (auto& e : d.events) dies += e.kind == organ::Event::DIE;
        CHECK(!d.alive() && dies == 1, "the heart dies, once");

        BellowsLungs L; L.wake({-16, 6, 22}, {16, 6, 22}, {0, 0, -1}, 0.f);
        bool gust = false, opened = false;
        for (int f = 0; f < 60 * 8; ++f) { L.tick(DT); opened |= L.throatOpen(0); for (int k = 0; k < 2; ++k) gust |= glm::length(L.gustAt(k, {L.b[k].pos.x, 0, 5})) > 5.f; L.events.clear(); }
        CHECK(opened && gust, "the lungs breathe in turn: their throats open, then a gust down the room");
        BellowsLungs g = L; g.b[0].ph = BellowsLungs::OUT; g.b[0].t = 0.1f;
        CHECK(glm::length(g.gustAt(0, {-16, 0, 0})) > 10.f && glm::length(g.gustAt(0, {-16, 0, 30})) < 0.01f, "a gust pushes you in front of the bellows, not behind it");
        BellowsLungs k2 = L; for (int i = 0; i < 100 && k2.alive(); ++i) { k2.hurt(0, 60.f); k2.hurt(1, 60.f); }
        CHECK(!k2.alive(), "both lungs torn, they stop");

        SortingCore c; c.wake({0, 15, 0});
        int thrown = 0; bool open = false;
        for (int f = 0; f < 60 * 8 && !open; ++f) { c.tick(DT); for (auto& e : c.events) { thrown += e.kind == organ::Event::THROW; open |= e.kind == organ::Event::OPEN; } c.events.clear(); }
        glm::vec3 below{4, 0, 4};
        int zl = c.ray(below + glm::vec3{0, 1.7f, 0}, glm::normalize(c.lens() - below - glm::vec3{0, 1.7f, 0}), 100.f, tt);
        CHECK(thrown == 3 && open && c.lensOpen() && zl == 2, "the core throws three bodies, then opens its lens (its weak point)");
        SortingCore cd = c; for (int i = 0; i < 400 && cd.alive(); ++i) cd.hurt(40.f);
        CHECK(!cd.alive(), "the core dies");
    }
    // ---- called meteors ----
    {
        MeteorRain rain; rain.reset(); rain.next = 1e9f;
        glm::vec3 you{50, 0, 50};
        int i = rain.launchCalled(you, 6.f);
        std::vector<MeteorImpact> h; float t = 0.f; bool landed = false, called = false; float closeIn = 0.f;
        while (!landed && t < 5.f) {
            closeIn = std::max(closeIn, MeteorRain::calledIn(rain.pool[i]));
            rain.tick(DT, you, h); t += DT;
            for (auto& m : h) { landed = true; called = m.called && glm::length(m.pos - you) < 0.5f; }
        }
        CHECK(landed && called && std::abs(t - MeteorRain::CALLED_FALL) < 0.1f && closeIn > 0.9f,
              "a called meteor lands where it was aimed (at you), on time, its marker closing in");
        MeteorRain up; up.reset(); up.next = 1e9f;
        glm::vec3 back{50, 326, 50};
        up.launchCalled(back, 6.f, back.y);
        landed = false; t = 0.f; glm::vec3 at{0.f};
        while (!landed && t < 5.f) { up.tick(DT, back, h); t += DT; for (auto& m : h) { landed = true; at = m.pos; } }
        CHECK(landed && glm::length(at - back) < 0.5f && std::abs(t - MeteorRain::CALLED_FALL) < 0.1f, "one called onto the Gleaner's back lands on its back, on time");
    }

    // ---- the reveal ----
    {
        PlainReveal r; r.start();
        bool order = PlainReveal::RISE0 < PlainReveal::BLINK1 && PlainReveal::BLINK1 <= PlainReveal::TURN0 + 0.2f && PlainReveal::TURN1 <= PlainReveal::SCREAM + 0.5f;
        CHECK(order, "the reveal's beats come in order: it rises, the moon shuts, it turns, it screams");
        bool lidBeforeEye = true, played = false; float t = 0.f;
        while (r.active() && t < 60.f) {
            if (r.eyeOpen() > 0.01f) lidBeforeEye &= r.lid() > 0.99f;
            played = r.update(DT, false); t += DT;
        }
        CHECK(played && std::abs(t - PlainReveal::DURATION) < 0.05f, "the reveal plays through and hands control back");
        CHECK(lidBeforeEye, "the moon has shut before the eye in its chest opens");
        PlainReveal held; held.start(); int f = 0;
        while (held.active() && f < 600) { held.update(DT, true); ++f; }
        CHECK(f == 600 && held.active(), "a Jump still held from the menu cannot skip it");
        held.update(DT, false); f = 0;
        while (held.active() && f < 600) { held.update(DT, true); ++f; }
        CHECK(!held.active() && f < 60, "release and hold Jump: it skips");
        PlainReveal c; c.time = PlainReveal::DURATION - 0.01f;
        CHECK(std::abs(c.yaw() + 90.f) < 1.f && c.white() == 0.f, "it ends looking straight ahead, the white long gone");
    }

    // ---- the save slot ----
    {
        StorySave s; s.chapter = 4; s.checkpoint = 2; s.owned = 15; s.grapple = true;
        StorySave back; CHECK(back.parse(s.text()) && back.chapter == 4 && back.checkpoint == 2, "a Chapter 4 checkpoint saves and loads");
        s.checkpoint = 8; CHECK(back.parse(s.text()) && back.checkpoint == 8, "the last checkpoint inside it (the core dead) saves");
        s.checkpoint = 9; CHECK(!back.parse(s.text()), "a Chapter 4 checkpoint past its last phase is rejected");
        s.chapter = 7; s.checkpoint = 0; CHECK(!back.parse(s.text()), "a chapter that does not exist is rejected");
    }

    // ---- the ground ----
    {
        auto px = MaterialTex::cinder();
        int glow = 0; double seam = 0.0;
        const int S = MaterialTex::SIZE;
        for (int i = 0; i < S * S; ++i) glow += px[i * 4 + 3] < 250;
        for (int y = 0; y < S; ++y) for (int c = 0; c < 3; ++c) seam += std::abs((int)px[(y * S) * 4 + c] - (int)px[(y * S + S - 1) * 4 + c]);
        float glowShare = (float)glow / (S * S);
        std::printf("      cinder: %.1f%% of texels glow, mean edge difference %.1f\n", glowShare * 100.f, seam / (S * 3));
        CHECK(glowShare > 0.002f && glowShare < 0.04f, "the cinder ground glows in a few cracks, not all of them");
        CHECK(seam / (S * 3) < 40.0, "the cinder texture wraps without a hard seam at its edge");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
