# THE ABSOLUTE in detail (python3 tools/blender/live.py tools/blender/absolute_build.py), and MORNINGSTAR, his second
# form, from the same body (tools/blender/morningstar_build.py sets FORM and runs this).
#
#   THE ABSOLUTE  a 2.6 m fighter of white light: porcelain plate over grey, every muscle shaped, seams of light between
#                 the plates, a face with two crimson eyes, gauntlets of dark iron cut with blue runes, wraps trailing
#                 light from his wrists, a broken halo of gold turning behind his head, in phase 3 a blade of light.
#   MORNINGSTAR   the eclipse's: blood orange lit from within, his plate split by white-hot cracks, horns of light, solar
#                 flares off his shoulders and spine, flame round his waist, obsidian gauntlets with white runes, blank
#                 white eyes, and behind his head an eclipse: a black sun in a corona of rays.
#
# Every tell lights the striking limb (its seams: tell*, its glow: aura*, its gauntlet's runes: rune*; AbsoluteRig.h).
# Joints (his rest pose, arms hanging, facing +Z; right is -X):
#   hipL/R (+-0.15, 1.15, 0)  kneeL/R (y 0.57)  ankleL/R (y 0.02)   waist (0, 1.33, 0)  clothF/B (0, 1.30, +-0.14)
#   torso (0, 1.30, 0)  head (0, 2.25, 0)  crown (on the head)  halo (0, 2.49, -0.34)
#   armL/R (+-0.31, 2.06, 0)  elbowL/R (y 1.58)  ribbonL/R (y 1.22, z -0.06)  handL/R (y 1.14)
#   fingersL/R (y 1.04, z 0.01)  tipsL/R (y 0.98)  thumbL/R (x +-0.255, y 1.10, z 0.03)  thumbTipL/R (y 1.05)
#   weapon (-0.31, 1.07, -0.035): the blade of light (shown with the "blade" flag), out of the fist's thumb side
#   head hitbox: x within +-0.20, y 2.19..2.69, z -0.22..0.20 (the crown's fins and horns ride "crown": outside it)
# The first build starts from a skeleton (tools/enemy_export --joints absolute <file>, its path set as SKELETON).
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
FORM = globals().get("FORM", "absolute")
MS = FORM == "morningstar"
open_enemy(FORM, joints=globals().get("SKELETON")); clear_parts()
RT = FORM   # (the root: rides his hips)

if MS:
    PLATE, PLATE2, SKIN, SEAM, GAUNT, GOLD = "ember", "ember*0.8", "blood", "char", "obsidian", "plate"
else:
    PLATE, PLATE2, SKIN, SEAM, GAUNT, GOLD = "plate", "plate*0.88", "skin", "seam", "gauntlet", "gold"
LIGHT = "plate"   # (a part made of light: white, and lit)


def crack(pts, parent, w=0.012, z=None, k=1.0, glow="seam"):
    """MORNINGSTAR's: a jagged crack of white light through game-space points"""
    for a, b in zip(pts[:-1], pts[1:]):
        dx, dy, dz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
        ln = math.sqrt(dx * dx + dy * dy + dz * dz)
        c = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2)
        B(c, (w, ln + w, w), LIGHT, parent, rot=aimed(c, (dx, dy, dz)), glow=glow, k=k)


