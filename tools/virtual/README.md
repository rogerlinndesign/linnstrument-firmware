Virtual LinnStrument
====================

The Virtual LinnStrument is the LinnStrument firmware running on your computer, behind a window
that shows its lights and turns the mouse into a finger, with a virtual MIDI port named "Virtual
LinnStrument". It's the real firmware, not an imitation. The same `.ino` files are merged into
one C++ file the way the Arduino build and the desktop tests do it, compiled against the
stand-ins in `tests/support/`, and run on a thread with the computer's clock. Everything the
instrument does over MIDI, the notes, expression, NRPNs, User Firmware Mode, lights and text,
behaves exactly as on the hardware. This makes it the quick way to develop against the
instrument without having one on your desk.

Building
--------

You need CMake 3.22 or later, a C++17 compiler (Xcode's on macOS) and Go for the sketch merge.
The first time, you also need git and an internet connection to fetch JUCE 9.0.4:

```
cmake -S tools/virtual -B build/virtual -DCMAKE_BUILD_TYPE=Release
cmake --build build/virtual --config Release
```

The first build fetches JUCE and takes a few minutes, while a rebuild after a firmware edit
takes about twenty seconds. The app lands in `build/virtual/VirtualLinnStrument_artefacts/Release/`,
and you can start it with:

```
open "build/virtual/VirtualLinnStrument_artefacts/Release/Virtual LinnStrument.app"
```

Its type is Mulish, the typeface of rogerlinndesign.com. The name and line printed on the panel
are set in Sarpanch Black and Sarpanch Bold narrowed, the open typeface nearest the instrument's
Eurostile Bold Extended and Bold Condensed. All of these are bundled from `Resources/Fonts` under
the SIL Open Font License, and the icon is the LinnStrument Updater's. If you already have a JUCE
checkout, pass `-DJUCE_PATH=/path/to/JUCE` to the first command to use it. Every build re-merges
the sketch when a firmware file changed, so a firmware edit is just one `cmake --build` away from
running in the window.

Using it
--------

* Settings persist the way the instrument's do, in `Virtual LinnStrument/flash-200.bin` and
  `flash-128.bin` in the user's application data folder (`~/Library/Application Support` on
  macOS). They're written on quit, on a model switch and periodically while they change. Delete
  the file for a factory-fresh unit, and also before judging a change to default settings, since
  an old file would hide it.
* The zip from a GitHub Actions run of this repository, `virtual-linnstrument-macos-` followed
  by the firmware version and the commit, holds an app signed with a Developer ID and notarised
  by Apple, so it opens like any downloaded app. Its window title and status line show the same
  version and commit, and Get Info in the Finder shows the version. A build without those
  credentials, such as a pull request from a fork, is signed ad hoc but not notarised. macOS
  then blocks its first launch, and current macOS no longer lets a right-click Open get past
  that. Instead, unzip it, then either run `xattr -dr com.apple.quarantine "Virtual
  LinnStrument.app"` in a terminal in that folder, or try to open it once and then choose Open
  Anyway in System Settings > Privacy & Security.
* The menu at the top left picks a 200 or a 128, and switching restarts the firmware.
* Click a pad to play it, drag to slide along a row or bend inside a cell, and shift-click or
  right-click to hold a touch down until the next click on it. The left column works like the
  instrument's, so you hold Global Settings, Per-Split Settings and the others to open the
  panels.
* The keyboard works too. Tab to the surface, then the arrow keys move between pads, Home and End
  jump along the row, and Space or Return holds the pad down until it's pressed again. A screen
  reader reads the focused pad as its column, row, light and touch state, and every control has
  a name and a help text.
* To pass options from a shell, run the binary inside the bundle (with `open` the current
  directory is `/`):

  ```
  app=build/virtual/VirtualLinnStrument_artefacts/Release
  "$app/Virtual LinnStrument.app/Contents/MacOS/Virtual LinnStrument" --model 128 \
    --screenshot build/shot.png --after 6
  ```

  `--model 128` starts as a 128. `--screenshot` writes the window to a PNG after `--after`
  seconds (2.5 by default) and quits, which is handy for a session that can't see the screen.
  Note that the path is relative to the current directory.
