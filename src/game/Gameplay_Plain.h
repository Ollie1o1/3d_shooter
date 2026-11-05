#pragma once
// =============================================================================
// Gameplay_Plain.h — GameplayState: Chapter 4, THE BLOOD MOON PLAIN
// (LevelPlain.h, docs/chapter-4-design.md). The arrival, the long view (the
// camera sees 4.5 km here), and the meteors: their flight, the impact you see
// at once and hear late, the shake when one comes down near, and the craters
// that burn and smoke while they cool.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::beginPlain() {
    storyOwned |= 15; storyArmed = storyGrapple = true;
    wakeT = -1; clawT = -1; beltBodies.clear(); player.eyeHeight = 1.7f;
    meteors.reset(); meteorHits.clear(); plainQuakes.clear(); plainSmokeT = 0.f;
    player.camera.farPlane = plain::FAR_PLANE;
    beginGleaner();   // (the arrival plays its reveal unless it has been seen: wakeSeen)
    // A few already on their way down, so the sky is busy from the first frame
    for (int k = 0; k < 5; ++k) {
        int i = meteors.launch(meteors.pickTarget(player.position), meteors.rnd(3.f, 7.f));
        if (i >= 0) {
            auto& m = meteors.pool[i];
            float along = meteors.rnd(0.15f, 0.8f) * (-m.pos.y / m.vel.y);
            m.pos += m.vel * along; m.prev = m.pos;
        }
    }
    // --overlay plainimpact: a big one lands 160 m ahead about two seconds in (screenshots)
    if (g_devOverlay == "plainimpact") {
        glm::vec3 f = player.camera.flatForward();
        int i = meteors.launch(player.position + f * 160.f, 12.f);
        if (i >= 0) { auto& m = meteors.pool[i]; float left = 2.2f; m.pos = player.position + f * 160.f - m.vel * left; m.prev = m.pos; m.age = 3.f; }
    }
    player.camera.position = player.position + glm::vec3{0, player.eyeHeight, 0}; prevCamPos = player.camera.position;
}

inline void GameplayState::updatePlain(float dt) {
    player.camera.farPlane = plain::FAR_PLANE;   // (a retry makes a fresh camera)
    if (plainReveal.active()) updatePlainReveal(dt);
    // The plain's air: a hot wind that never drops, rising on its back (hundreds of metres up)
    const bool inGut = insideGleaner();
    audio.setWind(playerDead ? 0.f : inGut ? 0.08f : onGleanerBack() ? 0.6f : 0.32f + 0.08f * std::sin(gameClock * 0.21f));
    if (inGut) meteors.next = std::max(meteors.next, 1.f);   // (no sky in here: none fall while you are inside it)
    updateGleaner(dt);
    meteors.tick(dt, player.position, meteorHits);
    for (const auto& hit : meteorHits) meteorImpact(hit);

    // The ground answers late: the shake arrives with the sound
    for (auto& q : plainQuakes) q.t -= dt;
    for (const auto& q : plainQuakes) if (q.t <= 0.f) shake(0.35f + q.amp, q.amp * 0.06f);
    plainQuakes.erase(std::remove_if(plainQuakes.begin(), plainQuakes.end(), [](const PlainQuake& q) { return q.t <= 0.f; }), plainQuakes.end());

    // Lightning inside the cloud banks: two or three flickers somewhere overhead, the thunder rolling in
    // a few seconds later (none inside the Gleaner: no sky in there)
    plainFlashNext -= dt;
    if (plainFlashT >= 0.f) plainFlashT += dt;
    if (plainFlashT > 1.2f) plainFlashT = -1.f;
    // (its own hash of the clock: drawing from rand() would shift every seeded run after it)
    auto rnd = [&](float k) { float v = std::sin((gameClock * 13.17f + k) * 12.9898f) * 43758.5453f; return v - std::floor(v); };
    if (plainFlashNext <= 0.f && !inGut && !playerDead) {
        plainFlashNext = 7.f + 13.f * (0.f + (1.f - 0.f) * rnd(1.f));
        float az = (0.f + (6.2832f - 0.f) * rnd(2.f)), el = (0.18f + (0.5f - 0.18f) * rnd(3.f));
        plainFlashDir = glm::normalize(glm::vec3{std::cos(az) * std::cos(el), std::sin(el), std::sin(az) * std::cos(el)});
        plainFlashT = 0.f; plainFlashPulses = 2 + ((0.f + (1.f - 0.f) * rnd(4.f)) > 0.5f ? 1 : 0);
        float far = (0.f + (1.f - 0.f) * rnd(5.f));
        audio.room.thunder(1.5f + 3.5f * far, 1.f - 0.55f * far);
    }

    // Smoke off the craters still burning near you
    plainSmokeT -= dt;
    if (plainSmokeT <= 0.f) {
        plainSmokeT = 0.12f;
        for (const auto& r : meteors.remains) {
            if (!r.alive || glm::length(r.pos - player.position) > 600.f) continue;
            float heat = MeteorRain::heat(r);
            fx.spawnBurst(r.pos + glm::vec3{0.f, r.size * 0.4f, 0.f}, glm::vec3{0.12f, 0.06f, 0.05f} + glm::vec3{0.5f, 0.12f, 0.03f} * heat * heat,
                          1, 2.5f, 3.5f, -2.5f);
        }
    }

    // Walking out of the playable square: the zone keeps you in; say why once
    const glm::vec3 p = player.position;
    if (!plainEdgeWarned && !insideGleaner() && (std::abs(p.x) > plain::HALF - 25.f || std::abs(p.z) > plain::HALF - 25.f)) {
        plainEdgeWarned = true;
        ui.feed("NOTHING OUT THERE BUT MORE OF THIS", {1.f, 0.45f, 0.3f});
    }
}

