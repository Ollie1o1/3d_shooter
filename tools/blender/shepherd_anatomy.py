"""Anatomical foundation for the Shepherd, adapted from pinned CC0 MakeHuman data.
Only mesh and morph data are used. See reference/makehuman/PROVENANCE.md.
Executed by shepherd_build.py after the Blender mesh helpers are defined.
"""
REF=os.path.join(HERE,'reference','makehuman')
_MV=[];_MF={};_group='body'
for line in open(os.path.join(REF,'base.obj')):
    words=line.split()
    if not words:continue
    if words[0]=='v':_MV.append(Vector(tuple(map(float,words[1:4]))))
    elif words[0]=='g':_group=words[1]
    elif words[0]=='f':_MF.setdefault(_group,[]).append(tuple(int(w.split('/')[0])-1 for w in words[1:]))
MORPHS=[('caucasian-male-young.target',1.0),
        ('universal-male-young-maxmuscle-averageweight.target',.72),
        ('head-fat-decr.target',.32),('head-rectangular.target',.14),
        ('chin-prominent-incr.target',.18),('chin-width-incr.target',.12),
        ('nose-greek-incr.target',.12)]
for filename,weight in MORPHS:
    for line in open(os.path.join(REF,filename)):
        words=line.split()
        if len(words)==4 and words[0].isdigit():_MV[int(words[0])]+=Vector(tuple(map(float,words[1:])))*weight


def anatomical_patch(name,accept,transform,subdivide=1,clips=()):
    faces=[f for f in _MF['body'] if accept([_MV[i] for i in f])]
    used=sorted({i for f in faces for i in f});mapping={old:new for new,old in enumerate(used)}
    ob=mesh_obj(name,[tuple(transform(_MV[i])) for i in used],[tuple(mapping[i] for i in f) for f in faces],'sh_skin')
    if clips:
        bm=bmesh.new();bm.from_mesh(ob.data)
        for axis,limit,keep_positive in clips:
            # Convert a game-space clipping plane to Blender space.
            normal=[0.,0.,0.];normal[axis]=1.
            co=[0.,0.,0.];co[axis]=limit
            plane_co=_og.to_blender(Vector(co));plane_no=_og.to_blender(Vector(normal))
            bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                plane_co=plane_co,plane_no=plane_no,clear_inner=keep_positive,clear_outer=not keep_positive,dist=1e-6)
            border=[e for e in bm.edges if e.is_boundary and all(abs((v.co-plane_co).dot(plane_no))<1e-5 for v in e.verts)]
            if border:bmesh.ops.holes_fill(bm,edges=border,sides=0)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free()
    if subdivide:
        mod=ob.modifiers.new('anatomical surface subdivision','SUBSURF');mod.levels=subdivide;mod.render_levels=subdivide
    return ob


def anatomical_joint(name):
    ids={i for f in _MF['joint-'+name] for i in f}
    return sum((_MV[i] for i in ids),Vector())/len(ids)

HEAD_ORIGIN=Vector((0,8.10,.47))

def head_point(point):return (Vector(point)-HEAD_ORIGIN)*.43


def adult_head():
    anatomical_patch('adult craniofacial topology',lambda ps:all(p.y>6.40 for p in ps),head_point,clips=((1,(6.90-8.1)*.43,True),))
    # Real eyelid openings reveal spherical, pupil-less eyes. No slit decals,
    # added mouth line, cheek lumps, separate nose wedge, or painted expression.
    for sd in ('l','r'):
        eye=head_point(anatomical_joint(sd+'-eye'))
        E(eye,(.118,.118,.118),'sh_light',None,seg=32,rings=20,glow='glow',k=1.9)


def adult_torso():
    # Crop the A-pose mesh at shoulder sockets and below the pelvis. The actual
    # lat, clavicle, pectoral and abdominal forms are retained in the surface.
    def accept(ps):
        return all(.40<p.y<7.40 and abs(p.x)<(2.2 if p.y>4.6 else 1.8) for p in ps)
    def transform(p):return Vector((p.x*.365,(p.y-4.70)*.345,p.z*.34))
    anatomical_patch('athletic clavicle pectoral lat abdominal topology',accept,transform,
        clips=((1,(6.95-4.70)*.345,False),(1,(.70-4.70)*.345,True),(0,.68,False),(0,-.68,True)))


def adult_limb(forearm=False):
    a=anatomical_joint('l-elbow' if forearm else 'l-shoulder')
    b=anatomical_joint('l-hand' if forearm else 'l-elbow')
    z=(b-a).normalized();length=(b-a).length;center=(a+b)*.5
    x=Vector((0,0,1)).cross(z).normalized();y=z.cross(x).normalized()
    def accept(ps):
        if any(p.x<1.6 for p in ps):return False
        ts=[(p-a).dot(z)/length for p in ps]
        return min(ts)>-.025 and max(ts)<1.025 and all(((p-a)-z*(p-a).dot(z)).length<.85 for p in ps)
    def transform(p):
        q=p-center
        return Vector((q.dot(x)/1.20,q.dot(y)/1.20,q.dot(z)/length))
    anatomical_patch('anatomical forearm' if forearm else 'anatomical upper arm',accept,transform,clips=((2,-.49,True),(2,.49,False)))
