# The HUSK, rebuilt in detail: run over the live link (python3 tools/blender/live.py tools/blender/husk_build.py)
# in a scene opened from assets/enemies/husk.glb. Keeps its joints where they are (the code turns them there,
# and the head hitbox is built from the same numbers); replaces every box. Game space: +Y up, faces +Z,
# its right side is -X, metres. Then save it (Save gun with Gun = husk, or save_gun("husk")).
#
#   joints at rest   hipL/hipR (+-0.13, 0.90, 0)   torso (0, 1.04, 0)   head (0, 1.66, 0)
#                    armL/armR (+-0.365, 1.575, 0); the arms hang down -Y and raise to aim along +Z
#   head hitbox      within x, z +-0.22 and y 1.60..2.06 of the head (a test holds it)

ARMOR, DARK, METAL = "husk", "gunmetal*1.3", "gunmetal"   # rust plate over a dark frame

for ob in [o for o in bpy.context.scene.objects if o.type == "MESH"]:
    bpy.data.objects.remove(ob, do_unlink=True)


def B(c, s, mat, parent, rot=(0, 0, 0), glow=None, k=1.0):
    ob = add_box(c, s, mat, parent=parent, rot=rot)
    if glow:
        set_glow(ob, glow, k)
    return ob


# ---- legs: armoured thigh and shin over a dark frame, a mechanical knee, a calf piston, a heavy boot
for side, hip in ((1, "hipL"), (-1, "hipR")):
    x = side * 0.13
    B((x, 0.68, 0.0), (0.18, 0.40, 0.20), DARK, hip)                                   # thigh
    B((x, 0.70, 0.105), (0.16, 0.30, 0.04), ARMOR, hip)                                # thigh plate
    B((x + side * 0.095, 0.72, 0.0), (0.03, 0.26, 0.16), "husk*0.85", hip)             # outer thigh plate
    B((x, 0.46, 0.0), (0.19, 0.10, 0.18), METAL, hip)                                  # knee joint
    B((x, 0.47, 0.11), (0.14, 0.12, 0.07), "husk*1.1", hip, rot=(-12, 0, 0))           # knee cap
    B((x, 0.26, -0.01), (0.15, 0.34, 0.17), DARK, hip)                                 # shin
    B((x, 0.28, 0.09), (0.14, 0.28, 0.04), ARMOR, hip)                                 # shin guard
    B((x, 0.27, -0.105), (0.05, 0.28, 0.05), METAL, hip)                               # calf piston
    B((x, 0.09, 0.0), (0.12, 0.06, 0.12), METAL, hip)                                  # ankle
    B((x, 0.04, 0.05), (0.20, 0.08, 0.31), "husk*0.7", hip)                            # boot
    B((x, 0.075, 0.18), (0.18, 0.05, 0.09), ARMOR, hip, rot=(18, 0, 0))                # toe cap
    B((x, 0.05, -0.11), (0.12, 0.08, 0.06), METAL, hip)                                # heel

# ---- pelvis: rides the body (bob, crouch)
B((0, 0.97, 0.0), (0.40, 0.14, 0.24), DARK, "husk")                                    # pelvis
B((0, 1.02, 0.0), (0.46, 0.05, 0.27), METAL, "husk")                                   # belt
B((0, 1.02, 0.137), (0.08, 0.035, 0.01), "glow*0.3", "husk", glow="glow", k=0.8)       # buckle light
B((0, 0.92, 0.12), (0.15, 0.13, 0.04), ARMOR, "husk", rot=(8, 0, 0))                   # groin plate
for side in (1, -1):
    B((side * 0.235, 0.93, 0.02), (0.035, 0.15, 0.19), "husk*0.85", "husk", rot=(0, 0, side * 8))   # hip plates
B((0, 0.94, -0.12), (0.30, 0.10, 0.04), "husk*0.85", "husk")                           # rear plate

# ---- torso: a narrow armoured gut, a broad chest round a glowing core, a reactor pack on its back
T = "torso"
B((0, 1.17, 0.0), (0.34, 0.24, 0.22), DARK, T)                                         # gut
B((0, 1.11, 0.115), (0.25, 0.07, 0.03), "husk*0.85", T)                                # gut plates
B((0, 1.20, 0.12), (0.27, 0.07, 0.03), "husk*0.85", T)
B((0, 1.44, 0.0), (0.54, 0.34, 0.32), ARMOR, T)                                        # chest
B((0, 1.47, 0.163), (0.42, 0.24, 0.04), "husk*1.08", T, rot=(-6, 0, 0))                # breastplate
B((0, 1.45, 0.186), (0.17, 0.15, 0.016), METAL, T)                                     # core frame
B((0, 1.45, 0.192), (0.11, 0.09, 0.02), "glow*0.3", T, glow="glow", k=1.4)             # core
for side in (1, -1):
    B((side * 0.275, 1.42, 0.02), (0.02, 0.18, 0.20), METAL, T)                        # side vents
