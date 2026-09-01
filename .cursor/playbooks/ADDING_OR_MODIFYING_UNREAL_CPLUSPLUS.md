# Playbook: Adding or Modifying Unreal C++

Use this playbook whenever adding or changing Unreal Engine C++ code.

The goal is to make changes that fit the existing codebase, compile cleanly, preserve Blueprint/API compatibility, and avoid unnecessary architectural drift.

## 1. Inspect Before Editing

Before modifying code:

- identify the relevant module
- inspect the existing class hierarchy
- find similar classes or components
- check Blueprint exposure
- check delegates, interfaces, subsystems, and data assets involved
- inspect related `.Build.cs` dependencies if needed

Do not create a new pattern if an established one already solves the problem.

## 2. Determine Ownership

Before adding new logic, decide where it belongs.

Possible owners include:

- Actor
- ActorComponent
- UObject
- Subsystem
- GameMode
- PlayerController
- PlayerState
- GameState
- data asset
- plugin
- hardware communication layer

Prefer the narrowest reusable owner that matches existing project architecture.

Avoid putting unrelated responsibilities into large existing classes simply because they are convenient.

## 3. Preserve Public Interfaces

Before changing:

- UPROPERTY names
- UFUNCTION signatures
- BlueprintCallable functions
- BlueprintImplementableEvent / BlueprintNativeEvent
- delegates
- interfaces
- enums
- structs

search for references.

Treat Blueprint-facing API changes as potentially breaking changes.

Prefer additive changes over renaming or removing existing public API.

## 4. Follow Unreal Reflection Rules

When using Unreal reflection:

- use appropriate UCLASS, USTRUCT, UENUM, UPROPERTY, and UFUNCTION macros
- preserve GENERATED_BODY()
- use Unreal-supported property types
- use forward declarations where appropriate
- include full headers where required
- avoid circular dependencies

Do not expose implementation details to Blueprint without a reason.

## 5. Respect Module Boundaries

Before adding includes or dependencies:

- identify which module owns the type
- check whether the dependency belongs in PublicDependencyModuleNames or PrivateDependencyModuleNames
- avoid unnecessary module coupling
- do not introduce plugin-to-game dependencies casually

If an unusual dependency already exists, understand it before extending it.

## 6. Prefer Existing Project Patterns

Reuse established approaches for:

- logging
- serial parsing
- ShipId routing
- hardware message handling
- diagnostics
- delegates
- components
- subsystems
- replication
- timers
- data-driven configuration

Do not introduce a second parser, routing layer, diagnostic format, or ownership model without a clear architectural need.

## 7. Minimize Header Coupling

Prefer:

- forward declarations in headers
- implementation includes in `.cpp`
- narrow interfaces
- private implementation details

Avoid adding broad includes to commonly used headers unless necessary.

Be mindful that unnecessary header dependencies increase Unreal compile times.

## 8. Blueprint Exposure

Expose C++ functionality to Blueprint when it provides real value.

Good candidates include:

- configurable properties
- designer-facing events
- state queries
- lightweight commands
- asset references
- presentation hooks

Keep complex reusable logic in C++ where appropriate.

Do not expose every internal variable simply because Blueprint access is possible.

## 9. Logging and Diagnostics

Use existing project logging conventions.

For hardware, networking, or stateful systems, add useful diagnostics where failure would otherwise be opaque.

Logs should help answer:

- what happened
- which object/node/ship was involved
- what state changed
- why something failed

Avoid high-frequency log spam in hot paths.

Do not remove useful existing diagnostics to make output quieter without addressing the underlying issue.

## 10. Networking and Authority

When modifying networked gameplay:

- identify server vs client ownership
- preserve authoritative paths
- check replication requirements
- use RPCs only where appropriate
- avoid trusting client-owned state for authoritative gameplay

If hardware input participates in multiplayer behavior, understand the current ShipId and authority model before changing it.

## 11. Lifetime and Pointer Safety

Use Unreal ownership patterns appropriately.

Be careful with:

- raw UObject pointers
- weak references
- delegates
- timers
- async callbacks
- component destruction
- PIE teardown
- editor reloads

Prefer UPROPERTY-managed references where required for garbage collection.

Avoid retaining stale UObject pointers across lifecycle boundaries.

## 12. Implement Incrementally

For substantial changes:

1. add or adjust data definitions
2. implement the smallest core behavior
3. expose required hooks
4. compile
5. fix errors before adding more
6. integrate Blueprint/editor changes
7. test runtime behavior

Do not stack many speculative changes before compiling.

## 13. Compile and Verify

After meaningful C++ changes:

- compile the affected module/project
- inspect compiler errors
- inspect Unreal Header Tool errors
- inspect warnings
- check Blueprint breakage caused by reflected API changes

Do not treat successful text edits as successful implementation.

## 14. Check Hot Reload / Live Coding Limitations

Be aware that some changes are unsafe or unreliable with Live Coding, especially:

- reflected type layout changes
- constructor changes
- UPROPERTY additions/removals
- UFUNCTION signature changes
- inheritance changes
- enum/struct changes

When appropriate, recommend or perform a full editor restart and rebuild rather than trusting Live Coding.

Do not assume Live Coding proves a reflected API change is valid.

## 15. Runtime Test

When practical:

- run PIE
- trigger the changed behavior
- inspect logs
- verify expected delegates/events
- verify Blueprint integration
- verify hardware flow if relevant
- verify multiplayer behavior if relevant

## 16. Regression Check

Test systems that share the modified code path.

Examples:

- both weapons
- all hardware nodes
- diagnostics
- serial routing
- multiple ships
- mission systems
- UI listeners
- child Blueprints

Target actual dependencies rather than performing unrelated checks.

## 17. Refactoring

Refactor when it improves maintainability and is directly related to the task.

Do not use a small feature request as an excuse for a broad rewrite.

If a larger refactor would materially improve the system but carries risk, propose it separately.

## 18. Documentation

Update project documentation when the change introduces:

- a new subsystem
- a new architectural responsibility
- a new public integration path
- a new networking or hardware pattern
- a meaningful architectural decision

Do not document routine implementation details.

## 19. Completion Report

When finished, summarize:

- C++ files changed
- classes/functions added or modified
- Blueprint-facing API changes
- module dependency changes
- compile status
- runtime tests performed
- regression checks performed
- documentation updated
- any editor restart or manual verification still required