/******************************** ls_sensor: LinnStrument Sensor **********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions handle the sensing of touches on the LinnStrument's touch surface.
**************************************************************************************************/

// These will be filled in by two-dimensional interpolation from the device Z sensitivity setting
short Z_SENSITIVITY[MAXCOLS][MAXROWS];

// readX:
// Reads raw X value at the currently addressed column and row
const short READX_FLATZONE = 25;
const short READX_RANGE = 25;
const short READX_MAX_DELAY = 250;
const short READX_MIN_DELAY = 150;
const short READX_RANGE_DELAY = READX_MAX_DELAY - READX_MIN_DELAY;

void initializeSensors() {
 for (byte r = 0; r < MAXROWS; ++r) {
    Z_SENSITIVITY[0][r] = 300;
    for (byte c = 1; c < MAXCOLS; ++c) {
      Z_SENSITIVITY[c][r] = DEFAULT_SENSOR_SENSITIVITY_Z;
    }
  }
  for (int h = 0; h < 3; ++h) {
    for (int v = 0; v < 3; ++v) {
      Device.sensorSensitivityZ[h][v] = DEFAULT_SENSOR_SENSITIVITY_Z;
    }
  }
  Device.sensorLoZ = DEFAULT_SENSOR_LO_Z;
  Device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  Device.sensorRangeZ = DEFAULT_SENSOR_RANGE_Z;

  calculateInterpolatedZSensitivity();
}

void calculateInterpolatedZSensitivity() {
  int left_col = 1;
  int mid_col = NUMCOLS/2+1;
  int right_col = NUMCOLS-1;

  int btm_row = 0;
  int mid_row = NUMROWS/2;
  int top_row = NUMROWS-1;

  performContinuousTasks();
 
  // first interpolate in one dimension along the vertical calibration axis
  int r_range = mid_row - btm_row;
  for (int r = btm_row; r <= mid_row; ++r) {
    int btm_part = r_range - (r - btm_row);
    int mid_part = r - btm_row;
    Z_SENSITIVITY[left_col][r]  = (Device.sensorSensitivityZ[0][0] * btm_part + Device.sensorSensitivityZ[0][1] * mid_part + r_range / 2) / r_range;
    Z_SENSITIVITY[mid_col][r]   = (Device.sensorSensitivityZ[1][0] * btm_part + Device.sensorSensitivityZ[1][1] * mid_part + r_range / 2) / r_range;
    Z_SENSITIVITY[right_col][r] = (Device.sensorSensitivityZ[2][0] * btm_part + Device.sensorSensitivityZ[2][1] * mid_part + r_range / 2) / r_range;
  }
  performContinuousTasks();
  r_range = top_row - mid_row;
  for (int r = mid_row; r <= top_row; ++r) {
    int mid_part = r_range - (r - mid_row);
    int top_part = r - mid_row;
    Z_SENSITIVITY[left_col][r]  = (Device.sensorSensitivityZ[0][1] * mid_part + Device.sensorSensitivityZ[0][2] * top_part + r_range / 2) / r_range;
    Z_SENSITIVITY[mid_col][r]   = (Device.sensorSensitivityZ[1][1] * mid_part + Device.sensorSensitivityZ[1][2] * top_part + r_range / 2) / r_range;
    Z_SENSITIVITY[right_col][r] = (Device.sensorSensitivityZ[2][1] * mid_part + Device.sensorSensitivityZ[2][2] * top_part + r_range / 2) / r_range;
  }
  performContinuousTasks();

  int c_range = mid_col - left_col;
  for (int c = left_col; c <= mid_col; ++c) {
    int left_part = c_range - (c - left_col);
    int mid_part = c - left_col;
    for (int r = 0; r < NUMROWS; ++r) {
      Z_SENSITIVITY[c][r] = (Z_SENSITIVITY[left_col][r] * left_part + Z_SENSITIVITY[mid_col][r] * mid_part + c_range / 2) / c_range;
    }
  }
  performContinuousTasks();
  c_range = right_col - mid_col;
  for (int c = mid_col; c <= right_col; ++c) {
    int mid_part = c_range - (c - mid_col);
    int right_part = c - mid_col;
     for (int r = 0; r < NUMROWS; ++r) {
      Z_SENSITIVITY[c][r] = (Z_SENSITIVITY[mid_col][r] * mid_part + Z_SENSITIVITY[right_col][r] * right_part + c_range / 2) / c_range;
    }
  }
  performContinuousTasks();
}

