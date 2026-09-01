# Unduinocpp — Architectural Decisions

This document explains **why** the project is structured the way it is.

It is **not** a class inventory or code walkthrough. For what exists today, see [PROJECT_ARCHITECTURE.md](./PROJECT_ARCHITECTURE.md). For how engineers should decide going forward, see [ENGINEERING_PHILOSOPHY.md](./ENGINEERING_PHILOSOPHY.md).

---

## How to read this document

Each entry uses this shape:

- **Decision** — what was chosen
- **Reason** — why, when known
- **Alternatives rejected** — only when they can be verified or reasonably inferred
- **Tradeoffs** — costs of the choice
- **Consequences for future engineers** — what to preserve or avoid

### Fact vs inferred rationale

| Label | Meaning |
|-------|---------|
| **Fact** | Stated in headers, guides, rules, README usage notes, or clearly implemented in code |
| **Inferred** | Reasonable explanation from structure and comments; not an authored design memo |

Do not treat inferred rationale as binding product requirements. Prefer extending verified patterns over inventing new stacks.

---

## 1. Product identity: physical bridge simulator, not a game with optional hardware

### Decision

Treat Unreal gameplay, physical controls/displays, ESP32 hardware, serial/ESP-NOW paths, and diagnostics as **one product**.

### Reason

**Fact:** Project identity in `.cursor/rules/00-project-core.mdc` and architecture docs describes a physical spaceship bridge simulator combining software and hardware.

**Inferred:** Hardware bring-up, fault reporting, and multi-station I/O are first-class because sessions depend on physical devices working, not only on virtual input.

### Alternatives rejected (inferred)

- Pure software game with hardware as a late bolt-on
- Hardware-only demo with Unreal as a thin visualizer

### Tradeoffs

- Cross-cutting contracts (ShipId, node IDs, protocols) span firmware, Content, and C++
- “Cleanup” that removes diagnostics or legacy paths can break real sessions

### Consequences for future engineers

Preserve hardware support, diagnostic logging, and compatibility behavior unless explicitly asked to remove them. Prefer extending Andy/serial patterns over opening parallel I/O stacks.

---

## 2. Andy as the central hardware hub

### Decision

Each ship talks to Unreal through an **Andy** ESP32 hub over USB serial. Peripheral ESP nodes (weapons, engines, MiniCRT displays, etc.) sit behind Andy.

### Reason

**Fact:** `UAndySerialSubsystem` is documented as managing one serial connection per ship/Andy. `UShipHardwareInputComponent` describes itself as the interface between an ESP32 “Andy” hub and ship gameplay. MiniCRT comments state Andy receives `!crt` commands and forwards them via ESP-NOW.

**Inferred:** A single USB uplink per ship simplifies PC cabling, COM-port management, and Unreal ownership of one connection, while still allowing many physical stations on the bridge.

### Alternatives rejected (inferred)

- Every ESP32 opens its own USB serial (or TCP) session to Unreal
- Unreal speaks ESP-NOW or WiFi mesh directly to each node
- One global shared serial for all ships with no ShipId routing

### Tradeoffs

- Andy is a single point of failure for that ship’s hardware path
- Unreal cannot see ESP-NOW internals; it only sees what Andy serializes
- Firmware and Unreal must agree on hub commands (`!diag`, `!crt`, binary frames)

### Consequences for future engineers

Do not open a second serial port from gameplay components for the same ship. Route outbound commands and inbound telemetry through `UAndySerialSubsystem` using the ship’s `ShipId`. Extend Andy’s command/frame surface rather than inventing a peer link to leaf nodes.

---

## 3. ESP-NOW on the hardware side; Unreal stays on serial/TCP

### Decision

**ESP-NOW is a hardware-side transport** between Andy and leaf ESP32s. Unreal’s primary production path to the ship is **USB serial** (with a generic Serial/TCP component still available for simpler Arduino use cases).

### Reason

**Fact:** Architecture docs and MiniCRT headers state ESP-NOW forwarding happens on Andy; Unreal talks serial (and the generic plugin also supports TCP). Complete ESP-NOW protocol docs are **not** present in Unreal source.

