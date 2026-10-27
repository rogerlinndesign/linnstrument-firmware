# LinnStrument firmware: guide for AI coding assistants

This file is read automatically by Claude Code and similar tools. It tells the
assistant how this codebase is built, what its hard constraints are, and how
changes are expected to be made and verified. Humans should read
CONTRIBUTING.md.

## What this is

Firmware for the LinnStrument MIDI controller (LinnStrument 200 and
LinnStrument 128), running on an Arduino Due (Atmel SAM3X8E, ARM Cortex-M3,
84 MHz, 96 KB SRAM, 512 KB flash). It scans a pressure/position touch surface,
drives 208 RGB LEDs, and talks MIDI over USB and DIN.

The firmware's logic can be tested on the desktop (see Desktop tests below),
but sensing, timing and feel can only be verified on a physical instrument by
a person. Your job is to make changes that compile, pass the desktop tests,
are small and reviewable, and come with a way for the person to confirm them
on the device.

## Build

Compile from the command line with the Go companion tool, which wraps
arduino-cli, the same toolchain Arduino IDE 2.x uses. Go 1.25 or later and
arduino-cli are the prerequisites on macOS, Windows and Linux alike; macOS also
needs the Xcode Command Line Tools, because the serial port enumerator uses cgo:

```
go run ./tools/linnstrument build
```

That runs, from the repo root, and then reports static RAM use:

```
arduino-cli compile --fqbn arduino:sam:arduino_due_x \
  --libraries libraries --output-dir build .
```

Rules and facts:

* The folder holding `linnstrument-firmware.ino` must itself be named
  `linnstrument-firmware`, or the tool refuses (and arduino-cli underneath
  with "main file missing"). Any copy, snapshot or git worktree you build
  must keep that folder name.
* Requires the `arduino:sam` core, version 1.6.12. Install with
  `arduino-cli core install arduino:sam@1.6.12`.
* On Apple silicon the SAM toolchain is x86_64 only and needs Rosetta. If the
  compiler fails with "bad CPU type in executable", Rosetta is missing; tell the
  person rather than working around it. GitHub Actions
  (`.github/workflows/build.yml`) compiles every push on x86 runners and
  attaches the `.bin`, prebuilt `linnstrument` tool binaries for Linux,
  Windows and macOS, and a zip of the Virtual LinnStrument with a screenshot
  of it running, the one way to see the firmware run from a session that
  cannot build.
* The end of the output reports flash use ("Sketch uses N bytes") and, added
  by the tool, static RAM ("Global variables use N bytes", data plus bss of
  the 96 KB SRAM). Quote both in any change that adds arrays, buffers or
  features. Static RAM is already above 80%; keep additions small.
* Compile after every edit before claiming anything works. Where arduino-cli
  is not available, as in Claude Code on the web, say so, push the branch, and
  point the person at the GitHub Actions run. Never report a compile you did
  not run.

## Desktop tests

```
go run ./tools/linnstrument test
```

This compiles the whole sketch for the desktop against the stand-ins in
`tests/support/` and runs every `tests/test_*.cpp` in a few seconds; `test
NAME` runs only the tests whose name contains NAME. Run it after every change,
before the firmware build. `tools/ci-linux.sh` runs the build and tool
GitHub jobs in an x86_64 Ubuntu container with Docker; x86 traps on integer
division by zero where the Due and Apple silicon return 0, so a change that
touches arithmetic is worth one Linux run before pushing. `tests/README.md`
explains the mechanism and the helpers; the short version:

* A test is `TEST(name) { ... }` with `CHECK`, `CHECK_EQ` and `CHECK_NEAR`.
  Tests share one translation unit with the sketch, so they call firmware
  functions and read globals directly. Names must be unique across files.
* Start every test with `linn::reset()`, then `linn::select(col, row)` for the
  current-cell context, `linn::touch(col, row, rawZ)` to place touches,
  `linn::drainMidi()` before reading MIDI output from `host::serialOutput()`,
  `host::feedSerial()` plus `linn::pumpSerial()` for the serial protocol.
