# Chapter 6's crossing enemies (docs/chapter-6-design.md section 7): THE DISPATCHED.
# Run with
#   blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/ring_foes_build.py [ONLY=stray]
#
# Chapter 3's subjects were soft: flesh, gowns, ovals, a hospital's palette. These are the
# opposite on purpose. Everything here was shipped and has sat in hard vacuum for years:
#
#   - hard shells, chamfered and faceted, never round flesh
#   - sun-bleached enamel on the lit side, sand-coloured ring dust ground into every lower
#     surface, a skin of rime in the seams
#   - shipping hardware worn as if it were anatomy: cradle frames, foam blocks, webbing
#     straps, cargo tags, stencilled serials
#   - one warm light each: a visor slit (pale) and, on whatever still carries part of its
#     capsule, the capsule's amber lamp. No cyan (UNIT 24's alone), no red (the countdown's)
#
#   stray   a unit that climbed out of its capsule: lean, upright, still wearing the back
#           half of its packing cradle with the foam strapped in, its delivery tag on a cord
#   herder  a pusher drone off the reef: a squat block behind a broad plough plate with a
#           rubber bumper, cold-gas pods on swivel arms, one hooded search lamp
#   tally   a counting mast bolted into the reef on three spiked legs: abacus rods of beads,
#           a slowly turning counter drum, a lamp on top that blinks each count
#   porter  a heavy hauler that never put its load down: a dispatch capsule strapped upright
#           on its back, loader forearms with shovel hands for scooping ice to throw, head sunk
#           between its shoulders under the weight
#   crook   THE CROOK, the Shepherd's first sheepdog (design sections 3 and 6): a tall, thin
#           retrieval unit, stooped, its right arm one long pole ending in a hook for pulling
#           strays back into line; a cable winch on its back, the hook's line wound on it
#   drover  THE DROVER, the second sheepdog: a long, heavy tug that flies its herd of ice chunks
#           across the Gap, two push-forks out in front, four cold-gas pods, a mast at the back
#           with a hooded lamp and the herd's amber beacon
import os, math
exec(open(os.path.join(os.getcwd(), 'tools/blender/enemy_kit.py')).read())

_og.PALETTE.update({
    'ring_enamel': (.86, .84, .78),      # sun-bleached shell
    'ring_enamel_dk': (.62, .6, .55),
    'ring_dust': (.6, .5, .37),          # ring dust ground into the lower half
    'ring_dust_dk': (.38, .31, .23),
    'ring_graphite': (.11, .11, .12),    # actuators, joints, inner frame
    'ring_rubber': (.05, .05, .05),
    'ring_rime': (.78, .84, .88),        # frost in the seams
    'ring_foam': (.7, .7, .66),          # packing foam
    'ring_strap': (.24, .23, .17),       # webbing
    'ring_buckle': (.55, .56, .55),
    'ring_tag': (.9, .86, .62),          # the delivery tag
    'ring_stencil': (.07, .07, .07),
    'ring_plough': (.78, .66, .38),      # the herder's scuffed plate, like a snowplough's
    'ring_capsule': (.74, .76, .76),     # a dispatch capsule's frosted hull
    'ring_hatch': (.3, .31, .32),
    'ring_bead': (.82, .8, .72),
})
for _j in ['elbowL', 'elbowR', 'handL', 'handR', 'kneeL', 'kneeR', 'waist', 'halo', 'shield', 'wingL', 'wingR', 'body', 'crown']:
    if _j not in _og.JOINTS:
        _og.JOINTS.append(_j)

NEEDED['stray'] = 'waist torso head hipL hipR kneeL kneeR armL armR elbowL elbowR handL handR halo'
NEEDED['herder'] = 'body head shield wingL wingR'
NEEDED['tally'] = 'body head halo crown armL armR'
NEEDED['crook'] = 'waist torso head hipL hipR kneeL kneeR armL armR elbowL elbowR handL handR halo shield'
NEEDED['drover'] = 'body head shield wingL wingR'
NEEDED['porter'] = 'waist torso head hipL hipR kneeL kneeR armL armR elbowL elbowR handL handR shield'


def begin(name):
    sc = bpy.context.scene
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    sc.name = name.upper(); sc['overdrive_opened'] = name


def done(name):
    save_enemy(name)
    os.makedirs(os.path.join(REPO, 'build', 'ring-models'), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(REPO, 'build', 'ring-models', name + '.blend'))


def rod(a, b, r, mat, parent, sides=8, **kw):
    """A cylinder from a to b"""
    d = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
    n = math.sqrt(sum(x * x for x in d))
    return C(tuple((a[i] + b[i]) * .5 for i in range(3)), r, n, mat, parent, rot=_aim(d), sides=sides, **kw)


def stencil(c, w, h, parent, rot=(0, 0, 0), n=4):
    """A stencilled serial: a short row of dark blocks on a panel"""
    for k in range(n):
        B((c[0] + (k - (n - 1) / 2) * w * 1.35, c[1], c[2]), (w, h, .006), 'ring_stencil', parent, rot=rot)


