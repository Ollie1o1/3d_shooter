// Chapter 1: real Player collision, encounter progression and the Sorter's real AI.
#include "player/Player.h"
#include "core/Balance.h"
#include "world/LevelStory.h"
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
        p.floorY=l.baseFloor(p.position.x,p.position.z);
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
    balanceInit();auto L=buildCradle();const float A=-16.2f,V=A-4,T=V+39;
    CHECK(L.arenas.size()==9,"all nine Chapter 1 rooms are present");
    CHECK(L.arenas[4].waves.empty(),"Archive is quiet");
    CHECK(L.arenas[8].waves[0][0].type==EnemyType::SORTER,"chapter ends in the new Sorter encounter");
    // Open doors as they would be after their encounters, without changing walls or player movement.
    for(int i=0;i<(int)L.doors.size();++i)L.setDoorInstant(i,true);
    bool checkpoints=true;
    for(int i=0;i<9;++i){Walk w(L,L.arenas[i].playerStart);for(int k=0;k<120;++k)w.step(w.p.position);
        bool ok=L.arenaAt(w.p.position)==i&&glm::length(w.p.position-L.arenas[i].playerStart)<.3f;
        std::printf("  checkpoint %d: %.1f %.1f %.1f section %d\n",i,w.p.position.x,w.p.position.y,w.p.position.z,L.arenaAt(w.p.position));checkpoints&=ok;}
    CHECK(checkpoints,"every checkpoint stands on real ground in its own room");
    CHECK(L.arenaAt({0,0,8})==0 && L.arenaAt({0,T,8})==7,"stacked furnace and inspection line select the correct floor");
    {
        // Original rooms are part of the chapter's regression coverage too.
        Walk w(L,{47,-40,-22});
        bool climb=w.to({50,-38.6f,-21.5f},true);
        const glm::vec3 launch[]={{51.4f,-38.6f,-19.6f},{54,-37.2f,-14.6f},{52.6f,-35.8f,-9.4f},{46.6f,-34.4f,-7},{42,-33,-9.4f},{40.6f,-31.6f,-15},{36,-30.2f,-13.6f},{36,-28.8f,-8.6f}};
        const glm::vec3 land[]={{54,-37.2f,-16},{54,-35.8f,-10},{48,-34.4f,-7.5f},{42,-33,-7},{42,-31.6f,-15},{36,-30.2f,-15},{36,-28.8f,-9.5f},{36,-27.4f,-6}};
        for(int i=0;i<8 && climb;++i)climb=w.to(launch[i])&&w.to(land[i],true);
        CHECK(climb,"Scrap Pit's eight jumps climb from the floor to the duct exit");
        Walk d(L,L.arenas[2].playerStart);
        CHECK(d.to({32,-27.4f,16})&&d.to({36,-27.4f,17})&&d.to({37.5f,-27.4f,26})&&d.to({38,-27.4f,35}),"duct junction reaches its pipe corridor");
        CHECK(d.to({38,-27.4f,44},false,true),"the actual slide fits under the low pipe run");
        CHECK(d.to({44,-27.4f,53})&&d.to({48.5f,-27.4f,53}),"Lancer room reaches the climb shaft");
    }
    {
        WaveDirector d;d.level=&L;d.fast=true;d.approach(2);
        Walk w(L,{32,-27.4f,-6});w.boundaries=&d;
        CHECK(w.to({32,-27.4f,2}) && w.to({32,-27.4f,-6}),"pit-to-duct threshold has continuous floor and room bounds in both directions");
    }
    {
        Walk secret(L,{26.5f,-27.4f,26.5f});
        bool vent=secret.to({24.8f,-26.2f,26.5f},true)&&secret.to({24.8f,-24.9f,24.1f},true)&&secret.to({26.2f,-23.8f,21.5f},true)&&secret.to({21,-23.8f,21.5f},false,true);
        CHECK(vent,"UNIT 03's vent has a standing ledge and a real slide into the secret room");
        Walk vaultSecret(L,{-31.5f,V,66});
        bool cell=vaultSecret.to({-34,V+1.2f,66},true)&&vaultSecret.to({-36.5f,V+2.5f,66},true)&&vaultSecret.to({-36.5f,V+3.6f,69.5f},true)&&vaultSecret.to({-42,V+3.6f,69.5f},false,true);
        CHECK(cell,"UNIT 07's observation cell is reachable through its vent with real movement");
    }
    {
        auto& lift=L.movers[L.story.serviceLift];
        WaveDirector d;d.level=&L;d.fast=true;d.approach(2);
        Walk w(L,{44,-27.4f,53});
        CHECK(w.to({48.5f,-27.4f,53}),"walk straight from Lancer room onto service deck without jumping");
        // Jumping just after departure must not call the deck back down.
        lift.drive=.03f;L.updateMovers(0);
        Player jumping({48.5f,lift.base.max.y+lift.b.y*lift.drive+.5f,53});
        jumping.onGround=false;jumping.groundWall=-1;
        storylift::update(L,d,jumping,DT);
        CHECK(lift.drive>.03f,"jumping during the first metre of the service ascent keeps it rising");
        lift.drive=0;L.updateMovers(0);
        int air=0;
        for(int i=0;i<60*6;++i) {
            int ride=L.moverOfWall(w.p.groundWall);L.updateMovers(i*DT);
            if(ride>=0)w.p.position+=L.movers[ride].delta;
            w.step(w.p.position);storylift::update(L,d,w.p,DT);air+=!w.p.onGround;
        }
        CHECK(lift.drive==1.f && std::fabs(w.p.position.y-A)<.05f && air<5,"automatic service lift carries Player the full ascent in under six seconds");
        CHECK(w.to({59,A,53}),"service lift exits through the upper hatch into Assembly without jumping");
        Walk below(L,{44,-27.4f,53});
        for(int i=0;i<60*6;++i){storylift::update(L,d,below.p,DT);L.updateMovers(i*DT);}
        CHECK(lift.drive==0 && below.to({48.5f,-27.4f,53}),"service deck returns for a player waiting at the lower landing");
    }
    {
        // Run the real lift controller during the approach, not just after
        // placing the player at the centre of the deck.
        for(int id:{L.story.serviceLift,L.story.elevator}) {
            auto& lift=L.movers[id];lift.drive=0;L.updateMovers(0);
            const bool service=id==L.story.serviceLift;
            WaveDirector d;d.level=&L;d.fast=true;d.approach(service?2:6);
            Walk w(L,service?glm::vec3{44,-27.4f,53}:glm::vec3{-25,V,21});
            w.boundaries=&d;
            glm::vec3 goal=service?glm::vec3{48.8f,-27.4f,53}:glm::vec3{-25,V,6};
            for(int i=0;i<60*8;++i) {
                int ride=L.moverOfWall(w.p.groundWall);L.updateMovers(i*DT);
                if(ride>=0)w.p.position+=L.movers[ride].delta;
                w.step(goal);storylift::update(L,d,w.p,DT);
            }
            float expected=service?A:V+13;
            std::printf("  continuous boarding %s: %.2f %.2f %.2f drive %.3f\n",service?"service":"freight",w.p.position.x,w.p.position.y,w.p.position.z,lift.drive);
            CHECK(std::fabs(w.p.position.y-expected)<.1f,"walking onto an active lift boards and rises without a jump or dash");
            lift.drive=0;L.updateMovers(0);
        }
    }
    {
        Walk w(L,{102,A,72});
        CHECK(w.to({102,A-7,44},true),"assembly staircase descends to the production floor");
        CHECK(w.to({102,A,73},true),"assembly staircase can be climbed back to the archive exit");
        CHECK(w.to({60,A,74})&&w.to({60,A,69})&&w.to({51,A,69}),"assembly catwalk and exit reach the Archive");
        CHECK(w.to({20,A,75})&&w.to({12,A,75}),"Archive's west doorway reaches the pod-vault balcony");
        CHECK(w.to({12,V,58},true),"pod balcony staircase reaches the vault floor");
        CHECK(w.to({-10,V,57})&&w.to({-24,V,30})&&w.to({-25,V,21})&&w.to({-25,V,6}),"vault exit and loading bay reach the freight platform");
    }
    {
        auto& lift=L.movers[L.story.elevator];lift.drive=0;L.updateMovers(0);
        WaveDirector d;d.level=&L;d.fast=true;d.approach(6);
        Walk w(L,{-25,V,21});
        CHECK(w.to({-25,V,6}),"walk from loading bay onto the actual freight deck");
        int air=0;bool held=true;int stop=0,wait=0,spawns=0;
        for(int i=0;i<60*24 && d.arena==6;++i) {
            int ride=L.moverOfWall(w.p.groundWall);L.updateMovers(i*DT);
            if(ride>=0)w.p.position+=L.movers[ride].delta;
            w.step(w.p.position);storylift::update(L,d,w.p,DT);air+=!w.p.onGround;
            float target=(stop+1)/3.f;
            if(lift.drive>=target-.0001f)++wait;
            if(wait>0 && wait<60)held &= std::fabs(lift.drive-target)<.0002f;
            int alive=wait>60?0:1;
            int prev=d.wave;std::vector<SpawnRequest> out;d.update(DT,alive,w.p.position,out);spawns+=(int)out.size();
            if(d.wave>prev){++stop;wait=0;}
        }
        CHECK(d.arena==7 && spawns==9 && held,"boarding starts freight encounter; all three combat stops hold until defenders clear");
        CHECK(std::fabs(w.p.position.y-T)<.05f&&air<5,"freight lift carries the actual Player up all 39 metres through its gameplay controller");
        CHECK(w.to({-16,T,8})&&w.to({-10,T,8}),"lift's top landing reaches inspection");
        CHECK(w.to({50,T,8})&&w.to({59,T,8}),"scanner corridor reaches the Sorting Floor");
        CHECK(w.to({60,T,-46})&&w.to({82,T,-46})&&w.to({82,T,-77}),"sorting side path and opened gate reach the ash wasteland");
    }
    {
        std::vector<Interactable> items(2);int reads=0;
        items[0].type=InteractableType::READABLE;
        items[0].box={{-.6f,1.2f,-3},{.6f,2.4f,-2}};
        items[0].onInteract=[&](){++reads;};
        items[1].active=false;
        std::vector<Wall> walls;
        int target=interactTarget(items,{0,1.7f,0},{0,0,-1},walls);
        CHECK(target==0 && reads==0,"approaching and aiming at a readable selects a prompt without opening its log");
        if(target>=0)items[target].onInteract();
        CHECK(reads==1,"explicit interaction opens the selected readable");
        CHECK(interactTarget(items,{0,1.7f,3},{0,0,-1},walls)<0,"readable is unavailable outside interaction range");
        walls.push_back({{{-2,0,-1.5f},{2,3,-1.4f}}});
        CHECK(interactTarget(items,{0,1.7f,0},{0,0,-1},walls)<0,"a wall blocks reading and its prompt");
    }
    // Every hand-placed spawn has clearance (doors open, same capsule as Enemy uses).
    bool clear=true;
    for(auto& a:L.arenas)for(auto& wave:a.waves)for(auto& e:wave)for(auto p:e.at){float r=statsOf(e.type).radius,h=statsOf(e.type).height;
        AABB b{p+glm::vec3{-r,.08f,-r},p+glm::vec3{r,h,r}};
        for(const auto& w:L.walls)if(b.min.x<w.box.max.x && b.max.x>w.box.min.x && b.min.y<w.box.max.y && b.max.y>w.box.min.y && b.min.z<w.box.max.z && b.max.z>w.box.min.z){clear=false;std::printf("  blocked %s at %.1f %.1f %.1f\n",statsOf(e.type).name,p.x,p.y,p.z);break;}}
    CHECK(clear,"every scripted enemy spawns clear of the room geometry");
    // Run every combat room through the real director, with quiet rooms explicitly approached.
    bool flow=true;int bosses=0;
    for(int room:{1,2,3,5,6,7,8}){WaveDirector d;d.level=&L;d.fast=true;d.startArena(room);int spawns=0;
        for(int f=0;f<600&&d.arena==room&&d.phase!=WaveDirector::Phase::VICTORY;++f){std::vector<SpawnRequest> out;d.update(DT,0,L.arenas[room].playerStart,out);for(auto s:out){++spawns;bosses+=s.type==EnemyType::SORTER;}}
        flow&=spawns>0&&(d.arena==room+1||d.phase==WaveDirector::Phase::VICTORY);}
    CHECK(flow&&bosses==1,"every fight advances and only the Sorting Floor spawns the Sorter");
    {
        SpatialGrid g;g.build(L.walls);BossSim sim(L,g,L.arenas[8],EnemyType::SORTER,BossSim::SOLID);
        for(int f=0;f<60*240&&sim.bossAlive();++f)sim.step(DT);
        std::printf("  Sorter kill %.1fs, damage %.0f, parries %d\n",sim.t,sim.damageTaken,sim.parries);
        CHECK(!sim.bossAlive()&&sim.t>60&&sim.t<200,"BossSim: a skilled gun run can beat all three Sorter phases in 60-200 seconds");
        bool cheese=true;
        for(auto policy:{BossSim::STILL,BossSim::CORNER,BossSim::PERCH,BossSim::RANGED})for(float hp:{1.f,.5f,.2f}){
            BossSim s(L,g,L.arenas[8],EnemyType::SORTER,policy);s.boss.health=s.boss.maxHealth*hp;
            for(int f=0;f<60*14&&s.damageTaken<1;++f)s.step(DT);
            if(s.damageTaken<1){cheese=false;std::printf("  unpunished policy %d hp %.1f\n",policy,hp);}}
        CHECK(cheese,"BossSim: idle, corner, high-perch and ranged camping are punished in every phase");
    }
    // A targeted press commits before its visible downstroke; moving after the lock
    // must evade it, while standing on the committed point is hit at full extension.
    {
        Enemy e(EnemyType::SORTER,{82,T+2,-20});e.state=EnemyState::ACTIVE;e.spawnTimer=0;
        e.sortPhase=1;e.sortHome=e.position;e.sortHomeSet=true;e.sortStep=1;e.sortNext=AttackKind::SORT_STAMP;e.attackTimer=100;
        EnemyWorld w;w.bounds=L.arenas[8].bounds;w.playerFeet={80,T,-2};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};
        bool locked=false,dropped=false,hit=false,stable=true,matched=false;glm::vec3 committed{0};
        for(int i=0;i<180&&!hit;++i) {
            e.update(DT,w);
            if(e.ev.sortLocked){locked=true;committed=e.sortAim;}
            if(locked)stable&=glm::length(e.sortAim-committed)<.001f;
            dropped|=e.ev.sortDrop;
            if(e.ev.sortStamp){hit=true;matched=e.ev.meleeHit&&e.sortImpactT==0&&
                std::fabs(Enemy::sortPressHeight(0,e.sortImpactT)-Enemy::SORT_PRESS_DOWN)<.001f;}
        }
        CHECK(locked&&dropped&&stable&&hit&&matched,"press locks, cues release and reaches the floor on the damage tick");
        e.stagger(.3f);for(int i=0;i<12;++i)e.update(DT,w);
        CHECK(e.sortImpactT>0&&Enemy::sortPressHeight(0,e.sortImpactT)>Enemy::SORT_PRESS_DOWN,
              "the physical press retracts after impact even if the boss is staggered");
        e.staggerTimer=0;e.recoverTimer=0;e.attackTimer=100;e.sortStep=1;e.sortNext=AttackKind::SORT_STAMP;e.sortLocked=e.sortDropCued=false;
        bool dodged=false;locked=false;
        for(int i=0;i<180&&!dodged;++i) {
            e.update(DT,w);
            if(e.ev.sortLocked){locked=true;committed=e.sortAim;w.playerFeet.x+=10;w.playerEye.x+=10;}
            if(e.ev.sortStamp)dodged=locked&&!e.ev.meleeHit&&glm::length(e.sortAim-committed)<.001f;
        }
        CHECK(dodged,"leaving the locked press target avoids its hit without the target following");
        Enemy combo(EnemyType::SORTER,{82,T+2,-20});combo.state=EnemyState::ACTIVE;combo.spawnTimer=0;
        combo.health=combo.maxHealth*.2f;combo.sortPhase=3;combo.sortStep=1;combo.sortNext=AttackKind::SORT_ROW;combo.attackTimer=100;
        int presses=0;bool readable=false;
        for(int i=0;i<60*8&&presses<3;++i){combo.update(DT,w);if(combo.ev.telegraphStarted&&combo.attack==AttackKind::SORT_ROW){readable=combo.sortPresses.size()==3&&combo.sortPresses.front().remaining>=.95f;}presses+=combo.ev.sortStamp;}
        CHECK(presses==3&&readable,"press sequence warns all three physical heads before their staggered impacts");
        combo.sortFollowups=2;combo.stagger(1);
        CHECK(combo.sortFollowups==0,"stagger cancels queued Sorter follow-ups");
    }
    // White claw parries cancel their hit, clear marking and open double damage.
    {
        Enemy e(EnemyType::SORTER,{82,T+2,-20});e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.sortPhase=1;e.sortHome=e.position;e.sortHomeSet=true;
        e.attack=AttackKind::SORT_GRAB;e.telegraphTimer=.2f;e.sortMark=4;
        CHECK(e.parryWindow(),"claw's final quarter-second is parryable");
        e.stagger(e.staggerTime());EnemyWorld w;w.playerFeet={82,T,-20};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};e.update(DT,w);
        CHECK(!e.ev.meleeHit&&e.sortMark==0&&e.armorMult()==2,"a parried claw stops, clears the scanner mark and opens the core");
    }
    lightbake::Bake bake;std::string err;CHECK(!(bake=lightbake::load("story1",buildCradle(),err)).empty(),"Chapter 1's baked lighting matches the final geometry");
    std::printf("%s (%d failures)\n",failures?"FAILED":"ALL PASSED",failures);return failures?1:0;
}
