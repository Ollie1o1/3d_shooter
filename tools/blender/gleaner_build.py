# THE GLEANER's pieces (Chapter 4), built in Blender: each saved to assets/props/gleaner_<piece>.glb, which
# GleanerRig.h places on its skeleton (the old code boxes are the fallback when a file is missing).
#
#   make gleaner-art                                   (all of them, no window)
#   blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/gleaner_build.py ONLY=fist
#
# Or open Blender, run this script in the Scripting tab (it leaves the last piece in the scene), change
# what you like, and run save_piece("gleaner_fist") again. Every piece is in the Gleaner's own units
# (1 unit = 2.4 m in the game), game space: +Y up, +Z forward (Blender -Y). Where each piece's origin is,
# and which way it faces, is said above it: that is where the rig puts it.
# Materials: gl_rust (plate), gl_iron (cast iron), gl_bone, gl_soot (char), gl_heat (heat-cracked iron),
# gl_steel, gl_unit (wrecked units); a part glowing on "glow" takes the Gleaner's furnace light.
import os

_here = "/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/"
exec(open(_here + "enemy_kit.py").read())
exec(open(_here + "boss_kit.py").read())
ONLY = globals().get("ONLY")


def want(name):
    return not ONLY or ONLY == name


# ---- the fist: origin its centre, knuckles forward (+Z), resting on them (the ground at y -14) -------------
if want("fist"):
    begin()
    plate((0, 0, -2), (34, 22, 30), "gl_iron", bev=4)                       # the hand's mass
    plate((0, -11, -4), (30, 4, 24), "gl_soot", bev=1.5)                    # its underside, burnt
    for i in range(3):                                                       # the back of the hand: overlapping plates
        plate((0, 11.2 - i * 0.6, -11 + i * 9), (31 - i * 2, 3, 11), "gl_rust", rot=(-8, 0, 0), rivets_r=0.6)
    for x in (-12, -4, 4, 12):                                               # the knuckles, the fingers curled under
        plate((x, -5, 14), (7, 12, 10), "gl_bone", bev=1.6)
        plate((x, -11.5, 9), (6.5, 5, 8), "gl_rust", rot=(25, 0, 0), rivets_r=0.45)
        spike((x, 1, 17), (0, 0.45, 1), 6, 1.5)
        for y in (-1, -7):
            E((x, y, 19.2), (1.4, 1.0, 1.0), "gl_steel", None, seg=8, rings=4)
    for x in (-8, 0, 8):                                                     # light in the joints between them
        crack((x, 0, 13.5), (x, -10, 12.5), w=0.9, k=1.3)
    for sx in (-1, 1):                                                       # its flanks
        plate((sx * 17.6, 0, -2), (2.5, 18, 26), "gl_rust", rivets_r=0.5)
        crack((sx * 18.9, -4, -12), (sx * 18.9, 3, 8), w=0.6, k=0.7)
    C((0, 0, -17.5), 12.5, 4, "gl_heat", None, rot=(90, 0, 0), sides=14)      # the wrist ring
    C((0, 0, -20), 11.6, 1.2, "gl_ember", None, rot=(90, 0, 0), sides=14, glow="glow", k=1.1)
    rod((5, 11, -4), (9, 19, -8), 0.5, "gl_steel", sides=6)                  # scrap fused into it: rebar
    rod((-8, 11, 0), (-12, 18, 3), 0.5, "gl_steel", sides=6)
    rod((-2, 11, -14), (1, 17, -18), 0.4, "gl_steel", sides=6)
    save_piece("gleaner_fist")

