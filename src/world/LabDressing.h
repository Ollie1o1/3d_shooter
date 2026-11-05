#pragma once
// =============================================================================
// LabDressing.h — Chapter 3's set dressing, after the concept boards: a clean
// white-tiled laboratory that comes apart as the ward descends into a violet
// and teal void.
//
//   Early wards (0-5): specimen tanks and server racks along the walls, ring
//     pads, floor circuitry toward each exit, ceiling cable runs, lamps whose
//     light shafts reach the floor, warning beacons over the doors.
//   Long-Term Care (6): roots through the walls, debris drifting in the nave.
//   Late wards (7-9): neon-lit rails, slab edges broken off into the void with
//     rebar and cables hanging from them, a dark service deck under every
//     floor, broken islands of the lab floating outside, and in the final
//     assessment, a swirling portal held in a cabled frame.
//
// Nothing here changes a fight. Shapes are visual only; the only collision is
// a tank's or rack's footprint against a wall, placed by `clearAt` clear of
// doors, ramps, galleries, spawns, encounter points, terminals and pickups.
// =============================================================================
#include "world/Level.h"
#include <cstdint>

namespace labdress {
using glm::vec3;
inline const vec3 WHITE{.80f,.82f,.84f},PANEL{.6f,.64f,.68f},DARK{.07f,.08f,.105f},GRAPHITE{.15f,.17f,.21f},
    CABLE{.03f,.035f,.05f},RUST{.36f,.2f,.12f},ROOT{.33f,.16f,.14f};
inline const vec3 CYAN{.25f,1.f,1.2f},MAGENTA{1.f,.3f,1.25f},VIOLET{.55f,.3f,1.25f},RED{1.5f,.15f,.12f},
    AMBER{1.3f,.75f,.2f},GREEN{.3f,1.25f,.55f};
constexpr float TAU=6.2831853f;

inline float hash(int a,int b){
    uint32_t h=(uint32_t)a*374761393u+(uint32_t)b*668265263u+0x9E3779B9u;
    h=(h^(h>>13))*1274126177u;h^=h>>16;return (float)(h&0xFFFFFF)/16777215.f;
}
inline bool overlaps(const AABB& a,const AABB& b){
    return a.min.x<b.max.x&&a.max.x>b.min.x&&a.min.y<b.max.y&&a.max.y>b.min.y&&a.min.z<b.max.z&&a.max.z>b.min.z;
}
inline float flat(vec3 a,vec3 b){return glm::length(glm::vec2{a.x-b.x,a.z-b.z});}

struct Dresser{
    LevelBuilder& b;LevelData& l;int room;vec3 c;float y;int n=0;
    float entryX=0,exitX=0;std::vector<float> extraLanes;
    float rnd(){return hash(room*7919+(++n),913);}
    float rr(float lo,float hi){return lo+(hi-lo)*rnd();}
    ShapeKit kit(Mat m,bool glow=false){b.mat=m;return b.kit(glow);}
    float yawOf(vec3 d){return -std::atan2(d.z,d.x);}   // a box's local +X along d

    // ---- cables -------------------------------------------------------------
    // A sagging cable between two points: a shallow parabola of short rods
    void cable(vec3 a,vec3 z,float sag,float r=.06f,vec3 col=CABLE,int seg=10){
        auto k=kit(Mat::METAL);vec3 prev=a;
        for(int i=1;i<=seg;++i){float t=(float)i/seg;vec3 p=glm::mix(a,z,t)-vec3{0,sag*4*t*(1-t),0};k.rod(prev,p,r,col,5);prev=p;}
    }
    // A loose end hanging `len` from `top`, curling a little; some still spark
    void dangle(vec3 top,float len,float r=.05f,vec3 col=CABLE){
        auto k=kit(Mat::METAL);vec3 prev=top;float dx=rr(-.7f,.7f),dz=rr(-.7f,.7f);
        for(int i=1;i<=6;++i){float t=i/6.f;vec3 p=top+vec3{dx*t*t,-len*t,dz*t*t};k.rod(prev,p,r,col,5);prev=p;}
        bool spark=rnd()<.3f;vec3 hot=rnd()<.5f?CYAN*1.6f:AMBER*1.6f;
        if(spark)kit(Mat::METAL,true).box(prev,{.08f,.08f,.08f},hot);
    }
    // Three or four cables run together, each sagging its own way
    void bundle(vec3 a,vec3 z,float sag,int count=3){
        for(int i=0;i<count;++i){
            vec3 o{rr(-.3f,.3f),rr(-.15f,.15f),rr(-.3f,.3f)};
            float tint=rnd(),scale=rr(.75f,1.25f),thick=rr(.04f,.085f);
            vec3 col=i==0&&tint<.4f?vec3{.05f,.08f,.15f}:i==1&&tint<.3f?vec3{.13f,.05f,.06f}:CABLE;
            cable(a+o,z+o,sag*scale,thick,col);
        }
    }

