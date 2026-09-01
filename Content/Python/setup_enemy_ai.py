"""
Setup Enemy AI content assets for Unduinocpp.
Run in Unreal Editor Output Log:
  py "D:/Unreals/Undruinocpp/Content/Python/setup_enemy_ai.py"
Or: File -> Execute Python Script
"""

import unreal

FOLDER = "/Game/Blueprints/EnemyAI"
BB_PATH = f"{FOLDER}/BB_Enemy"
BT_PATH = f"{FOLDER}/BT_Enemy_Basic"
BT_COMBAT_PATH = f"{FOLDER}/BT_Enemy_Combat"
DEF_PATH = f"{FOLDER}/DA_EnemyDefinition_Flyer"
LOADOUT_PATH = f"{FOLDER}/DA_EnemyAbilityLoadout_Basic"
SHOT_PATH = f"{FOLDER}/DA_EnemyAbility_BasicShot"
SHIELD_PATH = f"{FOLDER}/DA_EnemyAbility_SelfShield"
PAWN_BP_PATH = f"{FOLDER}/BP_EnemyFlyer"
AIC_BP_PATH = f"{FOLDER}/BP_EnemyAIController"


def _log(msg):
    unreal.log(f"[setup_enemy_ai] {msg}")


def _save(asset):
    if not asset:
        return
    unreal.EditorAssetLibrary.save_loaded_asset(asset)


def ensure_data_assets():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    def create_da(name, class_path):
        path = f"{FOLDER}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.EditorAssetLibrary.load_asset(path)
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("DataAssetClass", unreal.load_class(None, class_path))
        asset = asset_tools.create_asset(name, FOLDER, unreal.DataAsset, factory)
        _log(f"Created {path}")
        return asset

    shot = create_da("DA_EnemyAbility_BasicShot", "/Script/Unduinocpp.EnemyAbility")
    shield = create_da("DA_EnemyAbility_SelfShield", "/Script/Unduinocpp.EnemyAbility")
    loadout = create_da("DA_EnemyAbilityLoadout_Basic", "/Script/Unduinocpp.EnemyAbilityLoadout")
    definition = create_da("DA_EnemyDefinition_Flyer", "/Script/Unduinocpp.EnemyDefinition")

    if shot:
        shot.set_editor_property("AbilityId", "BasicShot")
        shot.set_editor_property("DisplayName", unreal.Text("Basic Shot"))
        shot.set_editor_property("CooldownSeconds", 1.5)
        shot.set_editor_property("MaxRange", 4000.0)
        shot.set_editor_property("EffectType", unreal.EEnemyAbilityEffectType.DAMAGE)
        shot.set_editor_property("TargetRule", unreal.EEnemyAbilityTargetRule.CURRENT_TARGET)
        shot.set_editor_property("Magnitude", 15.0)
        _save(shot)

    if shield:
        shield.set_editor_property("AbilityId", "SelfShield")
        shield.set_editor_property("DisplayName", unreal.Text("Self Shield"))
        shield.set_editor_property("CooldownSeconds", 8.0)
        shield.set_editor_property("EffectType", unreal.EEnemyAbilityEffectType.SHIELD)
        shield.set_editor_property("TargetRule", unreal.EEnemyAbilityTargetRule.SELF)
        shield.set_editor_property("Magnitude", 50.0)
        _save(shield)

    if loadout and shot and shield:
        loadout.set_editor_property("Abilities", [shot, shield])
        _save(loadout)

    return shot, shield, loadout, definition


def ensure_blueprints():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    def create_bp(name, parent_class_path):
        path = f"{FOLDER}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.EditorAssetLibrary.load_asset(path)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("ParentClass", unreal.load_class(None, parent_class_path))
        bp = asset_tools.create_asset(name, FOLDER, unreal.Blueprint, factory)
        _log(f"Created {path}")
        return bp

    pawn_bp = create_bp("BP_EnemyFlyer", "/Script/Unduinocpp.EnemyPawn")
    aic_bp = create_bp("BP_EnemyAIController", "/Script/Unduinocpp.EnemyAIController")
    _save(pawn_bp)
    _save(aic_bp)
    return pawn_bp, aic_bp


def fix_blackboard():
    bb = unreal.EditorAssetLibrary.load_asset(BB_PATH)
    if not bb:
        _log("BB_Enemy missing — create it first.")
        return None

    keys = bb.get_editor_property("Keys")
    changed = False
    new_keys = []
    for entry in keys:
        name = str(entry.get_editor_property("EntryName"))
        if name == "SquadID":
            entry.set_editor_property("EntryName", "SquadId")
            changed = True
            _log("Renamed SquadID -> SquadId")
        new_keys.append(entry)
    if changed:
        bb.set_editor_property("Keys", new_keys)
        _save(bb)
    return bb


