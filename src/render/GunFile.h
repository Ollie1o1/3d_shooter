#pragma once
// =============================================================================
// GunFile.h — a gun's shape in and out of glTF (.glb, or .gltf text), the
// format Blender exports. Just what a gun needs:
//
//   nodes      a node named after a joint ("crane", "muzzle"... GunModel.h)
//              is that joint, at rest where the node is; everything under it
//              rides it. Any other node is just grouping.
//   meshes     each primitive of a mesh is one box: its bounds (the
//              accessor's min/max), placed by its node. Boxes in Blender.
//   materials  a part's colour is its material's base colour, except a
//              material named "glow" or "glow*0.5": the gun's glow colour (x 0.5)
//   extras     (Blender: custom properties, exported when "Include > Custom
//              Properties" is ticked) on a part's node:
//                glow  = "breath"  the channel it glows on (GunModel.h)
//                k     = 0.8 or [r, g, b]   how strongly (default 1)
//                phase = 1.2       its glow pulses along a wave, at this phase
//                show  = "loaded" / "!bigmag"   only while the flag holds (or doesn't)
//
// Gun space is glTF's: +Y up, +Z along the barrel (Blender: -Y), metres.
// No OpenGL; a tiny JSON reader below, standard library only.
// =============================================================================
#include "render/GunModel.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace gunkit {

