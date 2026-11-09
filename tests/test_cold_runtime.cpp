// Actual Chapter 5 gameplay, mutable boss collision and rendered attack cues.
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
    auto* window=SDL_CreateWindow("Cold combat checks",0,0,SCREEN_W,SCREEN_H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window)return 1;auto context=SDL_GL_CreateContext(window);if(!context)return 1;
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental=GL_TRUE;glewInit();glGetError();
#endif
    glViewport(0,0,SCREEN_W,SCREEN_H);balanceInit();balancepanel::init(window,context);
    g_devNoMouse=true;g_fixedDt=1.0/60;g_devCam=false;g_devOverlay.clear();
    StorySave::key="build/cold-pass/story.cfg";
    {
        App app;app.window=window;app.ctx=context;
        app.pending=App::NextState::Menu;app.applyPendingState();
        auto& menu=*static_cast<MenuState*>(app.currentState.get());
        StartOptions options;options.storyChapter=5;options.arena=5;options.practice=true;options.god=true;
        menu.onStart(GameMode::STORY,options);app.applyPendingState();
        auto& g=*static_cast<GameplayState*>(app.currentState.get());g.armoryOpen=false;
        CHECK(g.cold()&&g.director.arena==5,"the real Chapter 5 practice start opens the Chillers");
        const auto deck=cold::chiller(1).c;const auto safe=deck+glm::vec3{-3,0,4.5f};
        aim(g,safe,deck);g.player.onGround=true;g.updateCold(1.f/60);
        CHECK(glm::length(g.level.arenas[5].respawn-safe)<.01f,"safe ground on the second deck becomes the fall recovery point");
        aim(g,{150,-300,-830},deck);g.player.onGround=false;g.keepPlayerInZone(1.f/60);
        CHECK(glm::length(g.player.position-safe)<.01f&&glm::length(g.player.velocity)==0,"a real gap fall returns to the visited deck with cleared momentum");
        g.enemies.clear();g.spawnEnemy(EnemyType::THAWLING,cold::chiller(2).c);
        g.enemies[0].spawnTimer=0;g.enemies[0].state=EnemyState::ACTIVE;
        aim(g,cold::chiller(0).c,cold::chiller(1).c);bool reached=false,held=true;
        for(int f=0;f<60*70;++f){g.updateEnemies(1.f/60);const auto p=g.enemies[0].position;held&=p.y>-.3f;
            if(glm::length(glm::vec2{p.x-g.player.position.x,p.z-g.player.position.z})<4){reached=true;break;}}
        CHECK(reached&&held,"actual gameplay routes a Thawling across both catwalks without falling");
        g.ui.damageVignette=0;
        g.enemies.clear();g.gameClock=5;aim(g,cold::chiller(0).c+glm::vec3{4,0,16},cold::chiller(0).c+glm::vec3{0,2,0});
        g.render();capture("build/cold-pass/chillers-floor.bmp");
        aim(g,cold::chiller(0).c+glm::vec3{-13,3.2f,0},cold::chiller(0).c+glm::vec3{10,3,-1});
        g.render();capture("build/cold-pass/chillers-roof.bmp");
        g.enterArena(7);g.armoryOpen=false;g.director.phase=WaveDirector::Phase::ACTIVE;
        g.enemies.clear();g.spawnEnemy(EnemyType::UNIT24K,cold::station()+glm::vec3{8,0,13.5f});
        g.enemies[0].spawnTimer=0;g.enemies[0].state=EnemyState::ACTIVE;
        bool active=false,inside=true;
        for(int f=0;f<60*24;++f){auto target=cold::station()+glm::vec3{f%600<300?-5.f:5.f,.6f,-5.f};aim(g,target,cold::station());
            g.updateEnemies(1.f/60);active|=g.enemies[0].ev.telegraphStarted;
            inside&=spawnClearance(g.level,g.enemies[0].position,g.enemies[0].radius(),g.enemies[0].height(),false);}
        CHECK(active&&inside,"24-K moves and attacks in the furnished yard without entering solid cover");
        g.ui.damageVignette=0;
        aim(g,cold::station()+glm::vec3{-23.5f,3.2f,-4},cold::station()+glm::vec3{5,1,3});
        g.render();capture("build/cold-pass/dispatch-balcony.bmp");
        g.enterArena(9);g.armoryOpen=false;g.enemies.clear();g.spawnEnemy(EnemyType::RESTORER,cold::vaultC()+glm::vec3{0,0,-12});
        auto& boss=g.enemies[0];boss.spawnTimer=0;boss.state=EnemyState::ACTIVE;
        const auto vc=cold::vaultC();aim(g,vc+glm::vec3{4,0,5},vc);
        g.rsTrailN=1;g.rsTrailHead=0;g.rsTrail[0]=vc+glm::vec3{24,0,0};
        boss.attack=AttackKind::RS_ROLLBACK;boss.telegraphDuration=2;
        EnemyEvents tell;tell.telegraphStarted=true;g.restorerEvents(boss,tell);
        CHECK(spawnClearance(g.level,g.rsRollbackAt,g.player.radius,g.player.height,true),"the rollback ghost marks a clear landing beside static cover");
        const auto ghost=g.rsRollbackAt;
        g.rsPlaceWall(8,{ghost+glm::vec3{-1,0,-1},ghost+glm::vec3{1,5,1}});
        const float yaw=g.player.camera.yaw,pitch=g.player.camera.pitch;
        g.gameClock=20;g.recallPressedAt=-100;
        EnemyEvents cast;cast.rsCast=AttackKind::RS_ROLLBACK;g.restorerEvents(boss,cast);
        CHECK(spawnClearance(g.level,g.player.position,g.player.radius,g.player.height,true)
            &&glm::length(g.player.position-ghost)<6.1f&&g.player.camera.yaw==yaw&&g.player.camera.pitch==pitch,
            "rollback rechecks newly placed racks and keeps the player's view at a nearby safe landing");
        const auto before=g.player.position;g.recallPressedAt=g.gameClock;g.restorerEvents(boss,cast);
        CHECK(g.player.position==before,"a last-moment Recall still overrides rollback");
        g.resetRestorerFight();g.rsCells={vc+glm::vec3{25,0,0}};g.rsFreezeT=.8f;
        std::vector<BoxInstance> warnings;g.gatherRestorerFight(warnings);bool topWarning=false;
        for(const auto& box:warnings)topWarning|=box.model[3].y>1.6f&&box.model[3].y<1.8f;
        CHECK(topWarning,"the actual freeze rendering includes the cover's upper surface");
        g_godMode=false;g.invincFrames=0;g.styleSystem.health=100;
        aim(g,vc+glm::vec3{24,1.6f,0},vc);cast.rsCast=AttackKind::RS_FREEZE;g.restorerEvents(boss,cast);
        CHECK(g.styleSystem.health<100&&g.player.velocity.y>=7,"standing on low cover remains vulnerable to the marked freeze cell");
        g_godMode=true;g.rsSpikeT=-1;g.rsFreezeT=.8f;g.ui.damageVignette=0;g.styleSystem.health=100;
        aim(g,vc+glm::vec3{24,1.6f,2},vc+glm::vec3{24,0,-2});g.render();capture("build/cold-pass/vault-freeze-cover.bmp");
        aim(g,vc+glm::vec3{-16,0,16},vc+glm::vec3{0,3,-8});g.render();capture("build/cold-pass/vault-floor.bmp");
        g.enterArena(9);
        CHECK(g.rsStatues.empty()&&g.rsRacks.empty()&&!spawnClearance(g.level,vc+glm::vec3{24,0,0},.4f,1.8f,false),"retry clears dynamic shelving and retains the authored cover");
        CHECK(glGetError()==GL_NO_ERROR,"all three combat spaces and raised freeze cues render without GL errors");
        CHECK(persist::load(StorySave::key).empty(),"practice verification leaves Story progress untouched");
        balancepanel::extraBuild={};app.currentState.reset();
    }
    std::remove(StorySave::key);ImGui_ImplOpenGL3_Shutdown();ImGui_ImplSDL2_Shutdown();ImGui::DestroyContext();
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    std::printf("%d failures\n",failures);return failures?1:0;
}
