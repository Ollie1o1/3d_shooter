// Exercise deferred App transitions, actual input, collision and GL rendering.
#include <SDL2/SDL.h>
#define main overdrive_app_main
#include "../src/main.cpp"
#undef main

static int failures=0;
#define CHECK(c,m) do{bool ok=(c);std::printf("%s %s\n",ok?"ok:":"FAIL:",m);failures+=!ok;}while(0)
static void press(GameState& state,SDL_Keycode key){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;e.key.keysym.scancode=SDL_GetScancodeFromKey(key);state.handleEvent(e);}
static void capture(const char* file){
    auto* s=SDL_CreateRGBSurfaceWithFormat(0,SCREEN_W,SCREEN_H,24,SDL_PIXELFORMAT_RGB24);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,SCREEN_W,SCREEN_H,GL_RGB,GL_UNSIGNED_BYTE,s->pixels);
    std::vector<unsigned char> row(s->pitch);auto* p=(unsigned char*)s->pixels;
    for(int y=0;y<SCREEN_H/2;++y){auto* a=p+y*s->pitch;auto* b=p+(SCREEN_H-1-y)*s->pitch;std::memcpy(row.data(),a,s->pitch);std::memcpy(a,b,s->pitch);std::memcpy(b,row.data(),s->pitch);}
    SDL_SaveBMP(s,file);SDL_FreeSurface(s);
}
static void aimAt(GameplayState& g,glm::vec3 feet,glm::vec3 target){
    g.player.position=feet;g.player.velocity={0,0,0};g.player.camera.position=feet+glm::vec3{0,g.player.eyeHeight,0};g.prevCamPos=g.player.camera.position;
    auto d=glm::normalize(target-g.player.camera.position);g.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));g.player.camera.pitch=glm::degrees(std::asin(d.y));
}
static MenuState& menu(App& app){app.pending=App::NextState::Menu;app.applyPendingState();return *static_cast<MenuState*>(app.currentState.get());}
static GameplayState& game(App& app){app.applyPendingState();return *static_cast<GameplayState*>(app.currentState.get());}

