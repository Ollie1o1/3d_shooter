# Your arm in first person (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/arm_build.py):
# assets/guns/arm.glb (ArmFile.h). A right arm, laid out down +Z in camera units (metres in your hands): a crimson coat
# sleeve with its folds, the cuff turned back at the elbow, a steel elbow cop, a blackened vambrace with steel bands and
# a seam lit in the sword's colour ("seam"), and a gloved hand with five jointed fingers. Over it, the parrying
# gauntlet ("gauntlet": steel plate on every finger bone, a knuckle-duster with bone studs that blaze on a parry,
# "flare"; a rune line on the back of the hand and a ring of light at its cuff, "rune").
#
#   upper (0, 0, 0)    the shoulder; the upper arm runs to z 0.4 (stretched to fit)
#   fore  (0, 0, 0.4)  the elbow; the forearm runs to z 0.76 (stretched to fit)
#   fist  (0, 0, 0.8)  the hand, framed by what it holds: the grip runs through the origin along +Z (the index finger's
#                      end), the wrist is at -X, the fingers reach out along +X (and curl round the grip toward -Y),
#                      +Y the back of the hand
#   thumb1-3, index1-3, middle1-3, ring1-3, pinky1-3: each finger's three joints, from the knuckle out. Each joint's
#                      own +Z runs down its bone toward the tip, +Y is the nail's side; it bends about its own +X
#                      (positive: toward the palm). The code bends them (ArmFile.h: HandPose); here the hand is flat.
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
FINGERS = ("thumb", "index", "middle", "ring", "pinky")
for _j in ["upper", "fore", "fist"] + ["%s%d" % (f, i) for f in FINGERS for i in (1, 2, 3)]:
    if _j not in _og.JOINTS:
        _og.JOINTS.append(_j)
for _c in ("flare", "rune"):
    if _c not in _og.CHANNELS:
        _og.CHANNELS.append(_c)
for _f in ("gauntlet", "!gauntlet"):
    if _f not in _og.FLAGS:
        _og.FLAGS.append(_f)
_og.PALETTE.update({"sleeve": (0.13, 0.045, 0.06), "swordglow": (1.0, 0.25, 0.45), "leather": (0.075, 0.07, 0.075)})

