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
        bool correct=false;EnemyType expected=i==6?EnemyType::PACEMAKER:i==7?EnemyType::SORTER:EnemyType::BALLAST;
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
        CHECK(keyboard && checkpoints.size() == 17, "all 17 checkpoints launch the correct chapter and room with practice/god flags");
        CHECK(menu.devScroll[2] > 0, "keyboard navigation reveals the later Chapter 2 checkpoints");
        menu.render(); screenshot("build/dev-story-menu-scrolled.bmp");
        // The last checkpoint is at the bottom of the visible Story column.
        SDL_Event click{}; click.type = SDL_MOUSEBUTTONDOWN; click.button.button = SDL_BUTTON_LEFT;
        click.button.x = 1000; click.button.y = 232 + 10 * 34;
        int before = starts; menu.handleEvent(click);
        CHECK(starts == before + 1 && selected.storyChapter == 2 && selected.arena == 7,
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
            int expectedGuns = o.storyChapter == 1 ? expectedGuns1[o.arena] : o.arena > 4 ? 15 : 7;
            bool expectedGrapple = o.storyChapter == 2 || o.arena > 7;
            char label[128];
            std::snprintf(label, sizeof(label), "Story %d.%d starts on its checkpoint, awake, with earlier gear", o.storyChapter, o.arena + 1);
            CHECK(game.level.story.chapter == o.storyChapter && game.director.arena == o.arena && game.storyCheckpoint == o.arena &&
                  glm::length(game.player.position - game.level.arenas[o.arena].playerStart) < .01f && !game.storyLocked() &&
                  game.storyOwned == expectedGuns && game.storyGrapple == expectedGrapple && game.storyArmed == (expectedGuns != 0) && !game.ranked, label);
            game.retryArena();
            CHECK(game.director.arena == o.arena && game.storyOwned == expectedGuns && !game.storyLocked(), "retry preserves the selected checkpoint and gear");
            // Launching a checkpoint must survive its first drawn frames too:
            // those load the chapter's nearby rigs, scenery and HUD paths.
            for (int frame=0;frame<3;++frame) { game.update(1.f/60.f);game.render(); }
            CHECK(!game.playerDead, "Story checkpoint survives its first gameplay and render frames");
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
