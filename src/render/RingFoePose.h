#pragma once
// =============================================================================
// RingFoePose.h — how Chapter 6's DISPATCHED move (included by EnemyModel.h
// inside namespace rig; models: tools/blender/ring_foes_build.py).
//
//   STRAY   a loping low-gravity bound: long hang-time strides, the body lifting
//           off the ground at the top of each one, arms out for balance, the
//           cradle on its back springing as it lands. Its claw is raised for a
//           swipe; a lunge throws it forward arms-first (white: parry).
//   HERDER  hangs nose-down behind its plough, pods swivelling to steer, its
//           lamp sweeping; a ram tilts it right over (gold), pods flared back.
//   TALLY   still. The bead rows slide and the drum turns a notch every count,
//           its lamp blinking each time.
//   PORTER  a heavy, rolling trudge, bent under its capsule (which sways on its
//           straps); both shovel hands come up over its head for a slam, one
//           hauls back to throw.
// Signs as in the lab rigs: RX(-) raises an arm forward and leans a torso forward.
// =============================================================================

inline bool ringFoePose(const Enemy& e, float time, const mat4& root, mat4& fileRoot, FilePose& P, vec3& glowOut) {
    using namespace enemyfile;
    const EnemyStats& st = e.stats();
    const float tp = e.telegraphProgress(), id = (float)e.uid;
    const float run = glm::clamp(e.moveSpeed / std::max(0.1f, st.speed), 0.f, 1.3f);
    vec3 glow = st.glow * (1.f + 2.f * tp);
    P = FilePose{};
    fileRoot = root;
    P.chan[C_EYE] = st.glow; P.chan[C_CORE] = vec3{2.2f, 1.25f, 0.35f};   // the capsule lamp: amber
    P.chan[C_GLOW] = glow;
    const vec3 white{2.2f}, gold = abGoldTell(vec3{1.9f, 1.15f, 0.15f});

    switch (e.type) {
    case EnemyType::STRAY: {
        const float ph = e.animPhase * 0.55f;   // long strides: low gravity
        const float s = std::sin(ph), c = std::cos(ph);
        const float bound = std::fabs(std::sin(ph)) * 0.38f * run;   // airborne between steps
        fileRoot = root * T({0.f, bound, 0.f});
        P.motion[E_HIPL] = RX(s * 0.75f * run); P.motion[E_HIPR] = RX(-s * 0.75f * run);
        P.motion[E_KNEE_L] = RX(-(0.15f + std::max(0.f, -c) * 0.9f) * run);
        P.motion[E_KNEE_R] = RX(-(0.15f + std::max(0.f, c) * 0.9f) * run);
        // Arms out for balance, swinging slow against the legs
        P.motion[E_ARML] = RX(-s * 0.5f * run) * RZ(0.15f + 0.35f * run);
        P.motion[E_ARMR] = RX(s * 0.5f * run) * RZ(-0.15f - 0.35f * run);
        P.motion[E_ELBOW_L] = P.motion[E_ELBOW_R] = RX(-0.3f - 0.3f * run);
        P.motion[E_TORSO] = RX(-0.12f * run) * RY(s * 0.08f * run);
        P.motion[E_WAIST] = RZ(std::sin(ph * 2.f) * 0.03f * run);
        P.motion[E_HALO] = RX(-std::cos(ph * 2.f) * 0.07f * run);       // the cradle bucks on its mounts
        P.motion[E_HEAD] = RY(std::sin(time * 0.7f + id) * 0.25f * (1.f - run)) * RX(-0.05f);
        if (e.ringSeekSet && run > 0.3f) P.motion[E_HEAD] = RX(0.15f);  // running for shelter: head down
        if (e.telegraphTimer > 0.f || e.dashTimer > 0.f) {
            const float lift = e.dashTimer > 0.f ? 1.f : smooth01c(tp);
            if (e.attack == AttackKind::LAB_SWIPE) {
                P.motion[E_ARMR] = RX(-1.9f * lift) * RZ(-0.5f * lift);
                P.motion[E_ELBOW_R] = RX(-0.9f * lift);
                P.motion[E_TORSO] = RY(0.35f * lift);
            } else {   // the lunge: crouched low, then thrown forward, both claws out
                P.motion[E_TORSO] = RX(-0.5f * lift);
                P.motion[E_ARML] = RX(-1.4f * lift) * RZ(0.2f); P.motion[E_ARMR] = RX(-1.4f * lift) * RZ(-0.2f);
                P.motion[E_KNEE_L] = P.motion[E_KNEE_R] = RX(-0.8f * lift * (e.dashTimer > 0.f ? 0.3f : 1.f));
                if (e.dashTimer <= 0.f) fileRoot = root * T({0.f, -0.25f * lift, 0.f});
            }
            const vec3 tell = e.parryWindow() ? white : st.glow * (1.f + 2.f * tp);
            P.chan[C_EYE] = tell;
        }
        if (e.labOpen > 0.f || e.staggered()) {   // exposed: slumped, the visor guttering
            P.motion[E_TORSO] = RX(0.3f); P.motion[E_HEAD] = RX(0.3f);
            P.chan[C_EYE] = st.glow * (0.4f + 0.3f * std::sin(time * 14.f));
        }
        break;
    }
    case EnemyType::HERDER: {
        const float pitch = e.dashTimer > 0.f ? 0.35f : e.telegraphTimer > 0.f ? -0.2f * tp : 0.08f + 0.06f * run;
        fileRoot = root * RX(pitch) * RZ(std::sin(time * 1.3f + id) * 0.05f);
        const float steer = std::sin(time * 0.9f + id) * 0.25f;
        const float flare = e.dashTimer > 0.f ? 0.9f : e.telegraphTimer > 0.f ? 0.6f * tp : 0.f;
        P.motion[E_WING_L] = RX(steer + flare) * RZ(0.1f * std::sin(time * 2.f));
        P.motion[E_WING_R] = RX(-steer + flare) * RZ(-0.1f * std::sin(time * 2.f));
        P.motion[E_HEAD] = RY(std::sin(time * 0.6f + id) * 0.6f);   // the lamp sweeps
        P.motion[E_SHIELD] = RX(e.ringPush >= 0 ? 0.15f : 0.f);
        P.chan[C_CORE] = vec3{0.7f, 0.75f, 0.8f} * (0.3f + 0.4f * run + (e.dashTimer > 0.f ? 2.f : 0.f));   // gas jets, pale
        if (e.telegraphTimer > 0.f || e.dashTimer > 0.f) { P.chan[C_EYE] = gold * (1.f + tp); glow = gold; }
        if (e.staggered()) P.chan[C_EYE] = st.glow * 0.3f;
        break;
    }
    case EnemyType::TALLY: {
        // A count every 1.5 s: the drum turns a notch, a bead slides over, the lamp blinks
        const float t = time * (1.f / 1.5f) + id * 0.37f, k = std::floor(t), f = t - k;
        const float notch = (k + smooth01c(f * 4.f)) * 0.2618f;
        P.motion[E_HALO] = RY(notch);
        P.motion[E_ARML] = T({0.04f * std::sin(k * 1.7f) * smooth01c(f * 3.f), 0.f, 0.f});
        P.motion[E_ARMR] = T({-0.03f * std::cos(k * 2.3f) * smooth01c(f * 3.f), 0.f, 0.f});
        P.motion[E_HEAD] = RY(time * 0.4f);
        P.chan[C_CORE] = vec3{2.6f, 1.5f, 0.4f} * (f < 0.12f ? 1.6f : 0.35f);
        P.chan[C_EYE] = st.glow * (0.8f + 0.2f * std::sin(time * 3.f));
        if (e.staggered()) P.chan[C_CORE] = vec3{0.3f, 0.15f, 0.05f};
        break;
    }
    case EnemyType::PORTER: {
        const float ph = e.animPhase, s = std::sin(ph), c = std::cos(ph);
        fileRoot = root * RZ(s * 0.06f * run) * T({0.f, -std::fabs(c) * 0.05f * run, 0.f});
        P.motion[E_HIPL] = RX(s * 0.4f * run); P.motion[E_HIPR] = RX(-s * 0.4f * run);
        P.motion[E_KNEE_L] = RX(-(0.1f + std::max(0.f, -c) * 0.5f) * run);
        P.motion[E_KNEE_R] = RX(-(0.1f + std::max(0.f, c) * 0.5f) * run);
        P.motion[E_TORSO] = RX(-0.1f) * RZ(-s * 0.05f * run);
        P.motion[E_ARML] = RX(-s * 0.25f * run) * RZ(0.12f); P.motion[E_ARMR] = RX(s * 0.25f * run) * RZ(-0.12f);
        P.motion[E_ELBOW_L] = P.motion[E_ELBOW_R] = RX(-0.35f);
        P.motion[E_SHIELD] = RX(std::sin(ph * 2.f + 0.6f) * 0.04f * run + std::sin(time * 0.8f) * 0.01f) * RZ(s * 0.04f * run);
        if (e.telegraphTimer > 0.f) {
            const float lift = smooth01c(tp * 1.4f);
            if (e.attack == AttackKind::SLAM) {
                P.motion[E_ARML] = RX(-2.5f * lift) * RZ(0.3f); P.motion[E_ARMR] = RX(-2.5f * lift) * RZ(-0.3f);
                P.motion[E_ELBOW_L] = P.motion[E_ELBOW_R] = RX(-0.4f * lift);
                P.motion[E_TORSO] = RX(0.15f * lift);
                P.chan[C_EYE] = gold * (1.f + tp);
            } else {   // a throw: the right arm hauls back with a scoop of ice
                P.motion[E_ARMR] = RX(0.9f * lift) * RZ(-0.3f);
                P.motion[E_ELBOW_R] = RX(-1.2f * lift);
                P.motion[E_TORSO] = RY(-0.35f * lift);
                P.chan[C_EYE] = st.glow * (1.f + 2.f * tp);
            }
        }
        if (e.staggered()) { P.motion[E_TORSO] = RX(-0.35f); P.motion[E_HEAD] = RX(0.3f); }
        break;
    }
    default: return false;
    }
    glowOut = glow;
    return true;
}
