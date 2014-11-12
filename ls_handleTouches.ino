/*********************** ls_handleTouches: LinnStrument Handle Touch Events ***********************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These routines handle the processing of new touch events, continuous updates of touch events and
released touch events
**************************************************************************************************/

void cellTouched(TouchState state);
void cellTouched(TouchState state) {
  // turn on the bit that correspond to the column and row of this cell,
  // this allows us to very quickly find other touched cells and detect
  // phantom key presses without having to evaluate every cell on the board
  if ( state != untouchedCell) {
    rowsInColsTouched[sensorCol] |= (1 << sensorRow);
    colsInRowsTouched[sensorRow] |= (1 << sensorCol);
  }
  // if the state is untouched, turn off the appropriate bit in the
  // bitmasks that track the touched cells
  else {
    rowsInColsTouched[sensorCol] &= ~(1 << sensorRow);
    colsInRowsTouched[sensorRow] &= ~(1 << sensorCol);
  }

  // save the touched state for each cell
  cell(sensorCol, sensorRow).touched = state;
}

// Velocity calculation, it merely looks for the highest pressure value in the last
// few samples (set in VELOCITY_SAMPLES define), and uses that as the velocity value

// Re-initialize the velocity detection
void initVelocity() {
  cell().vcount = 0;
  cell().velocity = 0;
}

byte calcPreferredVelocity(byte velocity) {
  // determine the preferred velocity based on the sensitivity settings
  if (Global.velocitySensitivity == velocityFixed) {
    return 96;
  }
  else {
    return constrain(unsigned(velocity) + 16 * Global.velocitySensitivity, 0, 127);
  }
}

byte calcPreferredPressure(byte z) {
    return constrain(unsigned(z) + 16 * Global.pressureSensitivity, 0, 127);
}

// Calculate the velocity value by providing pressure (z) data.
// This function will return true when a new stable velocity value has been
// calculated. This is the moment when a new note should be sent out.
boolean calcVelocity(byte z) {
  if (cell().vcount < VELOCITY_SAMPLES) {
    cell().velocity = max(cell().velocity, z);
    cell().vcount++;
    if (cell().vcount == VELOCITY_SAMPLES && cell().velocity > 0) {

      cell().velocity = calcPreferredVelocity(cell().velocity);

      return true;
    }
  }

  return false;
}

#define TRANSFER_SLIDE_PROXIMITY 100

boolean potentialSlideTransferCandidate(int col) {
  if (col < 1) return false;
  if (sensorSplit != getSplitOf(col)) return false;
  if (!isLowRow() && (!Split[sensorSplit].sendX || !isFocusedCell(col, sensorRow))) return false;
  if (isLowRow() && !lowRowRequiresSlideTracking()) return false;
  if (isStrummingSplit(sensorSplit)) return false;

  return cell(col, sensorRow).touched != untouchedCell &&                                               // the sibling cell has an active touch
    (cell(col, sensorRow).pendingReleaseCount ||                                                        // either a release is pending to be performed, or
     abs(cell().calibratedX() - cell(col, sensorRow).currentCalibratedX) < TRANSFER_SLIDE_PROXIMITY);   // both cells are touched simultaneously on the edges
}

boolean isReadyForSlideTransfer(int col) {
  return cell(col, sensorRow).pendingReleaseCount ||     // there's a pending release waiting
    cell().rawZ > cell(col, sensorRow).rawZ;             // the cell pressure is higher
}

boolean hasImpossibleX() {             // checks whether the calibrated X is outside of the possible bounds for the current cell
  return Global.calibrated &&
    (cell().calibratedX() < FXD_TO_INT(Global.calRows[sensorCol][0].fxdReferenceX - CALX_FULL_UNIT) ||
     cell().calibratedX() > FXD_TO_INT(Global.calRows[sensorCol][0].fxdReferenceX + CALX_FULL_UNIT));
}

