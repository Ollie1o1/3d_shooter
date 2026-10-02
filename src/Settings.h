#pragma once
#include "Persist.h"
#include "Difficulty.h"
#include <sstream>
#include <cstdio>
#include <string>

// Every option on the settings page (main menu and pause menu share it).
// Saved as "key value" lines through Persist.h: a file on desktop, the
// browser's localStorage on the web.
struct GameSettings {
    // Video
    float fov         = 90.f;   // 60..120
    int   frameCap    = 1;      // index into FRAME_CAPS: MATCH DISPLAY by default
    bool  vsync       = false;  // wait for the display (no tearing, a little more latency)
    bool  fullscreen  = false;  // borderless fullscreen at the desktop resolution (F11 / Alt+Enter)
    // Graphics quality: the resolution everything renders at (16:9 lines,
    // whatever the window), plus texture sharpness at a glancing angle. Above
    // the screen's own resolution it's supersampling: smoother edges.
    static constexpr int QUALITY_LEVELS = 4;
#ifdef __EMSCRIPTEN__
    static constexpr int QUALITY_DEFAULT = 1;   // MEDIUM: kind to laptops in a browser
#else
    static constexpr int QUALITY_DEFAULT = 2;   // HIGH
#endif
    int   quality = QUALITY_DEFAULT;
    static int wrapQuality(int i) { return (i % QUALITY_LEVELS + QUALITY_LEVELS) % QUALITY_LEVELS; }
    static int qualityLines(int i) {
        static const int L[QUALITY_LEVELS] = {720, 1080, 1440, 2160};
        return L[wrapQuality(i)];
    }
    static float qualityAnisotropy(int i) {
        static const float A[QUALITY_LEVELS] = {1.f, 4.f, 8.f, 16.f};
        return A[wrapQuality(i)];
    }
    static const char* qualityLabel(int i) {
        static const char* N[QUALITY_LEVELS] = {"LOW (720P)", "MEDIUM (1080P)", "HIGH (1440P)", "EXTREME (4K)"};
        return N[wrapQuality(i)];
    }
    bool  showFPS     = false;
    bool  crtFilter   = false;  // CRT post-process effect
    float screenShake = 1.0f;   // 0..1
    bool  viewBob     = true;
    // Controls
    float sensitivity = 0.10f;  // degrees per mouse count, 0.01..1.00
    float zoomSens    = 1.0f;   // multiplier while aiming down sights (after FOV scaling), 0.3..1.5
    bool  invertY     = false;
    bool  mouseFilter = true;   // drop single-event mouse spikes (see MouseFilter.h)
    int   grappleKey  = 0;      // see grappleLabel(): Q by default; right mouse always aims
    static constexpr int GRAPPLE_KEYS = 5;
    static const char* grappleLabel(int i) {
        static const char* N[] = {"Q", "E", "MOUSE 4", "MOUSE 5", "MIDDLE MOUSE"};
        return N[(i % GRAPPLE_KEYS + GRAPPLE_KEYS) % GRAPPLE_KEYS];
    }
    // Does this SDL event press the grapple button?
    bool isGrappleEvent(int sdlType, int keyOrButton) const {
        switch (grappleKey) {
            case 1:  return sdlType == 0 && keyOrButton == 'e';
            case 2:  return sdlType == 1 && keyOrButton == 4;   // SDL_BUTTON_X1
            case 3:  return sdlType == 1 && keyOrButton == 5;   // SDL_BUTTON_X2
            case 4:  return sdlType == 1 && keyOrButton == 2;   // SDL_BUTTON_MIDDLE
            default: return sdlType == 0 && keyOrButton == 'q';
        }
    }
    // Audio
    float audioVolume = 0.8f;   // 0.0..1.0, everything
    float musicVolume = 0.6f;   // 0.0..1.0, the soundtrack on top of that
    // Gameplay / HUD
    int   difficulty   = DIFFICULTY_DEFAULT;   // see Difficulty.h
    bool  showTimer    = true;  // run clock in Arena mode (FAST mode always shows it)
    bool  ghost        = true;  // FAST: race the ghost of your best run
    bool  damageNumbers = true;
    int   crosshair    = 0;     // colour index, see crosshairColor()

    static constexpr int CROSSHAIR_COLORS = 5;
    static const char* crosshairLabel(int i) {
        static const char* N[] = {"WHITE", "GREEN", "CYAN", "YELLOW", "PINK"};
        return N[(i % CROSSHAIR_COLORS + CROSSHAIR_COLORS) % CROSSHAIR_COLORS];
    }
    void crosshairColor(float& r, float& g, float& b) const {
        static const float C[][3] = {{1.f,1.f,1.f},{0.3f,1.f,0.35f},{0.3f,0.95f,1.f},{1.f,0.9f,0.2f},{1.f,0.35f,0.8f}};
        int i = (crosshair % CROSSHAIR_COLORS + CROSSHAIR_COLORS) % CROSSHAIR_COLORS;
        r = C[i][0]; g = C[i][1]; b = C[i][2];
    }

