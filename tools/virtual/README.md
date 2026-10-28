Virtual LinnStrument
====================

The LinnStrument firmware running on your computer, behind a window that shows its lights and
turns the mouse into a finger, with a virtual MIDI port named "Virtual LinnStrument". It is the
real firmware, not an imitation: the same `.ino` files are merged into one C++ file the way the
Arduino build and the desktop tests do it, compiled against the stand-ins in `tests/support/`,
and run on a thread with the computer's clock. Everything the instrument does over MIDI, the
notes, expression, NRPNs, User Firmware Mode, lights and text, behaves exactly as on the
hardware, which makes it the quick way to develop against the instrument without one on the desk.

Building
--------

Needs CMake 3.22 or later, a C++17 compiler (Xcode's on macOS), Go for the sketch merge, and git
and an internet connection the first time, to fetch JUCE 9.0.4:

```
cmake -S tools/virtual -B build/virtual -DCMAKE_BUILD_TYPE=Release
cmake --build build/virtual --config Release
```

The app lands in `build/virtual/VirtualLinnStrument_artefacts/Release/`. Its type is Mulish,
the typeface of rogerlinndesign.com; the name and line printed on the panel are set in Sarpanch
Black and Sarpanch Bold narrowed, the open typeface nearest the instrument's Eurostile Bold
Extended and Bold Condensed, all bundled from `Resources/Fonts` under the SIL Open Font License,
and its icon is the LinnStrument Updater's. Pass
`-DJUCE_PATH=/path/to/JUCE` to the first command to use a JUCE checkout you already have.
Every build re-merges the sketch when a firmware file changed, so a firmware edit is one
`cmake --build` away from running in the window.

Using it
--------

* Settings persist the way the instrument's do, in `Virtual LinnStrument/flash-200.bin` and
  `flash-128.bin` in the user's application data folder (`~/Library/Application Support` on
  macOS), written on quit, on a model switch and periodically while they change. Delete the
  file for a factory-fresh unit, and before judging a change to default settings, which an old
  file would hide.
* The zip from a GitHub Actions run is not signed or notarised: the first launch is blocked by
  macOS; right-click the app, choose Open, and confirm once.
* The menu at the top left picks a 200 or a 128; switching restarts the firmware.
* Click a pad to play it, drag to slide along a row or bend inside a cell, shift-click or
  right-click to hold a touch down until the next click on it. The left column works like the
  instrument's: hold Global Settings, Per-Split Settings and the others to open the panels.
* The keyboard works too: Tab to the surface, the arrow keys move between pads, Home and End
  jump along the row, Space or Return holds the pad down until pressed again. A screen reader
  reads the focused pad as its column, row, light and touch state, and every control has a
  name and a help text.
* From a shell, `--model 128` starts as a 128, and `--screenshot shot.png --after 6` writes the
  window to a PNG after six seconds (2.5 by default) and quits, for a session that cannot see
  the screen. The path is relative to the current directory.
