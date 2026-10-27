# THE PENITENT in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/penitent_build.py).
# A 13 m penitent in rags over blackened iron: a deep hood over a cracked bone mask, an iron rib cage strapped over
# its robe, a collar of great chain links, shackles on its bare bone feet, bell sleeves, and in each fist a chain
# with a caged brass censer that burns ("tellR" the right one, which sweeps; "tellL" the left). Its back is a
# lattice of old scars, stapled shut; while it scourges itself ("open") they split into a burning wound.
#   hipL/R (+-0.95, 4.5, 0)  torso (0, 5.25, 0)  head (0, 9.5, 0)  armL/R (+-2.8, 8.95, 0): arms hang down -Y
#   head hitbox: x, z within +-1.13 of the head, y 9.32..11.58   the wound (woundBox): its back at y ~7.6, z -1.4
# Starts from the saved file's joints; the first build, from a skeleton (SKELETON=<tools/enemy_export --joints penitent>).
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("penitent", joints=globals().get("SKELETON")); clear_parts()

IRON, IRON2, IRON3 = "iron", "iron*1.5", "iron*2.2"     # blackened iron: base, raised, a worn edge
RAG, RAG2, RAG3 = "rag", "rag*1.35", "rag*0.7"          # its robe: base, a lighter wrap, the shadowed folds
BONE, BONE2 = "bone", "bone*0.75"
BRASS, BRASS2 = "censer", "censer*0.7"
LIT = "glow*0.3"                                        # (lit by "light": its hood's rim, its hem, its seam)
RT = "penitent"


# ---- legs: rag-wound iron greaves, a knee plate, a shackle at each ankle with a broken chain, bare bone feet ----
for s, hip in ((1, "hipL"), (-1, "hipR")):
    x = s * 0.95
    C((x, 3.3, 0.0), 0.62, 2.4, RAG3, hip, sides=14, r2=0.7)                                        # thigh, in rags
    for i in range(4):
        C((x, 2.4 + i * 0.55, 0.0), 0.66 + 0.02 * i, 0.22, RAG2 if i % 2 else RAG, hip, rot=(0, 0, s * (6 - 3 * i)), sides=14)   # windings
    E((x, 2.05, 0.30), (1.1, 0.9, 0.7), IRON2, hip, seg=14, rings=8)                                # knee plate
    K((x, 2.05, 0.62), 0.12, 0.35, IRON3, hip, rot=(90, 0, 0), sides=6)
    P((x, 1.05, 0.05), (1.10, 1.70, 1.05), IRON, hip, top=(1.15, 1.1), bev=0.08, seg=2)            # greave
    B((x, 1.1, 0.58), (0.10, 1.4, 0.06), IRON3, hip, bev=0.02)                                     # ridge
    for y in (1.55, 0.95):
        C((x, y, 0.05), 0.62, 0.16, RAG2, hip, rot=(0, 0, s * 8), sides=14)                        # rag straps
    # the shackle
    R((x, 0.42, 0.0), 0.62, 0.11, IRON2, hip, rot=(90, 0, 0), sides=20, tsides=6)
    B((x + s * 0.66, 0.42, 0.0), (0.16, 0.26, 0.3), IRON3, hip, bev=0.03)                          # its lock
    for i in range(2):
        R((x + s * (0.9 + i * 0.36), 0.38 - i * 0.06, 0.05), 0.2, 0.06, IRON2, hip, rot=(0, 90 * (i % 2), 15 * s), sides=12, tsides=5)
    # the bare foot: bone, long toes
    P((x, 0.17, 0.30), (0.90, 0.34, 1.30), BONE2, hip, top=(0.9, 0.85), bev=0.1, seg=2)
    for k in range(4):
        z0 = 1.05 - abs(k - 1.5) * 0.08
        P((x + s * (-0.33 + 0.22 * k), 0.08, z0), (0.16, 0.46, 0.16), BONE, hip, rot=(90, 0, 0), top=(0.6, 0.6), bev=0.03)   # (+Y: forward)
    E((x, 0.22, -0.40), (0.62, 0.44, 0.50), BONE2, hip, seg=10, rings=6)                            # heel

# ---- the hips: a heavy rope belt, the robe's skirt in layered panels with a lit hem, tabards front and back ----
B((0, 5.45, 0.0), (3.9, 0.55, 2.5), RAG, RT, bev=0.12, seg=2)
C((0, 5.62, 0.0), 1.95, 0.32, RAG2, RT, sides=24)                                                  # the rope belt
R((0, 5.62, 0.0), 1.98, 0.1, RAG3, RT, rot=(90, 0, 0), sides=28, tsides=5)
for k in range(10):   # skirt panels round its hips
    a = k * math.pi / 5 + math.pi / 10
    px, pz = 1.85 * math.sin(a), 1.15 * math.cos(a)
    P((px, 4.45, pz), (1.25, 2.2, 0.14), RAG if k % 2 else RAG3, RT, rot=(-8 * math.cos(a), math.degrees(a), 8 * math.sin(a) * 0),
      top=(0.85, 1.0), bev=0.04)
    B((px * 1.04, 3.42, pz * 1.04), (1.3, 0.12, 0.16), LIT, RT, rot=(0, math.degrees(a), 0), glow="light", k=0.45)   # the lit hem
