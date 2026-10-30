#pragma once
#include "game/StoryLift.h"
#include "bosses/sorter/SorterRig.h"
// Upper-factory sequences. Real platform walls are driven here; Player carries
// their delta in the normal physics path. No camera teleport is used in play.
inline void GameplayState::resetStoryFactory() {
    auto& S=level.story;
    for(const auto& cover:S.bossCover)level.walls[cover.wall].box=cover.home;
    spatialGrid.build(level.walls);
    if(wasteland()) {
        for(float& angle:storyTurbineAngle)angle=0;
        storyPodsBroken=false;storyBossDown=director.arena>level.story.bossSection;storyCollapseT=-1;storyTerminal=storyLog=-1;
        if(director.arena>4)for(int id:S.turbineBrakes)level.damageAnchor(id,1e9f);
        storyCheckpoint=director.arena;director.hold=false;return;
    }
    if(storyGlassBoxes.empty())for(int w:S.podGlass)storyGlassBoxes.push_back(level.walls[w].box);
    if(storyTileBoxes.empty())for(int w:S.sortingTiles)storyTileBoxes.push_back(level.walls[w].box);
    for(size_t i=0;i<S.podGlass.size();++i) {auto& w=level.walls[S.podGlass[i]];w.box=storyGlassBoxes[i];w.hidden=true;}
    for(size_t i=0;i<S.sortingTiles.size();++i) {auto& w=level.walls[S.sortingTiles[i]];w.box=storyTileBoxes[i];w.hidden=true;}
    storyPodsBroken=director.arena>5; storyBossDown=false;storyCollapseT=-1;
    level.arenas[8].ambient=Ambient::EMBERS;
    storyScanCooldown=0; storyTerminal=-1;storyLiftStop=0;director.hold=false;
    if(S.elevator>=0)level.movers[S.elevator].drive=director.arena>6?1.f:0.f;
    if(S.serviceLift>=0)level.movers[S.serviceLift].drive=director.arena>2?1.f:0.f;
    storyLog=-1; storyLogT=0;
    level.updateMovers(moverClock);spatialGrid.build(level.walls);
    storyCheckpoint=director.arena;
}

