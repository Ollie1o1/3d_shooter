# OVERDRIVE guns: a Blender add-on for working on assets/guns/*.glb
#
# Install: Blender > Edit > Preferences > Add-ons > Install... > pick this file,
# tick "OVERDRIVE guns". The panel is in the 3D view's sidebar (N) > OVERDRIVE.
#
#   Open gun    imports assets/guns/<gun>.glb, sets millimetre units and a
#               near clip small enough for 1 mm parts
#   Save gun    exports the scene back to that file (GLB, +Y up, custom
#               properties on). Run ./shooter --viewer <gun>: it reloads on save
#   Add box     a 1 m cube at the 3D cursor, in a palette material, under the
#               selected joint (scale it in Object Mode)
#   Add joint   an empty with a joint name (crane, pump, muzzle...) at the cursor
#   Glow / Show set or clear the selected boxes' glow channel and show flag
#
# The rules the game reads (src/render/GunFile.h has them all):
#   - every mesh is a box: the game takes its bounds, nothing finer
#   - a node named after a joint is that joint; its children ride it
#   - material "glow" / "glow*0.5" is the gun's glow colour; any other uses its base colour
#   - custom properties: glow (channel), k (strength), phase, show ("loaded", "!bigmag")
bl_info = {
    "name": "OVERDRIVE guns",
    "author": "OVERDRIVE",
    "version": (1, 0, 0),
    "blender": (4, 0, 0),
    "location": "View3D > Sidebar > OVERDRIVE",
    "description": "Open, edit and save OVERDRIVE's guns (assets/guns/*.glb)",
    "category": "Import-Export",
}

import os
import bpy

GUNS = ["revolver", "shotgun", "lancer", "longshot"]
JOINTS = ["crane", "cylinder", "hammer", "trigger", "needle", "pump", "bolt", "mag", "muzzle", "cells"]
CHANNELS = ["const", "glow", "accent", "breath", "stripe", "rim", "needle", "port"]
FLAGS = ["loaded", "!loaded", "bigmag", "!bigmag"]
# The arsenal's palette (GunKit.h): blackened gunmetal, worn steel, edges, bone, brass, and the glow
PALETTE = {
    "black": (0.10, 0.10, 0.11), "steel": (0.32, 0.32, 0.34), "edge": (0.46, 0.46, 0.49),
    "bone": (0.76, 0.71, 0.61), "brass": (0.70, 0.52, 0.24), "glow": (1.0, 0.68, 0.22),
}


def guns_dir(folder):
    """The assets/guns folder, whether Repo points at the repo, at assets, at assets/guns or inside it"""
    d = os.path.normpath(bpy.path.abspath(folder or "."))
    for _ in range(4):
        if os.path.basename(d) == "guns" and os.path.basename(os.path.dirname(d)) == "assets":
            return d
        if os.path.isdir(os.path.join(d, "assets", "guns")):
            return os.path.join(d, "assets", "guns")
        if os.path.basename(d) == "assets" and os.path.isdir(os.path.join(d, "guns")):
            return os.path.join(d, "guns")
        d = os.path.dirname(d)
    return os.path.join(os.path.normpath(bpy.path.abspath(folder or ".")), "assets", "guns")


def gun_path(scene):
    return os.path.join(guns_dir(scene.overdrive_repo), scene.overdrive_gun + ".glb")


def palette_material(name):
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    base = name.split("*")[0]
    shade = float(name.split("*")[1]) if "*" in name else 1.0
    rgb = PALETTE.get(base, (0.5, 0.5, 0.5))
    col = (min(1.0, rgb[0] * shade), min(1.0, rgb[1] * shade), min(1.0, rgb[2] * shade), 1.0)
    if bsdf:
        bsdf.inputs["Base Color"].default_value = col
        bsdf.inputs["Roughness"].default_value = 1.0
    m.diffuse_color = col
    return m


def near_clip(context):
    for area in context.screen.areas:
        if area.type == "VIEW_3D":
            for space in area.spaces:
                if space.type == "VIEW_3D":
                    space.clip_start = 0.0005


class OVERDRIVE_OT_open(bpy.types.Operator):
    bl_idname = "overdrive.open_gun"
    bl_label = "Open gun"
    bl_description = "Import assets/guns/<gun>.glb into this scene"

    def execute(self, context):
        path = gun_path(context.scene)
        if not os.path.exists(path):
            self.report({"ERROR"}, "No file at " + path + " (set the repo folder)")
            return {"CANCELLED"}
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=path)
        # The file shares one cube mesh per material: give each box its own, so
        # editing one never changes the rest
        for ob in set(bpy.data.objects) - before:
            if ob.type == "MESH" and ob.data.users > 1:
                ob.data = ob.data.copy()
        context.scene.unit_settings.length_unit = "MILLIMETERS"
        near_clip(context)
        self.report({"INFO"}, "Opened " + path)
        return {"FINISHED"}


class OVERDRIVE_OT_save(bpy.types.Operator):
    bl_idname = "overdrive.save_gun"
    bl_label = "Save gun"
    bl_description = "Export this scene to assets/guns/<gun>.glb (the viewer and the game read it)"

    def execute(self, context):
        path = gun_path(context.scene)
        bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", export_extras=True,
                                  export_yup=True, export_apply=True, use_selection=False)
        self.report({"INFO"}, "Saved " + path)
        return {"FINISHED"}


