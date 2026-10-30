#pragma once
#include "world/Level.h"
#include "core/StoryText.h"
#include "ui/PixelFont.h"
#include <cstdio>

// Chapter 2: a continuous, newly authored wasteland road. Y is up; the route
// folds west into the relay, east across the span and north toward the tower.
inline LevelData buildWasteland() {
    using glm::vec3;using W=WaveEntry;
    LevelData L;LevelBuilder B{L};auto& S=L.story;
    S.built=true;S.chapter=2;S.nextChapter=0;S.title=S.nextName="THE DEAD SPAN";
    S.startWeapons=7;S.startGrapple=true;S.pitSection=-1;S.bossSection=6;
    const vec3 ash{.45f,.4f,.35f},rock{.32f,.28f,.25f},road{.27f,.28f,.27f},
        steel{.29f,.32f,.31f},rust{.44f,.27f,.16f},dark{.1f,.13f,.14f},
        paint{.68f,.61f,.43f},cyan{.28f,.85f,1.f},warm{1.3f,.85f,.38f};
    auto box=&LevelBuilder::aabb;
    // Only the bridge has a void floor. Outdoor enemies and flying threats
    // need the road's actual elevation, rather than the canyon's bottom.
    L.basins.push_back({box(-88,-12,-126,-28,-12,-76),-12});
    L.basins.push_back({box(-100,-5,-64,-42,-5,-26),-5});
    auto solid=[&](float x0,float y0,float z0,float x1,float y1,float z1,vec3 c){return B.wall(x0,y0,z0,x1,y1,z1,c);};
    auto detail=[&](float x0,float y0,float z0,float x1,float y1,float z1,vec3 c){return B.prop(x0,y0,z0,x1,y1,z1,c);};
    auto floor=[&](float x0,float z0,float x1,float z1,float y){B.mat=Mat::ROCK;solid(x0,y-.6f,z0,x1,y,z1,ash);B.mat=Mat::METAL;};
    auto lamp=[&](vec3 p,float yaw=0.f){B.place("canal_lamp",p,yaw,.75f);B.neon(p.x-.3f,p.y+2.3f,p.z-.15f,p.x+.3f,p.y+2.4f,p.z+.15f,warm*.55f);};
    auto stencil=[&](const char* text,vec3 p,vec3 right,float pixel){
        for(int i=0;text[i];++i) {
            unsigned char ch=text[i];if(ch<32||ch>127)continue;
            for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(PIXEL_FONT[ch-32][y]&(1<<(4-x))) {
                vec3 a=p+right*((i*6+x)*pixel)-vec3{0,y*pixel,0};
                detail(a.x,a.y,a.z,a.x+pixel*.83f,a.y+pixel*.83f,a.z+pixel*.83f,paint);
            }
        }
    };
    auto sign=[&](const char* text,vec3 p,vec3 right,float pixel){
        float width=std::strlen(text)*6*pixel;
        vec3 a=p-right*.3f,b=p+right*(width+.3f);
        detail(std::min(a.x,b.x)-.1f,p.y-7*pixel-.25f,std::min(a.z,b.z)-.1f,
               std::max(a.x,b.x)+.1f,p.y+.35f,std::max(a.z,b.z)+.1f,dark);
        stencil(text,p+vec3{0,0,.12f},right,pixel);
    };
    auto terminal=[&](vec3 p,const char* title,const char* line){
        B.place("console",p,180,.6f);
        solid(p.x-.65f,p.y,p.z-.4f,p.x+.65f,p.y+1.1f,p.z+.4f,dark);
        B.neon(p.x-.45f,p.y+1.3f,p.z+.1f,p.x+.45f,p.y+1.7f,p.z+.13f,cyan*.45f);
        S.terminals.push_back({p,title,line});
    };
    auto pylon=[&](vec3 p,float yaw){B.place("waste_pylon",p,yaw);solid(p.x-1.4f,p.y,p.z-1.4f,p.x+1.4f,p.y+1.1f,p.z+1.4f,rock);};
    auto hauler=[&](float x,float z,float yaw){
        bool turn=std::fabs(std::sin(glm::radians(yaw)))>.5f;
        float rx=turn?3.3f:1.9f,rz=turn?1.9f:3.3f;
        int id=solid(x-rx,0,z-rz,x+rx,2.2f,z+rz,steel);
        B.dress(id,"waste_hauler",yaw);
    };
    auto reel=[&](vec3 p){int id=solid(p.x-1.15f,p.y,p.z-1.15f,p.x+1.15f,p.y+2.25f,p.z+1.15f,rust);B.dress(id,"waste_cable_reel");};
    auto ramp=[&](AABB area,int axis,float from,float to){
        L.ramps.push_back({area,axis,from,to});
        float run=area.max[axis]-area.min[axis],rise=to-from;
        float angle=std::atan2(rise,run),width=area.max[axis==0?2:0]-area.min[axis==0?2:0];
        vec3 center=(area.min+area.max)*.5f;center.y=(from+to)*.5f-.25f/std::cos(angle);
        // Local X points along +X or +Z; its roll follows the ground plane.
        B.mat=Mat::CONCRETE;
        B.kit().box(center,{std::sqrt(run*run+rise*rise),.5f,width},road,axis==0?0.f:-1.5707963f,0,angle);
        B.mat=Mat::METAL;
    };
    auto boundaryX=[&](float x0,float x1,float z,float y,std::vector<LevelBuilder::Gap> gaps={}){
        B.mat=Mat::ROCK;B.wallX(x0,x1,z-.35f,z+.35f,y,y+3.2f,rock,gaps);B.mat=Mat::METAL;
    };
    auto boundaryZ=[&](float z0,float z1,float x,float y,std::vector<LevelBuilder::Gap> gaps={}){
        B.mat=Mat::ROCK;B.wallZ(z0,z1,x-.35f,x+.35f,y,y+3.2f,rock,gaps);B.mat=Mat::METAL;
    };
    auto room=[&](const char* name,const char* subtitle,AABB zone,vec3 start,float yaw,RoomTone tone=RoomTone::SPAN){
        Arena a;a.name=name;a.subtitle=subtitle;a.zone=zone;a.bounds=zone;
        a.bounds.min.y=start.y;a.playerStart=start;a.startYaw=yaw;a.tone=tone;a.space=ReverbSpace::OPEN;
        a.ambient=Ambient::ASH;a.maxAlive=99;a.damageScale=.9f;
        a.theme=Theme{{.14f,.055f,.065f},{.55f,.23f,.14f},{.24f,.18f,.15f},
            glm::normalize(vec3{.3f,.28f,-.8f}),{1.3f,.72f,.3f},.035f,.015f,
            {.19f,.09f,.075f},.05f,glm::normalize(vec3{-.3f,-.7f,.35f}),{.9f,.72f,.58f},
            {.37f,.31f,.28f},{.2f,.17f,.14f},{.3f,.2f,.17f},.0038f};
        return a;
    };

    // 1. ASH APPROACH: a quiet factory threshold, a road and a visible destination.
    floor(-24,0,24,55,0);
    boundaryZ(0,55,-24,0);boundaryZ(0,55,24,0);boundaryX(-24,24,55,0);
    boundaryX(-24,24,0,0,{{-6,6,0,6}});
    auto approach=room("THE ASH APPROACH","FACTORY SERVICE ROAD",box(-24,-1,0,24,38,55),{0,0,48},-90);
    approach.scripted=true;L.arenas.push_back(approach);
    B.mat=Mat::CONCRETE;detail(-6,.008f,0,6,.018f,55,road);B.mat=Mat::METAL;
    for(float z=4;z<52;z+=7)detail(-.1f,.025f,z,.1f,.035f,z+2.5f,paint);
    terminal({9,0,37},"EXTERNAL TRANSIT","CREW ACCESS: RELAY 02, THEN THE LOWER SPAN. MAIN DECK CLOSED AFTER SUBSIDENCE.");
    pylon({-17,0,13},-12);lamp({7,0,46});lamp({-8,0,5});
    sign("TRANSIT / 01",{11,4.5f,54.4f},{-1,0,0},.14f);
    // The factory behind is real exterior mass, not a flat backdrop.
    B.mat=Mat::CONCRETE;
    solid(-23,0,54.8f,-7,20,64,rock);solid(7,0,54.8f,23,20,64,rock);
    solid(-7,7,55,7,20,64,dark);
    for(float x:{-17.f,-10.f,10.f,17.f}) {
        detail(x-.3f,2,54.4f,x+.3f,19,54.8f,steel);
        B.place("cradle_extractor",{x,9,54.4f},180,1.6f);
    }
    B.mat=Mat::METAL;

    // 2. DEAD CONVOY: vehicles describe the failed evacuation and form cover.
    floor(-42,-66,42,0,0);
    boundaryZ(-66,0,-42,0,{{-56,-46,0,5}});boundaryZ(-66,0,42,0);
    boundaryX(-42,42,-66,0); // the road diverts through the relay to the west
    int convoyEntry=B.doorway(true,-5.8f,5.8f,-.35f,.35f,0,6,cyan);
    int convoyExit=B.doorway(false,-55.8f,-46.2f,-42.35f,-41.65f,0,5,cyan,true);
    auto convoy=room("THE DEAD CONVOY","ABANDONED FREIGHT",box(-42,-1,-66,42,38,0),{0,0,-5},-90,RoomTone::YARD);
    convoy.hasTrigger=true;convoy.trigger=box(-41,-1,-65,41,7,-13);convoy.entryGate=convoyEntry;convoy.exitDoor=convoyExit;
    convoy.waves={{W(EnemyType::SENTINEL,0,{{30,0,-55},{-32,0,-21}}),
                   W(EnemyType::RIPPER,0,{{26,0,-47},{-29,0,-42}}),
                   W(EnemyType::SERAPH,0,{{-12,8,-38}}),W(EnemyType::SHIELDBEARER,0,{{-8,0,-52}})}};
    L.arenas.push_back(convoy);
    hauler(-14,-29,0);hauler(12,-43,90);hauler(28,-18,0);reel({-27,0,-54});
    pylon({-34,0,-12},-10);pylon({34,0,-40},12);
    // A fallen freight canopy offers a sheltered flank against the convoy beam.
    // Both ends stay open, so it is a route rather than a camping cupboard.
    solid(-38,0,-40,-37.5f,3.6f,-27,steel);
    solid(-38,3.6f,-40,-27,3.9f,-27,dark);
    for(float z:{-39.f,-28.f})solid(-27.4f,0,z-.2f,-27,3.6f,z+.2f,steel);
    for(float z=-39;z<-27;z+=3)detail(-37,3.35f,z,-28,3.5f,z+.15f,rust);
    S.wrecks.push_back({{25,0,-11},3.14f,9,-1,storytext::spanLogs[0],1});
    L.placedPickups.push_back({{33,0,-7},1});
    sign("RELAY <",{-41.4f,4.7f,-45.5f},{0,0,-1},.18f);

    // 3. BURIED RELAY: descend into warmth, fight, and return to daylight.
    floor(-88,-64,-42,-26,-5);
    B.mat=Mat::CONCRETE;
    B.wallZ(-64,-26,-42.3f,-41.7f,-5,7,rock,{{-56,-46,0,5}});
    B.wallZ(-64,-26,-88.3f,-87.7f,-5,7,rock,{{-42,-34,-5,-1}});
    B.wallX(-88,-42,-64.3f,-63.7f,-5,7,rock,{{-79,-69,-5,0}});
    solid(-88,-5,-26.3f,-42,7,-25.7f,rock);
    solid(-88,7,-64,-42,7.5f,-26,dark);
    ramp(box(-54,-5,-56,-42,0,-46),0,-5,0);
    ramp(box(-79,-5,-76,-69,0,-64),2,0,-5);
    // Closed retaining sides protect the ramp and expose a broad, continuous threshold.
    solid(-79.5f,-6,-76,-79,5,-64,rock);solid(-69,-6,-76,-68.5f,5,-64,rock);
    B.mat=Mat::METAL;
    int relayExit=B.doorway(true,-78.8f,-69.2f,-64.3f,-63.7f,-5,5,cyan,true);
    auto relay=room("THE BURIED RELAY","SERVICE POWER",box(-88,-7,-64,-42,12,-26),{-56,-5,-51},180,RoomTone::PUMPWORKS);
    relay.extraZones.push_back(box(-79,-6,-76,-69,8,-64));
    relay.hasTrigger=true;relay.trigger=box(-87,-6,-63,-57,4,-27);relay.exitDoor=relayExit;relay.entryGate=convoyExit;
    relay.space=ReverbSpace::HALL;relay.ambient=Ambient::DUST;
    relay.theme.skyAmb={.5f,.43f,.34f};relay.theme.groundAmb={.24f,.21f,.17f};relay.theme.fogDensity=.009f;
    relay.maxAlive=5;
    relay.waves={{W(EnemyType::RIPPER,0,{{-81,-5,-54},{-65,-5,-31}}),
                  W(EnemyType::MITE,0,{{-66,-5,-57},{-84,-5,-44}})}};
    relay.goals={WaveGoal::hold("SEAT THE RELAY CONTACT",{-74,-5,-52},4.5f,9.f)};
    L.arenas.push_back(relay);
    reel({-65,-5,-42});reel({-78,-5,-39});
    // A contact cabinet explains the reset pad: repairing an actual stuck
    // service contact, rather than holding an unexplained patch of floor.
    B.place("waste_relay_panel",{-74,-5,-48},180,.9f);
    solid(-75.5f,-5,-48.3f,-72.5f,-2,-47.7f,dark);
    sign("CONTACT RESET",{-75.7f,-.65f,-48.55f},{1,0,0},.045f);
    for(int k=0;k<8;++k) {
        float a=k*6.2831853f/8;
        vec3 p{-74+std::sin(a)*4.f,-4.96f,-52+std::cos(a)*4.f};
        B.kit().box(p,{.7f,.025f,.18f},paint,-a);
    }
    for(float x:{-82.f,-72.f,-60.f}) {
        B.place("waste_relay_panel",{x,-5,-26.5f},180);
        B.neon(x-1,-1,-27,x+1,-.9f,-26.8f,warm*.55f);
        B.kit().rod({x,-1,-27},{x,5,-27},.15f,rust,10);
    }
    terminal({-84,-5,-60},"RELAY / PURPOSE","EXIT CONTACT FAILED. MAINTENANCE RESET: HOLD THE LIT SERVICE PAD UNTIL THE CONTACT SEATS.");
    L.placedPickups.push_back({{-59,-5,-31},1});
    for(float x:{-80.f,-64.f,-48.f})B.neon(x-1,5.35f,-54,x+1,5.48f,-38,warm*.85f);
    // Secret 1 is an actual connected service room, not a wall that can be read through.
    floor(-100,-42,-88,-34,-5);
    solid(-100.4f,-5,-42,-100,3,-34,rock);solid(-100,-5,-42.4f,-88,3,-42,rock);
    solid(-100,-5,-34,-88,3,-33.6f,rock);solid(-100,3,-42,-88,3.4f,-34,dark);
    L.arenas.back().extraZones.push_back(box(-100,-6,-42,-88,3,-34));
    S.wrecks.push_back({{-97,-5,-38},1.57f,10,-1,storytext::spanLogs[1],2,true});
    lamp({-95,-5,-41});

    // 4. BROKEN SPAN: a six-metre break, high grapple beams and a broad lower
    // maintenance bypass. The fallback is a route, not a skip or invisible wall.
    floor(-88,-126,-68,-76,0);floor(-42,-126,-28,-76,0);
    floor(-68,-94,-42,-88,-3);
    ramp(box(-68,-3,-94,-62,0,-88),0,0,-3);
    ramp(box(-48,-3,-94,-42,0,-88),0,-3,0);
    floor(-68,-110,-59,-98,0);floor(-53,-110,-42,-98,0);
    boundaryX(-88,-28,-126,-8);boundaryX(-88,-79,-76,-8);boundaryX(-69,-28,-76,-8);
    boundaryZ(-126,-76,-88,-8);boundaryZ(-126,-76,-28,-8,{{-110,-98,0,6}});
    auto span=room("THE BROKEN SPAN","MAIN DECK CLOSED / SERVICE ROUTE BELOW",box(-88,-10,-126,-28,40,-76),{-74,0,-80},-90);
    span.scripted=true;span.voidY=-5;span.hasRespawn=true;span.respawn={-74,0,-80};L.arenas.push_back(span);
    for(float x:{-69.f,-55.f,-41.f}) {
        solid(x-1,-10,-112,x+1,10,-110,rock);
        solid(x-1,-10,-98,x+1,10,-96,rock);
        solid(x-1,9,-112,x+1,10,-96,steel); // real grappleable cross-beams
        B.neon(x-.65f,8.8f,-108,x+.65f,9,-100,cyan*.5f);
    }
    for(float z:{-98.2f,-109.8f})for(float x:{-66.f,-49.f})detail(x,.05f,z,x+6,.12f,z+.12f,paint);
    lamp({-72,0,-91});lamp({-36,0,-91});
    sign("SERVICE BELOW",{-78,2.5f,-96},{1,0,0},.055f);
    for(float x:{-78.f,-73.5f})detail(x-.05f,0,-96,x+.05f,2.8f,-95.8f,steel);

    // 5. TURBINE GRAVEYARD: widely spaced cover and blades above the fighting lane.
    floor(-28,-126,42,-66,0);
    boundaryZ(-126,-66,42,0);boundaryZ(-126,-66,-28,0,{{-110,-98,0,6}});
    // The convoy retaining wall is also the north edge of this district.
    boundaryX(-28,42,-126,0,{{24,34,0,6}});
    int turbineEntry=B.doorway(false,-109.8f,-98.2f,-28.3f,-27.7f,0,6,cyan);
    int turbineExit=B.doorway(true,24.2f,33.8f,-126.3f,-125.7f,0,6,cyan,true);
    auto turbines=room("THE TURBINE GRAVEYARD","THREE SEIZED BRAKES",box(-28,-1,-126,42,42,-66),{-23,0,-104},0,RoomTone::SPIRE);
    turbines.entryGate=turbineEntry;turbines.exitDoor=turbineExit;turbines.hasTrigger=true;turbines.trigger=box(-14,-1,-125,41,8,-67);
    turbines.waves={{W(EnemyType::WEAVER,0,{{14,0,-115}}),
                     W(EnemyType::SHIELDBEARER,0,{{-12,0,-78}}),
                     W(EnemyType::RAPTOR,0,{{14,9,-105},{28,11,-77}})}};
    turbines.goals={WaveGoal::external("SHOOT THE THREE TURBINE BRAKES")};
    L.arenas.push_back(turbines);
    for(vec3 p:{vec3{-20,0,-116},vec3{33,0,-119},vec3{33,0,-82},vec3{-19,0,-68}}) {
        B.place("waste_turbine_frame",p,15);
        solid(p.x-1.75f,0,p.z-1.75f,p.x+1.75f,.6f,p.z+1.75f,rock);
        solid(p.x-.72f,.6f,p.z-.72f,p.x+.72f,11.5f,p.z+.72f,steel);
    }
    // Seized safety locks sit on the machines' accessible, road-facing side.
    for(vec3 p:{vec3{-20,0,-116},vec3{33,0,-119},vec3{33,0,-82}}) {
        vec3 at=p+vec3{p.x<0?2.4f:-2.4f,2.1f,0};
        int panel=solid(at.x-.65f,at.y-.55f,at.z-.65f,at.x+.65f,at.y+.55f,at.z+.65f,dark);
        L.walls[panel].hidden=true;
        S.turbineBrakes.push_back(L.addAnchor(panel,at,55.f,LevelData::ChainAnchor::BRAKE));
        B.neon(p.x-.8f,3.2f,p.z-.2f,p.x+.8f,3.35f,p.z+.2f,warm*.4f);
    }
    sign("BRAKE LOCKS",{-25,3.5f,-99},{0,0,-1},.07f);
    hauler(7,-96,90);reel({-17,0,-88});reel({24,0,-73});
    // A maintenance deck and broad ramp offer a second firing angle on the
    // northern brake. The ground route and weapon wreck remain unobstructed.
    solid(7,2.9f,-78,18,3.2f,-68,steel);
    ramp(box(8,0,-91,14,3.2f,-78),2,0,3.2f);
    for(float x:{7.f,17.7f}) {
        solid(x,0,-76,x+.25f,2.9f,-75.7f,steel);
        detail(x,3.2f,-77,x+.12f,4.1f,-68,rust);
    }
    solid(7,8.6f,-78,18,9,-77.5f,steel); // a grappleable service beam above the ramp
    B.neon(8,8.4f,-77.8f,17,8.55f,-77.6f,cyan*.4f);
    S.wrecks.push_back({{35,0,-69},3.14f,11,3,storytext::spanLogs[2],4});
    L.placedPickups.push_back({{-23,0,-120},1});

    // 6. LAST TOLL: sheltered quiet, a gatehouse and one last look back.
    floor(14,-170,48,-126,0);boundaryZ(-170,-126,14,0);boundaryZ(-170,-126,48,0,{{-154,-142,3,7}});
    boundaryX(14,48,-170,0,{{24,36,0,7}});
    auto toll=room("THE LAST TOLL","CREW CHECKPOINT",box(14,-1,-170,48,28,-126),{29,0,-131},-90,RoomTone::TOWER);
    toll.scripted=true;L.arenas.push_back(toll);
    B.mat=Mat::CONCRETE;
    solid(15,0,-164,20,6,-142,rock);solid(38,0,-166,47,6,-158,rock);
    solid(15,6,-164,44,6.5f,-141,dark);
    B.mat=Mat::METAL;
    for(float x:{22.f,36.f})B.place("waste_relay_panel",{x,0,-169.3f});
    terminal({17,0,-136},"TOLL / LAST ENTRY","LAST CONVOY: THREE VEHICLES. DRIVER CHECK-INS: ZERO. RECOVERY REQUEST FORWARDED TO CLEARANCE.");
    terminal({41,0,-154},"CLEARANCE / SHIFT LOG","BALLAST: REAR BRAKE INOPERATIVE. REPLACEMENT PART REQUEST RETURNED: MODEL NO LONGER SUPPORTED.");
    L.placedPickups.push_back({{21,0,-157},1});
    lamp({24,0,-143});lamp({34,0,-161});
    // Secret 2: a roof-side store reached by an ordinary service ramp.
    floor(42,-154,54,-146,3);
    ramp(box(42,0,-146,46,3,-134),2,3,0);
    L.arenas.back().extraZones.push_back(box(42,-1,-154,54,15,-134));
    solid(53.8f,3,-154,54.2f,7,-146,rock);solid(46,3,-154.3f,54,7,-154,rock);
    S.wrecks.push_back({{51,3,-151},3.14f,12,-1,storytext::spanLogs[3],5,true});

    // 7. THE CUT: Ballast's working lane, with windbreaks and overhead hooks.
    floor(-18,-250,70,-170,0);
    boundaryZ(-250,-170,-18,0);boundaryZ(-250,-170,70,0);
    boundaryX(-18,70,-250,0,{{20,36,0,8}});
    int basinEntry=B.doorway(true,24.2f,35.8f,-170.3f,-169.7f,0,7,cyan);
    int basinExit=B.doorway(true,20.2f,35.8f,-250.3f,-249.7f,0,8,warm,true);
    auto basin=room("THE CUT","CLEARANCE IN PROGRESS",box(-18,-1,-250,70,48,-170),{30,0,-176},-90,RoomTone::CORE);
    basin.entryGate=basinEntry;basin.exitDoor=basinExit;basin.hasTrigger=true;basin.trigger=box(-17,-1,-249,69,12,-186);
    basin.bossSpawn={28,0,-218};basin.waves={{W(EnemyType::BALLAST,1)}};basin.damageScale=1.f;
    basin.tone=RoomTone::SPAN;
    L.arenas.push_back(basin);
    // Scoured asphalt, parked grading blades and sheltered service bays establish
    // what this sibling has been doing. The centre is a real, unobstructed cut.
    for(int sx:{-1,1})for(int sz:{-1,1}) {
        vec3 p{26+sx*23.f,0,-216+sz*23.f};
        solid(p.x-1,0,p.z-1,p.x+1,7,p.z+1,steel);
        solid(p.x-.4f,7,p.z-3,p.x+.4f,7.7f,p.z+3,steel);
        B.neon(p.x-.3f,6.8f,p.z-2.5f,p.x+.3f,7,p.z+2.5f,cyan*.6f);
    }
    for(vec3 p:{vec3{5,0,-202},vec3{51,0,-233}}) {
        solid(p.x-5,0,p.z-.6f,p.x+5,3.1f,p.z+.6f,rock);
        for(int k=0;k<4;++k)detail(p.x-4+k*2,2.7f,p.z-.65f,p.x-3+k*2,2.95f,p.z+.65f,paint);
    }
    for(int k=0;k<13;++k) {
        float x=18+k*1.5f;
        detail(x,.026f,-245,x+.13f,.04f,-185,dark*.7f);
    }
    for(float z:{-185.f,-245.f})for(int k=0;k<8;++k)
        detail(19+k*2.3f,.045f,z,20+k*2.3f,.06f,z+.6f,paint);
    for(vec3 p:{vec3{-8,0,-208},vec3{60,0,-229}})reel(p);
    pylon({-11,0,-242},-10);pylon({63,0,-180},10);
    L.placedPickups.push_back({{-12,0,-177},1});L.placedPickups.push_back({{64,0,-243},1});
    sign("CLEARANCE / 07",{12,10,-249.4f},{1,0,0},.25f);

    for(float x:{12.f,39.f})detail(x-.15f,0,-249.8f,x+.15f,10.8f,-249.5f,steel);

    // 8. FAR ROAD: a reward vista and a chapter card, not a dead-end wall.
    floor(4,-294,52,-250,0);boundaryZ(-294,-250,4,0);boundaryZ(-294,-250,52,0);boundaryX(4,52,-294,0);
    auto far=room("THE FAR ROAD","NORTHBOUND SERVICE ROAD",box(4,-1,-294,52,45,-250),{28,0,-255},-90,RoomTone::SPAN);
    far.scripted=true;L.arenas.push_back(far);S.end=box(18,-1,-288,38,6,-278);
    terminal({42,0,-267},"TRANSIT / UNRESOLVED","CREW STATION NORTH: NO RESPONSE. SERVICE ROAD CONTINUES TO THE TOWER.");

    // Old blade passes continue through the approach and freight lay-by.
    // They let the player recognise the Cut before its operator is revealed.
    for(float z:{23.f,10.f,-17.f,-46.f,-135.f})for(int k=0;k<5;++k)
        detail(-2.f+k*.7f,.027f,z-3,-1.9f+k*.7f,.045f,z+3,dark*.7f);

    // Failed evacuees sheltered beside the freight, outside the combat lanes.
    for(int k=0;k<18;++k) {
        glm::vec3 p{k%2?-18.f:31.f,0,-24.f-(k/2)*3.8f};
        S.dumped.push_back({p,k*1.71f,k%3==0,.1f*(k%4)});
    }
    for(int k=0;k<6;++k)S.dumped.push_back({{15.5f,0,-142.f-k*3.f},k*.8f,k%2==0,.12f*k});

    // Detail clusters, eroded road edges and geology are deterministic. They
    // leave space around spawns, thresholds and the centre of each route.
    B.mat=Mat::ROCK;
    for(int section=0;section<(int)L.arenas.size();++section) {
        const auto& a=L.arenas[section];const auto& z=a.zone;
        if(section==2)continue;
        float y=section==3?-8.f:0.f;
        for(int k=0;k<12;++k) {
            bool west=k%2==0;
            float x=west?z.min.x-5.5f:z.max.x+5.5f;
            float pz=glm::mix(z.min.z+3,z.max.z-3,(k/2+.5f)/6.f);
            // Keep connected thresholds and optional roof route clear of banks.
            bool passage=(section==1&&west&&pz>-63&&pz<-39)||(section==4&&west&&pz>-117&&pz<-91)
                ||(section==5&&!west&&pz>-161&&pz<-127);
            if(!passage)B.kit().rock({x,y-1,pz},7.f+(k%3),5.f+(k%4)*1.7f,rock,2000+section*31+k,.65f,9);
        }
        for(int k=0;k<6;++k) {
            float x=k%2?z.min.x+5:z.max.x-5;
            float pz=glm::mix(z.min.z+6,z.max.z-6,(k+.5f)/6.f);
            if(section!=3)B.place("waste_road_debris",{x,0.f,pz},k*47,.65f+(k%2)*.2f);
        }
    }

    // Road remains describe a single infrastructure network across the open
    // districts. Flush paint and broken asphalt do not obstruct movement.
    B.mat=Mat::CONCRETE;
    for(int k=0;k<9;++k) {
        float z=-5-k*6.4f;
        detail(-5,.012f,z-4,5,.023f,z,road);
        detail(-.12f,.025f,z-3,.12f,.035f,z-1,paint);
        for(float x:{-4.7f,4.55f})detail(x,.025f,z-4,x+.16f,.035f,z,paint*.7f);
    }
    for(int k=0;k<9;++k) {
        float z=-174-k*12.f;
        detail(22,.012f,z-9,34,.023f,z,road);
        detail(27.9f,.025f,z-6,28.1f,.035f,z-2,paint);
    }
    // The turbine road turns east before reconnecting to the toll road.
    for(int k=0;k<8;++k) {
        float x=-24+k*7.f;
        detail(x,.012f,-107,x+5,.023f,-101,road);
        detail(x,.025f,-104.1f,x+3,.035f,-103.9f,paint);
    }
    detail(-87,-4.99f,-63,-43,-4.98f,-27,{.31f,.32f,.3f});
    for(float x=-84;x<-44;x+=4)detail(x,-4.975f,-62,x+.035f,-4.97f,-28,dark);
    for(float z=-60;z<-28;z+=4)detail(-86,-4.975f,z,-44,-4.97f,z+.035f,dark);
    B.mat=Mat::METAL;
    // Believable relay machinery: transfer frames, overhead feed trays, bundled
    // pipes, numbered maintenance bays and shallow floor service channels.
    for(float x:{-83.f,-69.f,-55.f}) {
        detail(x-.15f,-5,-28,x+.15f,6.8f,-27.65f,steel);
        detail(x-.15f,5.7f,-63,x+.15f,6.2f,-27.65f,steel);
        for(float z=-61;z<-28;z+=3)detail(x-.7f,5.8f,z,x+.7f,5.9f,z+.16f,rust);
        B.place("cradle_archive_bank",{x,-5,-28},180,.75f);
        for(float dx:{-.4f,0.f,.4f})B.kit().rod({x+dx,4.6f,-28},{x+dx,4.6f,-62},.09f,dark,8);
    }
    B.place("cradle_pump_skid",{-47,-5,-33},90,.85f);
    B.place("cradle_service_cart",{-84,-5,-47},10,.8f);
    for(float z:{-32.f,-46.f,-60.f}) {
        B.place("cradle_extractor",{-87.6f,3,z},90,1.1f);
        B.neon(-87.5f,1.4f,z-1,-87.3f,1.55f,z+1,warm*.4f);
    }
    for(float x:{-84.f,-76.f,-62.f})for(float z:{-29.f,-61.f})
        detail(x,-4.96f,z,x+4,-4.94f,z+.12f,paint);
    sign("RELAY / 02",{-85,1,-25.5f},{1,0,0},.13f);
    // Roadside shelter islands: a fallen sign, dislodged barriers, tow gear,
    // crates and cargo remain at the edges of the convoy, away from spawns.
    for(glm::vec3 p:{vec3{-35,0,-31},vec3{34,0,-60},vec3{18,0,-9}}) {
        B.place("cradle_salvage_bin",p,20,.8f);
        for(int k=0;k<3;++k)B.place("waste_road_debris",p+vec3{k*1.1f,0,2.f},k*63,.8f);
    }
    for(int k=0;k<8;++k) {
        float z=-8-k*7.f;
        for(float x:{-9.f,9.f}) {
            detail(x-.35f,.035f,z-1.6f,x+.35f,.05f,z+1.6f,dark*.7f);
            for(int j=0;j<5;++j)detail(x-.4f,.052f,z-1.5f+j*.65f,x+.4f,.055f,z-1.4f+j*.65f,ash);
        }
    }
    for(float z:{-16.f,-35.f,-59.f}) {
        detail(38.7f,0,z-1.5f,39.3f,.8f,z+1.5f,road);
        lamp({38,0,z+2},90);
    }
    // Heavy transmission cables sag between pylons, with dark loops rather
    // than bright effects which might compete with enemy tells.
    for(int span=0;span<4;++span)for(float dx:{-1.f,1.f}) {
        vec3 prev{78+dx,13,-25-span*53.f};
        for(int k=1;k<=12;++k) {
            float u=k/12.f;vec3 q{78+dx,13-3*std::sin(u*3.14159265f),-25-span*53.f-u*53};
            B.kit().rod(prev,q,.055f,dark,6);prev=q;
        }
    }
    // Span deck joints, edge supports and a visible handrail for the safe route.
    for(float z:{-94.f,-88.f})for(float x=-66;x<-44;x+=4) {
        detail(x,-3,z-.08f,x+.1f,-1.7f,z+.08f,rust);
        detail(x,-1.8f,z-.08f,x+3.7f,-1.7f,z+.08f,steel);
    }
    for(float x:{-66.f,-48.f})for(float z=-109;z<-99;z+=2)
        detail(x,.025f,z,x+5,.035f,z+.08f,dark);
    // Turbines belong to a former substation, with wall-mounted switchgear,
    // buried cable troughs and service plinths at each intact machine.
    for(float z:{-73.f,-91.f,-112.f}) {
        B.place("waste_relay_panel",{40,0,z},-90,.9f);
        detail(38,.02f,z-2,39,.06f,z+2,dark);
        B.kit().rod({40,2,z},{40,4,z},.09f,rust,8);
    }
    for(float x:{-20.f,33.f})for(float z:{-119.f,-82.f}) {
        for(int k=0;k<7;++k)detail(x-2.3f+k*.7f,.03f,z-2.4f,x-2.f+k*.7f,.04f,z-2.15f,paint);
    }
    B.place("cradle_service_cart",{37,0,-95},15);
    B.place("waste_road_debris",{-23,0,-77},53,1.5f);
    // Tollhouse canopy ribs, inspection windows, queues and an abandoned
    // operator's station make the breather feel used rather than empty.
    for(float z=-162;z<-140;z+=3)detail(20,5.5f,z,43,5.8f,z+.2f,steel);
    for(float z:{-146.f,-153.f,-160.f}) {
        detail(20.01f,1.2f,z,20.07f,3.2f,z+1.6f,dark);
        B.neon(20.08f,1.3f,z+.1f,20.09f,1.4f,z+1.5f,warm*.35f);
        detail(22,.028f,z,36,.04f,z+.12f,paint);
    }
    B.place("waste_relay_panel",{19.8f,0,-158},90,.7f);
    B.place("cradle_service_cart",{39,0,-149},-15,.8f);
    sign("LAST TRANSIT",{21,5.1f,-140.7f},{1,0,0},.17f);
    // Maintenance equipment stays on the rim, clear of the grading and crosswind lanes.
    for(float z:{-188.f,-217.f,-243.f})for(float x:{-16.f,68.f}) {
        B.place("waste_relay_panel",{x,0,z},x<0?90:-90,1.1f);
        lamp({x+(x<0?1.f:-1.f),0,z+3},x<0?90:-90);
    }
    for(int k=0;k<24;++k) {
        float angle=k*6.2831853f/24;vec3 p{26+std::cos(angle)*14,.025f,-216+std::sin(angle)*14};
        B.kit().box(p,{1.7f,.02f,.16f},paint,-angle);
    }
    // More distant implied spaces: failed viaducts, empty freight shells and
    // mast farms, all below the tower's silhouette and outside traversal.
    for(int k=0;k<8;++k) {
        float z=-35-k*36.f;
        B.mat=Mat::CONCRETE;
        for(float x:{-108.f,-96.f})solid(x-1.3f,-12,z-1.3f,x+1.3f,10+(k%3)*2,z+1.3f,rock);
        detail(-110,10+(k%3)*2,z-2,-94,11+(k%3)*2,z+2,road);
        B.mat=Mat::METAL;
        if(k%2==0)B.place("waste_hauler",{-101,11+(k%3)*2,z},90,.9f);
    }

    // Beyond playable banks: layered silhouettes and fallen infrastructure.
    for(int k=0;k<18;++k) {
        float z=45-k*20.f;
        for(float x:{-118.f,92.f})B.kit().rock({x,-12,z},9+(k%4)*3.f,14+(k%5)*3.f,rock,2800+k+(int)x,.45f,9);
    }
    B.mat=Mat::METAL;
    for(int k=0;k<5;++k)B.place("waste_pylon",{78,0,-25-k*53.f},k%2?18.f:-9.f,1.15f);
    // The tower is kept in a consistent place throughout the chapter.
    B.mat=Mat::CONCRETE;
    solid(16,-12,-390,40,78,-370,{.16f,.13f,.15f});
    for(int k=0;k<10;++k) {
        float y=k*8.f;detail(13,y,-393,43,y+1,-368,steel);
        B.neon(26,y+1,-369,30,y+4,-368.8f,{1.5f,.4f,.18f});
    }
    B.neon(23,78,-386,33,89,-374,{2.f,.5f,.15f});
    B.mat=Mat::METAL;
    return L;
}
