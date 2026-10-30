"""Room-specific factory equipment. Run from the repository root:
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/cradle_room_props_build.py
Saves GLBs in assets/props and editable scenes in build/cradle-props.
"""
from pathlib import Path
import math
import random
repo=Path.cwd()
exec((repo/'tools/blender/enemy_kit.py').read_text())
REPO=str(repo)
if 'const' not in _og.CHANNELS: _og.CHANNELS.append('const')
_og.PALETTE.update({'steel':(.32,.36,.37),'deepsteel':(.08,.12,.14),'ceramic':(.65,.68,.63),
                    'bluecase':(.16,.27,.32),'copper':(.5,.28,.13),'sage':(.29,.36,.3)})

def begin():
    for ob in list(bpy.context.scene.objects): bpy.data.objects.remove(ob,do_unlink=True)

def save(name):
    for ob in bpy.context.scene.objects:
        if ob.type=='MESH' and 'glow' not in ob: ob['tex']='metal'
    out=repo/'assets/props'/(name+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(out),export_format='GLB',export_extras=True,
                             export_yup=True,export_apply=True,export_texcoords=False)
    shrink(str(out))
    source=repo/'build/cradle-props'/(name+'.blend');source.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    print('ROOM PROP:',name)

# Pump skid: a large round silhouette, ribbed motor, flanges, valve and gauges.
begin()
B((0,.16,0),(3.5,.32,1.65),'deepsteel',None,bev=.04)
C((-.65,1.05,0),.68,1.6,'bluecase',None,rot=(0,0,90),sides=16)
for x in (-1.35,-.95,-.55,-.15):
    C((x,1.05,0),.73,.08,'steel',None,rot=(0,0,90),sides=16)
C((.65,1.05,0),.5,1.,'copper',None,rot=(0,0,90),sides=14)
for x in (.2,1.1):
    C((x,1.05,0),.65,.13,'steel',None,rot=(0,0,90),sides=12)
C((1.25,1.8,0),.19,1.6,'copper',None,sides=12)
C((1.25,2.55,0),.3,.12,'steel',None,sides=12)
C((1.25,2.1,.32),.3,.09,'hazard',None,rot=(90,0,0),sides=12)
C((.3,1.85,.35),.2,.12,'ceramic',None,rot=(90,0,0),sides=12)
B((.3,1.87,.43),(.025,.22,.015),'deepsteel',None,rot=(0,0,-35))
for x in (-1.35,1.35):
    for z in (-.65,.65): C((x,.35,z),.08,.09,'silver',None,sides=6)
save('cradle_pump_skid')

# Detailed server bank: recessed modules, handles, vent slots and tiny lamps.
begin()
B((0,2.5,0),(2.9,5,1.45),'deepsteel',None,bev=.06)
for x in (-1.42,1.42): B((x,2.5,.78),(.12,5,.16),'steel',None)
for k in range(8):
    y=.35+k*.57
    B((0,y,.78),(2.62,.46,.12),'bluecase' if k%2 else 'steel',None,bev=.025)
    for x in (-1.05,.95): B((x,y,.88),(.07,.26,.08),'deepsteel',None)
    for x in (-.75,-.5,-.25,0,.25): B((x,y,.85),(.1,.025,.02),'deepsteel',None)
    B((.65,y,.86),(.1,.08,.025),'ceramic',None,glow='const',k=(.12,.48,.5))
    B((.85,y,.86),(.06,.08,.025),'hazard',None,glow='const',k=(.4,.23,.06))
B((0,5.05,0),(3.08,.14,1.6),'steel',None,bev=.03)
save('cradle_archive_bank')

# Fabrication fixture: open arch; the centre remains visibly open around a unit.
begin()
for x in (-3,3):
    B((x,.16,0),(.9,.32,2.6),'deepsteel',None,bev=.05)
    B((x,2.5,0),(.48,4.7,.7),'steel',None,bev=.035)
    B((x,2.5,.38),(.18,3.8,.06),'hazard',None)
    for y in (.8,1.5,2.2,2.9,3.6,4.3): B((x,y,.44),(.25,.08,.055),'deepsteel',None)
    C((x,3.8,0),.38,.85,'bluecase',None,rot=(90,0,0),sides=12)
    B((x*.64,3.5,0),(1.2,.22,.3),'steel',None,rot=(0,0,15 if x<0 else -15))
