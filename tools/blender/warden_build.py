# THE WARDEN in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/warden_build.py,
# or over the live link). The Core's jailer: a 4.6 m reactor colossus in violet-grey plate trimmed in gold, its head
# sunk low between two great pauldrons, each crowned with a conduit pylon like the four it feeds from. Its heart is
# a reactor behind an iris of six plates that draw back when it vents ("open"): the weak point, white-hot. A
# reactor stack on its back breathes through two cooling towers; a split cape hangs either side of it. In meltdown
# ("enraged") its plate cracks open on the fire inside.
#   hipL/R (+-0.4, 1.7, 0)  torso (0, 2.0, 0)  head (0, 3.75, 0)  armL/R (+-1.225, 3.475, 0): arms hang down -Y
#   cape (0, 3.6, -0.63), its hem capeTip (0, 1.9, -0.63)   crown (0, 3.75, 0): rides the head
#   head hitbox: x, z within +-0.39 of the head, y 3.64..4.81 (the crown's points ride "crown")
#   the core (coreBox): the chest's front at y ~2.96, z 0.55
# Starts from the saved file's joints; the first build, from a skeleton (SKELETON=<tools/enemy_export --joints warden>).
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("warden", joints=globals().get("SKELETON")); clear_parts()

PLATE, PLATE2, EDGE = "warden*1.25", "warden*1.65", "warden*2.3"   # violet-grey plate: base, raised, a worn edge
DARK, VOID = "warden*0.55", "warden*0.22"
GOLD, GOLD2 = "gold", "gold*0.7"
GK = (0.15, 0.11, 0.03)              # gold's faint shine
STEEL, CABLE = "gunmetal*1.7", "gunmetal*0.9"
LINE = "glow*0.3"                    # (a conduit line: glow "pulse")
HOT = "glow*0.5"
RT = "warden"                        # (the root: rides the body's bob)


def rivet(c, parent, r=0.03, mat=GOLD):
    return E(c, (r * 2, r * 2, r * 1.2), mat, parent, seg=8, rings=4)


def crack(c, s, parent, rot=(0, 0, 0)):
    """A seam of fire through the plate, shown only in meltdown"""
    return B(c, s, HOT, parent, rot=rot, glow="core", k=0.9, show="enraged")