    // ---- furniture ------------------------------------------------------------
    // A specimen tank: dark plinth, lit collar, a column of glowing fluid held
    // by four struts, a white head with feed lines. Its currents and bubbles
    // move at render time (labTanks).
    void tank(vec3 p,float r,float h,vec3 fluid,bool solid){
        auto m=kit(Mat::METAL),g=kit(Mat::METAL,true);
        m.column(p,r*1.34f,.32f,DARK,18);
        kit(Mat::CLINIC).column(p+vec3{0,.32f,0},r*1.2f,.42f,WHITE,18);
        g.column(p+vec3{0,.74f,0},r*1.24f,.06f,fluid*.9f,18);
        g.column(p+vec3{0,.8f,0},r,h-1.6f,fluid*.3f,18);
        for(int s=0;s<4;++s){float a=s*TAU/4+.4f;vec3 o{std::cos(a)*r*1.07f,0,std::sin(a)*r*1.07f};
            m.box(p+o+vec3{0,h*.5f,0},{.1f,h-1.4f,.1f},GRAPHITE,-a);}
        g.column(p+vec3{0,h-.86f,0},r*1.24f,.06f,fluid*.7f,18);
        kit(Mat::CLINIC).column(p+vec3{0,h-.8f,0},r*1.2f,.44f,WHITE,18);
        m.column(p+vec3{0,h-.36f,0},r*.92f,.36f,DARK,14);
        m.column(p+vec3{0,h,0},r*.3f,.3f,GRAPHITE,8);
        kit(Mat::METAL,true).box(p+vec3{r*1.21f,h-.58f,0},{.04f,.12f,.3f},rnd()<.5f?RED:GREEN);
        // A dim light inside the fluid, so the floor round it takes its colour
        b.neon(p.x-r*.4f,p.y+h*.45f,p.z-r*.4f,p.x+r*.4f,p.y+h*.55f,p.z+r*.4f,fluid*.32f);
        if(solid)b.solid(p.x-r*1.34f,p.y,p.z-r*1.34f,p.x+r*1.34f,p.y+h,p.z+r*1.34f);
        l.story.labTanks.push_back({p,r,h,fluid,room});
    }
    // A ring pad flush with the floor: two lit rings, three chevrons, a dark core
    void pad(vec3 p,float r,vec3 col=CYAN){
        auto m=kit(Mat::METAL),g=kit(Mat::METAL,true);
        m.column(p,r+.45f,.05f,GRAPHITE,36);
        m.column(p+vec3{0,.05f,0},r*.56f,.015f,DARK,28);
        g.curve(p,r,0,TAU,.05f,.085f,.16f,col*.95f,40);
        g.curve(p,r*.72f,0,TAU,.05f,.07f,.06f,col*.5f,32);
        for(int s=0;s<3;++s){float a=s*TAU/3+.5f;
            g.box(p+vec3{std::cos(a)*(r+.27f),.065f,std::sin(a)*(r+.27f)},{.2f,.025f,.2f},col*.8f,-a+.785f);}
        b.neon(p.x-.25f,p.y+.05f,p.z-.25f,p.x+.25f,p.y+.07f,p.z+.25f,col*.16f);
    }
    // A server rack with its back to a wall, facing `face` (a unit axis):
    // slotted front, scattered status lights, a lit edge strip
    void rack(vec3 p,vec3 face,float h,bool solid){
        vec3 side{-face.z,0,face.x};
        auto at=[&](float u,float v,float w){return p+face*u+side*v+vec3{0,w,0};};
        auto slab=[&](float u0,float v0,float w0,float u1,float v1,float w1,vec3 col,bool glow=false){
            vec3 a=at(u0,v0,w0),z=at(u1,v1,w1);
            if(glow)kit(Mat::METAL,true).box((a+z)*.5f,glm::abs(z-a),col);
            else{b.mat=Mat::METAL;b.prop(a.x,a.y,a.z,z.x,z.y,z.z,col);}
        };
        slab(-.45f,-.62f,0,.45f,.62f,h,DARK);
        slab(.45f,-.54f,.12f,.47f,.54f,h-.12f,GRAPHITE);
        int slots=(int)((h-.3f)/.24f);
        for(int j=0;j<slots;++j){float w=.18f+j*.24f;
            slab(.47f,-.5f,w,.49f,.5f,w+.03f,DARK);
            int lights=1+(int)(rnd()*3);
            for(int k=0;k<lights;++k){float v=rr(-.42f,.36f);float pick=rnd();
                vec3 col=pick<.5f?CYAN:pick<.75f?GREEN:pick<.9f?AMBER:RED;
                slab(.48f,v,w+.08f,.5f,v+.05f,w+.12f,col*1.1f,true);}
        }
        slab(.46f,.56f,.1f,.5f,.6f,h-.1f,(rnd()<.5f?CYAN:VIOLET)*.8f,true);
        slab(-.4f,-.5f,h,.4f,.5f,h+.04f,GRAPHITE);
        if(solid){vec3 a=at(-.45f,-.62f,0),z=at(.45f,.62f,h);b.solid(a.x,a.y,a.z,z.x,z.y,z.z);}
    }
    // An old terminal desk (island scenery only, so it isn't mistaken for a record)
    void desk(vec3 p,float yaw){
        auto w=kit(Mat::CLINIC),m=kit(Mat::METAL),g=kit(Mat::METAL,true);
        vec3 f{std::cos(yaw),0,-std::sin(yaw)},s{std::sin(yaw),0,std::cos(yaw)};
        w.box(p+vec3{0,.45f,0},{1.8f,.9f,.8f},PANEL,yaw);
        m.box(p+vec3{0,.95f,0}-s*.1f,{1.7f,.12f,.55f},GRAPHITE,yaw,0,0);
        for(int k=-1;k<=1;++k){vec3 q=p+f*(k*.55f)+vec3{0,1.25f,0}-s*.12f;
            m.box(q,{.48f,.42f,.4f},DARK,yaw);
            g.box(q+s*.21f,{.38f,.3f,.02f},(k?CYAN:RED)*.7f,yaw);}
    }
    // A fence of light: posts and four glowing bars (visual: where it bounds play, a real rail stands under it)
    void fence(vec3 a,vec3 z,float h=1.1f,vec3 col=CYAN){
        auto m=kit(Mat::METAL),g=kit(Mat::METAL,true);vec3 d=z-a;float len=glm::length(d),yaw=yawOf(d);
        int posts=std::max(2,(int)(len/2.4f)+1);
        for(int i=0;i<posts;++i){vec3 p=glm::mix(a,z,i/(float)(posts-1));
            m.box(p+vec3{0,h*.5f,0},{.1f,h,.1f},GRAPHITE,yaw);g.box(p+vec3{0,h+.03f,0},{.13f,.05f,.13f},col);}
        for(int r=1;r<=4;++r)g.box((a+z)*.5f+vec3{0,h*r/4.4f,0},{len,.035f,.035f},col*(r==4?1.f:.55f),yaw);
    }
    // Lit strips on both faces of an existing rail (the rail keeps its collision)
    void railGlow(vec3 a,vec3 z,float half,float h,vec3 col=CYAN){
        auto g=kit(Mat::METAL,true);vec3 d=z-a;float len=glm::length(d),yaw=yawOf(d);
        vec3 n=glm::normalize(vec3{-d.z,0,d.x});
        for(float s:{-1.f,1.f})for(int r=1;r<=3;++r)
            g.box((a+z)*.5f+n*(s*(half+.012f))+vec3{0,h*r/3.6f,0},{len-.2f,.035f,.02f},col*(r==3?.8f:.45f),yaw);
    }

