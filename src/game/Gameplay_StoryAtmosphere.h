#pragma once
// Small changes in machinery, pressure and room lighting give the factory a
// life between encounters. These effects have no hidden damage or collision.
inline void GameplayState::updateStoryAtmosphere(float dt) {
    const int room=level.arenaAt(player.position);
    if(playerDead||storyLocked())return;
    for(const auto& vent:level.story.vents) {
        if(vent.section!=room || glm::length(vent.pos-player.position)>28.f)continue;
        float now=(gameClock+vent.phase)/vent.period,was=(gameClock-dt+vent.phase)/vent.period;
        if(std::floor(now)==std::floor(was))continue;
        fx.spawnAmbientBurst(vent.pos,{.21f,.25f,.26f},5,.55f,1.3f,-.3f);
        if(!director.fighting())audio.playAt("clank",vent.pos,22,SoundGroup::WORLD);
    }
    const float A=level.arenas[3].playerStart.y,F=A-7.f;
    if(room==3 && std::floor(gameClock/4.6f)!=std::floor((gameClock-dt)/4.6f)) {
        int station=(int)(gameClock/4.6f)%3;
        glm::vec3 at{70.f+11.f*station,F+.8f,44};
        fx.spawnAmbientBurst(at,{.75f,.45f,.18f},5,1.1f,.35f,6.f);
        if(!director.fighting())audio.playAt("casing",at,34,SoundGroup::WORLD);
    }
    if(ambientDetail()>0 && (room==1||room==3||room==6) &&
       std::floor(gameClock*2.f)!=std::floor((gameClock-dt)*2.f)) {
        glm::vec3 source=player.position+glm::vec3{5.f*std::sin(gameClock*1.1f),5.f,5.f*std::cos(gameClock*.7f)};
        fx.spawnAmbientBurst(source,{.32f,.28f,.22f},5,.25f,2.5f,1.7f);
    }
    // The vault has a genuine state change: cold standby becomes a warm
    // containment alarm when the glass breaks. The room stays readable.
    if(room==5) {
        level.arenas[5].theme.skyAmb=storyPodsBroken?glm::vec3{.34f,.25f,.24f}:glm::vec3{.34f,.42f,.5f};
        level.arenas[5].theme.groundAmb=storyPodsBroken?glm::vec3{.2f,.14f,.14f}:glm::vec3{.19f,.25f,.31f};
    }
    if(storyBossDown)level.arenas[8].ambient=Ambient::ASH;
}

inline void GameplayState::gatherStoryAtmosphere(std::vector<BoxInstance>& out) {
    using namespace rig;using glm::vec3;
    const auto& S=level.story;
    const float A=level.arenas[3].playerStart.y,F=A-7.f,V=A-4.f,T=V+39.f;
    const int room=level.arenaAt(player.position);
    Rig r{out};
    auto lamp=[&](vec3 p,vec3 size,vec3 emission){r.box(rig::T(p),{0,0,0},size,{.22f,.26f,.26f},emission);};
    if(room==3 && ambientDetail()>0) {
        // Power travels down the production lines as each fixture cycles.
        for(auto p:S.assemblyUnits) {
            float phase=gameClock*.9f-p.z*.12f-p.x*.07f;
            float active=.35f+.35f*(.5f+.5f*std::sin(phase));
            lamp(p+vec3{0,4.7f,.63f},{1.9f,.1f,.06f},{active,active*.64f,active*.2f});
            float z=p.z-1.2f+2.4f*std::fmod(gameClock*.18f+p.x*.01f,1.f);
            lamp({p.x,F+.34f,z},{1.8f,.025f,.06f},{.23f,.3f,.26f});
        }
    }
    if(room==4 && ambientDetail()>0) {
        // Slow data activity on the front faces, without a flashing full-room effect.
        for(int k=0;k<3;++k) {
            float x=25.5f+k*10;
            int row=((int)(gameClock*.7f)+k*3)%7;
            lamp({x+.7f,A+.45f+row*.66f,80.85f},{.1f,.09f,.04f},{.15f,.43f,.4f});
            lamp({x+.7f,A+.38f+row*.48f,66.85f},{.1f,.07f,.04f},{.15f,.43f,.4f});
        }
    }
    if(room==5)for(auto p:S.pods) {
        vec3 color=storyPodsBroken?vec3{1.1f,.26f,.13f}:vec3{.12f,.35f,.55f};
        float pulse=storyPodsBroken?.7f+.15f*std::sin(gameClock*1.6f+p.x*.1f):.8f;
        lamp({p.x,V+4.8f,p.z+1.55f},{1.5f,.14f,.08f},color*pulse);
    }
    if(room==6 && S.elevator>=0) {
        float drive=level.movers[S.elevator].drive;
        for(float x:{-35.5f,-14.5f}) {
            float y=V+39.f*(1.f-drive)+1.5f;
            // Counterweights stay beside the rails, outside the deck and landings.
            r.box(rig::T({x,y,-2.5f}),{0,0,0},{.55f,3.f,1.1f},{.27f,.3f,.31f});
            for(float dx:{-.12f,.12f})r.box(rig::T({x+dx,(T+8+y)*.5f,-2.5f}),{0,0,0},{.035f,T+8-y,.035f},{.43f,.4f,.32f});
        }
    }
    if(room==7)for(int k=0;k<6;++k) {
        float x=-4.f+9*k;
        vec3 c=storyScanCooldown>0?vec3{.8f,.18f,.08f}:vec3{.13f,.44f,.26f};
        lamp({x,T+6.25f,14.7f},{.3f,.14f,.1f},c);
    }
}
