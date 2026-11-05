"""Story dressing for the bare walls and road shoulders, authored in Blender. Run from the repo root:
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/story_dressing_build.py
Exports game GLBs to assets/props/ and editable scenes to build/story-dressing/.

  cradle_pipe_run   THE CRADLE: a 4 m wall-mounted pipe bundle (length along X, the wall at z 0, standing
                    off toward +Z). LevelStory.h runs it along the halls' tall walls, out of reach.
  waste_car_wreck   THE DEAD SPAN: a burnt-out car shell, 4.3 m along Z, a wheel gone, the boot sprung.
  waste_road_sign   THE DEAD SPAN: a road sign knocked crooked and shot through, on its footing.

Each part's surface (`tex`) goes by what it's made of: painted metal takes the chipped paint, bare
steel the chapter's metal (hull steel in the Cradle, rusted sheet on the Dead Span), concrete its own.
"""
from pathlib import Path
import random

repo = Path.cwd()
exec((repo / 'tools/blender/enemy_kit.py').read_text())
REPO = str(repo)
if 'const' not in _og.CHANNELS:
    _og.CHANNELS.append('const')
_og.PALETTE.update({'steel': (.27, .31, .33), 'darksteel': (.09, .12, .14), 'porcelain': (.63, .62, .55),
                    'ash': (.47, .43, .38), 'paint': (.56, .5, .36), 'carpaint': (.42, .2, .14),
                    'blacksteel': (.08, .1, .11), 'duststeel': (.35, .36, .32), 'concrete': (.4, .38, .33),
                    'char': (.06, .05, .05), 'signwhite': (.62, .6, .52)})
TEX = {'hazard': 'painted', 'porcelain': 'painted', 'paint': 'painted', 'carpaint': 'painted', 'signwhite': 'painted',
       'rust': 'painted', 'concrete': 'concrete', 'ash': 'rock'}


def begin():
    for ob in list(bpy.context.scene.objects):
        bpy.data.objects.remove(ob, do_unlink=True)


def save(name):
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH' and 'glow' not in ob:
            material = ob.data.materials[0].name.split('*')[0] if ob.data.materials else ''
            ob['tex'] = TEX.get(material, 'metal')
    out = repo / 'assets/props' / (name + '.glb')
    bpy.ops.export_scene.gltf(filepath=str(out), export_format='GLB', export_extras=True,
                              export_yup=True, export_apply=True, export_texcoords=False)
    shrink(str(out))
    source = repo / 'build/story-dressing' / (name + '.blend')
    source.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    print('STORY DRESSING:', name)


# ---- THE CRADLE: a pipe run ---------------------------------------------------------------------
# Three lines on brackets: a fat process main with flanged joints, a valve and a hazard band; a
# steam line with a gauge; a thin conduit above. A drip tray under the valve.
begin()
B((0, .5, .03), (4.0, .1, .06), 'darksteel', None)                      # the wall rail
B((0, .02, .03), (4.0, .06, .05), 'darksteel', None)
for x in (-1.5, 0.0, 1.5):                                               # brackets
    B((x, .5, .22), (.07, 1.0, .38), 'steel', None, bev=.01)
    for y in (.25, .55):
        B((x, y - .17, .36), (.12, .03, .32), 'darksteel', None)        # a saddle under each line
C((0, .25, .36), .15, 4.0, 'steel', None, rot=(0, 0, 90), sides=14)    # the main
for x in (-1.0, 1.0):
    C((x, .25, .36), .2, .09, 'darksteel', None, rot=(0, 0, 90), sides=14)   # flanges
    for a in range(6):                                                   # their bolts
        import math
        t = a * math.pi / 3
        C((x, .25 + .17 * math.sin(t), .36 + .17 * math.cos(t)), .018, .14, 'edge', None, rot=(0, 0, 90), sides=6)
C((1.65, .25, .36), .155, .3, 'hazard', None, rot=(0, 0, 90), sides=14)   # the line's hazard band
for k in range(3):
    B((1.53 + k * .12, .25, .52), (.05, .2, .02), 'black', None, rot=(30, 0, 0))
C((.55, .25, .55), .03, .28, 'steel', None, rot=(90, 0, 0), sides=8)    # the valve: stem, bonnet, wheel
C((.55, .25, .5), .07, .12, 'darksteel', None, rot=(90, 0, 0), sides=10)
R((.55, .25, .7), .14, .022, 'hazard', None, sides=20, tsides=6)
for a in (0, 60, 120):
    B((.55, .25, .7), (.26, .025, .02), 'hazard', None, rot=(0, 0, a))
B((.55, -.05, .4), (.5, .04, .34), 'rust', None)                        # the drip tray
C((0, .56, .3), .09, 4.0, 'rust', None, rot=(0, 0, 90), sides=12)      # the steam line
C((-.6, .56, .3), .13, .1, 'darksteel', None, rot=(0, 0, 90), sides=12)
C((-.6, .56, .44), .015, .2, 'steel', None, rot=(90, 0, 0), sides=6)  # a gauge on a stub
C((-.6, .56, .56), .08, .05, 'darksteel', None, rot=(90, 0, 0), sides=14)
C((-.6, .56, .585), .065, .01, 'porcelain', None, rot=(90, 0, 0), sides=14, glow='const', k=.35)
B((-.6, .58, .595), (.012, .07, .01), 'black', None, rot=(0, 0, -35))
C((0, .84, .18), .045, 4.0, 'darksteel', None, rot=(0, 0, 90), sides=8)   # conduit
for x in (-1.9, -.75, .75, 1.9):
    B((x, .84, .18), (.03, .12, .12), 'steel', None)
