#pragma once
#include "ui/TextWrap.h"
// =============================================================================
// Gameplay_Story.h — GameplayState: STORY mode (docs/superpowers/specs/2026-10-28-story-mode-design.md).
// Included at the end of GameplayState.h.
//
// CHAPTER 1, THE CRADLE: nine factory rooms, the Sorter, and the ash exit.
//
// THE WAKE-UP (wakeT, seconds; first time only, a retry starts on your feet):
//    0.0  black. The boot text types out: ONLINE, OPTICS, PURPOSE (it searches,
//         and finds nothing), STATUS: REJECTED, stamped in red with a thud
//    4.6  the optics come on: black, then grey, soft and tearing, then clear.
//         On your back on the belt, gantry beams sliding past overhead
//   10.4  your right hand rises into view; at 11.4 each finger is tried in
//         turn, thumb to little finger (MOTOR [■■□□□] ... OK); it opens, turns,
//         and goes down
//   16.4  you sit up: the belt runs on to the furnace, a body ahead of you
//         tips over the lip into the fire (19.6). You can look about now
//   19.8  a claw on the gantry rail slides over the west belt, comes down
//         with another body, lets it go, and goes back up
//   23.2  you stand. 24.0: yours. CHAPTER 1 / THE CRADLE; GET OFF THE BELT
// Holding jump through it skips it.
//
// Bodies ride every belt (unnumbered: rejects never given one) and tip into
// the pit; ride in yourself and you burn. The chute in the east floor is the
// way on: falling into it carries you down to the Scrap Pit.
// UNIT 24 begins without a gun (the revolver is on Unit 1's wreck, in the Scrap
// Pit): his empty hands are drawn, the gun's HUD hidden, the trigger dead.
// =============================================================================

