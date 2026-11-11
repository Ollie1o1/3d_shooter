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
    // the sword hand's wrist (phase 3): the blade leaves the fist out of the thumb side, cocked
    // `bladeCock` toward the forearm's line (0: square to it, pi/2: along it), the fist turned
    // `bladeRoll` about the forearm. Solved per pose (hand where it should be, blade where it points)
    float bladeRoll = 0.f, bladeCock = 1.3f;
    // the wrists: the forearm's twist (+ turns the palm in toward his centre line: 0 with the arm out in
    // front is palm down, pi/2 palm in, pi palm up), a bend (+ toward the palm) and a tilt (+ the fist's
    // top leans in toward his centre line). Each side mirrors the other: the same numbers mean the same.
    float twR = 0.f, twL = 0.f, flR = 0.f, flL = 0.f, dvR = 0.f, dvL = 0.f;
    float abdR = 0.f, abdL = 0.f;   // one leg lifted out to the side (a roundhouse's chamber), on top of stanceW
    float swayX = 0.f, swayZ = 0.f; // his pelvis off the middle of his stance (+X his left, +Z forward): his weight, over one foot or the other
    float legIK = 1.f;              // 1: his feet stay where they're planted and the legs reach for them (absFeetTick); 0: this move poses the legs itself
    int   glow = -1;   // what lights with the tell: 0 right arm, 1 left arm, 2 right leg, 3 both arms, 4 the blade, 5 all of him
    float omega = 14.f;   // how hard his body chases this pose (a spring's rate, per second)
};
// (the springs' channels, in Enemy::abBody)
static float AbsPose::* const AB_CH[] = {
    &AbsPose::crouch, &AbsPose::lift, &AbsPose::torsoYaw, &AbsPose::torsoPitch, &AbsPose::headPitch,
    &AbsPose::shR, &AbsPose::shRz, &AbsPose::elR, &AbsPose::shL, &AbsPose::shLz, &AbsPose::elL,
    &AbsPose::hipR, &AbsPose::kneeR, &AbsPose::hipL, &AbsPose::kneeL, &AbsPose::fistR, &AbsPose::fistL,
    &AbsPose::hipYaw, &AbsPose::torsoRoll, &AbsPose::shrug, &AbsPose::stanceW, &AbsPose::ankle, &AbsPose::plant,
    &AbsPose::bladeRoll, &AbsPose::bladeCock,
    &AbsPose::twR, &AbsPose::twL, &AbsPose::flR, &AbsPose::flL, &AbsPose::dvR, &AbsPose::dvL, &AbsPose::abdR, &AbsPose::abdL,
    &AbsPose::swayX, &AbsPose::swayZ};
static constexpr int AB_NCH = (int)(sizeof(AB_CH) / sizeof(AB_CH[0]));
static_assert(AB_NCH <= Enemy::AB_BODY, "Enemy::abBody holds every channel");
static constexpr int AB_CORE = 5;   // (channels before this, and hipYaw / torsoRoll / shrug, are his core: they lead a stroke)
inline bool absCore(int k) { return k < AB_CORE || k == 17 || k == 18 || k == 19; }

// How far his hips sit below standing tall, for these legs (the longer leg's foot on the floor)
inline float absLegDrop(const AbsPose& P) {
    auto len = [&](float h, float k, float abd) {
        const float cw = std::cos(P.stanceW + abd);
        return 0.58f * cw * std::cos(h) + 0.55f * (std::cos(k) * cw * std::cos(h) - std::sin(k) * std::sin(h));
    };
    return 1.129f - std::max(len(P.hipR, P.kneeR, P.abdR), len(P.hipL, P.kneeL, P.abdL));
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
    o.legIK = limb > 0.5f ? b.legIK : a.legIK;   // (whichever pose the legs are mostly in says who places the feet)
    return o;
}

// His guard: an MMA stance, orthodox - hips bladed, lead (left) foot forward, sat low in the knees,
// shoulders hunched over a tucked chin. The rear (right) fist high at his cheek, elbow tucked to his
// ribs, forearm upright; the lead (left) fist lower and further out, forearm angled up at you, elbow
// down. Both forearms turned so the palms face in and the knuckles out, the fists leaning in toward
// his centre line. It lives: his weight rocks, his head slips side to side, the lead hand paws,
// and every few seconds a feint - the rear shoulder dips as if the cross were coming.
inline AbsPose absoluteGuard(const Enemy& e, float time) {
    AbsPose P;
    P.plant = 1.f; P.ankle = 1.f; P.stanceW = 0.2f; P.shrug = 0.05f;
    P.hipYaw = -0.45f; P.torsoYaw = 0.17f; P.torsoPitch = 0.38f; P.headPitch = -0.3f;
    P.hipL = -0.45f; P.kneeL = 0.65f; P.hipR = 0.f; P.kneeR = 0.5f;
    P.shL = -1.05f; P.shLz = -0.22f; P.elL = 1.45f; P.fistL = 1.f;                      // lead: lower, out in front
    P.twL = 1.75f; P.flL = 0.1f; P.dvL = 0.3f;
    P.shR = -0.95f; P.shRz = -0.3f; P.elR = 2.45f; P.fistR = 1.f;                       // rear: high at his cheek, elbow down
    P.twR = 2.0f; P.flR = 0.1f; P.dvR = 0.35f;
    if (e.abBlade) {   // the sword up in front of his chest, point high and toward you, across his centre line
        P.shR = -0.13f; P.shRz = -0.6f; P.elR = 2.06f; P.bladeRoll = -0.54f; P.bladeCock = 0.52f;
        P.twR = P.flR = P.dvR = 0.f;   // (the sword's wrist is solved with the blade)
    }
    P.dip = 0.08f;
    const float t = e.morning() ? time * 0.58f : time;   // (MORNINGSTAR's guard lives slower: a colossus breathing, not a duellist bouncing)
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
    // on the move (animPhase walks with his feet: abGaitTick): short steps that keep the stance; at
    // a run he squares up and leans in, the guard still up. The swinging leg's knee folds as it
    // passes under him; the hips turn with the legs, the shoulders and arms against them
    const float stride = glm::clamp(std::fabs(e.abGait) / Enemy::AB_RUN, 0.f, 1.2f), ph = e.animPhase;
    if (stride > 0.03f) {
        const float run = glm::clamp((stride - 0.35f) / 0.5f, 0.f, 1.f);
        const float on = glm::clamp(stride * 2.5f, 0.f, 1.f);   // (even a slow step is a real step)
        const float dir = e.abGait < 0.f ? -1.f : 1.f;          // (backing off: the swing runs the other way)
        const float sn = std::sin(ph), cs = std::cos(ph);
        const float sw = sn * (0.22f + 0.43f * run) * on;
        P.hipR += sw; P.hipL -= sw;
        const float fold = (0.45f + 0.95f * run) * on;
        P.kneeR += 0.08f * on + std::max(0.f, -cs * dir) * fold;
        P.kneeL += 0.08f * on + std::max(0.f, cs * dir) * fold;
        P.ankle -= 0.35f * run;                                   // (off the toes at a run)
        P.hipYaw += 0.38f * glm::clamp(stride * 1.5f, 0.f, 1.f) - 0.2f * sn * (0.4f + 0.6f * run) * on;
        P.torsoYaw += -0.1f * run + 0.32f * sn * (0.4f + 0.6f * run) * on;
        P.torsoPitch += 0.22f * run;
        const float pump = (0.1f + 0.3f * run) * on;
        P.shR -= pump * sn; P.shL += pump * sn;                   // each arm with the opposite leg
        P.elR += 0.15f * pump * cs; P.elL -= 0.15f * pump * cs;
        P.torsoRoll += 0.05f * cs * on; P.dip += 0.04f * stride;
    }
    return P;
}
// (his body's rise and fall over a stride: highest as the legs scissor, in a run's flight)
inline float absBob(const Enemy& e) {
    return std::fabs(std::sin(e.animPhase)) * 0.06f * glm::clamp(std::fabs(e.abGait) / 11.f, 0.f, 1.f);
}

