/*********************** ls_handleTouches: LinnStrument Handle Touch Events ***********************
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
These routines handle the processing of new touch events, continuous updates of touch events and
released touch events
**************************************************************************************************/

void cellTouched(TouchState state) {
  cellTouched(sensorCol, sensorRow, state);
};
void cellTouched(byte col, byte row, TouchState state) {
  // turn on the bit that correspond to the column and row of this cell,
  // this allows us to very quickly find other touched cells and detect
  // phantom key presses without having to evaluate every cell on the board
  if (state != untouchedCell &&
      state != transferCell) {
    // keep track of how many cells are currently touched
    if (!(rowsInColsTouched[col] & (int32_t)(1 << row))) {
      cellsTouched++;
    }

    // flip the bits to indicate that this cell is now touched
    rowsInColsTouched[col] |= (int32_t)(1 << row);
    colsInRowsTouched[row] |= (int32_t)(1 << col);
  }
  // if the state is untouched, turn off the appropriate bit in the
  // bitmasks that track the touched cells
  else {
    // keep track of how many cells are currently touched
    if ((rowsInColsTouched[col] & (int32_t)(1 << row))) {
      cellsTouched--;
    }

    // flip the bits to indicate that this cell is now untouched
    rowsInColsTouched[col] &= ~(int32_t)(1 << row);
    colsInRowsTouched[row] &= ~(int32_t)(1 << col);
  }
  
  // save the touched state for each cell
  cell(col, row).touched = state;
}

#define TRANSFER_SLIDE_PROXIMITY 100

byte countTouchesForMidiChannel(byte split, byte col, byte row) {
  if (!cell(col, row).hasNote()) {
    return 0;
  }

  return noteTouchMapping[split].getMusicalTouchCount(cell(col, row).channel);
}

const int32_t PENDING_RELEASE_RATE_X = FXD_FROM_INT(5);

boolean potentialSlideTransferCandidate(byte col) {
  if (controlModeActive) return false;
  if (col < 1) return false;
  if (userFirmwareActive) {
    if (!userFirmwareSlideMode[sensorRow]) return false;
  }
  else if (Split[Global.currentPerSplit].sequencer) {
    if (!requiresSequencerSlideTracking()) return false;
  }
  else {
    if (sensorSplit != getSplitOf(col)) return false;
    if (!isLowRow() &&                                                   // don't perform slide transfers
        (!Split[sensorSplit].sendX ||                                    // if pitch slides are disabled
         !isFocusedCell(col, sensorRow) ||                               // if this is not a focused cell
         countTouchesForMidiChannel(sensorSplit, col, sensorRow) > 1)) { // when there are several touches for the same MIDI channel
      return false;
    }
    if (isLowRow() && !lowRowRequiresSlideTracking()) return false;
    if (isStrummingSplit(sensorSplit)) return false;
  }

  if (cell(col, sensorRow).pendingReleaseCount &&                        // if there's a pending release but not enough X change
      cell(col, sensorRow).fxdRateX <= PENDING_RELEASE_RATE_X) {
    return false;
  }

  return cell(col, sensorRow).touched != untouchedCell &&                                                    // the sibling cell has an active touch
    (cell(col, sensorRow).pendingReleaseCount ||                                                             // either a release is pending to be performed, or
     abs(sensorCell->calibratedX() - cell(col, sensorRow).currentCalibratedX) < TRANSFER_SLIDE_PROXIMITY);   // both cells are touched simultaneously on the edges
}

boolean isReadyForSlideTransfer(byte col) {
  return cell(col, sensorRow).pendingReleaseCount ||                 // there's a pending release waiting
    sensorCell->currentRawZ > cell(col, sensorRow).currentRawZ;      // the cell pressure is higher
}

boolean hasImpossibleX() {             // checks whether the calibrated X is outside of the possible bounds for the current cell
  return Device.calibrated &&
    (sensorCell->calibratedX() < FXD_TO_INT(Device.calRows[sensorCol][0].fxdReferenceX - FXD_CALX_PHANTOM_RANGE) ||
     sensorCell->calibratedX() > FXD_TO_INT(Device.calRows[sensorCol][0].fxdReferenceX + FXD_CALX_PHANTOM_RANGE));
}

void transferFromSameRowCell(byte col) {
  TouchInfo* fromCell = &cell(col, sensorRow);

  sensorCell->lastTouch = fromCell->lastTouch;
  sensorCell->didMove = fromCell->didMove;
  sensorCell->initialX = fromCell->initialX;
  sensorCell->initialColumn = fromCell->initialColumn;  
  sensorCell->quantizationOffsetX = 0; // as soon as we transfer to an adjacent cell, the pitch quantization is reset to play the absolute pitch position instead
  sensorCell->lastMovedX = fromCell->lastMovedX;
  sensorCell->fxdRateX = fromCell->fxdRateX;
  sensorCell->fxdRateCountX = fromCell->fxdRateCountX;
  sensorCell->slideTransfer = true;
  sensorCell->rogueSweepX = fromCell->rogueSweepX;
  sensorCell->initialY = fromCell->initialY;
  sensorCell->note = fromCell->note;
  sensorCell->channel = fromCell->channel;
  sensorCell->octaveOffset = fromCell->octaveOffset;
  sensorCell->fxdPrevPressure = fromCell->fxdPrevPressure;
  sensorCell->fxdPrevTimbre = fromCell->fxdPrevTimbre;
  sensorCell->velocity = fromCell->velocity;
  sensorCell->vcount = fromCell->vcount;
  noteTouchMapping[sensorSplit].changeCell(sensorCell->note, sensorCell->channel, sensorCol, sensorRow);

  fromCell->lastTouch = 0;
  fromCell->didMove = false;
  fromCell->initialX = INVALID_DATA;
  fromCell->initialColumn = -1;
  fromCell->quantizationOffsetX = 0;
  fromCell->lastMovedX = 0;
  fromCell->fxdRateX = 0;
  fromCell->fxdRateCountX = 0;
  fromCell->slideTransfer = true;
  fromCell->rogueSweepX = false;
  fromCell->initialY = -1;
  fromCell->pendingReleaseCount = 0;

  fromCell->note = -1;
  fromCell->channel = -1;
  fromCell->octaveOffset = 0;
  fromCell->fxdPrevPressure = 0;
  fromCell->fxdPrevTimbre = FXD_CONST_255;
  fromCell->velocity = 0;
  // do not reset vcount!

  signed char channel = sensorCell->channel;
  if (channel > 0 && col == focus(sensorSplit, channel).col && sensorRow == focus(sensorSplit, channel).row) {
    focus(sensorSplit, channel).col = sensorCol;
    focus(sensorSplit, channel).row = sensorRow;
  }
}

void transferToSameRowCell(byte col) {
  TouchInfo* toCell = &cell(col, sensorRow);
  
  toCell->lastTouch = sensorCell->lastTouch;
  toCell->didMove = sensorCell->didMove;
  toCell->initialX = sensorCell->initialX;
  toCell->initialColumn = sensorCell->initialColumn;
  toCell->quantizationOffsetX = 0; // as soon as we transfer to an adjacent cell, the pitch quantization is reset to play the absolute pitch position instead
  toCell->lastMovedX = sensorCell->lastMovedX;
  toCell->fxdRateX = sensorCell->fxdRateX;
  toCell->fxdRateCountX = sensorCell->fxdRateCountX;
  toCell->slideTransfer = true;
  toCell->rogueSweepX = sensorCell->rogueSweepX;
  toCell->initialY = sensorCell->initialY;
  toCell->note = sensorCell->note;
  toCell->channel = sensorCell->channel;
  toCell->octaveOffset = sensorCell->octaveOffset;
  toCell->fxdPrevPressure = sensorCell->fxdPrevPressure;
  toCell->fxdPrevTimbre = sensorCell->fxdPrevTimbre;
  toCell->velocity = sensorCell->velocity;
  toCell->vcount = sensorCell->vcount;
  noteTouchMapping[sensorSplit].changeCell(toCell->note, toCell->channel, col, sensorRow);

  sensorCell->lastTouch = 0;
  sensorCell->didMove = false;
  sensorCell->initialX = INVALID_DATA;
  sensorCell->initialColumn = -1;
  sensorCell->quantizationOffsetX = 0;
  sensorCell->lastMovedX = 0;
  sensorCell->fxdRateX = 0;
  sensorCell->fxdRateCountX = 0;
  sensorCell->slideTransfer = true;
  sensorCell->rogueSweepX = false;
  sensorCell->initialY = -1;
  sensorCell->pendingReleaseCount = 0;

  sensorCell->note = -1;
  sensorCell->channel = -1;
  sensorCell->octaveOffset = 0;
  sensorCell->fxdPrevPressure = 0;
  sensorCell->fxdPrevTimbre = FXD_CONST_255;
  sensorCell->velocity = 0;
  // do not reset vcount!

  signed char channel = toCell->channel;
  if (channel > 0 && sensorCol == focus(sensorSplit, channel).col && sensorRow == focus(sensorSplit, channel).row) {
    focus(sensorSplit, channel).col = col;
    focus(sensorSplit, channel).row = sensorRow;
  }
}

boolean isPhantomTouchIndividual() {
  // when the device is calibrated we fully rely on the plausability of the X readings to determine
  // if a touch is a phantom touch or not
  if (Device.calibrated) {
    if (hasImpossibleX()) {
      sensorCell->setPhantoms(sensorCol, sensorCol, sensorRow, sensorRow);
      return true;
    }
  }
  return false;
}

