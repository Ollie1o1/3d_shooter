#pragma once

inline void GameplayState::setupStoryChoices() {
    const auto& s=level.story;
    auto control=[&](glm::vec3 at,const char* label,std::function<void()> use) {
        Interactable it;it.position=at+glm::vec3{0,1.4f,0};
        it.box={it.position-glm::vec3{.8f,.6f,.8f},it.position+glm::vec3{.8f,.6f,.8f}};
        it.label=label;it.onInteract=std::move(use);interactables.push_back(std::move(it));
    };
    control(s.relayIsolation,"ISOLATE SECURITY FEED / STOP REINFORCEMENTS",[this](){
        if(storyRelayIsolated)return;
        storyRelayIsolated=true;level.arenas[2].goals[0].refill=false;
        if(director.arena==2 && director.fighting())director.queue.clear();
        audio.play("clank",100,SoundGroup::WORLD,true);
        ui.feed("SECURITY FEED OFF / CLEAR THE GUARDS, THEN SEAT THE CONTACT",{.35f,1,.65f});
        saveStoryProgress();
    });
    control(s.winchControl,"RELEASE COUNTERWEIGHT / KEEP CLEAR OF MARKED FLOOR",[this](){
        if(storyWinchT>=0 || storyWinchLanded)return;
        storyWinchT=0;
        audio.playAt("telegraph",level.story.winchDrop,110,SoundGroup::WORLD,true);
        ui.feed("WINCH RELEASE / CLEAR THE GOLD DROP ZONE",{1,.75f,.25f});
    });
}

inline void GameplayState::updateStoryChoices(float dt) {
    auto& s=level.story;
    for(auto& it:interactables) {
        if(it.label.rfind("ISOLATE SECURITY",0)==0)it.active=!storyRelayIsolated;
        if(it.label.rfind("RELEASE COUNTERWEIGHT",0)==0)it.active=storyWinchT<0&&!storyWinchLanded;
    }
    if(s.winchWall<0 || storyWinchT<0 || storyWinchLanded)return;
    storyWinchT+=dt;
    float height=9.f*(1.f-std::clamp((storyWinchT-1.3f)/.45f,0.f,1.f));
    level.walls[s.winchWall].box={s.winchDrop+glm::vec3{-2,height,-2},s.winchDrop+glm::vec3{2,height+2,2}};
    if(storyWinchT<1.75f)return;
    storyWinchLanded=true;spatialGrid.build(level.walls);
    const glm::vec3 impact=s.winchDrop+glm::vec3{0,1,0};
    fx.spawnShockwave(s.winchDrop+glm::vec3{0,.1f,0},7.f,{1,.7f,.2f});
    fx.spawnBurst(impact,{.6f,.5f,.35f},35,8.f,1.f,9.f);
    audio.playAt("slam",impact,128,SoundGroup::WORLD,true);
    shake(.25f,.05f);
    for(auto& e:enemies)if(e.targetable() && !isBoss(e.type) && glm::length(e.position-impact)<7.f)
        hurtEnemy(e,500.f,e.position,8.f,2.f,StyleSource::ENVIRONMENT,false,true);
    if(glm::length(player.position-impact)<3.5f)damagePlayer(55.f,impact,.25f,.05f);
}

inline void GameplayState::gatherStoryChoices(std::vector<BoxInstance>& out) {
    using namespace rig;
    Rig r{out};const auto& s=level.story;
    const glm::vec3 on{1.2f,.7f,.2f},off{.2f,1.1f,.6f};
    auto panel=[&](glm::vec3 p,bool used){
        auto root=T(p+glm::vec3{0,1.6f,0});
        r.box(root,{0,0,0},{1.5f,.75f,.18f},{.04f,.07f,.09f});
        for(int k=0;k<3;++k)r.box(root,{-.45f+k*.45f,0,-.13f},{.22f,.3f,.08f},{.4f,.5f,.55f},used?off:on);
    };
    panel(s.relayIsolation,storyRelayIsolated);panel(s.winchControl,storyWinchLanded);
    if(s.winchWall<0)return;
    const auto& box=level.walls[s.winchWall].box;
    r.box(T((box.min+box.max)*.5f),{0,0,0},box.max-box.min,{.28f,.32f,.34f});
    for(float x:{-1.5f,0.f,1.5f})r.box(T((box.min+box.max)*.5f),{x,0,2.02f},{.5f,1.6f,.04f},{.8f,.57f,.17f});
    if(!storyWinchLanded) {
        glm::vec3 top{13,13,-112};
        r.box(T(top),{0,(box.max.y-13.f)*.5f,0},{.1f,13.f-box.max.y,.1f},{.5f,.55f,.58f});
        float glow=storyWinchT>=0?1.4f+.5f*std::sin(gameClock*15.f):.3f;
        for(float side:{-1.f,1.f}) {
            r.box(T(s.winchDrop),{side*3.5f,.04f,0},{.16f,.06f,7.f},{.65f,.45f,.15f},on*glow);
            r.box(T(s.winchDrop),{0,.04f,side*3.5f},{7.f,.06f,.16f},{.65f,.45f,.15f},on*glow);
        }
    }
}