// A move's two keys on top of his guard: the chamber it winds into (strike false) and the blow at
// full stretch (strike true). Moves with no stroke hold one shape.
// The sword arm (phase 3) for a move: hand and blade solved together so the edge leads every cut.
// His right-hand punches become sword work: the cross a backhand cut, the uppercut a rising cut,
// the haymaker a diagonal cut from over his right shoulder. (Only the look: tells and reach are the
// punches'.) Moves that fling the arm out carry the blade along it; the rest keep the guard's grip.
inline void absoluteMoveBody(AbsPose& P, AttackKind a, bool strike, float tp);
inline void absSwordArm(AbsPose& P, AttackKind a, bool k) {
    using A = AttackKind;
    auto arm = [&](float sh, float shz, float el, float roll, float cock) { P.shR = sh; P.shRz = shz; P.elR = el; P.bladeRoll = roll; P.bladeCock = cock; };
    P.fistR = 1.f; P.twR = P.flR = P.dvR = 0.f;   // (the blade solves the sword wrist)
    switch (a) {
    case A::ASWEEP:    if (!k) arm(-0.62f, -0.6f, 2.51f, -2.72f, 0.67f); else arm(-0.63f, 0.4f, 1.7f, -2.f, 1.9f);   break;   // cocked back over the shoulder, flat across
    case A::ALUNGE:    if (!k) arm(1.f, -0.05f, 2.37f, -0.73f, 0.62f);  else arm(-1.02f, 0.6f, 0.7f, -0.67f, 1.66f); break;   // drawn back at the hip, the point driven out
    case A::ACRESCENT: if (!k) arm(-1.73f, -0.36f, 1.57f, -1.56f, 0.77f); else arm(-0.69f, -0.6f, 0.84f, -1.18f, 1.9f); break;  // over his head, then down through you
    case A::ACROSS:    // a backhand cut: wound over his left shoulder, then flat out to his right (the torso turns the other way to the punch)
        if (!k) { arm(-0.23f, -0.6f, 2.48f, -0.31f, -0.37f); P.hipYaw = -0.2f; P.torsoYaw = 0.6f; P.torsoPitch = 0.3f; }
        else    { arm(-1.25f, -0.6f, 0.54f, -0.24f, 1.65f);  P.hipYaw = -0.7f; P.torsoYaw = -0.5f; P.torsoPitch = 0.3f; }
        break;
    case A::AUPPER:    if (!k) arm(0.04f, -0.6f, 1.49f, -1.45f, 1.9f);  else arm(-0.9f, -0.38f, 1.17f, -1.41f, 1.27f); break;  // point low, then a rising cut
    case A::AHAYMAKER: if (!k) arm(-1.85f, -0.26f, 0.f, -3.08f, 0.57f); else arm(-0.28f, 0.37f, 1.5f, -2.f, 1.9f);   break;   // over the right shoulder, down across
    case A::ARAY: case A::AZENITH: case A::ASEAMS: case A::AHALO: case A::ASERPENT: case A::ACLAP: case A::ATHUNDER:
    case A::AJUDGE: case A::AMETEOR: case A::AELBOW:
        P.bladeRoll = 0.f; P.bladeCock = 1.3f; break;   // the blade carried along the arm
    default: break;
    }
}

