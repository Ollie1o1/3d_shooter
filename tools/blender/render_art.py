# The menus' art, rendered in Blender (Eevee): a shot of every arena in its own light, and every boss.
#
#   make level-dump
#   blender -b --factory-startup --python tools/blender/render_art.py               (everything)
#   blender -b --factory-startup --python tools/blender/render_art.py -- arenas act1   (or: bosses)
#
# Into assets/art/: arena_<level>_<n>.jpg (960 x 540, n from 0) and boss_<n>.jpg (480 x 600, in the BOSS
# page's order: Progression.h's bossEntry). The menus draw them behind their pages (ui/MenuArt.h).
import math
import os
import sys

import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import level_import as li  # noqa: E402

REPO = li.REPO
OUT = os.path.join(REPO, "assets", "art")
BOSSES = ["warden", "sovereign", "penitent", "leviathan", "absolute", "morningstar"]   # (bossEntry's order)
BOSS_GLOW = {   # each one's glow colour (EnemyTypes.h stats), for its glowing parts
    "warden": (1.0, 0.2, 0.75), "sovereign": (1.0, 0.75, 0.3), "penitent": (1.0, 0.45, 0.15),
    "leviathan": (0.3, 0.9, 1.0), "absolute": (1.0, 0.95, 0.8), "morningstar": (1.0, 0.55, 0.2),
}


def setup_render(w, h):
    sc = bpy.context.scene
    sc.render.engine = "BLENDER_EEVEE"
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sc.render.resolution_percentage = 100
    sc.render.image_settings.file_format = "JPEG"
    sc.render.image_settings.quality = 82
    try:
        sc.eevee.taa_render_samples = 48
        sc.eevee.use_raytracing = True
    except AttributeError:
        pass
    sc.view_settings.view_transform = "AgX"
    try:
        sc.view_settings.look = "AgX - Medium High Contrast"
    except TypeError:
        pass
    return sc


def camera(sc, at, look, lens):
    cam = bpy.data.objects.get("OD_CAM")
    if cam is None:
        cam = bpy.data.objects.new("OD_CAM", bpy.data.cameras.new("OD_CAM"))
        sc.collection.objects.link(cam)
    sc.camera = cam
    cam.location = at
    cam.rotation_euler = (look - at).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = lens
    cam.data.clip_end = 2000
    return cam


# Arenas whose middle is a tower (the shot from the entry runs into it): from a top corner instead
CORNER = {("act1", 2)}


