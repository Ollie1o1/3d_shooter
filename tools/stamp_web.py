#!/usr/bin/env python3
"""Give a compiled web release one identity shared by its loader and assets."""
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path


def stamp(directory):
    directory = Path(directory)
    assets = {}
    identity = hashlib.sha256()
    for name in ("overdrive.js", "overdrive.wasm", "overdrive.data"):
        content = (directory / name).read_bytes()
        digest = hashlib.sha256(content).hexdigest()
        assets[name] = {"sha256": digest, "bytes": len(content)}
        identity.update((name + ":" + digest + "\n").encode())
    build = identity.hexdigest()[:12]
    shell = (Path(__file__).resolve().parent.parent / "web/index.html").read_text()
    if "__OVERDRIVE_BUILD__" not in shell:
        raise ValueError("web shell is missing its release marker")
    content = shell.replace("__OVERDRIVE_BUILD__", build).encode()
    (directory / "index.html").write_bytes(content)
    assets["index.html"] = {"sha256": hashlib.sha256(content).hexdigest(), "bytes": len(content)}
    manifest = {"build": build, "built_at": datetime.now(timezone.utc).isoformat(), "assets": assets}
    (directory / "build-info.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("OVERDRIVE web build " + build)
    return manifest


if __name__ == "__main__":
    stamp(sys.argv[1] if len(sys.argv) > 1 else "web/dist")
