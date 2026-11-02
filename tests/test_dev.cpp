// Native integration: the dev menu's real input paths and Story checkpoint starts.
#include "ui/MenuState.h"
#include "game/GameplayState.h"
#include <cstdio>

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { ++failures; std::printf("FAIL: %s\n", m); } else std::printf("ok: %s\n", m); } while (0)

static void press(GameState& state, SDL_Keycode key) {
    SDL_Event e{}; e.type = SDL_KEYDOWN; e.key.keysym.sym = key;
    state.handleEvent(e);
}

static void screenshot(const char* path) {
    auto* pixels = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 24, SDL_PIXELFORMAT_RGB24);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, pixels->pixels);
    std::vector<unsigned char> row(pixels->pitch);
    auto* data = static_cast<unsigned char*>(pixels->pixels);
    for (int y = 0; y < SCREEN_H / 2; ++y) {
        auto* top = data + y * pixels->pitch;
        auto* bottom = data + (SCREEN_H - y - 1) * pixels->pitch;
        std::memcpy(row.data(), top, row.size());
        std::memcpy(top, bottom, row.size());
        std::memcpy(bottom, row.data(), row.size());
    }
    SDL_SaveBMP(pixels, path); SDL_FreeSurface(pixels);
}

static void checkImprovements(AudioSystem& audio) {
    struct TestSlot {
        const char* old=StorySave::key;
        TestSlot(){StorySave::key="build/story-integration.cfg";}
        ~TestSlot(){std::remove(StorySave::key);StorySave::key=old;}
    } slot;
    g_devOverlay.clear();g_bossRun=g_roomAttack=g_arenaSolo=-1;
    g_practice=g_godMode=false;g_continueStory=false;
    for(int chapter:{1,2}) {
        StorySave s;s.chapter=chapter;s.checkpoint=chapter==1?5:4;s.owned=chapter==1?7:15;
        s.grapple=s.wakeSeen=true;s.logs=5;s.terminals=3;s.relayIsolated=chapter==2;
        s.prog.level=5;s.prog.points=3;s.prog.up[1].mod=s.prog.up[1].alternate=true;
        s.kills=23;s.deaths=2;s.elapsed=150;s.style=450;s.damage=80;s.save();
        g_continueStory=true;g_storyChapter=chapter==1?2:1;g_startArena=0;
        {
            auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
            CHECK(game.level.story.chapter==chapter && game.storyCheckpoint==s.checkpoint && game.director.arena==s.checkpoint,
                "Continue restores the saved chapter and checkpoint instead of the launch defaults");
            CHECK(game.storyOwned==s.owned && game.storyLogsSeen==5 && game.storyTerminalsSeen==3 && game.storyGrapple && game.prog.up[1].alternate,
                "Continue restores found equipment, records and the chosen weapon mod");
            CHECK(game.totalKills==23 && game.deaths==2 && game.elapsedTime==150 && game.bankedStyle==450 && game.styleSystem.health==100,
                "Continue keeps statistics and returns to a safe checkpoint with full health");
            game.render();screenshot(chapter==1?"build/continue-cradle.bmp":"build/continue-span.bmp");
            const auto before=persist::load(StorySave::key);
            g_practice=true;g_devNoMouse=false;game.saveStoryProgress();g_devNoMouse=true;g_practice=false;
            CHECK(persist::load(StorySave::key)==before,"practice cannot overwrite the Story continue slot");
        }
    }
    {
        StorySave s;s.chapter=1;s.checkpoint=8;s.owned=7;s.grapple=s.wakeSeen=s.bossDown=true;
        s.prog.level=4;s.prog.points=2;s.save();g_continueStory=true;
        auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
        CHECK(game.storyBossDown && game.director.phase==WaveDirector::Phase::CLEARED && !game.level.doors[game.level.arenas[8].exitDoor].locked,
            "resuming after the Sorter keeps it defeated and its exit open");
        game.retryArena();CHECK(game.storyBossDown && game.director.phase==WaveDirector::Phase::CLEARED,
            "retry cannot resurrect a defeated Story boss");
    }
    {
        StorySave s;s.chapter=1;s.checkpoint=8;s.owned=7;s.grapple=s.wakeSeen=s.bossDown=s.completed=true;
        s.prog.level=4;s.prog.points=2;s.save();g_continueStory=true;
        auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
        CHECK(game.wasteland() && game.storyCheckpoint==0 && game.prog.level==4 && game.prog.points==2 && !game.storyBossDown,
            "Continue on a completed Cradle starts Chapter 2 with the carried upgrades");
    }
    {
        MenuState menu(SCREEN_W,SCREEN_H);GameMode chosen=GameMode::ARENA;StartOptions options;
        menu.onStart=[&](GameMode m,StartOptions o){chosen=m;options=o;};
        press(menu,SDLK_F3);CHECK(chosen==GameMode::TRAINING && options.practice,"F3 starts unranked combat practice through the real menu");
        menu.page=MenuState::STORYPAGE;menu.render();screenshot("build/improved-story-menu.bmp");
        press(menu,SDLK_RETURN);CHECK(options.continueStory && chosen==GameMode::STORY,"Continue menu input requests the stored Story run");
        menu.page=MenuState::BOSS;menu.bossSel=8;menu.render();screenshot("build/improved-boss-menu.bmp");
        for(int i=6;i<BOSS_COUNT;++i){menu.bossSel=i;press(menu,SDLK_RETURN);CHECK(options.boss==i && chosen==bossEntry(i).mode() && options.storyChapter==bossEntry(i).chapter,
            "the appended boss entry selects its actual mode, map and chapter");}
    }
    g_practice=true;
    for(int i=6;i<BOSS_COUNT;++i) {
        g_bossRun=i;g_storyChapter=bossEntry(i).chapter;g_startArena=bossEntry(i).arena;g_startWave=0;
        auto live=std::make_unique<GameplayState>(audio,nullptr,bossEntry(i).mode());auto& game=*live;
        CHECK(game.director.wave==game.bossWaveOf(game.director.arena) && !game.ranked,
            "new Boss entries start on the boss wave with an unranked run");
        if(game.armoryOpen)press(game,SDLK_TAB);
        std::vector<SpawnRequest> spawns;
        for(int n=0;n<200 && spawns.empty();++n)game.director.update(1.f/60,0,game.player.position,spawns);
        bool correct=false;EnemyType expected=i==6?EnemyType::PACEMAKER:i==7?EnemyType::SORTER:i==8?EnemyType::BALLAST:EnemyType::SUCCESSOR;
        for(const auto& spawn:spawns){correct|=spawn.type==expected;game.spawnEnemy(spawn.type,spawn.pos,spawn.hollow);}
        CHECK(correct,"the real director spawns the selected new boss without its preceding waves");
        game.handleDirectorEvents();game.render();
        for(auto& e:game.enemies)if(e.type==expected){e.health=0;e.alive=false;game.onEnemyKilled(e,StyleSource::REVOLVER);}
        game.director.update(1.f/60,0,game.player.position,spawns);game.handleDirectorEvents();
        CHECK(game.bossDone && !game.bossPost,"the new boss finishes locally without submitting an unsupported world board");
    }
    g_bossRun=-1;g_practice=true;g_startArena=g_startWave=0;
    {
        GameSettings settings;auto live=std::make_unique<GameplayState>(audio,&settings,GameMode::TRAINING);auto& game=*live;
        settings.binds.bind(Act::PARRY,0,SDL_SCANCODE_H);
        CHECK(lessons::action(0,game.binds()).find("H AS A SHOT")!=std::string::npos,"the field guide teaches the player's actual binding");
        CHECK(game.guideOpen && game.paused && !game.ranked,"combat practice starts in its selectable field guide");
        game.render();screenshot("build/combat-field-guide.bmp");
        press(game,SDLK_DOWN);press(game,SDLK_RETURN);game.updateTraining(.2f);
        CHECK(!game.guideOpen && !game.paused && game.trainingLesson==1 && game.enemies.size()==1,
            "selecting a lesson closes the guide and creates its repeatable target");
        for(auto& e:game.enemies){e.state=EnemyState::ACTIVE;e.spawnTimer=0;}
        bool finished=game.tryFinisher();
        CHECK(finished && game.trainingPassed[1] && game.styleSystem.health>55 && game.weapons[0].ammo==weaponMag(WeaponId::REVOLVER,game.prog.up[0]),
            "a real training finisher completes its lesson, restores health and reloads the gun");
        game.startTrainingLesson(0);game.projSystem.fire(game.player.camera.position+glm::vec3{0,0,-1.5f},{0,0,10},10,false,{1,1,1});
        game.punch(-1);CHECK(game.trainingPassed[0],"parrying an actual incoming projectile completes the parry lesson");
        game.startTrainingLesson(2);game.updateTraining(.2f);
        for(auto& e:game.enemies){e.state=EnemyState::ACTIVE;e.spawnTimer=0;}
        game.hookEnemy(0);CHECK(game.trainingPassed[2],"hooking the elevated light target completes enemy grapple practice");
        game.startTrainingLesson(3);game.player.camera.pitch=-80;game.doubleShot=true;game.fireWeapon(1);game.doubleShot=false;
        const float normalKick=game.player.velocity.y;
        CHECK(game.trainingPassed[3] && normalKick>10,"a real double-barrel jump completes the recoil lesson");
        game.prog.up[1].mod=game.prog.up[1].alternate=true;game.startTrainingLesson(3);game.player.camera.pitch=-80;
        game.doubleShot=true;game.fireWeapon(1);game.doubleShot=false;
        CHECK(game.player.velocity.y>normalKick*1.7f,"Breach Kick changes actual movement rather than only the armory description");
        game.startTrainingLesson(4);game.throwGrenade();int grenade=-1;
        for(int n=0;n<ProjectileSystem::POOL_SIZE;++n)if(game.projSystem.pool[n].alive && game.projSystem.pool[n].isGrenade)grenade=n;
        game.punch(grenade);CHECK(game.trainingPassed[4] && !game.pendingBlasts.empty(),"punching the thrown grenade completes the real explosive interaction");
        game.invincFrames=0;game.damagePlayer(1000,{0,0,0},0,0);
        CHECK(!game.playerDead && game.styleSystem.health>=1,"practice damage leaves the player alive for another attempt");
        game.startTrainingLesson(7);game.openArmory();game.armoryW=0;game.armoryS=3;
        const int points=game.prog.points;press(game,SDLK_m);press(game,SDLK_RETURN);
        CHECK(game.prog.up[0].mod && game.prog.up[0].alternate && game.prog.points==points-2,"the selected alternative can be purchased through armory input");
        press(game,SDLK_RETURN);CHECK(!game.prog.up[0].alternate && game.prog.points==points-2,"an owned mod swaps without charging again");
        game.render();screenshot("build/weapon-mod-choices.bmp");
        press(game,SDLK_TAB);press(game,SDLK_F4);press(game,SDLK_ESCAPE);
        CHECK(!game.paused && !game.guideOpen,"F4 reference returns directly to the previous live game");
        game.render();screenshot("build/combat-practice.bmp");
        bool targets=true;
        for(int choice=0;choice<lessons::TARGET_COUNT;++choice){
            game.trainingEnemy=choice;game.startTrainingLesson(7);game.updateTraining(.2f);
            targets&=game.enemies.size()==(lessons::squad(choice)?4u:1u);
            for(const auto& e:game.enemies)targets&=!bodyEmbedded(game.level.walls,game.level.moverWalls,game.spatialGrid,e.position,e.radius(),e.height());
        }
        CHECK(targets,"every free-practice target and the mixed Ward squad spawn with real capsule clearance");
        game.trainingEnemy=lessons::SOLO_TARGETS-1;game.startTrainingLesson(7);
        game.padButton(SDL_CONTROLLER_BUTTON_X);game.updateTraining(.2f);
        CHECK(lessons::squad(game.trainingEnemy)&&game.enemies.size()==4,"controller X can select and play the mixed Ward squad during free practice");
        const glm::vec3 display[]={{-6,0,0},{0,0,-4},{5,6,-4},{8,0,-1}};
        for(int n=0;n<4;++n){auto& e=game.enemies[n];e.position=display[n];e.prevPosition=e.position;e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.telegraphDuration=1;e.telegraphTimer=.65f;
            e.attack=n==0?AttackKind::LAB_LUNGE:n==1?AttackKind::LAB_PRESS:n==2?AttackKind::LAB_BEAM:AttackKind::LAB_CROSS;
            e.labAim=game.player.camera.position;e.labExit={-2,0,3};if(n==0)e.telegraphTimer=.2f;
        }
        game.player.camera.yaw=-90;game.player.camera.pitch=-5;game.render();screenshot("build/enemy-ward-counters.bmp");
        std::vector<BoxInstance> tellGeometry;game.gatherBoxes(tellGeometry,game.player.camera.viewMatrix());bool cameraClear=true;
        for(const auto& box:tellGeometry){auto centre=glm::vec3(box.model[3]);auto eye=game.player.camera.position;
            if(std::fabs(centre.y-eye.y)<.2f && glm::length(glm::vec2{centre.x-eye.x,centre.z-eye.z})<1.6f)cameraClear=false;}
        CHECK(cameraClear,"a Suspended aim tell never wraps a ring around the camera at eye height");
        auto& beam=game.enemies[2];beam.attack=AttackKind::NONE;beam.telegraphTimer=0;beam.labBeam=.4f;
        // Capture a sweep the player has sidestepped, rather than looking
        // straight down its axis (which projects the whole ray to one point).
        beam.labDir=glm::normalize(glm::vec3{-6,1,5}-(beam.position+glm::vec3{0,beam.height()*.85f,0}));
        EnemyWorld beamWorld;beamWorld.walls=game.level.walls.data();beamWorld.wallCount=(int)game.level.walls.size();beamWorld.bounds=game.level.arenas[0].bounds;
        beam.update(1.f/60.f,beamWorld);game.render();screenshot("build/enemy-ward-beam.bmp");
    }
    {
        g_storyChapter=2;g_startArena=2;auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
        auto find=[&](const char* label)->Interactable*{for(auto& it:game.interactables)if(it.label.rfind(label,0)==0)return &it;return nullptr;};
        auto* isolation=find("ISOLATE SECURITY");CHECK(isolation!=nullptr,"the relay has an explicit optional power control");
        if(isolation)isolation->onInteract();
        CHECK(game.storyRelayIsolated && !game.level.arenas[2].goals[0].refill,"the relay control turns off reinforcements without completing the repair");
        game.retryArena();CHECK(!game.level.arenas[2].goals[0].refill,"the chosen relay isolation survives a checkpoint retry");
        game.enterArena(4);auto* winch=find("RELEASE COUNTERWEIGHT");
        auto drop=game.level.story.winchDrop;
        game.spawnEnemy(EnemyType::HUSK,drop+glm::vec3{3,0,0});
        for(auto& e:game.enemies){e.state=EnemyState::ACTIVE;e.spawnTimer=0;}
        if(winch)winch->onInteract();
        game.updateStoryChoices(1.25f);
        CHECK(game.enemies[0].alive && !game.storyWinchLanded,"the winch warns before damaging anything");
        game.render();screenshot("build/winch-warning.bmp");
        game.player.position=drop+glm::vec3{4,0,0};game.styleSystem.health=100;
        game.updateStoryChoices(.55f);
        CHECK(game.styleSystem.health==100,"standing outside the marked winch zone avoids player impact damage");
        CHECK(game.storyWinchLanded && !game.enemies[0].alive && game.level.walls[game.level.story.winchWall].box.min.y==0,
            "the falling load hits enemies and becomes solid ground-level cover");
        game.retryArena();CHECK(!game.storyWinchLanded && game.level.walls[game.level.story.winchWall].box.min.y==9,
            "retry restores the winch, its collision and its one-use control");
    }
    g_bossRun=-1;g_startArena=g_startWave=0;g_storyChapter=1;g_practice=g_godMode=false;g_continueStory=false;
}

