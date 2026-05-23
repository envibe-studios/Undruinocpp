# Mission System Guide

This guide explains how to create missions with the new data-driven mission system in `Unduinocpp`.

The system is built around:

- `UMissionDataAsset` for individual missions
- `UMissionRegistryAsset` for a session's mission list
- `UMissionManagerSubsystem` for runtime mission logic
- `AMissionGameState` for replicated mission and threat state
- `AMissionGameModeBase` as an optional GameMode that wires the system up for you

## Quick Start

1. Compile the project.
2. In the Content Browser, create a `Primary Data Asset`.
3. Choose `MissionDataAsset` to make a mission.
4. Create another `Primary Data Asset`.
5. Choose `MissionRegistryAsset` to make a registry.
6. Add all mission assets to the registry and set `MainMissionID`.
7. Use `MissionGameModeBase` as your GameMode, or call `RegisterMissionsFromRegistry()` manually from your own GameMode.

## How the System Works

Each session can contain:

- 1 Main Mission
- 0 or more Sub missions
- 0 or more Side missions

Each mission has a runtime state:

- `Hidden`
- `Available`
- `Active`
- `Completed_Success`
- `Completed_Failure`
- `Expired`

General flow:

- The main mission starts `Active`
- Other missions usually start `Hidden`
- Visibility conditions can move a mission to `Available`
- Mission actions can move another mission to `Available` or `Active`
- Success and failure can both change threat and unlock more content

This is a fail-forward system. Mission failure should usually escalate the session instead of ending it.

## Asset Types

### `MissionDataAsset`

Create one of these for each mission.

Important fields:

- `MissionID`
- `MissionType`
- `ParentMissionID`
- `FactionID`
- `Objectives`
- `VisibilityConditions`
- `TimeLimitSeconds`
- `OnSuccessActions`
- `OnFailureActions`
- `ThreatDeltaOnSuccess`
- `ThreatDeltaOnFailure`
- `bRandomizeTargetCount`
- `MinTargetCount`
- `MaxTargetCount`

### `MissionRegistryAsset`

This asset defines the session mission set.

Important fields:

- `Missions` - every mission used in the session
- `MainMissionID` - the mission that starts active
- `ThreatThresholds` - thresholds for world reactions like AI escalation or audio hooks

## Setting Up the Session

### Option A: Use `MissionGameModeBase`

This is the simplest setup.

1. Create a Blueprint based on `MissionGameModeBase`.
2. Set its `MissionRegistry`.
3. Assign that GameMode to the level or project.

What it does:

- Uses `AMissionGameState`
- Registers the registry on `BeginPlay`
- Ticks the mission manager every frame

### Option B: Use your own GameMode

If you already have a GameMode Blueprint or C++ class, keep it and wire the system manually.

Required setup:

1. Set Game State Class to `MissionGameState` or a Blueprint subclass of it.
2. On `BeginPlay`, get `MissionManagerSubsystem`.
3. Call `RegisterMissionsFromRegistry(YourRegistry)`.
4. Set **Default Player State Class** to `MissionPlayerState` (or use `MissionGameModeBase`, which sets this for you). Required for per-player objectives.

If you skip this setup, missions will not initialize.

## Creating a Mission

Create a `Primary Data Asset` of type `MissionDataAsset`.

### Step 1: Give it an ID

Every mission needs a unique `MissionID`.

Example:

- `Main_SecureRelay`
- `Sub_ScanAnomalies`
- `Side_EmergencyRepair`

Keep IDs short and stable. Other assets reference them by name.

### Step 2: Choose a mission type

Set `MissionType` to one of:

- `Main`
- `Sub`
- `Side`

Use:

- `Main` for the primary session objective
- `Sub` for children of the main mission
- `Side` for optional or hidden extra content

### Step 3: Set parent mission if needed

If the mission is a sub-mission, set `ParentMissionID`.

Example:

- Main mission: `MissionID = Main_SecureRelay`
- Sub mission: `MissionID = Sub_RepairScanner`, `ParentMissionID = Main_SecureRelay`

