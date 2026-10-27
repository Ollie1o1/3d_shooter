# THE LEVIATHAN in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/leviathan_build.py).
# The serpent in the Maw: a long iron skull plated like a pangolin's back, a hooded brow over the eclipse eye (a black
# disc in a ring of fire, twelve flame blades turning round it: "halo"), swept horns, a frill of bone spines, turbine
# teeth in both jaws and a throat that lights for the torrent and the swallow ("shot"). Seams of light run along it.
#   head (0, 0, 0): the skull, in the head rig's units (the game draws it x1.5); faces +Z
#   jaw (0, -0.7, -1.2): the hinge under the skull (it opens about X)   halo (0, 1.25, 2.2): the eye's centre
#   body (0, 0, -12): ONE body segment, r 2.5, running from z -12 to -9.5 (+Z toward the head); the game lays it
#   along each stretch of its body, scaled to it: "pulse" lights its seam ring, "light" its fin
# Starts from the saved file's joints; the first build, from a skeleton (SKELETON=<tools/enemy_export --joints leviathan>).
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("leviathan", joints=globals().get("SKELETON")); clear_parts()

PLATE, PLATE2, EDGE = "leviathan*1.6", "leviathan*2.2", "leviathan*3.0"
DARK, VOID, BLACK = "leviathan*0.8", "leviathan*0.3", "leviathan*0.05"
BONE, BONE2 = "bone*0.66", "bone*0.5"
SEAM = "leviathan*1.2"                   # (lit by "seam")
H, J, HALO, BODY = "head", "jaw", "halo", "body"

# ---- the skull: a long cranium plated along the top, cheek plates, a snout, seams of light ----
E((0, 0.3, -0.5), (3.0, 2.0, 4.4), PLATE, H, seg=20, rings=10)
for i in range(5):   # the crest: scales overlapping back along its crown, each lifting at the rear
    E((0, 1.18 - 0.06 * i, 0.55 - 0.72 * i), (2.2 - 0.22 * i, 0.38, 1.2), PLATE2 if i % 2 == 0 else PLATE, H,
      rot=(-10, 0, 0), seg=14, rings=6)
E((0, 1.2, 1.0), (2.5, 0.65, 2.0), PLATE2, H, seg=16, rings=8)                                   # the brow's hood
for s in (1, -1):
    K((s * 0.95, 1.45, 1.4), 0.16, 0.7, EDGE, H, rot=_aim((s * 0.5, 0.5, -1.0)), sides=6)          # brow spurs
R((0, 1.25, 2.08), 0.8, 0.09, DARK, H, sides=28, tsides=6)                                       # the eye's socket
E((0, 0.0, 1.9), (2.4, 1.1, 2.9), PLATE, H, seg=18, rings=9)                                     # snout
P((0, 0.42, 2.0), (1.7, 2.2, 0.22), PLATE2, H, rot=(90, 0, 0), top=(0.45, 1.0), bev=0.08, seg=2)  # its plate
B((0, 0.55, 2.0), (0.14, 0.1, 1.9), EDGE, H, bev=0.03)                                           # its ridge
for s in (1, -1):
    B((s * 0.4, 0.45, 3.05), (0.32, 0.06, 0.1), SEAM, H, glow="seam", k=0.8)                     # nostrils
    E((s * 1.45, 0.1, 0.0), (0.4, 1.3, 2.7), PLATE2, H, seg=12, rings=8)                         # cheek plates
    B((s * 1.69, 0.05, 0.2), (0.05, 0.12, 2.5), SEAM, H, glow="seam", k=0.45)
    for k in range(3):                                                                          # seams of light
        B((s * (1.36 - 0.06 * k), 0.75 - 0.35 * k, -0.7 + 0.2 * k), (0.06, 0.06, 2.4 - 0.5 * k), SEAM, H, rot=(0, s * -4, 0), glow="seam", k=1.0)
    # horns: two swept back each side, a great one and a lesser
    tube([(s * 1.15, 1.15, -1.0), (s * 1.75, 1.75, -3.0), (s * 1.95, 2.45, -4.4)], [0.34, 0.24, 0.14], BONE, H, sides=10, tip=0.5)
    tube([(s * 1.45, 0.35, -1.5), (s * 2.1, 0.45, -2.9), (s * 2.35, 0.85, -3.8)], [0.22, 0.15, 0.09], BONE2, H, sides=8, tip=0.35)
    for k in range(3):   # side frill: spines fanned back behind the cheek
        a = math.radians(-20 + 25 * k)
        K((s * 1.45, 0.2 + 0.25 * k, -1.9), 0.12, 1.1 - 0.15 * k, BONE2, H, rot=_aim((s * 0.6, math.sin(a) * 0.6, -1.0)), sides=6, squash=0.5)
for k in range(5):   # the back frill: spines off the back of its skull
    a = math.radians(-40 + 20 * k)
    K((0.5 * math.sin(a), 1.2, -2.5), 0.16, 1.5 - 0.2 * abs(k - 2), BONE, H, rot=_aim((math.sin(a) * 0.5, 0.7, -1.0)), sides=6, squash=0.5)
