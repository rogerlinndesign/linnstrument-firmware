/******************************** ls_sensor: LinnStrument Sensor **********************************
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
These functions handle the sensing of touches on the LinnStrument's touch surface.
**************************************************************************************************/

// These are the rectified pressure sensititivies for each column
// CAREFUL, contrary to all the other arrays these are rows first and columns second since it makes it much easier to visualize and edit the
// actual values in a spreadsheet
short Z_BIAS[MAXROWS][MAXCOLS];
const short Z_BIAS_200_SEPTEMBER2014[MAXROWS][MAXCOLS] =  {
    {350, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1153, 1129, 1109, 1093, 1087, 1087, 1089, 1095, 1093, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1256},
    {350, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1101, 1086, 1070, 1062, 1054, 1050, 1050, 1054, 1062, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1256},
    {350, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1067, 1051, 1039, 1031, 1019, 1016, 1018, 1023, 1029, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1193},
    {350, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1048, 1036, 1024, 1016, 1006, 1000, 1000, 1006, 1012, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1150},
    {350, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1048, 1036, 1024, 1016, 1006, 1000, 1000, 1006, 1012, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1150},
    {350, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1067, 1051, 1039, 1031, 1019, 1016, 1018, 1023, 1029, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1193},
    {350, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1101, 1086, 1070, 1062, 1054, 1050, 1050, 1054, 1062, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1256},
    {350, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1153, 1129, 1109, 1093, 1087, 1087, 1089, 1095, 1093, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1256}
  };
const short Z_BIAS_128_SEPTEMBER2016[MAXROWS][MAXCOLS] =  {
    {500, 2560, 2320, 2150, 2020, 1920, 1840, 1780, 1720, 1700, 1730, 1790, 1860, 1940, 2020, 2100, 2160, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {500, 2220, 2040, 1900, 1780, 1680, 1600, 1560, 1520, 1500, 1530, 1570, 1640, 1720, 1800, 1900, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {500, 2200, 1980, 1860, 1720, 1600, 1510, 1470, 1440, 1440, 1460, 1470, 1500, 1580, 1680, 1780, 1900, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {500, 2100, 1960, 1820, 1700, 1580, 1500, 1440, 1420, 1400, 1440, 1500, 1560, 1640, 1740, 1860, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {500, 1920, 1840, 1740, 1660, 1600, 1540, 1520, 1490, 1480, 1500, 1560, 1660, 1760, 1840, 1960, 2040, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {500, 2080, 1920, 1800, 1720, 1640, 1580, 1524, 1500, 1480, 1520, 1580, 1660, 1760, 1860, 1960, 2080, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {500, 2240, 2080, 1940, 1800, 1720, 1640, 1580, 1540, 1540, 1560, 1600, 1660, 1760, 1880, 2000, 2140, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {500, 2320, 2120, 1980, 1900, 1820, 1740, 1680, 1650, 1660, 1700, 1760, 1820, 1880, 1960, 2060, 2200, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };
// Make LS128 feel more like LS200, here is the LS200 bias array but with the center 9 columns removed: 
//                                      delete from here ^                                             to here ^
const short Z_BIAS_128_SEPTEMBER2019[MAXROWS][MAXCOLS] =  {   
    {350, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1256, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {350, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1256, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {350, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1193, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {350, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1150, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {350, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1150, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {350, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1193, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {350, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1256, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {350, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1256, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };


const short Z_BIAS_MULTIPLIER = 1400;

// readX:
// Reads raw X value at the currently addressed column and row
const short READX_FLATZONE = 25;
const short READX_RANGE = 25;
const short READX_MAX_DELAY = 250;
const short READX_MIN_DELAY = 150;
const short READX_RANGE_DELAY = READX_MAX_DELAY - READX_MIN_DELAY;

void initializeSensors() {
  if (LINNMODEL == 200) {
    for (byte r = 0; r < MAXROWS; ++r) {
      for (byte c = 0; c < MAXCOLS; ++c) {
        Z_BIAS[r][c] = Z_BIAS_200_SEPTEMBER2014[r][c];
      }
    }
  }
  else if (LINNMODEL == 128) {
    for (byte r = 0; r < MAXROWS; ++r) {
      for (byte c = 0; c < MAXCOLS; ++c) {
        Z_BIAS[r][c] = Z_BIAS_128_SEPTEMBER2019[r][c];
      }
    }
  }

  Device.sensorSensitivityZ = DEFAULT_SENSOR_SENSITIVITY_Z;
  Device.sensorLoZ = DEFAULT_SENSOR_LO_Z;
  Device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  Device.sensorRangeZ = DEFAULT_SENSOR_RANGE_Z;
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

inline short applyRawZBias(short rawZ) {
  // apply the bias for each column, we also raise the baseline values to make the highest points just as sensitive and the lowest ones more sensitive
  return rawZ = (rawZ * Z_BIAS_MULTIPLIER) / Z_BIAS[sensorRow][sensorCol];
}

inline unsigned short readZ() {                       // returns the raw Z value
#ifdef TESTING_SENSOR_DISABLE
    if (sensorCell->disabled) {
      return 0;
    }
#endif

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
