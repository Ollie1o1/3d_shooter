#pragma once
// =============================================================================
// Gameplay_Shadow.h — Chapter 6, level 4: THE SHADOW BAND (docs/chapter-6-design.md
// sections 3, 9, 13.4). The giant's shadow crosses the ring and it goes black.
//
//   the dark      ringDark(): 0 outside, 1 once you are SHADE_IN m in. The world's light
//                 goes to the giant's night side (ring::darkTheme); the sky shows its
//                 dark face (uRingNight). No music: true silence (section 9).
//   your lamp     a pale light a few metres ahead of you (point light 3), and the
//                 capsules' amber lamps along the route (neon: they light the ice).
//   lightning     silent flickers in the giant's night side every 4-11 s; for a frame
//                 the whole ring is lit grey, showing how much is out there.
//   the sweep     the mid tier (10 s, two waves). Its wall is barely visible here:
//                 the count, the floor's tick and the lit posts are what you go by.
//   the sunrise   past SUNRISE_AT the sun clears the giant's edge: a red arc, a
//                 diamond flash, then hard white light along the ring toward you at
//                 a sweep's speed. The one wall in the chapter you don't hide from.
// =============================================================================

// (its numbers: shadowband:: in world/LevelRing.h)

inline float GameplayState::ringDark() const {
    if (!ring() || g_devRingTest) return 0.f;
    float d = glm::clamp((ring::BAND_S - player.position.z) / ring::SHADE_IN, 0.f, 1.f);
    if (ringSunHit >= 0.f) d *= 1.f - glm::clamp((ringSunT - ringSunHit) / shadowband::LIFT, 0.f, 1.f);
    return d;
}

inline void GameplayState::tickShadow(float dt) {
    using namespace shadowband;
    const float dark = ringDark();
    // Lightning, deep in the giant's night side
    if (ringBoltT >= 0.f && (ringBoltT += dt) > 0.7f) ringBoltT = -1.f;
    if (g_devOverlay == "bolt" && ringBoltT < 0.f) ringBoltNext = std::min(ringBoltNext, 1.f);   // dev: one a second or so, for screenshots
    if (dark > 0.5f && ringBoltT < 0.f && (ringBoltNext -= dt) <= 0.f) {
        ringBoltT = 0.f; ringBoltNext = cosmetic::range(4.f, 11.f);
        ringBoltAt = {cosmetic::range(-0.7f, 0.7f), cosmetic::range(-0.1f, 0.7f)};
    }
    ringBolt = bolt(ringBoltT) * dark * (g_reduceFlashing ? 0.35f : 1.f);
    if (dark > 0.6f && !ringShadeHint) {
        ringShadeHint = true;
        ui.toast("THE SHADOW BAND", "NO SUN. YOU WON'T SEE THE WALL: COUNT, FEEL THE FLOOR, FIND A LIT POST", HOLDFAST_STEEL, 5.f);
    }
    // The sunrise
    if (ringSunT < 0.f && player.position.z < ring::SUNRISE_AT && dark > 0.5f && !playerDead) {
        ringSunT = 0.f; ringSunHit = -1.f; ringSunFrom = player.position.z - FRONT_FROM;
    }
    if (ringSunT >= 0.f) {
        const float before = ringSunT;
        ringSunT += dt;
        if (before < DIAMOND && ringSunT >= DIAMOND) {   // the diamond: the sun's edge clears the giant
            whiteFade = std::max(whiteFade, g_reduceFlashing ? 0.15f : 0.45f);
            audio.duck(20.f, 0.6f);
        }
        if (ringSunHit < 0.f && ringSunT > DIAMOND && ringSunFrom + FRONT_SPEED * (ringSunT - DIAMOND) >= player.position.z) {
            ringSunHit = ringSunT;   // the light reaches you
            shake(0.2f, 0.01f);
            audio.play("wave", 80, SoundGroup::UI, true);
        }
    }
}

// The world's light in the band: the giant's night side, the lightning and the Shepherd's flash
inline Theme GameplayState::ringShade(Theme th) const {
    const float d = ringDark();
    if (d <= 0.f) return th;
    th = lerpTheme(th, ring::darkTheme(), d);
    const glm::vec3 grey{0.32f, 0.34f, 0.42f};
    th.skyAmb += grey * 2.f * ringBolt; th.groundAmb += grey * 1.2f * ringBolt; th.lightColor += grey * 0.8f * ringBolt;   // (for a frame the whole ring is grey: how much is out there)
    if (sweep.active() && sweep.phase == RingSweep::Phase::FLASH && sweep.t < 0.5f) {   // the ridge lamps, far off: a breath of white
        const float f = (1.f - sweep.t / 0.5f) * 0.25f * d * (g_reduceFlashing ? 0.4f : 1.f);
        th.skyAmb += glm::vec3{f}; th.groundAmb += glm::vec3{f * 0.6f};
    }
    return th;
}

// Your lamp: point light 3, a few metres ahead and a little down, so it lights where you are going
inline void GameplayState::ringLamp() {
    const float d = ringDark();
    if (d <= 0.02f || playerDead) return;
    pointLightPos[MAX_POINT_LIGHTS - 1] = player.camera.position + player.camera.flatForward() * 4.5f - glm::vec3{0.f, 0.9f, 0.f};
    pointLightColor[MAX_POINT_LIGHTS - 1] = glm::vec3{2.4f, 2.35f, 2.2f} * d;
}

// The sunrise's light, racing along the ring toward you: a hard bright edge across the ground
inline void GameplayState::gatherShadow(std::vector<BoxInstance>& out) {
    using namespace rig; using namespace shadowband;
    if (ringSunT < DIAMOND || ringSunHit >= 0.f) return;
    const float z = ringSunFrom + FRONT_SPEED * (ringSunT - DIAMOND);
    const glm::vec3 warm{2.8f, 2.6f, 2.2f};
    // Lying on the ice: a hard bright edge, and behind it (north) the ice already in the sun, fading
    // into the distance. Nothing stands up out of it: it is light, not a wall
    const glm::vec3 ice{0.9f, 0.86f, 0.78f};
    for (float x = -130.f; x <= 130.f; x += 20.f) {
        push(out, T(glm::vec3{x, 0.03f, z - 0.6f}) * S(glm::vec3{20.2f, 0.02f, 1.2f}), warm * 0.5f, warm * 1.4f);
        for (int k = 0; k < 4; ++k)
            push(out, T(glm::vec3{x, 0.02f, z - 1.2f - 20.f * (k + 0.5f)}) * S(glm::vec3{20.2f, 0.02f, 20.2f}), ice * 0.6f, ice * (0.55f - 0.1f * k));
    }
}