for z, sgn in ((1.32, 1), (-1.32, -1)):                                                             # tabards
    P((0, 3.9, z), (1.6, 3.2, 0.12), RAG2, RT, rot=(sgn * -4, 0, 0), top=(1.15, 1.0), bev=0.04)
    B((0, 2.33, z + sgn * 0.11), (1.62, 0.14, 0.12), LIT, RT, glow="light", k=0.45)
    for k in range(3):
        W((-0.5 + 0.5 * k, 2.15, z + sgn * 0.11), (0.42, 0.04, 0.34), RAG2, RT, rot=(90, 0, 0), taper=0.15)   # torn

# ---- the torso: the robe, an iron rib cage strapped over it, a seam of light down its front, a mantle of spikes,
#      the collar of chain; on its back, scars stapled shut that split into the wound ----
T_ = "torso"
P((0, 7.30, 0.0), (3.9, 4.1, 2.35), RAG, T_, top=(1.18, 1.05), bev=0.18, seg=2)                    # the robe's body
# a scapular falling down its front in folds, under the cage
for k in range(5):
    P((-1.2 + 0.6 * k, 7.2, 1.18 + 0.06 * (k % 2)), (0.62, 3.9, 0.12), RAG2 if k % 2 else RAG3, T_, rot=(0, (k - 2) * 7, 0),
      top=(0.85, 1.0), bev=0.05)
for i in range(5):   # the cage: bone ribs strapped over its robe, curving round its sides
    y = 6.3 + i * 0.58
    w = 1.85 - 0.12 * abs(i - 2.5)
    for s in (1, -1):
        P((s * 0.8, y, 1.34), (1.45 * w / 1.85, 0.26, 0.2), BONE, T_, rot=(0, s * 10, s * -14), top=(0.75, 0.9), bev=0.07, seg=2)
        P((s * 1.72, y - 0.18, 0.75), (0.24, 0.24, 0.95), BONE2, T_, rot=(0, s * 40, s * -14), top=(0.8, 0.8), bev=0.06)
B((0, 7.5, 1.42), (0.38, 3.9, 0.14), BONE2, T_, bev=0.06, seg=2)                                     # sternum
B((0, 7.5, 1.5), (0.12, 3.6, 0.06), LIT, T_, glow="light", k=0.8)                                    # the seam of light
for s in (1, -1):
    B((s * 1.9, 7.4, 0.0), (0.18, 3.2, 2.2), IRON, T_, bev=0.05)                                    # side straps
    for y in (6.6, 8.2):
        B((s * 1.9, y, 0.0), (0.26, 0.26, 2.3), IRON2, T_, bev=0.04)
        rivet = E((s * 2.02, y, 0.0), (0.18, 0.18, 0.18), IRON3, T_, seg=8, rings=4)
# the mantle: iron plates over its shoulders with spikes
for s in (1, -1):
    E((s * 1.75, 9.05, 0.0), (2.4, 0.9, 2.5), IRON, T_, seg=16, rings=8)
    for k in range(3):
        K((s * (1.15 + 0.5 * k), 9.35, -0.2 + 0.2 * k), 0.16, 0.9 - 0.15 * k, IRON3, T_, rot=(-15, 0, s * -(20 + 12 * k)), sides=6)
# the collar of chain: eight great links round its neck
for k in range(8):
    a = k * math.pi / 4
    R((1.55 * math.sin(a), 9.35, 0.95 * math.cos(a)), 0.38, 0.12, IRON2, T_, rot=(0, math.degrees(a) + 90 * (k % 2), 90 * (k % 2)), sides=14, tsides=5)
# the back: the robe torn open on a lattice of scars, iron staples across them
E((0, 7.6, -1.22), (2.5, 3.0, 0.3), "bone*0.6", T_, seg=18, rings=10)                               # its bared back
for k in range(4):   # scars (always) and the wound (only while it scourges)
    y = 6.75 + 0.55 * k
    B((0.1 * (k % 2), y, -1.35), (1.7, 0.08, 0.06), "bone*0.45", T_, rot=(0, 0, 9 - 6 * k), show="!open")
    B((0.1 * (k % 2), y, -1.38), (1.9, 0.18, 0.08), "glow*0.4", T_, rot=(0, 0, 9 - 6 * k), glow="core", k=1.0, show="open")
    for j in range(3):
        B((-0.55 + 0.55 * j, y, -1.37), (0.05, 0.3, 0.05), IRON3, T_, rot=(0, 0, 9 - 6 * k))         # staples
E((0, 7.6, -1.33), (1.7, 2.2, 0.12), "glow*0.25", T_, glow="core", k=0.6, show="open", seg=14, rings=8)   # the wound's heat
for s in (1, -1):   # the robe's torn edges round it
    P((s * 1.45, 7.6, -1.25), (0.6, 2.9, 0.12), RAG2, T_, rot=(0, s * -10, 0), top=(0.7, 1.0), bev=0.04)
