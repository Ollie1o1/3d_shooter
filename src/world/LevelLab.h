#pragma once
#include "world/Level.h"
#include "ui/PixelFont.h"

namespace lab {
constexpr int ROOMS=10;
inline glm::vec3 centre(int i){return {0,-14.f*i,-120.f*i};}
inline float routeX(int i){static constexpr float x[]={-18,18,-22,22,-18,18,-22,20,-18,0};return x[std::clamp(i,0,9)];}
inline float entryX(int i){return i?-routeX(i-1)*.5f:0.f;}
inline float roofHeight(int i){static constexpr float h[]={12,16,20,26,32,38,48,64,72,110};return h[std::clamp(i,0,9)];}
struct Bay {float nearZ,farZ,left,right;};
// Each footprint is a connected sequence of bays. The changes in width and
// offset are real floor/wall boundaries, rather than scenery inside a box.
inline std::vector<Bay> footprint(int i){
    switch(i){
    case 0:return {{40,-40,-32,32}};
    // Admissions' lower flight spans X=3..15. The receiving bay includes
    // the full corridor and a solid right jamb, not just its centre line.
    case 1:return {{40,10,-36,16},{10,-8,-36,36},{-8,-40,-10,36}};
    case 2:return {{40,22,-30,30},{22,-28,-42,42},{-28,-40,-30,30}};
    case 3:return {{40,22,-30,42},{22,-28,-42,42},{-28,-40,-42,30}};
    case 4:return {{40,16,-32,42},{16,-16,-48,48},{-16,-40,-42,42}};
    case 5:return {{40,24,-28,42},{24,-24,-42,42},{-24,-40,-42,30}};
    case 6:return {{40,22,-30,30},{22,-28,-42,42},{-28,-40,-30,30}};
    case 7:return {{40,22,-36,48},{22,-12,-54,54},{-12,-32,-48,48},{-32,-40,-42,42}};
    case 8:return {{40,18,-48,36},{18,-10,-56,44},{-10,-32,-44,56},{-32,-40,-42,42}};
    default:return {{40,32,-44,44},{32,-48,-76,76},{-48,-72,-54,54}};
    }
}
// Windows onto the void in the enclosed wards, in room coordinates. `x` is the
// wall's centre line (its sign is the side); z0..z1 and y0..y1 are the opening.
// Every outer side wall is glass from just off the floor to just under the roof,
// in panes of up to ~9 m between slim piers: the ward stands in the void and you
// see it from everywhere in the room. Each pane keeps a hidden collision panel, a
// glass quad and a deep frame. The garden (4) keeps its own walls and sky; the
// surgical theatre's east bay keeps its original observation glass.
struct Window {float x,z0,z1,y0,y1;};
inline std::vector<Window> windows(int i){
    std::vector<Window> w;
    if(i==4 || i>=7)return w;
    const float roof=roofHeight(i),y0=.7f,y1=roof-1.1f;
    for(const auto& bay:footprint(i))for(float x:{bay.left,bay.right}){
        if(i==3 && x==42 && bay.farZ<14 && bay.nearZ>-14)continue;
        float z0=bay.farZ+1.f,z1=bay.nearZ-1.f,len=z1-z0,pier=.9f;
        int n=std::max(1,(int)std::ceil(len/9.5f));float pane=(len-(n-1)*pier)/n;
        for(int k=0;k<n;++k){float a=z0+k*(pane+pier);w.push_back({x,a,a+pane,y0,y1});}
    }
    return w;
}
// Motor Assessment's floor has a glass plate looking straight down into the void
inline AABB glassFloor(){return AABB{{5,-1,24},{17,0,36}};}
// Admissions is reached by a service lift: its car sits outside the entry wall
constexpr float LIFT_HALF=2.2f,LIFT_DOOR_H=3.4f;
// A descent has two flights separated by a lateral landing. Late flights
// become exposed bridges; early ones have a continuous sloping ceiling.
inline std::vector<glm::vec3> passageRoute(int i){
    auto c=centre(i);float x=routeX(i),next=entryX(i+1);
    return {c+glm::vec3{x,0,-38},c+glm::vec3{x,-7,-59},
            c+glm::vec3{next,-7,-61},centre(i+1)+glm::vec3{next,0,37},
            centre(i+1)+glm::vec3{next,0,31}};
}
inline glm::vec3 contactPosition(int room,int side=-1){
    return centre(room)+(room==2?glm::vec3{31,7.5f,-17}:room==6?glm::vec3{-31,7.5f,-17}:glm::vec3{side*20.f,1.5f,-18});
}
inline glm::vec3 motorLoadAt(){return centre(2)+glm::vec3{0,0,-18};}
inline glm::vec3 motorLoadControl(){return centre(2)+glm::vec3{-25,7.4f,-13};}
inline const char* name(int i){static const char* n[]={"ADMISSIONS","RECOVERY WARD","MOTOR ASSESSMENT","SURGICAL OBSERVATION","THE CONSERVATORY","LOAD TESTING","LONG-TERM CARE","CELESTIAL OBSERVATION","HADAL RESEARCH","FINAL ASSESSMENT"};return n[std::clamp(i,0,9)];}
inline const char* objective(int i){static const char* n[]={"ENTER THE WARD","REACH THE OBSERVATION GALLERY","RELEASE THE MOTOR LOCK","CROSS THE SURGICAL FLOOR","FIND THE SERVICE DOOR","CROSS THE LOAD BRIDGE","RELEASE THE SUSPENSION LOCK","ROUTE THE OBSERVATION BRIDGE","SEAT THE HADAL CONTACT","DEFEAT THE SUCCESSOR"};return n[std::clamp(i,0,9)];}
inline Theme theme(int i){
    Theme t{{.1f,.17f,.22f},{.22f,.35f,.4f},{.1f,.15f,.18f},glm::normalize(glm::vec3{.2f,.8f,-.3f}),{.8f,.85f,.9f},.025f,0,{.06f,.1f,.15f},.1f,glm::normalize(glm::vec3{.3f,-.8f,.4f}),{.65f,.72f,.8f},{.6f,.66f,.7f},{.3f,.34f,.37f},{.2f,.3f,.36f},.002f};
    if(i==4){t.zenith={.22f,.48f,.66f};t.horizon={.7f,.8f,.72f};t.sunColor={1.2f,1.05f,.7f};t.skyAmb={.6f,.67f,.52f};t.groundAmb={.24f,.32f,.2f};}
    if(i==7||i==9){t.zenith={.05f,.025f,.16f};t.horizon={.22f,.06f,.25f};t.stars=.95f;t.fogColor={.09f,.07f,.19f};}
    if(i==8){t.zenith={.008f,.04f,.09f};t.horizon={.01f,.12f,.2f};t.fogColor={.015f,.09f,.14f};t.stars=0;}
    // The enclosed wards look out on the void too: their distance takes its violet
    if(i<7 && i!=4){t.fogColor={.07f,.065f,.15f};t.stars=.9f;}
    return t;
}
// Chapter 2's sky over the Far Road (LevelWasteland.h's rooms): where the arrival begins
inline Theme ashTheme(){
    return Theme{{.14f,.055f,.065f},{.55f,.23f,.14f},{.24f,.18f,.15f},
        glm::normalize(glm::vec3{.3f,.28f,-.8f}),{1.3f,.72f,.3f},.035f,.015f,
        {.19f,.09f,.075f},.05f,glm::normalize(glm::vec3{-.3f,-.7f,.35f}),{.9f,.72f,.58f},
        {.37f,.31f,.28f},{.2f,.17f,.14f},{.3f,.2f,.17f},.0038f};
}
// Stencils are actual small surfaces and do not consume a light per letter.
inline void stencil(LevelBuilder& b,const char* text,glm::vec3 p,float px,glm::vec3 color={.1f,.24f,.28f}){
    for(int i=0;text[i];++i){unsigned char ch=text[i];if(ch<32||ch>127)continue;
        for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(PIXEL_FONT[ch-32][y]&(1<<(4-x))){
            auto q=p+glm::vec3{(i*6+x)*px,-y*px,0};b.prop(q.x,q.y,q.z,q.x+px*.8f,q.y+px*.8f,q.z+.035f,color);
        }
    }
}

// Test apparatus occupies the combat floor, rather than decorating only the
// walls. Casings stop shots and can be climbed; the low circuit decks link
// floor, apparatus and observation galleries without sealing off the room.
inline void combatStations(LevelBuilder& b,int room,glm::vec3 c) {
    using glm::vec3;
    if(room<2 || room==4 || room>8)return;
    const vec3 shell{.65f,.72f,.71f},frame{.18f,.28f,.31f},inset{.055f,.11f,.14f},
        edge{.27f,.95f,1.05f},paint{.75f,.68f,.42f};
    auto box=[&](vec3 a,vec3 z,vec3 color){b.wall(a.x,a.y,a.z,z.x,z.y,z.z,color);};
    auto detail=[&](vec3 a,vec3 z,vec3 color){b.prop(a.x,a.y,a.z,z.x,z.y,z.z,color);};
    auto glow=[&](vec3 a,vec3 z,vec3 color){b.neon(a.x,a.y,a.z,z.x,z.y,z.z,color);};
    auto station=[&](vec3 local,const char* label) {
        vec3 p=c+local;b.mat=Mat::CLINIC;
        box(p+vec3{-3.5f,0,-2.5f},p+vec3{3.5f,2.4f,2.5f},shell);
        b.mat=Mat::METAL;
        box(p+vec3{-4.8f,0,.3f},p+vec3{-3.5f,1.1f,2.5f},frame);
        detail(p+vec3{-3.4f,2.4f,-2.4f},p+vec3{3.4f,2.46f,2.4f},frame);
        detail(p+vec3{-3.2f,.35f,2.51f},p+vec3{3.2f,1.9f,2.55f},inset);
        for(int k=0;k<5;++k)detail(p+vec3{-2.9f,.5f+k*.22f,2.56f},p+vec3{-.9f,.58f+k*.22f,2.6f},frame);
        glow(p+vec3{.4f,.65f,2.56f},p+vec3{2.7f,1.35f,2.61f},edge*.35f);
        glow(p+vec3{-3.45f,2.47f,-2.3f},p+vec3{-3.35f,2.51f,2.3f},edge*.6f);
        // Curved diagnostic hoops give the machines a clinical silhouette,
        // contrasting with the ward's long rectangular galleries.
        b.kit().arch(p+vec3{0,2.4f,-1.8f},6.4f,4.1f,.48f,.7f,shell,0,20);
        b.kit(true).arch(p+vec3{0,2.6f,-1.79f},5.65f,3.65f,.06f,.5f,edge*.45f,0,20);
        b.kit().column(p+vec3{0,2.47f,-.4f},1.25f,.06f,inset,20);
        b.kit(true).curve(p+vec3{0,2.54f,-.4f},1.3f,0,6.2831853f,0,.03f,.05f,edge*.5f,24);
        stencil(b,label,p+vec3{-3.1f,1.85f,2.62f},.055f,{.6f,.76f,.76f});
        for(float z:{-3.2f,3.2f})detail(p+vec3{-4.8f,.01f,z},p+vec3{4.8f,.025f,z+.18f},frame);
        for(float x:{-4.8f,4.6f})detail(p+vec3{x,.01f,-3.2f},p+vec3{x+.18f,.025f,3.4f},frame);
        for(float x=-4.f;x<4;x+=1.f)detail(p+vec3{x,.028f,3.45f},p+vec3{x+.5f,.035f,3.62f},paint);
    };
    if(room==2 || room==6) {
        float z=room==2?6.f:15.f;
        for(float x:{-17.f,17.f}) {
            station({x,0,z},room==2?"MOTOR / RESPONSE":"SUSPENSION / MONITOR");
            AABB ramp=LevelBuilder::aabb(x-3,c.y,c.z+z+2.5f,x+3,c.y+2.4f,c.z+z+14.5f);
            b.L.ramps.push_back({ramp,2,c.y+2.4f,c.y});
            b.kit().box(c+vec3{x,1.05f,z+8.5f},{std::hypot(12.f,2.4f),.3f,6},frame,-1.5707963f,0,-.19739556f);
            glow(c+vec3{x-3,2.42f,z+2.5f},c+vec3{x+3,2.48f,z+2.62f},edge*.4f);
        }
        box(c+vec3{-13.5f,2.1f,z-2},c+vec3{13.5f,2.4f,z+2},frame);
        for(float dz:{z-1.9f,z+1.8f})glow(c+vec3{-13.4f,2.42f,dz},c+vec3{13.4f,2.46f,dz+.08f},edge*.4f);
        // A dark assessment runway breaks up the blank tile at the entrance.
        // Flush paint leaves the original walking line and floor unchanged.
        detail(c+vec3{-5,.008f,-27},c+vec3{5,.016f,22},inset*.85f);
        for(float dz=-25;dz<22;dz+=3.f)for(float x:{-4.9f,4.7f})
            detail(c+vec3{x,.018f,dz},c+vec3{x+.18f,.026f,dz+1.2f},paint);
        for(float dz:{-23.f,-3.f,19.f})for(int k=0;k<9;++k)
            detail(c+vec3{-4.5f+k,.019f,dz},c+vec3{-4.05f+k,.027f,dz+.12f},shell);
        if(room==2) {
            // Freestanding diagnostic banks break the long diagonal sightlines.
            // Both ends stay open, and low steps make their roofs fighting space.
            for(float side:{-1.f,1.f})for(float dz:{-26.f,-2.f,25.f}) {
                vec3 p=c+vec3{side*22,0,dz};b.mat=Mat::CLINIC;
                box(p+vec3{-6,0,-2},p+vec3{6,2.4f,2},shell);
                b.mat=Mat::METAL;
                box(p+vec3{-5,0,2},p+vec3{5,1.1f,3.2f},frame);
                detail(p+vec3{-5.7f,.45f,-2.04f},p+vec3{5.7f,1.85f,-2.01f},inset);
                for(float x:{-5.8f,5.7f})glow(p+vec3{x,2.42f,-1.9f},p+vec3{x+.1f,2.48f,1.9f},{.65f,.4f,.12f});
                glow(p+vec3{-5.8f,2.42f,1.9f},p+vec3{5.8f,2.48f,2.f},{.65f,.4f,.12f});
            }
            for(vec3 local:{vec3{-4,0,-30},vec3{4,0,-30},vec3{0,0,15},vec3{0,0,-6},vec3{0,0,24}}) {
                const auto p=c+local;b.mat=Mat::CLINIC;
                box(p+vec3{-2.5f,0,-1.2f},p+vec3{2.5f,1.6f,1.2f},shell);
                glow(p+vec3{-2.4f,1.62f,1.1f},p+vec3{2.4f,1.68f,1.2f},edge*.35f);
                detail(p+vec3{-2.2f,.4f,1.21f},p+vec3{2.2f,1.25f,1.25f},inset);
            }
            // The suspended calibration load can be released from the gallery.
            const auto target=motorLoadAt();
            b.L.story.motorLoad=b.mover(target+vec3{0,13.2f,0},{4,1.2f,4},Mover::Path::DRIVEN,
                {0,0,0},{0,-12,0},1,0,{.18f,.1f,.025f});
            for(float x:{-4.7f,4.7f})glow(target+vec3{x,.03f,-4.7f},target+vec3{x+.12f,.06f,4.8f},{.7f,.4f,.1f});
            for(float dz:{-4.7f,4.7f})glow(target+vec3{-4.7f,.03f,dz},target+vec3{4.8f,.06f,dz+.12f},{.7f,.4f,.1f});
            stencil(b,"LOAD DROP / KEEP CLEAR",target+vec3{-4.7f,.04f,5.3f},.065f,paint);
            // A cyan service record is visible from the west gallery. Reaching
            // the new bank roof is optional; its discovery uses the normal save.
            for(float dz:{-27.7f,-24.3f})glow(c+vec3{-27.7f,2.44f,dz},c+vec3{-16.3f,2.5f,dz+.1f},edge*.25f);
        }
    } else if(room==3) {
        station({-19,0,6},"SURGICAL / PREPARATION");station({8,0,-16},"STERILE / INSTRUMENTS");
    } else if(room==5) {
        station({-29,0,0},"LOAD / WEST");station({29,0,0},"LOAD / EAST");
    } else if(room==7)station({30,0,3},"EXTERIOR / UNREGISTERED");
    else if(room==8)station({-28,0,5},"PRESSURE / HOLDING");
}
}

