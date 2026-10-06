#pragma once
// =============================================================================
// WaveDirector.h — runs the arena → wave → arena loop. No OpenGL.
//
//   INTRO ──(2.5 s)──▶ ACTIVE ──(queue empty, all dead)──▶ BREAK ──(3 s)──▶ ACTIVE …
//                        │                                  (last wave)
//                        └─────────────────────────────────▶ CLEARED ──(player walks
//                                                             into next arena)──▶ INTRO
//                                                            (last arena) ──▶ VICTORY
//
// A wave is a queue of enemies. At most Arena::maxAlive are on the field at
// once; the rest trickle in as you kill, so a wave keeps pressure on without
// dumping fifteen enemies on you in one frame. Spawn points are picked away
// from the player so nothing materialises on top of you.
//
// FAST mode (fast = true) is a time trial: a section's whole wave appears at
// once at its hand-placed points, and the next wave follows the moment one is
// cleared. Clearing a section opens its gate and moves to APPROACH for the
// next one: a breather stretch, until the player walks into that section's
// trigger and its fight begins. Clearing the last section sends FINISH_OPEN
// instead of VICTORY: the run ends when the player reaches the beacon.
//
// A wave can have a goal other than killing everything (WaveGoal, Level.h):
// hold a circle, destroy the conduits it spawns out of, or survive a timer.
// Those waves refill as you kill until the goal is met; then GOAL_DONE tells
// the caller to collapse whatever is left, and the wave clears. Entries can
// be squads: a leader and its escort, spawned together in formation.
//
// The director only *requests* spawns and reports events (wave started, new
// enemy type seen, arena cleared…); GameplayState owns the enemies, banners
// and doors. That split is what lets tests/test_game.cpp drive a whole run.
// =============================================================================
#include "Level.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>

struct SpawnRequest { EnemyType type; glm::vec3 pos; Hollow hollow = Hollow::NONE; };

enum class DirectorEvent { ARENA_START, WAVE_START, BOSS_START, NEW_TYPE, WAVE_CLEARED, ARENA_CLEARED, VICTORY, FINISH_OPEN,
                           GOAL_DONE };
struct DirectorEventRec { DirectorEvent kind; int value; };

class WaveDirector {
public:
    // APPROACH (FAST): the section is reached but its fight hasn't started;
    // it does when the player walks into the section's trigger.
    enum class Phase { INTRO, ACTIVE, BREAK, CLEARED, VICTORY, APPROACH };

    static constexpr float INTRO_TIME  = 2.5f;
    static constexpr float BREAK_TIME  = 3.0f;
    static constexpr float SPAWN_GAP   = 0.45f;   // seconds between trickled spawns
    static constexpr float SAFE_RADIUS = 13.f;    // don't spawn closer than this to the player
    static constexpr float DRY_DEPTH   = 1.2f;    // ground spawns under more water than this aren't used

    const LevelData* level = nullptr;
    bool  fast = false;
    bool  hold = false;   // set by the caller: keep the next wave waiting (the Descent's cage is riding)
    // Difficulty (ARENA): waves are this many times bigger (the boss is still
    // one Warden) and this many more may be on the field at once
    float countScale    = 1.f;
    int   maxAliveBonus = 0;
    int   arena = 0, wave = 0;
    Phase phase = Phase::INTRO;
    float timer = 0.f;
    struct Queued { EnemyType type; bool fixed; glm::vec3 pos; std::vector<EnemyType> escort; Hollow variant; };
    std::vector<Queued>            queue;
    std::vector<DirectorEventRec>  events;      // drained by the caller every frame
    bool  seen[(int)EnemyType::COUNT] = {};
    bool  seenHollow[(int)EnemyType::COUNT][4] = {};   // variants introduced so far

    // ---- the current wave's goal ----
    float goalTimer = 0.f;          // HOLD: seconds held; SURVIVE: seconds left
    bool  goalDone  = false;
    bool  holding   = false;        // HOLD: the player is in the circle (for the HUD)
    bool  zoneContested = false;    // HOLD: set by the caller each tick, an enemy on foot is in the circle
    std::vector<bool> conduitAlive; // CONDUITS: which pylons still stand

