#pragma once
#include "render/HeadMesh.h"
#include "bosses/shepherd/ShepherdModel.h"
#include "bosses/shepherd/ShepherdCombat.h"
#include "game/Gameplay_ShepherdMotion.h"
#include "game/Gameplay_ShepherdPressure.h"
#include "game/Gameplay_ShepherdCourt.h"
// THE COUNTERWEIGHT: the Shepherd's human form rises out of the machine moon.
// Always vulnerable. White fists can be parried; grounded palms take HOLDFAST.
// Pinning a palm turns its captive orbit into a counterweight against its heart.
inline float GameplayState::shepHeadThird() const { return shepHeadHp / shephead::HP; }
inline void GameplayState::shepHeadReset() {
    using namespace shephead;
    stopShepherdAudio();
    if (!shepHeadArmed) { shepHeadArmed=true; shepHeadHp=HP; shepHeadSeen=false; }
    else shepHeadHp=checkpointHp(shepHeadHp);
    shepAtk=NONE; shepAtkT=0.f; shepIdleT=1.2f; shepOpenT=0.f; shepRearT=-1.f; shepCycle=0;
    shepArmDown[0]=shepArmDown[1]=shepPinT[0]=shepPinT[1]=0.f;
    shepSlamParried[0]=shepSlamParried[1]=false;
    shepHitHealT=0.f;shepCounterT=0.f; shepEchoN=shepEchoHit=0;
    shepHitSoundT=0.f;
    shepCounterHit[0]=shepCounterHit[1]=true;shepCourtT=99.f;
    shepBeamOn=false; shepHeadRise=shepHeadSeen?1.f:0.f; shepRiseT=shepHeadSeen?-1.f:0.f;
    shepHeadPhase=shepHeadHp>HP*2.f/3.f?0:shepHeadHp>HP/3.f?1:2;
    if(director.arena==10)audio.music.resetShepherdMovement(shepHeadHp<=0.f?shepherdscore::LAST_COUNT:shepherdscore::CONTROL+shepHeadPhase);
    for(int k=0;k<2;++k)shepReleaseT[k]=k<shepHeadPhase?12.f:-1.f;
    shepGazeX=-600.f; shepGape=0.f;
    shepPoseReady=false;shepPoseAttack=NONE;shepBodyAngles=shepBodyOffset=shepSpine=glm::vec3{0.f};
    shepMotionClock=shepBreath=0;shepLookAt=player.camera.position;
    shepGrabT=-1;shepGrabHp=0;shepGrabCaught=false;shepAttackHits=0;shepScars.clear();shepGazeTick=0;
    for(int k=0;k<2;++k){shepPosePlanted[k]=false;shepRecoverT[k]=2.f;shepFingerCurl[k]=.3f;shepWristTilt[k]=-.3f;}
}
inline bool GameplayState::shepFinCovers(glm::vec3 feet) const {
    const glm::vec3 o=feet+glm::vec3{0.f,1.f,0.f};
    for (const auto& s:sweepShelters) if(s.alive && s.section==10) {
        const float t=rayBoxHit(o,{0.f,0.f,-1.f},s.box);
        if(t>0.f && t<16.f)return true;
    }
    return false;
}
inline void GameplayState::shepHeadHurt(float dmg) {
    using namespace shephead;
    if(shepHeadHp<=0.f || shepRiseT>=0.f || shepRearT>=0.f || dmg<=0.f)return;
    // No single shotgun burst/rocket skips a new movement of the fight.
    const float gate=shepHeadPhase==0?HP*2.f/3.f:shepHeadPhase==1?HP/3.f:0.f;
    shepHeadHp=std::max(gate,shepHeadHp-dmg); shepherdDamageT=.3f;
    if(shepHeadHp<=gate && gate>0.f) {
        stopShepherdAudio();
        shepReleaseGrab(false);shepScars.clear();
        ++shepHeadPhase; shepRearT=0.f; shepCycle=0; sweep.reset();
        audio.music.setShepherdMovement(shepherdscore::CONTROL+shepHeadPhase);
        shepCourtT=0.f;shepCounterHit[0]=shepCounterHit[1]=true;
        shepReleaseT[shepHeadPhase-1]=0.f;
        shepherdSound("shep_break",shep::courtClamp(shepHeadPhase-1));
        shepAtk=NONE; shepBeamOn=false; shepOpenT=0.f; shepEchoN=shepEchoHit=0;
        wakeRings.clear();
        for(auto& p:projSystem.pool)if(p.owner==SHOT_OWNER)p.alive=false;
    }
    if(shepHeadHp<=0.f) {
        stopShepherdAudio();
        shepReleaseGrab(false);shepScars.clear();
        shepCounterHit[0]=shepCounterHit[1]=true;
        sweep.reset(); shepAtk=NONE; shepBeamOn=false; wakeRings.clear();
        for(auto& p:projSystem.pool)if(p.owner==SHOT_OWNER)p.alive=false;
        pushBanner("THE ORBIT BREAKS","PIN ITS HEART. END THE COUNT",POST_LIT*.7f,3.f);
        shake(1.6f,.08f); fx.spawnBurst(headC(1.f),POST_LIT,100,22.f,1.5f,3.f);
        shepherdSound("shep_break",shepMouthPose());
        for(auto& e:enemies)if(e.targetable()&&e.takeDamage(1e6f))onEnemyKilled(e,StyleSource::ENVIRONMENT);
        shepLastT=0.f;
    }
}
// Both punch and sword call this during the bright final part of the wind-up.
inline bool GameplayState::parryHead() {
    using namespace shephead;
    if(!shepherd()||level.arenaAt(player.position)!=10||shepHeadHp<=0.f)return false;
    if(shepGrabT>=0.f){shepGrabHp-=GRAB_BREAK;parryFeedback(player.camera.position+player.camera.forward()*2.f,true);shepReleaseGrab(true);return true;}
    if(shepAtk==GRAB&&shepAtkT>=GRAB_CLOSE-.22f&&shepAtkT<GRAB_CLOSE&&!(shepAttackHits&1)&&grabHits(player.position,player.height,shepAttackAt)){
        auto to=shepHandPose(shepSlamArm)-player.camera.position;
        if(glm::length(to)>.01f&&glm::dot(player.camera.forward(),glm::normalize(to))<.1f)return false;
        shepAttackHits|=1;shepOpenT=OPEN;parryFeedback(player.camera.position+player.camera.forward()*2.f,true);
        shepHeadHurt(PARRY_DAMAGE);styleSystem.heal(10.f*tune().heal);styleSystem.addStyle(120.f,StyleSource::PARRY);return true;
    }
    if(shepAtk!=SLAM&&shepAtk!=SLAM2)return false;
    for(int a=0;a<(shepAtk==SLAM2?2:1);++a) {
        const float impact=SLAM_TELL+(shepAtk==SLAM2&&a==1?.42f:0.f);
        if(shepAtkT<impact-.27f||shepAtkT>=impact)continue;
        if(shepSlamParried[a]||!slamHits(player.position,shepSlamAt[a]))continue;
        const glm::vec3 to=shepHandPose(shepAtk==SLAM2?a:shepSlamArm)-player.camera.position;
        if(glm::dot(player.camera.forward(),glm::normalize(to))<.1f)return false;
        shepSlamParried[a]=true; const int k=shepAtk==SLAM2?a:shepSlamArm;
        // The turned fist still follows its descent. It becomes a grounded
        // weak point at impact, rather than teleporting during the parry flash.
        shepHandAt[k]=shepSlamAt[a]; shepOpenT=OPEN;
        parryFeedback(player.camera.position+player.camera.forward()*2.f,true);
        styleSystem.heal(10.f*tune().heal); styleSystem.addStyle(120.f,StyleSource::PARRY);
        ui.feed("A GOD'S HAND, TURNED",POST_LIT); shepHeadHurt(PARRY_DAMAGE);
        return true;
    }
    return false;
}
inline void GameplayState::shepHead(float dt) {
    using namespace shephead;
    if(playerDead){stopShepherdAudio();return;}
    // Every early-return path still advances the pose exactly once per sim tick.
    struct PoseStep { GameplayState& g; float dt; ~PoseStep(){g.animateShepherd(dt);} } poseStep{*this,dt};
    const auto p=player.position;
    shepHitSoundT=std::max(0.f,shepHitSoundT-dt);
    shepGazeTick=std::max(0.f,shepGazeTick-dt);
    for(auto& scar:shepScars){scar.life-=dt;
        if(shepGazeTick<=0&&scar.life>0&&p.y<shep::FALL_Y+2.4f&&segmentDistance(glm::vec2(p.x,p.z),glm::vec2(scar.from.x,scar.from.z),glm::vec2(scar.to.x,scar.to.z))<1.25f){
            if(damagePlayer(9.f,scar.to,.1f,.02f,.2f,{EnemyType::COUNT,DamageKind::BEAM}))shepGazeTick=.4f;
        }
    }
    shepScars.erase(std::remove_if(shepScars.begin(),shepScars.end(),[](const ShepScar& s){return s.life<=0;}),shepScars.end());
    shepGazeX+=(glm::clamp(p.x,shep::FALL_X0,shep::FALL_X1)-shepGazeX)*std::min(1.f,dt*2.f);
    shepHitHealT=std::max(0.f,shepHitHealT-dt);
    shepGape+=(float(shepAtk==AIM||shepAtk==PULL)-shepGape)*std::min(1.f,dt*6.f);
    updateShepherdCourt(dt);
    if(shepHeadHp<=0.f)return;
    if(shepRiseT>=0.f) {
        const float before=shepRiseT; shepRiseT+=dt; shepHeadRise=std::clamp(shepRiseT/RISE,0.f,1.f);
        shake(.1f,.025f);
        if(before==0.f){pushBanner("THE SHEPHERD","THE BODY THAT HOLDS THE RING",POST_LIT*.7f,4.f);shepherdSound("shep_gather",chest(1.f));}
        if(shepRiseT>=RISE){shepRiseT=-1.f;shepHeadRise=1.f;shepHeadSeen=true;shepIdleT=.7f;
            ui.toast("COUNTERWEIGHT","PARRY WHITE FISTS. PIN A GROUNDED PALM WITH "+binds().label(Act::HOLDFAST)+" TO TURN ITS ORBIT AGAINST IT",HOLDFAST_STEEL,7.f);}
        return;
    }
    if(shepRearT>=0.f) {
        if(shepRearT==0.f){pushBanner(shepHeadPhase==1?"I HOLD EVERY ONE.":"THEN FALL WITH THEM.",shepHeadPhase==1?"WEST CLAMP BROKEN. THE GALLERY OPENS":"EAST CLAMP BROKEN. THE FLEET BREAKS FREE",POST_LIT*.7f,3.f);shake(1.f,.05f);}
        shepRearT+=dt;
        if(shepRearT>=REAR){shepRearT=-1.f;shepIdleT=.5f;}
        return;
    }
    shepOpenT=std::max(0.f,shepOpenT-dt);
    if(swordOut&&sword.parryOpen()&&parryHead()){sword.tryParry();if(shepRearT>=0.f)return;}
    if(shepAtk==NONE) {
        if((shepIdleT-=dt)>0.f)return;
        shepAtk=nextAttack(shepHeadPhase,shepCycle++);shepAtkT=0.f;
        shepVariation=((shepCycle-1)/(shepHeadPhase==0?7:shepHeadPhase==1?8:9)+shepHeadPhase)%2;
        shepAttackHits=0;shepGrabCaught=false;shepGrabT=-1;
        shepSlamArm=1-shepSlamArm;
        // A pinned/grounded arm stays planted. Use the other hand, or pressure
        // with the eyes while both hands are committed to the hull.
        const auto busy=[&](int k){return shepArmDown[k]>0.f||shepPinT[k]>0.f;};
        if(shepAtk==SLAM||shepAtk==SLAM2||shepAtk==SWEEP||shepAtk==GRAB){
            if(busy(shepSlamArm))shepSlamArm=1-shepSlamArm;
            if(busy(shepSlamArm))shepAtk=GAZE;
            else if(shepAtk==SLAM2&&(busy(0)||busy(1)))shepAtk=SLAM;
        }
        shepSlamParried[0]=shepSlamParried[1]=false;
        shepSlamAt[0]=shepSlamAt[1]=p;shepEchoN=shepEchoHit=0;
        shepAttackAt=p;shepAttackAt.y=level.groundAt(p.x,p.z,p.y);
        shepSweepZ=p.z;shepGrabFrom=shepHandPose(shepSlamArm)+glm::vec3{0,12,-15};
        if(shepAtk==SWEEP){shepAttackAt.z=glm::clamp(p.z+(p.z>-3972.f?-18.f:18.f),shep::FALL_ZN+10.f,shep::FALL_ZS-10.f);
            ui.feed(shepVariation?"CROSS-SWEEP: TWO LANES. CHANGE YOUR DEPTH":"ARM SWEEP: DASH FORWARD OR BACK",SWEEP_RED);}
        if(shepAtk==GRAB)ui.feed("OPEN HAND: DODGE SIDEWAYS. BREAK ITS GRIP IF CAUGHT",HOLDFAST_STEEL);
        if(shepAtk==GAZE)ui.feed("ITS EYES FIND YOU. MOVE WHEN THE STARE LOCKS",SWEEP_RED);
        audio.stop(shepWindSound,.012f);shepWindSound=0;
        if(const char* cue=shepherdaudio::attackCue(shepAtk))
            shepWindSound=shepherdSound(cue,shepAtk==SLAM||shepAtk==SLAM2||shepAtk==SWEEP||shepAtk==GRAB?shepHandPose(shepSlamArm):shepMouthPose());
        if(shepAtk==BEAM){shepBeamDir=p.x<-600.f?1.f:-1.f;shepBeamX=shepBeamDir>0.f?shep::FALL_X0-2.f:shep::FALL_X1+2.f;shepBeamOn=shepBeamHit=false;}
        if(shepAtk==AIM){auto s=sweeptier::aimed(shepMouthPose(),p,AIM_WARN);s.flashLead=AIM_FLASH;s.range=30.f;
            const float dist=glm::length(glm::vec2{p.x-s.origin.x,p.z-s.origin.z});s.coneHalf=std::atan(12.f/std::max(dist,1.f));sweep.start(s);++sweepCount;}
        if(shepAtk==PULL)ui.feed("IT GATHERS THE RING: HOLDFAST TO THE HULL",HOLDFAST_STEEL);
        if(shepAtk==ECHO)ui.feed("IT REMEMBERS: LEAVE YOUR MARKS BEHIND",POST_LIT);
        return;
    }
    const float before=shepAtkT;shepAtkT+=dt;
    auto finish=[&](){shepAtk=NONE;shepIdleT=shepHeadPhase==0?.6f:.38f;shepBeamOn=false;shepEchoN=shepEchoHit=0;};
    if(shepAtk==SWEEP||shepAtk==GRAB||shepAtk==GAZE){shepPressure(dt,before);
    }else if(shepAtk==SLAM||shepAtk==SLAM2) {
        const int n=shepAtk==SLAM2?2:1;
        for(int a=0;a<n;++a)if(shepAtkT<SLAM_LOCK+(a==1?.42f:0.f))
            shepSlamAt[a]=p+glm::vec3{n==2?float(a?6:-6):0.f,0.f,0.f};
        for(int a=0;a<n;++a){auto& q=shepSlamAt[a];q.x=glm::clamp(q.x,shep::FALL_X0+8.f,shep::FALL_X1-8.f);q.z=glm::clamp(q.z,shep::FALL_ZN+10.f,shep::FALL_ZS-8.f);q.y=level.groundAt(q.x,q.z,p.y);}
        for(int a=0;a<n;++a){const float impact=SLAM_TELL+(a==1?.42f:0.f);
        if(before<impact-.27f&&shepAtkT>=impact-.27f)shepherdSound("shep_parry",shepSlamAt[a]);
        if(before<impact&&shepAtkT>=impact) {
            {const int k=n==2?a:shepSlamArm;const auto at=shepSlamAt[a];
                shepHandAt[k]=at;shepArmDown[k]=HAND_DOWN;
                if(shepSlamParried[a]){fx.spawnShockwave(at,14.f,HOLDFAST_STEEL);shepherdSound("shep_contact",at,.9f);continue;}
                wakeRings.push_back({at,.6f,false});
                fx.spawnBurst(at,POST_LIT,45,13.f,1.f,3.f);fx.spawnShockwave(at,14.f,POST_LIT);
                if(slamHits(p,at)&&invincFrames<=0.f)damagePlayer(SLAM_DAMAGE,at,.5f,.06f,0.f,{EnemyType::COUNT,DamageKind::SHOCKWAVE});
                for(auto& e:enemies)if(e.targetable()&&slamHits(e.position,at)&&e.takeDamage(1e6f))onEnemyKilled(e,StyleSource::ENVIRONMENT);
            }
            shake(.8f,.06f);shepherdSound("shep_contact",shepSlamAt[a]);
            if(!shepHandHint){shepHandHint=true;ui.toast("ITS PALM IS GROUNDED","SHOOT IT, CUT IT, OR DRIVE HOLDFAST INTO IT",HOLDFAST_STEEL,4.f);}
        }}
        if(shepAtkT>SLAM_TELL+(n==2?.42f:0.f)+.4f)finish();
    } else if(shepAtk==BEAM) {
        if(shepAtkT>=BEAM_TELL){
            if(!shepBeamOn){shepBeamOn=true;shepherdSound("shep_beam",{shepBeamX,shep::FALL_Y+2.f,p.z});}
            const float old=shepBeamX;shepBeamX+=shepBeamDir*BEAM_SPEED*dt;
            if(!shepBeamHit&&p.x>std::min(old,shepBeamX)-BEAM_HALF&&p.x<std::max(old,shepBeamX)+BEAM_HALF&&p.y<shep::FALL_Y+4.5f&&invincFrames<=0.f&&!shepFinCovers(p)){
                shepBeamHit=true;damagePlayer(BEAM_DAMAGE,p+glm::vec3{0,1,-5},.3f,.04f,0.f,{EnemyType::COUNT,DamageKind::UNKNOWN});}
            if(shepBeamX<shep::FALL_X0-6.f||shepBeamX>shep::FALL_X1+6.f)finish();
        }
    } else if(shepAtk==AIM) {if(!sweep.active()&&shepAtkT>AIM_FLASH+.5f)finish();
    } else if(shepAtk==ORRERY) {
        // Five moons cast from its halo in a fan. White cores can be punched back.
        for(int k=0;k<(shepVariation?7:5);++k){const float at=.8f+k*.32f;if(before<at&&shepAtkT>=at){
            const float a=3.14159265f*(.15f+.175f*k);
            const auto from=headC(1.f)+glm::vec3{60.f*std::cos(a),44.f*std::sin(a),18.f};
            const auto dir=glm::normalize(player.camera.position+player.velocity*.25f-from);
            if(auto* shot=projSystem.fire(from,dir*(45.f+shepHeadPhase*6.f),18.f*tune().damage,false,glm::vec3{2.5f},false,0.f,2.6f,true)){shot->owner=SHOT_OWNER;shot->parryDamage=180.f;shot->lifetime=8.f;}
            fx.spawnShockwave(from,6.f,POST_LIT);
            shepherdSound("shep_moon",from,.65f);
        }}
        if(shepAtkT>3.1f)finish();
    } else if(shepAtk==ECHO) {
        // Three captured positions; each mark has its own full 1.8-second tell.
        for(int k=0;k<3;++k){const float record=.2f+k*.65f;if(before<record&&shepAtkT>=record){shepEchoAt[k]=p;shepEchoAt[k].y=level.groundAt(p.x,p.z,p.y);++shepEchoN;shepherdSound("shep_memory",shepEchoAt[k]);}
            const float hit=record+1.8f;if(before<hit&&shepAtkT>=hit){++shepEchoHit;const auto at=shepEchoAt[k];wakeRings.push_back({at,.6f,false});fx.spawnBurst(at,POST_LIT,40,12.f,.7f,3.f);shake(.5f,.035f);shepherdSound("shep_contact",at,.8f);
                if(slamHits(p,at)&&invincFrames<=0.f)damagePlayer(26.f,at,.3f,.04f,0.f,{EnemyType::COUNT,DamageKind::SHOCKWAVE});
            }}
        if(shepAtkT>3.8f)finish();
    } else if(shepAtk==PULL) {
        if(!holdfast.held()){auto to=glm::normalize(glm::vec3{-600.f-p.x,0.f,-4040.f-p.z}+glm::vec3{.0001f});player.velocity+=to*(PULL_SPEED*dt*4.f);}
        if(before<PULL_LEN&&shepAtkT>=PULL_LEN){
            // Inward rings instead of a lethal sweep: HOLDFAST counters the drag, jump the fronts.
            wakeRings.push_back({{-600.f,shep::FALL_Y,-4040.f},.6f,false});
            wakeRings.push_back({{-600.f,shep::FALL_Y,-3950.f},.6f,false});
            fx.spawnShockwave(chest(1.f),35.f,POST_LIT);shepherdSound("shep_contact",chest(1.f));
        }
        if(shepAtkT>PULL_LEN+.6f)finish();
    }
}
// Choose the closest actual body part, respecting scenery and enemies in front.
inline bool GameplayState::shootHead(glm::vec3 from,glm::vec3 dir,float& distance,float dmg) {
    using namespace shephead;
    if(!shepherd()||level.arenaAt(player.position)!=10||shepHeadHp<=0.f||shepHeadRise<.99f)return false;
    if(shepBreakGrip(from,dir,distance,dmg))return true;
    float best=distance,mult=0.f;
    auto test=[&](const AABB& b,float m){float t=hitBody(from,dir,b);if(t>0.f&&t<best){best=t;mult=m;}};
    const auto bodyInv=glm::inverse(shepChestPose());
    float bodyHit=hitBody(glm::vec3(bodyInv*glm::vec4(from,1)),glm::vec3(bodyInv*glm::vec4(dir,0)),chestBox(shepHeadRise));
    if(bodyHit>0.f&&bodyHit<best){best=bodyHit;mult=BODY_MULT;}
    const auto H=shepFacePose(),inv=glm::inverse(H);
    float t=rayBoxHit(glm::vec3(inv*glm::vec4(from,1.f)),glm::vec3(inv*glm::vec4(dir,0.f)),{{-HW,-HH,-HD},{HW,HH,HD}});
    if(t>0.f&&t<best){best=t;mult=1.f;}
    for(int k=0;k<2;++k)if(shepArmDown[k]>0.f||shepPinT[k]>0.f)test(handBox(shepHandAt[k]),HAND_MULT);
    if(mult==0.f)return false;
    distance=best;fx.spawnHitSparks(from+dir*best,shepOpenT>0.f?POST_LIT:glm::vec3{.6f,1.5f,2.5f});
    const float hp=shepHeadHp;shepHeadHurt(dmg*mult*(shepOpenT>0.f?1.6f:1.f));
    if(shepHeadHp<hp)shepherdHitSound(from+dir*best,dmg);
    if(shepHeadHp<hp&&shepHitHealT<=0.f){
        const bool committed=(mult==HAND_MULT&&best<42.f)||(shepOpenT>0.f&&best<100.f);
        shepHitHealT=committed?.3f:.8f;
        styleSystem.heal((committed?3.f:1.f)*tune().heal);styleSystem.addStyle(4.f,StyleSource::ENVIRONMENT);
    }
    return true;
}
// HOLDFAST uses the same nearest-hit limit as walls; no pinning through cover.
inline glm::vec3 GameplayState::shepMoon(int k) const { return shephead::moon(k,shepOrbitClock[k]); }
inline bool GameplayState::pinShepherd(glm::vec3 from,glm::vec3 dir,float distance) {
    using namespace shephead;
    if(!shepherd())return false;
    if(shepGrabT>=0.f){float range=distance;
        if(shepBreakGrip(from,dir,range,GRAB_BREAK)){holdfast.letGo(Holdfast::COOLDOWN);return true;}}
    const int arena=level.arenaAt(player.position);int hit=-1;float best=distance;
    if(arena==10&&shepHeadHp>0.f&&shepRearT<0.f){
        for(int k=0;k<2;++k)if(shepArmDown[k]>0.f&&shepPinT[k]<=0.f){float t=hitBody(from,dir,handBox(shepHandAt[k]));if(t>0.f&&t<best){best=t;hit=k;}}
        if(hit<0)return false;
        shepPinT[hit]=PIN_TIME;shepArmDown[hit]=PIN_TIME;shepCounterHit[hit]=false;
        shepherdSound("shep_pin",shepHandAt[hit]);
        audio.stop(shepStrainSound[hit],.012f);shepStrainSound[hit]=shepherdSound("shep_strain",shepHandAt[hit],.65f);
        // A palm pin is a deliberate counter, like turning a white fist. It
        // can earn Overdrive instead of stopping at the passive hazard cap.
        styleSystem.addStyle(180.f,StyleSource::PARRY);styleSystem.heal(8.f*tune().heal);
        ui.feed("PALM PINNED. ITS ORBIT IS COMING DOWN",HOLDFAST_STEEL);
        shake(.7f,.045f);fx.spawnShockwave(shepHandAt[hit],15.f,HOLDFAST_STEEL);
    }else if(arena==3&&shepTurnT<0.f&&shepInT<0.f){
        for(int k=0;k<3;++k)if(shepVentHp[k]>0.f&&shepOrbitPin[k]<=0.f){auto c=shepMoon(k);float t=rayBoxHit(from,dir,{c-glm::vec3{MOON_RADIUS},c+glm::vec3{MOON_RADIUS}});if(t>0.f&&t<best){best=t;hit=k;}}
        if(hit<0)return false;
        shepOrbitPin[hit]=5.f;ui.feed("ORBIT PINNED: BREAK THE MOON",HOLDFAST_STEEL);
    }else return false;
    holdfast.letGo(Holdfast::COOLDOWN);
    fx.spawnTracer(from,from+dir*best,.08f,.4f,HOLDFAST_STEEL*2.f);
    if(arena!=10)shepherdSound("shep_pin",from+dir*best);
    return true;
}
inline bool GameplayState::shepherdSplinterPoint(glm::vec3 from,glm::vec3& at) const {
    if(!shepherd())return false;
    const int arena=level.arenaAt(player.position);float best=SplinterSystem::REACH;bool found=false;
    auto test=[&](glm::vec3 q){const auto off=q-from;float d=glm::length(off);if(d<.3f||d>=best)return;float wall=wallAlong(from,off/d,d);if(wall>0.f&&wall<d-.5f)return;best=d;at=q;found=true;};
    if(arena==10&&shepHeadHp>0.f&&shepHeadRise>.99f){
        test(glm::vec3(shepChestPose()*glm::vec4(shephead::chest(1.f)+glm::vec3{0,5,26},1)));
        test(glm::vec3(shepFacePose()*glm::vec4{0,0,33,1}));
        for(int k=0;k<2;++k)if(shepArmDown[k]>0.f)test(shepHandAt[k]+glm::vec3{0,3,0});
    }else if(arena==3&&shepTurnT<0.f)for(int k=0;k<3;++k)if(shepVentHp[k]>0.f)test(shepMoon(k));
    return found;
}
// Called before the projectile step, so a rocket or returned moon cannot tunnel
// through the animated body or disappear on scenery behind it.
inline void GameplayState::shepherdShots(float dt) {
    using namespace shephead;
    if(!shepherd())return;
    for(auto& p:projSystem.pool){
        if(!p.alive||!p.isPlayer)continue;
        if(p.parried&&p.owner==SHOT_OWNER&&shepHeadHp>0.f){const auto to=glm::vec3(shepChestPose()*glm::vec4(chest(1.f),1))-p.position;p.velocity=glm::normalize(to)*90.f;}
        const float speed=glm::length(p.velocity);if(speed<.001f)continue;
        float dist=speed*dt+std::max(.1f,p.size*.25f);
        const auto dir=p.velocity/speed;
        const float wall=wallAlong(p.position,dir,dist);if(wall>0.f)dist=std::min(dist,wall);
        if(shootVent(p.position,dir,dist,p.damage)){p.alive=false;if(p.blastRadius>0.f){fx.spawnExplosionParticles(p.position,p.blastRadius);playBoom("explosion",p.position,110,SoundGroup::WORLD);}if(p.parried){fx.spawnShockwave(p.position,12.f,POST_LIT);styleSystem.addStyle(80.f,StyleSource::PARRY);}}
    }
}
inline void GameplayState::blastShepherd(glm::vec3 at,float radius,float damage) {
    if(!shepherd()||damage<=0.f||radius<=0.f)return;
    using namespace shephead;
    if(level.arenaAt(player.position)==10&&shepHeadHp>0.f){
        if(shepGrabT>=0.f&&glm::length(at-shepHandPose(shepSlamArm))<radius+16.f){shepGrabHp-=damage;if(shepGrabHp<=0)shepReleaseGrab(true);}
        float near=1e9f,mult=1.f;
        auto test=[&](AABB b,float m,glm::vec3 point){float d=glm::length(point-glm::clamp(point,b.min,b.max));if(d<near){near=d;mult=m;}};
        test(chestBox(shepHeadRise),BODY_MULT,glm::vec3(glm::inverse(shepChestPose())*glm::vec4(at,1)));
        test({{-HW,-HH,-HD},{HW,HH,HD}},1.f,glm::vec3(glm::inverse(shepFacePose())*glm::vec4(at,1)));
        for(int k=0;k<2;++k)if(shepArmDown[k]>0.f)test(handBox(shepHandAt[k]),HAND_MULT,at);
        if(near<radius){const float hp=shepHeadHp;shepHeadHurt(damage*mult*(1.f-near/radius)*(shepOpenT>0.f?1.6f:1.f));if(shepHeadHp<hp)shepherdHitSound(at,damage);}
    }else if(level.arenaAt(player.position)==3){
        for(int k=0;k<3;++k)if(shepVentHp[k]>0.f){auto c=shepMoon(k);float d=glm::length(c-at);if(d<radius+MOON_RADIUS&&d>.001f){float range=d+MOON_RADIUS;shootVent(at,(c-at)/d,range,damage*(1.f-std::max(0.f,d-MOON_RADIUS)/radius));}}
    }
}
inline void GameplayState::gatherHead(std::vector<BoxInstance>& out) {
    using namespace rig;using namespace shephead;
    if(level.arenaAt(player.position)!=10&&shepFinT<0.f)return;
    const size_t first=out.size();
    const bool dead=shepHeadHp<=0.f;const float rise=shepHeadRise;
    const auto B=shepBodyPose(),C=shepChestPose(),H=shepFacePose();
    const glm::vec3 skin{.045f,.065f,.085f},light{.14f,.22f,.30f};
    const float expose=shepOpenT>0.f?1.f:0.f;
    auto part=[&](glm::mat4 xf,glm::vec3 color,glm::vec3 glow,int mesh){BoxInstance b;b.mesh=(float)mesh;b.model=xf;b.color=color;b.emissive=glow;out.push_back(b);};
    auto ellipsoid=[&](glm::vec3 at,glm::vec3 size,glm::vec3 color,glm::vec3 glow){part(T(at)*S(size),color,glow,headmesh::orb());};
    auto bone=[&](glm::vec3 a,glm::vec3 b,float w,float depth){const auto d=b-a;const float len=glm::length(d);if(len<.01f)return;
        const auto f=d/len;part(T((a+b)*.5f)*RY(std::atan2(f.x,f.z))*RX(-std::asin(glm::clamp(f.y,-1.f,1.f)))*S(glm::vec3{w,depth,len}),skin,light*.07f,headmesh::capsule());};
    // Adult anatomy and swept hull armor are actual Blender meshes. The torso
    // has broad lats and flat pecs, with a narrow waist under the ship's ribs.
    auto chestC=chest(rise);const auto hc=glm::vec3(B*glm::vec4(headC(rise),1));
    const auto anatomyLight=dead?glm::vec3{0.f}:expose?HOLDFAST_STEEL:glm::vec3{.13f,.145f,.16f};
    shepherdmodel::torso(out,shepRootPose()*T(chestC)*S(glm::vec3{shepherdmodel::TORSO_SCALE}),anatomyLight,glm::vec4{shepSpine.x,shepSpine.y,shepBreath,1.f});
    // The heart is an orbit cage: each phase tears a ring away.
    const auto heart=glm::vec3(C*glm::vec4(chestC+glm::vec3{0.f,5.f,26.f},1));
    ellipsoid(heart,{10,14,8},glm::vec3{.005f},dead?POST_LIT:expose?POST_LIT*1.5f:light*.15f);
    float strain=0.f;
    for(int k=0;k<2;++k)if(shepPinT[k]>0.f&&!shepCounterHit[k])strain=std::max(strain,smooth((PIN_TIME-shepPinT[k])/COUNTER_HIT));
    for(int ring=0;ring<3;++ring)for(int j=0;j<40;++j){
        if(ring<shepHeadPhase&&j%10>=4)continue;
        float a=6.2831853f*j/40.f;float r=14.f+ring*4.f;
        const auto M=T(heart)*RY(.25f*std::sin(gameClock*.3f+ring)+strain*.6f)*RZ((ring-1)*.65f+gameClock*.12f*(ring%2?1.f:-1.f)+strain*.4f);
        glm::vec3 glow=ring<shepHeadPhase?light*.1f:expose?HOLDFAST_STEEL:light*.15f;
        push(out,M*T(glm::vec3{std::cos(a)*r,std::sin(a)*r,0})*RZ(a)*S(glm::vec3{.55f,r*.158f,.6f}),skin*.4f,glow);}
    // Persistent open seams make damage readable on the body itself.
    for(int stage=0;stage<shepHeadPhase;++stage)for(int j=0;j<5;++j){
        float sd=stage?1.f:-1.f;auto a=heart+glm::vec3{sd*(8.f+j*4.f),-8.f+j*7.f,3.f};
        bone(a,a+glm::vec3{sd*5.f,9.f,0},.65f,.65f);
        push(out,T(a)*RZ(sd*-.45f)*S({.45f,9.f,.3f}),glm::vec3{.06f},HOLDFAST_STEEL*.5f);
    }
    const glm::vec3 gaze=dead?glm::vec3{0.f}:shepAtk==AIM||shepAtk==GAZE||shepAtk==GRAB?SWEEP_RED:expose?POST_LIT:glm::vec3{.58f,.60f,.63f};
    shepherdmodel::piece(out,"shepherd_head",H*S(glm::vec3{shepherdmodel::HEAD_SCALE}),gaze);
    // A broken orrery behind the skull. Moons travel on distinct inclined planes.
    for(int r=0;r<3;++r){const float rad=58.f+r*9.f;auto M=T(hc+glm::vec3{0,7,-23})*RZ((r-1)*.45f)*RY((r-1)*.32f);
        for(int j=0;j<72;++j){if(j%12>=10||(r<shepHeadPhase&&j%5<2))continue;float a=6.2831853f*j/72.f;
            push(out,M*T(glm::vec3{std::cos(a)*rad,std::sin(a)*rad,0})*RZ(a)*S(glm::vec3{1.1f,rad*.083f,1.8f}),skin*.45f,expose?light*.5f:light*.025f);}
        for(int m=0;m<4;++m){float a=gameClock*(.22f+r*.08f)+m*1.5707963f;part(M*T(glm::vec3{std::cos(a)*rad,std::sin(a)*rad,0})*S(glm::vec3{3.f+r}),skin*.5f,expose?POST_LIT:light*.5f,headmesh::orb());}}
    for(int k=0;k<2;++k){auto sh=glm::vec3(B*glm::vec4(shoulder(k,rise),1)),hand=shepHandPose(k);const float sd=k?1.f:-1.f;
        const auto elbow=shephead::elbowFrom(k,sh,hand);
        shepherdmodel::piece(out,"shepherd_shoulder",T(sh)*S(glm::vec3{33.f}),anatomyLight);
        shepherdmodel::limb(out,"shepherd_upperarm",sh,elbow,26.f,28.f,anatomyLight);
        shepherdmodel::piece(out,"shepherd_shoulder",T(elbow)*S(glm::vec3{22.f}),anatomyLight*.5f);
        shepherdmodel::limb(out,"shepherd_forearm",elbow,hand,20.f,22.f,anatomyLight);
        const bool down=shepArmDown[k]>0.f||shepPinT[k]>0.f;
        float localTime=shepAtkT-(shepAtk==SLAM2&&k==1?.42f:0.f);
        const bool white=(shepAtk==SLAM2||(shepAtk==SLAM&&shepSlamArm==k))&&localTime>=SLAM_TELL-.27f&&localTime<SLAM_TELL;
        const bool catchWhite=shepAtk==GRAB&&shepSlamArm==k&&shepAtkT>=GRAB_CLOSE-.22f&&shepAtkT<GRAB_CLOSE;
        const auto glow=white||catchWhite?POST_LIT*3.f:shepPinT[k]>0.f?HOLDFAST_STEEL*1.4f:down?glm::vec3{1.7f,.85f,.22f}:anatomyLight;
        const auto palmRotation=down?RY(3.14159265f):shepPoseReady?glm::mat4_cast(shepPalmRotation[k]):RY(3.14159265f);
        const auto P=T(hand)*palmRotation*RX(down?0.f:shepWristTilt[k])*S(glm::vec3{1.2f});
        shepherdmodel::piece(out,"shepherd_palm",P*S(glm::vec3{13.f,4.f,13.f}),glow);
        for(int f=0;f<4;++f){float len=f==1||f==2?7.f:5.8f;const float curl=down?0.f:shepFingerCurl[k]+.08f*std::sin(gameClock*1.1f+f*.8f+k);
            const auto F=P*T(glm::vec3{-4.5f+3.f*f,0.f,5.f})*RY((f-1.5f)*.035f*(1.f-curl))*RX(curl*.4f);
            shepherdmodel::piece(out,"shepherd_finger",F*T(glm::vec3{0,0,len*.5f})*S(glm::vec3{2.4f,2.5f,len}),glow);
            shepherdmodel::piece(out,"shepherd_finger",F*T(glm::vec3{0,0,len})*RX(curl)*T(glm::vec3{0,0,len*.4f})*S(glm::vec3{2.f,2.f,len*.8f}),glow);}
        shepherdmodel::piece(out,"shepherd_finger",P*T(glm::vec3{sd*7,0,0})*RY(sd*-.8f)*S(glm::vec3{2.8f,2.8f,9.f}),glow);
        if(shepPinT[k]>0.f){bone(shepHandAt[k]+glm::vec3{0,-.1f,0},hand+glm::vec3{0,8,0},.6f,.6f);
            push(out,T(shepHandAt[k]+glm::vec3{0,5.f,0})*S(glm::vec3{1.f,9.f,1.f}),HOLDFAST_STEEL,HOLDFAST_STEEL*2.f);}
    }
    // Mandatory attack cues: ground marks, descending ghost palms, the gaze beam.
    auto mark=[&](glm::vec3 at,float r,glm::vec3 col){for(int j=0;j<48;++j){float a=6.2831853f*j/48.f;push(out,T(at+glm::vec3{std::cos(a)*r,.09f,std::sin(a)*r})*RY(-a)*S(glm::vec3{.22f,.1f,r*.132f}),col*.1f,col);}};
    if(shepAtk==SLAM||shepAtk==SLAM2)for(int a=0;a<(shepAtk==SLAM2?2:1);++a){
        float t=shepAtkT-(a==1?.42f:0.f);if(t>=SLAM_TELL)continue;
        auto col=t>=SLAM_TELL-.27f?POST_LIT*2.f:SWEEP_RED*1.6f;
        mark(shepSlamAt[a],SLAM_R,col);mark(shepSlamAt[a],SLAM_R*(1.7f-.7f*std::clamp(t/SLAM_TELL,0.f,1.f)),col*.7f);
    }
    if(shepAtk==SWEEP){
        for(int pass=0;pass<(shepVariation?2:1);++pass){float start=SWEEP_TELL+pass*(SWEEP_TRAVEL+.35f);
            if(shepAtkT>start+SWEEP_TRAVEL)continue;float z=pass?shepAttackAt.z:shepSweepZ;
            float pulse=.7f+.3f*std::sin(shepMotionClock*14.f);
            for(float edge:{-SWEEP_HALF,SWEEP_HALF})push(out,T(glm::vec3{-600,shep::FALL_Y+.14f,z+edge})*S(glm::vec3{shep::FALL_X1-shep::FALL_X0,.15f,.3f}),SWEEP_RED*.1f,SWEEP_RED*pulse);
            // Chevrons point out of the swept strip, so forward/back reads at a glance.
            for(float x=shep::FALL_X0+8.f;x<shep::FALL_X1;x+=12.f)for(float dir:{-1.f,1.f}){
                bone({x-1,shep::FALL_Y+.18f,z+dir*4},{x,shep::FALL_Y+.18f,z+dir*6},.17f,.12f);
                bone({x+1,shep::FALL_Y+.18f,z+dir*4},{x,shep::FALL_Y+.18f,z+dir*6},.17f,.12f);
            }
        }
    }
    if(shepAtk==GRAB&&shepAtkT<GRAB_CLOSE){
        auto col=shepAtkT>=GRAB_CLOSE-.22f?POST_LIT*2.f:SWEEP_RED*1.5f;
        mark(shepAttackAt,7.f,col);mark(shepAttackAt,10.f-3.f*smooth(shepAtkT/GRAB_CLOSE),col*.7f);
        bone(shepHandPose(shepSlamArm),shepAttackAt+glm::vec3{0,3,0},.15f,.15f);
    }
    if(!dead){
        // Eye shafts make his attention readable even while he holds his guard.
        for(int eye=0;eye<2;++eye){auto from=shepEyePose(eye);auto target=shepLookAt;auto delta=target-from;
            if(glm::length(delta)>.01f){
                if(shepAtk==GAZE){auto hit=shepGazeMark(gazeIndex(shepAtkT,shepVariation))+glm::vec3{eye?1.6f:-1.6f,.15f,0};
                    const bool locked=shepAtkT>=GAZE_TRACK;const auto col=locked?SWEEP_RED*1.6f:glm::vec3{.5f,.55f,.65f};
                    const auto d=hit-from;part(propdraw::along((from+hit)*.5f,d)*S(glm::vec3{locked?.36f:.22f,locked?.36f:.22f,glm::length(d)}),glm::vec3{.01f},col,headmesh::capsule());
                }else{auto end=from+glm::normalize(delta)*22.f;bone(from,end,.5f,.5f);}
            }
            ellipsoid(from,glm::vec3{1.4f},glm::vec3{.01f},shepAtk==GAZE?SWEEP_RED*2.f:POST_LIT*.7f);
        }
    }
    if(shepAtk==GAZE){int n=shepVariation?3:1;
        for(int k=0;k<n;++k){if(shepAtkT>GAZE_FIRE+k*.6f+.18f)continue;
            const auto at=shepGazeMark(k);
            mark(at,4.8f,SWEEP_RED*(shepAtkT>=GAZE_TRACK?1.8f:.6f));
        }
    }
    for(const auto& scar:shepScars){auto d=scar.to-scar.from;
        push(out,propdraw::along((scar.from+scar.to)*.5f,d)*S(glm::vec3{2.5f,.13f,glm::length(d)}),glm::vec3{.02f},SWEEP_RED*.65f*std::min(1.f,scar.life));
    }
    if(shepGrabT>=0.f){auto hand=shepHandPose(shepSlamArm);ellipsoid(hand+glm::vec3{0,5,8},glm::vec3{2.3f},glm::vec3{.02f},HOLDFAST_STEEL*2.f);}
    if(shepAtk==ECHO)for(int k=shepEchoHit;k<shepEchoN;++k){float age=shepAtkT-(.2f+k*.65f);mark(shepEchoAt[k],SLAM_R,SWEEP_RED*1.5f);
        const float y=30.f*(1.f-std::clamp(age/1.8f,0.f,1.f));part(T(shepEchoAt[k]+glm::vec3{0,y+2,0})*S(glm::vec3{12,1,12}),glm::vec3{.08f},light*.3f,headmesh::orb());}
    if(shepAtk==BEAM){const float zc=(shep::FALL_ZN+shep::FALL_ZS)*.5f,zl=shep::FALL_ZS-shep::FALL_ZN;
        push(out,T(glm::vec3{shepBeamX,shep::FALL_Y+(shepBeamOn?2.2f:.1f),zc})*S(glm::vec3{shepBeamOn?2.f*BEAM_HALF:.5f,shepBeamOn?4.4f:.08f,zl}),glm::vec3{0},shepBeamOn?POST_LIT*2.f:SWEEP_RED*2.f);
        if(shepBeamOn)bone(shepMouthPose(),{shepBeamX,shep::FALL_Y+2.f,zc},.8f,.8f);}
    if(shepAtk==PULL)for(int j=0;j<6;++j)mark({-600.f,shep::FALL_Y,-4040.f},8.f+std::fmod(100.f+j*17.f-shepAtkT*28.f,100.f),HOLDFAST_STEEL*.5f);
    // Follow the force from the planted palm to a captive mass, then into the heart.
    for(int arm=0;arm<2;++arm)if(shepPinT[arm]>0.f&&!shepCounterHit[arm]){
        float age=PIN_TIME-shepPinT[arm],u=smooth((age-.15f)/(COUNTER_HIT-.15f)),sd=arm?1.f:-1.f;
        auto source=hc+glm::vec3{sd*60.f,35.f,12.f};
        auto at=glm::mix(source,heart,u)+glm::vec3{sd*22.f*std::sin(u*3.14159265f),15.f*std::sin(u*3.14159265f),16.f*std::sin(u*3.14159265f)};
        bone(shepHandAt[arm]+glm::vec3{0,7.f,0},at,.28f,.28f);
        ellipsoid(at,{6,6,6},skin,HOLDFAST_STEEL*(.7f+u));
        for(int j=1;j<=4;++j){float trail=std::max(0.f,u-j*.055f);auto p=glm::mix(source,heart,trail)+glm::vec3{sd*22.f*std::sin(trail*3.14159265f),15.f*std::sin(trail*3.14159265f),16.f*std::sin(trail*3.14159265f)};
            ellipsoid(p,glm::vec3{2.6f-j*.4f},skin,HOLDFAST_STEEL*(.6f-j*.1f));}
    }
    if(dead&&shepFinT>shep::FIN_DRAIN){float sink=(shepFinT-shep::FIN_DRAIN)*9.f;for(size_t i=first;i<out.size();++i)out[i].model=glm::translate(glm::mat4{1.f},glm::vec3{0,-sink,0})*out[i].model;}
}
inline void GameplayState::headMarkers(const glm::mat4& view,const glm::mat4& proj) {
    if(shepHeadHp<=0.f||shepHeadRise<.99f)return;
    float sx,sy;char buf[80];
    if(shepOpenT>0.f){const auto at=glm::vec3(shepBodyPose()*glm::vec4(shephead::chest(1.f)+glm::vec3{0,26,30},1));bool on=projectToScreen(at,view,proj,sx,sy);snprintf(buf,sizeof(buf),"COUNTERWEIGHT  %dS",(int)std::ceil(shepOpenT));ui.renderMarker(sx,sy,on,HOLDFAST_STEEL,buf);}
    for(int k=0;k<2;++k)if(shepArmDown[k]>0.f){bool on=projectToScreen(shepHandAt[k]+glm::vec3{0,7,0},view,proj,sx,sy);
        snprintf(buf,sizeof(buf),shepPinT[k]>0.f?"PINNED":"PALM: HOLDFAST (%s)",binds().label(Act::HOLDFAST).c_str());ui.renderMarker(sx,sy,on,shepPinT[k]>0.f?HOLDFAST_STEEL:glm::vec3{1,.7f,.2f},buf);}
}
inline const char* GameplayState::headObjective() {
    using namespace shephead;static char line[120];
    const char* step="HIT ITS FACE, HEART OR GROUNDED HANDS";
    if(shepRiseT>=0.f)step="THE SHEPHERD RISES";
    else if(shepRearT>=0.f)step="THE ORBIT IS BREAKING";
    else if(shepAtk==SLAM||shepAtk==SLAM2)step="DODGE THE PALM OR PARRY THE WHITE FLASH";
    else if(shepGrabT>=0.f)step="BREAK THE GRIP: FIRE, PUNCH OR HOLDFAST";
    else if(shepAtk==SWEEP)step=shepVariation?"CROSS-SWEEP: MOVE BETWEEN THE TWO LANES":"ARM SWEEP: DASH FORWARD OR BACK";
    else if(shepAtk==GRAB)step="DODGE SIDEWAYS OR PARRY THE CLOSING HAND";
    else if(shepAtk==GAZE)step="MOVE WHEN ITS GAZE LOCKS. AVOID THE BURNING HULL";
    else if(shepAtk==ECHO)step="LEAVE YOUR MARKS. JUMP THE AFTERSHOCKS";
    else if(shepAtk==BEAM)step="JUMP THE GAZE OR TAKE COVER BEHIND A FIN";
    else if(shepAtk==ORRERY)step="PARRY THE WHITE MOONS BACK INTO ITS HEART";
    else if(shepAtk==PULL)step="HOLDFAST TO THE HULL. JUMP THE RINGS";
    else if(shepAtk==AIM)step="LEAVE THE RED CONE BEFORE ZERO";
    else if((shepPinT[0]>0.f&&!shepCounterHit[0])||(shepPinT[1]>0.f&&!shepCounterHit[1]))step="PALM HELD. ITS ORBIT IS STRIKING THE HEART";
    else if(shepOpenT>0.f)step="COUNTERWEIGHT: ITS BODY TAKES EXTRA DAMAGE";
    snprintf(line,sizeof(line),"%d/3  /  %s",shepHeadPhase+1,step);return line;
}
