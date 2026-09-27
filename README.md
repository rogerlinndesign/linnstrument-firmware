LinnStrument
============

This is the firmware for LinnStrument, running on an Arduino Due processor.

Building
--------

The sources are an Arduino sketch. They build with Arduino IDE 2.x or with
arduino-cli, using the Arduino SAM Boards core version 1.6.12 (select board
"Arduino Due (Native USB Port)"). Other versions of the SAM core are untested
and may create unknown build or execution problems.

From the command line, `go run ./tools/linnstrument build` compiles the firmware into
`build/`, which needs Go and arduino-cli, plus the Xcode Command Line Tools on
macOS, and `go run ./tools/linnstrument test` runs the desktop tests in `tests/`. The
same tool flashes a unit and backs up and restores its settings. Every push to
GitHub is also compiled by GitHub Actions, which attaches the resulting `.bin`
file to the workflow run.

On Apple silicon Macs the Arduino SAM toolchain is an Intel binary and needs
Rosetta installed (`softwareupdate --install-rosetta`).

Flash the resulting `.bin` with the LinnStrument Updater, which keeps your
settings. Uploading with bossac or the Arduino IDE directly erases all settings
and calibration.

Contributing
------------

See CONTRIBUTING.md for the branch and pull request workflow, how to test on
the instrument, and how to work on this code with an AI coding assistant.
CLAUDE.md holds the architecture notes and constraints that such assistants
read automatically; they are useful reading for people too.

Note that customer bug reports or feature requests should not be posted here.
Instead, email Roger at support@rogerlinndesign.com.
