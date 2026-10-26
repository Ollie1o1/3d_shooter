#pragma once
#include "core/Persist.h"
#include "core/Difficulty.h"
#include "core/Bindings.h"
#include "core/Accessibility.h"
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
    // Anti-aliasing: MSAA samples per pixel. Smooths the edges of boxes and keeps thin
    // neon lines whole in the distance. (No 8X: Apple GPUs stop at 4, and 4X is the sweet spot.)
    static constexpr int AA_LEVELS = 3, AA_DEFAULT = 2;   // 4X
    int   antiAlias = AA_DEFAULT;
    static int aaSamples(int i) { static const int S[AA_LEVELS] = {0, 2, 4}; return S[(i % AA_LEVELS + AA_LEVELS) % AA_LEVELS]; }
    static const char* aaLabel(int i) { static const char* N[AA_LEVELS] = {"OFF", "2X", "4X"}; return N[(i % AA_LEVELS + AA_LEVELS) % AA_LEVELS]; }
    bool  showFPS     = false;
    bool  crtFilter   = false;  // CRT post-process effect
    float screenShake = 1.0f;   // 0..1
    bool  viewBob     = true;
    // Controls
    float sensitivity = 0.10f;  // degrees per mouse count, 0.01..1.00
    float zoomSens    = 1.0f;   // multiplier while aiming down sights (after FOV scaling), 0.3..1.5
    bool  invertY     = false;
    bool  mouseFilter = true;   // drop single-event mouse spikes (see MouseFilter.h)
    Bindings binds;             // every key and button (Bindings.h); right mouse always aims
    // Audio
    float audioVolume = 0.8f;   // 0.0..1.0, everything
    float musicVolume = 0.6f;   // 0.0..1.0, the soundtrack on top of that
    // Gameplay / HUD
    int   difficulty   = DIFFICULTY_DEFAULT;   // see Difficulty.h
    bool  showTimer    = true;  // run clock in Arena mode (FAST mode always shows it)
    bool  ghost        = true;  // FAST: race the ghost of your best run
    bool  damageNumbers = true;
    int   crosshair    = 0;     // colour index, see crosshairColor()
    // Accessibility (Accessibility.h)
    bool  tellsColourblind = false;   // the Absolute's gold tells shown blue
    bool  reduceFlashing   = false;   // full-screen flashes and sky flares dimmed

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

    void save() const { persist::save(kSaveKey, text()); }
    void load() { parse(persist::load(kSaveKey)); }

    std::string text() const {
        std::ostringstream f;
        f << "fov "           << fov           << "\n";
        f << "sensitivity "   << sensitivity   << "\n";
        f << "zoomSens "      << zoomSens      << "\n";
        f << "invertY "       << (invertY ? 1 : 0) << "\n";
        f << "mouseFilter "   << (mouseFilter ? 1 : 0) << "\n";
        for (int a = 0; a < ACTS; ++a)
            f << "bind_" << Bindings::actKey((Act)a) << " " << binds.slot[a][0] << " " << binds.slot[a][1] << "\n";
        f << "audioVolume "   << audioVolume   << "\n";
        f << "musicVolume "   << musicVolume   << "\n";
        f << "frameCap "      << frameCap      << "\n";
        f << "vsync "         << (vsync ? 1 : 0) << "\n";
        f << "fullscreen "    << (fullscreen ? 1 : 0) << "\n";
        f << "quality "       << quality       << "\n";
        f << "antiAlias "     << antiAlias     << "\n";
        f << "showFPS "       << (showFPS   ? 1 : 0) << "\n";
        f << "crtFilter "     << (crtFilter ? 1 : 0) << "\n";
        f << "screenShake "   << screenShake   << "\n";
        f << "viewBob "       << (viewBob ? 1 : 0) << "\n";
        f << "showTimer "     << (showTimer ? 1 : 0) << "\n";
        f << "ghost "         << (ghost ? 1 : 0) << "\n";
        f << "damageNumbers " << (damageNumbers ? 1 : 0) << "\n";
        f << "crosshair "     << crosshair     << "\n";
        f << "tellsColourblind " << (tellsColourblind ? 1 : 0) << "\n";
        f << "reduceFlashing " << (reduceFlashing ? 1 : 0) << "\n";
        f << "difficulty "    << difficulty    << "\n";
        return f.str();
    }

    void parse(const std::string& saved) {
        std::istringstream f(saved);
        std::string key;
        auto flag = [&](bool& b) { int v = 0; f >> v; b = v != 0; };
        bool boundGrapple = false;
        while (f >> key) {
            if      (key == "fov")           f >> fov;
            else if (key == "sensitivity")   f >> sensitivity;
            else if (key == "zoomSens")      f >> zoomSens;
            else if (key == "invertY")       flag(invertY);
            else if (key == "mouseFilter")   flag(mouseFilter);
            else if (key == "grappleKey")    { int g = 0; f >> g; if (!boundGrapple) binds.fromOldGrapple(g); }   // (older saves)
            else if (key.rfind("bind_", 0) == 0) {
                int c0 = 0, c1 = 0; f >> c0 >> c1;
                for (int a = 0; a < ACTS; ++a)
                    if (key == std::string("bind_") + Bindings::actKey((Act)a)) {
                        binds.slot[a][0] = Bindings::valid(c0) ? c0 : Bindings::NONE;
                        binds.slot[a][1] = Bindings::valid(c1) ? c1 : Bindings::NONE;
                        if ((Act)a == Act::GRAPPLE) boundGrapple = true;
                    }
            }
            else if (key == "audioVolume")   f >> audioVolume;
            else if (key == "musicVolume")   f >> musicVolume;
            else if (key == "frameCap")      f >> frameCap;
            else if (key == "vsync")         flag(vsync);
            else if (key == "fullscreen")    flag(fullscreen);
            else if (key == "quality")       f >> quality;
            else if (key == "antiAlias")     f >> antiAlias;
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
            else if (key == "tellsColourblind") flag(tellsColourblind);
            else if (key == "reduceFlashing") flag(reduceFlashing);
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
        antiAlias = (antiAlias % AA_LEVELS + AA_LEVELS) % AA_LEVELS;
        crosshair = (crosshair % CROSSHAIR_COLORS + CROSSHAIR_COLORS) % CROSSHAIR_COLORS;
        if (difficulty < 0 || difficulty >= DIFFICULTY_LEVELS) difficulty = DIFFICULTY_DEFAULT;
        binds.tidy();
        g_tellsColourblind = tellsColourblind; g_reduceFlashing = reduceFlashing;   // (everything else reads these)
    }

    static float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
};
