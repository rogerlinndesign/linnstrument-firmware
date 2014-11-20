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

byte lastNrpnMsb = 127;
byte lastNrpnLsb = 127;
byte lastDataMsb = 0;
byte lastDataLsb = 0;

void applyMidiIoSetting() {
  if (Global.midiIO == 0) {
    digitalWrite(36, 0);     // Set LOW for DIN jacks
    Serial.begin(31250);     // set serial port at MIDI DIN speed 31250
    Serial.flush();          // clear the serial port
  }
  else if (Global.midiIO == 1) {
    digitalWrite(36, 1);     // Set HIGH for USB
    Serial.begin(115200);    // set serial port at fastest speed 115200
    Serial.flush();          // clear the serial port
  }
}

void handleMidiInput() {
  unsigned long now = micros();

  // handle turning off the MIDI clock led after minimum 30ms
  if (midiClockLedOn != 0 && calcTimeDelta(now, midiClockLedOn) > LED_FLASH_DELAY) {
    midiClockLedOn = 0;
    setLed( 0, 0, COLOR_BLACK, 0);
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
          setLed(25, 2, COLOR_GREEN, 3);
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
            setLed(0, GLOBAL_SETTINGS_ROW, COLOR_GREEN, 3);
            midiClockLedOn = now;
          }

          // play the next arpeggiator step if needed
          checkAdvanceArpeggiator(now);

          // flash the tempo led in the global display when it is on
          updateGlobalDisplay();
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
              highlightNoteCells(Split[split].colorNoteon, split, midiData1);
            }
            break;
          }
          // purposely fall-through in case of velocity 0
        }

        case MIDINoteOff:
        {
          if (split != -1 && (split == LEFT || splitActive)) {
            resetNoteCells(split, midiData1);
          }
          break;
        }

        case MIDIProgramChange:
        {
          if (split != -1) {
            Split[split].preset = midiData1;
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
              // if an NRPN parameter was selected, start constituting the data
              // otherwise control the fader of MIDI CC 6
              if (lastNrpnMsb != 127 && lastNrpnLsb != 127) {
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
                ccFaderValues[split][midiData1-1] = midiData2;
                if ((displayMode == displayNormal && Split[split].ccFaders) ||
                    displayMode == displayVolume) {
                  updateDisplay();
                }
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
                if (midiData2 <= COLOR_MAGENTA) {
                  setLed(midiCellColCC, midiCellRowCC, midiData2, 3);
                }
                else {
                  paintNormalDisplayCell(getSplitOf(midiCellColCC), midiCellColCC, midiCellRowCC);
                }
              }
              break;
            case 38:
              if (lastNrpnMsb != 127 && lastNrpnLsb != 127) {
                lastDataLsb = midiData2;
                receivedNrpn((lastNrpnMsb<<7)+lastNrpnLsb, (lastDataMsb<<7)+lastDataLsb);
                break;
              }
            case 99:
            case 101:
              lastNrpnMsb = midiData2;
              break;
            case 98:
            case 100:
              lastNrpnLsb = midiData2;
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

int determineSplitForChannel(byte channel) {
  if (channel > 15) {
    return -1;
  }

  for (int split = LEFT; split <= RIGHT; ++split) {
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
        byte basechan = Split[split].midiChanPerRow-1;
        if ((basechan <= 8 && channel >= basechan && channel < basechan+8) ||
            (basechan > 8 && channel < basechan && channel < basechan+8-16)) {
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
      }
      break;
    // Split MIDI Main Channel
    case 1:
      if (inRange(value, 1, 16)) {
        Split[split].midiChanMain = value;
      }
    // Split MIDI Per Note Channels
    case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
    case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
      if (inRange(value, 0, 1)) {
        Split[split].midiChanSet[parameter-2] = value;
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
      if (value == 2 || value == 3 || value == 12 || value == 24) {
        Split[split].bendRange = value;
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
      if (inRange(value, 0, 1)) {
        Split[split].pitchCorrectHold = value;
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
        Split[split].ccForY = value;
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
        Split[split].ccForZ = value;
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
        Global.mainNotes[parameter-203] = value;
      }
      break;
    // Global Accent Note Lights
    case 215: case 216: case 217: case 218: case 219: case 220:
    case 221: case 222: case 223: case 224: case 225: case 226:
      if (inRange(value, 0, 1)) {
        Global.accentNotes[parameter-215] = value;
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
      if (inRange(value, 0, 5)) {
        Global.switchAssignment[3] = value;
      }
      break;
    // Global Switch 2 Assignment
    case 229:
      if (inRange(value, 0, 5)) {
        Global.switchAssignment[2] = value;
      }
      break;
    // Global Foot Left Assignment
    case 230:
      if (inRange(value, 0, 5)) {
        Global.switchAssignment[0] = value;
      }
      break;
    // Global Foot Right Assignment
    case 231:
      if (inRange(value, 0, 5)) {
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
  }

  updateDisplay();
}

boolean isMidiClockRunning() {
  return midiClockStarted;
}

short getMidiClockCount() {
  return midiClockMessageCount - 1;
}

void highlightNoteCells(byte color, byte split, byte notenum) {
  if (displayMode != displayNormal) return;

  int row = 0;
  if (Split[split].lowRowMode != lowRowNormal) {
    row = 1;
  }
  for (; row < NUMROWS; ++row) {
    int col = getNoteNumColumn(split, notenum, row);
    if (col > 0) {
      setLed(col, row, color, 3);
    }
  }
}

void resetNoteCells(byte split, byte notenum) {
  if (displayMode != displayNormal) return;
  
  int row = 0;
  if (Split[split].lowRowMode != lowRowNormal) {
    row = 1;
  }
  for (; row < NUMROWS; ++row) {
    int col = getNoteNumColumn(split, notenum, row);
    if (col > 0) {
      paintNormalDisplayCell(split, col, row);
    }
  }
}

int getNoteNumColumn(byte split, byte notenum, byte row) {
  int offset, lowest;
  determineNoteOffsetAndLowest(split, row, offset, lowest);

  int col = notenum - (lowest + (row * offset) + Split[split].transposeOctave) + 1   // calculate the column that this MIDI note can be played on
            + Split[split].transposeLights - Split[split].transposePitch;;           // adapt for transposition settings

  byte lowColSplit, highColSplit;
  getSplitBoundaries(split, lowColSplit, highColSplit);
  if (col < lowColSplit || col >= highColSplit) {                                    // only return columns that are valid for the split
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

int scalePitch(byte split, int pitchValue) {
  switch(Split[split].bendRange)               // Switch on bend range (+/- 2, 3, 12 or 24 semitones)
  {
  case 2:                                      // If 2, multiply by 24
    pitchValue = pitchValue * 24;
    break;
  case 3:                                      // If 3, multiply by 16
    pitchValue = pitchValue * 16;
    break;
  case 12:                                     // If 12, multiply by 4
    pitchValue = pitchValue << 2;
    break;
  case 24:
    pitchValue = pitchValue << 1;              // If 24, multiply by 2
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
        byte ch = Split[split].midiChanMain + row;
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

// Called to send Y-axis movements
void preSendY(byte split, byte yValue, byte channel) {
  midiSendControlChange(Split[split].ccForY, yValue, channel);
}

// Called to send Pressure message. Depending on midiMode, sends different types of Channel Pressure or Poly Pressure message.
void preSendPressure(byte note, byte pressureValue, byte channel) {
  switch(Split[sensorSplit].expressionForZ)
  {
    case loudnessPolyPressure:
      midiSendPolyPressure(note, pressureValue, channel);
      break;

    case loudnessChannelPressure:
      midiSendAfterTouch(pressureValue, channel);
      break;

    case loudnessCC:
      midiSendControlChange(Split[sensorSplit].ccForZ, pressureValue, channel);
      break;
  }
}

void initializeLastMidiTracking() {
  // Initialize the arrays that track the latest MIDI values by setting all entries
  // to invalid MIDI values. This ensures that the first messages will always be sent.
  for (int ch = 0; ch < 16; ++ch) {
    lastValueMidiPB[ch] = 0x7FFF;
    lastValueMidiAT[ch] = 0xFF;
    lastMomentMidiPB[ch] = 0;
    lastMomentMidiAT[ch] = 0;
    for (int msg = 0; msg < 128; ++msg) {
      lastValueMidiCC[ch*msg] = 0xFF;
      lastValueMidiPP[ch*msg] = 0xFF;
      lastMomentMidiCC[ch*msg] = 0;
      lastMomentMidiPP[ch*msg] = 0;
    }
  }

  // Initialize the arrays that track which MIDI notes are on
  for (int s = 0; s < 2; ++s) {
    for (int n = 0; n < 128; ++n) {
      for (int c = 0; c < 16; ++c) {
        lastValueMidiNotesOn[s][n][c] = 0;
      }
    }
  }
}

void queueMidiMessage(MIDIStatus type, byte param1, byte param2, byte channel) {
    param1 &= 0x7F;
    param2 &= 0x7F;
    midiOutQueue.push((byte)type | (channel & 0x0F));
    midiOutQueue.push(param1);
    if (type != MIDIProgramChange && type != MIDIChannelPressure) {
      midiOutQueue.push(param2);
    }
}

void handlePendingMidi(unsigned long now) {
  static unsigned long lastEnvoy = 0;

  // when in low power mode only send one MIDI byte every 150 microseconds
  if (operatingLowPower && calcTimeDelta(now, lastEnvoy) < 150) {
    return;
  }

  if (!midiOutQueue.empty()) {
#ifdef PATCHED_ARDUINO_SERIAL_WRITE
    if (Serial.write(midiOutQueue.peek(), false) > 0) {
      midiOutQueue.pop();
      lastEnvoy = now;
    }
#else
    Serial.write(midiOutQueue.pop());
    lastEnvoy = now;
#endif

  }
}

void midiSendVolume(byte v, byte channel) {
  midiSendControlChange(7, v, channel);
}

void preSendSustain(byte v) {
  preSendControlChange(sensorSplit, 64, v);
}

void preSendControlChange(byte split, byte controlnum, byte v) {
  switch (Split[split].midiMode)
  {
    case channelPerNote:
    {
      for (byte ch = 0; ch < 16; ++ch) {
        if (Split[split].midiChanSet[ch]) {
          midiSendControlChange(controlnum, v, ch+1);
        }
      }
      break;
    }

    case channelPerRow:
    {
      for ( byte row = 0; row < 8; ++row) {
        byte ch = Split[split].midiChanMain + row;
        if (ch > 16) {
          ch -= 16;
        }
        midiSendControlChange(controlnum, v, ch);
      }
      break;
    }

    case oneChannel:
    {
      midiSendControlChange(controlnum, v, Split[split].midiChanMain);
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
            midiSendNoteOffRaw(notenum, ch);
          }
        }
        break;
      }

      case channelPerRow:
      {
        for ( byte row = 0; row < 8; ++row) {
          byte ch = Split[split].midiChanMain + row;
          if (ch > 16) {
            ch -= 16;
          }
          midiSendNoteOffRaw(notenum, ch-1);
        }
        break;
      }

      case oneChannel:
      {
        midiSendNoteOffRaw(notenum, Split[split].midiChanMain-1);
        break;
      }
    }
  }
}

void midiSendControlChange(byte controlnum, byte controlval, byte channel) {
  controlval = constrain(controlval, 0, 127);
  channel = constrain(channel-1, 0, 15);

  unsigned long now = millis();
  if (controlnum < 120 && controlnum != 64) {  // always send channel mode messages and sustain
    short index = controlnum + 128*channel;
    if (lastValueMidiCC[index] == controlval) return;
    if (controlval != 0 && calcTimeDelta(now, lastMomentMidiCC[index]) <= midiDecimateRate) return;
  }
  lastValueMidiCC[controlnum + 128*channel] = controlval;
  lastMomentMidiCC[controlnum + 128*channel] = now;

  if (Global.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("MIDI.sendControlChange controlnum=");
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

void midiSendNoteOn(byte split, byte notenum, byte velocity, byte channel) {
  notenum = constrain(notenum, 0, 127);
  velocity = constrain(velocity, 0, 127);
  channel = constrain(channel-1, 0, 15);

  lastValueMidiNotesOn[split][notenum][channel]++;

  if (Global.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("MIDI.sendNoteOn notenum=");
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
    midiSendNoteOffRaw(notenum, channel);
    lastValueMidiPB[channel] = 0x7FFF;
  }
}

void midiSendNoteOffRaw(byte notenum, byte channel) {
  if (Global.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("MIDI.sendNoteOff notenum=");
      Serial.print((int)notenum);
      Serial.print(", channel=");
      Serial.print((int)channel);
      Serial.print("\n");
    }
#endif
  } else {
    queueMidiMessage(MIDINoteOff, notenum, 0x40, channel);
  }
}

void midiSendNoteOffForAllTouches(byte split) {
  signed char note = noteTouchMapping[split].firstNote;
  signed char channel = noteTouchMapping[split].firstChannel;

  while (note != -1) {
    midiSendNoteOff(split, note, channel);
    NoteEntry& entry = noteTouchMapping[split].mapping[note][channel];
    note = entry.getNextNote();
    channel = entry.getNextChannel();
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

  if (Global.serialMode) {
#ifdef DEBUG_ENABLED
    if (SWITCH_DEBUGMIDI) {
      Serial.print("MIDI.sendPitchBend pitchval=");
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

  if (Global.serialMode) {
    if (SWITCH_DEBUGMIDI && debugLevel >= 0) {
      Serial.print("MIDI.sendProgramChange preset=");
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

  if (Global.serialMode) {
    if (SWITCH_DEBUGMIDI && debugLevel >= 0) {
      Serial.print("MIDI.sendAfterTouch value=");
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
  short index = notenum + 128*channel;
  if (lastValueMidiPP[index] == value) return;
  if (calcTimeDelta(now, lastMomentMidiPP[index]) <= midiDecimateRate) return;
  lastValueMidiPP[index] = value;
  lastMomentMidiPP[index] = now;

  if (Global.serialMode) {
    if (SWITCH_DEBUGMIDI && debugLevel >= 0) {
      Serial.print("MIDI.sendPolyPressure notenum=");
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
