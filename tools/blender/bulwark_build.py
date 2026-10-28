# THE BULWARK in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/bulwark_build.py).
# The Sluice's mini-boss, on the SHIELDBEARER's skeleton (drawn x1.7): a lock-keeper in verdigris plate behind a sluice
# gate for a shield - riveted steel, bars down its face, a valve wheel at its centre, a lit seam along its top and
# bottom ("rim": brightens through a wind-up) and weed and rust at its foot. A pump cannon on its right arm fires fans.
#   hipL/R (+-0.15, 0.95, 0)  torso (0, 1.11, 0)  head (0, 1.81, 0)  armL/R (+-0.405, 1.715, 0)  shield (0.12, 0.96, 0.5)
#   head hitbox: x, z within +-0.22 of the head, y 1.75..2.21
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
import os
open_enemy("bulwark", joints=os.path.join(REPO, "assets", "enemies", BASE_FILE["bulwark"] + ".glb")); clear_parts()
PLATE, PLATE2, DARK, METAL, GREEN, BRONZE = "bulwark", "bulwark*1.25", "gunmetal*1.3", "gunmetal", "verdigris", "bronze"
RT = "shieldbearer"   # (the root it came in on)

for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.15
    B((x, 0.74, 0.0), (0.22, 0.40, 0.24), DARK, hip, bev=0.015)                         # thigh
    B((x, 0.76, 0.12), (0.20, 0.34, 0.04), GREEN, hip, bev=0.012)                       # thigh plate
    C((x, 0.52, 0.0), 0.10, 0.24, METAL, hip, rot=(0, 0, 90), sides=12)                 # knee
    B((x, 0.53, 0.12), (0.18, 0.14, 0.08), PLATE2, hip, bev=0.02, seg=2)                # knee guard
    B((x, 0.30, 0.0), (0.20, 0.36, 0.22), DARK, hip, bev=0.015)                         # shin
    B((x, 0.30, 0.115), (0.19, 0.32, 0.05), GREEN, hip, bev=0.012)                      # greave
    B((x, 0.05, 0.05), (0.26, 0.10, 0.36), "bulwark*0.7", hip, bev=0.015)               # boot
    W((x, 0.05, 0.26), (0.24, 0.08, 0.08), PLATE, hip, taper=0.4)                       # toe cap

B((0, 1.03, 0.0), (0.54, 0.18, 0.32), DARK, RT, bev=0.015)                              # pelvis
B((0, 1.11, 0.0), (0.60, 0.06, 0.36), BRONZE, RT)                                       # belt
for s in (1, -1):
    B((s * 0.30, 1.00, 0.03), (0.05, 0.22, 0.28), GREEN, RT, rot=(0, 0, s * 8), bev=0.01)   # tassets

T = "torso"
B((0, 1.25, 0.0), (0.46, 0.28, 0.30), DARK, T, bev=0.02)                                # gut
B((0, 1.58, 0.0), (0.68, 0.50, 0.40), PLATE, T, bev=0.045, seg=2)                       # chest
B((0, 1.60, 0.20), (0.52, 0.36, 0.05), GREEN, T, bev=0.015)                             # breastplate
for y in (1.50, 1.68):
    B((0, y, 0.228), (0.40, 0.025, 0.01), "glow*0.3", T, glow="glow", k=1.0)            # lit gauge lines
B((0, 1.80, 0.0), (0.40, 0.07, 0.32), METAL, T, bev=0.01)                               # collar
B((0, 1.55, -0.24), (0.46, 0.40, 0.12), "bulwark*0.6", T, bev=0.02)                     # pump pack
for s in (1, -1):
    C((s * 0.14, 1.55, -0.32), 0.06, 0.38, "rust", T, sides=10)                         # its tanks

H = "head"
B((0, 1.97, -0.01), (0.30, 0.30, 0.32), PLATE, H, bev=0.06, seg=3)                      # diving helm
for k in range(3):
    B((0, 1.93 + 0.05 * k, 0.16), (0.20, 0.018, 0.02), METAL, H)                        # its grille
B((0, 1.98, 0.168), (0.18, 0.12, 0.01), "glow*0.3", H, glow="glow", k=1.6)              # light behind the grille
B((0, 2.17, -0.02), (0.06, 0.06, 0.30), BRONZE, H, bev=0.01)                            # crest
for s in (1, -1):
    C((s * 0.16, 1.98, 0.0), 0.05, 0.03, BRONZE, H, rot=(0, 0, 90), sides=10)          # bolts at the temples

for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.405
    B((s * 0.46, 1.77, 0.0), (0.26, 0.12, 0.34), PLATE2, arm, rot=(0, 0, s * -12), bev=0.035, seg=2)   # pauldron
    B((x, 1.52, 0.0), (0.17, 0.30, 0.18), DARK, arm, bev=0.012)                         # upper arm
    C((x, 1.33, 0.0), 0.08, 0.19, METAL, arm, rot=(0, 0, 90), sides=10)                 # elbow
    B((x, 1.17, 0.0), (0.19, 0.28, 0.20), GREEN, arm, bev=0.015)                        # forearm
    B((x, 0.98, 0.0), (0.15, 0.12, 0.15), DARK, arm)                                    # hand
R_, gx = "armR", -0.405                                                                 # the pump cannon
C((gx, 0.92, 0.06), 0.11, 0.34, METAL, R_, sides=14)
C((gx, 0.73, 0.06), 0.085, 0.06, "rust", R_, sides=14, glow="shot", k=1.0)              # muzzle
R((gx, 1.0, 0.06), 0.12, 0.02, BRONZE, R_, rot=(90, 0, 0), sides=16, tsides=4)

S = "shield"   # the sluice gate
B((0.12, 0.98, 0.50), (1.25, 1.90, 0.14), "bulwark*0.9", S, bev=0.03, seg=2)           # the gate
B((0.12, 0.98, 0.58), (1.08, 1.70, 0.03), "rust", S, bev=0.01)                          # its face, rusted
for dx in (-0.42, -0.14, 0.14, 0.42):
    B((0.12 + dx, 0.98, 0.61), (0.07, 1.74, 0.05), METAL, S, bev=0.01)                  # bars
for y in (1.91, 0.05):
    B((0.12, y, 0.54), (1.28, 0.08, 0.18), BRONZE, S, glow="rim", k=1.0, bev=0.015)     # lit seams: brighten in a wind-up
B((0.12, 0.30, 0.62), (1.10, 0.18, 0.02), GREEN, S)                                     # weed line at its foot
R((0.12, 1.08, 0.66), 0.24, 0.03, BRONZE, S, sides=24, tsides=6)                        # valve wheel
for a in (0, 60, 120):
    B((0.12, 1.08, 0.66), (0.46, 0.035, 0.03), BRONZE, S, rot=(0, 0, a))
C((0.12, 1.08, 0.67), 0.06, 0.05, "glow*0.4", S, rot=(90, 0, 0), sides=12, glow="glow", k=1.4)   # its hub
for dx in (-0.55, 0.55):
    for dy in (-0.85, -0.3, 0.3, 0.85):
        C((0.12 + dx, 0.98 + dy, 0.60), 0.03, 0.04, METAL, S, rot=(90, 0, 0), sides=8)  # rivets
B((0.12, 0.98, 0.40), (0.30, 0.22, 0.06), METAL, S)                                     # grip
report("bulwark")
save_enemy("bulwark")
