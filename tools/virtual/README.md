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

Needs CMake 3.22 or later, a C++17 compiler (Xcode's on macOS), Go for the sketch merge, and an
internet connection the first time, to fetch JUCE 9.0.4:

```
cmake -S tools/virtual -B build/virtual
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

* The menu at the top left picks a 200 or a 128; switching restarts the firmware.
* Click a pad to play it, drag to slide along a row or bend inside a cell, shift-click or
  right-click to hold a touch down until the next click on it. The left column works like the
  instrument's: hold Global Settings, Per-Split Settings and the others to open the panels.
* The keyboard works too: Tab to the surface, the arrow keys move between pads, Home and End
  jump along the row, Space or Return holds the pad down until pressed again. A screen reader
  reads the focused pad as its column, row, light and touch state, and every control has a
  name and a help text.
* Pressure sets how hard every touch presses, Velocity how fast the strike arrives.
* Foot L and Foot R are the two foot switches.
* MIDI goes out on "Virtual LinnStrument" and comes in on the port of the same name, so
  `examples/user_firmware_console.html`, `examples/settings_editor.html`, `sendmidi`,
  `receivemidi` and RouteMIDI talk to it as they would to the instrument. The quickest tour of
  User Firmware Mode on it is `LINNSTRUMENT_DEVICE="Virtual LinnStrument"
  examples/user_firmware_demo.sh`: lights, text and a scroll appear in the window. Windows has no virtual ports; a loopMIDI pair is the workaround,
  not wired up yet.
* Settings persist the way the instrument's do, in `Virtual LinnStrument/flash-200.bin` and
  `flash-128.bin` in the user's application data folder. Delete a file for a factory-fresh unit.

What it cannot do
-----------------

Multi-touch with a mouse, beyond holding touches with shift. Calibration, which needs real
sensor readings; the firmware runs on its default calibration. Serial mode: tapping OS Update
switches the firmware to it, and with no serial port behind the window the instrument goes
quiet until OS Update is tapped again.
