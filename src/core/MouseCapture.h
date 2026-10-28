#pragma once
// =============================================================================
// MouseCapture.h — the one way the game takes or frees the mouse.
//
// On the web it also tells the page (web/index.html) whether the game wants the
// mouse right now. The browser only grants a pointer lock inside a click or a
// key press, so a request made between them is held until the next one; by
// then the game may have moved on to a menu or the armory, and the lock would
// land on a screen that needs a free cursor (with Escape kept by fullscreen,
// there was no way out but leaving fullscreen). Told what the game wants, the
// page drops a request that comes too late and lets go of a lock nobody wants.
// =============================================================================
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

inline void setMouseCaptured(bool on) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ if (window.overdriveMouseWanted) window.overdriveMouseWanted(!!$0); }, on ? 1 : 0);
#endif
    SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE);
}
