# Ballast: a weathered, four-legged clearance grader carrying a salvaged turbine.
# All moving parts use engine-native joints; this is a separate model and silhouette.
import os, math
exec(open(os.path.join(os.path.dirname(os.path.abspath('tools/blender/enemy_kit.py')), 'enemy_kit.py')).read())
_og.PALETTE.update({'ball_iron':(.29,.26,.21),'ball_paint':(.49,.40,.24),
                    'ball_enamel':(.63,.61,.50),'ball_rust':(.31,.17,.09),'ball_black':(.08,.095,.09)})
NAME='ballast'
NEEDED[NAME]='body head legFL legFR legBL legBR halo shield armR handR'
sc=bpy.context.scene
for ob in list(sc.objects):bpy.data.objects.remove(ob,do_unlink=True)
sc.name='BALLAST';sc['overdrive_opened']=NAME
add_joint('body',(0,2.5,0))
add_joint('head',(0,4.1,2.3),'body')
add_joint('halo',(0,5.9,-1.2),'body')
add_joint('shield',(0,1.05,3.7),'body')
add_joint('armR',(-2.7,3.7,0),'body')
add_joint('handR',(-3.8,1.5,.8),'armR')
for side,x in (('L',2.2),('R',-2.2)):
 for end,z in (('F',1.5),('B',-1.8)):
  j='leg'+end+side;add_joint(j,(x,2.5,z),'body')
  B((x,1.65,z),(.8,2.0,1.0),'ball_iron',j,rot=(0,0,(-1 if x<0 else 1)*16),bev=.1)
  C((x,2.45,z),.62,1.05,'steel',j,rot=(90,0,0),sides=12)
  B((x*1.24,.32,z),(1.5,.6,2.0),'ball_black',j,bev=.06)
  B((x*1.24,.66,z),(1.3,.12,1.8),'ball_paint',j)
# Low broad engine deck, sloped hood, service panels and worn enamel ribs.
B((0,2.9,-.2),(5.6,1.8,5.2),'ball_iron','body',bev=.16)
B((0,3.85,1.15),(3.8,.6,2.2),'ball_enamel','body',rot=(12,0,0),bev=.08)
for x in (-2.82,2.82):
 B((x,2.85,-.4),(.2,1.15,4.5),'ball_paint','body',bev=.025)
 for z in (-2.1,-1.2,-.3,.6,1.5):B((x*1.01,2.95,z),(.08,.65,.12),'ball_black','body',rot=(0,0,20))
for x in (-1.4,1.4):
 C((x,4.3,-2),.24,1.7,'steel','body',sides=10)
 C((x,5.16,-2),.32,.18,'ball_black','body',sides=10)
for x in (-1.7,-.8,.1,1):B((x,3.15,2.48),(.65,.2,.1),'ball_black','body')
# Narrow, offset cab visor. It reads as an operator rather than a crowned humanoid.
B((0,4.45,2.3),(2.5,1.0,1.45),'ball_enamel','head',bev=.1)
B((0,4.45,3.05),(2.15,.45,.16),'ball_black','head',bev=.03)
for x in (-.72,.72):B((x,4.45,3.15),(.52,.18,.08),'glow','head',glow='eye',k=1.5)
B((0,5.05,2.35),(2.9,.18,1.8),'ball_iron','head',bev=.03)
# A caged turbine: fixed annulus and stays, animated six-blade rotor.
# An open annulus, not a solid disc: the rotor reads from front and back.
for k in range(20):
 a=k*math.tau/20
 B((math.sin(a)*2,5.9+math.cos(a)*2,-1.2),(.67,.20,.6),'ball_iron','body',rot=(0,0,-math.degrees(a)),bev=.025)
for k in range(4):
 a=k*math.pi/2
 for z in (-1.55,-.42):
  B((math.sin(a),5.9+math.cos(a),z),(.12,2.4,.12),'steel','body',rot=(0,0,-math.degrees(a)))
for k in range(6):
 a=k*math.pi/3
 B((math.sin(a)*.9,5.9+math.cos(a)*.9,-.65),(.38,1.45,.18),'ball_paint','halo',rot=(0,0,-k*60+16),bev=.035)
C((0,5.9,-.48),.42,.35,'steel','halo',rot=(90,0,0),sides=12)
C((0,5.9,-.26),.17,.12,'glow','halo',rot=(90,0,0),sides=10,glow='core',k=1.3)
for x in (-1.95,1.95):B((x,5.85,-.3),(.14,3.2,.18),'steel','body')
# Full-width grading blade, segmented cutting teeth and exposed hydraulic rams.
B((0,1.1,3.7),(7.1,1.55,.55),'ball_iron','shield',rot=(12,0,0),bev=.08)
B((0,1.75,3.85),(7.3,.18,.35),'ball_paint','shield',bev=.025)
for k in range(12):B((-3.2+k*.58,.35,4.0),(.42,.25,.62),'steel','shield',rot=(12,0,0))
B((0,.5,4.15),(6.9,.09,.09),'glow','shield',glow='edge',k=.8)
for x in (-1.65,1.65):
 B((x,1.85,2.8),(.23,.23,2.5),'steel','body',rot=(-15,0,0),bev=.02)
 C((x,1.45,3.4),.24,.8,'ball_black','shield',rot=(90,0,0),sides=10)
# Cable winch and the replaceable anchor weight hanging off the left outrigger.
B((-3.1,3.0,0),(.7,2.5,.7),'steel','armR',rot=(0,0,-20),bev=.06)
C((-3.5,3.7,0),.65,.8,'ball_iron','armR',rot=(0,0,90),sides=12)
B((-3.8,2.2,.8),(.06,1.4,.06),'ball_black','handR')
B((-3.8,1.3,.8),(1.15,1.2,1.15),'ball_iron','handR',bev=.12)
for x in (-4.15,-3.45):B((x,1.3,1.4),(.18,.7,.08),'ball_paint','handR')
# Worn service panels, exposed radiator ribs and a replaced cab corner.
for side in (-1,1):
 for k in range(7):
  B((side*2.84,2.7,-1.65+k*.35),(.07,.75,.12),'ball_black','body')
 for y,z,w in ((3.15,-1.6,.4),(2.65,.35,.65),(2.35,1.3,.3)):
  B((side*2.88,y,z),(.035,.17,w),'ball_rust','body',rot=(0,0,side*8))
 for z in (-2.2,1.8):
  B((side*2.9,3.1,z),(.08,.12,.12),'steel','body')
B((.9,4.82,3.09),(.45,.18,.04),'ball_rust','head',rot=(0,0,-10))
# A few chips on the cutting edge make the blade belong to the scraped road.
for x,y in ((-2.7,1.45),(-1.3,.85),(.4,1.55),(2.1,1.1),(3,1.6)):
 B((x,y,4.04),(.22,.10,.035),'ball_rust','shield',rot=(0,0,x*7))
# Maintenance lettering rather than ornamental runes.
for x in (-1.2,-.4,.4,1.2):B((x,2.7,-2.85),(.35,.65,.04),'ball_enamel','body')
os.makedirs(os.path.join(REPO,'build','ballast'),exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(REPO,'build','ballast','ballast.blend'))
save_enemy(NAME)
