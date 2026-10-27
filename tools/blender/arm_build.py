# Your arm in first person (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/arm_build.py):
# assets/guns/arm.glb (ArmFile.h). A right arm, laid out straight down +Z in camera units (metres in your hands):
# a crimson coat sleeve with its folds, the cuff turned back at the elbow, a steel elbow cop, a blackened vambrace with
# steel bands and a seam lit in the sword's colour ("seam"), and three hands on "fist": the gloved fist that closes on a
# grip ("!open"), a flat palm for the blade ("open"), and the parrying gauntlet over it ("gauntlet": a knuckle-duster
# with bone studs that blaze on a parry, "flare"; a rune line and a ring of light at its cuff, "rune").
#   upper (0, 0, 0): the shoulder; the upper arm runs to z 0.4 (stretched to fit)
#   fore (0, 0, 0.4): the elbow; the forearm runs to z 0.76 (stretched to fit)
#   fist (0, 0, 0.8): the hand. +Z along the grip (or the punch), +Y the back of the hand, +X the thumb's side
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
for _j in ("upper", "fore", "fist"):
    if _j not in _og.JOINTS:
        _og.JOINTS.append(_j)
for _c in ("flare", "rune"):
    if _c not in _og.CHANNELS:
        _og.CHANNELS.append(_c)
for _f in ("gauntlet", "!gauntlet"):
    if _f not in _og.FLAGS:
        _og.FLAGS.append(_f)
_og.PALETTE.update({"sleeve": (0.13, 0.045, 0.06), "swordglow": (1.0, 0.25, 0.45)})

