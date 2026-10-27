# OVERDRIVE guns: a Blender add-on for working on assets/guns/*.glb
#
# Install: Blender > Preferences > Add-ons > (the v menu, top right) > Install
# from Disk... > this file, and tick "OVERDRIVE guns". The panel is in the 3D
# view's sidebar (N) > OVERDRIVE.
#
#   Open gun    imports assets/guns/<gun>.glb (each box gets its own mesh),
#               millimetre units, a near clip small enough for 1 mm parts
#   Save gun    checks the scene (joints, boxes, glow and show values), then
#               exports it to that file. ./shooter --viewer <gun> reloads on save
#   Add box     a box at the 3D cursor, in a palette material, under the selection
#   Add joint   an empty named for a joint (crane, pump, muzzle...) at the cursor
#   Glow / Show set or clear the selected boxes' glow channel and show flag
#   Live link   (off until you turn it on) lets Claude edit this scene: it runs
#               the Python files Claude drops in ~/.overdrive_blender/inbox, one
#               undo step each, and writes back what happened. Off again when
#               you turn it off or restart Blender.
#
# The rules the game reads (src/render/GunFile.h has them all):
#   - a gun's meshes are boxes (the game takes their bounds); an enemy's may be any shape
#     (bevelled, round: kept whole), and a mesh that is a box is drawn as one
#   - an object named after a joint is that joint; its children ride it
#   - material "glow" / "glow*0.5" is the gun's glow colour; any other uses its base colour
#   - custom properties: glow (channel), k (strength), phase, show ("loaded", "!bigmag")
#
# Scripts (the live link, or Blender's own console) can use the helpers below,
# which think in GAME space (+Y up, +Z along the barrel, metres), the space the
# game's code and GunKit.h use:  add_box, add_joint, set_glow, set_show, find,
# game_box, describe, open_gun, save_gun, check_scene.
bl_info = {
    "name": "OVERDRIVE guns",
    "author": "OVERDRIVE",
    "version": (1, 1, 0),
    "blender": (4, 0, 0),
    "location": "View3D > Sidebar > OVERDRIVE",
    "description": "Open, edit and save OVERDRIVE's guns (assets/guns/*.glb), and let Claude edit them live",
    "category": "Import-Export",
}

import atexit
import contextlib
import io
import json
import os
import time
import traceback

import bpy
from mathutils import Euler, Matrix, Vector

GUNS = ["revolver", "shotgun", "lancer", "longshot", "sword", "husk", "sentinel", "brute", "ripper", "raptor", "mite"]
# Enemies (assets/enemies/*.glb, src/render/EnemyFile.h): a humanoid's joints. The code turns
# them where the file puts them; anything not under a joint rides the body (bob, crouch)
ENEMIES = ["husk", "sentinel", "brute", "ripper", "raptor", "mite"]
HUMANOID = ["hipL", "hipR", "torso", "head", "armL", "armR"]
RIPPER = ["body", "head", "legFL", "legFR", "legBL", "legBR", "bladeL", "bladeR"]
RAPTOR = ["head", "tail", "wingL", "wingR", "tipL", "tipR"]
MITE = ["body", "legL1", "legL2", "legL3", "legR1", "legR2", "legR3"]
ENEMY_JOINTS = sorted(set(HUMANOID + RIPPER + RAPTOR + MITE))
JOINTS = ["crane", "cylinder", "hammer", "trigger", "needle", "pump", "bolt", "mag", "muzzle", "cells"] + ENEMY_JOINTS
# What each gun's rig and extras need (jointsNeeded() in src/render/GunKit.h)
NEEDED = {
    "revolver": ["crane", "cylinder", "hammer", "trigger", "needle", "muzzle", "cells"],
    "shotgun": ["pump", "muzzle", "cells"],
    "lancer": ["bolt", "muzzle", "cells"],
    "longshot": ["bolt", "mag", "muzzle", "cells"],
    "sword": ["cells"],
    "husk": HUMANOID, "sentinel": HUMANOID, "brute": HUMANOID,
    "ripper": RIPPER, "raptor": RAPTOR, "mite": MITE,
}
CHANNELS = ["const", "glow", "accent", "breath", "stripe", "rim", "needle", "port", "gem", "charge"]
# glow: the enemy's glow, flaring in a wind-up; shot: its shot colour; pulse: a throb (BRUTE's core);
# dive: lit while diving (RAPTOR); fuse: the MITE's core; blade: the RIPPER's blades (src/render/EnemyFile.h)
ENEMY_CHANNELS = ["const", "glow", "shot", "pulse", "dive", "fuse", "blade"]
FLAGS = ["loaded", "!loaded", "bigmag", "!bigmag", "broken", "!broken"]
# The arsenal's palette (GunKit.h): blackened gunmetal, worn steel, edges, bone, brass, and the glow
PALETTE = {
    "black": (0.10, 0.10, 0.11), "steel": (0.32, 0.32, 0.34), "edge": (0.46, 0.46, 0.49),
    "bone": (0.76, 0.71, 0.61), "brass": (0.70, 0.52, 0.24), "glow": (1.0, 0.68, 0.22),
    # the HUSK's: its rust-red plate, dark gunmetal
    "husk": (0.70, 0.26, 0.18), "gunmetal": (0.16, 0.15, 0.17),
    # the other enemies' armour (EnemyTypes.h)
    "sentinel": (0.20, 0.32, 0.62), "brute": (0.58, 0.24, 0.17), "ripper": (0.80, 0.64, 0.14),
    "raptor": (0.40, 0.16, 0.52), "mite": (0.18, 0.30, 0.16),
}
LIVE_DIR = os.environ.get("OVERDRIVE_LIVE_DIR") or os.path.join(os.path.expanduser("~"), ".overdrive_blender")


