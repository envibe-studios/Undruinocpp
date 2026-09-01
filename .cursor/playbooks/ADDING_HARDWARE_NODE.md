# Playbook: Adding a New Hardware Node

Use this playbook whenever adding a new ESP32-based physical hardware node to the project.

The goal is to integrate the node without breaking existing hardware, diagnostics, routing, or Unreal behavior.

## 1. Inspect Existing Patterns First

Before changing code:

- Identify the most similar existing hardware node.
- Inspect its firmware, node ID, ESP-NOW registration, diagnostic behavior, and Unreal integration.
- Reuse established patterns where appropriate.
- Do not create a parallel protocol or duplicate parser unless necessary.

## 2. Define the Node

Determine:

- Node name
- Node ID
- Hardware purpose
- Inputs
- Outputs
- Expected diagnostic capabilities
- Whether it communicates directly with Andy
- Whether Unreal needs live data from it
- Whether Unreal needs to send commands to it

Do not reuse an existing node ID.

## 3. Update Shared Identifiers

Update the authoritative location for:

- Node IDs
- Message types
- Device names
- Any shared enums or protocol definitions

Avoid scattering the same numeric IDs across multiple files.

If an existing centralized definition exists, extend it rather than creating a new source of truth.

## 4. Implement ESP-NOW Communication

For the new ESP32 node:

- Configure the correct network version.
- Configure the node ID.
- Register required peers.
- Use the existing packet structure.
- Preserve existing CRC/version behavior.
- Preserve routing conventions.
- Avoid incompatible packet changes unless explicitly planned.

If the node sends data to Unreal, route it through Andy unless the existing architecture clearly specifies otherwise.

## 5. Integrate with Andy

Update Andy only where required.

Verify:

- peer registration
- incoming message handling
- outgoing routing
- serial forwarding
- command handling
- diagnostic roll-call expectations

Do not disrupt existing node routing.

Do not create a second hardware-to-Unreal transport path unless there is a documented architectural reason.

## 6. Add Diagnostics

Every new hardware node should participate in diagnostics where practical.

At minimum consider:

- online/offline state
- uptime
- boot status
- communication status
- sensor/input initialization
- hardware-specific failures

If the node is expected in quick diagnostics, update the expected roll call.

Diagnostic output should remain human-readable and consistent with existing formats.

Do not remove existing diagnostics to simplify implementation.

## 7. Integrate with Unreal

If Unreal consumes data from the node:

- extend existing parsers rather than duplicating them
- add new message handling using established conventions
- preserve ShipId routing
- expose data to Blueprint only where useful
- keep reusable core logic in C++ where appropriate

If Unreal sends commands to the node:

- use the existing serial command path
- preserve command formatting conventions
- route through Andy unless architecture specifies otherwise

## 8. Blueprint / Editor Integration

Use Unreal MCP where useful to inspect and modify live editor assets.

Before changing an existing Blueprint:

- inspect it first
- preserve existing connections
- make targeted edits
- compile after changes

If a new Blueprint asset is required:

- follow existing naming conventions
- place it in the appropriate Content folder
- use C++ parent classes where consistent with similar systems

## 9. Test the Node

Test in stages.

### Firmware
Verify:
- boot
- initialization
- ESP-NOW peer setup
- message send/receive
- hardware inputs/outputs

### Andy
Verify:
- node appears online
- diagnostics pass
- messages arrive
- messages are forwarded correctly

### Unreal
Verify:
- parser recognizes the node
- values update correctly
- Blueprint events fire if applicable
- no unrelated hardware paths regress

### Full Integration
Verify the real physical device through the complete chain:

Hardware Node → ESP-NOW → Andy → Serial → Unreal

and, if applicable:

Unreal → Serial → Andy → ESP-NOW → Hardware Node

## 10. Regression Check

Before considering the work complete, confirm that existing nodes still function.

Pay particular attention to:

- Port weapon
- Starboard weapon
- MiniCRT displays
- diagnostics
- serial parsing
- ShipId routing
- any shared enums or packet definitions changed during the work

## 11. Update Documentation

If the new node changes project architecture or documented hardware inventory, update the relevant files under:

`.cursor/docs/`

Typical candidates:

- PROJECT_ARCHITECTURE.md
- ARCHITECTURAL_DECISIONS.md

Do not update documents that were not actually affected.

## 12. Completion Report

When finished, summarize:

- files changed
- node ID assigned
- protocol/message changes
- Andy changes
- Unreal changes
- diagnostic changes
- tests performed
- any remaining manual hardware verification required