def strap(a, b, w, parent, t=.02):
    """Webbing from a to b (flat), with a buckle in the middle"""
    d = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
    n = math.sqrt(sum(x * x for x in d))
    mid = tuple((a[i] + b[i]) * .5 for i in range(3))
    B(mid, (w, n, t), 'ring_strap', parent, rot=_aim(d))
    B(mid, (w * 1.3, w * .9, t * 2.2), 'ring_buckle', parent, rot=_aim(d))


def want(n):
    return not globals().get('ONLY') or ONLY == n


# =====================================================================================
# STRAY (2.0 m): lean and upright, the cradle on its back
# =====================================================================================
if want('stray'):
    begin('stray')
    add_joint('waist', (0, 1.05, 0)); add_joint('torso', (0, 1.25, 0), 'waist'); add_joint('head', (0, 1.72, .02), 'torso')
    add_joint('halo', (0, 1.4, -.2), 'torso')   # the cradle frame: it springs on its mounts
    for sd, s in (('L', 1), ('R', -1)):
        add_joint('hip' + sd, (s * .13, 1.0, 0), 'waist'); add_joint('knee' + sd, (s * .14, .55, .02), 'hip' + sd)
        add_joint('arm' + sd, (s * .3, 1.58, 0), 'torso'); add_joint('elbow' + sd, (s * .36, 1.25, .01), 'arm' + sd)
        add_joint('hand' + sd, (s * .38, .93, .04), 'elbow' + sd)

    # Pelvis and spine: graphite actuators, an enamel hip shell
    P((0, 1.02, 0), (.36, .18, .22), 'ring_enamel_dk', 'waist', top=(.85, .9), bev=.025, seg=2)
    B((0, .96, .1), (.2, .1, .04), 'ring_dust', 'waist', bev=.01)
    for k in range(3):
        C((0, 1.12 + k * .055, -.02), .055 - k * .004, .04, 'ring_graphite', 'waist', sides=10)
    # Chest: a faceted shell, narrow at the waist, broad at the shoulders; one shoulder plate gone
    P((0, 1.4, .01), (.36, .34, .24), 'ring_enamel', 'torso', top=(1.25, 1.0), bev=.03, seg=2)
    P((0, 1.42, .135), (.24, .2, .03), 'ring_enamel_dk', 'torso', top=(1.3, 1), bev=.01)   # sternum plate
    stencil((.06, 1.47, .152), .022, .035, 'torso')
    B((0, 1.25, .06), (.26, .07, .18), 'ring_graphite', 'torso', bev=.01)                 # the waist seal
    B((.21, 1.55, 0), (.14, .08, .2), 'ring_enamel', 'torso', bev=.025, seg=2, rot=(0, 0, -14))   # its left pauldron
    C((-.24, 1.56, 0), .065, .1, 'ring_graphite', 'torso', rot=(0, 0, 90), sides=10)      # the right one torn off: bare actuator
    B((-.2, 1.5, .02), (.05, .12, .03), 'ring_rime', 'torso')
    # Webbing across the chest, buckled: it was strapped into its cradle and never unstrapped
    strap((.17, 1.56, .13), (-.15, 1.28, .13), .045, 'torso')
    strap((-.17, 1.56, .13), (.15, 1.28, .13), .045, 'torso')
    # Neck and head: a chamfered helmet, a single pale visor slit, a ridge, a tag on a cord
    C((0, 1.66, 0), .055, .1, 'ring_graphite', 'torso', sides=10)
    P((0, 1.8, .02), (.22, .22, .25), 'ring_enamel', 'head', top=(.82, .78), bev=.04, seg=3)
    B((0, 1.81, .145), (.17, .032, .02), 'glow', 'head', glow='eye', k=1.4)
    B((0, 1.81, .138), (.21, .07, .02), 'ring_graphite', 'head')
    B((0, 1.92, .0), (.03, .03, .22), 'ring_enamel_dk', 'head')
    B((.105, 1.79, -.02), (.02, .1, .14), 'ring_dust_dk', 'head')
    B((-.105, 1.79, -.02), (.02, .1, .14), 'ring_dust_dk', 'head')
    rod((0, 1.66, .07), (.03, 1.52, .17), .004, 'ring_stencil', 'torso', sides=4)
    B((.035, 1.48, .175), (.06, .085, .006), 'ring_tag', 'torso', rot=(8, 0, -10))
    B((.035, 1.485, .179), (.035, .01, .004), 'ring_stencil', 'torso', rot=(8, 0, -10))

    # The cradle: two rails up its back, crossbars, foam blocks still strapped in, its capsule's lamp on top
    for s in (1, -1):
        B((s * .17, 1.42, -.22), (.035, .74, .04), 'ring_graphite', 'halo', bev=.008)
        B((s * .17, 1.78, -.22), (.05, .04, .05), 'ring_enamel_dk', 'halo')
    for y in (1.12, 1.4, 1.7):
        B((0, y, -.24), (.38, .03, .03), 'ring_graphite', 'halo')
    B((0, 1.27, -.25), (.26, .2, .1), 'ring_foam', 'halo', bev=.03, seg=2)
    B((.04, 1.56, -.25), (.22, .18, .09), 'ring_foam', 'halo', bev=.03, seg=2, rot=(0, 0, 4))
    strap((-.17, 1.38, -.3), (.17, 1.38, -.3), .03, 'halo', t=.015)
    C((-.17, 1.84, -.22), .035, .06, 'ring_hatch', 'halo', sides=8)
    E((-.17, 1.88, -.22), (.05, .045, .05), 'glow', 'halo', glow='core', k=1.6)

    for sd, s in (('L', 1), ('R', -1)):
        # Thigh, knee cap, shin, a broad mag-boot packed with dust
        P((s * .14, .79, 0), (.15, .4, .17), 'ring_enamel', 'hip' + sd, top=(1.15, 1.1), bev=.025, seg=2)
        C((s * .14, .56, .01), .06, .14, 'ring_graphite', 'knee' + sd, rot=(0, 0, 90), sides=10)
        B((s * .14, .57, .085), (.1, .1, .04), 'ring_enamel_dk', 'knee' + sd, bev=.015)
        P((s * .14, .3, 0), (.12, .44, .14), 'ring_dust', 'knee' + sd, top=(1.2, 1.2), bev=.02, seg=2)
        B((s * .14, .07, .05), (.17, .12, .32), 'ring_dust_dk', 'knee' + sd, bev=.025, seg=2)
        B((s * .14, .015, .05), (.19, .03, .34), 'ring_rubber', 'knee' + sd)
        B((s * .14, .2, .07), (.1, .03, .02), 'ring_rime', 'knee' + sd)
        # Arm: a long upper arm, a graphite elbow, a forearm guard, a three-fingered gripper
        P((s * .33, 1.42, 0), (.1, .3, .11), 'ring_enamel' if s > 0 else 'ring_graphite', 'arm' + sd, top=(1.1, 1.1), bev=.02, seg=2)
        C((s * .36, 1.25, .01), .045, .1, 'ring_graphite', 'elbow' + sd, rot=(0, 0, 90), sides=10)
        P((s * .37, 1.09, .02), (.09, .3, .1), 'ring_enamel_dk', 'elbow' + sd, top=(1.25, 1.2), bev=.02, seg=2)
        B((s * .38, .93, .04), (.08, .07, .08), 'ring_graphite', 'hand' + sd, bev=.01)
        for f in (-1, 0, 1):
            K((s * .38 + f * .025, .9, .04 + abs(f) * -.01), .016, .14, 'ring_graphite', 'hand' + sd, rot=(180, 0, f * 8), sides=6)
        K((s * .38, .9, .085), .014, .1, 'ring_enamel_dk', 'hand' + sd, rot=(160, 0, 0), sides=6)
    done('stray')


