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
    double v = pow(z/1016., 0.6) * 1016.;
    v = (2032 * v) / (1016 + v);
    printf("%d, ", (int)round(v));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE_HIGH[1017] = {
    0, 31, 47, 60, 71, 80, 89, 98, 105, 113, 120, 126, 132, 139, 144, 150, 155, 161, 166, 171, 176, 181, 185, 190, 194, 199, 203, 207, 211, 215, 219,
    223, 227, 230, 234, 238, 241, 245, 248, 252, 255, 258, 262, 265, 268, 271, 274, 278, 281, 284, 287, 289, 292, 295, 298, 301, 304, 306, 309, 312,
    315, 317, 320, 322, 325, 327, 330, 333, 335, 337, 340, 342, 345, 347, 349, 352, 354, 356, 359, 361, 363, 365, 368, 370, 372, 374, 376, 378, 381,
    383, 385, 387, 389, 391, 393, 395, 397, 399, 401, 403, 405, 407, 409, 411, 413, 414, 416, 418, 420, 422, 424, 426, 427, 429, 431, 433, 434, 436,
    438, 440, 441, 443, 445, 447, 448, 450, 452, 453, 455, 457, 458, 460, 462, 463, 465, 466, 468, 470, 471, 473, 474, 476, 477, 479, 480, 482, 483,
    485, 487, 488, 489, 491, 492, 494, 495, 497, 498, 500, 501, 503, 504, 505, 507, 508, 510, 511, 512, 514, 515, 517, 518, 519, 521, 522, 523, 525,
    526, 527, 529, 530, 531, 533, 534, 535, 536, 538, 539, 540, 542, 543, 544, 545, 547, 548, 549, 550, 552, 553, 554, 555, 556, 558, 559, 560, 561,
    562, 564, 565, 566, 567, 568, 570, 571, 572, 573, 574, 575, 576, 578, 579, 580, 581, 582, 583, 584, 585, 587, 588, 589, 590, 591, 592, 593, 594,
    595, 596, 597, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623, 624,
    625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 644, 645, 646, 647, 648, 649, 650, 651, 652,
    653, 654, 655, 655, 656, 657, 658, 659, 660, 661, 662, 663, 663, 664, 665, 666, 667, 668, 669, 670, 670, 671, 672, 673, 674, 675, 676, 676, 677,
    678, 679, 680, 681, 682, 682, 683, 684, 685, 686, 687, 687, 688, 689, 690, 691, 691, 692, 693, 694, 695, 695, 696, 697, 698, 699, 699, 700, 701,
    702, 703, 703, 704, 705, 706, 707, 707, 708, 709, 710, 710, 711, 712, 713, 713, 714, 715, 716, 716, 717, 718, 719, 719, 720, 721, 722, 722, 723,
    724, 725, 725, 726, 727, 728, 728, 729, 730, 730, 731, 732, 733, 733, 734, 735, 736, 736, 737, 738, 738, 739, 740, 740, 741, 742, 743, 743, 744,
    745, 745, 746, 747, 747, 748, 749, 749, 750, 751, 752, 752, 753, 754, 754, 755, 756, 756, 757, 758, 758, 759, 760, 760, 761, 762, 762, 763, 764,
    764, 765, 766, 766, 767, 767, 768, 769, 769, 770, 771, 771, 772, 773, 773, 774, 775, 775, 776, 776, 777, 778, 778, 779, 780, 780, 781, 781, 782,
    783, 783, 784, 785, 785, 786, 786, 787, 788, 788, 789, 789, 790, 791, 791, 792, 792, 793, 794, 794, 795, 795, 796, 797, 797, 798, 798, 799, 800,
    800, 801, 801, 802, 803, 803, 804, 804, 805, 805, 806, 807, 807, 808, 808, 809, 809, 810, 811, 811, 812, 812, 813, 813, 814, 815, 815, 816, 816,
    817, 817, 818, 818, 819, 820, 820, 821, 821, 822, 822, 823, 823, 824, 825, 825, 826, 826, 827, 827, 828, 828, 829, 829, 830, 830, 831, 832, 832,
    833, 833, 834, 834, 835, 835, 836, 836, 837, 837, 838, 838, 839, 839, 840, 841, 841, 842, 842, 843, 843, 844, 844, 845, 845, 846, 846, 847, 847,
    848, 848, 849, 849, 850, 850, 851, 851, 852, 852, 853, 853, 854, 854, 855, 855, 856, 856, 857, 857, 858, 858, 859, 859, 860, 860, 861, 861, 862,
    862, 863, 863, 864, 864, 865, 865, 866, 866, 867, 867, 868, 868, 868, 869, 869, 870, 870, 871, 871, 872, 872, 873, 873, 874, 874, 875, 875, 876,
    876, 876, 877, 877, 878, 878, 879, 879, 880, 880, 881, 881, 882, 882, 883, 883, 883, 884, 884, 885, 885, 886, 886, 887, 887, 888, 888, 888, 889,
    889, 890, 890, 891, 891, 892, 892, 892, 893, 893, 894, 894, 895, 895, 896, 896, 896, 897, 897, 898, 898, 899, 899, 899, 900, 900, 901, 901, 902,
    902, 902, 903, 903, 904, 904, 905, 905, 905, 906, 906, 907, 907, 908, 908, 908, 909, 909, 910, 910, 911, 911, 911, 912, 912, 913, 913, 913, 914,
    914, 915, 915, 916, 916, 916, 917, 917, 918, 918, 918, 919, 919, 920, 920, 920, 921, 921, 922, 922, 923, 923, 923, 924, 924, 925, 925, 925, 926,
    926, 927, 927, 927, 928, 928, 929, 929, 929, 930, 930, 931, 931, 931, 932, 932, 932, 933, 933, 934, 934, 934, 935, 935, 936, 936, 936, 937, 937,
    938, 938, 938, 939, 939, 939, 940, 940, 941, 941, 941, 942, 942, 943, 943, 943, 944, 944, 944, 945, 945, 946, 946, 946, 947, 947, 947, 948, 948,
    949, 949, 949, 950, 950, 950, 951, 951, 952, 952, 952, 953, 953, 953, 954, 954, 954, 955, 955, 956, 956, 956, 957, 957, 957, 958, 958, 958, 959,
    959, 960, 960, 960, 961, 961, 961, 962, 962, 962, 963, 963, 963, 964, 964, 965, 965, 965, 966, 966, 966, 967, 967, 967, 968, 968, 968, 969, 969,
    969, 970, 970, 970, 971, 971, 972, 972, 972, 973, 973, 973, 974, 974, 974, 975, 975, 975, 976, 976, 976, 977, 977, 977, 978, 978, 978, 979, 979,
    979, 980, 980, 980, 981, 981, 981, 982, 982, 982, 983, 983, 983, 984, 984, 984, 985, 985, 985, 986, 986, 986, 987, 987, 987, 988, 988, 988, 989,
    989, 989, 990, 990, 990, 991, 991, 991, 992, 992, 992, 993, 993, 993, 994, 994, 994, 995, 995, 995, 996, 996, 996, 996, 997, 997, 997, 998, 998,
    998, 999, 999, 999, 1000, 1000, 1000, 1001, 1001, 1001, 1002, 1002, 1002, 1003, 1003, 1003, 1003, 1004, 1004, 1004, 1005, 1005, 1005, 1006, 1006,
    1006, 1007, 1007, 1007, 1007, 1008, 1008, 1008, 1009, 1009, 1009, 1010, 1010, 1010, 1011, 1011, 1011, 1011, 1012, 1012, 1012, 1013, 1013, 1013,
    1014, 1014, 1014, 1014, 1015, 1015, 1015, 1016, 1016
  };
