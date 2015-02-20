/******************************** ls_sensor: LinnStrument Sensor **********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions handle the sensing of touches on the LinnStrument's touch surface.
**************************************************************************************************/

// These are the rectified pressure sensititivies for each column
// CAREFUL, contrary to all the other arrays these are rows first and columns second since it makes it much easier to visualize and edit the
// actual values in a spreadsheet
const short Z_BIAS_SEPTEMBER[NUMROWS][NUMCOLS] =  {
    {350, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1153, 1129, 1109, 1093, 1087, 1087, 1089, 1095, 1093, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1256},
    {350, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1101, 1086, 1070, 1062, 1054, 1050, 1050, 1054, 1062, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1256},
    {350, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1067, 1051, 1039, 1031, 1019, 1016, 1018, 1023, 1029, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1193},
    {350, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1048, 1036, 1024, 1016, 1006, 1000, 1000, 1006, 1012, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1150},
    {350, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1048, 1036, 1024, 1016, 1006, 1000, 1000, 1006, 1012, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1150},
    {350, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1067, 1051, 1039, 1031, 1019, 1016, 1018, 1023, 1029, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1193},
    {350, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1101, 1086, 1070, 1062, 1054, 1050, 1050, 1054, 1062, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1256},
    {350, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1153, 1129, 1109, 1093, 1087, 1087, 1089, 1095, 1093, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1256}
  };
const short Z_BIAS_MULTIPLIER_SEPTEMBER = 1400;

const short Z_BIAS_NOVEMBER[NUMROWS][NUMCOLS] =  {
    {516, 667, 686, 699, 704, 702, 692, 675, 654, 631, 609, 591, 578, 572, 575, 585, 601, 622, 645, 667, 686, 699, 704, 702, 692, 675},
    {516, 893, 888, 872, 846, 813, 777, 740, 707, 680, 663, 657, 663, 680, 706, 739, 776, 813, 846, 871, 888, 893, 887, 869, 842, 809},
    {516, 732, 744, 748, 744, 732, 714, 691, 665, 641, 619, 602, 592, 590, 596, 609, 629, 653, 678, 702, 723, 739, 747, 747, 739, 723},
    {516, 811, 804, 785, 759, 726, 691, 656, 626, 602, 588, 585, 594, 612, 639, 672, 707, 742, 772, 795, 808, 811, 803, 784, 757, 724},
    {516, 724, 738, 744, 739, 726, 704, 677, 647, 617, 591, 570, 557, 554, 560, 576, 599, 627, 657, 686, 712, 731, 742, 743, 734, 717},
    {516, 751, 751, 745, 732, 716, 696, 674, 653, 635, 621, 612, 609, 613, 623, 638, 657, 679, 700, 719, 735, 746, 751, 750, 742, 728},
    {516, 828, 849, 858, 853, 835, 807, 770, 731, 692, 659, 635, 624, 626, 641, 668, 703, 742, 782, 816, 842, 856, 857, 844, 820, 786},
    {516, 766, 775, 780, 780, 775, 765, 750, 733, 713, 692, 671, 652, 635, 622, 613, 610, 611, 617, 628, 643, 661, 682, 703, 723, 742}
  };
const short Z_BIAS_MULTIPLIER_NOVEMBER = 860;

// readX:
// Reads raw X value at the currently addressed column and row
inline short readX() {                                // returns the raw X value at the addressed cell
  DEBUGPRINT((3,"readX\n"));

  selectSensorCell(sensorCol, sensorRow, READ_X);     // set analog switches to this column and row, and to read X
  delayUsec(250);                                     // delay required after setting analog switches for stable X read. This needs further research
  short raw = spiAnalogRead();
  #ifdef APRIL_2014_PROTOTYPE
  raw = 4095 - raw;
  #endif

  return raw;                                        // return the raw X value
}

// readY:
// Reads Y value for current cell and returns a value of 0-127 within cell's y axis
inline short readY() {                                // returns a value of 0-127 within cell's y axis
  DEBUGPRINT((3,"readY\n"));

  selectSensorCell(sensorCol, sensorRow, READ_Y);     // set analog switches to this cell and to read Y
  delayUsec(35);                                      // delay required after setting analog switches for stable Y read. Requires further research
  short raw = spiAnalogRead();
  #ifdef APRIL_2014_PROTOTYPE
  raw = 4095 - raw;
  #endif

  return raw;
}

// readZ:
// Reads Z value at current cell
inline unsigned short readZ() {                              // returns the raw Z value
  selectSensorCell(sensorCol, sensorRow, READ_Z);            // set analog switches to current cell in touch sensor and read Z
  // prevent phantom reads when vertically adjacent cells are pressed
  if (sensorCol == 0) {
    delayUsec(24);
  }
  else {
    delayMicroseconds(11);
  }

  short rawZ = 4095 - spiAnalogRead();                       // read raw Z value and invert it from (4095 - 0) to (0-4095)

  // apply the bias for each column, we also raise the baseline values to make the highest points just as sensitive and the lowest ones more sensitive
  rawZ = (rawZ * Z_BIAS_MULTIPLIER_SEPTEMBER) / Z_BIAS_SEPTEMBER[sensorRow][sensorCol];

  // this bias is totally experimental and not considered finished, it's merely an experiment to see
  // if different values can make the november sensor better, at the moment it's not!
  // rawZ = (rawZ * Z_BIAS_MULTIPLIER_NOVEMBER) / Z_BIAS_NOVEMBER[sensorRow][sensorCol];

  return rawZ;
}

// spiAnalogRead:
// returns raw ADC output at current cell
inline short spiAnalogRead() {
  byte msb = SPI.transfer(SPI_ADC, 0, SPI_CONTINUE);         // read byte MSB
  byte lsb = SPI.transfer(SPI_ADC, 0);                       // read byte LSB

  short raw = short(msb) << 8;             // assemble the 2 transfered bytes into an int
  raw |= lsb;
  return raw >> 2;                         // shift the 14-bit value from bits 16-2 to bits 14-0
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



inline void selectSensorCell(byte col,             // column to be addressed by analog switches
                             byte row,             // row to be addressed by analog switches
                             byte switchCode) {    // set analog switches to read X (0), Y (1) or Z (2)
  #ifdef APRIL_2014_PROTOTYPE
  col = (NUMCOLS-1) - col;                         // debug: for Apr 2014 prototypes, this corrects for reverse column layout
  row = (NUMROWS-1) - row;                         // debug: for Apr 2014 prototypes, this corrects for reverse row layout
  #endif

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
