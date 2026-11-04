#pragma once
inline void GameplayState::ballastEvents(Enemy& e,const EnemyEvents& ev) {
    if(ev.ballPhaseStarted) {
        const char* line=ev.ballPhaseStarted==1?storytext::ballastIntro:ev.ballPhaseStarted==2?storytext::ballastDamaged:storytext::ballastFinal;
        if(ev.ballPhaseStarted==1)pushBanner("BALLAST / CLEARANCE UNIT",line,{.95f,.7f,.35f},3.f);
        else ui.feed(line,{.95f,.7f,.35f});
    }
    if(ev.telegraphStarted) {
        audio.playAt("telegraph",(e.attack==AttackKind::BALL_ANCHOR||e.attack==AttackKind::BALL_WINCH)?e.ballAim:e.position,120,SoundGroup::ENEMY,true);
        const char* hint=e.attack==AttackKind::BALL_GRADE?"GRADING PASS / JUMP OR DASH SIDEWAYS":
                         e.attack==AttackKind::BALL_GUST?(e.ballReverse?"REVERSE DRAW / LEAVE THE LANE OR TAKE COVER":"CROSSWIND / LEAVE THE LANE OR TAKE COVER"):
                         e.attack==AttackKind::BALL_ANCHOR?"CABLE WEIGHT / WHITE: PARRY":
                         e.attack==AttackKind::BALL_WINCH?"WINCH / PARRY THE HEAD OR CUT THE CABLE":
                         e.attack==AttackKind::BALL_RIP?"ROAD TEAR / JUMP OR CROSS THE GAPS":
                         e.attack==AttackKind::BALL_SWEEP?"BLADE SWEEP / JUMP":"SCOOP VOLLEY / PARRY THE HEAVY SCRAP";
        ui.feed(hint,{1.f,.8f,.45f});
    }
    if(ev.ballLocked)audio.playAt("count_tick",e.ballAim,115,SoundGroup::ENEMY,true);
    if(ev.ballRip){ballastHazards.addRip(ev.ballFrom,ev.ballTo);audio.playAt("ab_boom_stone",e.position,120,SoundGroup::ENEMY,true);shake(.5f,.06f);}
    if(ev.ballWinch)audio.playAt("hook_yank",e.position,120,SoundGroup::ENEMY,true);
    if(ev.ballSweep)audio.playAt("slam",e.position,120,SoundGroup::ENEMY,true);
    if(ev.ballYank&&!playerDead&&!g_godMode&&dashMomentumTimer<=0) {
        glm::vec3 pull=e.position-player.position;pull.y=0;
        if(glm::length(pull)>1)player.velocity+=glm::normalize(pull)*(18.f/60.f);
    }
    if(ev.ballGust)ballastHazards.add(ev.ballFrom,ev.ballTo);
    if(ev.ballGrade)audio.playAt("land_heavy",e.position,120,SoundGroup::ENEMY,true);
    if(ev.ballAnchor) {
        fx.spawnShockwave(ev.ballTo,Enemy::BALL_ANCHOR_R,{.8f,.65f,.4f});
        audio.playAt("slam",ev.ballTo,128,SoundGroup::ENEMY,true);shake(.4f,.04f);
    }
}

