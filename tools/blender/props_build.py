# Level props made in Blender (blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/props_build.py
# [ONLY=name]): each saved to assets/props/<name>.glb (PropFile.h). Built at the size of the box they dress (the
# level stretches them to fit it exactly) or, for skylines, at their own size. Game space: +Y up, metres; the floor
# at y 0. A part's surface comes from its material's name (wood, metal, marble, adobe... below: TEX); a glowing part
# (self-lit, baked with the neon) glows on "const" with k its colour.
import math

exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())
if "const" not in _og.CHANNELS:
    _og.CHANNELS.append("const")
_og.PALETTE.update({
    "wood": (0.47, 0.31, 0.21), "plank": (0.36, 0.23, 0.15), "iron": (0.26, 0.24, 0.25), "rust": (0.48, 0.25, 0.15),
    "marble": (0.56, 0.48, 0.43), "adobe": (0.66, 0.40, 0.30), "adobedark": (0.50, 0.28, 0.22),
    "cloth1": (0.62, 0.12, 0.16), "cloth2": (0.86, 0.74, 0.52), "cloth3": (0.12, 0.40, 0.44),
    "fruit": (0.95, 0.45, 0.08), "fruit2": (0.55, 0.65, 0.12), "clay": (0.62, 0.34, 0.22),
    "girder": (0.32, 0.29, 0.28), "slate": (0.22, 0.25, 0.30), "basalt": (0.17, 0.15, 0.16), "lit": (1.0, 0.7, 0.3),
})
TEX = {"wood": "wood", "plank": "wood", "iron": "metal", "rust": "metal", "girder": "plate", "marble": "marble",
       "adobe": "adobe", "adobedark": "adobe", "clay": "adobe", "slate": "panel", "basalt": "basalt", "gold": "brass"}
ONLY = globals().get("ONLY")
SC = bpy.context.scene


def begin():
    for ob in list(SC.objects):
        bpy.data.objects.remove(ob, do_unlink=True)


def glow(ob, rgb):
    set_glow(ob, "const", k=rgb)
    return ob


def save(name):
    for ob in SC.objects:   # each part's surface from its material's name
        if ob.type == "MESH" and ob.data.materials and "glow" not in ob:
            base = ob.data.materials[0].name.split("*")[0]
            if base in TEX:
                ob["tex"] = TEX[base]
    os.makedirs(os.path.join(REPO, "assets", "props"), exist_ok=True)
    with contextlib.redirect_stdout(io.StringIO()):
        bpy.ops.export_scene.gltf(filepath=os.path.join(REPO, "assets", "props", name + ".glb"), export_format="GLB",
                                  export_extras=True, export_yup=True, export_apply=True, use_selection=False,
                                  use_active_scene=True, export_texcoords=False)
    shrink(os.path.join(REPO, "assets", "props", name + ".glb"))
    report(name)


def want(name):
    return not ONLY or ONLY == name


# ---- SUNSET YARD ----------------------------------------------------------------------------------
if want("crates"):   # 2 x 1 x 2: four crates in a square, iron corners, a stencilled band; flat on top (you stand there)
    begin()
    for ix in (-0.5, 0.5):
        for iz in (-0.5, 0.5):
            c = (ix, 0.5, iz)
            B(c, (0.96, 0.98, 0.96), "wood", None, bev=0.03, seg=2)
            for k in (-1, 1):   # planks: dark seams across two faces
                B((ix, 0.5 + k * 0.24, iz + 0.485), (0.9, 0.025, 0.012), "plank*0.7", None)
                B((ix + 0.485, 0.5 + k * 0.24, iz), (0.012, 0.025, 0.9), "plank*0.7", None)
                B((ix - 0.485, 0.5 + k * 0.24, iz), (0.012, 0.025, 0.9), "plank*0.7", None)
                B((ix, 0.5 + k * 0.24, iz - 0.485), (0.9, 0.025, 0.012), "plank*0.7", None)
            for sx in (-1, 1):
                for sz in (-1, 1):
                    B((ix + sx * 0.46, 0.5, iz + sz * 0.46), (0.07, 0.99, 0.07), "iron", None, bev=0.01)   # corner irons
            B((ix, 0.97, iz), (0.9, 0.02, 0.9), "plank", None)                                             # its lid
    B((0, 0.5, 1.0), (0.5, 0.18, 0.012), "cloth2*0.9", None)                                               # a stencil
    save("crates")