# =====================================================================================
# HERDER (about 1.5 m across): a plough plate in front, gas pods out to the sides
# =====================================================================================
if want('herder'):
    begin('herder')
    add_joint('body', (0, .0, 0))
    add_joint('head', (0, .42, -.15), 'body')
    add_joint('shield', (0, .0, .42), 'body')
    add_joint('wingL', (.48, .05, -.1), 'body'); add_joint('wingR', (-.48, .05, -.1), 'body')
    # The block: a low chamfered body, darker below, vents along its flanks
    P((0, 0, -.12), (.72, .5, .78), 'ring_enamel', 'body', top=(.8, .82), bev=.05, seg=2)
    B((0, -.24, -.12), (.62, .08, .66), 'ring_dust_dk', 'body', bev=.02)
    for k in range(4):
        B((.365, -.02, -.36 + k * .14), (.02, .16, .07), 'ring_graphite', 'body')
        B((-.365, -.02, -.36 + k * .14), (.02, .16, .07), 'ring_graphite', 'body')
    stencil((0, .1, .28), .03, .05, 'body', n=3)
    B((0, .26, -.4), (.42, .04, .2), 'ring_dust', 'body')
    rod((.2, .25, -.45), (.24, .78, -.55), .008, 'ring_graphite', 'body', sides=4)        # antenna
    E((.24, .79, -.55), (.03, .03, .03), 'ring_enamel_dk', 'body')
    # The plough: a broad curved plate (three facets), a rubber bumper strip, black chevrons, scuffs
    for i, (x, yaw) in enumerate(((-.42, -26), (0, 0), (.42, 26))):
        B((x, 0, .45 - abs(x) * .32), (.46, .54, .05), 'ring_plough', 'shield', rot=(-8, yaw, 0), bev=.012)
        for c in range(2):
            B((x + (c - .5) * .18, .06, .48 - abs(x) * .32), (.06, .3, .01), 'ring_stencil', 'shield', rot=(-8, yaw, 35 if c else -35))
    B((0, -.33, .46), (1.3, .08, .1), 'ring_rubber', 'shield', bev=.02)
    B((0, .3, .3), (1.0, .04, .06), 'ring_graphite', 'shield')
    for x in (-.5, 0, .5):
        B((x, .05, .22 - abs(x) * .3), (.06, .44, .06), 'ring_graphite', 'shield')                      # its ribs behind
        B((x * .9, -.12, .25), (.05, .05, .3), 'ring_graphite', 'shield', rot=(-20, 0, 0))   # struts to the body
    B((.2, .18, .49), (.12, .03, .01), 'ring_rime', 'shield')
    # Its lamp: a hooded searchlight that swivels
    B((0, .02, 0), (.18, .16, .2), 'ring_graphite', 'head', bev=.02)
    P((0, .1, .05), (.24, .12, .28), 'ring_enamel_dk', 'head', top=(.9, .8), bev=.02)
    C((0, .0, .12), .07, .06, 'ring_rubber', 'head', rot=(90, 0, 0), sides=12)
    C((0, .0, .155), .055, .02, 'glow', 'head', rot=(90, 0, 0), sides=12, glow='eye', k=1.8)
    # The pods: cold-gas thrusters on swivel arms, nozzles back and down
    for sd, s in (('L', 1), ('R', -1)):
        B((s * .1, 0, 0), (.22, .06, .08), 'ring_graphite', 'wing' + sd)
        C((s * .26, 0, 0), .13, .38, 'ring_enamel', 'wing' + sd, rot=(90, 0, 0), sides=12)
        C((s * .26, 0, .21), .1, .06, 'ring_enamel_dk', 'wing' + sd, rot=(90, 0, 0), sides=12)
        C((s * .26, 0, -.23), .08, .1, 'ring_graphite', 'wing' + sd, rot=(90, 0, 0), sides=10, r2=.11)
        C((s * .26, 0, -.29), .065, .02, 'glow', 'wing' + sd, rot=(90, 0, 0), sides=10, glow='core', k=.9)
        C((s * .26, -.15, -.02), .03, .08, 'ring_graphite', 'wing' + sd, sides=8)
        B((s * .26, .13, 0), (.14, .02, .26), 'ring_dust', 'wing' + sd)
    done('herder')


