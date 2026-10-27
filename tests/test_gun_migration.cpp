// TEMPORARY (gun-files migration): the guns drawn from assets/guns/*.glb (rig +
// file + extras) must be the guns the old C++ recipes drew, box for box, in
// every state: reloads beat by beat, pumps and bolts, firing, heat, breath,
// every magazine size.   make test-migration
#include "legacy/GunKitLegacy.h"
#include "legacy/GunShapes.h"
#include <cstdio>
#include <random>

using namespace gunkit;

static bool close(const GunPart& a, const GunPart& b) {
    if (a.tag != b.tag) return false;
    const float* x = &a.xf[0][0]; const float* y = &b.xf[0][0];
    for (int i = 0; i < 16; ++i) if (std::fabs(x[i] - y[i]) > 2e-5f) return false;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(a.color[i] - b.color[i]) > 1e-4f) return false;
        if (std::fabs(a.emissive[i] - b.emissive[i]) > 1e-4f * (1.f + std::fabs(a.emissive[i]))) return false;
    }
    return true;
}
// The same boxes (in any order)?
static bool same(const std::vector<GunPart>& want, const std::vector<GunPart>& got, std::string& why) {
    if (want.size() != got.size()) { why = std::to_string(want.size()) + " boxes, got " + std::to_string(got.size()); }
    std::vector<char> used(got.size(), 0);
    for (size_t i = 0; i < want.size(); ++i) {
        bool hit = false;
        for (size_t j = 0; j < got.size() && !hit; ++j)
            if (!used[j] && close(want[i], got[j])) { used[j] = 1; hit = true; }
        if (!hit) {
            const GunPart& p = want[i];
            char b[256];
            std::snprintf(b, sizeof b, " | box %zu missing: at (%.4f %.4f %.4f) colour (%.3f %.3f %.3f) glow (%.3f %.3f %.3f) tag %d",
                          i, p.xf[3][0], p.xf[3][1], p.xf[3][2], p.color.x, p.color.y, p.color.z, p.emissive.x, p.emissive.y, p.emissive.z, p.tag);
            why += b;
            return false;
        }
    }
    return why.empty();
}

struct State { GunLook L; GunPose P; std::string what; };

static std::vector<State> states() {
    std::vector<State> s;
    s.push_back({GunLook{}, GunPose{}, "rest"});
    for (int mag : {1, 2, 5, 8, 9, 10, 14, 16})
        for (int ammo : {0, mag / 2, mag}) {
            GunLook L; L.mag = mag; L.ammo = ammo; L.lit = ammo;
            s.push_back({L, GunPose{}, "mag " + std::to_string(mag) + " ammo " + std::to_string(ammo)});
        }
    for (int refill : {1, 3, 5})
        for (int i = 0; i <= 100; ++i) {
            GunLook L; L.refill = refill; L.ammo = 2; L.lit = i / 25;
            GunPose P; P.reloadU = i / 100.f;
            s.push_back({L, P, "reload " + std::to_string(i) + "% refill " + std::to_string(refill)});
        }
    for (int i = 0; i <= 40; ++i) { GunPose P; P.cycleU = i / 40.f; s.push_back({GunLook{}, P, "cycle " + std::to_string(i)}); }
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> U(0.f, 1.f);
    for (int i = 0; i < 800; ++i) {
        GunLook L; GunPose P;
        L.mag = 1 + (int)(U(rng) * 16); L.ammo = (int)(U(rng) * (L.mag + 1)); L.lit = std::min(L.mag, (int)(U(rng) * (L.mag + 1)));
        L.flash = U(rng) < 0.4f ? U(rng) : 0.f; L.heat = U(rng); L.breath = U(rng); L.cylSpin = U(rng) < 0.5f ? U(rng) * 45.f : 0.f;
        L.refill = 1 + (int)(U(rng) * 5); L.aim = U(rng);
        P.fireU = U(rng) < 0.5f ? U(rng) : 0.f; P.boot = U(rng) < 0.3f ? U(rng) : 1.f;
        float r = U(rng);
        if (r < 0.3f) P.reloadU = U(rng); else if (r < 0.5f) P.cycleU = U(rng);
        s.push_back({L, P, "random " + std::to_string(i)});
    }
    return s;
}

int main() {
    int failed = 0, checks = 0;
    auto check = [&](bool ok, const std::string& what) { ++checks; if (!ok) { ++failed; std::printf("  FAIL %s\n", what.c_str()); } };
    auto S = states();
    for (int g = 0; g < 4; ++g) {
        GunModel onDisk = readGunFile(gunPath(g));
        check(onDisk.error.empty(), std::string(gunName(g)) + ": " + onDisk.error);
        GunModel rec = recordGun(g);
        GunModel trip = readGunText(writeGunText(rec, gunName(g), [](const ModelPart& p) {
            char b[64]; std::snprintf(b, sizeof b, "%.9g %.9g %.9g", p.color.x, p.color.y, p.color.z);
            return p.glowScale >= 0.f ? "glow*" + std::to_string(p.glowScale) : std::string(b); }), "memory");
        check(trip.error.empty(), std::string(gunName(g)) + " round trip: " + trip.error);
        int bad[3] = {0, 0, 0};
        for (auto& st : S) {
            std::vector<GunPart> want, a, b, c;
            gunkit_legacy::buildGun(g, st.L, st.P, want);
            buildGunFromShapes(g, st.L, st.P, a);
            buildGunFrom(trip, g, st.L, st.P, b);
            buildGunFrom(onDisk, g, st.L, st.P, c);
            std::string why;
            const std::vector<GunPart>* got[3] = {&a, &b, &c};
            const char* name[3] = {"C++ shapes", "round trip", "assets file"};
            for (int k = 0; k < 3; ++k) {
                why.clear();
                if (!same(want, *got[k], why) && bad[k]++ < 3)
                    std::printf("  %s, %s, %s:%s\n", gunName(g), name[k], st.what.c_str(), why.c_str());
            }
        }
        check(!bad[0], std::string(gunName(g)) + ": the C++ shapes draw what the recipe drew (" + std::to_string(bad[0]) + " states differ)");
        check(!bad[1], std::string(gunName(g)) + ": written and read back, still the same (" + std::to_string(bad[1]) + " differ)");
        check(!bad[2], std::string(gunName(g)) + ": assets/guns file draws what the recipe drew (" + std::to_string(bad[2]) + " differ)");
        std::printf("%-9s %zu states, %zu boxes at rest\n", gunName(g), S.size(), rec.parts.size());
    }
    std::printf("%d/%d checks passed\n", checks - failed, checks);
    return failed ? 1 : 0;
}
