#pragma once
#include <fstream>
#include <sstream>
#include <string>

struct GameSettings {
    float fov         = 90.f;   // 60..120
    float sensitivity = 0.10f;  // 0.02..0.50
    float audioVolume = 1.0f;   // 0.0..1.0
    int   fpsCap      = 0;      // 0=uncapped,1=60,2=144,3=180,4=240
    bool  showFPS     = false;
    bool  crtFilter   = false;  // CRT post-process effect

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

    static constexpr const char* kSavePath = "settings.cfg";

    void save() const {
        std::ofstream f(kSavePath, std::ios::trunc);
        if (!f) return;
        f << "fov "         << fov         << "\n";
        f << "sensitivity " << sensitivity << "\n";
        f << "audioVolume " << audioVolume << "\n";
        f << "fpsCap "      << fpsCap      << "\n";
        f << "showFPS "     << (showFPS   ? 1 : 0) << "\n";
        f << "crtFilter "   << (crtFilter ? 1 : 0) << "\n";
    }

    void load() {
        std::ifstream f(kSavePath);
        if (!f) return; // no saved settings yet — keep defaults
        std::string key;
        while (f >> key) {
            if      (key == "fov")         f >> fov;
            else if (key == "sensitivity") f >> sensitivity;
            else if (key == "audioVolume") f >> audioVolume;
            else if (key == "fpsCap")      f >> fpsCap;
            else if (key == "showFPS")     { int v; f >> v; showFPS   = v != 0; }
            else if (key == "crtFilter")   { int v; f >> v; crtFilter = v != 0; }
            else { std::string skip; f >> skip; } // unknown key — skip its value
        }
        fov         = glm_clampf(fov, 60.f, 120.f);
        sensitivity = glm_clampf(sensitivity, 0.02f, 0.50f);
        audioVolume = glm_clampf(audioVolume, 0.f, 1.f);
        if (fpsCap < 0 || fpsCap > 4) fpsCap = 0;
    }

private:
    static float glm_clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
};