def arena_shot(doc, i, corner=False):
    """From the arena's entry side, up high, looking across it (or from a top corner)"""
    a = doc["arenas"][i]
    lo, hi = Vector(a["bounds"]["min"]), Vector(a["bounds"]["max"])
    c = (lo + hi) * 0.5
    floor = lo.y if lo.y > -1e3 else 0.0
    h = hi.y - floor
    g = a["ground"]
    if g:   # where the fight is: its ground spawns' middle, on their floor (Act II's bounds run far up and down)
        ys = sorted(p[1] for p in g)
        floor = ys[len(ys) // 2]
        c = Vector((sum(p[0] for p in g) / len(g), floor, sum(p[2] for p in g) / len(g)))
        h = min(h, 24.0)
    s = Vector(a["start"])
    d = Vector((c.x - s.x, 0.0, c.z - s.z))
    if d.length < 5.0:
        yaw = math.radians(a["yaw"])
        d = Vector((math.cos(yaw), 0.0, math.sin(yaw)))
    d.normalize()
    half = abs(d.x) * (hi.x - lo.x) * 0.5 + abs(d.z) * (hi.z - lo.z) * 0.5
    side = Vector((-d.z, 0.0, d.x))
    eye = c - d * (half * 0.78) + side * (half * 0.42)
    eye.y = floor + max(9.0, min(h * 0.7, 20.0))
    if corner:
        eye = c - d * (half * 0.95) + side * (half * 0.95)
        eye.y = floor + h * 0.85
    target = c + d * (half * 0.1) - side * (half * 0.1)
    target.y = floor + min(2.0, h * 0.15)
    return li.to_b(eye), li.to_b(target)


def clear_view(sc, target, eye):
    """The eye pulled in front of anything between it and what it looks at (a room's wall, a roof)"""
    dg = bpy.context.evaluated_depsgraph_get()
    d = eye - target
    dist = d.length
    d.normalize()
    hit, loc, *_ = sc.ray_cast(dg, target + d * 0.5, d, distance=dist)
    if hit:
        return target + d * max(4.0, (loc - target).length - 1.5)
    return eye


def render_arenas(levels):
    os.makedirs(OUT, exist_ok=True)
    for level in levels:
        doc = li.import_level(level)
        sc = setup_render(960, 540)
        for i, a in enumerate(doc["arenas"]):
            li.set_light(a["theme"])
            eye, target = arena_shot(doc, i, (level, i) in CORNER)
            eye = clear_view(sc, target, eye)
            camera(sc, eye, target, 17)
            sc.render.filepath = os.path.join(OUT, "arena_%s_%d.jpg" % (level, i))
            bpy.ops.render.render(write_still=True)
            print("render_art:", sc.render.filepath)


def glowing(m):
    return m.name.startswith("glow")


def render_bosses():
    os.makedirs(OUT, exist_ok=True)
    for n, rig in enumerate(BOSSES):
        for ob in list(bpy.data.objects):
            bpy.data.objects.remove(ob, do_unlink=True)
        bpy.ops.import_scene.gltf(filepath=os.path.join(REPO, "assets", "enemies", rig + ".glb"))
        g = BOSS_GLOW[rig]
        for m in bpy.data.materials:   # its glow parts lit (the game lights them from code)
            if not m.use_nodes:
                continue
            b = m.node_tree.nodes.get("Principled BSDF")
            if b is None:
                continue
            if glowing(m):
                b.inputs["Base Color"].default_value = (g[0], g[1], g[2], 1)
                b.inputs["Emission Color"].default_value = (g[0], g[1], g[2], 1)
                b.inputs["Emission Strength"].default_value = 6.0
            b.inputs["Roughness"].default_value = 0.55
            b.inputs["Metallic"].default_value = 0.25
        bpy.context.view_layer.update()
        # frame what's near its head (the Leviathan's body is one segment laid 12 m behind it)
        keep = None
        if rig == "leviathan":
            head = bpy.data.objects.get("head")
            keep = set([head] + list(head.children_recursive)) if head else None
        pts = []
        for o in bpy.data.objects:
            if o.type == "MESH" and (keep is None or o in keep):
                pts += [o.matrix_world @ Vector(c) for c in o.bound_box]
        lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
        hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
        c, size = (lo + hi) * 0.5, (hi - lo)
        tall = max(size.z * 0.82, size.x * 0.7, 0.5) * (1.6 if rig == "leviathan" else 1.0)
        sc = setup_render(480, 600)
        w = sc.world or bpy.data.worlds.new("w")
        sc.world = w
        w.use_nodes = True
        bg = w.node_tree.nodes.get("Background")
        bg.inputs["Color"].default_value = (0.02, 0.015, 0.03, 1)
        bg.inputs["Strength"].default_value = 1.0
        # three-quarter, from in front (the game's +Z, Blender's -Y), a little below its eyes
        eye = c + Vector((tall * 0.75, -tall * 1.55, tall * 0.08))
        camera(sc, eye, c + Vector((0, 0, tall * 0.06)), 50)
        for name, rot, energy, col in (("key", (0.9, 0.0, 0.7), 3.5, (1.0, 0.92, 0.85)),
                                       ("rim", (1.3, 0.0, 3.6), 6.0, g),
                                       ("fill", (1.2, 0.0, -1.2), 0.8, (0.5, 0.55, 0.8))):
            lt = bpy.data.objects.new(name, bpy.data.lights.new(name, "SUN"))
            lt.data.energy, lt.data.color = energy, col
            lt.rotation_euler = rot
            sc.collection.objects.link(lt)
        sc.render.filepath = os.path.join(OUT, "boss_%d.jpg" % n)
        bpy.ops.render.render(write_still=True)
        print("render_art:", sc.render.filepath)


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    what = args[0] if args else "all"
    if what in ("all", "arenas"):
        render_arenas(args[1:] or ["act1", "act2"])
    if what in ("all", "bosses"):
        render_bosses()
