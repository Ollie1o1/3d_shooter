"""THE DEAD SPAN: detailed wasteland infrastructure, game GLBs and editable scenes.
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/wasteland_props_build.py
"""
from pathlib import Path
import math
import random
repo=Path.cwd()
exec((repo/'tools/blender/enemy_kit.py').read_text())
REPO=str(repo)
if 'const' not in _og.CHANNELS:_og.CHANNELS.append('const')
_og.PALETTE.update({'ash':(.47,.43,.38),'road':(.2,.22,.23),'duststeel':(.35,.36,.32),
                    'paint':(.65,.57,.38),'blacksteel':(.08,.1,.11),'concrete':(.4,.38,.33)})
def begin():
 for ob in list(bpy.context.scene.objects):bpy.data.objects.remove(ob,do_unlink=True)
def save(name):
 for ob in bpy.context.scene.objects:
  if ob.type=='MESH' and 'glow' not in ob:
   material=ob.data.materials[0].name.split('*')[0] if ob.data.materials else ''
   ob['tex']='concrete' if material=='concrete' else 'rock' if material=='ash' else 'metal'
 path=repo/'assets/props'/(name+'.glb')
 bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',export_extras=True,export_yup=True,export_apply=True,export_texcoords=False)
 shrink(str(path))
 folder=repo/'build/wasteland-props';folder.mkdir(parents=True,exist_ok=True)
 bpy.ops.wm.save_as_mainfile(filepath=str(folder/(name+'.blend')))
 print('WASTELAND PROP:',name)

# Six-metre freight hauler, one cab panel missing, exposed axles and ribbed load.
begin()
B((0,.65,0),(3.1,.5,6.4),'blacksteel',None,bev=.06)
B((0,1.4,2),(3,1.5,1.9),'paint',None,bev=.12)
B((0,1.8,2.98),(2.5,.65,.035),'blacksteel',None)
for x in (-1.3,1.3):
 B((x,1.4,2.1),(.1,1.2,1.6),'duststeel',None)
 B((x,1.83,2.4),(.12,.42,.6),'blacksteel',None)
for z in (-2,-.2,2):
 C((0,.57,z),.13,3.8,'rust',None,rot=(0,0,90),sides=10)
 for x in (-1.65,1.65):
  C((x,.6,z),.6,.35,'blacksteel',None,rot=(0,0,90),sides=14)
  C((x*1.11,.6,z),.28,.06,'duststeel',None,rot=(0,0,90),sides=10)
B((0,1.05,-1.4),(2.9,.25,3.8),'rust',None,bev=.03)
for x in (-1.4,1.4):
 for k in range(6):B((x,1.5,-2.8+k*.55),(.12,1.1,.18),'duststeel',None)
 B((x,2,-1.35),(.14,.15,3.8),'duststeel',None)
for k in range(5):B((-.8+k*.4,1.3,-1.5),(.28,.3,2.7),'concrete',None,rot=(0,k*4,3),bev=.05)
for x in (-1.1,1.1):
 B((x,.97,3.2),(.38,.21,.09),'duststeel',None)
 B((x,.97,3.27),(.23,.12,.025),'paint',None)
save('waste_hauler')

# Transmission pylon: bolted concrete foot, open lattice, cross arms and ceramic insulators.
begin()
B((0,.55,0),(2.8,1.1,2.8),'concrete',None,bev=.08)
for x in (-1.1,1.1):
 for z in (-1.1,1.1):
  B((x,7,z),(.22,13,.22),'duststeel',None,rot=(0,0,0))
  C((x,1.16,z),.09,.12,'rust',None,sides=6)
for y in (2,4,6,8,10,12):
 for z in (-1.1,1.1):
  B((0,y,z),(2.2,.16,.16),'rust',None)
  B((0,y+1,z),(.14,2.9,.14),'duststeel',None,rot=(0,0,48 if y%4 else -48))
 for x in (-1.1,1.1):B((x,y+1,0),(.14,2.9,.14),'duststeel',None,rot=(48,0,0))
