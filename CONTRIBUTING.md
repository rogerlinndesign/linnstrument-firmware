Contributing to the LinnStrument firmware
=========================================

This guide is for anyone who wants to fix a bug or add a feature, whether you
write the code yourself or work with an AI coding assistant. It describes the
workflow that keeps changes easy to review and safe to ship. If you only want
to use the browser pages with your instrument, `examples/README.md` is all you
need. A glossary of the terms used here is at the end.

Bug reports and feature requests from customers go to
support@rogerlinndesign.com, not to this repository. GitHub issues are disabled.


Getting the code
----------------

* Collaborators with write access create branches directly in this repository.
* Everyone else forks it on GitHub and creates branches in the fork.
* Always start from `master`. That is the current development state and the
  base for the next release. The `stable` branch is historical and years behind.
* Make one branch per topic (one bug or one feature), named after you and the
  topic, for example `dev/roger-transpose-switch`. A branch name must not
  contain spaces, apostrophes or other punctuation besides `-`, `_` and `/`.

GitHub Desktop is fine for creating branches, looking at changes and pushing. If
an AI assistant does the editing and committing, you rarely need anything else.


What to install
---------------

Nothing, for the browser pages, or for changing the firmware with Claude Code
on the web and the LinnStrument Updater (see "Working with an AI coding
assistant"). For development on your own computer, in this order:

* **Git and a GitHub account.** GitHub Desktop covers branches and pushing.
* **Claude Code**, if you want an assistant: the installers are at
  claude.ai/code. The terminal program is started with `claude` in this
  folder; the VS Code extension opens the folder instead. Both read `CLAUDE.md`
  on their own.
* **Go 1.25 or later** (go.dev/dl) and **arduino-cli** (arduino.github.io), each
  a single installer, then once: `arduino-cli core install arduino:sam@1.6.12`.
* **macOS:** the Xcode Command Line Tools, `xcode-select --install`, and on
  Apple silicon Rosetta, `softwareupdate --install-rosetta`, which the Arduino
  compiler needs. **Linux:** g++, and your user in the serial port group.
  **Windows:** MinGW for the desktop tests.
* **CMake 3.22 or later** for the Virtual LinnStrument. Its first build fetches
  JUCE and takes a few minutes; later builds take about twenty seconds.
* Optional: **Node** for the example pages' tests, **Docker** for
  `tools/ci-linux.sh`, **sendmidi** and **receivemidi**
  (github.com/gbevin) and **RouteMIDI** for MIDI from the shell or as tools for
  the assistant. `.mcp.json` registers RouteMIDI's MCP server; when it is not
  installed, Claude Code reports that server as failed at startup, which is
  harmless.

Then `go run ./tools/linnstrument test` (a few seconds) and `go run
./tools/linnstrument build` (a minute or two the first time, about ten seconds
after that) prove the setup.


Building
--------

Three ways, all producing the same firmware:

1. **Arduino IDE 2.x.** Install the "Arduino SAM Boards (32-bits ARM Cortex-M3)"
   core, version 1.6.12, select the board "Arduino Due (Native USB Port)", open
   `linnstrument-firmware.ino` and press Verify. "Sketch > Export Compiled
   Binary" writes the `.bin` into `build/arduino.sam.arduino_due_x/`.
2. **Command line.** `go run ./tools/linnstrument build` runs arduino-cli and
   writes the `.bin` into `build/`, in about ten seconds once the core has been
   compiled. See "What to install".
3. **GitHub Actions.** Every push to any branch, and every pull request, is
   compiled automatically. Open the "Actions" tab on GitHub, click the run for
   your commit, and download the firmware from "Artifacts" at the bottom. The
   download is a zip; unzip it to get the `.bin`. The same run attaches
   prebuilt `linnstrument` tool binaries for Linux, Windows and macOS, and a
   zip of the Virtual LinnStrument for macOS with a screenshot of it running.
   Artifacts are kept for 90 days. On a fork, Actions must be enabled once
   from the fork's Actions tab. This needs no software on your computer at
   all.

On an Apple silicon Mac the Arduino SAM toolchain is an Intel binary and needs
Rosetta (`softwareupdate --install-rosetta`). Apple is ending Rosetta after
macOS 27, so the GitHub Actions build is the reliable long-term path on Macs.

On Linux, your user must be in the group that owns the serial port (`dialout`
on Debian and Ubuntu, `uucp` on Arch), and ModemManager may hold the port for
a few seconds after the LinnStrument enters serial mode.


Desktop tests
-------------

`go run ./tools/linnstrument test` compiles the firmware for your computer and
runs the tests in `tests/` in a few seconds, with no instrument attached;
`test NAME` runs only the tests whose name contains NAME, and `mutate` proves
every test can fail, at about six seconds per test. They cover the
logic that can be checked without hardware: note layout, velocity, phantom touch
rejection, settings migration, MIDI and serial framing. Run them before every
push, and add one when you fix a bug in logic they can reach. See
`tests/README.md` for how to write one. GitHub Actions runs them on every push
as well, together with `go vet ./...`, `go test ./...`, `go run
./tools/linnstrument nrpns` (which checks the NRPN table in midi.md against
the firmware) and `node --test examples/` for the example code, and builds
the Virtual LinnStrument on macOS. `tools/ci-linux.sh` runs the build and
test jobs in an x86_64 Ubuntu container with Docker, for checking a change on
Linux before pushing it.

`go run ./tools/linnstrument serve` serves the browser pages in `examples/` on
a local port and opens them; see `examples/README.md` for the pages.

`tools/virtual/` builds Virtual LinnStrument: the same firmware running on
your computer behind a window that shows its lights, takes the mouse as a
finger and has a virtual MIDI port, for trying a change without an instrument
on the desk. Its README explains the CMake build, `go run
./tools/linnstrument mcp` serves it to an AI assistant as an MCP server, and
`tools/virtual/build-release.sh TAG` builds it from an older firmware release.


What is safe and what is not
----------------------------

* The browser pages change nothing on the instrument beyond the settings and
  lights you ask for. User Firmware Mode, which the designer and the console
  use, is off again after a power cycle.
* The LinnStrument Updater keeps settings and calibration: it backs them up,
  flashes, and restores them. `go run ./tools/linnstrument update` does the
  same from the command line. `flash` on its own, bossac and the Arduino IDE
  erase settings and calibration; the unit then starts in serial mode and asks
  to be calibrated.
* An assistant may flash a unit only when you say so in that conversation
  (`CLAUDE.md` makes that a rule), and the command allowlist in
  `.claude/settings.json` covers building, testing, probing and backing up but
  never flashing or restoring, so those always ask you first.
* Keep the official firmware `.bin` of the release you were running (from
  rogerlinndesign.com) so you can always go back, and keep a `backup` file
  from before the first flash.
* The one way to make a unit unreachable over USB: a build in which Global
  Settings or its OS Update cell no longer work has no serial port and cannot
  be reflashed; the unit has to be opened to reach the Due's erase button.
  `CLAUDE.md` forbids an assistant to disable those. Everything else is
  recoverable.
* Recovery when a flashed build misbehaves: hold Global Settings, tap OS
  Update (the unit switches to serial mode), run the Updater with the official
  `.bin`, calibrate if the unit asks, and `go run ./tools/linnstrument restore
  FILE` if you had made a backup with the tool.


Flashing and testing on the instrument
--------------------------------------

Flash the `.bin` with the LinnStrument Updater, the same tool customers use.
Put the file next to the Updater app or choose it from inside the app. The
Updater saves your settings before flashing and restores them afterwards.

<img src="docs/images/linnstrument-updater.png" width="600"
     alt="The LinnStrument Updater">

For developers, `tools/linnstrument` does from the command line what the
Updater does, with the Updater's signed bossac for macOS and Windows bundled
in `tools/bossac/` (see its README) so no Arduino installation is needed.
`go run ./tools/linnstrument update` backs up the settings and sequencer
projects, flashes `build/linnstrument-firmware.ino.bin` and restores them.
`flash` alone
erases the settings, `backup` and `restore` move them separately, and `probe`
confirms that a unit in serial mode is talking. `midimode` and `serialmode`
switch the unit between its two USB personalities from the command line; the
latter sends NRPN 271 through [sendmidi](https://github.com/gbevin/SendMIDI),
which must be installed. Run the tool without arguments for the full list.

Before considering a change done, test on the instrument:

* The thing you changed works as intended.
* The feature it is closest to still works (the same screen, the same switch,
  the other split).
* Settings survive a power cycle and, for anything touching settings, a flash
  with the Updater from the previous firmware.
* If you own a LinnStrument 128 and 200, try both. If not, say which one you
  tested on.
* Check the MIDI output with a MIDI monitor when the change affects what is
  sent.


Commits
-------

* One change per commit. Do not mix a bug fix with an unrelated cleanup.
* Message: one line, past tense, describing what changed, like the existing
  history: "Added ...", "Fixed ...", "Changed ...".
* Never change `OSVersion` or `OSVersionBuild`. Version numbers are bumped by
  the maintainer when a release is assembled.
* Do not commit `build/`, and do not commit the `port` line in
  `.vscode/arduino.json` if it changed for your machine.
* Leave the debugging `#define` switches at the top of
  `linnstrument-firmware.ino` commented out.


Pull requests
-------------

Open one pull request per branch, against `master`. In the description, say:

* What changed and why, in a few sentences.
* How you tested it, on which model, with which firmware you upgraded from.
* Anything you are unsure about.

The maintainer (Geert) reviews every pull request, may ask for changes, and
merges it. Merged changes go out in the next firmware release, usually together
with other changes, so there is no need to coordinate release timing.

Small pull requests get reviewed quickly. A pull request that touches many
files for several reasons will wait.


Working with an AI coding assistant
-----------------------------------

The file `CLAUDE.md` in this repository is read automatically by Claude Code
and tells it how the firmware is built, what it must not do, and how a change
is verified. You do not need to repeat any of that.

### How the work is divided

Four stages, each settling something the previous one cannot:

1. **The assistant** edits the code and runs the desktop tests in `tests/`,
   which check the firmware's logic in seconds: note layout, velocity,
   settings, MIDI, the serial protocol. A change that breaks them is not done.
2. **The Virtual LinnStrument** (`tools/virtual/`) runs the real firmware on
   your computer. The assistant can drive it through the MCP server
   (`go run ./tools/linnstrument mcp`): press cells, read the lights, send and
   receive MIDI, take a screenshot. That settles what the lights and the MIDI
   do.
3. **Your instrument** settles feel, pressure, timing and sound. Only a person
   at the unit can; the assistant flashes it only when you say so.
4. **GitHub Actions** is the build of record: every push is compiled and the
   firmware attached to the run, so a reviewer, and you on another machine,
   get the same binary.

### Setting up

* **Claude Code on the web** (claude.ai/code): connect your GitHub account,
  pick this repository, and work from the browser. It edits on a branch in the
  cloud, commits, and can open the pull request. Nothing needs to be installed.
  It cannot compile there, so the loop is: ask, review the change, have it
  push, wait for the Actions run, download the `.bin`, flash with the Updater,
  test.
* **Claude Code in VS Code**: install the Claude Code extension, open the
  repository folder, and chat in the side panel. Same workflow, with the files
  visible next to the conversation, and with the toolchain installed it
  compiles and tests on your machine.
* **Claude Code in the terminal**: type `claude` in the repository folder.
  This is the fullest setup: it compiles, runs the tests, drives the Virtual
  LinnStrument and, with your permission, a connected instrument. The first
  time it runs a command it asks; `.claude/settings.json` pre-approves
  building, testing, probing and backing up, never flashing.

Either way, make sure you are on your own branch before you start, and tell
the assistant the branch name.

### Your first session

Prompts that work as they are, to be adapted:

> I am on a LinnStrument 200 with firmware 2.3.4, on the branch
> dev/me-played-colour. Read CLAUDE.md and CONTRIBUTING.md and tell me in a
> few lines how a change gets from here onto my instrument.

> Change the default colour of played notes from the current one to cyan.
> Run the desktop tests, compile, and give me the steps to check it on the
> instrument.

> Build the Virtual LinnStrument, start it, play a few notes on it and tell me
> which lights came on and what MIDI it sent.

> Commit this on my branch with a one-line past-tense message, push, and tell
> me which Actions run to download the firmware from.

### How to brief it

Describe behaviour, not code. Good briefs look like:

> When the low row is set to Strum and I release a note on the right split
> while holding one on the left, the right note keeps sounding. It should stop.
> I am on a LinnStrument 200 with firmware 2.3.4.

> Add "Transpose up" and "Transpose down" as options for the foot switches,
> next to the existing Octave options.

Include: what happens now, what should happen, how to reproduce it, which
model and firmware you are on. If you know the settings involved, name them as
they appear on the instrument.

### What to ask of it, every time

1. **Compile and run the desktop tests before reporting back.** If it cannot
   compile, as on the web, it should say so and point you to the Actions run,
   not guess. For anything about lights or MIDI, ask it to try the change on
   the Virtual LinnStrument too.
2. **Explain the change in plain words** and list the files it touched. If the
   explanation does not match what you asked, say so.
3. **Give you test steps** for the instrument: what to set, what to press,
   what you should see or hear.
4. **Commit on your branch only**, one commit per change, with a one-line
   past-tense message. No version bumps, no commits to `master`.
5. **Keep the change small.** If it proposes a rewrite or a cleanup of code you
   did not ask about, decline it and ask for the minimal change.

### What it can and cannot do

It can compile, run the desktop tests, run the firmware in the Virtual
LinnStrument and read its lights and MIDI, and with your word flash a unit and
back up and restore its settings. It cannot feel the surface, hear anything,
or judge timing and pressure. For those you are the test harness: when it
says "this should work", it means "this compiles, passes the tests and
behaves on the virtual instrument". Only your test on the instrument settles
the rest.

If it tells you an existing constant or check "looks like a bug", check the
"Things that look wrong but are deliberate" list in `CLAUDE.md` before letting
it change anything. Several values in this firmware were tuned on hardware
and look arbitrary from the code alone.

### Iterating

Flash, test, and report back what you observed, as precisely as you can ("the
LED lit but no MIDI note was sent", "the transpose went up two semitones
instead of one"). If something touch or timing related does not behave, ask
it to add serial debug output first so you can paste real numbers back, rather
than letting it guess at a second fix.

When the change works, ask it to open the pull request, or open it yourself on
GitHub, with the description described above.


Glossary
--------

* **Fork, branch, pull request.** Your own copy of the repository on GitHub, a
  named line of changes inside it, and the request to take those changes into
  this repository.
* **GitHub Actions, artifact.** The builds GitHub runs on every push, and the
  files a run attaches: the firmware `.bin`, the tool, the Virtual LinnStrument.
* **`.bin`.** The compiled firmware file the Updater flashes.
* **Arduino core.** The compiler and libraries for the Due, installed once with
  `arduino-cli core install arduino:sam@1.6.12`.
* **Serial mode, MIDI mode.** The instrument's two USB personalities: a serial
  port while the Updater talks to it (OS Update in Global Settings), a MIDI
  device the rest of the time.
* **Calibration.** The per-unit measurement of the surface's X and Y, kept
  with the settings and lost when a flash erases them.
* **NRPN, CC.** MIDI messages: a control change number with a value, and a
  numbered parameter sent as four of them. `midi.md` lists the instrument's.
* **User Firmware Mode.** The mode in which the instrument sends raw touches
  and takes lights over MIDI, which the browser pages use
  (`user_firmware_mode.md`).
* **Desktop tests, mutation.** The checks in `tests/` that run on your
  computer, and the proof that each can fail (`tests/README.md`).
* **MCP server.** The way an assistant gets tools: `go run ./tools/linnstrument
  mcp` offers the Virtual LinnStrument's commands as tools, RouteMIDI's offers
  MIDI ports.
* **Virtual LinnStrument.** The firmware running on your computer behind a
  window (`tools/virtual/README.md`).