inline void GameplayState::updateStoryFactory(float dt) {
    using glm::vec3;
    auto& S=level.story; const vec3 p=player.position;
    const int at=level.arenaAt(p);
    if(at>=0 && at>storyCheckpoint && !director.fighting()) {
        storyCheckpoint=at;
        ui.feed("CHECKPOINT",{.35f,.85f,1.f});
    }
    // A quiet room has no encounter to advance the director. Walking into the
    // next room explicitly approaches it; active fights still own their gates.
    if(at>director.arena && !director.fighting() && director.phase!=WaveDirector::Phase::INTRO) director.approach(at);
    // Small, spatially grounded activity: the reclamation machinery still
    // sheds dust and the damaged panels arc occasionally, even between fights.
    if(at==1 && std::floor(gameClock/6.f)!=std::floor((gameClock-dt)/6.f)) {
        fx.spawnBurst({18.4f,-22,-31.5f},{.4f,.32f,.22f},10,2.f,2.2f,7.f);
        audio.playAt("land_metal",{19,-31,-31.5f},40,SoundGroup::WORLD);
    }
    if((at==1 || at==3) && std::floor(gameClock/8.f)!=std::floor((gameClock-dt)/8.f)) {
        vec3 spark=at==1?vec3{57.4f,-38.4f,-25}:vec3{107.5f,level.arenas[3].playerStart.y-5,36};
        fx.spawnBurst(spark,{1.f,.55f,.16f},6,2.5f,.5f,7.f);
    }
    if(!storyGrapple && at==7 && glm::length(p-S.grappleAt)<3.f) {
        storyGrapple=true;audio.play("pickup",128,SoundGroup::UI,true);
        pushBanner("THE GRAPPLE","Q - ATTACH / RELEASE   SPACE - JUMP OFF",{.35f,.85f,1.f},4.f);
    }
    // The pod fronts become debris at the ambush trigger, leaving no invisible collision.
    if(director.arena==5 && director.fighting() && !storyPodsBroken) {
        storyPodsBroken=true;
        for(int id:S.podGlass) {
            auto& w=level.walls[id];vec3 c=(w.box.min+w.box.max)*.5f;
            w.box.min.y-=500;w.box.max.y-=500;w.hidden=true;
            fx.spawnBurst(c,{.4f,.9f,1.f},14,6.f,.8f,12.f);
        }
        spatialGrid.build(level.walls);
        audio.play("explosion",105,SoundGroup::WORLD,true);shake(.65f,.08f);
    }
    updateStoryAtmosphere(dt);
    storylift::update(level,director,player,dt);
    if(S.elevator>=0 && director.arena==6) {
        int stop=(int)std::floor(level.movers[S.elevator].drive*3.f+.001f);
        if(stop>storyLiftStop) {storyLiftStop=stop;audio.play("anvil",100,SoundGroup::WORLD,true);}
    }
    // Sliding goes under the oscillating inspection strip; otherwise dash through
    // its narrow plane while it is high. The red beam has no hidden hit volume.
    storyScanCooldown=std::max(0.f,storyScanCooldown-dt);
    if(at==7 && storyScanCooldown<=0.f && !playerDead) {
        for(const auto& sc:S.scanners) {
            float y=sc.a.y+1.5f+1.25f*std::sin(gameClock*1.7f+sc.phase);
            if(std::fabs(p.x-sc.a.x)<.45f && p.z>1 && p.z<15 && y>p.y && y<p.y+player.height) {
                storyScanCooldown=5.f;
                audio.play("telegraph",128,SoundGroup::WORLD,true);
                ui.toast("MARKED","THE COLLECTORS ARE COMING",{1.f,.3f,.15f},2.f);
                if(director.phase==WaveDirector::Phase::APPROACH) director.startArena(7);
                else if(director.fighting()) damagePlayer(8.f,sc.a,.2f,.02f);
                break;
            }
        }
    }
    if(storyCollapseT>=0 && !storyBossDown) {
        float before=storyCollapseT;storyCollapseT+=dt;
        for(int k=0;k<(int)S.sortingTiles.size();++k) {
            float impact=2.2f+k*1.1f;
            if(before<impact&&storyCollapseT>=impact) {
                auto& w=level.walls[S.sortingTiles[k]];vec3 c=(w.box.min+w.box.max)*.5f;
                fx.spawnBurst(c,{1.f,.45f,.08f},ambientDetail()>0?24:8,8.f,1.2f,16.f);
                w.box.min.y-=8;w.box.max.y-=8;w.hidden=true;spatialGrid.build(level.walls);
                audio.playAt("explosion",c,120,SoundGroup::WORLD,true);shake(.7f,.08f);
            }
        }
    }
    if(at==8 && p.y<level.arenas[8].playerStart.y-5.f && !storyBossDown) {
        resetToCheckpoint(8);if(!g_godMode)styleSystem.takeDamage(18.f);
    }
    // The exit vista's sky warms as the gate opens; it is visible before the end card.
    if(storyBossDown) {
        auto& th=level.arenas.back().theme;
        th.zenith={.2f,.035f,.055f};th.horizon={.75f,.13f,.06f};th.ground={.16f,.09f,.1f};
        th.mountain={.13f,.05f,.07f};th.sunColor={2.f,.5f,.2f};th.sunSize=.075f;
        th.skyAmb={.38f,.24f,.22f};th.fogColor={.3f,.09f,.065f};th.fogDensity=.004f;
    }
}

