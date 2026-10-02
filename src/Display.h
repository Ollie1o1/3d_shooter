#pragma once
// =============================================================================
// Display.h — where a frame ends up.
//
// Everything is drawn into a 16:9 offscreen canvas at the render resolution;
// present() scales that into the window, letterboxed, whatever size the window
// or screen is. The HUD and menus keep laying themselves out in a virtual
// 1280x720 (UIBatch's projection stretches it over the canvas), so going
// fullscreen at 1440p only changes how many pixels the 3D scene gets.
//
//   game render ──▶ PostProcess scene FBO ──▶ canvas (render res) ──▶ window
//                                              + HUD / menus          (scaled)
// =============================================================================
#include "gl.h"
#include <algorithm>

namespace display {

constexpr int VIRTUAL_W = 1280, VIRTUAL_H = 720;   // the UI's coordinate space

struct Canvas { GLuint fbo = 0, color = 0, depth = 0; int w = 0, h = 0; };
inline Canvas& canvas() { static Canvas c; return c; }
inline int renderW() { return canvas().w > 0 ? canvas().w : VIRTUAL_W; }
inline int renderH() { return canvas().h > 0 ? canvas().h : VIRTUAL_H; }

// The largest 16:9 rectangle that fits in W x H, centred
struct Rect { int x, y, w, h; };
inline Rect fit(int W, int H) {
    int w = W, h = W * 9 / 16;
    if (h > H) { h = H; w = H * 16 / 9; }
    return {(W - w) / 2, (H - h) / 2, std::max(1, w), std::max(1, h)};
}

// (Re)create the canvas at w x h if it isn't that size already
inline void ensure(int w, int h) {
    Canvas& c = canvas();
    w = std::max(320, w); h = std::max(180, h);
    if (c.fbo && c.w == w && c.h == h) return;
    if (!c.fbo) { glGenFramebuffers(1, &c.fbo); glGenTextures(1, &c.color); glGenRenderbuffers(1, &c.depth); }
    c.w = w; c.h = h;
    glBindTexture(GL_TEXTURE_2D, c.color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindRenderbuffer(GL_RENDERBUFFER, c.depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, c.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, c.color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, c.depth);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Draw into the canvas from here on
inline void bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, canvas().fbo);
    glViewport(0, 0, renderW(), renderH());
}

// Scale the canvas into the window's drawable area (black bars if the
// window isn't 16:9)
inline void present(int drawW, int drawH) {
    Canvas& c = canvas();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, drawW, drawH);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    Rect r = fit(drawW, drawH);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, c.fbo);
    glBlitFramebuffer(0, 0, c.w, c.h, r.x, r.y, r.x + r.w, r.y + r.h, GL_COLOR_BUFFER_BIT,
                      (c.w == r.w && c.h == r.h) ? GL_NEAREST : GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// A window-space point (SDL mouse event, in window points) to the virtual 1280x720
inline void toVirtual(int winW, int winH, int& x, int& y) {
    Rect r = fit(winW, winH);
    x = (x - r.x) * VIRTUAL_W / r.w;
    y = (y - r.y) * VIRTUAL_H / r.h;
}

} // namespace display
