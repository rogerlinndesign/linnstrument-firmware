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

const int32_t CALX_HALF_UNIT = FXD_MAKE(85.3125);    // 4095 / 48
const int32_t CALX_FULL_UNIT = FXD_MAKE(170.625);    // 4095 / 24

const int32_t CALY_FULL_UNIT = FXD_FROM_INT(127);    // range of 7-bit CC

// these are default starting points for uncalibrated LinnStruments, might need tweaking
const int32_t CALX_DEFAULT_LEFT_EDGE = FXD_FROM_INT(188);
const int32_t CALX_DEFAULT_FIRST_CELL = FXD_FROM_INT(248);
const int32_t CALX_DEFAULT_CELL_WIDTH = FXD_FROM_INT(157);
const int32_t CALX_DEFAULT_RIGHT_EDGE = FXD_FROM_INT(4064);

const short CALY_DEFAULT_MIN[NUMROWS] = {243, 781, 1299, 1810, 2281, 2718, 3187, 3599};
const short CALY_DEFAULT_MAX[NUMROWS] = {473, 991, 1486, 1965, 2449, 2925, 3401, 3851};

// the leftmost and rightmost cells don't reach as far on the edges as other cells, this compensates for that
const int32_t CALX_BORDER_OFFSET = FXD_FROM_INT(10);

// only use a portion of the Y distance, since the fingers can't comfortably reach until the real edges
const int32_t CALY_MARGIN_FRACTION = 4;