**Inferred:** ESP-NOW fits dense local bridge hardware (low latency, no WiFi infrastructure dependency between nodes). Serial to the PC is reliable for bring-up, Windows COM tooling, and a single authoritative uplink. Keeping ESP-NOW out of Unreal avoids embedding Espressif wireless stacks in the game process.

### Alternatives rejected (inferred)

- Unreal ↔ each node over WiFi/TCP
- Unreal ↔ mesh with Unreal as an ESP-NOW peer
- All traffic as line-oriented text over WiFi only

### Tradeoffs

- Debugging leaf-node wireless issues requires firmware tools, not only Unreal logs
- PC path and mesh path can fail independently; diagnostics must cover both
- Generic TCP sketches in the plugin can mislead newcomers into thinking WiFi is the ship architecture

### Consequences for future engineers

Do not assume Unreal should “speak ESP-NOW.” Document and implement leaf behavior on the firmware/Andy side. When adding a new physical display or controller, prefer “Andy forwards” over “new COM port in Unreal.”

---

## 4. Dual wire protocols on one UART: binary frames + ASCII lines

### Decision

The Andy serial link carries **both**:

1. **Binary framed telemetry** (`0xAA` … `0x55`, CRC, typed payloads such as IMU / tags / wheel)
2. **ASCII line commands and diagnostics** (`!diag,quick`, `DIAG_*` lines, `!crt,...`)

### Reason

**Fact:** `UByteStreamPacketParser` defines the binary frame. `UAndyDiagSubsystem` / MiniCRT use ASCII lines. `ArduinoSerialPort` comments state binary NetMsg frames share the UART with ASCII diag/`!cmd` lines. Line consumers are told to filter (e.g. only `DIAG_`).

**Inferred:** High-rate sensor traffic wants compact binary with CRC and sequence numbers. Operator/debug/command surfaces want human-readable lines that are easy to log and type. Multiplexing both on one hub link avoids a second cable.

### Alternatives rejected (inferred)

- All-text protocol for IMU-rate data
- All-binary including diagnostics (harder for operators)
- Separate COM ports for telemetry vs commands

### Tradeoffs

- Shared-bus complexity: consumers must filter; log formatters must not corrupt binary
- Framing bugs can look like “diag flakiness” and vice versa
- Two mental models for one cable

### Consequences for future engineers

Never assume exclusive ownership of the Andy UART. Filter by frame type or line prefix. Prefer adding a new `EEspMsgType` or a new `!command` that Andy understands — not a second port “to keep it simple.”

---

## 5. Centralized multi-port serial ownership (`UAndySerialSubsystem`)

### Decision

Serial ports are owned by a **GameInstance subsystem** keyed by `ShipId`, not by ephemeral actor components that open their own ports.

### Reason

**Fact:** Header docs state the subsystem persists across level loads, manages multiple ports (one per ship/Andy), parses frames, and broadcasts `OnFrameParsed` / `OnConnectionChanged` / `OnLineReceived`.

**Inferred:** Bridge sessions may change maps or recreate pawns; hardware connections should outlive that. Multi-ship setups need a registry of COM ports without each pawn fighting over the same resource.

### Alternatives rejected (inferred)

- Each ship pawn owns and opens its COM port in BeginPlay
- A singleton actor in the level for serial (destroyed on map change)
- One hardcoded global COM3 with no ShipId

### Tradeoffs

- Port registration must be orchestrated (GameMode / persistent manager)
- Components depend on correct `ShipId` matching `AddPort`
- Misconfigured ShipId looks like “hardware dead” when the port is fine

### Consequences for future engineers

Register ports once through the subsystem; bind consumers by `ShipId`. New outbound features (like MiniCRT) should call `SendLine`/`SendBytes` on the existing subsystem, not construct `UArduinoSerialPort` locally.

---

## 6. Ship-scoped hardware binding via component, not raw subsystem fans

### Decision

Gameplay consumes hardware through **`UShipHardwareInputComponent`**: filter by `ShipId`, decode typed events, optionally auto-apply mags/IMU to firing components.

### Reason

**Fact:** Component docs describe ShipId filtering, Blueprint-friendly events, weapon-mag application, Port/Starboard IMU routing, and server-only defaults. Primary-handler logic avoids duplicate binding when both PlayerController and pawn have the component.

