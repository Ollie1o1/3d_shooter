#pragma once
#include "Persist.h"
#include <sstream>
#include <string>

// Every option on the settings page (main menu and pause menu share it).
// Saved as "key value" lines through Persist.h: a file on desktop, the
// browser's localStorage on the web.
struct GameSettings {
    // Video
    float fov         = 90.f;   // 60..120
    int   fpsCap      = 0;      // 0=uncapped,1=60,2=144,3=180,4=240
    bool  showFPS     = false;
    bool  crtFilter   = false;  // CRT post-process effect
    float screenShake = 1.0f;   // 0..1
    bool  viewBob     = true;
    // Controls
    float sensitivity = 0.10f;  // degrees per mouse count, 0.01..1.00
    float zoomSens    = 1.0f;   // multiplier while aiming down sights (after FOV scaling), 0.3..1.5
    bool  invertY     = false;
    bool  mouseFilter = true;   // drop single-event mouse spikes (see MouseFilter.h)
    // Audio
    float audioVolume = 0.8f;   // 0.0..1.0
    // Gameplay / HUD
    bool  showTimer    = true;  // run clock in Arena mode (FAST mode always shows it)
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

    int getFPSCapValue() const {
        static const int caps[] = {0,60,144,180,240};
        return (fpsCap >= 0 && fpsCap <= 4) ? caps[fpsCap] : 0;
    }
    const char* getFPSCapLabel() const {
        switch (fpsCap) {
            case 1: return "60";
            case 2: return "144";
            case 3: return "180";
            case 4: return "240";
            default: return "UNCAPPED";
        }
    }

    static constexpr const char* kSaveKey = "settings.cfg";

    void save() const {
        std::ostringstream f;
        f << "fov "           << fov           << "\n";
        f << "sensitivity "   << sensitivity   << "\n";
        f << "zoomSens "      << zoomSens      << "\n";
        f << "invertY "       << (invertY ? 1 : 0) << "\n";
        f << "mouseFilter "   << (mouseFilter ? 1 : 0) << "\n";
        f << "audioVolume "   << audioVolume   << "\n";
        f << "fpsCap "        << fpsCap        << "\n";
        f << "showFPS "       << (showFPS   ? 1 : 0) << "\n";
        f << "crtFilter "     << (crtFilter ? 1 : 0) << "\n";
        f << "screenShake "   << screenShake   << "\n";
        f << "viewBob "       << (viewBob ? 1 : 0) << "\n";
        f << "showTimer "     << (showTimer ? 1 : 0) << "\n";
        f << "damageNumbers " << (damageNumbers ? 1 : 0) << "\n";
        f << "crosshair "     << crosshair     << "\n";
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
            else if (key == "audioVolume")   f >> audioVolume;
            else if (key == "fpsCap")        f >> fpsCap;
            else if (key == "showFPS")       flag(showFPS);
            else if (key == "crtFilter")     flag(crtFilter);
            else if (key == "screenShake")   f >> screenShake;
            else if (key == "viewBob")       flag(viewBob);
            else if (key == "showTimer")     flag(showTimer);
            else if (key == "damageNumbers") flag(damageNumbers);
            else if (key == "crosshair")     f >> crosshair;
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
        screenShake = clampf(screenShake, 0.f, 1.f);
        if (fpsCap < 0 || fpsCap > 4) fpsCap = 0;
        crosshair = (crosshair % CROSSHAIR_COLORS + CROSSHAIR_COLORS) % CROSSHAIR_COLORS;
    }

    static float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
};
