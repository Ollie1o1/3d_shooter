#pragma once
// The final ward is an exposed assessment platform, not another hospital bay.
// Included after namespace lab's footprint/wayfinding helpers are defined.

// OATHBREAKER's statue, at the back left of the floor: a stone giant kneeling on a dais
// under a broken arch, head bowed, its hands resting either side of the sword's grip,
// the sword plunged into a cracked block in front of it. The sword itself (and its seal)
// is drawn live by GameplayState::gatherSwordStatue: it goes when you pull it.
namespace lab {
inline glm::vec3 swordStatue(){return centre(9)+glm::vec3{-40,0,-60};}   // the dais's centre on the floor; the statue faces +Z, the floor
inline glm::vec3 swordStatueGrip(){return swordStatue()+glm::vec3{0,2.35f,1.6f};}
}
inline void buildSwordStatue(LevelBuilder& b,glm::vec3 S){
    using glm::vec3;auto& l=b.L;
    const vec3 stone{.86f,.88f,.86f},dark{.1f,.105f,.13f},seam{.25f,.85f,1.2f},rust{1.4f,.07f,.18f};
    auto hidden=[&](vec3 a,vec3 d){int w=b.wall(a.x,a.y,a.z,d.x,d.y,d.z,dark);l.walls[w].hidden=true;};
    b.mat=Mat::CLINIC;
    // The dais: a low black step, a rim of light round it
    b.wall(S.x-3.6f,S.y,S.z-3.6f,S.x+3.6f,S.y+.4f,S.z+3.6f,dark);
    b.neon(S.x-3.62f,S.y+.4f,S.z+3.58f,S.x+3.62f,S.y+.44f,S.z+3.64f,seam*.45f);
    for(float sx:{-1.f,1.f})b.neon(S.x+sx*3.6f-.03f,S.y+.4f,S.z-3.6f,S.x+sx*3.6f+.03f,S.y+.44f,S.z+3.6f,seam*.3f);
    lab::stencil(b,"OATHBREAKER",S+vec3{-1.48f,.37f,3.605f},.045f,{.65f,.09f,.14f});
    // The block it is plunged into: cracked, the cracks glowing
    b.wall(S.x-.9f,S.y+.4f,S.z+.9f,S.x+.9f,S.y+1.3f,S.z+2.3f,stone);
    for(int k=0;k<5;++k){float a=k*1.2566f+.3f;
        vec3 p=S+vec3{std::cos(a)*.45f,1.305f,1.6f+std::sin(a)*.35f};
        b.kit().box(p,{.62f,.012f,.035f},rust*.8f,-a);
    }
    b.kit().box(S+vec3{-.92f,.85f,1.85f},{.012f,.7f,.03f},rust*.6f,0,0,.25f);   // a crack running down its side
    // The giant: kneeling on its left knee, right foot planted, leaning over the sword
    {   auto k=b.kit();
        const float y0=S.y+.4f;
        auto P=[&](float x,float y,float z){return S+vec3{x,y,z};};
        k.box(P(-.9f,.85f,-1.5f),{.95f,.9f,2.4f},stone);                       // the left shin, flat on the dais
        k.rod(P(-.9f,1.f,-.4f),P(-.75f,2.7f,-1.15f),.52f,stone,8);              // the left thigh, up from the knee
        k.box(P(1.f,.7f,.55f),{.8f,.35f,1.2f},dark);                            // the right foot
        k.rod(P(1.f,.75f,.4f),P(1.f,2.25f,.3f),.45f,stone,8);                   // the right shin
        k.rod(P(1.f,2.25f,.3f),P(.8f,2.75f,-1.15f),.55f,stone,8);               // the right thigh, back to the hip
        k.column(P(1.f,2.f,.3f),.55f,.5f,dark,8);                               // the right kneecap
        k.box(P(0,2.85f,-1.15f),{2.2f,.9f,1.4f},dark);                          // the hips
        k.box(P(0,4.1f,-.7f),{2.6f,2.6f,1.5f},stone,0,.32f);                    // the chest, leaning in
        k.box(P(0,4.3f,-.02f),{.18f,1.8f,.05f},seam*.9f,0,.32f).neon=true;      // a seam of light down its sternum
        k.box(P(0,3.15f,-.55f),{1.9f,.5f,1.2f},dark,0,.25f);                    // the waist
        for(float sx:{-1.f,1.f}){
            k.box(P(sx*1.6f,5.f,-.35f),{1.25f,.95f,1.5f},dark,0,.2f,sx*-.25f);  // its pauldrons
            k.rod(P(sx*1.55f,4.75f,-.25f),P(sx*1.2f,3.45f,.6f),.4f,stone,8);    // the upper arms
            k.column(P(sx*1.2f,3.2f,.6f),.42f,.5f,dark,8);                      // the elbows
            k.rod(P(sx*1.2f,3.45f,.6f),P(sx*.42f,2.6f,1.5f),.34f,stone,8);      // the forearms, down to the grip
            k.box(P(sx*.36f,2.5f,1.6f),{.3f,.5f,.55f},stone,0,0,sx*.2f);        // its open hands, either side of the grip
        }
        k.box(P(0,5.55f,.15f),{.95f,1.05f,1.05f},stone,0,.55f);                 // its head, bowed over the sword
        k.box(P(0,5.5f,.68f),{.62f,.07f,.04f},seam*1.2f,0,.55f).neon=true;      // the visor slit, still lit
        k.box(P(0,6.25f,-.2f),{.2f,.5f,1.2f},dark,0,.55f);                      // a crest
        (void)y0;
    }
    // The broken arch over it, and two black obelisks with seams of light
    b.kit().arch(S+vec3{0,.4f,-3.f},7.4f,8.f,.55f,1.f,stone,0,18);
    b.kit().box(S+vec3{2.9f,8.6f,-3.f},{1.6f,.6f,1.05f},stone,0,0,-.6f);       // a fallen piece of it, wedged
    for(float sx:{-1.f,1.f}){   // (behind it, either side: they frame it, never hide it)
        b.kit().box(S+vec3{sx*3.f,3.f,-2.4f},{.7f,5.2f,.7f},dark);
        b.kit().box(S+vec3{sx*3.f,3.f,-2.04f},{.08f,4.6f,.04f},seam*.7f).neon=true;
        hidden(S+vec3{sx*3.f-.35f,0,-2.75f},S+vec3{sx*3.f+.35f,5.6f,-2.05f});
    }
    // Light up out of the dais onto it, and seams of light along its limbs
    for(float sx:{-1.f,1.f})b.neon(S.x+sx*.95f-.03f,S.y+.4f,S.z+.85f,S.x+sx*.95f+.03f,S.y+.46f,S.z+2.35f,rust*.7f);
    b.neon(S.x-.95f,S.y+.4f,S.z+2.32f,S.x+.95f,S.y+.46f,S.z+2.38f,rust*.7f);
    {   auto k=b.kit();k.neon=true;
        for(float sx:{-1.f,1.f}){
            k.rod(S+vec3{sx*1.8f,4.75f,-.25f},S+vec3{sx*1.45f,3.45f,.6f},.05f,seam*.8f,4);
            k.rod(S+vec3{sx*1.45f,3.45f,.6f},S+vec3{sx*.62f,2.7f,1.45f},.05f,seam*.8f,4);
        }
        k.rod(S+vec3{1.45f,.85f,.4f},S+vec3{1.45f,2.2f,.3f},.04f,seam*.6f,4);
    }
    // What you bump into: the giant (the dais and the block are walls already)
    hidden(S+vec3{-2.1f,.4f,-2.7f},S+vec3{2.1f,6.4f,.2f});
    hidden(S+vec3{-1.3f,.4f,.2f},S+vec3{1.3f,3.2f,.9f});
}
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
    buildSwordStatue(b,lab::swordStatue());
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
