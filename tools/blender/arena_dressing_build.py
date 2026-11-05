"""ARENA, ACT II and FAST dressing and far distance, authored in Blender. Run from the repo root:
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/arena_dressing_build.py [ONLY=name]
Exports assets/props/<name>.glb, editable scenes in build/arena-dressing/.

Everything here is static: the level places it (LevelBuilder::place) and it is baked into the world mesh,
so a copy costs nothing per frame beyond its triangles (and only when its chunk is in view: WorldMesh.h).
Parts are still joined by look before export so the files stay small. Self-lit parts carry their own
colour (k = r, g, b); every other part names its surface (`tex`, a Mat), so nothing depends on the level.

ACT I
  yard_palm         THE SUNSET YARD: a date palm, 10 m, its trunk curving, ringed bark, drooping fronds
  yard_mesa         THE SUNSET YARD: a desert butte on the horizon, stepped strata, talus at its foot (~70 m)
  yard_festoon      THE SUNSET YARD: strings of lamps from the obelisk out to the four rooftops, and across
                    the four arms (built in place: stand it at the dais' centre)
  foundry_chain     THE FOUNDRY: 5 m of heavy chain and a crane hook
  wall_pipes        THE FOUNDRY, THE PUMPWORKS: 16 m of pipes along a high wall (few sides: seen from below)
  foundry_truss     THE FOUNDRY: a roof truss spanning the hall (66 m along X), a lamp hanging every 16 m
  spire_peak        THE SPIRE: a mountain, snow on its shoulders (~40 m tall at scale 1)
  core_tower_a/b    THE CORE: dead towers of the city around the reactor, a few windows still lit
  sanctum_colossus  THE SANCTUM: a kneeling knight, hands on the pommel of a sword driven into the ground
  sanctum_spire     THE SANCTUM: a black gothic spire, buttressed, a red band near its top
ACT II
  descent_cage      THE DESCENT: a gibbet cage on its chain, hung down the shaft wall
  zenith_ruin       !!!!!: a broken marble colonnade adrift on a rock, far out in the red sky
  maw_tooth         THE MAW: a great curved fang, half sunk, round the pool
  nave_chandelier   THE DROWNED NAVE: a wheel of candles slung on cables between the nave's walls (x +-28.4)
"""
from pathlib import Path
import math
import random

repo = Path.cwd()
exec((repo / 'tools/blender/enemy_kit.py').read_text())
REPO = str(repo)
if 'const' not in _og.CHANNELS:
    _og.CHANNELS.append('const')
_og.PALETTE.update({
    'trunk': (.30, .17, .14), 'trunkdark': (.20, .11, .10), 'frond': (.14, .16, .09), 'frond2': (.10, .12, .07),
    'date': (.45, .22, .08),
    'mesa': (.62, .36, .26), 'mesadark': (.46, .25, .19), 'mesapale': (.74, .50, .36),
    'wire': (.06, .05, .05), 'post': (.36, .22, .16),
    'iron': (.30, .28, .27), 'irondark': (.17, .16, .16), 'hookrust': (.42, .23, .14),
    'peak': (.33, .36, .45), 'peakdark': (.26, .28, .36), 'snowcap': (.88, .91, .97),
    'tower': (.07, .08, .11), 'towerdark': (.04, .05, .07), 'towertrim': (.12, .14, .18),
    'colossus': (.10, .08, .09), 'colossus2': (.15, .12, .13),
    'spire': (.07, .06, .07),
    'cage': (.18, .16, .15),
    'marble': (.80, .76, .72), 'marbledark': (.55, .50, .48), 'islandrock': (.30, .16, .15),
    'fang': (.72, .68, .58), 'fangroot': (.32, .14, .13),
})
# Each palette name's surface in the game (a Mat name, Collision.h); others take the prop's default
TEX = {'trunk': 'wood', 'trunkdark': 'wood', 'frond': 'wood', 'frond2': 'wood', 'date': 'wood',
       'mesa': 'rock', 'mesadark': 'rock', 'mesapale': 'adobe', 'wire': 'metal', 'post': 'wood',
       'iron': 'metal', 'irondark': 'metal', 'hookrust': 'plate',
       'peak': 'rock', 'peakdark': 'rock', 'snowcap': 'snow',
       'tower': 'concrete', 'towerdark': 'concrete', 'towertrim': 'metal',
       'colossus': 'basalt', 'colossus2': 'basalt', 'spire': 'basalt',
       'cage': 'metal', 'marble': 'marble', 'marbledark': 'marble', 'islandrock': 'rock',
       'fang': 'limestone', 'fangroot': 'rock'}
ONLY = globals().get('ONLY')


