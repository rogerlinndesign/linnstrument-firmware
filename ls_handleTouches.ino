/*********************** ls_handleTouches: LinnStrument Handle Touch Events ***********************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
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
    rowsInColsTouched[col] |= (int32_t)(1 << row);
    colsInRowsTouched[row] |= (int32_t)(1 << col);
  }
  // if the state is untouched, turn off the appropriate bit in the
  // bitmasks that track the touched cells
  else {
    rowsInColsTouched[col] &= ~(int32_t)(1 << row);
    colsInRowsTouched[row] &= ~(int32_t)(1 << col);
  }
  
  // save the touched state for each cell
  cell(col, row).touched = state;
}

// Re-initialize the velocity detection
void initVelocity() {
  sensorCell().velSumY = 0;
  sensorCell().velSumXY = 0;

  sensorCell().vcount = 0;
  sensorCell().velocity = 0;
}

byte calcPreferredVelocity(byte velocity) {
  // determine the preferred velocity based on the sensitivity settings
  if (Global.velocitySensitivity == velocityFixed) {
    return 96;
  }
  else {
    return constrain(velocity, 1, 127);
  }
}

#define TRANSFER_SLIDE_PROXIMITY 100

boolean severalTouchesForMidiChannel(byte split, byte col, byte row) {
  if (!cell(col, row).hasNote()) {
    return false;
  }

  if (noteTouchMapping[split].getMusicalTouchCount(cell(col, row).channel) > 1) {
    return true;
  }

  return false;
}

const int32_t PENDING_RELEASE_RATE_X = FXD_FROM_INT(7);

boolean potentialSlideTransferCandidate(byte col) {
  if (col < 1) return false;
  if (userFirmwareActive) {
    if (!userFirmwareSlideMode[sensorRow]) return false;
  }
  else {
    if (sensorSplit != getSplitOf(col)) return false;
    if (!isLowRow() &&                                                   // don't perform slide transfers
        (!Split[sensorSplit].sendX ||                                    // if pitch slides are disabled
         !isFocusedCell(col, sensorRow) ||                               // if this is not a focused cell
         severalTouchesForMidiChannel(sensorSplit, col, sensorRow))) {   // when there are several touches for the same MIDI channel
      return false;
    }
    if (isLowRow() && !lowRowRequiresSlideTracking()) return false;
    if (isStrummingSplit(sensorSplit)) return false;
  }

  if (cell(col, sensorRow).pendingReleaseCount &&                        // if there's a pending release but not enough X change
      cell(col, sensorRow).fxdRateX <= PENDING_RELEASE_RATE_X) {
    return false;
  }

  return cell(col, sensorRow).touched != untouchedCell &&                                                     // the sibling cell has an active touch
    (cell(col, sensorRow).pendingReleaseCount ||                                                              // either a release is pending to be performed, or
     abs(sensorCell().calibratedX() - cell(col, sensorRow).currentCalibratedX) < TRANSFER_SLIDE_PROXIMITY);   // both cells are touched simultaneously on the edges
}

boolean isReadyForSlideTransfer(byte col) {
  return cell(col, sensorRow).pendingReleaseCount ||                 // there's a pending release waiting
    sensorCell().currentRawZ > cell(col, sensorRow).currentRawZ;     // the cell pressure is higher
}

boolean hasImpossibleX() {             // checks whether the calibrated X is outside of the possible bounds for the current cell
  return Device.calibrated &&
    (sensorCell().calibratedX() < FXD_TO_INT(Device.calRows[sensorCol][0].fxdReferenceX - CALX_FULL_UNIT) ||
     sensorCell().calibratedX() > FXD_TO_INT(Device.calRows[sensorCol][0].fxdReferenceX + CALX_FULL_UNIT));
}

void transferFromSameRowCell(byte col) {
  sensorCell().initialX = cell(col, sensorRow).initialX;
  sensorCell().initialReferenceX = cell(col, sensorRow).initialReferenceX;  
  sensorCell().quantizationOffsetX = cell(col, sensorRow).quantizationOffsetX;  
  sensorCell().lastMovedX = cell(col, sensorRow).lastMovedX;
  sensorCell().fxdLastMovedX = cell(col, sensorRow).fxdLastMovedX;
  sensorCell().fxdRateX = cell(col, sensorRow).fxdRateX;
  sensorCell().fxdRateCountX = cell(col, sensorRow).fxdRateCountX;  
  sensorCell().initialY = cell(col, sensorRow).initialY;
  sensorCell().note = cell(col, sensorRow).note;
  sensorCell().channel = cell(col, sensorRow).channel;
  sensorCell().octaveOffset = cell(col, sensorRow).octaveOffset;
  sensorCell().fxdPrevPressure = cell(col, sensorRow).fxdPrevPressure;
  sensorCell().fxdPrevTimbre = cell(col, sensorRow).fxdPrevTimbre;
  sensorCell().velocity = cell(col, sensorRow).velocity;
  sensorCell().vcount = cell(col, sensorRow).vcount;
  noteTouchMapping[sensorSplit].changeCell(sensorCell().note, sensorCell().channel, sensorCol, sensorRow);

  cell(col, sensorRow).initialX = -1;
  cell(col, sensorRow).initialReferenceX = 0;
  cell(col, sensorRow).quantizationOffsetX = 0;
  cell(col, sensorRow).lastMovedX = 0;
  cell(col, sensorRow).fxdLastMovedX = 0;
  cell(col, sensorRow).fxdRateX = 0;
  cell(col, sensorRow).fxdRateCountX = 0;
  cell(col, sensorRow).initialY = -1;
  cell(col, sensorRow).pendingReleaseCount = 0;

  cell(col, sensorRow).note = -1;
  cell(col, sensorRow).channel = -1;
  cell(col, sensorRow).octaveOffset = 0;
  cell(col, sensorRow).fxdPrevPressure = 0;
  cell(col, sensorRow).fxdPrevTimbre = 0;
  cell(col, sensorRow).velocity = 0;
  // do not reset vcount!

  byte channel = sensorCell().channel;
  if (channel != -1 && col == focus(sensorSplit, channel).col && sensorRow == focus(sensorSplit, channel).row) {
    focus(sensorSplit, channel).col = sensorCol;
    focus(sensorSplit, channel).row = sensorRow;
  }
}

void transferToSameRowCell(byte col) {
  cell(col, sensorRow).initialX = sensorCell().initialX;
  cell(col, sensorRow).initialReferenceX = sensorCell().initialReferenceX;
  cell(col, sensorRow).quantizationOffsetX = sensorCell().quantizationOffsetX;
  cell(col, sensorRow).lastMovedX = sensorCell().lastMovedX;
  cell(col, sensorRow).fxdLastMovedX = sensorCell().fxdLastMovedX;
  cell(col, sensorRow).fxdRateX = sensorCell().fxdRateX;
  cell(col, sensorRow).fxdRateCountX = sensorCell().fxdRateCountX;
  cell(col, sensorRow).initialY = sensorCell().initialY;
  cell(col, sensorRow).note = sensorCell().note;
  cell(col, sensorRow).channel = sensorCell().channel;
  cell(col, sensorRow).octaveOffset = sensorCell().octaveOffset;
  cell(col, sensorRow).fxdPrevPressure = sensorCell().fxdPrevPressure;
  cell(col, sensorRow).fxdPrevTimbre = sensorCell().fxdPrevTimbre;
  cell(col, sensorRow).velocity = sensorCell().velocity;
  cell(col, sensorRow).vcount = sensorCell().vcount;
  noteTouchMapping[sensorSplit].changeCell(cell(col, sensorRow).note, cell(col, sensorRow).channel, col, sensorRow);

  sensorCell().initialX = -1;
  sensorCell().initialReferenceX = 0;
  sensorCell().quantizationOffsetX = 0;
  sensorCell().lastMovedX = 0;
  sensorCell().fxdLastMovedX = 0;
  sensorCell().fxdRateX = 0;
  sensorCell().fxdRateCountX = 0;
  sensorCell().initialY = -1;
  sensorCell().pendingReleaseCount = 0;

  sensorCell().note = -1;
  sensorCell().channel = -1;
  sensorCell().octaveOffset = 0;
  sensorCell().fxdPrevPressure = 0;
  sensorCell().fxdPrevTimbre = 0;
  sensorCell().velocity = 0;
  // do not reset vcount!

  byte channel = cell(col, sensorRow).channel;
  if (channel != -1 && col == focus(sensorSplit, channel).col && sensorRow == focus(sensorSplit, channel).row) {
    focus(sensorSplit, channel).col = sensorCol;
    focus(sensorSplit, channel).row = sensorRow;
  }
}

boolean isPhantomTouch() {
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
          if (hasImpossibleX() ||
              (cell(touchedCol, touchedRow).isHigherPhantomPressure(sensorCell().currentRawZ) &&
               cell(sensorCol, touchedRow).isHigherPhantomPressure(sensorCell().currentRawZ) &&
               cell(touchedCol, sensorRow).isHigherPhantomPressure(sensorCell().currentRawZ))) {

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

  // this might be a lone touch outside of a square formation, we can detect this if calibration reference points are present
  if (hasImpossibleX()) {
    sensorCell().setPhantoms(sensorCol, sensorCol, sensorRow, sensorRow);
    return true;
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

void handleSlideTransferCandidate(byte siblingCol) {
  // if the pressure gets higher than adjacent cell, the slide is transitioning over
  if (isReadyForSlideTransfer(siblingCol)) {
    transferFromSameRowCell(siblingCol);

    if (userFirmwareActive) {
      // if user firmware is active, we implement a particular transition scheme to allow touches to be tracked over MIDI
      sensorCell().note = sensorCol;
      midiSendControlChange(119, siblingCol, sensorCell().channel, true);
      midiSendNoteOn(LEFT, sensorCol, sensorCell().velocity, sensorCell().channel);
      midiSendNoteOffWithVelocity(LEFT, siblingCol, sensorCol, sensorCell().channel);
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

void handleNewTouch() {
  DEBUGPRINT((1,"handleNewTouch"));
  DEBUGPRINT((1," col="));DEBUGPRINT((1,(int)sensorCol));
  DEBUGPRINT((1," row="));DEBUGPRINT((1,(int)sensorRow));
  DEBUGPRINT((1," velocityZ="));DEBUGPRINT((1,(int)sensorCell().velocityZ));
  DEBUGPRINT((1," pressureZ="));DEBUGPRINT((1,(int)sensorCell().pressureZ));
  DEBUGPRINT((1,"\n"));

  cellTouched(touchedCell);                                 // mark this cell as touched

  if (animationActive) {                                    // allow any new touch to cancel scrolling
    if (sensorCol == 0 || displayMode == displayPromo) stopAnimation = true;  //-- custom animations stop only on control button - jas 2015/01/22 --
    return;
  }

  // if it's a command button, handle it
  if (sensorCol == 0 &&
      // user firmware mode only handles the global settings command button
      (!userFirmwareActive || sensorRow == GLOBAL_SETTINGS_ROW)) {

    if (sensorRow != SWITCH_1_ROW &&                        // if commands buttons are pressed that are not the two switches
        sensorRow != SWITCH_2_ROW) {                        // only activate them if there's note being played on the playing surface
      for (int r = 0; r < NUMROWS; ++r) {                   // this prevents accidental settings modifications while playing
        if ((colsInRowsTouched[r] & ~(int32_t)(1)) != 0) {
          cellTouched(ignoredCell);
          return;
        }
      }
    }
    handleControlButtonNewTouch();
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

    case displayCustom:  //-- act here as if normal for custom - jas 2015/10/21 --
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
        calcVelocity(sensorCell().velocityZ);
      }
      else {
        cellTouched(untouchedCell);
      }

      break;
    case displayPerSplit:                                          // it's a change to one of the Split settings
      handlePerSplitSettingNewTouch();
      break;
    case displayPreset:                                            // it's a preset change
      handlePresetNewTouch();
      break;
    case displayBendRange:                                         // it's a bend range change
      handleBendRangeNewTouch();
      break;
    case displayCCForY:                                            // it's a CC for Y change
      handleCCForYNewTouch();
      break;
    case displayCCForZ:                                            // it's a CC for Z change
      handleCCForZNewTouch();
      break;
    case displayCCForFader:                                        // it's a CC for fader change
      handleCCForFaderNewTouch();
      break;
    case displaySensorLoZ:                                         // it's a sensor low Z change
      handleSensorLoZNewTouch();
      break;
    case displaySensorFeatherZ:                                    // it's a sensor feather Z change
      handleSensorFeatherZNewTouch();
      break;
    case displaySensorRangeZ:                                      // it's a sensor Z range change
      handleSensorRangeZNewTouch();
      break;
    case displayOctaveTranspose:                                   // it's a transpose change
      handleOctaveTransposeNewTouch();
      break;
    case displayGlobal:                                            // it's a change to one of the global settings
    case displayGlobalWithTempo:
      handleGlobalSettingNewTouch();
      break;
    case displayCalibration:
      initVelocity();
      break;
    case displayEditAudienceMessage:
      handleEditAudienceMessageNewTouch();
      break;
    }
  }
}

// Calculate the transposed note number for the current cell by taken the transposition settings into account
short cellTransposedNote() {
  return transposedNote(sensorSplit, sensorCol, sensorRow);
}

short transposedNote(byte split, byte col, byte row) {
  return getNoteNumber(col, row) - Split[split].transposeLights + Split[split].transposePitch;
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

byte takeChannel() {
  switch (Split[sensorSplit].midiMode)
  {
    case channelPerNote:
    {
      return splitChannels[sensorSplit].take();
    }

    case channelPerRow:
    {
      byte channel = Split[sensorSplit].midiChanPerRow + sensorRow;
      if (channel > 16) {
        channel -= 16;
      }
      return channel;
    }

    case oneChannel:
    {
      return Split[sensorSplit].midiChanMain;
    }
  }
}

#define INVALID_DATA SHRT_MAX

// handleXYZupdate:
// Called when a cell is held, in order to read X, Y or Z movements and send MIDI messages as appropriate
void handleXYZupdate() {
  // if the touch is in the control buttons column, ignore it
  if (sensorCol == 0 &&
     // except for user firmware mode where only the global settings button is ignored for continuous updates
     (!userFirmwareActive || sensorRow == GLOBAL_SETTINGS_ROW)) return;

  // if this data point serves as a calibration sample, return immediately
  if (handleCalibrationSample()) return;

  // some features need hold functionality
  switch (displayMode) {
    case displayPerSplit:
      handlePerSplitSettingHold();
      return;
    case displayPreset:
      handlePresetHold();
      return;
    case displayGlobal:
    case displayGlobalWithTempo:
      handleGlobalSettingHold();
      return;
  }
  
  // only continue if the active display modes require finger tracking
  if (displayMode != displayNormal &&
      displayMode != displayVolume &&
      (displayMode != displaySplitPoint || splitButtonDown)) {
    return;
  }

  DEBUGPRINT((2,"handleXYZupdate"));
  DEBUGPRINT((2," col="));DEBUGPRINT((2,(int)sensorCol));
  DEBUGPRINT((2," row="));DEBUGPRINT((2,(int)sensorRow));
  DEBUGPRINT((2," velocityZ="));DEBUGPRINT((2,(int)sensorCell().velocityZ));
  DEBUGPRINT((2," pressureZ="));DEBUGPRINT((2,(int)sensorCell().pressureZ));
  DEBUGPRINT((2,"\n"));

  boolean newVelocity = calcVelocity(sensorCell().velocityZ);

  // check if after a new velocity calculation, this cell is not a phantom touch
  // we piggy-back off of the velocity calculation delay to ensure that we have at
  // least one full scan of the surface before making comparisons against other cells
  if (newVelocity && isPhantomTouch()) {
    cellTouched(untouchedCell);
    return;
  }

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
      isStrummingSplit(sensorSplit)) {
    handleNotes = false;
  }

  // this cell corresponds to a playing note
  if (newVelocity) {
    sensorCell().lastTouch = millis();
    sensorCell().lastMovedX = 0;
    sensorCell().fxdLastMovedX = 0;
    sensorCell().shouldRefreshX = true;
    sensorCell().initialX = -1;
    sensorCell().quantizationOffsetX = 0;

    if (userFirmwareActive) {
      handleNewUserFirmwareTouch();
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
      short notenum = cellTransposedNote();

      // if the note number is outside of MIDI range, don't start it
      if (notenum >= 0 && notenum <= 127) {
        handleNewNote(notenum);
      }
    }
  }

  // get the processed expression data
  short valueX = INVALID_DATA;
  short valueY = INVALID_DATA;
  byte valueZ = handleZExpression();

  // Only process x and y data when there's meaningful pressure on the cell
  if (sensorCell().isMeaningfulTouch()) {
    valueX = handleXExpression();
    valueY = handleYExpression();
  }

  // update the low row state unless this was a new low row touch, which is handled by lowRowStart()
  if (!newVelocity || !isLowRow()) {
    handleLowRowState(valueX, valueY, valueZ);
  }

  // the volume fader has its own operation mode
  if (displayMode == displayVolume) {
    if (sensorCell().isMeaningfulTouch()) {
      handleVolumeNewTouch(newVelocity);
    }
  }
  else if (Split[sensorSplit].ccFaders) {
    if (sensorCell().isMeaningfulTouch()) {
      handleFaderTouch(newVelocity);
    }
  }
  else if (handleNotes && sensorCell().hasNote()) {
    if (userFirmwareActive) {
      // don't send expression data for the control switches
      if (sensorCol != 0) {
        // Z-axis movements are encoded using Poly Pressure with the note as the column and the channel as the row
        midiSendPolyPressure(sensorCell().note, valueZ, sensorCell().channel);

        // X-axis movements are encoded in 14-bit with MIDI CC 0-25 / 32-57 as the column and the channel as the row
        if (valueX != INVALID_DATA) {
          short positionX = valueX + sensorCell().initialReferenceX;

          // compensate for the -85 offset at the left side since 0 is positioned at the center of the left-most cell
          positionX = positionX + 85;
          
          midiSendControlChange14Bit(sensorCol, sensorCol+32, positionX, sensorCell().channel);
        }

        // Y-axis movements are encoded using MIDI CC 64-89 as the column and the channel as the row
        if (valueY != INVALID_DATA) {
          midiSendControlChange(sensorCol+64, valueY, sensorCell().channel);
        }
      }
    }
    else {
      // after the initial velocity, new velocity values are continuously being calculated simply based
      // on the Z data so that velocity can change during the arpeggiation
      sensorCell().velocity = calcPreferredVelocity(sensorCell().velocityZ);

      // if sensing Z is enabled...
      // send different pressure update depending on midiMode
      if (Split[sensorSplit].sendZ && isZExpressiveCell()) {
        preSendLoudness(sensorSplit, valueZ, sensorCell().note, sensorCell().channel);
      }

      // if X-axis movements are enabled and it's a candidate for
      // X/Y expression based on the MIDI mode and the currently held down cells
      if (valueX != INVALID_DATA &&
          isXExpressiveCell() && Split[sensorSplit].sendX && !isLowRowBendActive(sensorSplit)) {
        int pitch = valueX;
        if (severalTouchesForMidiChannel(sensorSplit, sensorCol, sensorRow)) {
          pitch = 0;
        }
        preSendPitchBend(sensorSplit, pitch, sensorCell().channel);
      }

      // if Y-axis movements are enabled and it's a candidate for
      // X/Y expression based on the MIDI mode and the currently held down cells
      if (valueY != INVALID_DATA &&
          isYExpressiveCell() && Split[sensorSplit].sendY &&
          (!isLowRowCC1Active(sensorSplit) || Split[sensorSplit].ccForY != 1)) {
        preSendTimbre(sensorSplit, valueY, sensorCell().note, sensorCell().channel);
      }
    }
  }
}

void handleSplitStrum() {
  // we use the bitmask of the touched columns in the current row, and already turns off the column
  // of the strum touch itself
  int32_t colsInSensorRowTouched = colsInRowsTouched[sensorRow] & ~(1 << sensorCol);

  // now we check each touched column in the row of the current sensor
  // we gradually flip the touched bits to zero until they're all turned off
  // this allows us to loop only over the touched columns, and none other
  while (colsInSensorRowTouched) {
    // we use the ARM Cortex-M3 instruction that reports the leading bit zeros of any number
    // we determine that the left-most bit is that is turned on by substracting the leading zero
    // count from the bitdepth of a 32-bit int

    byte touchedCol = 31 - __builtin_clz(colsInSensorRowTouched);
    TouchInfo& cell = cell(touchedCol, sensorRow);
    if (cell.hasNote()) {
      // use the velocity of the strum touch
      cell.velocity = cell(sensorCol, sensorRow).velocity;

      // retrigger the MIDI note
      byte split = getSplitOf(touchedCol);
      midiSendNoteOff(split, cell.note, cell.channel);
      midiSendNoteOn(split, cell.note, cell.velocity, cell.channel);
    }

    colsInSensorRowTouched &= ~(1 << touchedCol);
  }
}

boolean isStrummedSplit(byte split) {
  return splitActive && Split[RIGHT-split].strum;
}

boolean isStrummingSplit(byte split) {
  return splitActive && Split[split].strum;
}

void handleNewNote(signed char notenum) {
  byte channel = takeChannel();
  sensorCell().note = notenum;
  sensorCell().channel = channel;
  sensorCell().octaveOffset = Split[sensorSplit].transposeOctave;
  
  // change the focused cell
  FocusCell& focused = focus(sensorSplit, channel);
  focused.col = sensorCol;
  focused.row = sensorRow;

  // reset the pitch bend right before sending the note on
  if (isXExpressiveCell() && !isLowRowBendActive(sensorSplit)) {
    preSendPitchBend(sensorSplit, 0, sensorCell().channel);
  }

  // register the reverse mapping
  noteTouchMapping[sensorSplit].noteOn(notenum, channel, sensorCol, sensorRow);


  // send the note on
  if (!isArpeggiatorEnabled(sensorSplit) && !isStrummedSplit(sensorSplit)) {
    midiSendNoteOn(sensorSplit, sensorCell().note, sensorCell().velocity, sensorCell().channel);

    //-- send the row and column of the cell -- experimental - jas 2015/02/19 --
    byte sendRowCol = midiSendRowCol() ;
    if (sendRowCol % 2 == 1) midiSendControlChange(21, sensorRow, channel, true); // avoid decimation - always send
    if (sendRowCol / 2 == 1) midiSendControlChange(20, sensorCol, channel, true); // avoid decimation - always send
  
  }

  // highlight same notes of this is activated
  if (Split[sensorSplit].colorNoteon) {
    highlightPossibleNoteCells(sensorSplit, sensorCell().note);
  }
}

void handleNewUserFirmwareTouch() {
  sensorCell().note = sensorCol;
  sensorCell().channel = sensorRow+1;
  midiSendNoteOn(LEFT, sensorCell().note, sensorCell().velocity, sensorCell().channel);
}

byte handleZExpression() {
  byte preferredPressure = sensorCell().pressureZ;

  // handle pressure transition between adjacent cells if they are not playing their own note
  byte adjacentZ = 0;
  if (cell(sensorCol-1, sensorRow).currentRawZ && !cell(sensorCol-1, sensorRow).hasNote()) {
    adjacentZ = cell(sensorCol-1, sensorRow).currentRawZ;
  }
  else if (cell(sensorCol+1, sensorRow).currentRawZ && !cell(sensorCol+1, sensorRow).hasNote()) {
    adjacentZ = cell(sensorCol+1, sensorRow).currentRawZ;
  }
  // the adjacent Z value is adapted so that it can be added the active cell's pressure to make
  // up for the pressure differential while moving across cells
  adjacentZ = (adjacentZ / 5) & 0x7F;
  preferredPressure = constrain(preferredPressure + adjacentZ, 0, 127);

  // the faster we move the slower the slew rate becomes,
  // if we're holding still the pressure changes are almost instant, if we're moving faster they are averaged out
  int32_t slewRate = sensorCell().fxdRateX;

  // adapt the slew rate based on the rate of change on the pressure, the smaller the change, the higher the slew rate
  slewRate += FXD_CONST_2 - FXD_DIV(abs(FXD_FROM_INT(preferredPressure) - sensorCell().fxdPrevPressure), FXD_FROM_INT(64));

  if (slewRate > FXD_CONST_1) {
    // we also keep track of the previous pressure on the cell and average it out with
    // the current pressure to smooth over the rate of change when transiting between cells
    int32_t fxdAveragedPressure = sensorCell().fxdPrevPressure;
    fxdAveragedPressure += FXD_DIV(FXD_FROM_INT(preferredPressure), slewRate);
    fxdAveragedPressure -= FXD_DIV(sensorCell().fxdPrevPressure, slewRate);
    sensorCell().fxdPrevPressure = fxdAveragedPressure;

    // calculate the final pressure value
    preferredPressure = constrain(FXD_TO_INT(fxdAveragedPressure), 0, 127);
  }
  else {
    sensorCell().fxdPrevPressure = FXD_FROM_INT(preferredPressure);
  }

  return preferredPressure;
}

const int32_t fxdRateXSamples = FXD_FROM_INT(5);    // the number of samples over which the average rate of change of X is calculated

short handleXExpression() {
  sensorCell().refreshX();

  short movedX;
  int calibratedX = sensorCell().calibratedX();

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
    short totalZ = cell(transferCol, sensorRow).currentRawZ + sensorCell().currentRawZ;
    int32_t fxdTransferRatio = FXD_DIV(FXD_FROM_INT(cell(transferCol, sensorRow).currentRawZ), FXD_FROM_INT(totalZ));
    int32_t fxdCellRatio = FXD_CONST_1 - fxdTransferRatio;

    int32_t fxdTransferCalibratedX = FXD_MUL(FXD_FROM_INT(cell(transferCol, sensorRow).currentCalibratedX), fxdTransferRatio);
    int32_t fxdCellCalibratedX = FXD_MUL(FXD_FROM_INT(sensorCell().calibratedX()), fxdCellRatio);
    calibratedX = FXD_TO_INT(fxdTransferCalibratedX + fxdCellCalibratedX);
  }

  // calculate the distance from the initial X position
  // if pitch quantize is on, the first X position becomes the center point and considered 0
  if (!userFirmwareActive && Split[sensorSplit].pitchCorrectQuantize) {
    movedX = calibratedX - sensorCell().initialX + sensorCell().quantizationOffsetX;
  }
  // otherwise we use the intended centerpoint based on the calibration
  else {
    movedX = calibratedX - sensorCell().initialReferenceX;
  }

  // calculate how much change there was since the last X update
  short deltaX = abs(movedX - sensorCell().lastMovedX);
        
  if ((countTouchesInColumn() < 2 ||
       sensorCell().currentRawZ > (Device.sensorLoZ + SENSOR_PITCH_Z)) &&  // when there are multiple touches in the same column, reduce the pitch bend Z sensitivity to prevent unwanted pitch slides
      (!sensorCell().hasPhantoms() ||                                      // if no phantom presses are active, send the pitch bend change
       deltaX < ROGUE_PITCH_SWEEP_THRESHOLD)) {                            // if there are phantom presses, only send those changes that are small and gradual to prevent rogue pitch sweeps

    // calculate the average rate of X value changes over a number of samples
    sensorCell().fxdRateX -= FXD_DIV(sensorCell().fxdRateX, fxdRateXSamples);
    sensorCell().fxdRateX += FXD_DIV(FXD_FROM_INT(deltaX), fxdRateXSamples);

    // remember the last X movement
    sensorCell().lastMovedX = movedX;
    sensorCell().fxdLastMovedX = FXD_FROM_INT(movedX);

    // if pitch quantize on hold is disabled, just output the current touch pitch
    if (userFirmwareActive || Split[sensorSplit].pitchCorrectHold == pitchCorrectHoldOff) {
      return movedX;
    }
    // if pitch quantize is active on hold, interpolate between the ideal pitch and the current touch pitch
    else {
      return handleQuantizeHoldCorrection(sensorSplit, sensorCol, sensorRow);
    }
  }

  return INVALID_DATA;
}

short handleQuantizeHoldCorrection(byte split, byte col, byte row) {
  int32_t fxdMovedRatio = FXD_DIV(fxdPitchHoldDuration[split] - cell(col, row).fxdRateCountX, fxdPitchHoldDuration[split]);
  int32_t fxdCorrectedRatio = FXD_CONST_1 - fxdMovedRatio;
  int32_t fxdQuantizedDistance = Device.calRows[col][0].fxdReferenceX - FXD_FROM_INT(cell(col, row).initialReferenceX);
  
  int32_t fxdInterpolatedX = FXD_MUL(cell(col, row).fxdLastMovedX, fxdMovedRatio) + FXD_MUL(fxdQuantizedDistance, fxdCorrectedRatio);

  // keep track of how many times the X changement rate drops below the threshold or above
  int32_t fxdRateDiff = fxdRateXThreshold[split] - cell(col, row).fxdRateX;
  if (fxdRateDiff > 0) {
    if (cell(col, row).fxdRateCountX < fxdPitchHoldDuration[split]) {
      cell(col, row).fxdRateCountX += fxdRateDiff;

      // if the pich has just stabilized, adapt the touch's initial X position so that pitch changes start from the stabilized pitch
      if (cell(col, row).fxdRateCountX >= fxdPitchHoldDuration[split]) {
        cell(col, row).quantizationOffsetX = cell(col, row).initialX - (cell(col, row).currentCalibratedX - FXD_TO_INT(fxdQuantizedDistance));
      }
    }
  }
  else if (cell(col, row).fxdRateCountX > 0) {
    cell(col, row).fxdRateCountX -= FXD_CONST_1;
  }

  return FXD_TO_INT(fxdInterpolatedX);
}

void handleQuantizeHoldForOtherCells() {
  for (byte row = 0; row < NUMROWS; ++row) {
    int32_t colsInRowTouched = colsInRowsTouched[row];
    while (colsInRowTouched) {
      byte col = 31 - __builtin_clz(colsInRowTouched);
      // turn the left-most active bit off, to continue the iteration over the touched rows
      colsInRowTouched &= ~(int32_t)(1 << col);

      if (col != sensorCol && row != sensorRow) {
        byte split = getSplitOf(col);

        if (Split[split].sendX &&
            Split[split].pitchCorrectHold != pitchCorrectHoldOff &&
            !isLowRowBendActive(split) &&
            cell(col, row).hasNote() &&
            isXExpressiveCell(col, row)) {
          short pitch = handleQuantizeHoldCorrection(split, col, row);
          if (severalTouchesForMidiChannel(split, col, row)) {
            pitch = 0;
          }
          preSendPitchBend(split, pitch, cell(col, row).channel);
        }
      }
    }
  }
}

const int32_t MAX_VALUE_Y = FXD_FROM_INT(127);
const int32_t SLEW_DIVIDER_Y = FXD_FROM_INT(26);
const int32_t BASE_SLEW_Y = FXD_FROM_INT(2);
const int32_t MIN_SLEW_Y = FXD_FROM_INT(3);

short handleYExpression() {
  sensorCell().refreshY();

  short preferredTimbre = INVALID_DATA;
  if (Split[sensorSplit].relativeY) {
    preferredTimbre = constrain(64 + (sensorCell().currentCalibratedY - sensorCell().initialY ), 0, 127);
  }
  else {
    preferredTimbre = sensorCell().currentCalibratedY;
  }

  // base slew rate for Y is 2
  int32_t slewRate = BASE_SLEW_Y;

  // the faster we move horizontally, the slower the slew rate becomes,
  // if we're holding still the timbre changes are almost instant, if we're moving faster they are averaged out
  slewRate += sensorCell().fxdRateX;

  // average the Y movements in reverse relation to the pressure, the harder you press, the less averaged they are
  slewRate += FXD_DIV(MAX_VALUE_Y - sensorCell().fxdPrevPressure, SLEW_DIVIDER_Y);

  // never use an Y slew rate below 3
  if (slewRate < MIN_SLEW_Y) {
    slewRate = MIN_SLEW_Y;
  }

  int32_t fxdAveragedTimbre = sensorCell().fxdPrevTimbre;
  fxdAveragedTimbre += FXD_DIV(FXD_FROM_INT(preferredTimbre), slewRate);
  fxdAveragedTimbre -= FXD_DIV(sensorCell().fxdPrevTimbre, slewRate);
  sensorCell().fxdPrevTimbre = fxdAveragedTimbre;

  return FXD_TO_INT(fxdAveragedTimbre);
}

void releaseChannel(byte channel) {
  if (Split[sensorSplit].midiMode == channelPerNote) {
    splitChannels[sensorSplit].release(channel);
  }
}

#define PENDING_RELEASE_MOVEMENT   3

// Called when a touch is released to handle note off or other release events
void handleTouchRelease() {
  DEBUGPRINT((1,"handleTouchRelease"));
  DEBUGPRINT((1," col="));DEBUGPRINT((1,(int)sensorCol));
  DEBUGPRINT((1," row="));DEBUGPRINT((1,(int)sensorRow));
  DEBUGPRINT((1,"\n"));

  // if a release is pending, decrease the counter
  if (sensorCell().pendingReleaseCount > 0) {
    sensorCell().pendingReleaseCount--;
  }
  // if no release is pending, start a pending release
  else  if (sensorCell().fxdRateX > PENDING_RELEASE_RATE_X) {
    sensorCell().pendingReleaseCount = PENDING_RELEASE_MOVEMENT;
  }

  // if a release is pending, don't perform the release logic yet
  if (sensorCell().pendingReleaseCount > 0) {
    return;
  }

  // mark this cell as no longer touched
  cellTouched(untouchedCell);

  // if touch release is in column 0, it's a command key release so handle it
  if (sensorCol == 0 &&
      // user firmware mode only handles the global settings command button
      (!userFirmwareActive || sensorRow == GLOBAL_SETTINGS_ROW)) {
    handleControlButtonRelease();
    return;
  }

  // Some of the displayModes handle Release events
  switch (displayMode) {
    case displayPerSplit:
      handlePerSplitSettingRelease();
      return;
    case displayPreset:
      handlePresetRelease();
      return;
    case displayBendRange:
      handleBendRangeRelease();
      return;
    case displayCCForY:
      handleCCForYRelease();
      return;
    case displayCCForZ:
      handleCCForZRelease();
      return;
    case displayCCForFader:
      handleCCForFaderRelease();
      return;
    case displaySensorLoZ:
      handleSensorLoZRelease();
      return;
    case displaySensorFeatherZ:
      handleSensorFeatherZRelease();
      return;
    case displaySensorRangeZ:
      handleSensorRangeZRelease();
      return;
    case displayVolume:
      handleVolumeRelease();
      return;
    case displayOctaveTranspose:
      handleOctaveTransposeRelease();
      return;
    case displayGlobal:
    case displayGlobalWithTempo:
      handleGlobalSettingRelease();
      return;
    //case displayCustom Animation - placeholder - jas 2015/01/16 --
    case displayEditAudienceMessage:
      handleEditAudienceMessageRelease();
      return;
  }

  // check if calibration is active and its cell release logic needs to be executed
  if (handleCalibrationRelease()) {
    // do nothing, calibration is handled elsewhere
  }
  // user firmware mode had its own mode of operation
  else if (userFirmwareActive) {
    midiSendNoteOffWithVelocity(LEFT, sensorCell().note, 0, sensorCell().channel);
    sensorCell().clearMusicalData();
  }
  // CC faders have their own operation mode
  else if (Split[sensorSplit].ccFaders) {
    handleFaderRelease();
  }
  // is this cell used for low row functionality
  else if (isLowRow()) {
    lowRowStop();
  }
  else if (sensorCell().hasNote()) {

    // unregister the note <> cell mapping
    noteTouchMapping[sensorSplit].noteOff(sensorCell().note, sensorCell().channel);

    // send the Note Off
    if (isArpeggiatorEnabled(sensorSplit)) {
      handleArpeggiatorNoteOff(sensorSplit, sensorCell().note, sensorCell().channel);
    } else {
      midiSendNoteOff(sensorSplit, sensorCell().note, sensorCell().channel);
    }

    // unhighlight the same notes if this is activated
    if (Split[sensorSplit].colorNoteon) {
      // calculate the difference between the octave offset when the note was turned on and the octave offset
      // that is currently in use on the split, since the octave can change on the fly, while playing,
      // hence changing the position of notes on the surface
      short octaveOffsetDifference = Split[sensorSplit].transposeOctave - sensorCell().octaveOffset;

      // ensure that no other notes of the same value are still active
      boolean allNotesOff = true;
      for (byte ch = 1; ch <= 16; ++ch) {
        if (noteTouchMapping[sensorSplit].hasTouch(sensorCell().note + octaveOffsetDifference, ch)) {
          allNotesOff = false;
          break;
        }
      }
      // if no notes are active anymore, reset the highlighted cells
      if (allNotesOff) {
        resetPossibleNoteCells(sensorSplit, sensorCell().note + octaveOffsetDifference);
      }
    }

    // reset the pressure when the note is release and that settings is active
    if (Split[sensorSplit].sendZ && isZExpressiveCell()) {
      preSendLoudness(sensorSplit, 0, sensorCell().note, sensorCell().channel);
    }

    // reset the pitch bend when the note is released and that setting is active
    if (Split[sensorSplit].pitchResetOnRelease && isXExpressiveCell() && !isLowRowBendActive(sensorSplit)) {
      preSendPitchBend(sensorSplit, 0, sensorCell().channel);
    }

    // If the released cell had focus, reassign focus to the latest touched cell
    if (isFocusedCell()) {
      setFocusCellToLatest(sensorSplit, sensorCell().channel);
    }

    releaseChannel(sensorCell().channel);

    // Reset all this cell's musical data
    sensorCell().clearMusicalData();
  }

  sensorCell().clearAllPhantoms();

  // reset velocity calculations
  sensorCell().velocity = 0;
  sensorCell().vcount = 0;

  sensorCell().clearSensorData();
}

// nextSensorCell:
// Moves on to the next cell witin the total surface scan of all 208 cells.

// Columns and rows are scanned in non-sequential order to minimize sensor crosstalk
byte scannedCells[201][2] = {
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

inline void nextSensorCell() {
  static byte cellCount = 0;
  static byte controlRow = 0;

  // we're keeping track of the state of X and Y so that we don't refresh it needlessly for finger tracking
  sensorCell().shouldRefreshData();

  sensorCol = scannedCells[cellCount][0];
  sensorRow = scannedCells[cellCount][1];
  sensorSplit = getSplitOf(sensorCol);

  if (++cellCount >= 201) {
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
}


// getNoteNumber:
// computes MIDI note number from current row, column, row offset, octave button and transposition amount
byte getNoteNumber(byte col, byte row) {
  byte notenum = 0;
  byte sp = getSplitOf(col);

  short offset, lowest;
  determineNoteOffsetAndLowest(sp, row, offset, lowest);

  // return the computed note based on the selected rowOffset
  notenum = lowest + (row * offset ) + (col - 1) * Global.colOffset + Split[sp].transposeOctave; //-- whole tone etc per column - jas 2014/12/11

  return notenum;
}

void determineNoteOffsetAndLowest(byte split, byte row, short& offset, short& lowest) {
  offset = Global.rowOffset;
  lowest = LOWEST_NOTE;

  if (Global.rowOffset <= 12) {                       // if rowOffset is set to between 0 and 12..
    if (offset == 0) {                                // no overlap mode
      byte lowCol, highCol;
      getSplitBoundaries(split, lowCol, highCol);

      offset = highCol - lowCol;                      // calculate the row offset based on the width of the split the column belongs to
      if (splitActive && split == RIGHT) {            // if the right split is displayed, change the column so that it the lower left starting
        getSplitBoundaries(LEFT, lowCol, highCol);    // point starts at the same point as the left split, behaving as if there were two independent
        lowest = lowest - (highCol - lowCol);         // LinnStruments next to each-other
      }
    }
    else if (offset == 12) {                          // start the octave offset one octave lower to prevent having disabled notes at the top in the default configuration
      lowest -= 12;
    }

  } else if (Global.rowOffset == 13) {                // if is rowOffset is set to guitar tuning...
    offset = 5;                                       // standard guitar offset is 5 semitones

    if (row >= 6) {                                   // except from row 6 onwards where it's shifted by one
      lowest -= 1;
    }
  }
}

void getSplitBoundaries(byte sp, byte& lowCol, byte& highCol) {
  // Set ranges of columns to be scanned (all of one split only)
  if (splitActive) {                    // if Split mode is on
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

// If split mode is on and the specified column is in the right split, returns RIGHT, otherwise LEFT.
inline byte getSplitOf(byte col) {
  if (splitActive)
  {
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
