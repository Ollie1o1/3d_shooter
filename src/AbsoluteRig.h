#pragma once
// =============================================================================
// AbsoluteRig.h — THE ABSOLUTE's body (included by EnemyModel.h inside
// namespace rig). A 2.6 m figure of white light, built finer than anything
// else in the game: thighs, knee caps, shins, calves and feet; a waist ring
// and hanging wraps; abdomen plates, obliques, ribs, pectorals, collarbones, a
// spine and shoulder blades; a skull with brow, cheekbones and a gold face
// slit; deltoids, biceps, elbows, forearms with wraps trailing light, hands
// with jointed fingers and thumbs; a broken halo turning behind his head; in
// phase 3 a blade of light. Every tell lights the striking limb in its colour
// (white: parry; gold: dodge or block; red: get out).
// Joints: hip / knee / ankle, shoulder / elbow / wrist / knuckle. Right is -X.
// =============================================================================


// Motion: absolutePose says where his body is going (an MMA guard that never stands still, each
// blow a chamber that winds through the tell, a stroke that whips out in the last instant and
// lands as the blow does, and a follow-through back to guard); tickAbsoluteBody springs the drawn
// pose after it, so nothing he does ever jumps. The hips lead every stroke, the limb follows.
// Signs: + torsoYaw / hipYaw bring his RIGHT shoulder forward; + torsoRoll tips his head to his right.

struct AbsPose {
    float crouch = 0.f, lift = 0.f, torsoYaw = 0.f, torsoPitch = 0.f, headPitch = 0.f;
    float shR = 0.f, shRz = 0.f, elR = 0.f, shL = 0.f, shLz = 0.f, elL = 0.f;   // shoulder swing (- forward/up), splay (+ out), elbow bend
    float hipR = 0.f, kneeR = 0.f, hipL = 0.f, kneeL = 0.f;                      // thigh swing (- forward), knee bend
    float fistR = 1.f, fistL = 1.f;                                              // 0 an open hand .. 1 a fist
    float hipYaw = 0.f, torsoRoll = 0.f, shrug = 0.f;                            // his hips' turn, a lean to the side, shoulders up
    float stanceW = 0.04f, ankle = 0.6f;                                         // legs' splay; how flat the feet stay (1: flat on the floor)
    float plant = 0.f, dip = 0.f;                                                // 1: feet on the floor (crouch from the legs); a sink into the knees
    int   glow = -1;   // what lights with the tell: 0 right arm, 1 left arm, 2 right leg, 3 both arms, 4 the blade, 5 all of him
    float omega = 14.f;   // how hard his body chases this pose (a spring's rate, per second)
};
// (the springs' channels, in Enemy::abBody)
static float AbsPose::* const AB_CH[] = {
    &AbsPose::crouch, &AbsPose::lift, &AbsPose::torsoYaw, &AbsPose::torsoPitch, &AbsPose::headPitch,
    &AbsPose::shR, &AbsPose::shRz, &AbsPose::elR, &AbsPose::shL, &AbsPose::shLz, &AbsPose::elL,
    &AbsPose::hipR, &AbsPose::kneeR, &AbsPose::hipL, &AbsPose::kneeL, &AbsPose::fistR, &AbsPose::fistL,
    &AbsPose::hipYaw, &AbsPose::torsoRoll, &AbsPose::shrug, &AbsPose::stanceW, &AbsPose::ankle, &AbsPose::plant};
static constexpr int AB_NCH = (int)(sizeof(AB_CH) / sizeof(AB_CH[0]));
static_assert(AB_NCH <= Enemy::AB_BODY, "Enemy::abBody holds every channel");
static constexpr int AB_CORE = 5;   // (channels before this, and hipYaw / torsoRoll / shrug, are his core: they lead a stroke)
inline bool absCore(int k) { return k < AB_CORE || k == 17 || k == 18 || k == 19; }

// How far his hips sit below standing tall, for these legs (the longer leg's foot on the floor)
inline float absLegDrop(const AbsPose& P) {
    auto len = [&](float h, float k) {
        const float cw = std::cos(P.stanceW);
        return 0.58f * cw * std::cos(h) + 0.55f * (std::cos(k) * cw * std::cos(h) - std::sin(k) * std::sin(h));
    };
    return 1.129f - std::max(len(P.hipR, P.kneeR), len(P.hipL, P.kneeL));
}
// Sink his hips by d, by bending both knees (hips forward as they go) - his feet stay where they are
inline void absSink(AbsPose& P, float d) {
    if (d <= 0.f) return;
    const float base = absLegDrop(P);
    float lo = 0.f, hi = 1.2f;
    for (int i = 0; i < 12; ++i) {
        float a = 0.5f * (lo + hi);
        AbsPose Q = P; Q.hipR -= a; Q.hipL -= a; Q.kneeR += 2.f * a; Q.kneeL += 2.f * a;
        (absLegDrop(Q) - base < d ? lo : hi) = a;
    }
    P.hipR -= lo; P.hipL -= lo; P.kneeR += 2.f * lo; P.kneeL += 2.f * lo;
}

inline float absEaseOut(float x) { x = glm::clamp(x, 0.f, 1.f); return 1.f - (1.f - x) * (1.f - x) * (1.f - x); }
inline float absBackOut(float x) { x = glm::clamp(x, 0.f, 1.f) - 1.f; return 1.f + x * x * (2.4f * x + 1.4f); }   // past it, and back

inline AbsPose absMix(const AbsPose& a, const AbsPose& b, float core, float limb) {
    AbsPose o = b;
    for (int k = 0; k < AB_NCH; ++k) {
        const float t = absCore(k) ? core : limb;
        o.*AB_CH[k] = a.*AB_CH[k] + (b.*AB_CH[k] - a.*AB_CH[k]) * t;
    }
    return o;
}

