# The ANCHOR in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/anchor_build.py).
# A gravity-well heavy, bent under its own weight: slate plate over piston legs, slab pauldrons, great fists, a low
# helm with a red visor, and on its back the anchor itself: a ring of ten segments round a core ("pulse" the ring,
# "core" its heart, flaring as it fires a volley).
#   hipL/R (+-0.25, 0.95, 0)  torso (0, 1.15, 0)  head (0, 2.2, 0)  armL/R (+-0.825, 2.0, 0): arms hang down -Y
#   head hitbox: x, z within +-0.27 of the head, y 2.13..2.69   (the game hunches it forward: RX 0.18 at the feet)
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("anchor", joints=globals().get("SKELETON")); clear_parts()

PLATE, PLATE2, EDGE = "anchor*1.35", "anchor*1.9", "anchor*2.7"
DARK, STEEL = "anchor*0.6", "gunmetal*1.5"
RED = "glow*0.3"
RT = "anchor"

# ---- legs: piston thighs, a thick knee, a stub greave, broad feet ----
for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.25
    C((x, 0.72, 0.0), 0.14, 0.5, DARK, hip, sides=12)                                           # thigh
    P((x, 0.74, 0.05), (0.38, 0.42, 0.3), PLATE, hip, top=(1.12, 1.05), bev=0.04, seg=2)        # cuisse
    for k in (-1, 1):
        C((x + k * 0.12, 0.72, -0.12), 0.035, 0.42, STEEL, hip, sides=8)                        # pistons
    E((x, 0.47, 0.06), (0.32, 0.24, 0.26), PLATE2, hip, seg=12, rings=6)                        # knee
    E((x, 0.47, 0.19), (0.08, 0.08, 0.04), RED, hip, glow="glow", k=1.0)
    P((x, 0.24, 0.04), (0.36, 0.36, 0.32), PLATE, hip, top=(1.15, 1.1), bev=0.04, seg=2)        # greave
    B((x, 0.25, 0.21), (0.05, 0.28, 0.03), EDGE, hip, bev=0.01)
    B((x, 0.04, 0.1), (0.4, 0.08, 0.56), DARK, hip, bev=0.02)                                    # sole
    P((x, 0.1, 0.14), (0.38, 0.1, 0.4), PLATE2, hip, rot=(10, 0, 0), top=(1.0, 0.85), bev=0.03)  # foot plate

# ---- hips: a heavy belt, a groin plate, tassets ----
B((0, 1.05, 0.0), (0.72, 0.22, 0.6), DARK, RT, bev=0.03)
B((0, 1.15, 0.0), (0.82, 0.1, 0.66), PLATE2, RT, bev=0.025)
P((0, 0.95, 0.31), (0.3, 0.28, 0.06), PLATE, RT, top=(0.7, 1.0), bev=0.02)
for s in (1, -1):
    P((s * 0.4, 0.95, 0.06), (0.26, 0.3, 0.06), PLATE2, RT, rot=(0, 0, s * 10), top=(1.1, 1.0), bev=0.02)

# ---- torso: a barrel of slab plate, a red stripe, vents, a gorget; the anchor ring on its back ----
T_ = "torso"
P((0, 1.68, 0.0), (1.2, 1.0, 0.78), PLATE, T_, top=(1.08, 1.0), bev=0.07, seg=2)
P((0, 1.74, 0.36), (0.9, 0.72, 0.14), PLATE2, T_, top=(1.1, 1.0), bev=0.05)                     # breastplate
B((0, 1.92, 0.44), (0.5, 0.07, 0.04), RED, T_, glow="glow", k=1.2)                              # the stripe
for k in range(3):
    B((0, 1.5 + 0.08 * k, 0.43), (0.56, 0.035, 0.03), DARK, T_)                                  # vents
for s in (1, -1):
    B((s * 0.6, 1.62, 0.0), (0.06, 0.7, 0.6), EDGE, T_, bev=0.015)
C((0, 2.17, 0.0), 0.2, 0.12, DARK, T_, sides=12)                                                 # gorget
# the anchor: a frame, ten segments round a core, braces into its pauldrons
RC = (0.0, 1.9, -0.62)
B((0, 1.75, -0.44), (0.7, 0.8, 0.2), STEEL, T_, bev=0.04)                                         # its mount
for k in range(10):
    a = k * math.pi / 5
    c = (RC[0] + 0.55 * math.sin(a), RC[1] + 0.55 * math.cos(a), RC[2])
    B(c, (0.3, 0.14, 0.2), PLATE, T_, rot=(0, 0, -math.degrees(a)), bev=0.025)
    B((RC[0] + 0.5 * math.sin(a), RC[1] + 0.5 * math.cos(a), RC[2] - 0.06), (0.22, 0.05, 0.12), RED, T_,
      rot=(0, 0, -math.degrees(a)), glow="pulse", k=1.0)
R(RC, 0.62, 0.035, STEEL, T_, sides=30, tsides=5)
R(RC, 0.38, 0.04, STEEL, T_, sides=24, tsides=5)
E(RC, (0.36, 0.36, 0.2), RED, T_, glow="core", k=1.0, seg=14, rings=7)                           # its heart
for s in (1, -1):
    C((s * 0.38, 2.15, -0.42), 0.04, 0.5, STEEL, T_, rot=(0, 0, s * 60), sides=8)                # braces
# pauldrons: slabs
for s in (1, -1):
    P((s * 0.72, 2.13, 0.0), (0.58, 0.36, 0.72), PLATE2, T_, top=(0.85, 0.9), bev=0.06, seg=2)
    B((s * 0.72, 2.32, 0.0), (0.42, 0.04, 0.56), EDGE, T_)
    B((s * 0.98, 2.1, 0.0), (0.04, 0.22, 0.6), RED, T_, glow="glow", k=0.5)

# ---- head: a low helm sunk between its pauldrons, a red visor slit, a jaw guard ----
H = "head"
P((0, 2.4, 0.0), (0.4, 0.38, 0.42), PLATE2, H, top=(0.85, 0.9), bev=0.06, seg=2)
B((0, 2.42, 0.21), (0.32, 0.06, 0.03), RED, H, glow="glow", k=2.0)                               # visor
P((0, 2.26, 0.1), (0.36, 0.12, 0.26), PLATE, H, top=(0.85, 1.0), bev=0.02)                       # jaw guard
B((0, 2.6, 0.0), (0.06, 0.06, 0.38), EDGE, H, bev=0.015)                                         # ridge

# ---- arms: thick plate, an elbow, great fists ----
for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.825
    C((x, 1.66, 0.0), 0.15, 0.5, DARK, arm, sides=12)
    P((x, 1.68, 0.0), (0.38, 0.44, 0.38), PLATE, arm, top=(1.08, 1.05), bev=0.04)
    E((x, 1.38, 0.0), (0.3, 0.24, 0.3), PLATE2, arm, seg=10, rings=5)                           # elbow
    P((x, 1.1, 0.0), (0.4, 0.44, 0.4), PLATE, arm, top=(1.2, 1.2), bev=0.04)                    # forearm
    B((x, 0.7, 0.0), (0.5, 0.45, 0.5), PLATE2, arm, bev=0.1, seg=2)                              # fist
    for k in range(4):
        B((x - s * 0.2, 0.56, -0.18 + 0.12 * k), (0.12, 0.14, 0.1), EDGE, arm, bev=0.025)       # knuckles
    B((x, 0.7, 0.26), (0.3, 0.06, 0.02), RED, arm, glow="glow", k=0.8)

report("anchor")
save_enemy("anchor")
