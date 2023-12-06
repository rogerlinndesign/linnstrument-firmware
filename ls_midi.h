/**************************** ls_midi: LinnStrument MIDI status codes *****************************
Copyright 2023 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
***************************************************************************************************
The definitions of the MIDI status codes that are used by the LinnStrument MIDI code.
**************************************************************************************************/

#ifndef MIDI_H_
#define MIDI_H_

enum MIDIStatus {
  // Channel Voice Messages
  MIDINoteOff               = B10000000,     // Note Off
  MIDINoteOn                = B10010000,     // Note On
  MIDIPolyphonicPressure    = B10100000,     // Polyphonic Key Pressure
  MIDIControlChange         = B10110000,     // Control Change / Channel Mode
  MIDIProgramChange         = B11000000,     // Program Change
  MIDIChannelPressure       = B11010000,     // Channel Pressure
  MIDIPitchBend             = B11100000,     // Pitch Bend Change
  // System Common Messages
  MIDISystemExclusive       = B11110000,     // System Exclusive
  MIDITimeCodeQuarterFrame  = B11110001,     // MIDI Time Code Quarter Frame
  MIDISongPositionPointer   = B11110010,     // Song Position Pointer
  MIDISongSelect            = B11110011,     // Song Select
  MIDIUndefined1            = B11110100,     // Undefined
  MIDIUndefined2            = B11110101,     // Undefined
  MIDITuneRequest           = B11110110,     // Tune Request
  MIDIEndOfExclusive        = B11110111,     // End of Exclusive
  // System Real-Time Messages
  MIDITimingClock           = B11111000,     // Timing Clock
  MIDIUndefined3            = B11111001,     // Undefined
  MIDIStart                 = B11111010,     // Start
  MIDIContinue              = B11111011,     // Continue
  MIDIStop                  = B11111100,     // Stop
  MIDIUndefined4            = B11111101,     // Undefined
  MIDIActiveSensing         = B11111110,     // Active Sensing
  MIDIReset                 = B11111111      // Reset
};

#endif