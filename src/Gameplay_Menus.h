#pragma once
// =============================================================================
// Gameplay_Menus.h — GameplayState: Input, and the screens over the game: pause, armory, the victory
// screen's name entry and leaderboard, and gamepad buttons.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::renderLeaderboardPanel() {
    float t = gameClock + (float)SDL_GetTicks() * 0.001f;
    float x = 926.f, w = 334.f, y = 110.f;
    Board bd = boardFor();
    Leaderboard::Entry me = runEntry();
    ui.begin2D();
    UIBatch& b = ui.ui;
    if (!ranked) {
        b.text("PRACTICE RUN - NOT RANKED", x + w / 2, y + 8, 1, {0.7f, 0.7f, 0.75f, 0.85f}, true);
        y += 30.f;
    } else if (nameEntry) {
        b.rect(x, y, w, 112, {0.08f, 0.05f, 0.02f, 0.9f});
        b.frame(x, y, w, 112, 2, {1.f, 0.75f, 0.2f, 0.9f});
        char buf[64];
        int wp = worldLoaded ? worldBoard.placeFor(bd, me) : -1;
        if (wp >= 0) std::snprintf(buf, sizeof(buf), "YOU MADE THE WORLD BOARD - #%d", wp + 1);
        else         std::snprintf(buf, sizeof(buf), "YOU MADE THE BOARD - #%d", std::max(1, board.placeFor(bd, me) + 1));
        b.text(buf, x + w / 2, y + 12, 2, {1.f, 0.85f, 0.3f, 1.f}, true);
        b.rect(x + 20, y + 40, w - 40, 32, {0.f, 0.f, 0.f, 0.7f});
        std::string shown = nameBuf;
        if (std::fmod(t, 1.f) < 0.55f && (int)nameBuf.size() < Leaderboard::MAX_NAME_LEN) shown += "_";
        b.text(shown.empty() ? " " : shown.c_str(), x + 30, y + 48, 3, {1.f, 1.f, 1.f, 1.f});
        b.text("TYPE YOUR NAME   ENTER - SAVE   ESC - SKIP", x + w / 2, y + 88, 1, {0.8f, 0.75f, 0.65f, 0.9f}, true);
        y += 128.f;
    } else if (boardPlace >= 0) {
        b.text("SAVED TO THE LEADERBOARD", x + w / 2, y + 8, 2, {1.f, 0.85f, 0.3f, 0.95f}, true);
        y += 34.f;
    }
    // The shared board when there is one (re-read every second: the post
    // and the refresh land asynchronously), else this browser's
    if ((worldPoll -= 1.f / 60.f) <= 0.f) { worldPoll = 1.f; worldLoaded = worldBoard.loadOnline(); }
    auto same = [&](const Leaderboard::Entry& e) {
        return e.name == board.lastName && (Leaderboard::byScore(bd) ? e.score == me.score : std::fabs(e.time - me.time) < 0.02f);
    };
    std::string title = dailyRun() ? "DAILY " + daily.label() : "";
    if (worldLoaded) {
        int hi = -1;
        const auto& l = worldBoard.list(bd);
        for (int i = 0; i < (int)l.size(); ++i) if (boardPlace >= 0 && same(l[i])) hi = i;
        b.text("WORLD", x + 8, y + 14, 1, {0.4f, 0.9f, 1.f, 0.9f});
        drawLeaderboardTable(b, worldBoard, bd, x, y, w, nameEntry ? 8 : Leaderboard::KEEP, hi, t, title.empty() ? nullptr : title.c_str());
    } else {
        drawLeaderboardTable(b, board, bd, x, y, w, nameEntry ? 8 : Leaderboard::KEEP, boardPlace < 99 ? boardPlace : -1, t,
                             title.empty() ? nullptr : title.c_str());
    }
    ui.end2D();
}

