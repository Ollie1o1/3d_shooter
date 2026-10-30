#pragma once
#include "world/Level.h"
#include "ui/PixelFont.h"
#include <cstdio>

// The Cradle's room identities. Structural dressing stays outside the playable
// lanes; substantial floor equipment receives visible, matching collision.
inline void buildCradleAtmosphere(LevelBuilder& B,float A) {
    using glm::vec3;auto& L=B.L;auto& S=L.story;
    const float D=-27.4f,P=-40.f,F=A-7.f,V=A-4.f,T=V+39.f;
    const vec3 iron{.27f,.32f,.34f},dark{.09f,.13f,.15f},silver{.5f,.54f,.53f},
        rust{.4f,.25f,.15f},cream{.65f,.67f,.58f},yellow{.7f,.51f,.13f},
        amber{1.2f,.76f,.3f};
    B.mat=Mat::METAL;
    auto detail=[&](vec3 lo,vec3 hi,vec3 c){return B.prop(lo.x,lo.y,lo.z,hi.x,hi.y,hi.z,c);};
    auto lamp=[&](vec3 lo,vec3 hi,vec3 c){B.neon(lo.x,lo.y,lo.z,hi.x,hi.y,hi.z,c);};
    auto pipe=[&](vec3 a,vec3 b,float r,vec3 c){B.kit().rod(a,b,r,c,10);};
    // Stencilled text receives the room's lighting; it doesn't add a neon light
    // per pixel or compete with the cyan floating readable lamps.
    auto stencil=[&](const char* text,vec3 at,vec3 right,float pixel,vec3 color){
        for(int i=0;text[i];++i) {
            unsigned char ch=text[i];if(ch<32||ch>127)continue;
            for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(PIXEL_FONT[ch-32][y]&(1<<(4-x))) {
                vec3 p=at+right*((i*6+x)*pixel)-vec3{0,y*pixel,0};
                detail(p,p+vec3{pixel*.83f},color);
            }
        }
    };
    auto truss=[&](float x0,float x1,float y,float z,float depth){
        for(float h:{0.f,depth})detail({x0,y+h,z-.18f},{x1,y+h+.18f,z+.18f},iron);
        for(float x=x0;x<x1-.1f;x+=4.f) {
            float end=std::min(x+4.f,x1);
            pipe({x,y+.18f,z},{end,y+depth,z},.08f,silver);
            pipe({x,y+depth,z},{end,y+.18f,z},.08f,iron);
        }
    };
    auto extractor=[&](vec3 p,float yaw,float scale=1.f){
        B.place("cradle_extractor",p,yaw,scale);
        float a=glm::radians(yaw);vec3 normal{std::sin(a),0,std::cos(a)};
        vec3 center=p+vec3{0,1.5f*scale,0}+normal*(.3f*scale);
        L.fans.push_back({center,1.0f*scale,std::fabs(normal.x)>.5f?0:2,{.09f,.12f,.12f}});
    };
    auto pump=[&](vec3 p){
        B.place("cradle_pump_skid",p);
        B.solid(p.x-1.75f,p.y,p.z-.82f,p.x+1.75f,p.y+1.8f,p.z+.82f);
        B.solid(p.x+1.04f,p.y+1.8f,p.z-.2f,p.x+1.48f,p.y+2.65f,p.z+.2f);
        lamp(p+vec3{-1.5f,2.8f,-.3f},p+vec3{1.5f,2.86f,-.1f},amber*.55f);
    };
    auto floorBand=[&](float x0,float x1,float z,float y,vec3 c){detail({x0,y+.008f,z},{x1,y+.018f,z+.09f},c);};

    // 1 / FURNACE: heavy exhaust, hot refractory panels, cable feeds and
    // labelled conveyor stations. The boot's ceiling view has a real silhouette.
    for(float z:{-10.f,-30.f,-49.f}) {
        truss(-14,14,9.4f,z,1.8f);
        for(float x:{-12.f,12.f}) {
            pipe({x,7.2f,z},{x,12.7f,z},.52f,rust);
            for(float y:{8.f,10.f,12.f})B.kit().column({x,y,z},.65f,.13f,iron,12);
        }
    }
    for(float z:{-6.f,-22.f,-46.f}) {
        extractor({-14.55f,3.4f,z},90,1.15f);
        extractor({14.55f,4.4f,z},-90,1.15f);
        detail({-14.7f,.3f,z-2},{-14.3f,2.7f,z+2},rust);
        for(int k=0;k<6;++k)detail({-14.25f,.5f+k*.32f,z-1.7f},{-14.2f,.63f+k*.32f,z+1.7f},iron);
    }
    for(float x:{-7.f,0.f,7.f}) {
        char name[32];std::snprintf(name,sizeof(name),"REJECT / %02d",(int)(x+7)/7+1);
        stencil(name,{x+1.2f,5.3f,8.86f},{-1,0,0},.075f,cream);
    }
    floorBand(9,14,-26,0,yellow);floorBand(9,14,-38,0,yellow);
    S.vents.push_back({{14.1f,5.4f,-45},0,7.f,1.f});

    // 2 / SCRAP: huge bare heap faces become recognisable compressed salvage.
    // Keep their exact collision and flat jump surfaces.
    for(int i=0;i<(int)L.walls.size();++i) {
        auto& w=L.walls[i];auto b=w.box;
        if(w.hidden||w.dynamic||b.min.y>P+.01f||b.max.y<P+1.f||b.max.y>D+.01f)continue;
        if(b.min.x<21.9f||b.max.x>56.6f||b.min.z< -58||b.max.z> -4.01f)continue;
        if(b.max.x-b.min.x<2.f||b.max.z-b.min.z<2.f)continue;
        B.dress(i,"cradle_compacted_scrap",(i%4)*90.f);
    }
    // Recessed service hardware, cable runs and clear edge paint around the climb.
    for(float z:{-54.f,-38.f,-16.f}) {
        extractor({57.65f,-26.f,z},-90,1.3f);
        for(float y:{-24.f,-21.f})pipe({18.2f,y,z},{57.7f,y,z},.12f,iron);
    }
    stencil("RECLAMATION / 02",{18.4f,-25.f,-25},{0,0,-1},.13f,cream);
    stencil("SERVICE EXIT",{35,-24.5f,-4.65f},{-1,0,0},.11f,cream);

    // 3 / DUCTS: pressure equipment, exposed services, bolted panel joints.
    // Round tanks and pipe elbows break the corridor's rectangular silhouette.
    for(float z:{0.f,5.f,10.f})for(float x:{30.35f,33.65f}) {
        pipe({x,D+2.5f,z-1.8f},{x,D+2.5f,z+1.8f},.12f,rust);
        B.kit().column({x,D+2.37f,z},.18f,.26f,iron,8);
    }
    pump({30.8f,D,28.8f});
    for(float z:{14.f,27.f}) {
        pipe({24.3f,D+5.25f,z},{43.7f,D+5.25f,z},.24f,iron);
        pipe({43.7f,D+1.3f,z},{43.7f,D+5.25f,z},.24f,rust);
    }
    extractor({43.7f,D+2.4f,20},-90,.7f);
    for(float z:{32.f,35.f,43.f}) {
        detail({36.1f,D+.008f,z},{39.9f,D+.018f,z+.1f},cream);
        detail({36.1f,D+.008f,z+.18f},{36.22f,D+.018f,z+.65f},yellow);
    }
    stencil("LOW CLEARANCE",{39.75f,D+2.65f,37.72f},{-1,0,0},.042f,cream);
    stencil("PRESSURE / 03",{26.1f,D+4.1f,12.23f},{1,0,0},.095f,cream);
    B.place("cradle_service_cart",{32.4f,D,51.5f},30,.9f);
    detail({30.1f,D+1.4f,47},{30.22f,D+1.55f,56},rust);
    detail({30.1f,D+2.8f,47},{30.22f,D+2.95f,56},iron);
    S.vents.push_back({{43.2f,D+3.7f,26.8f},2,5.5f,.8f});

    // 4 / ASSEMBLY: a readable forest of fabrication arches under deep trusses,
    // with services feeding each line and warm work lights close to the floor.
    for(auto& piston:L.pistons)if(piston.top.x>65.f&&piston.top.z>=32.f&&piston.top.z<=56.f)
        piston.top.y=F+7.3f; // the press head finishes above the belt, rather than below the floor
    for(auto p:S.assemblyUnits) {
        B.place("cradle_fabrication_frame",p-vec3{0,.32f,0});
        floorBand(p.x-2.5f,p.x+2.5f,p.z-1.5f,F,yellow);
        floorBand(p.x-2.5f,p.x+2.5f,p.z+1.5f,F,yellow);
    }
    for(float z:{29.f,43.f,57.f,73.f}) {
        truss(57,107,F+17.5f,z,2.2f);
        for(float x:{57.f,107.f}) {
            detail({x-.3f,F+1,z-.8f},{x+.3f,F+15.7f,z+.8f},iron);
            for(int k=0;k<6;++k)detail({x-.32f,F+2+k*2.f,z-.9f},{x+.32f,F+2.18f+k*2.f,z+.9f},silver);
        }
    }
    // The entry catwalk has an operator's edge and a generous six-metre
    // opening onto the floor, rather than an unmarked eight-metre slab.
    for(auto span:{glm::vec2{48,57},glm::vec2{63,70}}) {
        for(float z=span.x;z<=span.y;z+=3.f) {
            B.wall(63.9f,A,z-.06f,64.02f,A+1.12f,z+.06f,iron);
        }
        B.wall(63.9f,A+1.02f,span.x,64.02f,A+1.12f,span.y,yellow);
    }
    lamp({62.5f,A+2.7f,62.5f},{63.8f,A+2.76f,65},amber*.45f);
    for(float x:{62.f,77.f,92.f,103.f})extractor({x,F+7.4f,24.4f},0,2.f);
    for(float x:{70.f,81.f,92.f}) {
        pipe({x,F+7,27},{x,F+7,65},.22f,rust);
        for(float z:{32.f,44.f,56.f})pipe({x,F+5.2f,z},{x,F+7,z},.13f,iron);
        char label[24];std::snprintf(label,sizeof(label),"FABRICATION %02d",(int)((x-70)/11)+1);
        stencil(label,{x-3,F+9.8f,24.4f},{1,0,0},.12f,cream);
    }
    pump({61,F,31});pump({104,F,32});
    B.place("cradle_service_cart",{59.4f,A,64},90);
    for(float x:{65.f,76.f,87.f,98.f}) {
        extractor({x,F+9,77.55f},180,1.4f);
        detail({x-2,F+.01f,68.5f},{x+2,F+.02f,68.6f},cream);
    }
    S.vents.push_back({{61,F+2.7f,31},3,6.f,.2f});
    S.vents.push_back({{104,F+2.7f,32},3,7.5f,3.f});
    stencil("ARCHIVE <",{63,A+2.8f,77.8f},{-1,0,0},.09f,cream);

    // 5 / ARCHIVE: close, ordered surfaces and quiet service aisles. Existing
    // racks face the player now; their former bright faces pointed into a wall.
    for(int i=0;i<(int)L.walls.size();++i) {
        const auto& b=L.walls[i].box;
        if(b.min.z==81.f&&b.max.z==83.f&&b.min.y==A&&b.max.y>A+5.f)
            B.dress(i,"cradle_archive_bank",180);
    }
    for(float x:{25.5f,35.5f,45.5f}) {
        int w=B.wall(x-1.5f,A,65.3f,x+1.5f,A+4.2f,66.8f,dark);
        B.dress(w,"cradle_archive_bank");
        floorBand(x-2,x+2,67.4f,A,cream);
        for(float z:{68.f,79.6f})detail({x-2,A+.008f,z},{x+2,A+.018f,z+.12f},silver);
    }
    for(float x:{21.f,31.f,41.f,51.f}) {
        detail({x,A+5.7f,64.3f},{x+.12f,A+6.f,83.7f},iron);
        pipe({x,A+6.1f,64.3f},{x,A+6.1f,83.7f},.07f,dark);
    }
    // Sealed memory drawers turn the blank north wall into a dense archive.
    for(int k=0;k<12;++k) {
        float x=19+k*2.9f;
        for(int row=0;row<2;++row) {
            float y=A+1.4f+row*2.f;
            detail({x,y,83.65f},{x+2.3f,y+1.45f,83.88f},iron);
            detail({x+.8f,y+.4f,83.58f},{x+1.5f,y+.48f,83.65f},cream);
        }
    }
    B.place("cradle_service_cart",{19,A,67.5f},15,.8f);
    stencil("RETENTION / 05",{41,A+5.9f,83.5f},{-1,0,0},.12f,cream);
    stencil("VAULT",{16.3f,A+5.8f,77.3f},{0,0,-1},.13f,cream);

    // 6 / VAULT: individual clinical cradles, coolant arteries and bay numbers.
    int bay=0;
    for(auto p:S.pods) {
        B.place("cradle_pod_spine",p);
        pipe(p+vec3{-1.1f,4.4f,-.85f},p+vec3{-1.1f,8.6f,-.85f},.1f,silver);
        char label[20];std::snprintf(label,sizeof(label),"BAY %02d",++bay);
        stencil(label,{p.x-.9f,V+5.5f,p.z+.2f},{1,0,0},.055f,cream);
        floorBand(p.x-1.7f,p.x+1.7f,p.z+1.6f,V,cream);
        detail({p.x-2.7f,V+.004f,p.z-2.7f},{p.x+2.7f,V+.012f,p.z+2.7f},{.32f,.39f,.43f});
        for(float side:{-1.f,1.f}) {
            float x=p.x+side*1.26f;
            B.wall(x-.07f,V,p.z-1.3f,x+.07f,V+6.f,p.z-1.15f,silver);
            detail({x-.04f,V+1,p.z-1.13f},{x+.04f,V+5.5f,p.z-1.07f},iron);
            detail({p.x+side*2.65f-.08f,V+.016f,p.z+1.9f},{p.x+side*2.65f+.08f,V+.026f,p.z+2.65f},cream);
        }
        detail({p.x-1.5f,V+5.8f,p.z-1.7f},{p.x+1.5f,V+6.1f,p.z-1.4f},silver);
    }
    for(float x:{-31.1f,-18.1f,-5.1f}) {
        pipe({x,V+9.1f,28},{x,V+9.1f,82},.28f,iron);
        for(float z:{37.f,49.f,61.f,75.f})pipe({x,V+8.6f,z-.85f},{x,V+9.1f,z-.85f},.17f,silver);
    }
    for(float z:{32.f,56.f,80.f}) {
        extractor({-37.6f,V+5.5f,z},90,1.15f);
        detail({-37.7f,V+.01f,z-1},{-32.7f,V+.02f,z-.85f},cream);
    }
    stencil("KEEP",{-37.5f,V+5.7f,31.7f},{0,0,-1},.15f,cream);
    stencil("MEASURE",{-37.5f,V+5.7f,80.f},{0,0,-1},.12f,cream);
    // The stopped dial is environmental evidence, not an extra timed puzzle.
    B.kit().column({-35,V+.02f,69},1.1f,.025f,dark,24);
    detail({-35.04f,V+.05f,68.2f},{-34.96f,V+.08f,69.1f},cream);
    detail({-35,V+.05f,68.96f},{-34.3f,V+.08f,69.04f},yellow);
    S.vents.push_back({{-37.1f,V+.4f,55},5,8.f,2.f});

    // 7 / FREIGHT: steel cross-bracing, cable bundles, maintenance grilles and
    // visible machinery at each stop make the ascent measure its own progress.
    for(int k=0;k<3;++k) {
        float y=V+k*13.f;
        truss(-35.6f,-14.4f,y+9.5f,-3.4f,2.f);
        for(float z:{-.5f,14.f}) {
            pipe({-35.7f,y+1,z},{-35.7f,y+11,z+1.2f},.12f,silver);
            extractor({-35.6f,y+4,z},90,.65f);
        }
        for(float x:{-34.5f,-15.5f})for(float dx:{0.f,.25f,.5f})
            pipe({x+dx,y,-3.6f},{x+dx,y+13,-3.6f},.055f,rust);
    }
    stencil("LOAD / ONE",{-29,V+3.7f,16.45f},{1,0,0},.085f,cream);

    // 8 / INSPECTION: a controlled sequence of booths and floor calibration
    // marks; mechanisms are above or outside the actual scanner crossing lane.
    for(int k=0;k<6;++k) {
        float x=-4.f+k*9.f;
        for(float z:{.15f,15.55f}) {
            detail({x-1.25f,T+3.2f,z},{x+1.25f,T+6.2f,z+.25f},dark);
            for(int row=0;row<5;++row)detail({x-1,T+3.5f+row*.44f,z+.27f},{x+1,T+3.64f+row*.44f,z+.3f},iron);
        }
        detail({x-.15f,T+.008f,1},{x+.15f,T+.018f,15},cream);
        for(float z:{5.8f,10.2f})detail({x-1.3f,T+.008f,z},{x+1.3f,T+.018f,z+.12f},yellow);
        char label[24];std::snprintf(label,sizeof(label),"SCAN %02d",k+1);
        stencil(label,{x-.9f,T+7.55f,1.1f},{1,0,0},.055f,cream);
        pipe({x,T+7.4f,1},{x,T+7.4f,15},.1f,iron);
    }
    for(float x:{8.f,26.f,44.f})extractor({x,T+3.3f,.35f},0,.75f);
    stencil("CLEAN / CONTINUE",{4,T+5.8f,.6f},{1,0,0},.11f,cream);
    stencil("REJECT / RETURN",{44,T+5.8f,15.5f},{-1,0,0},.11f,cream);
    S.vents.push_back({{26,T+4.7f,.8f},7,7.f,1.8f});

    // 9 / SORTER: a cathedral of inspection hardware around the fighting
    // floor. Wall ribs and overhead cages leave the cross, sides and rails open.
    for(float z:{-44.f,-30.f,-16.f,-2.f,12.f}) {
        truss(57,107,T+21,z,1.4f);
        for(float x:{56.25f,107.45f}) {
            detail({x,T+1,z-.7f},{x+.3f,T+20,z+.7f},iron);
            for(float y:{4.f,9.f,14.f,19.f})detail({x-.08f,T+y,z-.85f},{x+.38f,T+y+.2f,z+.85f},silver);
        }
    }
    for(float x:{68.f,96.f})for(float z:{-38.f,-8.f}) {
        detail({x-2,T+17,z-1.8f},{x+2,T+17.2f,z+1.8f},dark);
        detail({x-2,T+20,z-1.8f},{x+2,T+20.2f,z+1.8f},iron);
        for(float sx:{-2.f,2.f})for(float dz:{-1.8f,0.f,1.8f})
            detail({x+sx-.04f,T+17,z+dz-.04f},{x+sx+.04f,T+20,z+dz+.04f},silver);
        B.place("cradle_compacted_scrap",{x,T+17.25f,z},0,1.4f);
        pipe({x,T+20.2f,z},{x,T+23.5f,z},.06f,iron);
    }
    for(float z:{-35.f,-10.f,10.f}) {
        extractor({107.5f,T+8,z},-90,1.6f);
        extractor({56.5f,T+8,z},90,1.6f);
    }
    stencil("ACCEPTANCE",{71,T+11.5f,-49.65f},{1,0,0},.25f,cream);
    stencil("RETURN TO LINE",{107.1f,T+4.f,-35},{0,0,1},.17f,cream);
    for(float z:{-40.f,-20.f,0.f})floorBand(57.5f,61.5f,z,T,yellow);

    // Door mechanisms explain the interlocks without reducing their openings.
    for(const auto& door:L.doors) {
        const auto& b=door.closed;float y=door.baseY+door.height;
        if(door.alongX()) {
            float z=(b.min.z+b.max.z)*.5f;
            detail({b.min.x-.22f,y+.12f,z-.3f},{b.max.x+.22f,y+.52f,z+.3f},iron);
            for(float x:{b.min.x-.18f,b.max.x+.18f})
                pipe({x,y+.31f,z-.4f},{x,y+.31f,z+.4f},.16f,rust);
        } else {
            float x=(b.min.x+b.max.x)*.5f;
            detail({x-.3f,y+.12f,b.min.z-.22f},{x+.3f,y+.52f,b.max.z+.22f},iron);
            for(float z:{b.min.z-.18f,b.max.z+.18f})
                pipe({x-.4f,y+.31f,z},{x+.4f,y+.31f,z},.16f,rust);
        }
    }

    // Exterior: ash banks frame a clear approach to the tower rather than a
    // tiled empty rectangle. They sit outside the walk-out corridor.
    B.mat=Mat::ROCK;
    for(int k=0;k<8;++k)for(float x:{59.f,104.f})
        B.kit().rock({x,T-.2f,-67-k*4.f},1.7f+(k%3)*.5f,.8f+(k%2)*1.1f,{.26f,.2f,.18f},240+k*3+(int)x,.45f,7);
    B.mat=Mat::METAL;

    // Different working surfaces give the rooms scale at eye level: rough
    // production concrete, an ordered Archive aisle and pale clinical tiles.
    for(auto& w:L.walls) {
        const auto& b=w.box;
        if(b.min.x==56.f && b.max.x==108.f && b.min.z==24.f && b.max.z==78.f && b.max.y==F) {
            w.mat=Mat::CONCRETE;w.color={.34f,.32f,.28f};
        }
        if(b.min.x==16.f && b.max.x==56.f && b.min.z==64.f && b.max.z==84.f && b.max.y==A) {
            w.mat=Mat::PANEL;w.color={.32f,.36f,.37f};
        }
        if(b.min.x==-38.f && b.max.x==16.f && b.min.z==26.f && b.max.z==84.f && b.max.y==V) {
            w.mat=Mat::LIMESTONE;w.color={.38f,.43f,.47f};
        }
        if(b.min.x==-14.f && b.max.x==56.f && b.min.z==0.f && b.max.z==16.f && b.max.y==T) {
            w.mat=Mat::CONCRETE;w.color={.26f,.29f,.3f};
        }
    }
    B.mat=Mat::PANEL;
    detail({18,A+.003f,73},{54,A+.007f,76},{.16f,.21f,.23f});
    detail({50,A+.003f,68},{54,A+.007f,79},{.16f,.21f,.23f});
    B.mat=Mat::METAL;
    floorBand(18,54,72.85f,A,silver);floorBand(18,54,76.05f,A,silver);
    for(float z:{31.f,43.f,55.f,67.f,79.f}) {
        detail({4.5f,V+.006f,z},{7.5f,V+.014f,z+2},{.56f,.6f,.59f});
        detail({4.7f,V+.015f,z+.3f},{5.1f,V+.022f,z+.6f},yellow);
    }

    // Lighting, air and acoustic identities are deliberately distinct. All
    // backgrounds remain quieter than the combat tells in the normal mixer.
    L.arenas[0].theme.skyAmb={.22f,.19f,.17f};L.arenas[0].theme.groundAmb={.12f,.09f,.065f};
    L.arenas[1].theme.skyAmb={.23f,.23f,.24f};L.arenas[1].theme.groundAmb={.14f,.12f,.1f};L.arenas[1].theme.fogDensity=.011f;
    L.arenas[2].theme.skyAmb={.23f,.29f,.27f};L.arenas[2].theme.groundAmb={.12f,.16f,.14f};
    L.arenas[3].theme.skyAmb={.42f,.39f,.33f};L.arenas[3].theme.groundAmb={.22f,.19f,.15f};L.arenas[3].theme.fogDensity=.006f;
    L.arenas[3].tone=RoomTone::FOUNDRY;L.arenas[3].space=ReverbSpace::HALL;L.arenas[3].ambient=Ambient::DUST;
    L.arenas[4].theme.skyAmb={.3f,.38f,.43f};L.arenas[4].theme.groundAmb={.17f,.23f,.27f};L.arenas[4].theme.fogDensity=.005f;
    L.arenas[4].tone=RoomTone::CORE;L.arenas[4].space=ReverbSpace::METAL;L.arenas[4].ambient=Ambient::DUST;
    L.arenas[5].theme.skyAmb={.34f,.42f,.5f};L.arenas[5].theme.groundAmb={.19f,.25f,.31f};L.arenas[5].theme.fogDensity=.007f;L.arenas[5].theme.fogColor={.08f,.11f,.14f};
    L.arenas[5].tone=RoomTone::SANCTUM;L.arenas[5].space=ReverbSpace::HALL;L.arenas[5].ambient=Ambient::STEAM;
    L.arenas[6].theme.skyAmb={.26f,.28f,.29f};L.arenas[6].theme.groundAmb={.13f,.15f,.17f};L.arenas[6].theme.fogDensity=.006f;
    L.arenas[6].tone=RoomTone::DESCENT;L.arenas[6].space=ReverbSpace::SHAFT;L.arenas[6].ambient=Ambient::DUST;
    L.arenas[7].theme.skyAmb={.3f,.34f,.35f};L.arenas[7].theme.groundAmb={.16f,.19f,.2f};L.arenas[7].theme.fogDensity=.009f;
    L.arenas[7].tone=RoomTone::ORRERY;L.arenas[7].ambient=Ambient::DUST;
    L.arenas[8].theme.fogDensity=.008f;
    L.arenas[8].tone=RoomTone::FOUNDRY;L.arenas[8].space=ReverbSpace::HALL;L.arenas[8].ambient=Ambient::EMBERS;
}