* The clock is fake: `host::advanceMicros()` moves it, and every `millis()` or
  `micros()` read adds a microsecond so busy waits end. `setupDone` stays
  false, so `performContinuousTasks()` is a no-op unless a test sets it around
  one call; call the function under test directly instead. `linn::reset()`
  wipes the whole settings struct, nothing survives from the previous test.
* A fix to host-reachable logic ships with a test that fails without it, and
  every test gets a line in `tests/mutations.tsv`: a snippet of firmware text
  that occurs exactly once (`\n` in the field is a line break) and its broken
  replacement, which must make exactly that test fail. `go run
  ./tools/linnstrument mutate` applies each line, checks the test fails, and
  restores the file; `mutate NAME` runs only the lines whose test name
  contains NAME. A test that survives its mutation is not a test.
* Keep the suite fast. A test that needs more than a fraction of a second is
  doing too much.
* Sensor reads return zero on the desktop. Set `currentRawZ` on a cell and
  clear its `shouldRefreshZ` flag instead, which is what `linn::touch()` does.
* Serial output goes through a 64 byte write buffer that drains one byte per
  10 us of fake clock, like the Due's USB port. Advance the clock between
  passes, or use `linn::drainMidi()`, or output stalls.

## User Firmware Mode

`user_firmware_mode.md` describes the mode in which the instrument is a raw MIDI
touch surface with MIDI-controlled lights: the way to customise what it does
without touching the firmware. The claims in that document that the desktop
can reach are checked by `tests/test_userfirmware.cpp`; change both together,
and give a new claim a test. `examples/` holds a browser
console for the mode and a shell walkthrough, with the protocol code in
`examples/user_firmware_protocol.js`, a surface designer page
(`examples/surface_designer.html`, logic in `examples/surface_designer.js`)
that turns the instrument into a custom controller of pads, keys, faders, XY
pads and buttons for any DAW, the shared connection widget and Web MIDI handling
in `examples/connection.js`, a front page `examples/index.html` for hosting
the folder on a static https host, and a settings editor page
(`examples/settings_editor.html`, logic in `examples/settings_editor.js`) whose
form is generated from the NRPN table in `midi.md` by `go run
./tools/linnstrument nrpns` into `examples/nrpn_parameters.js`, never edited
by hand. That command also checks the table against `receivedNrpn()` and
`sendNrpnParameter()` in `ls_midi.ino`: every number, its `inRange()` bounds,
and that a query answers it. Run it after changing either side; CI fails on a
mismatch or a stale generated file. `go run ./tools/linnstrument serve` serves
the pages on a local port (the browser then remembers the MIDI permission,
which it cannot for a page opened from a file). `node --test examples/` runs
every page's tests. A session can drive a connected unit in that mode over
MIDI through RouteMIDI's MCP server, which `.mcp.json` registers when
`routemidi` is installed, or through `sendmidi` and `receivemidi` from the
shell.

## Virtual LinnStrument

`tools/virtual/` is a JUCE app that runs this same firmware on the desktop:
the sketch merged into one file (`go run ./tools/linnstrument merge`), compiled
against `tests/support/` with the computer's clock, behind a window that
draws the LEDs, turns the mouse into touches and exposes a virtual MIDI port
named "Virtual LinnStrument". It is the fastest way to see a firmware or user
firmware mode change behave, without an instrument:

```
cmake -S tools/virtual -B build/virtual -DCMAKE_BUILD_TYPE=Release
cmake --build build/virtual --config Release
app=build/virtual/VirtualLinnStrument_artefacts/Release
"$app/Virtual LinnStrument.app/Contents/MacOS/Virtual LinnStrument" \
  --model 128 --screenshot shot.png --after 6
```

