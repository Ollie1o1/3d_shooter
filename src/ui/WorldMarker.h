#pragma once
#include "core/Collision.h"
#include <cmath>

struct WorldMarker {
    float x=0, y=0;
    bool onScreen=false, behind=false;
};

// Items and hostiles must agree on both projection and edge placement. A
// target behind the camera is always a direction at the edge, never a point
// apparently attached to empty space in front of the player.
inline WorldMarker projectWorldMarker(glm::vec3 target,const glm::mat4& viewProjection,float width,float height) {
    glm::vec4 clip=viewProjection*glm::vec4(target,1.f);
    WorldMarker marker;marker.behind=clip.w<=.01f;
    glm::vec2 direction=glm::vec2(clip)/std::max(std::fabs(clip.w),.01f);
    marker.onScreen=!marker.behind && std::fabs(direction.x)<.92f && std::fabs(direction.y)<.85f;
    if(!marker.onScreen) {
        float scale=std::max(std::fabs(direction.x)/.92f,std::fabs(direction.y)/.85f);
        // Exactly behind: choose a stable horizontal turn instead of pointing
        // at the crosshair, or changing edges with tiny floating point errors.
        if(scale<1e-4f)direction={.92f,0};
        else direction/=scale;
    }
    marker.x=(direction.x*.5f+.5f)*width;
    marker.y=(.5f-direction.y*.5f)*height;
    return marker;
}

inline bool markerBehindCover(glm::vec3 eye,glm::vec3 target,const std::vector<Wall>& walls) {
    glm::vec3 delta=target-eye;float distance=glm::length(delta);
    if(distance<.01f)return false;
    for(const auto& wall:walls) {
        float hit=rayBoxHit(eye,delta/distance,wall.box);
        if(hit>0.f && hit<distance-.15f)return true;
    }
    return false;
}