The current runtime logic does not require the parent for execution, but designers should still fill it in for organization and future branching logic.

### Step 4: Add objectives

Each mission needs at least one objective in `Objectives`.

Each objective supports:

- `ObjectiveType`
- `TargetActorClass`
- `TargetCount`
- **`ObjectiveScope`** — `Session (shared)` or `Per player` (see below)
- `RequiredRole`
- `TimeLimitSeconds`
- `ZoneReference`
- `StateThresholdPercent`
- `DisplayName`

## Objective Types

Supported objective types:

- `Exploration`
- `EnterCollider`
- `RemainInZoneForDuration`
- `DestroyTarget`
- `DestroyClassCount`
- `TimedKill`
- `CollectResourceCount`
- `DeliverResourceToZone`
- `RepairSystem`
- `ReloadAmmo`
- `RestoreOxygenAboveThreshold`
- `MaintainSystemAboveThresholdForDuration`
- `SurviveDuration`
- `DefendTarget`
- `MaintainOxygenAboveThreshold`
- `MaintainHullIntegrity`
- `MaintainThrusterHealth`

Important note:

The mission system stores these objective definitions and validates role restrictions, but your gameplay code or Blueprints still need to report the actual progress events at runtime.

Example:

- When a player enters a mission zone, call `ReportObjectiveProgress()`
- When a repair completes, call `ReportObjectiveProgress()`
- When an enemy generator is destroyed, call `ReportObjectiveProgress()`

## Per-player objectives (races, checkpoints)

By default, objective progress is **session-wide** (one shared bar on `MissionGameState`). The first overlap increments once for everyone.

For **each racer / each player** to have their own progress (waypoints, laps, personal checkpoints):

1. On the **objective** in the mission asset, set **`ObjectiveScope`** to **`Per player`**.
2. Set **Default Player State Class** on your GameMode to **`MissionPlayerState`** (already done if you use `MissionGameModeBase`).
3. From your waypoint overlap (on the **server**), call:
   - **`Report Objective Progress For Pawn`** → pass the overlapping **Pawn**, `MissionID`, `ObjectiveIndex`, `DeltaCount`, `ReporterRole`
   - or **`Report Objective Progress For Player`** with that pawn’s **Player State**

Per-player progress is stored on **`MissionPlayerState.PerPlayerObjectiveProgress`** and replicates to that player’s client for UI.

**Do not** call the regular `Report Objective Progress` for Per-player objectives — it will no-op. That keeps session co-op objectives unchanged.

### Optional: mission completes when everyone finishes

On the **mission asset**, enable:

- **`bCompleteWhenAllPlayersFinishPerPlayerObjectives`**

Then the mission completes successfully only when:

- all **Session-scoped** objectives are complete (if any), **and**
- every **connected player** has completed all **Per-player** objectives for that mission.

Use this for “whole lobby must finish the lap” style goals. For “first to finish wins”, leave this off and handle win logic separately (e.g. custom Blueprint when a player completes their last per-player objective).

## Role-Specific Objectives

Use `RequiredRole` to restrict who can progress the objective:

- `Any`
- `Pilot`
- `Gunner`
- `Engineer`

Example use cases:

- Pilot enters a canyon checkpoint
- Gunner scans three anomalies
- Engineer repairs a damaged scanner

If `RequiredRole` is set to `Engineer`, progress reported by `Pilot` or `Gunner` will be ignored.

## Time Limits

There are two levels of time limit support.

### Mission-level

Use `TimeLimitSeconds` on the mission asset.

If this expires while the mission is active:

- the mission becomes `Expired`
- failure-style actions run
- threat can increase
- fail-forward side content can unlock

### Objective-level

Use `TimeLimitSeconds` on an individual objective.

If the objective timer expires:

- the mission fails
- failure actions run

## Visibility Conditions

Use `VisibilityConditions` to reveal missions dynamically.

Each condition can check:

