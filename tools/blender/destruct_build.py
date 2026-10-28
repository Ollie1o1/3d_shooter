# Pieces of the level that break (game/Gameplay_Destruct.h): each made whole, then cut up in Blender.
#
#   blender -b --factory-startup --python tools/blender/destruct_build.py [-- NAME]
#
# For each, into assets/props/: <name>.glb (whole: drawn until it breaks) and <name>_pieces.glb (the same
# shape cut into chunks: each one closed, one colour, its node at its middle - the game throws each one
# from where it sat). Game space: +Y up, metres, the base at y 0; the game stretches it to the box a
# layout gives it (LevelLayout.h: kind "destructible", name).
#
# The cutting: every part is split by planes through it (random, but the same every build: SEED), the
# biggest chunk first, until it's in as many as asked; each cut capped, so a chunk is a closed convex
# shape. Parts are cut on their own (a plinth, a shaft), so chunks break along the parts' joins too.
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SEED = 7
PALETTE = {"adobe": (0.66, 0.40, 0.30), "adobedark": (0.50, 0.28, 0.22), "sandstone": (0.72, 0.58, 0.42),
           "wood": (0.47, 0.31, 0.21), "plank": (0.36, 0.23, 0.15), "iron": (0.26, 0.24, 0.25),
           "slate": (0.30, 0.31, 0.35), "basalt": (0.17, 0.15, 0.16)}
TEX = {"adobe": "adobe", "adobedark": "adobe", "sandstone": "limestone", "wood": "wood", "plank": "wood",
       "iron": "metal", "slate": "panel", "basalt": "basalt"}


def to_b(x, y, z):   # game -> Blender
    return Vector((x, -z, y))


def material(name):
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name)
    base = name.split("*")[0]
    shade = float(name.split("*")[1]) if "*" in name else 1.0
    r, g, b = PALETTE.get(base, (0.5, 0.5, 0.5))
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    col = (min(1, r * shade), min(1, g * shade), min(1, b * shade), 1.0)
    bsdf.inputs["Base Color"].default_value = col
    bsdf.inputs["Roughness"].default_value = 1.0
    m.diffuse_color = col
    return m


# ---- parts (game coordinates: centre x, y, z; sizes) ----------------------------------------------
def box(c, size):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector((size[0], size[2], size[1])), verts=bm.verts)
    bmesh.ops.translate(bm, vec=to_b(*c), verts=bm.verts)
    return bm


def prism(c, r, h, sides=8, r_top=None):
    """An upright prism (r at its foot, r_top at its head), centred on c"""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=sides, radius1=r, radius2=r_top or r, depth=h)
    bmesh.ops.rotate(bm, cent=Vector(), matrix=Matrix.Rotation(math.pi / sides, 3, "Z"), verts=bm.verts)
    bmesh.ops.translate(bm, vec=to_b(*c), verts=bm.verts)
    return bm


# ---- the cutting --------------------------------------------------------------------------------
def volume(bm):
    try:
        return abs(bm.calc_volume())
    except Exception:
        return 0.0


def split(bm, co, no):
    """bm cut in two by the plane (co, no): both halves, each capped; None if the plane misses it"""
    halves = []
    for keep_outer in (False, True):
        h = bm.copy()
        geom = h.verts[:] + h.edges[:] + h.faces[:]
        res = bmesh.ops.bisect_plane(h, geom=geom, plane_co=co, plane_no=no, clear_inner=keep_outer, clear_outer=not keep_outer)
        cut = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
        if cut:
            bmesh.ops.holes_fill(h, edges=cut, sides=0)
        bmesh.ops.recalc_face_normals(h, faces=h.faces)
        if len(h.faces) < 4 or volume(h) < 1e-4:
            h.free()
            for o in halves:
                o.free()
            return None
        halves.append(h)
    return halves


