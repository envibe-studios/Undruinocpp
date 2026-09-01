# Playbook: Testing and Validation

Use this playbook after any meaningful code, Blueprint, hardware, networking, UI, mission, or systems change.

The goal is to prove the change works at the correct level and that shared systems were not unintentionally broken.

## 1. Define What Success Means

Before testing, identify:

- what behavior should now work
- what state should change
- what output should appear
- which systems are expected to remain unaffected

Testing should verify specific expected outcomes, not just "nothing crashed."

## 2. Test the Smallest Relevant Layer First

Start as close as possible to the changed code.

Examples:

- parser unit/input behavior
- component state change
- Blueprint event firing
- hardware packet receipt
- diagnostic response
- widget value update

Only move outward once the lower layer behaves correctly.

## 3. Compile Verification

For C++ changes:

- compile the affected module/project
- inspect compiler errors
- inspect Unreal Header Tool errors
- inspect warnings

For Blueprint changes:

- compile affected Blueprints
- inspect errors
- inspect warnings
- check broken references

A successful text edit is not a successful implementation.

## 4. Editor-State Verification

Use Unreal MCP where appropriate to inspect:

- created assets
- actor placement
- component configuration
- Blueprint compile status
- asset references
- level state

Do not assume an MCP command succeeded merely because it returned without an obvious error.

Verify the resulting Unreal state.

## 5. Runtime Verification

When appropriate, run PIE and exercise the changed behavior.

Check:

- expected state transitions
- gameplay result
- events/delegates
- logs
- UI
- actor behavior
- hardware callbacks
- replication

If the feature cannot be fully tested automatically, state what still requires manual testing.

## 6. Hardware Validation

For hardware-related work, verify the complete path relevant to the feature.

Possible inbound path:

Hardware
→ ESP32 firmware
→ ESP-NOW
→ Andy
→ Serial
→ Unreal parser
→ Gameplay/Blueprint/UI

Possible outbound path:

Unreal
→ Serial
→ Andy
→ ESP-NOW
→ Hardware node

Do not infer end-to-end success from only one layer.

## 7. Diagnostic Validation

If the feature participates in diagnostics:

- verify it appears where expected
- verify success state
- intentionally inspect failure handling when practical
- verify useful failure information is reported
- verify existing diagnostic output still parses correctly

## 8. Regression Testing

Identify the systems that share the modified code or data path.

Test those specifically.

Examples include:

- Port and Starboard equivalents
- other hardware nodes
- shared parser paths
- shared components
- child Blueprints
- multiple ships
- mission consumers
- widgets listening to the same delegates
- common message formats

Do not run arbitrary regression tests with no connection to the change.

## 9. Symmetry Checks

Whenever one side or variant is modified, check its counterpart where applicable.

Examples:

- Port / Starboard
- Client / Server
- Inbound / Outbound
- Physical hardware / PIE fallback
- Single ship / multiple ships
- Loaded magazine / no magazine
- Online / offline

Many bugs hide in asymmetric implementations.

## 10. Failure-Mode Testing

Where practical, test what happens when:

- hardware is absent
- a node is offline
- serial data is malformed
- a reference is null
- an asset is missing
- initialization occurs late
- data arrives out of order
- a device reconnects
- PIE begins without physical hardware

The system should fail visibly and safely rather than silently.

## 11. Log Review

Inspect relevant logs after testing.

Look for:

- warnings
- errors
- repeated retries
- unexpected spam
- stale references
- missing assets
- failed casts
- network warnings
- Blueprint runtime errors

A feature can appear to work while still generating important errors.

## 12. Performance Awareness

For changes affecting frequent updates, consider:

- Tick usage
- high-frequency serial messages
- repeated allocations
- excessive Blueprint events
- repeated asset lookup
- excessive logging
- unnecessary replication

Do not optimize prematurely, but do not introduce obviously expensive behavior into hot paths.

## 13. Save and Restart When Required

Some Unreal changes require more than Live Coding.

Use a full editor restart/rebuild when appropriate, especially after:

- reflected type changes
- inheritance changes
- UPROPERTY changes
- UFUNCTION signature changes
- major plugin/module changes

Do not treat Live Coding success as proof that all reflected state is correct.

## 14. Documentation Check

Before declaring the task complete, ask:

Did this change make any project documentation inaccurate?

If yes, update only the affected documentation.

## 15. Completion Criteria

A meaningful task is complete only when:

- implementation exists
- affected code/assets compile
- expected behavior was verified
- relevant regressions were checked
- diagnostics work where applicable
- required documentation is current
- unresolved manual tests are clearly identified

## 16. Completion Report

When reporting completion, include:

- what was tested
- what passed
- what failed
- what could not be tested automatically
- what manual verification remains
- whether regression checks were performed