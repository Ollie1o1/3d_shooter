// Headless game-logic tests — no window or GL context needed.
// Build + run with `make test`.
//
// Covers both maps' data (spawns clear of walls, jump pads that actually land
// you on their platform, moving platforms that never pass through a wall,
// ceilings above every spawn), each enemy type's AI, the box rigs, the wave
// director driven through an entire simulated ARENA run and FAST run, the
// weapon and upgrade numbers, XP, and the mouse spike filter.
#include "../src/WaveDirector.h"
#include "../src/LevelGauntlet.h"
#include "../src/LevelAct2.h"
#include "../src/WorldGeometry.h"
#include "../src/EnemyModel.h"
#include "../src/Weapons.h"
#include "../src/Progression.h"
#include "../src/MouseFilter.h"
#include "../src/MusicSynth.h"
#include "../src/Ghost.h"
#include "../src/StyleSystem.h"
#include "../src/Projectile.h"
#include "../src/ArenaShifts.h"
#include "../src/Score.h"
#include "../src/Daily.h"
#include "../src/EndlessWaves.h"
#include "../src/SovereignHazards.h"
#include "../src/SfxMixer.h"
#include "../src/PenitentHazards.h"
#include "../src/GunKit.h"
#include "../src/VoiceSynth.h"
#include "../src/EnemyVoice.h"
#include "../src/WardenHazards.h"
#include "BossSim.h"
#include "../src/RelicHazards.h"
#include "../src/LeviathanHazards.h"
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <functional>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)

static constexpr float DT = 1.f / 60.f;

static bool overlapsBox(const AABB& b, const AABB& o, float eps = 0.f) {
    return b.max.x > o.min.x + eps && b.min.x < o.max.x - eps && b.max.y > o.min.y + eps && b.min.y < o.max.y - eps &&
           b.max.z > o.min.z + eps && b.min.z < o.max.z - eps;
}

