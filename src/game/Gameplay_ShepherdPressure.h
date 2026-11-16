#pragma once
// Spatial attacks: lane control, a catch with several player-driven escapes,
// and a locked stare that leaves short-lived routes of burning hull.
inline void GameplayState::shepReleaseGrab(bool counter) {
    if(shepGrabT<0.f)return;
    shepGrabT=-1.f;shepGrabCaught=false;shepGrabHp=0.f;
    auto away=player.position-shephead::chest(shepHeadRise);away.y=0;
    if(glm::length(away)>.01f)player.velocity+=glm::normalize(away)*9.f;
    player.velocity.y=std::max(6.f,player.velocity.y);player.launched();
    shepAtk=shephead::NONE;shepIdleT=counter?1.f:.55f;
    if(counter){shepOpenT=3.5f;shepHeadHurt(110.f);styleSystem.heal(10.f*tune().heal);styleSystem.addStyle(150.f,StyleSource::PARRY);
        ui.feed("THE GRIP BREAKS",HOLDFAST_STEEL);fx.spawnShockwave(player.position,14.f,HOLDFAST_STEEL);shepherdSound("shep_pin",player.position);}
}
inline bool GameplayState::shepBreakGrip(glm::vec3 from,glm::vec3 dir,float& distance,float damage) {
    if(shepGrabT<0.f)return false;
    const auto hand=shepHandPose(shepSlamArm);
    float hit=shephead::hitBody(from,dir,{hand-glm::vec3{12,8,16},hand+glm::vec3{12,9,16}});
    if(hit<=0.f||hit>=distance)return false;
    distance=hit;shepGrabHp-=damage;fx.spawnHitSparks(from+dir*hit,HOLDFAST_STEEL);
    if(shepGrabHp<=0.f)shepReleaseGrab(true);
    return true;
}
inline void GameplayState::shepPressure(float dt,float before) {
    using namespace shephead;
    const auto p=player.position;
    auto finish=[&](){shepAtk=NONE;shepIdleT=shepHeadPhase==0?.6f:.38f;};
    if(shepAtk==SWEEP){
        const int passes=shepVariation==1?2:1;
        for(int pass=0;pass<passes;++pass){
            float start=SWEEP_TELL+pass*(SWEEP_TRAVEL+.35f),end=start+SWEEP_TRAVEL;
            if(pass&&before<start-.35f&&shepAtkT>=start-.35f)shepherdSound("shep_sweep_tell",{player.position.x,shep::FALL_Y+6.f,shepAttackAt.z},.8f);
            if(shepAtkT<start||before>end)continue;
            float z=pass?shepAttackAt.z:shepSweepZ;int side=pass?1-shepSlamArm:shepSlamArm;
            float a=sweepX(std::max(before,start)-pass*(SWEEP_TRAVEL+.35f),side,shep::FALL_X0-14.f,shep::FALL_X1+14.f);
            float b=sweepX(std::min(shepAtkT,end)-pass*(SWEEP_TRAVEL+.35f),side,shep::FALL_X0-14.f,shep::FALL_X1+14.f);
            if(!(shepAttackHits&(1<<pass))&&sweepHits(p,player.height,a,b,z,shep::FALL_Y)){
                shepAttackHits|=1<<pass;
                if(damagePlayer(30.f,glm::vec3{b,shep::FALL_Y+6,z},.35f,.065f,.4f,{EnemyType::COUNT,DamageKind::MELEE})){
                    player.velocity+=glm::vec3{side?-15.f:15.f,5.f,0};player.launched();
                }
            }
            if(before<start&&shepAtkT>=start){shepherdSound("shep_sweep",{b,shep::FALL_Y+6.f,z},.8f);shake(.3f,.035f);}
        }
        if(shepAtkT>SWEEP_TELL+passes*SWEEP_TRAVEL+(passes-1)*.35f+.3f)finish();
    }else if(shepAtk==GRAB){
        if(shepAtkT<GRAB_LOCK){shepAttackAt=p;shepAttackAt.y=level.groundAt(p.x,p.z,p.y);}
        if(before<GRAB_LOCK&&shepAtkT>=GRAB_LOCK)shepherdSound("shep_lock",shepAttackAt);
        if(before<GRAB_CLOSE-.22f&&shepAtkT>=GRAB_CLOSE-.22f)shepherdSound("shep_parry",shepAttackAt);
        if(before<GRAB_CLOSE&&shepAtkT>=GRAB_CLOSE){
            if(!(shepAttackHits&1)&&grabHits(p,player.height,shepAttackAt)){
                shepGrabT=0.f;shepGrabHp=GRAB_BREAK;shepGrabCaught=true;
                shepGrabOffset=p-(shepAttackAt+glm::vec3{0,3,0});
                grapple.release();
                for(auto& shot:projSystem.pool)if(shot.owner==SHOT_OWNER&&!shot.isPlayer)shot.alive=false;
                ui.feed("CAUGHT: BREAK THE HAND OR ANCHOR THE HULL",HOLDFAST_STEEL);
                shepherdSound("shep_grip",shepAttackAt);shake(.3f,.04f);
            }else{
                shepHandAt[shepSlamArm]=shepAttackAt;shepArmDown[shepSlamArm]=1.3f;
                ui.feed("THE HAND MISSES. PUNISH IT",HOLDFAST_STEEL);
            }
        }
        if(shepGrabT>=0.f){
            shepGrabT+=dt;
            // Inputs and weapons remain live. A spike, damage, or a strong dash
            // out of the grip can free you; the camera is never forced to turn.
            auto hand=glm::mix(shepAttackAt+glm::vec3{0,3,0},
                glm::vec3(shepBodyPose()*glm::vec4(chest(shepHeadRise)+glm::vec3{shepSlamArm?42.f:-42.f,20,88},1)),
                smooth((shepAtkT-GRAB_CLOSE)/1.3f));
            if(holdfast.held()||glm::length(player.position-(hand+shepGrabOffset))>18.f){shepReleaseGrab(true);return;}
            const auto delta=(hand+shepGrabOffset-player.position)*(1.f-std::exp(-dt*8.f));
            // Don't carry the player through scenery: stop at the first solid.
            const float length=glm::length(delta);float wall=length>.001f?wallAlong(player.position+glm::vec3{0,1,0},delta/length,length+.4f):-1.f;
            if(wall>0.f&&wall<length+.35f){shepReleaseGrab(true);return;}
            player.position+=delta;player.velocity*=std::exp(-dt*3.f);player.onGround=false;player.launched();
            if(shepGrabT>=GRAB_ESCAPE){damagePlayer(40.f,hand,.45f,.07f,.6f,{EnemyType::COUNT,DamageKind::MELEE});shepReleaseGrab(false);return;}
        }else if(shepAtkT>GRAB_CLOSE+.8f)finish();
    }else if(shepAtk==GAZE){
        if(shepAtkT<GAZE_TRACK){shepAttackAt=p;shepAttackAt.y=level.groundAt(p.x,p.z,p.y);}
        if(before<GAZE_TRACK&&shepAtkT>=GAZE_TRACK)shepherdSound("shep_lock",shepAttackAt);
        const int count=shepVariation==1?3:1;
        for(int k=0;k<count;++k){const float fire=GAZE_FIRE+k*.6f;
            if(before<fire&&shepAtkT>=fire){
                const auto at=shepGazeMark(k);
                for(int eye=0;eye<2;++eye){auto origin=shepEyePose(eye);auto hit=at+glm::vec3{eye?1.6f:-1.6f,0,0};
                    shepScars.push_back({{origin.x,shep::FALL_Y+.08f,shep::FALL_ZN+1.f},hit+glm::vec3{0,.08f,0},3.8f});
                    fx.spawnTracer(origin,hit,.28f,.35f,SWEEP_RED*2.f);
                }
                if(glm::length(glm::vec2(p.x-at.x,p.z-at.z))<4.8f&&p.y<at.y+14.f)
                    damagePlayer(27.f,at,.25f,.045f,.4f,{EnemyType::COUNT,DamageKind::BEAM});
                fx.spawnBurst(at,SWEEP_RED,30,10.f,.6f,2.f);shepherdSound("shep_sweep",at,.7f);shake(.15f,.025f);
            }
        }
        if(shepAtkT>GAZE_END+(count-1)*.6f)finish();
    }
}