# ---- game space <-> Blender ------------------------------------------------------
# glTF (the game) is +Y up, +Z forward; Blender's importer turns that into +Z up,
# -Y forward: game (x, y, z) is Blender (x, -z, y)
G2B = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
B2G = G2B.inverted()


def to_blender(v):
    return Vector((v[0], -v[2], v[1]))


def to_game(v):
    return Vector((v[0], v[2], -v[1]))


def game_to_blender(m):
    """A game-space transform as a Blender object's: the same thing, with Blender's own axes
    (an unrotated box or joint stays unrotated, so the rig moves a joint along the axis you'd expect)"""
    return G2B @ m @ B2G


# ---- folders -------------------------------------------------------------------------
def guns_dir(folder):
    """The assets/guns folder, whether Repo points at the repo, at assets, at assets/guns or inside it"""
    d = os.path.normpath(bpy.path.abspath(folder or "."))
    for _ in range(4):
        if os.path.basename(d) == "guns" and os.path.basename(os.path.dirname(d)) == "assets":
            return d
        if os.path.isdir(os.path.join(d, "assets", "guns")):
            return os.path.join(d, "assets", "guns")
        if os.path.basename(d) == "assets" and os.path.isdir(os.path.join(d, "guns")):
            return os.path.join(d, "guns")
        d = os.path.dirname(d)
    return os.path.join(os.path.normpath(bpy.path.abspath(folder or ".")), "assets", "guns")


def gun_path(scene, gun=None):
    gun = gun or scene.overdrive_gun
    d = guns_dir(scene.overdrive_repo)
    if gun in ENEMIES:   # assets/enemies, beside assets/guns
        d = os.path.join(os.path.dirname(d), "enemies")
    return os.path.join(d, gun + ".glb")


# ---- building blocks (data API only: they work from the live link, scripts, anywhere) ----
def palette_material(name):
    """A material by palette name ("steel", "black*0.6", "glow*0.5"); made if it isn't there"""
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    base = name.split("*")[0]
    try:
        shade = float(name.split("*")[1]) if "*" in name else 1.0
    except ValueError:
        shade = 1.0
    rgb = PALETTE.get(base, (0.5, 0.5, 0.5))
    col = (min(1.0, rgb[0] * shade), min(1.0, rgb[1] * shade), min(1.0, rgb[2] * shade), 1.0)
    if bsdf:
        bsdf.inputs["Base Color"].default_value = col
        bsdf.inputs["Roughness"].default_value = 1.0
    m.diffuse_color = col
    return m


