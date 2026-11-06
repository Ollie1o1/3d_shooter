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
    LevelData& l;SpatialGrid grid;Player p;bool falseFall=false;
    Walk(LevelData& level,glm::vec3 start):l(level),p(start){grid.build(l.walls);p.dynWalls=l.moverWalls.data();p.dynCount=(int)l.moverWalls.size();}
    void step(glm::vec3 goal){
        Uint8 keys[SDL_NUM_SCANCODES]={};auto d=goal-p.position;d.y=0;float n=glm::length(d);
        p.camera.yaw=glm::degrees(std::atan2(d.z,d.x));keys[SDL_SCANCODE_W]=n>.2f;
        float along=n>.01f?glm::dot(glm::vec2{p.velocity.x,p.velocity.z},glm::vec2{d.x,d.z}/n):0.f;
        if(n<1.5f && along>n*4){keys[SDL_SCANCODE_W]=0;keys[SDL_SCANCODE_S]=1;}
        applyWater(p,l);p.update(DT,keys,l.walls.data(),(int)l.walls.size(),false,&grid);
        std::vector<const AABB*> zones;for(auto& a:l.arenas){zones.push_back(&a.zone);for(auto& z:a.extraZones)zones.push_back(&z);}
        constrainToZones(p.position,p.velocity,p.height,zones,true);
        int room=l.arenaAt(p.position);
        if(room>=0 && p.position.y<l.voidHeightAt(room,p.position))falseFall=true;
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
    for(int i=0;i<9&&route;++i){auto c=lab::centre(i);float entry=lab::entryX(i),exit=lab::routeX(i);
        if(i==0)route&=walk.to(c+glm::vec3{-18,0,22})&&walk.to(c+glm::vec3{-18,0,-25});
        else if(i==3)route&=walk.to(c+glm::vec3{19,0,20})&&walk.to(c+glm::vec3{19,0,-28});
        else if(i==5)route&=walk.to(c+glm::vec3{0,0,28})&&walk.to(c+glm::vec3{0,0,-28});
        else if(i==7||i==8){float side=i==7?18.f:-18.f;
            route&=walk.to(c+glm::vec3{side,0,22})&&walk.to(c+glm::vec3{side,0,-17})&&walk.to(c+glm::vec3{38,0,-17})&&walk.to(c+glm::vec3{38,0,-36});}
        else route&=walk.to(c+glm::vec3{entry,0,-28});
        route&=walk.to(c+glm::vec3{exit,0,-32});
        for(auto point:lab::passageRoute(i))if(route)route&=walk.to(point);
        std::printf("  route room %d: %s\n",i+1,route?"clear":"blocked");
    }
    CHECK(route,"entire descent uses real walking ramps and room boundaries without mandatory jump, dash or teleport");
    CHECK(!walk.falseFall,"normal traversal, including lower flight landings, never crosses a fall recovery plane");
    bool reverse=true;for(int i=0;i<9;++i){Walk w(L,L.arenas[i+1].playerStart);auto points=lab::passageRoute(i);
        for(auto point=points.rbegin();point!=points.rend() && reverse;++point)reverse&=w.to(*point);
        reverse&=!w.falseFall;}
    CHECK(reverse,"every bent passage can also be walked uphill without a false fall reset");
    {
        // Recovery's front bay is narrower than its room-wide bounds. Test
        // the full doorway width, since the centre route missed the escape.
        const auto c=lab::centre(1);const float entry=lab::entryX(1),front=c.z+40;
        bool sealed=true;
        for(float side:{-5.f,0.f,5.f}) {
            glm::vec3 from{entry+side,L.baseFloor(entry+side,front+2),front+2};
            Walk approach(L,from);
            sealed&=approach.to({entry+side,c.y,front-6}) && !approach.falseFall;
        }
        CHECK(sealed,"the whole Recovery Ward doorway leads onto its actual floor, including both corridor edges");
        const auto bay=lab::footprint(1).front();
        bool jambs=true;
        for(float side:{-1.f,1.f}) {
            float x=side<0?bay.left+.9f:bay.right-.9f;
            Walk edge(L,{x,c.y,front-2});
            for(int f=0;f<180;++f)edge.step({x+side*6,c.y,front-2});
            jambs&=!edge.falseFall && edge.p.position.x>bay.left && edge.p.position.x<bay.right
                && std::fabs(edge.p.position.y-c.y)<.1f;
        }
        CHECK(jambs,"both Recovery Ward entrance walls physically stop attempts to walk outside beside the door");
    }
    bool galleries=true;for(int i:{2,3,6,9})for(float side:{-1.f,1.f}){auto c=lab::centre(i);float x=side*(i==9?62:31),height=i==9?8:6,start=i==9?28:20;Walk w(L,c+glm::vec3{x,0,start});galleries&=w.to(c+glm::vec3{x,height,-18})&&w.to(c+glm::vec3{x,0,start});
        if(i==9)galleries&=w.to(c+glm::vec3{x,height,-18})&&w.to(c+glm::vec3{x,0,-47})&&w.to(c+glm::vec3{x,height,-18})&&!w.falseFall;}
    CHECK(galleries,"all combat galleries can be walked up and down, including the boss arena");
    bool loops=true;for(int room:{2,6}){auto c=lab::centre(room);float bridge=room==2?-22.5f:-15.5f;
        Walk upper(L,c+glm::vec3{-31,0,20});
        loops&=upper.to(c+glm::vec3{-31,6,bridge})&&upper.to(c+glm::vec3{31,6,bridge})&&upper.to(c+glm::vec3{31,0,20})&&!upper.falseFall;}
    CHECK(loops,"Motor Assessment and Long-Term Care have complete upper loops joining both floor approaches");
    bool circuits=true;
    for(int room:{2,6}) {
        auto c=lab::centre(room);float z=room==2?6.f:15.f;
        Walk circuit(L,c+glm::vec3{-17,0,z+16});
        circuits&=circuit.to(c+glm::vec3{-17,2.4f,z}) && circuit.to(c+glm::vec3{17,2.4f,z})
            && circuit.to(c+glm::vec3{17,0,z+16}) && !circuit.falseFall;
    }
    CHECK(circuits,"assessment apparatus forms a second complete fighting circuit with real ramps and roof collision");
    auto motor=lab::centre(2),garden=lab::centre(4);
    Walk maintenance(L,motor+glm::vec3{-31,0,20}),observation(L,garden+glm::vec3{0,0,28});
    CHECK(maintenance.to(motor+glm::vec3{-31,6,-12}) && maintenance.to(L.story.wrecks[0].pos)
        && observation.to(garden+glm::vec3{38,0,28}) && observation.to(garden+glm::vec3{38,0,-34}) && observation.to(L.story.wrecks[1].pos),
        "both secret records have real walkable discovery routes without blind leaps");
    bool clear=true;int count=0;for(const auto& a:L.arenas)for(const auto& wave:a.waves)for(const auto& e:wave)for(auto p:e.at){++count;float r=statsOf(e.type).radius,h=statsOf(e.type).height;AABB capsule{p+glm::vec3{-r,.08f,-r},p+glm::vec3{r,h,r}};
        for(const auto& wall:L.walls){const auto& b=wall.box;if(capsule.min.x<b.max.x&&capsule.max.x>b.min.x&&capsule.min.y<b.max.y&&capsule.max.y>b.min.y&&capsule.min.z<b.max.z&&capsule.max.z>b.min.z){clear=false;std::printf("  blocked %s %.1f %.1f %.1f\n",statsOf(e.type).name,p.x,p.y,p.z);break;}}}
    CHECK(clear && count>35,"all authored enemies have full body clearance at their actual spawn");
    bool density=true, support=true, separated=true;
    const int roomCounts[]={10,24,14,20,0,14,16,14,22};
    for(int room=0;room<9;++room){
        const auto& a=L.arenas[room];int total=0;
        for(const auto& wave:a.waves){
            std::vector<std::pair<glm::vec3,float>> bodies;
            for(const auto& e:wave)for(auto p:e.at){
                ++total;const auto& stats=statsOf(e.type);
                bool valid=spawnClearance(L,p,stats.radius,stats.height,!stats.flying) && L.arenaAt(p)==room;
                if(!valid)std::printf("  unsupported %s room %d %.1f %.1f %.1f\n",stats.name,room,p.x,p.y,p.z);
                support&=valid;
                for(const auto& body:bodies){
                    bool clear=glm::length(p-body.first)>=stats.radius+body.second;
                    if(!clear)std::printf("  overlapping room %d %.1f %.1f %.1f\n",room,p.x,p.y,p.z);
                    separated&=clear;
                }
                bodies.push_back({p,stats.radius});
            }
        }
        density&=total==roomCounts[room] && (room==4 || a.maxAlive==14);
        if(room!=4){
            WaveDirector d;d.level=&L;d.fast=true;d.startArena(room);
            std::vector<SpawnRequest> spawned;d.update(DT,0,a.playerStart,spawned);d.update(DT,0,a.playerStart,spawned);
            int first=0;for(const auto& e:a.waves.front())first+=(int)e.at.size();
            density&=(int)spawned.size()==first && d.queue.empty();
        }
    }
    CHECK(density,"Chapter 3 fields twice the enemies in every combat room, with up to fourteen together");
    CHECK(support,"every Chapter 3 spawn has full body clearance and ground enemies have floor support");
    CHECK(separated,"doubled Chapter 3 waves use separate positions without overlapping enemy bodies");
    int bosses=0;for(const auto& wave:L.arenas[9].waves)for(const auto& e:wave)
        if(isBoss(e.type))bosses+=e.at.empty()?e.count:(int)e.at.size();
    CHECK(bosses==1,"the final assessment remains one Successor");
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
    LabIntro intro;intro.start(false);
    CHECK(intro.exterior() && intro.eye().y<LabIntro::PLAIN+2 && intro.carY()==LabIntro::PLAIN,"the arrival begins on Chapter 2's road at the tower's foot, the lift car waiting on the plain");
    for(int f=0;f<60*10;++f)intro.update(DT,false);CHECK(intro.active() && intro.carY()<LabIntro::PLAIN+LabIntro::TOWER,"ten seconds in, the car is still climbing inside the tower");
    for(int f=0;f<60*12;++f)intro.update(DT,false);CHECK(intro.active() && std::fabs(intro.carY())<1e-3f && intro.frontDoors()>.99f && intro.skyMix()>.99f,"the car docks at Admissions' floor and its doors open under the void");
    {LabTheatre theatre;CHECK(theatre.rise()==0 && !theatre.floating(),"the theatre's subject walks the floor until the player comes close");theatre.time=8;CHECK(theatre.floating() && theatre.rise()>1.3f,"then it lifts off it");}
    for(int f=0;f<60*12;++f)intro.update(DT,false);
    CHECK(!intro.active()&&intro.seen,"normal cutscene completion marks it seen exactly once");
    intro.start(false);for(int f=0;f<100;++f)intro.update(DT,true);CHECK(intro.active()&&!intro.seen,"holding Space from chapter selection cannot accidentally skip the intro");
    intro.update(DT,false);for(int f=0;f<45;++f)intro.update(DT,true);CHECK(!intro.active()&&intro.seen,"release then hold the bound skip action exits cleanly");intro.start(true);CHECK(!intro.active(),"seen cutscenes stay skipped on retry/Continue");
    bool localFall=true;for(int i=0;i<9;++i){auto c=lab::centre(i);auto lower=c+glm::vec3{lab::entryX(i+1),-14,-79};
        int owner=L.arenaAt(lower);localFall&=owner==i && lower.y>L.voidHeightAt(owner,lower);
        lower.y-=14;localFall&=lower.y<L.voidHeightAt(owner,lower);}
    CHECK(localFall,"lower passage floors are safe but real falls below them still trigger recovery");
    auto hadal=lab::centre(8);Walk shelf(L,hadal+glm::vec3{-36,0,21});
    CHECK(shelf.to(hadal+glm::vec3{-36,8,-8})&&shelf.to(hadal+glm::vec3{-36,8,-20})&&shelf.to(hadal+glm::vec3{-36,0,-36}),"Hadal's elevated shelf makes a complete alternate route with an exit ramp");
    StorySave save;save.chapter=3;save.checkpoint=8;save.labIntroSeen=true;save.labRoutes=(1u<<7)|(1u<<10);StorySave back;
    CHECK(back.parse(save.text()) && back.labIntroSeen && back.labRoutes==save.labRoutes,"lab intro and chosen observation contact survive save round-trip");
    StorySave old;auto v1=old.text();v1.replace(v1.find(" 3\n"),3," 1\n");for(int cut=0;cut<2;++cut)v1=v1.substr(0,v1.rfind('\n',v1.size()-2)+1);   // (v1: without v2's lab line and v3's sword line)
    save.sword=true;CHECK(back.parse(save.text()) && back.sword,"OATHBREAKER, once pulled, survives a save round-trip");
    CHECK(back.parse(v1)&&back.chapter==1&&!back.labIntroSeen&&back.labRoutes==0,"version-one Story saves migrate without inventing lab progress");
    bool bodies=true;for(auto type:{EnemyType::PATIENT,EnemyType::GRAFTED,EnemyType::SUSPENDED,EnemyType::PHASED,EnemyType::SUCCESSOR}){auto e=ready(type);AABB head;std::vector<BoxInstance> boxes;rig::buildEnemy(e,0,boxes);bodies&=enemyfile::model(type)&&boxes.size()>40&&rig::headBox(e,head)&&head.max.y>e.height()*.85f;}
    CHECK(bodies,"five original articulated models load, animate and expose a head hitbox in the visible body");
    bool recordBodies=true;for(int n:{18,21}){auto file=gunkit::readGunFile("assets/enemies/unit"+std::to_string(n)+".glb",enemyfile::names());recordBodies&=file.error.empty()&&file.parts.size()>200;}
    CHECK(recordBodies,"both numbered secret bodies ship as complete readable wrecks");
    EnemyWorld w;w.bounds={{-40,0,-38},{40,22,38}};w.playerFeet={0,0,16};w.playerEye={0,1.7f,16};w.playerVel={8,0,0};
    {
        EnemyWorld arena;arena.bounds={{-40,0,-38},{40,22,38}};arena.playerFeet={0,0,9};arena.playerEye={0,1.7f,9};
        auto patient=ready(EnemyType::PATIENT);patient.attackTimer=100;patient.update(DT,arena);
        bool patientTell=patient.attack==AttackKind::LAB_LUNGE && patient.telegraphTimer>.5f;
        for(int f=0;f<90 && patient.dashTimer<=0;++f)patient.update(DT,arena);
        auto committed=patient.labDir;arena.playerFeet={15,0,9};arena.playerEye={15,1.7f,9};bool dodged=true;
        for(int f=0;f<30;++f){patient.update(DT,arena);dodged&=!patient.ev.meleeHit;}
        CHECK(patientTell && committed.z>.9f && dodged && patient.labOpen>0,"Patient announces its lunge, commits its direction and exposes itself after a dodge");
        auto rushing=ready(EnemyType::PATIENT);rushing.dashTimer=.3f;rushing.labDir={0,0,1};
        EnemyWorld blocked=arena;std::vector<Wall> barrier={{{{-3,0,1},{3,5,1.5f}},{.3f,.3f,.3f}}};
        blocked.walls=barrier.data();blocked.wallCount=(int)barrier.size();blocked.playerFeet={0,0,2};blocked.playerEye={0,1.7f,2};
        bool throughWall=false;for(int f=0;f<20;++f){rushing.update(DT,blocked);throughWall|=rushing.ev.meleeHit;}
        CHECK(!throughWall && rushing.position.z<1 && rushing.labOpen>0 && rushing.armorMult()>1,
            "a Patient cannot lunge through cover and a stopped rush leaves a damage opening");
        auto parry=ready(EnemyType::PATIENT);parry.attack=AttackKind::LAB_LUNGE;parry.telegraphTimer=.2f;
        CHECK(parry.parryWindow(),"the final white brace gives Patient and Phase lunges a real parry window");

        auto grafted=ready(EnemyType::GRAFTED);grafted.yaw=0;
        bool directional=grafted.blocks({0,0,-1}) && !grafted.blocks({0,0,1});
        arena.playerFeet={0,0,5};arena.playerEye={0,1.7f,5};grafted.attackTimer=100;grafted.update(DT,arena);
        bool press=grafted.attack==AttackKind::LAB_PRESS && !grafted.blocks({0,0,-1});bool impact=false;
        for(int f=0;f<90;++f){grafted.update(DT,arena);impact|=grafted.ev.slam&&grafted.ev.slamDamage==24;}
        CHECK(directional && press && impact && grafted.labOpen>0,"Grafted blocks frontal body fire, opens during its marked press and leaves a recovery opening");
        auto gunner=ready(EnemyType::GRAFTED);arena.playerFeet={0,0,18};arena.playerEye={0,1.7f,18};gunner.attackTimer=100;
        bool returnable=false;for(int f=0;f<90;++f){gunner.update(DT,arena);if(gunner.ev.shots)
            returnable=gunner.ev.shots==1 && gunner.ev.shotHeavy && gunner.ev.shotParry>gunner.maxHealth*.6f;}
        CHECK(returnable,"Grafted's slow heavy orb gives a return-shot counter strong enough to create a finisher opportunity");

        arena.playerFeet={0,0,16};arena.playerEye={0,1.7f,16};auto suspended=ready(EnemyType::SUSPENDED,{0,6,0});suspended.attackTimer=100;suspended.update(DT,arena);
        bool beamTell=suspended.attack==AttackKind::LAB_BEAM && suspended.telegraphTimer>.8f;
        for(int f=0;f<90 && !suspended.labLocked;++f)suspended.update(DT,arena);
        auto beamAim=suspended.labAim;arena.playerFeet={15,0,16};arena.playerEye={15,1.7f,16};
        bool beam=false,locked=true,evaded=true;glm::vec3 firstRay{0},lastRay{0};
        AABB moved{{14.6f,0,15.6f},{15.4f,1.8f,16.4f}};
        for(int f=0;f<100;++f){suspended.update(DT,arena);if(suspended.ev.beamOn){
            if(!beam)firstRay=suspended.ev.beamTo;lastRay=suspended.ev.beamTo;beam=true;
            locked&=glm::length(suspended.labAim-beamAim)<.01f;
            evaded&=!segmentHitsBox(suspended.ev.beamFrom,suspended.ev.beamTo,moved);
        }}
        CHECK(beamTell && beam && locked && evaded && glm::length(lastRay-firstRay)>4,
            "Suspended locks before firing, sweeps a committed arc and lets a late dodge escape");
        std::vector<Wall> cover={{{{-8,0,5},{8,20,6}},{.3f,.3f,.3f}}};
        EnemyWorld shield=arena;shield.walls=cover.data();shield.wallCount=(int)cover.size();
        auto covered=ready(EnemyType::SUSPENDED,{0,6,0});covered.labBeam=.4f;covered.labDir={0,0,1};covered.update(DT,shield);
        CHECK(covered.ev.beamOn && covered.ev.beamTo.z<=5.01f,"solid cover clips the damaging Suspended ray before it reaches the player");
        CHECK(covered.onBeamHit(42,false) && covered.stunned() && !covered.attackBusy() && covered.armorMult()>1,
            "a magnum shot breaks the active beam and exposes the subject for a follow-up");

        arena.playerFeet={0,0,18};arena.playerEye={0,1.7f,18};auto phased=ready(EnemyType::PHASED);phased.attackTimer=100;phased.update(DT,arena);
        auto exit=phased.labExit;bool crossTell=phased.attack==AttackKind::LAB_CROSS && phased.telegraphTimer>.6f;
        arena.playerFeet={20,0,18};arena.playerEye={20,1.7f,18};bool arrived=false,safeArrival=false;
        for(int f=0;f<120&&!arrived;++f){phased.update(DT,arena);if(phased.ev.blinked){arrived=true;safeArrival=glm::length(phased.position-exit)<.05f && !phased.ev.meleeHit && phased.attack==AttackKind::LAB_LUNGE && phased.telegraphTimer>.5f;}}
        CHECK(crossTell && arrived && safeArrival,"Phase Subject commits its announced exit and gives a separate lunge warning after arriving");
        auto cancelledExit=ready(EnemyType::PHASED);cancelledExit.labCross=.01f;cancelledExit.labExit={0,0,18};
        std::vector<Wall> closed={{{{-2,0,16},{2,5,20}},{.3f,.3f,.3f}}};
        EnemyWorld shut=arena;shut.walls=closed.data();shut.wallCount=(int)closed.size();cancelledExit.update(DT,shut);
        CHECK(!cancelledExit.ev.blinked && cancelledExit.position.z==0 && cancelledExit.recoverTimer>0,
            "a Phase crossing cancels if its marked exit becomes blocked before arrival");
        std::vector<Wall> island={{{{-3,-1,-3},{3,0,3}},{.3f,.3f,.3f}}};
        EnemyWorld gap=arena;gap.walls=island.data();gap.wallCount=(int)island.size();gap.voidUnder=true;
        auto stranded=ready(EnemyType::PHASED);stranded.attackTimer=100;stranded.update(DT,gap);
        CHECK(stranded.attack!=AttackKind::LAB_CROSS && stranded.labCross==0,"Phase subjects reject unsupported exits over a void instead of teleporting to their death");

        bool cancelled=true;for(auto type:{EnemyType::PATIENT,EnemyType::GRAFTED,EnemyType::SUSPENDED,EnemyType::PHASED}){
            auto enemy=ready(type);enemy.labBeam=enemy.labCross=enemy.dashTimer=.4f;
            bool stun=enemy.takeHit({0,0,1},0,200,1,false,0);
            cancelled&=stun && enemy.labBeam==0 && enemy.labCross==0 && enemy.dashTimer==0 && enemy.labOpen>0;
        }
        CHECK(cancelled,"gun poise breaks cancel every lab enemy's pending beam, crossing and lunge instead of resuming them after stun");
    }
    auto boss=ready(EnemyType::SUCCESSOR);boss.suPhase=1;boss.attackTimer=100;boss.suRotation=1;boss.update(DT,w);
    CHECK(boss.attack==AttackKind::SU_VECTOR && boss.telegraphTimer>.45f,"Vector Run begins with a long readable lane tell");
    while(boss.telegraphTimer>SuccessorState::TRACK_LOCK)boss.update(DT,w);
    auto locked=boss.suAim;w.playerFeet={20,0,10};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};
    while(boss.telegraphTimer>0)boss.update(DT,w);
    CHECK(glm::length(locked-boss.suAim)<.01f&&boss.suBurst>0,"a late direction reversal cannot rotate the committed Vector lane");
    {
        auto elevated=ready(EnemyType::SUCCESSOR);elevated.suPhase=2;elevated.health=elevated.maxHealth*.6f;elevated.attackTimer=100;
        EnemyWorld high=w;high.playerFeet={20,6,12};high.playerEye=high.playerFeet+glm::vec3{0,1.7f,0};high.playerVel={0,0,0};elevated.update(DT,high);
        bool announced=elevated.ev.suMark && elevated.suCross>0 && elevated.suExit.y==6;
        bool noContact=true;for(int f=0;f<20&&elevated.suCross>0;++f){elevated.update(DT,high);noContact&=!elevated.ev.meleeHit;}
        CHECK(announced && noContact && elevated.attack==AttackKind::SU_TRANSPOSE && elevated.telegraphTimer>.3f,"a stationary gallery player draws an announced crossing and a separate white arrival tell, with no transit hit");
        auto ranged=ready(EnemyType::SUCCESSOR);ranged.suPhase=1;ranged.attackTimer=100;high.bounds={{-70,0,-70},{70,30,70}};high.playerFeet={0,0,50};high.playerEye={0,1.7f,50};ranged.update(DT,high);
        CHECK(ranged.attack==AttackKind::SU_VECTOR && ranged.telegraphTimer>.45f,"phase one closes long gaps with a marked rush instead of camping with slow projectiles");
    }
    {
        EnemyWorld arena=w;arena.playerFeet={0,0,16};arena.playerEye={0,1.7f,16};arena.playerVel={0,0,0};
        auto chain=ready(EnemyType::SUCCESSOR);chain.suPhase=2;chain.health=chain.maxHealth*.6f;chain.attackTimer=100;chain.suRotation=1;
        bool repeat=false;int bursts=0;bool running=false;
        for(int f=0;f<200;++f){chain.update(DT,arena);if(chain.suBurst>0&&!running)++bursts;running=chain.suBurst>0;
            if(bursts==1 && chain.attack==AttackKind::SU_VECTOR && chain.telegraphTimer>.3f)repeat=true;}
        CHECK(repeat && bursts>=2,"later Vector chains acquire a new lane with a separate warning before their second dash");
        EnemyWorld distant=arena;distant.bounds={{-70,0,-70},{70,30,70}};distant.playerFeet={0,0,50};distant.playerEye={0,1.7f,50};
        auto engage=ready(EnemyType::SUCCESSOR);float contactTime=9;
        for(int f=0;f<240;++f){engage.update(DT,distant);if(engage.ev.meleeHit){contactTime=f*DT;break;}}
        CHECK(contactTime<3.8f,"the boss reaches a player fifty metres away within four seconds of entering combat");
        auto stride=ready(EnemyType::SUCCESSOR);stride.suPhase=1;stride.suGait=22;
        float low=100,high=-100;
        for(int f=0;f<240;++f){stride.animPhase=f*.12f;auto gait=rig::successorGuard(stride,0);
            auto leg=[](float h,float k){return .51f*std::cos(h)+.46f*std::cos(h+k)+.13f;};
            float planted=std::max(leg(gait.hipR,gait.kneeR),leg(gait.hipL,gait.kneeL))*stride.height()/2.65f;
            low=std::min(low,planted);high=std::max(high,planted);}
        CHECK(high-low<.22f,"the running gait keeps the supporting leg long without bouncing the body between two bent knees");
        auto drive=ready(EnemyType::SUCCESSOR);drive.suPhase=1;drive.attackTimer=100;
        EnemyWorld close=arena;close.playerFeet={0,0,8};close.playerEye={0,1.7f,8};float longest=0;bool contact=false;
        for(int f=0;f<100&&!contact;++f){auto before=drive.position;drive.update(DT,close);longest=std::max(longest,glm::length(drive.position-before));contact=drive.ev.meleeHit;}
        CHECK(contact && longest<.5f,"melee drives forward over animation frames without an instant root teleport at impact");
        auto breaker=ready(EnemyType::SUCCESSOR);breaker.suPhase=2;breaker.health=breaker.maxHealth*.6f;breaker.suRotation=2;breaker.attackTimer=100;
        breaker.update(DT,arena);bool cast=breaker.attack==AttackKind::SU_BREAKER;
        while(breaker.telegraphTimer>SuccessorState::TRACK_LOCK)breaker.update(DT,arena);
        auto landing=breaker.suAim;arena.playerFeet={20,0,16};arena.playerEye={20,1.7f,16};
        bool flightSafe=true,landed=false;float peak=0;
        for(int f=0;f<100&&!landed;++f){breaker.update(DT,arena);peak=std::max(peak,breaker.position.y);flightSafe&=!breaker.ev.meleeHit;
            landed=breaker.ev.suBreaker;}
        CHECK(cast && flightSafe && landed && peak>4 && glm::length(breaker.position-landing)<.01f && breaker.suOpen>0,
            "Breaker commits its marked landing, arcs overhead without transit damage and exposes the core on landing");
        arena.playerFeet={0,0,6};arena.playerEye={0,1.7f,6};
        auto reap=ready(EnemyType::SUCCESSOR);reap.suPhase=1;reap.suRotation=3;reap.attackTimer=100;reap.update(DT,arena);
        bool swept=false;for(int f=0;f<90&&!swept;++f){reap.update(DT,arena);swept=reap.ev.suReap&&reap.ev.slam&&reap.ev.slamRadius==9;}
        CHECK(swept,"the gold low sweep resolves through the jumpable ground-slam path");
        auto stranded=ready(EnemyType::SUCCESSOR,{0,8,0});stranded.suPhase=1;stranded.attackTimer=100;stranded.update(DT,arena);
        CHECK(stranded.suCross>0 && stranded.suExit.y==0,"the boss can leave a gallery to pursue a player below rather than remaining stranded");
        std::vector<Wall> floor={{{{-4,-1,-4},{4,0,4}},{.3f,.3f,.3f}}};
        arena.walls=floor.data();arena.wallCount=(int)floor.size();arena.voidUnder=true;
        auto edge=ready(EnemyType::SUCCESSOR);edge.suPhase=1;edge.suBurst=1;edge.suDir={1,0,0};edge.suAim={30,0,0};edge.suFrom={0,0,0};
        bool supported=true;for(int f=0;f<60;++f){edge.update(DT,arena);supported&=edge.position.x<=4 && edge.position.y>=0;}
        CHECK(supported,"committed boss dashes stop at physical floor edges even inside wider arena bounds");
        auto pose=ready(EnemyType::SUCCESSOR);pose.suPhase=1;rig::tickSuccessorBody(pose,DT);float guard=pose.suBody[5];
        pose.attack=AttackKind::SU_THRUST;pose.telegraphDuration=.8f;pose.telegraphTimer=.02f;rig::tickSuccessorBody(pose,DT);
        float first=pose.suBody[5];bool smooth=pose.suBodyPrev[5]==guard && std::fabs(first-guard)<.5f;
        for(int f=0;f<30;++f)rig::tickSuccessorBody(pose,DT);
        bool finite=true;for(float channel:pose.suBody)finite&=std::isfinite(channel);
        CHECK(smooth && finite && std::fabs(pose.suBody[5]-guard)>.6f,"the dedicated Successor spring rig blends into its thrust and preserves render interpolation history");
        SuccessorHazards impact;impact.circle({0,0,0},.01f,32,3,4.8f,6.f);float damage=0;impact.update(.02f,{0,2,0},damage);
        CHECK(damage==32,"the red aerial breaker reaches above an ordinary jump, requiring a lateral dodge");
    }
    boss.suNodes=2;boss.suCross=.2f;boss.suAir=.4f;boss.suDashes=boss.suVolley=2;boss.suStep=2;int epoch=boss.suEpoch;boss.stagger(1.35f);
    CHECK(boss.suBurst==0&&boss.suNodes==0&&boss.suCross==0&&boss.suAir==0&&boss.suDashes==0&&boss.suVolley==0&&boss.suStep==0&&boss.suEpoch>epoch,"a parry cancels every pending movement, node and combo continuation");
    auto white=ready(EnemyType::SUCCESSOR);white.attack=AttackKind::SU_THRUST;white.telegraphTimer=.2f;CHECK(white.parryWindow(),"the third Incision strike has a real white parry window");white.attack=AttackKind::SU_INCISION;CHECK(!white.parryWindow(),"gold Incision strikes cannot be parried");
    auto phase=ready(EnemyType::SUCCESSOR);phase.update(DT,w);phase.health=phase.maxHealth*.64f;phase.update(DT,w);bool two=phase.suPhase==2;phase.health=phase.maxHealth*.29f;phase.update(DT,w);CHECK(two&&phase.suPhase==3&&phase.alive,"one health bar makes the two explicit phase transitions without refills");
    SuccessorHazards hazard;hazard.circle({0,0,0},1,24);float damage=0;hazard.update(.9f,{0,0,0},damage);bool safe=damage==0;hazard.update(.11f,{0,0,0},damage);bool hits=damage==24;hazard.update(.1f,{0,0,0},damage);CHECK(safe&&hits&&damage==0,"Recall gives a full warning, hits once and never ticks repeatedly");
    CHECK(musicTrack(13).bpm==176 && std::string(musicTrack(13).name)=="RELEASE DENIED" && musicTrack(13).techno==0,"Successor routes to its original fast choir/organ arrangement");
    auto reactive=labBossRun(buildLab(),true,WeaponId::REVOLVER),passive=labBossRun(buildLab(),false,WeaponId::REVOLVER);
    std::printf("  Successor real revolver rehearsal: %.1fs / %d hits / %d parries / damage %.0f, passive %.0f / %zu moves\n",reactive.seconds,reactive.hits,reactive.parries,reactive.damage,passive.damage,reactive.moves.size());
    CHECK(reactive.won && reactive.lowest>0 && reactive.seconds>60 && reactive.seconds<250,"the real boss can be defeated with an unupgraded gun, normal movement and bounded parries");
    CHECK(reactive.moves.size()>=9 && reactive.damage<passive.damage,"the expanded attack set occurs in a full fight and reading the tells reduces incoming damage");
    for(float opening:{0.f,.6f}){
        auto run=labBossRun(buildLab(),true,WeaponId::REVOLVER,opening);
        std::printf("  opening %.1f: %.1fs / health low %.1f / damage %.0f\n",opening,run.seconds,run.lowest,run.damage);
        CHECK(run.won && run.lowest>0,"reactive gun play survives both ends of the live randomized opening clock");
    }
    return failures?1:0;
}