sc = bpy.context.scene
sc.name = "ARM"
for ob in list(sc.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
add_joint("upper", (0, 0, 0))
add_joint("fore", (0, 0, 0.4))
add_joint("fist", (0, 0, 0.8))

SLEEVE, SLEEVE2, SLEEVE3 = "sleeve", "sleeve*1.25", "sleeve*0.7"
BLACK, BLACK2, STEEL, STEEL2, BONE = "black*1.1", "black*1.35", "steel", "steel*0.85", "bone"
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
P((0, 0, Z0 + 0.234), (0.05, 0.252, 0.046), BLACK2, F, rot=ALONG, top=(0.88, 0.9), bev=0.008, seg=2)   # vambrace
for t in (0.42, 0.7, 0.94):
    C((0, 0, Z0 + 0.36 * t), 0.0285 - 0.003 * t, 0.02, STEEL, F, rot=ALONG, sides=14)            # bands
B((0, 0.0245, Z0 + 0.36 * 0.66), (0.008, 0.004, 0.36 * 0.52), "swordglow", F, glow="seam", k=1.0)   # the seam along its top
for t in (0.55, 0.82):
    for s in (1, -1):
        E((s * 0.025, 0.0, Z0 + 0.36 * t), (0.006, 0.006, 0.006), STEEL, F, seg=6, rings=3)      # rivets

# ---- the closed hand: knuckles round the grip on the far side, the thumb over the top, the back of the glove ----
H, ZH = "fist", 0.8
B((0, 0.004, ZH), (0.05, 0.054, 0.05), BLACK, H, bev=0.012, seg=2, show="!open")                 # fingers round the grip
for k in range(4):
    z = ZH + 0.018 - 0.012 * k
    B((-0.026, 0.006, z), (0.012, 0.022, 0.011), BLACK2, H, bev=0.003, show="!open")             # each finger's roll
    B((-0.031, 0.008, z), (0.004, 0.012, 0.008), BONE, H, show="!open")                          # its bone plate
B((0.022, -0.004, ZH - 0.006), (0.02, 0.04, 0.04), BLACK2, H, bev=0.006, show="!open")           # the back of the hand
P((0.02, 0.022, ZH + 0.016), (0.016, 0.03, 0.017), BLACK2, H, rot=ALONG, top=(0.8, 0.8), bev=0.004, show="!open")   # thumb
C((0, 0.0, ZH - 0.03), 0.031, 0.014, STEEL, H, rot=ALONG, sides=14, show="!open")                # the glove's cuff band

# ---- the open palm: flat on the blade ----
B((0, 0, ZH), (0.058, 0.02, 0.07), BLACK, H, bev=0.006, show="open")
for k in range(4):
    x = -0.021 + 0.014 * k
    B((x, 0.002, ZH + 0.046), (0.011, 0.014, 0.024), BLACK2, H, bev=0.003, show="open")          # fingers
    B((x, 0.004, ZH + 0.06), (0.01, 0.012, 0.008), BONE, H, show="open")                         # their tips
P((0.034, 0.0, ZH + 0.006), (0.014, 0.034, 0.014), BLACK2, H, rot=(90, 0, -35), top=(0.8, 0.8), bev=0.003, show="open")   # thumb

# ---- the parrying gauntlet: a shell, curled plated fingers, a knuckle-duster with lit bone studs, the thumb, a plate
#      over the back of the hand with a rune line, a cuff with a ring of light ----
G = "gauntlet"
B((0, 0, ZH), (0.085, 0.075, 0.08), "black*1.0", H, bev=0.014, seg=2, show=G)                     # the shell
B((0, -0.022, ZH + 0.036), (0.08, 0.03, 0.036), BLACK2, H, bev=0.008, show=G)                     # curled fingers
for k in range(4):
    x = -0.03 + 0.02 * k
    B((x, -0.006, ZH + 0.05), (0.017, 0.012, 0.016), STEEL2, H, bev=0.003, show=G)               # their plates
    B((x, 0.016, ZH + 0.059), (0.015, 0.02, 0.01), BONE, H, bev=0.003, glow="flare", k=1.0, show=G)   # bone studs
    K((x, 0.016, ZH + 0.063), 0.006, 0.012, BONE, H, rot=ALONG, sides=6, show=G)                 # their points
B((0, 0.012, ZH + 0.046), (0.092, 0.046, 0.02), STEEL, H, bev=0.006, show=G)                      # the knuckle-duster
B((0.04, -0.012, ZH + 0.024), (0.02, 0.026, 0.05), BLACK2, H, bev=0.006, show=G)                  # the thumb across them
for k in range(3):   # the back of the hand: three lames overlapping toward the knuckles, a raised ridge
    P((0, 0.04 + 0.002 * k, ZH - 0.03 + 0.024 * k), (0.078 - 0.004 * k, 0.03, 0.012), "steel*0.9" if k % 2 == 0 else STEEL, H,
      rot=(80, 0, 0), top=(0.92, 1.0), bev=0.003, show=G)
for s_ in (1, -1):
    B((s_ * 0.026, 0.05, ZH - 0.004), (0.006, 0.006, 0.07), "steel*1.3", H, bev=0.002, show=G)   # its ridges
B((0, 0.048, ZH - 0.004), (0.012, 0.004, 0.06), "glow*0.5", H, glow="rune", k=1.0, show=G)        # its rune line
for s in (1, -1):
    B((s * 0.016, 0.047, ZH + 0.012), (0.004, 0.004, 0.028), "glow*0.5", H, rot=(0, s * 30, 0), glow="rune", k=0.7, show=G)
R((0, 0, ZH - 0.056), 0.046, 0.005, "glow*0.5", H, sides=20, tsides=4, glow="rune", k=1.0, show=G)   # the cuff's ring
C((0, 0, ZH - 0.068), 0.047, 0.022, STEEL, H, rot=ALONG, sides=16, show=G)                       # the cuff

n = sum(1 for o in sc.objects if o.type == "MESH")
with contextlib.redirect_stdout(io.StringIO()):
    bpy.ops.export_scene.gltf(filepath=os.path.join(REPO, "assets", "guns", "arm.glb"), export_format="GLB",
                              export_extras=True, export_yup=True, export_apply=True, use_selection=False,
                              use_active_scene=True, export_texcoords=False)
print("arm: %d parts saved" % n)
