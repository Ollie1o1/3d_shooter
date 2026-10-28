# THE CANTOR in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/cantor_build.py).
# The Ascent's mini-boss, on the SERAPH's skeleton (drawn x1.9): a choir-construct in bell bronze and ivory that hangs
# under the rose window. A bell-shaped robe with lit runes, a lens in its chest ("core": blazes through the beam), a
# ring for a head with a bell hung in it, and wings of organ pipes whose mouths burn as they flare ("edge").
#   head (0, 1.42, 0): its ring (head hitbox: x, y within +-0.38 of it, z within +-0.17)
#   wingL/R (+-0.2, 1.05, -0.05)  tipL/R (+-0.2, 0.70, -0.05): built flat, turned to rest like the SERAPH's
import math
import os

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("cantor", joints=os.path.join(REPO, "assets", "enemies", BASE_FILE["cantor"] + ".glb")); clear_parts()

IVORY, IVORY2, BELL, BELL2, DARK = "cantor*1.0", "cantor*1.15", "bell", "bell*1.4", "cantor*0.5"
GOLD = "gold"
LIGHT = "glow*0.3"
RT = "seraph"

# ---- the body: a bell-shaped robe, ribbed, runes lit down its front; the lens; a collar ----
C((0, 0.55, 0.0), 0.42, 0.7, BELL, RT, sides=20, r2=0.2)                                       # the robe (a bell)
R((0, 0.21, 0.0), 0.43, 0.035, BELL2, RT, rot=(90, 0, 0), sides=24, tsides=5)                   # its lip
for k in range(3):
    R((0, 0.38 + 0.15 * k, 0.0), 0.37 - 0.06 * k, 0.018, GOLD, RT, rot=(90, 0, 0), sides=22, tsides=4)   # bands
for k in range(5):   # runes down its front
    B((0, 0.32 + 0.1 * k, 0.33 - 0.04 * k), (0.06, 0.04, 0.02), LIGHT, RT, rot=(-12, 0, 0), glow="glow", k=1.0)
E((0, 0.96, 0.0), (0.42, 0.44, 0.32), IVORY, RT, seg=14, rings=8)                               # chest
P((0, 1.0, 0.1), (0.34, 0.34, 0.1), IVORY2, RT, top=(1.15, 1.0), bev=0.02)                      # breastplate
R((0, 0.96, 0.165), 0.11, 0.02, GOLD, RT, sides=20, tsides=4)
E((0, 0.96, 0.17), (0.16, 0.16, 0.06), LIGHT, RT, glow="core", k=1.0, seg=12, rings=6)          # the lens
C((0, 1.25, 0.0), 0.17, 0.06, BELL, RT, sides=16)                                               # collar
C((0, 1.31, 0.0), 0.05, 0.08, IVORY2, RT, sides=8)                                              # neck
# the clapper hanging under the robe
C((0, 0.1, 0.0), 0.02, 0.22, DARK, RT, sides=6)
E((0, -0.04, 0.0), (0.11, 0.11, 0.11), BELL2, RT, seg=10, rings=6)

# ---- the head: a ring of bronze, a small bell hung in it ----
H = "head"
R((0, 1.42, 0.0), 0.28, 0.045, BELL, H, sides=28, tsides=6)
R((0, 1.42, 0.0), 0.21, 0.018, GOLD, H, sides=24, tsides=4)
for k in range(8):
    a = k * math.pi / 4
    B((0.3 * math.sin(a), 1.42 + 0.3 * math.cos(a), 0.0), (0.08, 0.06, 0.07), IVORY2, H, rot=(0, 0, -math.degrees(a)), bev=0.012)
    B((0.3 * math.sin(a), 1.42 + 0.3 * math.cos(a), 0.045), (0.03, 0.025, 0.02), LIGHT, H, rot=(0, 0, -math.degrees(a)), glow="glow", k=0.9)
C((0, 1.62, 0.0), 0.012, 0.08, GOLD, H, sides=6)                                                # the bell's hanger
C((0, 1.44, 0.0), 0.11, 0.16, BELL2, H, sides=16, r2=0.06)                                      # the bell
E((0, 1.34, 0.0), (0.05, 0.05, 0.05), LIGHT, H, glow="glow", k=2.0, seg=8, rings=5)             # its lit clapper


# ---- wings: a gold bone with organ pipes hanging off it, their mouths burning; built flat, turned to rest ----
def wing(s, joint, at, span, lift, n):
    before = parts_now()
    ax, ay, az = at
    C((ax + s * span * 0.45, ay, az), 0.03, span * 0.9, GOLD, joint, rot=(0, 0, 90), sides=8)      # its bone
    for k in range(n):
        f = (k + 0.5) / n
        L = (0.75 - 0.35 * abs(f - 0.5)) * (span / 1.2) ** 0.5                                     # pipe length
        x = ax + s * span * f
        C((x, ay - L * 0.5, az - 0.02), 0.045, L, BELL if k % 2 else BELL2, joint, sides=10)        # a pipe
        B((x, ay - L * 0.72, az + 0.03), (0.05, 0.05, 0.02), LIGHT, joint, glow="edge", k=1.0)     # its mouth
        K((x, ay - L, az - 0.02), 0.045, 0.08, GOLD, joint, rot=(180, 0, 0), sides=8)              # its foot
    turn_about(new_since(before), at, rot_game('Z', math.degrees(s * lift)) @ rot_game('Y', math.degrees(-s * 0.25)))


for s, up, low in ((1, "wingL", "tipL"), (-1, "wingR", "tipR")):
    wing(s, up, (s * 0.2, 1.05, -0.05), 1.2, 0.5, 7)
    wing(s, low, (s * 0.2, 0.70, -0.05), 0.9, -0.15, 5)

report("cantor")
save_enemy("cantor")
