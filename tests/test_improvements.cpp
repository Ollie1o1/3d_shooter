#include "game/StorySave.h"
#include "game/CombatLessons.h"
#include "world/LevelTraining.h"
#include "world/LevelWasteland.h"
#include "game/WaveDirector.h"
#include "render/LightBake.h"
#include <cstdio>

static int failures=0;
#define CHECK(c,m) do {if(!(c)){++failures;std::printf("FAIL: %s\n",m);}else std::printf("ok: %s\n",m);}while(0)
int main() {
    StorySave original;
    original.chapter=2;original.checkpoint=4;original.owned=15;original.logs=5;original.terminals=3;
    original.grapple=original.wakeSeen=original.relayIsolated=true;
    original.prog.addXp(700);original.prog.buy(WeaponId::SHOTGUN,UpgradeStat::MOD);
    original.prog.up[1].alternate=true;original.elapsed=1234.75f;original.kills=37;original.deaths=3;
    original.style=1000;original.damage=150;
    StorySave restored;
    CHECK(restored.parse(original.text()) && restored.text()==original.text(),"Story save preserves chapter, equipment, discoveries, mods, choices and statistics");
    const std::string good=restored.text();
    CHECK(!restored.parse(good.substr(0,good.size()/2)) && restored.text()==good,"truncated saves cannot partially replace a valid checkpoint");
    auto bad=original;bad.chapter=5;CHECK(!restored.parse(bad.text()),"future chapters are rejected safely");
    bad=original;bad.checkpoint=8;CHECK(!restored.parse(bad.text()),"a chapter cannot restore an out-of-bounds checkpoint");
    bad=original;bad.prog.up[0].tier[0]=100;CHECK(!restored.parse(bad.text()),"corrupt upgrade tiers cannot reach gameplay");
    bad=original;bad.elapsed=-1;CHECK(!restored.parse(bad.text()),"invalid run times are rejected");
    CHECK(!restored.parse(good+"trailing garbage") && !restored.parse("OVERDRIVE_STORY 999\n"),"unknown versions and trailing corruption are rejected");
    Records legacy;legacy.parse("bestBoss 6 50 60 70 80 90 100\n");
    CHECK(legacy.bestBoss[5]==100 && legacy.bestBoss[6]==0 && legacy.bestBoss[8]==0,"the expanded boss menu preserves all six existing personal records");
    WeaponUpgrades a;a.mod=true;auto b=a;b.alternate=true;
    CHECK(weaponPierce(WeaponId::REVOLVER,a)>weaponPierce(WeaponId::REVOLVER,b)
        && weaponHeadMult(WeaponId::REVOLVER,b)>weaponHeadMult(WeaponId::REVOLVER,a)
        && weaponBodyMult(WeaponId::REVOLVER,b)<weaponBodyMult(WeaponId::REVOLVER,a),"revolver mods trade crowd penetration for precision and weaker body shots");
    CHECK(weaponPellets(WeaponId::SHOTGUN,a)>weaponPellets(WeaponId::SHOTGUN,b)
        && weaponKickMult(WeaponId::SHOTGUN,b)>weaponKickMult(WeaponId::SHOTGUN,a),"shotgun mods trade extra pellets for movement recoil");
    CHECK(weaponAirSpread(WeaponId::KAR,b,0)==0 && weaponSpread(WeaponId::KAR,b,0)<weaponSpread(WeaponId::KAR,a,0),"Skirmisher supports airborne hip fire");
    CHECK(weaponPierce(WeaponId::LONGSHOT,b)==weaponPierce(WeaponId::LONGSHOT,a)+3,"Rail Driver reaches three additional targets");
    auto training=buildTraining();
    CHECK(training.arenas.size()==1 && training.arenas[0].scripted && training.arenas[0].waves.empty(),"practice has no wave-clear, ranking or victory trigger");
    bool ledge=false,rail=false;
    for(const auto& w:training.walls) {
        ledge|=w.box.min.x<=0 && w.box.max.x>=0 && w.box.min.z<=-14 && w.box.max.z>=-14 && w.box.max.y==4;
        rail|=w.box.min.y==9 && w.box.min.x<0 && w.box.max.x>12;
    }
    CHECK(ledge&&rail,"the grapple lesson has a real gunner ledge and reachable overhead rail");
    auto road=buildWasteland();WaveDirector director;director.level=&road;director.fast=true;director.startArena(2);
    std::vector<SpawnRequest> spawned;director.update(3,0,road.arenas[2].playerStart,spawned);
    road.arenas[2].goals[0].refill=false;director.queue.clear();spawned.clear();
    for(int i=0;i<1200;++i)director.update(1.f/60,0,road.arenas[2].playerStart,spawned);
    CHECK(spawned.empty()&&!director.goalDone,"isolating the feed stops new guards while leaving the relay repair necessary");
    auto pad=road.arenas[2].goals[0].pos;
    for(int i=0;i<560;++i)director.update(1.f/60,0,pad,spawned);
    CHECK(director.arena==3,"the isolated relay can still complete and open the route");
    {   // The relay's guards: far from the pad, and a quiet window after each detail
        auto r=buildWasteland();WaveDirector d;d.level=&r;d.fast=true;d.startArena(2);
        auto pad2=r.arenas[2].goals[0].pos;
        std::vector<SpawnRequest> out;d.update(3,0,pad2,out);
        for(int i=0;i<5;++i)d.update(1.f/60,(int)out.size(),pad2,out);
        bool far=true;for(auto& s:out)far&=glm::length(glm::vec2{s.pos.x-pad2.x,s.pos.z-pad2.z})>=20.f;
        CHECK(far&&!out.empty()&&out.size()<=4,"the relay's first guards arrive at the far end, away from the contact pad");
        bool open=true;
        for(auto p:r.arenas[2].groundSpawns)
            for(auto& w:r.walls)
                for(float h:{.3f,1.f,1.8f})
                    open&=!(p.x>w.box.min.x-.5f&&p.x<w.box.max.x+.5f&&p.z>w.box.min.z-.5f&&p.z<w.box.max.z+.5f&&p.y+h>w.box.min.y&&p.y+h<w.box.max.y);
        CHECK(open,"every relay spawn point stands on open floor");
        // Three still standing: no refill, however long it takes
        out.clear();for(int i=0;i<600;++i)d.update(1.f/60,3,{-56,-5,-51},out);
        CHECK(out.empty(),"no reinforcements while most of the guards are still up");
        // Down to none: nothing for the next 7 s, then at most two
        out.clear();int first=-1;
        for(int i=0;i<600&&first<0;++i){d.update(1.f/60,0,{-56,-5,-51},out);if(!out.empty())first=i;}
        CHECK(first>=60*6.9f&&out.size()<=2,"after clearing the room the next detail waits ~7 s and is only two guards");
    }
    CHECK(road.story.winchWall>=0 && road.walls[road.story.winchWall].dynamic && road.walls[road.story.winchWall].hidden,"the counterweight has real movable collision and a separate visible rig");
    std::string error;auto bake=lightbake::load("training",training,error);
    CHECK(!bake.empty(),"the practice arena ships with current baked lighting");
    return failures?1:0;
}
