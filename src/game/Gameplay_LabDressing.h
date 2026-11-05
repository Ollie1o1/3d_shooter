#pragma once
// Chapter 3's moving set dressing (built by world/LabDressing.h): debris adrift
// in the void, currents and bubbles in the specimen tanks, power arcing to the
// assessment rail, light shafts under the ward lamps and the portal's vortex.
// Only the wards beside the player are drawn.

namespace labfx {
inline int nearRoom(glm::vec3 p){return std::clamp((int)std::lround(-p.z/120.f),0,9);}
inline bool near(int room,int at){return std::abs(room-at)<=1;}
inline float hash(float a,float b){return glm::fract(std::sin(a*12.9898f+b*78.233f)*43758.5453f);}
}

inline void GameplayState::gatherLabDressing(std::vector<BoxInstance>& out){
    using namespace rig;using glm::vec3;Rig r{out};
    const auto& st=level.story;const float t=gameClock;const int room=labfx::nearRoom(player.position);
    auto line=[&](vec3 a,vec3 b,float width,vec3 color,vec3 glow){auto d=b-a;float n=glm::length(d);if(n<.001f)return;
        auto root=T((a+b)*.5f)*RY(std::atan2(d.x,d.z))*RX(-std::atan2(d.y,glm::length(glm::vec2{d.x,d.z})));
        r.box(root,{0,0,0},{width,width,n},color,glow);
    };

    // Debris adrift: each piece bobs on its own slow orbit and tumbles
    for(const auto& d:st.labDrift){
        if(!labfx::near(room,d.room))continue;
        float ph=d.phase;
        vec3 p=d.pos+vec3{std::sin(t*.13f+ph)*1.4f,std::sin(t*.21f+ph*1.7f)*1.1f,std::cos(t*.11f+ph)*1.4f};
        mat4 root=T(p)*RY(ph+t*d.spin)*RX(ph*.7f+std::sin(t*.17f+ph)*.5f)*RZ(t*d.spin*.6f);
        vec3 s=d.size;
        switch(d.kind){
        case 0:{   // server rack, still blinking
            r.box(root,{0,0,0},s,d.color);
            for(int k=0;k<6;++k){float on=labfx::hash(ph,(float)k+std::floor(t*(1.5f+k*.4f)))>.35f?1.f:.15f;
                vec3 col=k%3==0?vec3{.3f,1.1f,1.2f}:k%3==1?vec3{.3f,1.2f,.5f}:vec3{1.4f,.2f,.15f};
                r.box(root,{s.x*(-.3f+.12f*(k%5)),s.y*(-.4f+.14f*k),s.z*.51f},{.06f,.04f,.02f},col*.2f,col*on);}
            r.box(root,{s.x*.46f,0,s.z*.51f},{.03f,s.y*.9f,.02f},{.1f,.2f,.3f},vec3{.25f,.9f,1.1f}*.8f);
            break;}
        case 1:{   // a ward door, its little window dark
            r.box(root,{0,0,0},s,d.color);
            r.box(root,{0,s.y*.22f,0},{s.x*.45f,s.y*.24f,s.z*1.2f},{.05f,.07f,.09f},{.02f,.06f,.08f});
            r.box(root,{s.x*.36f,-s.y*.04f,s.z*.7f},{.12f,.04f,.06f},{.4f,.42f,.45f});
            break;}
        case 2:   // floor tiles, still grouted to their bedding
            r.box(root,{0,0,0},s,d.color);
            r.box(root,{0,-s.y*.75f,0},{s.x*.96f,s.y*.6f,s.z*.96f},{.2f,.21f,.24f});
            break;
        case 3:   // a chunk of deck with its tiles on top
            r.box(root,{0,0,0},s,d.color);
            r.box(root,{0,s.y*.55f,0},{s.x*1.02f,.12f,s.z*1.02f},{.78f,.8f,.82f});
            break;
        case 4:{   // a monitor, its screen flickering
            r.box(root,{0,0,0},s,d.color);
            float f=.6f+.4f*labfx::hash(ph,std::floor(t*9));
            vec3 col=labfx::hash(ph,3)<.6f?vec3{.25f,.95f,1.1f}:vec3{1.3f,.2f,.2f};
            r.box(root,{0,0,s.z*.51f},{s.x*.8f,s.y*.72f,.02f},col*.1f,col*f*.8f);
            break;}
        default:   // a locker, vents across it
            r.box(root,{0,0,0},s,d.color);
            for(int k=0;k<3;++k)r.box(root,{0,s.y*(.3f-.06f*k),s.z*.51f},{s.x*.6f,.025f,.02f},{.1f,.11f,.13f});
        }
    }

    // Specimen tanks: two currents winding up the fluid, bubbles rising through it
    for(const auto& k:st.labTanks){
        if(!labfx::near(room,k.room))continue;
        float lo=.9f,hi=k.h-.95f,span=hi-lo,rad=k.r*1.012f;
        float seed=k.base.x*.31f+k.base.z*.17f;
        vec3 other=k.color.r>.6f?vec3{.3f,1.f,1.2f}:vec3{1.f,.3f,1.25f};
        // Each current is a continuous ribbon wound round the fluid, its pitch
        // breathing; the second turns the other way in the other colour
        for(int strand=0;strand<3;++strand){
            vec3 glow=(strand==1?other:k.color)*(strand==2?.7f:1.4f);
            float turn=strand==1?-1.f:1.f,off=seed+strand*2.1f;
            auto at=[&](float u){
                float a=off+turn*(u*7.5f+t*(.5f+.15f*strand))+std::sin(t*.6f+u*4+strand)*.35f;
                return k.base+vec3{std::cos(a)*rad,lo+span*u,std::sin(a)*rad};
            };
            vec3 prev=at(0);
            for(int j=1;j<=28;++j){float u=j/28.f;vec3 p=at(u);
                float fade=std::sin(3.1416f*u);
                line(prev,p,.07f*fade+.02f,glow*.12f,glow*(.35f+.65f*fade));prev=p;}
        }
        for(int b=0;b<7;++b){
            float rise=glm::fract(t*(.12f+.03f*b)+b*.37f+seed);
            float a=seed*3+b*1.9f+std::sin(t*.9f+b)*.3f;
            vec3 p=k.base+vec3{std::cos(a)*rad,lo+rise*span,std::sin(a)*rad};
            float s=.06f+.05f*labfx::hash((float)b,seed);
            r.box(T(p),{0,0,0},{s,s,s},{.6f,.8f,.9f},vec3{.7f,1.1f,1.2f}*(1-rise*.6f));
        }
    }

    // Power arcing from the islands' tanks to the rail: a jagged line, re-struck
    // fifteen times a second, sometimes gone for a beat
    for(size_t i=0;i<st.labArcs.size();++i){
        const auto& a=st.labArcs[i];
        if(!labfx::near(room,a.room))continue;
        float beat=std::floor(t*15);
        if(labfx::hash((float)i,std::floor(t*4))>.82f)continue;
        vec3 d=a.b-a.a,side=glm::normalize(glm::cross(d,vec3{0,1,0})),up=glm::normalize(glm::cross(side,d));
        vec3 prev=a.a;
        for(int s=1;s<=10;++s){
            float u=s/10.f,env=std::sin(3.1416f*u);
            vec3 p=glm::mix(a.a,a.b,u)+(side*(labfx::hash((float)i*7+s,beat)-.5f)+up*(labfx::hash((float)i*3+s,beat+.5f)-.5f))*env*1.6f;
            if(s==10)p=a.b;
            line(prev,p,.12f,a.color*.2f,a.color*1.4f);
            line(prev,p,.045f,vec3{.8f},vec3{1.6f,1.6f,1.8f});
            prev=p;
        }
    }
    gatherLabVista(out);
}

