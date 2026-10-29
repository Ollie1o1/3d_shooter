# The Sorter: a rail-mounted inspection machine with a scanner eye, two hydraulic
# claws and a reject stamp. Uses the engine's existing mechanical joints.
import os
exec(open(os.path.join(os.path.dirname(os.path.abspath('tools/blender/enemy_kit.py')), 'enemy_kit.py')).read())
NAME='sorter'
NEEDED[NAME]='torso head armL armR elbowL elbowR handL handR'
sc=bpy.context.scene
for ob in list(sc.objects): bpy.data.objects.remove(ob,do_unlink=True)
sc.name='SORTER';sc['overdrive_opened']=NAME
add_joint('torso',(0,3,0))
add_joint('head',(0,4.4,0),'torso')
for s,side in ((1,'L'),(-1,'R')):
    add_joint('arm'+side,(s*2.5,3.8,0),'torso')
    add_joint('elbow'+side,(s*3.8,1.7,.5),'arm'+side)
    add_joint('hand'+side,(s*3.6,.25,1),'elbow'+side)
B((0,3.4,0),(5.4,1.25,2.6),'iron','torso',bev=.14)
B((0,4.2,-.1),(3.4,.45,2),'hazard','torso',bev=.06)
for x in (-2.3,2.3):
    B((x,5.4,-.4),(.5,2.8,.7),'steel','torso',bev=.08)
    C((x,6.5,-.4),.4,.35,'dark','torso',rot=(90,0,0),sides=12)
for x in (-1.7,-.85,0,.85,1.7):
    B((x,3.35,1.34),(.45,.45,.12),'glow','torso',glow='core',k=1.5)
B((0,4.9,.5),(2.4,1.15,1.7),'enamel','head',bev=.12)
B((0,4.92,1.39),(2.15,.75,.18),'dark','head',bev=.1)
C((0,4.92,1.53),.32,.12,'glow','head',rot=(90,0,0),sides=16,glow='eye',k=2)
for x in (-.83,.83):B((x,4.92,1.52),(.2,.5,.12),'glow','head',glow='eye',k=1.2)
# Stamp in the middle; its piston shortens as the machine recovers.
C((0,1.8,0),.35,2.4,'steel','torso',sides=12)
B((0,.5,0),(2.5,.6,2),'iron','torso',bev=.1)
for x in (-.9,-.3,.3,.9):B((x,.16,.2),(.2,.12,1.25),'hazard','torso')
for s,side in ((1,'L'),(-1,'R')):
    a,e,h='arm'+side,'elbow'+side,'hand'+side
    E((s*2.5,3.8,0),(.7,.7,.7),'steel',a,seg=10,rings=6)
    B((s*3.15,2.8,.25),(.85,2.5,.9),'iron',a,rot=(0,0,s*30),bev=.08)
    B((s*3.2,2.9,.76),(.45,1.6,.1),'hazard',a,rot=(0,0,s*30))
    E((s*3.8,1.7,.5),(.6,.6,.6),'steel',e,seg=10,rings=6)
    B((s*3.7,1,.75),(.45,1.6,.5),'steel',e,bev=.05)
    B((s*3.6,.3,1),(1.2,.45,1.2),'iron',h,bev=.08)
    for dx in (-.48,.48):
        B((s*3.6+dx,-.1,1),(.22,.8,.8),'enamel',h,rot=(0,0,-dx*30),bev=.04)
        B((s*3.6+dx*.7,-.45,1.2),(.35,.2,.6),'steel',h,bev=.03)
    B((s*3.6,.55,1.45),(.7,.12,.08),'glow',h,glow='edge',k=1)
save_enemy(NAME)
