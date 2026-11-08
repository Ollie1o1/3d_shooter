#pragma once
#include "world/Level.h"
#include "world/SpawnSafety.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Editable wave data, independent of UI and rendering. Authored objectives and
// geometry stay with the level. An imported patch is validated before applying.
struct EncounterDraft {
    int room=-1,maxAlive=1;
    std::string map;
    std::vector<std::vector<WaveEntry>> waves;
    void capture(const LevelData& level,int r,const std::string& key) {
        room=r;map=key;waves=level.arenas[r].waves;maxAlive=level.arenas[r].maxAlive;
    }
    static bool editable(const Arena& a) {
        if(a.waves.empty() || a.scripted)return false;
        for(const auto& wave:a.waves)for(const auto& entry:wave)if(isBoss(entry.type))return false;
        return true;
    }
    std::string validate(const LevelData& level)const {
        if(room<0 || room>=(int)level.arenas.size())return "Choose a room.";
        const auto& a=level.arenas[room];
        if(!editable(a))return "Scripted and boss encounters are read-only.";
        if(waves.size()!=a.waves.size())return "Keep the authored number of waves and objectives.";
        if(maxAlive<1 || maxAlive>40)return "Concurrent enemies must be between 1 and 40.";
        for(const auto& wave:waves) {
            if(wave.empty() || wave.size()>64)return "Each wave needs 1-64 entries.";
            struct Body{glm::vec3 position;float radius,height;};
            std::vector<Body> positions;
            int total=0;
            for(const auto& e:wave) {
                if((int)e.type<0 || e.type>=EnemyType::COUNT || isBoss(e.type) || e.type==EnemyType::CONDUIT)
                    return "Use ordinary enemies in editable waves.";
                if(e.at.size()>64 || e.count<0 || e.count>64 || (e.at.empty() && e.count<1) || e.escort.size()>8)
                    return "An entry needs 1-64 enemies or explicit positions.";
                if((int)e.variant<0 || (int)e.variant>3 || (e.variant!=Hollow::NONE && !canBeHollow(e.type)))return "Invalid enemy variant.";
                for(auto escort:e.escort)if((int)escort<0 || escort>=EnemyType::COUNT || isBoss(escort))return "Invalid escort.";
                total+=e.total();if(total>128)return "Keep each wave at or below 128 enemies.";
                const auto& st=statsOf(e.type);
                for(auto p:e.at) {
                    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z) ||
                       level.arenaAt(p)!=room || !spawnClearance(level,p,st.radius,st.height,!st.flying))
                        return "A spawn needs body clearance, room bounds and floor support.";
                    for(const auto& q:positions)if(glm::length(glm::vec2{p.x-q.position.x,p.z-q.position.z})<st.radius+q.radius &&
                        p.y<q.position.y+q.height && q.position.y<p.y+st.height)return "Separate overlapping spawn positions.";
                    positions.push_back({p,st.radius,st.height});
                }
            }
        }
        return {};
    }
    bool apply(LevelData& level,std::string& error)const {
        error=validate(level);if(!error.empty())return false;
        level.arenas[room].waves=waves;level.arenas[room].maxAlive=maxAlive;return true;
    }
    std::string text()const {
        std::ostringstream out;out<<std::setprecision(9);
        out<<"encounter 1 "<<map<<' '<<room<<' '<<maxAlive<<' '<<waves.size()<<'\n';
        for(const auto& wave:waves) {
            out<<wave.size()<<'\n';
            for(const auto& e:wave) {
                out<<(int)e.type<<' '<<e.count<<' '<<(int)e.variant<<' '<<e.escort.size()<<' '<<e.at.size()<<'\n';
                for(auto escort:e.escort)out<<(int)escort<<' ';
                out<<'\n';
                for(auto p:e.at)out<<p.x<<' '<<p.y<<' '<<p.z<<'\n';
            }
        }
        return out.str();
    }
    bool read(const std::string& text,const LevelData& level,std::string& error) {
        if(text.size()>256*1024){error="Patch is too large.";return false;}
        EncounterDraft parsed;std::istringstream in(text);std::string tag;int version=0,n=0;
        if(!(in>>tag>>version>>parsed.map>>parsed.room>>parsed.maxAlive>>n) || tag!="encounter" || version!=1 ||
            parsed.map!=map || parsed.room!=room || n<1 || n>32){error="Patch does not match this map and room.";return false;}
        parsed.waves.resize(n);
        for(auto& wave:parsed.waves) {
            int entries=0;if(!(in>>entries)||entries<1||entries>64){error="Invalid wave.";return false;}
            for(int j=0;j<entries;++j) {
                int type=0,count=0,variant=0,escorts=0,points=0;
                if(!(in>>type>>count>>variant>>escorts>>points) || variant<0 || variant>3 || escorts<0 || escorts>8 || points<0 || points>64)
                    {error="Invalid entry.";return false;}
                WaveEntry e((EnemyType)type,count);e.variant=(Hollow)variant;
                for(int k=0;k<escorts;++k){int t=0;if(!(in>>t)){error="Truncated escorts.";return false;}e.escort.push_back((EnemyType)t);}
                for(int k=0;k<points;++k){glm::vec3 p;if(!(in>>p.x>>p.y>>p.z)){error="Truncated positions.";return false;}e.at.push_back(p);}
                wave.push_back(e);
            }
        }
        in>>std::ws;if(!in.eof()){error="Unexpected trailing patch data.";return false;}
        error=parsed.validate(level);if(!error.empty())return false;
        *this=std::move(parsed);return true;
    }
};