B((0,4.8,0),(7.2,.65,1.1),'deepsteel',None,bev=.04)
B((0,4.65,.58),(2,.2,.06),'hazard',None,glow='const',k=(.7,.43,.12))
for x in (-1.7,0,1.7): C((x,4.8,-.6),.28,.32,'copper',None,rot=(90,0,0),sides=10)
save('cradle_fabrication_frame')

# A pod's backrest and coolant spine: pipes, restraint arms and head contacts.
begin()
B((0,.18,-.25),(2.3,.36,2),'deepsteel',None,bev=.05)
B((0,2.3,-.9),(1.7,3.9,.2),'ceramic',None,bev=.06)
for x in (-1.02,1.02):
    C((x,2.3,-.85),.12,3.9,'copper',None,sides=10)
    for y in (.6,2.3,4.1): C((x,y,-.85),.2,.14,'steel',None,sides=10)
    B((x*.82,1.2,-.35),(.3,.2,1.1),'steel',None,bev=.025)
B((0,4.35,-.4),(2,.3,1.1),'deepsteel',None,bev=.04)
B((0,4.15,-.82),(.8,.12,.06),'ceramic',None,glow='const',k=(.13,.4,.65))
for x in (-.6,.6): B((x,3.3,-.65),(.25,.55,.2),'bluecase',None,bev=.03)
save('cradle_pod_spine')

# Square extractor housing with a recessed, circular intake and grille bars.
begin()
B((0,1.5,-.2),(3.4,3,.5),'deepsteel',None,bev=.08)
for x in (-1.55,1.55): B((x,1.5,.1),(.25,3.2,.28),'steel',None,bev=.025)
for y in (.05,2.95): B((0,y,.1),(3.4,.18,.28),'steel',None)
C((0,1.5,.12),1.25,.18,'steel',None,rot=(90,0,0),sides=20)
C((0,1.5,.24),1.08,.08,'deepsteel',None,rot=(90,0,0),sides=20)
for x in (-.8,-.4,0,.4,.8): B((x,1.5,.34),(.045,2,.06),'steel',None)
for x in (-1.48,1.48):
    for y in (.3,2.7): C((x,y,.28),.06,.08,'silver',None,rot=(90,0,0),sides=6)
save('cradle_extractor')

# Compressed salvage: full rectangular volume and flat load-bearing top,
# with layered plate fragments and exposed frames across all four faces.
begin()
B((0,.5,0),(.99,1,.99),'deepsteel',None,bev=.015)
rng=random.Random(240)
for face in range(4):
    for k in range(12):
        x=rng.uniform(-.37,.37);y=.12+(k%4)*.24
        if face<2:
            at=(x,y,(-1 if face==0 else 1)*.505);size=(rng.uniform(.2,.46),.17,.045)
        else:
            at=((-1 if face==2 else 1)*.505,y,x);size=(.045,.17,rng.uniform(.2,.46))
        B(at,size,'ceramic' if k%5==0 else 'rust' if k%3==0 else 'steel',None,bev=.012)
for z in (-.3,.1,.35): B((0,1.015,z),(.96,.025,.12),'steel',None)
save('cradle_compacted_scrap')

# Low wheeled service cart with tools and an abandoned component.
begin()
B((0,.6,0),(2,.12,1.2),'steel',None,bev=.035)
B((0,.22,0),(1.8,.08,1),'deepsteel',None)
for x in (-.8,.8):
    for z in (-.45,.45):
        B((x,.4,z),(.07,.48,.07),'steel',None)
        C((x,.12,z),.12,.08,'deepsteel',None,rot=(90,0,0),sides=10)
for z in (-.5,.5): B((1.02,.88,z),(.06,.6,.06),'steel',None)
B((1.02,1.16,0),(.06,.06,1.06),'steel',None)
B((-.35,.75,0),(.55,.2,.5),'ceramic',None,rot=(0,18,0),bev=.04)
for k in range(3): B((.1+k*.18,.69,.2),(.07,.04,.5),'copper',None,rot=(0,k*12,0))
save('cradle_service_cart')