- `RequiredMissionID`
- `RequiredMissionState`
- `ThreatLevelMin`
- `ThreatLevelMax`
- `TimeElapsedSeconds`

Current behavior:

- all conditions on a mission are treated as AND
- if they all pass, the mission can move from `Hidden` to `Available`

Example:

- Mission stays hidden until `Sub_RepairScanner` is `Completed_Success`
- Mission only appears if global threat is at least `20`
- Mission only appears after `180` seconds

## Success and Failure Actions

Every mission should define what happens on success and on failure.

Each action supports:

- `TargetMissionID`
- `bActivateMission`
- `ThreatDelta`
- `FactionThreatDelta`
- `WorldEventName`

Use this for branching and fail-forward design.

### Example success action

- `TargetMissionID = Side_BonusCache`
- `bActivateMission = false`
- `ThreatDelta = -5`
- `WorldEventName = AudioEvent_MissionSuccess`

Result:

- bonus mission becomes available
- threat decreases
- world systems can respond to the audio event hook

### Example failure action

- `TargetMissionID = Side_EmergencyRepair`
- `bActivateMission = true`
- `ThreatDelta = 10`
- `WorldEventName = AudioEvent_MissionFailure`

Result:

- emergency repair mission activates immediately
- threat increases
- world systems can react

## Threat System

Threat is tracked in two ways:

- global threat
- optional per-faction threat

Threat affects:

- enemy aggression
- spawn density
- reinforcement rate
- world reactions

Threat changes come from:

- `ThreatDeltaOnSuccess`
- `ThreatDeltaOnFailure`
- action-level `ThreatDelta`
- action-level `FactionThreatDelta`

Use `ThreatThresholds` in the registry to define important escalation points.

Example thresholds:

- `25`
- `50`
- `75`

World systems can bind to `OnThreatThresholdCrossed` to react when those thresholds are crossed.

## Replayability Features

The mission asset supports basic replay variation.

### Random target counts

Enable:

- `bRandomizeTargetCount = true`
- `MinTargetCount`
- `MaxTargetCount`

Example:

- Destroy between 2 and 5 shield generators

### Variable time limits

You can make multiple mission assets with different timers, or extend the system later to randomize timers too.

### Hidden missions

Use `Hidden` start state plus `VisibilityConditions` or failure actions.

### Faction variation

Set `FactionID` and use `FactionThreatDelta` inside actions.

## How to Report Progress From Gameplay

Mission assets do not automatically detect gameplay events on their own. Your gameplay code or Blueprints must notify the mission manager.

Use:

- `ReportObjectiveProgress(MissionID, ObjectiveIndex, DeltaCount, ReporterRole)`

Examples:

- Player enters anomaly zone:
  - `MissionID = Sub_InvestigateAnomalies`
  - `ObjectiveIndex = 0`
  - `DeltaCount = 1`
  - `ReporterRole = Pilot`

- Gunner scans an anomaly:
  - `MissionID = Sub_InvestigateAnomalies`
  - `ObjectiveIndex = 1`
  - `DeltaCount = 1`
  - `ReporterRole = Gunner`

- Engineer repairs scanner:
  - `MissionID = Sub_RepairScanner`
  - `ObjectiveIndex = 0`
  - `DeltaCount = 1`
  - `ReporterRole = Engineer`

This call must happen on the server or through a server-authoritative flow.

## Mission UI (text readout)

To show **current active missions** and **objective progress** in a widget, and have the text update when `ReportObjectiveProgress` (or per-player progress) runs:

### 1. Get data for the readout

In your widget Blueprint:

- **Get Owning Player** → **Get Player State** (this is the local player’s `PlayerState`).
- **Get Game Instance** → **Get Subsystem** → **Mission Manager Subsystem**.
- Call **Get Active Missions For UI** with **For Player** = that Player State.

This returns an array of **Mission Summary For UI**:

