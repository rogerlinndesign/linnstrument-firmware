LinnStrument User Firmware Mode
===============================

User Firmware Mode turns LinnStrument into a raw MIDI touch surface. The firmware stops playing
notes and stops drawing its own lights, and instead reports every cell press and, if you ask for
it, the X, Y and Z of every touch over MIDI, while you drive the LEDs over MIDI. This makes it
possible to build custom applications without changing the firmware. MIDI, even over USB, is slow
compared to the processor's own access to the sensor, so anything that needs the first
milliseconds of a touch, like velocity detection, can't be built this way; the firmware does that
part for you and sends the result.

The messages, values and ranges in this document are verified by the desktop tests in
`tests/test_userfirmware.cpp`; when the firmware and this text disagree, a test fails, so treat
the tables as exact. What only a hand on the instrument can check, the panel gestures, is not.


What it can and cannot do
=========================

It can:

* report each cell press and release as a MIDI note, with the velocity the firmware measured
* stream the pressure, the left-right position and the up-down position of every touch
* track a finger sliding from cell to cell along a row
* light any cell in any of the eleven colors
* be switched on and off remotely, and tell you when the player switches it by hand

It cannot:

* give you a better velocity than the firmware's own, since the strike happens before MIDI could
  report it
* draw anything but per-cell colors; there is no text or bulk update message
* survive a power cycle, it always starts switched off
* keep the LEDs lit while the Global Settings panel is open


Switching it on and off
=======================

Send NRPN 245 with value 1 on any channel to switch User Firmware Mode on, value 0 to switch it
off. NRPN 299 with value 245 asks for the current state.

Whenever the mode changes, whether you asked for it or the player toggled it by hand, LinnStrument
sends NRPN 245 with the new state on channel 9. The player can leave the mode by holding OS Update
in Global Settings for a full second, and can enter it the same way from the normal firmware. A
short tap on that cell is something else: it toggles serial mode for firmware updates, which takes
the instrument off MIDI until the cell is tapped again.

On entering, all lights go out, the Global Settings switch lights yellow, every per-row setting
below starts switched off and the decimation interval is back at its default. Set up your lights
and your X, Y, Z and slide options again every time. The light cursor is not reset by the switch,
so set it before you paint.

The Global Settings panel keeps working so the player can still reach Velocity Sensitivity,
Pressure Sensitivity, MIDI I/O and Actions. While that panel is open, no touches are reported
and the LED commands are ignored. The foot switches keep whatever they are assigned to and send
their normal messages: by default the right one is sustain and the left one the arpeggiator.


Coordinates
===========

Rows are MIDI channels: the bottom row is channel 1, the top row channel 8. Columns are note
numbers and CC numbers: the control switches on the left are column 0, the first playing column
is 1. A LinnStrument 200 has columns 0 to 25, a LinnStrument 128 columns 0 to 16.

The control switch column reports presses like any other cell, as note 0, except the Global
Settings switch at the bottom, which opens the panel instead. It never sends X, Y or Z.

| What            | Carried in    | Range                                           |
|-----------------|---------------|-------------------------------------------------|
| Row             | Channel       | 1 to 8, bottom to top                           |
| Column          | Note or CC    | 0 to 25 on a 200, 0 to 16 on a 128              |
| LED column, row | CC 20, CC 21  | Same numbering, column 0 is the switch column   |


Cell presses
============

A press sends Note On with the column as note number on the row's channel. The velocity is what
the firmware measured from the strike, shaped by the Velocity Sensitivity setting; with a fixed
velocity configured, every note carries that value.

A release sends Note Off on the same note and channel. Its velocity is a release velocity, from
how fast the pressure fell away: 127 for a finger that lifts instantly, less for a slow release.


3D touch data
=============

X, Y and Z are off until you enable them, separately for each row and each axis. Send the CC
below on the row's channel with value 1 to enable, 0 to disable.

