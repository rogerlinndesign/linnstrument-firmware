/******************************** ls_lowRow: LinnStrument Low Row *********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These are the functions for the low row functionality of the LinnStrument

The low row operations are designed to be driven by the main cell scanning loop. When actions
occur, they are registered to provide the appropriate state during the handleLowRowState() function.
This function also takes cell that are outside the low row into account, to trigger to relevant
operations for those when needed.
**************************************************************************************************/

enum ColumnState {
  inactive,
  pressed,
  continuous
};

ColumnState lowRowState[NUMCOLS];
boolean lowRowBendActive[NUMSPLITS];
boolean lowRowCCXActive[NUMSPLITS];
boolean lowRowCCXYZActive[NUMSPLITS];
short lowRowInitialColumn[NUMSPLITS];

inline boolean isLowRow() {
  if (sensorRow != 0) return false;
  if (Split[sensorSplit].lowRowMode == lowRowNormal) return false;
  if (Split[sensorSplit].ccFaders) return false;
  if (isStrummingSplit(sensorSplit)) return false;

  return true;
}

void initializeLowRowState() {
  for (byte col = 0; col < NUMCOLS; ++col) {
    lowRowState[col] = inactive;
  }
  for (byte split = 0; split < NUMSPLITS; ++split) {
    lowRowBendActive[split] = false;
    lowRowCCXActive[split] = false;
    lowRowCCXYZActive[split] = false;
    lowRowInitialColumn[split] = -1;
  }
}

boolean lowRowRequiresSlideTracking() {
  switch (Split[sensorSplit].lowRowMode)
  {
    case lowRowRestrike:
    case lowRowStrum:
    case lowRowSustain:
      return false;
    case lowRowArpeggiator:
    case lowRowBend:
    case lowRowCCX:
    case lowRowCCXYZ:
      return true;
  }
}

boolean allowNewTouchOnLowRow() {
  switch (Split[sensorSplit].lowRowMode)
  {
    case lowRowRestrike:
    case lowRowStrum:
    case lowRowArpeggiator:
    case lowRowBend:
    case lowRowCCX:
    case lowRowCCXYZ:
      return true;
    case lowRowSustain:
      return lowRowState[sensorSplit] == inactive;
  }
}

#define LOWROW_X_LEFT_LIMIT   0
#define LOWROW_X_RIGHT_LIMIT  4095

void handleLowRowState(boolean newVelocity, short pitchBend, short timbre, byte pressure) {
  // if we're processing a low-row sensor, mark the appropriate column as continuous
  // if it was previously presssed
  if (isLowRow()) {
    // get fader dimensions for possible later use
    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);

    // it's a new touch which is complementary to lowRowStart since we have access to the expression data
    if (newVelocity) {
      // when the fader only spans one cell, it acts as a toggle in fader mode
      if (faderLength == 0) {
        switch (Split[sensorSplit].lowRowMode)
        {
          case lowRowCCX:
          {
            if (Split[sensorSplit].lowRowCCXBehavior == lowRowCCFader) {
              if (ccFaderValues[sensorSplit][Split[sensorSplit].ccForLowRow] > 0) {
                sendLowRowCCX(0);
              }
              else {
                sendLowRowCCX(127);
              }
            }
            break;
          }
          case lowRowCCXYZ:
          {
            // determine the X value based on the fader behavior, also update the fader position if that's needed
            if (Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCFader) {
              if (ccFaderValues[sensorSplit][Split[sensorSplit].ccForLowRowX] > 0) {
                sendLowRowCCXYZ(0, timbre, pressure);
              }
              else {
                sendLowRowCCXYZ(127, timbre, pressure);
              }
            }
            break;
          }
        }
      }
    }
    // send out the continuous data for the low row cells
    else if (sensorCell->velocity) {
      switch (Split[sensorSplit].lowRowMode)
      {
        case lowRowArpeggiator:
        case lowRowBend:
        case lowRowCCX:
        case lowRowCCXYZ:
          // We limit the low row data to not go past the center of the leftmost and rightmost cell.
          // This gives us exactly 2 octaves in the main split.
          byte lowCol, highCol;
          getSplitBoundaries(sensorSplit, lowCol, highCol);

          short xDelta = constrain(sensorCell->calibratedX() - sensorCell->initialX >> 3, 0, 127);
          short xPosition = calculateFaderValue(sensorCell->calibratedX(), faderLeft, faderLength);

          switch (Split[sensorSplit].lowRowMode)
          {
            case lowRowArpeggiator:
            {
              arpTempoDelta[sensorSplit] = sensorCol - lowRowInitialColumn[sensorSplit];
              break;
            }
            case lowRowBend:
            {
              if (pitchBend != SHRT_MAX) {
                if (Split[sensorSplit].mpe) {
                  midiSendPitchBend(scalePitch(sensorSplit, pitchBend), Split[sensorSplit].midiChanMain);
                }
                else {
                  preSendPitchBend(sensorSplit, pitchBend);
                }
              }
              break;
            }
            case lowRowCCX:
            {
              if (Split[sensorSplit].lowRowCCXBehavior == lowRowCCFader) {
                if (faderLength > 0) {
                  sendLowRowCCX(xPosition);
                }
              }
              else {
                sendLowRowCCX(xDelta);
              }

              break;
            }
            case lowRowCCXYZ:
            {
              if (Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCFader) {
                if (faderLength > 0) {
                  sendLowRowCCXYZ(xPosition, timbre, pressure);
                }
              }
              else {
                sendLowRowCCXYZ(xDelta, timbre, pressure);
              }
              break;
            }
          }
          break;
      }
    }

    // properly transition the low row states based on the active mode (global or per-column state transitions)
    switch (Split[sensorSplit].lowRowMode)
    {
      case lowRowRestrike:
      case lowRowArpeggiator:
      case lowRowSustain:
      case lowRowBend:
      case lowRowCCX:
      case lowRowCCXYZ:
        if (lowRowState[sensorSplit] == pressed) {
          lowRowState[sensorSplit] = continuous;
        }
        break;
      case lowRowStrum:
        if (lowRowState[sensorCol] == pressed) {
          lowRowState[sensorCol] = continuous;
        }
        break;
    }
  }
  else {
    switch (Split[sensorSplit].lowRowMode)
    {
      case lowRowRestrike:
        handleLowRowRestrike();
        break;
      case lowRowStrum:
        handleLowRowStrum();
        break;
    }
  }
}

