/********************************** ls_midi: LinnStrument MIDI ************************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These are the MIDI functions for the LinnStrument
**************************************************************************************************/

#include "ls_bytebuffer.h"
#include "ls_midi.h"

// first byte is the status byte, the two following bytes are the data bytes and
// the channel will be encoded in the 4th byte if applicable
byte midiMessage[4];
byte midiMessageBytes = 0; // the number of bytes that are expected in the message that is being constituted
byte midiMessageIndex = 0; // the message array index of the message that is being constituted

byte midiCellColCC = 0;
byte midiCellRowCC = 0;

// MIDI Clock State
const int32_t fxd4MidiClockUnit = FXD4_FROM_INT(2500000);  // 1000000 ( microsecond) * 60 ( minutes - bpm) / 24 ( frames per beat)
const int32_t fxd4MidiClockSamples = FXD4_FROM_INT(6);

boolean midiClockStarted = false;                          // indicates whether the MIDI clock transport is running
unsigned long lastMidiClockTime = 0;                       // the last time we received a MIDI clock message in micros
int32_t fxd4MidiTempoAverage = fxd4CurrentTempo;           // the current average of the MIDI clock tempo, in fixes precision
byte midiClockMessageCount = 0;                            // the number of MIDI clock messages we've received, from 1 to 24, with 0 meaning none has been received yet
unsigned long midiClockLedOn = 0;                          // indicates when the MIDI clock led was turned on

byte lastRpnMsb = 127;
byte lastRpnLsb = 127;
byte lastNrpnMsb = 127;
byte lastNrpnLsb = 127;
byte lastDataMsb = 0;
byte lastDataLsb = 0;

boolean isMidiUsingDIN() {
  return Global.midiIO == 0;
}

void applyMidiIo() {
  if (isMidiUsingDIN()) {
    digitalWrite(36, 0);     // Set LOW for DIN jacks
    Serial.begin(31250);     // set serial port at MIDI DIN speed 31250
    Serial.flush();          // clear the serial port
  }
  else {
    digitalWrite(36, 1);     // Set HIGH for USB
    Serial.begin(115200);    // set serial port at fastest speed 115200
    Serial.flush();          // clear the serial port
  }
}

