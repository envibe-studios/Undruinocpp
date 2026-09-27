# Playbook: Modifying Blueprints with MCP

Use this playbook whenever an AI agent modifies an existing Unreal Blueprint through MCP.

The goal is to make targeted, reversible editor changes without damaging working Blueprint logic.

## 1. Inspect Before Editing

Before changing an existing Blueprint:

- Open or inspect the Blueprint.
- Identify its parent class.
- Identify relevant components.
- Identify the specific graph, function, macro, or variable involved.
- Understand how the requested change connects to existing behavior.

Do not rebuild a Blueprint from scratch when a targeted edit is possible.

Do not assume the Blueprint is disposable.

## 2. Confirm Scope

Determine exactly what needs to change.

Examples:

- add one component
- add one variable
- change a default value
- connect an existing event
- add a branch
- expose a C++ property
- update a widget binding
- add diagnostic output

Avoid unrelated cleanup during the same edit.

## 3. Preserve Existing Logic

Do not:

- delete unrelated nodes
- disconnect unrelated pins
- replace working graphs for stylistic reasons
- rename public variables without checking references
- change parent classes casually
- remove interfaces
- remove implemented events
- remove existing diagnostics

Prefer additive or narrowly targeted modifications.

## 4. Check References Before Structural Changes

Before changing or deleting:

- variables
- functions
- components
- interfaces
- event dispatchers
- inherited classes

check whether they are referenced elsewhere.

If the change may break dependent Blueprints, explain the risk before proceeding.

## 5. Prefer C++ for Core Logic When Appropriate

If an edit would introduce substantial reusable logic into Blueprint, consider whether the behavior belongs in C++ instead.

Good Blueprint uses include:

- editor configuration
- asset references
- presentation
- level-specific logic
- tuning
- simple orchestration

Do not migrate working Blueprint logic into C++ unless there is a clear architectural benefit.

## 6. Make the Smallest Coherent Change

When using MCP:

- modify only the necessary graph or asset
- preserve node layout where practical
- use established naming conventions
- avoid creating duplicate variables or functions
- reuse existing helpers and interfaces

Do not create a second implementation of behavior that already exists elsewhere.

## 7. Compile Immediately

After a meaningful Blueprint change:

- compile the Blueprint
- inspect compile errors
- inspect warnings
- verify references remain valid

Do not continue stacking edits on top of a Blueprint that no longer compiles.

## 8. Save Intentionally

Save the asset after successful verification.

Do not repeatedly save broken intermediate states when avoidable.

If a change fails midway, restore or repair the Blueprint before continuing.

## 9. Test Runtime Behavior

When appropriate:

- run PIE
- trigger the modified behavior
- verify expected events fire
- inspect runtime logs
- check visual output
- check hardware interaction if relevant

A successful Blueprint compile does not prove the feature works.

## 10. Use Temporary Diagnostics Carefully

Temporary Print String nodes or logging may be added during debugging.

When debugging is complete:

- remove temporary noise unless it has lasting diagnostic value
- preserve existing permanent diagnostics
- do not remove useful observability simply because the immediate bug is fixed

## 11. Avoid Destructive MCP Operations

Before any destructive operation, such as:

- deleting an asset
- replacing a major graph
- changing a Blueprint parent
- renaming widely referenced assets
- moving assets
- deleting variables/functions/components

confirm that the change is necessary and understand its dependencies.

If uncertainty remains, ask before proceeding.

## 12. Verify Neighboring Systems

After changing a Blueprint, test dependent systems where relevant.

Examples:

- child Blueprints
- actors placed in the level
- widgets
- interfaces
- event dispatchers
- C++ bindings
- mission logic
- hardware callbacks

## 13. Update Documentation Only When Needed

Update project documentation if the Blueprint change introduces:

- a new architecture pattern
- a new major asset responsibility
- a new subsystem dependency
- a new reusable workflow

Do not document routine graph edits.

## 14. Completion Report

When finished, summarize:

- Blueprint assets changed
- graphs/functions/components affected
- compile status
- runtime verification performed
- any dependent assets checked
- any unresolved risks or manual verification still required