inline void absoluteMove(AbsPose& P, AttackKind a, bool strike, float tp, bool blade = false) {
    absoluteMoveBody(P, a, strike, tp);
    if (blade) absSwordArm(P, a, strike);
}
inline void absoluteMoveBody(AbsPose& P, AttackKind a, bool strike, float tp) {
    const bool k = strike;
    switch (a) {
    case AttackKind::MJAB:        // MORNINGSTAR's jab: the same snap, the size of a man
    case AttackKind::AJAB:        // the lead hand, straight and snapping, a step in behind it
        P.glow = 1;
        if (!k) { P.torsoYaw = 0.12f; P.dip += 0.05f; P.shL = -1.1f; P.elL = 1.8f; P.twL = 1.75f; }
        else { P.hipYaw = -0.7f; P.torsoYaw = -0.18f; P.torsoRoll = -0.08f; P.headPitch = -0.2f; P.shL = -1.55f; P.shLz = -0.08f; P.elL = 0.06f; P.fistL = 1.f;
               P.twL = 0.15f; P.flL = 0.f; P.dvL = 0.f; P.hipL = -0.48f; P.kneeL = 0.42f; }   // turned over: palm down as it lands
        break;
    case AttackKind::ACROSS:      // the rear hand down the middle, the hips turning through it, the lead hand home to his chin
        P.glow = 0;
        if (!k) { P.hipYaw = -0.75f; P.torsoYaw = -0.15f; P.dip += 0.08f; P.shR = -1.0f; P.elR = 2.4f; P.twR = 2.0f; }
        else { P.hipYaw = -0.15f; P.torsoYaw = 0.7f; P.torsoPitch = 0.26f; P.torsoRoll = -0.1f; P.shR = -1.6f; P.shRz = -0.06f; P.elR = 0.05f;
               P.twR = 0.15f; P.flR = 0.f; P.dvR = 0.f;   // turned over: palm down
               P.hipR = 0.38f; P.kneeR = 0.38f; P.shL = -1.1f; P.shLz = -0.3f; P.elL = 2.3f; P.twL = 1.75f; P.fistL = 1.f; }   // the lead hand home at his chin
        break;
    case AttackKind::AHOOK:       // the lead hook: weight loaded onto the lead leg, the elbow comes up level with the shoulder,
                                  // forearm flat, palm down, and the hips and shoulders turn it across; the rear hand stays on his chin
        P.glow = 1;
        if (!k) { P.torsoYaw = 0.45f; P.torsoRoll = 0.12f; P.dip += 0.1f; P.shL = -0.6f; P.shLz = 0.75f; P.elL = 1.75f; P.fistL = 1.f; P.twL = 1.7f; }
        else { P.hipYaw = -0.95f; P.torsoYaw = -0.75f; P.torsoRoll = 0.1f; P.shL = -0.2f; P.shLz = 1.3f; P.elL = 1.55f; P.fistL = 1.f; P.dip += 0.06f;
               P.twL = 1.57f; P.flL = 0.f; P.dvL = 0.f; P.kneeL += 0.12f; }
        break;
    case AttackKind::AUPPER:      // down low on his right, then up through you, his whole body rising behind the fist
        P.glow = 0;
        if (!k) { P.dip += 0.32f; P.torsoRoll = 0.2f; P.torsoPitch = 0.34f; P.torsoYaw = -0.2f; P.shR = 0.35f; P.shRz = 0.05f; P.elR = 1.5f;
                  P.twR = 2.9f; P.flR = 0.2f; P.dvR = 0.f; }   // palm up, low at his hip
        else { P.dip = 0.f; P.torsoPitch = -0.15f; P.torsoYaw = 0.5f; P.torsoRoll = -0.08f; P.headPitch = -0.12f; P.shR = -1.35f; P.shRz = -0.15f; P.elR = 1.7f;
               P.twR = 2.9f; P.flR = 0.15f; P.dvR = 0.f;   // the elbow kept bent, the fist up through the chin, palm toward him
               P.shL = -1.1f; P.elL = 2.3f; P.twL = 1.75f;
               P.hipR = 0.3f; P.kneeR = 0.2f; P.hipL = -0.15f; P.kneeL = 0.2f; }
        break;
    case AttackKind::ADROP:       // up off the floor, the right leg chambered, then driven out
        P.glow = 2; P.plant = 0.f; P.legIK = 0.f; P.ankle = 0.3f; P.crouch = 0.f; P.lift = 2.6f * smooth01(tp); P.torsoPitch = -0.35f; P.hipYaw = 0.f;
        P.hipR = k ? -1.55f : -0.9f; P.kneeR = k ? 0.05f : 2.1f; P.hipL = 0.45f; P.kneeL = 1.7f;
        P.shL = -1.0f; P.shLz = -0.1f; P.elL = 2.2f; P.twL = 1.75f;                     // the lead hand up at his face
        P.shR = k ? 0.8f : 0.4f; P.shRz = 0.55f; P.elR = 0.4f; P.twR = 0.f;              // the rear arm flung back
        P.torsoRoll = k ? -0.15f : 0.f;
        break;
    case AttackKind::ARAY: case AttackKind::AZENITH: case AttackKind::ASEAMS: case AttackKind::AHALO:
        P.glow = 0; P.fistR = 0.f;
        if (a == AttackKind::ARAY) { P.hipYaw = k ? -.1f : -.45f; P.shR = k ? -1.57f : -.9f; P.shRz = -0.05f; P.elR = k ? .08f : 1.65f; P.torsoYaw = k ? .45f : -.3f; P.dip += k ? .08f : .16f; P.shL = -0.3f; P.elL = 0.6f;
                                     P.twR = 0.f; P.flR = -1.3f; P.dvR = 0.f; }   // the palm out at you
        else { P.shR = -3.0f; P.elR = 0.1f; P.shL = -0.2f; P.shLz = 0.5f; P.elL = 0.5f; P.headPitch = -0.35f; P.torsoPitch = -0.1f; }   // one hand to the sky
        break;
    case AttackKind::ASTOMP:      // the right knee up to his chest, then down through the floor
        P.glow = 2; P.legIK = 0.f;
        if (!k) { P.hipR = -1.65f; P.kneeR = 2.05f; P.abdR = 0.12f; P.torsoPitch = -0.1f; P.ankle = 0.4f;   // knee high, both fists raised
                  P.shR = P.shL = -1.4f; P.shRz = P.shLz = 0.5f; P.elR = P.elL = 1.4f; P.twR = P.twL = 1.57f; }
        else { P.hipR = -0.25f; P.kneeR = 0.45f; P.dip += 0.22f; P.torsoPitch = 0.42f; P.headPitch = 0.1f;   // driven down, fists with it
               P.shR = P.shL = 0.25f; P.shRz = P.shLz = 0.55f; P.elR = P.elL = 0.35f; P.twR = P.twL = 1.57f; }
        break;
    case AttackKind::ASERPENT:
        P.glow = 3; P.fistR = P.fistL = 0.2f; P.hipYaw = -0.25f; P.torsoYaw = 0.25f; P.twR = P.twL = 1.57f; P.dvR = P.dvL = 0.f;
        P.shR = P.shL = -1.45f; P.shRz = P.shLz = -0.25f; P.elR = P.elL = 0.25f;
        break;
    case AttackKind::ATELEPORT: case AttackKind::AHOP: case AttackKind::ADASH:
        P.torsoPitch = 0.4f; P.shR = P.shL = 0.7f; P.elR = P.elL = 0.4f; P.dip += 0.18f;
        if (a == AttackKind::ADASH) {   // one long burst of a step, lead foot out, the rear leg driving
            P.legIK = 0.f;
            P.hipYaw = -0.15f; P.hipL = -0.85f; P.kneeL = 0.55f; P.hipR = 0.6f; P.kneeR = 0.35f; P.ankle = 0.6f;
        }
        break;
    case AttackKind::ASWEEP:      // the blade cocked back behind his right shoulder, then flat across
        P.glow = 4; P.shL = -0.8f; P.elL = 1.6f; P.dip += 0.12f;
        if (!k) { P.torsoYaw = -0.85f; }
        else { P.hipYaw = -0.25f; P.torsoYaw = 0.85f; }
        break;   // (the sword arm: absSwordArm)
    case AttackKind::ALUNGE:      // the blade drawn back, then the long step and the point
        P.glow = 4;
        if (!k) { P.dip += 0.22f; P.torsoYaw = -0.2f; }
        else { P.legIK = 0.f; P.hipYaw = 0.2f; P.torsoYaw = 0.45f; P.torsoPitch = 0.35f; P.hipR = -0.9f; P.kneeR = 0.95f; P.hipL = 0.6f; P.kneeL = 0.3f; }
        break;
    case AttackKind::ACRESCENT:   // over his head, then down
        P.glow = 4;
        if (!k) { P.torsoPitch = -0.2f; }
        else { P.torsoPitch = 0.3f; P.dip += 0.12f; }
        break;
    case AttackKind::MBACKHAND:   // MORNINGSTAR: the backfist as a colossus swings it, his whole body behind it
    case AttackKind::ABACKFIST:   // he spins away (his back to you), and the back of the fist comes round with him
        P.glow = a == AttackKind::MBACKHAND ? 5 : 1;
        if (!k) { P.hipYaw = -0.05f; P.torsoYaw = 1.1f; P.shL = -1.3f; P.shLz = -0.1f; P.elL = 1.95f; P.dip += 0.08f; P.twL = 1.75f; P.flL = 0.2f; }
        else { P.hipYaw = -0.95f; P.torsoYaw = -1.0f; P.shL = -1.5f; P.shLz = 1.25f; P.elL = 0.15f; P.twL = -1.4f; P.flL = 0.f; P.dvL = 0.f;   // the back of the fist leading
               P.shR = -1.15f; P.elR = 2.35f; }
        break;
    case AttackKind::AELBOW:      // short: the rear elbow, flat and turning over
        P.glow = 0;
        if (!k) { P.torsoYaw = -0.4f; P.shR = -0.9f; P.shRz = 0.3f; P.elR = 2.5f; P.twR = 2.0f; P.flR = 0.3f; }
        else { P.hipYaw = -0.2f; P.torsoYaw = 0.65f; P.shR = -1.5f; P.shRz = 0.95f; P.elR = 2.7f; P.dip += 0.1f; P.twR = 1.4f; P.flR = 0.4f; }
        break;
    case AttackKind::AROUNDHOUSE: // the knee up, then the hips turn over and the shin comes round at your ribs
        // (the knee comes up and out to the side, the shin folded flat; the hips turn over, he leans away from it,
        // the rear arm swings back to balance him, the lead hand stays on his face)
        P.glow = 2; P.legIK = 0.f; P.ankle = 0.35f; P.shL = -1.2f; P.shLz = -0.25f; P.elL = 2.3f; P.twL = 1.75f;
        if (!k) { P.hipYaw = 0.1f; P.hipR = -1.25f; P.abdR = 0.7f; P.kneeR = 2.1f; P.torsoRoll = -0.22f; P.torsoPitch = -0.05f; P.kneeL = 0.5f;
                  P.shR = 0.2f; P.shRz = 0.35f; P.elR = 0.9f; P.twR = 0.f; }
        else { P.hipYaw = 0.9f; P.torsoYaw = -0.3f; P.hipR = -1.4f; P.abdR = 1.35f; P.kneeR = 0.12f; P.torsoRoll = -0.45f; P.torsoPitch = -0.12f;
               P.kneeL = 0.35f; P.shR = 0.75f; P.shRz = 0.5f; P.elR = 0.3f; P.twR = 0.f; P.flR = 0.f; P.dvR = 0.f; }
        break;
    case AttackKind::AHAYMAKER:   // wound right back, then a looping right hook over the top with everything behind it: the elbow
                                  // up level with his shoulder, forearm flat, palm down, the lead hand home on his chin
        P.glow = 0;
        if (!k) { P.hipYaw = -0.8f; P.torsoYaw = -0.95f; P.torsoPitch = 0.1f; P.shR = -0.4f; P.shRz = 0.95f; P.elR = 1.9f; P.dip += 0.12f; P.twR = 1.9f; }
        else { P.hipYaw = -0.05f; P.torsoYaw = 0.9f; P.torsoPitch = 0.35f; P.torsoRoll = -0.12f; P.shR = -0.35f; P.shRz = 1.25f; P.elR = 1.5f; P.dip += 0.2f;
               P.twR = 1.57f; P.flR = 0.f; P.dvR = 0.f; P.shL = -1.1f; P.elL = 2.3f; P.twL = 1.75f; P.hipR = 0.3f; P.kneeR = 0.4f; }
        break;
    case AttackKind::ACHARGE:     // head down, lead shoulder first
        P.glow = 5; P.torsoPitch = 0.75f; P.hipYaw = -0.6f; P.torsoYaw = -0.4f; P.shL = -0.3f; P.shLz = -0.2f; P.elL = 2.3f;
        P.shR = 0.4f; P.elR = 1.2f; P.dip += 0.25f; P.twL = 1.75f; P.twR = 2.0f;
        break;
    case AttackKind::AAXE:        // the heel up past his head, then straight down
        // (the leg swung up and out past his head, then straight down the middle; the guard stays up)
        P.glow = 2; P.legIK = 0.f; P.ankle = 0.3f; P.shL = -1.1f; P.elL = 2.2f; P.shR = -0.9f; P.shRz = 0.1f; P.elR = 2.3f;
        if (!k) { P.hipR = -2.5f; P.abdR = 0.45f; P.kneeR = 0.15f; P.torsoPitch = -0.3f; P.torsoRoll = -0.15f; }
        else { P.hipR = -0.35f; P.abdR = 0.05f; P.kneeR = 0.12f; P.torsoPitch = 0.35f; P.dip += 0.15f; P.shR = -0.5f; P.elR = 1.6f; }
        break;
    case AttackKind::ACLAP:       // both arms wide, chest open, then palms together in front
    case AttackKind::ATHUNDER:    // (risen: the same, over your head)
        P.glow = 3; P.fistR = P.fistL = 0.f; P.shR = P.shL = -1.5f; P.elR = P.elL = 0.15f; P.hipYaw = -0.15f; P.torsoYaw = 0.15f;
        P.twR = P.twL = 1.57f; P.dvR = P.dvL = 0.f; P.flR = P.flL = -0.2f;   // palms facing
        if (!k) { P.shRz = P.shLz = 1.3f; P.torsoPitch = -0.15f; }
        else { P.shRz = P.shLz = -0.2f; P.torsoPitch = 0.2f; }
        if (a == AttackKind::ATHUNDER) P.plant = 0.f;
        break;
    case AttackKind::MGRASP:      // MORNINGSTAR: the right hand up and open, then down to the floor where you stand
        P.glow = 0; P.fistR = 0.f;
        if (!k) { P.shR = -2.6f; P.shRz = 0.15f; P.elR = 0.35f; P.torsoPitch = -0.2f; P.torsoYaw = 0.3f; P.twR = 0.f; P.flR = -0.3f; }
        else { P.shR = -0.95f; P.elR = 0.1f; P.torsoPitch = 0.75f; P.torsoYaw = 0.4f; P.dip += 0.35f; P.hipR = -0.6f; P.kneeR = 0.7f;
               P.twR = 0.f; P.flR = 0.3f; P.dvR = 0.f; }   // the hand palm down over you, fingers curling to close
        break;
    case AttackKind::MSUNDER:     // MORNINGSTAR: both fists over his head, then down into the floor like a hammer
        P.glow = 3; P.fistR = P.fistL = 1.f; P.twR = P.twL = 1.5f; P.dvR = P.dvL = 0.f; P.flR = P.flL = 0.3f;   // hammer fists
        if (!k) { P.shR = P.shL = -2.9f; P.shRz = P.shLz = 0.05f; P.elR = P.elL = 0.3f; P.torsoPitch = -0.25f; }
        else { P.shR = P.shL = -0.6f; P.shRz = P.shLz = -0.1f; P.elR = P.elL = 0.2f; P.torsoPitch = 0.8f; P.dip += 0.4f; P.headPitch = 0.2f; }
        break;
    case AttackKind::MFISSURE:    // MORNINGSTAR: the right fist raised, a long step, and down into the floor at you
        P.glow = 0;
        P.twR = 1.5f; P.flR = 0.3f; P.dvR = 0.f;
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
        P.fistR = P.fistL = 0.f; P.twR = P.twL = 0.f; P.dvR = P.dvL = 0.f; P.flR = P.flL = 0.f; P.headPitch = -0.3f; P.hipR = P.hipL = 0.15f; P.kneeR = P.kneeL = 0.3f; P.ankle = 0.3f;
        break;
    default: break;
    }
}

