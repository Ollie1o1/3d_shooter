// Headless game-logic tests — no window or GL context needed.
// Build + run with `make test`.
//
// Covers both maps' data (spawns clear of walls, jump pads that actually land
// you on their platform, moving platforms that never pass through a wall,
// ceilings above every spawn), each enemy type's AI, the box rigs, the wave
// director driven through an entire simulated ARENA run and FAST run, the
// weapon and upgrade numbers, XP, and the mouse spike filter.
#include "../src/WaveDirector.h"
#include "../src/LevelDescent.h"
#include "../src/EnemyModel.h"
#include "../src/Weapons.h"
#include "../src/Progression.h"
#include "../src/MouseFilter.h"
#include <cstdio>
#include <cstring>
#include <map>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)

static constexpr float DT = 1.f / 60.f;

static bool overlapsBox(const AABB& b, const AABB& o, float eps = 0.f) {
    return b.max.x > o.min.x + eps && b.min.x < o.max.x - eps && b.max.y > o.min.y + eps && b.min.y < o.max.y - eps &&
           b.max.z > o.min.z + eps && b.min.z < o.max.z - eps;
}
static bool overlapsWall(const LevelData& L, const AABB& b) {
    for (auto& w : L.walls) if (overlapsBox(b, w.box)) return true;
    return false;
}
static AABB boxAt(glm::vec3 p, float r, float h) {
    return { p + glm::vec3{-r, 0.02f, -r}, p + glm::vec3{r, h, r} };
}
static bool inside(const AABB& b, glm::vec3 p) {
    return p.x >= b.min.x && p.x <= b.max.x && p.z >= b.min.z && p.z <= b.max.z;
}

// Every point a ground enemy can spawn at in an arena (all wave tiers)
static std::vector<glm::vec3> allGround(const Arena& a) {
    std::vector<glm::vec3> v = a.groundSpawns;
    for (auto& w : a.waveGround) v.insert(v.end(), w.begin(), w.end());
    return v;
}

// Every jump pad must actually deliver the player onto something higher
static bool padsLand(const LevelData& L, const SpatialGrid& grid, Uint8* keys) {
    bool allLand = true;
    for (auto& pad : L.pads) {
        Player p(pad.centre);
        p.velocity = pad.launch;
        p.onGround = false;
        p.update(DT, keys, L.walls.data(), (int)L.walls.size(), false, &grid);
        int ticks = 0;
        while (!p.onGround && ticks < 600) { p.update(DT, keys, L.walls.data(), (int)L.walls.size(), false, &grid); ++ticks; }
        std::printf("      pad (%.1f, %.1f, %.1f) -> lands y=%.2f after %.2fs\n", pad.centre.x, pad.centre.y, pad.centre.z, p.position.y, ticks * DT);
        if (p.position.y < pad.centre.y + 3.4f) allLand = false;
    }
    return allLand;
}

// Sweep every mover over two full periods: it must never pass through a static wall
static bool moversClear(LevelData L) {
    bool ok = true;
    for (int mi = 0; mi < (int)L.movers.size(); ++mi) {
        const Mover& m = L.movers[mi];
        for (float t = 0.f; t < m.period * 2.f && ok; t += 0.05f) {
            L.updateMovers(t);
            const AABB& b = L.walls[m.wall].box;
            for (auto& w : L.walls) {
                if (w.dynamic) continue;
                if (overlapsBox(b, w.box, 0.01f)) {
                    std::printf("      mover %d hits a wall at t=%.2f (%.1f %.1f %.1f)\n", mi, t, b.min.x, b.min.y, b.min.z);
                    ok = false; break;
                }
            }
        }
    }
    return ok;
}

