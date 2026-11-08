#pragma once
#include "game/Progression.h"
#include <array>
#include <cmath>

namespace bosspractice {
inline bool supports(int boss){return boss==1 || boss==7 || boss==8 || boss==9;}
inline void startPhase(Enemy& e,int phase) {
    phase=std::clamp(phase,1,3);
    float share=1.f;
    if(e.type==EnemyType::SOVEREIGN) {
        share=phase==1?1.f:phase==2?.45f:.15f;
        e.enraged=phase>1;
    } else if(e.type==EnemyType::SORTER || e.type==EnemyType::SUCCESSOR)share=phase==1?1.f:phase==2?.6f:.25f;
    else if(e.type==EnemyType::BALLAST)share=phase==1?1.f:phase==2?.5f:.18f;
    e.health=e.maxHealth*share;
}
}

// The original six fights in their encounter order. Owned by App across level
// transitions, separate from an individual GameplayState and ordinary records.
struct BossRush {
    static constexpr int COUNT=6;
    bool active=false;
    int stage=0;
    float health=100;
    std::array<float,COUNT> splits{};
    void begin(){*this=BossRush{};active=true;}
    bool last()const{return stage==COUNT-1;}
    bool record(float seconds){
        if(!active || stage<0 || stage>=COUNT || splits[stage]>0 || !std::isfinite(seconds) || seconds<=0)return false;
        splits[stage]=seconds;return true;
    }
    bool advance(float hp){
        if(!active || last() || splits[stage]<=0)return false;
        health=std::clamp(hp+25.f,1.f,100.f);++stage;return true;
    }
    float total()const{float t=0;for(float s:splits)t+=s;return t;}
    bool complete()const{return active && last() && splits.back()>0;}
};

struct BossRushRecords {
    inline static const char* key="bossrush.v1.cfg";
    std::array<float,4> best{};
    void load(){
        best={};std::istringstream in(persist::load(key));std::string tag;int version=0;
        std::array<float,4> values{};
        if(!(in>>tag>>version)||tag!="bossrush"||version!=1)return;
        for(float& t:values)if(!(in>>t)||!std::isfinite(t)||t<0)return;
        best=values;
    }
    bool record(int difficulty,float time){
        if(difficulty<0||difficulty>3||!std::isfinite(time)||time<=0)return false;
        float& old=best[difficulty];if(old>0 && old<=time)return false;
        old=time;std::ostringstream out;out<<"bossrush 1";
        for(float t:best)out<<' '<<t;
        out<<'\n';persist::save(key,out.str());return true;
    }
};
