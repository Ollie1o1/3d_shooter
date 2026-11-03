# THE RESTORER's pieces (Chapter 5's boss), built in Blender: each saved to assets/props/restorer_<piece>.glb,
# which the Restorer's rig (EnemyModel.h, case RESTORER) places on its stilts and arms. The old code boxes
# are the fallback when a file is missing.
#
#   make restorer-art                                   (all of them, no window)
#   blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/restorer_build.py ONLY=dome
#
# Game space, metres: +Y up, +Z forward. Each piece's origin is its centre. The limb pieces (strut, hose, jaw)
# are a 1 m unit along +Y, about 1 m across: the rig stretches them between two joints (x and z by the limb's
# thickness, y by its length), so their detail is laid out in that unit.
#
# Its look: careful and cold. Pale blue-grey steel, a coat of frost-crusted hoses, brass fittings, a glass dome
# with one cold lamp in it ("glow": the rig tints it with its tell), a cryo cylinder on its back with the small
# unit it saved first asleep inside.
import os

_here = "/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/"
exec(open(_here + "enemy_kit.py").read())
exec(open(_here + "boss_kit.py").read())
ONLY = globals().get("ONLY")
_og.PALETTE.update({
    "rs_steel": (0.42, 0.47, 0.52), "rs_dark": (0.11, 0.12, 0.14), "rs_frost": (0.84, 0.89, 0.94),
    "rs_hose": (0.50, 0.56, 0.62), "rs_brass": (0.58, 0.44, 0.20), "rs_glass": (0.55, 0.72, 0.82),
    "rs_rubber": (0.07, 0.07, 0.08), "rs_label": (0.86, 0.62, 0.20), "rs_unit": (0.30, 0.32, 0.35),
})
SURF.update({"rs_steel": "metal", "rs_dark": "metal"})


def want(name):
    return not ONLY or ONLY == name


def frost_crust(c, s, n, seed=1):
    """Lumps of frost crusted on a surface round c, inside s"""
    import random
    rnd = random.Random(seed)
    for i in range(n):
        p = (c[0] + (rnd.random() - 0.5) * s[0], c[1] + (rnd.random() - 0.5) * s[1], c[2] + (rnd.random() - 0.5) * s[2])
        r = 0.03 + rnd.random() * 0.05
        E(p, (r * 2.2, r * 1.2, r * 2.0), "rs_frost", None, seg=8, rings=4)


# ---- the dome: its head. A glass bell on a brass collar, one cold lamp hanging in it, a visor ring ----------
if want("dome"):
    begin()
    E((0, 0.05, 0), (0.95, 0.85, 0.95), "rs_glass", None, seg=24, rings=12)                    # the glass
    E((0, 0.05, 0), (0.86, 0.77, 0.86), "rs_dark", None, seg=20, rings=10)                     # dark inside it
    R((0, -0.3, 0), 0.47, 0.05, "rs_brass", None, sides=28, tsides=8)                          # the collar
    C((0, -0.37, 0), 0.45, 0.1, "rs_steel", None, sides=24)
    R((0, 0.05, 0), 0.475, 0.018, "rs_steel", None, sides=28, tsides=4)                        # the visor ring
    for i in range(6):                                                                          # ribs over the glass
        a = math.radians(i * 60)
        rod((math.sin(a) * 0.44, -0.25, math.cos(a) * 0.44), (math.sin(a) * 0.2, 0.42, math.cos(a) * 0.2), 0.012, "rs_steel", sides=6)
    rod((0, 0.42, 0), (0, 0.1, 0.05), 0.012, "rs_dark", sides=6)                                # the lamp's stalk
    E((0, 0.05, 0.08), (0.24, 0.24, 0.24), "glow", None, glow="glow", k=1.4, seg=16, rings=8)  # THE LAMP (its tell)
    E((0, 0.05, 0.08), (0.12, 0.12, 0.12), "glow*2", None, glow="glow", k=2.4, seg=12, rings=6)
    for s in (-1, 1):                                                                           # lens housings either side
        C((s * 0.47, -0.05, 0.1), 0.08, 0.08, "rs_brass", None, rot=(0, 0, 90), sides=12)
        C((s * 0.52, -0.05, 0.1), 0.05, 0.04, "rs_dark", None, rot=(0, 0, 90), sides=10)
    frost_crust((0, -0.3, 0), (0.9, 0.08, 0.9), 14, 3)
    save_piece("restorer_dome")

