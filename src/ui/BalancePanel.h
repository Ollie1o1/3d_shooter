#pragma once
// =============================================================================
// BalancePanel.h — F1: the game's numbers on sliders (core/Balance.h), live.
// Opening it freezes the game and frees the mouse; F1 again and you're back
// in, with the new numbers. SAVE writes what you changed to assets/balance.txt
// (the game and the tests load it at startup); REVERT goes back to that file;
// DEFAULTS back to the code's. Ctrl+click a slider to type a value.
// Desktop builds only: the web build leaves Dear ImGui out (~190 KB more to
// download for a tool no visitor uses), though it still loads balance.txt, so
// saved numbers ship there too. `make web WEB_FLAGS=-DWEB_BALANCE_PANEL` puts
// the panel in (opened with ?balance; it can't save from a browser).
// =============================================================================
#include "core/Balance.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#if !defined(__EMSCRIPTEN__) || defined(WEB_BALANCE_PANEL)
#define OVERDRIVE_BALANCE_PANEL 1
#include "ui/ImGuiBuild.h"
#endif

#ifndef OVERDRIVE_BALANCE_PANEL
// (the web build: the same calls, doing nothing)
inline bool g_balanceAllowed = false;
namespace balancepanel {
struct State { char filter[64] = {}; };
inline State& st() { static State s; return s; }
inline void init(SDL_Window*, SDL_GLContext) {}
inline bool handleEvent(const SDL_Event&) { return false; }
inline void draw(unsigned, int, int) {}
}
#else

#ifdef __EMSCRIPTEN__
inline bool g_balanceAllowed = false;   // ?balance
#else
inline bool g_balanceAllowed = true;
#endif

