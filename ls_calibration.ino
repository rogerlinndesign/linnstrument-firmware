/***************************** ls_calibration: LinnStrument Calibration ***************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
Functions to sample the measured X and Y values on a particular instrument and to rectify so that
these values become predictible.

The aim is to calculate the ratio that needs to be applied to the scanned X position, so that it
can be uniformized to have identical distances between the centers of the cells. Additionally, this
distance is chosen to make the calibrated X values perfectly correct values for pitchbend when the
target is 48 semitones.

For the Y values, it simply measures the top and bottom extremes for cells in 5 columns. Then we
calculate for each cell the ratio that converts this to usable CC values.
***************************************************************************************************/

byte CALROWNUM;
byte CALCOLNUM;

// these are default starting points for uncalibrated LinnStruments, might need tweaking
int32_t FXD_CALX_DEFAULT_LEFT_EDGE;
int32_t FXD_CALX_DEFAULT_FIRST_CELL;
int32_t FXD_CALX_DEFAULT_CELL_WIDTH;
int32_t FXD_CALX_DEFAULT_RIGHT_EDGE;

// the leftmost and rightmost cells don't reach as far on the edges as other cells, this compensates for that
int32_t FXD_CALX_BORDER_OFFSET;

const short CALY_DEFAULT_MIN[MAXROWS] = {243, 781, 1299, 1810, 2281, 2718, 3187, 3599};
const short CALY_DEFAULT_MAX[MAXROWS] = {473, 991, 1486, 1965, 2449, 2925, 3401, 3851};

// only use a portion of the Y distance, since the fingers can't comfortably reach until the real edges
const byte CALY_MARGIN_FRACTION = 4;

void initializeCalibration() {
  if (LINNMODEL == 200) {
    CALROWNUM = 4;
    CALCOLNUM = 9;

    // these are default starting points for uncalibrated LinnStruments, might need tweaking
    FXD_CALX_DEFAULT_LEFT_EDGE = FXD_MAKE(188);
    FXD_CALX_DEFAULT_FIRST_CELL = FXD_MAKE(248);
    FXD_CALX_DEFAULT_CELL_WIDTH = FXD_MAKE(157);
    FXD_CALX_DEFAULT_RIGHT_EDGE = FXD_MAKE(4064);

    // the leftmost and rightmost cells don't reach as far on the edges as other cells, this compensates for that
    FXD_CALX_BORDER_OFFSET = FXD_MAKE(10);
  }
  else if (LINNMODEL == 128) {
    CALROWNUM = 4;
    CALCOLNUM = 6;

    // these are default starting points for uncalibrated LinnStruments, might need tweaking
    FXD_CALX_DEFAULT_LEFT_EDGE = FXD_MAKE(293.75);
    FXD_CALX_DEFAULT_FIRST_CELL = FXD_MAKE(387.5);
    FXD_CALX_DEFAULT_CELL_WIDTH = FXD_MAKE(245.3125);
    FXD_CALX_DEFAULT_RIGHT_EDGE = FXD_MAKE(4064);

    // the leftmost and rightmost cells don't reach as far on the edges as other cells, this compensates for that
    FXD_CALX_BORDER_OFFSET = FXD_MAKE(15.625);
  }
}

void initializeCalibrationSamples() {
  calibrationPhase = calibrationRows;

  for (byte col = 0; col < NUMCOLS; ++col) {
    for (byte row = 0; row < CALROWNUM; ++row) {
      calSampleRows[col][row].minValue = 4095;
      calSampleRows[col][row].maxValue = 0;
      calSampleRows[col][row].pass = 0;
    }
  }
  for (byte col = 0; col < CALCOLNUM; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      calSampleCols[col][row].minValue = 4095;
      calSampleCols[col][row].maxValue = 0;
      calSampleCols[col][row].pass = 0;
    }
  }
}

