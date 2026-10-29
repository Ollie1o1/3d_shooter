# UNIT 24's arm in first person, STORY mode (blender -b --factory-startup --python tools/blender/headless.py --
# tools/blender/unit24_arm_build.py): assets/guns/unit24_arm.glb, the same joints and layout as arm.glb (ArmFile.h,
# tools/blender/arm_build.py), so every hand pose and spring drives it. Off-white ceramic plates over a dark grey
# frame (unit24_build.py's): a plated upper arm on a bare shoulder servo, a bare elbow, the forearm plated on top
# with a piston, cables and a cyan status strip underneath, the back of the hand plated, the knuckles bare grey
# servos, every finger bone a frame core with a ceramic plate over its back, bare joints between. Over it, the
# parrying gauntlet ("gauntlet"): steel knuckle plates and studs that blaze on a parry ("flare").
#
#   upper (0, 0, 0)    the shoulder; the upper arm runs to z 0.4 (stretched to fit)
#   fore  (0, 0, 0.4)  the elbow; the forearm runs to z 0.76
#   fist  (0, 0, 0.8)  the hand (ArmFile.h's frame: fingers along +X, the back of the hand +Y, the wrist at -X)
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
_og.PALETTE.update({"ceramic": (0.84, 0.83, 0.79), "frame": (0.15, 0.16, 0.18), "servo": (0.30, 0.31, 0.34),
                    "chrome": (0.55, 0.57, 0.60), "cable": (0.07, 0.07, 0.08), "wire_red": (0.45, 0.08, 0.06),
                    "wire_amber": (0.62, 0.40, 0.08), "stencil": (0.10, 0.10, 0.11)})

