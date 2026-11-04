"""Chapter 1 industrial dressing, authored in Blender. Run from the repo root:
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/cradle_props_build.py
Exports four game-ready GLBs and editable source scenes under build/cradle-props/.
"""
from pathlib import Path
import math
import random

repo = Path.cwd()
exec((repo / 'tools/blender/enemy_kit.py').read_text())
REPO = str(repo)  # Keep this recipe portable when run from another checkout.
if 'const' not in _og.CHANNELS:
    _og.CHANNELS.append('const')
_og.PALETTE.update({'steel': (.27,.31,.33), 'darksteel': (.09,.12,.14),
                    'porcelain': (.63,.62,.55), 'oil': (.055,.044,.032)})

def begin():
    for ob in list(bpy.context.scene.objects):
        bpy.data.objects.remove(ob, do_unlink=True)

def save(name):
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH' and 'glow' not in ob:
            ob['tex'] = 'metal'
    out = repo / 'assets/props' / (name + '.glb')
    bpy.ops.export_scene.gltf(filepath=str(out), export_format='GLB', export_extras=True,
                              export_yup=True, export_apply=True, export_texcoords=False)
    shrink(str(out))
    source = repo / 'build/cradle-props' / (name + '.blend')
    source.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    print('CRADLE PROP:', name)

# A broken refuse-transfer hatch. West side is open to approach on foot;
# the peeled grate, thick collar, rollers and hydraulic arms explain its use.
begin()
for x in (-1.85, 1.85):
    B((x,.08,0),(.22,.16,5.2),'steel',None,bev=.035)
    for z in (-2,-1,0,1,2):
        B((x,.18,z),(.24,.07,.4),'hazard',None)
for z in (-2.6,2.6):
    B((0,.08,z),(3.9,.16,.22),'steel',None,bev=.03)
    C((0,-.25,z),.18,3.5,'darksteel',None,rot=(0,0,90),sides=12)
# Park a buckled grille up against the eastern housing, clear of the hole.
for z in (-2.3,-1.7,-1.1,-.5,.1,.7,1.3,1.9,2.3):
    B((1.96,.85,z),(.1,1.5,.08),'rust',None,rot=(0,0,12))
for y in (.25,1.4):
    B((2.02,y,0),(.12,.1,4.8),'steel',None)
for z in (-2.3,2.3):
    C((2.12,.6,z),.11,1.2,'silver',None,sides=10)
    B((1.7,.22,z),(.2,.1,.6),'darksteel',None)
save('cradle_reject_hatch')

# A low tangle of cracked armour, snapped girders, gears and dead cables.
# Small enough to sit on a heap or beside a route without hiding its collision.
begin()
rng=random.Random(24)
for k in range(16):
    x,z=rng.uniform(-1.65,1.65),rng.uniform(-1.15,1.15)
    y=.13+rng.uniform(0,.3)
    B((x,y,z),(rng.uniform(.25,.85),.09,rng.uniform(.3,.85)),
      'porcelain' if k%3==0 else 'rust' if k%3==1 else 'steel',None,
      rot=(rng.uniform(-18,18),rng.uniform(-180,180),rng.uniform(-16,16)),bev=.025)
for k in range(5):
    C((-1.3+k*.6,.15,(-1)**k*.6),.12,.7,'darksteel',None,rot=(70,0,k*27),sides=8)
for k in range(12):
    a=k*math.tau/12
    B((.65+math.cos(a)*.44,.12,-.25+math.sin(a)*.44),(.25,.1,.09),'darksteel',None,rot=(0,-math.degrees(a),0))
C((-.7,.24,.3),.3,.11,'steel',None,sides=12)
C((-.7,.31,.3),.14,.08,'darksteel',None,sides=10)
save('cradle_scrap_cluster')

# A reinforced open collection bin with a visibly overflowing contents line.
begin()
B((0,.12,0),(2.7,.24,1.9),'darksteel',None,bev=.04)
for x in (-1.3,1.3):
    B((x,.8,0),(.12,1.4,1.9),'steel',None,bev=.025)
for z in (-.9,.9):
    B((0,.8,z),(2.6,1.4,.12),'steel',None,bev=.025)
    B((0,1.3,z*1.08),(2.75,.14,.1),'rust',None)
    for x in (-.9,-.45,0,.45,.9):
        B((x,.7,z*1.08),(.16,.22,.025),'hazard',None,rot=(0,0,22))
for k in range(9):
    B((rng.uniform(-1,1),1.3+rng.uniform(0,.35),rng.uniform(-.65,.65)),(.7,.15,.5),
      'porcelain' if k%2 else 'rust',None,rot=(8,k*47,16),bev=.025)
save('cradle_salvage_bin')

# Reusable wall machinery: switchgear, capped conduits, valve and warning lamps.
begin()
B((0,1.3,0),(1.65,2.6,.32),'darksteel',None,bev=.04)
B((-.35,1.7,.19),(.75,1.15,.16),'steel',None,bev=.025)
for y in (1.4,1.65,1.9,2.15):
    B((-.35,y,.29),(.55,.04,.025),'darksteel',None)
for x in (.3,.65):
    C((x,1.3,.3),.09,2.3,'rust',None,sides=8)
    C((x,2.45,.3),.15,.13,'steel',None,sides=10)
C((.4,1,.47),.29,.08,'hazard',None,rot=(90,0,0),sides=12)
C((.4,1,.53),.18,.1,'darksteel',None,rot=(90,0,0),sides=10)
for x in (-.4,-.1):
    B((x,.6,.22),(.12,.12,.07),'hazard',None,glow='const',k=(1.1,.45,.08))
save('cradle_switchgear')
