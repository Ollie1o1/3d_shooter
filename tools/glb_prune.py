#!/usr/bin/env python3
"""
Keep only a .glb's default scene: the nodes it reaches, their meshes, materials, accessors and data.

Blender's glTF exporter, run over the live link, can write every open scene into the file (its "active scene"
option notwithstanding). The game reads only the default scene, so the rest is dead weight: this drops it.
The rifle build scripts run it after each save (tools/blender/rifle_kit.py); by hand:

    python3 tools/glb_prune.py assets/guns/lancer.glb [more.glb ...]

Standard library only (it also runs inside Blender's Python).
"""
import json
import struct
import sys


def prune(path):
    data = open(path, "rb").read()
    magic, version, _ = struct.unpack_from("<III", data, 0)
    assert magic == 0x46546C67, path + ": not a glb"
    jlen = struct.unpack_from("<I", data, 12)[0]
    doc = json.loads(data[20:20 + jlen])
    at = 20 + jlen
    blen = struct.unpack_from("<I", data, at)[0]
    binary = data[at + 8:at + 8 + blen]
    scenes = doc.get("scenes", [])
    if len(scenes) <= 1:
        return len(data), len(data)
    d = doc.get("scene", 0)
    keep_scene = scenes[d]

    # nodes reachable from the default scene, in order
    nodes, order, seen = doc["nodes"], [], set()

    def walk(i):
        if i in seen:
            return
        seen.add(i)
        order.append(i)
        for c in nodes[i].get("children", []):
            walk(c)
    for r in keep_scene.get("nodes", []):
        walk(r)
    nmap = {old: new for new, old in enumerate(order)}

    mmap = {}
    materials, matmap = doc.get("materials", []), {}
    accessors, amap = doc.get("accessors", []), {}
    views, vmap = doc.get("bufferViews", []), {}
    out_nodes, out_meshes, out_mats, out_acc, out_views = [], [], [], [], []
    out_bin = bytearray()

    def view(i):
        if i not in vmap:
            v = dict(views[i])
            chunk = binary[v.get("byteOffset", 0):v.get("byteOffset", 0) + v["byteLength"]]
            while len(out_bin) % 4:
                out_bin.append(0)
            v["byteOffset"] = len(out_bin)
            v["buffer"] = 0
            out_bin.extend(chunk)
            vmap[i] = len(out_views)
            out_views.append(v)
        return vmap[i]

    def accessor(i):
        if i not in amap:
            a = dict(accessors[i])
            if "bufferView" in a:
                a["bufferView"] = view(a["bufferView"])
            amap[i] = len(out_acc)
            out_acc.append(a)
        return amap[i]

    def material(i):
        if i not in matmap:
            matmap[i] = len(out_mats)
            out_mats.append(materials[i])
        return matmap[i]

    def mesh(i):
        if i not in mmap:
            m = dict(doc["meshes"][i])
            prims = []
            for p in m["primitives"]:
                p = dict(p)
                p["attributes"] = {k: accessor(v) for k, v in p["attributes"].items()}
                if "indices" in p:
                    p["indices"] = accessor(p["indices"])
                if "material" in p:
                    p["material"] = material(p["material"])
                prims.append(p)
            m["primitives"] = prims
            mmap[i] = len(out_meshes)
            out_meshes.append(m)
        return mmap[i]

    for old in order:
        n = dict(nodes[old])
        if "children" in n:
            n["children"] = [nmap[c] for c in n["children"]]
        if "mesh" in n:
            n["mesh"] = mesh(n["mesh"])
        out_nodes.append(n)

    s = dict(keep_scene)
    s["nodes"] = [nmap[r] for r in keep_scene.get("nodes", [])]
    doc["scenes"], doc["scene"] = [s], 0
    doc["nodes"], doc["meshes"], doc["accessors"], doc["bufferViews"] = out_nodes, out_meshes, out_acc, out_views
    if out_mats:
        doc["materials"] = out_mats
    else:
        doc.pop("materials", None)
    while len(out_bin) % 4:
        out_bin.append(0)
    doc["buffers"] = [{"byteLength": len(out_bin)}]

    js = json.dumps(doc, separators=(",", ":")).encode()
    while len(js) % 4:
        js += b" "
    out = struct.pack("<III", 0x46546C67, version, 12 + 8 + len(js) + 8 + len(out_bin))
    out += struct.pack("<II", len(js), 0x4E4F534A) + js + struct.pack("<II", len(out_bin), 0x004E4942) + bytes(out_bin)
    open(path, "wb").write(out)
    return len(data), len(out)


if __name__ == "__main__":
    for p in sys.argv[1:]:
        before, after = prune(p)
        print("%s: %d -> %d bytes" % (p, before, after))