inline void GameplayState::padButton(Uint8 b) {
    auto key = [&](SDL_Keycode k) { if (k != SDLK_UNKNOWN) { SDL_Event e = gamepad::keyEvent(k); handleEvent(e); } };
    if (victory && nameEntry) {   // no on-screen keyboard: A saves the name as it stands, B skips
        key(b == SDL_CONTROLLER_BUTTON_A || b == SDL_CONTROLLER_BUTTON_START ? SDLK_RETURN
          : b == SDL_CONTROLLER_BUTTON_B ? SDLK_ESCAPE : SDLK_UNKNOWN);
        return;
    }
    if (playerDead) {             // A retries, X starts over, B to the menu
        key(b == SDL_CONTROLLER_BUTTON_A ? SDLK_r : b == SDL_CONTROLLER_BUTTON_X ? SDLK_RETURN
          : b == SDL_CONTROLLER_BUTTON_B ? SDLK_ESCAPE : SDLK_UNKNOWN);
        return;
    }
    if (victory) { key(b == SDL_CONTROLLER_BUTTON_A ? SDLK_RETURN : b == SDL_CONTROLLER_BUTTON_B ? SDLK_ESCAPE : SDLK_UNKNOWN); return; }
    if (armoryOpen) { key(b == SDL_CONTROLLER_BUTTON_BACK ? SDLK_TAB : gamepad::menuKey(b)); return; }
    if (paused) { key(gamepad::menuKey(b)); return; }
    switch (b) {
        case SDL_CONTROLLER_BUTTON_START:         key(SDLK_ESCAPE); break;
        case SDL_CONTROLLER_BUTTON_BACK:          key(SDLK_TAB); break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  pendingGrapple = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:       pendingGrenade = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     key(SDLK_v); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     trySwitch((activeWeapon + WEAPON_COUNT - 1) % WEAPON_COUNT); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    trySwitch((activeWeapon + 1) % WEAPON_COUNT); break;
        default: break;
    }
}

inline void GameplayState::submitWorld(const std::string& name, int diff) {
#ifdef __EMSCRIPTEN__
    Leaderboard::Entry e = runEntry();
    char js[240];
    std::snprintf(js, sizeof(js),
                  "window.overdriveBoard&&window.overdriveBoard.submit({mode:'%s',name:'%s',time:%.2f,difficulty:%d,score:%d,wave:%d,date:'%s'})",
                  boardKey(boardFor()), name.c_str(), elapsedTime, diff, e.score, e.wave, daily.key().c_str());
    emscripten_run_script(js);
#else
    (void)name; (void)diff;
#endif
}

inline void GameplayState::handleNameEntry(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;
    SDL_Keycode k = e.key.keysym.sym;
    if (k == SDLK_BACKSPACE) { if (!nameBuf.empty()) nameBuf.pop_back(); return; }
    if (e.key.repeat) return;
    if (k == SDLK_ESCAPE) { nameEntry = false; return; }
    if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
        int diff = settings ? settings->difficulty : DIFFICULTY_DEFAULT;
        std::string name = Leaderboard::cleanName(nameBuf);
        boardPlace = board.add(boardFor(), nameBuf, runEntry());
        if (boardPlace >= 0) board.save();
        if (!name.empty()) {
            audio.play("upgrade");
            submitWorld(name, diff);
            if (boardPlace < 0) boardPlace = 99;   // saved to the shared board only
        }
        nameEntry = false;
        return;
    }
    char c = 0;
    if (k >= SDLK_a && k <= SDLK_z) c = (char)('A' + (k - SDLK_a));
    else if (k >= SDLK_0 && k <= SDLK_9) c = (char)('0' + (k - SDLK_0));
    else if (k >= SDLK_KP_1 && k <= SDLK_KP_9) c = (char)('1' + (k - SDLK_KP_1));
    else if (k == SDLK_KP_0) c = '0';
    else if (k == SDLK_SPACE) c = ' ';
    else if (k == SDLK_MINUS) c = '-';
    else if (k == SDLK_PERIOD) c = '.';
    if (c && (int)nameBuf.size() < Leaderboard::MAX_NAME_LEN && !(c == ' ' && nameBuf.empty())) nameBuf += c;
}

