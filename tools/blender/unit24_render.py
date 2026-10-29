# UNIT 24 from every side, for looking at (blender -b --factory-startup --python tools/blender/unit24_render.py
# -- OUT.png [model.glb]): front, three-quarter from his left (the open temple), his left side, back,
# three-quarter from his right (the open ribs) and his head up close, lit as a studio would, his glow cyan,
# on one sheet (3 x 2).
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

REPO = "/Users/ollie/Projects/overdrive/3d_shooter"
args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = args[0] if args else os.path.join(REPO, "build", "unit24_sheet.png")
MODEL = args[1] if len(args) > 1 else os.path.join(REPO, "assets", "enemies", "unit24.glb")
CYAN = (0.05, 0.6, 1.0)
W, H = 640, 800

for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob, do_unlink=True)
bpy.ops.import_scene.gltf(filepath=MODEL)
for m in bpy.data.materials:
    if not m.use_nodes:
        continue
    b = m.node_tree.nodes.get("Principled BSDF")
    if b is None:
        continue
    # (the palette is in display colours, as the game draws them: Blender wants them linear)
    bc = b.inputs["Base Color"].default_value
    b.inputs["Base Color"].default_value = (bc[0] ** 2.2, bc[1] ** 2.2, bc[2] ** 2.2, 1)
    if m.name.startswith("glow"):
        b.inputs["Base Color"].default_value = (*CYAN, 1)
        b.inputs["Emission Color"].default_value = (*CYAN, 1)
        b.inputs["Emission Strength"].default_value = 2.2
    elif m.name.startswith("ceramic"):
        b.inputs["Roughness"].default_value = 0.35
        b.inputs["Metallic"].default_value = 0.0
    elif m.name.startswith("chrome"):
        b.inputs["Roughness"].default_value = 0.3
        b.inputs["Metallic"].default_value = 0.85
    elif m.name.startswith("servo"):
        b.inputs["Roughness"].default_value = 0.45
        b.inputs["Metallic"].default_value = 0.5
    else:
        b.inputs["Roughness"].default_value = 0.6
        b.inputs["Metallic"].default_value = 0.15

sc = bpy.context.scene
sc.render.engine = "BLENDER_EEVEE"
sc.render.resolution_x, sc.render.resolution_y = W, H
sc.render.resolution_percentage = 100
sc.render.image_settings.file_format = "PNG"
try:
    sc.eevee.taa_render_samples = 48
    sc.eevee.use_raytracing = True
except AttributeError:
    pass
sc.view_settings.view_transform = "AgX"
w = sc.world or bpy.data.worlds.new("w")
sc.world = w
w.use_nodes = True
bg = w.node_tree.nodes.get("Background")
bg.inputs["Color"].default_value = (0.035, 0.035, 0.045, 1)
bg.inputs["Strength"].default_value = 1.0

# a floor to stand on, lights
bpy.ops.mesh.primitive_plane_add(size=12, location=(0, 0, 0))
floor = bpy.context.object
fm = bpy.data.materials.new("floor")
fm.use_nodes = True
fm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.05, 0.05, 0.06, 1)
fm.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.8
floor.data.materials.append(fm)
for name, loc, energy, col, size in (("key", (2.2, -2.6, 3.0), 380, (1.0, 0.95, 0.9), 1.5),
                                     ("fill", (-2.6, -1.8, 1.6), 120, (0.75, 0.8, 1.0), 2.0),
                                     ("rim", (0.5, 2.8, 2.6), 420, (0.6, 0.85, 1.0), 1.0)):
    lt = bpy.data.objects.new(name, bpy.data.lights.new(name, "AREA"))
    lt.data.energy, lt.data.color, lt.data.size = energy, col, size
    lt.location = loc
    lt.rotation_euler = (Vector((0, 0, 1.0)) - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
    sc.collection.objects.link(lt)

cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
sc.collection.objects.link(cam)
sc.camera = cam


def shot(path, yaw_deg, height, dist, look_z, lens):
    """yaw 0: in front of him (game +Z = Blender -Y); positive: round to his left (+X)"""
    a = math.radians(yaw_deg)
    at = Vector((math.sin(a) * dist, -math.cos(a) * dist, height))
    cam.location = at
    cam.rotation_euler = (Vector((0, 0, look_z)) - at).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = lens
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)


tmp = os.path.join(os.path.dirname(OUT), "_unit24_views")
os.makedirs(tmp, exist_ok=True)
views = [(0, 1.05, 4.2, 0.92, 50), (40, 1.2, 4.2, 0.92, 50), (90, 1.05, 4.2, 0.92, 50),
         (180, 1.05, 4.2, 0.92, 50), (-40, 1.2, 4.2, 0.92, 50), (28, 1.74, 1.0, 1.68, 55)]
paths = []
for i, v in enumerate(views):
    p = os.path.join(tmp, "v%d.png" % i)
    shot(p, *v)
    paths.append(p)

# one sheet, 3 x 2
sheet = np.zeros((H * 2, W * 3, 4), dtype=np.float32)
for i, p in enumerate(paths):
    img = bpy.data.images.load(p)
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)   # (bottom row first)
    r, c = 1 - i // 3, i % 3
    sheet[r * H:(r + 1) * H, c * W:(c + 1) * W] = px
out = bpy.data.images.new("sheet", W * 3, H * 2, alpha=True)
out.pixels = sheet.ravel()
out.filepath_raw = OUT
out.file_format = "PNG"
out.save()
print("unit24_render:", OUT)
