# boss_kit.py — building blocks for detailing the big bosses in Blender (the Gleaner first), on top of
# enemy_kit.py's B / C / E / P / W. A boss piece is a prop file (assets/props/<name>.glb, PropFile.h)
# the boss's rig places on its skeleton; everything here is in that piece's own units, game space
# (+Y up, +Z forward).
#
#   exec(open(".../boss_kit.py").read())          (after enemy_kit.py)
#   begin()                                         clear the scene for the next piece
#   plate(c, s, "gl_rust", rot, rivets=0.9)         an armour plate, bevelled, a row of rivets round it
#   rivets(a, b, n, r)                              a row of rivets from a to b
#   rod(a, b, r, mat)                               a round bar from a to b (a cable, a bone, rebar)
#   cable(points, r, mat)                           a sagging bundle through the points
#   spike(base, dir, length, r, mat)                a tapering spike
#   crack(a, b, w, k)                               a seam of furnace light from a to b (glows with the boss)
#   chain(a, b, links, r)                           hanging links
#   teeth(start, step, n, size)                     a row of teeth
#   wreck(c, size, yaw)                             a rejected unit, wrecked: torso, head, limbs, a still-lit eye
#   save_piece("gleaner_fist")                      to assets/props/, surfaces from the materials (SURF below)
#
# Surfaces: each palette material maps to a surface the boss shader draws (box_inst.frag): gl_rust a
# riveted rusting plate, gl_iron pitted cast iron, gl_bone cracked bone, gl_soot char, gl_heat
# heat-cracked iron with light in its cracks. A part glowing on "glow" takes the boss's fire (x k).
import math

from mathutils import Euler, Vector

_og.PALETTE.update({
    "gl_iron": (0.13, 0.11, 0.10), "gl_rust": (0.34, 0.16, 0.08), "gl_bone": (0.46, 0.41, 0.34),
    "gl_soot": (0.06, 0.045, 0.04), "gl_heat": (0.16, 0.09, 0.07), "gl_ember": (0.12, 0.05, 0.03),
    "gl_steel": (0.22, 0.22, 0.24), "gl_unit": (0.17, 0.18, 0.21),
})
if "const" not in _og.CHANNELS:
    _og.CHANNELS.append("const")
SURF = {"gl_rust": "plate", "gl_iron": "metal", "gl_bone": "marble", "gl_soot": "basalt", "gl_heat": "rock",
        "gl_steel": "metal", "gl_unit": "metal"}
SC = bpy.context.scene


def begin():
    for ob in list(SC.objects):
        bpy.data.objects.remove(ob, do_unlink=True)


def aim(d):
    """The rotation (degrees, game XYZ) that turns +Y along d"""
    v = Vector(d)
    if v.length < 1e-6:
        return (0.0, 0.0, 0.0)
    e = Vector((0.0, 1.0, 0.0)).rotation_difference(v.normalized()).to_euler("XYZ")
    return (math.degrees(e.x), math.degrees(e.y), math.degrees(e.z))


def rivets(a, b, n, r=0.5, mat="gl_steel"):
    a, b = Vector(a), Vector(b)
    for i in range(n):
        p = a.lerp(b, (i + 0.5) / n)
        E(tuple(p), (r * 2, r * 1.2, r * 2), mat, None, seg=8, rings=4)


def plate(c, s, mat="gl_rust", rot=(0, 0, 0), rivets_r=0.0, bev=None):
    """An armour plate: bevelled, and (rivets_r > 0) rivets round its top face's edge"""
    ob = B(c, s, mat, None, rot=rot, bev=bev if bev is not None else min(s) * 0.25, seg=2)
    if rivets_r > 0:
        m = Euler([math.radians(x) for x in rot], "XYZ").to_matrix()
        hx, hy, hz = s[0] * 0.5 - rivets_r * 2.2, s[1] * 0.5, s[2] * 0.5 - rivets_r * 2.2
        corners = [Vector((-hx, hy, -hz)), Vector((hx, hy, -hz)), Vector((hx, hy, hz)), Vector((-hx, hy, hz))]
        for i in range(4):
            a, b = corners[i], corners[(i + 1) % 4]
            n = max(2, int((b - a).length / (rivets_r * 6)))
            rivets(tuple(Vector(c) + m @ a), tuple(Vector(c) + m @ b), n, rivets_r)
    return ob


def rod(a, b, r, mat="gl_iron", sides=8, r2=None, glow=None, k=1.0):
    a, b = Vector(a), Vector(b)
    d = b - a
    return C(tuple((a + b) * 0.5), r, d.length, mat, None, rot=aim(d), sides=sides, r2=r2, glow=glow, k=k)


