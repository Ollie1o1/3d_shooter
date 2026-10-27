# Shared helpers for the enemy build scripts (tools/blender/<enemy>_build.py), run over the
# live link: each script starts with  exec(open(KIT).read())  and then
#
#   open_enemy("ripper")     a scene of its own (the one you have open stays as it is),
#                            the enemy's file imported: its joints are where the code turns them
#   clear_parts()            every box gone, the joints kept
#   B(...) / C(...)          a box (bevelled: a real mesh) / a cylinder, in game space
#   save_enemy("ripper")     exported to assets/enemies/ripper.glb (that scene only)
#
# Game space: +Y up, the enemy faces +Z, its right side is -X, metres.
import contextlib
import io
import os
import sys

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector

_og = sys.modules["overdrive_guns"]
REPO = "/Users/ollie/Projects/overdrive/3d_shooter"

# (an add-on loaded before the enemies were added: teach it their words)
for _j in ["hipL", "hipR", "torso", "head", "armL", "armR", "body", "legFL", "legFR", "legBL", "legBR",
           "bladeL", "bladeR", "tail", "wingL", "wingR", "tipL", "tipR",
           "legL1", "legL2", "legL3", "legR1", "legR2", "legR3"]:
    if _j not in _og.JOINTS:
        _og.JOINTS.append(_j)
for _c in ["shot", "pulse", "dive", "fuse", "blade"]:
    if _c not in _og.CHANNELS:
        _og.CHANNELS.append(_c)
_og.PALETTE.update({"husk": (0.70, 0.26, 0.18), "gunmetal": (0.16, 0.15, 0.17), "sentinel": (0.20, 0.32, 0.62),
                    "brute": (0.58, 0.24, 0.17), "ripper": (0.80, 0.64, 0.14), "raptor": (0.40, 0.16, 0.52),
                    "mite": (0.18, 0.30, 0.16), "bone": (0.76, 0.71, 0.61), "beak": (0.9, 0.6, 0.15)})


ENEMY_SCENES = ["HUSK", "SENTINEL", "BRUTE", "RIPPER", "RAPTOR", "MITE"]
NEEDED = {"husk": "hipL hipR torso head armL armR", "sentinel": "hipL hipR torso head armL armR",
          "brute": "hipL hipR torso head armL armR", "ripper": "body head legFL legFR legBL legBR bladeL bladeR",
          "raptor": "head tail wingL wingR tipL tipR", "mite": "body legL1 legL2 legL3 legR1 legR2 legR3"}


def open_enemy(name):
    """Its own scene, its file imported. Object names are shared by every scene in Blender, so the other
    enemy scenes these scripts made (each saved to its file) are removed first, or 'hipL' would come in
    as 'hipL.001' and the game wouldn't find it. Other scenes (a gun you're working on) are left alone."""
    sc = bpy.data.scenes.get(name.upper()) or bpy.data.scenes.new(name.upper())
    bpy.context.window_manager.windows[0].scene = sc
    for other in [x for x in bpy.data.scenes if x.name in ENEMY_SCENES and x != sc]:
        for ob in list(other.objects):
            bpy.data.objects.remove(ob, do_unlink=True)
        bpy.data.scenes.remove(other)
    for ob in list(sc.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    with contextlib.redirect_stdout(io.StringIO()):
        bpy.ops.import_scene.gltf(filepath=os.path.join(REPO, "assets", "enemies", name + ".glb"))
    for ob in sc.objects:
        if ob.type == "MESH" and ob.data.users > 1:
            ob.data = ob.data.copy()
    for ob in sc.objects:   # (any joint that still came in renamed: its own name back)
        base = ob.name.split(".")[0]
        if base != ob.name and base in NEEDED[name].split() and not bpy.data.objects.get(base):
            ob.name = base
    sc["overdrive_opened"] = name
    return sc


def clear_parts():
    for ob in [o for o in bpy.context.scene.objects if o.type == "MESH"]:
        bpy.data.objects.remove(ob, do_unlink=True)


def save_enemy(name):
    sc = bpy.context.scene
    assert sc.get("overdrive_opened") == name, "this scene isn't the " + name
    names = {o.name for o in sc.objects}
    missing = [j for j in NEEDED[name].split() if j not in names]
    assert not missing, "not saved: no joint named " + ", ".join(missing) + " (renamed by Blender?)"
    with contextlib.redirect_stdout(io.StringIO()):
        bpy.ops.export_scene.gltf(filepath=os.path.join(REPO, "assets", "enemies", name + ".glb"), export_format="GLB",
                                  export_extras=True, export_yup=True, export_apply=True, use_selection=False,
                                  use_active_scene=True, export_texcoords=False)
    n = sum(1 for o in sc.objects if o.type == "MESH")
    print("%s: %d parts saved" % (name, n))


def _apply_scale(ob):
    """Bake the object's scale into its mesh (so a bevel is the same width on every edge)"""
    loc, rot, sca = ob.matrix_basis.decompose()
    ob.data.transform(Matrix.Diagonal((sca.x, sca.y, sca.z, 1.0)))
    ob.matrix_basis = Matrix.Translation(loc) @ rot.to_matrix().to_4x4()


def B(c, s, mat, parent, rot=(0, 0, 0), glow=None, k=1.0, bev=0.0, seg=1):
    """A box; bev > 0 bevels its edges (metres, `seg` segments): a real mesh in the game, not a box"""
    ob = add_box(c, s, mat, parent=parent, rot=rot)
    if bev > 0:
        _apply_scale(ob)
        m = ob.modifiers.new("bevel", "BEVEL")
        m.width = min(bev, 0.45 * min(s))
        m.segments = seg
        m.limit_method = "NONE"
    if glow:
        set_glow(ob, glow, k)
    return ob


def C(c, radius, length, mat, parent, rot=(0, 0, 0), sides=12, glow=None, k=1.0, r2=None):
    """A cylinder along the game's +Y (rot turns it), smooth round its side, flat caps; r2: the radius at +Y"""
    if isinstance(parent, str):
        parent = find(parent)
    me = bpy.data.meshes.new(mat)
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=sides, radius1=radius,
                          radius2=radius if r2 is None else r2, depth=length)
    for f in bm.faces:
        f.smooth = len(f.verts) == 4
    for e in bm.edges:
        e.smooth = not any(len(f.verts) != 4 for f in e.link_faces)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(mat, me)
    ob.data.materials.append(palette_material(mat))
    bpy.context.scene.collection.objects.link(ob)
    g = Matrix.Translation(Vector(c)) @ Euler([r * 3.14159265358979 / 180.0 for r in rot], "XYZ").to_matrix().to_4x4()
    ob.parent = parent
    ob.matrix_world = _og.game_to_blender(g)
    if glow:
        set_glow(ob, glow, k)
    return ob


def W(c, s, mat, parent, rot=(0, 0, 0), glow=None, k=1.0, taper=0.4):
    """A wedge: a box whose +Z end narrows to `taper` of its height (a beak, a blade's point, a claw)"""
    ob = add_box(c, s, mat, parent=parent, rot=rot)
    _apply_scale(ob)
    me = ob.data
    for v in me.vertices:
        # the unit cube's +Z end (in game space: Blender -Y) closes up in height (Blender Z)
        if v.co.y < 0:
            v.co.z *= taper
    me.update()
    if glow:
        set_glow(ob, glow, k)
    return ob
