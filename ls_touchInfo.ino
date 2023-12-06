/************************** ls_touchInfo: LinnStrument TouchInfo methods **************************
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
These are the methods of the TouchInfo class, handling data related to the touch information of
each individual cell.
**************************************************************************************************/

/********************** the curve was generated with the following C program **********************
#include <stdio.h>
#include <math.h>
int main(int argc, const char* argv[]) {
  for (int z = 0; z <= 1016; ++z) {
    double x = ((double)z) / 1016.0;
    double v = (x * (2.0 - x) + pow(x, 0.2)) / 2.0;
    v *= 1016.0;
    printf("%d, ", (int)round(v));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE_HIGH[1017] = {
    0, 128, 148, 161, 172, 180, 188, 195, 201, 206, 212, 216, 221, 225, 230, 234, 237, 241, 245, 248, 251, 255, 258, 261, 264, 267, 270, 273, 275,
    278, 281, 283, 286, 288, 291, 293, 296, 298, 301, 303, 305, 307, 310, 312, 314, 316, 319, 321, 323, 325, 327, 329, 331, 333, 335, 337, 339, 341,
    343, 345, 347, 349, 350, 352, 354, 356, 358, 360, 362, 363, 365, 367, 369, 370, 372, 374, 376, 377, 379, 381, 382, 384, 386, 387, 389, 391, 392,
    394, 396, 397, 399, 400, 402, 404, 405, 407, 408, 410, 411, 413, 415, 416, 418, 419, 421, 422, 424, 425, 427, 428, 430, 431, 433, 434, 436, 437,
    439, 440, 441, 443, 444, 446, 447, 449, 450, 451, 453, 454, 456, 457, 458, 460, 461, 463, 464, 465, 467, 468, 469, 471, 472, 473, 475, 476, 477,
    479, 480, 481, 483, 484, 485, 487, 488, 489, 491, 492, 493, 495, 496, 497, 498, 500, 501, 502, 504, 505, 506, 507, 509, 510, 511, 512, 514, 515,
    516, 517, 519, 520, 521, 522, 523, 525, 526, 527, 528, 529, 531, 532, 533, 534, 536, 537, 538, 539, 540, 541, 543, 544, 545, 546, 547, 549, 550,
    551, 552, 553, 554, 555, 557, 558, 559, 560, 561, 562, 563, 565, 566, 567, 568, 569, 570, 571, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582,
    584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 594, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 614,
    615, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643,
    644, 645, 646, 647, 648, 649, 650, 651, 652, 653, 654, 655, 656, 657, 658, 659, 660, 660, 661, 662, 663, 664, 665, 666, 667, 668, 669, 670, 671,
    672, 673, 674, 675, 676, 677, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 688, 689, 689, 690, 691, 692, 693, 694, 695, 696, 697, 698,
    698, 699, 700, 701, 702, 703, 704, 705, 706, 706, 707, 708, 709, 710, 711, 712, 713, 713, 714, 715, 716, 717, 718, 719, 719, 720, 721, 722, 723,
    724, 725, 725, 726, 727, 728, 729, 730, 730, 731, 732, 733, 734, 735, 735, 736, 737, 738, 739, 740, 740, 741, 742, 743, 744, 744, 745, 746, 747,
    748, 749, 749, 750, 751, 752, 753, 753, 754, 755, 756, 757, 757, 758, 759, 760, 760, 761, 762, 763, 764, 764, 765, 766, 767, 768, 768, 769, 770,
    771, 771, 772, 773, 774, 774, 775, 776, 777, 777, 778, 779, 780, 780, 781, 782, 783, 783, 784, 785, 786, 786, 787, 788, 789, 789, 790, 791, 792,
    792, 793, 794, 795, 795, 796, 797, 797, 798, 799, 800, 800, 801, 802, 802, 803, 804, 805, 805, 806, 807, 807, 808, 809, 810, 810, 811, 812, 812,
    813, 814, 814, 815, 816, 816, 817, 818, 818, 819, 820, 821, 821, 822, 823, 823, 824, 825, 825, 826, 827, 827, 828, 829, 829, 830, 831, 831, 832,
    833, 833, 834, 835, 835, 836, 836, 837, 838, 838, 839, 840, 840, 841, 842, 842, 843, 844, 844, 845, 845, 846, 847, 847, 848, 849, 849, 850, 850,
    851, 852, 852, 853, 854, 854, 855, 855, 856, 857, 857, 858, 858, 859, 860, 860, 861, 861, 862, 863, 863, 864, 864, 865, 866, 866, 867, 867, 868,
    869, 869, 870, 870, 871, 871, 872, 873, 873, 874, 874, 875, 876, 876, 877, 877, 878, 878, 879, 879, 880, 881, 881, 882, 882, 883, 883, 884, 885,
    885, 886, 886, 887, 887, 888, 888, 889, 889, 890, 891, 891, 892, 892, 893, 893, 894, 894, 895, 895, 896, 896, 897, 897, 898, 898, 899, 900, 900,
    901, 901, 902, 902, 903, 903, 904, 904, 905, 905, 906, 906, 907, 907, 908, 908, 909, 909, 910, 910, 911, 911, 912, 912, 913, 913, 914, 914, 915,
    915, 916, 916, 916, 917, 917, 918, 918, 919, 919, 920, 920, 921, 921, 922, 922, 923, 923, 924, 924, 924, 925, 925, 926, 926, 927, 927, 928, 928,
    929, 929, 929, 930, 930, 931, 931, 932, 932, 933, 933, 933, 934, 934, 935, 935, 936, 936, 937, 937, 937, 938, 938, 939, 939, 939, 940, 940, 941,
    941, 942, 942, 942, 943, 943, 944, 944, 944, 945, 945, 946, 946, 947, 947, 947, 948, 948, 949, 949, 949, 950, 950, 950, 951, 951, 952, 952, 952,
    953, 953, 954, 954, 954, 955, 955, 955, 956, 956, 957, 957, 957, 958, 958, 958, 959, 959, 960, 960, 960, 961, 961, 961, 962, 962, 962, 963, 963,
    963, 964, 964, 965, 965, 965, 966, 966, 966, 967, 967, 967, 968, 968, 968, 969, 969, 969, 970, 970, 970, 971, 971, 971, 972, 972, 972, 973, 973,
    973, 974, 974, 974, 975, 975, 975, 975, 976, 976, 976, 977, 977, 977, 978, 978, 978, 979, 979, 979, 979, 980, 980, 980, 981, 981, 981, 982, 982,
    982, 982, 983, 983, 983, 984, 984, 984, 984, 985, 985, 985, 985, 986, 986, 986, 987, 987, 987, 987, 988, 988, 988, 988, 989, 989, 989, 989, 990,
    990, 990, 991, 991, 991, 991, 992, 992, 992, 992, 993, 993, 993, 993, 993, 994, 994, 994, 994, 995, 995, 995, 995, 996, 996, 996, 996, 997, 997,
    997, 997, 997, 998, 998, 998, 998, 999, 999, 999, 999, 999, 1000, 1000, 1000, 1000, 1000, 1001, 1001, 1001, 1001, 1001, 1002, 1002, 1002, 1002,
    1002, 1003, 1003, 1003, 1003, 1003, 1004, 1004, 1004, 1004, 1004, 1005, 1005, 1005, 1005, 1005, 1005, 1006, 1006, 1006, 1006, 1006, 1007, 1007,
    1007, 1007, 1007, 1007, 1008, 1008, 1008, 1008, 1008, 1008, 1009, 1009, 1009, 1009, 1009, 1009, 1010, 1010, 1010, 1010, 1010, 1010, 1010, 1011,
    1011, 1011, 1011, 1011, 1011, 1011, 1012, 1012, 1012, 1012, 1012, 1012, 1012, 1013, 1013, 1013, 1013, 1013, 1013, 1013, 1013, 1014, 1014, 1014,
    1014, 1014, 1014, 1014, 1014, 1014, 1015, 1015, 1015, 1015, 1015, 1015, 1015, 1015, 1015, 1016, 1016, 1016, 1016, 1016
};
/********************** the curve was generated with the following C program **********************
#include <stdio.h>
#include <math.h>
int main(int argc, const char* argv[]) {
  for (int z = 0; z <= 1016; ++z) {
    double x = ((double)z) / 1016.0;
    double v = (x * (2.3 - x) + pow(x, 0.23)) / 2.3;
    v *= 1016.0;
    printf("%d, ", (int)round(v));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE_MEDIUM[1017] = {
    0, 91, 107, 119, 128, 135, 142, 148, 153, 158, 163, 167, 171, 175, 179, 182, 186, 189, 193, 196, 199, 202, 205, 208, 210, 213, 216, 218, 221, 224,
    226, 229, 231, 233, 236, 238, 240, 243, 245, 247, 249, 251, 254, 256, 258, 260, 262, 264, 266, 268, 270, 272, 274, 276, 278, 280, 281, 283, 285, 287,
    289, 291, 293, 294, 296, 298, 300, 301, 303, 305, 307, 308, 310, 312, 313, 315, 317, 319, 320, 322, 323, 325, 327, 328, 330, 332, 333, 335, 336, 338,
    339, 341, 343, 344, 346, 347, 349, 350, 352, 353, 355, 356, 358, 359, 361, 362, 364, 365, 367, 368, 370, 371, 373, 374, 376, 377, 378, 380, 381, 383,
    384, 386, 387, 388, 390, 391, 393, 394, 395, 397, 398, 399, 401, 402, 404, 405, 406, 408, 409, 410, 412, 413, 414, 416, 417, 418, 420, 421, 422, 424,
    425, 426, 427, 429, 430, 431, 433, 434, 435, 437, 438, 439, 440, 442, 443, 444, 445, 447, 448, 449, 450, 452, 453, 454, 455, 457, 458, 459, 460, 462,
    463, 464, 465, 466, 468, 469, 470, 471, 473, 474, 475, 476, 477, 479, 480, 481, 482, 483, 484, 486, 487, 488, 489, 490, 492, 493, 494, 495, 496, 497,
    499, 500, 501, 502, 503, 504, 505, 507, 508, 509, 510, 511, 512, 513, 515, 516, 517, 518, 519, 520, 521, 522, 523, 525, 526, 527, 528, 529, 530, 531,
    532, 533, 535, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 548, 549, 550, 551, 552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 562, 563,
    564, 566, 567, 568, 569, 570, 571, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 594,
    595, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623, 624,
    624, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 651, 652,
    652, 653, 654, 655, 656, 657, 658, 659, 660, 661, 662, 662, 663, 664, 665, 666, 667, 668, 669, 670, 671, 671, 672, 673, 674, 675, 676, 677, 678, 678,
    679, 680, 681, 682, 683, 684, 685, 685, 686, 687, 688, 689, 690, 691, 691, 692, 693, 694, 695, 696, 697, 697, 698, 699, 700, 701, 702, 703, 703, 704,
    705, 706, 707, 708, 708, 709, 710, 711, 712, 713, 713, 714, 715, 716, 717, 717, 718, 719, 720, 721, 722, 722, 723, 724, 725, 726, 726, 727, 728, 729,
    730, 730, 731, 732, 733, 734, 734, 735, 736, 737, 738, 738, 739, 740, 741, 742, 742, 743, 744, 745, 745, 746, 747, 748, 749, 749, 750, 751, 752, 752,
    753, 754, 755, 755, 756, 757, 758, 759, 759, 760, 761, 762, 762, 763, 764, 765, 765, 766, 767, 768, 768, 769, 770, 771, 771, 772, 773, 773, 774, 775,
    776, 776, 777, 778, 779, 779, 780, 781, 782, 782, 783, 784, 784, 785, 786, 787, 787, 788, 789, 789, 790, 791, 792, 792, 793, 794, 794, 795, 796, 796,
    797, 798, 799, 799, 800, 801, 801, 802, 803, 803, 804, 805, 806, 806, 807, 808, 808, 809, 810, 810, 811, 812, 812, 813, 814, 814, 815, 816, 816, 817,
    818, 818, 819, 820, 820, 821, 822, 822, 823, 824, 824, 825, 826, 826, 827, 828, 828, 829, 830, 830, 831, 832, 832, 833, 833, 834, 835, 835, 836, 837,
    837, 838, 839, 839, 840, 840, 841, 842, 842, 843, 844, 844, 845, 845, 846, 847, 847, 848, 849, 849, 850, 850, 851, 852, 852, 853, 853, 854, 855, 855,
    856, 857, 857, 858, 858, 859, 860, 860, 861, 861, 862, 863, 863, 864, 864, 865, 865, 866, 867, 867, 868, 868, 869, 870, 870, 871, 871, 872, 872, 873,
    874, 874, 875, 875, 876, 876, 877, 878, 878, 879, 879, 880, 880, 881, 882, 882, 883, 883, 884, 884, 885, 885, 886, 887, 887, 888, 888, 889, 889, 890,
    890, 891, 891, 892, 893, 893, 894, 894, 895, 895, 896, 896, 897, 897, 898, 898, 899, 899, 900, 901, 901, 902, 902, 903, 903, 904, 904, 905, 905, 906,
    906, 907, 907, 908, 908, 909, 909, 910, 910, 911, 911, 912, 912, 913, 913, 914, 914, 915, 915, 916, 916, 917, 917, 918, 918, 919, 919, 920, 920, 921,
    921, 922, 922, 923, 923, 924, 924, 925, 925, 926, 926, 927, 927, 927, 928, 928, 929, 929, 930, 930, 931, 931, 932, 932, 933, 933, 933, 934, 934, 935,
    935, 936, 936, 937, 937, 938, 938, 938, 939, 939, 940, 940, 941, 941, 942, 942, 942, 943, 943, 944, 944, 945, 945, 946, 946, 946, 947, 947, 948, 948,
    949, 949, 949, 950, 950, 951, 951, 951, 952, 952, 953, 953, 954, 954, 954, 955, 955, 956, 956, 956, 957, 957, 958, 958, 958, 959, 959, 960, 960, 960,
    961, 961, 962, 962, 962, 963, 963, 964, 964, 964, 965, 965, 966, 966, 966, 967, 967, 967, 968, 968, 969, 969, 969, 970, 970, 970, 971, 971, 972, 972,
    972, 973, 973, 973, 974, 974, 975, 975, 975, 976, 976, 976, 977, 977, 977, 978, 978, 978, 979, 979, 980, 980, 980, 981, 981, 981, 982, 982, 982, 983,
    983, 983, 984, 984, 984, 985, 985, 985, 986, 986, 986, 987, 987, 987, 988, 988, 988, 989, 989, 989, 990, 990, 990, 991, 991, 991, 991, 992, 992, 992,
    993, 993, 993, 994, 994, 994, 995, 995, 995, 995, 996, 996, 996, 997, 997, 997, 998, 998, 998, 998, 999, 999, 999, 1000, 1000, 1000, 1000, 1001, 1001,
    1001, 1002, 1002, 1002, 1002, 1003, 1003, 1003, 1004, 1004, 1004, 1004, 1005, 1005, 1005, 1005, 1006, 1006, 1006, 1007, 1007, 1007, 1007, 1008, 1008,
    1008, 1008, 1009, 1009, 1009, 1009, 1010, 1010, 1010, 1010, 1011, 1011, 1011, 1011, 1012, 1012, 1012, 1012, 1013, 1013, 1013, 1013, 1014, 1014, 1014,
    1014, 1015, 1015, 1015, 1015, 1016, 1016, 1016
};

/********************** the curve was generated with the following C program **********************
#include <stdio.h>
#include <math.h>
int main(int argc, const char* argv[]) {
  for (int z = 0; z <= 1016; ++z) {
    double x = ((double)z) / 1016.0;
    double v = (x * (2.8 - x) + pow(x, 0.28)) / 2.8;
    v *= 1016.0;
    printf("%d, ", (int)round(v));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE_LOW[1017] = {
    0, 53, 65, 74, 81, 87, 92, 97, 101, 106, 109, 113, 117, 120, 123, 126, 129, 132, 135, 138, 141, 143, 146, 148, 151, 153, 156, 158, 160, 163, 165,
    167, 169, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200, 202, 203, 205, 207, 209, 211, 213, 214, 216, 218, 220, 221, 223,
    225, 226, 228, 230, 232, 233, 235, 237, 238, 240, 241, 243, 245, 246, 248, 250, 251, 253, 254, 256, 257, 259, 261, 262, 264, 265, 267, 268, 270, 271,
    273, 274, 276, 277, 279, 280, 282, 283, 285, 286, 288, 289, 290, 292, 293, 295, 296, 298, 299, 300, 302, 303, 305, 306, 308, 309, 310, 312, 313, 314,
    316, 317, 319, 320, 321, 323, 324, 325, 327, 328, 329, 331, 332, 333, 335, 336, 337, 339, 340, 341, 343, 344, 345, 347, 348, 349, 351, 352, 353, 354,
    356, 357, 358, 360, 361, 362, 363, 365, 366, 367, 369, 370, 371, 372, 374, 375, 376, 377, 379, 380, 381, 382, 384, 385, 386, 387, 388, 390, 391, 392,
    393, 395, 396, 397, 398, 399, 401, 402, 403, 404, 405, 407, 408, 409, 410, 411, 413, 414, 415, 416, 417, 418, 420, 421, 422, 423, 424, 426, 427, 428,
    429, 430, 431, 432, 434, 435, 436, 437, 438, 439, 441, 442, 443, 444, 445, 446, 447, 449, 450, 451, 452, 453, 454, 455, 456, 458, 459, 460, 461, 462,
    463, 464, 465, 466, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 492, 493, 494, 495,
    496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 525, 526,
    527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553, 554, 555, 556, 557,
    558, 559, 560, 561, 562, 562, 563, 564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 586,
    587, 588, 589, 590, 591, 592, 592, 593, 594, 595, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608, 608, 609, 610, 611, 612, 613, 614,
    615, 616, 617, 618, 619, 620, 621, 621, 622, 623, 624, 625, 626, 627, 628, 629, 630, 631, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 640, 641,
    642, 643, 644, 645, 646, 647, 648, 648, 649, 650, 651, 652, 653, 654, 655, 656, 656, 657, 658, 659, 660, 661, 662, 662, 663, 664, 665, 666, 667, 668,
    669, 669, 670, 671, 672, 673, 674, 675, 675, 676, 677, 678, 679, 680, 681, 681, 682, 683, 684, 685, 686, 686, 687, 688, 689, 690, 691, 691, 692, 693,
    694, 695, 696, 696, 697, 698, 699, 700, 701, 701, 702, 703, 704, 705, 706, 706, 707, 708, 709, 710, 710, 711, 712, 713, 714, 715, 715, 716, 717, 718,
    719, 719, 720, 721, 722, 723, 723, 724, 725, 726, 727, 727, 728, 729, 730, 731, 731, 732, 733, 734, 734, 735, 736, 737, 738, 738, 739, 740, 741, 741,
    742, 743, 744, 745, 745, 746, 747, 748, 748, 749, 750, 751, 752, 752, 753, 754, 755, 755, 756, 757, 758, 758, 759, 760, 761, 761, 762, 763, 764, 764,
    765, 766, 767, 767, 768, 769, 770, 770, 771, 772, 773, 773, 774, 775, 776, 776, 777, 778, 779, 779, 780, 781, 781, 782, 783, 784, 784, 785, 786, 787,
    787, 788, 789, 789, 790, 791, 792, 792, 793, 794, 794, 795, 796, 797, 797, 798, 799, 799, 800, 801, 802, 802, 803, 804, 804, 805, 806, 806, 807, 808,
    809, 809, 810, 811, 811, 812, 813, 813, 814, 815, 816, 816, 817, 818, 818, 819, 820, 820, 821, 822, 822, 823, 824, 824, 825, 826, 826, 827, 828, 828,
    829, 830, 830, 831, 832, 832, 833, 834, 834, 835, 836, 836, 837, 838, 838, 839, 840, 840, 841, 842, 842, 843, 844, 844, 845, 846, 846, 847, 848, 848,
    849, 850, 850, 851, 851, 852, 853, 853, 854, 855, 855, 856, 857, 857, 858, 858, 859, 860, 860, 861, 862, 862, 863, 864, 864, 865, 865, 866, 867, 867,
    868, 869, 869, 870, 870, 871, 872, 872, 873, 873, 874, 875, 875, 876, 877, 877, 878, 878, 879, 880, 880, 881, 881, 882, 883, 883, 884, 884, 885, 886,
    886, 887, 887, 888, 889, 889, 890, 890, 891, 891, 892, 893, 893, 894, 894, 895, 896, 896, 897, 897, 898, 898, 899, 900, 900, 901, 901, 902, 903, 903,
    904, 904, 905, 905, 906, 907, 907, 908, 908, 909, 909, 910, 910, 911, 912, 912, 913, 913, 914, 914, 915, 916, 916, 917, 917, 918, 918, 919, 919, 920,
    920, 921, 922, 922, 923, 923, 924, 924, 925, 925, 926, 926, 927, 928, 928, 929, 929, 930, 930, 931, 931, 932, 932, 933, 933, 934, 934, 935, 935, 936,
    937, 937, 938, 938, 939, 939, 940, 940, 941, 941, 942, 942, 943, 943, 944, 944, 945, 945, 946, 946, 947, 947, 948, 948, 949, 949, 950, 950, 951, 951,
    952, 952, 953, 953, 954, 954, 955, 955, 956, 956, 957, 957, 958, 958, 959, 959, 960, 960, 961, 961, 962, 962, 963, 963, 964, 964, 965, 965, 966, 966,
    966, 967, 967, 968, 968, 969, 969, 970, 970, 971, 971, 972, 972, 973, 973, 974, 974, 974, 975, 975, 976, 976, 977, 977, 978, 978, 979, 979, 980, 980,
    980, 981, 981, 982, 982, 983, 983, 984, 984, 984, 985, 985, 986, 986, 987, 987, 988, 988, 988, 989, 989, 990, 990, 991, 991, 991, 992, 992, 993, 993,
    994, 994, 994, 995, 995, 996, 996, 997, 997, 997, 998, 998, 999, 999, 1000, 1000, 1000, 1001, 1001, 1002, 1002, 1002, 1003, 1003, 1004, 1004, 1004,
    1005, 1005, 1006, 1006, 1007, 1007, 1007, 1008, 1008, 1009, 1009, 1009, 1010, 1010, 1011, 1011, 1011, 1012, 1012, 1012, 1013, 1013, 1014, 1014, 1014,
    1015, 1015, 1016, 1016
};

void initializeTouchInfo() {
  // Initialize the cells array, starting operating with no touched cells
  for (byte col = 0; col < NUMCOLS; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
#ifdef TESTING_SENSOR_DISABLE
      cell(col, row).disabled = false;
#endif
      cell(col, row).touched = untouchedCell;
      cell(col, row).lastTouch = 0;
      cell(col, row).clearSensorData();
      cell(col, row).vcount = 0;
      cell(col, row).velocity = 0;
      cell(col, row).note = -1;
      cell(col, row).channel = -1;
      cell(col, row).octaveOffset = 0;
      cell(col, row).fxdPrevPressure = 0;
      cell(col, row).fxdPrevTimbre = FXD_CONST_255;
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

  cellsTouched = 0;

  // Initialize the virtual touch info
  for (byte row = 0; row < NUMROWS; ++row) {
    virtualTouchInfo[row].split = 0;
    virtualTouchInfo[row].note = -1;
    virtualTouchInfo[row].channel = -1;
  }
}

// Calculate the velocity value by providing pressure (z) data.
//
// This function will return true when a new stable velocity value has been
// calculated. This is the moment when a new note should be sent out.
//
// The velocity is calculated using the slope of a linear regression algorithm:
//   ((n * sumXY) - sumX * sumY) / ((n * sumXSquared) - sumX * sumX)
//
// The X values are a linear progression from 0 to the number of samples, meaning
// that they can be pre-calculated beforehand and are the same for every press.
//
// An initial cooridinate with Y zero is always used (0,0), which causes the number
// of data points to be always one more than the number of velocity samples

// This element of the linear regression algorithm is constant based on the number of velocity samples 
const int VELOCITY_SXX = (VELOCITY_N * VELOCITY_SUMXSQ) - VELOCITY_SUMX * VELOCITY_SUMX;

inline byte scale1016to127(int v, boolean allowZero) {
  // reduce 1016 > 127 by dividing, but we do this so that values are rounded instead of truncated
  // we also assume that all values that are equal to zero are filtered out already, so the bottom
  // value is 1
  return constrain(((v * 10) + 40) / 80, allowZero ? 0 : 1, 127);
}

// since the velocity calculation is short-circuited to remain on the same cell,
// we don't need to store the temporary values independently for each cell, hence saving memory
static byte vcount1;
static unsigned short velSumY1;
static unsigned short velSumXY1;
static byte vcount2;
static unsigned short velSumY2;
static unsigned short velSumXY2;

// Re-initialize the velocity detection
void initVelocity() {
  vcount1 = 0;
  velSumY1 = 0;
  velSumXY1 = 0;

  vcount2 = 0;
  velSumY2 = 0;
  velSumXY2 = 0;

  sensorCell->vcount = 0;
  sensorCell->velocity = 0;
}

VelocityState calcVelocity(unsigned short z) {
  if (sensorCell->vcount < VELOCITY_TOTAL_SAMPLES) {

    // calculate the linear regression sums that are variable with the pressure
    if (sensorCell->vcount % 2 == 0) {
        velSumY1 += z;
        velSumXY1 += (vcount1 + VELOCITY_ZERO_POINTS) * z;
        vcount1 += 1;
    }
    else {
        velSumY2 += z;
        velSumXY2 += (vcount2 + VELOCITY_ZERO_POINTS) * z;
        vcount2 += 1;
    }

    sensorCell->vcount += 1;

    // when the number of samples are reached, calculate the final velocity
    if (sensorCell->vcount == VELOCITY_TOTAL_SAMPLES) {
      int scale;
      const short* curve;
      switch (Global.velocitySensitivity) {
        case velocityHigh:
          scale = VELOCITY_SCALE_HIGH;
          curve = Z_CURVE_HIGH;
          break;
        case velocityMedium:
        default:
          scale = VELOCITY_SCALE_MEDIUM;
          curve = Z_CURVE_MEDIUM;
          break;
        case velocityLow:
          scale = VELOCITY_SCALE_LOW;
          curve = Z_CURVE_LOW;
          break;
      }
      int sxy1 = (VELOCITY_N * velSumXY1) - VELOCITY_SUMX * velSumY1;
      int slope1 = curve[constrain((scale * sxy1) / VELOCITY_SXX, 1, 1016)];
      int sxy2 = (VELOCITY_N * velSumXY2) - VELOCITY_SUMX * velSumY2;
      int slope2 = curve[constrain((scale * sxy2) / VELOCITY_SXX, 1, 1016)];

      int slope = FXD_TO_INT(fxdMinVelOffset + FXD_MUL(FXD_DIV(FXD_FROM_INT(slope1 + slope2), FXD_CONST_2), fxdVelRatio));

      slope = scale1016to127(slope, false);

      sensorCell->velocity = calcPreferredVelocity(slope);

      return velocityNew;
    }
    else {
      return velocityCalculating;
    }
  }

  return velocityCalculated;
}

byte calcPreferredVelocity(byte velocity) {
  // determine the preferred velocity based on the sensitivity settings
  if (Global.velocitySensitivity == velocityFixed) {
    return Global.valueForFixedVelocity;
  }
  else {
    return constrain(velocity, 1, 127);
  }
}

boolean TouchInfo::isCalculatingVelocity() {
  return sensorCell->vcount > 0 && sensorCell->vcount < VELOCITY_TOTAL_SAMPLES;
}

boolean TouchInfo::isPastDebounceDelay() {
    // dynamically adapt the debounce time based on whether the touch has moved a significant amount
    return calcTimeDelta(millis(), lastTouch) > (didMove ? 70 : 35);
}

void TouchInfo::shouldRefreshData() {
  shouldRefreshX = true;
  shouldRefreshY = true;
  shouldRefreshZ = true;
}

unsigned short TouchInfo::rawX() {
  refreshX();
  return currentRawX;
}

short TouchInfo::calibratedX() {
  refreshX();
  return currentCalibratedX;
}

inline void TouchInfo::refreshX() {
  if (shouldRefreshX) {
    currentRawX = readX(percentRawZ);
    currentCalibratedX = calculateCalibratedX(currentRawX);
    shouldRefreshX = false;

    // if this is the first X read for this touch...
    if (initialX == INVALID_DATA) {
      // store the calibrated X reference that corresponds to the cell's note without any pitch bend
      initialColumn = sensorCol;

      // store the initial X position
      initialX = currentCalibratedX;
      
      // if pitch quantize is on, the first X position becomes the center point and considered 0
      quantizationOffsetX = currentCalibratedX - FXD_TO_INT(fxdInitialReferenceX());
      
      fxdRateX = 0;
      lastMovedX = 0;
      lastValueX = INVALID_DATA;
    }
    else if (abs(initialX - currentCalibratedX) > CALX_QUARTER_UNIT) {
        didMove = true;
    }
  }
}

inline int32_t TouchInfo::fxdInitialReferenceX() {
  if (initialColumn == -1) {
    return 0;
  }
  return Device.calRows[initialColumn][0].fxdReferenceX;
}

unsigned short TouchInfo::rawY() {
  refreshY();
  return currentRawY;
}

inline void TouchInfo::refreshY() {
  if (shouldRefreshY) {
    currentRawY = readY(percentRawZ);
    currentCalibratedY = calculateCalibratedY(currentRawY);
    shouldRefreshY = false;

    // if this is the first Y read for this touch...
    if (initialY == -1) {
      // store the initial Y position
      initialY = currentCalibratedY;
    }
  }
}

byte TouchInfo::calibratedY() {
  refreshY();
  return currentCalibratedY;
}

unsigned short TouchInfo::rawZ() {
  refreshZ();
  return currentRawZ;
}

inline boolean TouchInfo::isMeaningfulTouch() {
  refreshZ();
  return velocityZ > 0;
}

inline boolean TouchInfo::isStableYTouch() {    
  return sensorCell->isMeaningfulTouch() && sensorCell->rawZ() > Device.sensorLoZ + Device.sensorRangeZ / 4;
}

inline boolean TouchInfo::isActiveTouch() {
  refreshZ();
  return featherTouch || velocityZ > 0 || pressureZ > 0;
}

const short CONTROL_VELOCITY = 127;
const short CONTROL_PRESSURE = 1016;

const short CONTROL_MODE_LOZ = 20;
const short STRUM_FEATHERZ = 20;
const short STRUM_LOZ = 20;
const short SWITCH_FEATHERZ = 120;
const short SWITCH_LOZ = 230;

inline unsigned short calculateSensorRangeZ() {
    return constrain(Device.sensorRangeZ + 127, 3 * 127, MAX_SENSOR_RANGE_Z - 127);
}

inline unsigned short calculatePreferredPressureRange(unsigned short sensorRangeZ) {
    // when in pressure sensitivity calibration mode, always use medium sensitivity
    PressureSensitivity pressureSensitivity = Global.pressureSensitivity;
    if (displayMode == displaySensorSensitivityZ) {
        pressureSensitivity = pressureMedium;
    }
    unsigned short sensorRangePressure = sensorRangeZ;
    switch (pressureSensitivity) {
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
    return sensorRangePressure;
}

inline void TouchInfo::refreshZ() {
  if (shouldRefreshZ) {
    // store the raw Z data for later comparisons and calculations
    unsigned short previousPreviousRawZ = previousRawZ;
    previousRawZ = currentRawZ;
    currentRawZ = readZ();
    featherTouch = false;
    velocityZ = 0;
    pressureZ = 0;

    shouldRefreshZ = false;

    if (controlModeActive && currentRawZ >= CONTROL_MODE_LOZ) {
      featherTouch = true;
      velocityZ = CONTROL_VELOCITY;
      pressureZ = CONTROL_PRESSURE;
      return;
    }

    unsigned short featherZ = Device.sensorFeatherZ;
    unsigned short loZ = Device.sensorLoZ;
    if (sensorCol == 0) {
      featherZ = SWITCH_FEATHERZ;
      loZ = SWITCH_LOZ;
    }
    else if (isStrummingSplit(sensorSplit) && displayMode == displayNormal) {
      featherZ = STRUM_FEATHERZ;
      loZ = STRUM_LOZ;
    }

    // if the raw touch is below feather touch, keep 0 for the Z values
    if (currentRawZ < featherZ) {
      if (previousRawZ > featherZ && velocity) {
        int x1 = previousPreviousRawZ - currentRawZ;
        int x2 = previousRawZ - currentRawZ;
        velocity = constrain((x1 + x2) / 5, 1, 127);
      }
      return;
    }

    // subtract minimum from value
    short usableZ = currentRawZ - loZ;

    // if it's below the acceptable minimum, store it as a feather touch
    if (usableZ <= 0) {
      featherTouch = true;
      return;
    }

    // the control switches always have maximum velocity and pressure
    if (sensorCol == 0) {
      velocityZ = CONTROL_VELOCITY;
      pressureZ = CONTROL_PRESSURE;
      return;
    }

    // calculate the velocity and pressure for the playing cells
    unsigned short sensorRange = calculateSensorRangeZ();

    unsigned short sensorRangeVelocity = sensorRange;
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
      case velocityFixed:
        // no change
        break;
    }

    unsigned short sensorRangePressure = calculatePreferredPressureRange(sensorRange);

    unsigned short usableVelocityZ = constrain(usableZ, 1, sensorRangeVelocity);
    unsigned short usablePressureZ;
    if (Global.pressureAftertouch) {
      sensorRangePressure /= 5;
      usablePressureZ = constrain(usableZ - (sensorRangeVelocity - sensorRangePressure), 0, sensorRangePressure);
    }
    else {
      usablePressureZ = constrain(usableZ, 1, sensorRangePressure);
    }
    percentRawZ = (constrain(usableZ, 0, sensorRange) * 100) / sensorRange;

    int32_t fxd_usableVelocityZ = FXD_MUL(FXD_FROM_INT(usableVelocityZ), FXD_DIV(FXD_FROM_INT(MAX_SENSOR_RANGE_Z), FXD_FROM_INT(sensorRangeVelocity)));
    int32_t fxd_usablePressureZ = FXD_MUL(FXD_FROM_INT(usablePressureZ), FXD_DIV(FXD_FROM_INT(MAX_SENSOR_RANGE_Z), FXD_FROM_INT(sensorRangePressure)));

    // apply the sensitivity curve
    usableVelocityZ = FXD_TO_INT(fxd_usableVelocityZ);
    usablePressureZ = FXD_TO_INT(fxd_usablePressureZ);

    // scale the result and store it as a byte in the range 0-127
    velocityZ = scale1016to127(usableVelocityZ, false);
    pressureZ = constrain(usablePressureZ, 0, 1016);
  }
}

boolean TouchInfo::hasNote() {
  return note != -1 && channel != -1;
}

void TouchInfo::clearAllPhantoms() {
  if (hasPhantoms()) {
    cell(phantomCol1, phantomRow1).clearPhantoms();
    cell(phantomCol2, phantomRow1).clearPhantoms();
    cell(phantomCol1, phantomRow2).clearPhantoms();
    cell(phantomCol2, phantomRow2).clearPhantoms();
  }
}

void TouchInfo::clearPhantoms() {
  phantomSet = false;
  phantomCol1 = 0;
  phantomCol2 = 0;
  phantomRow1 = 0;
  phantomRow2 = 0;
}

boolean TouchInfo::hasPhantoms() {
  return phantomSet;
}

void TouchInfo::setPhantoms(byte col1, byte col2, byte row1, byte row2) {
  phantomSet = true;
  phantomCol1 = col1;
  phantomCol2 = col2;
  phantomRow1 = row1;
  phantomRow2 = row2;
}

boolean TouchInfo::isHigherPhantomPressure(short other) {
  return hasNote() || currentRawZ > other;
}

boolean TouchInfo::hasRogueSweepX() {
  return rogueSweepX;
}

boolean TouchInfo::hasUsableX() {
    return !phantomSet || !rogueSweepX;
}

void TouchInfo::clearMusicalData() {
  note = -1;
  channel = -1;
  octaveOffset = 0;
  fxdPrevPressure = 0;
  fxdPrevTimbre = FXD_CONST_255;
}

void TouchInfo::clearSensorData() {
  initialX = INVALID_DATA;
  initialColumn = -1;
  quantizationOffsetX = 0;
  currentRawX = 0;
  currentCalibratedX = 0;
  lastMovedX = 0;
  lastValueX = INVALID_DATA;
  fxdRateX = 0;
  fxdRateCountX = 0;
  shouldRefreshX = true;
  slideTransfer = false;
  rogueSweepX = false;
  initialY = -1;
  currentRawY = 0;
  currentCalibratedY = 0;
  shouldRefreshY = true;
  previousRawZ = 0;
  currentRawZ = 0;
  percentRawZ = 0;
  featherTouch = false;
  velocityZ = 0;
  pressureZ = 0;
  shouldRefreshZ = true;
  pendingReleaseCount = 0;
  didMove = false;
}

boolean VirtualTouchInfo::hasNote() {
  return note != -1 && channel != -1;
}

void VirtualTouchInfo::clearData() {
  split = 0;
  note = -1;
  channel = -1;
}

void VirtualTouchInfo::releaseNote() {
  if (hasNote()) {
    midiSendNoteOff(split, note, channel);

    releaseChannel(split, channel);
    clearData();
  }
}
