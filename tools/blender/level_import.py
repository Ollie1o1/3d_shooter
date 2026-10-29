# A level, as the game draws it, into Blender: the place to lay new things out (OD_LAYOUT) and to render it.
#
#   make level-dump                      (writes build/levels/<level>/: the world, its textures, its arenas)
#   blender --python tools/blender/level_import.py -- act1             (act1, act2, fast or story1; opens it to work on)
#   blender -b --python tools/blender/level_import.py -- act1 --save build/levels/act1.blend
#
# What you get (Blender is Z up; the game is Y up: game (x, y, z) is Blender (x, -z, y)):
#   World       the level's faces as the game draws them, one object per texture, with the game's own
#               textures, UVs and shading; NEON self-lit. For looking at: edits here go nowhere.
#   Collision   every collision box as one mesh (hidden).
#   Arenas      per arena: its bounds (a wire box), its start (an arrow), its spawns (spheres: ground,
#               cones: air). Each carries od_arena (its index).
#   OD_LAYOUT   what assets/levels/<level>.json adds. EDIT THIS: add, move and delete objects in it, then
#               tools/blender/layout_export.py writes the file back. Each object's od_kind says what it is:
#                 a mesh   wall / prop / neon / destructible (its box: the bounds as placed; od_mat the
#                          surface - brick panel rock metal concrete adobe plate marble circuit basalt
#                          wood limestone brass; od_name and od_hp a destructible's)
#                 an empty spawn_ground / spawn_air, or model (od_name: assets/props/<name>.glb)
#               No od_kind: a mesh is a wall; a name starting "spawn_ground", "spawn_air", "prop", "neon"
#               says it too.
import json
import math
import os
import sys

import bpy
import bmesh
from mathutils import Matrix, Vector

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
C = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))   # game -> Blender
CI = C.inverted()
MATS = ["brick", "panel", "rock", "metal", "concrete", "adobe", "plate", "marble", "circuit", "basalt", "wood",
        "limestone", "brass"]


def to_b(v):
    return Vector((v[0], -v[2], v[1]))


def to_game(v):
    return [v[0], v[2], -v[1]]


def collection(name, parent=None, hide=False):
    col = bpy.data.collections.get(name)
    if col is None:
        col = bpy.data.collections.new(name)
        (parent or bpy.context.scene.collection).children.link(col)
    col.hide_render = hide
    if hide:
        col.hide_viewport = True
    return col


def world_material(tex, level_dir, emit=False):
    name = "od_" + tex
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    vc = nt.nodes.new("ShaderNodeVertexColor")
    vc.layer_name = "Col"
    if emit:
        em = nt.nodes.new("ShaderNodeEmission")
        em.inputs["Strength"].default_value = 6.0
        nt.links.new(vc.outputs["Color"], em.inputs["Color"])
        nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
        return m
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Roughness"].default_value = 0.8
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = 1.0
    path = os.path.join(level_dir, "tex_" + tex + ".bmp")
    if os.path.exists(path):
        img = nt.nodes.new("ShaderNodeTexImage")
        img.image = bpy.data.images.load(path, check_existing=True)
        nt.links.new(img.outputs["Color"], mix.inputs["A"])
    else:
        mix.inputs["A"].default_value = (1, 1, 1, 1)
    nt.links.new(vc.outputs["Color"], mix.inputs["B"])
    nt.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return m