- **Mission ID** — name of the mission
- **Mission Display Name** — from the mission asset (or mission ID if empty)
- **Objectives** — array of **Mission Objective Summary For UI**:
  - **Display Name** — from the objective (or "Objective 1", etc.)
  - **Current Count** / **Target Count** — e.g. 2/3
  - **b Completed**
  - **b Is Per Player** — whether this row is per-player progress

Build your text by looping over the returned array and each mission’s **Objectives**, e.g.  
`MissionDisplayName` then for each objective `DisplayName: CurrentCount/TargetCount` or a checkmark when **b Completed**.

**Tip:** If you only show objectives where **b Completed** is false, completed ones disappear. To show progress clearly (e.g. "Visit Waypoint 1 ✓" and "Visit Waypoint 2"), show all objectives and use **b Completed** to add a checkmark or strikethrough instead of hiding them.

### 2. When to refresh

- **On Construct** or **Add to Viewport**: call **Get Active Missions For UI** once and set your text.
- **When progress changes**: bind to the subsystem’s **On Objective Progress** (and optionally **On Per Player Objective Progress**). **On Objective Progress** fires for both session and per-player progress, so a single binding is enough for waypoint-style updates. In the bound event, call **Get Active Missions For UI** again and update the text.
- **Multiplayer clients**: progress is reported on the server, so the server’s subsystem broadcasts delegates; the client’s subsystem does not. On clients, also bind to **Mission Game State**’s **On Replicated Mission Data Changed** (get Game State, cast to **Mission Game State**). When that fires, call **Get Active Missions For UI** and refresh—replicated data has just arrived.

So: **one function** supplies all active missions + progress; **subsystem events** (and **Mission Game State**’s **On Replicated Mission Data Changed** for clients) tell the widget when to refresh. No polling needed.

### 3. Binding to the subsystem from the widget

- Store a reference to **Mission Manager Subsystem** (e.g. in **Construct** from **Get Game Instance** → **Get Subsystem**).
- **Bind Event to On Mission Activated**, **On Objective Progress**, and **On Mission Completed** on that reference. **On Objective Progress** fires for both session and per-player progress (e.g. waypoints calling **Report Objective Progress For Pawn**).
- In each bound function, call **Get Active Missions For UI** (with **Get Owning Player** → **Get Player State**) and refresh your mission text.
- For **multiplayer client** UI: get **Game State** from the world, cast to **Mission Game State**, and **Bind Event to On Replicated Mission Data Changed**. In that handler, call **Get Active Missions For UI** and refresh so the client UI updates when replicated mission/progress data arrives.

**Important: full refresh when mission changes.** When **On Mission Completed** or **On Mission Activated** fires (e.g. Main_Test1 succeeds and Main_Test2 is activated), the **list of active missions** changes: the completed mission is no longer active, and the new one is. Your refresh logic must **refetch and rebuild** the display from the new list. For example:
- Call **Get Active Missions For UI** in the event handler.
- If you show a **list** of missions: clear existing mission row widgets and create new ones (or rebind data) from the returned array.
- If you show a **single** mission: set the displayed mission from the **first** entry of the returned array (e.g. `Missions[0]`). When the list switches from `[Main_Test1]` to `[Main_Test2]`, the first entry is now Main_Test2, so the display updates. Do not only update "current count" for the previous mission—that leaves the old mission on screen.

### 4. Troubleshooting: "Mission succeeded but UI doesn't show Main_Test2"

- **Check Output Log**: When Main_Test1 completes you should see `MissionManager: CompleteMission Main_Test1 -> Success` and then `MissionManager: Fail-forward activated mission Main_Test2`. If you see both, the backend is correct and the issue is the UI refresh (see "full refresh" above). If you don't see them, the mission may not be completing (e.g. objectives not both reported, or On Success Actions not set: Target Mission ID = Main_Test2, Activate Mission = true).
- **Registry**: Main_Test2 must be in the same **Mission Registry** as Main_Test1 (the one used in **Register Missions From Registry**). Only missions in the registry are in **Mission Data Map** and appear in **Get Active Missions For UI**.
- **Full refresh**: In **On Mission Completed** and **On Mission Activated**, your handler must call **Get Active Missions For UI** and set the **entire** display from the result (e.g. first mission in list = current mission to show). If you only update progress counts for the existing widget, the widget will still show Main_Test1's name and objectives.
- **Set title every refresh**: In **Update Mission Text**, set the **mission title** from `Missions[0].Mission Display Name` (or `Mission ID`) and the objective list from `Missions[0].Objectives` on **every** refresh—not only in **Event Construct**. Otherwise when the list switches to Main_Test2, the title stays "Main_Test1".

