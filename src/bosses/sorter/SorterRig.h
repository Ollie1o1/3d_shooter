#pragma once
#include "render/EnemyModel.h"
#include "ui/PixelFont.h"

// Mandatory combat geometry: the press, pistons, claw and projected warnings
// stay visible at every ambient detail setting.
namespace sorterrig {
using glm::vec3;using namespace rig;
inline void beam(Rig& r,vec3 a,vec3 b,float width,vec3 color,vec3 glow=vec3{0}) {
    vec3 d=b-a;float n=glm::length(d);if(n<.01f)return;
    vec3 z=d/n,x=glm::cross(vec3{0,1,0},z);
    x=glm::length(x)>.01f?glm::normalize(x):vec3{1,0,0};
    vec3 y=glm::normalize(glm::cross(z,x));
    glm::mat4 m(1);m[0]=glm::vec4(x*width,0);m[1]=glm::vec4(y*width,0);
    m[2]=glm::vec4(z*n,0);m[3]=glm::vec4((a+b)*.5f,1);r.part(m,color,glow);
}
inline void warning(Rig& r,vec3 at,float radius,vec3 color,bool stamp) {
    for(int k=0;k<48;++k) {
        float a=k*6.2831853f/48.f;
        r.box(T(at+vec3{std::cos(a)*radius,.07f,std::sin(a)*radius})*RY(-a),{0,0,0},{.32f,.07f,.12f},color*.35f,color);
    }
    if(stamp)for(int k=0;k<4;++k) {
        float a=k*1.5707963f+.785398f;
        vec3 p=at+vec3{std::cos(a)*radius,0,std::sin(a)*radius};
        // Projector traces reach eye height; looking forward still shows the warning.
        for(int j=0;j<5;++j)r.box(T(p+vec3{0,.35f+j*.45f,0}),{0,0,0},{.06f,.22f,.06f},color*.3f,color);
        r.box(T(p+vec3{0,1.7f,0}),{0,0,0},{.24f,.10f,.24f},color,color);
    }
}
inline int flameMesh() {
    static int id=[](){
        meshlib::MeshData mesh;mesh.size={1,1,1};
        for(int k=0;k<6;++k) {
            float a=k*6.2831853f/6.f,b=(k+1)*6.2831853f/6.f;
            vec3 v[3]={{std::cos(a)*.5f,-.5f,std::sin(a)*.5f},{0,.5f,0},{std::cos(b)*.5f,-.5f,std::sin(b)*.5f}};
            vec3 normal=glm::normalize(glm::cross(v[1]-v[0],v[2]-v[0]));
            for(auto p:v){mesh.idx.push_back((uint32_t)mesh.pos.size());mesh.pos.push_back(p);mesh.nrm.push_back(normal);}
        }
        return meshlib::addShared(std::move(mesh));
    }();return id;
}
inline void press(Rig& r,vec3 at,float top,float height,vec3 lamps) {
    vec3 head=at+vec3{0,height,0};
    const vec3 iron{.22f,.25f,.27f},steel{.5f,.53f,.54f},paint{.68f,.43f,.1f};
    // Both axes join the room's existing overhead gantry, including its carriage.
    r.box(T({82,top,at.z}),{0,0,0},{52,.45f,1.1f},iron);
    r.box(T({at.x,top-.5f,at.z}),{0,0,0},{4.2f,.65f,2.5f},paint);
    float base=top-.85f,end=head.y+.55f;
    for(float side:{-1.f,1.f}) {
        vec3 upper{at.x+side*1.35f,base,at.z},lower{at.x+side*1.35f,end,at.z};
        beam(r,upper,lower,.26f,steel);
        beam(r,upper,glm::mix(upper,lower,.55f),.52f,iron);
        r.box(T(head),{side*1.7f,.65f,0},{.45f,.5f,.8f},paint);
    }
    r.box(T(head),{0,0,0},{4.4f,.8f,4.4f},iron);
    // An octagonal shoe fills the warning circle instead of a small floating plate.
    for(int k=0;k<8;++k) {
        float a=k*.785398f;
        auto xf=T(head+vec3{std::cos(a)*2.15f,0,std::sin(a)*2.15f})*RY(-a);
        r.box(xf,{0,0,0},{1.7f,.8f,2.2f},iron);
        r.box(xf,{.72f,.06f,0},{.12f,.45f,1.8f},paint);
        r.box(xf,{.81f,.31f,0},{.08f,.1f,.55f},paint,lamps);
    }
    // Teeth meet the floor exactly when the damage event fires (head height .4).
    for(float x:{-1.5f,-.5f,.5f,1.5f})r.box(T(head),{x,-.34f,0},{.38f,.12f,3.8f},steel);
    const char* label="REJECT";
    for(int i=0;label[i];++i)for(int y=0;y<7;++y)for(int x=0;x<5;++x)
        if(PIXEL_FONT[label[i]-32][y]&(1<<(4-x)))
            r.box(T(head),{-1.42f+(i*6+x)*.08f,.25f-y*.07f,2.22f},{.065f,.055f,.025f},paint);
}
inline void claw(Rig& r,const Enemy& e,vec3 at,float closed) {
    vec3 wrist=at+vec3{0,1.2f,0};
    vec3 shoulder=e.position+vec3{std::cos(e.yaw)*2.5f,3.8f,-std::sin(e.yaw)*2.5f};
    vec3 elbow=glm::mix(shoulder,wrist,.5f)+vec3{0,2.4f,0};
    beam(r,shoulder,elbow,.45f,{.3f,.33f,.35f});beam(r,elbow,wrist,.25f,{.55f,.58f,.59f});
    r.box(T(wrist),{0,0,0},{1.2f,.5f,1.1f},{.25f,.28f,.3f});
    for(float s:{-1.f,1.f}) {
        float spread=1.4f-closed*.8f;
        r.box(T(wrist),{s*spread,-.12f,.45f},{.25f,.65f,1.8f},{.6f,.62f,.6f},vec3{.55f});
        r.box(T(wrist),{s*(spread-.25f),-.35f,1.2f},{.65f,.2f,.35f},{.65f,.65f,.6f},vec3{.75f});
    }
}
}
