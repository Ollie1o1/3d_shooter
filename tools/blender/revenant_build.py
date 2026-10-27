# The REVENANT in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/revenant_build.py).
# A gaunt machine-knight: thin limbs in blued steel over a bone frame, a skull-helm with one burning slit, a cracked
# crest, a ragged mantle, and its soul burning white-blue in the crack down its chest ("core": it flares for a
# soulbolt). Long claws of bone, lit while it rakes ("blade").
#   hipL/R (+-0.105, 1.1, 0)  torso (0, 1.22, 0)  head (0, 1.92, 0)  armL/R (+-0.295, 1.855, 0): arms hang down -Y
#   head hitbox: x, z within +-0.21 of the head, y 1.86..2.30
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("revenant", joints=globals().get("SKELETON")); clear_parts()

STEEL, STEEL2, EDGE = "revenant*1.4", "revenant*2.0", "revenant*3.0"
DARK, BONE, BONE2 = "revenant*0.6", "bone*0.95", "bone*0.7"
CLOTH = "revenant*0.45"
SOUL = "glow*0.25"
RT = "revenant"

# ---- legs: a bone shin and thigh in thin steel plate, a spiked knee, clawed feet ----
for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.105
    C((x, 0.82, 0.0), 0.05, 0.5, BONE2, hip, sides=8)                                          # thigh bone
    P((x, 0.84, 0.035), (0.15, 0.44, 0.1), STEEL, hip, top=(1.15, 1.0), bev=0.02)              # cuisse
    E((x, 0.57, 0.03), (0.13, 0.11, 0.12), STEEL2, hip, seg=10, rings=5)                       # knee
    K((x, 0.57, 0.08), 0.025, 0.12, EDGE, hip, rot=(70, 0, 0), sides=5)
    C((x, 0.3, 0.0), 0.04, 0.5, BONE2, hip, sides=8)                                           # shin bone
    P((x, 0.32, 0.03), (0.12, 0.44, 0.09), STEEL, hip, top=(1.2, 1.1), bev=0.02)               # greave
    B((x, 0.32, 0.077), (0.02, 0.36, 0.015), EDGE, hip)
    B((x, 0.03, 0.04), (0.13, 0.06, 0.24), DARK, hip, bev=0.015)                               # foot
    for k in (-1, 0, 1):
        W((x + k * 0.04, 0.02, 0.19), (0.03, 0.035, 0.1), BONE, hip, taper=0.2)               # claws
    K((x, 0.04, -0.09), 0.02, 0.07, BONE, hip, rot=(-90, 0, 0), sides=5)                       # heel spur

# ---- hips: a pelvis of bone in a steel belt, a ragged loincloth ----
E((0, 1.15, 0.0), (0.3, 0.14, 0.2), BONE2, RT, seg=10, rings=5)
B((0, 1.2, 0.0), (0.36, 0.05, 0.24), STEEL2, RT, bev=0.012)
for z, sgn in ((0.11, 1), (-0.11, -1)):
    P((0, 0.95, z), (0.18, 0.42, 0.02), CLOTH, RT, rot=(sgn * -5, 0, 0), top=(1.2, 1.0))
    for k in (-1, 1):
        W((k * 0.05, 0.72, z), (0.07, 0.015, 0.1), CLOTH, RT, rot=(90, 0, 0), taper=0.1)

# ---- the torso: a ribcage in steel plate cracked open down the front on its soul, a spine, a mantle ----
T_ = "torso"
C((0, 1.42, 0.0), 0.06, 0.4, BONE2, T_, sides=8)                                               # spine column
for i in range(4):   # ribs either side of the crack
    y = 1.48 + 0.08 * i
    for s in (1, -1):
        P((s * 0.12, y, 0.08), (0.17, 0.035, 0.08), BONE, T_, rot=(0, s * 20, s * -12), top=(0.6, 0.8), bev=0.01)
for s in (1, -1):   # its plate: two halves that don't meet
    P((s * 0.14, 1.64, 0.02), (0.2, 0.44, 0.24), STEEL, T_, rot=(0, s * -6, 0), top=(1.15, 1.0), bev=0.03)
    B((s * 0.045, 1.64, 0.13), (0.015, 0.4, 0.03), EDGE, T_, rot=(0, 0, s * 4))                # the crack's edge
    P((s * 0.13, 1.33, 0.0), (0.16, 0.16, 0.2), STEEL2, T_, top=(1.2, 1.0), bev=0.02)          # its belly plates