# the upper jaw: a dark palate and turbine teeth
E((0, -0.35, 1.3), (2.3, 0.55, 3.9), PLATE, H, seg=16, rings=8)                                    # the upper jaw
E((0, -0.52, 1.4), (1.9, 0.12, 3.4), VOID, H, seg=14, rings=4)                                     # its palate
for k in range(6):
    K((-0.8 + 0.32 * k, -0.5, 2.85 - 0.12 * abs(k - 2.5)), 0.11, 0.55, BONE, H, rot=(180, 0, 0), sides=6)
for s in (1, -1):
    for k in range(3):
        K((s * (1.0 - 0.12 * k), -0.5, 0.6 + 0.6 * k), 0.09, 0.42, BONE2, H, rot=(180, 0, 0), sides=6)

# ---- the jaw: plated, a chin spike, turbine teeth, the throat's glow ----
E((0, -1.0, 0.75), (2.5, 0.75, 4.2), PLATE, J, seg=18, rings=9)                                  # the jaw
E((0, -1.3, 0.6), (1.9, 0.28, 3.4), PLATE2, J, seg=14, rings=6)                                    # its belly plate
K((0, -1.25, 2.6), 0.18, 0.7, BONE, J, rot=_aim((0, -0.6, 1.0)), sides=6)                        # chin spike
E((0, -0.7, 0.9), (1.9, 0.1, 3.3), VOID, J, seg=14, rings=4)                                       # the mouth's floor
B((0, -0.66, 0.4), (1.6, 0.1, 2.4), "leviathan*0.25", J, glow="shot", k=1.0)                      # the throat's glow
for k in range(6):
    K((-0.8 + 0.32 * k, -0.75, 2.45 - 0.12 * abs(k - 2.5)), 0.11, 0.5, BONE, J, sides=6)
for s in (1, -1):
    for k in range(3):
        K((s * (0.95 - 0.12 * k), -0.75, 0.4 + 0.6 * k), 0.09, 0.4, BONE2, J, sides=6)
    B((s * 1.3, -1.0, 0.6), (0.06, 0.1, 2.6), SEAM, J, glow="seam", k=0.5)

# ---- the eye: a black disc in a ring of fire, twelve flame blades turning round it (the halo flickers each) ----
C((0, 1.25, 2.16), 0.66, 0.12, BLACK, H, rot=(90, 0, 0), sides=28, glow="eye", k=1.0)
R((0, 1.25, 2.22), 0.70, 0.06, "glow*0.3", H, sides=28, tsides=5, glow="glow", k=1.1)
for k in range(12):
    a = k * math.pi / 6
    d = (math.cos(a), math.sin(a), 0.0)
    K((0.74 * d[0], 1.25 + 0.74 * d[1], 2.2), 0.13, 0.55 + 0.15 * (k % 2), "glow*0.3", HALO, rot=_aim(d), sides=6,
      squash=0.4, glow="glow", k=1.0, phase=k * 1.7)

# ---- one body segment (on "body", z -12 to -9.5): a core, seven armour bands round its back and sides, a pale
#      belly with two ridges, a seam ring of light at its root, a bone fin, two side spikes ----
Z0, ZC = -12.0, -10.75
C((0, 0, ZC), 2.2, 3.0, DARK, BODY, rot=(90, 0, 0), sides=16)
for k in range(7):
    a = math.radians(-120 + 40 * k)
    B((2.2 * math.sin(a), 2.2 * math.cos(a), ZC + 0.05), (1.75, 0.34, 2.85), PLATE2 if k % 2 == 0 else PLATE, BODY,
      rot=(-5, 0, -math.degrees(a)), bev=0.12, seg=2)
B((0, -2.15, ZC), (2.4, 0.36, 2.9), "leviathan*2.8", BODY, bev=0.1, seg=2)                         # belly
for s in (1, -1):
    B((s * 0.55, -2.33, ZC), (0.14, 0.1, 2.7), "leviathan*1.4", BODY, bev=0.03)
R((0, 0, Z0 + 0.05), 2.42, 0.13, SEAM, BODY, sides=24, tsides=5, glow="pulse", k=1.0)              # seam ring
P((0, 2.9, ZC - 0.1), (0.24, 1.5, 2.1), BONE, BODY, top=(0.3, 0.5), shift=(0.0, -0.5), bev=0.04)   # dorsal fin
B((0, 2.9, ZC + 0.85), (0.08, 1.2, 0.08), SEAM, BODY, rot=(-20, 0, 0), glow="light", k=1.0)          # its lit edge
for s in (1, -1):
    K((s * 2.35, 0.3, ZC + 0.4), 0.18, 0.8, BONE2, BODY, rot=_aim((s * 1.0, 0.2, -0.6)), sides=6)

report("leviathan")
save_enemy("leviathan")