# ---- legs: thigh, its quadriceps plate, outer and inner plates, hamstring, seams; a hip cap; knee cap in a ring of
#      light; shin, shin plate, calf and its plate, the shin's seam; ankle wraps; a foot: sole, instep, two toes, heel ----
for s in (1, -1):
    L = s > 0
    hip, knee, ank = ("hipL", "kneeL", "ankleL") if L else ("hipR", "kneeR", "ankleR")
    tl, au = ("tellLegL", "auraLegL") if L else ("tellLegR", "auraLegR")
    x = s * 0.15
    E((x, 0.87, 0.01), (0.21, 0.66, 0.23), SKIN, hip, glow=au, seg=16, rings=10)                   # thigh
    E((x, 0.89, 0.075), (0.17, 0.50, 0.10), PLATE, hip, glow=au, seg=14, rings=10)                  # quadriceps
    P((x, 0.98, 0.115), (0.12, 0.22, 0.03), PLATE2, hip, top=(1.25, 1.0), bev=0.012, seg=2)          # its plate
    P((x + s * 0.088, 0.92, 0.0), (0.05, 0.40, 0.16), PLATE2, hip, top=(1.1, 1.2), bev=0.015)        # outer
    P((x - s * 0.072, 0.95, 0.02), (0.05, 0.30, 0.13), PLATE2, hip, top=(1.2, 1.0), bev=0.015)       # inner
    E((x, 0.86, -0.07), (0.17, 0.46, 0.12), SKIN, hip, seg=12, rings=8)                            # hamstring
    B((x + s * 0.108, 0.88, 0.03), (0.014, 0.40, 0.022), SEAM, hip, glow=tl)                       # seams
    B((x - s * 0.098, 0.93, 0.05), (0.014, 0.28, 0.022), SEAM, hip, glow=tl, k=0.6)
    B((x, 0.89, 0.123), (0.012, 0.38, 0.012), SEAM, hip, glow=tl, k=0.8)
    E((x + s * 0.05, 1.12, 0.0), (0.21, 0.15, 0.23), PLATE, hip, glow=au, seg=14, rings=8)          # hip cap
    E((x, 0.578, 0.085), (0.12, 0.12, 0.07), PLATE, knee, glow=au, seg=12, rings=8)                # knee cap
    R((x, 0.578, 0.093), 0.066, 0.008, SEAM, knee, sides=18, tsides=4, glow=tl, k=0.8)
    E((x, 0.57, 0.0), (0.13, 0.12, 0.14), SKIN, knee, seg=10, rings=6)
    C((x, 0.31, 0.0), 0.05, 0.50, SKIN, knee, sides=12, r2=0.07, glow=au)                           # shin
    P((x, 0.32, 0.07), (0.10, 0.44, 0.04), PLATE, knee, top=(1.35, 1.0), bev=0.015, seg=2)          # shin plate
    E((x, 0.38, -0.06), (0.14, 0.28, 0.12), SKIN, knee, seg=12, rings=8)                           # calf
    P((x, 0.40, -0.105), (0.10, 0.22, 0.035), PLATE2, knee, top=(1.2, 1.0), bev=0.01)
    B((x, 0.30, 0.093), (0.022, 0.40, 0.012), SEAM, knee, glow=tl)                                 # shin seam
    for y in (0.085, 0.125):
        R((x, y, 0.0), 0.068, 0.01, SEAM, knee, rot=(90, 0, 0), sides=16, tsides=4, glow="light", k=0.45)   # ankle wraps
    B((x, 0.015, 0.06), (0.12, 0.03, 0.26), SEAM, ank, bev=0.008)                                  # sole
    P((x, 0.06, 0.04), (0.11, 0.07, 0.19), PLATE, ank, rot=(-14, 0, 0), top=(0.9, 0.8), bev=0.02, seg=2)   # instep
    E((x - 0.03, 0.035, 0.175), (0.06, 0.05, 0.08), PLATE2, ank, seg=8, rings=5)                   # toes
    E((x + 0.032, 0.035, 0.165), (0.055, 0.045, 0.07), PLATE2, ank, seg=8, rings=5)
    E((x, 0.06, -0.05), (0.10, 0.10, 0.09), SKIN, ank, seg=10, rings=6)                            # heel
    if MS:   # white light through the cracks; flare spurs off the heels
        crack([(x + s * 0.06, 1.05, 0.11), (x + s * 0.02, 0.95, 0.125), (x + s * 0.05, 0.84, 0.125), (x, 0.72, 0.11)], hip)
        crack([(x - s * 0.04, 0.48, 0.095), (x, 0.36, 0.098), (x - s * 0.03, 0.22, 0.092)], knee, w=0.01)
        K((x, 0.07, -0.09), 0.025, 0.14, LIGHT, ank, rot=(-110, 0, 0), sides=6, glow="seam", k=0.6)

# ---- the waist: pelvis; a ring of light with eight plates on it (turning with his hips); wraps hanging front and
#      back with lit edges and a sigil; MORNINGSTAR's ring burns ----
P((0, 1.22, 0.0), (0.38, 0.20, 0.25), SKIN, RT, top=(1.15, 1.1), glow="soft")
for s in (1, -1):
    P((s * 0.17, 1.24, 0.03), (0.07, 0.16, 0.20), PLATE2, RT, rot=(0, 0, s * 12), bev=0.015)        # iliac plates
R((0, 1.33, 0.0), 0.185, 0.016, SEAM, "waist", rot=(90, 0, 0), sides=32, tsides=6, glow="seam", k=0.8)
for k in range(8):
    a = k * 45 + 22.5
    B((math.sin(math.radians(a)) * 0.19, 1.33, math.cos(math.radians(a)) * 0.19), (0.09, 0.045, 0.022), PLATE, "waist",
      rot=(0, a, 0), bev=0.008)
    if MS:   # flame round the waist
        K((math.sin(math.radians(a)) * 0.20, 1.31, math.cos(math.radians(a)) * 0.20), 0.03, 0.16 + 0.06 * (k % 2), LIGHT, "waist",
          rot=(180 + 25 * math.cos(math.radians(a)), 0, -25 * math.sin(math.radians(a))), sides=5, glow="soft", k=2.5)
