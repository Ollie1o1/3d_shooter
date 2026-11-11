#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
// Film strips of a boss in motion, drawn on the CPU from the game's own rigs, meshes and springs (no
// graphics context): a row of frames a fixed time apart, the camera fixed in the world over a ruled
// floor (a line every half metre), so a foot that slides or a pose that snaps shows from one frame
// to the next. `make boss-strips` writes build/boss-strips/*.ppm (and .jpg if ffmpeg is installed).
//   tools/boss_strip [only]     only: a substring of the strip names to draw
#include "core/Balance.h"
#include "render/EnemyModel.h"
#include <fstream>
#include <functional>
#include <string>
using glm::vec3; using glm::vec4;

struct Canvas {
    int W, H; std::vector<vec3> px; std::vector<float> depth;
    Canvas(int w, int h) : W(w), H(h), px((size_t)w * h, vec3{.035f, .045f, .065f}), depth((size_t)w * h, 1e20f) {}
};
// One frame into the canvas' cell (x0, cell wide): the enemy as the game builds it, seen from `eye`
static void drawFrame(Canvas& c, int x0, int cell, const Enemy& e, vec3 eye, vec3 target, float zoom) {
    std::vector<BoxInstance> parts; rig::buildEnemy(e, e.age, parts);
    const auto V = glm::lookAt(eye, target, vec3{0, 1, 0});
    auto project = [&](vec3 p) { vec3 v = vec3(V * vec4(p, 1)); return vec3{x0 + cell * .5f + v.x * zoom, c.H * .62f - v.y * zoom, -v.z}; };
    auto edge = [](vec3 a, vec3 b, vec3 p) { return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x); };
    auto tri = [&](vec3 a, vec3 b, vec3 cc, vec3 col, vec3 emi, bool flat) {
        vec3 n = glm::cross(b - a, cc - a); if (glm::length(n) < 1e-8f) return; n = glm::normalize(n);
        vec3 color = flat ? col : col * (.3f + .7f * std::fabs(glm::dot(n, glm::normalize(vec3{-.5f, 1, .7f})))) + emi * .28f;
        color = glm::clamp(color, vec3{0}, vec3{1});
        a = project(a); b = project(b); cc = project(cc); float area = edge(a, b, cc); if (std::fabs(area) < .0001f) return;
        int xa = std::max(x0, (int)std::floor(std::min({a.x, b.x, cc.x}))), xb = std::min(x0 + cell - 1, (int)std::ceil(std::max({a.x, b.x, cc.x})));
        int ya = std::max(0, (int)std::floor(std::min({a.y, b.y, cc.y}))), yb = std::min(c.H - 1, (int)std::ceil(std::max({a.y, b.y, cc.y})));
        for (int y = ya; y <= yb; ++y) for (int x = xa; x <= xb; ++x) {
            vec3 p{x + .5f, y + .5f, 0};
            float u = edge(b, cc, p) / area, v = edge(cc, a, p) / area, w = 1 - u - v; if (u < 0 || v < 0 || w < 0) continue;
            float z = u * a.z + v * b.z + w * cc.z; size_t k = (size_t)y * c.W + x; if (z < c.depth[k]) { c.depth[k] = z; c.px[k] = color; }
        }
    };
    auto quad = [&](vec3 a, vec3 b, vec3 cc, vec3 d, vec3 col) { tri(a, b, cc, col, vec3{0}, true); tri(a, cc, d, col, vec3{0}, true); };
    const float fy = e.floorY;
    quad({-14, fy - .02f, -14}, {14, fy - .02f, -14}, {14, fy - .02f, 14}, {-14, fy - .02f, 14}, {.075f, .09f, .115f});
    for (int i = -28; i <= 28; ++i) {   // the ruled floor: a line every half metre, a brighter one every two
        const float p = i * .5f, w = .012f; const vec3 col = i % 4 == 0 ? vec3{.3f, .34f, .4f} : vec3{.15f, .17f, .2f};
        quad({p - w, fy - .01f, -14}, {p + w, fy - .01f, -14}, {p + w, fy - .01f, 14}, {p - w, fy - .01f, 14}, col);
        quad({-14, fy - .01f, p - w}, {14, fy - .01f, p - w}, {14, fy - .01f, p + w}, {-14, fy - .01f, p + w}, col);
    }
    const vec3 cube[] = {{-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, .5f, -.5f}, {-.5f, .5f, -.5f}, {-.5f, -.5f, .5f}, {.5f, -.5f, .5f}, {.5f, .5f, .5f}, {-.5f, .5f, .5f}};
    const int ids[] = {0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 4, 5, 0, 5, 1, 3, 2, 6, 3, 6, 7, 0, 3, 7, 0, 7, 4, 1, 5, 6, 1, 6, 2};
    for (auto& p : parts) {
        auto tf = [&](vec3 v) { return vec3(p.model * vec4(v, 1)); };
        if (const auto* m = meshlib::get((int)p.mesh)) {
            for (size_t k = 0; k + 2 < m->idx.size(); k += 3) tri(tf(m->pos[m->idx[k]]), tf(m->pos[m->idx[k + 1]]), tf(m->pos[m->idx[k + 2]]), p.color, p.emissive, false);
        } else for (int k = 0; k < 36; k += 3) tri(tf(cube[ids[k]]), tf(cube[ids[k + 1]]), tf(cube[ids[k + 2]]), p.color, p.emissive, false);
    }
    for (int y = 0; y < c.H; ++y) c.px[(size_t)y * c.W + x0] = vec3{.2f, .22f, .26f};   // (the frame's edge)
}
static void save(const Canvas& c, const std::string& path) {
    std::ofstream f(path, std::ios::binary); f << "P6\n" << c.W << " " << c.H << "\n255\n";
    for (auto v : c.px) for (int k = 0; k < 3; ++k) { unsigned char b = (unsigned char)(std::pow(v[k], 1.f / 2.2f) * 255); f.write((char*)&b, 1); }
}

