#pragma once
#include "core/Collision.h"

// A surface answers with its own material, including each chapter's restyles.
inline const char* surfaceImpactSound(Mat m) {
    switch (m) {
        case Mat::METAL: case Mat::PANEL: case Mat::PLATE: case Mat::CIRCUIT:
        case Mat::BRASS: case Mat::HULL: case Mat::PAINTED: case Mat::CORRUGATED:
        case Mat::FROSTSTEEL: case Mat::SURGICAL: case Mat::SALVAGE:
            return "impact_metal";
        case Mat::WOOD: return "impact_wood";
        case Mat::ICE: return "impact_ice";
        default: return "impact";
    }
}