boolean isPhantomTouchContextual() {
  // check if this is a potential corner of a rectangle to filter out ghost notes, this first check matches
  // any cells that have other cells on the same row and column, so it's not sufficient by itself, but it's fast
  int32_t rowsInSensorColTouched = rowsInColsTouched[sensorCol] & ~(int32_t)(1 << sensorRow);
  int32_t colsInSensorRowTouched = colsInRowsTouched[sensorRow] & ~(int32_t)(1 << sensorCol);
  if (rowsInSensorColTouched && colsInSensorRowTouched) {

    // now we check each touched row in the column of the current sensor
    // we gradually flip the touched bits to zero until they're all turned off
    // this allows us to loop only over the touched rows, and none other
    while (rowsInSensorColTouched) {
      // we use the ARM Cortex-M3 instruction that reports the leading bit zeros of any number
      // we determine that the left-most bit is that is turned on by substracting the leading zero
      // count from the bitdepth of a 32-bit int
      byte touchedRow = 31 - __builtin_clz(rowsInSensorColTouched);

      // for each touched row we also check each touched column in the row of the current sensor
      int32_t colsInRowTouched = colsInSensorRowTouched;

      // we use the same looping approach as explained for the rows
      while (colsInRowTouched) {

        // we use the same leading zeros approach to dermine the left-most active bit
        byte touchedCol = 31 - __builtin_clz(colsInRowTouched);

        // if we find a cell that has both the touched row and touched column set,
        // then the current sensor completed a rectangle by being the fourth corner
        if (rowsInColsTouched[touchedCol] & (int32_t)(1 << touchedRow)) {

          // since we found four corners, we now have to determine which ones are
          // real presses and which ones are phantom presses, so we're looking for
          // the other corner that was scanned twice to determine which one has the
          // lowest pressure, this is the most likely to be the phantom press
          if ((cell(touchedCol, touchedRow).isHigherPhantomPressure(sensorCell->currentRawZ) &&
               cell(sensorCol, touchedRow).isHigherPhantomPressure(sensorCell->currentRawZ) &&
               cell(touchedCol, sensorRow).isHigherPhantomPressure(sensorCell->currentRawZ))) {

            // store coordinates of the rectangle, which also serves as an indicator that we
            // should stop looking for a phantom press
            cell(sensorCol, sensorRow).setPhantoms(sensorCol, touchedCol, sensorRow, touchedRow);
            cell(touchedCol, touchedRow).setPhantoms(sensorCol, touchedCol, sensorRow, touchedRow);
            cell(sensorCol, touchedRow).setPhantoms(sensorCol, touchedCol, sensorRow, touchedRow);
            cell(touchedCol, sensorRow).setPhantoms(sensorCol, touchedCol, sensorRow, touchedRow);

            return true;
          }
        }

        // turn the left-most active bit off, to continue the iteration over the touched columns
        colsInRowTouched &= ~(1 << touchedCol);
      }

      // turn the left-most active bit off, to continue the iteration over the touched rows
      rowsInSensorColTouched &= ~(1 << touchedRow);
    }
  }
  return false;
}

byte countTouchesInColumn() {
  byte count = 0;
  int32_t rowsInSensorColTouched = rowsInColsTouched[sensorCol];
  if (rowsInSensorColTouched) {
    while (rowsInSensorColTouched) {
      byte touchedRow = 31 - __builtin_clz(rowsInSensorColTouched);
      count++;

      // turn the left-most active bit off, to continue the iteration over the touched rows
      rowsInSensorColTouched &= ~(int32_t)(1 << touchedRow);
    }
  }

  return count;
}

boolean hasOtherTouchInSplit(byte split) {
  for (int r = 0; r < NUMROWS; ++r) {
    int32_t colsInRowTouchedAdapted = colsInRowsTouched[r];
    if (r == sensorRow) {
      colsInRowTouchedAdapted &= ~(int32_t)(1 << sensorCol);
    }

    if (colsInRowTouchedAdapted) {
      // if split is not active and there's a touch on the row, it's obviously in the current split
      if (!Global.splitActive) {
        return true;
      }

      // determine which columns need to be active in the touched row for this to be considered
      // part of either split
      if (split == LEFT && (colsInRowTouchedAdapted & ((int32_t)(1 << Global.splitPoint) - 1))) {
        return true;
      }
      if (split == RIGHT && (colsInRowTouchedAdapted & ~((int32_t)(1 << Global.splitPoint) - 1))) {
        return true;
      }
    }
  }
  return false;
}

boolean hasTouchInSplitOnRow(byte split, byte row) {
  if (colsInRowsTouched[row]) {
    // if split is not active and there's a touch on the row, it's obviously in the current split
    if (!Global.splitActive) {
      return true;
    }

    // determine which columns need to be active in the touched row for this to be considered
    // part of either split
    if (split == LEFT && (colsInRowsTouched[row] & ((int32_t)(1 << Global.splitPoint) - 1))) {
      return true;
    }
    if (split == RIGHT && (colsInRowsTouched[row] & ~((int32_t)(1 << Global.splitPoint) - 1))) {
      return true;
    }
  }

  return false;
}

void handleSlideTransferCandidate(byte siblingCol) {
  // if the pressure gets higher than adjacent cell, the slide is transitioning over
  if (isReadyForSlideTransfer(siblingCol)) {
    transferFromSameRowCell(siblingCol);
 
    // if a slide transfer happened, but the pitch hold was still quantized, reset the
    // X rate and threshold exceed count so that the real X position will be used as soon as
    // the transfer cell is active, this makes the onset of slides from a stationary position
    // smoother when quantize hold is on
    if (fxdRateXThreshold[sensorSplit] - sensorCell->fxdRateX > 0) {
      sensorCell->fxdRateX = fxdRateXThreshold[sensorSplit];
      sensorCell->fxdRateCountX = 0;
    }

    if (userFirmwareActive) {
      // if user firmware is active, we implement a particular transition scheme to allow touches to be tracked over MIDI
      sensorCell->note = sensorCol;
      midiSendControlChange(119, siblingCol, sensorCell->channel, true);
      midiSendNoteOn(LEFT, sensorCol, sensorCell->velocity, sensorCell->channel);
      midiSendNoteOffWithVelocity(LEFT, siblingCol, sensorCol, sensorCell->channel);
    }
    else {
      if (Split[sensorSplit].colorPlayed && Split[sensorSplit].playedTouchMode == playedCell) {
        setLed(siblingCol, sensorRow, COLOR_OFF, cellOff, LED_LAYER_PLAYED);
        if (cell(sensorCol, sensorRow).hasNote()) {
          setLed(sensorCol, sensorRow, Split[sensorSplit].colorPlayed, cellOn, LED_LAYER_PLAYED);
        }
      }
    }

    if (cell(siblingCol, sensorRow).touched != untouchedCell) {
      cellTouched(siblingCol, sensorRow, transferCell);
    }
    
    handleXYZupdate();
  }
  // otherwise act as if this new touch never happend
  else {
    cellTouched(transferCell);
  }
}

boolean handleNewTouch() {
  DEBUGPRINT((1,"handleNewTouch"));
  DEBUGPRINT((1," col="));DEBUGPRINT((1,(int)sensorCol));
  DEBUGPRINT((1," row="));DEBUGPRINT((1,(int)sensorRow));
  DEBUGPRINT((1," velocityZ="));DEBUGPRINT((1,(int)sensorCell->velocityZ));
  DEBUGPRINT((1," pressureZ="));DEBUGPRINT((1,(int)sensorCell->pressureZ));
  DEBUGPRINT((1,"\n"));

  lastTouchMoment = millis();
  
  // if the touches are restricted to a particular row, any touch outside this row is ignored
  if (restrictedRow != -1 && sensorRow != restrictedRow) {
    cellTouched(ignoredCell);
    return false;
  }

  // allow any new touch to cancel scrolling
  if (animationActive) {
    stopAnimation = true;
    cellTouched(ignoredCell);
    return false;
  }

  // any touch will wake up LinnStrument again, and should be ignored
  if (displayMode == displaySleep) {
    cellTouched(ignoredCell);
    setDisplayMode(displayNormal);
    updateDisplay();
    return false;
  }

  boolean result = false;

  cellTouched(touchedCell);                                 // mark this cell as touched

  // if it's a command button, handle it
  if (sensorCol == 0) {
    if (controlModeActive) {
      switchSerialMode(false);
      return false;
    }
    
    // check if we should activate sleep mode
    if ((sensorRow == GLOBAL_SETTINGS_ROW && cell(0, PER_SPLIT_ROW).touched == touchedCell) ||
        (sensorRow == PER_SPLIT_ROW && cell(0, GLOBAL_SETTINGS_ROW).touched == touchedCell)) {
      activateSleepMode();
      return false;
    }

    // user firmware mode only handles the global settings command button
    if (!userFirmwareActive || sensorRow == GLOBAL_SETTINGS_ROW) {
      if (sensorRow != SWITCH_1_ROW &&                        // if commands buttons are pressed that are not the two switches
          sensorRow != SWITCH_2_ROW) {                        // only activate them if there's note being played on the playing surface
        for (int r = 0; r < NUMROWS; ++r) {                   // this prevents accidental settings modifications while playing
          if ((colsInRowsTouched[r] & ~(int32_t)(1)) != 0) {
            cellTouched(ignoredCell);
            return false;
          }
        }
      }
      handleControlButtonNewTouch();
    }
  }
  else {                                                    // or if it's in column 1-25...
    switch (displayMode)
    {
      case displaySplitPoint:                                 // if the Split button is held, this touch changes the split point
        if (splitButtonDown) {
          handleSplitPointNewTouch();
          break;
        }
        // If we get here, we're displaying in displaySplitPoint mode, but we've just gotten a normal new touch.
        // THE FALL THROUGH HERE (no break statement) IS PURPOSEFUL!

      case displayNormal:                                            // it's normal performance mode
      case displayVolume:                                            // it's a volume change

        // check if the new touch could be an ongoing slide to the right
        if (potentialSlideTransferCandidate(sensorCol-1)) {
          handleSlideTransferCandidate(sensorCol-1);
        }
        // check if the new touch could be an ongoing slide to the left
        else if (potentialSlideTransferCandidate(sensorCol+1)) {
          handleSlideTransferCandidate(sensorCol+1);
        }
        // only allow a certain number of touches in a single column to prevent cross talk
        else if (countTouchesInColumn() > MAX_TOUCHES_IN_COLUMN) {
          cellTouched(ignoredCell);
        }
        // this is really a new touch without any relationship to an ongoing slide
        // however, it could be the low row and in certain situations it doesn't allow new touches
        else if (!isLowRow() || allowNewTouchOnLowRow()) {
          initVelocity();
          calcVelocity(sensorCell->velocityZ);
          result = true;
        }
        else {
          cellTouched(untouchedCell);
        }

        break;
      default:
        initVelocity();
        calcVelocity(sensorCell->velocityZ);
        result = true;
        break;
    }
  }

  return result;
}

// Calculate the transposed note number for the current cell by taken the transposition settings into account
short cellTransposedNote(byte split) {
  return transposedNote(split, sensorCol, sensorRow);
}

short transposedNote(byte split, byte col, byte row) {
  return getNoteNumber(split, col, row) + Split[split].transposePitch + Split[split].transposeOctave;
}

