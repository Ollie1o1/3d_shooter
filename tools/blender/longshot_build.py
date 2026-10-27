# THE LONGSHOT in detail (python3 tools/blender/live.py tools/blender/longshot_build.py): a heavy anti-materiel
# sniper. A railed receiver, a fluted barrel ending in a big brake whose ports glow violet, a slotted handguard with a
# bipod folded under it; a full scope (objective and eyepiece bells with violet glass, turrets, a power ring, rings
# on the rail) with its ammo readout built into the side you see; a ribbed magazine with a lit window, a chassis
# stock with a bone cheek riser and a monopod, a bone-panelled grip, and a tactical bolt knob ringed in violet.
#
# Fixed by the game (GunKit.h): bolt (0.034, 0.025, -0.06) lifts and draws back; mag (0, -0.07, 0.06) drops 0.18 on
# a reload; the cells hang on "cells" (-0.0235, 0.098, -0.04) along the scope's left, eight along +Z 0.028 apart, a
# second row 0.012 below; the muzzle (0, 0.026, 0.84); the scope's axis 0.098 above the origin (LONG_SCOPE_Y).
# The brake's ports and the lit lines glow on "accent" (brighter the fuller the gun, flaring as it fires).
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/rifle_kit.py").read())
root = new_gun("longshot", [("bolt", (0.034, 0.025, -0.06), None), ("mag", (0.0, -0.07, 0.06), None),
                            ("cells", (-0.0235, 0.098, -0.04), None), ("muzzle", (0.0, 0.026, 0.84), None)])
G = root
BLK, BLK2, DARK, VOID = "black", "black*1.6", "black*0.75", "black*0.35"
STL, STL2, EDGE, BONE = "steel", "steel*0.8", "edge", "bone"
LIT, GLASS = "glow*0.5", "glow*0.3"
X90 = (90, 0, 0)
SY = 0.098   # the scope's axis

# ---- the receiver: its body, a recessed side panel and a lit line, the ejection port, the bolt shroud, the rail ----
P((0, 0.012, 0.02), (0.058, 0.064, 0.25), BLK, G, bev=0.006, seg=2)
B((-0.0295, 0.014, 0.02), (0.002, 0.036, 0.2), DARK, G)
B((-0.0302, -0.006, 0.02), (0.002, 0.004, 0.2), LIT, G, glow="accent", k=0.8)
for z in (-0.07, 0.11):
    E((-0.0298, 0.03, z), (0.007, 0.007, 0.007), STL, G, seg=8, rings=4)                     # screws
B((0.0295, 0.03, 0.03), (0.003, 0.018, 0.07), VOID, G)                                      # ejection port
C((0, 0.025, -0.118), 0.021, 0.03, STL2, G, rot=X90, sides=16)                              # bolt shroud
E((0, 0.025, -0.135), (0.03, 0.03, 0.016), STL, G, seg=12, rings=6)
B((0, 0.0475, 0.04), (0.03, 0.008, 0.32), BLK2, G, bev=0.002)                               # rail
for i in range(16):
    B((0, 0.0535, -0.11 + i * 0.02), (0.032, 0.004, 0.008), STL, G)                         # its ridges

# ---- the scope: main tube, objective bell and its violet glass and shade, eyepiece bell, glass and cup, a power
#      ring, turrets on a saddle, rings and their bases on the rail; the ammo readout on its left (the cells sit on it) ----
C((0, SY, 0.04), 0.019, 0.34, BLK2, G, rot=X90, sides=20)
C((0, SY, 0.245), 0.019, 0.07, BLK2, G, rot=X90, sides=24, r2=0.034)                         # objective bell
C((0, SY, 0.281), 0.031, 0.003, GLASS, G, rot=X90, sides=24, glow="glow", k=0.5)            # its glass
R((0, SY, 0.282), 0.0335, 0.0032, STL, G, sides=28, tsides=5)
C((0, SY, -0.155), 0.028, 0.05, BLK2, G, rot=X90, sides=24, r2=0.019)                        # eyepiece bell
C((0, SY, -0.181), 0.024, 0.003, GLASS, G, rot=X90, sides=24, glow="glow", k=0.4)
R((0, SY, -0.184), 0.026, 0.0045, VOID, G, sides=24, tsides=5)                               # eye cup
for z in (-0.124, -0.117, -0.110):
    R((0, SY, z), 0.0205, 0.0026, STL, G, sides=20, tsides=4)                                # power ring
