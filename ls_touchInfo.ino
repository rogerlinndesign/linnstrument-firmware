/************************** ls_touchInfo: LinnStrument TouchInfo methods **************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These are the methods of the TouchInfo class, handling data related to the touch information of
each individual cell.
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
    0, 15, 24, 30, 36, 41, 46, 51, 55, 59, 63, 67, 70, 74, 77, 80, 84, 87, 90, 93, 96, 99, 101, 104, 107, 110, 112, 115, 117, 120, 122, 125, 127, 129, 132, 134, 136, 139,
    141, 143, 145, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 187, 189, 191, 193, 195, 197, 198, 200, 202, 204,
    205, 207, 209, 211, 212, 214, 216, 217, 219, 221, 222, 224, 226, 227, 229, 230, 232, 234, 235, 237, 238, 240, 242, 243, 245, 246, 248, 249, 251, 252, 254, 255, 257,
    258, 260, 261, 263, 264, 266, 267, 269, 270, 272, 273, 274, 276, 277, 279, 280, 282, 283, 284, 286, 287, 289, 290, 291, 293, 294, 295, 297, 298, 299, 301, 302, 304,
    305, 306, 308, 309, 310, 311, 313, 314, 315, 317, 318, 319, 321, 322, 323, 324, 326, 327, 328, 330, 331, 332, 333, 335, 336, 337, 338, 340, 341, 342, 343, 345, 346,
    347, 348, 350, 351, 352, 353, 354, 356, 357, 358, 359, 360, 362, 363, 364, 365, 366, 368, 369, 370, 371, 372, 373, 375, 376, 377, 378, 379, 380, 382, 383, 384, 385,
    386, 387, 388, 390, 391, 392, 393, 394, 395, 396, 397, 399, 400, 401, 402, 403, 404, 405, 406, 407, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 421, 422,
    423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456,
    457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 478, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489,
    490, 491, 492, 493, 494, 495, 496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521,
    522, 523, 524, 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 546, 547, 548, 549, 550, 551,
    552, 553, 554, 555, 556, 556, 557, 558, 559, 560, 561, 562, 563, 564, 564, 565, 566, 567, 568, 569, 570, 571, 572, 572, 573, 574, 575, 576, 577, 578, 579, 579, 580,
    581, 582, 583, 584, 585, 585, 586, 587, 588, 589, 590, 591, 592, 592, 593, 594, 595, 596, 597, 598, 598, 599, 600, 601, 602, 603, 603, 604, 605, 606, 607, 608, 609,
    609, 610, 611, 612, 613, 614, 614, 615, 616, 617, 618, 619, 619, 620, 621, 622, 623, 624, 624, 625, 626, 627, 628, 629, 629, 630, 631, 632, 633, 634, 634, 635, 636,
    637, 638, 638, 639, 640, 641, 642, 643, 643, 644, 645, 646, 647, 647, 648, 649, 650, 651, 651, 652, 653, 654, 655, 655, 656, 657, 658, 659, 659, 660, 661, 662, 663,
    663, 664, 665, 666, 667, 667, 668, 669, 670, 671, 671, 672, 673, 674, 675, 675, 676, 677, 678, 678, 679, 680, 681, 682, 682, 683, 684, 685, 686, 686, 687, 688, 689,
    689, 690, 691, 692, 693, 693, 694, 695, 696, 696, 697, 698, 699, 699, 700, 701, 702, 703, 703, 704, 705, 706, 706, 707, 708, 709, 709, 710, 711, 712, 712, 713, 714,
    715, 715, 716, 717, 718, 719, 719, 720, 721, 722, 722, 723, 724, 725, 725, 726, 727, 728, 728, 729, 730, 731, 731, 732, 733, 734, 734, 735, 736, 737, 737, 738, 739,
    739, 740, 741, 742, 742, 743, 744, 745, 745, 746, 747, 748, 748, 749, 750, 751, 751, 752, 753, 753, 754, 755, 756, 756, 757, 758, 759, 759, 760, 761, 761, 762, 763,
    764, 764, 765, 766, 767, 767, 768, 769, 769, 770, 771, 772, 772, 773, 774, 774, 775, 776, 777, 777, 778, 779, 780, 780, 781, 782, 782, 783, 784, 785, 785, 786, 787,
    787, 788, 789, 789, 790, 791, 792, 792, 793, 794, 794, 795, 796, 797, 797, 798, 799, 799, 800, 801, 801, 802, 803, 804, 804, 805, 806, 806, 807, 808, 808, 809, 810,
    811, 811, 812, 813, 813, 814, 815, 815, 816, 817, 818, 818, 819, 820, 820, 821, 822, 822, 823, 824, 824, 825, 826, 827, 827, 828, 829, 829, 830, 831, 831, 832, 833,
    833, 834, 835, 835, 836, 837, 837, 838, 839, 840, 840, 841, 842, 842, 843, 844, 844, 845, 846, 846, 847, 848, 848, 849, 850, 850, 851, 852, 852, 853, 854, 854, 855,
    856, 856, 857, 858, 858, 859, 860, 860, 861, 862, 862, 863, 864, 864, 865, 866, 866, 867, 868, 868, 869, 870, 870, 871, 872, 872, 873, 874, 874, 875, 876, 876, 877,
    878, 878, 879, 880, 880, 881, 882, 882, 883, 884, 884, 885, 886, 886, 887, 888, 888, 889, 890, 890, 891, 892, 892, 893, 894, 894, 895, 896, 896, 897, 897, 898, 899,
    899, 900, 901, 901, 902, 903, 903, 904, 905, 905, 906, 907, 907, 908, 908, 909, 910, 910, 911, 912, 912, 913, 914, 914, 915, 916, 916, 917, 918, 918, 919, 919, 920,
    921, 921, 922, 923, 923, 924, 925, 925, 926, 926, 927, 928, 928, 929, 930, 930, 931, 932, 932, 933, 933, 934, 935, 935, 936, 937, 937, 938, 939, 939, 940, 940, 941,
    942, 942, 943, 944, 944, 945, 945, 946, 947, 947, 948, 949, 949, 950, 951, 951, 952, 952, 953, 954, 954, 955, 956, 956, 957, 957, 958, 959, 959, 960, 961, 961, 962,
    962, 963, 964, 964, 965, 965, 966, 967, 967, 968, 969, 969, 970, 970, 971, 972, 972, 973, 974, 974, 975, 975, 976, 977, 977, 978, 978, 979, 980, 980, 981, 982, 982,
    983, 983, 984, 985, 985, 986, 986, 987, 988, 988, 989, 989, 990, 991, 991, 992, 993, 993, 994, 994, 995, 996, 996, 997, 997, 998, 999, 999, 1000, 1000, 1001, 1002,
    1002, 1003, 1003, 1004, 1005, 1005, 1006, 1006, 1007, 1008, 1008, 1009, 1009, 1010, 1011, 1011, 1012, 1012, 1013, 1014, 1014, 1015, 1016
  };


void initializeTouchInfo() {
  // Initialize the cells array, starting operating with no touched cells
  for (byte col = 0; col < NUMCOLS; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      cell(col, row).touched = untouchedCell;
      cell(col, row).lastTouch = 0;
      cell(col, row).clearSensorData();
      cell(col, row).vcount = 0;
      cell(col, row).velocity = 0;
      cell(col, row).note = -1;
      cell(col, row).channel = -1;
      cell(col, row).fxdPrevPressure = 0;
      cell(col, row).fxdPrevTimbre = 0;
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

short TouchInfo::rawX() {
  refreshX();
  return currentRawX;
}

short TouchInfo::calibratedX() {
  refreshX();
  return currentCalibratedX;
}

inline void TouchInfo::refreshX() {
  if (shouldRefreshX) {
    currentRawX = readX();
    currentCalibratedX = calculateCalibratedX(currentRawX);
    shouldRefreshX = false;

    // if this is the first X read for this touch...
    if (initialX == -1) {
      // store the calibrated X reference that corresponds to the cell's note without any pitch bend
      initialReferenceX = FXD_TO_INT(Global.calRows[sensorCol][0].fxdReferenceX);

      // store the initial X position
      initialX = currentCalibratedX;
      
      quantizationOffsetX = 0;
      fxdRateX = 0;
      lastMovedX = 0;
    }
  }
}

short TouchInfo::rawY() {
  refreshY();
  return currentRawY;
}

inline void TouchInfo::refreshY() {
  if (shouldRefreshY) {
    currentRawY = readY();
    currentCalibratedY = calculateCalibratedY(currentRawY);
    shouldRefreshY = false;

    // if this is the first Y read for this touch...
    if (initialY == -1) {
      // store the initial Y position
      initialY = currentCalibratedY;
    }
  }
}

signed char TouchInfo::calibratedY() {
  refreshY();
  return currentCalibratedY;
}

short TouchInfo::rawZ() {
  refreshZ();
  return currentRawZ;
}

inline boolean TouchInfo::isMeaningfulTouch() {
  refreshZ();
  return velocityZ > 0 || pressureZ > 0;
}

inline boolean TouchInfo::isActiveTouch() {
  refreshZ();
  return featherTouch || velocityZ > 0 || pressureZ > 0;
}

inline void TouchInfo::refreshZ() {
  if (shouldRefreshZ) {
    currentRawZ = readZ();                            // store the raw Z data for later comparisons and calculations
    featherTouch = false;
    velocityZ = 0;
    pressureZ = 0;

    shouldRefreshZ = false;

    if (currentRawZ < Global.sensorFeatherZ) {        // if the raw touch is below feather touch, keep 0 for the Z values
      return;
    }

    short usableZ = currentRawZ - Global.sensorLoZ;   // subtract minimum from value

    if (usableZ <= 0) {                               // if it's below the acceptable minimum, store it as a feather touch
      featherTouch = true;
      return;
    }

    unsigned short sensorRange = constrain(Global.sensorRangeZ + 127, 3 * 127, MAX_SENSOR_RANGE_Z);

    unsigned short sensorRangeVelocity = sensorRange;
    unsigned short sensorRangePressure = sensorRange;
    switch (Global.velocitySensitivity) {
      case velocityHigh:
        sensorRangeVelocity -= 254;
        break;
      case velocityMedium:
        sensorRangeVelocity -= 63;
        break;
      case velocityLow:
        sensorRangeVelocity += 127;
        break;
    }
    switch (Global.pressureSensitivity) {
      case pressureHigh:
        sensorRangePressure -= 254;
        break;
      case pressureMedium:
        sensorRangePressure -= 63;
        break;
      case pressureLow:
        sensorRangePressure += 127;
        break;
    }

    short usableVelocityZ = constrain(usableZ, 0, sensorRangeVelocity);
    short usablePressureZ = constrain(usableZ, 0, sensorRangePressure);

    int32_t fxd_usableVelocityZ = FXD_MUL(FXD_FROM_INT(usableVelocityZ), FXD_DIV(FXD_FROM_INT(MAX_SENSOR_RANGE_Z), FXD_FROM_INT(sensorRangeVelocity)));
    int32_t fxd_usablePressureZ = FXD_MUL(FXD_FROM_INT(usablePressureZ), FXD_DIV(FXD_FROM_INT(MAX_SENSOR_RANGE_Z), FXD_FROM_INT(sensorRangePressure)));

    // apply the sensitivity curve
    usableVelocityZ = FXD_TO_INT(fxd_usableVelocityZ);
    usablePressureZ = FXD_TO_INT(fxd_usablePressureZ);
    if (Global.velocitySensitivity != velocityLow) {
      usableVelocityZ = Z_CURVE[usableVelocityZ];
    }
    if (Global.velocitySensitivity != velocityLow) {
      usablePressureZ = Z_CURVE[usablePressureZ];
    }

    // scale the result and store it as a byte in the range 1-127
    velocityZ = byte(FXD_TO_INT(FXD_DIV(FXD_FROM_INT(usableVelocityZ), FXD_FROM_INT(8))) & B01111111);
    pressureZ = byte(FXD_TO_INT(FXD_DIV(FXD_FROM_INT(usablePressureZ), FXD_FROM_INT(8))) & B01111111);
  }
}

boolean TouchInfo::isCalculatingVelocity() {
  return sensorCell().vcount > 0 && sensorCell().vcount != VELOCITY_SAMPLES;
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

boolean TouchInfo::isHigherPhantomPressure(short other) {
  return hasNote() || currentRawZ > other;
}

void TouchInfo::clearSensorData() {
  initialX = -1;
  initialReferenceX = 0;
  quantizationOffsetX = 0;
  currentRawX = 0;
  currentCalibratedX = 0;
  lastMovedX = 0;
  fxdRateX = 0;
  rateCountX = 0;
  shouldRefreshX = true;
  initialY = -1;
  currentRawY = 0;
  currentCalibratedY = 0;
  shouldRefreshY = true;
  currentRawZ = 0;
  featherTouch = false;
  velocityZ = 0;
  pressureZ = 0;
  shouldRefreshZ = true;
  pendingReleaseCount = 0;
  velSumY = 0;
  velSumXY = 0;
}