// His pelvis in the rig's root frame: sunk into his legs, risen off the floor, shifted over his feet, turned
inline mat4 absBodyLocal(const Enemy& e, const AbsPose& P) {
    return T({P.swayX, P.lift - P.crouch + absBob(e), P.swayZ}) * RY(P.hipYaw);
}
constexpr float AB_THIGH = 0.58f, AB_SHIN = 0.55f, AB_SOLE = 0.03f;   // (an ankle's height with the foot flat on the floor)
// An ankle, in the root frame, by the pose's own leg angles
inline vec3 absAnkleLocal(const Enemy& e, const AbsPose& P, bool right) {
    const float s = right ? -1.f : 1.f;
    const mat4 kn = absBodyLocal(e, P) * T({s * 0.15f, 1.15f, 0.f}) * RX(right ? P.hipR : P.hipL) * RZ(s * (P.stanceW + (right ? P.abdR : P.abdL)))
                  * T({0.f, -AB_THIGH, 0.f}) * RX(right ? P.kneeR : P.kneeL);
    return vec3(kn * glm::vec4(0.f, -AB_SHIN, 0.f, 1.f));
}
// Turn one leg's joints so its ankle is at `target` (root frame), by `weight`. Out of reach: the leg straight at it.
inline void absLegReach(const Enemy& e, AbsPose& P, bool right, vec3 target, float weight) {
    const float s = right ? -1.f : 1.f;
    vec3 t = vec3(glm::inverse(absBodyLocal(e, P)) * glm::vec4(target, 1.f)) - vec3{s * 0.15f, 1.15f, 0.f};
    const float len = glm::length(t), d = glm::clamp(len, 0.3f, AB_THIGH + AB_SHIN - 0.004f);
    if (len < 1e-4f) return;
    t *= d / len;
    const float knee = 3.14159265f - std::acos(glm::clamp((AB_THIGH * AB_THIGH + AB_SHIN * AB_SHIN - d * d) / (2.f * AB_THIGH * AB_SHIN), -1.f, 1.f));
    const float vy = -AB_THIGH - AB_SHIN * std::cos(knee), vz = -AB_SHIN * std::sin(knee);   // the ankle under a hip that hasn't turned
    const float a = std::asin(glm::clamp(-t.x / std::min(vy, -1e-3f), -0.95f, 0.95f));        // out to the side (the hip's RZ)
    const float hip = std::atan2(t.z, t.y) - std::atan2(vz, vy * std::cos(a));               // fore and aft (its RX)
    float& H = right ? P.hipR : P.hipL; float& K = right ? P.kneeR : P.kneeL; float& A = right ? P.abdR : P.abdL;
    H += std::remainder(hip - H, 6.2831853f) * weight;
    K += (knee - K) * weight;
    A += (s * a - P.stanceW - A) * weight;
}

// (where his feet are: planted, the hips sit on the legs; otherwise crouch is as given)
inline void absPlant(AbsPose& P) {
    if (P.plant <= 0.f) return;
    absSink(P, P.dip); P.dip = 0.f;
    P.crouch += (absLegDrop(P) - P.crouch) * glm::clamp(P.plant, 0.f, 1.f);
}

constexpr float AB_FOLLOW_T = 0.42f;   // a blow's follow-through, back to guard

// Contact pose shared by the entrance and aerial hammer attacks: a planted, asymmetric landing.
inline AbsPose absoluteLanding(const AbsPose& guard) {
    AbsPose p = guard;
    p.plant = 1.f; p.legIK = 0.f; p.lift = 0.f; p.ankle = 1.f; p.dip = .2f;
    p.hipR = -1.15f; p.kneeR = 1.65f; p.hipL = .2f; p.kneeL = 1.25f;
    p.hipYaw = -.2f; p.torsoYaw = .25f; p.torsoPitch = .7f; p.headPitch = -.3f;
    p.shR = -.45f; p.elR = .12f; p.shRz = -.15f; p.fistR = 1.f;
    p.shL = .45f; p.shLz = .5f; p.elL = .65f; p.fistL = .35f;
    p.omega = 48.f;
    return p;
}