// Check if the currently scanned cell is a focused cell
boolean isFocusedCell() {
  return isFocusedCell(sensorCol, sensorRow);
}

// Check if a specific cell is a focused cell
boolean isFocusedCell(byte col, byte row) {
  if (cell(col, row).channel < 1) {
    return false;
  }

  FocusCell& focused = focus(getSplitOf(col), cell(col, row).channel);
  return col == focused.col && row == focused.row;
}

// Check if X expression should be sent for this cell
boolean isXExpressiveCell() {
  return isFocusedCell();
}

boolean isXExpressiveCell(byte col, byte row) {
  return isFocusedCell(col, row);
}

// Check if Y expression should be sent for this cell
boolean isYExpressiveCell() {
  if (Split[sensorSplit].expressionForY == timbrePolyPressure) {
    return true;
  }
  else {
    return isFocusedCell();
  }
}

// Check if Z expression should be sent for this cell
boolean isZExpressiveCell() {
  if (Split[sensorSplit].expressionForZ == loudnessPolyPressure) {
    return true;
  }
  else {
    return isFocusedCell();
  }
}

byte takeChannel(byte split, byte row) {
  switch (Split[split].midiMode)
  {
    case channelPerNote:
    {
      return splitChannels[split].take();
    }

    case channelPerRow:
    {
      byte channel = Split[split].midiChanPerRow;
      if (Split[split].midiChanPerRowReversed) {
        channel += (NUMROWS - 1) - row;
      }
      else {
        channel += row;
      }
      if (channel > 16) {
        channel -= 16;
      }
      return channel;
    }

    case oneChannel:
    default:
    {
      return Split[split].midiChanMain;
    }
  }
}

void handleNonPlayingTouch() {
  switch (displayMode) {
    case displayNormal:
    case displaySplitPoint:
    case displayVolume:
    case displayReset:
    case displayAnimation:
    case displaySleep:
      // handled elsewhere
      break;
    case displayPerSplit:
      handlePerSplitSettingNewTouch();
      break;
    case displayPreset:
      handlePresetNewTouch();
      break;
    case displayBendRange:
      handleBendRangeNewTouch();
      break;
    case displayLimitsForY:
      handleLimitsForYNewTouch();
      break;
    case displayCCForY:
      handleCCForYNewTouch();
      break;
    case displayInitialForRelativeY:
      handleInitialForRelativeYNewTouch();
      break;
    case displayLimitsForZ:
      handleLimitsForZNewTouch();
      break;
    case displayCCForZ:
      handleCCForZNewTouch();
      break;
    case displayPlayedTouchModeConfig:
      handlePlayedTouchModeNewTouch();
      break;
    case displayCCForFader:
      handleCCForFaderNewTouch();
      break;
    case displayLowRowCCXConfig:
      handleLowRowCCXConfigNewTouch();
      break;
    case displayLowRowCCXYZConfig:
      handleLowRowCCXYZConfigNewTouch();
      break;
    case displayCCForSwitchCC65:
      handleCCForSwitchCC65ConfigNewTouch();
      break;
    case displayCCForSwitchSustain:
      handleCCForSwitchSustainConfigNewTouch();
      break;
    case displayCustomSwitchAssignment:
      handleCustomSwitchAssignmentConfigNewTouch();
      break;
    case displayLimitsForVelocity:
      handleLimitsForVelocityNewTouch();
      break;
    case displayValueForFixedVelocity:
      handleValueForFixedVelocityNewTouch();
      break;
    case displaySleepConfig:
      handleSleepConfigNewTouch();
      break;
    case displaySplitHandedness:
      handleSplitHandednessNewTouch();
      break;
    case displayRowOffset:
      handleRowOffsetNewTouch();
      break;
    case displayGuitarTuning:
      handleGuitarTuningNewTouch();
      break;
    case displayMinUSBMIDIInterval:
      handleMinUSBMIDIIntervalNewTouch();
      break;
    case displayMIDIThrough:
      handleMIDIThroughNewTouch();
      break;
    case displaySensorSensitivityZ:
      handleSensorSensitivityZNewTouch();
      break;
    case displaySensorLoZ:
      handleSensorLoZNewTouch();
      break;
    case displaySensorFeatherZ:
      handleSensorFeatherZNewTouch();
      break;
    case displaySensorRangeZ:
      handleSensorRangeZNewTouch();
      break;
    case displayOctaveTranspose:
      handleOctaveTransposeNewTouch();
      break;
    case displayGlobal:
    case displayGlobalWithTempo:
      handleGlobalSettingNewTouch();
      break;
    case displayOsVersion:
      setDisplayMode(displayOsVersionBuild);
      updateDisplay();
      break;
    case displayOsVersionBuild:
      setDisplayMode(displayOsVersion);
      updateDisplay();
      break;
    case displayCalibration:
      initVelocity();
      break;
    case displayEditAudienceMessage:
      handleEditAudienceMessageNewTouch();
      break;
    case displaySequencerProjects:
      handleSequencerProjectsNewTouch();
      break;
    case displaySequencerDrum0107:
      handleSequencerDrum0107NewTouch();
      break;
    case displaySequencerDrum0814:
      handleSequencerDrum0814NewTouch();
      break;
    case displaySequencerColors:
      handleSequencerColorsNewTouch();
      break;
    case displayCustomLedsEditor:
      handleCustomLedsEditorNewTouch();
      break;
  }
}