def fracture(bm, n, rng, flat=0.5):
    """bm in n chunks: the biggest split by a random plane near its middle until there are n.
    flat (0..1): how much the cuts lean to lying flat (stacked slabs) rather than standing (wedges)"""
    pieces = [bm]
    tries = 0
    while len(pieces) < n and tries < n * 30:
        tries += 1
        pieces.sort(key=volume)
        big = pieces[-1]
        vs = [v.co for v in big.verts]
        lo = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
        hi = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
        c = (lo + hi) * 0.5
        span = hi - lo
        co = c + Vector((rng.uniform(-0.2, 0.2) * span.x, rng.uniform(-0.2, 0.2) * span.y, rng.uniform(-0.2, 0.2) * span.z))
        a = rng.uniform(0, 2 * math.pi)
        tilt = rng.uniform(0.25, 1.0) if rng.random() < flat else rng.uniform(0.0, 0.3)   # (Blender Z is up)
        no = Vector((math.cos(a) * (1 - tilt), math.sin(a) * (1 - tilt), tilt)).normalized()
        if span.z > 1.6 * max(span.x, span.y):   # tall: cut across it more often
            no = Vector((no.x * 0.4, no.y * 0.4, 1.0)).normalized()
        halves = split(big, co, no)
        if halves is None:
            continue
        pieces.pop()
        big.free()
        pieces += halves
    return pieces


# ---- out ------------------------------------------------------------------------------------------
def clear():
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    for me in list(bpy.data.meshes):
        bpy.data.meshes.remove(me)


def add(bm, name, mat):
    """bm as an object, its origin at its middle (the game throws it from there)"""
    vs = [v.co for v in bm.verts]
    c = sum(vs, Vector()) / len(vs)
    bmesh.ops.translate(bm, vec=-c, verts=bm.verts)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(material(mat))
    ob = bpy.data.objects.new(name, me)
    ob.location = c
    bpy.context.scene.collection.objects.link(ob)
    base = mat.split("*")[0]
    if base in TEX:
        ob["tex"] = TEX[base]
    return ob


def export(name):
    path = os.path.join(REPO, "assets", "props", name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", export_extras=True, export_yup=True,
                              export_apply=True, use_selection=False, use_active_scene=True, export_texcoords=False,
                              export_normals=True)
    print("destruct_build: %s (%d parts)" % (path, len(bpy.context.scene.objects)))


def build(name, parts):
    """parts: [(make() -> bmesh, material, chunks, flat)]: the whole, then the whole cut up"""
    clear()
    for i, (make, mat, _, _) in enumerate(parts):
        add(make(), "%s_%d" % (name, i), mat)
    export(name)
    clear()
    rng = random.Random(SEED)
    k = 0
    for make, mat, n, flat in parts:
        for p in fracture(make(), n, rng, flat):
            add(p, "%s_piece_%d" % (name, k), mat)
            k += 1
    export(name + "_pieces")


# ---- the pieces -----------------------------------------------------------------------------------
# A sandstone pillar, 1.2 x 3.6 x 1.2: a plinth, an eight-sided shaft, a capital. Cover that breaks
PILLAR = [
    (lambda: box((0, 0.2, 0), (1.2, 0.4, 1.2)), "sandstone*0.85", 3, 0.2),
    (lambda: prism((0, 1.75, 0), 0.48, 2.7, 8, 0.44), "sandstone", 9, 0.7),
    (lambda: box((0, 3.3, 0), (1.15, 0.3, 1.15)), "sandstone*0.9", 3, 0.3),
]
# Two crates, one on the other, 1.5 x 2.4 x 1.5, iron at the corners: Sunset Yard's cover, but it breaks
CRATES = [
    (lambda: box((0, 0.6, 0), (1.5, 1.2, 1.5)), "wood", 6, 0.4),
    (lambda: box((0.1, 1.8, -0.05), (1.2, 1.2, 1.2)), "wood*0.9", 5, 0.4),
] + [(lambda sx=sx, sz=sz: box((sx * 0.7, 0.6, sz * 0.7), (0.12, 1.2, 0.12)), "iron", 1, 0.0)
     for sx in (-1, 1) for sz in (-1, 1)]
PIECES = {"pillar_sandstone": PILLAR, "crates_breakable": CRATES}

if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    for nm in (args or list(PIECES)):
        build(nm, PIECES[nm])
