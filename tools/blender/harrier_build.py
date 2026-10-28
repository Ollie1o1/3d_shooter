# THE HARRIER in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/harrier_build.py).
# The Span's mini-boss, on the RAPTOR's skeleton (drawn x2.3): a storm-steel hawk-machine. Jet pods under its wings,
# twin tail booms with a lit fan between them, a hooked beak, harpoon talons that light as it dives ("dive"), cyan
# leading edges ("glow").
#   head (0, 0.62, 0.55)  tail (0, 0.5, -0.5)  wingL/R (+-0.25, 0.55, 0.05)  tipL/R (+-1.25, 0.55, 0.05)
#   head hitbox: x within +-0.235, y -0.18..0.28, z -0.16..0.68 of the head
import os

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("harrier", joints=os.path.join(REPO, "assets", "enemies", BASE_FILE["harrier"] + ".glb")); clear_parts()
ARM, ARM2, DARK, METAL, R_ = "harrier", "harrier*1.3", "gunmetal*1.3", "gunmetal", "raptor"
LIGHT = "glow*0.3"

B((0, 0.46, -0.02), (0.46, 0.38, 0.95), ARM, R_, bev=0.06, seg=2)                          # fuselage
B((0, 0.37, 0.30), (0.42, 0.30, 0.42), ARM2, R_, bev=0.05, seg=2)                          # keel
B((0, 0.34, 0.515), (0.18, 0.12, 0.02), LIGHT, R_, glow="glow", k=1.6)                      # chest core
for z in (0.25, 0.0, -0.25):
    B((0, 0.67, z), (0.20, 0.06, 0.20), ARM2, R_, bev=0.02)                                 # back plates
    B((0, 0.705, z), (0.04, 0.02, 0.16), LIGHT, R_, glow="glow", k=0.8)
for s in (1, -1):
    C((s * 0.30, 0.34, 0.05), 0.11, 0.42, METAL, R_, rot=(90, 0, 0), sides=14)               # jet pods under its wings
    C((s * 0.30, 0.34, -0.17), 0.08, 0.03, LIGHT, R_, rot=(90, 0, 0), sides=14, glow="glow", k=1.8)   # their burners
    R((s * 0.30, 0.34, 0.26), 0.11, 0.015, ARM2, R_, sides=16, tsides=4)
    B((s * 0.14, 0.20, 0.12), (0.08, 0.20, 0.08), "harrier*0.5", R_, glow="dive", k=1.0)    # talons: lit in a dive
    W((s * 0.14, 0.07, 0.18), (0.09, 0.07, 0.16), METAL, R_, rot=(35, 0, 0), taper=0.25)   # harpoon tips
C((0, 0.46, -0.52), 0.10, 0.10, METAL, R_, rot=(90, 0, 0), sides=12)                        # thruster
C((0, 0.46, -0.58), 0.07, 0.02, LIGHT, R_, rot=(90, 0, 0), sides=12, glow="glow", k=1.5)

H = "head"
B((0, 0.67, 0.67), (0.32, 0.27, 0.34), ARM, H, bev=0.05, seg=2)                            # skull
B((0, 0.75, 0.66), (0.26, 0.05, 0.28), ARM2, H, bev=0.015)                                 # brow plate
W((0, 0.60, 0.96), (0.13, 0.11, 0.30), "beak", H, glow="const", k=(0.22, 0.12, 0.02), taper=0.2)   # hooked beak
W((0, 0.55, 1.05), (0.06, 0.06, 0.10), METAL, H, rot=(40, 0, 0), taper=0.2)                # its hook
for s in (1, -1):
    B((s * 0.13, 0.70, 0.80), (0.05, 0.05, 0.08), "glow*1", H, glow="glow", k=2.6)         # eyes
    W((s * 0.09, 0.82, 0.55), (0.04, 0.10, 0.26), ARM2, H, rot=(0, 180, 0), taper=0.3)     # swept crests

Ta = "tail"
for s in (1, -1):
    B((s * 0.12, 0.50, -0.80), (0.08, 0.08, 0.60), ARM, Ta, bev=0.02)                       # twin booms
    W((s * 0.12, 0.62, -1.02), (0.04, 0.20, 0.20), ARM2, Ta, rot=(0, 180, 0), taper=0.3)    # their fins
B((0, 0.50, -1.08), (0.56, 0.04, 0.24), "harrier*0.8", Ta, bev=0.02)                        # the fan between them
B((0, 0.50, -1.21), (0.58, 0.05, 0.04), LIGHT, Ta, glow="glow", k=1.3)

for s, wg, tp in ((1, "wingL", "tipL"), (-1, "wingR", "tipR")):
    C((s * 0.75, 0.58, 0.30), 0.04, 1.0, METAL, wg, rot=(0, 0, 90), sides=10)               # spar
    B((s * 0.75, 0.55, 0.04), (1.00, 0.05, 0.60), ARM, wg, bev=0.02)                        # wing plate
    for x in (0.45, 0.80, 1.10):
        B((s * x, 0.53, -0.30), (0.30, 0.035, 0.22), "harrier*0.85", wg, bev=0.01)          # vanes
    B((s * 0.75, 0.585, 0.35), (1.00, 0.05, 0.03), LIGHT, wg, glow="glow", k=1.3)           # leading edge
    B((s * 0.30, 0.61, 0.10), (0.20, 0.15, 0.24), METAL, wg, bev=0.02)                      # shoulder
    C((s * 1.60, 0.58, 0.25), 0.03, 0.70, METAL, tp, rot=(0, 0, 90), sides=8)               # tip spar
    B((s * 1.70, 0.55, -0.02), (0.90, 0.04, 0.50), ARM2, tp, bev=0.015)                     # tip plate, swept
    B((s * 2.02, 0.53, -0.32), (0.32, 0.03, 0.18), "harrier*0.7", tp)
    B((s * 2.16, 0.55, -0.08), (0.14, 0.05, 0.54), LIGHT, tp, glow="glow", k=1.6)           # tip light
report("harrier")
save_enemy("harrier")