static void checkLabRuntime(AudioSystem& audio){
    g_storyChapter=3;g_startArena=g_startWave=0;g_practice=g_godMode=false;g_devOverlay.clear();g_bossRun=-1;
    auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
    CHECK(game.labIntro.active() && game.storyLocked() && game.director.hold,"normal Chapter 3 begins in the first-person observation cutscene with combat held");
    game.labIntro.time=13;game.updateLab(.01f);game.render();screenshot("build/lab-intro-runtime.bmp");
    game.storySkipKey=true;game.updateLab(.7f);game.storySkipKey=false;
    CHECK(game.labIntro.seen && !game.storyLocked() && !game.director.hold && glm::length(game.player.position-glm::vec3{2.2f,0,20})<.01f,"the real skip path restores camera, combat and player control");
    game.retryArena();CHECK(!game.labIntro.active() && game.storyOwned==15 && game.storyGrapple,"a normal retry preserves the seen intro and full lab equipment");
    bool ledges=true;
    for(int room=0;room<9;++room){
        game.enterArena(room);auto c=lab::centre(room);
        auto feet=c+glm::vec3{lab::entryX(room+1),-13.6f,-79};
        game.player.position=feet;game.keepPlayerInZone(1.f/60.f);
        ledges&=glm::length(game.player.position-feet)<.01f;
        game.player.position=feet-glm::vec3{0,14,0};game.keepPlayerInZone(1.f/60.f);
        ledges&=glm::length(game.player.position-game.level.arenas[room].respawn)<.01f;
    }
    CHECK(ledges,"live recovery leaves every lower descent landing alone and recovers actual falls underneath it");
    game.saySuccessor(0);game.labDialogueT=0;game.retryArena();game.saySuccessor(0);
    CHECK(game.labDialogueT==0 && (game.successorHeard&1u),"normal checkpoint retries do not replay a Successor speech already heard in this run");
    game.enterArena(4);game.updateLab(.01f);
    CHECK(!game.level.doors[game.level.arenas[4].exitDoor].locked && game.enemies.empty(),"the quiet Conservatory opens its descent without inventing a combat wave");
    for(int room:{2,6,7,8}){
        game.enterArena(room);game.director.phase=WaveDirector::Phase::ACTIVE;game.director.queue.clear();game.enemies.clear();
        bool found=false;for(auto& it:game.interactables)if(glm::length(it.position-lab::contactPosition(room,-1))<.01f && it.type!=InteractableType::READABLE){it.onInteract();found=true;break;}
        CHECK(found && (game.labRoutes&(1u<<room)) && game.director.goalDone,"the actual lab control completes its external objective after the threats are cleared");
        game.updateLab(3);game.level.updateMovers(game.moverClock);
        if(room>=7){for(const auto& contact:game.level.story.labBridges)if(contact.room==room){auto& m=game.level.movers[contact.mover];CHECK(m.drive==(contact.side<0?1.f:0.f),"the selected contact moves only its corresponding physical bridge");}}
        game.retryArena();CHECK(game.labRoutes&(1u<<room),"checkpoint retry retains the selected assessment contact");
    }
    game.enterArena(7);game.director.approach(8);game.render();float realm=0;
    auto realmUniform=glGetUniformLocation(game.skyboxShader.id,"uLabRealm");glGetUniformfv(game.skyboxShader.id,realmUniform,&realm);
    CHECK(realm==1.f,"clearing Celestial Observation keeps its nebula while the next encounter waits");
    game.player.position=game.level.arenas[8].playerStart;game.prevCamPos=game.player.camera.position=game.player.position+glm::vec3{0,1.7f,0};game.render();
    glGetUniformfv(game.skyboxShader.id,realmUniform,&realm);
    CHECK(realm==2.f,"physically crossing the hospital doorway changes the rendered exterior to the trench");
    // Resolve each new enemy's real event path against the player, then use
    // actual gun hit/poise processing to interrupt its special motion.
    game.enterArena(1);
    for(auto type:{EnemyType::PATIENT,EnemyType::GRAFTED,EnemyType::SUSPENDED,EnemyType::PHASED}){
        game.enemies.clear();game.player.position=lab::centre(1)+glm::vec3{0,0,15};game.player.velocity={0,0,0};
        game.player.camera.position=game.player.position+glm::vec3{0,1.7f,0};game.styleSystem.health=100;game.invincFrames=0;game.dashMomentumTimer=0;
        game.spawnEnemy(type,game.player.position+glm::vec3{0,0,-3});auto& enemy=game.enemies.back();enemy.state=EnemyState::ACTIVE;enemy.spawnTimer=0;
        if(type==EnemyType::GRAFTED){enemy.attack=AttackKind::LAB_PRESS;enemy.telegraphTimer=.005f;}
        else if(type==EnemyType::SUSPENDED){enemy.labBeam=.5f;enemy.labDir=glm::normalize(game.player.camera.position-(enemy.position+glm::vec3{0,enemy.height()*.85f,0}));game.beamTickCd=0;}
        else{enemy.dashTimer=.3f;enemy.labDir={0,0,1};enemy.position=game.player.position+glm::vec3{0,0,-.6f};}
        game.updateEnemies(1.f/60);
        CHECK(game.styleSystem.health<100,"each new lab enemy resolves actual beam, lunge or press damage through the live combat system");
        enemy.labBeam=enemy.labCross=enemy.dashTimer=.4f;GameplayState::HitPush push;push.dir={1,0,0};push.push=0;push.poise=10;
        game.hurtEnemy(enemy,20,enemy.position+glm::vec3{0,1,0},0,0,StyleSource::REVOLVER,false,false,&push);
        CHECK(enemy.stunned() && enemy.labBeam==0 && enemy.labCross==0 && enemy.dashTimer==0,"live gun poise interrupts lab special attacks and removes their pending motion");
    }
    game.enterArena(1);game.enemies.clear();game.spawnEnemy(EnemyType::PATIENT,game.player.position+glm::vec3{0,0,-3});
    auto& rusher=game.enemies.back();rusher.state=EnemyState::ACTIVE;rusher.spawnTimer=0;rusher.attack=AttackKind::LAB_LUNGE;rusher.telegraphTimer=.2f;
    game.punchCooldown=0;game.player.camera.yaw=-90;game.player.camera.pitch=0;game.punch(-1);
    CHECK(rusher.staggered() && rusher.dashTimer==0 && rusher.armorMult()>1,"the real fist input parries a Patient's white brace and creates a damage opening");
    game.enterArena(1);game.enemies.clear();game.spawnEnemy(EnemyType::GRAFTED,game.player.position+glm::vec3{0,0,-4});
    game.enemies.back().state=EnemyState::ACTIVE;game.enemies.back().spawnTimer=0;
    auto* orb=game.projSystem.fire(game.player.camera.position+glm::vec3{0,0,-1.2f},{0,0,4},20,false,{1,1,1});
    orb->heavy=true;orb->parryDamage=230;orb->owner=game.enemies.back().uid;
    game.player.camera.yaw=-90;game.player.camera.pitch=0;game.punchCooldown=0;game.punch(-1);
    Uint8 still[SDL_NUM_SCANCODES]={};for(int f=0;f<12;++f)game.physicsTick(1.f/60.f,still,false);
    CHECK(game.enemies.back().health<game.enemies.back().maxHealth && game.enemies.back().stunned() && game.enemies.back().finishable(false),"returning a heavy orb through the real projectile path breaks Grafted poise and sets up a finisher");
    game.enterArena(5);game.enemies.clear();game.spawnEnemy(EnemyType::SUSPENDED,lab::centre(5)+glm::vec3{-21,8,-12});
    CHECK(game.enemies.back().hoverY>5 && game.enemies.back().hoverY<12,"lab flyers initialize their hover height from the ward floor rather than the abyss basin");
    auto& flyer=game.enemies.back();flyer.state=EnemyState::ACTIVE;flyer.spawnTimer=0;flyer.mayAttack=false;
    for(int f=0;f<600;++f){flyer.floorY=game.level.enemyFloor(5,flyer.position,true,-300);EnemyWorld air;air.bounds=game.level.arenas[5].bounds;air.playerFeet=lab::centre(5);air.playerEye=air.playerFeet+glm::vec3{0,1.7f,0};flyer.update(1.f/60.f,air);}
    CHECK(flyer.position.y<lab::centre(5).y+12 && flyer.position.y>lab::centre(5).y+5,"a Suspended subject remains within combat height over Load Testing's pit");
    game.enterArena(1);game.enemies.clear();
    for(auto type:{EnemyType::SUSPENDED,EnemyType::PHASED,EnemyType::PATIENT,EnemyType::HUSK}){
        game.spawnEnemy(type,game.player.position+glm::vec3{0,0,-4});auto& e=game.enemies.back();e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.attackTimer=100;
    }
    game.enemies[0].labBeam=.4f;game.enemies[1].labCross=.2f;game.enemies[2].dashTimer=.3f;
    game.assignAttackers();CHECK(!game.enemies[3].mayAttack,"ongoing lab beams, crossings and rushes reserve their attack slots until they finish");
    game.enemies[0].labBeam=0;game.enemies[0].recoverTimer=1;game.assignAttackers();
    CHECK(!game.enemies[0].mayAttack && game.enemies[3].mayAttack,"recovering enemies yield their slots so another threat can keep the fight moving");
    game.enterArena(9);game.enemies.clear();game.spawnEnemy(EnemyType::SUCCESSOR,game.level.arenas[9].bossSpawn);
    auto& boss=game.enemies.back();boss.state=EnemyState::ACTIVE;boss.spawnTimer=0;boss.suPhase=2;boss.suNodes=2;
    boss.suAnchor[0]=game.player.position+glm::vec3{0,1.7f,-8};boss.suAnchor[1]=boss.suAnchor[0]+glm::vec3{8,0,0};
    game.player.camera.yaw=-90;game.player.camera.pitch=0;game.prevCamPos=game.player.camera.position=game.player.position+glm::vec3{0,1.7f,0};
    game.activeWeapon=0;game.fireWeapon(0);
    CHECK(boss.staggered() && boss.suNodes==0,"a real revolver input cuts the visible Suture node and interrupts its pending attack");
    boss.staggerTimer=0;boss.attack=AttackKind::SU_THRUST;boss.telegraphTimer=.2f;
    boss.position=game.player.position+glm::vec3{0,0,-6.5f};boss.yaw=0;game.styleSystem.health=60;game.punchCooldown=0;
    game.punch(-1);CHECK(boss.staggered()&&game.styleSystem.health>60&&boss.suStep==0,"a real fist parry interrupts the Successor and returns bounded health");
    boss.health=boss.maxHealth*.6f;boss.suPhase=2;boss.suPause=boss.staggerTimer=boss.suBurst=boss.suCross=boss.suAir=boss.suOpen=0;
    for(float height:{0.f,1.3f}){
        game.player.position=boss.position+glm::vec3{0,height,6};game.styleSystem.health=100;game.invincFrames=0;
        boss.attack=AttackKind::SU_REAP;boss.telegraphTimer=.005f;boss.suSince=10;
        game.updateEnemies(1.f/60.f);
        CHECK(height==0?game.styleSystem.health<100:game.styleSystem.health==100,"the live low sweep hurts grounded players and spares a correctly timed jump");
    }
    game.player.position=boss.position+glm::vec3{0,2,6};game.styleSystem.health=100;game.invincFrames=0;
    boss.suAim=game.player.position-glm::vec3{0,2,0};boss.suFrom=boss.position;boss.suAir=.005f;boss.attack=AttackKind::NONE;boss.telegraphTimer=0;
    game.updateEnemies(1.f/60.f);game.updateSuccessor(1.f/60.f);
    CHECK(game.styleSystem.health<100 && !game.successorHazards.marks.empty(),"the live aerial breaker creates its damaging landing hazard above jump height");
    game.successorHazards.clear();
    // Inspect the dedicated boss rig from both sides and through its new moves.
    auto centre=lab::centre(9);game.fx.clear();game.ui.damageVignette=0;game.muzzleFlashTimer=0;game.labDialogueT=0;game.styleSystem.health=100;game.ui.update(5.f,game.styleSystem);
    boss.staggerTimer=0;boss.suPause=boss.suOpen=boss.suBurst=boss.suCross=boss.suAir=0;boss.suNodes=0;boss.suSince=10;
    boss.position=boss.prevPosition=centre+glm::vec3{0,0,-18};boss.yaw=boss.prevYaw=0;boss.suGait=0;boss.suBodyLive=false;
    auto capture=[&](const char* path,glm::vec3 camera,float yaw,float pitch){
        game.player.position=camera-glm::vec3{0,1.7f,0};game.prevCamPos=game.player.camera.position=camera;
        game.player.camera.yaw=yaw;game.player.camera.pitch=pitch;
        game.render();screenshot(path);CHECK(glGetError()==GL_NO_ERROR,"Successor rig and exposed arena render without OpenGL errors");
    };
    boss.attack=AttackKind::NONE;boss.telegraphTimer=0;
    for(int f=0;f<60;++f)rig::tickSuccessorBody(boss,1.f/60);
    capture("build/successor-front.bmp",centre+glm::vec3{3.6f,1.8f,-12},-121,4);
    capture("build/successor-spine.bmp",centre+glm::vec3{-3.6f,2,-24},59,4);
    struct PoseShot{AttackKind kind;const char* path;};
    for(auto pose:{PoseShot{AttackKind::SU_INCISION,"build/successor-cut.bmp"},PoseShot{AttackKind::SU_THRUST,"build/successor-thrust.bmp"},
                  PoseShot{AttackKind::SU_REAP,"build/successor-reap.bmp"},PoseShot{AttackKind::SU_BREAKER,"build/successor-breaker.bmp"}}){
        boss.attack=pose.kind;boss.telegraphDuration=.8f;boss.telegraphTimer=.28f;boss.suAim=centre+glm::vec3{4,0,-6};
        for(int f=0;f<40;++f)rig::tickSuccessorBody(boss,1.f/60);
        capture(pose.path,centre+glm::vec3{3.6f,1.8f,-12},-121,4);
    }
    boss.attack=AttackKind::NONE;boss.telegraphTimer=0;boss.health=boss.maxHealth*.25f;boss.suPhase=3;
    capture("build/successor-arena.bmp",centre+glm::vec3{31,5,28},-112,17);
    float assessment=0;glGetUniformfv(game.skyboxShader.id,glGetUniformLocation(game.skyboxShader.id,"uAssessment"),&assessment);
    CHECK(assessment>.7f,"the final arena's lensed exterior intensifies with the live boss phase");
    boss.alive=false;boss.health=0;game.onEnemyKilled(boss,StyleSource::REVOLVER);
    CHECK(game.storyBossDown && game.successorHazards.marks.empty(),"Successor death cancels hazards and records the release of the chapter exit");
    game.retryArena();bool successorAlive=false;for(const auto& e:game.enemies)successorAlive|=e.alive&&e.type==EnemyType::SUCCESSOR;
    CHECK(game.storyBossDown && !successorAlive && !game.level.doors[game.level.arenas[9].exitDoor].locked,"retry after victory preserves the defeated boss and open threshold");
    game.director.phase=WaveDirector::Phase::VICTORY;game.player.position=(game.level.story.end.min+game.level.story.end.max)*.5f;game.updateLab(.1f);
    CHECK(game.storyEndT>=0,"walking into the fractured garden starts the actual Chapter 3 ending");
    g_storyChapter=1;g_startArena=g_startWave=0;g_practice=g_godMode=false;
}

