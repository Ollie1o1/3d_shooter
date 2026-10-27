# Shared by the gun build scripts (tools/blender/lancer_build.py, longshot_build.py): each starts with
#   exec(open(RIFLE_KIT).read())
#   root = new_gun("lancer", [("bolt", (x, y, z), None), ...])   a scene of its own, the root and the joints
#   ... B / C / E / P / R / K / W / tube (enemy_kit.py's shapes, in gun space) ...
#   save_built("lancer")                                             checked and saved to assets/guns/lancer.glb
#
# Gun space: +Y up, +Z along the barrel, metres; the side you see in your hands is -X (the left).
# Blender's object names are shared by every scene, so an object in another open scene that already
# uses one of the gun's names (a 'cells' socket, say) is moved aside while the gun is built and saved,
# then given its name back; this gun's own joints keep a ".<gun>" suffix afterwards (Open the gun from
# the OVERDRIVE panel to work on it by hand).
exec(open("/Users/ollie/Projects/overdrive/3d_shooter/tools/blender/enemy_kit.py").read())

_AWAY = {}


def new_gun(name, joints):
    sc = bpy.data.scenes.get(name.upper()) or bpy.data.scenes.new(name.upper())
    bpy.context.window_manager.windows[0].scene = sc
    for ob in list(sc.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    _AWAY.clear()
    for n in [name] + [j[0] for j in joints]:
        ob = bpy.data.objects.get(n)
        if ob is not None:
            _AWAY[n] = ob
            ob.name = n + "__away"
    root = bpy.data.objects.new(name, None)
    sc.collection.objects.link(root)
    for jn, at, parent in joints:
        add_joint(jn, at, parent=find(parent) if parent else root)
    sc.overdrive_repo = REPO
    sc.overdrive_gun = name
    sc["overdrive_opened"] = name
    return root


def save_built(name):
    """Saved by the add-on (checked first), then pruned to this gun's scene: the exporter, run over the live
    link, can write every open scene into the file (tools/glb_prune.py)"""
    sc = bpy.data.scenes[name.upper()]
    path, warnings = _og.save_gun(name)
    _prune = {}
    exec(open(os.path.join(REPO, "tools", "glb_prune.py")).read(), _prune)
    before, after = _prune["prune"](path)
    if after < before:
        print("(%s: other scenes dropped from the file, %d -> %d bytes)" % (name, before, after))
    shrink(path)
    n = sum(1 for o in sc.objects if o.type == "MESH")
    print("%s: %d parts saved to %s" % (name, n, path))
    for w in warnings:
        print("  warning:", w)
    if _AWAY:   # (the other scene's objects get their names back)
        for n_, ob in _AWAY.items():
            mine = bpy.data.objects.get(n_)
            if mine is not None and mine != ob:
                mine.name = n_ + "." + name
            ob.name = n_
        _AWAY.clear()