# ---- the torso: a stooped steel barrel wrapped in a coat of frosted hoses, a brass badge, vents --------------
if want("torso"):
    begin()
    P((0, 0, 0), (1.3, 1.45, 0.9), "rs_steel", None, top=(0.8, 0.85), bev=0.12, seg=3)        # the barrel
    B((0, 0.1, 0.4), (0.9, 0.9, 0.12), "rs_dark", None, bev=0.05)                               # its chest panel
    for i in range(4):                                                                          # vents in it
        B((0, -0.15 + i * 0.16, 0.47), (0.7, 0.05, 0.03), "rs_steel*0.7", None)
    B((0.0, 0.48, 0.47), (0.36, 0.14, 0.03), "rs_label", None, bev=0.01)                        # its badge: UNIT ?
    B((0.0, 0.48, 0.488), (0.05, 0.08, 0.01), "rs_dark", None)
    R((0, 0.75, 0), 0.32, 0.06, "rs_brass", None, rot=(0, 0, 0), sides=24, tsides=8)            # the neck's ring
    for s in (-1, 1):                                                                           # shoulder sockets: upper and lower arms
        E((s * 0.66, 0.5, 0.15), (0.34, 0.34, 0.34), "rs_steel*0.9", None, seg=16, rings=8)
        E((s * 0.62, -0.1, 0.2), (0.26, 0.26, 0.26), "rs_steel*0.9", None, seg=14, rings=8)
        R((s * 0.66, 0.5, 0.15), 0.17, 0.03, "rs_brass", None, rot=(0, 0, 90), sides=16, tsides=6)
    # the hoses: a coat of them hanging off its shoulders, frost caked on
    for i in range(9):
        x = -0.6 + i * 0.15
        top, bot = (x, 0.68, 0.32 if abs(x) < 0.45 else 0.1), (x * 1.15, -0.85 - 0.12 * (i % 3), 0.42 if abs(x) < 0.45 else 0.2)
        mid = ((top[0] + bot[0]) / 2 + 0.03 * (i % 2), (top[1] + bot[1]) / 2, max(top[2], bot[2]) + 0.12)
        tube([top, mid, bot], [0.045, 0.05, 0.045], "rs_hose" if i % 3 else "rs_rubber", None, sides=8)
        R(mid, 0.055, 0.012, "rs_brass", None, rot=(90, 0, 0), sides=10, tsides=4)
    for i in range(6):                                                                          # and round its back
        x = -0.5 + i * 0.2
        tube([(x, 0.6, -0.42), (x * 1.1, 0.0, -0.55), (x, -0.7, -0.45)], [0.04, 0.045, 0.04], "rs_hose", None, sides=8)
    frost_crust((0, 0.55, 0.2), (1.3, 0.3, 0.7), 26, 5)
    frost_crust((0, -0.6, 0.3), (1.3, 0.5, 0.5), 18, 6)
    save_piece("restorer_torso")

# ---- the pelvis: a wide steel saddle with the two hip drums ---------------------------------------------------
if want("pelvis"):
    begin()
    B((0, 0, 0), (1.5, 0.4, 0.8), "rs_steel*0.9", None, bev=0.1, seg=2)
    for s in (-1, 1):
        C((s * 0.72, 0, 0), 0.26, 0.3, "rs_dark", None, rot=(0, 0, 90), sides=18)
        R((s * 0.72, 0, 0), 0.27, 0.03, "rs_brass", None, rot=(0, 0, 90), sides=18, tsides=6)
    for i in range(5):
        tube([(-0.5 + i * 0.25, 0.18, 0.4), (-0.52 + i * 0.26, -0.25, 0.45)], [0.035, 0.035], "rs_hose", None, sides=6)
    frost_crust((0, 0.2, 0), (1.4, 0.05, 0.7), 10, 7)
    save_piece("restorer_pelvis")

# ---- the cryo cylinder on its back: frosted glass between brass caps; asleep inside, the small unit it saved first
if want("cylinder"):
    begin()
    C((0, 0, 0), 0.47, 1.45, "rs_glass", None, sides=24)                                       # the glass
    C((0, 0, 0), 0.43, 1.4, "rs_dark*1.5", None, sides=20)                                     # the cold inside
    for y in (-0.78, 0.78):
        C((0, y, 0), 0.5, 0.14, "rs_brass", None, sides=24)
        R((0, y, 0), 0.5, 0.025, "rs_steel", None, sides=24, tsides=6)
    for i in range(4):                                                                          # its cage bars
        a = math.radians(45 + i * 90)
        rod((math.sin(a) * 0.49, -0.72, math.cos(a) * 0.49), (math.sin(a) * 0.49, 0.72, math.cos(a) * 0.49), 0.02, "rs_steel", sides=6)
    # the small unit: knees up, head down, a dim light still in it
    E((0, 0.2, 0.02), (0.2, 0.2, 0.2), "rs_unit", None, seg=12, rings=6)                       # head
    B((0, -0.08, 0.0), (0.22, 0.32, 0.16), "rs_unit", None, bev=0.04)                           # body
    for s in (-1, 1):
        B((s * 0.07, -0.32, 0.1), (0.07, 0.24, 0.07), "rs_unit*0.9", None, rot=(40, 0, 0))     # legs drawn up
        B((s * 0.13, -0.05, 0.06), (0.05, 0.22, 0.05), "rs_unit*0.9", None, rot=(30, 0, s * 10))
    E((0.04, 0.2, 0.11), (0.03, 0.02, 0.01), "glow", None, glow="const", k=(0.15, 0.25, 0.35), seg=8, rings=4)
    C((0, 0, 0.47), 0.06, 0.06, "rs_label", None, rot=(90, 0, 0), sides=10)                    # a tag on the glass
    frost_crust((0, 0.5, 0), (0.95, 0.4, 0.95), 16, 9)
    frost_crust((0, -0.6, 0), (0.95, 0.2, 0.95), 10, 10)
    save_piece("restorer_cylinder")