E((0, 1.64, 0.1), (0.08, 0.28, 0.06), SOUL, T_, glow="core", k=1.0, seg=10, rings=6)           # the soul in the crack
E((0, 1.64, 0.09), (0.14, 0.36, 0.04), "glow*0.1", T_, glow="core", k=0.35, seg=10, rings=5)   # its glow on the bone
for k in range(5):   # its spine, knuckled down its back
    E((0, 1.36 + 0.1 * k, -0.13), (0.06, 0.05, 0.05), BONE, T_, seg=8, rings=4)
# the mantle: ragged cloth over its shoulders, falling behind
P((0, 1.55, -0.14), (0.5, 0.6, 0.03), CLOTH, T_, rot=(8, 0, 0), top=(0.85, 1.0))
for k in range(4):
    W((-0.18 + 0.12 * k, 1.21, -0.18), (0.11, 0.02, 0.14 + 0.03 * (k % 2)), CLOTH, T_, rot=(98, 0, 0), taper=0.1)
for s in (1, -1):   # pauldrons: layered, swept up into a point
    E((s * 0.27, 1.88, 0.0), (0.2, 0.12, 0.26), STEEL2, T_, seg=10, rings=6)
    P((s * 0.31, 1.84, 0.0), (0.16, 0.05, 0.26), STEEL, T_, rot=(0, 0, s * -30), bev=0.01)
    K((s * 0.3, 1.92, -0.04), 0.03, 0.16, EDGE, T_, rot=(-20, 0, s * -35), sides=5)
C((0, 1.9, 0.0), 0.05, 0.08, BONE2, T_, sides=8)                                               # neck

# ---- the head: a skull-helm with one burning slit, a cracked crest ----
H = "head"
E((0, 2.05, 0.0), (0.24, 0.27, 0.27), STEEL, H, seg=12, rings=8)                               # helm
P((0, 1.95, 0.05), (0.2, 0.12, 0.22), BONE, H, top=(0.8, 0.9), bev=0.015)                      # the jaw of its skull
for k in range(3):
    B((-0.04 + 0.04 * k, 1.94, 0.15), (0.012, 0.06, 0.01), DARK, H)                           # teeth gaps
B((0, 2.07, 0.13), (0.16, 0.03, 0.02), "glow*0.2", H, glow="eye", k=1.0)                       # the slit
B((0, 2.11, 0.125), (0.2, 0.02, 0.02), EDGE, H)                                                # brow
P((0, 2.2, -0.03), (0.04, 0.12, 0.22), STEEL2, H, top=(1.0, 0.6), bev=0.008)                   # crest
B((0.008, 2.21, 0.0), (0.006, 0.09, 0.006), "glow*0.2", H, rot=(0, 0, 20), glow="eye", k=0.25)  # its crack

# ---- arms: thin steel over bone, a spiked elbow, long bone claws that light as it rakes ----
for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.295
    C((x, 1.62, 0.0), 0.035, 0.42, BONE2, arm, sides=8)                                        # upper arm bone
    P((x + s * 0.01, 1.64, 0.0), (0.1, 0.36, 0.1), STEEL, arm, top=(1.1, 1.0), bev=0.015)
    E((x, 1.42, 0.0), (0.09, 0.08, 0.09), STEEL2, arm, seg=8, rings=5)                         # elbow
    K((x, 1.42, -0.04), 0.02, 0.1, EDGE, arm, rot=(-90, 0, 0), sides=5)
    C((x, 1.22, 0.0), 0.03, 0.38, BONE2, arm, sides=8)                                         # forearm bone
    P((x, 1.22, 0.01), (0.1, 0.34, 0.1), STEEL, arm, top=(1.2, 1.1), bev=0.015)                # vambrace
    B((x + s * 0.052, 1.22, 0.0), (0.012, 0.3, 0.05), EDGE, arm)
    B((x, 1.02, 0.03), (0.1, 0.08, 0.08), DARK, arm, bev=0.015)                                # hand
    for k in range(3):   # claws: long, curved, lit as it rakes
        cx = x - s * 0.03 + s * 0.03 * k
        tube([(cx, 0.99, 0.06), (cx, 0.88, 0.09), (cx, 0.78, 0.06)], [0.014, 0.011, 0.006], BONE, arm, sides=6, tip=0.04, glow="blade", k=0.6)

report("revenant")
save_enemy("revenant")
