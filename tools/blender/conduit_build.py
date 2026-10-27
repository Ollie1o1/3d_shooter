# The CONDUIT in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/conduit_build.py).
# A reactor pylon: an octagonal plinth, four buttresses with lit veins, a caged core column that beats faster as it's
# hurt ("core"), two broken rings turning round it ("halo" low, "crown" high: "pulse" lights their nodes), and a
# crystal turning above it all ("head").
#   halo (0, 1.2, 0)  crown (0, 2.3, 0)  head (0, 3.1, 0); it stands on the floor
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("conduit", joints=globals().get("SKELETON")); clear_parts()

DARK, DARK2, TRIM = "conduit*1.2", "conduit*1.7", "conduit*2.6"
STEEL = "gunmetal*1.5"
HOT = "glow*0.3"
RT = "conduit"

# ---- the plinth ----
C((0, 0.12, 0.0), 0.82, 0.24, DARK, RT, sides=8)
C((0, 0.3, 0.0), 0.72, 0.12, DARK2, RT, sides=8)
R((0, 0.37, 0.0), 0.72, 0.03, HOT, RT, rot=(90, 0, 0), sides=8, tsides=4, glow="glow", k=0.5)    # its lit trim
for k in range(8):
    a = k * math.pi / 4 + math.pi / 8
    B((0.8 * math.sin(a), 0.12, 0.8 * math.cos(a)), (0.12, 0.2, 0.08), TRIM, RT, rot=(0, math.degrees(a), 0), bev=0.015)

# ---- four buttresses, veins of light up their inner edges ----
for k in range(4):
    a = k * math.pi / 2 + math.pi / 4
    sx, sz = math.sin(a), math.cos(a)
    P((0.6 * sx, 1.15, 0.6 * sz), (0.16, 1.7, 0.42), DARK2, RT, rot=(0, math.degrees(a), 0), top=(0.6, 0.35), shift=(0.0, -0.12), bev=0.03)
    B((0.47 * sx, 1.15, 0.47 * sz), (0.04, 1.3, 0.04), HOT, RT, rot=(0, math.degrees(a), 0), glow="core", k=0.6)
    B((0.72 * sx, 0.5, 0.72 * sz), (0.2, 0.3, 0.46), TRIM, RT, rot=(0, math.degrees(a), 0), bev=0.03)   # its foot

# ---- the core: a lit column in a cage, capped ----
C((0, 1.7, 0.0), 0.22, 2.4, HOT, RT, sides=14, glow="core", k=1.0)
for k in range(4):
    a = k * math.pi / 2
    C((0.3 * math.sin(a), 1.7, 0.3 * math.cos(a)), 0.025, 2.3, STEEL, RT, sides=6)                 # cage bars
for y in (0.6, 1.7, 2.8):
    R((0, y, 0.0), 0.3, 0.035, STEEL, RT, rot=(90, 0, 0), sides=16, tsides=4)                    # cage hoops
C((0, 2.95, 0.0), 0.34, 0.12, DARK2, RT, sides=8)                                                # cap
K((0, 3.0, 0.0), 0.24, 0.12, TRIM, RT, sides=8)


# ---- two broken rings, each four arcs with lit nodes ----
def ring(joint, y, r, turn):
    for k in range(4):
        a0 = k * 90 + turn
        R((0, y, 0.0), r, 0.05, TRIM, joint, rot=(90, a0, 0), sides=10, tsides=5, arc=62)   # (flat, then turned about Y)
        b = math.radians(31 - a0)                                                                # (the arc's middle)
        E((r * math.cos(b), y, r * math.sin(b)), (0.12, 0.12, 0.12), HOT, joint, glow="pulse", k=1.0, seg=8, rings=4)


ring("halo", 1.2, 0.58, 0)
ring("crown", 2.3, 0.52, 45)

# ---- the crystal: a long diamond, lit ----
E((0, 3.1, 0.0), (0.36, 0.56, 0.36), HOT, "head", glow="core", k=1.2, seg=4, rings=2)
E((0, 3.1, 0.0), (0.46, 0.08, 0.46), TRIM, "head", seg=8, rings=2)                                  # its girdle

report("conduit")
save_enemy("conduit")