int main() {
    srand(7);
    LevelData L = buildLevel();
    SpatialGrid grid; grid.build(L.walls);
    Uint8 keys[SDL_NUM_SCANCODES];
    std::memset(keys, 0, sizeof(keys));

    // ---------------------------------------------------------------- arena map
    {
        CHECK(L.arenas.size() == 4, "four arenas");
        CHECK(L.corridors.size() == 3, "three corridors join them");
        bool groundOk = true, airOk = true, inBounds = true, starts = true, underCeiling = true;
        for (auto& a : L.arenas) {
            // Largest ground enemy (Brute) must fit at every ground spawn
            for (auto& s : allGround(a)) {
                if (overlapsWall(L, boxAt(s, statsOf(EnemyType::BRUTE).radius, statsOf(EnemyType::BRUTE).height))) {
                    std::printf("      ground spawn (%.1f %.1f %.1f) in a wall\n", s.x, s.y, s.z); groundOk = false; }
                if (!inside(a.bounds, s)) inBounds = false;
                if (s.y + 3.f > a.zone.max.y) underCeiling = false;
            }
            for (auto& s : a.airSpawns) {
                if (overlapsWall(L, boxAt(s, statsOf(EnemyType::RAPTOR).radius, statsOf(EnemyType::RAPTOR).height))) {
                    std::printf("      air spawn (%.1f %.1f %.1f) in a wall\n", s.x, s.y, s.z); airOk = false; }
                if (!inside(a.bounds, s) || s.y + statsOf(EnemyType::RAPTOR).height > a.bounds.max.y) inBounds = false;
            }
            if (overlapsWall(L, boxAt(a.playerStart, 0.4f, 1.8f)) || !inside(a.zone, a.playerStart)) starts = false;
        }
        const Arena& last = L.arenas.back();
        glm::vec3 mirrored{-last.bossSpawn.x, last.bossSpawn.y, last.bossSpawn.z};
        CHECK(!overlapsWall(L, boxAt(last.bossSpawn, statsOf(EnemyType::WARDEN).radius, statsOf(EnemyType::WARDEN).height)) &&
              !overlapsWall(L, boxAt(mirrored, statsOf(EnemyType::WARDEN).radius, statsOf(EnemyType::WARDEN).height)),
              "the Warden fits at both of his spawn points");
        CHECK(groundOk, "every ground spawn (all Spire tiers too) fits a Brute without touching a wall");
        CHECK(airOk, "every air spawn is clear of walls");
        CHECK(inBounds, "every spawn is inside its arena's bounds");
        CHECK(starts, "every player start is inside its arena and clear of walls");
        CHECK(underCeiling, "every arena's ceiling leaves headroom above its highest spawn");
        // Scenery (props have no collision) must stay out of the playable space:
        // anything taller than a person inside a zone would be a ghost wall
        bool sceneryClear = true;
        for (auto& pr : L.props) {
            if (pr.box.max.y - pr.box.min.y < 9.f) continue;   // struts, chains, palms are fine
            for (auto& a : L.arenas)
                if (pr.box.max.x > a.zone.min.x && pr.box.min.x < a.zone.max.x &&
                    pr.box.max.z > a.zone.min.z && pr.box.min.z < a.zone.max.z) {
                    std::printf("      scenery (%.0f..%.0f, %.0f..%.0f) inside %s\n", pr.box.min.x, pr.box.max.x,
                                pr.box.min.z, pr.box.max.z, a.name);
                    sceneryClear = false;
                }
        }
        CHECK(sceneryClear, "no tall scenery sits inside an arena (you'd walk through it)");
        bool ceilings = true;
        for (auto& a : L.arenas) if (a.zone.max.y > 36.f || a.zone.max.y < 12.f) ceilings = false;
        CHECK(ceilings, "every arena has an invisible ceiling (12-36 m), so you can't fly out of the map");
        int waves = 0;
        for (auto& a : L.arenas) waves += (int)a.waves.size();
        CHECK(waves == 12, "three waves per arena");
    }

    // The Spire: each wave spawns a tier higher, and it has things to ride
    {
        const Arena& S = L.arenas[2];
        auto meanY = [](const std::vector<glm::vec3>& v) { float s = 0; for (auto& p : v) s += p.y; return s / v.size(); };
        bool climbs = S.waveGround.size() == 3 && meanY(S.waveGround[0]) < meanY(S.waveGround[1]) &&
                      meanY(S.waveGround[1]) < meanY(S.waveGround[2]);
        std::printf("      spire wave spawn heights: %.1f  %.1f  %.1f\n", meanY(S.waveGround[0]), meanY(S.waveGround[1]), meanY(S.waveGround[2]));
        CHECK(std::string(S.name) == "THE SPIRE" && climbs, "the Spire's waves spawn higher and higher");
        int moversInSpire = 0;
        for (auto& m : L.movers) if (inside(S.zone, (m.base.min + m.base.max) * 0.5f)) ++moversInSpire;
        CHECK(moversInSpire >= 8, "the Spire has lifts, sweepers and orbiting platforms");
        float top = 0.f;
        for (auto& p : S.waveGround[2]) top = std::max(top, p.y);
        CHECK(top >= 26.f, "the last Spire wave reaches the summit (26 m)");
    }

    CHECK(padsLand(L, grid, keys), "every arena jump pad lands the player on something higher");

    // Moving platforms
    {
        bool flagged = true;
        for (auto& m : L.movers) if (!L.walls[m.wall].dynamic || !L.walls[m.wall].hidden) flagged = false;
        CHECK(!L.movers.empty() && flagged, "movers are dynamic walls kept out of the static mesh and grid");
        LevelData M = L;
        glm::vec3 before = M.walls[M.movers[0].wall].box.min;
        M.updateMovers(M.movers[0].period * 0.5f);
        glm::vec3 after = M.walls[M.movers[0].wall].box.min;
        CHECK(glm::length(after - before) > 1.f && glm::length(M.movers[0].delta - (after - before)) < 1e-4f,
              "updateMovers moves the wall and records how far it went");
        CHECK(moversClear(L), "no arena mover ever passes through a wall");
    }

    // Doors: closed doors block the gap, and open ones sit fully under their base
    {
        bool ok = true;
        for (auto& d : L.doors) {
            const AABB& b = L.walls[d.wall].box;
            if (d.open  && b.max.y > d.baseY + 0.001f) ok = false;
            if (!d.open && b.max.y < d.baseY + d.height - 0.001f) ok = false;
        }
        CHECK(ok, "doors start fully open or fully closed");
        CHECK(L.arenas[0].exitDoor >= 0 && !L.doors[L.arenas[0].exitDoor].open, "arena 1's exit starts locked");
    }

    // ---------------------------------------------------------------- the Descent (FAST)
    LevelData D = buildDescent();
    SpatialGrid dgrid; dgrid.build(D.walls);
    {
        CHECK(D.fast && D.arenas.size() == 6, "the Descent has six sections");
        bool descends = true, placedOk = true, inBounds = true, starts = true, gates = true;
        for (size_t i = 0; i < D.arenas.size(); ++i) {
            const Arena& a = D.arenas[i];
            if (i > 0 && D.arenas[i].bounds.min.y > D.arenas[i - 1].bounds.min.y) descends = false;
            for (auto& w : a.waves) for (auto& e : w) {
                if (e.at.empty()) placedOk = false;
                for (auto& p : e.at) {
                    const EnemyStats& st = statsOf(e.type);
                    if (overlapsWall(D, boxAt(p, st.radius, st.height))) {
                        std::printf("      %s at (%.1f %.1f %.1f) is in a wall\n", st.name, p.x, p.y, p.z); placedOk = false; }
                    if (!inside(a.bounds, p) || p.y < a.bounds.min.y - 0.01f) {
                        std::printf("      %s at (%.1f %.1f %.1f) is outside section %d\n", st.name, p.x, p.y, p.z, (int)i); inBounds = false; }
                }
            }
            if (overlapsWall(D, boxAt(a.playerStart, 0.4f, 1.8f)) || !inside(a.zone, a.playerStart)) {
                std::printf("      section %d start (%.1f %.1f %.1f) blocked\n", (int)i, a.playerStart.x, a.playerStart.y, a.playerStart.z); starts = false; }
            if (i + 1 < D.arenas.size() && (a.exitDoor < 0 || D.doors[a.exitDoor].open)) gates = false;
        }
        CHECK(descends, "every section is lower than the one before it");
        CHECK(placedOk, "every FAST enemy is hand-placed and clear of walls");
        CHECK(inBounds, "every FAST enemy is placed inside its own section");
        CHECK(starts, "every section's checkpoint is clear of walls");
        CHECK(gates, "every section but the last is gated until it's cleared");
        CHECK(D.arenas[2].voidY > 0.f, "the chasm has a void plane that sends you back");
        CHECK(D.finishPos.z < D.arenas.back().zone.max.z && D.finishPos.z > D.arenas.back().zone.min.z,
              "the finish beacon is in the last section");
        CHECK(D.parTimes[0] > 0.f && D.parTimes[0] < D.parTimes[1] && D.parTimes[1] < D.parTimes[2] && D.parTimes[2] < D.parTimes[3],
              "par times are ordered S < A < B < C");
        CHECK(padsLand(D, dgrid, keys), "every Descent jump pad lands the player on something higher");
        CHECK(moversClear(D), "no Descent mover ever passes through a wall");
    }

    // A player standing on a lift rides it up (the carry GameplayState does)
    {
        LevelData M = L;
        // the Spire's first lift: tier 1 (y 6) to tier 2 (y 12)
        const Mover& lift = M.movers[0];
        glm::vec3 c = (M.walls[lift.wall].box.min + M.walls[lift.wall].box.max) * 0.5f;
        Player p({c.x, M.walls[lift.wall].box.max.y + 0.01f, c.z});
        p.dynWalls = M.moverWalls.data(); p.dynCount = (int)M.moverWalls.size();
        float topY = 0.f, t = 0.f; int airborne = 0;
        for (int i = 0; i < (int)(lift.period / DT); ++i) {
            int ride = M.moverOfWall(p.groundWall);
            t += DT;
            M.updateMovers(t);
            if (ride >= 0) p.position += M.movers[ride].delta;
            p.update(DT, keys, M.walls.data(), (int)M.walls.size(), false, &grid);
            if (!p.onGround) ++airborne;
            topY = std::max(topY, p.position.y);
        }
        std::printf("      rode the lift to y=%.2f, airborne %d ticks\n", topY, airborne);
        CHECK(topY > 11.5f && airborne < 5, "a lift carries the player up to the next tier without bouncing");
    }

    // ---------------------------------------------------------------- models
    {
        bool ok = true;
        for (int t = 0; t < (int)EnemyType::COUNT; ++t) {
            Enemy e((EnemyType)t, {0, 0, 0});
            std::vector<BoxInstance> parts;
            buildEnemy(e, 0.f, parts);
            std::printf("      %-8s %2d parts\n", statsOf((EnemyType)t).name, (int)parts.size());
            if (parts.size() < 8) ok = false;
        }
        CHECK(ok, "every enemy is a multi-part model (>= 8 boxes), not a single cube");
    }

    // ---------------------------------------------------------------- AI
    const Arena& A0 = L.arenas[0];
    const Arena& BOSS = L.arenas.back();
    auto worldFor = [&](const LevelData& lv, const SpatialGrid& g, glm::vec3 playerFeet, const Arena& a) {
        EnemyWorld w;
        w.playerFeet = playerFeet;
        w.playerEye  = playerFeet + glm::vec3{0, 1.7f, 0};
        w.walls = lv.walls.data(); w.wallCount = (int)lv.walls.size(); w.grid = &g;
        w.bounds = a.bounds;
        return w;
    };

    {
        Enemy e(EnemyType::HUSK, {0, 0, -20});
        bool hitWhileSpawning = e.takeDamage(1000.f);
        CHECK(!hitWhileSpawning && e.alive, "enemies can't be hurt while materialising");
    }

    // Run one enemy against a stationary player and report what it did
    struct Seen { int shots = 0, melee = 0, slams = 0, summons = 0; bool detonated = false, dived = false,
                  leftBounds = false, inWall = false; float closest = 1e9f, minY = 1e9f, maxY = -1e9f; };
    auto simulate = [&](const LevelData& lv, const SpatialGrid& g, EnemyType t, glm::vec3 start, glm::vec3 player,
                        const Arena& a, float seconds) {
        Seen s;
        Enemy e(t, start);
        EnemyWorld w = worldFor(lv, g, player, a);
        for (int i = 0; i < (int)(seconds / DT) && e.alive; ++i) {
            e.update(DT, w);
            s.shots += e.ev.shots;
            s.melee += e.ev.meleeHit;
            s.slams += e.ev.slam;
            s.summons += e.ev.summonMites + e.ev.summonRippers;
            s.detonated |= e.ev.detonated;
            s.dived |= e.diveTimer > 0.f;
            if (!inside(a.bounds, e.position)) s.leftBounds = true;
            if (!e.stats().flying && e.state == EnemyState::ACTIVE) {
                AABB box = boxAt(e.position + glm::vec3{0, 0.05f, 0}, e.radius() - 0.05f, e.height() - 0.1f);
                for (auto& wl : lv.walls) if (!wl.dynamic && overlapsBox(box, wl.box)) s.inWall = true;
            }
            s.closest = std::min(s.closest, glm::length(glm::vec2(e.position.x - player.x, e.position.z - player.z)));
            if (e.state == EnemyState::ACTIVE) { s.minY = std::min(s.minY, e.position.y); s.maxY = std::max(s.maxY, e.position.y); }
        }
        return s;
    };
    glm::vec3 P0 = A0.playerStart;

    { auto s = simulate(L, grid, EnemyType::HUSK, {0,0,-20}, P0, A0, 12.f);
      CHECK(s.shots >= 3, "HUSK keeps shooting at a player in the open");
      CHECK(!s.inWall && !s.leftBounds, "HUSK stays out of walls and inside the arena"); }
    { auto s = simulate(L, grid, EnemyType::RIPPER, {-16,0,-24}, P0, A0, 12.f);
      CHECK(s.melee >= 1, "RIPPER closes the distance and lands a lunge");
      CHECK(!s.inWall, "RIPPER steers around cover instead of clipping into it"); }
    { auto s = simulate(L, grid, EnemyType::SENTINEL, {16,0,-24}, P0, A0, 12.f);
      CHECK(s.shots >= 3 && s.shots % 3 == 0, "SENTINEL fires in 3-round bursts"); }
    { auto s = simulate(L, grid, EnemyType::RAPTOR, {0,8,-10}, P0, A0, 15.f);
      CHECK(s.shots >= 1, "RAPTOR shoots from the air");
      CHECK(s.dived, "RAPTOR dives at the player"); }
    { auto s = simulate(L, grid, EnemyType::BRUTE, {0,0,10}, P0, A0, 15.f);
      CHECK(s.slams >= 1, "BRUTE walks up and slams the ground"); }
    { auto s = simulate(L, grid, EnemyType::MITE, {0,0,8}, P0, A0, 10.f);
      CHECK(s.detonated, "MITE runs in and detonates"); }
    { auto s = simulate(L, grid, EnemyType::WARDEN, BOSS.bossSpawn, BOSS.playerStart, BOSS, 30.f);
      CHECK(s.shots >= 7, "WARDEN fires volleys");
      CHECK(s.summons >= 1, "WARDEN summons adds");
      CHECK(!s.leftBounds, "WARDEN stays inside the arena"); }
    { Enemy w(EnemyType::WARDEN, BOSS.bossSpawn);
      w.update(Enemy::SPAWN_TIME + DT, worldFor(L, grid, BOSS.playerStart, BOSS));
      w.takeDamage(w.maxHealth * 0.55f);
      CHECK(w.enraged && w.ev.enraged, "WARDEN enrages below half health"); }

    // A Ripper starting behind the furnace in the Foundry has to go around it
    { const Arena& A1 = L.arenas[1];
      auto s = simulate(L, grid, EnemyType::RIPPER, {0,0,-86}, {0,0,-64}, A1, 10.f);
      std::printf("      ripper behind furnace: closest approach %.1f m\n", s.closest);
      CHECK(s.closest < 3.f, "RIPPER finds its way around the furnace to the player"); }

    // Gunners hold the high ground; rushers jump down after you
    { const Arena& S = L.arenas[2];
      glm::vec3 below{0.f, 0.f, -134.f};
      auto sen = simulate(L, grid, EnemyType::SENTINEL, {0, 26.05f, -156.f}, below, S, 15.f);
      auto husk = simulate(L, grid, EnemyType::HUSK, {-26, 6.05f, -156.f}, below, S, 15.f);
      auto rip = simulate(L, grid, EnemyType::RIPPER, {26, 6.05f, -156.f}, below, S, 15.f);
      std::printf("      summit sentinel lowest y=%.1f, ledge husk lowest y=%.1f, ledge ripper lowest y=%.1f\n", sen.minY, husk.minY, rip.minY);
      CHECK(sen.minY > 25.5f && sen.shots >= 3, "a Sentinel on the Spire's summit holds it and keeps firing");
      CHECK(husk.minY > 5.5f, "a Husk on a ledge doesn't walk off it");
      CHECK(rip.minY < 0.5f, "a Ripper on a ledge drops down to chase you"); }

    // Flyers stay above the floor of a high section
    { const Arena& S0 = D.arenas[0];
      auto s = simulate(D, dgrid, EnemyType::RAPTOR, {0, 66.f, -15.f}, S0.playerStart, S0, 15.f);
      CHECK(s.minY >= S0.bounds.min.y + 1.4f, "a Raptor high in the Descent never sinks below its section's floor"); }

    // ---------------------------------------------------------------- director: ARENA
    {
        WaveDirector d; d.level = &L;
        d.startArena(0);
        std::map<DirectorEvent, int> counts;
        int spawned = 0, expected = 0, maxAliveSeen = 0, bossSpawns = 0;
        bool tooClose = false, overCap = false;
        for (auto& a : L.arenas) for (auto& w : a.waves) for (auto& e : w) expected += e.total();

        glm::vec3 player = L.arenas[0].playerStart;
        std::vector<float> alive;   // remaining lifetime of each live enemy
        for (int tick = 0; tick < 60 * 60 * 30 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
            std::vector<SpawnRequest> out;
            d.update(DT, (int)alive.size(), player, out);
            for (auto& r : out) {
                ++spawned;
                if (r.type == EnemyType::WARDEN) ++bossSpawns;
                if (glm::length(glm::vec2(r.pos.x - player.x, r.pos.z - player.z)) < WaveDirector::SAFE_RADIUS) tooClose = true;
                alive.push_back(3.f);   // each enemy "survives" 3 s, so the cap gets exercised
            }
            if ((int)alive.size() > d.current().maxAlive) overCap = true;
            maxAliveSeen = std::max(maxAliveSeen, (int)alive.size());
            for (auto& t : alive) t -= DT;
            alive.erase(std::remove_if(alive.begin(), alive.end(), [](float t){ return t <= 0.f; }), alive.end());
            for (auto& ev : d.events) counts[ev.kind]++;
            d.events.clear();
            // Walk into the next arena once the gate opens
            if (d.phase == WaveDirector::Phase::CLEARED) player = L.arenas[d.arena + 1].playerStart;
        }
        int n = (int)L.arenas.size();
        std::printf("      spawned %d of %d, peak alive %d\n", spawned, expected, maxAliveSeen);
        CHECK(d.phase == WaveDirector::Phase::VICTORY, "a simulated ARENA run reaches victory");
        CHECK(spawned == expected, "every queued enemy spawns exactly once");
        CHECK(bossSpawns == 1, "the Warden spawns once");
        CHECK(counts[DirectorEvent::ARENA_START] == n, "every arena starts");
        CHECK(counts[DirectorEvent::WAVE_START] == 3 * n - 1 && counts[DirectorEvent::BOSS_START] == 1, "every wave starts, the last is the boss");
        CHECK(counts[DirectorEvent::ARENA_CLEARED] == n && counts[DirectorEvent::VICTORY] == 1 &&
              counts[DirectorEvent::FINISH_OPEN] == 0, "each arena clears, then victory");
        CHECK(counts[DirectorEvent::NEW_TYPE] == (int)EnemyType::COUNT, "each enemy type is introduced exactly once");
        CHECK(!overCap, "never more enemies alive than the arena's cap");
        CHECK(!tooClose, "nothing spawns within the safe radius of the player");
    }

    // Regression: a wave whose last enemy spawns into an empty field must not
    // count as cleared on that same tick (the one-enemy boss wave always did).
    {
        WaveDirector d; d.level = &L;
        d.startArena((int)L.arenas.size() - 1);
        d.wave = d.waveCount() - 1;       // the Warden's wave
        std::vector<SpawnRequest> out;
        int alive = 0;
        bool clearedWithBossAlive = false;
        for (int tick = 0; tick < 60 * 10; ++tick) {
            out.clear();
            d.update(DT, alive, L.arenas.back().playerStart, out);
            alive += (int)out.size();
            if (alive > 0 && d.phase != WaveDirector::Phase::ACTIVE) clearedWithBossAlive = true;
        }
        CHECK(alive == 1 && d.phase == WaveDirector::Phase::ACTIVE && !clearedWithBossAlive,
              "the boss wave stays active while the Warden is alive");
    }

    // ---------------------------------------------------------------- director: FAST
    {
        WaveDirector d; d.level = &D; d.fast = true;
        d.startArena(0);
        std::map<DirectorEvent, int> counts;
        int spawned = 0, expected = 0, sectionsSeen = 0;
        bool allAtOnce = true, exact = true;
        for (auto& a : D.arenas) for (auto& w : a.waves) for (auto& e : w) expected += e.total();
        int alive = 0, killIn = 0;
        float clock = 0.f;
        for (int tick = 0; tick < 60 * 60 * 10 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
            std::vector<SpawnRequest> out;
            int sec = d.arena, wave = d.wave;
            d.update(DT, alive, D.arenas[d.arena].playerStart, out);
            if (!out.empty()) {
                int want = 0;
                for (auto& e : D.arenas[sec].waves[wave]) want += e.total();
                if ((int)out.size() != want) allAtOnce = false;
                for (auto& r : out) {
                    bool found = false;
                    for (auto& e : D.arenas[sec].waves[wave]) for (auto& p : e.at) if (p == r.pos && e.type == r.type) found = true;
                    if (!found) exact = false;
                }
                spawned += (int)out.size();
                alive += (int)out.size();
                killIn = 90;   // the player kills the lot 1.5 s later
            }
            if (killIn > 0 && --killIn == 0) alive = 0;
            for (auto& ev : d.events) { counts[ev.kind]++; if (ev.kind == DirectorEvent::ARENA_START) ++sectionsSeen; }
            d.events.clear();
            clock += DT;
        }
        std::printf("      FAST: spawned %d of %d across %d sections in %.1fs of simulated play\n", spawned, expected, sectionsSeen, clock);
        CHECK(d.phase == WaveDirector::Phase::VICTORY, "a simulated FAST run clears every section");
        CHECK(spawned == expected, "every hand-placed enemy spawns exactly once");
        CHECK(allAtOnce, "a FAST wave arrives all at once, not trickled");
        CHECK(exact, "FAST enemies appear exactly at their hand-placed points");
        CHECK(sectionsSeen == (int)D.arenas.size(), "sections chain straight into each other");
        CHECK(counts[DirectorEvent::FINISH_OPEN] == 1 && counts[DirectorEvent::VICTORY] == 0,
              "clearing the last section opens the finish (the beacon ends the run)");
        CHECK(clock < 60.f, "FAST mode never makes you wait (no intros or breaks)");
    }

    // ---------------------------------------------------------------- weapons + progression
    {
        WeaponUpgrades none;
        float maxRegular = 0.f;
        for (int t = 0; t < (int)EnemyType::COUNT; ++t)
            if ((EnemyType)t != EnemyType::WARDEN) maxRegular = std::max(maxRegular, statsOf((EnemyType)t).health);
        CHECK(weaponDamage(WeaponId::LONGSHOT, none) >= maxRegular, "the Longshot one-shots every regular enemy");
        CHECK(weaponDamage(WeaponId::KAR, none) * weaponDef(WeaponId::KAR).headMult >= maxRegular,
              "a Kar98 headshot one-shots every regular enemy");
        CHECK(weaponDamage(WeaponId::KAR, none) >= statsOf(EnemyType::HUSK).health, "a Kar98 body shot drops a Husk");
        CHECK(weaponDef(WeaponId::KAR).canAim && !weaponDef(WeaponId::KAR).scope &&
              weaponDef(WeaponId::LONGSHOT).canAim && weaponDef(WeaponId::LONGSHOT).scope,
              "the Kar98 uses iron sights, the Longshot a full scope");
        CHECK(weaponDef(WeaponId::LONGSHOT).aimFov < weaponDef(WeaponId::KAR).aimFov,
              "the Longshot zooms in much further than the Kar98");
        CHECK(weaponSpread(WeaponId::LONGSHOT, none, 1.f) == 0.f && weaponSpread(WeaponId::LONGSHOT, none, 0.f) > 0.05f,
              "the Longshot is pinpoint scoped and wild from the hip");
        CHECK(weaponPierce(WeaponId::LONGSHOT, none) >= 2, "Longshot rounds punch through several enemies");

        bool monotonic = true;
        for (int w = 0; w < WEAPON_COUNT; ++w) {
            WeaponId id = (WeaponId)w;
            WeaponUpgrades u;
            for (int k = 0; k < MAX_TIER; ++k) {
                WeaponUpgrades n = u;
                ++n.tier[0]; ++n.tier[1]; ++n.tier[2];
                if (!(weaponDamage(id, n) > weaponDamage(id, u) && weaponCooldown(id, n) < weaponCooldown(id, u) &&
                      weaponMag(id, n) > weaponMag(id, u) && weaponReload(id, n) < weaponReload(id, u) &&
                      weaponCooldown(id, n) > 0.05f)) monotonic = false;
                u = n;
            }
        }
        CHECK(monotonic, "every upgrade tier makes its gun strictly better");

        Progression p;
        CHECK(!p.buy(WeaponId::KAR, UpgradeStat::DAMAGE), "you can't buy an upgrade without points");
        int gained = p.addXp(Progression::xpToNext(1) + Progression::xpToNext(2) + 5);
        CHECK(gained == 2 && p.level == 3 && p.points == 2 && p.xp == 5, "XP carries over across level-ups");
        CHECK(p.buy(WeaponId::KAR, UpgradeStat::DAMAGE) && p.tierOf(WeaponId::KAR, UpgradeStat::DAMAGE) == 1 && p.points == 1,
              "buying an upgrade spends a point and raises the tier");
        CHECK(!p.buy(WeaponId::KAR, UpgradeStat::MOD), "a MOD costs two points");
        p.points = 10;
        for (int k = 0; k < 5; ++k) p.buy(WeaponId::REVOLVER, UpgradeStat::RATE);
        CHECK(p.tierOf(WeaponId::REVOLVER, UpgradeStat::RATE) == MAX_TIER && p.points == 10 - MAX_TIER,
              "upgrades stop at the max tier");
        CHECK(styleXpMultiplier(StyleRank::SSS) > styleXpMultiplier(StyleRank::D), "stylish kills earn more XP");
        CHECK(formatTime(83.456f) == "1:23.46" && formatTime(5.f, false) == "0:05", "times format as m:ss.hh");
    }

    // ---------------------------------------------------------------- mouse filter
    {
        MouseFilter f;
        f.onCapture();
        bool skipsCapture = !f.accept(900.f, 0.f) && !f.accept(400.f, 0.f);
        bool normal = true;
        for (int i = 0; i < 20; ++i) normal &= f.accept(12.f, -3.f);
        bool dropsSpike = !f.accept(2400.f, 10.f);
        bool resumes = f.accept(14.f, 2.f);
        // A real flick: two big events in a row get through after at most one drop
        int through = 0;
        for (float v : {300.f, 600.f, 700.f, 500.f}) through += f.accept(v, 0.f);
        CHECK(skipsCapture, "the jump on (re)capturing the mouse is ignored");
        CHECK(normal, "ordinary mouse movement passes through");
        CHECK(dropsSpike && resumes, "a single huge spike (the 180 snap) is dropped");
        CHECK(through >= 3, "a genuine fast flick still turns the camera");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