for cl, z, ln in (("clothF", 0.15, 0.56), ("clothB", -0.15, 0.64)):
    d = 1 if z > 0 else -1
    P((0, 1.30 - ln / 2, z), (0.17, ln, 0.012), SEAM, cl, top=(1.4, 1.0), glow="light", k=0.18)     # the wrap
    for s in (1, -1):
        B((s * 0.083 * (1 - 0.15), 1.30 - ln / 2, z + d * 0.004), (0.01, ln, 0.012), SEAM, cl, rot=(0, 0, s * 2.5),
          glow="light", k=0.45)                                                                    # its lit edges
        P((s * 0.11, 1.30 - ln * 0.38, z - d * 0.008), (0.06, ln * 0.76, 0.01), SEAM, cl, rot=(0, 0, s * 4), top=(1.3, 1.0),
          glow="light", k=0.12)                                                                    # side strips
    R((0, 1.08 if d > 0 else 1.02, z + d * 0.008), 0.035, 0.006, LIGHT, cl, sides=14, tsides=4, glow="seam", k=0.7)   # sigil
    B((0, 1.30 - ln - 0.005, z), (0.24, 0.012, 0.014), SEAM, cl, glow="light", k=0.5)              # hem

# ---- the torso: abdomen and its six plates, the linea of light, obliques, ribs, chest and pectoral plates, the light
#      in his chest, collarbones, traps, shoulder blades, lats, spine, veins; the neck and its seam ----
T = "torso"
E((0, 1.52, 0.0), (0.30, 0.44, 0.23), SKIN, T, glow="soft", seg=16, rings=10)                     # abdomen
for r in range(3):
    for s in (1, -1):
        E((s * 0.056, 1.40 + 0.115 * r, 0.10), (0.095, 0.10, 0.06), PLATE, T, seg=10, rings=6, glow="soft")
B((0, 1.52, 0.125), (0.012, 0.36, 0.012), SEAM, T, glow="seam")                                    # linea
for s in (1, -1):
    P((s * 0.165, 1.50, 0.03), (0.06, 0.32, 0.18), SKIN, T, rot=(0, 0, s * -8), top=(1.3, 1.0))      # obliques
    for k in range(3):
        P((s * 0.20, 1.72 + 0.075 * k, 0.05), (0.07, 0.033, 0.20), PLATE2, T, rot=(0, s * 10, s * 22), bev=0.008)   # ribs
    E((s * 0.12, 1.925, 0.115), (0.25, 0.21, 0.11), PLATE, T, rot=(-8, s * 12, 0), seg=16, rings=8, glow="soft")   # pectorals
    P((s * 0.13, 1.99, 0.155), (0.17, 0.05, 0.02), PLATE2, T, rot=(-8, s * 12, s * 8), bev=0.008)    # their plates
    B((s * 0.12, 1.815, 0.16), (0.20, 0.012, 0.012), SEAM, T, rot=(0, s * 12, s * -6), glow="seam", k=0.5)
    P((s * 0.13, 2.08, 0.10), (0.24, 0.035, 0.04), PLATE, T, rot=(0, 0, s * 9), bev=0.008)          # collarbones
    E((s * 0.14, 2.10, -0.04), (0.18, 0.12, 0.18), SKIN, T, seg=12, rings=6)                       # traps
    P((s * 0.13, 1.90, -0.152), (0.16, 0.20, 0.03), PLATE, T, rot=(8, s * -15, 0), top=(1.15, 1.0), bev=0.01)   # shoulder blades
    E((s * 0.17, 1.72, -0.08), (0.12, 0.30, 0.14), SKIN, T, seg=12, rings=8)                       # lats
    B((s * 0.10, 1.62, 0.15), (0.018, 0.30, 0.01), SEAM, T, rot=(0, 0, s * 28.6), glow="seam")      # veins
E((0, 1.90, -0.01), (0.54, 0.34, 0.30), SKIN, T, glow="soft", seg=18, rings=10)                   # chest
B((0, 1.80, 0.162), (0.022, 0.48, 0.014), SEAM, T, glow="seam", k=1.3)                             # sternum seam
B((0, 1.92, 0.172), (0.07, 0.07, 0.02), LIGHT, T, rot=(0, 0, 45), glow="core")                     # the light in his chest
R((0, 1.92, 0.168), 0.07, 0.008, SEAM, T, sides=18, tsides=4, glow="seam", k=0.8)
for k in range(6):
    E((0, 1.38 + 0.13 * k, -0.135), (0.05, 0.06, 0.04), PLATE, T, seg=8, rings=4, glow="seam", k=0.4)   # spine