void handleMidiInput(unsigned long now) {
  // handle turning off the MIDI clock led after minimum 30ms
  if (midiClockLedOn != 0 && calcTimeDelta(now, midiClockLedOn) > LED_FLASH_DELAY) {
    midiClockLedOn = 0;
    clearLed( 0, 0);
  }

  // if no serial data is available, return
  if (Serial.available() <= 0) {
    return;
  }

  // get the next byte from the serial bus
  byte d = Serial.read();

  // check if we're dealing with a status byte
  if ((d & B10000000) == B10000000) {
    midiMessage[0] = 0;
    midiMessageBytes = 0;
    midiMessageIndex = 0;

    switch (d) {
      case MIDIActiveSensing:
        // indicate MIDI activity sensing in test mode
        if (operatingMode == modeManufacturingTest) {
          setLed(25, 2, COLOR_GREEN, cellOn);
        }
        break;
      case MIDIStart:
      case MIDIContinue:
        midiClockStarted = true;
        resetArpeggiatorAdvancement(now);
        lastMidiClockTime = 0;
        break;
      case MIDIStop:
        midiClockStarted = false;
        midiClockMessageCount = 0;
        lastMidiClockTime = 0;
        resetArpeggiatorAdvancement(now);
        break;
      case MIDISongPositionPointer:
        midiMessage[0] = MIDISongPositionPointer;
        midiMessageBytes = 3;
        midiMessageIndex = 1;
        lastMidiClockTime = 0;
        break;
      case MIDITimingClock:
      {
        if (midiClockStarted) {
          if (lastMidiClockTime > 0) {
            unsigned long clockDelta;
            // check if the micros timer has overflown
            if (now < lastMidiClockTime) {
              clockDelta = now + ~lastMidiClockTime;
            }
            // otherwise simply substract
            else {
              clockDelta = now - lastMidiClockTime;
            }

            if (clockDelta > 0) {
              fxd4MidiTempoAverage -= FXD4_DIV(fxd4MidiTempoAverage, fxd4MidiClockSamples);
              fxd4MidiTempoAverage += FXD4_DIV(FXD4_DIV(fxd4MidiClockUnit, FXD4_FROM_INT(int(clockDelta))), fxd4MidiClockSamples);
              fxd4CurrentTempo = fxd4MidiTempoAverage;
            }
          }

          lastMidiClockTime = now;

          midiClockMessageCount++;
          if (midiClockMessageCount == 25) {
            midiClockMessageCount = 1;
          }

          // flash the global settings led green on tempo, unless it's currently pressed down
          if (controlButton != 0 && midiClockMessageCount == 1) {
            setLed(0, GLOBAL_SETTINGS_ROW, COLOR_GREEN, cellOn);
            midiClockLedOn = now;
          }

          // play the next arpeggiator step if needed
          checkAdvanceArpeggiator(now);

          // flash the tempo led in the global display when it is on
          updateGlobalSettingsFlashTempo(now);
        }
        break;
      }
      default:
      {
        byte channelStatus = (d & B11110000); // remove channel nibble
        switch (channelStatus) {
          case MIDINoteOff:
          case MIDINoteOn:
          case MIDIControlChange:
            midiMessage[0] = channelStatus;
            midiMessage[3] = (d & B00001111);
            midiMessageBytes = 3;
            midiMessageIndex = 1;
            break;
          case MIDIProgramChange:
            midiMessage[0] = channelStatus;
            midiMessage[3] = (d & B00001111);
            midiMessageBytes = 2;
            midiMessageIndex = 1;
            break;
        }
        break;
      }
    }
  }
  // otherwise this is a data byte
  else if (midiMessageBytes) {
    midiMessage[midiMessageIndex++] = d;
  }

  // we have all the bytes we need for the message that is being constituted
  if (midiMessageBytes && midiMessageIndex == midiMessageBytes) {
    if (midiMessage[0] == MIDISongPositionPointer) {
      unsigned pos = midiMessage[2] << 7 | midiMessage[1];
      midiClockMessageCount = (pos * 6) % 24 + 1;
    }
    else {
      byte midiStatus = midiMessage[0];
      byte midiChannel = midiMessage[3];
      byte midiData1 = midiMessage[1];
      byte midiData2 = midiMessage[2];

      int split = determineSplitForChannel(midiChannel);

      switch (midiStatus) {
        case MIDINoteOn:
        {
          // velocity 0 means the same as note off, so don't handle it further in this case
          if (midiData2 > 0) {
            if (split != -1 && (split == LEFT || splitActive)) {
              // attempts to highlight the exact cell that belongs to a midi note and channel
              if (!highlightExactNoteCell(split, midiData1, midiChannel)) {
                // if there's not one exact location, we highlight all cells that could correspond to the note number
                highlightPossibleNoteCells(split, midiData1);
              }
            }
            break;
          }
          // purposely fall-through in case of velocity 0
        }

        case MIDINoteOff:
        {
          if (split != -1 && (split == LEFT || splitActive)) {
            // attempts to reset the exact cell that belongs to a midi note and channel
            if (!resetExactNoteCell(split, midiData1, midiChannel)) {
              // if there's not one exact location, we reset all cells that could correspond to the note number
            resetPossibleNoteCells(split, midiData1);
            }
          }
          break;
        }

        case MIDIProgramChange:
        {
          if (split != -1) {
            midiPreset[split] = midiData1;
            if (displayMode == displayPreset) {
              updateDisplay();
            }
          }
          break;
        }

        case MIDIControlChange:
        {
          switch (midiData1) {
            case 6:
              // if an NRPN or RPN parameter was selected, start constituting the data
              // otherwise control the fader of MIDI CC 6
              if ((lastRpnMsb != 127 || lastRpnLsb != 127) ||
                  (lastNrpnMsb != 127 || lastNrpnLsb != 127)) {
                lastDataMsb = midiData2;
                break;
              }
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 7:
            case 8:
              if (split != -1) {
                unsigned short ccForFader = Split[split].ccForFader[midiData1-1];
                ccFaderValues[split][ccForFader] = midiData2;
                if ((displayMode == displayNormal && Split[split].ccFaders) ||
                    displayMode == displayVolume) {
                  updateDisplay();
                }
              }
              break;
            case 9:
              if (userFirmwareActive && midiChannel < NUMROWS && (midiData2 == 0 || midiData2 == 1)) {
                userFirmwareSlideMode[midiChannel] = midiData2;
              }
              break;
            case 10:
              if (userFirmwareActive && midiChannel < NUMROWS && (midiData2 == 0 || midiData2 == 1)) {
                userFirmwareXActive[midiChannel] = midiData2;
              }
              break;
            case 11:
              if (userFirmwareActive && midiChannel < NUMROWS && (midiData2 == 0 || midiData2 == 1)) {
                userFirmwareYActive[midiChannel] = midiData2;
              }
              break;
            case 12:
              if (userFirmwareActive && midiChannel < NUMROWS && (midiData2 == 0 || midiData2 == 1)) {
                userFirmwareZActive[midiChannel] = midiData2;
              }
              break;
            case 13:
              if (userFirmwareActive && (!Device.operatingLowPower || midiData2 > LOWPOWER_MIDI_DECIMATION)) {
                midiDecimateRate = midiData2;
              }
              break;
            case 20:
              if (midiData2 < NUMCOLS) {
                midiCellColCC = midiData2;
              }
              break;
            case 21:
              if (midiData2 < NUMROWS) {
                midiCellRowCC = midiData2;
              }
              break;
            case 22:
              if (displayMode == displayNormal) {
                byte layer = LED_LAYER_CUSTOM1;
                // we light the LEDs of user firmware mode in a dedicated custom layer
                // this will be cleared when switching back to regular firmware mode
                if (userFirmwareActive) {
                  layer = LED_LAYER_CUSTOM2;
                }
                if (midiData2 <= COLOR_BLACK && midiData2 != COLOR_OFF) {
                  setLed(midiCellColCC, midiCellRowCC, midiData2, cellOn, layer);
                }
                else {
                  setLed(midiCellColCC, midiCellRowCC, COLOR_OFF, cellOff, layer);
                }
                checkRefreshLedColumn(micros());
              }
              break;
            case 38:
              if (lastRpnMsb != 127 || lastRpnLsb != 127) {
                lastDataLsb = midiData2;
                receivedRpn((lastRpnMsb<<7)+lastRpnLsb, (lastDataMsb<<7)+lastDataLsb);
                break;
              }
              if (lastNrpnMsb != 127 || lastNrpnLsb != 127) {
                lastDataLsb = midiData2;
                receivedNrpn((lastNrpnMsb<<7)+lastNrpnLsb, (lastDataMsb<<7)+lastDataLsb);
                break;
              }
            case 98:
              lastNrpnLsb = midiData2;
              break;
            case 99:
              lastNrpnMsb = midiData2;
              break;
            case 100:
              lastRpnLsb = midiData2;
              // resetting RPN numbers also resets NRPN numbers
              if (lastRpnLsb == 127 && lastRpnMsb == 127) {
                lastNrpnLsb = 127;
                lastNrpnMsb = 127;
              }
              break;
            case 101:
              lastRpnMsb = midiData2;
              break;
            case 127:
              // support for activating MPE mode with the standard MPE message
              if (midiChannel == 0 || midiChannel == 15) {
                byte split = LEFT;
                if (midiChannel == 15) {
                  split = RIGHT;
                }

                if (midiData2 == 0) {
                  disableMpe(split);
                }
                else {
                  enableMpe(split, midiChannel + 1, midiData2);
                }

                updateDisplay();
              }
              break;
          }
        }
      }
    }

    // reset the message
    midiMessage[0] = 0;
    midiMessageBytes = 0;
    midiMessageIndex = 0;
  }
}

signed char determineSplitForChannel(byte channel) {
  if (channel > 15) {
    return -1;
  }

  for (byte split = LEFT; split <= RIGHT; ++split) {
    switch (Split[split].midiMode) {
      case oneChannel:
        if (Split[split].midiChanMain-1 == channel) {
          return split;
        }
        break;
      case channelPerNote:
        if (Split[split].midiChanSet[channel] == true) {
          return split;
        }
        break;
      case channelPerRow:
        if (calculateRowPerChannelRow(split, channel) < NUMROWS) {
          return split;
        }
        break;
    }
  }

  return -1;
}

inline boolean inRange(int value, int lower, int upper) {
  return value >= lower && value <= upper;
}

void receivedRpn(int parameter, int value) {
  switch (parameter) {
    // Pitch Bend Sensitivity
    case 0:
      applyBendRange(Split[LEFT], constrain(value, 1, 96));
      applyBendRange(Split[RIGHT], constrain(value, 1, 96));
      break;
  }

  updateDisplay();
}

