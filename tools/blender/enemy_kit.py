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
           "legL1", "legL2", "legL3", "legR1", "legR2", "legR3", "shield",
           "weapon", "cape", "capeTip", "halo", "crown", "kneeL", "kneeR", "ankleL", "ankleR", "waist", "clothF", "clothB",
           "elbowL", "elbowR", "handL", "handR", "fingersL", "fingersR", "tipsL", "tipsR",
           "thumbL", "thumbR", "thumbTipL", "thumbTipR", "ribbonL", "ribbonR", "jaw",
           "shinL1", "shinL2", "shinL3", "shinR1", "shinR2", "shinR3"]:
    if _j not in _og.JOINTS:
        _og.JOINTS.append(_j)
for _c in ["shot", "pulse", "dive", "fuse", "blade", "smash", "rim", "edge", "cape", "halo", "seam", "soft", "light", "eye",
           "core", "tellR", "tellL", "tellLegR", "tellLegL", "auraR", "auraL", "auraLegR", "auraLegL", "runeR", "runeL", "sword"]:
    if _c not in _og.CHANNELS:
        _og.CHANNELS.append(_c)
for _f in ["blade", "!blade", "enraged", "!enraged", "open", "!open"]:   # (the bosses' flags: EnemyFile.h; "broken" the guns have already)
    if _f not in _og.FLAGS:
        _og.FLAGS.append(_f)
_og.PALETTE.update({"husk": (0.70, 0.26, 0.18), "gunmetal": (0.16, 0.15, 0.17), "sentinel": (0.20, 0.32, 0.62),
                    "brute": (0.58, 0.24, 0.17), "ripper": (0.80, 0.64, 0.14), "raptor": (0.40, 0.16, 0.52),
                    "mite": (0.18, 0.30, 0.16), "bone": (0.76, 0.71, 0.61), "beak": (0.9, 0.6, 0.15),
                    "juggernaut": (0.36, 0.38, 0.44), "shieldbearer": (0.34, 0.37, 0.42),
                    "gold": (0.85, 0.62, 0.18), "bronze": (0.62, 0.46, 0.20),
                    # the bosses: the SOVEREIGN's blackened plate, crimson and steel; THE ABSOLUTE's white
                    # porcelain over grey, dark seams; MORNINGSTAR's blood orange, char and obsidian
                    "sovereign": (0.12, 0.11, 0.14), "crimson": (0.32, 0.04, 0.05), "silver": (0.50, 0.52, 0.58),
                    "plate": (0.96, 0.96, 0.99), "skin": (0.58, 0.58, 0.64), "seam": (0.16, 0.16, 0.20),
                    "gauntlet": (0.22, 0.22, 0.28), "ember": (0.95, 0.27, 0.05), "blood": (0.80, 0.15, 0.03),
                    "char": (0.10, 0.03, 0.02), "obsidian": (0.05, 0.04, 0.05),
                    "warden": (0.34, 0.27, 0.40),
                    "iron": (0.26, 0.24, 0.25), "leviathan": (0.20, 0.18, 0.21),
                    "revenant": (0.20, 0.21, 0.26), "weaver": (0.16, 0.13, 0.20), "seraph": (0.82, 0.78, 0.66),
                    "anchor": (0.24, 0.27, 0.32), "conduit": (0.22, 0.20, 0.26), "conductor": (0.18, 0.30, 0.34), "rag": (0.15, 0.10, 0.11), "censer": (0.60, 0.45, 0.22)})


ENEMY_SCENES = ["HUSK", "SENTINEL", "BRUTE", "RIPPER", "RAPTOR", "MITE", "JUGGERNAUT", "SHIELDBEARER",
                "SOVEREIGN", "ABSOLUTE", "MORNINGSTAR", "WARDEN", "PENITENT", "LEVIATHAN",
                "REVENANT", "WEAVER", "SERAPH", "ANCHOR", "CONDUIT", "CONDUCTOR"]
_ABS = ("hipL hipR torso head armL armR halo crown weapon kneeL kneeR ankleL ankleR waist clothF clothB elbowL elbowR "
        "handL handR fingersL fingersR tipsL tipsR thumbL thumbR thumbTipL thumbTipR ribbonL ribbonR")
NEEDED = {"husk": "hipL hipR torso head armL armR", "sentinel": "hipL hipR torso head armL armR",
          "brute": "hipL hipR torso head armL armR", "ripper": "body head legFL legFR legBL legBR bladeL bladeR",
          "raptor": "head tail wingL wingR tipL tipR", "mite": "body legL1 legL2 legL3 legR1 legR2 legR3",
          "juggernaut": "hipL hipR torso head armL armR", "shieldbearer": "hipL hipR torso head armL armR shield",
          "sovereign": "hipL hipR torso head armL armR weapon cape capeTip halo crown",
          "warden": "hipL hipR torso head armL armR cape capeTip crown",
          "penitent": "hipL hipR torso head armL armR", "leviathan": "head jaw halo body",
          "revenant": "hipL hipR torso head armL armR", "anchor": "hipL hipR torso head armL armR",
          "weaver": "body legL1 legL2 legL3 legR1 legR2 legR3 shinL1 shinL2 shinL3 shinR1 shinR2 shinR3",
          "seraph": "head wingL wingR tipL tipR", "conduit": "halo crown head", "conductor": "body halo", "absolute": _ABS, "morningstar": _ABS}


