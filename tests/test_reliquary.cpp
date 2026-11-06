#include "world/LevelAct2.h"
#include "world/LevelTraining.h"
#include "game/WaveDirector.h"
#include <cstdio>
int main() {
    int failures=0;
    auto check=[&](bool ok,const char* msg){std::printf("%s %s\n",ok?"ok:":"FAIL:",msg);failures+=!ok;};
    auto level=buildAct2Level();auto& fm=level.formation;
    bool spawns=true,squads=true,landings=true;
    const EnemyType types[]={EnemyType::HUSK,EnemyType::REVENANT,EnemyType::WEAVER,EnemyType::ANCHOR,
        EnemyType::SENTINEL,EnemyType::RIPPER,EnemyType::MITE,EnemyType::SHIELDBEARER,
        EnemyType::JUGGERNAUT,EnemyType::CONDUCTOR,EnemyType::SERAPH,EnemyType::RAPTOR};
    WaveDirector director;director.level=&level;director.arena=3;
    for(int arrangement=0;arrangement<4;++arrangement) {
        fm.at=fm.to=arrangement;fm.t=0;fm.apply(level);level.updateMovers(0);
        for(int c=0;c<(int)fm.chunks.size();++c){glm::vec3 p;landings &= relicLanding(level,c,.4f,1.8f,p)&&spawnClearance(level,p,.4f,1.8f,true);}
        if(arrangement==3)continue;
        director.wave=arrangement;
        for(auto type:types)for(int seed=0;seed<80;++seed) {
            std::srand(seed);auto p=director.pickSpawnForTest(type,fm.top(seed%4));
            bool valid=spawnClearance(level,p,statsOf(type).radius,statsOf(type).height,!statsOf(type).flying);
            if(!valid&&spawns)std::printf("bad spawn: formation %d type %d at %.1f %.1f %.1f\n",arrangement,(int)type,p.x,p.y,p.z);
            spawns &= valid;
        }
        director.startArena(3);director.wave=arrangement;director.timer=0;
        std::vector<SpawnRequest> out;
        for(int tick=0;tick<1500 && (tick==0 || !director.queue.empty());++tick)
            director.update(1.f/60,0,fm.top(0),out);
        for(auto s:out)squads &= spawnClearance(level,s.pos,statsOf(s.type).radius,statsOf(s.type).height,!statsOf(s.type).flying);
    }
    check(spawns,"all Reliquary spawn selections clear each enemy's full body and supported feet over 80 seeds per type/formation");
    check(squads,"actual waves and escorts spawn on supported relics, including large bodies");
    check(landings,"all twelve relics have a clear player recovery point in every formation");
    bool movingLandings=true;
    for(int from=0;from<3;++from)for(float fraction:{.1f,.3f,.5f,.7f,.9f}) {
        fm.at=from;fm.to=from+1;fm.t=fraction*fm.GLIDE_TIME;fm.apply(level);level.updateMovers(0);
        for(int c=0;c<(int)fm.chunks.size();++c) {
            glm::vec3 p;
            movingLandings &= relicLanding(level,c,.4f,1.8f,p) && spawnClearance(level,p,.4f,1.8f,true);
        }
    }
    check(movingLandings,"recovery remains supported throughout all three platform glides");
    auto training=buildTraining();director.level=&training;director.startArena(0);
    check(!director.bossWave(),"a scripted training arena without waves is safely classified as non-boss");
    std::printf("%d Reliquary failures\n",failures);return failures?1:0;
}