    const WaveGoal& goal() const {
        static const WaveGoal none;
        const auto& g = current().goals;
        return (phase == Phase::ACTIVE || phase == Phase::BREAK || phase == Phase::INTRO) && wave < (int)g.size() ? g[wave] : none;
    }
    bool hasGoal() const { return goal().kind != WaveGoal::KILL_ALL; }
    int  conduitsLeft() const { int n = 0; for (bool a : conduitAlive) n += a; return n; }
    // 0..1 toward the goal
    float goalProgress() const {
        const WaveGoal& g = goal();
        switch (g.kind) {
            case WaveGoal::HOLD:     return std::min(1.f, goalTimer / g.seconds);
            case WaveGoal::SURVIVE:  return 1.f - std::max(0.f, goalTimer) / g.seconds;
            case WaveGoal::CONDUITS: return conduitAlive.empty() ? 0.f : 1.f - (float)conduitsLeft() / conduitAlive.size();
            default:                 return 0.f;
        }
    }
    bool inHoldZone(glm::vec3 p) const {
        const WaveGoal& g = goal();
        glm::vec3 c = goalPos();
        return glm::length(glm::vec2{p.x - c.x, p.z - c.z}) < g.radius && p.y > c.y - 0.6f && p.y < c.y + 3.5f;
    }
    // The caller destroyed a conduit (the one nearest p)
    void onConduitDestroyed(glm::vec3 p) {
        const WaveGoal& g = goal();
        int best = -1; float bestD = 1e9f;
        for (int i = 0; i < (int)conduitAlive.size() && i < (int)g.points.size(); ++i) {
            float d = glm::length(g.points[i] - p);
            if (conduitAlive[i] && d < bestD) { bestD = d; best = i; }
        }
        if (best >= 0) conduitAlive[best] = false;
    }

    // Where the current goal's circle is now (a HOLD riding a mover follows it)
    glm::vec3 goalPos() const {
        const WaveGoal& g = goal();
        if (g.mover >= 0 && level && g.mover < (int)level->movers.size()) {
            const AABB& b = level->walls[level->movers[g.mover].wall].box;
            return {(b.min.x + b.max.x) * 0.5f, b.max.y, (b.min.z + b.max.z) * 0.5f};
        }
        return g.pos;
    }

    void startArena(int a) {
        arena = a; wave = 0;
        phase = Phase::INTRO; timer = fast ? 0.f : INTRO_TIME;
        queue.clear(); spawnTimer = 0.f;
        events.push_back({DirectorEvent::ARENA_START, a});
    }

    // FAST: wait at section a's breather until the player reaches its trigger
    void approach(int a) {
        arena = a; wave = 0;
        phase = Phase::APPROACH;
        queue.clear(); spawnTimer = 0.f;
    }

    const Arena& current() const { return level->arenas[arena]; }
    int  waveCount() const       { return (int)current().waves.size(); }
    int  queued() const          { return (int)queue.size(); }
    int  maxAlive() const        { return current().maxAlive + maxAliveBonus; }
    // How many of an entry this wave brings (hand-placed ones are exact)
    // (leaders: a squad's escort comes on top)
    int  countOf(const WaveEntry& e) const {
        if (!e.at.empty() || isBoss(e.type)) return e.at.empty() ? e.count : (int)e.at.size();
        return std::max(1, (int)std::lround(e.count * countScale));
    }
    bool fighting() const        { return phase == Phase::ACTIVE; }
    bool bossWave() const {
        for (auto& e : current().waves[wave]) if (isBoss(e.type)) return true;
        return false;
    }

    // aliveCount: enemies currently on the field (spawning ones included).
    void update(float dt, int aliveCount, glm::vec3 playerPos, std::vector<SpawnRequest>& out) {
        switch (phase) {
        case Phase::INTRO:
            if (!hold) timer -= dt;
            if (timer <= 0.f) beginWave();
            break;
        case Phase::ACTIVE: {
            spawnTimer -= dt;
            for (auto& r : fixedOut) out.push_back(r);   // a goal's conduits: straight away, outside the cap
            aliveCount += (int)fixedOut.size();
            fixedOut.clear();
            if (fast) {
                for (auto& q : queue) aliveCount += emitSquad(q, playerPos, out);
                queue.clear();
            } else if (!queue.empty() && spawnTimer <= 0.f &&
                       (aliveCount + 1 + (int)queue.front().escort.size() <= maxAlive() || aliveCount == 0)) {
                Queued q = queue.front();
                queue.erase(queue.begin());
                aliveCount += emitSquad(q, playerPos, out);   // on the field now; don't call the wave clear this tick
                spawnTimer = SPAWN_GAP;
            }
            if (hasGoal() && !goalDone) {
                updateGoal(dt, playerPos);
                if (queue.empty() && !goalDone) buildQueue(false);   // they keep coming until it's done
            }
            if (queue.empty() && aliveCount == 0 && (!hasGoal() || goalDone)) {
                if (wave + 1 < waveCount()) {
                    events.push_back({DirectorEvent::WAVE_CLEARED, wave});
                    if (fast) { ++wave; beginWave(); }
                    else      { phase = Phase::BREAK; timer = BREAK_TIME; }
                } else if (arena + 1 < (int)level->arenas.size()) {
                    events.push_back({DirectorEvent::ARENA_CLEARED, arena});
                    if (fast) approach(arena + 1);
                    else      phase = Phase::CLEARED;
                } else {
                    phase = Phase::VICTORY;
                    events.push_back({DirectorEvent::ARENA_CLEARED, arena});
                    events.push_back({fast ? DirectorEvent::FINISH_OPEN : DirectorEvent::VICTORY, arena});
                }
            }
            break;
        }
        case Phase::BREAK:
            if (!hold) timer -= dt;
            if (timer <= 0.f) { ++wave; beginWave(); }
            break;
        case Phase::CLEARED: {
            // Start the next arena once the player is a few metres inside it,
            // well clear of the gate that is about to close behind them.
            const Arena& next = level->arenas[arena + 1];
            if (level->arenaAt(playerPos) == arena + 1 && playerPos.z < next.zone.max.z - 3.5f)
                startArena(arena + 1);
            break;
        }
        case Phase::APPROACH: {
            const Arena& a = current();
            if (!a.hasTrigger) { startArena(arena); break; }
            const AABB& t = a.trigger;
            if (playerPos.x >= t.min.x && playerPos.x <= t.max.x && playerPos.y >= t.min.y && playerPos.y <= t.max.y &&
                playerPos.z >= t.min.z && playerPos.z <= t.max.z)
                startArena(arena);
            break;
        }
        case Phase::VICTORY: break;
        }
    }

