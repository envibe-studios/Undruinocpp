# Unduinocpp — Engineering Philosophy

This document describes the **design philosophy** that future engineers and AI assistants should follow when working on Unduinocpp.

It does **not** describe system inventories, class lists, or implementation details. For those, see [PROJECT_ARCHITECTURE.md](./PROJECT_ARCHITECTURE.md).

**Authority ladder (when guidance conflicts):**

1. `.cursor/rules/00-project-core.mdc` — mandatory engineering priorities
2. This document — how to think and decide
3. `.cursor/docs/PROJECT_ARCHITECTURE.md` — what exists today
4. Designer / plugin guides — domain-specific authoring
5. Older root `README.md` / `CLAUDE.md` — historical; may be stale

Where this document goes beyond explicit project rules, statements are labeled as **Recommended engineering guideline** (inferred from established patterns). Unlabeled principles below are treated as project facts from rules and documented practice.

---

## 1. Project identity (why these principles exist)

Unduinocpp is a **physical spaceship bridge simulator**. Software, Blueprints, physical controls, displays, ESP32 hardware, serial/ESP-NOW paths, and diagnostic/fault-reporting systems form one product — not a game with optional hardware bolted on later.

Engineering decisions should respect that:

- Hardware bring-up and in-session diagnostics are first-class.
- Gameplay systems must remain composable with physical I/O and multi-station setups.
- Stability of working paths matters more than aesthetic purity or speculative redesign.

---

## 2. Architecture philosophy

### 2.1 Preserve-first

**Established:** Preserve existing working systems unless there is a clear reason to change them. Prefer maintainable, extensible architecture over quick hacks. Do not silently remove functionality. Do not remove diagnostic logging, hardware support, compatibility behavior, or existing interfaces unless explicitly requested or clearly necessary.

Implication: the default answer to “should we rewrite / replace this?” is **no**, unless the current path is blocking a real requirement and the replacement plan preserves behavior for existing consumers.

### 2.2 Extend, do not parallelize

**Established (architecture guidance):** Do not invent parallel hardware stacks. Extend the existing Andy/serial patterns and shared hubs.

**Recommended engineering guideline:** The same “extend, don’t fork” mindset applies beyond hardware:

- Prefer adding a focused component, subsystem, data asset, or delegate hook to an existing layer.
- Avoid creating a second mission system, second serial hub, second diagnostic channel, or second “almost the same” input path.
- If a new feature seems to need its own I/O stack, treat that as a design smell and justify it explicitly before proceeding.

### 2.3 Layer by responsibility

**Recommended engineering guideline:** Keep responsibilities separated so each layer has one job:

| Layer concern | Typical ownership |
|---------------|-------------------|
| Session-global state / hubs | GameInstance (or World) subsystems |
| Per-actor binding & local behavior | Components on pawns / weapons / actors |
| Content definitions | Primary Data Assets + Content Blueprints |
| Presentation / FX / level-specific response | Blueprint, widgets, interfaces |
| Cross-system reactions | Delegates / events, not hardwired presentation inside cores |

Low-level I/O, framing, routing, simulation rules, and replication writes belong in durable C++ cores. Side effects that vary by content (physics pull, VFX, audio, door opens) should hang off events when practical.

### 2.4 Pragmatic module boundaries

**Established:** The ArduinoCommunication plugin and game module are coupled in places (the plugin is not a fully isolated product in this checkout). Changes to shared game types can affect the plugin.

**Recommended engineering guideline:**

- Put reusable hardware/diag/communication concerns in the communication plugin when that matches existing ownership.
- Put gameplay simulation (hover, missions, AI, weapons) in the game module.
- Accept pragmatic coupling when it already exists; do not “purify” boundaries by large moves unless there is a concrete maintainability win and a migration plan.
- When touching shared types used across modules, treat the change as cross-cutting and verify both sides.

### 2.5 Optional integration over forced rewiring

**Recommended engineering guideline:** New systems should prefer optional wiring so existing GameModes, widgets, and pawns keep working. Provide a convenient base path (e.g. a helper GameMode) *and* a manual registration path when designers already have a custom setup.

---