void receivedNrpn(int parameter, int value) {
  byte split = LEFT;
  if (parameter >= 100 && parameter < 200) {
    parameter -= 100;
    split = RIGHT;
  }

  switch (parameter) {
    // Split MIDI Mode
    case 0:
      if (inRange(value, 0, 2)) {
        Split[split].midiMode = value;
        // ensure MPE is turned off
        disableMpe(split);
      }
      break;
    // Split MIDI Main Channel
    case 1:
      if (inRange(value, 1, 16)) {
        Split[split].midiChanMain = value;
        // ensure MPE is turned off
        disableMpe(split);
      }
    // Split MIDI Per Note Channels
    case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
    case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
      if (inRange(value, 0, 1)) {
        Split[split].midiChanSet[parameter-2] = value;
        // ensure MPE is turned off
        disableMpe(split);
      }
      break;
    // Split MIDI Per Row Lowest Channel
    case 18:
      if (inRange(value, 1, 16)) {
        Split[split].midiChanPerRow = value;
      }
      break;
    // Split MIDI Bend Range
    case 19:
      if (inRange(value, 1, 96)) {
        applyBendRange(Split[split], value);
      }
      break;
    // Split Send X
    case 20:
      if (inRange(value, 0, 1)) {
        Split[split].sendX = value;
      }
      break;
    // Split Pitch Quantize
    case 21:
      if (inRange(value, 0, 1)) {
        Split[split].pitchCorrectQuantize = value;
      }
      break;
    // Split Pitch Quantize Hold
    case 22:
      if (inRange(value, 0, 3)) {
        Split[split].pitchCorrectHold = value;
        applyPitchCorrectHold();
      }
      break;
    // Split Pitch Reset On Release
    case 23:
      if (inRange(value, 0, 1)) {
        Split[split].pitchResetOnRelease = value;
      }
      break;
    // Split Send Y
    case 24:
      if (inRange(value, 0, 1)) {
        Split[split].sendY = value;
      }
      break;
    // Split MIDI CC For Y
    case 25:
      if (inRange(value, 0, 127)) {
        if (Split[split].expressionForY == timbreCC1 && value != 1) {
          Split[split].expressionForY == timbreCC74;
        }
        Split[split].customCCForY = value;
      }
      break;
    // Split Relative Y
    case 26:
      if (inRange(value, 0, 1)) {
        Split[split].relativeY = value;
      }
      break;
    // Split Send Z
    case 27:
      if (inRange(value, 0, 1)) {
        Split[split].sendZ = value;
      }
      break;
    // Split MIDI Expression For Z
    case 28:
      if (inRange(value, 0, 2)) {
        Split[split].expressionForZ = (LoudnessExpression)value;
      }
      break;
    // Split MIDI CC For Z
    case 29:
      if (inRange(value, 0, 127)) {
        Split[split].customCCForZ = value;
      }
      break;
    // Split Color Main
    case 30:
      if (inRange(value, 1, 6)) {
        Split[split].colorMain = value;
      }
      break;
    // Split Color Accent
    case 31:
      if (inRange(value, 1, 6)) {
        Split[split].colorAccent = value;
      }
      break;
    // Split Color Played
    case 32:
      if (inRange(value, 0, 6)) {
        Split[split].colorNoteon = value;
      }
      break;
    // Split Color LowRow
    case 33:
      if (inRange(value, 1, 6)) {
        Split[split].colorLowRow = value;
      }
      break;
    // Split LowRow Mode
    case 34:
      if (inRange(value, 0, 7)) {
        Split[split].lowRowMode = value;
      }
      break;
    // Split Special
    case 35:
      if (inRange(value, 0, 3)) {
        switch (value) {
          case 0:
            Split[split].arpeggiator = false;
            Split[split].ccFaders = false;
            Split[split].strum = false;
            break;
          case 1:
            Split[split].arpeggiator = true;
            Split[split].ccFaders = false;
            Split[split].strum = false;
            break;
          case 2:
            Split[split].arpeggiator = false;
            Split[split].ccFaders = true;
            Split[split].strum = false;
            break;
          case 3:
            Split[split].arpeggiator = false;
            Split[split].ccFaders = false;
            Split[split].strum = true;
            break;
        }
      }
      break;
    // Split Octave
    case 36:
      if (inRange(value, 0, 10)) {
        Split[split].transposeOctave = (value-5)*12;
      }
      break;
    // Split Transpose Pitch
    case 37:
      if (inRange(value, 0, 14)) {
        Split[split].transposePitch = value-7;
      }
      break;
    // Split Transpose Lights
    case 38:
      if (inRange(value, 0, 14)) {
        Split[split].transposeLights = value-7;
      }
      break;
    // Split MIDI Expression For Y
    case 39:
      if (inRange(value, 0, 2)) {
        Split[split].expressionForY = (TimbreExpression)value;
        if (Split[split].expressionForY == timbrePolyPressure) {
          Split[split].customCCForY = 128;
        }
        else if (Split[split].expressionForY == timbreChannelPressure) {
          Split[split].customCCForY = 129;
        }
        else {
          if (Split[split].customCCForY == 1) {
            Split[split].expressionForY = timbreCC1;
          }
          else {
            Split[split].expressionForY = timbreCC74;
          }
        }
      }
      break;
    // Split MIDI CC For Fader 1
    case 40:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[0] = value;
      }
      break;
    // Split MIDI CC For Fader 2
    case 41:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[1] = value;
      }
      break;
    // Split MIDI CC For Fader 3
    case 42:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[2] = value;
      }
      break;
    // Split MIDI CC For Fader 4
    case 43:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[3] = value;
      }
      break;
    // Split MIDI CC For Fader 5
    case 44:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[4] = value;
      }
      break;
    // Split MIDI CC For Fader 6
    case 45:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[5] = value;
      }
      break;
    // Split MIDI CC For Fader 7
    case 46:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[6] = value;
      }
      break;
    // Split MIDI CC For Fader 8
    case 47:
      if (inRange(value, 0, 127)) {
        Split[split].ccForFader[7] = value;
      }
      break;
    // Split LowRow X Behavior
    case 48:
      if (inRange(value, 0, 1)) {
        Split[split].lowRowCCXBehavior = (LowRowCCBehavior)value;
      }
      break;
    // Split MIDI CC For LowRow X
    case 49:
      if (inRange(value, 0, 127)) {
        Split[split].ccForLowRow = value;
      }
      break;
    // Split LowRow XYZ Behavior
    case 50:
      if (inRange(value, 0, 1)) {
        Split[split].lowRowCCXYZBehavior = (LowRowCCBehavior)value;
      }
      break;
    // Split MIDI CC For LowRow XYZ X
    case 51:
      if (inRange(value, 0, 127)) {
        Split[split].ccForLowRowX = value;
      }
      break;
    // Split MIDI CC For LowRow XYZ Y
    case 52:
      if (inRange(value, 0, 127)) {
        Split[split].ccForLowRowY = value;
      }
      break;
    // Split MIDI CC For LowRow XYZ Z
    case 53:
      if (inRange(value, 0, 127)) {
        Split[split].ccForLowRowZ = value;
      }
      break;
    // Split Minimum CC Value For Y
    case 54:
      if (inRange(value, 0, 127)) {
        Split[split].minForY = value;
        applyLimitsForY();
      }
      break;
    // Split Maximum CC Value For Y
    case 55:
      if (inRange(value, 0, 127)) {
        Split[split].maxForY = value;
        applyLimitsForY();
      }
      break;
    // Split Minimum CC Value For Z
    case 56:
      if (inRange(value, 0, 127)) {
        Split[split].minForZ = value;
        applyLimitsForZ();
      }
      break;
    // Split Maximum CC Value For Z
    case 57:
      if (inRange(value, 0, 127)) {
        Split[split].maxForZ = value;
        applyLimitsForZ();
      }
      break;
    // Global Split Active
    case 200:
      if (inRange(value, 0, 1)) {
        splitActive = value;
      }
      break;
    // Global Selected Split
    case 201:
      if (inRange(value, 0, 1)) {
        Global.currentPerSplit = value;
      }
      break;
    // Global Split Point Column
    case 202:
      if (inRange(value, 2, 25)) {
        Global.splitPoint = value;
      }
      break;
    // Global Main Note Lights
    case 203: case 204: case 205: case 206: case 207: case 208:
    case 209: case 210: case 211: case 212: case 213: case 214:
      if (inRange(value, 0, 1)) {
        if (value) {
          Global.mainNotes[Global.activeNotes] |= 1 << parameter-203;
        }
        else {
          Global.mainNotes[Global.activeNotes] &= ~(1 << parameter-203);
        }
      }
      break;
    // Global Accent Note Lights
    case 215: case 216: case 217: case 218: case 219: case 220:
    case 221: case 222: case 223: case 224: case 225: case 226:
      if (inRange(value, 0, 1)) {
        if (value) {
          Global.accentNotes[Global.activeNotes] |= 1 << parameter-215;
        }
        else {
          Global.accentNotes[Global.activeNotes] &= ~(1 << parameter-215);
        }
      }
      break;
    // Global Row Offset
    case 227:
      if (value == 0 || value == 3 || value == 4 || value == 5 || value == 6 || value == 7 || value == 12 || value == 13) {
        Global.rowOffset = value;
      }
      break;
    // Global Switch 1 Assignment
    case 228:
      if (inRange(value, 0, 6)) {
        Global.switchAssignment[3] = value;
      }
      break;
    // Global Switch 2 Assignment
    case 229:
      if (inRange(value, 0, 6)) {
        Global.switchAssignment[2] = value;
      }
      break;
    // Global Foot Left Assignment
    case 230:
      if (inRange(value, 0, 6)) {
        Global.switchAssignment[0] = value;
      }
      break;
    // Global Foot Right Assignment
    case 231:
      if (inRange(value, 0, 6)) {
        Global.switchAssignment[1] = value;
      }
      break;
    // Global Velocity Sensitivity
    case 232:
      if (inRange(value, 0, 3)) {
        Global.velocitySensitivity = (VelocitySensitivity)value;
      }
      break;
    // Global Pressure Sensitivity
    case 233:
      if (inRange(value, 0, 2)) {
        Global.pressureSensitivity = (PressureSensitivity)value;
      }
      break;
    // Global MIDI I/O
    case 234:
      if (inRange(value, 0, 1)) {
        changeMidiIO(value);
      }
      break;
    // Global Arp Direction
    case 235:
      if (inRange(value, 0, 4)) {
        Global.arpDirection = (ArpeggiatorDirection)value;
      }
      break;
    // Global Arp Tempo Note Value
    case 236:
      if (inRange(value, 1, 7)) {
        Global.arpTempo = (ArpeggiatorStepTempo)value;
      }
      break;
    // Global Arp Octave Extension
    case 237:
      if (inRange(value, 0, 2)) {
        Global.arpOctave = value;
      }
      break;
    // Global Clock BPM
    case 238:
      if (inRange(value, 1, 360)) {
        fxd4CurrentTempo = FXD4_FROM_INT(value);
      }
      break;
    // Global Switch 1 Both Splits
    case 239:
      if (inRange(value, 0, 1)) {
        Global.switchBothSplits[3] = value;
      }
      break;
    // Global Switch 2 Both Splits
    case 240:
      if (inRange(value, 0, 1)) {
        Global.switchBothSplits[2] = value;
      }
      break;
    // Global Foot Left Both Splits
    case 241:
      if (inRange(value, 0, 1)) {
        Global.switchBothSplits[0] = value;
      }
      break;
    // Global Foot Right Both Splits
    case 242:
      if (inRange(value, 0, 1)) {
        Global.switchBothSplits[1] = value;
      }
      break;
    // Global Settings Preset Load
    case 243:
      if (inRange(value, 0, 3)) {
        applyPresetSettings(config.preset[value]);
      }
      break;
    // Global Pressure Aftertouch Active
    case 244:
      if (inRange(value, 0, 1)) {
        Global.pressureAftertouch = value;
      }
      break;
    // User Firmware Mode Active
    case 245:
      if (inRange(value, 0, 1)) {
        changeUserFirmwareMode(value);
      }
      break;
    // Left Handed Operation Active
    case 246:
      if (inRange(value, 0, 1)) {
        Device.leftHanded = value;
        completelyRefreshLeds();
        updateDisplay();
      }
      break;
    // Active note lights preset
    case 247:
      if (inRange(value, 0, 11)) {
        Global.activeNotes = value;
        completelyRefreshLeds();
        updateDisplay();
      }
      break;
    // Global MIDI CC For Switch CC65
    case 248:
      if (inRange(value, 0, 127)) {
        Global.ccForSwitch = value;
      }
      break;
    // Global Minimum Value For Velocity
    case 249:
      if (inRange(value, 0, 127)) {
        Global.minForVelocity = value;
        applyLimitsForVelocity();
      }
      break;
    // Global Maximum Value For Velocity
    case 250:
      if (inRange(value, 0, 127)) {
        Global.maxForVelocity = value;
        applyLimitsForVelocity();
      }
      break;
  }

  updateDisplay();
}

