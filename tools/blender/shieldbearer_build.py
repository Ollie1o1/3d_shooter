# The SHIELDBEARER in detail (python3 tools/blender/live.py tools/blender/shieldbearer_build.py). A phalanx soldier in
# steel and bronze: a tower shield whose rims brighten as it winds up, a sigil on its boss, a scattergun.
#   hipL/R (+-0.15, 0.95, 0)  torso (0, 1.11, 0)  head (0, 1.81, 0)  armL/R (+-0.405, 1.715, 0)  shield (0.12, 0.96, 0.5)
#   head hitbox: x, z within +-0.22 of the head, y 1.75..2.21 (the crest stays inside it)
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("shieldbearer"); clear_parts()
ARM, DARK, METAL, BRONZE = "shieldbearer", "gunmetal*1.3", "gunmetal", "bronze"

for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.15
    B((x, 0.74, 0.0), (0.20, 0.40, 0.22), DARK, hip, bev=0.015)                        # thigh
    B((x, 0.76, 0.115), (0.18, 0.32, 0.03), ARM, hip, bev=0.01)                        # thigh plate
    C((x, 0.52, 0.0), 0.09, 0.22, METAL, hip, rot=(0, 0, 90), sides=10)                # knee
    B((x, 0.53, 0.11), (0.16, 0.13, 0.07), "shieldbearer*1.15", hip, bev=0.02, seg=2)  # knee guard
    B((x, 0.30, 0.0), (0.18, 0.36, 0.20), DARK, hip, bev=0.015)                        # shin
    B((x, 0.30, 0.105), (0.17, 0.32, 0.04), ARM, hip, bev=0.012)                       # greave
    B((x, 0.045, 0.05), (0.22, 0.09, 0.32), "shieldbearer*0.7", hip, bev=0.015)        # foot
    W((x, 0.05, 0.24), (0.20, 0.07, 0.08), ARM, hip, taper=0.4)                        # toe

B((0, 1.03, 0.0), (0.50, 0.16, 0.30), DARK, "shieldbearer", bev=0.015)                 # pelvis
B((0, 1.10, 0.0), (0.56, 0.05, 0.34), "bronze*0.8", "shieldbearer")                    # belt
for s in (1, -1):
    B((s * 0.28, 1.00, 0.03), (0.04, 0.20, 0.26), ARM, "shieldbearer", rot=(0, 0, s * 8), bev=0.01)   # tassets
B((0, 0.99, 0.16), (0.16, 0.16, 0.04), ARM, "shieldbearer", bev=0.01)                  # groin plate

T = "torso"
B((0, 1.24, 0.0), (0.42, 0.26, 0.28), DARK, T, bev=0.02)                               # gut
B((0, 1.24, 0.14), (0.30, 0.20, 0.03), "shieldbearer*0.85", T, bev=0.008)
B((0, 1.57, 0.0), (0.62, 0.48, 0.36), ARM, T, bev=0.04, seg=2)                         # chest
B((0, 1.60, 0.18), (0.48, 0.34, 0.04), "shieldbearer*1.15", T, bev=0.015)              # breastplate
B((0, 1.545, 0.202), (0.20, 0.03, 0.01), "glow*0.3", T, glow="glow", k=1.2)            # glow strip
B((0, 1.74, 0.17), (0.50, 0.04, 0.04), BRONZE, T)                                      # bronze trim
B((0, 1.80, 0.0), (0.36, 0.06, 0.30), METAL, T, bev=0.01)                              # collar
B((0, 1.55, -0.22), (0.40, 0.36, 0.10), "shieldbearer*0.6", T, bev=0.02)               # pack

H = "head"
B((0, 1.97, -0.01), (0.30, 0.30, 0.32), "shieldbearer*0.85", H, bev=0.05, seg=3)       # helmet
B((0, 1.94, 0.15), (0.24, 0.18, 0.04), METAL, H, bev=0.01)                             # face guard
B((0, 1.98, 0.172), (0.20, 0.035, 0.01), "glow*0.3", H, glow="glow", k=2.0)            # visor slit
B((0, 1.96, 0.18), (0.04, 0.12, 0.02), BRONZE, H)                                      # nasal
B((0, 2.17, -0.02), (0.05, 0.08, 0.34), BRONZE, H, bev=0.01)                           # crest
for s in (1, -1):
    B((s * 0.16, 1.93, 0.05), (0.03, 0.18, 0.20), ARM, H)                              # cheek plates

for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.405
    B((s * 0.44, 1.76, 0.0), (0.22, 0.10, 0.30), ARM, arm, rot=(0, 0, s * -12), bev=0.03, seg=2)   # pauldron
    B((s * 0.53, 1.70, 0.0), (0.03, 0.10, 0.28), BRONZE, arm, rot=(0, 0, s * -12))
    B((x, 1.52, 0.0), (0.15, 0.30, 0.16), DARK, arm, bev=0.012)                        # upper arm
    C((x, 1.33, 0.0), 0.07, 0.17, METAL, arm, rot=(0, 0, 90), sides=10)                # elbow
    B((x, 1.17, 0.0), (0.17, 0.28, 0.18), "shieldbearer*0.9", arm, bev=0.015)          # forearm
    B((x, 0.98, 0.0), (0.13, 0.12, 0.13), DARK, arm)                                   # hand
R, gx = "armR", -0.405                                                                 # the scattergun
B((gx, 0.975, 0.05), (0.14, 0.36, 0.18), METAL, R, bev=0.015)                          # body
for d in (-0.035, 0.035):
    C((gx + d, 0.84, 0.06), 0.03, 0.16, "gunmetal*1.4", R, sides=8)                    # barrels
C((gx, 0.755, 0.06), 0.08, 0.05, METAL, R, sides=12, glow="shot", k=1.0)               # muzzle

S = "shield"
B((0.12, 0.96, 0.50), (1.05, 1.60, 0.12), "shieldbearer*1.25", S, bev=0.03, seg=2)     # tower shield
B((0.12, 0.96, 0.57), (0.86, 1.38, 0.03), "shieldbearer*0.9", S, bev=0.01)             # its face
for y in (1.79, 0.13):
    B((0.12, y, 0.53), (1.08, 0.07, 0.16), BRONZE, S, glow="rim", k=1.0, bev=0.015)    # rims: brighten in a wind-up
for s in (1, -1):
    B((0.12 + s * 0.535, 0.96, 0.53), (0.05, 1.60, 0.14), "bronze*0.8", S, bev=0.01)   # side rims
C((0.12, 1.06, 0.60), 0.16, 0.05, "bronze*0.7", S, rot=(90, 0, 0), sides=18)           # boss
B((0.12, 1.06, 0.63), (0.08, 0.56, 0.02), "bronze*0.5", S, glow="glow", k=1.2)         # sigil
B((0.12, 1.06, 0.63), (0.34, 0.08, 0.02), "bronze*0.5", S, glow="glow", k=1.2)
for dx in (-0.4, 0.4):
    for dy in (-0.68, 0.68):
        B((0.12 + dx, 0.96 + dy, 0.60), (0.05, 0.05, 0.03), METAL, S)                  # rivets
B((0.12, 0.96, 0.42), (0.30, 0.20, 0.06), METAL, S)                                    # grip
save_enemy("shieldbearer")
