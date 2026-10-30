#pragma once
#include "core/Collision.h"

// Pick the same room volume for normal gameplay and traversal regressions.
// This is separate from physical walls: a gap between volumes can otherwise
// block an apparently open doorway even when its collision is clear.
inline const AABB* constrainToZones(glm::vec3& p, glm::vec3& velocity, float height,
                                   const std::vector<const AABB*>& zones, bool vertical) {
    if(zones.empty())return nullptr;
    float bestD=1e9f;glm::vec2 best{p.x,p.z};const AABB* in=zones.front();
    for(const auto* z:zones) {
        glm::vec2 c{glm::clamp(p.x,z->min.x,z->max.x),glm::clamp(p.z,z->min.z,z->max.z)};
        float d=glm::length(c-glm::vec2{p.x,p.z});
        if(vertical)d+=std::max({z->min.y-p.y,p.y+height-z->max.y,0.f})*2.f;
        if(d<bestD-1e-4f || (d<1e-4f && bestD<1e-4f && z->max.y>in->max.y)) {
            bestD=d;best=c;in=z;
        }
    }
    if(bestD>0.f) {
        if(best.x!=p.x)velocity.x=0.f;
        if(best.y!=p.z)velocity.z=0.f;
        p.x=best.x;p.z=best.y;
    }
    return in;
}