def batch_object(b, level_dir, col):
    pos, nrm, uv, cl, idx = b["pos"], b["nrm"], b["uv"], b["col"], b["idx"]
    nv, nt = len(pos) // 3, len(idx) // 3
    me = bpy.data.meshes.new("world_" + b["tex"])
    verts = []
    for i in range(nv):
        verts += [pos[3 * i], -pos[3 * i + 2], pos[3 * i + 1]]
    me.vertices.add(nv)
    me.vertices.foreach_set("co", verts)
    me.loops.add(nt * 3)
    me.loops.foreach_set("vertex_index", idx)
    me.polygons.add(nt)
    me.polygons.foreach_set("loop_start", list(range(0, nt * 3, 3)))
    me.update()
    layer = me.uv_layers.new(name="UV")
    luv = [0.0] * (nt * 6)
    for k, vi in enumerate(idx):
        luv[2 * k] = uv[2 * vi]
        luv[2 * k + 1] = uv[2 * vi + 1]
    layer.data.foreach_set("uv", luv)
    ca = me.color_attributes.new("Col", "FLOAT_COLOR", "POINT")
    rgba = []
    for i in range(nv):
        rgba += [min(cl[3 * i], 4.0), min(cl[3 * i + 1], 4.0), min(cl[3 * i + 2], 4.0), 1.0]
    ca.data.foreach_set("color", rgba)
    ns = [(nrm[3 * i], -nrm[3 * i + 2], nrm[3 * i + 1]) for i in range(nv)]
    try:
        me.normals_split_custom_set_from_vertices(ns)
    except Exception:
        pass
    me.materials.append(world_material(b["tex"], level_dir, b.get("emit", False)))
    ob = bpy.data.objects.new(("NEON" if b.get("emit") else "world_" + b["tex"]), me)
    col.objects.link(ob)
    return ob


def box_mesh(name, lo, hi):
    """A box from game-space corners, as a mesh centred on its object (the object sits at its centre)"""
    blo, bhi = to_b(lo), to_b(hi)
    mn = Vector((min(blo.x, bhi.x), min(blo.y, bhi.y), min(blo.z, bhi.z)))
    mx = Vector((max(blo.x, bhi.x), max(blo.y, bhi.y), max(blo.z, bhi.z)))
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=mx - mn, verts=bm.verts)
    bm.to_mesh(me)
    bm.free()
    return me, (mn + mx) * 0.5


def empty(name, col, at, kind="PLAIN_AXES", size=0.6):
    ob = bpy.data.objects.new(name, None)
    ob.empty_display_type = kind
    ob.empty_display_size = size
    ob.location = at
    col.objects.link(ob)
    return ob


def flat_material(name, rgb, emit=False):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.diffuse_color = (rgb[0], rgb[1], rgb[2], 1.0)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (rgb[0], rgb[1], rgb[2], 1.0)
        if emit:
            bsdf.inputs["Emission Color"].default_value = (rgb[0], rgb[1], rgb[2], 1.0)
            bsdf.inputs["Emission Strength"].default_value = 5.0
    return m


def place_prop(name, xf_game, col):
    """assets/props/<name>.glb under an empty placed by the game's matrix (column major)"""
    path = os.path.join(REPO, "assets", "props", name + ".glb")
    holder = empty("model:" + name, col, (0, 0, 0), "CUBE", 0.25)
    holder["od_kind"] = "model"
    holder["od_name"] = name
    m = Matrix([[xf_game[c * 4 + r] for c in range(4)] for r in range(4)])
    holder.matrix_world = C @ m @ CI
    if os.path.exists(path):
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=path)
        for ob in set(bpy.data.objects) - before:
            for c in list(ob.users_collection):
                c.objects.unlink(ob)
            col.objects.link(ob)
            if ob.parent is None:
                ob.parent = holder
            ob.hide_select = True   # (move the holder, not its parts)
    return holder


