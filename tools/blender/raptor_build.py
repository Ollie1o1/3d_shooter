# The RAPTOR in detail (python3 tools/blender/live.py tools/blender/raptor_build.py). A mechanical hawk: jointed
# wings with glowing leading edges, a tail fan, a thruster, talons that light as it dives.
#   head (0, 0.62, 0.55)  tail (0, 0.5, -0.5)  wingL/R (+-0.25, 0.55, 0.05)  tipL/R (+-1.25, 0.55, 0.05)
#   head hitbox: x within +-0.235, y -0.18..0.28, z -0.16..0.68 of the head
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("raptor"); clear_parts()
ARM, DARK, METAL, R = "raptor", "gunmetal*1.3", "gunmetal", "raptor"

B((0, 0.45, -0.02), (0.44, 0.36, 0.90), ARM, R, bev=0.06, seg=2)                         # fuselage
B((0, 0.36, 0.30), (0.40, 0.30, 0.40), "raptor*1.15", R, bev=0.05, seg=2)               # keel
B((0, 0.33, 0.505), (0.16, 0.12, 0.02), "glow*0.3", R, glow="glow", k=1.5)              # chest core
B((0, 0.66, 0.10), (0.18, 0.06, 0.30), "raptor*1.2", R, bev=0.02)                       # back ridge
B((0, 0.65, -0.25), (0.16, 0.06, 0.28), "raptor*1.2", R, bev=0.02)
C((0, 0.45, -0.50), 0.10, 0.12, METAL, R, rot=(90, 0, 0), sides=12)                     # thruster
C((0, 0.45, -0.565), 0.07, 0.02, "glow*0.4", R, rot=(90, 0, 0), sides=12, glow="glow", k=1.5)
for s in (1, -1):
    B((s * 0.12, 0.22, 0.12), (0.07, 0.18, 0.07), "raptor*0.5", R, glow="dive", k=1.0)  # talons: lit in a dive
    W((s * 0.12, 0.10, 0.16), (0.08, 0.06, 0.12), METAL, R, rot=(30, 0, 0), taper=0.3)

H = "head"
B((0, 0.67, 0.67), (0.30, 0.26, 0.32), "raptor*0.9", H, bev=0.05, seg=2)                # skull
W((0, 0.60, 0.95), (0.12, 0.10, 0.30), "beak", H, glow="const", k=(0.25, 0.12, 0.0), taper=0.25)   # beak
for s in (1, -1):
    B((s * 0.12, 0.70, 0.80), (0.05, 0.06, 0.06), "glow*1", H, glow="glow", k=2.5)      # eyes
W((0, 0.82, 0.58), (0.05, 0.12, 0.30), "raptor*0.5", H, rot=(0, 180, 0), glow="glow", k=0.6, taper=0.3)   # crest

Ta = "tail"
B((0, 0.50, -0.75), (0.14, 0.08, 0.50), "raptor*0.8", Ta, bev=0.02)                     # tail boom
B((0, 0.50, -1.05), (0.50, 0.04, 0.24), "raptor*0.8", Ta, bev=0.02)                     # tail fan
B((0, 0.50, -1.18), (0.52, 0.05, 0.04), "glow*0.3", Ta, glow="glow", k=1.2)             # its glowing edge

for s, wg, tp in ((1, "wingL", "tipL"), (-1, "wingR", "tipR")):
    C((s * 0.75, 0.58, 0.30), 0.035, 1.0, METAL, wg, rot=(0, 0, 90), sides=10)          # arm spar
    B((s * 0.75, 0.55, 0.02), (1.00, 0.04, 0.56), ARM, wg, bev=0.02)                    # wing plate
    B((s * 0.60, 0.53, -0.28), (0.36, 0.03, 0.18), "raptor*0.85", wg, bev=0.01)         # feathers
    B((s * 1.00, 0.53, -0.30), (0.36, 0.03, 0.20), "raptor*0.85", wg, bev=0.01)
    B((s * 0.75, 0.58, 0.33), (1.00, 0.05, 0.03), "glow*0.3", wg, glow="glow", k=1.2)   # leading edge
    B((s * 0.30, 0.60, 0.10), (0.18, 0.14, 0.22), METAL, wg, bev=0.02)                  # shoulder
    C((s * 1.60, 0.58, 0.25), 0.028, 0.70, METAL, tp, rot=(0, 0, 90), sides=8)          # tip spar
    B((s * 1.68, 0.55, -0.05), (0.86, 0.035, 0.46), "raptor*0.8", tp, bev=0.015)        # tip plate
    B((s * 1.95, 0.53, -0.32), (0.30, 0.03, 0.16), "raptor*0.7", tp)                    # tip feather
    B((s * 2.10, 0.55, -0.10), (0.14, 0.05, 0.50), "glow*0.4", tp, glow="glow", k=1.5)  # tip glow
save_enemy("raptor")
