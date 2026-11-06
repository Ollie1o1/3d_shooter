// The actual game, input path, sword contact and HUD, with a native GL context.
#include "game/GameplayState.h"
#include "ui/MenuState.h"
#include <cstdio>

static int failures = 0;
#define CHECK(c, msg) do { bool ok = (c); std::printf("%s %s\n", ok ? "ok:" : "FAIL:", msg); failures += !ok; } while (0)
static constexpr float DT = 1.f / 60.f;
static void screenshot(const char* path) {
    auto* s = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 24, SDL_PIXELFORMAT_RGB24);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
    auto* data = static_cast<unsigned char*>(s->pixels);
    std::vector<unsigned char> row(s->pitch);
    for (int y = 0; y < SCREEN_H / 2; ++y) {
        auto* a = data + y * s->pitch; auto* b = data + (SCREEN_H - 1 - y) * s->pitch;
        std::memcpy(row.data(), a, s->pitch); std::memcpy(a, b, s->pitch); std::memcpy(b, row.data(), s->pitch);
    }
    SDL_SaveBMP(s, path); SDL_FreeSurface(s);
}

int main() {
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    auto* window = SDL_CreateWindow("Combat feel regression", 0, 0, SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) { std::fprintf(stderr, "%s\n", SDL_GetError()); return 1; }
    auto context = SDL_GL_CreateContext(window);
    if (!context) { std::fprintf(stderr, "%s\n", SDL_GetError()); return 1; }
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental = GL_TRUE; glewInit(); glGetError();
#endif
    glViewport(0, 0, SCREEN_W, SCREEN_H); balanceInit();
    g_devNoMouse = g_devSword = g_practice = true; g_fixedDt = DT;
    {
        AudioSystem audio;
        for (const auto& m : mixTable()) audio.loadSound(m.name, std::string("assets/sfx/") + m.name + ".wav");
        GameSettings settings;
        GameMode selected=GameMode::ARENA; bool practice=false;
        MenuState menu(SCREEN_W,SCREEN_H);
        menu.onStart=[&](GameMode mode,StartOptions o){selected=mode;practice=o.practice;};
        SDL_Event f3{};f3.type=SDL_KEYDOWN;f3.key.keysym.sym=SDLK_F3;menu.handleEvent(f3);
        CHECK(selected==GameMode::TRAINING && practice,"F3 requests the real combat-training mode from the menu");
        auto live = std::make_unique<GameplayState>(audio, &settings, selected);
        auto& g = *live;
        CHECK(g.guideOpen && g.paused,"F3 enters a usable field guide");
        for(int i=0;i<10;++i)g.update(DT);
        g.render();screenshot("build/feel-pass/training-guide-1.bmp");
        bool allLessons=true;
        for(int lesson=0;lesson<lessons::COUNT;++lesson) {
            g.startTrainingLesson(lesson);g.updateTraining(.2f);
            for(int i=0;i<4;++i)g.update(DT);
            allLessons &= !g.playerDead && !g.victory && !g.ranked;
            if(lesson==8 || lesson==16) {
                g.openGuide();g.render();screenshot(lesson==8?"build/feel-pass/training-guide-2.bmp":"build/feel-pass/training-guide-3.bmp");
            }
        }
        CHECK(allLessons,"all twenty lessons start and update through the real game without reading a nonexistent wave");
        g.startTrainingLesson(lessons::SWORD_COMBO);g.updateTraining(.2f);
        for(auto& e:g.enemies){e.state=EnemyState::ACTIVE;e.spawnTimer=0;}
        for(int cut=0;cut<3;++cut)g.swordContact(cut,Sword::strokeBase(cut),false);
        CHECK(g.trainingPassed[lessons::SWORD_COMBO],"three actual sword contacts complete combo practice");
        g.startTrainingLesson(lessons::SWORD_BLOCK);
        g.sword.raised=true;g.sword.windowLive=false;g.invincFrames=0;
        g.damagePlayerStrike(10,g.player.position+g.player.camera.forward()*2.f,Sword::Strike::WHITE);
        CHECK(g.trainingPassed[lessons::SWORD_BLOCK],"blocking a real strike completes sword defence");
        g.startTrainingLesson(lessons::SWORD_RIPOSTE);g.updateTraining(.2f);
        for(auto& e:g.enemies){e.state=EnemyState::ACTIVE;e.spawnTimer=0;}
        g.swordContact(0,70*Sword::RIPOSTE_MULT,true);
        CHECK(g.trainingPassed[lessons::SWORD_RIPOSTE],"an empowered contact completes riposte practice");
        for(int lesson:{(int)lessons::SPLINTER,(int)lessons::SPLINTER_CHAIN}) {
            g.startTrainingLesson(lesson);g.updateTraining(.2f);
            for(auto& e:g.enemies){e.state=EnemyState::ACTIVE;e.spawnTimer=0;}
            auto& a=g.splinters.pool[0];a.alive=true;a.age=.3f;a.vel={0,0,0};
            a.pos=a.prev=g.player.camera.position+glm::vec3{0,1,-5};
            if(lesson==lessons::SPLINTER_CHAIN){auto& b=g.splinters.pool[1];b.alive=true;b.age=.3f;b.vel={0,0,0};b.pos=b.prev=g.player.camera.position+glm::vec3{2,2,-8};}
            auto dir=glm::normalize(a.pos-g.player.camera.position);
            g.player.camera.yaw=glm::degrees(std::atan2(dir.z,dir.x));g.player.camera.pitch=glm::degrees(std::asin(dir.y));
            g.fireWeapon(0);
            CHECK(g.splinterOwned() && g.trainingPassed[lesson],"a real SPLINTER redirect/chain completes its own lesson");
        }
        g.startTrainingLesson(lessons::RECALL);
        auto mark=g.player.position;g.pressRecall();g.recall.tick(.4f);g.player.position.x+=4;g.pressRecall();
        CHECK(g.recallOwned() && g.trainingPassed[lessons::RECALL] && glm::length(g.player.position-mark)<.01f,
              "placing, moving and actually returning completes RECALL practice");
        g.startTrainingLesson(7); g.enemies.clear(); g.paused = g.guideOpen = g.armoryOpen = false;
        g.countdown = 0; g.player.camera.yaw = -90; g.player.camera.pitch = 0;
        const auto feet = g.player.position;
        auto target = [&](EnemyType type, bool shielded) {
            g.enemies.clear(); g.fx.clear(); g.player.position = feet;
            g.player.camera.position = feet + glm::vec3{0, g.player.eyeHeight, 0};
            g.player.camera.yaw = -90; g.player.camera.pitch = 0;
            Enemy e(type, feet + glm::vec3{0, 0, -2.6f});
            e.state = EnemyState::ACTIVE; e.spawnTimer = 0; e.shielded = shielded;
            e.health = e.maxHealth = 1000; g.enemies.push_back(std::move(e));
            g.hitStopFrames = 0; g.bodyHitPush = g.bodyHitStrongest = 0;
            g.hitSoundCount = 0; g.ui.hitmarkerTimer = 0; g.sword = Sword{};
            g.swordOut = true; g.lmbPrev = g.rmbPrev = false;
        };
        target(EnemyType::BRUTE, false);
        gamepad::state().fire = true;
        g.updateSword(DT);
        CHECK(g.enemies[0].health == 1000 && g.hitStopFrames == 0, "real sword press starts motion without early damage or world freeze");
        int contacts = 0;
        for (int i = 0; i < 20; ++i) { float before = g.enemies[0].health; g.updateSword(DT); contacts += g.enemies[0].health < before; }
        CHECK(contacts == 1 && g.enemies[0].health == 930, "real blade contact deals the light stroke's damage once");
        CHECK(g.hitStopFrames == 0 && g.hitSoundCount == 0 && g.bodyHitPush == 0,
              "ordinary sword impact keeps the world running and does not layer bullet/body thumps over steel");
        gamepad::state().fire = false;
        target(EnemyType::BRUTE, true); g.styleSystem.health = 60;
        g.swordContact(0, 70, false);
        CHECK(g.enemies[0].health == 1000 && g.styleSystem.health == 60 && g.ui.hitmarkerBlocked && !g.ui.hitmarkerKill,
              "shielded sword contact shows resistance, deals no damage and grants no healing");
        g.render(); screenshot("build/feel-pass/blocked-contact.bmp");
        target(EnemyType::SOVEREIGN, false);
        auto attack = g.enemies[0].attack;
        g.swordContact(1, 70, true);
        CHECK(g.enemies[0].flinchTwist > 0 && g.enemies[0].flinchAmp <= hitreact::BOSS_FLINCH_MAX && g.enemies[0].attack == attack,
              "a sword cut deflects the boss in its direction without cancelling its attack");
        g.mode=GameMode::ARENA;g.ui.bossDamageTrail.reset(1); g.render();
        g.enemies[0].health = 670; g.render(); screenshot("build/feel-pass/boss-damage-trail.bmp");g.mode=GameMode::TRAINING;
        CHECK(g.ui.bossDamageTrail.value > g.ui.bossDamageTrail.health, "real HUD retains a visible boss damage chunk");
        g.enemies.clear(); g.swordOut = false; g.activeWeapon = 0; g.pendingWeapon = -1;
        auto& gun = g.weapons[0]; gun.ammo = 8; gun.reloading = false; gun.cooldown = .04f;
        g.gameClock = 10; g.pendingFire = true; g.shootInput();
        CHECK(gun.ammo == 8, "a buffered trigger never bypasses the weapon's fire rate");
        g.gameClock += .05f; gun.tick(.05f, 8); g.shootInput(); g.shootInput();
        CHECK(gun.ammo == 7, "a near-ready trigger fires once when the real gun becomes ready");
        gun.cooldown = .2f; g.pendingFire = true; g.shootInput();
        g.gameClock += .21f; gun.tick(.21f, 8); g.shootInput();
        CHECK(gun.ammo == 7, "an expired trigger never fires unexpectedly later");
        gun.ammo = 0; gun.startReload(.04f); g.pendingFire = true; g.shootInput();
        g.gameClock += .05f; gun.tick(.05f, 8); g.shootInput();
        CHECK(gun.ammo == 7, "a press just before reload completion fires the loaded gun once");
        g.fireInput.clear(); g.pendingWeapon = 1; g.weapons[1].ammo = 2; g.weapons[1].cooldown = 0;
        g.weapons[1].reloading = false; g.pendingFire = true; g.shootInput();
        CHECK(g.weapons[1].ammo == 2 && g.fireInput.context == 1, "a press during a switch belongs to the destination gun");
        g.activeWeapon = 1; g.pendingWeapon = -1; g.gameClock += .04f; g.shootInput();
        CHECK(g.weapons[1].ammo == 1, "the queued press fires the destination gun, not the holstered weapon");
        g.ui.hitmarkerTimer = 0; g.ui.onHit(true, true); g.ui.onHit(false, false); g.ui.onBlocked();
        CHECK(g.ui.hitmarkerKill && g.ui.hitmarkerHead && !g.ui.hitmarkerBlocked, "a later pellet or blocked contact cannot erase a same-burst kill/headshot marker");
        g.fireInput.press(1, g.gameClock); g.dashInput.press(0, g.gameClock); g.pendingFire = true;
        g.paused = true; g.update(DT);
        CHECK(g.fireInput.context < 0 && g.dashInput.context < 0 && !g.pendingFire, "pause cancels queued actions instead of firing them on resume");
        g.paused = false; g.swordOut = false; g.hitStopFrames = 0; g.player.position = feet;
        g.player.velocity = {0,0,0}; g.player.camera.position = feet + glm::vec3{0,g.player.eyeHeight,0};
        g.prevDashKey = false; g.dashCharges = 0; g.dashCooldown = .03f;
        Uint8 keys[SDL_NUM_SCANCODES] = {}; keys[SDL_SCANCODE_LSHIFT] = 1;
        g.physicsTick(DT, keys, false); g.gameClock += DT;
        keys[SDL_SCANCODE_LSHIFT] = 0; g.physicsTick(DT, keys, false); g.gameClock += DT;
        g.physicsTick(DT, keys, false);
        CHECK(g.dashMomentumTimer > .2f && g.dashCharges == 0, "an early dash press survives the final recharge ticks and executes once");
        Enemy body(EnemyType::BRUTE, {0,0,0});
        g.bodyHitPush = g.bodyHitStrongest = 0;
        g.bodyHit(body, {1,0,0}, 4, false); g.bodyHit(body, {2,0,0}, 4, false); g.bodyHit(body, {3,0,0}, 6, false);
        CHECK(g.bodyHitAt.x == 3 && g.bodyHitPush == 14, "the impact mix follows the strongest contact rather than comparing against accumulated pellets");
    }
    {
        AudioSystem audio;g_startArena=3;g_godMode=true;
        auto live=std::make_unique<GameplayState>(audio,nullptr,GameMode::ACT2);auto& g=*live;
        g.enterArena(3);g.act2Falling=false;g.director.hold=true;
        bool floor=true,hover=true,recovery=true,souls=true;
        for(int wave=0;wave<3;++wave) {
            auto& fm=g.level.formation;fm.at=fm.to=wave;fm.t=0;fm.apply(g.level);g.level.updateMovers(0);
            g.enemies.clear();g.director.wave=wave;
            auto point=g.director.pickSpawnForTest(EnemyType::HUSK,g.player.position);
            g.spawnEnemy(EnemyType::HUSK,point);
            g.spawnEnemy(EnemyType::RAPTOR,g.level.arenas[3].waveAir[wave][0]);
            hover &= g.enemies.back().hoverY>2 && g.enemies.back().hoverY<24;
            for(int tick=0;tick<40;++tick){g.gameClock+=DT;g.updateEnemies(DT);}
            floor &= g.enemies[0].position.y < -200 && std::fabs(g.enemies[0].floorY-point.y)<4;
            g.relic.clear();
            Enemy revenant(EnemyType::REVENANT,fm.top(0));revenant.scale=1.3f;
            g.releaseRevenantSoul(revenant,false);
            souls &= g.relic.souls.size()==1 && spawnClearance(g.level,g.relic.souls[0].to,revenant.radius(),revenant.height(),true);
            g.enemies.clear();g.updateReliquary(RelicHazards::SOUL_TIME+.01f);
            souls &= g.enemies.size()==1 && spawnClearance(g.level,g.enemies[0].position,g.enemies[0].radius(),g.enemies[0].height(),true);
        }
        for(int arrangement=0;arrangement<4;++arrangement) {
            auto& fm=g.level.formation;fm.at=fm.to=arrangement;fm.t=0;fm.apply(g.level);g.level.updateMovers(0);
            for(int c=0;c<(int)fm.chunks.size();++c) {
                g.lastChunk=c;g.player.onGround=false;g.updateReliquary(DT);
                auto p=g.level.arenas[3].respawn-glm::vec3{0,.05f,0};
                recovery &= spawnClearance(g.level,p,g.player.radius,g.player.height,true);
                g.player.position={p.x,g.level.arenas[3].voidY-2.f,p.z};
                g.keepPlayerInZone(DT);
                recovery &= glm::length(g.player.position-g.level.arenas[3].respawn)<.1f &&
                    spawnClearance(g.level,g.player.position,g.player.radius,g.player.height,true);
            }
        }
        CHECK(floor,"Reliquary walkers stay on the actual relic instead of snapping toward world Y zero");
        CHECK(hover,"Reliquary flyers are initialized above the formation, not hundreds of metres above the void floor");
        CHECK(souls,"Revenant souls reform with their full scaled body on a supported clear platform");
        CHECK(recovery,"every Reliquary recovery point clears the actual player's body in all four formations");
        g.enemies.clear();g.banners.clear();g.ui.cards.clear();g.ui.feedLines.clear();
        g.ui.damageVignette=g.ui.parryFlash=g.ui.shootFlashTimer=0;
        for(int arrangement=0;arrangement<4;++arrangement) {
            auto& fm=g.level.formation;fm.at=fm.to=arrangement;fm.t=0;fm.apply(g.level);g.level.updateMovers(0);g.level.updateMovers(0);
            g.player.position=RELIQUARY_Q+glm::vec3{0,15,59};
            g.player.camera.position=g.player.position+glm::vec3{0,g.player.eyeHeight,0};
            g.prevCamPos=g.player.camera.position;g.renderAlpha=1;
            g.player.camera.yaw=-90;g.player.camera.pitch=-18;g.gameClock=5;
            g.render();std::string path="build/feel-pass/reliquary-"+std::to_string(arrangement)+".bmp";screenshot(path.c_str());
        }
        g_godMode=false;g_startArena=0;
    }
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    std::printf("%d runtime feel failures\n", failures);
    return failures ? 1 : 0;
}