// Sound-effects mixer helpers: deterministic test signals and measurements
static std::vector<float> sfxNoise(int n, uint32_t seed = 1) {
    std::vector<float> v(n);
    for (auto& x : v) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; x = (seed & 0xFFFF) / 32767.5f - 1.f; }
    return v;
}
static std::vector<float> sfxSquare(int n) {
    std::vector<float> v(n);
    for (int i = 0; i < n; ++i) v[i] = (i / 50) % 2 ? 1.f : -1.f;
    return v;
}
static std::vector<float> sfxRender(SfxMixer& m, int frames, float music = 0.f) {
    std::vector<float> b(2 * frames, music);
    m.render(b.data(), frames);
    return b;
}
static float sfxRms(const std::vector<float>& b, int ch) {
    double s = 0; int n = (int)b.size() / 2;
    for (int i = 0; i < n; ++i) s += b[2 * i + ch] * b[2 * i + ch];
    return n ? (float)std::sqrt(s / n) : 0.f;
}
static float sfxHf(const std::vector<float>& b, int ch) {   // RMS of the first difference: high-frequency energy
    double s = 0; int n = (int)b.size() / 2;
    for (int i = 1; i < n; ++i) { float d = b[2 * i + ch] - b[2 * (i - 1) + ch]; s += d * d; }
    return n > 1 ? (float)std::sqrt(s / (n - 1)) : 0.f;
}
static float sfxPeak(const std::vector<float>& b) { float p = 0.f; for (float x : b) p = std::max(p, std::fabs(x)); return p; }
static bool sfxFinite(const std::vector<float>& b) { for (float x : b) if (!std::isfinite(x)) return false; return true; }
// A 16-bit PCM WAV, channel 0, as floats (empty if missing)
static std::vector<float> readWav16(const std::string& path, float& rate) {
    std::vector<float> out; rate = 0.f;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return out;
    char id[4], wave[4]; uint32_t len = 0; int ch = 1, bits = 16;
    if (std::fread(id, 1, 4, f) != 4 || std::fread(&len, 4, 1, f) != 1 || std::fread(wave, 1, 4, f) != 4) { std::fclose(f); return out; }
    while (std::fread(id, 1, 4, f) == 4 && std::fread(&len, 4, 1, f) == 1) {
        if (!std::memcmp(id, "fmt ", 4)) {
            std::vector<uint8_t> b(len); if (std::fread(b.data(), 1, len, f) != len) break;
            uint16_t c, bp; uint32_t sr; std::memcpy(&c, &b[2], 2); std::memcpy(&sr, &b[4], 4); std::memcpy(&bp, &b[14], 2);
            ch = c; rate = (float)sr; bits = bp;
            if (len & 1) std::fseek(f, 1, SEEK_CUR);
        } else if (!std::memcmp(id, "data", 4)) {
            std::vector<int16_t> s(len / 2); size_t got = std::fread(s.data(), 2, s.size(), f);
            if (bits == 16) for (size_t i = 0; i + ch <= got; i += ch) out.push_back(s[i] / 32768.f);
            break;
        } else std::fseek(f, len + (len & 1), SEEK_CUR);
    }
    std::fclose(f);
    return out;
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
        p.floorY = L.baseFloor(pad.centre.x, pad.centre.z);
        p.update(DT, keys, L.walls.data(), (int)L.walls.size(), false, &grid);
        int ticks = 0;
        while (!p.onGround && ticks < 600) {
            p.floorY = L.baseFloor(p.position.x, p.position.z);
            p.update(DT, keys, L.walls.data(), (int)L.walls.size(), false, &grid); ++ticks;
        }
        std::printf("      pad (%.1f, %.1f, %.1f) -> lands y=%.2f after %.2fs\n", pad.centre.x, pad.centre.y, pad.centre.z, p.position.y, ticks * DT);
        if (p.position.y < pad.centre.y + 2.5f) allLand = false;
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

// A player-sized box swept along a straight walk: does it ever overlap a wall?
// Door halves are skipped when `doorsOpen` (they part as you arrive).
static bool walkClear(const LevelData& L, glm::vec3 a, glm::vec3 b, bool doorsOpen, const char* what) {
    float len = glm::length(b - a);
    int n = std::max(1, (int)(len / 0.2f));
    for (int i = 0; i <= n; ++i) {
        glm::vec3 p = glm::mix(a, b, (float)i / n);
        AABB box = boxAt(p + glm::vec3{0, 0.05f, 0}, 0.4f, 1.75f);
        for (int w = 0; w < (int)L.walls.size(); ++w) {
            if (L.walls[w].dynamic || (doorsOpen && L.isDoorWall(w))) continue;
            if (overlapsBox(box, L.walls[w].box)) {
                if (what) std::printf("      %s blocked at (%.1f %.1f %.1f) by wall %d (%.1f..%.1f, %.1f..%.1f, %.1f..%.1f)\n", what, p.x, p.y, p.z, w,
                                      L.walls[w].box.min.x, L.walls[w].box.max.x, L.walls[w].box.min.y, L.walls[w].box.max.y,
                                      L.walls[w].box.min.z, L.walls[w].box.max.z);
                return false;
            }
        }
    }
    return true;
}
static bool routeClear(const LevelData& L, const std::vector<glm::vec3>& pts, const char* what) {
    for (size_t i = 0; i + 1 < pts.size(); ++i) if (!walkClear(L, pts[i], pts[i + 1], true, what)) return false;
    return true;
}

// Drop a player into a level at p and let the boosters, doors and physics run
// for `seconds` with nobody on the keys. Returns where they ended up.
static Player ride(LevelData L, const SpatialGrid& grid, glm::vec3 p, float seconds, Uint8* keys, float* topSpeed = nullptr) {
    Player pl(p);
    pl.dynWalls = L.moverWalls.data(); pl.dynCount = (int)L.moverWalls.size();
    for (int d = 0; d < (int)L.doors.size(); ++d) L.doors[d].locked = false;
    float best = 0.f;
    for (int i = 0; i < (int)(seconds / DT); ++i) {
        L.updateDoors(DT, pl.position);
        int b = L.boosterAt(pl.position);
        pl.update(DT, keys, L.walls.data(), (int)L.walls.size(), b >= 0, &grid);
        if (b >= 0) applyBooster(L.boosters[b], pl, DT);
        best = std::max(best, glm::length(pl.velocity));
    }
    if (topSpeed) *topSpeed = best;
    return pl;
}

int main() {
    srand(7);
    LevelData L = buildLevel();
    SpatialGrid grid; grid.build(L.walls);
    Uint8 keys[SDL_NUM_SCANCODES];
    std::memset(keys, 0, sizeof(keys));

    // ---------------------------------------------------------------- arena map
    {
        CHECK(L.arenas.size() == 5, "five arenas");
        // Act I as built before the Act II split: nothing added, nothing lost
        CHECK(L.walls.size() == 256 && L.props.size() == 274 && L.neon.size() == 296 && L.shapes.size() == 0 &&
              L.floors.size() == 13 && L.doors.size() == 8 && L.pads.size() == 29 && L.movers.size() == 13,
              "buildAct1 builds exactly what buildLevel did");
        CHECK(L.corridors.size() == 4, "four corridors join them");
        bool groundOk = true, airOk = true, inBounds = true, starts = true, underCeiling = true;
        for (auto& a : L.arenas) {
            // Largest regular ground enemy (the Juggernaut) must fit at every ground spawn
            for (auto& s : allGround(a)) {
                if (overlapsWall(L, boxAt(s, statsOf(EnemyType::JUGGERNAUT).radius, statsOf(EnemyType::JUGGERNAUT).height))) {
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
        const Arena& core = L.arenas[3];
        glm::vec3 mirrored{-core.bossSpawn.x, core.bossSpawn.y, core.bossSpawn.z};
        CHECK(!overlapsWall(L, boxAt(core.bossSpawn, statsOf(EnemyType::WARDEN).radius, statsOf(EnemyType::WARDEN).height)) &&
              !overlapsWall(L, boxAt(mirrored, statsOf(EnemyType::WARDEN).radius, statsOf(EnemyType::WARDEN).height)),
              "the Warden fits at both of his spawn points");
        const Arena& sanctum = L.arenas.back();
        CHECK(std::string(sanctum.name) == "THE SANCTUM" && sanctum.waves.size() == 1 &&
              sanctum.waves[0].size() == 1 && sanctum.waves[0][0].type == EnemyType::SOVEREIGN &&
              !overlapsWall(L, boxAt(sanctum.bossSpawn, statsOf(EnemyType::SOVEREIGN).radius, statsOf(EnemyType::SOVEREIGN).height)),
              "the Sanctum holds the Sovereign alone, and he fits where he waits");
        CHECK((sanctum.zone.max.x - sanctum.zone.min.x) * (sanctum.zone.max.z - sanctum.zone.min.z) >
              2.f * (core.zone.max.x - core.zone.min.x) * (core.zone.max.z - core.zone.min.z),
              "the Sanctum is more than twice the size of any arena before it");
        CHECK(core.exitDoor >= 0 && L.doors[core.exitDoor].locked, "the Core's way on to the Sanctum starts locked");
        CHECK(groundOk, "every ground spawn (all Spire tiers too) fits a Juggernaut without touching a wall");
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
        CHECK(waves == 13, "three waves per arena, then the Sovereign");
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

    CHECK(padsLand(L, grid, keys), "every arena jump pad lands the player on something higher (>= 2.5 m up)");

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

    // Doors: two halves filling a gap in a wall; they part when you come
    // near (unless locked) and close again once you've gone
    auto doorsFill = [&](const LevelData& lv, const char* name) {
        bool ok = true;
        for (auto& d : lv.doors) {
            const AABB &a = lv.walls[d.wall].box, &b = lv.walls[d.wall2].box, &c = d.closed;
            int ax0 = d.alongX() ? 0 : 2;
            bool halves = a.min == c.min && b.max == c.max && std::fabs(a.max[ax0] - b.min[ax0]) < 1e-4f &&
                          a.max.y == c.max.y && b.min.y == c.min.y && a.max[2 - ax0] == c.max[2 - ax0] && b.min[2 - ax0] == c.min[2 - ax0];
            // Something solid either side of the door and above it: it fills its gap
            glm::vec3 m = (c.min + c.max) * 0.5f;
            int ax = d.alongX() ? 0 : 2;
            glm::vec3 j0 = m, j1 = m, top = m;
            j0[ax] = c.min[ax] - 0.25f; j1[ax] = c.max[ax] + 0.25f; top.y = c.max.y + 0.25f;
            auto solid = [&](glm::vec3 q) {
                for (int w = 0; w < (int)lv.walls.size(); ++w) {
                    if (lv.isDoorWall(w)) continue;
                    const AABB& bx = lv.walls[w].box;
                    if (q.x > bx.min.x && q.x < bx.max.x && q.y > bx.min.y && q.y < bx.max.y && q.z > bx.min.z && q.z < bx.max.z) return true;
                }
                return false;
            };
            if (!halves || !solid(j0) || !solid(j1) || !solid(top)) {
                std::printf("      %s door at (%.1f %.1f %.1f) doesn't fill its gap\n", name, m.x, m.y, m.z); ok = false; }
        }
        return ok;
    };
    {
        CHECK(doorsFill(L, "arena"), "every arena door is two halves filling a gap in a wall");
        CHECK(L.arenas[0].exitDoor >= 0 && L.doors[L.arenas[0].exitDoor].locked, "arena 1's exit starts locked");
        LevelData M = L;
        int ex = M.arenas[0].exitDoor, en = M.arenas[1].entryGate;
        glm::vec3 nearEx = (M.doors[ex].closed.min + M.doors[ex].closed.max) * 0.5f + glm::vec3{0, 0, 4.f};
        nearEx.y = 0.f;
        for (int i = 0; i < 60; ++i) M.updateDoors(DT, nearEx);
        bool lockedStays = M.doors[ex].openAmount == 0.f && overlapsBox(boxAt(nearEx - glm::vec3{0, 0, 4.f}, 0.4f, 1.8f), M.walls[M.doors[ex].wall].box);
        M.doors[ex].locked = false;
        for (int i = 0; i < 30; ++i) M.updateDoors(DT, nearEx);
        bool opens = M.doors[ex].openAmount == 1.f && walkClear(M, nearEx, nearEx - glm::vec3{0, 0, 8.f}, false, "open gate");
        for (int i = 0; i < 60; ++i) M.updateDoors(DT, {0.f, 0.f, 10.f});
        bool closes = M.doors[ex].openAmount == 0.f && M.doors[en].openAmount == 0.f;
        CHECK(lockedStays, "a locked door stays shut when you walk up to it");
        CHECK(opens, "an unlocked door parts in under half a second as you approach, and you can walk through");
        CHECK(closes, "doors close again once you've moved away");
    }

    // ---------------------------------------------------------------- floors below Y 0
    {
        LevelData B;
        B.basins.push_back({LevelBuilder::aabb(-10, 0, -10, 10, 0, 10), -60.f});
        CHECK(B.baseFloor(0.f, 0.f) == -60.f && B.baseFloor(50.f, 0.f) == 0.f && B.lowestFloor() == -60.f,
              "a basin lowers the floor inside it and nowhere else");
        Enemy e(EnemyType::HUSK, {0.f, -55.f, 0.f});
        e.floorY = -60.f;
        e.state = EnemyState::ACTIVE; e.staggerTimer = 10.f;   // stands still: just physics
        EnemyWorld w;
        for (int i = 0; i < 120; ++i) e.update(DT, w);
        CHECK(std::fabs(e.position.y + 60.f) < 0.001f && e.grounded, "an enemy lands on its floorY");
        ProjectileSystem ps; ps.floorY = -60.f;
        ps.fire({0.f, -50.f, 0.f}, {0.f, -1.f, 0.f}, 10.f, false);
        std::vector<Enemy> none;
        ps.update(0.5f, nullptr, 0, none, {1000.f, 0.f, 1000.f});
        bool alive = false; for (auto& p : ps.pool) alive |= p.alive;
        CHECK(alive, "a shot below Y 0 but above floorY keeps flying");
    }

    // ---------------------------------------------------------------- ledges below Y 0
    {
        // A Brute on a 6 m perch in Act II's basin holds it (treats the drop
        // as a wall), as it would in Act I, instead of walking off
        std::vector<Wall> walls{ Wall{ AABB{{-3.f, -61.f, -3.f}, {3.f, -54.f, 3.f}} } };
        SpatialGrid g; g.build(walls);
        Enemy e(EnemyType::BRUTE, {0.f, -54.f, 0.f});
        e.state = EnemyState::ACTIVE; e.floorY = -60.f;
        EnemyWorld w;
        w.walls = walls.data(); w.wallCount = (int)walls.size(); w.grid = &g;
        w.playerFeet = {16.f, -60.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        for (int i = 0; i < 60 * 5; ++i) e.update(DT, w);
        std::printf("      brute on a perch below Y 0 ends at (%.1f %.1f %.1f)\n", e.position.x, e.position.y, e.position.z);
        CHECK(e.position.y > -54.5f, "a ledge-aware enemy holds its perch below Y 0 too");
    }

    // ---------------------------------------------------------------- water
    {
        LevelData W;
        W.basins.push_back({LevelBuilder::aabb(-20, 0, -20, 20, 0, 20), -60.f});
        W.water.push_back({LevelBuilder::aabb(-10, -60, -10, 10, -60, 10), -57.4f});   // 2.6 m deep
        CHECK(std::fabs(W.waterDepthAt({0.f, -60.f, 0.f}) - 2.6f) < 1e-4f && W.waterDepthAt({15.f, -60.f, 0.f}) == 0.f,
              "water depth inside a volume, none outside");
        CHECK(std::fabs(W.floorWithWater(0.f, 0.f, false) - (-57.4f - LevelData::WADE_MAX)) < 1e-4f,
              "deep water holds your feet 1.5 m under the surface");
        CHECK(std::fabs(W.floorWithWater(0.f, 0.f, true) - (-57.4f - LevelData::SKIM_DEPTH)) < 1e-4f &&
              -57.4f - LevelData::SKIM_DEPTH + Player::SLIDE_EYE_H > -57.4f,
              "a slide planes on top: the camera stays above the surface");
        Player p({0.f, -50.f, 0.f});
        for (int i = 0; i < 240; ++i) { p.floorY = W.floorWithWater(p.position.x, p.position.z, p.sliding); p.update(DT, keys, nullptr, 0); }
        CHECK(p.position.y + p.eyeHeight > -57.4f && p.onGround, "dropped into deep water, you wade with your eyes above it");
    }

    // ---------------------------------------------------------------- sliding in and out of deep water
    {
        LevelData W;
        W.basins.push_back({LevelBuilder::aabb(-100, 0, -100, 100, 0, 100), -60.f});
        W.water.push_back({LevelBuilder::aabb(-100, -60, -100, 100, -60, 100), -57.4f});   // 2.6 m deep
        const float surf = -57.4f;
        Player p({0.f, -59.f, 0.f});
        Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
        for (int i = 0; i < 60; ++i) { applyWater(p, W); p.update(DT, k, nullptr, 0); }
        k[SDL_SCANCODE_W] = 1;
        for (int i = 0; i < 60; ++i) { applyWater(p, W); p.update(DT, k, nullptr, 0); }
        k[SDL_SCANCODE_LCTRL] = 1;
        float minEye = 1e9f; bool slid = false;
        for (int i = 0; i < 20; ++i) {
            applyWater(p, W); p.update(DT, k, nullptr, 0);
            slid |= p.sliding; minEye = std::min(minEye, p.camera.position.y - surf);
        }
        k[SDL_SCANCODE_LCTRL] = 0;
        int airborne = 0;
        for (int i = 0; i < 20; ++i) {
            applyWater(p, W); p.update(DT, k, nullptr, 0);
            airborne += !p.onGround; minEye = std::min(minEye, p.camera.position.y - surf);
        }
        std::printf("      deep-water slide: lowest eye %.2f m over the surface, %d ticks airborne after it\n", minEye, airborne);
        CHECK(slid && minEye > 0.f, "starting and ending a slide in deep water never puts the camera under the surface");
        CHECK(airborne == 0, "standing up out of a skim keeps you on your feet (your jump isn't lost)");
    }

    // ---------------------------------------------------------------- flyers below Y 0
    {
        Enemy hi(EnemyType::RAPTOR, {0.f, -47.f, 0.f}, -60.f), lo(EnemyType::RAPTOR, {0.f, -60.f, 0.f}, -60.f);
        Enemy act1(EnemyType::RAPTOR, {0.f, 12.f, 0.f}), act1lo(EnemyType::RAPTOR, {0.f, 0.f, 0.f});
        CHECK(std::fabs(hi.hoverY - 13.f) < 1e-4f && std::fabs(lo.hoverY - 8.f) < 1e-4f,
              "a flyer in Act II hovers at its spawn height over the Nave's floor (8 m if it spawned low)");
        CHECK(std::fabs(act1.hoverY - 12.f) < 1e-4f && std::fabs(act1lo.hoverY - 8.f) < 1e-4f, "Act I flyers hover as before");
        hi.state = EnemyState::ACTIVE;
        EnemyWorld w; w.playerFeet = {30.f, -60.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        w.bounds = LevelBuilder::aabb(-49, -60, -50, 49, -34, 50);
        float maxY = -1e9f;
        for (int i = 0; i < 60 * 2; ++i) { hi.floorY = -60.f; hi.update(DT, w); maxY = std::max(maxY, hi.position.y); }
        CHECK(maxY < -42.f, "it doesn't climb to the ceiling");
    }

    // ---------------------------------------------------------------- the flood
    {
        LevelData Fd;
        Arena a; a.name = "TANK"; a.subtitle = "";
        a.bounds = LevelBuilder::aabb(-20, -60, -20, 20, -30, 20);
        a.zone = a.bounds;
        a.shift = ArenaShift::FLOOD;
        a.floodLevels = {-59.6f, -58.f, -57.4f};
        a.groundSpawns = {{-15.f, -60.f, 0.f}, {15.f, -57.f, 0.f}};   // a low one and one on a 3 m ledge
        a.waves = {{{EnemyType::HUSK, 4}}, {{EnemyType::HUSK, 4}}, {{EnemyType::HUSK, 4}}};
        Fd.arenas.push_back(a);
        Fd.water.push_back({LevelBuilder::aabb(-20, -60, -20, 20, -60, 20), -59.6f});
        ArenaShifts sh; sh.capture(Fd);
        sh.onWaveCleared(Fd, 0, 2);
        bool started = false;
        for (int i = 0; i < 60 * 7; ++i) { sh.update(DT, Fd, 0, true); started |= sh.floodStarted; }
        CHECK(started && std::fabs(Fd.water[0].level - (-57.4f)) < 1e-3f, "the water rises to the next wave's level within ~6 s");
        WaveDirector d; d.level = &Fd; d.startArena(0); d.wave = 2;
        bool allDry = true;
        for (int k = 0; k < 20; ++k) {
            glm::vec3 sp = d.pickSpawnForTest(EnemyType::HUSK, {0.f, -57.f, 0.f});
            allDry &= Fd.waterDepthAt(sp) <= WaveDirector::DRY_DEPTH;
        }
        CHECK(allDry, "nothing spawns under more than 1.2 m of water");
        sh.reset(Fd);
        CHECK(std::fabs(Fd.water[0].level - (-59.6f)) < 1e-4f, "a retry drains the flood back to how it was built");
    }

    // ---------------------------------------------------------------- ACT II: the Drowned Nave
    {
        LevelData N = buildAct2Level();
        SpatialGrid ng; ng.build(N.walls);
        CHECK(N.arenas.size() == 5 && std::string(N.arenas[0].name) == "THE DROWNED NAVE" && std::string(N.arenas[1].name) == "THE ORRERY" &&
              std::string(N.arenas[2].name) == "THE DESCENT" && std::string(N.arenas[3].name) == "THE RELIQUARY" &&
              std::string(N.arenas[4].name) == "THE MAW" && N.corridors.size() == 4,
              "act II: the Drowned Nave, the Orrery, the Descent, the Reliquary, then the Maw, joined by corridors");
        const Arena& nave = N.arenas[0];
        bool groundOk = true, airOk = true, inB = true, under = true;
        for (auto& sp : allGround(nave)) {
            if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::JUGGERNAUT).radius, statsOf(EnemyType::JUGGERNAUT).height))) {
                std::printf("      nave ground spawn (%.1f %.1f %.1f) in a wall\n", sp.x, sp.y, sp.z); groundOk = false; }
            if (!inside(nave.bounds, sp)) inB = false;
            if (sp.y + 3.f > nave.zone.max.y) under = false;
        }
        for (auto& sp : nave.airSpawns)
            if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::RAPTOR).radius, statsOf(EnemyType::RAPTOR).height))) {
                std::printf("      nave air spawn (%.1f %.1f %.1f) in a wall\n", sp.x, sp.y, sp.z); airOk = false; }
        CHECK(groundOk && airOk && inB && under, "the Nave's spawns are clear of walls, inside it and under its ceiling");
        bool condOk = true;
        for (auto& g : nave.goals) for (auto& cp : g.points)
            if (overlapsWall(N, boxAt(cp, statsOf(EnemyType::CONDUIT).radius, statsOf(EnemyType::CONDUIT).height))) {
                std::printf("      conduit (%.1f %.1f %.1f) in a wall\n", cp.x, cp.y, cp.z); condOk = false; }
        CHECK(condOk, "the gallery conduits stand clear of walls");
        CHECK(padsLand(N, ng, keys), "every Nave jump pad lands you on something higher");
        CHECK(nave.exitDoor >= 0 && N.doors[nave.exitDoor].locked, "the way on starts sealed");
        CHECK(nave.waves.size() == 3 && nave.goals.size() == 3 && nave.goals[1].kind == WaveGoal::CONDUITS &&
              nave.shift == ArenaShift::FLOOD && nave.floodLevels.size() == 3 && nave.maxAlive == 11 &&
              std::fabs(nave.damageScale - 1.3f) < 1e-4f, "three waves, conduits in the second, a flood, 11 at once, x1.3 damage");
        const Arena& sanctum = L.arenas.back();
        CHECK((nave.zone.max.x - nave.zone.min.x) * (nave.zone.max.z - nave.zone.min.z) >
              (sanctum.zone.max.x - sanctum.zone.min.x) * (sanctum.zone.max.z - sanctum.zone.min.z),
              "the Nave is bigger than the Sanctum");
        bool apart = true;
        for (auto& a1 : L.arenas) if (a1.zone.min.z < nave.zone.max.z) apart = false;
        CHECK(apart, "Act II never overlaps Act I (ASCENT can build both)");
        // The fall: from the top of the shaft to the narthex floor
        Player p(nave.playerStart);
        int t = 0;
        for (; t < 60 * 6 && !(p.onGround && t > 5); ++t) {
            p.floorY = N.floorWithWater(p.position.x, p.position.z, false);
            p.update(DT, keys, N.walls.data(), (int)N.walls.size(), false, &ng);
        }
        std::printf("      fell %.1f m in %.2f s\n", nave.playerStart.y - p.position.y, t * DT);
        CHECK(p.onGround && p.position.y < -58.f && p.position.z > -472.f && t < 60 * 4,
              "the fall lands you in the narthex within 4 s");
        // Every flood level: the effective spawns stay wadeable
        LevelData Nf = buildAct2Level();
        bool dryOk = true;
        for (int w = 0; w < 3; ++w) {
            Nf.water[0].level = Nf.arenas[0].floodLevels[w];
            WaveDirector d; d.level = &Nf; d.startArena(0); d.wave = w;
            for (int k = 0; k < 30; ++k) {
                glm::vec3 sp = d.pickSpawnForTest(EnemyType::HUSK, Nf.arenas[0].playerStart);
                if (Nf.waterDepthAt(sp) > WaveDirector::DRY_DEPTH) dryOk = false;
            }
        }
        CHECK(dryOk, "at every flood level the Nave's spawns are out of deep water");
    }

    // ---------------------------------------------------------------- the Act II unlock
    {
        Records r; r.load();
        Records keep = r;
        r.act2Unlocked = false; r.save();
        Records a; a.load();
        CHECK(!a.act2Unlocked && !canStartAct2(a), "act II starts locked: the menu row won't start it");
        a.act2Unlocked = true; a.save();
        Records b; b.load();
        CHECK(b.act2Unlocked && canStartAct2(b), "the unlock is saved and loaded");
        keep.save();   // leave the developer's records as they were
    }

    // ---------------------------------------------------------------- ACT II: the Orrery
    {
        LevelData N = buildAct2Level();
        SpatialGrid ng; ng.build(N.walls);
        const Arena& orr = N.arenas[1];
        const glm::vec3 C{0.f, -80.f, -732.f};
        CHECK(N.baseFloor(0.f, -500.f) == -60.f && N.baseFloor(0.f, -732.f) == -140.f, "the Nave's floor at -60, the Orrery's void bottoms out at -140");
        bool groundOk = true, airOk = true, inB = true, under = true;
        for (auto& sp : allGround(orr)) {
            if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::ANCHOR).radius, statsOf(EnemyType::JUGGERNAUT).height))) {
                std::printf("      orrery ground spawn (%.1f %.1f %.1f) in a wall\n", sp.x, sp.y, sp.z); groundOk = false; }
            if (!inside(orr.bounds, sp)) inB = false;
            if (sp.y + 3.f > orr.zone.max.y) under = false;
            float r = glm::length(glm::vec2(sp.x - C.x, sp.z - C.z));
            if (r < 31.f || r > 51.f) { std::printf("      orrery ground spawn off the terrace (r %.1f)\n", r); groundOk = false; }
        }
        for (auto& sp : orr.airSpawns)
            if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::SERAPH).radius, statsOf(EnemyType::SERAPH).height))) airOk = false;
        CHECK(groundOk && airOk && inB && under, "the Orrery's spawns: ground on the terrace, air over the pit, all clear");
        CHECK(padsLand(N, ng, keys), "every pad in Act II lands you higher");
        CHECK(moversClear(N), "the rings never pass through the spokes, pillars or terrace");
        CHECK(orr.entryGate >= 0 && orr.voidY == -105.f && orr.hasRespawn && orr.shift == ArenaShift::SOLAR &&
              orr.maxAlive == 12 && std::fabs(orr.damageScale - 1.35f) < 1e-4f, "a gate behind you, a void with a way back, the sun's flare");
        CHECK(orr.waves.size() == 3 && orr.goals.size() == 3 && orr.goals[1].kind == WaveGoal::HOLD && orr.goals[1].mover >= 0,
              "wave 2 is a HOLD that rides a ring");
        CHECK(orr.exitDoor >= 0 && N.doors[orr.exitDoor].locked, "the Orrery's north arch, the way on down, starts locked");
        // The rings turn as rings
        auto ringOf = [&](int mi) { return mi < 26 ? 0 : 1; };
        bool rings = N.movers.size() > 42 + 4;   // the Orrery's rings, then the Descent's cage (then the Reliquary's relics)
        LevelData R = N;
        for (float t : {0.f, 7.3f, 31.f}) {
            R.updateMovers(t);
            for (int mi = 0; mi < 42; ++mi) {
                const AABB& b = R.walls[R.movers[mi].wall].box;
                glm::vec2 c{(b.min.x + b.max.x) * 0.5f - C.x, (b.min.z + b.max.z) * 0.5f - C.z};
                float want = ringOf(mi) == 0 ? 22.f : 12.f;
                if (std::fabs(glm::length(c) - want) > 0.01f) rings = false;
            }
        }
        CHECK(rings, "26 outer and 16 inner segments, each always on its ring's circle");
        // A player standing on a ring is carried round and stays on
        auto rideFor = [&](LevelData& L, Player& p, float t0, float secs, std::function<void(int, Player&)> act) {
            SpatialGrid g; g.build(L.walls);
            p.dynWalls = L.moverWalls.data(); p.dynCount = (int)L.moverWalls.size();
            float t = t0;
            L.updateMovers(t);
            for (int i = 0; i < (int)(secs * 60); ++i) {
                int ride = L.moverOfWall(p.groundWall);
                t += DT; L.updateMovers(t);
                if (ride >= 0) p.position += L.movers[ride].delta;
                act(i, p);
                p.floorY = L.baseFloor(p.position.x, p.position.z);
                Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
                p.update(DT, k, L.walls.data(), (int)L.walls.size(), false, &g);
                if (p.position.y < -105.f) return false;
            }
            return true;
        };
        {
            LevelData L = N; L.updateMovers(0.f);
            const AABB& b = L.walls[L.movers[0].wall].box;
            Player p({(b.min.x + b.max.x) * 0.5f, b.max.y, (b.min.z + b.max.z) * 0.5f});
            bool stayed = rideFor(L, p, 0.f, 10.f, [](int, Player&) {});
            CHECK(stayed && L.moverOfWall(p.groundWall) >= 0 && std::fabs(p.position.y + 80.f) < 0.05f,
                  "standing on the outer ring for 10 s, it carries you round and you stay on");
        }
        // Running jumps: spoke tip to the outer ring, and outer ring to the inner one (with the double jump)
        auto runJump = [&](glm::vec3 start, float yaw, float jumpAtDist, bool doubleJump, float t0) {
            LevelData L = N;
            Player p(start); p.camera.yaw = yaw;
            glm::vec2 s2{start.x - C.x, start.z - C.z};
            float startR = glm::length(s2);
            bool jumped = false, doubled = false, landedOnMover = false;
            SpatialGrid g; g.build(L.walls);
            p.dynWalls = L.moverWalls.data(); p.dynCount = (int)L.moverWalls.size();
            float t = t0; L.updateMovers(t);
            for (int i = 0; i < 60 * 4; ++i) {
                int ride = L.moverOfWall(p.groundWall);
                t += DT; L.updateMovers(t);
                if (ride >= 0 && jumped) { landedOnMover = true; break; }
                if (ride >= 0) p.position += L.movers[ride].delta;
                Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
                k[SDL_SCANCODE_W] = 1;
                float r = glm::length(glm::vec2(p.position.x - C.x, p.position.z - C.z));
                if (!jumped && p.onGround && startR - r >= jumpAtDist) { k[SDL_SCANCODE_SPACE] = 1; jumped = true; }
                if (jumped && doubleJump && !doubled && !p.onGround && p.velocity.y < 0.f) { p.velocity.y = p.jumpForce; doubled = true; }
                p.floorY = L.baseFloor(p.position.x, p.position.z);
                p.update(DT, k, L.walls.data(), (int)L.walls.size(), false, &g);
                if (jumped && p.onGround && L.moverOfWall(p.groundWall) >= 0) { landedOnMover = true; break; }
                if (p.position.y < -100.f) break;
            }
            return landedOnMover;
        };
        int spokeOk = 0, innerOk = 0;
        for (int k = 0; k < 8; ++k) {
            float t0 = k * 62.83f / 8.f;
            // east spoke: tip at R 25, run west from R 29.5 and jump right at the tip
            if (runJump(C + glm::vec3{29.5f, 0.f, 0.f}, 180.f, 29.5f - 25.2f, false, t0)) ++spokeOk;
            // from the outer ring's inner edge (R 19) toward the sun, jump and double jump
            float ti = k * 34.27f / 8.f;
            if (runJump(C + glm::vec3{20.5f, 0.f, 0.f}, 180.f, 20.5f - 19.3f, true, ti)) ++innerOk;
        }
        std::printf("      spoke -> outer ring %d/8, outer -> inner ring %d/8\n", spokeOk, innerOk);
        CHECK(spokeOk == 8, "from a spoke's tip a running jump always lands on the outer ring");
        CHECK(innerOk >= 6, "from the outer ring a jump and double jump reaches the inner ring");
        // The flare reaches the open terrace whatever the rings are doing (they don't shade it)
        {
            ArenaShifts fs; fs.capture(N);
            fs.flareClock = 2.f;   // burning
            int hits = 0, tries = 0;
            LevelData L = N;
            for (float t = 0.f; t < 60.f; t += 1.7f) {
                L.updateMovers(t);
                for (float r : {32.f, 40.f, 50.f}) {
                    glm::vec3 p = C + glm::vec3{r, 0.f, 0.f};
                    fs.flareAngle = 0.f;
                    ++tries; hits += fs.flareHits(L, 1, p);
                }
            }
            std::printf("      flare on the open terrace: %d/%d\n", hits, tries);
            CHECK(hits == tries, "the burning flare reaches anyone in the open on the terrace");
        }
        // No floor over the void: nothing solid at the terrace's height inside the pit's edge
        {
            bool clear = true, floored = true;
            for (int k = 0; k < 72; ++k) {
                float ang = k * 6.2831853f / 72.f;
                for (float r : {12.f, 20.f, 26.f, 29.6f}) {
                    if (r > 24.f && std::fabs(std::sin(ang)) < 0.06f) continue;   // the E/W spokes
                    if (r > 24.f && std::fabs(std::cos(ang)) < 0.06f) continue;   // the N/S spokes
                    glm::vec3 p = C + glm::vec3{std::cos(ang) * r, -0.5f, std::sin(ang) * r};
                    for (auto& w : N.walls) {
                        if (w.dynamic) continue;
                        const AABB& b = w.box;
                        if (p.x > b.min.x && p.x < b.max.x && p.z > b.min.z && p.z < b.max.z && p.y > b.min.y && p.y < b.max.y) {
                            std::printf("      solid over the pit at r %.1f, angle %.0f\n", r, glm::degrees(ang)); clear = false; break;
                        }
                    }
                }
                glm::vec3 q = C + glm::vec3{std::cos(ang) * 33.f, -0.5f, std::sin(ang) * 33.f};
                bool any = false;
                for (auto& w : N.walls) {
                    const AABB& b = w.box;
                    if (q.x > b.min.x && q.x < b.max.x && q.z > b.min.z && q.z < b.max.z && q.y > b.min.y && q.y < b.max.y) any = true;
                }
                if (!any) { std::printf("      no floor at r 33, angle %.0f\n", glm::degrees(ang)); floored = false; }
            }
            CHECK(clear, "no floor (seen or hidden) reaches over the pit");
            CHECK(floored, "the terrace is solid all the way round");
        }
        // Walk it: from the Nave's passage north, drop down the shaft, through the gate onto the terrace
        {
            LevelData L = N;
            L.setDoorInstant(L.arenas[0].exitDoor, true);
            L.setDoorInstant(L.arenas[1].entryGate, true);
            SpatialGrid g; g.build(L.walls);
            Player p({0.f, -57.f, -655.f}); p.camera.yaw = -90.f;
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k)); k[SDL_SCANCODE_W] = 1;
            float lowest = 0.f;
            for (int i = 0; i < 60 * 8; ++i) {
                p.floorY = L.baseFloor(p.position.x, p.position.z);
                p.update(DT, k, L.walls.data(), (int)L.walls.size(), false, &g);
                lowest = std::min(lowest, p.position.y);
            }
            std::printf("      walked to (%.1f %.1f %.1f), lowest %.1f\n", p.position.x, p.position.y, p.position.z, lowest);
            CHECK(L.arenaAt(p.position) == 1 && std::fabs(p.position.y + 80.f) < 0.05f && lowest > -80.5f,
                  "the way down: the passage, the shaft, the gate, and you're on the Orrery's terrace");
        }
    }

    // ---------------------------------------------------------------- ACT II: the Descent
    {
        LevelData N = buildAct2Level();
        const Arena& D = N.arenas[2];
        const glm::vec3 C{0.f, 0.f, -840.f};
        CHECK(D.space == ReverbSpace::SHAFT && D.waves.size() == 4 && D.maxAlive == 12 && std::fabs(D.damageScale - 1.4f) < 1e-4f,
              "the Descent: a shaft's reverb, three waves and the boss");
        CHECK(N.lift.movers.size() == 4 && N.lift.stops.size() == 5 && N.lift.stops[0] == -80.f && N.lift.stops[4] == -240.f,
              "the cage: four boxes (a 12-sided platform), five stops from -80 to -240");
        CHECK(N.anchors.size() == 6 && N.anchorsAlive() == 6, "six chain anchors in the pit");
        bool anchorsHigh = true;
        for (auto& a : N.anchors) anchorsHigh &= a.pos.y >= -232.f && a.pos.y <= -228.f + 0.01f &&
                                                glm::length(glm::vec2{a.pos.x - C.x, a.pos.z - C.z}) > 30.f;
        CHECK(anchorsHigh, "the anchors hang 8-12 m up the pit wall");
        // Spawns: each wave's ground points clear of walls and inside; air points clear
        bool ok = true;
        for (int w = 0; w < 3; ++w) {
            for (auto& sp : D.waveGround[w]) {
                if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::JUGGERNAUT).radius, statsOf(EnemyType::JUGGERNAUT).height))) {
                    std::printf("      descent wave %d ground spawn (%.1f %.1f %.1f) in a wall\n", w, sp.x, sp.y, sp.z); ok = false; }
                if (!inside(D.bounds, sp)) { std::printf("      descent spawn outside bounds\n"); ok = false; }
            }
            for (auto& sp : D.waveAir[w])
                if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::RAPTOR).radius, statsOf(EnemyType::RAPTOR).height))) {
                    std::printf("      descent wave %d air spawn in a wall\n", w); ok = false; }
        }
        for (auto& cp : D.goals[1].points)
            if (overlapsWall(N, boxAt(cp, statsOf(EnemyType::CONDUIT).radius, statsOf(EnemyType::CONDUIT).height))) ok = false;
        CHECK(ok, "the Descent's spawns and clamps stand clear of walls, inside the shaft");
        CHECK(D.goals[1].kind == WaveGoal::CONDUITS && D.goals[1].points.size() == 4, "stop 2: release the four clamps");
        // Every stop's ground spawns are at that stop's height (gallery or cage)
        bool levels = true;
        for (int w = 0; w < 3; ++w) for (auto& sp : D.waveGround[w]) levels &= std::fabs(sp.y - N.lift.stops[w + 1]) < 0.05f;
        CHECK(levels, "each wave spawns on its own stop's floor");
        CHECK(D.exitDoor >= 0 && N.doors[D.exitDoor].locked && N.doors[D.exitDoor].closed.max.z < -873.f,
              "the way on, a door in the pit's south wall, starts locked");
        CHECK(N.arenas[1].exitDoor >= 0, "the Orrery has a way on (its north arch)");
        // groundAt sees the cage
        N.lift.reset(); N.lift.update(0.f, N); N.updateMovers(0.f);
        CHECK(std::fabs(N.groundAt(0.f, -840.f, -79.f) - (-80.f)) < 1e-3f, "something standing on the cage stands on it (movers count as ground)");
        // Anchors: damage breaks once
        LevelData M = buildAct2Level();
        bool first = M.damageAnchor(0, 300.f), second = M.damageAnchor(0, 300.f), third = M.damageAnchor(0, 300.f);
        CHECK(!first && second && !third && M.anchorsAlive() == 5, "an anchor breaks once, on the hit that takes it past 400");
        glm::vec3 a1 = M.anchors[1].pos;
        glm::vec3 from = glm::vec3{0.f, -236.f, -840.f};
        int hit = M.anchorAlong(from, glm::normalize(a1 - from), 60.f);
        CHECK(hit == 1, "a shot at an anchor finds it");
    }

    // ---------------------------------------------------------------- the Descent's ride
    {
        LevelData N = buildAct2Level();
        ArenaShifts sh; sh.capture(N);
        CHECK(N.arenas[2].shift == ArenaShift::DESCENT, "the Descent's shift is the ride");
        N.lift.request(3); N.lift.start();
        for (int i = 0; i < 60 * 3; ++i) sh.update(DT, N, 2, true);
        bool moving = N.lift.riding();
        sh.reset(N);
        CHECK(moving && N.lift.at == 0 && !N.lift.busy(), "a retry stops the ride and puts the cage back at the top");
        sh.onWaveCleared(N, 2, 1);
        CHECK(N.lift.pending == 2, "clearing a wave asks for the next stop (it waits for the player to board)");
        CHECK(N.onLift(N.movers[N.lift.movers[0]].wall) && !N.onLift(0), "the game can tell when you're standing on the cage");
        // The cage never passes through a static wall on the way down, and is flush with every stop's bridges
        N.lift.reset(); N.lift.request(4); N.lift.start();
        float clock = 0.f; bool clear = true;
        SpatialGrid g; g.build(N.walls);
        for (int i = 0; i < 60 * 9; ++i) {
            clock += DT; N.lift.update(DT, N); N.updateMovers(clock);
            for (int m : N.lift.movers) {
                AABB b = N.walls[N.movers[m].wall].box;
                b.min += glm::vec3{0.02f}; b.max -= glm::vec3{0.02f};
                for (size_t w = 0; w < N.walls.size(); ++w)
                    if (!N.walls[w].dynamic && overlapsBox(b, N.walls[w].box)) { clear = false; }
            }
        }
        CHECK(clear, "the cage rides all the way down without touching a wall");
        bool flush = true;
        for (int s = 1; s < 5; ++s) {
            N.lift.reset(); N.lift.request(s); N.lift.start();
            for (int i = 0; i < 60 * 9; ++i) { clock += DT; N.lift.update(DT, N); N.updateMovers(clock); }
            float top = N.walls[N.movers[N.lift.movers[1]].wall].box.max.y;
            float bridge = N.groundAt(12.6f, -840.f, top + 0.1f);
            flush &= std::fabs(top - N.lift.stops[s]) < 0.02f && std::fabs(bridge - top) < 0.02f;
        }
        CHECK(flush, "at every stop the cage sits flush with the floor round it");
        // A player standing on the cage for a whole ride is carried and stays on
        N.lift.reset(); N.lift.update(0.f, N); N.updateMovers(clock);
        Player p({3.f, -80.f, -838.f});
        p.dynWalls = N.moverWalls.data(); p.dynCount = (int)N.moverWalls.size();
        SpatialGrid pg; pg.build(N.walls);
        N.lift.request(1); N.lift.start();
        bool stayed = true;
        for (int i = 0; i < 60 * 9; ++i) {
            int rm = N.moverOfWall(p.groundWall);
            clock += DT; N.lift.update(DT, N); N.updateMovers(clock);
            if (rm >= 0) p.position += N.movers[rm].delta;
            p.floorY = N.baseFloor(p.position.x, p.position.z);
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
            p.update(DT, k, N.walls.data(), (int)N.walls.size(), false, &pg);
            stayed &= p.position.y > N.lift.y() - 1.0f && p.position.y < N.lift.y() + 1.5f;
        }
        CHECK(stayed && std::fabs(p.position.y - (-120.f)) < 0.1f, "standing on the cage, you ride it down and stay on");
    }

    // ---------------------------------------------------------------- a simulated ACT II run
    {
        LevelData N = buildAct2Level();
        WaveDirector d; d.level = &N;
        d.startArena(0);
        glm::vec3 player = N.arenas[0].playerStart; player.y = -60.f;
        std::vector<float> alive;
        int seraphs = 0, anchors = 0, haloed = 0, twinned = 0, enragedSpawns = 0;
        float clock = 0.f;
        for (int tick = 0; tick < 60 * 60 * 40 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
            clock += DT; N.lift.update(DT, N); N.updateMovers(clock);
            if (d.phase == WaveDirector::Phase::CLEARED) player = N.arenas[d.arena + 1].playerStart;   // down the corridor
            else if (d.goal().kind == WaveGoal::HOLD) player = d.goalPos() + glm::vec3{0, 0.05f, 0};   // onto the ring
            if (d.arena == 3 && d.phase != WaveDirector::Phase::CLEARED) {   // the Reliquary: the relics drift into the next wave's arrangement first
                int want = std::min(d.wave + (d.phase == WaveDirector::Phase::BREAK ? 1 : 0), 3);
                if (N.formation.at != want && !N.formation.gliding()) N.formation.glideTo(want);
                N.formation.update(DT, N);
                d.hold = N.formation.gliding();
                player = N.formation.top(0) + glm::vec3{0, 0.05f, 0};
            }
            if (d.arena == 2 && d.phase != WaveDirector::Phase::CLEARED) {   // the Descent: ride to each wave's stop first
                int want = std::min(d.wave + (d.phase == WaveDirector::Phase::BREAK ? 2 : 1), 4);
                if (N.lift.at != want && !N.lift.busy()) { N.lift.request(want); N.lift.start(); }
                d.hold = N.lift.busy();
                player = glm::vec3{0.f, N.lift.y(), -840.f};
            }
            std::vector<SpawnRequest> out;
            d.update(DT, (int)alive.size(), player, out);
            for (auto& r : out) {
                alive.push_back(3.f);
                seraphs += r.type == EnemyType::SERAPH; anchors += r.type == EnemyType::ANCHOR;
                haloed += r.hollow == Hollow::HALOED; twinned += r.hollow == Hollow::TWINNED;
                enragedSpawns += r.hollow == Hollow::ENRAGED;
            }
            for (auto& t : alive) t -= DT;
            for (auto& t : alive)
                if (t <= 0.f && d.conduitsLeft() > 0) d.onConduitDestroyed(d.goal().points[0]);
            alive.erase(std::remove_if(alive.begin(), alive.end(), [](float t) { return t <= 0.f; }), alive.end());
            for (auto& ev : d.events) if (ev.kind == DirectorEvent::GOAL_DONE) alive.clear();
            d.events.clear();
        }
        std::printf("      act II run ended: arena %d wave %d phase %d\n", d.arena, d.wave, (int)d.phase);
        CHECK(d.phase == WaveDirector::Phase::VICTORY && d.arena == 4,
              "a simulated ACT II run clears the Nave and the Orrery, rides the Descent, kills the Penitent, clears the Reliquary and kills the Leviathan");
        std::printf("      nave run: %d seraphs, %d anchors, %d haloed, %d twinned, %d enraged\n", seraphs, anchors, haloed, twinned, enragedSpawns);
        CHECK(seraphs >= 3 && anchors >= 1 && haloed >= 3 && twinned >= 2 && enragedSpawns >= 1,
              "the Nave's waves bring Seraphs, an Anchor and every variant");
        CHECK(MUSIC_TRACKS >= 9 && std::string(musicTrack(5).name) == "NAVE" && std::string(musicTrack(6).name) == "ORRERY" &&
              std::string(musicTrack(7).name) == "DESCENT", "the Nave, the Orrery and the Descent have their own tracks");
    }

    // ---------------------------------------------------------------- Hollowed variants: data
    {
        CHECK(WaveEntry(EnemyType::HUSK, 3).hollow(Hollow::HALOED).variant == Hollow::HALOED &&
              WaveEntry(EnemyType::WARDEN, 1).hollow(Hollow::HALOED).variant == Hollow::NONE &&
              WaveEntry(EnemyType::CONDUIT, 1).hollow(Hollow::TWINNED).variant == Hollow::NONE &&
              WaveEntry(EnemyType::CONDUCTOR, 1).hollow(Hollow::ENRAGED).variant == Hollow::NONE,
              "bosses, conduits and conductors never take a variant");
        Enemy h(EnemyType::HUSK, {0, 0, 0}); h.setHollow(Hollow::HALOED);
        Enemy w(EnemyType::WARDEN, {0, 0, 0}); w.setHollow(Hollow::HALOED);
        CHECK(h.hollow == Hollow::HALOED && h.halo && w.hollow == Hollow::NONE && !w.halo, "a Haloed enemy spawns with its halo up");
        LevelData V; Arena a; a.name = "V"; a.subtitle = "";
        a.bounds = a.zone = LevelBuilder::aabb(-30, 0, -30, 30, 10, 30);
        a.groundSpawns = {{-20, 0, 0}, {20, 0, 0}, {0, 0, 20}};
        a.waves = {{WaveEntry(EnemyType::HUSK, 2), WaveEntry(EnemyType::HUSK, 2).hollow(Hollow::HALOED),
                    WaveEntry(EnemyType::SHIELDBEARER, 1).with({EnemyType::HUSK}).hollow(Hollow::ENRAGED)}};
        V.arenas.push_back(a);
        WaveDirector d; d.level = &V; d.startArena(0);
        std::vector<SpawnRequest> out;
        for (int i = 0; i < 60 * 30; ++i) d.update(DT, 0, {0, 0, 0}, out);
        int haloed = 0, enraged = 0, escortsPlain = 1;
        for (size_t k = 0; k < out.size(); ++k) {
            haloed += out[k].hollow == Hollow::HALOED;
            enraged += out[k].hollow == Hollow::ENRAGED;
            if (out[k].type == EnemyType::SHIELDBEARER && k + 1 < out.size() && out[k + 1].hollow != Hollow::NONE) escortsPlain = 0;
        }
        int introPlain = 0, introHaloHusk = 0;
        for (auto& ev : d.events) if (ev.kind == DirectorEvent::NEW_TYPE) {
            introPlain += ev.value == (int)EnemyType::HUSK;
            introHaloHusk += ev.value == ((int)EnemyType::HUSK | ((int)Hollow::HALOED << 8));
        }
        CHECK(haloed == 2 && enraged == 1 && escortsPlain, "spawn requests carry the variant; a squad's escort stays plain");
        CHECK(introPlain == 1 && introHaloHusk == 1, "a variant is introduced once, on top of its plain type");
    }

    // ---------------------------------------------------------------- Enraged
    {
        EnemyWorld w; w.playerFeet = {30.f, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        auto run = [&](Hollow h) {
            Enemy r(EnemyType::RIPPER, {0, 0, 0}); r.setHollow(h); r.state = EnemyState::ACTIVE;
            for (int i = 0; i < 30; ++i) r.update(DT, w);
            return r.position.x;
        };
        float plain = run(Hollow::NONE), fast = run(Hollow::ENRAGED);
        auto windup = [&](Hollow h) {
            Enemy b(EnemyType::BRUTE, {0, 0, 0}); b.setHollow(h); b.state = EnemyState::ACTIVE;
            EnemyWorld near = w; near.playerFeet = {4.f, 0.f, 0.f}; near.playerEye = near.playerFeet + glm::vec3{0, 1.7f, 0};
            for (int i = 0; i < 600 && b.telegraphDuration <= 0.f; ++i) b.update(DT, near);
            return b.telegraphDuration;
        };
        float wp = windup(Hollow::NONE), we = windup(Hollow::ENRAGED);
        std::printf("      ripper 0.5 s: plain %.2f m, enraged %.2f m; brute wind-up %.2f vs %.2f\n", plain, fast, wp, we);
        CHECK(fast > plain * 1.2f, "an Enraged enemy moves faster");
        CHECK(we > 0.f && std::fabs(we / wp - 0.65f) < 0.02f, "an Enraged enemy winds up in 65% of the time");
        Enemy e(EnemyType::HUSK, {0, 0, 0}); e.setHollow(Hollow::ENRAGED);
        CHECK(std::fabs(e.damageMult() - 1.25f) < 1e-4f, "an Enraged enemy hits 25% harder");
    }

    // ---------------------------------------------------------------- Haloed
    {
        Enemy e(EnemyType::HUSK, {0, 0, 0}); e.setHollow(Hollow::HALOED); e.state = EnemyState::ACTIVE;
        CHECK(std::fabs(e.incomingMult() - 0.1f) < 1e-4f, "a halo cuts damage to a tenth");
        e.breakHalo();
        CHECK(!e.halo && e.staggered() && std::fabs(e.incomingMult() - 2.f) < 1e-4f, "broken: staggered, and it takes double");
        EnemyWorld w; w.playerFeet = {20, 0, 0}; w.playerEye = {20, 1.7f, 0};
        for (int i = 0; i < 60 * 2 + 3; ++i) e.update(DT, w);
        CHECK(std::fabs(e.incomingMult() - 1.f) < 1e-4f, "the double-damage window ends after 2 s");
        Enemy plain(EnemyType::HUSK, {0, 0, 0});
        CHECK(plain.incomingMult() == 1.f, "a plain enemy takes normal damage");
        ProjectileSystem ps;
        Projectile* pr = ps.fire({0, 1, 0}, {1, 0, 0}, 10.f, false);
        CHECK(pr && pr->alive && pr->owner == -1 && pr->parryDamage == 0.f, "fire hands back the shot so its owner can be set");
    }

    // ---------------------------------------------------------------- Twinned
    {
        Enemy p(EnemyType::BRUTE, {5, 0, 5}); p.setHollow(Hollow::TWINNED); p.yaw = 0.3f;
        CHECK(p.splitsOnDeath(), "a Twinned enemy splits when it dies");
        auto tw = twinsOf(p);
        bool ok = tw.size() == 2;
        for (auto& t : tw)
            ok &= t.type == EnemyType::BRUTE && t.hollow == Hollow::NONE && !t.splitsOnDeath() &&
                  std::fabs(t.maxHealth - p.maxHealth * 0.35f) < 1e-3f && t.health == t.maxHealth &&
                  std::fabs(t.scale - 0.75f) < 1e-4f && t.targetable();
        ok &= tw.size() == 2 && glm::length(tw[0].position - tw[1].position) > 2.f;
        CHECK(ok, "into two plain copies at 35% health, three-quarter size, apart, that don't split again");
        CHECK(std::fabs(tw[0].radius() - p.radius() * 0.75f) < 1e-4f, "a copy's hitbox is smaller too");
        std::vector<BoxInstance> big, small;
        Enemy pb = p; pb.spawnTimer = 0.f;
        buildEnemy(pb, 0.f, big); buildEnemy(tw[0], 0.f, small);
        float hb = 0.f, hs = 0.f;
        for (auto& b : big) hb = std::max(hb, b.model[3].y);
        for (auto& b : small) hs = std::max(hs, b.model[3].y);
        CHECK(hs < hb * 0.85f, "and drawn smaller");
    }

    // ---------------------------------------------------------------- the Seraph
    {
        CHECK((int)EnemyType::SERAPH == 12 && statsOf(EnemyType::SERAPH).flying, "the Seraph flies (dev spawn 12)");
        auto sweep = [&](glm::vec3 playerVel, bool wall, int& hitTicks, int& beamTicks, float& startGap, float& maxStep, bool& charged) {
            std::vector<Wall> walls;
            // low enough that it sees your head over it, high enough to stop a beam at your feet
            if (wall) walls.push_back(Wall{LevelBuilder::aabb(17, 0, -6, 18, 2.5f, 6)});
            SpatialGrid g; g.build(walls);
            Enemy s(EnemyType::SERAPH, {0, 12, 0}); s.state = EnemyState::ACTIVE; s.attackTimer = 99.f;
            EnemyWorld w; w.walls = walls.empty() ? nullptr : walls.data(); w.wallCount = (int)walls.size(); w.grid = &g;
            glm::vec3 feet{20.f, 0.f, 0.f};
            hitTicks = beamTicks = 0; startGap = -1.f; maxStep = 0.f; charged = false;
            glm::vec3 lastTo{0.f}; bool had = false;
            for (int i = 0; i < 60 * 6; ++i) {
                feet += playerVel * DT;
                w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0}; w.playerVel = playerVel;
                s.update(DT, w);
                charged |= s.attack == AttackKind::BEAM && s.telegraphTimer > 0.f;
                if (!s.ev.beamOn) { had = false; continue; }
                ++beamTicks;
                glm::vec3 pt = s.beamPoint;
                if (startGap < 0.f) startGap = glm::length(glm::vec2(pt.x - feet.x, pt.z - feet.z));
                if (had) maxStep = std::max(maxStep, glm::length(pt - lastTo));
                lastTo = pt; had = true;
                AABB pb{feet + glm::vec3{-0.4f, 0, -0.4f}, feet + glm::vec3{0.4f, 1.8f, 0.4f}};
                hitTicks += segmentHitsBox(s.ev.beamFrom, s.ev.beamTo, pb);
            }
        };
        int hit, beam; float gap, step; bool charged;
        sweep({0, 0, 0}, false, hit, beam, gap, step, charged);
        std::printf("      seraph vs still player: beam %d ticks, hit %d, start %.1f m off, max step %.3f m\n", beam, hit, gap, step);
        CHECK(charged && beam > 60, "a Seraph charges, then sweeps its beam");
        CHECK(gap >= 4.f && step <= 8.f * DT + 1e-3f, "the beam starts metres to the side and turns no faster than 8 m/s");
        CHECK(hit > beam / 2, "standing still, you're caught");
        sweep({0, 0, 7.f}, false, hit, beam, gap, step, charged);
        std::printf("      seraph vs strafing player: hit %d of %d\n", hit, beam);
        CHECK(hit < beam / 5, "strafing at walking speed, you're grazed at most");
        sweep({0, 0, 0}, true, hit, beam, gap, step, charged);
        std::printf("      seraph behind a wall: hit %d of %d\n", hit, beam);
        CHECK(beam > 0 && hit == 0, "a wall between you blocks the beam");
        Enemy c(EnemyType::SERAPH, {0, 12, 0}); c.state = EnemyState::ACTIVE; c.attackTimer = 99.f;
        EnemyWorld w; w.playerFeet = {20, 0, 0}; w.playerEye = {20, 1.7f, 0};
        for (int i = 0; i < 30 && !(c.attack == AttackKind::BEAM && c.telegraphTimer > 0.f); ++i) c.update(DT, w);
        bool cancelled = c.onBeamHit(45.f, false);
        bool beamed = false;
        for (int i = 0; i < 90; ++i) { c.update(DT, w); beamed |= c.ev.beamOn; }
        CHECK(cancelled && !beamed, "a big hit during the charge cancels the beam");
        c.alive = false; c.update(DT, w);
        CHECK(!c.ev.beamOn, "a dead Seraph's beam is gone");
    }

    // ---------------------------------------------------------------- the Anchor
    {
        CHECK((int)EnemyType::ANCHOR == 13 && !statsOf(EnemyType::ANCHOR).flying, "the Anchor walks (dev spawn 13)");
        Enemy a(EnemyType::ANCHOR, {0, 0, 0}); a.state = EnemyState::ACTIVE;
        CHECK(inAnchorField(a, {9.5f, 0, 0}) && !inAnchorField(a, {10.5f, 0, 0}) &&
              inAnchorField(a, {0, 2.9f, 5}) && !inAnchorField(a, {0, 3.2f, 5}), "its field: 10 m round, 6 m tall");
        Enemy r(EnemyType::ANCHOR, {0, 0, 0}); r.state = EnemyState::ACTIVE; r.setHollow(Hollow::ENRAGED);
        CHECK(inAnchorField(r, {12.5f, 0, 0}) && std::fabs(r.fieldRadius() - 13.f) < 1e-4f, "an Enraged Anchor's field is 13 m");
        a.alive = false;
        CHECK(!inAnchorField(a, {1, 0, 0}), "a dead Anchor's field is gone");
        Enemy b(EnemyType::ANCHOR, {0, 0, 0}); b.state = EnemyState::ACTIVE;
        EnemyWorld w; w.playerFeet = {20, 0, 0}; w.playerEye = {20, 1.7f, 0};
        int volleys = 0, shots = 0; float parry = 0.f;
        for (int i = 0; i < 60 * 12; ++i) { b.update(DT, w); if (b.ev.shots) { ++volleys; shots = b.ev.shots; parry = b.ev.shotParry; } }
        float dist = glm::length(glm::vec2(20.f - b.position.x, -b.position.z));
        std::printf("      anchor holds at %.1f m, %d volleys of %d\n", dist, volleys, shots);
        CHECK(dist > 7.f && dist < 9.5f, "an Anchor walks in and holds about 8 m away");
        CHECK(volleys >= 2 && shots == 3 && std::fabs(parry - 120.f) < 1e-4f, "it fires volleys of three orbs that parry back for 120");
        Enemy hb(EnemyType::ANCHOR, {0, 0, 0}); hb.spawnTimer = 0.f;
        rig::HumanoidLook L = rig::humanoidDims(EnemyType::ANCHOR);
        float neck = L.legLen + L.pelvisH + L.torsoH;
        AABB head; bool has = headBox(hb, head);
        glm::vec3 eye{0.f, 1.7f, 12.f};
        CHECK(has && rayBoxHit(eye, glm::normalize(glm::vec3{0, neck + L.headS * 0.5f, 0} - eye), head) > 0.f,
              "shooting an Anchor's head is a headshot");
        Enemy sh(EnemyType::SERAPH, {0, 12, 0}); sh.spawnTimer = 0.f;
        AABB sHead; bool sHas = headBox(sh, sHead);
        CHECK(sHas && sHead.min.y > 12.9f, "a Seraph's head is its ring, up top");
        bool follows = true;
        for (float tm : {0.f, 0.4f, 1.3f, 2.2f}) for (float ph : {0.f, 2.f, 5.f}) {
            Enemy sp(EnemyType::SERAPH, {0, 12, 0}); sp.spawnTimer = 0.f; sp.animPhase = ph;
            std::vector<BoxInstance> parts; buildEnemy(sp, tm, parts);
            float top = -1e9f; for (auto& b : parts) top = std::max(top, b.model[3].y);   // the ring's top piece
            AABB hbx; headBox(sp, hbx);
            if (top < hbx.min.y || top > hbx.max.y) follows = false;
        }
        CHECK(follows, "the Seraph's head box follows its drawn head as it bobs");
    }

    // ---------------------------------------------------------------- ENDLESS stays as it was
    {
        bool clean = true;
        EndlessWaves g; g.begin(99u, {}, true);
        for (int n = 0; n < 40; ++n)
            for (auto& e : g.wave(n).first) {
                if (e.type == EnemyType::SERAPH || e.type == EnemyType::ANCHOR || e.variant != Hollow::NONE) clean = false;
                for (auto t : e.escort) if (t == EnemyType::SERAPH || t == EnemyType::ANCHOR) clean = false;
            }
        CHECK(clean, "ENDLESS never brings a Seraph, an Anchor or a Hollowed enemy");
    }

    // ---------------------------------------------------------------- HOLD circles that ride a mover
    {
        LevelData M; LevelBuilder MB{M};
        MB.mover({0.f, 2.f, 0.f}, {2.5f, 0.5f, 2.5f}, Mover::Path::ORBIT, {10.f, 0.f, 0.f}, {0.f, 0.f, 10.f}, 20.f, 0.f, {1, 1, 1});
        Arena a; a.name = "RIDE"; a.subtitle = "";
        a.bounds = a.zone = LevelBuilder::aabb(-30, 0, -30, 30, 10, 30);
        a.groundSpawns = {{-25, 0, -25}, {25, 0, 25}};
        a.waves = {{{EnemyType::HUSK, 1}}};
        a.goals = {WaveGoal::hold("HOLD", {0, 0, 0}, 2.5f, 3.f).onMover(0)};
        M.arenas.push_back(a);
        WaveDirector d; d.level = &M; d.startArena(0);
        bool follows = true;
        for (float t : {0.f, 5.f, 12.f}) {
            M.updateMovers(t);
            const AABB& b = M.walls[M.movers[0].wall].box;
            glm::vec3 want{(b.min.x + b.max.x) * 0.5f, b.max.y, (b.min.z + b.max.z) * 0.5f};
            for (int i = 0; i < 60 * 3; ++i) { std::vector<SpawnRequest> o; d.update(DT, 0, {99, 0, 99}, o); }   // into the wave
            if (glm::length(d.goalPos() - want) > 1e-4f) follows = false;
        }
        CHECK(follows, "a HOLD circle on a moving platform follows it");
        auto holdOn = [&](bool onIt) {
            WaveDirector h; h.level = &M; h.startArena(0);
            float t = 0.f; bool done = false;
            for (int i = 0; i < 60 * 8 && !done; ++i, t += DT) {
                M.updateMovers(t);
                glm::vec3 p = onIt ? h.goalPos() : glm::vec3{25.f, 0.f, 0.f};
                std::vector<SpawnRequest> o;
                h.update(DT, 1, p, o);   // one enemy alive somewhere: the wave stays up
                for (auto& ev : h.events) done |= ev.kind == DirectorEvent::GOAL_DONE;
                h.events.clear();
            }
            return done;
        };
        CHECK(holdOn(true) && !holdOn(false), "standing on it fills the hold; standing elsewhere doesn't");
    }

    // ---------------------------------------------------------------- the sun's flare (SOLAR)
    {
        LevelData S; LevelBuilder SB{S};
        for (int k = 0; k < 2; ++k)
            SB.mover({0.f, -80.5f, 0.f}, {2.f, 0.5f, 2.f}, Mover::Path::ORBIT, {20.f, 0.f, 0.f}, {0.f, 0.f, 20.f}, 60.f, k * 0.5f, {1, 1, 1});
        Arena a; a.name = "SUN"; a.subtitle = "";
        a.bounds = a.zone = LevelBuilder::aabb(-55, -100, -55, 55, -50, 55);
        a.shift = ArenaShift::SOLAR; a.sunPos = {0.f, -92.f, 0.f};
        a.waves = {{{EnemyType::HUSK, 1}}, {{EnemyType::HUSK, 1}}, {{EnemyType::HUSK, 1}}};
        S.arenas.push_back(a);
        ArenaShifts sh; sh.capture(S);
        sh.onWave(S, 0, 0, WaveGoal{}, 0.f);
        auto phaseAt = [&](float t) {
            ArenaShifts c = sh; c.flareClock = 0.f;
            for (float x = 0.f; x < t - 1e-4f; x += DT) c.update(DT, S, 0, true);
            return c.flarePhase();
        };
        CHECK(phaseAt(0.5f) == FlarePhase::WARN && phaseAt(1.7f) == FlarePhase::BURN && phaseAt(6.3f) == FlarePhase::BURN &&
              phaseAt(7.f) == FlarePhase::OFF && phaseAt(10.4f) == FlarePhase::OFF && phaseAt(10.7f) == FlarePhase::WARN,
              "the flare warns 1.5 s, burns 5 s, rests 4 s, and goes round again");
        ArenaShifts b = sh; b.flareClock = 2.f; b.flareAngle = 0.f;
        CHECK(b.flareHits(S, 0, {40.f, -80.f, 0.f}) && !b.flareHits(S, 0, {40.f, -80.f, 15.f}),
              "burning, it hits what's in its 24 degree wedge and nothing beside it");
        CHECK(!b.flareHits(S, 0, {-40.f, -80.f, 0.f}), "one arm in wave 1: the far side is safe");
        LevelData Sc = S; Sc.walls.push_back(Wall{LevelBuilder::aabb(30, -82, -2, 32, -70, 2)});
        CHECK(!b.flareHits(Sc, 0, {40.f, -80.f, 0.f}), "a pillar between you and the sun is cover");
        ArenaShifts w = sh; w.flareClock = 6.8f; w.flareAngle = 0.f;
        CHECK(!w.flareHits(S, 0, {40.f, -80.f, 0.f}), "resting, it burns nothing");
        sh.onWave(S, 0, 1, WaveGoal{}, 0.f);
        bool w1 = sh.flareArms == 1 && std::fabs(sh.speed[0] - 1.3f) < 1e-4f;
        sh.onWave(S, 0, 2, WaveGoal{}, 0.f);
        bool w2 = sh.flareArms == 2 && std::fabs(sh.speed[0] - 1.6f) < 1e-4f;
        ArenaShifts two = sh; two.flareClock = 2.f; two.flareAngle = 0.f;
        CHECK(w1 && w2 && two.flareHits(S, 0, {-40.f, -80.f, 0.f}), "wave 2: the rings quicken; wave 3: a second arm, opposite");
        sh.reset(S);
        CHECK(sh.flareArms == 1 && sh.speed[0] == 1.f && sh.flareClock == 0.f &&
              std::fabs(S.movers[0].period - 60.f) < 1e-3f, "a retry puts the sun and the rings back");
        float turned = 0.f; { ArenaShifts c = sh; c.flareAngle = 0.f; for (int i = 0; i < 60; ++i) c.update(DT, S, 0, true); turned = c.flareAngle; }
        CHECK(std::fabs(glm::degrees(turned) - 15.f) < 0.5f, "the flare turns 15 degrees a second");
    }

    // ---------------------------------------------------------------- nothing see-through, from any side
    {
        struct Lvl { const char* name; LevelData L; };
        std::vector<Lvl> lvls{{"Act I", buildLevel()}, {"the Gauntlet", buildGauntlet()}, {"Act II", buildAct2Level()}};
        bool undersides = true, shapesClosed = true;
        for (auto& lv : lvls) {
            const LevelData& L = lv.L;
            WorldGeometry G = buildWorldGeometry(L);
            // Every drawn box raised off the ground under it shows its underside
            auto hasUnderside = [&](const AABB& b) {
                glm::vec2 c{(b.min.x + b.max.x) * 0.5f, (b.min.z + b.max.z) * 0.5f};
                for (int t = 0; t < TEX_COUNT; ++t)
                    for (size_t i = 0; i + 3 < G.V[t].size(); ++i) {
                        const Vertex& v = G.V[t][i];
                        if (v.normal.y > -0.99f || std::fabs(v.position.y - b.min.y) > 1e-3f) continue;
                        // the face's four corners are consecutive (pushFace): test the quad's extent
                        if (i + 3 >= G.V[t].size()) continue;
                        float x0 = 1e9f, x1 = -1e9f, z0 = 1e9f, z1 = -1e9f;
                        bool quad = true;
                        for (int k = 0; k < 4; ++k)
                            quad &= G.V[t][i + k].normal.y < -0.99f && std::fabs(G.V[t][i + k].position.y - b.min.y) < 1e-3f;
                        if (!quad) continue;
                        for (int k = 0; k < 4; ++k) {
                            x0 = std::min(x0, G.V[t][i + k].position.x); x1 = std::max(x1, G.V[t][i + k].position.x);
                            z0 = std::min(z0, G.V[t][i + k].position.z); z1 = std::max(z1, G.V[t][i + k].position.z);
                        }
                        if (c.x > x0 && c.x < x1 && c.y > z0 && c.y < z1) return true;
                    }
                return false;
            };
            int missing = 0;
            auto checkBox = [&](const AABB& b) {
                float ground = L.baseFloor((b.min.x + b.max.x) * 0.5f, (b.min.z + b.max.z) * 0.5f);
                if (b.min.y <= ground + 0.1f) return;
                if (!hasUnderside(b)) {
                    if (missing++ < 3) std::printf("      %s: no underside on the box at (%.1f %.1f %.1f)\n", lv.name, b.min.x, b.min.y, b.min.z);
                    undersides = false;
                }
            };
            for (int i = 0; i < (int)L.walls.size(); ++i)
                if (!L.walls[i].hidden && !L.walls[i].dynamic && !L.isDoorWall(i)) checkBox(L.walls[i].box);
            for (auto& p : L.props) checkBox(p.box);
            // Every shape is closed, or drawn from both sides
            int open = 0;
            for (auto& sh : L.shapes) {
                if (sh.twoSided) continue;
                std::map<std::tuple<long, long, long, long, long, long>, int> edges;
                auto key = [](glm::vec3 p) { return std::make_tuple(std::lround(p.x * 200), std::lround(p.y * 200), std::lround(p.z * 200)); };
                sh.forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3) {
                    glm::vec3 v[3] = {a, b, c};
                    for (int k = 0; k < 3; ++k) {
                        auto p = key(v[k]), q = key(v[(k + 1) % 3]);
                        edges[std::tuple_cat(p, q)]++;
                    }
                });
                bool closed = true;
                for (auto& e : edges) {
                    auto& k = e.first;
                    auto rev = std::make_tuple(std::get<3>(k), std::get<4>(k), std::get<5>(k), std::get<0>(k), std::get<1>(k), std::get<2>(k));
                    if (!edges.count(rev)) { closed = false; break; }
                }
                if (!closed) { if (open++ < 3) std::printf("      %s: an open shape that's one-sided\n", lv.name); shapesClosed = false; }
            }
        }
        CHECK(undersides, "every raised box and prop shows its underside, in every level (Act II is below Y 0)");
        CHECK(shapesClosed, "every shape is closed or drawn from both sides: nothing vanishes seen from inside or below");
        // A curved wall has no slits: its band is covered all round, out to its outer edge
        std::vector<Shape> out; ShapeKit k{out, 0, false};
        k.curve({0.f, 0.f, 0.f}, 20.f, 0.f, 6.2831853f, 0.f, 1.f, 6.f, {1, 1, 1}, 24);
        bool covered = true;
        for (int i = 0; i < 360 && covered; ++i) {
            float a = glm::radians(i + 0.5f);
            for (float r : {17.2f, 20.f, 22.8f}) {
                glm::vec3 p{std::cos(a) * r, 0.5f, std::sin(a) * r};
                bool in = false;
                for (auto& sh : out) {
                    glm::vec3 lp = glm::vec3(glm::inverse(sh.xf) * glm::vec4(p, 1.f));
                    if (std::fabs(lp.x) <= 0.5f && std::fabs(lp.y) <= 0.5f && std::fabs(lp.z) <= 0.5f) { in = true; break; }
                }
                if (!in) { std::printf("      curve slit at %d degrees, r %.1f\n", i, r); covered = false; break; }
            }
        }
        CHECK(covered, "a curved wall has no slits, even at its outer edge");
        // The Orrery's terrace is drawn exactly where you can stand on it
        LevelData N = buildAct2Level();
        const glm::vec3 C{0.f, -80.f, -732.f};
        bool drawn = true;
        for (int i = 0; i < 180 && drawn; ++i) {
            float a = glm::radians(i * 2.f + 1.f);
            for (float r : {32.f, 41.f, 50.f}) {
                glm::vec3 p = C + glm::vec3{std::cos(a) * r, -0.5f, std::sin(a) * r};
                bool top = false;
                for (auto& w : N.walls)
                    if (!w.hidden && p.x > w.box.min.x && p.x < w.box.max.x && p.z > w.box.min.z && p.z < w.box.max.z &&
                        p.y > w.box.min.y && p.y < w.box.max.y && std::fabs(w.box.max.y - C.y) < 1e-3f) top = true;
                if (!top) { std::printf("      terrace not drawn at r %.0f, %d degrees\n", r, i * 2 + 1); drawn = false; break; }
            }
        }
        CHECK(drawn, "the Orrery's terrace is drawn wherever you can stand on it");
    }

    // ---------------------------------------------------------------- the Gauntlet (FAST)
    LevelData D = buildGauntlet();
    SpatialGrid dgrid; dgrid.build(D.walls);
    auto inBox = [](const AABB& b, glm::vec3 p) {
        return p.x >= b.min.x && p.x <= b.max.x && p.y >= b.min.y && p.y <= b.max.y && p.z >= b.min.z && p.z <= b.max.z;
    };
    {
        CHECK(D.fast && D.arenas.size() == 7, "the Gauntlet has seven rooms");
        bool placedOk = true, inBounds = true, starts = true, gates = true, breathers = true, roomy = true, triggers = true;
        int big = 0;
        for (size_t i = 0; i < D.arenas.size(); ++i) {
            const Arena& a = D.arenas[i];
            for (auto& w : a.waves) for (auto& e : w) {
                if (e.at.empty()) placedOk = false;
                for (auto& p : e.at) {
                    const EnemyStats& st = statsOf(e.type);
                    if (overlapsWall(D, boxAt(p, st.radius, st.height))) {
                        std::printf("      %s at (%.1f %.1f %.1f) is in a wall\n", st.name, p.x, p.y, p.z); placedOk = false; }
                    if (!inside(a.bounds, p) || p.y < a.bounds.min.y - 0.01f) {
                        std::printf("      %s at (%.1f %.1f %.1f) is outside level %d\n", st.name, p.x, p.y, p.z, (int)i); inBounds = false; }
                    // Nothing spawns in the breather: every enemy is in the fight zone
                    if (!inBox(a.trigger, p)) { std::printf("      %s at (%.1f %.1f %.1f) is before level %d's trigger\n", st.name, p.x, p.y, p.z, (int)i); breathers = false; }
                }
            }
            if (overlapsWall(D, boxAt(a.playerStart, 0.4f, 1.8f)) || D.arenaAt(a.playerStart) != (int)i) {
                std::printf("      level %d start (%.1f %.1f %.1f) blocked\n", (int)i, a.playerStart.x, a.playerStart.y, a.playerStart.z); starts = false; }
            if (!a.hasTrigger || inBox(a.trigger, a.playerStart)) triggers = false;
            if (i + 1 < D.arenas.size() && (a.exitDoor < 0 || !D.doors[a.exitDoor].locked)) gates = false;
            if (a.entryGate < 0 || D.doors[a.entryGate].locked) gates = false;
            glm::vec3 ext = a.zone.max - a.zone.min;
            if (std::min(ext.x, ext.z) < 20.f) roomy = false;
            if (std::max(ext.x, ext.z) >= 60.f) ++big;
            if (a.extraZones.empty()) roomy = false;
        }
        CHECK(placedOk, "every FAST enemy is hand-placed and clear of walls");
        CHECK(inBounds, "every FAST enemy is placed inside its own level");
        CHECK(starts, "every level's checkpoint is clear of walls");
        CHECK(triggers, "every level starts with a breather before its fight trigger");
        CHECK(breathers, "no enemy spawns in a breather");
        CHECK(gates, "every room has a way in, and every exit but the last is locked until the room is cleared");
        CHECK(roomy && big >= 4, "every room is at least 20 m across and reached by a tube; four or more are 60 m+ halls");
        // The route changes direction: some exits go north, some west, some east
        int north = 0, west = 0, east = 0, up = 0, down = 0;
        for (size_t i = 0; i + 1 < D.arenas.size(); ++i) {
            glm::vec3 d = D.arenas[i + 1].playerStart - D.arenas[i].playerStart;
            if (std::fabs(d.z) > std::fabs(d.x)) north += d.z < 0; else (d.x < 0 ? west : east)++;
            up += d.y > 4.f; down += d.y < -4.f;
        }
        CHECK(north > 0 && west > 0 && east > 0 && up > 0 && down > 0, "the route turns left and right and goes both up and down");
        int potions = 0;
        for (auto& p : D.placedPickups) potions += p.kind == 1;
        CHECK(potions >= 4, "breathers have health potions waiting");
        bool pickupsOk = true;
        for (auto& p : D.placedPickups) if (D.arenaAt(p.pos) < 0 || overlapsWall(D, boxAt(p.pos, 0.3f, 0.6f))) pickupsOk = false;
        CHECK(pickupsOk, "placed pickups sit in the open inside a level");
        CHECK(std::string(D.arenas[3].name) == "THE SPAN" && D.arenas[3].voidY > 0.f && D.arenas[3].hasRespawn &&
              D.arenas[3].respawn.y >= 20.f, "the Span has a void plane that sends you back onto its entry cliff");
        CHECK(inside(D.arenas.back().zone, D.finishPos) && D.finishPos.y > 20.f, "the finish beacon is on top of the tower");
        CHECK(D.parTimes[0] > 0.f && D.parTimes[0] < D.parTimes[1] && D.parTimes[1] < D.parTimes[2] && D.parTimes[2] < D.parTimes[3],
              "par times are ordered S < A < B < C");
        CHECK(padsLand(D, dgrid, keys), "every Gauntlet jump pad lands the player on something higher (>= 2.5 m up)");
        CHECK(moversClear(D), "no Gauntlet mover ever passes through a wall");
        // Consecutive rooms' zones (tubes included) overlap through the doors,
        // or you couldn't walk between them
        bool linked = true;
        auto zonesOf = [](const Arena& a) { std::vector<AABB> v = a.extraZones; v.push_back(a.zone); return v; };
        for (size_t i = 0; i + 1 < D.arenas.size(); ++i) {
            bool any = false;
            for (auto& z0 : zonesOf(D.arenas[i])) for (auto& z1 : zonesOf(D.arenas[i + 1]))
                if (z0.max.x > z1.min.x && z0.min.x < z1.max.x && z0.max.z > z1.min.z && z0.min.z < z1.max.z) any = true;
            if (!any) { std::printf("      room %d doesn't connect to room %d\n", (int)i, (int)i + 1); linked = false; }
        }
        CHECK(linked, "each room's zones connect to the next room's");
        CHECK(doorsFill(D, "gauntlet") && D.doors.size() >= 16, "every Gauntlet door is two halves filling a gap (16+ doors)");

        // The main route can be walked: tubes, doorways and rooms leave room
        // for the player (with the doors open), and every door, shut, blocks it
        bool routes = true;
        routes &= routeClear(D, {{0,3,0}, {0,3,-50}, {-10,3,-62}, {-10,3,-118.5f}, {-59,3,-118.5f}}, "canal");
        routes &= routeClear(D, {{-62,0,-113}, {-79,0,-113}, {-79,0,-118.5f}, {-124,0,-118.5f}}, "sluice");
        routes &= routeClear(D, {{-190.5f,20,-112}, {-190.5f,20,-164}}, "ascent top");
        routes &= routeClear(D, {{-185,20,-263.5f}, {-155,20,-263.5f}}, "span to well");
        routes &= routeClear(D, {{-128,0,-263.5f}, {-60,0,-263.5f}}, "well to pumpworks");
        routes &= routeClear(D, {{-49.5f,5,-280}, {-49.5f,5,-290}}, "control room");
        routes &= routeClear(D, {{-30,0,-263.5f}, {40,0,-263.5f}}, "pumpworks to tower");
        CHECK(routes, "the route through every tube and doorway is wide and tall enough to walk");
        bool shutBlocks = !walkClear(D, {0,3,0}, {0,3,-50}, false, nullptr) && !walkClear(D, {-10,3,-118.5f}, {-59,3,-118.5f}, false, nullptr) &&
                          !walkClear(D, {-30,0,-263.5f}, {40,0,-263.5f}, false, nullptr);
        CHECK(shutBlocks, "a shut door blocks its doorway");

        // Every horizontal boost tube fires you down it; the lift shaft takes
        // you up to the tower's top balcony
        bool fired = true;
        for (auto& b : D.boosters) {
            if (b.dir.y > 0.5f || b.box.min.y > 26.f) continue;
            glm::vec3 c = (b.box.min + b.box.max) * 0.5f, startP = c - b.dir * (std::fabs(glm::dot(b.box.max - b.box.min, b.dir)) * 0.5f - 0.5f);
            startP.y = b.box.min.y;
            float top = 0.f;
            Player pl = ride(D, dgrid, startP, 1.2f, keys, &top);
            float went = glm::dot(pl.position - startP, b.dir);
            float len = std::fabs(glm::dot(b.box.max - b.box.min, b.dir));
            if (went < len - 1.f || top < b.speed * 0.95f) {
                std::printf("      booster at (%.0f %.0f %.0f) carried the player %.1f m, top speed %.1f\n", c.x, c.y, c.z, went, top); fired = false; }
        }
        CHECK(fired, "every boost tube shoots you down it at full speed");
        {
            const Booster* shaft = nullptr;
            for (auto& b : D.boosters) if (b.dir.y > 0.5f) shaft = &b;
            bool up = false;
            if (shaft) {
                glm::vec3 c = (shaft->box.min + shaft->box.max) * 0.5f; c.y = 0.f;
                Player pl = ride(D, dgrid, c, 4.f, keys);
                std::printf("      lift shaft: ended at (%.1f %.1f %.1f) %s\n", pl.position.x, pl.position.y, pl.position.z, pl.onGround ? "standing" : "airborne");
                up = pl.onGround && std::fabs(pl.position.y - 24.f) < 0.1f;
            }
            CHECK(up, "the tower's lift shaft puts you on its top balcony");
        }
    }

    // The tower at the end can be climbed by its pads alone (lifts aside)
    {
        const JumpPad* top = nullptr;
        for (auto& p : D.pads) if (p.centre.y > 20.f) top = &p;
        bool ok = top != nullptr;
        if (top) {
            Player p(top->centre); p.velocity = top->launch; p.onGround = false;
            for (int i = 0; i < 600 && !(p.onGround && i > 2); ++i) p.update(DT, keys, D.walls.data(), (int)D.walls.size(), false, &dgrid);
            std::printf("      tower pad lands at y=%.1f (beacon at %.1f)\n", p.position.y, D.finishPos.y);
            ok = std::fabs(p.position.y - D.finishPos.y) < 0.1f && glm::length(glm::vec2(p.position.x - D.finishPos.x, p.position.z - D.finishPos.z)) < 6.f;
        }
        CHECK(ok, "the top balcony's pads land you next to the finish beacon");
    }

    // A player standing on a lift rides it up (the carry GameplayState does)
    {
        LevelData M = L;
        // the Spire's first lift: tier 1 (y 6) to tier 2 (y 12)
        const Mover* liftP = nullptr;
        for (auto& m : M.movers) if (m.path == Mover::Path::PINGPONG && m.b.y > 4.f && m.a == glm::vec3{0.f}) { liftP = &m; break; }
        const Mover& lift = *liftP;
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
    {
        // Head hitboxes: a shot from the player's eye at the drawn head is a
        // headshot (the JUGGERNAUT included); one at the chest is not
        bool headsHit = true, chestsMiss = true;
        for (EnemyType t : {EnemyType::HUSK, EnemyType::SENTINEL, EnemyType::BRUTE,
                            EnemyType::JUGGERNAUT, EnemyType::WARDEN}) {
            Enemy e(t, {0, 0, 0});
            e.spawnTimer = 0.f; e.yaw = 0.7f;
            rig::HumanoidLook L = rig::humanoidDims(t);
            float neck = L.legLen + L.pelvisH + L.torsoH;
            glm::vec3 eye{0.f, 1.7f, 12.f};
            AABB head;
            bool has = headBox(e, head);
            auto hits = [&](float y) { glm::vec3 to{0.f, y, 0.f}; return rayBoxHit(eye, glm::normalize(to - eye), head) > 0.f; };
            std::printf("      %-10s head %.2f..%.2f (neck %.2f)\n", statsOf(t).name, head.min.y, head.max.y, neck);
            if (!has || !hits(neck + L.headS * 0.5f) || !hits(neck + 0.05f)) headsHit = false;
            if (hits(L.legLen + L.pelvisH + L.torsoH * 0.5f)) chestsMiss = false;
        }
        CHECK(headsHit, "shooting a humanoid's head (including the Juggernaut's) lands in its head hitbox");
        CHECK(chestsMiss, "chest shots are not headshots");
        Enemy mite(EnemyType::MITE, {0, 0, 0});
        AABB none;
        CHECK(!headBox(mite, none), "mites have no head to hit");
    }

    // ---------------------------------------------------------------- shapes
    {
        std::vector<Shape> v;
        ShapeKit k{v};
        k.column({5, 0, 5}, 2.f, 6.f, glm::vec3{0.5f}, 10, 0.6f);
        k.shaft({0, 0, 0}, 8.f, 10.f, glm::vec3{0.5f});
        k.box({0, 3, 0}, {2, 1, 4}, glm::vec3{0.5f}, 0.7f, 0.3f, 0.2f);
        k.rod({0, 2, 0}, {6, 5, -3}, 0.5f, glm::vec3{0.5f});
        bool outCol = true, inShaft = true, outBox = true, finite = true, rodEnds = true;
        float rodMin = 1e9f, rodMax = -1e9f;
        v[0].forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
            glm::vec3 m = (a + b + c) / 3.f - glm::vec3{5, 0, 5};
            if (std::fabs(n.y) < 0.9f) { glm::vec3 r{m.x, 0, m.z}; if (glm::dot(n, r) <= 0.f) outCol = false; }
            else if ((n.y > 0.f) != (m.y > 3.f)) outCol = false;   // the top faces up, the base down
        });
        v[1].forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
            glm::vec3 m = (a + b + c) / 3.f;
            if (glm::dot(n, glm::vec3{m.x, 0, m.z}) >= 0.f) inShaft = false;
        });
        v[2].forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
            if (glm::dot(n, (a + b + c) / 3.f - glm::vec3{0, 3, 0}) <= 0.f) outBox = false;
            for (auto p : {a, b, c}) if (!std::isfinite(p.x + p.y + p.z)) finite = false;
        });
        v[3].forEachTri([&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3) {
            for (auto p : {a, b, c}) { float t = glm::dot(p - glm::vec3{0, 2, 0}, glm::normalize(glm::vec3{6, 3, -3})); rodMin = std::min(rodMin, t); rodMax = std::max(rodMax, t); }
        });
        rodEnds = std::fabs(rodMin) < 0.01f && std::fabs(rodMax - glm::length(glm::vec3{6, 3, -3})) < 0.01f;
        CHECK(outCol && outBox && finite, "shapes: columns and turned boxes face outward");
        CHECK(inShaft, "shapes: a shaft faces inward, to be seen from inside");
        CHECK(rodEnds, "shapes: a rod runs exactly from one end to the other");
        size_t tris = 0;
        for (auto& sh : D.shapes) sh.forEachTri([&](glm::vec3, glm::vec3, glm::vec3, glm::vec3) { ++tris; });
        std::printf("      gauntlet: %zu shapes, %zu triangles\n", D.shapes.size(), tris);
        CHECK(tris < 120000, "the gauntlet's shapes stay within the triangle budget");
    }

    // ---------------------------------------------------------------- AI
    const Arena& A0 = L.arenas[0];
    const Arena& BOSS = L.arenas[3];          // the Core: the Warden
    const Arena& SAN  = L.arenas.back();      // the Sanctum: the Sovereign
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
    // The Juggernaut: siege shells at range, a smash up close, armor, a parry window
    { Enemy j(EnemyType::JUGGERNAUT, {0,0,-10});
      EnemyWorld w = worldFor(L, grid, P0, A0);
      bool heavyShot = false, smashed = false, window = false;
      for (int i = 0; i < 60 * 15; ++i) { j.update(DT, w); heavyShot |= j.ev.shots > 0 && j.ev.shotHeavy; }
      Enemy k(EnemyType::JUGGERNAUT, {0,0,20});
      EnemyWorld w2 = worldFor(L, grid, {0,0,22.5f}, A0);
      for (int i = 0; i < 60 * 10; ++i) { k.update(DT, w2); window |= k.parryWindow(); smashed |= k.ev.meleeHit; }
      CHECK(heavyShot, "JUGGERNAUT fires heavy (parryable) siege shells at range");
      CHECK(smashed && window, "JUGGERNAUT smashes up close, with a parry window before it lands");
      float armored = k.armorMult();
      k.stagger(2.5f);
      CHECK(armored == 0.5f && k.armorMult() == 2.f && k.attack == AttackKind::NONE,
            "its armor halves bullets; once broken it takes double damage");
      bool still = true; glm::vec3 p0 = k.position;
      for (int i = 0; i < 60; ++i) { k.update(DT, w2); if (k.ev.meleeHit || k.ev.shots) still = false; }
      CHECK(still && glm::length(glm::vec2(k.position.x - p0.x, k.position.z - p0.z)) < 0.01f,
            "a broken JUGGERNAUT stands still and can't attack"); }
    // The Shieldbearer: blocks from the front only, bashes up close, can be parried
    {
        Enemy sb(EnemyType::SHIELDBEARER, {0, 0, -10});
        sb.spawnTimer = 0.f; sb.state = EnemyState::ACTIVE; sb.yaw = 0.f;   // facing +Z
        CHECK(sb.blocks({0, 0, -1}) && !sb.blocks({0, 0, 1}) && !sb.blocks({-1, 0, 0}),
              "SHIELDBEARER blocks shots from the front, not from behind or the side");
        Enemy k(EnemyType::SHIELDBEARER, {0, 0, 20});
        EnemyWorld w2 = worldFor(L, grid, {0, 0, 22.f}, A0);
        bool bashed = false, window = false;
        for (int i = 0; i < 60 * 10; ++i) { k.update(DT, w2); bashed |= k.ev.meleeHit; window |= k.parryWindow(); }
        CHECK(bashed && window, "SHIELDBEARER bashes up close, with a parry window");
        k.stagger(k.staggerTime());
        CHECK(!k.blocks(-glm::vec3{std::sin(k.yaw), 0.f, std::cos(k.yaw)}), "a parried SHIELDBEARER's shield is down");
        // It starts behind the Yard's dais and obelisk, so it has to work its
        // way round them; across seeds (its strafe is random) it nearly always does
        int firing = 0; bool walled = false;
        for (int sd = 1; sd <= 30; ++sd) {
            srand(sd);
            auto q = simulate(L, grid, EnemyType::SHIELDBEARER, {0,0,-14}, P0, A0, 12.f);
            firing += q.shots >= 3; walled |= q.inWall;
        }
        std::printf("      SHIELDBEARER fired from cover in %d/30 runs\n", firing);
        CHECK(firing >= 28 && !walled, "SHIELDBEARER gets round cover to fire spreads at mid range, and stays out of walls");
    }

    // The Conductor: tethers (and shields) its three nearest allies, never a
    // boss or a conduit, each ally once; hovers behind them; keeps its distance
    {
        auto ready = [](Enemy& e) { e.state = EnemyState::ACTIVE; e.spawnTimer = 0.f; };
        std::vector<Enemy> es;
        es.emplace_back(EnemyType::CONDUCTOR, glm::vec3{0, 5, -10});
        es.emplace_back(EnemyType::CONDUCTOR, glm::vec3{1, 5, -10});
        es.emplace_back(EnemyType::HUSK,    glm::vec3{2, 0, -12});
        es.emplace_back(EnemyType::RIPPER,  glm::vec3{-2, 0, -12});
        es.emplace_back(EnemyType::SENTINEL,glm::vec3{0, 0, -15});
        es.emplace_back(EnemyType::BRUTE,   glm::vec3{4, 0, -8});
        es.emplace_back(EnemyType::WARDEN,  glm::vec3{0, 0, -9});
        es.emplace_back(EnemyType::CONDUIT, glm::vec3{-1, 0, -9});
        es.emplace_back(EnemyType::HUSK,    glm::vec3{0, 0, -40});   // out of range
        for (auto& e : es) ready(e);
        glm::vec3 player{0, 0, 10};
        linkConductors(es, player);
        int shielded = 0; bool wrong = es[6].shielded || es[7].shielded || es[8].shielded || es[0].shielded;
        for (auto& e : es) shielded += e.shielded;
        CHECK(es[0].linkCount == 3 && es[1].linkCount == 1 && shielded == 4 && !wrong,
              "a CONDUCTOR tethers its three nearest allies - not a boss, a conduit or one out of range - and each ally once");
        CHECK(es[0].hasAnchor && es[0].supportAnchor.z < -12.f && es[0].supportAnchor.y > 3.f,
              "a CONDUCTOR hovers above and behind the allies it shields");
        es.erase(es.begin());   // one down: the other takes up what it can (still three at most)
        linkConductors(es, player);
        shielded = 0; for (auto& e : es) shielded += e.shielded;
        bool regrouped = shielded == es[0].linkCount && es[0].linkCount <= 3;
        es.erase(es.begin());   // both down
        linkConductors(es, player);
        shielded = 0; for (auto& e : es) shielded += e.shielded;
        CHECK(regrouped && shielded == 0, "kill the CONDUCTORs and their tethers drop");

        auto s = simulate(L, grid, EnemyType::CONDUCTOR, {0, 6, -20}, P0, A0, 12.f);
        CHECK(!s.leftBounds && s.closest > 7.f && s.minY > 2.f && s.shots == 0 && s.melee == 0,
              "a CONDUCTOR on its own keeps its distance, flies, and never attacks");
    }

    // The Sovereign: dashes (and dashes again), sweep-sweep-cleave combos,
    // leaps onto high ground, crescents at range, a parry window, enrage
    {
        glm::vec3 C{0.f, 0.f, -348.f};
        Enemy b(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -14});
        EnemyWorld w = worldFor(L, grid, C, SAN);
        int melee = 0, cleaves = 0, slams = 0; bool sweepL = false, sweepR = false;
        bool window = false, inWall = false, outside = false;
        for (int i = 0; i < 60 * 25; ++i) {
            b.update(DT, w);
            melee += b.ev.meleeHit; slams += b.ev.slam;
            sweepL |= b.ev.slash == 0; sweepR |= b.ev.slash == 1; cleaves += b.ev.slash == 2;
            window |= b.parryWindow();
            if (!inside(SAN.bounds, b.position)) outside = true;
            AABB box = boxAt(b.position + glm::vec3{0, 0.05f, 0}, b.radius() - 0.05f, b.height() - 0.1f);
            for (auto& wl : L.walls) if (!wl.dynamic && overlapsBox(box, wl.box)) inWall = true;
        }
        std::printf("      sovereign (player standing): %d hits, %d cleaves, %d slams\n", melee, cleaves, slams);
        // A player kiting him in a wide circle: he closes the gap by dashing
        Enemy k(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -20});
        int dashes = 0; bool chained = false; float lastDash = -10.f;
        for (int i = 0; i < 60 * 20; ++i) {
            float t = i * DT, ang = t * 0.4f;
            glm::vec3 p = C + glm::vec3{std::cos(ang) * 16.f, 0.f, std::sin(ang) * 16.f};
            EnemyWorld kw = worldFor(L, grid, p, SAN);
            kw.playerVel = glm::vec3{-std::sin(ang), 0.f, std::cos(ang)} * 6.4f;
            k.update(DT, kw);
            if (k.ev.dashStarted) { if (t - lastDash < 1.4f) chained = true; lastDash = t; ++dashes; }
        }
        std::printf("      sovereign (player kiting): %d dashes\n", dashes);
        CHECK(dashes >= 4 && chained, "SOVEREIGN dashes at a kiting player, and chains a second dash out of the first");
        CHECK(sweepL && sweepR && cleaves >= 1 && slams >= 1, "SOVEREIGN's combo: a sweep each way, then a cleave with a shockwave");
        CHECK(melee >= 4, "SOVEREIGN's sword connects with a player who just stands there");
        CHECK(window, "SOVEREIGN's sweeps and cleave can be parried as they land");
        CHECK(!inWall && !outside, "SOVEREIGN stays out of walls and inside the Sanctum");

        // High ground: stand on the north island (11 m) and he comes up
        Enemy h(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -6});
        EnemyWorld hw = worldFor(L, grid, C + glm::vec3{0, 11.f, -24.f}, SAN);
        bool leapt = false; float topY = 0.f;
        for (int i = 0; i < 60 * 12; ++i) { h.update(DT, hw); leapt |= h.ev.leapStarted; topY = std::max(topY, h.grounded ? h.position.y : 0.f); }
        CHECK(leapt && topY > 10.5f, "SOVEREIGN leaps up onto the island you're standing on");

        // Far away: crescents
        Enemy f(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -30});
        EnemyWorld fw = worldFor(L, grid, C + glm::vec3{0, 0, 28}, SAN);
        int shots = 0;
        for (int i = 0; i < 60 * 12; ++i) { f.update(DT, fw); shots += f.ev.shots; }
        CHECK(shots >= 5, "SOVEREIGN throws crescent slashes at range");

        float before = b.armorMult();
        b.stagger(b.staggerTime());
        CHECK(before == 1.f && b.armorMult() == 2.f && b.dashTimer == 0.f && b.attack == AttackKind::NONE,
              "a parried SOVEREIGN drops everything and takes extra damage");
        Enemy r(EnemyType::SOVEREIGN, C); r.spawnTimer = 0.f; r.state = EnemyState::ACTIVE;
        r.takeDamage(r.maxHealth * 0.55f);
        CHECK(r.enraged, "SOVEREIGN enrages below half health");

        // No camping: a player who keeps running to the far corner perches
        // (5 m up) gets visited - he shadow-steps or leaps to them - and
        // has blades called down on them
        {
            Enemy c(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -20});
            const glm::vec3 perch[2] = {C + glm::vec3{39.f, 5.f, 39.f}, C + glm::vec3{-39.f, 5.f, -39.f}};
            int at = 0, blinks = 0, leaps = 0, nearStrikes = 0; bool reached = false;
            for (int i = 0; i < 60 * 30; ++i) {
                glm::vec3 p = perch[at];
                EnemyWorld cw = worldFor(L, grid, p, SAN);
                c.update(DT, cw);
                blinks += c.ev.blinked; leaps += c.ev.leapStarted;
                for (int k = 0; k < c.ev.strikes; ++k)
                    if (glm::length(glm::vec2(c.ev.strikePos[k].x - p.x, c.ev.strikePos[k].z - p.z)) < 3.f) ++nearStrikes;
                glm::vec3 to = p - c.position;
                if (glm::length(glm::vec2(to.x, to.z)) < 6.f && std::fabs(to.y) < 1.5f) { reached = true; at = 1 - at; }
            }
            std::printf("      sovereign (perch camper): %d blinks, %d leaps, %d blades near you\n", blinks, leaps, nearStrikes);
            CHECK(reached && blinks + leaps >= 2, "SOVEREIGN comes up to a player camping a far perch (shadow step or leap)");
            CHECK(nearStrikes >= 1, "SOVEREIGN calls blades down on a player who keeps their distance");
        }
        // Shots from range bounce off his guard (when he's not mid-attack),
        // and he answers with a crescent; up close, behind, or broken they land
        {
            Enemy g(EnemyType::SOVEREIGN, C); g.spawnTimer = 0.f; g.state = EnemyState::ACTIVE; g.yaw = 0.f;   // faces +Z
            glm::vec3 fromFront{0.f, 0.f, -1.f};   // a round travelling -Z: fired from in front of him
            bool far = g.deflects(fromFront, 30.f), close = g.deflects(fromFront, 8.f), back = g.deflects(-fromFront, 30.f);
            g.stagger(g.staggerTime());
            bool broken = g.deflects(fromFront, 30.f);
            CHECK(far && !close && !back && !broken, "SOVEREIGN deflects shots from range, but not up close, from behind, or broken");
        }
        // Up close he has more than one combo: whirlwind, thrust, rupture
        {
            Enemy m(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -9});
            EnemyWorld mw = worldFor(L, grid, C, SAN);
            bool whirl = false, thrust = false, rupture = false;
            for (int i = 0; i < 60 * 45; ++i) {
                m.update(DT, mw);
                whirl |= m.whirlTimer > 0.f; thrust |= m.attack == AttackKind::THRUST || m.thrustTimer > 0.f;
                for (int k = 0; k < m.ev.strikes; ++k) rupture |= m.ev.strikeKind[k] == 1;
            }
            std::printf("      sovereign (duel): whirl %d thrust %d rupture %d\n", whirl, thrust, rupture);
            CHECK(whirl && thrust && rupture, "SOVEREIGN mixes whirlwinds, thrusts and ruptures into the duel");
            CHECK(inside(SAN.bounds, m.position), "SOVEREIGN stays in the Sanctum through all of it");
        }
        // Enraged, he sends phantoms of himself dashing in from the sides
        {
            Enemy e(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -12}); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
            e.takeDamage(e.maxHealth * 0.6f);
            EnemyWorld ew = worldFor(L, grid, C, SAN);
            int phantoms = 0;
            for (int i = 0; i < 60 * 30; ++i) { e.update(DT, ew); phantoms += e.ev.phantoms; }
            CHECK(phantoms >= 2, "an enraged SOVEREIGN sends phantoms after you");
        }
    }
    // The Sanctum's hazards: blades that land after their warning, eruptions
    // you can jump, phantom dashes, and the last stand's burning edge
    {
        glm::vec3 C{0.f, 0.f, -348.f};
        SovereignHazards hz;
        hz.addStrike(C, 0, 0.8f);
        hz.addStrike(C + glm::vec3{10, 0, 0}, 1, 0.3f);
        int early = 0, hitsBlade = 0, hitsJumped = 0;
        for (int i = 0; i < 30; ++i) early += (int)hz.update(DT, C + glm::vec3{0.5f, 0, 0}).size();
        for (int i = 0; i < 60; ++i) {
            for (auto& h : hz.update(DT, C + glm::vec3{0.5f, 0, 0})) hitsBlade += h.kind == 0;
        }
        SovereignHazards hz2;
        hz2.addStrike(C, 1, 0.2f);
        for (int i = 0; i < 40; ++i) hitsJumped += (int)hz2.update(DT, C + glm::vec3{0, 2.2f, 0}).size();
        CHECK(early == 0 && hitsBlade == 1, "a falling blade waits for its warning, then hits whoever's under it");
        CHECK(hitsJumped == 0, "a rupture's eruption can be jumped");
        SovereignHazards hz3;
        hz3.addPhantom(C + glm::vec3{-14, 0, 0}, {1, 0, 0});
        int phHits = 0;
        for (int i = 0; i < 90; ++i) phHits += (int)hz3.update(DT, C).size();
        CHECK(phHits == 1, "a phantom's dash hits once on its way through");
        CHECK(SovereignHazards::burns(C + glm::vec3{30, 0, 0}, C) && SovereignHazards::burns(C + glm::vec3{0, 11, 24}, C) &&
              !SovereignHazards::burns(C + glm::vec3{10, 0, -10}, C) && !SovereignHazards::burns(C + glm::vec3{19, 6.3f, 0}, C),
              "the last stand burns the edges and the islands, not the middle or the orbiting platforms");
    }
    { auto s = simulate(L, grid, EnemyType::WARDEN, BOSS.bossSpawn, BOSS.playerStart, BOSS, 30.f);
      CHECK(s.shots >= 7, "WARDEN fires volleys");
      CHECK(s.summons >= 1, "WARDEN summons adds");
      CHECK(!s.leftBounds, "WARDEN stays inside the arena"); }

    // A Ripper starting behind the furnace in the Foundry has to go around it
    // (every time, not just with lucky dice: 20 runs with different seeds)
    { const Arena& A1 = L.arenas[1];
      int made = 0; float worst = 0.f;
      for (int seed = 0; seed < 20; ++seed) {
          srand(1000 + seed);
          auto s = simulate(L, grid, EnemyType::RIPPER, {0,0,-86}, {0,0,-64}, A1, 10.f);
          made += s.closest < 3.f; worst = std::max(worst, s.closest);
      }
      std::printf("      ripper behind furnace: %d/20 reach the player, worst closest approach %.1f m\n", made, worst);
      CHECK(made == 20, "RIPPER finds its way around the furnace to the player, every time"); }

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
    { const Arena& S3 = D.arenas[3];
      auto s = simulate(D, dgrid, EnemyType::RAPTOR, {-193, 30.f, -212.f}, S3.respawn, S3, 15.f);
      CHECK(s.minY >= S3.bounds.min.y + 1.4f, "a Raptor over the Span never sinks below the room's floor"); }
    { const Arena& S2 = D.arenas[2];   // gunners on the Ascent's terraces stay up there
      auto s = simulate(D, dgrid, EnemyType::HUSK, {-196, 20.f, -114.f}, {-120.f, 0.f, -118.f}, S2, 15.f);
      CHECK(s.minY > 19.5f, "a Husk on the Ascent's top terrace holds it while you climb"); }

    // ---------------------------------------------------------------- difficulty
    {
        bool ordered = true;
        for (int i = 0; i + 1 < DIFFICULTY_LEVELS; ++i) {
            const DifficultyTuning &a = difficulty(i), &b = difficulty(i + 1);
            if (!(b.damage > a.damage && b.attackRate > a.attackRate && b.lead >= a.lead && b.windup < a.windup + 1e-4f &&
                  b.heal < a.heal && b.waveSize > a.waveSize)) ordered = false;
        }
        CHECK(ordered && difficulty(DIFFICULTY_DEFAULT).damage > 1.f && difficulty(0).lead == 0.f,
              "each difficulty is harder than the last; STANDARD is harder than the original game (LENIENT)");
        // A Husk leads a strafing player at STANDARD, and doesn't at LENIENT
        auto aimAt = [&](int level) {
            Enemy e(EnemyType::HUSK, {0, 0, -14});
            EnemyWorld w = worldFor(L, grid, {0, 0, 4}, A0);
            w.playerVel = {7.f, 0.f, 0.f};
            w.tune = &difficulty(level);
            for (int i = 0; i < 60 * 10; ++i) {
                e.update(DT, w);
                if (e.ev.shots) return e.ev.shotDir[0].x - glm::normalize(w.playerEye - e.ev.shotOrigin).x;   // ahead of the direct line
            }
            return -1.f;
        };
        float lenient = aimAt(0), standard = aimAt(DIFFICULTY_DEFAULT);
        std::printf("      husk aim x: lenient %.3f, standard %.3f\n", lenient, standard);
        CHECK(std::fabs(lenient) < 0.02f && standard > 0.1f, "enemies lead a moving player's shots (from STANDARD up)");
        WaveDirector d; d.level = &L; d.countScale = difficulty(3).waveSize; d.maxAliveBonus = difficulty(3).maxAliveBonus;
        int base = 0, scaled = 0;
        for (auto& e : L.arenas[0].waves[0]) { base += e.total(); scaled += d.countOf(e); }
        WaveEntry boss(EnemyType::WARDEN, 1);
        CHECK(scaled > base && d.countOf(boss) == 1 && d.maxAlive() > L.arenas[0].maxAlive,
              "harder difficulties bring bigger waves and more at once - but still one Warden");
    }

    // ---------------------------------------------------------------- director: ARENA
    {
        WaveDirector d; d.level = &L;
        d.startArena(0);
        std::map<DirectorEvent, int> counts;
        int spawned = 0, expected = 0, maxAliveSeen = 0, bossSpawns = 0, conduitSpawns = 0, conduitsWanted = 0;
        bool tooClose = false, overCap = false, squadsTogether = true;
        // Kill-everything waves spawn exactly what they list (squads included);
        // goal waves refill until the goal is met, so they're checked by finishing
        for (auto& a : L.arenas)
            for (int wv = 0; wv < (int)a.waves.size(); ++wv) {
                bool goal = wv < (int)a.goals.size() && a.goals[wv].kind != WaveGoal::KILL_ALL;
                if (goal) { if (a.goals[wv].kind == WaveGoal::CONDUITS) conduitsWanted += (int)a.goals[wv].points.size(); continue; }
                for (auto& e : a.waves[wv]) expected += e.total();
            }

        glm::vec3 player = L.arenas[0].playerStart;
        struct Sim { float t; EnemyType type; glm::vec3 pos; };
        std::vector<Sim> alive;   // each enemy "survives" 3 s, so the cap gets exercised
        std::map<int, int> goalsDone;
        for (int tick = 0; tick < 60 * 60 * 30 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
            std::vector<SpawnRequest> out;
            bool goalWave = d.hasGoal();
            // HOLD: the player stands in the circle
            if (d.goal().kind == WaveGoal::HOLD) player = d.goal().pos + glm::vec3{0.f, 0.05f, 0.f};
            int fighters = 0;
            for (auto& a : alive) fighters += a.type != EnemyType::CONDUIT;
            d.update(DT, (int)alive.size(), player, out);
            for (size_t k = 0; k < out.size(); ++k) {
                auto& r = out[k];
                if (r.type == EnemyType::CONDUIT) { ++conduitSpawns; alive.push_back({3.f, r.type, r.pos}); continue; }
                if (!goalWave) ++spawned;
                if (isBoss(r.type)) ++bossSpawns;
                if (glm::length(glm::vec2(r.pos.x - player.x, r.pos.z - player.z)) < WaveDirector::SAFE_RADIUS - 0.5f &&
                    d.goal().kind != WaveGoal::HOLD) tooClose = true;
                if (k > 0 && out[k - 1].type != EnemyType::CONDUIT && !statsOf(r.type).flying &&
                    glm::length(r.pos - out[0].pos) > 6.f && out.size() > 1) squadsTogether = false;
                alive.push_back({3.f, r.type, r.pos});
            }
            fighters = 0;
            for (auto& a : alive) fighters += a.type != EnemyType::CONDUIT;
            if (fighters > d.maxAlive()) overCap = true;
            maxAliveSeen = std::max(maxAliveSeen, fighters);
            for (auto& a : alive) a.t -= DT;
            for (auto& a : alive) if (a.t <= 0.f && a.type == EnemyType::CONDUIT) d.onConduitDestroyed(a.pos);
            alive.erase(std::remove_if(alive.begin(), alive.end(), [](const Sim& a){ return a.t <= 0.f; }), alive.end());
            for (auto& ev : d.events) {
                counts[ev.kind]++;
                if (ev.kind == DirectorEvent::GOAL_DONE) { alive.clear(); goalsDone[d.arena * 10 + ev.value]++; }   // the rest collapse
            }
            d.events.clear();
            // Walk into the next arena once the gate opens
            if (d.phase == WaveDirector::Phase::CLEARED) player = L.arenas[d.arena + 1].playerStart;
            else if (d.goal().kind != WaveGoal::HOLD && d.phase != WaveDirector::Phase::VICTORY) player = L.arenas[d.arena].playerStart;
        }
        int n = (int)L.arenas.size(), goalWaves = 0;
        for (auto& a : L.arenas) for (auto& g : a.goals) goalWaves += g.kind != WaveGoal::KILL_ALL;
        std::printf("      spawned %d of %d (kill-all waves), peak alive %d, %d goal waves met\n",
                    spawned, expected, maxAliveSeen, (int)goalsDone.size());
        CHECK(d.phase == WaveDirector::Phase::VICTORY, "a simulated ARENA run reaches victory");
        CHECK(spawned == expected, "every queued enemy of a kill-all wave spawns exactly once (squads included)");
        CHECK(bossSpawns == 2, "each boss (the Warden, the Sovereign) spawns once");
        CHECK(counts[DirectorEvent::ARENA_START] == n, "every arena starts");
        CHECK(counts[DirectorEvent::WAVE_START] == 3 * (n - 1) - 1 && counts[DirectorEvent::BOSS_START] == 2,
              "every wave starts; the Core and the Sanctum end on their bosses");
        CHECK(counts[DirectorEvent::ARENA_CLEARED] == n && counts[DirectorEvent::VICTORY] == 1 &&
              counts[DirectorEvent::FINISH_OPEN] == 0, "each arena clears, then victory");
        CHECK(counts[DirectorEvent::NEW_TYPE] == (int)EnemyType::COUNT - 6,   // the Seraph, the Anchor, the Penitent, the Revenant, the Weaver and the Leviathan are Act II's
              "each Act I enemy type is introduced exactly once");
        CHECK(goalWaves >= 3 && (int)goalsDone.size() == goalWaves && counts[DirectorEvent::GOAL_DONE] == goalWaves,
              "every goal wave (hold, conduits, survive) is met once, and that ends it");
        CHECK(conduitSpawns == conduitsWanted && conduitsWanted > 0, "a conduit wave raises its conduits once");
        CHECK(squadsTogether, "a squad arrives together, its escort close behind its leader");
        CHECK(!overCap, "never more fighters alive than the arena's cap");
        CHECK(!tooClose, "nothing spawns within the safe radius of the player");
    }
    {
        // HOLD only counts while you're in the circle and it isn't contested;
        // SURVIVE runs out on its own; a conduit wave keeps coming until they're down
        WaveDirector d; d.level = &L;
        int yard = 0, hw = -1;
        for (int i = 0; i < (int)L.arenas[yard].goals.size(); ++i) if (L.arenas[yard].goals[i].kind == WaveGoal::HOLD) hw = i;
        d.startArena(yard); d.wave = hw; d.skipIntro();
        std::vector<SpawnRequest> out;
        d.update(DT, 0, L.arenas[yard].playerStart, out);
        const WaveGoal g = d.goal();
        float secs = g.seconds;
        for (int t = 0; t < (int)(secs * 2.f / DT); ++t) { out.clear(); d.update(DT, 3, L.arenas[yard].playerStart, out); }
        bool outsideNothing = d.goalTimer == 0.f && !d.goalDone;
        d.zoneContested = true;
        for (int t = 0; t < (int)(secs * 2.f / DT); ++t) { out.clear(); d.update(DT, 3, g.pos, out); }
        bool contestedNothing = d.goalTimer == 0.f && !d.goalDone && d.holding;
        d.zoneContested = false;
        bool refilled = false;
        for (int t = 0; t < (int)((secs + 1.f) / DT) && !d.goalDone; ++t) { out.clear(); d.update(DT, 3, g.pos, out); refilled |= d.queued() > 0; }
        CHECK(outsideNothing && contestedNothing && d.goalDone, "HOLD fills only while you stand in the circle and no enemy does");
        CHECK(refilled, "a goal wave keeps its queue topped up until the goal is met");
    }

    // ---------------------------------------------------------------- arenas that change
    {
        LevelData M = buildLevel();
        ArenaShifts sh; sh.capture(M);
        auto find = [&](ArenaShift k) { for (int i = 0; i < (int)M.arenas.size(); ++i) if (M.arenas[i].shift == k) return i; return -1; };
        int yard = find(ArenaShift::NIGHTFALL), foundry = find(ArenaShift::LAVA_RISE), spire = find(ArenaShift::SPEED_UP), core = find(ArenaShift::OVERLOAD);
        CHECK(yard >= 0 && foundry >= 0 && spire >= 0 && core >= 0, "the Yard, Foundry, Spire and Core each change as their fight goes on");

        // Lava: nothing until the last wave, then the channels spread 3 m each side; reset puts them back
        auto lavaArea = [&]() { float A = 0.f; for (auto& h : M.hazards) A += (h.box.max.x - h.box.min.x) * (h.box.max.z - h.box.min.z); return A; };
        float before = lavaArea();
        sh.onWave(M, foundry, 0, WaveGoal{}, 0.f);
        for (int i = 0; i < 600; ++i) sh.update(DT, M, foundry, true);
        bool still = lavaArea() == before;
        sh.onWave(M, foundry, (int)M.arenas[foundry].waves.size() - 1, WaveGoal{}, 0.f);
        bool started = false;
        for (int i = 0; i < 60 * 8; ++i) { sh.update(DT, M, foundry, true); started |= sh.lavaStarted; }
        float widened = lavaArea();
        CHECK(still && started && widened > before * 2.5f, "the Foundry's lava stays put until the last wave, then spreads");
        bool spawnsDry = true;
        for (auto& h : M.hazards)
            for (int wv = 0; wv < 3; ++wv)
                for (auto& p : M.arenas[foundry].groundSpawns)
                    if (p.x > h.box.min.x && p.x < h.box.max.x && p.z > h.box.min.z && p.z < h.box.max.z && p.y < 1.f) spawnsDry = false;
        CHECK(spawnsDry, "no Foundry spawn point ends up in the widened lava");
        sh.reset(M);
        CHECK(lavaArea() == before, "a retry puts the lava back");

        // Platforms: faster, without jumping
        float t = 37.3f;
        M.updateMovers(t);
        std::vector<glm::vec3> at; for (auto& m : M.movers) at.push_back(m.offsetAt(t));
        sh.onWave(M, spire, 2, WaveGoal{}, t);
        bool smooth = true, faster = false;
        for (int i = 0; i < (int)M.movers.size(); ++i) {
            if (glm::length(M.movers[i].offsetAt(t) - at[i]) > 1e-3f) smooth = false;
            faster |= M.movers[i].period < 0.99f * buildLevel().movers[i].period;
        }
        CHECK(smooth && faster && sh.speed[spire] > 1.5f, "the Spire's platforms speed up without a jump");
        sh.reset(M);

        // Overload: rings on the SURVIVE wave only; they hit feet on the floor, not on a walkway
        int sw = -1;
        for (int i = 0; i < (int)M.arenas[core].goals.size(); ++i) if (M.arenas[core].goals[i].kind == WaveGoal::SURVIVE) sw = i;
        sh.onWave(M, core, 0, WaveGoal{}, 0.f);
        for (int i = 0; i < 60 * 10; ++i) sh.update(DT, M, core, true);
        bool quietFirst = sh.rings.empty();
        sh.onWave(M, core, sw, M.arenas[core].goals[sw], 0.f);
        glm::vec3 floorP = M.reactorPos + glm::vec3{12.f, 0.f, 0.f}; floorP.y = M.arenas[core].playerStart.y;
        glm::vec3 high = floorP + glm::vec3{0.f, 4.f, 0.f};
        int floorHits = 0, highHits = 0, fired = 0;
        for (int i = 0; i < 60 * 12; ++i) {
            sh.update(DT, M, core, true);
            fired += sh.pulseFired; floorHits += sh.ringHits(floorP); highHits += sh.ringHits(high);
        }
        std::printf("      overload: %d rings in 12 s, floor hit %d times, walkway %d\n", fired, floorHits, highHits);
        CHECK(quietFirst && fired >= 2 && floorHits == fired && highHits == 0,
              "the Core's overload rings hit once each on the floor and pass under the walkways");
        sh.onWaveOver();
        for (int i = 0; i < 60 * 6; ++i) sh.update(DT, M, core, true);
        CHECK(sh.rings.empty() && sh.alarm == 0.f, "the overload stops when its wave does");

        // Nightfall: darker each wave; a retry brings the sun back
        Theme day = M.arenas[yard].theme;
        sh.onWave(M, yard, 2, WaveGoal{}, 0.f);
        for (int i = 0; i < 60 * 6; ++i) sh.update(DT, M, yard, true);
        bool darker = glm::length(M.arenas[yard].theme.lightColor) < glm::length(day.lightColor) * 0.7f && M.arenas[yard].theme.sunDir.y < 0.f;
        sh.reset(M);
        CHECK(darker && M.arenas[yard].theme.sunDir == day.sunDir, "the Yard's sun sets by the last wave, and rises again on a retry");
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
              "the boss wave stays active while the boss is alive");
    }

    // ---------------------------------------------------------------- director: FAST
    {
        WaveDirector d; d.level = &D; d.fast = true;
        d.approach(0);
        std::map<DirectorEvent, int> counts;
        int spawned = 0, expected = 0, sectionsSeen = 0;
        bool allAtOnce = true, exact = true, waitedForTrigger = true;
        for (auto& a : D.arenas) for (auto& w : a.waves) for (auto& e : w) expected += e.total();
        int alive = 0, killIn = 0, idle = 0;
        // The player stands at each level's breather for a while, then walks into its trigger
        for (int tick = 0; tick < 60 * 60 * 10 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
            std::vector<SpawnRequest> out;
            int sec = d.arena, wave = d.wave;
            glm::vec3 where = D.arenas[d.arena].playerStart;
            if (d.phase == WaveDirector::Phase::APPROACH) ++idle;
            if (d.phase == WaveDirector::Phase::APPROACH && idle > 120) {
                const AABB& t = D.arenas[d.arena].trigger;
                where = (t.min + t.max) * 0.5f;
                where.y = std::max(t.min.y + 0.5f, std::min(t.max.y - 0.5f, D.arenas[d.arena].playerStart.y));
            }
            d.update(DT, alive, where, out);
            if (!out.empty()) {
                if (wave == 0 && idle <= 120) waitedForTrigger = false;   // later waves follow straight on
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
            if (killIn > 0 && --killIn == 0) { alive = 0; idle = 0; }
            for (auto& ev : d.events) { counts[ev.kind]++; if (ev.kind == DirectorEvent::ARENA_START) ++sectionsSeen; }
            d.events.clear();
        }
        std::printf("      FAST: spawned %d of %d across %d levels\n", spawned, expected, sectionsSeen);
        CHECK(d.phase == WaveDirector::Phase::VICTORY, "a simulated FAST run clears every level");
        CHECK(spawned == expected, "every hand-placed enemy spawns exactly once");
        CHECK(allAtOnce, "a FAST wave arrives all at once, not trickled");
        CHECK(exact, "FAST enemies appear exactly at their hand-placed points");
        CHECK(waitedForTrigger, "a level's fight waits until you reach its trigger (the breather is safe)");
        CHECK(sectionsSeen == (int)D.arenas.size(), "every level's fight starts once");
        CHECK(counts[DirectorEvent::FINISH_OPEN] == 1 && counts[DirectorEvent::VICTORY] == 0,
              "clearing the last level opens the finish (the beacon ends the run)");
    }

    // ---------------------------------------------------------------- weapons + progression
    {
        WeaponUpgrades none;
        float maxRegular = 0.f;
        for (int t = 0; t < (int)EnemyType::COUNT; ++t)
            if (!isBoss((EnemyType)t) && (EnemyType)t != EnemyType::JUGGERNAUT)   // the heavies: parry them
                maxRegular = std::max(maxRegular, statsOf((EnemyType)t).health);
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
    {
        Leaderboard lb;
        auto timed = [](float t) { Leaderboard::Entry e; e.time = t; return e; };
        auto scored = [](int sc, float t = 600.f) { Leaderboard::Entry e; e.score = sc; e.time = t; return e; };
        CHECK(Leaderboard::cleanName("  ollie <3!!  ") == "OLLIE 3" && Leaderboard::cleanName("abcdefghijklmnopq").size() == 12,
              "leaderboard names are upper-cased, filtered to what the font draws, and capped at 12");
        CHECK(lb.add(Board::FAST, "   ", timed(100.f)) == -1 && lb.list(Board::FAST).empty(), "a blank name is not saved");
        lb.add(Board::FAST, "B", timed(200.f)); lb.add(Board::FAST, "A", timed(100.f)); lb.add(Board::FAST, "C", timed(300.f));
        CHECK(lb.list(Board::FAST).size() == 3 && lb.list(Board::FAST)[0].name == "A" && lb.list(Board::FAST)[2].name == "C" &&
              lb.list(Board::ARENA).empty(), "FAST sorts fastest first, per mode");
        for (int i = 0; i < 20; ++i) lb.add(Board::FAST, "X", timed(50.f + i));
        CHECK((int)lb.list(Board::FAST).size() == Leaderboard::KEEP && lb.placeFor(Board::FAST, timed(1000.f)) == -1 &&
              lb.placeFor(Board::FAST, timed(1.f)) == 0, "the board keeps the top 10; a slower time doesn't place");
        lb.add(Board::ARENA, "LOW", scored(4000)); lb.add(Board::ARENA, "HIGH", scored(9000, 900.f)); lb.add(Board::ARENA, "MID", scored(6000, 300.f));
        CHECK(lb.list(Board::ARENA)[0].name == "HIGH" && lb.list(Board::ARENA)[2].name == "LOW",
              "ARENA ranks by score, not time: a slower run that scored more is ahead");
        CHECK(lb.add(Board::ENDLESS, "ZERO", scored(0)) == -1, "a run that scored nothing isn't saved");
    }
    {
        // The combined score
        RunScore a = arenaScore(5000.f, 600.f, 300.f, 1);
        CHECK(a.style == 5000 && a.time == 3000 && a.damage == 600 && a.total == 7400, "ARENA score = style + time under par - damage");
        RunScore slow = arenaScore(5000.f, 1200.f, 300.f, 1), hard = arenaScore(5000.f, 600.f, 300.f, 3);
        CHECK(slow.time == 0 && slow.total == 4400 && hard.total == 11100, "no time bonus past par; harder difficulties multiply it");
        CHECK(arenaScore(100.f, 2000.f, 5000.f, 1).total == 0, "a score never goes below zero");
        RunScore e = endlessScore(2000.f, 7, 2);
        CHECK(e.waves == 3500 && e.total == (int)std::lround(5500 * 1.25f), "ENDLESS score = style + 500 a wave, x difficulty");
    }
    {
        // The daily challenge: the same date is always the same run; days differ
        DailyInfo d1 = DailyInfo::forDate(20261005), d2 = DailyInfo::forDate(20261005), d3 = DailyInfo::forDate(20261006);
        bool variety = false;
        for (int day = 1; day <= 28; ++day) {
            DailyInfo x = DailyInfo::forDate(20261100 + day);
            if (x.arena != d1.arena || x.mod != d1.mod) variety = true;
            if (x.arena < 0 || x.arena > 3) variety = false;
        }
        CHECK(d1.seed == d2.seed && d1.arena == d2.arena && d1.mod == d2.mod && d1.seed != d3.seed && variety,
              "a DAILY is the same for everyone on a date, and changes from day to day");
        CHECK(d1.key() == "20261005" && d1.label() == "2026-10-05", "a DAILY's board is named by its date");
        int today = DailyInfo::todayUtc();
        CHECK(today > 20250000 && today < 21000000, "today's date (UTC) reads as YYYYMMDD");
    }
    {
        // ENDLESS waves: deterministic from the seed, growing, unlocking, with
        // an objective every 4th wave and a heavy wave every 10th
        LevelData M = buildLevel();
        EndlessWaves g1, g2;
        g1.begin(1234u, M.arenas[3].goals, true); g2.begin(1234u, M.arenas[3].goals, true);
        bool same = true, grows = true, unlockOk = true, goalsOk = true, heavyOk = true;
        int prevTotal = 0;
        for (int n = 0; n < 30; ++n) {
            auto a = g1.wave(n), b = g2.wave(n);
            int ta = 0, tb = 0;
            for (auto& e : a.first) ta += e.total();
            for (auto& e : b.first) tb += e.total();
            if (ta != tb || a.first.size() != b.first.size() || a.second.kind != b.second.kind) same = false;
            for (auto& e : a.first) {
                if (n < 4 && (e.type == EnemyType::BRUTE || e.type == EnemyType::JUGGERNAUT || e.type == EnemyType::CONDUCTOR)) unlockOk = false;
                if (n < 7 && e.type == EnemyType::JUGGERNAUT && n % 10 != 9) unlockOk = false;
            }
            bool isGoal = a.second.kind != WaveGoal::KILL_ALL;
            if (isGoal != (n % 4 == 3 && n % 10 != 9)) goalsOk = false;
            bool warden = false;
            for (auto& e : a.first) warden |= e.type == EnemyType::WARDEN;
            if (warden != (n % 10 == 9)) heavyOk = false;
            if (n % 10 != 9 && n >= 2 && n % 4 != 3 && ta + 3 < prevTotal / 2) grows = false;
            if (n % 10 != 9) prevTotal = ta;
        }
        std::printf("      endless wave 1: %d enemies, wave 20: %d\n", [&]{ EndlessWaves g; g.begin(1234u, {}, true); int t = 0; for (auto& e : g.wave(0).first) t += e.total(); return t; }(),
                    [&]{ EndlessWaves g; g.begin(1234u, {}, true); for (int i = 0; i < 19; ++i) g.wave(i); int t = 0; for (auto& e : g.wave(19).first) t += e.total(); return t; }());
        CHECK(same, "the same seed gives the same ENDLESS waves");
        CHECK(unlockOk && grows, "ENDLESS waves grow, and the heavies only turn up later");
        CHECK(goalsOk && heavyOk, "every 4th ENDLESS wave is an objective; every 10th brings the Warden (in the Core)");
        EndlessWaves y; y.begin(99u, M.arenas[0].goals, false);
        for (int i = 0; i < 9; ++i) y.wave(i);
        auto heavy = y.wave(9);
        bool jugg = false, noWarden = true;
        for (auto& e : heavy.first) { jugg |= e.type == EnemyType::JUGGERNAUT; noWarden &= e.type != EnemyType::WARDEN; }
        CHECK(jugg && noWarden, "outside the Core the heavy wave is Juggernauts, not the Warden");

        // Fed one wave ahead, the director never runs out or clears the arena
        Arena& core = M.arenas[3];
        core.waves.clear(); core.goals.clear();
        EndlessWaves g; g.begin(7u, buildLevel().arenas[3].goals, true);
        WaveDirector d; d.level = &M;
        d.startArena(3);
        int waveStarts = 0; bool cleared = false;
        std::vector<float> alive;
        for (int tick = 0; tick < 60 * 60 * 12 && waveStarts < 12; ++tick) {
            while ((int)core.waves.size() < d.wave + 2) { auto w = g.wave((int)core.waves.size()); core.waves.push_back(w.first); core.goals.push_back(w.second); }
            std::vector<SpawnRequest> out;
            d.update(DT, (int)alive.size(), d.goal().kind == WaveGoal::HOLD ? d.goal().pos : core.playerStart, out);
            for (auto& r : out) { (void)r; alive.push_back(1.f); }
            for (auto& t : alive) t -= DT;
            alive.erase(std::remove_if(alive.begin(), alive.end(), [](float t) { return t <= 0.f; }), alive.end());
            for (auto& ev : d.events) {
                if (ev.kind == DirectorEvent::WAVE_START || ev.kind == DirectorEvent::BOSS_START) ++waveStarts;
                if (ev.kind == DirectorEvent::ARENA_CLEARED || ev.kind == DirectorEvent::VICTORY) cleared = true;
                if (ev.kind == DirectorEvent::GOAL_DONE) alive.clear();
            }
            d.events.clear();
        }
        CHECK(waveStarts >= 12 && !cleared, "an ENDLESS run keeps going wave after wave and never clears its arena");
    }
    {
        // Style freshness: the same gun over and over scores less, switching restores it
        StyleSystem st;
        CHECK(st.freshness(StyleSource::REVOLVER) == Freshness::FRESH, "every source starts fresh");
        float first = st.addStyle(10.f, StyleSource::REVOLVER);
        CHECK(std::fabs(first - 15.f) < 1e-3f, "a fresh source scores x1.5");
        int kills = 0;
        while (st.freshness(StyleSource::REVOLVER) != Freshness::DULL && kills < 50) { st.addStyle(40.f, StyleSource::REVOLVER); ++kills; }
        CHECK(kills >= 4 && kills <= 9, "a gun goes dull after a handful of kills with nothing else");
        CHECK(st.addStyle(10.f, StyleSource::REVOLVER) < 2.5f, "a dull source scores x0.2");
        CHECK(st.freshness(StyleSource::SHOTGUN) == Freshness::FRESH && std::fabs(st.addStyle(10.f, StyleSource::SHOTGUN) - 15.f) < 1e-3f,
              "another gun is still fresh");
        float before = st.fresh[(int)StyleSource::REVOLVER];
        for (int k = 0; k < 6; ++k) st.addStyle(40.f, StyleSource::PARRY);
        CHECK(st.fresh[(int)StyleSource::REVOLVER] > before + 0.15f, "using other things freshens a worn source");
        float mid = st.fresh[(int)StyleSource::REVOLVER];
        st.update(4.f);
        CHECK(std::fabs(st.fresh[(int)StyleSource::REVOLVER] - std::min(1.f, mid + 0.2f)) < 1e-3f, "worn sources recover with time");
        StyleSystem pv;
        for (int k = 0; k < 40; ++k) { pv.addStyle(15.f, StyleSource::ENVIRONMENT); pv.addStyle(15.f, StyleSource::FRIENDLY); }
        CHECK(pv.style <= StyleSystem::PASSIVE_CAP + 1e-3f && !pv.overdrive, "lava and friendly-fire kills alone can't carry the meter past B");
        pv.addStyle(40.f, StyleSource::KAR);
        CHECK(pv.style > StyleSystem::PASSIVE_CAP, "your own kills take it on from there");
        float s0 = st.style; st.addStyle(8.f);
        CHECK(std::fabs(st.style - std::min(st.maxStyle, s0 + 8.f)) < 1e-3f, "movement style isn't tracked or scaled");
    }
    {
        // Friendly fire: a Juggernaut's siege shell hits the enemies in its path, but not its own Juggernaut or a boss
        auto ready = [](Enemy& e) { e.state = EnemyState::ACTIVE; e.spawnTimer = 0.f; };
        std::vector<Enemy> es;
        es.emplace_back(EnemyType::JUGGERNAUT, glm::vec3{0, 0, 0});
        es.emplace_back(EnemyType::HUSK, glm::vec3{0, 0, -6});
        for (auto& e : es) ready(e);
        ProjectileSystem ps;
        ps.fire({0, 1.f, -0.5f}, {0, 0, -13.f}, 20.f, false, {1, 1, 1}, false, 0.f, 3.f, true);
        bool hitHusk = false, hitJugg = false;
        for (int t = 0; t < 60 && !hitHusk; ++t) {
            auto r = ps.update(1.f / 60.f, nullptr, 0, es, glm::vec3{0, 1.6f, 40.f});
            for (auto& [pi, ei] : r.friendlyHits) { (ei == 1 ? hitHusk : hitJugg) = true; }
        }
        CHECK(hitHusk && !hitJugg, "a siege shell clears its own Juggernaut and hits the Husk in its way");
        std::vector<Enemy> boss;
        boss.emplace_back(EnemyType::WARDEN, glm::vec3{0, 0, -6});
        ready(boss[0]);
        ProjectileSystem ps2;
        ps2.fire({0, 1.f, -0.5f}, {0, 0, -13.f}, 20.f, false, {1, 1, 1}, false, 0.f, 3.f, true);
        bool hitBoss = false;
        for (int t = 0; t < 60; ++t) if (!ps2.update(1.f / 60.f, nullptr, 0, boss, glm::vec3{0, 1.6f, 40.f}).friendlyHits.empty()) hitBoss = true;
        CHECK(!hitBoss, "bosses shrug off friendly fire");
        ProjectileSystem ps3;
        ps3.fire({0, 1.f, -5.f}, {0, 0, -13.f}, 20.f, false);
        bool orbHit = false;
        for (int t = 0; t < 60; ++t) if (!ps3.update(1.f / 60.f, nullptr, 0, es, glm::vec3{0, 1.6f, 40.f}).friendlyHits.empty()) orbHit = true;
        CHECK(!orbHit, "ordinary enemy shots pass through their friends");
    }
    {
        GhostRun g;
        for (int i = 0; i <= 120; ++i) g.record(i * DT, {i * DT * 6.f, 0.f, 0.f}, 170.f + i * 0.2f);   // 2 s at 6 m/s
        glm::vec3 p; float yaw;
        bool ok = g.at(1.05f, p, yaw);
        CHECK(ok && std::fabs(g.duration() - 2.f) < 0.11f && std::fabs(p.x - 6.3f) < 0.05f,
              "the ghost records ten samples a second and plays back in between them");
        g.at(99.f, p, yaw);
        CHECK(std::fabs(p.x - 12.f) < 0.05f && !GhostRun{}.at(0.f, p, yaw), "it waits at the finish; no run, no ghost");
    }

    // ---------------------------------------------------------------- music
    {
        auto measure = [](int track, float level, bool muffled, float& rms, float& peak, float& hf) {
            MusicSynth m; m.setTrack(track); m.setIntensity(level); m.setVolume(1.f); m.setMuffle(muffled);
            std::vector<float> buf(44100 * 2 * 3);
            m.render(buf.data(), 44100 * 3);             // settle (track switch, layer fades)
            m.render(buf.data(), 44100 * 3);
            double s2 = 0, d2 = 0; peak = 0.f; bool finite = true;
            for (size_t i = 0; i < buf.size(); ++i) {
                s2 += buf[i] * buf[i]; peak = std::max(peak, std::fabs(buf[i]));
                if (i >= 2) { float d = buf[i] - buf[i - 2]; d2 += d * d; }   // first difference: high-frequency energy
                if (!std::isfinite(buf[i])) finite = false;
            }
            rms = (float)std::sqrt(s2 / buf.size()); hf = (float)std::sqrt(d2 / buf.size());
            if (!finite) peak = 99.f;
        };
        bool sane = true, layers = true;
        for (int t = 0; t < MUSIC_TRACKS; ++t) {
            float r0, p0, h0, r1, p1, h1;
            measure(t, 0.f, false, r0, p0, h0);
            measure(t, 1.f, false, r1, p1, h1);
            std::printf("      music %-7s calm rms %.3f, fight rms %.3f peak %.2f\n", musicTrack(t).name, r0, r1, p1);
            if (p0 > 1.f || p1 > 1.f || r1 < 0.08f || r1 > 0.6f) sane = false;
            if (!(r1 > r0 * 1.3f)) layers = false;
        }
        float r, p, h, rm, pm, hm;
        measure(0, 1.f, false, r, p, h);
        measure(0, 1.f, true, rm, pm, hm);
        CHECK(sane, "every soundtrack renders clean: finite, never clipping, at a sensible level");
        CHECK(layers, "the music gets bigger when a fight starts");
        CHECK(hm < h * 0.5f, "the pause menu muffles the music");
        MusicSynth m; m.setTrack(2);
        std::vector<float> buf(2 * 4410);
        m.render(buf.data(), 4410);
        CHECK(m.currentTrack() == 2, "a track change waits for the bar line, then takes");
    }

    // ---------------------------------------------------------------- sound effects mixer: core
    {
        using G = SoundGroup;
        auto opts = [](float vol, G g, bool prio = false) { SfxMixer::Opts o; o.volume = vol; o.group = g; o.priority = prio; return o; };
        {   // silence in, silence out
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            auto b = sfxRender(m, 44100);
            CHECK(sfxPeak(b) == 0.f, "no sounds playing: the mixer adds exactly nothing");
        }
        {   // unknown names and zero volume do nothing
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            SoundHandle a = m.play("nope", opts(1.f, G::WORLD));
            SoundHandle b = m.play("noise", opts(0.f, G::WORLD));
            sfxRender(m, 64);
            CHECK(a == 0 && b == 0 && m.active(G::WORLD) == 0, "an unknown sound or zero volume plays nothing (handle 0)");
        }
        {   // a full group steals its quietest voice
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            SoundHandle quiet = 0;
            for (int i = 0; i < 20; ++i) {
                SoundHandle h = m.play("noise", opts(i == 3 ? 0.05f : 0.5f, G::ENEMY));
                if (i == 3) quiet = h;
            }
            sfxRender(m, 64);
            bool full = m.active(G::ENEMY) == 20;
            SoundHandle fresh = m.play("noise", opts(0.5f, G::ENEMY));
            sfxRender(m, 64);
            CHECK(full && m.active(G::ENEMY) == 20 && !m.isPlaying(quiet) && m.isPlaying(fresh),
                  "a full group (ENEMY 20) gives its quietest voice to the new sound");
        }
        {   // priority voices are never stolen by a non-priority sound
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            std::vector<SoundHandle> hs;
            for (int i = 0; i < 20; ++i) hs.push_back(m.play("noise", opts(0.2f, G::ENEMY, true)));
            sfxRender(m, 64);
            SoundHandle extra = m.play("noise", opts(1.f, G::ENEMY));
            sfxRender(m, 64);
            bool all = true; for (auto h : hs) all &= m.isPlaying(h);
            CHECK(all && !m.isPlaying(extra), "a group full of priority voices (the boss) keeps them all");
        }
        {   // pitch jitter: varied, within bounds; UI tighter
            SfxMixer m(44100.f, 11); m.addSound("noise", sfxNoise(441));
            float lo = 9.f, hi = 0.f, uiLo = 9.f, uiHi = 0.f;
            for (int i = 0; i < 1000; ++i) {
                m.play("noise", opts(1.f, G::WORLD)); lo = std::min(lo, m.lastPlayPitch()); hi = std::max(hi, m.lastPlayPitch());
                m.play("noise", opts(1.f, G::UI));    uiLo = std::min(uiLo, m.lastPlayPitch()); uiHi = std::max(uiHi, m.lastPlayPitch());
                if (i % 200 == 0) sfxRender(m, 16);
            }
            CHECK(lo >= 0.96f && hi <= 1.04f && hi - lo > 0.05f, "every play gets a pitch within +-4% (and they differ)");
            CHECK(uiLo >= 0.99f && uiHi <= 1.01f, "UI sounds vary by at most +-1%");
        }
        {   // variants: never the same one twice running
            SfxMixer m(44100.f, 5);
            m.addSound("step", sfxNoise(100, 1)); m.addSound("step", sfxNoise(100, 2)); m.addSound("step", sfxNoise(100, 3));
            bool noRepeat = m.variants("step") == 3; int last = -1; bool usedAll[3] = {};
            for (int i = 0; i < 60; ++i) {
                m.play("step", opts(1.f, G::PLAYER));
                int v = m.lastPlayVariant();
                noRepeat &= v != last && v >= 0 && v < 3; last = v; if (v >= 0 && v < 3) usedAll[v] = true;
                sfxRender(m, 8);
            }
            CHECK(noRepeat && usedAll[0] && usedAll[1] && usedAll[2], "a sound with variants picks among them, never the same twice running");
        }
        {   // overload: 48 full-scale voices stay within +-1 and finite
            SfxMixer m(44100.f, 3); m.addSound("loud", sfxSquare(44100));
            for (int g = 0; g < 4; ++g) for (int i = 0; i < 20; ++i) m.play("loud", opts(1.f, (G)g));
            auto b = sfxRender(m, 8192, 0.9f);
            CHECK(sfxFinite(b) && sfxPeak(b) <= 1.f, "48 full-scale voices over loud music: never past +-1, never NaN");
        }
        {   // the command ring: a flood drops, later commands still work
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            for (int i = 0; i < 600; ++i) m.move(12345, {0.f, 0.f, 0.f});
            sfxRender(m, 64);
            SoundHandle h = m.play("noise", opts(1.f, G::WORLD));
            sfxRender(m, 64);
            CHECK(h != 0 && m.isPlaying(h), "a flooded command ring drops the overflow; the next play still lands");
        }
        {   // a sound ends when its sample does
            SfxMixer m(44100.f, 3); m.addSound("blip", sfxNoise(100));
            SoundHandle h = m.play("blip", opts(1.f, G::WORLD));
            sfxRender(m, 512);
            CHECK(!m.isPlaying(h) && m.active(G::WORLD) == 0, "a short sound frees its voice when it finishes");
        }
        // ---- positional
        auto at = [](glm::vec3 p, float vol = 1.f, G g = G::WORLD) {
            SfxMixer::Opts o; o.volume = vol; o.group = g; o.positional = true; o.pos = p; return o;
        };
        auto lisN = [](SfxMixer& m) { m.setListener({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, -1.f}); };   // facing -Z, right = +X
        {
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); lisN(m);
            m.play("noise", at({10.f, 0.f, 0.f}));
            auto r = sfxRender(m, 4410);
            SfxMixer m2(44100.f, 3); m2.addSound("noise", sfxNoise(44100)); lisN(m2);
            m2.play("noise", at({-10.f, 0.f, 0.f}));
            auto l = sfxRender(m2, 4410);
            CHECK(sfxRms(r, 1) > 4.f * sfxRms(r, 0) && sfxRms(l, 0) > 4.f * sfxRms(l, 1),
                  "a sound on your right is in the right ear, on your left in the left");
        }
        {
            SfxMixer f(44100.f, 3); f.addSound("noise", sfxNoise(44100)); lisN(f);
            f.play("noise", at({0.f, 0.f, -10.f}));
            auto front = sfxRender(f, 4410);
            SfxMixer b(44100.f, 3); b.addSound("noise", sfxNoise(44100)); lisN(b);
            b.play("noise", at({0.f, 0.f, 10.f}));
            auto behind = sfxRender(b, 4410);
            CHECK(sfxHf(behind, 0) / sfxRms(behind, 0) < 0.9f * sfxHf(front, 0) / sfxRms(front, 0),
                  "the same sound behind you is darker than in front");
        }
        {
            float rms[4]; const float D[4] = {4.f, 15.f, 40.f, 95.f};
            for (int k = 0; k < 4; ++k) {
                SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); lisN(m);
                m.play("noise", at({0.f, 0.f, -D[k]}));
                auto b = sfxRender(m, 4410);
                rms[k] = k == 3 ? sfxPeak(b) : sfxRms(b, 0);
            }
            CHECK(rms[0] > rms[1] && rms[1] > rms[2] && rms[2] > 0.f, "quieter the further away (4, 15, 40 m)");
            CHECK(rms[3] == 0.f, "beyond 90 m: silent");
        }
        {   // on top of the listener: centred, full, finite
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); lisN(m);
            m.play("noise", at({0.f, 0.f, 0.f}));
            auto b = sfxRender(m, 4410);
            float l = sfxRms(b, 0), r = sfxRms(b, 1);
            CHECK(sfxFinite(b) && l > 0.3f && std::fabs(l - r) < 0.01f * l, "a sound right on you is centred and full (no NaN)");
        }
        {   // a moving source follows; a stale handle moves nothing
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); m.addSound("blip", sfxNoise(100)); lisN(m);
            SoundHandle old = m.play("blip", at({0.f, 0.f, -2.f}));
            sfxRender(m, 512);
            SoundHandle h = m.play("noise", at({10.f, 0.f, 0.f}));
            m.move(old, {-10.f, 0.f, 0.f});   // finished long ago: must not touch the new voice
            auto first = sfxRender(m, 4410);
            m.move(h, {-10.f, 0.f, 0.f});
            sfxRender(m, 512);                // the swing across
            auto second = sfxRender(m, 4410);
            CHECK(sfxRms(first, 1) > 4.f * sfxRms(first, 0), "a stale handle moves nothing");
            CHECK(sfxRms(second, 0) > 4.f * sfxRms(second, 1), "a moved source is heard where it went");
        }
        // ---- duck and reverb
        {
            SfxMixer m(44100.f, 3);
            sfxRender(m, 4410, 0.25f);                                // under the bus compressor's threshold
            m.duck(6.f, 0.1f);
            auto during = sfxRender(m, 2205, 0.25f);                 // 50 ms in
            float dipped = during[during.size() - 2];
            sfxRender(m, 88200, 0.25f);                               // hold 0.1 s, release 250 ms
            auto after = sfxRender(m, 441, 0.25f);
            CHECK(std::fabs(dipped - 0.25f * 0.501f) < 0.015f, "a 6 dB duck dips the music to half within 50 ms");
            CHECK(std::fabs(after.back() - 0.25f) < 0.0025f, "the music comes back once the duck is over");
            SfxMixer o(44100.f, 3);
            o.duck(3.f, 0.5f); o.duck(9.f, 0.1f);
            sfxRender(o, 4410, 0.5f);
            CHECK(std::fabs(o.duckLevel() - 0.355f) < 0.03f, "overlapping ducks take the deepest");
        }
        {
            auto tail = [](ReverbSpace s) {
                SfxMixer m(44100.f, 3); m.addSound("blip", sfxNoise(2205));
                m.setSpace(s);
                sfxRender(m, 66150);                                  // let the space settle (1 s crossfade)
                m.play("blip", SfxMixer::Opts{1.f, SoundGroup::WORLD});
                sfxRender(m, 4410);                                   // the blip (50 ms) and its first echoes
                return sfxRms(sfxRender(m, 22050), 0);                // the 0.1-0.6 s tail
            };
            float open = tail(ReverbSpace::OPEN), hall = tail(ReverbSpace::HALL);
            CHECK(hall > 3.f * open && open >= 0.f, "a HALL rings on far longer than the OPEN yard");
            SfxMixer u(44100.f, 3); u.addSound("blip", sfxNoise(2205)); u.setSpace(ReverbSpace::HALL);
            sfxRender(u, 66150);
            u.play("blip", SfxMixer::Opts{1.f, SoundGroup::UI});
            sfxRender(u, 4410);
            CHECK(sfxPeak(sfxRender(u, 22050)) == 0.f, "UI sounds stay dry (no reverb)");
        }
        {   // a 48 kHz device (browsers): same behaviour, silence still silent
            SfxMixer m(48000.f, 3); m.addSound("noise", sfxNoise(48000)); m.setSpace(ReverbSpace::SHAFT);
            bool silent = sfxPeak(sfxRender(m, 48000)) == 0.f;
            m.setListener({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, -1.f});
            SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = {10.f, 0.f, 0.f};
            m.play("noise", o);
            auto b = sfxRender(m, 4800);
            CHECK(silent && sfxFinite(b) && sfxRms(b, 1) > 3.f * sfxRms(b, 0), "at 48 kHz: silent when idle, panned when not");
        }
        {   // after a sound dies away the reverb settles to true zero (subnormals cost x86 / wasm dearly)
            SfxMixer m(44100.f, 3); m.addSound("blip", sfxNoise(2205)); m.setSpace(ReverbSpace::HALL);
            sfxRender(m, 66150);
            m.play("blip", SfxMixer::Opts{1.f, SoundGroup::WORLD});
            for (int k = 0; k < 60; ++k) sfxRender(m, 44100);
            CHECK(m.reverbSubnormals() == 0, "a minute after a sound, the reverb holds no denormals");
        }
        {   // a NaN anywhere (listener, music) never kills the audio, even built with -ffast-math
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            float nan = std::numeric_limits<float>::quiet_NaN();
            m.setListener({nan, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, -1.f});
            SfxMixer::Opts o; o.group = SoundGroup::WORLD; o.positional = true; o.pos = {3.f, 0.f, 0.f};
            m.play("noise", o);
            auto bad = sfxRender(m, 4410, nan);
            m.setListener({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, -1.f});
            m.play("noise", o);
            auto good = sfxRender(m, 4410);
            CHECK(sfxFinite(bad) && sfxPeak(bad) <= 1.f, "NaN in (listener or music): finite, bounded out");
            CHECK(sfxFinite(good) && sfxRms(good, 1) > 0.05f, "...and the next sound plays normally");
        }
        {   // a level floor keeps a must-hear cue (a kill far away) audible
            auto lvl = [](float d, float floor) {
                SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
                m.setListener({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, -1.f});
                SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = {0.f, 0.f, -d}; o.floor = floor;
                m.play("noise", o);
                return sfxRms(sfxRender(m, 4410), 0);
            };
            float near = lvl(4.f, 0.f), far = lvl(60.f, 0.f), floored = lvl(60.f, 0.5f), beyond = lvl(120.f, 0.5f);
            CHECK(far < 0.15f * near && floored > 0.4f * near, "a floored cue at 60 m stays at least half as loud as up close");
            CHECK(beyond > 0.3f * near, "...even past 90 m");
        }
    }

    // ---------------------------------------------------------------- sound effects mixer: roles and the bus
    {
        using G = SoundGroup; using R = SoundRole;
        auto ro = [](R role, float vol = 0.5f, bool prio = false) {
            SfxMixer::Opts o; o.volume = vol; o.group = SoundGroup::ENEMY; o.role = role; o.priority = prio; return o;
        };
        {   // chatter: at most 6 at once, a 7th takes the oldest
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            std::vector<SoundHandle> hs;
            for (int i = 0; i < 6; ++i) { hs.push_back(m.play("noise", ro(R::CHATTER))); sfxRender(m, 16); }
            SoundHandle seventh = m.play("noise", ro(R::CHATTER)); sfxRender(m, 16);
            CHECK(m.roleActive(R::CHATTER) == 6 && !m.isPlaying(hs[0]) && m.isPlaying(hs[1]) && m.isPlaying(seventh),
                  "enemy chatter: 6 voices at most, a 7th takes the oldest");
        }
        {   // chatter piling up never takes a tell
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            SoundHandle t = m.play("noise", ro(R::TELL, 0.05f, true)); sfxRender(m, 16);
            for (int i = 0; i < 8; ++i) { m.play("noise", ro(R::CHATTER)); sfxRender(m, 16); }
            CHECK(m.isPlaying(t) && m.roleActive(R::TELL) == 1 && m.roleActive(R::CHATTER) == 6,
                  "chatter piling up never silences a tell");
        }
        {   // a full ENEMY group: a tell takes chatter, never a (quieter) tell
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            for (int i = 0; i < 15; ++i) m.play("noise", ro(R::TELL, 0.01f, true));
            for (int i = 0; i < 5; ++i) m.play("noise", ro(R::CHATTER, 0.9f));
            sfxRender(m, 16);
            SoundHandle t = m.play("noise", ro(R::TELL, 0.5f, true)); sfxRender(m, 16);
            CHECK(m.active(G::ENEMY) == 20 && m.isPlaying(t) && m.roleActive(R::TELL) == 16 && m.roleActive(R::CHATTER) == 4,
                  "a tell in a full group takes a chatter voice, not a quieter tell");
        }
        {   // the sidechain: while a tell plays chatter dips 8 dB and the music 3 dB; both come back after
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            m.addSound("hush", std::vector<float>(13230, 0.f));   // a silent 0.3 s tell
            m.play("noise", ro(R::CHATTER));
            m.play("hush", ro(R::TELL, 1.f, true));
            sfxRender(m, 8820, 0.1f);   // 0.2 s
            float chatDb = 20.f * std::log10(m.chatterGain()), musDb = 20.f * std::log10(m.musicGain());
            CHECK(std::fabs(chatDb + 3.f) < 0.5f && std::fabs(musDb) < 0.05f, "while a tell plays, chatter dips 3 dB and the music is left alone");
            sfxRender(m, 4410); sfxRender(m, 26460);   // the tell ends at 0.3 s, then 0.6 s more
            CHECK(m.chatterGain() > 0.9f && m.musicGain() > 0.95f, "...and both come back within about half a second after it");
        }
        {   // the music really is 3 dB down under a tell
            SfxMixer m(44100.f, 3); m.addSound("hush", std::vector<float>(44100, 0.f));
            m.play("hush", ro(R::TELL, 1.f, true));
            auto b = sfxRender(m, 8820, 0.1f);
            CHECK(std::fabs(std::fabs(b[b.size() - 2]) - 0.1f) < 0.002f, "the music under a tell is untouched");
        }
        {   // nothing pumps the mix: loud music passes at its own level (only the soft clip above 0.8)
            SfxMixer m(44100.f, 3);
            auto b = sfxRender(m, 22050, 0.6f);
            CHECK(std::fabs(b[b.size() - 2] - 0.6f) < 1e-4f, "loud music passes through at its own level: nothing pumps the mix");
        }
        {   // a 22.05 kHz sound plays for its real length
            SfxMixer m(44100.f, 3); m.addSound("half", std::vector<float>(2205, 0.3f), 22050.f);   // 0.1 s
            SfxMixer::Opts o; o.group = G::UI;
            SoundHandle h = m.play("half", o);
            sfxRender(m, 4300); bool mid = m.isPlaying(h);
            sfxRender(m, 200);  bool done = !m.isPlaying(h);
            CHECK(mid && done, "a sound built at 22.05 kHz plays for its real length at 44.1 kHz");
        }
        {   // pitch: twice the rate, half the length
            SfxMixer m(44100.f, 3); m.addSound("dc", std::vector<float>(4410, 0.3f));
            SfxMixer::Opts o; o.group = G::UI; o.pitch = 2.f;
            SoundHandle h = m.play("dc", o);
            sfxRender(m, 2300);
            CHECK(!m.isPlaying(h), "a sound at pitch 2 lasts half as long");
        }
        {   // delay: silent until it's due
            SfxMixer m(44100.f, 3); m.addSound("dc", std::vector<float>(44100, 0.5f));
            SfxMixer::Opts o; o.group = G::UI; o.delay = 0.012f;
            m.play("dc", o);
            auto b = sfxRender(m, 1024);
            CHECK(b[2 * 500] == 0.f && std::fabs(b[2 * 1000]) > 0.1f, "a delayed sound (a Twinned echo) starts 12 ms late");
        }
        {   // drive: saturated (more edge), never louder
            std::vector<float> sine(44100);
            for (int i = 0; i < 44100; ++i) sine[i] = 0.6f * std::sin(6.2831853f * 220.f * i / 44100.f);
            auto run = [&](float drive) {
                SfxMixer m(44100.f, 3); m.addSound("sine", sine);
                SfxMixer::Opts o; o.group = G::UI; o.drive = drive; o.volume = 0.5f;
                m.play("sine", o); return sfxRender(m, 8192);
            };
            auto clean = run(0.f), driven = run(1.f);
            CHECK(sfxPeak(driven) <= sfxPeak(clean) + 1e-4f && sfxHf(driven, 0) / sfxRms(driven, 0) > 1.05f * sfxHf(clean, 0) / sfxRms(clean, 0),
                  "drive (an Enraged voice) adds edge without adding level");
        }
        {   // dev solo: only the chosen roles, no music
            SfxMixer m(44100.f, 3); m.addSound("dc", std::vector<float>(44100, 0.2f));
            m.devSolo(1 << (int)R::TELL);
            m.play("dc", ro(R::CHATTER, 1.f));
            auto b = sfxRender(m, 512, 0.1f);
            CHECK(sfxPeak(b) == 0.f, "soloing tells mutes chatter and the music");
        }
        {   // reading the bank back (the mix report)
            SfxMixer m(44100.f, 3); m.addSound("a", std::vector<float>(10, 0.1f), 22050.f); m.addSound("a", std::vector<float>(20, 0.1f), 22050.f);
            CHECK(m.sample("a", 1) && m.sample("a", 1)->size() == 20 && !m.sample("a", 2) && !m.sample("b", 0) && m.sampleRateOf("a") == 22050.f,
                  "the bank can be read back: each variant and its source rate");
        }
    }

    // ---------------------------------------------------------------- enemy voices: the bank
    {
        const auto& bank = voiceBank();
        std::map<std::string, int> seen;
        bool unique = true;
        for (auto& s : bank) unique &= seen[s.name]++ == 0;
        CHECK(unique, "every voice has its own name");
        bool table = true;
        for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
            EnemyType t = (EnemyType)i;
            for (VoiceKind k : {VoiceKind::SPAWN, VoiceKind::IDLE, VoiceKind::HURT, VoiceKind::DEATH}) table &= seen.count(voiceName(t, k)) > 0;
            if (moveCadence(t) > 0.f) table &= seen.count(voiceName(t, VoiceKind::MOVE)) > 0;
            for (AttackKind a : attacksOf(t)) {
                table &= seen.count(voiceName(t, VoiceKind::TELL, a)) > 0;
                if (hasRelease(a)) table &= seen.count(voiceName(t, VoiceKind::ATTACK, a)) > 0;
            }
        }
        CHECK(table, "every enemy type has spawn, idle, hurt and death voices, steps if it moves, a tell (and release) per attack");

        // Every attack an enemy really starts has a tell: run each type against a player at a few ranges
        bool covered = true;
        for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
            EnemyType t = (EnemyType)i;
            auto known = attacksOf(t);
            for (float dist : {3.f, 8.f, 14.f, 24.f}) {
                Enemy e(t, {0.f, 0.f, 0.f});
                EnemyWorld w; w.playerFeet = {dist, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
                for (int f = 0; f < 60 * 40 && e.alive; ++f) {
                    if (f == 60 * 20) e.health = e.maxHealth * 0.2f;   // bosses: their late-fight moves too
                    e.update(DT, w);
                    if (e.ev.telegraphStarted && std::find(known.begin(), known.end(), e.attack) == known.end()) {
                        covered = false;
                        std::printf("      %s starts attack %d with no tell\n", statsOf(t).name, (int)e.attack);
                    }
                }
            }
        }
        CHECK(covered, "every attack an enemy starts has a tell of its own");

        float total = 0.f; bool built = true, levels = true, varied = true, same = true;
        int idx = 0;
        for (const auto& s : bank) {
            std::vector<float> first;
            for (int v = 0; v < s.variants; ++v) {
                auto b = VoiceSynth::build(s, v);
                total += b.size() / VoiceSynth::RATE;
                float pk = 0.f; bool fin = true;
                for (float x : b) { pk = std::max(pk, std::fabs(x)); fin &= std::isfinite(x); }
                float lv = shortTermDb(b.data(), b.size(), VoiceSynth::RATE);
                if (b.size() < 200 || !fin || pk > 0.951f) { built = false; std::printf("      %s/%d: %zu samples, peak %.2f\n", s.name.c_str(), v, b.size(), pk); }
                if (std::fabs(lv - mixTargetDb(s.cls)) > 2.f) { levels = false; std::printf("      %s/%d at %.1f dB (wants %.1f)\n", s.name.c_str(), v, lv, mixTargetDb(s.cls)); }
                if (v == 0) first = b; else if (b == first) varied = false;
            }
            if (idx++ % 7 == 0) same &= VoiceSynth::build(s, 0) == first;
        }
        std::printf("      voice bank: %zu names, %.1f s of audio\n", bank.size(), total);
        CHECK(built, "every voice builds: a real length, finite, peak under 0.95");
        CHECK(levels, "every voice sits within 2 dB of its mix class's level");
        CHECK(varied, "a voice's variants differ");
        CHECK(same, "the same voice builds the same every time");
        CHECK(total <= 200.f, "the whole voice bank is at most 200 s of audio");   // (180 before the Leviathan)
    }

    // ---------------------------------------------------------------- enemy voices: who gets to speak
    {
        auto in = [](int uid, EnemyType t, glm::vec3 p) { VoiceIn v; v.uid = uid; v.type = t; v.pos = p; v.health = 100.f; return v; };
        auto count = [](const std::vector<VoiceCue>& cs, SoundRole r, const std::string& part = "") {
            int n = 0; for (auto& c : cs) n += c.role == r && c.name.find(part) != std::string::npos; return n;
        };
        auto windup = [](VoiceIn v, AttackKind a, float t = 0.45f) { v.attack = a; v.telegraphTimer = t; v.telegraphStarted = true; return v; };
        const glm::vec3 O{0.f};
        const float F = 1.f / 60.f;
        {   // a wind-up 35 m off, behind a wall: heard, by name, at priority
            VoiceDirector d;
            d.begin(O, F); d.enemy(in(1, EnemyType::HUSK, {35, 0, 0})); d.end();
            d.begin(O, F); d.enemy(windup(in(1, EnemyType::HUSK, {35, 0, 0}), AttackKind::SHOT));
            auto cs = d.end();
            CHECK(count(cs, SoundRole::TELL, "v_husk_tell_shot") == 1 && cs[0].priority, "a wind-up 35 m off, out of sight, is heard: its own tell, at priority");
        }
        {   // the same enemy: not twice within 0.15 s
            VoiceDirector d; int tells = 0;
            for (int f = 0; f < 7; ++f) {
                d.begin(O, F); VoiceIn v = in(1, EnemyType::RIPPER, {5, 0, 0});
                if (f == 1 || f == 6) v = windup(v, AttackKind::LUNGE, 0.3f);
                d.enemy(v); tells += count(d.end(), SoundRole::TELL);
            }
            CHECK(tells == 1, "the same enemy can't repeat its tell within 0.15 s");
        }
        {   // six at once: the nearest four
            VoiceDirector d; d.begin(O, F); for (int i = 0; i < 6; ++i) d.enemy(in(i + 1, EnemyType::HUSK, {5.f + 5.f * i, 0, 0})); d.end();
            d.begin(O, F); for (int i = 0; i < 6; ++i) d.enemy(windup(in(i + 1, EnemyType::HUSK, {5.f + 5.f * i, 0, 0}), AttackKind::SHOT));
            int n = 0; bool nearest = true; for (auto& c : d.end()) if (c.role == SoundRole::TELL) { ++n; nearest &= c.uid <= 4; }
            CHECK(n == 4 && nearest, "six wind-ups at once: the nearest four are heard");
        }
        {   // chatter: only the nearest four, and they do chatter
            VoiceDirector d(7); std::set<int> spoke;
            for (int f = 0; f < 60 * 20; ++f) {
                d.begin(O, F);
                for (int i = 0; i < 10; ++i) { VoiceIn v = in(i + 1, EnemyType::HUSK, {2.f + 2.f * i, 0, 0}); v.moveSpeed = statsOf(EnemyType::HUSK).speed; d.enemy(v); }
                for (auto& c : d.end()) if (c.role == SoundRole::CHATTER && c.name.find("spawn") == std::string::npos) spoke.insert(c.uid);
            }
            CHECK(spoke.size() == 4 && *spoke.rbegin() <= 4, "ten enemies close by: the nearest four chatter, no one else");
        }
        {   // hurts: one per enemy per 0.4 s
            VoiceDirector d; int hurts = 0; float hp = 100.f;
            for (int f = 0; f < 60; ++f) {
                d.begin(O, F); VoiceIn v = in(1, EnemyType::BRUTE, {5, 0, 0});
                if (f >= 1 && f <= 5) hp -= 8.f;
                v.health = hp; d.enemy(v); hurts += count(d.end(), SoundRole::ACTION, "hurt");
            }
            CHECK(hurts == 1, "pellets landing over a few frames: one hurt, not eight");
        }
        {   // hurts: six a second at most
            VoiceDirector d; d.begin(O, F); for (int i = 0; i < 20; ++i) d.enemy(in(i + 1, EnemyType::HUSK, {3.f + i, 0, 0})); d.end();
            d.begin(O, F); for (int i = 0; i < 20; ++i) { VoiceIn v = in(i + 1, EnemyType::HUSK, {3.f + i, 0, 0}); v.health = 50.f; d.enemy(v); }
            CHECK(count(d.end(), SoundRole::ACTION, "hurt") == 6, "twenty hit at once: six hurts a second at most");
        }
        {   // the release: when the wind-up runs out (not when it's broken off)
            auto run = [&](bool stagger) {
                VoiceDirector d; int atk = 0;
                for (int f = 0; f < 40; ++f) {
                    d.begin(O, F); VoiceIn v = in(1, EnemyType::HUSK, {6, 0, 0});
                    if (f >= 1 && f < 28) { v.attack = AttackKind::SHOT; v.telegraphTimer = 0.45f - (f - 1) * F; v.telegraphStarted = f == 1; }
                    else if (f >= 28) { v.attack = stagger ? AttackKind::NONE : AttackKind::SHOT; v.staggered = stagger; }
                    d.enemy(v); atk += count(d.end(), SoundRole::ACTION, "v_husk_atk_shot");
                }
                return atk;
            };
            CHECK(run(false) == 1 && run(true) == 0, "the release sounds when the wind-up runs out, not when a stagger breaks it off");
        }
        {   // beam held, death cries at any range and ends it, the dead are forgotten
            VoiceDirector d; d.begin(O, F); d.enemy(in(1, EnemyType::SERAPH, {80, 0, 0})); d.end();
            d.begin(O, F); VoiceIn v = in(1, EnemyType::SERAPH, {80, 0, 0}); v.beamOn = true; d.enemy(v); auto start = d.end();
            auto cs = d.death(v);
            bool started = false, cry = false, stop = false;
            for (auto& c : start) started |= c.loop == VoiceCue::START && c.name == "v_seraph_atk_beam";
            for (auto& c : cs) { cry |= c.name == "v_seraph_death" && c.floor >= 0.5f; stop |= c.loop == VoiceCue::STOP && c.uid == 1; }
            CHECK(started, "a Seraph's beam starts a held voice");
            CHECK(cry && stop, "a death always cries out (any range, with a floor) and ends its held voice");
            d.begin(O, F); d.end();
            CHECK(d.tracked() == 0, "the dead are forgotten");
        }
        {   // Review focus 5: a beam that ends and starts again
            VoiceDirector d; int starts = 0, stops = 0;
            for (int f = 0; f < 4; ++f) {
                d.begin(O, F); VoiceIn v = in(1, EnemyType::SERAPH, {10, 0, 0}); v.beamOn = f == 1 || f == 3; d.enemy(v);
                for (auto& c : d.end()) { starts += c.loop == VoiceCue::START; stops += c.loop == VoiceCue::STOP; }
            }
            CHECK(starts == 2 && stops == 1, "a beam off and on again: stopped, then a fresh held voice");
        }
        {   // gone without a death: forgotten after half a second, its held voice stopped
            VoiceDirector d; d.begin(O, F); VoiceIn v = in(1, EnemyType::SERAPH, {10, 0, 0}); v.beamOn = true; d.enemy(v); d.end();
            bool stop = false;
            for (int f = 0; f < 40; ++f) { d.begin(O, F); for (auto& c : d.end()) stop |= c.loop == VoiceCue::STOP && c.uid == 1; }
            CHECK(d.tracked() == 0 && stop, "an enemy that vanishes is forgotten and its held voice stopped");
        }
        {   // Review focus 1: a retry forgets everyone at once
            VoiceDirector d; d.begin(O, F); d.enemy(in(1, EnemyType::HUSK, {5, 0, 0})); d.end();
            d.reset();
            CHECK(d.tracked() == 0, "reset forgets everyone");
        }
        {   // Review focus 2: a frame's gap is not a new enemy
            VoiceDirector d; int spawns = 0;
            for (int f = 0; f < 3; ++f) { d.begin(O, F); if (f != 1) d.enemy(in(1, EnemyType::HUSK, {5, 0, 0})); spawns += count(d.end(), SoundRole::CHATTER, "spawn"); }
            CHECK(spawns == 1, "an enemy missing for a frame and back again doesn't spawn twice");
        }
        {   // Review focus 3: killed after it was reported this frame
            VoiceDirector d; d.begin(O, F); d.enemy(in(1, EnemyType::HUSK, {5, 0, 0})); d.end();
            d.begin(O, F); VoiceIn v = in(1, EnemyType::HUSK, {5, 0, 0}); v.health = 0.f; d.enemy(v);
            auto deathCues = d.death(v);
            auto cs = d.end();
            CHECK(count(deathCues, SoundRole::ACTION, "death") == 1 && cs.empty() && d.tracked() == 0,
                  "killed mid-frame: its death cry and nothing else (no spawn, no hurt)");
        }
        {   // Hollowed: same name, changed voice
            auto tellOf = [&](Hollow h) {
                VoiceDirector d; VoiceIn v = in(1, EnemyType::HUSK, {5, 0, 0}); v.hollow = h; v.halo = h == Hollow::HALOED;
                d.begin(O, F); d.enemy(v); d.end();
                d.begin(O, F); d.enemy(windup(v, AttackKind::SHOT)); return d.end();
            };
            auto en = tellOf(Hollow::ENRAGED), tw = tellOf(Hollow::TWINNED), ha = tellOf(Hollow::HALOED);
            int twins = 0; float delay = 0.f; bool shimmer = false;
            for (auto& c : tw) if (c.name == "v_husk_tell_shot") { ++twins; delay = std::max(delay, c.delay); }
            for (auto& c : ha) shimmer |= c.name == VOICE_HALO_SHIMMER;
            CHECK(!en.empty() && en[0].name == "v_husk_tell_shot" && en[0].pitch > 1.1f && en[0].drive > 0.f, "an Enraged voice: the same tell, higher and driven");
            CHECK(twins == 2 && std::fabs(delay - 0.012f) < 1e-4f, "a Twinned voice is doubled, the copy 12 ms late");
            CHECK(shimmer, "a Haloed one's tell shimmers");
            VoiceDirector d; VoiceIn v = in(1, EnemyType::HUSK, {5, 0, 0}); v.hollow = Hollow::HALOED; v.halo = true;
            d.begin(O, F); d.enemy(v); d.end();
            v.halo = false; d.begin(O, F); d.enemy(v);
            CHECK(count(d.end(), SoundRole::ACTION, VOICE_HALO_BREAK) == 1, "a halo breaking shatters");
        }
        {   // Review focus 4: a dozen appearing at once
            VoiceDirector d; d.begin(O, F); for (int i = 0; i < 12; ++i) d.enemy(in(i + 1, EnemyType::MITE, {3.f + i, 0, 0}));
            int n = 0; bool nearest = true; for (auto& c : d.end()) if (c.name.find("spawn") != std::string::npos) { ++n; nearest &= c.uid <= 3; }
            CHECK(n == 3 && nearest, "a dozen appearing at once: three spawn cues, the nearest");
        }
        {   // bosses: chatter whoever else is near; their wind-ups duck the mix (not the Penitent's every sweep)
            VoiceDirector d; bool bossSpoke = false;
            for (int f = 0; f < 60 * 15; ++f) {
                d.begin(O, F);
                for (int i = 0; i < 6; ++i) d.enemy(in(i + 1, EnemyType::HUSK, {2.f + i, 0, 0}));
                VoiceIn w = in(99, EnemyType::WARDEN, {40, 0, 0}); w.moveSpeed = 2.4f; d.enemy(w);
                for (auto& c : d.end()) bossSpoke |= c.uid == 99 && c.role == SoundRole::CHATTER && c.name.find("spawn") == std::string::npos;
            }
            CHECK(bossSpoke, "a boss chatters at 40 m whoever else is near");
            auto duckOf = [&](EnemyType t, AttackKind a) {
                VoiceDirector e; e.begin(O, F); e.enemy(in(5, t, {10, 0, 0})); e.end();
                e.begin(O, F); e.enemy(windup(in(5, t, {10, 0, 0}), a, 1.f));
                for (auto& c : e.end()) if (c.role == SoundRole::TELL) return c.duckDb;
                return -1.f;
            };
            CHECK(duckOf(EnemyType::WARDEN, AttackKind::SLAM) == 6.f && duckOf(EnemyType::PENITENT, AttackKind::PSLAM) == 6.f &&
                  duckOf(EnemyType::PENITENT, AttackKind::CENSER_LOW) == 0.f && duckOf(EnemyType::HUSK, AttackKind::SHOT) == 0.f,
                  "a boss's wind-up ducks the mix (not the Penitent's every sweep, not a Husk's)");
        }
        {   // every cue the director can ask for is in the bank
            std::set<std::string> names; for (auto& s : voiceBank()) names.insert(s.name);
            bool all = true;
            auto check = [&](const std::vector<VoiceCue>& cs) {
                for (auto& c : cs) if (!names.count(c.name)) { all = false; std::printf("      no voice called %s\n", c.name.c_str()); }
            };
            for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
                const EnemyType t = (EnemyType)i; VoiceDirector d((uint32_t)i + 3); float hp = 100.f;
                const auto atks = attacksOf(t);
                for (int f = 0; f < 60 * 10; ++f) {
                    d.begin(O, F); VoiceIn v = in(1, t, {5, 0, 0});
                    v.moveSpeed = statsOf(t).speed; hp -= f % 50 == 0 ? 5.f : 0.f; v.health = hp;
                    if (!atks.empty()) {
                        const int k = (f / 40) % (int)atks.size(), ph = f % 40;
                        v.attack = atks[k]; v.telegraphTimer = ph < 30 ? 0.5f - ph * F : 0.f; v.telegraphStarted = ph == 0;
                        v.beamOn = atks[k] == AttackKind::BEAM && ph >= 30;
                    }
                    v.enraged = f == 300; v.rose = f == 310; v.hollow = f < 200 ? Hollow::HALOED : Hollow::NONE; v.halo = f < 200;
                    v.linkCount = f / 100;
                    d.enemy(v); check(d.end());
                }
                check(d.death(in(1, t, {5, 0, 0})));
            }
            CHECK(all, "every voice the director can ask for is in the bank");
        }
    }

    // ---------------------------------------------------------------- the mix: every sound file at its class's level
    {
        bool levels = true, complete = true;
        for (const MixEntry& m : mixTable()) {
            for (int k = 0; k <= 8; ++k) {
                const std::string path = "assets/sfx/" + std::string(m.name) + (k ? "_" + std::to_string(k) : std::string()) + ".wav";
                float sr; auto x = readWav16(path, sr);
                if (x.empty()) { if (k == 0) { complete = false; std::printf("      missing %s\n", path.c_str()); } continue; }
                const float lv = shortTermDb(x.data(), x.size(), sr) + m.trimDb;
                if (std::fabs(lv - mixTargetDb(m.cls)) > 2.f) { levels = false; std::printf("      %s at %.1f dB after trim (wants %.1f)\n", path.c_str(), lv, mixTargetDb(m.cls)); }
            }
        }
        CHECK(complete, "every sound in the mix table has its file");
        CHECK(levels, "every sound file, after its trim, sits within 2 dB of its class's level");
        const char* loaded[] = {"jump", "land", "dash", "slam", "revolver", "shotgun", "reload", "grapple_fire", "hit", "player_hit",
                                "parry", "telegraph", "explosion", "wave", "pickup", "kar", "longshot", "bolt", "scope", "levelup",
                                "potion", "barrier", "split", "upgrade", "clank", "punch", "step1", "step2", "step3", "step4", "door",
                                "door_close", "boost", "cyl_open", "cyl_close", "eject", "shell_in", "pump", "wade", "skim", "cell",
                                "dry", "switch_up0", "switch_up1", "switch_up2", "switch_up3"};
        bool all = true; for (const char* n : loaded) all &= mixEntry(n) != nullptr;
        CHECK(all && mixTable().size() == sizeof(loaded) / sizeof(loaded[0]), "every sound file the game loads has exactly one place in the mix");
        CHECK(std::fabs(mixGain("jump") - std::pow(10.f, -4.1f / 20.f)) < 1e-4f && mixGain("v_husk_idle") == 1.f && mixGain("nope") == 1.f,
              "a sound's mix gain is its trim; built voices and unknown names pass at 1");
    }

    // ---------------------------------------------------------------- review fixes: enemy voices
    {
        const float F = 1.f / 60.f;
        {   // drive keeps a voice about as loud: an Enraged one isn't quieter than the rest
            auto run = [](float amp, float drive) {
                std::vector<float> sine(44100);
                for (int i = 0; i < 44100; ++i) sine[i] = amp * std::sin(6.2831853f * 220.f * i / 44100.f);
                SfxMixer m(44100.f, 3); m.addSound("sine", sine);
                SfxMixer::Opts o; o.group = SoundGroup::UI; o.drive = drive;
                m.play("sine", o); return sfxRender(m, 8192);
            };
            bool level = true;
            for (float amp : {0.2f, 0.6f, 0.8f}) {
                auto clean = run(amp, 0.f), driven = run(amp, 0.35f);
                float d = 20.f * std::log10(sfxRms(driven, 0) / sfxRms(clean, 0));
                std::printf("      drive at %.1f: %+.1f dB\n", amp, d);
                level &= std::fabs(d) < 1.f && sfxPeak(driven) <= 1.f;
            }
            CHECK(level, "an Enraged voice's drive keeps it within 1 dB of a normal one, quiet or loud");
        }
        {   // gone without a kill (an objective met, a boss's summons): forgotten, its held voice stopped at once
            VoiceDirector d; VoiceIn v; v.uid = 7; v.type = EnemyType::SERAPH; v.pos = {10, 0, 0}; v.health = 100.f; v.beamOn = true;
            d.begin({0, 0, 0}, F); d.enemy(v); d.end();
            auto cs = d.forget(7);
            bool stop = false; for (auto& c : cs) stop |= c.loop == VoiceCue::STOP && c.uid == 7;
            CHECK(stop && d.tracked() == 0, "an enemy removed without a kill is forgotten and its held voice stops at once");
        }
        {   // a death cry outranks the tells filling the enemy voices
            VoiceDirector d; VoiceIn v; v.uid = 3; v.type = EnemyType::HUSK; v.pos = {5, 0, 0}; v.health = 0.f;
            auto cs = d.death(v);
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); m.addSound(cs[0].name, sfxNoise(44100));
            for (int i = 0; i < 20; ++i) { SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.role = SoundRole::TELL; o.priority = true; o.volume = 0.5f; m.play("noise", o); }
            sfxRender(m, 16);
            SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.role = cs[0].role; o.priority = cs[0].priority; o.floor = cs[0].floor;
            SoundHandle h = m.play(cs[0].name, o); sfxRender(m, 16);
            CHECK(m.isPlaying(h), "a kill is heard even when wind-ups fill every enemy voice");
        }
    }

    // ---------------------------------------------------------------- the boss pass: the Sovereign, tuned
    {
        const LevelData& SL = L;
        const Arena& SA = SL.arenas.back();   // the Sanctum
        const glm::vec3 C = SA.bossSpawn;
        // Every tell readable: on every difficulty, enraged and in the last stand
        bool floors = true; float shortest = 9.f;
        for (int lvl = 0; lvl < DIFFICULTY_LEVELS; ++lvl)
            for (float hp : {1.f, 0.45f, 0.15f})
                for (float dist : {3.f, 9.f, 20.f}) {
                    Enemy s(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -dist});
                    s.spawnTimer = 0.f; s.state = EnemyState::ACTIVE;
                    EnemyWorld w = worldFor(SL, grid, C, SA); w.tune = &difficulty(lvl);
                    s.health = s.maxHealth * hp; if (hp < 0.5f) s.enraged = true;
                    for (int f = 0; f < 60 * 25 && s.alive; ++f) {
                        w.playerFeet = C + glm::vec3{std::sin(f * 0.01f) * 6.f, 0, std::cos(f * 0.013f) * 6.f};
                        w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
                        s.update(DT, w);
                        if (s.ev.telegraphStarted) {
                            float need = s.tellFollow ? Enemy::TELL_FLOOR_FOLLOW : Enemy::TELL_FLOOR;
                            shortest = std::min(shortest, s.telegraphDuration);
                            if (s.telegraphDuration < need - 1e-4f) floors = false;
                        }
                    }
                }
        std::printf("      sovereign's shortest tell: %.3f s\n", shortest);
        CHECK(floors, "every Sovereign tell is at least 0.35 s (0.28 s inside a combo), on every difficulty, enraged or not");
        // Every recovery is a real window
        bool windows = true; float minRec = 9.f;
        {
            Enemy s(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -6}); s.spawnTimer = 0.f; s.state = EnemyState::ACTIVE; s.enraged = true;
            EnemyWorld w = worldFor(SL, grid, C, SA); w.tune = &difficulty(DIFFICULTY_LEVELS - 1);
            float prev = 0.f;
            for (int f = 0; f < 60 * 40; ++f) {
                s.update(DT, w);
                if (s.recoverTimer > prev + 1e-4f && prev <= 0.f) { minRec = std::min(minRec, s.recoverTimer); windows &= s.recoverTimer >= Enemy::RECOVER_MIN - DT; }
                prev = s.recoverTimer;
            }
        }
        std::printf("      sovereign's shortest recovery: %.2f s\n", minRec);
        CHECK(windows, "every Sovereign recovery leaves at least 0.8 s, enraged on the hardest difficulty");
        // A parry: 3 s at x2
        Enemy p(EnemyType::SOVEREIGN, C); p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
        p.stagger(p.staggerTime());
        CHECK(std::fabs(p.staggerTimer - 3.f) < 1e-4f && p.armorMult() == 2.f, "a parried Sovereign is broken for 3 s and takes double");
        // Phase changes: a breath before the next attack
        auto pauseAfter = [&](float to) {
            Enemy s(EnemyType::SOVEREIGN, C + glm::vec3{0, 0, -5}); s.spawnTimer = 0.f; s.state = EnemyState::ACTIVE;
            EnemyWorld w = worldFor(SL, grid, C, SA);
            if (to < 0.3f) { s.health = s.maxHealth * 0.3f; s.enraged = true; s.update(DT, w); }
            for (int f = 0; f < 60 * 3 && !(s.attack == AttackKind::NONE && s.recoverTimer <= 0.f && s.dashTimer <= 0.f); ++f) s.update(DT, w);
            s.takeDamage(s.health - s.maxHealth * to);
            int started = 0;
            for (int f = 0; f < 33; ++f) { s.update(DT, w); started += s.ev.telegraphStarted; }   // 0.55 s
            return started;
        };
        CHECK(pauseAfter(0.49f) == 0 && pauseAfter(0.19f) == 0, "at 50 % and at 20 % the Sovereign starts nothing new for 0.6 s");
    }

    // ---------------------------------------------------------------- the boss pass: the Warden's conduits
    {
        const Arena& CORE = L.arenas[3];
        int conduits = 0; bool onPillars = true;
        for (const auto& a : L.anchors) {
            if (a.kind != LevelData::ChainAnchor::CONDUIT) continue;
            ++conduits;
            const AABB& b = L.walls[a.wall].box;
            float r = glm::length(glm::vec2(a.pos.x - L.reactorPos.x, a.pos.z - L.reactorPos.z));
            onPillars &= inside(CORE.bounds, a.pos) && r > 15.f && r < 18.f && a.pos.y > 2.f && a.pos.y < 4.f &&
                         a.pos.x >= b.min.x - 0.01f && a.pos.x <= b.max.x + 0.01f && a.hp == 250.f && a.maxHp == 250.f;
        }
        CHECK(conduits == 4 && onPillars, "four conduit nodes, 3 m up the pillars, facing the reactor, 250 hp each");
        CHECK(L.anchorsAlive(LevelData::ChainAnchor::CONDUIT) == 4 && L.anchorsAlive() == 0, "conduits are counted apart from the Penitent's chains");
        LevelData M = L;
        int c0 = -1; for (int i = 0; i < (int)M.anchors.size(); ++i) if (M.anchors[i].kind == LevelData::ChainAnchor::CONDUIT) { c0 = i; break; }
        AABB home = M.walls[M.anchors[c0].wall].box;
        bool cut = M.damageAnchor(c0, 300.f);
        bool parked = M.walls[M.anchors[c0].wall].box.max.y < -100.f;
        M.restoreAnchor(c0);
        CHECK(cut && parked && M.anchors[c0].alive && M.anchors[c0].hp == 250.f && M.walls[M.anchors[c0].wall].box.min == home.min,
              "a cut conduit's node is parked, and restoring it puts it back whole");
        ConduitClock k; k.allCut();
        int at19 = 0, at21 = 0, at26 = 0, at36 = 0;
        for (int f = 0; f < 60 * 37; ++f) { int n = k.update(DT); float t = (f + 1) * DT;
            if (std::fabs(t - 19.f) < DT / 2) at19 = n; if (std::fabs(t - 21.f) < DT / 2) at21 = n;
            if (std::fabs(t - 26.f) < DT / 2) at26 = n; if (std::fabs(t - 36.f) < DT / 2) at36 = n; }
        CHECK(at19 == 0 && at21 == 1 && at26 == 2 && at36 == 4, "conduits re-attach one at a time, 5 s apart, from 20 s after the last is cut");
        ConduitClock idle;
        CHECK(idle.update(1.f) == 0, "nothing re-attaches until all four have been cut");
    }

    // ---------------------------------------------------------------- the boss pass: the Warden's mind
    {
        auto warden = [&](glm::vec3 at) { Enemy w(EnemyType::WARDEN, at); w.spawnTimer = 0.f; w.state = EnemyState::ACTIVE; return w; };
        auto core = [&](glm::vec3 feet) { EnemyWorld w = worldFor(L, grid, feet, BOSS); w.reactor = L.reactorPos; w.hasReactor = true; return w; };
        const glm::vec3 R = L.reactorPos;
        CHECK(statsOf(EnemyType::WARDEN).health == 3600.f, "the Warden has 3600 health");
        // Damage: x0.5 while fed, x1 cut off, the core x3 in a vent, x2 in meltdown
        Enemy w = warden(R + glm::vec3{12, 0, 0}); w.conduitsLeft = 2;
        float fed = w.armorMult(); w.conduitsLeft = 0; float cut = w.armorMult();
        w.ventTimer = 1.f; float vent = w.woundMult(); bool open = w.coreOpen(); w.ventTimer = 0.f; bool shut = !w.coreOpen();
        w.conduitsLeft = 2; w.ventTimer = 1.f; float ventFed = w.armorMult(); w.ventTimer = 0.f;
        w.wardenPhase = 3; float melt = w.woundMult(); bool meltOpen = w.coreOpen();
        CHECK(fed == 0.5f && cut == 1.f && ventFed == 1.f, "fed by a conduit it takes half (a vent drops the shield); cut off, full");
        CHECK(open && shut && vent == 3.f && melt == 2.f && meltOpen, "its core: x3 in a vent, x2 open all through the meltdown");
        // Phases by health
        Enemy p = warden(R + glm::vec3{12, 0, 0}); EnemyWorld cw = core(R + glm::vec3{0, 0, 14});
        p.update(DT, cw); p.health = p.maxHealth * 0.59f; p.update(DT, cw);
        bool two = p.wardenPhase == 2 && p.ev.wPhase == 2 && p.ev.enraged;
        for (int f = 0; f < 60 * 12 && !p.atReactor; ++f) p.update(DT, cw);
        float atR = glm::length(glm::vec2(p.position.x - R.x, p.position.z - R.z));
        bool planted = p.atReactor;
        p.health = p.maxHealth * 0.24f; p.update(DT, cw);
        bool three = p.wardenPhase == 3 && p.ev.wPhase == 3 && std::fabs(p.meltClock - Enemy::W_MELT_TIME) < 0.1f && !p.atReactor;
        CHECK(two && planted && atR < 10.f, "at 60 % it walks to the reactor and plants itself there");
        CHECK(three, "at 25 % it tears free into the meltdown, its clock at 40 s");
        // Phase 1: vents after every third attack
        Enemy v = warden(R + glm::vec3{14, 0, 0}); EnemyWorld vw = core(R + glm::vec3{-14, 0, 0});
        int attacks = 0, vents = 0, volleys = 0; bool parryable = false;
        for (int f = 0; f < 60 * 40; ++f) {
            v.update(DT, vw);
            if (v.ev.telegraphStarted) { ++attacks; vents += v.attack == AttackKind::WVENT; }
            volleys += v.ev.shots > 0; parryable |= v.ev.shots > 0 && v.ev.shotParry == 150.f;
        }
        CHECK(vents >= 2 && vents * 4 <= attacks + 4, "it vents after every third attack");
        CHECK(volleys >= 2 && parryable, "its volleys can be parried back for 150");
        // Phase 2: the lance, then a vent; the seeker only for a player away or hidden
        Enemy l = warden(R + glm::vec3{6.5f, 0, 0}); l.wardenPhase = 2; l.atReactor = true; l.reactorSpot = l.position;
        EnemyWorld lw = core(R + glm::vec3{15, 0, -4});   // its side of the reactor: close and in sight
        bool lanced = false, ventAfter = false, seekClose = false;
        for (int f = 0; f < 60 * 20; ++f) {
            l.update(DT, lw);
            lanced |= l.ev.wLance; seekClose |= l.ev.wSeeker;
            if (lanced && l.ev.wVent) ventAfter = true;
        }
        CHECK(lanced && ventAfter, "overloaded, it sweeps a lance and vents when it ends");
        Enemy s = warden(R + glm::vec3{6.5f, 0, 0}); s.wardenPhase = 2; s.atReactor = true; s.reactorSpot = s.position;
        EnemyWorld far = core(R + glm::vec3{0, 0, 34}); far.walls = nullptr; far.wallCount = 0;   // out in the open, but past 28 m
        float firstSeek = -1.f;
        for (int f = 0; f < 60 * 8 && firstSeek < 0.f; ++f) { s.update(DT, far); if (s.ev.wSeeker) firstSeek = (f + 1) * DT; }
        CHECK(!seekClose, "no seeker at a player close by and in sight");
        CHECK(firstSeek >= Enemy::W_SEEK_AFTER - 0.05f && firstSeek < Enemy::W_SEEK_AFTER + 1.5f, "keep past 28 m for 4 s and a seeker comes for you");
        // Phase 3: meltdown never kills, the lunge can be punched
        Enemy m = warden(R + glm::vec3{8, 0, 0}); EnemyWorld mw = core(R + glm::vec3{-8, 0, 0});
        m.update(DT, mw); m.health = m.maxHealth * 0.2f; m.update(DT, mw);
        bool det = false; float hpAfter = 0.f;
        for (int f = 0; f < 60 * 45 && !det; ++f) { m.update(DT, mw); if (m.ev.wDetonate) { det = true; hpAfter = m.health / m.maxHealth; } }
        CHECK(det && std::fabs(hpAfter - Enemy::W_PHASE3) < 0.01f && m.wardenPhase == 3 && m.meltClock > 38.f,
              "the meltdown goes off at 0 and it heals back to 25 % with the clock reset");
        Enemy u = warden(R + glm::vec3{12, 0, 0}); u.wardenPhase = 3; u.meltClock = 40.f;
        EnemyWorld uw = core(R + glm::vec3{-6, 0, 0});
        bool window = false;
        for (int f = 0; f < 60 * 20 && !window; ++f) { u.update(DT, uw); if (u.attack == AttackKind::WLUNGE && u.parryWindow()) window = true; }
        u.stagger(u.staggerTime());
        CHECK(window && std::fabs(u.staggerTimer - 2.f) < 1e-4f && u.armorMult() == 2.f, "its lunge can be punched in the last 0.25 s: staggered 2 s, double damage");
        // Tell floors hold for it too
        bool floors = true;
        for (int lvl = 0; lvl < DIFFICULTY_LEVELS; ++lvl)
            for (int ph = 1; ph <= 3; ++ph) {
                Enemy t = warden(R + glm::vec3{10, 0, 0}); t.wardenPhase = ph; if (ph == 2) { t.atReactor = true; t.reactorSpot = t.position; }
                if (ph == 3) t.meltClock = 40.f;
                EnemyWorld tw = core(R + glm::vec3{-9, 0, 0}); tw.tune = &difficulty(lvl);
                for (int f = 0; f < 60 * 20; ++f) { t.update(DT, tw); if (t.ev.telegraphStarted && t.telegraphDuration < Enemy::TELL_FLOOR - 1e-4f) floors = false; }
            }
        CHECK(floors, "every Warden tell is at least 0.35 s, in every phase, on every difficulty");
    }

    // ---------------------------------------------------------------- the boss pass: the Warden's hazards
    {
        const glm::vec3 O{0.f, 0.f, 0.f};
        // The lance sweeps 120 degrees at 35 a second; a pillar between you stops it
        WardenHazards h; h.addLance(O + glm::vec3{0, 3.f, 0}, 0.f, 1.f);   // from facing +Z, sweeping toward +X
        float firstHit = -1.f, total = 0.f;
        glm::vec3 feet{std::sin(0.7f) * 12.f, 0.f, std::cos(0.7f) * 12.f};   // 40 degrees round, 12 m out
        for (int f = 0; f < 60 * 5; ++f) for (auto& x : h.update(DT, feet, 1.8f, nullptr, 0, O + glm::vec3{0, 0, 30}, false)) { if (firstHit < 0.f) firstHit = (f + 1) * DT; total += x.damage; }
        CHECK(firstHit > 1.0f && firstHit < 1.35f && total > 5.f && total < 15.f, "the lance reaches you as its sweep passes (35 degrees a second) and burns while it's on you");
        std::vector<Wall> pillar{Wall{LevelBuilder::aabb(std::sin(0.7f) * 6.f - 1.f, 0, std::cos(0.7f) * 6.f - 1.f, std::sin(0.7f) * 6.f + 1.f, 7, std::cos(0.7f) * 6.f + 1.f), {}}};
        WardenHazards hb; hb.addLance(O + glm::vec3{0, 3.f, 0}, 0.f, 1.f);
        float blocked = 0.f;
        for (int f = 0; f < 60 * 5; ++f) for (auto& x : hb.update(DT, feet, 1.8f, pillar.data(), 1, O + glm::vec3{0, 0, 30}, false)) blocked += x.damage;
        CHECK(blocked == 0.f, "a pillar between you and the Warden stops the lance");
        // Review focus 4: a player up on a walkway passes over the beam's band
        WardenHazards hw; hw.addLance(O + glm::vec3{0, 3.f, 0}, 0.f, 1.f);
        float high = 0.f;
        for (int f = 0; f < 60 * 5; ++f) for (auto& x : hw.update(DT, feet + glm::vec3{0, 7.f, 0}, 1.8f, nullptr, 0, O + glm::vec3{0, 0, 30}, false)) high += x.damage;
        CHECK(high == 0.f, "a lance passes under a player on a walkway 7 m up");
        CHECK(std::fabs(WardenHazards::LANCE_TIME - Enemy::W_LANCE_TIME) < 1e-4f, "the lance lasts as long as the Warden stands for it");
        // The seeker: 1 s of warning, then a burst where you stood
        WardenHazards s; s.addSeeker(glm::vec3{5, 0, 5});
        int early = 0, hits = 0, missed = 0;
        for (int f = 0; f < 50; ++f) early += (int)s.update(DT, glm::vec3{5, 0, 5}, 1.8f, nullptr, 0, O, false).size();
        for (int f = 0; f < 30; ++f) hits += (int)s.update(DT, glm::vec3{5, 0, 5}, 1.8f, nullptr, 0, O, false).size();
        WardenHazards s2; s2.addSeeker(glm::vec3{5, 0, 5});
        for (int f = 0; f < 90; ++f) missed += (int)s2.update(DT, glm::vec3{10, 0, 5}, 1.8f, nullptr, 0, O, false).size();
        CHECK(early == 0 && hits == 1 && missed == 0, "a seeker bursts after its 1 s marker, on whoever stayed in it");
        // Steam: 15/s within 4 m of an open vent
        WardenHazards st; float steam = 0.f, cold = 0.f;
        for (int f = 0; f < 60; ++f) for (auto& x : st.update(DT, glm::vec3{3, 0, 0}, 1.8f, nullptr, 0, O, true)) steam += x.damage;
        for (int f = 0; f < 60; ++f) for (auto& x : st.update(DT, glm::vec3{6, 0, 0}, 1.8f, nullptr, 0, O, true)) cold += x.damage;
        CHECK(std::fabs(steam - 15.f) < 4.f && cold == 0.f, "an open vent scalds within 4 m (15/s), not past it");
        st.addLance(O, 0.f, 1.f); st.addSeeker(O); st.clear();
        CHECK(st.lances.empty() && st.seekers.empty(), "clear() drops every lance and seeker (a retry)");
        // The rings, driven by the boss: every 3.5 s from where it says, off again when it's done
        LevelData RL = L; ArenaShifts sh; sh.capture(RL); sh.reset(RL);
        sh.bossPulse(3.5f, RL.reactorPos);
        int rings = 0;
        for (int f = 0; f < 60 * 8; ++f) { sh.update(DT, RL, 3, true); rings += sh.pulseFired; }
        sh.bossPulseOff();
        int after = 0;
        for (int f = 0; f < 60 * 8; ++f) { sh.update(DT, RL, 3, true); after += sh.pulseFired; }
        CHECK(rings == 2 && after == 0, "the Warden drives the reactor's rings every 3.5 s, and they stop when it says");
        LevelData NR = L; NR.hasReactor = false; ArenaShifts sn; sn.capture(NR); sn.reset(NR);
        sn.bossPulse(5.f, glm::vec3{1, 0, -240});
        int nr = 0; for (int f = 0; f < 60 * 6; ++f) { sn.update(DT, NR, 0, true); nr += sn.pulseFired; }
        CHECK(nr == 1, "with no reactor, the rings come off the Warden itself");
    }

    // ---------------------------------------------------------------- the boss pass: the Warden in the game
    {
        // Review focus 5: a parried orb steers into a living Warden's core
        glm::vec3 pos{0, 2, 0}, vel{20, 0, 0}, target{0, 3, 20};
        for (int f = 0; f < 60; ++f) { vel = steerParried(pos, vel, target, DT); pos += vel * DT; }
        CHECK(glm::length(pos - target) < 2.5f && std::fabs(glm::length(vel) - 20.f) < 0.1f, "a parried orb curves into the Warden's core, keeping its speed");
        // Review focus 1: conduits are only targets while the Warden is alive and feeding
        CHECK(!conduitShootable(false, 1) && conduitShootable(true, 1) && !conduitShootable(true, 2), "conduits can only be cut while the Warden is alive and feeding");
        // Its core: in front of its chest, only from the front
        Enemy w(EnemyType::WARDEN, {0, 0, 0}); w.spawnTimer = 0.f; w.state = EnemyState::ACTIVE; w.yaw = 0.f; w.ventTimer = 1.f;
        AABB cb; bool has = coreBox(w, cb);
        float tf = 0.f, tb = 0.f;
        bool front = woundShot(w, glm::vec3{0, (cb.min.y + cb.max.y) * 0.5f, 10.f}, glm::vec3{0, 0, -1}, tf);
        bool back = woundShot(w, glm::vec3{0, (cb.min.y + cb.max.y) * 0.5f, -10.f}, glm::vec3{0, 0, 1}, tb);
        w.ventTimer = 0.f; AABB shut; bool closed = !coreBox(w, shut);
        CHECK(has && (cb.min.z + cb.max.z) * 0.5f > 0.3f && front && !back && closed, "the Warden's core opens on its chest and is hit from the front");
    }

    // ---------------------------------------------------------------- the boss pass: no cheese, and fights that end
    {
        const Arena& CORE = L.arenas[3]; const Arena& SAN = L.arenas.back();
        auto punished = [&](const Arena& a, EnemyType t, BossSim::Policy p, float hpFrac) {
            BossSim s(L, grid, a, t, p);
            s.boss.health = s.boss.maxHealth * hpFrac;
            for (int f = 0; f < 60 * 8 && s.damageTaken <= 0.f; ++f) s.step(DT);
            return s.damageTaken > 0.f ? s.t : 99.f;
        };
        const char* names[] = {"corner-camper", "perch-sitter", "edge-kiter", "ranged-only"};
        bool ok = true;
        for (int p = 0; p < 4; ++p) {
            float w1 = punished(CORE, EnemyType::WARDEN, (BossSim::Policy)p, 1.f);
            float w2 = punished(CORE, EnemyType::WARDEN, (BossSim::Policy)p, 0.5f);
            float sv = punished(SAN, EnemyType::SOVEREIGN, (BossSim::Policy)p, 1.f);
            std::printf("      %-14s hit after: warden %.1f s, warden overloaded %.1f s, sovereign %.1f s\n", names[p], w1, w2, sv);
            ok &= w1 <= 8.f && w2 <= 8.f && sv <= 8.f;
        }
        CHECK(ok, "every cheese (corner, perch, edge-kiting, ranged-only) is punished within 8 s by both bosses");
        auto killTime = [&](const Arena& a, EnemyType t) {
            BossSim s(L, grid, a, t, BossSim::SOLID);
            for (int f = 0; f < 60 * 400 && s.bossAlive(); ++f) s.step(DT);
            return s.bossAlive() ? 999.f : s.t;
        };
        float kw = killTime(CORE, EnemyType::WARDEN), ks = killTime(SAN, EnemyType::SOVEREIGN);
        std::printf("      solid player's kill time: warden %.0f s, sovereign %.0f s\n", kw, ks);
        // The windows are what the approved design gives this scripted player (who
        // parries every other window and dodges perfectly); real fights run longer
        CHECK(kw >= 75.f && kw <= 120.f, "a solid player kills the Warden in 75-120 s");
        CHECK(ks >= 100.f && ks <= 150.f, "a solid player kills the Sovereign in 100-150 s");
    }

    // ---------------------------------------------------------------- review fixes: the boss pass
    {
        const glm::vec3 R = L.reactorPos;
        EnemyWorld cw = worldFor(L, grid, R + glm::vec3{-9, 0, 0}, BOSS); cw.reactor = R; cw.hasReactor = true;
        {   // the meltdown always winds up, even when another attack lands the same tick
            Enemy m(EnemyType::WARDEN, R + glm::vec3{9, 0, 0}); m.spawnTimer = 0.f; m.state = EnemyState::ACTIVE;
            m.wardenPhase = 3; m.health = m.maxHealth * 0.2f;
            m.attack = AttackKind::VOLLEY; m.telegraphTimer = DT * 0.5f; m.telegraphDuration = 1.f;
            m.meltClock = 1.5f + DT * 0.5f;
            m.update(DT, cw);
            bool noBlast = !m.ev.wDetonate, volleyed = m.ev.shots > 0;
            m.update(DT, cw);
            CHECK(noBlast && volleyed && m.attack == AttackKind::DETONATE && m.telegraphTimer > 1.f,
                  "the meltdown never goes off unannounced: the attack landing that tick lands, then the meltdown winds up");
        }
        {   // the lunge can be punched as it reaches you
            Enemy u(EnemyType::WARDEN, R + glm::vec3{9, 0, 0}); u.spawnTimer = 0.f; u.state = EnemyState::ACTIVE;
            u.wardenPhase = 3; u.meltClock = 40.f; u.health = u.maxHealth * 0.2f;
            bool punchable = false;
            for (int f = 0; f < 60 * 15 && !punchable; ++f) {
                u.update(DT, cw);
                glm::vec3 mid = u.position + glm::vec3{0, u.height() * 0.5f, 0};
                if (u.parryWindow() && glm::length(mid - cw.playerEye) < 5.5f) punchable = true;
            }
            CHECK(punchable, "a lunge can be punched within the parry's reach as it arrives");
        }
    }

    // ---------------------------------------------------------------- the arsenal's sounds, against their recordings
    {
        // Each rebuilt sound keeps the character of the recording it's built on
        // (assets/sfx/src/<name>_rec.wav): its midrange body, its ring-out, its brightness
        struct Shape { float mid, ringOut, bright; };
        auto shape = [](const std::vector<float>& x, float sr) {
            auto lp = [&](float hz) { std::vector<float> o(x.size()); float a = 1.f - std::exp(-6.2831853f * hz / sr), y = 0.f;
                                      for (size_t i = 0; i < x.size(); ++i) { y += a * (x[i] - y); o[i] = y; } return o; };
            auto l1 = lp(300.f), l2 = lp(3000.f);
            double low = 0, mid = 0, all = 0;
            for (size_t i = 0; i < x.size(); ++i) { low += l1[i] * l1[i]; mid += (l2[i] - l1[i]) * (l2[i] - l1[i]); all += x[i] * x[i]; }
            float pk = 0.f; for (float v : x) pk = std::max(pk, std::fabs(v));
            size_t last = 0; for (size_t i = 0; i < x.size(); ++i) if (std::fabs(x[i]) > 0.01f * pk) last = i;
            int n = 0, z = 0; for (size_t i = 1; i < x.size(); ++i) if (std::fabs(x[i]) > 0.05f * pk) { ++n; z += (x[i] > 0.f) != (x[i - 1] > 0.f); }
            return Shape{(float)(100.0 * mid / std::max(all, 1e-12)), last / sr, n ? z * sr / 2.f / n : 0.f};
        };
        bool body = true, ring = true, bright = true, refs = true;
        for (const char* n : {"revolver", "shotgun", "kar", "longshot", "reload", "cyl_open", "cyl_close", "eject", "shell_in", "bolt", "pump"}) {
            float rs; auto ref = readWav16(std::string("assets/sfx/src/") + n + "_rec.wav", rs);
            if (ref.empty()) { refs = false; std::printf("      no recording for %s\n", n); continue; }
            Shape R = shape(ref, rs);
            for (int k = 0; k <= 2; ++k) {
                std::string path = std::string("assets/sfx/") + n + (k ? "_" + std::to_string(k) : std::string()) + ".wav";
                float sr; auto x = readWav16(path, sr);
                if (x.empty()) continue;
                Shape S = shape(x, sr);
                std::printf("      %-12s mid %3.0f%% (rec %3.0f%%)  ring-out %.2f s (rec %.2f)  brightness %5.0f Hz (rec %5.0f)\n",
                            (std::string(n) + (k ? "_" + std::to_string(k) : "")).c_str(), S.mid, R.mid, S.ringOut, R.ringOut, S.bright, R.bright);
                body &= S.mid >= R.mid - 10.f;
                ring &= S.ringOut >= 0.85f * R.ringOut;
                bright &= S.bright <= 1.3f * R.bright;
            }
        }
        CHECK(refs, "every gunshot and reload sound has its recording frozen beside the generator");
        CHECK(body, "every rebuilt gun sound keeps its recording's midrange body (within 10 points)");
        CHECK(ring, "every rebuilt gun sound rings out at least 85% as long as its recording");
        CHECK(bright, "no rebuilt gun sound is more than 30% brighter than its recording");
    }

    {   // skittering steps (Rippers, Mites) sit under the rest of the chatter
        auto stepVol = [&](EnemyType t) {
            VoiceDirector d(5); float vol = -1.f;
            for (int f = 0; f < 60 * 4 && vol < 0.f; ++f) {
                d.begin({0, 0, 0}, 1.f / 60.f);
                VoiceIn v; v.uid = 1; v.type = t; v.pos = {4, 0, 0}; v.health = 100.f; v.moveSpeed = statsOf(t).speed; d.enemy(v);
                for (auto& c : d.end()) if (c.name.find("_move") != std::string::npos) vol = c.volume;
            }
            return vol;
        };
        float husk = stepVol(EnemyType::HUSK), ripper = stepVol(EnemyType::RIPPER), mite = stepVol(EnemyType::MITE);
        CHECK(husk > 0.f && ripper > 0.f && mite > 0.f && ripper <= husk * 0.5f && mite <= husk * 0.5f,
              "Ripper and Mite steps play at half the level of other enemies' steps");
    }

    // ---------------------------------------------------------------- the Reliquary: a formation that drifts
    {
        LevelData F; LevelBuilder B{F};
        auto chunk = [&](glm::vec3 top, glm::vec2 half, std::vector<glm::vec3> at) {
            LevelData::Formation::Chunk c; c.home = top; c.half = half; c.at = at;
            c.movers.push_back(B.mover(top - glm::vec3{0, 0.75f, 0}, {half.x, 0.75f, half.y}, Mover::Path::DRIVEN, {0, 0, 0}, {0, 0, 0}, 1.f, 0.f, {1, 1, 1}));
            F.formation.chunks.push_back(c);
        };
        chunk({0, 0, 0},  {5, 5}, {{0, 0, 0}, {0, 0, -20}, {10, 3, 0}});
        chunk({20, 0, 0}, {4, 4}, {{0, 0, 0}, {0, 2, 0},   {-40, 0, 0}});
        LevelData::Formation& fm = F.formation;
        fm.reset(F); F.updateMovers(0.f);
        CHECK(fm.arrangements() == 3 && !fm.gliding() && F.walls[F.movers[0].wall].box.max.y == 0.f, "a formation starts in its first arrangement");
        fm.glideTo(1);
        float clock = 0.f; bool eased = true; float prevZ = 0.f, prevStep = 0.f;
        for (int i = 0; i < 60 * 3; ++i) {
            clock += DT; fm.update(DT, F); F.updateMovers(clock);
            float z = F.walls[F.movers[0].wall].box.max.z, step = std::fabs(z - prevZ);
            if (i > 5 && i < 60 && step + 1e-5f < prevStep) eased = false;   // speeding up in the first second
            prevStep = step; prevZ = z;
        }
        bool midway = fm.gliding();
        for (int i = 0; i < 60 * 4; ++i) { clock += DT; fm.update(DT, F); F.updateMovers(clock); }
        CHECK(midway && eased && !fm.gliding() && fm.at == 1, "a glide eases in, takes 6 s, and ends");
        CHECK(std::fabs(F.walls[F.movers[0].wall].box.max.z - (5.f - 20.f)) < 1e-4f && std::fabs(F.walls[F.movers[1].wall].box.max.y - 2.f) < 1e-4f,
              "each chunk ends exactly in its next arrangement");
        CHECK(fm.chunkOfWall(F, F.movers[1].wall) == 1 && fm.chunkOfWall(F, 9999) == -1 && (fm.top(1) == glm::vec3{20, 2, 0}),
              "a formation knows which chunk a wall belongs to, and where its top is");
        // Review focus 1: a retry mid-glide puts everything back at once
        fm.glideTo(2);
        for (int i = 0; i < 60 * 2; ++i) { clock += DT; fm.update(DT, F); F.updateMovers(clock); }
        fm.reset(F); F.updateMovers(clock);
        CHECK(!fm.gliding() && fm.at == 0 && F.walls[F.movers[0].wall].box.max.y == 0.f && F.walls[F.movers[0].wall].box.max.x == 5.f,
              "reset mid-glide puts every chunk back in its first arrangement");
        // Review focus 5: a player standing at a chunk's edge when it glides is carried and stays on
        fm.glideTo(2);
        Player p({4.6f, 0.f, 4.6f});
        p.dynWalls = F.moverWalls.data(); p.dynCount = (int)F.moverWalls.size();
        SpatialGrid pg; pg.build(F.walls);
        bool stayed = true;
        for (int i = 0; i < 60 * 7; ++i) {
            int rm = F.moverOfWall(p.groundWall);
            clock += DT; fm.update(DT, F); F.updateMovers(clock);
            if (rm >= 0) p.position += F.movers[rm].delta;
            p.floorY = -100.f;
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
            p.update(DT, k, F.walls.data(), (int)F.walls.size(), false, &pg);
            if (i > 10) stayed &= p.position.y > fm.top(0).y - 0.3f && p.position.y < fm.top(0).y + 1.2f;
        }
        CHECK(stayed && std::fabs(p.position.x - 14.6f) < 0.3f, "standing at a chunk's edge you're carried through the glide and stay on");
    }

    // ---------------------------------------------------------------- the Reliquary: the map
    {
        LevelData N = buildAct2Level();
        CHECK(N.arenas.size() == 5 && std::string(N.arenas[3].name) == "THE RELIQUARY" && N.arenas[3].shift == ArenaShift::DRIFT &&
              N.arenas[3].waves.size() == 3 && N.arenas[3].maxAlive == 12 && std::fabs(N.arenas[3].damageScale - 1.45f) < 1e-4f,
              "the Reliquary: the fourth Act II arena, three waves, 12 at once, x1.45");
        const Arena& R = N.arenas[3];
        auto& fm = N.formation;
        CHECK(fm.chunks.size() >= 12 && fm.arrangements() == 4, "four relics and their debris, in four arrangements (three waves and the way to the hole)");
        SpatialGrid g; g.build(N.walls);
        float clock = 0.f;
        // Spawns stand on a chunk in their wave's arrangement, inside bounds, clear of walls
        bool spawnsOk = true;
        for (int w = 0; w < 3; ++w) {
            fm.at = fm.to = w; fm.t = 0.f; fm.apply(N); N.updateMovers(clock);
            for (auto& s : R.waveGround[w]) {
                float top = N.groundAt(s.x, s.z, s.y + 0.5f);
                bool clear = !overlapsWall(N, boxAt(s + glm::vec3{0, 0.05f, 0}, 0.6f, 1.8f));
                if (std::fabs(top - s.y) > 0.05f || !inside(R.bounds, s) || !clear) { spawnsOk = false; std::printf("      wave %d spawn (%.1f %.1f %.1f) top %.2f clear %d\n", w, s.x, s.y, s.z, top, clear); }
            }
        }
        CHECK(spawnsOk, "every wave's spawns stand on a chunk, inside the Reliquary, clear of walls");
        // Every chunk can be reached from the landing in every arrangement: jump+dash gaps or a grapple point in reach
        const float GRAPPLE = 50.f - 2.f;   // GrappleHook::maxLength (its header needs GL)
        bool reach = true;
        for (int k = 0; k < 4; ++k) {
            fm.at = fm.to = k; fm.t = 0.f; fm.apply(N); N.updateMovers(clock);
            int n = (int)fm.chunks.size();
            auto rect = [&](int c) { glm::vec3 t = fm.top(c); return glm::vec4{t.x - fm.chunks[c].half.x, t.z - fm.chunks[c].half.y, t.x + fm.chunks[c].half.x, t.z + fm.chunks[c].half.y}; };
            auto gap = [&](int a, int b) { glm::vec4 A = rect(a), Bq = rect(b);
                float dx = std::max(0.f, std::max(A.x - Bq.z, Bq.x - A.z)), dz = std::max(0.f, std::max(A.y - Bq.w, Bq.y - A.w)); return std::sqrt(dx * dx + dz * dz); };
            std::vector<char> seen(n, 0); std::vector<int> q{0}; seen[0] = 1;   // chunk 0: the Yard, where the bridge lands
            while (!q.empty()) {
                int a = q.back(); q.pop_back();
                for (int b = 0; b < n; ++b) {
                    if (seen[b]) continue;
                    float up = fm.top(b).y - fm.top(a).y;
                    bool hop = gap(a, b) <= 9.f && up <= 3.f;
                    bool hook = fm.chunks[b].movers.size() > 1 && gap(a, b) <= GRAPPLE;   // big chunks carry a pillar
                    if (hop || hook) { seen[b] = 1; q.push_back(b); }
                }
            }
            for (int c = 0; c < n; ++c) if (!seen[c]) { reach = false; std::printf("      arrangement %d: chunk %d unreachable\n", k, c); }
        }
        CHECK(reach, "in every arrangement every chunk can be reached from the Yard (a jump and a dash, or a grapple)");
        // Gliding, no chunk passes through another or through the static world
        bool clean = true;
        for (int k = 0; k < 3; ++k) {
            fm.at = fm.to = k; fm.t = 0.f; fm.apply(N); N.updateMovers(clock); fm.glideTo(k + 1);
            for (int i = 0; i < 60 * 7; ++i) {
                clock += DT; fm.update(DT, N); N.updateMovers(clock);
                if (i % 6) continue;
                for (int a = 0; a < (int)fm.chunks.size(); ++a)
                    for (int ma : fm.chunks[a].movers) {
                        AABB A = N.walls[N.movers[ma].wall].box;
                        for (int b = a + 1; b < (int)fm.chunks.size(); ++b)
                            for (int mb : fm.chunks[b].movers) if (overlapsBox(A, N.walls[N.movers[mb].wall].box, 0.02f)) { if (clean) std::printf("      glide %d->%d at %.1f s: chunk %d meets chunk %d\n", k, k + 1, i * DT, a, b); clean = false; }
                        for (auto& wl : N.walls) if (!wl.dynamic && overlapsBox(A, wl.box, 0.02f)) { if (clean) std::printf("      glide %d->%d at %.1f s: chunk %d meets a wall at (%.0f %.0f %.0f)\n", k, k + 1, i * DT, a, wl.box.min.x, wl.box.min.y, wl.box.min.z); clean = false; }
                    }
            }
        }
        CHECK(clean, "gliding, no relic passes through another or through the static world");
        // The way in: a bridge from the pit's south wall to the Yard; the way out: the hole's rim
        fm.reset(N); N.updateMovers(0.f);
        float bridge = N.groundAt(0.f, -890.f, -239.f), landing = N.groundAt(fm.top(0).x + 5.f, fm.top(0).z + 6.f, fm.top(0).y + 0.5f);   // beside the dais
        CHECK(std::fabs(bridge + 240.f) < 0.05f && std::fabs(landing - fm.top(0).y) < 0.05f && N.arenas[2].exitDoor >= 0,
              "a bridge leads from a door in the pit's south wall out to the Yard");
        fm.at = fm.to = 3; fm.apply(N); N.updateMovers(0.f);
        CHECK(std::fabs(N.groundAt(0.f, -1020.f, -239.f) + 240.f) < 0.05f && N.arenaAt({0.f, -240.f, -1021.f}) == 3 &&
              N.arenaAt({0.f, -240.f, -1023.f}) == 4, "the rim stands past the last arrangement; beyond its edge is the Maw");
        CHECK(MUSIC_TRACKS >= 9 && std::string(musicTrack(8).name) == "RELIQUARY", "the Reliquary has its own track");
    }

    // ---------------------------------------------------------------- the Revenant
    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& R = N.arenas[3];
        N.formation.reset(N); N.updateMovers(0.f);
        auto world = [&](glm::vec3 feet) { EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
                                           w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = R.bounds; return w; };
        CHECK(statsOf(EnemyType::REVENANT).health == 160.f && statsOf(EnemyType::WEAVER).health == 120.f, "a Revenant has 160 health, a Weaver 120");
        // It rakes up close and bolts at range, each with its tell
        glm::vec3 yard = N.formation.top(0);
        auto run = [&](float dist) {
            Enemy e(EnemyType::REVENANT, yard + glm::vec3{0, 0, -dist}); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
            EnemyWorld w = world(yard + glm::vec3{0, 0, 3});
            int rakes = 0, bolts = 0, hits = 0; float rakeTell = 0.f, boltTell = 0.f;
            for (int f = 0; f < 60 * 10; ++f) {
                e.update(DT, w);
                if (e.ev.telegraphStarted && e.attack == AttackKind::RAKE && !e.tellFollow) { ++rakes; rakeTell = e.telegraphDuration; }   // an opening rake (the second hand follows on)
                if (e.ev.telegraphStarted && e.attack == AttackKind::SOULBOLT) { ++bolts; boltTell = e.telegraphDuration; }
                hits += e.ev.meleeHit;
            }
            return std::tuple<int, int, int, float, float>{rakes, bolts, hits, rakeTell, boltTell};
        };
        auto [r1, b1, h1, rt, bt0] = run(1.f);
        auto [r2, b2, h2, rt2, bt] = run(9.f);
        std::printf("      revenant: close rakes %d hits %d tell %.2f; far bolts %d tell %.2f\n", r1, h1, rt, b2, bt);
        CHECK(r1 >= 2 && h1 >= 2 * r1 - 1 && std::fabs(rt - 0.45f) < 0.05f, "up close it rakes, two hits a time (0.45 s tell)");
        CHECK(b2 >= 1 && std::fabs(bt - 0.6f) < 0.06f, "at range it throws a soul bolt (0.6 s tell) before it closes in");
        // Its soul: flees toward another chunk, can be shot, punched, or re-forms at half
        RelicHazards hz;
        Enemy dead(EnemyType::REVENANT, yard); dead.maxHealth = dead.health = 160.f;
        glm::vec3 to = soulDestination(R.waveGround[0], yard, 15.f);
        Soul* s = hz.releaseSoul(dead, to);
        CHECK(s && glm::length(to - yard) >= 15.f && s->bodyHealth == 80.f && s->reforms == 1, "a dying Revenant releases a soul toward a spot on another chunk, carrying half its health");
        float t = 0.f; int hit = hz.raySoul(s->pos + glm::vec3{0, 0, 10}, {0, 0, -1}, 50.f, t);
        bool shot = hit == 0 && hz.hurtSoul(0, 40.f) && hz.souls.empty();
        CHECK(shot, "a soul shot for 40 is gone for good");
        hz.releaseSoul(dead, to);
        bool punched = hz.soulNear(hz.souls[0].pos + glm::vec3{1, 0, 0}, 2.5f) == 0;
        CHECK(punched, "a soul passing within reach can be punched");
        hz.clear(); hz.releaseSoul(dead, to);
        std::vector<Soul> back; float flown = 0.f;
        for (int f = 0; f < 60 * 6 && back.empty(); ++f) { auto a = hz.arrived(DT); flown += DT; back.insert(back.end(), a.begin(), a.end()); }
        CHECK(back.size() == 1 && std::fabs(flown - RelicHazards::SOUL_TIME) < 0.1f && glm::length(back[0].pos - to) < 0.1f,
              "an uncaught soul arrives after about 4 s at its spot");
        Enemy twice(EnemyType::REVENANT, yard); twice.reforms = 2;
        CHECK(hz.releaseSoul(twice, to) == nullptr, "after two re-forms, the third death is final");
        // Review focus 4: twins each flee
        Enemy parent(EnemyType::REVENANT, yard); parent.setHollow(Hollow::TWINNED); parent.reforms = 1;
        auto twins = twinsOf(parent);
        hz.clear();
        bool splitNoSoul = hz.releaseSoul(parent, to) == nullptr;
        CHECK(splitNoSoul && twins.size() == 2 && twins[0].reforms == 1 && twins[1].reforms == 1 &&
              hz.releaseSoul(twins[0], to) && hz.releaseSoul(twins[1], to) && hz.souls.size() == 2 && hz.souls[0].reforms == 2,
              "a Twinned Revenant's soul goes into its twins: each carries its own, with the lives the parent had left");
    }

    // ---------------------------------------------------------------- the Weaver
    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& R = N.arenas[3];
        N.formation.reset(N); N.updateMovers(0.f);
        glm::vec3 core = N.formation.top(3);
        Enemy wv(EnemyType::WEAVER, core + glm::vec3{0, 0, -5}); wv.spawnTimer = 0.f; wv.state = EnemyState::ACTIVE;
        EnemyWorld w; w.playerFeet = core + glm::vec3{0, 0, 6}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = R.bounds; w.playerVel = {4, 0, 0};
        int strung = 0; float tell = 0.f; glm::vec3 A{0.f}, Bw{0.f};
        for (int f = 0; f < 60 * 14; ++f) {
            wv.update(DT, w);
            if (wv.ev.telegraphStarted && wv.attack == AttackKind::STRING) tell = wv.telegraphDuration;
            if (wv.ev.wire) { ++strung; A = wv.ev.wireA; Bw = wv.ev.wireB; }
        }
        CHECK(strung >= 2 && std::fabs(tell - 0.8f * difficulty(DIFFICULTY_DEFAULT).windup) < 0.02f, "a Weaver strings a wire every ~6 s after a 0.8 s tell (x the difficulty's wind-up)");
        CHECK(std::fabs(A.y - (w.playerFeet.y + RelicHazards::WIRE_HEIGHT)) < 0.05f && std::fabs(glm::length(Bw - A) - RelicHazards::WIRE_LEN) < 0.1f,
              "its wire runs 10 m across your way at chest height");
        RelicHazards hz;
        glm::vec3 F{0, 0, 0};
        hz.addWire(1, {-5, 1.2f, 0}, {5, 1.2f, 0});
        // Review focus 3: not armed on the tick it's strung
        int fresh = hz.touchedWire(F, 1.8f, 0.4f);
        hz.age(DT * 7);
        int standing = hz.touchedWire(F, 1.8f, 0.4f), sliding = hz.touchedWire(F, 0.9f, 0.4f), jumped = hz.touchedWire(F + glm::vec3{0, 1.4f, 0}, 1.8f, 0.4f),
            beside = hz.touchedWire(F + glm::vec3{0, 0, 2}, 1.8f, 0.4f);
        CHECK(fresh < 0, "a fresh wire doesn't snare where it's strung through you");
        CHECK(standing == 0 && sliding < 0 && jumped < 0 && beside < 0, "standing in a wire snares you; sliding under or jumping over doesn't");
        for (int k = 0; k < 4; ++k) hz.addWire(1, {-5, 1.2f, 3.f + k}, {5, 1.2f, 3.f + k});
        CHECK(hz.wires.size() == 4 && hz.wires[0].a.z == 3.f, "at most 4 wires per Weaver: a fifth replaces its oldest");
        float t = 0.f; int node = hz.rayNode({5, 1.2f, 10}, {0, 0, -1}, 50.f, t);
        if (node >= 0) hz.cutWire(node);
        CHECK(node >= 0 && hz.wires.size() == 3, "shooting a wire's node cuts it");
        hz.addWire(2, {0, 1.2f, -5}, {0, 1.2f, 5});
        hz.dropOwner(1);
        CHECK(hz.wires.size() == 1 && hz.wires[0].owner == 2, "a Weaver's death drops all its wires (and only its)");
        hz.clearWires();
        CHECK(hz.wires.empty(), "a drift clears every wire");
    }

    {   // Review focus 2: a Revenant lost to the void doesn't come back; killed any other way, its soul flees
        CHECK(!revenantSoulEscapes(StyleSource::ENVIRONMENT, true) && revenantSoulEscapes(StyleSource::ENVIRONMENT, false) &&
              revenantSoulEscapes(StyleSource::FRIENDLY, false) && revenantSoulEscapes(StyleSource::REVOLVER, false),
              "a Revenant lost to the void doesn't come back; killed any other way, its soul flees");
    }

    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& R = N.arenas[3]; N.formation.reset(N); N.updateMovers(0.f);
        auto pressure = [&](EnemyType t, glm::vec3 feet) {
            // From the nearest wave-1 spawn on another relic (waves spawn on every relic)
            int mine = -1;
            for (int c = 0; c < (int)N.formation.chunks.size(); ++c) {
                glm::vec3 tp = N.formation.top(c); glm::vec2 h = N.formation.chunks[c].half;
                if (std::fabs(feet.x - tp.x) <= h.x && std::fabs(feet.z - tp.z) <= h.y) mine = c;
            }
            glm::vec3 from{0.f}; float best = 1e9f;
            for (auto& sp : R.waveGround[0]) {
                glm::vec3 tp = N.formation.top(mine); glm::vec2 h = N.formation.chunks[mine].half;
                bool same = std::fabs(sp.x - tp.x) <= h.x && std::fabs(sp.z - tp.z) <= h.y;
                if (!same && glm::length(sp - feet) < best) { best = glm::length(sp - feet); from = sp; }
            }
            Enemy e(t, from); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
            EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
            w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = R.bounds;
            w.dynWalls = N.moverWalls.data(); w.dynCount = (int)N.moverWalls.size(); w.voidUnder = true;
            ProjectileSystem ps; ps.floorY = -600.f; std::vector<Enemy> none;
            for (int f = 0; f < 60 * 10; ++f) {
                e.floorY = N.enemyFloor(3, e.position, false, N.groundAt(e.position.x, e.position.z, e.position.y + 0.5f));
                e.update(DT, w);
                if (e.ev.meleeHit || e.ev.wire) return true;
                for (int k = 0; k < e.ev.shots; ++k) ps.fire(e.ev.shotOrigin, e.ev.shotDir[k] * e.ev.shotSpeed, e.ev.shotDamage, false);
                if (ps.update(DT, N.walls.data(), (int)N.walls.size(), none, w.playerEye, &g).hitPlayer) return true;
            }
            return false;
        };
        glm::vec3 corner = N.formation.top(0) + glm::vec3{-8.5f, 0, 8.5f}   /* the far corner, clear of the lamp post */, far = N.formation.top(1) + glm::vec3{-6, 0, 0};
        std::printf("      pressure: revenant corner %d far %d, weaver corner %d far %d\n", (int)pressure(EnemyType::REVENANT, corner), (int)pressure(EnemyType::REVENANT, far), (int)pressure(EnemyType::WEAVER, corner), (int)pressure(EnemyType::WEAVER, far));
        CHECK(pressure(EnemyType::REVENANT, corner) && pressure(EnemyType::REVENANT, far) &&
              pressure(EnemyType::WEAVER, corner) && pressure(EnemyType::WEAVER, far),
              "neither camping a corner nor keeping away escapes a Revenant or a Weaver for 10 s");
    }

    {   // Walkers chasing you across the void stay on their relic (they don't walk off the edge)
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& R = N.arenas[3]; N.formation.reset(N); N.updateMovers(0.f);
        bool stayed = true;
        for (EnemyType t : {EnemyType::REVENANT, EnemyType::WEAVER, EnemyType::HUSK, EnemyType::RIPPER}) {
            Enemy e(t, N.formation.top(3)); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
            glm::vec3 feet = N.formation.top(1) + glm::vec3{-6, 0, 0};
            EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
            w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = R.bounds;
            w.dynWalls = N.moverWalls.data(); w.dynCount = (int)N.moverWalls.size(); w.voidUnder = true;
            for (int f = 0; f < 60 * 10; ++f) {
                e.floorY = N.enemyFloor(3, e.position, false, N.groundAt(e.position.x, e.position.z, e.position.y + 0.5f));
                e.update(DT, w);
            }
            if (e.position.y < N.formation.top(3).y - 0.5f) { stayed = false; std::printf("      %s fell off its relic\n", statsOf(t).name); }
        }
        CHECK(stayed, "walkers chasing you across the void stay on their relic");
    }

    // ---------------------------------------------------------------- review fixes: the Reliquary
    {
        RelicHazards hz; Enemy r(EnemyType::REVENANT, {0, 0, 0}); r.maxHealth = r.health = 160.f;
        hz.releaseSoul(r, {20, 0, 0});
        CHECK(hz.alive() == 1, "a soul in flight still counts as alive (the wave isn't over, the relics don't drift)");
        // A wire for a player standing still goes up beside them, not through them
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        N.formation.reset(N); N.updateMovers(0.f);
        glm::vec3 core = N.formation.top(3);
        Enemy wv(EnemyType::WEAVER, core + glm::vec3{0, 0, -5}); wv.spawnTimer = 0.f; wv.state = EnemyState::ACTIVE;
        EnemyWorld w; w.playerFeet = core + glm::vec3{0, 0, 6}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = N.arenas[3].bounds; w.playerVel = {0, 0, 0};
        bool strung = false, through = false;
        for (int f = 0; f < 60 * 10; ++f) {
            wv.update(DT, w);
            if (wv.ev.wire) { strung = true; Wire t{1, wv.ev.wireA, wv.ev.wireB, 1.f}; through |= RelicHazards::wireTouches(t, w.playerFeet, 1.8f, 0.4f); }
        }
        CHECK(strung && !through, "a wire for a player standing still goes up beside them, never through them");
        // The snare holds you to a crawl whatever pushes you (a grapple, a slide)
        Player p({0.f, 0.f, 0.f});
        p.speedCap = 3.f; p.velocity = {20.f, 0.f, 0.f};
        Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
        float x0 = p.position.x;
        p.update(DT, k, nullptr, 0, false, nullptr);
        CHECK(p.position.x - x0 <= 3.f * DT + 1e-4f, "snared, you move at a crawl however fast you were pushed");
    }

    // ---------------------------------------------------------------- the lift (driven movers) and the director's hold
    {
        LevelData L; LevelBuilder B{L};
        int m0 = B.mover({0.f, -80.5f, 0.f}, {10.f, 0.5f, 10.f}, Mover::Path::DRIVEN, {0.f, 0.f, 0.f}, {0.f, -160.f, 0.f}, 1.f, 0.f, {1, 1, 1});
        int m1 = B.mover({12.f, -80.5f, 0.f}, {2.f, 0.5f, 5.f}, Mover::Path::DRIVEN, {0.f, 0.f, 0.f}, {0.f, -160.f, 0.f}, 1.f, 0.f, {1, 1, 1});
        L.lift.movers = {m0, m1};
        L.lift.stops = {-80.f, -120.f, -160.f, -200.f, -240.f};
        L.lift.reset(); L.lift.update(0.f, L); L.updateMovers(0.f);
        bool atTop = std::fabs(L.walls[L.movers[m0].wall].box.max.y - (-80.f)) < 1e-3f;
        L.lift.request(1);
        bool waits = L.lift.busy() && !L.lift.riding();
        L.lift.start();
        float clock = 0.f, maxStep = 0.f, prevY = L.lift.y();
        bool together = true;
        for (int i = 0; i < 60 * 9; ++i) {
            clock += DT; L.lift.update(DT, L); L.updateMovers(clock);
            maxStep = std::max(maxStep, std::fabs(L.lift.y() - prevY)); prevY = L.lift.y();
            together &= std::fabs(L.walls[L.movers[m0].wall].box.max.y - L.walls[L.movers[m1].wall].box.max.y) < 1e-4f;
        }
        bool arrived = !L.lift.busy() && L.lift.at == 1 && std::fabs(L.walls[L.movers[m0].wall].box.max.y - (-120.f)) < 1e-3f;
        CHECK(atTop && waits, "the cage starts at the top; a requested ride waits until it's started");
        CHECK(arrived && together, "a ride takes the whole cage down to the next stop together, within 9 s");
        CHECK(maxStep < 0.25f, "the ride eases (never more than 15 m/s)");
        L.lift.reset(); L.lift.update(0.f, L); L.updateMovers(clock);
        CHECK(L.lift.at == 0 && std::fabs(L.walls[L.movers[m0].wall].box.max.y - (-80.f)) < 1e-3f, "a reset puts the cage back at the top");

        LevelData A = buildLevel();
        WaveDirector d; d.level = &A; d.startArena(0);
        d.hold = true;
        std::vector<SpawnRequest> out;
        for (int i = 0; i < 60 * 10; ++i) d.update(DT, 0, A.arenas[0].playerStart, out);
        bool held = d.phase == WaveDirector::Phase::INTRO && out.empty();
        d.hold = false;
        for (int i = 0; i < 60 * 4; ++i) d.update(DT, 0, A.arenas[0].playerStart, out);
        CHECK(held && d.phase == WaveDirector::Phase::ACTIVE, "the director holds the wave while told to, then starts it");
    }

    // ---------------------------------------------------------------- the PENITENT
    {
        auto world = [](glm::vec3 feet) { EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0}; return w; };
        const glm::vec3 home{0.f, -240.f, -864.f};
        {   // chained: never moves, takes a quarter
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            for (int i = 0; i < 60 * 12; ++i) p.update(DT, world({0.f, -240.f, -846.f}));
            CHECK(glm::length(glm::vec2{p.position.x - home.x, p.position.z - home.z}) < 0.01f, "PENITENT: chained, it never moves");
            CHECK(std::fabs(p.armorMult() - 0.25f) < 1e-4f, "PENITENT: chained, it takes a quarter of the damage");
            CHECK(std::fabs(p.height() - 8.2f) < 1e-3f, "PENITENT: kneeling, 8.2 m");
        }
        {   // it attacks with both sweeps, a slam, and drops incense
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            int low = 0, high = 0, slams = 0, incense = 0; bool tell = false, window = false;
            for (int i = 0; i < 60 * 40; ++i) {
                p.update(DT, world({0.f, -240.f, -852.f}));
                low += p.ev.penSweep == 0; high += p.ev.penSweep == 1; slams += p.ev.penSlam; incense += p.ev.penIncense;
                tell |= p.ev.telegraphStarted; window |= p.parryWindow();
            }
            CHECK(low > 0 && high > 0 && slams > 0, "PENITENT: low sweeps, high sweeps and slams");
            CHECK(tell && window, "PENITENT: every blow has a wind-up, with a parry window at its end");
            CHECK(incense >= 6, "PENITENT: incense every ~12 s, three pools at a time");
        }
        {   // the lash: only after 3 s far away or up high
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            bool lashedClose = false;
            for (int i = 0; i < 60 * 10; ++i) { p.update(DT, world({0.f, -240.f, -850.f})); lashedClose |= p.ev.penLash; }
            float firstLash = -1.f;
            for (int i = 0; i < 60 * 6 && firstLash < 0.f; ++i) { p.update(DT, world({0.f, -240.f, -830.f + 0.f * i + 4.f})); if (p.ev.penLash) firstLash = i * DT; }
            CHECK(!lashedClose, "PENITENT: no lash while you fight it up close on the floor");
            CHECK(firstLash >= 2.9f && firstLash < 4.6f, "PENITENT: stay 22 m away for 3 s and the chain lashes at you");
            Enemy q(EnemyType::PENITENT, home, -240.f); q.anchorsLeft = 6; q.spawnTimer = 0.f; q.state = EnemyState::ACTIVE;
            bool perched = false;
            for (int i = 0; i < 60 * 5; ++i) { q.update(DT, world({0.f, -233.f, -850.f})); perched |= q.ev.penLash; }
            CHECK(perched, "PENITENT: perch up high and the chain finds you there too");
        }
        {   // unchained: it rises and walks; scourge: faster, and it lashes itself
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 0; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            bool rose = false; glm::vec3 start = p.position;
            for (int i = 0; i < 60 * 8; ++i) { p.update(DT, world({0.f, -240.f, -830.f})); rose |= p.ev.penRose; }
            CHECK(rose && p.risen && glm::length(p.position - start) > 4.f && std::fabs(p.height() - 11.5f) < 1e-3f,
                  "PENITENT: with every chain broken it rises (11.5 m) and stalks you");
            CHECK(std::fabs(p.armorMult() - 1.f) < 1e-4f, "PENITENT: unchained, it takes full damage");
            int summons = 0;
            for (int i = 0; i < 60 * 30; ++i) { p.update(DT, world({0.f, -240.f, -830.f})); summons += p.ev.penSummon; }
            CHECK(summons >= 4, "PENITENT: risen, it calls up Hollowed Husks every 25 s");
            p.health = p.maxHealth * 0.2f;
            int embers = 0; float firstTell = -1.f;
            for (int i = 0; i < 60 * 15; ++i) {
                p.update(DT, world({0.f, -240.f, -850.f}));
                embers += p.ev.penEmbers;
                if (p.ev.telegraphStarted && firstTell < 0.f && p.attack != AttackKind::SCOURGE) firstTell = p.telegraphDuration;
            }
            CHECK(p.scourging && embers >= 1, "PENITENT: under a quarter it scourges itself, throwing rings of embers");
            CHECK(firstTell > 0.f && firstTell < 0.72f, "PENITENT: scourging, its wind-ups come 30% sooner (a sweep 0.63 s, a slam 0.7 s)");
        }
        {   // a parry staggers it: no blows during the stagger, double damage
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 0; p.risen = true; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            p.stagger(p.staggerTime());
            bool hit = false;
            for (int i = 0; i < 60 * 2; ++i) { p.update(DT, world({0.f, -240.f, -858.f})); hit |= p.ev.penSweep >= 0 || p.ev.penSlam || p.ev.penStomp; }
            CHECK(std::fabs(p.staggerTime() - 2.5f) < 1e-4f && !hit, "PENITENT: parried, it reels for 2.5 s and strikes nothing");
            CHECK(std::fabs(p.armorMult() - 2.f) < 1e-4f || !p.staggered(), "PENITENT: reeling, it takes double");
        }
    }

    // ---------------------------------------------------------------- the PENITENT's body
    {
        Enemy p(EnemyType::PENITENT, {0.f, -240.f, -864.f}, -240.f); p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE; p.anchorsLeft = 6;
        std::vector<BoxInstance> boxes; buildEnemy(p, 0.f, boxes);
        AABB head, wound;
        bool hasHead = headBox(p, head), hasWound = woundBox(p, wound);
        CHECK(boxes.size() >= 30 && hasHead && !hasWound, "PENITENT: a big rig, a head, no wound until it scourges");
        CHECK(head.min.y > -240.f + 5.f && head.max.y < -240.f + 9.5f, "PENITENT: kneeling, its head is 5-9.5 m up");
        p.anchorsLeft = 0; p.risen = true;
        headBox(p, head);
        CHECK(head.min.y > -240.f + 8.5f, "PENITENT: risen, its head is up at 8.5 m+");
        p.scourging = true;
        bool w = woundBox(p, wound);
        glm::vec3 back{-std::sin(p.yaw), 0.f, -std::cos(p.yaw)};
        glm::vec3 wc = (wound.min + wound.max) * 0.5f;
        CHECK(w && glm::dot(glm::vec2{wc.x - p.position.x, wc.z - p.position.z}, glm::vec2{back.x, back.z}) > 0.5f,
              "PENITENT: scourging, the wound is on its back");
    }

    // ---------------------------------------------------------------- the Penitent's hazards
    {
        using PH = PenitentHazards;
        const glm::vec3 c{0.f, -240.f, -864.f};
        const float yaw = 3.14159265f;   // facing +Z... (yaw 0 faces +Z): yaw pi faces -Z; the player is at +Z, so use 0
        const glm::vec3 front = c + glm::vec3{0.f, 0.f, 8.f};
        CHECK(PH::sweepHits(0, c, 0.f, 16.f, front, 1.8f, -240.f), "a low sweep hits you standing");
        CHECK(!PH::sweepHits(0, c, 0.f, 16.f, front + glm::vec3{0, 1.2f, 0}, 1.8f, -240.f), "...and passes under you mid-jump");
        CHECK(PH::sweepHits(1, c, 0.f, 16.f, front, 1.8f, -240.f), "a high sweep hits you standing");
        CHECK(PH::sweepHits(1, c, 0.f, 16.f, front + glm::vec3{0, 1.2f, 0}, 1.8f, -240.f), "...and mid-jump");
        CHECK(!PH::sweepHits(1, c, 0.f, 16.f, front, 0.9f, -240.f), "...but passes over you sliding");
        CHECK(!PH::sweepHits(0, c, 0.f, 16.f, c + glm::vec3{0, 0, -8.f}, 1.8f, -240.f), "a sweep misses behind it");
        CHECK(!PH::sweepHits(0, c, 0.f, 16.f, c + glm::vec3{0, 0, 17.f}, 1.8f, -240.f), "...and beyond its reach");
        (void)yaw;
        // A slam ring hits once, as its edge passes; not if you're in the air
        PH h; h.addRing(c, PH::SLAM_RADIUS, PH::SLAM_DAMAGE, 0);
        int hits = 0; float t = 0.f;
        for (int i = 0; i < 120; ++i) { auto hs = h.update(DT, c + glm::vec3{0, 0, 8.f}, -240.f); hits += (int)hs.size(); t += DT; }
        PH j; j.addRing(c, PH::SLAM_RADIUS, PH::SLAM_DAMAGE, 0);
        int airHits = 0;
        for (int i = 0; i < 120; ++i) airHits += (int)j.update(DT, c + glm::vec3{0, 1.5f, 8.f}, -240.f).size();
        CHECK(hits == 1 && airHits == 0 && h.rings.empty(), "a slam's ring hits you once on the floor, never in the air, then is gone");
        // The lash: warned, then it hits along its line (and only there)
        PH l; l.addLash(c, {0.f, 0.f, 1.f}, true);
        int before = 0, on = 0;
        for (int i = 0; i < 30; ++i) before += (int)l.update(DT, c + glm::vec3{0, 0, 20.f}, -240.f).size();   // 0.5 s: still the warning
        std::vector<PH::Hit> hs;
        for (int i = 0; i < 30; ++i) { auto x = l.update(DT, c + glm::vec3{0.5f, 0, 20.f}, -240.f); on += (int)x.size(); if (!x.empty()) hs = x; }
        PH l2; l2.addLash(c, {0.f, 0.f, 1.f}, false);
        int off = 0;
        for (int i = 0; i < 60; ++i) off += (int)l2.update(DT, c + glm::vec3{5.f, 0, 20.f}, -240.f).size();
        CHECK(before == 0 && on == 1 && off == 0 && !hs.empty() && hs[0].yank && std::fabs(hs[0].damage - 35.f) < 1e-4f,
              "the lash waits out its warning, hits once along its line (and can yank), misses beside it");
        // Incense: harmless while it lands, 20/s inside, nothing outside, gone after 8 s
        PH p; p.addPool(c);
        float in = 0.f, out = 0.f;
        for (int i = 0; i < 60 * 9; ++i) {
            for (auto& x : p.update(DT, c + glm::vec3{1.f, 0, 0}, -240.f)) in += x.damage;
        }
        PH q; q.addPool(c);
        for (int i = 0; i < 60 * 9; ++i) for (auto& x : q.update(DT, c + glm::vec3{5.f, 0, 0}, -240.f)) out += x.damage;
        CHECK(std::fabs(in - 20.f * (8.f - 0.6f)) < 6.f && out == 0.f && p.pools.empty(),
              "incense burns 20/s inside it after landing, nothing outside, and fades after 8 s");
    }

    // ---------------------------------------------------------------- review fixes: the Penitent in the game's world
    {
        LevelData N = buildAct2Level();
        SpatialGrid g; g.build(N.walls);
        N.lift.reset(); N.lift.at = N.lift.to = 4; N.lift.update(0.f, N); N.updateMovers(0.f);   // the cage docked in the pit
        const Arena& D = N.arenas[2];
        // 1. Its floor is the pit, not the void below the shaft
        glm::vec3 bp = D.bossSpawn;
        float pf = N.enemyFloor(2, bp, false, N.groundAt(bp.x, bp.z, bp.y + 0.5f));
        CHECK(std::fabs(pf - (-240.f)) < 1e-3f, "in the Descent the Penitent stands on the pit's floor (-240), not the void (-300)");
        N.lift.at = N.lift.to = 1; N.lift.update(0.f, N); N.updateMovers(0.f);
        glm::vec3 air = D.waveAir[0][0];
        CHECK(N.enemyFloor(2, air, true, N.groundAt(air.x, air.z, air.y + 0.5f)) >= -120.f - 1e-3f,
              "fliers in the Descent hover over the cage's stop, not the bottom of the shaft");
        // With that floor, a chained Penitent 10 m away hits a standing player
        N.lift.at = N.lift.to = 4; N.lift.update(0.f, N); N.updateMovers(0.f);
        Enemy p(EnemyType::PENITENT, bp, -300.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
        glm::vec3 feet = bp + glm::vec3{0.f, 0.f, 10.f};
        EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
        int sweeps = 0, hits = 0, lashes = 0;
        for (int i = 0; i < 60 * 40; ++i) {
            p.floorY = N.enemyFloor(2, p.position, false, N.groundAt(p.position.x, p.position.z, p.position.y + 0.5f));
            p.update(DT, w);
            if (p.ev.penSweep >= 0) { ++sweeps; hits += PenitentHazards::sweepHits(p.ev.penSweep, p.position, p.yaw, p.sweepReach(), feet, 1.8f, p.floorY); }
            lashes += p.ev.penLash;
        }
        CHECK(sweeps > 0 && hits == sweeps && lashes == 0, "fed the game's floor, its sweeps reach a player standing in front of it, and it doesn't lash in melee");
        // 2. Walkers step onto the cage and stand on it
        Enemy h(EnemyType::BRUTE, glm::vec3{0.f, -240.f, -840.f + 20.f}, -240.f); h.spawnTimer = 0.f; h.state = EnemyState::ACTIVE;
        EnemyWorld hw; hw.walls = N.walls.data(); hw.wallCount = (int)N.walls.size(); hw.grid = &g;
        hw.dynWalls = N.moverWalls.data(); hw.dynCount = (int)N.moverWalls.size();
        hw.playerFeet = {0.f, -240.f, -840.f}; hw.playerEye = hw.playerFeet + glm::vec3{0, 1.7f, 0};
        bool onCage = false;
        for (int i = 0; i < 60 * 8; ++i) {
            h.floorY = N.enemyFloor(2, h.position, false, N.groundAt(h.position.x, h.position.z, h.position.y + 0.5f));
            h.update(DT, hw);
            onCage |= glm::length(glm::vec2{h.position.x, h.position.z + 840.f}) < 11.f && std::fabs(h.position.y + 240.f) < 0.2f;
        }
        CHECK(onCage, "a ledge-wary Brute walks off the pit floor onto the docked cage and stands on it");
    }
    {   // 3. Phase 1 has no safe distance: between its reach and 22 m, and across the whole pit
        const glm::vec3 home{0.f, -240.f, -864.f};
        auto lashAt = [&](float dz, float dy) {
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            EnemyWorld w; w.playerFeet = home + glm::vec3{0.f, dy, dz}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
            for (int i = 0; i < 60 * 6; ++i) {
                p.update(DT, w);
                if (p.ev.penLash) {
                    PenitentHazards hz; hz.addLash(p.ev.penLashFrom, p.ev.penLashDir, false);
                    int hit = 0;
                    for (int k = 0; k < 60; ++k) hit += (int)hz.update(DT, w.playerFeet, -240.f).size();
                    return hit;
                }
            }
            return -1;
        };
        CHECK(lashAt(19.f, 0.f) == 1, "chained, stand just out of its sweeps (19 m) and the chain finds you");
        CHECK(lashAt(54.f, 0.f) == 1, "...and across the whole pit (54 m)");
        CHECK(lashAt(10.f, 9.f) == 1, "...and perched 9 m up, close in");
    }
    {   // 4. Ripping: hook an anchor from anywhere; the hook letting go as you arrive still counts
        AnchorRip r;
        int broke = -1; float t = 0.f;
        for (int i = 0; i < 60 && broke < 0; ++i, t += DT) {
            bool active = t < 0.24f;                    // the hook lets go on arrival (2 m)
            float dist = std::max(1.5f, 11.f - 45.f * t);
            broke = r.update(DT, active ? 3 : -1, active, dist);
        }
        CHECK(broke == 3 && t >= 0.49f && t < 0.55f, "an anchor hooked from 11 m rips out after half a second (hanging on at the anchor counts)");
        AnchorRip c;
        int early = -1;
        for (int i = 0; i < 60; ++i) {
            float tt = i * DT; bool active = tt < 0.2f;
            int x = c.update(DT, active ? 3 : -1, active, 15.f);   // let go early, far away
            if (x >= 0) early = x;
        }
        CHECK(early == -1, "letting go of the hook early, far from the anchor, rips nothing");
    }
    {   // 5. The wound counts only from behind
        Enemy p(EnemyType::PENITENT, {0.f, -240.f, -864.f}, -240.f); p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
        p.anchorsLeft = 0; p.risen = true; p.scourging = true; p.yaw = 0.f;   // facing +Z
        AABB wb; woundBox(p, wb); glm::vec3 wc = (wb.min + wb.max) * 0.5f;
        float t;
        glm::vec3 front = wc + glm::vec3{0.f, 0.f, 20.f}, back = wc - glm::vec3{0.f, 0.f, 20.f};
        CHECK(!woundShot(p, front, glm::normalize(wc - front), t), "a shot from the front never counts as the wound");
        CHECK(woundShot(p, back, glm::normalize(wc - back), t), "a shot from behind finds the wound");
    }
    {   // 6. No sweep starts on top of its ember ring
        Enemy p(EnemyType::PENITENT, {0.f, -240.f, -864.f}, -240.f); p.anchorsLeft = 0; p.risen = true;
        p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE; p.health = p.maxHealth * 0.2f;
        EnemyWorld w; w.playerFeet = {0.f, -240.f, -852.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        float sinceEmbers = 99.f; bool clash = false;
        for (int i = 0; i < 60 * 30; ++i) {
            p.update(DT, w);
            sinceEmbers += DT;
            if (p.ev.penEmbers) sinceEmbers = 0.f;
            if (p.ev.telegraphStarted && p.attack != AttackKind::SCOURGE && sinceEmbers < 1.0f) clash = true;
        }
        CHECK(!clash, "after it scourges itself, a second passes before its next blow (no jump-and-slide at once)");
    }

    // ---------------------------------------------------------------- the arsenal: one motion grammar
    {
        // Kick: instant, then settles within each gun's `settle`
        bool settles = true;
        for (int g = 0; g < 4; ++g) {
            GunMotion m; m.gun = g; m.fire();
            float at0 = m.kickAmount();
            float t = 0.f;
            while (t < gunWeight(g).settle) { m.update(DT * 0.25f); t += DT * 0.25f; }
            settles &= at0 > 0.99f && m.kickAmount() < 0.03f;
        }
        CHECK(settles, "every gun kicks at once and settles within its weight's time");
        CHECK(gunWeight(0).settle < gunWeight(1).settle && gunWeight(1).settle <= gunWeight(3).settle &&
              gunWeight(2).settle < gunWeight(3).settle, "light guns snap back sooner than heavy ones");
        // Switch: the old gun until it's down, the new one rising, ~0.32 s, one chime
        GunMotion s; s.gun = 0;
        s.switchTo(2);
        float t = 0.f; bool oldFirst = true; int chimes = 0; float done = -1.f;
        for (int i = 0; i < 240 && done < 0.f; ++i) {
            s.update(DT * 0.25f); t += DT * 0.25f;
            if (t < SWITCH_DOWN - 0.005f) oldFirst &= s.shownGun() == 0;
            for (auto& c : s.cues) chimes += std::string(c.name) == "switch_up2";
            s.cues.clear();
            GunPose p = s.pose(false);
            if (t > SWITCH_DOWN + SWITCH_UP && glm::length(p.offset) < 1e-3f) done = t;
        }
        CHECK(oldFirst && s.shownGun() == 2 && chimes == 1 && done > 0.3f && done < 0.36f,
              "a switch lowers the old gun, raises the new one with one chime, in about 0.32 s");
        // Reload: three beats; cells go dark on OPEN (revolver ejects), relight evenly during FEED, all lit at CLOSE
        GunMotion r; r.gun = 0; r.reload(1.2f, 0, 8, 8);
        std::vector<float> relit; int lastLit = r.litCells(0, 8), maxLit = 0; float tt = 0.f;
        std::vector<std::string> order;
        while (r.reloading()) {
            r.update(DT * 0.25f); tt += DT * 0.25f;
            int lit = r.litCells(0, 8);
            if (lit > lastLit) relit.push_back(tt / 1.2f);
            lastLit = lit; maxLit = std::max(maxLit, lit);
            for (auto& c : r.cues) order.push_back(c.name);
            r.cues.clear();
        }
        bool even = relit.size() == 8 && relit.front() >= BEAT_OPEN - 0.01f && relit.back() <= BEAT_FEED_END + 0.01f;
        for (size_t i = 2; i < relit.size() && even; ++i)
            even &= std::fabs((relit[i] - relit[i - 1]) - (relit[1] - relit[0])) < 0.02f;
        CHECK(even, "a reload relights its cells one by one, evenly, inside the FEED beat");
        CHECK(maxLit == 8 && r.litCells(0, 8) == 0, "all eight relight; after the reload the gun shows what the game says (ammo)");
        int opens = 0, closes = 0, cells = 0; bool inOrder = !order.empty() && order.front() == "cyl_open" && order.back() == "cyl_close";
        for (auto& n : order) { opens += n == "cyl_open"; closes += n == "cyl_close"; cells += n == "cell"; }
        CHECK(inOrder && opens == 1 && closes == 1 && cells == 8, "reload cues: open first, a tick per cell, close last, each once");
        // A switch cancels a reload: no more cues, back to rest
        GunMotion c; c.gun = 1; c.reload(1.4f, 0, 2, 2);
        for (int i = 0; i < 20; ++i) c.update(DT);
        c.cues.clear(); c.switchTo(3);
        bool quiet = true;
        for (int i = 0; i < 60; ++i) { c.update(DT); for (auto& q : c.cues) quiet &= std::string(q.name) == "switch_up3"; c.cues.clear(); }
        CHECK(quiet && !c.reloading() && c.pose().reloadU < 0.f, "switching mid-reload drops the reload and its sounds");
        // A second switch mid-switch restarts cleanly to the newest gun
        GunMotion d; d.gun = 0; d.switchTo(1);
        for (int i = 0; i < 4; ++i) d.update(DT);
        d.switchTo(2);
        for (int i = 0; i < 40; ++i) d.update(DT);
        CHECK(d.shownGun() == 2 && glm::length(d.pose(false).offset) < 1e-3f, "switching again mid-switch lands on the newest gun");
        // Cycling cues: the shotgun pumps, the rifles work the bolt
        GunMotion b; b.gun = 2; b.cycle(0.7f);
        int bolts = 0; for (int i = 0; i < 60; ++i) { b.update(DT); for (auto& q : b.cues) bolts += std::string(q.name) == "bolt"; b.cues.clear(); }
        CHECK(bolts == 1, "after a rifle shot the bolt sounds once, on its beat");
    }

    // ---------------------------------------------------------------- the arsenal: one look
    {
        GunPose rest;
        bool cellsOk = true, litOk = true, glowOk = true;
        for (int g = 0; g < 4; ++g)
            for (int mag : {1, 2, 4, 5, 8, 10, 14, 16})
                for (int ammo : {0, mag / 2, mag}) {
                    GunLook L; L.mag = mag; L.ammo = ammo; L.lit = ammo;
                    std::vector<GunPart> parts; gunkit::buildGun(g, L, rest, parts);
                    int lit = 0, dark = 0;
                    for (auto& p : parts) { lit += p.tag == 1; dark += p.tag == 2; }
                    if (lit + dark != mag) { std::printf("      gun %d mag %d: %d cells\n", g, mag, lit + dark); cellsOk = false; }
                    litOk &= lit == ammo;
                    if (ammo > 0) {
                        bool any = false;
                        for (auto& p : parts)
                            if (p.tag == 1) any |= glm::length(glm::normalize(p.emissive) - glm::normalize(gunkit::glowOf(g))) < 0.01f;
                        glowOk &= any;
                    }
                }
        CHECK(cellsOk, "every gun shows one cell per round of its magazine (1-16, upgrades included)");
        CHECK(litOk, "the lit cells are the rounds left");
        CHECK(glowOk, "each gun's cells glow its own colour");
        CHECK(glm::length(gunkit::glowOf(0) - glm::vec3{1.f, 0.68f, 0.22f}) < 1e-4f && glm::length(gunkit::glowOf(2) - glm::vec3{0.3f, 0.9f, 1.f}) < 1e-4f,
              "amber revolver, cyan Lancer");
        // Framing: at the hip, every gun's parts project into the lower-right of a 16:9 screen
        glm::mat4 proj = glm::perspective(glm::radians(65.f), 16.f / 9.f, 0.03f, 10.f);
        bool framed = true; float areas[4];
        for (int g = 0; g < 4; ++g) {
            GunLook L; std::vector<GunPart> parts; gunkit::buildGun(g, L, rest, parts);
            glm::vec3 hip = gunkit::hipOf(g);
            float x0 = 9, x1 = -9, y0 = 9, y1 = -9;
            for (auto& p : parts) {
                glm::vec3 c = glm::vec3(p.xf[3]) + hip;
                if (c.z < 0.06f) continue;   // behind the near plane (the butt of a stock): off screen
                glm::vec4 clip = proj * glm::vec4(c.x, c.y, -c.z, 1.f);    // camera looks down -Z
                glm::vec2 ndc{clip.x / clip.w, clip.y / clip.w};
                x0 = std::min(x0, ndc.x); x1 = std::max(x1, ndc.x); y0 = std::min(y0, ndc.y); y1 = std::max(y1, ndc.y);
            }
            areas[g] = (std::min(x1, 1.f) - std::max(x0, -1.f)) * (std::min(y1, 0.3f) - std::max(y0, -1.f));
            if (x0 < -0.25f || y1 > 0.3f || x1 < 0.2f) { std::printf("      gun %d frame x %.2f..%.2f y %.2f..%.2f\n", g, x0, x1, y0, y1); framed = false; }
        }
        CHECK(framed, "at the hip every gun sits in the lower right, its muzzle toward the crosshair");
        bool similar = true;
        for (int g = 1; g < 4; ++g) similar &= areas[g] > areas[0] * 0.5f && areas[g] < areas[0] * 3.f;
        CHECK(similar, "the four guns fill a similar share of the screen");
        std::vector<GunPart> fist; gunkit::buildFist(gunkit::glowOf(1), 1.f, true, fist);
        bool seam = false; for (auto& p : fist) seam |= glm::length(p.emissive) > 0.1f;
        CHECK(fist.size() >= 8 && seam, "the gauntlet: a fist with a seam glowing in the current gun's colour");
        CHECK(std::string(weaponDef(WeaponId::KAR).name) == "LANCER" && weaponDef(WeaponId::KAR).damage == 120.f &&
              weaponDef(WeaponId::KAR).magSize == 5 && std::fabs(weaponDef(WeaponId::KAR).reloadTime - 2.0f) < 1e-4f,
              "the Kar98 is now the LANCER, with the same stats");
    }

    // ---------------------------------------------------------------- arsenal review fixes
    {
        // 1. A reset (retry, restart) drops whatever the gun was doing
        GunMotion m; m.gun = 3; m.reload(2.8f, 0, 4, 4);
        for (int i = 0; i < 30; ++i) m.update(DT);
        m.switchTo(1); for (int i = 0; i < 3; ++i) m.update(DT);
        m.reset(0);
        m.cues.clear();
        for (int i = 0; i < 180; ++i) m.update(DT);
        CHECK(!m.reloading() && !m.switchingNow() && m.shownGun() == 0 && m.litCells(8, 8) == 8 && m.cues.empty(),
              "a retry resets the gun: no leftover reload or switch, every cell lit, nothing playing");
        // 2. Switching back to a gun mid-reload picks its reload up where it is
        GunMotion b; b.gun = 0;
        b.reload(1.2f, 3, 8, 8, 0.5f);
        bool noEarly = true; int lit = b.litCells(3, 8);
        for (auto& c : b.cues) noEarly &= std::string(c.name) != "cyl_open" && std::string(c.name) != "eject";
        b.update(DT);
        for (auto& c : b.cues) noEarly &= std::string(c.name) != "cyl_open" && std::string(c.name) != "eject";
        int closes = 0;
        for (int i = 0; i < 60 && b.reloading(); ++i) { b.update(DT); for (auto& c : b.cues) closes += std::string(c.name) == "cyl_close"; b.cues.clear(); }
        CHECK(b.pose().reloadU < 0.f && noEarly && closes == 1 && lit >= 3 && lit < 8,
              "switching back mid-reload resumes it halfway: no repeat of its opening, it still closes with its sound");
        // 4. Firing or reloading puts an inspect away
        GunMotion i1; i1.gun = 2; i1.inspect(); for (int k = 0; k < 20; ++k) i1.update(DT);
        i1.fire();
        GunMotion i2; i2.gun = 2; i2.inspect(); for (int k = 0; k < 20; ++k) i2.update(DT);
        i2.reload(2.f, 0, 5, 5);
        CHECK(i1.pose(false).inspectU < 0.f && i2.pose(false).inspectU < 0.f, "shooting or reloading puts the inspect away at once");
        // 5. An upgraded revolver's extra rounds sit where you can see them (its left side)
        GunLook L; L.mag = 14; L.ammo = 14; L.lit = 14;
        std::vector<GunPart> parts; gunkit::buildGun(0, L, GunPose{}, parts);
        int k = 0; bool leftSide = true;
        for (auto& p : parts) if (p.tag) { if (k >= 8) leftSide &= p.xf[3].x < -0.026f; ++k; }
        CHECK(k == 14 && leftSide, "rounds past the eighth show on the revolver's left side, where you can see them");
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

    // ---------------------------------------------------------------- the Maw: the map
    {
        LevelData N = buildAct2Level();
        const Arena& M = N.arenas[4];
        const glm::vec3 C = MAW_C;
        CHECK(M.waves.size() == 1 && M.waves[0][0].type == EnemyType::LEVIATHAN && std::fabs(M.damageScale - 1.5f) < 1e-4f &&
              M.space == ReverbSpace::HALL, "the Maw: one wave, the Leviathan, x1.5, a hall");
        bool ring = true;
        for (int k = 0; k < 16; ++k) {
            float a = k * 0.3926991f + 0.1f;
            for (float rr : {16.f, 24.f, 35.f, 44.f}) {
                glm::vec3 p = C + glm::vec3{std::cos(a) * rr, 0.f, std::sin(a) * rr};
                bool onWell = false;
                for (int w = 0; w < 4; ++w) onWell |= glm::length(glm::vec2{p.x - mawWell(w).x, p.z - mawWell(w).z}) < MAW_WELL;
                float g = N.groundAt(p.x, p.z, C.y + 0.5f);
                if (std::fabs(g - C.y) > 0.05f && !onWell) {
                    bool solidHere = false;   // a rib's pillar, a step or a pipe stands here
                    for (auto& wl : N.walls) solidHere |= p.x > wl.box.min.x && p.x < wl.box.max.x && p.z > wl.box.min.z && p.z < wl.box.max.z && wl.box.max.y > C.y + 0.5f;
                    if (!solidHere) { ring = false; std::printf("      no ring at r %.0f, %.0f deg: %.2f\n", rr, a * 57.3f, g); }
                }
            }
        }
        CHECK(ring, "the ring floor stands at Y -300 from the pool to the wall");
        CHECK(N.groundAt(C.x, C.z, C.y + 0.5f) < C.y - 50.f && N.groundAt(C.x + 6.f, C.z - 6.f, C.y + 0.5f) < C.y - 50.f &&
              M.voidY > C.y - 20.f && M.voidY < C.y - 2.f, "the pool is open: a fall into it is a fall");
        bool wells = N.hasLair && N.wells.size() == 4;
        for (auto& w : N.wells) wells &= std::fabs(glm::length(glm::vec2{w.x - C.x, w.z - C.z}) - MAW_WELL_R) < 0.01f &&
                                         std::fabs(N.groundAt(w.x, w.z, C.y + 0.5f) - C.y) < 0.05f;
        CHECK(wells && std::fabs(N.lairFloor - C.y) < 1e-4f, "four wells on the ring, flush with it; the lair is known to the level");
        bool ledges = true;
        for (int k = 0; k < 6; ++k) {
            float a = mawRibAngle(k);
            glm::vec3 out{std::cos(a), 0.f, std::sin(a)}, side{-out.z, 0.f, out.x};
            glm::vec3 l = C + out * (MAW_RIB_R - 3.8f), st = l + side * 3.6f - out * 1.2f;
            float lt = N.groundAt(l.x, l.z, C.y + 10.f), sh = N.groundAt(st.x, st.z, C.y + 10.f);
            ledges &= std::fabs(lt - (C.y + MAW_LEDGE_UP)) < 0.05f && std::fabs(sh - (C.y + MAW_STEP_UP)) < 0.05f &&
                      sh - C.y <= 2.6f && lt - sh <= 2.6f;
        }
        CHECK(ledges, "six dry ledges at +5 m, each with a step at +2.5 m");
        // Off the rim and down: you land on the ring, in the Maw, never in the Reliquary's void
        SpatialGrid g; g.build(N.walls);
        N.formation.at = N.formation.to = 3; N.formation.apply(N); N.updateMovers(0.f);
        Player p({0.f, -240.f, -1021.5f});
        Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
        k[SDL_SCANCODE_W] = 1;
        bool neverVoid = true;
        for (int i = 0; i < 60 * 6; ++i) {
            applyWater(p, N);
            p.update(DT, k, N.walls.data(), (int)N.walls.size(), false, &g);
            int a = N.arenaAt(p.position);
            if (a >= 0 && p.position.y < N.arenas[a].voidY) neverVoid = false;
            if (p.position.z < -1030.f) k[SDL_SCANCODE_W] = 0;
        }
        std::printf("      off the rim: landed at (%.1f %.1f %.1f) in arena %d\n", p.position.x, p.position.y, p.position.z, N.arenaAt(p.position));
        CHECK(neverVoid && N.arenaAt(p.position) == 4 && p.position.y > C.y - 0.1f && p.position.y < C.y + 3.1f,
              "running off the Reliquary's rim you fall into the Maw and land on its ring");
        CHECK(N.arenaAt(M.playerStart) == 4 && M.playerStart.y > C.y + 20.f && std::fabs(N.groundAt(M.playerStart.x, M.playerStart.z, M.playerStart.y) - C.y) < 0.05f,
              "a retry drops you in again, over the ring");
        CHECK(MUSIC_TRACKS == 10 && std::string(musicTrack(9).name) == "LEVIATHAN", "the Maw has its own track");
    }

    // ---------------------------------------------------------------- the Leviathan: its mind
    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& M = N.arenas[4];
        const glm::vec3 C = MAW_C;
        auto world = [&](glm::vec3 feet, int diff = DIFFICULTY_DEFAULT) {
            EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
            w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = M.bounds;
            w.hasLair = true; w.lair = N.lair; w.lairFloor = N.lairFloor; w.wells = N.wells.data(); w.wellCount = (int)N.wells.size();
            w.tune = &difficulty(diff);
            return w;
        };
        auto fresh = [&]() { Enemy e(EnemyType::LEVIATHAN, M.bossSpawn); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE; return e; };
        glm::vec3 south = C + glm::vec3{0.f, 0.f, 25.f};
        // It rises out of the pool over 3 s, untouchable, then towers over it
        {
            Enemy e = fresh(); EnemyWorld w = world(south);
            bool hidden = true; float upAt = -1.f;
            for (int f = 0; f < 60 * 4; ++f) {
                e.update(DT, w);
                if (f < 60 * 2.9f) hidden &= !e.targetable();
                if (upAt < 0.f && e.targetable()) upAt = f * DT;
            }
            CHECK(hidden && upAt > 2.9f && upAt < 3.1f && e.position.y > C.y + 8.f && e.levAtPool(),
                  "it rises out of the pool over 3 s, untouchable, then towers over it");
        }
        // Its phases
        {
            Enemy e = fresh(); EnemyWorld w = world(south);
            for (int f = 0; f < 60 * 4; ++f) e.update(DT, w);
            e.health = e.maxHealth * 0.64f; e.update(DT, w);
            bool p2 = e.levPhase == 2 && e.ev.lvPhase == 2;
            e.health = e.maxHealth * 0.29f; e.update(DT, w);
            CHECK(p2 && e.levPhase == 3 && e.ev.lvPhase == 3 && statsOf(EnemyType::LEVIATHAN).health == 9000.f,
                  "9000 health; under 65 % it hunts, under 30 % the eclipse");
        }
        // What a hit does where
        {
            Enemy e = fresh(); e.levRoot = e.levHome = N.lair; e.levFloor = C.y; e.levStage = Enemy::LevStage::FIGHT;
            bool base = leviathanZoneMult(e, LevZone::BODY) == 0.2f && leviathanZoneMult(e, LevZone::HEAD) == 1.f &&
                        leviathanZoneMult(e, LevZone::EYE) == 1.f && leviathanZoneMult(e, LevZone::THROAT) == 0.f && e.armorMult() == 1.f;
            e.levBeached = 2.f; bool beached = leviathanZoneMult(e, LevZone::EYE) == 3.f; e.levBeached = 0.f;
            e.levPhase = 3; bool eclipse = leviathanZoneMult(e, LevZone::EYE) == 2.f;
            e.levInhale = 1.f; bool throat = leviathanZoneMult(e, LevZone::THROAT) == 3.f; e.levInhale = 0.f;
            e.stagger(4.f); bool stag = e.armorMult() == 2.f && leviathanZoneMult(e, LevZone::EYE) == 3.f;
            CHECK(base && beached && eclipse && throat && stag,
                  "body x0.2, head x1, the eye x3 beached or staggered and x2 in the eclipse, the throat x3 only while it inhales, x2 staggered");
        }
        // Shots find the zones: the eye from in front, the body from the side
        {
            Enemy e = fresh(); EnemyWorld w = world(south);
            for (int f = 0; f < 60 * 4; ++f) e.update(DT, w);
            glm::vec3 fwd{std::sin(e.yaw), 0.f, std::cos(e.yaw)};
            AABB eye = leviathanEye(e); glm::vec3 ec = (eye.min + eye.max) * 0.5f;
            glm::vec3 from = ec + fwd * 15.f; float t;
            LevZone ze = leviathanRay(e, from, glm::normalize(ec - from), t);
            LevSegment seg[LV_SEGMENTS]; leviathanBody(e, seg);
            glm::vec3 sc = seg[4].c, sfrom = sc + glm::vec3{fwd.z, 0.f, -fwd.x} * 15.f;
            LevZone zb = leviathanRay(e, sfrom, glm::normalize(sc - sfrom), t);
            CHECK(ze == LevZone::EYE && zb == LevZone::BODY, "a shot at the eye from in front finds the eye; one at its neck finds plates");
        }
        // CRASH: locked on where you stood, a 1.1 s tell, it lands there and lies beached 3 s; the parry window is its last quarter second
        {
            Enemy e = fresh(); EnemyWorld w = world(south);
            float tell = 0.f; glm::vec3 marked{0.f}; bool landed = false, beachedThree = false, window = true, openEye = false;
            float landT = -1.f;
            for (int f = 0; f < 60 * 14 && !landed; ++f) {
                e.update(DT, w);
                if (e.ev.lvCrashMark) { tell = e.telegraphDuration; marked = e.ev.lvTo; }
                if (e.attack == AttackKind::CRASH && e.parryWindow() != (e.telegraphTimer < 0.25f)) window = false;
                if (e.attack != AttackKind::CRASH && e.parryWindow()) window = false;
                if (e.ev.lvCrash) { landed = true; landT = f * DT; openEye = e.levEyeOpen(); beachedThree = std::fabs(e.levBeached - Enemy::LV_BEACHED) < 1e-4f; }
            }
            float lieFor = 0.f;
            for (int f = 0; f < 60 * 5; ++f) { e.update(DT, w); if (e.levBeached > 0.f) lieFor += DT; }
            (void)landT;
            CHECK(landed && std::fabs(tell - 1.1f * difficulty(DIFFICULTY_DEFAULT).windup) < 1e-3f && glm::length(glm::vec2{marked.x - south.x, marked.z - south.z}) < 0.1f,
                  "a crash is marked on where you stand and winds up 1.1 s");
            CHECK(beachedThree && openEye && lieFor > 2.9f && lieFor < 3.05f && window,
                  "after a crash its head lies beached 3 s, the eye open; the parry window is the last quarter second of the crash");
        }
        // Hugging its root brings a crash; TORRENT orbs go back into its eye for 180; TIDE rolls out from the pool to the wall
        {
            Enemy e = fresh(); EnemyWorld w = world(C + glm::vec3{0.f, 0.f, 16.f});
            for (int f = 0; f < 60 * 4; ++f) e.update(DT, w);
            AttackKind first = AttackKind::NONE;
            for (int f = 0; f < 60 * 4 && first == AttackKind::NONE; ++f) { e.update(DT, w); if (e.ev.telegraphStarted) first = e.attack; }
            CHECK(first == AttackKind::CRASH, "stand at its pool's lip and it crashes down on you");
            Enemy t = fresh(); EnemyWorld tw = world(south);
            int orbs = 0; float parry = 0.f; bool tide = false; glm::vec3 tideAt{0.f}; float tideR = 0.f;
            for (int f = 0; f < 60 * 40; ++f) {
                t.update(DT, tw);
                if (t.ev.shots > 0) { orbs = t.ev.shots; parry = t.ev.shotParry; }
                if (t.ev.lvTide) { tide = true; tideAt = t.ev.lvTideAt; tideR = t.ev.lvTideR; }
                if (t.levPhase != 1) break;
            }
            CHECK(orbs >= 9 && parry == 180.f, "its torrent is a fan of orbs, each parried back worth 180");
            CHECK(tide && std::fabs(tideR - 46.f) < 1e-3f && glm::length(glm::vec2{tideAt.x - C.x, tideAt.z - C.z}) < 0.1f && std::fabs(tideAt.y - C.y) < 1e-3f,
                  "its tide rolls out over the ring from the pool, to the wall");
        }
        // SPIT: never at a close, visible, grounded player; within ~5 s for one far off or up on a ledge
        {
            auto spitAfter = [&](glm::vec3 feet) {
                Enemy e = fresh(); EnemyWorld w = world(feet);
                for (int f = 0; f < 60 * 3; ++f) e.update(DT, w);   // risen
                for (int f = 0; f < 60 * 20; ++f) { e.update(DT, w); if (e.ev.lvSpit) return f * DT; e.levBeached = 0.f; }
                return -1.f;
            };
            float close = spitAfter(C + glm::vec3{0.f, 0.f, 24.f});
            float far = spitAfter(C + glm::vec3{0.f, 0.f, 40.f});
            float a = mawRibAngle(1); glm::vec3 ledge = C + glm::vec3{std::cos(a), 0.f, std::sin(a)} * (MAW_RIB_R - 3.8f) + glm::vec3{0.f, MAW_LEDGE_UP, 0.f};
            float perch = spitAfter(ledge);
            std::printf("      spit: close %.1f s, far %.1f s, perched %.1f s\n", close, far, perch);
            CHECK(close < 0.f && far > 3.9f && far < 7.f && perch > 3.9f && perch < 7.f,
                  "it spits only at someone keeping away or perched (after 4 s), never at a close, grounded player");
        }
        // Tell floors on every difficulty, in every phase
        {
            float least = 9.f, leastFollow = 9.f;
            for (int dI = 0; dI < DIFFICULTY_LEVELS; ++dI)
                for (int ph = 1; ph <= 3; ++ph) {
                    Enemy e = fresh(); EnemyWorld w = world(south, dI);
                    for (int f = 0; f < 60 * 4; ++f) e.update(DT, w);
                    e.health = e.maxHealth * (ph == 1 ? 0.9f : ph == 2 ? 0.5f : 0.2f);
                    for (int f = 0; f < 60 * 40; ++f) {
                        e.update(DT, w);
                        if (e.health > 0.f) e.health = e.maxHealth * (ph == 1 ? 0.9f : ph == 2 ? 0.5f : 0.2f);
                        if (e.ev.telegraphStarted) (e.tellFollow ? leastFollow : least) = std::min(e.tellFollow ? leastFollow : least, e.telegraphDuration);
                    }
                }
            std::printf("      leviathan: shortest tell %.2f s (follow-ups %.2f s)\n", least, leastFollow);
            CHECK(least >= Enemy::TELL_FLOOR - 1e-4f && leastFollow >= Enemy::TELL_FLOOR_FOLLOW - 1e-4f, "every Leviathan tell is at least 0.35 s on every difficulty");
        }
    }

    // ---------------------------------------------------------------- the Leviathan: the hunt and the eclipse
    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& M = N.arenas[4];
        const glm::vec3 C = MAW_C;
        auto world = [&](glm::vec3 feet) {
            EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
            w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = M.bounds;
            w.hasLair = true; w.lair = N.lair; w.lairFloor = N.lairFloor; w.wells = N.wells.data(); w.wellCount = (int)N.wells.size();
            return w;
        };
        Enemy e(EnemyType::LEVIATHAN, M.bossSpawn); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
        glm::vec3 nearWell = N.wells[1] + glm::vec3{2.f, 0.f, 2.f};
        EnemyWorld w = world(nearWell);
        for (int f = 0; f < 60 * 4; ++f) e.update(DT, w);
        e.health = e.maxHealth * 0.6f;
        // Into the hunt: it goes under, hidden, and comes up out of the well nearest you after a 1.2 s boil
        bool submerged = false, hiddenUntouchable = true, sawHidden = false; float boil = -1.f; glm::vec3 site{0.f}; bool breached = false;
        for (int f = 0; f < 60 * 10 && !breached; ++f) {
            e.update(DT, w);
            if (e.ev.telegraphStarted && e.attack == AttackKind::SUBMERGE) submerged = true;
            if (e.levHidden()) { sawHidden = true; hiddenUntouchable &= !e.targetable(); }
            if (e.ev.lvBreachTell) { boil = e.telegraphDuration; site = e.ev.lvSite; }
            if (e.ev.lvBreach) breached = true;
        }
        CHECK(submerged && sawHidden && hiddenUntouchable, "in the hunt it goes under: hidden, untouchable");
        CHECK(breached && std::fabs(boil - 1.2f) < 1e-4f && glm::length(glm::vec2{site.x - N.wells[1].x, site.z - N.wells[1].z}) < 0.1f &&
              glm::length(glm::vec2{e.levRoot.x - N.wells[1].x, e.levRoot.z - N.wells[1].z}) < 0.1f && e.targetable(),
              "it breaches out of the well nearest you after a 1.2 s boil, and fights from there");
        // Two attacks from a root, then under again - and never back up the same well
        int attacks = 0; bool dove = false; glm::vec3 next{0.f};
        for (int f = 0; f < 60 * 30 && !dove; ++f) {
            e.update(DT, w);
            if (e.ev.lvCrash || (e.ev.shots > 0) || e.ev.lvTide) ++attacks;
            if (e.ev.lvBreachTell) { dove = true; next = e.ev.lvSite; }
            e.levBeached = std::min(e.levBeached, 0.5f);
        }
        CHECK(dove && attacks >= 1 && attacks <= 2 && glm::length(glm::vec2{next.x - N.wells[1].x, next.z - N.wells[1].z}) > 1.f,
              "after two attacks from a root it dives again, and comes up somewhere else");
        // The eclipse: it goes home to the pool, and every third attack is a swallow
        for (int f = 0; f < 60 * 3; ++f) e.update(DT, w);
        e.health = e.maxHealth * 0.25f;
        EnemyWorld w3 = world(C + glm::vec3{0.f, 0.f, 26.f});
        bool home = false; std::vector<AttackKind> seq;
        for (int f = 0; f < 60 * 60 && seq.size() < 6; ++f) {
            e.update(DT, w3);
            if (e.levPhase == 3 && e.levAtPool() && e.levStage == Enemy::LevStage::FIGHT && e.attack != AttackKind::BREACH) home = true;
            if (home && e.ev.telegraphStarted && e.attack != AttackKind::SPIT && e.attack != AttackKind::SUBMERGE && e.attack != AttackKind::BREACH) seq.push_back(e.attack);
            e.levBeached = std::min(e.levBeached, 0.3f);
            if (e.levInhale > 0.f) e.levInhale = std::min(e.levInhale, 0.1f);
        }
        int swallows = 0; for (auto k : seq) swallows += k == AttackKind::SWALLOW;
        CHECK(home && seq.size() == 6 && seq[2] == AttackKind::SWALLOW && seq[5] == AttackKind::SWALLOW && swallows == 2,
              "in the eclipse it fights from its pool, a swallow every third attack");
        // A swallow: inhaling 3 s; 450 down its throat in one breath and it chokes, staggered 4 s
        Enemy s(EnemyType::LEVIATHAN, M.bossSpawn); s.levInit = true; s.levRoot = s.levHome = N.lair; s.levFloor = C.y;
        s.levStage = Enemy::LevStage::FIGHT; s.state = EnemyState::ACTIVE; s.spawnTimer = 0.f; s.levPhase = 3;
        s.levInhale = Enemy::LV_INHALE;
        bool notYet = !s.levThroatHit(300.f);
        bool choked = s.levThroatHit(160.f);
        CHECK(notYet && choked && s.staggered() && std::fabs(s.staggerTimer - 4.f) < 1e-4f && s.levInhale <= 0.f,
              "450 down its throat in one inhale and it chokes: the inhale ends, staggered 4 s");
        CHECK(!s.levThroatHit(500.f), "no throat to hit when it isn't inhaling");
    }

    // ---------------------------------------------------------------- the Leviathan's hazards
    {
        using LH = LeviathanHazards;
        glm::vec3 from{0, -300, 0}, to{0, -300, 30};
        CHECK(LH::inStrip(from, to, {0.f, -300.f, 20.f}, -300.f) && LH::inStrip(from, to, {2.f, -300.f, 33.f}, -300.f) &&
              !LH::inStrip(from, to, {3.f, -300.f, 20.f}, -300.f) && !LH::inStrip(from, to, {0.f, -297.f, 20.f}, -300.f) &&
              !LH::inStrip(from, to, {0.f, -300.f, 36.f}, -300.f), "a crash's strip: 5 m wide, out past where the head lands, on the ground; 3 m aside you're clear");
        LH h;
        h.markCrash(from, to); h.markCrash(from, to * 0.5f); h.landCrash();
        bool newest = h.strips[1].landed && !h.strips[0].landed;
        h.dropMarks();
        CHECK(newest && h.strips.size() == 1 && h.strips[0].landed, "a crash lands along its newest mark; a mark it never strikes is dropped");
        // A tide: hits a grounded player as it passes, misses one in the air
        auto tideHits = [&](float up) {
            LH t; t.addTide({0, -300, 0}, 13.f, 46.f);
            int hits = 0;
            for (int i = 0; i < 60 * 4; ++i) for (auto& x : t.update(DT, {20.f, -300.f + up, 0.f}, -300.f)) hits += x.damage == LH::TIDE_DAMAGE;
            return hits;
        };
        CHECK(tideHits(0.f) == 1 && tideHits(1.5f) == 0, "a tide hits you on the ground as it passes (once), and passes under a jump");
        // A spit: bursts after a second where it was marked, then burns
        {
            LH t; t.addSpit({0, -300, 0});
            float burst = 0.f, burn = 0.f; float burstAt = -1.f;
            for (int i = 0; i < 60 * 6; ++i)
                for (auto& x : t.update(DT, {1.f, -300.f, 0.f}, -300.f)) {
                    if (x.damage == LH::SPIT_DAMAGE) { burst += x.damage; burstAt = i * DT; } else burn += x.damage;
                }
            LH f; f.addSpit({0, -300, 0}); float away = 0.f;
            for (int i = 0; i < 60 * 6; ++i) for (auto& x : f.update(DT, {6.f, -300.f, 0.f}, -300.f)) away += x.damage;
            CHECK(burst == 30.f && burstAt > 0.95f && burstAt < 1.05f && burn > 50.f && burn < 65.f && away == 0.f,
                  "a spit bursts a second after its marker (30), then burns there for 4 s at 15/s; 6 m away nothing");
        }
        CHECK(LH::breachHits({0, -300, 0}, {5.f, -300.f, 0.f}) && !LH::breachHits({0, -300, 0}, {8.f, -300.f, 0.f}) &&
              !LH::breachHits({0, -300, 0}, {2.f, -293.f, 0.f}), "a breach erupts within 6 m of its well, not at 8 m, not up on a ledge");
        glm::vec3 pl = LH::pull({0, -300, 0}, {10.f, -300.f, 0.f});
        CHECK(std::fabs(glm::length(pl) - 3.5f) < 1e-4f && pl.x < 0.f && pl.y == 0.f &&
              LH::PULL_SPEED < 7.f * Player::wadeFactor(0.6f), "the swallow drags you toward its mouth at 3.5 m/s, slower than you wade in its flood");
        CHECK(LH::bites({0, -300, 0}, {4.f, -300.f, 0.f}) && !LH::bites({0, -300, 0}, {5.f, -300.f, 0.f}), "it bites inside 4.5 m");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