def want(name):
    return not ONLY or ONLY == name


def begin():
    for ob in list(bpy.context.scene.objects):
        bpy.data.objects.remove(ob, do_unlink=True)


def join_by_look():
    groups = {}
    for ob in list(bpy.context.scene.objects):
        if ob.type != 'MESH':
            continue
        for m in ob.modifiers:
            bpy.context.view_layer.objects.active = ob
            bpy.ops.object.modifier_apply(modifier=m.name)
        k = ob.get('k', 1.0)
        k = tuple(round(float(x), 3) for x in k) if hasattr(k, '__len__') else round(float(k), 3)
        key = (ob.data.materials[0].name if ob.data.materials else '', ob.get('glow'), k, ob.get('tex'))
        groups.setdefault(key, []).append(ob)
    for obs in groups.values():
        if len(obs) < 2:
            continue
        bpy.ops.object.select_all(action='DESELECT')
        for o in obs:
            o.select_set(True)
        bpy.context.view_layer.objects.active = obs[0]
        bpy.ops.object.join()


def save(name, default='metal'):
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH' and 'glow' not in ob:
            material = ob.data.materials[0].name.split('*')[0] if ob.data.materials else ''
            ob['tex'] = TEX.get(material, default)
    join_by_look()
    report(name)
    out = repo / 'assets/props' / (name + '.glb')
    bpy.ops.export_scene.gltf(filepath=str(out), export_format='GLB', export_extras=True,
                              export_yup=True, export_apply=True, export_texcoords=False)
    shrink(str(out))
    source = repo / 'build/arena-dressing' / (name + '.blend')
    source.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source))


def lamp(c, r, col):
    """A self-lit bulb (a small ball)"""
    return E(c, (r * 2, r * 2.4, r * 2), 'wire', None, seg=4, rings=3, glow='const', k=col)


def strand(a, b, sag, step, cols, rng, r=.07):
    """A wire hung from a to b, sagging `sag` at its middle, lamps every `step` metres along it"""
    a, b = Vector(a), Vector(b)
    n = max(4, int((b - a).length / 1.6))
    pts = []
    for i in range(n + 1):
        t = i / n
        p = a.lerp(b, t)
        p.y -= sag * 4 * t * (1 - t)
        pts.append(p)
    for i in range(n):
        d = pts[i + 1] - pts[i]
        C(tuple((pts[i] + pts[i + 1]) * .5), .025, d.length, 'wire', None, rot=aimed(None, d), sides=3)
    count = max(2, int((b - a).length / step))
    for i in range(1, count):
        t = i / count
        p = a.lerp(b, t)
        p.y -= sag * 4 * t * (1 - t) + .16
        lamp(tuple(p), r, cols[(i + rng.randrange(2)) % len(cols)])


# ---- THE SUNSET YARD: a date palm --------------------------------------------------------------
if want('yard_palm'):
    begin()
    rng = random.Random(11)
    H = 10.0
    pts, radii = [], []
    for i in range(8):
        t = i / 7
        pts.append((1.6 * t * t, H * t, .25 * math.sin(t * 2.5)))       # the trunk bends as it rises
        radii.append(.36 - .14 * t)
    tube(pts, radii, 'trunk', None, sides=7)
    E((0, .25, 0), (1.1, .6, 1.1), 'trunkdark', None, seg=9, rings=4)     # its flared foot
    for i in range(8):                                                     # the bark's rings
        t = (i + .5) / 8
        y = H * t * .97
        x = 1.6 * t * t
        z = .25 * math.sin(t * 2.5)
        R((x, y, z), .36 - .14 * t, .05, 'trunkdark', None, rot=(90, 0, 0), sides=7, tsides=3)
    top = Vector((1.6, H, .25 * math.sin(2.5)))
    E(tuple(top + Vector((0, .2, 0))), (.9, .8, .9), 'trunkdark', None, seg=8, rings=5)   # the crown
    for k in range(6):                                                     # clusters of dates
        a = k * math.tau / 6 + .3
        E(tuple(top + Vector((math.cos(a) * .45, -.35, math.sin(a) * .45))), (.32, .4, .32), 'date', None, seg=5, rings=3)
    N = 11
    for f in range(N):                                                     # fronds: arched, drooping, tapering
        a = f * math.tau / N + rng.uniform(-.15, .15)
        lift = rng.uniform(.35, .9) if f % 3 else rng.uniform(-.1, .2)
        L = rng.uniform(4.0, 5.2)
        d = Vector((math.cos(a), 0, math.sin(a)))
        p = top + Vector((0, .25, 0))
        segs = 5
        for s in range(segs):
            t = s / segs
            ang = lift - 1.6 * t * t                                        # up, then over and down
            step = L / segs
            q = p + d * (step * math.cos(ang)) + Vector((0, step * math.sin(ang), 0))
            mid = (p + q) * .5
            w = .95 * (1 - .7 * t)
            seg_dir = q - p
            pitch = math.atan2(seg_dir.y, Vector((seg_dir.x, 0, seg_dir.z)).length)
            # a flat blade: long along its run (local x), wide across it, thin; pitched, then turned to its heading
            M = Matrix.Translation(mid) @ rot_game('Y', -math.degrees(a)) @ rot_game('Z', math.degrees(pitch))
            rib = P((0, 0, 0), (step * 1.05, .08, .09), 'trunkdark', None)
            rib.matrix_world = _og.game_to_blender(M)
            for side in (-1, 1):                                            # leaflets either side, hanging
                for j in range(1):
                    lf = P((0, 0, 0), (step * .95, .04, .9 * w), 'frond' if (f + side) % 2 else 'frond2', None, top=(1.0, .35))
                    lf.matrix_world = _og.game_to_blender(M @ Matrix.Translation((0, -.12, side * .45 * w))
                                                          @ rot_game('Y', side * 28) @ rot_game('X', side * 38))
            p = q
    save('yard_palm', 'wood')

