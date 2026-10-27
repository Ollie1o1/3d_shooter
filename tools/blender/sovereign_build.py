# THE SOVEREIGN in detail (python3 tools/blender/live.py tools/blender/sovereign_build.py). The Black King:
# blackened plate edged in gold, a crowned great helm with a burning T-slit and three swept horns, a sunburst on
# his breastplate that beats with his heart, a crimson cape, a halo of twelve gold blades turning behind him, and
# a greatsword with a lit edge and runes that ripple down the fuller. Enraged, his plate cracks open on the fire
# inside (the "enraged" flag) and his cape smoulders.
#   hipL/R (+-0.23, 1.35, 0)  torso (0, 1.57, 0)  head (0, 2.82, 0)  armL/R (+-0.675, 2.67, 0)
#   weapon (-0.675, 1.28, 0): the greatsword, turned on the right fist (sovereignPose's grip)
#   cape (0, 2.77, -0.34), its hem capeTip (0, 1.67, -0.34)   halo (0, 3.072, -0.42)   crown: on the helm
#   head hitbox: x, z within +-0.28 of the head, y 2.73..3.53 (horns and crown points ride "crown": outside it)
# Starts from the saved file's joints; the first build, from a skeleton (tools/enemy_export --joints sovereign
# <file>, its path set as SKELETON before this runs).
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("sovereign", joints=globals().get("SKELETON")); clear_parts()   # (SKELETON: a first build's skeleton)

IRON, IRON2, IRON3 = "sovereign*1.35", "sovereign*1.9", "sovereign*3.0"   # blackened plate: base, raised, a polished edge
DARK, VOID = "sovereign*0.8", "sovereign*0.35"
GOLD, GOLD2 = "gold", "gold*0.75"
GK = (0.17, 0.124, 0.036)          # gold's faint shine
FIRE = "glow*0.3"                  # (his glow: red, or blazing when enraged)
CRIM, CRIM2 = "crimson", "crimson*0.7"
STEEL = "silver"


def rivet(c, parent, r=0.022, mat=GOLD):
    return E(c, (r * 2, r * 2, r * 1.2), mat, parent, seg=8, rings=4)


# ---- legs: mail, cuisse and its lames, a poleyn with a fan and spike, a greave with a ridge and gold bands,
#      sabatons in four lames over a toe cap, a heel and a gold spur ----
for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.23
    C((x, 1.02, 0.0), 0.15, 0.66, DARK, hip, sides=12, r2=0.17)                                    # mail
    P((x, 1.04, 0.06), (0.34, 0.56, 0.26), IRON, hip, top=(1.12, 1.1), bev=0.04, seg=2)             # cuisse
    B((x, 1.04, 0.195), (0.04, 0.48, 0.03), IRON3, hip, bev=0.01)                                  # its ridge
    for i in range(3):
        P((x, 1.33 - i * 0.065, 0.07), (0.38 - i * 0.01, 0.07, 0.30), IRON2, hip, top=(1.04, 1.04), bev=0.02)   # lames
    B((x, 1.205, 0.215), (0.30, 0.018, 0.02), GOLD2, hip)
    P((x + s * 0.17, 1.02, 0.02), (0.04, 0.44, 0.24), IRON, hip, top=(1.0, 1.15), bev=0.015)        # outer plate
    # the knee
    E((x, 0.70, 0.11), (0.27, 0.23, 0.17), IRON2, hip, seg=14, rings=8)                             # poleyn
    R((x, 0.70, 0.17), 0.105, 0.012, GOLD, hip, sides=20, tsides=5, glow="const", k=GK)
    P((x + s * 0.16, 0.70, 0.05), (0.04, 0.28, 0.24), IRON, hip, rot=(0, 0, s * -12), top=(0.7, 0.8), bev=0.012)   # fan
    K((x, 0.70, 0.19), 0.04, 0.13, IRON3, hip, rot=(90, 0, 0), sides=6)                            # spike
    C((x, 0.70, 0.0), 0.14, 0.34, DARK, hip, rot=(0, 0, 90), sides=10)                             # the joint
    # shin
    P((x, 0.38, 0.03), (0.30, 0.56, 0.28), IRON, hip, top=(1.14, 1.16), bev=0.045, seg=2)           # greave
    B((x, 0.40, 0.18), (0.035, 0.46, 0.04), IRON3, hip, bev=0.01)                                  # ridge
    for y in (0.60, 0.17):
        B((x, y, 0.03), (0.335 if y > 0.3 else 0.31, 0.03, 0.31 if y > 0.3 else 0.30), GOLD2, hip)  # bands
    P((x, 0.42, -0.11), (0.26, 0.42, 0.06), IRON2, hip, top=(1.15, 1.0), bev=0.02)                 # calf plate
    for y in (0.52, 0.40, 0.28):
        rivet((x + s * 0.155, y, 0.03), hip)
    # foot
    B((x, 0.025, 0.07), (0.27, 0.05, 0.46), VOID, hip, bev=0.01)                                   # sole
    for i in range(4):
        P((x, 0.135 - i * 0.022, 0.0 + i * 0.075), (0.27 - i * 0.012, 0.075, 0.11), IRON2 if i % 2 == 0 else IRON, hip,
          rot=(12, 0, 0), top=(1.0, 0.9), bev=0.02)                                                # lames
    E((x, 0.06, 0.25), (0.22, 0.10, 0.17), IRON2, hip, seg=12, rings=6)                            # toe cap
    B((x, 0.05, 0.31), (0.16, 0.012, 0.04), GOLD2, hip)
    B((x, 0.09, -0.12), (0.23, 0.14, 0.12), IRON, hip, bev=0.03)                                   # heel
    K((x, 0.10, -0.18), 0.02, 0.10, GOLD, hip, rot=(-90, 0, 0), sides=6)                           # spur
    R((x, 0.10, -0.30), 0.04, 0.007, GOLD, hip, rot=(0, 90, 0), sides=12, tsides=4)                # its rowel