def cable(points, r, mat="gl_soot", sag=0.0):
    pts = [Vector(p) for p in points]
    for i in range(len(pts) - 1):
        a, b = pts[i], pts[i + 1]
        m = (a + b) * 0.5 - Vector((0, sag, 0))
        rod(a, m, r, mat, sides=6); rod(m, b, r, mat, sides=6)


def spike(base, d, length, r, mat="gl_bone"):
    v = Vector(d).normalized()
    return C(tuple(Vector(base) + v * (length * 0.5)), r, length, mat, None, rot=aim(v), sides=6, r2=r * 0.08)


def crack(a, b, w=0.6, k=1.0):
    """A seam of light: the boss's fire (the rig tints it each frame) x k"""
    a, b = Vector(a), Vector(b)
    d = b - a
    return B(tuple((a + b) * 0.5), (w, w, d.length), "gl_ember", None, rot=aim_z(d), glow="glow", k=k)


def aim_z(d):
    """The rotation that turns +Z along d (for boxes long in z)"""
    v = Vector(d)
    if v.length < 1e-6:
        return (0.0, 0.0, 0.0)
    e = Vector((0.0, 0.0, 1.0)).rotation_difference(v.normalized()).to_euler("XYZ")
    return (math.degrees(e.x), math.degrees(e.y), math.degrees(e.z))


def chain(a, b, links, r=0.4, mat="gl_soot"):
    a, b = Vector(a), Vector(b)
    d = b - a
    for i in range(links):
        p = a.lerp(b, (i + 0.5) / links)
        rot = list(aim_z(d))
        rot[2] += 90.0 * (i % 2)
        B(tuple(p), (r * 0.8, r * 2.2, d.length / links * 1.15), mat, None, rot=tuple(rot), bev=r * 0.3)


def teeth(start, step, n, size, mat="gl_bone", down=True):
    p = Vector(start)
    for i in range(n):
        q = p + Vector(step) * i
        h = size[1] * (0.75 + 0.35 * ((i * 7) % 3) / 2.0)
        W(tuple(q + Vector((0, -h * 0.5 if down else h * 0.5, 0))), (size[0], h, size[2]), mat, None,
          rot=(90 if down else -90, 0, 0), taper=0.25)


def wreck(c, size=1.0, yaw=0.0, lit=True):
    """A rejected unit (UNIT 24's kind), wrecked: what it reclaims and hangs off itself"""
    c = Vector(c)
    m = Euler((0, math.radians(yaw), 0), "XYZ").to_matrix()
    def at(x, y, z):
        return tuple(c + m @ (Vector((x, y, z)) * size))
    def sz(x, y, z):
        return (x * size, y * size, z * size)
    B(at(0, 0, 0), sz(0.7, 0.85, 0.42), "gl_unit", None, rot=(0, yaw, 6), bev=0.06 * size)            # chest
    B(at(0, 0.62, 0.02), sz(0.42, 0.4, 0.4), "gl_unit", None, rot=(14, yaw, -10), bev=0.05 * size)   # head, lolling
    if lit:
        B(at(0.08, 0.64, 0.22), sz(0.22, 0.05, 0.03), "gl_ember", None, rot=(14, yaw, -10), glow="const", k=(0.3, 1.2, 1.6))
    B(at(0, -0.62, 0), sz(0.55, 0.4, 0.34), "gl_steel", None, rot=(0, yaw, -4), bev=0.04 * size)     # hips
    B(at(-0.5, -0.1, 0), sz(0.16, 0.9, 0.18), "gl_unit", None, rot=(0, yaw, 12))                     # an arm, hanging
    B(at(0.22, -1.25, 0.05), sz(0.2, 0.95, 0.2), "gl_unit", None, rot=(8, yaw, -6))                  # a leg (the other gone)
    rod(at(0.38, 0.25, 0), at(0.42, -0.25, 0.05), 0.03 * size, "gl_soot", sides=4)                   # torn wiring


def save_piece(name):
    """To assets/props/<name>.glb: each part's surface from its material (SURF)"""
    for ob in SC.objects:
        if ob.type == "MESH" and ob.data.materials and "glow" not in ob:
            base = ob.data.materials[0].name.split("*")[0]
            if base in SURF:
                ob["tex"] = SURF[base]
    os.makedirs(os.path.join(REPO, "assets", "props"), exist_ok=True)
    path = os.path.join(REPO, "assets", "props", name + ".glb")
    with contextlib.redirect_stdout(io.StringIO()):
        bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", export_extras=True, export_yup=True,
                                  export_apply=True, use_selection=False, use_active_scene=True, export_texcoords=False)
    shrink(path)
    n = sum(1 for o in SC.objects if o.type == "MESH")
    print("%s: %d parts" % (name, n))