`--screenshot` writes the window to a PNG after `--after` seconds and quits,
so a session that cannot see the screen still can. `--script FILE` (or `-`
for stdin) reads text commands instead, the way to drive it from a terminal:
`press COL ROW [X Y [Z]]`, `move`, `release`, `tap`, `foot`, `leds` (the
lights as `ROW: c c c ...` lines from the top row down, 0 off, 7 black),
`status`, `send BYTES` (MIDI into the firmware, in hex), `received [SECONDS]`
(the MIDI it sent since the last call), `screenshot FILE [SCALE]`, `model`,
`sleep SECONDS`, `quit`; each answers with its output and `ok`. Touches made
this way go through the real firmware, so they produce the same MIDI an
instrument would. The simplest way in from a session is the MCP server:
`go run ./tools/linnstrument mcp` serves the app over stdio with those
commands as tools and screenshots as images, and `.mcp.json` registers it as
`virtual-linnstrument`, so the tools are there in Claude Code once the app is
built. From a shell, keep it running on a pipe from the binary inside the
bundle (`: > cmds.txt; tail -f cmds.txt | "$app/Virtual
LinnStrument.app/Contents/MacOS/Virtual LinnStrument" --script - &`), append
commands, and read the lights with `leds` or look at a screenshot; `sendmidi
dev "Virtual LinnStrument" ...` and `receivemidi dev "Virtual LinnStrument"`
reach its MIDI port too. A script file without `quit` leaves the app running
(stdin ending quits it), and only one instance runs at a time (a second
launch exits and the first ignores its arguments).
`tools/virtual/build-release.sh 2.3.4` builds the app from that release's
firmware instead, the way to see what customers' units do with a page or a
MIDI script before asking the person to try. The first build fetches JUCE
9.0.4 and takes a few minutes; later builds re-merge the sketch only when a
firmware file changed. `tools/virtual/README.md` has the details
and the limits (no multi-touch with the mouse, no calibration, no serial mode).

## Flashing and talking to a connected instrument

Flashing erases all settings and calibration on the unit. Only do it when the
person has said, in this conversation, that the connected unit may be erased.
Otherwise flashing is theirs to do with the LinnStrument Updater, which saves
and restores settings around the update.

When allowed, with the unit in serial mode (OS Update in Global Settings) and
showing up as a serial port (`/dev/cu.usbmodem*` on macOS, `/dev/ttyACM*` on
Linux, `COMn` on Windows):

```
go run ./tools/linnstrument <command>, with one of:
  ports              # list serial ports, mark the LinnStrument
  probe              # handshake, report the settings version
  led 5 3 3          # light one LED, a visible sign of life
  backup [FILE]      # settings and projects to JSON, ~35 s
  flash [FILE.bin]   # flash the .bin from build/ (about a minute with verify)
  restore FILE       # send a backup back, leaves serial mode
  update [FILE.bin]  # backup, flash, restore, like the Updater
  midimode           # leave serial mode without restoring
  serialmode         # enter serial mode over MIDI (NRPN 271), needs sendmidi
```

