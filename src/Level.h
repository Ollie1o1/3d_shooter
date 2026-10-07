#pragma once
// =============================================================================
// Level.h — the ARENA mode map: four arenas, the corridors between them, and
// their moods. (FAST mode's map, the Gauntlet, is in LevelGauntlet.h and uses
// the same data structures.)
//
//          +Z (south)
//   ┌─────────────────┐  ARENA 1  SUNSET YARD   X ±30  Z  -30..30   open sky
//   │   player start  │           adobe walls, side platforms, a sun obelisk
//   └──────┐ ┌────────┘
//          │ │          corridor  Z -46..-31
//   ┌──────┘ └────────┐  ARENA 2  THE FOUNDRY   X ±32  Z -110..-46  roofed
//   │  furnace, lava  │           catwalks, lava channels, furnace you can climb
//   └──────┐ ┌────────┘
//          │ │          corridor  Z -126..-111
//   ┌──────┘ └────────┐  ARENA 3  THE SPIRE     X ±30  Z -186..-126 dawn sky
//   │ tower, 5 tiers  │           a 26 m tower; ledges, bridges, a balcony and
//   └──────┐ ┌────────┘           the summit. Lifts, sweepers and orbiting
//          │ │                    platforms (grapple them). Each wave spawns a
//          │ │                    tier higher: climb to reach the shooters.
//          │ │          corridor  Z -202..-187
//   ┌──────┘ └────────┐  ARENA 4  THE CORE      X ±36  Z -274..-202 night sky
//   │ reactor + boss  │           pillar ring, corner perches, the Warden
//   └──────┐ ┌────────┘
//          │ │          corridor  Z -291..-275
//   ┌──────┘ └──────────────┐  ARENA 5  THE SANCTUM  X ±56  Z -404..-292  eclipse
//   │                       │     a duelling ground under an eclipse: open
//   │   the Sovereign       │     floor, a ring of tall pillars to grapple,
//   │                       │     raised corners, floating islands, orbiting
//   └───────────────────────┘     platforms. One enemy: the final boss.
//          -Z (north)
//
// Act II (beneath the Sanctum, 60 m down) is built by LevelAct2.h.
//
// Each arena: three waves (the Sanctum: just the boss). Clearing the last opens the exit door; walking far
// enough into the next arena slams the gate shut behind you and starts it.
// Every arena has a ceiling (zone.max.y): an invisible barrier that stops
// dashes and grapples from launching you out over the walls.
//
// Everything here is plain data (no OpenGL), so tests can check that spawn
// points aren't inside walls, pads land on their platforms, and so on.
// =============================================================================
#include "Player.h"
#include "AudioTypes.h"
#include "Enemy.h"
#include "Shapes.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

// Lighting, fog and sky for one arena. Blended across corridors.
struct Theme {
    glm::vec3 zenith, horizon, ground;        // sky gradient
    glm::vec3 sunDir, sunColor;
    float     sunSize, sunStripes;            // radians; 1 = synthwave bands
    glm::vec3 mountain;
    float     stars;
    glm::vec3 lightDir, lightColor;           // key light (direction light travels)
    glm::vec3 skyAmb, groundAmb;              // hemisphere ambient
    glm::vec3 fogColor;
    float     fogDensity;
};

inline Theme lerpTheme(const Theme& a, const Theme& b, float t) {
    auto m = [t](glm::vec3 x, glm::vec3 y) { return glm::mix(x, y, t); };
    auto f = [t](float x, float y) { return x + (y - x) * t; };
    Theme o;
    o.zenith = m(a.zenith, b.zenith);   o.horizon = m(a.horizon, b.horizon); o.ground = m(a.ground, b.ground);
    o.sunDir = glm::normalize(m(a.sunDir, b.sunDir));  o.sunColor = m(a.sunColor, b.sunColor);
    o.sunSize = f(a.sunSize, b.sunSize); o.sunStripes = f(a.sunStripes, b.sunStripes);
    o.mountain = m(a.mountain, b.mountain); o.stars = f(a.stars, b.stars);
    o.lightDir = glm::normalize(m(a.lightDir, b.lightDir)); o.lightColor = m(a.lightColor, b.lightColor);
    o.skyAmb = m(a.skyAmb, b.skyAmb); o.groundAmb = m(a.groundAmb, b.groundAmb);
    o.fogColor = m(a.fogColor, b.fogColor); o.fogDensity = f(a.fogDensity, b.fogDensity);
    return o;
}

// `count` enemies of `type`. If `at` is filled, they spawn exactly there (one
// per point, count ignored) — FAST mode's hand-placed encounters.
// A squad: each one arrives with its `escort` in formation behind it, e.g.
// WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}).
struct WaveEntry {
    EnemyType type;
    int count = 0;
    std::vector<glm::vec3> at;
    std::vector<EnemyType> escort;
    Hollow variant = Hollow::NONE;   // a Hollowed variant (Act II): the leader only
    WaveEntry(EnemyType t, int n, std::vector<glm::vec3> pts = {}) : type(t), count(n), at(std::move(pts)) {}
    WaveEntry with(std::vector<EnemyType> e) const { WaveEntry w = *this; w.escort = std::move(e); return w; }
    WaveEntry hollow(Hollow h) const { WaveEntry w = *this; w.variant = canBeHollow(type) ? h : Hollow::NONE; return w; }
    int total() const { return (at.empty() ? count : (int)at.size()) * (1 + (int)escort.size()); }
};

// What ends a wave. KILL_ALL is the default. The others keep the wave coming
// (it refills as you kill) until they're met, then whatever's left collapses:
//   HOLD      stand in the circle (`pos`, `radius`) for `seconds`; it only
//             counts while no enemy on foot is inside it
//   CONDUITS  destroy the CONDUIT pylons at `points`; the wave spawns out of them
//   SURVIVE   last `seconds`
struct WaveGoal {
    enum Kind { KILL_ALL, HOLD, CONDUITS, SURVIVE } kind = KILL_ALL;
    const char* label = "";        // the objective line, e.g. "HOLD THE DAIS"
    glm::vec3 pos{0.f};
    float radius = 0.f, seconds = 0.f;
    std::vector<glm::vec3> points;
    int mover = -1;   // HOLD: the circle rides this mover (on its top) instead of staying at pos
    WaveGoal onMover(int m) const { WaveGoal g = *this; g.mover = m; return g; }
    static WaveGoal hold(const char* l, glm::vec3 p, float r, float s) { WaveGoal g; g.kind = HOLD; g.label = l; g.pos = p; g.radius = r; g.seconds = s; return g; }
    static WaveGoal conduits(const char* l, std::vector<glm::vec3> pts) { WaveGoal g; g.kind = CONDUITS; g.label = l; g.points = std::move(pts); return g; }
    static WaveGoal survive(const char* l, float s) { WaveGoal g; g.kind = SURVIVE; g.label = l; g.seconds = s; return g; }
};

// A door is a pair of walls that part in the middle, each half sliding into
// its jamb. Every door opens by itself when the player comes near (Ultrakill
// style) unless it's locked: an arena's exit until the fight there is won,
// the way in once a fight has started. LevelData::updateDoors() runs them.
struct Door {
    int   wall = -1, wall2 = -1;     // the two halves (indices into LevelData::walls)
    AABB  closed{};                  // the whole doorway when shut
    float height = 4.f;
    float baseY  = 0.f;              // floor the door stands on
    bool  locked = false;
    bool  open   = false;            // where it's heading (set by updateDoors)
    float openAmount = 0.f;          // 0 shut .. 1 fully parted
    float sense = 8.f;               // opens when the player is this close (m)
    bool  alongX() const { return closed.max.x - closed.min.x >= closed.max.z - closed.min.z; }
};

struct JumpPad {
    glm::vec3 centre;        // on the floor
    glm::vec2 half;          // XZ half-extents of the trigger
    glm::vec3 launch;        // velocity applied to the player
};

// A boost tube: inside the box the player is driven along `dir` at `speed`
// (at least). Horizontal ones fire you down a duct; vertical ones are lifts.
struct Booster { AABB box; glm::vec3 dir; float speed; };

struct Hazard { AABB box; float dps; };

struct FloorPatch { float x0, z0, x1, z1, y; glm::vec3 color; };

// Ground that isn't at Y 0: inside `xz` (its X and Z only) the hard floor is
// at `y`. Act II sits in one, 60 m under Act I.
struct Basin { AABB xz; float y; };

// Standing water: over `box` (X and Z; box.min.y is the floor it fills from)
// the surface is at `level`. ArenaShifts raises it (FLOOD).
struct WaterVolume { AABB box; float level; };

// A platform that moves along a path. Its collision box is an ordinary wall
// (flagged dynamic), so the player stands on it, bullets stop on it and the
// grapple hooks it; LevelData::updateMovers() moves that wall every tick and
// records how far it went so GameplayState can carry the player along.
struct Mover {
    enum class Path { PINGPONG, ORBIT, DRIVEN };   // DRIVEN: placed by the game (drive), not by the clock
    int       wall = -1;
    AABB      base;              // box at the path's origin
    Path      path = Path::PINGPONG;
    glm::vec3 a{0.f}, b{0.f};    // PINGPONG: offsets of the two ends. ORBIT: the two radius vectors
    float     period = 6.f;      // seconds for a full cycle
    float     phase  = 0.f;      // 0..1
    glm::vec3 color{0.35f, 0.38f, 0.45f};
    glm::vec3 glow {0.3f, 1.0f, 1.0f};
    glm::vec3 delta{0.f};        // how far it moved on the last update
    float     drive  = 0.f;      // DRIVEN: 0..1 along a -> b

    glm::vec3 offsetAt(float t) const {
        if (path == Path::DRIVEN) return glm::mix(a, b, drive);
        float u = t / period + phase;
        if (path == Path::ORBIT) {
            float ang = u * 6.2831853f;
            return a * std::cos(ang) + b * std::sin(ang);
        }
        // Eased back and forth, lingering briefly at each end so you can board
        float s = 0.5f - 0.5f * std::cos(u * 6.2831853f);
        s = glm::clamp((s - 0.08f) / 0.84f, 0.f, 1.f);
        s = s * s * (3.f - 2.f * s);
        return glm::mix(a, b, s);
    }
};

enum class Ambient { DUST, EMBERS, MOTES, WIND, ASH, STEAM };
// How an arena changes as its fight goes on (ArenaShifts.h)
enum class ArenaShift { NONE, NIGHTFALL, LAVA_RISE, SPEED_UP, OVERLOAD, FLOOD, SOLAR, DESCENT };

struct Arena {
    const char* name;
    const char* subtitle;
    ReverbSpace space = ReverbSpace::METAL;   // its reverb (AudioSystem::setSpace)
    AABB        bounds;          // interior; enemies are clamped inside, max.y caps flyers
    AABB        zone;            // where the player may be (XZ); max.y is the ceiling
    // More boxes that count as this arena/section: the tubes and side rooms
    // leading into and off it. Same rules as `zone`.
    std::vector<AABB> extraZones;
    glm::vec3   playerStart;
    float       startYaw = -90.f;   // which way the checkpoint faces (-90 = north)
    // Where a fall into the void puts you back (FAST rooms mid-fight, when the
    // way back to playerStart is locked). Unset: playerStart.
    glm::vec3   respawn{0.f};
    bool        hasRespawn = false;
    std::vector<glm::vec3> groundSpawns, airSpawns;
    // Optional per-wave ground spawns (the Spire: each wave a tier higher).
    // Empty for a wave → groundSpawns.
    std::vector<std::vector<glm::vec3>> waveGround;
    // Optional per-wave air spawns (the Descent: each wave a stop lower).
    // Empty for a wave → airSpawns.
    std::vector<std::vector<glm::vec3>> waveAir;
    glm::vec3   bossSpawn{0.f};
    std::vector<std::vector<WaveEntry>> waves;
    std::vector<WaveGoal> goals;    // per wave; missing: KILL_ALL
    int         maxAlive = 8;    // concurrent enemies; the rest trickle in as you kill
    float       damageScale = 1.f;  // enemy damage multiplier: the first arena is forgiving
    int         entryGate = -1;  // door behind you once you're in (index into doors)
    int         exitDoor  = -1;
    float       voidY = -1e9f;   // fall below this inside the zone: back to the checkpoint
    // FAST: the fight starts when the player walks into this box. The stretch
    // before it (from playerStart) is a breather with health to pick up.
    AABB        trigger{{0, 0, 0}, {0, 0, 0}};
    bool        hasTrigger = false;
    Ambient     ambient = Ambient::DUST;
    ArenaShift  shift = ArenaShift::NONE;
    std::vector<float> floodLevels;   // FLOOD: the water's surface (Y) for each wave
    glm::vec3   sunPos{0.f};          // SOLAR: the sun the flare turns about
    Theme       theme;

