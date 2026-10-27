# THE LANCER in detail (python3 tools/blender/live.py tools/blender/lancer_build.py): a bolt-action coil rifle.
# A round receiver with its ammo cells in a bay on the side you see, a barrel wound with cyan coils under a slotted
# heat shield, a ported brake, three-dot iron sights, and the lance itself: a bayonet blade under the muzzle with a
# lit edge. A skeleton thumbhole stock with a bone cheek riser, a bolt with a cyan-ringed knob, a stripper-clip guide.
#
# Fixed by the game (GunKit.h): bolt (0.02, 0.022, -0.035) lifts and draws back; the cells hang on "cells"
# (-0.0215, 0.026, -0.03), eight along +Z 0.022 apart, a second row 0.012 below; the stripper clip seats at
# (0, 0.07, -0.005) from above; the muzzle (0, 0.028, 0.68); the sight line 0.052 above the origin (LANCER_SIGHT_Y).
# The coils and lit lines glow on "accent" (brighter the fuller the gun, flaring as it fires).
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/rifle_kit.py").read())
root = new_gun("lancer", [("bolt", (0.02, 0.022, -0.035), None), ("cells", (-0.0215, 0.026, -0.03), None),
                          ("muzzle", (0.0, 0.028, 0.68), None)])
G = root
BLK, BLK2, DARK, VOID = "black", "black*1.6", "black*0.75", "black*0.35"
STL, STL2, EDGE, BONE = "steel", "steel*0.8", "edge", "bone"
LIT = "glow*0.5"
X90 = (90, 0, 0)   # (a cylinder or ring along the barrel)

# ---- the receiver: a round action, its lower housing, the bolt shroud and cocking piece, the stripper-clip guide,
#      the ejection port, the cell bay (where the game draws the cells) with its rails, screws, a lit line ----
C((0, 0.024, 0.02), 0.021, 0.21, BLK, G, rot=X90, sides=18)                                  # action
for z in (-0.083, 0.123):
    C((0, 0.024, z), 0.0225, 0.008, BLK2, G, rot=X90, sides=18)                               # its rings
P((0, -0.008, 0.03), (0.034, 0.034, 0.17), DARK, G, top=(1.0, 1.0), bev=0.004, seg=2)          # lower housing
C((0, 0.024, -0.098), 0.016, 0.03, STL2, G, rot=X90, sides=14)                                # bolt shroud
E((0, 0.026, -0.116), (0.02, 0.022, 0.016), STL, G, seg=10, rings=6)                         # cocking piece
for d in (1, -1):
    B((d * 0.011, 0.047, -0.004), (0.005, 0.008, 0.026), STL, G, bev=0.001)                    # stripper guide
B((0.0195, 0.032, 0.045), (0.004, 0.014, 0.055), VOID, G)                                    # ejection port
B((-0.0205, 0.020, 0.047), (0.003, 0.030, 0.19), VOID, G)                                    # the cell bay
for y in (0.0365, 0.0035):
    B((-0.0225, y, 0.047), (0.004, 0.003, 0.19), STL, G, bev=0.0008)                          # its rails
for z in (-0.044, 0.138):
    B((-0.0225, 0.020, z), (0.004, 0.036, 0.004), STL, G)
for z in (-0.06, 0.0, 0.10):
    E((-0.0172, -0.006, z), (0.006, 0.006, 0.006), STL, G, seg=8, rings=4)                    # screws
B((-0.0175, -0.018, 0.03), (0.002, 0.004, 0.15), LIT, G, glow="accent", k=0.8)               # lit line

# ---- the barrel: wound with seven cyan coils between tie rods, under a slotted heat shield on a lower rail; a
#      front band with a sling loop; a ported brake ----
C((0, 0.028, 0.40), 0.0095, 0.56, STL2, G, rot=X90, sides=14)                                 # barrel
for i in range(7):
    z = 0.165 + i * 0.044
    R((0, 0.028, z), 0.0165, 0.0026, LIT, G, sides=18, tsides=5, glow="accent", k=0.7)       # coils
    R((0, 0.028, z + 0.022), 0.0125, 0.0018, STL, G, sides=16, tsides=4)                      # spacers
for d in (1, -1):
    C((d * 0.02, 0.028, 0.30), 0.0024, 0.32, STL, G, rot=X90, sides=8)                        # tie rods
P((0, 0.0405, 0.30), (0.03, 0.01, 0.32), BLK, G, bev=0.003, seg=2)                            # heat shield
for i in range(8):
    B((0, 0.0457, 0.17 + i * 0.037), (0.018, 0.0012, 0.014), VOID, G)                        # its slots
P((0, 0.012, 0.30), (0.03, 0.01, 0.32), DARK, G, bev=0.003)                                   # lower rail
for z in (0.14, 0.455):
    P((0, 0.026, z), (0.042, 0.042, 0.016), BLK2, G, bev=0.004, seg=2)                         # end caps
P((0, 0.022, 0.48), (0.03, 0.04, 0.018), STL, G, bev=0.003)                                   # front band
R((0, -0.003, 0.48), 0.007, 0.0014, STL, G, rot=(0, 90, 0), sides=12, tsides=4)              # sling loop
C((0, 0.028, 0.663), 0.0145, 0.04, EDGE, G, rot=X90, sides=16)                                # brake
for d in (1, -1):
    for o in (-0.01, 0.006):
        B((d * 0.0142, 0.028, 0.663 + o), (0.004, 0.012, 0.006), VOID, G)                     # its ports