# ---- the hips: mail skirt, a belt with a burning gem, fauld lames front and back, tassets, a crimson tabard ----
RT = "sovereign"   # (the root: rides the body)
B((0, 1.46, 0.0), (0.78, 0.22, 0.48), DARK, RT, bev=0.03)
B((0, 1.565, 0.0), (0.90, 0.10, 0.60), IRON2, RT, bev=0.025)                                       # belt
B((0, 1.565, 0.0), (0.91, 0.022, 0.61), GOLD2, RT)
P((0, 1.565, 0.31), (0.20, 0.15, 0.06), GOLD, RT, top=(1.2, 1.0), bev=0.02, glow="const", k=GK)   # buckle
E((0, 1.565, 0.345), (0.075, 0.075, 0.04), FIRE, RT, glow="glow", k=1.6)                          # its gem
for i in range(3):
    P((0, 1.47 - i * 0.075, 0.25 + i * 0.012), (0.80 + i * 0.04, 0.085, 0.07), IRON if i % 2 else IRON2, RT,
      top=(0.96, 1.0), bev=0.02)                                                                   # fauld
    P((0, 1.47 - i * 0.075, -0.25 - i * 0.012), (0.80 + i * 0.04, 0.085, 0.07), IRON if i % 2 else IRON2, RT,
      top=(0.96, 1.0), bev=0.02)
for s in (1, -1):
    for i in range(2):
        P((s * (0.40 + 0.02 * i), 1.30 - i * 0.16, 0.10 - 0.02 * i), (0.30, 0.22, 0.05), IRON2 if i == 0 else IRON, RT,
          rot=(-10, s * 8, s * 14), top=(1.12, 1.0), bev=0.02)                                     # tassets
        B((s * (0.42 + 0.02 * i), 1.20 - i * 0.16, 0.115 - 0.02 * i), (0.26, 0.02, 0.03), GOLD2, RT, rot=(-10, s * 8, s * 14))
P((0, 1.15, 0.29), (0.34, 0.64, 0.025), CRIM, RT, top=(1.25, 1.0), glow="cape", k=0.6)            # tabard
for s in (1, -1):
    B((s * 0.175, 1.17, 0.30), (0.02, 0.60, 0.02), GOLD2, RT, rot=(0, 0, s * -2.5))
B((0, 0.84, 0.30), (0.34, 0.03, 0.02), GOLD2, RT)
P((0, 1.12, 0.305), (0.12, 0.16, 0.012), GOLD, RT, rot=(0, 0, 180), top=(0.1, 1.0), glow="const", k=GK)   # its crown sign