// A strip: `frames` cells, `every` seconds apart from `from`; step(e, dt) advances the boss one 1/120 s tick
struct Strip {
    std::string name; int frames; float from, every;
    std::function<void(Enemy&)> start;
    std::function<void(Enemy&, float t, float dt)> step;
    vec3 eye{5.2f, 2.2f, 4.4f}; float zoomPerHeight = 330.f; bool follow = true;
};
static const char* g_only = nullptr;
static void run(const Strip& s, EnemyType type) {
    if (g_only && s.name.find(g_only) == std::string::npos) return;
    const int cell = 400, H = 460;
    Canvas c(cell * s.frames, H);
    Enemy e(type, {0, 0, 0}); e.state = EnemyState::ACTIVE; e.spawnTimer = 0;
    if (s.start) s.start(e);
    const float dt = 1.f / 120;
    int shot = 0; const vec3 origin = e.position;
    for (int i = 0; shot < s.frames && i < 120 * 60; ++i) {
        const float t = i * dt;
        s.step(e, t, dt);
        if (t + 1e-4f >= s.from + shot * s.every) {
            const float h = e.height();
            const vec3 at = (s.follow ? e.position : origin) + vec3{0, h * .42f, 0};
            drawFrame(c, shot * cell, cell, e, at + s.eye * (h / 2.6f), at, s.zoomPerHeight / h);
            ++shot;
        }
    }
    save(c, "build/boss-strips/" + s.name + ".ppm");
    std::printf("build/boss-strips/%s.ppm\n", s.name.c_str());
}

