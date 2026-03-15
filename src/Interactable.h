#pragma once
// =============================================================================
// Interactable.h — Objects the player can press E to activate.
//
// HOW TO ADD AN INTERACTABLE IN GameplayState:
//   Interactable it;
//   it.position = {x, y, z};
//   it.box      = { {x-0.4f, y, z-0.4f}, {x+0.4f, y+1.5f, z+0.4f} };
//   it.type     = InteractableType::BUTTON;
//   it.label    = "Open Gate";
//   it.onInteract = [&]() { /* do something */ };
//   interactables.push_back(std::move(it));
//
// Player presses E. GameplayState raycasts forward, finds the closest
// interactable whose AABB the ray hits within INTERACT_RANGE. If found,
// onInteract() is called and the crosshair turns gold.
// =============================================================================
#include "Player.h"          // for AABB
#include <glm/glm.hpp>
#include <functional>
#include <string>

enum class InteractableType {
    BUTTON,          // triggers a one-shot event (door, lift, etc.)
    PICKUP_HEALTH,   // restores health on touch (or press)
    PICKUP_AMMO,     // restores a consumable (grenades, etc.)
};

struct Interactable {
    glm::vec3        position;
    AABB             box;
    InteractableType type   = InteractableType::BUTTON;
    std::string      label;         // shown on crosshair prompt (future text render)
    bool             active = true; // inactive interactables are ignored by the raycast

    // Called once when the player successfully interacts.
    // Lambda captures let you reference GameplayState members directly.
    std::function<void()> onInteract;

    static constexpr float INTERACT_RANGE = 3.5f;
};