**Inferred:** Subsystem broadcasts are ship-agnostic; ships need a local, editor-friendly binding surface. Auto-apply paths reduce Blueprint boilerplate for the common dual-gun layout while still exposing events for custom presentation.

### Alternatives rejected (inferred)

- Every Blueprint binds directly to `OnFrameParsed` and reimplements parsing
- Hardcoding Port/Starboard routing only in one ship Blueprint with no reusable component

### Tradeoffs

- Plugin component depends on game types (`UFiringComponent`) — module coupling
- Auto-routing heuristics (Src fallbacks, primary handler) must stay documented
- Two components with the same ShipId can still confuse bring-up if primary selection is misunderstood

### Consequences for future engineers

Extend this component (or clearly related helpers) for new inbound hardware message types. Do not fork a second “ship input” stack. Preserve Port/Starboard and mag/IMU contracts when changing firing APIs.

---

## 7. Server authority for hardware-driven simulation paths

### Decision

Hardware input and outbound MiniCRT commands default to **server-only** (`bServerOnly = true`) with authority checks.

### Reason

**Fact:** Both `UShipHardwareInputComponent` and `UMiniCRTWeaponDisplayComponent` expose `bServerOnly` defaulting to true. Mission manager is documented as server-authoritative for state changes. Architecture summary notes server-only hardware paths.

**Inferred:** Physical bridge input is inherently authoritative for shared simulation (steering, fire, mags). Clients should not independently apply the same serial stream to replicated state. Presentation can still listen to Blueprint events where appropriate.

### Alternatives rejected (inferred)

- Every client opens serial and simulates locally
- Fully client-authoritative hardware with late reconciliation

### Tradeoffs

- Dedicated-server / listen-server topologies must ensure the machine with the COM port is the authority (or relay carefully)
- Local PIE clients may appear “dead” for hardware if they lack authority — by design when `bServerOnly` is on

### Consequences for future engineers

Keep authority defaults for anything that mutates shared gameplay from hardware. If a feature needs local-only preview, make that opt-in and do not silently write replicated state from clients.

---

## 8. Centralized diagnostics with an expected roll call

### Decision

Diagnostics are centralized in **`UAndyDiagSubsystem`** (+ registry/widgets): Unreal sends `!diag,quick`, Andy returns `DIAG_*` lines, UI merges results with a **default 10-node roll call** so missing nodes show as offline.

### Reason

**Fact:** Diag subsystem and registry implement quick-diag request/accumulation, ordered roll call, canonical IDs (`ANDY`, `PORT`, `WPN_STBD`, engine corners, `DISP_*`), and aliases. Project rules treat diagnostics as first-class. Philosophy docs emphasize expected inventory vs observed reality.

**Inferred:** Operators need “what should be online” during live sessions, not only “what happened to reply.” Centralizing parsing avoids each widget reinventing `DIAG_` line handling.

### Alternatives rejected (inferred)

- Per-node Unreal polling over separate links
- Diagnostics only as ad-hoc `UE_LOG` with no operator UI
- UI that lists only nodes that responded (hiding absences)

### Tradeoffs

- Roll-call list can drift from physical hardware if firmware IDs change without updating aliases
- Shared UART means diag traffic coexists with binary telemetry
- Legacy two-node widget layouts still exist alongside fuller roll-call UI

### Consequences for future engineers

Add new expected nodes through the registry/alias path. Do not delete diagnostic interfaces to reduce log noise — filter verbosity instead. Keep canonical IDs stable; prefer aliases over renaming firmware and Content simultaneously.

---

## 9. MiniCRT displays as separate ESP nodes, driven by Unreal commands through Andy

### Decision

MiniCRT weapon displays are **separate nodes** (`DISP_PORT` / `DISP_STBD` in roll call). Unreal does **not** open a CRT serial port; `UMiniCRTWeaponDisplayComponent` emits `!crt,{Side},...` on the existing Andy connection. Andy forwards via ESP-NOW.

### Reason

**Fact:** MiniCRT header states it never opens its own serial port; Andy forwards via ESP-NOW; dual CRT uses Side `0`/`1`; sends are change-driven, debounced, not Tick-based. Displays appear in the default diagnostic roll call.