void displaySensitivityValues() {
  for (byte r = 0; r < NUMROWS; ++r) {
    for (byte c = 1; c < NUMCOLS; ++c) {
      DEBUGPRINT((-1, Z_SENSITIVITY[c][r]));
      DEBUGPRINT((-1, ", "));
    }
    DEBUGPRINT((-1, "\n"));
  }
  DEBUGPRINT((-1, "\n"));
}

inline short readX(byte zPct) {                       // returns the raw X value at the addressed cell
#ifdef TESTING_SENSOR_DISABLE
    if (sensorCell->disabled) {
      return 0;
    }
#endif

  DEBUGPRINT((3,"readX\n"));

  selectSensorCell(sensorCol, sensorRow, READ_X);     // set analog switches to this column and row, and to read X

  short d;
  if (zPct <= READX_FLATZONE) {
    d = READX_MAX_DELAY;
  }
  else {
    d = READX_MAX_DELAY - (READX_RANGE_DELAY * min(zPct - READX_FLATZONE, READX_RANGE) / READX_RANGE);
  }

  delayUsec(d);                                       // delay required after setting analog switches for stable X read
  return spiAnalogRead();
}

// readY:
// Reads Y value for current cell and returns a value of 0-127 within cell's y axis
const short READY_FLATZONE = 30;
const short READY_RANGE = 40;
const short READY_MAX_DELAY = 200;
const short READY_MIN_DELAY = 60;
const short READY_RANGE_DELAY = READY_MAX_DELAY - READY_MIN_DELAY;

inline short readY(byte zPct) {                       // returns a value of 0-127 within cell's y axis
#ifdef TESTING_SENSOR_DISABLE
    if (sensorCell->disabled) {
      return 0;
    }
#endif

  DEBUGPRINT((3,"readY\n"));

  selectSensorCell(sensorCol, sensorRow, READ_Y);     // set analog switches to this cell and to read Y

  short d;
  if (zPct <= READY_FLATZONE) {
    d = READY_MAX_DELAY;
  }
  else {
    d = READY_MAX_DELAY - (READY_RANGE_DELAY * min(zPct - READY_FLATZONE, READY_RANGE) / READY_RANGE);
  }

  delayUsec(d);                                       // delay required after setting analog switches for stable Y read
  return spiAnalogRead();
}

// readZ:
// Reads Z value at current cell
const short READZ_DELAY_CONTROLMODE = 50;
const short READZ_DELAY_SWITCH = 24;
const short READZ_DELAY_SENSOR = 15;
const short READZ_DELAY_SENSORINITIAL = 14;
const short READZ_SETTLING_PRESSURE_THRESHOLD = 80;

inline short readAdcZ(byte col, byte row) {           // returns the ADC Z value for a particular column and row
  selectSensorCell(col, row, READ_Z);                 // set analog switches to current cell in touch sensor and read Z

  short rawZ;

  if (controlModeActive) {
    delayUsec(READZ_DELAY_CONTROLMODE);

    // read raw Z value and invert it from (4095 - 0) to (0-4095)
    rawZ = 4095 - spiAnalogRead();
  }
  else {
    // if there are active touches in the column, always use a settling time
    if (col == 0) {
      delayUsec(READZ_DELAY_SWITCH);
    }
    else if (rowsInColsTouched[col]) {
      delayUsec(READZ_DELAY_SENSOR);
    }

    // read raw Z value and invert it from (4095 - 0) to (0-4095)
    rawZ = 4095 - spiAnalogRead();

    // if there are no active touches in the column, but the raw pressure without settling time exceeds the value threshold,
    // introduce a settling time to read the proper stabilized value
    if (rowsInColsTouched[col] == 0 && rawZ > READZ_SETTLING_PRESSURE_THRESHOLD) {
        delayUsec(READZ_DELAY_SENSORINITIAL);
        rawZ = 4095 - spiAnalogRead();
    }
  }

  return rawZ;
}

inline unsigned short readZ() {                       // returns the raw Z value
#ifdef TESTING_SENSOR_DISABLE
    if (sensorCell->disabled) {
      return 0;
    }
#endif

  DEBUGPRINT((3,"readZ\n"));

  short rawZ = readAdcZ(sensorCol, sensorRow);
  rawZ = rawZ * Z_SENSITIVITY[sensorCol][sensorRow] / 100;
  return rawZ;
}