// handleXYZupdate:
// Called when a cell is held, in order to read X, Y or Z movements and send MIDI messages as appropriate
// Returns a flag to indicate if the performance loop can be short-circuited
boolean handleXYZupdate() {
  // if the touch is in the control buttons column, ignore it
  if (sensorCol == 0 &&
     // except for user firmware mode where only the global settings button is ignored for continuous updates
     (!userFirmwareActive || sensorRow == GLOBAL_SETTINGS_ROW)) return false;

  // if this data point serves as a calibration sample, return immediately
  if (handleCalibrationSample()) return false;

  // some features need hold functionality
  if (sensorCell->velocity) {
    switch (displayMode) {
      case displayPerSplit:
        handlePerSplitSettingHold();
        return false;

      case displayPreset:
        handlePresetHold();
        return false;

      case displayGlobal:
      case displayGlobalWithTempo:
        handleGlobalSettingHold();
        return false;

      case displaySequencerProjects:
        handleSequencerProjectsHold();
        break;

      case displaySensorSensitivityZ:
        handleSensorSensitivityZHold();
        break;

      case displayCustomLedsEditor:
        handleCustomLedsEditorHold();
        return false;

      default:
        // other displays don't need hold features
        break;
    }
  }
  
  VelocityState velState = calcVelocity(sensorCell->velocityZ);

  // velocity calculation works in stages, handle each one
  boolean newVelocity = false;
  switch (velState) {
    // when the velocity is being calculated, the performance loop can be short-circuited
    case velocityCalculating:
      return true;

    case velocityNew:
      if (isPhantomTouchIndividual() || isPhantomTouchContextual()) {
        cellTouched(untouchedCell);
        return false;
      }

      // mark this as a valid new velocity and process it as such further down the method
      newVelocity = true;
      break;

    case velocityCalculated:
      // velocity has been calculated, no need to short-circuit anymore and we can continue
      // with the main touch logic
      break;
  }

  // only continue if the active display modes require finger tracking
  if (displayMode != displayNormal &&
      displayMode != displayVolume &&
      (displayMode != displaySplitPoint || splitButtonDown)) {
    // check if this should be handled as a non-playing touch
    if (newVelocity) {
      handleNonPlayingTouch();
      performContinuousTasks();
    }
    return false;
  }

  DEBUGPRINT((2,"handleXYZupdate"));
  DEBUGPRINT((2," col="));DEBUGPRINT((2,(int)sensorCol));
  DEBUGPRINT((2," row="));DEBUGPRINT((2,(int)sensorRow));
  DEBUGPRINT((2," velocityZ="));DEBUGPRINT((2,(int)sensorCell->velocityZ));
  DEBUGPRINT((2," pressureZ="));DEBUGPRINT((2,(int)sensorCell->pressureZ));
  DEBUGPRINT((2,"\n"));

  lastTouchMoment = millis();
    
  // turn off note handling and note expression features for low row, volume, cc faders and strumming
  boolean handleNotes = true;
  // in user firmware mode, everything is always encoded as MIDI notes and information
  if (userFirmwareActive) {
    handleNotes = true;
  }
  // in regular firmware mode, some features need special non-MIDI note handling
  else if (isLowRow() ||
      displayMode == displayVolume ||
      Split[sensorSplit].ccFaders ||
      Split[Global.currentPerSplit].sequencer ||
      isStrummingSplit(sensorSplit)) {
    handleNotes = false;
  }

  // this cell corresponds to a playing note
  if (newVelocity) {
    sensorCell->lastTouch = millis();
    sensorCell->didMove = false;
    sensorCell->lastMovedX = 0;
    sensorCell->lastValueX = INVALID_DATA;
    sensorCell->shouldRefreshX = true;
    sensorCell->initialX = INVALID_DATA;
    sensorCell->quantizationOffsetX = 0;
    sensorCell->fxdRateCountX = fxdPitchHoldSamples[sensorSplit];

    if (userFirmwareActive) {
      handleNewUserFirmwareTouch();
    }
    else if (controlModeActive) {
      handleNewControlModeTouch();
    }
    // is this cell used for low row functionality
    else if (isLowRow()) {
      lowRowStart();
    }
    // Split strum only triggers notes in the other split
    else if (isStrummingSplit(sensorSplit)) {
      handleSplitStrum();
    }
    else if (handleNotes) {
      short notenum = cellTransposedNote(sensorSplit);

      // if there was a previous note and automatic octave switching is enabled,
      // check if the conditions are met to change the octave up or down while playing
      if (latestNoteNumberForAutoOctave != -1 && isSwitchAutoOctavePressed(sensorSplit)) {
        short octaveChange = 0;

        // if the previous note was at least a perfect fifth lower, transpose one octave down
        // since the arpeggio would be in a downward movement
        if (notenum - latestNoteNumberForAutoOctave >= 7) {
          octaveChange = -12;
        }
        // if the previous note was at least a perfect fifth higher, transpose one octave up
        // since the arpeggio would be in a upward movement
        else if (notenum - latestNoteNumberForAutoOctave <= -7) {
          octaveChange = 12;
        }

        // apply the automatic octave change and adapt the note number
        if (octaveChange != 0) {
          Split[sensorSplit].transposeOctave = constrain(Split[sensorSplit].transposeOctave + octaveChange, -60, 60);
          notenum += octaveChange;

          // switching octaves might turn off some note cells since they fall outside of the MIDI note range
          updateDisplay();
        }
      }

      // if the note number is outside of MIDI range, don't start it
      if (notenum >= 0 && notenum <= 127) {
        prepareNewNote(notenum);
      }
    }
  }

  // we don't need to handle any expression in control mode
  if (controlModeActive && !newVelocity) {
    return false;
  }

  // get the processed expression data
  short valueX = INVALID_DATA;
  short valueY = INVALID_DATA;
  unsigned short valueZHi = handleZExpression();
  byte valueZ = scale1016to127(valueZHi, true);
  performContinuousTasks();

  // Only process x and y data when there's meaningful pressure on the cell
  if (sensorCell->isMeaningfulTouch() || (doQuantizeHold() && isQuantizeHoldStable())) {
    valueX = handleXExpression();
    if (valueX != INVALID_DATA && isLeftHandedSplit(sensorSplit)) {
      valueX = -1 * valueX;
    }

    performContinuousTasks();
    sensorCell->lastValueX = valueX;
  }

  short tempY = handleYExpression();;
  if (tempY == 0 || tempY == 127 || sensorCell->isMeaningfulTouch()) {
    valueY = tempY;
  }
  performContinuousTasks();

  // update the low row state, but not for the low row cells themselves when there's a new velocity
  // this is handled in lowRowStart, and immediately calling handleLowRowState will wrongly handle the
  // low row state transitions
  if ((!newVelocity || !isLowRow()) && !userFirmwareActive) {
    handleLowRowState(newVelocity, valueX, valueY, valueZ);
  }

  // the volume fader has its own operation mode
  if (displayMode == displayVolume) {
    if (sensorCell->isMeaningfulTouch()) {
      handleVolumeNewTouch(newVelocity);
    }
  }
  else if (Split[sensorSplit].ccFaders && !userFirmwareActive) {
    if (sensorCell->isMeaningfulTouch()) {
      handleFaderTouch(newVelocity);
    }
  }
  else if (Split[Global.currentPerSplit].sequencer && !userFirmwareActive) {
    if (sensorCell->isMeaningfulTouch()) {
      handleSequencerTouch(newVelocity);
    }
  }
  else if (handleNotes && sensorCell->hasNote()) {
    if (userFirmwareActive) {
      // don't send expression data for the control switches
      if (sensorCol != 0) {
        // Z-axis movements are encoded using Poly Pressure with the note as the column and the channel as the row
        if (userFirmwareZActive[sensorRow]) {
          midiSendPolyPressure(sensorCell->note, valueZ, sensorCell->channel);
        }

        // X-axis movements are encoded in 14-bit with MIDI CC 0-25 / 32-57 as the column and the channel as the row
        if (userFirmwareXActive[sensorRow] && valueX != INVALID_DATA) {
          short positionX = valueX + FXD_TO_INT(sensorCell->fxdInitialReferenceX());

          // compensate for the -85 offset at the left side since 0 is positioned at the center of the left-most cell
          positionX = positionX + 85;
          
          midiSendControlChange14BitUserFirmware(sensorCol, sensorCol+32, positionX, sensorCell->channel);
        }

        // Y-axis movements are encoded using MIDI CC 64-89 as the column and the channel as the row
        if (userFirmwareYActive[sensorRow] && valueY != INVALID_DATA) {
          midiSendControlChange(sensorCol+64, valueY, sensorCell->channel);
        }
      }
    }
    else {
      
      // if X-axis movements are enabled and it's a candidate for
      // X/Y expression based on the MIDI mode and the currently held down cells
      if (valueX != INVALID_DATA &&
          Split[sensorSplit].sendX && isXExpressiveCell() && !isLowRowBendActive(sensorSplit)) {

        int pitch = valueX;

        // if there are several touches for the same MIDI channel (for instance in one channel mode)
        // we average the X values to have only one global X value for those touches
        if (countTouchesForMidiChannel(sensorSplit, sensorCol, sensorRow) > 2) {

          // start with the current sensor's pitch and note
          int highestNotePitch = valueX;
          signed char highestNote = sensorCell->note;

          // start with the current sensor's X value
          long averagePitch = valueX;
          byte averageDivider = 1;

          // iterate over all the rows
          for (byte row = 0; row < NUMROWS; ++row) {

            // exclude the current sensor for the rest of the logic, we already
            // took it into account
            int32_t colsInRowTouched = colsInRowsTouched[row];
            if (row == sensorRow) {
              colsInRowTouched = colsInRowTouched & ~(1 << sensorCol);
            }

            // continue while there are touched columns in the row
            while (colsInRowTouched) {
              byte touchedCol = 31 - __builtin_clz(colsInRowTouched);
              
              // add the X value of the cell to the average that's being calculated if the cell
              // is on the same channel
              if (cell(touchedCol, row).touched == touchedCell &&
                  cell(touchedCol, row).lastValueX != INVALID_DATA &&
                  cell(touchedCol, row).channel == sensorCell->channel) {

                if (cell(touchedCol, row).note >= highestNote) {
                  highestNote = cell(touchedCol, row).note;
                  highestNotePitch = cell(touchedCol, row).lastValueX;
                }

                averagePitch += cell(touchedCol, row).lastValueX;
                averageDivider++;
              }
              // exclude the cell we just processed by flipping its bit
              colsInRowTouched &= ~(1 << touchedCol);
            }
          }

          // calculate the average pitch of the notes that exclude the highest note
          averagePitch = (averagePitch - highestNotePitch) / (averageDivider - 1);

          // use the pitch that has the most influence
          if (abs(highestNotePitch) > abs(averagePitch)) {
            pitch = highestNotePitch;
          }
          else {
            pitch = averagePitch;
          }
        }

        preSendPitchBend(sensorSplit, pitch, sensorCell->channel);
      }

      // if Y-axis movements are enabled and it's a candidate for
      // X/Y expression based on the MIDI mode and the currently held down cells
      if (valueY != INVALID_DATA &&
          Split[sensorSplit].sendY && isYExpressiveCell()) {
        preSendTimbre(sensorSplit, valueY, sensorCell->note, sensorCell->channel);
      }

      // send the note on if this in a newly calculated velocity
      if (newVelocity) {
        if (isStrummedSplit(sensorSplit)) {
          handleStrummedRowChange(true, 0);
        }
        else {
          sendNewNote();
        }

        // when the legato switch is pressed and this is the only new touch in the split,
        // release all the latched notes after the new note on message
        if (isSwitchLegatoPressed(sensorSplit) && !hasOtherTouchInSplit(sensorSplit)) {
          noteTouchMapping[sensorSplit].releaseLatched();
        }
      }

      // if sensing Z is enabled...
      // send different pressure update depending on midiMode
      if (Split[sensorSplit].sendZ && isZExpressiveCell()) {
        preSendLoudness(sensorSplit, valueZ, valueZHi, sensorCell->note, sensorCell->channel);
      }

      // after the initial velocity, new velocity values are continuously being calculated simply based
      // on the Z data, during arpeggiation this is handled in the arpeggiator itself in order to have
      // snapshotted velocities that can be latched
      if (!isArpeggiatorEnabled(sensorSplit)) {
        sensorCell->velocity = calcPreferredVelocity(sensorCell->velocityZ);      
      }
    }
  }

  return false;
}

void handleSplitStrum() {
  // handle open strings by checking if no cells are touched in the strummed split,
  // this corresponds to checking of a fret is pushed down on a string, in which case is can't be open
  if (!hasTouchInSplitOnRow(otherSplit(), sensorRow)) {
    handleStrummedOpenRow(otherSplit(), cell(sensorCol, sensorRow).velocity);
  }
  // handle fretted strings
  else {
    handleStrummedRowChange(false, cell(sensorCol, sensorRow).velocity);
  }
}

void handleStrummedOpenRow(byte split, byte velocity) {
  // if a note is already playing for this open string, turn it off so that the exact same note
  // can be played, but with a different velocity
  if (virtualCell().hasNote()) {
    midiSendNoteOff(virtualCell().split, virtualCell().note, virtualCell().channel);
  }
  // since no note was already playing, determined what the note details are
  else {
    virtualCell().split = split;
    virtualCell().note = transposedNote(split, splitLowestEdge(virtualCell().split), sensorRow);
    virtualCell().channel = takeChannel(split, sensorRow);
  }

  // use the velocity of the strum touch
  virtualCell().velocity = velocity;

  if (Split[split].sendX && !isLowRowBendActive(split)) {
    resetLastMidiPitchBend(virtualCell().channel);
    preSendPitchBend(split, 0, virtualCell().channel);
  }
  if (Split[split].sendY) {
    preResetLastTimbre(split, virtualCell().note, virtualCell().channel);
    preSendTimbre(split, 0, virtualCell().note, virtualCell().channel);
  }
  if (Split[split].sendZ) {
    preResetLastLoudness(split, virtualCell().note, virtualCell().channel);
    preSendLoudness(split, 0, 0, virtualCell().note, virtualCell().channel);
  }

  // send a new MIDI note
  midiSendNoteOn(split, virtualCell().note, virtualCell().velocity, virtualCell().channel);
}

