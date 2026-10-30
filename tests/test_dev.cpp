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
    }
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    return failures ? 1 : 0;
}