def _unit_cube(name):
    me = bpy.data.meshes.new(name)
    v = [(x, y, z) for x in (-0.5, 0.5) for y in (-0.5, 0.5) for z in (-0.5, 0.5)]
    f = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    me.from_pydata(v, [], f)
    me.update()
    return me


def _place(ob, parent, world):
    if parent is not None:
        ob.parent = parent
    ob.matrix_world = world


def find(name):
    """An object by name (None if there isn't one)"""
    return bpy.data.objects.get(name)


def add_box(center, size, material="black", parent=None, rot=(0.0, 0.0, 0.0), name=None, collection=None):
    """A box in game space: centre (x, y, z), size (along x, y, z), rot in degrees about the
    game's x, y, z (applied in that order). parent: an object or its name (a joint, usually)."""
    if isinstance(parent, str):
        parent = find(parent)
    ob = bpy.data.objects.new(name or material, _unit_cube(name or material))
    ob.data.materials.append(palette_material(material))
    (collection or bpy.context.scene.collection).objects.link(ob)
    g = Matrix.Translation(Vector(center)) @ Euler([r * 3.14159265358979 / 180.0 for r in rot], "XYZ").to_matrix().to_4x4()
    g = g @ Matrix.Diagonal((size[0], size[1], size[2], 1.0))
    _place(ob, parent, game_to_blender(g))
    return ob


def add_joint(name, at=(0.0, 0.0, 0.0), parent=None):
    """A joint (an empty named for it) at a game-space point"""
    if name not in JOINTS:
        raise ValueError("'%s' isn't a joint: %s" % (name, ", ".join(JOINTS)))
    if find(name):
        raise ValueError("there's already a '%s'" % name)
    if isinstance(parent, str):
        parent = find(parent)
    ob = bpy.data.objects.new(name, None)
    ob.empty_display_type = "ARROWS"
    ob.empty_display_size = 0.01
    bpy.context.scene.collection.objects.link(ob)
    _place(ob, parent, game_to_blender(Matrix.Translation(Vector(at))))
    return ob


def set_glow(ob, channel=None, k=1.0, phase=None):
    """Glow on a channel (None: stop glowing); k a number or an (r, g, b)"""
    if isinstance(ob, str):
        ob = find(ob)
    for key in ("glow", "k", "phase"):
        if key in ob:
            del ob[key]
    if channel is None:
        return
    if channel not in CHANNELS + ENEMY_CHANNELS:
        raise ValueError("'%s' isn't a glow channel: %s" % (channel, ", ".join(CHANNELS + ENEMY_CHANNELS)))
    ob["glow"] = channel
    if isinstance(k, (tuple, list)):
        ob["k"] = [float(x) for x in k]
    elif abs(k - 1.0) > 1e-6:
        ob["k"] = float(k)
    if phase is not None:
        ob["phase"] = float(phase)


def set_show(ob, flag=None):
    """Show only while a flag holds ("loaded", "!bigmag"...); None: always"""
    if isinstance(ob, str):
        ob = find(ob)
    if "show" in ob:
        del ob["show"]
    if flag is None:
        return
    if flag not in FLAGS:
        raise ValueError("'%s' isn't a flag: %s" % (flag, ", ".join(FLAGS)))
    ob["show"] = flag


def game_box(ob):
    """A box's centre and size in game space (as the game will see it)"""
    m = B2G @ ob.matrix_world @ G2B
    if ob.type != "MESH" or not ob.data.vertices:
        return Vector(m.translation), Vector((0, 0, 0))
    lo = to_game(Vector([min(v.co[i] for v in ob.data.vertices) for i in range(3)]))
    hi = to_game(Vector([max(v.co[i] for v in ob.data.vertices) for i in range(3)]))
    lo, hi = Vector([min(lo[i], hi[i]) for i in range(3)]), Vector([max(lo[i], hi[i]) for i in range(3)])
    c = m @ ((lo + hi) * 0.5)
    axes = m.to_3x3()
    size = Vector([(axes.col[i] * (hi[i] - lo[i])).length for i in range(3)])
    return c, size


