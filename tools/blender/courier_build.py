# THE COURIER in detail (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/courier_build.py).
# The Canal's mini-boss: a courier hound in hi-vis orange and hazard stripes, on the RIPPER's skeleton (drawn x1.55).
# The room's key rides on its back in a lit canister ("glow"); satchels on its flanks; a beacon on a mast; short
# hooked blades ("blade") for the lunge it saves for when it's cornered.
#   body (0, 0.62, 0)  head (0, 0.72, 0.6)  legFL/FR (+-0.27, 0.5, 0.42)  legBL/BR (+-0.27, 0.5, -0.42)
#   bladeL/R (+-0.36, 0.67, 0.42); the head hitbox: x within +-0.28, y -0.30..0.24, z -0.08..0.52 of the head
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
import os
open_enemy("courier", joints=os.path.join(REPO, "assets", "enemies", BASE_FILE["courier"] + ".glb")); clear_parts()
ORANGE, ORANGE2, DARK, METAL, STRIPE = "courier", "courier*1.2", "gunmetal*1.3", "gunmetal", "hazard"

Bo = "body"
B((0, 0.63, 0.0), (0.60, 0.40, 1.10), ORANGE, Bo, bev=0.06, seg=2)                         # hull
B((0, 0.46, 0.0), (0.46, 0.12, 0.94), DARK, Bo, bev=0.02)                                  # belly
for z in (-0.38, -0.12, 0.14, 0.40):                                                       # hazard stripes over its back
    B((0, 0.835, z), (0.62, 0.02, 0.09), STRIPE, Bo, bev=0.008)
for s in (1, -1):
    B((s * 0.32, 0.62, 0.0), (0.06, 0.30, 0.92), ORANGE2, Bo, bev=0.02)                    # flank plates
    B((s * 0.37, 0.56, -0.10), (0.10, 0.22, 0.40), "rag", Bo, bev=0.03, seg=2)             # satchels
    B((s * 0.43, 0.60, -0.10), (0.02, 0.06, 0.30), "glow*0.3", Bo, glow="glow", k=0.8)     # reflective strip
    B((s * 0.31, 0.74, 0.36), (0.09, 0.26, 0.34), ORANGE2, Bo, bev=0.02)                   # shoulder plates
    B((s * 0.31, 0.72, -0.36), (0.09, 0.24, 0.30), ORANGE2, Bo, bev=0.02)                  # hip plates
# The key: a canister strapped across its back, lit from inside
C((0, 0.97, -0.05), 0.13, 0.46, METAL, Bo, rot=(90, 0, 0), sides=14)                        # canister
C((0, 0.97, -0.05), 0.105, 0.36, "glow*0.4", Bo, rot=(90, 0, 0), sides=14, glow="glow", k=2.2)   # the key's light
for z in (-0.28, 0.18):
    R((0, 0.97, z), 0.14, 0.02, "brass", Bo, sides=18, tsides=4)                            # its bands
for s in (1, -1):
    B((s * 0.16, 0.86, -0.05), (0.04, 0.14, 0.06), DARK, Bo)                                # straps
C((0, 1.22, -0.36), 0.015, 0.50, METAL, Bo, sides=6)                                        # beacon mast
E((0, 1.48, -0.36), (0.07, 0.07, 0.07), "glow*0.5", Bo, glow="glow", k=2.5, seg=8, rings=5)
W((0, 0.70, -0.70), (0.10, 0.08, 0.36), METAL, Bo, rot=(15, 180, 0), taper=0.2)            # tail spike
B((0, 0.72, 0.50), (0.28, 0.24, 0.20), DARK, Bo, bev=0.02)                                  # neck

H = "head"
B((0, 0.74, 0.82), (0.38, 0.28, 0.42), ORANGE, H, bev=0.05, seg=2)                         # skull
W((0, 0.71, 1.04), (0.30, 0.20, 0.16), ORANGE2, H, taper=0.5)                              # snout
B((0, 0.58, 0.88), (0.34, 0.08, 0.36), DARK, H, bev=0.015)                                 # jaw
B((0, 0.63, 1.00), (0.28, 0.03, 0.08), "bone", H)                                          # teeth
B((0, 0.85, 0.80), (0.30, 0.03, 0.30), STRIPE, H, bev=0.006)                               # stripe on its brow
for s in (1, -1):
    B((s * 0.11, 0.78, 1.03), (0.08, 0.05, 0.02), "glow*1", H, glow="glow", k=2.8)         # eyes
    W((s * 0.15, 0.86, 0.72), (0.04, 0.12, 0.16), ORANGE, H, rot=(-30, 0, 0), taper=0.2)   # ear fins

for leg, (x, z) in (("legFL", (0.27, 0.42)), ("legFR", (-0.27, 0.42)), ("legBL", (0.27, -0.42)), ("legBR", (-0.27, -0.42))):
    B((x, 0.36, z), (0.13, 0.28, 0.15), ORANGE2, leg, bev=0.02)                            # upper leg
    C((x, 0.22, z), 0.07, 0.16, METAL, leg, rot=(0, 0, 90), sides=12)                      # knee hub
    C((x, 0.22, z), 0.035, 0.18, STRIPE, leg, rot=(0, 0, 90), sides=8)
    B((x, 0.11, z), (0.10, 0.22, 0.11), DARK, leg, bev=0.01)                               # lower leg
    W((x, 0.02, z + 0.06), (0.13, 0.04, 0.15), METAL, leg, taper=0.4)                      # claw foot

for s, bl in ((1, "bladeL"), (-1, "bladeR")):
    C((s * 0.36, 0.67, 0.42), 0.06, 0.08, METAL, bl, rot=(0, 0, 90), sides=10)             # blade mount
    W((s * 0.36, 0.67, 0.68), (0.04, 0.12, 0.46), "glow*0.4", bl, glow="blade", k=1.0, taper=0.15)   # hooked blade
    B((s * 0.36, 0.73, 0.62), (0.05, 0.03, 0.34), METAL, bl)
report("courier")
save_enemy("courier")