// ---- the Absolute and MORNINGSTAR ----------------------------------------------------------------
static void absStart(Enemy& e, bool morning, bool blade) {
    e.abArrive = 0; e.abAlt = e.abAltWant = 0; e.abBlade = blade;
    if (morning) { e.abForm = 1; e.scale = 1.65f; }
}
// One blow from his guard: a second of guard, the tell, the blow, the follow-through
static Strip absBlow(const std::string& name, AttackKind a, float tell, bool morning, bool blade) {
    Strip s; s.name = name; s.frames = 6; s.from = 1.f - 0.02f; s.every = (tell + 0.45f) / 5.f;
    s.start = [=](Enemy& e) { absStart(e, morning, blade); };
    s.step = [=](Enemy& e, float t, float dt) {
        e.age = t;
        const float into = t - 1.f;
        if (into >= 0.f && into < tell) { e.attack = a; e.telegraphDuration = tell; e.telegraphTimer = tell - into; }
        else { e.telegraphTimer = 0.f; if (into >= tell) e.attack = AttackKind::NONE; }
        rig::tickAbsoluteBody(e, dt);
    };
    return s;
}
static Strip absWalk(const std::string& name, float pace, bool morning) {
    Strip s; s.name = name; s.frames = 6; s.from = 1.5f; s.every = 0.14f; s.follow = false; s.eye = {6.5f, 2.0f, 0.6f};
    s.start = [=](Enemy& e) { absStart(e, morning, false); e.yaw = 0.f; };
    s.step = [=](Enemy& e, float t, float dt) {
        e.age = t;
        const float v = pace * e.scale, z = v * (t - 1.5f) - 0.3f * e.scale;
        const glm::vec3 was = e.position; e.position.z = z;
        // (the game's abGaitTick, which is private to his mind: the run cycle's phase from the floor covered)
        const float dist = std::fabs(e.position.z - was.z);
        e.abGait += (dist / dt - e.abGait) * (1.f - std::exp(-8.f * dt));
        e.animPhase += dist / (glm::clamp(0.75f + 0.15f * std::fabs(e.abGait), 0.75f, 2.4f) * e.scale) * 3.1415927f;
        rig::tickAbsoluteBody(e, dt);
    };
    return s;
}
static Strip absGuard(const std::string& name, bool morning, bool blade) {
    Strip s; s.name = name; s.frames = 6; s.from = 1.f; s.every = 0.4f;
    s.start = [=](Enemy& e) { absStart(e, morning, blade); };
    s.step = [](Enemy& e, float t, float dt) { e.age = t; rig::tickAbsoluteBody(e, dt); };
    return s;
}

// A shot taken in the guard, from in front and to his right: the recoil and the settle
static Strip absHit(const std::string& name, bool morning) {
    Strip s; s.name = name; s.frames = 6; s.from = 0.98f; s.every = morning ? 0.14f : 0.09f;
    s.start = [=](Enemy& e) { absStart(e, morning, false); };
    s.step = [](Enemy& e, float t, float dt) {
        e.age = t;
        if (std::fabs(t - 1.f) < dt * 0.5f) rig::absoluteHit(e, glm::vec3{0.5f, 0.f, -1.f}, 0.9f);
        rig::tickAbsoluteBody(e, dt);
    };
    return s;
}

// ---- the Successor ---------------------------------------------------------------------------------
static Strip suGuard(const std::string& name) {
    Strip s; s.name = name; s.frames = 6; s.from = 1.f; s.every = 0.6f;
    s.start = [](Enemy& e) { e.suPhase = 2; };
    s.step = [](Enemy& e, float t, float dt) { e.age = t; rig::tickSuccessorBody(e, dt); };
    return s;
}
static Strip suBlow(const std::string& name, AttackKind a, float tell) {
    Strip s; s.name = name; s.frames = 6; s.from = 0.98f; s.every = (tell + 0.5f) / 5.f;
    s.start = [](Enemy& e) { e.suPhase = 2; };
    s.step = [=](Enemy& e, float t, float dt) {
        e.age = t; const float into = t - 1.f;
        if (into >= 0.f && into < tell) { e.attack = a; e.telegraphDuration = tell; e.telegraphTimer = tell - into; }
        else { e.telegraphTimer = 0.f; if (into >= tell) { e.attack = AttackKind::NONE; e.suLanded = a; e.suSince = into - tell; } }
        rig::tickSuccessorBody(e, dt);
    };
    return s;
}
static Strip suHit(const std::string& name) {
    Strip s; s.name = name; s.frames = 6; s.from = 0.98f; s.every = 0.09f;
    s.start = [](Enemy& e) { e.suPhase = 2; };
    s.step = [](Enemy& e, float t, float dt) {
        e.age = t;
        if (std::fabs(t - 1.f) < dt * 0.5f) rig::successorHit(e, glm::vec3{0.5f, 0.f, -1.f}, 0.9f);
        rig::tickSuccessorBody(e, dt);
    };
    return s;
}

