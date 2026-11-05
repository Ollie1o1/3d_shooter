#pragma once
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

// Chapter 3's arrival, as a deterministic clock shared by live gameplay and regression tests.
//
// Chapter 2 ends on the Far Road with the tower ahead. This begins at its foot, on the ash:
//   0-4    the tower from the road, its beacon lit
//   4-8    walking in through its door to a service lift
//   8-14   the lift climbs the tower, its lit slots racing past the car's side glass
//   14-19  out of the top: the plain falls away, the red sky goes black, then violet; debris
//          turns past the car and the laboratory hangs overhead, ward after ward going down
//   19-21  the car slows under Admissions and docks
//   21-25  the doors open on the ward; looking back down through the car's glass floor, the
//          tower and its plain are small and still sinking
//   25-29  out onto the tiles; over your shoulder the doors close on it
//   29-32  a scanner passes over the chassis, and play begins where you stand
//
// The lift car runs outside Admissions' entry wall (z 40.6..46.4); the plain is far below it.
struct LabIntro {
    static constexpr float DURATION=32.f, SKIP_HOLD=.65f;
    static constexpr float PLAIN=-260.f, TOWER=90.f, CAR_Z=43.5f;
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
    static float ease(float a,float b,float t){float x=std::clamp((t-a)/(b-a),0.f,1.f);return x*x*x*(x*(x*6-15)+10);}

    // ---- the set ----
    // The ash plain's height: it sinks away once you are up, as if the world let go of the tower
    float plainY() const {return PLAIN-210.f*ease(21.5f,29.f,time);}
    // The car's floor: from the plain to Admissions' floor (y 0), slow off the mark and into the dock
    float carY() const {return PLAIN*(1-ease(8.6f,21.f,time));}
    // 0 closed .. 1 open: the car's rear doors (the tower side) and its front doors + the landing's
    float rearDoors() const {return 1-smooth(7.7f,8.5f,time);}
    float frontDoors() const {return smooth(21.f,22.f,time)*(1-smooth(28.f,28.9f,time));}
    // The sky: Chapter 2's ash (0) to the ward's void (1); the nebula itself comes up a little later
    float skyMix() const {return smooth(13.6f,17.5f,time);}
    float realm() const {return smooth(14.3f,17.8f,time);}
    // A sheet of cyan light passing down over the chassis
    float scan() const {return smooth(29.4f,31.2f,time);}
    bool exterior() const {return time<8.f;}

