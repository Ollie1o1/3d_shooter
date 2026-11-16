"""THE SHEPHERD: a black-ship colossus with adult, carved anatomy.

Run through headless.py. Exports seven articulated GLB pieces; also saves an
assembled, editable .blend in tools/blender/scenes/ and studio renders in
build/shepherd-art/.
The game places the same pieces on its live combat skeleton.
"""
import math
import os
from collections import defaultdict

HERE = os.path.join(os.getcwd(), "tools", "blender")
exec(open(os.path.join(HERE, "enemy_kit.py")).read())
exec(open(os.path.join(HERE, "boss_kit.py")).read())
OUT = os.path.join(REPO, "build", "shepherd-art")
os.makedirs(OUT, exist_ok=True)
_og.PALETTE.update({
    "sh_skin": (.022, .024, .029), "sh_plane": (.034, .037, .044),
    "sh_hull": (.012, .014, .018), "sh_edge": (.065, .069, .076),
    "sh_void": (.006, .009, .014), "sh_bronze": (.20, .15, .095),
    "sh_light": (.10, .26, .38),
})
SURF.update({"sh_hull": "metal", "sh_edge": "metal", "sh_bronze": "metal"})


def mesh_obj(name, verts, faces, mat, smooth=True, bevel=0):
    me=bpy.data.meshes.new(name)
    me.from_pydata([tuple(_og.to_blender(Vector(v))) for v in verts], [], faces)
    me.update()
    ob=bpy.data.objects.new(name,me);SC.collection.objects.link(ob)
    me.materials.append(palette_material(mat))
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    for p in me.polygons:p.use_smooth=smooth
    if bevel:
        mod=ob.modifiers.new("machined edge", "BEVEL");mod.width=bevel;mod.segments=2
    return ob


def gauss(x,y,cx,cy,rx,ry,h):
    return h*math.exp(-((x-cx)/rx)**2-((y-cy)/ry)**2)


def profile(rows,y):
    for i,(a,b) in enumerate(zip(rows,rows[1:])):
        if a[0]<=y<=b[0]:
            t=(y-a[0])/(b[0]-a[0]);prev=rows[max(0,i-1)];nxt=rows[min(len(rows)-1,i+2)]
            out=[]
            for k in range(1,len(a)):
                ma=(b[k]-prev[k])/(b[0]-prev[0]);mb=(nxt[k]-a[k])/(nxt[0]-a[0]);span=b[0]-a[0]
                out.append((2*t**3-3*t*t+1)*a[k]+(t**3-2*t*t+t)*ma*span+(-2*t**3+3*t*t)*b[k]+(t**3-t*t)*mb*span)
            return tuple(out)
    return rows[0][1:] if y<rows[0][0] else rows[-1][1:]


exec(open(os.path.join(HERE,'shepherd_anatomy.py')).read())


def head():
    adult_head()
    # The human facial planes stay exposed; spacecraft structure is confined
    # to the temples, occiput and mandibular rails.
    for sd in (-1,1):
        P((sd*.355,.17,-.24),(.075,.43,.30),"sh_hull",None,top=(.75,.7),bev=.015,seg=3)
        tube([(sd*.34,-.02,-.17),(sd*.36,-.26,-.04),(sd*.29,-.40,.10)],
             [.023,.025,.018],"sh_hull",None,sides=8)
        for k in range(4):
            B((sd*.395,.01+k*.08,-.11),(.012,.006,.12),"sh_edge",None,bev=.002)
    for k,x in enumerate([-.385,0,.385]):
        P((x,.48,-.32),(.085,.86+(k==1)*.22,.29),"sh_hull",None,
          rot=(-18,0,-x*19),top=(.3,.55),shift=(0,-.1),bev=.009,seg=3)
        B((x,.73,-.175),(.008,.17,.009),"sh_light",None,glow="glow",k=.12,bev=.002)