inline void GameplayState::gatherBallast(std::vector<BoxInstance>& out) {
    using namespace rig;using glm::vec3;
    Rig r{out};
    auto line=[&](vec3 a,vec3 b,float width,vec3 color){
        vec3 d=b-a;float length=glm::length(d);if(length<.01f)return;
        vec3 z=d/length,x=glm::cross(vec3{0,1,0},z);
        if(glm::length(x)<.01f)x={1,0,0};else x=glm::normalize(x);
        vec3 y=glm::normalize(glm::cross(z,x));
        glm::mat4 m(1);m[0]=glm::vec4(x*width,0);m[1]=glm::vec4(y*width,0);m[2]=glm::vec4(z*length,0);m[3]=glm::vec4((a+b)*.5f,1);
        r.part(m,color*.5f,color);
    };
    auto lane=[&](vec3 from,vec3 to,float half,vec3 color){
        vec3 d=to-from;d.y=0;float n=glm::length(d);if(n<.01f)return;
        vec3 dir=d/n,side{-dir.z,0,dir.x};from.y+=.08f;to.y=from.y;
        for(float sign:{-1.f,1.f})line(from+side*half*sign,to+side*half*sign,.08f,color);
        for(float distance=2.f;distance<n;distance+=4.f) {
            vec3 c=from+dir*distance;
            line(c-dir*1.1f+side*.75f,c,.08f,color);
            line(c-dir*1.1f-side*.75f,c,.08f,color);
        }
    };
    for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::BALLAST&&(e.telegraphTimer>0||e.ballDrive>0||e.ballCableShow>0||e.ballSwing>0)) {
        if(e.attack==AttackKind::BALL_GRADE||e.ballDrive>0)lane(e.ballFrom,e.ballAim,3.7f,abGoldTell({1.7f,1.1f,.15f}));
        if(e.attack==AttackKind::BALL_WINCH||e.ballCableShow>0) {
            vec3 muzzle=e.position+vec3{-1.5f,4,1};
            float reach=e.attack==AttackKind::BALL_WINCH?glm::clamp(1.f-e.telegraphTimer/.22f,0.f,1.f):1.f;
            vec3 head=glm::mix(muzzle,e.attack==AttackKind::BALL_WINCH?e.ballAim:e.ballCableAt,reach);
            line(muzzle,head,.075f,vec3{1.5f});
            r.box(T(head)*RY(e.yaw),{0,0,0},{.45f,.45f,1.1f},{.6f,.59f,.5f},vec3{1.7f});
            if(e.telegraphTimer>0)sorterrig::warning(r,e.ballAim-vec3{0,.6f,0},1.1f,vec3{1.5f},false);
        }
        if(e.attack==AttackKind::BALL_RIP) {
            vec3 d=e.ballAim-e.position;d.y=0;
            if(glm::length(d)>.01f){d=glm::normalize(d);for(float a:{-.25f,0.f,.25f}) {
                vec3 dir{d.x*std::cos(a)+d.z*std::sin(a),0,d.z*std::cos(a)-d.x*std::sin(a)};
                lane(e.position,e.position+dir*55.f,1.8f,{1.7f,.45f,.1f});
            }}
        }
        if(e.attack==AttackKind::BALL_SWEEP) {
            float facing=std::atan2(e.ballSwingDir.x,e.ballSwingDir.z);
            for(int k=0;k<24;++k){float a=facing-1.05f+k*2.1f/24.f,b=facing-1.05f+(k+1)*2.1f/24.f;
                line(e.position+vec3{std::sin(a)*6.2f,.08f,std::cos(a)*6.2f},e.position+vec3{std::sin(b)*6.2f,.08f,std::cos(b)*6.2f},.08f,abGoldTell({1.8f,1.1f,.2f}));}
        }
        if(e.attack==AttackKind::BALL_GUST) {
            vec3 d=e.ballAim-e.ballFrom;d.y=0;
            if(glm::length(d)>.01f)lane(e.ballFrom,e.ballFrom+glm::normalize(d)*BallastHazards::RANGE,BallastHazards::HALF_WIDTH,{1.7f,.35f,.13f});
        }
        if(e.attack==AttackKind::BALL_ANCHOR) {
            vec3 at=e.ballAim+vec3{0,.08f,0};
            for(int k=0;k<24;++k) {
                float a=k*6.2831853f/24.f,b=(k+1)*6.2831853f/24.f;
                line(at+vec3{std::cos(a)*Enemy::BALL_ANCHOR_R,0,std::sin(a)*Enemy::BALL_ANCHOR_R},
                     at+vec3{std::cos(b)*Enemy::BALL_ANCHOR_R,0,std::sin(b)*Enemy::BALL_ANCHOR_R},.08f,vec3{1.8f});
            }
            vec3 weight=at+vec3{0,.8f+4.f*(1.f-e.telegraphProgress()),0};
            line(e.position+vec3{-3,4,0},weight,.06f,{.5f,.5f,.45f});
            r.box(T(weight)*RY(e.age),{0,0,0},{1.1f,1.1f,1.1f},{.35f,.31f,.24f},vec3{1.5f});
        }
    }
    for(const auto& f:ballastHazards.fronts) {
        vec3 center=f.from+f.dir*f.distance,side{-f.dir.z,0,f.dir.x};
        if(f.rubble) {
            for(int k=0;k<4;++k){vec3 p=center+side*((k-1.5f)*.8f)+vec3{0,.65f+(k%2)*.3f,0};
                r.box(T(p)*RY(std::atan2(f.dir.x,f.dir.z))*RX(-.35f+std::sin(gameClock*12+k)*.12f),{0,0,0},{.85f,1.3f,1.7f},{.32f,.29f,.24f},{.5f,.2f,.04f});}
            line(center-side*1.8f,center+side*1.8f,.1f,{1.4f,.45f,.1f});continue;
        }
        // The clear front and shoulder lamps remain at every detail level.
        line(center-side*BallastHazards::HALF_WIDTH+vec3{0,.3f,0},center+side*BallastHazards::HALF_WIDTH+vec3{0,.3f,0},.14f,{1.5f,.4f,.14f});
        for(float sign:{-1.f,1.f})line(center+side*BallastHazards::HALF_WIDTH*sign,center+side*BallastHazards::HALF_WIDTH*sign+vec3{0,2.7f,0},.1f,{1.5f,.4f,.14f});
        if(ambientDetail()>0)for(int k=0;k<ambientDetail()*5;++k) {
            float across=(k+.5f)/(ambientDetail()*5.f)*2-1;
            vec3 p=center+side*(across*BallastHazards::HALF_WIDTH)+vec3{0,.5f+std::fmod(k*.73f+gameClock,2.f),0};
            r.box(T(p)*RY(gameClock+k),{0,0,0},{.12f,.06f,.18f},{.42f,.35f,.26f},{.2f,.12f,.04f});
        }
    }
}