# ---- legs: a mail sleeve, a thigh plate with lames and a conduit line, a great knee cop with a lit stud,
#      a flared greave, sabatons of three lames, three claws and a heel spike ----
for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.4
    C((x, 1.30, 0.0), 0.27, 0.80, DARK, hip, sides=14, r2=0.30)                                   # mail sleeve
    P((x, 1.34, 0.06), (0.62, 0.78, 0.50), PLATE, hip, top=(1.12, 1.08), bev=0.06, seg=2)          # thigh plate
    B((x, 1.32, 0.315), (0.07, 0.66, 0.04), EDGE, hip, bev=0.015)                                 # its ridge
    B((x + s * 0.17, 1.32, 0.30), (0.04, 0.6, 0.03), LINE, hip, glow="pulse", k=1.0)               # conduit line
    for i in range(3):
        P((x, 1.70 - i * 0.09, 0.07), (0.70 - i * 0.02, 0.10, 0.56), PLATE2 if i % 2 == 0 else PLATE, hip,
          top=(1.04, 1.04), bev=0.025)                                                             # lames
    B((x, 1.56, 0.355), (0.56, 0.025, 0.025), GOLD2, hip)
    P((x + s * 0.33, 1.30, 0.03), (0.06, 0.66, 0.44), PLATE2, hip, top=(1.0, 1.12), bev=0.02)      # outer plate
    crack((x + s * 0.05, 1.28, 0.33), (0.03, 0.42, 0.02), hip, rot=(0, 0, s * 18))
    # the knee
    E((x, 0.86, 0.16), (0.52, 0.44, 0.32), PLATE2, hip, seg=16, rings=8)                           # cop
    R((x, 0.86, 0.29), 0.17, 0.02, GOLD, hip, sides=20, tsides=5, glow="const", k=GK)
    E((x, 0.86, 0.315), (0.13, 0.13, 0.07), HOT, hip, glow="glow", k=1.6)                          # lit stud
    P((x + s * 0.29, 0.86, 0.05), (0.06, 0.46, 0.42), PLATE, hip, rot=(0, 0, s * -10), top=(0.7, 0.8), bev=0.015)   # fan
    C((x, 0.86, 0.0), 0.25, 0.62, DARK, hip, rot=(0, 0, 90), sides=12)                             # the joint
    # shin
    P((x, 0.42, 0.04), (0.56, 0.74, 0.50), PLATE, hip, top=(1.16, 1.14), bev=0.07, seg=2)          # greave
    B((x, 0.44, 0.30), (0.06, 0.62, 0.05), EDGE, hip, bev=0.015)                                  # ridge
    for y in (0.74, 0.20):
        B((x, y, 0.04), (0.64 if y > 0.4 else 0.58, 0.045, 0.56 if y > 0.4 else 0.54), GOLD2, hip)  # bands
    B((x - s * 0.18, 0.46, 0.29), (0.035, 0.46, 0.03), LINE, hip, glow="pulse", k=1.0)            # conduit line
    P((x, 0.46, -0.21), (0.48, 0.58, 0.10), PLATE2, hip, top=(1.15, 1.0), bev=0.03)                # calf plate
    for y in (0.62, 0.46, 0.30):
        rivet((x + s * 0.29, y, 0.04), hip)
    # foot
    B((x, 0.04, 0.10), (0.56, 0.08, 0.86), VOID, hip, bev=0.015)                                   # sole
    for i in range(3):
        P((x, 0.18 - i * 0.03, 0.06 + i * 0.13), (0.56 - i * 0.04, 0.12, 0.2), PLATE2 if i % 2 == 0 else PLATE, hip,
          rot=(14, 0, 0), top=(1.0, 0.9), bev=0.03)                                                # lames
    for k in (-1, 0, 1):
        W((x + k * 0.17, 0.07, 0.62), (0.13, 0.11, 0.24), STEEL, hip, taper=0.3)                  # claws
    B((x, 0.13, -0.24), (0.46, 0.22, 0.22), PLATE, hip, bev=0.04)                                  # heel
    K((x, 0.12, -0.35), 0.05, 0.18, STEEL, hip, rot=(-90, 0, 0), sides=6)                          # heel spike

# ---- the hips: a mail skirt, a belt of power cells, fauld lames, tassets, a gold buckle with a lit gem ----
B((0, 1.83, 0.0), (1.30, 0.34, 0.86), DARK, RT, bev=0.04)
B((0, 1.99, 0.0), (1.50, 0.16, 1.02), PLATE2, RT, bev=0.035)                                       # belt
B((0, 1.99, 0.0), (1.51, 0.03, 1.03), GOLD2, RT)
P((0, 1.99, 0.52), (0.36, 0.26, 0.08), GOLD, RT, top=(1.25, 1.0), bev=0.03, glow="const", k=GK)   # buckle
E((0, 1.99, 0.57), (0.13, 0.13, 0.06), HOT, RT, glow="glow", k=1.8)                               # its gem
for s in (1, -1):
    for i in range(3):                                                                             # power cells
        a = math.radians(30 + i * 22)
        cx, cz = s * 0.78 * math.sin(a), 0.54 * math.cos(a)
        C((cx, 1.99, cz), 0.06, 0.22, STEEL, RT, sides=10)
        C((cx, 1.99, cz + 0.005), 0.035, 0.18, LINE, RT, sides=8, glow="pulse", k=1.2)