B((0, SY + 0.021, -0.117), (0.003, 0.002, 0.02), LIT, G, glow="glow", k=0.8)                # its index
P((0, SY + 0.016, 0.05), (0.03, 0.012, 0.04), BLK2, G, bev=0.003)                           # turret saddle
C((0, SY + 0.027, 0.05), 0.011, 0.012, STL, G, sides=16)                                     # elevation
C((0, SY + 0.036, 0.05), 0.012, 0.008, EDGE, G, sides=16)
R((0, SY + 0.031, 0.05), 0.0122, 0.0015, VOID, G, rot=X90, sides=16, tsides=4)
C((0.027, SY, 0.05), 0.011, 0.012, STL, G, rot=(0, 0, -90), sides=16)                        # windage
C((0.036, SY, 0.05), 0.012, 0.008, EDGE, G, rot=(0, 0, -90), sides=16)
for z in (-0.095, 0.19):
    R((0, SY, z), 0.0215, 0.0042, STL, G, sides=22, tsides=5)                                # rings
    B((0, 0.068, z), (0.024, 0.026, 0.016), BLK2, G, bev=0.003)                              # their bases
    E((0.014, 0.07, z), (0.008, 0.008, 0.008), STL, G, seg=8, rings=4)                       # clamp bolts
B((-0.019, 0.093, 0.058), (0.004, 0.03, 0.23), VOID, G)                                    # the readout
for y in (0.1065, 0.0795):
    B((-0.0225, y, 0.058), (0.004, 0.003, 0.23), STL, G, bev=0.0008)
for z in (-0.058, 0.174):
    B((-0.0225, 0.093, z), (0.004, 0.03, 0.004), STL, G)

# ---- the barrel: fluted, a heavy brake with violet ports; the handguard, slotted, with a lit line, its cap; a
#      bipod folded under it ----
C((0, 0.026, 0.48), 0.016, 0.62, STL2, G, rot=X90, sides=16)
for (dx, dy) in ((-0.0158, 0.006), (-0.0158, -0.006), (-0.006, 0.0158), (0.006, 0.0158)):
    B((dx, 0.026 + dy, 0.66), (0.003 if abs(dx) > 0.01 else 0.004, 0.004 if abs(dx) > 0.01 else 0.003, 0.2), VOID, G)   # flutes
P((0, 0.026, 0.80), (0.056, 0.036, 0.08), EDGE, G, bev=0.006, seg=2)                          # brake
for d in (1, -1):
    for i in range(3):
        B((d * 0.0285, 0.026, 0.776 + 0.022 * i), (0.004, 0.022, 0.012), VOID, G)
        B((d * 0.0268, 0.026, 0.776 + 0.022 * i), (0.002, 0.016, 0.008), LIT, G, glow="accent", k=0.7)
R((0, 0.026, 0.8405), 0.012, 0.002, LIT, G, sides=18, tsides=4, glow="accent", k=0.5)
P((0, 0.02, 0.35), (0.054, 0.054, 0.36), BLK, G, bev=0.006, seg=2)                            # handguard
for i in range(6):
    B((-0.0272, 0.016, 0.20 + 0.05 * i), (0.002, 0.012, 0.032), VOID, G)                     # its slots
    B((0.0272, 0.016, 0.20 + 0.05 * i), (0.002, 0.012, 0.032), VOID, G)
B((-0.0275, 0.038, 0.35), (0.002, 0.003, 0.32), LIT, G, glow="accent", k=0.8)
P((0, 0.02, 0.535), (0.058, 0.058, 0.012), BLK2, G, bev=0.004)
B((0, -0.012, 0.50), (0.03, 0.012, 0.03), STL, G, bev=0.002)                                 # bipod hinge
for d in (1, -1):
    C((d * 0.012, -0.016, 0.38), 0.005, 0.22, STL2, G, rot=X90, sides=10)                     # its legs
    E((d * 0.012, -0.016, 0.268), (0.012, 0.01, 0.014), BLK2, G, seg=8, rings=4)
    R((d * 0.012, -0.016, 0.44), 0.0062, 0.0014, STL, G, sides=12, tsides=4)