// The far distance of the open wards, 230-450 m out where the fog turns things to silhouettes
// (the camera sees 500 m): what the lab is part of. Celestial Observation: other wings of the lab
// adrift, lit, trailing cable, under a slow colossal ring. Hadal Research: something vast in the
// trench, curled up asleep, and something long swimming round it. Final Assessment: the wings again,
// and over the arena a surgical lamp the size of a building, looking down at the fight (its lenses
// redden as the Successor fails)
inline void GameplayState::gatherLabVista(std::vector<BoxInstance>& out){
    // Each piece is a Blender model joined by look (tools/blender/story_vista_build.py): a handful of
    // instanced parts per copy, and copies share their meshes, so the whole vista is a few draw calls
    using namespace rig;using glm::vec3;
    const float t=gameClock;const int room=labfx::nearRoom(player.position);
    if(room<7)return;
    const vec3 c=lab::centre(room);
    const vec3 CYAN{.3f,1.1f,1.3f},WARM{1.5f,1.3f,1.f},GLOW{.2f,.9f,1.f};

    if(room==7||room==9){
        // Wings of the lab adrift (the model is 60 x 16 x 34 m), bobbing and turning very slowly; the
        // flickering windows of each blink on their own beat
        static const float WING[6][6]={{-300,40,-120,60,16,34},{330,-20,-60,70,18,30},{-260,-60,180,44,12,26},
                                       {280,70,210,52,14,28},{-60,110,-420,80,18,40},{120,-90,330,46,12,24}};
        for(int k=0;k<6;++k){
            float seed=.13f+k*.17f;
            mat4 m=T(c+vec3{WING[k][0],WING[k][1]+std::sin(t*.05f+seed)*4.f,WING[k][2]})*RY(seed*2.f+t*.006f*(k&1?1.f:-1.f))
                  *RZ(.08f*std::sin(seed*9.f))*RX(.06f*std::cos(seed*5.f))*S(vec3{WING[k][3]/60.f,WING[k][4]/16.f,WING[k][5]/34.f});
            float flick=labfx::hash(seed*31,std::floor(t*(.7f+.3f*k)))>.3f?1.f:.1f;
            propdraw::push(out,"lab_wing",m,CYAN*flick);
        }
    }
    if(room==7){
        // A colossal ring turning slowly far over the hall: 40 segments, the band alternating cyan and violet
        const vec3 at=c+vec3{40,170,-300};const float R=110;
        mat4 ring=T(at)*RX(1.1f)*RZ(t*.02f);
        for(int k=0;k<40;++k)
            propdraw::push(out,"lab_ring",ring*RZ(k*6.2832f/40)*T(vec3{R,0,0}),(k&1?CYAN:vec3{.6f,.3f,1.2f})*.9f);
    }
    if(room==8){
        // Far under the floor, seen down the wells and over the edges: something vast asleep on its
        // side, the lights down its spine pulsing as slowly as breathing
        float breathe=.55f+.45f*std::sin(t*.35f);
        mat4 body=T(c+vec3{10,-190+std::sin(t*.04f)*3.f,-10})*RY(.6f+t*.004f)*RX(1.35f)*RZ(.25f);
        propdraw::push(out,"hadal_sleeper",body,GLOW*breathe);
        // and something long swimming a slow circle round it: one segment model, strung and waved
        for(int k=0;k<18;++k){float ph=t*.05f-k*.035f,rad=150.f;
            vec3 p=c+vec3{std::cos(ph)*rad,-120+std::sin(ph*3+k*.4f)*6.f,std::sin(ph)*rad};
            vec3 q=c+vec3{std::cos(ph+.01f)*rad,-120,std::sin(ph+.01f)*rad};
            float yaw=std::atan2(q.x-p.x,q.z-p.z);float sz=k<3?7.f+k*2:k>12?(18-k)*2.f+2:13.f;
            propdraw::push(out,"hadal_segment",T(p)*RY(yaw+std::sin(t*.8f-k*.5f)*.15f)*S(vec3{sz/13.f}),GLOW*(.5f+.5f*std::sin(t*2-k)));}
    }
    if(room==9){
        // The lamp over the arena: it turns to follow the player; its lenses go from white to red as the
        // Successor loses. Its arm reaches up out of sight in two lengths of the ring's segment
        float heat=0;for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::SUCCESSOR)heat=1-e.health/e.maxHealth;
        vec3 head=c+vec3{std::sin(t*.05f)*20.f,150,-60};
        vec3 to=player.position-head;float yaw=std::atan2(to.x,to.z),pitch=std::atan2(-to.y,glm::length(glm::vec2{to.x,to.z}));
        propdraw::push(out,"lab_lamp",T(head)*RY(yaw)*RX(pitch-1.5708f),glm::mix(WARM,vec3{2.f,.25f,.15f},heat));
        vec3 j0=head+vec3{0,30,0},j1=head+vec3{60,110,-120},j2=head+vec3{20,260,-260};
        for(auto [a,b]:{std::pair<vec3,vec3>{j0,j1},{j1,j2}}){vec3 d=b-a;
            mat4 m=propdraw::along((a+b)*.5f,d)*RX(1.5708f)*S(vec3{1.2f,glm::length(d)/18.f,1.2f});
            propdraw::push(out,"lab_ring",m,CYAN*.5f);}
    }
}