### 5. Troubleshooting: "Waypoints work but Destroy Object never updates progress or completes"

If you use a shared **MissionCompleter** (or similar) that calls an interface **Update Mission for Pawn** and then the controller/BP_Missions implements it and calls **Report Objective Progress for Pawn**:

- **Pass the Pawn, not the Controller:** The C++ API is `ReportObjectiveProgressForPawn(APawn* Pawn, ...)`. Your interface’s "Pawn Actor" parameter must receive the **player Pawn**. In the MissionCompleter, after **Cast To Pawn (instigator)** succeeds, feed **As Pawn** into the **Pawn Actor** input of **Update Mission for Pawn**. Do **not** feed the Controller (e.g. **As PC Controller**) into that input. If you pass the Controller, the implementation will **Cast To Pawn (Pawn Actor)** and it will always fail (Controller is not a Pawn), so progress is never reported and you may see "Failed to cast proper pawn for Objective Reporting".
- **Instigator must be the player Pawn for both waypoints and destroy:** The component’s **AdvanceObjective** often checks that the **instigator** is the player (e.g. class == BP_Movescript) and then uses it to get the Pawn. For waypoints, the overlapping actor is usually the pawn. For **DestroyObject**, when the object is destroyed you must call **AdvanceObjective** with the **player Pawn** as instigator (e.g. damage instigator from the destroy event, or the owning pawn that triggered the destroy). If you pass the projectile, the destroyed actor, or something that isn’t the player pawn, the class check fails and **Update Mission for Pawn** is never called, so no progress.

After fixing: re-run, destroy one object, and confirm in Output Log that you do **not** see "Failed to cast proper pawn...". You should see mission progress logs and the UI updating.

### 6. Troubleshooting: "Destroy 4 objects – progress goes 1→4 but UI never shows mission complete"

- **Session vs Per-player**: If the mission objective is **Session** (e.g. "destroy 4 targets"), use **Report Objective Progress** (Mission ID, Objective Index 0, Delta 1). Do **not** use **Report Objective Progress For Pawn** for that objective—that path only updates **Per-player** objectives and will no-op for Session. If you reuse a waypoint component that calls **Report Objective Progress For Pawn**, either switch the mission objective to **Per player** (so the same call works) or add a separate call from the destructible to **Report Objective Progress** when the object is destroyed.
- **UI must refresh when mission completes**: **Get Active Missions For UI** only returns missions in **Active** state. When Main_Test2 completes it becomes **Completed_Success** and is no longer in that list. If the widget only refreshes on **On Objective Progress**, you will see 4/4 and the mission will never disappear. **Bind to On Mission Completed** on the Mission Manager Subsystem and in that handler call **Get Active Missions For UI** and rebuild the entire display (e.g. first mission in list or clear list). For multiplayer clients, also bind to **Mission Game State** → **On Replicated Mission Data Changed** and do the same full refresh.
- **Confirm in log**: When the 4th object is destroyed, check Output Log for `MissionManager: CompleteMission Main_Test2 -> Success`. If you see it, the backend is completing and the fix is UI refresh (see above). If you do not see it, the mission is not completing—check objective **Scope** (Session vs Per player) and that you use the matching report function.

### 7. Troubleshooting: "On Mission Completed fires for Main_Test1 but not for Main_Test2"