// His guard: an MMA stance, orthodox - hips bladed, lead (left) foot forward, sat low in the knees,
// shoulders hunched over a tucked chin, the lead hand out and half open, measuring you, the rear
// fist on his cheek. It lives: his weight rocks, his head slips side to side, the lead hand paws,
// and every few seconds a feint - the rear shoulder dips as if the cross were coming.
inline AbsPose absoluteGuard(const Enemy& e, float time) {
    AbsPose P;
    P.plant = 1.f; P.ankle = 1.f; P.stanceW = 0.2f; P.shrug = 0.05f;
    P.hipYaw = -0.45f; P.torsoYaw = 0.17f; P.torsoPitch = 0.42f; P.headPitch = -0.32f;
    P.hipL = -0.45f; P.kneeL = 0.65f; P.hipR = 0.f; P.kneeR = 0.5f;
    P.shL = -0.85f; P.shLz = -0.06f; P.elL = 1.3f; P.fistL = 0.9f;
    P.shR = -0.5f; P.shRz = -0.1f; P.elR = 2.05f; P.fistR = 1.f;
    P.dip = 0.08f;
    const float t = time;
    P.dip += 0.035f * (0.5f + 0.5f * std::sin(t * 4.8f));                    // a bounce in the knees
    P.torsoPitch += 0.035f * std::sin(t * 2.4f);                              // his weight rocking
    const float weave = std::sin(t * 1.25f) * 0.65f + std::sin(t * 0.77f + 1.3f) * 0.35f;
    P.torsoRoll = 0.09f * weave; P.torsoYaw += 0.07f * weave;                  // head movement
    const float paw = std::pow(std::max(0.f, std::sin(t * 2.1f)), 8.f) * (std::sin(t * 0.61f) > 0.f ? 1.f : 0.35f);
    P.shL -= 0.3f * paw; P.elL -= 0.55f * paw; P.torsoYaw -= 0.06f * paw;   // the lead hand reaching, measuring
    P.shR += 0.05f * std::sin(t * 3.3f); P.elR += 0.07f * std::cos(t * 3.3f);
    const float feint = std::pow(std::max(0.f, std::sin(t * 1.37f + 2.f)), 14.f);
    P.dip += 0.08f * feint; P.torsoYaw += 0.2f * feint; P.torsoRoll += 0.08f * feint; P.shR -= 0.25f * feint; P.elR -= 0.35f * feint;
    P.shrug += 0.015f * std::sin(t * 1.1f);                                   // breathing
    // on the move: short shuffling steps that keep the stance; at a run he squares up, leans in, guard still up
    const float stride = glm::clamp(e.moveSpeed / Enemy::AB_RUN, 0.f, 1.2f), ph = e.animPhase;
    if (stride > 0.05f) {
        const float run = glm::clamp((stride - 0.35f) / 0.5f, 0.f, 1.f);
        const float sw = std::sin(ph) * (0.25f + 0.4f * run) * stride;
        P.hipR += sw; P.hipL -= sw;
        P.kneeR += std::max(0.f, -std::sin(ph)) * (0.5f + 0.7f * run) * stride;
        P.kneeL += std::max(0.f, std::sin(ph)) * (0.5f + 0.7f * run) * stride;
        P.hipYaw += 0.35f * run; P.torsoYaw -= 0.1f * run; P.torsoPitch += 0.2f * run;
        P.shR += 0.12f * std::sin(ph) * run; P.shL -= 0.12f * std::sin(ph) * run;
        P.torsoRoll += 0.05f * std::sin(ph) * stride; P.dip += 0.04f * stride;
    }
    return P;
}