    bool containsXZ(glm::vec3 p) const {
        auto in = [&](const AABB& z) { return p.x >= z.min.x && p.x <= z.max.x && p.z >= z.min.z && p.z <= z.max.z; };
        if (in(zone)) return true;
        for (auto& z : extraZones) if (in(z)) return true;
        return false;
    }
};

struct LevelData {
    std::vector<Wall>       walls;     // collidable (doors and movers included)
    std::vector<Wall>       props;     // visual only
    std::vector<Wall>       neon;      // visual only, self-lit (drawn with vertex glow)
    std::vector<Shape>      shapes;    // visual only: round, curved, turned (Shapes.h)
    std::vector<FloorPatch> floors;
    std::vector<Door>       doors;
    std::vector<JumpPad>    pads;
    std::vector<Booster>    boosters;
    std::vector<Hazard>     hazards;
    std::vector<Basin>      basins;
    std::vector<WaterVolume> water;
    std::vector<Arena>      arenas;
    std::vector<AABB>       corridors; // corridor i joins arena i and i+1 (zone, XZ)
    // Lighting crossfades between two arenas across a box, along one axis
    // (FAST's course turns, so not always along -Z like the corridors)
    struct Blend { AABB box; int from, to; int axis; bool decreasing; };
    std::vector<Blend>      blends;
    // Health and XP placed in the level (FAST's breathers): kind 0 orb, 1 potion, 2 XP
    struct Placed { glm::vec3 pos; int kind; };
    std::vector<Placed>     placedPickups;
    std::vector<Mover>      movers;
    std::vector<int>        moverWalls;   // wall index of every mover (for Player::dynWalls)
    // The Descent's cage: movers driven together between stops (Y of the
    // cage's top at each). A ride is requested between waves, starts when the
    // player is aboard (GameplayState calls start()), and eases over RIDE_TIME.
    struct Lift {
        std::vector<int>   movers;
        std::vector<float> stops;
        int   at = 0, to = 0, pending = -1;
        float t = 0.f;
        static constexpr float RIDE_TIME = 8.f;
        bool  riding() const { return to != at; }
        bool  busy() const { return riding() || pending >= 0; }
        float y() const {
            if (stops.empty()) return 0.f;
            if (!riding()) return stops[at];
            float u = glm::clamp(t / RIDE_TIME, 0.f, 1.f);
            u = u * u * (3.f - 2.f * u);
            return glm::mix(stops[at], stops[to], u);
        }
        void request(int stop) { if (!stops.empty()) pending = std::clamp(stop, 0, (int)stops.size() - 1); }
        void start() { if (pending >= 0 && !riding()) { to = pending; pending = -1; t = 0.f; if (to == at) to = at; } }
        void reset() { at = to = 0; pending = -1; t = 0.f; }
        void update(float dt, LevelData& L) {
            if (movers.empty() || stops.empty()) return;
            if (riding()) {
                t += dt;
                if (t >= RIDE_TIME) { at = to; t = 0.f; }
            }
            float span = stops.back() - stops.front();
            float drive = std::fabs(span) > 1e-4f ? (y() - stops.front()) / span : 0.f;
            for (int m : movers) L.movers[m].drive = drive;
        }
    };
    Lift lift;

    // The Penitent's chains are fixed to these, high on the pit wall: shoot
    // one out (hp) or grapple onto it and hang on (GameplayState rips it)
    // The floor an enemy at pos stands on (or hovers over)
    // (groundUnder: the highest top under it, moving platforms included). In
    // the Descent the void runs down to -300, so walkers stand on whatever is
    // really under them (a gallery, the cage, the pit) and fliers keep to the
    // cage's stop instead of sinking to the bottom of the shaft.
    float enemyFloor(int arena, glm::vec3 pos, bool flying, float groundUnder) const {
        float base = floorWithWater(pos.x, pos.z, false);
        if (arena < 0 || arena >= (int)arenas.size() || arenas[arena].shift != ArenaShift::DESCENT) return base;
        return flying ? std::max(base, lift.y()) : std::max(base, groundUnder);
    }
    bool onLift(int groundWall) const {
        for (int m : lift.movers) if (movers[m].wall == groundWall) return true;
        return false;
    }
    // Wall targets a boss is tied to: the PENITENT's chain anchors (shoot or
    // rip them), the WARDEN's conduit nodes (shoot them; they re-attach)
    struct ChainAnchor {
        int wall = -1; glm::vec3 pos{0.f}; float hp = 400.f; bool alive = true;
        enum Kind { CHAIN, CONDUIT } kind = CHAIN;
        AABB home{};          // its wall as built (restoreAnchor)
        float maxHp = 400.f;
    };
    std::vector<ChainAnchor> anchors;
    int addAnchor(int wall, glm::vec3 pos, float hp, ChainAnchor::Kind kind) {
        ChainAnchor a; a.wall = wall; a.pos = pos; a.hp = a.maxHp = hp; a.kind = kind; a.home = walls[wall].box;
        anchors.push_back(a);
        return (int)anchors.size() - 1;
    }
    int anchorsAlive(ChainAnchor::Kind kind = ChainAnchor::CHAIN) const {
        int n = 0; for (auto& a : anchors) n += a.alive && a.kind == kind; return n;
    }
    // A conduit re-attached: its node back where it was, whole
    void restoreAnchor(int i) {
        if (i < 0 || i >= (int)anchors.size()) return;
        anchors[i].alive = true; anchors[i].hp = anchors[i].maxHp;
        walls[anchors[i].wall].box = anchors[i].home;
    }
    // True only on the hit that breaks it. A broken anchor's wall is parked
    // far below, inside the grid cells it was filed under (like an open door)
    bool damageAnchor(int i, float dmg) {
        if (i < 0 || i >= (int)anchors.size() || !anchors[i].alive) return false;
        anchors[i].hp -= dmg;
        if (anchors[i].hp > 0.f) return false;
        anchors[i].alive = false;
        AABB& b = walls[anchors[i].wall].box;
        b.min.y -= 500.f; b.max.y = b.min.y + 0.01f;
        return true;
    }
    int anchorAlong(glm::vec3 o, glm::vec3 d, float maxT) const {
        int best = -1; float bt = maxT + 0.05f;
        for (int i = 0; i < (int)anchors.size(); ++i) {
            if (!anchors[i].alive) continue;
            const AABB& b = walls[anchors[i].wall].box;
            glm::vec3 inv{1.f / (d.x + 1e-9f), 1.f / (d.y + 1e-9f), 1.f / (d.z + 1e-9f)};
            glm::vec3 t0 = (b.min - o) * inv, t1 = (b.max - o) * inv;
            glm::vec3 tn = glm::min(t0, t1), tx = glm::max(t0, t1);
            float te = std::max({tn.x, tn.y, tn.z}), tl = std::min({tx.x, tx.y, tx.z});
            if (tl >= te && te > 0.f && te < bt) { bt = te; best = i; }
        }
        return best;
    }
    // The highest top (static wall or moving platform) under (x, z) at or
    // below fromY + 0.5, else the hard floor
    float groundAt(float x, float z, float fromY) const {
        float best = baseFloor(x, z);
        for (const auto& w : walls) {
            const AABB& b = w.box;
            if (x >= b.min.x && x <= b.max.x && z >= b.min.z && z <= b.max.z && b.max.y <= fromY + 0.5f)
                best = std::max(best, b.max.y);
        }
        return best;
    }

    // Animated set dressing, drawn by GameplayState
    struct Gem { glm::vec3 pos; glm::vec3 color; float size; bool beam; };
    std::vector<Gem> gems;
    // Spinning fans set into walls and floors (drawn by GameplayState)
    struct Fan { glm::vec3 pos; float radius; int axis; glm::vec3 glow; };
    std::vector<Fan> fans;
    // Pistons hanging from a ceiling: a sleeve at `top`, a rod that pumps
    // down between minLen and maxLen once every `period`, a heavy head on it
    struct Piston {
        glm::vec3 top; float radius, minLen, maxLen, period, phase; glm::vec3 glow;
        float length(float t) const {
            float u = 0.5f - 0.5f * std::cos((t / period + phase) * 6.2831853f);
            u = u * u * (3.f - 2.f * u);   // dwell at each end, slam between
            return minLen + (maxLen - minLen) * u;
        }
    };
    std::vector<Piston> pistons;
    // Rings of light turning about a vertical axis (round a turbine, a core)
    struct Spinner { glm::vec3 pos; float radius, speed; int segs; glm::vec3 glow; };
    std::vector<Spinner> spinners;
    bool      hasReactor = false;
    glm::vec3 reactorPos{0.f};

    // FAST mode
    bool      fast = false;
    glm::vec3 finishPos{0.f};      // touch this once the last section is clear
    float     parTimes[4] = {0, 0, 0, 0};   // S / A / B / C thresholds in seconds
    float     moverClock = 0.f;

    // The hard floor at (x, z): a basin's, else Y 0
    float baseFloor(float x, float z) const {
        for (auto& b : basins)
            if (x >= b.xz.min.x && x <= b.xz.max.x && z >= b.xz.min.z && z <= b.xz.max.z) return b.y;
        return 0.f;
    }
    static constexpr float WADE_MAX   = Player::WADE_MAX;     // feet never deeper than this under the surface
    static constexpr float SKIM_DEPTH = Player::SKIM_DEPTH;   // a slide planes this far under it
    // The water's surface over (x, z), or -1e9 where it's dry
    float waterSurfaceAt(float x, float z) const {
        float s = -1e9f;
        for (auto& w : water)
            if (x >= w.box.min.x && x <= w.box.max.x && z >= w.box.min.z && z <= w.box.max.z) s = std::max(s, w.level);
        return s;
    }
    float waterDepthAt(glm::vec3 p) const { return std::max(0.f, waterSurfaceAt(p.x, p.z) - p.y); }
    // The hard floor with deep water's lift on top of it
    float floorWithWater(float x, float z, bool skimming) const {
        return std::max(baseFloor(x, z), waterSurfaceAt(x, z) - (skimming ? SKIM_DEPTH : WADE_MAX));
    }
    float lowestFloor() const {
        float y = 0.f;
        for (auto& b : basins) y = std::min(y, b.y);
        return y;
    }

    bool isDoorWall(int w) const {
        for (auto& d : doors) if (d.wall == w || d.wall2 == w) return true;
        return false;
    }
    int moverOfWall(int w) const {
        for (int i = 0; i < (int)movers.size(); ++i) if (movers[i].wall == w) return i;
        return -1;
    }

    // Arena whose zone (or one of its extra zones) contains p, or -1 if p is
    // in an ARENA corridor / outside.
    int arenaAt(glm::vec3 p) const {
        for (int i = 0; i < (int)arenas.size(); ++i) if (arenas[i].containsXZ(p)) return i;
        return -1;
    }

    // Lighting for a position: an arena's own theme, or a blend while walking
    // down the corridor between two arenas.
    Theme themeAt(glm::vec3 p) const {
        for (auto& b : blends) {
            if (p.x < b.box.min.x || p.x > b.box.max.x || p.z < b.box.min.z || p.z > b.box.max.z) continue;
            if (p.y < b.box.min.y || p.y > b.box.max.y) continue;
            float lo = b.box.min[b.axis], hi = b.box.max[b.axis];
            float t = b.decreasing ? (hi - p[b.axis]) / (hi - lo) : (p[b.axis] - lo) / (hi - lo);
            t = glm::clamp(t, 0.f, 1.f);
            t = t * t * (3.f - 2.f * t);
            return lerpTheme(arenas[b.from].theme, arenas[b.to].theme, t);
        }
        for (int i = 0; i < (int)corridors.size() && i + 1 < (int)arenas.size(); ++i) {
            const AABB& c = corridors[i];
            if (p.z <= c.max.z && p.z >= c.min.z && p.x >= c.min.x - 2.f && p.x <= c.max.x + 2.f) {
                float t = (c.max.z - p.z) / (c.max.z - c.min.z);
                t = t * t * (3.f - 2.f * t);
                return lerpTheme(arenas[i].theme, arenas[i + 1].theme, t);
            }
        }
        int a = arenaAt(p);
        if (a >= 0) return arenas[a].theme;
        // Outside every zone (shouldn't happen): use the nearest arena by Z
        int best = 0; float bestD = 1e9f;
        for (int i = 0; i < (int)arenas.size(); ++i) {
            float cz = (arenas[i].zone.min.z + arenas[i].zone.max.z) * 0.5f;
            if (std::fabs(cz - p.z) < bestD) { bestD = std::fabs(cz - p.z); best = i; }
        }
        return arenas[best].theme;
    }

