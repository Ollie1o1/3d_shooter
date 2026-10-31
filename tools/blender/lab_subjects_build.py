# Original flesh chassis, with a shared articulated anatomy and distinct support equipment.
# Run with blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/lab_subjects_build.py
import os, math
exec(open(os.path.join(os.getcwd(), 'tools/blender/enemy_kit.py')).read())
_og.PALETTE.update({'lab_skin':(.73,.5,.45),'lab_tendon':(.51,.3,.28),'lab_vein':(.38,.19,.22),
 'lab_bone':(.79,.76,.65),'lab_frame':(.14,.24,.27),'lab_face':(.82,.84,.78),'lab_inset':(.035,.075,.09),'lab_band':(.35,.55,.57)})
J='waist torso head hipL hipR kneeL kneeR ankleL ankleR armL armR elbowL elbowR handL handR fingersL fingersR thumbL thumbR'
for name,height,width in [('patient',2.2,1.),('grafted',3.2,1.5),('suspended',2.3,.88),('phased',2.4,.94),('successor',2.65,1.)]:
 sc=bpy.context.scene
 for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
 sc.name=name.upper();sc['overdrive_opened']=name;NEEDED[name]=J
 sy=height/2.65
 def p(v):return (v[0]*width,v[1]*sy,v[2]*width)
 def sz(v):return p(v)
 def joint(n,v,parent=None):add_joint(n,p(v),parent)
 def oval(v,s,mat,parent,**kw):return E(p(v),sz(s),mat,parent,seg=16 if name=='successor' else 12,rings=10 if name=='successor' else 8,**kw)
 def box(v,s,mat,parent,**kw):return B(p(v),sz(s),mat,parent,**kw)
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
  # Openable forearm incision and a subtle chest manifold, not armor or a halo.
  for side,sign in [('L',1),('R',-1)]:
   box((sign*.44,1.43,.15),(.045,.37,.045),'lab_bone','elbow'+side,bev=.01)
   for k in range(3):box((sign*(.41+k*.026),1.39,.17),(.01,.29,.014),'lab_vein','elbow'+side)
  oval((0,1.9,.235),(.13,.2,.035),'glow','torso',glow='core',k=.4)
  box((0,2.21,-.14),(.09,.12,.04),'lab_face','torso')
  for k in range(3):box((-.065+k*.065,2.2,-.165),(.025,.025,.02),'glow','torso',glow='eye',k=.5)
 save_enemy(name)
 os.makedirs(os.path.join(REPO,'build','lab-models'),exist_ok=True)
 bpy.ops.wm.save_as_mainfile(filepath=os.path.join(REPO,'build','lab-models',name+'.blend'))