# =====================================================================================
# TALLY (3.6 m): a mast on spiked legs, rods of beads, a counter drum, a lamp
# =====================================================================================
if want('tally'):
    begin('tally')
    add_joint('body', (0, 0, 0))
    add_joint('halo', (0, 2.7, 0), 'body')      # the counter drum: it turns a notch each count
    add_joint('crown', (0, 3.45, 0), 'body')    # the lamp
    add_joint('head', (0, 2.25, 0), 'body')     # the reading eye under the drum
    add_joint('armL', (0, 1.55, 0), 'body')     # the bead rows: they slide
    add_joint('armR', (0, 1.15, 0), 'body')
    # Three spiked legs driven into the reef, braced, dust to the knees
    for k in range(3):
        a = k * 2.0944 + .5
        foot = (math.sin(a) * .95, 0, math.cos(a) * .95)
        rod((foot[0] * .2, .9, foot[2] * .2), foot, .06, 'ring_graphite', 'body')
        rod((foot[0] * .55, .45, foot[2] * .55), (foot[0] * .12, .5, foot[2] * .12), .03, 'ring_graphite', 'body')
        K((foot[0], .05, foot[2]), .08, .45, 'ring_dust_dk', 'body', rot=(180, 0, 0), sides=6)
        B((foot[0], .08, foot[2]), (.22, .06, .22), 'ring_dust', 'body', rot=(0, math.degrees(a), 0))
    # The mast: a column of chamfered segments, rimed, with a serial
    C((0, .95, 0), .2, .35, 'ring_enamel_dk', 'body', sides=8)
    for k in range(5):
        P((0, 1.2 + k * .3, 0), (.24, .26, .24), 'ring_enamel' if k % 2 else 'ring_enamel_dk', 'body', top=(.92, .92), bev=.02)
    stencil((0, 2.05, .125), .025, .05, 'body', n=5)
    B((.1, 1.3, .12), (.03, .25, .01), 'ring_rime', 'body')
    # The bead rows: two racks, each a frame of rods with beads strung on them
    for jn, y, w in (('armL', 1.55, .9), ('armR', 1.15, .74)):
        for z in (-.08, .08):
            rod((-w / 2, y, z), (w / 2, y, z), .012, 'ring_graphite', jn, sides=6)
        B((-w / 2, y, 0), (.03, .1, .22), 'ring_graphite', jn)
        B((w / 2, y, 0), (.03, .1, .22), 'ring_graphite', jn)
        n = 7 if jn == 'armL' else 5
        for b in range(n):
            for z in (-.08, .08):
                x = -w / 2 + .08 + b * .07 + (.22 if b >= n - 2 else 0)
                C((x, y, z), .035, .055, 'ring_bead' if b < n - 2 else 'ring_enamel_dk', jn, rot=(0, 0, 90), sides=8)
    # The reading eye: a ring of pale lenses looking out all round
    C((0, 2.25, 0), .2, .14, 'ring_graphite', 'head', sides=12)
    for k in range(6):
        a = k * 1.0472
        B((math.sin(a) * .2, 2.25, math.cos(a) * .2), (.06, .05, .02), 'glow', 'head', rot=(0, math.degrees(a), 0), glow='eye', k=1.2)
    # The counter drum: a wide flat cylinder with tick marks round it
    C((0, 2.7, 0), .42, .26, 'ring_enamel', 'halo', sides=24)
    C((0, 2.84, 0), .36, .04, 'ring_graphite', 'halo', sides=24)
    for k in range(24):
        a = k * .2618
        B((math.sin(a) * .425, 2.7, math.cos(a) * .425), (.02, .14 if k % 4 else .2, .01), 'ring_stencil', 'halo', rot=(0, math.degrees(a), 0))
    # The lamp: a capsule lamp on a short post (amber: it was taken off a capsule)
    C((0, 3.1, 0), .05, .6, 'ring_graphite', 'body', sides=8)
    C((0, 3.42, 0), .1, .06, 'ring_hatch', 'crown', sides=10)
    E((0, 3.5, 0), (.14, .12, .14), 'glow', 'crown', glow='core', k=1.6)
    done('tally')


