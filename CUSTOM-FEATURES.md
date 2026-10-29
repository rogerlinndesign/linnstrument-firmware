# Custom Expression Features

This fork of the LinnStrument firmware (based on OS v2.3.4) adds four custom
features aimed at expressive playing and integration with orchestral/sample
libraries. None of these change stock behavior unless you explicitly enable
them. This document describes each feature, how to configure it, and how to
test it.

---

## 1. Pressure Mode

**Where:** Global Settings, column 14, row 2.

Controls how Z (pressure) is emitted at note transitions. Tap the cell to cycle
through three modes; the LED color shows the active mode.

| LED color   | Mode           | Description |
|-------------|----------------|-------------|
| Dark (off)  | Normal         | Stock behavior — Z is reset to 0 before every note-on. |
| Cyan        | Legato Hold    | Holds the pre-slur pressure flat during a lock window, then eases to live pressure once a note is sustained past that window. Great for fast slurs where volume should stay steady. |
| Yellow      | Velocity Blend | Starts Z at the note-on velocity, holds it, then fades to live pressure. Works in all MIDI modes. |

### Legato Hold — how it works

When you press a new note while another is still held, the firmware does **not**
drop pressure to 0. Instead:

1. **Hold phase** — pressure is held flat at the value it had when the slur
   began, for `legatoFadePauseMs`. Every new note pressed within this window
   resets the timer, so a rapid run of legato notes keeps the volume steady.
2. **Fade phase** — once a note survives past the lock window without a new note
   interrupting, pressure interpolates from the held value to your live finger
   pressure over an equal-length window.

This lets you play quick slurs/runs without the mapped volume dipping or spiking
on every note, while still giving natural expression on sustained notes.

**Adjusting the lock/fade window (Legato Hold):**
- Hold the **calibration cell** (col 16, row 3) and tap col 14, row 2 → the
  display shows the pause value in ms (cyan numerals).
- Slide left/right to change the value (0–99, stored as 0–990 ms). Default: 10.
- The hold window and the fade window are always **equal** to this value.

**Adjusting the velocity-blend timing (Velocity Blend):**
- Hold the calibration cell and tap col 14, row 2 → shows the hold duration (yellow).
- The fade duration uses `velocityBlendFadeMs` (default 10).

### Testing

- **Normal:** Every note-on momentarily resets pressure to 0 — same as stock.
- **Legato Hold:** Map Z to volume (e.g. CC 11 or channel pressure). Hold one
  note at a steady pressure, then rapidly play several legato notes under the
  pause time — the volume should stay flat throughout the run. Then hold a
  single note past the pause window — the volume should smoothly track your
  finger pressure again.
- **Velocity Blend:** Strike notes at different velocities and confirm the
  initial loudness matches the velocity before settling to live pressure.

---

## 2. Single-Column CC Fader

**Where:** Per-Split Settings — set a split to CC Faders mode (col 14, row 6),
then make the split exactly **one column wide** via the split point.

A one-column-wide CC fader replaces the stock on/off toggle with a continuous,
latching fader:

- Press the pad → the value tracks pressure (Z) or vertical position (Y) and is
  sent as the fader's CC.
- Finger in the **left 60%** of the pad → value updates live.
- Roll **right past 60%** → value locks at its last level (no further updates).
- Release → the value stays latched (the CC does **not** drop to 0).

**Source-axis toggle:** Per-Split Settings, column 14, row 3.
- Dark = **Z (pressure)** — natural for dynamics/expression.
- Lit  = **Y (vertical position)** — good for timbre/mod; steady if you slide.

### Testing

Assign a single-column CC fader. Confirm the value rides your pressure/Y in the
left region, freezes when you roll right, and holds its value after release.
Toggle the axis and confirm Z vs. Y sourcing.

---

## 3. Keyswitch Column

**Where:** Global Settings, column 17, row 1 (pink LED when on).
**LinnStrument 200 only** (col 17 doesn't exist on the 128).

Reserves the leftmost column of each non-fader split as a dedicated keyswitch
column, useful for triggering articulation keyswitches in sample libraries.

- Keyswitch pads rest at **dim pink**; the play area shifts right by one column.
- Press → full pink + Note On on the split's main MIDI channel.
- Release → back to dim pink + Note Off.
- Fader-mode splits are unaffected.

**Configuring notes:** Per-Split Settings, column 15 shows a pink indicator when
enabled. **Hold** it to open the keyswitch-note editor. The right column selects
which row (0–7) to edit; the left area is a slider for the MIDI note value
(0–127). Defaults: rows 0–7 → notes 0–7.

### Testing

Enable the column, confirm the leftmost column shows dim pink and the play area
shifts right. Press pads and confirm Note On/Off on the split channel. Open the
editor, reassign a row's note, and confirm the new note is sent.

---

## 4. Low-Row Slide Fix

**Where:** Global Settings, column 17, row 0 (cyan LED when on).
**LinnStrument 200 only.**

Fixes two artifacts when sliding across low-row pads in CC X (absolute) or
CC XYZ mode:

1. **X backward jump at cell transitions** — as a finger crosses from column N
   to N+1, the new cell reads the touch near its left edge, producing a backward
   X jump. The fix accumulates a correction offset at each transition so X stays
   continuous.
2. **Y/Z instability during transitions** — for a short window (~20 ms) after a
   column change, Y/Z readings are unreliable. The fix gates output during that
   window (Z sends 0, Y is skipped).

### Testing

Set up a low row in CC X (absolute) or CC XYZ mode. Slide a finger horizontally
across several columns and confirm the X CC stream is monotonic (no backward
jumps) and that Y/Z don't glitch at the boundaries.

---

## Note-priority for Poly Pressure

In polyphonic Z modes (Poly Pressure, or channel-per-note / channel-per-row),
only the **most recently pressed note** emits Z. When that note is released, the
previously held note resumes sending its own pressure. One-channel mode is
unaffected (its single focused note already owns the channel-pressure stream).

---

## Defaults & Safety

All new settings default to their stock/off state and are appended to the end of
the saved-settings structs, so existing presets load without corruption. A fresh
firmware flash re-initializes settings and runs calibration as usual.
