/************************** ls_touchInfo: LinnStrument TouchInfo methods **************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These are the methods of the TouchInfo class, handling data related to the touch information of
each individual cell.
**************************************************************************************************/

void initializeTouchInfo() {
  // Initialize the cells array, starting operating with no touched cells
  for (byte col = 0; col < NUMCOLS; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      cell(col, row).touched = untouchedCell;
      cell(col, row).lastTouch = 0;
      cell(col, row).initialX = -1;
      cell(col, row).initialReferenceX = 0;
      cell(col, row).currentRawX = 0;
      cell(col, row).currentCalibratedX = 0;
      cell(col, row).lastMovedX = 0;
      cell(col, row).fxdRateX = 0;
      cell(col, row).rateCountX = 0;
      cell(col, row).shouldRefreshX = true;
      cell(col, row).initialY = -1;
      cell(col, row).currentRawY = 0;
      cell(col, row).currentCalibratedY = 0;
      cell(col, row).shouldRefreshY = true;
      cell(col, row).currentZ = 0;
      cell(col, row).rawZ = 0;
      cell(col, row).vcount = 0;
      cell(col, row).velocity = 0;
      cell(col, row).note = -1;
      cell(col, row).channel = -1;
      cell(col, row).fxdPrevPressure = 0;
      cell(col, row).fxdPrevTimbre = 0;
      cell(col, row).pendingReleaseCount = 0;
      cell(col, row).clearPhantoms();
    }
  }

  // Initialize the touch bitmasks
  for (byte col = 0; col < NUMCOLS; ++col) {
    rowsInColsTouched[col] = 0;
  }
  for (byte row = 0; row < NUMROWS; ++row) {
    colsInRowsTouched[row] = 0;
  }
}

int TouchInfo::rawX() {
  refreshX();
  return currentRawX;
}

int TouchInfo::calibratedX() {
  refreshX();
  return currentCalibratedX;
}

void TouchInfo::refreshX() {
  if (shouldRefreshX) {
    currentRawX = readX();
    currentCalibratedX = calculateCalibratedX(currentRawX);
    shouldRefreshX = false;

    // if this is the first X read for this touch...
    if (cell().initialX == -1) {
      // store the calibrated X reference that corresponds to the cell's note without any pitch bend
      cell().initialReferenceX = FXD_TO_INT(Global.calRows[sensorCol][0].fxdReferenceX);

      // store the initial X position
      cell().initialX = cell().currentCalibratedX;

      cell().fxdRateX = 0;
      cell().lastMovedX = 0;
    }
  }
}

int TouchInfo::rawY() {
  refreshY();
  return currentRawY;
}

void TouchInfo::refreshY() {
  if (shouldRefreshY) {
    currentRawY = readY();
    currentCalibratedY = calculateCalibratedY(currentRawY);
    shouldRefreshY = false;

    // if this is the first Y read for this touch...
    if (cell().initialY == -1) {
      // store the initial Y position
      cell().initialY = cell().currentCalibratedY;
    }
  }
}

int TouchInfo::calibratedY() {
  refreshY();
  return currentCalibratedY;
}

boolean TouchInfo::hasNote() {
  return note != -1 && channel != -1;
}

void TouchInfo::clearAllPhantoms() {
  if (hasPhantoms()) {
    cell(phantomCoords[0], phantomCoords[2]).clearPhantoms();
    cell(phantomCoords[1], phantomCoords[2]).clearPhantoms();
    cell(phantomCoords[0], phantomCoords[3]).clearPhantoms();
    cell(phantomCoords[1], phantomCoords[3]).clearPhantoms();
  }
}

void TouchInfo::clearPhantoms() {
  for (byte coord = 0; coord < 4; ++ coord) {
    phantomCoords[coord] = -1;
  }
}

boolean TouchInfo::hasPhantoms() {
  return phantomCoords[0] != -1 && phantomCoords[1] != -1 &&
    phantomCoords[2] != -1 && phantomCoords[3] != -1;
}

void TouchInfo::setPhantoms(byte col1, byte col2, byte row1, byte row2) {
  phantomCoords[0] = col1;
  phantomCoords[1] = col2;
  phantomCoords[2] = row1;
  phantomCoords[3] = row2;
}

boolean TouchInfo::isHigherPhantomPressure(int other) {
  return hasNote() || rawZ > other;
}