def describe(limit=None):
    """Every object as text, in game space: name, parent, centre, size, material, glow, show"""
    lines = []
    obs = sorted(bpy.context.scene.objects, key=lambda o: o.name)
    for ob in obs[:limit] if limit else obs:
        c, s = game_box(ob)
        mat = ob.material_slots[0].material.name if ob.type == "MESH" and ob.material_slots and ob.material_slots[0].material else ""
        extra = " ".join("%s=%s" % (k, list(ob[k]) if hasattr(ob[k], "to_list") else ob[k]) for k in ("glow", "k", "phase", "show") if k in ob)
        kind = "joint" if ob.name in JOINTS else ob.type.lower()
        lines.append("%-14s %-6s parent=%-10s at (%.4f %.4f %.4f) size (%.4f %.4f %.4f) %s %s" % (
            ob.name, kind, ob.parent.name if ob.parent else "-", c.x, c.y, c.z, s.x, s.y, s.z, mat, extra))
    return "\n".join(lines)


def _is_box(ob):
    """Every vertex on a corner of the mesh's bounds (the game sees only the bounds)"""
    vs = ob.data.vertices
    if not vs:
        return False
    lo = [min(v.co[i] for v in vs) for i in range(3)]
    hi = [max(v.co[i] for v in vs) for i in range(3)]
    eps = 1e-5 + 1e-4 * max(hi[i] - lo[i] for i in range(3))
    return all(all(abs(v.co[i] - lo[i]) < eps or abs(v.co[i] - hi[i]) < eps for i in range(3)) for v in vs)


def check_scene(gun):
    """What the game would object to (errors) or quietly get wrong (warnings)"""
    errors, warnings = [], []
    names = {ob.name for ob in bpy.context.scene.objects}
    for j in NEEDED.get(gun, []):
        if j not in names:
            near = [n for n in names if n.split(".")[0] == j]
            errors.append("no '%s' joint%s" % (j, " (there's a '%s': rename it '%s')" % (near[0], j) if near else ""))
    odd, glow, show = [], [], []
    for ob in bpy.context.scene.objects:
        if ob.name.split(".")[0] in JOINTS and ob.name not in JOINTS:
            warnings.append("'%s' looks like a copied joint: it won't move (only '%s' does)" % (ob.name, ob.name.split(".")[0]))
        if ob.type == "MESH" and not _is_box(ob):
            odd.append(ob.name)
        if "glow" in ob and ob["glow"] not in (ENEMY_CHANNELS if gun in ENEMIES else CHANNELS):
            glow.append("%s (%s)" % (ob.name, ob["glow"]))
        if "show" in ob and ob["show"] not in FLAGS:
            show.append("%s (%s)" % (ob.name, ob["show"]))
        if ob.type not in ("MESH", "EMPTY"):
            warnings.append("'%s' (a %s) isn't used by the game" % (ob.name, ob.type.lower()))
    if glow:
        errors.append("unknown glow channel on " + ", ".join(glow[:5]) + " (channels: " + " ".join(ENEMY_CHANNELS if gun in ENEMIES else CHANNELS) + ")")
    if show:
        errors.append("unknown show flag on " + ", ".join(show[:5]) + " (flags: " + " ".join(FLAGS) + ")")
    if odd and gun not in ENEMIES:   # (an enemy keeps its meshes whole: src/render/MeshLib.h)
        warnings.append("%d mesh(es) aren't boxes; a gun is drawn from their bounds (so far): %s" % (len(odd), ", ".join(odd[:5])))
    return errors, warnings