    // Move every platform to where it is at time t.
    void updateMovers(float t) {
        moverClock = t;
        for (auto& m : movers) {
            glm::vec3 off = m.offsetAt(t);
            AABB& b = walls[m.wall].box;
            glm::vec3 before = b.min;
            b.min = m.base.min + off;
            b.max = m.base.max + off;
            m.delta = b.min - before;
        }
    }

    // ---- Doors ----
    // Size the two halves for how far the door has parted. Fully open, both
    // are parked under the floor (inside the cells they were filed under), so
    // nothing snags on a sliver of door in the jamb.
    void applyDoor(int di) {
        Door& d = doors[di];
        AABB a = d.closed, b = d.closed;
        if (d.openAmount >= 0.999f) {
            a.min.y = b.min.y = d.baseY - 60.f;
            a.max.y = b.max.y = d.baseY - 59.f;
        } else {
            int ax = d.alongX() ? 0 : 2;
            float mid = (d.closed.min[ax] + d.closed.max[ax]) * 0.5f;
            float half = (d.closed.max[ax] - d.closed.min[ax]) * 0.5f;
            a.max[ax] = mid - d.openAmount * half;
            b.min[ax] = mid + d.openAmount * half;
        }
        walls[d.wall].box = a;
        walls[d.wall2].box = b;
    }
    void setDoorInstant(int di, bool open) {
        doors[di].open = open;
        doors[di].openAmount = open ? 1.f : 0.f;
        applyDoor(di);
    }
    // Is the player close enough to a door to open it?
    static bool nearDoor(const Door& d, glm::vec3 feet, float pad) {
        const AABB& c = d.closed;
        float dx = std::max({c.min.x - feet.x, 0.f, feet.x - c.max.x});
        float dz = std::max({c.min.z - feet.z, 0.f, feet.z - c.max.z});
        if (feet.y < d.baseY - 3.f || feet.y > d.baseY + d.height + 1.f) return false;
        return dx * dx + dz * dz < pad * pad;
    }
    // Open every unlocked door the player is near, close the rest. A door
    // never closes on the player standing in it. Calls onChange(door, opening)
    // when a door starts to move (for its sound).
    template <typename F>
    void updateDoors(float dt, glm::vec3 feet, F&& onChange) {
        for (int i = 0; i < (int)doors.size(); ++i) {
            Door& d = doors[i];
            bool want = !d.locked && nearDoor(d, feet, d.sense);
            if (!want && nearDoor(d, feet, 0.6f)) want = true;   // standing in the doorway
            if (want != d.open) { d.open = want; onChange(i, want); }
            float target = d.open ? 1.f : 0.f;
            if (d.openAmount == target) continue;
            float step = dt * (d.open ? 4.5f : 2.5f);   // parts in under a quarter second
            d.openAmount = d.open ? std::min(1.f, d.openAmount + step) : std::max(0.f, d.openAmount - step);
            applyDoor(i);
        }
    }
    void updateDoors(float dt, glm::vec3 feet) { updateDoors(dt, feet, [](int, bool) {}); }

    // Which booster (if any) is the player in?
    int boosterAt(glm::vec3 feet) const {
        glm::vec3 c = feet + glm::vec3{0, 0.9f, 0};
        for (int i = 0; i < (int)boosters.size(); ++i) {
            const AABB& b = boosters[i].box;
            if (c.x >= b.min.x && c.x <= b.max.x && c.y >= b.min.y && c.y <= b.max.y && c.z >= b.min.z && c.z <= b.max.z) return i;
        }
        return -1;
    }
};

// Before Player::update(): the floor and water where the player stands
inline void applyWater(Player& p, const LevelData& L) {
    float surf = L.waterSurfaceAt(p.position.x, p.position.z);
    p.wadeDepth = std::max(0.f, surf - p.position.y);
    p.waterSurface = surf;
    p.floorY = L.baseFloor(p.position.x, p.position.z);   // the Player adds the water's lift itself
}

// Inside a boost tube the player is driven along it at (at least) its speed
// and straightened up; a vertical one is a lift shaft that also draws you to
// its middle so you don't scrape up the wall. Call after Player::update().
inline void applyBooster(const Booster& b, Player& p, float dt) {
    glm::vec3& v = p.velocity;
    if (b.dir.y > 0.5f) {
        glm::vec3 c = (b.box.min + b.box.max) * 0.5f;
        glm::vec2 off{p.position.x - c.x, p.position.z - c.z};
        float k = std::exp(-5.f * dt);
        v.x = v.x * k - off.x * 3.f * (1.f - k);
        v.z = v.z * k - off.y * 3.f * (1.f - k);
        if (v.y < b.speed) v.y += (b.speed - v.y) * std::min(1.f, dt * 12.f);
        p.onGround = false;
        return;
    }
    glm::vec3 flat{v.x, 0.f, v.z};
    float along = glm::dot(flat, b.dir);
    glm::vec3 side = (flat - b.dir * along) * std::exp(-7.f * dt);
    along = std::max(along, b.speed);
    v.x = side.x + b.dir.x * along;
    v.z = side.z + b.dir.z * along;
}

// Shared building helpers for both maps
struct LevelBuilder {
    LevelData& L;
    Mat mat = Mat::BRICK;          // material for walls and props built from here on

    // An opening cut in a wall: [a0, a1] along the wall, open from y0 to y1
    struct Gap { float a0, a1, y0, y1; };

    static AABB aabb(float x0, float y0, float z0, float x1, float y1, float z1) {
        return AABB{ {std::min(x0,x1), std::min(y0,y1), std::min(z0,z1)},
                     {std::max(x0,x1), std::max(y0,y1), std::max(z0,z1)} };
    }
    int wall(float x0, float y0, float z0, float x1, float y1, float z1, glm::vec3 c) {
        Wall w{aabb(x0,y0,z0,x1,y1,z1), c};
        w.mat = mat;
        L.walls.push_back(w);
        return (int)L.walls.size() - 1;
    }
    void prop(float x0, float y0, float z0, float x1, float y1, float z1, glm::vec3 c) {
        Wall w{aabb(x0,y0,z0,x1,y1,z1), c};
        w.mat = mat;
        L.props.push_back(w);
    }
    void neon(float x0, float y0, float z0, float x1, float y1, float z1, glm::vec3 c) {
        L.neon.push_back(Wall{aabb(x0,y0,z0,x1,y1,z1), c});
    }
    // Non-box geometry in the current material (Shapes.h); `glow` for self-lit
    ShapeKit kit(bool glow = false) { return ShapeKit{L.shapes, (int)mat, glow}; }
    // Collision with nothing drawn: what a shape stands on, or stops you
    int solid(float x0, float y0, float z0, float x1, float y1, float z1) {
        int w = wall(x0, y0, z0, x1, y1, z1, {0.3f, 0.3f, 0.3f});
        L.walls[w].hidden = true;
        return w;
    }
    // A band of light wrapped around a box (slightly larger, so only its sides show)
    void ring(float x0, float z0, float x1, float z1, float y0, float y1, glm::vec3 c) {
        neon(x0 - 0.06f, y0, z0 - 0.06f, x1 + 0.06f, y1, z1 + 0.06f, c);
    }

    // A wall running along X (thin in Z) with doorways / windows cut in it
    void wallX(float x0, float x1, float z0, float z1, float y0, float y1, glm::vec3 c, std::vector<Gap> gaps = {}) {
        std::sort(gaps.begin(), gaps.end(), [](const Gap& a, const Gap& b) { return a.a0 < b.a0; });
        float x = x0;
        for (auto& g : gaps) {
            if (g.a0 > x) wall(x, y0, z0, g.a0, y1, z1, c);
            if (g.y0 > y0) wall(g.a0, y0, z0, g.a1, g.y0, z1, c);   // sill
            if (g.y1 < y1) wall(g.a0, g.y1, z0, g.a1, y1, z1, c);   // lintel
            x = g.a1;
        }
        if (x < x1) wall(x, y0, z0, x1, y1, z1, c);
    }
    // A wall running along Z (thin in X) with doorways / windows cut in it
    void wallZ(float z0, float z1, float x0, float x1, float y0, float y1, glm::vec3 c, std::vector<Gap> gaps = {}) {
        std::sort(gaps.begin(), gaps.end(), [](const Gap& a, const Gap& b) { return a.a0 < b.a0; });
        float z = z0;
        for (auto& g : gaps) {
            if (g.a0 > z) wall(x0, y0, z, x1, y1, g.a0, c);
            if (g.y0 > y0) wall(x0, y0, g.a0, x1, g.y0, g.a1, c);
            if (g.y1 < y1) wall(x0, g.y1, g.a0, x1, y1, g.a1, c);
            z = g.a1;
        }
        if (z < z1) wall(x0, y0, z, x1, y1, z1, c);
    }

    // A door filling a doorway [a0, a1] in a wall whose thickness spans w0..w1
    // across it (alongX: the wall runs along X, so w is Z). Glowing trim frames
    // the opening on both faces. Returns the door index.
    int doorway(bool alongX, float a0, float a1, float w0, float w1, float baseY, float h,
                glm::vec3 trim, bool locked = false) {
        float m0 = std::min(w0, w1) + 0.15f, m1 = std::max(w0, w1) - 0.15f;
        AABB box = alongX ? aabb(a0, baseY, m0, a1, baseY + h, m1) : aabb(m0, baseY, a0, m1, baseY + h, a1);
        glm::vec3 col{0.17f, 0.17f, 0.21f};
        Wall wa{box, col}, wb{box, col};
        wa.hidden = wb.hidden = true;
        L.walls.push_back(wa);
        L.walls.push_back(wb);
        Door d;
        d.wall = (int)L.walls.size() - 2; d.wall2 = (int)L.walls.size() - 1;
        d.closed = box; d.height = h; d.baseY = baseY; d.locked = locked;
        L.doors.push_back(d);
        L.applyDoor((int)L.doors.size() - 1);
        // Trim on both faces: two jambs and a header
        float lo = std::min(w0, w1), hi = std::max(w0, w1);
        for (float f : {lo - 0.07f, hi}) {
            float g0 = f, g1 = f + 0.07f;
            if (alongX) {
                neon(a0 - 0.3f, baseY, g0, a0, baseY + h + 0.3f, g1, trim);
                neon(a1, baseY, g0, a1 + 0.3f, baseY + h + 0.3f, g1, trim);
                neon(a0 - 0.3f, baseY + h, g0, a1 + 0.3f, baseY + h + 0.3f, g1, trim);
            } else {
                neon(g0, baseY, a0 - 0.3f, g1, baseY + h + 0.3f, a0, trim);
                neon(g0, baseY, a1, g1, baseY + h + 0.3f, a1 + 0.3f, trim);
                neon(g0, baseY + h, a0 - 0.3f, g1, baseY + h + 0.3f, a1 + 0.3f, trim);
            }
        }
        return (int)L.doors.size() - 1;
    }