- **Server-only delegate**: **On Mission Completed** is broadcast only on the **server** (where mission logic runs). In single-player PIE you are the server, so the delegate fires. In **multiplayer** or **2+ player PIE**, the UI often runs on the **client**; the client’s Mission Manager Subsystem never broadcasts **On Mission Completed**. So the same UI that worked for Main_Test1 (e.g. in 1-player) may not get the event for Main_Test2 if you are testing as a client. **Fix:** Also bind your mission widget to **Mission Game State** → **On Replicated Mission Data Changed**. In that handler, call **Get Active Missions For UI** and refresh the display. When Main_Test2 completes on the server, its state replicates; the client’s Game State fires **On Replicated Mission Data Changed** and your refresh runs, so the completed mission disappears from the list.
- **Wrong report for Session objective**: If Main_Test2’s “destroy 4” objective is **Session** but the destructibles call **Report Objective Progress For Pawn** (same as waypoints), the subsystem does nothing for that objective (it only accepts Per-player when using the For Pawn path). You will see a log: `ReportObjectiveProgressForPlayer called for Main_Test2 [0] but objective is Session (use ReportObjectiveProgress instead)`. Progress will not update and the mission will never complete. **Fix:** From the destructible, call **Report Objective Progress** (Mission ID, Objective Index 0, Delta 1)—not Report Objective Progress For Pawn. Or set the objective to **Per player** and keep using For Pawn (and pass a valid player pawn).
- **Mixed objectives**: If Main_Test2 has both a **Session** objective (e.g. destroy 4) and a **Per-player** objective, the mission will not complete from session progress alone unless **bCompleteWhenAllPlayersFinishPerPlayerObjectives** is true on the mission asset. You will see a log: `Main_Test2 has PerPlayer objectives and bCompleteWhenAllPlayersFinishPerPlayerObjectives is false`. **Fix:** Use only one objective for the “destroy 4” mission, or enable the checkbox and ensure all players complete the per-player objective.
- **No progress row**: If you see `ReportObjectiveProgress Main_Test2 [0] no progress row`, the mission was never activated with a session objective (e.g. Main_Test2 was not activated after Main_Test1, or the registry is wrong). **Fix:** Ensure Main_Test1’s On Success Actions activate Main_Test2 and Main_Test2 is in the same registry.

When the 4th object is destroyed, check the Output Log for **Session objective complete Main_Test2 [0], bShouldComplete=1** and **Broadcasting OnMissionCompleted Main_Test2**. If the first appears with **bShouldComplete=0**, read the log line above it for the reason. If neither appears, the report is not running on the server or the wrong report function is used.

If the widget is created before the subsystem exists, get the subsystem when the widget is added to the viewport or when the player is ready.

## Example Mission Set

This section shows one possible setup matching your hovercraft mission structure.

### Main mission: `Main_SecureRelay`

- `MissionType = Main`
- starts active through the registry

Objectives:

1. Repair scanner
2. Defend relay for 60 seconds

Success:

- `ThreatDeltaOnSuccess = -5`
- activate next sub-mission

Failure:

- `ThreatDeltaOnFailure = 10`
- activate `Side_OxygenLeak`
- fire `AudioEvent_MissionFailure`

### Sub mission: `Sub_InvestigateAnomalies`

- `MissionType = Sub`
- `ParentMissionID = Main_SecureRelay`

Objectives:

1. Pilot enters anomaly zone A
2. Gunner scans anomaly A
3. Pilot enters anomaly zone B
4. Gunner scans anomaly B
5. Pilot enters anomaly zone C
6. Gunner scans anomaly C

Failure path:

- increase faction threat
- activate reinforcement side mission

### Side mission: `Side_EmergencyRepair`

- `MissionType = Side`
- hidden by default
- activated when a repair-related mission fails

Objectives:

1. Engineer repair damaged subsystem
2. Maintain oxygen above threshold for duration

Reward:

- reduce further escalation

## Recommended Design Patterns

### Pattern 1: Main mission with child phases

Use one main mission plus several sub missions.

Example:

- `Main_SecureRelay`
- `Sub_RepairScanner`
- `Sub_InvestigateAnomalies`
- `Sub_DestroyGenerators`

