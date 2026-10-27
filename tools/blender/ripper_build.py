# The RIPPER in detail (python3 tools/blender/live.py tools/blender/ripper_build.py). A low machine hound with a
# segmented back and scythe blades that brighten as it crouches to lunge.
#   body (0, 0.62, 0)  head (0, 0.72, 0.6)  legFL/FR (+-0.27, 0.5, 0.42)  legBL/BR (+-0.27, 0.5, -0.42)
#   bladeL/R (+-0.36, 0.67, 0.42); the head hitbox: x within +-0.28, y -0.30..0.24, z -0.08..0.52 of the head
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
open_enemy("ripper"); clear_parts()
ARM, DARK, METAL = "ripper", "gunmetal*1.3", "gunmetal"

Bo = "body"
B((0, 0.62, 0.0), (0.56, 0.38, 1.05), ARM, Bo, bev=0.05, seg=2)                         # hull
B((0, 0.46, 0.0), (0.42, 0.12, 0.90), DARK, Bo, bev=0.02)                               # belly
for i in range(3):
    B((0, 0.83, -0.30 + 0.30 * i), (0.50, 0.10, 0.28), "ripper*1.1", Bo, bev=0.03, seg=2)    # back plates
    B((0, 0.93, -0.38 + 0.28 * i), (0.06, 0.18, 0.10), "ripper*0.45", Bo, glow="glow", k=0.4)   # spine fins
for s in (1, -1):
    B((s * 0.30, 0.72, 0.35), (0.08, 0.24, 0.34), "ripper*0.85", Bo, bev=0.02)          # shoulder plates
    B((s * 0.30, 0.70, -0.35), (0.08, 0.22, 0.30), "ripper*0.85", Bo, bev=0.02)         # hip plates
W((0, 0.68, -0.68), (0.08, 0.08, 0.34), METAL, Bo, rot=(15, 180, 0), taper=0.2)          # tail spike
B((0, 0.72, 0.50), (0.26, 0.22, 0.20), DARK, Bo, bev=0.02)                              # neck

H = "head"
B((0, 0.74, 0.82), (0.38, 0.28, 0.42), "ripper*0.9", H, bev=0.05, seg=2)                # skull
W((0, 0.71, 1.04), (0.30, 0.20, 0.16), "ripper*0.9", H, taper=0.5)                      # snout
B((0, 0.58, 0.88), (0.32, 0.08, 0.36), DARK, H, bev=0.015)                              # jaw
B((0, 0.63, 1.00), (0.26, 0.03, 0.08), "bone", H)                                       # teeth
for s in (1, -1):
    B((s * 0.11, 0.78, 1.03), (0.07, 0.05, 0.02), "glow*1", H, glow="glow", k=2.5)      # eyes
    W((s * 0.16, 0.86, 0.74), (0.04, 0.14, 0.16), ARM, H, rot=(-30, 0, 0), taper=0.2)   # ear fins

for leg, (x, z) in (("legFL", (0.27, 0.42)), ("legFR", (-0.27, 0.42)), ("legBL", (0.27, -0.42)), ("legBR", (-0.27, -0.42))):
    B((x, 0.36, z), (0.12, 0.28, 0.14), "ripper*0.85", leg, bev=0.02)                   # upper leg
    C((x, 0.22, z), 0.055, 0.15, METAL, leg, rot=(0, 0, 90), sides=10)                  # knee
    B((x, 0.11, z), (0.09, 0.22, 0.10), DARK, leg, bev=0.01)                            # lower leg
    W((x, 0.02, z + 0.06), (0.12, 0.04, 0.14), METAL, leg, taper=0.4)                   # claw foot

for s, bl in ((1, "bladeL"), (-1, "bladeR")):
    C((s * 0.36, 0.67, 0.42), 0.06, 0.08, METAL, bl, rot=(0, 0, 90), sides=10)          # blade mount
    W((s * 0.36, 0.67, 0.80), (0.04, 0.12, 0.70), "glow*0.4", bl, glow="blade", k=1.0, taper=0.15)   # scythe blade
    B((s * 0.36, 0.73, 0.72), (0.05, 0.03, 0.50), METAL, bl)                            # its spine
save_enemy("ripper")
