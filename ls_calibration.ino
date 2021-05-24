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

byte CALROWNUM = 4;
byte CALCOLNUM = 9;

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

int32_t calculateReferenceX(byte col) {
  if (col == 0) {
    return FXD_MUL(FXD_FROM_INT(-1), FXD_CALX_HALF_UNIT) + FXD_CALX_BORDER_OFFSET;;
  }
  else if (col < NUMCOLS) {
    return FXD_MUL(FXD_CALX_FULL_UNIT, FXD_FROM_INT(col - 1)); // center in the middle of the cells
  }
  else {
    return FXD_MUL(FXD_CALX_FULL_UNIT, FXD_FROM_INT(NUMCOLS - 1)) - FXD_CALX_HALF_UNIT - FXD_CALX_BORDER_OFFSET;
  }
}

int32_t calculateDefaultMeasuredX(byte col) {
  if (col == 0) {
    return FXD_CALX_DEFAULT_LEFT_EDGE;
  }
  else if (col == NUMCOLS) {
    return FXD_CALX_DEFAULT_RIGHT_EDGE;
  }
  else {
    return FXD_CALX_DEFAULT_FIRST_CELL + FXD_MUL(FXD_CALX_DEFAULT_CELL_WIDTH, FXD_FROM_INT(col - 1));
  }
}