for y,w in ((11,6.5),(13,8)):
 B((0,y,0),(w,.25,.4),'duststeel',None)
 for x in (-w*.42,w*.42):
  for k in range(4):C((x,y-.2-k*.23,0),.22,.13,'bone',None,sides=10)
  B((x,y-.7,0),(.06,1.5,.06),'rust',None)
save('waste_pylon')

# A stalled wind turbine. Broken blade tips expose the frame.
begin()
B((0,.3,0),(3.5,.6,3.5),'concrete',None,bev=.1)
C((0,6,0),.7,11.4,'duststeel',None,sides=14,r2=.42)
B((0,11.6,-.5),(2.2,1.6,3.2),'paint',None,bev=.14)
C((0,11.6,1.2),.65,.6,'duststeel',None,rot=(90,0,0),sides=16)
save('waste_turbine_frame')
for k in range(3):
 a=math.radians(k*120+15)
 # Blade local centres in the XY plane; the game's Z is its rotor axis.
 x,y=math.sin(a)*2.8,11.6+math.cos(a)*2.8
 P((x,y,1.65),(.75,5.2,.18),'paint' if k!=1 else 'rust',None,rot=(0,0,-math.degrees(a)),top=(.35,.8),bev=.03)
 for t in (1.5,2.5,3.5):
  B((math.sin(a)*t,11.6+math.cos(a)*t,1.78),(.32,.08,.05),'duststeel',None,rot=(0,0,-math.degrees(a)))
for y in (1.5,3,4.5,6,7.5,9):
 B((.72,y,0),(.07,.08,.65),'rust',None)
B((0,1.2,.73),(.65,.9,.06),'blacksteel',None)
save('waste_turbine')

# Relay face: dusty switchgear, cracked ceramic panel, cable glands and gauge.
begin()
B((0,2,0),(3.4,4,.4),'concrete',None,bev=.07)
B((-.45,2,.3),(1.7,2.6,.3),'duststeel',None,bev=.05)
for y in (1.1,1.5,1.9,2.3,2.7):B((-.45,y,.5),(1.25,.05,.04),'blacksteel',None)
C((.95,2.7,.38),.34,.18,'bone',None,rot=(90,0,0),sides=16)
B((.95,2.72,.5),(.025,.39,.015),'blacksteel',None,rot=(0,0,25))
for x in (.5,1,1.5):
 C((x,.8,.28),.08,1.4,'rust',None,sides=8)
 B((x,.4,.43),(.2,.15,.06),'paint',None)
B((-.8,3.3,.48),(.18,.12,.035),'paint',None,glow='const',k=(.45,.3,.08))
save('waste_relay_panel')

# Heavy abandoned cable reel, suitable for readable low cover.
begin()
for x in (-1,1):
 C((x,1.15,0),1.1,.18,'duststeel',None,rot=(0,0,90),sides=16)
 for k in range(8):
  a=k*math.tau/8
  B((x*1.1,1.15+math.sin(a)*.7,math.cos(a)*.7),(.04,.1,.1),'rust',None)
C((0,1.15,0),.78,1.9,'blacksteel',None,rot=(0,0,90),sides=20)
for x in (-.8,-.5,-.2,.1,.4,.7):C((x,1.15,0),.82,.06,'rust',None,rot=(0,0,90),sides=20)
B((0,.1,0),(2.3,.2,2.3),'concrete',None,bev=.03)
save('waste_cable_reel')

# Low debris islands: fractured road slabs, pipe, loose bolts and ash drifts.
begin()
rng=random.Random(209)
for k in range(14):
 x,z=rng.uniform(-2.1,2.1),rng.uniform(-1.3,1.3)
 P((x,.08+rng.uniform(0,.12),z),(rng.uniform(.3,1.1),rng.uniform(.08,.2),rng.uniform(.3,.9)),
   'concrete' if k%3 else 'road',None,rot=(rng.uniform(-8,8),k*47,rng.uniform(-10,10)),top=(.7,.85),bev=.02)
C((.8,.18,-.6),.17,1.7,'rust',None,rot=(0,0,85),sides=10)
for k in range(5):B((-1+k*.3,.12,.7),(.07,.1,.6),'duststeel',None,rot=(0,k*13,0))
save('waste_road_debris')
