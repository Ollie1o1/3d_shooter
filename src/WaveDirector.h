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
// The director only *requests* spawns and reports events (wave started, new
// enemy type seen, arena cleared…); GameplayState owns the enemies, banners
// and doors. That split is what lets tests/test_game.cpp drive a whole run.
// =============================================================================
#include "Level.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>

struct SpawnRequest { EnemyType type; glm::vec3 pos; };

enum class DirectorEvent { ARENA_START, WAVE_START, BOSS_START, NEW_TYPE, WAVE_CLEARED, ARENA_CLEARED, VICTORY, FINISH_OPEN };
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

    const LevelData* level = nullptr;
    bool  fast = false;
    // Difficulty (ARENA): waves are this many times bigger (the boss is still
    // one Warden) and this many more may be on the field at once
    float countScale    = 1.f;
    int   maxAliveBonus = 0;
    int   arena = 0, wave = 0;
    Phase phase = Phase::INTRO;
    float timer = 0.f;
    struct Queued { EnemyType type; bool fixed; glm::vec3 pos; };
    std::vector<Queued>            queue;
    std::vector<DirectorEventRec>  events;      // drained by the caller every frame
    bool  seen[(int)EnemyType::COUNT] = {};

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
    int  countOf(const WaveEntry& e) const {
        if (!e.at.empty() || e.type == EnemyType::WARDEN) return e.total();
        return std::max(1, (int)std::lround(e.count * countScale));
    }
    bool fighting() const        { return phase == Phase::ACTIVE; }
    bool bossWave() const {
        for (auto& e : current().waves[wave]) if (e.type == EnemyType::WARDEN) return true;
        return false;
    }

    // aliveCount: enemies currently on the field (spawning ones included).
    void update(float dt, int aliveCount, glm::vec3 playerPos, std::vector<SpawnRequest>& out) {
        switch (phase) {
        case Phase::INTRO:
            timer -= dt;
            if (timer <= 0.f) beginWave();
            break;
        case Phase::ACTIVE: {
            spawnTimer -= dt;
            if (fast) {
                for (auto& q : queue) out.push_back({q.type, q.fixed ? q.pos : pickSpawn(q.type, playerPos)});
                aliveCount += (int)queue.size();
                queue.clear();
            } else if (!queue.empty() && aliveCount < maxAlive() && spawnTimer <= 0.f) {
                Queued q = queue.front();
                queue.erase(queue.begin());
                out.push_back({q.type, q.fixed ? q.pos : pickSpawn(q.type, playerPos)});
                spawnTimer = SPAWN_GAP;
                ++aliveCount;   // it's on the field now; don't call the wave clear this tick
            }
            if (queue.empty() && aliveCount == 0) {
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
            timer -= dt;
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

    // Debug/dev: put the director straight into a running arena.
    void skipIntro() { if (phase == Phase::INTRO) timer = 0.f; }

private:
    float spawnTimer = 0.f;
    int   lastSpawn  = -1;

    void beginWave() {
        phase = Phase::ACTIVE;
        spawnTimer = 0.f;
        queue.clear();
        // Interleave the types (round-robin) so a mixed wave arrives mixed
        const auto& entries = current().waves[wave];
        bool any = true;
        for (int round = 0; any; ++round) {
            any = false;
            for (auto& e : entries)
                if (round < countOf(e)) {
                    bool fixed = !e.at.empty();
                    queue.push_back({e.type, fixed, fixed ? e.at[round] : glm::vec3{0.f}});
                    any = true;
                }
        }
        events.push_back({bossWave() ? DirectorEvent::BOSS_START : DirectorEvent::WAVE_START, wave});
        for (auto& e : entries)
            if (!seen[(int)e.type]) {
                seen[(int)e.type] = true;
                events.push_back({DirectorEvent::NEW_TYPE, (int)e.type});
            }
    }

    glm::vec3 pickSpawn(EnemyType t, glm::vec3 player) {
        const Arena& a = current();
        if (t == EnemyType::WARDEN) {   // beside the reactor, on the side away from the player
            glm::vec3 s = a.bossSpawn;
            if ((player.x > 0.f) == (s.x > 0.f)) s.x = -s.x;
            return s;
        }
        bool flying = statsOf(t).flying;
        const auto& pts = flying ? a.airSpawns
                        : (wave < (int)a.waveGround.size() && !a.waveGround[wave].empty()) ? a.waveGround[wave]
                        : a.groundSpawns;
        // Random point beyond the safe radius, not the one used last time
        int n = (int)pts.size();
        int start = rand() % n;
        for (int k = 0; k < n; ++k) {
            int i = (start + k) % n;
            glm::vec2 d{pts[i].x - player.x, pts[i].z - player.z};
            if (glm::length(d) >= SAFE_RADIUS && i != lastSpawn) { lastSpawn = i; return pts[i]; }
        }
        // Everything is close (player standing in the middle of the spawns): farthest wins
        int best = 0; float bestD = -1.f;
        for (int i = 0; i < n; ++i) {
            float d = glm::length(glm::vec2{pts[i].x - player.x, pts[i].z - player.z});
            if (d > bestD) { bestD = d; best = i; }
        }
        lastSpawn = best;
        return pts[best];
    }
};