# ---- the forearm: from the elbow (origin) 86 along +Z to the wrist, +Y its top ------------------------------
if want("forearm"):
    begin()
    plate((0, 0, 43), (18, 17, 86), "gl_iron", bev=3)
    for i in range(4):                                                       # armour down its top
        plate((0, 9.6, 10 + i * 19.5), (20 - i, 3, 17.5), "gl_rust", rot=(-4, 0, 0), rivets_r=0.55)
    for sx in (-1, 1):
        for i in range(3):
            if sx > 0 and i == 1:
                continue                                                     # (one plate torn away: below)
            plate((sx * 10, 0, 14 + i * 27), (2.5, 14, 23), "gl_rust", rivets_r=0.5)
    for i in range(3):                                                       # bone spines along its back
        spike((0, 11, 20 + i * 22), (0, 1, -0.5), 12 - 2 * i, 1.6)
    for dx in (-4, 0, 4):                                                    # the cable bundle under it
        cable([(dx, -9, 2), (dx, -11, 28), (dx, -11.5, 58), (dx, -9, 84)], 0.9)
    for z in (28, 56):                                                       # seams of light round it
        C((0, 0, z), 10.4, 1.2, "gl_ember", None, rot=(90, 0, 0), sides=10, glow="glow", k=0.7)
    B((10.6, 0, 41), (0.6, 9, 12), "gl_ember", None, glow="glow", k=1.5)     # the tear: its insides burning
    plate((13, 5, 44), (2, 6, 9), "gl_rust", rot=(0, 0, -38))                # the plate, peeled back
    rod((10, 1, 39), (16, 6, 36), 0.35, "gl_steel", sides=6)
    rod((10, -2, 44), (15, -6, 47), 0.3, "gl_steel", sides=6)
    plate((0, 0, 84), (20, 19, 4), "gl_heat", bev=1)                         # the wrist's collar
    save_piece("gleaner_forearm")

# ---- the upper arm: from the shoulder (origin) 80 along +Z to the elbow, +Y its top --------------------------
if want("upperarm"):
    begin()
    plate((0, 0, 40), (21, 19, 80), "gl_iron", bev=3.5)
    plate((0, 11, 40), (17, 6, 60), "gl_rust", rivets_r=0.7)                 # the great plate down it
    plate((0, 14.2, 34), (11, 2, 34), "gl_iron", rivets_r=0.5)
    for sx in (-1, 1):
        rod((sx * 12, -3, 8), (sx * 12, -3, 70), 1.6, "gl_steel", sides=10)  # its pistons
        rod((sx * 12, -3, 8), (sx * 12, -3, 40), 2.4, "gl_iron", sides=10)
        for i in range(2):
            plate((sx * 11, 4, 18 + i * 34), (2.5, 9, 26), "gl_rust", rivets_r=0.5)
    B((0, -10, 50), (8, 0.6, 14), "gl_ember", None, glow="glow", k=1.5)     # torn open underneath
    for i in range(4):
        plate((-5 + i * 3.5, -10.6, 43 + (i % 2) * 14), (3, 1.2, 5), "gl_rust", rot=(0, i * 20, 15 - i * 9))
    for z in (20, 62):
        C((0, 0, z), 12, 1.2, "gl_ember", None, rot=(90, 0, 0), sides=10, glow="glow", k=0.6)
    chain((-6, -10, 28), (-6, -24, 30), 6, 0.6)                              # a trophy on a chain
    wreck((-6, -27.5, 30), 3.0, 30)
    save_piece("gleaner_upperarm")

# ---- the pauldron: origin over the shoulder (the rig's shoulder + 8 up), +Z forward ---------------------------
if want("pauldron"):
    begin()
    plate((0, 0, 0), (40, 10, 40), "gl_rust", bev=4, rivets_r=0.7)
    plate((0, 7, -2), (32, 8, 32), "gl_rust", rot=(-6, 0, 0), rivets_r=0.6)
    plate((0, 12.5, -4), (22, 5, 22), "gl_iron", bev=2)
    for i in range(6):
        a = i * math.tau / 6 + 0.3
        spike((math.cos(a) * 14, 11, math.sin(a) * 14), (math.cos(a) * 0.6, 1, math.sin(a) * 0.6), 14 - (i % 2) * 4, 2)
    for z in (-10, 0, 10):
        crack((-20.5, 1, z - 3), (-20.5, 1, z + 3), w=0.8, k=0.8)
        crack((20.5, 1, z - 3), (20.5, 1, z + 3), w=0.8, k=0.8)
    chain((18, -4, 10), (19, -20, 11), 6, 0.7)
    wreck((19, -23.5, 11), 3.2, 40)
    chain((-17, -4, -12), (-18, -14, -13), 4, 0.6)
    save_piece("gleaner_pauldron")

