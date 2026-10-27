# The JUGGERNAUT in detail (python3 tools/blender/live.py tools/blender/juggernaut_build.py). A siege mech in steel
# and gold: a throbbing furnace core, a cannon arm that heats as it charges a shell, a hammer fist lit as it winds up.
#   hipL/R (+-0.325, 1.15, 0)  torso (0, 1.4, 0)  head (0, 2.7, 0)  armL/R (+-0.975, 2.475, 0)
#   head hitbox: x, z within +-0.39 of the head, y 2.62..3.35
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("juggernaut"); clear_parts()
ARM, DARK, METAL, GOLD = "juggernaut", "gunmetal*1.3", "gunmetal", "gold"
GOLD_K = (0.17, 0.124, 0.036)   # gold * 0.2: its trim's faint shine

for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.325
    B((x, 0.88, 0.0), (0.46, 0.50, 0.50), DARK, hip, bev=0.04)                         # thigh
    B((x, 0.90, 0.26), (0.42, 0.40, 0.06), ARM, hip, bev=0.025)                        # thigh plate
    B((x + s * 0.24, 0.90, 0.0), (0.05, 0.36, 0.40), "juggernaut*0.85", hip, bev=0.015)
    C((x, 0.60, 0.0), 0.20, 0.52, METAL, hip, rot=(0, 0, 90), sides=12)                # knee
    B((x, 0.62, 0.25), (0.36, 0.24, 0.14), "juggernaut*1.15", hip, bev=0.05, seg=2)    # knee cap
    B((x, 0.33, 0.0), (0.44, 0.44, 0.48), "juggernaut*0.85", hip, bev=0.04)            # shin
    B((x, 0.35, 0.25), (0.40, 0.34, 0.06), ARM, hip, bev=0.025)                        # greave
    B((x, 0.52, 0.285), (0.32, 0.03, 0.02), GOLD, hip)                                 # gold trim
    B((x, 0.08, 0.08), (0.56, 0.16, 0.72), "juggernaut*0.7", hip, bev=0.04)            # foot
    W((x, 0.10, 0.48), (0.50, 0.12, 0.16), ARM, hip, taper=0.4)                        # toe plate
    B((x, 0.10, -0.30), (0.40, 0.16, 0.16), METAL, hip, bev=0.02)                      # heel

B((0, 1.27, 0.0), (1.00, 0.25, 0.70), DARK, "juggernaut", bev=0.04)                    # pelvis
B((0, 1.40, 0.0), (1.10, 0.08, 0.76), METAL, "juggernaut", bev=0.02)                   # belt
B((0, 1.40, 0.385), (0.22, 0.07, 0.02), GOLD, "juggernaut", glow="const", k=GOLD_K)    # buckle
B((0, 1.20, 0.36), (0.34, 0.26, 0.08), ARM, "juggernaut", bev=0.03)                    # groin plate
for s in (1, -1):
    B((s * 0.58, 1.20, 0.04), (0.08, 0.34, 0.60), ARM, "juggernaut", rot=(0, 0, s * 10), bev=0.025)   # tassets

T = "torso"
B((0, 1.65, 0.0), (1.10, 0.50, 0.75), DARK, T, bev=0.05)                               # gut
B((0, 1.56, 0.38), (0.80, 0.14, 0.06), "juggernaut*0.85", T, bev=0.02)
B((0, 1.74, 0.385), (0.86, 0.14, 0.06), "juggernaut*0.85", T, bev=0.02)
B((0, 2.25, 0.0), (1.50, 0.90, 0.95), ARM, T, bev=0.08, seg=2)                         # chest
B((0, 2.10, 0.50), (1.20, 0.90, 0.10), "juggernaut*1.15", T, bev=0.04, seg=2)          # chest plate
C((0, 2.12, 0.56), 0.26, 0.06, METAL, T, rot=(90, 0, 0), sides=20)                     # core housing
C((0, 2.12, 0.585), 0.20, 0.04, "glow*0.3", T, rot=(90, 0, 0), sides=20, glow="pulse", k=1.0)   # the core
for s in (1, -1):
    B((s * 0.45, 2.12, 0.555), (0.06, 0.50, 0.03), GOLD, T)                            # gold bars