# =====================================================================================
# PORTER (2.9 m): a capsule on its back, loader arms, head sunk under the load
# =====================================================================================
if want('porter'):
    begin('porter')
    add_joint('waist', (0, 1.25, 0)); add_joint('torso', (0, 1.45, 0), 'waist'); add_joint('head', (0, 2.1, .32), 'torso')
    add_joint('shield', (0, 1.9, -.45), 'torso')   # the capsule: it sways on its straps
    for sd, s in (('L', 1), ('R', -1)):
        add_joint('hip' + sd, (s * .26, 1.18, 0), 'waist'); add_joint('knee' + sd, (s * .3, .62, .08), 'hip' + sd)
        add_joint('arm' + sd, (s * .62, 2.05, .12), 'torso'); add_joint('elbow' + sd, (s * .72, 1.55, .22), 'arm' + sd)
        add_joint('hand' + sd, (s * .74, .98, .38), 'elbow' + sd)
    # Hips and a thick spine
    P((0, 1.2, 0), (.62, .3, .42), 'ring_enamel_dk', 'waist', top=(.9, .9), bev=.04, seg=2)
    B((0, 1.1, .2), (.4, .14, .06), 'ring_dust', 'waist', bev=.01)
    # A barrel chest bent forward under the load: broad, plated, the shoulders high
    P((0, 1.75, .12), (.82, .62, .6), 'ring_enamel', 'torso', rot=(14, 0, 0), top=(1.3, 1.0), bev=.05, seg=2)
    P((0, 1.78, .44), (.5, .36, .05), 'ring_enamel_dk', 'torso', rot=(14, 0, 0), top=(1.2, 1), bev=.012)
    stencil((.0, 1.86, .47), .035, .055, 'torso', rot=(14, 0, 0), n=3)
    for s in (1, -1):
        B((s * .5, 2.1, .1), (.36, .2, .5), 'ring_enamel', 'torso', bev=.05, seg=2, rot=(10, 0, s * -10))   # shoulder yokes
        B((s * .5, 2.22, .1), (.3, .04, .44), 'ring_dust', 'torso', rot=(10, 0, s * -10))
        strap((s * .34, 2.2, .3), (s * .3, 1.4, .42), .07, 'torso')                                       # the load straps over its shoulders
    # Head: small, sunk low between the yokes, a heavy brow, a pale slit
    B((0, 2.1, .4), (.3, .22, .26), 'ring_enamel_dk', 'head', bev=.04, seg=2)
    B((0, 2.2, .44), (.36, .06, .22), 'ring_enamel', 'head', bev=.015)
    B((0, 2.1, .535), (.22, .03, .02), 'glow', 'head', glow='eye', k=1.5)
    B((0, 2.1, .528), (.26, .07, .02), 'ring_graphite', 'head')
    # The capsule on its back: frosted hull, ribs, a domed top and bottom, the hatch, its amber lamp
    C((0, 1.9, -.6), .42, 1.5, 'ring_capsule', 'shield', sides=16)
    E((0, 2.65, -.6), (.84, .4, .84), 'ring_capsule', 'shield', seg=16, rings=8)
    E((0, 1.15, -.6), (.84, .34, .84), 'ring_hatch', 'shield', seg=16, rings=8)
    for y in (1.35, 1.75, 2.15, 2.5):
        C((0, y, -.6), .435, .05, 'ring_graphite', 'shield', sides=16)
    B((0, 1.95, -1.01), (.36, .62, .04), 'ring_hatch', 'shield', bev=.02)                 # its hatch (outward, behind)
    B((0, 1.95, -1.03), (.24, .03, .02), 'ring_stencil', 'shield')
    for y in (1.72, 2.18):
        B((.15, y, -1.035), (.05, .05, .02), 'ring_buckle', 'shield')
    C((0, 2.98, -.6), .05, .1, 'ring_hatch', 'shield', sides=8)
    E((0, 3.06, -.6), (.12, .1, .12), 'glow', 'shield', glow='core', k=1.6)
    B((.25, 2.35, -.98), (.15, .3, .02), 'ring_rime', 'shield', rot=(0, 20, 0))
    stencil((-.2, 1.5, -1.0), .03, .05, 'shield', n=4)
    for sd, s in (('L', 1), ('R', -1)):
        # Short thick legs, a wide mag boot
        P((s * .3, .9, .04), (.28, .55, .3), 'ring_enamel', 'hip' + sd, top=(1.15, 1.1), bev=.04, seg=2)
        C((s * .3, .62, .08), .11, .3, 'ring_graphite', 'knee' + sd, rot=(0, 0, 90), sides=10)
        P((s * .3, .34, .08), (.24, .5, .28), 'ring_dust', 'knee' + sd, top=(1.2, 1.15), bev=.03, seg=2)
        B((s * .3, .08, .16), (.34, .16, .52), 'ring_dust_dk', 'knee' + sd, bev=.03, seg=2)
        B((s * .3, .015, .16), (.36, .03, .54), 'ring_rubber', 'knee' + sd)
        # Upper arm: graphite; a big loader forearm; a flat shovel hand
        P((s * .67, 1.8, .16), (.22, .5, .24), 'ring_graphite', 'arm' + sd, top=(1.1, 1.1), bev=.03)
        C((s * .72, 1.55, .22), .12, .26, 'ring_graphite', 'elbow' + sd, rot=(0, 0, 90), sides=10)
        P((s * .74, 1.25, .3), (.3, .55, .32), 'ring_enamel', 'elbow' + sd, top=(1.25, 1.2), bev=.04, seg=2)
        B((s * .74, 1.25, .47), (.2, .4, .02), 'ring_dust', 'elbow' + sd)
        rod((s * .6, 1.5, .1), (s * .65, 1.05, .2), .035, 'ring_buckle', 'elbow' + sd)        # its hydraulic ram
        B((s * .74, .92, .42), (.34, .1, .34), 'ring_graphite', 'hand' + sd, bev=.02)
        P((s * .74, .78, .48), (.42, .22, .06), 'ring_enamel_dk', 'hand' + sd, rot=(-25, 0, 0), top=(1.1, 1), bev=.015)
        for f in (-1, 0, 1):
            B((s * .74 + f * .13, .66, .54), (.08, .06, .1), 'ring_dust_dk', 'hand' + sd, rot=(-25, 0, 0))
    done('porter')



