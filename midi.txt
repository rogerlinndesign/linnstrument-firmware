LinnStrument responds to the following MIDI input
=================================================

Status     Data       Message                 Used for
-------------------------------------------------------------------------------------------------------------
1001cccc   0nnnnnnn   Note On                 Highlight cells that correspond to the note
           0vvvvvvv                           The split is determined by its active MIDI mode channels
                                              c is the channel, n is the note number, v is the velocity
1000cccc   0nnnnnnn   Note Off                Un-highlight cells that correspond to the note
           0vvvvvvv                           The split is determined by its active MIDI mode channels
                                              c is the channel, n is the note number, v is the velocity
1011cccc   0nnnnnnn   Control Change          See table below
           0vvvvvvv                           c is the channel, n is the cc number, v is the velocity
11111010              Start                   Start listening to MIDI clock and reset ARP
11111011              Continue                Same behavior as Start
11111100              Stop                    Stop listening to MIDI clock and reset ARP
11111000              Timing Clock            Tempo sync, sent 24 times per quarter note
11110010   0lllllll   Song Position Pointer   Beats since song start, used for tempo sync alignment
           0mmmmmmm                           l is the LSB, m the MSB
1100cccc   0ppppppp   Program Change          Set the active preset for a split
                                              The split is determined by its active MIDI mode channels
                                              c is the channel, p is the preset number
11111110              Active Sensing          During manufacturing test, show green led at col 25 row 2

MIDI Control Change input
=========================

CC Number   Used for
-------------------------------------------------------------------------------------------------------------
1-8         Change the position of the CC faders, the split is determined by its active MIDI mode channels
9           Configure User Firmware X-axis row slide, the channel specifies the row (0: disable, 1: enable)
10          Configure User Firmware X-axis data, the channel specifies the row, default is off (0: disable, 1: enable)
11          Configure User Firmware Y-axis data, the channel specifies the row, default is off (0: disable, 1: enable)
12          Configure User Firmware Z-axis data, the channel specifies the row, default is off (0: disable, 1: enable)
13          Configure User Firmware MIDI decimation rate in milliseconds (minimum 12 ms in low power mode)
20          Column coordinate for cell color change with CC 22 (starts from 0)
21          Row coordinate for cell color change with CC 22 (starts from 0)
22          Change the color of the cell with the provided column and row coordinates
            see color value table below, 11+: default color
23			Make current custom cell colors persistent as pattern (0-2) 
24			Clear persisted custom cell colors pattern (0-2)

NRPN input
==========

NRPN messages are a series of MIDI CC messages that allow changing more parameters than are supported by
the standard MIDI CC message list. LinnStrument always expects an exact series of 6 MIDI CC messages to be
received for setting one NRPN to a different value. The first two select the NRPN parameter number, the
next two set the NRPN parameter value (both MSB and LSB are used), and the last two reset the active NRPN
parameter number. Failure to reset the NPRN parameter number can result in other MIDI input messages to
behave unpredictably.

This is an overview of the message chain:

1011nnnn   01100011 ( 99)  0vvvvvvv         NRPN parameter number MSB CC
1011nnnn   01100010 ( 98)  0vvvvvvv         NRPN parameter number LSB CC
1011nnnn   00000110 (  6)  0vvvvvvv         NRPN parameter value MSB CC
1011nnnn   00100110 ( 38)  0vvvvvvv         NRPN parameter value LSB CC
1011nnnn   01100101 (101)  01111111 (127)   RPN parameter number Reset MSB CC
1011nnnn   01100100 (100)  01111111 (127)   RPN parameter number Reset LSB CC

The following table lists all the NRPN input values that LinnStrument understands.