    // ---- the camera (eye position, then yaw/pitch in degrees: yaw -90 faces -z, into the ward) ----
    glm::vec3 eye() const {
        float t=time,bob=0;
        if(t<4)return {1.2f,plainY()+1.7f,150-2.5f*t};   // a held shot from the road
        if(t<8){float k=ease(4,8,t);bob=.045f*std::sin(t*8.5f)*(1-k*k);
            return {glm::mix(.6f,0.f,k),PLAIN+1.7f+bob,glm::mix(70.f,CAR_Z+.4f,k)};}
        float z=CAR_Z+.4f;
        z=glm::mix(z,41.3f,smooth(21.2f,22.4f,t));      // to the doors
        z=glm::mix(z,42.9f,smooth(22.6f,23.6f,t));      // half a step back over the glass
        float out=ease(25.6f,27.8f,t);z=glm::mix(z,35.8f,out);   // out onto the ward floor
        bob=.04f*std::sin(t*8.5f)*std::sin(3.1416f*out);
        float x=std::sin(t*.7f)*.03f;
        return {x,carY()+1.7f+bob,z};
    }
    float yaw() const {
        float t=time;
        if(t<8)return -90.f;
        float y=-90;
        y=glm::mix(y,-180.f,smooth(8.6f,10.2f,t));     // to the side glass
        y=glm::mix(y,-200.f,smooth(14.f,15.f,t));      // and a little back, watching the tower go
        y=glm::mix(y,-95.f,smooth(15.6f,17.f,t));      // forward and up, to the ward overhead
        y=glm::mix(y,-90.f,smooth(19.2f,20.5f,t));
        y=glm::mix(y,90.f,smooth(22.5f,23.8f,t));      // turn round: back down the shaft
        y=glm::mix(y,-90.f,smooth(25.2f,26.4f,t));     // and back
        y=glm::mix(y,80.f,smooth(27.6f,28.4f,t));      // a look over the shoulder as the doors close
        y=glm::mix(y,-90.f,smooth(29.f,30.f,t));
        return y;
    }
    float pitch() const {
        float t=time;
        if(t<4)return glm::mix(15.f,10.f,smooth(0,4,t));
        if(t<8)return glm::mix(6.f,-2.f,smooth(4.f,7.f,t));
        float p=-2;
        p=glm::mix(p,-4.f,smooth(8.6f,10.f,t));
        p=glm::mix(p,10.f,smooth(12.4f,13.6f,t));      // the beacon's glow coming down the shaft
        p=glm::mix(p,-38.f,smooth(14.f,15.f,t));       // the plain falling away
        p=glm::mix(p,24.f,smooth(15.6f,17.f,t));       // the laboratory hanging overhead
        p=glm::mix(p,0.f,smooth(19.3f,20.6f,t));
        p=glm::mix(p,-88.f,smooth(22.8f,24.f,t));      // straight down through the glass floor
        p=glm::mix(p,0.f,smooth(25.2f,26.4f,t));
        p=glm::mix(p,-6.f,smooth(27.6f,28.4f,t));
        p=glm::mix(p,0.f,smooth(29.f,30.f,t));
        return p;
    }
    // Where play begins: just inside the ward, facing into it
    static glm::vec3 handoff() {return {0.f,0.f,35.8f};}

    // ---- words ----
    const char* header() const {return time<14.5f?"THE FAR ROAD / SERVICE TOWER":time<21.f?"THE LIVING WARD":"THE LIVING WARD / ADMISSIONS";}
    const char* caption() const {
        float t=time;
        if(t>=8.7f && t<13.6f)return "SERVICE LIFT / ASCENDING";
        if(t>=15.f && t<20.f)return "ALTITUDE: UNREGISTERED";
        if(t>=21.2f && t<23.2f)return "ARRIVAL / ADMISSIONS";
        if(t>=23.3f && t<26.5f)return "FLOOR CONTACT: CONFIRMED";
        if(t>=29.6f)return "LEGACY CHASSIS / MANUAL ASSESSMENT";
        return "";
    }
    // The public address (assets/sfx/lab_pa_N.wav) and the lift's own sounds, each at its moment
    struct Cue {float at;const char* sound;int volume;};
    static const Cue* cues(int& count){
        static const Cue c[]={{7.7f,"door_close",90},{8.7f,"lab_pa_5",110},{8.8f,"clank",100},{9.f,"ab_rumble",80},
            {13.7f,"ab_rumble",95},{20.6f,"clank",105},{21.f,"door",95},{21.3f,"lab_pa_6",110},{23.3f,"lab_pa_2",110},
            {28.f,"door_close",100},{29.6f,"lab_pa_0",110}};
        count=(int)(sizeof(c)/sizeof(c[0]));return c;
    }
};

// The observation theatre in Admissions (it was the old arrival scene): once you walk up to
// its glass, the subject inside lifts off the floor while its shadow keeps walking, and the
// window behind it turns to water. One clock; -1 until you come near.
struct LabTheatre {
    float time=-1.f;
    bool started() const {return time>=0;}
    static float smooth(float a,float b,float t){return LabIntro::smooth(a,b,t);}
    float rise() const {return started()?smooth(1.5f,6.5f,time)*1.35f:0.f;}
    float windowChange() const {return started()?smooth(6.5f,10.5f,time):0.f;}
    bool floating() const {return started() && time>=1.5f;}
};
