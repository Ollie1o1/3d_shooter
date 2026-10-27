// enemy_export: write an enemy's CODE rig out as a file (assets/enemies/<name>.glb), the
// starting point for working on it in Blender. Build and run from the repo root:
//
//   $(CXX) $(CXXFLAGS) tools/enemy_export.cpp -o tools/enemy_export && tools/enemy_export ripper
//
// It draws the rig in many states (walking, winding up, diving...) with the box trace on
// (rig::traceSink), hangs each box on the joint whose frame it was drawn from (the frames
// filePoseOf gives), and works out each box's colour (fixed, or a shade of its glow) and
// glow channel (the one its emission follows in every state). Any box it can't explain is
// an error, not a guess. The test suite then holds the file to the code, box for box.
#include "render/EnemyModel.h"
#include <array>
#include <cstdio>
#include <string>
using namespace rig;
using namespace enemyfile;

struct State { Enemy e; float time; };
static std::vector<State> states(EnemyType t) {
    const EnemyStats& st = statsOf(t);
    std::vector<State> v;
    auto base = [&]() { Enemy e(t, {0.f, 0.f, 0.f}); e.spawnTimer = 0.f; e.yaw = 0.f; e.animPhase = 0.f; e.moveSpeed = 0.f; e.age = 0.f; return e; };
    v.push_back({base(), 0.f});   // [0]: at rest (its joints are matched here)
    AttackKind kinds[2] = {AttackKind::NONE, AttackKind::NONE};
    switch (t) {
        case EnemyType::HUSK: kinds[0] = AttackKind::SHOT; break;
        case EnemyType::SENTINEL: kinds[0] = AttackKind::BURST; break;
        case EnemyType::BRUTE: kinds[0] = AttackKind::SLAM; kinds[1] = AttackKind::LOB; break;
        case EnemyType::RIPPER: kinds[0] = AttackKind::LUNGE; break;
        case EnemyType::RAPTOR: kinds[0] = AttackKind::DIVE; break;
        case EnemyType::MITE: kinds[0] = AttackKind::FUSE; break;
        case EnemyType::JUGGERNAUT: kinds[0] = AttackKind::SHELL; kinds[1] = AttackKind::SMASH; break;
        case EnemyType::SHIELDBEARER: kinds[0] = AttackKind::SHOT; kinds[1] = AttackKind::BASH; break;
        default: break;
    }
    int n = 0;
    for (float ph : {0.7f, 2.2f, 3.9f})
        for (float spd : {0.f, st.speed * 0.7f})
            for (int a = 0; a < 3; ++a)
                for (float left : {0.6f, 0.05f}) {
                    State s{base(), 0.11f + 0.37f * (float)(n++)};
                    s.e.animPhase = ph; s.e.moveSpeed = spd; s.e.age = 0.03f * (float)n;
                    AttackKind k = a == 0 ? AttackKind::NONE : kinds[a - 1];
                    if (k != AttackKind::NONE) { s.e.attack = k; s.e.telegraphDuration = 0.8f; s.e.telegraphTimer = left; }
                    if (t == EnemyType::RAPTOR && a == 2) s.e.diveTimer = 0.5f;
                    if (t == EnemyType::SENTINEL && a == 2) s.e.burstLeft = 2;
                    v.push_back(s);
                }
    for (float ph : {0.4f, 2.9f}) {   // broken (parried): the JUGGERNAUT slumps, the SHIELDBEARER's shield is knocked aside
        State s{base(), 0.23f + 0.41f * (float)(n++)};
        s.e.animPhase = ph; s.e.staggerTimer = 1.f;
        v.push_back(s);
    }
    return v;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: enemy_export <husk|sentinel|brute|ripper|raptor|mite> [out.glb]\n"); return 2; }
    std::string name = argv[1];
    EnemyType t = EnemyType::COUNT;
    for (int k = 0; k < (int)EnemyType::COUNT; ++k) if (fileName((EnemyType)k) && name == fileName((EnemyType)k)) t = (EnemyType)k;
    if (t == EnemyType::COUNT) { std::fprintf(stderr, "no file rig called '%s'\n", name.c_str()); return 2; }
    std::string out = argc > 2 ? argv[2] : path(t);
    const EnemyStats& st = statsOf(t);

    model(t);           // (marks it tried) ...
    slot(t) = {};       // ... and empties it: buildEnemy draws the code rig

    gunkit::GunModel jm; jm.joints = restJointsOf(t);   // (just the joints: drawFile gives their frames)

    auto S = states(t);
    std::vector<std::vector<TracedBox>> traces;
    std::vector<FilePose> poses; std::vector<vec3> glows;
    std::vector<std::array<mat4, E_COUNT>> Ws;   // every joint's frame, in every state
    for (size_t si = 0; si < S.size(); ++si) {
        std::vector<TracedBox> tr; std::vector<BoxInstance> sink;
        traceSink() = &tr;
        buildEnemy(S[si].e, S[si].time, sink);
        traceSink() = nullptr;
        mat4 fr; FilePose P; vec3 g;
        if (!filePoseOf(S[si].e, S[si].time, mat4(1.f), fr, P, g)) { std::fprintf(stderr, "no file pose for %s\n", name.c_str()); return 1; }
        std::array<mat4, E_COUNT> W; { Rig dummy{sink}; drawFile(dummy, fr, jm, P, g, W.data()); }
        Ws.push_back(W);
        if (si && tr.size() != traces[0].size()) { std::fprintf(stderr, "state %zu draws %zu boxes, not %zu\n", si, tr.size(), traces[0].size()); return 1; }
        traces.push_back(tr); poses.push_back(P); glows.push_back(g);
    }

    gunkit::GunModel m; m.joints = jm.joints;
    auto close = [](vec3 a, vec3 b) { return glm::length(a - b) < 1e-4f * (1.f + glm::length(b)); };
    for (size_t i = 0; i < traces[0].size(); ++i) {
        const TracedBox& b0 = traces[0][i];
        gunkit::ModelPart p;
        // its joint: the one it keeps the same offset from in every state (usually none at all:
        // drawn from the joint's own frame; a JUGGERNAUT's cannon rings are turned on its arm)
        int joint = -1; mat4 off(1.f);
        for (int j = E_COUNT - 1; j >= 0 && joint < 0; --j) {
            if (j != E_ROOT && !jm.has(j)) continue;
            mat4 R0 = glm::inverse(Ws[0][j]) * b0.parent;
            bool rigid = true;
            for (size_t s = 1; s < traces.size() && rigid; ++s) {
                mat4 R = glm::inverse(Ws[s][j]) * traces[s][i].parent;
                for (int c = 0; c < 4 && rigid; ++c) rigid = glm::length(R[c] - R0[c]) < 1e-4f;
            }
            if (rigid) { joint = j; off = R0; }
        }
        if (joint < 0) { std::fprintf(stderr, "box %zu: rides no joint\n", i); return 1; }
        p.joint = joint; p.rel = off * b0.local;
        // its colour: fixed, or a shade of the (flaring) glow
        bool fixed = true;
        for (auto& tr : traces) fixed &= close(tr[i].col, b0.col);
        if (fixed) p.color = b0.col;
        else {
            float g = b0.col.x / std::max(1e-6f, glows[0].x);
            bool shade = true;
            for (size_t s = 0; s < traces.size(); ++s) shade &= close(traces[s][i].col, glows[s] * g);
            if (!shade) { std::fprintf(stderr, "box %zu: its colour follows nothing it knows\n", i); return 1; }
            p.glowScale = g;
        }
        // its glow: the channel its emission follows in every state (with one k)
        bool dark = true;
        for (auto& tr : traces) dark &= glm::length(tr[i].emi) < 1e-6f;
        if (!dark) {
            const int order[] = {C_GLOW, C_SHOT, C_PULSE, C_DIVE, C_FUSE, C_BLADE, C_SMASH, C_RIM, C_CONST};
            for (int c : order) {
                vec3 k{0.f}; bool have[3] = {false, false, false}, ok = true;
                for (size_t s = 0; s < traces.size() && ok; ++s)
                    for (int a = 0; a < 3 && ok; ++a) {
                        float ch = poses[s].chan[c][a], em = traces[s][i].emi[a];
                        if (std::fabs(ch) < 1e-6f) { ok = std::fabs(em) < 1e-5f; continue; }
                        if (!have[a]) { k[a] = em / ch; have[a] = true; }
                        ok = std::fabs(em - ch * k[a]) < 1e-4f * (1.f + std::fabs(em));
                    }
                if (ok) { p.chan = c; p.k = k; break; }
            }
            if (p.chan < 0) { std::fprintf(stderr, "box %zu: its glow follows no channel\n", i); return 1; }
            if (p.chan != C_CONST && std::fabs(p.k.x - p.k.y) < 1e-4f && std::fabs(p.k.y - p.k.z) < 1e-4f) p.k = vec3{p.k.x};
        }
        m.parts.push_back(p);
    }
    auto matName = [&](const gunkit::ModelPart& p) -> std::string {
        char b[64];
        if (p.glowScale >= 0.f) { std::snprintf(b, sizeof b, "glow*%g", std::round(p.glowScale * 1000.f) / 1000.f); return b; }
        float s = p.color.x / st.color.x;
        if (close(p.color, st.color * s)) {
            if (std::fabs(s - 1.f) < 1e-4f) return name;
            std::snprintf(b, sizeof b, "%s*%g", name.c_str(), std::round(s * 1000.f) / 1000.f); return b;
        }
        std::snprintf(b, sizeof b, "c%02x%02x%02x", (int)std::round(std::min(1.f, p.color.x) * 255.f),
                      (int)std::round(std::min(1.f, p.color.y) * 255.f), (int)std::round(std::min(1.f, p.color.z) * 255.f));
        return b;
    };
    if (!gunkit::writeGunFile(out, m, name, matName, names())) { std::fprintf(stderr, "can't write %s\n", out.c_str()); return 1; }
    std::printf("%s: %zu boxes on %zu joints -> %s\n", name.c_str(), m.parts.size(), m.joints.size(), out.c_str());
    return 0;
}