// Light shafts and the portal: additive, depth-tested, not written to depth
inline void GameplayState::renderLabAtmosphere(const glm::mat4& view,const glm::mat4& proj,glm::vec3 camPos){
    using glm::vec3;const auto& st=level.story;const int room=labfx::nearRoom(player.position);
    labGlassShader.use();labGlassShader.setMat4("projection",proj);labGlassShader.setMat4("view",view);
    labGlassShader.setVec3("uViewPos",camPos);labGlassShader.setFloat("uTime",gameClock);
    labGlassShader.setVec3("uOrigin",vec3{0});labGlassShader.setVec3("uNormal",vec3{0,0,1});labGlassShader.setFloat("uAnomaly",0);
    glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
    glBindVertexArray(waterVAO);glBindBuffer(GL_ARRAY_BUFFER,waterVBO);
    auto quad=[&](vec3 a,vec3 b,vec3 d,vec3 e){const vec3 v[]={a,b,d,a,d,e};glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);glDrawArrays(GL_TRIANGLES,0,6);};
    labGlassShader.setFloat("uMode",3);
    for(const auto& beam:st.labBeams){
        if(!labfx::near(room,beam.room))continue;
        vec3 mid=(beam.top+beam.bottom)*.5f,axis=glm::normalize(beam.bottom-beam.top);
        vec3 side=glm::cross(axis,camPos-mid);float n=glm::length(side);
        if(n<.001f || glm::length(camPos-mid)>140)continue;
        side/=n;
        labGlassShader.setVec3("uBeamA",beam.top);labGlassShader.setVec3("uBeamB",beam.bottom);
        labGlassShader.setVec2("uBeamR",glm::vec2{beam.r0,beam.r1});labGlassShader.setVec3("uBeamColor",beam.color);
        quad(beam.top-side*beam.r0,beam.top+side*beam.r0,beam.bottom+side*beam.r1,beam.bottom-side*beam.r1);
    }
    if(st.labPortalRadius>0 && room>=8){
        float R=st.labPortalRadius;vec3 p=st.labPortal;
        labGlassShader.setFloat("uMode",4);labGlassShader.setVec3("uBeamA",p);labGlassShader.setVec2("uBeamR",glm::vec2{R,0});
        float heat=0;for(const auto& e:enemies)if(e.alive && e.type==EnemyType::SUCCESSOR)heat=1-e.health/e.maxHealth;
        labGlassShader.setVec3("uBeamColor",vec3{heat,0,0});
        quad(p+vec3{-R,-R,0},p+vec3{R,-R,0},p+vec3{R,R,0},p+vec3{-R,R,0});
    }
    glBindVertexArray(0);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glEnable(GL_CULL_FACE);
}