### Alternatives rejected (inferred)

- Driving CRTs from Unreal as additional COM devices
- Embedding ammo UI only in Unreal screen UI (no physical CRT)
- Streaming full framebuffer from Unreal to CRTs over serial

### Tradeoffs

- Display correctness depends on Andy forwarding and Side routing
- Unreal state and CRT state can desync if sends are dropped (mitigated by change + debounce design, not a full ack protocol in Unreal)
- Game module depends on Andy serial subsystem patterns

### Consequences for future engineers

Add physical secondary displays the same way: command through Andy, identify by Side/node ID, register in diagnostics. Do not attach a new `UArduinoSerialPort` to the weapon actor for CRT traffic.

---

## 10. Generic Arduino component retained alongside the Andy ship path

### Decision

Keep `UArduinoCommunicationComponent` (Serial **or** TCP, line/byte events, sketch-oriented text commands) **and** the Andy multi-ship subsystem path.

### Reason

**Fact:** Both exist in the plugin. README documents generic `PING`/`LED_ON` sketches separately from Andy binary frames. Project philosophy prefers coexistence of legacy paths.

**Inferred:** The generic component supports prototyping, teaching, and non-ship devices. The Andy path is the production bridge architecture. Removing the generic path would break simpler demos without helping the ship.

### Alternatives rejected (inferred)

- Delete generic Serial/TCP API once Andy existed
- Force all sketches to speak Andy binary frames

### Tradeoffs

- Two onboarding stories; docs can contradict if readers mix them
- Engineers may wire `BPC_ESP32Comm`-style components where Andy subsystem was intended

### Consequences for future engineers

For ship bridge features, use Andy + ShipHardwareInput. Use the generic component for isolated device tests. Do not “unify” by deleting either path without an explicit migration plan.

---

## 11. Pragmatic plugin ↔ game module coupling

### Decision

`ArduinoCommunication` **publicly depends on `Unduinocpp`** and uses game types (e.g. firing mode / `UFiringComponent` via weapon mag and related paths). MiniCRT lives in the **game module** but sends through the plugin serial subsystem.

### Reason

**Fact:** `ArduinoCommunication.Build.cs` lists `Unduinocpp` as a public dependency and adds game source to private include paths. Architecture docs call out that the plugin is not fully isolated.

**Inferred:** Fast integration of RFID mags and IMU into real weapons was valued over clean marketplace-plugin boundaries. The checkout is a product repo, not a pure redistributable plugin.

### Alternatives rejected (inferred)

- Fully isolated communication plugin with only byte/string APIs
- All hardware gameplay code moved into the plugin (or all hardware code moved into the game module)

### Tradeoffs

- Plugin is harder to reuse in unrelated projects
- Firing API changes can break the plugin
- Boundary “purification” refactors are high risk

### Consequences for future engineers

Treat shared types as cross-cutting. Do not large-move modules for purity alone. When adding hardware↔gameplay glue, follow the existing ownership (plugin for I/O/diag hubs; game for simulation) and accept intentional coupling where it already exists.

---

## 12. Blueprint vs C++ split for a hardware-coupled product

### Decision

**C++** owns reusable cores (serial framing, subsystems, hover/firing simulation, mission manager, AI frameworks). **Blueprint** owns ship assembly, presentation, level-specific responses, designer content, and rapid iteration. Working Blueprint is not moved to C++ for style.

### Reason

**Fact:** Stated in project rules and engineering philosophy. Content shows heavy Blueprint use for ship/GameMode/AI assets alongside C++ components and BT nodes.

**Inferred:** Physical installation iteration needs designer-accessible surfaces; protocol and authority rules need compile-time structure and testability.

### Alternatives rejected (inferred)

- All gameplay in Blueprint
- Rewrite all Content logic into C++ for consistency

### Tradeoffs

- Behavior can be split across BP graphs and C++ (harder to search)
- Enum/struct mirrors between Content and C++ can drift

### Consequences for future engineers

Put new reusable rules in C++ with Blueprint hooks. Keep presentation and one-off station behavior in Blueprint. Preserve Content interfaces (`BPI_*`) as integration points for world reactions.

---