# ---- THE SUNSET YARD: a butte on the horizon ---------------------------------------------------
if want('yard_mesa'):
    begin()
    rng = random.Random(5)
    W, D, H = 70.0, 44.0, 34.0
    y = 0.0
    layers = [(.42, 'mesadark'), (.22, 'mesa'), (.08, 'mesapale'), (.18, 'mesa'), (.1, 'mesadark')]
    for i, (h, col) in enumerate(layers):
        hh = H * h
        shrink_ = 1.0 - .12 * i
        for k in range(3):                                                   # each stratum: a few eroded blocks
            w = W * shrink_ * rng.uniform(.55, .75)
            d = D * shrink_ * rng.uniform(.6, .85)
            x = rng.uniform(-W * .12, W * .12) * shrink_
            z = rng.uniform(-D * .1, D * .1)
            P((x, y + hh * .5, z), (w, hh, d), col, None, rot=(0, rng.uniform(-14, 14), 0),
              top=(rng.uniform(.86, .97), rng.uniform(.86, .97)), bev=min(1.6, hh * .3))
        y += hh
    P((W * .1, y + 1.5, 0), (W * .34, 3.0, D * .38), 'mesadark', None, rot=(0, 8, 0), top=(.8, .85), bev=.8)   # the cap
    for k in range(26):                                                      # talus: fallen blocks round its foot
        a = rng.uniform(0, math.tau)
        r = rng.uniform(.42, .6)
        s = rng.uniform(2.5, 7.5)
        B((math.cos(a) * W * r, s * .35, math.sin(a) * D * r), (s, s * .7, s * rng.uniform(.7, 1.2)), 'mesadark' if k % 2 else 'mesa', None,
          rot=(rng.uniform(-20, 20), rng.uniform(0, 90), rng.uniform(-20, 20)), bev=.5)
    for k in range(5):                                                       # a hoodoo standing apart
        hx = W * .62
        B((hx, 3 + k * 4, -D * .2), (5 - k * .6, 4, 5 - k * .5), 'mesa' if k % 2 else 'mesadark', None, rot=(0, k * 17, 0), bev=.6)
    save('yard_mesa', 'rock')

# ---- THE SUNSET YARD: strings of lamps -----------------------------------------------------------
# In place round the dais (0, 0, 0): the obelisk's cap (7.6 m) out to a post on each rooftop's inner
# corner (+-19, 5, +-19), and across the mouth of each arm between those posts
if want('yard_festoon'):
    begin()
    rng = random.Random(9)
    warm, pink, cyan, amber = (1.25, .72, .32), (1.15, .3, .7), (.3, 1.0, 1.15), (1.3, .5, .15)
    for sx in (-1, 1):
        for sz in (-1, 1):
            px, pz = sx * 19.4, sz * 19.4
            C((px, 5.9, pz), .07, 1.8, 'post', None, sides=6)                 # a post on the roof's corner
            B((px, 6.75, pz), (.5, .05, .05), 'post', None, rot=(0, 45 * sx * sz, 0))
            strand((sx * .5, 7.45, sz * .5), (px, 6.75, pz), 1.1, 1.25, (warm, pink, amber), rng)
    for sx, sz, ex, ez in ((-1, -1, 1, -1), (-1, 1, 1, 1), (-1, -1, -1, 1), (1, -1, 1, 1)):
        strand((sx * 19.4, 6.75, sz * 19.4), (ex * 19.4, 6.75, ez * 19.4), 1.6, 1.4, (warm, cyan, warm, pink), rng)
    save('yard_festoon', 'metal')