void transferFromSameRowCell(byte col) {
  cell().initialX = cell(col, sensorRow).initialX;
  cell().initialReferenceX = cell(col, sensorRow).initialReferenceX;  
  cell().lastMovedX = cell(col, sensorRow).lastMovedX;
  cell().fxdRateX = cell(col, sensorRow).fxdRateX;
  cell().rateCountX = cell(col, sensorRow).rateCountX;  
  cell().initialY = cell(col, sensorRow).initialY;
  cell().note = cell(col, sensorRow).note;
  cell().channel = cell(col, sensorRow).channel;
  cell().fxdPrevPressure = cell(col, sensorRow).fxdPrevPressure;
  cell().fxdPrevTimbre = cell(col, sensorRow).fxdPrevTimbre;
  cell().velocity = cell(col, sensorRow).velocity;
  cell().vcount = cell(col, sensorRow).vcount;
  noteTouchMapping[sensorSplit].changeCell(cell().note, cell().channel, sensorCol, sensorRow);

  cell(col, sensorRow).clearSensorData();

  cell(col, sensorRow).note = -1;
  cell(col, sensorRow).channel = -1;
  cell(col, sensorRow).fxdPrevPressure = 0;
  cell(col, sensorRow).fxdPrevTimbre = 0;
  cell(col, sensorRow).velocity = 0;
  // do not reset vcount!

  byte channel = cell().channel;
  if (channel != -1 && col == focus(sensorSplit, channel).col && sensorRow == focus(sensorSplit, channel).row) {
    focus(sensorSplit, channel).col = sensorCol;
    focus(sensorSplit, channel).row = sensorRow;
  }
}