    // Tests: where the next spawn of type t would go
    glm::vec3 pickSpawnForTest(EnemyType t, glm::vec3 player) { return pickSpawn(t, player); }

    // Debug/dev: put the director straight into a running arena.
    void skipIntro() { if (phase == Phase::INTRO) timer = 0.f; }

private:
    float spawnTimer = 0.f;
    int   lastSpawn  = -1;

    std::vector<SpawnRequest> fixedOut;   // spawned on the next update, outside the cap

    void beginWave() {
        phase = Phase::ACTIVE;
        spawnTimer = 0.f;
        goalDone = false; holding = false;
        const WaveGoal& g = goal();
        goalTimer = g.kind == WaveGoal::SURVIVE ? g.seconds : 0.f;
        conduitAlive.assign(g.kind == WaveGoal::CONDUITS ? g.points.size() : 0, true);
        fixedOut.clear();
        for (auto& p : g.kind == WaveGoal::CONDUITS ? g.points : std::vector<glm::vec3>{})
            fixedOut.push_back({EnemyType::CONDUIT, p});
        buildQueue(true);
        events.push_back({bossWave() ? DirectorEvent::BOSS_START : DirectorEvent::WAVE_START, wave});
        auto introduce = [&](EnemyType t) {
            if (seen[(int)t]) return;
            seen[(int)t] = true;
            events.push_back({DirectorEvent::NEW_TYPE, (int)t});
        };
        if (!conduitAlive.empty()) introduce(EnemyType::CONDUIT);
        for (auto& e : current().waves[wave]) {
            introduce(e.type);
            if (e.variant != Hollow::NONE && !seenHollow[(int)e.type][(int)e.variant]) {
                seenHollow[(int)e.type][(int)e.variant] = true;
                events.push_back({DirectorEvent::NEW_TYPE, (int)e.type | ((int)e.variant << 8)});
            }
            for (EnemyType t : e.escort) introduce(t);
        }
    }

    // Queue the wave's entries, interleaved (round-robin) so a mixed wave
    // arrives mixed. A refill (a goal wave that keeps coming) leaves out bosses.
    void buildQueue(bool first) {
        queue.clear();
        const auto& entries = current().waves[wave];
        bool any = true;
        for (int round = 0; any; ++round) {
            any = false;
            for (auto& e : entries)
                if (round < countOf(e) && (first || !isBoss(e.type))) {
                    bool fixed = !e.at.empty();
                    queue.push_back({e.type, fixed, fixed ? e.at[round] : glm::vec3{0.f}, e.escort, e.variant});
                    any = true;
                }
        }
    }

    void updateGoal(float dt, glm::vec3 playerPos) {
        const WaveGoal& g = goal();
        bool met = false;
        switch (g.kind) {
            case WaveGoal::HOLD:
                holding = inHoldZone(playerPos);
                if (holding && !zoneContested) goalTimer += dt;
                met = goalTimer >= g.seconds;
                break;
            case WaveGoal::SURVIVE:  goalTimer -= dt; met = goalTimer <= 0.f; break;
            case WaveGoal::CONDUITS: met = !conduitAlive.empty() && conduitsLeft() == 0; break;
            default: break;
        }
        if (met) {
            goalDone = true; holding = false;
            queue.clear();
            events.push_back({DirectorEvent::GOAL_DONE, wave});
        }
    }