def open_enemy(name, joints=None):
    """Its own scene, its file imported. Object names are shared by every scene in Blender, so the other
    enemy scenes these scripts made (each saved to its file) are removed first, or 'hipL' would come in
    as 'hipL.001' and the game wouldn't find it. Other scenes (a gun you're working on) are left alone."""
    wins = bpy.context.window_manager.windows
    sc = bpy.data.scenes.get(name.upper())
    if not sc and not wins:   # (headless.py, no window to switch: the one scene there is becomes this enemy's)
        sc = bpy.context.scene
        sc.name = name.upper()
    sc = sc or bpy.data.scenes.new(name.upper())
    if wins:
        wins[0].scene = sc
    for other in [x for x in bpy.data.scenes if x.name in ENEMY_SCENES and x != sc]:
        for ob in list(other.objects):
            bpy.data.objects.remove(ob, do_unlink=True)
        bpy.data.scenes.remove(other)
    for ob in list(sc.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    with contextlib.redirect_stdout(io.StringIO()):
        # (joints: a skeleton to start from instead, tools/enemy_export --joints NAME OUT.glb)
        bpy.ops.import_scene.gltf(filepath=joints or os.path.join(REPO, "assets", "enemies", name + ".glb"))
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


def B(c, s, mat, parent, rot=(0, 0, 0), glow=None, k=1.0, bev=0.0, seg=1, show=None, phase=None):
    """A box; bev > 0 bevels its edges (metres, `seg` segments): a real mesh in the game, not a box"""
    ob = add_box(c, s, mat, parent=parent, rot=rot)
    if bev > 0:
        _apply_scale(ob)
        m = ob.modifiers.new("bevel", "BEVEL")
        m.width = min(bev, 0.45 * min(s))
        m.segments = seg
        m.limit_method = "NONE"
    if glow:
        set_glow(ob, glow, k, phase)
    if show:
        set_show(ob, show)
    return ob


def C(c, radius, length, mat, parent, rot=(0, 0, 0), sides=12, glow=None, k=1.0, r2=None, show=None):
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
    if show:
        set_show(ob, show)
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


# ---- the bosses' shapes ------------------------------------------------------------------
def _finish(ob, mat, parent, g, glow, k, show, phase=None):
    if isinstance(parent, str):
        parent = find(parent)
    ob.data.materials.append(palette_material(mat))
    bpy.context.scene.collection.objects.link(ob)
    ob.parent = parent
    ob.matrix_world = _og.game_to_blender(g)
    if glow:
        set_glow(ob, glow, k, phase)
    if show:
        set_show(ob, show)
    return ob


def _game(c, rot):
    return Matrix.Translation(Vector(c)) @ Euler([r * 3.14159265358979 / 180.0 for r in rot], "XYZ").to_matrix().to_4x4()


def E(c, s, mat, parent, rot=(0, 0, 0), glow=None, k=1.0, seg=12, rings=8, show=None, phase=None):
    """An ellipsoid: size s along the game's x, y, z, smooth (a muscle, a gem, a pommel, a skull)"""
    me = bpy.data.meshes.new(mat)
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=seg, v_segments=rings, radius=0.5)
    # (bmesh's sphere is Blender z-up: game y is Blender z, game z is Blender -y)
    bmesh.ops.scale(bm, vec=Vector((s[0], s[2], s[1])), verts=bm.verts)
    for f in bm.faces:
        f.smooth = True
    bm.to_mesh(me)
    bm.free()
    return _finish(bpy.data.objects.new(mat, me), mat, parent, _game(c, rot), glow, k, show, phase)


def P(c, s, mat, parent, rot=(0, 0, 0), top=(1.0, 1.0), shift=(0.0, 0.0), glow=None, k=1.0, bev=0.0, seg=1, show=None, phase=None):
    """A tapered box: size s, its +Y end scaled by top (x, z) and moved by shift (x, z) - a limb, a plate, a horn's
    root, a blade; bev > 0 bevels it"""
    me = bpy.data.meshes.new(mat)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:   # Blender (x, y, z) is game (x, -z, y): the game's +Y end is Blender +z
        if v.co.z > 0:
            v.co.x = v.co.x * top[0] + shift[0]
            v.co.y = v.co.y * top[1] - shift[1]
    bmesh.ops.scale(bm, vec=Vector((s[0], s[2], s[1])), verts=bm.verts)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(mat, me)
    if bev > 0:
        m = ob.modifiers.new("bevel", "BEVEL")
        m.width = min(bev, 0.45 * min(s[0] * min(1.0, top[0]), s[1], s[2] * min(1.0, top[1])))
        m.segments = seg
        m.limit_method = "NONE"
    return _finish(ob, mat, parent, _game(c, rot), glow, k, show, phase)