def open_gun(gun=None, folder=None):
    """Clear the scene and import a gun"""
    s = bpy.context.scene
    if folder:
        s.overdrive_repo = folder
    if gun:
        s.overdrive_gun = gun
    path = gun_path(s)
    if not os.path.exists(path):
        raise FileNotFoundError("No file at " + path + " (set the Repo folder)")
    for ob in list(s.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    for coll in (bpy.data.meshes, bpy.data.materials):
        for d in list(coll):
            if d.users == 0:
                coll.remove(d)
    before = set(bpy.data.objects)
    with contextlib.redirect_stdout(io.StringIO()):   # (the importer's log: noise in a live link's reply)
        bpy.ops.import_scene.gltf(filepath=path)
    # The file shares one cube mesh per material: give each box its own, so editing one never changes the rest
    for ob in set(bpy.data.objects) - before:
        if ob.type == "MESH" and ob.data.users > 1:
            ob.data = ob.data.copy()
    s.unit_settings.length_unit = "MILLIMETERS"
    s["overdrive_opened"] = s.overdrive_gun   # which gun this scene is (Save won't write it over another)
    return path


def save_gun(gun=None):
    """Check, then export; returns (path, warnings). Raises on errors (the file is left alone)."""
    s = bpy.context.scene
    gun = gun or s.overdrive_gun
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")   # (edits made in Edit Mode only reach the mesh in Object Mode)
    errors, warnings = check_scene(gun)
    opened = s.get("overdrive_opened")
    if opened and opened != gun:
        errors.insert(0, "this scene is the %s (opened from %s.glb), but Gun says %s: set Gun back to %s to save it, "
                         "or Open %s first" % (opened, opened, gun, opened, gun))
    if errors:
        raise ValueError("Not saved: " + "; ".join(errors))
    path = gun_path(s, gun)
    with contextlib.redirect_stdout(io.StringIO()):
        bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", export_extras=True,
                                  export_yup=True, export_apply=True, use_selection=False,
                                  use_active_scene=True, export_texcoords=False)   # (only this scene: another may hold a different gun)
    return path, warnings


# ---- the live link -------------------------------------------------------------------
_live = {"on": False, "last": "", "count": 0}


def _live_dirs():
    inbox, outbox = os.path.join(LIVE_DIR, "inbox"), os.path.join(LIVE_DIR, "outbox")
    for d in (LIVE_DIR, inbox, outbox):
        os.makedirs(d, mode=0o700, exist_ok=True)
    return inbox, outbox


def _view3d_override():
    wm = bpy.context.window_manager
    for win in (wm.windows if wm else []):
        for area in win.screen.areas:
            if area.type == "VIEW_3D":
                region = next((r for r in area.regions if r.type == "WINDOW"), None)
                return {"window": win, "area": area, "region": region}
    return {}


def run_live_file(path):
    """Run one script from the inbox; returns the reply written to the outbox"""
    name = os.path.splitext(os.path.basename(path))[0]
    with open(path, encoding="utf-8") as f:
        code = f.read()
    os.remove(path)
    out = io.StringIO()
    reply = {"id": name, "ok": True, "error": "", "time": time.time()}
    env = {"bpy": bpy, "Vector": Vector, "Matrix": Matrix}
    env.update({k: v for k, v in globals().items() if k in (
        "add_box", "add_joint", "set_glow", "set_show", "find", "game_box", "describe", "open_gun",
        "save_gun", "check_scene", "palette_material", "to_blender", "to_game", "JOINTS", "CHANNELS",
        "FLAGS", "PALETTE", "NEEDED")})
    try:
        with contextlib.redirect_stdout(out), bpy.context.temp_override(**_view3d_override()):
            exec(compile(code, name + ".py", "exec"), env)
    except Exception:
        reply["ok"] = False
        reply["error"] = traceback.format_exc(limit=4)
    reply["output"] = out.getvalue()
    try:
        bpy.ops.ed.undo_push(message="Claude: " + name)
    except Exception:
        pass
    _live["count"] += 1
    _live["last"] = name + (" (ok)" if reply["ok"] else " (error)")
    tmp = os.path.join(_live_dirs()[1], name + ".json.part")
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(reply, f)
    os.replace(tmp, tmp[:-5])
    return reply


def _live_tick():
    if not _live["on"]:
        return None
    try:
        inbox, _ = _live_dirs()
        for fn in sorted(os.listdir(inbox)):
            if fn.endswith(".py"):
                run_live_file(os.path.join(inbox, fn))
        for area in (bpy.context.screen.areas if bpy.context.screen else []):
            if area.type == "VIEW_3D":
                area.tag_redraw()
    except Exception:
        _live["last"] = "link error: " + traceback.format_exc(limit=1).strip().splitlines()[-1]
    return 0.3


def live_on():
    _live_dirs()
    with open(os.path.join(LIVE_DIR, "status.json"), "w", encoding="utf-8") as f:
        json.dump({"on": True, "pid": os.getpid(), "since": time.time()}, f)
    _live["on"] = True
    if not bpy.app.timers.is_registered(_live_tick):
        bpy.app.timers.register(_live_tick, first_interval=0.1, persistent=True)
    atexit.register(live_off)   # (quitting Blender turns it off: no stale "on" left behind)