boolean isMidiClockRunning() {
  return midiClockStarted;
}

short getMidiClockCount() {
  return midiClockMessageCount - 1;
}

boolean highlightExactNoteCell(byte split, byte notenum, byte channel) {
  if (userFirmwareActive) return false;
  if (displayMode != displayNormal) return false;
  if (Split[split].midiMode != channelPerRow) return false;

  byte row = calculateRowPerChannelRow(split, channel);
  if (row < NUMROWS &&                                            // it's not possible to display cells on rows that don't exist
      (Split[split].lowRowMode == lowRowNormal || row != 0)) {    // it's not possible to display cells on the low row if it's active

    short col = getNoteNumColumn(split, notenum, row);
    if (col > 0) {
      setLed(col, row, Split[split].colorNoteon, cellOn, LED_LAYER_PLAYED);
    }
  }

  return true;
}

byte calculateRowPerChannelRow(byte split, byte channel) {
  // calculate the row that corresponds to the incoming MIDI channel and
  // the active split MIDI Channel Per Row configuration
  byte row = 0;
  byte basechan = Split[split].midiChanPerRow-1;
  if (channel >= basechan) {
    row = channel - basechan;
  }
  else {
    row = (channel + 16) - basechan;
  }

  return row;
}

void highlightPossibleNoteCells(byte split, byte notenum) {
  if (userFirmwareActive) return;
  if (displayMode != displayNormal) return;

  byte row = 0;
  if (Split[split].lowRowMode != lowRowNormal) {
    row = 1;
  }
  for (; row < NUMROWS; ++row) {
    short col = getNoteNumColumn(split, notenum, row);
    if (col > 0) {
      setLed(col, row, Split[split].colorNoteon, cellOn, LED_LAYER_PLAYED);
    }
  }
}

