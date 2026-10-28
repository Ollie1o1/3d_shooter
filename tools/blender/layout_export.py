# The scene's OD_LAYOUT collection out to assets/levels/<level>.json (LevelLayout.h puts it in the game).
#
#   In Blender (after level_import.py):  Scripting > open this file > Run Script
#   Or headless:  blender -b build/levels/act1.blend --python tools/blender/layout_export.py
#
# The level is the scene's od_level (level_import.py sets it). Each object in OD_LAYOUT is written by its
# od_kind (level_import.py lists them): a mesh as its box (its bounds as placed: keep boxes turned by
# multiples of 90 degrees, the game's are upright), an empty as a spawn or a placed prop.
# Anything it can't write is listed and nothing is saved.
import json
import os
import sys

import bpy
from mathutils import Matrix, Vector

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
C = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
CI = C.inverted()
MATS = ["brick", "panel", "rock", "metal", "concrete", "adobe", "plate", "marble", "circuit", "basalt", "wood",
        "limestone", "brass"]
BOX_KINDS = ("wall", "prop", "neon", "destructible")


def to_game(v):
    return [round(v[0], 4), round(v[2], 4), round(-v[1], 4)]


def kind_of(ob):
    k = ob.get("od_kind")
    if k:
        return str(k)
    n = ob.name.lower()
    for pre in ("spawn_ground", "spawn_air", "prop", "neon", "wall"):
        if n.startswith(pre):
            return pre
    if n.startswith("destructible:"):
        return "destructible"
    if n.startswith("model:"):
        return "model"
    return "wall" if ob.type == "MESH" else None


def colour_of(ob):
    if "od_color" in ob:
        return [round(float(c), 4) for c in ob["od_color"]][:3]
    if ob.type == "MESH" and ob.data.materials and ob.data.materials[0]:
        c = ob.data.materials[0].diffuse_color
        return [round(c[0], 4), round(c[1], 4), round(c[2], 4)]
    return None


def export(level=None):
    sc = bpy.context.scene
    level = level or sc.get("od_level")
    if not level:
        raise SystemExit("no od_level on the scene: import a level with level_import.py first")
    col = bpy.data.collections.get("OD_LAYOUT")
    if col is None:
        raise SystemExit("no OD_LAYOUT collection")
    bpy.context.view_layer.update()
    objects, problems = [], []
    for ob in sorted(col.objects, key=lambda o: o.name):
        if ob.parent is not None and ob.parent.name in col.objects:
            continue   # (a placed prop's parts, a destructible's look)
        k = kind_of(ob)
        if k is None:
            continue
        o = {"kind": k}
        if k in BOX_KINDS:
            if ob.type != "MESH":
                problems.append("%s: a %s must be a mesh" % (ob.name, k))
                continue
            pts = [ob.matrix_world @ Vector(c) for c in ob.bound_box]
            g = [to_game(p) for p in pts]
            o["min"] = [min(p[i] for p in g) for i in range(3)]
            o["max"] = [max(p[i] for p in g) for i in range(3)]
            if k != "neon":
                m = str(ob.get("od_mat", "brick"))
                if m not in MATS:
                    problems.append("%s: od_mat '%s' isn't one of %s" % (ob.name, m, " ".join(MATS)))
                    continue
                o["mat"] = m
            c = colour_of(ob)
            if c:
                o["color"] = c
            if k == "destructible":
                o["name"] = str(ob.get("od_name", ""))
                o["hp"] = float(ob.get("od_hp", 300))
                if not o["name"]:
                    problems.append("%s: a destructible needs od_name (its prop)" % ob.name)
                    continue
        elif k in ("spawn_ground", "spawn_air"):
            o["pos"] = to_game(ob.matrix_world.translation)
            if "od_arena" in ob:
                o["arena"] = int(ob["od_arena"])
        elif k == "model":
            name = str(ob.get("od_name", ob.name.split(":", 1)[-1]))
            m = CI @ ob.matrix_world @ C
            o["name"] = name
            o["xf"] = [round(m[r][c], 5) for c in range(4) for r in range(4)]   # (column major, like glm)
        else:
            problems.append("%s: unknown od_kind '%s'" % (ob.name, k))
            continue
        objects.append(o)
    if problems:
        for p in problems:
            print("layout_export:", p)
        raise SystemExit("nothing saved: %d problem(s)" % len(problems))
    path = os.path.join(REPO, "assets", "levels", level + ".json")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write('{"objects": [\n')
        f.write(",\n".join("  " + json.dumps(o) for o in objects))
        f.write("\n]}\n")
    print("layout_export: %d objects -> %s" % (len(objects), path))
    return path


if __name__ == "__main__" or __name__ == "<run_path>":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    export(args[0] if args else None)
