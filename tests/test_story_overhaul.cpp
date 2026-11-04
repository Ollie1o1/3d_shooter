// Deterministic gameplay contracts for the Story pacing, weather and new sibling.
#include "core/Balance.h"
#include "core/Settings.h"
#include "ui/TextWrap.h"
#include "game/WaveDirector.h"
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "render/Effects.h"
#include "bosses/sovereign/SovereignHazards.h"
#include "bosses/warden/WardenHazards.h"
#include "bosses/leviathan/LeviathanHazards.h"
#include "world/ArenaShifts.h"
#include "player/Projectile.h"
#include "BossSim.h"
#include <set>
#include <cstdio>
static int failures=0;
#define CHECK(c,m) do{if(!(c)){++failures;std::printf("FAIL: %s\n",m);}else std::printf("ok: %s\n",m);}while(0)
constexpr float DT=1.f/60.f;
int main() {
    balanceInit();auto cradle=buildCradle();auto road=buildWasteland();
    CHECK(cradle.arenas[1].waves.size()==1&&cradle.arenas[2].waves.size()==1&&cradle.arenas[3].waves.size()==2&&
          cradle.arenas[5].waves.size()==1&&cradle.arenas[6].waves.size()==3,"factory rhythm varies; only the moving lift retains three distinct combat stops");
    CHECK(road.arenas[1].waves.size()==1&&road.arenas[2].goals[0].kind==WaveGoal::HOLD&&
          road.arenas[4].goals[0].kind==WaveGoal::EXTERNAL&&road.arenas[5].waves.empty(),"road alternates ambush, repair, crossing, brake release, breather and boss");
    {
        WaveDirector d;d.level=&road;d.fast=true;d.startArena(2);
        auto pad=d.current().goals[0].pos;std::vector<SpawnRequest> out;
        d.update(DT,0,pad,out);d.update(DT,0,pad,out);
        int alive=(int)out.size(),total=alive;bool bounded=alive<=d.maxAlive();
        for(int i=0;i<180;++i) {
            out.clear();d.update(DT,alive,pad+glm::vec3{10,0,0},out);
            alive+=(int)out.size();total+=(int)out.size();bounded&=alive<=d.maxAlive();
        }
        CHECK(bounded&&total<=d.maxAlive(),"repair reinforcements stay bounded when the player is not killing");
        d.zoneContested=true;float before=d.goalTimer;out.clear();d.update(1.f,alive,pad,out);
        CHECK(d.goalTimer==before,"an enemy on the service pad pauses repair progress");
        d.zoneContested=false;int initial=total;
        for(int i=0;i<150;++i){out.clear();d.update(DT,0,pad,out);total+=(int)out.size();}
        CHECK(total-initial<=8,"killing guards does not produce a new security detail every frame");
        for(int i=0;i<700&&d.arena==2;++i){out.clear();d.update(DT,0,pad,out);}
        CHECK(d.arena==3,"finishing the repair opens the next traversal section");
    }
    {
        WaveDirector d;d.level=&road;d.fast=true;d.startArena(4);std::vector<SpawnRequest> out;
        for(int i=0;i<900;++i){out.clear();d.update(DT,0,road.arenas[4].playerStart,out);}
        CHECK(d.arena==4&&!d.goalDone,"killing turbine guards alone does not silently bypass the repair");
        bool realTargets=true;
        for(int id:road.story.turbineBrakes) {
            const auto& a=road.anchors[id];glm::vec3 eye=a.pos+glm::vec3{a.pos.x<0?4.f:-4.f,0,0};
            auto dir=glm::normalize(a.pos-eye);float t=rayBoxHit(eye,dir,road.walls[a.wall].box);
            realTargets&=t>0&&road.anchorAlong(eye,dir,t+.01f)==id;
            realTargets&=road.damageAnchor(id,100.f);
        }
        CHECK(realTargets&&road.anchorsAlive(LevelData::ChainAnchor::BRAKE)==0,"all three brakes are real raycast targets and accept weapon damage");
        d.completeGoal();out.clear();d.update(DT,0,road.arenas[4].playerStart,out);
        CHECK(d.arena==5,"brake release uses the director completion path");
    }
    {
        bool recordsFit=true;
        for(const auto* chapter:{&cradle,&road})for(const auto& terminal:chapter->story.terminals) {
            auto lines=ui_text::wrap(terminal.line,90);std::string joined;
            for(const auto& line:lines){recordsFit&=line.size()<=90;joined+=line;}
            auto noSpaces=[](std::string text){text.erase(std::remove(text.begin(),text.end(),' '),text.end());return text;};
            recordsFit&=lines.size()<=3&&noSpaces(joined)==noSpaces(terminal.line);
        }
        CHECK(recordsFit,"every chapter record fits its panel without losing dialogue or instructions");
    }
    {
        GameSettings s;s.enemyMarkers=1;GameSettings restored;restored.parse(s.text());
        CHECK(restored.enemyMarkers==1,"detailed enemy markers round-trip through saved settings");
        restored.parse("enemyMarkers -5\n");CHECK(restored.enemyMarkers==0,"invalid enemy marker detail clamps to Low");
        restored.parse("enemyMarkers 100\n");CHECK(restored.enemyMarkers==1,"enemy marker detail clamps to the available choices");
        s.ambientDetail=0;restored.parse(s.text());
        CHECK(restored.ambientDetail==0,"ambient detail round-trips through saved settings");
        restored.parse("ambientDetail 999\n");CHECK(restored.ambientDetail==3,"invalid detail values clamp safely");
        int counts[4]={};bool reserve=true;
        for(int detail=0;detail<4;++detail) {
            Effects fx;
            for(int i=0;i<600;++i){fx.updateParticles(DT);fx.spawnAmbientParticles(road,{0,0,30},false,DT,detail,i*DT);}
            counts[detail]=fx.ambientCount();reserve&=counts[detail]<=Effects::ambientBudget(detail);
            auto* combat=fx.freeParticle();combat->alive=true;combat->life=combat->maxLife=3;combat->color={1,0,0};
            fx.setAmbientDetail(0);
            reserve&=fx.ambientCount()==0&&combat->alive&&!combat->ambient;
        }
        CHECK(reserve&&counts[0]==0&&counts[1]>0&&counts[2]>counts[1]&&counts[3]>counts[2],"detail levels reduce actual particle work, preserve combat particles and clear immediately");
    }
    {
        BallastHazards h;std::vector<Wall> empty;
        h.add({0,0,0},{0,0,20});bool hit=false;
        for(int i=0;i<60;++i)hit|=!h.update(DT,{0,0,10},empty).empty();
        CHECK(hit,"an exposed player in the grit front is hit");
        Wall cover;cover.box={{-5,0,4},{5,3,5}};std::vector<Wall> walls{cover};h.clear();h.add({0,0,0},{0,0,20});hit=false;
        for(int i=0;i<60;++i)hit|=!h.update(DT,{0,0,10},walls).empty();
        CHECK(!hit,"a real windbreak protects the player from crosswind");
        h.clear();h.add({0,0,0},{0,0,20});
        for(int i=0;i<60;++i)hit|=!h.update(DT,{9,0,10},empty).empty();
        CHECK(!hit,"leaving the marked lane avoids crosswind");
    }
    {
        SpatialGrid g;g.build(road.walls);
        auto model=enemyfile::modelOf((int)EnemyType::BALLAST);
        CHECK(model&&!model->parts.empty()&&enemyfile::fileName(EnemyType::BALLAST)==std::string("ballast"),"Ballast loads its own articulated model");
        Enemy e(EnemyType::BALLAST,{28,0,-218});e.state=EnemyState::ACTIVE;e.spawnTimer=0;
        e.attack=AttackKind::BALL_ANCHOR;e.telegraphTimer=.2f;
        CHECK(e.parryWindow(),"the white cable weight has a real parry window");
        auto hull=e.getAABB();
        CHECK(hull.max.x>=e.position.x+3.5f&&hull.max.z>=e.position.z+4.1f,"the grader blade and outriggers accept shots beyond the navigation footprint");
        e.attack=AttackKind::BALL_GRADE;CHECK(!e.parryWindow(),"the grading blade cannot be parried");
        {
            Enemy fan(EnemyType::BALLAST,road.arenas[6].bossSpawn);fan.state=EnemyState::ACTIVE;fan.spawnTimer=0;fan.health=fan.maxHealth*.15f;
            EnemyWorld w;w.bounds=road.arenas[6].bounds;w.playerFeet={28,0,-190};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};
            w.walls=road.walls.data();w.wallCount=(int)road.walls.size();w.grid=&g;
            bool forward=false,reverse=false;
            for(int i=0;i<60*90;++i){fan.update(DT,w);if(fan.ev.ballGust){reverse|=fan.ballReverse;forward|=!fan.ballReverse;}}
            CHECK(forward&&reverse,"the damaged fan alternates marked blowing and reverse-draw lanes");
        }
        {
            Enemy combo(EnemyType::BALLAST,road.arenas[6].bossSpawn);combo.state=EnemyState::ACTIVE;combo.spawnTimer=0;
            combo.health=combo.maxHealth*.15f;combo.ballPhase=3;combo.ballStep=2;combo.ballFollow=AttackKind::BALL_GUST;combo.attackTimer=100;
            EnemyWorld w;w.bounds=road.arenas[6].bounds;w.playerFeet={28,0,-190};w.playerEye=w.playerFeet+glm::vec3{0,1.7f,0};
            w.walls=road.walls.data();w.wallCount=(int)road.walls.size();w.grid=&g;
            std::vector<AttackKind> tells;bool readable=true,charged=false;
            for(int i=0;i<60*10&&tells.size()<4;++i) {
                combo.update(DT,w);charged|=combo.ballDrive>0;
                if(combo.ev.telegraphStarted){tells.push_back(combo.attack);readable&=combo.telegraphDuration>=.8f;}
            }
            CHECK(tells==std::vector<AttackKind>({AttackKind::BALL_GUST,AttackKind::BALL_GRADE,AttackKind::BALL_GRADE,AttackKind::BALL_ANCHOR})&&charged&&readable,
                  "final-phase crosswind chains into two committed grading passes and a cable weight, each freshly warned");
            combo.ballFollow=AttackKind::BALL_GRADE;combo.stagger(1);
            CHECK(combo.ballDrive==0&&combo.ballFollow==AttackKind::NONE,"parry stagger cancels Ballast's drive and queued follow-up");
        }
        for(float hp:{1.f,.4f,.15f}) {
            for(auto policy:{BossSim::STILL,BossSim::CORNER,BossSim::PERCH,BossSim::RANGED}) {
                BossSim sim(road,g,road.arenas[6],EnemyType::BALLAST,policy);sim.boss.health=sim.boss.maxHealth*hp;
                for(int i=0;i<60*65;++i)sim.step(DT);
                std::printf("  phase hp %.2f policy %d damage %.0f\n",hp,(int)policy,sim.damageTaken);
                CHECK(sim.damageTaken>=70.f,"idling, corners, elevation and long range all remain threatened in each phase");
            }
        }
        BossSim sim(road,g,road.arenas[6],EnemyType::BALLAST,BossSim::SOLID);
        for(int i=0;i<60*220&&sim.bossAlive();++i)sim.step(DT);
        std::printf("  skilled Ballast run %.1fs, damage %.0f, parries %d\n",sim.t,sim.damageTaken,sim.parries);
        CHECK(!sim.bossAlive()&&sim.t>60&&sim.t<200,"a skilled gun-and-movement policy beats the whole Ballast fight within its pacing budget");
    }
    std::printf("%s (%d failures)\n",failures?"FAILED":"ALL PASSED",failures);return failures?1:0;
}