// A move's two keys on top of his guard: the chamber it winds into (strike false) and the blow at
// full stretch (strike true). Moves with no stroke hold one shape.
inline void absoluteMove(AbsPose& P, AttackKind a, bool strike, float tp) {
    const bool k = strike;
    switch (a) {
    case AttackKind::MJAB:        // MORNINGSTAR's jab: the same snap, the size of a man
    case AttackKind::AJAB:        // the lead hand, straight and snapping, a step in behind it
        P.glow = 1;
        if (!k) { P.torsoYaw = 0.12f; P.dip += 0.05f; P.shL = -0.85f; P.elL = 1.7f; }
        else { P.hipYaw = -0.7f; P.torsoYaw = -0.18f; P.torsoRoll = -0.05f; P.shL = -1.55f; P.shLz = -0.08f; P.elL = 0.06f; P.fistL = 1.f; P.hipL = -0.48f; P.kneeL = 0.42f; }
        break;
    case AttackKind::ACROSS:      // the rear hand down the middle, the hips turning through it, the lead hand home to his chin
        P.glow = 0;
        if (!k) { P.hipYaw = -0.75f; P.torsoYaw = -0.15f; P.dip += 0.08f; P.shR = -0.5f; P.elR = 2.35f; }
        else { P.hipYaw = -0.15f; P.torsoYaw = 0.7f; P.torsoPitch = 0.26f; P.torsoRoll = -0.06f; P.shR = -1.6f; P.shRz = -0.06f; P.elR = 0.05f;
               P.hipR = 0.38f; P.kneeR = 0.38f; P.shL = -0.65f; P.elL = 2.05f; P.fistL = 1.f; }
        break;
    case AttackKind::AHOOK:       // the lead hand round the side: coiled back, then the hips whip it across
        P.glow = 1;
        if (!k) { P.torsoYaw = 0.5f; P.torsoRoll = -0.14f; P.dip += 0.1f; P.shL = -1.2f; P.shLz = 0.85f; P.elL = 1.45f; P.fistL = 1.f; }
        else { P.hipYaw = -0.85f; P.torsoYaw = -0.7f; P.torsoRoll = 0.1f; P.shL = -1.5f; P.shLz = -0.3f; P.elL = 1.45f; P.fistL = 1.f; P.dip += 0.04f; }
        break;
    case AttackKind::AUPPER:      // down low on his right, then up through you, his whole body rising behind the fist
        P.glow = 0;
        if (!k) { P.dip += 0.32f; P.torsoRoll = 0.2f; P.torsoPitch = 0.34f; P.torsoYaw = -0.2f; P.shR = 0.35f; P.shRz = 0.05f; P.elR = 1.5f; }
        else { P.dip = 0.f; P.torsoPitch = -0.22f; P.torsoYaw = 0.5f; P.torsoRoll = -0.08f; P.headPitch = -0.12f; P.shR = -2.55f; P.shRz = -0.1f; P.elR = 0.95f;
               P.hipR = 0.3f; P.kneeR = 0.2f; P.hipL = -0.15f; P.kneeL = 0.2f; }
        break;
    case AttackKind::ADROP:       // up off the floor, the right leg chambered, then driven out
        P.glow = 2; P.plant = 0.f; P.ankle = 0.3f; P.crouch = 0.f; P.lift = 2.6f * smooth01(tp); P.torsoPitch = -0.35f; P.hipYaw = 0.f;
        P.hipR = k ? -1.55f : -0.7f; P.kneeR = k ? 0.05f : 1.9f; P.hipL = 0.45f; P.kneeL = 1.7f;
        P.shR = 0.6f; P.shL = 0.5f; P.shRz = P.shLz = 0.7f; P.elR = P.elL = 0.4f;
        break;
    case AttackKind::ARAY: case AttackKind::AZENITH: case AttackKind::ASEAMS: case AttackKind::AHALO:
        P.glow = 0; P.fistR = 0.f;
        if (a == AttackKind::ARAY) { P.hipYaw = -0.2f; P.shR = -1.57f; P.shRz = -0.05f; P.elR = 0.f; P.torsoYaw = 0.45f; P.shL = -0.3f; P.elL = 0.6f; }
        else { P.shR = -3.0f; P.elR = 0.1f; P.shL = -0.2f; P.shLz = 0.5f; P.elL = 0.5f; P.headPitch = -0.35f; P.torsoPitch = -0.1f; }   // one hand to the sky
        break;
    case AttackKind::ASTOMP:      // the right knee up to his chest, then down through the floor
        P.glow = 2; P.shRz = P.shLz = 0.8f; P.shR = P.shL = -0.4f; P.elR = P.elL = 0.6f;
        if (!k) { P.hipR = -1.5f; P.kneeR = 1.8f; P.torsoPitch = -0.08f; P.ankle = 0.4f; }
        else { P.hipR = -0.25f; P.kneeR = 0.45f; P.dip += 0.18f; P.torsoPitch = 0.35f; P.headPitch = 0.1f; }
        break;
    case AttackKind::ASERPENT:
        P.glow = 3; P.fistR = P.fistL = 0.2f; P.hipYaw = -0.25f; P.torsoYaw = 0.25f;
        P.shR = P.shL = -1.45f; P.shRz = P.shLz = -0.25f; P.elR = P.elL = 0.25f;
        break;
    case AttackKind::ATELEPORT: case AttackKind::AHOP: case AttackKind::ADASH:
        P.torsoPitch = 0.4f; P.shR = P.shL = 0.7f; P.elR = P.elL = 0.4f; P.dip += 0.18f;
        break;
    case AttackKind::ASWEEP:      // the blade cocked back behind his right shoulder, then flat across
        P.glow = 4; P.shL = -0.8f; P.elL = 1.6f; P.dip += 0.12f;
        if (!k) { P.torsoYaw = -0.85f; P.shR = -1.3f; P.shRz = 0.8f; P.elR = 0.6f; }
        else { P.hipYaw = -0.25f; P.torsoYaw = 0.85f; P.shR = -1.35f; P.shRz = -0.25f; P.elR = 0.15f; }
        break;
    case AttackKind::ALUNGE:      // the blade drawn back, then the long step and the point
        P.glow = 4;
        if (!k) { P.dip += 0.22f; P.torsoYaw = -0.2f; P.shR = -0.4f; P.elR = 1.8f; }
        else { P.hipYaw = 0.2f; P.torsoYaw = 0.45f; P.torsoPitch = 0.35f; P.shR = -1.6f; P.elR = 0.05f; P.hipR = -0.9f; P.kneeR = 0.95f; P.hipL = 0.6f; P.kneeL = 0.3f; }
        break;
    case AttackKind::ACRESCENT:   // over his head, then down
        P.glow = 4;
        if (!k) { P.shR = -2.85f; P.shRz = 0.1f; P.elR = 0.4f; P.torsoPitch = -0.2f; }
        else { P.shR = -0.95f; P.elR = 0.15f; P.torsoPitch = 0.3f; P.dip += 0.12f; }
        break;
    case AttackKind::MBACKHAND:   // MORNINGSTAR: the backfist as a colossus swings it, his whole body behind it
    case AttackKind::ABACKFIST:   // he spins away (his back to you), and the back of the fist comes round with him
        P.glow = a == AttackKind::MBACKHAND ? 5 : 1;
        if (!k) { P.hipYaw = -0.05f; P.torsoYaw = 1.1f; P.shL = -1.3f; P.shLz = -0.1f; P.elL = 1.95f; P.dip += 0.08f; }
        else { P.hipYaw = -0.95f; P.torsoYaw = -1.0f; P.shL = -1.5f; P.shLz = 1.25f; P.elL = 0.15f; }
        break;
    case AttackKind::AELBOW:      // short: the rear elbow, flat and turning over
        P.glow = 0;
        if (!k) { P.torsoYaw = -0.4f; P.shR = -0.9f; P.shRz = 0.3f; P.elR = 2.5f; }
        else { P.hipYaw = -0.2f; P.torsoYaw = 0.65f; P.shR = -1.5f; P.shRz = 0.95f; P.elR = 2.7f; P.dip += 0.1f; }
        break;
    case AttackKind::AROUNDHOUSE: // the knee up, then the hips turn over and the shin comes round at your ribs
        P.glow = 2; P.ankle = 0.35f; P.shL = -1.25f; P.elL = 2.f;
        if (!k) { P.hipR = -0.7f; P.kneeR = 1.8f; P.torsoRoll = -0.15f; P.torsoPitch = -0.1f; }
        else { P.hipYaw = 0.75f; P.torsoYaw = -0.2f; P.hipR = -1.5f; P.kneeR = 0.15f; P.torsoRoll = -0.35f; P.torsoPitch = -0.15f; P.shR = 0.4f; P.shRz = 0.6f; P.elR = 0.6f; }
        break;
    case AttackKind::AHAYMAKER:   // wound right back, then over the top with everything behind it
        P.glow = 0;
        if (!k) { P.hipYaw = -0.8f; P.torsoYaw = -1.0f; P.torsoPitch = -0.2f; P.shR = 0.9f; P.shRz = 0.7f; P.elR = 1.8f; P.dip += 0.1f; }
        else { P.hipYaw = -0.1f; P.torsoYaw = 0.85f; P.torsoPitch = 0.38f; P.shR = -2.2f; P.shRz = 0.1f; P.elR = 0.4f; P.dip += 0.2f; }
        break;
    case AttackKind::ACHARGE:     // head down, lead shoulder first
        P.glow = 5; P.torsoPitch = 0.75f; P.hipYaw = -0.6f; P.torsoYaw = -0.4f; P.shL = -0.3f; P.shLz = -0.2f; P.elL = 2.3f;
        P.shR = 0.4f; P.elR = 1.2f; P.dip += 0.25f;
        break;
    case AttackKind::AAXE:        // the heel up past his head, then straight down
        P.glow = 2; P.ankle = 0.3f; P.shR = P.shL = -0.5f; P.shRz = P.shLz = 0.7f;
        if (!k) { P.hipR = -2.6f; P.kneeR = 0.05f; P.torsoPitch = -0.3f; }
        else { P.hipR = -0.3f; P.kneeR = 0.1f; P.torsoPitch = 0.3f; P.dip += 0.15f; }
        break;
    case AttackKind::ACLAP:       // both arms wide, chest open, then palms together in front
    case AttackKind::ATHUNDER:    // (risen: the same, over your head)
        P.glow = 3; P.fistR = P.fistL = 0.f; P.shR = P.shL = -1.5f; P.elR = P.elL = 0.15f; P.hipYaw = -0.15f; P.torsoYaw = 0.15f;
        if (!k) { P.shRz = P.shLz = 1.3f; P.torsoPitch = -0.15f; }
        else { P.shRz = P.shLz = -0.2f; P.torsoPitch = 0.2f; }
        if (a == AttackKind::ATHUNDER) P.plant = 0.f;
        break;
    case AttackKind::MGRASP:      // MORNINGSTAR: the right hand up and open, then down to the floor where you stand
        P.glow = 0; P.fistR = 0.f;
        if (!k) { P.shR = -2.6f; P.shRz = 0.15f; P.elR = 0.35f; P.torsoPitch = -0.2f; P.torsoYaw = 0.3f; }
        else { P.shR = -0.95f; P.elR = 0.1f; P.torsoPitch = 0.75f; P.torsoYaw = 0.4f; P.dip += 0.35f; P.hipR = -0.6f; P.kneeR = 0.7f; }
        break;
    case AttackKind::MSUNDER:     // MORNINGSTAR: both fists over his head, then down into the floor like a hammer
        P.glow = 3; P.fistR = P.fistL = 1.f;
        if (!k) { P.shR = P.shL = -2.9f; P.shRz = P.shLz = 0.05f; P.elR = P.elL = 0.3f; P.torsoPitch = -0.25f; }
        else { P.shR = P.shL = -0.6f; P.shRz = P.shLz = -0.1f; P.elR = P.elL = 0.2f; P.torsoPitch = 0.8f; P.dip += 0.4f; P.headPitch = 0.2f; }
        break;
    case AttackKind::MFISSURE:    // MORNINGSTAR: the right fist raised, a long step, and down into the floor at you
        P.glow = 0;
        if (!k) { P.shR = -2.7f; P.shRz = 0.1f; P.elR = 0.4f; P.torsoYaw = -0.3f; P.torsoPitch = -0.15f; }
        else { P.shR = -0.7f; P.elR = 0.1f; P.torsoYaw = 0.35f; P.torsoPitch = 0.7f; P.dip += 0.35f; P.hipR = -0.6f; P.kneeR = 0.7f; }
        break;
    case AttackKind::MFLARES:     // MORNINGSTAR: both arms flung up to the ring of fire
        P.glow = 5; P.fistR = P.fistL = 0.f; P.shR = P.shL = -2.8f; P.shRz = P.shLz = k ? 0.9f : 0.5f; P.elR = P.elL = 0.1f;
        P.headPitch = -0.35f; P.torsoPitch = -0.15f;
        break;
    case AttackKind::MDROP:       // MORNINGSTAR's Eclipse drop: SKYFALL's hammer from much higher
    case AttackKind::ASKYFALL:    // knees up, both fists over his head, then down like a hammer
        P.glow = 3; P.plant = 0.f; P.ankle = 0.3f; P.hipYaw = 0.f; P.torsoYaw = 0.f;
        P.shR = P.shL = k ? -0.4f : -2.9f; P.elR = P.elL = 0.3f; P.hipR = P.hipL = -1.1f; P.kneeR = P.kneeL = 1.6f; P.torsoPitch = k ? 0.4f : -0.2f;
        break;
    case AttackKind::AMETEOR:     // one fist first, the body behind it like a comet
        P.glow = 0; P.plant = 0.f; P.ankle = 0.3f; P.hipYaw = 0.f; P.torsoYaw = 0.f;
        P.shR = -2.9f; P.elR = 0.05f; P.shL = 0.6f; P.elL = 0.4f; P.torsoPitch = 0.9f; P.hipR = P.hipL = 0.4f; P.kneeR = 0.3f; P.kneeL = 0.6f;
        break;
    case AttackKind::ATAKEOFF:    // a deep crouch, arms flung back, then up
        P.shR = P.shL = 0.6f; P.shRz = P.shLz = 0.6f; P.elR = P.elL = 0.3f; P.torsoPitch = 0.3f;
        P.dip += k ? 0.f : 0.38f;
        break;
    case AttackKind::MCOLLAPSE:   // MORNINGSTAR: the last judgment's shape as he pulls the ring of fire in
    case AttackKind::AJUDGE:
        P.glow = 5; P.plant = 0.f; P.lift = 1.2f + 0.6f * tp; P.hipYaw = 0.f; P.torsoYaw = 0.f; P.torsoRoll = 0.f; P.torsoPitch = 0.f;
        P.shR = P.shL = -1.57f; P.shRz = P.shLz = 1.25f; P.elR = P.elL = 0.f;
        P.fistR = P.fistL = 0.f; P.headPitch = -0.3f; P.hipR = P.hipL = 0.15f; P.kneeR = P.kneeL = 0.3f; P.ankle = 0.3f;
        break;
    default: break;
    }
}

