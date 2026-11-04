# Original flesh chassis, with a shared articulated anatomy and distinct support equipment.
# Run with blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/lab_subjects_build.py
import os, math
exec(open(os.path.join(os.getcwd(), 'tools/blender/enemy_kit.py')).read())
exec(open(os.path.join(os.getcwd(), 'tools/blender/lab_ward_subjects.py')).read())
_og.PALETTE.update({'lab_skin':(.73,.5,.45),'lab_tendon':(.51,.3,.28),'lab_vein':(.38,.19,.22),
 'lab_bone':(.79,.76,.65),'lab_frame':(.14,.24,.27),'lab_face':(.82,.84,.78),'lab_inset':(.035,.075,.09),'lab_band':(.35,.55,.57)})
J='waist torso head hipL hipR kneeL kneeR ankleL ankleR armL armR elbowL elbowR handL handR fingersL fingersR thumbL thumbR'
_og.PALETTE.update({'successor_skin':(.48,.46,.47),'successor_black':(.045,.025,.035),
 'successor_plate':(.16,.12,.15),'successor_red':(.55,.035,.055),'successor_ivory':(.79,.78,.72)})
for name,height,width in [('patient',2.2,1.),('grafted',3.2,1.5),('suspended',2.3,.88),('phased',2.4,.94),('successor',3.8,1.25)]:
 if globals().get('ONLY') and name!=ONLY:continue
 sc=bpy.context.scene
 for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
 sc.name=name.upper();sc['overdrive_opened']=name;NEEDED[name]=J
 sy=height/2.65
 def p(v):return (v[0]*width,v[1]*sy,v[2]*width)
 def sz(v):return p(v)
 def joint(n,v,parent=None):add_joint(n,p(v),parent)
 def oval(v,s,mat,parent,**kw):return E(p(v),sz(s),'successor_skin' if name=='successor' and mat=='lab_skin' else mat,parent,seg=16 if name=='successor' else 12,rings=10 if name=='successor' else 8,**kw)
 def box(v,s,mat,parent,**kw):return B(p(v),sz(s),mat,parent,**kw)
 if name!='successor':
  # The ward's subjects have their own builder (lab_ward_subjects.py)
  build_ward_subject(name,width,sy);NEEDED[name]=J+' jaw clothF clothB ribbonL ribbonR';save_enemy(name)
  os.makedirs(os.path.join(REPO,'build','lab-models'),exist_ok=True)
  bpy.ops.wm.save_as_mainfile(filepath=os.path.join(REPO,'build','lab-models',name+'.blend'))
  continue
 joint('waist',(0,1.17,0));joint('torso',(0,1.53,0),'waist');joint('head',(0,2.43,0),'torso')
 for side,sign in [('L',1),('R',-1)]:
  joint('hip'+side,(sign*.18,1.1,0),'waist');joint('knee'+side,(sign*.18,.59,.015),'hip'+side);joint('ankle'+side,(sign*.18,.13,.015),'knee'+side)
  joint('arm'+side,(sign*.39,2.1,0),'torso');joint('elbow'+side,(sign*.44,1.65,.015),'arm'+side);joint('hand'+side,(sign*.44,1.16,.055),'elbow'+side)
  joint('fingers'+side,(sign*.44,1.06,.08),'hand'+side);joint('thumb'+side,(sign*.36,1.16,.11),'hand'+side)
  oval((sign*.18,.86,0),(.29,.59,.32),'lab_skin','hip'+side)
  oval((sign*.18,.36,-.025),(.23,.52,.26),'lab_skin','knee'+side)
  oval((sign*.18,.12,.09),(.26,.24,.47),'lab_skin','ankle'+side)
  oval((sign*.18,.59,.14),(.16,.16,.09),'lab_bone','knee'+side)
  oval((sign*.39,2.05,0),(.35,.39,.39),'lab_skin','arm'+side)
  oval((sign*.43,1.87,0),(.26,.45,.29),'lab_skin','arm'+side)
  oval((sign*.44,1.43,.025),(.19,.48,.23),'lab_skin','elbow'+side)
  oval((sign*.44,1.15,.06),(.18,.22,.12),'lab_skin','hand'+side)
  for f in range(4):
   x=sign*(.38+f*.038)
   oval((x,1.02,.07),(.033,.17,.042),'lab_skin','fingers'+side)
   oval((x,.94,.075),(.03,.07,.037),'lab_skin','fingers'+side)
  oval((sign*.33,1.09,.11),(.047,.14,.055),'lab_skin','thumb'+side,rot=(0,0,sign*-24))
  box((sign*.18,.61,-.135),(.11,.08,.08),'lab_frame','knee'+side)
  box((sign*.43,1.64,-.1),(.11,.08,.08),'lab_frame','elbow'+side)
  if name!='successor':
   box((sign*.18,.88,.17),(.34,.08,.06),'lab_band','hip'+side)
   box((sign*.44,1.44,.15),(.23,.075,.05),'lab_band','elbow'+side)
 oval((0,1.18,0),(.6,.36,.38),'lab_skin','waist')
 oval((0,1.53,0),(.52,.5,.34),'lab_skin','torso')
 oval((0,1.88,-.015),(.78,.59,.46),'lab_skin','torso')
 oval((0,2.22,0),(.2,.25,.22),'lab_tendon','torso')
 for x in [-.17,.17]:
  oval((x,1.96,.205),(.31,.2,.065),'lab_skin','torso')
  for k in range(3):oval((x*.65,1.66-k*.105,.17),(.17,.095,.045),'lab_tendon','torso')
 # A mechanical face: paired cyan apertures, rigid brow, side seams, no human mouth.
 oval((0,2.44,0),(.36,.4,.34),'lab_frame','head')
 box((0,2.47,.165),(.32,.31,.055),'lab_face','head',bev=.025,seg=2)
 box((0,2.54,.202),(.3,.035,.018),'lab_inset','head')
 for sign in [-1,1]:
  box((sign*.075,2.49,.202),(.055,.045,.02),'glow','head',glow='eye',k=1.2)
  box((sign*.15,2.36,.19),(.022,.08,.022),'lab_frame','head')
  box((sign*.12,2.22,.04),(.045,.14,.045),'lab_frame','torso')
 for k in range(7):box((0,1.31+k*.13,-.24),(.06,.055,.035),'lab_frame','torso')
 # Subtle tissue seams and surgical identification, without replacing the readable anatomy.
 for k in range(4):box((-.23+k*.15,2.12,.18),(.07,.012,.012),'lab_vein','torso',rot=(0,0,-12))
 if name=='patient':
  for sign,side in [(1,'L'),(-1,'R')]:
   box((sign*.27,.77,-.1),(.07,.78,.11),'lab_face','hip'+side,bev=.012)
   box((sign*.5,1.64,-.075),(.075,.7,.11),'lab_face','arm'+side,bev=.012)
  box((0,1.9,-.3),(.5,.44,.14),'lab_frame','torso')
 elif name=='grafted':
  box((0,1.82,.38),(.96,.86,.18),'lab_face','torso',bev=.035,seg=2)
  for k in range(4):box((-.35+k*.23,1.82,.485),(.055,.67,.045),'lab_frame','torso')
  oval((0,1.75,-.4),(.6,.72,.4),'lab_tendon','torso')
  oval((0,1.82,-.6),(.25,.38,.04),'glow','torso',glow='core',k=.8)
  for sign in [-1,1]:box((sign*.37,1.76,-.43),(.09,.88,.1),'lab_frame','torso')
 elif name=='suspended':
  for sign,side in [(1,'L'),(-1,'R')]:
   box((sign*.64,1.65,0),(.08,1.7,.11),'lab_face','arm'+side)
   box((sign*.56,2.38,-.16),(.62,.08,.12),'lab_face','torso',rot=(0,0,sign*25))
  box((0,1.75,-.32),(.18,1.5,.16),'lab_frame','torso')
  oval((0,2.07,.25),(.28,.32,.11),'glow','torso',glow='core',k=1.1)
 elif name=='phased':
  for sign,side in [(1,'L'),(-1,'R')]:
   for k in range(4):box((sign*(.28+k*.09),1.6+k*.17,-.26),(.12,.04,.1),'lab_frame','torso',rot=(0,0,sign*-25))
  for k in range(4):box((0,1.65+k*.13,.19),(.06,.025,.025),'glow','torso',glow='core',k=.7)
 else:
  # The completed chassis: a taller athletic frame, exposed drive spine,
  # asymmetric black augments and branching red power channels on the arms.
  for k in range(10):
   y=1.28+k*.105
   box((0,y,-.31),(.18,.075,.11),'successor_black','torso',bev=.014)
   oval((0,y,-.39),(.105,.07,.07),'glow','torso',glow='core',k=.9+k*.06)
   for sign in [-1,1]:
    box((sign*.12,y,-.34),(.13,.035,.045),'successor_ivory','torso',rot=(0,0,sign*20))
  # Three dorsal vanes break the silhouette without copying a halo/crown.
  for k in range(3):
   y=1.65+k*.28
   box((0,y,-.53-k*.055),(.105,.43,.38),'successor_black','torso',rot=(20,0,0),bev=.02,seg=2)
   box((0,y+.05,-.75-k*.055),(.045,.29,.035),'glow','torso',glow='core',k=1.4)
  for side,sign in [('L',1),('R',-1)]:
   # Left shoulder is a broad drive housing; right is a lean cutting frame.
   box((sign*.43,2.09,-.025),(.38 if sign>0 else .28,.28,.42),'successor_black','arm'+side,rot=(0,0,sign*-12),bev=.04,seg=2)
   box((sign*.55,2.15,-.12),(.1,.34,.32),'successor_ivory','arm'+side,rot=(0,0,sign*-20),bev=.02)
   oval((sign*.42,1.87,-.08),(.29,.43,.32),'successor_plate','arm'+side)
   oval((sign*.44,1.43,-.015),(.26,.49,.3),'successor_black','elbow'+side)
   box((sign*.44,1.23,.15),(.28,.18,.18),'successor_ivory','elbow'+side,bev=.025,seg=2)
   for k in range(6):
    y=1.24+k*.125;parent=('elbow' if y<1.64 else 'arm')+side
    x=sign*(.43+(.035 if k%2 else -.015))
    box((x,y,.16),(.12,.13,.035),'successor_red',parent,rot=(0,0,sign*(32 if k%2 else -24)))
    box((x+sign*.025,y,.183),(.035,.145,.025),'glow',parent,rot=(0,0,sign*(32 if k%2 else -24)),glow='tellL' if sign>0 else 'tellR',k=.9)
    if k%2==0:box((x-sign*.07,y+.04,.17),(.13,.025,.025),'glow',parent,rot=(0,0,sign*-30),glow='seam',k=.65)
   # Finger/ankle reinforcement keeps the augments attached through full poses.
   box((sign*.44,1.12,.09),(.22,.13,.16),'successor_black','hand'+side,bev=.02)
   for k in range(3):box((sign*(.39+k*.045),1.09,.19),(.023,.06,.025),'glow','fingers'+side,glow='tellL' if sign>0 else 'tellR',k=.7)
   box((sign*.18,.32,-.13),(.17,.43,.08),'successor_black','knee'+side,bev=.02)
   box((sign*.18,.6,.13),(.23,.18,.09),'successor_ivory','knee'+side,bev=.02)
   box((sign*.23,.37,-.18),(.03,.33,.03),'glow','knee'+side,glow='seam',k=.55)
  # Split black face and narrow red visor replace the ward's monitor-like mask.
  box((0,2.47,.205),(.36,.34,.065),'successor_black','head',bev=.025,seg=2)
  for sign in [-1,1]:
   box((sign*.11,2.43,.251),(.075,.29,.035),'successor_ivory','head',rot=(0,0,sign*-14),bev=.008)
   box((sign*.075,2.52,.253),(.095,.035,.025),'glow','head',glow='eye',k=1.25)
   box((sign*.15,2.33,.09),(.07,.18,.17),'successor_black','head',rot=(0,0,sign*14),bev=.015)
  oval((0,1.93,.25),(.19,.26,.055),'successor_black','torso')
  oval((0,1.93,.29),(.10,.19,.035),'glow','torso',glow='core',k=1.1)
  for sign in [-1,1]:
   box((sign*.29,1.87,.24),(.06,.48,.08),'successor_black','torso',rot=(0,0,sign*-14))
   box((sign*.29,1.87,.287),(.022,.41,.02),'glow','torso',glow='seam',k=.7)
 save_enemy(name)
 os.makedirs(os.path.join(REPO,'build','lab-models'),exist_ok=True)
 bpy.ops.wm.save_as_mainfile(filepath=os.path.join(REPO,'build','lab-models',name+'.blend'))