void handleStrummedRowChange(boolean newFretting, byte velocity) {
  // we use the bitmask of the touched columns in the current row, and turn off all the columns
  // that belong to the strumming split
  int32_t colsInSensorRowTouched = colsInRowsTouched[sensorRow];
  if (isStrummingSplit(LEFT)) {
    colsInSensorRowTouched &= ~((int32_t)(1 << Global.splitPoint) - 1);
  }
  else if (isStrummingSplit(RIGHT)) {
    colsInSensorRowTouched &= ((int32_t)(1 << Global.splitPoint) - 1);
  }

  byte highestNoteCol = 0;
  byte numNotesTouched = 0;

  // automatically turn of notes for open strings
  if (colsInSensorRowTouched && virtualCell().hasNote()) {
      virtualCell().releaseNote();
      numNotesTouched += 1;
  }

  // if this was not a new fretting and the change is coming from the strummed split,
  // it's a pull-off. The pulled-off cell is already set to untouched, but the note is still
  // sounding, take that into account.
  if (!newFretting && isStrummedSplit(sensorSplit) &&
      sensorCell->hasNote() && hasActiveMidiNote(sensorSplit, sensorCell->note, sensorCell->channel)) {
    numNotesTouched += 1;
  }

  // now we check each touched column in the row of the current sensor
  // we gradually flip the touched bits to zero until they're all turned off
  // this allows us to loop only over the touched columns, and none other
  while (colsInSensorRowTouched) {
    // we use the ARM Cortex-M3 instruction that reports the leading bit zeros of any number
    // we determine that what left-most bit is that is turned on by substracting the leading zero
    // count from the bitdepth of a 32-bit int
    byte touchedCol = 31 - __builtin_clz(colsInSensorRowTouched);
    TouchInfo& cell = cell(touchedCol, sensorRow);
    byte split = getSplitOf(touchedCol);
    if (cell.hasNote()) {
      if (hasActiveMidiNote(split, cell.note, cell.channel)) {
        numNotesTouched += 1;
      }

      // remembed the top-most touch of the row, which will trigger a new note
      if (highestNoteCol == 0) {
        highestNoteCol = touchedCol;
      }
      else {
        // turn off the notes for all the active touches
        midiSendNoteOff(split, cell.note, cell.channel);
      }
    }

    colsInSensorRowTouched &= ~(1 << touchedCol);
  }

  // trigger a new note for the top-most touch of the row
  if (highestNoteCol > 0) {
    // turn off open strings that are sounding
    if (virtualCell().hasNote()) {
      numNotesTouched += 1;
      virtualCell().releaseNote();
    }

    if (isStrummingSplit(sensorSplit) || (numNotesTouched && (!newFretting || sensorCol == highestNoteCol))) {
      TouchInfo& cell = cell(highestNoteCol, sensorRow);
      byte split = getSplitOf(highestNoteCol);
      midiSendNoteOff(split, cell.note, cell.channel);
      midiSendNoteOn(split, cell.note, velocity > 0 ? velocity : cell.velocity, cell.channel);
    }
  }

  if (isStrummedSplit(sensorSplit) &&
      !newFretting &&
      hasTouchInSplitOnRow(otherSplit(sensorSplit), sensorRow) &&
      !hasTouchInSplitOnRow(sensorSplit, sensorRow) && numNotesTouched == 1) {
    handleStrummedOpenRow(sensorSplit, velocity);
  }
}

boolean isStrummedSplit(byte split) {
  return Global.splitActive && Split[otherSplit(split)].strum;
}

boolean isStrummingSplit(byte split) {
  return Global.splitActive && Split[split].strum;
}

void prepareNewNote(signed char notenum) {
  byte channel = takeChannel(sensorSplit, sensorRow);
  sensorCell->note = notenum;
  sensorCell->channel = channel;
  sensorCell->octaveOffset = Split[sensorSplit].transposeOctave;
  
  // change the focused cell
  FocusCell& focused = focus(sensorSplit, channel);
  focused.col = sensorCol;
  focused.row = sensorRow;

  // reset the pitch bend and pressure right before sending the note on
  if (!userFirmwareActive) {
    if (Split[sensorSplit].sendX && isXExpressiveCell() && !isLowRowBendActive(sensorSplit)) {
      resetLastMidiPitchBend(sensorCell->channel);
    }
    if (Split[sensorSplit].sendZ && isZExpressiveCell()) {
      preResetLastLoudness(sensorSplit, sensorCell->note, sensorCell->channel);
    }
    if (Split[sensorSplit].sendY && isYExpressiveCell()) {
      preResetLastTimbre(sensorSplit, sensorCell->note, sensorCell->channel);
    }
  }

  // register the reverse mapping
  noteTouchMapping[sensorSplit].noteOn(notenum, channel, sensorCol, sensorRow);

  // highlight the touch animation if this is activated
  if (Split[sensorSplit].colorPlayed) {
    if (Split[sensorSplit].playedTouchMode == playedCell) {
      setLed(sensorCol, sensorRow, Split[sensorSplit].colorPlayed, cellOn, LED_LAYER_PLAYED);
    }
    else if (Split[sensorSplit].playedTouchMode == playedSame) {
      highlightPossibleNoteCells(sensorSplit, sensorCell->note);
    }
    else {
      startTouchAnimation(sensorCol, sensorRow, calcTouchAnimationSpeed(Split[sensorSplit].playedTouchMode, sensorCell->velocity));
    }
  }

  // keep track of the last note number
  latestNoteNumberForAutoOctave = notenum;
}

void sendNewNote() {
  if (!isArpeggiatorEnabled(sensorSplit)) {
    // if we've switched from pitch X enabled to pitch X disabled and the last
    // pitch bend value was not neutral, reset it first to prevent skewed pitches
    if (!Split[sensorSplit].sendX && hasPreviousPitchBendValue(sensorCell->channel)) {
      preSendPitchBend(sensorSplit, 0, sensorCell->channel);
    }

    // reset pressure to 0 before sending the note, the actually pressure value will
    // be sent right after the note on
    if (Split[sensorSplit].sendZ && isZExpressiveCell()) {
      preSendLoudness(sensorSplit, 0, 0, sensorCell->note, sensorCell->channel);
    }

    // if the same channel and note is already active, send a note off first
    // so that the new touch properly triggers a new note
    if (hasActiveMidiNote(sensorSplit, sensorCell->note, sensorCell->channel)) {
      midiSendNoteOff(sensorSplit, sensorCell->note, sensorCell->channel);
    }

    // send the note on
    midiSendNoteOn(sensorSplit, sensorCell->note, sensorCell->velocity, sensorCell->channel);
  }
}

void sendReleasedNote() {
  if (!isArpeggiatorEnabled(sensorSplit) && !isSwitchLegatoPressed(sensorSplit)) {
    // iterate over all the rows
    for (byte row = 0; row < NUMROWS; ++row) {

      // exclude the current sensor for the rest of the logic, we're looking
      // for other touches
      int32_t colsInRowTouched = colsInRowsTouched[row];
      if (row == sensorRow) {
        colsInRowTouched = colsInRowTouched & ~(1 << sensorCol);
      }

      // continue while there are touched columns in the row
      while (colsInRowTouched) {
        byte touchedCol = 31 - __builtin_clz(colsInRowTouched);
        
        if (cell(touchedCol, row).touched == touchedCell &&
            cell(touchedCol, row).note == sensorCell->note &&
            cell(touchedCol, row).channel == sensorCell->channel) {
          // if there is another touch active with the same exact note and channel,
          // don't send the note off for the released note
          return;
        }

        // exclude the cell we just processed by flipping its bit
        colsInRowTouched &= ~(1 << touchedCol);
      }
    }

    // if there are no other touches down with the same note and channel, send the note off message
    midiSendNoteOffWithVelocity(sensorSplit, sensorCell->note, sensorCell->velocity, sensorCell->channel);
  }
}

void handleNewUserFirmwareTouch() {
  sensorCell->note = sensorCol;
  sensorCell->channel = sensorRow+1;
  midiSendNoteOn(LEFT, sensorCell->note, sensorCell->velocity, sensorCell->channel);
}

void handleNewControlModeTouch() {
  Serial.write((byte)1);
  Serial.write((byte)sensorCol);
  Serial.write((byte)sensorRow);
  Serial.write("\n");

  sensorCell->note = sensorCol;
  sensorCell->channel = sensorRow+1;

  setLed(sensorCol, sensorRow, Split[Global.currentPerSplit].colorPlayed, cellOn, LED_LAYER_PLAYED);
}

unsigned short handleZExpression() {
  unsigned short preferredPressure = sensorCell->pressureZ;

  // handle pressure transition between adjacent cells if they are not playing their own note
  unsigned short adjacentZ = 0;
  if (cell(sensorCol-1, sensorRow).currentRawZ && !cell(sensorCol-1, sensorRow).hasNote()) {
    adjacentZ = cell(sensorCol-1, sensorRow).currentRawZ;
  }
  else if (cell(sensorCol+1, sensorRow).currentRawZ && !cell(sensorCol+1, sensorRow).hasNote()) {
    adjacentZ = cell(sensorCol+1, sensorRow).currentRawZ;
  }
  // the adjacent Z value is added the active cell's pressure to make
  // up for the pressure differential while moving across cells
  preferredPressure = constrain(preferredPressure + adjacentZ, 0, 1016);

  // the faster we move the slower the slew rate becomes,
  // if we're holding still the pressure changes are almost instant, if we're moving faster they are averaged out
  int32_t slewRate = sensorCell->fxdRateX;

  // adapt the slew rate based on the rate of change on the pressure, the smaller the change, the higher the slew rate
  slewRate += FXD_CONST_2 - FXD_DIV(abs(FXD_FROM_INT(preferredPressure) - sensorCell->fxdPrevPressure), FXD_FROM_INT(508));

  if (slewRate > FXD_CONST_1) {
    // we also keep track of the previous pressure on the cell and average it out with
    // the current pressure to smooth over the rate of change when transiting between cells
    int32_t fxdAveragedPressure = sensorCell->fxdPrevPressure;
    fxdAveragedPressure += FXD_DIV(FXD_FROM_INT(preferredPressure), slewRate);
    fxdAveragedPressure -= FXD_DIV(sensorCell->fxdPrevPressure, slewRate);
    sensorCell->fxdPrevPressure = fxdAveragedPressure;

    // calculate the final pressure value
    preferredPressure = constrain(FXD_TO_INT(fxdAveragedPressure), 0, 1016);
  }
  else {
    sensorCell->fxdPrevPressure = FXD_FROM_INT(preferredPressure);
  }

  return preferredPressure;
}

const int32_t fxdRateXSamples = FXD_FROM_INT(5);    // the number of samples over which the average rate of change of X is calculated

