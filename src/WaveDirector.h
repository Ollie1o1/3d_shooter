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
// The director only *requests* spawns and reports events (wave started, new
// enemy type seen, arena cleared…); GameplayState owns the enemies, banners
// and doors. That split is what lets tests/test_game.cpp drive a whole run.
// =============================================================================
#include "Level.h"
#include <vector>
#include <cstdlib>

struct SpawnRequest { EnemyType type; glm::vec3 pos; };

enum class DirectorEvent { ARENA_START, WAVE_START, BOSS_START, NEW_TYPE, WAVE_CLEARED, ARENA_CLEARED, VICTORY };
struct DirectorEventRec { DirectorEvent kind; int value; };

class WaveDirector {
public:
    enum class Phase { INTRO, ACTIVE, BREAK, CLEARED, VICTORY };

    static constexpr float INTRO_TIME  = 2.5f;
    static constexpr float BREAK_TIME  = 3.0f;
    static constexpr float SPAWN_GAP   = 0.45f;   // seconds between trickled spawns
    static constexpr float SAFE_RADIUS = 13.f;    // don't spawn closer than this to the player

    const LevelData* level = nullptr;
    int   arena = 0, wave = 0;
    Phase phase = Phase::INTRO;
    float timer = 0.f;
    std::vector<EnemyType>         queue;
    std::vector<DirectorEventRec>  events;      // drained by the caller every frame
    bool  seen[(int)EnemyType::COUNT] = {};

    void startArena(int a) {
        arena = a; wave = 0;
        phase = Phase::INTRO; timer = INTRO_TIME;
        queue.clear(); spawnTimer = 0.f;
        events.push_back({DirectorEvent::ARENA_START, a});
    }

    const Arena& current() const { return level->arenas[arena]; }
    int  waveCount() const       { return (int)current().waves.size(); }
    int  queued() const          { return (int)queue.size(); }
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
            if (!queue.empty() && aliveCount < current().maxAlive && spawnTimer <= 0.f) {
                EnemyType t = queue.front();
                queue.erase(queue.begin());
                out.push_back({t, pickSpawn(t, playerPos)});
                spawnTimer = SPAWN_GAP;
                ++aliveCount;   // it's on the field now; don't call the wave clear this tick
            }
            if (queue.empty() && aliveCount == 0) {
                if (wave + 1 < waveCount()) {
                    phase = Phase::BREAK; timer = BREAK_TIME;
                    events.push_back({DirectorEvent::WAVE_CLEARED, wave});
                } else if (arena + 1 < (int)level->arenas.size()) {
                    phase = Phase::CLEARED;
                    events.push_back({DirectorEvent::ARENA_CLEARED, arena});
                } else {
                    phase = Phase::VICTORY;
                    events.push_back({DirectorEvent::ARENA_CLEARED, arena});
                    events.push_back({DirectorEvent::VICTORY, arena});
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
        auto entries = current().waves[wave];
        bool any = true;
        for (int round = 0; any; ++round) {
            any = false;
            for (auto& e : entries)
                if (round < e.count) { queue.push_back(e.type); any = true; }
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
        const auto& pts = statsOf(t).flying ? a.airSpawns : a.groundSpawns;
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
