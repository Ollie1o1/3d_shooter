#pragma once
// The spine deforms the actual torso mesh. Head, shoulder roots, heart and
// collision volumes use the same weighted spine transforms as that mesh.
inline glm::mat4 GameplayState::shepRootPose() const {
    auto c=shephead::chest(shepHeadRise);
    auto m=glm::translate(glm::mat4{1.f},c+shepBodyOffset);
    m=glm::rotate(m,shepBodyAngles.y,glm::vec3{0,1,0});
    m=glm::rotate(m,shepBodyAngles.z,glm::vec3{0,0,1});
    return glm::translate(m,-c);
}
inline glm::mat4 GameplayState::shepSpinePose(float weight) const {
    const auto c=shephead::chest(shepHeadRise)+glm::vec3{0,-52.5f,0};
    auto m=glm::translate(glm::mat4{1.f},c);
    m=glm::rotate(m,shepSpine.y*weight,glm::vec3{0,1,0});
    m=glm::rotate(m,shepSpine.x*weight,glm::vec3{1,0,0});
    m=glm::scale(m,glm::vec3{1.f+shepBreath*.025f*weight,1.f,1.f+shepBreath*.06f*weight});
    return glm::translate(m,-c);
}
inline glm::mat4 GameplayState::shepBodyPose() const {return shepRootPose()*shepSpinePose();}
inline glm::mat4 GameplayState::shepChestPose() const {return shepRootPose()*shepSpinePose(shephead::smooth(.9f/1.35f));}
inline glm::mat4 GameplayState::shepFacePose() const {
    using namespace shephead;
    const auto body=shepBodyPose();
    const auto neck=glm::vec3(body*glm::vec4(headC(shepHeadRise)+glm::vec3{0,-34,0},1));
    auto to=shepLookAt-neck;if(glm::length(to)<1.f)to=glm::vec3{0,-.5f,1};
    const float yaw=glm::clamp(std::atan2(to.x,to.z),-1.05f,1.05f);
    const float pitch=glm::clamp(-std::atan2(to.y,std::max(1.f,glm::length(glm::vec2(to.x,to.z)))),.08f,.95f);
    auto m=glm::translate(glm::mat4{1.f},neck);
    m=glm::rotate(m,yaw,glm::vec3{0,1,0});
    m=glm::rotate(m,pitch,glm::vec3{1,0,0});
    m=glm::rotate(m,shepHeadHp<=0.f?.26f:-shepBodyAngles.z*.4f,glm::vec3{0,0,1});
    return glm::translate(m,glm::vec3{0,34,0});
}
inline glm::vec3 GameplayState::shepEyePose(int side) const {
    // Anatomical eyeball centres, authored in the head's source coordinates.
    return glm::vec3(shepFacePose()*glm::vec4{side?8.0668f:-8.0668f,3.203328f,22.961312f,1});
}
inline glm::vec3 GameplayState::shepGazeMark(int shot) const {
    auto at=shepAttackAt+glm::vec3{shot==1?-12.f:shot==2?12.f:0.f,0,0};
    at.x=glm::clamp(at.x,shep::FALL_X0+3.f,shep::FALL_X1-3.f);return at;
}
inline glm::vec3 GameplayState::shepMouthPose() const {return glm::vec3(shepFacePose()*glm::vec4{0,-16,29,1});}
inline glm::vec3 GameplayState::shepHandPose(int k) const {
    if(shepArmDown[k]>0.f||shepPinT[k]>0.f)return shepHandAt[k]+glm::vec3{0,2,0};
    return shepPoseReady?shepPoseHand[k]:shephead::handRest(k,shepHeadRise);
}
inline void GameplayState::animateShepherd(float dt) {
    using namespace shephead;
    const float clock=shepMotionClock+=dt;
    auto curve=[](glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec3 d,float t){float u=1.f-t;return a*(u*u*u)+b*(3.f*u*u*t)+c*(3.f*u*t*t)+d*(t*t*t);};
    const float blend=1.f-std::exp(-dt*5.f);
    auto watch=player.camera.position;
    if(shepAtk==GAZE&&shepAtkT>=GAZE_TRACK)watch=shepGazeMark(gazeIndex(shepAtkT,shepVariation))+glm::vec3{0,1,0};
    if(!shepPoseReady)shepLookAt=watch;
    shepLookAt=glm::mix(shepLookAt,watch,1.f-std::exp(-dt*4.f));
    const float breath=std::sin(clock*1.05f);
    shepBreath=breath;
    glm::vec3 angles{0,.065f*std::sin(clock*.43f),.035f*std::sin(clock*.61f+.7f)};
    angles.y+=glm::clamp((shepGazeX+600.f)/650.f,-.13f,.13f);
    glm::vec3 spine{.11f+.045f*breath,.065f*std::sin(clock*.55f),0};
    glm::vec3 offset{5.f*std::sin(clock*.43f),3.5f*breath,9.f+4.f*std::sin(clock*.37f)};
    const bool slam=shepAtk==SLAM||shepAtk==SLAM2;
    if(slam){float wind=smooth(shepAtkT/1.15f),hit=smooth((shepAtkT-1.25f)/.4f);
        spine.x+=-.17f*wind+.32f*hit;spine.y+=(shepSlamArm?-.13f:.13f)*wind*(1.f-hit);
        offset.z+=36.f*hit;offset.y-=5.f*hit;}
    if(shepAtk==SWEEP){float u=glm::clamp((shepAtkT-SWEEP_TELL)/SWEEP_TRAVEL,0.f,1.f);
        if(shepVariation&&shepAtkT>SWEEP_TELL+SWEEP_TRAVEL+.35f)u=1.f-glm::clamp((shepAtkT-SWEEP_TELL-SWEEP_TRAVEL-.35f)/SWEEP_TRAVEL,0.f,1.f);
        float side=shepSlamArm?1.f:-1.f;
        spine.y+=side*(.25f-.53f*smooth(u))*smooth(shepAtkT/.7f);spine.x+=.09f*std::sin(u*3.14159265f);
        offset.x+=side*(8.f-16.f*smooth(u));offset.z+=34.f*std::sin(u*3.14159265f);}
    if(shepAtk==GRAB){float reach=smooth((shepAtkT-GRAB_LOCK)/(GRAB_REACH-GRAB_LOCK));
        float recoil=smooth((shepAtkT-GRAB_CLOSE)/1.3f);spine.x+=.23f*reach-.20f*recoil;
        spine.y+=(shepSlamArm?-.12f:.12f)*reach;offset.z+=42.f*reach*(1.f-recoil);}
    if(shepAtk==ORRERY){spine.x-=.12f*std::sin(glm::clamp(shepAtkT/3.1f,0.f,1.f)*3.14159265f);}
    if(shepAtk==PULL){float gather=smooth(shepAtkT/PULL_LEN);spine.x-=.15f*gather;offset.y+=7.f*gather;}
    // A pinned hand strains against its captive orbit before the mass strikes home.
    for(int k=0;k<2;++k)if(shepPinT[k]>0.f&&!shepCounterHit[k]){
        float strain=smooth((PIN_TIME-shepPinT[k])/COUNTER_HIT),sd=k?1.f:-1.f;
        spine.y+=sd*.12f*strain;offset.x+=sd*8.f*strain;
    }
    if(shepCounterT>0.f){float recoil=std::sin(3.14159265f*(1.f-shepCounterT/COUNTER_RECOIL));
        spine.x-=.22f*recoil;offset.z-=24.f*recoil;offset.y+=4.f*recoil;
    }
    if(shepRearT>=0.f){float recoil=std::sin(3.14159265f*glm::clamp(shepRearT/REAR,0.f,1.f));spine.x-=.24f*recoil;offset.z-=22.f*recoil;}
    shepBodyAngles=glm::mix(shepBodyAngles,angles,blend);shepSpine=glm::mix(shepSpine,spine,blend);shepBodyOffset=glm::mix(shepBodyOffset,offset,blend);
    const auto body=shepBodyPose();
    for(int k=0;k<2;++k){
        const float sd=k?1.f:-1.f,phase=clock+k*2.1f;
        auto rest=glm::vec3(body*glm::vec4(handRest(k,shepHeadRise),1))
            +glm::vec3{6.f*std::sin(phase*.72f),7.f*std::sin(phase*.9f),8.f*std::cos(phase*.57f)};
        if(!shepPoseReady){shepPoseHand[k]=rest;shepPoseStart[k]=rest;shepRecoverT[k]=2.f;}
        if(shepPoseAttack!=shepAtk)shepPoseStart[k]=shepPoseHand[k];
        const bool planted=shepArmDown[k]>0.f||shepPinT[k]>0.f;
        float curl=1.05f+.16f*std::sin(phase*.7f),tilt=.12f;
        if(planted){shepPoseHand[k]=shepHandAt[k]+glm::vec3{0,2,0};curl=0.f;tilt=0.f;}
        else if(slam&&(shepAtk==SLAM2||shepSlamArm==k)){
            const int a=shepAtk==SLAM2?k:0;const float tAtk=shepAtkT-(shepAtk==SLAM2&&k==1?.42f:0.f);
            const auto target=shepSlamAt[a]+glm::vec3{0,2,0};const auto high=target+glm::vec3{sd*6.f,48.f,-9.f};
            if(tAtk<SLAM_TELL-.3f){float t=smooth(tAtk/(SLAM_TELL-.3f));shepPoseHand[k]=curve(shepPoseStart[k],shepPoseStart[k]+glm::vec3{sd*16.f,34.f,-12.f},high+glm::vec3{sd*9.f,14.f,-15.f},high,t);}
            else{float t=glm::clamp((tAtk-SLAM_TELL+.3f)/.3f,0.f,1.f);shepPoseHand[k]=glm::mix(high,target,t*t*t);}
            curl=1.25f*smooth(tAtk/.8f);tilt=-.4f*(1.f-smooth((tAtk-1.3f)/.35f));
        }else if(shepAtk==SWEEP&&k==shepSlamArm){
            float x=sweepX(shepAtkT,shepSlamArm,shep::FALL_X0-14.f,shep::FALL_X1+14.f);
            auto target=glm::vec3{x,shep::FALL_Y+6.f,shepSweepZ};
            if(shepVariation==1){float change=smooth((shepAtkT-SWEEP_TELL-SWEEP_TRAVEL)/.35f);target.z=glm::mix(shepSweepZ,shepAttackAt.z,change);}
            if(shepVariation==1&&shepAtkT>SWEEP_TELL+SWEEP_TRAVEL+.35f)
                target.x=sweepX(shepAtkT-SWEEP_TRAVEL-.35f,1-shepSlamArm,shep::FALL_X0-14.f,shep::FALL_X1+14.f);
            if(shepAtkT<SWEEP_TELL){float t=smooth(shepAtkT/SWEEP_TELL);shepPoseHand[k]=curve(shepPoseStart[k],shepPoseStart[k]+glm::vec3{sd*20,30,-15},target+glm::vec3{0,15,-15},target,t);}
            else shepPoseHand[k]=target;
            curl=.55f;tilt=0.f;
        }else if(shepAtk==GRAB&&k==shepSlamArm){
            if(shepAtkT<GRAB_LOCK)shepPoseHand[k]=glm::mix(shepPoseStart[k],shepGrabFrom,smooth(shepAtkT/GRAB_LOCK));
            else if(shepAtkT<GRAB_CLOSE)shepPoseHand[k]=grabPath(shepGrabFrom,shepAttackAt,shepAtkT);
            else {auto draw=glm::vec3(body*glm::vec4(chest(shepHeadRise)+glm::vec3{sd*42,20,88},1));
                shepPoseHand[k]=glm::mix(shepAttackAt+glm::vec3{0,3,0},draw,smooth((shepAtkT-GRAB_CLOSE)/1.3f));}
            curl=shepAtkT<GRAB_REACH?.03f:1.45f*smooth((shepAtkT-GRAB_REACH)/.4f);
        }else{
            if(shepPosePlanted[k]){shepRecoverFrom[k]=shepPoseHand[k];shepRecoverT[k]=0.f;}
            shepRecoverT[k]+=dt;auto target=rest;
            if(slam){target=glm::vec3(body*glm::vec4(chest(shepHeadRise)+glm::vec3{sd*86,-2,68},1));curl=.9f;}
            if(shepAtk==ECHO&&shepEchoN>0){int n=std::min(shepEchoHit,shepEchoN-1);target=glm::mix(rest,shepEchoAt[n]+glm::vec3{sd*20.f,32.f,-5.f},.8f);curl=.15f;}
            if(shepAtk==ORRERY){target+=glm::vec3{-sd*30.f,28.f,-6.f};curl=.12f;tilt=.2f;}
            if(shepAtk==PULL){float g=smooth(shepAtkT/PULL_LEN);target+=glm::vec3{-sd*60.f,25.f,-30.f}*g;curl=.1f+1.1f*g;}
            if(shepRecoverT[k]<1.3f){float t=smooth(shepRecoverT[k]/1.3f);shepPoseHand[k]=curve(shepRecoverFrom[k],shepRecoverFrom[k]+glm::vec3{sd*10.f,36.f,0},target+glm::vec3{sd*14.f,12.f,12.f},target,t);}
            else shepPoseHand[k]=glm::mix(shepPoseHand[k],target,1.f-std::exp(-dt*4.f));
        }
        shepPosePlanted[k]=planted;shepFingerCurl[k]+=(curl-shepFingerCurl[k])*(1.f-std::exp(-dt*10.f));shepWristTilt[k]+=(tilt-shepWristTilt[k])*(1.f-std::exp(-dt*9.f));
        auto sh=glm::vec3(body*glm::vec4(shoulder(k,shepHeadRise),1));auto el=elbowFrom(k,sh,shepPoseHand[k]);
        auto orientation=glm::quat_cast(glm::mat3(propdraw::along(glm::vec3{0},shepPoseHand[k]-el)));
        float flat=planted?1.f:slam&&(shepAtk==SLAM2||shepSlamArm==k)?smooth((shepAtkT-1.2f)/.4f):shepAtk==SWEEP&&k==shepSlamArm?1.f:0.f;
        orientation=glm::slerp(orientation,glm::angleAxis(3.14159265f,glm::vec3{0,1,0}),flat);
        if(!shepPoseReady)shepPalmRotation[k]=orientation;
        else shepPalmRotation[k]=glm::slerp(shepPalmRotation[k],orientation,1.f-std::exp(-dt*13.f));
    }
    shepPoseReady=true;shepPoseAttack=shepAtk;
    updateShepherdMotionAudio(dt);
}
