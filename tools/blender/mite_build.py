# The MITE in detail (python3 tools/blender/live.py tools/blender/mite_build.py). A bomb-beetle: a rounded shell
# under a glowing dome that flashes red as its fuse burns, mandibles, jointed legs.
#   body (0, 0.32, 0)  legL1-3 (0.22, 0.32, 0.18 / -0.02 / -0.22), legR1-3 the same at -0.22
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("mite"); clear_parts()
ARM, DARK, METAL = "mite", "gunmetal*1.3", "gunmetal"

Bo = "body"
B((0, 0.35, -0.14), (0.48, 0.30, 0.48), ARM, Bo, bev=0.10, seg=3)                       # shell
C((0, 0.53, -0.12), 0.13, 0.10, "glow*0.3", Bo, sides=16, glow="fuse", k=1.0, r2=0.09)  # the charge: steady, then flashing
C((0, 0.485, -0.12), 0.165, 0.03, METAL, Bo, sides=16)                                  # its collar, below it
B((0, 0.36, 0.02), (0.50, 0.26, 0.03), METAL, Bo, bev=0.01)                             # shell bands
B((0, 0.36, -0.30), (0.50, 0.26, 0.03), METAL, Bo, bev=0.01)
B((0, 0.32, 0.20), (0.30, 0.20, 0.22), "mite*0.5", Bo, bev=0.04, seg=2)                 # head
for s in (1, -1):
    W((s * 0.07, 0.26, 0.34), (0.04, 0.04, 0.10), METAL, Bo, taper=0.3)                 # mandibles
    B((s * 0.07, 0.36, 0.31), (0.05, 0.04, 0.02), "glow*1", Bo, glow="glow", k=2.0)     # eyes
C((0, 0.60, -0.26), 0.008, 0.12, METAL, Bo, sides=6)                                    # fuse wire
B((0, 0.67, -0.26), (0.025, 0.025, 0.025), "glow*0.5", Bo, glow="fuse", k=0.6)          # its spark

for i in range(3):
    z = 0.18 - 0.2 * i
    for s, leg in ((1, "legL%d" % (i + 1)), (-1, "legR%d" % (i + 1))):
        B((s * 0.34, 0.32, z), (0.26, 0.05, 0.05), "mite*0.5", leg, bev=0.01)           # femur
        B((s * 0.55, 0.30, z), (0.18, 0.04, 0.04), METAL, leg, rot=(0, 0, s * -25))     # shin, bent down
save_enemy("mite")