| CC | Enables                  |
|----|--------------------------|
| 9  | Slide mode for the row   |
| 10 | X data for the row       |
| 11 | Y data for the row       |
| 12 | Z data for the row       |

Z, the pressure, is sent as Polyphonic Pressure on the row's channel with the column as note
number. 0 is the lightest touch that still registers, 127 the firmest. Pressure is smoothed over
successive scans, so a sudden change arrives as a short ramp.

X, the left-right position, is one axis across the whole surface: 0 at the left edge, 4265 at
the right edge of a 200 and about 2660 on a 128, with one cell being about 171 units. The exact
ends depend on the calibration. It is sent as a 14-bit value in two Control Changes on the
row's channel, the LSB first on CC 32 plus the column, then the MSB on CC 0 plus the column.

Y, the up-down position within the cell, is sent as CC 64 plus the column on the row's channel,
0 at the bottom edge to 127 at the top edge. Like pressure it is smoothed over scans.

A value is only sent when it changed, and never faster than the decimation interval below.

One thing to watch when decoding: column 6 sends its X on CC 6 and CC 38, which are also the
NRPN data entry controllers. LinnStrument only sends NRPNs for the mode announcement on channel 9
and as the reply to a query, on the channel the query came from, so treat CC 6 and 38 as data
entry only right after CC 99 and 98 arrived on that channel, and as X otherwise.


Data decimation
===============

Every touch produces a stream of values, which can overwhelm a slow receiver. The firmware
therefore sends each value at most once per decimation interval. The default is about 8
milliseconds. CC 13 sets it in milliseconds, and 0 switches the interval off so every change is
sent. In low power mode only values above 12 are accepted: 13 is the shortest interval there,
and 12 or less leaves it at 12.


Slide mode
==========

Sliding a finger along a row from cell to cell is something LinnStrument tracks far better than
you could by combining the messages above, so there is a slide mode per row, enabled with CC 9 on
the row's channel.

When a finger crosses into the next cell, three messages follow each other on the row's channel:
CC 119 with the column the finger is leaving, Note On for the new column with the same velocity
as the first cell of the slide, then Note Off for the old column with the new column as its
velocity. That order lets software hand the touch over without a gap.


Lights
======

CC 20 selects a column and CC 21 a row, counting from 0 on the switch column and the bottom row.
CC 22 then sets that cell's color, from the color table in `midi.md`: 1 to 6 and 8 to 11 light
it; 0, 7 (black) and any value of 12 and higher switch it off. Column 0 can be painted like any
other, including the Global Settings cell, whose yellow light is then covered. A CC 20 or CC 21
beyond the instrument's columns or rows is ignored and the cursor keeps its previous value. These lights live on their own layer, which is cleared when
the mode is left, so nothing you draw survives into the normal firmware.

CC 23 and CC 24, which store and clear custom light patterns, belong to the normal firmware and
have no use here.


MIDI message summary
====================

Outgoing:

| Message             | Meaning            | Channel | Data 1      | Data 2            |
|---------------------|--------------------|---------|-------------|-------------------|
| Note On             | Cell pressed       | Row     | Column      | Strike velocity   |
| Note Off            | Cell released      | Row     | Column      | Release velocity  |
| Polyphonic Pressure | Z                  | Row     | Column      | 0 to 127          |
| CC 32 to 57         | X, low 7 bits      | Row     | 32 + column | LSB, sent first   |
| CC 0 to 25          | X, high 7 bits     | Row     | Column      | MSB               |
| CC 64 to 89         | Y                  | Row     | 64 + column | 0 to 127          |
| CC 119              | Slide leaving cell | Row     | 119         | Column being left |
| NRPN 245            | Mode changed       | 9       | 245         | 0 off, 1 on       |

Incoming:

