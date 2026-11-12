// Live menu/input, attack scheduling, causal rewards, retries and rendering.
#include <SDL2/SDL.h>
#define main overdrive_app_main
#include "../src/main.cpp"
#undef main

static int failures=0;
#define CHECK(c,m) do{bool ok=(c);std::printf("%s %s\n",ok?"ok:":"FAIL:",m);failures+=!ok;}while(0)
static constexpr float DT=1.f/60;
static void press(GameState& state,SDL_Keycode key) {
    SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;
    e.key.keysym.scancode=SDL_GetScancodeFromKey(key);state.handleEvent(e);
}
static void capture(const char* name) {
    auto* s=SDL_CreateRGBSurfaceWithFormat(0,SCREEN_W,SCREEN_H,24,SDL_PIXELFORMAT_RGB24);
    if(!s){CHECK(false,"capture buffer");return;}
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,SCREEN_W,SCREEN_H,GL_RGB,GL_UNSIGNED_BYTE,s->pixels);
    auto* p=(unsigned char*)s->pixels;std::vector<unsigned char> row(s->pitch);
    for(int y=0;y<SCREEN_H/2;++y){auto* a=p+y*s->pitch;auto* b=p+(SCREEN_H-1-y)*s->pitch;
        std::memcpy(row.data(),a,s->pitch);std::memcpy(a,b,s->pitch);std::memcpy(b,row.data(),s->pitch);}
    CHECK(SDL_SaveBMP(s,(std::string("build/combat-pass/")+name+".bmp").c_str())==0,"review capture saved");SDL_FreeSurface(s);
}
static void look(GameplayState& g,glm::vec3 feet,glm::vec3 target) {
    g.player.position=feet;g.player.velocity={0,0,0};g.player.camera.position=feet+glm::vec3{0,g.player.eyeHeight,0};
    g.prevCamPos=g.player.camera.position;auto d=glm::normalize(target-g.player.camera.position);
    g.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));g.player.camera.pitch=glm::degrees(std::asin(d.y));
}
int main() {
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!=0)return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window=SDL_CreateWindow("Combat pass checks",0,0,SCREEN_W,SCREEN_H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window)return 1;auto context=SDL_GL_CreateContext(window);if(!context)return 1;
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental=GL_TRUE;glewInit();glGetError();
#endif
    glViewport(0,0,SCREEN_W,SCREEN_H);balanceInit();balancepanel::init(window,context);
    g_devNoMouse=true;g_fixedDt=DT;g_devCam=g_godMode=g_combatRemix=false;g_devOverlay.clear();
    g_bossRun=-1;g_bossRush=false;g_startArena=0;g_storyChapter=1;
    StorySave::key="build/combat-pass/story.cfg";persist::save(StorySave::key,"");
    const auto originalRecords=persist::load(Records::kKey);
    {
        App app;app.window=window;app.ctx=context;
        for(const auto& cue:mixTable())app.audio.loadSound(cue.name,std::string("assets/sfx/")+cue.name+".wav");
        app.pending=App::NextState::Menu;app.applyPendingState();
        auto& menu=*static_cast<MenuState*>(app.currentState.get());
        menu.page=MenuState::STORYPAGE;menu.hasStorySave=false;menu.storySel=7;   // (remix: after the six chapters and practice)
        menu.render();capture("remix-menu");press(menu,SDLK_RETURN);app.applyPendingState();
        auto& g=*static_cast<GameplayState*>(app.currentState.get());g.closeArmory();g.paused=false;
        CHECK(g.combatRemix() && g.director.arena==3 && g_practice && !g.ranked && g.storyOwned==15,
              "the Story page launches one unranked, fully armed remix through the real deferred transition");
        CHECK(g.splinterOwned() && g.recallOwned() && g.swordAllowed() && g.storyGrapple,
              "SPLINTER, RECALL, Oathbreaker, guns and grapple coexist without developer flags");
        const auto c=lab::centre(3);g.enemies.clear();g.director.hold=true;g.banners.clear();
        look(g,c+glm::vec3{0,0,18},c+glm::vec3{0,1,10});
        press(g,SDLK_g);g.shootInput();press(g,SDLK_x);g.shootInput();
        CHECK(g.splinters.live()>0 && g.recall.placed,"normal grenade and RECALL bindings operate together in the remix");
        press(g,SDLK_5);CHECK(g.swordOut,"the normal sword key draws Oathbreaker in the remix");g.holsterSword();
        g.styleSystem.addStyle(400);g.attackOverdrive();
        CHECK(g.styleSystem.overdriveReady && !g.styleSystem.overdrive,"shooting an empty room cannot spend a banked Overdrive charge");
        g.render();capture("overdrive-ready");
        g.spawnEnemy(EnemyType::HUSK,c+glm::vec3{0,0,10});g.enemies.back().spawnTimer=0;g.enemies.back().state=EnemyState::ACTIVE;
        g.weapons[0].ammo=0;g.weapons[0].reloading=true;g.weapons[0].cooldown=0;
        g.pendingFire=true;g.shootInput();
        CHECK(g.styleSystem.overdrive && !g.styleSystem.overdriveReady && g.runLog.overdrives==1 && g.weapons[0].ammo==weaponMag(WeaponId::REVOLVER,g.prog.up[0]),
              "the next trigger pull refills an empty gun, activates once and receives Overdrive's ammo behavior immediately");

        g.styleSystem=StyleSystem{};g.enemies.clear();app.settings.difficulty=1;
        for(auto type:{EnemyType::SUSPENDED,EnemyType::GRAFTED,EnemyType::SUSPENDED}) {
            g.spawnEnemy(type,c+glm::vec3{float(g.enemies.size())*7-7,8,-20});
            auto& e=g.enemies.back();e.spawnTimer=0;e.state=EnemyState::ACTIVE;e.attackTimer=100;
        }
        g.enemies[0].labBeam=1.f;g.assignAttackers();int granted=0;
        for(size_t i=1;i<g.enemies.size();++i)granted+=g.enemies[i].mayAttack;
        CHECK(granted==1,"an active beam reserves pressure so only one further heavy attack may start on Standard");
        g.enemies[0].labBeam=0;g.enemies[0].stunTimer=.5f;g.assignAttackers();
        CHECK(g.enemies[1].mayAttack && g.enemies[2].mayAttack,"interrupting the beam frees pressure for the remaining threats");

        g.enemies.clear();g.styleSystem=StyleSystem{};g.spawnEnemy(EnemyType::BRUTE,c+glm::vec3{0,0,15});
        auto& brute=g.enemies.back();brute.spawnTimer=0;brute.state=EnemyState::ACTIVE;
        GameplayState::HitPush stagger{{0,0,-1},8,20};
        g.hurtEnemy(brute,20,brute.position+glm::vec3{0,1,0},0,0,StyleSource::SHOTGUN,false,true,&stagger);
        CHECK(brute.stunned() && brute.staggerSetupAge==brute.age,"an actual nonlethal hit records the player's stagger setup");
        float before=g.styleSystem.earned;
        g.hurtEnemy(brute,1000,brute.position,0,0,StyleSource::REVOLVER,false,true);
        CHECK(!brute.alive && g.styleSystem.earned-before>60,"following the stagger with a real kill awards a combat-chain bonus");
        g.enemies.clear();g.styleSystem=StyleSystem{};g.styleSystem.style=80;
        g.spawnEnemy(EnemyType::HUSK,c+glm::vec3{0,0,15});auto& pushed=g.enemies.back();pushed.spawnTimer=0;pushed.state=EnemyState::ACTIVE;
        GameplayState::HitPush shove{{0,0,-1},10,0};
        g.hurtEnemy(pushed,1,pushed.position,0,0,StyleSource::SHOTGUN,false,true,&shove);before=g.styleSystem.earned;
        g.hurtEnemy(pushed,1000,pushed.position,0,0,StyleSource::ENVIRONMENT,false,true);
        CHECK(g.styleSystem.earned>before && g.styleSystem.style==80,"a player shove followed by environmental death earns setup points without bypassing the passive meter cap");
        g.enemies.clear();g.spawnEnemy(EnemyType::UNIT24K,c+glm::vec3{0,0,15});auto& clone=g.enemies.back();
        clone.spawnTimer=0;clone.state=EnemyState::ACTIVE;clone.attack=AttackKind::SHOT;clone.telegraphTimer=.8f;
        g.hurtEnemy(clone,60,clone.position+glm::vec3{0,1.7f,0},0,0,StyleSource::REVOLVER,true,true);
        CHECK(clone.staggered() && clone.attack==AttackKind::NONE,"the live precision damage path interrupts 24-K's Lancer charge");

        g.enterArena(3);g.director.hold=false;g.director.startArena(3);g.banners.clear();
        g.ui.overdriveFlash=0;g.muzzleFlashTimer=0;
        std::vector<SpawnRequest> spawned;g.director.update(DT,0,g.player.position,spawned);g.director.update(DT,0,g.player.position,spawned);
        for(const auto& s:spawned)g.spawnEnemy(s.type,s.pos,s.hollow);
        for(auto& e:g.enemies){e.spawnTimer=0;e.state=EnemyState::ACTIVE;}
        look(g,c+glm::vec3{0,10,31},c+glm::vec3{0,1,-12});g.ui.damageVignette=0;g.render();capture("surgical-opening");
        g.director.events.clear();spawned.clear();g.director.update(DT,2,g.player.position,spawned);g.handleDirectorEvents();
        g.render();capture("reinforcement-warning");
        g.director.events={{DirectorEvent::ARENA_CLEARED,3}};g.handleDirectorEvents();
        CHECK(g.victory,"clearing Surgical Observation finishes the remix instead of advancing the campaign");
        g.render();capture("remix-complete");press(g,SDLK_RETURN);
        CHECK(g.director.arena==3 && g.storyOwned==15 && g.storyGrapple && g.splinters.charges==3 && !g.recall.placed && !g.victory,
              "Enter retries the completed remix with its full equipment and fresh ability state");
        g.enterArena(6);const auto care=lab::centre(6);
        look(g,care+glm::vec3{0,10,31},care+glm::vec3{0,1,0});g.banners.clear();g.ui.damageVignette=0;g.render();capture("long-term-care");
        {
            Player p(care+glm::vec3{-22,0,8});p.camera.yaw=-90;SpatialGrid grid;grid.build(g.level.walls);bool roof=false;
            for(int f=0;f<150;++f){Uint8 keys[SDL_NUM_SCANCODES]={};keys[SDL_SCANCODE_W]=p.position.z>care.z+1;
                keys[SDL_SCANCODE_SPACE]=(f>=5&&f<26)||(f>=30&&f<65);if(f==30)p.airJump();
                applyWater(p,g.level);p.update(DT,keys,g.level.walls.data(),(int)g.level.walls.size(),false,&grid);
                roof|=p.onGround && std::abs(p.position.y-(care.y+2.4f))<.1f;}
            CHECK(roof,"a new Long-Term Care bank roof is reachable using the real double-jump controller");
        }
        CHECK(glGetError()==GL_NO_ERROR,"new menu, combat states and revised room render without OpenGL errors");
        CHECK(persist::load(Records::kKey)==originalRecords && persist::load(StorySave::key).empty(),"the remix leaves ordinary records and Story progress untouched");
        app.pending=App::NextState::Menu;app.applyPendingState();auto& normal=*static_cast<MenuState*>(app.currentState.get());
        normal.onStart(GameMode::STORY,StartOptions{});app.applyPendingState();
        CHECK(!g_combatRemix && !static_cast<GameplayState*>(app.currentState.get())->splinterOwned(),"a later normal campaign clears the remix's additional tools");
        balancepanel::extraBuild={};app.currentState.reset();
    }
    std::remove(StorySave::key);
    ImGui_ImplOpenGL3_Shutdown();ImGui_ImplSDL2_Shutdown();ImGui::DestroyContext();
    SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
    std::printf("%d live combat failures\n",failures);return failures?1:0;
}
