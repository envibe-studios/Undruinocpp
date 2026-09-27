"""
BP_Hovercraft component hierarchy fix.

Problem: Body (scaled StaticMesh) must not be the scene root / parent of other
components — children inherit Body's scale.

Fix (UE best practice):
  Root (SceneComponent, scale 1,1,1)
    ├── Body (keeps its mesh scale)
    ├── Thrusters / weapons / collision / captures / arrows / ...
    └── (logic components remain non-scene)

Writes:
  D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_before.json
  D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_after.json
  D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_fix_log.txt
"""

from __future__ import annotations

import json
import traceback
from typing import Any, Dict, List, Optional, Tuple

import unreal

BP_PATH = "/Game/Blueprints/BP_Hovercraft"
OUT_BEFORE = "D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_before.json"
OUT_AFTER = "D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_after.json"
OUT_LOG = "D:/Unreals/Undruinocpp/.cursor/_hover_hierarchy_fix_log.txt"

ROOT_NAME = "Root"
BODY_NAME = "Body"

# Components that should stay parented under Body (none — avoid inherited scale).
KEEP_UNDER_BODY = set()

BFL = unreal.SubobjectDataBlueprintFunctionLibrary
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def _log(lines: List[str], msg: str) -> None:
    unreal.log(msg)
    lines.append(msg)


def _safe_name(handle) -> str:
    data = BFL.get_data(handle)
    try:
        return str(BFL.get_variable_name(data))
    except Exception:
        try:
            obj = BFL.get_object(data)
            return obj.get_name() if obj else "<unknown>"
        except Exception:
            return "<unknown>"


def _safe_class(handle) -> str:
    data = BFL.get_data(handle)
    try:
        obj = BFL.get_object(data)
        return obj.get_class().get_name() if obj else "<none>"
    except Exception:
        return "<none>"


def _is_scene_root(handle) -> bool:
    data = BFL.get_data(handle)
    try:
        return bool(BFL.is_root_component(data))
    except Exception:
        try:
            return bool(BFL.is_scene_root(data))
        except Exception:
            return False


def _is_scene_component(handle) -> bool:
    data = BFL.get_data(handle)
    try:
        return bool(BFL.is_scene_component(data))
    except Exception:
        obj = BFL.get_object(data)
        return isinstance(obj, unreal.SceneComponent)


def _parent_handle(handle, all_handles):
    data = BFL.get_data(handle)
    try:
        parent = BFL.get_parent_handle(data)
        if parent and BFL.is_valid(parent):
            return parent
    except Exception:
        pass
    # Fallback: walk attach via object attach parent name match
    try:
        obj = BFL.get_object(data)
        if isinstance(obj, unreal.SceneComponent):
            attach = obj.get_attach_parent()
            if attach:
                for h in all_handles:
                    if BFL.get_object(BFL.get_data(h)) == attach:
                        return h
    except Exception:
        pass
    return None


def _rel_transform(handle) -> Optional[Dict[str, Any]]:
    data = BFL.get_data(handle)
    obj = BFL.get_object(data)
    if not isinstance(obj, unreal.SceneComponent):
        return None
    loc = obj.get_relative_location()
    rot = obj.get_relative_rotation()
    scale = obj.get_relative_scale3d()
    return {
        "location": [loc.x, loc.y, loc.z],
        "rotation": [rot.roll, rot.pitch, rot.yaw],
        "scale": [scale.x, scale.y, scale.z],
    }


def dump_hierarchy(bp) -> List[Dict[str, Any]]:
    handles = list(SDS.k2_gather_subobject_data_for_blueprint(bp))
    rows: List[Dict[str, Any]] = []
    for h in handles:
        parent = _parent_handle(h, handles)
        rows.append(
            {
                "name": _safe_name(h),
                "class": _safe_class(h),
                "is_scene": _is_scene_component(h),
                "is_scene_root": _is_scene_root(h),
                "parent": _safe_name(parent) if parent else None,
                "relative": _rel_transform(h),
            }
        )
    return rows