def R(c, radius, tube, mat, parent, rot=(0, 0, 0), sides=24, tsides=6, arc=360.0, glow=None, k=1.0, show=None, phase=None):
    """A ring (torus) in the game's XY plane, facing +Z (rot turns it); arc < 360: an open arc"""
    import math
    me = bpy.data.meshes.new(mat)
    bm = bmesh.new()
    full = arc >= 359.9
    n = sides if full else sides + 1
    rings = []
    for i in range(n):
        a = math.radians(arc) * i / sides
        ring = []
        for j in range(tsides):
            b = 2 * math.pi * j / tsides
            r = radius + tube * math.cos(b)
            # game (x, y, z) -> Blender (x, -z, y)
            gx, gy, gz = r * math.cos(a), r * math.sin(a), tube * math.sin(b)
            ring.append(bm.verts.new((gx, -gz, gy)))
        rings.append(ring)
    for i in range(n if full else n - 1):
        a, b = rings[i], rings[(i + 1) % n]
        for j in range(tsides):
            f = bm.faces.new((a[j], b[j], b[(j + 1) % tsides], a[(j + 1) % tsides]))
            f.smooth = True
    if not full:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    return _finish(bpy.data.objects.new(mat, me), mat, parent, _game(c, rot), glow, k, show, phase)


def K(c, radius, length, mat, parent, rot=(0, 0, 0), sides=8, glow=None, k=1.0, show=None, phase=None, squash=1.0):
    """A spike: a cone along the game's +Y from its base at c (a horn, a crown's point, a flare); squash
    flattens it in z"""
    me = bpy.data.meshes.new(mat)
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=True, segments=sides, radius1=radius, radius2=0.0, depth=length)
    bmesh.ops.translate(bm, vec=Vector((0, 0, length * 0.5)), verts=bm.verts)
    bmesh.ops.scale(bm, vec=Vector((1.0, squash, 1.0)), verts=bm.verts)
    bm.to_mesh(me)
    bm.free()
    return _finish(bpy.data.objects.new(mat, me), mat, parent, _game(c, rot), glow, k, show, phase)


def show(ob, flag):
    set_show(ob, flag)
    return ob


def mirror_x(fn):
    """Call fn(s) for the left (+1) and right (-1) side"""
    for s in (1, -1):
        fn(s)


def report(name):
    sc = bpy.context.scene
    parts = [o for o in sc.objects if o.type == "MESH"]
    tris = 0
    dg = bpy.context.evaluated_depsgraph_get()
    for o in parts:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        me.calc_loop_triangles()
        tris += len(me.loop_triangles)
        ev.to_mesh_clear()
    print("%s: %d parts, %d triangles" % (name, len(parts), tris))


def _aim(d):
    """Euler degrees (game XYZ) that turn the game's +Y onto direction d"""
    import math
    q = Vector((0.0, 1.0, 0.0)).rotation_difference(Vector(d).normalized())
    return [a * 180.0 / math.pi for a in q.to_euler("XYZ")]


def tube(points, radii, mat, parent, sides=8, glow=None, k=1.0, tip=0.0, show=None):
    """A curved horn, tail or chain: a tapering cylinder through game-space points (radii at each),
    a ball at each bend, and (tip > 0) a spike of that length off the end"""
    out = []
    for i in range(len(points) - 1):
        a, b = Vector(points[i]), Vector(points[i + 1])
        d = b - a
        ob = C(tuple((a + b) * 0.5), radii[i], d.length, mat, parent, rot=_aim(d), sides=sides, glow=glow, k=k, r2=radii[i + 1])
        if show:
            set_show(ob, show)
        out.append(ob)
        if 0 < i:
            out.append(E(tuple(a), (radii[i] * 2,) * 3, mat, parent, seg=sides, rings=max(4, sides // 2), glow=glow, k=k, show=show))
    if tip > 0:
        a, b = Vector(points[-2]), Vector(points[-1])
        out.append(K(tuple(b), radii[-1], tip, mat, parent, rot=_aim(b - a), sides=sides, glow=glow, k=k, show=show))
    return out


def aimed(c, d):
    """Euler degrees turning +Y along d (for P/K/C/B placed by hand)"""
    return _aim(d)


def parts_now():
    """The scene's objects now (pass to new_since to get the parts made after)"""
    return set(bpy.context.scene.objects)


def new_since(before):
    return [o for o in bpy.context.scene.objects if o not in before]


def turn_about(obs, at, m):
    """Turn objects about the game-space point `at` by the game-space 4x4 rotation m (e.g. a wing built
    flat, set at its rest angle)"""
    T = Matrix.Translation(Vector(at))
    M = _og.game_to_blender(T @ m @ T.inverted())
    for ob in obs:
        ob.matrix_world = M @ ob.matrix_world


def rot_game(axis, deg):
    return Matrix.Rotation(deg * 3.14159265358979 / 180.0, 4, axis)
