#include "core/Balance.h"
#include "game/BossChallenge.h"
#include "game/EncounterDraft.h"
#include "game/EncounterTrap.h"
#include "game/EndlessChallenge.h"
#include "game/EndlessWaves.h"
#include "game/RunLog.h"
#include "game/WaveDirector.h"
#include "world/LevelLab.h"
#include "MapAudit.h"
#include <cstdio>
#include <limits>

static int failures=0;
#define CHECK(c,m) do{bool ok=(c);std::printf("%s %s\n",ok?"ok:":"FAIL:",m);failures+=!ok;}while(0)
int main(){
    balanceInit();
    EndlessChallenge code;
    CHECK(code.parse("OD1-4294967295-3") && code.seed==UINT32_MAX && code.difficulty==3,"challenge codes retain the full seed range and difficulty");
    bool rejected=true;const auto before=code.code();
    for(const auto& s:{"OD2-12-1","OD1--1","OD1-4294967296-1","OD1-0-4","OD1-1-1x","OD1-+2-1","OD1-1-1\n"})
        rejected&=!code.parse(s) && code.code()==before;
    CHECK(rejected,"invalid or unsupported challenge codes cannot partially replace a valid challenge");
    CHECK(code.parse("OD1-0-0") && code.seed==0 && code.code()=="OD1-0-0","zero is a valid shareable seed");
    EndlessWaves a,b;a.begin(12345,{},true);b.begin(12345,{},true);bool same=true;
    for(int n=0;n<50;++n){auto x=a.wave(n),y=b.wave(n);same&=x.first.size()==y.first.size() && x.second.kind==y.second.kind;
        for(size_t j=0;j<x.first.size();++j)same&=x.first[j].type==y.first[j].type && x.first[j].count==y.first[j].count && x.first[j].escort==y.first[j].escort;}
    CHECK(same,"a shared seed reproduces fifty complete wave compositions including objectives and bosses");
    EncounterTrap trap;CHECK(trap.trigger()&&!trap.trigger(),"an environmental load accepts one release per attempt");
    CHECK(!trap.update(1.29f)&&trap.progress()==0,"the full warning occurs before the load moves");
    CHECK(!trap.update(.31f)&&trap.progress()>.45f&&trap.progress()<.55f,"falling collision follows the announced drop interval");
    CHECK(trap.update(.31f)&&trap.landed&&!trap.update(1.f)&&trap.progress()==1,"impact occurs exactly once even across a long tick");
    auto level=buildLab();EncounterDraft draft;draft.capture(level,2,"story3");std::string error;
    const auto measured=audit::measure(level,2);
    std::printf("Motor Assessment: %.1f%% cover / %.1f m sight / %d layers / %.2f lights\n",measured.cover*100,measured.sight,measured.levels,measured.lights);
    CHECK(measured.cover>=.25f&&measured.cover<=.5f&&measured.sight>=12&&measured.sight<=30&&measured.levels>=3&&measured.lights>=.5f,
        "Motor Assessment meets the shared cover, sightline, verticality and lighting standard");
    CHECK(std::string(level.story.terminals.back().title)=="MANUAL CALIBRATION","the optional record appends an ID instead of shifting existing saved discoveries");
    const auto original=draft.text();
    CHECK(draft.validate(level).empty(),"the revised Motor Assessment remains a valid authored encounter");
    draft.waves[0][0].at[0]=lab::centre(2)+glm::vec3{-22,.1f,-26};
    CHECK(!draft.apply(level,error) && level.arenas[2].waves[0][0].at[0]!=draft.waves[0][0].at[0],"a spawn embedded in the new cover is rejected before applying any edits");
    CHECK(draft.read(original,level,error),"saved encounter patches round-trip with authored escorts and explicit spawns");
    draft.waves[0][0].at.push_back(lab::centre(2)+glm::vec3{7,0,-34});draft.maxAlive=15;
    CHECK(draft.apply(level,error),"a valid extra enemy and concurrent cap apply together");
    WaveDirector director;director.level=&level;director.fast=true;director.startArena(2);
    std::vector<SpawnRequest> out;director.update(1.f/60,0,level.arenas[2].playerStart,out);director.update(1.f/60,0,level.arenas[2].playerStart,out);
    CHECK(out.size()==15,"the real director emits the workshop's edited fifteen-enemy encounter");
    const auto good=draft.text();
    CHECK(!draft.read(good.substr(0,good.size()/2),level,error) && draft.text()==good,"truncated patches leave the current draft intact");
    std::string wrong=good;wrong.replace(wrong.find("story3"),6,"story2");
    CHECK(!draft.read(wrong,level,error)&&draft.text()==good,"patches for another map cannot change the draft");
    draft.waves[0][0].at[0].x=std::numeric_limits<float>::quiet_NaN();
    CHECK(!draft.apply(level,error),"non-finite spawn coordinates are rejected");
    BossRush rush;rush.begin();bool chain=!rush.advance(40);
    for(int i=0;i<BossRush::COUNT;++i){chain&=rush.record(10.f+i)&&!rush.record(1);if(i+1<BossRush::COUNT)chain&=rush.advance(40)&&rush.health==65;}
    CHECK(chain&&rush.complete()&&rush.total()==75&&!rush.advance(10),"rush stages require completed fights, retain splits and give exactly twenty-five recovery health");
    rush.begin();CHECK(rush.stage==0&&rush.total()==0&&rush.health==100&&!rush.complete(),"a failed rush restarts its whole sequence");
    EnemyWorld world;world.bounds={{-80,0,-80},{80,30,80}};world.playerFeet={0,0,16};world.playerEye={0,1.7f,16};
    bool phases=true;
    for(auto type:{EnemyType::SOVEREIGN,EnemyType::SORTER,EnemyType::BALLAST,EnemyType::SUCCESSOR})for(int phase=1;phase<=3;++phase){
        Enemy e(type,{0,0,0},0);e.spawnTimer=0;e.state=EnemyState::ACTIVE;bosspractice::startPhase(e,phase);e.update(1.f/60,world);
        if(type==EnemyType::SOVEREIGN)phases&=(e.enraged==(phase>1)) && (e.health<e.maxHealth*.2f)==(phase==3);
        if(type==EnemyType::SORTER)phases&=e.sortPhase==phase;
        if(type==EnemyType::BALLAST)phases&=e.ballPhase==phase;
        if(type==EnemyType::SUCCESSOR)phases&=e.suPhase==phase;
    }
    CHECK(phases,"all twelve phase-practice starts enter the requested phase through each boss's real AI");
    BossRushRecords::key="build/test-bossrush.cfg";std::remove(BossRushRecords::key);BossRushRecords records;
    CHECK(records.record(1,75)&&!records.record(1,80)&&records.record(2,100),"rush bests are separate by difficulty and only improve");
    BossRushRecords saved;saved.load();CHECK(saved.best[1]==75&&saved.best[2]==100,"rush records persist separately from ordinary boss records");
    persist::save(BossRushRecords::key,"bossrush 1 0 3 -2 0");saved.load();CHECK(saved.best[1]==0,"malformed rush records do not partially load");std::remove(BossRushRecords::key);
    RunLog log;log.storyTick(25,3,2,{1,2,3},30,false);log.parryAttempts=3;log.parrySuccesses=1;log.retries=1;
    const auto json=log.json("retry","STORY",1,"MOTOR ASSESSMENT",1,25,true,true);
    CHECK(log.any()&&json.find("\"parryAttempts\":3")!=std::string::npos&&json.find("\"capture\":true")!=std::string::npos,"exploration, defensive inputs, retries and capture classification survive the local log");
    std::printf("%d failures\n",failures);return failures?1:0;
}