def find_handle_by_name(handles, name: str):
    for h in handles:
        if _safe_name(h) == name:
            return h
    return None


def find_scene_root_handle(handles):
    for h in handles:
        if _is_scene_root(h):
            return h
    # Fallback: first scene component with no parent
    for h in handles:
        if _is_scene_component(h) and _parent_handle(h, handles) is None:
            # Skip the blueprint actor context handle (usually index 0, not a component)
            if _safe_class(h) not in ("BP_Hovercraft_C", "Blueprint", "BlueprintGeneratedClass"):
                # Actor context often has the BP name; components have component classes
                cls = _safe_class(h)
                if "Component" in cls or cls in ("SceneComponent", "DefaultSceneRoot"):
                    return h
    return None


def ensure_unit_scale_root(bp, log: List[str]):
    handles = list(SDS.k2_gather_subobject_data_for_blueprint(bp))
    context = handles[0]
    root = find_scene_root_handle(handles)
    root_name = _safe_name(root) if root else None
    _log(log, f"Current scene root: {root_name} ({_safe_class(root) if root else 'None'})")

    # If root is already an unscaled pure SceneComponent named Root/DefaultSceneRoot, keep it.
    if root and _safe_class(root) == "SceneComponent":
        rel = _rel_transform(root)
        scale = rel["scale"] if rel else [1, 1, 1]
        if abs(scale[0] - 1) < 1e-4 and abs(scale[1] - 1) < 1e-4 and abs(scale[2] - 1) < 1e-4:
            # Rename DefaultSceneRoot -> Root for clarity if needed
            if _safe_name(root) in ("DefaultSceneRoot", ROOT_NAME):
                if _safe_name(root) != ROOT_NAME:
                    SDS.rename_subobject(root, unreal.Text(ROOT_NAME))
                    _log(log, "Renamed DefaultSceneRoot -> Root")
                return root

    # Prefer existing Root / DefaultSceneRoot scene component if present but not root
    candidate = find_handle_by_name(handles, ROOT_NAME) or find_handle_by_name(handles, "DefaultSceneRoot")
    if candidate and _is_scene_component(candidate) and _safe_class(candidate) == "SceneComponent":
        ok = SDS.make_new_scene_root(context, candidate, bp)
        _log(log, f"make_new_scene_root({_safe_name(candidate)}) -> {ok}")
        # Ensure unit scale on root template
        obj = BFL.get_object(BFL.get_data(candidate))
        if isinstance(obj, unreal.SceneComponent):
            obj.set_relative_scale3d(unreal.Vector(1, 1, 1))
            obj.set_relative_location(unreal.Vector(0, 0, 0))
            obj.set_relative_rotation(unreal.Rotator(0, 0, 0))
        if _safe_name(candidate) != ROOT_NAME:
            SDS.rename_subobject(candidate, unreal.Text(ROOT_NAME))
        return candidate

    # Create a new SceneComponent root
    params = unreal.AddNewSubobjectParams(
        parent_handle=context,
        new_class=unreal.SceneComponent,
        blueprint_context=bp,
    )
    new_handle, fail = SDS.add_new_subobject(params)
    if fail and str(fail):
        raise RuntimeError(f"Failed to add Root SceneComponent: {fail}")
    SDS.rename_subobject(new_handle, unreal.Text(ROOT_NAME))
    ok = SDS.make_new_scene_root(context, new_handle, bp)
    _log(log, f"Created Root and make_new_scene_root -> {ok}")
    obj = BFL.get_object(BFL.get_data(new_handle))
    if isinstance(obj, unreal.SceneComponent):
        obj.set_relative_scale3d(unreal.Vector(1, 1, 1))
        obj.set_relative_location(unreal.Vector(0, 0, 0))
        obj.set_relative_rotation(unreal.Rotator(0, 0, 0))
    return new_handle