## 3. Code quality expectations

### 3.1 Quality means fitness for a long-lived hardware-coupled product

Quality here is not cleverness. It is:

- Clear ownership of each concern
- Predictable behavior under hardware failure and reconnect
- Safe threading boundaries (background I/O never owns game-thread events)
- Readable editor-facing surfaces (categories, names, documented usage)
- Resistance to silent breakage of Content and firmware contracts

### 3.2 Before you change anything

**Established process:**

1. Inspect the existing implementation.
2. Understand how the affected system currently works.
3. Reuse established project patterns where appropriate.
4. Avoid creating parallel systems that duplicate existing functionality.

Do not assume a verbal description of the system is complete. Prefer evidence from source, assets, config, and editor tooling (including Unreal MCP when available).

### 3.3 Technical debt

**Established:** When a proposed approach introduces unnecessary technical debt, explain the concern and recommend a better approach.

**Recommended engineering guideline:** Prefer the slightly longer path that reuses the hub, subsystem, or data-asset pattern over a local hack that will force a second system later. Document intentional trade-offs briefly when shipping a pragmatic compromise.

### 3.4 Completeness for C++ work

**Established (project compile guidance):** After modifying C++, compile and fix errors before considering the task finished. A change that does not build is not done.

### 3.5 Naming and editor surface

**Recommended engineering guideline (from observed conventions):**

- Follow Unreal naming (`U`/`A`/`F`/`E`, `*Component`, `*Subsystem`, `*DataAsset`).
- Use Content prefixes consistently (`BP_`, `BPC_`, `BPI_`, `WBP_`, `DA_`, `BT_`, `BB_`, etc.).
- Expose properties under clear domain categories (pipe-separated categories such as hardware, hover, mission, diagnostics).
- Prefer stable IDs (`FName` ship IDs, mission IDs, diagnostic node IDs) over ad-hoc strings that drift between firmware and Unreal.

---

## 4. How new systems should be added

Use this mental checklist:

1. **Is there already a home?** Prefer extending an existing subsystem, component family, protocol, or data-asset pipeline.
2. **Choose the right shape:**
   - Session-wide / cross-level → subsystem
   - Bound to an actor or station → component
   - Designer-authored variation → data asset (+ optional Blueprint content)
   - Cross-feature reaction → delegate / interface event
3. **Keep the core in C++ when the logic is reusable architecture.** Expose Blueprint-callable / assignable surfaces for configuration and presentation.
4. **Wire through existing infrastructure** (serial hubs, mission/threat events, AI director hooks, etc.) instead of opening a private channel.
5. **Prefer optional integration** so legacy Content keeps working.
6. **Document for the next person** when the system is designer-facing or agent-facing (guide, header usage notes, or architecture update as appropriate).
7. **Verify:** compile for C++; for significant asset/MCP edits, inspect before and after and preserve existing connections.

**Recommended engineering guideline:** If a feature can be expressed as data (missions, enemy definitions, weapon mag mappings), put variation in data — not in a growing forest of one-off subclasses — unless behavior truly needs a strategy class or mode.

---

## 5. Diagnostics philosophy

Diagnostics and fault reporting are part of the product identity, not a debug-only afterthought.

### 5.1 Established expectations

- Do not strip diagnostic logging, hardware support, or diagnostic interfaces without a clear, justified reason.
- Verbose / bring-up diagnostics should remain configurable (INI and editor-facing toggles are an established pattern).

### 5.2 Recommended engineering guidelines

- **Operational, not decorative:** Diagnostics should help operators and engineers answer “what is missing, offline, or misrouted?” during real sessions.
- **Shared buses, filtered consumers:** Multiple systems may share a serial/line stream. Consumers should filter what they own rather than assuming exclusive ownership of the channel.
- **Expected inventory vs observed reality:** Prefer roll-call / registry thinking (what *should* be present vs what reported) so absences show as offline/faults in UI.
- **UI compatibility:** Prefer dynamic diagnostic UI that can grow with the node list, while keeping older widget bindings working when feasible.
- **Never “clean up” diagnostics to silence noise by deleting the path.** Tune verbosity, categories, and filters instead.