C((0, 2.18, 0.0), 0.066, 0.18, SKIN, T, sides=12)                                                 # neck
B((0, 2.17, 0.064), (0.03, 0.10, 0.01), SEAM, T, glow="seam")
for s in (1, -1):
    P((s * 0.04, 2.17, 0.03), (0.03, 0.16, 0.04), SKIN, T, rot=(-12, 0, s * -18))                   # its tendons
if MS:
    crack([(0.20, 2.02, 0.12), (0.12, 1.96, 0.16), (0.15, 1.86, 0.16), (0.06, 1.80, 0.17)], T)
    crack([(-0.22, 1.98, 0.10), (-0.14, 1.90, 0.16), (-0.17, 1.82, 0.15)], T)
    crack([(0.14, 1.62, 0.13), (0.09, 1.52, 0.125), (0.12, 1.42, 0.12)], T, w=0.01)
    crack([(-0.10, 1.66, 0.13), (-0.12, 1.55, 0.125)], T, w=0.01)
    for k in range(7):   # flares off his spine
        K((0, 1.45 + 0.11 * k, -0.15), 0.03 + 0.004 * k, 0.12 + 0.03 * (k % 3), LIGHT, T, rot=(-62 + 3 * k, 0, 0), sides=5,
          glow="soft", k=2.2)

# ---- the head: skull, a smooth face, jaw, brow, eyes (and their glow), the line of gold down his brow, cheekbones,
#      the crown ridge, the back of the skull ----
H = "head"
E((0, 2.45, -0.02), (0.24, 0.30, 0.27), PLATE, H, glow="soft", seg=20, rings=12)                  # skull
E((0, 2.42, 0.06), (0.20, 0.26, 0.17), PLATE, H, seg=18, rings=10)                                # face
P((0, 2.33, 0.04), (0.18, 0.10, 0.18), SKIN, H, top=(1.2, 1.0), bev=0.03, seg=2)                   # jaw
P((0, 2.51, 0.122), (0.22, 0.035, 0.045), PLATE2, H, bev=0.01)                                    # brow
es = 1.35 if MS else 1.0
for s in (1, -1):
    B((s * 0.055, 2.46, 0.142), (0.07 * es, 0.022 * es * 1.3, 0.012), SEAM, H, rot=(0, 0, s * 10.3), glow="eye")   # eyes
    B((s * 0.055, 2.46, 0.136), (0.11 * es, 0.05 * es, 0.006), SEAM, H, rot=(0, 0, s * 10.3), glow="eye", k=0.35)
    E((s * 0.085, 2.40, 0.118), (0.05, 0.035, 0.04), PLATE2, H, seg=8, rings=5)                    # cheekbones
B((0, 2.43, 0.148), (0.012, 0.11, 0.01), GOLD, H, glow="halo", k=0.35)                             # the gold line
P((0, 2.60, -0.02), (0.05, 0.04, 0.26), PLATE, H, glow="soft", bev=0.01)                           # crown ridge
E((0, 2.43, -0.13), (0.20, 0.22, 0.05), SKIN, H, seg=12, rings=6)
CR = "crown"
if MS:   # horns of light, and flares crowning him
    for s in (1, -1):
        tube([(s * 0.10, 2.58, 0.0), (s * 0.20, 2.68, -0.04), (s * 0.26, 2.82, -0.12), (s * 0.24, 2.96, -0.24)],
             [0.04, 0.034, 0.026, 0.016], SEAM, CR, sides=8, glow="seam", k=0.25, tip=0.14)
        for j, (cx, cy, cz) in enumerate(((s * 0.18, 2.66, -0.03), (s * 0.25, 2.78, -0.09))):
            R((cx, cy, cz), 0.037 - 0.006 * j, 0.007, LIGHT, CR, rot=(55, s * -35, 0), sides=12, tsides=4, glow="seam")
    for j in range(5):
        a = -40 + j * 20
        K((math.sin(math.radians(a)) * 0.07, 2.62, -0.05), 0.022, 0.16 - abs(j - 2) * 0.03, LIGHT, CR,
          rot=(-35, 0, -a * 0.8), sides=5, glow="seam", k=0.8)
else:    # three swept fins of light
    for j, (cx, a, ln) in enumerate(((0.0, 0, 0.24), (0.07, 14, 0.18), (-0.07, -14, 0.18))):
        P((cx, 2.66, -0.10), (0.025, ln, 0.06), PLATE, CR, rot=(-60, 0, -a), top=(0.15, 0.4), glow="seam", k=0.25)

