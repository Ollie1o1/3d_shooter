# The CONDUCTOR in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/conductor_build.py).
# A hovering emitter: a lit core between the two halves of a split shell, domed caps, an antenna with a lit tip, a prong
# beneath with the ring its tethers run from ("core": brighter while it holds links), and three fins spinning round it
# on a thin ring ("halo").
#   body (0, 0.85, 0): it bobs   halo (0, 0.85, 0): its fins spin about Y   (built at its drawn size: x1.3)
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("conductor", joints=globals().get("SKELETON")); clear_parts()

SHELL, SHELL2, EDGE = "conductor*1.4", "conductor*2.0", "conductor*2.8"
DARK = "conductor*0.6"
HOT = "glow*0.4"
Bo, Y = "body", 0.85

E((0, Y, 0.0), (0.42, 0.42, 0.42), HOT, Bo, glow="core", k=1.0, seg=14, rings=8)                    # the core
for s in (1, -1):   # the split shell
    E((s * 0.34, Y, 0.0), (0.22, 0.66, 0.62), SHELL, Bo, seg=14, rings=8)
    B((s * 0.45, Y, 0.0), (0.03, 0.44, 0.06), HOT, Bo, glow="glow", k=0.8)                        # a lit seam on it
    R((s * 0.23, Y, 0.0), 0.3, 0.02, EDGE, Bo, rot=(0, 90, 0), sides=20, tsides=4)                 # its rim
E((0, Y + 0.33, 0.0), (0.44, 0.18, 0.44), SHELL2, Bo, seg=14, rings=6)                              # top dome
E((0, Y - 0.33, 0.0), (0.44, 0.18, 0.44), DARK, Bo, seg=14, rings=6)                                # bottom dome
C((0, Y + 0.58, 0.0), 0.02, 0.36, EDGE, Bo, sides=6)                                                 # antenna
E((0, Y + 0.8, 0.0), (0.1, 0.1, 0.1), HOT, Bo, glow="core", k=0.8, seg=8, rings=4)
C((0, Y - 0.5, 0.0), 0.05, 0.3, DARK, Bo, sides=8)                                                   # the prong
R((0, Y - 0.68, 0.0), 0.11, 0.025, HOT, Bo, rot=(90, 0, 0), sides=16, tsides=4, glow="core", k=0.7)  # its tether ring
K((0, Y - 0.66, 0.0), 0.05, 0.12, EDGE, Bo, rot=(180, 0, 0), sides=6)

# ---- three fins on a ring, spinning ----
R((0, Y, 0.0), 0.72, 0.02, EDGE, "halo", rot=(90, 0, 0), sides=32, tsides=4)
for k in range(3):
    a = k * 2 * math.pi / 3
    c = (0.72 * math.sin(a), Y, 0.72 * math.cos(a))
    P(c, (0.1, 0.56, 0.3), SHELL2, "halo", rot=(0, math.degrees(a), 0), top=(0.5, 0.6), bev=0.015)
    B((0.76 * math.sin(a), Y, 0.76 * math.cos(a)), (0.02, 0.4, 0.04), HOT, "halo", rot=(0, math.degrees(a), 0), glow="glow", k=0.6)

report("conductor")
save_enemy("conductor")