R((0, 0.028, 0.6835), 0.0105, 0.0018, LIT, G, sides=16, tsides=4, glow="accent", k=0.5)     # its crown

# ---- the sights: a hooded front post (lit) on its base, a rear notch with two lit dots ----
B((0, 0.039, 0.63), (0.012, 0.008, 0.016), BLK, G, bev=0.002)
B((0, 0.047, 0.63), (0.0028, 0.010, 0.004), LIT, G, glow="glow", k=1.0)                      # post (top at 0.052)
R((0, 0.0475, 0.63), 0.011, 0.0014, BLK, G, sides=18, tsides=4)                              # hood
B((0, 0.043, 0.112), (0.026, 0.008, 0.018), BLK, G, bev=0.002)
for d in (1, -1):
    B((d * 0.008, 0.0485, 0.112), (0.005, 0.007, 0.012), STL, G)                               # notch ears (top 0.052)
    E((d * 0.008, 0.0495, 0.1185), (0.003, 0.003, 0.002), LIT, G, seg=6, rings=4, glow="glow", k=1.2)

# ---- the lance: a lug under the barrel and a blade past the muzzle, its edge lit ----
B((0, 0.006, 0.555), (0.012, 0.014, 0.032), STL, G, bev=0.002)
W((0, 0.004, 0.66), (0.004, 0.02, 0.19), EDGE, G, taper=0.06)                                 # blade
B((0, -0.0045, 0.645), (0.0046, 0.0018, 0.15), LIT, G, glow="accent", k=0.9)                 # its edge
B((0, 0.012, 0.64), (0.005, 0.003, 0.13), STL2, G)                                           # its spine

# ---- the trigger group: a guard, a curved trigger, the floorplate ----
B((0, -0.062, 0.002), (0.006, 0.004, 0.05), STL, G)
B((0, -0.046, 0.027), (0.006, 0.03, 0.004), STL, G, rot=(-12, 0, 0))
B((0, -0.048, -0.022), (0.006, 0.026, 0.004), STL, G, rot=(10, 0, 0))
P((0, -0.042, 0.004), (0.004, 0.022, 0.006), STL, G, rot=(18, 0, 0), top=(1.0, 1.4))         # trigger
B((0, -0.027, 0.06), (0.03, 0.006, 0.06), STL2, G, bev=0.001)                                 # floorplate

# ---- the grip and the stock: a bone pistol grip with grooves and a cap; a skeleton thumbhole stock (top beam,
#      bone cheek riser, lower beam), a butt plate and its pad, a lit line, a sling loop ----
P((0, -0.07, -0.05), (0.033, 0.082, 0.046), BONE, G, rot=(-20, 0, 0), top=(1.05, 1.2), bev=0.013, seg=3)   # grip
E((0, -0.068, -0.068), (0.034, 0.06, 0.02), BONE, G, rot=(-20, 0, 0), seg=12, rings=6)       # its palm swell
for i in range(3):
    P((0, -0.055 - i * 0.02, -0.029 + i * 0.0065), (0.031, 0.004, 0.006), "bone*0.6", G, rot=(-18, 0, 0))
P((0, -0.112, -0.064), (0.036, 0.008, 0.048), BLK, G, rot=(-18, 0, 0), bev=0.003)            # its cap
P((0, 0.005, -0.19), (0.036, 0.034, 0.21), BLK, G, bev=0.005, seg=2)                         # top beam
P((0, 0.029, -0.20), (0.03, 0.014, 0.13), BONE, G, bev=0.005, seg=2)                         # cheek riser
P((0, -0.097, -0.185), (0.03, 0.02, 0.22), BLK, G, bev=0.004)                                # lower beam, into the grip
P((0, -0.04, -0.302), (0.044, 0.135, 0.018), BLK2, G, bev=0.004)                             # butt plate
P((0, -0.04, -0.315), (0.046, 0.14, 0.01), VOID, G, bev=0.003)                               # its pad
B((-0.0182, 0.005, -0.19), (0.002, 0.004, 0.17), LIT, G, glow="accent", k=0.7)               # lit line
B((-0.0152, -0.097, -0.20), (0.002, 0.003, 0.15), LIT, G, glow="accent", k=0.5)
R((0, -0.111, -0.235), 0.008, 0.0015, STL, G, rot=(0, 90, 0), sides=12, tsides=4)            # sling loop

# ---- the bolt: its collar, the handle bent down, a knob with a cyan ring ----
BO = "bolt"
C((0.022, 0.022, -0.035), 0.0068, 0.008, STL, BO, rot=(0, 0, -90), sides=12)
C((0.037, 0.0195, -0.035), 0.0038, 0.032, STL, BO, rot=(0, 0, -105), sides=10)
E((0.055, 0.0145, -0.035), (0.017, 0.017, 0.017), STL, BO, seg=14, rings=8)
R((0.055, 0.0145, -0.035), 0.0086, 0.0013, LIT, BO, rot=(0, 90, 0), sides=16, tsides=4, glow="accent", k=0.8)

save_built("lancer")