save('cradle_pipe_run')

# ---- THE DEAD SPAN: a car wreck -----------------------------------------------------------------
# Burnt out and left: paint scorched to rust, the glass gone, the roof dented in, the boot sprung,
# the near front wheel missing (the hub on a block), ash drifted against it.
begin()
rng = random.Random(2)
B((0, .38, 0), (1.62, .22, 4.1), 'blacksteel', None, bev=.04)           # floorpan
B((0, .74, .05), (1.78, .55, 4.25), 'carpaint', None, bev=.12, seg=2)   # the body
B((0, 1.03, 1.42), (1.7, .1, 1.35), 'rust', None, rot=(-5, 0, 0), bev=.04)   # bonnet
B((0, 1.03, -1.75), (1.66, .1, .75), 'rust', None, bev=.03)
B((0, 1.22, -2.1), (1.6, .05, .72), 'carpaint', None, rot=(-48, 0, 0))   # the boot lid, sprung
for x in (-.8, .8):                                                       # pillars
    for z, lean in ((.72, -28), (-.95, 22)):
        B((x, 1.32, z), (.07, .62, .08), 'carpaint', None, rot=(lean, 0, 0))
    B((x, 1.34, -.12), (.06, .58, .07), 'carpaint', None)
B((.05, 1.62, -.12), (1.62, .07, 1.75), 'carpaint', None, rot=(3, 0, 4), bev=.03)   # the roof, dented
B((0, 1.1, -.15), (1.55, .3, 1.6), 'char', None)                         # the burnt-out cabin
for x in (-.42, .42):
    B((x, 1.12, .25), (.5, .12, .5), 'char', None)                       # seat frames
    B((x, 1.4, .02), (.5, .5, .08), 'char', None, rot=(-12, 0, 0))
C((-.38, 1.32, .55), .16, .03, 'blacksteel', None, rot=(60, 0, 0), sides=12)   # the wheel it was driven by
B((0, .7, 2.16), (1.5, .22, .08), 'blacksteel', None)                    # grille, bumpers
B((0, .55, 2.22), (1.82, .14, .12), 'duststeel', None, bev=.03)
B((0, .55, -2.18), (1.82, .14, .12), 'duststeel', None, rot=(0, 0, 6), bev=.03)
for x in (-.62, .62):
    B((x, .82, 2.17), (.26, .14, .04), 'char', None)                     # empty headlamps
for x, z, there in ((-.86, 1.35, False), (.86, 1.35, True), (-.86, -1.35, True), (.86, -1.35, True)):
    if there:
        C((x, .34, z), .34, .22, 'blacksteel', None, rot=(0, 0, 90), sides=14)
        C((x * 1.07, .34, z), .17, .04, 'duststeel', None, rot=(0, 0, 90), sides=10)
    else:
        C((x * .9, .5, z), .12, .1, 'duststeel', None, rot=(0, 0, 90), sides=10)   # bare hub
        B((x * .95, .14, z), (.3, .28, .36), 'concrete', None)                     # on a block
for k in range(5):                                                       # ash drifted against its side
    B((-1.0 - rng.uniform(0, .3), .1, -1.8 + k * .85), (.5, .2 + rng.uniform(0, .12), .9), 'ash', None,
      rot=(0, rng.uniform(-10, 10), rng.uniform(8, 18)))
save('waste_car_wreck')

# ---- THE DEAD SPAN: a road sign -----------------------------------------------------------------
begin()
B((0, .12, 0), (.5, .24, .5), 'concrete', None, bev=.03)               # footing
C((0, 1.35, 0), .05, 2.5, 'duststeel', None, rot=(0, 0, -4), sides=8)    # the post, knocked crooked
C((.1, 2.45, 0), .05, .5, 'duststeel', None, rot=(0, 0, -4), sides=8)
B((.14, 2.62, .06), (1.7, .82, .04), 'blacksteel', None, rot=(0, 0, -9))   # board and its face
B((.14, 2.62, .085), (1.6, .72, .02), 'paint', None, rot=(0, 0, -9))
B((.14, 2.62, .1), (1.42, .06, .01), 'signwhite', None, rot=(0, 0, -9))
for k in range(3):                                                       # an arrow, faded
    B((.75 - k * .06, 2.52 + k * .01, .1), (.22 - k * .05, .05, .01), 'signwhite', None, rot=(0, 0, -9 + (35 if k else 0)))
rng = random.Random(7)
for k in range(9):                                                       # shot through
    x, y = rng.uniform(-.6, .8), rng.uniform(2.35, 2.9)
    B((x, y, .1), (.035, .035, .03), 'char', None)
for z in (-.04, .04):
    B((0, 2.3, z), (.12, .06, .04), 'blacksteel', None)                  # clamps
save('waste_road_sign')