// (where his feet are: planted, the hips sit on the legs; otherwise crouch is as given)
inline void absPlant(AbsPose& P) {
    if (P.plant <= 0.f) return;
    absSink(P, P.dip); P.dip = 0.f;
    P.crouch += (absLegDrop(P) - P.crouch) * glm::clamp(P.plant, 0.f, 1.f);
}

constexpr float AB_FOLLOW_T = 0.42f;   // a blow's follow-through, back to guard

inline AbsPose absolutePose(const Enemy& e, float time) {
    AbsPose G = absoluteGuard(e, time);
    if (e.abAirborne() && !e.abDiving) {   // aloft: arms swept back, legs trailing, the guard gone
        G = AbsPose{}; G.omega = 8.f; G.ankle = 0.3f;
        G.shR = G.shL = 0.55f; G.shRz = G.shLz = 0.35f; G.elR = G.elL = 0.35f; G.fistR = G.fistL = 0.6f;
        G.hipR = 0.35f; G.hipL = 0.2f; G.kneeR = 0.6f; G.kneeL = 0.9f; G.torsoPitch = 0.3f;
        G.torsoRoll = 0.05f * std::sin(time * 1.3f);
    }
    AbsPose P = G;
    const AttackKind atk = e.telegraphTimer > 0.f ? e.attack : AttackKind::NONE;
    if (atk != AttackKind::NONE) {
        const float tp = e.telegraphProgress();
        const float dur = std::max(e.telegraphDuration, 1e-3f), rem = e.telegraphTimer;
        const float stroke = glm::clamp(0.3f * dur, 0.08f, 0.16f);         // the blow itself: the last instant of the tell
        const float w = smooth01((dur - rem) / std::max(dur - stroke, 1e-3f));
        const float s = glm::clamp(1.f - rem / stroke, 0.f, 1.f);
        AbsPose C = G; absoluteMove(C, atk, false, tp);
        AbsPose K = G; absoluteMove(K, atk, true, tp);
        // winding up: the core turns first, the limb draws after it; held there (a tremor) if the tell runs long
        P = absMix(G, C, absEaseOut(w * 1.15f), smooth01(w));
        const float tremor = 0.02f * std::sin(time * 37.f) * w * (1.f - s);
        P.torsoYaw += tremor; P.shR += tremor; P.shL -= tremor;
        P.omega = 18.f;
        if (s > 0.f) {   // the stroke: the hips snap round, the limb whips out after them, past the mark and back
            P = absMix(P, K, absBackOut(s * 1.4f), absBackOut((s - 0.15f) / 0.85f));
            P.omega = 42.f;
        }
        P.glow = C.glow;
    } else if (e.abBodyLanded != AttackKind::NONE && e.abBodySince < AB_FOLLOW_T && !e.staggered() && !(e.abAirborne() && !e.abDiving)) {
        // the follow-through: held at full stretch a beat, then gathered back into his guard
        AbsPose K = G; absoluteMove(K, e.abBodyLanded, true, 1.f);
        if (K.plant > 0.f || G.plant <= 0.f) {
            const float r = smooth01((e.abBodySince - 0.06f) / (AB_FOLLOW_T - 0.06f));
            P = absMix(K, G, r, smooth01(r * 1.2f));
            P.glow = -1; P.omega = 20.f;
        }
    }
    if (e.staggered() || e.abSwallow > Enemy::MS_RISE) {   // (the swallow: on his knees, as if broken)
        P = AbsPose{}; P.omega = 20.f;
        if (e.abReeling && e.abSwallow <= 0.f) {   // thrown back on his heels, the guard knocked open
            P.plant = 1.f; P.ankle = 1.f; P.stanceW = 0.12f; P.hipR = 0.3f; P.kneeR = 0.35f; P.hipL = -0.2f; P.kneeL = 0.3f;
            P.torsoPitch = -0.4f; P.torsoRoll = 0.12f; P.shR = P.shL = 0.25f; P.shRz = P.shLz = 0.55f; P.elR = P.elL = 0.4f; P.headPitch = -0.3f;
            P.fistR = P.fistL = 0.5f;
        } else {             // broken: down on one knee, head bowed
            P.omega = 12.f;
            P.crouch = 0.62f; P.hipR = -1.45f; P.kneeR = 1.5f; P.hipL = 0.55f; P.kneeL = 2.3f;
            P.torsoPitch = 0.38f; P.headPitch = 0.4f; P.shR = P.shL = 0.15f; P.shRz = P.shLz = 0.2f; P.elR = P.elL = 0.3f;
        }
    }
    absPlant(P);
    return P;
}