    // A square duct along X (axis 0) or Z (axis 2), from a to b along the
    // axis, centred on c across it, floor at y, inner width w and height h,
    // with a 1 m shell. Glowing ribs every 3 m and a light strip down the
    // middle of the floor. `solidBelow`: the floor is a block down to the
    // ground (else a 1 m slab over thin air). Returns the duct's zone, which
    // reaches 1.5 m past both ends so it overlaps the rooms it joins.
    AABB tube(int axis, float a, float b, float c, float y, float w, float h,
              glm::vec3 shell, glm::vec3 rib, bool solidBelow = true) {
        float lo = std::min(a, b), hi = std::max(a, b);
        float c0 = c - w * 0.5f, c1 = c + w * 0.5f;
        float base = solidBelow ? 0.f : y - 1.f;
        Mat keep = mat;
        mat = Mat::PANEL;
        auto box = [&](float u0, float v0, float y0, float u1, float v1, float y1, bool solid, glm::vec3 col) {
            // u: along the axis, v: across it
            if (axis == 0) { if (solid) wall(u0, y0, v0, u1, y1, v1, col); else neon(u0, y0, v0, u1, y1, v1, col); }
            else           { if (solid) wall(v0, y0, u0, v1, y1, u1, col); else neon(v0, y0, u0, v1, y1, u1, col); }
        };
        box(lo, c0 - 1.f, base, hi, c0, y + h, true, shell);          // sides
        box(lo, c1, base, hi, c1 + 1.f, y + h, true, shell);
        box(lo, c0 - 1.f, y + h, hi, c1 + 1.f, y + h + 1.f, true, shell * 0.8f);   // roof
        if (y > 0.01f) box(lo, c0, base, hi, c1, y, true, shell * 0.7f);          // floor
        mat = keep;
        box(lo, c - 0.18f, y, hi, c + 0.18f, y + 0.03f, false, rib * 0.55f);     // floor strip
        int n = (int)((hi - lo) / 3.f);
        for (int i = 1; i < n; ++i) {
            float u = lo + (hi - lo) * i / n;
            box(u - 0.12f, c0, y + 0.25f, u + 0.12f, c0 + 0.07f, y + h, false, rib);
            box(u - 0.12f, c1 - 0.07f, y + 0.25f, u + 0.12f, c1, y + h, false, rib);
            box(u - 0.12f, c0, y + h - 0.07f, u + 0.12f, c1, y + h, false, rib);
        }
        return axis == 0 ? aabb(lo - 1.5f, y, c0, hi + 1.5f, y + h, c1)
                         : aabb(c0, y, lo - 1.5f, c1, y + h, hi + 1.5f);
    }

    void booster(AABB box, glm::vec3 dir, float speed) { L.boosters.push_back({box, glm::normalize(dir), speed}); }

    // A moving platform: a box of half-size `half` centred at `centre` (its top
    // is centre.y + half.y), travelling along a path. Returns the mover index.
    int mover(glm::vec3 centre, glm::vec3 half, Mover::Path path, glm::vec3 a, glm::vec3 b,
              float period, float phase, glm::vec3 glow) {
        Mover m;
        m.base = AABB{centre - half, centre + half};
        m.path = path; m.a = a; m.b = b; m.period = period; m.phase = phase; m.glow = glow;
        glm::vec3 off = m.offsetAt(0.f);
        Wall w{AABB{m.base.min + off, m.base.max + off}, m.color, true};
        w.dynamic = true;
        L.walls.push_back(w);
        m.wall = (int)L.walls.size() - 1;
        L.movers.push_back(m);
        L.moverWalls.push_back(m.wall);
        return (int)L.movers.size() - 1;
    }
};