if want("stall_counter"):   # 4 x 1.2 x 2: a plank counter, a striped drape in front, baskets of fruit and jars on top
    begin()
    B((0, 0.55, 0), (4.0, 1.1, 2.0), "plank", None, bev=0.03)
    B((0, 1.15, 0), (4.0, 0.1, 2.0), "wood", None, bev=0.02)                                              # the top
    for k in range(9):
        B((-1.8 + 0.45 * k, 0.55, 1.0), (0.04, 1.0, 0.03), "plank*0.65", None)                            # boards
    for k in range(8):   # the drape
        P((-1.75 + 0.5 * k, 0.75, 1.02), (0.5, 0.7, 0.025), "cloth1" if k % 2 == 0 else "cloth2", None, top=(1.0, 1.0))
        W((-1.75 + 0.5 * k, 0.36, 1.02), (0.5, 0.025, 0.12), "cloth1" if k % 2 == 0 else "cloth2", None, rot=(90, 0, 0), taper=0.1)
    for bx, bz, f in ((-1.3, 0.3, "fruit"), (-0.3, -0.2, "fruit2"), (0.8, 0.35, "fruit")):
        C((bx, 1.24, bz), 0.32, 0.12, "plank*0.8", None, sides=12)                                         # basket
        for i in range(6):
            a = i * math.pi / 3
            E((bx + 0.16 * math.cos(a), 1.3, bz + 0.16 * math.sin(a)), (0.13, 0.11, 0.13), f, None, seg=8, rings=5)
        E((bx, 1.31, bz), (0.13, 0.12, 0.13), f, None, seg=8, rings=5)
    for jx in (1.5, 1.75):
        C((jx, 1.3, -0.4), 0.09, 0.22, "clay", None, sides=10, r2=0.06)                                    # jars
    save("stall_counter")

if want("stall_canopy"):   # 4.6 x 0.2 x 2.6: striped cloth, a scalloped edge front and back
    begin()
    for k in range(9):
        B((-2.3 + 0.256 + 0.511 * k, 0.12, 0), (0.511, 0.05, 2.6), "cloth3" if k % 2 == 0 else "cloth2", None)
    for z in (1.3, -1.3):
        for k in range(9):
            W((-2.3 + 0.256 + 0.511 * k, 0.04, z), (0.48, 0.02, 0.16), "cloth3" if k % 2 == 0 else "cloth2", None,
              rot=(90 if z > 0 else -90, 0, 0), taper=0.15)
    B((0, 0.17, 0), (4.6, 0.04, 0.08), "wood", None)                                                      # the ridge pole
    save("stall_canopy")

if want("column_broken"):   # 1.5 x 4 x 1.5: a plinth, a fluted shaft, its top broken off jagged
    begin()
    B((0, 0.15, 0), (1.5, 0.3, 1.5), "marble*0.9", None, bev=0.04)
    C((0, 0.36, 0), 0.68, 0.12, "marble", None, sides=20)
    C((0, 2.0, 0), 0.6, 3.2, "marble", None, sides=20, r2=0.56)
    for k in range(16):   # flutes
        a = k * math.pi / 8
        B((0.585 * math.cos(a), 2.0, 0.585 * math.sin(a)), (0.05, 3.1, 0.05), "marble*0.75", None, rot=(0, -math.degrees(a), 0))
    for k in range(6):   # the broken top
        a = k * math.pi / 3 + 0.3
        h = 0.18 + 0.2 * ((k * 7) % 3)
        P((0.35 * math.cos(a), 3.6 + h * 0.5, 0.35 * math.sin(a)), (0.45, h, 0.4), "marble*0.95", None,
          rot=(0, -math.degrees(a), 8 * ((k % 2) * 2 - 1)), top=(0.4, 0.5), bev=0.02)
    save("column_broken")

