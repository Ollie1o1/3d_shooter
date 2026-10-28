#pragma once
// =============================================================================
// AnimClip.h — keyframed motion made in Blender (assets/enemies/anim/<name>.glb,
// built by tools/blender/anim_build.py): a rig's joints keyed over time, played
// over the code's own pose for a move (a boss's wind-up).
//
// The file holds the rig's joints (nodes named as its model's: "torso", "armR",
// "weapon"...) at rest and glTF animations on them: rotation, translation and
// scale keys, LINEAR or STEP (CUBICSPLINE is read through its values). A
// joint's motion is how far it is from its rest, in its own frame - the same
// thing the code sets (FilePose::motion) - so a clip and the code mix joint by
// joint. A node named "root" moves the whole body (a crouch, a lift).
// Each animation in the file is a clip, by its name. No OpenGL.
// =============================================================================
#include "render/GunFile.h"
#include <glm/gtc/quaternion.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#  define GLM_ENABLE_EXPERIMENTAL   // (glm's matrix_decompose lives in gtx)
#endif
#include <glm/gtx/matrix_decompose.hpp>
#include <map>
#include <string>
#include <vector>

namespace anim {

constexpr int ROOT_TRACK = -2;   // "root": the whole body

struct Track {
    int joint = -1;                  // a joint id (the rig's Names), or ROOT_TRACK
    glm::vec3 restT{0.f}, restS{1.f};
    glm::quat restR{1.f, 0.f, 0.f, 0.f};
    std::vector<float> tT, tR, tS;   // key times per channel
    std::vector<glm::vec3> T, S;
    std::vector<glm::quat> R;
    bool stepT = false, stepR = false, stepS = false;
};
struct Clip {
    std::string name;
    float length = 0.f;              // seconds (its last key)
    std::vector<Track> tracks;
    const Track* track(int joint) const { for (auto& t : tracks) if (t.joint == joint) return &t; return nullptr; }
};
struct ClipFile {
    std::vector<Clip> clips;
    std::string error;
    const Clip* clip(const std::string& n) const { for (auto& c : clips) if (c.name == n) return &c; return nullptr; }
};

template <class V, class Mix>
inline V keyAt(const std::vector<float>& t, const std::vector<V>& v, float time, bool step, V rest, Mix mix) {
    if (t.empty() || v.empty()) return rest;
    if (time <= t.front()) return v.front();
    if (time >= t.back()) return v.back();
    size_t k = (size_t)(std::upper_bound(t.begin(), t.end(), time) - t.begin());   // t[k-1] <= time < t[k]
    if (step) return v[k - 1];
    float u = (time - t[k - 1]) / std::max(1e-6f, t[k] - t[k - 1]);
    return mix(v[k - 1], v[k], u);
}
// A track's local transform at `time`, and its move from rest (in its own frame)
inline glm::mat4 localAt(const Track& tr, float time) {
    glm::vec3 t = keyAt(tr.tT, tr.T, time, tr.stepT, tr.restT, [](glm::vec3 a, glm::vec3 b, float u) { return glm::mix(a, b, u); });
    glm::vec3 s = keyAt(tr.tS, tr.S, time, tr.stepS, tr.restS, [](glm::vec3 a, glm::vec3 b, float u) { return glm::mix(a, b, u); });
    glm::quat q = keyAt(tr.tR, tr.R, time, tr.stepR, tr.restR, [](glm::quat a, glm::quat b, float u) { return glm::slerp(a, b, u); });
    return glm::translate(glm::mat4(1.f), t) * glm::mat4_cast(q) * glm::scale(glm::mat4(1.f), s);
}
inline glm::mat4 restLocal(const Track& tr) {
    return glm::translate(glm::mat4(1.f), tr.restT) * glm::mat4_cast(tr.restR) * glm::scale(glm::mat4(1.f), tr.restS);
}
inline glm::mat4 motionAt(const Track& tr, float time) { return glm::inverse(restLocal(tr)) * localAt(tr, time); }

// Mix two joint motions (rotation slerped, offset and scale lerped): w 0 is a, 1 is b
inline glm::mat4 blend(const glm::mat4& a, const glm::mat4& b, float w) {
    if (w <= 0.f) return a;
    if (w >= 1.f) return b;
    glm::vec3 sa, ta, sb, tb, skew; glm::vec4 persp; glm::quat qa, qb;
    glm::decompose(a, sa, qa, ta, skew, persp);
    glm::decompose(b, sb, qb, tb, skew, persp);
    return glm::translate(glm::mat4(1.f), glm::mix(ta, tb, w)) * glm::mat4_cast(glm::slerp(qa, qb, w)) *
           glm::scale(glm::mat4(1.f), glm::mix(sa, sb, w));
}

inline ClipFile readClips(const std::string& bytes, const std::string& path, const gunkit::Names& N) {
    using namespace gunkit;
    ClipFile F;
    auto bad = [&](const std::string& why) { F = ClipFile{}; F.error = path + ": " + why; return F; };
    std::string json, err;
    if (!gltfJson(bytes, json, err)) return bad(err);
    JVal doc;
    if (!parseJson(json, doc, err)) return bad(err);
    const std::string bin = glbBin(bytes);
    const auto& nodes = doc.arr("nodes");
    for (const JVal& A : doc.arr("animations")) {
        Clip c;
        c.name = A.str("name");
        const auto& samplers = A.arr("samplers");
        std::map<int, size_t> byNode;   // node -> its track
        for (const JVal& ch : A.arr("channels")) {
            const JVal* tg = ch.get("target");
            if (!tg) continue;
            int ni = tg->idx("node"), si = ch.idx("sampler");
            std::string pathName = tg->str("path");
            if (ni < 0 || ni >= (int)nodes.size() || si < 0 || si >= (int)samplers.size()) return bad("clip '" + c.name + "': a broken channel");
            std::string nn = nodes[ni].str("name");
            int joint = nn == "root" ? ROOT_TRACK : lookupN(N.joints, N.joint, nn);
            if (joint == ROOT_TRACK || joint > 0) {} else continue;   // (a node that isn't a joint: not ours)
            if (!byNode.count(ni)) {
                Track t; t.joint = joint;
                glm::mat4 m = nodeMatrix(nodes[ni]);
                glm::vec3 skew; glm::vec4 persp;
                glm::decompose(m, t.restS, t.restR, t.restT, skew, persp);
                // The node as written is wherever it was on the frame it was saved at: its true rest, if the
                // file says (anim_build.py: od_rest, od_rest_loc, in Blender's axes - Z up), is that instead
                if (const JVal* x = nodes[ni].get("extras")) {
                    const auto& q = x->arr("od_rest");
                    const auto& l = x->arr("od_rest_loc");
                    if (q.size() == 4) t.restR = glm::normalize(glm::quat((float)q[0].n, (float)q[1].n, (float)q[3].n, -(float)q[2].n));
                    if (l.size() == 3) t.restT = {(float)l[0].n, (float)l[2].n, -(float)l[1].n};
                }
                byNode[ni] = c.tracks.size();
                c.tracks.push_back(t);
            }
            Track& t = c.tracks[byNode[ni]];
            const JVal& S = samplers[si];
            std::vector<float> times, vals; int tc = 0, vc = 0;
            if (!accessorData(doc, bin, S.idx("input"), &times, nullptr, tc) || tc != 1 ||
                !accessorData(doc, bin, S.idx("output"), &vals, nullptr, vc))
                return bad("clip '" + c.name + "': keys on '" + nn + "' can't be read");
            const std::string interp = S.str("interpolation");
            const bool cubic = interp == "CUBICSPLINE", step = interp == "STEP";
            const size_t n = times.size(), per = cubic ? 3 : 1;
            if (vals.size() != n * per * (size_t)vc) return bad("clip '" + c.name + "': '" + nn + "' has the wrong number of values");
            auto at = [&](size_t k, int comp) { return vals[(k * per + (cubic ? 1 : 0)) * vc + comp]; };   // (a cubic key: its value, between its tangents)
            if (pathName == "translation" || pathName == "scale") {
                if (vc != 3) return bad("clip '" + c.name + "': '" + nn + "' " + pathName + " isn't 3 numbers");
                auto& dst = pathName == "translation" ? t.T : t.S;
                for (size_t k = 0; k < n; ++k) dst.push_back({at(k, 0), at(k, 1), at(k, 2)});
                (pathName == "translation" ? t.tT : t.tS) = times;
                (pathName == "translation" ? t.stepT : t.stepS) = step;
            } else if (pathName == "rotation") {
                if (vc != 4) return bad("clip '" + c.name + "': '" + nn + "' rotation isn't 4 numbers");
                for (size_t k = 0; k < n; ++k) {
                    glm::quat q(at(k, 3), at(k, 0), at(k, 1), at(k, 2));   // (glTF: x y z w)
                    if (!t.R.empty() && glm::dot(t.R.back(), q) < 0.f) q = -q;   // (the short way round)
                    t.R.push_back(glm::normalize(q));
                }
                t.tR = times; t.stepR = step;
            } else continue;   // (weights: not ours)
            if (!times.empty()) c.length = std::max(c.length, times.back());
        }
        if (!c.tracks.empty()) F.clips.push_back(std::move(c));
    }
    if (F.clips.empty()) return bad("no animations on its joints");
    return F;
}

// assets/enemies/anim/<file>.glb, loaded once (nullptr if there's no such file or it's broken: printed)
inline const ClipFile* file(const std::string& name, const gunkit::Names& N) {
    static std::map<std::string, ClipFile> cache;
    auto it = cache.find(name);
    if (it == cache.end()) {
        const std::string path = "assets/enemies/anim/" + name + ".glb";
        std::ifstream f(path, std::ios::binary);
        ClipFile F;
        if (!f) F.error = "missing";
        else {
            std::stringstream ss; ss << f.rdbuf();
            F = readClips(ss.str(), path, N);
            if (!F.error.empty()) std::fprintf(stderr, "anim: %s\n", F.error.c_str());
        }
        it = cache.emplace(name, std::move(F)).first;
    }
    return it->second.error.empty() ? &it->second : nullptr;
}

} // namespace anim