| Message  | Channel | Value          | Effect                                                  |
|----------|---------|----------------|---------------------------------------------------------|
| NRPN 245 | any     | 0 or 1         | Leave or enter User Firmware Mode                       |
| NRPN 299 | any     | 245            | Reply with NRPN 245 and the current state               |
| CC 9     | row     | 0 or 1         | Slide mode for that row                                 |
| CC 10    | row     | 0 or 1         | X data for that row                                     |
| CC 11    | row     | 0 or 1         | Y data for that row                                     |
| CC 12    | row     | 0 or 1         | Z data for that row                                     |
| CC 13    | any     | milliseconds   | Decimation interval, 0 is off, above 12 in low power    |
| CC 20    | any     | column         | Column for the next CC 22                               |
| CC 21    | any     | row            | Row for the next CC 22                                  |
| CC 22    | any     | color          | Light the selected cell, 0 or 12 and up switch it off   |

Other incoming messages are handled as in the normal firmware, except that notes from the host
do not light cells while the mode is on.


Examples
========

A single press on column 12 of row 3 with X, Y and Z enabled for that row, in non-slide mode:

```
Ch3 Note On 12 Velocity 120

Ch3 PolyPressure Key 12 Value Z
Ch3 CC 44 Value X LSB
Ch3 CC 12 Value X MSB
Ch3 CC 76 Value Y

Ch3 Note Off 12 Velocity 127
```

The same press sliding into column 13 with slide mode on:

```
Ch3 Note On 12 Velocity 120

Ch3 PolyPressure Key 12 Value Z
Ch3 CC 44 Value X LSB
Ch3 CC 12 Value X MSB
Ch3 CC 76 Value Y
Ch3 CC 119 Value 12
Ch3 Note On 13 Velocity 120
Ch3 Note Off 12 Velocity 13
Ch3 PolyPressure Key 13 Value Z
Ch3 CC 45 Value X LSB
Ch3 CC 13 Value X MSB
Ch3 CC 77 Value Y

Ch3 Note Off 13 Velocity 127
```


Driving it from scripts and AI agents
=====================================

LinnStrument shows up as the MIDI device "LinnStrument MIDI". Three free tools cover everything
below on macOS, Windows and Linux, and they all speak the same text format for MIDI messages:
[SendMIDI](https://github.com/gbevin/SendMIDI), [ReceiveMIDI](https://github.com/gbevin/ReceiveMIDI)
and [RouteMIDI](https://github.com/gbevin/RouteMIDI).

From a shell:

```
sendmidi dev "LinnStrument MIDI" nrpn 245 1                 # enter User Firmware Mode
sendmidi dev "LinnStrument MIDI" ch 4 cc 12 1 cc 10 1       # Z and X data for row 4
sendmidi dev "LinnStrument MIDI" cc 20 5 cc 21 3 cc 22 3    # light column 5, row 3 green
receivemidi dev "LinnStrument MIDI"                         # watch the touches arrive
sendmidi dev "LinnStrument MIDI" nrpn 245 0                 # back to the normal firmware
```

From an AI agent, RouteMIDI's MCP server gives the same access as tools instead of commands. Add
it to Claude Code with `routemidi --install-mcp claude-code` (other clients: see
`routemidi --help`), or use the `.mcp.json` in this repository. Then:

* `list_midi_ports` shows "LinnStrument MIDI" among the inputs and outputs.
* `start_route` with the tokens `in "LinnStrument MIDI" out "LinnStrument MIDI"` opens a route
  that both sends to the instrument and captures what it sends.
* `inject_midi` sends text messages into that route. An NRPN is its four control changes:
  `channel 1 control-change 99 1`, `98 117`, `6 0`, `38 1` enters the mode (245 is 1 times 128
  plus 117). `channel 4 control-change 12 1` enables Z on row 4, `channel 1 control-change 20 5`
  then `21 3` and `22 3` lights a cell.
* `read_route` returns the touches as text messages, one per line, with a cursor to poll from.

Whatever the client, the loop is the same: enter the mode, enable what you need per row, set
your lights, read the stream, and leave the mode when done so the player gets the instrument
back.