inline void GameplayState::storySorterEvents(Enemy& e,const EnemyEvents& ev) {
    using glm::vec3;
    if(ev.telegraphStarted) {
        audio.playAt("telegraph",e.sortAim,120,SoundGroup::ENEMY,true);
        if(e.attack==AttackKind::SORT_STAMP)ui.feed(e.tellFollow?"PRESS FOLLOW-UP / KEEP MOVING":"OVERHEAD PRESS / WATCH THE LOCK",{1.f,.65f,.25f});
        else if(e.attack==AttackKind::SORT_GRAB)ui.feed("CLAW / WHITE: PARRY",{.95f,.95f,1.f});
        else if(e.attack==AttackKind::SORT_RUSH)ui.feed("GANTRY RUSH / DASH ACROSS ITS PATH",{1.f,.75f,.2f});
        else if(e.attack==AttackKind::SORT_ROW)ui.feed("PRESS SEQUENCE / CHANGE YOUR ROUTE",{1.f,.5f,.2f});
        else if(e.attack==AttackKind::SORT_PURGE)ui.feed("FURNACE PURGE / LEAVE THE GLOWING VENTS",{1.f,.5f,.2f});
    }
    if(ev.sortLocked)audio.playAt("count_tick",e.sortAim,120,SoundGroup::ENEMY,true);
    if(ev.sortDrop)audio.playAt("hook_yank",e.sortAim+vec3{0,5,0},120,SoundGroup::ENEMY,true);
    if(ev.sortPhaseStarted) {
        const char* names[]={"","INSPECTION","RECALIBRATION","INCINERATION"};
        if(ev.sortPhaseStarted==1)pushBanner(names[1],storytext::sorterIntro,{1.f,.5f,.2f},2.6f);
        else if(ev.sortPhaseStarted==2)ui.feed(storytext::sorterDamaged,{1.f,.5f,.2f});
        else ui.toast("FLOOR RELEASE","GET TO THE RAILS",{1.f,.35f,.15f},2.2f);
        audio.play("anvil_final",128,SoundGroup::WORLD,true);
        if(ev.sortPhaseStarted==3) storyCollapseT=0;
        const auto& a=level.arenas[8];
        for(auto& cv:level.conveyors) {
            const auto& b=level.walls[cv.wall].box;
            if(b.min.y>a.playerStart.y-1 && b.min.x>=64 && b.max.x<=100)
                cv.speed=ev.sortPhaseStarted==1?1.8f:ev.sortPhaseStarted==2?-3.f:4.2f;
        }
    }
    if(ev.sortRushStarted)audio.playAt("hook_yank",e.position,120,SoundGroup::ENEMY,true);
    if(ev.sortPurge)for(int i=0;i<e.sortVentCount;++i)audio.playAt("explosion",(e.sortVents[i].min+e.sortVents[i].max)*.5f,100,SoundGroup::WORLD,true);
    if(ev.sortMarked) {audio.play("dry",110,SoundGroup::UI,true);ui.feed("MARKED - PARRY OR BREAK SIGHT",{1.f,.3f,.1f});}
    if(ev.sortStamp) {
        breakStoryCover(ev.sortAt,ev.sortAt,Enemy::SORT_STAMP_R);
        fx.spawnShockwave(ev.sortAt,Enemy::SORT_STAMP_R,{1.f,.2f,.05f});
        fx.spawnBurst(ev.sortAt,{.8f,.5f,.2f},30,8,.7f,16);
        audio.playAt("slam",ev.sortAt,128,SoundGroup::ENEMY,true);shake(.7f,.08f);
    }
    if(ev.sortGrab && dashMomentumTimer<=0) {
        vec3 to={82.f,player.position.y,16.f};to-=player.position;to.y=0;
        if(glm::length(to)>.1f)player.velocity=glm::normalize(to)*18.f+vec3{0,7,0};
        audio.play("hook_yank",128,SoundGroup::WORLD,true);
    }
    if(ev.shots)audio.playAt("land_heavy",e.position,110,SoundGroup::ENEMY,true);
}

inline void GameplayState::breakStoryCover(glm::vec3 from,glm::vec3 to,float radius) {
    if(!story()||storyBossDown)return;
    bool shelter=false;for(const auto& f:ballastHazards.fronts)shelter|=!f.rubble;
    const auto broken=shatterStoryCover(level,from,to,radius,shelter);
    if(broken.empty())return;
    spatialGrid.build(level.walls);
    for(int id:broken) {
        const auto& cover=level.story.bossCover[id];const auto& wall=level.walls[cover.wall];
        glm::vec3 c=(cover.home.min+cover.home.max)*.5f,span=cover.home.max-cover.home.min;
        fx.spawnBurst(c,wall.color+glm::vec3{.15f},ambientDetail()>0?22:7,8,1,14);
        for(int k=0;k<(ambientDetail()==0?3:8);++k) {
            Effects::Debris piece;piece.shape=glm::mat3(.5f);piece.pos=c+glm::vec3{(k%4-1.5f)*span.x/4.f,(k/4-.5f)*span.y*.6f,0};
            piece.vel=glm::vec3{(k%3-1)*3.f,3.f+(k%2)*2.f,4.f};piece.axis={.4f,.7f,.2f};
            piece.color=wall.color;piece.emissive=glm::vec3{0};piece.life=piece.maxLife=3;piece.floorY=cover.home.min.y;piece.spin=2;piece.fade=1;
            fx.debris.push_back(piece);
        }
        audio.playAt("ab_boom_stone",c,115,SoundGroup::WORLD,true);shake(.3f,.05f);
    }
}