for i in range(3):
    P((0, 1.86 - i * 0.11, 0.43 + i * 0.015), (1.30 + i * 0.05, 0.12, 0.08), PLATE if i % 2 else PLATE2, RT,
      top=(0.96, 1.0), bev=0.025)                                                                  # fauld
    P((0, 1.86 - i * 0.11, -0.43 - i * 0.015), (1.30 + i * 0.05, 0.12, 0.08), PLATE if i % 2 else PLATE2, RT,
      top=(0.96, 1.0), bev=0.025)
for s in (1, -1):
    for i in range(2):
        P((s * (0.70 + 0.03 * i), 1.66 - i * 0.24, 0.10 - 0.03 * i), (0.48, 0.32, 0.07), PLATE2 if i == 0 else PLATE, RT,
          rot=(0, 0, s * (8 + 4 * i)), top=(1.08, 1.0), bev=0.025)                                 # tassets
    B((s * 0.72, 1.81, 0.14), (0.46, 0.03, 0.03), GOLD2, RT, rot=(0, 0, s * 8))

# ---- the torso: a barrel chest of layered plate, an armoured belly, ribbed cooling vents down each side,
#      a gorget, and its heart: the reactor behind an iris that opens when it vents ----
T_ = "torso"
P((0, 2.40, 0.0), (1.36, 0.80, 0.94), DARK, T_, top=(1.18, 1.10), bev=0.06, seg=2)                 # belly (under)
for i in range(4):
    P((0, 2.14 + i * 0.16, 0.42 + i * 0.01), (1.04 + i * 0.07, 0.14, 0.14), PLATE if i % 2 else PLATE2, T_,
      top=(1.04, 1.0), bev=0.03)                                                                   # belly lames
P((0, 3.20, 0.0), (1.78, 1.10, 1.06), PLATE, T_, top=(1.10, 0.96), bev=0.09, seg=3)                # chest
P((0, 3.30, 0.42), (1.30, 0.86, 0.30), PLATE2, T_, top=(1.14, 0.9), bev=0.07, seg=2)               # breastplate
for s in (1, -1):
    P((s * 0.50, 3.26, 0.58), (0.50, 0.74, 0.08), PLATE2, T_, rot=(0, s * 14, 0), top=(1.1, 1.0), bev=0.03)   # pectoral plates
    B((s * 0.73, 3.26, 0.52), (0.035, 0.70, 0.04), GOLD2, T_, rot=(0, s * 14, 0))
    crack((s * 0.42, 3.42, 0.63), (0.03, 0.34, 0.02), T_, rot=(0, s * 14, s * -25))
    # ribbed cooling vents down its side, lit from inside
    for k in range(5):
        B((s * 0.90, 2.80 + k * 0.13, 0.05), (0.07, 0.06, 0.66), EDGE, T_, bev=0.015)
        B((s * 0.875, 2.865 + k * 0.13, 0.05), (0.04, 0.05, 0.58), LINE, T_, glow="pulse", k=0.8)
    B((s * 0.62, 3.52, -0.50), (0.42, 0.08, 0.10), GOLD2, T_)
B((0, 3.72, 0.0), (1.20, 0.07, 0.86), GOLD2, T_)                                                   # the collar's band
C((0, 3.72, 0.0), 0.36, 0.20, DARK, T_, sides=14, r2=0.30)                                         # gorget
for i in range(3):
    R((0, 3.66 + i * 0.05, 0.0), 0.36 - i * 0.02, 0.02, PLATE2, T_, rot=(90, 0, 0), sides=18, tsides=5)
# the reactor: a housing, a gold ring, the core (small behind the iris, or swollen and blazing when open)
CY, CZ = 2.96, 0.60
C((0, CY, CZ - 0.06), 0.40, 0.14, STEEL, T_, rot=(90, 0, 0), sides=24)                              # housing
R((0, CY, CZ + 0.02), 0.40, 0.035, GOLD, T_, sides=28, tsides=6, glow="const", k=GK)
for k in range(8):                                                                                 # bolts round it
    a = k * math.pi / 4 + math.pi / 8
    rivet((0.46 * math.cos(a), CY + 0.46 * math.sin(a), CZ), T_, r=0.028)