# ---- THE FOUNDRY: chain and hook -----------------------------------------------------------------
if want('foundry_chain'):
    begin()
    L = 5.0
    n = int(L / .4)
    for i in range(n):                                                       # links, each turned a quarter from the last
        y = L - .2 - i * .4
        R((0, y, 0), .17, .045, 'iron', None, rot=(0, 90 * (i % 2), 90), sides=6, tsides=3)
    C((0, .5, 0), .12, .5, 'irondark', None, sides=8)                       # the hook's block
    B((0, .74, 0), (.3, .1, .3), 'irondark', None)
    R((0, -.05, 0), .32, .07, 'hookrust', None, rot=(0, 0, 200), arc=250, sides=9, tsides=4)   # the hook
    K((-.27, .08, 0), .07, .22, 'hookrust', None, rot=(0, 0, 30), sides=6)
    save('foundry_chain', 'metal')

# ---- THE FOUNDRY (and the Pumpworks): a long pipe run for high walls ----------------------------
# 16 m along X, the wall at z 0, standing off toward +Z. Seen from well below: no bolts, few sides
if want('wall_pipes'):
    begin()
    Lp = 16.0
    B((0, .5, .03), (Lp, .1, .06), 'irondark', None)                        # the wall rail
    for x in (-6.0, -2.0, 2.0, 6.0):                                         # brackets
        B((x, .5, .22), (.08, 1.0, .38), 'iron', None)
    for x in (-8.0, 0.0, 8.0):
        B((x, .5, .22), (.08, 1.0, .38), 'iron', None)
    C((0, .25, .38), .17, Lp, 'iron', None, rot=(0, 0, 90), sides=8)          # the main
    C((0, .62, .3), .1, Lp, 'hookrust', None, rot=(0, 0, 90), sides=6)        # a steam line
    C((0, .88, .16), .05, Lp, 'irondark', None, rot=(0, 0, 90), sides=4)      # conduit
    for x in (-4.0, 4.0):
        C((x, .25, .38), .22, .12, 'irondark', None, rot=(0, 0, 90), sides=8) # flanges
    C((-4.0, .25, .64), .035, .3, 'iron', None, rot=(90, 0, 0), sides=4)      # a valve, its wheel
    R((-4.0, .25, .8), .15, .025, 'hookrust', None, sides=8, tsides=3)
    save('wall_pipes', 'metal')

# ---- THE FOUNDRY: a roof truss ---------------------------------------------------------------------
# Spans X -33..33 (the hall's walls), 1.7 m deep: its top chord at the roof (y 0 is its bottom chord)
if want('foundry_truss'):
    begin()
    S, D = 66.0, 1.7
    B((0, D, 0), (S, .3, .35), 'irondark', None)                            # chords
    B((0, 0, 0), (S, .26, .3), 'irondark', None)
    n = 22
    for i in range(n + 1):
        x = -S / 2 + S * i / n
        B((x, D / 2, 0), (.16, D, .2), 'iron', None)                         # posts
        if i < n:
            xm = x + S / n / 2
            d = Vector((S / n, D * (1 if i % 2 else -1), 0))
            B((xm, D / 2, 0), (d.length, .12, .14), 'iron', None, rot=(0, 0, math.degrees(math.atan2(d.y, d.x))))   # web
    for x in (-S / 2 + .6, S / 2 - .6):                                      # bearing plates on the walls
        B((x, D * .5, 0), (1.2, D + .6, .7), 'irondark', None)
    for x in (-24.0, -8.0, 8.0, 24.0):                                       # hanging lamps
        C((x, -.7, 0), .02, 1.4, 'irondark', None, sides=4)
        K((x, -1.75, 0), .55, .4, 'irondark', None, rot=(180, 0, 0), sides=10)
        C((x, -1.42, 0), .45, .3, 'irondark', None, r2=.12, sides=10)
        E((x, -1.62, 0), (.5, .22, .5), 'irondark', None, seg=8, rings=4, glow='const', k=(1.35, .78, .4))
    save('foundry_truss', 'metal')

