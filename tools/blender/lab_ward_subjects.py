# The Living Ward's four subjects, rebuilt with character: gaunt flesh chassis
# with readable anatomy (ribcage, knuckled joints, long clawed hands, a hinged
# jaw full of teeth), each dressed for what the ward did to it.
#
#   patient   - an escaped patient: torn gown, restraint straps, a cracked
#               porcelain half-mask over one glowing eye, staples across the skull
#   grafted   - a heavy stitched together from mismatched parts: one huge splinted
#               graft arm, a riveted chest brace, a respirator, IV bags on its back
#   suspended - hung from a hovering hoist in a straitjacket, head bandaged,
#               legs limp, its core burning through a hole in the canvas
#   phased    - split down the middle by a glowing fracture, shards of itself
#               orbiting, a column of three eyes
#
# exec'd by lab_subjects_build.py (after enemy_kit.py), which saves each file.
import math

_og.PALETTE.update({
    'ward_skin': (.68, .47, .41), 'ward_bruise': (.46, .33, .36), 'ward_flesh': (.50, .22, .23),
    'ward_bone': (.82, .78, .69), 'ward_tooth': (.88, .85, .74), 'ward_gum': (.30, .07, .09),
    'ward_mouth': (.06, .015, .02), 'ward_claw': (.12, .10, .10), 'ward_gown': (.55, .69, .67),
    'ward_gown_dark': (.38, .5, .5), 'ward_strap': (.28, .18, .12), 'ward_buckle': (.62, .64, .66),
    'ward_steel': (.22, .25, .28), 'ward_mask': (.88, .88, .84), 'ward_crack': (.1, .09, .09),
    'ward_canvas': (.78, .75, .66), 'ward_bandage': (.72, .68, .6), 'ward_stitch': (.08, .05, .05),
    'ward_tube': (.66, .78, .8), 'ward_hazard': (.8, .55, .1), 'ward_phase': (.62, .67, .74),
    'ward_shard': (.84, .86, .88), 'ward_socket': (.03, .02, .03),
})


