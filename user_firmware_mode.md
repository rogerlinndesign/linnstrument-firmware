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


Quick start
===========

On the "LinnStrument MIDI" port, as bytes, everything on channel 1 unless said otherwise:

```
B0 63 01  B0 62 75  B0 06 00  B0 26 01    NRPN 245 = 1 enters the mode (245 is 1 x 128 + 117)
B0 63 02  B0 62 2B  B0 06 02  B0 26 10    NRPN 299 = 272 asks the model, the reply is NRPN 272
                                          = 200 or 128 (299 is 2 x 128 + 43, 272 is 2 x 128 + 16)
B3 0C 01                                  CC 12 = 1 on channel 4: pressure for row 3
B0 14 05  B0 15 03  B0 16 03              CC 20, 21, 22: light column 5, row 3 green
93 05 64  A3 05 40  83 05 7F              a press on that cell: note on, pressure, note off
B0 63 01  B0 62 75  B0 06 00  B0 26 00    NRPN 245 = 0 leaves the mode
```

A reply or an announcement from the instrument is the same four control changes, followed by
the null RPN, CC 101 and CC 100 with value 127.


What it can and cannot do
=========================

It can:

* report each cell press and release as a MIDI note, with the velocity the firmware measured
* stream the pressure, the left-right position and the up-down position of every touch
* track a finger sliding from cell to cell along a row
* light any cell in any of the eleven colors, one message per cell for a whole surface
* draw text in the firmware's own fonts, still or scrolling
* tell you which model and firmware it is
* be switched on and off remotely, and tell you when the player switches it by hand

It cannot:

* give you a better velocity than the firmware's own, since the strike happens before MIDI could
  report it
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
Pressure Sensitivity, MIDI I/O and Actions. While that panel is open, no touches are reported,
the LED commands are ignored and your lights are hidden; they come back when it closes. The
foot switches keep whatever they are assigned to and send their normal messages: by default
the right one is sustain and the left one the arpeggiator.

A few things from the normal firmware stay in force. Holding Global Settings and Per-Split
Settings together puts the instrument to sleep, and with Sleep on in Global Settings it goes to
sleep by itself after the sleep delay without a touch, lights included. The touch filtering is
the firmware's own: at most three touches in one column are tracked, and the fourth corner of a
rectangle of three held cells is rejected as a phantom touch. NRPN 271 still switches the
instrument to serial mode, after which it is gone from MIDI. With MIDI Through on (NRPN 254)
everything you send is echoed back to you.

If your script dies, the instrument stays in the mode with the last lights until something
sends NRPN 245 with value 0 or the player holds OS Update, so make leaving the mode part of
your exit path. Send every NRPN as all four control changes in a row: a query cut off after
CC 99 and CC 98 leaves the instrument expecting CC 6 and CC 38 next.


Coordinates
===========

Rows are MIDI channels: the bottom row is channel 1, the top row channel 8. Columns are note
numbers and CC numbers: the control switches on the left are column 0, the first playing column
is 1. A LinnStrument 200 has columns 0 to 25, a LinnStrument 128 columns 0 to 16.

The control switch column reports presses like any other cell, as note 0, except the Global
Settings switch at the bottom, which opens the panel instead. It never sends X, Y or Z.

| What            | Carried in   | Range                                         |
|-----------------|--------------|-----------------------------------------------|
| Row             | Channel      | 1 to 8, bottom to top                         |
| Column          | Note or CC   | 0 to 25 on a 200, 0 to 16 on a 128            |
| LED column, row | CC 20, CC 21 | Same numbering, column 0 is the switch column |


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

| CC | Enables                |
|----|------------------------|
| 9  | Slide mode for the row |
| 10 | X data for the row     |
| 11 | Y data for the row     |
| 12 | Z data for the row     |

Z, the pressure, is sent as Polyphonic Pressure on the row's channel with the column as note
number. 0 is the lightest touch that still registers, 127 the firmest. Pressure is smoothed over
successive scans, so a sudden change arrives as a short ramp.

X, the left-right position, is one axis across the whole surface: 0 at the left edge, 4265 at
the right edge of a 200 and about 2660 on a 128, with one cell being about 171 units. The exact
ends depend on the calibration. It is sent as a 14-bit value in two Control Changes on the
row's channel, the LSB first on CC 32 plus the column, then the MSB on CC 0 plus the column.

Y, the up-down position within the cell, is sent as CC 64 plus the column on the row's channel,
0 at the bottom edge to 127 at the top edge. Like pressure it is smoothed over scans.