def _compose_rel(parent_rel: Dict[str, Any], child_rel: Dict[str, Any]) -> Tuple[unreal.Transform, Dict[str, Any]]:
    """Compose parent*child relative transforms into a transform relative to grandparent."""
    p = unreal.Transform(
        unreal.Vector(*parent_rel["location"]),
        unreal.Rotator(parent_rel["rotation"][1], parent_rel["rotation"][2], parent_rel["rotation"][0]),  # pitch,yaw,roll ctor? 
        unreal.Vector(*parent_rel["scale"]),
    )
    # Rotator constructor in Python is (pitch, yaw, roll) — we stored [roll, pitch, yaw]
    p = unreal.Transform(
        unreal.Vector(*parent_rel["location"]),
        unreal.Rotator(parent_rel["rotation"][1], parent_rel["rotation"][2], parent_rel["rotation"][0]),
        unreal.Vector(*parent_rel["scale"]),
    )
    c = unreal.Transform(
        unreal.Vector(*child_rel["location"]),
        unreal.Rotator(child_rel["rotation"][1], child_rel["rotation"][2], child_rel["rotation"][0]),
        unreal.Vector(*child_rel["scale"]),
    )
    # Actually UE Python Rotator is pitch, yaw, roll
    p = unreal.Transform(
        unreal.Vector(parent_rel["location"][0], parent_rel["location"][1], parent_rel["location"][2]),
        unreal.Rotator(parent_rel["rotation"][1], parent_rel["rotation"][2], parent_rel["rotation"][0]),
        unreal.Vector(parent_rel["scale"][0], parent_rel["scale"][1], parent_rel["scale"][2]),
    )
    c = unreal.Transform(
        unreal.Vector(child_rel["location"][0], child_rel["location"][1], child_rel["location"][2]),
        unreal.Rotator(child_rel["rotation"][1], child_rel["rotation"][2], child_rel["rotation"][0]),
        unreal.Vector(child_rel["scale"][0], child_rel["scale"][1], child_rel["scale"][2]),
    )
    composed = p * c
    loc = composed.translation
    rot = composed.rotation.rotator()
    scale = composed.scale3d
    as_dict = {
        "location": [loc.x, loc.y, loc.z],
        "rotation": [rot.roll, rot.pitch, rot.yaw],
        "scale": [scale.x, scale.y, scale.z],
    }
    return composed, as_dict


def apply_rel(handle, rel: Dict[str, Any]) -> None:
    obj = BFL.get_object(BFL.get_data(handle))
    if not isinstance(obj, unreal.SceneComponent):
        return
    obj.set_relative_location(unreal.Vector(rel["location"][0], rel["location"][1], rel["location"][2]))
    obj.set_relative_rotation(unreal.Rotator(rel["rotation"][1], rel["rotation"][2], rel["rotation"][0]))
    obj.set_relative_scale3d(unreal.Vector(rel["scale"][0], rel["scale"][1], rel["scale"][2]))


def collect_descendants(body_name: str, rows: List[Dict[str, Any]]) -> List[str]:
    """All scene component names under Body (not including Body)."""
    children_map: Dict[Optional[str], List[str]] = {}
    for r in rows:
        children_map.setdefault(r["parent"], []).append(r["name"])
    out: List[str] = []

    def walk(n: str):
        for c in children_map.get(n, []):
            out.append(c)
            walk(c)

    walk(body_name)
    return out