boolean resetExactNoteCell(byte split, byte notenum, byte channel) {
  if (userFirmwareActive) return false;
  if (displayMode != displayNormal) return false;
  if (Split[split].midiMode != channelPerRow) return false;

  byte row = calculateRowPerChannelRow(split, channel);
  if (row < NUMROWS &&                                            // it's not possible to display cells on rows that don't exist
      (Split[split].lowRowMode == lowRowNormal || row != 0)) {    // it's not possible to display cells on the low row if it's active

    short col = getNoteNumColumn(split, notenum, row);
    if (col > 0) {
      setLed(col, row, COLOR_OFF, cellOff, LED_LAYER_PLAYED);
    }
  }
}

void resetPossibleNoteCells(byte split, byte notenum) {
  if (userFirmwareActive) return;
  if (displayMode != displayNormal) return;
  
  byte row = 0;
  if (Split[split].lowRowMode != lowRowNormal) {
    row = 1;
  }
  for (; row < NUMROWS; ++row) {
    short col = getNoteNumColumn(split, notenum, row);
    if (col > 0) {
      setLed(col, row, COLOR_OFF, cellOff, LED_LAYER_PLAYED);
    }
  }
}

short getNoteNumColumn(byte split, byte notenum, byte row) {
  short offset, lowest;
  determineNoteOffsetAndLowest(split, row, offset, lowest);

  short col = notenum - (lowest + (row * offset) + Split[split].transposeOctave) + 1   // calculate the column that this MIDI note can be played on
            + Split[split].transposeLights - Split[split].transposePitch;;             // adapt for transposition settings
  if (Device.leftHanded) {
    col = NUMCOLS - col;
  }

  byte lowColSplit, highColSplit;
  getSplitBoundaries(split, lowColSplit, highColSplit);
  if (col < lowColSplit || col >= highColSplit) {                                      // only return columns that are valid for the split
    col = -1;
  }

  return col;
}

ByteBuffer<4096> midiOutQueue;

// Arrays to keep track of the last sent MIDI values to allow the MIDI output
// routines to not unnecessarily send out duplicate data
byte lastValueMidiCC[16*128];
int  lastValueMidiPB[16];
byte lastValueMidiAT[16];
byte lastValueMidiPP[16*128];

// Arrays to keep track of the last moment of MIDI values to allow for MIDI output
// decimation
unsigned long lastMomentMidiCC[16*128];
unsigned long lastMomentMidiPB[16];
unsigned long lastMomentMidiAT[16];
unsigned long lastMomentMidiPP[16*128];

inline byte getBendRange(byte split) {
  byte bendRange = 0;

  switch (Split[split].bendRangeOption) {
    case bendRange2:
      bendRange = 2;
      break;
    case bendRange3:
      bendRange = 3;
      break;
    case bendRange12:
      bendRange = 12;
      break;
    case bendRange24:
      bendRange = Split[split].customBendRange;
      break;
  }

  return bendRange;
}

int scalePitch(byte split, int pitchValue) {
  byte bendRange = getBendRange(split);

  // Adapt for bend range
  switch(bendRange)
  {
    // pure integer math cases
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 8:
    case 12:
    case 16:
    case 24:
      pitchValue = pitchValue * (48 / bendRange);
      break;
    // no calculations needed
    case 48:
      break;
    // others need fixed point decimal math
    default:
      pitchValue = FXD_TO_INT(FXD_MUL(FXD_FROM_INT(pitchValue), FXD_DIV(FXD_FROM_INT(48), FXD_FROM_INT(bendRange))));
      break;
  }

  return pitchValue;
}

// Send pitch bend data to all the active channels of the split without changing the bend range
void preSendPitchBend(byte split, int pitchValue) {
  pitchValue = scalePitch(split, pitchValue);

  switch (Split[split].midiMode)
  {
    case channelPerNote:
    {
      for (byte ch = 0; ch < 16; ++ch) {
        if (Split[split].midiChanSet[ch]) {
          midiSendPitchBend(pitchValue, ch+1);
        }
      }
      break;
    }

    case channelPerRow:
    {
      for (byte row = 0; row < 8; ++row) {
        byte ch = Split[split].midiChanPerRow + row;
        if (ch > 16) {
          ch -= 16;
        }
        midiSendPitchBend(pitchValue, ch);
      }
      break;
    }

    case oneChannel:
    {
      midiSendPitchBend(pitchValue, Split[split].midiChanMain);
      break;
    }
  }
}

// Called to send a Pitch Bend message. Depending on mode, sends different Bend data
void preSendPitchBend(byte split, int pitchValue, byte channel) {
  midiSendPitchBend(scalePitch(split, pitchValue), channel);    // Send the bend amount as a difference from bend center (8192)
}

// Calculate the real value if custom limits are set
byte applyLimits(byte value, byte minValue, byte maxValue, int32_t ratio) {
  if (minValue != 0 || maxValue != 127) {
    value = minValue + FXD_TO_INT(FXD_MUL(FXD_FROM_INT(value), ratio));
  }

  return value;
}

