/**************************** ls_midi: LinnStrument MIDI status codes *****************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
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
  MIDITuneRequest           = B11110110,     // Tune Request
  MIDIEndOfExclusive        = B11110111,     // End of Exclusive
  // System Real-Time Messages
  MIDITimingClock           = B11111000,     // Timing Clock
  MIDIStart                 = B11111010,     // Start
  MIDIContinue              = B11111011,     // Continue
  MIDIStop                  = B11111100,     // Stop
  MIDIActiveSensing         = B11111110,     // Active Sensing
  MIDIReset                 = B11111111      // Reset
};

#endif