void initializeCalibrationSamples() {
  calibrationPhase = calibrationRows;

  for (byte col = 0; col < NUMCOLS; ++col) {
    for (byte row = 0; row < 4; ++row) {
      calSampleRows[col][row].minValue = 4095;
      calSampleRows[col][row].maxValue = 0;
      calSampleRows[col][row].pass = 0;
    }
  }
  for (byte col = 0; col < 9; ++col) {
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
  for (byte row = 0; row < 4; ++row) {
    Device.calRows[0][row].fxdReferenceX = FXD_MUL(FXD_FROM_INT(-1), CALX_HALF_UNIT) + CALX_BORDER_OFFSET;
    Device.calRows[0][row].fxdMeasuredX = CALX_DEFAULT_LEFT_EDGE;
    Device.calRows[0][row].fxdRatio = 0;

    for (byte col = 1; col < NUMCOLS; ++col) {
      Device.calRows[col][row].fxdReferenceX = FXD_MUL(CALX_FULL_UNIT, FXD_FROM_INT(col - 1)); // multiply by 1/24th of 4095 to be centered in the middle of the cells
      Device.calRows[col][row].fxdMeasuredX = CALX_DEFAULT_FIRST_CELL + FXD_MUL(CALX_DEFAULT_CELL_WIDTH, FXD_FROM_INT(col - 1));
      Device.calRows[col][row].fxdRatio = FXD_DIV(CALX_FULL_UNIT, CALX_DEFAULT_CELL_WIDTH);
    }

    Device.calRows[NUMCOLS][row].fxdReferenceX = FXD_MUL(CALX_FULL_UNIT, FXD_FROM_INT(NUMCOLS - 1)) - CALX_HALF_UNIT - CALX_BORDER_OFFSET;
    Device.calRows[NUMCOLS][row].fxdMeasuredX = CALX_DEFAULT_RIGHT_EDGE;
    Device.calRows[NUMCOLS][row].fxdRatio = 0;
  }

  // Initialize default Y calibration data
  for (byte col = 0; col < 9; ++col) {
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

  byte bottomRow;
  byte topRow;
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
  short result = FXD_TO_INT(FXD_MUL(fxdLeftY, FXD_DIV(FXD_FROM_INT(3 - bias), FXD_FROM_INT(3))) +
                            FXD_MUL(fxdRightY, FXD_DIV(FXD_FROM_INT(bias), FXD_FROM_INT(3))));

  // Bound the Y position to accepted value limits 
  result = constrain(result, 0, 127);

  return result;
}

bool handleCalibrationSample() {
  // calibrate the X value distribution by measuring the minimum and maximum for each cell
  if (displayMode == displayCalibration) {
    sensorCell().refreshX();
    sensorCell().refreshY();
    if (calibrationPhase == calibrationRows && (sensorRow == 0 || sensorRow == 2 || sensorRow == 5 || sensorRow == 7)) {
      byte row = (sensorRow / 2);
      calSampleRows[sensorCol][row].minValue = min(sensorCell().rawX(), calSampleRows[sensorCol][row].minValue);
      calSampleRows[sensorCol][row].maxValue = max(sensorCell().rawX(), calSampleRows[sensorCol][row].maxValue);
    }
    else if (calibrationPhase == calibrationCols && (sensorCol % 3 == 1)) {
      byte col = (sensorCol - 1) / 3;
      calSampleCols[col][sensorRow].minValue = min(sensorCell().rawY(), calSampleCols[col][sensorRow].minValue);
      calSampleCols[col][sensorRow].maxValue = max(sensorCell().rawY(), calSampleCols[col][sensorRow].maxValue);
    }

    return true;
  }

  return false;
}

void handleCalibrationRelease() {
  // Handle calibration passes, at least two before indicating green
  if (displayMode == displayCalibration) {
    signed char cellPass = -1;

    if (calibrationPhase == calibrationRows && (sensorRow == 0 || sensorRow == 2 || sensorRow == 5 || sensorRow == 7)) {
      byte i1 = sensorCol;
      byte i2 = (sensorRow / 2);

      if (calSampleRows[i1][i2].maxValue - calSampleRows[i1][i2].minValue > 80) {    // only proceed when at least a delta of 80 in X values is measured
        cellPass = (calSampleRows[i1][i2].pass++);
      }
    }
    else if (calibrationPhase == calibrationCols && (sensorCol % 3 == 1)) {
      byte i1 = (sensorCol - 1) / 3;
      byte i2 = sensorRow;

      if (calSampleCols[i1][i2].maxValue - calSampleCols[i1][i2].minValue > 180) {    // only proceed when at least a delta of 180 in Y values is measured
        cellPass = (calSampleCols[i1][i2].pass++);
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
        bool rowsOk = true;
        for (byte col = 1; col < NUMCOLS && rowsOk; ++col) {
          for (byte row = 0; row < 4 && rowsOk; ++row) {
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
        bool colsOk = true;

        for (byte row = 0; row < NUMROWS && colsOk; ++row) {
          for (byte col = 0; col < 9 && colsOk; ++col) {
            if (calSampleCols[col][row].pass < 2) {
              colsOk = false;
            }
          }
        }

        // When the calibration is done, calculate the calibration data and notify the user that everything is ok
        if (colsOk) {

          // Calculate the calibration X data based on the collected samples
          for (byte row = 0; row < 4; ++row) {

            // The first calibration entry basically indicates the leftmost limit of the measured X values
            Device.calRows[0][row].fxdMeasuredX = FXD_FROM_INT(calSampleRows[1][row].minValue);
            Device.calRows[0][row].fxdRatio = 0;

            // Calculate all the calibration entries in between that use the width of the cells
            for (byte col = 1; col < NUMCOLS; ++col) {
              Device.calRows[col][row].fxdMeasuredX = FXD_FROM_INT(calSampleRows[col][row].minValue) + FXD_DIV(FXD_FROM_INT(calSampleRows[col][row].maxValue - calSampleRows[col][row].minValue), FXD_FROM_INT(2));
              Device.calRows[col][row].fxdRatio = FXD_DIV(CALX_FULL_UNIT, FXD_FROM_INT(calSampleRows[col][row].maxValue - calSampleRows[col][row].minValue));
            }

            // The last entry marks the rightmost measured X value
            Device.calRows[NUMCOLS][row].fxdMeasuredX = FXD_FROM_INT(calSampleRows[NUMCOLS-1][row].maxValue);
            Device.calRows[NUMCOLS][row].fxdRatio = 0;
          }

          // Store and calculate the calibration Y data based on the collected samples
          for (byte row = 0; row < NUMROWS; ++row) {
            for (byte col = 0; col < 9; ++col) {
              int sampledRange = calSampleCols[col][row].maxValue - calSampleCols[col][row].minValue;
              int cellMarginY = (sampledRange / CALY_MARGIN_FRACTION);
              Device.calCols[col][row].minY = calSampleCols[col][row].minValue + cellMarginY;
              Device.calCols[col][row].maxY = calSampleCols[col][row].maxValue - cellMarginY;
              Device.calCols[col][row].fxdRatio = FXD_DIV(FXD_FROM_INT(Device.calCols[col][row].maxY - Device.calCols[col][row].minY), CALY_FULL_UNIT);
            }
          }

          Device.calibrated = true;

          debugCalibration();

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
          bigfont_draw_string(6, 0, "OK", globalColor, false);
          delayUsec(500000);
          setDisplayMode(displayNormal);
          controlButton = -1;
          clearLed(0, GLOBAL_SETTINGS_ROW);
          updateDisplay();
        }
      }
    }
  }
}

void debugCalibration() {
  for (byte row = 0; row < 4; ++row) {
    for (byte col = 0; col <= NUMCOLS; ++col) {
      DEBUGPRINT((0,"calRows"));
      DEBUGPRINT((0," col="));DEBUGPRINT((0,(int)col));
      DEBUGPRINT((0," row="));DEBUGPRINT((0,(int)row));
      DEBUGPRINT((0," sampleMin="));DEBUGPRINT((0,(int)calSampleRows[col][row].minValue));
      DEBUGPRINT((0," sampleMax="));DEBUGPRINT((0,(int)calSampleRows[col][row].maxValue));
      DEBUGPRINT((0," referenceX="));DEBUGPRINT((0,(int)FXD_TO_INT(Device.calRows[col][row].fxdReferenceX)));
      DEBUGPRINT((0," measuredX="));DEBUGPRINT((0,(int)FXD_TO_INT(Device.calRows[col][row].fxdMeasuredX)));
      DEBUGPRINT((0," ratio="));DEBUGPRINT((0,(int)FXD_TO_INT(FXD_MUL(Device.calRows[col][row].fxdRatio, FXD_FROM_INT(100)))));
      DEBUGPRINT((0,"\n"));
    }
  }
  for (byte col = 0; col < 5; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      DEBUGPRINT((0,"calCols"));
      DEBUGPRINT((0," col="));DEBUGPRINT((0,(int)col));
      DEBUGPRINT((0," row="));DEBUGPRINT((0,(int)row));
      DEBUGPRINT((0," sampleMin="));DEBUGPRINT((0,(int)calSampleCols[col][row].minValue));
      DEBUGPRINT((0," sampleMax="));DEBUGPRINT((0,(int)calSampleCols[col][row].maxValue));
      DEBUGPRINT((0," minY="));DEBUGPRINT((0,(int)Device.calCols[col][row].minY));
      DEBUGPRINT((0," maxY="));DEBUGPRINT((0,(int)Device.calCols[col][row].maxY));
      DEBUGPRINT((0," ratio="));DEBUGPRINT((0,(int)FXD_TO_INT(FXD_MUL(Device.calCols[col][row].fxdRatio, FXD_FROM_INT(100)))));
      DEBUGPRINT((0,"\n"));
    }
  }
}