---

## 6. Blueprint vs C++

### 6.1 Established split

- Prefer **C++** for reusable core systems and architecture when appropriate.
- Use **Blueprint** for editor-facing configuration, presentation logic, asset-specific behavior, and rapid iteration.
- Do **not** move working Blueprint functionality into C++ solely for stylistic reasons.

### 6.2 Recommended engineering guidelines

| Prefer C++ when… | Prefer Blueprint when… |
|------------------|------------------------|
| Rules must be reused across many assets | Behavior is specific to one pawn, level, or widget |
| Parsing, framing, replication writes, simulation cores | Presentation, FX, audio, layout, content tuning |
| Stable APIs other systems depend on | Fast iteration on designer-facing flow |
| Threading / authority / hardware marshaling | Assembling existing components into a ship or station |

Additional guidance:

- **Interfaces in Content** are a valid way for world objects to respond to C++-raised events (activatable, beamable, scannable-style contracts).
- **C++ detects and broadcasts; Blueprint (or other listeners) implement side effects** when the side effect is presentation or content-specific.
- When editing assets via MCP or editor automation: inspect first, preserve connections, make targeted changes, verify after significant edits.
- Mirroring enums / structs in Content for designer convenience is acceptable when it supports existing workflows; keep names and meanings aligned with C++ sources of truth.

---

## 7. Extensibility

Extensibility in this project means **adding capability without forking the architecture**.

**Recommended engineering guidelines:**

1. **Data-driven variation** — Primary Data Assets and registries for session content (missions, enemies, similar definition-driven features).
2. **Strategy / mode objects** — Abstract modes or config structs when behavior families diverge (movement modes, firing modes), instead of giant switch blocks that cannot be extended cleanly.
3. **Delegates as the integration bus** — Cores should expose events that other systems can bind; avoid pulling presentation and physics into every core.
4. **Stable IDs and aliases** — Prefer canonical IDs with alias mapping over breaking firmware or Content naming.
5. **Config-first knobs** — Expose tuning and feature flags through properties/INI where operators need them without a rebuild.
6. **Server authority for hardware-driven gameplay paths** — Default to authority-safe designs for inputs and outbound hardware commands that affect shared simulation state, while still exposing Blueprint events for local presentation.

Gameplay design note (missions): the documented fail-forward philosophy — failure escalates and unlocks rather than hard-stopping the session — is a product principle. New session systems should not casually introduce “fail = end everything” unless that is an explicit design choice.

---

## 8. Debugging philosophy

**Established:**

- Diagnose the underlying cause before changing code.
- Avoid speculative fixes when project evidence can be inspected.
- Do not assume the reported symptom fully describes the system.

**Recommended engineering guidelines:**

1. **Reproduce with evidence** — logs, diagnostic UI, serial traffic, replication/authority context, and asset wiring.
2. **Separate layers when isolating faults** — hardware link vs framing vs ShipId routing vs gameplay consumer vs presentation.
3. **Prefer the smallest fix that addresses the root cause** — especially near hardware and diagnostics, where “drive-by cleanups” break bring-up workflows.
4. **Preserve observability** — fixes should not remove the signals that would catch the next regression.
5. **Explain significant changes** — what changed and why, especially when behavior or compatibility paths shift.

If a fix would require a destructive architectural change and the right approach is unclear: **ask before proceeding** (established working style).

---

## 9. AI coding expectations

AI assistants are participants in this engineering culture, not a separate process.

**Established expectations for agents:**

- Follow `.cursor/rules/00-project-core.mdc`.
- Inspect the project when answers are available from source, assets, config, or Unreal MCP.
- Prefer evidence over assumptions when older docs conflict with the codebase (architecture doc and `.uproject` win over stale README/CLAUDE claims such as engine version).
- Preserve diagnostics, hardware support, compatibility behavior, and existing interfaces.
- Make targeted Blueprint/asset edits; do not rebuild assets unnecessarily.
- Compile after C++ changes; do not declare C++ work complete while it fails to build.
- Briefly explain significant changes; ask before potentially destructive architectural moves.
- Call out unnecessary technical debt and recommend a better approach.