## 13. Data-driven missions with fail-forward escalation

### Decision

Missions are **Primary Data Assets** + registry, runtime in a **GameInstance subsystem**, replicated via Mission GameState/PlayerState. Failure/expiry can raise threat and unlock further content (**fail-forward**), rather than hard-ending the session by default.

### Reason

**Fact:** `MISSION_SYSTEM_GUIDE.md` states fail-forward intent. `UMissionManagerSubsystem` is server-authoritative and exposes threat/mission delegates. Optional `AMissionGameModeBase` wires the system; custom GameModes can register manually.

**Inferred:** Bridge sessions are long-lived experiences; soft failure keeps the room/session alive and feeds AI/world systems through threat.

### Alternatives rejected (inferred)

- Hardcoded mission flow in one GameMode Blueprint
- Fail = immediate session end as the default model
- Client-authoritative objective completion

### Tradeoffs

- Designers must author actions/visibility carefully or content stays Hidden
- Threat coupling means mission data affects AI pacing
- Requires correct GameState/PlayerState classes

### Consequences for future engineers

Extend via data assets and documented actions. Hook world systems through threat/mission delegates (as the AI director does). Do not introduce a second mission runtime. Do not casually change fail-forward into hard-fail without an explicit design choice.

---

## 14. Enemy AI: definition assets + director scaled by mission threat

### Decision

Enemies are data-defined (`UEnemyDefinition`), run Behavior Trees with C++ tasks/services, and a **WorldSubsystem director** scales aggression/cooldowns from mission threat events.

### Reason

**Fact:** AI director binds mission threat delegates; definitions carry BT/blackboard/movement/abilities. Architecture maps this explicitly.

**Inferred:** Session tension should rise with fail-forward threat without rewriting each enemy Blueprint. Data assets keep archetypes editable without subclass explosion.

### Alternatives rejected (inferred)

- One monolithic AI controller Blueprint per enemy type with duplicated logic
- AI difficulty completely decoupled from mission/threat state

### Tradeoffs

- Threat tuning affects all listeners — easy to make the room suddenly hard
- Blueprint BT assets and C++ nodes must stay aligned on blackboard keys

### Consequences for future engineers

Add enemy variation through definitions/modes/abilities. Scale session pressure through threat hooks rather than one-off timers in random actors. Reuse `FEnemyBlackboardKeys` / shared enums.

---

## 15. Vehicle and weapons designed for analog/physical controls

### Decision

Hover movement accepts gradual analog inputs (pedals/wheels) as well as digital keys. Weapons accept IMU orientation and RFID mag tags from hardware, with firing modes (bullet / tractor / scanner / custom) configured in C++ and applied from mag data.

### Reason

**Fact:** `UHoverMovementComponent` documents pedals/wheels/triggers. Ship hardware events include wheel turn, weapon IMU, weapon/reload tags. `UFiringComponent` supports IMU apply and multiple modes; tractor mode is trace + delegates (no physics pull in C++).

**Inferred:** The simulation layer is shaped around physical bridge controls first; keyboard is a development fallback. Side effects like tractor pull stay in Blueprint/content so hardware-driven cores stay reusable.

### Alternatives rejected (inferred)

- Digital-only movement model
- Baking tractor/scanner presentation into C++ with no delegate seam

### Tradeoffs

- More configuration (smoothing, dual firing components, mag tables)
- Hardware calibration (IMU offsets) becomes part of the product surface

### Consequences for future engineers

Keep input APIs analog-friendly. When adding weapon modes, prefer config + delegates for content-specific effects. Preserve mag tag → firing apply as a data mapping, not hard-coded tag IDs in scattered Blueprints.

---

## 15b. Dual identical USB joysticks use winmm Joy IDs (not Gamepad/Enhanced Input)

### Decision

Tank-style dual Logitech Attack 3 control reads Windows joystick indices (`joyGetPosEx` Joy0/Joy1) in `UDualJoystickTankInputComponent`, then feeds existing `UHoverMovementComponent` analog setters. It does **not** map the sticks through Unreal Gamepad_* / Enhanced Input device axes.

### Reason

