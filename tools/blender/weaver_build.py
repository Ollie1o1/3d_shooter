# The WEAVER in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/weaver_build.py).
# A spider machine: a plated carapace, a head with six violet eyes and two fangs, a swollen spinner behind with lit
# seams that blaze as it strings a wire ("core"), and six long legs up to a high knee and down to a spiked foot.
#   body (0, 0.85, 0)   legL1-3 / legR1-3: hips at (+-0.45, 0.85, z), z 0.45 / 0 / -0.45 (1: the front pair)
#   shinL1-3 / shinR1-3: knees at (+-1.15, 1.45, 1.25 z); its feet reach the floor at (+-1.85, 0, 1.6 z)
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("weaver", joints=globals().get("SKELETON")); clear_parts()

SHELL, SHELL2, EDGE = "weaver*1.6", "weaver*2.3", "weaver*3.2"
DARK, STEEL = "weaver*0.7", "gunmetal*1.4"
VIO = "glow*0.3"
Bo = "body"

# ---- the body: a carapace in three plates over a dark frame ----
E((0, 0.86, -0.05), (0.92, 0.42, 1.22), DARK, Bo, seg=16, rings=8)
for i in range(3):
    E((0, 1.0 - 0.02 * i, 0.32 - 0.36 * i), (0.82 - 0.06 * i, 0.22, 0.5), SHELL2 if i % 2 == 0 else SHELL, Bo, rot=(-8, 0, 0), seg=14, rings=6)
B((0, 1.1, -0.05), (0.06, 0.04, 1.0), EDGE, Bo, bev=0.012)                                       # its ridge
for s in (1, -1):
    B((s * 0.4, 0.9, -0.05), (0.03, 0.05, 0.9), VIO, Bo, glow="glow", k=0.5)                    # seams down its flanks
# the head: six eyes, two fangs, palps
E((0, 0.88, 0.7), (0.52, 0.36, 0.44), SHELL, Bo, seg=14, rings=7)
for s in (1, -1):
    E((s * 0.1, 0.96, 0.9), (0.1, 0.1, 0.06), VIO, Bo, glow="glow", k=2.5)                       # the big eyes
    E((s * 0.2, 0.98, 0.84), (0.06, 0.06, 0.04), VIO, Bo, glow="glow", k=2.0)
    E((s * 0.07, 1.04, 0.86), (0.05, 0.05, 0.04), VIO, Bo, glow="glow", k=2.0)
    tube([(s * 0.09, 0.78, 0.86), (s * 0.08, 0.66, 0.96), (s * 0.04, 0.6, 0.92)], [0.035, 0.025, 0.012], EDGE, Bo, sides=6, tip=0.05)   # fangs
    C((s * 0.2, 0.76, 0.86), 0.025, 0.2, DARK, Bo, rot=(50, 0, 0), sides=6)                      # palps
# the spinner: swollen, banded, lit from inside
E((0, 1.02, -0.62), (0.62, 0.56, 0.72), DARK, Bo, seg=16, rings=8)
for k in range(4):
    R((0, 1.02, -0.4 - 0.13 * k), 0.29 - 0.03 * abs(k - 1.5) * 1.3, 0.025, VIO, Bo, sides=20, tsides=4, glow="core", k=1.0)
E((0, 1.0, -0.98), (0.14, 0.14, 0.1), VIO, Bo, glow="core", k=1.6)                                 # its spinneret
for s in (1, -1):
    E((s * 0.2, 1.22, -0.62), (0.2, 0.12, 0.44), SHELL, Bo, rot=(0, 0, s * 20), seg=10, rings=5)   # plates on it

# ---- six legs: a ball at the hip, a plated thigh up to a lit knee, a long shin down to a spiked foot ----
for k in range(3):
    z = 0.45 - 0.45 * k
    for s, side in ((1, "L"), (-1, "R")):
        hip, knee = "leg%s%d" % (side, k + 1), "shin%s%d" % (side, k + 1)
        h = Vector((s * 0.45, 0.85, z)); kn = Vector((s * 1.15, 1.45, 1.25 * z)); ft = Vector((s * 1.85, 0.0, 1.6 * z))
        E(tuple(h), (0.16, 0.16, 0.16), STEEL, hip, seg=8, rings=5)
        d = kn - h
        C(tuple((h + kn) * 0.5), 0.06, d.length, DARK, hip, rot=_aim(d), sides=8, r2=0.05)          # thigh
        P(tuple((h + kn) * 0.5 + Vector((0, 0.05, 0))), (0.13, d.length * 0.8, 0.1), SHELL, hip, rot=_aim(d), top=(0.7, 0.8), bev=0.02)
        E(tuple(kn), (0.13, 0.13, 0.13), VIO, knee, glow="glow", k=0.3, seg=8, rings=5)              # knee
        d2 = ft - kn
        mid = kn + d2 * 0.45
        C(tuple(mid), 0.05, d2.length * 0.9, DARK, knee, rot=_aim(d2), sides=8, r2=0.025)           # shin
        P(tuple(kn + d2 * 0.3), (0.09, d2.length * 0.4, 0.08), SHELL2, knee, rot=_aim(d2), top=(0.6, 0.7), bev=0.015)
        K(tuple(kn + d2 * 0.55), 0.03, 0.14, EDGE, knee, rot=_aim((s * 0.6, 0.3, 0.0)), sides=5)      # a barb
        K((ft.x, 0.16, ft.z), 0.035, 0.16, EDGE, knee, rot=(180, 0, 0), sides=6)                      # the foot's spike

report("weaver")
save_enemy("weaver")