// Chapter 4 through the live game: it loads, the meteors fall and land, a near
// one shakes the ground late, the long view survives a retry, and it renders.
static void checkPlainRuntime(AudioSystem& audio) {
    g_storyChapter=4;g_startArena=g_startWave=0;g_practice=true;g_godMode=false;g_devOverlay.clear();g_bossRun=-1;
    auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
    CHECK(game.plain() && game.level.arenas.size()==1 && game.storyOwns(0) && game.storyOwns(3) && game.storyGrapple,
          "--story4 starts THE BLOOD MOON PLAIN with every gun and the grapple");
    CHECK(game.meteors.live()>=4,"the sky is already full of meteors on arrival");
    // The reveal: it plays, it has the controls, the Gleaner rises, turns, opens its eye and screams
    CHECK(game.plainReveal.active() && game.storyLocked() && game.gleaner.rise<.01f,"the arrival plays the reveal with the Gleaner lying curled on the horizon");
    game.plainReveal.time=PlainReveal::RISE0+2.f;game.updateStory(1.f/60);game.render();screenshot("build/plain-reveal-rising.bmp");
    game.plainReveal.time=PlainReveal::SCREAM-.1f;bool screamed=false;
    for(int f=0;f<60*7 && game.plainReveal.active();++f){game.updateStory(1.f/60);screamed|=game.gleaner.screaming();}
    game.render();screenshot("build/plain-reveal-end.bmp");
    CHECK(screamed && !game.plainReveal.active() && !game.storyLocked() && game.gleaner.rise>.99f && game.gleaner.eyeOpen>.99f,
          "the Gleaner screams, and the reveal hands control back with it standing, eye open");
    CHECK(game.wakeSeen && game.plainRevealDone && std::abs(game.player.camera.yaw+90.f)<1.f,"the reveal is remembered and leaves you facing it");
    game.retryArena();game.updateStory(1.f/60);
    CHECK(!game.plainReveal.active() && game.gleaner.rise>.99f,"a retry does not replay the reveal");
    int landed=0,slams=0;
    for(int f=0;f<60*12;++f){game.updateStory(1.f/60);landed+=(int)game.meteorHits.size();for(const auto& e:game.gleaner.events)slams+=e.kind==GleanerEvent::SLAM;}
    CHECK(slams>=1,"the Gleaner crawls on the plain after the reveal, its fists landing");
    CHECK(landed>=3 && game.player.camera.farPlane==plain::FAR_PLANE,"meteors land in the live game and the camera sees across the plain");
    game.meteorImpact({game.player.position+glm::vec3{0,0,-150},9.f});
    CHECK(!game.plainQuakes.empty() && game.plainQuakes.back().t>.4f,"a near impact queues its shake to arrive with the sound");
    for(int f=0;f<60;++f)game.updateStory(1.f/60);
    bool due=true;for(const auto& q:game.plainQuakes)due&=q.t>0.f;   // (the Gleaner's fists keep queuing new ones)
    CHECK(due,"the queued shake lands and clears");
    while(glGetError()!=GL_NO_ERROR){}
    game.render();screenshot("build/plain-runtime.bmp");
    CHECK(glGetError()==GL_NO_ERROR,"the plain, its sky and its meteors render without OpenGL errors");
    game.retryArena();game.updateStory(1.f/60);
    CHECK(game.plain() && game.player.camera.farPlane==plain::FAR_PLANE && glm::length(game.player.position-plain::start())<.5f,
          "a retry returns to the arrival with the long view intact");
    g_practice=false;g_storyChapter=1;
}

