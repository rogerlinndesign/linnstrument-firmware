Virtual LinnStrument
====================

The Virtual LinnStrument is the LinnStrument firmware running on your computer, behind a window
that shows its lights and turns the mouse into a finger, with a virtual MIDI port named "Virtual
LinnStrument". It's the real firmware, not an imitation. The same `.ino` files are merged into
one C++ file the way the Arduino build and the desktop tests do it, compiled against the
stand-ins in `tests/support/`, and run on a thread with the computer's clock. What the firmware
decides is therefore what an instrument decides: the notes, channels and expression a touch
turns into, the NRPNs, the settings screens, User Firmware Mode, the lights and the text. What's
simulated is everything around it: the touches come from the mouse, a script or an assistant
instead of a sensor, and the lights are drawn instead of lit. This makes it the quick way to
develop against the instrument without having one on your desk.

What it's for
-------------

It's for trying a firmware change, a page or a MIDI script before it reaches an instrument, and
for an AI assistant to check its own work. It settles what the lights show, which screens and
settings a touch reaches, and what MIDI goes out for a given touch or comes back for a given
message. It doesn't settle how a change feels to play: pressure response, velocity, sliding,
timing and sound still need a real LinnStrument and a person at it. It isn't meant as an
instrument to perform with either, since a mouse plays one touch at a time.

The ready-made app that GitHub Actions attaches to every run is for Apple silicon Macs. It's
only been built and tried on macOS. Windows has no virtual MIDI ports, so the app has nothing to
send to there, and Linux hasn't been tried.

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

Its type is Mulish, the typeface of rogerlinndesign.com, bundled from `Resources/Fonts` under the
SIL Open Font License. The name, rule and line the panel prints at the instrument's right end are
an image, `Resources/Nameplate/Nameplate.png` (and `Name.png`, the name alone), which
`make_nameplate.py --font EUROSTILE.otf` draws
in the panel's lettering, Eurostile, measured off a photo of the panel; the font only draws the
image and is not in the repository. The icon is the LinnStrument Updater's. If you already have a JUCE
checkout, pass `-DJUCE_PATH=/path/to/JUCE` to the first command to use it. Every build re-merges
the sketch when a firmware file changed, so a firmware edit is just one `cmake --build` away from
running in the window.

The firmware keeps all of its state in globals, so each copy of it that runs at the same time is
compiled into a namespace of its own (`Firmware.cmake`, `Source/Firmware/FirmwareInstance.inc`)
and reached through `firmware::Instance` in `Source/Firmware/FirmwareBridge.h`. An `Engine` takes
a free copy when it's made. The app needs one, and `-DLINN_FIRMWARE_COPIES=8` compiles eight, for
plugin hosts that open several instances; each copy adds about 700 KB. The firmware's own sources
stay as they are.

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
  slide COL ROW TOCOL [Y]   a held touch slides into the next cell of its row
  tap COL ROW [MS]          press and release after MS milliseconds (default 120, at least 20)
  foot left|right down|up   a foot switch
  leds                      the lights as "ROW: c c c ..." per row from the top, a color number
                            per column, 0 off, 7 black (dark)
  status                    the instrument's status line
  view                      what part of the instrument the window shows
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

  An app built on this code can add commands of its own through `CommandTarget::runCommand()`, and
  `help` then lists them too.

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
  session. When its window was closed or the app was rebuilt, the next tool call starts it
  again, which releases any touch held down, and the reply says so. `.mcp.json` registers it for
  Claude Code as `virtual-linnstrument`, and other clients take the same command. `LINN_VIRTUAL`
  names the binary when it's not the one built under `build/virtual`.
* Pressure sets how hard every touch presses, and Velocity how fast the strike arrives.
* Foot L and Foot R are the two foot switches.
* MIDI goes out on "Virtual LinnStrument" and comes in on the port of the same name, so the
  pages in `examples/browser/`, `sendmidi`, `receivemidi` and RouteMIDI talk to it just as they
  would to the instrument. The quickest tour of User Firmware Mode on it is
  `LINNSTRUMENT_DEVICE="Virtual LinnStrument" examples/shell/user_firmware_demo.sh`, which makes
  lights, text and a scroll appear in the window. Windows has no virtual ports. A loopMIDI pair
  is the workaround, but it isn't wired up yet.

Tests
-----

`Tests/VirtualTests.cpp` tests the app's own code with JUCE's unit test framework: how fingers are
tracked and slide into the next cell, the engine's pace, the surface and the firmware bridge. The
slide tests drive the real firmware and read the MIDI it sends, in real time, at the instrument's
own scan rate, which takes most of the twelve seconds they run in. They keep their settings in a
folder of their own, never the app's. They build with the app:

```
cmake --build build/virtual --config Release --target VirtualTests
build/virtual/VirtualTests_artefacts/Release/VirtualTests [NAME]
```

`tools/virtual/mutate_tests.py [NAME]` proves that they can fail: it breaks each guarded line of
the app's code in turn, rebuilds, checks that the test it belongs to fails, and restores the
file. GitHub Actions runs the tests and drives the built app by script on every push.

What it cannot do
-----------------

* **Touches are idealised.** A touch gives the firmware one clean reading for its cell. It lands at
  a level that the Velocity slider sets, which is what the firmware measures the velocity from,
  then rises towards the pressure of the Pressure slider (or a script's Z), slowly for a soft
  press and within milliseconds for a hard one, and when you let go its pressure falls away over
  about 50 ms before the note ends. Those timings were measured on a LinnStrument 128, but a
  finger's readings vary and drift, so the velocity, pressure and position numbers still differ
  from an instrument's for the same gesture. Every cell responds alike, since the per-cell bias
  that evens out an instrument's sensor is turned off.
* **No sensor quirks.** Untouched cells read nothing, so there's no noise, no cross talk between
  touches in a column and no phantom touches for the firmware to reject.
* **No multi-touch with a mouse,** beyond holding touches with shift-click, right-click or
  Space. A script or an assistant can hold several touches at once.
* **Its own calibration.** Calibrating needs real sensor readings, but a touch here lands exactly
  where the firmware's default calibration expects it, so every unit is marked calibrated with that
  data and its CALIBRATE cell is green. Being calibrated, the firmware also rejects a new touch whose
  position lies far outside its cell, which a touch here never does.
* **Different timing.** Each scan takes as long as the instrument's, 27 µs a cell and 0.58 ms more
  for each held touch as measured on a LinnStrument 200, on the computer's clock. The computer runs
  the firmware itself much faster in between, so latency and anything that depends on the
  firmware's own speed have to be judged on hardware.
* **One MIDI port.** All MIDI goes through the virtual port, whether the settings choose USB or
  DIN.
* **No serial mode.** Tapping OS Update switches the firmware to it, and with no serial port
  behind the window the instrument goes quiet until OS Update is tapped again. That's also why
  the Updater and the tool's `backup`, `restore` and `flash` can't reach it. Its settings live in
  the flash file described above.
