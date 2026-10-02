#pragma once
// =============================================================================
// Gamepad.h — controller support through SDL's GameController API (an Xbox
// layout; PlayStation and others map onto it, in browsers through the
// Gamepad API). The game reads keys, so the pad is folded into them:
//
//   held (polled every tick):  left stick → W A S D     A → jump (Space)
//                              B → slide / slam (Ctrl)  RB → dash (Shift)
//                              Y → punch / parry (F)    X → reload (R)
//   analog:                    right stick → look       LT → aim   RT → fire
//   presses (handled as events by the states, see GameplayState::padButton):
//                              LB grapple, D-pad left/right weapons, D-pad up
//                              grenade, D-pad down inspect, Start pause,
//                              Back armory; in menus the D-pad and A/B move,
//                              confirm and go back.
// =============================================================================
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace gamepad {

struct State {
    SDL_GameController* pad = nullptr;
    Uint8     held[SDL_NUM_SCANCODES] = {};   // virtual keys the pad is holding
    glm::vec2 look{0.f};                      // right stick after deadzone and curve, -1..1
    bool      aim = false;                    // LT
    bool      fire = false, firePrev = false; // RT, and last tick's (to fire on the press)
};
inline State& state() { static State s; return s; }

// Hot-plugging: main forwards device events here
inline void onDeviceEvent(const SDL_Event& e) {
    State& s = state();
    if (e.type == SDL_CONTROLLERDEVICEADDED && !s.pad && SDL_IsGameController(e.cdevice.which))
        s.pad = SDL_GameControllerOpen(e.cdevice.which);
    if (e.type == SDL_CONTROLLERDEVICEREMOVED && s.pad &&
        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s.pad)) == e.cdevice.which) {
        SDL_GameControllerClose(s.pad);
        s = State{};
    }
}

inline float axis(SDL_GameControllerAxis a) {
    return state().pad ? SDL_GameControllerGetAxis(state().pad, a) / 32767.f : 0.f;
}
inline bool button(SDL_GameControllerButton b) {
    return state().pad && SDL_GameControllerGetButton(state().pad, b);
}

// Read the pad into held keys, look and triggers. Call once per tick.
inline void poll() {
    State& s = state();
    std::memset(s.held, 0, sizeof(s.held));
    s.firePrev = s.fire;
    if (!s.pad) { s.look = {0.f, 0.f}; s.aim = s.fire = false; return; }
    float lx = axis(SDL_CONTROLLER_AXIS_LEFTX), ly = axis(SDL_CONTROLLER_AXIS_LEFTY);
    const float MOVE = 0.35f;
    if (ly < -MOVE) s.held[SDL_SCANCODE_W] = 1;
    if (ly >  MOVE) s.held[SDL_SCANCODE_S] = 1;
    if (lx < -MOVE) s.held[SDL_SCANCODE_A] = 1;
    if (lx >  MOVE) s.held[SDL_SCANCODE_D] = 1;
    if (button(SDL_CONTROLLER_BUTTON_A))             s.held[SDL_SCANCODE_SPACE]  = 1;
    if (button(SDL_CONTROLLER_BUTTON_B))             s.held[SDL_SCANCODE_LCTRL]  = 1;
    if (button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) s.held[SDL_SCANCODE_LSHIFT] = 1;
    if (button(SDL_CONTROLLER_BUTTON_Y))             s.held[SDL_SCANCODE_F]      = 1;
    if (button(SDL_CONTROLLER_BUTTON_X))             s.held[SDL_SCANCODE_R]      = 1;
    // Look: a small deadzone, then a squared curve for fine aim near the centre
    glm::vec2 r{axis(SDL_CONTROLLER_AXIS_RIGHTX), axis(SDL_CONTROLLER_AXIS_RIGHTY)};
    float m = glm::length(r);
    const float DEAD = 0.14f;
    if (m < DEAD) s.look = {0.f, 0.f};
    else { float k = (std::min(m, 1.f) - DEAD) / (1.f - DEAD); s.look = r / m * (k * k); }
    s.aim  = axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT)  > 0.3f;
    s.fire = axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 0.3f;
}

// A key event standing in for a pad press (menus read keys)
inline SDL_Event keyEvent(SDL_Keycode k) {
    SDL_Event e;
    std::memset(&e, 0, sizeof(e));
    e.type = SDL_KEYDOWN;
    e.key.state = SDL_PRESSED;
    e.key.keysym.sym = k;
    e.key.keysym.scancode = SDL_GetScancodeFromKey(k);
    return e;
}

// Menu navigation for a pad press, or SDLK_UNKNOWN
inline SDL_Keycode menuKey(Uint8 b, bool backIsEscape = true) {
    switch (b) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    return SDLK_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  return SDLK_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  return SDLK_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return SDLK_RIGHT;
        case SDL_CONTROLLER_BUTTON_A:          return SDLK_RETURN;
        case SDL_CONTROLLER_BUTTON_B:          return backIsEscape ? SDLK_ESCAPE : SDLK_UNKNOWN;
        case SDL_CONTROLLER_BUTTON_START:      return backIsEscape ? SDLK_ESCAPE : SDLK_RETURN;
        default:                               return SDLK_UNKNOWN;
    }
}

} // namespace gamepad