    // Spawn a queued leader and its escort; returns how many
    int emitSquad(const Queued& q, glm::vec3 playerPos, std::vector<SpawnRequest>& out) {
        glm::vec3 at = q.fixed ? q.pos : pickSpawn(q.type, playerPos);
        out.push_back({q.type, at, q.variant});
        // The escort stands behind the leader (away from the player), in a fan
        glm::vec2 back{at.x - playerPos.x, at.z - playerPos.z};
        back = glm::length(back) > 0.01f ? glm::normalize(back) : glm::vec2{0.f, -1.f};
        glm::vec2 side{-back.y, back.x};
        for (int k = 0; k < (int)q.escort.size(); ++k) {
            EnemyType t = q.escort[k];
            if (statsOf(t).flying) { out.push_back({t, pickSpawn(t, playerPos)}); continue; }
            float lateral = (k % 2 ? -1.f : 1.f) * (1.6f + 1.2f * (k / 2));
            glm::vec2 o = back * 2.2f + side * lateral;
            glm::vec3 p = at + glm::vec3{o.x, 0.f, o.y};
            out.push_back({t, clearOfWalls(p) ? p : at});
        }
        return 1 + (int)q.escort.size();
    }

    // Is there room to stand at p (no wall through the body)?
    bool clearOfWalls(glm::vec3 p) const {
        for (auto& w : level->walls) {
            const AABB& b = w.box;
            if (p.x > b.min.x - 0.6f && p.x < b.max.x + 0.6f && p.z > b.min.z - 0.6f && p.z < b.max.z + 0.6f &&
                p.y + 0.2f < b.max.y && p.y + 1.8f > b.min.y) return false;
        }
        return level->arenaAt(p) == arena;
    }

    glm::vec3 pickSpawn(EnemyType t, glm::vec3 player) {
        const Arena& a = current();
        if (t == EnemyType::SOVEREIGN) return a.bossSpawn;   // at the far end of the Sanctum, waiting
        if (t == EnemyType::WARDEN) {   // beside the reactor, on the side away from the player
            glm::vec3 s = a.bossSpawn;
            if ((player.x > 0.f) == (s.x > 0.f)) s.x = -s.x;
            return s;
        }
        bool flying = statsOf(t).flying;
        // A conduit wave comes out of its standing conduits (not one right next to you)
        if (!flying && !conduitAlive.empty()) {
            const auto& cp = goal().points;
            int n = (int)conduitAlive.size(), start = rand() % n;
            for (int k = 0; k < n; ++k) {
                int i = (start + k) % n;
                if (!conduitAlive[i] || glm::length(glm::vec2{cp[i].x - player.x, cp[i].z - player.z}) < SAFE_RADIUS) continue;
                float ang = (rand() % 628) / 100.f;
                glm::vec3 p = cp[i] + glm::vec3{std::cos(ang) * 2.2f, 0.f, std::sin(ang) * 2.2f};
                return clearOfWalls(p) ? p : cp[i];   // on top of it: separation pushes it clear
            }
        }
        const auto& pts = flying ? a.airSpawns
                        : (wave < (int)a.waveGround.size() && !a.waveGround[wave].empty()) ? a.waveGround[wave]
                        : a.groundSpawns;
        // Under deep water (a flood): not used, unless every point is
        auto wet = [&](const glm::vec3& p) { return !flying && level->waterDepthAt(p) > DRY_DEPTH; };
        int n = (int)pts.size();
        bool anyDry = false;
        for (auto& p : pts) anyDry |= !wet(p);
        // Random point beyond the safe radius, not the one used last time
        int start = rand() % n;
        for (int k = 0; k < n; ++k) {
            int i = (start + k) % n;
            if (anyDry && wet(pts[i])) continue;
            glm::vec2 d{pts[i].x - player.x, pts[i].z - player.z};
            if (glm::length(d) >= SAFE_RADIUS && i != lastSpawn) { lastSpawn = i; return pts[i]; }
        }
        // Everything is close (player standing in the middle of the spawns): farthest wins
        int best = 0; float bestD = -1.f;
        for (int i = 0; i < n; ++i) {
            if (anyDry && wet(pts[i])) continue;
            float d = glm::length(glm::vec2{pts[i].x - player.x, pts[i].z - player.z});
            if (d > bestD) { bestD = d; best = i; }
        }
        lastSpawn = best;
        return pts[best];
    }
};