void initializeCalibrationData() {
  Device.calibrated = false;

  // Initialize default X calibration data
  for (byte row = 0; row < CALROWNUM; ++row) {
    Device.calRows[0][row].fxdReferenceX = FXD_MUL(FXD_FROM_INT(-1), CALX_HALF_UNIT) + FXD_CALX_BORDER_OFFSET;
    Device.calRows[0][row].fxdMeasuredX = FXD_CALX_DEFAULT_LEFT_EDGE;
    Device.calRows[0][row].fxdRatio = 0;

    for (byte col = 1; col < NUMCOLS; ++col) {
      Device.calRows[col][row].fxdReferenceX = FXD_MUL(CALX_FULL_UNIT, FXD_FROM_INT(col - 1)); // center in the middle of the cells
      Device.calRows[col][row].fxdMeasuredX = FXD_CALX_DEFAULT_FIRST_CELL + FXD_MUL(FXD_CALX_DEFAULT_CELL_WIDTH, FXD_FROM_INT(col - 1));
      Device.calRows[col][row].fxdRatio = FXD_DIV(CALX_FULL_UNIT, FXD_CALX_DEFAULT_CELL_WIDTH);
    }

    Device.calRows[NUMCOLS][row].fxdReferenceX = FXD_MUL(CALX_FULL_UNIT, FXD_FROM_INT(NUMCOLS - 1)) - CALX_HALF_UNIT - FXD_CALX_BORDER_OFFSET;
    Device.calRows[NUMCOLS][row].fxdMeasuredX = FXD_CALX_DEFAULT_RIGHT_EDGE;
    Device.calRows[NUMCOLS][row].fxdRatio = 0;
  }

  // Initialize default Y calibration data
  for (byte col = 0; col < CALCOLNUM; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      Device.calCols[col][row].minY = CALY_DEFAULT_MIN[row];
      Device.calCols[col][row].maxY = CALY_DEFAULT_MAX[row];
      Device.calCols[col][row].fxdRatio = FXD_DIV(FXD_FROM_INT(Device.calCols[col][row].maxY - Device.calCols[col][row].minY), CALY_FULL_UNIT);
    }
  }
}

short calculateCalibratedX(short rawX) {
  int32_t fxdRawX = FXD_FROM_INT(rawX);

  byte sector = (sensorRow / 3);
  byte sectorTop = sector + 1;

  byte bottomRow = 0;
  byte topRow = 2;
  switch (sector) {
    case 0: bottomRow = 0; topRow = 2; break;
    case 1: bottomRow = 2; topRow = 5; break;
    case 2: bottomRow = 5; topRow = 7; break;
  }

  // We calculate the calibrated X position for the bottom sector row for the current sensor column
  int32_t fxdBottomX = Device.calRows[sensorCol][sector].fxdReferenceX + FXD_MUL(fxdRawX - Device.calRows[sensorCol][sector].fxdMeasuredX, Device.calRows[sensorCol][sector].fxdRatio);

  // We calculate the calibrated X position for the top sector row for the current sensor column
  int32_t fxdTopX = Device.calRows[sensorCol][sectorTop].fxdReferenceX + FXD_MUL(fxdRawX - Device.calRows[sensorCol][sectorTop].fxdMeasuredX, Device.calRows[sensorCol][sectorTop].fxdRatio);

  // The final calibrated X position is the interpolation between the bottom and the top sector rows based on the current sensor row
  short result = FXD_TO_INT(fxdBottomX + FXD_MUL(FXD_DIV(fxdTopX - fxdBottomX, FXD_FROM_INT(topRow - bottomRow)), FXD_FROM_INT(sensorRow - bottomRow)));

  // constrain the calibrated X position to have a full 4095 range between the centers of the left and right cells,
  // but still have values for the remaining left and right halves
  result = constrain(result, -CALX_VALUE_MARGIN, 4095+CALX_VALUE_MARGIN);

  return result;
}

signed char calculateCalibratedY(short rawY) {
  byte col = (sensorCol - 1) / 3;
  byte row = sensorRow;

  int32_t fxdLeftY = FXD_DIV(FXD_FROM_INT(constrain(rawY, Device.calCols[col][row].minY, Device.calCols[col][row].maxY) - Device.calCols[col][row].minY), Device.calCols[col][row].fxdRatio);
  int32_t fxdRightY = 0;
  if (col < 8) {
    fxdRightY = FXD_DIV(FXD_FROM_INT(constrain(rawY, Device.calCols[col+1][row].minY, Device.calCols[col+1][row].maxY) - Device.calCols[col+1][row].minY), Device.calCols[col+1][row].fxdRatio);
  }

  byte bias = (sensorCol - 1) % 3;
  short result = FXD_TO_INT(FXD_MUL(fxdLeftY, FXD_DIV(FXD_CONST_3 - FXD_FROM_INT(bias), FXD_CONST_3)) +
                            FXD_MUL(fxdRightY, FXD_DIV(FXD_FROM_INT(bias), FXD_CONST_3)));

  // Bound the Y position to accepted value limits 
  result = constrain(result, 0, 127);

  return result;
}