namespace balancepanel {

struct State {
    bool ready = false;
    char filter[64] = {};
    std::string status;
    double statusUntil = 0.0;
    std::vector<std::string> problems;   // from the last load
};
inline State& st() { static State s; return s; }

inline void say(const std::string& s) { st().status = s; st().statusUntil = ImGui::GetTime() + 4.0; }

inline void init(SDL_Window* window, SDL_GLContext ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // (no imgui.ini dropped in the repo)
    int ww = 1, wh = 1, dw = 1, dh = 1;
    SDL_GetWindowSize(window, &ww, &wh);
    SDL_GL_GetDrawableSize(window, &dw, &dh);
    float dpi = std::max(1.f, (float)dw / (float)std::max(1, ww));
    ImFontConfig fc; fc.SizePixels = 15.f * dpi;   // rasterised at the screen's real pixels, drawn at its point size: sharp on retina
    io.Fonts->AddFontDefault(&fc);
    io.FontGlobalScale = 1.f / dpi;
    // The arsenal's look: blackened metal, an amber accent
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 2.f; s.FrameRounding = 2.f; s.GrabRounding = 2.f; s.WindowBorderSize = 1.f;
    ImVec4* c = s.Colors;
    const ImVec4 amber{1.f, 0.68f, 0.22f, 1.f}, amberDim{0.55f, 0.36f, 0.12f, 1.f};
    c[ImGuiCol_WindowBg]         = {0.06f, 0.06f, 0.08f, 0.94f};
    c[ImGuiCol_TitleBg]          = {0.10f, 0.08f, 0.06f, 1.f};
    c[ImGuiCol_TitleBgActive]    = {0.22f, 0.14f, 0.05f, 1.f};
    c[ImGuiCol_Header]           = {0.20f, 0.14f, 0.07f, 1.f};
    c[ImGuiCol_HeaderHovered]    = {0.32f, 0.22f, 0.09f, 1.f};
    c[ImGuiCol_HeaderActive]     = amberDim;
    c[ImGuiCol_FrameBg]          = {0.13f, 0.13f, 0.15f, 1.f};
    c[ImGuiCol_FrameBgHovered]   = {0.20f, 0.17f, 0.13f, 1.f};
    c[ImGuiCol_SliderGrab]       = amberDim;
    c[ImGuiCol_SliderGrabActive] = amber;
    c[ImGuiCol_Button]           = {0.20f, 0.14f, 0.07f, 1.f};
    c[ImGuiCol_ButtonHovered]    = {0.34f, 0.23f, 0.09f, 1.f};
    c[ImGuiCol_ButtonActive]     = amberDim;
    c[ImGuiCol_CheckMark]        = amber;
    ImGui_ImplSDL2_InitForOpenGL(window, ctx);
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330 core");
#endif
    st().ready = true;
}

// Input while it's open goes to the panel (true: the game shouldn't see it)
inline bool handleEvent(const SDL_Event& e) {
    if (!g_balanceOpen || !st().ready) return false;
    ImGui_ImplSDL2_ProcessEvent(&e);
    switch (e.type) {
        case SDL_KEYDOWN: case SDL_KEYUP: case SDL_TEXTINPUT: case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: case SDL_MOUSEWHEEL: case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP: case SDL_CONTROLLERAXISMOTION:
            return true;
        default: return false;
    }
}

inline bool matches(const BalanceVar& b, const char* f) {
    if (!f[0]) return true;
    auto low = [](std::string s) { for (auto& ch : s) ch = (char)std::tolower((unsigned char)ch); return s; };
    std::string q = low(f);
    return low(b.key).find(q) != std::string::npos || low(b.label).find(q) != std::string::npos || low(b.owner).find(q) != std::string::npos;
}

inline void build(float screenH) {
    State& S = st();
    auto& vars = balanceVars();
    int changed = 0;
    for (auto& b : vars) changed += b.changed();
    ImGui::SetNextWindowPos({16.f, 16.f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({470.f, std::max(300.f, screenH - 32.f)}, ImGuiCond_FirstUseEver);
    ImGui::Begin("BALANCE   (F1 to close and play)", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::TextWrapped("Changes apply as you drag (enemy health from the next spawn, magazines from the next reload). "
                       "Ctrl+click a slider to type a value.");
    ImGui::TextColored(changed ? ImVec4{1.f, 0.68f, 0.22f, 1.f} : ImVec4{0.6f, 0.6f, 0.65f, 1.f},
                       "%d changed from the code's defaults", changed);
#ifdef __EMSCRIPTEN__
    ImGui::BeginDisabled();
    ImGui::Button("SAVE");
    ImGui::EndDisabled();
    ImGui::SameLine(); ImGui::TextDisabled("(the browser can't save: it lasts until you reload)");
#else
    if (ImGui::Button("SAVE")) say(balanceSave() ? std::string("saved ") + BALANCE_PATH : std::string("couldn't write ") + BALANCE_PATH);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Write what's changed to %s (commit it to keep it)", BALANCE_PATH);
    ImGui::SameLine();
    if (ImGui::Button("REVERT")) {
        S.problems.clear();
        if (balanceLoad(BALANCE_PATH, &S.problems)) say("back to what's in the file");
        else { balanceResetAll(); say("no file yet: back to the defaults"); }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Undo everything since the last save");
#endif
    ImGui::SameLine();
    if (ImGui::Button("DEFAULTS")) { balanceResetAll(); say("everything back to the code's defaults (not saved)"); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Every value back to the code's default (SAVE to make it stick)");
    if (!S.status.empty() && ImGui::GetTime() < S.statusUntil) ImGui::TextColored({0.5f, 1.f, 0.6f, 1.f}, "%s", S.status.c_str());
    for (auto& p : S.problems) ImGui::TextColored({1.f, 0.45f, 0.4f, 1.f}, "%s", p.c_str());
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputTextWithHint("##filter", "search: damage, husk, overdrive...", S.filter, sizeof S.filter);
    ImGui::Separator();

    ImGui::BeginChild("vars");
    const bool filtering = S.filter[0] != 0;
    size_t i = 0;
    while (i < vars.size()) {
        const std::string group = vars[i].group;
        size_t gEnd = i; while (gEnd < vars.size() && vars[gEnd].group == group) ++gEnd;
        bool any = false; int gChanged = 0;
        for (size_t k = i; k < gEnd; ++k) { any |= matches(vars[k], S.filter); gChanged += vars[k].changed(); }
        if (!any) { i = gEnd; continue; }
        std::string title = group + (gChanged ? "  (" + std::to_string(gChanged) + " changed)" : "") + "###" + group;
        if (filtering) ImGui::SetNextItemOpen(true);
        if (ImGui::CollapsingHeader(title.c_str())) {
            size_t j = i;
            while (j < gEnd) {
                const std::string owner = vars[j].owner;
                size_t oEnd = j; while (oEnd < gEnd && vars[oEnd].owner == owner) ++oEnd;
                bool oAny = false; int oChanged = 0;
                for (size_t k = j; k < oEnd; ++k) { oAny |= matches(vars[k], S.filter); oChanged += vars[k].changed(); }
                if (oAny) {
                    std::string node = owner + (oChanged ? "  *" : "") + "###" + group + owner;
                    if (filtering) ImGui::SetNextItemOpen(true);
                    if (ImGui::TreeNode(node.c_str())) {
                        for (size_t k = j; k < oEnd; ++k) {
                            BalanceVar& b = vars[k];
                            if (!matches(b, S.filter)) continue;
                            ImGui::PushID(b.key.c_str());
                            bool ch = b.changed();
                            if (ch) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.f, 0.72f, 0.3f, 1.f});
                            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.58f);
                            if (b.f) {
                                float v = *b.f;
                                if (ImGui::SliderFloat(b.label.c_str(), &v, b.lo, b.hi, "%.3g")) b.set(v);
                            } else {
                                int v = *b.i;
                                if (ImGui::SliderInt(b.label.c_str(), &v, (int)b.lo, (int)b.hi)) b.set((float)v);
                            }
                            if (ch) ImGui::PopStyleColor();
                            if (ImGui::IsItemHovered()) {
                                if (b.i) ImGui::SetTooltip("%s\n%s\ndefault %d", b.key.c_str(), b.help, (int)b.def);
                                else     ImGui::SetTooltip("%s\n%s\ndefault %g", b.key.c_str(), b.help, b.def);
                            }
                            if (ch) {
                                ImGui::SameLine();
                                if (ImGui::SmallButton("reset")) b.set(b.def);
                            }
                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                }
                j = oEnd;
            }
        }
        i = gEnd;
    }
    ImGui::EndChild();
    ImGui::End();
}

// Draw it over the frame: into framebuffer `fbo`, `w` x `h` pixels
inline void draw(GLuint fbo, int w, int h) {
    if (!g_balanceOpen || !st().ready) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    build(ImGui::GetIO().DisplaySize.y);
    ImGui::Render();
    ImDrawData* dd = ImGui::GetDrawData();
    if (dd->DisplaySize.x > 0.f && dd->DisplaySize.y > 0.f) dd->FramebufferScale = {(float)w / dd->DisplaySize.x, (float)h / dd->DisplaySize.y};
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
    ImGui_ImplOpenGL3_RenderDrawData(dd);
}

} // namespace balancepanel
#endif
