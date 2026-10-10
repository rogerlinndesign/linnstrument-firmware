Desktop tests
=============

These tests compile the firmware for the computer you are on and exercise its
logic without a LinnStrument: note mapping, velocity, phantom rejection,
settings migration, MIDI framing, the serial protocol. Run them from the
repository root:

```
go run ./tools/linnstrument test
```

They need Go and a C++ compiler (the Xcode Command Line Tools on macOS, g++ on
Linux, MinGW on Windows). Compiling and running everything takes a few seconds.

How it works
------------

The tool merges the `.ino` files into one translation unit exactly as the
Arduino build does (main sketch first, then the others in name order, with a
prototype for every function inserted before the first definition) and compiles
it against the stand-ins in `support/` instead of the Arduino core: a serial
port that records what the sketch writes and feeds it what a test chose, a flash
fake, a clock that only moves when a test says so (and one microsecond per read,
so the sketch's busy waits terminate). Every `test_*.cpp` is included into that
same unit after the sketch, so a test can call any firmware function and read or
set any global directly.

`mocks.cpp` has the helpers tests start from: `linn::reset()` for a
factory-fresh instrument, `linn::select()` to point the sketch's current-cell
context at a cell, `linn::touch()` to put a touch on a cell without scanning,
`linn::press()`, `linn::hold()` and `linn::release()` to drive a finger through
real scans of the fake sensor, `linn::midiIn()` and `linn::midiInNrpn()` to feed
incoming MIDI, `linn::drainMidi()` to let the MIDI queue out,
`linn::pumpSerial()` to run the serial protocol over fed bytes.
`support/host_mock.h` controls the fakes, including `host::setSensor()` for the
raw X, Y and Z a cell reads. X and Y are single axes across the whole surface,
not per cell: about 171 raw units of X per column, and a raw Y band per row
(`CALY_DEFAULT_MIN` and `CALY_DEFAULT_MAX` in `ls_calibration.ino`).

Writing a test
--------------

```
TEST(notes_row_offset_changes_the_layout) {
  linn::reset();
  Global.rowOffset = 7;
  CHECK_EQ(getNoteNumber(LEFT, 1, 1) - getNoteNumber(LEFT, 1, 0), 7);
}
```

Every test also gets a line in `mutations.tsv`: the test's name, a firmware
file, a line of it, and a broken version of that line. `go run ./tools/linnstrument
mutate` first runs the whole suite on the real firmware, then applies each
line in turn, confirms the named test fails on the broken firmware, and
restores the file, also on an interrupt. A test with no line in the table is
reported as NO MUTATION and fails the run, as does a mutation that survives.
`go run ./tools/linnstrument mutate NAME` limits the run to one test.

`TEST(name)` registers itself; `CHECK(condition)`, `CHECK_EQ(actual, expected)`
and `CHECK_NEAR(actual, expected, tolerance)` report with file and line and keep
going. Names must be unique across all test files. A new file only needs to
match `tests/test_*.cpp`.

Things the fixture decides for you
----------------------------------

* `linn::reset()` wipes the whole settings struct before initialising it, so
  nothing survives from the previous test, and the tests run in the order they
  are written. A test must still not rely on the one before it.
* `setupDone` stays false, so `performContinuousTasks()` returns at once: LED
  refresh, foot switch polling, the clock, the arpeggiator and sequencer advance
  and in-loop MIDI input never run on their own. Call the function a test needs
  directly (`checkFootSwitches()`, `advanceArpeggiatorForSplit()`), or set
  `setupDone` around one `performContinuousTasks()` call and clear it again, as
  the scroll tests do. The fixture gives the project a tempo of 120 for that
  case, because the clock divides by it and x86 traps on a zero.
* `linn::drainMidi()` empties the whole output queue, however long, so what a
  test reads is everything the firmware sent since the previous drain.
* Sensor reads return zero; `host::setSensor()` is the only way a cell reads
  anything.

What cannot be tested here: anything that depends on real sensor readings, LED
timing, or the feel of the instrument. Those still need a flash and a person at
the unit.