# ---- arms: deltoid and its two plates, bicep, tricep, seams; elbow, forearm and its plate, three wraps of light and
#      ribbons trailing from the wrist; the gauntlet: shell, flared cuff, rune bands at the wrist and knuckles, a rune
#      per knuckle, the knuckles; four jointed fingers and a jointed thumb ----
for s in (1, -1):
    L = s > 0
    J = lambda l, r: l if L else r   # noqa: E731
    arm, elb, rib, hand = J("armL", "armR"), J("elbowL", "elbowR"), J("ribbonL", "ribbonR"), J("handL", "handR")
    fing, tips, th, tht = J("fingersL", "fingersR"), J("tipsL", "tipsR"), J("thumbL", "thumbR"), J("thumbTipL", "thumbTipR")
    tl, au, ru = J("tellL", "tellR"), J("auraL", "auraR"), J("runeL", "runeR")
    a = s * 0.31
    E((s * 0.33, 2.05, 0.0), (0.20, 0.18, 0.20), PLATE, arm, glow=au, seg=14, rings=8)              # deltoid
    P((s * 0.37, 2.07, 0.0), (0.12, 0.10, 0.22), PLATE2, arm, rot=(0, 0, s * -30), bev=0.02, seg=2)
    P((s * 0.385, 1.98, 0.0), (0.10, 0.09, 0.20), PLATE, arm, rot=(0, 0, s * -18), bev=0.02, seg=2)
    E((a, 1.81, 0.03), (0.12, 0.36, 0.12), SKIN, arm, glow=au, seg=12, rings=8)                    # bicep
    E((a, 1.83, -0.04), (0.11, 0.32, 0.10), PLATE, arm, seg=12, rings=8)                           # tricep
    B((a + s * 0.062, 1.81, 0.0), (0.014, 0.30, 0.02), SEAM, arm, glow=tl)
    B((a, 1.82, 0.088), (0.012, 0.24, 0.012), SEAM, arm, glow=tl, k=0.6)
    E((a, 1.58, -0.04), (0.10, 0.08, 0.07), PLATE, elb, seg=10, rings=6)                           # elbow
    E((a, 1.58, 0.0), (0.11, 0.10, 0.11), SKIN, elb, seg=10, rings=6)
    C((a, 1.37, 0.0), 0.048, 0.40, SKIN, elb, sides=12, r2=0.062, glow=au)                         # forearm
    E((a + s * 0.01, 1.47, 0.01), (0.10, 0.20, 0.10), SKIN, elb, seg=10, rings=6, glow=au)          # its muscle
    P((a + s * 0.03, 1.38, 0.03), (0.07, 0.30, 0.06), PLATE, elb, top=(1.2, 1.1), bev=0.015, seg=2, glow=au)
    B((a - s * 0.045, 1.38, 0.02), (0.012, 0.26, 0.014), SEAM, elb, glow=tl, k=0.7)
    for k in range(3):
        R((a, 1.29 - 0.05 * k, 0.0), 0.062, 0.012, SEAM, elb, rot=(90, 0, 0), sides=16, tsides=4, glow=tl, k=0.55)   # wraps
    for k, dx in enumerate((-0.03, 0.0, 0.03)):
        ln = 0.30 + 0.08 * (k == 1)
        P((a + dx, 1.22 - ln / 2, -0.06), (0.026, ln, 0.008), SEAM, rib, rot=(0, 0, 180 + dx * 120), top=(0.3, 1.0),
          glow="light", k=0.5)                                                                     # ribbons
    B((a, 1.09, 0.0), (0.10, 0.10, 0.05), SKIN, hand, glow=au)                                     # palm
    P((a, 1.08, 0.0), (0.125, 0.12, 0.075), GAUNT, hand, bev=0.012, glow=ru, k=0.08)                # gauntlet
    P((a, 1.175, 0.0), (0.125, 0.06, 0.082), GAUNT, hand, top=(1.18, 1.18), bev=0.01, glow=ru, k=0.08)   # cuff
    B((a, 1.16, 0.0), (0.135, 0.022, 0.085), SEAM, hand, glow=ru)                                  # wrist band
    B((a, 1.025, 0.041), (0.115, 0.018, 0.012), SEAM, hand, glow=ru)                               # across the knuckles
    B((a, 1.10, 0.04), (0.02, 0.06, 0.006), SEAM, hand, glow=ru, k=0.9)                             # a rune on its back
    B((a, 1.10, 0.04), (0.05, 0.012, 0.006), SEAM, hand, glow=ru, k=0.9)
    for f in range(4):
        fx = a - 0.035 + 0.023 * f
        B((fx, 1.05, 0.043), (0.008, 0.03, 0.008), SEAM, hand, glow=ru, k=0.8)                     # knuckle runes
        E((fx, 1.03, 0.025), (0.022, 0.02, 0.03), GAUNT, hand, seg=6, rings=4)                    # knuckles
        P((fx, 1.01, 0.01), (0.02, 0.06, 0.022), SKIN, fing, top=(0.9, 0.9), bev=0.005, glow=au)   # fingers
        P((fx, 0.955, 0.01), (0.018, 0.05, 0.02), PLATE, tips, top=(0.7, 0.7), bev=0.004, glow=au, k=1.5)
    tx = a - s * 0.055
    P((tx, 1.075, 0.03), (0.022, 0.05, 0.022), SKIN, th, bev=0.005, glow=au)                       # thumb
    P((tx, 1.03, 0.03), (0.02, 0.04, 0.02), PLATE, tht, top=(0.7, 0.7), bev=0.004, glow=au, k=1.5)
    if MS:   # flares off his shoulders, cracks down his arms, a heavier gauntlet
        for j, (dz, ln) in enumerate(((-0.07, 0.20), (0.0, 0.28), (0.07, 0.20))):
            K((s * 0.38, 2.12, dz), 0.04, ln, LIGHT, arm, rot=(dz * 200, 0, s * -40), sides=6, glow="soft", k=2.2)
        crack([(a + s * 0.05, 1.95, 0.05), (a + s * 0.03, 1.85, 0.065), (a + s * 0.055, 1.74, 0.05)], arm, w=0.01, glow=tl)
        crack([(a + s * 0.045, 1.50, 0.06), (a + s * 0.02, 1.40, 0.065), (a + s * 0.04, 1.30, 0.06)], elb, w=0.01, glow=tl)
        for k in range(3):
            K((a + s * 0.065, 1.30 + 0.07 * k, 0.0), 0.016, 0.07, GAUNT, elb, rot=(0, 0, s * -80), sides=4)   # forearm spurs