// ---- the Sovereign ---------------------------------------------------------------------------------
static Strip sovGuard(const std::string& name) {
    Strip s; s.name = name; s.frames = 6; s.from = 1.f; s.every = 0.7f;
    s.step = [](Enemy& e, float t, float dt) { e.age = t; rig::tickSovereignBody(e, dt); };
    return s;
}
static Strip sovBlow(const std::string& name, AttackKind a, float tell) {
    Strip s; s.name = name; s.frames = 6; s.from = 0.98f; s.every = (tell + 0.6f) / 5.f;
    s.step = [=](Enemy& e, float t, float dt) {
        e.age = t; const float into = t - 1.f;
        if (into >= 0.f && into < tell) { e.attack = a; e.telegraphDuration = tell; e.telegraphTimer = tell - into; }
        else {
            e.telegraphTimer = 0.f;
            if (into >= tell) { e.attack = AttackKind::NONE; e.lastSwing = a; e.swingTimer = std::max(0.f, Enemy::SWING_TIME - (into - tell)); }
        }
        rig::tickSovereignBody(e, dt);
    };
    return s;
}
static Strip sovHit(const std::string& name) {
    Strip s; s.name = name; s.frames = 6; s.from = 0.98f; s.every = 0.12f;
    s.step = [](Enemy& e, float t, float dt) {
        e.age = t;
        if (std::fabs(t - 1.f) < dt * 0.5f) rig::sovereignHit(e, glm::vec3{0.5f, 0.f, -1.f}, 1.f);
        rig::tickSovereignBody(e, dt);
    };
    return s;
}

int main(int argc, char** argv) {
    if (argc > 1) g_only = argv[1];
    balanceInit();
    const EnemyType A = EnemyType::ABSOLUTE;
    run(absGuard("absolute-guard", false, false), A);
    run(absGuard("absolute-guard-blade", false, true), A);
    run(absWalk("absolute-walk", 1.2f, false), A);
    run(absWalk("absolute-shuffle", 0.5f, false), A);
    run(absBlow("absolute-cross", AttackKind::ACROSS, 0.45f, false, false), A);
    run(absBlow("absolute-hook", AttackKind::AHOOK, 0.45f, false, false), A);
    run(absBlow("absolute-roundhouse", AttackKind::AROUNDHOUSE, 0.55f, false, false), A);
    run(absBlow("absolute-sweep-blade", AttackKind::ASWEEP, 0.5f, false, true), A);
    run(absBlow("absolute-haymaker", AttackKind::AHAYMAKER, 0.6f, false, false), A);
    run(absHit("absolute-hit", false), A);
    run(absHit("morningstar-hit", true), A);
    run(absGuard("morningstar-guard", true, false), A);
    run(absWalk("morningstar-walk", 1.0f, true), A);
    run(absBlow("morningstar-sunder", AttackKind::MSUNDER, 0.9f, true, false), A);
    run(absBlow("morningstar-backhand", AttackKind::MBACKHAND, 0.8f, true, false), A);
    run(absBlow("morningstar-grasp", AttackKind::MGRASP, 0.9f, true, false), A);
    const EnemyType SU = EnemyType::SUCCESSOR;
    run(suGuard("successor-guard"), SU);
    run(suBlow("successor-incision", AttackKind::SU_INCISION, 0.5f), SU);
    run(suBlow("successor-sever", AttackKind::SU_SEVER, 0.8f), SU);
    run(suBlow("successor-cascade", AttackKind::SU_CASCADE, 0.8f), SU);
    run(suHit("successor-hit"), SU);
    const EnemyType SV = EnemyType::SOVEREIGN;
    run(sovGuard("sovereign-guard"), SV);
    run(sovBlow("sovereign-sweep", AttackKind::SWEEP, 0.55f), SV);
    run(sovBlow("sovereign-cleave", AttackKind::CLEAVE, 0.6f), SV);
    run(sovBlow("sovereign-thrust", AttackKind::THRUST, 0.55f), SV);
    run(sovHit("sovereign-hit"), SV);
    return 0;
}
