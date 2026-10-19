#pragma once
// =============================================================================
// MouseFilter.h — drops the bogus mouse deltas that spin the camera around.
//
// Two sources of "the view suddenly snapped 180°":
//   * Chrome (and some other browsers) occasionally report one enormous
//     movementX/Y under pointer lock, especially with high-polling-rate mice.
//     The web shell also asks for raw "unadjustedMovement", which avoids most
//     of these, but not every browser/OS supports it.
//   * The first motion event after (re)capturing the mouse can carry the
//     whole distance the cursor travelled while it was free (pause menu, alt-
//     tab, clicking back into the browser tab).
//
// Real hand movement ramps up over a few events; the glitch is one isolated
// event far larger than its neighbours. So: skip the first event or two after
// a capture, and drop a single event that is both large and many times the
// previous one. Two big events in a row are a genuine flick and go through.
// =============================================================================
#include <cmath>

struct MouseFilter {
    bool  enabled   = true;
    int   skip      = 0;      // events to ignore (after capturing the mouse)
    float prevMag   = 0.f;    // magnitude of the last accepted event
    bool  lastDropped = false;

    static constexpr float SPIKE_MIN   = 180.f;   // never drop anything smaller (pixels)
    static constexpr float SPIKE_RATIO = 7.f;     // ...or anything less than this × the previous event

    void onCapture() { skip = 2; prevMag = 0.f; lastDropped = false; }

    // True if (dx, dy) should be applied to the camera.
    bool accept(float dx, float dy) {
        float m = std::sqrt(dx * dx + dy * dy);
        if (skip > 0) { --skip; return false; }
        if (!enabled) { prevMag = m; return true; }
        bool spike = m > SPIKE_MIN && m > SPIKE_RATIO * (prevMag + 10.f);
        if (spike && !lastDropped) {
            lastDropped = true;
            prevMag = m * 0.5f;   // if the next one is big too, it's real: let it through
            return false;
        }
        lastDropped = false;
        prevMag = m;
        return true;
    }
};
