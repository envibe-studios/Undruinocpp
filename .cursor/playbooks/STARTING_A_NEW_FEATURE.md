# Playbook: Starting a New Feature

Use this playbook before implementing any meaningful new gameplay, hardware, UI, networking, mission, AI, or systems feature.

The goal is to make new work fit the existing project instead of creating isolated one-off systems.

## 1. Define the Feature

Before editing anything, restate:

- What the user wants
- What problem the feature solves
- What the expected player or hardware behavior is
- What counts as "done"

If the request is ambiguous in a way that affects architecture, ask before implementing.

Do not ask unnecessary questions that can be answered by inspecting the project.

## 2. Inspect Before Designing

Search the project for:

- similar systems
- related C++ classes
- relevant Blueprints
- existing interfaces
- delegates
- subsystems
- components
- data assets
- message types
- UI patterns
- existing diagnostics

Use Unreal MCP when editor state or assets are relevant.

Do not design from memory when the project can provide evidence.

## 3. Identify the Existing Pattern

Determine whether the feature should extend:

- an existing subsystem
- an Actor Component
- an existing base class
- a Blueprint family
- a data-driven system
- the hardware protocol
- an existing UI framework

Prefer extension over duplication.

If no existing pattern fits, explicitly explain why a new pattern is needed.

## 4. Map Integration Points

Before implementation, identify which layers are affected.

Possible layers include:

- physical hardware
- ESP32 firmware
- ESP-NOW
- Andy
- serial
- Unreal parser
- C++ gameplay systems
- Blueprint
- replication
- AI
- missions
- UMG
- CRT displays
- audio
- level actors
- save/state systems
- diagnostics

Only modify layers actually required by the feature.

## 5. Consider Failure Behavior

Ask:

- What happens if hardware disconnects?
- What happens if data is missing?
- What happens if an asset is absent?
- What happens if the feature initializes in the wrong order?
- What happens in PIE without physical hardware?
- What happens on another ship?
- What happens during reconnect?

Prefer graceful degradation over silent failure.

## 6. Consider Diagnostics

For any system that can fail or become disconnected, determine whether diagnostics should expose:

- initialization state
- online/offline state
- current mode
- last update
- failure reason
- input/output status

Diagnostics are part of the feature where relevant, not a later cleanup task.

## 7. Decide C++ vs Blueprint

Use C++ when the feature involves:

- reusable architecture
- protocol parsing
- networking
- hardware integration
- shared gameplay logic
- performance-sensitive systems
- reusable components or subsystems

Use Blueprint when the feature benefits from:

- editor configuration
- content-specific behavior
- level scripting
- presentation
- animation
- rapid tuning
- designer-facing iteration

Hybrid implementations are preferred when they provide a clean reusable C++ core with flexible Blueprint configuration.

## 8. Create an Implementation Plan

For meaningful features, briefly outline the implementation order before making changes.

Example:

1. Extend shared data definitions.
2. Add core C++ behavior.
3. Expose required Blueprint hooks.
4. Modify editor assets through MCP.
5. Add diagnostics.
6. Test.
7. Update documentation.

Keep the plan proportional to the feature.

Do not produce elaborate design documents for trivial changes.

## 9. Implement Incrementally

Prefer small coherent changes.

After each major layer:

- compile
- inspect errors
- verify assumptions

Do not make large unrelated refactors unless necessary.

Do not alter working systems merely because a different style is preferred.

## 10. Verify the Result

Test the feature at the lowest useful level first, then through the full path.

Check:

- compilation
- Blueprint compile status
- runtime behavior
- expected events
- hardware flow where applicable
- diagnostics
- regressions

Use Unreal MCP to inspect or test editor-facing work when appropriate.

## 11. Perform Regression Checks

Identify systems that share the modified path and verify they still work.

Examples:

- other weapons
- other hardware nodes
- existing missions
- ship routing
- serial parser
- diagnostics
- shared UI
- replication

Regression testing should target actual dependencies, not an arbitrary checklist.

## 12. Update Project Knowledge

If the feature introduces:

- a new subsystem
- a new architectural pattern
- a new hardware node
- a new protocol decision
- a meaningful dependency
- a new standard workflow

update the appropriate documentation or playbook.

Do not document minor implementation details that do not affect future engineering.

## 13. Completion Report

When done, summarize:

- what was added
- architecture used
- important files/assets changed
- tests performed
- diagnostics added
- documentation updated
- anything still requiring manual verification