# ---- the finer work: finger knuckles and nails, runes down the gauntlets' sides, a gold armlet, more seams on the
#      forearms; a third toe, ankle bones, knee side plates, more seams and lights on the legs; serratus, back ribs, a ring
#      of lights round the light in his chest, gems on the waist ring; temples, ear plates, the bridge of the nose, lips,
#      a chin; a crest of fins down his skull ----
NAIL, NAILG = (LIGHT, "seam") if MS else (GOLD, "halo")
for s in (1, -1):
    L = s > 0
    J = lambda l, r: l if L else r   # noqa: E731
    arm, elb, hand, fing, tips = J("armL", "armR"), J("elbowL", "elbowR"), J("handL", "handR"), J("fingersL", "fingersR"), J("tipsL", "tipsR")
    hip, knee, ank = J("hipL", "hipR"), J("kneeL", "kneeR"), J("ankleL", "ankleR")
    tl, au, ru, tlg, aug = J("tellL", "tellR"), J("auraL", "auraR"), J("runeL", "runeR"), J("tellLegL", "tellLegR"), J("auraLegL", "auraLegR")
    a, x = s * 0.31, s * 0.15
    for f in range(4):
        fx = a - 0.035 + 0.023 * f
        E((fx, 1.036, 0.012), (0.024, 0.022, 0.026), SKIN, fing, seg=6, rings=4, glow=au)          # knuckle
        B((fx, 0.984, 0.012), (0.02, 0.012, 0.024), PLATE2, fing, glow=au)                          # its joint
        B((fx, 0.948, 0.021), (0.013, 0.024, 0.004), NAIL, tips, glow=NAILG, k=0.18)                # nail
    for d in (1, -1):
        B((a + d * 0.064, 1.08, 0.0), (0.004, 0.07, 0.012), SEAM, hand, glow=ru, k=0.8)             # side runes
        B((a + d * 0.064, 1.09, 0.0), (0.004, 0.012, 0.03), SEAM, hand, glow=ru, k=0.8)
    R((a, 1.70, 0.0), 0.064, 0.011, GOLD, arm, rot=(90, 0, 0), sides=18, tsides=4, glow=NAILG, k=0.2)   # armlet
    for k in range(4):
        an = math.radians(k * 90 + 45)
        E((a + math.sin(an) * 0.07, 1.70, math.cos(an) * 0.07), (0.018, 0.022, 0.018), LIGHT, arm, seg=6, rings=4, glow=tl, k=0.6)
    B((a + s * 0.05, 1.40, -0.02), (0.012, 0.22, 0.012), SEAM, elb, glow=tl, k=0.5)
    B((a, 1.43, -0.055), (0.012, 0.18, 0.012), SEAM, elb, glow=tl, k=0.5)
    E((x, 0.03, 0.18), (0.04, 0.035, 0.06), PLATE2, ank, seg=6, rings=4)                         # third toe
    for d in (1, -1):
        E((x + d * 0.055, 0.08, 0.0), (0.035, 0.05, 0.05), PLATE, ank, seg=6, rings=4)              # ankle bones
        P((x + d * 0.065, 0.58, 0.03), (0.02, 0.10, 0.08), PLATE2, knee, top=(0.6, 0.8), bev=0.005)  # knee side plates
    B((x + s * 0.07, 0.80, 0.09), (0.01, 0.22, 0.01), SEAM, hip, glow=tlg, k=0.5)
    B((x - s * 0.04, 0.78, 0.105), (0.01, 0.18, 0.01), SEAM, hip, glow=tlg, k=0.5)
    B((x, 0.38, -0.125), (0.012, 0.18, 0.01), SEAM, knee, glow=tlg, k=0.6)
    for k in range(3):
        E((x, 0.46 - 0.12 * k, 0.096), (0.018, 0.018, 0.012), LIGHT, knee, seg=6, rings=4, glow=tlg, k=0.7)   # shin lights
    for k in range(3):
        E((s * 0.235, 1.77 + 0.07 * k, 0.07), (0.04, 0.05, 0.07), PLATE, T, seg=6, rings=4)        # serratus
        P((s * 0.12, 1.60 + 0.09 * k, -0.125), (0.14, 0.025, 0.03), PLATE2, T, rot=(0, 0, s * -15), bev=0.006)   # back ribs
    P((s * 0.11, 2.47, 0.02), (0.04, 0.10, 0.12), PLATE2, H, bev=0.01)                             # temples
    P((s * 0.123, 2.41, -0.04), (0.02, 0.10, 0.07), PLATE, H, top=(1.0, 0.6), bev=0.005)           # ear plates
