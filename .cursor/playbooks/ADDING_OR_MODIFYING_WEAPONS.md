# Playbook: Adding or Modifying Weapon Systems

Use this playbook whenever implementing or modifying weapon hardware, weapon gameplay, firing modes, magazines, aiming, ammunition, or weapon UI.

Weapons are one of the project's most interconnected systems.

Treat them as a complete pipeline rather than isolated code.

---

# Weapon Philosophy

Weapons are composed of independent systems.

Avoid creating one large monolithic weapon implementation.

Where possible separate:

- physical hardware
- communication
- weapon state
- aiming
- firing
- ammunition
- magazine handling
- effects
- UI
- diagnostics

Each layer should have a clear responsibility.

---

# Typical Weapon Pipeline

Player

↓

Physical Weapon

↓

IMU / Trigger / RFID

↓

ESP32

↓

ESP-NOW

↓

Andy

↓

Serial

↓

Unreal Parser

↓

Weapon Component

↓

Gameplay

↓

Effects

↓

MiniCRT

---

# Step 1 — Identify the Requested Change

Determine whether the change affects:

- aiming
- firing
- recoil
- spread
- ammunition
- magazine
- reload
- fire mode
- diagnostics
- MiniCRT
- hardware
- Unreal gameplay
- AI weapons

Do not modify unrelated weapon systems.

---

# Step 2 — Inspect Existing Weapons

Before creating new functionality:

Inspect:

Port weapon

Starboard weapon

Shared weapon code

Weapon Components

Existing Blueprint hierarchy

Existing weapon messages

MiniCRT integration

Prefer extending shared systems.

Avoid creating weapon-specific implementations unless required.

---

# Step 3 — Hardware

If hardware changes:

Verify:

IMU

Trigger

RFID

GPIO

Calibration

Initialization

Diagnostics

Do not bypass Andy.

Do not communicate directly with Unreal.

---

# Step 4 — Weapon Messages

If communication changes:

Reuse:

existing packet structures

existing message IDs

existing parser

existing routing

Avoid creating duplicate serial protocols.

---

# Step 5 — Weapon State

Clearly define state.

Examples:

Idle

Ready

TriggerHeld

Firing

Reloading

NoMagazine

OutOfAmmo

Disabled

Cooldown

Avoid scattered boolean combinations.

Prefer explicit state.

---

# Step 6 — Ammunition

Separate:

Current ammo

Magazine capacity

Reserve ammo

Magazine type

Reload state

Do not mix gameplay ammo with UI presentation.

---

# Step 7 — Fire Modes

Fire modes should represent behavior.

Examples:

Single

Burst

Auto

Charge

Beam

Safety

Avoid duplicating firing logic.

Fire mode should select behavior rather than replace the weapon implementation.

---

# Step 8 — MiniCRT

Whenever weapon state changes consider whether MiniCRT should update.

Examples:

Ammo

Reloading

Magazine inserted

Fire mode

Warnings

Offline

WAIT

No Magazine

MiniCRT should reflect gameplay state rather than compute gameplay state.

---

# Step 9 — Diagnostics

Weapon diagnostics should expose:

IMU

Trigger

RFID

Magazine

Communication

Initialization

Display communication

Boot

Online

Failures

Do not remove useful diagnostics.

---

# Step 10 — Unreal Gameplay

Weapon gameplay should own:

damage

hit detection

spread

cooldown

effects

AI interaction

Hardware should provide input.

Gameplay should own outcomes.

---

# Step 11 — Blueprint

Blueprint should primarily configure:

Meshes

Effects

Animations

Audio

Presentation

Avoid placing reusable firing logic exclusively inside Blueprint.

---

# Step 12 — AI Compatibility

Whenever changing player weapons ask:

Can AI use this system?

If not,

can the core weapon behavior be shared?

Avoid creating player-only weapon logic unless required.

---

# Step 13 — Testing

Verify:

IMU movement

Trigger

Magazine detection

Reload

Ammo count

Fire mode

Damage

MiniCRT

Diagnostics

Serial messages

Both Port and Starboard

PIE

Physical hardware

---

# Step 14 — Regression Checks

Verify:

other weapon

other ship

MiniCRT

diagnostics

shared parser

shared enums

shared weapon code

Andy routing

---

# Step 15 — Documentation

Update documentation when:

new weapon architecture exists

new fire mode added

new protocol added

new hardware introduced

weapon pipeline changes

---

# Step 16 — Completion Report

Summarize:

weapon systems changed

hardware changes

message changes

Andy changes

Unreal changes

MiniCRT changes

diagnostics added

tests performed

manual hardware verification remaining