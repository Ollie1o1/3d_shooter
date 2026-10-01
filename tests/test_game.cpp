// Headless game-logic tests — no window or GL context needed.
// Build + run with `make test`.
//
// Covers the level data (spawns clear of walls, jump pads that actually land
// you on their platform), each enemy type's AI doing its thing, the box rigs,
// and the wave director driven through an entire simulated run.
#include "../src/WaveDirector.h"
#include "../src/EnemyModel.h"
#include <cstdio>
#include <cstring>
#include <map>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)

static constexpr float DT = 1.f / 60.f;

static bool overlapsWall(const LevelData& L, const AABB& b) {
    for (auto& w : L.walls) {
        const AABB& o = w.box;
        if (b.max.x > o.min.x && b.min.x < o.max.x && b.max.y > o.min.y && b.min.y < o.max.y &&
            b.max.z > o.min.z && b.min.z < o.max.z) return true;
    }
    return false;
}
static AABB boxAt(glm::vec3 p, float r, float h) {
    return { p + glm::vec3{-r, 0.02f, -r}, p + glm::vec3{r, h, r} };
}
static bool inside(const AABB& b, glm::vec3 p) {
    return p.x >= b.min.x && p.x <= b.max.x && p.z >= b.min.z && p.z <= b.max.z;
}