// One came down: a flash and a burst now; the boom and the shake when the sound gets here
inline void GameplayState::meteorImpact(const MeteorImpact& hit) {
    const glm::vec3 ear = player.camera.position;
    const float dist = glm::length(hit.pos - ear);
    const float big = hit.size / 7.f;
    if (dist < 900.f) {
        fx.spawnBurst(hit.pos + glm::vec3{0.f, 1.f, 0.f}, {2.2f, 0.7f, 0.18f}, (int)(24 + hit.size * 3), 18.f + hit.size * 2.f, 1.1f, 9.f);
        fx.spawnBurst(hit.pos + glm::vec3{0.f, 2.f, 0.f}, {0.16f, 0.08f, 0.06f}, (int)(10 + hit.size), 6.f, 3.f, -1.f);
    }
    // The sound: placed toward the impact (inside the mixer's reach), quieter and lower the further off
    SfxMixer::Opts o;
    o.group = SoundGroup::WORLD; o.positional = true;
    glm::vec3 to = hit.pos - ear;
    float len = glm::length(to);
    o.pos = ear + (len > 1.f ? to / len : glm::vec3{0.f, 0.f, -1.f}) * std::min(len, 45.f);
    float near = glm::clamp(1.f - dist / MeteorRain::MAX_FAR, 0.f, 1.f);
    o.floor = glm::clamp(0.08f + 0.75f * near * near * big, 0.05f, 0.9f);
    o.volume = 0.6f + 0.4f * near;
    o.pitch = 0.5f + 0.35f * near;
    o.delay = dist / 343.f;
    o.priority = dist < 400.f;
    audio.playOpts("explosion", o);
    if (dist < 500.f) plainQuakes.push_back({dist / 343.f, glm::clamp((1.f - dist / 500.f) * big, 0.05f, 1.f)});
    // One the Gleaner called down on you: it hurts if you were still under it
    if (hit.called && !playerDead) {
        glm::vec3 feet = player.position;
        float d = glm::length(glm::vec2{feet.x - hit.pos.x, feet.z - hit.pos.z});
        const float R = 9.f;
        if (d < R && feet.y - hit.pos.y < 12.f && feet.y > hit.pos.y - 3.f) {
            damagePlayer((10.f + 26.f * (1.f - d / R)) * tune().damage, hit.pos, 0.4f, 0.06f);
            if (!g_godMode) player.velocity += glm::normalize(glm::vec3{feet.x - hit.pos.x, 0.f, feet.z - hit.pos.z} + glm::vec3{0.f, 0.01f, 0.f}) * 12.f + glm::vec3{0.f, 9.f, 0.f};
        }
    }
}

// The meteors in the sky, the craters they leave, and the bodies on the heaps
// (the bodies are drawn with the story's dumped ones, Gameplay_Story.h). Fire,
// smoke and rock are lumpy faceted balls (MeshLib's lumpBall), not cubes.
// The flash's brightness now: quick flickers, each dimmer, then gone
inline float GameplayState::plainFlash() const {
    if (g_devOverlay == "plainflash") return 1.f;   // (dev: held on, for screenshots)
    if (plainFlashT < 0.f) return 0.f;
    float v = 0.f;
    for (int k = 0; k < plainFlashPulses; ++k) {
        float at = k * 0.17f + (k == 2 ? 0.2f : 0.f), d = plainFlashT - at;
        if (d >= 0.f) v = std::max(v, std::exp(-d * 14.f) * (1.f - 0.25f * k));
    }
    return v;
}

