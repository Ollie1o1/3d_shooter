"""Chapter 6's dressing (THE KEPT RING), authored in Blender. Run from the repo root:
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/ring_props_build.py
Dispatch capsules as the ring keeps them: frosted hulls, ribbed, a hatch, one amber lamp each.
  ring_capsule       lying on its side (4.6 m long along +Z, about 1.9 m across)
  ring_capsule_up    driven nose-first into the ice, leaning
  ring_capsule_open  lying, its hatch blown open and empty (yours, at the Landing)
"""
from pathlib import Path
import math
repo = Path.cwd()
exec((repo / 'tools/blender/enemy_kit.py').read_text())
if 'const' not in _og.CHANNELS:
    _og.CHANNELS.append('const')
_og.PALETTE.update({'cap_hull': (.74, .76, .76), 'cap_rib': (.2, .21, .22), 'cap_hatch': (.32, .33, .34),
                    'cap_dark': (.03, .03, .035), 'cap_dust': (.6, .5, .37), 'cap_rime': (.82, .87, .9),
                    'cap_stencil': (.07, .07, .07), 'cap_scorch': (.16, .13, .1)})

def begin():
    for ob in list(bpy.context.scene.objects):
        bpy.data.objects.remove(ob, do_unlink=True)

def save(name):
    for ob in bpy.context.scene.objects:
        if ob.type == 'MESH' and 'glow' not in ob:
            ob['tex'] = 'metal'
    out = repo / 'assets/props' / (name + '.glb')
    bpy.ops.export_scene.gltf(filepath=str(out), export_format='GLB', export_extras=True,
                              export_yup=True, export_apply=True, export_texcoords=False)
    shrink(str(out))
    print('RING PROP:', name)

def capsule(open_=False, root=(0, 0, 0), tilt=0.0):
    """A capsule along +Z centred on root, lying (tilt: degrees about X, nose down)"""
    rx = 90 + tilt
    def at(z, y=0.0):   # a point along its axis
        t = math.radians(tilt)
        return (root[0], root[1] + y * math.cos(t) - z * math.sin(t), root[2] + z * math.cos(t) + y * math.sin(t))
    C(at(0), .95, 3.2, 'cap_hull', None, rot=(rx, 0, 0), sides=20)
    E(at(1.6), (1.9, 1.9, 1.5), 'cap_hull', None, rot=(tilt, 0, 0), seg=20, rings=10)       # the nose
    C(at(-1.75), .9, .5, 'cap_rib', None, rot=(rx, 0, 0), sides=20, r2=.95)                 # the engine collar
    C(at(-2.1), .6, .4, 'cap_dark', None, rot=(rx, 0, 0), sides=14, r2=.8)                  # its bell
    for z in (-1.2, -.4, .4, 1.2):
        C(at(z), .975, .1, 'cap_rib', None, rot=(rx, 0, 0), sides=20)
    # Dust settled on its upper side, rime underneath
    B(at(0, .9), (.9, .08, 3.0), 'cap_dust', None, rot=(tilt, 0, 0))
    B(at(.3, -.9), (.6, .06, 2.0), 'cap_rime', None, rot=(tilt, 0, 0))
    # The hatch on its side (+X), a stencil, the lamp by it
    if open_:
        B((root[0] + .93, root[1] + .05, root[2] + .2), (.06, 1.0, 1.5), 'cap_dark', None)
        B((root[0] + 1.6, root[1] - .55, root[2] + .3), (.08, 1.0, 1.5), 'cap_hatch', None, rot=(0, 0, 70))   # thrown open
        B((root[0] + .9, root[1] + .7, root[2] - .5), (.1, .3, .9), 'cap_scorch', None)
        B((root[0] + .96, root[1] + .7, root[2] + 1.15), (.06, .12, .12), 'cap_rib', None)               # its lamp, dead
    else:
        p = at(.2)
        B((p[0] + .93, p[1] + .05, p[2]), (.08, 1.0, 1.5), 'cap_hatch', None, rot=(tilt, 0, 0))
        q = at(1.15, .7)
        B((q[0] + .93, q[1], q[2]), (.08, .14, .14), 'glow', None, glow='const', k=(2.4, 1.3, .32))
    for k in range(4):
        p = at(-.9 + k * .16, .35)
        B((p[0] + .955, p[1], p[2]), (.02, .1, .1), 'cap_stencil', None, rot=(tilt, 0, 0))

begin(); capsule(); save('ring_capsule')
begin(); capsule(tilt=58.0, root=(0, 1.2, 0)); save('ring_capsule_up')
begin(); capsule(open_=True); save('ring_capsule_open')