A value is only sent when it changed since the last value sent on that note or controller, not
since the touch began, and never faster than the decimation interval below. A second press at
the same spot with the same pressure therefore sends no Z, X or Y until something moves, so
take a touch's existence from its Note On and treat the three values as updates.

One thing to watch when decoding: column 6 sends its X on CC 6 and CC 38, which are also the
NRPN data entry controllers. LinnStrument only sends NRPNs for the mode announcement on channel 9
and as the reply to a query, on the channel described below, so treat CC 6 and 38 as data
entry only right after CC 99 and 98 arrived on that channel, and as X otherwise. In the same
way, column 1's X arrives on CC 1 and column 7's on CC 7, the mod wheel and volume controllers,
so filter before forwarding a row's channel to a synth.


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
beyond the instrument's columns or rows is ignored and the cursor keeps its previous value.
These lights live on their own layer, which is cleared when the mode is left, so nothing you
draw survives into the normal firmware.

CC 25 colors the cell under the cursor like CC 22 and then moves the cursor one cell to the
right, continuing at column 0 of the next row up. A whole surface is therefore one CC 20, one
CC 21 and then one CC 25 per cell: 208 messages on a 200, 136 on a 128. Above the top row the
cursor stops and CC 25 is ignored until CC 21 moves it back, and so is CC 22, because after a
full run the cursor sits above the top row. While the Global Settings panel is open the cursor
still moves and only the light is skipped, so a stream stays in step. There is no message that
clears the whole surface: a full run of CC 25 with value 0 does it, as does leaving the mode.
Repaint only the cells that changed rather than the whole surface on every step.

CC 23 and CC 24, which store and clear custom light patterns, belong to the normal firmware and
have no use here.


Text
====

The firmware's own fonts can be used as well. CC 27 appends a character to a text of up to 32,
with the 7-bit ASCII code as the value; a value of 0 clears the text. CC 26 selects the font:
0 is the tiny font, 1 the small font, 2 the big font, which is the default, and 3 the condensed
font the 128 uses for its own screens. Characters a font does not have are skipped, and every
character takes its width plus one blank column:

| Font        | Rows | Characters                                      | Width            |
|-------------|------|-------------------------------------------------|------------------|
| 0 tiny      | 4    | space and digits                                | 3, 1 and space 2 |
| 1 small     | 8    | space, punctuation, digits, capitals, t         | 1 to 5           |
| 2 big       | 8    | space, punctuation, digits, capitals, lowercase | always 5         |
| 3 condensed | 8    | space, + - . /, digits, capitals, b             | 1 to 5, mostly 3 |

In the small font the digits and most capitals are 4 wide, E F I J L T 3, M V W X Y 5. In the
condensed font the digits and capitals are 3 wide except G K N Q at 4 and M W at 5, the 1 is 2
and the dot 1. The small and condensed fonts leave their top pixel row empty, the condensed font
its bottom row too. So on a 128, "HELLO" in the condensed font is 20 columns and fits, in the
big font it is 30 and does not; the cursor and the width tell you where the text ends.

CC 28 draws the text in the color of its value, with the cursor cell as the bottom left pixel,
and then clears the text so the next one starts fresh. Blank pixels inside the text's box are
switched off, cells outside it are left alone, and whatever runs past the right edge or above
the top row is cut off. The switch column never holds text: a cursor there starts the text at
column 1.

CC 29 scrolls the text instead of drawing it: it enters from the right edge, moves along the
cursor's row one column every 40 milliseconds and leaves at the left edge, so it takes about
the column count plus the text's width times 40 milliseconds, 2 seconds for "HELLO" in the
condensed font on a 128. Nothing announces the end; count it from the width. The scroll owns
the band of rows it moves through, the font's height from the cursor's row up: lights you
painted there are erased as the text passes and are not put back. A new CC 29 replaces a scroll
that is still running and clears that whole band, a CC 29 with no text just stops and clears it,
and leaving the mode stops it as well. Repaint the band after a scroll if you need it.

Text uses the same layer as the lights and follows the same rules: nothing is drawn while the
Global Settings panel is open, though CC 28 and CC 29 still clear the text, and everything
disappears when the mode is left.


Asking which instrument this is
===============================

NRPN 299 is a query: send it with the number of another parameter as its value, and the
instrument replies with that parameter and its current value. Send queries on channel 1, where
the reply also arrives; a query on any other channel is currently answered one channel below
it. Three read-only parameters exist for scripts: 272 answers with the model, 200 or 128,
273 with the firmware version, 234 for firmware 234.074, 274 with its build, 74 for that same
firmware, and 275 with 1 when the instrument has been calibrated and 0 when it has not, in which
case X and Y come from the default calibration and are less exact. Together with 245, which
answers whether the mode is on, a script can find out what it is talking to before it sends
anything else. The model gives you the column count and the X range above.