B((0, 1.635, 0.0), (0.32, 0.07, 0.28), METAL, T)                                       # gorget
B((0, 1.68, -0.01), (0.13, 0.08, 0.13), METAL, T)                                      # neck
B((0, 1.22, -0.13), (0.06, 0.20, 0.05), METAL, T)                                      # spine
B((0, 1.42, -0.22), (0.38, 0.38, 0.14), "husk*0.6", T)                                 # reactor pack
B((0, 1.62, -0.21), (0.30, 0.04, 0.12), METAL, T)                                      # its frame
for side in (1, -1):
    B((side * 0.11, 1.42, -0.296), (0.05, 0.28, 0.02), "glow*0.3", T, glow="glow", k=0.9)   # cell tubes
    B((side * 0.14, 1.70, -0.24), (0.06, 0.16, 0.06), METAL, T)                              # exhaust stacks
    B((side * 0.14, 1.785, -0.24), (0.045, 0.015, 0.045), "glow*0.3", T, glow="glow", k=0.6)  # their embers

# ---- head: a helmet with a heavy brow, a visor slit and two hot eyes, a respirator, an antenna
H = "head"
B((0, 1.84, -0.01), (0.28, 0.28, 0.30), "husk*0.85", H)                                # helmet
B((0, 1.93, 0.13), (0.30, 0.05, 0.08), ARMOR, H, rot=(10, 0, 0))                       # brow
B((0, 1.78, 0.14), (0.24, 0.14, 0.04), METAL, H)                                       # faceplate
B((0, 1.86, 0.155), (0.22, 0.045, 0.02), "glow*0.3", H, glow="glow", k=0.9)            # visor slit
for side in (1, -1):
    B((side * 0.06, 1.86, 0.168), (0.04, 0.035, 0.01), "glow*0.5", H, glow="glow", k=3.5)   # eyes
    B((side * 0.15, 1.80, 0.06), (0.03, 0.16, 0.16), ARMOR, H)                               # cheek plates
B((0, 1.73, 0.14), (0.14, 0.07, 0.07), METAL, H)                                       # respirator
B((0, 2.00, -0.04), (0.06, 0.04, 0.22), "husk*1.1", H)                                 # crest
B((-0.13, 1.96, -0.10), (0.015, 0.14, 0.015), METAL, H)                                # antenna
B((-0.13, 2.035, -0.10), (0.025, 0.02, 0.025), "glow*0.5", H, glow="glow", k=1.5)      # its light

# ---- arms: pauldrons, armoured upper arm and forearm; the left a fist, the right its orb cannon
for side, arm in ((1, "armL"), (-1, "armR")):
    x = side * 0.37
    B((side * 0.40, 1.63, 0.0), (0.22, 0.09, 0.27), ARMOR, arm, rot=(0, 0, side * -14))           # pauldron
    B((side * 0.47, 1.57, 0.0), (0.04, 0.10, 0.25), "husk*0.85", arm, rot=(0, 0, side * -14))     # its rim
    B((x, 1.42, 0.0), (0.14, 0.28, 0.14), DARK, arm)                                   # upper arm
    B((side * 0.43, 1.42, 0.0), (0.03, 0.22, 0.12), "husk*0.9", arm)                   # bicep plate
    B((x, 1.26, -0.01), (0.15, 0.08, 0.15), METAL, arm)                                # elbow
    B((x, 1.10, 0.0), (0.15, 0.26, 0.16), "husk*0.9", arm)                             # forearm
    if side > 0:
        B((side * 0.445, 1.10, 0.0), (0.012, 0.16, 0.06), "glow*0.3", arm, glow="glow", k=0.8)   # forearm vent
        B((x, 0.92, 0.0), (0.12, 0.12, 0.13), DARK, arm)                               # fist
        B((x, 0.845, 0.03), (0.10, 0.05, 0.08), METAL, arm)                            # knuckles

A, gx = "armR", -0.365
B((gx, 0.95, -0.02), (0.12, 0.10, 0.12), DARK, A)                                      # gun hand
B((gx, 0.86, 0.05), (0.13, 0.24, 0.17), METAL, A)                                      # receiver
B((gx, 0.66, 0.06), (0.10, 0.24, 0.11), "gunmetal*1.4", A)                             # barrel housing
for y in (0.72, 0.65, 0.58):
    B((gx, y, 0.06), (0.12, 0.025, 0.13), METAL, A, glow="shot", k=0.5)                # coils: hotter as the shot builds
B((gx, 0.52, 0.06), (0.09, 0.08, 0.09), METAL, A, glow="shot", k=1.0)                  # muzzle
B((gx, 0.475, 0.06), (0.05, 0.02, 0.05), "glow*0.4", A, glow="shot", k=1.6)            # muzzle core
B((gx, 0.80, 0.145), (0.04, 0.30, 0.03), ARMOR, A)                                     # top rail
B((gx - 0.08, 0.86, 0.04), (0.04, 0.14, 0.08), "glow*0.3", A, glow="shot", k=0.7)      # orb canister

n = sum(1 for o in bpy.context.scene.objects if o.type == "MESH")
print("HUSK:", n, "boxes")