// ---- JSON (just enough for glTF) ------------------------------------------------
struct JVal {
    enum Type { NUL, BOOL, NUM, STR, ARR, OBJ } t = NUL;
    double n = 0.0; bool b = false;
    std::string s;
    std::vector<JVal> a;
    std::vector<std::pair<std::string, JVal>> o;
    const JVal* get(const char* key) const {
        if (t != OBJ) return nullptr;
        for (auto& kv : o) if (kv.first == key) return &kv.second;
        return nullptr;
    }
    double num(const char* key, double def) const { auto* v = get(key); return v && v->t == NUM ? v->n : def; }
    int    idx(const char* key) const { auto* v = get(key); return v && v->t == NUM ? (int)v->n : -1; }
    std::string str(const char* key) const { auto* v = get(key); return v && v->t == STR ? v->s : std::string(); }
    const std::vector<JVal>& arr(const char* key) const {
        static const std::vector<JVal> none;
        auto* v = get(key); return v && v->t == ARR ? v->a : none;
    }
};
struct JParser {
    const char* p; const char* e; std::string err;
    void ws() { while (p < e && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) ++p; }
    bool fail(const char* m) { if (err.empty()) err = m; return false; }
    bool lit(const char* w) { size_t n = std::strlen(w); if ((size_t)(e - p) < n || std::strncmp(p, w, n)) return false; p += n; return true; }
    bool str(std::string& out) {
        if (p >= e || *p != '"') return fail("expected a string");
        ++p;
        while (p < e && *p != '"') {
            char c = *p++;
            if (c != '\\') { out += c; continue; }
            if (p >= e) return fail("bad escape");
            char x = *p++;
            switch (x) {
                case 'n': out += '\n'; break; case 't': out += '\t'; break; case 'r': out += '\r'; break;
                case 'b': out += '\b'; break; case 'f': out += '\f'; break;
                case 'u': {
                    if (e - p < 4) return fail("bad \\u");
                    unsigned cp = (unsigned)std::strtoul(std::string(p, 4).c_str(), nullptr, 16); p += 4;
                    if (cp < 0x80) out += (char)cp;
                    else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
                    else { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
                    break;
                }
                default: out += x;
            }
        }
        if (p >= e) return fail("unterminated string");
        ++p;
        return true;
    }
    bool val(JVal& v, int depth = 0) {
        if (depth > 64) return fail("nested too deep");
        ws();
        if (p >= e) return fail("unexpected end");
        if (*p == '{') {
            v.t = JVal::OBJ; ++p; ws();
            if (p < e && *p == '}') { ++p; return true; }
            for (;;) {
                ws(); std::string k;
                if (!str(k)) return false;
                ws(); if (p >= e || *p != ':') return fail("expected ':'");
                ++p;
                v.o.emplace_back(std::move(k), JVal{});
                if (!val(v.o.back().second, depth + 1)) return false;
                ws();
                if (p < e && *p == ',') { ++p; continue; }
                if (p < e && *p == '}') { ++p; return true; }
                return fail("expected ',' or '}'");
            }
        }
        if (*p == '[') {
            v.t = JVal::ARR; ++p; ws();
            if (p < e && *p == ']') { ++p; return true; }
            for (;;) {
                v.a.emplace_back();
                if (!val(v.a.back(), depth + 1)) return false;
                ws();
                if (p < e && *p == ',') { ++p; continue; }
                if (p < e && *p == ']') { ++p; return true; }
                return fail("expected ',' or ']'");
            }
        }
        if (*p == '"') { v.t = JVal::STR; return str(v.s); }
        if (lit("true"))  { v.t = JVal::BOOL; v.b = true;  return true; }
        if (lit("false")) { v.t = JVal::BOOL; v.b = false; return true; }
        if (lit("null"))  { v.t = JVal::NUL; return true; }
        std::string tok(p, std::min<size_t>(64, e - p));
        char* end = nullptr;
        v.n = std::strtod(tok.c_str(), &end);
        size_t used = (size_t)(end - tok.c_str());
        if (!used) return fail("unexpected character");
        v.t = JVal::NUM; p += used;
        return true;
    }
};
inline bool parseJson(const std::string& text, JVal& out, std::string& err) {
    JParser P{text.data(), text.data() + text.size(), {}};
    if (!P.val(out)) { err = "bad JSON: " + P.err; return false; }
    return true;
}

// ---- reading ----------------------------------------------------------------------
inline glm::mat4 nodeMatrix(const JVal& n) {
    auto& m = n.arr("matrix");
    if (m.size() == 16) { float f[16]; for (int i = 0; i < 16; ++i) f[i] = (float)m[i].n; return glm::make_mat4(f); }
    glm::vec3 t{0.f}, s{1.f}; glm::quat q{1.f, 0.f, 0.f, 0.f};
    auto& T = n.arr("translation"); if (T.size() == 3) t = {(float)T[0].n, (float)T[1].n, (float)T[2].n};
    auto& S = n.arr("scale");       if (S.size() == 3) s = {(float)S[0].n, (float)S[1].n, (float)S[2].n};
    auto& Q = n.arr("rotation");    if (Q.size() == 4) q = glm::quat((float)Q[3].n, (float)Q[0].n, (float)Q[1].n, (float)Q[2].n);   // glTF: x y z w
    return glm::translate(glm::mat4(1.f), t) * glm::mat4_cast(q) * glm::scale(glm::mat4(1.f), s);
}

// The JSON text of a .glb (binary glTF) or a .gltf file
inline bool gltfJson(const std::string& bytes, std::string& json, std::string& err) {
    auto u32 = [&](size_t at) { uint32_t v = 0; std::memcpy(&v, bytes.data() + at, 4); return v; };   // glb is little-endian, as is every target
    if (bytes.size() >= 20 && u32(0) == 0x46546C67u) {   // "glTF"
        uint32_t len = u32(20 - 8), type = u32(16);
        if (type != 0x4E4F534Au || 20 + (size_t)len > bytes.size()) { err = "glb: no JSON chunk"; return false; }
        json = bytes.substr(20, len);
        return true;
    }
    json = bytes;
    return true;
}

inline GunModel readGunText(const std::string& bytes, const std::string& path) {
    GunModel m;
    auto bad = [&](const std::string& why) { m = GunModel{}; m.error = path + ": " + why; return m; };
    std::string json, err;
    if (!gltfJson(bytes, json, err)) return bad(err);
    JVal doc;
    if (!parseJson(json, doc, err)) return bad(err);

    struct Mat { glm::vec3 color{1.f}; float glowScale = -1.f; };
    std::vector<Mat> mats;
    for (auto& jm : doc.arr("materials")) {
        Mat mt;
        if (auto* pbr = jm.get("pbrMetallicRoughness")) {
            auto& c = pbr->arr("baseColorFactor");
            if (c.size() >= 3) mt.color = {(float)c[0].n, (float)c[1].n, (float)c[2].n};
        }
        std::string name = jm.str("name");
        if (name.compare(0, 4, "glow") == 0) {
            size_t star = name.find('*');
            mt.glowScale = star == std::string::npos ? 1.f : std::strtof(name.c_str() + star + 1, nullptr);
        }
        mats.push_back(mt);
    }
    struct Box { glm::mat4 box; int mat; };
    const auto& accessors = doc.arr("accessors");
    std::vector<std::vector<Box>> meshes;
    for (auto& jm : doc.arr("meshes")) {
        meshes.emplace_back();
        for (auto& pr : jm.arr("primitives")) {
            const JVal* at = pr.get("attributes");
            int a = at ? at->idx("POSITION") : -1;
            if (a < 0 || a >= (int)accessors.size()) return bad("a mesh with no positions");
            auto& lo = accessors[a].arr("min"); auto& hi = accessors[a].arr("max");
            if (lo.size() != 3 || hi.size() != 3) return bad("a mesh's positions have no bounds (min/max)");
            glm::vec3 l{(float)lo[0].n, (float)lo[1].n, (float)lo[2].n}, h{(float)hi[0].n, (float)hi[1].n, (float)hi[2].n};
            meshes.back().push_back({glm::translate(glm::mat4(1.f), (l + h) * 0.5f) * glm::scale(glm::mat4(1.f), h - l), pr.idx("material")});
        }
    }
    const auto& nodes = doc.arr("nodes");
    glm::mat4 frame[J_COUNT];
    for (auto& f : frame) f = glm::mat4(1.f);
    bool seen[J_COUNT] = {};
    std::string problem;
    std::vector<int> visiting(nodes.size(), 0);
    std::function<void(int, const glm::mat4&, int)> walk = [&](int ni, const glm::mat4& parentWorld, int ctx) {
        if (!problem.empty()) return;
        if (ni < 0 || ni >= (int)nodes.size() || visiting[ni]) { problem = "a broken node tree"; return; }
        visiting[ni] = 1;
        const JVal& n = nodes[ni];
        glm::mat4 world = parentWorld * nodeMatrix(n);
        std::string name = n.str("name");
        int j = lookup<J_COUNT>(jointName, name);
        if (j > J_ROOT) {
            if (seen[j]) { problem = "two joints named '" + name + "'"; return; }
            seen[j] = true; frame[j] = world;
            m.joints.push_back({j, ctx, world});
            ctx = j;
        }
        int mi = n.idx("mesh");
        if (mi >= 0) {
            if (mi >= (int)meshes.size()) { problem = "node '" + name + "' has no mesh " + std::to_string(mi); return; }
            ModelPart p;
            p.joint = ctx;
            if (const JVal* x = n.get("extras")) {
                std::string g = x->str("glow");
                if (!g.empty()) {
                    p.chan = lookup<C_COUNT>(chanName, g);
                    if (p.chan < 0) {
                        problem = "'" + name + "' glows on '" + g + "' - the channels are:";
                        for (int c = 0; c < C_COUNT; ++c) problem += std::string(" ") + chanName(c);
                        return;
                    }
                    p.k = glm::vec3(1.f);
                    if (const JVal* k = x->get("k")) {
                        if (k->t == JVal::NUM) p.k = glm::vec3((float)k->n);
                        else if (k->t == JVal::ARR && k->a.size() == 3) p.k = {(float)k->a[0].n, (float)k->a[1].n, (float)k->a[2].n};
                    }
                    if (const JVal* ph = x->get("phase"); ph && ph->t == JVal::NUM) { p.hasPhase = true; p.phase = (float)ph->n; }
                }
                std::string show = x->str("show");
                if (!show.empty()) {
                    p.want = show[0] != '!';
                    p.flag = lookup<F_COUNT>(flagName, p.want ? show : show.substr(1));
                    if (p.flag < 0) { problem = "'" + name + "' shows on '" + show + "' - the flags are: loaded bigmag (! for not)"; return; }
                }
            }
            glm::mat4 toJoint = glm::inverse(frame[ctx]) * world;
            for (const Box& bx : meshes[mi]) {
                ModelPart q = p;
                q.rel = toJoint * bx.box;
                if (bx.mat >= 0 && bx.mat < (int)mats.size()) { q.color = mats[bx.mat].color; q.glowScale = mats[bx.mat].glowScale; }
                m.parts.push_back(q);
            }
        }
        for (auto& c : n.arr("children")) walk((int)c.n, world, ctx);
        visiting[ni] = 0;
    };
    const auto& scenes = doc.arr("scenes");
    int si = std::max(0, doc.idx("scene"));
    if (si >= (int)scenes.size()) return bad("no scene");
    for (auto& r : scenes[si].arr("nodes")) walk((int)r.n, glm::mat4(1.f), J_ROOT);
    if (!problem.empty()) return bad(problem);
    if (m.parts.empty()) return bad("no boxes in it");
    return m;
}
inline GunModel readGunFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { GunModel m; m.error = path + ": can't open it"; return m; }
    std::stringstream ss; ss << f.rdbuf();
    return readGunText(ss.str(), path);
}