inline AbsPose absolutePose(const Enemy& e, float time) {
    AbsPose G = absoluteGuard(e, time);
    const AbsPose stance = G;   // (his legs as the guard stands them: what a planted move sinks from)
    float drive = 0.f;          // his weight through a blow: back onto the rear foot as he winds up (-), through the lead foot as it lands (+)
    if (e.abArrive > 0.f) {
        const float settle = Enemy::AB_ARRIVE_SETTLE;
        AbsPose land = absoluteLanding(G), p;
        if (e.abArrive > settle) {
            // Open silhouette, then tuck the legs and lead the fall with the striking arm.
            AbsPose flight = G; flight.plant = 0.f; flight.dip = 0.f; flight.ankle = .2f;
            flight.shR = -2.65f; flight.elR = .45f; flight.shL = .65f; flight.elL = .4f;
            flight.hipR = -.6f; flight.kneeR = 1.2f; flight.hipL = .45f; flight.kneeL = .8f;
            flight.torsoPitch = .12f; flight.torsoYaw = -.35f;
            const float brace = smooth01(1.f - (e.abArrive - settle) / .28f);
            p = absMix(flight, land, brace, brace);
            p.plant = brace; p.omega = 48.f;
        } else {
            const float recovery = smooth01((settle - e.abArrive - .065f) / (settle - .065f));
            p = absMix(land, G, recovery, smooth01(recovery * .8f));
            p.plant = 1.f; p.omega = 38.f;
        }
        absPlant(p); return p;
    }
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
        AbsPose C = G; absoluteMove(C, atk, false, tp, e.abBlade);
        AbsPose K = G; absoluteMove(K, atk, true, tp, e.abBlade);
        // winding up: the core turns first, the limb draws after it; held there (a tremor) if the tell runs long
        P = absMix(G, C, absEaseOut(w * 1.15f), smooth01(w));
        const float tremor = 0.02f * std::sin(time * 37.f) * w * (1.f - s);
        P.torsoYaw += tremor; P.shR += tremor; P.shL -= tremor;
        // not a statue while he winds up: the knees keep their bounce, the head keeps moving, and the
        // coil keeps tightening (the chamber's turn deepens) until the blow goes
        const float live = 1.f - s;
        P.dip += 0.03f * (0.5f + 0.5f * std::sin(time * 5.5f)) * live;
        P.torsoRoll += 0.035f * std::sin(time * 2.3f) * live;
        P.headPitch += 0.03f * std::sin(time * 3.1f + 1.f) * live;
        P.torsoYaw += (C.torsoYaw - G.torsoYaw) * 0.18f * w * live;
        P.hipYaw += (C.hipYaw - G.hipYaw) * 0.12f * w * live;
        P.omega = 18.f;
        drive = -0.55f * absEaseOut(w * 1.15f) * (1.f - s);
        if (s > 0.f) {   // the stroke: the hips snap round, the limb whips out after them, past the mark and back
            P = absMix(P, K, absBackOut(s * 1.4f), absBackOut((s - 0.15f) / 0.85f));
            P.omega = 42.f;
            drive += absBackOut(s * 1.4f);
        }
        P.glow = C.glow;
    } else if (e.abBodyLanded != AttackKind::NONE && e.abBodySince < AB_FOLLOW_T && !e.staggered() && !(e.abAirborne() && !e.abDiving)) {
        // the follow-through: held at full stretch a beat, then gathered back into his guard
        AbsPose K = G; absoluteMove(K, e.abBodyLanded, true, 1.f, e.abBlade);
        const bool groundImpact = e.abAlt < .3f && (e.abBodyLanded == AttackKind::ASKYFALL ||
            e.abBodyLanded == AttackKind::MDROP || e.abBodyLanded == AttackKind::AMETEOR);
        if (groundImpact) K = absoluteLanding(G);
        if (K.plant > 0.f || G.plant <= 0.f) {
            const float r = smooth01((e.abBodySince - 0.06f) / (AB_FOLLOW_T - 0.06f));
            P = absMix(K, G, r, smooth01(r * 1.2f));
            P.glow = -1; P.omega = 20.f;
            drive = 1.f - r;
        }
    }
    if (e.staggered() || e.abSwallow > Enemy::MS_RISE) {   // (the swallow: on his knees, as if broken)
        P = AbsPose{}; P.omega = 20.f;
        if (e.abReeling && e.abSwallow <= 0.f) {   // thrown back on his heels, the guard knocked open
            P.plant = 1.f; P.ankle = 1.f; P.stanceW = 0.12f; P.hipR = 0.3f; P.kneeR = 0.35f; P.hipL = -0.2f; P.kneeL = 0.3f;
            P.torsoPitch = -0.4f; P.torsoRoll = 0.12f; P.shR = P.shL = 0.25f; P.shRz = P.shLz = 0.55f; P.elR = P.elL = 0.4f; P.headPitch = -0.3f;
            P.fistR = P.fistL = 0.5f;
            // driven back by the shove: stumbling steps (abGait runs backwards), not a slide
            const float st = glm::clamp(std::fabs(e.abGait) / Enemy::AB_RUN * 2.5f, 0.f, 1.f), sn = std::sin(e.animPhase), cs = std::cos(e.animPhase);
            P.hipR += 0.35f * sn * st; P.hipL -= 0.35f * sn * st;
            P.kneeR += std::max(0.f, cs) * 0.7f * st; P.kneeL += std::max(0.f, -cs) * 0.7f * st;
            P.torsoPitch -= 0.15f * st;
        } else {             // broken: down on one knee, head bowed
            P.omega = 12.f; P.legIK = 0.f;
            P.crouch = 0.62f; P.hipR = -1.45f; P.kneeR = 1.5f; P.hipL = 0.55f; P.kneeL = 2.3f;
            P.torsoPitch = 0.38f; P.headPitch = 0.4f; P.shR = P.shL = 0.15f; P.shRz = P.shLz = 0.2f; P.elR = P.elL = 0.3f;
        }
    }
    if (P.plant > 0.5f && P.legIK > 0.5f) {
        // Feet planted and the legs reaching for them: the pose's own leg angles only set how low he sits, so
        // they are the guard's, sunk by the move's dip. A blow doesn't stand him up out of his stance; it
        // drives his hips down and through it
        P.hipR = stance.hipR; P.kneeR = stance.kneeR; P.hipL = stance.hipL; P.kneeL = stance.kneeL; P.abdR = P.abdL = 0.f; P.stanceW = stance.stanceW;
        P.dip = std::max(P.dip, stance.dip) + 0.07f * std::max(0.f, drive);
        P.swayZ += 0.17f * drive;
    }
    if (P.plant > 0.f) {
        // His weight goes where his body does: forward with a lean, over the side he tips to, and onto
        // the standing foot while the other steps. The legs take it up (his feet stay where they are)
        const float sR = std::sin(3.14159265f * glm::clamp(e.abFootS[0], 0.f, 1.f)), sL = std::sin(3.14159265f * glm::clamp(e.abFootS[1], 0.f, 1.f));
        P.swayZ += glm::clamp(0.26f * (std::sin(P.torsoPitch) - std::sin(0.38f)), -0.2f, 0.3f) * P.plant;
        P.swayX += (-0.16f * P.torsoRoll + 0.05f * (sR - sL)) * P.plant;
    }
    absPlant(P);
    return P;
}

