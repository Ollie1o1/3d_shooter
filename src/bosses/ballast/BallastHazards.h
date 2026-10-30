#pragma once
#include "core/Collision.h"
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

// The same moving front and cover test are used by gameplay and BossSim.
// Dust decoration is optional; the damaging front and its outline are not.
struct BallastHazards {
    struct Front { glm::vec3 from, dir; float distance=0, age=0; bool hit=false; bool rubble=false; };
    struct Hit { float damage; glm::vec3 push, at; };
    std::vector<Front> fronts;
    static constexpr float SPEED=19.f, HALF_WIDTH=6.f, RANGE=76.f;
    void clear() { fronts.clear(); }
    void add(glm::vec3 from, glm::vec3 to) {
        glm::vec3 d=to-from;d.y=0;float n=glm::length(d);
        if(n>.01f && fronts.size()<4)fronts.push_back({from,d/n});
    }
    void addRip(glm::vec3 from,glm::vec3 to) {
        glm::vec3 d=to-from;d.y=0;float n=glm::length(d);if(n<.01f)return;d/=n;
        for(float angle:{-.25f,0.f,.25f})if(fronts.size()<7) {
            glm::vec3 dir{d.x*std::cos(angle)+d.z*std::sin(angle),0,d.z*std::cos(angle)-d.x*std::sin(angle)};
            fronts.push_back({from,dir,0,0,false,true});
        }
    }
    static float halfWidth(const Front& f){return f.rubble?1.8f:HALF_WIDTH;}
    static float frontSpeed(const Front& f){return f.rubble?24.f:SPEED;}
    static float along(glm::vec3 from,glm::vec3 dir,glm::vec3 p) {return glm::dot(p-from,dir);}
    static float across(glm::vec3 from,glm::vec3 dir,glm::vec3 p) {
        return std::fabs(glm::dot(p-from,glm::vec3{-dir.z,0,dir.x}));
    }
    template<class Walls>
    std::vector<Hit> update(float dt,glm::vec3 feet,const Walls& walls) {
        std::vector<Hit> hits;
        for(auto& f:fronts) {
            float before=f.distance;f.age+=dt;f.distance+=frontSpeed(f)*dt;
            if(f.rubble) {
                glm::vec3 origin=f.from+f.dir*before+glm::vec3{0,.8f,0};
                for(const auto& wall:walls){float t=rayBoxHit(origin,f.dir,wall.box);if(t>=0&&t<=f.distance-before){f.distance=56;break;}}
                if(f.distance>55.f)continue;
            }
            float a=along(f.from,f.dir,feet),side=across(f.from,f.dir,feet);
            if(f.hit||a<before-1.f||a>f.distance+1.f||side>halfWidth(f)||feet.y<f.from.y-.6f||feet.y>f.from.y+(f.rubble?1.7f:3.f))continue;
            // A shelter protects the part of the front directly behind it.
            glm::vec3 sample=feet+glm::vec3{0,1.2f,0},origin=sample-f.dir*(a+.5f);
            glm::vec3 ray=sample-origin;float length=glm::length(ray);bool sheltered=false;
            if(length>.01f)for(const auto& wall:walls) {
                float t=rayBoxHit(origin,ray/length,wall.box);
                if(t>.05f&&t<length-.2f){sheltered=true;break;}
            }
            if(!sheltered){hits.push_back({f.rubble?28.f:24.f,f.dir*7.f+glm::vec3{0,2,0},sample});f.hit=true;}
        }
        fronts.erase(std::remove_if(fronts.begin(),fronts.end(),[](const Front& f){return f.distance>(f.rubble?55.f:RANGE);}),fronts.end());
        return hits;
    }
};