namespace story {
constexpr float W_BOOT = 4.6f, W_SEE = 9.4f, W_HAND = 10.4f, W_FING = 11.4f, W_FING_EACH = 0.56f, W_OPEN = 14.3f,
                W_DOWN = 15.5f, W_SIT = 16.4f, W_SAT = 18.4f, W_TIP = 19.6f, W_CLAW = 19.8f, W_STAND = 23.2f, W_END = 24.0f;
constexpr float LIE_EYE = 0.28f, SIT_EYE = 0.86f;
constexpr float SKIP_HOLD = 1.1f;   // seconds of jump held to skip the wake-up
inline float sm(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
inline float hash(float n) { float s = std::sin(n * 127.1f) * 43758.5453f; return s - std::floor(s); }
// A rejected body's file (made in Blender: tools/blender/unit24_build.py), loaded once
inline const gunkit::GunModel* bodyModel(const std::string& name = "unit_blank") {
    static std::map<std::string, gunkit::GunModel> loaded;
    auto it = loaded.find(name);
    if (it == loaded.end()) {
        it = loaded.emplace(name, gunkit::readGunFile("assets/enemies/" + name + ".glb", enemyfile::names())).first;
        if (!it->second.error.empty()) std::fprintf(stderr, "story: %s\n", it->second.error.c_str());
    }
    return it->second.parts.empty() || !it->second.error.empty() ? nullptr : &it->second;
}
inline std::string wreckFile(int number) { char b[16]; std::snprintf(b, sizeof(b), "unit%02d", number); return b; }
inline const char* gunTitle(int w) { return w == 0 ? "THE REVOLVER" : w == 1 ? "THE SHOTGUN" : w == 2 ? "THE LANCER" : "THE LONGSHOT"; }
// A wreck sitting where it stopped: its back to the wall, legs out, head down (in its joints)
inline rig::FilePose sittingPose(float u) {
    using namespace rig; using namespace enemyfile;
    FilePose P;
    P.motion[E_HIPL] = RX(-1.45f) * RZ(0.12f); P.motion[E_HIPR] = RX(-1.4f) * RZ(-0.1f);
    P.motion[E_KNEE_L] = RX(0.25f + 0.2f * u); P.motion[E_KNEE_R] = RX(0.1f);
    P.motion[E_ANKLE_L] = RX(-0.5f) * RZ(0.3f); P.motion[E_ANKLE_R] = RX(-0.4f) * RZ(-0.35f);
    P.motion[E_WAIST] = RX(-0.18f);
    P.motion[E_TORSO] = RX(0.08f) * RZ(0.06f * (u - 0.5f));
    P.motion[E_HEAD] = RX(0.62f) * RZ(0.25f - 0.5f * u) * RY(0.2f * (u - 0.5f));   // hung forward, to one side
    P.motion[E_ARML] = RZ(0.22f) * RX(-0.25f); P.motion[E_ELBOW_L] = RX(-0.3f);
    P.motion[E_ARMR] = RX(-0.75f) * RZ(-0.12f); P.motion[E_ELBOW_R] = RX(-0.7f);     // the right hand in the lap
    P.motion[E_FINGERS_L] = RX(0.4f); P.motion[E_FINGERS_R] = RX(0.9f);
    return P;
}
}

inline void GameplayState::beginStory() {
    using namespace story;
    const auto& S = level.story;
    beltBodies.clear();
    clawT = -1.f; storyEndT = -1.f; skipHold = 0.f; prevFinger = -1; offBeltShown = false;
    wakeHand = armfile::HandSpring{};
    storyArmed = storyOwned != 0;   // (what he's taken stays taken through a retry)
    storyFindT = 0.f; pitWakeT = -1.f; storyLog = -1;
    player.eyeHeight = 1.7f;
    if (!S.built) return;
    // the guns the wrecks are holding: look at one and press E
    interactables.clear();
    for (int i = 0; i < (int)S.wrecks.size(); ++i) {
        const auto& w = S.wrecks[i];
        if (w.weapon < 0 || storyOwns(w.weapon)) continue;
        Interactable it;
        it.position = glm::vec3(wreckGunXf(i)[3]);
        it.box = {it.position - glm::vec3{0.75f, 0.6f, 0.75f}, it.position + glm::vec3{0.75f, 0.9f, 0.75f}};
        it.label = std::string("TAKE ") + gunTitle(w.weapon);
        it.onInteract = [this, i]() { storyTakeGun(i); };
        interactables.push_back(std::move(it));
    }
    // Logs use the same explicit interaction path as weapon pickups.
    for (int i = 0; i < (int)S.wrecks.size(); ++i) {
        Interactable it; it.type = InteractableType::READABLE;
        it.position = S.wrecks[i].pos + glm::vec3{0, 1.65f, 0};
        it.box = {it.position - glm::vec3{.6f,.35f,.6f}, it.position + glm::vec3{.6f,.7f,.6f}};
        char label[48]; std::snprintf(label, sizeof(label), "READ UNIT %02d / LAST LOG", S.wrecks[i].number);
        it.label = label;
        it.onInteract = [this, i]() {
            storyLog = storyLog == i ? -1 : i; storyTerminal = -1; storyLogT = 0;
            if (storyLog >= 0) { storyLogsSeen |= 1u << i; saveStoryProgress(); audio.play("count_tick",90,SoundGroup::UI,true); }
        };
        interactables.push_back(std::move(it));
    }
    for (int i = 0; i < (int)S.terminals.size(); ++i) {
        Interactable it; it.type = InteractableType::READABLE;
        it.position = S.terminals[i].pos + glm::vec3{0, 1.8f, .25f};
        it.box = {it.position - glm::vec3{.85f,.55f,.65f}, it.position + glm::vec3{.85f,1.1f,.65f}};
        it.label = std::string("READ ") + S.terminals[i].title;
        it.onInteract = [this, i]() {
            storyTerminal = storyTerminal == i ? -1 : i; storyLog = -1;
            if (storyTerminal >= 0) { storyTerminalsSeen |= 1u << i; saveStoryProgress(); }
            audio.play("count_tick",90,SoundGroup::UI,true);
        };
        interactables.push_back(std::move(it));
    }
    if (activeWeapon >= 0 && !storyOwns(activeWeapon))
        for (int g = 0; g < WEAPON_COUNT; ++g) if (storyOwns(g)) { activeWeapon = g; break; }
    if(wasteland()) {beginWasteland();return;}
    // Bodies on the belts, spaced out down their length; the one ahead of you on yours timed to go over the lip
    // just after you sit up (W_TIP), and none close behind you
    for (int lane = 0; lane < (int)S.lanes.size(); ++lane) {
        float z = S.beltStart - 1.5f - hash(lane * 3.1f) * 4.f;
        int k = 0;
        while (z > S.beltEnd + 1.f) {
            bool mine = lane == 1;
            bool skip = mine && z > S.wake.z - 4.f;   // (where you lie, and just ahead)
            if (!skip) {
                BeltBody b;
                b.lane = lane; b.pos = {S.lanes[lane] + (hash(k * 7.7f + lane) - 0.5f) * 0.5f, S.beltTop + 0.115f, z};
                b.yaw = (hash(k * 1.3f + lane * 9.f) - 0.5f) * 0.3f;
                b.pose = hash(k * 4.1f + lane * 2.f);
                beltBodies.push_back(b);
            }
            z -= 6.5f + hash(k * 2.9f + lane * 5.3f) * 5.f;
            ++k;
        }
        beltSpawn[lane] = 2.f + hash(lane * 11.f) * 4.f;
        if (std::getenv("OD_STORYDBG")) for (auto& b : beltBodies) if (b.lane == lane) std::fprintf(stderr, "lane %d body x %.1f z %.1f\n", lane, b.pos.x, b.pos.z);
    }
    if (g_devCam) wakeSeen = true;   // (--cam: a screenshot from where it's put, no wake-up)
    // --overlay pit / pittake / ducts: start in the Scrap Pit (and take the revolver at once), or at the ducts with it
    if (g_devOverlay == "pit" || g_devOverlay == "pittake" || g_devOverlay == "ducts") {
        const bool ducts = g_devOverlay == "ducts";
        const int sec = ducts ? S.pitSection + 1 : S.pitSection;
        wakeSeen = true; storySeenSection = sec;
        if (ducts) { storyOwned |= 1; storyArmed = true; activeWeapon = 0; level.doors[level.arenas[S.pitSection].exitDoor].locked = false; }
        director.approach(sec);
        if (!g_devCam) {   // (--cam: from where it's put)
            player.position = level.arenas[sec].playerStart; player.velocity = glm::vec3{0.f};
            player.camera.yaw = level.arenas[sec].startYaw; player.camera.pitch = -8.f;
            player.camera.position = player.position + glm::vec3{0.f, 1.7f, 0.f}; prevCamPos = player.camera.position;
        }
        if (g_devOverlay == "pittake") { g_devOverlay.clear(); beginStory(); storyTakeGun(0); }
        return;
    }
    const char* roomNames[]={"assembly","archive","vault","lift","inspection","sorter"};
    for(int k=0;k<6;++k) if(g_devOverlay.rfind(roomNames[k],0)==0) {
        int sec=k+3; wakeSeen=true;wakeT=-1;storySeenSection=storyCheckpoint=sec;
        storyOwned=sec>=6?7:5;storyArmed=true;storyGrapple=sec>=8;activeWeapon=0;
        director.approach(sec);viewModel.reset(activeWeapon);
        for(int j=0;j<sec;++j)if(level.arenas[j].exitDoor>=0)level.doors[level.arenas[j].exitDoor].locked=false;
        if(g_devOverlay=="sorterend"||g_devOverlay=="sorterendcard") {
            storyBossDown=true;
            player.position={82,level.arenas[8].playerStart.y,g_devOverlay=="sorterend"?-66.f:-77.f};
            player.camera.yaw=-90;player.camera.pitch=5;
            if(g_devCam)player.position=g_devCamPos-glm::vec3{0,1.7f,0};
            player.camera.position=player.position+glm::vec3{0,1.7f,0};prevCamPos=player.camera.position;
            level.doors[level.arenas[8].exitDoor].locked=false;return;
        }
        if(!g_devCam){player.position=level.arenas[sec].playerStart;player.velocity=glm::vec3{0};player.camera.yaw=level.arenas[sec].startYaw;player.camera.pitch=-5;player.camera.position=player.position+glm::vec3{0,1.7f,0};prevCamPos=player.camera.position;}
        return;
    }
    if (g_devOverlay == "chute") {   // --overlay chute: on your feet over the chute (its ending, on film)
        wakeSeen = true;
        player.position = {(S.chute.min.x + S.chute.max.x) * 0.5f, 0.5f, (S.chute.min.z + S.chute.max.z) * 0.5f};
        player.camera.yaw = -90.f; player.camera.pitch = -30.f;
    }
    if (!wakeSeen) {   // the one that goes over as you sit up
        float z0 = S.beltEnd - 0.95f + S.beltSpeed * W_TIP;
        beltBodies.erase(std::remove_if(beltBodies.begin(), beltBodies.end(),
                         [&](const BeltBody& b) { return b.lane == 1 && std::fabs(b.pos.z - z0) < 5.f; }), beltBodies.end());
        BeltBody b; b.lane = 1; b.pos = {S.lanes[1] + 0.1f, S.beltTop + 0.115f, z0}; b.yaw = 0.12f; b.pose = 0.35f;
        beltBodies.push_back(b);
        // UNIT 24, on his back on the belt
        player.position = S.wake;
        player.velocity = glm::vec3{0.f};
        player.eyeHeight = LIE_EYE;
        player.camera.position = player.position + glm::vec3{0.f, LIE_EYE, 0.f};
        player.camera.yaw = -90.f; player.camera.pitch = 78.f;
        prevCamPos = player.camera.position;
        wakeT = 0.f;
        // --overlay wakeNNN: fast-forward the wake-up to NNN tenths of a second (screenshots)
        if (g_devOverlay.rfind("wake", 0) == 0 && g_devOverlay.size() > 4) {
            const float to = std::atoi(g_devOverlay.c_str() + 4) / 10.f, h = 1.f / 60.f;
            for (float t = 0.f; t < to; t += h) {
                player.position.z -= S.beltSpeed * h;
                updateStory(h);
            }
            player.camera.position = player.position + glm::vec3{0.f, player.eyeHeight, 0.f};
            prevCamPos = player.camera.position;
            g_devOverlay.clear();
        }
    } else wakeT = -1.f;
}

// The optics booting, for the composite (bloom_composite.frag uWake): 1 black .. 0 clear
inline float GameplayState::storyWakeFx() const {
    using namespace story;
    if (!storyLocked() && storyEndT < 0.f) return 0.f;
    if (storyEndT >= 0.f) return std::clamp(storyEndT / 1.4f, 0.f, 1.f);   // the chute: going dark
    if (wakeT < W_BOOT) return 1.f;
    float u = (wakeT - W_BOOT) / (W_SEE - W_BOOT);
    if (u >= 1.f) return 0.f;
    float base = 1.f - sm(u);
    // it doesn't come on cleanly: dips back toward black, harder early on
    float stutter = std::max(0.f, std::sin(wakeT * 21.f) * std::sin(wakeT * 6.7f + 1.f)) * 0.55f * (1.f - u);
    return std::clamp(base * 0.92f + stutter, 0.f, 1.f);
}

inline void GameplayState::updateStory(float dt) {
    using namespace story;
    const auto& S = level.story;
    if (!S.built) return;
    if(wasteland()) {updateWasteland(dt);return;}

    // ---- the wake-up ----
    if (wakeT >= 0.f) {
        const float was = wakeT;
        wakeT += dt;
        auto crossed = [&](float at) { return was < at && wakeT >= at; };
        // skip: hold jump
        skipHold = storySkipKey && was > 1.f ? skipHold + dt : std::max(0.f, skipHold - dt * 2.f);
        if (skipHold >= SKIP_HOLD && wakeT < W_STAND) { wakeT = W_STAND; skipHold = 0.f; }
        // the boot's beats: a tick per line, PURPOSE searching, the stamp
        for (float at : {0.5f, 1.2f, 2.0f}) if (crossed(at)) audio.play("count_tick", 70, SoundGroup::UI, true);
        if (crossed(3.3f)) audio.play("dry", 90, SoundGroup::UI, true);
        if (crossed(3.9f)) { audio.play("anvil_final", 128, SoundGroup::UI, true); audio.play("slam", 90, SoundGroup::UI, true); }
        if (crossed(W_BOOT)) audio.play("spawn", 70, SoundGroup::UI, true);
        // the finger test: a servo's click on each
        if (wakeT >= W_FING && wakeT < W_FING + 5 * W_FING_EACH) {
            int f = (int)((wakeT - W_FING) / W_FING_EACH);
            if (f != prevFinger) { prevFinger = f; audio.play(f == 4 ? "cyl_close" : "cyl_open", 55 + f * 6, SoundGroup::PLAYER); }
        }
        if (crossed(W_FING + 5 * W_FING_EACH)) audio.play("dash_ready", 80, SoundGroup::UI, true);   // MOTOR ... OK
        if (crossed(W_SIT)) audio.play("land_metal", 60, SoundGroup::PLAYER);
        if (crossed(W_STAND)) audio.play("land_metal", 90, SoundGroup::PLAYER);

        // the camera: lying, sitting up, standing
        float eye = LIE_EYE;
        if (wakeT >= W_SIT) eye = glm::mix(LIE_EYE, SIT_EYE, sm((wakeT - W_SIT) / (W_SAT - W_SIT)));
        if (wakeT >= W_STAND) eye = glm::mix(SIT_EYE, 1.7f, sm((wakeT - W_STAND) / (W_END - W_STAND)));
        // a jolt as each seam of the belt runs under you (every 1.2 m)
        float seam = std::fmod(player.position.z, 1.2f);
        eye += (seam < 0.05f ? -0.012f : 0.f) * (wakeT < W_STAND ? 1.f : 0.f);
        player.eyeHeight = eye;
        if (wakeT < W_SAT) {   // held: looking up, then sitting up to face the furnace
            float sit = wakeT < W_SIT ? 0.f : sm((wakeT - W_SIT) / (W_SAT - W_SIT));
            player.camera.yaw = -90.f + 6.f * std::sin(wakeT * 0.4f) * (1.f - sit);
            player.camera.pitch = glm::mix(78.f + 2.f * std::sin(wakeT * 0.7f), 2.f, sit);
            if (wakeT > W_HAND && wakeT < W_DOWN) player.camera.pitch -= 9.f * sm((wakeT - W_HAND) / 0.9f) * (1.f - sm((wakeT - W_OPEN) / 1.2f));   // looking down at the hand
        } else {               // free to look about (not all the way round: he's sitting)
            player.camera.yaw = glm::clamp(player.camera.yaw, -90.f - 75.f, -90.f + 75.f);
            player.camera.pitch = glm::clamp(player.camera.pitch, -40.f, 55.f);
        }
        // the claw: slides over the west belt ahead of you, lowers a body, lets it go, goes back up
        if (crossed(W_CLAW)) {
            clawT = 0.f;
            clawAt = {S.clawRail.x, S.clawRail.y, player.position.z - 9.f};
            audio.playAt("clank", clawAt, 110, SoundGroup::WORLD);
        }
        if (wakeT >= W_END) {
            wakeT = -1.f; wakeSeen = true;
            if (std::getenv("OD_STORYDBG")) std::fprintf(stderr, "wake over: at (%.1f %.1f %.1f) ground %d\n", player.position.x, player.position.y, player.position.z, player.groundWall);
            player.eyeHeight = 1.7f;
            pushBanner("THE CRADLE", "CHAPTER 1", {0.35f, 0.85f, 1.f}, 3.5f);
            audio.play("wave", 100, SoundGroup::UI, true);
        }
    }
    // the wake-hand's fingers
    {
        armfile::HandPose want = armfile::handRelaxed();
        if (wakeT >= W_FING && wakeT < W_OPEN) {
            int f = std::min(4, (int)((wakeT - W_FING) / W_FING_EACH));
            float u = (wakeT - W_FING - f * W_FING_EACH) / W_FING_EACH;
            float w = std::sin(3.14159f * std::clamp(u * 1.15f, 0.f, 1.f));
            const armfile::HandPose fist = armfile::handFist(), open = armfile::handFlat();
            for (int k = 0; k < 3; ++k) want.curl[f][k] = glm::mix(open.curl[f][k], fist.curl[f][k], w);
            for (int o = 0; o < 5; ++o) if (o != f) for (int k = 0; k < 3; ++k) want.curl[o][k] = glm::mix(want.curl[o][k], open.curl[o][k], 0.6f);
        } else if (wakeT >= W_OPEN && wakeT < W_DOWN) want = armfile::mixHand(armfile::handSpread(), armfile::handFlat(), sm((wakeT - W_OPEN) / 0.8f));
        wakeHand.step(want, dt, wakeT >= W_FING && wakeT < W_OPEN ? 1.6f : 1.f);
        if (wakeT > W_HAND && wakeT < W_FING && hash(std::floor(wakeT * 9.f)) > 0.8f) wakeHand.jolt(0.25f);   // twitches before it's tried
    }

    // ---- the claw ----
    if (clawT >= 0.f) {
        float was = clawT;
        clawT += dt;
        if (was < 1.8f && clawT >= 1.8f) {   // it lets go: the body drops onto the belt
            BeltBody b; b.lane = 0; b.pos = {S.lanes[0], S.beltTop + 1.6f, clawAt.z}; b.yaw = -0.2f; b.pose = 0.7f;
            b.dropped = true; b.vel = {0.f, 0.f, -S.beltSpeed};
            beltBodies.push_back(b);
            audio.playAt("hook_yank", clawAt, 110, SoundGroup::WORLD);
        }
        if (clawT > 5.5f) clawT = -1.f;
    }

    // ---- the bodies: ride the belts, tip over the lip, fall into the fire ----
    for (int lane = 0; lane < (int)S.lanes.size(); ++lane) {   // more keep coming out of the hatches
        beltSpawn[lane] -= dt;
        if (beltSpawn[lane] <= 0.f) {
            beltSpawn[lane] = 5.f + hash(gameClock * 1.7f + lane) * 5.f;
            bool clear = true;
            for (auto& b : beltBodies) if (b.lane == lane && b.pos.z > S.beltStart - 4.f) clear = false;
            if (clear && !(lane == 1 && std::fabs(player.position.x - S.lanes[1]) < 1.5f && player.position.z > S.beltStart - 6.f)) {
                BeltBody b; b.lane = lane; b.pos = {S.lanes[lane] + (hash(gameClock) - 0.5f) * 0.4f, S.beltTop + 0.115f, S.beltStart + 1.5f};
                b.yaw = (hash(gameClock * 3.f) - 0.5f) * 0.3f; b.pose = hash(gameClock * 5.f);
                beltBodies.push_back(b);
            }
        }
    }
    for (auto& b : beltBodies) {
        if (b.dropped) {   // let go by the claw: falls onto the belt, settles, then rides
            b.vel.y -= 18.f * dt;
            b.pos += b.vel * dt;
            if (b.pos.y <= S.beltTop + 0.115f) {
                b.pos.y = S.beltTop + 0.115f; b.dropped = false;
                audio.playAt("land_heavy", b.pos, 100, SoundGroup::WORLD);
                fx.spawnBurst(b.pos + glm::vec3{0.f, 0.1f, 0.9f}, {0.6f, 0.6f, 0.65f}, 10, 2.5f, 0.4f, -6.f);
            }
            continue;
        }
        if (!b.falling) {
            b.pos.z -= S.beltSpeed * dt;
            if (b.pos.z < S.beltEnd - 0.95f) { b.falling = true; b.vel = {0.f, 0.f, -S.beltSpeed}; b.spinV = 1.6f; }
        } else {
            b.vel.y -= 18.f * dt;
            b.pos += b.vel * dt;
            b.tip += b.spinV * dt;
            b.spinV += 2.5f * dt;
        }
    }
    for (auto& b : beltBodies)
        if (b.falling && b.pos.y < S.pitFloor + 0.4f) {   // into the slag
            fx.spawnBurst(b.pos + glm::vec3{0.f, 0.3f, 0.f}, {1.f, 0.5f, 0.1f}, 26, 7.f, 0.9f, -9.f);
            fx.spawnBurst(b.pos + glm::vec3{0.f, 0.6f, 0.f}, {1.f, 0.85f, 0.4f}, 12, 4.f, 0.5f, -3.f);
            audio.playAt("explosion", b.pos, 70, SoundGroup::WORLD);
            b.pos.y = -1e9f;
        }
    beltBodies.erase(std::remove_if(beltBodies.begin(), beltBodies.end(), [](const BeltBody& b) { return b.pos.y < -1e8f; }),
                     beltBodies.end());

    // ---- you: off the belt, into the fire, down the chute ----
    if (wakeT < 0.f && !offBeltShown && player.onGround && level.conveyorOfWall(player.groundWall) < 0 && wakeSeen && level.arenaAt(player.position) == 0) {
        offBeltShown = true;
        ui.feed("OFF THE BELT", {0.35f, 0.85f, 1.f});
    }
    const glm::vec3 p = player.position;
    const AABB& F = S.furnace;
    if (!playerDead && p.x > F.min.x && p.x < F.max.x && p.z > F.min.z && p.z < F.max.z && p.y < F.max.y) {
        if (!g_godMode) { styleSystem.health = 0.f; ui.onDamage(); }
        audio.play("explosion", 110, SoundGroup::PLAYER);
        fx.spawnBurst(p + glm::vec3{0.f, 0.5f, 0.f}, {1.f, 0.5f, 0.1f}, 30, 8.f, 0.9f, -9.f);
    }
    // ---- the sections: walking into the next one, the pit's fight, the wrecks' logs, the end ----
    {
        const int at = level.arenaAt(p);
        if (at > director.arena && director.phase == WaveDirector::Phase::APPROACH) director.approach(at);
        if (at > storySeenSection && at >= 0 && !playerDead) {
            storySeenSection = at;
            pushBanner(level.arenas[at].name, at == S.pitSection ? "CHAPTER 1  THE CRADLE" : "", {0.35f, 0.85f, 1.f}, 2.8f);
            audio.play("wave", 80, SoundGroup::UI, true);
        }
        if (director.arena == S.pitSection && director.phase == WaveDirector::Phase::APPROACH && at == S.pitSection) {
            if (!storyOwns(0)) storyFindT += dt;
            else {   // (a retry with the gun already taken: the scrap stirs a moment after you land)
                if (pitWakeT < 0.f) pitWakeT = 1.6f;
                pitWakeT -= dt;
                if (pitWakeT <= 0.f) { director.startArena(S.pitSection); pitWakeT = -1.f; shake(0.5f, 0.04f); }
            }
        }
        // Walking past a log never opens it or counts a secret. Leaving the
        // reading spot closes it; E on its beacon toggles it while nearby.
        if (storyLog >= 0) {
            storyLogT += dt;
            if (glm::length(S.wrecks[storyLog].pos - p) > 6.f) storyLog = -1;
        }
        if (storyTerminal >= 0 && glm::length(S.terminals[storyTerminal].pos - p) > 6.f) storyTerminal = -1;
    }
    updateStoryFactory(dt);
    const AABB& E = S.end;
    if (storyBossDown && storyEndT < 0.f && E.max.x > E.min.x && p.x > E.min.x && p.x < E.max.x && p.z > E.min.z && p.z < E.max.z &&
        p.y > E.min.y - 0.2f && p.y < E.max.y) {
        storyEndT = 0.f;
        saveStoryProgress();
        logRun("end");
        audio.play("wave", 90, SoundGroup::UI, true);
    }
    if (storyEndT >= 0.f) {
        storyEndT += dt;
        player.velocity = glm::vec3{0.f, std::max(player.velocity.y, 0.f), 0.f};
        if(storyEndT>3.f) captureMouse(false);
    }
}

// Taking a wreck's gun: it's yours (for good: through any retry), drawn with a flourish. In the pit, the scrap wakes.
inline void GameplayState::storyTakeGun(int i) {
    const auto& S = level.story;
    if (i < 0 || i >= (int)S.wrecks.size()) return;
    const auto& w = S.wrecks[i];
    if (w.weapon < 0 || storyOwns(w.weapon)) return;
    storyOwned |= 1 << w.weapon;
    storyArmed = true;
    saveStoryProgress();
    for (auto& it : interactables) if (it.type != InteractableType::READABLE && glm::length(it.position - glm::vec3(wreckGunXf(i)[3])) < 0.1f) it.active = false;
    activeWeapon = w.weapon; pendingWeapon = -1;
    weapons[w.weapon] = WeaponState{};
    weapons[w.weapon].ammo = weaponMag((WeaponId)w.weapon, prog.up[w.weapon]);
    viewModel.reset(w.weapon);
    viewModel.triggerSwitch(w.weapon);   // drawn: its flourish
    audio.play("pickup", 128, SoundGroup::UI, true);
    audio.play(w.weapon == 0 ? "cyl_close" : "bolt", 110, SoundGroup::PLAYER);
    char sub[64]; std::snprintf(sub, sizeof(sub), "FROM UNIT %02d", w.number);
    pushBanner(story::gunTitle(w.weapon), sub, {0.35f, 0.85f, 1.f}, 2.6f);
    if(w.weapon==0)teach(TEACH_PARRY);
    if(w.weapon==1)teach(TEACH_JUMP);
    ui.feed(w.weapon == 0 ? "LMB - FIRE    R - RELOAD" : "1 / 2 - SWITCH    RMB - AIM", {0.35f, 0.85f, 1.f});
    if (w.section == S.pitSection && director.arena == S.pitSection && director.phase == WaveDirector::Phase::APPROACH) {
        director.startArena(S.pitSection);   // the scrap wakes
        shake(0.8f, 0.05f);
        audio.play("explosion", 60, SoundGroup::WORLD);
    }
}

// A wreck sitting on the floor where it stopped: its hips just off the ground, facing `yaw`
inline glm::mat4 GameplayState::wreckRoot(int i) const {
    const auto& w = level.story.wrecks[i];
    return rig::T(w.pos + glm::vec3{0.f, -0.8f, 0.f}) * rig::RY(w.yaw);
}
// The gun it holds: the revolver in its right hand in its lap, a long gun across its knees
inline glm::mat4 GameplayState::wreckGunXf(int i) const {
    const auto& w = level.story.wrecks[i];
    using namespace rig;
    if (w.weapon == 0) return wreckRoot(i) * T({-0.15f, 1.1f, 0.34f}) * RY(0.35f) * RX(0.15f) * RZ(1.45f);
    return wreckRoot(i) * T({0.f, 1.04f, 0.38f}) * RY(1.5708f) * RZ(1.5708f) * RX(0.08f);
}

// The bodies, and the claw on its rail (into the frame's boxes)
inline void GameplayState::gatherStoryBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    const auto& SL = level.story;
    using glm::vec3; using glm::mat4;
    if (!SL.built) return;
    gatherStoryFactory(out);
    const gunkit::GunModel* M = story::bodyModel();
    const float PI = 3.14159265f;
    const glm::mat4 lying = RY(PI) * RX(-PI * 0.5f);   // on his back, feet first (to -Z), face up
    if (g_devOverlay == "unit" && M) {   // --overlay unit: one standing 3 m ahead, facing you (screenshots)
        glm::vec3 f = player.camera.flatForward();
        Rig r{out};
        FilePose P; P.chan[enemyfile::C_EYE] = glm::vec3{0.2f, 0.85f, 1.f}; P.chan[enemyfile::C_CORE] = glm::vec3{0.2f, 0.85f, 1.f};
        drawFile(r, T(player.position + f * 3.f) * RY(std::atan2(-f.x, -f.z)), *M, P, glm::vec3{0.2f, 0.85f, 1.f});
    }
    for (const auto& b : beltBodies) {
        if (glm::length(b.pos - player.camera.position) > 90.f) continue;
        glm::vec3 at = b.pos;
        if (!b.falling && !b.dropped) at.z -= SL.beltSpeed * renderAlpha * (1.f / 60.f);   // (between ticks)
        const mat4 root = T(at) * RX(-b.tip) * RY(b.yaw) * lying;
        if (!M || glm::length(at - player.camera.position) > 35.f) {   // far off (or no file): its silhouette in six boxes
            const vec3 cer{0.6f, 0.59f, 0.56f}, fr{0.12f, 0.12f, 0.14f};
            push(out, root * T({0.f, 1.32f, 0.f}) * S({0.3f, 0.32f, 0.2f}), cer);     // chest
            push(out, root * T({0.f, 1.68f, 0.f}) * S({0.2f, 0.2f, 0.2f}), cer);      // head
            push(out, root * T({0.f, 1.06f, 0.f}) * S({0.12f, 0.2f, 0.12f}), fr);     // waist
            push(out, root * T({0.f, 0.48f, 0.f}) * S({0.3f, 0.9f, 0.14f}), cer);     // legs
            push(out, root * T({0.26f, 1.15f, 0.f}) * S({0.08f, 0.6f, 0.09f}), cer);  // arms
            push(out, root * T({-0.26f, 1.15f, 0.f}) * S({0.08f, 0.6f, 0.09f}), cer);
            continue;
        }
        FilePose P;
        using namespace enemyfile;
        const float u = b.pose, side = u > 0.5f ? 1.f : -1.f;
        P.motion[E_ARML] = RZ(0.08f + 0.35f * u) * RX(0.2f * (u - 0.5f));            // arms lying where they fell
        P.motion[E_ARMR] = RZ(-0.08f - 0.35f * (1.f - u)) * RX(-0.15f * u);
        P.motion[E_ELBOW_L] = RX(-0.3f * u); P.motion[E_ELBOW_R] = RX(-0.25f * (1.f - u));
        P.motion[E_FINGERS_L] = RX(0.6f); P.motion[E_FINGERS_R] = RX(0.5f + 0.3f * u);
        P.motion[E_HEAD] = RY(side * (0.25f + 0.4f * u)) * RX(0.15f * u);            // the head lolled to one side
        P.motion[E_HIPL] = RZ(0.05f + 0.06f * u); P.motion[E_HIPR] = RZ(-0.05f - 0.05f * u);
        P.motion[E_KNEE_L] = RX(0.12f * u); P.motion[E_KNEE_R] = RX(0.1f * (1.f - u));
        P.motion[E_ANKLE_L] = RZ(0.3f); P.motion[E_ANKLE_R] = RZ(-0.3f);
        if (b.falling) {   // limbs flung as it goes over
            P.motion[E_ARML] = RZ(0.9f) * RX(-0.6f); P.motion[E_ARMR] = RZ(-1.0f) * RX(-0.4f);
            P.motion[E_HEAD] = RX(-0.5f);
        }
        // dead: dark eyes; a few still flicker in the core, very faintly
        const float flick = u > 0.7f ? 0.25f * std::max(0.f, std::sin(gameClock * 13.f + u * 40.f)) : 0.f;
        P.chan[C_EYE] = glm::vec3{0.f};
        P.chan[C_CORE] = glm::vec3{0.2f, 0.8f, 1.f} * flick;
        Rig r{out};
        drawFile(r, root, *M, P, glm::vec3{0.06f, 0.09f, 0.11f} + glm::vec3{0.2f, 0.8f, 1.f} * flick);
    }
    // ---- the dumped: bodies thrown into the pit, sprawled on their backs or fronts (six boxes far off) ----
    auto sprawled = [&](float u) {
        FilePose P;
        using namespace enemyfile;
        const float side = u > 0.5f ? 1.f : -1.f;
        P.motion[E_ARML] = RZ(0.15f + 0.9f * u) * RX(0.4f * (u - 0.5f)); P.motion[E_ELBOW_L] = RX(-0.6f * u);
        P.motion[E_ARMR] = RZ(-0.2f - 0.8f * (1.f - u)) * RX(-0.5f * u); P.motion[E_ELBOW_R] = RX(-0.5f * (1.f - u));
        P.motion[E_HEAD] = RY(side * (0.4f + 0.5f * u)) * RX(0.2f);
        P.motion[E_HIPL] = RZ(0.1f + 0.25f * u) * RX(-0.3f * u); P.motion[E_KNEE_L] = RX(0.6f * u);
        P.motion[E_HIPR] = RZ(-0.08f - 0.2f * (1.f - u)); P.motion[E_KNEE_R] = RX(0.3f * (1.f - u));
        P.motion[E_FINGERS_L] = RX(0.7f); P.motion[E_FINGERS_R] = RX(0.5f);
        P.chan[C_EYE] = vec3{0.f}; P.chan[C_CORE] = vec3{0.f};
        return P;
    };
    auto silhouette = [&](const mat4& root) {
        const vec3 cer{0.6f, 0.59f, 0.56f}, fr{0.12f, 0.12f, 0.14f};
        push(out, root * T({0.f, 1.32f, 0.f}) * S({0.3f, 0.32f, 0.2f}), cer);
        push(out, root * T({0.f, 1.68f, 0.f}) * S({0.2f, 0.2f, 0.2f}), cer);
        push(out, root * T({0.f, 1.06f, 0.f}) * S({0.12f, 0.2f, 0.12f}), fr);
        push(out, root * T({0.f, 0.48f, 0.f}) * S({0.3f, 0.9f, 0.14f}), cer);
        push(out, root * T({0.26f, 1.15f, 0.f}) * S({0.08f, 0.6f, 0.09f}), cer);
        push(out, root * T({-0.26f, 1.15f, 0.f}) * S({0.08f, 0.6f, 0.09f}), cer);
    };
    const vec3 dead{0.06f, 0.09f, 0.11f};
    for (const auto& d : SL.dumped) {
        float dist = glm::length(d.pos - player.camera.position);
        if (dist > 80.f) continue;
        const mat4 root = T(d.pos + vec3{0.f, 0.12f, 0.f}) * RY(d.yaw) * RZ(d.tilt) * RX((d.face ? 1.f : -1.f) * PI * 0.5f);
        if (!M || dist > 30.f) { silhouette(root); continue; }
        Rig r{out};
        drawFile(r, root, *M, sprawled(d.pose), dead);
    }
    // ---- the wrecks: the units before you, sitting where they stopped, a gun in the lap of some ----
    for (int i = 0; i < (int)SL.wrecks.size(); ++i) {
        const auto& w = SL.wrecks[i];
        if (glm::length(w.pos - player.camera.position) > 70.f) continue;
        const gunkit::GunModel* WM = story::bodyModel(story::wreckFile(w.number));
        const mat4 root = wreckRoot(i);
        if (!WM) { silhouette(root); continue; }
        FilePose P = story::sittingPose(0.3f + 0.2f * (float)(w.number % 3));
        const float flick = w.number == 3 ? 0.3f * std::max(0.f, std::sin(gameClock * 0.9f)) : 0.f;   // (03: not quite gone)
        P.chan[enemyfile::C_EYE] = vec3{0.2f, 0.85f, 1.f} * flick;
        P.chan[enemyfile::C_CORE] = vec3{0.2f, 0.85f, 1.f} * flick * 0.5f;
        Rig r{out};
        drawFile(r, root, *WM, P, dead + vec3{0.2f, 0.85f, 1.f} * flick);
        if (w.weapon >= 0 && !storyOwns(w.weapon)) {   // the gun, glinting: the thing to find
            static std::vector<GunPart> gp;
            gp.clear();
            gunkit::buildGun(w.weapon, GunLook{}, GunPose{}, gp);
            const mat4 gx = wreckGunXf(i);
            const float glint = 0.5f + 0.5f * std::sin(gameClock * 2.6f);
            for (const auto& q : gp) {
                push(out, gx * q.xf, q.color, q.emissive + vec3{0.15f, 0.55f, 0.75f} * (0.15f + 0.25f * glint));
                out.back().mesh = (float)q.mesh;
            }
            const vec3 at = vec3(gx[3]);
            const float star = std::pow(std::max(0.f, std::sin(gameClock * 1.3f + i)), 24.f);   // a glint runs along it now and then
            push(out, T(at + vec3{0.f, 0.06f, 0.f}) * RY(gameClock) * RX(0.785f) * S(vec3{0.025f + 0.05f * star}),
                 vec3{0.6f, 0.9f, 1.f}, vec3{1.5f, 2.6f, 3.f} * (0.3f + 2.5f * star));
        }
    }
    // the claw belongs to the factory, never to the outdoor chapter.
    if(!wasteland()) {
        float z = SL.beltStart - 70.f, drop = 0.f, open = 0.f;   // parked at the north end of the rail
        bool holding = false;
        if (clawT >= 0.f) {
            const float t = clawT;
            float slide = story::sm(t / 0.8f), back = story::sm((t - 3.6f) / 1.9f);
            z = glm::mix(clawAt.z - 16.f, clawAt.z, slide) + (clawAt.z - 30.f - clawAt.z) * back;
            drop = story::sm((t - 0.8f) / 1.0f) * (1.f - story::sm((t - 2.0f) / 1.4f));
            open = story::sm((t - 1.75f) / 0.15f);
            holding = t < 1.8f;
        } else if (wakeT < 0.f && wakeSeen) z = SL.beltEnd - 2.f;
        const float x = SL.clawRail.x, top = SL.clawRail.y;
        const float headY = top - 0.6f - drop * (top - SL.beltTop - 2.6f);
        const vec3 steel{0.25f, 0.26f, 0.3f}, dark{0.1f, 0.1f, 0.12f}, hazard{0.78f, 0.6f, 0.08f};
        push(out, T({x, top - 0.25f, z}) * S({1.4f, 0.5f, 1.6f}), hazard);                       // trolley
        push(out, T({x, top - 0.05f, z}) * S({1.5f, 0.1f, 1.7f}), dark);
        push(out, T({x + 0.72f, top - 0.25f, z + 0.5f}) * S({0.06f, 0.12f, 0.12f}), dark, glm::vec3{1.6f, 0.15f, 0.1f});   // its lamp
        const float cable = top - 0.5f - headY;
        if (cable > 0.05f) push(out, T({x, headY + cable * 0.5f, z}) * S({0.07f, cable, 0.07f}), dark);
        push(out, T({x, headY, z}) * S({0.7f, 0.35f, 0.7f}), steel);                             // the head
        for (int k = 0; k < 3; ++k) {                                                             // its fingers
            float a = k * 2.0944f;
            vec3 d{std::cos(a), 0.f, std::sin(a)};
            float spread = 0.22f + 0.25f * open;
            push(out, T(vec3{x, headY - 0.45f, z} + d * spread) * RY(-a) * RZ(0.25f + 0.6f * open) * S({0.1f, 0.7f, 0.14f}), steel);
        }
        if (holding && M && clawT >= 0.f) {   // the body it's carrying, hanging by its chest
            FilePose P;
            using namespace enemyfile;
            P.motion[E_ARML] = RZ(0.15f); P.motion[E_ARMR] = RZ(-0.15f); P.motion[E_HEAD] = RX(0.45f);
            P.motion[E_KNEE_L] = RX(0.1f);
            Rig r{out};
            drawFile(r, T({x, headY - 2.05f, z}) * RY(0.6f + 0.2f * std::sin(gameClock * 1.3f)), *M, P, glm::vec3{0.06f, 0.09f, 0.11f});
        }
    }
    // the belts' slats, moving (the belt's surface is a still box: these make it run)
    for (int lane = 0; lane < (int)SL.lanes.size(); ++lane) {
        const float off = std::fmod(moverClock * SL.beltSpeed + renderAlpha * SL.beltSpeed / 60.f, 1.2f);
        for (float z = SL.beltStart + 1.f - off; z > SL.beltEnd; z -= 1.2f) {
            if (std::fabs(z - player.camera.position.z) > 70.f) continue;
            push(out, T({SL.lanes[lane], SL.beltTop + 0.004f, z}) * S({2.56f, 0.012f, 0.08f}), {0.16f, 0.16f, 0.18f});
        }
    }
}