def reparent_off_body(bp, root_handle, log: List[str]) -> None:
    handles = list(SDS.k2_gather_subobject_data_for_blueprint(bp))
    body = find_handle_by_name(handles, BODY_NAME)
    if not body:
        _log(log, "Body not found — nothing to reparent off Body")
        return

    rows = dump_hierarchy(bp)
    body_row = next((r for r in rows if r["name"] == BODY_NAME), None)
    if not body_row or not body_row["relative"]:
        _log(log, "Body has no relative transform info")
        return

    # Only direct children of Body need transform bake when moving to Root;
    # deeper descendants keep relative-to-parent if we move whole subtrees by
    # only reparenting direct children.
    direct = [r for r in rows if r["parent"] == BODY_NAME and r["name"] not in KEEP_UNDER_BODY and r["is_scene"]]
    _log(log, f"Direct Body children to move under Root: {[r['name'] for r in direct]}")

    body_rel = body_row["relative"]
    for row in direct:
        child = find_handle_by_name(handles, row["name"])
        if not child:
            continue
        child_rel = row["relative"]
        if not child_rel:
            _log(log, f"  skip {row['name']} (no transform)")
            continue
        _, new_rel = _compose_rel(body_rel, child_rel)
        apply_rel(child, new_rel)
        params = unreal.ReparentSubobjectParams(
            new_parent_handle=root_handle,
            blueprint_context=bp,
        )
        ok = SDS.reparent_subobject(params, child)
        if not ok:
            # Fallback attach API
            ok = SDS.attach_subobject(root_handle, child)
        _log(log, f"  reparent {row['name']} -> Root ({ok}) new_scale={new_rel['scale']}")


def compile_and_save(bp, log: List[str]) -> None:
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        _log(log, "Compiled blueprint")
    except Exception as e:
        _log(log, f"Compile via BlueprintEditorLibrary failed: {e}")
        try:
            unreal.KismetEditorUtilities.compile_blueprint(bp)
            _log(log, "Compiled via KismetEditorUtilities")
        except Exception as e2:
            _log(log, f"Compile failed: {e2}")
    try:
        unreal.EditorAssetLibrary.save_loaded_asset(bp)
        _log(log, "Saved asset")
    except Exception as e:
        _log(log, f"Save failed: {e}")


def main() -> None:
    log: List[str] = []
    try:
        bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
        if not bp:
            raise RuntimeError(f"Could not load {BP_PATH}")

        before = dump_hierarchy(bp)
        with open(OUT_BEFORE, "w", encoding="utf-8") as f:
            json.dump(before, f, indent=2)
        _log(log, f"Wrote before dump ({len(before)} entries)")

        root_handle = ensure_unit_scale_root(bp, log)
        # Refresh handle after possible structural change
        handles = list(SDS.k2_gather_subobject_data_for_blueprint(bp))
        root_handle = find_handle_by_name(handles, ROOT_NAME) or find_scene_root_handle(handles)
        _log(log, f"Using root: {_safe_name(root_handle)}")

        reparent_off_body(bp, root_handle, log)

        # Ensure Body is under Root (make_new_scene_root usually does this)
        handles = list(SDS.k2_gather_subobject_data_for_blueprint(bp))
        body = find_handle_by_name(handles, BODY_NAME)
        root_handle = find_handle_by_name(handles, ROOT_NAME) or find_scene_root_handle(handles)
        if body and root_handle:
            parent = _parent_handle(body, handles)
            if not parent or _safe_name(parent) != _safe_name(root_handle):
                params = unreal.ReparentSubobjectParams(
                    new_parent_handle=root_handle,
                    blueprint_context=bp,
                )
                ok = SDS.reparent_subobject(params, body) or SDS.attach_subobject(root_handle, body)
                _log(log, f"Ensure Body under Root -> {ok}")

        compile_and_save(bp, log)

        after = dump_hierarchy(bp)
        with open(OUT_AFTER, "w", encoding="utf-8") as f:
            json.dump(after, f, indent=2)
        _log(log, f"Wrote after dump ({len(after)} entries)")

        # Sanity checks
        root_row = next((r for r in after if r["is_scene_root"]), None)
        body_row = next((r for r in after if r["name"] == BODY_NAME), None)
        scaled_children = [
            r["name"]
            for r in after
            if r["parent"] == BODY_NAME and r["is_scene"]
        ]
        _log(log, f"AFTER root={root_row}")
        _log(log, f"AFTER body parent={body_row['parent'] if body_row else None} scale={body_row['relative']['scale'] if body_row and body_row['relative'] else None}")
        _log(log, f"AFTER still under Body: {scaled_children}")

    except Exception:
        _log(log, traceback.format_exc())

    with open(OUT_LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(log))


if __name__ == "__main__":
    main()