// The Gleaner fight through the live game: it goes down when you come close, its updraft carries you
// onto its back with real physics, the Tender wakes and fights, and dies to real shots.
static void checkGleanerFightRuntime(AudioSystem& audio) {
    g_storyChapter=4;g_startArena=g_startWave=0;g_practice=true;g_godMode=true;g_bossRun=-1;
    g_devOverlay="plainclose";
    auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
    g_devOverlay.clear();
    static const Uint8 noKeys[SDL_NUM_SCANCODES]={};
    auto tick=[&](float s){for(int f=0;f<(int)(s*60);++f){game.physicsTick(1.f/60,noKeys,false);}};
    CHECK(game.plain() && !game.plainReveal.active() && game.gleanerPhase==GameplayState::GleanerPhase::APPROACH,"the fight begins on the approach");
    tick(1.f);
    CHECK(game.gleanerPhase==GameplayState::GleanerPhase::KNEELING,"close to it, the Gleaner goes down");
    tick(9.f);
    CHECK(game.gleanerPhase==GameplayState::GleanerPhase::CLIMB && game.deckWallsFrom>=0 && game.gleaner.kneel>=1.f,"it kneels and its back becomes a battlefield");
    std::printf("      deck at %.0f m, updraft at %.0f %.0f\n",game.deckY,game.updraftBase.x,game.updraftBase.z);
    // Into the updraft, and let it carry us (real physics, real boosters)
    game.player.position=game.updraftBase+glm::vec3{0,0.1f,0};game.player.velocity={0,0,0};
    float top=0;for(int f=0;f<60*20 && !game.onGleanerDeck();++f){game.physicsTick(1.f/60,noKeys,false);top=std::max(top,game.player.position.y);}
    std::printf("      updraft: rose to %.0f m, now at %.1f %.1f %.1f\n",top,game.player.position.x,game.player.position.y,game.player.position.z);
    CHECK(game.onGleanerDeck(),"the updraft carries you up and over the parapet onto its back");
    for(int f=0;f<60*5 && game.gleanerPhase!=GameplayState::GleanerPhase::TENDER;++f)game.physicsTick(1.f/60,noKeys,false);
    CHECK(game.gleanerPhase==GameplayState::GleanerPhase::TENDER && game.tender.alive() && game.player.onGround,"you land on its back and the Tender climbs out of the hatch");
    tick(3.f);
    glm::vec3 was=game.tender.pos;tick(6.f);
    float moved=glm::length(game.tender.pos-was);
    std::printf("      tender: state %d, moved %.1f m in 3 s, %.0f m from you\n",(int)game.tender.state,moved,glm::length(game.tender.pos-game.player.position));
    CHECK(moved>3.f || glm::length(game.tender.pos-game.player.position)<14.f,"the Tender comes for you across the roof");
    {   // face it and look: the render camera follows the physics camera
        glm::vec3 d=glm::normalize(game.tender.base()+glm::vec3{0,5,0}-game.player.camera.position);
        game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));game.player.camera.pitch=glm::degrees(std::asin(d.y));
        game.prevCamPos=game.player.camera.position;game.render();screenshot("build/gleaner-tender.bmp");
    }
    // Shoot it to death with the revolver, aimed at its body
    int shots=0;
    while(game.tender.fighting() && shots<400){
        Tender::Box b=game.tender.bodyBox();glm::vec3 c=(b.lo+b.hi)*.5f;
        glm::vec3 d=glm::normalize(c-game.player.camera.position);
        game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));game.player.camera.pitch=glm::degrees(std::asin(d.y));
        game.activeWeapon=0;game.weapons[0].ammo=8;game.weapons[0].cooldown=0;game.fireWeapon(0);++shots;
        tick(.05f);
    }
    std::printf("      tender down after %d revolver shots\n",shots);
    CHECK(!game.tender.fighting() && game.gleanerPhase==GameplayState::GleanerPhase::SIEVE && game.deckFrontOpen,"real shots kill the Tender: the roof's front opens");
    game.retryArena();
    CHECK(game.gleanerPhase==GameplayState::GleanerPhase::SIEVE && game.onSaddle() && game.deckFrontOpen,"a retry after it puts you on its shoulders, the Sieve to come");
    // (from the roof, as you would: back up there for the walk off its front)
    {
        Uint8 keys[SDL_NUM_SCANCODES]={};
        auto walkTo=[&](glm::vec3 goal,int frames,auto until){
            for(int f=0;f<frames;++f){
                if(until())return true;
                glm::vec3 d=goal-game.player.position;d.y=0;
                std::fill(std::begin(keys),std::end(keys),0);
                if(glm::length(d)>.3f){game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));keys[SDL_SCANCODE_W]=1;}
                game.physicsTick(1.f/60,keys,false);
            }
            return until();
        };
        game.player.position=game.deckCheckpoint();game.player.velocity={0,0,0};game.sieveForged=false;game.sieve=Sieve{};
        for(int f=0;f<30;++f)game.physicsTick(1.f/60,noKeys,false);
        float cx=(game.deckLo.x+game.deckHi.x)*.5f;
        bool down=walkTo({cx,game.deckY,game.deckHi.y+8.f},60*30,[&]{return game.onSaddle()&&game.player.onGround;});
        std::printf("      off the roof's front: saddle at %.0f m (%.0f below the roof), now at %.1f %.1f %.1f\n",game.saddleY,game.deckY-game.saddleY,game.player.position.x,game.player.position.y,game.player.position.z);
        CHECK(down,"walk off the roof's front and you drop onto the saddle over its shoulders");
        tick(.1f);
        CHECK(game.sieveForged,"landing there calls a meteor down onto the saddle");
        tick(MeteorRain::CALLED_FALL+3.5f);
        CHECK(game.sieve.fighting() && glm::length(glm::vec2{game.sieve.pos.x-game.saddleCheckpoint().x,game.sieve.pos.z-game.saddleCheckpoint().z})<90.f,"the Sieve unfolds out of its crater on the saddle and fights");
        {
            glm::vec3 d=glm::normalize(game.sieve.heart()-game.player.camera.position);
            game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));game.player.camera.pitch=glm::degrees(std::asin(d.y));
            game.prevCamPos=game.player.camera.position;game.render();screenshot("build/gleaner-sieve.bmp");
        }
        // Kill it with real shots at its heart, walking in under it to get them through the grate
        int frames=0,sieveShots=0,heartHits=0;float hp0=game.sieve.hp;
        while(game.sieve.fighting()&&frames<60*120){
            glm::vec3 goal=game.sieve.pos;glm::vec3 d=goal-game.player.position;d.y=0;
            std::fill(std::begin(keys),std::end(keys),0);
            if(glm::length(d)>4.f){game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));keys[SDL_SCANCODE_W]=1;}
            game.physicsTick(1.f/60,keys,false);++frames;
            if(frames%3==0){
                glm::vec3 a=glm::normalize(game.sieve.heart()-game.player.camera.position);
                game.player.camera.yaw=glm::degrees(std::atan2(a.z,a.x));game.player.camera.pitch=glm::degrees(std::asin(a.y));
                float hpBefore=game.sieve.hp;
                game.activeWeapon=0;game.weapons[0].ammo=8;game.weapons[0].cooldown=0;game.fireWeapon(0);++sieveShots;
                if(hpBefore-game.sieve.hp>100.f)++heartHits;
            }
        }
        tick(.1f);   // (its death, raised by the shot, is read on the next tick)
        std::printf("      sieve: down after %d shots (%d on its heart) in %.1f s, from %.0f hp (phase %d, checkpoint %d, state %d)\n",sieveShots,heartHits,frames/60.f,hp0,(int)game.gleanerPhase,game.storyCheckpoint,(int)game.sieve.state);
        CHECK(!game.sieve.fighting() && heartHits>0 && game.gleanerPhase==GameplayState::GleanerPhase::CALLER && game.storyCheckpoint>=3,"real shots through its grate kill the Sieve; the Gleaner rears its head");
        tick(12.f);
        CHECK(game.deckCrown && game.gleaner.headHold>=1.f,"its head comes up and holds: a bridge along its neck to its crown");
        game.retryArena();
        CHECK(game.gleanerPhase==GameplayState::GleanerPhase::CALLER && game.deckCrown && game.onSaddle(),"a retry after the Sieve starts at the saddle's front, its head up");
        glm::vec3 crown{(game.crownLo.x+game.crownHi.x)*.5f,game.saddleY,(game.crownLo.y+game.crownHi.y)*.5f};
        bool across=walkTo(crown,60*30,[&]{return game.onCrown()&&glm::length(glm::vec2{game.player.position.x-crown.x,game.player.position.z-crown.z})<2.f;});
        std::printf("      across its neck: now at %.1f %.1f %.1f (crown at %.0f m)\n",game.player.position.x,game.player.position.y,game.player.position.z,game.saddleY);
        CHECK(across && game.player.onGround,"walk along its neck onto the crown of its skull");
        CHECK(game.caller.alive(),"the Caller comes down out of the sky");
        tick(Caller::ARRIVE_TIME+.5f);
        CHECK(game.caller.fighting(),"and circles over its crown");
        {
            glm::vec3 d=glm::normalize(game.caller.pos-game.player.camera.position);
            game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));game.player.camera.pitch=glm::degrees(std::asin(d.y));
            game.prevCamPos=game.player.camera.position;game.render();screenshot("build/gleaner-caller.bmp");
        }
        // Punch its dives as they arrive (the real punch), and shoot its lamp: from under it, or while it lies on its side
        int parries=0,lampHits=0;frames=0;
        while(game.caller.fighting()&&frames<60*150){
            bool punch=false;
            if(game.caller.parryable()){
                glm::vec3 to=game.caller.pos-game.player.camera.position;
                if(glm::length(to)<Caller::PARRY_REACH-1.f){
                    to=glm::normalize(to);game.player.camera.yaw=glm::degrees(std::atan2(to.z,to.x));game.player.camera.pitch=glm::degrees(std::asin(to.y));
                    punch=true;
                }
            }
            bool was=game.caller.stunned();
            game.punchCooldown=0.f;
            game.physicsTick(1.f/60,noKeys,punch);++frames;
            if(!was&&game.caller.stunned())++parries;
            if(frames%6==0&&(parries>0||frames>60*40)){   // (only once a dive has been met: the parry is what this checks first)
                glm::vec3 a=glm::normalize(game.caller.lamp()-game.player.camera.position);
                if(game.caller.lampSeen(a,false)){
                    game.player.camera.yaw=glm::degrees(std::atan2(a.z,a.x));game.player.camera.pitch=glm::degrees(std::asin(a.y));
                    float hpBefore=game.caller.hp;
                    game.activeWeapon=0;game.weapons[0].ammo=8;game.weapons[0].cooldown=0;game.fireWeapon(0);
                    if(hpBefore-game.caller.hp>50.f)++lampHits;
                }
            }
        }
        tick(.1f);
        std::printf("      caller: down after %.1f s, %d parries, %d lamp hits (phase %d, checkpoint %d)\n",frames/60.f,parries,lampHits,(int)game.gleanerPhase,game.storyCheckpoint);
        CHECK(parries>0,"its dive is parried by a real punch: it goes down on its side");
        CHECK(!game.caller.fighting() && lampHits>0 && game.gleanerPhase==GameplayState::GleanerPhase::BROOD && game.storyCheckpoint>=4,"real shots at its lamp kill the Caller: the brood is dead");
        game.retryArena();
        CHECK(game.gleanerPhase==GameplayState::GleanerPhase::BROOD && game.onGleanerBack(),"a retry after the brood keeps you on the Gleaner's back");
    }
    g_godMode=false;g_practice=false;g_storyChapter=1;
}