def load_layout(level, col):
    path = os.path.join(REPO, "assets", "levels", level + ".json")
    if not os.path.exists(path):
        return 0
    with open(path) as f:
        doc = json.load(f)
    n = 0
    for o in doc.get("objects", []):
        k = o.get("kind")
        if k in ("wall", "prop", "neon", "destructible"):
            me, centre = box_mesh(k, o["min"], o["max"])
            ob = bpy.data.objects.new(k if k != "destructible" else "destructible:" + o.get("name", ""), me)
            ob.location = centre
            col.objects.link(ob)
            ob["od_kind"] = k
            if o.get("mat"):
                ob["od_mat"] = o["mat"]
            if o.get("color"):
                ob["od_color"] = o["color"]
                me.materials.append(flat_material("od_flat_%s_%.2f_%.2f_%.2f" % ((k,) + tuple(o["color"])), o["color"], k == "neon"))
            if k == "destructible":
                ob["od_name"] = o.get("name", "")
                ob["od_hp"] = o.get("hp", 300)
                ob.display_type = "WIRE"
                path = os.path.join(REPO, "assets", "props", o.get("name", "") + ".glb")
                if os.path.exists(path):   # what it looks like whole, fitted to its box (for looking at only)
                    before = set(bpy.data.objects)
                    bpy.ops.import_scene.gltf(filepath=path)
                    new = set(bpy.data.objects) - before
                    for p in new:
                        for c in list(p.users_collection):
                            c.objects.unlink(p)
                        col.objects.link(p)
                        p.hide_select = True
                    fit_to(new, ob)
        elif k in ("spawn_ground", "spawn_air"):
            ob = empty(k, col, to_b(o["pos"]), "SPHERE" if k == "spawn_ground" else "CONE", 0.6)
            ob["od_kind"] = k
            if "arena" in o:
                ob["od_arena"] = o["arena"]
        elif k == "model":
            place_prop(o["name"], o["xf"], col)
        n += 1
    return n


def fit_to(objs, box_ob):
    """Scale and move imported parts so their bounds fill box_ob's (a destructible's look in its box)"""
    roots = [o for o in objs if o.parent is None or o.parent not in objs]
    pts = []
    bpy.context.view_layer.update()
    for o in objs:
        if o.type == "MESH":
            pts += [o.matrix_world @ Vector(c) for c in o.bound_box]
    if not pts:
        return
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    dims = box_ob.dimensions
    s = Vector((dims.x / max(hi.x - lo.x, 1e-4), dims.y / max(hi.y - lo.y, 1e-4), dims.z / max(hi.z - lo.z, 1e-4)))
    target_lo = box_ob.location - dims * 0.5
    holder = bpy.data.objects.new("look", None)
    box_ob.users_collection[0].objects.link(holder)
    holder.matrix_world = Matrix.Translation(target_lo) @ Matrix.Diagonal((s.x, s.y, s.z, 1)) @ Matrix.Translation(-lo)
    for r in roots:
        r.parent = holder
    holder.parent = box_ob
    holder.matrix_parent_inverse = box_ob.matrix_world.inverted()
    holder.hide_select = True


def set_light(theme, arena_name=""):
    """The scene's sun and sky from an arena's theme"""
    sc = bpy.context.scene
    sun = bpy.data.objects.get("OD_SUN")
    if sun is None:
        sun = bpy.data.objects.new("OD_SUN", bpy.data.lights.new("OD_SUN", "SUN"))
        sc.collection.objects.link(sun)
    d = to_b(theme["lightDir"]).normalized()   # (the way the light travels)
    sun.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    lc = theme["lightColor"]
    peak = max(lc[0], lc[1], lc[2], 1e-3)
    sun.data.color = (lc[0] / peak, lc[1] / peak, lc[2] / peak)
    sun.data.energy = 3.0 * peak
    sun.data.angle = math.radians(2.0)
    w = sc.world or bpy.data.worlds.new("OD_WORLD")
    sc.world = w
    w.use_nodes = True
    nt = w.node_tree
    bg = nt.nodes.get("Background")
    if bg:   # the arena's sky: its horizon colour up to its zenith's (as skybox.frag blends them)
        mix = nt.nodes.get("OD_SKY")
        if mix is None:
            tc = nt.nodes.new("ShaderNodeTexCoord")
            sep = nt.nodes.new("ShaderNodeSeparateXYZ")
            rng = nt.nodes.new("ShaderNodeMapRange")
            rng.inputs["From Min"].default_value, rng.inputs["From Max"].default_value = -0.05, 0.6
            mix = nt.nodes.new("ShaderNodeMix")
            mix.name = "OD_SKY"
            mix.data_type = "RGBA"
            nt.links.new(tc.outputs["Generated"], sep.inputs["Vector"])
            nt.links.new(sep.outputs["Z"], rng.inputs["Value"])
            nt.links.new(rng.outputs["Result"], mix.inputs["Factor"])
            nt.links.new(mix.outputs["Result"], bg.inputs["Color"])
        hz, zn = theme["horizon"], theme["zenith"]
        mix.inputs["A"].default_value = (hz[0], hz[1], hz[2], 1.0)
        mix.inputs["B"].default_value = (zn[0], zn[1], zn[2], 1.0)
        bg.inputs["Strength"].default_value = 1.0