# ---- THE SPIRE: a mountain ----------------------------------------------------------------------
if want('spire_peak'):
    begin()
    import bmesh as _bm

    def crag(name, mat, R, H, seed, cut=0.0, grow=0.0, rough=1.0):
        # a cone of rings, each vertex pushed out and about by noise: ridges, gullies, a crooked summit
        rng = random.Random(seed)
        me = bpy.data.meshes.new(mat)
        bm = _bm.new()
        rings, segs = 9, 14
        wob = [rng.uniform(-1, 1) for _ in range(segs)]
        top = (rng.uniform(-.12, .12) * R, rng.uniform(-.12, .12) * R)
        grid = []
        for i in range(rings):
            t = i / (rings - 1)
            if t < cut:
                continue
            ring = []
            for j in range(segs):
                a = j * math.tau / segs
                ridge = 1 + .3 * wob[j] * rough + .16 * rng.uniform(-1, 1) * rough
                r = (R * (1 - t) ** 1.6 * ridge) + grow
                x = math.cos(a) * r + top[0] * t
                z = math.sin(a) * r + top[1] * t
                y = H * t + rng.uniform(-.04, .04) * H * (1 - t) * rough
                ring.append(bm.verts.new((x, -z, y)))
            grid.append(ring)
        apex = bm.verts.new((top[0], -top[1], H + grow * .6))
        for i in range(len(grid) - 1):
            for j in range(segs):
                bm.faces.new((grid[i][j], grid[i][(j + 1) % segs], grid[i + 1][(j + 1) % segs], grid[i + 1][j]))
        for j in range(segs):
            bm.faces.new((grid[-1][j], grid[-1][(j + 1) % segs], apex))
        bm.faces.new(list(reversed(grid[0])))
        _bm.ops.recalc_face_normals(bm, faces=bm.faces)
        bm.to_mesh(me)
        bm.free()
        ob = bpy.data.objects.new(mat, me)
        ob.data.materials.append(palette_material(mat))
        bpy.context.scene.collection.objects.link(ob)
        return ob

    H = 40.0
    crag('rock', 'peakdark', 25, H, 3, rough=1.4)
    crag('snow', 'snowcap', 25, H, 3, rough=1.4, cut=.62, grow=.35)       # the same noise: the snow sits on the rock
    b2 = crag('rock2', 'peak', 15, H * .62, 8, rough=1.4)                             # a lower shoulder off to one side
    b2.location = (13, -5, 0)
    s2 = crag('snow2', 'snowcap', 15, H * .62, 8, rough=1.4, cut=.7, grow=.3)
    s2.location = (13, -5, 0)
    save('spire_peak', 'rock')

# ---- THE CORE: dead towers of the city ------------------------------------------------------------
def tower(seed, tiers, name):
    begin()
    rng = random.Random(seed)
    lit = [(.2, .85, 1.0), (.2, .85, 1.0), (1.0, .25, .75), (.9, .75, .5)]
    y = 0.0
    for (w, d, h) in tiers:
        B((0, y + h / 2, 0), (w, h, d), 'tower', None)
        B((0, y + h - .2, 0), (w + .4, .4, d + .4), 'towertrim', None)          # each setback's cornice
        for x in (-w / 2, w / 2):                                                # corner piers
            for z in (-d / 2, d / 2):
                B((x, y + h / 2, z), (.6, h, .6), 'towerdark', None)
        floors = int(h / 3.2)
        for f in range(1, floors):                                               # windows: a few rows still lit
            fy = y + f * 3.2
            for face in range(4):
                along = w if face < 2 else d
                n = max(2, int(along / 1.6))
                for i in range(n):
                    if rng.random() > .2:
                        continue
                    u = -along / 2 + along * (i + .5) / n
                    run = 1 + (rng.random() < .3)
                    c = lit[rng.randrange(len(lit))]
                    k = tuple(v * rng.uniform(.5, 1.0) for v in c)
                    if face < 2:
                        B((u, fy, (d / 2 + .03) * (1 if face == 0 else -1)), (1.1 * run, 1.3, .06), 'wire', None, glow='const', k=k)
                    else:
                        B(((w / 2 + .03) * (1 if face == 2 else -1), fy, u), (.06, 1.3, 1.1 * run), 'wire', None, glow='const', k=k)
        y += h
    w, d, _ = tiers[-1]
    for k in range(3):                                                           # plant on the roof
        B((rng.uniform(-w / 3, w / 3), y + .9, rng.uniform(-d / 3, d / 3)), (rng.uniform(1.2, 2.5), 1.8, rng.uniform(1.2, 2.5)), 'towertrim', None)
    C((w * .2, y + 6, 0), .15, 12, 'towertrim', None, sides=6)                   # a mast and its warning light
    for yy in (y + 4, y + 8):
        B((w * .2, yy, 0), (1.4, .1, .1), 'towertrim', None)
    E((w * .2, y + 12.2, 0), (.5, .5, .5), 'wire', None, seg=6, rings=4, glow='const', k=(1.6, .12, .1))
    save(name, 'concrete')