inline void GameplayState::gatherPlain(std::vector<BoxInstance>& out) {
    using namespace rig;
    using glm::vec3;
    const float ball = (float)meshlib::lumpBall();
    auto lump = [&](const mat4& m, vec3 col, vec3 emi) { push(out, m, col, emi); out.back().mesh = ball; };
    const vec3 hot{2.6f, 0.9f, 0.25f}, fire{1.8f, 0.4f, 0.08f}, rockCol{0.12f, 0.08f, 0.07f};
    const float sub = renderAlpha * (1.f / 60.f);
    gatherGleaner(out);
    gatherGleanerFight(out);
    // Fires on the horizon: the big falls that never went out, each a burning heap under a column of
    // smoke climbing ~450 m and leaning off with the wind. They give the plain a far distance on
    // every side (the Gleaner holds the north)
    if (!insideGleaner()) {
        static const float PLUME[7][3] = {{-1900, -700, 1.1f}, {-2300, 600, 0.8f}, {1700, -900, 1.f}, {2200, 400, 1.3f},
                                          {-900, 2300, 0.9f}, {1200, 2000, 1.2f}, {-2500, -1700, 0.7f}};
        const vec3 lean = glm::normalize(meteors.drift + vec3{0.001f, 0.f, 0.f});
        for (int pi = 0; pi < 7; ++pi) {
            const vec3 base{PLUME[pi][0], 0.f, PLUME[pi][1]};
            const float s = PLUME[pi][2], seed = pi * 3.7f;
            auto h = [&](float k) { float v = std::sin(seed * 12.9898f + k * 78.233f) * 43758.5453f; return v - std::floor(v); };
            for (int k = 0; k < 5; ++k) {   // the burning heap
                float a = k * 1.257f + seed, flick = 0.75f + 0.25f * std::sin(gameClock * (2.f + k) + seed);
                lump(T(base + vec3{std::cos(a) * 18.f * s, 6.f * s, std::sin(a) * 18.f * s}) * RY(a) * S(vec3{24.f * s, 14.f * s, 22.f * s}),
                     rockCol, fire * 1.2f * flick);
            }
            const int PUFFS = 26;
            for (int k = 0; k < PUFFS; ++k) {
                float u = std::fmod(gameClock * 0.012f + (float)k / PUFFS + h(k), 1.f);
                float ht = 20.f + 460.f * u * s;
                float sz = (30.f + 120.f * u) * s * (0.75f + 0.5f * h(k + 30.f));
                vec3 p = base + vec3{(h(k + 60.f) - 0.5f) * 30.f, ht, (h(k + 90.f) - 0.5f) * 30.f} + lean * (u * u * 380.f * s);
                vec3 glow = fire * 0.35f * std::max(0.f, 1.f - u * 3.f);   // lit from below near the fire
                float dark = 0.05f + 0.03f * h(k + 120.f);
                lump(T(p) * RY(k + u * 2.f) * RX(u + k) * S(vec3{sz, sz * 0.8f, sz}), vec3{dark, dark * 0.62f, dark * 0.55f}, glow);
            }
        }
    }
    for (const auto& m : meteors.pool) {
        if (!m.alive) continue;
        vec3 at = lerpPos(m.prev, m.pos);
        vec3 dir = glm::normalize(m.vel);
        float spin = m.spin + sub * 1.7f;
        // The rock: a dark lump round a white-hot core, a skin of flame
        mat4 root = T(at) * RY(spin) * RX(spin * 0.7f);
        lump(root * S(vec3{m.size * 0.75f}), {1.f, 0.9f, 0.7f}, hot * 1.6f);
        lump(root * RZ(0.7f) * S(vec3{m.size * 0.95f, m.size * 0.7f, m.size * 0.85f}), rockCol, fire * 0.45f);
        // The trail: fire close behind, thinning into smoke
        float speed = glm::length(m.vel);
        const int TRAIL = 30;
        const float step = m.size * 0.5f / std::max(speed, 1.f);   // overlapping: one unbroken streak
        for (int k = 1; k <= TRAIL; ++k) {
            float back = (float)k * step;
            if (back > m.age) break;
            float f = (float)k / TRAIL;
            vec3 tp = at - dir * speed * back;
            float sz = m.size * (0.85f - 0.2f * f) * (1.f + 0.8f * f);
            vec3 emi = glm::mix(hot, fire * 0.25f, std::sqrt(f)) * (1.f - f);
            vec3 col = glm::mix(vec3{0.9f, 0.5f, 0.2f}, vec3{0.12f, 0.07f, 0.06f}, f);
            lump(T(tp) * RY(spin + k) * RX(k * 0.7f) * S(vec3{sz}), col, emi);
        }
    }
    for (int ri = 0; ri < MeteorRain::REMAINS; ++ri) {
        const auto& r = meteors.remains[ri];
        if (!r.alive) continue;
        const float heat = MeteorRain::heat(r), seed = (float)ri * 7.13f + r.size;
        auto hash = [&](float k) { float v = std::sin(seed * 12.9898f + k * 78.233f) * 43758.5453f; return v - std::floor(v); };
        // The fireball, the first two seconds: a billowing cluster swelling, rising and going dark
        if (r.age < 2.f) {
            float u = r.age / 2.f;
            for (int k = 0; k < 9; ++k) {
                vec3 off{hash(k) - 0.5f, hash(k + 20.f) * 0.8f, hash(k + 40.f) - 0.5f};
                float sz = r.size * (1.2f + 3.5f * std::sqrt(u)) * (0.6f + 0.5f * hash(k + 60.f));
                vec3 p = r.pos + off * r.size * (2.f + 6.f * u) + vec3{0.f, r.size * (0.6f + 5.f * u), 0.f};
                vec3 emi = glm::mix(vec3{4.f, 2.4f, 1.f}, fire * 0.2f, std::min(1.f, u * 1.3f + 0.3f * hash(k + 80.f))) * (1.f - u * u);
                lump(T(p) * RY(k + u) * S(vec3{sz}), glm::mix(vec3{0.5f, 0.2f, 0.08f}, vec3{0.08f, 0.05f, 0.04f}, u), emi);
            }
            // The shock ring running out over the ground
            if (r.age < 0.9f) {
                float rr = r.size * (2.f + 30.f * r.age), w = std::max(0.f, 1.f - r.age / 0.9f);
                for (int k = 0; k < 20; ++k) {
                    float a = k * 0.31416f;
                    push(out, T(r.pos + vec3{std::cos(a) * rr, 0.25f, std::sin(a) * rr}) * RY(-a) * S(vec3{0.6f + rr * 0.06f, 0.3f, rr * 0.33f}),
                         vec3{0.6f, 0.3f, 0.15f}, vec3{2.f, 0.8f, 0.3f} * w);
                }
            }
            // Thrown rock, glowing, arcing out and down
            for (int k = 0; k < 8; ++k) {
                float a = hash(k + 100.f) * 6.283f, up = 14.f + 18.f * hash(k + 120.f), out2 = 10.f + 22.f * hash(k + 140.f);
                float t = r.age;
                vec3 p = r.pos + vec3{std::cos(a) * out2 * t, up * t - 9.8f * t * t, std::sin(a) * out2 * t};
                if (p.y < 0.f) continue;
                lump(T(p) * RY(t * 5.f + k) * RX(t * 3.f) * S(vec3{r.size * (0.12f + 0.1f * hash(k + 160.f))}), rockCol, fire * (1.f - u));
            }
        }
        // What is left: glowing broken rock, cooling
        for (int k = 0; k < 6; ++k) {
            float a = k * 1.047f + seed;
            vec3 p = r.pos + vec3{std::cos(a) * r.size * 0.55f, r.size * 0.1f, std::sin(a) * r.size * 0.55f};
            lump(T(p) * RY(a) * RX(0.4f * k) * S(vec3{r.size * (0.35f + 0.08f * (k % 3)), r.size * 0.3f, r.size * 0.4f}), rockCol,
                 fire * heat * (0.35f + 0.35f * (k % 2)));
        }
        // The smoke: puffs rising off it, drifting with the wind and spreading, a lit underside while it burns
        const vec3 wind = glm::normalize(meteors.drift) * 4.f;
        const int PUFFS = 20;
        for (int k = 0; k < PUFFS; ++k) {
            float t = std::fmod(r.age * 0.22f + (float)k / PUFFS, 1.f);
            if (r.age * 0.22f < (float)k / PUFFS) continue;   // (not born yet: the column grows up from the ground)
            float h = r.size * 0.6f + 55.f * t;
            float sz = r.size * (0.8f + 1.8f * t) * (0.7f + 0.5f * hash(k + 200.f));
            float fade = (1.f - t) * std::min(1.f, heat * 2.5f);
            if (fade <= 0.03f) continue;
            vec3 p = r.pos + vec3{(hash(k + 220.f) - 0.5f) * r.size, h, (hash(k + 240.f) - 0.5f) * r.size} + wind * (t * 12.f);
            lump(T(p) * RY(k + t) * S(vec3{sz * (0.6f + 0.4f * fade)}), vec3{0.06f, 0.035f, 0.03f} * (0.7f + 0.6f * fade),
                 fire * 0.25f * heat * (1.f - t) * (1.f - t));
        }
    }
}
