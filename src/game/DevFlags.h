#pragma once
// =============================================================================
// DevFlags.h — what the command line (main.cpp) or the web URL (web/index.html)
// asks of a run: where it starts, god mode, a BOSS run, and the dev and
// footage tools (a fixed camera, poses held for screenshots, recordings).
// See "Dev tools" in the README for every flag.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include "player/Sword.h"

// Set from the command line (--arena N) or the web URL (?arena=N) to start a
// run at a later arena — handy for testing and for showing off the boss.
inline int g_startArena = 0;
inline int g_startWave  = 0;   // --wave N / ?wave=N (with --arena): skip to that wave
// --god / ?god: the player takes no damage (for recording footage)
inline bool g_godMode = false;
inline bool g_practice = false;   // dev level select: no records, no leaderboard
inline int  g_bossRun = -1;       // BOSS (the main menu): which boss this run is, fought on its own (-1: not one)
inline int  g_roomAttack = -1;    // FAST's ROOM ATTACK (the FAST page, --room N): one room on its own, for its medal (-1: the whole run)
// --cam x y z yaw pitch: start the camera somewhere specific (screenshots)
inline bool      g_devCam = false;
inline glm::vec3 g_devCamPos{0.f};
inline float     g_devCamYaw = -90.f, g_devCamPitch = 0.f;
// --weapon N --aim --overlay armory|pause|settings: pose a screenshot
inline int       g_devWeapon = -1;
inline bool      g_devSword = false;   // --sword / ?sword: the sword (key 5) anywhere, not just the Zenith
inline bool      g_devAim = false;
inline std::string g_devOverlay;
// --skip sky,world,boxes,water,fx,viewmodel,bloom,hud: leave render passes out (finding what the GPU spends)
inline std::string g_devSkip;
inline int g_devAA = -1;   // --aa N: MSAA samples whatever the settings say (0 = off)
inline bool devSkip(const char* pass) {
    if (g_devSkip.empty()) return false;
    std::string list = "," + g_devSkip + ",";
    return list.find("," + std::string(pass) + ",") != std::string::npos;
}
inline bool      g_devNoMouse = false;   // screenshot runs: never grab or read the mouse
inline int g_devHollow = 0;            // --hollow N: --spawn enemies get Hollow variant N (1 ENRAGED, 2 TWINNED, 3 HALOED)
inline std::vector<int> g_devSpawns;    // --spawn N (repeatable): enemies of type N in an arc 9-12 m in front of the camera
// Footage (--record, see main.cpp): every frame advances a fixed step, the
// camera glides from --cam to --campath, --autoaim turns it onto the nearest
// enemy in sight and fires, --clean hides title cards and control hints
inline double    g_fixedDt = 0.0;
inline bool      g_devCamPath = false;
inline glm::vec3 g_devCamPos2{0.f};
inline float     g_devCamYaw2 = -90.f, g_devCamPitch2 = 0.f;
inline float     g_recProgress = 0.f;    // 0..1 through the recording
inline bool      g_devAutoAim = false;
inline bool      g_devFpsLog = false;    // --fps HZ: the web build logs the rate its loop runs at
inline bool      g_devClean = false;
inline bool      g_devKite = false;      // --kite: the camera backs off from whatever gets close
// What the view model shows: the sword, or (dev: --overlay swingSU) stroke S frozen at U/10, for screenshots
inline Sword swordShownFor(const Sword& s) {
    if (g_devOverlay == "guard") { Sword f = s; f.raised = true; f.raisedFor = 1.f; return f; }
    if (g_devOverlay.rfind("swing", 0) != 0 || g_devOverlay.size() < 7) return s;
    Sword f = s; f.lastStroke = g_devOverlay[5] - '0';
    f.swingT = std::max(1e-4f, Sword::SWING_TIME * (1.f - (g_devOverlay[6] - '0') / 10.f)); f.sinceSwing = 0.f;
    return f;
}
