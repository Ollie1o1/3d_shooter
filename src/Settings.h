#pragma once

struct GameSettings {
    float fov         = 90.f;   // 60..120
    float sensitivity = 0.10f;  // 0.02..0.50
    float audioVolume = 1.0f;   // 0.0..1.0
    int   fpsCap      = 0;      // 0=uncapped,1=60,2=144,3=180,4=240
    bool  showFPS     = false;

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
};