B((0, 2.68, 0.0), (0.80, 0.12, 0.70), METAL, T, bev=0.03)                              # collar
B((0, 2.30, -0.60), (0.90, 0.90, 0.30), "gunmetal*1.4", T, bev=0.05, seg=2)            # back reactor
for x in (-0.25, 0.0, 0.25):
    B((x, 2.30, -0.755), (0.12, 0.60, 0.02), "glow*0.3", T, glow="glow", k=0.9)        # its vents
for s in (1, -1):
    C((s * 0.32, 2.85, -0.60), 0.10, 0.40, METAL, T, sides=12)                         # stacks

H = "head"
B((0, 3.00, 0.0), (0.62, 0.55, 0.62), "juggernaut*1.2", H, bev=0.08, seg=3)            # helmet
B((0, 3.03, 0.30), (0.50, 0.12, 0.04), METAL, H)                                       # visor recess
B((0, 3.03, 0.322), (0.46, 0.07, 0.02), "glow*0.3", H, glow="glow", k=2.5)             # visor
B((0, 3.12, 0.29), (0.60, 0.07, 0.08), "juggernaut*1.3", H, bev=0.02)                  # brow
B((0, 3.30, -0.02), (0.12, 0.08, 0.50), GOLD, H, bev=0.02)                             # crest
for s in (1, -1):
    B((s * 0.33, 2.92, 0.08), (0.05, 0.30, 0.40), ARM, H, bev=0.015)                   # cheek guards
B((0, 2.80, 0.26), (0.30, 0.12, 0.10), METAL, H, bev=0.02)                             # chin

for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.975
    B((s * 0.98, 2.60, 0.0), (0.70, 0.50, 1.05), "juggernaut*1.3", arm, rot=(0, 0, s * -8), bev=0.07, seg=2)   # shoulder slab
    B((s * 0.98, 2.87, 0.0), (0.60, 0.06, 0.90), GOLD, arm, rot=(0, 0, s * -8), glow="const", k=GOLD_K)
    B((s * 1.32, 2.45, 0.0), (0.08, 0.36, 1.00), "juggernaut*0.9", arm, rot=(0, 0, s * -8), bev=0.02)
    B((x, 2.05, 0.0), (0.40, 0.60, 0.42), DARK, arm, bev=0.03)                         # upper arm
    C((x, 1.70, 0.0), 0.20, 0.46, METAL, arm, rot=(0, 0, 90), sides=12)                # elbow

L = "armL"                                                                             # the hammer
B((0.975, 1.40, 0.0), (0.50, 0.50, 0.52), "juggernaut*0.9", L, bev=0.04)               # forearm
B((0.975, 0.925, 0.0), (0.85, 0.70, 0.85), "gold*0.75", L, glow="smash", k=1.0, bev=0.06, seg=2)   # hammer fist
B((0.975, 0.585, 0.0), (0.78, 0.05, 0.78), METAL, L)                                   # its face
B((0.975, 0.93, 0.43), (0.80, 0.12, 0.04), METAL, L, bev=0.01)                         # knuckle band

R, gx = "armR", -0.975                                                                 # the cannon
C((gx, 1.025, 0.0), 0.30, 1.00, "gunmetal*0.9", R, sides=16)                           # barrel
C((gx, 1.275, 0.0), 0.34, 0.08, "gold*0.7", R, sides=16)                               # gold rings
C((gx, 0.850, 0.0), 0.34, 0.06, "gold*0.7", R, sides=16)
C((gx, 0.495, 0.0), 0.24, 0.12, "gunmetal*0.6", R, sides=16, glow="shot", k=1.0)       # muzzle: heats as it charges
C((gx, 0.430, 0.0), 0.15, 0.02, "gunmetal*0.3", R, sides=16, glow="shot", k=1.5)       # bore
for s in (1, -1):
    B((gx + s * 0.31, 1.05, 0.0), (0.02, 0.50, 0.12), "glow*0.3", R, glow="shot", k=0.5)   # vents
save_enemy("juggernaut")
