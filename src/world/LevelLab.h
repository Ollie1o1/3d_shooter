#pragma once
#include "world/Level.h"
#include "ui/PixelFont.h"

namespace lab {
constexpr int ROOMS=10;
inline glm::vec3 centre(int i){return {0,-14.f*i,-120.f*i};}
inline float routeX(int i){static constexpr float x[]={-18,18,-22,22,-18,18,-22,20,-18,0};return x[std::clamp(i,0,9)];}
inline glm::vec3 contactPosition(int room,int side=-1){
    return centre(room)+(room==2?glm::vec3{31,7.5f,-17}:room==6?glm::vec3{-31,7.5f,-17}:glm::vec3{side*20.f,1.5f,-18});
}
inline const char* name(int i){static const char* n[]={"ADMISSIONS","RECOVERY WARD","MOTOR ASSESSMENT","SURGICAL OBSERVATION","THE CONSERVATORY","LOAD TESTING","LONG-TERM CARE","CELESTIAL OBSERVATION","HADAL RESEARCH","FINAL ASSESSMENT"};return n[std::clamp(i,0,9)];}
inline const char* objective(int i){static const char* n[]={"ENTER THE WARD","REACH THE OBSERVATION GALLERY","RELEASE THE MOTOR LOCK","CROSS THE SURGICAL FLOOR","FIND THE SERVICE DOOR","CROSS THE LOAD BRIDGE","RELEASE THE SUSPENSION LOCK","ROUTE THE OBSERVATION BRIDGE","SEAT THE HADAL CONTACT","DEFEAT THE SUCCESSOR"};return n[std::clamp(i,0,9)];}
inline Theme theme(int i){
    Theme t{{.1f,.17f,.22f},{.22f,.35f,.4f},{.1f,.15f,.18f},glm::normalize(glm::vec3{.2f,.8f,-.3f}),{.8f,.85f,.9f},.025f,0,{.06f,.1f,.15f},.1f,glm::normalize(glm::vec3{.3f,-.8f,.4f}),{.65f,.72f,.8f},{.6f,.66f,.7f},{.3f,.34f,.37f},{.2f,.3f,.36f},.002f};
    if(i==4){t.zenith={.22f,.48f,.66f};t.horizon={.7f,.8f,.72f};t.sunColor={1.2f,1.05f,.7f};t.skyAmb={.6f,.67f,.52f};t.groundAmb={.24f,.32f,.2f};}
    if(i==7||i==9){t.zenith={.05f,.025f,.16f};t.horizon={.22f,.06f,.25f};t.stars=.95f;t.fogColor={.09f,.07f,.19f};}
    if(i==8){t.zenith={.008f,.04f,.09f};t.horizon={.01f,.12f,.2f};t.fogColor={.015f,.09f,.14f};t.stars=0;}
    return t;
}
// Stencils are actual small surfaces and do not consume a light per letter.
inline void stencil(LevelBuilder& b,const char* text,glm::vec3 p,float px,glm::vec3 color={.1f,.24f,.28f}){
    for(int i=0;text[i];++i){unsigned char ch=text[i];if(ch<32||ch>127)continue;
        for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(PIXEL_FONT[ch-32][y]&(1<<(4-x))){
            auto q=p+glm::vec3{(i*6+x)*px,-y*px,0};b.prop(q.x,q.y,q.z,q.x+px*.8f,q.y+px*.8f,q.z+.035f,color);
        }
    }
}
}