inline void GameplayState::gatherStoryCover(std::vector<BoxInstance>& out) {
    using namespace rig;Rig r{out};
    for(const auto& c:level.story.bossCover) {
        const auto& wall=level.walls[c.wall];if(wall.box.max.y<=c.home.min.y)continue;
        auto center=(wall.box.min+wall.box.max)*.5f;auto span=wall.box.max-wall.box.min;
        r.box(T(center),{0,0,0},span,wall.color);
        for(int k=0;k<4;++k) {
            float x=glm::mix(wall.box.min.x+.3f,wall.box.max.x-.3f,(k+.5f)/4.f);
            r.box(T({x,wall.box.max.y-.3f,wall.box.max.z+.04f})*RZ(.35f),{0,0,0},{.55f,.35f,.07f},{.8f,.57f,.12f});
        }
    }
}

inline bool GameplayState::shootStoryCable(glm::vec3 from,glm::vec3 dir,float& distance) {
    for(auto& e:enemies)if(e.alive&&e.type==EnemyType::BALLAST&&e.ballTether>0) {
        glm::vec3 a=e.position+glm::vec3{-1.5f,4,1},end=e.ballCableAt;
        if(glm::length(end-from)<.6f&&glm::length(a-end)>.8f)end+=glm::normalize(a-end)*.8f;
        glm::vec3 v=end-a,w=from-a;
        float c=glm::dot(v,v),b=glm::dot(dir,v),d=glm::dot(dir,w),f=glm::dot(v,w),den=c-b*b;
        float t=c>.001f?glm::clamp(den>.001f?(f-b*d)/den:f/c,0.f,1.f):0.f;
        float ray=glm::clamp(glm::dot(a+v*t-from,dir),0.f,distance);
        if(c>.001f)t=glm::clamp(glm::dot(from+dir*ray-a,v)/c,0.f,1.f);
        if(ray<.15f||ray>=distance||glm::length(from+dir*ray-a-v*t)>.24f)continue;
        e.ballTether=0;e.ballCableShow=.2f;e.ballOpen=1.f;distance=ray;
        fx.spawnHitSparks(from+dir*ray,{1,.8f,.4f});audio.playAt("clank",from+dir*ray,100,SoundGroup::WORLD);
        ui.feed("CABLE CUT",{.5f,1,.8f});return true;
    }
    return false;
}