// =============================================================================
// buildAct1() — ARENA mode (Act I)
// =============================================================================
inline void buildAct1(LevelBuilder& B) {
    using glm::vec3;
    LevelData& L = B.L;
    auto aabb = &LevelBuilder::aabb;
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0,y0,z0,x1,y1,z1,c); };
    auto prop = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.prop(x0,y0,z0,x1,y1,z1,c); };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.neon(x0,y0,z0,x1,y1,z1,c); };
    auto ring = [&](float x0, float z0, float x1, float z1, float y0, float y1, vec3 c) { B.ring(x0,z0,x1,z1,y0,y1,c); };
    // A blocky synthwave palm: a leaning trunk and drooping fronds
    auto palm = [&](float x, float z, float h, float lean) {
        vec3 trunk{0.20f, 0.09f, 0.13f}, leaf{0.09f, 0.04f, 0.10f};
        int segs = (int)(h / 1.5f);
        float tx = x;
        for (int i = 0; i < segs; ++i) {
            prop(tx - 0.3f, i * 1.5f, z - 0.3f, tx + 0.3f, (i + 1) * 1.5f, z + 0.3f, trunk);
            tx += lean * (0.1f + 0.05f * i);
        }
        float top = segs * 1.5f;
        prop(tx - 0.6f, top - 0.2f, z - 0.6f, tx + 0.6f, top + 0.4f, z + 0.6f, leaf);
        for (int s = -1; s <= 1; s += 2) {
            prop(tx, top, z - 0.35f, tx + s * 2.6f, top + 0.2f, z + 0.35f, leaf);
            prop(tx + s * 2.4f, top - 1.0f, z - 0.3f, tx + s * 3.6f, top, z + 0.3f, leaf);
            prop(tx - 0.35f, top, z, tx + 0.35f, top + 0.2f, z + s * 2.6f, leaf);
            prop(tx - 0.3f, top - 1.0f, z + s * 2.4f, tx + 0.3f, top, z + s * 3.6f, leaf);
        }
    };

    // Ground outside the arenas (seen from wall tops), just under the arena floors
    L.floors.push_back({-100.f, -320.f, 100.f, 100.f, -0.3f, {0.12f, 0.07f, 0.08f}});

    // =========================================================================
    // ARENA 1 — SUNSET YARD
    // =========================================================================
    {
        vec3 adobe{0.66f,0.40f,0.30f}, adobeDark{0.50f,0.28f,0.22f}, stone{0.56f,0.48f,0.43f},
             crate{0.47f,0.31f,0.21f}, pink{1.0f,0.25f,0.62f}, cyan{0.2f,0.9f,1.0f};
        Arena a;
        a.name = "SUNSET YARD"; a.space = ReverbSpace::OPEN;
        a.subtitle = "SURVIVE 3 WAVES";
        a.bounds = aabb(-30, 0, -30, 30, 12, 30);
        a.zone   = aabb(-31.5f, 0, -31.5f, 31.5f, 18, 31.5f);   // ceiling 18 m
        a.playerStart = {0.f, 0.f, 24.f};
        L.floors.push_back({-31.f, -31.f, 31.f, 31.f, 0.f, {0.62f, 0.46f, 0.34f}});

        // Perimeter (low, so the sun shows over it)
        wall(-31,0, 30,  31,5, 31, adobe);
        wall( 30,0,-31,  31,5, 31, adobe);
        wall(-31,0,-31, -30,5, 31, adobe);
        wall(-31,0,-31,  -4,5,-30, adobe);
        wall(  4,0,-31,  31,5,-30, adobe);
        a.exitDoor = B.doorway(true, -4, 4, -31, -30, 0.f, 5.f, cyan, true);
        // Gate frame
        wall(-5.5f,0,-31.5f, -4,7.5f,-29.5f, adobeDark);
        wall(  4,0,-31.5f, 5.5f,7.5f,-29.5f, adobeDark);
        wall(-5.5f,5,-31.5f, 5.5f,7.5f,-29.5f, adobeDark);
        neon(-4.9f,0.2f,-29.5f, -4.6f,7.3f,-29.38f, cyan);
        neon( 4.6f,0.2f,-29.5f,  4.9f,7.3f,-29.38f, cyan);
        neon(-5.5f,6.0f,-29.5f,  5.5f,6.3f,-29.38f, cyan);
        // Neon band around the inside of the perimeter
        neon(-30,4.3f, 29.86f,  30,4.5f, 29.98f, pink);
        neon( 29.86f,4.3f,-30,  29.98f,4.5f, 30, pink);
        neon(-29.98f,4.3f,-30, -29.86f,4.5f, 30, pink);
        neon(-30,4.3f,-29.98f, -5.5f,4.5f,-29.86f, pink);
        neon(5.5f,4.3f,-29.98f, 30,4.5f,-29.86f, pink);

        // Central dais + sun obelisk
        wall(-4,0,-4, 4,1,4, stone);
        ring(-4,-4, 4,4, 0.82f, 0.94f, cyan);
        wall(-1,1,-1, 1,7,1, adobeDark);
        ring(-1,-1, 1,1, 3.0f, 3.2f, cyan);
        ring(-1,-1, 1,1, 5.0f, 5.2f, cyan);
        neon(-0.6f,7,-0.6f, 0.6f,7.6f,0.6f, pink);

        // The square is broken up by four adobe buildings in the corners: the
        // ground becomes a central plaza with four arms, and their rooftops
        // are joined by bridges into a ring at 5 m, so there's always a loop
        // to run, on the ground or above it.
        // Each building is hollow: a dim room with an arch onto each arm, so
        // you can duck through it (and grab the health inside) mid-fight.
        auto mn = [](float a, float b) { return std::min(a, b); };
        auto mx = [](float a, float b) { return std::max(a, b); };
        vec3 warm{1.f,0.7f,0.35f};
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            vec3 col = sx * sz > 0 ? adobe : adobeDark;
            float xi = sx * 19.f, xo = sx * 30.f, zi = sz * 19.f, zo = sz * 30.f;
            B.wallZ(mn(zi, zo), mx(zi, zo), mn(xi, xi + sx), mx(xi, xi + sx), 0, 4.6f, col,
                    {LevelBuilder::Gap{mn(sz * 22.5f, sz * 25.5f), mx(sz * 22.5f, sz * 25.5f), 0.f, 2.6f}});
            B.wallX(mn(xi, xo), mx(xi, xo), mn(zi, zi + sz), mx(zi, zi + sz), 0, 4.6f, col,
                    {LevelBuilder::Gap{mn(sx * 22.5f, sx * 25.5f), mx(sx * 22.5f, sx * 25.5f), 0.f, 2.6f}});
            wall(xi, 4.6f, zi, xo, 5, zo, col);                                         // roof (the rooftop ring's floor)
            neon(sx * 18.9f,4.7f,sz * 19.f, sx * 18.96f,4.95f,sz * 30.f, pink);         // roof edge
            neon(sx * 19.f,4.7f,sz * 18.9f, sx * 30.f,4.95f,sz * 18.96f, pink);
            neon(sx * 18.94f,3.0f,sz * 22.5f, sx * 18.98f,3.6f,sz * 25.5f, warm * 0.8f);   // lit windows over the arches
            neon(sx * 22.5f,3.0f,sz * 18.94f, sx * 25.5f,3.6f,sz * 18.98f, warm * 0.8f);
            prop(sx * 18.2f,2.65f,sz * 21.5f, sx * 19.f,2.8f,sz * 27.f, adobeDark);     // awnings
            wall(sx * 17.f,0,sz * 19.f, sx * 19.f,2.5f,sz * 21.f, stone);               // step up to the roof
            wall(sx * 23.f,5,sz * 23.f, sx * 25.f,6.2f,sz * 25.f, crate);               // rooftop cover
            // Inside: a lamp, a bench, a crate, and a health orb
            float cx = sx * 25.f, cz = sz * 25.f;
            neon(cx - 1.2f, 4.48f, cz - 1.2f, cx + 1.2f, 4.6f, cz + 1.2f, warm * 0.7f);
            wall(sx * 27.f, 0, sz * 21.f, sx * 29.f, 0.9f, sz * 23.f, crate);
            wall(sx * 21.f, 0, sz * 27.5f, sx * 24.f, 0.6f, sz * 29.f, adobeDark);
            L.placedPickups.push_back({{sx * 26.f, 0.f, sz * 26.5f}, 0});
        }
        // The rooftop ring: four bridges over the arms (you can walk under them)
        wall(-19,4.6f,-26, 19,5,-23, stone);
        wall(-19,4.6f, 23, 19,5, 26, stone);
        wall(-26,4.6f,-19, -23,5,19, stone);
        wall( 23,4.6f,-19,  26,5,19, stone);
        neon(-19,4.45f,-23.08f, 19,4.6f,-22.95f, cyan); neon(-19,4.45f,22.95f, 19,4.6f,23.08f, cyan);
        neon(-23.08f,4.45f,-19, -22.95f,4.6f,19, cyan); neon(22.95f,4.45f,-19, 23.08f,4.6f,19, cyan);
        // Pads up onto each bridge from the arm below it
        L.pads.push_back({{0.f, 0.f, -20.5f}, {1.3f, 1.3f}, {0.f, 17.f, -5.f}});
        L.pads.push_back({{0.f, 0.f,  20.5f}, {1.3f, 1.3f}, {0.f, 17.f,  5.f}});
        L.pads.push_back({{-20.5f, 0.f, 0.f}, {1.3f, 1.3f}, {-5.f, 17.f, 0.f}});
        L.pads.push_back({{ 20.5f, 0.f, 0.f}, {1.3f, 1.3f}, { 5.f, 17.f, 0.f}});

        // Market stalls in the arms: a counter to duck behind, a canopy to hop on
        const float stalls[][4] = {{-8,-16,-4,-14},{4,-14,8,-12},{-8,12,-4,14},{4,14,8,16},{-16,-6,-14,-2},{14,2,16,6}};
        for (auto& st : stalls) {
            wall(st[0],0,st[1], st[2],1.2f,st[3], crate);
            wall(st[0] - 0.3f,2.6f,st[1] - 0.3f, st[2] + 0.3f,2.8f,st[3] + 0.3f, adobeDark);
            prop(st[0],1.2f,st[1], st[0] + 0.2f,2.6f,st[1] + 0.2f, stone);
            prop(st[2] - 0.2f,1.2f,st[3] - 0.2f, st[2],2.6f,st[3], stone);
        }
        // Broken columns around the plaza
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            float h = 2.5f + 1.5f * ((sx + sz + 2) % 3);
            wall(sx * 10.f - 0.75f,0,sz * 10.f - 0.75f, sx * 10.f + 0.75f,h,sz * 10.f + 0.75f, stone);
        }

        // Palms outside the walls, silhouetted against the sunset
        palm(-37, -18, 10.f,  0.4f); palm(-38, 4, 8.f, -0.3f); palm(-36, 22, 11.f, 0.2f);
        palm( 37, -16, 11.f, -0.4f); palm( 38, 6, 9.f,  0.3f); palm( 36, 24, 8.f, -0.2f);
        palm(-16, -38, 9.f, 0.3f);   palm(14, -37, 11.f, -0.3f); palm(-28, -40, 12.f, 0.2f);
        palm(26, -41, 9.f, 0.2f);    palm(-12, 38, 9.f, 0.3f);   palm(14, 37, 10.f, -0.2f);

        a.groundSpawns = {{-12,0,-20},{12,0,-20},{0,0,-19},{-24,0,-10},{24,0,-10},{-24,0,10},{24,0,10},
                          {-24.5f,5.05f,-27},{24.5f,5.05f,-27},{-27,5.05f,24.5f},{27,5.05f,24.5f},
                          {0,5.05f,-24.5f},{-24.5f,5.05f,0},{24.5f,5.05f,0}};
        a.airSpawns    = {{-15,9,-15},{15,9,-15},{0,10,-20},{-15,9,15},{15,9,15},{0,10,12}};
        a.waves = {
            {{EnemyType::HUSK, 5}, {EnemyType::RIPPER, 2}},
            {{EnemyType::HUSK, 4}, {EnemyType::RIPPER, 4}, {EnemyType::SENTINEL, 1}},
            {{EnemyType::HUSK, 3}, {EnemyType::RIPPER, 3}, {EnemyType::RAPTOR, 3}, {EnemyType::BRUTE, 1}},
        };
        a.goals = { {}, {}, WaveGoal::hold("HOLD THE DAIS", {0.f, 1.f, 0.f}, 3.9f, 15.f) };
        a.maxAlive = 7;
        a.damageScale = 0.85f;
        a.ambient = Ambient::DUST;
        a.shift = ArenaShift::NIGHTFALL;   // the sun sets as the fight goes on
        L.gems.push_back({{0.f, 9.f, 0.f}, {1.6f, 0.35f, 0.9f}, 1.1f, false});   // above the obelisk
        a.theme = Theme{
            {0.12f,0.05f,0.22f}, {1.0f,0.48f,0.32f}, {0.16f,0.07f,0.10f},
            glm::normalize(vec3{0.f, 0.3f, -1.f}), {1.5f,0.62f,0.18f}, 0.2f, 1.f,
            {0.16f,0.06f,0.16f}, 0.35f,
            glm::normalize(vec3{0.25f,-0.45f,0.85f}), {1.15f,0.72f,0.5f},
            {0.34f,0.22f,0.36f}, {0.16f,0.09f,0.08f},
            {0.58f,0.30f,0.30f}, 0.006f };
        L.arenas.push_back(std::move(a));
    }

    // ---- Corridor 1 → 2 ------------------------------------------------------
    // A short ribbed duct between the gates (a tube, like the Gauntlet's)
    {
        B.tube(2, -45.f, -31.f, 0.f, 0.f, 8.f, 6.f, {0.22f,0.21f,0.25f}, {0.2f,0.9f,1.0f});
        L.floors.push_back({-4.f, -46.f, 4.f, -31.f, 0.f, {0.15f,0.15f,0.18f}});
        L.corridors.push_back(aabb(-5, 0, -46.5f, 5, 40, -30.5f));
    }

    // =========================================================================
    // ARENA 2 — THE FOUNDRY
    // =========================================================================
    {
        vec3 iron{0.32f,0.29f,0.28f}, rust{0.48f,0.25f,0.15f}, dark{0.18f,0.17f,0.17f},
             orange{1.0f,0.30f,0.04f}, lava{1.0f,0.24f,0.02f};
        Arena a;
        a.name = "THE FOUNDRY";
        a.subtitle = "SURVIVE 3 WAVES";
        a.bounds = aabb(-32, 0, -110, 32, 13, -46);
        a.zone   = aabb(-33.5f, 0, -111.5f, 33.5f, 14, -44.5f);  // roofed at 14 m
        a.playerStart = {0.f, 0.f, -50.f};
        L.floors.push_back({-33.f, -111.f, 33.f, -45.f, 0.f, {0.25f,0.23f,0.22f}});

        wall(-33,0,-46, -4,14,-45, iron);
        wall(  4,0,-46, 33,14,-45, iron);
        wall( -4,5,-46,  4,14,-45, iron);
        a.entryGate = B.doorway(true, -4, 4, -46, -45, 0.f, 5.f, orange, false);
        wall( 32,0,-111, 33,14,-45, iron);
        wall(-33,0,-111,-32,14,-45, iron);
        wall(-33,0,-111, -4,14,-110, iron);
        wall(  4,0,-111, 33,14,-110, iron);
        wall( -4,5,-111,  4,14,-110, iron);
        a.exitDoor = B.doorway(true, -4, 4, -111, -110, 0.f, 5.f, orange, true);
        wall(-33,14,-111, 33,15,-45, dark);                                    // roof
        // Wall stripes at shoulder height, like hazard paint
        neon(-31.98f,2.0f,-110, -31.88f,2.2f,-46, orange);
        neon( 31.88f,2.0f,-110,  31.98f,2.2f,-46, orange);

        // Central furnace (climb it from the jump pad) with a chimney to the roof
        wall(-5,0,-83, 5,6,-73, rust);
        ring(-5,-83, 5,-73, 1.4f, 1.7f, orange);
        ring(-5,-83, 5,-73, 4.0f, 4.3f, orange);
        neon(-2.f,0.3f,-72.94f, 2.f,2.5f,-72.84f, lava);                      // furnace mouth
        wall(-2,6,-80, 2,14,-76, dark);
        ring(-2,-80, 2,-76, 9.0f, 9.3f, orange);
        ring(-2,-80, 2,-76, 12.0f, 12.3f, orange);
        L.pads.push_back({{0.f, 0.f, -68.5f}, {1.4f, 1.4f}, {0.f, 18.f, -5.f}});

        // Lava channels: jump them, or lure enemies through
        for (float z0 : {-62.f, -97.f}) {
            AABB lv = aabb(-26, 0, z0, 26, 0.06f, z0 + 3.f);
            L.hazards.push_back({lv, 30.f});
            neon(lv.min.x, lv.min.y, lv.min.z, lv.max.x, lv.max.y, lv.max.z, lava);
            prop(-26.4f,0,z0 - 0.4f, 26.4f,0.12f,z0, dark);                  // curbs
            prop(-26.4f,0,z0 + 3.f, 26.4f,0.12f,z0 + 3.4f, dark);
        }

        // Catwalks along the side walls, reached by jump pads
        for (int s : {-1, 1}) {
            float in = s * 24.f, out = s * 32.f;
            wall(in,4.6f,-100, out,5,-56, dark);
            neon(in - s * 0.12f,4.6f,-100, in - s * 0.02f,5.0f,-56, orange);
            for (float z : {-100.f, -78.5f, -57.f})
                wall(in,0,z, in + s * 1.f,4.6f,z + 1.f, iron);
            L.pads.push_back({{s * 20.5f, 0.f, -78.f}, {1.4f, 1.4f}, {s * 6.5f, 16.5f, 0.f}});
        }

        // Cross catwalks over both lava channels join the side catwalks into a
        // loop at 5 m around the furnace
        for (float z : {-61.5f, -96.5f}) {
            wall(-24,4.6f,z, 24,5,z + 2.f, dark);
            neon(-24,4.45f,z - 0.08f, 24,4.6f,z + 2.08f, orange);
        }
        // A crane platform sweeps across the hall: ride it or grapple it
        B.mover({0.f, 9.25f, -85.f}, {2.5f, 0.25f, 1.5f}, Mover::Path::PINGPONG,
                {-18.f, 0, 0}, {18.f, 0, 0}, 10.f, 0.f, orange);
        prop(-20,13.2f,-85.4f, 20,13.6f,-84.6f, dark);                        // its rail on the roof

        // Full-height pillars for cover
        for (int sx : {-1, 1}) for (float z : {-70.f, -90.f}) {
            wall(sx * 13.f,0,z, sx * 15.f,14,z + 2.f, iron);
            ring(std::min(sx * 13.f, sx * 15.f), z, std::max(sx * 13.f, sx * 15.f), z + 2.f, 3.0f, 3.25f, orange);
        }
        // Ingot stacks: low cover with a step on top
        for (auto c : {std::pair<float,float>{-7.f, -103.f}, {7.f, -103.f}, {7.f, -54.f}, {-7.f, -54.f}}) {
            wall(c.first - 3,0,c.second - 1, c.first + 3,1.2f,c.second + 1, rust);
            wall(c.first - 1.5f,1.2f,c.second - 1, c.first + 1.5f,2.4f,c.second + 1, rust * 0.8f);
        }
        wall(-22,0,-84, -18,1.4f,-80,  iron);
        wall( 18,0,-84,  22,1.4f,-80,  iron);

        // Hanging chains from the roof (visual)
        for (float x : {-18.f, 18.f, -8.f, 8.f})
            for (float z : {-60.f, -100.f})
                prop(x - 0.08f, 9.f + std::fabs(x) * 0.05f, z - 0.08f, x + 0.08f, 14.f, z + 0.08f, dark);

        a.groundSpawns = {{-26,0,-104},{26,0,-104},{0,0,-104},{-20,0,-88},{20,0,-88},
                          {-28,5.05f,-90},{28,5.05f,-90},{-28,5.05f,-66},{28,5.05f,-66},
                          {-20,0,-68},{20,0,-68},{-26,0,-52},{26,0,-52},{0,6.05f,-82}};
        a.airSpawns    = {{-12,9,-78},{12,9,-78},{0,10,-95},{0,10,-60},{-20,10,-100},{20,10,-56}};
        a.waves = {
            {{EnemyType::HUSK, 5}, {EnemyType::MITE, 4}, {EnemyType::SENTINEL, 2}},
            {{EnemyType::BRUTE, 1}, WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}), {EnemyType::RIPPER, 5}, {EnemyType::RAPTOR, 2}},
            {WaveEntry(EnemyType::JUGGERNAUT, 1).with({EnemyType::MITE, EnemyType::MITE}), {EnemyType::BRUTE, 1}, {EnemyType::MITE, 4}, {EnemyType::HUSK, 3}, {EnemyType::RAPTOR, 2}},
        };
        // Wave 2 pours out of three conduits: one on each side catwalk, one at the far end
        a.goals = { {}, WaveGoal::conduits("DESTROY THE CONDUITS", {{-28.f, 5.f, -80.f}, {28.f, 5.f, -80.f}, {0.f, 0.f, -104.f}}) };
        a.maxAlive = 9;
        a.damageScale = 1.0f;
        a.ambient = Ambient::EMBERS;
        a.shift = ArenaShift::LAVA_RISE;   // the last wave floods the channels
        a.theme = Theme{
            {0.04f,0.02f,0.02f}, {0.30f,0.10f,0.04f}, {0.05f,0.02f,0.01f},
            glm::normalize(vec3{0.f, 0.3f, -1.f}), {0.f,0.f,0.f}, 0.01f, 0.f,
            {0.06f,0.02f,0.01f}, 0.f,
            glm::normalize(vec3{0.2f,-1.f,0.15f}), {0.55f,0.38f,0.28f},
            {0.10f,0.08f,0.08f}, {0.42f,0.16f,0.05f},
            {0.16f,0.06f,0.03f}, 0.016f };
        L.arenas.push_back(std::move(a));
    }

    // ---- Corridor 2 → 3 ------------------------------------------------------
    // A short ribbed duct between the gates (a tube, like the Gauntlet's)
    {
        B.tube(2, -125.f, -111.f, 0.f, 0.f, 8.f, 6.f, {0.22f,0.21f,0.25f}, {0.2f,0.9f,1.0f});
        L.floors.push_back({-4.f, -126.f, 4.f, -111.f, 0.f, {0.15f,0.15f,0.18f}});
        L.corridors.push_back(aabb(-5, 0, -126.5f, 5, 40, -110.5f));
    }

    // =========================================================================
    // ARENA 3 — THE SPIRE
    //
    //   tier   y    what                         how you get up
    //   ground 0    cover around the tower base
    //   1      6    long ledges on both side walls   jump pads (±19, -176/-136)
    //   2      12   corner landings + two bridges    lifts in the ledge notches
    //   3      18   balcony ringing the tower        diagonal lifts off the bridges
    //   summit 26   the tower top, a beacon          pads on the balcony, or grapple
    //
    // Sweepers cross the middle at ledge height and two platforms orbit the
    // tower; all of them can be grappled. Wave 1 spawns on the ground and the
    // ledges, wave 2 on the ledges and tier 2, wave 3 on the balcony and the
    // summit — and the gunners up there won't step off their perch.
    // =========================================================================
    {
        vec3 stone{0.58f,0.60f,0.68f}, stoneDark{0.36f,0.39f,0.47f}, slab{0.46f,0.48f,0.55f},
             ice{0.35f,0.9f,1.0f}, gold{1.0f,0.72f,0.28f};
        const float CZ = -156.f;    // tower centre (z)
        Arena a;
        a.name = "THE SPIRE";
        a.subtitle = "CLIMB - THEY HOLD THE HIGH GROUND";
        a.bounds = aabb(-30, 0, -186, 30, 34, -126);
        a.zone   = aabb(-31.5f, 0, -187.5f, 31.5f, 36, -124.5f);   // ceiling 36 m
        a.playerStart = {0.f, 0.f, -130.f};
        L.floors.push_back({-31.f, -187.f, 31.f, -125.f, 0.f, {0.44f,0.45f,0.50f}});

        // Perimeter, 9 m, with a gate in each end wall
        wall(-31,0,-126, -4,9,-125, stone);
        wall(  4,0,-126, 31,9,-125, stone);
        wall( -4,6,-126,  4,9,-125, stone);
        a.entryGate = B.doorway(true, -4, 4, -126, -125, 0.f, 6.f, gold, false);
        wall( 30,0,-187, 31,9,-125, stone);
        wall(-31,0,-187,-30,9,-125, stone);
        wall(-31,0,-187, -4,9,-186, stone);
        wall(  4,0,-187, 31,9,-186, stone);
        wall( -4,6,-187,  4,9,-186, stone);
        a.exitDoor = B.doorway(true, -4, 4, -187, -186, 0.f, 6.f, gold, true);
        neon(-30,8.5f,-125.98f, 30,8.7f,-125.86f, ice);                       // trim along the top
        neon(-30,8.5f,-186.14f, 30,8.7f,-186.02f, ice);
        neon( 29.86f,8.5f,-186, 29.98f,8.7f,-126, ice);
        neon(-29.98f,8.5f,-186, -29.86f,8.7f,-126, ice);

        // The tower: solid, 26 m, glowing bands at every tier
        wall(-5,0,CZ - 5, 5,26,CZ + 5, stoneDark);
        for (float y : {5.6f, 11.6f, 17.6f}) ring(-5, CZ - 5, 5, CZ + 5, y, y + 0.3f, ice);
        ring(-5, CZ - 5, 5, CZ + 5, 25.7f, 26.f, gold);
        for (int sx : {-1, 1}) for (int sz : {-1, 1})                         // crown spikes
            wall(sx * 5.f, 26, CZ + sz * 5.f, sx * 4.f, 27.2f, CZ + sz * 4.f, stone);
        L.gems.push_back({{0.f, 30.f, CZ}, {0.4f, 1.4f, 1.8f}, 1.4f, true});  // the beacon

        // Tier 1: ledges along both side walls, with notches for the lifts
        for (int s : {-1, 1}) {
            float in = s * 22.f, out = s * 30.f;
            for (auto seg : {std::pair<float,float>{-184.f, -170.f}, {-166.f, -146.f}, {-142.f, -128.f}}) {
                wall(in, 5.4f, seg.first, out, 6.f, seg.second, slab);
                neon(in - s * 0.12f, 5.45f, seg.first, in - s * 0.02f, 5.95f, seg.second, ice);
            }
            for (float z : {-182.f, -160.f, -150.f, -130.f})                  // struts under it (visual)
                prop(in + s * 0.2f, 0, z, in + s * 1.f, 5.4f, z + 0.8f, stoneDark);
            L.pads.push_back({{s * 19.f, 0.f, -176.f}, {1.4f, 1.4f}, {s * 5.5f, 19.5f, 0.f}});
            L.pads.push_back({{s * 19.f, 0.f, -136.f}, {1.4f, 1.4f}, {s * 5.5f, 19.5f, 0.f}});
            // Lifts in the notches: tier 1 (top 6) up to tier 2 (top 12)
            B.mover({s * 26.f, 5.75f, -168.f}, {3.f, 0.25f, 1.9f}, Mover::Path::PINGPONG,
                    {0, 0, 0}, {0, 6.f, 0}, 7.f, s > 0 ? 0.f : 0.5f, ice);
            B.mover({s * 26.f, 5.75f, -144.f}, {3.f, 0.25f, 1.9f}, Mover::Path::PINGPONG,
                    {0, 0, 0}, {0, 6.f, 0}, 7.f, s > 0 ? 0.5f : 0.f, ice);
        }

        // Tier 2: corner landings and two bridges across the middle
        for (int s : {-1, 1}) {
            for (auto seg : {std::pair<float,float>{-184.f, -170.f}, {-142.f, -128.f}}) {
                wall(s * 22.f, 11.5f, seg.first, s * 30.f, 12.f, seg.second, slab);
                neon(s * 21.88f, 11.55f, seg.first, s * 21.98f, 11.95f, seg.second, gold);
            }
        }
        wall(-22, 11.5f, -182, 22, 12, -176, slab);
        wall(-22, 11.5f, -136, 22, 12, -130, slab);
        neon(-22, 11.55f, -176.02f, 22, 11.95f, -175.9f, gold);
        neon(-22, 11.55f, -136.1f, 22, 11.95f, -135.98f, gold);
        // Diagonal lifts from each bridge up to the balcony
        B.mover({0.f, 11.75f, -174.f}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG,
                {0, 0, 0}, {0, 6.f, 7.f}, 8.f, 0.f, gold);
        B.mover({0.f, 11.75f, -138.f}, {2.f, 0.25f, 2.f}, Mover::Path::PINGPONG,
                {0, 0, 0}, {0, 6.f, -7.f}, 8.f, 0.5f, gold);

        // Tier 3: the balcony ringing the tower, pads up to the summit
        wall(-9, 17.5f, CZ - 9, 9, 18, CZ + 9, slab);
        ring(-9, CZ - 9, 9, CZ + 9, 17.55f, 17.95f, ice);
        for (int s : {-1, 1})
            L.pads.push_back({{s * 7.f, 18.f, CZ}, {1.2f, 1.2f}, {-s * 3.5f, 23.f, 0.f}});

        // Sweepers across the middle at ledge height, and two platforms orbiting the tower
        B.mover({0.f, 5.75f, -167.5f}, {2.5f, 0.25f, 1.5f}, Mover::Path::PINGPONG,
                {-19.f, 0, 0}, {19.f, 0, 0}, 11.f, 0.f, ice);
        B.mover({0.f, 5.75f, -144.5f}, {2.5f, 0.25f, 1.5f}, Mover::Path::PINGPONG,
                {-19.f, 0, 0}, {19.f, 0, 0}, 11.f, 0.5f, ice);
        B.mover({0.f, 23.25f, CZ}, {1.8f, 0.25f, 1.8f}, Mover::Path::ORBIT,
                {13.f, 0, 0}, {0, 0, 13.f}, 16.f, 0.f, gold);
        B.mover({0.f, 23.25f, CZ}, {1.8f, 0.25f, 1.8f}, Mover::Path::ORBIT,
                {13.f, 0, 0}, {0, 0, 13.f}, 16.f, 0.5f, gold);

        // Ground cover
        wall(-14,0,-142, -10,1.3f,-140, stoneDark);
        wall( 10,0,-142,  14,1.3f,-140, stoneDark);
        wall(-14,0,-172, -10,1.3f,-170, stoneDark);
        wall( 10,0,-172,  14,1.3f,-170, stoneDark);
        wall(-17,0,-158, -15,1.5f,-154, stoneDark);
        wall( 15,0,-158,  17,1.5f,-154, stoneDark);

        // Distant peaks, off to either side (north of here is the Core, so keep clear of it)
        const float peaks[][4] = {{-62,-145,26,24},{-76,-182,38,32},{64,-140,24,20},{76,-180,36,34},
                                  {-92,-118,30,22},{94,-120,28,26},{-58,-112,16,14},{60,-198,22,18}};
        for (auto& p : peaks) {
            float x = p[0], z = p[1], w = p[2] * 0.5f, h = p[3];
            prop(x - w, 0, z - w, x + w, h * 0.6f, z + w, {0.30f,0.33f,0.42f});
            prop(x - w * 0.55f, h * 0.6f, z - w * 0.55f, x + w * 0.55f, h, z + w * 0.55f, {0.34f,0.37f,0.47f});
            prop(x - w * 0.25f, h, z - w * 0.25f, x + w * 0.25f, h + 3.f, z + w * 0.25f, {0.85f,0.88f,0.95f});
        }

        // Per-wave spawn tiers
        std::vector<vec3> ground = {{-14,0,-138},{14,0,-138},{-14,0,-176},{14,0,-176},{0,0,-178},{0,0,-134},
                                    {-22,0,-150},{22,0,-150}};
        std::vector<vec3> tier1  = {{-26,6.05f,-178},{26,6.05f,-178},{-26,6.05f,-134},{26,6.05f,-134},
                                    {-26,6.05f,-156},{26,6.05f,-156}};
        std::vector<vec3> tier2  = {{-26,12.05f,-177},{26,12.05f,-177},{-26,12.05f,-135},{26,12.05f,-135},
                                    {-12,12.05f,-179},{12,12.05f,-179},{-12,12.05f,-133},{12,12.05f,-133}};
        std::vector<vec3> tier3  = {{0,18.05f,-163},{0,18.05f,-149},{-7,18.05f,-163},{7,18.05f,-149},
                                    {0,26.05f,CZ},{-2.5f,26.05f,CZ + 2.5f},{2.5f,26.05f,CZ - 2.5f}};
        auto join = [](std::vector<vec3> x, const std::vector<vec3>& y) { x.insert(x.end(), y.begin(), y.end()); return x; };
        a.groundSpawns = join(ground, tier1);
        a.waveGround = { join(ground, tier1), join(tier1, tier2), join(tier2, tier3) };
        a.airSpawns  = {{-14,16,-140},{14,16,-140},{-14,16,-172},{14,16,-172},{0,30,-140},{-20,26,CZ},{20,26,CZ}};
        a.waves = {
            {{EnemyType::HUSK, 4}, {EnemyType::RIPPER, 3}, {EnemyType::SENTINEL, 2}},
            {{EnemyType::SENTINEL, 3}, WaveEntry(EnemyType::SHIELDBEARER, 1).with({EnemyType::HUSK, EnemyType::HUSK}), {EnemyType::HUSK, 1}, {EnemyType::RAPTOR, 3}, {EnemyType::MITE, 4}},
            {{EnemyType::JUGGERNAUT, 1}, {EnemyType::BRUTE, 1}, {EnemyType::SENTINEL, 2}, {EnemyType::HUSK, 3}, {EnemyType::RAPTOR, 2},
             {EnemyType::CONDUCTOR, 1}},
        };
        // Wave 2: take the balcony ringing the tower, under fire from the ledges and tier 2
        a.goals = { {}, WaveGoal::hold("HOLD THE BALCONY", {0.f, 18.f, CZ}, 8.6f, 20.f) };
        a.maxAlive = 9;
        a.damageScale = 1.1f;
        a.ambient = Ambient::WIND;
        a.shift = ArenaShift::SPEED_UP;    // the platforms quicken each wave
        a.theme = Theme{
            {0.10f,0.18f,0.40f}, {0.88f,0.74f,0.62f}, {0.24f,0.24f,0.31f},
            glm::normalize(vec3{-0.55f, 0.16f, -1.f}), {1.7f,1.25f,0.8f}, 0.07f, 0.f,
            {0.30f,0.32f,0.44f}, 0.12f,
            glm::normalize(vec3{0.45f,-0.5f,0.8f}), {1.15f,0.98f,0.82f},
            {0.40f,0.45f,0.60f}, {0.17f,0.16f,0.20f},
            {0.60f,0.58f,0.66f}, 0.0075f };
        L.arenas.push_back(std::move(a));
    }

    // ---- Corridor 3 → 4 ------------------------------------------------------
    // A short ribbed duct between the gates (a tube, like the Gauntlet's)
    {
        B.tube(2, -201.f, -187.f, 0.f, 0.f, 8.f, 6.f, {0.22f,0.21f,0.25f}, {0.2f,0.9f,1.0f});
        L.floors.push_back({-4.f, -202.f, 4.f, -187.f, 0.f, {0.15f,0.15f,0.18f}});
        L.corridors.push_back(aabb(-5, 0, -202.5f, 5, 40, -186.5f));
    }

    // =========================================================================
    // ARENA 4 — THE CORE
    // =========================================================================
    {
        vec3 slate{0.24f,0.28f,0.34f}, slateDark{0.15f,0.18f,0.22f},
             cyan{0.15f,0.95f,1.0f}, magenta{1.0f,0.2f,0.7f};
        const float CZ = -238.f;    // reactor centre (z)
        Arena a;
        a.name = "THE CORE";
        a.subtitle = "SURVIVE 2 WAVES - THEN THE WARDEN";
        a.bounds = aabb(-36, 0, -274, 36, 14, -202);
        a.zone   = aabb(-37.5f, 0, -275.5f, 37.5f, 18, -200.5f);   // ceiling 18 m
        a.playerStart = {0.f, 0.f, -206.f};
        a.bossSpawn   = {20.f, 0.f, CZ};   // mirrored to whichever side is farther from you
        L.floors.push_back({-37.f, -275.f, 37.f, -201.f, 0.f, {0.20f,0.24f,0.28f}});

        wall(-37,0,-202, -4,6,-201, slate);
        wall(  4,0,-202, 37,6,-201, slate);
        a.entryGate = B.doorway(true, -4, 4, -202, -201, 0.f, 6.f, cyan, false);
        // Gate frame: posts and a lintel standing proud of the low wall
        wall(-5.5f,0,-202.5f, -4,8,-201, slateDark); wall(4,0,-202.5f, 5.5f,8,-201, slateDark);
        wall(-5.5f,6,-202.5f, 5.5f,8,-201, slateDark);
        neon(-5.5f,7.6f,-202.62f, 5.5f,7.8f,-202.5f, magenta);
        wall( 36,0,-275, 37,6,-201, slate);
        wall(-37,0,-275,-36,6,-201, slate);
        // North wall: the way on to the Sanctum, shut until the Warden falls
        // (low enough to pass under the walkway that runs above it)
        wall(-37,0,-275, -4,6,-274, slate);
        wall(  4,0,-275, 37,6,-274, slate);
        wall( -4,3.4f,-275, 4,6,-274, slate);
        a.exitDoor = B.doorway(true, -4, 4, -275, -274, 0.f, 3.4f, magenta, true);
        neon(-36,5.3f,-274.0f+0.02f, 36,5.5f,-273.88f, cyan);
        neon( 35.88f,5.3f,-274, 35.98f,5.5f,-202, cyan);
        neon(-35.98f,5.3f,-274, -35.88f,5.5f,-202, cyan);
        neon(-36,5.3f,-202.12f, -4.6f,5.5f,-202.02f, cyan);
        neon(4.6f,5.3f,-202.12f, 36,5.5f,-202.02f, cyan);

        // Reactor pedestal; the core itself is animated in GameplayState
        wall(-5,0,CZ - 5, 5,1.5f,CZ + 5, slate);
        ring(-5,CZ - 5, 5,CZ + 5, 1.2f, 1.4f, magenta);
        L.walls.push_back(Wall{aabb(-2,1.5f,CZ - 2, 2,11,CZ + 2), {0.1f,0.1f,0.1f}, true});
        L.hasReactor = true;
        L.reactorPos = {0.f, 1.5f, CZ};

        // Ring of eight pillars
        for (int k = 0; k < 8; ++k) {
            float ang = glm::radians(22.5f + 45.f * k);
            float cx = std::cos(ang) * 18.f, cz = CZ + std::sin(ang) * 18.f;
            wall(cx - 1,0,cz - 1, cx + 1,7,cz + 1, slate);
            neon(cx - 1.05f,7,cz - 1.05f, cx + 1.05f,7.25f,cz + 1.05f, cyan);
            ring(cx - 1,cz - 1, cx + 1,cz + 1, 2.0f, 2.15f, cyan);
        }

        // The Warden's four conduits: a node on every other pillar, 3 m up,
        // facing the reactor (LevelData::anchors, CONDUIT; Gameplay_Warden.h)
        for (int k = 0; k < 8; k += 2) {
            float ang = glm::radians(22.5f + 45.f * k);
            glm::vec3 p{std::cos(ang) * 16.6f, 3.f, CZ + std::sin(ang) * 16.6f};
            int wi = wall(p.x - 0.5f, 2.5f, p.z - 0.5f, p.x + 0.5f, 3.5f, p.z + 0.5f, slateDark);
            L.addAnchor(wi, p, 250.f, LevelData::ChainAnchor::CONDUIT);
        }

        // Corner perches with jump pads
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            float zIn = sz < 0 ? CZ - 24.f : CZ + 24.f, zOut = sz < 0 ? CZ - 36.f : CZ + 36.f;
            wall(sx * 24.f,0,zIn, sx * 36.f,4,zOut, slateDark);
            neon(sx * 23.88f,3.6f,zIn, sx * 23.98f,3.85f,zOut, magenta);
            float zFace = zIn + (sz < 0 ? 0.12f : -0.12f);
            neon(sx * 24.f,3.6f,zFace, sx * 36.f,3.85f,zIn + (sz < 0 ? 0.02f : -0.02f), magenta);
            float pz = sz < 0 ? CZ - 21.f : CZ + 21.f;
            L.pads.push_back({{sx * 21.f, 0.f, pz}, {1.4f, 1.4f}, {sx * 6.f, 15.5f, sz < 0 ? -6.f : 6.f}});
        }

        // Outer ring at 4 m: walkways along every wall join the four perches
        wall(-24,3.6f,CZ - 36, 24,4,CZ - 33, slateDark);
        wall(-24,3.6f,CZ + 33, 24,4,CZ + 36, slateDark);
        wall(-36,3.6f,CZ - 24, -33,4,CZ + 24, slateDark);
        wall( 33,3.6f,CZ - 24,  36,4,CZ + 24, slateDark);
        neon(-24,3.45f,CZ - 33.f, 24,3.6f,CZ - 32.9f, magenta); neon(-24,3.45f,CZ + 32.9f, 24,3.6f,CZ + 33.f, magenta);
        neon(-33.f,3.45f,CZ - 24, -32.9f,3.6f,CZ + 24, magenta); neon(32.9f,3.45f,CZ - 24, 33.f,3.6f,CZ + 24, magenta);
        // Inner ring at 7 m across the pillar tops: bridges on the four straight
        // edges, stepping slabs to jump across the diagonals; pads up from the floor
        const float P = 16.63f, Q = 6.89f;
        wall(-Q,6.6f,CZ + P - 1, Q,7,CZ + P + 1, slate);
        wall(-Q,6.6f,CZ - P - 1, Q,7,CZ - P + 1, slate);
        wall( P - 1,6.6f,CZ - Q,  P + 1,7,CZ + Q, slate);
        wall(-P - 1,6.6f,CZ - Q, -P + 1,7,CZ + Q, slate);
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            wall(sx * 11.76f - 1.5f,6.6f,CZ + sz * 11.76f - 1.5f, sx * 11.76f + 1.5f,7,CZ + sz * 11.76f + 1.5f, slate);
            ring(sx * 11.76f - 1.5f, CZ + sz * 11.76f - 1.5f, sx * 11.76f + 1.5f, CZ + sz * 11.76f + 1.5f, 6.6f, 6.75f, cyan);
        }
        L.pads.push_back({{0.f, 0.f, CZ + 11.f}, {1.2f, 1.2f}, {0.f, 20.f,  5.f}});
        L.pads.push_back({{0.f, 0.f, CZ - 11.f}, {1.2f, 1.2f}, {0.f, 20.f, -5.f}});
        L.pads.push_back({{ 11.f, 0.f, CZ}, {1.2f, 1.2f}, { 5.f, 20.f, 0.f}});
        L.pads.push_back({{-11.f, 0.f, CZ}, {1.2f, 1.2f}, {-5.f, 20.f, 0.f}});

        // Low cover
        wall(-12,0,CZ + 22, -8,1.3f,CZ + 24, slateDark);
        wall(  8,0,CZ + 22, 12,1.3f,CZ + 24, slateDark);
        wall(-12,0,CZ - 24, -8,1.3f,CZ - 22, slateDark);
        wall(  8,0,CZ - 24, 12,1.3f,CZ - 22, slateDark);
        wall(-27,0,CZ - 2, -25,1.3f,CZ + 2, slateDark);
        wall( 25,0,CZ - 2,  27,1.3f,CZ + 2, slateDark);

        // A dead city skyline beyond the walls
        const float towers[][4] = {{-52,12,8,34},{-58,-16,10,26},{-48,-42,7,40},{52,22,9,30},
                                   {60,-8,12,22},{50,-48,8,44}};
        for (auto& t : towers) {
            float x = t[0], z = CZ + t[1], w = t[2] * 0.5f, h = t[3];
            prop(x - w,0,z - w, x + w,h,z + w, {0.05f,0.06f,0.08f});
            for (float y = 4.f; y < h - 2.f; y += 5.f)
                neon(x - w - 0.05f,y,z - w - 0.05f, x + w + 0.05f,y + 0.25f,z + w + 0.05f,
                     (int)(y + x) % 2 ? cyan * 0.5f : magenta * 0.4f);
        }

        a.groundSpawns = {{30,4.05f,CZ - 30},{-30,4.05f,CZ - 30},{30,4.05f,CZ + 30},{-30,4.05f,CZ + 30},
                          {0,0,CZ - 30},{-18,0,CZ - 30},{18,0,CZ - 30},{-31,0,CZ},{31,0,CZ},
                          {0,0,CZ + 13},{-14,0,CZ - 13},{14,0,CZ - 13},
                          {0,4.05f,CZ - 34.5f},{34.5f,4.05f,CZ},{-34.5f,4.05f,CZ},{0,7.05f,CZ + P},{0,7.05f,CZ - P}};
        a.airSpawns    = {{-18,10,CZ - 18},{18,10,CZ - 18},{0,12,CZ - 28},{-20,10,CZ + 22},{20,10,CZ + 22},{0,12,CZ + 17}};
        a.waves = {
            {{EnemyType::BRUTE, 2}, WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}), {EnemyType::SENTINEL, 1}, {EnemyType::RAPTOR, 2}, {EnemyType::RIPPER, 5},
             {EnemyType::CONDUCTOR, 1}},
            {WaveEntry(EnemyType::JUGGERNAUT, 1).with({EnemyType::MITE, EnemyType::MITE, EnemyType::MITE}), {EnemyType::BRUTE, 1}, {EnemyType::MITE, 5}, {EnemyType::HUSK, 5}, {EnemyType::RAPTOR, 2}, {EnemyType::SENTINEL, 2},
             {EnemyType::CONDUCTOR, 1}},
            {{EnemyType::WARDEN, 1}},
        };
        // Wave 2: the reactor overloads; hold out until it vents
        a.goals = { {}, WaveGoal::survive("SURVIVE THE OVERLOAD", 45.f) };
        a.maxAlive = 11;
        a.damageScale = 1.25f;
        a.ambient = Ambient::MOTES;
        a.shift = ArenaShift::OVERLOAD;    // the overload wave: rings of energy off the reactor
        a.theme = Theme{
            {0.005f,0.012f,0.04f}, {0.05f,0.22f,0.30f}, {0.01f,0.03f,0.04f},
            glm::normalize(vec3{0.45f, 0.32f, -1.f}), {0.8f,1.1f,1.3f}, 0.09f, 0.f,
            {0.02f,0.05f,0.08f}, 1.f,
            glm::normalize(vec3{-0.35f,-0.6f,0.7f}), {0.55f,0.75f,0.95f},
            {0.14f,0.22f,0.32f}, {0.04f,0.06f,0.08f},
            {0.04f,0.12f,0.16f}, 0.012f };
        L.arenas.push_back(std::move(a));
    }

    // ---- Corridor 4 → 5 ------------------------------------------------------
    {
        B.tube(2, -291.f, -275.f, 0.f, 0.f, 8.f, 6.f, {0.2f,0.16f,0.18f}, {1.0f,0.3f,0.2f});
        L.floors.push_back({-4.f, -291.f, 4.f, -275.f, 0.f, {0.14f,0.11f,0.12f}});
        L.corridors.push_back(aabb(-5, 0, -291.5f, 5, 40, -274.5f));
    }

    // =========================================================================
    // ARENA 5 — THE SANCTUM: the Sovereign's duelling ground
    // =========================================================================
    // Built for movement as much as for the fight: a wide open floor to dash
    // around him, a ring of tall pillars to grapple, four raised corners and
    // four floating islands reached by jump pads, and two platforms orbiting
    // the middle. High ground buys you a moment, not safety: he leaps.
    {
        vec3 basalt{0.2f,0.17f,0.19f}, basaltDark{0.12f,0.1f,0.12f},
             crimson{1.0f,0.22f,0.12f}, gold{1.0f,0.72f,0.3f};
        const float CZ = -348.f;    // centre (z)
        Arena a;
        a.name = "THE SANCTUM"; a.space = ReverbSpace::HALL;
        a.subtitle = "HE'S WAITING";
        a.bounds = aabb(-56, 0, -404, 56, 30, -292);
        a.zone   = aabb(-57.5f, 0, -405.5f, 57.5f, 34, -290.5f);   // ceiling 34 m
        a.playerStart = {0.f, 0.f, -296.f};
        a.bossSpawn   = {0.f, 0.f, CZ - 30.f};
        // The floor: a dark plaza with a ringed seal in the middle
        L.floors.push_back({-57.f, -405.f, 57.f, -291.f, 0.f, {0.15f,0.12f,0.13f}});
        L.floors.push_back({-16.f, CZ - 16.f, 16.f, CZ + 16.f, 0.005f, {0.22f,0.15f,0.12f}});
        L.floors.push_back({-11.f, CZ - 11.f, 11.f, CZ + 11.f, 0.01f, {0.12f,0.08f,0.09f}});
        L.floors.push_back({ -5.f, CZ -  5.f,  5.f, CZ +  5.f, 0.015f, {0.3f,0.2f,0.12f}});
        for (float r : {16.f, 11.f}) {
            neon(-r, 0.01f, CZ - r - 0.12f, r, 0.05f, CZ - r, crimson * 0.7f);
            neon(-r, 0.01f, CZ + r, r, 0.05f, CZ + r + 0.12f, crimson * 0.7f);
            neon(-r - 0.12f, 0.01f, CZ - r, -r, 0.05f, CZ + r, crimson * 0.7f);
            neon(r, 0.01f, CZ - r, r + 0.12f, 0.05f, CZ + r, crimson * 0.7f);
        }

        // Gate and perimeter
        wall(-57,0,-292, -4,9,-291, basalt);
        wall(  4,0,-292, 57,9,-291, basalt);
        wall( -4,6,-292,  4,9,-291, basalt);
        a.entryGate = B.doorway(true, -4, 4, -292, -291, 0.f, 6.f, crimson, false);
        wall(-5.5f,0,-292.5f, -4,10,-291, basaltDark); wall(4,0,-292.5f, 5.5f,10,-291, basaltDark);
        wall( 56,0,-405, 57,9,-291, basalt);
        wall(-57,0,-405,-56,9,-291, basalt);
        wall(-57,0,-405, 57,9,-404, basalt);
        neon(-56,8.2f,-403.88f, 56,8.45f,-403.78f, crimson);
        neon( 55.78f,8.2f,-404, 55.88f,8.45f,-292, crimson);
        neon(-55.88f,8.2f,-404, -55.78f,8.45f,-292, crimson);
        neon(-56,8.2f,-292.22f, -5.5f,8.45f,-292.12f, crimson);
        neon(5.5f,8.2f,-292.22f, 56,8.45f,-292.12f, crimson);

        // A ring of eight tall pillars: grapple anchors and cover
        for (int k = 0; k < 8; ++k) {
            float ang = glm::radians(22.5f + 45.f * k);
            float cx = std::cos(ang) * 34.f, cz = CZ + std::sin(ang) * 34.f;
            wall(cx - 1.5f,0,cz - 1.5f, cx + 1.5f,18,cz + 1.5f, basalt);
            ring(cx - 1.5f,cz - 1.5f, cx + 1.5f,cz + 1.5f, 6.f, 6.2f, gold * 0.8f);
            ring(cx - 1.5f,cz - 1.5f, cx + 1.5f,cz + 1.5f, 12.f, 12.2f, gold * 0.8f);
            neon(cx - 1.55f,18,cz - 1.55f, cx + 1.55f,18.2f,cz + 1.55f, crimson);
        }

        // Raised corners (5 m) with jump pads up to them
        for (int sx : {-1, 1}) for (int sz : {-1, 1}) {
            float x0 = sx * 34.f, x1 = sx * 44.f, z0 = CZ + sz * 34.f, z1 = CZ + sz * 44.f;
            wall(x0,0,z0, x1,5,z1, basaltDark);
            ring(std::min(x0,x1), std::min(z0,z1), std::max(x0,x1), std::max(z0,z1), 4.7f, 4.9f, crimson);
            L.pads.push_back({{sx * 28.5f, 0.f, CZ + sz * 28.5f}, {1.3f, 1.3f}, {sx * 8.f, 17.f, sz * 8.f}});
        }

        // Four floating islands at 11 m (north, south, east, west), each with a pad below
        for (int k = 0; k < 4; ++k) {
            float dx = k == 0 ? 0.f : k == 1 ? 0.f : k == 2 ? 1.f : -1.f;
            float dz = k == 0 ? -1.f : k == 1 ? 1.f : 0.f;
            float cx = dx * 24.f, cz = CZ + dz * 24.f;
            wall(cx - 3.f,10,cz - 3.f, cx + 3.f,11,cz + 3.f, basalt);
            ring(cx - 3.f,cz - 3.f, cx + 3.f,cz + 3.f, 10.f, 10.15f, gold);
            L.pads.push_back({{dx * 14.f, 0.f, CZ + dz * 14.f}, {1.2f, 1.2f}, {dx * 6.5f, 25.f, dz * 6.5f}});
        }

        // Two platforms orbiting the seal at 6 m: ride them, hook them
        B.mover({0.f, 6.f, CZ}, {2.5f, 0.3f, 2.5f}, Mover::Path::ORBIT, {19.f, 0.f, 0.f}, {0.f, 0.f, 19.f}, 18.f, 0.f, crimson);
        B.mover({0.f, 6.f, CZ}, {2.5f, 0.3f, 2.5f}, Mover::Path::ORBIT, {19.f, 0.f, 0.f}, {0.f, 0.f, 19.f}, 18.f, 0.5f, crimson);

        // Broken column stumps: a little low cover, nothing that boxes you in
        const float stumps[][2] = {{-20, -9}, {20, 9}, {-9, 20}, {9, -20}};
        for (auto& st : stumps) {
            wall(st[0] - 1.2f,0,CZ + st[1] - 1.2f, st[0] + 1.2f,1.3f,CZ + st[1] + 1.2f, basaltDark);
            ring(st[0] - 1.2f,CZ + st[1] - 1.2f, st[0] + 1.2f,CZ + st[1] + 1.2f, 1.1f, 1.25f, gold * 0.6f);
        }

        // Beyond the walls: kneeling colossi and black spires against the eclipse
        for (int k = 0; k < 3; ++k) {
            float x = -40.f + 40.f * k, z = -430.f;
            vec3 st{0.06f,0.05f,0.06f};
            prop(x - 5,0,z - 4, x + 5,10,z + 4, st);                 // folded legs
            prop(x - 4,10,z - 3, x + 4,26,z + 3, st);                // torso
            prop(x - 2,26,z - 2, x + 2,31,z + 2, st);                // head
            prop(x - 7,14,z - 1, x - 4,24,z + 1, st);                // arms on the sword
            prop(x + 4,14,z - 1, x + 7,24,z + 1, st);
            prop(x - 0.6f,0,z + 4, x + 0.6f,22,z + 5.5f, st);        // the sword, point down
            neon(x - 1.2f,28.5f,z + 2.02f, x + 1.2f,28.8f,z + 2.1f, crimson);   // eyes
        }
        const float spires[][4] = {{-75,-330,6,46},{78,-360,7,52},{-80,-385,8,38},{72,-310,5,34},{-70,-300,5,30},{82,-395,6,44}};
        for (auto& t : spires) {
            prop(t[0] - t[2] * 0.5f,0,t[1] - t[2] * 0.5f, t[0] + t[2] * 0.5f,t[3],t[1] + t[2] * 0.5f, {0.05f,0.04f,0.05f});
            neon(t[0] - t[2] * 0.5f - 0.05f,t[3] - 3.f,t[1] - t[2] * 0.5f - 0.05f,
                 t[0] + t[2] * 0.5f + 0.05f,t[3] - 2.6f,t[1] + t[2] * 0.5f + 0.05f, crimson * 0.6f);
        }

        a.groundSpawns = {{0,0,CZ - 30},{-30,0,CZ},{30,0,CZ},{0,0,CZ + 25}};
        a.airSpawns    = {{0,12,CZ}};
        a.waves = { {{EnemyType::SOVEREIGN, 1}} };
        a.maxAlive = 1;
        a.damageScale = 1.2f;
        a.ambient = Ambient::ASH;
        a.theme = Theme{
            {0.025f,0.0f,0.015f}, {0.42f,0.08f,0.05f}, {0.04f,0.01f,0.01f},
            glm::normalize(vec3{0.f, 0.3f, -1.f}), {2.0f,1.55f,1.0f}, 0.16f, 0.f,
            {0.07f,0.02f,0.02f}, 0.7f,
            // Key light from behind the gate (the eclipse is a dark disc), so
            // the boss is lit from where you face him
            glm::normalize(vec3{-0.25f,-0.55f,-1.f}), {1.2f,0.85f,0.7f},
            {0.32f,0.16f,0.15f}, {0.09f,0.04f,0.04f},
            {0.16f,0.04f,0.04f}, 0.007f };
        L.arenas.push_back(std::move(a));
    }
}

// ARENA mode's level: Act I alone
inline LevelData buildLevel() {
    LevelData L;
    LevelBuilder B{L};
    buildAct1(B);
    return L;
}