if want('core_tower_a'):
    tower(31, [(10, 9, 22), (8, 7, 12), (5.5, 5, 8)], 'core_tower_a')
if want('core_tower_b'):
    tower(32, [(13, 11, 14), (11, 10, 10), (9, 8, 6)], 'core_tower_b')

# ---- THE SANCTUM: a kneeling colossus ------------------------------------------------------------
# Faces +Z (toward the arena), ~31 m to the top of its hood
if want('sanctum_colossus'):
    begin()
    c1, c2 = 'colossus', 'colossus2'
    for s_ in (-1, 1):
        x = s_ * 2.7
        P((x, 1.4, -.8), (3.0, 2.8, 7.6), c1, None, top=(.9, .9), bev=.3)                     # shin, on the ground
        P((x, .9, -4.6), (2.6, 1.8, 2.2), c2, None, rot=(-50, 0, 0), bev=.2)                  # foot, toes down
        E((x, 2.6, 3.0), (3.5, 3.4, 3.4), c2, None, seg=10, rings=6)                          # knee cop
        P((x, 4.6, .4), (3.4, 3.0, 6.6), c1, None, rot=(-14, 0, 0), top=(1.0, .95), bev=.3)   # thigh, resting on it
    P((0, 6.6, -1.6), (8.0, 3.4, 5.0), c2, None, bev=.4)                                      # hips
    for k in range(3):
        P((0, 6.2 - k * .9, 1.0 + k * .5), (7.6 - k * .5, .9, 1.0), c1, None, rot=(30, 0, 0), bev=.12)   # tassets over the thighs
    P((0, 12.3, -.8), (8.4, 8.6, 5.6), c1, None, rot=(10, 0, 0), top=(1.2, 1.06), bev=.6)    # chest, leaning in
    P((0, 12.0, 1.9), (5.4, 5.0, .9), c2, None, rot=(10, 0, 0), top=(.65, 1.0), bev=.3)      # breastplate ridge
    for s_ in (-1, 1):
        E((s_ * 5.6, 16.0, -.2), (4.6, 4.0, 5.2), c2, None, seg=10, rings=6)                  # pauldron
        for k in range(3):
            P((s_ * (6.3 + .3 * k), 15.0 - k * 1.1, -.1), (3.6, .6, 5.0 - .4 * k), c1, None, rot=(0, 0, -s_ * 28), bev=.12)
        tube([(s_ * 5.6, 15.2, .4), (s_ * 4.8, 10.6, 3.6), (s_ * 1.3, 9.6, 6.4)], [1.25, 1.05, .9], c1, None, sides=8)   # the arm, down to the pommel
        E((s_ * 4.8, 10.6, 3.6), (2.0, 2.0, 2.0), c2, None, seg=8, rings=6)                   # couter
        E((s_ * 1.05, 9.7, 6.7), (2.1, 2.0, 2.3), c2, None, seg=8, rings=6)                   # gauntlet
    P((0, 17.3, .2), (3.4, 1.8, 3.4), c2, None, bev=.2)                                       # gorget
    E((0, 19.0, 1.6), (4.0, 4.6, 4.4), c1, None, rot=(28, 0, 0), seg=12, rings=8)            # the hood, bowed
    K((0, 20.4, .0), 2.0, 3.6, c1, None, rot=(-55, 0, 0), sides=8)                            # its peak falls back
    E((0, 18.2, 3.3), (2.4, 2.2, 1.0), 'wire', None, seg=8, rings=5)                          # the dark under it
    for s_ in (-1, 1):
        B((s_ * .55, 18.3, 3.78), (.6, .2, .1), 'wire', None, glow='const', k=(1.7, .2, .1))   # eyes
    P((0, 9.0, -4.0), (9.6, 16.0, 1.1), c2, None, rot=(-6, 0, 0), top=(.72, 1.0), bev=.3)   # the cloak down its back
    P((0, .5, -5.2), (11.0, 1.0, 3.6), c2, None, bev=.3)                                     # its hem, pooled
    P((0, 3.8, 6.8), (1.4, 9.0, .45), c2, None, top=(.6, 1.0), bev=.08)                       # blade, point driven in
    B((0, 8.5, 6.8), (5.4, .7, .9), c1, None, bev=.12)                                        # crossguard
    C((0, 9.6, 6.8), .36, 1.6, c1, None, sides=8)                                             # grip (the hands cover it)
    E((0, 10.7, 6.8), (1.3, 1.3, 1.3), c2, None, seg=8, rings=6)                             # pommel
    B((0, 4.6, 7.06), (.18, 6.5, .06), 'wire', None, glow='const', k=(1.2, .16, .08))          # a red fuller
    rng = random.Random(4)
    for k in range(10):                                                                       # the ground cracked round it
        a = k * math.tau / 10
        B((math.cos(a) * 7.8, .25, math.sin(a) * 7.0), (rng.uniform(2.5, 4.5), .7, rng.uniform(1.8, 3.0)), c2, None,
          rot=(rng.uniform(-6, 6), math.degrees(a), rng.uniform(4, 12)), bev=.2)
    save('sanctum_colossus', 'basalt')

