#pragma once
#include "core/Collision.h"
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

// The same moving front and cover test are used by gameplay and BossSim.
// Dust decoration is optional; the damaging front and its outline are not.
struct BallastHazards {
    struct Front { glm::vec3 from, dir; float distance=0, age=0; bool hit=false; };
    struct Hit { float damage; glm::vec3 push, at; };
    std::vector<Front> fronts;
    static constexpr float SPEED=19.f, HALF_WIDTH=6.f, RANGE=76.f;
    void clear() { fronts.clear(); }
    void add(glm::vec3 from, glm::vec3 to) {
        glm::vec3 d=to-from;d.y=0;float n=glm::length(d);
        if(n>.01f && fronts.size()<4)fronts.push_back({from,d/n});
    }
    static float along(glm::vec3 from,glm::vec3 dir,glm::vec3 p) {return glm::dot(p-from,dir);}
    static float across(glm::vec3 from,glm::vec3 dir,glm::vec3 p) {
        return std::fabs(glm::dot(p-from,glm::vec3{-dir.z,0,dir.x}));
    }
    template<class Walls>
    std::vector<Hit> update(float dt,glm::vec3 feet,const Walls& walls) {
        std::vector<Hit> hits;
        for(auto& f:fronts) {
            float before=f.distance;f.age+=dt;f.distance+=SPEED*dt;
            float a=along(f.from,f.dir,feet),side=across(f.from,f.dir,feet);
            if(f.hit||a<before-1.f||a>f.distance+1.f||side>HALF_WIDTH||feet.y<f.from.y-.6f||feet.y>f.from.y+3.f)continue;
            // A shelter protects the part of the front directly behind it.
            glm::vec3 sample=feet+glm::vec3{0,1.2f,0},origin=sample-f.dir*(a+.5f);
            glm::vec3 ray=sample-origin;float length=glm::length(ray);bool sheltered=false;
            if(length>.01f)for(const auto& wall:walls) {
                float t=rayBoxHit(origin,ray/length,wall.box);
                if(t>.05f&&t<length-.2f){sheltered=true;break;}
            }
            if(!sheltered){hits.push_back({24.f,f.dir*7.f+glm::vec3{0,2,0},sample});f.hit=true;}
        }
        fronts.erase(std::remove_if(fronts.begin(),fronts.end(),[](const Front& f){return f.distance>RANGE;}),fronts.end());
        return hits;
    }
};