void preResetLastTimbre(byte split, byte note, byte channel) {
  switch(Split[split].expressionForY)
  {
    case timbrePolyPressure:
      resetLastMidiPolyPressure(note, channel);
      break;

    case timbreChannelPressure:
      resetLastMidiAfterTouch(channel);
      break;

    default:
    {
      byte ccForY;
      if (Split[split].expressionForY == timbreCC1) {
        ccForY = 1;
      }
      else {
        ccForY = Split[split].customCCForY;
      }
      resetLastMidiCC(ccForY, channel);
      break;
    }
  }
}

// Called to send Y-axis movements
void preSendTimbre(byte split, byte yValue, byte note, byte channel) {
  yValue = applyLimits(yValue, Split[split].minForY, Split[split].maxForY, fxdLimitsForYRatio[split]);

  switch(Split[split].expressionForY)
  {
    case timbrePolyPressure:
      midiSendPolyPressure(note, yValue, channel);
      break;

    case timbreChannelPressure:
      midiSendAfterTouch(yValue, channel);
      break;

    default:
    {
      byte ccForY;
      if (Split[split].expressionForY == timbreCC1) {
        ccForY = 1;
      }
      else {
        ccForY = Split[split].customCCForY;
      }
      // if the low row is down, only send the CC for Y if it's not being sent by the low row already
      if ((!isLowRowCCXActive(split) ||
            ccForY != Split[split].ccForLowRow) &&
          (!isLowRowCCXYZActive(split) ||
            (ccForY != Split[split].ccForLowRowX &&
             ccForY != Split[split].ccForLowRowY &&
             ccForY != Split[split].ccForLowRowZ))) {
          midiSendControlChange(ccForY, yValue, channel);
      }
      break;
    }
  }
}

void preResetLastLoudness(byte split, byte note, byte channel) {
  switch(Split[split].expressionForZ)
  {
    case loudnessPolyPressure:
      resetLastMidiPolyPressure(note, channel);
      break;

    case loudnessChannelPressure:
      resetLastMidiAfterTouch(channel);
      break;

    case loudnessCC11:
      resetLastMidiCC(Split[split].customCCForZ, channel);
      break;
  }
}

// Called to send Z message. Depending on midiMode, sends different types of Channel Pressure or Poly Pressure message.
void preSendLoudness(byte split, byte pressureValue, byte note, byte channel) {
  pressureValue = applyLimits(pressureValue, Split[split].minForZ, Split[split].maxForZ, fxdLimitsForZRatio[split]);

  switch(Split[split].expressionForZ)
  {
    case loudnessPolyPressure:
      midiSendPolyPressure(note, pressureValue, channel);
      break;

    case loudnessChannelPressure:
      midiSendAfterTouch(pressureValue, channel);
      break;

    case loudnessCC11:
      // if the low row is down, only send the CC for Z if it's not being sent by the low row already
      if ((!isLowRowCCXActive(split) ||
            Split[split].customCCForZ != Split[split].ccForLowRow) &&
          (!isLowRowCCXYZActive(split) ||
            (Split[split].customCCForZ != Split[split].ccForLowRowX &&
             Split[split].customCCForZ != Split[split].ccForLowRowY &&
             Split[split].customCCForZ != Split[split].ccForLowRowZ))) {
        midiSendControlChange(Split[split].customCCForZ, pressureValue, channel);
      }
      break;
  }
}

void resetLastMidiPolyPressure(byte note, byte channel) {
  lastValueMidiPP[channel * note] = 0xFF;
  lastMomentMidiPP[channel * note] = 0;
}

void resetLastMidiAfterTouch(byte channel) {
  lastValueMidiAT[channel] = 0xFF;
  lastMomentMidiAT[channel] = 0;
}

void resetLastMidiCC(byte controlnum, byte channel) {
  lastValueMidiCC[channel * controlnum] = 0xFF;
  lastMomentMidiCC[channel * controlnum] = 0;
}

void resetLastMidiPitchBend(byte channel) {
  lastValueMidiPB[channel] = 0x7FFF;
  lastMomentMidiPB[channel] = 0;
}

void initializeLastMidiTracking() {
  // Initialize the arrays that track the latest MIDI values by setting all entries
  // to invalid MIDI values. This ensures that the first messages will always be sent.
  for (byte ch = 0; ch < 16; ++ch) {
    lastValueMidiPB[ch] = 0x7FFF;
    lastValueMidiAT[ch] = 0xFF;
    lastMomentMidiPB[ch] = 0;
    lastMomentMidiAT[ch] = 0;
    for (byte msg = 0; msg < 128; ++msg) {
      lastValueMidiCC[ch*msg] = 0xFF;
      lastValueMidiPP[ch*msg] = 0xFF;
      lastMomentMidiCC[ch*msg] = 0;
      lastMomentMidiPP[ch*msg] = 0;
    }
  }

  // Initialize the arrays that track which MIDI notes are on
  for (byte s = 0; s < 2; ++s) {
    for (byte n = 0; n < 128; ++n) {
      for (byte c = 0; c < 16; ++c) {
        lastValueMidiNotesOn[s][n][c] = 0;
      }
    }
  }
}

void queueMidiMessage(MIDIStatus type, byte param1, byte param2, byte channel) {
  // we always queue four bytes and will process them as MIDI messages in the handlePendingMidi
  midiOutQueue.push(channel & 0x0F);
  midiOutQueue.push((byte)type);
  midiOutQueue.push(param1 & 0x7F);
  midiOutQueue.push(param2 & 0x7F);
}