short handleXExpression() {
  sensorCell->refreshX();

  short movedX;
  int calibratedX = sensorCell->calibratedX();

  // determine if a slide transfer is in progress and which column it is with
  short transferCol = 0;
  if (cell(sensorCol-1, sensorRow).touched == transferCell) {
    transferCol = sensorCol-1;
  }
  else if (cell(sensorCol+1, sensorRow).touched == transferCell) {
    transferCol = sensorCol+1;
  }

  // if there is a slide transfer column, interpolate the X position based on the relative pressure
  // of the cells, this allows for a smoother transition as a finger slides over the groove between
  // cells and distributes the pressure
  if (transferCol != 0) {
    short totalZ = cell(transferCol, sensorRow).currentRawZ + sensorCell->currentRawZ;
    int32_t fxdTransferRatio = FXD_DIV(FXD_FROM_INT(cell(transferCol, sensorRow).currentRawZ), FXD_FROM_INT(totalZ));
    int32_t fxdCellRatio = FXD_CONST_1 - fxdTransferRatio;

    int32_t fxdTransferCalibratedX = FXD_MUL(FXD_FROM_INT(cell(transferCol, sensorRow).currentCalibratedX), fxdTransferRatio);
    int32_t fxdCellCalibratedX = FXD_MUL(FXD_FROM_INT(sensorCell->calibratedX()), fxdCellRatio);
    calibratedX = FXD_TO_INT(fxdTransferCalibratedX + fxdCellCalibratedX);
  }

  // calculate the distance from the initial X position
  if (!userFirmwareActive && Split[sensorSplit].pitchCorrectQuantize) {
    movedX = calibratedX - (FXD_TO_INT(sensorCell->fxdInitialReferenceX()) + sensorCell->quantizationOffsetX);

    // if initial quantize offset and quantize hold are active, ensure that movement on that cell will never exceed half the cell width
    // as soon as a finger transits to an adjacent cell during a slide, the quantization offset will be set to 0 and
    // fingers will track in absolute positions
    // this ensures that disregarding where you place your finger with pitch quantize on, target slides will always result
    // in the same absolute X position
    if (Split[sensorSplit].pitchCorrectHold != pitchCorrectHoldOff && sensorCell->quantizationOffsetX != 0) {
      int32_t fxdQuantRefDist = FXD_FROM_INT(calibratedX - sensorCell->quantizationOffsetX) - Device.calRows[sensorCol][0].fxdReferenceX;
      if (fxdQuantRefDist > 0) {
        if (fxdQuantRefDist > FXD_CALX_HALF_UNIT) {
          movedX = FXD_TO_INT(FXD_FROM_INT(movedX) - (fxdQuantRefDist - FXD_CALX_HALF_UNIT));
        }
      }
      else {
        if (abs(fxdQuantRefDist) > FXD_CALX_HALF_UNIT) {
          movedX = FXD_TO_INT(FXD_FROM_INT(movedX) - (fxdQuantRefDist + FXD_CALX_HALF_UNIT));
        }
      }
    }
  }
  // otherwise we use the intended centerpoint based on the calibration
  else {
    movedX = calibratedX - FXD_TO_INT(sensorCell->fxdInitialReferenceX());
  }

  short result = INVALID_DATA;

  // calculate how much change there was since the last X update
  short deltaX = abs(movedX - sensorCell->lastMovedX);

  // determine if the last X movement was a rogue sweep
  sensorCell->rogueSweepX = (deltaX >= ROGUE_SWEEP_X_THRESHOLD);
        
  if ((countTouchesInColumn() < 2 ||
       sensorCell->currentRawZ > (Device.sensorLoZ + SENSOR_PITCH_Z)) &&  // when there are multiple touches in the same column, reduce the pitch bend Z sensitivity to prevent unwanted pitch slides
      sensorCell->hasUsableX()) {                                         // if no phantom presses are active, send the pitch bend change, otherwise only send those changes that are small and gradual to prevent rogue pitch sweeps

    // calculate the average rate of X value changes over a number of samples
    sensorCell->fxdRateX -= FXD_DIV(sensorCell->fxdRateX, fxdRateXSamples);
    sensorCell->fxdRateX += FXD_DIV(FXD_FROM_INT(deltaX), fxdRateXSamples);

    // remember the last X movement
    sensorCell->lastMovedX = movedX;

    // if pitch quantize on hold is disabled, just output the current touch pitch
    if (!doQuantizeHold()) {
      result = movedX;
    }
    // if pitch quantize is active on hold, interpolate between the ideal pitch and the current touch pitch
    else {
      int32_t fxdMovedRatio = FXD_DIV(fxdPitchHoldSamples[sensorSplit] - sensorCell->fxdRateCountX, fxdPitchHoldSamples[sensorSplit]);
      if (fxdMovedRatio > FXD_CONST_1) fxdMovedRatio = FXD_CONST_1;
      else if (fxdMovedRatio < 0)      fxdMovedRatio = 0;
      int32_t fxdCorrectedRatio = FXD_CONST_1 - fxdMovedRatio;
      int32_t fxdQuantizedDistance = Device.calRows[sensorCol][0].fxdReferenceX - sensorCell->fxdInitialReferenceX();
      
      int32_t fxdInterpolatedX = FXD_MUL(FXD_FROM_INT(movedX), fxdMovedRatio) + FXD_MUL(fxdQuantizedDistance, fxdCorrectedRatio);

      result = FXD_TO_INT(fxdInterpolatedX);
    }

    // keep track of how many times the X changement rate drops below the threshold or above
    if (fxdRateXThreshold[sensorSplit] >= sensorCell->fxdRateX) {
      if (sensorCell->fxdRateCountX < fxdPitchHoldSamples[sensorSplit]) {
        sensorCell->fxdRateCountX += FXD_CONST_1;

        // if the pich has just stabilized, adapt the touch's initial X position so that pitch changes start from the stabilized pitch
        if (isQuantizeHoldStable()) {
          // ensure that the rate count can never exceed the pitch hold duration
          sensorCell->fxdRateCountX = fxdPitchHoldSamples[sensorSplit];

          if (Split[sensorSplit].pitchCorrectQuantize && Split[sensorSplit].pitchCorrectHold != pitchCorrectHoldOff) {
            sensorCell->quantizationOffsetX = calibratedX - FXD_TO_INT(Device.calRows[sensorCol][0].fxdReferenceX);
          }
        }
      }
    }
    else if (sensorCell->fxdRateCountX > 0) {
      if (sensorCell->fxdRateCountX > 0) {
        sensorCell->fxdRateCountX -= FXD_CONST_1;
      }
    }
  }

  return result;
}

boolean doQuantizeHold() {
  return !userFirmwareActive && Split[sensorSplit].pitchCorrectHold != pitchCorrectHoldOff;
}

boolean isQuantizeHoldStable() {
  return sensorCell->fxdRateCountX >= fxdPitchHoldSamples[sensorSplit];
}

short handleYExpression() {
  sensorCell->refreshY();

  short preferredTimbre = INVALID_DATA;
  if (Split[sensorSplit].relativeY) {
    preferredTimbre = constrain(Split[sensorSplit].initialRelativeY + (sensorCell->currentCalibratedY - sensorCell->initialY), 0, 127);
  }
  else {
    preferredTimbre = sensorCell->currentCalibratedY;
  }

  // the faster we move horizontally, the slower the slew rate becomes,
  // if we're holding still the timbre changes are almost instant, if we're moving faster they are averaged out
  int32_t slewRate = FXD_CONST_1 + sensorCell->fxdRateX;

  int32_t fxdAveragedTimbre;
  if (sensorCell->fxdPrevTimbre == FXD_CONST_255) {
    fxdAveragedTimbre = FXD_FROM_INT(preferredTimbre);
  }
  else {
    fxdAveragedTimbre = sensorCell->fxdPrevTimbre;
    fxdAveragedTimbre += FXD_DIV(FXD_FROM_INT(preferredTimbre), slewRate);
    fxdAveragedTimbre -= FXD_DIV(sensorCell->fxdPrevTimbre, slewRate);
  }
  sensorCell->fxdPrevTimbre = fxdAveragedTimbre;

  return FXD_TO_INT(fxdAveragedTimbre);
}

void releaseChannel(byte split, byte channel) {
  if (Split[split].midiMode == channelPerNote) {
    splitChannels[split].release(channel);
  }
}

boolean handleNonPlayingRelease() {
  if (sensorCell->velocity) {
    switch (displayMode) {
      case displayPerSplit:
        handlePerSplitSettingRelease();
        break;
      case displayPreset:
        handlePresetRelease();
        break;
      case displayBendRange:
        handleBendRangeRelease();
        break;
      case displayLimitsForY:
        handleLimitsForYRelease();
        break;
      case displayCCForY:
        handleCCForYRelease();
        break;
      case displayInitialForRelativeY:
        handleInitialForRelativeYRelease();
        break;
      case displayLimitsForZ:
        handleLimitsForZRelease();
        break;
      case displayCCForZ:
        handleCCForZRelease();
        break;
      case displayPlayedTouchModeConfig:
        handlePlayedTouchModeRelease();
        break;
      case displayCCForFader:
        handleCCForFaderRelease();
        break;
      case displayLowRowCCXConfig:
        handleLowRowCCXConfigRelease();
        break;
      case displayLowRowCCXYZConfig:
        handleLowRowCCXYZConfigRelease();
        break;
      case displayCCForSwitchCC65:
        handleCCForSwitchCC65ConfigRelease();
        break;
      case displayCCForSwitchSustain:
        handleCCForSwitchSustainConfigRelease();
        break;
      case displayCustomSwitchAssignment:
        handleCustomSwitchAssignmentConfigRelease();
        break;
      case displayLimitsForVelocity:
        handleLimitsForVelocityRelease();
        break;
      case displaySleepConfig:
        handleSleepConfigRelease();
        break;
      case displaySplitHandedness:
        handleSplitHandednessRelease();
        break;
      case displayRowOffset:
        handleRowOffsetRelease();
        break;
      case displayGuitarTuning:
        handleGuitarTuningRelease();
        break;
      case displayMinUSBMIDIInterval:
        handleMinUSBMIDIIntervalRelease();
        break;
      case displayMIDIThrough:
        handleMIDIThroughRelease();
        break;
      case displayValueForFixedVelocity:
        handleValueForFixedVelocityRelease();
        break;
      case displaySensorSensitivityZ:
        handleSensorSensitivityZRelease();
        break;
      case displaySensorLoZ:
        handleSensorLoZRelease();
        break;
      case displaySensorFeatherZ:
        handleSensorFeatherZRelease();
        break;
      case displaySensorRangeZ:
        handleSensorRangeZRelease();
        break;
      case displayVolume:
        handleVolumeRelease();
        break;
      case displayOctaveTranspose:
        handleOctaveTransposeRelease();
        break;
      case displayGlobal:
      case displayGlobalWithTempo:
        handleGlobalSettingRelease();
        break;
      case displayEditAudienceMessage:
        handleEditAudienceMessageRelease();
        break;
      case displaySequencerProjects:
        handleSequencerProjectsRelease();
        break;
      case displaySequencerDrum0107:
        handleSequencerDrum0107Release();
        break;
      case displaySequencerDrum0814:
        handleSequencerDrum0814Release();
        break;
      case displaySequencerColors:
        handleSequencerColorsRelease();
        break;
      case displayCustomLedsEditor:
        handleCustomLedsEditorRelease();
        break;
      default:
        return false;
    }

    return true;
  }

  return false;
}