def import_level(level, dump_root=None, with_layout=True):
    level_dir = dump_root or os.path.join(REPO, "build", "levels", level)
    path = os.path.join(level_dir, "level.json")
    if not os.path.exists(path):
        raise SystemExit("no %s: run `make level-dump` first" % path)
    with open(path) as f:
        doc = json.load(f)
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    world = collection("World")
    for b in doc["batches"]:
        batch_object(b, level_dir, world)
    # the collision boxes, one hidden mesh
    coll = collection("Collision", hide=True)
    bm = bmesh.new()
    for w in doc["walls"]:
        lo, hi = to_b(w["box"]["min"]), to_b(w["box"]["max"])
        mn = Vector((min(lo.x, hi.x), min(lo.y, hi.y), min(lo.z, hi.z)))
        mx = Vector((max(lo.x, hi.x), max(lo.y, hi.y), max(lo.z, hi.z)))
        r = bmesh.ops.create_cube(bm, size=1.0)
        bmesh.ops.scale(bm, vec=mx - mn, verts=r["verts"])
        bmesh.ops.translate(bm, vec=(mn + mx) * 0.5, verts=r["verts"])
    me = bpy.data.meshes.new("collision")
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new("collision", me)
    ob.display_type = "WIRE"
    coll.objects.link(ob)
    # the arenas
    arenas = collection("Arenas")
    for i, a in enumerate(doc["arenas"]):
        ac = collection("%d %s" % (i + 1, a["name"]), arenas)
        me, centre = box_mesh("bounds", a["bounds"]["min"], a["bounds"]["max"])
        ob = bpy.data.objects.new("bounds %d" % (i + 1), me)
        ob.location = centre
        ob.display_type = "WIRE"
        ob.hide_render = True
        ob["od_arena"] = i
        ac.objects.link(ob)
        st = empty("start %d" % (i + 1), ac, to_b(a["start"]), "SINGLE_ARROW", 2.0)
        yaw = math.radians(a["yaw"])   # (game: -90 faces -Z, north)
        fwd = Vector((math.cos(yaw), 0.0, math.sin(yaw)))
        st.rotation_euler = to_b(fwd).to_track_quat("Z", "Y").to_euler()
        st["od_arena"] = i
        for p in a["ground"]:
            empty("ground %d" % (i + 1), ac, to_b(p), "SPHERE", 0.5)["od_arena"] = i
        for p in a["air"]:
            empty("air %d" % (i + 1), ac, to_b(p), "CONE", 0.5)["od_arena"] = i
    lay = collection("OD_LAYOUT")
    n = load_layout(level, lay) if with_layout else 0
    set_light(doc["arenas"][0]["theme"])
    sc = bpy.context.scene
    sc["od_level"] = level
    sc.unit_settings.system = "METRIC"
    print("level %s: %d batches, %d walls, %d arenas, %d layout objects" %
          (level, len(doc["batches"]), len(doc["walls"]), len(doc["arenas"]), n))
    return doc


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not args:
        print("usage: blender [-b] --python tools/blender/level_import.py -- act1|act2|fast|story1 [--save FILE.blend]")
        sys.exit(2)
    import_level(args[0])
    if "--save" in args:
        out = os.path.abspath(args[args.index("--save") + 1])
        os.makedirs(os.path.dirname(out), exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=out)
        print("saved", out)