NRPN   Value   Description
-------------------------------------------------------------------------------------------------------------
0      0-2     Split Left MIDI Mode (0: One Channel, 1: Channel Per Note, 2: Channel Per Row)
1      1-16    Split Left MIDI Main Channel
2      0-1     Split Left MIDI Per Note Channel 1 (0: Off, 1: On)
3      0-1     Split Left MIDI Per Note Channel 2 (0: Off, 1: On)
4      0-1     Split Left MIDI Per Note Channel 3 (0: Off, 1: On)
5      0-1     Split Left MIDI Per Note Channel 4 (0: Off, 1: On)
6      0-1     Split Left MIDI Per Note Channel 5 (0: Off, 1: On)
7      0-1     Split Left MIDI Per Note Channel 6 (0: Off, 1: On)
8      0-1     Split Left MIDI Per Note Channel 7 (0: Off, 1: On)
9      0-1     Split Left MIDI Per Note Channel 8 (0: Off, 1: On)
10     0-1     Split Left MIDI Per Note Channel 9 (0: Off, 1: On)
11     0-1     Split Left MIDI Per Note Channel 10 (0: Off, 1: On)
12     0-1     Split Left MIDI Per Note Channel 11 (0: Off, 1: On)
13     0-1     Split Left MIDI Per Note Channel 12 (0: Off, 1: On)
14     0-1     Split Left MIDI Per Note Channel 13 (0: Off, 1: On)
15     0-1     Split Left MIDI Per Note Channel 14 (0: Off, 1: On)
16     0-1     Split Left MIDI Per Note Channel 15 (0: Off, 1: On)
17     0-1     Split Left MIDI Per Note Channel 16 (0: Off, 1: On)
18     1-16    Split Left MIDI Per Row Lowest Channel
19     1-96    Split Left MIDI Bend Range
20     0-1     Split Left Send X (0: Off, 1: On)
21     0-1     Split Left Pitch Quantize (0: Off, 1: On)
22     0-3     Split Left Pitch Quantize Hold (0: Off, 1: Medium, 2: Fast, 3: Slow)
23     0-1     Split Left Pitch Reset On Release (unprinted parameter, 0: Off, 1: On)
24     0-1     Split Left Send Y (0: Off, 1: On)
25     0-127   Split Left MIDI CC For Y (CC 1 or CC 74 are recommended, any CC is possible though)
26     0-1     Split Left Relative Y (0: Off, 1: On)
27     0-1     Split Left Send Z (0: Off, 1: On)
28     0-2     Split Left MIDI Expression For Z (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #29) 
29     0-127   Split Left MIDI CC For Z (CC 11 is recommended, any CC is possible though)
30     1-6     Split Left Color Main (see color value table below)
31     1-6     Split Left Color Accent (see color value table below)
32     0-6     Split Left Color Played (see color value table below)
33     1-6     Split Left Color LowRow (see color value table below)
34     0-7     Split Left LowRow Mode (0: Off, 1: Sust, 2: Restr, 3: Strum, 4: Arp, 5: Bend, 6: CC1, 7: CC16-18)
35     0-4     Split Left Special (0: Off, 1: Arpeg, 2: CC Faders, 3: Strum, 4: Sequencer)
36     0-10    Split Left Octave (0: —5, 1: -4, 2: -3, 3: -2, 4: -1, 5: 0, 6: +1, 7: +2, 8: +3, 9: +4. 10: +5)
37     0-14    Split Left Transpose Pitch (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)
38     0-14    Split Left Transpose Lights (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)
39     0-2     Split Left MIDI Expression For Y (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #25) 
40     0-128   Split Left MIDI CC For Fader 1 (0-127: CC, 128: Channel Aftertouch) 
41     0-128   Split Left MIDI CC For Fader 2 (0-127: CC, 128: Channel Aftertouch) 
42     0-128   Split Left MIDI CC For Fader 3 (0-127: CC, 128: Channel Aftertouch) 
43     0-128   Split Left MIDI CC For Fader 4 (0-127: CC, 128: Channel Aftertouch) 
44     0-128   Split Left MIDI CC For Fader 5 (0-127: CC, 128: Channel Aftertouch) 
45     0-128   Split Left MIDI CC For Fader 6 (0-127: CC, 128: Channel Aftertouch) 
46     0-128   Split Left MIDI CC For Fader 7 (0-127: CC, 128: Channel Aftertouch) 
47     0-128   Split Left MIDI CC For Fader 8 (0-127: CC, 128: Channel Aftertouch) 
48     0-1     Split Left LowRow X Behavior (0: Hold, 1: Fader)
49     0-128   Split Left MIDI CC For LowRow X (0-127: CC, 128: Channel Aftertouch) 
50     0-1     Split Left LowRow XYZ Behavior (0: Hold, 1: Fader)
51     0-128   Split Left MIDI CC For LowRow XYZ X (0-127: CC, 128: Channel Aftertouch) 
52     0-128   Split Left MIDI CC For LowRow XYZ Y (0-127: CC, 128: Channel Aftertouch) 
53     0-128   Split Left MIDI CC For LowRow XYZ Z (0-127: CC, 128: Channel Aftertouch) 
54     0-127   Split Left Minimum CC Value For Y
55     0-127   Split Left Maximum CC Value For Y
56     0-127   Split Left Minimum CC Value For Z
57     0-127   Split Left Maximum CC Value For Z
58     0-1     Split Left CC Value For Z in 14-bit
59     0-127   Split Left Initial Value For Relative Y
60     0-1     Split Left Channel Per Row MIDI Channel Order (0: Normal, 1: Reversed)
61     0-14    Split Left Touch Animation (0: Same, 1: Crosses, 2: Circles, 3: Squares, 4: Diamonds, 5: Stars, 6: Sparkles, 7: Curtains, 8: Blinds, 9: Targets, 10: Up, 11: Down, 12: Left, 13: Right, 14: Orbits)
62     1       Split Left Sequencer Toggle Play
63     1       Split Left Sequencer Previous Pattern
64     1       Split Left Sequencer Next Pattern
65     0-3     Split Left Sequencer Select Pattern Number
66     1       Split Left Sequencer Toggle Mute
100    0-2     Split Right MIDI Mode (0: One Channel, 1: Channel Per Note, 2: Channel Per Row)
101    1-16    Split Right MIDI Main Channel
102    0-1     Split Right MIDI Per Note Channel 1 (0: Off, 1: On)
103    0-1     Split Right MIDI Per Note Channel 2 (0: Off, 1: On)
104    0-1     Split Right MIDI Per Note Channel 3 (0: Off, 1: On)
105    0-1     Split Right MIDI Per Note Channel 4 (0: Off, 1: On)
106    0-1     Split Right MIDI Per Note Channel 5 (0: Off, 1: On)
107    0-1     Split Right MIDI Per Note Channel 6 (0: Off, 1: On)
108    0-1     Split Right MIDI Per Note Channel 7 (0: Off, 1: On)
109    0-1     Split Right MIDI Per Note Channel 8 (0: Off, 1: On)
110    0-1     Split Right MIDI Per Note Channel 9 (0: Off, 1: On)
111    0-1     Split Right MIDI Per Note Channel 10 (0: Off, 1: On)
112    0-1     Split Right MIDI Per Note Channel 11 (0: Off, 1: On)
113    0-1     Split Right MIDI Per Note Channel 12 (0: Off, 1: On)
114    0-1     Split Right MIDI Per Note Channel 13 (0: Off, 1: On)
115    0-1     Split Right MIDI Per Note Channel 14 (0: Off, 1: On)
116    0-1     Split Right MIDI Per Note Channel 15 (0: Off, 1: On)
117    0-1     Split Right MIDI Per Note Channel 16 (0: Off, 1: On)
118    1-16    Split Right MIDI Per Row Lowest Channel
119    1-96    Split Right MIDI Bend Range
120    0-1     Split Right Send X (0: Off, 1: On)
121    0-1     Split Right Pitch Quantize (0: Off, 1: On)
122    0-3     Split Right Pitch Quantize Hold (0: Off, 1: Medium, 2: Fast, 3: Slow)
123    0-1     Split Right Pitch Reset On Release (unprinted parameter, 0: Off, 1: On)
124    0-1     Split Right Send Y (0: Off, 1: On)
125    0-127   Split Right MIDI CC For Y (CC 1 or CC 74 are recommended, any CC is possible though)
126    0-1     Split Right Relative Y (0: Off, 1: On)
127    0-1     Split Right Send Z (0: Off, 1: On)
128    0-2     Split Right MIDI Expression For Z (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #129)
129    0-127   Split Right MIDI CC For Z (CC 11 is recommended, any CC is possible though)
130    1-6     Split Right Color Main (see color value table below)
131    1-6     Split Right Color Accent (see color value table below)
132    0-6     Split Right Color Played (see color value table below)
133    1-6     Split Right Color LowRow (see color value table below)
134    0-7     Split Right LowRow Mode (0: Off, 1: Sust, 2: Restr, 3: Strum, 4: Arp, 5: Bend, 6: CC1, 7: CC16-18)
135    0-4     Split Right Special (0: Off, 1: Arpeg, 2: CC Faders, 3: Strum, 4: Sequencer)
136    0-10    Split Right Octave (0: —5, 1: -4, 2: -3, 3: -2, 4: -1, 5: 0, 6: +1, 7: +2, 8: +3, 9: +4. 10: +5)
137    0-14    Split Right Transpose Pitch (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)
138    0-14    Split Right Transpose Lights (0-6: -7 to -1, 7: 0, 8-14: +1 to +7)
139    0-2     Split Right MIDI Expression For Y (0: Poly Aftertouch, 1: Channel Aftertouch, 2: CC defined in #125) 
140    0-128   Split Right MIDI CC For Fader 1 (0-127: CC, 128: Channel Aftertouch) 
141    0-128   Split Right MIDI CC For Fader 2 (0-127: CC, 128: Channel Aftertouch) 
142    0-128   Split Right MIDI CC For Fader 3 (0-127: CC, 128: Channel Aftertouch) 
143    0-128   Split Right MIDI CC For Fader 4 (0-127: CC, 128: Channel Aftertouch) 
144    0-128   Split Right MIDI CC For Fader 5 (0-127: CC, 128: Channel Aftertouch) 
145    0-128   Split Right MIDI CC For Fader 6 (0-127: CC, 128: Channel Aftertouch) 
146    0-128   Split Right MIDI CC For Fader 7 (0-127: CC, 128: Channel Aftertouch) 
147    0-128   Split Right MIDI CC For Fader 8 (0-127: CC, 128: Channel Aftertouch) 
148    0-1     Split Right LowRow X Behavior (0: Hold, 1: Fader)
149    0-128   Split Right MIDI CC For LowRow X (0-127: CC, 128: Channel Aftertouch) 
150    0-1     Split Right LowRow XYZ Behavior (0: Hold, 1: Fader)
151    0-128   Split Right MIDI CC For LowRow XYZ X (0-127: CC, 128: Channel Aftertouch) 
152    0-128   Split Right MIDI CC For LowRow XYZ Y (0-127: CC, 128: Channel Aftertouch) 
153    0-128   Split Right MIDI CC For LowRow XYZ Z (0-127: CC, 128: Channel Aftertouch) 
154    0-127   Split Right Minimum CC Value For Y
155    0-127   Split Right Maximum CC Value For Y
156    0-127   Split Right Minimum CC Value For Z
157    0-127   Split Right Maximum CC Value For Z
158    0-1     Split Right CC Value For Z in 14-bit
159    0-127   Split Right Initial Value For Relative Y
160    0-1     Split Right Channel Per Row MIDI Channel Order (0: Normal, 1: Reversed)
161    0-14    Split Right Touch Animation (0: Same, 1: Crosses, 2: Circles, 3: Squares, 4: Diamonds, 5: Stars, 6: Sparkles, 7: Curtains, 8: Blinds, 9: Targets, 10: Up, 11: Down, 12: Left, 13: Right, 14: Orbits)
162    1       Split Right Sequencer Toggle Play
163    1       Split Right Sequencer Previous Pattern
164    1       Split Right Sequencer Next Pattern
165    0-3     Split Right Sequencer Select Pattern Number
166    1       Split Right Sequencer Toggle Mute
200    0-1     Global Split Active (0: Inactive, 1: Active)
201    0-1     Global Selected Split (0: Left Split, 1: Right Split)
202    2-25    Global Split Point Column
203    0-1     Global Main Note Light C (0: Off, 1: On)
204    0-1     Global Main Note Light C# (0: Off, 1: On)
205    0-1     Global Main Note Light D (0: Off, 1: On)
206    0-1     Global Main Note Light D# (0: Off, 1: On)
207    0-1     Global Main Note Light E (0: Off, 1: On)
208    0-1     Global Main Note Light F (0: Off, 1: On)
209    0-1     Global Main Note Light F# (0: Off, 1: On)
210    0-1     Global Main Note Light G (0: Off, 1: On)
211    0-1     Global Main Note Light G# (0: Off, 1: On)
212    0-1     Global Main Note Light A (0: Off, 1: On)
213    0-1     Global Main Note Light A# (0: Off, 1: On)
214    0-1     Global Main Note Light B (0: Off, 1: On)
215    0-1     Global Accent Note Light C (0: Off, 1: On)
216    0-1     Global Accent Note Light C# (0: Off, 1: On)
217    0-1     Global Accent Note Light D (0: Off, 1: On)
218    0-1     Global Accent Note Light D# (0: Off, 1: On)
219    0-1     Global Accent Note Light E (0: Off, 1: On)
220    0-1     Global Accent Note Light F (0: Off, 1: On)
221    0-1     Global Accent Note Light F# (0: Off, 1: On)
222    0-1     Global Accent Note Light G (0: Off, 1: On)
223    0-1     Global Accent Note Light G# (0: Off, 1: On)
224    0-1     Global Accent Note Light A (0: Off, 1: On)
225    0-1     Global Accent Note Light A# (0: Off, 1: On)
226    0-1     Global Accent Note Light B (0: Off, 1: On)
227    0-13    Global Row Offset (only supports, 0: No overlap, 3 4 5 6 7 12: Intervals, 13: Guitar, 127: 0 offset)
228    0-17    Global Switch 1 Assignment (0: Oct Down, 1: Oct Up, 2: Sustain, 3: CC65, 4: Arp, 5: Alt Split, 6: Auto Octave, 7: Tap Tempo, 8: Legato, 9: Latch, 10: Preset Up, 11: Preset Down, 12: Reverse Pitch X, 13: Sequencer Play, 14: Sequencer Previous, 15: Sequencer Next, 16: Send MIDI Clock, 17: Sequencer Mute)
229    0-17    Global Switch 2 Assignment (0: Oct Down, 1: Oct Up, 2: Sustain, 3: CC65, 4: Arp, 5: Alt Split, 6: Auto Octave, 7: Tap Tempo, 8: Legato, 9: Latch, 10: Preset Up, 11: Preset Down, 12: Reverse Pitch X, 13: Sequencer Play, 14: Sequencer Previous, 15: Sequencer Next, 16: Send MIDI Clock, 17: Sequencer Mute)
230    0-17    Global Foot Left Assignment (0: Oct Down, 1: Oct Up, 2: Sustain, 3: CC65, 4: Arp, 5: Alt Split, 6: Auto Octave, 7: Tap Tempo, 8: Legato, 9: Latch, 10: Preset Up, 11: Preset Down, 12: Reverse Pitch X, 13: Sequencer Play, 14: Sequencer Previous, 15: Sequencer Next, 16: Send MIDI Clock, 17: Sequencer Mute)
231    0-17    Global Foot Right Assignment (0: Oct Down, 1: Oct Up, 2: Sustain, 3: CC65, 4: Arp, 5: Alt Split, 6: Auto Octave, 7: Tap Tempo, 8: Legato, 9: Latch, 10: Preset Up, 11: Preset Down, 12: Reverse Pitch X, 13: Sequencer Play, 14: Sequencer Previous, 15: Sequencer Next, 16: Send MIDI Clock, 17: Sequencer Mute)
232    0-3     Global Velocity Sensitivity (0: Low, 1: Medium, 2: High, 3: Fixed)
233    0-2     Global Pressure Sensitivity (0: Low, 1: Medium, 2: High)
234    0-1     Device MIDI I/O (0: MIDI Jacks, 1: USB)
235    0-4     Global Arp Direction (0: Up, 1: Down, 2: Up Down, 3: Random, 4: Replay All)
236    1-7     Global Arp Tempo Note Value (1: 8, 2: 8 Tri, 3: 16, 4: 16 Swing, 5: 16 Tri, 6: 32, 7: 32 Tri)
237    0-2     Global Arp Octave Extension (0: None, 1: +1, 2: +2)
238    1-360   Global Clock BPM (applies when receiving no MIDI clock)
239    0-1     Global Switch 1 Both Splits (0: Off, 1: On)
240    0-1     Global Switch 2 Both Splits (0: Off, 1: On)
241    0-1     Global Foot Left Both Splits (0: Off, 1: On)
242    0-1     Global Foot Right Both Splits (0: Off, 1: On)
243    0-5     Global Settings Preset Load
244    0-1     Global Pressure Aftertouch (0: Off, 1: On)
245    0-1     Device User Firmware Mode (0: Off, 1: On)
246    0-1     Device Left Handed Operation (0: Off, 1: On)
247    0-11    Global Active Note Lights Preset
248    0-127   Global MIDI CC For Switch CC65 (Changes the CC for all switches - Legacy option, see NRPN 255-258)
249    1-127   Global Minimum Value For Velocity
250    1-127   Global Maximum Value For Velocity
251    1-127   Global Value For Fixed Velocity
252    0-512   Device Minimum Interval Between MIDI Bytes Over USB
253    0-33    Global Custom Row Offset Instead Of Octave (0-32: -16-16 semitone intervals, 33: inverted Guitar)
254    0-1     Device MIDI Through (0: Off, 1: On)
255    0-127   Global MIDI CC For Foot Left CC65
256    0-127   Global MIDI CC For Foot Right CC65
257    0-127   Global MIDI CC For Switch 1 CC65
258    0-127   Global MIDI CC For Switch 2 CC65
259    0-127   Global MIDI CC For Foot Left Sustain
260    0-127   Global MIDI CC For Foot Right Sustain
261    0-127   Global MIDI CC For Switch 1 Sustain
262    0-127   Global MIDI CC For Switch 2 Sustain
263    0-127   Global Note Number For Guitar Tuning Row 1
264    0-127   Global Note Number For Guitar Tuning Row 2
265    0-127   Global Note Number For Guitar Tuning Row 3
266    0-127   Global Note Number For Guitar Tuning Row 4
267    0-127   Global Note Number For Guitar Tuning Row 5
268    0-127   Global Note Number For Guitar Tuning Row 6
269    0-127   Global Note Number For Guitar Tuning Row 7
270    0-127   Global Note Number For Guitar Tuning Row 8
299    <any>   Send the current value of a particular NRPN configuration parameter, when possible

Color Values
============
0   as set in Note Lights settings
1   Red
2   Yellow
3   Green
4   Cyan
5   Blue
6   Magenta
7   Off
8   White
9   Orange
10  Lime
11  Pink