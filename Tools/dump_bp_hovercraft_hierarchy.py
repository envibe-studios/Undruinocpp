"""Dump BP_Hovercraft SCS hierarchy (read-only)."""
import json
import traceback
import unreal

BP_PATH = "/Game/Blueprints/BP_Hovercraft"
OUT = "D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_before.json"
LOG = "D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_dump_log.txt"

BFL = unreal.SubobjectDataBlueprintFunctionLibrary
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)

lines = []

def log(m):
    unreal.log(m)
    lines.append(m)

try:
    bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
    handles = list(SDS.k2_gather_subobject_data_for_blueprint(bp))
    log(f"handles={len(handles)}")

    # Probe available BFL methods once
    probe = dir(BFL)
    interesting = [n for n in probe if any(k in n.lower() for k in ("parent", "root", "name", "scene", "object", "data", "valid"))]
    log("BFL methods: " + ", ".join(interesting))

    rows = []
    for h in handles:
        data = BFL.get_data(h)
        name = None
        cls = None
        is_root = False
        is_scene = False
        parent_name = None
        rel = None
        try:
            name = str(BFL.get_variable_name(data))
        except Exception as e:
            name = f"<varname err {e}>"
        try:
            obj = BFL.get_object(data)
            cls = obj.get_class().get_name() if obj else None
            if isinstance(obj, unreal.SceneComponent):
                is_scene = True
                loc = obj.get_relative_location()
                rot = obj.get_relative_rotation()
                scale = obj.get_relative_scale3d()
                rel = {
                    "location": [loc.x, loc.y, loc.z],
                    "rotation": [rot.roll, rot.pitch, rot.yaw],
                    "scale": [scale.x, scale.y, scale.z],
                }
                attach = obj.get_attach_parent()
                if attach:
                    parent_name = attach.get_name()
                    # strip _GEN_VARIABLE style suffixes for readability
        except Exception as e:
            cls = f"<obj err {e}>"
        for fn in ("is_root_component", "is_scene_root", "is_root_actor"):
            if hasattr(BFL, fn):
                try:
                    if bool(getattr(BFL, fn)(data)):
                        is_root = True
                except Exception:
                    pass
        # parent via BFL
        if hasattr(BFL, "get_parent_handle"):
            try:
                ph = BFL.get_parent_handle(data)
                if ph is not None:
                    pdata = BFL.get_data(ph)
                    parent_name = str(BFL.get_variable_name(pdata))
            except Exception:
                pass

        rows.append({
            "name": name,
            "class": cls,
            "is_scene": is_scene,
            "is_scene_root": is_root,
            "parent": parent_name,
            "relative": rel,
        })
        log(f"{name} | {cls} | root={is_root} | parent={parent_name} | rel={rel}")

    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rows, f, indent=2)
    log(f"wrote {OUT}")
except Exception:
    log(traceback.format_exc())

with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