void initializeCalibrationData() {
  Device.calCrc = 0;
  Device.calCrcCalculated = false;
  Device.calibrated = false;
  Device.calibrationHealed = false;

  // Initialize default X calibration data
  for (byte row = 0; row < CALROWNUM; ++row) {
    Device.calRows[0][row].fxdReferenceX = calculateReferenceX(0);
    Device.calRows[0][row].fxdMeasuredX = calculateDefaultMeasuredX(0);
    Device.calRows[0][row].fxdRatio = 0;

    for (byte col = 1; col < NUMCOLS; ++col) {
      Device.calRows[col][row].fxdReferenceX = calculateReferenceX(col);
      Device.calRows[col][row].fxdMeasuredX = calculateDefaultMeasuredX(col);
      Device.calRows[col][row].fxdRatio = FXD_DIV(FXD_CALX_FULL_UNIT, FXD_CALX_DEFAULT_CELL_WIDTH);
    }

    Device.calRows[NUMCOLS][row].fxdReferenceX = calculateReferenceX(NUMCOLS);
    Device.calRows[NUMCOLS][row].fxdMeasuredX = calculateDefaultMeasuredX(NUMCOLS);
    Device.calRows[NUMCOLS][row].fxdRatio = 0;
  }

  // Initialize default Y calibration data
  for (byte col = 0; col < CALCOLNUM; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      Device.calCols[col][row].minY = CALY_DEFAULT_MIN[row];
      Device.calCols[col][row].maxY = CALY_DEFAULT_MAX[row];
      Device.calCols[col][row].fxdRatio = FXD_DIV(FXD_FROM_INT(Device.calCols[col][row].maxY - Device.calCols[col][row].minY), FXD_CALY_FULL_UNIT);
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
  int result = FXD_TO_INT(fxdBottomX + FXD_MUL(FXD_DIV(fxdTopX - fxdBottomX, FXD_FROM_INT(topRow - bottomRow)), FXD_FROM_INT(sensorRow - bottomRow)));

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
  int result = FXD_TO_INT(FXD_MUL(fxdLeftY, FXD_DIV(FXD_CONST_3 - FXD_FROM_INT(bias), FXD_CONST_3)) +
                          FXD_MUL(fxdRightY, FXD_DIV(FXD_FROM_INT(bias), FXD_CONST_3)));

  // Bound the Y position to accepted value limits 
  result = constrain(result, 0, 127);

  return result;
}

boolean handleCalibrationSample() {
  // calibrate the X value distribution by measuring the minimum and maximum for each cell
  if (displayMode == displayCalibration) {
    // only calibrate a deliberate touch that is at least half-way through the pressure sensitivity range
    if (sensorCell->isStableYTouch() && cellsTouched == 1) {
      short rawX = readX(0);
      short rawY = readY(0);
      if (calibrationPhase == calibrationRows && (sensorRow == 0 || sensorRow == 2 || sensorRow == 5 || sensorRow == 7)) {
        byte row = (sensorRow / 2);
        int32_t fxd_default_center = FXD_CALX_DEFAULT_FIRST_CELL + FXD_MUL(FXD_FROM_INT(sensorCol - 1), FXD_CALX_DEFAULT_CELL_WIDTH);
        int min_limit = FXD_TO_INT(fxd_default_center - FXD_MUL(FXD_CALX_DEFAULT_CELL_WIDTH, FXD_CONST_2));
        int max_limit = FXD_TO_INT(fxd_default_center + FXD_MUL(FXD_CALX_DEFAULT_CELL_WIDTH, FXD_CONST_2));
        if (rawX < min_limit || rawX > max_limit) return false;
        calSampleRows[sensorCol][row].minValue = min(rawX, calSampleRows[sensorCol][row].minValue);
        calSampleRows[sensorCol][row].maxValue = max(rawX, calSampleRows[sensorCol][row].maxValue);
      }
      else if (calibrationPhase == calibrationCols && sensorCol > 0 && (sensorCol % 3 == 1)) {
        byte col = (sensorCol - 1) / 3;
        calSampleCols[col][sensorRow].minValue = min(rawY, calSampleCols[col][sensorRow].minValue);
        calSampleCols[col][sensorRow].maxValue = max(rawY, calSampleCols[col][sensorRow].maxValue);
      }
    }

    return true;
  }

  return false;
}

uint32_t calculateCalibrationCRC() {
  uint32_t crc = ~0L;

  for (uint8_t c = 0; c < MAXCOLS+1; ++c) {
    for (uint8_t r = 0; r < 4; ++r) {
      uint8_t* bytes = (uint8_t*)&Device.calRows[c][r];
      for (uint8_t i = 0; i < sizeof(CalibrationX); ++i) {
        crc = crc_update(crc, *bytes++);
      }
    }
  }

  for (uint8_t c = 0; c < 9; ++c) {
    for (uint8_t r = 0; r < MAXROWS; ++r) {
      uint8_t* bytes = (uint8_t*)&Device.calCols[c][r];
      for (uint8_t i = 0; i < sizeof(CalibrationY); ++i) {
        crc = crc_update(crc, *bytes++);
      }
    }
  }

  crc = ~crc;

  return crc;
}

boolean isValidCalibrationRatioX(byte col, byte row) {
  int ratio = FXD_TO_INT(FXD_MUL(Device.calRows[col][row].fxdRatio, FXD_CONST_100));
  return ratio >= 0 && ratio <= 200;
}

boolean isValidCalibrationMeasuredX(byte col, byte row) {
  int measured_x = FXD_TO_INT(Device.calRows[col][row].fxdMeasuredX);
  if (measured_x < 0x000 || measured_x > 0xfff) {
    return false;
  }
  int32_t default_measured_x = calculateDefaultMeasuredX(col);
  return FXD_TO_INT(Device.calRows[col][row].fxdMeasuredX) > FXD_TO_INT(default_measured_x - FXD_MUL(FXD_CALX_DEFAULT_CELL_WIDTH, FXD_CONST_2)) &&
         FXD_TO_INT(Device.calRows[col][row].fxdMeasuredX) < FXD_TO_INT(default_measured_x + FXD_MUL(FXD_CALX_DEFAULT_CELL_WIDTH, FXD_CONST_2));
}

boolean isValidCalibrationRatioY(byte col, byte row) {
  int ratio = FXD_TO_INT(FXD_MUL(Device.calCols[col][row].fxdRatio, FXD_CONST_100));
  return ratio >= 0 && ratio <= 200;
}

boolean validateAndHealCalibrationData() {
  for (uint8_t r = 0; r < CALROWNUM; ++r) {
    for (uint8_t c = 0; c <= NUMCOLS; ++c) {
      // ensure the correct reference X data for this column
      int32_t reference_x =  calculateReferenceX(c);
      if (Device.calRows[c][r].fxdReferenceX != reference_x) {
        Device.calRows[c][r].fxdReferenceX = reference_x;
        Device.calibrationHealed = true;
      }
    }

    int32_t previous_measured_x = FXD_FROM_INT(-500);
    for (uint8_t c = 0; c <= NUMCOLS; ++c) {
      if (FXD_TO_INT(Device.calRows[c][r].fxdMeasuredX) <= FXD_TO_INT(previous_measured_x) || !isValidCalibrationMeasuredX(c, r)) {
        // try to heal the measured X data for this column
        if (c > 1 && c < NUMCOLS-1 && isValidCalibrationMeasuredX(c+1, r)) {
          Device.calRows[c][r].fxdMeasuredX = FXD_DIV(Device.calRows[c-1][r].fxdMeasuredX + Device.calRows[c+1][r].fxdMeasuredX, FXD_CONST_2);
          Device.calibrationHealed = true;
        }
        else if (c > 0 && c < NUMCOLS && r > 0 && isValidCalibrationMeasuredX(c, r-1)) {
          Device.calRows[c][r].fxdMeasuredX = Device.calRows[c][r-1].fxdMeasuredX;
          Device.calibrationHealed = true;
        }
        else if (c > 0 && c < NUMCOLS && r < CALROWNUM - 1 && isValidCalibrationMeasuredX(c, r+1)) {
          Device.calRows[c][r].fxdMeasuredX = Device.calRows[c][r+1].fxdMeasuredX;
          Device.calibrationHealed = true;
        }
        else {
          return false;
        }
      }

      if (!isValidCalibrationRatioX(c, r)) {
        // try to heal the X ratio data for this column
        if (c > 1 && c < NUMCOLS-1 && isValidCalibrationRatioX(c+1, r)) {
          Device.calRows[c][r].fxdRatio = FXD_DIV(Device.calRows[c-1][r].fxdRatio + Device.calRows[c+1][r].fxdRatio, FXD_CONST_2);
          Device.calibrationHealed = true;
        }
        else if (c > 0 && c < NUMCOLS && r > 0 && isValidCalibrationRatioX(c, r-1)) {
          Device.calRows[c][r].fxdRatio = Device.calRows[c][r-1].fxdRatio;
          Device.calibrationHealed = true;
        }
        else if (c > 0 && c < NUMCOLS && r < CALROWNUM-1 && isValidCalibrationRatioX(c, r+1)) {
          Device.calRows[c][r].fxdRatio = Device.calRows[c][r+1].fxdRatio;
          Device.calibrationHealed = true;
        }
        else {
          return false;
        }
      }

      previous_measured_x = Device.calRows[c][r].fxdMeasuredX;
    }
  }

  // first find a valid Y column after the first one that can be used
  // to repair the first column in case it is invalid
  int valid_column_y = -1;
  for (uint8_t c = 1; c < CALCOLNUM; ++c) {
    unsigned short previous_max_y = 0;
    uint8_t r;
    for (r = 0; r < NUMROWS; ++r) {
      if (Device.calCols[c][r].minY <= previous_max_y ||
          Device.calCols[c][r].maxY <= Device.calCols[c][r].minY ||
          !isValidCalibrationRatioY(c, r)) {
        break;
      }
      previous_max_y = Device.calCols[c][r].maxY;
    }

    if (r == NUMROWS) {
      valid_column_y = c;
      break;
    }
  }

  for (uint8_t c = 0; c < CALCOLNUM; ++c) {
    unsigned short previous_max_y = 0;
    for (uint8_t r = 0; r < NUMROWS; ++r) {
      if (Device.calCols[c][r].minY <= previous_max_y) {
        // try to heal min Y for this row
        if (c > 0 && c < CALCOLNUM-1) {
          Device.calCols[c][r].minY = (int(Device.calCols[c-1][r].minY) + int(Device.calCols[c+1][r].minY)) / 2;
          Device.calibrationHealed = true;
        }
        else if (c > 0) {
          Device.calCols[c][r].minY = Device.calCols[c-1][r].minY;
          Device.calibrationHealed = true;
        }
        else if (c == 0 && valid_column_y != -1) {
          Device.calCols[c][r].minY = Device.calCols[valid_column_y][r].minY;
          Device.calibrationHealed = true;
        }
        else {
          return false;
        }
      }

      if (Device.calCols[c][r].maxY <= Device.calCols[c][r].minY) {
        // try to heal max Y for this row
        if (c > 0 && c < CALCOLNUM-1) {
          Device.calCols[c][r].maxY = (int(Device.calCols[c-1][r].maxY) + int(Device.calCols[c+1][r].maxY)) / 2;
          Device.calibrationHealed = true;
        }
        else if (c > 0) {
          Device.calCols[c][r].maxY = Device.calCols[c-1][r].maxY;
          Device.calibrationHealed = true;
        }
        else if (c == 0 && valid_column_y != -1) {
          Device.calCols[c][r].maxY = Device.calCols[valid_column_y][r].maxY;
          Device.calibrationHealed = true;
        }
        else {
          return false;
        }
      }

      if (!isValidCalibrationRatioY(c, r)) {
        // try to heal the Y ratio data for this column
        if (c > 0 && c < CALCOLNUM-1 && isValidCalibrationRatioY(c+1, r)) {
          Device.calCols[c][r].fxdRatio = FXD_DIV(Device.calCols[c-1][r].fxdRatio + Device.calCols[c+1][r].fxdRatio, FXD_CONST_2);
          Device.calibrationHealed = true;
        }
        else if (c > 0) {
          Device.calCols[c][r].fxdRatio = Device.calCols[c-1][r].fxdRatio;
          Device.calibrationHealed = true;
        }
        else if (c == 0 && valid_column_y != -1) {
          Device.calCols[c][r].fxdRatio = Device.calCols[valid_column_y][r].fxdRatio;
          Device.calibrationHealed = true;
        }
        else {
          return false;
        }
      }
      previous_max_y = Device.calCols[c][r].maxY;
    }
  }

  return true;
}

boolean handleCalibrationRelease() {
  // Handle calibration passes, at least two before indicating green
  if (displayMode == displayCalibration) {
    int cellPass = -1;
    byte cellColor = COLOR_OFF;

    if (calibrationPhase == calibrationRows && (sensorRow == 0 || sensorRow == 2 || sensorRow == 5 || sensorRow == 7)) {
      byte i1 = sensorCol;
      byte i2 = (sensorRow / 2);

#ifdef DEBUG_ENABLED
      DEBUGPRINT((0,"calRows"));
      DEBUGPRINT((0," col="));DEBUGPRINT((0,(int)sensorCol));
      DEBUGPRINT((0," row="));DEBUGPRINT((0,(int)sensorRow));
      DEBUGPRINT((0," sampleMin="));DEBUGPRINT((0,(int)calSampleRows[i1][i2].minValue));
      DEBUGPRINT((0," sampleMax="));DEBUGPRINT((0,(int)calSampleRows[i1][i2].maxValue));
      DEBUGPRINT((0," diff="));DEBUGPRINT((0,(int)calSampleRows[i1][i2].maxValue - calSampleRows[i1][i2].minValue));
      DEBUGPRINT((0,"\n"));
#endif

      // Only proceed when at least a delta of 20 in X values is measured
      if (i2 < 4) {
        int delta = calSampleRows[i1][i2].maxValue - calSampleRows[i1][i2].minValue;
        if (delta >= 20) {
          cellPass = calSampleRows[i1][i2].pass;

          // Adapt the color if the the delta is below expected values
          if (delta <= 40) {
            cellColor = COLOR_RED;
          }
          else {
            // Only advance the pass when at least a delta of 40 in X values is measured
            calSampleRows[i1][i2].pass += 1;

            if (delta <= 65) {
              cellColor = COLOR_YELLOW;
            }
            // This is the first pass for a sensor, switch the led to cyan
            else if (cellPass == 0) {
              cellColor = COLOR_CYAN;
            }
            // This is the second pass for a sensor, switch the led to green
            else if (cellPass > 0) {
              cellColor = COLOR_GREEN;
            }
          }
        }
      }
    }
    else if (calibrationPhase == calibrationCols && sensorCol > 0 && (sensorCol % 3 == 1)) {
      byte i1 = (sensorCol - 1) / 3;
      byte i2 = sensorRow;

      // Only proceed when at least a delta of 60 in Y values is measured
      if (i1 < 9) {
        int delta = calSampleCols[i1][i2].maxValue - calSampleCols[i1][i2].minValue;
        if (delta >= 60) {
          cellPass = calSampleCols[i1][i2].pass;

          // Adapt the color if the the delta is below expected values
          if (delta <= 110) {
            cellColor = COLOR_RED;
          }
          else {
            // Only advance the pass when at least a delta of 110 in Y values is measured
            calSampleCols[i1][i2].pass += 1;

            if (delta <= 180) {
              cellColor = COLOR_YELLOW;
            }
            // This is the first pass for a sensor, switch the led to cyan
            else if (cellPass == 0) {
              cellColor = COLOR_CYAN;
            }
            // This is the second pass for a sensor, switch the led to green
            else if (cellPass > 0) {
              cellColor = COLOR_GREEN;
            }
          }
        }
      }
    }

    // Only update the cell calibration LED when a change occurred
    if (cellColor != COLOR_OFF) {
      setLed(sensorCol, sensorRow, cellColor, cellOn);
    }

    // We need at least two passes to consider the calibration viable
    if (cellPass > 0) {
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
              Device.calRows[col][row].fxdRatio = FXD_DIV(FXD_CALX_FULL_UNIT, FXD_FROM_INT(calSampleRows[col][row].maxValue - calSampleRows[col][row].minValue));
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
              Device.calCols[col][row].minY = constrain(calSampleCols[col][row].minValue + cellMarginY, 0, 4095);
              Device.calCols[col][row].maxY = constrain(calSampleCols[col][row].maxValue - cellMarginY, 0, 4095);
              Device.calCols[col][row].fxdRatio = FXD_DIV(FXD_FROM_INT(Device.calCols[col][row].maxY - Device.calCols[col][row].minY), FXD_CALY_FULL_UNIT);
            }
          }

          Device.calCrc = calculateCalibrationCRC();
          Device.calCrcCalculated = true;
          Device.calibrated = true;
          Device.calibrationHealed = false;

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