inline void GameplayState::gatherStoryFactory(std::vector<BoxInstance>& out) {
    using namespace rig;using namespace enemyfile;using glm::vec3;
    if(wasteland()) {gatherWasteland(out);return;}
    gatherStoryCover(out);
    const auto& S=level.story;const auto* unit=story::bodyModel();
    if(unit) {
        auto drawUnit=[&](vec3 p,float yaw,bool awake) {
            if(glm::length(p-player.camera.position)>80)return;
            FilePose P;P.chan[C_EYE]=awake?vec3{.2f,.8f,1}:vec3{0};P.chan[C_CORE]=P.chan[C_EYE]*.4f;
            P.motion[E_HEAD]=RY(std::sin(gameClock*.6f+p.x)*.18f);
            P.motion[E_ARML]=RZ(.12f);P.motion[E_ARMR]=RZ(-.12f);
            Rig r{out};drawFile(r,T(p)*RY(yaw),*unit,P,{.15f,.8f,1});
        };
        for(auto p:S.assemblyUnits)drawUnit(p,3.14159f,true);
        if(!storyPodsBroken)for(auto p:S.pods)drawUnit(p,0,false);
    }
    gatherStoryAtmosphere(out);
    Rig r{out};
    // Cyan floating lamps are a consistent visual vocabulary for optional
    // records. Bloom comes from the emissive mesh, without a light per prop.
    auto beacon=[&](vec3 p, float phase, bool read) {
        if(glm::length(p-player.camera.position)>55.f)return;
        p.y += .1f*std::sin(gameClock*2.f+phase);
        float pulse=.8f+.2f*std::sin(gameClock*2.4f+phase);
        vec3 glow=(read?vec3{.15f,.45f,.55f}:vec3{.25f,1.7f,2.f})*pulse;
        auto xf=T(p)*RY(gameClock*.65f+phase);
        r.box(xf,{0,0,0},{.13f,.22f,.13f},{.7f,.95f,1},glow);
        for(float side:{-1.f,1.f}) {
            r.box(xf,{side*.22f,0,0},{.035f,.3f,.035f},{.3f,.7f,.8f},glow*.55f);
            r.box(xf,{side*.17f,-.15f,0},{.13f,.035f,.035f},{.3f,.7f,.8f},glow*.55f);
        }
    };
    for(int i=0;i<(int)S.wrecks.size();++i)
        beacon(S.wrecks[i].pos+vec3{0,1.95f,0},i*.8f,(storyLogsSeen>>i)&1u);
    for(int i=0;i<(int)S.terminals.size();++i)
        beacon(S.terminals[i].pos+vec3{0,2.65f,.25f},i*.6f,storyTerminal==i);
    // The service deck's stripes and suspension travel with its real wall.
    for(int id:{S.serviceLift,S.elevator})if(id>=0) {
        auto b=level.walls[level.movers[id].wall].box;
        float y=b.max.y+.025f;
        for(float x:{b.min.x+.12f,b.max.x-.12f})
            r.box(T({x,y,(b.min.z+b.max.z)*.5f}),{0,0,0},{.1f,.035f,b.max.z-b.min.z-.2f},{.2f,.7f,.8f},{.1f,.7f,.9f});
        for(int k=0;k<6;++k) {
            float x=glm::mix(b.min.x+.25f,b.max.x-.25f,k/5.f);
            for(float z:{b.min.z+.14f,b.max.z-.14f})
                r.box(T({x,y,z}),{0,0,0},{.25f,.04f,.2f},{.85f,.65f,.1f});
        }
    }
    for(int w:S.podGlass) {
        auto b=level.walls[w].box;if(b.max.y< -100)continue;
        vec3 c=(b.min+b.max)*.5f;
        // Frosted edges and fine glass ribs leave the sleeping silhouettes visible.
        for(float sx:{b.min.x,b.max.x})r.box(T({sx,c.y,c.z}),{0,0,0},{.06f,b.max.y-b.min.y,.1f},{.25f,.65f,.7f},{.02f,.09f,.1f});
        for(int k=0;k<6;++k)r.box(T({c.x,b.min.y+k*.7f,c.z}),{0,0,0},{b.max.x-b.min.x,.025f,.04f},{.18f,.45f,.48f});
    }
    for(int w:S.sortingTiles) {auto b=level.walls[w].box;if(b.max.y<level.arenas[8].playerStart.y-1)continue;
        r.box(T((b.min+b.max)*.5f),{0,0,0},b.max-b.min,{.14f,.16f,.18f});
    }
    for(const auto& sc:S.scanners) {
        float y=sc.a.y+1.5f+1.25f*std::sin(gameClock*1.7f+sc.phase);
        r.box(T({sc.a.x,y,8}),{0,0,0},{.08f,.07f,14},{1,.15f,.05f},{2,.2f,.05f});
    }
    if(!storyGrapple) {
        auto p=S.grappleAt+vec3{0,1.4f+.12f*std::sin(gameClock*3),0};
        r.box(T(p)*RY(gameClock),{0,0,0},{.55f,.2f,.25f},{.7f,.75f,.8f},{.1f,.7f,1});
        for(float s:{-1.f,1.f})r.box(T(p)*RY(gameClock),{s*.27f,0,.16f},{.1f,.18f,.3f},{.65f,.7f,.72f});
    }
    auto crane=[&](vec3 p) {
        float top=level.arenas[8].playerStart.y+15;
        r.box(T({82,top,p.z}),{0,0,0},{40,.65f,1.8f},{.48f,.36f,.1f});
        r.box(T({p.x,top-.7f,p.z}),{0,0,0},{6,1,2.4f},{.3f,.33f,.35f});
        for(float s:{-1.f,1.f}) r.box(T({p.x+s*2.3f,(p.y+6.5f+top-1.f)*.5f,p.z}),{0,0,0},{.13f,top-1.f-p.y-6.5f,.13f},{.6f,.63f,.65f});
    };
    // Through the inspection doorway, the Sorter is already working. The real
    // targetable enemy replaces this preview when the fight starts.
    bool live=false;for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::SORTER)live=true;
    if(!live&&!storyBossDown && glm::length(player.position-level.arenas[8].bossSpawn)<95) {
        Enemy e{EnemyType::SORTER,level.arenas[8].bossSpawn};e.spawnTimer=0;e.yaw=1.57f;
        e.yaw=std::atan2(player.position.x-e.position.x,player.position.z-e.position.z);e.sortOpen=.5f*(1+std::sin(gameClock));buildEnemy(e,gameClock,out);crane(e.position);
    }
    for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::SORTER) {
        crane(e.position);
        const float top=level.arenas[8].playerStart.y+15.f;
        if(e.telegraphTimer>0 && (e.attack==AttackKind::SORT_GRAB||e.attack==AttackKind::SORT_STAMP)) {
            bool stamp=e.attack==AttackKind::SORT_STAMP;
            vec3 col=stamp?(e.sortLocked?vec3{2.f,.13f,.04f}:vec3{1.5f,.75f,.1f}):vec3{1.7f};
            sorterrig::warning(r,e.sortAim,stamp?Enemy::SORT_STAMP_R:Enemy::SORT_GRAB_R,col,stamp);
            if(stamp) {
                float raised=glm::clamp(top-e.sortAim.y-2.f,1.f,Enemy::SORT_PRESS_UP);
                sorterrig::press(r,e.sortAim,top,Enemy::sortPressHeight(e.telegraphTimer,-1.f,raised),col);
            } else sorterrig::claw(r,e,e.sortAim,glm::clamp(1.f-e.telegraphTimer/.25f,0.f,1.f));
        }
        if(e.sortImpactT>=0) {
            if(e.sortImpactKind==AttackKind::SORT_STAMP) {
                float raised=glm::clamp(top-e.sortImpactAt.y-2.f,1.f,Enemy::SORT_PRESS_UP);
                sorterrig::press(r,e.sortImpactAt,top,Enemy::sortPressHeight(0,e.sortImpactT,raised),{.5f,.2f,.05f});
            } else sorterrig::claw(r,e,e.sortImpactAt,1.f-e.sortImpactT/Enemy::SORT_IMPACT);
        }
        for(const auto& press:e.sortPresses) {
            vec3 col=press.remaining>Enemy::SORT_LOCK?vec3{1.5f,.75f,.1f}:vec3{2,.12f,.04f};
            if(press.remaining>0)sorterrig::warning(r,press.at,Enemy::SORT_STAMP_R,col,true);
            float raised=glm::clamp(top-press.at.y-2.f,1.f,Enemy::SORT_PRESS_UP);
            sorterrig::press(r,press.at,top,Enemy::sortPressHeight(press.remaining,press.impact,raised),col);
        }
        if(e.sortRush>0||e.attack==AttackKind::SORT_RUSH) {
            vec3 from=e.sortRush>0?e.sortRushFrom:e.position,to=e.sortRushTo;
            from.y=to.y=level.arenas[8].playerStart.y+.08f;
            vec3 dir=to-from;dir.y=0;if(glm::length(dir)>.01f) {
                dir=glm::normalize(dir);vec3 side{-dir.z,0,dir.x};
                for(float sign:{-1.f,1.f})sorterrig::beam(r,from+side*3.4f*sign,to+side*3.4f*sign,.1f,{.8f,.5f,.1f},abGoldTell({1.7f,1.1f,.2f}));
                sorterrig::claw(r,e,e.position+dir*3.f-glm::vec3{0,2,0},.25f);
            }
        }
        if(e.attack==AttackKind::SORT_PURGE||e.sortFire>0)for(int i=0;i<e.sortVentCount;++i) {
            const auto& box=e.sortVents[i];vec3 center=(box.min+box.max)*.5f;center.y=box.min.y+.06f;
            bool active=e.sortFire>0;
            r.box(T(center),{0,0,0},{box.max.x-box.min.x,.09f,box.max.z-box.min.z},{.4f,.18f,.03f},{active?2.f:1.f,.35f,.03f});
            for(int j=0;j<12;++j) {
                float x=glm::mix(box.min.x+.6f,box.max.x-.6f,(j+.5f)/12.f);
                r.box(T({x,box.min.y+.12f,center.z}),{0,0,0},{.18f,.12f,3.2f},{.25f,.23f,.2f});
                if(active) {
                    float h=1.7f+.6f*std::sin(gameClock*13+j);
                    r.part(T({x,box.min.y+h*.5f,center.z})*RZ(.12f*std::sin(gameClock*7+j))*rig::S({.75f,h,2.8f}),{1.f,.35f,.04f},{2.f,.6f,.05f},sorterrig::flameMesh());
                    r.part(T({x,box.min.y+.7f,center.z})*rig::S({.55f,1.4f,2.1f}),{1.f,.7f,.1f},{2.f,1.f,.15f},sorterrig::flameMesh());
                }
            }
        }
        if(e.sortScan>0) {
            vec3 from=e.position+vec3{0,3,0};float a=e.sortScanYaw+(1.6f-e.sortScan)*(e.sortPhase==3?1.1f:.9f);
            vec3 to=from+glm::normalize(vec3{std::sin(a),e.sortScanSlope,std::cos(a)})*55.f;
            vec3 d=to-from;float len=glm::length(d);vec3 dir=d/len;
            for(const auto& w:level.walls){float hit=rayBoxHit(from,dir,w.box);if(hit>.05f)len=std::min(len,hit);}
            d=dir*len;
            vec3 side=glm::normalize(glm::cross(glm::normalize(d),vec3{0,1,0}));
            // A row of thin cubes follows the same ray as the collision test.
            for(float t=0;t<len;t+=.16f)r.box(T(from+d*(t/len)),{0,0,0},{.12f,.12f,.12f},{1,.3f,.05f},{2,.4f,.08f});
            (void)side;
        }
    }
    if(storyCollapseT>=0)for(int k=0;k<(int)S.sortingTiles.size();++k) {
        float remaining=2.2f+k*1.1f-storyCollapseT;
        if(remaining<-.5f)continue;
        auto b=storyTileBoxes[k];vec3 c=(b.min+b.max)*.5f;c.y=b.max.y;
        if(remaining>0)r.box(T(c+vec3{0,.04f,0}),{0,0,0},{b.max.x-b.min.x,.04f,b.max.z-b.min.z},{.65f,.18f,.04f},vec3{1.5f,.25f,.05f}*(.5f+.5f*std::sin(gameClock*8)));
        sorterrig::press(r,c,level.arenas[8].playerStart.y+15.f,Enemy::sortPressHeight(std::max(0.f,remaining),remaining<=0?-remaining:-1.f),{1.4f,.35f,.05f});
    }
}