def torso():
    adult_torso()
    # Sternocleidomastoid and trapezius: tense anatomy supporting a smaller adult head.
    for sd in (-1,1):
        # Swept ship-bow armor frames the muscles instead of ballooning over them.
        P((sd*.63,.33,.0),(.34,.18,.54),"sh_hull",None,rot=(0,sd*-14,sd*10),top=(.8,.8),shift=(-sd*.04,.03),bev=.035,seg=3)
        plate((sd*.45,-.10,.29),(.55,.075,.11),"sh_hull",rot=(0,0,sd*-13),bev=.018)
        for k in range(4):
            # Rib plates are overlapping hull strata, exposing dark blue muscle between them.
            y=-.27-k*.19;x=sd*(.45-k*.045)
            P((x,y,.11),(.15,.09,.29),"sh_hull",None,rot=(0,sd*-20,sd*-12),top=(.7,.85),bev=.016,seg=2)
        # Dense capsule/docking rows make the body read as a city-scale vessel.
        for k in range(7):
            x=sd*(.26+.073*k)
            B((x,.43,.16),(.043,.075,.095),"sh_edge",None,bev=.009,seg=2)
            B((x,.435,.212),(.017,.018,.006),"sh_light",None,glow="glow",k=.22)
    # A closed collar bridges the cropped neck and torso without torn edges.
    E((0,.80,.06),(.61,.23,.52),"sh_hull",None,seg=40,rings=16)
    R((0,.87,.06),.29,.014,"sh_edge",None,rot=(90,0,0),sides=48,tsides=8)
    # Narrow throat tendon column, with a black split collar seated below the jaw.
    for y in (.54,.65,.76):
        B((0,y,-.18),(.39,.035,.12),"sh_hull",None,bev=.01,seg=2)
    # Empty sternum, a dark gravitational aperture rather than a glowing breastplate.
    E((0,.03,.25),(.28,.43,.17),"sh_void",None,seg=32,rings=20)
    R((0,.03,.31),.19,.017,"sh_edge",None,rot=(0,0,0),sides=48,tsides=8)
    for k in range(6):
        a=2*math.pi*k/6
        B((math.cos(a)*.20,.03+math.sin(a)*.20,.31),(.018,.08,.04),"sh_bronze",None,rot=(0,0,math.degrees(a)),bev=.006)
    # The rear carries the Shepherd's original long, comb-like ship tines.
    for sd in (-1,1):
        for i in range(3):
            P((sd*(.48+i*.13),.25-i*.09,-.45-i*.04),(.075,.70-i*.1,.45),"sh_hull",None,
              rot=(-22,sd*16,sd*-16),top=(.35,.45),shift=(sd*.07,-.12),bev=.013,seg=2)
    P((0,-.92,-.08),(.68,.44,.51),"sh_hull",None,top=(.84,.86),bev=.03,seg=3)


def shoulder():
    E((0,0,0),(.92,.98,.91),"sh_skin",None,seg=32,rings=22)
    for k in range(3):
        P((0,.26-k*.19,-.18-k*.025),(.98-k*.06,.16,.65),"sh_hull",None,
          rot=(-8,0,0),top=(.88,.85),shift=(0,-.02),bev=.025,seg=3)
    rod((-.32,-.15,.34),(.31,-.15,.34),.016,"sh_edge",sides=8)


def limb(forearm=False):
    adult_limb(forearm)
    if forearm:
        P((0,.26,-.02),(.65,.18,.78),"sh_hull",None,rot=(0,0,0),top=(.72,.90),shift=(0,-.08),bev=.025,seg=3)
        for sd in (-1,1):rod((sd*.22,.37,-.32),(sd*.14,.29,.33),.018,"sh_edge",sides=8)
        B((0,.37,-.08),(.018,.012,.45),"sh_light",None,glow="glow",k=.25,bev=.004)
    else:
        tube([(-.20,.24,-.34),(-.24,.34,-.10),(-.14,.23,.39)],[.025,.028,.018],"sh_plane",None,sides=10)
        P((0,-.20,-.15),(.46,.18,.51),"sh_hull",None,top=(.8,.8),bev=.023,seg=3)
    for z in (-.46,.46):R((0,0,z),.23,.024,"sh_edge",None,rot=(0,0,0),sides=24,tsides=6)


def palm():
    P((0,0,0),(.96,.88,1.0),"sh_skin",None,top=(.80,.95),bev=.11,seg=4)
    # Four distinct metacarpals, dorsal hull shields, and an inset magnetic target.
    for k in range(4):
        x=-.34+k*.225
        B((x,.40,-.04),(.17,.13,.77),"sh_hull",None,rot=(0,(k-1.5)*-3,0),bev=.045,seg=3)
        B((x,.48,-.07),(.025,.015,.37),"sh_light",None,glow="glow",k=.35,bev=.006,seg=2)
    R((0,.49,-.12),.17,.025,"sh_edge",None,rot=(90,0,0),sides=32,tsides=8)