int main(){
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!=0)return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window=SDL_CreateWindow("Encounter pass checks",0,0,SCREEN_W,SCREEN_H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window)return 1;auto context=SDL_GL_CreateContext(window);if(!context)return 1;
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental=GL_TRUE;glewInit();glGetError();
#endif
    glViewport(0,0,SCREEN_W,SCREEN_H);balanceInit();balancepanel::init(window,context);
    g_devNoMouse=true;g_fixedDt=1.0/60;g_devCam=g_godMode=false;g_devOverlay.clear();
    StorySave::key="build/next-pass/story.cfg";BossRushRecords::key="build/next-pass/rush.cfg";
    const auto ordinaryRecords=persist::load(Records::kKey);
    {
        App app;app.window=window;app.ctx=context;
        for(const auto& cue:mixTable())app.audio.loadSound(cue.name,std::string("assets/sfx/")+cue.name+".wav");
        auto& main=menu(app);main.selected=3;press(main,SDLK_RETURN);
        CHECK(main.page==MenuState::ENDLESSPAGE,"Endless opens the random-run and challenge-code page through the main menu");
        main.challengeInput="OD1-12345-2";main.endlessSel=2;press(main,SDLK_RETURN);
        auto& seeded=game(app);
        CHECK(seeded.endless()&&seeded.endlessChallenge.seed==12345&&app.settings.difficulty==2&&!seeded.ranked&&seeded.settingsMenu.lockDifficulty,"a menu challenge starts its seed with fixed difficulty and no ranking");
        const auto first=seeded.level.arenas[3].waves[0];seeded.newRun();
        CHECK(!seeded.ranked&&seeded.level.arenas[3].waves[0][0].type==first[0].type,"restarting a challenge keeps its sequence and unranked status");
        seeded.paused=true;seeded.render();capture("build/next-pass/challenge-pause.bmp");
        auto& normalMenu=menu(app);normalMenu.selected=3;press(normalMenu,SDLK_RETURN);press(normalMenu,SDLK_RETURN);auto& normal=game(app);
        CHECK(!normal.endlessChallenge.active&&!g_endlessChallenge.active,"a later random run clears the selected challenge");
        auto& bossMenu=menu(app);bossMenu.selected=4;press(bossMenu,SDLK_RETURN);bossMenu.bossSel=1;
        press(bossMenu,SDLK_p);press(bossMenu,SDLK_p);bossMenu.render();capture("build/next-pass/boss-menu.bmp");press(bossMenu,SDLK_RETURN);
        auto& practice=game(app);practice.closeArmory();practice.director.skipIntro();
        for(int f=0;f<40;++f)practice.update(1.f/60);
        auto findSovereign=[](GameplayState& g)->Enemy*{for(auto& e:g.enemies)if(e.type==EnemyType::SOVEREIGN)return &e;return nullptr;};
        auto* sovereign=findSovereign(practice);
        CHECK(g_bossPhase==2&&g_practice&&sovereign&&sovereign->enraged&&std::abs(sovereign->health/sovereign->maxHealth-.45f)<.001f,"phase selection reaches a real enraged Sovereign fight through App");
        press(practice,SDLK_BACKSPACE);practice.director.skipIntro();
        for(int f=0;f<40;++f)practice.update(1.f/60);
        sovereign=findSovereign(practice);
        CHECK(sovereign&&sovereign->enraged&&!practice.ranked,"immediate retry restores the chosen phase");
        auto& rushMenu=menu(app);rushMenu.selected=4;press(rushMenu,SDLK_RETURN);press(rushMenu,SDLK_r);auto& firstBoss=game(app);
        firstBoss.closeArmory();firstBoss.prog.up[0].tier[0]=2;firstBoss.prog.points=0;
        firstBoss.styleSystem.health=20;firstBoss.bossStartAt=0;firstBoss.elapsedTime=10;firstBoss.bossRunDone();firstBoss.finishRun();
        press(firstBoss,SDLK_RETURN);
        CHECK(app.bossRush.stage==1&&app.pending==App::NextState::Game&&app.currentState.get()==&firstBoss,"rush advancement defers destroying the active game until the next state transition");
        auto& secondBoss=game(app);
        CHECK(g_bossRun==1&&secondBoss.styleSystem.health==45&&secondBoss.prog.up[0].tier[0]==2&&secondBoss.prog.points==1,"the next boss carries upgrades, adds its point allowance and restores twenty-five health");
        secondBoss.armoryOpen=false;secondBoss.playerDead=true;press(secondBoss,SDLK_r);auto& restarted=game(app);
        CHECK(app.bossRush.stage==0&&app.bossRush.total()==0&&g_bossRun==0&&restarted.prog.up[0].tier[0]==0,"death retry restarts the entire one-life rush and its upgrades");
        bool complete=true;
        for(int stage=0;stage<6;++stage){
            auto& live=*static_cast<GameplayState*>(app.currentState.get());live.armoryOpen=false;
            live.bossStartAt=0;live.elapsedTime=10.f+stage;live.bossRunDone();live.finishRun();
            complete&=app.bossRush.stage==stage&&app.bossRush.splits[stage]==10.f+stage&&!live.bossPost;
            if(stage==5){live.render();capture("build/next-pass/rush-complete.bmp");}
            else {press(live,SDLK_RETURN);app.applyPendingState();}
        }
        CHECK(complete&&app.bossRush.complete()&&app.bossRush.total()==75,"all six real level transitions complete the rush without posting individual boss scores");
        auto& labMenu=menu(app);StartOptions start;start.storyChapter=3;start.arena=2;start.practice=true;labMenu.onStart(GameMode::STORY,start);
        auto& motor=game(app);motor.armoryOpen=false;motor.director.phase=WaveDirector::Phase::ACTIVE;
        motor.enemies.clear();motor.spawnEnemy(EnemyType::PATIENT,lab::motorLoadAt());motor.spawnEnemy(EnemyType::PATIENT,lab::motorLoadAt()+glm::vec3{10,0,0});
        for(auto& e:motor.enemies){e.spawnTimer=0;e.state=EnemyState::ACTIVE;}
        aimAt(motor,lab::centre(2)+glm::vec3{-26,6,-10},lab::motorLoadControl());press(motor,SDLK_e);
        CHECK(motor.motorLoad.age==0,"the gallery control releases the environmental load through the real interact ray");
        motor.updateMotorLoad(1.29f);CHECK(motor.enemies[0].alive&&motor.level.movers[motor.level.story.motorLoad].drive==0,"the announced warning cannot damage or move the load early");
        motor.player.position=lab::motorLoadAt();motor.invincFrames=0;motor.styleSystem.health=100;
        motor.updateMotorLoad(.62f);motor.level.updateMovers(0);
        CHECK(!motor.enemies[0].alive&&motor.enemies[1].alive&&motor.styleSystem.health==45,"the landed load damages its marked footprint and leaves distant enemies alone");
        CHECK(spawnClearance(motor.level,motor.player.position,.45f,motor.player.height,true),"a surviving player is pushed onto supported clear ground rather than trapped in landed cover");
        motor.enterArena(2);
        CHECK(motor.motorLoad.age<0&&motor.level.movers[motor.level.story.motorLoad].drive==0,"retry restores the load, control and moving collision");
        {
            Player p(lab::centre(2)+glm::vec3{-22,0,-20});p.camera.yaw=-90;SpatialGrid grid;grid.build(motor.level.walls);bool roof=false;
            for(int f=0;f<150;++f){Uint8 keys[SDL_NUM_SCANCODES]={};keys[SDL_SCANCODE_W]=p.position.z>lab::centre(2).z-26;
                keys[SDL_SCANCODE_SPACE]=(f>=5&&f<26)||(f>=30&&f<65);
                if(f==30)p.airJump(); // GameplayState's normal second-jump action.
                applyWater(p,motor.level);p.update(1.f/60,keys,motor.level.walls.data(),(int)motor.level.walls.size(),false,&grid);
                roof|=p.onGround&&std::abs(p.position.y-(lab::centre(2).y+2.4f))<.1f;}
            CHECK(roof,"the new diagnostic bank can actually be climbed with normal jump inputs");
        }
        aimAt(motor,lab::centre(2)+glm::vec3{-22,2.4f,-25},motor.level.story.terminals.back().pos+glm::vec3{0,1.8f,.25f});
        motor.enemies.clear();press(motor,SDLK_e);
        CHECK(motor.storyTerminal==(int)motor.level.story.terminals.size()-1,"the roof discovery can be read through the normal interact path");
        motor.storyTerminal=-1;motor.ui.damageVignette=0;
        motor.encounterDraft.capture(motor.level,2,encounterpanel::mapKey(motor));motor.encounterPreview=true;g_balanceOpen=true;balancepanel::showEncounter=true;
        motor.render();balancepanel::draw(0,SCREEN_W,SCREEN_H);capture("build/next-pass/workshop.bmp");g_balanceOpen=false;motor.encounterPreview=false;
        aimAt(motor,lab::centre(2)+glm::vec3{0,11,31},lab::centre(2)+glm::vec3{0,0,-12});motor.render();capture("build/next-pass/motor-overlook.bmp");
        CHECK(glGetError()==GL_NO_ERROR,"the revised room, workshop, challenge page and rush results render without GL errors");
        CHECK(persist::load(Records::kKey)==ordinaryRecords&&persist::load(StorySave::key).empty(),"practice, edited encounters and captured rushes leave ordinary records and Story saves untouched");
        balancepanel::extraBuild={};app.currentState.reset();
    }
    std::remove(StorySave::key);std::remove(BossRushRecords::key);
    ImGui_ImplOpenGL3_Shutdown();ImGui_ImplSDL2_Shutdown();ImGui::DestroyContext();
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    std::printf("%d failures\n",failures);return failures?1:0;
}