inline void GameplayState::gatherStoryProjectile(std::vector<BoxInstance>& out,const Projectile& shot,glm::vec3 at,float spin) {
    using namespace rig;using namespace enemyfile;
    if(shot.ownerType==(int)EnemyType::BALLAST) {
        Rig r{out};auto root=T(at)*RY(spin*.3f)*RX(spin);
        r.box(root,{0,0,0},{1.05f,.65f,.9f},{.3f,.27f,.21f},shot.parried?glm::vec3{.9f,.65f,.25f}:glm::vec3{.13f,.08f,.03f});
        r.box(root,{0,.34f,0},{.95f,.06f,.75f},{.67f,.56f,.3f});return;
    }
    glm::mat4 root=T(at)*RY(spin*.3f)*RX(spin)*S(glm::vec3{shot.parried?1.25f:1.f})*T({0,-.9f,0});
    if(const auto* m=story::bodyModel()) {
        FilePose pose=story::sittingPose(.6f);pose.chan[C_EYE]=glm::vec3{0};pose.chan[C_CORE]=shot.parried?glm::vec3{1,.7f,.1f}:glm::vec3{0};
        Rig r{out};drawFile(r,root,*m,pose,{.1f,.1f,.12f});
    } else {Rig r{out};r.box(root,{0,1.2f,0},{.35f,.55f,.25f},{.7f,.7f,.65f});r.box(root,{0,1.7f,0},{.24f,.24f,.24f},{.7f,.7f,.65f});}
}