    // Frame rate limit choices: 0 = unlimited, -1 = the display's refresh rate
    static constexpr int FRAME_CAPS = 8;
    static int frameCapHz(int i) {
        static const int C[FRAME_CAPS] = {0, -1, 60, 120, 144, 165, 240, 360};
        return C[(i % FRAME_CAPS + FRAME_CAPS) % FRAME_CAPS];
    }
    // The display's refresh rate, filled in by main (60 if unknown)
    static int& displayHz() { static int hz = 60; return hz; }
    // Frames per second to hold, or 0 for no limit
    int getFPSCapValue() const {
        int hz = frameCapHz(frameCap);
        return hz < 0 ? displayHz() : hz;
    }
    const char* getFPSCapLabel() const {
        static char buf[32];
        int hz = frameCapHz(frameCap);
        if (hz == 0) return "UNLIMITED";
        if (hz < 0) std::snprintf(buf, sizeof(buf), "DISPLAY (%d)", displayHz());
        else        std::snprintf(buf, sizeof(buf), "%d", hz);
        return buf;
    }

    static constexpr const char* kSaveKey = "settings.cfg";

    void save() const {
        std::ostringstream f;
        f << "fov "           << fov           << "\n";
        f << "sensitivity "   << sensitivity   << "\n";
        f << "zoomSens "      << zoomSens      << "\n";
        f << "invertY "       << (invertY ? 1 : 0) << "\n";
        f << "mouseFilter "   << (mouseFilter ? 1 : 0) << "\n";
        f << "grappleKey "    << grappleKey    << "\n";
        f << "audioVolume "   << audioVolume   << "\n";
        f << "musicVolume "   << musicVolume   << "\n";
        f << "frameCap "      << frameCap      << "\n";
        f << "vsync "         << (vsync ? 1 : 0) << "\n";
        f << "fullscreen "    << (fullscreen ? 1 : 0) << "\n";
        f << "quality "       << quality       << "\n";
        f << "showFPS "       << (showFPS   ? 1 : 0) << "\n";
        f << "crtFilter "     << (crtFilter ? 1 : 0) << "\n";
        f << "screenShake "   << screenShake   << "\n";
        f << "viewBob "       << (viewBob ? 1 : 0) << "\n";
        f << "showTimer "     << (showTimer ? 1 : 0) << "\n";
        f << "ghost "         << (ghost ? 1 : 0) << "\n";
        f << "damageNumbers " << (damageNumbers ? 1 : 0) << "\n";
        f << "crosshair "     << crosshair     << "\n";
        f << "difficulty "    << difficulty    << "\n";
        persist::save(kSaveKey, f.str());
    }

    void load() {
        std::istringstream f(persist::load(kSaveKey));
        std::string key;
        auto flag = [&](bool& b) { int v = 0; f >> v; b = v != 0; };
        while (f >> key) {
            if      (key == "fov")           f >> fov;
            else if (key == "sensitivity")   f >> sensitivity;
            else if (key == "zoomSens")      f >> zoomSens;
            else if (key == "invertY")       flag(invertY);
            else if (key == "mouseFilter")   flag(mouseFilter);
            else if (key == "grappleKey")    f >> grappleKey;
            else if (key == "audioVolume")   f >> audioVolume;
            else if (key == "musicVolume")   f >> musicVolume;
            else if (key == "frameCap")      f >> frameCap;
            else if (key == "vsync")         flag(vsync);
            else if (key == "fullscreen")    flag(fullscreen);
            else if (key == "quality")       f >> quality;
            else if (key == "fpsCap") {      // older saves: 0 uncapped, 1 60, 2 144, 3 180, 4 240
                int old = 0; f >> old;
                static const int MAP[] = {0, 2, 4, 4, 6};
                frameCap = (old >= 0 && old <= 4) ? MAP[old] : 1;
            }
            else if (key == "showFPS")       flag(showFPS);
            else if (key == "crtFilter")     flag(crtFilter);
            else if (key == "screenShake")   f >> screenShake;
            else if (key == "viewBob")       flag(viewBob);
            else if (key == "showTimer")     flag(showTimer);
            else if (key == "ghost")         flag(ghost);
            else if (key == "damageNumbers") flag(damageNumbers);
            else if (key == "crosshair")     f >> crosshair;
            else if (key == "difficulty")    f >> difficulty;
            else { std::string skip; f >> skip; } // unknown key — skip its value
            if (!f) break;
        }
        clampAll();
    }

    void clampAll() {
        fov         = clampf(fov, 60.f, 120.f);
        sensitivity = clampf(sensitivity, 0.01f, 1.00f);
        zoomSens    = clampf(zoomSens, 0.3f, 1.5f);
        audioVolume = clampf(audioVolume, 0.f, 1.f);
        musicVolume = clampf(musicVolume, 0.f, 1.f);
        screenShake = clampf(screenShake, 0.f, 1.f);
        frameCap = (frameCap % FRAME_CAPS + FRAME_CAPS) % FRAME_CAPS;
        quality = wrapQuality(quality);
        crosshair = (crosshair % CROSSHAIR_COLORS + CROSSHAIR_COLORS) % CROSSHAIR_COLORS;
        if (difficulty < 0 || difficulty >= DIFFICULTY_LEVELS) difficulty = DIFFICULTY_DEFAULT;
        grappleKey = (grappleKey % GRAPPLE_KEYS + GRAPPLE_KEYS) % GRAPPLE_KEYS;
    }

    static float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
};