class OVERDRIVE_OT_add_box(bpy.types.Operator):
    bl_idname = "overdrive.add_box"
    bl_label = "Add box"
    bl_description = "A box at the 3D cursor, under the selected joint, in the chosen material"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        parent = context.active_object
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=context.scene.cursor.location)
        ob = context.active_object
        ob.scale = (0.01, 0.01, 0.01)
        ob.data.materials.append(palette_material(context.scene.overdrive_material))
        ob.name = context.scene.overdrive_material
        if parent and parent != ob:
            world = ob.matrix_world.copy()
            ob.parent = parent
            ob.matrix_world = world
        return {"FINISHED"}


class OVERDRIVE_OT_add_joint(bpy.types.Operator):
    bl_idname = "overdrive.add_joint"
    bl_label = "Add joint"
    bl_description = "An empty named for a joint, at the 3D cursor, under the selection"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        name = context.scene.overdrive_joint
        if name in bpy.data.objects:
            self.report({"ERROR"}, "There's already a '" + name + "' (a joint name can be used once)")
            return {"CANCELLED"}
        parent = context.active_object
        bpy.ops.object.empty_add(type="ARROWS", radius=0.01, location=context.scene.cursor.location)
        ob = context.active_object
        ob.name = name
        if parent and parent != ob:
            world = ob.matrix_world.copy()
            ob.parent = parent
            ob.matrix_world = world
        return {"FINISHED"}


class OVERDRIVE_OT_glow(bpy.types.Operator):
    bl_idname = "overdrive.set_glow"
    bl_label = "Set glow"
    bl_description = "The selected boxes glow on this channel, this strongly"
    bl_options = {"REGISTER", "UNDO"}
    clear: bpy.props.BoolProperty(default=False)

    def execute(self, context):
        s = context.scene
        for ob in context.selected_objects:
            if ob.type != "MESH":
                continue
            for key in ("glow", "k", "phase"):
                if key in ob:
                    del ob[key]
            if not self.clear:
                ob["glow"] = s.overdrive_channel
                if abs(s.overdrive_k - 1.0) > 1e-6:
                    ob["k"] = s.overdrive_k
        return {"FINISHED"}


class OVERDRIVE_OT_show(bpy.types.Operator):
    bl_idname = "overdrive.set_show"
    bl_label = "Set show"
    bl_description = "The selected boxes show only while this flag holds"
    bl_options = {"REGISTER", "UNDO"}
    clear: bpy.props.BoolProperty(default=False)

    def execute(self, context):
        for ob in context.selected_objects:
            if "show" in ob:
                del ob["show"]
            if not self.clear and ob.type == "MESH":
                ob["show"] = context.scene.overdrive_flag
        return {"FINISHED"}


class OVERDRIVE_PT_panel(bpy.types.Panel):
    bl_label = "OVERDRIVE guns"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "OVERDRIVE"

    def draw(self, context):
        s = context.scene
        col = self.layout.column()
        col.prop(s, "overdrive_repo")
        col.prop(s, "overdrive_gun")
        row = col.row()
        row.operator("overdrive.open_gun")
        row.operator("overdrive.save_gun")
        col.separator()
        col.prop(s, "overdrive_material")
        col.operator("overdrive.add_box")
        col.prop(s, "overdrive_joint")
        col.operator("overdrive.add_joint")
        col.separator()
        col.prop(s, "overdrive_channel")
        col.prop(s, "overdrive_k")
        row = col.row()
        row.operator("overdrive.set_glow", text="Glow").clear = False
        row.operator("overdrive.set_glow", text="No glow").clear = True
        col.prop(s, "overdrive_flag")
        row = col.row()
        row.operator("overdrive.set_show", text="Show on flag").clear = False
        row.operator("overdrive.set_show", text="Always").clear = True
        ob = context.active_object
        if ob:
            col.separator()
            col.label(text=ob.name + ("  (joint)" if ob.name in JOINTS else ""))
            for key in ("glow", "k", "phase", "show"):
                if key in ob:
                    col.label(text="  %s = %s" % (key, ob[key] if not hasattr(ob[key], "to_list") else ob[key].to_list()))


CLASSES = (OVERDRIVE_OT_open, OVERDRIVE_OT_save, OVERDRIVE_OT_add_box, OVERDRIVE_OT_add_joint,
           OVERDRIVE_OT_glow, OVERDRIVE_OT_show, OVERDRIVE_PT_panel)


def register():
    T = bpy.types.Scene
    T.overdrive_repo = bpy.props.StringProperty(name="Repo", subtype="DIR_PATH",
                                                description="The 3d_shooter folder (the one with assets/ in it)")
    T.overdrive_gun = bpy.props.EnumProperty(name="Gun", items=[(g, g, "") for g in GUNS])
    T.overdrive_material = bpy.props.EnumProperty(name="Material", items=[(m, m, "") for m in PALETTE])
    T.overdrive_joint = bpy.props.EnumProperty(name="Joint", items=[(j, j, "") for j in JOINTS])
    T.overdrive_channel = bpy.props.EnumProperty(name="Channel", items=[(c, c, "") for c in CHANNELS])
    T.overdrive_k = bpy.props.FloatProperty(name="Strength (k)", default=1.0, min=0.0, soft_max=4.0)
    T.overdrive_flag = bpy.props.EnumProperty(name="Flag", items=[(f, f, "") for f in FLAGS])
    for c in CLASSES:
        bpy.utils.register_class(c)


def unregister():
    for c in reversed(CLASSES):
        bpy.utils.unregister_class(c)
    for p in ("overdrive_repo", "overdrive_gun", "overdrive_material", "overdrive_joint",
              "overdrive_channel", "overdrive_k", "overdrive_flag"):
        delattr(bpy.types.Scene, p)


if __name__ == "__main__":
    register()
