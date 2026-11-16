#pragma once
// Shared geometry/timing for simulation, collision and the colossus rig.
#include "core/Collision.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
namespace shephead {
constexpr float HP = 3000.f, HAND_MULT = 1.8f, BODY_MULT = 0.75f;
constexpr float RISE = 4.2f, OPEN = 5.f, REAR = 2.4f;
constexpr float SLAM_TELL = 1.65f, SLAM_LOCK = 0.85f, SLAM_R = 7.f, SLAM_DAMAGE = 32.f, HAND_DOWN = 4.8f;
constexpr float BEAM_TELL = 1.4f, BEAM_SPEED = 44.f, BEAM_HALF = 1.5f, BEAM_DAMAGE = 25.f;
constexpr float AIM_WARN = 4.f, AIM_FLASH = 1.f;
constexpr float PULL_LEN = 3.2f, PULL_SPEED = 7.f;
constexpr float SCALE = 64.f, HW = 27.f, HH = 42.f, HD = 38.f;
constexpr float PIN_DAMAGE = 150.f, PIN_TIME = 5.5f, PARRY_DAMAGE = 120.f;
constexpr int SHOT_OWNER = -606;
enum Atk { NONE, SLAM, SLAM2, BEAM, AIM, ORRERY, PULL, ECHO, SWEEP, GRAB, GAZE };
constexpr float SWEEP_TELL=1.25f, SWEEP_TRAVEL=1.05f, SWEEP_HALF=8.f;
constexpr float GRAB_LOCK=.65f, GRAB_REACH=1.45f, GRAB_CLOSE=1.85f, GRAB_ESCAPE=2.3f, GRAB_BREAK=90.f;
constexpr float GAZE_TRACK=1.25f, GAZE_LOCK=1.65f, GAZE_FIRE=2.05f, GAZE_END=3.3f;
inline int gazeIndex(float t,int variation){return variation&&t>=GAZE_FIRE?std::min(2,int((t-GAZE_FIRE)/.6f)+1):0;}
inline float sweepX(float t,int side,float x0,float x1){
    float u=std::clamp((t-SWEEP_TELL)/SWEEP_TRAVEL,0.f,1.f);
    return glm::mix(side?x1:x0,side?x0:x1,u);
}
inline glm::vec3 grabPath(glm::vec3 start,glm::vec3 at,float t){
    float u=std::clamp((t-GRAB_LOCK)/(GRAB_REACH-GRAB_LOCK),0.f,1.f);u=u*u*(3.f-2.f*u);
    return glm::mix(start,at+glm::vec3{0,3,0},u)+glm::vec3{0,18.f*std::sin(3.14159265f*u),0};
}
inline float segmentDistance(glm::vec2 p,glm::vec2 a,glm::vec2 b){auto d=b-a;
    return glm::length(p-(a+d*glm::clamp(glm::dot(p-a,d)/std::max(.001f,glm::dot(d,d)),0.f,1.f)));}
inline bool sweepHits(glm::vec3 feet,float height,float xA,float xB,float z,float floor){
    return feet.x>=std::min(xA,xB)-11.f&&feet.x<=std::max(xA,xB)+11.f&&std::fabs(feet.z-z)<SWEEP_HALF
        &&feet.y<floor+15.f&&feet.y+height>floor;
}
inline bool grabHits(glm::vec3 feet,float height,glm::vec3 at){
    return glm::length(glm::vec2(feet.x-at.x,feet.z-at.z))<7.f&&feet.y<at.y+13.f&&feet.y+height>at.y;
}
inline float smooth(float x) { x = std::clamp(x, 0.f, 1.f); return x*x*(3.f-2.f*x); }
inline glm::vec3 headC(float rise) { return glm::mix(glm::vec3{-600.f, 105.f, -4140.f}, glm::vec3{-600.f, 255.f, -4135.f}, smooth(rise)); }
inline glm::vec3 chest(float rise) { return headC(rise) + glm::vec3{0.f,-93.f,-18.f}; }
inline glm::vec3 shoulder(int k, float rise) { return chest(rise) + glm::vec3{k?61.f:-61.f,30.f,0.f}; }
inline glm::vec3 handRest(int k, float rise) { return headC(rise) + glm::vec3{k?78.f:-84.f,k?-48.f:-70.f,k?38.f:42.f}; }
constexpr float UPPER_ARM=85.f, FOREARM=82.f;
inline glm::vec3 elbowFrom(int side,glm::vec3 root,glm::vec3 hand){
    const auto delta=hand-root;const float distance=std::max(.001f,glm::length(delta));const auto direction=delta/distance;
    // Two-bone proportions stay consistent. Extreme platform-edge punches
    // stretch both segments equally instead of collapsing one into a stub.
    const float stretch=std::max(1.f,distance/(UPPER_ARM+FOREARM)*1.001f);
    const float a=UPPER_ARM*stretch,b=FOREARM*stretch,along=(a*a-b*b+distance*distance)/(2.f*distance);
    glm::vec3 pole{side?.8f:-.8f,-.7f,-.15f};pole-=direction*glm::dot(pole,direction);
    if(glm::length(pole)<.001f)pole=glm::cross(direction,glm::vec3{1,0,0});
    return root+direction*along+glm::normalize(pole)*std::sqrt(std::max(0.f,a*a-along*along));
}
inline glm::vec3 elbow(int side,float rise,glm::vec3 hand){return elbowFrom(side,shoulder(side,rise),hand);}
inline glm::vec3 mouthAt(float rise) { return headC(rise) + glm::vec3{0.f,-16.f,29.f}; }
inline glm::mat4 facePose(float rise, float clock, float gazeX, float rear, bool dead) {
    const glm::vec3 c = headC(rise) + glm::vec3{0.f,1.2f*std::sin(clock*.47f)+(rear>=0.f?8.f*std::sin(3.14159265f*std::clamp(rear/REAR,0.f,1.f)):0.f),0.f};
    auto m=glm::translate(glm::mat4{1.f},c);
    m=glm::rotate(m,std::clamp((gazeX+600.f)/180.f,-.18f,.18f),glm::vec3{0,1,0});
    m=glm::rotate(m,.18f,glm::vec3{1,0,0});
    return glm::rotate(m,dead?.23f:.015f*std::sin(clock*.3f),glm::vec3{0,0,1});
}
inline float hitBody(glm::vec3 from,glm::vec3 dir,const AABB& box) {
    // A sword or muzzle can already be inside a colossal palm's volume.
    if(from.x>=box.min.x&&from.x<=box.max.x&&from.y>=box.min.y&&from.y<=box.max.y&&from.z>=box.min.z&&from.z<=box.max.z)return .001f;
    return rayBoxHit(from,dir,box);
}
inline AABB handBox(glm::vec3 at) { return {at+glm::vec3{-8.5f,0.f,-16.f},at+glm::vec3{8.5f,6.f,16.f}}; }
inline AABB chestBox(float rise) { auto c=chest(rise); return {c-glm::vec3{42.f,32.f,20.f},c+glm::vec3{42.f,32.f,20.f}}; }
inline bool slamHits(glm::vec3 p, glm::vec3 at) { return glm::length(glm::vec2{p.x-at.x,p.z-at.z})<SLAM_R && p.y < at.y+10.f && p.y > at.y-3.f; }
inline Atk nextAttack(int phase,int cycle) {
    constexpr Atk a[]={SLAM,SWEEP,GRAB,ORRERY,GAZE,SWEEP,SLAM2};
    constexpr Atk b[]={ECHO,GRAB,SWEEP,GAZE,SLAM2,ORRERY,SWEEP,GRAB};
    constexpr Atk c[]={PULL,SWEEP,GRAB,SLAM2,GAZE,ECHO,SWEEP,ORRERY,GRAB};
    return phase==0?a[cycle%7]:phase==1?b[cycle%8]:c[cycle%9];
}
inline float checkpointHp(float hp) { return hp<=0.f?0.f:hp>HP*2.f/3.f?HP:hp>HP/3.f?HP*2.f/3.f:HP/3.f; }
// THREE CAPTIVE MOONS: low enough to fight with the whole arsenal, always exposed.
inline glm::vec3 moon(int i,float clock) {
    const float a=clock*.32f+6.2831853f*i/3.f;
    return {630.f+36.f*std::cos(a),6.5f+2.f*std::sin(a*2.f),-3398.f+30.f*std::sin(a)};
}
constexpr float MOON_RADIUS=3.4f;
}