inline void GameplayState::handleEvent(const SDL_Event& e) {
    // Gameplay keys act once per press; held-key auto-repeat would toggle
    // the grapple, armory or pause on and off. (Menus below read `e`
    // directly, where repeat is wanted for arrows and sliders.)
    SDL_Keycode key = (e.type == SDL_KEYDOWN && !e.key.repeat) ? e.key.keysym.sym : SDLK_UNKNOWN;

    if (e.type == SDL_CONTROLLERBUTTONDOWN) { padButton(e.cbutton.button); return; }

    // Alt-tabbing away mid-fight pauses (the web build pauses on losing the pointer lock)
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && !g_devNoMouse) { pause(); return; }

    // Armory (TAB) — a paused overlay for spending upgrade points
    if (armoryOpen) {
        if (key == SDLK_TAB || key == SDLK_ESCAPE || key == SDLK_p) { closeArmory(); return; }
        handleArmoryEvent(e);
        return;
    }
    if (paused && pauseSettings) { settingsMenu.handleEvent(e); return; }
    if (victory && nameEntry) { handleNameEntry(e); return; }

    if (key == SDLK_ESCAPE || key == SDLK_p) {
        if (playerDead || victory) {
            if (key != SDLK_ESCAPE) return;  // P only pauses
            captureMouse(false);
            if (onReturnToMenu) onReturnToMenu();
            return;
        }
        paused = !paused;
        pauseSelected = 0;
        captureMouse(!paused);
        return;
    }
    if (paused) { handlePauseEvent(e); return; }
    if (g_practice && !playerDead && !victory) {
        if (key == SDLK_F5) { devClearWave(); return; }    // practice: skip the fight in progress
        if (key == SDLK_F6) { styleSystem.heal(1000.f); ui.feed("HEALTH REFILLED", {0.4f, 1.f, 0.6f}); return; }
    }
    if (key == SDLK_TAB && !playerDead && !victory) { openArmory(); return; }
    // Backspace: straight back to the checkpoint and the start of this fight
    if (key == SDLK_BACKSPACE && !victory) {
        if (!playerDead) restartHere(); else if (!endless()) retryArena();   // ENDLESS: one life
        return;
    }
    if (e.type == SDL_KEYDOWN && (playerDead || victory)) {
        if (key == SDLK_r && playerDead && !endless()) { retryArena(); return; }   // ENDLESS: one life
        if (key == SDLK_RETURN)          { newRun();     return; }
        return;
    }
    if (key == SDLK_e) {
        int idx = findInteractTarget();
        if (idx >= 0 && interactables[idx].onInteract) interactables[idx].onInteract();
    }
    // Grapple on its own button (Q by default, see Settings). Right mouse is
    // only ever aim (rifles), so scoping and grappling never fight over it.
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) pendingFire = true;
    {
        GameSettings def;
        const GameSettings& gs = settings ? *settings : def;
        if (key != SDLK_UNKNOWN && gs.isGrappleEvent(0, (int)key)) pendingGrapple = true;
        if (e.type == SDL_MOUSEBUTTONDOWN && gs.isGrappleEvent(1, e.button.button)) pendingGrapple = true;
    }
    if (e.type == SDL_MOUSEWHEEL && e.wheel.y != 0)
        trySwitch((activeWeapon + (e.wheel.y > 0 ? WEAPON_COUNT - 1 : 1)) % WEAPON_COUNT);
    if (key >= SDLK_1 && key <= SDLK_4) trySwitch(key - SDLK_1);
    if (key == SDLK_g) pendingGrenade = true;
    if (key == SDLK_v && pendingWeapon < 0 && !weapons[activeWeapon].reloading) viewModel.triggerInspect();
    if (e.type == SDL_MOUSEMOTION && !g_devNoMouse) {
        float dx = (float)e.motion.xrel, dy = (float)e.motion.yrel;
        mouseFilter.enabled = settings ? settings->mouseFilter : true;
        if (!mouseFilter.accept(dx, dy)) return;
        float sens = (settings ? settings->sensitivity : 0.1f) * aimSensScale();
        if (settings && settings->invertY) dy = -dy;
        player.applyMouseLook(dx, dy, sens);
    }
}

inline void GameplayState::trySwitch(int w) {
    if (modOn(DailyMod::MARKSMAN) && w != (int)WeaponId::KAR) return;   // DAILY: the Kar98 only
    if (w != activeWeapon && pendingWeapon < 0) {
        pendingWeapon = w;
        weaponSwitchTimer = 0.15f;
        viewModel.triggerSwitch();
        audio.play("reload", 80);
    }
}

inline float GameplayState::aimSensScale() const {
    const WeaponDef& d = weaponDef((WeaponId)activeWeapon);
    if (!d.canAim || aim <= 0.f) return 1.f;
    float base = settings ? settings->fov : 90.f;
    float zoomed = base * d.aimFov;
    float ratio = std::tan(glm::radians(zoomed) * 0.5f) / std::tan(glm::radians(base) * 0.5f);
    float k = (settings ? settings->zoomSens : 1.f) * ratio;
    return glm::mix(1.f, k, aim);
}

inline void GameplayState::pause() {
    if (paused || playerDead || victory || armoryOpen) return;
    paused = true;
    pauseSelected = 0;
    captureMouse(false);
}

