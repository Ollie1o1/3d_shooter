# Keyframed boss moves (render/AnimClip.h): each rig's clips into assets/enemies/anim/<rig>.glb.
#
#   blender -b --factory-startup --python tools/blender/anim_build.py               (every rig's clips)
#   blender --python tools/blender/anim_build.py -- sovereign                       (open it to work on)
#   blender -b build/anim/sovereign.blend --python tools/blender/anim_build.py -- --export-only sovereign
#
# A rig's model (assets/enemies/<rig>.glb) comes in for looking at; each clip is one Blender action over
# its joints (the empties named as the game's joints: torso, head, armL, armR, weapon...), and "root"
# (renamed from the model's top node) moves the whole body. Only the joints and their keys go out: the
# model file is never touched. build/anim/<rig>.blend is saved to keep working in: change the keys in
# the Dope Sheet, then --export-only writes the file from it.
#
# A clip plays over its move's wind-up from start to end, however long the game makes that, so frames
# are just its timing (24 to the second; a clip 24 frames long).
#
# Keys below are written in the GAME's terms (as the code poses a humanoid, EnemyModel.h): arms turn
# RX(rx) * RZ(rz) (rx < 0 raises them forward and up), the torso RY(yaw) * RX(pitch) (pitch < 0 leans
# back), the head RX(pitch), the sword ("weapon") RX(grip), hips RX(swing), root a lift in metres.
import math
import os
import sys

import bpy
from mathutils import Quaternion

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FPS = 24


def gx(a):   # game RX
    return Quaternion((1, 0, 0), a)


def gy(a):   # game RY (game Y is Blender Z)
    return Quaternion((0, 0, 1), a)


def gz(a):   # game RZ (game Z is Blender -Y)
    return Quaternion((0, -1, 0), a)


# ---- THE SOVEREIGN: JUDGMENT, the sword raised to the sky to call the blades down -----------------
# A gather (low, weight back, blade dragged behind), the drive up, an overshoot arched back on his
# toes, then the hold the code's follow-through swings down from (EnemyModel.h: sovereignPose)
SOVEREIGN_JUDGMENT = [
    # (time 0..1, {pose})
    (0.00, dict(lift=0.00, yaw=0.00, pitch=0.00, head=0.00, rxR=-0.45, rzR=0.00, rxL=0.00, rzL=0.00, grip=-1.05, hipL=0.00, hipR=0.00)),
    (0.28, dict(lift=-0.28, yaw=-0.22, pitch=0.32, head=0.25, rxR=0.55, rzR=-0.20, rxL=0.35, rzL=0.10, grip=-1.50, hipL=0.35, hipR=-0.30)),
    (0.55, dict(lift=-0.10, yaw=0.10, pitch=-0.10, head=-0.10, rxR=-1.90, rzR=-0.05, rxL=-1.60, rzL=0.05, grip=-0.70, hipL=0.15, hipR=-0.12)),
    (0.74, dict(lift=0.06, yaw=0.05, pitch=-0.42, head=-0.45, rxR=-3.30, rzR=0.00, rxL=-3.15, rzL=0.12, grip=0.20, hipL=0.05, hipR=-0.05)),
    (1.00, dict(lift=0.00, yaw=0.00, pitch=-0.32, head=-0.35, rxR=-3.05, rzR=0.00, rxL=-2.95, rzL=0.00, grip=0.00, hipL=0.00, hipR=0.00)),
]


def humanoid_keys(pose):
    """A humanoid pose (game terms) as each joint's turn and the root's lift (Blender terms)"""
    s = 0.08   # (the code's arms sit out a little: armRz = side * 0.08 + rz)
    return {
        "torso": gy(pose["yaw"]) @ gx(pose["pitch"]),
        "head": gx(pose["head"]),
        "armR": gx(pose["rxR"]) @ gz(-s + pose["rzR"]),
        "armL": gx(pose["rxL"]) @ gz(s + pose["rzL"]),
        "weapon": gx(pose["grip"]),
        "hipL": gx(pose["hipL"]),
        "hipR": gx(pose["hipR"]),
    }, pose["lift"]


RIGS = {
    "sovereign": {"judgment": (SOVEREIGN_JUDGMENT, humanoid_keys)},
}


def load_rig(rig):
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)
    bpy.ops.import_scene.gltf(filepath=os.path.join(REPO, "assets", "enemies", rig + ".glb"))
    tops = [o for o in bpy.data.objects if o.parent is None]
    if len(tops) != 1:
        raise SystemExit("%s.glb: expected one top node, found %d" % (rig, len(tops)))
    tops[0].name = "root"
    for ob in bpy.data.objects:
        ob.rotation_mode = "QUATERNION"
        ob["od_rest"] = list(ob.rotation_quaternion)   # (keys are turns from here)
        ob["od_rest_loc"] = list(ob.location)
    sc = bpy.context.scene
    sc.render.fps = FPS


def key_clip(rig, name, keys, to_joints):
    act = bpy.data.actions.new(name)
    last = 0
    for t, pose in keys:
        frame = round(t * FPS)
        last = max(last, frame)
        turns, lift = to_joints(pose)
        for jn, q in list(turns.items()) + [("root", None)]:
            ob = bpy.data.objects.get(jn)
            if ob is None:
                raise SystemExit("%s has no joint '%s'" % (rig, jn))
            if ob.animation_data is None:
                ob.animation_data_create()
            if ob.animation_data.action is not act:
                ob.animation_data.action = act
            if jn == "root":
                rl = ob["od_rest_loc"]
                ob.location = (rl[0], rl[1], rl[2] + lift)
                ob.keyframe_insert("location", frame=frame)
            else:
                ob.rotation_quaternion = Quaternion(ob["od_rest"]) @ q
                ob.keyframe_insert("rotation_quaternion", frame=frame)
    bpy.context.scene.frame_start, bpy.context.scene.frame_end = 0, last
    return act


def export(rig):
    out = os.path.join(REPO, "assets", "enemies", "anim", rig + ".glb")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    keep = [o for o in bpy.data.objects if o.type == "EMPTY"]
    for o in keep:
        o.select_set(True)
    bpy.ops.export_scene.gltf(filepath=out, export_format="GLB", use_selection=True, export_yup=True,
                              export_animations=True, export_animation_mode="ACTIONS", export_force_sampling=True,
                              export_frame_range=False, export_extras=True, export_apply=False)   # (extras: each joint's true rest)
    print("anim_build: %s (%d joints)" % (out, len(keep)))


def build(rig):
    load_rig(rig)
    for name, (keys, to_joints) in RIGS[rig].items():
        key_clip(rig, name, keys, to_joints)
    blend = os.path.join(REPO, "build", "anim", rig + ".blend")
    os.makedirs(os.path.dirname(blend), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=blend)
    export(rig)


if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if "--export-only" in args:
        args.remove("--export-only")
        export(args[0] if args else bpy.path.basename(bpy.data.filepath).split(".")[0])
    else:
        for rig in (args or list(RIGS)):
            build(rig)
