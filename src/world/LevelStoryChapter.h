#pragma once
#include "world/Level.h"
#include "core/StoryText.h"
#include "ui/PixelFont.h"
#include "world/LevelStoryAtmosphere.h"

// THE CRADLE's upper factory. Coordinates are metres, Y up. A continuous route:
// ducts → assembly catwalks → archive → vault → cargo lift → scanners → Sorter → ash.
// The lower three rooms stay in LevelStory.h; this keeps the chapter readable by room.
inline void buildCradleChapter(LevelBuilder& B, float A) {
    using glm::vec3;
    auto& L = B.L; auto& S = L.story;
    auto box = &LevelBuilder::aabb;
    const vec3 iron{.24f,.27f,.29f}, plate{.36f,.39f,.4f}, dark{.09f,.11f,.13f}, white{.72f,.72f,.65f},
               rust{.43f,.23f,.12f}, gold{.82f,.62f,.12f}, blue{.18f,1.1f,1.3f}, warm{1.5f,1.05f,.5f}, red{2.f,.16f,.07f};
    const float V = A - 4.f, T = V + 39.f;
    B.mat = Mat::METAL;
    auto solid = [&](float x0,float y0,float z0,float x1,float y1,float z1,vec3 c) {return B.wall(x0,y0,z0,x1,y1,z1,c);};
    auto detail = [&](float x0,float y0,float z0,float x1,float y1,float z1,vec3 c) {B.prop(x0,y0,z0,x1,y1,z1,c);};
    auto light = [&](float x0,float y0,float z0,float x1,float y1,float z1,vec3 c) {B.neon(x0,y0,z0,x1,y1,z1,c);};
    auto slab = [&](float x0,float z0,float x1,float z1,float y) { solid(x0,y-.5f,z0,x1,y,z1,plate); };
    // Room walls with door openings are authored explicitly; no invisible wall covers a doorway.
    auto railX = [&](float x0,float x1,float z,float y) {
        detail(x0,y+1.05f,z,x1,y+1.13f,z+.08f,gold);
        for(float x=x0;x<x1;x+=3.f) detail(x,y,z,x+.08f,y+1.1f,z+.08f,iron);
    };
    auto stairs = [&](float x0,float x1,float z0,int n,float rise,float run,float base) {
        for(int k=0;k<n;++k) {float z=z0+k*run,y=base+(k+1)*rise; solid(x0,base-.5f,z,x1,y,z+run,iron); light(x0,y,z,x1,y+.025f,z+.06f,gold*.8f);}
    };
    auto terminal = [&](vec3 p,const char* title,const char* line) {
        solid(p.x-.65f,p.y,p.z-.4f,p.x+.65f,p.y+1.2f,p.z+.4f,dark);
        detail(p.x-.8f,p.y+1.2f,p.z-.45f,p.x+.8f,p.y+2.25f,p.z+.1f,iron);
        light(p.x-.64f,p.y+1.4f,p.z+.11f,p.x+.64f,p.y+2.1f,p.z+.14f,blue*.65f);
        for(int k=0;k<4;++k) detail(p.x-.5f,p.y+1.5f+k*.13f,p.z+.15f,p.x+.25f+(k%2)*.25f,p.y+1.54f+k*.13f,p.z+.17f,white);
        S.terminals.push_back({p,title,line});
    };
    const Theme factory = L.arenas.back().theme;
    auto room = [&](const char* name,const char* sub,AABB zone,vec3 start,float yaw) {
        Arena r; r.name=name; r.subtitle=sub; r.zone=zone; r.bounds=zone;
        r.bounds.min.y=start.y; r.playerStart=start; r.startYaw=yaw;
        r.space=ReverbSpace::METAL; r.tone=RoomTone::PUMPWORKS; r.theme=factory;
        r.theme.skyAmb={.24f,.27f,.28f}; r.theme.groundAmb={.11f,.09f,.07f};
        r.maxAlive=99; r.damageScale=1.f; r.ambient=Ambient::STEAM;
        return r;
    };
    using W=WaveEntry;

    // The lower factory: service panels, cable trays, warm pools of light,
    // a cyan route to the chute and readable jump edges amongst the scrap.
    for(float z:{-5.f,-17.f,-41.f,-51.f}) {
        for(float x:{-14.8f,14.5f}) {
            detail(x,2,z,x+.25f,4.2f,z+2.5f,dark);
            for(int k=0;k<5;++k)detail(x-.02f,2.3f+k*.3f,z+.2f,x+.28f,2.4f+k*.3f,z+2.3f,plate);
            light(x-.02f,4.3f,z+.7f,x+.28f,4.4f,z+1.8f,warm*.5f);
        }
        detail(-11,8.7f,z,11,9,z+.3f,iron);
        for(float x:{-10.f,10.f})light(x-.5f,8.6f,z,x+.5f,8.7f,z+.3f,warm*.45f);
    }
    for(float z=-24;z>-29;z-=1.2f)light(11,.015f,z,11.3f,.04f,z+.5f,blue*.55f);
    light(10,.015f,-29,13.5f,.04f,-28.8f,blue*.7f);
    for(float z:{-56.f,-44.f,-20.f,-10.f}) {
        detail(57.7f,-39,z,57.95f,-29,z+.7f,rust);
        light(57.65f,-33,z+.15f,57.7f,-32,z+.55f,warm*.6f);
    }
    for(float z:{2.f,8.f,32.f,44.f}) {
        float x=z<12?30.05f:36.05f;
        detail(x,-26.9f,z,x+.12f,-25.2f,z+1,dark);
        light(x+.13f,-26.5f,z+.2f,x+.15f,-26.35f,z+.7f,blue*.35f);
    }
    for(auto& a:L.arenas){a.theme.skyAmb*=1.2f;a.theme.groundAmb*=1.15f;}

    auto sign=[&](const char* text,vec3 at,vec3 right,float pixel,vec3 color) {
        for(int i=0;text[i];++i){unsigned char ch=text[i];if(ch<32||ch>127)continue;
            for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(PIXEL_FONT[ch-32][y]&(1<<(4-x))) {
                vec3 p=at+right*((i*6+x)*pixel)-vec3{0,y*pixel,0};
                B.neon(p.x,p.y,p.z,p.x+pixel*.85f,p.y+pixel*.85f,p.z+pixel*.85f,color);
            }
        }
    };
    sign("REJECT TRANSFER",{14.15f,3.8f,-36.f},{0,0,1},.065f,warm*.65f);
    sign("SALVAGE",{14.15f,2.7f,-34.f},{0,0,1},.09f,blue*.7f);
    sign("ASSEMBLY ^",{46.3f,A-8.f,50.2f},{0,0,1},.065f,blue*.7f);
    sign("FREIGHT / 07",{-35.8f,V+4.f,14.8f},{1,0,0},.16f,warm*.65f);
    sign("DUCTS >",{57.65f,-29.f,-23.f},{0,0,1},.14f,warm*.6f);
    sign("ASSEMBLY / 04",{68,A+9,24.05f},{1,0,0},.24f,warm*.6f);
    sign("UNITS 1-23: DECOMMISSIONED",{24,A+5.2f,64.05f},{1,0,0},.15f,blue*.6f);
    sign("UNIT 24: REJECTED",{24,A+3.7f,64.05f},{1,0,0},.15f,red*.6f);
    sign("DO NOT WAKE",{-30,V+8.8f,26.05f},{1,0,0},.24f,blue*.6f);
    sign("JUDGE",{55.4f,T+7.2f,3},{0,0,1},.21f,warm*.7f);

    // 4. ASSEMBLY HALL: the catwalk frames three working lines seven metres below.
    // A down-and-back route puts the player amongst the machines, then out west.
    const float F=A-7.f;
    slab(51.5f,50,64,56,A);
    solid(51.5f,A,49.5f,56,A+3.f,50,iron); solid(51.5f,A,56,56,A+3.f,56.5f,iron);
    solid(51.5f,A+3.f,49.5f,56,A+3.4f,56.5f,dark);
    slab(56,24,108,78,F);
    solid(56,F,23.5f,108,F+22,24,iron); solid(108,F,24,108.5f,F+22,78,iron);
    solid(56,F,78,108,F+22,78.5f,iron);
    B.wallZ(24,78,55.5f,56,F,F+22,iron,{{50,56,A,A+3},{66,72,A,A+4}});
    solid(55.5f,F+22,23.5f,108.5f,F+22.5f,78.5f,dark);
    slab(56,48,64,78,A); slab(64,70,108,78,A);
    railX(64,98,70,A); railX(56,64,48,A);
    // Broad stairs: small risers require no jumping, visible from either floor.
    stairs(98,106,46,28,.25f,24.f/28.f,F);
    // Production lanes: rubber between silver rails, cyan control boxes and amber fixtures.
    for(float x:{70.f,81.f,92.f}) {
        int w=solid(x-1.5f,F,28,x+1.5f,F+.3f,64,dark);
        L.conveyors.push_back({w,{0,0,1},1.2f});
        detail(x-1.7f,F+.1f,28,x-1.55f,F+.7f,64,plate); detail(x+1.55f,F+.1f,28,x+1.7f,F+.7f,64,plate);
        for(float z:{32.f,44.f,56.f}) {
            S.assemblyUnits.push_back({x,F+.32f,z});
            for(float s:{-1.f,1.f}) {
                detail(x+s*3.f-.2f,F,z-.3f,x+s*3.f+.2f,F+4.8f,z+.3f,iron);
                detail(std::min(x,x+s*3),F+4.4f,z-.25f,std::max(x,x+s*3),F+4.8f,z+.25f,gold);
                L.pistons.push_back({{x+s*1.3f,F+4.4f,z},.18f,.5f,2.8f,4.f,z*.13f,blue});
            }
            light(x-.6f,F+5.5f,z-1,x+.6f,F+5.6f,z+1,warm);
        }
    }
    for(float z:{28.f,40.f,52.f,64.f,76.f}) {
        detail(56,F+16,z,108,F+17,z+.4f,plate);
        light(57,F+15.9f,z,62,F+16,z+.3f,warm); light(101,F+15.9f,z,107,F+16,z+.3f,warm);
        for(float x:{56.1f,107.f}) {detail(x,F,z,x+.8f,F+16,z+.6f,plate);light(x+.1f,F+2,z+.61f,x+.65f,F+7,z+.65f,blue*.3f);}
    }
    // Machine services and failed output accumulate beside production, away
    // from the floor's fighting lanes and the broad return stair.
    for(float x:{64.f,76.f,88.f,100.f}) {
        B.place("cradle_switchgear",{x,F,24.35f});
        B.place("cradle_scrap_cluster",{x,F+.02f,26.f},x*3,.65f);
    }
    for(float z:{36.f,60.f})B.place("cradle_switchgear",{107.65f,F,z},-90);
    terminal({60,A,76},"ASSEMBLY / 04","LINE 04: REJECT RETURN JAMMED. OUTPUT DIVERTED TO LINE 05.");
    S.wrecks.push_back({{104,F,27},3.14f,4,-1,storytext::cradleLogs[4],3});
    const int assemblyGate=B.doorway(false,50.2f,55.8f,55.6f,56.2f,A,3,blue);
    const int assemblyExit=B.doorway(false,66.2f,71.8f,55.5f,56.1f,A,4,blue,true);
    Arena hall=room("THE ASSEMBLY HALL","PRODUCTION FLOOR",box(56,F-2,24,108,F+21.9f,78),{58,A,53},0);
    hall.extraZones.push_back(box(51.5f,A-1,50,56,A+3,56));
    hall.bounds=box(57,F,25,107,F+15,77);
    hall.trigger=box(65,F-1,26,108,A+2,69); hall.hasTrigger=true;
    hall.entryGate=assemblyGate; hall.exitDoor=assemblyExit;
    hall.waves={
        {W(EnemyType::SENTINEL,0,{{102,A,73},{59,A,62}}),W(EnemyType::RIPPER,0,{{76,F,63},{99,F,37}}),
         W(EnemyType::CONDUCTOR,0,{{84,F+7,47}})},
        {W(EnemyType::BRUTE,0,{{86,F,65}}),W(EnemyType::MITE,0,{{66,F,60},{88,F,62},{98,F,45}})}};
    L.arenas.push_back(hall);
    L.placedPickups.push_back({{59,A,58},1});

    // 5. ARCHIVE: deliberately quiet. Each terminal is optional, one sentence.
    slab(16,64,56,84,A);
    solid(16,A,83.9f,56,A+7,84.4f,iron); solid(16,A,63.5f,56,A+7,64,iron);
    B.wallZ(64,84,15.5f,16,A,A+7,iron,{{72,78,A,A+4}});
    B.wallZ(64,84,55.5f,56,A,A+7,iron,{{66,72,A,A+4}});
    solid(15.5f,A+7,63.5f,56,A+7.5f,84.4f,dark);
    for(float x:{24.f,34.f,44.f}) {
        for(float z:{81.f}) {
            solid(x,A,z,x+3,A+5.5f,z+2,iron);
            for(int k=0;k<7;++k) {detail(x+.2f,A+.4f+k*.7f,z+2.01f,x+2.8f,A+.9f+k*.7f,z+2.08f,dark); light(x+.4f,A+.55f+k*.7f,z+2.09f,x+.5f,A+.65f+k*.7f,z+2.12f,blue);}
        }
        light(x,A+6.9f,71,x+2,A+7,75,blue*.7f);
    }
    // Under-rack trunking gives the Archive its own orderly visual rhythm.
    for(float x:{24.f,34.f,44.f}) {
        detail(x,A+.08f,80.6f,x+3,A+.18f,80.8f,dark);
        B.place("cradle_switchgear",{x+1.5f,A,83.6f},180,.7f);
    }
    terminal({49,A,77},"DECOMMISSION REGISTER","UNITS 1-23: DECOMMISSIONED. UNIT 24: REJECTED.");
    terminal({37,A,70},"PURPOSE REGISTER","ASSIGNMENT REQUIRES SUPERVISOR SIGN-OFF. LAST SIGN-OFF: UNIT 00.");
    terminal({25,A,77},"REJECTION RECORD","24: MOTOR TEST PASSED. REJECTION ENTERED BEFORE FINAL INSPECTION.");
    S.wrecks.push_back({{19,A,82},3.14f,5,-1,storytext::cradleLogs[5],4});
    L.arenas.push_back(room("THE ARCHIVE","ASSIGNMENT RECORDS",box(16,A-1,64,56,A+7,84),{53,A,69},180));

    // 6. POD VAULT: a high entry balcony overlooks six rows; glass breaks on the far half.
    slab(-38,26,16,84,V);
    B.wallZ(26,84,-38.5f,-38,V,V+12,iron,{{68,71,V+3.6f,V+5.f}}); solid(-38,V,84,16,V+12,84.5f,iron);
    B.wallX(-38,16,25.5f,26,V,V+12,iron,{{-30,-22,V,V+5}});
    B.wallZ(26,84,16,16.5f,V,V+12,iron,{{72,78,A,A+4}});
    solid(-38,V+12,26,16,V+12.5f,84,dark);
    slab(8,72,16,84,A); stairs(8,16,60,16,.25f,.75f,V);
    for(float x:{-30.f,-17.f,-4.f}) for(float z:{37.f,49.f,61.f,75.f}) {
        solid(x-1.4f,V,z-1.4f,x+1.4f,V+.5f,z+1.4f,dark);
        detail(x-1.5f,V+4.5f,z-1.5f,x+1.5f,V+5,z+1.5f,plate);
        for(float s:{-1.f,1.f}) {detail(x+s*1.35f-.06f,V+.5f,z-1.35f,x+s*1.35f+.06f,V+4.5f,z+1.35f,plate);}
        int g=solid(x-1.25f,V+.5f,z+.85f,x+1.25f,V+4.5f,z+1.f,{.15f,.36f,.37f});
        L.walls[g].hidden=true;
        S.podGlass.push_back(g); S.pods.push_back({x,V+.5f,z});
        light(x-.9f,V+.51f,z-1,x+.9f,V+.56f,z+1,blue*.6f);
        light(x-.7f,V+4.4f,z-.7f,x+.7f,V+4.5f,z+.7f,blue*.8f);
    }
    for(float z:{33.f,45.f,57.f,69.f,81.f}) light(-37,V+10,z,15,V+10.15f,z+.25f,blue*.35f);
    // Two little test bays: an empty watch post and a running track with a stopped clock.
    solid(-37,V,28,-33,V+2.6f,31,iron); light(-36.9f,V+2.7f,28,-33.1f,V+2.8f,31,warm*.6f);
    for(int k=0;k<4;++k) detail(-37,V+.01f,66+k*1.2f,-33,V+.04f,66.08f+k*1.2f,white);
    terminal({-35,V,79},"TRIAL / MEASURE","ENDURANCE TRIAL 06: STOP SIGNAL SENT. SUBJECT DID NOT RECEIVE IT.");
    terminal({-35,V,32},"TRIAL / KEEP","GUARD TRIAL 07: RELIEF REQUEST PENDING. OPERATOR DISCONNECTED.");
    S.wrecks.push_back({{11,A,82},3.14f,6,1,storytext::cradleLogs[6],5});
    // A broken service vent leads to a hidden observation cell.
    slab(-46,65,-38,73,V+3.6f);
    solid(-46.5f,V+3.6f,65,-46,V+6.8f,73,iron);
    solid(-46,V+3.6f,64.5f,-38,V+6.8f,65,iron);solid(-46,V+3.6f,73,-38,V+6.8f,73.5f,iron);
    solid(-46,V+6.8f,65,-38,V+7.2f,73,dark);
    solid(-35,V,65,-33,V+1.2f,67,iron);solid(-38,V+2.2f,65,-35,V+2.5f,67,plate);solid(-38,V+3.3f,68,-35,V+3.6f,71,plate);
    light(-45,V+6.6f,68,-44,V+6.7f,70,blue*.5f);
    S.wrecks.push_back({{-44,V+3.6f,69},1.57f,7,-1,storytext::cradleLogs[7],5,true});
    const int vaultGate=B.doorway(false,72.2f,77.8f,15.8f,16.4f,A,4,blue);
    const int vaultExit=B.doorway(true,-29.8f,-22.2f,25.6f,26.2f,V,5,blue,true);
    Arena vault=room("THE POD VAULT","CONTAINMENT STORAGE",box(-38,V-2,26,16,V+12,84),{12,A,75},180);
    vault.extraZones.push_back(box(-46,V+3.f,65,-38,V+6.8f,73));
    vault.entryGate=vaultGate; vault.exitDoor=vaultExit; vault.hasTrigger=true; vault.trigger=box(-38,V-1,27,7,V+7,59);
    vault.waves={{W(EnemyType::SHIELDBEARER,0,{{-17,V,35},{-30,V,59}}),
                  W(EnemyType::CONDUCTOR,0,{{-17,V+6,51}}),
                  W(EnemyType::RIPPER,0,{{-4,V,39},{-17,V,63}}),
                  W(EnemyType::SERAPH,0,{{-8,V+8,55}})}};
    L.arenas.push_back(vault); L.placedPickups.push_back({{6,V,62},1});

    for(vec3 p:{vec3{-36,V,42},vec3{-36,V,56}}) B.place("cradle_switchgear",p,90);

    // 7. ELEVATOR SHAFT: a driven freight platform, stopped at each of three landings.
    slab(-30,16,-22,26,V);
    solid(-30.5f,V,16,-30,V+5,26,iron); solid(-22,V,16,-21.5f,V+5,26,iron);
    slab(-36,-4,-32,16,V); slab(-18,-4,-14,16,V);
    slab(-32,-4,-18,-1,V); slab(-32,13,-18,16,V);
    solid(-32,V-1.2f,-1,-18,V-.65f,13,dark);
    solid(-36.5f,V,-4,-36,T+10,16,iron); solid(-36,V,-4.5f,-14,T+10,-4,iron);
    B.wallX(-36,-14,16,16.5f,V,T+10,iron,{{-30,-22,V,V+5}});
    B.wallZ(-4,16,-14,-13.5f,V,T+10,iron,{{4,12,T,T+6}});
    solid(-36,T+10,-4,-14,T+10.5f,16,dark);
    S.elevator=B.mover({-25,V-.3f,6},{7,.3f,7},Mover::Path::DRIVEN,{0,0,0},{0,39,0},1,0,blue);
    for(float x:{-34.f,-16.f}) {detail(x,V,-3,x+.3f,T+8,-2.7f,plate);light(x+.1f,V,-2.65f,x+.2f,T+7,-2.6f,blue*.4f);}
    for(int k=1;k<=3;++k) {
        float y=V+13*k;
        slab(-36,-4,-32,16,y); slab(-18,-4,-14,16,y);
        light(-35.9f,y+2,-1,-35.8f,y+2.2f,13,warm);
        detail(-36,y+6,-4,-14,y+6.4f,-3.5f,plate);
    }
    slab(-18,4,-14,12,T);
    Arena lift=room("THE ELEVATOR SHAFT","FREIGHT ACCESS",box(-36,V-2,-4,-14,T+10,16),{-25,V,21},-90);
    lift.extraZones.push_back(box(-30,V-1,16,-22,V+5,26));
    lift.scripted=true; lift.bounds=box(-35,V,-3,-15,T+8,15);
    lift.waves={
        {W(EnemyType::HUSK,0,{{-34,V+13,1},{-34,V+13,11}}),W(EnemyType::RAPTOR,0,{{-25,V+18,5}})},
        {W(EnemyType::SENTINEL,0,{{-34,V+26,9}}),W(EnemyType::RAPTOR,0,{{-22,V+31,1},{-28,V+32,12}})},
        {W(EnemyType::HUSK,0,{{-34,T,3},{-16,T,11}}),W(EnemyType::RAPTOR,0,{{-25,T+5,6}})}};
    lift.exitDoor=B.doorway(false,4.2f,11.8f,-14.2f,-13.6f,T,6,blue,true);
    L.arenas.push_back(lift);

    B.place("cradle_switchgear",{-35.6f,V,14},90);
    for(int k=1;k<=3;++k) {
        float y=V+13*k;
        sign(k==3?"03 / EXIT":k==2?"02":"01",{-35.8f,y+3.f,2},{0,0,1},.18f,warm*.6f);
        B.place("cradle_switchgear",{-35.6f,y,13},90,.7f);
    }

    // 8. INSPECTION LINE: alternating side bays and overhead beams, then a viewing slit.
    slab(-14,0,56,16,T);
    solid(-14,T,-.5f,56,T+8,0,iron); solid(-14,T,16,56,T+8,16.5f,iron);
    B.wallZ(0,16,-14,-13.5f,T,T+8,iron,{{4,12,T,T+6}});
    B.wallZ(0,16,55.5f,56,T,T+8,iron,{{4,12,T,T+6}});
    solid(-14,T+8,0,56,T+8.5f,16,dark);
    for(int k=0;k<6;++k) {
        float x=-4.f+k*9.f;
        detail(x-.25f,T,0,x+.25f,T+7,1,plate); detail(x-.25f,T,15,x+.25f,T+7,16,plate);
        detail(x-.25f,T+6.6f,0,x+.25f,T+7,16,plate);
        light(x-.15f,T+6.5f,1,x+.15f,T+6.6f,15,red*.8f);
        S.scanners.push_back({{x,T+.2f,1},{x,T+.2f,15},k*.8f});
        if(k<5) solid(x+2,T,k%2?11.f:1.f,x+4,T+2.5f,k%2?15.f:5.f,iron);
    }
    terminal({-8,T,14},"INSPECTION PROTOCOL","SCANNER CONTACT DISPATCHES A RECOVERY DETAIL. SERVICE STAFF USE THE LOWER CHANNEL.");
    terminal({49,T,14},"SORTING / LIVE","SORTER: TRANSFER ACTUATOR FAULT. MANUAL CLEARANCE OVERDUE.");
    S.grappleAt={45,T,2};
    solid(44,T,1,46,T+1.1f,3,dark); light(44.2f,T+1.1f,1.2f,45.8f,T+1.2f,2.8f,blue);
    S.wrecks.push_back({{48,T,1.2f},0,8,-1,storytext::cradleLogs[8],7});
    const int inspectExit=B.doorway(false,4.2f,11.8f,55.6f,56.2f,T,6,gold);
    Arena inspect=room("THE INSPECTION LINE","RECOVERY SCANNERS",box(-14,T-1,0,56,T+8,16),{-11,T,8},0);
    inspect.scripted=true;
    // This encounter only wakes if a scanner catches you. A clean run keeps the room quiet.
    inspect.waves={{W(EnemyType::RIPPER,0,{{6,T,8},{24,T,8}}),W(EnemyType::SENTINEL,0,{{46,T,8}})}};
    L.arenas.push_back(inspect); L.placedPickups.push_back({{50,T,13},1});

    // 9. SORTING FLOOR: segmented belts over the furnace, safe side rails and gantry hooks.
    solid(56,T-9,-50,108,T-8,18,rust);
    light(57,T-7.95f,-49,107,T-7.8f,17,{2.5f,.65f,.08f});
    solid(108,T-8,-50,108.5f,T+24,18,iron); solid(56,T-8,18,108,T+24,18.5f,iron);
    B.wallZ(-50,18,55.5f,56,T-8,T+24,iron,{{4,12,T,T+6}});
    B.wallX(56,108,-50.5f,-50,T-8,T+24,iron,{{76,88,T,T+8}});
    solid(56,T+24,-50,108,T+24.5f,18,dark);
    slab(56,-50,64,18,T); slab(100,-50,108,18,T); slab(64,10,100,18,T); slab(64,-50,100,-42,T);
    // Nine tiles, the four corners drop only after a two-second warning in the last phase.
    for(int ix=0;ix<3;++ix) for(int iz=0;iz<3;++iz) {
        float x=64+ix*12.f,z=-42+iz*(52.f/3);
        int w=solid(x,T-.6f,z,x+12,T,z+52.f/3,dark);
        L.conveyors.push_back({w,{0,0,1},1.8f});
        if(ix!=1 && iz!=1) {L.walls[w].hidden=true; S.sortingTiles.push_back(w);}
        for(float dx:{.15f,11.7f}) light(x+dx,T+.015f,z,x+dx+.12f,T+.035f,z+52.f/3,gold*.6f);
    }
    for(float x:{62.f,102.f}) {
        solid(x-1,T+15,-48,x+1,T+16,16,iron);
        light(x-.2f,T+14.8f,-47,x+.2f,T+15,15,blue);
        for(float z:{-35.f,-15.f,5.f}) {
            solid(x-1,T,z-1,x+1,T+16,z+1,plate);
            light(x-1.05f,T+3,z-.6f,x-1,T+7,z+.6f,blue*.5f);
        }
    }
    for(float z:{-44.f,-24.f,-4.f,14.f}) {detail(56,T+21,z,108,T+22,z+.6f,plate);light(75,T+20.8f,z,89,T+21,z+.4f,warm);}
    const int sortingExit=B.doorway(true,76.2f,87.8f,-50.6f,-49.9f,T,8,red,true);
    Arena sorting=room("THE SORTING FLOOR","TRANSFER FAULT",box(56,T-12,-50,108,T+23.9f,18),{59,T,8},-45);
    sorting.bounds=box(57,T,-49,107,T+20,17); sorting.entryGate=inspectExit; sorting.exitDoor=sortingExit;
    sorting.hasTrigger=true; sorting.trigger=box(65,T-1,-41,100,T+10,9);
    sorting.bossSpawn={82,T+2,-20}; sorting.waves={{W(EnemyType::SORTER,1)}};
    sorting.voidY=T-6;
    sorting.theme.skyAmb={.28f,.24f,.21f}; sorting.theme.groundAmb={.19f,.1f,.05f};
    L.arenas.push_back(sorting);
    L.placedPickups.push_back({{59,T,-9},1}); L.placedPickups.push_back({{105,T,-32},1});
    // An airlock opens onto ash under a red sky. The creator's tower is a real distant silhouette.
    slab(72,-67,92,-50,T); solid(71.5f,T,-62,72,T+9,-50,iron); solid(92,T,-62,92.5f,T+9,-50,iron);
    B.mat=Mat::ROCK;
    slab(56,-100,108,-62,T-.15f);
    for(int k=0;k<13;++k) {float x=58+(k*17)%45,z=-70-(k*11)%27;detail(x,T-.15f,z,x+1.8f,T+.15f+(k%3)*.2f,z+2.4f,{.3f,.24f,.22f});}
    B.mat=Mat::METAL;
    solid(76,T-2,-135,88,T+40,-125,{.13f,.1f,.12f});
    for(int k=0;k<7;++k) {float y=T+k*6;detail(74,y,-136,90,y+1,-124,iron);light(81,y,-124,83,y+3,-123.9f,red);}
    light(79,T+40,-134,85,T+47,-126,{2.6f,.4f,.15f});
    sorting.extraZones.clear(); // only the exit vista belongs to this section, no extra encounter.
    L.arenas.back().extraZones.push_back(box(72,T-1,-67,92,T+12,-50));
    L.arenas.back().extraZones.push_back(box(56,T-2,-100,108,T+60,-62));
    S.end=box(72,T-1,-84,92,T+5,-74); S.nextName="THE CRADLE";
    buildCradleAtmosphere(B,A);
}