void handlePendingMidi(unsigned long now) {
  static unsigned long lastEnvoy = 0;
  static byte messageIndex = 0;
  static byte lastChannel = 0;
  static byte lastType = 0;

  if (!midiOutQueue.empty()) {
    // the first queued byte is always the MIDI channel
    if (messageIndex == 0) {
      lastChannel = midiOutQueue.pop();
      messageIndex++;
    }
    // the others will constitute the actual MIDI message
    else {
      byte nextByte = midiOutQueue.peek();

      // always insert a 1 ms delay around MIDI note on and note off boundaries
      unsigned long additionalInterval = 0;
      if (messageIndex == 1 &&
          (lastType == MIDINoteOn ||
           nextByte == MIDINoteOff || lastType == MIDINoteOff)) {
        additionalInterval = 2000;
      }

      // if the time between now and the last MIDI byte exceeds the required interval, process it
      if (calcTimeDelta(now, lastEnvoy) >= midiMinimumInterval + additionalInterval) {
        // construct the correct MIDI byte that needs to be sent
        byte midiByte;
        if (messageIndex == 1) {
          midiByte = nextByte | lastChannel;
        }
        else {
          midiByte = nextByte;
        }

        // try to send the MIDI message byte
        if (sendNextMidiOutputByte(midiByte)) {
          // keep track of the last MIDI message type that was successfully sent
          if (messageIndex == 1) {
            lastType = nextByte;
          }

          // remove the sent byte from the queue and keep track of the message sequence
          midiOutQueue.pop();
          messageIndex++;

          // in case of program change and channel pressure, the MIDI message length is one shorter,
          // so automatically remove the fourth byte from the queue since it's unused
          if (messageIndex == 3 &&
              (lastType == MIDIProgramChange || lastType == MIDIChannelPressure)) {
            midiOutQueue.pop();
            messageIndex++;
          }
        }

        // update the last time a MIDI byte was attempted to be sent
        lastEnvoy = now;
      }
    }

    // each time four bytes have been processed from the queue, start a new MIDI message
    if (messageIndex == 4) {
      messageIndex = 0;
      lastChannel = 0;
    }
  }
}

// send a MIDI message byte using the most appropriate method, the return value
// indicates whether the byte was sent successfully
boolean sendNextMidiOutputByte(byte b) {
#ifdef PATCHED_ARDUINO_SERIAL_WRITE
  if (Serial.write(b, false) > 0) {
    return true;
  }
  return false;
#else
  Serial.write(b);
  return true;
#endif
}

void midiSendVolume(byte v, byte channel) {
  midiSendControlChange(7, v, channel);
}

void preSendSustain(byte split, byte v) {
  if (Split[split].mpe) {
    midiSendControlChange(64, v, Split[split].midiChanMain, true);
  }
  else {
    preSendControlChange(split, 64, v);
  }
}

void preSendSwitchCC65(byte split, byte v) {
  if (Split[split].mpe) {
    midiSendControlChange(Global.ccForSwitch, v, Split[split].midiChanMain, true);
  }
  else {
    preSendControlChange(split, Global.ccForSwitch, v);
  }
}

void preSendControlChange(byte split, byte controlnum, byte v) {
  switch (Split[split].midiMode)
  {
    case channelPerNote:
    {
      for (byte ch = 0; ch < 16; ++ch) {
        if (Split[split].midiChanSet[ch]) {
          midiSendControlChange(controlnum, v, ch+1, true);
        }
      }
      break;
    }

    case channelPerRow:
    {
      for ( byte row = 0; row < 8; ++row) {
        byte ch = Split[split].midiChanPerRow + row;
        if (ch > 16) {
          ch -= 16;
        }
        midiSendControlChange(controlnum, v, ch, true);
      }
      break;
    }

    case oneChannel:
    {
      midiSendControlChange(controlnum, v, Split[split].midiChanMain, true);
      break;
    }
  }
}

void midiSendPreset(byte p, byte channel) {
  midiSendProgramChange(p, channel);
}

void midiSendAllNotesOff(byte split) {
  preSendControlChange(split, 120, 0);
  preSendControlChange(split, 123, 0);

  preSendControlChange(split, 64, 0);
  for (byte notenum = 0; notenum < 128; ++notenum) {
    switch (Split[split].midiMode)
    {
      case channelPerNote:
      {
        for (byte ch = 0; ch < 16; ++ch) {
          if (Split[split].midiChanSet[ch]) {
            midiSendNoteOffRaw(notenum, 0x40, ch);
          }
        }
        break;
      }

      case channelPerRow:
      {
        for ( byte row = 0; row < 8; ++row) {
          byte ch = Split[split].midiChanPerRow + row;
          if (ch > 16) {
            ch -= 16;
          }
          midiSendNoteOffRaw(notenum, 0x40, ch-1);
        }
        break;
      }

      case oneChannel:
      {
        midiSendNoteOffRaw(notenum, 0x40, Split[split].midiChanMain-1);
        break;
      }
    }
  }
}

void midiSendControlChange(byte controlnum, byte controlval, byte channel) {
  midiSendControlChange(controlnum, controlval, channel, false);
}

void midiSendControlChange(byte controlnum, byte controlval, byte channel, boolean always) {
  controlval = constrain(controlval, 0, 127);
  channel = constrain(channel-1, 0, 15);

  unsigned long now = millis();
  // always send channel mode messages and sustain, as well as messages that are flagged as always (for instance 14 bit MIDI)
  short index = controlnum * channel;
  if (!always && controlnum < 120 && controlnum != 64) {
    if (lastValueMidiCC[index] == controlval) return;
    if (controlval != 0 && calcTimeDelta(now, lastMomentMidiCC[index]) <= midiDecimateRate) return;
  }
  lastValueMidiCC[index] = controlval;
  lastMomentMidiCC[index] = now;

  if (Device.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("midiSendControlChange controlnum=");
      Serial.print((int)controlnum);
      Serial.print(", controlval=");
      Serial.print((int)controlval);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
#endif
  } else {
    queueMidiMessage(MIDIControlChange, controlnum, controlval, channel);
  }
}

void midiSendControlChange14Bit(byte controlMsb, byte controlLsb, short controlval, byte channel) {
  controlval = constrain(controlval, 0, 0x3fff);
  channel = constrain(channel-1, 0, 15);

  unsigned long now = millis();

  // calculate the 14-bit msb and lsb
  unsigned msb = (controlval & 0x3fff) >> 7;
  unsigned lsb = controlval & 0x7f;

  short indexMsb = controlMsb * channel;
  short indexLsb = controlLsb * channel;

  if (lastValueMidiCC[indexMsb] == msb && lastValueMidiCC[indexLsb] == lsb) return;
  if (controlval != 0 &&
      (calcTimeDelta(now, lastMomentMidiCC[indexMsb]) <= midiDecimateRate ||
       calcTimeDelta(now, lastMomentMidiCC[indexLsb]) <= midiDecimateRate)) return;
  lastValueMidiCC[indexMsb] = msb;
  lastMomentMidiCC[indexMsb] = now;
  lastValueMidiCC[indexLsb] = lsb;
  lastMomentMidiCC[indexLsb] = now;

  if (Device.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("midiSendControlChange14Bit controlMsb=");
      Serial.print((int)controlMsb);
      Serial.print(", controlLsb=");
      Serial.print((int)controlLsb);
      Serial.print(", controlval=");
      Serial.print((int)controlval);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
#endif
  } else {
    queueMidiMessage(MIDIControlChange, controlLsb, lsb, channel);
    queueMidiMessage(MIDIControlChange, controlMsb, msb, channel);
  }
}