# ---- the torso: abdomen lames, a breastplate flaring to the shoulders, a sunburst over the heart, a gorget,
#      a standing collar behind the helm, back plate and spine, side lames, cape clasps and the chain between ----
T = "torso"
B((0, 2.15, 0.0), (0.78, 1.12, 0.50), DARK, T)
for i in range(3):
    P((0, 1.67 + i * 0.115, 0.215 + i * 0.01), (0.66 + i * 0.05, 0.12, 0.10), IRON2 if i % 2 else IRON, T,
      top=(1.05, 1.0), bev=0.025)                                                                  # abdomen lames
P((0, 2.40, 0.04), (0.90, 0.80, 0.60), IRON, T, top=(1.3, 1.0), bev=0.08, seg=3)                  # breastplate, flaring
for s in (1, -1):
    P((s * 0.25, 2.45, 0.35), (0.50, 0.58, 0.08), IRON2, T, rot=(-6, s * 10, 0), top=(1.18, 1.0), bev=0.035, seg=2)   # its two halves
    B((s * 0.23, 2.165, 0.40), (0.44, 0.02, 0.025), GOLD2, T, rot=(0, s * 10, s * -4))              # their gold edges
    B((s * 0.49, 2.47, 0.35), (0.02, 0.50, 0.025), GOLD2, T, rot=(0, s * 10, s * 7))
B((0, 2.42, 0.44), (0.05, 0.60, 0.05), IRON3, T, bev=0.012)                                        # the keel
B((0, 2.005, 0.30), (0.80, 0.025, 0.03), GOLD, T, glow="const", k=GK)                              # lower edge
# the sunburst
SB = (0, 2.47, 0.47)
R(SB, 0.13, 0.016, GOLD, T, sides=24, tsides=6, glow="const", k=GK)
R(SB, 0.085, 0.01, GOLD2, T, sides=18, tsides=4)
E((0, 2.47, 0.465), (0.13, 0.13, 0.07), FIRE, T, glow="pulse", seg=16, rings=8)                   # his heart
for i in range(16):
    a = math.radians(i * 22.5 + 90)
    long = i % 2 == 0
    r0 = 0.15
    ln = 0.16 if long else 0.09
    P((math.cos(a) * (r0 + ln / 2), SB[1] + math.sin(a) * (r0 + ln / 2), 0.455), (0.035 if long else 0.025, ln, 0.02),
      GOLD if long else GOLD2, T, rot=(0, 0, i * 22.5), top=(0.15, 1.0), glow="pulse" if i % 4 == 0 else None, k=0.12)
# gorget, collar
P((0, 2.78, 0.0), (0.66, 0.10, 0.52), IRON2, T, top=(0.86, 0.86), bev=0.03)
P((0, 2.86, 0.0), (0.56, 0.08, 0.44), IRON, T, top=(0.86, 0.86), bev=0.025)
B((0, 2.735, 0.0), (0.67, 0.02, 0.53), GOLD2, T)
P((0, 2.98, -0.24), (0.80, 0.36, 0.07), IRON, T, rot=(-16, 0, 0), top=(1.3, 1.0), bev=0.025)       # standing collar
B((0, 3.155, -0.29), (1.02, 0.025, 0.03), GOLD, T, rot=(-16, 0, 0), glow="const", k=GK)
for s in (1, -1):
    K((s * 0.50, 3.15, -0.29), 0.03, 0.14, GOLD2, T, rot=(-16, 0, s * -30), sides=6)
# back
P((0, 2.32, -0.29), (0.98, 0.92, 0.08), IRON, T, top=(1.1, 1.0), bev=0.03)
for i in range(6):
    E((0, 1.80 + i * 0.16, -0.335), (0.07, 0.09, 0.05), IRON3, T, seg=8, rings=4)                  # spine
for s in (1, -1):
    for i in range(3):
        P((s * 0.46, 1.86 + i * 0.13, 0.0), (0.06, 0.12, 0.48), IRON2 if i % 2 else IRON, T, top=(1.0, 1.06), bev=0.015)   # side lames
    E((s * 0.40, 2.70, 0.25), (0.14, 0.14, 0.06), GOLD, T, seg=14, rings=6, glow="const", k=GK)    # cape clasps
    E((s * 0.40, 2.70, 0.28), (0.06, 0.06, 0.03), FIRE, T, glow="glow", k=1.2)
for i in range(7):   # the chain between them
    u = (i - 3) / 3.5
    R((u * 0.33, 2.64 - 0.07 * (1 - u * u), 0.29), 0.028, 0.007, GOLD2, T, rot=(0, 90 if i % 2 else 0, 0), sides=10, tsides=4)
