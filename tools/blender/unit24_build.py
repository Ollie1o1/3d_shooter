# UNIT 24, STORY mode's robot (blender -b --factory-startup --python tools/blender/headless.py --
# tools/blender/unit24_build.py): assets/enemies/unit24.glb. Lean and human-sized (1.8 m), off-white ceramic
# plates over a dark grey frame, half-finished: the plate over his left temple was never fitted (wiring and the
# back of his eye show) and his right side is open down the ribs on a cyan glow. A rounded head with a brow plate
# over two expressive cyan eyes, no mouth. 24 stencilled on his chest (the wrecks carry their own numbers there:
# NUMBER= on the command line; NUMBER= OUTNAME=unit_blank: none), a cyan power spine down his back. Cyan parts glow on "eye" (his eyes) and "core"
# (the spine, the light inside him); the code dims them (a wreck: dark).
#
# Game space: +Y up, he faces +Z, his left is +X, metres. Joints (a full skeleton, so he can be posed: the
# cutscene, the bodies on the belt, the wrecks slumped where they fell):
#   waist (0, 1.02, 0) > torso (0, 1.18, 0) > head (0, 1.56, 0)
#                                           > armL/R (+-0.215, 1.45, 0) > elbowL/R (+-0.235, 1.17, -0.01)
#                                             > handL/R (+-0.24, 0.90, 0) > fingersL/R (+-0.24, 0.83, 0.005),
#                                                                           thumbL/R (+-0.215, 0.88, 0.025)
#   hipL/R (+-0.095, 0.95, 0) > kneeL/R (+-0.10, 0.52, 0.01) > ankleL/R (+-0.10, 0.085, -0.01)
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())

NAME = globals().get("OUTNAME", "unit24")
NUMBER = str(globals().get("NUMBER", "24"))
NEEDED[NAME] = ("waist torso head armL armR elbowL elbowR handL handR fingersL fingersR thumbL thumbR "
                "hipL hipR kneeL kneeR ankleL ankleR")
_og.PALETTE.update({
    "ceramic": (0.84, 0.83, 0.79),     # his plates: off-white, a little warm
    "frame": (0.15, 0.16, 0.18),       # the frame under them
    "servo": (0.30, 0.31, 0.34),       # joints, pistons
    "chrome": (0.55, 0.57, 0.60),      # piston rods
    "cable": (0.07, 0.07, 0.08),
    "wire_red": (0.45, 0.08, 0.06), "wire_amber": (0.62, 0.40, 0.08), "wire_blue": (0.10, 0.20, 0.42),
    "board": (0.08, 0.20, 0.12),       # a circuit board behind the temple
    "stencil": (0.10, 0.10, 0.11),
    "lens": (0.02, 0.03, 0.035),
})