inline AbsPose absoluteSprungPose(const Enemy& e, float time);
inline const vec3* absFeetHome();
inline mat4 absRootOf(const Enemy& e);
// His feet. A planted foot stays where it is in the world; when the body has moved far enough off it
// (or turned, or the leg can't reach) it steps: lifts, swings to where it will be wanted (his stance's
// home for that foot, led by his speed), and plants again. One foot at a time unless he is moving
// faster than a leg can cover, when both may be off the floor (a run's flight). A stride is as long as
// the ground he covers, at any speed and either size. While a move poses his legs itself (a kick, a
// leap, the kneel) the feet just follow the legs, so they are where the legs left them when it ends.
inline void absFeetTick(Enemy& e, float dt, const AbsPose& goal) {
    e.abStepped = 0;
    for (int k = 0; k < 2; ++k) { e.abFootPrev[k] = e.abFoot[k]; e.abFootUpPrev[k] = e.abFootUp[k]; }
    e.abFootIKPrev = e.abFootIK;
    const float sc = std::max(e.scale, 1e-3f);
    const mat4 root = absRootOf(e);
    const glm::vec2 moved{e.position.x - e.abFeetAt.x, e.position.z - e.abFeetAt.z};
    const bool jumped = glm::length(moved) > 0.9f * sc || std::fabs(e.position.y - e.abFeetAt.y) > 0.6f * sc;   // (a blink, a dash's burst)
    e.abFeetAt = e.position;
    if (jumped || dt <= 0.f) e.abFeetVel = glm::vec2{0.f};
    else e.abFeetVel += (moved / dt - e.abFeetVel) * (1.f - std::exp(-dt * 14.f));
    // Placed feet are for standing, shuffling and walking. At a run (over two body lengths a second) a leg
    // can't cover the ground one step at a time: the run cycle takes the legs back (its strides are matched
    // to the floor by abGaitTick) and the feet follow them
    const float pace = glm::length(e.abFeetVel) / sc;
    const bool grounded = goal.plant > 0.5f && goal.legIK > 0.5f && e.abAlt < 0.05f && e.abArrive <= 0.f && !e.abDiving;
    const float wantIK = grounded ? 1.f - glm::smoothstep(2.0f, 3.4f, pace) : 0.f;
    e.abFootIK += (wantIK - e.abFootIK) * (1.f - std::exp(-dt * (wantIK > e.abFootIK ? 9.f : 14.f)));
    if (jumped && e.abFeetLive) {   // a blink: he is simply standing there, feet under him as his stance has them
        const vec3* h = absFeetHome();
        for (int k = 0; k < 2; ++k) {
            e.abFoot[k] = vec3(root * glm::vec4(h[k], 1.f)); e.abFootPrev[k] = e.abFootFrom[k] = e.abFoot[k];
            e.abFootS[k] = 1.f; e.abFootUp[k] = e.abFootUpPrev[k] = 0.f;
        }
        return;
    }
    if (!e.abFeetLive || e.abFootIK < 0.02f) {
        const AbsPose P = absoluteSprungPose(e, e.age);
        for (int k = 0; k < 2; ++k) {
            const vec3 a = absAnkleLocal(e, P, k == 0);
            e.abFoot[k] = vec3(root * glm::vec4(a, 1.f));
            e.abFootUp[k] = std::max(0.f, a.y - AB_SOLE); e.abFootS[k] = 1.f;
            if (!e.abFeetLive) { e.abFootPrev[k] = e.abFoot[k]; e.abFootUpPrev[k] = e.abFootUp[k]; }
        }
        e.abFeetLive = true;
        return;
    }
    const glm::vec2 vel = e.abFeetVel;
    const float speed = pace;                                                    // (in his own body lengths)
    const float swingT = glm::clamp(0.30f - 0.016f * speed, 0.13f, 0.30f) * std::sqrt(sc);   // a long slow step for the colossus
    const vec3* home = absFeetHome();
    const AbsPose P = absoluteSprungPose(e, e.age);
    glm::vec2 want2[2]; float err[2];
    for (int k = 0; k < 2; ++k) {
        const vec3 h = vec3(root * glm::vec4(home[k], 1.f));
        glm::vec2 lead = vel * (swingT * 0.85f);                                 // where this foot should come down: ahead of its home by his speed
        const float ll = glm::length(lead);
        if (ll > 0.55f * sc) lead *= 0.55f * sc / ll;                            // (never further than a leg reaches)
        want2[k] = glm::vec2{h.x, h.z} + lead;
        err[k] = glm::length(glm::vec2{e.abFoot[k].x, e.abFoot[k].z} - glm::vec2{h.x, h.z});
    }
    const float limit = std::max(0.13f, speed * swingT * 0.8f) * sc;             // how far a planted foot may trail its home
    // A burst across the floor (a step-in, a dash's start: faster than five body lengths a second this
    // tick): no foot can stay planted through it. Both leave the floor and travel with him, to land as it ends
    if (dt > 0.f && glm::length(moved) / dt / sc > 5.f) {
        const float pull = 1.f - std::exp(-dt * 40.f);
        for (int k = 0; k < 2; ++k) {
            const vec3 h = vec3(root * glm::vec4(home[k], 1.f));
            e.abFoot[k].x += (h.x - e.abFoot[k].x) * pull; e.abFoot[k].z += (h.z - e.abFoot[k].z) * pull;
            e.abFootUp[k] += (0.1f - e.abFootUp[k]) * pull;
            e.abFootS[k] = 0.6f; e.abFootFrom[k] = e.abFoot[k]; e.abFootT[k] = 0.16f * std::sqrt(sc);
        }
        return;
    }
    for (int k = 0; k < 2; ++k) {
        const int o = 1 - k;
        if (e.abFootS[k] >= 1.f) {
            e.abFootUp[k] *= std::exp(-dt * 20.f);
            const vec3 hip = vec3(root * absBodyLocal(e, P) * glm::vec4(k == 0 ? -0.15f : 0.15f, 1.15f, 0.f, 1.f));
            const float off = glm::length(glm::vec2{e.abFoot[k].x - hip.x, e.abFoot[k].z - hip.z});
            const bool torn = off > 0.92f * sc;                                  // the leg can't reach it any more
            const bool turn = e.abFootS[o] >= 1.f && err[k] > limit && err[k] >= err[o];
            if (torn || turn) { e.abFootS[k] = 0.f; e.abFootFrom[k] = e.abFoot[k]; e.abFootT[k] = swingT; }
        }
        if (e.abFootS[k] < 1.f) {
            const float u0 = e.abFootS[k], e0 = u0 * u0 * (3.f - 2.f * u0);
            e.abFootS[k] = std::min(1.f, e.abFootS[k] + dt / std::max(e.abFootT[k], 0.05f));
            const float u = e.abFootS[k], ease = u * u * (3.f - 2.f * u);
            // (closing on where it is wanted now: a share of what is left each tick, so a target that moves under it never jerks it)
            const float share = e0 < 0.999f ? glm::clamp((ease - e0) / (1.f - e0), 0.f, 1.f) : 1.f;
            e.abFoot[k].x += (want2[k].x - e.abFoot[k].x) * share; e.abFoot[k].z += (want2[k].y - e.abFoot[k].z) * share;
            const float stepLen = glm::length(want2[k] - glm::vec2{e.abFootFrom[k].x, e.abFootFrom[k].z}) / sc;
            e.abFootUp[k] = std::max(e.abFootUp[k] * std::exp(-dt * 14.f), glm::clamp(0.05f + 0.16f * stepLen, 0.05f, 0.34f) * std::sin(3.14159265f * u));
            if (u >= 1.f) { e.abFootUp[k] = 0.f; e.abStepped |= 1 << k; }
        }
    }
}

// One step of a damped spring toward `goal`, solved exactly (the same motion at any tick length).
// zeta 1: critically damped (arrives and stops); under 1: runs past and settles.
inline void absSpring(float& x, float& v, float goal, float omega, float zeta, float dt) {
    const float x0 = x - goal, v0 = v;
    if (zeta >= 0.999f) {
        const float ex = std::exp(-omega * dt), c = v0 + omega * x0;
        x = goal + (x0 + c * dt) * ex;
        v = (v0 - omega * c * dt) * ex;
        return;
    }
    const float wd = omega * std::sqrt(1.f - zeta * zeta), ex = std::exp(-zeta * omega * dt);
    const float A = x0, B = (v0 + zeta * omega * x0) / wd, cs = std::cos(wd * dt), sn = std::sin(wd * dt);
    x = goal + ex * (A * cs + B * sn);
    v = ex * ((B * wd - zeta * omega * A) * cs - (A * wd + zeta * omega * B) * sn);
}
// A hit goes into his body: his trunk rocks away from where it came from, his head snaps after it, his
// weight goes back off the blow, and the springs bring him home. dir: the way the hit travels (world);
// strength 0..1 by its damage. MORNINGSTAR takes the same hit like a wall takes it.
inline void absoluteHit(Enemy& e, glm::vec3 dir, float strength) {
    if (!e.abBodyLive || e.telegraphTimer > 0.f || e.abAirborne()) return;   // (never across a tell: the blow you must read keeps its shape)
    dir.y = 0.f;
    const float len = glm::length(dir);
    if (len < 1e-4f) return;
    dir /= len;
    const float k = glm::clamp(strength, 0.f, 1.f) * (e.morning() ? 0.4f : 1.f);
    const float fwd = dir.x * std::sin(e.yaw) + dir.z * std::cos(e.yaw);      // + : the hit runs the way he faces (from behind)
    const float left = dir.x * std::cos(e.yaw) - dir.z * std::sin(e.yaw);     // + : it runs toward his left
    e.abBodyV[3] += 7.f * k * fwd;          // torsoPitch: struck from the front he rocks back
    e.abBodyV[4] += 11.f * k * fwd;         // headPitch
    e.abBodyV[18] -= 6.f * k * left;        // torsoRoll: away from the side it came from
    e.abBodyV[2] += 5.f * k * left;         // torsoYaw: the struck shoulder goes with it
    e.abBodyV[33] += 2.2f * k * left;       // swayX
    e.abBodyV[34] += 2.6f * k * fwd;        // swayZ
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
    // Each joint is a spring of its own kind. His core and legs are critically damped: they go where they
    // are sent and stop. His arms and head are a little loose, so they trail the body, run past and settle
    // (overlapping action). During a blow's stroke (omega over 30) everything is stiff: the fist is on its
    // mark as the blow lands. MORNINGSTAR is the same body with far more mass: every spring slower when he
    // isn't striking, and looser, so his trunk carries on past a stop and takes a moment to come to rest.
    const bool striking = goal.omega > 30.f, ms = e.morning();
    for (int k = 0; k < AB_NCH; ++k) {
        e.abBodyPrev[k] = e.abBody[k];
        const bool head = k == 4, core = absCore(k) && !head, limb = (k >= 5 && k <= 10) || k == 15 || k == 16 || (k >= 23 && k <= 30);
        float w = goal.omega, z = 1.f;
        if (!striking) {
            if (limb) { z = ms ? 0.6f : 0.78f; w *= ms ? 0.7f : 0.92f; }
            else if (head) { z = ms ? 0.62f : 0.72f; w *= ms ? 0.6f : 0.8f; }
            else if (core) { z = ms ? 0.78f : 1.f; w *= ms ? 0.68f : 1.f; }
            else w *= ms ? 0.8f : 1.f;
        } else if (ms) w *= 0.92f;
        absSpring(e.abBody[k], e.abBodyV[k], goal.*AB_CH[k], w, z, dt);
    }
    absFeetTick(e, dt, goal);
}