def finger():
    # A tapered phalanx and angular knuckle cover, never a row of sausages.
    P((0,0,0),(.78,.78,1.0),"sh_skin",None,top=(.78,.90),bev=.16,seg=4)
    P((0,.28,-.08),(.65,.18,.66),"sh_hull",None,top=(.8,.9),bev=.07,seg=3)
    B((0,.38,-.10),(.035,.012,.29),"sh_light",None,glow="glow",k=.25,bev=.008,seg=2)


def consolidate():
    # Merge by material AND animation channel: a handful of GPU meshes per piece.
    groups=defaultdict(list)
    for ob in list(SC.objects):
        if ob.type!='MESH':continue
        for mod in list(ob.modifiers):
            bpy.context.view_layer.objects.active=ob
            bpy.ops.object.modifier_apply(modifier=mod.name)
        key=(ob.data.materials[0].name,ob.get('glow',''),str(ob.get('k','')),ob.get('tex',''))
        groups[key].append(ob)
    for group in groups.values():
        bpy.ops.object.select_all(action='DESELECT')
        for ob in group:ob.select_set(True)
        bpy.context.view_layer.objects.active=group[0]
        bpy.ops.object.join()


for name,build in [('head',head),('torso',torso),('shoulder',shoulder),('upperarm',lambda:limb(False)),
                   ('forearm',lambda:limb(True)),('palm',palm),('finger',finger)]:
    begin();build();consolidate();save_piece('shepherd_'+name)

# Assemble the same pieces at the game's proportions for editing and studio QA.
begin()

def place(name,at=(0,0,0),scale=(1,1,1),rot=(0,0,0)):
    before=set(SC.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(REPO,'assets','props','shepherd_'+name+'.glb'))
    objs=[o for o in SC.objects if o not in before]
    root=bpy.data.objects.new('SHEPHERD_'+name,None);SC.collection.objects.link(root)
    for ob in objs:
        if ob.parent is None:ob.parent=root
    root.matrix_world=_og.game_to_blender(Matrix.Translation(Vector(at))@Euler(rot,'XYZ').to_matrix().to_4x4()@Matrix.Diagonal((*scale,1)))
    return root

place('torso')
place('head',(0,93/70,18/70),scale=(64/70,)*3,rot=(.18,0,0))

def elbow_position(sh,hand,sd):
    delta=hand-sh;dist=delta.length;direction=delta.normalized()
    stretch=max(1.0,dist/(167/70)*1.001);a,b=85/70*stretch,82/70*stretch
    along=(a*a-b*b+dist*dist)/(2*dist);height=max(0,a*a-along*along)**.5
    pole=Vector((sd*.8,-.7,-.15));pole=(pole-direction*pole.dot(direction)).normalized()
    return sh+direction*along+pole*height

for sd in (-1,1):
    sh=Vector((sd*61/70,30/70,0));hand=Vector((sd*1.20,-1.64,.27))
    elbow=elbow_position(sh,hand,sd)
    place('shoulder',sh,scale=(.47,.47,.47))
    for name,a,b,w,d in [('upperarm',sh,elbow,.36,.38),('forearm',elbow,hand,.27,.29)]:
        vec=b-a;rot=Vector((0,0,1)).rotation_difference(vec.normalized()).to_euler()
        place(name,(a+b)*.5,scale=(w,d,vec.length),rot=rot)
    place('shoulder',elbow,scale=(.32,.32,.32))
    place('palm',hand,scale=(.233,.071,.233),rot=(math.pi/2,0,0))
    for k in range(4):
        at=hand+Vector(((-4.5+3*k)/70*1.25,-.15,0));place('finger',at,scale=(.043,.045,.125),rot=(math.pi/2,0,0))
        place('finger',at+Vector((0,-.104,0)),scale=(.036,.036,.099),rot=(math.pi/2,0,0))
    place('finger',hand+Vector((sd*.137,-.031,0)),scale=(.05,.05,.163),rot=(math.pi/2,0,sd*.8))

