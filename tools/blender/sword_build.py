# OATHBREAKER: builds assets/guns/sword.glb from nothing, in sword space
# (+Z along the blade from the guard, the flats facing +-Y, +Y the side you
# see at rest, the edges along +-X; ~1 unit = 1 m before SWORD_SCALE).
# Run it in Blender with the OVERDRIVE add-on's helpers loaded, e.g. over the
# live link:  python3 tools/blender/live.py tools/blender/sword_build.py
# (it replaces the scene), or headless (see the bottom).
#
# Fixed by the game: the fuller's 8 cells hang on the "cells" socket at
# (0, 0.011, 0.13), 0.09 apart (the code draws them: the guard meter); the
# hands close on the grip at z -0.055 and -0.165; the point near z 0.94.
import math

for ob in list(bpy.context.scene.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
root = bpy.data.objects.new("sword", None)
bpy.context.scene.collection.objects.link(root)
add_joint("cells", (0.0, 0.011, 0.13), parent=root)
made = []


def box(name, c, s, mat, rot=(0, 0, 0), glow=None, k=1.0, show=None):
    b = add_box(c, s, mat, parent=root, rot=rot, name=name)
    if glow:
        set_glow(b, glow, k)
    if show:
        set_show(b, show)
    made.append(b)
    return b


def both(fn):   # once for each flat: the +Y face (seen at rest) and the -Y face
    for sy in (1, -1):
        fn(sy)


# ---------------------------------------------------------------- the blade
Z0, Z1 = 0.075, 0.80          # the straight of the blade (the point runs on to ~0.945)
ZC, ZL = (Z0 + Z1) / 2, Z1 - Z0
box("blade core", (0, 0, ZC), (0.040, 0.016, ZL), "black")
for sx in (-1, 1):
    box("bevel", (sx * 0.025, 0, ZC), (0.012, 0.010, ZL), "edge*0.85")
    box("honed edge", (sx * 0.0315, 0, ZC), (0.0025, 0.004, ZL), "edge*1.25")
    # the charge line: faint along the bevel, lit by a charged thrust, flaring on a riposte
    for sy in (1, -1):
        box("charge line", (sx * 0.0205, sy * 0.0054, ZC + 0.01), (0.0014, 0.0008, ZL - 0.04), "glow", glow="charge")

# the fuller: a dark inlay framed in steel; the cells (code) sit in it on +Y,
# and a continuous rose line runs down it on -Y
def fuller(sy):
    box("fuller", (0, sy * 0.0086, 0.445), (0.024, 0.0012, 0.71), "black*0.35")
    for sx in (-1, 1):
        box("fuller rim", (sx * 0.0128, sy * 0.0089, 0.445), (0.0016, 0.0012, 0.71), "edge*0.7")
    if sy < 0:
        box("fuller line", (0, sy * 0.0094, 0.445), (0.004, 0.0008, 0.69), "glow", glow="glow", k=0.6)
both(fuller)

# the point: the core stepping in, the bevels and honed edges converging
for z0, z1, w, t in ((0.80, 0.84, 0.030, 0.015), (0.84, 0.875, 0.019, 0.013), (0.875, 0.905, 0.010, 0.011), (0.905, 0.925, 0.004, 0.009)):   # (inside the bevels)
    box("point core", (0, 0, (z0 + z1) / 2), (w, t, z1 - z0), "black")
tip_dx, tip_dz = 0.031, 0.145
ang = math.degrees(math.atan2(tip_dx, tip_dz))
length = math.hypot(tip_dx, tip_dz)
for sx in (-1, 1):
    th = -sx * ang                                   # each side leans in to meet at the point
    cx, cz = sx * tip_dx / 2, Z1 + tip_dz / 2
    box("point bevel", (cx, 0, cz), (0.012, 0.010, length), "edge*0.85", rot=(0, th, 0))
    nx, nz = math.cos(math.radians(th)) * sx, -math.sin(math.radians(th)) * sx
    box("point honed", (cx + nx * 0.0062, 0, cz + nz * 0.0062), (0.0025, 0.004, length - 0.006), "edge*1.25", rot=(0, th, 0))

# the crack: only while the guard is broken, a jagged fissure right across the blade
# (both flats, over the bevels), embers glowing in it, and a split running down the fuller
def crack(sy):
    pts = [(-0.033, 0.418), (-0.021, 0.436), (-0.009, 0.412), (0.002, 0.440), (0.013, 0.414), (0.024, 0.433), (0.033, 0.420)]
    for (x0, z0), (x1, z1) in zip(pts, pts[1:]):
        a = math.degrees(math.atan2(z1 - z0, x1 - x0))
        L = math.hypot(x1 - x0, z1 - z0)
        box("crack", ((x0 + x1) / 2, sy * 0.0102, (z0 + z1) / 2), (L + 0.003, 0.0016, 0.0045), "black*0.15", rot=(0, -a, 0), show="broken")
        box("crack ember", ((x0 + x1) / 2, sy * 0.0112, (z0 + z1) / 2), (L + 0.001, 0.0010, 0.0016), "glow", rot=(0, -a, 0), glow="gem", k=3.0, show="broken")
    for z0, z1, x in ((0.440, 0.52, 0.002), (0.52, 0.575, -0.003)):   # the split, running on toward the point
        a = math.degrees(math.atan2(z1 - z0, 0.005))
        box("split", (x, sy * 0.0112, (z0 + z1) / 2), (0.0012, 0.0010, z1 - z0), "glow", glow="gem", k=2.0, show="broken")
both(crack)

# ---------------------------------------------------------------- the ricasso
box("ricasso", (0, 0, 0.0475), (0.044, 0.020, 0.055), "steel")
box("collar", (0, 0, 0.0735), (0.050, 0.024, 0.007), "brass")
box("collar lip", (0, 0, 0.0785), (0.046, 0.019, 0.003), "black*1.4")
def ricasso_face(sy):
    y = sy * 0.0102
    box("langet", (0, sy * 0.0112, 0.040), (0.022, 0.0016, 0.046), "steel*0.8")
    for z in (0.026, 0.056):
        box("langet rivet", (0, sy * 0.0124, z), (0.004, 0.0014, 0.004), "brass")
    # the maker's mark: a lit diamond, a rune stroke either side
    box("mark socket", (0, sy * 0.0124, 0.041), (0.012, 0.0012, 0.012), "black*0.5", rot=(0, 45, 0))
    box("mark", (0, sy * 0.0131, 0.041), (0.0075, 0.0010, 0.0075), "glow", rot=(0, 45, 0), glow="glow", k=1.1)
    for sx in (-1, 1):
        box("rune", (sx * 0.0175, y + sy * 0.0004, 0.047), (0.0014, 0.0008, 0.016), "glow", glow="glow", k=0.8)
        box("rune foot", (sx * 0.0155, y + sy * 0.0004, 0.0395), (0.0045, 0.0008, 0.0014), "glow", glow="glow", k=0.8)
both(ricasso_face)

# ---------------------------------------------------------------- the crossguard
box("guard block", (0, 0, 0), (0.064, 0.034, 0.040), "black")
box("guard cap", (0, 0, 0.0215), (0.058, 0.030, 0.003), "edge*0.8")
box("guard base", (0, 0, -0.0215), (0.058, 0.030, 0.003), "edge*0.6")
def guard_face(sy):
    box("guard plate", (0, sy * 0.0182, 0), (0.054, 0.0026, 0.032), "brass")
    box("gem socket", (0, sy * 0.0201, 0), (0.017, 0.0016, 0.017), "brass*0.5", rot=(0, 45, 0))
    box("gem", (0, sy * 0.0212, 0), (0.011, 0.0022, 0.011), "glow", rot=(0, 45, 0), glow="gem")
    for sx in (-1, 1):
        box("guard bolt", (sx * 0.021, sy * 0.0199, 0.011), (0.004, 0.0014, 0.004), "black*1.6")
        box("guard bolt", (sx * 0.021, sy * 0.0199, -0.011), (0.004, 0.0014, 0.004), "black*1.6")
both(guard_face)
# the quillons: three segments a side, sweeping forward toward the blade, tapering, brass-tipped
segs = [(0.034, 15, 0.026), (0.030, 32, 0.021), (0.026, 52, 0.016)]
for sx in (-1, 1):
    x, z = 0.031, 0.0
    for i, (L, a, t) in enumerate(segs):
        dx, dz = math.cos(math.radians(a)) * L, math.sin(math.radians(a)) * L
        cx, cz = x + dx / 2, z + dz / 2
        rot = (0, -sx * a, 0)
        box("quillon", (sx * cx, 0, cz), (L + 0.004, t, t), "black", rot=rot)
        for sy in (1, -1):
            box("quillon rail", (sx * cx, sy * (t / 2 + 0.0006), cz), (L, 0.0012, t * 0.55), "edge*0.8", rot=rot)
            if i < 2:
                box("quillon slit", (sx * cx, sy * (t / 2 + 0.0013), cz), (L * 0.7, 0.0008, 0.0018), "glow", rot=rot, glow="glow", k=0.55)
        x, z = x + dx, z + dz
    box("finial", (sx * (x + 0.002), 0, z + 0.002), (0.013, 0.017, 0.013), "brass", rot=(0, 45, 0))
    box("finial tip", (sx * (x + 0.006), 0, z + 0.006), (0.006, 0.010, 0.006), "edge", rot=(0, 45, 0))

# ---------------------------------------------------------------- the grip
box("grip", (0, 0, -0.11), (0.030, 0.034, 0.19), "bone")
for i in range(10):
    z = -0.029 - i * 0.0181
    box("wire", (0, 0, z), (0.0336, 0.0376, 0.0055), "brass" if i % 2 == 0 else "brass*0.8", rot=(0, 0, 7 if i % 2 else -7))
box("ferrule", (0, 0, -0.0215), (0.038, 0.041, 0.011), "steel")
box("ferrule", (0, 0, -0.2), (0.038, 0.041, 0.011), "steel")
for z in (-0.0215, -0.2):
    box("ferrule line", (0, 0, z), (0.0395, 0.0425, 0.0025), "edge*0.9")

# ---------------------------------------------------------------- the pommel
box("pommel neck", (0, 0, -0.2115), (0.026, 0.028, 0.012), "steel*0.8")
PZ = -0.237
for r in (0, 45):   # an octagon in the blade's plane: a brass rim, a black disc on it
    box("pommel rim", (0, 0, PZ), (0.052, 0.018, 0.040), "brass", rot=(0, r, 0))
    box("pommel", (0, 0, PZ), (0.046, 0.024, 0.034), "black", rot=(0, r, 0))
def pommel_face(sy):
    box("pommel socket", (0, sy * 0.0128, PZ), (0.020, 0.0016, 0.020), "brass*0.6", rot=(0, 45, 0))
    box("pommel gem", (0, sy * 0.0139, PZ), (0.013, 0.0022, 0.013), "glow", rot=(0, 45, 0), glow="gem")
    for a in (0, 90):
        box("pommel cross", (0, sy * 0.0127, PZ), (0.0016, 0.0010, 0.030), "edge*0.7", rot=(0, a + 45, 0))
both(pommel_face)
box("end cap", (0, 0, -0.2615), (0.014, 0.014, 0.009), "steel")
box("end gem", (0, 0, -0.267), (0.007, 0.007, 0.004), "glow", glow="gem", k=0.7)

print("sword:", len(made), "boxes")
