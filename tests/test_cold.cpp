// THE COLD STORE (Chapter 5): the planet. The shelf, its ragged lip and the
// Void past it, walking it with the real controller, falling off it, and the
// snow and ice textures. No OpenGL context required. `make test-cold`.
#include "core/Balance.h"
#include "world/LevelCold.h"
#include "world/ZoneConstraint.h"
#include "render/MaterialTex.h"
#include "enemies/Enemy.h"
#include <algorithm>
#include <cstdio>
static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { ++failures; std::printf("FAIL: %s\n", m); } else std::printf("ok: %s\n", m); } while (0)
constexpr float DT = 1.f / 60;

// The player as the game moves it: keys, collision, the zone, and the hard floor
// under it (the Void's is far down)
struct Walk {
    LevelData& l; SpatialGrid grid; Player p; float stall = 0.f;
    Walk(LevelData& level, glm::vec3 start) : l(level), p(start) { grid.build(l.walls); }
    void step(glm::vec3 goal, bool mayJump) {
        Uint8 keys[SDL_NUM_SCANCODES] = {};
        auto d = goal - p.position; d.y = 0; float n = glm::length(d);
        p.camera.yaw = glm::degrees(std::atan2(d.z, d.x)); keys[SDL_SCANCODE_W] = n > .2f;
        float speed = glm::length(glm::vec2{p.velocity.x, p.velocity.z});
        stall = (n > 1.f && speed < 1.f) ? stall + DT : 0.f;
        if (mayJump && stall > 0.15f && p.onGround) { keys[SDL_SCANCODE_SPACE] = 1; stall = 0.f; }
        p.floorY = l.baseFloor(p.position.x, p.position.z);
        p.update(DT, keys, l.walls.data(), (int)l.walls.size(), false, &grid);
        std::vector<const AABB*> zones; for (auto& a : l.arenas) zones.push_back(&a.zone);
        constrainToZones(p.position, p.velocity, p.height, zones, true);
    }
    bool to(glm::vec3 goal, bool mayJump = false, int frames = 9000, float tol = 1.f) {
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
    auto L = buildCold();
    const Arena& A = L.arenas[0];

    // ---- the level ----
    CHECK(L.story.built && L.story.chapter == 5 && std::string(L.story.title) == "THE COLD STORE", "Chapter 5 builds as THE COLD STORE");
    CHECK(L.arenas.size() == 1 && A.hasRespawn && A.voidY > cold::VOID_FLOOR + 100.f, "one open arena; falling into the Void respawns you long before its floor");
    const glm::vec3 s = cold::start();
    CHECK(L.baseFloor(s.x, s.z) == 0.f, "the arrival stands on the shelf");
    bool clearStart = true;
    for (const auto& w : L.walls)
        if (std::abs(w.box.min.x + w.box.max.x - 2.f * s.x) < (w.box.max.x - w.box.min.x) + 30.f
            && std::abs(w.box.min.z + w.box.max.z - 2.f * s.z) < (w.box.max.z - w.box.min.z) + 30.f && w.box.max.y > 0.1f) clearStart = false;
    CHECK(clearStart, "nothing solid within 15 m of the arrival");

    // ---- the lip ----
    bool lip = true, zoneOver = true, ragged = false;
    float lo = 1e9f, hi = -1e9f;
    for (float x = -cold::HALF_X + 1.5f; x < cold::HALF_X; x += 7.f) {   // (off the strips' boundary lines, where either side may answer)
        float ez = cold::edgeZ(x);
        lo = std::min(lo, ez); hi = std::max(hi, ez);
        lip &= L.baseFloor(x, ez + 0.5f) == 0.f && L.baseFloor(x, ez - 0.5f) == cold::VOID_FLOOR;
        zoneOver &= A.zone.min.z < ez - 10.f;
    }
    ragged = hi - lo > 15.f;
    std::printf("      the lip runs from z %.0f to %.0f across the shelf\n", lo, hi);
    CHECK(lip, "the shelf ends at its lip: snow on one side, the Void's long drop on the other");
    CHECK(zoneOver, "the play zone runs past the lip everywhere, so you can walk off it");
    CHECK(ragged, "the lip is ragged, not a ruler line");
    bool floorsOnShelf = true;
    for (const auto& f : L.floors) floorsOnShelf &= f.z0 >= cold::edgeZ(f.x0) - 0.01f;
    CHECK(floorsOnShelf, "no snow is drawn out over the Void");

    // ---- walking it ----
    {
        Walk w(L, s);
        glm::vec3 nearLip{0.f, 0.f, cold::edgeZ(0.f) + 6.f};
        CHECK(w.to(nearLip, true), "you can walk from the arrival to the lip (the marker line is clear)");
        CHECK(w.p.position.y > -0.1f && w.p.onGround, "...and stand there on the snow");
        // Walk on, off the edge: you fall, past the respawn line
        glm::vec3 beyond{0.f, 0.f, cold::edgeZ(0.f) - 40.f};
        float lowest = 0.f;
        for (int f = 0; f < 60 * 8; ++f) { w.step(beyond, false); lowest = std::min(lowest, w.p.position.y); }
        CHECK(lowest < A.voidY, "walk off the lip and you fall into the Void");
    }
    {   // Along the shelf, east to west, near the lip: ridges and blocks are in the way at most for a jump
        Walk w(L, {cold::HALF_X - 20.f, 0.f, cold::edgeZ(cold::HALF_X - 20.f) + 30.f});
        bool ok = true;
        for (float x = cold::HALF_X - 60.f; x > -cold::HALF_X + 20.f && ok; x -= 40.f)
            ok = w.to({x, 0.f, std::max(cold::edgeZ(x), cold::edgeZ(x + 40.f)) + 30.f}, true);
        CHECK(ok && w.p.position.y > -1.f, "you can walk the whole length of the shelf along the lip without falling in");
    }

    // ---- what you see ----
    CHECK(cold::giants().size() >= 4, "frozen giants stand round the horizon");
    bool farEnough = true;
    for (const auto& g : cold::giants()) farEnough &= glm::length(glm::vec2{g.pos.x, g.pos.z}) > 1500.f && glm::length(glm::vec2{g.pos.x, g.pos.z}) + g.height < cold::FAR_PLANE;
    CHECK(farEnough, "...far off the shelf, and inside what the camera can see");
    bool towersOver = true;
    for (const auto& t : cold::towers()) towersOver &= t.base.z + t.half < cold::EDGE_Z - 300.f && t.base.y + t.height > 300.f;
    CHECK(towersOver, "the Store's towers rise out of the Void across the chasm, well above the shelf");
    CHECK(glm::dot(cold::giantDir(), glm::vec3{0.f, 0.f, -1.f}) > 0.7f && cold::giantDir().y > 0.2f, "the gas giant hangs over the Store, clear of the horizon");

    // ---- the ground ----
    for (int which = 0; which < 2; ++which) {
        auto px = which ? MaterialTex::ice() : MaterialTex::snow();
        const int S = MaterialTex::SIZE;
        double seamX = 0.0, seamY = 0.0, mean = 0.0;
        for (int y = 0; y < S; ++y) for (int c = 0; c < 3; ++c) seamX += std::abs((int)px[(y * S) * 4 + c] - (int)px[(y * S + S - 1) * 4 + c]);
        for (int x = 0; x < S; ++x) for (int c = 0; c < 3; ++c) seamY += std::abs((int)px[x * 4 + c] - (int)px[((S - 1) * S + x) * 4 + c]);
        for (int i = 0; i < S * S; ++i) mean += px[i * 4];
        mean /= S * S;
        std::printf("      %s: edge difference %.1f / %.1f, mean red %.0f\n", which ? "ice" : "snow", seamX / (S * 3), seamY / (S * 3), mean);
        CHECK(seamX / (S * 3) < 40.0 && seamY / (S * 3) < 40.0, which ? "the ice texture wraps without a hard seam" : "the snow texture wraps without a hard seam");
    }

    // ---- the enemies ----
    {   // THAWLING: crawls, then stands and gets quicker as it warms
        EnemyWorld w; w.playerFeet = {30.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        Enemy e(EnemyType::THAWLING, {0.f, 0.f, 0.f});
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        float crawlX = 0.f, earlyAttacks = 0;
        for (int f = 0; f < (int)(60 * coldfoe::THAW_RISE) - 2; ++f) { e.update(DT, w); earlyAttacks += e.ev.telegraphStarted; }
        crawlX = e.position.x;
        float x0 = e.position.x; for (int f = 0; f < 60; ++f) e.update(DT, w); float coldStep = e.position.x - x0;
        for (int f = 0; f < 60 * 12; ++f) { e.update(DT, w); if (e.position.x > 20.f) e.position.x = 5.f; }
        float x1 = e.position.x; for (int f = 0; f < 60; ++f) e.update(DT, w); float warmStep = e.position.x - x1;
        std::printf("      thawling: crawled %.1f m, cold %.1f m/s, warm %.1f m/s\n", crawlX, coldStep, warmStep);
        CHECK(earlyAttacks == 0 && crawlX > 0.5f && crawlX < 6.f, "a THAWLING crawls at first, slowly and without attacking");
        CHECK(warmStep > coldStep * 1.4f, "...and is much quicker once it has warmed up");
    }
    {   // PICKER: a white tell over you, then the drop takes a player who stays put
        EnemyWorld w; w.playerFeet = {6.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        Enemy e(EnemyType::PICKER, {0.f, 4.6f, 0.f}, 0.f);
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        bool tell = false, white = false, grabbed = false;
        for (int f = 0; f < 60 * 15 && !grabbed; ++f) {
            e.update(DT, w);
            tell |= e.attack == AttackKind::SORT_GRAB && e.telegraphTimer > 0.f;
            white |= e.parryWindow();
            grabbed |= e.ev.sortGrab;
        }
        CHECK(tell && white, "a PICKER winds its snatch up with a parry window");
        CHECK(grabbed && e.pickT > 0.f, "...and its drop takes you if you stay under it");
        e.stun(1.f);
        CHECK(e.pickT <= 0.f, "stunned (punched), it lets go");
    }
    {   // CLERK: never attacks, keeps its distance
        EnemyWorld w; w.playerFeet = {4.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        Enemy e(EnemyType::CLERK, {0.f, 3.4f, 0.f}, 0.f);
        e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        bool attacked = false;
        for (int f = 0; f < 60 * 10; ++f) { e.update(DT, w); attacked |= e.ev.telegraphStarted || e.ev.shots > 0 || e.ev.meleeHit; }
        float d = glm::length(glm::vec2{e.position.x - 4.f, e.position.z});
        CHECK(!attacked, "a CLERK never attacks");
        CHECK(d > 9.f, "...and backs off to keep its distance from you");
    }

    std::printf(failures ? "\n%d FAILED\n" : "\nALL PASSED (0 failures)\n", failures);
    return failures ? 1 : 0;
}
