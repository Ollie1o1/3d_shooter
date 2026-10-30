#pragma once
#include "world/Level.h"
#include "player/Player.h"
#include "game/WaveDirector.h"

namespace storylift {
inline bool aboard(const LevelData& level, const Player& p, int mover) {
    const auto& m = level.movers[mover];
    const auto& b = level.walls[m.wall].box;
    // Test feet as well as groundWall: flush boarding seams must not depend
    // on which of two supporting boxes resolved collision last.
    return p.onGround && (p.groundWall == m.wall ||
        (std::fabs(p.position.y-b.max.y)<.09f &&
         p.position.x>b.min.x+p.radius && p.position.x<b.max.x-p.radius &&
         p.position.z>b.min.z+p.radius && p.position.z<b.max.z-p.radius));
}

inline void update(LevelData& level, WaveDirector& director, const Player& p, float dt) {
    const auto& s = level.story;
    if (s.serviceLift >= 0) {
        auto& lift = level.movers[s.serviceLift];
        // Calling is automatic from either landing. Once aboard it completes
        // the ascent, including while the rider briefly jumps on the deck.
        const auto& deck = level.walls[lift.wall].box;
        bool inShaft = p.position.x > lift.base.min.x-.4f && p.position.x < lift.base.max.x+.1f &&
                       p.position.z > lift.base.min.z-.4f && p.position.z < lift.base.max.z+.4f;
        bool atUpperLanding = std::fabs(p.position.y-(lift.base.max.y+lift.b.y))<1.f &&
                              p.position.x>lift.base.min.x-.4f && p.position.x<lift.base.max.x+5.f &&
                              p.position.z>lift.base.min.z-.4f && p.position.z<lift.base.max.z+.4f;
        bool rise = aboard(level,p,s.serviceLift) ||
                    (inShaft && lift.drive>0 && p.position.y>deck.max.y-.2f) || atUpperLanding;
        float target = rise ? 1.f : 0.f;
        float step = dt/4.5f;
        lift.drive += std::clamp(target-lift.drive,-step,step);
    }
    director.hold = false;
    if (s.elevator < 0 || director.arena != 6) return;
    auto& lift = level.movers[s.elevator];
    const auto& b = level.walls[lift.wall].box;
    auto& arena = level.arenas[6];
    arena.hasRespawn = true;
    arena.respawn = {(b.min.x+b.max.x)*.5f,b.max.y+.05f,(b.min.z+b.max.z)*.5f};
    arena.voidY = b.max.y-7.f;
    bool riding = aboard(level,p,s.elevator);
    if (director.phase == WaveDirector::Phase::APPROACH && riding) director.startArena(6);
    if (director.phase == WaveDirector::Phase::INTRO || director.fighting() ||
        director.phase == WaveDirector::Phase::BREAK) {
        float target = (director.wave+1)/3.f;
        if (riding || lift.drive>director.wave/3.f+.001f)
            lift.drive = std::min(target,lift.drive+dt/15.f);
        director.hold = lift.drive<target-.0001f;
    }
}
}