// UNIT 24's hands: the right one raised as he wakes; both, empty, while he has no gun
inline void GameplayState::drawStoryArms(const Camera& cam) {
    using namespace story;
    using glm::vec3;
    ViewModel::FreeArm A[2];
    int n = 0;
    if (wakeT >= W_HAND && wakeT < W_DOWN + 0.9f) {
        float in = sm((wakeT - W_HAND) / 0.9f), out = sm((wakeT - W_DOWN) / 0.8f);
        float k = in * (1.f - out);
        vec3 at = glm::mix(vec3{0.22f, -0.7f, 0.22f}, vec3{0.035f, -0.05f, 0.235f}, k);
        at.y += 0.006f * std::sin(wakeT * 2.3f);   // not quite steady
        vec3 fingers = glm::normalize(glm::mix(vec3{0.35f, 1.f, 0.25f}, vec3{-0.12f, 1.f, 0.05f}, k));
        // he turns it over to look at it, then back
        float turn = 1.2f * sm((wakeT - W_OPEN) / 0.7f) * (1.f - sm((wakeT - W_OPEN - 0.9f) / 0.5f));
        vec3 back = glm::normalize(vec3{0.12f + std::sin(turn) * 1.2f, 0.05f, -std::cos(turn)});
        A[n].shoulder = {0.2f, -0.46f, -0.08f};
        A[n].fist = armfile::handFrame(at, fingers, back, 1.f);
        A[n].side = 1.f;
        A[n].hand = wakeHand.shown(0.6f);
        ++n;
    } else if (!storyLocked() && !storyArmed && !playerDead && storyEndT < 0.f) {
        // empty-handed: both hands low at the edges of the view, swinging a little with the stride
        float stride = camFeel.strideAt(renderAlpha);
        float spd = std::min(1.f, playerXZSpeed / 10.f);
        for (float s : {1.f, -1.f}) {
            float sw = std::sin(stride + (s > 0.f ? 0.f : 3.14159f)) * 0.035f * spd;
            vec3 at{s * 0.30f, -0.37f + std::fabs(sw) * 0.4f, 0.33f + sw};
            vec3 fingers = glm::normalize(vec3{-s * 0.25f, 0.45f, 1.f});
            vec3 back = glm::normalize(vec3{s * 0.9f, 0.3f, -0.1f});
            A[n].shoulder = {s * 0.34f, -0.62f, -0.02f};
            A[n].fist = armfile::handFrame(at, fingers, back, s);
            A[n].side = s;
            A[n].hand = armfile::handRelaxed();
            ++n;
        }
    }
    if (n) viewModel.drawFreeArms(worldShader, cam, A, n);
}

