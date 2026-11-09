/******************************** ls_lowRow: LinnStrument Low Row *********************************
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

ColumnState lowRowColumnState[MAXCOLS];
ColumnState lowRowSplitState[NUMSPLITS];
boolean lowRowBendActive[NUMSPLITS];
boolean lowRowCCXActive[NUMSPLITS];
boolean lowRowCCXYZActive[NUMSPLITS];
short lowRowInitialColumn[NUMSPLITS];
short lastRestrikeColumn[NUMSPLITS];

// slide smoothing state (used when Global.lowRowSlideFix is on)
short lowRowSlideLastCol[NUMSPLITS];       // column active on last scan, to detect cell transitions
short lowRowSlideCumulativeX[NUMSPLITS];   // accumulated X offset to bridge calibration gaps at cell handoffs
short lowRowSlideLastSentX[NUMSPLITS];     // last X value output, used to compute correction at next handoff
unsigned long lowRowSlideCellTransMs[NUMSPLITS]; // millis() of the most recent cell transition (for Y/Z gating)
#define LOWROW_YZ_GATE_MS  20              // milliseconds to suppress Y/Z output after a cell transition

inline boolean isLowRow() {
  if (sensorRow != 0) return false;
  if (Split[sensorSplit].lowRowMode == lowRowNormal) return false;
  if (Split[sensorSplit].ccFaders) return false;
  if (Split[sensorSplit].sequencer) return false;
  if (isStrummingSplit(sensorSplit)) return false;

  return true;
}

void initializeLowRowState() {
  for (byte col = 0; col < NUMCOLS; ++col) {
    lowRowColumnState[col] = inactive;
  }
  for (byte split = 0; split < NUMSPLITS; ++split) {
    lowRowSplitState[split] = inactive;
    lowRowBendActive[split] = false;
    lowRowCCXActive[split] = false;
    lowRowCCXYZActive[split] = false;
    lowRowInitialColumn[split] = -1;
    lastRestrikeColumn[split] = 0;
    lowRowSlideLastCol[split] = -1;
    lowRowSlideCumulativeX[split] = 0;
    lowRowSlideLastSentX[split] = 0;
    lowRowSlideCellTransMs[split] = 0;
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
    default:
      return false;
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
      return lowRowSplitState[sensorSplit] == inactive;
    default:
      return false;
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

          short rawXDelta = constrain((sensorCell->calibratedX() - sensorCell->initialX) >> 3, 0, 127);
          short xPosition = calculateFaderValue(sensorCell->calibratedX(), faderLeft, faderLength);

          // X continuity correction: when lowRowSlideFix is enabled and operating in
          // delta (non-fader) mode, bridge calibration gaps at cell handoff boundaries
          // so the output never jumps backward when the finger crosses to a new column.
          short xDelta = rawXDelta;
          if (Global.lowRowSlideFix) {
            if (lowRowSlideLastCol[sensorSplit] != sensorCol) {
              // cell transition: compute an additive offset so output resumes from last sent value
              lowRowSlideCumulativeX[sensorSplit] = lowRowSlideLastSentX[sensorSplit] - rawXDelta;
              lowRowSlideCellTransMs[sensorSplit] = millis();
              lowRowSlideLastCol[sensorSplit] = sensorCol;
            }
            xDelta = constrain(rawXDelta + lowRowSlideCumulativeX[sensorSplit], 0, 127);
            lowRowSlideLastSentX[sensorSplit] = xDelta;
          }

          switch (Split[sensorSplit].lowRowMode)
          {
            case lowRowArpeggiator:
            {
              arpTempoDelta[sensorSplit] = sensorCol - lowRowInitialColumn[sensorSplit];
              break;
            }
            case lowRowBend:
            {
              if (Split[sensorSplit].lowRowBendBehavior == lowRowBendBend) {
                if (pitchBend != SHRT_MAX) {
                  preSendPitchBend(sensorSplit, pitchBend);
                }
              }
              else if (Split[sensorSplit].lowRowBendBehavior == lowRowBendTranspose) {
                startBufferedLeds();

                // if both splits are set to low row transpose, and split is active,
                // combine both split's low rows into a single full-width low row transpose
                // that changes the pitch transpose of both splits
                if (Split[LEFT].lowRowMode == lowRowBend && Split[LEFT].lowRowBendBehavior == lowRowBendTranspose &&
                    Split[RIGHT].lowRowMode == lowRowBend && Split[RIGHT].lowRowBendBehavior == lowRowBendTranspose &&
                    (Global.splitActive || displayMode == displaySplitPoint)) {
                  lowCol = 1;
                  highCol = NUMCOLS;
                  signed char pitch = sensorCol - (lowCol + (highCol - lowCol - 1) / 2);
                  Split[LEFT].transposePitch = pitch;
                  Split[RIGHT].transposePitch = pitch;
                  paintLowRowTranspose(Global.currentPerSplit);
                }
                // otherwise treat the low row transpose specifically for the appropriate split
                else {
                  Split[sensorSplit].transposePitch = sensorCol - (lowCol + (highCol - lowCol - 1) / 2);
                  paintLowRowTranspose(sensorSplit);
                }

                paintOctaveTransposeLed();
                finishBufferedLeds();
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
              // Y/Z gating: suppress Y and Z for LOWROW_YZ_GATE_MS after a cell transition
              // to avoid spurious jumps while the finger straddles two sensors
              boolean inTransition = Global.lowRowSlideFix &&
                                     (millis() - lowRowSlideCellTransMs[sensorSplit]) < LOWROW_YZ_GATE_MS;

              short xOut;
              if (Split[sensorSplit].lowRowXYZAbsoluteX) {
                xOut = calculateFaderValue(sensorCell->calibratedX(), lowCol, highCol - lowCol);
              }
              else if (Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCFader) {
                if (faderLength > 0) {
                  sendLowRowCCXYZ(xPosition, inTransition ? SHRT_MAX : timbre, inTransition ? SHRT_MAX : pressure);
                }
                break;
              }
              else {
                xOut = xDelta;
              }
              sendLowRowCCXYZ(xOut, inTransition ? SHRT_MAX : timbre, inTransition ? SHRT_MAX : pressure);
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
        if (lowRowSplitState[sensorSplit] == pressed && sensorCol == lastRestrikeColumn[sensorSplit]) {
          lowRowSplitState[sensorSplit] = continuous;
        }
        break;
      case lowRowArpeggiator:
      case lowRowSustain:
      case lowRowBend:
      case lowRowCCX:
      case lowRowCCXYZ:
        if (lowRowSplitState[sensorSplit] == pressed) {
          lowRowSplitState[sensorSplit] = continuous;
        }
        break;
      case lowRowStrum:
        if (lowRowColumnState[sensorCol] == pressed) {
          lowRowColumnState[sensorCol] = continuous;
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
    paintCCFaderDisplayRow(sensorSplit, sensorRow, Split[sensorSplit].colorLowRow, Split[sensorSplit].ccForLowRow, faderLeft, faderLength, LED_LAYER_LOWROW);
  }

  // send out the MIDI CC
  preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRow, x, false);
}

void sendLowRowCCXYZ(unsigned short x, short y, short z) {
  if (Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCFader) {
    ccFaderValues[sensorSplit][Split[sensorSplit].ccForLowRowX] = x;

    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);
    paintCCFaderDisplayRow(sensorSplit, sensorRow, Split[sensorSplit].colorLowRow, Split[sensorSplit].ccForLowRowX, faderLeft, faderLength, LED_LAYER_LOWROW);
  }

  // send out the MIDI CCs
  preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRowX, x, false);

  if (y != SHRT_MAX) {
    preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRowY, y, false);
  }

  if (z != SHRT_MAX) {
    preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRowZ, z, false);
  }
}

void handleLowRowRestrike() {
  // we're processing a cell that's not on the low-row, check if column 0 was pressed
  // and retrigger in that case
  if (sensorCell->hasNote() && lowRowSplitState[sensorSplit] == pressed) {
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
  if (sensorCell->hasNote() && lowRowColumnState[sensorCol] == pressed) {
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
      lowRowSplitState[sensorSplit] = pressed;
      lastRestrikeColumn[sensorSplit] = sensorCol;
      cell(0, 0).velocity = sensorCell->velocity;
      break;
    case lowRowStrum:
      lowRowColumnState[sensorCol] = pressed;
      break;
    case lowRowArpeggiator:
      if (lowRowSplitState[sensorSplit] == inactive) {
        if (-1 == lowRowInitialColumn[sensorSplit]) {
          lowRowInitialColumn[sensorSplit] = sensorCol;
        }

        lowRowSplitState[sensorSplit] = pressed;
        if (!Split[sensorSplit].arpeggiator) {
          temporarilyEnableArpeggiator();
        }
      }
      startLowRowContinuousExpression();
      break;
    case lowRowSustain:
      lowRowSplitState[sensorSplit] = pressed;
      preSendSustain(sensorSplit, 127);
      break;
    case lowRowBend:
      if (Split[sensorSplit].lowRowBendBehavior == lowRowBendBend) {
        lowRowBendActive[sensorSplit] = true;
        preResetLastMidiPitchBend(sensorSplit);
        startLowRowContinuousExpression();
      }
      break;
    case lowRowCCX:
      lowRowCCXActive[sensorSplit] = true;
      lowRowSlideLastCol[sensorSplit] = sensorCol;
      lowRowSlideCumulativeX[sensorSplit] = 0;
      lowRowSlideLastSentX[sensorSplit] = 0;
      preResetLastMidiCC(sensorSplit, Split[sensorSplit].ccForLowRow);
      startLowRowContinuousExpression();
      break;
    case lowRowCCXYZ:
      lowRowCCXYZActive[sensorSplit] = true;
      lowRowSlideLastCol[sensorSplit] = sensorCol;
      lowRowSlideCumulativeX[sensorSplit] = 0;
      lowRowSlideLastSentX[sensorSplit] = 0;
      preResetLastMidiCC(sensorSplit, Split[sensorSplit].ccForLowRowX);
      preResetLastMidiCC(sensorSplit, Split[sensorSplit].ccForLowRowY);
      preResetLastMidiCC(sensorSplit, Split[sensorSplit].ccForLowRowZ);
      startLowRowContinuousExpression();
      break;
  }

  updateSwitchLeds();
}

void startLowRowContinuousExpression() {
  if (lowRowSplitState[sensorSplit] != inactive) {
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
    lowRowSplitState[sensorSplit] = pressed;
  }
}

void lowRowStop() {
  switch (Split[sensorSplit].lowRowMode)
  {
    case lowRowRestrike:
      if (lastRestrikeColumn[sensorSplit] && sensorCol == lastRestrikeColumn[sensorSplit]) {
        lowRowSplitState[sensorSplit] = inactive;
        lastRestrikeColumn[sensorSplit] = 0;
        cell(0, 0).velocity = 0;
      }
      break;
    case lowRowStrum:
      lowRowColumnState[sensorCol] = inactive;
      break;
    case lowRowSustain:
      lowRowSplitState[sensorSplit] = inactive;
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

        if (lowRowSplitState[sensorSplit] != inactive) {
          switch (Split[sensorSplit].lowRowMode)
          {
            case lowRowArpeggiator:
              arpTempoDelta[sensorSplit] = 0;
              if (!Split[sensorSplit].arpeggiator) {
                disableTemporaryArpeggiator();
              }
              lowRowInitialColumn[sensorSplit] = -1;
              break;
            case lowRowBend:
              if (Split[sensorSplit].lowRowBendBehavior == lowRowBendBend) {
                // reset the pitchbend since no low row touch is active anymore
                lowRowBendActive[sensorSplit] = false;
                preSendPitchBend(sensorSplit, 0);
              }
              break;
            case lowRowCCX:
              lowRowCCXActive[sensorSplit] = false;
              if (Split[sensorSplit].lowRowCCXBehavior == lowRowCCHold) {
                // reset CC for lowRowX since no low row touch is active anymore
                preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRow, 0, false);
              }
              break;
            case lowRowCCXYZ:
              lowRowCCXYZActive[sensorSplit] = false;
              if (Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCHold) {
                // reset CCs for lowRowXYZ since no low row touch is active anymore
                preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRowX, 0, false);
                preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRowY, 0, false);
                preSendControlChange(sensorSplit, Split[sensorSplit].ccForLowRowZ, 0, false);
              }
              break;
          }

          lowRowSplitState[sensorSplit] = inactive;
        }
      }
      break;
  }

  updateSwitchLeds();
}

inline boolean isLowRowArpeggiatorPressed(byte split) {
  return Split[split].lowRowMode == lowRowArpeggiator && lowRowSplitState[split] != inactive;
}

inline boolean isLowRowSustainPressed(byte split) {
  return Split[split].lowRowMode == lowRowSustain && lowRowSplitState[split] != inactive;
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