void midiSendNoteOn(byte split, byte notenum, byte velocity, byte channel) {
  notenum = constrain(notenum, 0, 127);
  velocity = constrain(velocity, 0, 127);
  channel = constrain(channel-1, 0, 15);

  lastValueMidiNotesOn[split][notenum][channel]++;

  if (Device.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("midiSendNoteOn notenum=");
      Serial.print((int)notenum);
      Serial.print(", velocity=");
      Serial.print((int)velocity);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
#endif
  } else {
    queueMidiMessage(MIDINoteOn, notenum, velocity, channel);
  }
}

void midiSendNoteOff(byte split, byte notenum, byte channel) {
  notenum = constrain(notenum, 0, 127);
  channel = constrain(channel-1, 0, 15);

  if (lastValueMidiNotesOn[split][notenum][channel] > 0) {
      lastValueMidiNotesOn[split][notenum][channel]--;
    midiSendNoteOffRaw(notenum, 0x40, channel);
    lastValueMidiPB[channel] = 0x7FFF;
  }
}

void midiSendNoteOffWithVelocity(byte split, byte notenum, byte velocity, byte channel) {
  notenum = constrain(notenum, 0, 127);
  channel = constrain(channel-1, 0, 15);

  if (lastValueMidiNotesOn[split][notenum][channel] > 0) {
      lastValueMidiNotesOn[split][notenum][channel]--;
    midiSendNoteOffRaw(notenum, velocity, channel);
    lastValueMidiPB[channel] = 0x7FFF;
  }
}

void midiSendNoteOffRaw(byte notenum, byte velocity, byte channel) {
  if (Device.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("midiSendNoteOff notenum=");
      Serial.print((int)notenum);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
#endif
  } else {
    queueMidiMessage(MIDINoteOff, notenum, velocity, channel);
  }
}

void midiSendNoteOffForAllTouches(byte split) {
  signed char note = noteTouchMapping[split].firstNote;
  signed char channel = noteTouchMapping[split].firstChannel;

  while (note != -1) {
    midiSendNoteOff(split, note, channel);
    NoteEntry* entry = noteTouchMapping[split].getNoteEntry(note, channel);
    if (entry == NULL) {
      note = -1;
    }
    else {
      note = entry->getNextNote();
      channel = entry->getNextChannel();
    }
  }
}

void midiSendPitchBend(int pitchval, byte channel) {
  unsigned int bend = constrain(pitchval + 8192, 0, 16383);
  channel = constrain(channel-1, 0, 15);

  unsigned long now = millis();
  if (lastValueMidiPB[channel] == bend) return;
  if (pitchval != 0 && calcTimeDelta(now, lastMomentMidiPB[channel]) <= midiDecimateRate) return;
  lastValueMidiPB[channel] = bend;
  lastMomentMidiPB[channel] = now;

  if (Device.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("midiSendPitchBend pitchval=");
      Serial.print(pitchval);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
#endif
  } else {
    queueMidiMessage(MIDIPitchBend, bend & 0x7F, (bend >> 7) & 0x7F, channel);
  }
}

void midiSendProgramChange(byte preset, byte channel) {
  preset = constrain(preset, 0, 127);
  channel = constrain(channel-1, 0, 15);

  if (Device.serialMode) {
    if (SWITCH_DEBUGMIDI && debugLevel >= 0) {
      Serial.print("midiSendProgramChange preset=");
      Serial.print(preset);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
  } else {
    queueMidiMessage(MIDIProgramChange, preset, 0, channel);
  }
}

void midiSendAfterTouch(byte value, byte channel) {
  value = constrain(value, 0, 127);
  channel = constrain(channel-1, 0, 15);

  unsigned long now = millis();
  if (lastValueMidiAT[channel] == value) return;
  if (value != 0 && calcTimeDelta(now, lastMomentMidiAT[channel]) <= midiDecimateRate) return;
  lastValueMidiAT[channel] = value;
  lastMomentMidiAT[channel] = now;

  if (Device.serialMode) {
    if (SWITCH_DEBUGMIDI && debugLevel >= 0) {
      Serial.print("midiSendAfterTouch value=");
      Serial.print(value);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
  } else {
    queueMidiMessage(MIDIChannelPressure, value, 0, channel);
  }
}

void midiSendPolyPressure(byte notenum, byte value, byte channel) {
  value = constrain(value, 0, 127);
  channel = constrain(channel-1, 0, 15);

  unsigned long now = millis();
  short index = notenum * channel;
  if (lastValueMidiPP[index] == value) return;
  if (calcTimeDelta(now, lastMomentMidiPP[index]) <= midiDecimateRate) return;
  lastValueMidiPP[index] = value;
  lastMomentMidiPP[index] = now;

  if (Device.serialMode) {
    if (SWITCH_DEBUGMIDI && debugLevel >= 0) {
      Serial.print("midiSendPolyPressure notenum=");
      Serial.print((int)notenum);
      Serial.print(", value=");
      Serial.print((int)value);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
  } else {
    queueMidiMessage(MIDIPolyphonicPressure, notenum, value, channel);
  }
}

void midiSendNRPN(unsigned short number, unsigned short value, byte channel) {
  number = constrain(number, 0, 0x3fff);
  value = constrain(value, 0, 0x3fff);
  channel = constrain(channel-1, 0, 15);

  if (Device.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("midiSendNRPN number=");
      Serial.print((int)number);
      Serial.print(", value=");
      Serial.print((int)value);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
#endif
  } else {
    unsigned numberMsb = (number & 0x3fff) >> 7;
    unsigned numberLsb = number & 0x7f;
    unsigned valueMsb = (value & 0x3fff) >> 7;
    unsigned valueLsb = value & 0x7f;

    queueMidiMessage(MIDIControlChange, 99, numberMsb, channel);
    queueMidiMessage(MIDIControlChange, 98, numberLsb, channel);
    queueMidiMessage(MIDIControlChange, 6, valueMsb, channel);
    queueMidiMessage(MIDIControlChange, 38, valueLsb, channel);
    queueMidiMessage(MIDIControlChange, 101, 127, channel);
    queueMidiMessage(MIDIControlChange, 100, 127, channel);
  }
}

void midiSendMpeState(byte mainChannel, byte polyphony) {
  midiSendControlChange(127, polyphony, mainChannel, true);
}

void midiSendMpePitchBendRange(byte split) {
  if (Split[split].mpe && getBendRange(split) == 24) {
    midiSendNRPN(0, 24, Split[split].midiChanMain);
  }
}