#include "world/LevelSuccessor.h"
#include "world/LabDressing.h"

inline LevelData buildLab(){
    using glm::vec3;using W=WaveEntry;
    LevelData l;LevelBuilder b{l};auto& s=l.story;
    s.built=true;s.chapter=3;s.nextChapter=4;s.title="THE LIVING WARD";s.nextName="THE BLOOD MOON PLAIN";
    s.startWeapons=15;s.startGrapple=true;s.bossSection=9;s.pitSection=-1;
    const vec3 cream{.71f,.75f,.72f},wall{.53f,.64f,.65f},steel{.19f,.29f,.33f},cyan{.25f,.9f,1.f},warm{1.1f,.85f,.5f};
    auto solid=[&](vec3 a,vec3 z,vec3 c){return b.wall(a.x,a.y,a.z,z.x,z.y,z.z,c);};
    auto prop=[&](vec3 a,vec3 z,vec3 c){return b.prop(a.x,a.y,a.z,z.x,z.y,z.z,c);};
    auto bed=[&](vec3 p,int number){
        b.mat=Mat::METAL;solid(p+vec3{-1,0,-2},p+vec3{1,.65f,2},steel);
        prop(p+vec3{-.95f,.65f,-1.85f},p+vec3{.95f,.95f,1.85f},{.82f,.84f,.76f});
        prop(p+vec3{-.75f,.96f,-1.6f},p+vec3{.75f,1.2f,-.7f},{.9f,.91f,.86f});
        for(float x:{-1.12f,1.02f})prop(p+vec3{x,.8f,-1.4f},p+vec3{x+.1f,1.35f,1.4f},steel);
        char n[32];std::snprintf(n,sizeof(n),"SUBJECT %02d",number);lab::stencil(b,n,p+vec3{-1,1.8f,2.05f},.055f);
    };
    auto console=[&](vec3 p,const char* title,const char* line){
        solid(p+vec3{-.8f,0,-.45f},p+vec3{.8f,1.1f,.45f},steel);
        b.neon(p.x-.6f,p.y+1.2f,p.z+.48f,p.x+.6f,p.y+1.65f,p.z+.5f,cyan*.4f);
        s.terminals.push_back({p,title,line});
    };
    auto trolley=[&](vec3 p,bool damaged){
        b.mat=Mat::METAL;
        for(float dx:{-.7f,.7f})for(float dz:{-1.1f,1.1f}){
            prop(p+vec3{dx-.08f,.1f,dz-.08f},p+vec3{dx+.08f,1.25f,dz+.08f},steel);
            b.kit().box(p+vec3{dx,.12f,dz},{.12f,.24f,.28f},steel,0,0,damaged&&dx>0?.45f:0.f);
        }
        prop(p+vec3{-.9f,.45f,-1.3f},p+vec3{.9f,.52f,1.3f},cream);
        prop(p+vec3{-.9f,1.15f,-1.3f},p+vec3{.9f,1.3f,1.3f},cream);
        prop(p+vec3{-.55f,1.3f,-.6f},p+vec3{.5f,1.39f,.4f},steel);
        prop(p+vec3{-.3f,1.39f,-.2f},p+vec3{.1f,1.7f,.15f},{.62f,.76f,.72f});
        lab::stencil(b,"C-17",p+vec3{-.6f,.96f,1.31f},.07f);
        if(damaged)for(int k=0;k<3;++k)prop(p+vec3{-.65f+k*.35f,.75f,1.315f},p+vec3{-.4f+k*.35f,.81f,1.33f},{.12f,.23f,.25f});
    };
    auto ramp=[&](float x,float za,float zb,float ya,float yb,bool covered){
        AABB a=LevelBuilder::aabb(x-6,std::min(ya,yb)-1,za,x+6,std::max(ya,yb)+8,zb);
        l.ramps.push_back({a,2,yb,ya});
        float angle=std::atan2(ya-yb,za-zb);vec3 c{x,(ya+yb)*.5f-.3f,(za+zb)*.5f};
        b.mat=Mat::METAL;b.kit().box(c,{std::hypot(za-zb,ya-yb),.6f,12},steel,-1.5707963f,0,angle);
        // Follow the flight instead of a solid wall rising from its lowest end.
        for(int k=0;k<8;++k){
            float t=k/8.f,t1=(k+1)/8.f,hi=glm::mix(ya,yb,t),lo=glm::mix(ya,yb,t1);
            float near=glm::mix(za,zb,t),far=glm::mix(za,zb,t1);
            for(float side:{-1.f,1.f})solid({x+side*6-.18f,lo-.3f,far},{x+side*6+.18f,hi+(covered?7.6f:1.15f),near},covered?wall:steel);
            if(covered)solid({x-6,lo+7.6f,far},{x+6,hi+8,near},cream);
            for(float side:{-1.f,1.f})prop({x+side*5.5f-.06f,hi+.04f,far},{x+side*5.5f+.06f,hi+.08f,near},cyan*.55f);
        }
    };
    for(int i=0;i<10;++i){
        const vec3 c=lab::centre(i);const float y=c.y,z=c.z;
        if(i==9){buildFinalAssessment(b,c,lab::entryX(i));labdress::dressRoom(b,i,c,lab::footprint(i),lab::roofHeight(i),lab::entryX(i),0);continue;}
        const auto bays=lab::footprint(i);float half=0;
        for(const auto& bay:bays)half=std::max({half,-bay.left,bay.right});
        const float entry=lab::entryX(i),exit=lab::routeX(i),roof=lab::roofHeight(i);
        b.mat=Mat::CLINIC;
        l.basins.push_back({LevelBuilder::aabb(-half-1,y-300,z-42,half+1,y+roof+1,z+42),y-250});
        // Clip all floor slabs to the authored footprint, including bridge banks.
        auto floor=[&](float left,float far,float right,float near,vec3 color){
            for(const auto& bay:bays){
                float x0=std::max(left,bay.left),x1=std::min(right,bay.right);
                float z0=std::max(far,bay.farZ),z1=std::min(near,bay.nearZ);
                if(x1>x0 && z1>z0)solid(c+vec3{x0,-.8f,z0},c+vec3{x1,0,z1},color);
            }
        };
        if(i==7||i==8){
            // A central open well splits the observation floor into two routes.
            float well=i==7?10.f:6.f;
            floor(-half,-22,half,-8,cream);floor(-half,14,half,40,cream);
            floor(-half,-8,-well,14,cream);floor(well,-8,half,14,cream);
            floor(-half,-40,half,-32,cream);
            for(float side:{-1.f,1.f}){
                float x=side*well;
                solid(c+vec3{x-.16f,0,-8},c+vec3{x+.16f,1.05f,14},steel);
                b.neon(x-.04f,y+1.06f,z-8,x+.04f,y+1.1f,z+14,cyan*.4f);
            }
            for(float side:{-1.f,1.f}){
                floor(side<0?-half:34.f,-33,side<0?-34.f:half,-21,steel);
                int mover=b.mover(c+vec3{side*20,-9.4f,-27},{6,.4f,5.3f},Mover::Path::DRIVEN,{0,0,0},{0,9,0},1,0,cyan*.35f);
                l.movers[mover].color=cream;s.labBridges.push_back({i,side<0?-1:1,mover});
            }
            lab::stencil(b,"CONTACT ROUTE / SELECT A SIDE",{-23,y+3.5f,z-21.8f},.08f);
        }else if(i==2){
            // The floor round a glass plate over the void (walkable: a hidden pane holds it)
            const AABB g=lab::glassFloor();
            floor(-half,-40,half,g.min.z,cream);floor(-half,g.max.z,half,40,cream);
            floor(-half,g.min.z,g.min.x,g.max.z,cream);floor(g.max.x,g.min.z,half,g.max.z,cream);
            b.solid(g.min.x,y-.3f,z+g.min.z,g.max.x,y,z+g.max.z);
        }else if(i!=5)floor(-half,-40,half,40,i==4?vec3{.25f,.35f,.23f}:cream);
        else{
            floor(-half,22,half,40,steel);floor(-half,-40,half,-22,steel);
            solid(c+vec3{-7,-1,-23},c+vec3{7,0,23},cream);
            for(float side:{-1.f,1.f}){
                solid(c+vec3{side<0?-34.f:24.f,-1,-24},c+vec3{side<0?-24.f:34.f,0,24},steel);
                for(float dz:{-22.f,18.f})solid(c+vec3{side<0?-25.f:6.f,-1,dz},c+vec3{side<0?-6.f:25.f,0,dz+5},steel);
            }
        }
        // The perimeter follows each bay. Late spaces retain a low boundary
        // and tall isolated piers, opening their full height to the exterior.
        for(int n=0;n<(int)bays.size();++n){
            const auto& bay=bays[n];
            for(float x:{bay.left,bay.right}){
                bool surgical=i==3 && x==42 && bay.farZ<14 && bay.nearZ>-14;
                if(surgical){
                    solid({x-.4f,y,z+bay.farZ},{x+.4f,y+3,z+bay.nearZ},wall);
                    solid({x-.4f,y+14,z+bay.farZ},{x+.4f,y+roof,z+bay.nearZ},wall);
                    for(float dz:{-14.f,0.f,14.f})solid({x-.5f,y+3,z+dz-.12f},{x+.5f,y+14,z+dz+.12f},steel);
                }else if(i<7){
                    // Openings onto the void, each held by an invisible pane
                    std::vector<LevelBuilder::Gap> gaps;
                    for(const auto& w:lab::windows(i))if(w.x==x && w.z0>=bay.farZ && w.z1<=bay.nearZ){
                        gaps.push_back({z+w.z0,z+w.z1,y+w.y0,y+w.y1});
                        b.solid(x-.06f,y+w.y0,z+w.z0,x+.06f,y+w.y1,z+w.z1);
                    }
                    b.wallZ(z+bay.farZ,z+bay.nearZ,x-.4f,x+.4f,y,y+roof,wall,gaps);
                }
                else{
                    solid({x-.4f,y,z+bay.farZ},{x+.4f,y+1.4f,z+bay.nearZ},wall);
                    for(float dz=bay.farZ;dz<=bay.nearZ;dz+=12){
                        solid({x-.5f,y,z+dz-.25f},{x+.5f,y+roof,z+dz+.25f},steel);
                        prop({x-.55f,y+6,z+dz-.12f},{x+.55f,y+roof-2,z+dz+.12f},cyan*.35f);
                    }
                }
                prop({x-.2f,y+.15f,z+bay.farZ},{x+.2f,y+.3f,z+bay.nearZ},steel);
            }
            if(n){const auto& prev=bays[n-1];float dz=z+bay.nearZ;
                for(auto edge:{glm::vec2{prev.left,bay.left},glm::vec2{prev.right,bay.right}})
                    if(edge.x!=edge.y)solid({std::min(edge.x,edge.y),y,dz-.4f},{std::max(edge.x,edge.y),y+(i<7?roof:1.4f),dz+.4f},wall);
            }
            // Early rooms have full ceilings. Long-term care has a roof slit;
            // the garden and late observations are open to the sky.
            if(i<4 || i==5)solid(c+vec3{bay.left,roof,bay.farZ},c+vec3{bay.right,roof+.8f,bay.nearZ},cream);
            if(i==6)for(float side:{-1.f,1.f})
                solid(c+vec3{side<0?bay.left:12.f,roof,bay.farZ},c+vec3{side<0?-12.f:bay.right,roof+.8f,bay.nearZ},cream);
            float mid=(bay.nearZ+bay.farZ)*.5f;
            solid(c+vec3{bay.left,roof-1.2f,mid},c+vec3{bay.right,roof-.6f,mid+.5f},steel);
            b.neon(bay.left+3,y+roof-1.4f,z+mid,bay.right-3,y+roof-1.25f,z+mid+.22f,(i>=6?cyan:warm)*.32f);
            for(float xx:{bay.left+3,bay.right-3}){
                prop(c+vec3{xx-.08f,.02f,bay.farZ+1},c+vec3{xx+.08f,.04f,bay.nearZ-1},cyan*.4f);
            }
        }
        float endHeight=i>=6?10.f:roof;
        if(i==0){
            // The lift's landing: a narrow lit doorway; its doors are drawn (Gameplay_Lab.h)
            // over a hidden collision panel that never opens once you've stepped out
            const float h=lab::LIFT_HALF;
            b.wallX(bays.front().left,bays.front().right,z+39.6f,z+40.4f,y,y+endHeight,wall,{{-h,h,y,y+lab::LIFT_DOOR_H}});
            b.solid(-h,y,z+39.7f,h,y+lab::LIFT_DOOR_H,z+40.3f);
            for(float s:{-1.f,1.f})b.neon(s*h-.08f,y,z+39.5f,s*h+.08f,y+lab::LIFT_DOOR_H,z+39.56f,cyan*.5f);
            b.neon(-h,y+lab::LIFT_DOOR_H,z+39.5f,h,y+lab::LIFT_DOOR_H+.12f,z+39.56f,cyan*.5f);
        }else
        b.wallX(bays.front().left,bays.front().right,z+39.6f,z+40.4f,y,y+endHeight,wall,{{entry-6,entry+6,y,y+7}});
        b.wallX(bays.back().left,bays.back().right,z-40.4f,z-39.6f,y,y+endHeight,wall,{{exit-6,exit+6,y,y+7}});
        if(i>=6)for(float dz:{-40.f,40.f}){
            const auto& bay=dz<0?bays.back():bays.front();
            for(float x:{bay.left,bay.right})solid({x-.5f,y+10,z+dz-.3f},{x+.5f,y+roof,z+dz+.3f},steel);
            b.kit().arch(c+vec3{(bay.left+bay.right)*.5f,roof-12,dz},bay.right-bay.left,12,.7f,1.f,steel,0,20);
        }
        // Floor wayfinding sits outside the traversal capsule and casts no point lights.
        prop(c+vec3{exit-.09f,.02f,-37},c+vec3{exit+.09f,.035f,-29},cyan*.45f);
        if(i==0){
            prop(c+vec3{-17,.02f,20},c+vec3{0,.035f,20.15f},cyan*.45f);
            prop(c+vec3{-17,.02f,-27},c+vec3{-16.85f,.035f,20},cyan*.45f);
        }
        // Broad entry/exit frames; a number remains legible through the open room.
        int door=b.doorway(true,exit-5.8f,exit+5.8f,z-40.3f,z-39.7f,y,7,cyan,true);
        lab::stencil(b,lab::name(i),{exit-5.5f,y+9,z-39.4f},.13f);
        char id[32];std::snprintf(id,sizeof(id),"LEVEL %02d / DESCENT",i+1);
        lab::stencil(b,id,{-half+6,y+4,z+39.3f},.1f);
        for(float side:{-1.f,1.f})b.neon(side<0?-half+2:half-2.2f,y+.06f,z-38,side<0?-half+2.2f:half-2,y+.12f,z+38,cyan*.22f);
        Arena a;a.name=lab::name(i);a.subtitle=lab::objective(i);a.zone=LevelBuilder::aabb(-half,y-300,z-41,half,y+roof-.05f,z+41);a.bounds=a.zone;a.bounds.min.y=y;
        a.entryGate=i>0?b.doorway(true,entry-5.8f,entry+5.8f,z+39.7f,z+40.3f,y,7,cyan,false):-1;
        a.playerStart={entry,y,z+31};a.startYaw=-90;a.exitDoor=door;a.maxAlive=14;a.damageScale=.9f;a.theme=lab::theme(i);
        a.ambient=i==8?Ambient::MARINE:i>=7?Ambient::NEBULA:i==4?Ambient::WIND:Ambient::SPECKS;a.space=ReverbSpace::HALL;a.tone=i==4?RoomTone::YARD:i==8?RoomTone::HADAL:i>=7?RoomTone::CELESTIAL:RoomTone::WARD;
        a.hasTrigger=i!=4;a.trigger=LevelBuilder::aabb(-40,y-1,z-37,40,y+14,z+17);a.scripted=i==4;
        a.voidY=y-12;a.hasRespawn=true;a.respawn={entry,y,z+28};
        a.groundSpawns={{-24,y,z-18},{24,y,z-22},{-18,y,z+2},{18,y,z-6}};a.airSpawns={{-22,y+8,z-14},{22,y+9,z-24}};
        if(i==1)a.groundSpawns={{-4,y,z-18},{18,y,z-22},{-18,y,z+2},{18,y,z-6}};
        if(i==5)a.groundSpawns={{-28,y,z-18},{28,y,z-18},{0,y,z+2},{28,y,z-6}};
        if(i==7||i==8)a.groundSpawns={{-18,y,z-14},{18,y,z-18},{-18,y,z+2},{18,y,z-6}};
        // Doubled finite encounters: retain the original placements and give each
        // reinforcement its own nearby position, including on the galleries.
        auto at=[&](EnemyType t,std::initializer_list<vec3> points){std::vector<vec3> p;for(auto q:points)p.push_back(c+q);return W(t,0,p);};
        using E=EnemyType;
        if(i==0)a.waves={{at(E::PATIENT,{{-22,0,-14},{24,0,-22},{-20,0,0},{14,0,-8},{-8,0,-24},{-22,0,-10.5f},{24,0,-18.5f},{-20,0,3.5f},{14,0,-4.5f},{-8,0,-20.5f}})}};
        if(i==1)a.waves={{at(E::PATIENT,{{-4,0,-16},{16,0,-24},{0,0,0},{-24,0,-3},{8,0,-30},{12,0,-34},{-4,0,-12.5f},{16,0,-20.5f},{-4,0,0},{-24,0,0.5f},{8,0,-26.5f},{12,0,-30.5f}})},
                         {at(E::PATIENT,{{4,0,-19},{20,0,-12},{-20,0,1},{-6,0,-31},{24,0,-4},{4,0,-15.5f},{20,0,-8.5f},{-20,0,4.5f},{-6,0,-27.5f},{24,0,-0.5f}}),at(E::GRAFTED,{{0,0,-25},{0,0,-21.5f}})}};
        if(i==2)a.waves={{at(E::GRAFTED,{{0,0,-18},{0,0,-14.5f}}),at(E::PATIENT,{{-15,0,-9},{15,0,-3},{-31,6,-12},{31,6,-12},{-8,0,-24},{8,0,-12},{-15,0,-5.5f},{15,0,0.5f},{-31,6,-8.5f},{31,6,-8.5f},{-8,0,-20.5f},{8,0,-8.5f}})}};
        if(i==3)a.waves={{at(E::SUSPENDED,{{0,8,-20},{0,8,-16.5f}}),at(E::PATIENT,{{-19,0,-4},{20,0,-16},{-14,0,12},{10,0,-24},{-6,0,8},{-19,0,-0.5f},{20,0,-12.5f},{-14,0,15.5f},{10,0,-20.5f},{-6,0,11.5f}})},
                         {at(E::SUSPENDED,{{24,9,-19},{24,9,-15.5f}}),at(E::GRAFTED,{{-16,0,-12},{-16,0,-8.5f}}),at(E::PATIENT,{{31,6,-12},{-31,6,-12},{31,6,-8.5f},{-31,6,-8.5f}})}};
        if(i==5)a.waves={{at(E::SUSPENDED,{{-21,8,-12},{-21,8,-8.5f}}),at(E::PATIENT,{{0,0,-12},{28,0,-10},{-28,0,-7},{28,0,10},{-28,0,12},{0,0,8},{0,0,-8.5f},{28,0,-6.5f},{-28,0,-3.5f},{28,0,13.5f},{-28,0,15.5f},{0,0,11.5f}})}};
        if(i==6)a.waves={{at(E::PHASED,{{0,0,-20},{0,0,-16.5f}})},
                         {at(E::PHASED,{{-18,0,-20},{-18,0,-16.5f}}),at(E::PATIENT,{{14,0,-10},{-17,0,-5},{0,0,2},{-6,0,-12},{14,0,-6.5f},{-17,0,-1.5f},{0,0,5.5f},{-6,0,-8.5f}}),at(E::GRAFTED,{{-18,6,-15.5f},{-14,6,-15.5f}}),at(E::SUSPENDED,{{24,8,-23},{24,8,-19.5f}})}};
        if(i==7)a.waves={{at(E::PHASED,{{-16,0,-8},{20,0,-18},{-16,0,-4.5f},{20,0,-14.5f}}),at(E::SUSPENDED,{{-26,8,-26},{-26,8,-22.5f}}),at(E::PATIENT,{{0,0,-16},{-18,0,10},{18,0,8},{6,0,-16},{0,0,-12.5f},{-18,0,13.5f},{18,0,11.5f},{6,0,-12.5f}})}};
        if(i==8)a.waves={{at(E::GRAFTED,{{0,0,-18},{0,0,-14.5f}}),at(E::SUSPENDED,{{-24,8,-20},{-24,8,-16.5f}}),at(E::PHASED,{{21,0,-16},{21,0,-12.5f}}),at(E::PATIENT,{{-18,0,-4},{18,0,-4},{-18,0,-0.5f},{18,0,-0.5f}})},
                         {at(E::PHASED,{{-19,0,-13},{20,0,-18},{-19,0,-9.5f},{20,0,-14.5f}}),at(E::PATIENT,{{-10,0,-17},{14,0,-8},{18,0,8},{-4,0,-20},{-10,0,-13.5f},{14,0,-4.5f},{18,0,11.5f},{-4,0,-16.5f}})}};
        if(i==2||i==6||i==7||i==8)a.goals={WaveGoal::external(lab::objective(i))};
        l.arenas.push_back(a);
        if(i<9){
            float next=lab::entryX(i+1),left=std::min(exit,next)-6,right=std::max(exit,next)+6;
            bool covered=i<3 || i==5;
            ramp(exit,z-40,z-56,y,y-7,covered);
            ramp(next,z-64,z-80,y-7,y-14,covered);
            solid({left,y-7.6f,z-64},{right,y-7,z-56},steel);
            b.wallX(left,right,z-56.2f,z-55.8f,y-7,y+(covered?.6f:-5.8f),wall,{{exit-6,exit+6,y-7,y+1}});
            b.wallX(left,right,z-64.2f,z-63.8f,y-7,y+(covered?.6f:-5.8f),wall,{{next-6,next+6,y-7,y+1}});
            for(float x:{left,right})solid({x-.18f,y-7,z-64},{x+.18f,y+(covered?.6f:-5.8f),z-56},wall);
            if(covered)solid({left,y+.6f,z-64},{right,y+1,z-56},cream);
            else{
                // A freestanding canopy frames the turn, rather than a gutter
                // between high walls. Beyond it the next room's skyline opens.
                for(float x:{left,right})solid({x-.25f,y-7,z-61},{x+.25f,y+8,z-60.5f},steel);
                solid({left,y+8,z-61},{right,y+8.5f,z-60.5f},cream);
            }
            for(float x:{exit,next})solid({x-6,y-14.6f,z-81},{x+6,y-14,z-79.9f},steel);
            auto passage=LevelBuilder::aabb(left,y-300,z-81,right,y+10,z-39);
            l.basins.push_back({passage,y-250});
            auto& owner=l.arenas.back();
            owner.extraZones.push_back(LevelBuilder::aabb(exit-6,y-300,z-56,exit+6,y+9,z-39));
            owner.extraZones.push_back(LevelBuilder::aabb(left,y-300,z-64,right,y+9,z-56));
            owner.extraZones.push_back(LevelBuilder::aabb(next-6,y-300,z-81,next+6,y+2,z-64));
            owner.fallRegions.push_back({passage,y-26});
            l.blends.push_back({passage,i,i+1,2,true});
            // A turn's horizontal strip is readable from either flight.
            prop({left+.5f,y-6.98f,z-60.1f},{right-.5f,y-6.94f,z-59.9f},cyan*.55f);
        }
        if(i==0){
            b.mat=Mat::CLINIC;
            // The glass theatre lies directly in the entrance camera's line of sight.
            solid({-10,y,z+8},{10,y+.6f,z+17},steel);
            for(float x:{-10.f,10.f})solid({x-.2f,y,z+7},{x+.2f,y+8,z+18},cream);
            solid({-10,y+8,z+7},{10,y+8.3f,z+18},cream);
            solid({-10,y,z+7},{10,y+8,z+7.2f},cream);
            lab::stencil(b,"MOTOR TEST 03",{-9,y+6.3f,z+17.95f},.11f);
            lab::stencil(b,"FLOOR CONTACT: CONFIRMED",{-9,y+4.8f,z+17.95f},.065f);
            for(float side:{-1.f,1.f}){
                solid({side*25-6,y,z+15},{side*25+6,y+3,z+19},cream);
                for(int n=0;n<4;++n)prop({side*25-5+n*3.f,y+3.1f,z+16},{side*25-3+n*3.f,y+3.4f,z+18},steel);
            }
            console({-16,y,z+20},"ADMISSIONS / LEGACY","CHASSIS 24: NO COMPATIBLE PATIENT RECORD. MANUAL ASSESSMENT REQUESTED.");
        }
        if(i==1){
            b.mat=Mat::CLINIC;
            solid(c+vec3{-6,0,3},c+vec3{6,1.2f,5},cream);
            solid(c+vec3{4,0,-3},c+vec3{6,1.2f,3},cream);
            prop(c+vec3{-6,1.2f,2.8f},c+vec3{6.2f,1.35f,5.2f},steel);
            lab::stencil(b,"NURSE STATION / NO STAFF ON DUTY",{-5,y+1.1f,z+5.05f},.05f);
            console(c+vec3{0,0,13},"RECOVERY / 17","ALL BEDS OCCUPIED. ALL PATIENTS DISCHARGED. PLEASE LEAVE THE LINEN IN PLACE.");
            for(float x:{-27.f,27.f})for(int n=0;n<4;++n){
                if(x<0 && n>=2)continue;
                vec3 p{x,y,z+24-n*15.f};if(x>0)p.z=z+4-n*12.f;bed(p,10+n+(x>0?4:0));
                // Open-ended curtain bays keep patients readable without dividing the combat floor.
                b.mat=Mat::METAL;for(float dz:{-3.f,3.f}){
                    solid(p+vec3{-3.5f,0,dz-.08f},p+vec3{-3.35f,3.8f,dz+.08f},steel);
                    prop(p+vec3{-3.5f,3.65f,dz-.08f},p+vec3{3.5f,3.8f,dz+.08f},steel);
                    for(int k=0;k<6;++k)prop(p+vec3{-3.2f+k*.46f,.9f,dz-.03f},p+vec3{-2.82f+k*.46f,3.65f,dz+.03f},{.45f,.65f,.65f});
                }
                prop(p+vec3{2.2f,0,-1},p+vec3{2.3f,3.1f,-.9f},steel);
                prop(p+vec3{1.7f,2.2f,-1.1f},p+vec3{2.05f,3,-.8f},{.58f,.73f,.67f});
            }
            lab::stencil(b,"LOAD: 73.4KG",{-28,y+2.3f,z+11.1f},.055f);}
        if(i==2||i==3||i==6){
            for(float side:{-1.f,1.f}){
                solid(c+vec3{side<0?-39.f:23.f,5.4f,-27},c+vec3{side<0?-23.f:39.f,6,-5},steel);
                // Wide walkable ramps to both galleries, not vertical walls disguised as stairs.
                AABB area=LevelBuilder::aabb(side<0?-36:26,y,z-5,side<0?-26:36,y+6,z+19);
                l.ramps.push_back({area,2,6+y,y});
                b.kit().box(c+vec3{side*31,2.7f,7},{24.74f,.6f,10},steel,-1.5707963f,0,-.24497866f);
                b.neon(side*31-4,y+6.05f,z-26,side*31+4,y+6.12f,z-25.8f,cyan*.3f);
            }
        }
        if(i==2 || i==6){
            // Motor assessment's U gallery becomes a suspended cross-route in
            // Long-Term Care. Both ramps lead somewhere, with a view back down.
            float dz=i==2?-25.f:-18.f;
            solid(c+vec3{-31,5.4f,dz},c+vec3{31,6,dz+5},steel);
            for(float edge:{dz,dz+5}){
                solid(c+vec3{-23,6,edge-.12f},c+vec3{23,7.1f,edge+.12f},steel);
                b.neon(-22,y+7.12f,z+edge,22,y+7.17f,z+edge+.08f,cyan*.3f);
            }
        }
        if(i==6){
            // Empty wards hang inside a much taller nave; the central roof is
            // physically absent, revealing the next impossible exterior.
            for(int k=0;k<3;++k){float dz=-23+k*21.f,h=20+k*7.f;
                b.kit().arch(c+vec3{0,h,dz},48,12,.9f,1.4f,cream,0,18);
                for(float side:{-1.f,1.f}){
                    solid(c+vec3{side*24-.3f,h,dz-.3f},c+vec3{side*24+.3f,roof,dz+.3f},steel);
                    b.kit().box(c+vec3{side*19,h+2,dz},{9,.6f,6},cream,side*.16f);
                }
            }
        }
        if(i==8){
            // A high trench shelf offers a second route around the well and
            // the contact bridge. Each end has a real ramp back to the floor.
            solid(c+vec3{-42,7.4f,-22},c+vec3{-30,8,-5},steel);
            for(auto flight:{glm::vec4{-5,19,8,0},glm::vec4{-34,-22,0,8}}){
                auto area=LevelBuilder::aabb(-40,y,z+flight.x,-32,y+8,z+flight.y);
                l.ramps.push_back({area,2,y+flight.z,y+flight.w});
                float span=flight.y-flight.x,dy=flight.w-flight.z;
                b.kit().box(c+vec3{-36,(flight.z+flight.w)*.5f-.3f,(flight.x+flight.y)*.5f},
                    {std::hypot(span,dy),.6f,8},steel,-1.5707963f,0,std::atan2(dy,span));
            }
            solid(c+vec3{-30.2f,8,-22},c+vec3{-29.8f,9.1f,-5},steel);
            console(c+vec3{-40,8,-14},"EXTERIOR ACCESS","SERVICE WALKWAY: SURFACE LEVEL. THE SOUND BELOW IS NOT THE SEA.");
        }
        if(i==2)console(c+vec3{-34,6,-23},"DISCHARGED / UNIT 18","THEY ASKED ME TO WALK IN A STRAIGHT LINE. I DID. THE ROOM IS STILL TURNING.");
        if(i==3)console(c+vec3{34,6,-23},"SURGICAL REGISTER","PROCEDURE COMPLETE. ANAESTHESIA DISCONTINUED. PATIENT HAS REPORTED DREAMING OF A ROOM WE HAVE NOT BUILT.");
        if(i==6)console(c+vec3{34,6,-22},"SUSPENSION","WE LOWERED THE BEDS. THE PATIENTS DID NOT FOLLOW. CONTACT SENSORS REMAIN WITHIN TOLERANCE.");
        if(i==2){
            b.mat=Mat::METAL;
            for(float xx:{-11.f,11.f}){
                solid(c+vec3{xx-1,0,-18},c+vec3{xx+1,.65f,-11},cream);
                prop(c+vec3{xx-.12f,.66f,-18},c+vec3{xx+.12f,.69f,-11},cyan*.6f);
            }
            lab::stencil(b,"WALK TOWARD THE LINE",{-10,y+.4f,z+39.3f},.1f);
        }
        if(i==3){
            // A sealed, unsupported observation booth beyond the glass. Its
            // chair faces the theatre, but none of its three walls has a door.
            b.mat=Mat::CLINIC;
            solid(c+vec3{56,5.4f,-7},c+vec3{65,6,7},cream);
            solid(c+vec3{56,12,-7},c+vec3{65,12.6f,7},cream);
            solid(c+vec3{64.5f,6,-7},c+vec3{65,12,7},wall);
            for(float dz:{-7.f,6.5f})solid(c+vec3{56,6,dz},c+vec3{65,12,dz+.5f},wall);
            prop(c+vec3{60,6,-.8f},c+vec3{60.15f,7.2f,.8f},steel);
            prop(c+vec3{59.3f,7.2f,-.8f},c+vec3{60.7f,7.4f,.8f},steel);
            prop(c+vec3{60.55f,7.4f,-.8f},c+vec3{60.7f,9,.8f},cream);
            b.neon(57,y+11.8f,z-5,64,y+11.9f,z-4.8f,cyan*.35f);
            b.mat=Mat::METAL;
            solid(c+vec3{-2,9,-10},c+vec3{2,9.5f,10},cream);
            for(float dz:{-7.f,7.f}){
                solid(c+vec3{-.18f,5,dz-.18f},c+vec3{.18f,9,dz+.18f},steel);
                b.kit().dome(c+vec3{0,5,dz},2.4f,cream,4,12);
                b.neon(-1.2f,y+4.85f,z+dz-.9f,1.2f,y+4.95f,z+dz+.9f,warm*.5f);
            }
            solid(c+vec3{-5,0,-7},c+vec3{5,1.5f,7},steel);bed(c+vec3{0,1.5f,0},31);}
        if(i==4){
            for(int n=0;n<13;++n){float x=n%2?-29.f:29.f,dz=-32.f+(n/2)*10.f;
                b.mat=Mat::WOOD;b.kit().rod(c+vec3{x,0,dz},c+vec3{x+.6f,8+n%4,dz+.5f},.24f,{.28f,.22f,.16f},10);
                b.mat=Mat::ROCK;for(int k=0;k<3;++k)b.kit().dome(c+vec3{x+std::sin(k*2.f)*2,7.f+k*1.7f,dz+std::cos(k*2.f)*2},3.8f,{.22f+k*.02f,.39f,.24f},6,12);
            }
            // Low planted islands and a shallow running-water strip: a maintained garden
            // whose service terminal nevertheless says its supply is disconnected.
            for(float side:{-1.f,1.f})for(int n=0;n<4;++n){
                vec3 p=c+vec3{side*29,0,-28.f+n*17};b.mat=Mat::CLINIC;
                solid(p+vec3{-6,0,-4},p+vec3{6,.4f,4},{.58f,.64f,.55f});
                b.mat=Mat::ROCK;prop(p+vec3{-5.7f,.4f,-3.7f},p+vec3{5.7f,.45f,3.7f},{.2f,.25f,.14f});
                for(int k=0;k<7;++k){vec3 leaf=p+vec3{-4.5f+k*1.5f,.5f,-2.f+(k%3)*1.8f};
                    for(int v=0;v<3;++v)b.kit().box(leaf+vec3{0,.4f,0},{.1f,.85f,.2f},{.23f,.42f,.2f},k*.8f+v*2.f,0,.3f*(v-1));
                }
            }
            b.mat=Mat::CLINIC;
            for(float x:{-10.f,10.f})solid(c+vec3{x-2,-.1f,-25},c+vec3{x+2,0,23},{.22f,.38f,.42f});
            for(float x:{-10.f,10.f})l.water.push_back({LevelBuilder::aabb(x-1.9f,y-.3f,z-24.8f,x+1.9f,y,z+22.8f),y+.10f});
            // An ordinary drip stand, with its hose ending above dry soil.
            b.mat=Mat::METAL;solid(c+vec3{24,0,-25},c+vec3{24.08f,3.5f,-24.92f},steel);
            prop(c+vec3{23.5f,2.6f,-25.1f},c+vec3{23.8f,3.3f,-24.8f},{.55f,.7f,.65f});
            console(c+vec3{29,0,-28},"IRRIGATION","SUPPLY LINE DISCONNECTED. SOIL MOISTURE REMAINS CONSTANT. DO NOT ADJUST THE VALVE.");
            trolley(c+vec3{-21,0,27},false);
        }
        if(i==7||i==8){
            for(float x:{-24.f,24.f})for(float dz:{-20.f,12.f})bed(c+vec3{x,0,dz},17);
            console(c+vec3{-34,0,25},i==7?"OBSERVATION / CELESTIAL":"OBSERVATION / HADAL",
                i==7?"ALTITUDE: 0000. OUTSIDE PRESSURE: NOMINAL. WINDOW REPLACEMENT REQUEST DENIED: NO EXTERIOR HAS BEEN REGISTERED.":"DEPTH: 10912 M. SURFACE CONTACT: ACTIVE. LAST CONTACT RECEIVED FROM THE ROOM ABOVE. DO NOT ACKNOWLEDGE KNOCKING.");
        }
        if(i==7){
            b.mat=Mat::METAL;b.kit().box(c+vec3{-62,7,-12},{7,.5f,7},cream,.08f);
            prop(c+vec3{-62.5f,7.7f,-12.5f},c+vec3{-61.5f,7.9f,-11.5f},steel);
            prop(c+vec3{-62.5f,7.9f,-12.5f},c+vec3{-61.5f,9,-12.35f},cream);
            for(float dx:{-62.4f,-61.6f})for(float dz:{-12.4f,-11.6f})prop(c+vec3{dx,7.2f,dz},c+vec3{dx+.08f,7.7f,dz+.08f},steel);
            prop(c+vec3{-60.7f,7.28f,-11},c+vec3{-60.1f,7.4f,-10.8f},{.7f,.55f,.25f});
        }
        if(i==8)trolley(c+vec3{29,0,27},true);
        if(i>=7){
            // Exterior objects live beyond the windows, with real parallax and scale.
            for(int k=0;k<12;++k){float side=k%2?-1.f:1.f;vec3 p=c+vec3{side*(65+k*9.f),8.f+k*3,z*0.f-45.f+k*9};
                if(i==8){b.mat=Mat::ROCK;b.kit().rock(p+vec3{0,-140,0},13.f+k,210.f,{.025f,.09f,.12f},490+k,.7f,11);}
                else {b.kit().box(p,{5.f+k,2.f,12.f+k},steel,k*.3f);}
            }
        }
        b.mat=Mat::METAL;
        if(i!=4&&i!=5&&i!=9)for(vec3 p:{c+vec3{-12,0,-9},c+vec3{12,0,9}})solid(p+vec3{-2,0,-1},p+vec3{2,1.6f,1},steel);
        l.placedPickups.push_back({c+vec3{entry+8,0,30},1});
        lab::combatStations(b,i,c);
        // Set dressing after the concept boards (LabDressing.h), once everything it must keep clear of exists
        labdress::dressRoom(b,i,c,bays,roof,entry,exit);
    }
    const vec3 end=lab::centre(9);s.end=LevelBuilder::aabb(-6,end.y-1,end.z-116,6,end.y+6,end.z-101);
    b.mat=Mat::CLINIC;
    // A playable coda beyond the final assessment, inside the tenth room's region.
    solid(end+vec3{-7,-.7f,-118},end+vec3{7,0,-72},cream);
    l.arenas.back().extraZones.push_back(LevelBuilder::aabb(-7,end.y-300,end.z-120,7,end.y+28,end.z-71));
    l.basins.push_back({LevelBuilder::aabb(-8,end.y-300,end.z-120,8,end.y+30,end.z-72),end.y-250});
    // Two optional records reward looking above the test floor and behind the impossible garden.
    // Append new terminal IDs after the existing records: old save bitmasks
    // must continue referring to the same terminals.
    s.terminals.push_back({lab::centre(2)+vec3{-22,2.4f,-26},"MANUAL CALIBRATION",
        "THE LOAD RETURNS TO ZERO. THE SUBJECT DOES NOT. AFTER EACH RESET, IT CHOOSES THE HIGHER ROUTE."});
    s.wrecks.push_back({lab::centre(2)+vec3{-25,6,-24},0,18,-1,"THE LOCK OPENS FROM THE GALLERY. THEY DID NOT EXPECT US TO LOOK UP.",2,true});
    s.wrecks.push_back({lab::centre(4)+vec3{37,0,-34},0,21,-1,"NO ROOTS IN THE SOIL. NO WATER IN THE PIPE. SOMETHING HERE STILL KNOWS WHAT A GARDEN SHOULD BE.",4,true});
    // A little garden, out beyond the corridor: the exit is a place you can actually walk into.
    b.mat=Mat::CLINIC;
    solid(end+vec3{-18,-1,-128},end+vec3{18,0,-100},cream);
    for(float side:{-1.f,1.f})for(int k=0;k<3;++k){auto p=end+vec3{side*13,0,-105.f-k*8};
        b.mat=Mat::WOOD;b.kit().rod(p,p+vec3{0,6,0},.2f,{.3f,.22f,.15f},8);
        b.mat=Mat::ROCK;b.kit().dome(p+vec3{0,5,0},3.f,{.3f,.5f,.3f},5,10);
    }
    l.arenas.back().extraZones.push_back(LevelBuilder::aabb(-19,end.y-300,end.z-130,19,end.y+28,end.z-99));
    // THE LIVING WARD's steel: brushed stainless, perforated where it's walked (MaterialTex.h)
    l.restyle({{Mat::METAL, Mat::SURGICAL}});
    return l;
}

// The end of Chapter 3 (Gameplay_LabEnding.h): where it plays
namespace labend {
inline glm::vec3 arena() { return lab::centre(9); }
inline float floorY() { return arena().y; }
inline float doorZ() { return arena().z - 72.f; }
inline glm::vec3 walkFrom() { return arena() + glm::vec3{0.f, 0.f, -110.f}; }
inline glm::vec3 edge() { return arena() + glm::vec3{0.f, 0.f, -126.4f}; }
inline glm::vec3 flat(glm::vec3 v) { v.y = 0.f; float l = glm::length(v); return l > 1e-4f ? v / l : glm::vec3{0.f, 0.f, 1.f}; }
// A little life in a held camera: slow, small, never still
inline glm::vec3 handheld(float t, float k) {
    return glm::vec3{std::sin(t * 1.31f) + 0.5f * std::sin(t * 2.9f + 1.f), std::sin(t * 1.73f + 2.f) * 0.8f, std::sin(t * 1.07f + 4.f)} * (0.035f * k);
}
}