for k in range(8):
    an = math.radians(k * 45)
    E((math.cos(an) * 0.10, 1.92 + math.sin(an) * 0.10, 0.168), (0.014, 0.014, 0.01), LIGHT, T, seg=6, rings=4, glow="seam", k=0.8)
    B((math.sin(math.radians(k * 45 + 22.5)) * 0.19, 1.33, math.cos(math.radians(k * 45 + 22.5)) * 0.19 + 0.0), (0.02, 0.02, 0.03),
      LIGHT, "waist", rot=(0, k * 45 + 22.5, 0), glow="seam", k=0.7)                               # waist gems
P((0, 2.445, 0.148), (0.025, 0.06, 0.03), PLATE, H, top=(0.6, 0.6), bev=0.005)                     # nose
B((0, 2.355, 0.128), (0.06, 0.006, 0.006), SEAM, H, glow="seam", k=0.3)                            # lips
E((0, 2.305, 0.095), (0.07, 0.05, 0.05), PLATE, H, seg=8, rings=4)                                 # chin
if not MS:
    for i in range(5):   # the crest
        P((0, 2.63 - 0.025 * i, -0.15 - 0.055 * i), (0.018, 0.13 - 0.018 * i, 0.04), PLATE, CR, rot=(-50 - 6 * i, 0, 0),
          top=(0.2, 0.4), glow="seam", k=0.2)

# ---- the halo: thirteen of sixteen gold shards round him (three broken out, drifting), each with a point outward;
#      an inner ring of light; a fine circle between. MORNINGSTAR's: an eclipse, a black sun in a corona ----
HA, hc = "halo", (0, 2.49, -0.34)
if MS:
    E((0, hc[1], hc[2] - 0.02), (0.84, 0.84, 0.03), "obsidian*0.2", HA, seg=32, rings=4)          # the black sun
    R(hc, 0.43, 0.022, LIGHT, HA, sides=48, tsides=6, glow="halo", k=1.2)                          # its ring of fire
    R(hc, 0.47, 0.008, LIGHT, HA, sides=48, tsides=4, glow="halo", k=0.6)
    for i in range(24):   # the corona
        a = math.radians(i * 15 + 90)
        ln = (0.42, 0.20, 0.30, 0.20)[i % 4] + 0.04 * math.sin(i * 2.7)
        r0 = 0.46
        P((math.cos(a) * (r0 + ln / 2), hc[1] + math.sin(a) * (r0 + ln / 2), hc[2] + 0.005), (0.05 if i % 4 == 0 else 0.035, ln, 0.015),
          LIGHT, HA, rot=(0, 0, i * 15), top=(0.08, 1.0), glow="halo", k=0.55 if i % 2 else 0.8, phase=i * 0.8)
    for i in range(6):    # prominences: loops of fire off its rim
        a = math.radians(i * 60 + 30)
        R((math.cos(a) * 0.50, hc[1] + math.sin(a) * 0.50, hc[2]), 0.07, 0.01, LIGHT, HA, rot=(0, 0, i * 60 - 60), sides=14,
          tsides=4, arc=180, glow="soft", k=2.5)