# ---- THE SANCTUM: a gothic spire -------------------------------------------------------------------
if want('sanctum_spire'):
    begin()
    H = 46.0
    P((0, H * .2, 0), (7, H * .4, 7), 'spire', None, top=(.86, .86), bev=.3)
    P((0, H * .5, 0), (5.6, H * .2, 5.6), 'spire', None, top=(.8, .8), bev=.25)
    for s in range(4):                                                         # buttress fins
        a = s * math.pi / 2
        dx, dz = math.cos(a), math.sin(a)
        P((dx * 4.4, H * .16, dz * 4.4), (1.0 if dz == 0 else 3.4, H * .32, 3.4 if dz == 0 else 1.0), 'spire', None,
          top=(.3 if dz == 0 else 1.0, 1.0 if dz == 0 else .3), shift=(-dx * 1.2, -dz * 1.2), bev=.1)
        K((dx * 3.2, H * .6, dz * 3.2), .7, 5.5, 'spire', None, sides=4)          # pinnacles
        B((dx * 2.82, H * .38, dz * 2.82), (.9 if dz == 0 else .1, 4.0, .1 if dz == 0 else .9), 'wire', None,
          glow='const', k=(.9, .1, .06))                                          # lancet windows, lit from inside
    B((0, H * .6, 0), (5.0, .5, 5.0), 'spire', None)
    K((0, H * .6, 0), 2.6, H * .42, 'spire', None, sides=8)                     # the spire itself
    for k in (0, 1):
        R((0, H * (.66 + .1 * k), 0), 2.0 - .6 * k, .12, 'wire', None, rot=(90, 0, 0), sides=12, tsides=3,
          glow='const', k=(1.25, .18, .1))                                        # red bands near the top
    save('sanctum_spire', 'basalt')

# ---- THE DESCENT: a gibbet cage on its chain -------------------------------------------------------
if want('descent_cage'):
    begin()
    L = 6.0
    for i in range(int(L / .42)):
        R((0, 2.6 + L - .21 - i * .42, 0), .17, .045, 'cage', None, rot=(0, 90 * (i % 2), 90), sides=6, tsides=3)
    C((0, 2.5, 0), .55, .12, 'cage', None, sides=12)                         # its cap
    K((0, 2.55, 0), .55, .4, 'cage', None, sides=12)
    for k in range(10):                                                       # bars, bellied out
        a = k * math.tau / 10
        tube([(math.cos(a) * .5, 2.5, math.sin(a) * .5), (math.cos(a) * .72, 1.3, math.sin(a) * .72),
              (math.cos(a) * .45, .05, math.sin(a) * .45)], [.04, .04, .04], 'cage', None, sides=3)
    for y in (.05, 1.3):
        R((0, y, 0), .45 if y < .5 else .72, .045, 'cage', None, rot=(90, 0, 0), sides=14, tsides=4)
    C((0, .02, 0), .45, .06, 'cage', None, sides=12)
    E((0, .3, 0), (.5, .4, .45), 'fangroot', None, seg=6, rings=4)            # what's left inside
    save('descent_cage', 'metal')