// The pose as drawn: his sprung body (what tickAbsoluteBody has), the tell's glow from where he's going
inline AbsPose absoluteSprungPose(const Enemy& e, float time) {
    AbsPose P = absolutePose(e, time);
    if (!e.abBodyLive) return P;
    for (int k = 0; k < AB_NCH; ++k) P.*AB_CH[k] = e.abBody[k];
    if (P.plant > 0.f) P.crouch += (absLegDrop(P) - P.crouch) * glm::clamp(P.plant, 0.f, 1.f);   // (the feet stay on the floor)
    return P;
}
// His feet's home under him, in the root frame: where the guard stands them (lead foot forward, bladed)
inline const vec3* absFeetHome() {
    static vec3 home[2]; static bool made = false;
    if (!made) {
        Enemy t(EnemyType::ABSOLUTE, vec3{0.f});
        AbsPose G = absoluteGuard(t, 0.f); G.torsoRoll = 0.f; absPlant(G);
        home[0] = absAnkleLocal(t, G, true); home[1] = absAnkleLocal(t, G, false);
        home[0].y = home[1].y = AB_SOLE; made = true;
    }
    return home;
}
inline mat4 absRootOf(const Enemy& e) { return T(e.position) * RY(e.yaw) * S(vec3{e.scale}); }
// The legs reaching for his planted feet. Sinks his hips first if a foot is further off than a leg is long.
inline void absFeetApply(const Enemy& e, AbsPose& P) {
    // (only with his feet on the floor in the pose he is actually in: a leap's rise is not a reach for the ground)
    const float w = glm::clamp(e.abFootIK, 0.f, 1.f) * glm::clamp(P.plant, 0.f, 1.f) * (1.f - glm::clamp(P.lift * 5.f, 0.f, 1.f));
    if (!e.abFeetLive || w < 0.01f) return;
    const mat4 inv = glm::inverse(absRootOf(e));
    vec3 target[2];
    for (int k = 0; k < 2; ++k) {
        target[k] = vec3(inv * glm::vec4(e.abFoot[k].x, e.position.y, e.abFoot[k].z, 1.f));
        target[k].y = AB_SOLE + e.abFootUp[k];
    }
    const float reach = AB_THIGH + AB_SHIN - 0.03f;
    for (int it = 0; it < 3; ++it) {
        float over = 0.f;
        for (int k = 0; k < 2; ++k) {
            const vec3 hip = vec3(absBodyLocal(e, P) * glm::vec4(k == 0 ? -0.15f : 0.15f, 1.15f, 0.f, 1.f));
            const vec3 d = target[k] - hip;
            const float flat = glm::length(glm::vec2{d.x, d.z});
            if (flat < reach && glm::length(d) > reach) over = std::max(over, -d.y - std::sqrt(reach * reach - flat * flat));
        }
        if (over <= 1e-4f) break;
        P.crouch += over * w;
    }
    for (int k = 0; k < 2; ++k) absLegReach(e, P, k == 0, target[k], w);
    P.ankle += (1.f - P.ankle) * w;
}
inline AbsPose absoluteLivePose(const Enemy& e, float time) {
    AbsPose P = absoluteSprungPose(e, time);
    absFeetApply(e, P);
    return P;
}

// How far a fist curls (fist 1): the fingers' first joint, their second (tucked right back into the
// palm), the thumb across them, its tip. The palm is the hand's -Z (the knuckles, +Z, its back)
constexpr float AB_CURL[4] = {1.45f, 1.6f, 1.0f, 0.9f};

// A wrist's turn (in the hand's frame, about the wrist): s +1 his left hand, -1 his right
inline mat4 absWrist(const AbsPose& P, float s) {
    const bool right = s < 0.f;
    return RY(s * (right ? P.twR : P.twL)) * RZ(-s * (right ? P.dvR : P.dvL)) * RX(right ? P.flR : P.flL);
}

// His torso's frame (the rig and headBox share it, so the head you see is the head you hit)
inline mat4 absoluteTorsoFrame(const Enemy& e, const mat4& root, const AbsPose& P) {
    return root * absBodyLocal(e, P) * T({0.f, 1.30f, 0.f})
         * RY(P.torsoYaw) * RZ(P.torsoRoll) * RX(P.torsoPitch);
}
inline mat4 absoluteHeadFrame(const Enemy& e, const mat4& root, const AbsPose& P) {
    return absoluteTorsoFrame(e, root, P) * T({0.f, 0.95f, 0.f}) * RY(-0.4f * (P.torsoYaw + P.hipYaw)) * RX(P.headPitch);   // (his eyes stay on you)
}

inline void absoluteWhiteHot(const Enemy& e, Rig& r, size_t first);
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
    const vec3 tc = tell == AbTell::GOLD ? abGoldTell(vec3{2.2f, 1.72f, 0.7f}) : tell == AbTell::RED ? vec3{2.4f, 0.25f, 0.15f} : vec3{1.9f, 1.9f, 2.0f};
    const float tk = 0.5f + 3.f * tp;   // (always the brightest thing on him by the end)
    auto lit = [&](int limb, vec3 base) {
        if (tell == AbTell::NONE || P.glow < 0) return base;
        bool on = P.glow == limb || P.glow == 5 || (P.glow == 3 && (limb == 0 || limb == 1));
        return on ? tc * tk : base;
    };
    const mat4 body = root * absBodyLocal(e, P);

    // ---- legs: thigh, its plate and two veins, knee cap, shin, calf, its seam, foot, two toes, heel, an ankle wrap ----
    for (float s : {-1.f, 1.f}) {
        const bool right = s < 0.f;
        const int limb = right ? 2 : 6;
        mat4 hip = body * T({s * 0.15f, 1.15f, 0.f}) * RX(right ? P.hipR : P.hipL) * RZ(s * (P.stanceW + (right ? P.abdR : P.abdL)));
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
        mat4 hand = elbow * T({0.f, -0.44f, 0.f}) * absWrist(P, s);
        if (right && e.abBlade) {   // the sword hand: the fist turns on the wrist about the grip (AbsPose::bladeRoll / bladeCock)
            const vec3 grip{0.f, -0.07f, -0.035f};
            hand = hand * T(grip) * RY(P.bladeRoll) * RZ(-P.bladeCock) * T(-grip);
        }
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
            mat4 f1 = hand * T({-0.035f + 0.023f * f, -0.1f, 0.01f}) * RX(fist * AB_CURL[0]);
            r.box(f1, {0.f, -0.03f, 0.f}, {0.02f, 0.06f, 0.022f}, skin, lit(limb, soft));
            mat4 f2 = f1 * T({0.f, -0.06f, 0.f}) * RX(fist * AB_CURL[1]);
            r.box(f2, {0.f, -0.025f, 0.f}, {0.018f, 0.05f, 0.02f}, plate, lit(limb, soft * 1.5f));
        }
        mat4 t1 = hand * T({-s * 0.055f, -0.04f, 0.03f}) * RZ(-s * 0.5f) * RX(fist * AB_CURL[2]);
        r.box(t1, {0.f, -0.025f, 0.f}, {0.022f, 0.05f, 0.022f}, skin, lit(limb, soft));
        r.box(t1 * T({0.f, -0.05f, 0.f}) * RX(fist * AB_CURL[3]), {0.f, -0.02f, 0.f}, {0.02f, 0.04f, 0.02f}, plate, lit(limb, soft));
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
        const mat4 sw = handR * T({0.f, -0.07f, -0.035f}) * RZ(1.5707963f);   // (through the fist, out of the thumb side)
        const vec3 bl = tell != AbTell::NONE && P.glow == 4 ? tc * tk : vec3{1.9f, 1.75f, 2.1f};
        r.box(sw, {0.f, 0.f, 0.f}, {0.05f, 0.18f, 0.05f}, dark, light * 0.5f);
        r.box(sw, {0.f, -0.1f, 0.f}, {0.32f, 0.04f, 0.07f}, plate, gold);
        for (int k = 0; k < 3; ++k)
            r.box(sw, {0.f, -0.35f - 0.45f * k, 0.f}, {0.09f - 0.02f * k, 0.46f, 0.025f}, plate, bl * (0.9f + 0.1f * k));
        r.box(sw, {0.f, -0.75f, 0.015f}, {0.02f, 1.3f, 0.01f}, light, bl * 1.4f);
    }
    absoluteWhiteHot(e, r, first);
}

