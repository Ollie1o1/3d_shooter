#pragma once
// =============================================================================
// RunLog.h — what happened in a stretch of play, so tuning can come from play
// and not from guesses. Each death, finished run or quit appends one JSON line
// to runs.jsonl beside the binary (desktop only; the web build keeps nothing):
// what hurt you and how much, what killed you, each gun's shots, hits and
// kills, how long each enemy type took to kill from its first hit, stuns,
// finishers and OVERDRIVE. STORY adds the chapter, where you were, the time
// spent in each section and the time spent lost (out of a fight and not getting
// any closer to the way on). tools/runlog.py sums the file up.
// No OpenGL; the counters are plain so the tests can check them.
// =============================================================================
#include "enemies/EnemyTypes.h"
#include "player/StyleSystem.h"
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <glm/glm.hpp>

struct RunLog {
    static constexpr int TYPES = (int)EnemyType::COUNT + 1;   // the last: anything else (hazards, blasts with no owner)
    float startAt = 0.f;                // elapsedTime the stretch began
    float dmgBy[TYPES] = {};            // damage taken from each
    int   hitsBy[TYPES] = {};
    int   killedBy = -1;                // what landed the last blow (-1: alive)
    int   shots[4] = {}, hits[4] = {};  // per gun
    int   killsBy[(int)StyleSource::COUNT] = {};
    float ttk[TYPES] = {};              // seconds from first hit to death, summed
    int   ttkN[TYPES] = {};
    int   kills = 0, stuns = 0, finishers = 0, overdrives = 0;
    float overdriveTime = 0.f, finisherHeal = 0.f;
    // STORY: where it happened, how long each section took, and where you were lost
    static constexpr int SECTIONS = 16;
    static constexpr float LOST_AFTER = 20.f;   // seconds out of a fight without getting 1 m nearer the way on
    int   chapter = 0;                  // 0: not STORY
    glm::vec3 at{0.f};                  // where you were when the line was written
    float sectionTime[SECTIONS] = {};
    float lostTime[SECTIONS] = {};
    glm::vec3 lostAt[4] = {};           // the first few places you were lost
    int   lostN = 0;
    int   lastSection = -1;
    float bestDist = 1e9f, stall = 0.f; // nearest you've been to the way on, and how long since you got nearer
    bool  lostNow = false;
    // Each tick of STORY play: the section you're in, how far its way on is, whether a fight is on
    void storyTick(float dt, int chapterNo, int section, glm::vec3 pos, float distToNext, bool fighting) {
        chapter = chapterNo; at = pos;
        if (section < 0 || section >= SECTIONS) return;
        sectionTime[section] += dt;
        if (section != lastSection) { lastSection = section; bestDist = distToNext; stall = 0.f; lostNow = false; }
        if (fighting || distToNext < bestDist - 1.f) { bestDist = std::min(bestDist, distToNext); stall = 0.f; lostNow = false; return; }
        stall += dt;
        if (stall < LOST_AFTER) return;
        if (!lostNow) { lostNow = true; lostTime[section] += LOST_AFTER; if (lostN < 4) lostAt[lostN++] = pos; }
        else lostTime[section] += dt;
    }
    bool  any() const {
        int took = 0; for (int s = 0; s < TYPES; ++s) took += hitsBy[s];
        return kills > 0 || took > 0 || shots[0] + shots[1] + shots[2] + shots[3] > 0;
    }

    static int slot(EnemyType t) { return t == EnemyType::COUNT ? TYPES - 1 : (int)t; }
    void took(EnemyType by, float dmg) { int s = slot(by); dmgBy[s] += dmg; ++hitsBy[s]; killedBy = s; }
    void killed(EnemyType t, StyleSource src, float ttkSec) {
        ++kills; ++killsBy[(int)src];
        if (ttkSec >= 0.f) { int s = slot(t); ttk[s] += ttkSec; ++ttkN[s]; }
    }
    void reset(float now) { *this = RunLog{}; startAt = now; }

