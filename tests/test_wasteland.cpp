// Chapter 1: real Player collision, encounter progression and the Sorter's real AI.
#include "player/Player.h"
#include "core/Balance.h"
#include "world/LevelWasteland.h"
#include "game/WaveDirector.h"
#include "game/StoryLift.h"
#include "world/Interactable.h"
#include "world/ZoneConstraint.h"
#include "player/Projectile.h"
#include "world/ArenaShifts.h"
#include "bosses/sovereign/SovereignHazards.h"
#include "bosses/warden/WardenHazards.h"
#include "bosses/leviathan/LeviathanHazards.h"
#include "render/EnemyModel.h"
#include "render/LightBake.h"
#include "BossSim.h"
#include <set>
#include <cstdio>
#include <cstring>
static int failures=0;
#define CHECK(c,m) do{if(!(c)){++failures;std::printf("FAIL: %s\n",m);}else std::printf("ok: %s\n",m);}while(0)
constexpr float DT=1.f/60;
struct Walk {
    LevelData& l; SpatialGrid grid; Player p; int tick=0; WaveDirector* boundaries=nullptr;
    Walk(LevelData& level,glm::vec3 start):l(level),p(start) {grid.build(l.walls);p.dynWalls=l.moverWalls.data();p.dynCount=(int)l.moverWalls.size();}
    void step(glm::vec3 goal,bool jump=false,bool slide=false) {
        Uint8 keys[SDL_NUM_SCANCODES]={};
        glm::vec3 d=goal-p.position;d.y=0;float n=glm::length(d);
        p.camera.yaw=glm::degrees(std::atan2(d.z,d.x));
        keys[SDL_SCANCODE_W]=n>.2f;
        float toward=n>.01f?glm::dot(glm::vec2{p.velocity.x,p.velocity.z},glm::vec2{d.x,d.z}/n):0.f;
        if(n<1.5f && toward>n*4.f){keys[SDL_SCANCODE_W]=0;keys[SDL_SCANCODE_S]=1;}
        keys[SDL_SCANCODE_SPACE]=jump && tick%48<29;
        keys[SDL_SCANCODE_LCTRL]=slide && (p.sliding || toward>4.3f);
        applyWater(p,l);
        p.update(DT,keys,l.walls.data(),(int)l.walls.size(),false,&grid);++tick;
        if(boundaries) {
            std::vector<const AABB*> zones;
            int last=std::min(boundaries->arena+(boundaries->fighting()?0:1),(int)l.arenas.size()-1);
            for(int i=0;i<=last;++i) {
                zones.push_back(&l.arenas[i].zone);
                for(const auto& z:l.arenas[i].extraZones)zones.push_back(&z);
            }
            const auto* zone=constrainToZones(p.position,p.velocity,p.height,zones,true);
            if(zone && p.position.y+p.height>zone->max.y) {
                p.position.y=zone->max.y-p.height;
                if(p.velocity.y>0)p.velocity.y=-1;
            }
        }
    }
    bool to(glm::vec3 goal,bool jump=false,bool slide=false) {
        tick=0;
        for(int i=0;i<1200;++i) {
            if(glm::length(glm::vec2(p.position.x-goal.x,p.position.z-goal.z))<.55f && std::fabs(p.position.y-goal.y)<.3f && p.onGround)return true;
            step(goal,jump,slide);
        }
        std::printf("  stuck at %.2f %.2f %.2f, wanted %.2f %.2f %.2f\n",p.position.x,p.position.y,p.position.z,goal.x,goal.y,goal.z);return false;
    }
};
int main(){
    balanceInit();auto L=buildWasteland();
    CHECK(L.arenas.size()==8&&L.story.chapter==2,"eight wasteland regions and chapter metadata");
    CHECK(L.story.startWeapons==7&&L.story.startGrapple,"Chapter 2 starts with Cradle gear and grapple");
    for(int i=0;i<(int)L.doors.size();++i)L.setDoorInstant(i,true);
    bool checkpoints=true;
    for(int i=0;i<8;++i){Walk w(L,L.arenas[i].playerStart);for(int k=0;k<120;++k)w.step(w.p.position);
        bool ok=L.arenaAt(w.p.position)==i&&glm::length(w.p.position-L.arenas[i].playerStart)<.3f;
        std::printf("  checkpoint %d: %.2f %.2f %.2f\n",i,w.p.position.x,w.p.position.y,w.p.position.z);checkpoints&=ok;}
    CHECK(checkpoints,"every checkpoint rests on real ground in its own region");
    WaveDirector d;d.level=&L;d.fast=true;d.approach(7);
    Walk w(L,L.arenas[0].playerStart);w.boundaries=&d;
    auto route=[&](std::initializer_list<glm::vec3> points){for(auto p:points)if(!w.to(p))return false;return true;};
    CHECK(route({{0,0,18},{0,0,-9},{0,0,-34},{-31,0,-51},{-39,0,-51},{-56,-5,-51}}),"walk from factory through convoy and down relay ramp without jumping or dashing");
    CHECK(route({{-59,-5,-58},{-74,-5,-58},{-74,0,-80}}),"relay exit ramp climbs to daylight with continuous bounds");
    CHECK(route({{-74,0,-91},{-61,-3,-91},{-36,0,-91},{-36,0,-104},{-23,0,-104}}),"broad maintenance bypass crosses the canyon without forced precision movement");
    CHECK(route({{-8,0,-104},{20,0,-89},{35,0,-69}}),"Longshot wreck is reachable from turbine entrance");
    CHECK(route({{20,0,-89},{-8,0,-104},{-8,0,-118},{23,0,-120},{29,0,-132},{29,0,-160},{30,0,-176}}),"turbines and toll connect through ordinary walking thresholds");
    CHECK(route({{9,0,-190},{-13.5f,0,-190},{-13.5f,0,-245},{15,0,-245},{28,0,-246},{28,0,-282}}),"basin has a wide continuous route to the chapter ending");
    Walk deck(L,{11,0,-92});deck.boundaries=&d;
    CHECK(deck.to({11,3.2f,-73})&&std::fabs(deck.p.position.y-3.2f)<.2f&&deck.to({11,0,-93}),
          "optional turbine maintenance deck is accessible uphill and downhill without jumping");
    Walk shelter(L,{-32,0,-26});shelter.boundaries=&d;
    CHECK(shelter.to({-32,0,-41}),"convoy windbreak is an open flank that can be crossed in normal movement");
    Walk relay(L,{-85,-5,-46});relay.boundaries=&d;
    CHECK(relay.to({-85,-5,-38})&&relay.to({-96,-5,-38})&&relay.to({-85,-5,-38}),"buried relay secret is connected in both directions");
    Walk toll(L,{44,0,-133});toll.boundaries=&d;
    CHECK(toll.to({44,3,-148})&&toll.to({51,3,-151})&&toll.to({44,3,-148})&&toll.to({44,0,-133}),"roof secret ramp works uphill and downhill without a jump");
    {
        glm::vec3 eye{-64,1.7f,-104},aim=glm::normalize(glm::vec3{-55,9.5f,-104}-eye);
        float nearest=50.f;glm::vec3 hook{0.f};
        for(const auto& wall:L.walls){float t=rayBoxHit(eye,aim,wall.box);if(t>0&&t<nearest){nearest=t;hook=eye+aim*t;}}
        CHECK(nearest<20&&hook.y>=8.8f,"span crossbeam is a real unoccluded grapple target within range");
        CHECK(L.baseFloor(-56,-104)==-12&&L.arenas[3].voidY>-12&&L.arenas[3].hasRespawn,"broken deck has a true void with a recovery checkpoint above its bottom");
    }
    bool hoverFloors=true;
    for(const auto& a:L.arenas)for(const auto& wave:a.waves)for(const auto& entry:wave)for(auto p:entry.at)
        hoverFloors &= std::fabs(L.enemyFloor((int)(&a-L.arenas.data()),p,statsOf(entry.type).flying,a.playerStart.y)-a.playerStart.y)<.01f;
    CHECK(hoverFloors,"enemy and flyer ground references match each district rather than the canyon bottom");
    CHECK(L.enemyFloor(6,L.arenas[6].bossSpawn,true,0)==0,"Ballast stands on the Cut road level");
    bool clear=true;int count=0;
    for(auto& a:L.arenas)for(auto& wave:a.waves)for(auto& e:wave)for(auto p:e.at){++count;float r=statsOf(e.type).radius,h=statsOf(e.type).height;
        AABB b{p+glm::vec3{-r,.08f,-r},p+glm::vec3{r,h,r}};
        for(const auto& wall:L.walls)if(b.min.x<wall.box.max.x&&b.max.x>wall.box.min.x&&b.min.y<wall.box.max.y&&b.max.y>wall.box.min.y&&b.min.z<wall.box.max.z&&b.max.z>wall.box.min.z){clear=false;std::printf("  blocked %s at %.1f %.1f %.1f\n",statsOf(e.type).name,p.x,p.y,p.z);break;}}
    CHECK(clear&&count>=14,"all authored wave spawns have enemy capsule clearance");
    bool flow=true;int bosses=0;
    for(int room:{1,2,4,6}){WaveDirector director;director.level=&L;director.fast=true;director.startArena(room);int spawns=0;
        for(int f=0;f<600&&director.arena==room&&director.phase!=WaveDirector::Phase::VICTORY;++f){std::vector<SpawnRequest> out;director.update(DT,0,room==2?L.arenas[room].goals[0].pos:L.arenas[room].playerStart,out);if(room==4&&f==120)director.completeGoal();for(auto s:out){++spawns;bosses+=s.type==EnemyType::BALLAST;}}
        flow&=spawns>0&&(director.arena==room+1||director.phase==WaveDirector::Phase::VICTORY);}
    CHECK(flow&&bosses==1,"all four encounters advance through distinct goals and only the Cut spawns Ballast");
    CHECK(L.anchors.size()==3&&!L.hasReactor&&L.anchorsAlive(LevelData::ChainAnchor::BRAKE)==3,"three shootable turbine brakes and no reused Warden reactor");
    {
        SpatialGrid g;g.build(L.walls);BossSim sim(L,g,L.arenas[6],EnemyType::BALLAST,BossSim::SOLID);
        for(int f=0;f<60*240&&sim.bossAlive();++f)sim.step(DT);
        std::printf("  Ballast kill %.1fs, damage %.0f, parries %d\n",sim.t,sim.damageTaken,sim.parries);
        CHECK(!sim.bossAlive()&&sim.t>20&&sim.t<240,"Ballast can be defeated with guns, movement and cable parries in the new Cut");
    }
    lightbake::Bake bake;std::string err;CHECK(!(bake=lightbake::load("story2",buildWasteland(),err)).empty(),"Chapter 2 light bake matches final geometry");
    std::printf("%s (%d failures)\n",failures?"FAILED":"ALL PASSED",failures);return failures?1:0;
}
