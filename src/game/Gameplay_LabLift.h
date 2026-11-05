#pragma once
// Chapter 3's arrival set (LabIntro.h has the clock): Admissions' landing doors and the rail
// the lift climbs, always; during the arrival, the car itself, Chapter 2's tower round its
// foot and the ash plain and road below. All drawn, none of it solid: the doorway's
// collision is a hidden panel (LevelLab.h) and the camera is carried during the scene.

inline void GameplayState::gatherLabLift(rig::Rig& r){
    using namespace rig;using glm::vec3;
    auto B=[&](float x0,float y0,float z0,float x1,float y1,float z1,vec3 col,vec3 emi=vec3{0}){
        vec3 a{std::min(x0,x1),std::min(y0,y1),std::min(z0,z1)},b{std::max(x0,x1),std::max(y0,y1),std::max(z0,z1)};
        r.box(T((a+b)*.5f),{0,0,0},b-a,col,emi);
    };
    const vec3 PANEL{.6f,.64f,.68f},WHITE{.8f,.82f,.84f},DARK{.07f,.08f,.105f},GRAPHITE{.15f,.17f,.21f},
        CYAN{.25f,1.f,1.2f},WARM{1.25f,1.18f,1.f},EMBER{1.6f,.42f,.16f};
    const bool scene=labIntro.active();
    const float open=scene?labIntro.frontDoors():0.f,H=lab::LIFT_HALF,DH=lab::LIFT_DOOR_H;

    // Admissions' landing doors, standing just proud of the wall, a cyan seam where they meet
    for(float s:{-1.f,1.f}){float o=open*(H-.05f)*s;
        B(o,0,39.42f,o+s*H,DH,39.57f,PANEL);
        B(o+s*.02f,0,39.4f,o+s*.07f,DH,39.43f,DARK,open<.02f?CYAN*.6f:vec3{0});
        B(o+s*(H-.5f),1.3f,39.39f,o+s*(H-.35f),1.9f,39.42f,GRAPHITE);}

    // The rail: four beams from far below up under the entry, braced every ten metres,
    // and a collar where it meets the ward
    const float P=labIntro.plainY(),top=scene?P+LabIntro::TOWER:-210.f;
    for(float x:{-3.05f,3.05f})for(float z:{40.25f,46.75f})B(x-.16f,(scene?P:top),z-.16f,x+.16f,-.85f,z+.16f,GRAPHITE);
    for(float y=-10.f;y>top;y-=10.f){
        for(float s:{-1.f,1.f})B(s*3.05f-.08f,y-.12f,40.25f,s*3.05f+.08f,y+.12f,46.75f,GRAPHITE);
        B(-3.05f,y-.12f,46.67f,3.05f,y+.12f,46.83f,GRAPHITE);
        if((int)std::lround(-y/10)%2==0)for(float s:{-1.f,1.f})B(s*3.05f-.1f,y+.2f,46.86f,s*3.05f+.1f,y+.5f,46.95f,DARK,CYAN*.9f);
    }
    for(float s:{-1.f,1.f})B(s*3.25f,-2.2f,39.4f,s*3.95f,-.8f,47.4f,DARK);
    B(-3.95f,-2.2f,46.9f,3.95f,-.8f,47.4f,DARK);
    B(-3.95f,-1.f,47.4f,3.95f,-.9f,47.45f,DARK,CYAN*.5f);
    if(!scene)return;

    // ---- the car ----
    const float cy=labIntro.carY();
    // floor round a glass plate (renderLabGlass draws the glass), lit at its rim
    B(-2.8f,cy-.3f,40.6f,2.8f,cy,42.2f,PANEL);B(-2.8f,cy-.3f,45.6f,2.8f,cy,46.4f,PANEL);
    for(float s:{-1.f,1.f})B(s*1.6f,cy-.3f,42.2f,s*2.8f,cy,45.6f,PANEL);
    for(float z:{42.16f,45.64f})B(-1.64f,cy-.01f,z-.04f,1.64f,cy+.012f,z+.04f,DARK,CYAN*.8f);
    for(float s:{-1.f,1.f})B(s*1.6f-.04f,cy-.01f,42.2f,s*1.6f+.04f,cy+.012f,45.6f,DARK,CYAN*.8f);
    B(-2.6f,cy-1.2f,40.8f,2.6f,cy-.3f,41.9f,GRAPHITE);B(-2.6f,cy-1.2f,45.9f,2.6f,cy-.3f,46.3f,GRAPHITE);
    // side walls: a panel to the handrail, glass above it, posts at the corners and between
    for(float s:{-1.f,1.f}){
        B(s*2.74f,cy,40.6f,s*2.86f,cy+1.f,46.4f,PANEL);
        B(s*2.74f,cy+3.3f,40.6f,s*2.86f,cy+3.6f,46.4f,PANEL);
        for(float z:{40.75f,43.5f,46.25f})B(s*2.72f,cy+1.f,z-.15f,s*2.88f,cy+3.3f,z+.15f,GRAPHITE);
        B(s*2.55f,cy+.92f,41.f,s*2.65f,cy+1.f,46.f,GRAPHITE);
        B(s*2.66f,cy+.9f,41.f,s*2.7f,cy+.93f,46.f,DARK,CYAN*.5f);
    }
    // a glass roof between two lamp strips
    B(-2.8f,cy+3.6f,40.6f,2.8f,cy+3.85f,41.4f,WHITE);B(-2.8f,cy+3.6f,45.6f,2.8f,cy+3.85f,46.4f,WHITE);
    for(float s:{-1.f,1.f}){B(s*1.8f,cy+3.6f,41.4f,s*2.8f,cy+3.85f,45.6f,WHITE);B(s*1.9f,cy+3.55f,41.6f,s*2.6f,cy+3.6f,45.4f,WHITE,WARM);}
    // the front (the ward side) and the back (the tower side): posts, a lintel, two door leaves each
    for(float z:{40.6f,46.4f}){
        for(float s:{-1.f,1.f})B(s*2.f,cy,z-.1f,s*2.8f,cy+3.6f,z+.1f,PANEL);
        B(-2.f,cy+3.2f,z-.1f,2.f,cy+3.6f,z+.1f,PANEL);
    }
    auto leaf=[&](float x0,float x1,float z,bool front){   // a framed leaf; its glass is drawn by renderLabGlass
        float l=std::min(x0,x1),h=std::max(x0,x1);
        if(front)B(l,cy,z-.05f,h,cy+1.f,z+.05f,PANEL);
        else B(l,cy,z-.05f,h,cy+.1f,z+.05f,GRAPHITE);
        B(l,cy+3.f,z-.05f,h,cy+3.2f,z+.05f,front?PANEL:GRAPHITE);
        for(float x:{l,h})B(x-.05f,cy,z-.06f,x+.05f,cy+3.2f,z+.06f,GRAPHITE);
    };
    {float o=labIntro.frontDoors()*1.95f;for(float s:{-1.f,1.f})leaf(s*o,s*(o+2.f),40.67f,true);}
    {float o=labIntro.rearDoors()*1.95f;for(float s:{-1.f,1.f})leaf(s*o,s*(o+2.f),46.33f,false);}
    // the scanner's sheet of light, passing down over the chassis as it steps out
    if(float k=labIntro.scan();k>0 && k<1)B(-2.6f,3.4f-3.4f*k,33.f,2.6f,3.43f-3.4f*k,38.6f,DARK,CYAN*1.4f*std::sin(3.1416f*k));

    // ---- the tower (Chapter 2's: LevelWasteland.h's 24 x 20 x 90, rings every 8 m, a lit column
    // of slots up its face and the beacon on top) built round the car's shaft ----
    const vec3 CONC{.16f,.13f,.15f},RING{.15f,.23f,.26f};
    const float z0=33.5f,z1=53.5f,TH=LabIntro::TOWER;
    B(-12,P,z0,12,P+TH,z0+1,CONC);
    for(float s:{-1.f,1.f})B(s*11,P,z0+1,s*12,P+TH,z1-1,CONC);
    B(-12,P,z1-1,-2.5f,P+TH,z1,CONC);B(2.5f,P,z1-1,12,P+TH,z1,CONC);B(-2.5f,P+4.5f,z1-1,2.5f,P+TH,z1,CONC);
    B(-12,P+TH-1,z0+1,12,P+TH,40,CONC);B(-12,P+TH-1,47.2f,12,P+TH,z1-1,CONC);
    for(float s:{-1.f,1.f})B(s*3.4f,P+TH-1,40,s*11,P+TH,47.2f,CONC);
    for(int k=0;k<10;++k){float y=P+k*8.f;
        B(-13.5f,y,z1,13.5f,y+1,z1+2,RING);B(-13.5f,y,z0-2,13.5f,y+1,z0,RING);
        for(float s:{-1.f,1.f})B(s*12,y,z0,s*13.5f,y+1,z1,RING);
        if(k>0)B(-2,y+1,z1-1.1f,2,y+4,z1+.1f,DARK,EMBER*1.3f);   // the lit slots up its face (the road sees these)
        for(float s:{-1.f,1.f}){
            if(k>0)B(s*10.9f,y+3+(k&1)*2.f,41.5f,s*12.1f,y+5.5f+(k&1)*2.f,45.5f,DARK,EMBER*.9f);
            B(s*10.4f,y+.6f,z0+1,s*11,y+1,z1-1,RING);                       // a ledge inside, every floor
            for(float z:{37.f,50.f})B(s*10.3f,y+1,z-.3f,s*10.6f,y+1.4f,z+.3f,DARK,EMBER*1.1f);
        }
    }
    B(-1.5f,P+4.7f,z1+.05f,1.5f,P+5.f,z1+.35f,DARK,EMBER);   // a lamp over the door
    // the beacon: a lit crown round the shaft's top, which the car rises through
    const vec3 BEACON{2.f,.5f,.15f};const float bt=P+TH;
    B(-5,bt,37.5f,5,bt+11,40,DARK,BEACON);B(-5,bt,47.2f,5,bt+11,49.5f,DARK,BEACON);
    for(float s:{-1.f,1.f})B(s*3.4f,bt,40,s*5,bt+11,47.2f,DARK,BEACON);

    // ---- the plain: ash hardpan, the road north to the tower, canyon walls either side. It is
    // a broken-off piece of Chapter 2's world: from above, its edges show, then it sinks away ----
    if(P<-560)return;   // (sunk out of sight)
    const vec3 ASH{.42f,.31f,.24f};
    auto G=[&](float x0,float y0,float z0,float x1,float y1,float z1,vec3 col){B(x0,y0,z0,x1,y1,z1,col);r.out.back().surface=6.f;};
    G(-420,P-60,-420,420,P-.03f,760,ASH);
    G(-430,P-64,-430,430,P-40,770,{.13f,.09f,.08f});
    G(-4,P-.03f,z1,4,P+.005f,760,{.09f,.085f,.08f});
    for(int k=0;k<36;++k)B(-.12f,P+.005f,58+k*13.f,.12f,P+.02f,64+k*13.f,{.5f,.4f,.18f});
    // the canyon: long broken walls of strata either side of the road, a lower shelf before each
    for(int k=0;k<36;++k){float s=k&1?-1.f:1.f;
        float h0=labfx::hash((float)k,1.f),h1=labfx::hash((float)k,2.f),h2=labfx::hash((float)k,3.f);
        vec3 at{s*(46+h0*40),P,70+(k/2)*42.f+h1*10};
        vec3 size{20+h1*22,16+h2*30,60+h0*50};
        mat4 m=T(at)*RY(s*(h2-.5f)*.35f);
        r.box(m,{0,size.y*.45f,0},size,vec3{.3f,.16f,.12f}*(.85f+.3f*h1));r.out.back().surface=6.f;
        r.box(m,{-s*size.x*.55f,size.y*.18f,0},{size.x*.5f,size.y*.36f,size.z*.9f},vec3{.34f,.2f,.14f});r.out.back().surface=6.f;
    }
}
