#pragma once
#include "bosses/StoryBossTools.h"
#include <vector>
struct SuccessorHazards {
    struct Mark {glm::vec3 a,b;float wait,life,radius,damage;int kind;bool fired=false;};
    std::vector<Mark> marks;
    int epoch=-1;
    void clear(){marks.clear();epoch=-1;}
    void circle(glm::vec3 p,float delay,float damage,int kind=0){if(marks.size()<8)marks.push_back({p,p,delay,.35f,2.8f,damage,kind,false});}
    void line(glm::vec3 a,glm::vec3 b,float delay){if(marks.size()<8)marks.push_back({a,b,delay,.28f,.8f,22,1,false});}
    std::vector<glm::vec3> update(float dt,glm::vec3 feet,float& damage){
        std::vector<glm::vec3> hits;damage=0;
        for(auto& m:marks){
            if(m.wait>0){m.wait-=dt;if(m.wait>0)continue;}
            if(!m.fired){m.fired=true;bool hit=machinerySweepHits(feet,m.a,m.b,m.radius,m.a.y,2.8f);
                if(hit){damage=std::max(damage,m.damage);hits.push_back(m.a);}}
            m.life-=dt;
        }
        marks.erase(std::remove_if(marks.begin(),marks.end(),[](const Mark&m){return m.wait<=0&&m.life<=0;}),marks.end());
        return hits;
    }
};
