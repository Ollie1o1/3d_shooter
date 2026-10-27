#!/usr/bin/env python3
"""
Shrink .glb files losslessly: every accessor whose bytes match an earlier one is replaced by it (Blender writes
a rivet, a bevelled box, a cylinder of the same size once per part), then every mesh whose primitives match an
earlier one's is replaced by it, and what's left unused is dropped. Nothing the game reads changes: the same
nodes, transforms, materials and extras; only shared data.

    python3 tools/glb_dedupe.py assets/enemies/*.glb assets/guns/*.glb assets/props/*.glb
    python3 tools/glb_dedupe.py --check FILE...   (just report what it would save)

Stdlib only.
"""
import hashlib
import json
import struct
import sys

COMP = {5120: 1, 5121: 1, 5122: 2, 5123: 2, 5125: 4, 5126: 4}
NUM = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT2": 4, "MAT3": 9, "MAT4": 16}


def read_glb(path):
    data = open(path, "rb").read()
    magic, version, length = struct.unpack_from("<III", data, 0)
    assert magic == 0x46546C67 and version == 2, path + ": not a glTF 2 binary"
    off, js, bin_ = 12, None, b""
    while off < length:
        clen, ctype = struct.unpack_from("<II", data, off)
        chunk = data[off + 8: off + 8 + clen]
        if ctype == 0x4E4F534A:
            js = json.loads(chunk.decode("utf-8"))
        elif ctype == 0x004E4942:
            bin_ = chunk
        off += 8 + clen
    return js, bin_


def write_glb(path, js, bin_):
    j = json.dumps(js, separators=(",", ":")).encode("utf-8")
    j += b" " * ((4 - len(j) % 4) % 4)
    b = bin_ + b"\0" * ((4 - len(bin_) % 4) % 4)
    total = 12 + 8 + len(j) + (8 + len(b) if b else 0)
    out = struct.pack("<III", 0x46546C67, 2, total) + struct.pack("<II", len(j), 0x4E4F534A) + j
    if b:
        out += struct.pack("<II", len(b), 0x004E4942) + b
    open(path, "wb").write(out)
    return total


def accessor_bytes(js, bin_, a):
    bv = js["bufferViews"][a["bufferView"]]
    size = COMP[a["componentType"]] * NUM[a["type"]]
    stride = bv.get("byteStride", size)
    start = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
    if stride == size:
        return bin_[start: start + size * a["count"]]
    return b"".join(bin_[start + i * stride: start + i * stride + size] for i in range(a["count"]))


def dedupe(js, bin_):
    accs = js.get("accessors", [])
    if not accs:
        return js, bin_
    # 1. accessors: the same bytes (and the same layout) are the same accessor
    first, remap = {}, {}
    for i, a in enumerate(accs):
        if "bufferView" not in a or a.get("sparse"):
            remap[i] = i
            continue
        key = (a["componentType"], a["type"], a["count"], a.get("normalized", False),
               hashlib.sha1(accessor_bytes(js, bin_, a)).hexdigest())
        remap[i] = first.setdefault(key, i)
    for m in js.get("meshes", []):
        for p in m["primitives"]:
            p["attributes"] = {k: remap[v] for k, v in p["attributes"].items()}
            if "indices" in p:
                p["indices"] = remap[p["indices"]]
            for t in p.get("targets", []):
                for k in t:
                    t[k] = remap[t[k]]
    # 2. meshes: the same primitives (after that) are the same mesh
    mfirst, mremap = {}, {}
    for i, m in enumerate(js.get("meshes", [])):
        key = json.dumps(m["primitives"], sort_keys=True) + json.dumps(m.get("weights"))
        mremap[i] = mfirst.setdefault(key, i)
    for n in js.get("nodes", []):
        if "mesh" in n:
            n["mesh"] = mremap[n["mesh"]]
    # 3. keep only what's used, renumbered, and the bytes those accessors need
    used_m = sorted({n["mesh"] for n in js.get("nodes", []) if "mesh" in n})
    mnew = {old: k for k, old in enumerate(used_m)}
    js["meshes"] = [js["meshes"][i] for i in used_m]
    for n in js.get("nodes", []):
        if "mesh" in n:
            n["mesh"] = mnew[n["mesh"]]
    used_a = sorted({v for m in js["meshes"] for p in m["primitives"]
                     for v in list(p["attributes"].values()) + ([p["indices"]] if "indices" in p else [])})
    anew = {old: k for k, old in enumerate(used_a)}
    out, views, new_accs = bytearray(), [], []
    for old in used_a:
        a = dict(accs[old])
        raw = accessor_bytes(js, bin_, accs[old])
        while len(out) % 4:
            out.append(0)
        views.append({"buffer": 0, "byteOffset": len(out), "byteLength": len(raw)})
        out += raw
        a["bufferView"] = len(views) - 1
        a.pop("byteOffset", None)
        new_accs.append(a)
    for m in js["meshes"]:
        for p in m["primitives"]:
            p["attributes"] = {k: anew[v] for k, v in p["attributes"].items()}
            if "indices" in p:
                p["indices"] = anew[p["indices"]]
    if any("bufferView" in im for im in js.get("images", [])):
        return None, None   # (images in the buffer: leave such a file alone)
    js["accessors"], js["bufferViews"] = new_accs, views
    js["buffers"] = [{"byteLength": len(out)}]
    return js, bytes(out)


def main(argv):
    check = "--check" in argv
    files = [a for a in argv if a != "--check"]
    before = after = 0
    for f in files:
        js, bin_ = read_glb(f)
        size = len(open(f, "rb").read())
        js2, bin2 = dedupe(js, bin_)
        if js2 is None:
            print("%s: skipped (embedded images)" % f)
            continue
        if check:
            j = json.dumps(js2, separators=(",", ":")).encode("utf-8")
            new = 12 + 8 + len(j) + 8 + len(bin2) + 8
        else:
            new = write_glb(f, js2, bin2)
        before += size
        after += new
        print("%-40s %8d -> %8d bytes (%d%%)" % (f, size, new, round(100 * new / max(1, size))))
    print("total %d -> %d bytes (%.1f MB -> %.1f MB)" % (before, after, before / 1e6, after / 1e6))


if __name__ == "__main__":
    main(sys.argv[1:])
