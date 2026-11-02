#pragma once
// The final ward is an exposed assessment platform, not another hospital bay.
// Included after namespace lab's footprint/wayfinding helpers are defined.
inline void buildFinalAssessment(LevelBuilder& b,glm::vec3 c,float entry){
    using glm::vec3;auto& l=b.L;auto& s=l.story;float y=c.y,z=c.z;
    const vec3 ivory{.69f,.72f,.72f},black{.055f,.065f,.095f},steel{.18f,.24f,.29f},red{1.8f,.08f,.22f},cyan{.25f,.85f,1.2f};
    auto solid=[&](vec3 a,vec3 d,vec3 color){return b.wall(a.x,a.y,a.z,d.x,d.y,d.z,color);};
    b.mat=Mat::CLINIC;
    auto bays=lab::footprint(9);
    l.basins.push_back({LevelBuilder::aabb(-78,y-300,z-74,78,y+115,z+42),y-250});
    for(int n=0;n<(int)bays.size();++n){const auto& bay=bays[n];
        solid(c+vec3{bay.left,-1.2f,bay.farZ},c+vec3{bay.right,0,bay.nearZ},ivory);
        for(float x:{bay.left,bay.right}){
            solid(c+vec3{x-.22f,0,bay.farZ},c+vec3{x+.22f,1.1f,bay.nearZ},steel);
            b.neon(x-.06f,y+1.12f,z+bay.farZ,x+.06f,y+1.18f,z+bay.nearZ,cyan*.4f);
        }
        if(n){const auto& prev=bays[n-1];for(auto pair:{glm::vec2{prev.left,bay.left},glm::vec2{prev.right,bay.right}})
            if(pair.x!=pair.y)solid(c+vec3{std::min(pair.x,pair.y),0,bay.nearZ-.22f},c+vec3{std::max(pair.x,pair.y),1.1f,bay.nearZ+.22f},steel);}
    }
    // Low threshold walls only: every side and the entire roof reveal space.
    b.wallX(-44,44,z+39.7f,z+40.3f,y,y+2,steel,{{entry-6,entry+6,y,y+2}});
    b.wallX(-54,54,z-72.3f,z-71.7f,y,y+2,steel,{{-7,7,y,y+2}});
    for(float x:{entry-7,entry+7})solid({x-.55f,y,z+39.3f},{x+.55f,y+13,z+40.7f},black);
    solid({entry-7.5f,y+13,z+39.3f},{entry+7.5f,y+14,z+40.7f},ivory);
    for(float x:{-8.f,8.f})solid(c+vec3{x-.6f,0,-72.7f},c+vec3{x+.6f,15,-71.3f},black);
    b.kit().arch(c+vec3{0,15,-72},16,6,.8f,1.4f,ivory,0,24);
    lab::stencil(b,"FINAL ASSESSMENT",{entry-6,y+11.5f,z+39.25f},.13f);
    lab::stencil(b,"RELEASE DENIED",{-7,y+12,z-71.25f},.15f,{.65f,.09f,.14f});
    int door=b.doorway(true,-6.8f,6.8f,z-72.3f,z-71.7f,y,8,red,true);
    int gate=b.doorway(true,entry-5.8f,entry+5.8f,z+39.7f,z+40.3f,y,7,cyan,false);
    // Two flanking galleries, each with an approach and a return ramp.
    b.mat=Mat::METAL;
    for(float side:{-1.f,1.f}){
        float x=side*62;
        solid(c+vec3{x-8,7.2f,-34},c+vec3{x+8,8,-6},steel);
        for(auto run:{glm::vec4{-6,26,8,0},glm::vec4{-46,-34,0,8}}){
            auto area=LevelBuilder::aabb(x-5,y,z+run.x,x+5,y+8,z+run.y);l.ramps.push_back({area,2,y+run.z,y+run.w});
            b.kit().box(c+vec3{x,(run.z+run.w)*.5f-.3f,(run.x+run.y)*.5f},{std::hypot(run.y-run.x,run.w-run.z),.6f,10},steel,-1.5707963f,0,std::atan2(run.w-run.z,run.y-run.x));
        }
        b.neon(x-7,y+8.04f,z-33,x+7,y+8.1f,z-32.8f,cyan*.35f);
        for(float dz:{-28.f,-13.f}){
            solid(c+vec3{x+side*7-.15f,8,dz-3},c+vec3{x+side*7+.15f,9.1f,dz+3},steel);
        }
        l.placedPickups.push_back({c+vec3{side*42,0,6},1});
    }
    // A wide uninterrupted combat floor between the galleries. Fine rings
    // and spokes give distance cues without filling the floor with obstacles.
    auto centre=c+vec3{0,0,-10};
    for(float radius:{14.f,28.f,42.f})b.kit().curve(centre,radius,0,6.2831853f,.018f,.045f,.13f,steel,72);
    for(int k=0;k<32;++k){float angle=k*.19634954f;
        b.kit().box(centre+vec3{std::cos(angle)*43,.04f,std::sin(angle)*43},{1.8f,.035f,.17f},steel,-angle);
    }
    // Distant split pylons and broken diagnostic frames establish the scale.
    // These are beyond the playable boundary and never block combat sightlines.
    for(int k=0;k<10;++k){float a=k*.6283185f;auto p=centre+vec3{std::cos(a)*104,-28,std::sin(a)*104};
        float h=48.f+(k%3)*15;
        b.kit().box(p+vec3{0,h*.5f,0},{5,h,9},black,a+.2f,0,(k%2?.12f:-.12f));
        b.kit().box(p+vec3{0,h+9,0},{5,10,9},ivory,a+.5f,.1f,-.2f);
        b.kit().box(p+vec3{0,h*.6f,4.6f},{.3f,h*.55f,.12f},red*.25f,a+.2f);
        b.kit().box(p+vec3{0,h+20,0},{10,1.2f,16},steel,a+.35f,.15f);
    }
    for(int k=0;k<3;++k){
        auto p=centre+vec3{0,30.f+k*21.f,-15};
        b.kit().arch(p,118.f-k*18,26,1.3f,2.2f,ivory,k*.32f,36);
    }
    // Readable centre landmark and a final optional record above the floor.
    auto terminal=c+vec3{68,8,-24};
    solid(terminal+vec3{-.8f,0,-.45f},terminal+vec3{.8f,1.1f,.45f},steel);
    b.neon(terminal.x-.6f,terminal.y+1.2f,terminal.z+.48f,terminal.x+.6f,terminal.y+1.65f,terminal.z+.5f,cyan*.4f);
    s.terminals.push_back({terminal,"FINAL BODY / RELEASE","SUCCESSOR: CROSSING STABLE. RELEASE AUTHORISATION REVOKED. DO NOT TELL THE SUBJECT."});
    Arena a;a.name=lab::name(9);a.subtitle=lab::objective(9);
    a.zone=LevelBuilder::aabb(-76,y-300,z-73,76,y+110,z+41);
    a.bounds=LevelBuilder::aabb(-74,y,z-70,74,y+40,z+38);
    a.playerStart={entry,y,z+31};a.startYaw=-90;a.entryGate=gate;a.exitDoor=door;
    a.voidY=y-15;a.hasRespawn=true;a.respawn={entry,y,z+29};a.hasTrigger=true;
    a.trigger=LevelBuilder::aabb(-72,y-1,z-68,72,y+18,z+18);
    a.bossSpawn=c+vec3{0,0,-20};a.waves={{WaveEntry(EnemyType::SUCCESSOR,1)}};a.maxAlive=1;a.damageScale=1.f;
    a.theme=lab::theme(9);a.theme.fogDensity=.001f;a.theme.skyAmb={.3f,.28f,.4f};a.theme.groundAmb={.18f,.16f,.22f};
    a.ambient=Ambient::MOTES;a.space=ReverbSpace::HALL;a.tone=RoomTone::CELESTIAL;
    l.arenas.push_back(a);
}
