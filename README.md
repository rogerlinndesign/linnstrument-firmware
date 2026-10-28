LinnStrument
============

This is the firmware for LinnStrument, the expressive MIDI controller made by Roger Linn
Design, together with the tools around it. The firmware runs on the Arduino Due inside the
instrument. The repository is set up so that anyone can work on it with an AI coding
assistant, musicians included, not only programmers.

Three ways in
-------------

1. **Use the browser pages, nothing to install.** The pages in `examples/browser/` talk to a
   LinnStrument over MIDI from Chrome, Edge or Firefox: a surface designer that turns the
   instrument into a custom controller of pads, keys, faders, XY pads and buttons for any DAW,
   an editor for every setting, and a live view of User Firmware Mode. Open
   `examples/browser/index.html`, allow MIDI when the browser asks, pick the instrument at the top
   right. `examples/README.md` describes them. Full support needs firmware 2.4.0 or later; on
   an older one the page says what is missing.

2. **Change the firmware without installing anything.** Claude Code on the web edits the
   firmware on a branch in the cloud, every push is compiled by GitHub Actions, which attaches
   the firmware file to the run, and the LinnStrument Updater flashes it while keeping your
   settings. `CONTRIBUTING.md` has the loop, the prompts to start with, and what is safe.

3. **Develop on your own computer, with or without an assistant.** Plain development works as
   it always has: open `linnstrument-firmware.ino` in Arduino IDE 2.x and press Verify, or
   build from the command line with Go and arduino-cli; a desktop test suite checks the
   firmware's logic in seconds without an instrument, the Virtual LinnStrument runs the
   firmware behind a window with a virtual MIDI port, and a pull request brings the change
   in. An AI assistant in the terminal can drive all of that for you and, when you say so, a
   connected instrument. `CONTRIBUTING.md` lists what to install and the workflow;
   `CLAUDE.md` is what the assistant reads, and is useful reading for people too.

![The surface designer page with a LinnStrument 128 connected](docs/images/surface-designer.png)

![The Virtual LinnStrument running the firmware on a Mac](docs/images/virtual-linnstrument.png)

Read in this order: this file, then `CONTRIBUTING.md`, which takes half an hour and covers the
workflow, safety and working with an assistant, then `CLAUDE.md` when you want to know how the
firmware is built, how it runs and what it must not do.

Building
--------

The sources are an Arduino sketch. They build with Arduino IDE 2.x or with arduino-cli, using
the Arduino SAM Boards core version 1.6.12 (select the board "Arduino Due (Native USB Port)").
Other versions of the SAM core are untested and may create unknown build or execution problems.

From the command line, `go run ./tools/linnstrument build` compiles the firmware into `build/`,
and `go run ./tools/linnstrument test` runs the desktop tests in `tests/`; the same tool flashes
a unit and backs up and restores its settings. `tools/virtual/` builds the Virtual LinnStrument.
Every push to GitHub is also compiled by GitHub Actions, which attaches the resulting `.bin`
file, the tool and the Virtual LinnStrument to the workflow run, so nothing has to be installed
to get a build.

On Apple silicon Macs the Arduino SAM toolchain is an Intel binary and needs Rosetta installed
(`softwareupdate --install-rosetta`).

Flash a `.bin` with the LinnStrument Updater, which keeps your settings. Uploading with bossac
or the Arduino IDE directly erases all settings and calibration.

Contributing
------------

See `CONTRIBUTING.md` for the branch and pull request workflow, what to install, what is safe
and what is not, and how to work on this code with an AI coding assistant.

Note that customer bug reports or feature requests should not be posted here. Instead, email
Roger at support@rogerlinndesign.com.
