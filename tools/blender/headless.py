# Run a build script in Blender with no window (no live link needed):
#
#   blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/warden_build.py [NAME=value ...]
#
# It loads the OVERDRIVE add-on's helpers and runs the script with the same names the live link gives it
# (add_box, find, set_glow, palette_material...). NAME=value pairs become globals first (SKELETON=/path.glb).
# Exits 1 if the script raised.
import os
import sys
import traceback

import bpy
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import overdrive_guns as og  # noqa: E402  (registers itself in sys.modules: enemy_kit finds it there)

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
if not args:
    print("usage: blender -b --factory-startup --python headless.py -- SCRIPT.py [NAME=value ...]")
    sys.exit(2)
script, extra = args[0], args[1:]

env = {"bpy": bpy, "Vector": Vector, "Matrix": Matrix, "__name__": "__overdrive__"}
env.update({k: getattr(og, k) for k in (
    "add_box", "add_joint", "set_glow", "set_show", "find", "game_box", "describe", "open_gun",
    "save_gun", "check_scene", "palette_material", "to_blender", "to_game", "JOINTS", "CHANNELS",
    "FLAGS", "PALETTE") if hasattr(og, k)})
for kv in extra:
    k, _, v = kv.partition("=")
    env[k] = v
try:
    with open(script, encoding="utf-8") as f:
        exec(compile(f.read(), script, "exec"), env)
except Exception:
    traceback.print_exc()
    sys.exit(1)