// Over everything: the boot text, the motor check, the skip prompt, the chute's ending
inline void GameplayState::renderStoryOverlay(const glm::mat4& viewProj) {
    using namespace story;
    auto& U = ui.ui;
    const auto& SL = level.story;
    // ---- in play: the way to a gun, the prompt to take it, a wreck's last log ----
    if (wakeT < 0.f && storyEndT < 0.f && !playerDead && SL.built) {
        // a waypoint on a gun that's yours to take in the section you're in (in the pit, once you've looked about)
        for (int i = 0; i < (int)SL.wrecks.size(); ++i) {
            const auto& w = SL.wrecks[i];
            if (w.weapon < 0 || storyOwns(w.weapon) || w.section != level.arenaAt(player.position)) continue;
            if (w.section == SL.pitSection && storyFindT < 3.f) continue;
            if (director.fighting()) continue;   // (not while something's trying to kill you)
            const glm::vec3 at = glm::vec3(wreckGunXf(i)[3]) + glm::vec3{0.f, 0.5f, 0.f};
            const float dist = glm::length(at - player.camera.position);
            if (dist < 3.f) continue;
            const auto marker=projectWorldMarker(at,viewProj,SCREEN_W,SCREEN_H);
            bool covered=!marker.behind && markerBehindCover(player.camera.position,at,level.walls);
            char lbl[96];std::snprintf(lbl,sizeof(lbl),"%s %.0fM%s",gunTitle(w.weapon)+4,dist,covered?" / BEHIND COVER":"");
            ui.renderMarker(marker.x,marker.y,marker.onScreen,{.35f,.85f,1.f},lbl,marker.behind);
        }
    }
    ui.begin2D();
    if (wakeT < 0.f && storyEndT < 0.f && !playerDead && SL.built) {
        int it = findInteractTarget();
        if (it >= 0 && interactables[it].active) {   // the prompt, under the crosshair
            char pr[128]; std::snprintf(pr, sizeof(pr), "[%s]  %s", (gamepad::state().pad ? "R3" : binds().label(Act::INTERACT).c_str()), interactables[it].label.c_str());
            const float pulse = 0.8f + 0.2f * std::sin(gameClock * 6.f);
            U.textShadow(pr, SCREEN_W / 2.f, SCREEN_H / 2.f + 44.f, 2, {0.45f, 0.9f, 1.f, pulse}, true);
        }
        if (storyLog >= 0 && storyLog < (int)SL.wrecks.size()) {   // the last log, typed out in a terminal at the foot of the screen
            const auto& w = SL.wrecks[storyLog];
            const float a = sm(storyLogT / 0.3f);
            const float W = 760.f, H = 142.f, X = SCREEN_W / 2.f - W / 2.f, Y = SCREEN_H - 190.f;
            U.rect(X, Y, W, H, {0.01f, 0.03f, 0.04f, 0.78f * a});
            U.rect(X, Y, 4.f, H, {0.35f, 0.85f, 1.f, 0.9f * a});
            char head[48]; std::snprintf(head, sizeof(head), "UNIT %02d  -  LAST LOG", w.number);
            U.text(head, X + 18.f, Y + 12.f, 1, {0.35f, 0.85f, 1.f, 0.9f * a});
            const std::string full = w.log;
            const int n = std::clamp((int)((storyLogT - 0.3f) * 40.f), 0, (int)full.size());
            // wrapped at about 46 characters a line
            std::string shown = full.substr(0, (size_t)n), line;
            float ly = Y + 34.f;
            size_t pos = 0;
            while (pos < shown.size() && ly < Y + H) {
                size_t end = std::min(shown.size(), pos + 46);
                if (end < full.size() && end < shown.size()) { size_t sp = shown.rfind(' ', end); if (sp != std::string::npos && sp > pos) end = sp; }
                else if (end < full.size()) { size_t sp = full.rfind(' ', pos + 46); if (sp != std::string::npos && sp > pos && sp < end) end = sp; }
                line = shown.substr(pos, end - pos);
                U.text(line.c_str(), X + 18.f, ly, 2, {0.82f, 0.9f, 0.92f, a});
                ly += 26.f; pos = end; while (pos < shown.size() && shown[pos] == ' ') ++pos;
            }
        }
    }
    if(wakeT<0 && storyEndT<0 && storyTerminal>=0 && storyLog<0) {
        const auto& t=SL.terminals[storyTerminal];
        const auto lines=ui_text::wrap(t.line,(SCREEN_W-200)/12);
        const float height=48.f+lines.size()*26.f,y=SCREEN_H-110.f-height;
        U.rect(80,y,SCREEN_W-160,height,{.01f,.03f,.04f,.9f});
        U.text(t.title,100,y+15,1,{.35f,.85f,1.f,1});
        for(size_t i=0;i<lines.size();++i)
            U.text(lines[i].c_str(),100,y+44+i*26,2,{.85f,.9f,.92f,1});
    }
    for(const auto& e:enemies)if(e.alive&&e.type==EnemyType::SORTER&&storyEndT<0) {
        const char* phase=e.sortPhase==3?"INCINERATION":e.sortPhase==2?"RECALIBRATION":"INSPECTION";
        U.textShadow(e.sortMark>0?"MARKED - PARRY / BREAK SIGHT":e.staggered()?"PARRIED - DOUBLE DAMAGE":e.sortOpen>0?"CORE OPEN - AIM FOR THE BLUE STRIP":phase,SCREEN_W/2.f,88,2,{1.f,.65f,.3f,1},true);
        if(e.attack==AttackKind::SORT_STAMP&&e.telegraphTimer>0) {
            char warning[80];std::snprintf(warning,sizeof(warning),e.sortLocked?"PRESS LOCKED  %.1fS / DASH OUT":"OVERHEAD PRESS  %.1fS / TARGETING",e.telegraphTimer);
            U.textShadow(warning,SCREEN_W/2.f,112,2,e.sortLocked?glm::vec4{1,.3f,.12f,1}:glm::vec4{1,.8f,.3f,1},true);
        }
    }
    if (wakeT >= 0.f) {
        const float t = wakeT;
        const float fade = 1.f - sm((t - W_BOOT + 0.3f) / 0.8f);
        const glm::vec4 green{0.45f, 1.f, 0.85f, 0.95f * fade}, grey{0.55f, 0.6f, 0.62f, 0.9f * fade};
        auto typed = [&](const char* s, float from, float cps = 38.f) {
            static char buf[96];
            int n = std::clamp((int)((t - from) * cps), 0, (int)std::strlen(s));
            std::snprintf(buf, sizeof(buf), "%.*s", n, s);
            return (const char*)buf;
        };
        const float X = 120.f, Y = 250.f, LH = 34.f;
        if (fade > 0.f) {
            if (t > 0.5f) U.text(typed("> UNIT 24 .............. ONLINE", 0.5f), X, Y, 2, green);
            if (t > 1.2f) U.text(typed("> OPTICS ............... CALIBRATING", 1.2f), X, Y + LH, 2, green);
            if (t > 2.0f) {
                U.text(typed("> PURPOSE: ", 2.0f), X, Y + 2 * LH, 2, green);
                const float px = X + UIBatch::textWidth("> PURPOSE: ", 2);
                if (t < 3.3f) {   // searching
                    char dots[16]; int nd = 1 + (int)(t * 5.f) % 6;
                    std::snprintf(dots, sizeof(dots), "%.*s", nd, "......");
                    if (t > 2.3f) U.text(dots, px, Y + 2 * LH, 2, green);
                } else U.text("------", px, Y + 2 * LH, 2, grey);   // nothing
            }
            if (t > 3.9f) {   // stamped
                const float k = 1.f + 1.5f * (1.f - sm((t - 3.9f) / 0.12f));
                U.text("> STATUS:", X, Y + 3 * LH, 2, green);
                const glm::vec4 red{1.f, 0.18f, 0.12f, fade};
                U.text("REJECTED", X + UIBatch::textWidth("> STATUS: ", 2), Y + 3 * LH - (k - 1.f) * 8.f, (int)std::round(2.f * k), red);
            }
            if ((int)(t * 2.5f) % 2 == 0 && t < W_BOOT - 0.2f) U.rect(X, Y + 4 * LH, 14, 22, green);   // the cursor
        }
        // the motor check, small in the corner, while the fingers are tried
        if (t >= W_FING - 0.2f && t < W_OPEN + 1.2f) {
            const float a = sm((t - W_FING + 0.2f) / 0.3f) * (1.f - sm((t - W_OPEN - 0.6f) / 0.6f));
            int done = std::clamp((int)((t - W_FING) / W_FING_EACH), 0, 5);
            char bar[64] = "> MOTOR  [";
            for (int f = 0; f < 5; ++f) std::strcat(bar, f < done ? "#" : ".");
            std::strcat(bar, done >= 5 ? "]  OK" : "]");
            U.text(bar, 60.f, 60.f, 2, {0.45f, 1.f, 0.85f, 0.85f * a});
        }
        if (t > 2.f && t < W_STAND) {   // skip
            const char* s = "HOLD SPACE TO SKIP";
            const float w = (float)UIBatch::textWidth(s, 1);
            U.text(s, SCREEN_W - 40.f - w, SCREEN_H - 50.f, 1, {0.8f, 0.85f, 0.9f, 0.45f + 0.4f * std::min(1.f, skipHold * 3.f)});
            if (skipHold > 0.f) U.rect(SCREEN_W - 40.f - w, SCREEN_H - 38.f, w * std::min(1.f, skipHold / SKIP_HOLD), 3.f, {0.35f, 0.85f, 1.f, 0.9f});
        }
    }
    if (storyEndT >= 0.f) {   // down the chute
        const float a = sm((storyEndT - 1.6f) / 0.8f);
        U.rect(0.f, 0.f, SCREEN_W, SCREEN_H, {0.f, 0.f, 0.f, sm(storyEndT / 1.4f)});
        U.text(SL.nextName, SCREEN_W / 2.f, SCREEN_H / 2.f - 40.f, 5, {0.85f, 0.9f, 0.92f, a}, true);
        char complete[48];std::snprintf(complete,sizeof(complete),"CHAPTER %d COMPLETE",SL.chapter);
        U.text(complete, SCREEN_W / 2.f, SCREEN_H / 2.f + 30.f, 2, {0.35f, 0.85f, 1.f, a * 0.85f}, true);
        char line[128];int secrets=0;for(size_t i=0;i<SL.wrecks.size();++i)if(SL.wrecks[i].secret && ((storyLogsSeen>>i)&1u))++secrets;
        std::snprintf(line,sizeof(line),"TIME %d:%02d   KILLS %d   SECRETS %d/2",(int)elapsedTime/60,(int)elapsedTime%60,totalKills,secrets);
        U.text(line,SCREEN_W/2.f,SCREEN_H/2.f+90,2,{.85f,.9f,.92f,a},true);
        float score=bankedStyle+styleSystem.earned;
        const char* rank=deaths==0&&elapsedTime<900&&score>5000?"S":deaths<3&&elapsedTime<1200?"A":deaths<6?"B":"C";
        std::snprintf(line,sizeof(line),"RANK %s   STYLE %.0f   DEATHS %d",rank,score,deaths);
        U.text(line,SCREEN_W/2.f,SCREEN_H/2.f+130,2,{1.f,.7f,.3f,a},true);
        U.text(SL.nextChapter>0?"ENTER - CONTINUE TO CHAPTER 2    ESC - MENU":"ENTER - RETURN TO MENU",SCREEN_W/2.f,SCREEN_H-65,1,{.35f,.85f,1.f,a},true);
    }
    ui.end2D();
}

#include "game/Gameplay_StoryFactory.h"
#include "game/Gameplay_StoryAtmosphere.h"