inline LevelData buildLab(){
    using glm::vec3;using W=WaveEntry;
    LevelData l;LevelBuilder b{l};auto& s=l.story;
    s.built=true;s.chapter=3;s.nextChapter=0;s.title="THE LIVING WARD";s.nextName="BEYOND THE ASSESSMENT";
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
    auto ramp=[&](float x,float za,float zb,float ya,float yb){
        AABB a=LevelBuilder::aabb(x-6,std::min(ya,yb)-1,za,x+6,std::max(ya,yb)+3,zb);
        // GroundRamp runs along increasing Z: the north end is the lower end.
        l.ramps.push_back({a,2,yb,ya});
        float angle=std::atan2(ya-yb,za-zb);vec3 c{x,(ya+yb)*.5f-.3f,(za+zb)*.5f};
        b.mat=Mat::METAL;b.kit().box(c,{std::sqrt((za-zb)*(za-zb)+(ya-yb)*(ya-yb)),.6f,12},steel,-1.5707963f,0,angle);
        for(float side:{-1.f,1.f})solid({x+side*6-.2f,std::min(ya,yb),zb},{x+side*6+.2f,std::max(ya,yb)+3,za},wall);
    };
    for(int i=0;i<10;++i){
        const vec3 c=lab::centre(i);const float y=c.y,z=c.z,half=i==0?32.f:i==1?36.f:42.f;float entry=i?lab::routeX(i-1):0,exit=lab::routeX(i);
        b.mat=Mat::CLINIC;
        const float roof=i==0?12.f:i==1||i==2?16.f:i==3||i==6?20.f:23.f;
        l.basins.push_back({LevelBuilder::aabb(-half-1,y-300,z-42,half+1,y+40,z+42),(i==5||i==7||i==8)?y-250:y});
        if(i==7||i==8){
            solid(c+vec3{-42,-.8f,-22},c+vec3{42,0,40},cream);
            solid(c+vec3{-42,-.8f,-40},c+vec3{42,0,-32},cream);
            for(float side:{-1.f,1.f}){
                solid(c+vec3{side<0?-42.f:34.f,-.8f,-33},c+vec3{side<0?-34.f:42.f,0,-21},steel);
                int mover=b.mover(c+vec3{side*20,-9.4f,-27},{6,.4f,5.3f},Mover::Path::DRIVEN,{0,0,0},{0,9,0},1,0,cyan*.35f);
                l.movers[mover].color=cream;s.labBridges.push_back({i,side<0?-1:1,mover});
            }
            lab::stencil(b,"CONTACT ROUTE / SELECT A SIDE",{-23,y+3.5f,z-21.8f},.08f);
        }else if(i!=5)solid(c+vec3{-half,-.8f,-40},c+vec3{half,0,40},i==4?vec3{.25f,.35f,.23f}:cream);
        else{
            solid(c+vec3{-42,-1,22},c+vec3{42,0,40},steel);solid(c+vec3{-42,-1,-40},c+vec3{42,0,-22},steel);
            solid(c+vec3{-7,-1,-23},c+vec3{7,0,23},cream);
            for(float side:{-1.f,1.f}){
                solid(c+vec3{side<0?-34.f:24.f,-1,-24},c+vec3{side<0?-24.f:34.f,0,24},steel);
                for(float dz:{-22.f,18.f})solid(c+vec3{side<0?-25.f:6.f,-1,dz},c+vec3{side<0?-6.f:25.f,0,dz+5},steel);
            }
        }
        // Tall perimeter; the late wards look through real wide window bays.
        for(float side:{-1.f,1.f}){
            float x=side*half;
            if(i<7)solid({x-.5f,y,z-40},{x+.5f,y+roof,z+40},wall);
            else{
                solid({x-.5f,y,z-40},{x+.5f,y+1.6f,z+40},wall);
                solid({x-.5f,y+18,z-40},{x+.5f,y+roof,z+40},wall);
                for(float dz=-40;dz<=40;dz+=10)solid({x-.6f,y,z+dz-.25f},{x+.6f,y+roof,z+dz+.25f},steel);
            }
        }
        b.wallX(-half,half,z+39.6f,z+40.4f,y,y+roof,wall,{{entry-6,entry+6,y,y+7}});
        b.wallX(-half,half,z-40.4f,z-39.6f,y,y+roof,wall,{{exit-6,exit+6,y,y+7}});
        if(i<7&&i!=4)solid(c+vec3{-half,roof,-40},c+vec3{half,roof+.8f,40},cream);
        for(float dz=-32;dz<40;dz+=16){
            solid(c+vec3{-half,roof-1.2f,dz},c+vec3{half,roof-.85f,dz+.45f},steel);
            b.neon(-half+8,y+roof-1.4f,z+dz,half-8,y+roof-1.25f,z+dz+.22f,(i>=7?cyan:warm)*.32f);
        }
        for(float side:{-1.f,1.f}) {
            float x=side*(half-.8f);
            prop({x-.18f,y+1,z-38},{x+.18f,y+1.17f,z+38},steel);
            prop({x-.15f,y+.15f,z-38},{x+.15f,y+.3f,z+38},cream*.7f);
            if(i<7)for(float dz:{-28.f,4.f,28.f}){
                prop({x-.4f,y+2.8f,z+dz-2.2f},{x+.4f,y+5.8f,z+dz+2.2f},cream);
                for(int k=0;k<3;++k)prop({x-.42f,y+3.1f+k*.7f,z+dz-1.6f},{x+.42f,y+3.2f+k*.7f,z+dz+1.6f},steel);
            }
        }
        // Repeated service stripe remains aligned even when the exterior stops making sense.
        for(float xx:{-half+5,half-9})for(float dz=-33;dz<=33;dz+=6)
            prop(c+vec3{xx,.012f,dz},c+vec3{xx+4,.025f,dz+.15f},{.26f,.54f,.57f});
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
        a.playerStart={entry,y,z+31};a.startYaw=-90;a.exitDoor=door;a.maxAlive=7;a.damageScale=.9f;a.theme=lab::theme(i);
        a.ambient=i>=7?Ambient::MOTES:i==4?Ambient::WIND:Ambient::DUST;a.space=ReverbSpace::HALL;a.tone=i==4?RoomTone::YARD:i==8?RoomTone::HADAL:i>=7?RoomTone::CELESTIAL:RoomTone::WARD;
        a.hasTrigger=i!=4;a.trigger=LevelBuilder::aabb(-40,y-1,z-37,40,y+14,z+17);a.scripted=i==4;
        if(i==5||i==7||i==8||i==9){a.voidY=y-12;a.hasRespawn=true;a.respawn={0,y,z+28};}
        a.groundSpawns={{-24,y,z-18},{24,y,z-22},{-18,y,z+2},{18,y,z-6}};a.airSpawns={{-22,y+8,z-14},{22,y+9,z-24}};
        // Each new body is first introduced in a readable, finite encounter.
        auto at=[&](EnemyType t,std::initializer_list<vec3> points){std::vector<vec3> p;for(auto q:points)p.push_back(c+q);return W(t,0,p);};
        using E=EnemyType;
        if(i==0)a.waves={{at(E::PATIENT,{{-22,0,-14},{24,0,-22}})}};
        if(i==1)a.waves={{at(E::PATIENT,{{-12,0,-16},{16,0,-24},{0,0,0}})},
                         {at(E::PATIENT,{{-20,0,-19},{20,0,-8}}),at(E::GRAFTED,{{0,0,-25}})}};
        if(i==2)a.waves={{at(E::GRAFTED,{{0,0,-18}}),at(E::PATIENT,{{-15,0,-9},{15,0,-3}})}};
        if(i==3)a.waves={{at(E::SUSPENDED,{{0,8,-20}}),at(E::PATIENT,{{-19,0,-4},{20,0,-16}})},
                         {at(E::SUSPENDED,{{-23,8,-20},{24,9,-19}}),at(E::GRAFTED,{{-16,0,-12}})}};
        if(i==5)a.waves={{at(E::SUSPENDED,{{-21,8,-12},{23,9,-18}}),at(E::PATIENT,{{0,0,-12},{28,0,-10},{-28,0,-7}})}};
        if(i==6)a.waves={{at(E::PHASED,{{0,0,-20}})},
                         {at(E::PHASED,{{-18,0,-20}}),at(E::PATIENT,{{14,0,-10},{-17,0,-5}}),at(E::SUSPENDED,{{24,8,-23}})}};
        if(i==7)a.waves={{at(E::PHASED,{{-16,0,-8},{20,0,-18}}),at(E::SUSPENDED,{{-26,8,-26}}),at(E::PATIENT,{{0,0,-16}})}};
        if(i==8)a.waves={{at(E::GRAFTED,{{0,0,-18}}),at(E::SUSPENDED,{{-24,8,-20}}),at(E::PHASED,{{21,0,-16}})},
                         {at(E::PHASED,{{-19,0,-13},{20,0,-18}}),at(E::PATIENT,{{-10,0,-17},{14,0,-8}})}};
        if(i==9){a.bossSpawn=c+vec3{0,0,-8};a.waves={{W(EnemyType::SUCCESSOR,1)}};}
        if(i==2||i==6||i==7||i==8)a.goals={WaveGoal::external(lab::objective(i))};
        l.arenas.push_back(a);
        if(i<9){
            const float end=z-80; ramp(exit,z-40,end,y,y-14);
            // A level threshold links the ramp to the following room with no ledge.
            solid({exit-6,y-14-.6f,end-1},{exit+6,y-14,end+.6f},steel);
            l.arenas.back().extraZones.push_back(LevelBuilder::aabb(exit-6,y-16,end-1,exit+6,y+7,z-39));
        }
        if(i==0){
            b.mat=Mat::CLINIC;
            // The glass theatre lies directly in the entrance camera's line of sight.
            solid({-10,y,z+8},{10,y+.6f,z+17},steel);
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
                vec3 p{x,y,z+24-n*15.f};bed(p,10+n+(x>0?4:0));
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
        if(i==2||i==3||i==6||i==9){
            for(float side:{-1.f,1.f}){
                solid(c+vec3{side<0?-39.f:23.f,5.4f,-27},c+vec3{side<0?-23.f:39.f,6,-5},steel);
                // Wide walkable ramps to both galleries, not vertical walls disguised as stairs.
                AABB area=LevelBuilder::aabb(side<0?-36:26,y,z-5,side<0?-26:36,y+6,z+19);
                l.ramps.push_back({area,2,6+y,y});
                b.kit().box(c+vec3{side*31,2.7f,7},{24.74f,.6f,10},steel,-1.5707963f,0,-.24497866f);
                if(i==9){
                    AABB rear=LevelBuilder::aabb(side<0?-36:26,y,z-39,side<0?-26:36,y+6,z-27);
                    l.ramps.push_back({rear,2,y,y+6});
                    b.kit().box(c+vec3{side*31,2.7f,-33},{13.4164f,.6f,10},steel,-1.5707963f,0,.4636476f);
                }
                b.neon(side*31-4,y+6.05f,z-26,side*31+4,y+6.12f,z-25.8f,cyan*.3f);
            }
        }
        if(i==2)console(c+vec3{-34,6,-23},"DISCHARGED / UNIT 18","THEY ASKED ME TO WALK IN A STRAIGHT LINE. I DID. THE ROOM IS STILL TURNING.");
        if(i==3)console(c+vec3{34,6,-23},"SURGICAL REGISTER","PROCEDURE COMPLETE. ANAESTHESIA DISCONTINUED. PATIENT HAS REPORTED DREAMING OF A ROOM WE HAVE NOT BUILT.");
        if(i==6)console(c+vec3{34,6,-22},"SUSPENSION","WE LOWERED THE BEDS. THE PATIENTS DID NOT FOLLOW. CONTACT SENSORS REMAIN WITHIN TOLERANCE.");
        if(i==9)console(c+vec3{31,6,-24},"FINAL BODY / RELEASE","SUCCESSOR: CROSSING STABLE. BEHAVIOURAL REVIEW OUTSTANDING. RELEASE AUTHORISATION REVOKED. DO NOT TELL THE SUBJECT.");
        if(i==2){
            b.mat=Mat::METAL;
            for(float xx:{-11.f,11.f}){
                solid(c+vec3{xx-1,0,-18},c+vec3{xx+1,.65f,-11},cream);
                prop(c+vec3{xx-.12f,.66f,-18},c+vec3{xx+.12f,.69f,-11},cyan*.6f);
            }
            lab::stencil(b,"WALK TOWARD THE LINE",{-10,y+.4f,z+39.3f},.1f);
        }
        if(i==3){
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
            for(float x:{-10.f,10.f})solid(c+vec3{x-2,-.1f,-25},c+vec3{x+2,.08f,23},{.22f,.38f,.42f});
            for(float x:{-10.f,10.f})l.water.push_back({LevelBuilder::aabb(x-1.9f,y-.3f,z-24.8f,x+1.9f,y,z+22.8f),y+.10f});
            // An ordinary drip stand, with its hose ending above dry soil.
            b.mat=Mat::METAL;solid(c+vec3{24,0,-25},c+vec3{24.08f,3.5f,-24.92f},steel);
            prop(c+vec3{23.5f,2.6f,-25.1f},c+vec3{23.8f,3.3f,-24.8f},{.55f,.7f,.65f});
            console(c+vec3{29,0,-28},"IRRIGATION","SUPPLY LINE DISCONNECTED. SOIL MOISTURE REMAINS CONSTANT. DO NOT ADJUST THE VALVE.");
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
        if(i>=7){
            // Exterior objects live beyond the windows, with real parallax and scale.
            for(int k=0;k<12;++k){float side=k%2?-1.f:1.f;vec3 p=c+vec3{side*(65+k*9.f),8.f+k*3,z*0.f-45.f+k*9};
                if(i==8){b.mat=Mat::ROCK;b.kit().rock(p+vec3{0,-140,0},13.f+k,210.f,{.025f,.09f,.12f},490+k,.7f,11);}
                else {b.kit().box(p,{5.f+k,2.f,12.f+k},steel,k*.3f);}
            }
        }
        b.mat=Mat::METAL;
        if(i!=4&&i!=5&&i!=9)for(vec3 p:{c+vec3{-12,0,-9},c+vec3{12,0,9}})solid(p+vec3{-2,0,-1},p+vec3{2,1.6f,1},steel);
        if(i==9){
            b.mat=Mat::CLINIC;
            for(float x:{-13.f,13.f})solid(c+vec3{x-1.2f,0,-26},c+vec3{x+1.2f,10,-23},cream);
            b.mat=Mat::METAL;b.kit().arch(c+vec3{0,10,-24.5f},26,7,1.25f,3,steel,0,20);
            b.kit().arch(c+vec3{0,10,-24.5f},23.5f,6, .18f,3.1f,cyan*.6f,0,20);
            b.kit().curve(c,15,0,6.2831853f,.025f,.045f,.12f,steel,64);
            for(int k=0;k<24;++k){float angle=k*.2617994f;vec3 p=c+vec3{std::cos(angle)*16,.04f,std::sin(angle)*16};
                b.kit().box(p,{1.2f,.025f,.14f},steel,-angle);
            }
            for(float x:{-25.f,25.f}){solid(c+vec3{x-2,0,-16},c+vec3{x+2,3,-12},steel);l.placedPickups.push_back({c+vec3{x,0,14},1});}}
        else l.placedPickups.push_back({c+vec3{entry+8,0,30},1});
    }
    const vec3 end=lab::centre(9);s.end=LevelBuilder::aabb(-6,end.y-1,end.z-116,6,end.y+6,end.z-101);
    b.mat=Mat::CLINIC;
    // A playable coda beyond the final assessment, inside the tenth room's region.
    solid(end+vec3{-6,-.7f,-118},end+vec3{6,0,-40},cream);
    l.arenas.back().extraZones.push_back(LevelBuilder::aabb(-7,end.y-300,end.z-120,7,end.y+28,end.z-39));
    l.basins.push_back({LevelBuilder::aabb(-8,end.y-300,end.z-120,8,end.y+30,end.z-40),end.y-250});
    // Two optional records reward looking above the test floor and behind the impossible garden.
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
    return l;
}
