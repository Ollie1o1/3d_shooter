// The Living Ward: real controller traversal, finite encounter flow, cinematic/save state,
// original bodies and committed Successor attacks. No OpenGL context required.
#include "core/Balance.h"
#include "world/LevelLab.h"
#include "world/ZoneConstraint.h"
#include "game/WaveDirector.h"
#include "game/StorySave.h"
#include "game/LabIntro.h"
#include "bosses/successor/SuccessorHazards.h"
#include "render/EnemyModel.h"
#include "render/LightBake.h"
#include "audio/MusicSynth.h"
#include <cstdio>
#include <set>
static int failures=0;
#define CHECK(c,m) do{if(!(c)){++failures;std::printf("FAIL: %s\n",m);}else std::printf("ok: %s\n",m);}while(0)
constexpr float DT=1.f/60;
struct Walk {
    LevelData& l;SpatialGrid grid;Player p;
    Walk(LevelData& level,glm::vec3 start):l(level),p(start){grid.build(l.walls);p.dynWalls=l.moverWalls.data();p.dynCount=(int)l.moverWalls.size();}
    void step(glm::vec3 goal){
        Uint8 keys[SDL_NUM_SCANCODES]={};auto d=goal-p.position;d.y=0;float n=glm::length(d);
        p.camera.yaw=glm::degrees(std::atan2(d.z,d.x));keys[SDL_SCANCODE_W]=n>.2f;
        float along=n>.01f?glm::dot(glm::vec2{p.velocity.x,p.velocity.z},glm::vec2{d.x,d.z}/n):0.f;
        if(n<1.5f && along>n*4){keys[SDL_SCANCODE_W]=0;keys[SDL_SCANCODE_S]=1;}
        applyWater(p,l);p.update(DT,keys,l.walls.data(),(int)l.walls.size(),false,&grid);
        std::vector<const AABB*> zones;for(auto& a:l.arenas){zones.push_back(&a.zone);for(auto& z:a.extraZones)zones.push_back(&z);}
        constrainToZones(p.position,p.velocity,p.height,zones,true);
    }
    bool to(glm::vec3 goal){for(int f=0;f<1600;++f){if(glm::length(glm::vec2{p.position.x-goal.x,p.position.z-goal.z})<.6f && std::fabs(p.position.y-goal.y)<.3f && p.onGround)return true;step(goal);}
        std::printf("  stuck %.2f %.2f %.2f -> %.2f %.2f %.2f\n",p.position.x,p.position.y,p.position.z,goal.x,goal.y,goal.z);return false;}
};
#include "LabBossRun.h"
static Enemy ready(EnemyType t,glm::vec3 p={0,0,0}){Enemy e(t,p,p.y);e.spawnTimer=0;e.state=EnemyState::ACTIVE;return e;}
int main(){
    balanceInit();auto L=buildLab();
    CHECK(L.arenas.size()==10 && L.story.chapter==3 && L.story.bossSection==9 && L.story.startWeapons==15,"ten lab rooms, final assessment tenth, all carried guns and grapple");
    std::string bakeError;CHECK(!lightbake::load("story3",L,bakeError).empty(),"the full lab ships with current baked lighting");
    for(int d=0;d<(int)L.doors.size();++d)L.setDoorInstant(d,true);
    bool starts=true;for(int i=0;i<10;++i){Walk w(L,L.arenas[i].playerStart);for(int f=0;f<120;++f)w.step(w.p.position);starts&=L.arenaAt(w.p.position)==i && glm::length(w.p.position-L.arenas[i].playerStart)<.3f;}
    CHECK(starts,"every checkpoint stands on real ground inside its own room");
    Walk walk(L,L.arenas[0].playerStart);bool route=true;
    for(int i=0;i<9&&route;++i){auto c=lab::centre(i);float entry=i?lab::routeX(i-1):0,exit=lab::routeX(i);
        if(i==0)route&=walk.to(c+glm::vec3{-18,0,22})&&walk.to(c+glm::vec3{-18,0,-25});
        else if(i==5)route&=walk.to(c+glm::vec3{0,0,28})&&walk.to(c+glm::vec3{0,0,-28});
        else if(i==7||i==8)route&=walk.to(c+glm::vec3{entry,0,-17})&&walk.to(c+glm::vec3{38,0,-17})&&walk.to(c+glm::vec3{38,0,-36});
        else route&=walk.to(c+glm::vec3{entry,0,-28});
        route&=walk.to(c+glm::vec3{exit,0,-32})&&walk.to(c+glm::vec3{exit,0,-39})&&walk.to(lab::centre(i+1)+glm::vec3{exit,0,31});
        std::printf("  route room %d: %s\n",i+1,route?"clear":"blocked");
    }
    CHECK(route,"entire descent uses real walking ramps and room boundaries without mandatory jump, dash or teleport");
    bool galleries=true;for(int i:{2,3,6,9})for(float side:{-1.f,1.f}){auto c=lab::centre(i);Walk w(L,c+glm::vec3{side*31,0,23});galleries&=w.to(c+glm::vec3{side*31,6,-12})&&w.to(c+glm::vec3{side*31,0,23});}
    CHECK(galleries,"all combat galleries can be walked up and down, including the boss arena");
    bool clear=true;int count=0;for(const auto& a:L.arenas)for(const auto& wave:a.waves)for(const auto& e:wave)for(auto p:e.at){++count;float r=statsOf(e.type).radius,h=statsOf(e.type).height;AABB capsule{p+glm::vec3{-r,.08f,-r},p+glm::vec3{r,h,r}};
        for(const auto& wall:L.walls){const auto& b=wall.box;if(capsule.min.x<b.max.x&&capsule.max.x>b.min.x&&capsule.min.y<b.max.y&&capsule.max.y>b.min.y&&capsule.min.z<b.max.z&&capsule.max.z>b.min.z){clear=false;std::printf("  blocked %s %.1f %.1f %.1f\n",statsOf(e.type).name,p.x,p.y,p.z);break;}}}
    CHECK(clear && count>35,"all authored enemies have full body clearance at their actual spawn");
    CHECK(L.arenas[4].scripted && L.arenas[4].waves.empty(),"the impossible garden stays a quiet recovery space");
    bool flow=true;for(int i=0;i<10;++i)if(i!=4){WaveDirector d;d.level=&L;d.fast=true;d.startArena(i);int emitted=0;
        for(int f=0;f<900 && d.arena==i && d.phase!=WaveDirector::Phase::VICTORY;++f){std::vector<SpawnRequest> spawns;d.update(DT,0,L.arenas[i].playerStart,spawns);emitted+=(int)spawns.size();if(f>300)d.completeGoal();}
        flow&=emitted>0 && (d.arena==i+1||d.phase==WaveDirector::Phase::VICTORY);}
    CHECK(flow,"every finite encounter and assessment lock can advance with no endless refill");
    bool bridges=true;for(const auto& route:L.story.labBridges){auto& m=L.movers[route.mover];auto c=lab::centre(route.room);float x=route.side*20.f;
        bridges&=L.groundAt(x,c.z-27,c.y)<c.y-1;m.drive=1;L.updateMovers(0);
        bridges&=std::fabs(L.groundAt(x,c.z-27,c.y)-c.y)<.02f;Walk w(L,c+glm::vec3{x,0,-17});bridges&=w.to(c+glm::vec3{x,0,-36});m.drive=0;L.updateMovers(0);}
    CHECK(bridges,"each contact deploys real bridge collision and provides a walkable alternative across the observation gap");
    auto end=lab::centre(9);Walk coda(L,end+glm::vec3{0,0,-32});CHECK(coda.to(end+glm::vec3{0,0,-110}),"boss exit leads to a physically playable fractured garden threshold");
    LabIntro intro;intro.start(false);for(int f=0;f<60*10;++f)intro.update(DT,false);CHECK(intro.active() && intro.patientRise()<.1f,"the entry anomaly waits for a sustained ordinary motor test");
    for(int f=0;f<60*14;++f)intro.update(DT,false);CHECK(!intro.active()&&intro.seen,"normal cutscene completion marks it seen exactly once");
    intro.start(false);for(int f=0;f<100;++f)intro.update(DT,true);CHECK(!intro.active()&&intro.seen,"holding the bound skip action exits cleanly");intro.start(true);CHECK(!intro.active(),"seen cutscenes stay skipped on retry/Continue");
    StorySave save;save.chapter=3;save.checkpoint=8;save.labIntroSeen=true;save.labRoutes=(1u<<7)|(1u<<10);StorySave back;
    CHECK(back.parse(save.text()) && back.labIntroSeen && back.labRoutes==save.labRoutes,"lab intro and chosen observation contact survive save round-trip");
    StorySave old;auto v1=old.text();v1.replace(v1.find(" 2\n"),3," 1\n");v1=v1.substr(0,v1.rfind('\n',v1.size()-2)+1);
    CHECK(back.parse(v1)&&back.chapter==1&&!back.labIntroSeen&&back.labRoutes==0,"version-one Story saves migrate without inventing lab progress");
    bool bodies=true;for(auto type:{EnemyType::PATIENT,EnemyType::GRAFTED,EnemyType::SUSPENDED,EnemyType::PHASED,EnemyType::SUCCESSOR}){auto e=ready(type);AABB head;std::vector<BoxInstance> boxes;rig::buildEnemy(e,0,boxes);bodies&=enemyfile::model(type)&&boxes.size()>40&&rig::headBox(e,head)&&head.max.y>e.height()*.85f;}
    CHECK(bodies,"five original articulated models load, animate and expose a head hitbox in the visible body");
    bool recordBodies=true;for(int n:{18,21}){auto file=gunkit::readGunFile("assets/enemies/unit"+std::to_string(n)+".glb",enemyfile::names());recordBodies&=file.error.empty()&&file.parts.size()>200;}
    CHECK(recordBodies,"both numbered secret bodies ship as complete readable wrecks");
    EnemyWorld w;w.bounds={{-40,0,-38},{40,22,38}};w.playerFeet={0,0,16};w.playerEye={0,1.7f,16};w.playerVel={8,0,0};
    auto boss=ready(EnemyType::SUCCESSOR);boss.suPhase=1;boss.attackTimer=100;boss.suRotation=1;boss.update(DT,w);
    CHECK(boss.attack==AttackKind::SU_VECTOR && boss.telegraphTimer>.7f,"Vector Run begins with a long readable lane tell");
    while(boss.telegraphTimer>.25f)boss.update(DT,w);auto locked=boss.suAim;w.playerFeet={20,0,10};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};while(boss.telegraphTimer>0)boss.update(DT,w);
    CHECK(glm::length(locked-boss.suAim)<.01f&&boss.suBurst>0,"a late direction reversal cannot rotate the committed Vector lane");
    boss.suNodes=2;boss.suCross=.2f;boss.suStep=2;int epoch=boss.suEpoch;boss.stagger(1.35f);
    CHECK(boss.suBurst==0&&boss.suNodes==0&&boss.suCross==0&&boss.suStep==0&&boss.suEpoch>epoch,"a parry cancels every pending movement, node and combo continuation");
    auto white=ready(EnemyType::SUCCESSOR);white.attack=AttackKind::SU_THRUST;white.telegraphTimer=.2f;CHECK(white.parryWindow(),"the third Incision strike has a real white parry window");white.attack=AttackKind::SU_INCISION;CHECK(!white.parryWindow(),"gold Incision strikes cannot be parried");
    auto phase=ready(EnemyType::SUCCESSOR);phase.update(DT,w);phase.health=phase.maxHealth*.64f;phase.update(DT,w);bool two=phase.suPhase==2;phase.health=phase.maxHealth*.29f;phase.update(DT,w);CHECK(two&&phase.suPhase==3&&phase.alive,"one health bar makes the two explicit phase transitions without refills");
    SuccessorHazards hazard;hazard.circle({0,0,0},1,24);float damage=0;hazard.update(.9f,{0,0,0},damage);bool safe=damage==0;hazard.update(.11f,{0,0,0},damage);bool hits=damage==24;hazard.update(.1f,{0,0,0},damage);CHECK(safe&&hits&&damage==0,"Recall gives a full warning, hits once and never ticks repeatedly");
    CHECK(musicTrack(13).bpm==176 && std::string(musicTrack(13).name)=="RELEASE DENIED" && musicTrack(13).techno==0,"Successor routes to its original fast choir/organ arrangement");
    auto reactive=labBossRun(buildLab(),true,WeaponId::REVOLVER),passive=labBossRun(buildLab(),false,WeaponId::REVOLVER);
    std::printf("  Successor real revolver rehearsal: %.1fs / %d hits / %d parries / damage %.0f, passive %.0f / %zu moves\n",reactive.seconds,reactive.hits,reactive.parries,reactive.damage,passive.damage,reactive.moves.size());
    CHECK(reactive.won && reactive.lowest>0 && reactive.seconds>60 && reactive.seconds<250,"the real boss can be defeated with an unupgraded gun, normal movement and bounded parries");
    CHECK(reactive.moves.size()>=7 && reactive.damage<passive.damage,"all seven attacks occur in a full fight and reading the tells reduces incoming damage");
    return failures?1:0;
}