# enraged: the plate splits on the fire inside
for (cx, cy, ln, a) in ((0.18, 2.20, 0.30, 25), (-0.22, 2.32, 0.26, -35), (0.30, 2.55, 0.18, -60), (-0.12, 1.86, 0.20, 70),
                        (0.08, 2.62, 0.14, 15), (-0.35, 2.12, 0.16, 50)):
    for j in range(3):   # each crack in three jagged runs
        aj = a + (-28, 22, -12)[j]
        off = (j - 1) * ln / 3
        px = cx - math.sin(math.radians(a)) * off
        py = cy + math.cos(math.radians(a)) * off
        z = 0.445 + (0.25 - abs(px)) * 0.176 if py > 2.1 else 0.31
        show(B((px, py, z), (0.022 - 0.004 * j, ln / 3 + 0.02, 0.02), FIRE, T, rot=(0, 10 if px > 0 else -10, aj), glow="glow", k=2.0),
             "enraged")

# ---- the helm: a dome, a faceplate with a burning T-slit, a brow, cheek plates, breaths, a bevor, a crest,
#      a gold crown band; on the crown joint, nine gold points and three horns swept back ----
H = "head"
E((0, 3.07, -0.01), (0.44, 0.50, 0.48), IRON, H, seg=20, rings=12)                                 # dome
P((0, 3.01, 0.12), (0.40, 0.40, 0.24), IRON2, H, top=(1.04, 0.9), bev=0.05, seg=2)                 # faceplate
B((0, 3.075, 0.236), (0.34, 0.075, 0.02), VOID, H)                                                 # slit recess
B((0, 3.075, 0.247), (0.30, 0.036, 0.012), FIRE, H, glow="glow", k=2.6)                            # the T
B((0, 2.985, 0.247), (0.048, 0.15, 0.012), FIRE, H, glow="glow", k=2.6)
for s in (1, -1):
    P((s * 0.105, 3.14, 0.215), (0.22, 0.05, 0.08), IRON3, H, rot=(0, 0, s * -9), bev=0.012)       # brow
    P((s * 0.19, 2.97, 0.12), (0.05, 0.26, 0.22), IRON, H, rot=(0, s * -15, 0), top=(1.0, 1.1), bev=0.015)   # cheek plates
    for i in range(3):
        E((s * (0.07 + 0.035 * i), 2.90, 0.236), (0.022, 0.05, 0.012), VOID, H, seg=6, rings=4)     # breaths
P((0, 2.86, 0.15), (0.28, 0.10, 0.14), IRON2, H, top=(1.2, 1.0), bev=0.025)                        # bevor
P((0, 3.33, -0.02), (0.05, 0.07, 0.40), IRON3, H, bev=0.015)                                       # crest
R((0, 3.215, -0.01), 0.232, 0.022, GOLD, H, rot=(90, 0, 0), sides=28, tsides=6, glow="const", k=GK)   # crown band
for i in range(10):
    a = math.radians(i * 36)
    E((math.sin(a) * 0.235, 3.215, math.cos(a) * 0.235 - 0.01), (0.04, 0.04, 0.04), FIRE if i % 5 == 0 else GOLD2, H,
      seg=8, rings=4, glow="glow" if i % 5 == 0 else None, k=1.4)                                 # set stones
C((0, 2.80, 0.0), 0.15, 0.10, DARK, H, sides=12)                                                   # neck
CR = "crown"
for i in range(9):   # the crown's points: tallest at the front
    a = math.radians(-80 + i * 20)
    h = 0.20 - abs(i - 4) * 0.028
    bx, bz = math.sin(a) * 0.235, math.cos(a) * 0.235 - 0.01
    K((bx, 3.235, bz), 0.026, h, GOLD, CR, rot=(math.degrees(math.cos(a)) * 0.25, 0, -math.degrees(math.sin(a)) * 0.25), sides=6,
      glow="const", k=GK)
E((0, 3.47, 0.23), (0.05, 0.07, 0.03), FIRE, CR, glow="glow", k=2.0)                               # the jewel on its tallest point
tube([(0, 3.36, -0.06), (0, 3.50, -0.18), (0, 3.57, -0.34), (0, 3.56, -0.48)], [0.055, 0.045, 0.032, 0.02], GOLD, CR,
     sides=10, tip=0.14)                                                                           # the centre horn