def build_ward_subject(name, width, sy):
    def p(v): return (v[0] * width, v[1] * sy, v[2] * width)
    def joint(n, v, parent=None): add_joint(n, p(v), parent)
    def oval(v, s, mat, parent, **kw): return E(p(v), p(s), mat, parent, **kw)
    def box(v, s, mat, parent, **kw): return B(p(v), p(s), mat, parent, **kw)
    def rad(r): return r * width * (1.3 if name != 'grafted' else 1.)
    def limb(points, radii, mat, parent, sides=10, **kw):
        return tube([p(q) for q in points], [rad(r) for r in radii], mat, parent, sides=sides, **kw)
    def band(a, b, r, mat, parent, sides=12, **kw):
        # A short cylinder from a to b: a strap round a limb, a filter, a rod
        a, b = p(a), p(b)
        d = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
        n = math.sqrt(sum(x * x for x in d))
        return C(tuple((a[i] + b[i]) * .5 for i in range(3)), rad(r), n, mat, parent, rot=_aim(d), sides=sides, **kw)
    def claw(v, d, length, parent, r=.012):
        return K(p(v), rad(r), length * sy, 'ward_claw', parent, rot=_aim(d), sides=6)
    def stitches(a, b, count, parent, size=.035):
        # Surgical cross-stitches along a seam
        for k in range(count):
            t = (k + .5) / count
            c = tuple(a[i] + (b[i] - a[i]) * t for i in range(3))
            for r in (40, -40):
                box(c, (size, .008, .008), 'ward_stitch', parent, rot=(0, 0, r))

    skin = {'patient': 'ward_skin', 'grafted': 'ward_skin', 'suspended': 'ward_skin', 'phased': 'ward_phase'}[name]
    heavy = name == 'grafted'

    # ---- the skeleton: the shared ward rig, plus a jaw, gown flaps and straps ----
    joint('waist', (0, 1.17, 0)); joint('torso', (0, 1.53, 0), 'waist'); joint('head', (0, 2.43, 0), 'torso')
    joint('jaw', (0, 2.36, .03), 'head')
    joint('clothF', (0, 1.32, .17), 'waist'); joint('clothB', (0, 1.36, -.17), 'waist')
    for side, s in (('L', 1), ('R', -1)):
        joint('hip' + side, (s * .18, 1.1, 0), 'waist'); joint('knee' + side, (s * .18, .59, .015), 'hip' + side)
        joint('ankle' + side, (s * .18, .13, .015), 'knee' + side)
        joint('arm' + side, (s * .39, 2.1, 0), 'torso'); joint('elbow' + side, (s * .44, 1.65, .015), 'arm' + side)
        joint('hand' + side, (s * .44, 1.16, .055), 'elbow' + side)
        joint('fingers' + side, (s * .44, 1.06, .08), 'hand' + side); joint('thumb' + side, (s * .36, 1.16, .11), 'hand' + side)
        joint('ribbon' + side, (s * .46, 1.24, -.02), 'elbow' + side)

    # ---- legs: lean thighs, knuckled knees, long bare feet ----
    for side, s in (('L', 1), ('R', -1)):
        big = 1.25 if heavy else 1.
        limb([(s * .18, 1.1, 0), (s * .18, .84, .01), (s * .18, .62, .02)], [.1 * big, .085 * big, .066 * big], skin, 'hip' + side)
        oval((s * .18, .61, .065), (.12 * big, .12, .1), 'ward_bone', 'knee' + side)
        limb([(s * .18, .58, .02), (s * .185, .36, -.01), (s * .18, .15, .01)], [.066 * big, .058 * big, .04 * big], skin, 'knee' + side)
        box((s * .18, .37, .05), (.03, .34, .02), 'ward_bone', 'knee' + side)                 # shin ridge
        oval((s * .18, .42, -.06), (.1 * big, .2, .1 * big), skin, 'knee' + side)            # calf
        oval((s * .18, .13, 0), (.09, .09, .09), 'ward_bone', 'ankle' + side)
        oval((s * .18, .06, .08), (.12 * big, .07, .27), skin, 'ankle' + side)
        for t in range(4):
            x = s * (.14 + t * .028)
            oval((x, .04, .22 + (.01 if t == 1 else 0)), (.028, .03, .07), skin, 'ankle' + side)
            claw((x, .04, .25), (0, -.4, 1), .03, 'ankle' + side, r=.008)
        if name == 'patient':
            band((s * .18, .9, -.005), (s * .18, .84, .005), .1, 'ward_strap', 'hip' + side)
            box((s * .27, .87, .02), (.04, .06, .05), 'ward_buckle', 'hip' + side)
        if heavy:
            # A steel leg brace on the outside of each leg, strapped at thigh and shin
            box((s * .3, .66, 0), (.025, .9, .04), 'ward_steel', 'hip' + side)
            box((s * .3, .32, 0), (.025, .4, .04), 'ward_steel', 'knee' + side)
            oval((s * .3, .6, 0), (.07, .07, .07), 'ward_buckle', 'knee' + side)
            for y, par in ((.95, 'hip'), (.3, 'knee')):
                band((s * .18, y + .03, 0), (s * .18, y - .03, 0), .11 * big, 'ward_strap', par + side)

    # ---- arms: bony shoulders, long forearms, clawed fingers ----
    for side, s in (('L', 1), ('R', -1)):
        graft = heavy and side == 'R'
        k = 1.9 if graft else (1.15 if heavy else 1.)
        oval((s * .37, 2.07, 0), (.17 * k, .15 * k, .17 * k), skin, 'arm' + side)
        limb([(s * .39, 2.04, 0), (s * .42, 1.85, .005), (s * .44, 1.68, .015)], [.07 * k, .062 * k, .052 * k], skin, 'arm' + side)
        oval((s * .44, 1.65, -.045), (.085 * k, .085 * k, .08 * k), 'ward_bone', 'elbow' + side)
        limb([(s * .44, 1.64, .02), (s * .445, 1.42, .04), (s * .44, 1.22, .055)], [.056 * k, .05 * k, .038 * k], skin, 'elbow' + side)
        oval((s * .44, 1.13, .06), (.1 * k, .12 * k, .055 * k), skin, 'hand' + side)
        nf = 3 if graft else 4
        for f in range(nf):
            x = s * (.395 + f * (.03 if not graft else .05))
            pts = [(x, 1.08, .07), (x, .97, .085), (x, .88, .075)]
            limb(pts, [.016 * k, .014 * k, .011 * k], skin, 'fingers' + side, sides=6)
            claw((x, .88, .075), (0, -1, .15), .06 * (1.4 if graft else 1.), 'fingers' + side, r=.011 * k)
        limb([(s * .38, 1.14, .09), (s * .35, 1.06, .12)], [.016 * k, .012 * k], skin, 'thumb' + side, sides=6)
        claw((s * .35, 1.06, .12), (0, -1, .3), .04, 'thumb' + side, r=.01 * k)
        if graft:
            # The graft: splint rods, clamp rings, a glowing seam where it was joined on
            for dx in (-.07, .07):
                box((s * .44 + dx, 1.42, .1), (.02, .42, .02), 'ward_steel', 'elbow' + side)
            for y in (1.58, 1.42, 1.26):
                band((s * .44, y + .025, .04), (s * .44, y - .025, .04), .115, 'ward_steel', 'elbow' + side)
            for t in range(8):
                a = t / 8 * math.tau
                box((s * (.37 + math.cos(a) * .14), 2.07 + math.sin(a) * .14, .1), (.035, .035, .03), 'glow', 'arm' + side, glow='seam', k=1.2)
            stitches((s * .44, 1.85, .09), (s * .44, 1.7, .08), 3, 'arm' + side)
        if name == 'patient':
            band((s * .44, 1.3, .045), (s * .44, 1.24, .05), .055, 'ward_strap', 'elbow' + side)
            box((s * .5, 1.27, .05), (.035, .05, .05), 'ward_buckle', 'elbow' + side)
            # A torn-off restraint strap, still buckled, swinging from the wrist
            box((s * .47, 1.0, -.02), (.04, .42, .012), 'ward_strap', 'ribbon' + side, rot=(0, 0, s * 6))
            box((s * .48, .8, -.02), (.05, .05, .02), 'ward_buckle', 'ribbon' + side)
            band((s * .44, 2.0, 0), (s * .43, 1.94, .003), .075, 'ward_strap', 'arm' + side)
        if name == 'suspended':
            # Straitjacket sleeves, their long ends strapped and hanging loose
            limb([(s * .39, 2.05, 0), (s * .44, 1.66, .015)], [.09, .08], 'ward_canvas', 'arm' + side)
            limb([(s * .44, 1.64, .02), (s * .44, 1.24, .055)], [.078, .066], 'ward_canvas', 'elbow' + side)
            box((s * .46, .95, .02), (.09, .55, .03), 'ward_canvas', 'ribbon' + side)
            box((s * .46, .72, .045), (.06, .07, .02), 'ward_buckle', 'ribbon' + side)
            band((s * .44, 1.5, .03), (s * .44, 1.45, .035), .085, 'ward_strap', 'elbow' + side)
        if name == 'phased':
            for t in range(3):
                box((s * .445, 1.5 - t * .12, .085), (.07, .012, .012), 'glow', 'elbow' + side, glow='seam', k=1.1, rot=(0, 0, s * 25))
            for f in range(4):
                oval((s * (.395 + f * .03), .87, .075), (.022, .022, .022), 'glow', 'fingers' + side, glow='core', k=1.3)

    # IV line out of the patient's forearm, swinging free
    if name == 'patient':
        box((.46, 1.38, .1), (.04, .02, .03), 'ward_steel', 'elbowL')
        for i, (a, b) in enumerate((((.47, 1.36, .1), (.52, 1.15, .1)), ((.52, 1.15, .1), (.5, .9, .06)), ((.5, .9, .06), (.53, .7, .07)))):
            band(a, b, .009, 'ward_tube', 'ribbonL', sides=6)
        box((-.44, 1.29, .1), (.04, .025, .015), 'glow', 'elbowR', glow='core', k=.8)       # ID band

    # ---- torso: pelvis, a sunken belly, a ribcage pressing through the skin ----
    oval((0, 1.15, 0), (.4 if not heavy else .5, .24, .26), skin, 'waist')
    oval((0, 1.42, .0), (.33 if not heavy else .62, .32, .22 if not heavy else .42), skin, 'torso')
    if not heavy:
        for t in range(3):
            oval((0, 1.33 + t * .08, .1), (.2 - t * .02, .05, .04), skin, 'torso')        # a starved abdomen
        oval((.12, 1.5, .09), (.1, .12, .03), 'ward_bruise', 'torso')
    oval((0, 1.84, -.015), (.5 if not heavy else .74, .46, .32 if not heavy else .44), skin, 'torso')
    if not heavy:
        for r in range(5):
            y = 1.98 - r * .085
            for s in (-1, 1):
                oval((s * .12, y, .145 - r * .006), (.22 - r * .015, .028, .05), 'ward_bone', 'torso', rot=(0, s * 18, s * -14))
        box((0, 1.86, .16), (.045, .34, .03), 'ward_bone', 'torso')
        for s in (-1, 1):
            limb([(s * .05, 2.09, .11), (s * .3, 2.12, .03)], [.022, .018], 'ward_bone', 'torso', sides=6)
    for t in range(8):
        oval((0, 1.3 + t * .1, -.17 if not heavy else -.24), (.07, .05, .05), 'ward_bone', 'torso')   # spine
    limb([(0, 2.1, 0), (0, 2.32, .03)], [.07 if not heavy else .12, .06 if not heavy else .1], skin, 'torso')
    for s in (-1, 1):
        limb([(s * .03, 2.14, .05), (s * .06, 2.3, .06)], [.013, .011], 'ward_flesh', 'torso', sides=5)

    # ---- heads ----
    def mouth(width_m=1.):
        # Upper teeth on the skull, lower teeth on the hinged jaw: it gapes when it screams
        oval((0, 2.37, .1), (.17 * width_m, .06, .1), 'ward_mouth', 'head')
        for t in range(7):
            x = (t - 3) * .022 * width_m
            box((x, 2.39, .155), (.016, .03, .014), 'ward_tooth', 'head')
            box((x, 2.335, .15), (.016, .028, .014), 'ward_tooth', 'jaw')
        oval((0, 2.39, .14), (.17 * width_m, .022, .04), 'ward_gum', 'head')
        oval((0, 2.31, .07), (.2 * width_m, .08, .17), skin, 'jaw')
        oval((0, 2.33, .13), (.16 * width_m, .02, .04), 'ward_gum', 'jaw')

    if name == 'patient':
        oval((0, 2.48, -.01), (.28, .34, .31), skin, 'head')
        for s in (-1, 1):
            oval((s * .1, 2.43, .1), (.08, .05, .07), skin, 'head')          # cheekbones
        mouth()
        # Cracked porcelain half-mask: one eye blazes through, the other is a dark socket
        box((0, 2.5, .14), (.27, .17, .05), 'ward_mask', 'head', bev=.02, seg=2)
        box((.02, 2.56, .168), (.05, .006, .006), 'ward_crack', 'head', rot=(0, 0, 35))
        box((.06, 2.47, .168), (.004, .07, .006), 'ward_crack', 'head', rot=(0, 0, -20))
        box((-.07, 2.53, .168), (.04, .005, .006), 'ward_crack', 'head', rot=(0, 0, -50))
        box((-.065, 2.5, .168), (.065, .05, .02), 'glow', 'head', glow='eye', k=1.7)
        box((.065, 2.5, .166), (.06, .045, .02), 'ward_socket', 'head')
        box((.065, 2.5, .177), (.012, .012, .01), 'glow', 'head', glow='eye', k=.6)
        # Staples across the skull: something was taken out
        for t in range(7):
            a = -.9 + t * .3
            box((math.sin(a) * .11, 2.62 + math.cos(a) * .02, math.cos(a) * .02 - .03), (.025, .008, .012), 'ward_buckle', 'head', rot=(0, math.degrees(a), 0))
        # What's left of its gown: a tattered skirt of strips round the hips (front
        # and back swing on their own joints) and a sash over one shoulder
        for t in range(6):
            a = (t - 2.5) * .32
            h = (.42, .3, .5, .36, .46, .28)[t]
            box((math.sin(a) * .2, 1.2 - h * .5, math.cos(a) * .17), (.1, h, .015), 'ward_gown', 'clothF', rot=(8, math.degrees(a), (t - 2.5) * 3))
        for t in range(5):
            a = (t - 2) * .36
            h = (.5, .38, .56, .34, .46)[t]
            box((math.sin(a) * .2, 1.24 - h * .5, -math.cos(a) * .17), (.11, h, .015), 'ward_gown_dark', 'clothB', rot=(-8, -math.degrees(a), (t - 2) * 3))
        oval((0, 1.21, 0), (.43, .08, .3), 'ward_gown_dark', 'waist')
        box((.05, 1.85, .0), (.11, .75, .38), 'ward_gown', 'torso', rot=(0, 0, -38))
        box((.22, 1.58, .17), (.04, .1, .02), 'ward_strap', 'torso')
    elif name == 'grafted':
        oval((0, 2.38, .02), (.3, .3, .3), skin, 'head')
        mouth(.9)
        # A respirator over the mouth and nose, filters either side, a hose to its back
        box((0, 2.36, .19), (.22, .14, .08), 'ward_steel', 'head', bev=.02, seg=2)
        for s in (-1, 1):
            band((s * .12, 2.34, .2), (s * .19, 2.32, .24), .05, 'ward_steel', 'head')
            box((s * .07, 2.48, .14), (.05, .03, .02), 'glow', 'head', glow='eye', k=1.4)
        limb([(0, 2.3, .15), (.1, 2.2, -.05), (.12, 2.1, -.3), (.1, 2.2, -.42)], [.025, .025, .025, .025], 'ward_steel', 'torso', sides=6)
        stitches((-.13, 2.52, .12), (.12, 2.55, .1), 5, 'head', size=.04)
        # The chest brace (its frontal plate): riveted, hazard-striped
        box((0, 1.8, .34), (.62, .62, .12), 'ward_steel', 'torso', bev=.04, seg=2)
        box((0, 1.78, .41), (.48, .46, .03), 'ward_mask', 'torso', bev=.012)
        box((0, 1.78, .43), (.3, .04, .02), 'ward_steel', 'torso')
        box((0, 1.78, .43), (.04, .3, .02), 'ward_steel', 'torso')
        for t in range(4):
            box((-.18 + t * .12, 2.06, .42), (.07, .04, .02), 'ward_hazard', 'torso', rot=(0, 0, 30))
        for x in (-.27, .27):
            for y in (1.55, 1.8, 2.05):
                oval((x, y, .42), (.035, .035, .03), 'ward_buckle', 'torso')
        for s in (-1, 1):
            band((s * .5, 1.82, .1), (s * .5, 1.82, -.2), .04, 'ward_strap', 'torso')
        # IV bags on a rack on its back (its weak core sits between them)
        box((0, 2.05, -.47), (.05, 1.2, .05), 'ward_steel', 'torso')
        box((0, 2.62, -.47), (.6, .04, .04), 'ward_steel', 'torso')
        for s in (-1, 1):
            box((s * .22, 2.38, -.5), (.2, .32, .08), 'glow', 'torso', glow='core', k=.9)
            limb([(s * .22, 2.2, -.5), (s * .3, 2.05, -.35), (s * .35, 2.0, -.1)], [.012, .012, .012], 'ward_tube', 'torso', sides=5)
        oval((0, 1.75, -.48), (.3, .4, .06), 'glow', 'torso', glow='core', k=.8)
        stitches((-.35, 1.35, .3), (.35, 1.4, .32), 8, 'torso')
        stitches((.3, 2.0, -.3), (.3, 1.4, -.35), 6, 'torso')
        box((.45, 1.5, .0), (.15, .02, .22), 'ward_flesh', 'torso', rot=(0, 0, 12))
    elif name == 'suspended':
        # Bandaged head: wraps round a narrow skull, one violet slit showing through
        oval((0, 2.47, 0), (.27, .33, .29), 'ward_bandage', 'head')
        for t in range(6):
            box((0, 2.32 + t * .055, .0), (.3, .028, .31), 'ward_bandage' if t % 2 else 'ward_canvas', 'head', rot=(t * 7 - 15, t * 23, 3 - t * 2))
        box((0, 2.5, .155), (.19, .028, .02), 'glow', 'head', glow='eye', k=1.6)
        box((.13, 2.25, .08), (.06, .2, .02), 'ward_bandage', 'head', rot=(0, 0, -15))      # a trailing end
        oval((0, 2.31, .07), (.18, .08, .16), 'ward_bandage', 'jaw')
        # Straitjacket body, buckled straps across it, a hole the core burns through
        oval((0, 1.84, 0), (.54, .5, .36), 'ward_canvas', 'torso')
        oval((0, 1.42, 0), (.42, .36, .3), 'ward_canvas', 'torso')
        for y in (1.62, 1.95):
            box((0, y, .17), (.5, .06, .03), 'ward_strap', 'torso')
            box((.08, y, .19), (.06, .07, .02), 'ward_buckle', 'torso')
        oval((0, 1.86, .17), (.24, .26, .1), 'ward_flesh', 'torso')
        oval((0, 1.86, .2), (.18, .2, .1), 'glow', 'torso', glow='core', k=1.3)
        # The harness and the hoist it hangs from
        for s in (-1, 1):
            R(p((s * .28, 2.16, 0)), rad(.06), rad(.012), 'ward_steel', 'torso', rot=(0, 90, 0), sides=12, tsides=4)
        R(p((0, 3.15, 0)), rad(.42), rad(.035), 'ward_steel', 'torso', rot=(90, 0, 0), sides=28, tsides=6)
        for t in range(6):
            a = t / 6 * math.tau
            box((math.cos(a) * .42, 3.13, math.sin(a) * .42), (.05, .03, .05), 'glow', 'torso', glow='eye', k=1.2)
        for a, b in (((.28, 2.18, .05), (.3, 3.12, .29)), ((-.28, 2.18, .05), (-.3, 3.12, .29)),
                     ((.22, 2.1, -.15), (.3, 3.12, -.29)), ((-.22, 2.1, -.15), (-.3, 3.12, -.29))):
            band(a, b, .012, 'ward_steel', 'torso', sides=5)
        # An open cage hanging round it from the hoist
        for t in range(5):
            a = (t + .5) / 5 * math.tau
            band((math.cos(a) * .42, 3.13, math.sin(a) * .42), (math.cos(a) * .52, 1.3, math.sin(a) * .42), .012, 'ward_steel', 'torso', sides=5)
    else:   # phased
        oval((0, 2.5, -.01), (.25, .38, .28), skin, 'head')
        mouth(.85)
        # Split down the middle by light, three eyes on the fracture
        box((0, 2.5, .145), (.012, .32, .02), 'glow', 'head', glow='seam', k=1.4)
        for t in range(3):
            oval((0, 2.62 - t * .085, .14), (.05, .03, .03), 'glow', 'head', glow='eye', k=1.6 - t * .3)
        for s in (-1, 1):
            box((s * .08, 2.52, .13), (.12, .22, .03), 'ward_shard', 'head', rot=(0, s * -15, s * 4), bev=.008)
        box((0, 1.75, .17), (.014, .7, .02), 'glow', 'torso', glow='seam', k=1.3)
        for t in range(5):
            y = 1.5 + t * .12
            box((t % 2 * .06 - .03, y, .17), (.12, .01, .015), 'glow', 'torso', glow='seam', k=1, rot=(0, 0, 30 if t % 2 else -30))
        # Shards of it, hanging just off the body
        for (v, s, r, par) in (((.32, 2.3, -.05), (.14, .2, .03), (10, 30, 20), 'torso'), ((-.34, 2.25, .0), (.12, .18, .03), (-15, -25, -30), 'torso'),
                               ((.2, 2.75, -.05), (.1, .16, .03), (20, 10, 35), 'head'), ((-.22, 2.72, .02), (.09, .14, .03), (-25, -10, -40), 'head'),
                               ((.0, 2.82, -.12), (.12, .1, .03), (40, 0, 0), 'head'), ((.42, 1.75, .05), (.08, .22, .03), (0, 20, 15), 'torso')):
            box(v, s, 'ward_shard', par, rot=r, bev=.006)
            box((v[0], v[1] - s[1] * .5, v[2] + .02), (s[0], .01, .012), 'glow', par, glow='seam', k=1.2, rot=r)