// The SPLINTER through the live game: a revolver round that hits a hanging
// splinter goes on into an enemy's head; two splinters chain and hit harder.
static void checkSplinterRuntime(AudioSystem& audio) {
    g_storyChapter=3;g_startArena=g_startWave=0;g_practice=true;g_godMode=false;g_devOverlay.clear();g_bossRun=-1;
    g_devSplinter=true;
    auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::STORY);auto& game=*live;
    game.labIntro.seen=true;game.storySkipKey=true;game.updateLab(.7f);game.storySkipKey=false;
    game.enterArena(1);game.director.hold=true;
    auto shootAt=[&](glm::vec3 p){
        glm::vec3 d=glm::normalize(p-game.player.camera.position);
        game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));game.player.camera.pitch=glm::degrees(std::asin(d.y));
        game.activeWeapon=0;game.weapons[0].ammo=8;game.weapons[0].cooldown=0;game.fireWeapon(0);
    };
    auto setup=[&](){
        game.enemies.clear();game.splinters.reset();
        game.player.position=lab::centre(1)+glm::vec3{0,0,15};game.player.velocity={0,0,0};
        game.player.camera.position=game.player.position+glm::vec3{0,1.7f,0};
        // An enemy off to the side, where the player is not looking
        game.spawnEnemy(EnemyType::GRAFTED,game.player.position+glm::vec3{9,0,-6});
        auto& e=game.enemies.back();e.state=EnemyState::ACTIVE;e.spawnTimer=0;e.hasShown=false;
        return game.enemies.size()-1;
    };
    {
        size_t ei=setup();float before=game.enemies[ei].health;
        game.player.camera.yaw=-90;game.player.camera.pitch=0;
        CHECK(game.splinterOwned(),"--splinter owns the SPLINTER in Story");
        game.pendingGrenade=true;game.shootInput();
        CHECK(game.splinters.live()==1 && game.splinters.charges==SplinterSystem::CHARGES-1,"the grenade button throws a splinter in Story");
        for(int k=0;k<40 && !SplinterSystem::glinting(game.splinters.pool[0]);++k)game.tickSplinters(1.f/60);
        CHECK(SplinterSystem::glinting(game.splinters.pool[0]),"the thrown splinter reaches its glint in the live arena");
        game.render();screenshot("build/splinter-glint.bmp");
        shootAt(game.splinters.pool[0].pos);
        float dealt=before-game.enemies[ei].health;
        std::printf("      splinter redirect: %.1f damage (revolver 42)\n",dealt);
        CHECK(game.splinters.live()==0 && dealt>42.f*1.5f,"shooting the splinter sends the round into the enemy's weak point, harder than the gun hits");
    }
    {
        size_t ei=setup();float before=game.enemies[ei].health;
        auto& a=game.splinters.pool[0];auto& b=game.splinters.pool[1];
        a.alive=b.alive=true;a.age=b.age=.3f;a.vel=b.vel={0,0,0};
        a.pos=a.prev=game.player.camera.position+glm::vec3{-3,1,-6};b.pos=b.prev=game.player.camera.position+glm::vec3{2,2,-8};
        shootAt(a.pos);
        float dealt=before-game.enemies[ei].health;
        std::printf("      splinter chain: %.1f damage\n",dealt);
        CHECK(game.splinters.live()==0 && dealt>42.f*2.f,"a round chains through both splinters before it lands");
    }
    g_devSplinter=false;g_practice=false;
}