sc = bpy.context.scene
sc.name = "UNIT24_ARM"
for ob in list(sc.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
add_joint("upper", (0, 0, 0))
add_joint("fore", (0, 0, 0.4))
add_joint("fist", (0, 0, 0.8))

CER, CER2, CER3 = "ceramic", "ceramic*0.9", "ceramic*0.75"
FR, FR2, SV, CH = "frame", "frame*1.45", "servo", "chrome"
STEEL, STEEL2 = "steel", "steel*0.85"
CYAN = (0.1, 0.75, 1.0)   # (a fixed cyan: "const" at this colour)
ALONG = (90, 0, 0)

# ---- the upper arm: a frame strut, a ceramic plate over its outside and top, the shoulder servo ----
U = "upper"
C((0, 0, 0.2), 0.026, 0.42, FR, U, rot=ALONG, sides=10)
P((0.006, 0.012, 0.21), (0.062, 0.32, 0.056), CER, U, rot=ALONG, top=(0.86, 0.9), bev=0.012, seg=2)
B((0.006, 0.041, 0.21), (0.03, 0.004, 0.26), CER3, U)                                          # its seam
C((0, 0, 0.02), 0.045, 0.07, SV, U, rot=(0, 0, 90), sides=14)                                  # shoulder servo
C((-0.02, -0.022, 0.2), 0.009, 0.30, SV, U, rot=ALONG, sides=8)                                # a piston underneath
C((-0.02, -0.022, 0.33), 0.005, 0.10, CH, U, rot=ALONG, sides=8)

# ---- the forearm: the elbow bare, a ceramic plate on top, the frame, a piston and cables underneath ----
F, Z0 = "fore", 0.4
C((0, 0, Z0), 0.036, 0.075, SV, F, rot=(0, 0, 90), sides=14)                                   # elbow servo
C((0, 0, Z0), 0.018, 0.08, FR, F, rot=(0, 0, 90), sides=10)
E((0, 0.0, Z0 - 0.018), (0.046, 0.042, 0.04), CER2, F, seg=12, rings=6)                       # elbow cap
C((0, -0.004, Z0 + 0.19), 0.02, 0.33, FR, F, rot=ALONG, sides=10)                              # strut
P((0, 0.014, Z0 + 0.20), (0.056, 0.27, 0.036), CER, F, rot=ALONG, top=(0.82, 0.8), bev=0.01, seg=2)   # top plate
B((0, 0.0335, Z0 + 0.20), (0.006, 0.003, 0.22), CER3, F)                                       # its seam
for t in (0.12, 0.30):
    for s in (1, -1):
        E((s * 0.024, 0.026, Z0 + t), (0.006, 0.006, 0.006), SV, F, seg=6, rings=3)            # bolts
C((0.012, -0.026, Z0 + 0.15), 0.009, 0.17, SV, F, rot=ALONG, sides=8)                          # piston: sleeve
C((0.012, -0.026, Z0 + 0.28), 0.005, 0.12, CH, F, rot=ALONG, sides=8)                          # its rod
tube([(-0.016, -0.02, Z0 + 0.06), (-0.02, -0.03, Z0 + 0.18), (-0.014, -0.022, Z0 + 0.33)], [0.0055] * 3, "cable", F, sides=6)
tube([(-0.006, -0.025, Z0 + 0.07), (-0.008, -0.034, Z0 + 0.2), (-0.004, -0.024, Z0 + 0.33)], [0.0045] * 3, "wire_amber", F, sides=6)
tube([(0.026, -0.012, Z0 + 0.08), (0.03, -0.018, Z0 + 0.2), (0.024, -0.01, Z0 + 0.32)], [0.004] * 3, "wire_red", F, sides=6)
for i in range(3):   # a status strip on the inside of the wrist, cyan
    B((0.03, 0.004, Z0 + 0.27 + i * 0.018), (0.004, 0.008, 0.01), "glow", F, glow="const", k=CYAN)
C((0, 0, Z0 + 0.345), 0.03, 0.014, SV, F, rot=ALONG, sides=14)                                 # wrist collar

# ---------------------------------------------------------------------------------------------------------------
# The hand, in hand space (the "fist" joint's frame), moved to where the joint is.
H = Vector((0.0, 0.0, 0.8))


def hv(x, y, z):
    return H + Vector((x, y, z))


def frame(at, along, back):
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
    r = m.to_3x3()
    if rot:
        r = r @ Euler([math.radians(v) for v in rot], "XYZ").to_matrix()
    e = r.to_euler("XYZ")
    return fn(tuple(m @ Vector(c)), s, *a, rot=tuple(math.degrees(v) for v in e), **kw)


def ellipsoid(m, c, s, mat, parent, **kw):
    return local(lambda cc, ss, *a, rot, **k: E(cc, ss, *a, rot=rot, **k), m, c, s, mat, parent, **kw)


# ---- the palm: a frame block, a ceramic plate on the back of the hand, the knuckle servo bar, the wrist joint ----
FIST = "fist"
PALM_Y0, PALM_Y1 = 0.011, 0.034
WRIST_X, KNUCKLE_X = -0.058, 0.02
cy = (PALM_Y0 + PALM_Y1) / 2
P(tuple(hv((WRIST_X + KNUCKLE_X) / 2, cy - 0.002, 0.0)), (PALM_Y1 - PALM_Y0 - 0.004, KNUCKLE_X - WRIST_X, 0.068), FR, FIST,
  rot=(0, 0, 90), top=(0.9, 0.82), bev=0.006, seg=2)                                           # palm frame
E(tuple(hv(-0.03, PALM_Y0 + 0.003, 0.02)), (0.036, 0.012, 0.03), FR2, FIST, seg=10, rings=6)   # the thumb's pad (a rubber grip)
E(tuple(hv(-0.034, PALM_Y0 + 0.003, -0.018)), (0.03, 0.012, 0.028), FR2, FIST, seg=10, rings=6)
P(tuple(hv(-0.018, PALM_Y1 + 0.002, 0.0)), (0.008, 0.062, 0.07), CER, FIST, rot=(0, 0, 90), top=(1.0, 0.86),
  bev=0.003, show="!gauntlet")                                                                 # back of the hand
B(tuple(hv(-0.018, PALM_Y1 + 0.0065, 0.0)), (0.046, 0.002, 0.003), CER3, FIST, show="!gauntlet")   # its seam
for s_ in (1, -1):
    E(tuple(hv(-0.04, PALM_Y1 + 0.006, s_ * 0.022)), (0.006, 0.005, 0.006), SV, FIST, seg=6, rings=3, show="!gauntlet")
C(tuple(hv(KNUCKLE_X - 0.006, cy + 0.002, -0.003)), 0.011, 0.074, SV, FIST, rot=(90, 0, 0), sides=10)   # knuckle bar
C(tuple(hv(WRIST_X - 0.002, cy, 0.0)), 0.024, 0.018, SV, FIST, rot=(0, 0, 90), sides=14)          # wrist servo
C(tuple(hv(WRIST_X - 0.002, cy, 0.0)), 0.012, 0.034, FR, FIST, rot=(90, 0, 0), sides=10)          # its axle

# ---- the fingers: three bones each: a dark frame core, a ceramic plate over its back, a bare grey joint ----
SPEC = {
    "index":  ((0.021, 0.022, 0.026),  (0.036, 0.022, 0.018),        0.0158, 0.0155),
    "middle": ((0.024, 0.022, 0.008),  (0.040, 0.025, 0.019),        0.0165, 0.0158),
    "ring":   ((0.021, 0.022, -0.010), (0.037, 0.023, 0.018),        0.0154, 0.0150),
    "pinky":  ((0.013, 0.021, -0.027), (0.029, 0.018, 0.016),        0.0135, 0.0136),
}
G = "gauntlet"


def finger_bones(name, frames, lengths, w, d, thumb=False):
    for i, (m, L) in enumerate(zip(frames, lengths)):
        J = "%s%d" % (name, i + 1)
        taper = 1.0 - 0.07 * i
        ww, dd = w * taper, d * taper
        tip = i == 2
        local(B, m, (0, -dd * 0.12, L * 0.52), (ww * 0.62, dd * 0.6, L * 0.86), FR, J, bev=0.002)            # frame core
        ellipsoid(m, (0, 0, 0), (ww * 0.96, dd * 0.92, dd * 0.92), SV, J, seg=10, rings=6)                     # the bare joint
        # the plate over its back, wrapping its sides; the tip's rounded to a fingertip
        if tip:
            local(P, m, (0, dd * 0.12, L * 0.52), (ww * 0.98, L * 0.82, dd * 0.72), CER, J, rot=(90, 0, 0),
                  top=(0.78, 0.7), bev=min(ww, dd) * 0.3, seg=2)
            ellipsoid(m, (0, dd * 0.02, L * 0.9), (ww * 0.74, dd * 0.6, L * 0.3), CER2, J, seg=10, rings=6)
        else:
            local(B, m, (0, dd * 0.14, L * 0.54), (ww * 0.98, dd * 0.68, L * 0.74), CER if i == 0 else CER2, J,
                  bev=min(ww, dd) * 0.26, seg=2)
            local(B, m, (0, dd * 0.49, L * 0.54), (ww * 0.2, 0.0015, L * 0.6), CER3, J)                      # a seam
        # the pad underneath: a dark grip strip
        local(B, m, (0, -dd * 0.44, L * 0.5), (ww * 0.7, 0.003, L * 0.62), FR2, J)
        # the gauntlet: steel plates over every bone
        local(P, m, (0, dd * 0.5 + 0.002, L * 0.52), (ww * 1.14, L * 1.02, 0.006), STEEL if i % 2 == 0 else "steel*0.9", J,
              rot=(90, 0, 0), top=(0.9, 1.0), bev=0.002, show=G)
        local(B, m, (0, dd * 0.5 + 0.0055, L * 0.5), (0.003, 0.002, L * 0.8), "steel*1.3", J, show=G)
        if tip:
            local(K, m, (0, dd * 0.18, L * 0.88), 0.0062 * taper, 0.012, "steel*1.2", J, rot=(90, 0, 0), sides=6, show=G)


for name, (kn, lengths, w, d) in SPEC.items():
    at = hv(*kn)
    frames, parent = [], FIST
    fan = {"index": 0.03, "middle": 0.0, "ring": -0.03, "pinky": -0.07}[name]
    along = Vector((1.0, 0.0, fan)).normalized()
    for i, L in enumerate(lengths):
        m = frame(at, along, (0, 1, 0))
        joint("%s%d" % (name, i + 1), m, parent)
        frames.append(m)
        parent = "%s%d" % (name, i + 1)
        at = at + along * L
    finger_bones(name, frames, lengths, w, d)

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
finger_bones("thumb", frames, th_len, 0.018, 0.016, thumb=True)
local(B, frames[0], (0, -0.004, 0.012), (0.02, 0.018, 0.026), FR2, "thumb1", bev=0.006, seg=2)   # the thumb's root
local(B, frames[0], (0, 0.006, 0.014), (0.021, 0.008, 0.024), CER, "thumb1", bev=0.003)           # plated

# ---- the parrying gauntlet over the hand: a steel knuckle-duster with studs that blaze, plates over the back ----
B(tuple(hv(0.02, 0.03, 0.0)), (0.016, 0.014, 0.074), STEEL, FIST, bev=0.004, show=G)
for k, (name, (kn, _, w, d)) in enumerate(SPEC.items()):
    B(tuple(hv(kn[0] + 0.004, 0.039, kn[2])), (0.012, 0.012, w * 0.82), "steel*1.2", FIST, bev=0.003, glow="flare", k=1.0, show=G)
for k in range(3):
    B(tuple(hv(-0.046 + 0.022 * k, PALM_Y1 + 0.004 + 0.0015 * k, 0.002)), (0.026, 0.006, 0.07 - 0.002 * k),
      "steel*0.9" if k % 2 == 0 else STEEL, FIST, rot=(0, 0, -6), bev=0.002, show=G)
B(tuple(hv(-0.016, PALM_Y1 + 0.0105, 0.0)), (0.058, 0.003, 0.009), "glow*0.5", FIST, glow="rune", k=1.0, show=G)

n = sum(1 for o in sc.objects if o.type == "MESH")
with contextlib.redirect_stdout(io.StringIO()):
    bpy.ops.export_scene.gltf(filepath=os.path.join(REPO, "assets", "guns", "unit24_arm.glb"), export_format="GLB",
                              export_extras=True, export_yup=True, export_apply=True, use_selection=False,
                              use_active_scene=True, export_texcoords=False)
shrink(os.path.join(REPO, "assets", "guns", "unit24_arm.glb"))
print("unit24_arm: %d parts saved" % n)