// Each tick: note a blow landing (its follow-through starts), then spring his drawn body after
// absolutePose. Critically damped, solved exactly, so it's steady at any tick length.
inline void tickAbsoluteBody(Enemy& e, float dt) {
    const bool tele = e.telegraphTimer > 0.f;
    const bool ended = e.abBodyTT > 0.f && (!tele || e.attack != e.abBodyAtk || e.telegraphTimer > e.abBodyTT + 1e-4f);
    if (ended && e.abBodyTT < 0.12f && !e.staggered()) { e.abBodyLanded = e.abBodyAtk; e.abBodySince = 0.f; }
    else e.abBodySince += dt;
    e.abBodyTT = tele ? e.telegraphTimer : 0.f;
    e.abBodyAtk = tele ? e.attack : AttackKind::NONE;

    const AbsPose goal = absolutePose(e, e.age);
    if (!e.abBodyLive) {
        for (int k = 0; k < AB_NCH; ++k) { e.abBody[k] = e.abBodyPrev[k] = goal.*AB_CH[k]; e.abBodyV[k] = 0.f; }
        e.abBodyLive = true;
        return;
    }
    const float w = goal.omega, ex = std::exp(-w * dt);
    for (int k = 0; k < AB_NCH; ++k) {
        e.abBodyPrev[k] = e.abBody[k];
        const float x0 = e.abBody[k] - goal.*AB_CH[k], v0 = e.abBodyV[k], c = v0 + w * x0;
        e.abBody[k]  = goal.*AB_CH[k] + (x0 + c * dt) * ex;
        e.abBodyV[k] = (v0 - w * c * dt) * ex;
    }
}

// The pose as drawn: his sprung body (what tickAbsoluteBody has), the tell's glow from where he's going
inline AbsPose absoluteLivePose(const Enemy& e, float time) {
    AbsPose P = absolutePose(e, time);
    if (!e.abBodyLive) return P;
    for (int k = 0; k < AB_NCH; ++k) P.*AB_CH[k] = e.abBody[k];
    if (P.plant > 0.f) P.crouch += (absLegDrop(P) - P.crouch) * glm::clamp(P.plant, 0.f, 1.f);   // (the feet stay on the floor)
    return P;
}

// His torso's frame (the rig and headBox share it, so the head you see is the head you hit)
inline mat4 absoluteTorsoFrame(const Enemy& e, const mat4& root, const AbsPose& P) {
    float bob = std::fabs(std::sin(e.animPhase)) * 0.04f * glm::clamp(e.moveSpeed / 11.f, 0.f, 1.f);
    return root * T({0.f, P.lift - P.crouch + bob, 0.f}) * RY(P.hipYaw) * T({0.f, 1.30f, 0.f})
         * RY(P.torsoYaw) * RZ(P.torsoRoll) * RX(P.torsoPitch);
}
inline mat4 absoluteHeadFrame(const Enemy& e, const mat4& root, const AbsPose& P) {
    return absoluteTorsoFrame(e, root, P) * T({0.f, 0.95f, 0.f}) * RY(-0.4f * (P.torsoYaw + P.hipYaw)) * RX(P.headPitch);   // (his eyes stay on you)
}