void transferToSameRowCell(byte col) {
  cell(col, sensorRow).initialX = cell().initialX;
  cell(col, sensorRow).initialReferenceX = cell().initialReferenceX;
  cell(col, sensorRow).lastMovedX = cell().lastMovedX;
  cell(col, sensorRow).fxdRateX = cell().fxdRateX;
  cell(col, sensorRow).rateCountX = cell().rateCountX;
  cell(col, sensorRow).initialY = cell().initialY;
  cell(col, sensorRow).note = cell().note;
  cell(col, sensorRow).channel = cell().channel;
  cell(col, sensorRow).fxdPrevPressure = cell().fxdPrevPressure;
  cell(col, sensorRow).fxdPrevTimbre = cell().fxdPrevTimbre;
  cell(col, sensorRow).velocity = cell().velocity;
  cell(col, sensorRow).vcount = cell().vcount;
  noteTouchMapping[sensorSplit].changeCell(cell(col, sensorRow).note, cell(col, sensorRow).channel, col, sensorRow);

  cell().clearSensorData();

  cell().note = -1;
  cell().channel = -1;
  cell().fxdPrevPressure = 0;
  cell().fxdPrevTimbre = 0;
  cell().velocity = 0;
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
  int32_t rowsInSensorColTouched = rowsInColsTouched[sensorCol] & ~(1 << sensorRow);
  int32_t colsInSensorRowTouched = colsInRowsTouched[sensorRow] & ~(1 << sensorCol);
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
        if (rowsInColsTouched[touchedCol] & (1 << touchedRow)) {

          // since we found four corners, we now have to determine which ones are
          // real presses and which ones are phantom presses, so we're looking for
          // the other corner that was scanned twice to determine which one has the
          // lowest pressure, this is the most likely to be the phantom press
          if (hasImpossibleX() ||
              (cell(touchedCol, touchedRow).isHigherPhantomPressure(cell().rawZ) &&
               cell(sensorCol, touchedRow).isHigherPhantomPressure(cell().rawZ) &&
               cell(touchedCol, sensorRow).isHigherPhantomPressure(cell().rawZ))) {

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
    cell().setPhantoms(sensorCol, sensorCol, sensorRow, sensorRow);
    return true;
  }

  return false;
}

int countTouchesInColumn() {
  int count = 0;
  int32_t rowsInSensorColTouched = rowsInColsTouched[sensorCol];
  if (rowsInSensorColTouched) {
    while (rowsInSensorColTouched) {
      byte touchedRow = 31 - __builtin_clz(rowsInSensorColTouched);
      count++;

      // turn the left-most active bit off, to continue the iteration over the touched rows
      rowsInSensorColTouched &= ~(1 << touchedRow);
    }
  }

  return count;
}

void handleSlideTransferCandidate(byte siblingCol, byte z) {
  // if the pressure gets higher than adjacent cell, the slide is transitioning over
  if (isReadyForSlideTransfer(siblingCol)) {
    transferFromSameRowCell(siblingCol);
    if (cell(siblingCol, sensorRow).touched != untouchedCell) {
      cell(siblingCol, sensorRow).touched = transferCell;
    }
    handleXYZupdate(z);
  }
  // otherwise act as if this new touch never happend
  else {
    cellTouched(transferCell);
  }
}

void handleNewTouch(byte z) {                             // the pressure value of the new touch

  DEBUGPRINT((1,"handleNewTouch"));
  DEBUGPRINT((1," col="));DEBUGPRINT((1,(int)sensorCol));
  DEBUGPRINT((1," row="));DEBUGPRINT((1,(int)sensorRow));
  DEBUGPRINT((1," z="));DEBUGPRINT((1,(int)z));
  DEBUGPRINT((1,"\n"));

  cellTouched(touchedCell);                                 // mark this cell as touched

  if (scrollingActive) {                                    // allow any new touch to cancel scrolling
    stopScrolling = true;
    return;
  }

  if (sensorCol == 0) {                                     // if it's a command button, handle it
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

    case displayNormal:                                            // it's normal performance mode
    case displayVolume:                                            // it's a volume change

      // check if the new touch could be an ongoing slide to the right
      if (potentialSlideTransferCandidate(sensorCol-1)) {
        handleSlideTransferCandidate(sensorCol-1, z);
      }
      // check if the new touch could be an ongoing slide to the left
      else if (potentialSlideTransferCandidate(sensorCol+1)) {
        handleSlideTransferCandidate(sensorCol+1, z);
      }
      // only allow a certain number of touches in a single column to prevent cross talk
      else if (countTouchesInColumn() > MAX_TOUCHES_IN_COLUMN) {
        cellTouched(ignoredCell);
      }
      // this is really a new touch without any relationship to an ongoing slide
      // however, it could be the low row and in certain situations it doesn't allow new touches
      else if (!isLowRow() || allowNewTouchOnLowRow()) {
        initVelocity();
        calcVelocity(z);
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
    case displayCCForY:                                            // it's a CC for Y change
      handleCCForYNewTouch();
      break;
    case displayCCForZ:                                            // it's a CC for Z change
      handleCCForZNewTouch();
      break;
    case displayOctaveTranspose:                                   // it's a transpose change
      handleOctaveTransposeNewTouch();
      break;
    case displayGlobal:                                            // it's a change to one of the global settings
    case displayGlobalWithTempo:
      handleGlobalSettingNewTouch();
      break;
    }
  }
}

// Calculate the transposed note number for the current cell by taken the transposition settings into account
int cellTransposedNote() {
  return transposedNote(sensorSplit, sensorCol, sensorRow);
}

int transposedNote(byte split, byte col, byte row) {
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

  FocusCell &focused = focus(getSplitOf(col), cell(col, row).channel);
  return col == focused.col && row == focused.row;
}

// Check if X/Y expression should be sent for this cell
boolean isXYExpressiveCell() {
  return isFocusedCell();
}

// Check if Z expression should be sent for this cell
boolean isZExpressiveCell() {
  switch (Split[sensorSplit].midiMode)
  {
    case oneChannel:
      if (Split[sensorSplit].expressionForZ == loudnessPolyPressure) {
        return true;
      }
      else {
        return isFocusedCell();
      }
    case channelPerNote:
    case channelPerRow:
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

#define PITCH_HOLD_DURATION 32             // the number of samples over which pitch hold quantize will interpolate to correct the pitch, the higher, the slower
#define ROGUE_PITCH_SWEEP_THRESHOLD 4      // the maximum threshold of instant X changes since the previous sample, anything higher will be considered a rogue pitch sweep

const int32_t fxdRateXSamples = FXD_FROM_INT(5);   // the number of samples over which the average rate of change of X is calculated
const int32_t fxdRateXThreshold = FXD_MAKE(1.6);   // the threshold below which the average rate of change of X is considered 'stationary' and pitch hold quantization will start to occur
const int32_t fxdPitchHoldDuration = FXD_FROM_INT(PITCH_HOLD_DURATION);

// handleXYZupdate:
// Called when a cell is held, in order to read X, Y or Z movements and send MIDI messages as appropriate
void handleXYZupdate(byte z) {                          // input: the current Z value of the cell

  // if the touch is in the control buttons column, ignore it
  if (sensorCol == 0) return;

  // if this data point serves as a calibration sample, return immediately
  if (handleCalibrationSample(z)) return;

  // only continue if the active display modes require finger tracking
  if (displayMode != displayNormal &&
      displayMode != displayVolume &&
      (displayMode != displaySplitPoint || splitButtonDown)) return;

  DEBUGPRINT((2,"handleXYZupdate"));
  DEBUGPRINT((2," col="));DEBUGPRINT((2,(int)sensorCol));
  DEBUGPRINT((2," row="));DEBUGPRINT((2,(int)sensorRow));
  DEBUGPRINT((2," z="));DEBUGPRINT((2,(int)z));
  DEBUGPRINT((2,"\n"));

  boolean newVelocity = calcVelocity(z);

  // check if after a new velocity calculation, this cell is not a phantom touch
  // we piggy-back off of the velocity calculation delay to ensure that we have at
  // least one full scan of the surface before making comparisons against other cells
  if (newVelocity && isPhantomTouch()) {
    cellTouched(untouchedCell);
    return;
  }

  // update the low row state
  handleLowRowState(z);

  // is this cell used for low row functionality
  if (isLowRow()) {
    if (newVelocity) {
      lowRowStart();
    }
    return;
  }

  // the volume fader has its own operation mode
  if (displayMode == displayVolume) {
    handleVolumeNewTouch();
    return;
  }

  // CC faders have their own operation mode
  if (Split[sensorSplit].ccFaders) {
    handleFaderTouch(z, newVelocity);
    return;
  }

  // this cell corresponds to a playing note
  if (newVelocity) {
    cell().lastTouch = millis();
    
    // Split strum only triggers notes in the other split
    if (isStrummingSplit(sensorSplit)) {
      handleSplitStrum();
      return;
    }
    else {
      int notenum = cellTransposedNote();
      if (notenum < 0 || notenum > 127) { // if the note number is outside of MIDI range, don't start it
        return;
      }
      handleNewNote(notenum);
    }
  }
  // no note was started, in that case, don't process the data update further
  else if (!cell().hasNote()) {
    return;
  }
  else {    
    cell().velocity = calcPreferredVelocity(z);
  }

  handleZExpression(z);

  // Only process x and y data when there's meaningful pressure on the cell
  if (z) {
    handleXExpression();
    handleYExpression();
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
    TouchInfo &touchedCell = cell(touchedCol, sensorRow);
    if (touchedCell.hasNote()) {
      // use the velocity of the strum touch
      touchedCell.velocity = cell(sensorCol, sensorRow).velocity;

      // retrigger the MIDI note
      midiSendNoteOff(touchedCell.note, touchedCell.channel);
      midiSendNoteOn(touchedCell.note, touchedCell.velocity, touchedCell.channel);
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

void handleNewNote(int notenum) {
  byte channel = takeChannel();
  cell().note = notenum;
  cell().channel = channel;
  
  // change the focused cell
  FocusCell &focused = focus(sensorSplit, channel);
  focused.col = sensorCol;
  focused.row = sensorRow;

  // reset the pitch bend right before sending the note on
  if (isXYExpressiveCell() && !isLowRowBendActive(sensorSplit)) {
    preSendPitchBend(sensorSplit, 0, cell().channel);
  }

  cell().lastMovedX = 0;
  cell().shouldRefreshX = true;
  cell().initialX = -1;

  // register the reverse mapping
  noteTouchMapping[sensorSplit].noteOn(notenum, channel, sensorCol, sensorRow);

  // send the note on
  if (!isArpeggiatorEnabled(sensorSplit) && !isStrummedSplit( sensorSplit)) {
    midiSendNoteOn(cell().note, cell().velocity, cell().channel);
  }

  // highlight same notes of this is activated
  if (Split[sensorSplit].colorNoteon) {
    highlightNoteCells(Split[sensorSplit].colorNoteon, sensorSplit, cell().note);
  }
}

const int32_t FXD_MIN_SLEW = FXD_FROM_INT(1);

void handleZExpression(byte z) {
  unsigned preferredPressure = calcPreferredPressure(z);

  // handle pressure transition between adjacent cells if they are not playing their own note
  int adjacentZ = 0;
  if (cell(sensorCol-1, sensorRow).rawZ && !cell(sensorCol-1, sensorRow).hasNote()) {
    adjacentZ = cell(sensorCol-1, sensorRow).rawZ;
  }
  else if (cell(sensorCol+1, sensorRow).rawZ && !cell(sensorCol+1, sensorRow).hasNote()) {
    adjacentZ = cell(sensorCol+1, sensorRow).rawZ;
  }
  // the adjacent Z value is adapted so that it can be added the active cell's pressure to make
  // up for the pressure differential while moving across cells
  adjacentZ = (adjacentZ / 5) & 0x7F;
  preferredPressure = constrain(preferredPressure + adjacentZ, 0, 127);

  // the faster we move the slower the slew rate becomes,
  // if we're holding still the pressure changes are almost instant, if we're moving faster they are averaged out
  int32_t slewRate = cell().fxdRateX;

  // adapt the slew rate based on the rate of change on the pressure, the smaller the change, the higher the slew rate
  slewRate += FXD_FROM_INT(2) - FXD_DIV(abs(FXD_FROM_INT(preferredPressure) - cell().fxdPrevPressure), FXD_FROM_INT(64));

  if (slewRate > FXD_MIN_SLEW) {
    // we also keep track of the previous pressure on the cell and average it out with
    // the current pressure to smooth over the rate of change when transiting between cells
    int32_t fxdAveragedPressure = cell().fxdPrevPressure;
    fxdAveragedPressure += FXD_DIV(FXD_FROM_INT(preferredPressure), slewRate);
    fxdAveragedPressure -= FXD_DIV(cell().fxdPrevPressure, slewRate);
    cell().fxdPrevPressure = fxdAveragedPressure;

    // calculate the final pressure value
    preferredPressure = constrain(FXD_TO_INT(fxdAveragedPressure), 0, 127);
  }
  else {
    cell().fxdPrevPressure = FXD_FROM_INT(preferredPressure);
  }

  // if sensing Z is enabled...
  // send different pressure update depending on midiMode
  if (Split[sensorSplit].sendZ && isZExpressiveCell()) {
    preSendPressure(cell().note, preferredPressure, cell().channel);
  }

  // save this cell's Z value
  cell().currentZ = z;
}

void handleXExpression() {
  cell().refreshX();

  int movedX;
  int calibratedX = cell().calibratedX();

  // determine if a slide transfer is in progress and which column it is with
  int transferCol = 0;
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
    int totalZ = cell(transferCol, sensorRow).rawZ + cell().rawZ;
    int32_t fxdTransferRatio = FXD_DIV(FXD_FROM_INT(cell(transferCol, sensorRow).rawZ), FXD_FROM_INT(totalZ));
    int32_t fxdCellRatio = FXD_FROM_INT(1) - fxdTransferRatio;

    int32_t fxdTransferCalibratedX = FXD_MUL(FXD_FROM_INT(cell(transferCol, sensorRow).currentCalibratedX), fxdTransferRatio);
    int32_t fxdCellCalibratedX = FXD_MUL(FXD_FROM_INT(cell().calibratedX()), fxdCellRatio);
    calibratedX = FXD_TO_INT(fxdTransferCalibratedX + fxdCellCalibratedX);
  }

  // calculate the distance from the initial X position
  // if pitch quantize is on, the first X position becomes the center point and considered 0
  if (Split[sensorSplit].pitchCorrectQuantize) {
    movedX = calibratedX - cell().initialX;
  }
  // otherwise we use the intended centerpoint based on the calibration
  else {
    movedX = calibratedX - cell().initialReferenceX;
  }

  int deltaX = abs(movedX - cell().lastMovedX);                                   // calculate how much change there was since the last X update
  cell().fxdRateX -= FXD_DIV(cell().fxdRateX, fxdRateXSamples);                   // calculate the average rate of X value changes over a number of samples
  cell().fxdRateX += FXD_DIV(FXD_FROM_INT(deltaX), fxdRateXSamples);

  if (!cell().hasPhantoms() ||                                                    // if no phantom presses are active, send the pitch bend change
      deltaX < ROGUE_PITCH_SWEEP_THRESHOLD ) {                                    // if there are phantom presses, only send those changes that are small and gradual to prevent rogue pitch sweeps

    cell().lastMovedX = movedX;
    int pitchBend = 0;

    if (Split[sensorSplit].pitchCorrectHold) {                                    // if pitch quantize is active on hold, interpolate between the ideal pitch and the current touch pitch
      int32_t fxdMovedRatio = FXD_DIV(FXD_FROM_INT(PITCH_HOLD_DURATION - cell().rateCountX), fxdPitchHoldDuration);
      int32_t fxdCorrectedRatio = FXD_FROM_INT(1) - fxdMovedRatio;
      int32_t fxdQuantizedDistance = Global.calRows[sensorCol][0].fxdReferenceX - FXD_FROM_INT(cell().initialReferenceX);
      
      int32_t fxdInterpolatedX = FXD_MUL(FXD_FROM_INT(movedX), fxdMovedRatio) + FXD_MUL(fxdQuantizedDistance, fxdCorrectedRatio);
      pitchBend = FXD_TO_INT(fxdInterpolatedX);

      // if the pich has stabilized, adapt the touch's initial X position so that pitch changes start from the stabilized pitch
      if (PITCH_HOLD_DURATION == cell().rateCountX) {
        cell().initialX = cell().currentCalibratedX - FXD_TO_INT(fxdQuantizedDistance);
      }
    }
    else {                                                                        // if pitch quantize on hold is disabled, just output the current touch pitch
      pitchBend = movedX;
    }

    // if X-axis movements are enabled and it's a candidate for
    // X/Y expression based on the MIDI mode and the currently held down cells
    if (isXYExpressiveCell() && Split[sensorSplit].sendX && !isLowRowBendActive(sensorSplit) &&
        // when there are multiple touches in the same column, reduce the pitch bend Z sensititivity to
        // prevent unwanted pitch slides
        (countTouchesInColumn() < 2 || cell().rawZ > SENSOR_PITCH_Z)) {
      preSendPitchBend(sensorSplit, pitchBend, cell().channel);
    }
  }

  // keep track of how many times the X changement rate drops below the threshold or above
  if (cell().fxdRateX < fxdRateXThreshold) {
    if (cell().rateCountX < PITCH_HOLD_DURATION) {
      cell().rateCountX++;
    }
  }
  else if (cell().rateCountX > 0) {
    cell().rateCountX--;
  }
}

const int32_t MAX_VALUE_Y = FXD_FROM_INT(127);
const int32_t SLEW_DIVIDER_Y = FXD_FROM_INT(26);
const int32_t BASE_SLEW_Y = FXD_FROM_INT(2);
const int32_t MIN_SLEW_Y = FXD_FROM_INT(3);

void handleYExpression() {
  cell().refreshY();

  int preferredTimbre;
  if (Split[sensorSplit].relativeY) {
    preferredTimbre = constrain(64 + (cell().currentCalibratedY - cell().initialY ), 0, 127);
  }
  else {
    preferredTimbre = cell().currentCalibratedY;
  }

  // base slew rate for Y is 2
  int32_t slewRate = BASE_SLEW_Y;

  // the faster we move horizontally, the slower the slew rate becomes,
  // if we're holding still the timbre changes are almost instant, if we're moving faster they are averaged out
  slewRate += cell().fxdRateX;

  // average the Y movements in reverse relation to the pressure, the harder you press, the less averaged they are
  slewRate += FXD_DIV(MAX_VALUE_Y - cell().fxdPrevPressure, SLEW_DIVIDER_Y);

  // never use an Y slew rate below 3
  if (slewRate < MIN_SLEW_Y) {
    slewRate = MIN_SLEW_Y;
  }

  int32_t fxdAveragedTimbre = cell().fxdPrevTimbre;
  fxdAveragedTimbre += FXD_DIV(FXD_FROM_INT(preferredTimbre), slewRate);
  fxdAveragedTimbre -= FXD_DIV(cell().fxdPrevTimbre, slewRate);
  cell().fxdPrevTimbre = fxdAveragedTimbre;

  // if Y-axis movements are enabled and it's a candidate for
  // X/Y expression based on the MIDI mode and the currently held down cells
  if (isXYExpressiveCell() && Split[sensorSplit].sendY &&
      (!isLowRowCC1Active(sensorSplit) || Split[sensorSplit].ccForY != 1)) {
    preSendY(sensorSplit, FXD_TO_INT(fxdAveragedTimbre), cell().channel);
  }
}

void releaseChannel(byte channel) {
  if (Split[sensorSplit].midiMode == channelPerNote) {
    splitChannels[sensorSplit].release(channel);
  }
}

#define PENDING_RELEASE_START 2

// Called when a touch is released to handle note off or other release events
void handleTouchRelease() {

  DEBUGPRINT((1,"handleTouchRelease"));
  DEBUGPRINT((1," col="));DEBUGPRINT((1,(int)sensorCol));
  DEBUGPRINT((1," row="));DEBUGPRINT((1,(int)sensorRow));
  DEBUGPRINT((1,"\n"));

  // if a release is pending, decrease the counter
  if (cell().pendingReleaseCount > 0) {
    cell().pendingReleaseCount--;
  }
  // if no release is pending and the rate of change of X is high, start a pending release
  else if (cell().fxdRateX > FXD_FROM_INT(7)) {
    cell().pendingReleaseCount = PENDING_RELEASE_START;
  }

  // if a release is pending, don't perform the release logic yet
  if (cell().pendingReleaseCount > 0) {
    return;
  }

  // mark this cell as no longer touched
  cellTouched(untouchedCell);

  // if touch release is in column 0, it's a command key release so handle it
  if (sensorCol == 0) {
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
    case displayCCForY:
      handleCCForYRelease();
      return;
    case displayCCForZ:
      handleCCForZRelease();
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
  }

  // check if calibration is active and its cell release logic needs to be executed
  handleCalibrationRelease();

  // is this cell used for low row functionality
  if (isLowRow()) {
    lowRowStop();
  }
  else if (cell().hasNote()) {

    // unregister the note <> cell mapping
    noteTouchMapping[sensorSplit].noteOff(cell().note, cell().channel);

    // send the Note Off
    if (isArpeggiatorEnabled(sensorSplit)) {
      handleArpeggiatorNoteOff(sensorSplit, cell().note, cell().channel);
    } else {
      midiSendNoteOff(cell().note, cell().channel);
    }

    // unhighlight the same notes if this is activated
    if (Split[sensorSplit].colorNoteon) {
      // ensure that no other notes of the same value are still active
      boolean allNotesOff = true;
      for (byte ch = 0; ch < 16; ++ch) {
        if (lastValueMidiNotesOn[cell().note][ch] > 0) {
          allNotesOff = false;
          break;
        }
      }
      // if no notes are active anymore, reset the highlighted cells
      if (allNotesOff) {
        resetNoteCells(sensorSplit, cell().note);
      }
    }

    // reset the pitch bend when the note is released if that setting is active
    if (Split[sensorSplit].pitchResetOnRelease && isXYExpressiveCell() && !isLowRowBendActive(sensorSplit)) {
      preSendPitchBend(sensorSplit, 0, cell().channel);
    }

    // If the released cell had focus, reassign focus to the latest touched cell
    if (isFocusedCell()) {
      setFocusCellToLatest(sensorSplit, cell().channel);
    }

    releaseChannel(cell().channel);

    // Reset all this cell's musical data
    cell().note = -1;
    cell().channel = -1;
    cell().fxdPrevPressure = 0;
    cell().fxdPrevTimbre = 0;
  }

  cell().clearAllPhantoms();

  // reset velocity calculations
  cell().velocity = 0;
  cell().vcount = 0;

  cell().clearSensorData();
}


// nextSensorCell:
// Moves on to the next cell witin the total surface scan of all 208 cells.

// Columns and rows are scanned in non-sequential order to minimize sensor crosstalk
byte rowIndex[NUMROWS] = {0, 4, 1, 5, 2, 6, 3, 7};

byte colIndex[NUMCOLS] = {0, 1, 6, 11, 16, 21, 2, 7, 12, 17, 22, 3, 8, 13, 18, 23, 4, 9, 14, 19, 24, 5, 10, 15, 20, 25};

byte scannedCells[208][2] = {
  {0, 0}, {3, 4}, {7, 1}, {10, 5}, {13, 2}, {17, 6}, {20, 3}, {24, 7}, {1, 4}, {4, 0}, {8, 5}, {11, 1}, {14, 6}, {18, 2}, {21, 7}, {25, 3}, {2, 0}, {5, 4}, {9, 1}, {12, 5}, {15, 2}, {19, 6}, {22, 3}, {6, 7}, {16, 4}, {23, 0},
  {0, 4}, {3, 0}, {7, 5}, {10, 1}, {13, 6}, {17, 2}, {20, 7}, {24, 3}, {1, 0}, {4, 4}, {8, 1}, {11, 5}, {14, 2}, {18, 6}, {21, 3}, {25, 7}, {2, 4}, {5, 0}, {9, 5}, {12, 1}, {15, 6}, {19, 2}, {22, 7}, {6, 3}, {16, 0}, {23, 4},
  {0, 1}, {3, 5}, {7, 2}, {10, 6}, {13, 3}, {17, 7}, {20, 4}, {24, 0}, {1, 5}, {4, 1}, {8, 6}, {11, 2}, {14, 7}, {18, 3}, {21, 0}, {25, 4}, {2, 1}, {5, 5}, {9, 2}, {12, 6}, {15, 3}, {19, 7}, {22, 4}, {6, 0}, {16, 5}, {23, 1},
  {0, 5}, {3, 1}, {7, 6}, {10, 2}, {13, 7}, {17, 3}, {20, 0}, {24, 4}, {1, 1}, {4, 5}, {8, 2}, {11, 6}, {14, 3}, {18, 7}, {21, 4}, {25, 0}, {2, 5}, {5, 1}, {9, 6}, {12, 2}, {15, 7}, {19, 3}, {22, 0}, {6, 4}, {16, 1}, {23, 5},
  {0, 2}, {3, 6}, {7, 3}, {10, 7}, {13, 4}, {17, 0}, {20, 5}, {24, 1}, {1, 6}, {4, 2}, {8, 7}, {11, 3}, {14, 0}, {18, 4}, {21, 1}, {25, 5}, {2, 2}, {5, 6}, {9, 3}, {12, 7}, {15, 4}, {19, 0}, {22, 5}, {6, 1}, {16, 6}, {23, 2},
  {0, 6}, {3, 2}, {7, 7}, {10, 3}, {13, 0}, {17, 4}, {20, 1}, {24, 5}, {1, 2}, {4, 6}, {8, 3}, {11, 7}, {14, 4}, {18, 0}, {21, 5}, {25, 1}, {2, 6}, {5, 2}, {9, 7}, {12, 3}, {15, 0}, {19, 4}, {22, 1}, {6, 5}, {16, 2}, {23, 6},
  {0, 3}, {3, 7}, {7, 4}, {10, 0}, {13, 5}, {17, 1}, {20, 6}, {24, 2}, {1, 7}, {4, 3}, {8, 0}, {11, 4}, {14, 1}, {18, 5}, {21, 2}, {25, 6}, {2, 3}, {5, 7}, {9, 4}, {12, 0}, {15, 5}, {19, 1}, {22, 6}, {6, 2}, {16, 7}, {23, 3},
  {0, 7}, {3, 3}, {7, 0}, {10, 4}, {13, 1}, {17, 5}, {20, 2}, {24, 6}, {1, 3}, {4, 7}, {8, 4}, {11, 0}, {14, 5}, {18, 1}, {21, 6}, {25, 2}, {2, 7}, {5, 3}, {9, 0}, {12, 4}, {15, 1}, {19, 5}, {22, 2}, {6, 6}, {16, 3}, {23, 7}
};

void nextSensorCell()
{
  static byte cellCount;

  if (++cellCount >= 208)
  {
    cellCount = 0;
    checkTimeToReadFootSwitches(micros());
  }

  // we're keeping track of the state of X and Y so that we don't refresh it needlessly for finger tracking
  cell().shouldRefreshX = true;
  cell().shouldRefreshY = true;

  sensorCol = scannedCells[cellCount][0];
  sensorRow = scannedCells[cellCount][1];
  sensorSplit = getSplitOf(sensorCol);
}


// getNoteNumber:
// computes MIDI note number from current row, column, row offset, octave button and transposition amount
int getNoteNumber(byte col,                               // column number to be computed
                  byte row ) {                            // row number to be computed

  int notenum = 0;
  byte sp = getSplitOf(col);

  int offset, lowest;
  determineNoteOffsetAndLowest(sp, row, offset, lowest);

  // return the computed note based on the selected rowOffset
  notenum = lowest + (row * offset ) + (col - 1) + Split[sp].transposeOctave;

  return notenum;
}

void determineNoteOffsetAndLowest(byte split, byte row, int &offset, int &lowest) {
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

void getSplitBoundaries(byte sp, byte &lowCol, byte &highCol) {
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
byte getSplitOf(byte col) {

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
