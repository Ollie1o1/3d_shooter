"""THE LIVING WARD's far distance, authored in Blender. Run from the repo root:
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/story_vista_build.py
Exports assets/props/lab_*.glb and hadal_*.glb, editable scenes in build/story-vista/.

These are drawn live (Gameplay_LabDressing.h gatherLabVista, through PropDraw.h), hundreds of metres
out, so they move and turn. To keep them cheap every model is JOINED by look before export: each
material + glow becomes one mesh, so a wing is a handful of instanced parts (one draw call each, and
every copy of the wing shares them) rather than the dozens of boxes it is built from.

  lab_wing        a wing of the lab adrift: tiled block, chamfered, rows of windows (steady and
                  flickering sets), its torn deck and the rebar and cable hanging out of it (~60 m)
  lab_ring        one segment of the colossal ring (the game turns 40 of them round)
  lab_lamp        the surgical lamp over Final Assessment: round housing, seven lenses (they take the
                  game's tint: white, then red as the Successor fails), yoke and the first arm joint
  hadal_sleeper   the thing asleep in the trench: a curled spine of plates, ribs, knees drawn in,
                  its head tucked down, lights along the spine (they pulse with the game's tint)
  hadal_segment   one segment of the swimmer (the game strings 18 together and waves them)
"""
from pathlib import Path
import math
import random

repo = Path.cwd()
exec((repo / 'tools/blender/enemy_kit.py').read_text())
REPO = str(repo)
if 'const' not in _og.CHANNELS:
    _og.CHANNELS.append('const')
_og.PALETTE.update({'tile': (.74, .76, .8), 'deck': (.3, .32, .36), 'void': (.06, .07, .09),
                    'hide': (.1, .17, .21), 'bone2': (.5, .55, .6), 'window': (.05, .06, .08)})


def begin():
    for ob in list(bpy.context.scene.objects):
        bpy.data.objects.remove(ob, do_unlink=True)


def join_by_look():
    """One mesh per (material, glow channel, glow strength): fewer parts, the same picture"""
    groups = {}
    for ob in list(bpy.context.scene.objects):
        if ob.type != 'MESH':
            continue
        for m in ob.modifiers:   # bevels baked in before joining
            bpy.context.view_layer.objects.active = ob
            bpy.ops.object.modifier_apply(modifier=m.name)
        key = (ob.data.materials[0].name if ob.data.materials else '', ob.get('glow'), round(float(ob.get('k', 1.0)), 3))
        groups.setdefault(key, []).append(ob)
    for obs in groups.values():
        if len(obs) < 2:
            continue
        bpy.ops.object.select_all(action='DESELECT')
        for o in obs:
            o.select_set(True)
        bpy.context.view_layer.objects.active = obs[0]
        bpy.ops.object.join()


def save(name):
    join_by_look()
    out = repo / 'assets/props' / (name + '.glb')
    bpy.ops.export_scene.gltf(filepath=str(out), export_format='GLB', export_extras=True,
                              export_yup=True, export_apply=True, export_texcoords=False)
    shrink(str(out))
    source = repo / 'build/story-vista' / (name + '.blend')
    source.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    n = sum(1 for o in bpy.context.scene.objects if o.type == 'MESH')
    print('STORY VISTA: %s, %d parts' % (name, n))


# ---- a wing of the lab, adrift (60 x 16 x 34 m) ------------------------------------------------
begin()
rng = random.Random(3)
W, H, D = 60.0, 16.0, 34.0
B((0, 0, 0), (W, H, D), 'tile', None, bev=1.2, seg=2)
B((0, H * .5 + .6, 0), (W * .92, 1.2, D * .9), 'tile', None, bev=.4)              # roof parapet
for x in (-W * .3, W * .15):
    B((x, H * .5 + 2.5, rng.uniform(-8, 8)), (6, 4, 6), 'deck', None, bev=.3)       # plant on the roof
for row in range(3):                                                                # windows, both long faces
    y = H * (.28 - .24 * row)
    for k in range(9):
        x = W * (-.42 + .105 * k)
        for z in (D * .5, -D * .5):
            B((x, y, z), (W * .08, H * .12, .8), 'window', None)
            if rng.random() > .3:
                flick = rng.random() < .35
                B((x, y, z * 1.012), (W * .07, H * .1, .3), 'window', None, glow='glow' if flick else 'const',
                  k=1.0 if flick else (.9 if rng.random() < .8 else 1.1))
for k in range(9):                                                                  # pilasters between them
    x = W * (-.47 + .105 * k)
    for z in (D * .5, -D * .5):
        B((x, 0, z * 1.01), (.9, H * .95, .6), 'tile', None)
# The torn underside: a stepped, jagged deck, chunks hanging off it, rebar and cable out of the break
B((W * .08, -H * .62, 0), (W * .8, H * .3, D * .85), 'deck', None, bev=.5)
for k in range(10):
    x, z = rng.uniform(-W * .4, W * .4), rng.uniform(-D * .38, D * .38)
    B((x, -H * .8 - rng.uniform(0, 3), z), (rng.uniform(4, 10), rng.uniform(2, 5), rng.uniform(3, 8)), 'void' if k % 3 else 'deck', None,
      rot=(rng.uniform(-25, 25), rng.uniform(0, 90), rng.uniform(-25, 25)), bev=.3)