* `--script FILE` reads text commands from a file, or from stdin with `-`, so that a script or an
  AI session can touch the instrument and read it back without a mouse or a screen:

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
  it, `quit` included. The way out of a long one is `--after` or killing the process. A command
  that's cut short by the app leaving answers `error: stopped`, and one the app didn't get to
  within five seconds answers `error: timed out waiting for the app`.

  Every command answers on stdout with its output and then `ok`, or with `error: ...`. Lines
  starting with `#` are comments. The touches go through the firmware's own sensor path, so a
  `press` produces the note, velocity, pressure and position an instrument would send. In the
  same way, `leds` shows what the firmware lit, including the lights a user firmware mode script
  painted. A script file that ends without `quit` leaves the app running. Commands on stdin that
  end (the driver closed its pipe) quit it, so that nothing stays behind. To keep it running and
  send commands over time, feed it a pipe from the binary inside the bundle, since the `.app`
  opened with `open` has no stdin:

  ```
  app=build/virtual/VirtualLinnStrument_artefacts/Release
  : > build/cmds.txt
  bin="$app/Virtual LinnStrument.app/Contents/MacOS/Virtual LinnStrument"
  tail -f build/cmds.txt | "$bin" --script - &
  echo "press 5 3" >> build/cmds.txt
  ```

  Only one instance runs at a time. A second launch exits at once and the running one ignores
  its arguments, so quit the first one before starting a scripted one. `send` and `received`
  carry MIDI over the same pipe, so a script doesn't need a MIDI tool. For instance,
  `send B0 63 02 B0 62 2B B0 06 02 B0 26 10` asks for the model and `received 1` prints the
  NRPN 272 answer.
* `tools/virtual/build-release.sh TAG` builds the app from an older release of the firmware, for
  example `2.3.4`, the version customers still run. The tag's sketch goes in a worktree under
  `build/virtual-TAG/src` with this checkout's app, mocks and Go module copied in, and the binary
  path is printed last. Pages and scripts then see what that firmware does with their MIDI. For
  instance, 2.3.4 answers no query for the model, so the pages' old-firmware path shows. The
  CMake option `VIRTUAL_OLD_FIRMWARE` that the script sets leaves out the text-over-MIDI resets
  that a sketch before 2.4.0 doesn't have. 2.3.4 is the oldest release tried.
* `go run ./tools/linnstrument mcp [200|128]` serves the app to an AI assistant as an MCP server
  over stdio. The commands above are its tools (`press`, `move`, `release`, `tap`, `foot`,
  `leds`, `status`, `screenshot`, `model`, `send_midi`, `received_midi`, `wait`), and a
  screenshot is returned as an image. The app starts on the first tool call and quits with the
  session. `.mcp.json` registers it for Claude Code as `virtual-linnstrument`, and other clients
  take the same command. `LINN_VIRTUAL` names the binary when it's not the one built under
  `build/virtual`.
* Pressure sets how hard every touch presses, and Velocity how fast the strike arrives.
* Foot L and Foot R are the two foot switches.
* MIDI goes out on "Virtual LinnStrument" and comes in on the port of the same name, so the
  pages in `examples/browser/`, `sendmidi`, `receivemidi` and RouteMIDI talk to it just as they
  would to the instrument. The quickest tour of User Firmware Mode on it is
  `LINNSTRUMENT_DEVICE="Virtual LinnStrument" examples/shell/user_firmware_demo.sh`, which makes
  lights, text and a scroll appear in the window. Windows has no virtual ports. A loopMIDI pair
  is the workaround, but it isn't wired up yet.

What it cannot do
-----------------

It can't do multi-touch with a mouse, beyond holding touches with shift-click, right-click or
Space. It can't do calibration either, which needs real sensor readings, so the firmware runs on
its default calibration. Finally, it can't do serial mode either: tapping OS Update switches the
firmware to it, and with no serial port behind the window the instrument goes quiet until OS
Update is tapped again.