# Studio materials mirror the game's carbon skin and scored spacecraft hull.
# Object coordinates keep the micro relief attached to the articulated pieces.
for mat in bpy.data.materials:
    if not mat.use_nodes:continue
    nodes,links=mat.node_tree.nodes,mat.node_tree.links
    bs=nodes.get('Principled BSDF')
    if not bs:continue
    skin=mat.name.startswith(('sh_skin','sh_plane','sh_void'))
    hull=mat.name.startswith(('sh_hull','sh_edge','sh_bronze'))
    if not (skin or hull):continue
    bs.inputs['Metallic'].default_value=.15 if skin else .68
    bs.inputs['Roughness'].default_value=.68 if skin else .48
    coord=nodes.new('ShaderNodeTexCoord')
    noise=nodes.new('ShaderNodeTexNoise');noise.name='Carbon mineral grain' if skin else 'Hull scoring'
    noise.inputs['Scale'].default_value=38 if skin else 65
    noise.inputs['Detail'].default_value=4
    links.new(coord.outputs['Generated'],noise.inputs['Vector'])
    ramp=nodes.new('ShaderNodeValToRGB');ramp.name='Near black with buried blue undertones'
    original=tuple(bs.inputs['Base Color'].default_value)
    ramp.color_ramp.elements[0].position=.2
    ramp.color_ramp.elements[0].color=(original[0]*.48,original[1]*.48,original[2]*.5,1)
    ramp.color_ramp.elements[1].position=.8
    ramp.color_ramp.elements[1].color=(original[0]*1.45,original[1]*1.45,original[2]*1.6,1)
    links.new(noise.outputs['Fac'],ramp.inputs['Fac']);links.new(ramp.outputs['Color'],bs.inputs['Base Color'])
    bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.32 if skin else .22
    bump.inputs['Distance'].default_value=.007 if skin else .004
    links.new(noise.outputs['Fac'],bump.inputs['Height']);links.new(bump.outputs['Normal'],bs.inputs['Normal'])
    rough=nodes.new('ShaderNodeMapRange');rough.inputs['To Min'].default_value=.48
    rough.inputs['To Max'].default_value=.82 if skin else .62
    links.new(noise.outputs['Fac'],rough.inputs['Value']);links.new(rough.outputs['Result'],bs.inputs['Roughness'])

sc=SC;sc.render.engine='CYCLES';sc.cycles.samples=32
sc.render.resolution_x=1400;sc.render.resolution_y=1400;sc.render.resolution_percentage=100
sc.render.image_settings.file_format='PNG';sc.world.use_nodes=True
sc.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.005,.009,.016,1)
sc.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.3
sc.view_settings.view_transform='AgX'

def area(name,at,power,color,size):
    ob=bpy.data.objects.new(name,bpy.data.lights.new(name,'AREA'));sc.collection.objects.link(ob)
    ob.location=_og.to_blender(Vector(at));ob.rotation_euler=(Vector((0,0,.65))-ob.location).to_track_quat('-Z','Y').to_euler()
    ob.data.energy=power;ob.data.color=color;ob.data.shape='DISK';ob.data.size=size
area('neutral key',(-3,4,4),650,(.91,.94,1),3)
area('warm rim',(3,3,-2),650,(1,.5,.22),3)
area('face fill',(0,1,5),95,(.88,.91,1),3)
cam=bpy.data.objects.new('art camera',bpy.data.cameras.new('art camera'));sc.collection.objects.link(cam);sc.camera=cam
cam.data.lens=65

def shot(name,at,target,lens=65):
    cam.data.lens=lens
    cam.location=_og.to_blender(Vector(at));target=_og.to_blender(Vector(target))
    cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
    sc.render.filepath=os.path.join(OUT,name+'.png');bpy.ops.render.render(write_still=True)

shot('shepherd-bust',(3.4,1.0,9.2),(0,.12,0),55)
shot('shepherd-front',(0,.7,10),(0,.12,0),55)
shot('shepherd-face',(.6,1.53,2.7),(0,1.35,.28))
cam.data.lens=55
cam.location=_og.to_blender(Vector((3.4,1.0,9.2)))
cam.rotation_euler=(_og.to_blender(Vector((0,.12,0)))-cam.location).to_track_quat('-Z','Y').to_euler()
source_dir=os.path.join(REPO,'tools','blender','scenes')
os.makedirs(source_dir,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(source_dir,'shepherd.blend'))
print('Shepherd GLBs, editable Blender assembly, and studio renders complete.')