void sendLowRowCCX(unsigned short x) {
  if (Split[sensorSplit].lowRowCCXBehavior == lowRowCCFader) {
    ccFaderValues[sensorSplit][Split[sensorSplit].ccForLowRow] = x;

    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);
    paintCCFaderDisplayRow(sensorSplit, sensorRow, Split[sensorSplit].colorLowRow, Split[sensorSplit].ccForLowRow, faderLeft, faderLength);
  }

  // send out the MIDI CC
  midiSendControlChange(Split[sensorSplit].ccForLowRow, x, Split[sensorSplit].midiChanMain);
}

void sendLowRowCCXYZ(unsigned short x, short y, short z) {
  if (Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCFader) {
    ccFaderValues[sensorSplit][Split[sensorSplit].ccForLowRowX] = x;

    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);
    paintCCFaderDisplayRow(sensorSplit, sensorRow, Split[sensorSplit].colorLowRow, Split[sensorSplit].ccForLowRowX, faderLeft, faderLength);
  }

  // send out the MIDI CCs
  midiSendControlChange(Split[sensorSplit].ccForLowRowX, x, Split[sensorSplit].midiChanMain);
  if (y != SHRT_MAX) {
    midiSendControlChange(Split[sensorSplit].ccForLowRowY, y, Split[sensorSplit].midiChanMain);
  }
  midiSendControlChange(Split[sensorSplit].ccForLowRowZ, z, Split[sensorSplit].midiChanMain);
}

void handleLowRowRestrike() {
  // we're processing a cell that's not on the low-row, check if column 0 was pressed
  // and retrigger in that case
  if (sensorCell->hasNote() && lowRowState[sensorSplit] == pressed) {
    // use the velocity of the low-row press
    sensorCell->velocity = cell(0, 0).velocity;

    // retrigger the MIDI note
    midiSendNoteOff(sensorSplit, sensorCell->note, sensorCell->channel);
    midiSendNoteOn(sensorSplit, sensorCell->note, sensorCell->velocity, sensorCell->channel);
  }
}

void handleLowRowStrum() {
  // we're processing a cell that's not on the low-row, check if the corresponding
  // low-row column was pressed and retrigger in that case
  if (sensorCell->hasNote() && lowRowState[sensorCol] == pressed) {
    // use the velocity of the low-row press
    sensorCell->velocity = cell(sensorCol, 0).velocity;

    // retrigger the MIDI note
    midiSendNoteOff(sensorSplit, sensorCell->note, sensorCell->channel);
    midiSendNoteOn(sensorSplit, sensorCell->note, sensorCell->velocity, sensorCell->channel);
  }
}

