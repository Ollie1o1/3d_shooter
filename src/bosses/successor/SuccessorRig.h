#pragma once
// Included inside namespace rig by EnemyModel.h. A dedicated, sprung fighting
// stance: hips lead the shoulders; each strike has a chamber and follow-through.
struct SuccessorPose {
    float hipYaw=0,torsoYaw=0,lean=.12f,roll=0,head=0;
    float shR=-.7f,splayR=-.15f,elR=1.2f,shL=-.95f,splayL=.15f,elL=1.4f;
    float hipR=.1f,kneeR=.3f,hipL=-.28f,kneeL=.5f,handR=0,handL=0,lift=0;
};
static float SuccessorPose::* const SU_CH[]={
    &SuccessorPose::hipYaw,&SuccessorPose::torsoYaw,&SuccessorPose::lean,&SuccessorPose::roll,&SuccessorPose::head,
    &SuccessorPose::shR,&SuccessorPose::splayR,&SuccessorPose::elR,&SuccessorPose::shL,&SuccessorPose::splayL,&SuccessorPose::elL,
    &SuccessorPose::hipR,&SuccessorPose::kneeR,&SuccessorPose::hipL,&SuccessorPose::kneeL,&SuccessorPose::handR,&SuccessorPose::handL,&SuccessorPose::lift};