boolean handleCalibrationSample() {
  // calibrate the X value distribution by measuring the minimum and maximum for each cell
  if (displayMode == displayCalibration) {
    // only calibrate a deliberate touch that is at least half-way through the pressure sensitivity range
    if (sensorCell->isStableYTouch()) {
      short rawX = readX(0);
      short rawY = readY(0);
      if (calibrationPhase == calibrationRows && (sensorRow == 0 || sensorRow == 2 || sensorRow == 5 || sensorRow == 7)) {
        byte row = (sensorRow / 2);
        calSampleRows[sensorCol][row].minValue = min(rawX, calSampleRows[sensorCol][row].minValue);
        calSampleRows[sensorCol][row].maxValue = max(rawX, calSampleRows[sensorCol][row].maxValue);
      }
      else if (calibrationPhase == calibrationCols && (sensorCol % 3 == 1)) {
        byte col = (sensorCol - 1) / 3;
        calSampleCols[col][sensorRow].minValue = min(rawY, calSampleCols[col][sensorRow].minValue);
        calSampleCols[col][sensorRow].maxValue = max(rawY, calSampleCols[col][sensorRow].maxValue);
      }
    }

    return true;
  }

  return false;
}

boolean handleCalibrationRelease() {
  // Handle calibration passes, at least two before indicating green
  if (displayMode == displayCalibration) {
    signed char cellPass = -1;

    if (calibrationPhase == calibrationRows && (sensorRow == 0 || sensorRow == 2 || sensorRow == 5 || sensorRow == 7)) {
      byte i1 = sensorCol;
      byte i2 = (sensorRow / 2);

      if (calSampleRows[i1][i2].maxValue - calSampleRows[i1][i2].minValue > 80) {    // only proceed when at least a delta of 80 in X values is measured
        cellPass = calSampleRows[i1][i2].pass;
        calSampleRows[i1][i2].pass += 1;
      }
    }
    else if (calibrationPhase == calibrationCols && (sensorCol % 3 == 1)) {
      byte i1 = (sensorCol - 1) / 3;
      byte i2 = sensorRow;

      if (calSampleCols[i1][i2].maxValue - calSampleCols[i1][i2].minValue > 180) {    // only proceed when at least a delta of 180 in Y values is measured
        cellPass = calSampleCols[i1][i2].pass;
        calSampleCols[i1][i2].pass += 1;
      }
    }

    // This is the first pass for a sensor, switch the led to cyan
    if (cellPass == 0) {
      setLed(sensorCol, sensorRow, COLOR_CYAN, cellOn);
    }
    // This is the second or more pass for a sensor, switch the led to green
    // We need at least two passes to consider the calibration viable
    else if (cellPass > 0) {
      setLed(sensorCol, sensorRow, COLOR_GREEN, cellOn);

      // Scan all the calibration samples to see if at least two passes were made
      // for each cell of the rows
      if (calibrationPhase == calibrationRows) {
        boolean rowsOk = true;
        for (byte col = 1; col < NUMCOLS && rowsOk; ++col) {
          for (byte row = 0; row < CALROWNUM && rowsOk; ++row) {
            if (calSampleRows[col][row].pass < 2) {
              rowsOk = false;
            }
          }
        }

        if (rowsOk) {
          calibrationPhase = calibrationCols;
          updateDisplay();
        }
      }
      // Scan all the calibration samples to see if at least two passes were made
      // for each cell of the columns
      else if (calibrationPhase == calibrationCols) {
        boolean colsOk = true;

        for (byte row = 0; row < NUMROWS && colsOk; ++row) {
          for (byte col = 0; col < CALCOLNUM && colsOk; ++col) {
            if (calSampleCols[col][row].pass < 2) {
              colsOk = false;
            }
          }
        }

        // When the calibration is done, calculate the calibration data and notify the user that everything is ok
        if (colsOk) {

          // Calculate the calibration X data based on the collected samples
          for (byte row = 0; row < CALROWNUM; ++row) {

            // The first calibration entry basically indicates the leftmost limit of the measured X values
            Device.calRows[0][row].fxdMeasuredX = FXD_FROM_INT(calSampleRows[1][row].minValue);
            Device.calRows[0][row].fxdRatio = 0;

            // Calculate all the calibration entries in between that use the width of the cells
            for (byte col = 1; col < NUMCOLS; ++col) {
              Device.calRows[col][row].fxdMeasuredX = FXD_FROM_INT(calSampleRows[col][row].minValue) + FXD_DIV(FXD_FROM_INT(calSampleRows[col][row].maxValue - calSampleRows[col][row].minValue), FXD_CONST_2);
              Device.calRows[col][row].fxdRatio = FXD_DIV(CALX_FULL_UNIT, FXD_FROM_INT(calSampleRows[col][row].maxValue - calSampleRows[col][row].minValue));
            }

            // The last entry marks the rightmost measured X value
            Device.calRows[NUMCOLS][row].fxdMeasuredX = FXD_FROM_INT(calSampleRows[NUMCOLS-1][row].maxValue);
            Device.calRows[NUMCOLS][row].fxdRatio = 0;
          }

          // Store and calculate the calibration Y data based on the collected samples
          for (byte row = 0; row < NUMROWS; ++row) {
            for (byte col = 0; col < CALCOLNUM; ++col) {
              int sampledRange = calSampleCols[col][row].maxValue - calSampleCols[col][row].minValue;
              int cellMarginY = (sampledRange / CALY_MARGIN_FRACTION);
              Device.calCols[col][row].minY = max(0, calSampleCols[col][row].minValue + cellMarginY);
              Device.calCols[col][row].maxY = min(4095, calSampleCols[col][row].maxValue - cellMarginY);
              Device.calCols[col][row].fxdRatio = FXD_DIV(FXD_FROM_INT(Device.calCols[col][row].maxY - Device.calCols[col][row].minY), CALY_FULL_UNIT);
            }
          }

          Device.calibrated = true;

#ifdef DEBUG_ENABLED
          debugCalibration();
#endif

          // automatically turn off serial mode when the calibration has been performed
          // immediately after the first boot since a firmware upgrade, this is to compensate
          // for older firmware versions that couldn't export their settings and still provide
          // a smooth user experience
          if (firstTimeBoot) {
            switchSerialMode(false);
          }

          // Draw the text OK and go back to normal display after a short delay
          calibrationPhase = calibrationInactive;
          clearDisplay();
          bigfont_draw_string((NUMCOLS-11)/2 - 1, 0, "OK", globalColor, false);
          delayUsec(500000);

          storeSettings();
                    
          initializeCalibrationSamples();
          initializeTouchInfo();

          setDisplayMode(displayNormal);
          clearLed(0, GLOBAL_SETTINGS_ROW);
          updateDisplay();
        }
      }
    }

    return true;
  }

  return false;
}