    static const char* typeName(int s) { return s == TYPES - 1 ? "OTHER" : statsOf((EnemyType)s).name; }
    static const char* sourceName(int s) {
        static const char* N[] = {"NONE", "REVOLVER", "SHOTGUN", "LANCER", "LONGSHOT", "PUNCH", "PARRY", "EXPLOSIVE", "SLAM",
                                  "ENVIRONMENT", "FRIENDLY", "MELEE"};
        return s >= 0 && s < (int)(sizeof(N) / sizeof(N[0])) ? N[s] : "?";
    }

    // One JSON object on one line. how: "death" / "end" / "quit"
    std::string json(const char* how, const char* mode, int difficulty, const char* arena, int wave, float now) const {
        std::string o;
        char b[256];
        std::snprintf(b, sizeof(b), "{\"how\":\"%s\",\"mode\":\"%s\",\"difficulty\":%d,\"arena\":\"%s\",\"wave\":%d,\"seconds\":%.1f,\"killedBy\":\"%s\"",
                      how, mode, difficulty, arena, wave, now - startAt, killedBy >= 0 && std::strcmp(how, "death") == 0 ? typeName(killedBy) : "");
        o += b;
        o += ",\"damageTaken\":{";
        bool first = true;
        for (int s = 0; s < TYPES; ++s) if (hitsBy[s]) {
            std::snprintf(b, sizeof(b), "%s\"%s\":[%.0f,%d]", first ? "" : ",", typeName(s), dmgBy[s], hitsBy[s]); o += b; first = false;
        }
        o += "},\"guns\":{";
        static const char* G[4] = {"REVOLVER", "SHOTGUN", "LANCER", "LONGSHOT"};
        for (int g = 0; g < 4; ++g) {
            std::snprintf(b, sizeof(b), "%s\"%s\":[%d,%d,%d]", g ? "," : "", G[g], shots[g], hits[g], killsBy[(int)StyleSource::REVOLVER + g]); o += b;
        }
        o += "},\"kills\":{";
        first = true;
        for (int s = 0; s < (int)StyleSource::COUNT; ++s) if (killsBy[s]) {
            std::snprintf(b, sizeof(b), "%s\"%s\":%d", first ? "" : ",", sourceName(s), killsBy[s]); o += b; first = false;
        }
        o += "},\"ttk\":{";
        first = true;
        for (int s = 0; s < TYPES; ++s) if (ttkN[s]) {
            std::snprintf(b, sizeof(b), "%s\"%s\":[%.2f,%d]", first ? "" : ",", typeName(s), ttk[s] / ttkN[s], ttkN[s]); o += b; first = false;
        }
        std::snprintf(b, sizeof(b), "},\"stuns\":%d,\"finishers\":%d,\"finisherHeal\":%.0f,\"overdrives\":%d,\"overdriveSeconds\":%.1f",
                      stuns, finishers, finisherHeal, overdrives, overdriveTime);
        o += b;
        if (chapter > 0) {
            std::snprintf(b, sizeof(b), ",\"chapter\":%d,\"section\":%d,\"at\":[%.1f,%.1f,%.1f],\"sectionSeconds\":{", chapter, lastSection, at.x, at.y, at.z);
            o += b;
            first = true;
            for (int i = 0; i < SECTIONS; ++i) if (sectionTime[i] > 0.f) {
                std::snprintf(b, sizeof(b), "%s\"%d\":%.1f", first ? "" : ",", i, sectionTime[i]); o += b; first = false;
            }
            o += "},\"lostSeconds\":{";
            first = true;
            for (int i = 0; i < SECTIONS; ++i) if (lostTime[i] > 0.f) {
                std::snprintf(b, sizeof(b), "%s\"%d\":%.1f", first ? "" : ",", i, lostTime[i]); o += b; first = false;
            }
            o += "},\"lostAt\":[";
            for (int i = 0; i < lostN; ++i) { std::snprintf(b, sizeof(b), "%s[%.1f,%.1f,%.1f]", i ? "," : "", lostAt[i].x, lostAt[i].y, lostAt[i].z); o += b; }
            o += "]";
        }
        o += "}";
        return o;
    }

    static void append(const std::string& line) {
#ifndef __EMSCRIPTEN__
        if (FILE* f = std::fopen("runs.jsonl", "a")) { std::fprintf(f, "%s\n", line.c_str()); std::fclose(f); }
#else
        (void)line;
#endif
    }
};