E((0, CY, CZ + 0.02), (0.36, 0.36, 0.18), HOT, T_, glow="core", k=1.0, show="!open", seg=16, rings=8)   # behind the iris
for k in range(6):   # the iris: six leaves meeting over the core
    a = k * math.pi / 3
    P((0.17 * math.cos(a), CY + 0.17 * math.sin(a), CZ + 0.07), (0.17, 0.3, 0.04), PLATE2, T_,
      rot=(0, 0, math.degrees(a) - 90 + 18), top=(0.25, 1.0), bev=0.008, show="!open")
E((0, CY, CZ + 0.06), (0.62, 0.62, 0.34), HOT, T_, glow="core", k=1.3, show="open", seg=20, rings=10)   # open: swollen
R((0, CY, CZ + 0.10), 0.34, 0.025, HOT, T_, sides=28, tsides=5, glow="core", k=1.6, show="open")
for k in range(6):   # the leaves drawn back into the housing's rim
    a = k * math.pi / 3
    P((0.52 * math.cos(a), CY + 0.52 * math.sin(a), CZ - 0.02), (0.15, 0.22, 0.05), PLATE2, T_,
      rot=(0, 0, math.degrees(a) - 90), top=(0.5, 1.0), bev=0.008, show="open")
# the reactor stack on its back: a housing between two cooling towers with lit vents and exhaust caps,
# cables over its shoulders
B((0, 3.05, -0.62), (0.74, 1.10, 0.30), STEEL, T_, bev=0.05, seg=2)
for k in range(4):
    B((0, 2.70 + k * 0.22, -0.78), (0.52, 0.05, 0.03), LINE, T_, glow="pulse", k=1.0)
for s in (1, -1):
    C((s * 0.22, 3.15, -0.86), 0.15, 1.30, PLATE, T_, sides=14)                                    # tower
    for k in range(3):
        R((s * 0.22, 2.70 + k * 0.42, -0.86), 0.16, 0.02, GOLD2, T_, rot=(90, 0, 0), sides=16, tsides=4)
    C((s * 0.22, 3.84, -0.86), 0.12, 0.08, VOID, T_, sides=14)                                     # exhaust
    C((s * 0.22, 3.86, -0.86), 0.09, 0.04, HOT, T_, sides=12, glow="glow", k=1.4)
    K((s * 0.22, 3.88, -0.86), 0.17, 0.10, PLATE2, T_, sides=10, squash=1.0)                       # its cap
    tube([(s * 0.12, 3.58, -0.74), (s * 0.45, 3.86, -0.55), (s * 0.82, 3.92, -0.35)], [0.05, 0.05, 0.045], CABLE, T_, sides=8)

# ---- the head: a domed helm sunk between its shoulders, a lit T-slit, a jaw guard, cheek plates;
#      a crown of five points that burn ----
H = "head"
E((0, 4.04, 0.0), (0.60, 0.64, 0.66), PLATE, H, seg=18, rings=10)                                 # dome
B((0, 4.30, 0.0), (0.07, 0.10, 0.64), EDGE, H, bev=0.02)                                          # crest ridge
P((0, 3.86, 0.10), (0.56, 0.30, 0.52), PLATE2, H, top=(1.12, 1.0), bev=0.04)                      # jaw guard
for s in (1, -1):
    P((s * 0.27, 3.99, 0.05), (0.08, 0.40, 0.52), PLATE2, H, top=(1.0, 0.85), bev=0.02)            # cheek plates
    for k in range(3):
        B((s * 0.06 + s * k * 0.06, 3.86, 0.36), (0.025, 0.16, 0.02), VOID, H)                     # breaths