# ---- the magazine (rides "mag"): ribbed, a lit window, a baseplate; its flared well ----
P((0, -0.022, 0.06), (0.05, 0.014, 0.084), BLK2, G, bev=0.003)
MG = "mag"
P((0, -0.068, 0.06), (0.042, 0.096, 0.072), STL2, MG, bev=0.004, seg=2)
for d in (1, -1):
    for o in (-0.02, 0.0, 0.02):
        B((d * 0.0215, -0.075 + o, 0.06), (0.002, 0.004, 0.058), BLK, MG)
B((-0.022, -0.05, 0.075), (0.002, 0.026, 0.024), LIT, MG, glow="glow", k=0.6)               # its window
P((0, -0.12, 0.06), (0.046, 0.01, 0.078), BLK2, MG, bev=0.003)

# ---- the trigger group and the grip: a guard, a trigger, a black grip with bone panels and finger grooves ----
B((0, -0.058, -0.02), (0.006, 0.004, 0.05), STL, G)
B((0, -0.042, 0.005), (0.006, 0.03, 0.004), STL, G, rot=(-12, 0, 0))
P((0, -0.037, -0.022), (0.004, 0.022, 0.006), STL, G, rot=(18, 0, 0), top=(1.0, 1.4))
P((0, -0.08, -0.08), (0.036, 0.09, 0.048), BLK, G, rot=(-18, 0, 0), top=(1.05, 1.2), bev=0.012, seg=3)
for d in (1, -1):
    P((d * 0.0185, -0.08, -0.082), (0.003, 0.068, 0.036), BONE, G, rot=(-18, 0, 0), bev=0.001)
for i in range(3):
    E((0, -0.065 - i * 0.022, -0.054 + i * 0.007), (0.03, 0.014, 0.012), BLK2, G, seg=8, rings=4)

# ---- the stock: a chassis (top beam, lower beam), a bone cheek riser on two posts, a butt plate and pad, a
#      monopod, lit lines ----
P((0, 0.01, -0.21), (0.04, 0.036, 0.22), BLK, G, bev=0.005, seg=2)
P((0, 0.044, -0.22), (0.032, 0.02, 0.15), BONE, G, bev=0.006, seg=2)
for z in (-0.17, -0.27):
    C((0, 0.031, z), 0.004, 0.012, STL, G, sides=8)
P((0, -0.075, -0.21), (0.034, 0.022, 0.21), BLK, G, bev=0.004)
P((0, -0.025, -0.322), (0.048, 0.14, 0.016), BLK2, G, bev=0.004)
P((0, -0.025, -0.334), (0.05, 0.145, 0.01), VOID, G, bev=0.003)
C((0, -0.105, -0.27), 0.006, 0.05, STL2, G, sides=10)                                       # monopod
E((0, -0.13, -0.27), (0.02, 0.008, 0.02), BLK2, G, seg=10, rings=4)
B((-0.0202, 0.01, -0.21), (0.002, 0.004, 0.18), LIT, G, glow="accent", k=0.7)
B((-0.0172, -0.075, -0.21), (0.002, 0.003, 0.17), LIT, G, glow="accent", k=0.5)

# ---- the bolt: collar, handle, a big tactical knob ringed in violet ----
BO = "bolt"
C((0.036, 0.025, -0.06), 0.008, 0.01, STL, BO, rot=(0, 0, -90), sides=12)
C((0.06, 0.025, -0.06), 0.0045, 0.05, STL, BO, rot=(0, 0, -95), sides=10)
E((0.093, 0.0225, -0.06), (0.026, 0.026, 0.026), BLK2, BO, seg=16, rings=8)
R((0.093, 0.0225, -0.06), 0.0128, 0.0016, LIT, BO, rot=(0, 90, 0), sides=18, tsides=4, glow="accent", k=0.8)
for o in (-0.006, 0.006):
    R((0.093 + o, 0.0225, -0.06), 0.0118, 0.0012, VOID, BO, rot=(0, 90, 0), sides=18, tsides=4)

save_built("longshot")
