# The SENTINEL in detail (run over the live link: python3 tools/blender/live.py tools/blender/sentinel_build.py).
# A lanky sniper: one cyclops eye, a long rail rifle with a glowing scope. Joints stay where the code turns them:
#   hipL/R (+-0.11, 1.25, 0)  torso (0, 1.39, 0)  head (0, 2.13, 0)  armL/R (+-0.29, 2.06, 0)
#   head hitbox: x, z within +-0.21 of the head, y 2.07..2.51
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("sentinel"); clear_parts()
ARM, DARK, METAL = "sentinel", "gunmetal*1.3", "gunmetal"

for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.11
    B((x, 0.98, 0.0), (0.12, 0.50, 0.13), DARK, hip, bev=0.01)                         # thigh
    B((x, 1.00, 0.07), (0.11, 0.40, 0.03), ARM, hip, bev=0.008)                        # thigh plate
    C((x, 0.72, 0.0), 0.055, 0.13, METAL, hip, rot=(0, 0, 90), sides=10)               # knee
    B((x, 0.74, 0.075), (0.10, 0.10, 0.05), "sentinel*1.1", hip, bev=0.015, seg=2)     # knee guard
    B((x, 0.42, -0.01), (0.10, 0.56, 0.11), DARK, hip, bev=0.01)                       # shin
    B((x, 0.44, 0.065), (0.09, 0.46, 0.03), ARM, hip, bev=0.008)                       # shin plate
    B((x, 0.10, 0.0), (0.08, 0.06, 0.08), METAL, hip)                                  # ankle
    B((x, 0.035, 0.04), (0.13, 0.07, 0.22), "sentinel*0.7", hip, bev=0.012)            # foot
    W((x, 0.04, 0.19), (0.12, 0.06, 0.09), ARM, hip, taper=0.45)                       # toe
    B((x, 0.06, -0.09), (0.05, 0.08, 0.06), METAL, hip)                                # heel spur

B((0, 1.32, 0.0), (0.30, 0.14, 0.22), DARK, "sentinel", bev=0.01)                      # pelvis
B((0, 1.37, 0.0), (0.34, 0.04, 0.24), METAL, "sentinel")                               # belt
for s in (1, -1):
    B((s * 0.17, 1.28, 0.02), (0.03, 0.16, 0.18), ARM, "sentinel", rot=(0, 0, s * 6), bev=0.008)   # tassets
B((0, 1.27, 0.10), (0.10, 0.12, 0.03), ARM, "sentinel", bev=0.008)                     # groin plate

T = "torso"
B((0, 1.52, 0.0), (0.26, 0.26, 0.20), DARK, T, bev=0.012)                              # abdomen
B((0, 1.52, 0.10), (0.18, 0.20, 0.025), "sentinel*0.85", T, bev=0.006)
B((0, 1.86, 0.0), (0.44, 0.42, 0.28), ARM, T, bev=0.03, seg=2)                         # chest
B((0, 1.90, 0.145), (0.32, 0.26, 0.03), "sentinel*1.15", T, rot=(-5, 0, 0), bev=0.012) # breastplate
B((0, 1.83, 0.163), (0.20, 0.02, 0.01), "glow*0.3", T, glow="glow", k=1.2)             # charge strip
B((0, 2.10, 0.0), (0.26, 0.06, 0.22), METAL, T, bev=0.01)                              # collar
C((0, 2.14, 0.0), 0.05, 0.08, METAL, T, sides=10)                                      # neck
B((0, 1.85, -0.18), (0.26, 0.34, 0.10), "sentinel*0.6", T, bev=0.02)                   # sensor pack
C((0.10, 2.30, -0.20), 0.012, 0.50, METAL, T, sides=6)                                 # antenna (off the head now)
C((0.10, 2.56, -0.20), 0.03, 0.04, "glow*0.5", T, sides=10, glow="glow", k=2.0)        # its light
C((-0.08, 2.18, -0.20), 0.01, 0.20, METAL, T, sides=6)                                 # a short whip
for s in (1, -1):
    B((s * 0.225, 1.85, 0.0), (0.015, 0.20, 0.16), METAL, T)                           # side vents
    B((s * 0.07, 1.85, -0.232), (0.04, 0.24, 0.012), "glow*0.3", T, glow="glow", k=0.8)   # cells

H = "head"
B((0, 2.29, -0.01), (0.24, 0.30, 0.26), "sentinel*0.85", H, bev=0.05, seg=3)           # helmet
B((0, 2.30, 0.12), (0.20, 0.14, 0.04), METAL, H, bev=0.01)                             # visor frame
C((0, 2.30, 0.14), 0.055, 0.03, METAL, H, rot=(90, 0, 0), sides=16)                    # eye ring
C((0, 2.30, 0.155), 0.04, 0.02, "glow*0.3", H, rot=(90, 0, 0), sides=16, glow="glow", k=3.0)   # cyclops eye
B((0, 2.30, 0.168), (0.03, 0.03, 0.01), "glow*0.6", H, glow="glow", k=4.5)             # its pupil
B((0, 2.45, -0.04), (0.03, 0.06, 0.22), ARM, H, bev=0.008)                             # crest fin
for s in (1, -1):
    B((s * 0.13, 2.27, 0.04), (0.02, 0.08, 0.12), ARM, H)                              # cheek sensors
B((0, 2.18, 0.09), (0.14, 0.05, 0.08), METAL, H, bev=0.008)                            # jaw

for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.29
    B((s * 0.32, 2.10, 0.0), (0.20, 0.08, 0.26), ARM, arm, rot=(0, 0, s * -12), bev=0.03, seg=2)   # pauldron
    B((s * 0.40, 2.06, 0.0), (0.03, 0.10, 0.22), "sentinel*0.85", arm, rot=(0, 0, s * -12), bev=0.008)
    B((x, 1.86, 0.0), (0.10, 0.34, 0.10), DARK, arm, bev=0.01)                         # upper arm
    C((x, 1.66, 0.0), 0.052, 0.11, METAL, arm, rot=(0, 0, 90), sides=10)               # elbow
    B((x, 1.45, 0.0), (0.11, 0.36, 0.12), "sentinel*0.9", arm, bev=0.015)              # forearm
    B((x, 1.24, 0.0), (0.09, 0.09, 0.10), DARK, arm)                                   # hand
B((0.29, 1.18, 0.02), (0.08, 0.05, 0.07), METAL, "armL")                               # fingers

A, gx = "armR", -0.29                                                                  # the rail rifle (down the arm)
B((gx, 1.08, 0.02), (0.09, 0.30, 0.13), METAL, A, bev=0.012)                           # receiver
C((gx, 0.62, 0.03), 0.025, 0.70, "gunmetal*1.4", A, sides=10)                          # barrel
B((gx, 0.86, 0.03), (0.07, 0.28, 0.08), METAL, A, bev=0.01)                            # shroud
B((gx, 0.86, 0.074), (0.02, 0.22, 0.01), "glow*0.3", A, glow="glow", k=0.9)            # its vents
C((gx, 0.25, 0.03), 0.038, 0.07, METAL, A, sides=10)                                   # muzzle brake
C((gx, 1.04, 0.11), 0.03, 0.26, METAL, A, sides=12)                                    # scope
C((gx, 0.905, 0.11), 0.026, 0.012, "glow*0.4", A, sides=12, glow="glow", k=1.2)        # its lens
B((gx, 1.32, -0.03), (0.06, 0.14, 0.08), "sentinel*0.7", A, bev=0.01)                  # stock
save_enemy("sentinel")