# =====================================================================================
# THE CROOK (2.9 m, stooped by the pose): thin, long-shinned, a hook on a pole for an arm
# =====================================================================================
if want('crook'):
    begin('crook')
    add_joint('waist', (0, 1.62, 0)); add_joint('torso', (0, 1.9, 0), 'waist'); add_joint('head', (0, 2.62, .08), 'torso')
    add_joint('halo', (0, 2.2, -.22), 'torso')   # the winch on its back
    for sd, s in (('L', 1), ('R', -1)):
        add_joint('hip' + sd, (s * .12, 1.56, 0), 'waist'); add_joint('knee' + sd, (s * .13, .86, .03), 'hip' + sd)
    add_joint('armL', (.3, 2.4, 0), 'torso'); add_joint('elbowL', (.36, 2.0, .02), 'armL'); add_joint('handL', (.38, 1.66, .06), 'elbowL')
    add_joint('armR', (-.32, 2.42, 0), 'torso'); add_joint('elbowR', (-.4, 1.9, .02), 'armR'); add_joint('handR', (-.44, 1.4, .06), 'elbowR')
    add_joint('shield', (-.46, .3, .46), 'handR')   # the hook's head, at the end of the pole

    # Pelvis: a narrow enamel cradle on a graphite spine of stacked rings, dust in the creases
    P((0, 1.6, 0), (.3, .16, .2), 'ring_enamel_dk', 'waist', top=(.85, .9), bev=.02, seg=2)
    for k in range(5):
        C((0, 1.7 + k * .055, -.02), .05 - k * .003, .04, 'ring_graphite', 'waist', sides=10)
    B((0, 1.55, .1), (.18, .08, .04), 'ring_dust', 'waist', bev=.01)
    # Chest: long and narrow, a keel down the front, both shoulder plates still on (it is cared for)
    P((0, 2.12, .01), (.3, .44, .22), 'ring_enamel', 'torso', top=(1.3, 1.05), bev=.03, seg=2)
    P((0, 2.14, .125), (.07, .4, .04), 'ring_enamel_dk', 'torso', top=(1.6, 1), bev=.01)   # the keel
    stencil((-.07, 2.24, .13), .02, .03, 'torso', n=5)
    B((0, 1.9, .05), (.22, .06, .16), 'ring_graphite', 'torso', bev=.01)
    for s in (1, -1):
        B((s * .22, 2.36, 0), (.16, .07, .22), 'ring_enamel', 'torso', bev=.025, seg=2, rot=(0, 0, s * -16))
        B((s * .22, 2.33, .06), (.14, .02, .06), 'ring_rime', 'torso', rot=(0, 0, s * -16))
    strap((.16, 2.36, .12), (-.12, 1.98, .12), .04, 'torso')
    # Neck and head: a long chamfered helm pushed forward, a visor slit, a crest like a hood
    C((0, 2.5, .04), .045, .16, 'ring_graphite', 'torso', sides=10, rot=(18, 0, 0))
    P((0, 2.66, .1), (.17, .2, .32), 'ring_enamel', 'head', top=(.8, .7), bev=.035, seg=3)
    B((0, 2.66, .265), (.12, .026, .02), 'glow', 'head', glow='eye', k=1.5)
    B((0, 2.66, .258), (.16, .06, .02), 'ring_graphite', 'head')
    P((0, 2.78, -.02), (.05, .12, .34), 'ring_enamel_dk', 'head', top=(.4, .6), bev=.01)   # the crest
    B((.09, 2.63, .02), (.02, .1, .18), 'ring_dust_dk', 'head'); B((-.09, 2.63, .02), (.02, .1, .18), 'ring_dust_dk', 'head')

    # The winch: a drum on a frame across its back, the line wound on it, the capsule lamp it took
    for s in (1, -1):
        B((s * .16, 2.2, -.22), (.03, .5, .05), 'ring_graphite', 'halo', bev=.006)
    C((0, 2.24, -.3), .13, .3, 'ring_hatch', 'halo', rot=(0, 0, 90), sides=14)
    C((0, 2.24, -.3), .1, .26, 'ring_strap', 'halo', rot=(0, 0, 90), sides=14)   # the line, wound
    for s in (1, -1):
        C((s * .16, 2.24, -.3), .14, .02, 'ring_enamel_dk', 'halo', rot=(0, 0, 90), sides=14)
    C((.12, 2.44, -.28), .03, .06, 'ring_hatch', 'halo', sides=8)
    E((.12, 2.49, -.28), (.045, .04, .045), 'glow', 'halo', glow='core', k=1.7)
    rod((-.1, 2.18, -.32), (-.34, 1.9, -.1), .008, 'ring_strap', 'halo', sides=5)   # the line, running to the arm

    for sd, s in (('L', 1), ('R', -1)):
        # Long legs: thigh, knee, a very long shin, a narrow splayed foot (it stands in the reef's gaps)
        P((s * .13, 1.22, 0), (.12, .6, .14), 'ring_enamel', 'hip' + sd, top=(1.15, 1.1), bev=.02, seg=2)
        C((s * .13, .87, .03), .05, .12, 'ring_graphite', 'knee' + sd, rot=(0, 0, 90), sides=10)
        B((s * .13, .88, .1), (.08, .1, .04), 'ring_enamel_dk', 'knee' + sd, bev=.012)
        P((s * .13, .46, 0), (.09, .76, .11), 'ring_dust', 'knee' + sd, top=(1.3, 1.25), bev=.015, seg=2)
        B((s * .13, .05, .08), (.12, .08, .34), 'ring_dust_dk', 'knee' + sd, bev=.02, seg=2)
        for t in (-1, 1):
            K((s * .13 + t * .04, .03, .25), .02, .12, 'ring_graphite', 'knee' + sd, rot=(90, 0, t * 12), sides=5)
        B((s * .13, .3, .06), (.08, .03, .02), 'ring_rime', 'knee' + sd)

    # Left arm: short, a gripper for holding what it has hooked
    P((.33, 2.22, 0), (.09, .36, .1), 'ring_enamel', 'armL', top=(1.1, 1.1), bev=.02, seg=2)
    C((.36, 2.0, .02), .04, .09, 'ring_graphite', 'elbowL', rot=(0, 0, 90), sides=10)
    P((.37, 1.84, .04), (.08, .3, .09), 'ring_enamel_dk', 'elbowL', top=(1.2, 1.2), bev=.02, seg=2)
    B((.38, 1.66, .06), (.07, .06, .07), 'ring_graphite', 'handL', bev=.01)
    for f in (-1, 1):
        K((.38 + f * .02, 1.63, .06), .015, .13, 'ring_graphite', 'handL', rot=(180, 0, f * 10), sides=6)

    # Right arm: the crook. Upper arm and forearm in heavy enamel, then a graphite pole
    P((-.36, 2.18, 0), (.11, .48, .12), 'ring_enamel', 'armR', top=(1.1, 1.1), bev=.02, seg=2)
    C((-.4, 1.9, .02), .055, .12, 'ring_graphite', 'elbowR', rot=(0, 0, 90), sides=10)
    P((-.42, 1.66, .04), (.11, .48, .12), 'ring_enamel_dk', 'elbowR', top=(1.15, 1.1), bev=.02, seg=2)
    B((-.44, 1.4, .06), (.12, .1, .12), 'ring_graphite', 'handR', bev=.015)   # the socket the pole is fixed in
    rod((-.44, 1.38, .08), (-.46, .38, .44), .028, 'ring_graphite', 'handR', sides=10)   # the pole
    for t in (0.25, 0.55, 0.8):   # collars, worn bright, dust caught under them
        a = (-.44 + (-.02) * t, 1.38 - 1.0 * t, .08 + .36 * t)
        C(a, .036, .05, 'ring_enamel_dk' if t != .55 else 'ring_buckle', 'handR', rot=(-20, 0, 0), sides=10)
    # The hook: a J of short bent segments, the inside edge bright from use, a barb
    pts = [(-.46, .38, .44), (-.46, .22, .5), (-.46, .12, .62), (-.46, .14, .76), (-.46, .26, .84), (-.46, .4, .82)]
    for a, b in zip(pts, pts[1:]):
        rod(a, b, .034, 'ring_enamel', 'shield', sides=8)
    for a, b in zip(pts[1:], pts[2:]):
        rod((a[0], a[1], a[2] + .02), (b[0], b[1], b[2] + .02), .012, 'ring_buckle', 'shield', sides=5)
    K((-.46, .4, .82), .03, .12, 'ring_graphite', 'shield', rot=(-30, 0, 0), sides=6)   # the barb
    B((-.46, .14, .66), (.05, .03, .04), 'ring_rime', 'shield')
    done('crook')