void lowRowStart() {
  switch (Split[sensorSplit].lowRowMode)
  {
    case lowRowRestrike:
      // no need to keep track of the columns, we want to restrike every note
      lowRowState[sensorSplit] = pressed;
      cell(0, 0).velocity = sensorCell->velocity;
      break;
    case lowRowStrum:
      lowRowState[sensorCol] = pressed;
      break;
    case lowRowArpeggiator:
      if (lowRowState[sensorSplit] == inactive) {
        if (-1 == lowRowInitialColumn[sensorSplit]) {
          lowRowInitialColumn[sensorSplit] = sensorCol;
        }

        lowRowState[sensorSplit] = pressed;
        temporarilyEnableArpeggiator();
      }
      startLowRowContinuousExpression();
      break;
    case lowRowSustain:
      lowRowState[sensorSplit] = pressed;
      preSendSustain(sensorSplit, 127);
      break;
    case lowRowBend:
      lowRowBendActive[sensorSplit] = true;
      resetLastMidiPitchBend(Split[sensorSplit].midiChanMain);
      startLowRowContinuousExpression();
      break;
    case lowRowCCX:
      lowRowCCXActive[sensorSplit] = true;
      resetLastMidiCC(Split[sensorSplit].ccForLowRow, Split[sensorSplit].midiChanMain);
      startLowRowContinuousExpression();
      break;
    case lowRowCCXYZ:
      lowRowCCXYZActive[sensorSplit] = true;
      resetLastMidiCC(Split[sensorSplit].ccForLowRowX, Split[sensorSplit].midiChanMain);
      resetLastMidiCC(Split[sensorSplit].ccForLowRowY, Split[sensorSplit].midiChanMain);
      resetLastMidiCC(Split[sensorSplit].ccForLowRowZ, Split[sensorSplit].midiChanMain);
      startLowRowContinuousExpression();
      break;
  }
}

void startLowRowContinuousExpression() {
  if (lowRowState[sensorSplit] != inactive) {
    // handle taking over an already active touch
    byte lowCol, highCol;
    getSplitBoundaries(sensorSplit, lowCol, highCol);
    for (byte col = lowCol; col < highCol; ++col) {
      if (col != sensorCol && cell(col, 0).velocity) {
        transferFromSameRowCell(col);
        return;
      }
    }
  }
  else {
    // initialize the initial low row touch
    lowRowState[sensorSplit] = pressed;
  }
}

void lowRowStop() {
  switch (Split[sensorSplit].lowRowMode)
  {
    case lowRowRestrike:
      lowRowState[sensorSplit] = inactive;
      cell(0, 0).velocity = 0;
      break;
    case lowRowStrum:
      lowRowState[sensorCol] = inactive;
      break;
    case lowRowSustain:
      lowRowState[sensorSplit] = inactive;
      preSendSustain(sensorSplit, 0);
      break;
    case lowRowArpeggiator:
    case lowRowBend:
    case lowRowCCX:
    case lowRowCCXYZ:
      if (sensorCell->velocity) {
        // handle taking over an already active touch, the highest already active touch wins
        byte lowCol, highCol;
        getSplitBoundaries(sensorSplit, lowCol, highCol);
        for (byte col = highCol-1; col >= lowCol; --col) {
          if (col != sensorCol && cell(col, 0).touched == touchedCell) {
            transferToSameRowCell(col);
            return;
          }
        }

        if (lowRowState[sensorSplit] != inactive) {
          switch (Split[sensorSplit].lowRowMode)
          {
            case lowRowArpeggiator:
              arpTempoDelta[sensorSplit] = 0;
              disableTemporaryArpeggiator();
              lowRowInitialColumn[sensorSplit] = -1;
              break;
            case lowRowBend:
              // reset the pitchbend since no low row touch is active anymore
              lowRowBendActive[sensorSplit] = false;
              if (Split[sensorSplit].mpe) {
                midiSendPitchBend(0, Split[sensorSplit].midiChanMain);
              }
              else {
                preSendPitchBend(sensorSplit, 0);
              }
              break;
            case lowRowCCX:
              lowRowCCXActive[sensorSplit] = false;
              if (Split[sensorSplit].lowRowCCXBehavior == lowRowCCHold) {
                // reset CC for lowRowX since no low row touch is active anymore
                midiSendControlChange(Split[sensorSplit].ccForLowRow, 0, Split[sensorSplit].midiChanMain);
              }
              break;
            case lowRowCCXYZ:
              lowRowCCXYZActive[sensorSplit] = false;
              if (Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCHold) {
                // reset CCs for lowRowXYZ since no low row touch is active anymore
                midiSendControlChange(Split[sensorSplit].ccForLowRowX, 0, Split[sensorSplit].midiChanMain);
                midiSendControlChange(Split[sensorSplit].ccForLowRowY, 0, Split[sensorSplit].midiChanMain);
                midiSendControlChange(Split[sensorSplit].ccForLowRowZ, 0, Split[sensorSplit].midiChanMain);
              }
              break;
          }

          lowRowState[sensorSplit] = inactive;
        }
      }
      break;
  }
}

inline boolean isLowRowArpeggiatorPressed(byte split) {
  return Split[split].lowRowMode == lowRowArpeggiator && lowRowState[split] != inactive;
}

inline boolean isLowRowBendActive(byte split) {
  return lowRowBendActive[split];
}

inline boolean isLowRowCCXActive(byte split) {
  return lowRowCCXActive[split];
}

inline boolean isLowRowCCXYZActive(byte split) {
  return lowRowCCXYZActive[split];
}