int main() {
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window = SDL_CreateWindow("Dev regression", 0, 0, SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) { std::fprintf(stderr, "SDL: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    auto context = SDL_GL_CreateContext(window);
    if (!context) { std::fprintf(stderr, "GL: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental = GL_TRUE; glewInit(); glGetError();
#endif
    glViewport(0, 0, SCREEN_W, SCREEN_H);
    balanceInit();
    g_devNoMouse = true;
    {
        AudioSystem audio;
        MenuState menu(SCREEN_W, SCREEN_H);
        StartOptions selected;
        GameMode mode = GameMode::ARENA;
        int starts = 0;
        menu.onStart = [&](GameMode m, StartOptions o) { mode = m; selected = o; ++starts; };
        press(menu, SDLK_F2);
        CHECK(menu.page == MenuState::DEV, "F2 opens the shared dev menu");
        press(menu, SDLK_RIGHT);
        CHECK(menu.devRows[menu.devSel - MenuState::DEV_OPTIONS].mode == GameMode::FAST, "right moves from ARENA to FAST");
        press(menu, SDLK_RIGHT);
        CHECK(menu.devRows[menu.devSel - MenuState::DEV_OPTIONS].mode == GameMode::STORY, "right moves from FAST to STORY");
        menu.render(); screenshot("build/dev-story-menu.bmp");

        std::vector<StartOptions> checkpoints;
        bool keyboard = true;
        menu.devWave = 5; // An ARENA wave selection must never leak into Story.
        for (int i = 0; i < (int)menu.devRows.size(); ++i) {
            const auto& row = menu.devRows[i];
            if (row.mode != GameMode::STORY) continue;
            int target = i + MenuState::DEV_OPTIONS;
            for (int n = 0; menu.devSel != target && n < 100; ++n) press(menu, SDLK_DOWN);
            int before = starts;
            press(menu, SDLK_RETURN);
            keyboard &= starts == before + 1 && mode == GameMode::STORY && selected.arena == row.arena &&
                        selected.storyChapter == row.chapter && selected.wave == 0 && selected.practice && selected.god;
            checkpoints.push_back(selected);
        }
        CHECK(keyboard && checkpoints.size() == 27, "all 27 checkpoints launch the correct chapter and room with practice/god flags");
        CHECK(menu.devScroll[2] > 0, "keyboard navigation reveals the later Chapter 2 checkpoints");
        menu.render(); screenshot("build/dev-story-menu-scrolled.bmp");
        // The last checkpoint is at the bottom of the visible Story column.
        SDL_Event click{}; click.type = SDL_MOUSEBUTTONDOWN; click.button.button = SDL_BUTTON_LEFT;
        click.button.x = 1000; click.button.y = 232 + 10 * 34;
        int before = starts; menu.handleEvent(click);
        CHECK(starts == before + 1 && selected.storyChapter == 3 && selected.arena == 9,
              "mouse activation uses the scrolled checkpoint rather than a hidden row");
        menu.devSel = 0; press(menu, SDLK_RETURN);
        menu.devSel = MenuState::DEV_OPTIONS + (int)menu.devRows.size() - 1; press(menu, SDLK_RETURN);
        CHECK(!selected.god && selected.practice, "god mode can be disabled independently of practice");

        const int expectedGuns1[] = {0, 0, 1, 5, 5, 5, 7, 7, 7};
        for (const auto& o : checkpoints) {
            g_storyChapter = o.storyChapter; g_startArena = o.arena; g_startWave = o.wave;
            g_practice = o.practice; g_godMode = o.god;
            // Use the same deferred handoff as App: finish the menu event,
            // construct gameplay, then destroy the old menu before drawing.
            std::unique_ptr<GameState> live=std::make_unique<MenuState>(SCREEN_W,SCREEN_H);
            auto* chooser=static_cast<MenuState*>(live.get());chooser->openDev();
            bool requested=false;
            chooser->onStart=[&](GameMode m,StartOptions start) {
                requested=m==GameMode::STORY&&start.storyChapter==o.storyChapter&&start.arena==o.arena;
                g_storyChapter=start.storyChapter;g_startArena=start.arena;g_startWave=start.wave;
                g_practice=start.practice;g_godMode=start.god;
            };
            for(int row=0;row<(int)chooser->devRows.size();++row) {
                const auto& entry=chooser->devRows[row];
                if(entry.mode==GameMode::STORY&&entry.chapter==o.storyChapter&&entry.arena==o.arena)
                    chooser->devSel=MenuState::DEV_OPTIONS+row;
            }
            press(*chooser,SDLK_RETURN);
            CHECK(requested,"real dev-menu input requests the selected Story checkpoint");
            auto* gameplay=new GameplayState(audio,nullptr,GameMode::STORY);
            live.reset(gameplay);
            auto& game=*gameplay;
            int expectedGuns = o.storyChapter == 1 ? expectedGuns1[o.arena] : o.storyChapter==3?15:o.arena > 4 ? 15 : 7;
            bool expectedGrapple = o.storyChapter >= 2 || o.arena > 7;
            bool introStart=o.storyChapter==3 && o.arena==0;
            auto expectedStart=introStart?glm::vec3{0,0,27}:game.level.arenas[o.arena].playerStart;
            char label[128];
            std::snprintf(label, sizeof(label), "Story %d.%d starts with its checkpoint, equipment and intended intro state", o.storyChapter, o.arena + 1);
            CHECK(game.level.story.chapter == o.storyChapter && game.director.arena == o.arena && game.storyCheckpoint == o.arena &&
                  glm::length(game.player.position - expectedStart) < .01f && game.storyLocked()==introStart &&
                  game.storyOwned == expectedGuns && game.storyGrapple == expectedGrapple && game.storyArmed == (expectedGuns != 0) && !game.ranked, label);
            if(introStart){
                CHECK(game.labIntro.active() && game.director.hold,"Admissions selected from the dev menu still plays the Chapter 3 cutscene");
                game.storySkipKey=false;game.updateLab(.5f);game.storySkipKey=true;game.updateLab(.7f);game.storySkipKey=false;
            }
            game.retryArena();
            CHECK(game.director.arena == o.arena && game.storyOwned == expectedGuns && !game.storyLocked(), "retry preserves the selected checkpoint and gear");
            // Launching a checkpoint must survive its first drawn frames too:
            // those load the chapter's nearby rigs, scenery and HUD paths.
            for (int frame=0;frame<3;++frame) { game.update(1.f/60.f);game.render(); }
            Uint8 idle[SDL_NUM_SCANCODES]={};
            for(int frame=0;frame<90;++frame){
                applyWater(game.player,game.level);
                game.player.update(1.f/60.f,idle,game.level.walls.data(),(int)game.level.walls.size(),false,&game.spatialGrid);
                game.keepPlayerInZone(1.f/60.f);
            }
            CHECK(!game.playerDead, "Story checkpoint survives its first gameplay and render frames");
            CHECK(game.bankedDamage+game.styleSystem.damageTaken==0,"checkpoint opening frames do not charge false fall damage");
            press(game,SDLK_TAB);
            CHECK(game.armoryOpen&&game.armoryW==game.activeWeapon,"Tab opens the armory on the current weapon at every Story checkpoint");
            press(game,SDLK_TAB);
            CHECK(!game.armoryOpen&&!game.paused,"Tab closes the Story armory back to gameplay");
            if (o.arena == game.level.story.bossSection) {
                for (bool queued : {false, true}) {
                    game.enterArena(o.arena);
                    game.director.startArena(o.arena);
                    game.handleDirectorEvents();
                    std::vector<SpawnRequest> spawns;
                    game.director.update(.1f, 0, game.player.position, spawns);
                    if (!queued) {
                        game.director.update(.1f, 0, game.player.position, spawns);
                        for (const auto& spawn : spawns) game.spawnEnemy(spawn.type, spawn.pos, spawn.hollow);
                    }
                    game.handleDirectorEvents();
                    press(game, SDLK_F5);
                    game.director.update(.1f, 0, game.player.position, spawns);
                    game.handleDirectorEvents();
                    CHECK(game.storyBossDown && !game.level.doors[game.level.arenas[o.arena].exitDoor].locked,
                          queued ? "F5 handles a queued Story boss and opens its exit" : "F5 handles a live Story boss and opens its exit");
                }
            }
            game.styleSystem.takeDamage(50.f);
            press(game, SDLK_F6);
            CHECK(game.styleSystem.health >= 99.f, "F6 refills health in Story practice");
        }
        // Capture both marker styles against the same cleanup encounter.
        g_storyChapter=1;g_startArena=8;g_startWave=0;g_practice=g_godMode=true;
        {
            GameSettings hints;
            GameplayState game(audio,&hints,GameMode::STORY);
            game.director.phase=WaveDirector::Phase::ACTIVE;game.director.queue.clear();game.banners.clear();
            game.enemies.clear();game.player.camera.yaw=-45.f;
            for(glm::vec3 at: {glm::vec3{74,18.8f,-2},glm::vec3{91,18.8f,-12},glm::vec3{100,18.8f,-27}}) {
                game.enemies.emplace_back(EnemyType::HUSK,at);
                auto& e=game.enemies.back();e.state=EnemyState::ACTIVE;e.spawnTimer=0;
            }
            for(int detail:{0,1}) {
                hints.enemyMarkers=detail;game.render();
                screenshot(detail?"build/enemy-markers-detailed.bmp":"build/enemy-markers-low.bmp");
            }
            game.enemies.clear();game.enemies.emplace_back(EnemyType::SORTER,game.level.arenas[8].bossSpawn);
            game.enemies.back().state=EnemyState::ACTIVE;game.enemies.back().spawnTimer=0;
            game.render();screenshot("build/boss-no-tracking-marker.bmp");
        }
        // Real gameplay restoration and the weapon path for the new machinery.
        for(int chapter:{1,2}) {
            g_storyChapter=chapter;g_startArena=chapter==1?8:6;g_practice=g_godMode=true;
            GameSettings detail;detail.ambientDetail=0;
            GameplayState game(audio,&detail,GameMode::STORY);
            game.director.phase=WaveDirector::Phase::ACTIVE;game.director.queue.clear();game.enemies.clear();game.banners.clear();
            const auto cover=game.level.story.bossCover.front();auto center=(cover.home.min+cover.home.max)*.5f;
            game.breakStoryCover(center,center,1);
            CHECK(game.level.walls[cover.wall].box.max.y<cover.home.min.y,"boss impact removes authored cover through real gameplay");
            if(chapter==1) {
                game.storyCollapseT=0;game.updateStoryFactory(2.3f);int dropped=0;
                for(size_t k=0;k<game.level.story.sortingTiles.size();++k)dropped+=game.level.walls[game.level.story.sortingTiles[k]].box.max.y<game.storyTileBoxes[k].max.y-1;
                CHECK(dropped==1,"the final phase drops only the first warned panel at 2.3 seconds");
                game.updateStoryFactory(3.4f);dropped=0;
                for(size_t k=0;k<game.level.story.sortingTiles.size();++k)dropped+=game.level.walls[game.level.story.sortingTiles[k]].box.max.y<game.storyTileBoxes[k].max.y-1;
                CHECK(dropped==4,"the remaining corner panels fail progressively");
                game.retryArena();bool restored=true;
                for(size_t k=0;k<game.level.story.sortingTiles.size();++k)restored&=glm::length(game.level.walls[game.level.story.sortingTiles[k]].box.max-game.storyTileBoxes[k].max)<.001f;
                CHECK(restored&&game.storyCollapseT<0,"retry restores all failed floor panels and clears their timer");
            } else {
                auto at=game.level.arenas[game.level.story.bossSection].bossSpawn;
                game.enemies.emplace_back(EnemyType::BALLAST,at);auto& boss=game.enemies.back();boss.spawnTimer=0;boss.state=EnemyState::ACTIVE;
                auto cableMid=at+glm::vec3{-1.5f,4,11};boss.ballCableAt=at+glm::vec3{-1.5f,4,21};boss.ballTether=.65f;boss.ballCableShow=.75f;
                game.player.camera.position=cableMid+glm::vec3{-8,0,0};game.player.camera.yaw=0;game.player.camera.pitch=0;
                game.fireWeapon(0);
                CHECK(boss.ballTether==0&&boss.ballOpen>0,"a real revolver shot cuts Ballast's cable and opens its drive");
                game.ballastHazards.addRip(at,at+glm::vec3{0,0,40});game.retryArena();
                CHECK(game.ballastHazards.fronts.empty(),"retry clears all active torn-road fronts");
            }
            CHECK(glm::length(game.level.walls[cover.wall].box.min-cover.home.min)<.001f,"retry restores shattered cover in both chapters");
        }
        // Real weapon raycasts must release all three locks and open the gate.
        g_startArena=4;g_startWave=0;g_storyChapter=2;g_practice=g_godMode=true;
        {
            GameSettings detailSettings;detailSettings.ambientDetail=3;
            GameplayState game(audio,&detailSettings,GameMode::STORY);
            game.director.startArena(4);std::vector<SpawnRequest> spawns;
            game.director.update(.1f,0,game.player.position,spawns);game.handleDirectorEvents();
            for(int id:game.level.story.turbineBrakes) {
                auto p=game.level.anchors[id].pos;
                game.player.camera.position=p+glm::vec3{p.x<0?4.f:-4.f,0,0};
                game.player.position=game.player.camera.position-glm::vec3{0,1.7f,0};
                auto d=glm::normalize(p-game.player.camera.position);
                game.player.camera.yaw=glm::degrees(std::atan2(d.z,d.x));game.player.camera.pitch=0;
                for(int shot=0;shot<4&&game.level.anchors[id].alive;++shot)game.fireWeapon(0);
            }
            CHECK(game.level.anchorsAlive(LevelData::ChainAnchor::BRAKE)==0,"real revolver shots release all turbine brakes");
            game.updateWasteland(1.f/60.f);game.director.update(.1f,0,game.player.position,spawns);game.handleDirectorEvents();
            CHECK(game.director.arena==5&&!game.level.doors[game.level.arenas[4].exitDoor].locked,"turbine repair opens the route through the real gameplay event path");
            game.storyTerminal=3;game.storyLog=-1;
            glClearColor(.04f,.04f,.05f,1);glClear(GL_COLOR_BUFFER_BIT);
            game.renderStoryOverlay();screenshot("build/story-record-wrapped.bmp");
            game.retryArena();
            CHECK(game.director.arena==4&&game.level.anchorsAlive(LevelData::ChainAnchor::BRAKE)==3,"retry restores the turbine objective and shot targets");
            game.director.startArena(4);game.director.update(.1f,0,game.player.position,spawns);press(game,SDLK_F5);
            CHECK(game.director.goalDone&&game.level.anchorsAlive(LevelData::ChainAnchor::BRAKE)==0,"practice F5 completes the repair and releases its locks");
            game.fx.setAmbientDetail(3);game.fx.spawnAmbientBurst(game.player.position,{.3f,.3f,.3f},9,1,3,0);
            bool hadAmbient=game.fx.ambientCount()>0;
            auto* combat=game.fx.freeParticle();combat->alive=true;combat->life=combat->maxLife=2;
            detailSettings.ambientDetail=0;game.paused=true;game.update(0.f);
            CHECK(hadAmbient&&game.fx.ambientCount()==0&&combat->alive,"lowering detail while paused immediately removes weather and preserves combat effects");
        }
        // Exercise the shared armory through actual Story input, purchases and retry.
        for(int chapter:{1,2}) {
            g_storyChapter=chapter;g_startArena=chapter==1?2:0;g_startWave=0;g_practice=g_godMode=false;
            GameplayState game(audio,nullptr,GameMode::STORY);
            game.gainXp(240); // Two earned levels, rather than directly granting points.
            CHECK(game.prog.points==2,"Story XP supplies spendable upgrade points");
            const int owned=game.storyOwned,ammo=game.weapons[0].ammo;
            const float damage=weaponDamage(WeaponId::REVOLVER,game.prog.up[0]);
            press(game,SDLK_TAB);
            const float clock=game.gameClock;auto position=game.player.position;game.update(.25f);
            CHECK(game.armoryOpen&&game.gameClock==clock&&glm::length(game.player.position-position)<.001f,"the Story armory freezes combat and player movement");
            SDL_Event repeat{};repeat.type=SDL_KEYDOWN;repeat.key.keysym.sym=SDLK_TAB;repeat.key.repeat=1;game.handleEvent(repeat);
            CHECK(game.armoryOpen,"holding Tab does not immediately close the armory");
            game.armoryS=0;press(game,SDLK_RETURN);
            CHECK(game.prog.points==1&&game.prog.up[0].tier[0]==1&&weaponDamage(WeaponId::REVOLVER,game.prog.up[0])>damage,"Enter buys a real Story damage upgrade and spends one earned point");
            press(game,SDLK_DOWN);press(game,SDLK_DOWN);press(game,SDLK_SPACE);
            CHECK(game.prog.points==0&&game.prog.up[0].tier[2]==1&&game.weapons[0].ammo>ammo,"arrows and Space buy a magazine upgrade and top up its added capacity");
            if(chapter==2){game.render();screenshot("build/story-armory.bmp");}
            press(game,SDLK_TAB);game.retryArena();
            CHECK(!game.armoryOpen&&game.prog.up[0].tier[0]==1&&game.prog.up[0].tier[2]==1&&game.prog.points==0&&game.storyOwned==owned,"checkpoint retry keeps purchased upgrades without changing the found weapons");
            SDL_Event back{};back.type=SDL_CONTROLLERBUTTONDOWN;back.cbutton.button=SDL_CONTROLLER_BUTTON_BACK;game.handleEvent(back);
            CHECK(game.armoryOpen,"the controller Back button opens the same Story armory");
            press(game,SDLK_ESCAPE);CHECK(!game.armoryOpen&&!game.paused,"Escape closes the armory without opening pause");
            game.storyEndT=1;press(game,SDLK_TAB);CHECK(!game.armoryOpen,"Tab leaves the chapter ending in control");game.storyEndT=-1;
            game.playerDead=true;press(game,SDLK_TAB);CHECK(!game.armoryOpen,"Tab cannot open the armory over Story death");game.playerDead=false;
            if(chapter==1){game.loadStoryChapter(2);press(game,SDLK_TAB);CHECK(game.armoryOpen&&game.prog.up[0].tier[0]==1&&game.prog.up[0].tier[2]==1&&game.prog.points==0,"continuing to Chapter 2 retains upgrades in its working armory");press(game,SDLK_TAB);}
        }
        checkImprovements(audio);
        checkLabRuntime(audio);
        checkSplinterRuntime(audio);
        checkPlainRuntime(audio);
        checkGleanerFightRuntime(audio);
        GameSettings visualSettings;visualSettings.ambientDetail=3;
        menu.settings=&visualSettings;menu.settingsMenu.s=&visualSettings;menu.page=MenuState::SETTINGS;
        const auto settingsRows=menu.settingsMenu.rows();
        bool markerSetting=false;
        for(const auto& row:settingsRows)if(std::string(row.label)=="ENEMY MARKERS")
            markerSetting=row.i==&visualSettings.enemyMarkers&&row.count==2&&std::string(row.name(0))=="LOW"&&std::string(row.name(1))=="DETAILED";
        CHECK(markerSetting,"shared settings expose Low and Detailed enemy markers");
        CHECK(menu.settingsMenu.rowY(settingsRows,(int)settingsRows.size()-1)+20<=SCREEN_H-24,"all settings including Back fit above the control legend");
        menu.render();screenshot("build/story-ambient-settings.bmp");
        g_startArena = g_startWave = 0; g_storyChapter = 1; g_practice = g_godMode = false;
        GameplayState normal(audio, nullptr, GameMode::STORY);
        CHECK(normal.storyLocked() && normal.storyOwned == 0 && !normal.storyArmed, "normal Story still begins with the unarmed wake-up");
        press(normal,SDLK_TAB);
        CHECK(!normal.armoryOpen&&normal.storyLocked(),"Tab leaves the initial wake-up sequence in control");
    }
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    return failures ? 1 : 0;
}