if want("skyline_town"):   # ~24 x 14 x 16 m: an adobe town on a rise, domes, a minaret, lit windows
    begin()
    B((0, 1.0, 0), (24, 2.0, 16), "adobedark*0.9", None, bev=0.4)                                          # the rise
    houses = [(-9, 0, 5, 5, 4), (-4, -3, 4, 6, 6), (1, 2, 6, 5, 5), (6, -2, 5, 4, 7), (9, 4, 4, 4, 4),
              (-6, 5, 4, 4, 3.5), (3, -5, 5, 4, 4.5), (-1, 6, 3, 3, 3)]
    for i, (x, z, w, d, h) in enumerate(houses):
        B((x, 2 + h * 0.5, z), (w, h, d), "adobe" if i % 2 else "adobedark", None, bev=0.15)
        B((x, 2 + h + 0.15, z), (w + 0.2, 0.3, d + 0.2), "adobedark*0.8", None)                            # parapet
        for k in range(max(1, int(w // 1.6))):   # lit windows on its front
            glow(B((x - w * 0.5 + 0.8 + 1.6 * k, 2 + h * 0.62, z + d * 0.5 + 0.02), (0.45, 0.6, 0.05), "lit", None),
                 (1.6, 0.9, 0.4) if (i + k) % 3 else (1.1, 0.5, 0.25))
    for x, z, r in ((-4, -3, 2.0), (6, -2, 1.7), (1, 2, 1.5)):
        h = [hh for (hx, hz, _, _, hh) in houses if hx == x and hz == z][0]
        E((x, 2 + h + 0.3, z), (2 * r, 2 * r, 2 * r), "clay", None, seg=16, rings=8)                        # domes
    C((11, 9, -5), 0.9, 14, "adobe", None, sides=10)                                                       # the minaret
    C((11, 14.5, -5), 1.3, 0.4, "adobedark", None, sides=10)
    glow(C((11, 15.2, -5), 0.75, 1.0, "lit", None, sides=10), (1.4, 0.8, 0.35))
    K((11, 15.7, -5), 0.9, 2.2, "clay", None, sides=10)
    save("skyline_town")

# ---- THE FOUNDRY ----------------------------------------------------------------------------------
if want("foundry_pillar"):   # 2 x 14 x 2: a riveted steel girder column on a bolted foot, braced to its plates
    begin()
    B((0, 0.3, 0), (2.0, 0.6, 2.0), "girder*0.8", None, bev=0.05)                                          # foot
    for sx in (-1, 1):
        for sz in (-1, 1):
            C((sx * 0.8, 0.65, sz * 0.8), 0.07, 0.12, "iron", None, sides=6)                               # bolts
    B((0, 7.3, 0), (0.24, 13.4, 1.6), "girder", None)                                                       # the web
    for sx in (-1, 1):
        B((sx * 0.85, 7.3, 0), (0.3, 13.4, 1.9), "girder*1.1", None, bev=0.02)                              # flanges
        for k in range(26):
            E((sx * 1.0, 1.0 + 0.5 * k, 0.75), (0.07, 0.07, 0.04), "iron", None, seg=6, rings=3)            # rivets
            E((sx * 1.0, 1.0 + 0.5 * k, -0.75), (0.07, 0.07, 0.04), "iron", None, seg=6, rings=3)
    for k in range(6):   # cross braces between the flanges
        y = 1.6 + 2.2 * k
        B((0, y, 0.85), (1.6, 0.18, 0.06), "rust", None, rot=(0, 0, 32 if k % 2 else -32))
        B((0, y, -0.85), (1.6, 0.18, 0.06), "rust", None, rot=(0, 0, -32 if k % 2 else 32))
    B((0, 13.8, 0), (2.0, 0.4, 2.0), "girder*0.8", None, bev=0.04)                                          # its cap at the roof
    save("foundry_pillar")

if want("ingots"):   # 6 x 1.2 x 2: ingots stacked in crossing layers
    begin()
    for layer in range(3):
        y = 0.2 + 0.4 * layer
        if layer % 2 == 0:
            for k in range(10):
                P((-2.7 + 0.6 * k, y, 0), (0.55, 0.38, 1.95), "rust*1.1" if k % 2 else "rust", None, top=(0.8, 0.9), bev=0.02)
        else:
            for k in range(4):
                P((0, y, -0.73 + 0.487 * k), (5.95, 0.38, 0.45), "rust*0.95" if k % 2 else "rust*1.15", None, top=(0.95, 0.8), bev=0.02)
    save("ingots")

# ---- THE CORE -------------------------------------------------------------------------------------
if want("core_pillar"):   # 2 x 7 x 2: panelled slate, cable bundles in its corners, lit conduits up its faces
    begin()
    B((0, 0.25, 0), (2.0, 0.5, 2.0), "slate*0.8", None, bev=0.04)
    B((0, 3.6, 0), (1.7, 6.2, 1.7), "slate", None, bev=0.06)
    for f in range(4):
        a = f * math.pi / 2
        nx, nz = math.cos(a), math.sin(a)
        for k in range(3):   # panels
            B((0.86 * nx, 1.3 + 2.0 * k, 0.86 * nz), (0.06 if nx else 1.3, 1.8, 1.3 if nx else 0.06), "slate*1.25", None, bev=0.02)
        glow(B((0.9 * nx, 3.6, 0.9 * nz), (0.04 if nx else 0.12, 5.6, 0.12 if nx else 0.04), "lit", None), (0.2, 0.9, 1.0))
    for sx in (-1, 1):
        for sz in (-1, 1):
            for j in range(3):
                C((sx * (0.82 + 0.04 * j), 3.6, sz * (0.82 - 0.04 * j)), 0.05, 6.2, "iron", None, sides=6)   # cables
    B((0, 6.85, 0), (2.0, 0.3, 2.0), "slate*0.8", None, bev=0.03)                                          # cap (the bridges sit on it)
    glow(B((0, 2.07, 0), (1.78, 0.15, 1.78), "lit", None), (0.2, 0.9, 1.0))                               # its lit band
    save("core_pillar")

# ---- THE SANCTUM ----------------------------------------------------------------------------------
if want("sanctum_pillar"):   # 3 x 18 x 3: carved basalt, a stepped base, a fluted shaft with gold bands, a crowned capital
    begin()
    for k in range(3):
        B((0, 0.2 + 0.4 * k, 0), (3.0 - 0.25 * k, 0.4, 3.0 - 0.25 * k), "basalt*0.9", None, bev=0.05)       # stepped base
    C((0, 9.2, 0), 1.2, 15.6, "basalt", None, sides=16, r2=1.1)
    for k in range(16):   # flutes
        a = k * math.pi / 8
        B((1.17 * math.cos(a), 9.2, 1.17 * math.sin(a)), (0.1, 15.2, 0.1), "basalt*0.7", None, rot=(0, -math.degrees(a), 0))
    for y in (3.0, 15.0):
        C((0, y, 0), 1.28, 0.3, "gold", None, sides=16)                                                    # gold bands
    for y in (6.1, 12.1):
        glow(C((0, y, 0), 1.24, 0.2, "lit", None, sides=16), (0.7, 0.5, 0.14))                            # lit bands
    for k in range(3):
        B((0, 17.1 + 0.3 * k, 0), (2.4 + 0.3 * k, 0.3, 2.4 + 0.3 * k), "basalt*0.95", None, bev=0.04)      # capital
    for k in range(8):   # its crown of points
        a = k * math.pi / 4
        K((1.3 * math.cos(a), 17.85, 1.3 * math.sin(a)), 0.12, 0.15, "gold", None, sides=4)
    save("sanctum_pillar")

# ---- SMALL MODELS (drawn live, PropDraw.h): their own size. "glow*s" parts take the tint's colour (x s);
#      parts glowing on "glow" take the tint (x k) as it pulses; on "const", k ----
def tglow(ob, k=1.0):
    set_glow(ob, "glow", k=k)
    return ob


if want("pickup_orb"):   # a health orb: a green heart in a cage of three steel rings, a cross on each face
    begin()
    tglow(E((0, 0, 0), (0.26, 0.26, 0.26), "glow*0.6", None, seg=14, rings=8), 1.0)
    for rot in ((0, 0, 0), (90, 0, 0), (0, 90, 0)):
        R((0, 0, 0), 0.17, 0.014, "girder*1.4", None, rot=rot, sides=24, tsides=5)
    for z in (0.135, -0.135):
        tglow(B((0, 0, z), (0.1, 0.03, 0.012), "glow*0.9", None), 1.6)
        tglow(B((0, 0, z), (0.03, 0.1, 0.012), "glow*0.9", None), 1.6)
    save("pickup_orb")

if want("pickup_potion"):   # a flask: a round glass belly lit from inside, a neck, a steel collar, a cork
    begin()
    tglow(E((0, 0, 0), (0.36, 0.36, 0.36), "glow*0.5", None, seg=14, rings=8), 1.0)
    E((0.07, 0.07, 0.12), (0.08, 0.1, 0.05), "cloth2*1.1", None, seg=8, rings=4)                         # a highlight
    C((0, 0.22, 0), 0.06, 0.14, "glow*0.35", None, sides=10)                                             # neck
    C((0, 0.29, 0), 0.075, 0.03, "girder*1.4", None, sides=12)                                           # collar
    C((0, 0.34, 0), 0.05, 0.08, "plank", None, sides=10, r2=0.055)                                       # cork
    R((0, 0.0, 0), 0.185, 0.012, "girder*1.4", None, rot=(90, 0, 0), sides=24, tsides=4)                 # a band round it
    save("pickup_potion")

if want("pickup_xp"):   # a crystal of experience: a long violet diamond, three shards turning with it
    begin()
    tglow(E((0, 0, 0), (0.22, 0.44, 0.22), "glow*0.55", None, seg=4, rings=2), 1.0)
    for k in range(3):
        a = k * 2 * math.pi / 3
        tglow(E((0.24 * math.cos(a), 0.06 * (k - 1), 0.24 * math.sin(a)), (0.07, 0.14, 0.07), "glow*0.55", None, seg=4, rings=2), 0.8)
    R((0, 0, 0), 0.15, 0.01, "gold", None, rot=(90, 0, 0), sides=20, tsides=4)
    save("pickup_xp")

if want("shot_orb"):   # an enemy's shot: a burning core, six spikes of its light, a ring round it (~0.5 m)
    begin()
    tglow(E((0, 0, 0), (0.28, 0.28, 0.28), "glow*0.9", None, seg=12, rings=6), 3.0)
    E((0, 0, 0), (0.36, 0.36, 0.36), "glow*0.25", None, seg=12, rings=6)                                 # its dark shell (the core shows through the spikes)
    for d in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)):
        tglow(K((d[0] * 0.15, d[1] * 0.15, d[2] * 0.15), 0.07, 0.16, "glow*0.6", None, rot=_aim(d), sides=5), 1.6)
    tglow(R((0, 0, 0), 0.26, 0.015, "glow*0.5", None, rot=(90, 0, 0), sides=20, tsides=4), 1.2)
    save("shot_orb")

if want("shot_shell"):   # a JUGGERNAUT's siege shell: a steel body, a lit band, a pointed nose, fins (+Z: its way)
    begin()
    C((0, 0, -0.05), 0.16, 0.5, "girder*1.2", None, rot=(90, 0, 0), sides=12)
    K((0, 0, 0.2), 0.16, 0.28, "girder*1.5", None, rot=(90, 0, 0), sides=12)
    tglow(C((0, 0, 0.08), 0.165, 0.06, "glow*0.8", None, rot=(90, 0, 0), sides=12), 2.0)
    for k in range(4):
        a = k * math.pi / 2 + math.pi / 4
        P((0.2 * math.cos(a), 0.2 * math.sin(a), -0.25), (0.02, 0.16, 0.18), "girder", None, rot=(90, 0, math.degrees(a) - 90), top=(1.0, 0.4))
    tglow(C((0, 0, -0.32), 0.1, 0.04, "glow*0.9", None, rot=(90, 0, 0), sides=10), 3.0)                  # its exhaust
    save("shot_shell")

if want("shot_grenade"):   # a lobbed bomb: a round iron case in bands, a lit seam, a fuse cap
    begin()
    E((0, 0, 0), (0.3, 0.3, 0.3), "iron*1.3", None, seg=14, rings=8)
    for rot in ((0, 0, 0), (0, 90, 0)):
        R((0, 0, 0), 0.152, 0.012, "girder*1.4", None, rot=rot, sides=24, tsides=4)
    tglow(R((0, 0, 0), 0.152, 0.008, "glow*0.8", None, rot=(90, 0, 0), sides=24, tsides=4), 2.0)
    C((0, 0.16, 0), 0.04, 0.05, "girder*1.4", None, sides=8)
    tglow(E((0, 0.2, 0), (0.05, 0.05, 0.05), "glow*0.9", None, seg=6, rings=3), 3.0)
    save("shot_grenade")

if want("hook"):   # the grapple's claw: a shaft, a collar, three prongs curving back (+Z: the way it flew)
    begin()
    C((0, 0, -0.12), 0.025, 0.24, "girder*1.4", None, rot=(90, 0, 0), sides=8)
    C((0, 0, 0.0), 0.045, 0.05, "girder", None, rot=(90, 0, 0), sides=10)
    tglow(R((0, 0, -0.03), 0.04, 0.008, "glow*0.6", None, sides=14, tsides=4), 1.5)
    K((0, 0, 0.03), 0.035, 0.08, "girder*1.6", None, rot=(90, 0, 0), sides=8)
    for k in range(3):
        a = k * 2 * math.pi / 3
        ca, sa = math.cos(a), math.sin(a)
        tube([(0.03 * ca, 0.03 * sa, 0.02), (0.1 * ca, 0.1 * sa, -0.01), (0.12 * ca, 0.12 * sa, -0.09)], [0.014, 0.011, 0.007], "girder*1.5", None,
             sides=6, tip=0.03)
    save("hook")