B((0, 4.08, 0.32), (0.40, 0.055, 0.05), HOT, H, glow="glow", k=2.2)                               # the slit
B((0, 3.97, 0.32), (0.06, 0.20, 0.05), HOT, H, glow="glow", k=2.0)
B((0, 4.18, 0.31), (0.50, 0.03, 0.03), GOLD2, H)                                                  # brow band
C((0, 4.24, 0.0), 0.31, 0.06, GOLD, H, sides=20, glow="const", k=GK)                              # the crown's circlet
CR = "crown"
for i in range(5):
    a = math.radians(-50 + 25 * i)
    hgt = 0.48 if i == 2 else (0.34 if i % 2 else 0.26)
    K((0.29 * math.sin(a), 4.26, 0.29 * math.cos(a)), 0.045, hgt, GOLD, CR, rot=(math.degrees(0.22 * math.cos(a)), 0, -math.degrees(0.22 * math.sin(a))), sides=6)
    E((0.27 * math.sin(a), 4.30, 0.27 * math.cos(a)), (0.06, 0.06, 0.04), HOT, CR, glow="glow", k=1.4)
for i in range(3):   # (the back of the crown: lower points)
    a = math.radians(150 + 30 * i)
    K((0.29 * math.sin(a), 4.26, 0.29 * math.cos(a)), 0.04, 0.18, GOLD2, CR, sides=6)

# ---- the shoulders: great layered pauldrons trimmed in gold, each with a conduit pylon (a lit fin and a
#      crystal), the arm in plate below: a vambrace, an elbow cop, a forearm with a conduit line, and a
#      gauntlet of gold with lit knuckle seams and an emitter in the palm (what it fires its volleys from) ----
for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 1.18
    E((x, 3.66, 0.0), (1.02, 0.62, 1.22), PLATE, T_, seg=18, rings=8)                              # pauldron dome
    for i in range(3):   # lames draping off the dome's rim, each steeper and further out
        P((x + s * (0.40 + 0.05 * i), 3.44 - i * 0.12, 0.0), (0.46, 0.11, 1.20 - i * 0.08), PLATE2 if i % 2 == 0 else PLATE, T_,
          rot=(0, 0, s * (-38 - 10 * i)), top=(1.0, 0.95), bev=0.03)
        B((x + s * (0.60 + 0.06 * i), 3.36 - i * 0.135, 0.0), (0.03, 0.03, 1.16 - i * 0.08), GOLD2, T_, rot=(0, 0, s * (-38 - 10 * i)))
    R((x, 3.66, 0.0), 0.50, 0.03, GOLD, T_, rot=(90, 0, 0), sides=26, tsides=5, glow="const", k=GK)
    crack((x - s * 0.08, 3.88, 0.28), (0.24, 0.03, 0.02), T_, rot=(0, 0, s * 20))
    # the pylon
    B((x + s * 0.04, 4.24, -0.12), (0.10, 0.72, 0.30), PLATE2, T_, rot=(0, 0, s * -10), bev=0.025)
    B((x + s * 0.04, 4.24, 0.035), (0.04, 0.62, 0.03), LINE, T_, rot=(0, 0, s * -10), glow="pulse", k=1.4)
    for k in range(2):
        B((x + s * 0.03, 4.02 + k * 0.30, -0.12), (0.16, 0.04, 0.34), GOLD2, T_, rot=(0, 0, s * -10))
    E((x + s * 0.11, 4.70, -0.12), (0.14, 0.26, 0.14), HOT, T_, rot=(0, 45, s * -10), glow="glow", k=1.8, seg=4, rings=2)   # crystal
    # the arm
    C((x + s * 0.045, 3.00, 0.0), 0.25, 0.70, DARK, arm, sides=12)                                 # upper arm
    P((x + s * 0.05, 3.00, 0.0), (0.56, 0.62, 0.58), PLATE, arm, top=(1.08, 1.06), bev=0.05, seg=2)    # vambrace
    B((x + s * 0.32, 3.00, 0.0), (0.04, 0.50, 0.30), EDGE, arm, bev=0.01)
    E((x + s * 0.05, 2.56, -0.04), (0.46, 0.38, 0.44), PLATE2, arm, seg=14, rings=7)               # elbow cop
    K((x + s * 0.05, 2.56, -0.24), 0.05, 0.16, STEEL, arm, rot=(-90, 0, 0), sides=6)
    P((x + s * 0.05, 2.12, 0.0), (0.58, 0.64, 0.60), PLATE, arm, top=(1.14, 1.12), bev=0.05, seg=2)    # forearm
    B((x + s * 0.05, 2.14, 0.31), (0.04, 0.52, 0.03), LINE, arm, glow="pulse", k=1.2)
    for y in (2.40, 1.86):
        B((x + s * 0.05, y, 0.0), (0.66, 0.05, 0.68), GOLD2, arm)
    # the gauntlet
    fx = x + s * 0.05
    B((fx + s * 0.06, 1.46, 0.0), (0.62, 0.66, 0.76), GOLD2, arm, bev=0.16, seg=3)                  # the back of the hand
    P((fx + s * 0.33, 1.48, 0.0), (0.10, 0.52, 0.62), GOLD, arm, top=(0.8, 0.9), bev=0.03, glow="const", k=GK)   # its plate
    for k in range(4):   # four curled fingers on the palm side, lit between
        z = -0.27 + 0.18 * k
        B((fx - s * 0.24, 1.36, z), (0.20, 0.50, 0.16), GOLD2, arm, bev=0.06, seg=2)
        E((fx - s * 0.10, 1.12, z), (0.22, 0.14, 0.17), GOLD, arm, seg=10, rings=5)                    # knuckle cap
        if k < 3:
            B((fx - s * 0.24, 1.36, z + 0.09), (0.18, 0.44, 0.02), HOT, arm, glow="glow", k=0.9)
    P((fx - s * 0.10, 1.54, 0.40), (0.18, 0.40, 0.16), GOLD2, arm, rot=(-12, 0, 0), top=(0.8, 0.8), bev=0.05)   # thumb
    C((fx, 1.86, 0.0), 0.36, 0.14, GOLD2, arm, sides=16)                                             # cuff
    C((x + s * 0.05, 1.075, 0.0), 0.15, 0.04, HOT, arm, sides=16, glow="glow", k=2.0)              # palm emitter
    R((x + s * 0.05, 1.08, 0.0), 0.19, 0.022, STEEL, arm, rot=(90, 0, 0), sides=18, tsides=4)