# ---- the scene: its own, with the skeleton made here (the first build has no file to open) ----
sc = bpy.context.scene
sc.name = NAME.upper()
for ob in list(sc.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
sc["overdrive_opened"] = NAME
add_joint("waist", (0, 1.02, 0))
add_joint("torso", (0, 1.18, 0), "waist")
add_joint("head", (0, 1.56, 0), "torso")
for s, side in ((1, "L"), (-1, "R")):
    add_joint("arm" + side, (s * 0.215, 1.45, 0), "torso")
    add_joint("elbow" + side, (s * 0.235, 1.17, -0.01), "arm" + side)
    add_joint("hand" + side, (s * 0.24, 0.90, 0), "elbow" + side)
    add_joint("fingers" + side, (s * 0.24, 0.83, 0.005), "hand" + side)
    add_joint("thumb" + side, (s * 0.215, 0.88, 0.025), "hand" + side)
    add_joint("hip" + side, (s * 0.095, 0.95, 0))
    add_joint("knee" + side, (s * 0.10, 0.52, 0.01), "hip" + side)
    add_joint("ankle" + side, (s * 0.10, 0.085, -0.01), "knee" + side)

CER, CER2, CER3 = "ceramic", "ceramic*0.9", "ceramic*0.78"   # plate, a shade down, a seam's shadow
FR, FR2 = "frame", "frame*1.4"
SV, CH = "servo", "chrome"
CYAN = "glow"   # (glows on a channel: the code sets the colour)


def bolt(c, parent, r=0.008, mat=SV):
    return E(c, (r * 2, r * 2, r * 1.2), mat, parent, seg=8, rings=4)


def cable(points, r, mat, parent, sides=6):
    return tube(points, [r] * len(points), mat, parent, sides=sides)


def piston(a, b, parent, r=0.012):
    """A sleeve and a rod: a servo cylinder from a, its chrome rod to b"""
    a, b = Vector(a), Vector(b)
    m = a + (b - a) * 0.55
    C(tuple((a + m) * 0.5), r, (m - a).length, SV, parent, rot=aimed(None, b - a), sides=8)
    C(tuple((m + b) * 0.5), r * 0.55, (b - m).length, CH, parent, rot=aimed(None, b - a), sides=8)


# ---- digits: stencilled, seven segments, the stencil's breaks at the joins ----
SEG = {"0": "abcdef", "1": "bc", "2": "abged", "3": "abgcd", "4": "fgbc", "5": "afgcd", "6": "afgedc", "7": "abc",
       "8": "abcdefg", "9": "abcdfg"}


def digit(ch, cx, cy, z, h, parent, mat="stencil"):
    w, t = h * 0.56, h * 0.15
    hw, hh = w / 2, h / 2
    seg = {"a": (cx, cy + hh, w * 0.78, t), "g": (cx, cy, w * 0.78, t), "d": (cx, cy - hh, w * 0.78, t),
           "f": (cx - hw, cy + hh / 2, t, hh * 0.78), "b": (cx + hw, cy + hh / 2, t, hh * 0.78),
           "e": (cx - hw, cy - hh / 2, t, hh * 0.78), "c": (cx + hw, cy - hh / 2, t, hh * 0.78)}
    for k in SEG.get(ch, ""):
        x, y, sx, sy = seg[k]
        # (seen from the front, his left is the viewer's right: +X reads as the digit's right)
        B((x, y, z), (sx, sy, 0.004), mat, parent)


# ============================================================================================
# LEGS: a frame strut down each, a ceramic thigh and shin plate in front, the knee and ankle bare servo, a
# piston behind each calf, plated feet
# ============================================================================================
for s, side in ((1, "L"), (-1, "R")):
    hip, knee, ank = "hip" + side, "knee" + side, "ankle" + side
    x = s * 0.10
    C((s * 0.095, 0.95, 0.0), 0.05, 0.10, SV, hip, rot=(0, 0, 90), sides=12)                         # hip servo
    C((x, 0.735, 0.0), 0.034, 0.40, FR, hip, sides=8)                                                # thigh strut
    P((x, 0.75, 0.032), (0.115, 0.34, 0.10), CER, hip, top=(1.2, 1.05), bev=0.028, seg=3)             # thigh plate
    B((x, 0.75, 0.093), (0.012, 0.26, 0.006), CER3, hip)                                             # its seam
    P((x, 0.74, -0.045), (0.09, 0.30, 0.045), FR2, hip, top=(1.15, 1.0), bev=0.012)                   # back of the thigh
    piston((x + s * 0.05, 0.88, -0.03), (x + s * 0.045, 0.58, -0.02), hip, r=0.011)                  # outer piston
    for y in (0.86, 0.64):
        bolt((x + s * 0.066, y, 0.04), hip)
    # the knee: a bare servo, a small ceramic cap
    C((x, 0.52, 0.01), 0.052, 0.12, SV, knee, rot=(0, 0, 90), sides=14)
    C((x, 0.52, 0.01), 0.030, 0.13, FR, knee, rot=(0, 0, 90), sides=10)
    P((x, 0.535, 0.065), (0.085, 0.09, 0.03), CER, knee, rot=(-12, 0, 0), top=(0.8, 1.0), bev=0.012)  # kneecap
    # shin
    C((x, 0.30, -0.005), 0.03, 0.40, FR, knee, sides=8)
    P((x, 0.30, 0.035), (0.095, 0.36, 0.07), CER, knee, top=(1.25, 1.1), bev=0.022, seg=3)            # shin guard
    B((x, 0.31, 0.077), (0.06, 0.012, 0.005), CER3, knee)
    piston((x, 0.46, -0.05), (x, 0.13, -0.045), knee, r=0.016)                                       # calf piston
    P((x, 0.33, -0.062), (0.075, 0.20, 0.03), FR2, knee, top=(1.2, 1.0), bev=0.01)                   # calf frame
    for (c1, c2) in (((x - 0.03, 0.45, -0.03), (x - 0.025, 0.16, -0.035)),):
        cable([c1, (c1[0] - 0.005, 0.30, -0.04), c2], 0.006, "cable", knee)
    # ankle and foot
    C((x, 0.085, -0.01), 0.034, 0.09, SV, ank, rot=(0, 0, 90), sides=12)
    B((x, 0.022, 0.03), (0.105, 0.044, 0.26), FR, ank, bev=0.012)                                    # sole
    P((x, 0.058, 0.075), (0.10, 0.05, 0.14), CER, ank, rot=(8, 0, 0), top=(0.9, 0.85), bev=0.016)    # toe plate
    B((x, 0.055, -0.065), (0.09, 0.07, 0.07), FR2, ank, bev=0.012)                                   # heel
    B((x, 0.030, 0.16), (0.095, 0.03, 0.03), SV, ank, bev=0.008)                                     # toe hinge

# ============================================================================================
# PELVIS and WAIST: a frame block with ceramic hip plates, the spine bare between it and the chest: discs and
# cables, a few abdominal plates in front
# ============================================================================================
B((0, 0.97, 0.0), (0.27, 0.11, 0.16), FR, None, bev=0.02)
P((0, 0.965, 0.07), (0.24, 0.12, 0.05), CER, None, rot=(0, 0, 180), top=(0.62, 1.0), bev=0.018, seg=2)   # front plate
P((0, 0.975, -0.07), (0.25, 0.11, 0.04), CER2, None, rot=(0, 0, 180), top=(0.7, 1.0), bev=0.014)          # back plate
for s in (1, -1):
    P((s * 0.135, 0.96, 0.0), (0.03, 0.12, 0.15), CER, None, rot=(0, 0, s * 6), top=(1.0, 0.85), bev=0.012)  # hip plates
W_ = "waist"
for i in range(4):   # spine discs
    C((0, 1.04 + i * 0.035, -0.02), 0.045 - 0.003 * (i % 2), 0.022, SV if i % 2 else FR2, W_, sides=12)
for s in (1, -1):    # cables either side of it
    cable([(s * 0.06, 1.01, 0.0), (s * 0.075, 1.09, 0.015), (s * 0.07, 1.17, 0.0)], 0.008, "cable", W_)
cable([(0.035, 1.01, 0.035), (0.04, 1.10, 0.05), (0.03, 1.17, 0.04)], 0.006, "wire_red", W_)
for i in range(2):   # abdominal plates (the lower two)
    P((0, 1.065 + i * 0.05, 0.065), (0.17 - i * 0.01, 0.04, 0.035), CER if i else CER2, W_, top=(1.08, 1.0), bev=0.01)

# ============================================================================================
# TORSO: the frame core; ceramic chest plates (his left whole, with the number; his right cut short over open
# ribs and the light inside); collar plates, a ceramic back plate split by the power spine; shoulder caps
# ============================================================================================
T = "torso"
B((0.04, 1.32, -0.01), (0.18, 0.30, 0.17), FR, T, bev=0.02)                                       # core (his left and the spine)
B((-0.09, 1.30, -0.075), (0.10, 0.26, 0.03), FR, T, bev=0.006)                                     # the open side's back wall
P((0, 1.205, 0.04), (0.22, 0.06, 0.10), CER2, T, top=(1.1, 1.0), bev=0.012)                       # upper abdomen plate
# his left chest plate, whole
P((0.07, 1.36, 0.07), (0.15, 0.24, 0.07), CER, T, rot=(0, 6, 0), top=(1.18, 1.0), shift=(0.015, 0.0), bev=0.03, seg=3)
if NUMBER:   # (NUMBER= blank: a body never given one, the rejects on the belt)
    digits = NUMBER[-2:].rjust(2, "0")
    for i, ch in enumerate(digits):
        digit(ch, 0.045 + i * 0.05, 1.395, 0.1075, 0.062, T)
    B((0.07, 1.30, 0.1065), (0.10, 0.004, 0.003), "stencil", T)                                  # an underline
# his right chest plate: only the top third fitted
P((-0.07, 1.435, 0.07), (0.15, 0.09, 0.07), CER, T, rot=(0, -6, 0), top=(1.15, 1.0), shift=(-0.012, 0.0), bev=0.025, seg=3)
B((-0.07, 1.39, 0.09), (0.13, 0.008, 0.03), CER3, T)                                             # its raw lower edge
for i in range(3):
    bolt((-0.12 + i * 0.045, 1.398, 0.101), T, r=0.006)                                          # bolts never tightened
B((0, 1.36, 0.098), (0.016, 0.22, 0.02), CER3, T)                                                # sternum seam
# the open ribs on his right: four frame bars round the side, the light inside, cables through them
for i in range(4):
    y = 1.22 + i * 0.045
    B((-0.115, y, 0.045), (0.07, 0.014, 0.012), FR2, T, rot=(0, -20, 0), bev=0.004)              # rib, front
    B((-0.145, y, -0.005), (0.012, 0.014, 0.10), FR2, T, bev=0.004)                              # rib, side
E((-0.085, 1.29, -0.02), (0.08, 0.16, 0.08), CYAN, T, glow="core", k=0.9, seg=12, rings=8)        # the light inside
C((-0.085, 1.29, -0.02), 0.022, 0.18, "glow*1.5", T, glow="core", k=1.6, sides=10)               # its bright heart
for (pts, mat) in (([(-0.04, 1.17, 0.05), (-0.11, 1.25, 0.06), (-0.12, 1.36, 0.04)], "cable"),
                   ([(-0.05, 1.18, 0.03), (-0.13, 1.26, 0.035), (-0.10, 1.38, 0.05)], "wire_amber"),
                   ([(-0.03, 1.19, 0.06), (-0.09, 1.30, 0.075), (-0.06, 1.40, 0.06)], "wire_blue")):
    cable(pts, 0.006, mat, T)
B((-0.135, 1.21, 0.05), (0.03, 0.012, 0.02), "wire_red", T)                                       # a loose wire's end
# collar: two plates over the shoulders' frame, a ring where the neck goes in
for s in (1, -1):
    P((s * 0.095, 1.475, 0.0), (0.12, 0.035, 0.16), CER2, T, rot=(0, 0, s * -10), top=(0.85, 0.9), bev=0.012)
R((0, 1.50, 0.0), 0.055, 0.012, SV, T, rot=(90, 0, 0), sides=16, tsides=6)
# back plate, split by the spine channel
for s in (1, -1):
    P((s * 0.07, 1.33, -0.088), (0.11, 0.26, 0.04), CER, T, top=(1.15, 1.0), bev=0.02, seg=2)
B((0, 1.33, -0.098), (0.034, 0.30, 0.03), FR, T)                                                 # the channel
for i in range(7):                                                                               # the power spine
    E((0, 1.20 + i * 0.044, -0.112), (0.026, 0.03, 0.014), CYAN, T, glow="core", k=1.2, phase=-i * 0.6)
for i in range(4):
    bolt((0.12, 1.24 + i * 0.06, -0.106), T)
# shoulder caps: rounded ceramic over a bare servo
for s, arm in ((1, "armL"), (-1, "armR")):
    C((s * 0.20, 1.45, 0.0), 0.045, 0.06, SV, arm, rot=(0, 0, 90), sides=14)
    E((s * 0.225, 1.47, 0.0), (0.11, 0.08, 0.13), CER, arm, seg=16, rings=8)
    B((s * 0.255, 1.455, 0.0), (0.012, 0.035, 0.10), CER3, arm, bev=0.004)

# ============================================================================================
# ARMS: a frame strut and an outer ceramic plate on the upper arm; the elbow bare; the forearm plated on top with
# pistons and cables underneath; the hands: a ceramic back plate, bare grey knuckles, plated fingers
# ============================================================================================
for s, side in ((1, "L"), (-1, "R")):
    arm, elb, hand, fing, thumb = "arm" + side, "elbow" + side, "hand" + side, "fingers" + side, "thumb" + side
    x = s * 0.225
    C((x, 1.31, 0.0), 0.026, 0.26, FR, arm, sides=8)
    P((x + s * 0.012, 1.32, 0.0), (0.075, 0.20, 0.09), CER, arm, top=(0.9, 0.9), bev=0.02, seg=2)
    piston((x - s * 0.03, 1.40, 0.02), (x - s * 0.03, 1.21, 0.015), arm, r=0.009)
    # elbow
    C((s * 0.235, 1.17, -0.01), 0.038, 0.085, SV, elb, rot=(0, 0, 90), sides=12)
    E((s * 0.235, 1.165, -0.045), (0.05, 0.05, 0.035), CER2, elb, seg=10, rings=6)               # elbow cap
    # forearm
    x = s * 0.238
    C((x, 1.03, 0.0), 0.024, 0.24, FR, elb, sides=8)
    P((x, 1.035, 0.012), (0.075, 0.22, 0.06), CER, elb, rot=(0, 0, 0), top=(0.82, 0.85), bev=0.02, seg=2)   # top plate
    piston((x - s * 0.015, 1.14, -0.03), (x - s * 0.012, 0.95, -0.028), elb, r=0.01)
    cable([(x + s * 0.02, 1.13, -0.025), (x + s * 0.024, 1.04, -0.035), (x + s * 0.02, 0.94, -0.02)], 0.006, "cable", elb)
    cable([(x + s * 0.005, 1.12, -0.035), (x + s * 0.0, 1.03, -0.042), (x + s * 0.004, 0.94, -0.03)], 0.005, "wire_amber", elb)
    R((x, 0.925, 0.0), 0.034, 0.008, SV, elb, rot=(90, 0, 0), sides=12, tsides=5)                # wrist ring
    # hand: palm frame, ceramic back plate (faces outward, +X for the left), a bare knuckle bar
    C((x, 0.90, 0.0), 0.022, 0.035, SV, hand, rot=(90, 0, 0), sides=10)                         # wrist servo
    B((x, 0.862, 0.0), (0.03, 0.07, 0.085), FR, hand, bev=0.008)                                 # palm
    P((x + s * 0.017, 0.865, 0.0), (0.012, 0.07, 0.08), CER, hand, top=(1.0, 0.92), bev=0.004)  # back plate
    C((x, 0.825, 0.0), 0.014, 0.088, SV, hand, rot=(90, 0, 0), sides=8)                         # knuckle bar
    # fingers: four, each a plated bone over a bare joint, curled a little
    for i in range(4):
        z = 0.032 - i * 0.021
        ln = (0.034, 0.038, 0.036, 0.03)[i]
        B((x, 0.80, z), (0.018, ln, 0.016), CER, fing, bev=0.004)                               # first bone
        E((x, 0.80 - ln / 2 - 0.004, z), (0.016, 0.012, 0.016), SV, fing, seg=8, rings=4)        # joint
        B((x - s * 0.003, 0.80 - ln - 0.012, z), (0.016, ln * 0.85, 0.015), CER2, fing, rot=(0, 0, s * 12), bev=0.004)   # tip
    # thumb
    B((x - s * 0.005, 0.87, 0.045), (0.016, 0.035, 0.016), CER, thumb, rot=(30, 0, 0), bev=0.004)
    E((x - s * 0.005, 0.855, 0.056), (0.014, 0.012, 0.014), SV, thumb, seg=8, rings=4)
    B((x - s * 0.006, 0.84, 0.066), (0.015, 0.03, 0.014), CER2, thumb, rot=(40, 0, 0), bev=0.004)

# ============================================================================================
# NECK: a servo column, a bundle of cables round it, a collar ring
# ============================================================================================
H = "head"
C((0, 1.535, 0.0), 0.024, 0.09, SV, H, sides=10)
for i, mat in enumerate(("cable", "cable", "wire_red", "cable", "wire_blue", "cable")):
    a = math.radians(i * 60 + 15)
    cable([(math.cos(a) * 0.04, 1.49, math.sin(a) * 0.035), (math.cos(a) * 0.036, 1.54, math.sin(a) * 0.03),
           (math.cos(a) * 0.032, 1.59, math.sin(a) * 0.028)], 0.0065, mat, H)

# ============================================================================================
# HEAD: a rounded shell on a dark skull frame, a brow plate over two cyan eyes in a dark face band, no mouth;
# his left temple open: the frame, the back of his eye, a board and wiring
# ============================================================================================
HC = (0, 1.685, 0.005)
B((0, 1.68, 0.0), (0.19, 0.20, 0.20), FR, H, bev=0.05, seg=4)                                    # skull frame
# shell panels: crown, back, his right side, the face, the chin
P((0, 1.765, -0.005), (0.215, 0.05, 0.225), CER, H, top=(0.85, 0.82), bev=0.02, seg=3)           # crown
B((0, 1.68, -0.098), (0.20, 0.17, 0.035), CER, H, bev=0.03, seg=4)                               # back
B((-0.1, 1.675, 0.0), (0.03, 0.18, 0.20), CER, H, bev=0.03, seg=4)                               # his right side
E((-0.118, 1.675, -0.005), (0.012, 0.05, 0.05), SV, H, seg=12, rings=6)                          # ear disc
B((0, 1.632, 0.096), (0.19, 0.058, 0.035), CER, H, bev=0.022, seg=4)                             # lower face
P((0, 1.592, 0.06), (0.15, 0.03, 0.10), CER2, H, rot=(0, 0, 180), top=(0.8, 0.9), bev=0.01)      # chin
B((0, 1.69, 0.103), (0.188, 0.056, 0.02), "lens", H, bev=0.012)                                 # the face band
# his left side: only the jaw and the back corner fitted
B((0.1, 1.615, 0.01), (0.03, 0.07, 0.18), CER, H, bev=0.015)
B((0.1, 1.71, -0.065), (0.03, 0.10, 0.07), CER, H, bev=0.015)
B((0.102, 1.665, -0.02), (0.012, 0.012, 0.11), CER3, H)                                          # its raw edge
for i in range(3):
    bolt((0.108, 1.735, -0.08 + i * 0.03), H, r=0.005)
# what shows through: the eye's housing running back, a small board, wiring
C((0.045, 1.69, 0.04), 0.024, 0.09, SV, H, rot=(90, 0, 0), sides=12)                             # eye housing
R((0.045, 1.69, -0.006), 0.019, 0.005, CYAN, H, sides=14, tsides=4, glow="eye", k=0.7)          # its back ring, lit
B((0.096, 1.69, 0.01), (0.008, 0.06, 0.07), "board", H)
for j in range(3):
    B((0.1, 1.68 + j * 0.015, 0.01), (0.004, 0.006, 0.05), "brass*0.8", H)                       # traces
E((0.101, 1.705, -0.01), (0.006, 0.006, 0.006), CYAN, H, glow="eye", k=1.0)                       # a status light
for (pts, mat) in (([(0.08, 1.62, -0.03), (0.095, 1.67, -0.04), (0.08, 1.73, -0.02)], "cable"),
                   ([(0.075, 1.62, 0.0), (0.098, 1.66, 0.03), (0.075, 1.70, 0.05)], "wire_red"),
                   ([(0.07, 1.63, -0.05), (0.093, 1.69, -0.055), (0.06, 1.75, -0.04)], "wire_amber")):
    cable(pts, 0.005, mat, H)
# the brow plate: overhanging the eyes (the code dims the eyes under it: a frown)
P((0, 1.729, 0.10), (0.20, 0.03, 0.055), CER, H, rot=(12, 0, 0), top=(1.0, 0.9), bev=0.012, seg=2)
B((0, 1.712, 0.122), (0.19, 0.006, 0.01), CER3, H)
# the eyes: dark sockets, cyan lenses, a ring round each
for s in (1, -1):
    ex = s * 0.045
    ex = s * 0.046
    B((ex, 1.69, 0.111), (0.058, 0.042, 0.008), "lens*0.6", H, bev=0.008)
    E((ex, 1.69, 0.114), (0.046, 0.031, 0.01), CYAN, H, glow="eye", k=1.8, seg=18, rings=6)
    E((ex - 0.008, 1.696, 0.119), (0.009, 0.007, 0.003), "glow*2.0", H, glow="eye", k=2.6, seg=10, rings=4)   # a glint
    R((ex, 1.69, 0.113), 0.027, 0.0035, SV, H, sides=18, tsides=4)

report(NAME)
save_enemy(NAME)