P((0, 9.15, -1.25), (3.0, 0.7, 0.14), RAG2, T_, top=(0.8, 1.0), bev=0.04)                            # the hood's fall

# ---- the head: a deep hood with a lit rim, a peak falling back, a cracked bone mask with two burning eyes ----
H = "head"
E((0, 10.35, -0.08), (2.05, 2.0, 1.9), RAG, H, seg=18, rings=10)                                    # hood
K((0, 10.9, -0.45), 0.5, 0.45, RAG, H, rot=(-40, 0, 0), sides=10)                                    # its peak
E((0, 10.25, 0.55), (1.4, 1.55, 0.6), "rag*0.25", H, seg=14, rings=8)                               # the dark inside it
R((0, 10.28, 0.80), 0.74, 0.07, LIT, H, rot=(0, 0, 0), sides=26, tsides=5, glow="light", k=0.5)     # its lit rim
R((0, 10.28, 0.80), 0.86, 0.09, RAG2, H, sides=26, tsides=5)                                        # the hood's lip
P((0, 10.2, 0.92), (1.25, 1.45, 0.14), BONE, H, top=(0.72, 1.0), bev=0.07, seg=2)                   # the mask
for s in (1, -1):   # its eyes: two burning slits, slanted down to the middle
    B((s * 0.27, 10.5, 0.995), (0.32, 0.1, 0.05), "glow*0.2", H, rot=(0, 0, s * 12), glow="eye", k=1.4)
B((0.18, 10.05, 0.985), (0.03, 0.55, 0.03), "bone*0.35", H, rot=(0, 0, -18))                         # a crack
B((0, 9.75, 0.98), (0.6, 0.04, 0.03), "bone*0.35", H)                                                # the mouth's seam

# ---- the arms: a bell sleeve, an iron vambrace bound in bone-white cloth, an iron gauntlet with bone claws,
#      and the censer on its chain: a caged brass lantern with a fire inside ----
for s, arm in ((1, "armL"), (-1, "armR")):
    x = s * 2.8
    tell = "tellL" if s > 0 else "tellR"
    P((x, 7.15, 0.0), (1.35, 3.7, 1.35), RAG, arm, top=(0.65, 0.65), bev=0.1, seg=2)               # bell sleeve
    for k in range(3):   # its folds
        a = math.radians(-60 + 60 * k)
        P((x + s * 0.62 * math.cos(a) * 0 + 0.66 * math.sin(a) * s, 6.9, 0.66 * math.cos(a)), (0.2, 3.0, 0.14), RAG3, arm,
          rot=(0, math.degrees(a) * s, 0), top=(0.6, 1.0), bev=0.04)
    R((x, 5.38, 0.0), 0.96, 0.1, LIT, arm, rot=(90, 0, 0), sides=22, tsides=5, glow="light", k=0.35)   # its lit cuff
    P((x, 4.55, 0.0), (0.95, 1.6, 0.95), IRON, arm, top=(1.15, 1.15), bev=0.07, seg=2)             # vambrace
    for y in (4.2, 4.6, 5.0):
        C((x, y, 0.0), 0.56, 0.12, "bone*0.85", arm, rot=(0, 0, s * 10), sides=14)                 # bindings
    B((x, 3.3, 0.0), (1.2, 1.1, 1.15), IRON2, arm, bev=0.18, seg=2)                                # the fist
    for k in range(4):
        z = -0.42 + 0.28 * k
        E((x - s * 0.45, 2.92, z), (0.3, 0.38, 0.24), IRON3, arm, seg=8, rings=5)                   # knuckles
        K((x - s * 0.55, 2.8, z), 0.08, 0.3, BONE, arm, rot=(180, 0, s * -20), sides=6)            # claws
    # the chain and the censer
    for i in range(2):
        R((x, 2.55 - i * 0.48, 0.0), 0.26, 0.08, IRON2, arm, rot=(0, 90 * (i % 2), 0), sides=12, tsides=5)
    K((x, 1.92, 0.0), 0.62, 0.42, BRASS, arm, rot=(180, 0, 0), sides=12)                            # its cap
    E((x, 2.05, 0.0), (0.22, 0.22, 0.22), BRASS, arm, seg=10, rings=5)                              # finial
    E((x, 1.25, 0.0), (1.0, 1.0, 1.0), "glow*0.4", arm, glow=tell, k=1.0, seg=14, rings=8)          # the fire
    for k in range(4):                                                                              # the cage
        R((x, 1.25, 0.0), 0.66, 0.06, BRASS, arm, rot=(0, 45 * k, 0), sides=20, tsides=5)
    R((x, 1.25, 0.0), 0.68, 0.09, BRASS2, arm, rot=(90, 0, 0), sides=22, tsides=5)                 # its belt
    C((x, 0.62, 0.0), 0.36, 0.18, BRASS2, arm, sides=14)                                            # its foot
    K((x, 0.53, 0.0), 0.22, 0.3, BRASS, arm, rot=(180, 0, 0), sides=10)

report("penitent")
save_enemy("penitent")