def live_off():
    _live["on"] = False
    if bpy.app.timers.is_registered(_live_tick):
        bpy.app.timers.unregister(_live_tick)
    with contextlib.suppress(OSError):
        os.remove(os.path.join(LIVE_DIR, "status.json"))


# ---- operators and the panel ---------------------------------------------------------
def _near_clip(context):
    for area in (context.screen.areas if context.screen else []):
        if area.type == "VIEW_3D":
            for space in area.spaces:
                if space.type == "VIEW_3D":
                    space.clip_start = 0.0005


class OVERDRIVE_OT_open(bpy.types.Operator):
    bl_idname = "overdrive.open_gun"
    bl_label = "Open gun"
    bl_description = "Replace this scene with assets/guns/<gun>.glb"

    def execute(self, context):
        try:
            path = open_gun()
        except FileNotFoundError as e:
            self.report({"ERROR"}, str(e))
            return {"CANCELLED"}
        _near_clip(context)
        self.report({"INFO"}, "Opened " + path)
        return {"FINISHED"}

    def invoke(self, context, event):
        if any(ob.type == "MESH" for ob in context.scene.objects):
            try:
                return context.window_manager.invoke_confirm(self, event, title="Open gun",
                                                             message="This replaces everything in the scene. Save first if you need to.")
            except TypeError:   # (Blender before 4.1: no title/message)
                return context.window_manager.invoke_confirm(self, event)
        return self.execute(context)


class OVERDRIVE_OT_save(bpy.types.Operator):
    bl_idname = "overdrive.save_gun"
    bl_label = "Save gun"
    bl_description = "Check the scene, then export it to assets/guns/<gun>.glb (the viewer and the game read it)"

    def execute(self, context):
        try:
            path, warnings = save_gun()
        except ValueError as e:
            self.report({"ERROR"}, str(e))
            return {"CANCELLED"}
        for w in warnings:
            self.report({"WARNING"}, w)
        self.report({"INFO"}, "Saved " + path)
        return {"FINISHED"}


class OVERDRIVE_OT_add_box(bpy.types.Operator):
    bl_idname = "overdrive.add_box"
    bl_label = "Add box"
    bl_description = "A 1 cm box at the 3D cursor, in the chosen material, under the selected object"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        parent = context.active_object
        mat = context.scene.overdrive_material
        ob = add_box(to_game(context.scene.cursor.location), (0.01, 0.01, 0.01), mat, parent=parent)
        for o in context.selected_objects:
            o.select_set(False)
        ob.select_set(True)
        context.view_layer.objects.active = ob
        return {"FINISHED"}


class OVERDRIVE_OT_add_joint(bpy.types.Operator):
    bl_idname = "overdrive.add_joint"
    bl_label = "Add joint"
    bl_description = "An empty named for a joint, at the 3D cursor, under the selection"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        try:
            ob = add_joint(context.scene.overdrive_joint, to_game(context.scene.cursor.location), parent=context.active_object)
        except ValueError as e:
            self.report({"ERROR"}, str(e))
            return {"CANCELLED"}
        for o in context.selected_objects:
            o.select_set(False)
        ob.select_set(True)
        context.view_layer.objects.active = ob
        return {"FINISHED"}


class OVERDRIVE_OT_glow(bpy.types.Operator):
    bl_idname = "overdrive.set_glow"
    bl_label = "Set glow"
    bl_description = "The selected boxes glow on this channel, this strongly"
    bl_options = {"REGISTER", "UNDO"}
    clear: bpy.props.BoolProperty(default=False)

    def execute(self, context):
        s = context.scene
        for ob in context.selected_objects:
            if ob.type == "MESH":
                set_glow(ob, None if self.clear else s.overdrive_channel, s.overdrive_k)
        return {"FINISHED"}


class OVERDRIVE_OT_show(bpy.types.Operator):
    bl_idname = "overdrive.set_show"
    bl_label = "Set show"
    bl_description = "The selected boxes show only while this flag holds"
    bl_options = {"REGISTER", "UNDO"}
    clear: bpy.props.BoolProperty(default=False)

    def execute(self, context):
        for ob in context.selected_objects:
            if ob.type == "MESH":
                set_show(ob, None if self.clear else context.scene.overdrive_flag)
        return {"FINISHED"}