    // ---- the floor and its edges ------------------------------------------------
    // A circuit trace across the floor: a run with 45-degree jogs, nodes at its ends
    void trace(vec3 p,vec3 dir,float len,vec3 col){
        auto g=kit(Mat::METAL,true);float run=0;vec3 d=dir;
        g.box(p+vec3{0,.02f,0},{.32f,.025f,.32f},col*.9f,.785f);
        while(run<len){
            float step=std::min(rr(1.5f,4.f),len-run);vec3 q=p+d*step;
            g.box((p+q)*.5f+vec3{0,.015f,0},{step+.06f,.015f,.07f},col*.6f,yawOf(d));
            p=q;run+=step;
            float turn=rnd();vec3 side{-dir.z,0,dir.x};
            d=turn<.3f?glm::normalize(dir+side):turn<.6f?glm::normalize(dir-side):dir;
        }
        g.curve(p,.18f,0,TAU,.0f,.03f,.06f,col,10);
    }
    // A slab edge broken off: tipped tiles, chunks of deck beneath, rebar and
    // cables hanging into the void. a..z run along the floor's top edge; `out` points off it.
    void fracture(vec3 a,vec3 z,vec3 out,float deck=.8f,bool cables=true){
        vec3 d=z-a;float len=glm::length(d),yaw=yawOf(d);
        // An exact 17 m edge must make ten fragments on SIMD and scalar builds.
        // One rounded-down division otherwise changes every later random draw.
        int count=std::max(1,(int)std::floor(len/1.7f+1e-4f));
        auto w=kit(Mat::CLINIC),con=kit(Mat::CONCRETE),m=kit(Mat::METAL);
        // (every draw in its own statement: argument order differs between compilers)
        for(int i=0;i<count;++i){float t=(i+rnd())/count;vec3 p=glm::mix(a,z,t);
            float reach=rr(.3f,1.5f);
            if(rnd()<.75f){
                float drop=rr(0,.45f),wide=rr(.7f,1.6f);vec3 col=rnd()<.75f?WHITE:PANEL;
                float ty=yaw+rr(-.25f,.25f),tp=rr(-.4f,.4f),tr=rr(-.25f,.25f);
                w.box(p+out*(reach*.5f+.15f)+vec3{0,-.2f-drop,0},{wide,.22f,reach+.3f},col,ty,tp,tr);
            }
            float off=rr(.1f,.8f),down=rr(.3f,1.1f);vec3 size{rr(.6f,1.6f),rr(.5f,1.4f),rr(.6f,1.4f)};
            float shade=rr(.8f,1.2f),cy=rr(0,TAU),cp=rr(-.5f,.5f),cr=rr(-.5f,.5f);
            con.box(p+out*off+vec3{0,-deck-down,0},size,GRAPHITE*shade,cy,cp,cr);
            if(rnd()<.5f){float len=rr(.7f,1.8f);vec3 bend{rr(-.3f,.3f),rr(-.7f,.15f),rr(-.3f,.3f)};
                m.rod(p+out*.1f+vec3{0,-.35f,0},p+out*len+bend,.035f,RUST,4);}
            if(cables&&rnd()<.45f){float at=rr(.1f,.6f),len=rr(1.5f,7.f),r=rr(.04f,.08f);
                dangle(p+out*at+vec3{0,-deck,0},len,r);}
        }
    }
    // The dark service deck under a floor slab: two stepped tiers, a lit seam, cables
    void underside(float x0,float x1,float z0,float z1,float bottom,float depth){
        if(x1-x0<5||z1-z0<5)return;
        auto m=kit(Mat::METAL);vec3 mid{(x0+x1)*.5f,0,(z0+z1)*.5f};
        m.box(mid+vec3{0,bottom-depth*.5f,0},{x1-x0-1,depth,z1-z0-1},GRAPHITE);
        m.box(mid+vec3{0,bottom-depth-.7f,0},{x1-x0-4,1.4f,z1-z0-4},DARK);
        kit(Mat::METAL,true).box(mid+vec3{0,bottom-depth*.55f,0},{x1-x0-.9f,.07f,z1-z0-.9f},CYAN*.3f);
        int hang=(int)((x1-x0+z1-z0)/9);
        for(int i=0;i<hang;++i){vec3 top{rr(x0+2,x1-2),bottom-depth-1.4f,rr(z0+2,z1-2)};float len=rr(2.f,9.f),r=rr(.05f,.1f);dangle(top,len,r);}
    }