// ---- writing ----------------------------------------------------------------------
// One unit cube (-0.5..0.5) shared by every part, one mesh per material.
// `materialName` names a part's material (it's what you see in Blender);
// parts of a shade of the glow must get "glow" / "glow*s" (readGunText reads them back so).
inline std::string writeGunText(const GunModel& m, const std::string& rootName,
                                const std::function<std::string(const ModelPart&)>& materialName) {
    auto num = [](double v) { char b[32]; std::snprintf(b, sizeof b, "%.9g", v); return std::string(b); };
    auto quote = [](const std::string& s) { std::string o = "\""; for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; } return o + "\""; };
    auto mat16 = [&](const glm::mat4& x) {
        const float* f = glm::value_ptr(x); std::string o = "[";
        for (int i = 0; i < 16; ++i) o += (i ? "," : "") + num(f[i]);
        return o + "]";
    };
    // materials
    std::vector<std::string> matNames; std::vector<glm::vec3> matColors;
    std::vector<int> partMat;
    for (auto& p : m.parts) {
        std::string nm = materialName(p);
        int found = -1;
        for (size_t i = 0; i < matNames.size(); ++i) if (matNames[i] == nm) found = (int)i;
        if (found < 0) { found = (int)matNames.size(); matNames.push_back(nm); matColors.push_back(p.color); }
        partMat.push_back(found);
    }
    // the node tree: 0 the gun, then its joints, then its parts
    std::vector<std::string> nodeJson;
    std::vector<std::vector<int>> kids(1 + m.joints.size());
    std::map<int, int> jointNode;   // joint id -> node index
    jointNode[J_ROOT] = 0;
    auto frameOf = [&](int id) { for (auto& J : m.joints) if (J.id == id) return J.frame; return glm::mat4(1.f); };
    for (size_t i = 0; i < m.joints.size(); ++i) {
        jointNode[m.joints[i].id] = (int)(1 + i);
        kids[jointNode.at(m.joints[i].parent)].push_back((int)(1 + i));
    }
    std::vector<std::string> partNodes;
    for (size_t i = 0; i < m.parts.size(); ++i) {
        const ModelPart& p = m.parts[i];
        int ni = (int)(1 + m.joints.size() + i);
        kids[jointNode.at(p.joint)].push_back(ni);
        std::string x;
        if (p.chan >= 0) {
            x += "\"glow\":" + quote(chanName(p.chan));
            if (p.k.x == p.k.y && p.k.y == p.k.z) { if (p.k.x != 1.f) x += ",\"k\":" + num(p.k.x); }
            else x += ",\"k\":[" + num(p.k.x) + "," + num(p.k.y) + "," + num(p.k.z) + "]";
            if (p.hasPhase) x += ",\"phase\":" + num(p.phase);
        }
        if (p.flag >= 0) x += std::string(x.empty() ? "" : ",") + "\"show\":" + quote(std::string(p.want ? "" : "!") + flagName(p.flag));
        partNodes.push_back("{\"name\":" + quote(matNames[partMat[i]]) + ",\"mesh\":" + std::to_string(partMat[i]) +
                            ",\"matrix\":" + mat16(p.rel) + (x.empty() ? "" : ",\"extras\":{" + x + "}") + "}");
    }
    auto children = [&](int ni) {
        if (ni >= (int)kids.size() || kids[ni].empty()) return std::string();
        std::string o = ",\"children\":[";
        for (size_t i = 0; i < kids[ni].size(); ++i) o += (i ? "," : "") + std::to_string(kids[ni][i]);
        return o + "]";
    };
    nodeJson.push_back("{\"name\":" + quote(rootName) + children(0) + "}");
    for (size_t i = 0; i < m.joints.size(); ++i) {
        const ModelJoint& J = m.joints[i];
        nodeJson.push_back("{\"name\":" + quote(jointName(J.id)) + ",\"matrix\":" + mat16(glm::inverse(frameOf(J.parent)) * J.frame) + children((int)(1 + i)) + "}");
    }
    for (auto& s : partNodes) nodeJson.push_back(s);

    std::string j = "{\"asset\":{\"version\":\"2.0\",\"generator\":\"OVERDRIVE GunFile.h\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],\"nodes\":[";
    for (size_t i = 0; i < nodeJson.size(); ++i) j += (i ? ",\n" : "\n") + nodeJson[i];
    j += "],\"materials\":[";
    for (size_t i = 0; i < matNames.size(); ++i) {
        glm::vec3 c = matColors[i];
        j += (i ? ",\n" : "\n") + std::string("{\"name\":") + quote(matNames[i]) + ",\"pbrMetallicRoughness\":{\"baseColorFactor\":[" +
             num(c.x) + "," + num(c.y) + "," + num(c.z) + ",1],\"metallicFactor\":0,\"roughnessFactor\":1}}";
    }
    j += "],\"meshes\":[";
    for (size_t i = 0; i < matNames.size(); ++i)
        j += (i ? "," : "") + std::string("{\"name\":") + quote(matNames[i]) + ",\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1,\"material\":" + std::to_string(i) + "}]}";
    j += "],\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":8,\"type\":\"VEC3\",\"min\":[-0.5,-0.5,-0.5],\"max\":[0.5,0.5,0.5]},"
         "{\"bufferView\":1,\"componentType\":5123,\"count\":36,\"type\":\"SCALAR\"}],"
         "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":96,\"target\":34962},{\"buffer\":0,\"byteOffset\":96,\"byteLength\":72,\"target\":34963}],"
         "\"buffers\":[{\"byteLength\":168}]}";

    // the cube: corner i at (x, y, z) = bits of i; each face two triangles wound outward
    std::string bin;
    auto put = [&](const void* d, size_t n) { bin.append((const char*)d, n); };
    for (int i = 0; i < 8; ++i) { float v[3] = {(i & 1) ? 0.5f : -0.5f, (i & 2) ? 0.5f : -0.5f, (i & 4) ? 0.5f : -0.5f}; put(v, 12); }
    auto corner = [](int i) { return glm::vec3((i & 1) ? 0.5f : -0.5f, (i & 2) ? 0.5f : -0.5f, (i & 4) ? 0.5f : -0.5f); };
    for (int axis = 0; axis < 3; ++axis)
        for (int side = 0; side < 2; ++side) {
            int bit = 1 << axis, u = 1 << ((axis + 1) % 3), w = 1 << ((axis + 2) % 3);
            int base = side ? bit : 0;
            int q[4] = {base, base | u, base | u | w, base | w};
            glm::vec3 nrm(0.f); nrm[axis] = side ? 1.f : -1.f;
            glm::vec3 a = corner(q[0]), b2 = corner(q[1]), c = corner(q[2]);
            if (glm::dot(glm::cross(b2 - a, c - a), nrm) < 0.f) std::swap(q[1], q[3]);
            uint16_t tri[6] = {(uint16_t)q[0], (uint16_t)q[1], (uint16_t)q[2], (uint16_t)q[0], (uint16_t)q[2], (uint16_t)q[3]};
            put(tri, 12);
        }
    while (j.size() % 4) j += ' ';
    std::string out;
    auto u32 = [&](uint32_t v) { out.append((const char*)&v, 4); };
    u32(0x46546C67u); u32(2); u32((uint32_t)(12 + 8 + j.size() + 8 + bin.size()));
    u32((uint32_t)j.size()); u32(0x4E4F534Au); out += j;
    u32((uint32_t)bin.size()); u32(0x004E4942u); out += bin;
    return out;
}
inline bool writeGunFile(const std::string& path, const GunModel& m, const std::string& rootName,
                         const std::function<std::string(const ModelPart&)>& materialName) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    std::string bytes = writeGunText(m, rootName, materialName);
    f.write(bytes.data(), (std::streamsize)bytes.size());
    return (bool)f;
}

} // namespace gunkit