int main() {
    srand(7);
    LevelData L = buildLevel();
    SpatialGrid grid; grid.build(L.walls);
    Uint8 keys[SDL_NUM_SCANCODES];
    std::memset(keys, 0, sizeof(keys));

    // ---------------------------------------------------------------- level
    {
        CHECK(L.arenas.size() == 3, "three arenas");
        CHECK(L.corridors.size() == 2, "two corridors join them");
        bool groundOk = true, airOk = true, inBounds = true, starts = true;
        for (auto& a : L.arenas) {
            // Largest ground enemy (Brute) must fit at every ground spawn
            for (auto& s : a.groundSpawns) {
                if (overlapsWall(L, boxAt(s, statsOf(EnemyType::BRUTE).radius, statsOf(EnemyType::BRUTE).height))) {
                    std::printf("      ground spawn (%.1f %.1f %.1f) in a wall\n", s.x, s.y, s.z); groundOk = false; }
                if (!inside(a.bounds, s)) inBounds = false;
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
        CHECK(groundOk, "every ground spawn fits a Brute without touching a wall");
        CHECK(airOk, "every air spawn is clear of walls");
        CHECK(inBounds, "every spawn is inside its arena's bounds");
        CHECK(starts, "every player start is inside its arena and clear of walls");
        int waves = 0;
        for (auto& a : L.arenas) waves += (int)a.waves.size();
        CHECK(waves == 9, "three waves per arena");
    }

    // Every jump pad must actually deliver the player onto something higher
    {
        bool allLand = true;
        for (auto& pad : L.pads) {
            Player p(pad.centre);
            p.velocity = pad.launch;
            p.onGround = false;
            p.update(DT, keys, L.walls.data(), (int)L.walls.size(), false, &grid);
            int ticks = 0;
            while (!p.onGround && ticks < 600) { p.update(DT, keys, L.walls.data(), (int)L.walls.size(), false, &grid); ++ticks; }
            std::printf("      pad (%.1f, %.1f) -> lands y=%.2f after %.2fs\n", pad.centre.x, pad.centre.z, p.position.y, ticks * DT);
            if (p.position.y < 3.4f) allLand = false;
        }
        CHECK(allLand, "every jump pad lands the player on a platform (y >= 3.4)");
    }

    // Doors: closed doors block the gap, and open ones sit fully under the floor
    {
        bool ok = true;
        for (auto& d : L.doors) {
            const AABB& b = L.walls[d.wall].box;
            if (d.open  && b.max.y > 0.001f) ok = false;
            if (!d.open && b.max.y < d.height - 0.001f) ok = false;
        }
        CHECK(ok, "doors start fully open or fully closed");
        CHECK(L.arenas[0].exitDoor >= 0 && !L.doors[L.arenas[0].exitDoor].open, "arena 1's exit starts locked");
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
    auto worldFor = [&](glm::vec3 playerFeet, const Arena& a) {
        EnemyWorld w;
        w.playerFeet = playerFeet;
        w.playerEye  = playerFeet + glm::vec3{0, 1.7f, 0};
        w.walls = L.walls.data(); w.wallCount = (int)L.walls.size(); w.grid = &grid;
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
                  leftBounds = false, inWall = false; float closest = 1e9f; };
    auto simulate = [&](EnemyType t, glm::vec3 start, glm::vec3 player, const Arena& a, float seconds) {
        Seen s;
        Enemy e(t, start);
        EnemyWorld w = worldFor(player, a);
        for (int i = 0; i < (int)(seconds / DT) && e.alive; ++i) {
            e.update(DT, w);
            s.shots += e.ev.shots;
            s.melee += e.ev.meleeHit;
            s.slams += e.ev.slam;
            s.summons += e.ev.summonMites + e.ev.summonRippers;
            s.detonated |= e.ev.detonated;
            s.dived |= e.diveTimer > 0.f;
            if (!inside(a.bounds, e.position)) s.leftBounds = true;
            if (!e.stats().flying && e.state == EnemyState::ACTIVE &&
                overlapsWall(L, boxAt(e.position + glm::vec3{0, 0.05f, 0}, e.radius() - 0.05f, e.height() - 0.1f)))
                s.inWall = true;
            s.closest = std::min(s.closest, glm::length(glm::vec2(e.position.x - player.x, e.position.z - player.z)));
        }
        return s;
    };
    glm::vec3 P0 = A0.playerStart;

    { auto s = simulate(EnemyType::HUSK, {0,0,-20}, P0, A0, 12.f);
      CHECK(s.shots >= 3, "HUSK keeps shooting at a player in the open");
      CHECK(!s.inWall && !s.leftBounds, "HUSK stays out of walls and inside the arena"); }
    { auto s = simulate(EnemyType::RIPPER, {-16,0,-24}, P0, A0, 12.f);
      CHECK(s.melee >= 1, "RIPPER closes the distance and lands a lunge");
      CHECK(!s.inWall, "RIPPER steers around cover instead of clipping into it"); }
    { auto s = simulate(EnemyType::SENTINEL, {16,0,-24}, P0, A0, 12.f);
      CHECK(s.shots >= 3 && s.shots % 3 == 0, "SENTINEL fires in 3-round bursts"); }
    { auto s = simulate(EnemyType::RAPTOR, {0,8,-10}, P0, A0, 15.f);
      CHECK(s.shots >= 1, "RAPTOR shoots from the air");
      CHECK(s.dived, "RAPTOR dives at the player"); }
    { auto s = simulate(EnemyType::BRUTE, {0,0,10}, P0, A0, 15.f);
      CHECK(s.slams >= 1, "BRUTE walks up and slams the ground"); }
    { auto s = simulate(EnemyType::MITE, {0,0,8}, P0, A0, 10.f);
      CHECK(s.detonated, "MITE runs in and detonates"); }
    { const Arena& A2 = L.arenas[2];
      auto s = simulate(EnemyType::WARDEN, A2.bossSpawn, A2.playerStart, A2, 30.f);
      CHECK(s.shots >= 7, "WARDEN fires volleys");
      CHECK(s.summons >= 1, "WARDEN summons adds");
      CHECK(!s.leftBounds, "WARDEN stays inside the arena"); }
    { Enemy w(EnemyType::WARDEN, {0,0,-180});
      w.update(Enemy::SPAWN_TIME + DT, worldFor({0,0,-130}, L.arenas[2]));
      w.takeDamage(w.maxHealth * 0.55f);
      CHECK(w.enraged && w.ev.enraged, "WARDEN enrages below half health"); }

    // A Ripper starting behind the furnace in the Foundry has to go around it
    { const Arena& A1 = L.arenas[1];
      auto s = simulate(EnemyType::RIPPER, {0,0,-86}, {0,0,-64}, A1, 10.f);
      std::printf("      ripper behind furnace: closest approach %.1f m\n", s.closest);
      CHECK(s.closest < 3.f, "RIPPER finds its way around the furnace to the player"); }

    // ---------------------------------------------------------------- director
    {
        WaveDirector d; d.level = &L;
        d.startArena(0);
        std::map<DirectorEvent, int> counts;
        int spawned = 0, expected = 0, maxAliveSeen = 0, bossSpawns = 0;
        bool tooClose = false, overCap = false;
        for (auto& a : L.arenas) for (auto& w : a.waves) for (auto& e : w) expected += e.count;

        glm::vec3 player = L.arenas[0].playerStart;
        std::vector<float> alive;   // remaining lifetime of each live enemy
        for (int tick = 0; tick < 60 * 60 * 20 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
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
        std::printf("      spawned %d of %d, peak alive %d\n", spawned, expected, maxAliveSeen);
        CHECK(d.phase == WaveDirector::Phase::VICTORY, "a simulated run reaches victory");
        CHECK(spawned == expected, "every queued enemy spawns exactly once");
        CHECK(bossSpawns == 1, "the Warden spawns once");
        CHECK(counts[DirectorEvent::ARENA_START] == 3, "three arenas start");
        CHECK(counts[DirectorEvent::WAVE_START] == 8 && counts[DirectorEvent::BOSS_START] == 1, "eight waves plus the boss wave");
        CHECK(counts[DirectorEvent::ARENA_CLEARED] == 3 && counts[DirectorEvent::VICTORY] == 1, "each arena clears, then victory");
        CHECK(counts[DirectorEvent::NEW_TYPE] == (int)EnemyType::COUNT, "each enemy type is introduced exactly once");
        CHECK(!overCap, "never more enemies alive than the arena's cap");
        CHECK(!tooClose, "nothing spawns within the safe radius of the player");
    }

    // Regression: a wave whose last enemy spawns into an empty field must not
    // count as cleared on that same tick (the one-enemy boss wave always did).
    {
        WaveDirector d; d.level = &L;
        d.startArena(2);
        d.wave = 2;                       // the Warden's wave
        std::vector<SpawnRequest> out;
        int alive = 0;
        bool clearedWithBossAlive = false;
        for (int tick = 0; tick < 60 * 10; ++tick) {
            out.clear();
            d.update(DT, alive, L.arenas[2].playerStart, out);
            alive += (int)out.size();
            if (alive > 0 && d.phase != WaveDirector::Phase::ACTIVE) clearedWithBossAlive = true;
        }
        CHECK(alive == 1 && d.phase == WaveDirector::Phase::ACTIVE && !clearedWithBossAlive,
              "the boss wave stays active while the Warden is alive");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