    // ---- set pieces -------------------------------------------------------------
    // A ceiling lamp and the shaft of light it throws to the floor
    void lamp(vec3 top,float floorY,vec3 col={.6f,.9f,1.f}){
        auto m=kit(Mat::METAL),g=kit(Mat::METAL,true);
        m.box(top+vec3{0,-.2f,0},{1.3f,.4f,1.3f},DARK);
        g.box(top+vec3{0,-.42f,0},{.95f,.05f,.95f},col*1.3f);
        m.rod(top,top+vec3{0,1.2f,0},.04f,CABLE,4);
        l.story.labBeams.push_back({top+vec3{0,-.45f,0},vec3{top.x,floorY+.02f,top.z},.42f,2.6f,col,room});
    }
    // A broken piece of the lab adrift: tiled slab, deck, torn edges, its own furniture
    void island(vec3 p,float w,float d,int style){
        b.mat=Mat::CLINIC;b.prop(p.x-w/2,p.y-.9f,p.z-d/2,p.x+w/2,p.y,p.z+d/2,WHITE);
        float deck=rr(1.4f,2.6f);
        underside(p.x-w/2,p.x+w/2,p.z-d/2,p.z+d/2,p.y-.9f,deck);
        fracture(p+vec3{-w/2,0,-d/2},p+vec3{w/2,0,-d/2},{0,0,-1},.9f);
        fracture(p+vec3{w/2,0,d/2},p+vec3{-w/2,0,d/2},{0,0,1},.9f);
        fracture(p+vec3{-w/2,0,d/2},p+vec3{-w/2,0,-d/2},{-1,0,0},.9f);
        fracture(p+vec3{w/2,0,-d/2},p+vec3{w/2,0,d/2},{1,0,0},.9f);
        vec3 fl=rnd()<.5f?CYAN:MAGENTA*.85f;
        if(style==0){tank(p+vec3{-w*.25f,0,-d*.15f},1.f,4.6f,fl,false);tank(p+vec3{w*.22f,0,d*.2f},.8f,3.8f,CYAN,false);pad(p+vec3{w*.18f,0,-d*.25f},1.1f);}
        if(style==1){for(int k=0;k<3;++k)rack(p+vec3{-w*.32f+k*1.3f,0,-d*.4f},{0,0,1},2.3f,false);
            desk(p+vec3{w*.18f,0,d*.1f},0);fence(p+vec3{-w*.45f,0,d*.42f},p+vec3{w*.45f,0,d*.42f});}
        if(style==2){pad(p,1.5f,MAGENTA*.9f);fence(p+vec3{w*.42f,0,-d*.4f},p+vec3{w*.42f,0,d*.4f});
            tank(p+vec3{-w*.3f,0,d*.25f},.9f,4.2f,fl,false);}
        if(style==3){desk(p+vec3{0,0,-d*.2f},0);rack(p+vec3{w*.35f,0,d*.2f},{-1,0,0},2.4f,false);
            rack(p+vec3{w*.35f,0,d*.2f-1.3f},{-1,0,0},2.1f,false);pad(p+vec3{-w*.25f,0,d*.2f},1.f);}
        // A lamp over it on a mast, and power arcing from the deck
        auto m=kit(Mat::METAL);vec3 mast=p+vec3{w*.42f,0,-d*.42f};
        m.box(mast+vec3{0,3.5f,0},{.2f,7.f,.2f},GRAPHITE);
        m.box(mast+vec3{-.6f,7.f,.6f},{1.6f,.15f,.15f},GRAPHITE,.785f);
        kit(Mat::METAL,true).box(mast+vec3{-1.1f,6.85f,1.1f},{.5f,.08f,.5f},vec3{.8f,1,1.1f});
        l.story.labBeams.push_back({mast+vec3{-1.1f,6.8f,1.1f},p+vec3{w*.42f-1.1f-1.5f,.02f,-d*.42f+1.1f+1.5f},.3f,2.4f,{.6f,.85f,1.f},room});
    }
    // Roots split through a wall: a thick trunk climbing it, branches spreading
    void roots(vec3 base,vec3 wallOut,float height){
        auto k=kit(Mat::WOOD);vec3 up{0,1,0},side{-wallOut.z,0,wallOut.x};
        vec3 prev=base;
        for(int i=1;i<=7;++i){float t=i/7.f;vec3 p=base+up*(height*t)+side*(std::sin(t*5+rnd())*.6f)+wallOut*.15f;
            k.rod(prev,p,.28f*(1-t*.6f),ROOT,7);
            if(i>1&&rnd()<.75f){vec3 q=p;float dir=rnd()<.5f?-1.f:1.f;
                for(int j=0;j<4;++j){float a=rr(.6f,1.4f),u=rr(-.3f,.9f),o=rr(0,.15f);vec3 r=q+side*(dir*a)+up*u+wallOut*o;
                    k.rod(q,r,.11f*(1-j*.2f),ROOT*1.1f,5);q=r;}}
            prev=p;
        }
    }
    // A wall-mounted beacon: a coloured lamp that lights the doorway beneath it
    void beacon(vec3 p,vec3 col){
        kit(Mat::METAL).box(p,{.42f,.18f,.42f},DARK);
        kit(Mat::METAL,true).column(p+vec3{0,.09f,0},.15f,.22f,col,8);
        b.neon(p.x-.12f,p.y+.1f,p.z-.12f,p.x+.12f,p.y+.3f,p.z+.12f,col*.25f);
    }
    // Something adrift (Gameplay_LabDressing.h moves it): 0 rack, 1 door,
    // 2 tile slab, 3 deck chunk, 4 monitor, 5 locker
    void drift(vec3 p,int kind){
        static const vec3 size[]={{.9f,2.2f,.8f},{1.f,2.1f,.09f},{1.3f,.16f,1.3f},{1.6f,1.1f,1.3f},{.7f,.5f,.5f},{.8f,1.9f,.6f}};
        static const vec3 col[]={DARK,WHITE,WHITE,GRAPHITE,DARK,PANEL};
        float s=kind==3?rr(.7f,2.4f):rr(.85f,1.2f);
        float phase=rr(0,TAU),spin=rr(.04f,.16f);if(rnd()<.5f)spin=-spin;
        l.story.labDrift.push_back({p,size[kind]*s,col[kind],kind,room,phase,spin});
    }
    void arc(vec3 a,vec3 z,vec3 col){l.story.labArcs.push_back({a,z,col,room});}