static_assert(sizeof(SU_CH)/sizeof(SU_CH[0])==SuccessorState::BODY_CHANNELS,"Successor body channel count");
inline SuccessorPose suMix(const SuccessorPose& a,const SuccessorPose& b,float t){
    SuccessorPose p;for(auto ch:SU_CH)p.*ch=glm::mix(a.*ch,b.*ch,t);return p;
}
inline SuccessorPose successorGuard(const Enemy& e,float time){
    SuccessorPose p;p.hipYaw=-.25f;p.torsoYaw=.18f;p.head=-.06f;
    p.lean=.16f+std::sin(time*2.6f)*.025f;p.roll=std::sin(time*1.8f)*.035f;
    float run=glm::clamp(e.suGait/18.f,0.f,1.f),s=std::sin(e.animPhase);
    // The planted leg stays long while the other knee folds through its swing.
    // Both knees bending on the old cosine beat made the whole body pogo.
    p.hipR=glm::mix(p.hipR,-.35f-s*.6f,run);p.hipL=glm::mix(p.hipL,-.35f+s*.6f,run);
    p.kneeR=glm::mix(p.kneeR,.4f+std::max(0.f,s)*.9f,run);p.kneeL=glm::mix(p.kneeL,.4f+std::max(0.f,-s)*.9f,run);
    p.torsoYaw-=s*.16f*run;p.hipYaw+=s*.15f*run;p.lean+=run*.18f;
    p.shR-=s*.28f*run;p.shL+=s*.28f*run;p.head-=run*.07f;
    return p;
}
inline SuccessorPose successorStroke(SuccessorPose p,AttackKind attack,int step,bool strike){
    float side=step%2?-1.f:1.f;
    if(attack==AttackKind::SU_INCISION){
        p.hipYaw=side*(strike?.42f:-.38f);p.torsoYaw=side*(strike?.62f:-.65f);p.lean=strike?.28f:.08f;p.roll=side*(strike?.16f:-.1f);
        if(side>0){p.shR=strike?-1.35f:-.15f;p.splayR=strike?-.15f:-1.05f;p.elR=strike?.2f:1.6f;p.shL=-.7f;p.elL=1.5f;}
        else{p.shL=strike?-1.35f:-.15f;p.splayL=strike?.15f:1.05f;p.elL=strike?.2f:1.6f;p.shR=-.7f;p.elR=1.5f;}
    }else if(attack==AttackKind::SU_THRUST || attack==AttackKind::SU_TRANSPOSE){
        p.hipYaw=strike?.3f:-.35f;p.torsoYaw=strike?.35f:-.35f;p.lean=strike?.42f:.12f;
        p.shR=strike?-1.65f:-.25f;p.elR=strike?.1f:1.9f;p.splayR=-.1f;p.shL=-1.05f;p.elL=1.5f;p.hipL=-.6f;p.kneeL=.8f;
    }else if(attack==AttackKind::SU_VECTOR){
        p.hipYaw=.1f;p.torsoYaw=-.15f;p.lean=strike?.8f:.55f;p.head=-.35f;
        p.shR=strike?.6f:.1f;p.elR=.45f;p.shL=-1.25f;p.elL=.3f;p.hipR=.65f;p.kneeR=1.2f;p.hipL=-.8f;p.kneeL=.9f;
    }else if(attack==AttackKind::SU_REAP){
        p.hipYaw=strike?1.5f:-1.2f;p.torsoYaw=strike?1.6f:-.8f;p.lean=.48f;p.roll=strike?-.3f:.2f;
        p.shR=-.45f;p.splayR=strike?-1.4f:-.5f;p.elR=.1f;p.shL=-.35f;p.splayL=1.4f;p.elL=.4f;
        p.hipR=-.7f;p.kneeR=1.3f;p.hipL=-.35f;p.kneeL=.95f;
    }else if(attack==AttackKind::SU_BREAKER){
        p.lean=strike?.65f:-.25f;p.hipYaw=-.2f;p.torsoYaw=.2f;p.head=strike?.1f:-.12f;
        p.hipR=strike?-.35f:-1.9f;p.kneeR=strike?.5f:1.05f;p.hipL=strike?-.4f:.4f;p.kneeL=strike?.9f:1.2f;
        p.shR=strike?-.4f:-2.2f;p.elR=.65f;p.shL=strike?-.3f:-1.8f;p.elL=.9f;
    }else if(attack==AttackKind::SU_SUTURE){
        p.shR=-.5f;p.shL=-.5f;p.splayR=strike?-1.5f:-.35f;p.splayL=strike?1.5f:.35f;p.elR=p.elL=strike?.2f:1.5f;p.handR=p.handL=strike?.6f:-.3f;
        p.lean=strike?-.15f:.25f;p.torsoYaw=.15f;
    }else if(attack==AttackKind::SU_RECALL){
        p.shR=strike?-1.1f:-2.4f;p.elR=strike?.15f:.7f;p.handR=strike?.65f:-.5f;p.lean=strike?.3f:-.1f;
    }else if(attack==AttackKind::SU_LATTICE){   // both hands raised, then raked down and apart
        p.shR=p.shL=strike?-.6f:-2.5f;p.splayR=strike?-1.2f:-.3f;p.splayL=strike?1.2f:.3f;p.elR=p.elL=strike?.15f:.6f;
        p.handR=p.handL=strike?.7f:-.4f;p.lean=strike?.4f:-.2f;p.head=strike?.15f:-.2f;
    }else if(attack==AttackKind::SU_CASCADE){   // a fist up, then down into the floor on one knee
        p.shR=strike?-.2f:-2.6f;p.elR=strike?.1f:.9f;p.handR=strike?.8f:-.5f;p.lean=strike?.75f:-.15f;
        p.hipR=strike?-1.1f:-.2f;p.kneeR=strike?1.5f:.4f;p.hipL=strike?.3f:-.3f;p.kneeL=strike?1.4f:.5f;p.head=strike?.25f:-.15f;
    }else if(attack==AttackKind::SU_SEVER){   // the wide backhand, flinging the fan of blades
        p.hipYaw=strike?1.1f:-.9f;p.torsoYaw=strike?1.25f:-1.05f;p.lean=.3f;p.roll=strike?-.2f:.15f;
        p.shR=-1.4f;p.splayR=strike?-1.5f:.3f;p.elR=strike?.1f:1.3f;p.shL=-.6f;p.elL=1.3f;p.hipL=-.55f;p.kneeL=.85f;
    }else if(attack==AttackKind::SU_PERFUSION){
        p.shR=p.shL=strike?-1.6f:-.9f;p.elR=p.elL=strike?.1f:1.4f;p.splayR=-.4f;p.splayL=.4f;p.lean=strike?.25f:-.12f;
    }
    return p;
}
inline bool successorCast(AttackKind a){
    return a==AttackKind::SU_SUTURE || a==AttackKind::SU_RECALL || a==AttackKind::SU_LATTICE ||
           a==AttackKind::SU_CASCADE || a==AttackKind::SU_SEVER || a==AttackKind::SU_PERFUSION;
}
inline float suEase(float x){x=glm::clamp(x,0.f,1.f);return x*x*(3.f-2.f*x);}
// Hips load first, shoulders follow, wrists arrive last. The planted back leg carries the release.
inline SuccessorPose successorCastPose(const SuccessorPose& guard,AttackKind a,float u,float remaining){
    auto chamber=successorStroke(guard,a,0,false),strike=successorStroke(guard,a,0,true);
    chamber.hipR=-.28f;chamber.kneeR=.65f;chamber.hipL=-.5f;chamber.kneeL=.8f;
    strike.hipR=.18f;strike.kneeR=.35f;strike.hipL=-.65f;strike.kneeL=.85f;
    // Separate the release silhouettes: pull the recall, split the suture, drive the barrage forward.
    if(a==AttackKind::SU_RECALL){chamber.torsoYaw=-.35f;strike.torsoYaw=.4f;strike.shL=-.8f;strike.elL=1.6f;}
    if(a==AttackKind::SU_SUTURE){chamber.torsoYaw=-.25f;strike.torsoYaw=.15f;strike.roll=-.1f;}
    if(a==AttackKind::SU_PERFUSION){chamber.lean=-.22f;chamber.elR=chamber.elL=1.75f;strike.lean=.38f;strike.head=-.12f;}
    if(a==AttackKind::SU_CASCADE){strike.hipR=-1.1f;strike.kneeR=1.5f;strike.hipL=.3f;strike.kneeL=1.4f;}
    SuccessorPose p=guard;
    const float core=suEase(u/.48f),arms=suEase((u-.08f)/.58f);
    for(int k=0;k<SuccessorState::BODY_CHANNELS;++k)
        p.*SU_CH[k]=glm::mix(guard.*SU_CH[k],chamber.*SU_CH[k],k<5 || (k>=11 && k<=14)?core:arms);
    const float stroke=a==AttackKind::SU_CASCADE?.18f:a==AttackKind::SU_SEVER?.16f:.12f;
    const float release=glm::clamp(1.f-remaining/stroke,0.f,1.f);
    const float hips=suEase(release*1.35f),hands=suEase((release-.12f)/.88f);
    for(int k=0;k<SuccessorState::BODY_CHANNELS;++k)
        p.*SU_CH[k]=glm::mix(p.*SU_CH[k],strike.*SU_CH[k],k<5 || (k>=11 && k<=14)?hips:hands);
    // A small compression under charge; no idle sway distracting from the locked aim.
    p.head-=.045f*std::sin(u*3.14159265f);return p;
}
inline SuccessorPose successorGoal(const Enemy& e,float time){
    auto guard=successorGuard(e,time),goal=guard;
    if(e.telegraphTimer>0){
        auto chamber=successorStroke(guard,e.attack,e.suStep,false),strike=successorStroke(guard,e.attack,e.suStep,true);
        float wind=glm::clamp(e.telegraphProgress()*2.5f,0.f,1.f);wind=wind*wind*(3-2*wind);
        goal=suMix(guard,chamber,wind);
        float whip=glm::clamp(1.f-e.telegraphTimer/.15f,0.f,1.f);whip=1-std::pow(1-whip,3.f);
        goal=suMix(goal,strike,whip);
        if(successorCast(e.attack))goal=successorCastPose(guard,e.attack,e.telegraphProgress(),e.telegraphTimer);
    }else if(e.suBurst>0)goal=successorStroke(guard,AttackKind::SU_VECTOR,0,true);
    else if(e.suAir>0)goal=successorStroke(guard,AttackKind::SU_BREAKER,0,e.suAir<.16f);
    const bool cast=successorCast(e.suLanded);
    const float follow=cast?.48f:.28f;
    if(e.suSince<follow && e.suLanded!=AttackKind::NONE && e.suBurst<=0 && e.suAir<=0){
        float release=e.telegraphTimer>0?.14f:cast?.46f:.24f;
        float hold=1-suEase((e.suSince-.055f)/(release-.055f));
        auto contact=cast?successorCastPose(guard,e.suLanded,1.f,0.f):successorStroke(guard,e.suLanded,e.suLandedStep,true);
        // The cast pushes back through shoulders before the elbows fold home.
        const float recoil=std::sin(glm::clamp(e.suSince/.16f,0.f,1.f)*3.14159265f);
        if(cast){contact.lean-=.12f*recoil;contact.elR+=.22f*recoil;contact.elL+=.18f*recoil;}
        goal=suMix(goal,contact,hold);
    }
    // Subsequent volleys have their own recoil; don't let the arms drop while shots are still leaving them.
    if(e.telegraphTimer<=0 && e.suLanded==AttackKind::SU_PERFUSION && e.suSince<.72f && e.suPhase>=2){
        auto fire=successorCastPose(guard,AttackKind::SU_PERFUSION,1.f,0.f);
        float pulse=std::exp(-std::fmod(std::max(0.f,e.suSince),.22f)*24.f);
        fire.lean-=.1f*pulse;fire.elR+=.28f*pulse;fire.elL+=.28f*pulse;
        goal=suMix(goal,fire,1-suEase((e.suSince-.48f)/.24f));
    }
    if(e.suPause>0){goal.shR=goal.shL=.1f;goal.elR=goal.elL=.2f;goal.lean=-.08f;goal.torsoYaw=std::sin(time)*.1f;}
    if(e.staggered()){goal=guard;goal.lean=.65f;goal.head=.35f;goal.shR=goal.shL=.35f;goal.elR=goal.elL=.3f;goal.kneeR=goal.kneeL=.85f;goal.hipR=goal.hipL=-.4f;}
    return goal;
}
inline void tickSuccessorBody(Enemy& e,float dt){
    auto p=successorGoal(e,e.age);
    if(!e.suBodyLive){for(int k=0;k<SuccessorState::BODY_CHANNELS;++k){e.suBody[k]=e.suBodyPrev[k]=p.*SU_CH[k];e.suBodyVel[k]=0;}e.suBodyLive=true;return;}
    bool strike=e.suSince<.16f || (e.telegraphTimer>0 && e.telegraphTimer<.16f);
    for(int k=0;k<SuccessorState::BODY_CHANNELS;++k){
        e.suBodyPrev[k]=e.suBody[k];float w=strike?(k<5?35.f:44.f):(k<5?24.f:22.f),decay=std::exp(-w*dt);
        float x=e.suBody[k]-p.*SU_CH[k],v=e.suBodyVel[k],c=v+w*x;
        e.suBody[k]=p.*SU_CH[k]+(x+c*dt)*decay;e.suBodyVel[k]=(v-w*c*dt)*decay;
    }
}
inline bool successorFilePose(const Enemy& e,float time,const mat4& root,mat4& fileRoot,FilePose& f,vec3& glow){
    using namespace enemyfile;auto p=successorGoal(e,time);
    if(e.suBodyLive)for(int k=0;k<SuccessorState::BODY_CHANNELS;++k)p.*SU_CH[k]=e.suBody[k];
    float sy=e.stats().height/2.65f;
    auto leg=[&](float hip,float knee){return .51f*std::cos(hip)+.46f*std::cos(hip+knee)+.13f;};
    float drop=sy*(1.1f-std::max(leg(p.hipR,p.kneeR),leg(p.hipL,p.kneeL)));
    fileRoot=root*T({0,p.lift-drop,0})*RY(p.hipYaw);f=FilePose{};
    f.motion[E_HIPR]=RX(p.hipR)*RZ(-.09f);f.motion[E_HIPL]=RX(p.hipL)*RZ(.09f);
    f.motion[E_KNEE_R]=RX(p.kneeR);f.motion[E_KNEE_L]=RX(p.kneeL);
    f.motion[E_ANKLE_R]=RX(-(p.hipR+p.kneeR));f.motion[E_ANKLE_L]=RX(-(p.hipL+p.kneeL));
    f.motion[E_WAIST]=RY(p.torsoYaw*.3f);f.motion[E_TORSO]=RY(p.torsoYaw)*RZ(p.roll)*RX(p.lean);
    f.motion[E_HEAD]=RY(-.45f*(p.torsoYaw+p.hipYaw))*RX(p.head);
    f.motion[E_ARMR]=RX(p.shR)*RZ(p.splayR);f.motion[E_ARML]=RX(p.shL)*RZ(p.splayL);
    f.motion[E_ELBOW_R]=RX(-p.elR);f.motion[E_ELBOW_L]=RX(-p.elL);
    f.motion[E_HAND_R]=RZ(p.handR);f.motion[E_HAND_L]=RZ(p.handL);
    const bool casting=(e.telegraphTimer>0 && successorCast(e.attack)) || (e.suSince<.4f && successorCast(e.suLanded));
    const float open=casting?.75f:0.f;
    f.motion[E_FINGERS_R]=RX(-.75f+open);f.motion[E_FINGERS_L]=RX(-.65f+open*.85f);
    f.motion[E_TIPS_R]=RX(-.5f+open*.6f);f.motion[E_TIPS_L]=RX(-.5f+open*.6f);
    float pulse=.8f+.2f*std::sin(time*(e.suPhase>=3?12.f:5.f));
    vec3 red{1.7f,.035f,.12f},base=red*(.55f+.18f*e.suPhase)*pulse;
    vec3 tell=SuccessorState::white(e.attack)?vec3{2.2f}:e.attack==AttackKind::SU_BREAKER?vec3{2.2f,.12f,.2f}:abGoldTell(vec3{2.f,1.3f,.15f});
    bool active=e.telegraphTimer>0 || e.suBurst>0 || e.suAir>0;
    f.chan[C_GLOW]=base;f.chan[C_SEAM]=base*.7f;f.chan[C_CORE]=e.suOpen>0 || e.staggered()?vec3{.15f,1.6f,2.f}:base*1.35f;
    f.chan[C_EYE]=active?tell:vec3{1.6f,.08f,.12f};
    f.chan[C_TELL_R]=active?tell*(.8f+e.telegraphProgress()):base;
    f.chan[C_TELL_L]=active && (e.suStep%2 || e.attack==AttackKind::SU_SUTURE || e.attack==AttackKind::SU_PERFUSION || e.attack==AttackKind::SU_LATTICE)?tell*(.8f+e.telegraphProgress()):base;
    glow=base;return true;
}

// Effects follow the animated palms, including hip twist and the sprung pose.
inline bool successorPalms(const Enemy& e,float time,vec3& right,vec3& left){
    using namespace enemyfile;
    const auto* model=enemyfile::model(e.type);if(!model)return false;
    mat4 root=T(e.position)*RY(e.yaw)*S(vec3{e.scale}),fileRoot;FilePose pose;vec3 glow;
    successorFilePose(e,time,root,fileRoot,pose,glow);
    mat4 local[E_COUNT],world[E_COUNT];int parent[E_COUNT];
    restLocals(*model,local,parent);for(auto& m:world)m=fileRoot;
    for(const auto& joint:model->joints)world[joint.id]=world[joint.parent]*local[joint.id]*pose.motion[joint.id];
    right=vec3(world[E_HAND_R]*glm::vec4{0,-.06f,.04f,1});
    left=vec3(world[E_HAND_L]*glm::vec4{0,-.06f,.04f,1});
    return true;
}
