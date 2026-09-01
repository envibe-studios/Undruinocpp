# Playbook: Debugging the Hardware Pipeline

This playbook should be followed whenever hardware is not behaving correctly.

Never begin by changing code.

Instead, identify exactly where communication stops.

The objective is to locate the first broken link in the chain.

---

# Philosophy

Treat every hardware system as a pipeline.

Do not guess.

Do not skip layers.

Verify each layer independently.

Only move to the next stage once the current stage has been verified.

---

# Typical Pipeline

Physical Hardware

↓

ESP32 Firmware

↓

ESP-NOW

↓

Andy

↓

Serial

↓

Unreal Parser

↓

Gameplay System

↓

Blueprint

↓

UI / World

---

# Step 1 — Define the Expected Behavior

Document:

What should happen?

What actually happens?

Does the problem happen:

- always
- intermittently
- only after boot
- only after reconnect
- only on one ship
- only in PIE
- only on physical hardware

Avoid debugging before clearly describing the symptom.

---

# Step 2 — Verify Physical Hardware

Check:

Power

Wiring

Connectors

Ground

Sensor orientation

Mechanical movement

Loose crimps

Damaged cables

Incorrect GPIO wiring

Never assume the hardware is correct simply because it worked previously.

---

# Step 3 — Verify Firmware

Confirm:

Firmware compiled successfully.

Correct firmware version flashed.

Correct Node ID.

Correct network version.

Correct initialization.

Expected setup completed.

Look for:

Serial logs

Boot messages

Initialization failures

Watchdog resets

Exceptions

---

# Step 4 — Verify ESP-NOW

Confirm:

Peer registration.

Packets transmitted.

Packets received.

Packet counts increase.

No version mismatch.

No CRC failures.

No duplicate IDs.

No unexpected packet flooding.

---

# Step 5 — Verify Andy

Andy is the authoritative bridge between hardware and Unreal.

Confirm:

Node appears during roll call.

Diagnostics report expected state.

Packets are received.

Packets are forwarded.

Expected routing occurs.

No parser failures.

No dropped packets.

Use diagnostic commands whenever available.

---

# Step 6 — Verify Serial

Confirm:

Expected serial messages exist.

Correct message type.

Correct formatting.

Correct ShipId.

Correct routing.

Correct update frequency.

If Unreal never receives data, inspect the serial stream before changing Unreal code.

---

# Step 7 — Verify Unreal Parser

Confirm:

Parser recognizes the message.

Correct message type.

Correct node ID.

Correct object updated.

Expected delegates/events fire.

Avoid changing gameplay code before verifying parser input.

---

# Step 8 — Verify Gameplay

Confirm:

Gameplay system receives updates.

State changes correctly.

Replication (if applicable).

Timers.

State machines.

Subsystem registration.

---

# Step 9 — Verify Blueprint

Inspect existing Blueprint.

Confirm:

Events fire.

Variables update.

Bindings exist.

No broken references.

Compile warnings.

Compile errors.

Avoid rebuilding Blueprint unless necessary.

---

# Step 10 — Verify Presentation

If gameplay works but UI does not:

Inspect:

Widget bindings

CRT displays

UMG

Actor Components

Materials

Animations

Do not continue debugging networking if the problem is presentation only.

---

# Regression Checks

After every fix verify:

Port weapon

Starboard weapon

MiniCRT

Andy diagnostics

Existing node IDs

Serial output

Multiple ships

PIE

Physical hardware

Do not assume unrelated systems remain working.

---

# Preferred Debugging Strategy

Prefer adding temporary diagnostics over guessing.

Prefer measurement over assumptions.

Prefer verifying one layer at a time.

Never "fix" several systems simultaneously.

One change.

One test.

Repeat.

---

# AI Expectations

Before modifying code:

Attempt to identify the failing stage.

Explain why that stage is suspected.

If evidence is insufficient:

Gather more evidence before implementing changes.

When the bug is fixed:

Explain the root cause.

Explain why the fix works.

Explain why alternative hypotheses were rejected.

Document architectural lessons if appropriate.