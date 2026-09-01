# Playbook: Adding or Modifying Missions

Use this playbook whenever creating, extending, or debugging mission content.

The goal is to preserve the existing mission architecture, keep missions modular, and avoid putting one-off gameplay logic into places that make future missions harder to maintain.

## 1. Inspect the Existing Mission System First

Before making changes:

- read the existing mission documentation
- inspect the mission manager / subsystem
- inspect existing mission classes or data assets
- inspect mission state handling
- inspect objective handling
- inspect completion/failure behavior
- inspect any UI listeners
- inspect any threat or enemy-system integration

Look for an existing mission that is structurally similar to the requested one.

Prefer extending established patterns over inventing new ones.

## 2. Define the Mission Experience

Before implementation, identify:

- mission premise
- player objective
- how the mission begins
- required gameplay interactions
- success condition
- failure condition, if any
- whether failure is terminal or fail-forward
- whether the mission changes threat level
- whether enemies are involved
- whether hardware stations are involved
- whether the mission requires new UI

Do not start coding until the mission's completion conditions are clear.

## 3. Separate Mission Logic from Gameplay Systems

Mission code should orchestrate existing gameplay systems rather than duplicate them.

For example:

A mission may require the player to disable an enemy ship.

The mission should observe the enemy ship's existing disabled/destroyed state.

It should not create a second enemy-health implementation purely for the mission.

Likewise, missions should reuse existing:

- weapon systems
- scanner systems
- tractor systems
- navigation
- enemies
- hardware inputs
- ship systems
- objective UI
- threat systems

Mission logic should primarily answer:

"What needs to happen next?"

rather than:

"How does this gameplay mechanic work?"

## 4. Prefer Event-Driven Progression

Where possible, advance missions using:

- delegates
- gameplay events
- state changes
- objective completion callbacks

Avoid unnecessary Tick-based polling.

If polling is necessary, document why.

Do not repeatedly inspect expensive world state every frame when an event can communicate the same information.

## 5. Define Mission States Clearly

Represent meaningful stages explicitly.

Examples:

- NotStarted
- Active
- Objective1
- Objective2
- Escalation
- Extraction
- Completed
- Failed

Use the existing mission-state conventions if present.

Avoid ambiguous combinations of unrelated booleans when a clear state machine would better represent progression.

## 6. Preserve Fail-Forward Behavior

Where the existing mission architecture supports fail-forward design, preserve it.

A player mistake should not automatically dead-end the experience unless the mission is intentionally designed that way.

Consider alternatives such as:

- alternate objectives
- degraded rewards
- escalation
- reinforcement waves
- changed dialogue
- emergency recovery objectives

Do not introduce hard failure states casually into systems designed around continued play.

## 7. Objective Design

Each objective should have:

- a clear trigger
- a clear completion condition
- a clear player-facing description
- an observable state
- a defined next step

Avoid objectives whose completion depends on hidden or ambiguous conditions.

If several systems can satisfy an objective, explicitly support those pathways rather than relying on accidental behavior.

## 8. Mission and Threat Integration

If the mission affects threat or enemy activity:

- inspect the existing threat system
- use established threat-scaling paths
- avoid spawning enemies through a parallel system
- preserve server authority where applicable
- ensure mission cleanup does not leave persistent unwanted threats

Mission-specific enemy behavior should reuse existing AI systems wherever practical.

## 9. Hardware Integration

If a mission requires physical controls:

- use the existing Unreal hardware abstraction
- do not access serial directly from mission logic
- do not create mission-specific ESP-NOW behavior unless necessary
- treat hardware input as another gameplay input source

The mission should react to gameplay state produced by hardware systems rather than owning hardware communication itself.

## 10. Multiplayer / Ship Ownership

When mission behavior involves multiple ships or networked players:

- respect ShipId routing
- identify authoritative mission state
- avoid global state when mission state belongs to a specific ship
- ensure events affect the intended ship/player
- verify replication where needed

Do not assume a single-ship environment unless the architecture explicitly guarantees it.

## 11. UI Integration

Mission UI should consume mission state rather than own it.

Use existing objective/toast/UI systems where possible.

Avoid placing core mission progression inside widgets.

UI should display:

- objective text
- status
- progress
- success/failure feedback

while mission systems remain authoritative.

## 12. Audio and Presentation

When adding presentation hooks:

- expose events for Blueprint/audio where appropriate
- avoid coupling core mission logic directly to specific presentation assets
- preserve the ability to change VO, VFX, widgets, or cinematics independently

## 13. Cleanup and Mission End

When a mission ends, verify cleanup of:

- delegates
- timers
- spawned actors
- temporary UI
- temporary threat modifiers
- mission-specific state
- temporary objectives
- event bindings

Do not leave listeners or timers active after mission completion.

## 14. Save / Reload Considerations

If mission state can persist across level transitions or saves, inspect the existing persistence architecture before adding state.

Do not create a separate persistence mechanism for one mission.

If persistence is not currently supported, do not imply that it is.

## 15. Test the Mission in Stages

Test:

### Start
- mission initializes
- first objective appears
- required actors/systems exist

### Progression
- each objective completes
- next stage triggers exactly once
- repeated events do not advance stages incorrectly

### Success
- completion state fires
- rewards/effects occur
- UI updates
- mission cleanup occurs

### Failure / Alternate Paths
- intended failure behavior works
- fail-forward paths work where applicable
- no permanent soft lock occurs unintentionally

## 16. Regression Checks

After changing shared mission infrastructure, test more than the new mission.

Verify at least one existing mission that uses the affected path.

Pay attention to:

- objective sequencing
- event binding
- threat integration
- enemy spawning
- UI
- mission completion
- fail-forward behavior

## 17. Use Unreal MCP Where Helpful

Use MCP to:

- inspect mission-related Blueprints
- inspect placed actors
- verify asset references
- compile affected Blueprints
- inspect level configuration

Do not recreate assets unnecessarily.

## 18. Update Documentation

If the change introduces:

- a new mission architecture pattern
- a new mission state type
- a new reusable objective type
- new threat integration
- a new mission-system dependency

update the appropriate project documentation.

If a repeatable mission-authoring workflow changes, update this playbook.

## 19. Completion Report

When finished, summarize:

- mission created or modified
- states/objectives added
- gameplay systems reused
- new systems introduced
- Blueprint/assets changed
- threat/enemy changes
- tests performed
- alternate/failure paths tested
- documentation updated
- any manual playtesting still required