for s in (1, -1):
    tube([(s * 0.20, 3.18, -0.04), (s * 0.34, 3.30, -0.13), (s * 0.45, 3.43, -0.28), (s * 0.50, 3.50, -0.44)],
         [0.06, 0.05, 0.036, 0.022], GOLD2, CR, sides=10, tip=0.18)                                # the side horns
    for j, (cx, cy, cz) in enumerate(((s * 0.30, 3.27, -0.11), (s * 0.40, 3.37, -0.24))):
        R((cx, cy, cz), 0.05 - 0.008 * j, 0.008, GOLD, CR, rot=(60, s * -40, 0), sides=12, tsides=4)   # their bands

# ---- arms: a great layered pauldron (the left spiked, the right with a gold sun), a rerebrace, a couter with a
#      fan and a back spike, a vambrace with gold bands, a flared gauntlet cuff; the left fist ----
for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 0.675
    E((s * 0.74, 2.80, 0.0), (0.56, 0.32, 0.70), IRON2, arm, seg=18, rings=10)                     # pauldron dome
    for i in range(3):
        P((s * (0.80 + 0.035 * i), 2.67 - i * 0.10, 0.0), (0.46 - 0.01 * i, 0.12, 0.74 - 0.04 * i), IRON if i % 2 else IRON2, arm,
          rot=(0, 0, s * -(20 + 7 * i)), top=(1.0, 1.0), bev=0.03, seg=2)                         # its lames
        B((s * (0.82 + 0.035 * i), 2.615 - i * 0.10, 0.0), (0.44 - 0.01 * i, 0.02, 0.75 - 0.04 * i), GOLD2, arm,
          rot=(0, 0, s * -(20 + 7 * i)))
    B((s * 0.74, 2.965, 0.0), (0.06, 0.035, 0.58), GOLD, arm, glow="const", k=GK)                  # gold ridge
    if s > 0:   # the left: three spikes
        for j, z in enumerate((-0.18, 0.0, 0.18)):
            K((s * 0.74, 2.95, z), 0.05, 0.24 + (0.08 if j == 1 else 0.0), IRON3, arm, rot=(z * 60, 0, -25), sides=6)
    else:       # the right: the sun of his house
        R((s * 1.03, 2.80, 0.0), 0.09, 0.014, GOLD, arm, rot=(0, 90, 0), sides=18, tsides=5, glow="const", k=GK)
        E((s * 1.035, 2.80, 0.0), (0.03, 0.09, 0.09), FIRE, arm, glow="pulse", k=0.6)
        for i in range(8):
            a = i * 45
            P((s * 1.04, 2.80 + math.cos(math.radians(a)) * 0.15, math.sin(math.radians(a)) * 0.15), (0.015, 0.08, 0.02),
              GOLD2, arm, rot=(a, 0, 0), top=(0.2, 1.0))
    C((x, 2.25, 0.0), 0.14, 0.52, DARK, arm, sides=12, r2=0.15)                                    # mail
    P((x, 2.25, 0.02), (0.29, 0.46, 0.28), IRON, arm, top=(1.08, 1.08), bev=0.035, seg=2)          # rerebrace
    E((x, 1.94, -0.02), (0.25, 0.22, 0.25), IRON2, arm, seg=14, rings=8)                           # couter
    P((x + s * 0.13, 1.94, 0.02), (0.04, 0.24, 0.26), IRON, arm, rot=(0, 0, s * 10), top=(0.7, 0.8), bev=0.012)   # its fan
    K((x, 1.94, -0.13), 0.04, 0.15, IRON3, arm, rot=(-100, 0, 0), sides=6)                         # elbow spike
    P((x, 1.66, 0.0), (0.24, 0.46, 0.24), IRON, arm, top=(1.18, 1.18), bev=0.035, seg=2)           # vambrace
    B((x, 1.66, 0.125), (0.035, 0.40, 0.03), IRON3, arm, bev=0.008)
    for y in (1.82, 1.52):
        B((x, y, 0.0), (0.27 if y > 1.6 else 0.25, 0.028, 0.27 if y > 1.6 else 0.25), GOLD2, arm)
    P((x, 1.41, 0.0), (0.24, 0.13, 0.24), IRON2, arm, top=(1.3, 1.3), bev=0.025)                   # cuff
    B((x, 1.475, 0.0), (0.315, 0.02, 0.315), GOLD, arm, glow="const", k=GK)
