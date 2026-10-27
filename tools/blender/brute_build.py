# The BRUTE in detail (python3 tools/blender/live.py tools/blender/brute_build.py). A hulking furnace-bodied
# brawler: a throbbing furnace grate in its chest, exhaust stacks on its back, fists like anvils.
#   hipL/R (+-0.275, 1.0, 0)  torso (0, 1.2, 0)  head (0, 2.35, 0)  armL/R (+-0.885, 2.14, 0)
#   head hitbox: x, z within +-0.29 of the head, y 2.28..2.88
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("brute"); clear_parts()
ARM, DARK, METAL = "brute", "gunmetal*1.3", "gunmetal"

for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.275
    B((x, 0.78, 0.0), (0.40, 0.42, 0.42), DARK, hip, bev=0.03)                         # thigh
    B((x, 0.80, 0.22), (0.36, 0.32, 0.05), ARM, hip, bev=0.02)                         # thigh plate
    C((x, 0.52, 0.0), 0.17, 0.46, METAL, hip, rot=(0, 0, 90), sides=12)                # knee
    B((x, 0.53, 0.20), (0.30, 0.20, 0.12), "brute*1.1", hip, bev=0.04, seg=2)          # knee cap
    B((x, 0.28, 0.0), (0.38, 0.40, 0.42), "brute*0.85", hip, bev=0.03)                 # shin
    B((x, 0.30, 0.22), (0.34, 0.30, 0.05), ARM, hip, bev=0.02)                         # shin plate
    B((x, 0.07, 0.08), (0.46, 0.14, 0.60), "brute*0.7", hip, bev=0.03)                 # foot
    for d in (-0.13, 0.0, 0.13):
        W((x + d, 0.06, 0.42), (0.09, 0.08, 0.14), METAL, hip, taper=0.3)              # toe claws

B((0, 1.10, 0.0), (0.85, 0.20, 0.60), DARK, "brute", bev=0.03)                         # pelvis
B((0, 1.20, 0.0), (0.95, 0.08, 0.66), METAL, "brute", bev=0.015)                       # belt
B((0, 1.20, 0.335), (0.16, 0.05, 0.01), "glow*0.3", "brute", glow="glow", k=1.0)       # buckle light
B((0, 1.03, 0.30), (0.30, 0.20, 0.06), ARM, "brute", bev=0.02)                         # groin plate
for s in (1, -1):
    B((s * 0.50, 1.02, 0.05), (0.06, 0.26, 0.50), ARM, "brute", rot=(0, 0, s * 10), bev=0.02)   # tassets

T = "torso"
B((0, 1.42, 0.0), (0.95, 0.44, 0.65), DARK, T, bev=0.04)                               # gut
B((0, 1.34, 0.33), (0.70, 0.12, 0.05), "brute*0.85", T, bev=0.015)                     # gut plates
B((0, 1.50, 0.335), (0.75, 0.12, 0.05), "brute*0.85", T, bev=0.015)
B((0, 1.98, 0.0), (1.35, 0.74, 0.85), ARM, T, bev=0.07, seg=2)                         # chest
B((0, 1.92, 0.43), (0.56, 0.50, 0.06), METAL, T, bev=0.02)                             # furnace frame
B((0, 1.92, 0.45), (0.44, 0.38, 0.04), "glow*0.3", T, glow="pulse", k=1.0)             # furnace: throbs, flares in a wind-up
for x in (-0.12, 0.0, 0.12):
    B((x, 1.92, 0.48), (0.05, 0.42, 0.03), METAL, T)                                   # grate bars
B((0, 2.30, 0.36), (1.00, 0.10, 0.12), "brute*1.1", T, bev=0.03)                       # chest rim
B((0, 1.95, -0.50), (0.90, 0.62, 0.30), "brute*0.6", T, bev=0.05, seg=2)               # back furnace
for s in (1, -1):
    C((s * 0.30, 2.38, -0.55), 0.09, 0.60, METAL, T, sides=12)                         # exhaust stacks
    C((s * 0.30, 2.69, -0.55), 0.075, 0.03, "glow*0.3", T, sides=12, glow="glow", k=0.5)   # their glow
    C((s * 0.48, 1.75, -0.45), 0.05, 0.50, METAL, T, sides=8)                          # pipes

H = "head"
B((0, 2.53, 0.06), (0.42, 0.36, 0.44), "brute*0.85", H, bev=0.06, seg=3)               # skull, sunk low
B((0, 2.64, 0.22), (0.44, 0.08, 0.10), ARM, H, rot=(12, 0, 0), bev=0.02)               # brow
B((0, 2.42, 0.25), (0.30, 0.12, 0.08), METAL, H, bev=0.015)                            # jaw grille
for s in (1, -1):
    B((s * 0.09, 2.56, 0.275), (0.07, 0.04, 0.01), "glow*0.5", H, glow="glow", k=3.0)  # eyes
    B((s * 0.20, 2.60, 0.0), (0.04, 0.20, 0.30), ARM, H, bev=0.01)                     # side plates

for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.885
    B((s * 0.92, 2.30, 0.0), (0.66, 0.40, 0.82), ARM, arm, rot=(0, 0, s * -10), bev=0.06, seg=2)   # shoulder pad
    B((s * 0.92, 2.51, 0.0), (0.50, 0.04, 0.60), "glow*0.2", arm, rot=(0, 0, s * -10), glow="glow", k=0.8)   # its stripe
    B((s * 1.20, 2.18, 0.0), (0.08, 0.30, 0.78), "brute*0.85", arm, rot=(0, 0, s * -10), bev=0.02)
    B((x, 1.78, 0.0), (0.36, 0.60, 0.38), DARK, arm, bev=0.03)                         # upper arm
    C((x, 1.43, 0.0), 0.17, 0.40, METAL, arm, rot=(0, 0, 90), sides=12)                # elbow
    B((x, 1.12, 0.0), (0.50, 0.60, 0.52), "brute*0.9", arm, bev=0.05, seg=2)           # forearm
    B((s * (0.885 + 0.255), 1.12, 0.0), (0.01, 0.40, 0.20), "glow*0.3", arm, glow="glow", k=0.6)   # forearm vent
    B((x, 0.69, 0.0), (0.62, 0.55, 0.62), "brute*0.8", arm, bev=0.06, seg=2)           # fist
    B((x, 0.62, 0.25), (0.56, 0.20, 0.14), METAL, arm, bev=0.02)                       # knuckles
save_enemy("brute")
