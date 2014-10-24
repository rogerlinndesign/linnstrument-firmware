/******************************** ls_sensor: LinnStrument Sensor **********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions handle the sensing of touches on the LinnStrument's touch surface.
**************************************************************************************************/

/********************** the curve was generated with the following C program **********************
#include <stdio.h>
#include <math.h>
int main(int argc, const char* argv[]) {
  for (int z = 0; z <= 508; ++z) {
    printf("%d, ", (int)(pow(z/508., 0.6) * 508.));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE[509] =
  {
    0, 12, 18, 23, 27, 31, 35, 38, 42, 45, 48, 50, 53, 56, 58, 61, 63, 66, 68, 70, 72, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 94, 96, 98, 100, 102, 103, 105,
    107, 108, 110, 112, 113, 115, 117, 118, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135, 136, 138, 139, 141, 142, 143, 145, 146, 147, 149, 150, 152,
    153, 154, 155, 157, 158, 159, 161, 162, 163, 165, 166, 167, 168, 170, 171, 172, 173, 175, 176, 177, 178, 179, 181, 182, 183, 184, 185, 186, 188, 189, 190,
    191, 192, 193, 195, 196, 197, 198, 199, 200, 201, 202, 203, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 253, 254,
    255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 278, 279, 280, 281, 282, 283,
    284, 285, 286, 286, 287, 288, 289, 290, 291, 292, 292, 293, 294, 295, 296, 297, 298, 299, 299, 300, 301, 302, 303, 304, 304, 305, 306, 307, 308, 309, 309,
    310, 311, 312, 313, 314, 314, 315, 316, 317, 318, 319, 319, 320, 321, 322, 323, 323, 324, 325, 326, 327, 327, 328, 329, 330, 331, 331, 332, 333, 334, 335,
    335, 336, 337, 338, 339, 339, 340, 341, 342, 343, 343, 344, 345, 346, 346, 347, 348, 349, 349, 350, 351, 352, 353, 353, 354, 355, 356, 356, 357, 358, 359,
    359, 360, 361, 362, 362, 363, 364, 365, 365, 366, 367, 368, 368, 369, 370, 371, 371, 372, 373, 374, 374, 375, 376, 376, 377, 378, 379, 379, 380, 381, 382,
    382, 383, 384, 384, 385, 386, 387, 387, 388, 389, 390, 390, 391, 392, 392, 393, 394, 394, 395, 396, 397, 397, 398, 399, 399, 400, 401, 402, 402, 403, 404,
    404, 405, 406, 406, 407, 408, 409, 409, 410, 411, 411, 412, 413, 413, 414, 415, 415, 416, 417, 417, 418, 419, 420, 420, 421, 422, 422, 423, 424, 424, 425,
    426, 426, 427, 428, 428, 429, 430, 430, 431, 432, 432, 433, 434, 434, 435, 436, 436, 437, 438, 438, 439, 440, 440, 441, 442, 442, 443, 444, 444, 445, 446,
    446, 447, 448, 448, 449, 449, 450, 451, 451, 452, 453, 453, 454, 455, 455, 456, 457, 457, 458, 459, 459, 460, 460, 461, 462, 462, 463, 464, 464, 465, 466,
    466, 467, 467, 468, 469, 469, 470, 471, 471, 472, 472, 473, 474, 474, 475, 476, 476, 477, 478, 478, 479, 479, 480, 481, 481, 482, 482, 483, 484, 484, 485,
    486, 486, 487, 487, 488, 489, 489, 490, 491, 491, 492, 492, 493, 494, 494, 495, 495, 496, 497, 497, 498, 498, 499, 500, 500, 501, 501, 502, 503, 503, 504,
    504, 505, 506, 506, 507, 508
  };

// These are the rectified pressure sensititivies for each column
// CAREFUL, contrary to all the other arrays these are rows first and columns second since it makes it much easier to visualize and edit the
// actual values in a spreadsheet
const int Z_BIAS[NUMROWS][NUMCOLS] = 
  {
    {850, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1153, 1129, 1109, 1093, 1087, 1087, 1089, 1095, 1093, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1506},
    {850, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1101, 1086, 1070, 1062, 1054, 1050, 1050, 1054, 1062, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1506},
    {850, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1067, 1051, 1039, 1031, 1019, 1016, 1018, 1023, 1029, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1443},
    {850, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1048, 1036, 1024, 1016, 1006, 1000, 1000, 1006, 1012, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1400},
    {850, 1400, 1320, 1260, 1200, 1152, 1120, 1096, 1072, 1048, 1036, 1024, 1016, 1006, 1000, 1000, 1006, 1012, 1020, 1032, 1056, 1088, 1150, 1216, 1293, 1400},
    {850, 1443, 1359, 1295, 1227, 1175, 1143, 1119, 1095, 1067, 1051, 1039, 1031, 1019, 1016, 1018, 1023, 1029, 1039, 1051, 1079, 1111, 1171, 1243, 1331, 1443},
    {850, 1506, 1418, 1350, 1282, 1222, 1178, 1150, 1126, 1101, 1086, 1070, 1062, 1054, 1050, 1050, 1054, 1062, 1074, 1086, 1114, 1150, 1214, 1290, 1386, 1506},
    {850, 1506, 1497, 1417, 1357, 1297, 1241, 1205, 1177, 1153, 1129, 1109, 1093, 1087, 1087, 1089, 1095, 1093, 1109, 1121, 1157, 1209, 1277, 1361, 1441, 1506}
  };

// readX:
// Reads raw X value at the currently addressed column and row
inline int readX()                                    // returns the raw X value at the addressed cell
{
  DEBUGPRINT((3,"readX\n"));

  selectSensorCell(sensorCol, sensorRow, READ_X);     // set analog switches to this column and row, and to read X
  delayUsec(250);                                     // delay required after setting analog switches for stable X read. This needs further research
  int raw = spiAnalogRead();
  #ifdef APRIL_2014_PROTOTYPE
  raw = 4095 - raw;
  #endif

  return raw;                                        // return the raw X value
}

// readY:
// Reads Y value for current cell and returns a value of 0-127 within cell's y axis
inline int readY()                                    // returns a value of 0-127 within cell's y axis
{
  DEBUGPRINT((3,"readY\n"));

  selectSensorCell(sensorCol, sensorRow, READ_Y);     // set analog switches to this cell and to read Y
  delayUsec(25);                                      // delay required after setting analog switches for stable Y read. Requires further research
  int raw = spiAnalogRead();
  #ifdef APRIL_2014_PROTOTYPE
  raw = 4095 - raw;
  #endif

  return raw;
}

// readZ:
// Reads Z value at current cell, return either 0 for non-touch (no pressure at),
// 255 for feather-touch (below touch threshold, but existing pressure),
// or 1 to 127
inline byte readZ()                                          // returns either 0 for non-touch (below touch threshold) or 1 to 127
{
  selectSensorCell(sensorCol, sensorRow, READ_Z);            // set analog switches to current cell in touch sensor and read Z
  delayUsec(7);                                              // prevent phantom reads when vertically adjacent cells are pressed, should be removed when the the ADC's pullup resistor is changed

  int rawZ = 4095 - spiAnalogRead();                         // read raw Z value and invert it from (4095 - 0) to (0-4095)
  rawZ = (rawZ * 1400) / Z_BIAS[sensorRow][sensorCol];       // apply the bias for each column, we also raise the baseline values to make the highest points just as sensitive and the lowest ones more sensitive

  cell().rawZ = rawZ;                                        // store the raw Z data for later comparisons and calculations
  if (rawZ < SENSOR_FEATHER_Z) return Z_VAL_NONE;            // if the raw touch is below feather touch, then return 0 as code for not touched

  int usableZ = rawZ - SENSOR_LO_Z;                          // subtract minimum from value

  if (usableZ <= 0) return Z_VAL_FEATHER;                    // if it's below the acceptable minimum, return the feather touch code value
  if (usableZ > SENSOR_RANGE_Z) usableZ = SENSOR_RANGE_Z;    // clip high end to highest acceptable raw value

  usableZ = Z_CURVE[usableZ];                                // apply the sensitivity curve

  // scale the result and return it as a byte in the range 1-127
  return byte(FXD_TO_INT(FXD_DIV(FXD_FROM_INT(usableZ), FXD_FROM_INT(4))) & B01111111);
}

// spiAnalogRead:
// returns raw ADC output at current cell
inline int spiAnalogRead() {

  byte msb = SPI.transfer(SPI_ADC, 0, SPI_CONTINUE);         // read byte MSB
  byte lsb = SPI.transfer(SPI_ADC, 0);                       // read byte LSB

  int raw = int(msb) << 8;                 // assemble the 2 transfered bytes into an int
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
                             byte switchCode)      // set analog switches to read X (0), Y (1) or Z (2)
{
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