L = "armL"   # the left fist
P((0.675, 1.26, 0.0), (0.18, 0.22, 0.19), IRON2, L, top=(1.05, 1.0), bev=0.04, seg=2)
for i in range(4):
    B((0.675, 1.335 - i * 0.055, 0.10), (0.17, 0.045, 0.035), IRON3 if i % 2 else IRON, L, bev=0.01)   # finger lames
    rivet((0.675 + 0.065, 1.335 - i * 0.055, 0.115), L, r=0.012)
P((0.675 - 0.08, 1.30, 0.05), (0.06, 0.12, 0.08), IRON, L, rot=(0, 0, -20), bev=0.015)             # thumb

# ---- the greatsword (on the weapon joint): pommel and its gem, a wrapped grip, the right fist round it, a gold
#      guard with swept quillons and a gem, a ricasso, the blade with a dark fuller, lit edges, a point, and runes ----
SW, x0 = "weapon", -0.675
E((x0, 1.60, 0.0), (0.14, 0.16, 0.14), GOLD, SW, seg=12, rings=8, glow="const", k=GK)              # pommel
E((x0, 1.60, 0.0), (0.075, 0.075, 0.16), FIRE, SW, glow="pulse", k=0.7)
R((x0, 1.60, 0.0), 0.085, 0.012, GOLD2, SW, rot=(0, 90, 0), sides=14, tsides=4)
K((x0, 1.67, 0.0), 0.03, 0.08, GOLD, SW, sides=6)
C((x0, 1.30, 0.0), 0.045, 0.46, VOID, SW, sides=8)                                                 # grip
for i in range(6):
    R((x0, 1.12 + i * 0.07, 0.0), 0.05, 0.009, GOLD2, SW, rot=(90, 0, 15), sides=10, tsides=4)
P((x0, 1.27, 0.0), (0.18, 0.22, 0.19), IRON2, SW, top=(1.05, 1.0), bev=0.04, seg=2)                # the right fist
for i in range(4):
    B((x0, 1.345 - i * 0.055, 0.10), (0.17, 0.045, 0.035), IRON3 if i % 2 else IRON, SW, bev=0.01)
    rivet((x0 - 0.065, 1.345 - i * 0.055, 0.115), SW, r=0.012)
P((x0 + 0.08, 1.31, 0.05), (0.06, 0.12, 0.08), IRON, SW, rot=(0, 0, 20), bev=0.015)
P((x0, 1.07, 0.0), (0.18, 0.10, 0.13), GOLD, SW, top=(0.8, 0.9), bev=0.025, glow="const", k=GK)    # the guard
E((x0, 1.07, 0.065), (0.06, 0.06, 0.03), FIRE, SW, glow="pulse", k=0.8)
E((x0, 1.07, -0.065), (0.06, 0.06, 0.03), FIRE, SW, glow="pulse", k=0.8)
for d in (1, -1):
    P((x0 + d * 0.21, 1.055, 0.0), (0.28, 0.05, 0.07), GOLD2, SW, rot=(0, 0, -d * 14), bev=0.012)  # quillons
    K((x0 + d * 0.34, 1.02, 0.0), 0.032, 0.12, GOLD, SW, rot=(0, 0, -d * 115), sides=6)            # their points
    P((x0 + d * 0.12, 1.12, 0.0), (0.12, 0.04, 0.05), GOLD, SW, rot=(0, 0, d * 30), top=(0.4, 1.0)) # wings
B((x0, 0.95, 0.0), (0.13, 0.16, 0.05), STEEL + "*0.75", SW, bev=0.01)                             # ricasso
for d in (1, -1):
    P((x0, 0.96, d * 0.03), (0.07, 0.14, 0.012), GOLD2, SW, rot=(0, 0, 180), top=(0.2, 1.0))       # langets
P((x0, -0.20, 0.0), (0.20, 2.14, 0.045), STEEL, SW, rot=(0, 0, 180), top=(0.6, 0.7), bev=0.008)    # the blade
K((x0, -1.27, 0.0), 0.06, 0.24, STEEL, SW, rot=(180, 0, 0), sides=4, squash=0.35)                  # its point
B((x0, -0.02, 0.0), (0.045, 1.66, 0.05), "sovereign*0.6", SW)                                     # fuller
for d in (1, -1):
    a = math.degrees(math.atan2(0.04, 2.14))
    B((x0 + d * 0.081, -0.20, 0.0), (0.016, 2.12, 0.02), STEEL + "*1.5", SW, rot=(0, 0, -d * a), glow="edge", k=0.8)   # lit edges