for k in range(14):
    x, z = rng.uniform(-W * .45, W * .45), rng.uniform(-D * .4, D * .4)
    L = rng.uniform(4, 14)
    C((x, -H * .75 - L * .5, z), .22, L, 'void', None, rot=(rng.uniform(-12, 12), 0, rng.uniform(-12, 12)), sides=6)
for k in range(5):                                                                  # cable bundles, longer
    x = W * (-.4 + .2 * k)
    L = rng.uniform(18, 34)
    C((x, -H * .7 - L * .5, rng.uniform(-6, 6)), .6, L, 'void', None, rot=(rng.uniform(-6, 6), 0, rng.uniform(-6, 6)), sides=6)
save('lab_wing')

# ---- one segment of the ring (the game turns 40 round a 110 m radius) ---------------------------
begin()
B((0, 0, 0), (7, 18, 9), 'tile', None, bev=.4)
B((-3.6, 0, 0), (.6, 18, 4), 'void', None, glow='glow', k=1.0)                       # the band (the game's tint)
B((3.8, 0, 0), (1.2, 16, 10), 'deck', None)
for y in (-6, 0, 6):
    B((0, y, 4.8), (5, 1.2, .6), 'deck', None)
save('lab_ring')

# ---- the surgical lamp (housing ~70 m across) --------------------------------------------------
begin()
C((0, 6, 0), 36, 10, 'deck', None, sides=32)                                         # housing: game +Y up
C((0, 12, 0), 28, 4, 'tile', None, sides=32)
C((0, 15.5, 0), 10, 4, 'deck', None, sides=16)
R((0, 1, 0), 36, 1.4, 'tile', None, rot=(90, 0, 0), sides=40, tsides=6)              # the rim
C((0, .9, 0), 10, 1.6, 'void', None, sides=24)
C((0, .2, 0), 8, .4, 'window', None, sides=24, glow='glow', k=.6)                    # centre lens
for k in range(6):
    a = k * math.pi / 3
    x, z = math.cos(a) * 24, math.sin(a) * 24
    C((x, 1.2, z), 8, 2.4, 'deck', None, sides=20)
    C((x, .1, z), 6.6, .3, 'window', None, sides=20, glow='glow', k=1.0)               # the lenses
    C((x * .55, 3, z * .55), .8, 18, 'void', None, rot=(0, 0, 90), sides=6)            # struts
for s in (-1, 1):                                                                     # the yoke
    B((s * 38, 14, 0), (4, 20, 8), 'deck', None, bev=.4)
B((0, 25, 0), (80, 4, 8), 'deck', None, bev=.4)
C((0, 30, 0), 7, 10, 'tile', None, sides=16)
save('lab_lamp')

# ---- the sleeper in the trench (~160 m curled) -------------------------------------------------
begin()
rng = random.Random(9)
for k in range(15):                                                                   # the curled spine, in the XY plane
    a = -1.25 + k * .21
    x, y = math.cos(a) * 70, math.sin(a) * 70
    deg = math.degrees(a)
    wide = 34 - abs(k - 7) * 1.6
    B((x, y, 0), (24, 16, wide), 'hide', None, rot=(0, 0, deg), bev=2.0)
    B((x * 1.08, y * 1.08, 0), (8, 6, wide * .6), 'bone2', None, rot=(0, 0, deg), bev=1.0)   # the spine ridge
    B((x * 1.16, y * 1.16, 0), (2, 3, 4), 'void', None, rot=(0, 0, deg), glow='glow', k=1.0 if k % 2 else .55)
    for s in (-1, 1):                                                                  # ribs
        if 3 < k < 12:
            B((x * .86, y * .86, s * wide * .45), (16, 4, 6), 'bone2', None, rot=(s * 20, 0, deg + 70), bev=.8)
B((8, -62, 0), (34, 30, 30), 'hide', None, rot=(0, 0, 28), bev=3)                     # the head, tucked down
B((22, -54, 14), (6, 3, 2), 'void', None, glow='glow', k=1.6)                          # one eye, half open
B((-4, -78, 0), (20, 10, 22), 'bone2', None, rot=(0, 0, 40), bev=2)                   # jaw
for s in (-1, 1):
    B((-20, -20, s * 22), (60, 14, 14), 'hide', None, rot=(0, 0, 52), bev=2)          # knees drawn in
    B((-5, -40, s * 25), (44, 12, 12), 'hide', None, rot=(0, 0, -23), bev=2)
    B((30, -30, s * 26), (40, 10, 10), 'hide', None, rot=(0, 0, 70), bev=2)           # an arm round them
save('hadal_sleeper')

# ---- one segment of the swimmer ----------------------------------------------------------------
begin()
B((0, 0, 0), (13, 10, 10), 'hide', None, bev=1.2, seg=2)
B((0, 5.5, 0), (3, 2, 8), 'bone2', None, bev=.5)
for s in (-1, 1):
    B((s * 6.8, 0, 0), (.6, 1.2, 2), 'void', None, glow='glow', k=1.0)                # its side lights
    B((s * 7, -2, -2), (4, .6, 6), 'hide', None, rot=(0, 0, s * 25))                  # fins
save('hadal_segment')