# ---- the skull: origin the head's centre, facing +Z (the jaw and the mouth's light are their own) -------------
if want("skull"):
    begin()
    plate((0, 1, 0), (26, 18, 34), "gl_bone", bev=4)
    plate((0, 9.4, -2), (22, 2.4, 26), "gl_iron", rot=(-3, 0, 0), rivets_r=0.6)      # an iron cap bolted on (under its crown: you walk there)
    plate((0, 7, 10), (29, 4, 14), "gl_rust", rot=(10, 0, 0), rivets_r=0.6)          # the brow
    for sx in (-1, 1):
        spike((sx * 12.5, 4, 2), (sx * 1, 0.3, 0.2), 10, 2)
        plate((sx * 13.6, 0, 0), (2, 12, 26), "gl_iron", rivets_r=0.5)               # cheek plates
        plate((sx * 7, 3, 16.6), (8, 6, 1.6), "gl_iron", bev=0.6)                    # the sockets' rims
    crack((-13.1, 6, -12), (-13.1, 1, 6), 0.5, 0.8)                                 # its skull cracked, lit inside
    crack((13.1, 7, -8), (13.1, 2, 6), 0.5, 0.7)
    crack((-12, 4, -10), (-12.8, -2, 6), 0.5, 0.6)
    for i in range(5):                                                               # the nose: a grille
        B((-4 + i * 2, -2, 17.4), (0.7, 6, 1), "gl_soot", None)
    teeth((-10, -9.5, 16), (4, 0, 0), 6, (2, 5, 2))
    for sx in (-1, 1):
        teeth((sx * 11, -9.5, 12), (0, 0, -4.5), 4, (2, 4.5, 2))
    save_piece("gleaner_skull")

# ---- the jaw: origin its centre (the rig hinges it at the back), facing +Z --------------------------------
if want("jaw"):
    begin()
    plate((0, 0, 0), (23, 6, 32), "gl_bone", bev=1.5)
    plate((0, -3.4, -2), (20, 2, 26), "gl_iron", rivets_r=0.5)
    teeth((-10, 3, 14), (4, 0, 0), 6, (2, 5, 2), down=False)
    for sx in (-1, 1):
        teeth((sx * 10.5, 3, 10), (0, 0, -4.5), 4, (2, 4.5, 2), down=False)
        plate((sx * 11.8, 0, -4), (1.6, 5, 18), "gl_rust", rivets_r=0.4)
    save_piece("gleaner_jaw")

# ---- an armour scale of its spine: 50 wide, 25 long (the rig stretches it to each vertebra), +Z toward its head
if want("scale"):
    begin()
    plate((0, 0, 0), (50, 4, 25), "gl_rust", bev=1.5)
    plate((0, 2.6, -4), (40, 2, 14), "gl_rust", rot=(-6, 0, 0), rivets_r=0.55)
    plate((0, 3.7, -6), (26, 1.5, 8), "gl_iron", rivets_r=0.45)
    crack((-18, 2.3, 6), (-6, 2.5, 10), 0.5, 0.6)
    crack((8, 2.3, 4), (19, 2.2, 9), 0.4, 0.5)
    rivets((-23, 2.1, 11.5), (23, 2.1, 11.5), 12, 0.5)
    save_piece("gleaner_scale")

# ---- a charm: a rejected unit on its hook, its chest at y -1.3 (in units of twice a unit's size) ---------------
if want("charm"):
    begin()
    wreck((0, -1.3, 0), 1.25, 0)
    chain((0, 0, 0), (0, -0.7, 0), 2, 0.08)
    save_piece("gleaner_charm")
