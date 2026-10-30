"""Rebuild only the turbine frame used by the animated Story machines."""
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

# A stalled wind turbine. Broken blade tips expose the frame.
begin()
B((0,.3,0),(3.5,.6,3.5),'concrete',None,bev=.1)
C((0,6,0),.7,11.4,'duststeel',None,sides=14,r2=.42)
B((0,11.6,-.5),(2.2,1.6,3.2),'paint',None,bev=.14)
C((0,11.6,1.2),.65,.6,'duststeel',None,rot=(90,0,0),sides=16)
save('waste_turbine_frame')