    // ---- windows onto the void ----------------------------------------------------
    // A deep frame round an opening in a side wall (LevelLab.h cuts it): jambs, sill and
    // head standing proud of the tile, a lit strip under the head, mullions and transoms
    // across the glass, a dark frame outside
    void windowFrame(const lab::Window& w){
        const float s=w.x<0?-1.f:1.f,in=-s,face=w.x+in*.4f;
        const float len=w.z1-w.z0,h=w.y1-w.y0,mz=z0(w)+len*.5f,my=y+w.y0+h*.5f;
        auto white=kit(Mat::CLINIC),m=kit(Mat::METAL),g=kit(Mat::METAL,true);
        for(float zz:{z0(w)-.16f,z0(w)+len+.16f})white.box({face+in*.25f,my,zz},{.5f,h+.6f,.32f},WHITE);
        white.box({face+in*.3f,y+w.y0-.13f,mz},{.6f,.26f,len+.64f},PANEL);
        white.box({face+in*.25f,y+w.y1+.15f,mz},{.5f,.3f,len+.64f},WHITE);
        g.box({face+in*.42f,y+w.y1-.03f,mz},{.07f,.05f,len-.3f},CYAN*.5f);
        g.box({face+in*.58f,y+w.y0-.01f,mz},{.04f,.03f,len-.3f},CYAN*.3f);
        int cols=std::max(1,(int)std::lround(len/3.4f)),rows=std::max(1,(int)std::lround(h/4.6f));
        for(int k=1;k<cols;++k)m.box({w.x,my,z0(w)+len*k/cols},{.1f,h,.08f},GRAPHITE);
        for(int k=1;k<rows;++k)m.box({w.x,y+w.y0+h*k/rows,mz},{.12f,.1f,len},GRAPHITE);
        for(float zz:{z0(w)-.2f,z0(w)+len+.2f})m.box({w.x+s*.5f,my,zz},{.22f,h+.8f,.4f},DARK);
        for(float yy:{y+w.y0-.2f,y+w.y1+.2f})m.box({w.x+s*.5f,yy,mz},{.22f,.4f,len+.8f},DARK);
    }
    float z0(const lab::Window& w)const{return c.z+w.z0;}
    // Whether something `height` tall standing against wall x at z would cover a window
    bool blocksWindow(float x,float z,float halfLen,float height)const{
        for(const auto& w:lab::windows(room))if(w.x==x && z+halfLen>c.z+w.z0-.5f && z-halfLen<c.z+w.z1+.5f && w.y0<height)return true;
        return false;
    }
    // A lit ward adrift: a slab of floor with three walls and a ceiling, open toward the
    // windows, a bed and a monitor, its lamps on. Someone paces it (drawn in Gameplay_Lab.h).
    void ward(vec3 p,float side){
        const float hx=4.5f,hz=8.f,top=4.2f;const float back=p.x+side*hx;
        auto w=kit(Mat::CLINIC),m=kit(Mat::METAL),g=kit(Mat::METAL,true);
        b.mat=Mat::CLINIC;b.prop(p.x-hx,p.y-.8f,p.z-hz,p.x+hx,p.y,p.z+hz,WHITE);
        underside(p.x-hx,p.x+hx,p.z-hz,p.z+hz,p.y-.8f,rr(1.4f,2.2f));
        w.box({back,p.y+top*.5f,p.z},{.3f,top,hz*2},{.53f,.64f,.65f});
        for(float e:{-1.f,1.f})w.box({p.x,p.y+top*.5f,p.z+e*hz},{hx*2,top,.3f},{.53f,.64f,.65f});
        w.box({p.x,p.y+top+.15f,p.z},{hx*2+.3f,.3f,hz*2+.3f},WHITE);
        // (out here no baked light reaches it: its own lit strips show it is lit)
        g.box({back-side*.17f,p.y+top-.35f,p.z},{.04f,.12f,hz*2-.6f},vec3{1.2f,1.1f,.9f});
        g.box({back-side*.17f,p.y+.12f,p.z},{.04f,.06f,hz*2-.6f},CYAN*.6f);
        for(float e:{-1.f,1.f})g.box({p.x,p.y+top-.35f,p.z+e*(hz-.17f)},{hx*2-.6f,.12f,.04f},vec3{1.2f,1.1f,.9f});
        w.box({back-side*.2f,p.y+top*.5f,p.z},{.04f,top-.9f,hz*2-.8f},vec3{.95f,.93f,.86f});
        for(float dz:{-4.f,4.f}){g.box({p.x,p.y+top-.03f,p.z+dz},{2.4f,.05f,1.1f},vec3{1.4f,1.3f,1.1f});
            b.neon(p.x-.8f,p.y+top-.3f,p.z+dz-.4f,p.x+.8f,p.y+top-.1f,p.z+dz+.4f,vec3{1.1f,1.f,.85f}*.35f);}
        // a bed against the back wall, its sheet still made, and a monitor beside it
        vec3 bed{back-side*1.4f,p.y,p.z-3.5f};
        m.box(bed+vec3{0,.33f,0},{2.f,.65f,3.8f},GRAPHITE);w.box(bed+vec3{0,.8f,0},{1.9f,.3f,3.7f},{.82f,.84f,.76f});
        m.box(bed+vec3{-side*.2f,1.5f,2.3f},{.1f,3.f,.1f},GRAPHITE);
        g.box(bed+vec3{-side*.2f,2.4f,2.3f},{.06f,.5f,.7f},CYAN*.8f);
        fracture({p.x-side*hx,p.y,p.z+hz},{p.x-side*hx,p.y,p.z-hz},{-side,0,0},.8f);
        l.story.labWards.push_back({p+vec3{0,0,1.5f},4.5f,room});
    }

