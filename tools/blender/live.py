#!/usr/bin/env python3
"""
Send Python to the Blender that has the OVERDRIVE add-on's Live link on, and
print what it says back. Each script is one undo step in Blender.

    python3 tools/blender/live.py -c "print(describe(10))"
    python3 tools/blender/live.py edit.py
    python3 tools/blender/live.py --status

The script runs inside Blender with bpy and the add-on's helpers (add_box,
add_joint, set_glow, set_show, find, game_box, describe, open_gun, save_gun,
check_scene), which work in game space: +Y up, +Z along the barrel, metres.
Stdlib only. Exit code 0 if the script ran, 1 if it raised, 2 if Blender
didn't answer.
"""
import argparse
import json
import os
import sys
import time
import uuid

LIVE_DIR = os.environ.get("OVERDRIVE_LIVE_DIR") or os.path.join(os.path.expanduser("~"), ".overdrive_blender")


def status():
    """The live link's status, or None if it's off (or the Blender that turned it on is gone)"""
    try:
        with open(os.path.join(LIVE_DIR, "status.json"), encoding="utf-8") as f:
            s = json.load(f)
        os.kill(int(s["pid"]), 0)   # (signal 0: just "is that process still there?")
        return s
    except PermissionError:
        return s
    except (OSError, ValueError, KeyError, TypeError):
        return None


def send(code, label="edit", timeout=30.0):
    inbox, outbox = os.path.join(LIVE_DIR, "inbox"), os.path.join(LIVE_DIR, "outbox")
    for d in (LIVE_DIR, inbox, outbox):
        os.makedirs(d, mode=0o700, exist_ok=True)
    name = "%s-%s-%s" % (time.strftime("%H%M%S"), label, uuid.uuid4().hex[:6])
    tmp = os.path.join(inbox, name + ".part")
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(code)
    os.replace(tmp, os.path.join(inbox, name + ".py"))   # whole files only: Blender never sees half a script
    reply = os.path.join(outbox, name + ".json")
    end = time.time() + timeout
    while time.time() < end:
        if os.path.exists(reply):
            with open(reply, encoding="utf-8") as f:
                r = json.load(f)
            os.remove(reply)
            return r
        time.sleep(0.1)
    # Not picked up: take it back so it doesn't run later, by surprise
    try:
        os.remove(os.path.join(inbox, name + ".py"))
    except OSError:
        pass
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", nargs="?", help="a Python file to run in Blender")
    ap.add_argument("-c", "--code", help="Python to run in Blender")
    ap.add_argument("--label", default="edit", help="what Blender's undo history calls it")
    ap.add_argument("--timeout", type=float, default=30.0)
    ap.add_argument("--status", action="store_true", help="is the live link on?")
    a = ap.parse_args()
    if a.status:
        s = status()
        print("live link on (Blender pid %s)" % s.get("pid") if s else "live link off")
        return 0 if s else 2
    if a.code is None and a.file is None:
        ap.error("give a file or -c CODE")
    code = a.code if a.code is not None else open(a.file, encoding="utf-8").read()
    if status() is None:
        print("The live link is off: open Blender, and in the OVERDRIVE panel click Live link.", file=sys.stderr)
        return 2
    r = send(code, a.label, a.timeout)
    if r is None:
        print("Blender didn't answer in %.0f s: is it open, with the OVERDRIVE panel's Live link on?" % a.timeout, file=sys.stderr)
        return 2
    sys.stdout.write(r.get("output", ""))
    if not r.get("ok"):
        sys.stderr.write(r.get("error", ""))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
