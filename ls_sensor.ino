/******************************** ls_sensor: LinnStrument Sensor **********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions handle the sensing of touches on the LinnStrument's touch surface.
**************************************************************************************************/

short R_COLS[MAXCOLS];
short R_ROWS[MAXROWS];
const short R_COLS_200[MAXCOLS] = {
    214, 270, 396, 511, 613, 704, 783, 851, 907, 951, 984, 1005, 1014, 1012, 998, 972, 935, 886, 826, 753, 670, 574, 467, 348, 218, 75
  };
const short R_ROWS_200[MAXROWS] = {
    65, 168, 237, 271, 271, 237, 168, 65
  };
const short R_COLS_128[MAXCOLS] = {
    207, 251, 359, 450, 523, 580, 619, 641, 647, 635, 606, 560, 496, 416, 318, 204, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
const short R_ROWS_128[MAXROWS] = {
    68, 177, 250, 286, 286, 250, 177, 68
  };
short ADC_MIN[MAXROWS][MAXCOLS] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };
short ADC_MAX[MAXROWS][MAXCOLS] =  {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };

// readX:
// Reads raw X value at the currently addressed column and row
const short READX_FLATZONE = 25;
const short READX_RANGE = 25;
const short READX_MAX_DELAY = 250;
const short READX_MIN_DELAY = 150;
const short READX_RANGE_DELAY = READX_MAX_DELAY - READX_MIN_DELAY;

void calculateADCValues() {
  if (LINNMODEL == 200) {
    for (byte r = 0; r < MAXROWS; ++r) {
      R_ROWS[r] = R_ROWS_200[r];
    }
    for (byte c = 0; c < MAXCOLS; ++c) {
      R_COLS[c] = R_COLS_200[c];
    }
  }
  else if (LINNMODEL == 128) {
    for (byte r = 0; r < MAXROWS; ++r) {
      R_ROWS[r] = R_ROWS_128[r];
    }
    for (byte c = 0; c < MAXCOLS; ++c) {
      R_COLS[c] = R_COLS_128[c];
    }
  }

  int pullup_r = 510;
  int fsr_min = 6000;
  int fsr_max = 600;
  for (byte r = 0; r < MAXROWS; ++r) {
    for (byte c = 0; c < MAXCOLS; ++c) {
      int fixed_r_min = R_ROWS[r] + fsr_min + R_COLS[c];
      ADC_MIN[r][c] = 4095 - ((4095 * fixed_r_min) / (fixed_r_min + pullup_r));
      int fixed_r_max = R_ROWS[r] + fsr_max + R_COLS[c];
      ADC_MAX[r][c] = 4095 - ((4095 * fixed_r_max) / (fixed_r_max + pullup_r));
    }
  }
}

void displayADCValues() {
  DEBUGPRINT((-1, "\nADC MIN:\n"));
  for (byte r = 0; r < NUMROWS; ++r) {
    for (byte c = 0; c < NUMCOLS; ++c) {
      DEBUGPRINT((-1, ADC_MIN[r][c]));
      DEBUGPRINT((-1, ", "));
    }
    DEBUGPRINT((-1, "\n"));
  }
  DEBUGPRINT((-1, "\nADC MAX:\n"));
  for (byte r = 0; r < NUMROWS; ++r) {
    for (byte c = 0; c < NUMCOLS; ++c) {
      DEBUGPRINT((-1, ADC_MAX[r][c]));
      DEBUGPRINT((-1, ", "));
    }
    DEBUGPRINT((-1, "\n"));
  }
  DEBUGPRINT((-1, "\n"));
}

void initializeSensors() {
  calculateADCValues();

  Device.sensorSensitivityZ = DEFAULT_SENSOR_SENSITIVITY_Z;
  Device.sensorLoZ = DEFAULT_SENSOR_LO_Z;
  Device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  Device.sensorRangeZ = DEFAULT_SENSOR_RANGE_Z;
}

inline short readX(byte zPct) {                       // returns the raw X value at the addressed cell
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

inline short applyRawZBias(short rawZ) {
  // apply the bias for each column, we also raise the baseline values to make the highest points just as sensitive and the lowest ones more sensitive
  if (sensorCol == 0) {
    rawZ = (rawZ * ADC_MAX[0][3]) / ADC_MIN[sensorRow][sensorCol];
  }
  else {
    rawZ = (rawZ * ADC_MAX[0][3]) / ADC_MAX[sensorRow][sensorCol];
  }

  return rawZ;
}

inline short readZ() {                                // returns the raw Z value
  DEBUGPRINT((3,"readZ\n"));

  selectSensorCell(sensorCol, sensorRow, READ_Z);     // set analog switches to current cell in touch sensor and read Z

  short rawZ;

  if (controlModeActive) {
    delayUsec(READZ_DELAY_CONTROLMODE);

    // read raw Z value and invert it from (4095 - 0) to (0-4095)
    rawZ = 4095 - spiAnalogRead();
  }
  else {
    // if there are active touches in the column, always use a settling time
    if (sensorCol == 0) {
      delayUsec(READZ_DELAY_SWITCH);
    }
    else if (rowsInColsTouched[sensorCol]) {
      delayUsec(READZ_DELAY_SENSOR);
    }

    // read raw Z value and invert it from (4095 - 0) to (0-4095)
    rawZ = 4095 - spiAnalogRead();

    // if there are no active touches in the column, but the raw pressure without settling time exceeds the value threshold,
    // introduce a settling time to read the proper stabilized value
    if (rowsInColsTouched[sensorCol] == 0 && rawZ > READZ_SETTLING_PRESSURE_THRESHOLD) {
        delayUsec(READZ_DELAY_SENSORINITIAL);
        rawZ = 4095 - spiAnalogRead();
    }
  }

  // store the last value that was read straight off of the sensor without any compensation
  lastReadSensorRawZ = rawZ;

  // scale the sensor based on the sensitivity setting
  rawZ = rawZ * Device.sensorSensitivityZ / 100;

  rawZ = applyRawZBias(rawZ);

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
