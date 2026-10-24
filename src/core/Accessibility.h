#pragma once
// =============================================================================
// Accessibility.h — the options that change how things look everywhere, as
// flags the renderer, the HUD and the bosses read. The settings set them
// (GameSettings::clampAll, on load and on every change).
// =============================================================================

// TELL COLOURS: COLOUR-BLIND. The Absolute's gold tells turn blue (red and
// gold are the pair red-green colour blindness mixes up). See abGoldTell.
inline bool g_tellsColourblind = false;

// REDUCE FLASHING: full-screen flashes, the eclipse's flares and explosion
// light dimmed to a fraction (the events still show, just not as a flash).
inline bool g_reduceFlashing = false;
inline float flashScale() { return g_reduceFlashing ? 0.25f : 1.f; }