sc = bpy.context.scene
sc.name = "ARM"
for ob in list(sc.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
add_joint("upper", (0, 0, 0))
add_joint("fore", (0, 0, 0.4))
add_joint("fist", (0, 0, 0.8))

SLEEVE, SLEEVE2, SLEEVE3 = "sleeve", "sleeve*1.25", "sleeve*0.7"
BLACK, BLACK2, STEEL, STEEL2, BONE = "black*1.1", "black*1.35", "steel", "steel*0.85", "bone"
GLOVE, GLOVE2 = "leather", "leather*1.4"
ALONG = (90, 0, 0)   # (a cylinder turned from +Y onto +Z)

# ---- the upper arm: the coat sleeve, tapering, with folds and a seam ----
U = "upper"
C((0, 0, 0.2), 0.043, 0.43, SLEEVE, U, rot=ALONG, sides=14, r2=0.038)
for z, r in ((0.13, 0.042), (0.23, 0.041), (0.31, 0.04)):
    R((0, -0.003, z), r, 0.005, SLEEVE3, U, sides=16, tsides=4)                                  # folds
B((0, 0.041, 0.2), (0.006, 0.004, 0.4), SLEEVE2, U)                                             # its seam

# ---- the forearm: the cuff turned back, the elbow cop, the vambrace with bands and a lit seam ----
F, Z0 = "fore", 0.4
C((0, 0, Z0 + 0.054), 0.042, 0.108, SLEEVE2, F, rot=ALONG, sides=14, r2=0.039)                  # cuff
R((0, 0, Z0 + 0.104), 0.039, 0.006, SLEEVE, F, sides=16, tsides=4)                              # its lip
E((0, 0.0, Z0), (0.062, 0.062, 0.062), STEEL2, F, seg=12, rings=6)                              # elbow cop
R((0, 0.0, Z0), 0.031, 0.004, STEEL, F, rot=(0, 90, 0), sides=16, tsides=4)
P((0, 0, Z0 + 0.234), (0.05, 0.252, 0.046), BLACK2, F, rot=ALONG, top=(0.84, 0.86), bev=0.008, seg=2)   # vambrace
for t in (0.42, 0.7, 0.94):
    C((0, 0, Z0 + 0.36 * t), 0.0285 - 0.004 * t, 0.02, STEEL, F, rot=ALONG, sides=14)            # bands
B((0, 0.0245, Z0 + 0.36 * 0.66), (0.008, 0.004, 0.36 * 0.52), "swordglow", F, glow="seam", k=1.0)   # the seam along its top
for t in (0.55, 0.82):
    for s in (1, -1):
        E((s * 0.025, 0.0, Z0 + 0.36 * t), (0.006, 0.006, 0.006), STEEL, F, seg=6, rings=3)      # rivets

# ---------------------------------------------------------------------------------------------------------------
# The hand. Everything below is in hand space (the "fist" joint's frame), moved to where the joint is.
H = Vector((0.0, 0.0, 0.8))


def hv(x, y, z):
    return H + Vector((x, y, z))


def frame(at, along, back):
    """A game-space frame at `at`: +Z along `along`, +Y toward `back` (squared up to it), +X = Y x Z"""
    z = Vector(along).normalized()
    y = Vector(back) - z * Vector(back).dot(z)
    y.normalize()
    x = y.cross(z)
    m = Matrix.Identity(4)
    for r in range(3):
        m[r][0], m[r][1], m[r][2], m[r][3] = x[r], y[r], z[r], at[r]
    return m


def joint(name, m, parent):
    ob = bpy.data.objects.new(name, None)
    ob.empty_display_type = "ARROWS"
    ob.empty_display_size = 0.01
    sc.collection.objects.link(ob)
    ob.parent = find(parent)
    ob.matrix_world = _og.game_to_blender(m)
    return m


def local(fn, m, c, s, *a, rot=None, **kw):
    """fn (B, E, P...) placed in frame m: centre c and size s along m's own axes, turned by rot (degrees) in it"""
    r = m.to_3x3()
    if rot:
        r = r @ Euler([math.radians(v) for v in rot], "XYZ").to_matrix()
    e = r.to_euler("XYZ")
    return fn(tuple(m @ Vector(c)), s, *a, rot=tuple(math.degrees(v) for v in e), **kw)


def ellipsoid(m, c, s, mat, parent, **kw):
    return local(lambda cc, ss, *a, rot, **k: E(cc, ss, *a, rot=rot, **k), m, c, s, mat, parent, **kw)


# ---- the palm: a padded glove over the back of the hand, the heel of the palm, the knuckle ridge ----
FIST = "fist"
PALM_Y0, PALM_Y1 = 0.011, 0.036          # the palm's face (against the grip) and the back of the hand
WRIST_X, KNUCKLE_X = -0.058, 0.02
pm = frame(H, (1, 0, 0), (0, 1, 0))      # (the palm's own frame: +Z toward the knuckles)
cy = (PALM_Y0 + PALM_Y1) / 2
# the body of the hand, wider at the knuckles than the wrist
P(tuple(hv((WRIST_X + KNUCKLE_X) / 2, cy, 0.0)), (PALM_Y1 - PALM_Y0, KNUCKLE_X - WRIST_X, 0.072), GLOVE, FIST,
  rot=(0, 0, 90), top=(0.92, 0.8), bev=0.008, seg=2)                       # (its +Y end, turned to the wrist, narrower)
E(tuple(hv(-0.03, PALM_Y0 + 0.004, 0.02)), (0.04, 0.016, 0.032), GLOVE2, FIST, seg=10, rings=6)   # the thumb's pad
E(tuple(hv(-0.034, PALM_Y0 + 0.004, -0.018)), (0.034, 0.014, 0.03), GLOVE2, FIST, seg=10, rings=6)   # the heel of the palm
C(tuple(hv(WRIST_X - 0.004, cy, 0.002)), 0.03, 0.016, STEEL, FIST, rot=(0, 0, 90), sides=16, r2=0.031,
  show="!gauntlet")                                                                                    # the glove's cuff band
B(tuple(hv(-0.016, PALM_Y1 + 0.001, 0.002)), (0.05, 0.004, 0.05), BLACK2, FIST, bev=0.004, show="!gauntlet")   # a plate on its back
for s_ in (1, -1):
    B(tuple(hv(-0.016, PALM_Y1 + 0.003, 0.002 + s_ * 0.014)), (0.044, 0.003, 0.004), STEEL2, FIST, show="!gauntlet")   # its edges

# ---- the fingers: three bones each, a knuckle at every joint ----
#          knuckle (x, y, z)         lengths (base, middle, tip)    width  depth
SPEC = {
    "index":  ((0.021, 0.023, 0.026),  (0.036, 0.022, 0.018),        0.0165, 0.0165),
    "middle": ((0.024, 0.023, 0.008),  (0.040, 0.025, 0.019),        0.0172, 0.0168),
    "ring":   ((0.021, 0.023, -0.010), (0.037, 0.023, 0.018),        0.0160, 0.0160),
    "pinky":  ((0.013, 0.022, -0.027), (0.029, 0.018, 0.016),        0.0140, 0.0145),
}
G = "gauntlet"


def finger_bones(name, frames, lengths, w, d, thumb=False):
    """A finger's bones on its joints (frames: each joint's game-space frame): a padded glove segment, a knuckle,
    a bone plate (gloved) or a steel plate (the gauntlet) over each"""
    for i, (m, L) in enumerate(zip(frames, lengths)):
        J = "%s%d" % (name, i + 1)
        taper = 1.0 - 0.08 * i
        ww, dd = w * taper, d * taper
        tip = i == 2
        # the segment: a rounded block down its bone (the tip rounder, narrowing to the fingertip)
        if tip:
            local(P, m, (0, 0, L * 0.45), (ww, L * 0.9, dd), GLOVE, J, rot=(90, 0, 0), top=(0.8, 0.72), bev=min(ww, dd) * 0.42, seg=2)
            ellipsoid(m, (0, -dd * 0.06, L * 0.86), (ww * 0.8, dd * 0.72, L * 0.34), GLOVE, J, seg=10, rings=6)
        else:
            local(B, m, (0, 0, L * 0.5), (ww, dd, L * 0.98), GLOVE, J, bev=min(ww, dd) * 0.36, seg=2)
        ellipsoid(m, (0, 0, 0), (ww * 1.04, dd * 1.0, dd * 0.96), GLOVE2, J, seg=10, rings=6)            # the knuckle
        # the glove: a bone plate over the base bone, small steel caps over the others
        if i == 0 and not thumb:
            local(B, m, (0, dd * 0.5 + 0.0015, L * 0.42), (ww * 0.78, 0.004, L * 0.56), BONE, J, bev=0.0015, show="!gauntlet")
        elif not tip:
            local(B, m, (0, dd * 0.5 + 0.001, L * 0.5), (ww * 0.66, 0.003, L * 0.6), STEEL2, J, bev=0.001, show="!gauntlet")
        else:
            local(B, m, (0, dd * 0.48 + 0.001, L * 0.45), (ww * 0.56, 0.003, L * 0.5), STEEL2, J, bev=0.001, show="!gauntlet")
        # the gauntlet: overlapping steel plates on every bone, a ridge down each, a claw-tip on the last
        local(P, m, (0, dd * 0.5 + 0.002, L * 0.52), (ww * 1.12, L * 1.02, 0.006), STEEL if i % 2 == 0 else "steel*0.9", J,
              rot=(90, 0, 0), top=(0.9, 1.0), bev=0.002, show=G)
        local(B, m, (0, dd * 0.5 + 0.0055, L * 0.5), (0.003, 0.002, L * 0.8), "steel*1.3", J, show=G)
        local(B, m, (ww * 0.56, 0, L * 0.5), (0.003, dd * 0.7, L * 0.8), STEEL2, J, show=G)                    # side plates
        local(B, m, (-ww * 0.56, 0, L * 0.5), (0.003, dd * 0.7, L * 0.8), STEEL2, J, show=G)
        if tip:
            local(K, m, (0, dd * 0.18, L * 0.88), 0.0062 * taper, 0.012, "steel*1.2", J, rot=(90, 0, 0), sides=6, show=G)


for name, (kn, lengths, w, d) in SPEC.items():
    at = hv(*kn)
    frames, parent = [], FIST
    # rest: straight out along +X, a touch of fan (the index toward +Z, the pinky away)
    fan = {"index": 0.03, "middle": 0.0, "ring": -0.03, "pinky": -0.07}[name]
    along = Vector((1.0, 0.0, fan)).normalized()
    for i, L in enumerate(lengths):
        m = frame(at, along, (0, 1, 0))
        joint("%s%d" % (name, i + 1), m, parent)
        frames.append(m)
        parent = "%s%d" % (name, i + 1)
        at = at + along * L
    finger_bones(name, frames, lengths, w, d)

# the thumb: from the root of the palm by the index finger, out along the index side, its nail facing out and up
th_at = hv(-0.029, 0.013, 0.027)
th_along = Vector((0.62, -0.18, 0.76)).normalized()
th_back = Vector((-0.35, 0.75, 0.55))
th_len = (0.03, 0.026, 0.021)
frames, parent, at = [], FIST, th_at
for i, L in enumerate(th_len):
    m = frame(at, th_along, th_back)
    joint("thumb%d" % (i + 1), m, parent)
    frames.append(m)
    parent = "thumb%d" % (i + 1)
    at = at + th_along * L
finger_bones("thumb", frames, th_len, 0.019, 0.017, thumb=True)
local(B, frames[0], (0, -0.004, 0.012), (0.022, 0.02, 0.028), GLOVE2, "thumb1", bev=0.008, seg=2)   # the thumb's root, filling the palm

# ---- the parrying gauntlet over the hand: a knuckle-duster across the knuckles with lit bone studs, lames over the back
#      of the hand with a rune line, a cuff with a ring of light ----
B(tuple(hv(0.02, 0.03, 0.0)), (0.016, 0.014, 0.074), STEEL, FIST, bev=0.004, show=G)                     # the knuckle-duster
for k, (name, (kn, _, w, d)) in enumerate(SPEC.items()):
    B(tuple(hv(kn[0] + 0.004, 0.039, kn[2])), (0.012, 0.012, w * 0.82), BONE, FIST, bev=0.003, glow="flare", k=1.0, show=G)   # bone studs
    K(tuple(hv(kn[0] + 0.004, 0.044, kn[2])), 0.0055, 0.011, BONE, FIST, sides=6, show=G)                                # their points
for k in range(3):   # the back of the hand: three lames overlapping toward the knuckles, a raised ridge
    B(tuple(hv(-0.046 + 0.022 * k, PALM_Y1 + 0.004 + 0.0015 * k, 0.002)), (0.026, 0.006, 0.07 - 0.002 * k),
      "steel*0.9" if k % 2 == 0 else STEEL, FIST, rot=(0, 0, -6), bev=0.002, show=G)
for s_ in (1, -1):
    B(tuple(hv(-0.016, PALM_Y1 + 0.009, s_ * 0.022)), (0.064, 0.004, 0.005), "steel*1.3", FIST, bev=0.001, show=G)   # its ridges
B(tuple(hv(-0.016, PALM_Y1 + 0.0105, 0.0)), (0.058, 0.003, 0.009), "glow*0.5", FIST, glow="rune", k=1.0, show=G)   # its rune line
for s_ in (1, -1):
    B(tuple(hv(-0.002, PALM_Y1 + 0.0105, s_ * 0.012)), (0.026, 0.003, 0.004), "glow*0.5", FIST, rot=(0, s_ * 30, 0),
      glow="rune", k=0.7, show=G)
R(tuple(hv(WRIST_X - 0.004, cy, 0.002)), 0.034, 0.004, "glow*0.5", FIST, rot=(0, 90, 0), sides=20, tsides=4, glow="rune", k=1.0, show=G)   # the cuff's ring
C(tuple(hv(WRIST_X - 0.016, cy, 0.002)), 0.035, 0.022, STEEL, FIST, rot=(0, 0, 90), sides=16, show=G)    # the cuff

n = sum(1 for o in sc.objects if o.type == "MESH")
with contextlib.redirect_stdout(io.StringIO()):
    bpy.ops.export_scene.gltf(filepath=os.path.join(REPO, "assets", "guns", "arm.glb"), export_format="GLB",
                              export_extras=True, export_yup=True, export_apply=True, use_selection=False,
                              use_active_scene=True, export_texcoords=False)
shrink(os.path.join(REPO, "assets", "guns", "arm.glb"))
print("arm: %d parts saved" % n)