**Recommended engineering guidelines for agents:**

- Treat [PROJECT_ARCHITECTURE.md](./PROJECT_ARCHITECTURE.md) as the current structural map; update philosophy/architecture docs when intentional policy changes, not after every feature.
- Prefer extending Andy/serial, mission, and AI director patterns over inventing parallel stacks.
- Do not “helpfully” delete legacy APIs, dual widget bindings, or older protocol paths without an explicit request.
- When inference is required in documentation or design proposals, label it as recommendation rather than presenting it as an existing project fact.

---

## 10. Backwards compatibility

Compatibility is a feature of a hardware-coupled simulator with long-lived Content and firmware.

**Established:** Do not remove compatibility behavior or existing interfaces unless explicitly requested or clearly necessary.

**Recommended engineering guidelines:**

1. **Prefer coexistence** — keep legacy APIs or protocol paths working alongside newer ones when consumers still exist.
2. **Prefer dual UI bindings** — support newer dynamic layouts without forcing immediate rebuilds of older widgets.
3. **Prefer aliases and adapters** — normalize IDs and names at the boundary rather than forcing firmware/Content renames.
4. **Deprecate loudly, remove rarely** — if something must go, require an explicit decision, a migration note, and verification of Content/hardware impact.
5. **Do not break ShipId / mission ID / diagnostic node contracts casually** — these are cross-system contracts spanning Unreal, Content, and devices.

---

## 11. Maintainability

Maintainability is the ability for the next engineer (or agent) to change one concern without accidentally rewriting three others.

**Recommended engineering guidelines:**

- **One clear owner per concern** — hub owns ports; component owns ship-scoped binding; data asset owns definition; widget owns presentation.
- **Small, intentional surfaces** — public Blueprint APIs and delegates should be stable and purposeful; avoid dumping every internal knob into the global namespace.
- **Header / guide usage notes for non-obvious systems** — especially hardware and mission wiring.
- **Thread and authority discipline** — background threads feed queues; game thread owns Unreal events; server authority owns shared simulation writes on hardware-driven paths.
- **Platform honesty** — stub or conditionally compile unsupported platforms rather than pretending hardware APIs are universal.
- **Documentation hygiene** — when policy changes, update `.cursor` docs; when only implementation details change, prefer architecture docs over rewriting philosophy.
- **Resist drive-by refactors** — unrelated renames, “cleanup” deletions, and stylistic Blueprint→C++ moves increase risk without improving the product.

---

## 12. Decision heuristics (quick reference)

When stuck, use these defaults:

| Question | Default answer |
|----------|----------------|
| Rewrite or extend? | Extend |
| New I/O stack? | No — use existing hubs |
| C++ or Blueprint for reusable rules? | C++ |
| C++ or Blueprint for presentation / one-off content? | Blueprint |
| Move working BP to C++ for style? | No |
| Remove a legacy path? | No, unless explicitly required |
| Speculative fix without evidence? | No — inspect first |
| Uncertain destructive architecture change? | Ask |
| Diagnostics feel noisy? | Tune filters/verbosity — don’t delete the system |
| Where does session-global state live? | Subsystem |
| Where does per-ship behavior live? | Component |
| How should systems react to each other? | Delegates / interfaces |

---

## 13. What this philosophy is not

- It is **not** a license to freeze the project. Change is expected; unjustified breakage is not.
- It is **not** a demand that everything be C++ or everything be Blueprint.
- It is **not** a claim that module boundaries are perfectly clean today.
- It is **not** a substitute for reading the architecture map and the code when implementing a change.

---

## Related documents

| Document | Role |
|----------|------|
| `.cursor/rules/00-project-core.mdc` | Mandatory priorities for agents and engineers |
| `.cursor/docs/PROJECT_ARCHITECTURE.md` | Verified structural map of the current project |
| `MISSION_SYSTEM_GUIDE.md` | Designer-facing mission authoring (includes fail-forward product intent) |
| `Plugins/ArduinoCommunication/README.md` | Communication plugin API and sketches |
| Root `README.md` / `CLAUDE.md` | Older overviews; verify against architecture before trusting versioning or scope claims |
