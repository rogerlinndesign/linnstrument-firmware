Desktop tests
=============

These tests compile the firmware for the computer you're on and exercise its
logic without a LinnStrument: note mapping, velocity, phantom rejection,
settings migration, MIDI framing and the serial protocol. Run them from the
repository root:

```
go run ./tools/linnstrument test
```

`test NAME` runs only the tests whose name contains NAME, which is how you
run a new test on its own first. Note that the match is case-sensitive, and
that a NAME no test contains is an error.

You need Go and a C++ compiler, which is the Xcode Command Line Tools on
macOS, g++ on Linux and MinGW on Windows. Compiling and running everything
takes a few seconds.

How it works
------------

The tool merges the `.ino` files into one translation unit exactly as the
Arduino build does: the main sketch first, then the others in name order, with
a prototype for every function inserted before the first definition. It then
compiles that unit against the stand-ins in `support/` instead of the Arduino
core. There's a serial port that records what the sketch writes and feeds it
what a test chose, behind a 64 byte write buffer that drains one byte per 10
microseconds of fake clock like the Due's USB port. There's also a flash fake,
and a clock that only moves when a test says so, plus one microsecond per
read so that the sketch's busy waits terminate. Every `test_*.cpp` is included
into that same unit after the sketch, so a test can call any firmware function
and read or set any global directly.

`tests/mocks.cpp` has the helpers tests start from. `linn::reset()` gives
you a factory-fresh LinnStrument 200 and `linn::reset(128)` a 128,
`linn::select()` points the sketch's current-cell context at a cell, and
`linn::touch()` puts a touch on a cell without scanning.
`linn::press(col, row, x, y, z)`, `linn::hold(col, row, x, y, z, scans = 3)`
and `linn::release(col, row)` drive a finger through real scans of the fake
sensor. `linn::midiIn()` and `linn::midiInNrpn()` feed incoming MIDI,
`linn::drainMidi()` lets the MIDI queue out, and `linn::pumpSerial()` runs the
serial protocol over fed bytes.

Use `linn::touch()` for tests of mapping and state, where a touch simply has
to be there. Use `press()`, `hold()` and `release()` for velocity, pressure and
anything timed, because they take the finger through the scans the firmware
measures. `support/host_mock.h` controls the fakes, including
`host::setSensor()` for the raw X, Y and Z a cell reads. Note that X and Y are
single axes across the whole surface, not per cell. There are about 171 raw
units of X per column, and a raw Y band per row (`CALY_DEFAULT_MIN` and
`CALY_DEFAULT_MAX` in `ls_calibration.ino`).

Writing a test
--------------

```
TEST(notes_row_offset_changes_the_layout) {
  linn::reset();
  Global.rowOffset = 7;
  CHECK_EQ(getNoteNumber(LEFT, 1, 1) - getNoteNumber(LEFT, 1, 0), 7);
}
```

Every test also gets a line in `mutations.tsv`, tab separated, with the
test's name, a firmware file, a snippet of that file that occurs exactly once
(`\n` in the field is a line break) and a broken version of that snippet. For
instance:

```
nrpn_271_enters_serial_mode	ls_midi.ino	    case 271:	    case 272:
```

`go run ./tools/linnstrument mutate` first runs the whole suite on the real
firmware (with a NAME, just the tests it selects). It then applies each line in
turn, confirms that the named test fails on the broken firmware, and restores
the file, also on an interrupt. A test with no line in the table is reported as
NO MUTATION and fails the run, and so does a mutation that survives.
`go run ./tools/linnstrument mutate NAME` runs only the lines whose test name
contains NAME. It reports those tests without a line as NO MUTATION too, and
stops with an error when no test name contains NAME.

`TEST(name)` registers itself. `CHECK(condition)`, `CHECK_EQ(actual, expected)`
and `CHECK_NEAR(actual, expected, tolerance)` report with file and line and keep
going. Names must be unique across all test files, and a new file only needs to
match `tests/test_*.cpp`.

Things the fixture decides for you
----------------------------------

* `linn::reset()` wipes the whole settings struct before initialising it, so
  nothing survives from the previous test, and the tests run in the order
  they're written. A test must still not rely on the one before it.
* `setupDone` stays false, so `performContinuousTasks()` returns at once. This
  means that LED refresh, foot switch polling, the clock, the arpeggiator and
  sequencer advance and in-loop MIDI input never run on their own. Instead,
  call the function a test needs directly (`checkFootSwitches()`,
  `advanceArpeggiatorForSplit()`), or set `setupDone` around one
  `performContinuousTasks()` call and clear it again, as the scroll tests do.
  The fixture gives the project a tempo of 120 for that case, because the clock
  divides by it and x86 traps on a zero.
* `linn::drainMidi()` empties the whole output queue, however long it is. It
  doesn't clear what was captured, though. `host::serialOutput()` returns
  everything sent since `linn::reset()` or the last
  `host::clearSerialOutput()`, so a test that checks two phases clears it in
  between, as `lowRowOutput()` in `test_lowrow.cpp` does. The drain advances
  the clock 20 ms per pass, because the serial write buffer only drains with
  the clock, and a loop over `handlePendingMidi()` that never advances it
  stalls after 64 bytes. If the firmware stops sending, the drain gives up
  after 5000 passes and fails the test rather than hanging it.
* Sensor reads return zero, and `host::setSensor()` is the only way a cell
  reads anything.

Each file covers one topic: `test_notes.cpp` (note mapping and layouts),
`test_layouts.cpp`, `test_velocity.cpp`, `test_playing.cpp` (touches to MIDI),
`test_phantom.cpp`, `test_lowrow.cpp`, `test_arpeggiator.cpp`,
`test_switches.cpp`, `test_presets.cpp`, `test_settings.cpp`,
`test_upgrade.cpp` (every settings layout since version 1),
`test_calibration.cpp`, `test_channelbucket.cpp`, `test_midi.cpp` (framing,
NRPNs, clock), `test_serialmode.cpp` (the Updater's protocol) and
`test_userfirmware.cpp` (every claim of `user_firmware_mode.md`).

What can't be tested here is anything that depends on real sensor readings,
LED timing, or the feel of the instrument. Those still need a flash and a
person at the unit.