# ---- !!!!!: a ruin adrift --------------------------------------------------------------------------
if want('zenith_ruin'):
    begin()
    rng = random.Random(41)
    K((0, 0, 0), 14, 20, 'islandrock', None, rot=(180, 0, 0), sides=7)         # the rock it stands on, hanging under
    for k in range(6):
        a = k * math.tau / 6
        K((math.cos(a) * 7, -1, math.sin(a) * 6), rng.uniform(3, 5), rng.uniform(6, 12), 'islandrock', None, rot=(180, 0, 0), sides=5)
    P((0, .6, 0), (24, 1.6, 20), 'marbledark', None, top=(.96, .96), bev=.3)    # the floor
    for i in range(7):                                                          # a colonnade, some fallen, some broken off
        x = -10 + i * 3.3
        h = 12 if i in (1, 2, 5) else rng.uniform(3, 8)
        C((x, 1.4 + h / 2, -6), .85, h, 'marble', None, sides=12)
        B((x, 1.6, -6), (2.2, .5, 2.2), 'marbledark', None)
        if h > 10:
            B((x, 1.4 + h + .35, -6), (2.4, .7, 2.4), 'marbledark', None)
    B((-5.0, 14.1, -6), (7.6, 1.2, 2.4), 'marble', None)                        # the lintel still on two of them
    C((4.5, 2.1, 1.5), .85, 9, 'marble', None, rot=(0, 25, 90), sides=12)       # one fallen
    for k in range(10):
        B((rng.uniform(-10, 10), 1.6, rng.uniform(-8, 8)), (rng.uniform(1, 2.5), rng.uniform(.6, 1.2), rng.uniform(1, 2.5)), 'marble', None,
          rot=(rng.uniform(-15, 15), rng.uniform(0, 90), rng.uniform(-15, 15)))
    B((7, 1.6 + 2.2, 5), (.2, 4.4, 3.0), 'wire', None, glow='const', k=(1.6, 1.25, .7))   # light through a doorway
    B((7, 1.6 + 2.2, 3.2), (.9, 4.6, .6), 'marble', None)
    B((7, 1.6 + 2.2, 6.8), (.9, 4.6, .6), 'marble', None)
    B((7, 6.4, 5), (.9, .6, 4.2), 'marble', None)
    save('zenith_ruin', 'marble')

# ---- THE MAW: a great fang ---------------------------------------------------------------------------
if want('maw_tooth'):
    begin()
    pts = [(0, -1, 0), (0, 3, .4), (.2, 6.5, 1.6), (.3, 9.5, 3.6), (.2, 11.5, 5.8)]
    tube(pts, [1.5, 1.35, 1.05, .65, .3], 'fang', None, sides=10, tip=1.4)
    E((0, .2, 0), (4.0, 1.8, 3.6), 'fangroot', None, seg=10, rings=5)         # its gum, torn
    for k in range(5):
        a = k * math.tau / 5
        B((math.cos(a) * 1.8, .4, math.sin(a) * 1.6), (1.2, .8, 1.0), 'fangroot', None, rot=(0, math.degrees(a), 20))
    save('maw_tooth', 'limestone')

# ---- THE DROWNED NAVE: a chandelier ---------------------------------------------------------------
if want('nave_chandelier'):
    begin()
    # a short chain up to a ring, slung from the two side walls (x +-28.4, 9.5 m above the wheel) on cables
    for i in range(int(3.0 / .45)):
        R((0, 3.0 + 3.0 - .22 - i * .45, 0), .18, .05, 'iron', None, rot=(0, 90 * (i % 2), 90), sides=6, tsides=3)
    R((0, 6.15, 0), .3, .07, 'iron', None, sides=10, tsides=4)
    for sx in (-1, 1):
        pts = []
        for i in range(13):
            t = i / 12
            pts.append((sx * 28.4 * t, 6.3 + 3.2 * t - 1.6 * 4 * t * (1 - t), 0))
        tube(pts, [.05] * len(pts), 'irondark', None, sides=4)
        B((sx * 28.6, 9.5, 0), (.4, .7, .5), 'iron', None)                     # its bracket in the wall
    for k in range(4):                                                        # four drops to the wheel
        a = k * math.pi / 2 + math.pi / 4
        tube([(0, 3.0, 0), (math.cos(a) * 2.3, .9, math.sin(a) * 2.3)], [.03, .03], 'irondark', None, sides=3)
    E((0, 3.0, 0), (.45, .6, .45), 'iron', None, seg=6, rings=4)
    R((0, .8, 0), 2.4, .1, 'iron', None, rot=(90, 0, 0), sides=20, tsides=4)    # the wheel
    R((0, .8, 0), 1.4, .07, 'irondark', None, rot=(90, 0, 0), sides=14, tsides=3)
    for k in range(6):
        a = k * math.tau / 6
        B((math.cos(a) * 1.9, .8, math.sin(a) * 1.9), (1.0, .06, .06), 'irondark', None, rot=(0, -math.degrees(a), 0))
    K((0, .7, 0), .35, 1.2, 'iron', None, rot=(180, 0, 0), sides=6)            # its finial
    for k in range(12):                                                       # candles, guttered, some out
        a = k * math.tau / 12
        x, z = math.cos(a) * 2.4, math.sin(a) * 2.4
        h = .3 + .25 * ((k * 7) % 3) / 2
        C((x, .9 + h / 2, z), .07, h, 'fang', None, sides=5)
        if k % 4 != 3:
            K((x, .92 + h, z), .06, .2, 'wire', None, sides=4, glow='const', k=(1.5, .85, .38))
    save('nave_chandelier', 'metal')