**Fact:** Two identical Attack 3 units share VID/PID `046D:C214`. Windows exposes them as separate DirectInput joystick indices. Unreal’s high-level gamepad mapping does not reliably present two identical DirectInput devices as independently addressable Gamepad axes.

**Fact:** `UHoverMovementComponent` already exposes `SetThrottleInput` / `SetSteeringInput` / `SetStrafeInput`.

### Alternatives rejected

- Enhanced Input per-device mapping for identical Attack 3s (unreliable device distinction)
- A second hover movement system for sticks
- Permanent Gamepad axis remaps in `DefaultInput.ini`

### Tradeoffs

- Windows-only path for this component
- Left/right assignment depends on USB enumeration order (mitigated with `bSwapLeftRightDevices` + Joy ID properties)
- New reflected class requires a full editor module rebuild (Live Coding alone is not enough)

### Consequences for future engineers

Prefer extending `UDualJoystickTankInputComponent` tuning/mix for stick experiments. Keep keyboard/`BPI_ESPComm` paths intact; only write movement while sticks are active.

---

## 16. Multi-monitor bridge views via OS windows (scene capture)

### Decision

Additional bridge monitors use **`UMultiDisplayCameraComponent`**: scene capture to a render target displayed in a separate OS window on a chosen display index (best as Standalone Game).

### Reason

**Fact:** Component setup guide describes one actor/component per monitor, target display index, fullscreen window, delay before open. Lives in the communication plugin alongside other installation concerns.

**Inferred:** A physical bridge needs multiple camera feeds without building a full custom multiplayer spectator stack. OS windows map cleanly onto real monitors.

### Alternatives rejected (inferred)

- Single view only
- Relying solely on Unreal’s default multi-viewport PIE workflows for installation

### Tradeoffs

- Extra GPU cost per capture
- PIE vs Standalone behavioral differences
- Window management is OS-specific

### Consequences for future engineers

Add bridge screens by placing configured capture actors; tune resolution before inventing another display path. Distinguish this (Unreal-rendered monitors) from MiniCRT (ESP display nodes).

---

## 17. External minimap over simple UDP, separate from Andy

### Decision

External minimap consumers (e.g. Raspberry Pi) receive plain-text `"X,Y,Yaw"` over **UDP** from `UMinimapUDPSenderComponent`, independent of Andy serial.

### Reason

**Fact:** Component header documents UDP plaintext, LAN IP/port, optional auto-send interval. Architecture lists this separately from serial/TCP hardware and UE replication.

**Inferred:** Lossy, low-stakes pose streaming fits UDP. Keeping it out of Andy avoids overloading the ship hub protocol with display-only traffic for a different device class.

### Alternatives rejected (inferred)

- Tunneling minimap pose through Andy/`!commands`
- TCP with reconnect complexity for high-rate pose

### Tradeoffs

- No delivery guarantee; consumers must tolerate gaps
- Another network path to configure on the LAN
- Not a general-purpose networking layer

### Consequences for future engineers

Use UDP-style side channels for simple external visualizations. Do not fold unrelated device traffic into Andy unless the device is part of the ESP node fabric.

---

## 18. Unreal multiplayer networking is selective, not a custom netcode stack

### Decision

Use **Unreal replication** for mission/threat and selected gameplay state; keep **hardware I/O as a separate authority-bound path**; use **UDP/serial/TCP** for external devices and tooling (minimap, Arduino, editor MCP). No project-wide custom netcode framework.

### Reason

**Fact:** Architecture “Networking summary” lists replication markers, server-only hardware paths, UDP minimap, serial/TCP hardware, editor MCP TCP — and explicitly non-claims a full listen/dedicated product architecture beyond those pieces.

**Inferred:** The hard problem is physical I/O + session systems; inventing a parallel netcode layer would not help the bridge. Authority defaults protect shared state when hardware is present.

### Alternatives rejected (inferred)

- Custom lockstep or fully homemade replication
- Replicating raw serial bytes to all clients as the primary design

### Tradeoffs

- Multi-machine setups need clear answers for “which process owns the COM port?”
- Not every class is multiplayer-ready; do not assume it is

### Consequences for future engineers

Replicate simulation results, not raw hardware streams, unless there is a strong reason. When adding networked features, follow existing GameState/PlayerState/component replication patterns and keep hardware writes authoritative.