class OVERDRIVE_OT_live(bpy.types.Operator):
    bl_idname = "overdrive.live_link"
    bl_label = "Live link"
    bl_description = "Let Claude edit this scene: runs the scripts Claude drops in ~/.overdrive_blender/inbox (each one undoable)"

    def execute(self, context):
        if _live["on"]:
            live_off()
            self.report({"INFO"}, "Live link off")
        else:
            live_on()
            self.report({"INFO"}, "Live link on: Claude can edit this scene")
        return {"FINISHED"}


class OVERDRIVE_PT_panel(bpy.types.Panel):
    bl_label = "OVERDRIVE guns"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "OVERDRIVE"

    def draw(self, context):
        s = context.scene
        col = self.layout.column()
        col.prop(s, "overdrive_repo")
        col.prop(s, "overdrive_gun")
        row = col.row()
        row.operator("overdrive.open_gun")
        row.operator("overdrive.save_gun")
        col.separator()
        col.prop(s, "overdrive_material")
        col.operator("overdrive.add_box")
        col.prop(s, "overdrive_joint")
        col.operator("overdrive.add_joint")
        col.separator()
        col.prop(s, "overdrive_channel")
        col.prop(s, "overdrive_k")
        row = col.row()
        row.operator("overdrive.set_glow", text="Glow").clear = False
        row.operator("overdrive.set_glow", text="No glow").clear = True
        col.prop(s, "overdrive_flag")
        row = col.row()
        row.operator("overdrive.set_show", text="Show on flag").clear = False
        row.operator("overdrive.set_show", text="Always").clear = True
        col.separator()
        box = col.box()
        on = _live["on"]
        box.operator("overdrive.live_link", text="Live link: ON (click to stop)" if on else "Live link: off (click to let Claude edit)",
                     icon="LINKED" if on else "UNLINKED", depress=on)
        if on:
            box.label(text="%d edit(s) from Claude" % _live["count"])
            if _live["last"]:
                box.label(text="last: " + _live["last"])
        ob = context.active_object
        if ob:
            col.separator()
            col.label(text=ob.name + ("  (joint)" if ob.name in JOINTS else ""))
            for key in ("glow", "k", "phase", "show"):
                if key in ob:
                    v = ob[key]
                    col.label(text="  %s = %s" % (key, v.to_list() if hasattr(v, "to_list") else v))


CLASSES = (OVERDRIVE_OT_open, OVERDRIVE_OT_save, OVERDRIVE_OT_add_box, OVERDRIVE_OT_add_joint,
           OVERDRIVE_OT_glow, OVERDRIVE_OT_show, OVERDRIVE_OT_live, OVERDRIVE_PT_panel)
PROPS = ("overdrive_repo", "overdrive_gun", "overdrive_material", "overdrive_joint",
         "overdrive_channel", "overdrive_k", "overdrive_flag")


def register():
    T = bpy.types.Scene
    T.overdrive_repo = bpy.props.StringProperty(name="Repo", subtype="DIR_PATH",
                                                description="The 3d_shooter folder (or its assets/guns folder)")
    T.overdrive_gun = bpy.props.EnumProperty(name="Gun", items=[(g, g, "") for g in GUNS])
    T.overdrive_material = bpy.props.EnumProperty(name="Material", items=[(m, m, "") for m in PALETTE])
    T.overdrive_joint = bpy.props.EnumProperty(name="Joint", items=[(j, j, "") for j in JOINTS])
    T.overdrive_channel = bpy.props.EnumProperty(name="Channel", items=[(c, c, "") for c in CHANNELS])
    T.overdrive_k = bpy.props.FloatProperty(name="Strength (k)", default=1.0, min=0.0, soft_max=4.0)
    T.overdrive_flag = bpy.props.EnumProperty(name="Flag", items=[(f, f, "") for f in FLAGS])
    for c in CLASSES:
        bpy.utils.register_class(c)


def unregister():
    live_off()
    for c in reversed(CLASSES):
        bpy.utils.unregister_class(c)
    for p in PROPS:
        delattr(bpy.types.Scene, p)


if __name__ == "__main__":
    register()