#define PENDING_RELEASE_CONTROL    1
#define PENDING_RELEASE_MOVEMENT   3
#define PENDING_RELEASE_EDITING    15

// Called when a touch is released to handle note off or other release events
void handleTouchRelease() {
  DEBUGPRINT((1,"handleTouchRelease"));
  DEBUGPRINT((1," col="));DEBUGPRINT((1,(int)sensorCol));
  DEBUGPRINT((1," row="));DEBUGPRINT((1,(int)sensorRow));
  DEBUGPRINT((1,"\n"));

  // if a release is pending, decrease the counter
  if (sensorCell->pendingReleaseCount > 0) {
    sensorCell->pendingReleaseCount--;
  }
  // if no release is pending, start a pending release
  else if (sensorCol == 0) {
    sensorCell->pendingReleaseCount = PENDING_RELEASE_CONTROL;
  }
  else if (isSequencerEditing()) {
    sensorCell->pendingReleaseCount = PENDING_RELEASE_EDITING;
  }
  else if (sensorCell->fxdRateX > PENDING_RELEASE_RATE_X) {
    sensorCell->pendingReleaseCount = PENDING_RELEASE_MOVEMENT;
  }

  // if a release is pending, don't perform the release logic yet
  if (sensorCell->pendingReleaseCount > 0) {
    return;
  }

  // remember whether this cell was ignored
  boolean wasIgnored = (sensorCell->touched == ignoredCell);

  // mark this cell as no longer touched
  cellTouched(untouchedCell);

  if (wasIgnored ||
      displayMode == displaySleep) {
    postTouchRelease();
    return;
  }

  // release open strings if no touches are down anymore
  handleOpenStringsRelease();

  // if touch release is in column 0, it's a command key release so handle it
  if (sensorCol == 0 &&
      // user firmware mode only handles the global settings command button
      (!userFirmwareActive || sensorRow == GLOBAL_SETTINGS_ROW)) {
    handleControlButtonRelease();
    postTouchRelease();
    return;
  }

  // Some of the displayModes handle Release events
  if (handleNonPlayingRelease()) {
    performContinuousTasks();
  }
  // check if calibration is active and its cell release logic needs to be executed
  else if (handleCalibrationRelease()) {
    // do nothing, calibration is handled elsewhere
  }
  // user firmware mode has its own mode of operation
  else if (userFirmwareActive) {
    midiSendNoteOffWithVelocity(LEFT, sensorCell->note, sensorCell->velocity, sensorCell->channel);
    sensorCell->clearMusicalData();
  }
  // control mode has its own mode of operation
  else if (controlModeActive) {
    if (sensorCell->hasNote()) {
      Serial.write((byte)0);
      Serial.write((byte)sensorCol);
      Serial.write((byte)sensorRow);
      Serial.write("\n");

      setLed(sensorCol, sensorRow, COLOR_OFF, cellOff, LED_LAYER_PLAYED);
    }
  }
  // CC faders have their own operation mode
  else if (Split[sensorSplit].ccFaders) {
    handleFaderRelease();
  }
  // sequencer has its own operation mode
  else if (Split[Global.currentPerSplit].sequencer) {
    handleSequencerRelease();
  }
  // is this cell used for low row functionality
  else if (isLowRow()) {
    lowRowStop();
  }
  else if (sensorCell->hasNote()) {

    // reset the pressure when the note is released and that setting is active
    if (Split[sensorSplit].sendZ && isZExpressiveCell()) {
      preSendLoudness(sensorSplit, 0, 0, sensorCell->note, sensorCell->channel);
    }

    // unregister the note <> cell mapping
    if (!isSwitchLegatoPressed(sensorSplit) && (!isArpeggiatorEnabled(sensorSplit) || !isSwitchLatchPressed(sensorSplit))) {
      noteTouchMapping[sensorSplit].noteOff(sensorCell->note, sensorCell->channel);
    }

    // send the Note Off
    if (isArpeggiatorEnabled(sensorSplit)) {
      if (!isSwitchLatchPressed(sensorSplit)) {
        handleArpeggiatorNoteOff(sensorSplit, sensorCell->note, sensorCell->channel);
      }
    }
    else {
      if (isStrummedSplit(sensorSplit)) {
        handleStrummedRowChange(false, sensorCell->velocity);
      }
      sendReleasedNote();
    }

    // unhighlight the touch animation if this is activated
    if (Split[sensorSplit].colorPlayed) {
      if (Split[sensorSplit].playedTouchMode == playedCell) {
        setLed(sensorCol, sensorRow, COLOR_OFF, cellOff, LED_LAYER_PLAYED);
      }
      // if no notes are active anymore, reset the highlighted cells
      else if (Split[sensorSplit].playedTouchMode == playedSame) {
        // calculate the difference between the octave offset when the note was turned on and the octave offset
        // that is currently in use on the split, since the octave can change on the fly, while playing,
        // hence changing the position of notes on the surface
        short octaveOffsetDifference = Split[sensorSplit].transposeOctave - sensorCell->octaveOffset;
        short realSensorNote = sensorCell->note + octaveOffsetDifference;

        // ensure that no other notes of the same value are still active
        boolean allNotesOff = true;

        // iterate over all the rows
        for (byte row = 0; row < NUMROWS && allNotesOff; ++row) {

          // continue while there are touched columns in the row
          int32_t colsInRowTouched = colsInRowsTouched[row];
          while (colsInRowTouched) {
            byte touchedCol = 31 - __builtin_clz(colsInRowTouched);
            
            // if another touch in the same split has the same note, the lights should remain lit
            if (!(sensorCol == touchedCol && sensorRow == row) &&
                sensorSplit == getSplitOf(touchedCol) &&
                cell(touchedCol, row).touched == touchedCell &&
                cell(touchedCol, row).note + Split[sensorSplit].transposeOctave - cell(touchedCol, row).octaveOffset == realSensorNote) {
              allNotesOff = false;
              break;
            }

            // exclude the cell we just processed by flipping its bit
            colsInRowTouched &= ~(1 << touchedCol);
          }
        }

        if (allNotesOff) {
          resetPossibleNoteCells(sensorSplit, realSensorNote);
        }
      }
    }

    // reset the pitch bend when the note is released and that setting is active
    if (Split[sensorSplit].pitchResetOnRelease && isXExpressiveCell() && !isLowRowBendActive(sensorSplit)) {
      preSendPitchBend(sensorSplit, 0, sensorCell->channel);
    }

    // If the released cell had focus, reassign focus to the latest touched cell
    if (isFocusedCell()) {
      setFocusCellToLatest(sensorSplit, sensorCell->channel);
    }

    releaseChannel(sensorSplit, sensorCell->channel);

    // Reset all this cell's musical data
    sensorCell->clearMusicalData();
  }

  postTouchRelease();
}

void postTouchRelease() {
  sensorCell->clearAllPhantoms();

  // reset velocity calculations
  sensorCell->vcount = 0;

  sensorCell->clearSensorData();

#ifdef TESTING_SENSOR_DISABLE
    sensorCell->disabled = true;
#endif  
}

void handleOpenStringsRelease() {
  if (cellsTouched == 0) {
    // turn off all the notes of sounding open strings since no touches are active at all anymore
    for (byte row = 0; row < NUMROWS; ++row) {
      virtualTouchInfo[row].releaseNote();
    }
  }
}

// nextSensorCell:
// Moves on to the next cell witin the total surface scan of all surface cells.

#define MAX_CELLCOUNT 201
byte CELLCOUNT = MAX_CELLCOUNT;
byte SCANNED_CELLS[MAX_CELLCOUNT][2];

