Contributing to the LinnStrument firmware
=========================================

This guide is for anyone who wants to fix a bug or add a feature, whether you
write the code yourself or work with an AI coding assistant. It describes the
workflow that keeps changes easy to review and safe to ship.

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


Building
--------

Three ways, all producing the same firmware:

1. **Arduino IDE 2.x.** Install the "Arduino SAM Boards (32-bits ARM Cortex-M3)"
   core, version 1.6.12, select the board "Arduino Due (Native USB Port)", open
   `linnstrument-firmware.ino` and press Verify. "Sketch > Export Compiled
   Binary" writes the `.bin` into `build/arduino.sam.arduino_due_x/`.
2. **Command line.** `go run ./tools/linnstrument build` runs arduino-cli and writes
   the `.bin` into `build/`. This needs Go (1.25 or later) and arduino-cli
   installed, both single installers on macOS, Windows and Linux. On macOS
   the tool also needs the Xcode Command Line Tools (`xcode-select --install`),
   which Homebrew requires anyway.
3. **GitHub Actions.** Every push to any branch, and every pull request, is
   compiled automatically. Open the "Actions" tab on GitHub, click the run for
   your commit, and download the firmware from "Artifacts" at the bottom. The
   download is a zip; unzip it to get the `.bin`. Artifacts are kept for 90
   days. On a fork, Actions must be enabled once from the fork's Actions tab.
   This needs no software on your computer at all.

On an Apple silicon Mac the Arduino SAM toolchain is an Intel binary and needs
Rosetta (`softwareupdate --install-rosetta`). Apple is ending Rosetta after
macOS 27, so the GitHub Actions build is the reliable long-term path on Macs.

On Linux, your user must be in the group that owns the serial port (`dialout`
on Debian and Ubuntu, `uucp` on Arch), and ModemManager may hold the port for
a few seconds after the LinnStrument enters serial mode.


Desktop tests
-------------

`go run ./tools/linnstrument test` compiles the firmware for your computer and runs
the tests in `tests/` in a few seconds, with no instrument attached. They cover
the logic that can be checked without hardware: note layout, velocity, phantom
touch rejection, settings migration, MIDI and serial framing. Run them before
every push, and add one when you fix a bug in logic they can reach. See
`tests/README.md` for how to write one. GitHub Actions runs them on every push
as well.


Flashing and testing on the instrument
--------------------------------------

Flash the `.bin` with the LinnStrument Updater, the same tool customers use.
Put the file next to the Updater app or choose it from inside the app. The
Updater saves your settings before flashing and restores them afterwards.
Flashing with bossac or the Arduino IDE directly erases all settings and the
calibration.

Keep the official firmware `.bin` of the release you were running so you can
always go back.

For developers, `tools/linnstrument` does from the command line what the Updater does.
`go run ./tools/linnstrument update` backs up the settings and sequencer projects,
flashes `build/linnstrument-firmware.ino.bin` and restores them. `flash` alone
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
* * Check the MIDI output with a MIDI monitor when the change affects what is
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

### Setting up without a terminal

* **Claude Code on the web** (claude.ai/code): connect your GitHub account,
  pick this repository, and work from the browser. It edits on a branch in the
  cloud, commits, and can open the pull request. Nothing needs to be installed.
  It cannot compile there, so the loop is: ask, review the change, have it
  push, wait for the Actions run, download the `.bin`, flash, test.
* **Claude Code in VS Code**: install the Claude Code extension, open the
  repository folder, and chat in the side panel. Same workflow, with the files
  visible next to the conversation. This needs the Arduino toolchain locally if
  you want it to compile on your machine, otherwise rely on GitHub Actions.

Either way, make sure you are on your own branch before you start, and tell
the assistant the branch name.

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

1. **Compile before reporting back.** If it cannot compile, as on the web, it
   should say so and point you to the Actions run, not guess.
2. **Explain the change in plain words** and list the files it touched. If the
   explanation does not match what you asked, say so.
3. **Give you test steps** for the instrument: what to set, what to press,
   what you should see or hear.
4. **Commit on your branch only**, one commit per change, with a one-line
   past-tense message. No version bumps, no commits to `master`.
5. **Keep the change small.** If it proposes a rewrite or a cleanup of code you
   did not ask about, decline it and ask for the minimal change.

### What it cannot do

It cannot run the firmware, touch the instrument, hear anything or see the LEDs.
You are the test harness. When it says "this should work", it means "this
compiles and I believe it is right". Only your test on the instrument settles
it.

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