* `--script FILE` reads text commands from a file, or from stdin with `-`, so a script or an AI
  session can touch the instrument and read it back without a mouse or a screen:

  ```
  press COL ROW [X Y [Z]]   touch a cell, X and Y 0 to 1 inside it (default 0.5), Z 1 to 4095
                            (0 or none: the pressure slider)
  move COL ROW X Y [Z]      move a touch inside its cell, keeping its pressure unless Z is given
  release COL ROW           lift a touch
  tap COL ROW [MS]          press and release after MS milliseconds (default 120, at least 20)
  foot left|right down|up   a foot switch
  leds                      the lights as "ROW: c c c ..." per row from the top, a color number
                            per column, 0 off, 7 black (dark)
  status                    the instrument's status line
  send BYTE [BYTE ...]      MIDI into the firmware, bytes in hex, like B0 14 05
  received [SECONDS]        the MIDI the firmware sent since the last received, one message per
                            line in hex, after waiting up to SECONDS for a first one (default 0)
  screenshot FILE.png [S]   write the window to a file (quote a path with spaces), at S times
                            its size (default 2, as on a retina screen)
  model 200|128             restart as the other model; the MIDI port is recreated, so a
                            listener such as receivemidi has to reconnect
  sleep SECONDS             wait
  help                      this text
  quit                      leave
  ```

  Commands run one after the other, so a `sleep` or a long `tap` holds everything written after
  it, `quit` included; `--after` or killing the process is the way out of a long one. A command
  cut short by the app leaving answers `error: stopped`, and one the app did not get to within
  five seconds answers `error: timed out waiting for the app`.

  Every command answers on stdout with its output and then `ok`, or with `error: ...`. Lines
  starting with `#` are comments. The touches go through the firmware's own sensor path, so a
  `press` produces the note, velocity, pressure and position an instrument would send, and
  `leds` shows what the firmware lit, including the lights a user firmware mode script painted.
  A script file that ends without `quit` leaves the app running; commands on stdin that end
  (the driver closed its pipe) quit it, so nothing stays behind. To keep it running and send
  commands over time, feed it a pipe from the binary inside the bundle (the `.app` opened with
  `open` has no stdin):

  ```
  app=build/virtual/VirtualLinnStrument_artefacts/Release
  : > cmds.txt
  bin="$app/Virtual LinnStrument.app/Contents/MacOS/Virtual LinnStrument"
  tail -f cmds.txt | "$bin" --script - &
  echo "press 5 3" >> cmds.txt
  ```

  Only one instance runs at a time; a second launch exits at once and the running one ignores
  its arguments, so quit the first before starting a scripted one. `send` and `received` carry
  MIDI over the same pipe, so a script needs no MIDI tool: `send B0 63 02 B0 62 2B B0 06 02 B0
  26 10` asks for the model and `received 1` prints the NRPN 272 answer.
* `tools/virtual/build-release.sh TAG` builds the app from an older release of the firmware, for
  example `2.3.4`, the version customers still run: the tag's sketch goes in a worktree under
  `build/virtual-TAG/src` with this checkout's app, mocks and Go module copied in, and the
  binary path is printed last. Pages and scripts then see what that firmware does with their
  MIDI (2.3.4 answers no query for the model, so the pages' old-firmware path shows). The
  CMake option `VIRTUAL_OLD_FIRMWARE` the script sets leaves out the text-over-MIDI resets a
  sketch before 2.4.0 does not have; 2.3.4 is the oldest release tried.
* `go run ./tools/linnstrument mcp [200|128]` serves the app to an AI assistant as an MCP server
  over stdio, with the commands above as tools (`press`, `move`, `release`, `tap`, `foot`,
  `leds`, `status`, `screenshot`, `model`, `send_midi`, `received_midi`, `wait`) and a
  screenshot returned as an image. The app starts on the first tool call and quits with the
  session. `.mcp.json` registers it for Claude Code as `virtual-linnstrument`; other clients
  take the same command. `LINN_VIRTUAL` names the binary when it is not the one built under
  `build/virtual`.
* Pressure sets how hard every touch presses, Velocity how fast the strike arrives.
* Foot L and Foot R are the two foot switches.
* MIDI goes out on "Virtual LinnStrument" and comes in on the port of the same name, so
  `examples/surface_designer.html`, `examples/user_firmware_console.html`,
  `examples/settings_editor.html`, `sendmidi`, `receivemidi` and RouteMIDI talk to it as they
  would to the instrument. The quickest tour of
  User Firmware Mode on it is `LINNSTRUMENT_DEVICE="Virtual LinnStrument"
  examples/user_firmware_demo.sh`: lights, text and a scroll appear in the window. Windows has
  no virtual ports; a loopMIDI pair is the workaround, not wired up yet.

What it cannot do
-----------------

Multi-touch with a mouse, beyond holding touches with shift-click, right-click or Space.
Calibration, which needs real sensor readings; the firmware runs on its default calibration.
Serial mode: tapping OS Update switches the firmware to it, and with no serial port behind the
window the instrument goes quiet until OS Update is tapped again.