MIDI message summary
====================

Outgoing:

| Message             | Meaning            | Channel     | Data 1      | Data 2            |
|---------------------|--------------------|-------------|-------------|-------------------|
| Note On             | Cell pressed       | Row         | Column      | Strike velocity   |
| Note Off            | Cell released      | Row         | Column      | Release velocity  |
| Polyphonic Pressure | Z                  | Row         | Column      | 0 to 127          |
| CC 32 to 57         | X, low 7 bits      | Row         | 32 + column | LSB, sent first   |
| CC 0 to 25          | X, high 7 bits     | Row         | Column      | MSB               |
| CC 64 to 89         | Y                  | Row         | 64 + column | 0 to 127          |
| CC 119              | Slide leaving cell | Row         | 119         | Column being left |
| NRPN 245            | Mode changed       | 9           | 245         | 0 off, 1 on       |
| NRPN 245, 272-275   | Reply to a query   | 1, see text | Parameter   | Its value         |

Incoming:

| Message  | Channel | Value        | Effect                                                    |
|----------|---------|--------------|-----------------------------------------------------------|
| NRPN 245 | any     | 0 or 1       | Leave or enter User Firmware Mode                         |
| NRPN 299 | any     | 245          | Reply with NRPN 245 and the current state                 |
| NRPN 299 | any     | 272 to 275   | Reply with the model, firmware version, build, calibrated |
| CC 9     | row     | 0 or 1       | Slide mode for that row                                   |
| CC 10    | row     | 0 or 1       | X data for that row                                       |
| CC 11    | row     | 0 or 1       | Y data for that row                                       |
| CC 12    | row     | 0 or 1       | Z data for that row                                       |
| CC 13    | any     | milliseconds | Decimation interval, 0 is off, above 12 in low power      |
| CC 20    | any     | column       | Column for the next CC 22                                 |
| CC 21    | any     | row          | Row for the next CC 22                                    |
| CC 22    | any     | color        | Light the selected cell, 0 or 12 and up switch it off     |
| CC 25    | any     | color        | Light the selected cell and select the next one           |
| CC 26    | any     | 0 to 3       | Font: tiny, small, big, condensed                         |
| CC 27    | any     | ASCII        | Append a character to the text, 0 clears the text         |
| CC 28    | any     | color        | Draw the text at the selected cell, then clear it         |
| CC 29    | any     | color        | Scroll the text along the selected row, then clear it     |

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
sendmidi dev "LinnStrument MIDI" cc 20 1 cc 21 0 cc 25 5 cc 25 5 cc 25 5   # three blue cells
sendmidi dev "LinnStrument MIDI" cc 21 0 cc 26 2 cc 27 72 cc 27 105 cc 29 3   # scroll "Hi"
sendmidi dev "LinnStrument MIDI" nrpn 299 272               # which model? NRPN 272 answers
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
  then `21 3` and `22 3` lights a cell, `27 72`, `27 105` and `28 3` draw "Hi" in green at the
  selected cell, and `99 2`, `98 43`, `6 2`, `38 16` asks for the model (NRPN 299 with value
  272: 299 is 2 times 128 plus 43, 272 is 2 times 128 plus 16).
* `read_route` returns the touches as text messages, one per line, with a cursor to poll from.

Whatever the client, the loop is the same: enter the mode, enable what you need per row, set
your lights, read the stream, and leave the mode when done so the player gets the instrument
back.


Recipes
=======

In `sendmidi` form, all on channel 1 unless the row's channel is meant:

* Clear the surface: `cc 20 0 cc 21 0` then `cc 25 0` once per cell, 208 times on a 200 and
  136 on a 128.
* Paint a column: `cc 20 5` then, for each row, `cc 21 ROW cc 22 COLOR`; CC 25 only runs along
  a row.
* Light the cell under a finger and restore it afterwards: on Note On `cc 20 COL cc 21 ROW
  cc 22 8` (white), on Note Off the same with the cell's own color from your copy of the
  surface. Keep that copy yourself, the instrument cannot be asked what is lit.
* Show a word, then get the row back: `cc 21 ROW cc 26 3 cc 27 72 cc 27 73 cc 29 COLOR`, wait
  the column count plus the text's width times 40 milliseconds, then repaint the band of rows
  the scroll used.
* Find out what you are talking to before painting: `nrpn 299 272` and read NRPN 272 back,
  26 columns for 200 and 17 for 128.