inline const char* GameplayState::pauseLabel(int i) const {
    static const char* L[] = {"RESUME", "SETTINGS", "", "", "", "QUIT TO MENU"};
    if (i == 2) return fullscreenOn() ? "EXIT FULLSCREEN" : "FULLSCREEN";
    if (i == 3) return fast() ? "RESTART ROOM" : "RESTART ARENA";
    if (i == 4) return fast() ? "RESTART RUN" : "NEW RUN";
    return L[i];
}

inline bool GameplayState::fullscreenOn() const {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({ return window.overdriveFullscreen ? (window.overdriveFullscreen.isOn() ? 1 : 0) : 0; }) != 0;
#else
    return settings && settings->fullscreen;
#endif
}

inline void GameplayState::toggleFullscreen() {
#ifdef __EMSCRIPTEN__
    EM_ASM({ if (window.overdriveFullscreen) window.overdriveFullscreen.toggle(); });
#else
    if (settings) { settings->fullscreen = !settings->fullscreen; settings->save(); }
#endif
}

inline void GameplayState::activatePauseItem(int idx) {
    switch (idx) {
    case 0: paused = false; captureMouse(true); break;
    case 1: pauseSettings = true; settingsMenu.selected = 1; break;
    case 2: toggleFullscreen(); break;
    case 3: restartHere(); break;
    case 4: newRun(); break;
    default:
        captureMouse(false);
        if (onReturnToMenu) onReturnToMenu();
        break;
    }
}

inline int GameplayState::pauseItemAt(int mx, int my) const {
    for (int i = 0; i < UIRenderer::PAUSE_ITEMS; ++i) {
        int by = ui.pauseButtonY(i);
        if (mx > SCREEN_W/2-150 && mx < SCREEN_W/2+150 && my > by && my < by+46) return i;
    }
    return -1;
}

inline void GameplayState::handlePauseEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_UP:   pauseSelected = (pauseSelected + UIRenderer::PAUSE_ITEMS - 1) % UIRenderer::PAUSE_ITEMS; break;
            case SDLK_DOWN: pauseSelected = (pauseSelected + 1) % UIRenderer::PAUSE_ITEMS; break;
            case SDLK_RETURN:
            case SDLK_SPACE:
                activatePauseItem(pauseSelected); break;
            default: break;
        }
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int i = pauseItemAt(e.button.x, e.button.y);
        if (i >= 0) activatePauseItem(i);
    }
    if (e.type == SDL_MOUSEMOTION) {
        int i = pauseItemAt(e.motion.x, e.motion.y);
        if (i >= 0) pauseSelected = i;
    }
}

inline void GameplayState::openArmory() {
    armoryOpen = true;
    captureMouse(false);
    // Start on the gun in hand
    armoryW = activeWeapon;
}

inline void GameplayState::closeArmory() {
    armoryOpen = false;
    captureMouse(true);
}

inline void GameplayState::buyUpgrade(int w, int s) {
    WeaponId wid = (WeaponId)w;
    int oldMag = weaponMag(wid, prog.up[w]);
    if (!prog.buy(wid, (UpgradeStat)s)) { audio.play("telegraph", 50); return; }
    // A bigger magazine is topped up straight away
    int newMag = weaponMag(wid, prog.up[w]);
    if (newMag > oldMag && !weapons[w].reloading) weapons[w].ammo += newMag - oldMag;
    audio.play("upgrade");
}

inline void GameplayState::handleArmoryEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_LEFT:  armoryW = (armoryW + WEAPON_COUNT - 1) % WEAPON_COUNT; break;
            case SDLK_RIGHT: armoryW = (armoryW + 1) % WEAPON_COUNT; break;
            case SDLK_UP:    armoryS = (armoryS + UPGRADE_STATS - 1) % UPGRADE_STATS; break;
            case SDLK_DOWN:  armoryS = (armoryS + 1) % UPGRADE_STATS; break;
            case SDLK_RETURN:
            case SDLK_SPACE: buyUpgrade(armoryW, armoryS); break;
            default: break;
        }
    }
    int w, s;
    if (e.type == SDL_MOUSEMOTION && ui.armoryCellAt(e.motion.x, e.motion.y, w, s)) { armoryW = w; armoryS = s; }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT &&
        ui.armoryCellAt(e.button.x, e.button.y, w, s)) {
        armoryW = w; armoryS = s;
        buyUpgrade(w, s);
    }
}