def configure_definition(definition, loadout, pawn_bp, aic_bp, bb):
    if not definition:
        return

    bt = unreal.EditorAssetLibrary.load_asset(BT_PATH)
    fly_mode = unreal.load_class(None, "/Script/Unduinocpp.FlyingMovementMode")
    pawn_class = unreal.load_class(None, f"{PAWN_BP_PATH}_C")
    if not pawn_class and pawn_bp:
        # Generated class after compile
        pawn_class = pawn_bp.generated_class()

    definition.set_editor_property("EnemyId", "FlyerBasic")
    definition.set_editor_property("DisplayName", unreal.Text("Basic Flyer"))
    definition.set_editor_property("FactionId", "Enemy")
    definition.set_editor_property("TargetPolicy", unreal.EEnemyTargetPolicy.NEAREST)
    definition.set_editor_property("MaxHitpoints", 100.0)
    definition.set_editor_property("SquadRole", unreal.EEnemySquadRole.NONE)
    definition.set_editor_property("ActorTags", ["Targetable", "Enemy"])

    if pawn_class:
        definition.set_editor_property("PawnClass", pawn_class)
    if bt:
        definition.set_editor_property("BehaviorTree", bt)
    if bb:
        definition.set_editor_property("BlackboardAsset", bb)
    if fly_mode:
        definition.set_editor_property("MovementModeClass", fly_mode)
    if loadout:
        definition.set_editor_property("AbilityLoadout", loadout)

    params = unreal.EnemyMovementParams()
    params.max_speed = 900.0
    params.acceleration = 2200.0
    params.turn_rate_deg_per_sec = 200.0
    params.acceptance_radius = 200.0
    params.preferred_altitude = 400.0
    definition.set_editor_property("MovementParams", params)

    perception = unreal.EnemyPerceptionParams()
    perception.sight_radius = 5000.0
    perception.lose_sight_radius = 6000.0
    perception.peripheral_vision_angle_deg = 90.0
    perception.b_detect_enemies = True
    perception.b_detect_neutrals = True
    perception.b_detect_friendlies = False
    definition.set_editor_property("PerceptionParams", perception)

    _save(definition)
    _log("Configured DA_EnemyDefinition_Flyer")


def configure_pawn_and_controller(pawn_bp, aic_bp, definition, bb):
    bt = unreal.EditorAssetLibrary.load_asset(BT_PATH)

    if aic_bp:
        cdo = unreal.get_default_object(aic_bp.generated_class())
        if bt:
            cdo.set_editor_property("DefaultBehaviorTree", bt)
        if bb:
            cdo.set_editor_property("DefaultBlackboard", bb)
        cdo.set_editor_property("bDrawDebug", True)
        _save(aic_bp)
        _log("Configured BP_EnemyAIController defaults")

    if pawn_bp:
        cdo = unreal.get_default_object(pawn_bp.generated_class())
        if definition:
            cdo.set_editor_property("EnemyDefinition", definition)
        cdo.set_editor_property("bDrawDebug", True)
        if aic_bp:
            cdo.set_editor_property("AIControllerClass", aic_bp.generated_class())
        cdo.set_editor_property("AutoPossessAI", unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
        _save(pawn_bp)
        _log("Configured BP_EnemyFlyer defaults")


def rebuild_behavior_tree():
    """
    Rebuild BT_Enemy_Combat with C++ tasks.
    Uses Behavior Tree editor utilities where available.
    """
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    bb = unreal.EditorAssetLibrary.load_asset(BB_PATH)

    # Prefer a clean combat tree asset
    if unreal.EditorAssetLibrary.does_asset_exist(BT_COMBAT_PATH):
        unreal.EditorAssetLibrary.delete_asset(BT_COMBAT_PATH)

    factory = unreal.BehaviorTreeFactory()
    bt = asset_tools.create_asset("BT_Enemy_Combat", FOLDER, unreal.BehaviorTree, factory)
    if not bt:
        _log("Failed to create BT_Enemy_Combat")
        return None

    if bb:
        bt.set_editor_property("BlackboardAsset", bb)

    # UE Python cannot easily author full BT graphs in all versions.
    # We wire Blackboard + leave a clear note; graph nodes still need editor.
    # Attempt: use BT editor subsystem if present.
    try:
        # Some projects expose helper libraries; keep graceful.
        _log("BT_Enemy_Combat created and linked to BB_Enemy.")
        _log("Open BT_Enemy_Combat and add: Root Service UpdateEnemyTarget; Selector -> Combat Sequence / Idle.")
        _log("Combat Sequence: Decorator TargetActor Is Set; tasks EnemyMoveTo, EnemyFaceTarget, EnemyUseAbility(BasicShot).")
    except Exception as ex:
        _log(f"BT graph authoring skipped: {ex}")

    _save(bt)

    # Also ensure existing BT_Enemy_Basic has blackboard set
    basic = unreal.EditorAssetLibrary.load_asset(BT_PATH)
    if basic and bb:
        basic.set_editor_property("BlackboardAsset", bb)
        _save(basic)

    return bt


def spawn_test_enemy():
    pawn_class = unreal.load_class(None, f"{PAWN_BP_PATH}_C")
    if not pawn_class:
        bp = unreal.EditorAssetLibrary.load_asset(PAWN_BP_PATH)
        if bp:
            pawn_class = bp.generated_class()
    if not pawn_class:
        _log("BP_EnemyFlyer class not ready — compile Blueprints then re-run.")
        return

    location = unreal.Vector(500.0, 0.0, 200.0)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(pawn_class, location)
    if actor:
        actor.set_actor_label("EnemyFlyer_Test")
        _log(f"Spawned test enemy: {actor.get_name()}")
    else:
        _log("Failed to spawn test enemy")


def main():
    unreal.EditorAssetLibrary.make_directory(FOLDER)
    shot, shield, loadout, definition = ensure_data_assets()
    pawn_bp, aic_bp = ensure_blueprints()
    bb = fix_blackboard()
    configure_definition(definition, loadout, pawn_bp, aic_bp, bb)
    configure_pawn_and_controller(pawn_bp, aic_bp, definition, bb)
    bt_combat = rebuild_behavior_tree()

    # Point definition at combat tree if created
    if definition and bt_combat:
        definition.set_editor_property("BehaviorTree", bt_combat)
        _save(definition)

    # Compile blueprints
    for bp in (pawn_bp, aic_bp):
        if bp:
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
            _save(bp)

    spawn_test_enemy()
    _log("Done. Open BT_Enemy_Combat and finish the node graph if empty.")


if __name__ == "__main__":
    main()