# ---- a limb: the stilts' and arms' segments. A dark core in a steel sleeve, brass bands, a piston down it ---
if want("strut"):
    begin()
    C((0, 0, 0), 0.32, 1.0, "rs_dark", None, sides=12)
    C((0, 0.12, 0), 0.42, 0.62, "rs_steel", None, sides=12, r2=0.36)
    for y in (-0.42, 0.42):
        C((0, y, 0), 0.45, 0.08, "rs_brass", None, sides=14)
    C((0.3, -0.05, 0.2), 0.08, 0.8, "rs_steel*1.2", None, sides=8)                             # a piston
    C((0.3, -0.42, 0.2), 0.05, 0.2, "rs_brass*1.2", None, sides=8)
    tube([(-0.35, 0.45, 0.1), (-0.44, 0.0, 0.18), (-0.36, -0.45, 0.12)], [0.07, 0.08, 0.07], "rs_hose", None, sides=6)
    frost_crust((0, 0.3, 0), (0.8, 0.3, 0.8), 6, 11)
    save_piece("restorer_strut")

# ---- a hose (its coat's loose ones, between joints) -----------------------------------------------------------
if want("hose"):
    begin()
    C((0, 0, 0), 0.42, 1.0, "rs_hose", None, sides=12)
    for i in range(6):
        R((0, -0.42 + i * 0.17, 0), 0.43, 0.06, "rs_hose*0.8", None, sides=12, tsides=4)
    frost_crust((0, 0.2, 0.2), (0.7, 0.6, 0.4), 6, 12)
    save_piece("restorer_hose")

# ---- a joint: a brass-ringed drum --------------------------------------------------------------------------------
if want("joint"):
    begin()
    C((0, 0, 0), 0.5, 0.7, "rs_dark", None, rot=(0, 0, 90), sides=16)
    for s in (-1, 1):
        R((s * 0.36, 0, 0), 0.5, 0.06, "rs_brass", None, rot=(0, 0, 90), sides=16, tsides=6)
        C((s * 0.4, 0, 0), 0.25, 0.08, "rs_steel", None, rot=(0, 0, 90), sides=12)
    frost_crust((0, 0.4, 0), (0.7, 0.1, 0.6), 5, 13)
    save_piece("restorer_joint")

# ---- a foot: a padded claw of three toes, frost on it -----------------------------------------------------------
if want("foot"):
    begin()
    B((0, 0.1, 0.1), (0.6, 0.2, 0.7), "rs_steel*0.9", None, bev=0.06)
    B((0, -0.02, 0.1), (0.62, 0.06, 0.74), "rs_rubber", None, bev=0.02)
    for i, x in enumerate((-0.22, 0.0, 0.22)):
        W((x, 0.02, 0.55), (0.12, 0.1, 0.3), "rs_dark", None, rot=(90, 0, 0), taper=0.3)
    C((0, 0.3, 0), 0.16, 0.2, "rs_brass", None, sides=12)
    frost_crust((0, 0.2, 0.1), (0.5, 0.05, 0.6), 6, 14)
    save_piece("restorer_foot")

# ---- a tong's jaw: a long hooked finger, a rubber pad inside its curl --------------------------------------------
if want("jaw"):
    begin()
    C((0, 0.1, 0), 0.32, 0.8, "rs_steel", None, sides=10, r2=0.22)
    W((0, -0.38, 0.12), (0.4, 0.3, 0.45), "rs_dark", None, rot=(-40, 0, 0), taper=0.25)        # the hook
    B((0, -0.25, 0.25), (0.3, 0.25, 0.1), "rs_rubber", None, bev=0.03)
    R((0, 0.45, 0), 0.34, 0.05, "rs_brass", None, sides=12, tsides=4)
    save_piece("restorer_jaw")
