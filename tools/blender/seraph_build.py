# The SERAPH in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/seraph_build.py).
# An ivory and gold construct that floats high and far: a ribbed body with a lens in its chest ("core": it blazes as
# the beam charges and sweeps), a tail of vertebrae tapering to a point, a ring for a head with an eye at its centre,
# and two pairs of feathered wings whose edges burn as they flare ("edge").
#   head (0, 1.42, 0): its ring, facing +Z (head hitbox: x, y within +-0.38 of it, z within +-0.17)
#   wingL/R (+-0.2, 1.05, -0.05): the upper pair, at rest raised 0.5 rad and swept back 0.25 (built flat, turned)
#   tipL/R (+-0.2, 0.70, -0.05): the lower pair, at rest lowered 0.15 rad and swept back 0.25
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("seraph", joints=globals().get("SKELETON")); clear_parts()

IVORY, IVORY2, DARK = "seraph*1.0", "seraph*1.15", "seraph*0.5"
GOLD, GOLD2 = "gold", "gold*0.7"
LIGHT = "glow*0.3"
RT = "seraph"

# ---- the body: a ribbed torso in ivory plate, a gold collar, the lens ----
E((0, 0.86, 0.0), (0.4, 0.8, 0.3), IVORY, RT, seg=14, rings=8)
for k in range(4):
    R((0, 0.6 + 0.12 * k, 0.0), 0.17 + 0.015 * k, 0.022, IVORY2, RT, rot=(90, 0, 0), sides=18, tsides=4)   # ribs
P((0, 1.0, 0.1), (0.34, 0.36, 0.1), IVORY2, RT, top=(1.15, 1.0), bev=0.02)                      # breastplate
R((0, 0.96, 0.165), 0.1, 0.018, GOLD, RT, sides=20, tsides=4)
E((0, 0.96, 0.17), (0.15, 0.15, 0.06), LIGHT, RT, glow="core", k=1.0, seg=12, rings=6)          # the lens
C((0, 1.25, 0.0), 0.16, 0.06, GOLD2, RT, sides=16)                                              # collar
C((0, 1.31, 0.0), 0.05, 0.08, IVORY2, RT, sides=8)                                              # neck
# its tail: vertebrae tapering to a point
for k in range(5):
    E((0, 0.42 - 0.09 * k, -0.01 * k), (0.15 - 0.022 * k, 0.07, 0.13 - 0.018 * k), IVORY2 if k % 2 else IVORY, RT, seg=10, rings=5)
K((0, 0.02, -0.05), 0.04, 0.16, GOLD, RT, rot=(180, 0, 0), sides=6)
for k in range(3):   # gold bands on it
    R((0, 0.42 - 0.09 * k - 0.045, -0.01 * k), 0.065 - 0.01 * k, 0.012, GOLD2, RT, rot=(90, 0, 0), sides=12, tsides=4)

# ---- the head: a ring of eight plates round an eye ----
H = "head"
R((0, 1.42, 0.0), 0.27, 0.045, IVORY, H, sides=28, tsides=6)
R((0, 1.42, 0.0), 0.2, 0.02, GOLD, H, sides=24, tsides=4)
for k in range(8):
    a = k * math.pi / 4
    B((0.3 * math.sin(a), 1.42 + 0.3 * math.cos(a), 0.0), (0.09, 0.07, 0.07), IVORY2, H, rot=(0, 0, -math.degrees(a)), bev=0.012)
    B((0.3 * math.sin(a), 1.42 + 0.3 * math.cos(a), 0.04), (0.04, 0.03, 0.02), LIGHT, H, rot=(0, 0, -math.degrees(a)), glow="glow", k=0.8)
E((0, 1.42, 0.0), (0.12, 0.12, 0.1), LIGHT, H, glow="glow", k=1.8, seg=10, rings=6)              # the eye
for s in (1, -1):
    C((s * 0.11, 1.42, 0.0), 0.012, 0.1, GOLD2, H, rot=(0, 0, 90), sides=6)                      # its spokes
C((0, 1.31, 0.0), 0.012, 0.1, GOLD2, H, sides=6)


# ---- wings: each a gold-boned arm with feathers of ivory, their tips burning; built flat out along +-X, then turned
#      to their rest angle about their joint ----
def wing(s, joint, at, span, lift, n):
    before = parts_now()
    ax, ay, az = at
    C((ax + s * span * 0.45, ay, az), 0.025, span * 0.9, GOLD2, joint, rot=(0, 0, 90), sides=8)     # its bone
    for k in range(n):   # feathers hang down and back from the bone: the wing faces forward
        f = (k + 0.5) / n
        L = (0.62 - 0.22 * abs(f - 0.55)) * (span / 1.2) ** 0.5                                     # feather length
        d = Vector((s * 0.12 * f, -0.78, -0.6)).normalized()
        c = Vector((ax + s * span * f, ay, az)) + d * (L * 0.5)
        P(tuple(c), (span / n * 1.3, L, 0.03), IVORY if k % 2 else IVORY2, joint, rot=_aim(d), top=(0.5, 1.0), bev=0.008)
        if k >= n - 3:   # the outer three burn along their middle
            P(tuple(c + Vector((0, 0, 0.025))), (span / n * 0.35, L * 0.9, 0.02), LIGHT, joint, rot=_aim(d), top=(0.3, 1.0), glow="edge", k=1.0)
    turn_about(new_since(before), at, rot_game('Z', math.degrees(s * lift)) @ rot_game('Y', math.degrees(-s * 0.25)))


for s, up, low in ((1, "wingL", "tipL"), (-1, "wingR", "tipR")):
    wing(s, up, (s * 0.2, 1.05, -0.05), 1.2, 0.5, 7)
    wing(s, low, (s * 0.2, 0.70, -0.05), 0.9, -0.15, 5)

report("seraph")
save_enemy("seraph")
