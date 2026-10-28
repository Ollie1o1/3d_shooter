# THE FOREMAN in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/foreman_build.py).
# The Pumpworks' mini-boss, on the CONDUCTOR's skeleton (drawn x1.7): a floating overseer in hazard yellow under a hard
# hat, a tannoy horn on its front, a crane hook hung beneath (its tethers run from the hook's ring: "core", brighter
# while its crew holds), and three striped fan blades spinning round it ("halo").
#   body (0, 0.85, 0): it bobs   halo (0, 0.85, 0): its blades spin about Y
import math
import os

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("foreman", joints=os.path.join(REPO, "assets", "enemies", BASE_FILE["foreman"] + ".glb")); clear_parts()

YEL, YEL2, DARK, METAL, STRIPE = "foreman*1.2", "hazard", "gunmetal*1.3", "gunmetal", "gunmetal*0.6"
HOT = "glow*0.4"
Bo, Y = "body", 0.85

E((0, Y, 0.0), (0.5, 0.46, 0.5), YEL, Bo, seg=16, rings=8)                                   # body
B((0, Y, 0.0), (0.54, 0.08, 0.54), STRIPE, Bo, bev=0.02)                                     # its belt
for k in range(4):   # hazard chevrons on the belt
    a = k * math.pi / 2 + math.pi / 4
    B((0.27 * math.sin(a), Y, 0.27 * math.cos(a)), (0.1, 0.085, 0.02), YEL2, Bo, rot=(0, math.degrees(a), 45))
E((0, Y + 0.24, 0.0), (0.62, 0.22, 0.62), YEL2, Bo, seg=16, rings=6)                         # hard hat
B((0, Y + 0.17, 0.30), (0.4, 0.04, 0.12), YEL2, Bo, bev=0.01)                                # its brim
C((0, Y + 0.38, 0.0), 0.05, 0.08, DARK, Bo, sides=8)
E((0, Y + 0.45, 0.0), (0.09, 0.09, 0.09), HOT, Bo, glow="glow", k=1.6, seg=8, rings=5)       # beacon on the hat
B((0, Y + 0.02, 0.24), (0.3, 0.1, 0.04), HOT, Bo, glow="glow", k=1.2)                        # its visor
C((0, Y - 0.12, 0.34), 0.06, 0.16, METAL, Bo, rot=(90, 0, 0), sides=10)                      # the tannoy
C((0, Y - 0.12, 0.47), 0.15, 0.12, DARK, Bo, rot=(90, 0, 0), sides=14, r2=0.06)              # its horn
for s in (1, -1):
    C((s * 0.36, Y - 0.05, 0.0), 0.08, 0.14, METAL, Bo, rot=(0, 0, 90), sides=10)            # thruster pods
    C((s * 0.36, Y - 0.14, 0.0), 0.05, 0.04, HOT, Bo, sides=10, glow="glow", k=1.0)
C((0, Y - 0.42, 0.0), 0.04, 0.3, DARK, Bo, sides=8)                                          # the hook's cable
B((0, Y - 0.6, 0.0), (0.14, 0.12, 0.1), METAL, Bo, bev=0.015)                                # its block
R((0, Y - 0.78, 0.04), 0.1, 0.03, HOT, Bo, rot=(0, 90, 0), sides=16, tsides=4, arc=250, glow="core", k=0.9)   # the lit hook

# ---- three fan blades on a ring, spinning ----
R((0, Y + 0.05, 0.0), 0.78, 0.02, METAL, "halo", rot=(90, 0, 0), sides=32, tsides=4)
for k in range(3):
    a = k * 2 * math.pi / 3
    c = (0.78 * math.sin(a), Y + 0.05, 0.78 * math.cos(a))
    P(c, (0.12, 0.08, 0.42), YEL2, "halo", rot=(0, math.degrees(a) + 90, 18), top=(0.6, 0.6), bev=0.012)
    B((0.84 * math.sin(a), Y + 0.05, 0.84 * math.cos(a)), (0.03, 0.05, 0.12), STRIPE, "halo", rot=(0, math.degrees(a), 0))
report("foreman")
save_enemy("foreman")
