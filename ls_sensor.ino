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
  for (int z = 0; z <= 1016; ++z) {
    printf("%d, ", (int)(pow(z/1016., 0.6) * 1016.));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE[1017] = {
    0, 15, 24, 30, 36, 41, 46, 51, 55, 59, 63, 67, 70, 74, 77, 80, 84, 87, 90, 93, 96, 99, 101, 104, 107, 110, 112, 115, 117, 120, 122, 125, 127, 129, 132, 134,
    136, 139, 141, 143, 145, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 187, 189, 191, 193, 195, 197,
    198, 200, 202, 204, 205, 207, 209, 211, 212, 214, 216, 217, 219, 221, 222, 224, 226, 227, 229, 230, 232, 234, 235, 237, 238, 240, 242, 243, 245, 246, 248,
    249, 251, 252, 254, 255, 257, 258, 260, 261, 263, 264, 266, 267, 269, 270, 272, 273, 274, 276, 277, 279, 280, 282, 283, 284, 286, 287, 289, 290, 291, 293,
    294, 295, 297, 298, 299, 301, 302, 304, 305, 306, 308, 309, 310, 311, 313, 314, 315, 317, 318, 319, 321, 322, 323, 324, 326, 327, 328, 330, 331, 332, 333,
    335, 336, 337, 338, 340, 341, 342, 343, 345, 346, 347, 348, 350, 351, 352, 353, 354, 356, 357, 358, 359, 360, 362, 363, 364, 365, 366, 368, 369, 370, 371,
    372, 373, 375, 376, 377, 378, 379, 380, 382, 383, 384, 385, 386, 387, 388, 390, 391, 392, 393, 394, 395, 396, 397, 399, 400, 401, 402, 403, 404, 405, 406,
    407, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 438, 439, 440,
    441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471,
    472, 473, 474, 475, 476, 477, 478, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 496, 497, 498, 499, 500, 501, 502,
    503, 504, 505, 506, 507, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 524, 525, 526, 527, 528, 529, 530, 531,
    532, 533, 534, 535, 536, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 546, 547, 548, 549, 550, 551, 552, 553, 554, 555, 556, 556, 557, 558, 559,
    560, 561, 562, 563, 564, 564, 565, 566, 567, 568, 569, 570, 571, 572, 572, 573, 574, 575, 576, 577, 578, 579, 579, 580, 581, 582, 583, 584, 585, 585, 586,
    587, 588, 589, 590, 591, 592, 592, 593, 594, 595, 596, 597, 598, 598, 599, 600, 601, 602, 603, 603, 604, 605, 606, 607, 608, 609, 609, 610, 611, 612, 613,
    614, 614, 615, 616, 617, 618, 619, 619, 620, 621, 622, 623, 624, 624, 625, 626, 627, 628, 629, 629, 630, 631, 632, 633, 634, 634, 635, 636, 637, 638, 638,
    639, 640, 641, 642, 643, 643, 644, 645, 646, 647, 647, 648, 649, 650, 651, 651, 652, 653, 654, 655, 655, 656, 657, 658, 659, 659, 660, 661, 662, 663, 663,
    664, 665, 666, 667, 667, 668, 669, 670, 671, 671, 672, 673, 674, 675, 675, 676, 677, 678, 678, 679, 680, 681, 682, 682, 683, 684, 685, 686, 686, 687, 688,
    689, 689, 690, 691, 692, 693, 693, 694, 695, 696, 696, 697, 698, 699, 699, 700, 701, 702, 703, 703, 704, 705, 706, 706, 707, 708, 709, 709, 710, 711, 712,
    712, 713, 714, 715, 715, 716, 717, 718, 719, 719, 720, 721, 722, 722, 723, 724, 725, 725, 726, 727, 728, 728, 729, 730, 731, 731, 732, 733, 734, 734, 735,
    736, 737, 737, 738, 739, 739, 740, 741, 742, 742, 743, 744, 745, 745, 746, 747, 748, 748, 749, 750, 751, 751, 752, 753, 753, 754, 755, 756, 756, 757, 758,
    759, 759, 760, 761, 761, 762, 763, 764, 764, 765, 766, 767, 767, 768, 769, 769, 770, 771, 772, 772, 773, 774, 774, 775, 776, 777, 777, 778, 779, 780, 780,
    781, 782, 782, 783, 784, 785, 785, 786, 787, 787, 788, 789, 789, 790, 791, 792, 792, 793, 794, 794, 795, 796, 797, 797, 798, 799, 799, 800, 801, 801, 802,
    803, 804, 804, 805, 806, 806, 807, 808, 808, 809, 810, 811, 811, 812, 813, 813, 814, 815, 815, 816, 817, 818, 818, 819, 820, 820, 821, 822, 822, 823, 824,
    824, 825, 826, 827, 827, 828, 829, 829, 830, 831, 831, 832, 833, 833, 834, 835, 835, 836, 837, 837, 838, 839, 840, 840, 841, 842, 842, 843, 844, 844, 845,
    846, 846, 847, 848, 848, 849, 850, 850, 851, 852, 852, 853, 854, 854, 855, 856, 856, 857, 858, 858, 859, 860, 860, 861, 862, 862, 863, 864, 864, 865, 866,
    866, 867, 868, 868, 869, 870, 870, 871, 872, 872, 873, 874, 874, 875, 876, 876, 877, 878, 878, 879, 880, 880, 881, 882, 882, 883, 884, 884, 885, 886, 886,
    887, 888, 888, 889, 890, 890, 891, 892, 892, 893, 894, 894, 895, 896, 896, 897, 897, 898, 899, 899, 900, 901, 901, 902, 903, 903, 904, 905, 905, 906, 907,
    907, 908, 908, 909, 910, 910, 911, 912, 912, 913, 914, 914, 915, 916, 916, 917, 918, 918, 919, 919, 920, 921, 921, 922, 923, 923, 924, 925, 925, 926, 926,
    927, 928, 928, 929, 930, 930, 931, 932, 932, 933, 933, 934, 935, 935, 936, 937, 937, 938, 939, 939, 940, 940, 941, 942, 942, 943, 944, 944, 945, 945, 946,
    947, 947, 948, 949, 949, 950, 951, 951, 952, 952, 953, 954, 954, 955, 956, 956, 957, 957, 958, 959, 959, 960, 961, 961, 962, 962, 963, 964, 964, 965, 965,
    966, 967, 967, 968, 969, 969, 970, 970, 971, 972, 972, 973, 974, 974, 975, 975, 976, 977, 977, 978, 978, 979, 980, 980, 981, 982, 982, 983, 983, 984, 985,
    985, 986, 986, 987, 988, 988, 989, 989, 990, 991, 991, 992, 993, 993, 994, 994, 995, 996, 996, 997, 997, 998, 999, 999, 1000, 1000, 1001, 1002, 1002, 1003,
    1003, 1004, 1005, 1005, 1006, 1006, 1007, 1008, 1008, 1009, 1009, 1010, 1011, 1011, 1012, 1012, 1013, 1014, 1014, 1015, 1016
  };

// These are the rectified pressure sensititivies for each column
// CAREFUL, contrary to all the other arrays these are rows first and columns second since it makes it much easier to visualize and edit the
// actual values in a spreadsheet
const int Z_BIAS[NUMROWS][NUMCOLS] =  {
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
inline int readX() {                                  // returns the raw X value at the addressed cell
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
inline int readY() {                                  // returns a value of 0-127 within cell's y axis
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
inline byte readZ() {                                        // returns either 0 for non-touch (below touch threshold) or 1 to 127
  selectSensorCell(sensorCol, sensorRow, READ_Z);            // set analog switches to current cell in touch sensor and read Z
  delayUsec(7);                                              // prevent phantom reads when vertically adjacent cells are pressed, should be removed when the the ADC's pullup resistor is changed

  int rawZ = 4095 - spiAnalogRead();                         // read raw Z value and invert it from (4095 - 0) to (0-4095)
  rawZ = (rawZ * 1400) / Z_BIAS[sensorRow][sensorCol];       // apply the bias for each column, we also raise the baseline values to make the highest points just as sensitive and the lowest ones more sensitive

  cell().rawZ = rawZ;                                        // store the raw Z data for later comparisons and calculations
  if (rawZ < Global.sensorFeatherZ) return Z_VAL_NONE;       // if the raw touch is below feather touch, then return 0 as code for not touched

  int usableZ = rawZ - Global.sensorLoZ;                     // subtract minimum from value

  if (usableZ <= 0) return Z_VAL_FEATHER;                    // if it's below the acceptable minimum, return the feather touch code value
  if (usableZ > Global.sensorRangeZ) {                       // clip high end to highest acceptable raw value
    usableZ = Global.sensorRangeZ;
  }

  int32_t fxd_usableZ = FXD_MUL(FXD_FROM_INT(usableZ), FXD_DIV(FXD_FROM_INT(MAX_SENSOR_RANGE_Z), FXD_FROM_INT(Global.sensorRangeZ)));

  usableZ = Z_CURVE[FXD_TO_INT(fxd_usableZ)];                // apply the sensitivity curve

  // scale the result and return it as a byte in the range 1-127
  return byte(FXD_TO_INT(FXD_DIV(FXD_FROM_INT(usableZ), FXD_FROM_INT(8))) & B01111111);
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