inline void buildAbsolute(const Enemy& e, float time, Rig& r, const mat4& root) {
    const AbsPose P = absoluteLivePose(e, time);
    const float tp = e.telegraphProgress();
    const bool  hurt = e.staggered() && !e.abReeling;
    const float pulse = 0.85f + 0.15f * std::sin(time * 3.f) * (hurt ? 3.f : 1.f);
    // MORNINGSTAR: blood orange, lit from within, white light through the cracks, white eyes. In
    // TOTALITY the light goes out of him: only his eyes and his white flashes are left
    const bool ms = e.morning();
    const float dim = ms && e.abPhase >= 6 ? 0.08f : 1.f;
    const vec3 skin = ms ? vec3{0.8f, 0.15f, 0.03f} * (0.3f + 0.7f * dim) : vec3{0.58f, 0.58f, 0.64f};
    const vec3 plate = ms ? vec3{0.95f, 0.27f, 0.05f} * (0.3f + 0.7f * dim) : vec3{0.96f, 0.96f, 0.99f};
    const vec3 dark = ms ? vec3{0.1f, 0.03f, 0.02f} : vec3{0.16f, 0.16f, 0.2f};
    const vec3 light = ms ? vec3{2.f, 1.9f, 1.8f} * dim : vec3{1.6f, 1.5f, 1.25f}, gold = ms ? vec3{2.f, 1.9f, 1.8f} * dim : vec3{1.5f, 1.1f, 0.45f};
    const vec3 seam = light * (hurt ? 0.5f : 1.3f) * pulse;
    const vec3 soft = ms ? vec3{1.f, 0.17f, 0.02f} * (hurt ? 0.35f : 0.6f) * dim : light * (hurt ? 0.04f : 0.1f);   // (he glows)
    // the strike's colour, blazing as it comes (MORNINGSTAR shows only white on his body: gold and red are on the floor)
    const AbTell tell0 = e.telegraphTimer > 0.f ? abTellOf(e.attack) : AbTell::NONE;
    const AbTell tell = ms && tell0 != AbTell::WHITE ? AbTell::NONE : tell0;
    const size_t first = r.out.size();
    const vec3 tc = tell == AbTell::GOLD ? vec3{2.2f, 1.72f, 0.7f} : tell == AbTell::RED ? vec3{2.4f, 0.25f, 0.15f} : vec3{1.9f, 1.9f, 2.0f};
    const float tk = 0.5f + 3.f * tp;   // (always the brightest thing on him by the end)
    auto lit = [&](int limb, vec3 base) {
        if (tell == AbTell::NONE || P.glow < 0) return base;
        bool on = P.glow == limb || P.glow == 5 || (P.glow == 3 && (limb == 0 || limb == 1));
        return on ? tc * tk : base;
    };
    const float bob = std::fabs(std::sin(e.animPhase)) * 0.04f * glm::clamp(e.moveSpeed / 11.f, 0.f, 1.f);
    const mat4 body = root * T({0.f, P.lift - P.crouch + bob, 0.f}) * RY(P.hipYaw);

    // ---- legs: thigh, its plate and two veins, knee cap, shin, calf, its seam, foot, two toes, heel, an ankle wrap ----
    for (float s : {-1.f, 1.f}) {
        const bool right = s < 0.f;
        const int limb = right ? 2 : 6;
        mat4 hip = body * T({s * 0.15f, 1.15f, 0.f}) * RX(right ? P.hipR : P.hipL) * RZ(s * P.stanceW);
        r.box(hip, {0.f, -0.29f, 0.f}, {0.2f, 0.58f, 0.22f}, skin, soft);
        r.box(hip, {0.f, -0.27f, 0.08f}, {0.14f, 0.42f, 0.08f}, plate, soft);
        r.box(hip, {s * 0.105f, -0.3f, 0.f}, {0.02f, 0.4f, 0.03f}, dark, lit(limb, seam));
        r.box(hip, {-s * 0.105f, -0.24f, 0.02f}, {0.02f, 0.3f, 0.03f}, dark, lit(limb, seam * 0.6f));
        mat4 kn = hip * T({0.f, -0.58f, 0.f}) * RX(right ? P.kneeR : P.kneeL);
        r.box(kn, {0.f, 0.f, 0.09f}, {0.13f, 0.12f, 0.06f}, plate, lit(limb, soft));
        r.box(kn, {0.f, -0.27f, 0.f}, {0.15f, 0.52f, 0.16f}, skin, soft);
        r.box(kn, {0.f, -0.22f, -0.06f}, {0.12f, 0.32f, 0.08f}, plate);
        r.box(kn, {0.f, -0.27f, 0.085f}, {0.03f, 0.46f, 0.02f}, dark, lit(limb, seam));
        mat4 an = kn * T({0.f, -0.55f, 0.f}) * RX(-(right ? P.kneeR + P.hipR : P.kneeL + P.hipL) * P.ankle);
        r.box(an, {0.f, -0.03f, 0.05f}, {0.13f, 0.06f, 0.24f}, plate);
        r.box(an, {-0.03f, -0.035f, 0.19f}, {0.06f, 0.05f, 0.07f}, dark);
        r.box(an, {0.035f, -0.035f, 0.185f}, {0.05f, 0.045f, 0.06f}, dark);
        r.box(an, {0.f, 0.02f, -0.04f}, {0.11f, 0.08f, 0.09f}, skin);
        r.box(an, {0.f, 0.08f, 0.f}, {0.16f, 0.03f, 0.16f}, dark, lit(limb, light * 0.6f));
    }
    // ---- the waist: hips, a ring of light, wraps hanging front and back ----
    r.box(body, {0.f, 1.22f, 0.f}, {0.42f, 0.18f, 0.26f}, skin, soft);
    const mat4 waist = body * T({0.f, 1.33f, 0.f}) * RY(P.torsoYaw * 0.5f);
    for (int k = 0; k < 8; ++k)
        r.box(waist * RY(k * 0.7853982f), {0.f, 0.f, 0.17f}, {0.13f, 0.04f, 0.03f}, dark, seam * 0.8f);
    const float sway = std::sin(time * 1.9f) * 0.08f + glm::clamp(e.moveSpeed / 30.f, 0.f, 0.6f);
    for (int k = 0; k < 4; ++k) {
        float x = (k & 1) ? 0.08f : -0.08f, z = k < 2 ? 0.14f : -0.14f;
        r.box(body * T({x, 1.3f, z}) * RX((k < 2 ? -1.f : 1.f) * (sway + 0.05f * k)), {0.f, -0.28f, 0.f}, {0.1f, 0.56f, 0.015f}, dark, light * 0.3f);
    }
    // ---- the torso ----
    const mat4 torso = absoluteTorsoFrame(e, root, P);
    r.box(torso, {0.f, 0.2f, 0.f}, {0.34f, 0.4f, 0.24f}, skin, soft);                             // abdomen
    for (int row = 0; row < 3; ++row) for (float s : {-1.f, 1.f})                                  // its plates
        r.box(torso, {s * 0.06f, 0.08f + 0.12f * row, 0.125f}, {0.1f, 0.1f, 0.03f}, plate, soft);
    for (float s : {-1.f, 1.f}) {
        r.box(torso, {s * 0.19f, 0.2f, 0.02f}, {0.06f, 0.34f, 0.2f}, skin);                      // obliques
        for (int k = 0; k < 3; ++k)                                                                // ribs
            r.box(torso, {s * 0.22f, 0.42f + 0.08f * k, 0.04f}, {0.08f, 0.04f, 0.22f}, plate);
        r.box(torso, {s * 0.13f, 0.62f, 0.15f}, {0.22f, 0.2f, 0.04f}, plate, soft);              // pectorals
        r.box(torso * T({s * 0.13f, 0.78f, 0.1f}) * RZ(s * 0.15f), {0.f, 0.f, 0.f}, {0.24f, 0.035f, 0.04f}, plate);   // collarbones
        r.box(torso, {s * 0.14f, 0.8f + P.shrug * 0.6f, -0.04f}, {0.16f, 0.1f + P.shrug, 0.18f}, skin);   // trapezius
        r.box(torso, {s * 0.13f, 0.6f, -0.15f}, {0.16f, 0.2f, 0.03f}, plate);                     // shoulder blades
        r.box(torso * T({s * 0.1f, 0.32f, 0.15f}) * RZ(s * 0.5f), {0.f, 0.f, 0.f}, {0.02f, 0.3f, 0.01f}, dark, seam);   // veins
    }
    r.box(torso, {0.f, 0.6f, 0.f}, {0.56f, 0.3f, 0.3f}, skin, soft);                              // chest
    r.box(torso, {0.f, 0.5f, 0.165f}, {0.025f, 0.5f, 0.015f}, dark, seam * 1.3f);                 // sternum seam
    r.box(torso, {0.f, 0.62f, 0.17f}, {0.08f, 0.08f, 0.02f}, light, seam * (1.2f + 0.4f * std::sin(time * 6.f)));   // the light in his chest
    for (int k = 0; k < 5; ++k)                                                                    // spine
        r.box(torso, {0.f, 0.1f + 0.15f * k, -0.14f}, {0.05f, 0.06f, 0.04f}, plate, seam * 0.4f);
    // ---- neck and head: throat seam, skull, jaw, brow, the gold slit, cheekbones, crown ridge ----
    const mat4 neck = torso * T({0.f, 0.85f, 0.f});
    r.box(neck, {0.f, 0.06f, 0.f}, {0.13f, 0.14f, 0.13f}, skin);
    r.box(neck, {0.f, 0.04f, 0.066f}, {0.03f, 0.1f, 0.01f}, dark, seam);
    const mat4 head = absoluteHeadFrame(e, root, P);
    r.box(head, {0.f, 0.19f, -0.01f}, {0.24f, 0.3f, 0.27f}, plate, soft);
    r.box(head, {0.f, 0.07f, 0.04f}, {0.2f, 0.09f, 0.2f}, skin);
    r.box(head, {0.f, 0.26f, 0.13f}, {0.23f, 0.04f, 0.04f}, plate);
    const vec3 eye = ms ? vec3{3.2f, 3.1f, 3.f} : vec3{2.8f, 0.18f, 0.12f};   // two crimson eyes (MORNINGSTAR's: white), blazing through a tell
    const float eyeK = (hurt ? 0.5f : 1.f) * (tell != AbTell::NONE ? 1.f + 0.6f * tp : 1.f) * pulse;
    for (float s : {-1.f, 1.f}) {
        const float es = ms ? 1.35f : 1.f;   // (MORNINGSTAR's: wider, blank)
        r.box(head * T({s * 0.055f, 0.21f, 0.142f}) * RZ(s * 0.18f), {0.f, 0.f, 0.f}, vec3{0.07f, 0.022f, 0.012f} * vec3{es, es * 1.3f, 1.f}, dark, eye * eyeK);
        r.box(head * T({s * 0.055f, 0.21f, 0.136f}) * RZ(s * 0.18f), {0.f, 0.f, 0.f}, {0.11f, 0.05f, 0.006f}, dark, eye * 0.35f * eyeK);   // their glow
    }
    for (float s : {-1.f, 1.f}) r.box(head, {s * 0.085f, 0.15f, 0.12f}, {0.05f, 0.04f, 0.04f}, plate);
    r.box(head, {0.f, 0.35f, -0.02f}, {0.05f, 0.04f, 0.26f}, plate, soft);
    r.box(head, {0.f, 0.2f, -0.145f}, {0.2f, 0.22f, 0.03f}, skin);
    // ---- arms: deltoid, bicep, tricep, vein, elbow, forearm and its plate, three wraps and two
    // ribbons trailing light, palm, knuckles, four jointed fingers, a jointed thumb ----
    mat4 handR{1.f};
    for (float s : {-1.f, 1.f}) {
        const bool right = s < 0.f;
        const int limb = right ? 0 : 1;
        const float sh = right ? P.shR : P.shL, shz = right ? P.shRz : P.shLz, el = right ? P.elR : P.elL;
        const float fist = right ? P.fistR : P.fistL;
        mat4 shoulder = torso * T({s * 0.31f, 0.76f + P.shrug, 0.f}) * RX(sh) * RZ(s * (0.1f + shz));
        r.box(shoulder, {s * 0.02f, -0.02f, 0.f}, {0.18f, 0.16f, 0.18f}, plate, soft);
        r.box(shoulder, {0.f, -0.25f, 0.03f}, {0.12f, 0.36f, 0.11f}, skin, soft);
        r.box(shoulder, {0.f, -0.24f, -0.04f}, {0.11f, 0.32f, 0.08f}, plate);
        r.box(shoulder, {s * 0.06f, -0.25f, 0.f}, {0.02f, 0.3f, 0.02f}, dark, lit(limb, seam));
        mat4 elbow = shoulder * T({0.f, -0.48f, 0.f}) * RX(-el);
        r.box(elbow, {0.f, 0.f, -0.04f}, {0.1f, 0.08f, 0.06f}, plate);
        r.box(elbow, {0.f, -0.21f, 0.f}, {0.11f, 0.4f, 0.11f}, skin, lit(limb, soft));
        r.box(elbow, {s * 0.03f, -0.2f, 0.03f}, {0.07f, 0.3f, 0.06f}, plate, lit(limb, soft));
        for (int k = 0; k < 3; ++k)
            r.box(elbow, {0.f, -0.3f - 0.05f * k, 0.f}, {0.13f, 0.025f, 0.13f}, dark, lit(limb, light * 0.7f));
        float trail = 0.5f + glm::clamp(e.moveSpeed / 20.f, 0.f, 1.2f) + 0.12f * std::sin(time * 4.f + s);
        for (int k = 0; k < 2; ++k)
            r.box(elbow * T({(k ? 0.03f : -0.03f), -0.36f, -0.06f}) * RX(trail + 0.2f * k - (sh - el)),   // (they hang from the wrist, whichever way the forearm points)
                  {0.f, -0.16f, 0.f}, {0.025f, 0.32f, 0.008f}, dark, light * 0.5f);
        mat4 hand = elbow * T({0.f, -0.44f, 0.f});
        if (right) handR = hand;
        r.box(hand, {0.f, -0.05f, 0.f}, {0.1f, 0.1f, 0.05f}, skin, lit(limb, soft));
        r.box(hand, {0.f, -0.1f, 0.02f}, {0.1f, 0.02f, 0.03f}, plate, lit(limb, soft));
        // the gauntlet: a heavier shell, rune bands at the wrist and knuckles, pale violet-blue
        // (no tell's colour); the striking one flares to its tell
        const vec3 rune = ms ? vec3{1.9f, 1.8f, 1.7f} * dim : vec3{0.6f, 0.7f, 1.7f};
        const vec3 rk = lit(limb, rune * (0.85f + 0.15f * std::sin(time * 3.f + s)));
        r.box(hand, {0.f, -0.06f, 0.f}, {0.125f, 0.12f, 0.075f}, dark * 1.4f, rune * 0.08f);
        r.box(hand, {0.f, 0.02f, 0.f}, {0.135f, 0.022f, 0.085f}, dark, rk);            // the wrist band
        r.box(hand, {0.f, -0.115f, 0.041f}, {0.115f, 0.018f, 0.012f}, dark, rk);       // across the knuckles
        for (int f = 0; f < 4; ++f)
            r.box(hand, {-0.035f + 0.023f * f, -0.09f, 0.043f}, {0.008f, 0.03f, 0.008f}, dark, rk * 0.8f);   // a rune per knuckle
        for (int f = 0; f < 4; ++f) {
            mat4 f1 = hand * T({-0.035f + 0.023f * f, -0.1f, 0.01f}) * RX(-fist * 0.8f);
            r.box(f1, {0.f, -0.03f, 0.f}, {0.02f, 0.06f, 0.022f}, skin, lit(limb, soft));
            mat4 f2 = f1 * T({0.f, -0.06f, 0.f}) * RX(-fist * 1.1f);
            r.box(f2, {0.f, -0.025f, 0.f}, {0.018f, 0.05f, 0.02f}, plate, lit(limb, soft * 1.5f));
        }
        mat4 t1 = hand * T({-s * 0.055f, -0.04f, 0.03f}) * RZ(-s * 0.5f) * RX(-fist * 0.6f);
        r.box(t1, {0.f, -0.025f, 0.f}, {0.022f, 0.05f, 0.022f}, skin, lit(limb, soft));
        r.box(t1 * T({0.f, -0.05f, 0.f}) * RX(-fist * 0.6f), {0.f, -0.02f, 0.f}, {0.02f, 0.04f, 0.02f}, plate, lit(limb, soft));
    }
    // ---- the broken halo: thirteen shards of a ring of sixteen, an inner ring, turning behind his head ----
    const mat4 halo = head * T({0.f, 0.24f, -0.34f}) * RX(0.15f + P.torsoPitch * 0.3f) * RZ(time * 0.35f);
    for (int k = 0; k < 16; ++k) {
        if (k == 3 || k == 4 || k == 11) continue;   // broken out
        float a = k * 0.3926991f, jit = 0.03f * std::sin(k * 7.1f);
        r.box(halo * RZ(a), {0.f, 0.5f + jit, 0.f}, {0.17f, 0.045f, 0.03f}, gold, gold * (hurt ? 0.5f : 1.6f) * pulse);
    }
    for (int k = 0; k < 8; ++k)
        r.box(halo * RZ(k * 0.7853982f + 0.39f), {0.f, 0.34f, 0.f}, {0.16f, 0.02f, 0.02f}, light, seam * 0.9f);
    // ---- the blade of light (phase 3): grip, guard, three lengths of blade, its edge ----
    if (e.abBlade) {
        const mat4 sw = handR * T({0.f, -0.07f, 0.03f}) * RX(1.5707963f);
        const vec3 bl = tell != AbTell::NONE && P.glow == 4 ? tc * tk : vec3{1.9f, 1.75f, 2.1f};
        r.box(sw, {0.f, 0.f, 0.f}, {0.05f, 0.18f, 0.05f}, dark, light * 0.5f);
        r.box(sw, {0.f, -0.1f, 0.f}, {0.32f, 0.04f, 0.07f}, plate, gold);
        for (int k = 0; k < 3; ++k)
            r.box(sw, {0.f, -0.35f - 0.45f * k, 0.f}, {0.09f - 0.02f * k, 0.46f, 0.025f}, plate, bl * (0.9f + 0.1f * k));
        r.box(sw, {0.f, -0.75f, 0.015f}, {0.02f, 1.3f, 0.01f}, light, bl * 1.4f);
    }
    // MORNINGSTAR's white tell: his whole body goes white-hot as the blow comes (the one thing to parry)
    if (ms && tell == AbTell::WHITE) {
        const float k = smooth01(tp * 1.2f);
        for (size_t j = first; j < r.out.size(); ++j) {
            r.out[j].color = glm::mix(r.out[j].color, vec3{1.f}, 0.7f * k);
            r.out[j].emissive = glm::mix(r.out[j].emissive, vec3{2.2f, 2.15f, 2.1f}, 0.25f + 0.65f * k);
        }
    }
}
