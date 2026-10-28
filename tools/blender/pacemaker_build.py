# THE PACEMAKER in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/pacemaker_build.py
# SKELETON=<tools/enemy_export --joints pacemaker out.glb> the first time; after that it rebuilds on its own file).
# The Gauntlet's last: the machine that set the par times. A long-legged runner in black and brass; a clock face in its
# chest (enamel, brass hands; its rim "core" ticks each second and races through a wind-up), a blade along each
# forearm ("edge": white-hot through a white tell), a pendulum swinging in a cage on its back, a narrow helm with one
# lit slit ("glow"), and vents down its back that open when it overclocks (show "enraged").
#   hipL/R (+-0.15, 1.2, 0)  torso (0, 1.35, 0)  head (0, 2.15, 0)  armL/R (+-0.385, 2.065, 0)
#   head hitbox: x, z within +-0.21 of the head, y 2.09..2.53
import math
import os

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("pacemaker", joints=globals().get("SKELETON")); clear_parts()
BLACK, BLACK2, BRASS, BRASS2, ENAMEL, METAL = "pacemaker", "pacemaker*1.6", "brass", "brass*0.7", "enamel", "gunmetal"
HOT = "glow*0.4"
RT = [o for o in bpy.context.scene.objects if o.type != "MESH" and o.parent is None][0].name   # (its root)

# ---- legs: long, sprung, brass at the joints ----
for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.15
    P((x, 0.92, 0.0), (0.2, 0.52, 0.22), BLACK, hip, top=(1.15, 1.1), bev=0.02)              # thigh
    B((x, 0.94, 0.11), (0.14, 0.40, 0.03), BLACK2, hip, bev=0.01)                            # its plate
    C((x, 0.63, 0.0), 0.085, 0.22, BRASS, hip, rot=(0, 0, 90), sides=14)                     # knee
    P((x, 0.36, -0.02), (0.15, 0.5, 0.17), BLACK, hip, top=(1.2, 1.2), bev=0.015)            # shin
    B((x, 0.36, 0.08), (0.03, 0.42, 0.02), HOT, hip, glow="glow", k=0.7)                     # a lit seam down it
    C((x, 0.36, -0.12), 0.025, 0.34, BRASS2, hip, sides=8)                                   # its spring
    P((x, 0.04, 0.06), (0.14, 0.08, 0.32), BLACK2, hip, top=(0.8, 0.9), bev=0.015)           # foot, a runner's blade
    W((x, 0.03, 0.26), (0.12, 0.06, 0.12), BRASS, hip, taper=0.3)                            # toe

B((0, 1.28, 0.0), (0.4, 0.16, 0.24), BLACK2, RT, bev=0.02)                                   # pelvis
B((0, 1.35, 0.0), (0.44, 0.04, 0.28), BRASS, RT)                                             # belt

# ---- the torso: a narrow cuirass, the clock in its chest, the pendulum on its back ----
T = "torso"
P((0, 1.55, 0.0), (0.4, 0.4, 0.24), BLACK, T, top=(1.35, 1.1), bev=0.02)                     # waist to chest
B((0, 1.88, 0.0), (0.6, 0.36, 0.32), BLACK2, T, bev=0.04, seg=2)                             # chest
C((0, 1.85, 0.17), 0.16, 0.03, ENAMEL, T, rot=(90, 0, 0), sides=24)                          # the clock face
R((0, 1.85, 0.185), 0.16, 0.022, HOT, T, sides=28, tsides=4, glow="core", k=1.0)             # its lit rim: ticks
for k in range(12):                                                                          # hour marks
    a = k * math.pi / 6
    B((0.12 * math.sin(a), 1.85 + 0.12 * math.cos(a), 0.19), (0.012, 0.03, 0.01), BLACK, T, rot=(0, 0, -math.degrees(a)))
B((0.0, 1.89, 0.195), (0.015, 0.09, 0.01), BRASS, T)                                         # its hands
B((0.035, 1.84, 0.195), (0.07, 0.014, 0.01), BRASS, T, rot=(0, 0, -20))
B((0, 2.08, 0.0), (0.34, 0.06, 0.28), BRASS2, T, bev=0.01)                                   # collar
B((0, 1.82, -0.22), (0.34, 0.5, 0.12), BLACK2, T, bev=0.02)                                  # the pendulum's cage
for dx in (-0.14, 0.14):
    B((dx, 1.82, -0.29), (0.025, 0.5, 0.025), BRASS, T)
C((0, 1.86, -0.29), 0.008, 0.36, BRASS, T, sides=6)                                          # the pendulum
E((0, 1.66, -0.29), (0.09, 0.09, 0.03), HOT, T, glow="core", k=0.8, seg=10, rings=5)         # its bob, lit
for k in range(3):                                                                           # vents: open in the overclock
    B((0.0, 1.62 + 0.1 * k, -0.155), (0.26, 0.04, 0.02), HOT, T, glow="glow", k=1.5, show="enraged")
    B((0.0, 1.62 + 0.1 * k, -0.155), (0.26, 0.04, 0.02), BLACK, T, show="!enraged")

# ---- the head: a narrow helm, one lit slit ----
H = "head"
P((0, 2.30, 0.0), (0.27, 0.3, 0.3), BLACK, H, top=(0.85, 0.9), bev=0.04, seg=2)              # helm
B((0, 2.32, 0.15), (0.2, 0.03, 0.02), HOT, H, glow="glow", k=2.4)                            # the slit
B((0, 2.47, -0.02), (0.05, 0.05, 0.3), BRASS, H, bev=0.01)                                   # crest
for s in (1, -1):
    B((s * 0.14, 2.28, 0.02), (0.02, 0.16, 0.2), BRASS2, H)                                  # cheek guards

# ---- arms: brass shoulders, a blade along each forearm ----
for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.385
    E((s * 0.4, 2.06, 0.0), (0.22, 0.16, 0.24), BRASS2, arm, seg=12, rings=6)                # shoulder
    P((x, 1.83, 0.0), (0.14, 0.4, 0.15), BLACK, arm, top=(1.1, 1.1), bev=0.012)              # upper arm
    C((x, 1.6, 0.0), 0.065, 0.16, BRASS, arm, rot=(0, 0, 90), sides=12)                      # elbow
    P((x, 1.38, 0.0), (0.13, 0.4, 0.14), BLACK2, arm, top=(1.2, 1.2), bev=0.012)             # forearm
    W((x + s * 0.06, 1.33, 0.02), (0.03, 0.62, 0.1), HOT, arm, rot=(90, 0, 0), glow="edge", k=1.0, taper=0.2)   # its blade
    B((x, 1.13, 0.0), (0.11, 0.11, 0.11), BRASS2, arm, bev=0.01)                             # fist
report("pacemaker")
save_enemy("pacemaker")
