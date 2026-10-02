#pragma once
// =============================================================================
// Ghost.h — FAST mode's ghost: the route of your best run, recorded ten times
// a second and played back as a glowing runner, so you can see where you're
// gaining or losing time. Saved with the records (file on desktop,
// localStorage on the web). No OpenGL.
// =============================================================================
#include "Persist.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cmath>

struct GhostRun {
    static constexpr float STEP = 0.1f;         // seconds between samples
    static constexpr int   MAX_SAMPLES = 20000; // over half an hour; plenty
    static constexpr const char* kKey = "ghost.v2.cfg";
    std::vector<glm::vec4> pts;                 // feet x y z, yaw (degrees)

    bool  empty() const { return pts.size() < 2; }
    float duration() const { return pts.empty() ? 0.f : (pts.size() - 1) * STEP; }

    // Record: call every tick with the run clock; keeps one sample per STEP
    void record(float t, glm::vec3 feet, float yaw) {
        if ((int)pts.size() >= MAX_SAMPLES) return;
        while ((float)pts.size() * STEP <= t + 1e-4f) pts.push_back({feet, yaw});
    }

    // Where the ghost is at run time t (holds the last point after the end)
    bool at(float t, glm::vec3& feet, float& yaw) const {
        if (empty() || t < 0.f) return false;
        float f = t / STEP;
        int i = (int)f;
        if (i >= (int)pts.size() - 1) { feet = glm::vec3(pts.back()); yaw = pts.back().w; return true; }
        float u = f - (float)i;
        const glm::vec4 &a = pts[i], &b = pts[i + 1];
        feet = glm::mix(glm::vec3(a), glm::vec3(b), u);
        float dy = std::remainder(b.w - a.w, 360.f);
        yaw = a.w + dy * u;
        return true;
    }

    void save() const {
        std::ostringstream f;
        f << "ghost " << pts.size() << "\n";
        char buf[64];
        for (auto& p : pts) {
            std::snprintf(buf, sizeof(buf), "%.2f %.2f %.2f %.0f\n", p.x, p.y, p.z, p.w);
            f << buf;
        }
        persist::save(kKey, f.str());
    }
    void load() {
        pts.clear();
        std::istringstream f(persist::load(kKey));
        std::string key; int n = 0;
        if (!(f >> key >> n) || key != "ghost" || n < 0 || n > MAX_SAMPLES) return;
        pts.reserve(n);
        for (int i = 0; i < n; ++i) {
            glm::vec4 p;
            if (!(f >> p.x >> p.y >> p.z >> p.w)) { pts.clear(); return; }
            pts.push_back(p);
        }
    }
};
