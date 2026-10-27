// TEMPORARY (gun-files migration): writes the four guns' C++ shapes
// (tests/legacy/GunShapes.h) to assets/guns/*.glb.   make export-guns
#include "legacy/GunShapes.h"
#include <cstdio>

using namespace gunkit;

// What a part's material is called in Blender: a palette colour and its shade
// ("steel", "black*0.6"), the gun's glow ("glow*0.5"), or failing those its RGB
static std::string materialName(const ModelPart& p) {
    char buf[64];
    auto shade = [&](const char* name, float s) {
        if (std::fabs(s - 1.f) < 1e-6f) return std::string(name);
        std::snprintf(buf, sizeof buf, "%s*%g", name, s); return std::string(buf);
    };
    if (p.glowScale >= 0.f) return shade("glow", p.glowScale);
    static const std::pair<const char*, vec3> PALETTE[] = {{"black", BLACK}, {"steel", STEEL}, {"edge", EDGE}, {"bone", BONE}, {"brass", BRASS}};
    for (auto& [name, c] : PALETTE) {
        float s = glm::dot(p.color, c) / glm::dot(c, c);
        if (glm::length(p.color - c * s) < 1e-6f) {
            float r = std::round(s * 1000.f) / 1000.f;   // the shades are all short decimals (0.85, 1.6...)
            if (std::fabs(r - s) < 1e-5f) return shade(name, r);
        }
    }
    std::snprintf(buf, sizeof buf, "rgb %.3g %.3g %.3g", p.color.x, p.color.y, p.color.z);
    return buf;
}

int main() {
    int bad = 0;
    for (int g = 0; g < 4; ++g) {
        GunModel m = recordGun(g);
        std::string path = gunPath(g);
        if (!writeGunFile(path, m, gunName(g), materialName)) { std::printf("can't write %s\n", path.c_str()); ++bad; continue; }
        std::printf("%-26s %zu joints, %zu boxes\n", path.c_str(), m.joints.size(), m.parts.size());
    }
    return bad;
}