// MORNINGSTAR's white tell: his whole body (the boxes from `first` on) goes white-hot as the blow
// comes (the one thing to parry)
inline void absoluteWhiteHot(const Enemy& e, Rig& r, size_t first) {
    if (!e.morning() || e.telegraphTimer <= 0.f || abTellOf(e.attack) != AbTell::WHITE) return;
    const float k = smooth01(e.telegraphProgress() * 1.2f);
    for (size_t j = first; j < r.out.size(); ++j) {
        r.out[j].color = glm::mix(r.out[j].color, vec3{1.f}, 0.7f * k);
        r.out[j].emissive = glm::mix(r.out[j].emissive, vec3{2.2f, 2.15f, 2.1f}, 0.25f + 0.65f * k);
    }
}

// His body from a file (assets/enemies/absolute.glb; MORNINGSTAR's, morningstar.glb): the joints turn
// by the same sprung pose buildAbsolute draws, and each glow channel shines as its parts do there.
// A limb's seams (tell*), its glow (aura*) and its gauntlet's runes (rune*) take the tell's colour
// when it's the striking limb. Every fixed colour dims with MORNINGSTAR's light in TOTALITY (tint).
inline bool& absPlainTells() { static bool on = false; return on; }   // (the viewer's "plain": no tell glow, to see the limb's shape)
inline bool absoluteFilePose(const Enemy& e, float time, const mat4& root, mat4& fileRoot, FilePose& F, vec3& glowOut) {
    using namespace enemyfile;
    const AbsPose P = absoluteLivePose(e, time);
    const float tp = e.telegraphProgress();
    const bool  hurt = e.staggered() && !e.abReeling;
    const float pulse = 0.85f + 0.15f * std::sin(time * 3.f) * (hurt ? 3.f : 1.f);
    const bool ms = e.morning();
    const float dim = ms && e.abPhase >= 6 ? 0.08f : 1.f;
    const vec3 light = ms ? vec3{2.f, 1.9f, 1.8f} * dim : vec3{1.6f, 1.5f, 1.25f}, gold = ms ? vec3{2.f, 1.9f, 1.8f} * dim : vec3{1.5f, 1.1f, 0.45f};
    const vec3 seam = light * (hurt ? 0.5f : 1.3f) * pulse;
    const vec3 soft = ms ? vec3{1.f, 0.17f, 0.02f} * (hurt ? 0.35f : 0.6f) * dim : light * (hurt ? 0.04f : 0.1f);
    const AbTell tell0 = e.telegraphTimer > 0.f ? abTellOf(e.attack) : AbTell::NONE;
    const AbTell tell = (ms && tell0 != AbTell::WHITE) || absPlainTells() ? AbTell::NONE : tell0;
    const vec3 tc = tell == AbTell::GOLD ? abGoldTell(vec3{2.2f, 1.72f, 0.7f}) : tell == AbTell::RED ? vec3{2.4f, 0.25f, 0.15f} : vec3{1.9f, 1.9f, 2.0f};
    const float tk = 0.5f + 3.f * tp;
    auto lit = [&](int limb, vec3 base) {
        if (tell == AbTell::NONE || P.glow < 0) return base;
        bool on = P.glow == limb || P.glow == 5 || (P.glow == 3 && (limb == 0 || limb == 1));
        return on ? tc * tk : base;
    };
    fileRoot = root * absBodyLocal(e, P);
    // legs
    F.motion[E_HIPR] = RX(P.hipR) * RZ(-(P.stanceW + P.abdR));  F.motion[E_HIPL] = RX(P.hipL) * RZ(P.stanceW + P.abdL);
    F.motion[E_KNEE_R] = RX(P.kneeR);                F.motion[E_KNEE_L] = RX(P.kneeL);
    F.motion[E_ANKLE_R] = RX(-(P.kneeR + P.hipR) * P.ankle);
    F.motion[E_ANKLE_L] = RX(-(P.kneeL + P.hipL) * P.ankle);
    // the waist, the wraps that hang from it
    F.motion[E_WAIST] = RY(P.torsoYaw * 0.5f);
    const float sway = std::sin(time * 1.9f) * 0.08f + glm::clamp(e.moveSpeed / 30.f, 0.f, 0.6f);
    F.motion[E_CLOTH_F] = RX(-(sway + 0.025f));
    F.motion[E_CLOTH_B] = RX(sway + 0.125f);
    // torso, head (his eyes stay on you), the halo turning behind it
    F.motion[E_TORSO] = RY(P.torsoYaw) * RZ(P.torsoRoll) * RX(P.torsoPitch);
    F.motion[E_HEAD] = RY(-0.4f * (P.torsoYaw + P.hipYaw)) * RX(P.headPitch);
    F.motion[E_HALO] = RX(0.15f + P.torsoPitch * 0.3f) * RZ(time * 0.35f);
    // arms: shoulder, elbow, the sword hand's wrist, fingers and thumb curling into a fist, the wraps trailing
    for (float s : {-1.f, 1.f}) {
        const bool right = s < 0.f;
        const float sh = right ? P.shR : P.shL, shz = right ? P.shRz : P.shLz, el = right ? P.elR : P.elL;
        const float fist = right ? P.fistR : P.fistL;
        F.motion[right ? E_ARMR : E_ARML] = T({0.f, P.shrug, 0.f}) * RX(sh) * RZ(s * (0.1f + shz));
        F.motion[right ? E_ELBOW_R : E_ELBOW_L] = RX(-el);
        F.motion[right ? E_HAND_R : E_HAND_L] = absWrist(P, s);
        if (right && e.abBlade) {
            const vec3 grip{0.f, -0.07f, -0.035f};
            F.motion[E_HAND_R] = F.motion[E_HAND_R] * T(grip) * RY(P.bladeRoll) * RZ(-P.bladeCock) * T(-grip);
        }
        F.motion[right ? E_FINGERS_R : E_FINGERS_L] = RX(fist * AB_CURL[0]);
        F.motion[right ? E_TIPS_R : E_TIPS_L] = RX(fist * AB_CURL[1]);
        F.motion[right ? E_THUMB_R : E_THUMB_L] = RZ(-s * 0.5f) * RX(fist * AB_CURL[2]);
        F.motion[right ? E_THUMB_TIP_R : E_THUMB_TIP_L] = RX(fist * AB_CURL[3]);
        const float trail = 0.5f + glm::clamp(e.moveSpeed / 20.f, 0.f, 1.2f) + 0.12f * std::sin(time * 4.f + s);
        F.motion[right ? E_RIBBON_R : E_RIBBON_L] = RX(trail + 0.1f - (sh - el));
    }
    F.motion[E_SWORD] = RZ(1.5707963f);   // (through the fist, out of the thumb side)
    // what glows
    const vec3 eye = ms ? vec3{3.2f, 3.1f, 3.f} : vec3{2.8f, 0.18f, 0.12f};
    const float eyeK = (hurt ? 0.5f : 1.f) * (tell != AbTell::NONE ? 1.f + 0.6f * tp : 1.f) * pulse;
    const vec3 rune = ms ? vec3{1.9f, 1.8f, 1.7f} * dim : vec3{0.6f, 0.7f, 1.7f};
    F.chan[C_GLOW] = light;
    F.chan[C_SEAM] = seam;
    F.chan[C_SOFT] = soft;
    F.chan[C_LIGHT] = light;
    F.chan[C_EYE] = eye * eyeK;
    F.chan[C_CORE] = seam * (1.2f + 0.4f * std::sin(time * 6.f));
    F.chan[C_HALO] = gold * (hurt ? 0.5f : 1.6f) * pulse;
    F.chan[C_TELL_R] = lit(0, seam);    F.chan[C_TELL_L] = lit(1, seam);
    F.chan[C_TELL_LEG_R] = lit(2, seam); F.chan[C_TELL_LEG_L] = lit(6, seam);
    F.chan[C_AURA_R] = lit(0, soft);    F.chan[C_AURA_L] = lit(1, soft);
    F.chan[C_AURA_LEG_R] = lit(2, soft); F.chan[C_AURA_LEG_L] = lit(6, soft);
    F.chan[C_RUNE_R] = lit(0, rune * (0.85f + 0.15f * std::sin(time * 3.f - 1.f)));
    F.chan[C_RUNE_L] = lit(1, rune * (0.85f + 0.15f * std::sin(time * 3.f + 1.f)));
    F.chan[C_SWORD] = tell != AbTell::NONE && P.glow == 4 ? tc * tk : vec3{1.9f, 1.75f, 2.1f};
    F.flag[F_BLADE] = e.abBlade;
    F.flag[F_BROKEN] = hurt;
    F.wave = time * 2.5f;
    F.tint = ms ? 0.3f + 0.7f * dim : 1.f;
    glowOut = light;
    return true;
}
