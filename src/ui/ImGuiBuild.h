#pragma once
// =============================================================================
// ImGuiBuild.h — Dear ImGui (src/third_party/imgui, 1.91.9, MIT) compiled into
// the game's one translation unit, with its SDL2 and OpenGL 3 backends. The GL
// backend uses the game's own GL headers (core/gl.h): Apple's framework on
// macOS, GLEW on Linux and Windows, GLES 3 (WebGL2) in the browser.
// Used only by the balance panel (ui/BalancePanel.h).
// =============================================================================
#include "core/gl.h"
#ifdef __EMSCRIPTEN__
#define IMGUI_IMPL_OPENGL_ES3   // WebGL2, like the game (ImGui would pick WebGL 1 on its own)
#else
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include "third_party/imgui/imgui.cpp"
#include "third_party/imgui/imgui_draw.cpp"
#include "third_party/imgui/imgui_tables.cpp"
#include "third_party/imgui/imgui_widgets.cpp"
#include "third_party/imgui/backends/imgui_impl_sdl2.cpp"
#include "third_party/imgui/backends/imgui_impl_opengl3.cpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
// On Linux the SDL backend pulls in X11 (SDL_syswm.h), which #defines everyday
// words: None (main.cpp's NextState::None), Bool, Status, True, False... Gone
// again here, so nothing after this sees them.
#undef None
#undef Bool
#undef Status
#undef Success
#undef True
#undef False
#undef Always
#undef Above
#undef Below
#undef CurrentTime
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef Expose
#undef Complex
#undef Convex