# ---- the cape: two halves hanging either side of its reactor stack, dark violet, gold-hemmed, torn at the
#      edge; each in two pieces (the top on "cape", the lower on "capeTip", which lags it) ----
CLOTH, CLOTH2 = "crimson*0.9", "warden*0.4"
for s in (1, -1):   # each half three pleats, turned in and out so the cloth folds
    for k in range(3):
        px, turn = s * (0.46 + 0.2 * k), (1 if k % 2 == 0 else -1) * 14
        dz = -0.03 if k % 2 else 0.0
        P((px, 2.80, -0.68 + dz), (0.22, 1.66, 0.04), CLOTH2, "cape", rot=(0, turn, 0), top=(0.9, 1.0))     # upper
        P((px + s * 0.01 * k, 1.14, -0.68 + dz), (0.23, 1.56, 0.04), CLOTH2, "capeTip", rot=(0, turn, 0), top=(1.04, 1.0))   # lower
        W((px + s * 0.01 * k, 0.28, -0.68 + dz), (0.22, 0.04, 0.22 - 0.05 * k), CLOTH2, "capeTip", rot=(90, turn, 0), taper=0.1)   # a torn tongue
        B((px + s * 0.01 * k, 0.37, -0.675 + dz), (0.23, 0.04, 0.05), GOLD2, "capeTip", rot=(0, turn, 0))   # gold hem
    x = s * 0.66
    B((x, 3.58, -0.64), (0.62, 0.10, 0.10), GOLD2, "cape", bev=0.02)                                # its clasp bar
    rivet((x, 3.58, -0.58), "cape", r=0.05, mat=GOLD)

report("warden")
save_enemy("warden")