void debugCalibration() {
  for (byte row = 0; row < CALROWNUM; ++row) {
    for (byte col = 0; col < NUMCOLS; ++col) {
      DEBUGPRINT((0,"calRows"));
      DEBUGPRINT((0," col="));DEBUGPRINT((0,(int)col));
      DEBUGPRINT((0," row="));DEBUGPRINT((0,(int)row));
      DEBUGPRINT((0," sampleMin="));DEBUGPRINT((0,(int)calSampleRows[col][row].minValue));
      DEBUGPRINT((0," sampleMax="));DEBUGPRINT((0,(int)calSampleRows[col][row].maxValue));
      DEBUGPRINT((0," referenceX="));DEBUGPRINT((0,(int)FXD_TO_INT(Device.calRows[col][row].fxdReferenceX)));
      DEBUGPRINT((0," measuredX="));DEBUGPRINT((0,(int)FXD_TO_INT(Device.calRows[col][row].fxdMeasuredX)));
      DEBUGPRINT((0," ratio="));DEBUGPRINT((0,(int)FXD_TO_INT(FXD_MUL(Device.calRows[col][row].fxdRatio, FXD_CONST_100))));
      DEBUGPRINT((0,"\n"));
    }
    DEBUGPRINT((0,"calRows"));
    DEBUGPRINT((0," col="));DEBUGPRINT((0,(int)NUMCOLS));
    DEBUGPRINT((0," row="));DEBUGPRINT((0,(int)row));
    DEBUGPRINT((0," referenceX="));DEBUGPRINT((0,(int)FXD_TO_INT(Device.calRows[NUMCOLS][row].fxdReferenceX)));
    DEBUGPRINT((0," measuredX="));DEBUGPRINT((0,(int)FXD_TO_INT(Device.calRows[NUMCOLS][row].fxdMeasuredX)));
    DEBUGPRINT((0," ratio="));DEBUGPRINT((0,(int)FXD_TO_INT(FXD_MUL(Device.calRows[NUMCOLS][row].fxdRatio, FXD_CONST_100))));
    DEBUGPRINT((0,"\n"));
  }
  for (byte col = 0; col < CALCOLNUM; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      DEBUGPRINT((0,"calCols"));
      DEBUGPRINT((0," col="));DEBUGPRINT((0,(int)col));
      DEBUGPRINT((0," row="));DEBUGPRINT((0,(int)row));
      DEBUGPRINT((0," sampleMin="));DEBUGPRINT((0,(int)calSampleCols[col][row].minValue));
      DEBUGPRINT((0," sampleMax="));DEBUGPRINT((0,(int)calSampleCols[col][row].maxValue));
      DEBUGPRINT((0," minY="));DEBUGPRINT((0,(int)Device.calCols[col][row].minY));
      DEBUGPRINT((0," maxY="));DEBUGPRINT((0,(int)Device.calCols[col][row].maxY));
      DEBUGPRINT((0," ratio="));DEBUGPRINT((0,(int)FXD_TO_INT(FXD_MUL(Device.calCols[col][row].fxdRatio, FXD_CONST_100))));
      DEBUGPRINT((0,"\n"));
    }
  }
}
