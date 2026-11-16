// Actual Shepherd combat, Blender model integration and rendered attack cues.
#include <SDL2/SDL.h>
#define main overdrive_app_main
#include "../src/main.cpp"
#undef main
static int failures=0;
#define CHECK(c,m) do{bool ok=(c);std::printf("%s %s\n",ok?"ok:":"FAIL:",m);failures+=!ok;}while(0)
static void aim(GameplayState& g,glm::vec3 feet,glm::vec3 target){
    g.player.position=feet;g.player.velocity={0,0,0};g.player.camera.position=feet+glm::vec3{0,g.player.eyeHeight,0};g.prevCamPos=g.player.camera.position;
    auto d=glm::normalize(target-g.player.camera.position);g.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));g.player.camera.pitch=glm::degrees(std::asin(d.y));
}
static void capture(const char* file){
    auto* s=SDL_CreateRGBSurfaceWithFormat(0,SCREEN_W,SCREEN_H,24,SDL_PIXELFORMAT_RGB24);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,SCREEN_W,SCREEN_H,GL_RGB,GL_UNSIGNED_BYTE,s->pixels);
    std::vector<unsigned char> row(s->pitch);auto* p=(unsigned char*)s->pixels;
    for(int y=0;y<SCREEN_H/2;++y){auto* a=p+y*s->pitch;auto* b=p+(SCREEN_H-1-y)*s->pitch;std::memcpy(row.data(),a,s->pitch);std::memcpy(a,b,s->pitch);std::memcpy(b,row.data(),s->pitch);}
    SDL_SaveBMP(s,file);SDL_FreeSurface(s);
}
int main(){
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!=0)return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window=SDL_CreateWindow("Shepherd combat checks",0,0,SCREEN_W,SCREEN_H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window)return 1;auto context=SDL_GL_CreateContext(window);if(!context)return 1;
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental=GL_TRUE;glewInit();glGetError();
#endif
    glViewport(0,0,SCREEN_W,SCREEN_H);balanceInit();balancepanel::init(window,context);
    g_devNoMouse=true;g_fixedDt=1.0/60;g_devCam=false;g_devOverlay.clear();
    StorySave::key="build/shepherd-pass/story.cfg";
    {
        App app;app.window=window;app.ctx=context;
        for(const auto& cue:mixTable())if(std::string(cue.name).find("shep_")==0)
            app.audio.loadSound(cue.name,std::string("assets/sfx/")+cue.name+".wav");
        app.audio.start();
        app.pending=App::NextState::Menu;app.applyPendingState();
        auto& menu=*static_cast<MenuState*>(app.currentState.get());
        StartOptions options;options.storyChapter=6;options.arena=10;options.practice=true;options.god=true;
        g_shepherd=true;
        menu.onStart(GameMode::STORY,options);app.applyPendingState();
        auto& g=*static_cast<GameplayState*>(app.currentState.get());g.armoryOpen=false;
        CHECK(g.shepherd()&&g.director.arena==10,"practice start loads the colossus arena");
        using namespace shephead;
        for(const char* name:shepherdmodel::PIECES)CHECK(propfile::get(name),"the live boss loads its Blender-authored anatomy");
        CHECK(g.swordAllowed()&&g.splinterOwned(),"Chapter 6 carries the sword and Splinter into the finale");
        auto ready=[&](){g.playerDead=false;g.styleSystem.health=100;g.invincFrames=0;g.shepHeadArmed=false;g.shepHeadReset();g.shepRiseT=-1;g.shepHeadRise=1;g.shepHeadSeen=true;g.shepIdleT=99;g.sweep.reset();g.enemies.clear();g.wakeRings.clear();g.fx.clear();for(auto& t:g.fx.tracers)t.alive=false;for(auto& p:g.projSystem.pool)p.alive=false;g.ui.clearIndicators();g.ui.parryFlash=0;g.ui.damageVignette=0;g.ui.shootFlashTimer=0;g.explosionFlashTimer=0;g.whiteFade=0;};
        ready();aim(g,shep::fallStart(),headC(1));
        g.updateMusic();CHECK(g.audio.music.targetLevel()>1.25f,"the Shore approach ramp cannot overwrite the actual boss music intensity");
        float range=300;auto dir=glm::normalize(headC(1)-g.player.camera.position);
        CHECK(g.shootHead(g.player.camera.position,dir,range,100)&&g.shepHeadHp<HP,"face takes damage during ordinary combat without a timed opening");
        ready();g.shepHeadHurt(1e5f);
        CHECK(g.shepHeadHp==HP*2.f/3.f&&g.shepHeadPhase==1&&g.shepRearT==0,"a huge hit stops at the next phase boundary");
        g.shepHeadHurt(500);CHECK(g.shepHeadHp==HP*2.f/3.f,"phase transition cannot be skipped by remaining shotgun pellets");
        g.shepRearT=-1;g.shepHeadHurt(1);g.shepHeadReset();
        CHECK(g.shepHeadHp==HP*2.f/3.f&&g.shepHeadPhase==1,"retry restarts the current third");
        ready();const glm::vec3 palm{-600,shep::FALL_Y,-3980};g.shepHandAt[0]=palm;g.shepArmDown[0]=HAND_DOWN;
        aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});g.holdfast.reset();
        g.pressHoldfast();
        CHECK(g.shepPinT[0]>0&&!g.shepCounterHit[0]&&g.shepHeadHp==HP,"HOLDFAST plants the palm before the captive mass reaches the heart");
        CHECK(!g.holdfast.held()&&g.holdfast.cooldown>0,"palm pin consumes one spike without tethering the player to a fake wall");
        CHECK(g.shepStrainSound[0]!=0,"pinning starts a tracked tension sound for the pending counterweight");
        g.shepHead(.45f);CHECK(g.shepHeadHp==HP&&g.shepOpenT==0,"the counterweight flight has no invisible early damage or opening");
        g.shepHead(.46f);CHECK(g.shepCounterHit[0]&&g.shepOpenT>0&&g.shepHeadHp==HP-PIN_DAMAGE,"the arriving counterweight damages the heart and opens the body on impact");
        g.shepHead(.2f);CHECK(g.shepHeadHp==HP-PIN_DAMAGE,"one palm pin applies its counterweight damage exactly once");
        CHECK(g.shepStrainSound[0]==0,"heart impact stops the pending tension sound");
        const float hp=g.shepHeadHp;range=50;g.shootHead(g.player.camera.position,g.player.camera.forward(),range,10);
        CHECK(g.shepHeadHp<hp-20,"a pinned hand receives the weak-point damage bonus");
        // A pending orbit must never leak through a phase boundary or a retry.
        ready();g.shepHandAt[0]=palm;g.shepArmDown[0]=HAND_DOWN;
        aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});g.holdfast.reset();g.pressHoldfast();
        g.shepHeadHurt(10000);g.shepHead(3.f);g.shepHead(.2f);
        CHECK(g.shepHeadHp==HP*2.f/3.f&&g.shepCounterHit[0],"entering a new movement cancels unfinished counterweights from the previous one");
        CHECK(g.shepStrainSound[0]==0&&g.shepWindSound==0,"a movement transition cancels obsolete wind-up and strain sounds");
        g.shepMovers(0);g.level.updateMovers(0);
        CHECK(g.level.movers[g.level.shepCourtClamps[0]].gone&&!g.level.movers[g.level.shepCourtClamps[1]].gone,"the first movement break opens only the west crossover");
        g.shepHeadReset();g.shepMovers(0);g.level.updateMovers(0);
        CHECK(g.shepHeadPhase==1&&g.shepCourtT==99.f&&g.level.movers[g.level.shepCourtClamps[0]].gone,"retry reconstructs the opened route without replaying its destruction");
        g.shepHeadHp=HP/3.f;g.shepHeadReset();g.shepMovers(0);g.level.updateMovers(0);
        CHECK(g.level.movers[g.level.shepCourtClamps[0]].gone&&g.level.movers[g.level.shepCourtClamps[1]].gone,"the last movement retains both opened gallery crossovers");
        ready();g.shepMovers(0);g.level.updateMovers(0);g.styleSystem.health=50;
        aim(g,shep::fallStart(),headC(1));range=400;g.shootHead(g.player.camera.position,g.player.camera.forward(),range,10);
        const float distantHeal=g.styleSystem.health-50,distantCooldown=g.shepHitHealT;
        ready();g.styleSystem.health=50;g.shepArmDown[0]=HAND_DOWN;g.shepHandAt[0]=palm;
        aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});range=50;g.shootHead(g.player.camera.position,g.player.camera.forward(),range,10);
        CHECK(g.styleSystem.health-50>distantHeal&&g.shepHitHealT<distantCooldown,"close palm punishment restores more health and faster than distant body fire");
        ready();g.shepArmDown[0]=HAND_DOWN;g.shepHandAt[0]=palm;aim(g,palm+glm::vec3{0,0,8},palm+glm::vec3{0,3,0});
        glm::vec3 splinterAt;CHECK(g.shepherdSplinterPoint(g.player.camera.position,splinterAt),"Splinter finds the grounded hand as a valid weak point");
        g.swordContact(0,30,false);CHECK(g.shepHeadHp<HP,"sword contact damages a grounded palm");
        ready();g.shepAtk=SLAM;g.shepAtkT=SLAM_TELL-.15f;g.shepSlamArm=0;g.shepSlamAt[0]=palm;
        aim(g,palm,palm+glm::vec3{0,25,0});g.animateShepherd(.016f);g.punch(-1);
        CHECK(g.shepSlamParried[0]&&g.shepHeadHp==HP-PARRY_DAMAGE,"punch input can turn a colossal fist in its white window");
        CHECK(g.shepArmDown[0]==0,"parrying does not teleport the descending fist onto the floor");
        g.shepHead(.2f);
        CHECK(g.shepArmDown[0]>0&&g.wakeRings.empty(),"a turned fist lands on its original timing without a damaging aftershock");
        ready();g.shepAtk=SLAM;g.shepAtkT=.3f;g.shepSlamAt[0]=palm;aim(g,palm,palm+glm::vec3{0,25,0});
        CHECK(!g.parryHead(),"an early punch cannot parry the fist");
        g.shepAtkT=SLAM_TELL-.01f;g.shepSlamArm=0;g.shepSlamAt[0]=palm;g_godMode=false;g.invincFrames=0;g.shepHead(.02f);
        CHECK(g.styleSystem.health<100&&g.shepArmDown[0]>0,"an unavoided slam damages the player and leaves its palm grounded");g_godMode=true;
        ready();aim(g,palm+glm::vec3{15,3,0},headC(1));g.wakeRings.push_back({palm,.6f,false});g.updateRing(.1f);
        CHECK(!g.wakeRings.empty()&&g.wakeRings[0].radius>.6f,"normal Chapter 6 update advances the boss aftershocks");
        ready();g.shepArmDown[0]=HAND_DOWN;g.shepHandAt[0]=palm;g.blastShepherd(palm+glm::vec3{0,2,0},5,50);
        CHECK(g.shepHeadHp<HP,"explosions damage grounded palms");
        ready();g.shepArmDown[0]=HAND_DOWN;g.shepHandAt[0]=palm;
        auto* rocket=g.projSystem.fire(palm+glm::vec3{0,2,16},{0,0,-100},80,true);
        g.shepherdShots(.1f);CHECK(rocket&&!rocket->alive&&g.shepHeadHp<HP,"fast player projectiles hit the hand before crossing it");
        ready();auto from=chest(1)+glm::vec3{0,0,35};auto* orb=g.projSystem.fire(from,{0,0,10},180,true);orb->owner=SHOT_OWNER;orb->parried=true;
        g.shepherdShots(.5f);CHECK(!orb->alive&&g.shepHeadHp<HP,"returned moons home into and damage the colossus");
        ready();g.shepHeadPhase=1;g.shepAtk=ECHO;g.shepAtkT=0;aim(g,palm,headC(1));
        for(int f=0;f<95;++f)g.shepHead(1.f/60);
        CHECK(g.shepEchoN==3&&g.shepEchoHit==0,"memory attack records three positions before the first impact");
        const auto recorded=g.shepEchoAt[0];g.player.position.x+=20;for(int f=0;f<45;++f)g.shepHead(1.f/60);
        CHECK(g.shepEchoAt[0]==recorded&&g.shepEchoHit>0,"memory positions stay locked while their individual warnings resolve");
        for(int phase=0;phase<3;++phase)for(int cycle=0;cycle<(phase==0?14:phase==1?16:18);++cycle){
            ready();g.shepHeadPhase=phase;g.shepCycle=cycle;g.shepIdleT=0;aim(g,shep::fallStart(),headC(1));g.shepHead(.016f);
            const int attack=g.shepAtk;bool done=false;
            for(int f=0;f<60*14;++f){g.shepHead(1.f/60);g.tickSweep(1.f/60);g.tickWake(1.f/60);if(g.shepAtk==NONE){done=true;break;}}
            CHECK(done&&std::isfinite(glm::length(g.player.velocity))&&std::isfinite(glm::length(g.shepHandPose(0)))&&std::isfinite(glm::length(g.shepHandPose(1))),"a scheduled boss attack resolves with finite player and rig motion");
            (void)attack;
        }
        // The actual mesh bends, and the head keeps its eyes on the player.
        ready();aim(g,shep::fallStart(),headC(1));for(int f=0;f<120;++f)g.animateShepherd(1.f/60);
        auto idleHead=glm::vec3(g.shepFacePose()*glm::vec4{0,0,0,1});
        std::vector<BoxInstance> idleA,idleB;g.gatherHead(idleA);
        for(int f=0;f<150;++f)g.animateShepherd(1.f/60);g.gatherHead(idleB);
        bool skinMoves=false;for(size_t i=0;i<std::min(idleA.size(),idleB.size());++i)if(idleA[i].deform.w>0){skinMoves|=glm::length(idleA[i].deform-idleB[i].deform)>.15f;}
        CHECK(skinMoves&&glm::length(glm::vec3(g.shepFacePose()*glm::vec4{0,0,0,1})-idleHead)>4.f,"idle bends the torso mesh and visibly shifts the head over the shoulders");
        auto forward=glm::normalize(glm::vec3(g.shepFacePose()*glm::vec4{0,0,1,0}));
        CHECK(glm::dot(forward,glm::normalize(g.player.camera.position-g.shepEyePose(0)))>.94f,"the eyes face the player instead of staring beyond the arena");
        auto setup=[&](int attack,glm::vec3 feet){ready();aim(g,feet,headC(1));g.shepAtk=attack;g.shepSlamArm=0;g.shepVariation=0;g.shepAttackAt=feet;g.shepAttackAt.y=shep::FALL_Y;g.shepSweepZ=feet.z;g.animateShepherd(.016f);g.shepGrabFrom=g.shepHandPose(0)+glm::vec3{0,12,-15};};
        auto step=[&](int n){for(int i=0;i<n;++i){g.shepHead(1.f/60);g.fx.tickMarks(1.f/60);if(g.shepAtk==NONE)g.shepIdleT=99;}};
        setup(SLAM2,palm);g_godMode=false;step(105);
        CHECK(g.styleSystem.health<100,"standing between the paired fists no longer exploits an unmarked safe seam");g_godMode=true;
        setup(SWEEP,palm);g.player.position.y+=3;g_godMode=false;step(150);
        CHECK(g.styleSystem.health<100,"jumping in place does not bypass the physical arm sweep");g_godMode=true;
        setup(SWEEP,palm);g.player.position.z+=14;g_godMode=false;step(150);
        CHECK(g.styleSystem.health==100,"moving out of the locked sweep lane avoids damage");g_godMode=true;
        setup(SWEEP,palm);g.shepVariation=1;g.shepAttackAt.z+=18;g.player.position.z+=18;g_godMode=false;step(150);
        CHECK(g.styleSystem.health==100,"the return sweep has not struck its second marked lane early");step(65);
        CHECK(g.styleSystem.health<100,"the return sweep attacks the second lane on its own timing");g_godMode=true;
        setup(GRAB,palm);step(50);auto lockedAttack=g.shepAttackAt;g.player.position.x+=18;step(65);
        CHECK(g.shepAttackAt==lockedAttack&&g.shepGrabT<0&&g.shepArmDown[0]>0,"a side dodge beats the locked grab and leaves a punishable palm");
        setup(GRAB,palm);step(113);
        CHECK(g.shepGrabT>=0&&g.shepGrabHp==GRAB_BREAK,"staying in the hand footprint starts a live, escapable grip");
        range=50;auto hand=g.shepHandPose(0);auto ray=glm::normalize(hand-g.player.camera.position);
        CHECK(g.shootHead(g.player.camera.position,ray,range,100)&&g.shepGrabT<0&&g.shepOpenT>0,"shooting the capturing hand breaks the grip and opens a counter");
        setup(GRAB,palm);step(113);g.player.camera.position=g.player.position+glm::vec3{0,g.player.eyeHeight,0};
        auto target=g.shepHandPose(0);aim(g,g.player.position,target);g.holdfast.reset();g.pressHoldfast();
        CHECK(g.shepGrabT<0&&g.shepOpenT>0,"the real HOLDFAST input breaks a capture without taking movement controls away");
        setup(GRAB,palm);step(101);aim(g,g.player.position,g.shepHandPose(0));g.punch(-1);step(12);
        CHECK(g.shepGrabT<0&&g.shepArmDown[0]>0&&g.shepHeadHp<HP,"the closing hand has a readable white parry and harmless landing");
        setup(GAZE,palm);step(80);lockedAttack=g.shepAttackAt;g.player.position.x+=14;step(45);
        CHECK(g.shepAttackAt==lockedAttack&&g.shepScars.size()==2,"the stare stops tracking before firing and leaves paired ground scars");
        step(250);CHECK(g.shepScars.empty(),"burned gaze routes expire rather than accumulating across the fight");
        setup(SLAM,palm);g.shepAtk=NONE;g.shepIdleT=0;g.shepCycle=1;g.shepArmDown[0]=g.shepArmDown[1]=3;g.shepHead(.016f);
        CHECK(g.shepAtk==GAZE,"pinning both arms prevents invisible hand attacks and forces the Shepherd to use its eyes");
        // A complete planted/recovery cycle may travel far, but must never
        // teleport at release or change merely because another render ran.
        ready();g.animateShepherd(1.f/60);g.shepHandAt[0]=palm;g.shepArmDown[0]=.2f;g.animateShepherd(1.f/60);
        const auto planted=g.shepHandPose(0);g.shepArmDown[0]=0;g.animateShepherd(1.f/60);
        CHECK(glm::length(g.shepHandPose(0)-planted)<.3f,"hand recovery begins at the planted palm without a pose snap");
        for(int f=0;f<120;++f){g.gameClock+=1.f/60;g.animateShepherd(1.f/60);}
        CHECK(glm::length(g.shepHandPose(0)-glm::vec3(g.shepBodyPose()*glm::vec4(handRest(0,1),1)))<20.f,"the lifted palm returns fluidly to its breathing pose");
        auto pose=g.shepHandPose(0);g.render();g.render();
        CHECK(g.shepHandPose(0)==pose,"rendering does not advance the simulation-owned rig");
        // Follow an actual strike through the impact and its full grounded pause.
        ready();aim(g,palm,headC(1));g.shepAtk=SLAM;g.shepSlamArm=0;g.shepSlamAt[0]=palm;g.animateShepherd(1.f/60);
        bool continuous=true,landed=false;float releaseStep=0;
        auto prior=g.shepHandPose(0);bool wasDown=false;
        for(int f=0;f<60*9;++f){g.gameClock+=1.f/60;g.shepHead(1.f/60);if(g.shepAtk==NONE)g.shepIdleT=99;auto next=g.shepHandPose(0);
            continuous&=std::isfinite(glm::length(next))&&glm::length(next-prior)<13.f;
            bool down=g.shepArmDown[0]>0;landed|=down&&glm::length(next-(g.shepHandAt[0]+glm::vec3{0,2,0}))<.001f;
            if(wasDown&&!down)releaseStep=glm::length(next-prior);
            wasDown=down;prior=next;
        }
        CHECK(continuous&&landed&&releaseStep<.3f,"live slam reaches its exact impact volume and releases without a jump");
        if(std::getenv("SHEPHERD_CAPTURE_MOTION")){
            int frame=0;
            for(int attack:{NONE,SLAM,SWEEP,GRAB,GAZE}){
                setup(attack,palm);g.shepVariation=1;g.shepAttackAt.z=palm.z+18;
                if(attack==GRAB||attack==GAZE)g.shepAttackAt=palm;
                int frames=attack==NONE?180:attack==SLAM?360:270;
                for(int f=0;f<frames;++f){
                    g.gameClock+=1.f/60;g.shepHead(1.f/60);g.tickWake(1.f/60);g.fx.tickMarks(1.f/60);g.ui.update(1.f/60,g.styleSystem);
                    if(g.shepAtk==NONE)g.shepIdleT=99;
                    if(g.shepGrabT>.7f)g.punch(-1);
                    const auto feet=g.player.position;
                    aim(g,{-600,shep::FALL_Y,-3910},{-600,214,-4085});g.player.position=feet;
                    if(f%4==0){char file[100];std::snprintf(file,sizeof(file),"build/shepherd-pass/pressure-%03d.bmp",frame++);g.render();capture(file);}
                }
            }
        }
        for(int attack:{SWEEP,GRAB,GAZE}){
            setup(attack,palm);g.shepVariation=1;g.shepAttackAt.z=palm.z+18;
            step(attack==SWEEP?105:attack==GRAB?90:105);
            const auto feet=g.player.position;aim(g,{-636,shep::FALL_Y,-3930},{-600,208,-4060});g.player.position=feet;
            g.render();capture(attack==SWEEP?"build/shepherd-pass/sweep.bmp":attack==GRAB?"build/shepherd-pass/grab.bmp":"build/shepherd-pass/gaze.bmp");
        }
        ready();aim(g,shep::fallStart(),headC(1));g.gameClock=8;for(int i=0;i<120;++i)g.animateShepherd(1.f/60);g.shepFallAmt=.25f;g.ui.damageVignette=0;
        g.render();capture("build/shepherd-pass/colossus.bmp");
        aim(g,{-600,shep::FALL_Y,-4000},headC(1));g.render();capture("build/shepherd-pass/face.bmp");
        aim(g,{-624,shep::FALL_Y,-3980},headC(1));g.shepAtk=SLAM;g.shepAtkT=1.45f;g.shepSlamAt[0]={-624,shep::FALL_Y,-3980};g.shepSlamArm=0;
        g.animateShepherd(.016f);g.render();capture("build/shepherd-pass/fist.bmp");
        ready();g.shepArmDown[0]=HAND_DOWN;g.shepHandAt[0]=palm;g.shepPinT[0]=PIN_TIME;g.shepOpenT=OPEN;
        aim(g,palm+glm::vec3{0,0,24},palm+glm::vec3{0,3,0});g.render();capture("build/shepherd-pass/pinned-palm.bmp");
        ready();g.gameClock=12;g.animateShepherd(.016f);
        aim(g,{-678,shep::FALL_Y+3.f,-3928},{-600,shep::FALL_Y,-3972});g.render();capture("build/shepherd-pass/court.bmp");
        aim(g,{-600,shep::FALL_Y,-3898},headC(1));g.render();capture("build/shepherd-pass/court-wide.bmp");
        // Capture the new cause/effect beats and checkpoint-restored court states.
        ready();g.shepMovers(0);g.level.updateMovers(0);g.shepHandAt[0]=palm;g.shepArmDown[0]=HAND_DOWN;
        aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});g.holdfast.reset();g.pressHoldfast();g.shepHead(.65f);
        aim(g,{-600,shep::FALL_Y,-3942},{-610,205,-4090});g.render();capture("build/shepherd-pass/counterweight-flight.bmp");
        for(int phase=1;phase<3;++phase){
            ready();g.shepHeadHp=HP*(3-phase)/3;g.shepHeadReset();g.shepRiseT=-1;g.shepHeadRise=1;g.shepHeadSeen=true;g.shepMovers(0);g.level.updateMovers(0);g.animateShepherd(.016f);
            aim(g,{-650,shep::FALL_Y+3.f,-3950},{-600,188,-4050});g.render();
            capture(phase==1?"build/shepherd-pass/court-movement-2.bmp":"build/shepherd-pass/court-movement-3.bmp");
        }
        g.shepVentsArmed=false;g.enterArena(3);g.armoryOpen=false;g.shepTurnT=-1;g.shepTurnSeen=true;g.enemies.clear();g.sweep.reset();
        CHECK(g.shepVents==0,"orrery starts with three captive moons");
        const auto old=g.shepMoon(0);g.shepRidge(.5f);CHECK(glm::length(g.shepMoon(0)-old)>1,"moon targets move through the arena");
        auto moon=g.shepMoon(0);aim(g,moon+glm::vec3{0,-moon.y,18},moon);g.holdfast.reset();g.pressHoldfast();
        CHECK(g.shepOrbitPin[0]>0,"real HOLDFAST ray pins a moving moon");
        const auto locked=g.shepMoon(0);g.shepRidge(.5f);CHECK(g.shepMoon(0)==locked,"pinned moon stops moving");
        for(int k=0;k<3;++k){moon=g.shepMoon(k);range=20;g.shootVent(moon+glm::vec3{0,0,10},{0,0,-1},range,1000);}
        CHECK(g.shepVents==3&&g.shepInT>=0,"destroying all moons opens the interior without vent windows");
        g.shepVentsArmed=false;g.enterArena(3);g.armoryOpen=false;g.shepTurnT=-1;g.enemies.clear();aim(g,{630,0,-3340},{630,8,-3400});g.render();capture("build/shepherd-pass/orrery.bmp");
        g.shepForksArmed=false;g.enterArena(4);g.armoryOpen=false;g.shepForkRingT=3;g.shepHallSweepT=1;g.shepStillFork(0);
        CHECK(g.shepForkRingT==shepfight::FORK_RING&&g.shepHallSweepT>=7,"breaking a fork transfers resonance and buys time to chain the next");
        g.enterArena(7);g.armoryOpen=false;g.director.phase=WaveDirector::Phase::ACTIVE;g.director.wave=0;g.director.goalDone=false;
        g.enemies.clear();g.spawnEnemy(EnemyType::HERDER,shep::ledge(0)+glm::vec3{13,0,13});g.enemies.back().state=EnemyState::ACTIVE;
        auto mooring=shep::ledgeMooring();aim(g,mooring+glm::vec3{0,-2,14},mooring);g.holdfast.reset();g.pressHoldfast();g.shepBreak(.016f);
        CHECK(g.shepMooringCalled&&!g.holdfast.held()&&g.shepLedgeTell>=0,"HOLDFAST calls the Shepherd's delayed strike onto the mooring and leaves the player free to dodge");
        g.render();capture("build/shepherd-pass/mooring.bmp");
        aim(g,shep::ledgeStart(0),mooring);for(int f=0;f<120;++f)g.shepBreak(1.f/60);
        CHECK(g.shepMooringBroken&&g.director.goalDone&&g.enemies[0].targetable(),"the boss's own strike breaks the mooring without a required guard cleanup");
        g.shepMovers(0);g.level.updateMovers(0);
        CHECK(g.level.movers[g.level.shepLedgeMooring].gone,"a broken mooring removes its collision as well as its intact appearance");
        g.enterArena(7);g.armoryOpen=false;g.director.phase=WaveDirector::Phase::ACTIVE;g.director.wave=0;g.director.goalDone=false;
        aim(g,shep::ledgeStart(0),mooring);g.shepLedgeTell=1.79f;g.shepLedgeAim=g.player.position;g.shepBreak(.02f);
        CHECK(!g.shepMooringBroken&&!g.director.goalDone,"a strike away from the mooring cannot complete its objective");
        g.shepLedgeTell=1.79f;g.shepLedgeAim=mooring;g.shepBreak(.02f);
        CHECK(g.shepMooringBroken&&g.director.goalDone,"baiting an ordinary strike onto the mooring is a valid alternative to HOLDFAST");
        g.enterArena(8);g.armoryOpen=false;g.director.phase=WaveDirector::Phase::ACTIVE;g.director.wave=0;g.director.goalDone=false;g.enemies.clear();
        auto& relayMover=g.level.movers[g.level.shepLedgeRelay];g.shepMovers(.5f);g.level.updateMovers(.5f);
        auto relay=(g.level.walls[relayMover.wall].box.min+g.level.walls[relayMover.wall].box.max)*.5f;
        aim(g,shep::ledge(1)+glm::vec3{9,0,8},relay);g.holdfast.reset();g.pressHoldfast();
        const float relayClock=g.shepRelayClock;g.shepMovers(.5f);g.level.updateMovers(1.f);g.shepBreak(1.2f);
        CHECK(g.holdfast.held()&&g.shepRelayClock==relayClock&&g.shepRelayCharge>1.f&&!g.director.goalDone,"holding the relay freezes its actual mover and builds progress while combat remains active");
        g.render();capture("build/shepherd-pass/relay.bmp");
        g.holdfast.cut();g.shepBreak(.1f);CHECK(g.shepRelayCharge==0,"releasing the relay resets an incomplete hold");
        g.holdfast.reset();g.pressHoldfast();g.shepBreak(2.51f);
        CHECK(g.shepRelayDone&&g.director.goalDone&&!g.holdfast.held(),"one uninterrupted relay hold completes the objective and releases the line for boarding");
        g.shepBreakReset();CHECK(!g.shepRelayDone&&g.shepRelayCharge==0,"retrying the relay reconstructs its unsolved objective without stale progress");
        g.enterArena(9);g.armoryOpen=false;g.director.phase=WaveDirector::Phase::ACTIVE;g.director.wave=0;g.director.goalDone=false;
        g.enemies.clear();g.spawnEnemy(EnemyType::STRAY,shep::ledge(2));g.enemies.back().state=EnemyState::ACTIVE;
        aim(g,shep::ledgeStart(2),shep::rideChunk(2,0));g.shepRides=2;g.shepLedgeFought=true;g.shepGatherT=-1;g.shepRideT=-1;g.shepCrownT=-1;g.shepThrownT=-1;g.sweep.reset();
        g.shepBreak(3.f);
        CHECK(g.sweep.active()&&g.enemies[0].targetable(),"the final Gather starts while the ledge guards are still alive");
        auto ride=shep::rideChunk(2,0);aim(g,ride+glm::vec3{0,0,10},ride-glm::vec3{0,1,0});g.holdfast.reset();g.pressHoldfast();
        for(int f=0;f<60*9&&g.shepRideT<0;++f){g.tickSweep(1.f/60);g.shepBreak(1.f/60);}
        CHECK(g.shepRideT>=0&&g.director.goalDone,"boarding the last Gather completes the objective without requiring guard kills");
        g.enterArena(10);g.armoryOpen=false;ready();g.shepHeadPhase=2;g.shepHeadHp=1;g.shepHeadHurt(5);
        CHECK(g.shepHeadHp==0&&g.shepLastT==0,"defeating the body starts the last count");
        g.shepFallReset();CHECK(g.shepLastT==0,"retry after defeating the body restarts the finisher without replaying the fight");
        aim(g,shep::fallCore()+glm::vec3{0,-8,14},shep::fallCore());g.holdfast.reset();g.pressHoldfast();g.shepFall(.016f);
        CHECK(g.shepFinT==0,"real HOLDFAST input into the heart starts the finisher");
        for(int f=0;f<600;++f)g.shepFall(1.f/60);
        CHECK(g.shepEndingT>=0&&g.director.arena==11,"finisher reaches the drifting capsule ending");
        CHECK(glGetError()==GL_NO_ERROR,"the colossus, fists, pinned palms and moving orrery render without GL errors");
        CHECK(persist::load(StorySave::key).empty(),"practice verification leaves Story progress untouched");
        balancepanel::extraBuild={};app.currentState.reset();
    }
    std::remove(StorySave::key);ImGui_ImplOpenGL3_Shutdown();ImGui_ImplSDL2_Shutdown();ImGui::DestroyContext();
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    std::printf("%d failures\n",failures);return failures?1:0;
}