Completion of one phase activates the next.

### Pattern 2: Hidden fail-forward missions

Keep recovery missions hidden until something goes wrong.

Example:

- failed defense activates `Side_EmergencyRepair`
- failed anomaly scan activates `Side_ContainBreach`

### Pattern 3: Threat-controlled branching

Use visibility conditions based on threat.

Example:

- if threat is low, reveal a bonus mission
- if threat is high, reveal a crisis mission

### Pattern 4: Role pairing

Split a mission into chained role tasks.

Example:

- Pilot enters zone
- Gunner scans target
- Engineer stabilizes system

This makes the multiplayer roles matter without hardcoding mission flow into level actors.

## Best Practices

- Keep `MissionID` values unique and stable
- Always define both success and failure consequences
- Use side missions for fail-forward recovery, not just bonuses
- Keep objective responsibilities clear by role
- Prefer mission assets over hardcoded Blueprint mission graphs
- Use `WorldEventName` for abstract hooks like audio, lighting, AI, and hazards
- Use threat thresholds for large world-state shifts

## Current System Notes

These are important for designers and implementers.

- Missions are data-driven, but gameplay events still need to call `ReportObjectiveProgress()`
- Visibility conditions currently behave as AND across entries
- The main mission is chosen by `MainMissionID` in the registry
- The system is server-authoritative
- Mission state, objective progress, and threat replicate through `MissionGameState`

## Troubleshooting

### I cannot create a mission asset

Make sure the project compiled successfully and create:

- `Add` -> `Miscellaneous` -> `Primary Data Asset`

Then choose:

- `MissionDataAsset`
- `MissionRegistryAsset`

### My mission never becomes visible

Check:

- `MissionID` references are spelled exactly the same
- the required mission state is actually reached
- threat min and max allow the current threat
- enough session time has elapsed

### My objective never completes

Check:

- your gameplay actually calls `ReportObjectiveProgress()`
- the objective index is correct
- the role matches `RequiredRole`
- the mission is already `Active`

### Both objectives disappear when I complete one waypoint

If the UI only shows objectives that are **not** completed, and both disappear after one waypoint visit, **both objectives are being marked complete**. The mission system only updates one objective per report, so either:

1. **Both waypoint volumes are triggering** when you enter one area (e.g. the two colliders overlap, or both overlap the pawn). Fix: separate the waypoint volumes so only one overlaps the player at a time.
2. **One waypoint Blueprint is reporting for both indices** (e.g. a loop that reports 0 and 1, or two Report nodes). Fix: ensure each waypoint has a single **Report Objective Progress For Pawn** call with **only** its own Objective Index (0 or 1).

To confirm: add a **Print String** in each waypoint with its Objective Index. If both print when you enter one waypoint, the volumes overlap. Optionally show completed objectives in the UI (e.g. with a checkmark) so you see "Objective 1 ✓" and "Objective 2" instead of hiding completed ones.

### Clients do not see updates

Check:

- your GameMode uses `MissionGameState`
- mission logic runs on the server
- progress is reported through a server-authoritative path

## Suggested Workflow For Designers

1. Create all mission assets first.
2. Assign unique mission IDs.
3. Build the mission registry.
4. Define success and failure outcomes for every mission.
5. Add threat changes next.
6. Add visibility conditions and side branches.
7. Hook gameplay events to `ReportObjectiveProgress()`.
8. Test the success path.
9. Test the failure path.
10. Test high-threat and low-threat session branches.

## Suggested Workflow For Blueprints Or C++

When gameplay events happen:

1. Get `MissionManagerSubsystem`
2. Call `ReportObjectiveProgress()`
3. Let the subsystem handle completion, threat, and branch activation
4. Use mission events to drive UI, spawning, music, or hazards

## Final Advice

Think of each mission asset as a rule set, not a script.

The asset should define:

- what the objective is
- who is allowed to do it
- when the mission appears
- what happens if players succeed
- what happens if players fail

The runtime system should only execute those rules and replicate the results.
