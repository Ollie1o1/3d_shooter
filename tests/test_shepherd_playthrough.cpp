// A deterministic input pilot through the real update loop. No god mode,
// damage injection, mid-fight teleport, ammo refill or altered attack schedule.
// This complements mechanic tests; it cannot judge how human controls feel.
#include <SDL2/SDL.h>
#define main overdrive_app_main
#include "../src/main.cpp"
#undef main
int main(){
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!=0)return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window=SDL_CreateWindow("Shepherd input pilot",0,0,SCREEN_W,SCREEN_H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window)return 1;auto context=SDL_GL_CreateContext(window);if(!context)return 1;
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental=GL_TRUE;glewInit();glGetError();
#endif
    glViewport(0,0,SCREEN_W,SCREEN_H);balanceInit();balancepanel::init(window,context);
    g_devNoMouse=true;g_fixedDt=1.0/60;g_devCam=false;g_devOverlay.clear();g_devAim=true;
    StorySave::key="build/shepherd-pass/pilot-story.cfg";
    bool finished=false;int shots=0,pins=0;float lowest=100,seconds=0;
    {
        App app;app.window=window;app.ctx=context;
        app.pending=App::NextState::Menu;app.applyPendingState();
        auto& menu=*static_cast<MenuState*>(app.currentState.get());
        StartOptions options;options.storyChapter=6;options.arena=10;options.practice=true;options.god=false;
        g_shepherd=true;menu.onStart(GameMode::STORY,options);app.applyPendingState();
        auto& g=*static_cast<GameplayState*>(app.currentState.get());g.armoryOpen=false;
        g.activeWeapon=int(WeaponId::KAR);
        // Base rifle, ordinary magazine and reload. Practice starts may inherit
        // a player's upgrades; this fixture establishes a repeatable loadout.
        for(int w=0;w<WEAPON_COUNT;++w){g.prog.up[w]=WeaponUpgrades{};g.weapons[w].ammo=weaponMag(WeaponId(w),g.prog.up[w]);}
        using namespace shephead;
        for(int f=0;f<60*180 && !g.playerDead && g.storyEndT<0;++f){
            seconds=f/60.f;std::memset(g_devKeys,0,sizeof(g_devKeys));
            glm::vec3 target=g.shepHeadHp>0?g.shepChestPose()*glm::vec4(chest(1),1):glm::vec4(shep::fallCore(),1);
            glm::vec3 goal{-600.f+18.f*std::sin(seconds*.7f),shep::FALL_Y,-3975.f+14.f*std::cos(seconds*.7f)};
            const float at=g.shepAtkT;
            bool dodge=false,parry=false;
            if(g.shepAtk==SLAM||g.shepAtk==SLAM2){
                const auto mark=g.shepSlamAt[0];
                if(g.shepAtk==SLAM && at<SLAM_TELL && slamHits(g.player.position,mark)){
                    goal=g.player.position;target=g.shepHandPose(g.shepSlamArm);
                    parry=at>SLAM_TELL-.20f&&at<SLAM_TELL-.12f;
                }else{
                    if(at>SLAM_LOCK){
                        const float edge=g.shepAtk==SLAM2?(mark.x>-600?std::max(g.shepSlamAt[0].x,g.shepSlamAt[1].x):std::min(g.shepSlamAt[0].x,g.shepSlamAt[1].x)):mark.x;
                        goal={edge+(mark.x>-600?16.f:-16.f),shep::FALL_Y,mark.z};
                    }
                    dodge=at>SLAM_LOCK+.05f&&at<SLAM_LOCK+.10f;
                }
            }
            if(g.shepAtk==SWEEP){
                const float north=std::min(g.shepSweepZ,g.shepVariation?g.shepAttackAt.z:g.shepSweepZ)-14.f;
                const float south=std::max(g.shepSweepZ,g.shepVariation?g.shepAttackAt.z:g.shepSweepZ)+14.f;
                const float safe=std::abs(g.shepSweepZ-north)<std::abs(g.shepSweepZ-south)?north:south;
                goal={g.player.position.x,shep::FALL_Y,safe};
                dodge=at>.55f&&at<.60f;
            }
            if(g.shepAtk==GRAB && at>GRAB_LOCK){goal={g.shepAttackAt.x+(g.shepAttackAt.x>-600?-20.f:20.f),shep::FALL_Y,g.shepAttackAt.z};dodge=at>.72f&&at<.77f;}
            if(g.shepAtk==GAZE && at>GAZE_TRACK){auto mark=g.shepGazeMark(gazeIndex(at,g.shepVariation));goal={mark.x+(mark.x>-600?-22.f:22.f),shep::FALL_Y,mark.z+6.f};dodge=at>1.35f&&at<1.40f;}
            if(g.shepLastT>=0)goal={-600.f,shep::FALL_Y,-4030.f};
            if(g.shepFinT>=0 || g.shepEndingT>=0)goal=g.player.position;
            // A grounded palm is an optional counter, reached through movement
            // and the actual HOLDFAST raycast rather than direct pin calls.
            int hand=-1;float near=1e9f;
            for(int k=0;k<2;++k)if(g.shepArmDown[k]>1.f && g.shepPinT[k]<=0){
                float d=glm::length(g.shepHandAt[k]-g.player.camera.position);
                if(d<near){near=d;hand=k;}
            }
            if(hand>=0 && near<42.f && g.holdfast.cooldown<=0 && !g.holdfast.held()){
                target=g.shepHandAt[hand]+glm::vec3{0,2,0};g.pendingHoldfast=true;++pins;
            }
            if(g.shepGrabT>=0)target=g.shepHandPose(g.shepSlamArm);
            auto d=glm::normalize(target-g.player.camera.position);
            g.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));g.player.camera.pitch=glm::degrees(std::asin(d.y));
            auto travel=goal-g.player.position;travel.y=0;
            const float fw=glm::dot(travel,g.player.camera.flatForward()),side=glm::dot(travel,g.player.camera.right());
            if(fw>1)g_devKeys[SDL_SCANCODE_W]=1;if(fw<-1)g_devKeys[SDL_SCANCODE_S]=1;
            if(side>1)g_devKeys[SDL_SCANCODE_D]=1;if(side<-1)g_devKeys[SDL_SCANCODE_A]=1;
            if(dodge)g_devKeys[SDL_SCANCODE_LSHIFT]=1;
            if(parry)g_devKeys[SDL_SCANCODE_F]=1;
            if(g.shepLastT<0 && g.player.onGround){
                for(const auto& ring:g.wakeRings){
                    const float gap=glm::length(glm::vec2(g.player.position.x-ring.at.x,g.player.position.z-ring.at.z))-ring.radius;
                    if(!ring.hit && gap>0 && gap<wakebell::SPEED*.35f)g_devKeys[SDL_SCANCODE_SPACE]=1;
                }
            }
            if(g.shepLastT>=0 && glm::length(shep::fallCore()-g.player.camera.position)<38.f)g.pendingHoldfast=true;
            if(g.shepHeadHp>0 && g.weapons[g.activeWeapon].ready()){g.pendingFire=true;++shots;}
            const float before=g.styleSystem.health;
            g.update(1.f/60);
            if(g.styleSystem.health<before-3)std::printf("  damage %.0f at %.2fs attack %d t %.2f position %.1f %.1f %.1f\n",before-g.styleSystem.health,seconds,g.shepAtk,g.shepAtkT,g.player.position.x,g.player.position.y,g.player.position.z);
            lowest=std::min(lowest,g.styleSystem.health);
            if(f%300==0)std::printf("pilot %.0fs: HP %.0f, boss %.0f, phase %d, attack %d, position %.1f %.1f %.1f\n",seconds,g.styleSystem.health,g.shepHeadHp,g.shepHeadPhase,g.shepAtk,g.player.position.x,g.player.position.y,g.player.position.z);
        }
        finished=g.storyEndT>=0&&!g.playerDead&&!g_godMode;
        std::printf("%s: input pilot %.1fs, %d trigger pulls, %d pin attempts, lowest HP %.1f, ending %d, god %d\n",finished?"ok":"FAIL",seconds,shots,pins,lowest,g.storyEndT>=0,g_godMode);
        balancepanel::extraBuild={};app.currentState.reset();
    }
    ImGui_ImplOpenGL3_Shutdown();ImGui_ImplSDL2_Shutdown();ImGui::DestroyContext();SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return finished?0:1;
}