// spiAnalogRead:
// returns raw ADC output at current cell
inline short spiAnalogRead() {
  byte msb = SPI.transfer(SPI_ADC, 0, SPI_CONTINUE);         // read byte MSB
  byte lsb = SPI.transfer(SPI_ADC, 0);                       // read byte LSB

  // assemble the 2 transfered bytes into an int
  short raw = short(msb) << 8;
  raw |= lsb;
  // shift the 14-bit value from bits 16-2 to bits 14-0
  return (raw >> 2) & 0xFFF;
}


/****************************************************** ANALOG SWITCHES *********************************************/

/*
 selectSensorCell:
 Sends a 16-bit word over SPI to the touch sensor in order to set the analog switches to:
 1) select a column and row, and
 2) connect ends of rows and columns to various combination of 3.3 volts, ground and ADC (with or without pullup) in order to read X, Y or Z.

 Here are what each of the bits do:

 MS byte:
                                7                6                5                4                3                2                1                0
                             colBotSw        ColTopSw         colAdr4inv        colAdr4          colAdr3          colAdr2          colAdr1          colAdr0
                            0=gnd, 1=ADC   0=ADC, 1=+3.3v
 if switchCode = READ_X:        1                0            colAdr4inv        colAdr4          colAdr3          colAdr2          colAdr1          colAdr0
 if switchCode = READ_Y:        0                1            colAdr4inv        colAdr4          colAdr3          colAdr2          colAdr1          colAdr0
 if switchCode = READ_Z:        1                0            colAdr4inv        colAdr4          colAdr3          colAdr2          colAdr1          colAdr0



 LS byte:
                                7                6                5                4                3                2                1                0
                            not used        rowRightSwB       adcPullup        rowRightSwA      rowLeftSw         rowAdr2          rowAdr1          rowAdr0
                                            0=ADC,1=+3.3    1=pullup,0=not   0=gnd,1=RT_SW_B   0=gnd, 1=ADC
 if switchCode = READ_X:                         1                0                1                0             rowAdr2          rowAdr1          rowAdr0
 if switchCode = READ_Y:                         0                0                1                1             rowAdr2          rowAdr1          rowAdr0
 if switchCode = READ_Z:                        N/A               1                0                0             rowAdr2          rowAdr1          rowAdr0
 */


// col: column to be addressed by analog switches
// row: row to be addressed by analog switches
// switchCode: set analog switches to read X (0), Y (1) or Z (2)
inline void selectSensorCell(byte col, byte row, byte switchCode) {
  // first set lower 5 bits of MSB to specified column
  byte msb = col;                                 // set MSB of SPI value to column
  if ((col & 16) == 0) msb = col | B00100000;     // if column address 4 is 0, set bit 5 of MSB (inverted state of bit 4) to 1

  // then set lower 3 bits of LSB to specified row
  byte lsb = row;                                 // set LSB of SPI value to row

  // now, set bits 5-7 of MSB and bits 3-6 of LSB (routing analog swiches)
  switch (switchCode)                             // set SPI values differently depending on reading X, Y or Z
  {
  case READ_X:                                    // if reading X...
    msb |= B10000000;                             // set colBotSw to ADC
    lsb |= B01010000;                             // set rowRightSwA to RT_SW_B and rowRightSwB to +3.3 (for low-R Analog Devices switches)
    break;
  case READ_Y:                                    // if reading Y...
    msb |= B01000000;                             // set colTopSw to +3.3v
    lsb |= B00011000;                             // set rowRightSwA to RT_SW_B and rowRightSwB to ADC (for low-R Analog Devices switches)
    break;
  case READ_Z:                                    // if reading Z...
    msb |= B10000000;                             // set colBotSw to ADC
    lsb |= B00100000;                             // set rowRightSwA to GND and rowRightSwB doesn't matter (for low-R Analog Devices switches)
    break;
  default:
    break;
  }

  SPI.transfer(SPI_SENSOR, lsb, SPI_CONTINUE);    // to daisy-chained 595 (LSB)
  SPI.transfer(SPI_SENSOR, msb);                  // to first 595 at MOSI (MSB, for both sensor columns and LED columns)
}