for i in range(7):   # runes down the fuller, rippling from the hilt
    y = 0.70 - i * 0.22
    for z in (0.026, -0.026):
        B((x0, y, z), (0.022, 0.10, 0.006), FIRE, SW, glow="edge", k=1.1, phase=-i * 0.9)
        B((x0, y + (0.025 if i % 2 else -0.025), z), (0.05, 0.016, 0.006), FIRE, SW, glow="edge", k=1.1, phase=-i * 0.9)

# ---- the cape: crimson, gold-edged at the shoulders, folds down its back, a gold sigil; the hem lags it, its edge
#      tattered over a gold band. Enraged, it smoulders ----
CA, CT = "cape", "capeTip"
P((0, 2.21, -0.37), (1.10, 1.20, 0.035), CRIM, CA, top=(0.82, 1.0), glow="cape")                  # upper
P((0, 2.21, -0.35), (1.04, 1.16, 0.02), VOID, CA, top=(0.82, 1.0))                                 # lining
B((0, 2.80, -0.36), (0.94, 0.07, 0.07), GOLD2, CA, bev=0.015)                                      # mantle edge
for xf in (-0.38, -0.13, 0.13, 0.38):
    P((xf, 2.17, -0.395), (0.09, 1.08, 0.035), CRIM2, CA, top=(0.6, 1.0), glow="cape", k=0.7)     # folds
R((0, 2.15, -0.395), 0.17, 0.016, GOLD, CA, sides=24, tsides=5, glow="const", k=GK)                # the sigil
for j, a in enumerate((-30, 0, 30)):
    K((math.sin(math.radians(a)) * 0.05, 2.17, -0.40), 0.03, 0.15 if j == 1 else 0.11, GOLD, CA, rot=(0, 0, -a), sides=4, squash=0.3)
B((0, 2.105, -0.40), (0.16, 0.02, 0.012), GOLD, CA)
P((0, 0.99, -0.37), (1.15, 1.28, 0.035), CRIM, CT, top=(0.96, 1.0), glow="cape")                  # hem
P((0, 0.99, -0.35), (1.10, 1.24, 0.02), VOID, CT, top=(0.96, 1.0))
for xf in (-0.40, -0.14, 0.14, 0.40):
    P((xf, 1.02, -0.395), (0.09, 1.18, 0.035), CRIM2, CT, top=(1.0, 1.0), glow="cape", k=0.7)
B((0, 0.42, -0.395), (1.15, 0.04, 0.02), GOLD2, CT)
for i in range(9):   # tattered
    xf = -0.51 + i * 0.1275
    ln = 0.10 + 0.08 * ((i * 7) % 3) / 2
    P((xf, 0.40 - ln / 2, -0.37), (0.12, ln, 0.03), CRIM, CT, rot=(0, 0, 180), top=(0.15, 1.0), glow="cape")

# ---- the halo: twelve gold blades and twelve beads turning behind his helm, an inner ring, a fine outer ring ----
HA, hc = "halo", (0, 3.072, -0.78)   # (the joint is at z -0.42: it turns about its own z, so the ring can sit further back)
R(hc, 0.50, 0.02, GOLD, HA, sides=40, tsides=6, glow="halo", k=0.9)
R(hc, 0.86, 0.008, GOLD2, HA, sides=48, tsides=4, glow="halo", k=0.35)
for i in range(12):
    a = math.radians(i * 30 + 90)
    ln = 0.30 if i % 3 == 0 else 0.20
    r0 = 0.53
    P((math.cos(a) * (r0 + ln / 2), hc[1] + math.sin(a) * (r0 + ln / 2), hc[2]), (0.075, ln, 0.025), GOLD, HA,
      rot=(0, 0, i * 30), top=(0.12, 1.0), glow="halo", k=0.6)
    b = a + math.radians(15)
    E((math.cos(b) * 0.66, hc[1] + math.sin(b) * 0.66, hc[2]), (0.06, 0.06, 0.04), GOLD, HA, seg=8, rings=4, glow="halo", k=1.1)

report("sovereign")
save_enemy("sovereign")