`flash` mirrors the Updater: a 1200 baud touch puts the SAM3X in its
bootloader, then bossac writes and verifies: whatever `LINN_BOSSAC` names
when set, otherwise the signed universal `tools/bossac/macos/bossac` on macOS
and `tools/bossac/windows/bossac.exe` on Windows (the one in the Arduino
package is Intel only), and arduino-cli's copy on Linux or when the bundled
one is missing. After a flash the firmware
boots in serial mode with calibration pending, so the unit stays reachable.
After `restore` or `update` the unit is back in MIDI mode, as for a customer.
`serialmode` brings it back into serial mode over MIDI through `sendmidi`
(Geert's cross-platform CLI, needed on the PATH), so a session can go both ways
without anyone at the instrument, as long as the unit runs firmware with NRPN
271. Prefer `flash` while iterating, and keep a `backup` file from before the
first flash.

Facts about the serial link, measured on a LinnStrument 128:

* Opening or closing the port toggles DTR, and the USB bridge then resets the
  instrument about one time in three, 0.4 to 1.5 seconds later. The port
  disappears for under a second and comes back. The tool waits out that
  window after opening and retries the handshake. Do everything in one open
  session; do not open and close the port per command.
* The protocol is the Updater's: send `5, 4, 3, 2, 1 ...\n`, expect
  `LinnStruments are go!\n`, then one-letter commands (`s` settings, `l` LED,
  `r` restore, `p`/`j`/`q` projects, `m` back to MIDI mode) with 96 byte
  batches and CRC32 per batch.
  `ls_serial.ino` is the firmware side, `tools/linnstrument/device.go` the host
  side.
* In serial mode the unit sends no MIDI, and in MIDI mode it has no serial port.
* A mode switch is stored with `storeSettings()`, which does nothing while the
  sequencer is running. Then the next DTR reset on a port open can undo the
  switch, so stop the sequencer before `serialmode` or `backup`.

## Source layout

The sketch is a set of `.ino` files that the Arduino build concatenates into a
single translation unit, in alphabetical order after the main file.
Consequences:

* Every global, function and type is visible from every file. Names must be
  unique across the whole sketch. When searching, grep all `.ino` files.
* The settings and touch structs and most enums are declared in
  `linnstrument-firmware.ino`; a few types live where they are used
  (`ls_sequencer.ino`, `ls_lowRow.ino`, `ls_serial.ino`, `ls_font.ino`,
  `ls_midi.h`). A type must be declared before the first function that uses
  it, which is why most sit in the main file. Function prototypes are
  generated automatically, so function order across files does not matter.
* `.h` files hold only the few things that need real headers (`ls_midi.h`,
  `ls_bytebuffer.h`, `ls_channelbucket.h`, `ls_debug.h`).

File map:

* `linnstrument-firmware.ino`: constants, the settings structs
  (`DeviceSettings`, `GlobalSettings`, `SplitSettings`, `PresetSettings`,
  `Configuration`), most other structs and enums, global state, `setup()`,
  `loop()`, and model detection (200 or 128 via pin 38).
* `ls_sensor.ino`: reading X, Y, Z from the touch surface over SPI, per-column
  sensitivities.
* `ls_touchInfo.ino`: `TouchInfo` methods: velocity, pressure curves, per-cell
  state.
* `ls_handleTouches.ino`: what happens on a new touch, a continuous XYZ update
  and a release: note on and off, pitch, timbre, loudness, phantom touch
  rejection, strumming.
* `ls_noteTouchMapping.ino`: which cells hold which MIDI notes and channels.
* `ls_channelbucket.h`: MIDI channel allocation for per-note channels (MPE
  style).
* `ls_midi.ino`, `ls_midi.h`: all MIDI in and out, the output queue
  (`midiOutQueue`), the NRPN settings API, clock, program change.
* `ls_midiLights.ino`: the helpers behind lights and text over MIDI (CC 25
  to 29) and the read-only model and version NRPNs, for scripts and user
  firmware mode; the CC and NRPN dispatch itself is in `ls_midi.ino`.
* `ls_lowRow.ino`: low row modes (sustain, restrike, strum, arpeggiator, bend,
  CC).
* `ls_arpeggiator.ino`, `ls_clock.ino`: the internal arpeggiator and the
  24 PPQ musical clock.
* `ls_sequencer.ino`: the polyphonic step sequencer, one per split, and project
  storage.
* `ls_settings.ino`: settings defaults, flash storage, presets, and the touch
  handlers for every settings screen.
* `ls_displayModes.ino`: `DisplayMode` painting for every screen (normal,
  per-split, global, and about 50 sub-screens).
* `ls_extstorage.ino`: settings transfer to the updater over serial, and
  migration from every older settings layout.
* `ls_serial.ino`: the serial protocol with the updater: handshake, send and
  restore settings and projects, switch to MIDI mode.
* `tools/virtual/`: the Virtual LinnStrument app, see below.
* `ls_leds.ino`, `ls_font.ino`, `ls_touchanim.ino`, `ls_playbook.ino`,
  `ls_promo.ino`: LED driving, text rendering, animations.
* `ls_switches.ino`: the two panel switches and two foot switches, and the
  switch assignments.
* `ls_faders.ino`, `ls_numericDataChange.ino`: CC fader screens and
  slide-to-change-value handling.
* `ls_calibration.ino`: X and Y calibration sampling and correction.
* `ls_rtos.ino`: `delayUsec()` and `performContinuousTasks()`: LED refresh,
  foot switch polling, arpeggiator and sequencer advance, sleep.
* `ls_test.ino`: debug printing and diagnostics.

## How the firmware runs

`loop()` dispatches on `operatingMode`. In performance mode each iteration
handles the one sensor cell that the previous `nextSensorCell()` read: a new
touch goes to `handleNewTouch()`, a continuing one to `handleXYZupdate()`, a
release to `handleTouchRelease()`. When a handler returns true the same cell is
processed again at once instead of moving on. Every `mainLoopDivider` cells
`performContinuousTasks()` runs: LED refresh, foot switches, arpeggiator and
sequencer advance, MIDI input or serial I/O, sleep.

Hard rules that follow from this:

* Never block. No `delay()`, no `delayMicroseconds()`, no busy loops waiting
  on I/O. Use `delayUsec()` from `ls_rtos.ino`, which keeps LEDs and switches
  alive.
* Anything that takes time is done incrementally across loop iterations, using
  `millis()` or `micros()` and `calcTimeDelta()`.
* MIDI goes out through `queueMidiMessage()` and the `preSend*` helpers, never
  by writing to `Serial` directly. The queue drains a few bytes per loop.
* No dynamic allocation, no `String`, no `new`. Fixed-size arrays only. Check
  RAM use after adding any.
* The device is either in MIDI mode or in serial mode (115200 baud) on its USB
  port, never both. Serial debug output and MIDI output cannot be observed at
  the same time. Design diagnostics so their result shows on serial or on LEDs.
* Two models share this firmware, see "The two models" below. Never hardcode
  26 or 25 for the column count; use `NUMCOLS`. `MAXCOLS` is for array sizing
  only.

## The two models

One firmware binary runs on both instruments. `setup()` reads pin 38 and sets
`LINNMODEL` to 200 or 128, `NUMCOLS` to 26 or 17 (column 0 is the control
column, so 25 or 16 playing columns), and every model-specific table is copied
into place from a `_200` or `_128` variant at boot. `NUMROWS` is 8 on both.

What differs, and where:

* Scan order and cell count: `SCANNED_CELLS_200` / `SCANNED_CELLS_128` and
  `CELLCOUNT` (201 or 129) in `ls_handleTouches.ino`, so a full scan of the
  128 is shorter.
* Sensor: the per-cell `Z_BIAS` table in `ls_sensor.ino`, and the X range over
  the whole surface (0 to 4265 on a 200, about 2660 on a 128, see
  `user_firmware_mode.md`). Calibration samples 9 columns on a 200 and 6 on a
  128 (`CALCOLNUM` in `ls_calibration.ino`).
* LEDs: `COL_INDEX_200` / `COL_INDEX_128` map columns to the LED hardware in
  `ls_leds.ino`.
* Layout: the default split point is column 12 on a 200 and 9 on a 128; the
  settings screens shift some cells (`ls_displayModes.ino`); scrolling text
  uses the condensed font on the 128 through `adaptfont_draw_string()`
  (`ls_font.ino`), with shorter labels ("SAM" for "SAME").
* Sequencer: patterns are 16 steps on a 200 and 8 on a 128, and the per-step
  faders are only shown on a 200 (`ls_sequencer.ino`).
* Animations and the manufacturing test have separate frames and cell lists.

Rules that follow:

* Any new screen, table or animation needs both variants, or a layout computed
  from `NUMCOLS`. Branch on `LINNMODEL` only for genuinely different hardware.
* The desktop tests run with `linn::reset()` for a 200 and `linn::reset(128)`
  for a 128; test layout code on both.
* Say which model a change was tried on.

## Idioms you will meet on the first edit

* The current cell is implicit: `sensorCol`, `sensorRow`, `sensorSplit` and
  `sensorCell` (a `TouchInfo*`) describe the cell being processed. Other cells
  are reached with `cell(col, row)`.
* `Device`, `Global` and `Split` are macros over the one `config` struct:
  `Device.serialMode`, `Global.currentPerSplit`, `Split[sensorSplit].sendX`.
* No floating point in the hot path. Positions and curves use the `FXD_*` and
  `FXD4_*` fixed-point macros in `linnstrument-firmware.ino`.
* LEDs: `setLed(col, row, color, cellOn)` sets one, `updateDisplay()` repaints
  the current display mode, `setDisplayMode()` switches screens. Never write to
  the LED hardware directly.
* Settings cells tell a tap from a hold with `ensureCellBeforeHoldWait()`,
  which returns true for a release within `SENSOR_HOLD_DELAY`; holds are
  handled in the `...Hold()` functions. The OS Update cell toggles serial mode
  on a tap and User Firmware Mode on a hold.

## Settings and persistence

All persistent state lives in the `Configuration` struct (device, global and
per-split settings, presets, sequencer project), stored in the Due's flash via
`DueFlashStorage` and exchanged with the updater over serial during upgrades.
The layout is versioned by `Device.version` (currently 17). On upgrade the
firmware receives the old blob and converts it through
`upgradeConfigurationSettings()` in `ls_extstorage.ino`, which selects a
converter by version number and then checks `confSize ==
sizeof(ConfigurationVN)`. A size mismatch silently falls back to default
settings, so every layout change must be done completely.

Changing any field of `DeviceSettings`, `GlobalSettings`, `SplitSettings` or
`PresetSettings` (adding, removing, reordering, retyping) means, in
`ls_extstorage.ino`:

1. Snapshot the struct you are changing as it is now, under the next number of
   its own series (`DeviceSettingsV13`, `GlobalSettingsV10`, `SplitSettingsV8`,
   `PresetSettingsV12`). The series are independent of each other and of
   `Device.version`.
2. Repoint every older snapshot that still names the live type to the new
   snapshot. `ConfigurationV16` for example holds a live `DeviceSettings`,
   which is only correct while `DeviceSettings` is unchanged.
3. Add `ConfigurationV17`, the layout version 17 had, built from snapshots.
4. Write `copyConfigurationV17()` mapping it onto the live `Configuration`,
   give the new field its default there, and give it a default in
   `initializeDeviceSettings()` or `initializePresetSettings()` in
   `ls_settings.ino` as well.
5. In `upgradeConfigurationSettings()`, make `case 17` use
   `sizeof(ConfigurationV17)` and `copyConfigurationV17`, and add `case 18`
   with the plain `memcpy` the newest version gets.
6. Set `Device.version = 18` in `initializeDeviceSettings()`.
7. If the setting should be reachable over MIDI, add the NRPN in `ls_midi.ino`
   and document it in `midi.md`.
8. Add `upgrade_from_v17` to `tests/test_upgrade.cpp`, which already upgrades
   a blob of every earlier layout and checks that planted values arrive, and
   give it a mutation line.

Commit 1f8858f (low row transpose) is a complete example of such a bump.

Sequencer projects (`SequencerProject`, `StepData`, `StepEvent`) have no
migration at all: they are restored raw and rejected when their size changed,
which discards all 16 of a user's projects on upgrade. Do not change those
structs without adding a migration path first.

A new switch assignment, display mode or setting screen usually also touches:
the enum in `linnstrument-firmware.ino`, a `paint...` function in
`ls_displayModes.ino`, and `handle...NewTouch` / `handle...Release` in
`ls_settings.ino`. Look at a recent small feature commit (for example the
Transpose switch assignments, commit 41c0451) as a template.

## Things that look wrong but are deliberate

* `MAX_TOUCHES_IN_COLUMN` is 3 and is load-bearing. Measured on hardware: with
  4 or more fingers held in one column, cross talk pulls X readings far enough
  that real touches in neighbouring columns are rejected as phantoms. Do not
  raise or remove it as a "fix".
* `Z_BIAS` and the other sensitivity tables in `ls_sensor.ino` are rows first,
  columns second, unlike every other array. The comment there says why.
* The `/*!!*/` markers in `setup()` flag hardware-specific sequencing. Do not
  reorder those lines. Pins 35 and 36 there select serial versus MIDI and USB
  versus DIN. A flashed build that can no longer reach the OS Update cell in
  Global Settings has no serial port and cannot be reflashed over USB; the
  unit then has to be opened to use the Due's erase button. Never disable
  Global Settings, the OS Update tap, or `applySerialMode()` in a build you
  flash.
* Commented-out `#define` lines near the top of `linnstrument-firmware.ino`
  (frame displays, `TESTING_SENSOR_DISABLE`, diagnostics) are switches for
  people debugging on hardware. Leave them as you found them and never enable
  one in a commit.

## Debugging on the instrument

Serial output needs the LinnStrument in serial mode (the OS Update cell in
Global Settings toggles it) and `DEBUG_ENABLED` defined in `ls_debug.h`.
`DEBUGPRINT((level, "text"))` prints when `debugLevel` is at least `level`.
`debugLevel` starts at -1, so only level -1 prints until the player sets it:
with `DEBUG_ENABLED`, column 17 rows 0 to 4 of Global Settings set levels -1
to 3, and the secret switches in column 18 rows 0 to 5 print MIDI debug
output and show X, Y and Z frames, scan time and free RAM. The
`DISPLAY_*_AT_LAUNCH` defines at the top of
`linnstrument-firmware.ino` force serial mode and one of those switches at
boot, for when the settings panel cannot be reached.

The person tests on the instrument and pastes serial logs or describes what
they saw, unless a unit is connected to this machine and they have allowed
flashing it, in which case you can flash a diagnostic build yourself and read
its serial output through a `Session` from `tools/linnstrument/device.go`. For
anything touch or timing related, the proven approach is: add a serial
diagnostic first, collect numbers, then change the code based on measurements
rather than reasoning alone.

## Working conventions

* Style: two-space indentation, K&R braces, `boolean`/`byte`/`short` types as
  in the surrounding code, constants in `UPPER_CASE`, camelCase functions. Match
  the file you are in.
* Comments: the least possible. If the code is self-explanatory, none. Keep only
  what the code cannot show: a constraint, a hardware reason, why a tempting fix
  would break something. Delete anything that narrates the next line. When
  extending a line that already has a comment, keep its wording.
* Never use an em dash in any text: code comments, docs, commit messages, pull
  request text, replies. Rewrite with a comma, a colon, parentheses or two
  sentences.
* Do not change `OSVersion` or `OSVersionBuild`. Release version bumps are done
  by the maintainer in a separate commit.
* Do not commit `build/`, or the `port` field change in `.vscode/arduino.json`.
* Reusable scripts and tools live in `tools/` with a line in CONTRIBUTING.md,
  never in a scratch directory. Machine facts (ports, paths) are looked up at
  run time, not hardcoded.

## Working agreements

These are the maintainer's rules for any contributor and any assistant.

**Self-review before declaring done.** Never call work complete straight after
it compiles. Re-read the full diff and answer two questions: does the defect
just fixed exist anywhere else, and did the change leave an adjacent path
inconsistent with the one touched? Fix the class of defect, not the reported
instance. Grep every `.ino` file for the pattern just changed, since the sketch
is one translation unit and the same idiom recurs across files. List every call
site of any helper introduced and confirm every branch of the method touched is
consistent. Then state what was verified, not just what was changed.

**Every fix comes with a way to prove it.** A desktop test when the logic can be
reached from `tests/`, and always concrete steps for the instrument: which
settings, which touches, what should be seen or heard, and what a MIDI monitor
should show. A change that compiles proves nothing about behaviour. For anything
touch or timing related, add a serial diagnostic first, measure, then change
code.

**After removing or renaming anything,** grep for every reference to the old
name: `midi.md`, `user_firmware_mode.md`, comments, adjacent code, this file.
Stale references survive a green build.

**Review findings are claims, not instructions.** Assess each one before
acting on it and say so when it is wrong. A fix implemented on a reviewer's
say-so has been disproved on the instrument and reversed before.

**Commits.** Never commit or push without explicit approval. One topic per
commit, a single line in the past tense matching the history ("Added ...",
"Fixed ...", "Updated ..."), a short factual body only for substantial
changes.

**Before reporting a change as done:** the desktop tests pass, it compiles for
the Due, the full diff was re-read, every `.ino` file was grepped for the same
pattern, and the person has exact steps to try on the instrument.
