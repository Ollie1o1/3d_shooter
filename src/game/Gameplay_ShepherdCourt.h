#pragma once
// Authored changes to the court, reconstructed from the checkpoint's movement.
// Playable floors and shelters remain intact; each broken clamp adds a route.
inline void GameplayState::updateShepherdCourt(float dt) {
    shepCourtT=std::min(99.f,shepCourtT+dt);
    for(float& t:shepReleaseT)if(t>=0.f)t+=dt;
    shepCounterT=std::max(0.f,shepCounterT-dt);
    for(int k=0;k<2;++k){
        shepPinT[k]=std::max(0.f,shepPinT[k]-dt);
        shepArmDown[k]=std::max(0.f,shepArmDown[k]-dt);
        if(shepPinT[k]<=0.f||shepCounterHit[k]||shephead::PIN_TIME-shepPinT[k]<shephead::COUNTER_HIT)continue;
        shepCounterHit[k]=true;
        audio.stop(shepStrainSound[k],.012f);shepStrainSound[k]=0;
        if(shepHeadHp<=0.f||shepRearT>=0.f)continue;
        shepCounterT=shephead::COUNTER_RECOIL;
        const auto heart=glm::vec3(shepChestPose()*glm::vec4(shephead::chest(shepHeadRise)+glm::vec3{0,5,26},1));
        shepHeadHurt(shephead::PIN_DAMAGE);
        // A phase boundary closes the opening; never carry a pending hit into it.
        if(shepRearT<0.f&&shepHeadHp>0.f)shepOpenT=shephead::OPEN;
        fx.spawnBurst(heart,POST_LIT,65,20.f,1.f,3.f);
        fx.spawnShockwave(heart,28.f,POST_LIT);
        shepherdSound("shep_counter",heart);
        shake(.65f,.065f);
        pushBanner("COUNTERWEIGHT","ITS OWN ORBIT STRIKES ITS HEART",HOLDFAST_STEEL,1.8f);
    }
}

inline void GameplayState::gatherShepherdCourt(std::vector<BoxInstance>& out) {
    using namespace rig;
    const glm::vec3 hull{.11f,.115f,.125f},brass{.28f,.22f,.13f};
    for(int side=0;side<(int)level.shepCourtClamps.size();++side){
        const bool broken=shepHeadPhase>side||shepHeadHp<=0.f;
        const float sd=shep::courtSide(side);
        auto centre=shep::courtClamp(side);
        auto M=T(centre);
        if(broken){
            // Only the latest break flies on screen; a checkpoint retains the wreckage.
            const float t=shepHeadPhase==side+1?shepCourtT:99.f;
            const float u=shephead::smooth(t/2.2f),arc=std::sin(3.14159265f*u);
            centre=glm::mix(centre,glm::vec3{-600.f+sd*105.f,shep::FALL_Y-12.f,-3998.f-side*27.f},u)+glm::vec3{0,20.f*arc,0};
            M=T(centre)*RZ(sd*.65f*u)*RX(.3f*arc);
        }
        // Split jaws and a captive brass ring make these read as orbital clamps.
        push(out,M*S({2.8f,8.f,12.f}),hull);
        for(float z:{-4.f,4.f})push(out,M*T({0,0,z})*S({3.2f,9.f,1.f}),brass);
        for(int j=0;j<24;++j){float a=j*6.2831853f/24;
            push(out,M*T({1.7f,3.5f*std::sin(a),3.5f*std::cos(a)})*RX(-a)*S({.22f,.22f,.92f}),brass);
        }
        // The opened crossover has a low, steady running light along each edge.
        if(broken)for(float dz:{-5.6f,5.6f})
            push(out,T({-600.f+sd*68.5f,shep::FALL_Y+3.04f,shep::COURT_CROSS_Z+dz})*S({16.f,.06f,.14f}),brass,HOLDFAST_STEEL*.35f);
    }
    // Freed capsules remain visible beyond the court after each movement.
    const int freed=shepHeadPhase;
    for(int group=0;group<freed;++group)for(int k=0;k<7;++k){
        float n=float(k),sd=group%2?1.f:-1.f;
        float drift=std::max(0.f,shepReleaseT[group]);
        glm::vec3 at{-600.f+sd*(45.f+n*10.f+drift*1.1f),shep::FALL_Y+13.f+n*5.f+drift*.6f,-4060.f-group*30.f-n*5.f};
        auto M=T(at)*RZ(sd*(.15f*n+drift*.035f))*RY(n*.7f);
        push(out,M*S({2.4f,2.2f,4.8f}),glm::vec3{.4f,.39f,.36f});
        push(out,M*T({0,1.15f,0})*S({.45f,.12f,.7f}),hull,glm::vec3{1.1f,.55f,.14f});
    }
    // Dark stress fractures remain in the plating; they are not holes or hazards.
    for(int stage=0;stage<std::min(shepHeadPhase,2);++stage){
        const float sd=shep::courtSide(stage);
        for(int k=0;k<8;++k){
            glm::vec3 at{-600.f+sd*(22.f+k*7.f),shep::FALL_Y+.065f,-4008.f+4.f*k+(k%2?2.f:-2.f)};
            push(out,T(at)*RY(sd*.8f)*S({.4f,.04f,9.f}),glm::vec3{.04f});
        }
    }
}