else:
    for k in range(16):
        if k in (3, 4, 11):
            continue
        a = k * 22.5
        jit = 0.03 * math.sin(k * 7.1)
        r = 0.5 + jit
        c = (-math.sin(math.radians(a)) * r, hc[1] + math.cos(math.radians(a)) * r, hc[2])
        P(c, (0.17, 0.045, 0.03), GOLD, HA, rot=(0, 0, a), top=(1.0, 1.0), bev=0.008, glow="halo")    # a shard
        K((-math.sin(math.radians(a)) * (r + 0.02), hc[1] + math.cos(math.radians(a)) * (r + 0.02), hc[2]), 0.018,
          0.10 if k % 2 else 0.16, GOLD, HA, rot=(0, 0, a), sides=4, glow="halo", k=0.7, squash=0.5)   # its point
    for j, (k, out, tw) in enumerate(((3.4, 0.66, 25), (4.2, 0.72, -40), (11.3, 0.62, 60))):   # broken out, drifting
        a = k * 22.5
        c = (-math.sin(math.radians(a)) * out, hc[1] + math.cos(math.radians(a)) * out, hc[2] - 0.04 * j)
        P(c, (0.12 - 0.02 * j, 0.04, 0.025), GOLD, HA, rot=(tw * 0.5, tw * 0.3, a + tw), bev=0.006, glow="halo", k=0.5)
    for k in range(8):
        a = k * 45 + 22.4
        c = (-math.sin(math.radians(a)) * 0.34, hc[1] + math.cos(math.radians(a)) * 0.34, hc[2])
        B(c, (0.16, 0.02, 0.02), LIGHT, HA, rot=(0, 0, a), glow="seam", k=0.9)                     # the inner ring
    R(hc, 0.42, 0.005, GOLD, HA, sides=48, tsides=4, glow="halo", k=0.3)
    for k in range(16):   # a bead inside each shard, a mote outside it
        a = math.radians(k * 22.5)
        if k not in (3, 4, 11):
            E((-math.sin(a) * 0.45, hc[1] + math.cos(a) * 0.45, hc[2]), (0.03, 0.03, 0.03), GOLD, HA, seg=6, rings=4, glow="halo", k=0.8)
        b = a + math.radians(11.25)
        E((-math.sin(b) * 0.64, hc[1] + math.cos(b) * 0.64, hc[2] - 0.02), (0.018, 0.018, 0.018), LIGHT, HA, seg=6, rings=4,
          glow="halo", k=0.5, phase=k * 1.3)
    for k in range(8):
        a = math.radians(k * 45)
        E((-math.sin(a) * 0.34, hc[1] + math.cos(a) * 0.34, hc[2]), (0.026, 0.026, 0.026), LIGHT, HA, seg=6, rings=4, glow="seam")

# ---- the blade of light (phase 3: the "blade" flag): a grip in the fist, a pommel, a gold crescent guard, the blade in
#      three lengths of light, its edge, a point ----
W0x, W0y, W0z, SW = -0.31, 1.07, -0.035, "weapon"   # (the grip, inside the fist)
show(C((W0x, W0y, W0z), 0.026, 0.20, SEAM, SW, sides=8, glow="light", k=0.5), "blade")
show(E((W0x, W0y + 0.11, W0z), (0.05, 0.05, 0.05), GOLD, SW, glow="halo", k=0.6), "blade")
show(R((W0x, W0y - 0.05, W0z), 0.15, 0.014, GOLD, SW, rot=(0, 0, 180), sides=20, tsides=4, arc=180, glow="halo", k=0.8), "blade")   # guard
show(P((W0x, W0y - 0.10, W0z), (0.32, 0.04, 0.07), PLATE, SW, top=(0.8, 0.8), bev=0.008, glow="halo", k=0.5), "blade")
for k in range(3):
    show(P((W0x, W0y - 0.35 - 0.45 * k, W0z), (0.09 - 0.02 * k, 0.44, 0.025), LIGHT, SW, rot=(0, 0, 180), top=(0.8, 1.0),
           glow="sword", k=0.9 + 0.1 * k), "blade")
show(B((W0x, W0y - 0.75, W0z + 0.015), (0.02, 1.3, 0.01), LIGHT, SW, glow="sword", k=1.4), "blade")
show(K((W0x, W0y - 1.47, W0z), 0.025, 0.14, LIGHT, SW, rot=(180, 0, 0), sides=4, glow="sword", k=1.2, squash=0.4), "blade")

report(FORM)
save_enemy(FORM)
