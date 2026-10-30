#pragma once
#include "world/Level.h"
#include "bosses/StoryBossTools.h"
#include <vector>

// Only authored light cover can break. Structural walls and escape rails stay.
inline std::vector<int> shatterStoryCover(LevelData& level,glm::vec3 from,glm::vec3 to,float radius,bool keepShelter=false) {
    std::vector<int> broken;int remaining=0;
    for(const auto& c:level.story.bossCover)remaining+=level.walls[c.wall].box.max.y>c.home.min.y;
    for(int i=0;i<(int)level.story.bossCover.size();++i) {
        const auto& c=level.story.bossCover[i];auto& box=level.walls[c.wall].box;
        if(box.max.y<=c.home.min.y||(keepShelter&&remaining<=1))continue;
        AABB expanded{box.min-glm::vec3{radius},box.max+glm::vec3{radius}};
        glm::vec3 delta=to-from;float length=glm::length(delta);
        bool intersects=from.x>=expanded.min.x&&from.x<=expanded.max.x&&from.y>=expanded.min.y&&from.y<=expanded.max.y&&from.z>=expanded.min.z&&from.z<=expanded.max.z;
        if(!intersects&&length>=.0001f){float t=rayBoxHit(from,delta/length,expanded);intersects=t>=0&&t<=length;}
        if(!intersects)continue;
        box.min.y-=500;box.max.y=box.min.y+.01f;--remaining;broken.push_back(i);
    }
    return broken;
}
