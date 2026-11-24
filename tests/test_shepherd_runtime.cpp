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
        for(const auto& cue:mixTable())if(std::string(cue.name).find("shep_")==0||std::string(cue.name).find("ring_")==0)
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
        auto ready=[&](){g.playerDead=false;g.styleSystem=StyleSystem{};g.invincFrames=0;g.shepHeadArmed=false;g.shepHeadReset();g.shepRiseT=-1;g.shepHeadRise=1;g.shepHeadSeen=true;g.shepIdleT=99;g.sweep.reset();g.enemies.clear();g.wakeRings.clear();g.fx.clear();for(auto& t:g.fx.tracers)t.alive=false;for(auto& p:g.projSystem.pool)p.alive=false;g.ui.clearIndicators();g.ui.parryFlash=0;g.ui.damageVignette=0;g.ui.shootFlashTimer=0;g.explosionFlashTimer=0;g.whiteFade=0;};
        ready();aim(g,shep::fallStart(),headC(1));
        g.updateMusic();CHECK(g.audio.music.targetLevel()>1.25f,"the Shore approach ramp cannot overwrite the actual boss music intensity");
        {
            const auto savedStyle=g.styleSystem;const auto savedWeapons=g.weapons;
            g.styleSystem=StyleSystem{};g.styleSystem.readyOverdrive();
            g.shepRiseT=0;g.attackOverdrive();
            CHECK(g.styleSystem.overdriveReady&&!g.overdriveOn(),"the entrance preserves a charged Overdrive");
            g.shepRiseT=-1;g.shepRearT=0;g.attackOverdrive();
            CHECK(g.styleSystem.overdriveReady&&!g.overdriveOn(),"a movement break preserves a charged Overdrive");
            g.shepRearT=-1;g.shepHeadHp=0;g.shepLastT=0;g.attackOverdrive();
            CHECK(g.styleSystem.overdriveReady&&!g.overdriveOn(),"the final core count cannot waste a charged Overdrive");
            g.shepLastT=-1;g.shepHeadHp=HP;
            aim(g,shep::ledgeStart(0),headC(1));g.attackOverdrive();
            CHECK(g.styleSystem.overdriveReady&&!g.overdriveOn(),"an empty ledge cannot spend a charge on the distant body");
            aim(g,shep::fallStart(),headC(1));g.pendingFire=true;g.shootInput();
            CHECK(g.enemies.empty()&&g.overdriveOn()&&!g.styleSystem.overdriveReady,"the actual trigger unleashes Overdrive against the live custom body without regular enemies");
            g.styleSystem=savedStyle;g.weapons=savedWeapons;g.slowT=0;g.hitStopFrames=0;g.fovKick=0;
            g.ui.overdriveFlash=0;g.camFeel.reset();g.viewModel.reset(g.activeWeapon);
            ready();aim(g,shep::fallStart(),headC(1));
        }
        float range=300;auto dir=glm::normalize(headC(1)-g.player.camera.position);
        CHECK(g.shootHead(g.player.camera.position,dir,range,100)&&g.shepHeadHp<HP,"face takes damage during ordinary combat without a timed opening");
        ready();g.shepHeadHurt(1e5f);
        CHECK(g.shepHeadHp==HP*2.f/3.f&&g.shepHeadPhase==1&&g.shepRearT==0,"a huge hit stops at the next phase boundary");
        g.shepHeadHurt(500);CHECK(g.shepHeadHp==HP*2.f/3.f,"phase transition cannot be skipped by remaining shotgun pellets");
        g.shepRearT=-1;g.shepHeadHurt(1);g.shepHeadReset();
        CHECK(g.shepHeadHp==HP*2.f/3.f&&g.shepHeadPhase==1,"retry restarts the current third");
        ready();const glm::vec3 palm{-600,shep::FALL_Y,-3980};g.shepHandAt[0]=palm;g.shepArmDown[0]=HAND_DOWN;
        aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});g.holdfast.reset();
        g.styleSystem.style=StyleSystem::PASSIVE_CAP;
        g.pressHoldfast();
        CHECK(g.styleSystem.style>StyleSystem::PASSIVE_CAP,"a deliberate palm counter can earn Overdrive beyond the passive environment cap");
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
        // Deterministic audio clock: game events feed the real mixer once per
        // simulation tick. Do not race the SDL callback's voice introspection.
        Mix_HookMusic(nullptr,nullptr);
        g.enterArena(10);g.armoryOpen=false;
        std::vector<float> audioBlock(4096*2);
        auto pump=[&](int frames){while(frames>0){int n=std::min(frames,4096);std::fill(audioBlock.begin(),audioBlock.end(),0.f);g.audio.sfx.render(audioBlock.data(),n);frames-=n;}};
        auto prepareAudio=[&](){ready();pump(44100*3);aim(g,shep::fallStart(),headC(1));g.audio.setListener(g.player.camera.position,g.player.camera.right(),g.player.camera.forward());};
        prepareAudio();g.shepHandAt[0]=palm;g.shepArmDown[0]=HAND_DOWN;
        aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});
        for(int k=0;k<8;++k){float d=50;g.shootHead(g.player.camera.position,g.player.camera.forward(),d,10);}
        g.audio.sfx.render(audioBlock.data(),0);
        CHECK(g.audio.sfx.active("shep_wound")==1,"damaging a custom boss body emits one audible impact per pellet burst");
        g.shepHead(.1f);float hitRange=50;g.shootHead(g.player.camera.position,g.player.camera.forward(),hitRange,10);g.audio.sfx.render(audioBlock.data(),0);
        CHECK(g.audio.sfx.active("shep_wound")==2,"the next rifle-rate hit has its own contact sound");
        prepareAudio();g.shepHandAt[0]=palm;g.shepArmDown[0]=HAND_DOWN;aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});hitRange=50;
        g.shootHead(g.player.camera.position,g.player.camera.forward(),hitRange,80);g.audio.sfx.render(audioBlock.data(),0);
        CHECK(g.audio.sfx.active("shep_wound_heavy")==1,"a heavy damaging hit has a separate structural impact");
        prepareAudio();g.updateShepherdMotionAudio(.016f);g.updateShepherdMotionAudio(.016f);g.audio.sfx.render(audioBlock.data(),0);
        CHECK(g.audio.sfx.active("shep_motion")==0&&g.audio.sfx.active("shep_lift")==0,"a stationary pose does not emit fake movement sounds");
        g.shepAtk=SLAM;g.shepSlamArm=0;g.shepSlamAt[0]=palm;
        bool audibleMotion=false;
        for(int f=0;f<80;++f){g.shepAtkT=f/60.f;g.animateShepherd(1.f/60);g.audio.sfx.render(audioBlock.data(),0);audibleMotion|=g.audio.sfx.active("shep_motion")>0||g.audio.sfx.active("shep_lift")>0;pump(735);}
        CHECK(!audibleMotion,"attack animation never adds generic idle movement over its signature cue");
        for(int hz:{30,60,120}){
            prepareAudio();g.shepAtk=NONE;int starts=0,audibleFrames=0;SoundHandle last=0;bool noOverlap=true;
            for(int f=0;f<hz*30;++f){
                g.animateShepherd(1.f/hz);g.audio.sfx.render(audioBlock.data(),0);
                SoundHandle active=0;for(auto h:g.shepMotionSound)if(g.audio.sfx.isPlaying(h))active=h;
                if(active&&active!=last)++starts;last=active;
                audibleFrames+=active!=0;
                noOverlap&=g.audio.sfx.active("shep_motion")+g.audio.sfx.active("shep_lift")<=1;
                pump(int(std::lround((f+1)*44100./hz)-std::lround(f*44100./hz)));
            }
            CHECK(noOverlap,"idle mechanisms never overlap");
            CHECK(starts>0&&starts<=5&&audibleFrames<hz*3,"thirty seconds of moving idle contains sparse clunks and more than 90 percent silence at each simulation rate");
        }
        prepareAudio();g.shepMotionSound[0]=g.shepherdSound("shep_motion",g.player.position);pump(128);
        auto motion=g.shepMotionSound[0];g.shepAtk=SLAM;g.updateShepherdMotionAudio(.016f);pump(2048);
        CHECK(motion&&!g.audio.sfx.isPlaying(motion),"starting an attack fades an already-playing idle clunk before the attack develops");
        prepareAudio();g.shepMotionSound[0]=g.shepherdSound("shep_motion",g.player.position);pump(128);
        motion=g.shepMotionSound[0];g.stopShepherdAudio();pump(4410);
        CHECK(motion&&!g.audio.sfx.isPlaying(motion),"retry/death cancellation releases the tracked movement sound");
        prepareAudio();g.shepLastT=g.shepFinT=g.shepEndingT=-1;
        for(int phase=0;phase<3;++phase){g.shepHeadPhase=phase;g.updateMusic();g.audio.music.render(audioBlock.data(),0);for(int k=0;k<24;++k)g.audio.music.render(audioBlock.data(),4096);
            CHECK(g.audio.music.currentTrack()==SHEPHERD_MUSIC_TRACK&&g.audio.music.currentShepherdMovement()==shepherdscore::CONTROL+phase,"the live fight selects and plays each movement's distinct arrangement");}
        g.enterArena(0);g.armoryOpen=false;g.updateMusic();for(int k=0;k<24;++k)g.audio.music.render(audioBlock.data(),4096);
        CHECK(g.audio.music.currentTrack()==SHEPHERD_MUSIC_TRACK&&g.audio.music.currentShepherdMovement()==shepherdscore::ASCENT,"boarding the giant changes from the crossing score immediately");
        g.enterArena(10);g.armoryOpen=false;
        const char* watched[]={"shep_fist_tell","shep_sweep_tell","shep_grab_tell","shep_gaze_tell","shep_lock","shep_parry","shep_contact","shep_sweep","shep_gaze_fire","shep_memory","shep_moon","shep_gather"};
        for(int hz:{30,60,120})for(int attack:{SLAM,SLAM2,SWEEP,GRAB,GAZE,ECHO,ORRERY,PULL})for(int variation=0;variation<(attack==SWEEP||attack==GAZE||attack==ORRERY?2:1);++variation){
            prepareAudio();
            const int phase=attack==ECHO?1:attack==PULL?2:0;
            g.shepHeadPhase=phase;g.shepCycle=0;
            while(nextAttack(phase,g.shepCycle)!=attack)++g.shepCycle;
            if(variation)g.shepCycle+=phase==0?7:phase==1?8:9;
            g.shepIdleT=0;
            std::unordered_map<std::string,std::vector<float>> onsets;
            for(int f=0;f<hz*6;++f){
                std::array<int,std::size(watched)> before{};
                for(size_t k=0;k<std::size(watched);++k)before[k]=g.audio.sfx.active(watched[k]);
                g.shepHead(1.f/hz);g.audio.sfx.render(audioBlock.data(),0);
                for(size_t k=0;k<std::size(watched);++k)if(g.audio.sfx.active(watched[k])>before[k])onsets[watched[k]].push_back(g.shepAtkT);
                pump(int(std::lround((f+1)*44100./hz)-std::lround(f*44100./hz)));
                if(g.shepAtk==NONE)break;
            }
            auto timed=[&](const char* name,std::initializer_list<float> expected){
                const auto& got=onsets[name];bool ok=got.size()==expected.size();size_t k=0;
                for(float t:expected){if(k<got.size())ok&=got[k]>=t-.001f&&got[k]<=t+1.f/hz+.001f;++k;}
                if(!ok){std::printf("audio @%d Hz, attack %d, %s:",hz,attack,name);for(float t:got)std::printf(" %.3f",t);std::puts("");}
                return ok;
            };
            bool synced=true;
            if(attack==SLAM||attack==SLAM2){
                synced&=timed("shep_fist_tell",{0});
                synced&=attack==SLAM?timed("shep_parry",{SLAM_TELL-.27f}):timed("shep_parry",{SLAM_TELL-.27f,SLAM_TELL+.42f-.27f});
                synced&=attack==SLAM?timed("shep_contact",{SLAM_TELL}):timed("shep_contact",{SLAM_TELL,SLAM_TELL+.42f});
            }
            if(attack==SWEEP)synced&=variation?
                timed("shep_sweep_tell",{0,SWEEP_TELL+SWEEP_TRAVEL})&&timed("shep_sweep",{SWEEP_TELL,SWEEP_TELL+SWEEP_TRAVEL+.35f}):
                timed("shep_sweep_tell",{0})&&timed("shep_sweep",{SWEEP_TELL});
            if(attack==GRAB)synced&=timed("shep_grab_tell",{0})&&timed("shep_lock",{GRAB_LOCK})&&timed("shep_parry",{GRAB_CLOSE-.22f});
            if(attack==GAZE){synced&=timed("shep_gaze_tell",{0})&&timed("shep_lock",{GAZE_TRACK})&&timed("shep_sweep",{});
                synced&=variation?timed("shep_gaze_fire",{GAZE_FIRE,GAZE_FIRE+.6f,GAZE_FIRE+1.2f}):timed("shep_gaze_fire",{GAZE_FIRE});}
            if(attack==ECHO)synced&=timed("shep_memory",{.2f,.85f,1.5f})&&timed("shep_contact",{2.f,2.65f,3.3f});
            if(attack==ORRERY)synced&=variation?timed("shep_moon",{.8f,1.12f,1.44f,1.76f,2.08f,2.4f,2.72f}):timed("shep_moon",{.8f,1.12f,1.44f,1.76f,2.08f});
            if(attack==PULL)synced&=timed("shep_gather",{0})&&timed("shep_contact",{PULL_LEN});
            CHECK(synced,"attack warnings, locks, parry windows and releases sound once at their simulation markers at 30/60/120 Hz");
        }
        prepareAudio();g.shepHandAt[0]=palm;g.shepArmDown[0]=HAND_DOWN;
        aim(g,palm+glm::vec3{0,0,20},palm+glm::vec3{0,2,0});g.holdfast.reset();g.pressHoldfast();pump(128);
        auto strainHandle=g.shepStrainSound[0];g.shepHeadHurt(10000);pump(1024);
        CHECK(!g.audio.sfx.isPlaying(strainHandle),"a phase break actually releases the tension voice in the mixer");
        prepareAudio();g.shepWindSound=g.shepherdSound("shep_gather",g.player.position);pump(128);
        auto windHandle=g.shepWindSound;g.playerDead=true;g.shepHead(.016f);pump(1024);
        CHECK(!g.audio.sfx.isPlaying(windHandle),"death actually releases the warning voice instead of just clearing its handle");
        prepareAudio();g.shepWindSound=g.shepherdSound("shep_gather",g.player.position);pump(128);
        windHandle=g.shepWindSound;g.shepHeadReset();pump(1024);
        CHECK(!g.audio.sfx.isPlaying(windHandle),"retry actually releases the previous attempt's warning voice");
        prepareAudio();auto knockWalls=g.level.knockWalls;auto creakWalls=g.level.creakWalls;
        g.level.knockWalls={0};g.level.creakWalls.clear();g.player.onGround=true;g.player.groundWall=0;g.ringKnockOn=-1;
        g.ringTouch(1.5f);g.audio.sfx.render(audioBlock.data(),0);
        CHECK(g.audio.sfx.active("ring_knock")==1,"capsule contact emits one authored double knock without a queued bullet-impact echo");
        pump(44100);g.level.knockWalls.clear();g.level.creakWalls={0};g.ringCreakT=0;
        const auto wallBox=g.level.walls[0].box;const auto feet=g.player.position;
        g.level.walls[0].box={feet+glm::vec3{.3f,0,-1},feet+glm::vec3{1.3f,3,1}};g.player.velocity=glm::vec3{0};
        g.ringTouch(.016f);g.audio.sfx.render(audioBlock.data(),0);
        CHECK(g.audio.sfx.active("ring_creak")==1,"leaning on the reef emits its own quiet contact creak");
        g.level.walls[0].box=wallBox;g.level.knockWalls=knockWalls;g.level.creakWalls=creakWalls;
        prepareAudio();g.storyEndT=-1;g.sweepArmed=true;g.sweep.start(sweeptier::far({0,0,1}).spec);
        g.tickSweep(.016f);g.audio.sfx.render(audioBlock.data(),0);g.tickSweep(.016f);g.audio.sfx.render(audioBlock.data(),0);
        CHECK(g.audio.sfx.active("ring_sweep_warn")==1,"a sweep flash emits its protected warning exactly once");
        g.sweep.reset();
        prepareAudio();g.shepFinT=shepfall::FIN_DRAIN-.01f;g.shepEndingT=-1;
        g.styleSystem.overdrive=true;g.styleSystem.overdriveTimer=2.f;g.updateMusic();
        for(int k=0;k<4;++k){std::fill(audioBlock.begin(),audioBlock.end(),0.f);g.audio.drive.render(audioBlock.data(),4096,44100,float(g.audio.music.rootNote()),g.audio.music.samplesPerStep(),g.audio.music.stepPos(),1.f);}
        CHECK(g.audio.drive.level()>.9f,"the ending regression starts with an audible Overdrive bed");
        g.audio.music.render(audioBlock.data(),4096);
        CHECK(std::any_of(audioBlock.begin(),audioBlock.end(),[](float x){return std::fabs(x)>.001f;}),"the score stays present until the actual internal explosion");
        g.shepFall(.02f);g.audio.music.render(audioBlock.data(),4096);
        CHECK(std::all_of(audioBlock.begin(),audioBlock.end(),[](float x){return x==0.f;}),"the internal explosion releases the score immediately instead of waiting for the capsule checkpoint");
        g.updateMusic();g.audio.music.render(audioBlock.data(),4096);
        CHECK(std::all_of(audioBlock.begin(),audioBlock.end(),[](float x){return x==0.f;}),"the next frame keeps the detonation's score release in place");
        std::fill(audioBlock.begin(),audioBlock.end(),0.f);
        for(int k=0;k<20;++k)g.audio.drive.render(audioBlock.data(),4096,44100,float(g.audio.music.rootNote()),g.audio.music.samplesPerStep(),g.audio.music.stepPos(),1.f);
        CHECK(g.audio.drive.level()<.002f,"an active Overdrive cannot restart its power bed under the quiet ending");
        g.shepFinT=-1;g.shepEndingT=-1;g.updateMusic();g.audio.music.render(audioBlock.data(),4096);
        CHECK(std::any_of(audioBlock.begin(),audioBlock.end(),[](float x){return std::fabs(x)>.001f;}),"retry restores the combat score after the silent ending");
        balancepanel::extraBuild={};app.currentState.reset();
    }
    std::remove(StorySave::key);ImGui_ImplOpenGL3_Shutdown();ImGui_ImplSDL2_Shutdown();ImGui::DestroyContext();
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    std::printf("%d failures\n",failures);return failures?1:0;
}