// Columns and rows are scanned in non-sequential order to minimize sensor crosstalk
const byte SCANNED_CELLS_200[MAX_CELLCOUNT][2] = {
  {0, 0},
  {3, 4}, {7, 1}, {10, 5}, {13, 2}, {17, 6}, {20, 3}, {24, 7}, {1, 4}, {4, 0}, {8, 5}, {11, 1}, {14, 6}, {18, 2}, {21, 7}, {25, 3}, {2, 0}, {5, 4}, {9, 1}, {12, 5}, {15, 2}, {19, 6}, {22, 3}, {6, 7}, {16, 4}, {23, 0},
  {3, 0}, {7, 5}, {10, 1}, {13, 6}, {17, 2}, {20, 7}, {24, 3}, {1, 0}, {4, 4}, {8, 1}, {11, 5}, {14, 2}, {18, 6}, {21, 3}, {25, 7}, {2, 4}, {5, 0}, {9, 5}, {12, 1}, {15, 6}, {19, 2}, {22, 7}, {6, 3}, {16, 0}, {23, 4},
  {3, 5}, {7, 2}, {10, 6}, {13, 3}, {17, 7}, {20, 4}, {24, 0}, {1, 5}, {4, 1}, {8, 6}, {11, 2}, {14, 7}, {18, 3}, {21, 0}, {25, 4}, {2, 1}, {5, 5}, {9, 2}, {12, 6}, {15, 3}, {19, 7}, {22, 4}, {6, 0}, {16, 5}, {23, 1},
  {3, 1}, {7, 6}, {10, 2}, {13, 7}, {17, 3}, {20, 0}, {24, 4}, {1, 1}, {4, 5}, {8, 2}, {11, 6}, {14, 3}, {18, 7}, {21, 4}, {25, 0}, {2, 5}, {5, 1}, {9, 6}, {12, 2}, {15, 7}, {19, 3}, {22, 0}, {6, 4}, {16, 1}, {23, 5},
  {3, 6}, {7, 3}, {10, 7}, {13, 4}, {17, 0}, {20, 5}, {24, 1}, {1, 6}, {4, 2}, {8, 7}, {11, 3}, {14, 0}, {18, 4}, {21, 1}, {25, 5}, {2, 2}, {5, 6}, {9, 3}, {12, 7}, {15, 4}, {19, 0}, {22, 5}, {6, 1}, {16, 6}, {23, 2},
  {3, 2}, {7, 7}, {10, 3}, {13, 0}, {17, 4}, {20, 1}, {24, 5}, {1, 2}, {4, 6}, {8, 3}, {11, 7}, {14, 4}, {18, 0}, {21, 5}, {25, 1}, {2, 6}, {5, 2}, {9, 7}, {12, 3}, {15, 0}, {19, 4}, {22, 1}, {6, 5}, {16, 2}, {23, 6},
  {3, 7}, {7, 4}, {10, 0}, {13, 5}, {17, 1}, {20, 6}, {24, 2}, {1, 7}, {4, 3}, {8, 0}, {11, 4}, {14, 1}, {18, 5}, {21, 2}, {25, 6}, {2, 3}, {5, 7}, {9, 4}, {12, 0}, {15, 5}, {19, 1}, {22, 6}, {6, 2}, {16, 7}, {23, 3},
  {3, 3}, {7, 0}, {10, 4}, {13, 1}, {17, 5}, {20, 2}, {24, 6}, {1, 3}, {4, 7}, {8, 4}, {11, 0}, {14, 5}, {18, 1}, {21, 6}, {25, 2}, {2, 7}, {5, 3}, {9, 0}, {12, 4}, {15, 1}, {19, 5}, {22, 2}, {6, 6}, {16, 3}, {23, 7}
};
const byte SCANNED_CELLS_128[MAX_CELLCOUNT][2] = {
  {0, 0},
  {3, 4}, {7, 1}, {10, 5}, {13, 2}, {1, 4}, {4, 0}, {8, 5}, {11, 1}, {14, 6}, {2, 0}, {5, 4}, {9, 1}, {12, 5}, {15, 2}, {6, 7}, {16, 4},
  {3, 0}, {7, 5}, {10, 1}, {13, 6}, {1, 0}, {4, 4}, {8, 1}, {11, 5}, {14, 2}, {2, 4}, {5, 0}, {9, 5}, {12, 1}, {15, 6}, {6, 3}, {16, 0},
  {3, 5}, {7, 2}, {10, 6}, {13, 3}, {1, 5}, {4, 1}, {8, 6}, {11, 2}, {14, 7}, {2, 1}, {5, 5}, {9, 2}, {12, 6}, {15, 3}, {6, 0}, {16, 5},
  {3, 1}, {7, 6}, {10, 2}, {13, 7}, {1, 1}, {4, 5}, {8, 2}, {11, 6}, {14, 3}, {2, 5}, {5, 1}, {9, 6}, {12, 2}, {15, 7}, {6, 4}, {16, 1},
  {3, 6}, {7, 3}, {10, 7}, {13, 4}, {1, 6}, {4, 2}, {8, 7}, {11, 3}, {14, 0}, {2, 2}, {5, 6}, {9, 3}, {12, 7}, {15, 4}, {6, 1}, {16, 6},
  {3, 2}, {7, 7}, {10, 3}, {13, 0}, {1, 2}, {4, 6}, {8, 3}, {11, 7}, {14, 4}, {2, 6}, {5, 2}, {9, 7}, {12, 3}, {15, 0}, {6, 5}, {16, 2},
  {3, 7}, {7, 4}, {10, 0}, {13, 5}, {1, 7}, {4, 3}, {8, 0}, {11, 4}, {14, 1}, {2, 3}, {5, 7}, {9, 4}, {12, 0}, {15, 5}, {6, 2}, {16, 7},
  {3, 3}, {7, 0}, {10, 4}, {13, 1}, {1, 3}, {4, 7}, {8, 4}, {11, 0}, {14, 5}, {2, 7}, {5, 3}, {9, 0}, {12, 4}, {15, 1}, {6, 6}, {16, 3},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

void initializeTouchHandling() {
  if (LINNMODEL == 200) {
    CELLCOUNT = 201;
    for (byte i = 0; i < MAX_CELLCOUNT; ++i) {
      SCANNED_CELLS[i][0] = SCANNED_CELLS_200[i][0];
      SCANNED_CELLS[i][1] = SCANNED_CELLS_200[i][1];
    }
  }
  else if (LINNMODEL == 128) {
    CELLCOUNT = 129;
    for (byte i = 0; i < MAX_CELLCOUNT; ++i) {
      SCANNED_CELLS[i][0] = SCANNED_CELLS_128[i][0];
      SCANNED_CELLS[i][1] = SCANNED_CELLS_128[i][1];
    }
  }
}

inline void nextSensorCell() {
  static byte controlRow = 0;

  sensorCol = SCANNED_CELLS[cellCount][0];
  sensorRow = SCANNED_CELLS[cellCount][1];
  if (++cellCount >= CELLCOUNT) {
    cellCount = 0;
  }

  // we're only scanning one of the eight control switches on each surface scan,
  // they don't need as many updates as the playing keys
  if (sensorCol == 0) {
    sensorRow = controlRow;
    if (++controlRow >= 8) {
      controlRow = 0;
    }
  }

  updateSensorCell();
}

inline void updateSensorCell() {
  sensorSplit = getSplitOf(sensorCol);
  sensorCell = &cell(sensorCol, sensorRow);
  // we're keeping track of the state of X, Y and Z so that we don't refresh it needlessly for finger tracking
  sensorCell->shouldRefreshData();
}


// getNoteNumber:
// computes MIDI note number from current row, column, row offset, octave button and transposition amount
byte getNoteNumber(byte split, byte col, byte row) {
  byte notenum = 0;

  // return the computed note based on the selected rowOffset
  short noteCol = col;
  if (isLeftHandedSplit(split)) {
    noteCol = (NUMCOLS - col);
  }

  notenum = determineRowOffsetNote(split, row) + noteCol - 1;

  return notenum - Split[split].transposeLights;
}

short determineRowOffsetNote(byte split, byte row) {
  short lowest = 30;                                  // 30 = F#2, which is 10 semitones below guitar low E (E3/52). High E = E5/76

  if (Global.rowOffset <= 12) {                       // if rowOffset is set to between 0 and 12..
    short offset = Global.rowOffset;

    if (Global.rowOffset == ROWOFFSET_OCTAVECUSTOM) {
      offset = Global.customRowOffset;
    }

    if (offset < 0) {
      lowest = 65;
    }

    if (Global.rowOffset == ROWOFFSET_NOOVERLAP) {    // no overlap mode
      byte lowCol, highCol;
      getSplitBoundaries(split, lowCol, highCol);

      offset = highCol - lowCol;                      // calculate the row offset based on the width of the split the column belongs to
      if (Global.splitActive && split == RIGHT) {     // if the right split is displayed, change the column so that it the lower left starting
        getSplitBoundaries(LEFT, lowCol, highCol);    // point starts at the same point as the left split, behaving as if there were two independent
        lowest = lowest - (highCol - lowCol);         // LinnStruments next to each-other
      }
    }
    else if (offset == -17) {                         // if custom row offset is set to inverted guitar tuning...
      offset = -5;                                    // standard guitar offset is 5 semitones

      if (row <= 1) {                                 // except from row 1 downwards where it's shifted by one
        lowest -= 1;
      }
    }
    else if (offset >= 12) {                          // start the octave offset one octave lower to prevent having disabled notes at the top in the default configuration
      lowest = 18;
    }
    else if (offset <= -12) {
      lowest = 18 - 7 * offset;
    }

    return lowest + (row * offset);
  }
  else if (Global.rowOffset == ROWOFFSET_GUITAR) {    // if rowOffset is set to guitar tuning...
    return Global.guitarTuning[row];
  }
  else {                                              // Global.rowOffset == ROWOFFSET_ZERO, rowOffset is set to zero...
    return lowest;
  }
}

void getSplitBoundaries(byte sp, byte& lowCol, byte& highCol) {
  // Set ranges of columns to be scanned (all of one split only)
  if (Global.splitActive) {                    // if Split mode is on
    if (sp == LEFT) {                   // and it's the left split
      lowCol = 1;                       // set column range to left split
      highCol = Global.splitPoint;
    }
    else {                              // if it's the right split
      lowCol = Global.splitPoint;       // set column range to right split
      highCol = NUMCOLS;
    }
  }
  else {                                // if Split mode is off
    lowCol = 1;                         // set column range to all columns
    highCol = NUMCOLS;
  }
}

void setFocusCellToLatest(byte sp, byte channel) {
  byte lowCol;                          // lowest column to be scanned
  byte highCol;                         // highest column to be scanned
  getSplitBoundaries(sp, lowCol, highCol);

  unsigned long latestTouch = 0;                 // temporarily holds the latest touch of all touched cells

  for (byte col = lowCol; col < highCol; ++col) {            // count through the columns
    for (byte row = 0; row < NUMROWS; ++row) {               // count through the rows within each column
      if (cell(col, row).touched == touchedCell &&           // if the addressed cell is touched...
          cell(col, row).channel == channel) {               // and uses the appropriate channel
        unsigned long last = cell(col, row).lastTouch;
        if (last > latestTouch) {                            // if this cell is touched later than any found so far...
          latestTouch = last;                                // then save it...
          focus(sp, channel).col = col;                      // and reassign focus to this cell
          focus(sp, channel).row = row;
        }
      }
    }
  }

  // at this point, all touched cells have been scanned and focus has been reassigned to the latest touched cell
  // if this was the last note to be released, reset the focused cell data
  if (latestTouch == 0) {
    focus(sp, channel).col = 0;   // column 0 are the control keys, so this combination can never be true for playing keys
    focus(sp, channel).row = 0;
  }
}

inline byte otherSplit() {
  return RIGHT-sensorSplit;
}

inline byte otherSplit(byte split) {
  return RIGHT-split;
}

inline byte splitLowestEdge(byte split) {
  if (isLeftHandedSplit(split)) {
    if (split == RIGHT) {
      return NUMCOLS - 1;
    }
    return Global.splitPoint - 1;
  }
  else {
    if (split == LEFT) {
      return 1;
    }
    return Global.splitPoint;
  }
}

inline boolean isLeftHandedSplit(byte split) {
  return !userFirmwareActive &&
    Device.otherHanded &&
    (Device.splitHandedness == reversedBoth ||
      (split == LEFT && Device.splitHandedness == reversedLeft) ||
      (split == RIGHT && Device.splitHandedness == reversedRight));
}

// If split mode is on and the specified column is in the right split, returns RIGHT, otherwise LEFT.
inline byte getSplitOf(byte col) {
  if (Global.splitActive && !Split[Global.currentPerSplit].sequencer) {
    if (col < Global.splitPoint) {
      return LEFT;
    }
    else {
      return RIGHT;
    }
  }
  else {
    return Global.currentPerSplit;
  }
}