/********************** the curve was generated with the following C program **********************
#include <stdio.h>
#include <math.h>
int main(int argc, const char* argv[]) {
  for (int z = 0; z <= 1016; ++z) {
    double v = pow(z/1016., 0.67) * 1016.;
    v = (2032 * v) / (1016 + v);
    printf("%d, ", (int)round(v));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE_MEDIUM[1017] = {
    0, 19, 31, 40, 49, 56, 63, 70, 76, 82, 88, 93, 99, 104, 109, 114, 119, 123, 128, 132, 136, 141, 145, 149, 153, 157, 161, 164, 168, 172, 175, 179,
    182, 186, 189, 193, 196, 199, 202, 206, 209, 212, 215, 218, 221, 224, 227, 230, 233, 236, 238, 241, 244, 247, 250, 252, 255, 258, 260, 263, 265,
    268, 270, 273, 276, 278, 280, 283, 285, 288, 290, 293, 295, 297, 300, 302, 304, 306, 309, 311, 313, 315, 318, 320, 322, 324, 326, 328, 330, 333,
    335, 337, 339, 341, 343, 345, 347, 349, 351, 353, 355, 357, 359, 361, 363, 364, 366, 368, 370, 372, 374, 376, 378, 379, 381, 383, 385, 387, 388,
    390, 392, 394, 396, 397, 399, 401, 402, 404, 406, 408, 409, 411, 413, 414, 416, 418, 419, 421, 422, 424, 426, 427, 429, 431, 432, 434, 435, 437,
    438, 440, 441, 443, 445, 446, 448, 449, 451, 452, 454, 455, 457, 458, 460, 461, 462, 464, 465, 467, 468, 470, 471, 473, 474, 475, 477, 478, 480,
    481, 482, 484, 485, 487, 488, 489, 491, 492, 493, 495, 496, 497, 499, 500, 501, 503, 504, 505, 507, 508, 509, 510, 512, 513, 514, 516, 517, 518,
    519, 521, 522, 523, 524, 526, 527, 528, 529, 530, 532, 533, 534, 535, 537, 538, 539, 540, 541, 542, 544, 545, 546, 547, 548, 550, 551, 552, 553,
    554, 555, 556, 558, 559, 560, 561, 562, 563, 564, 565, 567, 568, 569, 570, 571, 572, 573, 574, 575, 576, 578, 579, 580, 581, 582, 583, 584, 585,
    586, 587, 588, 589, 590, 591, 592, 593, 594, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615,
    616, 617, 618, 619, 620, 621, 622, 622, 623, 624, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 639, 640, 641, 642,
    643, 644, 645, 646, 647, 648, 649, 649, 650, 651, 652, 653, 654, 655, 656, 657, 658, 658, 659, 660, 661, 662, 663, 664, 664, 665, 666, 667, 668,
    669, 670, 671, 671, 672, 673, 674, 675, 676, 676, 677, 678, 679, 680, 681, 681, 682, 683, 684, 685, 686, 686, 687, 688, 689, 690, 690, 691, 692,
    693, 694, 694, 695, 696, 697, 698, 698, 699, 700, 701, 702, 702, 703, 704, 705, 706, 706, 707, 708, 709, 709, 710, 711, 712, 713, 713, 714, 715,
    716, 716, 717, 718, 719, 719, 720, 721, 722, 722, 723, 724, 725, 725, 726, 727, 728, 728, 729, 730, 730, 731, 732, 733, 733, 734, 735, 736, 736,
    737, 738, 738, 739, 740, 741, 741, 742, 743, 743, 744, 745, 745, 746, 747, 748, 748, 749, 750, 750, 751, 752, 752, 753, 754, 755, 755, 756, 757,
    757, 758, 759, 759, 760, 761, 761, 762, 763, 763, 764, 765, 765, 766, 767, 767, 768, 769, 769, 770, 771, 771, 772, 773, 773, 774, 775, 775, 776,
    777, 777, 778, 778, 779, 780, 780, 781, 782, 782, 783, 784, 784, 785, 785, 786, 787, 787, 788, 789, 789, 790, 791, 791, 792, 792, 793, 794, 794,
    795, 795, 796, 797, 797, 798, 799, 799, 800, 800, 801, 802, 802, 803, 803, 804, 805, 805, 806, 806, 807, 808, 808, 809, 809, 810, 811, 811, 812,
    812, 813, 814, 814, 815, 815, 816, 816, 817, 818, 818, 819, 819, 820, 821, 821, 822, 822, 823, 823, 824, 825, 825, 826, 826, 827, 827, 828, 829,
    829, 830, 830, 831, 831, 832, 832, 833, 834, 834, 835, 835, 836, 836, 837, 837, 838, 839, 839, 840, 840, 841, 841, 842, 842, 843, 843, 844, 845,
    845, 846, 846, 847, 847, 848, 848, 849, 849, 850, 850, 851, 852, 852, 853, 853, 854, 854, 855, 855, 856, 856, 857, 857, 858, 858, 859, 859, 860,
    860, 861, 861, 862, 863, 863, 864, 864, 865, 865, 866, 866, 867, 867, 868, 868, 869, 869, 870, 870, 871, 871, 872, 872, 873, 873, 874, 874, 875,
    875, 876, 876, 877, 877, 878, 878, 879, 879, 880, 880, 881, 881, 882, 882, 883, 883, 884, 884, 885, 885, 886, 886, 886, 887, 887, 888, 888, 889,
    889, 890, 890, 891, 891, 892, 892, 893, 893, 894, 894, 895, 895, 896, 896, 896, 897, 897, 898, 898, 899, 899, 900, 900, 901, 901, 902, 902, 903,
    903, 903, 904, 904, 905, 905, 906, 906, 907, 907, 908, 908, 909, 909, 909, 910, 910, 911, 911, 912, 912, 913, 913, 913, 914, 914, 915, 915, 916,
    916, 917, 917, 918, 918, 918, 919, 919, 920, 920, 921, 921, 921, 922, 922, 923, 923, 924, 924, 925, 925, 925, 926, 926, 927, 927, 928, 928, 928,
    929, 929, 930, 930, 931, 931, 931, 932, 932, 933, 933, 934, 934, 934, 935, 935, 936, 936, 937, 937, 937, 938, 938, 939, 939, 939, 940, 940, 941,
    941, 942, 942, 942, 943, 943, 944, 944, 944, 945, 945, 946, 946, 946, 947, 947, 948, 948, 949, 949, 949, 950, 950, 951, 951, 951, 952, 952, 953,
    953, 953, 954, 954, 955, 955, 955, 956, 956, 957, 957, 957, 958, 958, 959, 959, 959, 960, 960, 961, 961, 961, 962, 962, 962, 963, 963, 964, 964,
    964, 965, 965, 966, 966, 966, 967, 967, 968, 968, 968, 969, 969, 969, 970, 970, 971, 971, 971, 972, 972, 972, 973, 973, 974, 974, 974, 975, 975,
    976, 976, 976, 977, 977, 977, 978, 978, 979, 979, 979, 980, 980, 980, 981, 981, 981, 982, 982, 983, 983, 983, 984, 984, 984, 985, 985, 986, 986,
    986, 987, 987, 987, 988, 988, 988, 989, 989, 990, 990, 990, 991, 991, 991, 992, 992, 992, 993, 993, 994, 994, 994, 995, 995, 995, 996, 996, 996,
    997, 997, 997, 998, 998, 998, 999, 999, 1000, 1000, 1000, 1001, 1001, 1001, 1002, 1002, 1002, 1003, 1003, 1003, 1004, 1004, 1004, 1005, 1005, 1005,
    1006, 1006, 1006, 1007, 1007, 1008, 1008, 1008, 1009, 1009, 1009, 1010, 1010, 1010, 1011, 1011, 1011, 1012, 1012, 1012, 1013, 1013, 1013, 1014, 1014,
    1014, 1015, 1015, 1015, 1016, 1016
  };

/********************** the curve was generated with the following C program **********************
#include <stdio.h>
#include <math.h>
int main(int argc, const char* argv[]) {
  for (int z = 0; z <= 1016; ++z) {
    double v = pow(z/1016., 0.75) * 1016.;
    v = (2032 * v) / (1016 + v);
    printf("%d, ", (int)round(v));
  }
  printf("\n");
}
**************************************************************************************************/
const short Z_CURVE_LOW[1017] = {
    0, 11, 19, 25, 31, 37, 42, 47, 52, 57, 62, 66, 70, 74, 79, 83, 86, 90, 94, 98, 101, 105, 109, 112, 115, 119, 122, 125, 129, 132, 135, 138, 141,
    144, 147, 150, 153, 156, 159, 162, 165, 168, 171, 173, 176, 179, 182, 184, 187, 190, 192, 195, 197, 200, 203, 205, 208, 210, 212, 215, 217, 220,
    222, 225, 227, 229, 232, 234, 236, 239, 241, 243, 245, 248, 250, 252, 254, 256, 259, 261, 263, 265, 267, 269, 271, 274, 276, 278, 280, 282, 284,
    286, 288, 290, 292, 294, 296, 298, 300, 302, 304, 306, 308, 309, 311, 313, 315, 317, 319, 321, 323, 324, 326, 328, 330, 332, 334, 335, 337, 339,
    341, 343, 344, 346, 348, 350, 351, 353, 355, 356, 358, 360, 361, 363, 365, 367, 368, 370, 372, 373, 375, 376, 378, 380, 381, 383, 385, 386, 388,
    389, 391, 392, 394, 396, 397, 399, 400, 402, 403, 405, 406, 408, 409, 411, 412, 414, 415, 417, 418, 420, 421, 423, 424, 426, 427, 429, 430, 432,
    433, 434, 436, 437, 439, 440, 442, 443, 444, 446, 447, 449, 450, 451, 453, 454, 455, 457, 458, 459, 461, 462, 464, 465, 466, 468, 469, 470, 472,
    473, 474, 475, 477, 478, 479, 481, 482, 483, 485, 486, 487, 488, 490, 491, 492, 493, 495, 496, 497, 498, 500, 501, 502, 503, 505, 506, 507, 508,
    509, 511, 512, 513, 514, 515, 517, 518, 519, 520, 521, 523, 524, 525, 526, 527, 528, 530, 531, 532, 533, 534, 535, 537, 538, 539, 540, 541, 542,
    543, 544, 546, 547, 548, 549, 550, 551, 552, 553, 554, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 569, 570, 571, 572, 573, 574,
    575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 599, 600, 601, 602, 603,
    604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 627, 628, 629, 630, 631,
    632, 633, 634, 635, 636, 637, 638, 639, 640, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 649, 650, 651, 652, 653, 654, 655, 656, 657, 657,
    658, 659, 660, 661, 662, 663, 664, 664, 665, 666, 667, 668, 669, 670, 670, 671, 672, 673, 674, 675, 675, 676, 677, 678, 679, 680, 681, 681, 682,
    683, 684, 685, 685, 686, 687, 688, 689, 690, 690, 691, 692, 693, 694, 694, 695, 696, 697, 698, 698, 699, 700, 701, 702, 702, 703, 704, 705, 706,
    706, 707, 708, 709, 710, 710, 711, 712, 713, 713, 714, 715, 716, 717, 717, 718, 719, 720, 720, 721, 722, 723, 723, 724, 725, 726, 726, 727, 728,
    729, 729, 730, 731, 732, 732, 733, 734, 735, 735, 736, 737, 738, 738, 739, 740, 741, 741, 742, 743, 743, 744, 745, 746, 746, 747, 748, 748, 749,
    750, 751, 751, 752, 753, 753, 754, 755, 756, 756, 757, 758, 758, 759, 760, 760, 761, 762, 763, 763, 764, 765, 765, 766, 767, 767, 768, 769, 769,
    770, 771, 772, 772, 773, 774, 774, 775, 776, 776, 777, 778, 778, 779, 780, 780, 781, 782, 782, 783, 784, 784, 785, 786, 786, 787, 788, 788, 789,
    789, 790, 791, 791, 792, 793, 793, 794, 795, 795, 796, 797, 797, 798, 799, 799, 800, 800, 801, 802, 802, 803, 804, 804, 805, 806, 806, 807, 807,
    808, 809, 809, 810, 811, 811, 812, 812, 813, 814, 814, 815, 815, 816, 817, 817, 818, 819, 819, 820, 820, 821, 822, 822, 823, 823, 824, 825, 825,
    826, 826, 827, 828, 828, 829, 829, 830, 831, 831, 832, 832, 833, 833, 834, 835, 835, 836, 836, 837, 838, 838, 839, 839, 840, 841, 841, 842, 842,
    843, 843, 844, 845, 845, 846, 846, 847, 847, 848, 849, 849, 850, 850, 851, 851, 852, 852, 853, 854, 854, 855, 855, 856, 856, 857, 858, 858, 859,
    859, 860, 860, 861, 861, 862, 863, 863, 864, 864, 865, 865, 866, 866, 867, 867, 868, 869, 869, 870, 870, 871, 871, 872, 872, 873, 873, 874, 874,
    875, 876, 876, 877, 877, 878, 878, 879, 879, 880, 880, 881, 881, 882, 882, 883, 883, 884, 884, 885, 886, 886, 887, 887, 888, 888, 889, 889, 890,
    890, 891, 891, 892, 892, 893, 893, 894, 894, 895, 895, 896, 896, 897, 897, 898, 898, 899, 899, 900, 900, 901, 901, 902, 902, 903, 903, 904, 904,
    905, 905, 906, 906, 907, 907, 908, 908, 909, 909, 910, 910, 911, 911, 912, 912, 913, 913, 914, 914, 915, 915, 916, 916, 917, 917, 918, 918, 919,
    919, 919, 920, 920, 921, 921, 922, 922, 923, 923, 924, 924, 925, 925, 926, 926, 927, 927, 928, 928, 928, 929, 929, 930, 930, 931, 931, 932, 932,
    933, 933, 934, 934, 935, 935, 935, 936, 936, 937, 937, 938, 938, 939, 939, 940, 940, 940, 941, 941, 942, 942, 943, 943, 944, 944, 945, 945, 945,
    946, 946, 947, 947, 948, 948, 949, 949, 949, 950, 950, 951, 951, 952, 952, 953, 953, 953, 954, 954, 955, 955, 956, 956, 957, 957, 957, 958, 958,
    959, 959, 960, 960, 960, 961, 961, 962, 962, 963, 963, 963, 964, 964, 965, 965, 966, 966, 966, 967, 967, 968, 968, 969, 969, 969, 970, 970, 971,
    971, 972, 972, 972, 973, 973, 974, 974, 974, 975, 975, 976, 976, 977, 977, 977, 978, 978, 979, 979, 979, 980, 980, 981, 981, 981, 982, 982, 983,
    983, 984, 984, 984, 985, 985, 986, 986, 986, 987, 987, 988, 988, 988, 989, 989, 990, 990, 990, 991, 991, 992, 992, 992, 993, 993, 994, 994, 994,
    995, 995, 996, 996, 996, 997, 997, 998, 998, 998, 999, 999, 1000, 1000, 1000, 1001, 1001, 1001, 1002, 1002, 1003, 1003, 1003, 1004, 1004, 1005,
    1005, 1005, 1006, 1006, 1007, 1007, 1007, 1008, 1008, 1008, 1009, 1009, 1010, 1010, 1010, 1011, 1011, 1011, 1012, 1012, 1013, 1013, 1013, 1014,
    1014, 1014, 1015, 1015, 1016, 1016
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

boolean calcVelocity(unsigned short z) {
  if (sensorCell().vcount < VELOCITY_SAMPLES_DBL) {

    // we use the Z values in two passed, so that each sample point in the linear regression
    // algorithm is the maximum of two measurements, this stabilizes the velocity calculation
    if (sensorCell().vcount % 2 == 0) {
      sensorCell().velPreviousZ = z;
      sensorCell().vcount++;
      return false;
    }

    // calculate the maximum of the two pressure samples
    z = max(sensorCell().velPreviousZ, z);
    sensorCell().velPreviousZ = 0;

    // calculate the linear regression sums that are variable with the pressure
    sensorCell().velSumY += z;
    sensorCell().velSumXY += ((sensorCell().vcount >> 1) + VELOCITY_ZERO_POINTS) * z;

    sensorCell().vcount++;

    // when the number of samples are reached, calculate the final velocity
    if (sensorCell().vcount == VELOCITY_SAMPLES_DBL) {
      int scale;
      const short* curve;
      switch (Global.velocitySensitivity) {
        case velocityHigh:
          scale = VELOCITY_SCALE_HIGH;
          curve = Z_CURVE_HIGH;
          break;
        case velocityMedium:
          scale = VELOCITY_SCALE_MEDIUM;
          curve = Z_CURVE_MEDIUM;
          break;
        case velocityLow:
          scale = VELOCITY_SCALE_LOW;
          curve = Z_CURVE_LOW;
          break;
      }
      int sxy = (VELOCITY_N * sensorCell().velSumXY) - VELOCITY_SUMX * sensorCell().velSumY;
      int slope = curve[constrain((scale * sxy) / VELOCITY_SXX, 1, 1016)];

      slope = FXD_TO_INT(fxdMinVelOffset + FXD_MUL(FXD_FROM_INT(slope), fxdMinVelRatio));

      slope = scale1016to127(slope, false);

      sensorCell().velocity = calcPreferredVelocity(slope);

      return true;
    }
  }

  return false;
}

byte calcPreferredVelocity(byte velocity) {
  // determine the preferred velocity based on the sensitivity settings
  if (Global.velocitySensitivity == velocityFixed) {
    return 96;
  }
  else {
    return constrain(velocity, 1, 127);
  }
}

boolean TouchInfo::isCalculatingVelocity() {
  return sensorCell().vcount > 0 && sensorCell().vcount < VELOCITY_SAMPLES_DBL;
}

void TouchInfo::shouldRefreshData() {
  shouldRefreshX = true;
  shouldRefreshY = true;
  shouldRefreshZ = true;
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
      initialReferenceX = FXD_TO_INT(Device.calRows[sensorCol][0].fxdReferenceX);

      // store the initial X position
      initialX = currentCalibratedX;
      
      // if pitch quantize is on, the first X position becomes the center point and considered 0
      quantizationOffsetX = currentCalibratedX - initialReferenceX;
      
      fxdRateX = 0;
      lastMovedX = 0;
      lastValueX = INVALID_DATA;
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

inline boolean TouchInfo::isStableYTouch() {    
  return sensorCell().isMeaningfulTouch() && sensorCell().rawZ() > Device.sensorLoZ + Device.sensorRangeZ / 4;
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

    if (currentRawZ < Device.sensorFeatherZ) {        // if the raw touch is below feather touch, keep 0 for the Z values
      return;
    }

    short usableZ = currentRawZ - Device.sensorLoZ;   // subtract minimum from value

    if (usableZ <= 0) {                               // if it's below the acceptable minimum, store it as a feather touch
      featherTouch = true;
      return;
    }

    // the control switches always have maximum velocity and pressure
    if (sensorCol == 0) {
      velocityZ = 127;
      pressureZ = 127;
      return;
    }

    // calculate the velocity and pressure for the playing cells
    unsigned short sensorRange = constrain(Device.sensorRangeZ + 127, 3 * 127, MAX_SENSOR_RANGE_Z - 127);

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

    unsigned short usableVelocityZ = constrain(usableZ, 1, sensorRangeVelocity);
    unsigned short usablePressureZ;
    if (Global.pressureAftertouch) {
        sensorRangePressure /= 5;
        usablePressureZ = constrain(usableZ - (sensorRangeVelocity - sensorRangePressure), 0, sensorRangePressure);
    }
    else {
        usablePressureZ = constrain(usableZ, 1, sensorRangePressure);
    }

    int32_t fxd_usableVelocityZ = FXD_MUL(FXD_FROM_INT(usableVelocityZ), FXD_DIV(FXD_FROM_INT(MAX_SENSOR_RANGE_Z), FXD_FROM_INT(sensorRangeVelocity)));
    int32_t fxd_usablePressureZ = FXD_MUL(FXD_FROM_INT(usablePressureZ), FXD_DIV(FXD_FROM_INT(MAX_SENSOR_RANGE_Z), FXD_FROM_INT(sensorRangePressure)));

    // apply the sensitivity curve
    usableVelocityZ = FXD_TO_INT(fxd_usableVelocityZ);
    usablePressureZ = FXD_TO_INT(fxd_usablePressureZ);

    // scale the result and store it as a byte in the range 0-127
    velocityZ = scale1016to127(usableVelocityZ, false);
    pressureZ = scale1016to127(usablePressureZ, true);
  }
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

void TouchInfo::clearMusicalData() {
  note = -1;
  channel = -1;
  octaveOffset = 0;
  fxdPrevPressure = 0;
  fxdPrevTimbre = FXD_CONST_255;
}

void TouchInfo::clearSensorData() {
  initialX = -1;
  initialReferenceX = 0;
  quantizationOffsetX = 0;
  currentRawX = 0;
  currentCalibratedX = 0;
  lastMovedX = 0;
  lastValueX = INVALID_DATA;
  fxdRateX = 0;
  fxdRateCountX = 0;
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
  velPreviousZ = 0;
  velSumY = 0;
  velSumXY = 0;
}

boolean VirtualTouchInfo::hasNote() {
  return note != -1 && channel != -1;
}

void VirtualTouchInfo::clearData() {
  split = 0;
  note = -1;
  channel = -1;
}