# =====================================================================================
# THE DROVER (about 3.2 m long): a flying tug with push-forks; it moves ice in formation
# =====================================================================================
if want('drover'):
    begin('drover')
    add_joint('body', (0, 0, 0))
    add_joint('head', (0, 1.05, -1.2), 'body')       # the mast's lamp
    add_joint('shield', (0, -.05, 1.1), 'body')      # the forks
    add_joint('wingL', (.9, .1, -.3), 'body'); add_joint('wingR', (-.9, .1, -.3), 'body')
    # The hull: long, low and chamfered, scuffed enamel over a dust-dark keel, its flanks ribbed
    P((0, 0, -.2), (1.3, .62, 2.4), 'ring_enamel', 'body', top=(.82, .86), bev=.06, seg=2)
    B((0, -.32, -.2), (1.1, .12, 2.1), 'ring_dust_dk', 'body', bev=.03)
    for k in range(7):
        B((.66, -.02, -1.15 + k * .32), (.03, .3, .1), 'ring_graphite', 'body')
        B((-.66, -.02, -1.15 + k * .32), (.03, .3, .1), 'ring_graphite', 'body')
    B((0, .33, -.1), (.9, .05, 1.5), 'ring_dust', 'body')
    stencil((0, .12, .9), .04, .07, 'body', n=5)
    for z in (-.6, .3):   # cargo straps over its back
        B((0, .34, z), (1.36, .03, .12), 'ring_strap', 'body')
        B((.5, .36, z), (.12, .05, .16), 'ring_buckle', 'body')
    B((.3, .3, .5), (.16, .03, .06), 'ring_rime', 'body')
    # The forks: two long beams out in front, each ending in a broad push-pad with a rubber face
    for s in (1, -1):
        rod((s * .45, -.05, .8), (s * .55, -.1, 2.3), .07, 'ring_graphite', 'shield', sides=8)
        B((s * .55, -.1, 1.6), (.16, .2, .16), 'ring_enamel_dk', 'shield', bev=.02)
        P((s * .58, -.1, 2.36), (.7, .9, .12), 'ring_plough', 'shield', top=(1.0, 1.0), bev=.015)
        B((s * .58, -.1, 2.44), (.74, .94, .05), 'ring_rubber', 'shield', bev=.01)
        for c in range(2):
            B((s * .58 + (c - .5) * .26, -.05, 2.43), (.08, .5, .01), 'ring_stencil', 'shield', rot=(0, 0, 35 if c else -35))
    rod((.5, -.05, 1.3), (-.5, -.05, 1.3), .04, 'ring_graphite', 'shield', sides=6)   # the cross-brace
    # The mast at the back: a lattice post, the hooded lamp, and the herd's amber beacon on top
    for s in (1, -1):
        rod((s * .12, .3, -1.15), (s * .06, 1.0, -1.2), .03, 'ring_graphite', 'body', sides=6)
    B((0, 1.0, -1.2), (.24, .2, .28), 'ring_graphite', 'head', bev=.02)
    P((0, 1.13, -1.15), (.32, .14, .36), 'ring_enamel_dk', 'head', top=(.9, .8), bev=.02)
    C((0, 1.04, -1.0), .09, .06, 'ring_rubber', 'head', rot=(90, 0, 0), sides=12)
    C((0, 1.04, -.96), .07, .02, 'glow', 'head', rot=(90, 0, 0), sides=12, glow='eye', k=1.8)
    C((0, 1.24, -1.2), .05, .1, 'ring_hatch', 'head', sides=8)
    E((0, 1.33, -1.2), (.07, .06, .07), 'glow', 'head', glow='core', k=1.8)
    # Four pods on swivel arms, nozzles back
    for sd, s in (('L', 1), ('R', -1)):
        for z in (.55, -.85):
            B((s * .82, .1, z), (.36, .07, .1), 'ring_graphite', 'wing' + sd)
            C((s * 1.05, .1, z), .19, .55, 'ring_enamel', 'wing' + sd, rot=(90, 0, 0), sides=12)
            C((s * 1.05, .1, z + .3), .15, .07, 'ring_enamel_dk', 'wing' + sd, rot=(90, 0, 0), sides=12)
            C((s * 1.05, .1, z - .33), .12, .14, 'ring_graphite', 'wing' + sd, rot=(90, 0, 0), sides=10, r2=.16)
            C((s * 1.05, .1, z - .41), .1, .02, 'glow', 'wing' + sd, rot=(90, 0, 0), sides=10, glow='core', k=.9)
            B((s * 1.05, .3, z), (.2, .02, .36), 'ring_dust', 'wing' + sd)
    done('drover')
