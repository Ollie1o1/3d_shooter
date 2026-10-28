# THE PLUMB LINE in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/plumb_build.py).
# The Well's mini-boss, on the CONDUIT's skeleton (drawn x1.45): a cast-iron plumb weight that hangs down the shaft on
# its chain and follows you down. Its core sits behind vents in its belly ("core": white-hot while it's open), two
# armoured bands turn round it ("halo" low, "crown" high: "pulse" lights their studs), and a pulley block on top
# ("head") takes the chain up out of sight.
#   halo (0, 1.2, 0)  crown (0, 2.3, 0)  head (0, 3.1, 0); it hangs (it flies: no floor under it)
import math
import os

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("plumb", joints=os.path.join(REPO, "assets", "enemies", BASE_FILE["plumb"] + ".glb")); clear_parts()

IRON, IRON2, RUST, BRASS = "plumb*1.2", "plumb*1.7", "rust", "brass"
HOT = "glow*0.3"
RT = "conduit"

# ---- the weight: a cone to its point below, a drum above, vents round its core ----
K((0, 0.75, 0.0), 0.62, 0.75, IRON, RT, rot=(180, 0, 0), sides=16)                          # the point
C((0, 1.25, 0.0), 0.66, 1.0, IRON2, RT, sides=16)                                           # the drum
C((0, 1.25, 0.0), 0.55, 0.62, HOT, RT, sides=16, glow="core", k=1.0)                        # the core inside it
for k in range(8):   # vents: slats over the core
    a = k * math.pi / 4
    B((0.64 * math.sin(a), 1.25, 0.64 * math.cos(a)), (0.16, 0.56, 0.06), IRON, RT, rot=(0, math.degrees(a), 0), bev=0.015)
R((0, 1.76, 0.0), 0.68, 0.05, RUST, RT, rot=(90, 0, 0), sides=24, tsides=5)                 # its rims
R((0, 0.74, 0.0), 0.64, 0.05, RUST, RT, rot=(90, 0, 0), sides=24, tsides=5)
K((0, 1.75, 0.0), 0.62, 0.6, IRON, RT, sides=16)                                            # its shoulder, up to the neck
C((0, 2.6, 0.0), 0.12, 0.7, IRON2, RT, sides=10)                                            # the neck
E((0, 0.02, 0.0), (0.12, 0.12, 0.12), HOT, RT, glow="glow", k=1.4, seg=10, rings=6)         # a lit bead at its point
# ---- the chain, up out of sight ----
for k in range(10):
    R((0, 3.45 + 0.24 * k, 0.0), 0.09, 0.025, BRASS if k % 2 else IRON2, RT, rot=(0, 90 * (k % 2), 0), sides=12, tsides=4)

# ---- two armoured bands, turning ----
for joint, y, rad in (("halo", 1.2, 0.86), ("crown", 2.3, 0.62)):
    R((0, y, 0.0), rad, 0.06, IRON2, joint, rot=(90, 0, 0), sides=28, tsides=5)
    for k in range(6):
        a = k * math.pi / 3
        B((rad * math.sin(a), y, rad * math.cos(a)), (0.18, 0.2, 0.12), RUST, joint, rot=(0, math.degrees(a), 0), bev=0.02)
        E((rad * 1.06 * math.sin(a), y, rad * 1.06 * math.cos(a)), (0.07, 0.07, 0.07), HOT, joint, glow="pulse", k=1.2, seg=8, rings=5)

# ---- the pulley block on top ----
H = "head"
B((0, 3.1, 0.0), (0.34, 0.4, 0.22), IRON, H, bev=0.03, seg=2)
C((0, 3.12, 0.0), 0.16, 0.26, BRASS, H, rot=(90, 0, 0), sides=16)                           # its sheave
C((0, 3.12, 0.0), 0.06, 0.3, HOT, H, rot=(90, 0, 0), sides=10, glow="glow", k=1.2)          # lit axle
report("plumb")
save_enemy("plumb")