---

## 19. Optional integration for session systems

### Decision

Provide convenient bases (e.g. `AMissionGameModeBase`) **and** manual registration paths so existing GameModes/Content keep working.

### Reason

**Fact:** Mission guide documents Option A (base GameMode) and Option B (wire subsystem manually). Philosophy calls this optional integration.

**Inferred:** The project already has ship-focused GameModes (`GameMode_ShipPorts`); forcing a single base class would strand Content.

### Alternatives rejected (inferred)

- Mandatory inheritance from one uber-GameMode for all features

### Tradeoffs

- Easy to forget manual wiring (missions simply never start)
- More documentation burden

### Consequences for future engineers

New session-wide systems should offer a helper path and a manual path. Fail loudly or log clearly when registry/setup is missing.

---

## 20. Stable cross-system IDs with aliases

### Decision

Use stable `FName` contracts — `ShipId`, mission IDs, diagnostic node IDs — and **alias maps** where firmware/Content naming historically diverged.

### Reason

**Fact:** Andy diag registry canonicalizes aliases (`WEAPON_PORT` → `PORT`, etc.). Mission guide stresses stable mission IDs. Philosophy treats these IDs as cross-system contracts.

**Inferred:** Renaming across firmware + Unreal + widgets at once is operationally expensive; aliases absorb drift.

### Alternatives rejected (inferred)

- Free-form strings with no canonicalization
- Forced one-time rename of all firmware node names

### Tradeoffs

- Alias tables must be maintained
- Duplicate conceptual names can confuse newcomers

### Consequences for future engineers

Prefer adding an alias over breaking a firmware string. Never casually rename ShipId/mission/diag contracts without a migration plan across devices and Content.

---

## Decision index (quick scan)

| # | Decision | Primary locus |
|---|----------|----------------|
| 1 | Hardware + game are one product | Project rules / identity |
| 2 | Andy is the per-ship hub | `UAndySerialSubsystem` |
| 3 | ESP-NOW stays on hardware; Unreal uses serial/TCP | MiniCRT / architecture notes |
| 4 | Binary + ASCII on one UART | Parser + diag + serial port |
| 5 | GameInstance owns ports by ShipId | `UAndySerialSubsystem` |
| 6 | Ship component binds/filter/decodes | `UShipHardwareInputComponent` |
| 7 | Server-authoritative hardware gameplay | `bServerOnly` defaults |
| 8 | Central diag + expected roll call | `UAndyDiagSubsystem` / registry |
| 9 | MiniCRT via Andy `!crt`, separate nodes | `UMiniCRTWeaponDisplayComponent` |
| 10 | Keep generic Arduino API + Andy path | Plugin dual APIs |
| 11 | Pragmatic plugin↔game coupling | Build.cs / shared types |
| 12 | C++ cores, Blueprint presentation/content | Project rules |
| 13 | Data-driven fail-forward missions | Mission guide / subsystem |
| 14 | Definition AI + threat director | AI module / director |
| 15 | Analog/hardware-first vehicle & weapons | Hover / firing / mags |
| 16 | Multi-monitor via OS capture windows | `UMultiDisplayCameraComponent` |
| 17 | External minimap via UDP side channel | `UMinimapUDPSenderComponent` |
| 18 | Selective UE replication, not custom netcode | Architecture networking summary |
| 19 | Optional session wiring | Mission GameMode options |
| 20 | Stable IDs + aliases | Diag registry / mission IDs |

---

## Related documents

| Document | Role |
|----------|------|
| [PROJECT_ARCHITECTURE.md](./PROJECT_ARCHITECTURE.md) | Verified structural map |
| [ENGINEERING_PHILOSOPHY.md](./ENGINEERING_PHILOSOPHY.md) | How to decide going forward |
| `.cursor/rules/00-project-core.mdc` | Mandatory engineering priorities |
| `MISSION_SYSTEM_GUIDE.md` | Mission authoring + fail-forward intent |
| `Plugins/ArduinoCommunication/README.md` | Generic plugin API + Andy subsystem usage |

When a future change intentionally reverses one of these decisions, update this file and call out **Fact** changes versus new **Inferred** rationale.
