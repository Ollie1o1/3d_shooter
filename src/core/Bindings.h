#pragma once
// =============================================================================
// Bindings.h — which keys (or mouse buttons) do what. Two slots per action.
//
// A binding is a physical key (an SDL scancode, so the layout doesn't move
// it), or MOUSE + an SDL mouse button. The game itself still reads its
// default keys: once a frame, remap() folds whatever you've bound into the
// default key of each action, so the movement code, the gamepad (which holds
// the same default keys) and the tests don't need to know about any of this.
// One-shot actions (grapple, grenade, inspect, interact) come from events:
// pressed() says whether an event presses one.
// =============================================================================
#include <SDL2/SDL.h>
#include <cctype>
#include <cstring>
#include <string>

enum class Act { FORWARD, BACK, LEFT, RIGHT, JUMP, DASH, SLIDE, PARRY, GRAPPLE, RELOAD, GRENADE, INSPECT, INTERACT, COUNT };
inline constexpr int ACTS = (int)Act::COUNT;

struct Bindings {
    static constexpr int NONE = 0, MOUSE = 1000;   // (scancode 0 is SDL_SCANCODE_UNKNOWN)
    int slot[ACTS][2];

    Bindings() { reset(); }

    // The default keys, which are also the keys the game reads
    static int canonical(Act a) {
        static const int C[ACTS] = {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_SPACE,
                                    SDL_SCANCODE_LSHIFT, SDL_SCANCODE_LCTRL, SDL_SCANCODE_F, SDL_SCANCODE_Q, SDL_SCANCODE_R,
                                    SDL_SCANCODE_G, SDL_SCANCODE_V, SDL_SCANCODE_E};
        return C[(int)a];
    }
    void reset() {
        for (int i = 0; i < ACTS; ++i) { slot[i][0] = canonical((Act)i); slot[i][1] = NONE; }
        slot[(int)Act::SLIDE][1] = SDL_SCANCODE_C;
    }
    static const char* actName(Act a) {
        static const char* N[ACTS] = {"FORWARD", "BACK", "LEFT", "RIGHT", "JUMP", "DASH", "SLIDE / SLAM", "PARRY",
                                      "GRAPPLE", "RELOAD", "GRENADE", "INSPECT", "INTERACT"};
        return N[(int)a];
    }
    // The save file's word for it (no spaces)
    static const char* actKey(Act a) {
        static const char* N[ACTS] = {"forward", "back", "left", "right", "jump", "dash", "slide", "parry",
                                      "grapple", "reload", "grenade", "inspect", "interact"};
        return N[(int)a];
    }

    // Keys and buttons the game keeps for itself: menus, weapons, fire and aim
    static bool reserved(int code) {
        if (code == MOUSE + SDL_BUTTON_LEFT || code == MOUSE + SDL_BUTTON_RIGHT) return true;
        switch (code) {
            case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_TAB: case SDL_SCANCODE_P: case SDL_SCANCODE_BACKSPACE:
            case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: case SDL_SCANCODE_F5: case SDL_SCANCODE_F6: case SDL_SCANCODE_F11:
            case SDL_SCANCODE_1: case SDL_SCANCODE_2: case SDL_SCANCODE_3: case SDL_SCANCODE_4: case SDL_SCANCODE_5:
            case SDL_SCANCODE_UP: case SDL_SCANCODE_DOWN: case SDL_SCANCODE_LEFT: case SDL_SCANCODE_RIGHT:
                return true;
            default: return false;
        }
    }
    static bool valid(int code) {
        if (code >= MOUSE) return code > MOUSE && code <= MOUSE + 8;
        return code > 0 && code < SDL_NUM_SCANCODES;
    }

    // Bind `code` to an action's slot. It leaves any other place it was bound
    // (one key, one job); an action left with only its second key moves it up.
    bool bind(Act a, int s, int code) {
        if (!valid(code) || reserved(code)) return false;
        for (int i = 0; i < ACTS; ++i)
            for (int k = 0; k < 2; ++k)
                if (slot[i][k] == code) slot[i][k] = NONE;
        slot[(int)a][s] = code;
        tidy();
        return true;
    }
    void clear(Act a, int s) { slot[(int)a][s] = NONE; tidy(); }
    void tidy() {
        for (int i = 0; i < ACTS; ++i)
            if (slot[i][0] == NONE && slot[i][1] != NONE) { slot[i][0] = slot[i][1]; slot[i][1] = NONE; }
    }

    // What the code is called on screen, as the keyboard's layout labels it
    static std::string codeName(int code) {
        if (code == NONE) return "---";
        if (code >= MOUSE) {
            switch (code - MOUSE) {
                case SDL_BUTTON_MIDDLE: return "MIDDLE MOUSE";
                case SDL_BUTTON_X1:     return "MOUSE 4";
                case SDL_BUTTON_X2:     return "MOUSE 5";
                default:                return "MOUSE " + std::to_string(code - MOUSE);
            }
        }
        const char* n = SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)code));
        std::string s = n ? n : "";
        for (char c : s) if ((unsigned char)c < 32 || (unsigned char)c > 126) { s.clear(); break; }   // (the font is ASCII)
        if (s.empty()) { const char* sn = SDL_GetScancodeName((SDL_Scancode)code); s = sn && *sn ? sn : "KEY " + std::to_string(code); }
        for (char& c : s) c = (char)std::toupper((unsigned char)c);
        return s;
    }
    std::string label(Act a) const {
        std::string s = codeName(slot[(int)a][0]);
        if (slot[(int)a][1] != NONE) s += " / " + codeName(slot[(int)a][1]);
        return s;
    }
    // The code an event presses, or NONE
    static int codeOf(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN && !e.key.repeat) return e.key.keysym.scancode;
        if (e.type == SDL_MOUSEBUTTONDOWN) return MOUSE + e.button.button;
        return NONE;
    }
    bool pressed(Act a, const SDL_Event& e) const {
        int c = codeOf(e);
        return c != NONE && (slot[(int)a][0] == c || slot[(int)a][1] == c);
    }
    static bool down(int code, const Uint8* keys, Uint32 mouse) {
        if (code == NONE) return false;
        if (code >= MOUSE) return (mouse & SDL_BUTTON(code - MOUSE)) != 0;
        return keys[code] != 0;
    }

    // The keyboard as the game reads it: each action's default key held when
    // any key bound to it is. Keys bound to nothing read as up.
    void remap(const Uint8* keys, Uint32 mouse, Uint8* out) const {
        std::memset(out, 0, SDL_NUM_SCANCODES);
        for (int i = 0; i < ACTS; ++i)
            if (down(slot[i][0], keys, mouse) || down(slot[i][1], keys, mouse)) out[canonical((Act)i)] = 1;
    }

    // Older saves only chose the grapple's button from a list (E stays INTERACT
    // as well, as it was then)
    void fromOldGrapple(int i) {
        static const int OLD[5] = {SDL_SCANCODE_Q, SDL_SCANCODE_E, MOUSE + SDL_BUTTON_X1, MOUSE + SDL_BUTTON_X2, MOUSE + SDL_BUTTON_MIDDLE};
        if (i >= 0 && i < 5) slot[(int)Act::GRAPPLE][0] = OLD[i];
    }
};
