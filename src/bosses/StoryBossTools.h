#pragma once
#include "core/Collision.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// Shared physical geometry for AI, drawing and headless combat checks.
inline float machineryDistance(glm::vec3 p,glm::vec3 a,glm::vec3 b) {
    p.y=a.y=b.y=0;glm::vec3 d=b-a;float n=glm::dot(d,d);
    return glm::length(p-(a+d*(n>.0001f?glm::clamp(glm::dot(p-a,d)/n,0.f,1.f):0.f)));
}
inline bool machinerySweepHits(glm::vec3 feet,glm::vec3 a,glm::vec3 b,float radius,float floor,float height) {
    return feet.y<floor+height&&feet.y>floor-.7f&&machineryDistance(feet,a,b)<radius;
}
inline glm::vec3 machineryLead(glm::vec3 velocity,float limit=14.f) {
    velocity.y=0;float speed=glm::length(velocity);return speed>limit?velocity*(limit/speed):velocity;
}
inline glm::vec3 machineryClamp(glm::vec3 at,const AABB& bounds,float margin=4.f) {
    at.x=glm::clamp(at.x,bounds.min.x+margin,bounds.max.x-margin);
    at.z=glm::clamp(at.z,bounds.min.z+margin,bounds.max.z-margin);return at;
}
struct MachineryPress {
    glm::vec3 at{0};float remaining=0,impact=-1;
};