    // ---- placement ----------------------------------------------------------------
    // Whether furniture with footprint `box` stands clear of everything that matters
    bool clearAt(const AABB& box,const Arena& arena){
        AABB grown=box;grown.min-=vec3{.15f,0,.15f};grown.max+=vec3{.15f,0,.15f};grown.min.y+=.05f;
        for(const auto& w:l.walls)if(w.box.max.y>y+.06f&&overlaps(grown,w.box))return false;
        AABB wide=box;wide.min-=vec3{2.5f,0,2.5f};wide.max+=vec3{2.5f,8,2.5f};wide.min.y-=1;
        for(const auto& r:l.ramps){AABB q=r.xz;q.min.y=-1e9f;q.max.y=1e9f;if(overlaps(wide,q))return false;}
        vec3 mid=(box.min+box.max)*.5f;
        for(const auto& d:l.doors)if(flat(mid,(d.closed.min+d.closed.max)*.5f)<9)return false;
        auto near=[&](vec3 p,float r){return flat(mid,p)<r&&std::fabs(p.y-y)<12;};
        for(auto p:arena.groundSpawns)if(near(p,4.5f))return false;
        for(auto p:arena.airSpawns)if(near(p,4.5f))return false;
        for(const auto& wave:arena.waves)for(const auto& e:wave)for(auto p:e.at)if(near(p,4.5f))return false;
        for(const auto& t:l.story.terminals)if(near(t.pos,3.5f))return false;
        for(const auto& p:l.placedPickups)if(near(p.pos,3.5f))return false;
        for(const auto& w:l.story.wrecks)if(near(w.pos,3.5f))return false;
        if(near(arena.playerStart,6)||near(arena.respawn,6))return false;
        // Keep the walking line between the doors open: down from the entry,
        // across the room, and down to the exit (and the routes the tests walk)
        float halfX=(box.max.x-box.min.x)*.5f,lz=mid.z-c.z;
        auto lane=[&](float x){return std::fabs(mid.x-x)<halfX+5.f;};
        if(lane(entryX)||lane(exitX))return false;
        float t=glm::clamp((lz-31)/(-32.f-31),0.f,1.f),lineX=glm::mix(entryX,exitX,t);
        if(std::fabs(mid.x-lineX)<halfX+5.f)return false;
        for(float x:extraLanes)if(lane(x))return false;
        // It must stand on floor at every corner (Load Testing's pit, the bridge gaps, the wells)
        for(vec3 q:{box.min,box.max,vec3{box.min.x,0,box.max.z},vec3{box.max.x,0,box.min.z},mid}){
            bool held=false;
            for(const auto& w:l.walls)if(std::fabs(w.box.max.y-y)<.02f&&q.x>=w.box.min.x&&q.x<=w.box.max.x&&q.z>=w.box.min.z&&q.z<=w.box.max.z){held=true;break;}
            if(!held)return false;
        }
        return true;
    }
};

// The dressing for one ward, after its arena has been added (l.arenas.back()).
inline void dressRoom(LevelBuilder& b,int room,vec3 c,const std::vector<lab::Bay>& bays,float roof,float entry,float exit){
    LevelData& l=b.L;Dresser d{b,l,room,c,c.y,0,0,0,{}};const Arena& arena=l.arenas.back();
    d.entryX=entry;d.exitX=exit;
    if(room==0)d.extraLanes={-18};
    if(room==3)d.extraLanes={19};
    if(room==5)d.extraLanes={0};
    if(room==7)d.extraLanes={18,38};
    if(room==8)d.extraLanes={-18,38};
    const float y=c.y,z=c.z;
    float half=0;for(const auto& bay:bays)half=std::max({half,-bay.left,bay.right});
    const bool late=room>=7,enclosed=room<4||room==5;

    // Tanks and racks against the side walls, wherever the floor beside them is free
    if(room!=4&&room!=9){
        int placed=0;
        for(const auto& bay:bays)for(float side:{-1.f,1.f}){
            float wx=side<0?bay.left:bay.right;
            for(float bz=bay.farZ+5;bz<=bay.nearZ-5&&placed<12;bz+=7.5f){
                float pick=hash(room*131+(int)(bz*3)+(side<0?0:57),77);
                if(pick<.18f)continue;
                vec3 face{-side,0,0};
                const bool glass=d.blocksWindow(wx,z+bz,2.2f,4.6f);
                if(glass && pick>=.6f)continue;   // (a rack needs a wall at its back; a tank can stand before the glass)
                if(pick<.6f){
                    float r=pick<.4f?1.05f:.85f,h=pick<.4f?4.6f:3.9f;vec3 p{wx-side*(.55f+r*1.34f+.2f+(glass?1.2f:0.f)),y,z+bz};
                    AABB box=LevelBuilder::aabb(p.x-r*1.34f,y,p.z-r*1.34f,p.x+r*1.34f,y+h,p.z+r*1.34f);
                    if(!d.clearAt(box,arena))continue;
                    vec3 fluid=room==8?vec3{.15f,.75f,.8f}:hash(room,(int)bz)<.55f?CYAN:MAGENTA*.85f;
                    d.tank(p,r,h,fluid,true);++placed;
                }else{
                    vec3 p{wx-side*1.05f,y,z+bz};
                    AABB box=LevelBuilder::aabb(p.x-.5f,y,p.z-1.95f,p.x+.5f,y+2.5f,p.z+1.95f);
                    if(!d.clearAt(box,arena))continue;
                    for(int k=-1;k<=1;++k)d.rack(p+vec3{0,0,k*1.27f},face,2.2f+.25f*((k+room)&1),true);
                    ++placed;
                }
            }
        }
    }

    // Ring pads under the health pickups, and one by the exit
    if(room!=9)d.pad(c+vec3{entry+8,0,30},1.2f);
    else for(float s:{-1.f,1.f})d.pad(c+vec3{s*42,0,6},1.3f);
    if(room!=9)d.pad(c+vec3{exit+(exit>0?-11.f:11.f),0,-35.5f},1.f,room>=6?MAGENTA*.9f:CYAN);

    // Circuitry on the floor leading to the exit, and beacons over it
    if(room!=9){
        for(int k=0;k<4;++k){float x=exit+(k<2?-1.f:1.f)*(1.3f+(k&1)*1.4f);
            // (the late wards' floor breaks for the bridges at -32)
            float len=late?d.rr(3,4.5f):d.rr(7,14);
            d.trace(c+vec3{x,0,-37.5f},{0,0,1},len,(k&1)?VIOLET:CYAN);}
        d.beacon({exit-7.4f,y+7.5f,z-39.3f},RED);
        d.beacon({exit+7.4f,y+7.5f,z-39.3f},CYAN);
    }

    // Ceiling: lamps with their light shafts and sagging cable runs
    if(enclosed||room==6){
        for(const auto& bay:bays){
            float span=bay.nearZ-bay.farZ,mid=(bay.nearZ+bay.farZ)*.5f;
            for(float t:{-.3f,.3f}){
                if(span<14&&t>0)continue;
                float bz=mid+t*span,bx=(bay.left+bay.right)*.5f+(t>0?-1.f:1.f)*(bay.right-bay.left)*.22f;
                if(room==6&&std::fabs(bx)<12)bx=bx<0?-18.f:18.f;   // its roof is split down the middle
                d.lamp(c+vec3{bx,roof,bz},y);
            }
            int runs=room==6?3:2;
            for(int k=0;k<runs;++k){
                float x0=d.rr(bay.left+2,bay.right-2);float x1=d.rr(bay.left+2,bay.right-2);
                if(room==6){x0=x0<0?std::min(x0,-13.f):std::max(x0,13.f);x1=x0+d.rr(-6,6);}
                float maxSag=std::min(4.f,roof-8.f);
                float sag=d.rr(1.5f,maxSag);
                d.bundle(c+vec3{x0,roof-.3f,bay.farZ+1},c+vec3{x1,roof-.3f,bay.nearZ-1},sag,3+(k&1));
            }
            for(int k=0;k<2;++k){vec3 top=c+vec3{d.rr(bay.left+3,bay.right-3),roof-.3f,d.rr(bay.farZ+3,bay.nearZ-3)};
                float len=d.rr(1.5f,std::min(4.f,roof-8.f));d.dangle(top,len);}
        }
    }

    // The garden's walls and the long ward's nave are split by roots
    if(room==4||room==6||room==3)
        for(const auto& bay:bays)for(float side:{-1.f,1.f}){
            if(room==3&&side>0)continue;
            float wx=side<0?bay.left+.42f:bay.right-.42f;
            for(float bz=bay.farZ+6;bz<bay.nearZ-4;bz+=room==3?40.f:16.f)
            {float jog=d.rr(-2,2),tall=d.rr(9,18);
                d.roots({wx,y,z+bz+jog},{-side,0,0},std::min(roof-2,room==3?8.f:tall));}
        }

    // Long-Term Care: the lab is starting to come loose overhead
    // (between the hanging arches, which stand at z -23, -2 and 19)
    if(room==6)for(int k=0;k<14;++k){
        static const float lo[]={-36,-19,2,23},hi[]={-27,-6,15,36};
        d.drift(c+vec3{d.rr(-20,20),d.rr(36,46),d.rr(lo[k%4],hi[k%4])},k%6);}

    if(!late && room!=4){
        // ---- the enclosed wards' windows, and the void outside them ----
        for(const auto& w:lab::windows(room))d.windowFrame(w);
        if(room==2){
            // The glass plate in Motor Assessment's floor: a steel rim, lit corners, a grid under the glass
            const AABB g=lab::glassFloor();auto m=d.kit(Mat::METAL),gl=d.kit(Mat::METAL,true);
            vec3 mid=c+(g.min+g.max)*.5f;mid.y=y;float wx=g.max.x-g.min.x,wz=g.max.z-g.min.z;
            for(float s:{-1.f,1.f}){m.box(mid+vec3{s*(wx*.5f+.15f),.015f,0},{.3f,.05f,wz+.6f},GRAPHITE);
                m.box(mid+vec3{0,.015f,s*(wz*.5f+.15f)},{wx,.05f,.3f},GRAPHITE);}
            for(float sx:{-1.f,1.f})for(float sz:{-1.f,1.f})gl.box(mid+vec3{sx*(wx*.5f+.15f),.05f,sz*(wz*.5f+.15f)},{.22f,.03f,.22f},CYAN);
            for(int k=1;k<3;++k){m.box(mid+vec3{-wx*.5f+wx*k/3,-.08f,0},{.1f,.12f,wz},DARK);m.box(mid+vec3{0,-.08f,-wz*.5f+wz*k/3},{wx,.12f,.1f},DARK);}
            // Straight down through it: debris turning, and a broken piece of the lab far below
            for(int k=0;k<7;++k){vec3 p=mid+vec3{d.rr(-14,14),d.rr(-70,-14),d.rr(-14,14)};d.drift(p,k%6);}
            d.island(mid+vec3{6,-95,-8},16,13,1);
        }
        // the dark service deck under every floor slab, seen from outside and down through the glass
        for(const auto& w:l.walls){
            const auto& q=w.box;
            if(w.hidden || std::fabs(q.max.y-y)>.01f||q.min.y<y-1.3f||q.min.y>y-.7f)continue;
            if(q.min.z<z-41||q.max.z>z+41||q.min.x<-60||q.max.x>60)continue;
            d.underside(q.min.x,q.max.x,q.min.z,q.max.z,q.min.y,d.rr(1.4f,2.2f));
        }
        // Debris adrift beyond the windows on both sides
        for(int k=0;k<18;++k){float side=k&1?-1.f:1.f;
            float x=side*d.rr(half+9,half+60),yy=d.rr(-22,room>=5?34.f:20.f),zz=d.rr(-44,44);
            d.drift(c+vec3{x,yy,zz},k%6);}
        // Broken pieces of the lab further out, and in some wards, a lit ward someone still walks
        for(int k=0;k<2;++k){float side=k?-1.f:1.f;
            float x=side*d.rr(half+30,half+46),yy=d.rr(-12,room>=5?14.f:4.f),zz=d.rr(-26,26);
            float w=d.rr(11,15),dd=d.rr(9,12);int style=(room+k)%4;
            d.island(c+vec3{x,yy,zz},w,dd,style);}
        if(room==0)d.ward(c+vec3{-half-24,-2.5f,-12},-1);
        if(room==1)d.ward(c+vec3{-half-40,7.f,18},-1);
        if(room==5)d.ward(c+vec3{half+26,5,2},1);
        if(room==6)d.ward(c+vec3{-half-25,17,-3},-1);
        if(room==0){
            // Along the lift's climb from the tower (LabIntro.h): debris the car rises past
            for(int k=0;k<10;++k){float side=k&1?-1.f:1.f;
                vec3 p{side*d.rr(14,60),d.rr(-140,-14),d.rr(20,90)};d.drift(p,k%6);}
        }
    }
    if(!late)return;

    // ---- the late wards: open edges over the void ----
    // Lit rails: the low perimeter walls and the wells' rails glow like the boards' light fences
    if(room<9){
        for(const auto& bay:bays)for(float x:{bay.left,bay.right})
            d.railGlow({x,y,z+bay.farZ},{x,y,z+bay.nearZ},.4f,1.4f,room==8?vec3{.2f,.9f,1.f}:CYAN);
        float well=room==7?10.f:6.f;
        for(float side:{-1.f,1.f}){
            d.railGlow({side*well,y,z-8},{side*well,y,z+14},.16f,1.05f,MAGENTA*.9f);
            // the well's own broken edges, seen as you lean over
            d.fracture({side*well,y,z-7.5f},{side*well,y,z+13.5f},{-side,0,0});
        }
    }
    // Broken outer edges with the void beyond, and the service deck under every slab
    for(const auto& bay:bays)for(float side:{-1.f,1.f}){
        float x=(side<0?bay.left:bay.right)+side*.45f;
        d.fracture({x,y,z+bay.farZ+.5f},{x,y,z+bay.nearZ-.5f},{side,0,0},room==9?1.2f:.8f);
    }
    for(const auto& w:l.walls){
        const auto& q=w.box;
        if(std::fabs(q.max.y-y)>.01f||q.min.y<y-1.3f||q.min.y>y-.7f)continue;
        if(q.min.z<z-75||q.max.z>z+41||q.min.x<-80||q.max.x>80)continue;
        if(room<9&&(q.min.z<z-40.5f))continue;   // not the descending passage's landing
        d.underside(q.min.x,q.max.x,q.min.z,q.max.z,q.min.y,room==9?3.2f:d.rr(1.6f,2.4f));
    }

    // Islands of the lab adrift beyond the windows, and debris turning round them
    if(room==7){
        // Most ride at eye level or above, so they show over the low perimeter wall
        const vec3 at[]={{-74,1,-24},{-90,7,14},{78,3,-8},{96,9,26},{-108,-3,-2},{110,5,-30}};
        for(int k=0;k<6;++k){float w=d.rr(11,16),dd=d.rr(9,13);d.island(c+at[k],w,dd,k%4);}
    }
    if(room==9){
        const vec3 at[]={{-94,2,22},{94,3,22},{-96,4,-46},{96,1,-46}};
        for(int k=0;k<4;++k){d.island(c+at[k],14,12,k==0||k==3?0:2);}
        // Each island's tank feeds power across to the arena's rail
        for(int k=0;k<4;++k){float s=at[k].x<0?-1.f:1.f;
            for(int j=0;j<2;++j){
                // An emitter post on the rail (solid, like the rail it stands on)
                vec3 post=c+vec3{s*76,0,std::clamp(at[k].z+(j?-5.f:5.f),-46.f,30.f)};
                d.kit(Mat::METAL).box(post+vec3{0,1.6f,0},{.4f,3.2f,.4f},GRAPHITE);
                b.solid(post.x-.2f,post.y,post.z-.2f,post.x+.2f,post.y+3.2f,post.z+.2f);
                d.kit(Mat::METAL,true).column(post+vec3{0,3.2f,0},.32f,.3f,MAGENTA,8);
                float lift=d.rr(3.5f,5.f);
                d.arc(post+vec3{0,3.4f,0},c+at[k]+vec3{-s*3.5f,lift,-1.8f+j*3.6f},j?MAGENTA:CYAN);}
        }
        // Hovering pads off the entry corners
        for(float s:{-1.f,1.f}){vec3 p=c+vec3{s*88,-1.5f,34};
            d.kit(Mat::METAL).column(p+vec3{0,-.8f,0},2.9f,.8f,GRAPHITE,28);d.pad(p,2.2f);}
        // The portal: a frame of white segments and clamps, cabled to the pylons, a vortex inside
        vec3 portal=c+vec3{0,50,-165};float R=42;
        auto w=d.kit(Mat::CLINIC),m=d.kit(Mat::METAL),g=d.kit(Mat::METAL,true);
        for(int k=0;k<48;++k){float a=k*TAU/48,chord=2*(R+2)*std::sin(TAU/96)+.1f;
            vec3 p=portal+vec3{std::cos(a)*(R+1.6f),std::sin(a)*(R+1.6f),0};
            w.box(p,{4.4f,chord,4.f},k%6==0?PANEL:WHITE,0,0,a);
            m.box(portal+vec3{std::cos(a)*(R+3.4f),std::sin(a)*(R+3.4f),0},{.6f,chord,3.f},DARK,0,0,a);
            g.box(portal+vec3{std::cos(a)*(R-.05f),std::sin(a)*(R-.05f),0},{.18f,chord,1.2f},(k&1?CYAN:VIOLET)*1.2f,0,0,a);
        }
        for(int k=0;k<8;++k){float a=k*TAU/8+.2f;vec3 p=portal+vec3{std::cos(a)*(R+2.5f),std::sin(a)*(R+2.5f),0};
            m.box(p,{6.f,3.4f,6.f},GRAPHITE,0,0,a);g.box(p+vec3{0,0,3.05f},{2.2f,.4f,.1f},MAGENTA,0,0,a);}
        // Cables off the frame to the two pylons that flank it, and loose ones hanging
        for(int k=0;k<6;++k){float a=3.6f+k*.38f;vec3 from=portal+vec3{std::cos(a)*(R+3),std::sin(a)*(R+3),0};
            vec3 to=c+vec3{(k<3?-1.f:1.f)*36,d.rr(20,40),-118};float sag=d.rr(6,14);d.bundle(from,to,sag,2);}
        for(int k=0;k<10;++k){float a=3.4f+k*.29f,len=d.rr(6,22);d.dangle(portal+vec3{std::cos(a)*(R+3.5f),std::sin(a)*(R+3.5f),0},len,.12f);}
        l.story.labPortal=portal;l.story.labPortalRadius=R;
    }
    int drifting=room==9?44:32;
    for(int k=0,tries=0;k<drifting&&tries<400;++tries){
        float side=k&1?-1.f:1.f;float x=side*d.rr(half+12,half+70);
        float yy=d.rr(-14,room==9?40.f:24.f);float zz=d.rr(-60,50);
        if(room==9&&k%4==0){x=d.rr(-60,60);yy=d.rr(28,55);}   // over the floor too, well above any jump
        vec3 p=c+vec3{x,yy,zz};bool crowded=false;
        for(const auto& t:l.story.labTanks)if(t.room==room&&glm::length(t.base-p)<14)crowded=true;   // the islands
        if(crowded)continue;
        d.drift(p,k%6);++k;
    }
}
} // namespace labdress
