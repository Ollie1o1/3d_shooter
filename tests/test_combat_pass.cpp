#include "core/Balance.h"
#include "game/WaveDirector.h"
#include "game/EncounterDraft.h"
#include "player/StyleSystem.h"
#include "world/LevelLab.h"
#include <cstdio>

static int failures=0;
#define CHECK(c,m) do{bool ok=(c);std::printf("%s %s\n",ok?"ok:":"FAIL:",m);failures+=!ok;}while(0)
constexpr float DT=1.f/60;
int main() {
    balanceInit();auto level=buildLab();const auto& room=level.arenas[3];
    WaveDirector director;director.level=&level;director.fast=true;director.startArena(3);
    std::vector<SpawnRequest> out;
    director.update(DT,0,room.playerStart,out);director.update(DT,0,room.playerStart,out);
    CHECK(out.size()==5 && director.queued()==5,"Surgical Observation opens with five enemies and reserves five reinforcements");
    bool roles=false;int suspended=0,grafted=0,patients=0;
    for(const auto& s:out){suspended+=s.type==EnemyType::SUSPENDED;grafted+=s.type==EnemyType::GRAFTED;patients+=s.type==EnemyType::PATIENT;}
    roles=suspended==1 && grafted==1 && patients==3;
    CHECK(roles,"the opening combines a beam lane, an armoured approach and three rushers");
    director.events.clear();out.clear();director.update(.5f,3,room.playerStart,out);
    CHECK(out.empty() && director.events.empty(),"two casualties cannot release the next group");
    director.update(DT,2,room.playerStart,out);
    int warnings=0;for(auto event:director.events)warnings+=event.kind==DirectorEvent::REINFORCEMENTS;
    CHECK(warnings==1 && out.empty(),"the third casualty announces one reinforcement group before spawning anything");
    director.events.clear();director.update(1.1f,2,room.playerStart,out);
    CHECK(out.empty(),"the 1.2-second warning cannot damage the player or spawn enemies early");
    director.update(.11f,2,room.playerStart,out);
    CHECK(out.size()==5 && director.queue.empty(),"the complete second group arrives after the warning while earlier enemies may remain");
    int emitted=10;out.clear();director.update(DT,0,room.playerStart,out);
    CHECK(director.phase==WaveDirector::Phase::BREAK && director.wave==0,"clearing the first wave enters its authored breather");
    director.update(1.2f,0,room.playerStart,out);
    CHECK(director.phase==WaveDirector::Phase::BREAK,"the breather does not immediately dump the next wave");
    for(int i=0;i<240 && director.arena==3;++i){out.clear();director.update(DT,0,room.playerStart,out);emitted+=(int)out.size();}
    CHECK(emitted==20 && director.arena==4,"both staged waves complete with exactly the original twenty-enemy budget");
    director.startArena(3);out.clear();director.update(DT,0,room.playerStart,out);director.update(DT,0,room.playerStart,out);
    CHECK(out.size()==5 && director.queued()==5,"retry resets casualty counts, queued warnings and reinforcement timing");

    EncounterDraft draft;draft.capture(level,3,"story3");std::string error;
    CHECK(draft.validate(level).empty(),"the authored staged encounter has safe, supported, separated spawns");
    const auto patch=draft.text();
    CHECK(draft.read(patch,level,error) && draft.waves[0].back().afterKills==3,"workshop patches retain reinforcement stages across save/load");
    for(auto& entry:draft.waves[0])entry.afterKills=3;
    CHECK(!draft.apply(level,error),"workshop validation rejects a wave whose opening is locked behind its own casualties");
    draft.capture(level,3,"story3");
    const std::string legacy="encounter 1 story3 3 14 2\n1\n0 1 0 0 1\n\n-19 -42 -364\n1\n0 1 0 0 1\n\n-19 -42 -364\n";
    CHECK(draft.read(legacy,level,error) && draft.waves[0][0].afterKills==0,"existing version-one workshop patches remain loadable as immediate entries");
    auto small=buildLab();small.arenas[3].waves={{{EnemyType::PATIENT,4},WaveEntry(EnemyType::HUSK,2).after(4)}};
    WaveDirector reduced;reduced.level=&small;reduced.fast=true;reduced.countScale=.5f;reduced.startArena(3);
    emitted=0;for(int i=0;i<240 && reduced.arena==3;++i){out.clear();reduced.update(DT,0,room.playerStart,out);emitted+=(int)out.size();}
    CHECK(emitted==3 && reduced.arena==4,"reduced automatic wave counts cannot make a casualty gate unreachable");

    StyleSystem style;style.addStyle(400);
    CHECK(style.overdriveReady && !style.overdrive,"earning a full meter banks a charge instead of wasting it on the final kill");
    style.update(12.f);
    CHECK(style.overdriveReady && !style.overdrive && style.overdriveTimer==0,"a banked charge survives a quiet room without consuming active seconds");
    CHECK(style.activateOverdrive() && !style.overdriveReady && style.overdriveTimer==StyleSystem::OVERDRIVE_DURATION,"the next attack can consume one banked charge");
    CHECK(!style.activateOverdrive(),"an active charge cannot be consumed twice");
    style.update(9.f);style.addStyle(400);
    CHECK(!style.overdriveReady && !style.activateOverdrive(),"earning style during cooldown cannot reactivate Overdrive early");
    style.update(StyleSystem::OVERDRIVE_COOL);
    CHECK(style.overdriveReady && !style.overdrive,"a cooled full meter becomes ready for another offensive action");
    StyleSystem setups;setups.style=80;float before=setups.earned;
    setups.addStyle(25,StyleSource::ENVIRONMENT,1,true);
    CHECK(setups.earned>before && setups.style==80 && !setups.overdriveReady,"deliberate environmental setups retain run points above the passive meter cap");
    before=setups.earned;setups.addStyle(25,StyleSource::ENVIRONMENT);
    CHECK(setups.earned==before && setups.style==80,"unrelated environmental deaths still cannot farm score at the cap");

    Enemy clone(EnemyType::UNIT24K,{0,0,0});clone.spawnTimer=0;clone.state=EnemyState::ACTIVE;
    clone.attack=AttackKind::SHOT;clone.telegraphTimer=clone.telegraphDuration=.8f;
    CHECK(!clone.onPrecisionCounter(60,false) && !clone.onPrecisionCounter(10,true),"body hits and tiny pellets do not cancel 24-K's Lancer charge");
    CHECK(clone.onPrecisionCounter(60,true) && clone.staggered() && clone.attack==AttackKind::NONE,"a committed precision hit breaks the charge and creates a punish window");
    EnemyWorld world;world.bounds={{-30,0,-30},{30,20,30}};world.playerFeet={0,0,15};world.playerEye={0,1.7f,15};
    bool fired=false;for(int i=0;i<45;++i){clone.update(DT,world);fired|=clone.ev.shots>0;}
    CHECK(!fired,"the interrupted charge cannot emit its queued shot during the real AI recovery");
    clone.attack=AttackKind::DASH;clone.telegraphTimer=.4f;
    CHECK(!clone.onPrecisionCounter(80,true),"24-K's white rush keeps its existing parry response");
    std::printf("%d combat-pass failures\n",failures);return failures?1:0;
}
