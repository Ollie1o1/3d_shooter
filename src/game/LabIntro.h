#pragma once
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

// Deterministic cinematic clock, shared by live gameplay and regression tests.
struct LabIntro {
    static constexpr float DURATION=23.f, SKIP_HOLD=.65f;
    float time=-1.f, skip=0;
    bool seen=false;
    bool skipReady=false; // a menu's held Jump/Space must be released first
    bool active() const {return time>=0;}
    void start(bool alreadySeen){seen=alreadySeen;time=seen?-1.f:0.f;skip=0;skipReady=false;}
    bool update(float dt,bool held){
        if(!active())return false;
        time+=dt;if(!held)skipReady=true;skip=held&&skipReady?skip+dt:0;
        if(time>=DURATION || (time>1 && skip>=SKIP_HOLD)){seen=true;time=-1;skip=0;return true;}
        return false;
    }
    static float smooth(float a,float b,float t){float x=std::clamp((t-a)/(b-a),0.f,1.f);return x*x*(3-2*x);}
    glm::vec3 cameraOffset() const {
        float x=smooth(3,8,time),z=smooth(3,10,time);
        return {glm::mix(0.f,2.2f,x),1.7f,glm::mix(27.f,20.f,z)};
    }
    float yaw() const {return -90.f-10.f*smooth(3,8,time)+10.f*smooth(18,22,time);}
    float pitch() const {return -10.f*smooth(0,3,time)+12.f*smooth(4,9,time)-2.f*smooth(19,22,time);}
    float patientRise() const {return smooth(10,15,time)*1.35f;}
    float windowChange() const {return smooth(15,19,time);}
    const char* caption() const {
        if(time<4)return "LEGACY CHASSIS / MANUAL ASSESSMENT";
        if(time<10)return "MOTOR RESPONSE: WITHIN NORMAL RANGE";
        if(time<16)return "FLOOR CONTACT: CONFIRMED";
        if(time<20)return "OBSERVATION RECORD ACCEPTED";
        return "PROCEED TO OBSERVATION.";
    }
};
