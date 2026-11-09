// Actual damage, retry, projectile and death-card rendering paths.
#include "game/GameplayState.h"
#include <cstdio>
#include <memory>

static int failures=0;
#define CHECK(c,m) do{bool ok=(c);std::printf("%s %s\n",ok?"ok:":"FAIL:",m);failures+=!ok;}while(0)
static constexpr float DT=1.f/60;
static void capture(const char* file) {
    auto* s=SDL_CreateRGBSurfaceWithFormat(0,SCREEN_W,SCREEN_H,24,SDL_PIXELFORMAT_RGB24);
    if(!s){CHECK(false,"screenshot buffer allocation");return;}
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,SCREEN_W,SCREEN_H,GL_RGB,GL_UNSIGNED_BYTE,s->pixels);
    auto* p=(unsigned char*)s->pixels;std::vector<unsigned char> row(s->pitch);
    for(int y=0;y<SCREEN_H/2;++y){auto* a=p+y*s->pitch;auto* b=p+(SCREEN_H-1-y)*s->pitch;std::memcpy(row.data(),a,s->pitch);std::memcpy(a,b,s->pitch);std::memcpy(b,row.data(),s->pitch);}
    CHECK(SDL_SaveBMP(s,file)==0,"review capture saved");SDL_FreeSurface(s);
}
static void look(GameplayState& g,glm::vec3 eye,glm::vec3 target) {
    g.player.camera.position=eye;g.player.position=eye-glm::vec3{0,g.player.eyeHeight,0};g.prevCamPos=eye;
    auto d=glm::normalize(target-eye);g.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));g.player.camera.pitch=glm::degrees(std::asin(d.y));
}
int main() {
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!=0)return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window=SDL_CreateWindow("Iteration checks",0,0,SCREEN_W,SCREEN_H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window)return 1;auto context=SDL_GL_CreateContext(window);if(!context)return 1;
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental=GL_TRUE;glewInit();glGetError();
#endif
    glViewport(0,0,SCREEN_W,SCREEN_H);balanceInit();
    g_devNoMouse=g_practice=true;g_godMode=false;g_fixedDt=DT;g_storyChapter=3;g_startArena=3;
    {
        AudioSystem audio;
        for(const auto& m:mixTable())audio.loadSound(m.name,std::string("assets/sfx/")+m.name+".wav");
        GameSettings settings;
        auto live=std::make_unique<GameplayState>(audio,&settings,GameMode::STORY);auto& g=*live;
        g.closeArmory();g.paused=false;g.director.hold=true;g.enemies.clear();g.banners.clear();
        const auto c=lab::centre(3);
        look(g,c+glm::vec3{24,11,29},c+glm::vec3{-3,2,-8});
        g.render();capture("build/iteration/surgical-overview.bmp");
        look(g,c+glm::vec3{17,4.1f,6},c+glm::vec3{-19,2.7f,6});
        g.render();capture("build/iteration/surgical-circuit.bmp");
        CHECK(glGetError()==GL_NO_ERROR,"revised theatre and low circuit render without GL errors");
        look(g,c+glm::vec3{19,1.7f,22},c+glm::vec3{19,1.7f,0});g.player.velocity={0,0,0};
        g.invincFrames=0;g.styleSystem.health=100;
        DamageCause cleave{EnemyType::SOVEREIGN,DamageKind::MELEE,AttackKind::CLEAVE};
        CHECK(g.damagePlayer(20,g.player.position+glm::vec3{0,0,-3},.1f,.01f,.35f,cleave),"accepted damage records the contact through the live game");
        CHECK(!g.damagePlayer(100,g.player.position,0,0,0,{EnemyType::MITE,DamageKind::EXPLOSION}) && g.damageHistory.last.attack==AttackKind::CLEAVE,
              "an invulnerable contact cannot replace the recorded attack");
        g.invincFrames=0;g_godMode=true;
        CHECK(!g.damagePlayer(100,g.player.position,0,0,0,{}) && g.damageHistory.last.attack==AttackKind::CLEAVE,"god mode cannot manufacture damage advice");
        g_godMode=false;g.swordOut=true;g.sword= Sword{};
        g.sword.update(DT,{false,false,true,true});g.hurtBy=EnemyType::ABSOLUTE;
        CHECK(!g.damagePlayerStrike(40,g.player.position+glm::vec3{0,0,-3},Sword::Strike::WHITE) && g.damageHistory.last.attack==AttackKind::CLEAVE,
              "a successful live sword parry preserves the previous damage record");
        g.invincFrames=0;g.swordOut=false;g.styleSystem.health=20;
        g.damagePlayerStrike(30,g.player.position+glm::vec3{0,0,-3},Sword::Strike::GOLD);
        CHECK(g.damageHistory.last.enemy==EnemyType::ABSOLUTE && g.damageHistory.last.kind==DamageKind::GOLD_STRIKE,"a damaging gold strike records the unparryable counter");
        g.playerDead=true;g.render();capture("build/iteration/death-gold.bmp");
        g.restartHere();g.paused=false;g.director.hold=true;g.enemies.clear();
        CHECK(!g.playerDead && damageLabel(g.damageHistory.last).empty(),"real checkpoint retry clears the death advice");
        look(g,c+glm::vec3{-8,1.7f,24},c+glm::vec3{-8,1.7f,20});g.invincFrames=0;g.styleSystem.health=10;
        Enemy boss(EnemyType::SOVEREIGN,c+glm::vec3{-8,0,20});boss.state=EnemyState::ACTIVE;boss.spawnTimer=0;
        boss.attack=AttackKind::CLEAVE;boss.telegraphTimer=.001f;boss.yaw=0;g.enemies.push_back(boss);
        g.updateEnemies(DT);
        CHECK(g.styleSystem.health<=0 && g.damageHistory.last.attack==AttackKind::CLEAVE,
              "the real AI-to-damage path records a lethal cleave after the boss has cleared its wind-up");
        g.restartHere();g.paused=false;g.director.hold=true;g.enemies.clear();
        look(g,c+glm::vec3{19,1.7f,22},c+glm::vec3{19,1.7f,0});g.player.velocity={0,0,0};g.styleSystem.health=10;g.invincFrames=0;
        auto* shot=g.projSystem.fire(g.player.camera.position+glm::vec3{2,0,0},{-120,0,0},20,false);
        shot->owner=999;shot->ownerType=int(EnemyType::SENTINEL); // the shooter no longer exists
        Uint8 keys[SDL_NUM_SCANCODES]={};g.physicsTick(DT,keys,false);
        CHECK(g.styleSystem.health<=0 && g.damageHistory.last.enemy==EnemyType::SENTINEL && g.damageHistory.last.kind==DamageKind::PROJECTILE,
              "the real projectile path keeps the shooter's identity after its enemy has gone");
        g.update(DT);CHECK(g.playerDead,"lethal projectile damage reaches the actual death state");
        g.render();capture("build/iteration/death-projectile.bmp");
        g.restartHere();g.director.hold=true;g.enemies.clear();
        g.invincFrames=0;g.styleSystem.health=10;
        g.damagePlayer(20,g.player.position,0,0,0,cleave);g.playerDead=true;
        g.render();capture("build/iteration/death-cleave.bmp");
        g.restartHere();g.director.hold=true;g.enemies.clear();
        g.invincFrames=0;g.styleSystem.health=10;
        g.damagePlayer(20,g.player.position,0,0,0,{});g.playerDead=true;
        CHECK(damageLabel(g.damageHistory.last).empty(),"unknown live damage has no stale attack or counter");
        g.render();capture("build/iteration/death-unknown.bmp");
        CHECK(glGetError()==GL_NO_ERROR,"death cards with specific and unknown causes render cleanly");
    }
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    std::printf("%d iteration runtime failures\n",failures);return failures?1:0;
}
