#pragma once
// =============================================================================
// WorldSound.h — where a level's own sounds come from (audio/WorldEmitters.h),
// read off what the level already knows: its lava, its reactor, its fans, its
// water. No authoring: a new hazard or fan is heard as soon as it exists.
// No GL and no audio (the tests count them).
// =============================================================================
#include "world/Level.h"
#include "audio/WorldEmitters.h"

inline void gatherWorldEmitters(const LevelData& L, worldsound::Director& out) {
    using worldsound::Kind;
    out.clear();
    const size_t CAP = 96;
    // Lava: a voice every nine metres along each channel, at its surface
    for (const Hazard& h : L.hazards) {
        const glm::vec3 c = (h.box.min + h.box.max) * 0.5f, ext = h.box.max - h.box.min;
        const int axis = ext.x >= ext.z ? 0 : 2;
        const int n = glm::clamp((int)std::round(ext[axis] / 9.f), 1, 6);
        for (int i = 0; i < n && out.emitters.size() < CAP; ++i) {
            glm::vec3 p = c; p[axis] = h.box.min[axis] + ext[axis] * ((float)i + 0.5f) / (float)n; p.y = h.box.max.y;
            out.add(p, Kind::LAVA, glm::clamp(std::min(ext.x, ext.z) / 4.f, 0.6f, 1.3f));
        }
    }
    if (L.hasReactor) out.add(L.reactorPos, Kind::HUM, 1.5f);
    for (const auto& f : L.fans) if (out.emitters.size() < CAP) out.add(f.pos, Kind::FAN, glm::clamp(f.radius / 1.6f, 0.5f, 1.4f));
    // Water: at the edges of each pool (a wall it laps against), not out in the middle; none for a sea
    for (const WaterVolume& w : L.water) {
        const glm::vec3 ext = w.box.max - w.box.min;
        if (ext.x > 160.f || ext.z > 160.f || ext.x < 2.f || ext.z < 2.f) continue;
        const float ix = std::min(1.5f, ext.x * 0.25f), iz = std::min(1.5f, ext.z * 0.25f);
        for (int k = 0; k < 4 && out.emitters.size() < CAP; ++k) {
            const glm::vec3 p{k & 1 ? w.box.max.x - ix : w.box.min.x + ix, w.level, k & 2 ? w.box.max.z - iz : w.box.min.z + iz};
            out.add(p, Kind::WATER, 0.8f);
        }
    }
}
