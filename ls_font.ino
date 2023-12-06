/********************************* ls_font: LinnStrument LED Font Drawing *************************
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
These routines allow text to be drawn and scrolled using the LinnStrument LEDS.

Several sized of fonts are defined to take up a different amount of space on the LinnStrument
surface.
**************************************************************************************************/

struct Character {
  byte width;
  const char* data;
};

/******************************************* Tiny Font *******************************************/

static Character tiny_blank = { 2,
  "  "
  "  "
  "  "
  "  "  };

static Character tiny_0 = { 3,
  "000"
  "0 0"
  "0 0"
  "000" };

static Character tiny_1 = { 2,
  " 0"
  "00"
  " 0"
  " 0" };

static Character tiny_2 = { 3,
  "000"
  "  0"
  "00 "
  "000" };

static Character tiny_3 = { 3,
  "000"
  " 0 "
  "  0"
  "00 " };

static Character tiny_4 = { 3,
  "0 0"
  "0 0"
  "000"
  "  0" };

static Character tiny_5 = { 3,
  "000"
  "00 "
  "  0"
  "00 " };

static Character tiny_6 = { 3,
  "0  "
  "000"
  "0 0"
  "000" };

static Character tiny_7 = { 3,
  "000"
  "  0"
  " 0 "
  "0  " };

static Character tiny_8 = { 3,
  "000"
  "000"
  "0 0"
  "000" };

static Character tiny_9 = { 3,
  "000"
  "0 0"
  "000"
  "  0" };

/******************************************* Small Font ******************************************/

static Character small_blank = { 2,
  "  "
  "  "
  "  "
  "  "
  "  "
  "  "
  "  "
  "  "  };

static Character small_excl = { 3,
  "   "
  "   "
  " 0 "
  " 0 "
  " 0 "
  "   "
  " 0 "
  "   " };

static Character small_quot = { 2,
  "  "
  "  "
  "00"
  "00"
  "  "
  "  "
  "  "
  "  " };

static Character small_hash = { 5,
  "     "
  "     "
  " 0 0 "
  "00000"
  " 0 0 "
  "00000"
  " 0 0 "
  "     " };

static Character small_dollar = { 5,
  "     "
  "     "
  " 000 "
  "0 0  "
  " 000 "
  "  0 0"
  " 000 "
  "     " };

static Character small_perc = { 5,
  "     "
  "     "
  " 0   "
  "   0 "
  "  0  "
  " 0   "
  "   0 "
  "     " };

static Character small_amp = { 5,
  "     "
  "     "
  " 00  "
  "0  0 "
  " 00  "
  "0  00"
  " 00 0"
  "     " };

static Character small_squot = { 3,
  "   "
  "   "
  " 0 "
  " 0 "
  "   "
  "   "
  "   "
  "   " };

static Character small_lparen = { 2,
  "  "
  "  "
  " 0"
  "0 "
  "0 "
  "0 "
  " 0"
  "  " };

static Character small_rparen = { 2,
  "  "
  "  "
  "0 "
  " 0"
  " 0"
  " 0"
  "0 "
  "  " };

static Character small_mult = { 5,
  "     "
  "     "
  "0 0 0"
  " 000 "
  "00000"
  " 000 "
  "0 0 0"
  "     " };

static Character small_plus = { 3,
  "   "
  "   "
  "   "
  " 0 "
  "000"
  " 0 "
  "   "
  "   " };

static Character small_comma = { 2,
  "  "
  "  "
  "  "
  "  "
  "  "
  " 0"
  " 0"
  "0 " };

static Character small_minus = { 3,
  "   "
  "   "
  "   "
  "   "
  "000"
  "   "
  "   "
  "   " };

static Character small_dot = { 1,
  " "
  " "
  " "
  " "
  " "
  " "
  "0"
  " " };

static Character small_div = { 3,
  "   "
  "   "
  "   "
  "  0"
  " 0 "
  "0  "
  "   "
  "   " };

static Character small_0 = { 4,
  "    "
  " 00 "
  "0  0"
  "0  0"
  "0  0"
  "0  0"
  " 00 "
  "    " };

static Character small_1 = { 2,
  "  "
  " 0"
  "00"
  " 0"
  " 0"
  " 0"
  " 0"
  "  " };

static Character small_2 = { 4,
  "    "
  " 00 "
  "0  0"
  "  0 "
  " 0  "
  "0   "
  "0000"
  "    " };

static Character small_3 = { 4,
  "    "
  "000 "
  "   0"
  " 00 "
  "   0"
  "   0"
  "000 "
  "    " };

static Character small_4 = { 4,
  "    "
  "  00"
  " 0 0"
  "0  0"
  "0000"
  "   0"
  "   0"
  "    " };

static Character small_5 = { 4,
  "    "
  "0000"
  "0   "
  "000 "
  "   0"
  "0  0"
  " 00 "
  "    " };

static Character small_6 = { 4,
  "    "
  " 000"
  "0   "
  "000 "
  "0  0"
  "0  0"
  " 00 "
  "    " };

static Character small_7 = { 4,
  "    "
  "0000"
  "   0"
  "   0"
  "  0 "
  " 0  "
  "0   "
  "    " };

static Character small_8 = { 4,
  "    "
  " 00 "
  "0  0"
  " 00 "
  "0  0"
  "0  0"
  " 00 "
  "    " };

static Character small_9 = { 4,
  "    "
  " 00 "
  "0  0"
  " 000"
  "   0"
  "  0 "
  " 0  "
  "    " };

static Character small_colon = { 2,
  "  "
  "  "
  "  "
  "0 "
  "  "
  "0 "
  "  "
  "  " };

static Character small_semi = { 2,
  "  "
  "  "
  "  "
  "  "
  " 0"
  "  "
  " 0"
  "0 " };

static Character small_lt = { 3,
  "   "
  "   "
  "  0"
  " 0 "
  "0  "
  " 0 "
  "  0"
  "   " };

static Character small_eq = { 3,
  "   "
  "   "
  "   "
  "000"
  "   "
  "000"
  "   "
  "   " };

static Character small_gt = { 3,
  "   "
  "   "
  "0  "
  " 0 "
  "  0"
  " 0 "
  "0  "
  "   " };

static Character small_quest = { 3,
  "   "
  "   "
  "00 "
  "  0"
  " 0 "
  "   "
  " 0 "
  "   " };

static Character small_at = { 5,
  "     "
  "     "
  " 000 "
  "0 0 0"
  "0  0 "
  "0    "
  " 000 "
  "     " };

static Character small_A = { 4,
  "    "
  "    "
  " 00 "
  "0  0"
  "0000"
  "0  0"
  "0  0"
  "    " };

static Character small_B = { 4,
  "    "
  "    "
  "000 "
  "0  0"
  "000 "
  "0  0"
  "000 "
  "    " };

static Character small_C = { 4,
  "    "
  "    "
  " 000"
  "0   "
  "0   "
  "0   "
  " 000"
  "    " };

static Character small_D = { 4,
  "    "
  "    "
  "000 "
  "0  0"
  "0  0"
  "0  0"
  "000 "
  "    " };

static Character small_E = { 3,
  "   "
  "   "
  "000"
  "0  "
  "00 "
  "0  "
  "000"
  "   " };

static Character small_F = { 3,
  "   "
  "   "
  "000"
  "0  "
  "00 "
  "0  "
  "0  "
  "   " };

static Character small_G = { 4,
  "    "
  "    "
  " 00 "
  "0   "
  "0 00"
  "0  0"
  " 00 "
  "    " };

static Character small_H = { 4,
  "    "
  "    "
  "0  0"
  "0  0"
  "0000"
  "0  0"
  "0  0"
  "    " };

static Character small_I = { 3,
  "   "
  "   "
  "000"
  " 0 "
  " 0 "
  " 0 "
  "000"
  "   " };

static Character small_J = { 3,
  "   "
  "   "
  "000"
  "  0"
  "  0"
  "  0"
  "00 "
  "   " };

static Character small_K = { 4,
  "    "
  "    "
  "0  0"
  "0 0 "
  "00  "
  "0 0 "
  "0  0"
  "    " };

static Character small_L = { 3,
  "   "
  "   "
  "0  "
  "0  "
  "0  "
  "0  "
  "000"
  "   " };

static Character small_M = { 5,
  "     "
  "     "
  "0   0"
  "00 00"
  "0 0 0"
  "0   0"
  "0   0"
  "     " };

static Character small_N = { 4,
  "    "
  "    "
  "0  0"
  "00 0"
  "0 00"
  "0  0"
  "0  0"
  "    " };

static Character small_O = { 4,
  "    "
  "    "
  " 00 "
  "0  0"
  "0  0"
  "0  0"
  " 00 "
  "    " };

static Character small_P = { 4,
  "    "
  "    "
  "000 "
  "0  0"
  "000 "
  "0   "
  "0   "
  "    " };

static Character small_Q = { 4,
  "     "
  "     "
  " 00  "
  "0  0 "
  "0 00 "
  "0  0 "
  " 00 0"
  "     " };

static Character small_R = { 4,
  "    "
  "    "
  "000 "
  "0  0"
  "000 "
  "0 0 "
  "0  0"
  "    " };

static Character small_S = { 4,
  "    "
  "    "
  " 000"
  "0   "
  " 00 "
  "   0"
  "000 "
  "    " };

static Character small_T = { 3,
  "   "
  "   "
  "000"
  " 0 "
  " 0 "
  " 0 "
  " 0 "
  "   " };

static Character small_t = { 3,
  "   "
  "   "
  "0  "
  "000"
  "0  "
  "0  "
  " 00"
  "   " };

static Character small_U = { 4,
  "    "
  "    "
  "0  0"
  "0  0"
  "0  0"
  "0  0"
  " 00 "
  "    " };

static Character small_V = { 5,
  "     "
  "     "
  "0   0"
  "0   0"
  "0   0"
  " 0 0 "
  "  0  "
  "     " };

static Character small_W = { 5,
  "     "
  "     "
  "0   0"
  "0   0"
  "0 0 0"
  "00 00"
  "0   0"
  "     " };

static Character small_X = { 5,
  "     "
  "     "
  "0   0"
  " 0 0 "
  "  0  "
  " 0 0 "
  "0   0"
  "     " };

static Character small_Y = { 5,
  "     "
  "     "
  "0   0"
  " 0 0 "
  "  0  "
  "  0  "
  "  0  "
  "     " };

static Character small_Z = { 4,
  "    "
  "    "
  "0000"
  "   0"
  "  0 "
  " 0  "
  "0000"
  "    " };

static Character small_lsqbrack = { 2,
  "  "
  "  "
  "00"
  "0 "
  "0 "
  "0 "
  "00"
  "  " };

static Character small_backslash = { 3,
  "   "
  "   "
  "   "
  "0  "
  " 0 "
  "  0"
  "   "
  "   " };

static Character small_rsqbrack =  { 3,
  "  "
  "  "
  "00"
  " 0"
  " 0"
  " 0"
  "00"
  "  " };

static Character small_pow = { 3,
  "   "
  "   "
  " 0 "
  "0 0"
  "   "
  "   "
  "   "
  "   " };

static Character small_under = { 3,
  "   "
  "   "
  "   "
  "   "
  "   "
  "   "
  "000"
  "   " };

static Character small_backtick = { 3,
  "  "
  "  "
  "0 "
  " 0"
  "  "
  "  "
  "  "
  "  " };

static Character small_lbrace = { 3,
  "   "
  "   "
  " 00"
  " 0 "
  "0  "
  " 0 "
  " 00"
  "   " };

static Character small_pipe = { 1,
  " "
  " "
  "0"
  "0"
  "0"
  "0"
  "0"
  " " };

static Character small_rbrace = { 3,
  "   "
  "   "
  "00 "
  " 0 "
  "  0"
  " 0 "
  "00 "
  "   " };

static Character small_tilde = { 5,
  "     "
  "     "
  "     "
  " 0   "
  "0 0 0"
  "   0 "
  "     "
  "     " };

/******************************************** Big Font *******************************************/

static Character big_blank = { 5,
  "     "
  "     "
  "     "
  "     "
  "     "
  "     "
  "     "
  "     " };

static Character big_excl = { 5,
  "     "
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "     "
  "  0  "
  "     " };

static Character big_quot = { 5,
  "     "
  " 00  "
  " 00  "
  "  00 "
  "     "
  "     "
  "     "
  "     " };

static Character big_hash = { 5,
  "     "
  "     "
  " 0 0 "
  "00000"
  " 0 0 "
  "00000"
  " 0 0 "
  "     " };

static Character big_dollar = { 5,
  "     "
  "  0  "
  " 0000"
  "0 0  "
  " 000 "
  "  0 0"
  "0000 "
  "  0  " };

static Character big_perc = { 5,
  "     "
  "     "
  " 0  0"
  "   0 "
  "  0  "
  " 0   "
  "0  0 "
  "     " };

static Character big_amp = { 5,
  "     "
  "     "
  " 00  "
  "0  0 "
  " 00  "
  "0  00"
  " 00 0"
  "     " };

static Character big_squot = { 5,
  "     "
  "  0  "
  "  0  "
  "   0 "
  "     "
  "     "
  "     "
  "     " };

static Character big_lparen = { 5,
  "     "
  "  00 "
  " 0   "
  " 0   "
  " 0   "
  " 0   "
  "  00 "
  "     " };

static Character big_rparen = { 5,
  "     "
  " 00  "
  "   0 "
  "   0 "
  "   0 "
  "   0 "
  " 00  "
  "     " };

static Character big_mult = { 5,
  "     "
  "     "
  "0 0 0"
  " 000 "
  "00000"
  " 000 "
  "0 0 0"
  "     " };

static Character big_plus = { 5,
  "     "
  "     "
  "  0  "
  "  0  "
  "00000"
  "  0  "
  "  0  "
  "     " };

static Character big_comma = { 5,
  "     "
  "     "
  "     "
  "     "
  "     "
  "  0  "
  "  0  "
  " 0   " };

static Character big_minus = { 5,
  "     "
  "     "
  "     "
  "     "
  "00000"
  "     "
  "     "
  "     " };

static Character big_dot = { 5,
  "     "
  "     "
  "     "
  "     "
  "     "
  "     "
  "  0  "
  "     " };

static Character big_div = { 5,
  "     "
  "     "
  "    0"
  "   0 "
  "  0  "
  " 0   "
  "0    "
  "     " };

static Character big_0 = { 5,
  "     "
  " 000 "
  "0   0"
  "0  00"
  "0 0 0"
  "00  0"
  " 000 "
  "     " };

static Character big_1 = { 5,
  "     "
  "  0  "
  " 00  "
  "0 0  "
  "  0  "
  "  0  "
  "00000"
  "     " };

static Character big_2 = { 5,
  "     "
  " 000 "
  "0   0"
  "   0 "
  "  0  "
  " 0   "
  "00000"
  "     " };

static Character big_3 = { 5,
  "     "
  " 000 "
  "0   0"
  "  00 "
  "    0"
  "0   0"
  " 000 "
  "     " };

static Character big_4 = { 5,
  "     "
  "   00"
  "  0 0"
  " 0  0"
  "00000"
  "    0"
  "    0"
  "     " };

static Character big_5 = { 5,
  "     "
  "00000"
  "0    "
  "0000 "
  "    0"
  "0   0"
  " 000 "
  "     " };

static Character big_6 = { 5,
  "     "
  " 000 "
  "0    "
  "0000 "
  "0   0"
  "0   0"
  " 000 "
  "     " };

static Character big_7 = { 5,
  "     "
  "00000"
  "    0"
  "   0 "
  "  0  "
  " 0   "
  "0    "
  "     " };

static Character big_8 = { 5,
  "     "
  " 000 "
  "0   0"
  " 000 "
  "0   0"
  "0   0"
  " 000 "
  "     " };

static Character big_9 = { 5,
  "     "
  " 000 "
  "0   0"
  "0   0"
  " 0000"
  "    0"
  " 000 "
  "     " };

static Character big_colon = { 5,
  "     "
  "     "
  "     "
  "  0  "
  "     "
  "  0  "
  "     "
  "     " };

static Character big_semi = { 5,
  "     "
  "     "
  "     "
  "  0  "
  "     "
  "  0  "
  "  0  "
  " 0   " };

static Character big_lt = { 5,
  "     "
  "     "
  "   0 "
  "  0  "
  " 0   "
  "  0  "
  "   0 "
  "     " };

static Character big_eq = { 5,
  "     "
  "     "
  "     "
  "00000"
  "     "
  "00000"
  "     "
  "     " };

static Character big_gt = { 5,
  "     "
  "     "
  " 0   "
  "  0  "
  "   0 "
  "  0  "
  " 0   "
  "     " };

static Character big_quest = { 5,
  "     "
  " 000 "
  "0   0"
  "   0 "
  "  0  "
  "     "
  "  0  "
  "     " };

static Character big_at = { 5,
  "     "
  "     "
  " 000 "
  "0 0 0"
  "0  0 "
  "0    "
  " 000 "
  "     " };

static Character big_A = { 5,
  "     "
  " 000 "
  "0   0"
  "0   0"
  "00000"
  "0   0"
  "0   0"
  "     " };

static Character big_B = { 5,
  "     "
  "0000 "
  "0   0"
  "0000 "
  "0   0"
  "0   0"
  "0000 "
  "     " };

static Character big_C = { 5,
  "     "
  " 000 "
  "0   0"
  "0    "
  "0    "
  "0   0"
  " 000 "
  "     " };

static Character big_D = { 5,
  "     "
  "0000 "
  "0   0"
  "0   0"
  "0   0"
  "0   0"
  "0000 "
  "     " };

static Character big_E = { 5,
  "     "
  "00000"
  "0    "
  "0000 "
  "0    "
  "0    "
  "00000"
  "     " };

static Character big_F = { 5,
  "     "
  "00000"
  "0    "
  "0000 "
  "0    "
  "0    "
  "0    "
  "     " };

static Character big_G = { 5,
  "     "
  " 000 "
  "0   0"
  "0    "
  "0 000"
  "0   0"
  " 000 "
  "     " };

static Character big_H = { 5,
  "     "
  "0   0"
  "0   0"
  "00000"
  "0   0"
  "0   0"
  "0   0"
  "     " };

static Character big_I = { 5,
  "     "
  "00000"
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "00000"
  "     " };

static Character big_J = { 5,
  "     "
  "    0"
  "    0"
  "    0"
  "0   0"
  "0   0"
  " 000 "
  "     " };

static Character big_K = { 5,
  "     "
  "0   0"
  "0  0 "
  "000  "
  "0 0  "
  "0  0 "
  "0   0"
  "     " };

static Character big_L = { 5,
  "     "
  "0    "
  "0    "
  "0    "
  "0    "
  "0    "
  "00000"
  "     " };

static Character big_M = { 5,
  "     "
  "0   0"
  "00 00"
  "0 0 0"
  "0   0"
  "0   0"
  "0   0"
  "     " };

static Character big_N = { 5,
  "     "
  "0   0"
  "00  0"
  "0 0 0"
  "0  00"
  "0   0"
  "0   0"
  "     " };

static Character big_O = { 5,
  "     "
  " 000 "
  "0   0"
  "0   0"
  "0   0"
  "0   0"
  " 000 "
  "     " };

static Character big_P = { 5,
  "     "
  "0000 "
  "0   0"
  "0   0"
  "0000 "
  "0    "
  "0    "
  "     " };

static Character big_Q = { 5,
  "     "
  " 000 "
  "0   0"
  "0   0"
  "0 0 0"
  "0  0 "
  " 00 0"
  "     " };

static Character big_R = { 5,
  "     "
  "0000 "
  "0   0"
  "0   0"
  "0000 "
  "0  0 "
  "0   0"
  "     " };

static Character big_S = { 5,
  "     "
  " 0000"
  "0    "
  " 000 "
  "    0"
  "    0"
  "0000 "
  "     " };

static Character big_T = { 5,
  "     "
  "00000"
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "     " };

static Character big_U = { 5,
  "     "
  "0   0"
  "0   0"
  "0   0"
  "0   0"
  "0   0"
  " 000 "
  "     " };

static Character big_V = { 5,
  "     "
  "0   0"
  "0   0"
  "0   0"
  "0   0"
  " 0 0 "
  "  0  "
  "     " };

static Character big_W = { 5,
  "     "
  "0   0"
  "0   0"
  "0   0"
  "0 0 0"
  "0 0 0"
  " 0 0 "
  "     " };

static Character big_X = { 5,
  "     "
  "0   0"
  " 0 0 "
  "  0  "
  "  0  "
  " 0 0 "
  "0   0"
  "     " };

static Character big_Y = { 5,
  "     "
  "0   0"
  " 0 0 "
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "     " };

static Character big_Z = { 5,
  "     "
  "00000"
  "   0 "
  "  0  "
  " 0   "
  "0    "
  "00000"
  "     " };

static Character big_lsqbrack = { 5,
  "     "
  " 000 "
  " 0   "
  " 0   "
  " 0   "
  " 0   "
  " 000 "
  "     " };

static Character big_backslash = { 5,
  "     "
  "     "
  "0    "
  " 0   "
  "  0  "
  "   0 "
  "    0"
  "     " };

static Character big_rsqbrack =  { 5,
  "     "
  " 000 "
  "   0 "
  "   0 "
  "   0 "
  "   0 "
  " 000 "
  "     " };

static Character big_pow = { 5,
  "     "
  "  0  "
  " 0 0 "
  "0   0"
  "     "
  "     "
  "     "
  "     " };

static Character big_under = { 5,
  "     "
  "     "
  "     "
  "     "
  "     "
  "     "
  "     "
  "00000" };

static Character big_backtick = { 5,
  "     "
  " 0   "
  "  0  "
  "     "
  "     "
  "     "
  "     "
  "     " };

static Character big_a = { 5,
  "     "
  "     "
  "     "
  " 000 "
  "0  0 "
  "0  0 "
  " 0000"
  "     " };

static Character big_b = { 5,
  "     "
  "0    "
  "0    "
  "0000 "
  "0   0"
  "0   0"
  "0000 "
  "     " };

static Character big_c = { 5,
  "     "
  "     "
  "     "
  " 0000"
  "0    "
  "0    "
  " 0000"
  "     " };

static Character big_d = { 5,
  "     "
  "    0"
  "    0"
  " 0000"
  "0   0"
  "0   0"
  " 0000"
  "     " };

static Character big_e = { 5,
  "     "
  "     "
  "     "
  " 000 "
  "00000"
  "0    "
  " 000 "
  "     " };

static Character big_f = { 5,
  "     "
  "  00 "
  " 0  0"
  " 0   "
  "0000 "
  " 0   "
  " 0   "
  "     " };

static Character big_g = { 5,
  "     "
  "     "
  " 0000"
  "0   0"
  "0   0"
  " 0000"
  "    0"
  " 000 " };

static Character big_h = { 5,
  "     "
  "0    "
  "0    "
  "0000 "
  "0   0"
  "0   0"
  "0   0"
  "     " };

static Character big_i = { 5,
  "     "
  "  0  "
  "     "
  " 00  "
  "  0  "
  "  0  "
  " 000 "
  "     " };

static Character big_j = { 5,
  "     "
  "   0 "
  "     "
  "  00 "
  "   0 "
  "   0 "
  "0  0 "
  " 00  " };

static Character big_k = { 5,
  "     "
  "0    "
  "0    "
  "0 0  "
  "00   "
  "0 0  "
  "0  0 "
  "     " };

static Character big_l = { 5,
  "     "
  "00   "
  " 0   "
  " 0   "
  " 0   "
  " 0   "
  "  000"
  "     " };

static Character big_m = { 5,
  "     "
  "     "
  "     "
  "0000 "
  "0 0 0"
  "0 0 0"
  "0 0 0"
  "     " };

static Character big_n = { 5,
  "     "
  "     "
  "     "
  "0000 "
  "0   0"
  "0   0"
  "0   0"
  "     " };

static Character big_o = { 5,
  "     "
  "     "
  "     "
  " 000 "
  "0   0"
  "0   0"
  " 000 "
  "     " };

static Character big_p = { 5,
  "     "
  "     "
  "     "
  "0000 "
  "0   0"
  "0   0"
  "0000 "
  "0    " };

static Character big_q = { 5,
  "     "
  "     "
  "     "
  " 0000"
  "0   0"
  "0   0"
  " 0000"
  "    0" };

static Character big_r = { 5,
  "     "
  "     "
  "     "
  "0000 "
  "0   0"
  "0    "
  "0    "
  "     " };

static Character big_s = { 5,
  "     "
  "     "
  " 000 "
  "0    "
  " 000 "
  "    0"
  "0000 "
  "     " };

static Character big_t = { 5,
  "     "
  " 0   "
  " 0   "
  "0000 "
  " 0   "
  " 0  0"
  "  00 "
  "     " };

static Character big_u = { 5,
  "     "
  "     "
  "     "
  "0   0"
  "0   0"
  "0   0"
  " 000 "
  "     " };

static Character big_v = { 5,
  "     "
  "     "
  "     "
  "0   0"
  "0   0"
  " 0 0 "
  "  0  "
  "     " };

static Character big_w = { 5,
  "     "
  "     "
  "     "
  "0   0"
  "0 0 0"
  "0 0 0"
  " 0 0 "
  "     " };

static Character big_x = { 5,
  "     "
  "     "
  "     "
  "0  0 "
  " 00  "
  " 00  "
  "0  0 "
  "     " };

static Character big_y = { 5,
  "     "
  "     "
  "     "
  "0   0"
  " 0 0 "
  "  0  "
  " 0   " };

static Character big_z = { 5,
  "     "
  "     "
  "     "
  "0000 "
  "  0  "
  " 0   "
  "0000 "
  "     " };

static Character big_lbrace = { 5,
  "   00"
  "  0  "
  "  0  "
  " 0   "
  "  0  "
  "  0  "
  "   00"
  "     " };

static Character big_pipe = { 5,
  "     "
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "  0  "
  "     " };

static Character big_rbrace = { 5,
  "00   "
  "  0  "
  "  0  "
  "   0 "
  "  0  "
  "  0  "
  "00   "
  "     " };

static Character big_tilde = { 5,
  "     "
  "     "
  "     "
  " 0   "
  "0 0 0"
  "   0 "
  "     "
  "     " };

/***************************************** Condensed Font ****************************************/

static Character cond_blank = { 3,
  "   "
  "   "
  "   "
  "   "
  "   "
  "   "
  "   "
  "   " };

static Character cond_plus = { 3,
  "   "
  "   "
  "   "
  " 0 "
  "000"
  " 0 "
  "   "
  "   " };

static Character cond_minus = { 2,
  "  "
  "  "
  "  "
  "  "
  "00"
  "  "
  "  "
  "  " };

static Character cond_dot = { 1,
  " "
  " "
  " "
  " "
  " "
  " "
  "0"
  " " };

static Character cond_div = { 3,
  "   "
  "   "
  "   "
  "  0"
  " 0 "
  "0  "
  "   "
  "   " };

static Character cond_0 = { 3,
  "   "
  " 0 "
  "0 0"
  "0 0"
  "0 0"
  "0 0"
  " 0 "
  "   " };

static Character cond_1 = { 2,
  "  "
  " 0"
  "00"
  " 0"
  " 0"
  " 0"
  " 0"
  "  " };

static Character cond_2 = { 3,
  "   "
  " 0 "
  "0 0"
  "  0"
  " 0 "
  "0  "
  "000"
  "   " };

static Character cond_3 = { 3,
  "   "
  "00 "
  "  0"
  " 0 "
  "  0"
  "  0"
  "00 "
  "    " };

static Character cond_4 = { 3,
  "   "
  "0 0"
  "0 0"
  "000"
  "  0"
  "  0"
  "  0"
  "   " };

static Character cond_5 = { 3,
  "   "
  "000"
  "0  "
  "00 "
  "  0"
  "0 0"
  " 0 "
  "     " };

static Character cond_6 = { 3,
  "   "
  " 0 "
  "0  "
  "00 "
  "0 0"
  "0 0"
  " 0 "
  "   " };

static Character cond_7 = { 3,
  "   "
  "000"
  "  0"
  "  0"
  " 0 "
  "0  "
  "0  "
  "   " };

static Character cond_8 = { 3,
  "   "
  " 0 "
  "0 0"
  " 0 "
  "0 0"
  "0 0"
  " 0 "
  "   " };

static Character cond_9 = { 3,
  "   "
  " 0 "
  "0 0"
  "0 0"
  " 00"
  "  0"
  " 0 "
  "   " };

static Character cond_A = { 3,
  "   "
  " 0 "
  "0 0"
  "000"
  "0 0"
  "0 0"
  "0 0"
  "   " };

static Character cond_B = { 3,
  "   "
  "00 "
  "0 0"
  "00 "
  "0 0"
  "0 0"
  "00 "
  "   " };

static Character cond_C = { 3,
  "   "
  " 00"
  "0  "
  "0  "
  "0  "
  "0  "
  " 00"
  "   " };

static Character cond_D = { 3,
  "   "
  "00 "
  "0 0"
  "0 0"
  "0 0"
  "0 0"
  "00 "
  "   " };

static Character cond_E = { 3,
  "   "
  "000"
  "0  "
  "00 "
  "0  "
  "0  "
  "000"
  "   " };

static Character cond_F = { 3,
  "   "
  "000"
  "0  "
  "00 "
  "0  "
  "0  "
  "0  "
  "   " };

static Character cond_G = { 4,
  "    "
  " 00 "
  "0  0"
  "0   "
  "0 00"
  "0  0"
  " 00 "
  "    " };

static Character cond_H = { 3,
  "   "
  "0 0"
  "0 0"
  "000"
  "0 0"
  "0 0"
  "0 0"
  "   " };

static Character cond_I = { 3,
  "   "
  "000"
  " 0 "
  " 0 "
  " 0 "
  " 0 "
  "000"
  "   " };

static Character cond_J = { 3,
  "   "
  "  0"
  "  0"
  "  0"
  "0 0"
  "0 0"
  " 0 "
  "   " };

static Character cond_K = { 4,
  "    "
  "0  0"
  "0 0 "
  "00  "
  "00  "
  "0 0 "
  "0  0"
  "    " };


static Character cond_L = { 3,
  "   "
  "0  "
  "0  "
  "0  "
  "0  "
  "0  "
  "000"
  "   " };

static Character cond_M = { 5,
  "     "
  "0   0"
  "00 00"
  "0 0 0"
  "0   0"
  "0   0"
  "0   0"
  "     " };

static Character cond_N = { 4,
  "    "
  "0  0"
  "00 0"
  "0 00"
  "0  0"
  "0  0"
  "0  0"
  "    " };

static Character cond_O = { 3,
  "   "
  " 0 "
  "0 0"
  "0 0"
  "0 0"
  "0 0"
  " 0 "
  "   " };

static Character cond_P = { 3,
  "   "
  "00 "
  "0 0"
  "0 0"
  "00 "
  "0  "
  "0  "
  "   " };

static Character cond_Q = { 4,
  "    "
  " 00 "
  "0  0"
  "0  0"
  "0  0"
  "0 0 "
  " 0 0"
  "    " };

static Character cond_R = { 3,
  "   "
  "00 "
  "0 0"
  "0 0"
  "00 "
  "00 "
  "0 0"
  "   " };

static Character cond_S = { 3,
  "   "
  " 00"
  "0  "
  " 0 "
  "  0"
  "  0"
  "00 "
  "   " };

static Character cond_T = { 3,
  "   "
  "000"
  " 0 "
  " 0 "
  " 0 "
  " 0 "
  " 0 "
  "   " };

static Character cond_U = { 3,
  "   "
  "0 0"
  "0 0"
  "0 0"
  "0 0"
  "0 0"
  "000"
  "   " };

static Character cond_V = { 3,
  "   "
  "0 0"
  "0 0"
  "0 0"
  "0 0"
  "0 0"
  " 0 "
  "   " };

static Character cond_W = { 5,
  "     "
  "0   0"
  "0   0"
  "0   0"
  "0 0 0"
  "0 0 0"
  " 0 0 "
  "     " };

static Character cond_X = { 3,
  "   "
  "0 0"
  "0 0"
  " 0 "
  " 0 "
  "0 0"
  "0 0"
  "   " };

static Character cond_Y = { 3,
  "   "
  "0 0"
  "0 0"
  " 0 "
  " 0 "
  " 0 "
  " 0 "
  "   " };

static Character cond_Z = { 3,
  "   "
  "000"
  "  0"
  " 0 "
  "0  "
  "0  "
  "000"
  "   " };

static Character cond_b = { 3,
  "   "
  "   "
  "0  "
  "0  "
  "00 "
  "0 0"
  "00 "
  "   " };

struct Font {
  byte height;
  const char* chars;
  Character** data;
};

Character* tinyChars[] = {
  &tiny_blank, &tiny_0, &tiny_1, &tiny_2, &tiny_3, &tiny_4, &tiny_5, &tiny_6, &tiny_7, &tiny_8, &tiny_9
};
struct Font tinyFont = { 4, " 0123456789", tinyChars };

Character* smallChars[] = {
  &small_blank, &small_excl, &small_quot, &small_hash, &small_dollar, &small_perc, &small_amp, &small_squot, &small_lparen, &small_rparen, &small_mult, &small_plus, &small_comma, &small_minus, &small_dot, &small_div,
  &small_0, &small_1, &small_2, &small_3, &small_4, &small_5, &small_6, &small_7, &small_8, &small_9,
  &small_colon, &small_semi, &small_lt, &small_eq, &small_gt, &small_quest, &small_at,
  &small_A, &small_B, &small_C, &small_D, &small_E, &small_F, &small_G, &small_H, &small_I, &small_J, &small_K, &small_L, &small_M, &small_N, &small_O, &small_P, &small_Q, &small_R, &small_S, &small_T, &small_U, &small_V, &small_W, &small_X, &small_Y, &small_Z,
  &small_t,
  &small_lsqbrack, &small_backslash, &small_rsqbrack, &small_pow, &small_under, &small_backtick,
  &small_lbrace, &small_pipe, &small_rbrace, &small_tilde
};
struct Font smallFont = { 8, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZt[\\]^_`{|}~", smallChars };

Character* bigChars[] = {
  &big_blank, &big_excl, &big_quot, &big_hash, &big_dollar, &big_perc, &big_amp, &big_squot, &big_lparen, &big_rparen, &big_mult, &big_plus, &big_comma, &big_minus, &big_dot, &big_div,
  &big_0, &big_1, &big_2, &big_3, &big_4, &big_5, &big_6, &big_7, &big_8, &big_9,
  &big_colon, &big_semi, &big_lt, &big_eq, &big_gt, &big_quest, &big_at,
  &big_A, &big_B, &big_C, &big_D, &big_E, &big_F, &big_G, &big_H, &big_I, &big_J, &big_K, &big_L, &big_M, &big_N, &big_O, &big_P, &big_Q, &big_R, &big_S, &big_T, &big_U, &big_V, &big_W, &big_X, &big_Y, &big_Z,
  &big_lsqbrack, &big_backslash, &big_rsqbrack, &big_pow, &big_under, &big_backtick,
  &big_a, &big_b, &big_c, &big_d, &big_e, &big_f, &big_g, &big_h, &big_i, &big_j, &big_k, &big_l, &big_m, &big_n, &big_o, &big_p, &big_q, &big_r, &big_s, &big_t, &big_u, &big_v, &big_w, &big_x, &big_y, &big_z,
  &big_lbrace, &big_pipe, &big_rbrace, &big_tilde
};
struct Font bigFont = { 8, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", bigChars };

Character* condChars[] = {
  &cond_blank, &cond_plus, &cond_minus, &cond_dot, &cond_div, &cond_0, &cond_1, &cond_2, &cond_3, &cond_4, &cond_5, &cond_6, &cond_7, &cond_8, &cond_9,
  &cond_A, &cond_B, &cond_C, &cond_D, &cond_E, &cond_F, &cond_G, &cond_H, &cond_I, &cond_J, &cond_K, &cond_L, &cond_M, &cond_N, &cond_O, &cond_P, &cond_Q, &cond_R, &cond_S, &cond_T, &cond_U, &cond_V, &cond_W, &cond_X, &cond_Y, &cond_Z,
  &cond_b
};
struct Font condFont = { 8, " +-./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZb", condChars };


unsigned font_width_string(const char* str, struct Font* font) {
  unsigned width = 0;
  char c;
  while ((c=*str++) != 0) {
    char* p = strchr(font->chars,c);
    if (p) {
      width += (font->data[p - font->chars]->width + 1);
    }
  }

  return width;
}

// Draw a string of characters starting at a specific column/row
void font_draw_string(int col, int row, const char* str, byte color, struct Font* font, boolean erase, boolean reversed, byte seperationColor) {
  int i;
  if (reversed) { i = strlen(str) - 1; }
  else          { i = 0; }

  while ((reversed && i >= 0) || (!reversed && i < (int)strlen(str))) {
    char c = str[i];
    char* p = strchr(font->chars, c);
    if (p) {
      int i = p - font->chars;                        // offset into font_chars gives character index
      font_draw_char(col, row, font->data[i]->data, color, font->data[i]->width, font->height, erase, reversed);
      col += (font->data[i]->width + 1);
      if (erase) {
        font_draw_blank_column(col, row, font->height, seperationColor);     // 1 blank pixel between characters
      }
    }

    if (reversed) { --i; }
    else          { ++i; }
    
    performContinuousTasks();
  }
}

void tinyfont_draw_string(int col, int row, const char* str, byte color) {
  tinyfont_draw_string(col, row, str, color, true);
}

void tinyfont_draw_string(int col, int row, const char* str, byte color, boolean erase) {
  tinyfont_draw_string(col, row, str, color, erase, false);
}

void tinyfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed) {
  tinyfont_draw_string(col, row, str, color, erase, reversed, COLOR_OFF);
}

void tinyfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed, byte seperationColor) {
  font_draw_string(col, row, str, color, &tinyFont, erase, reversed, seperationColor);
}

void smallfont_draw_string(int col, int row, const char* str, byte color) {
  smallfont_draw_string(col, row, str, color, true);
}

void smallfont_draw_string(int col, int row, const char* str, byte color, boolean erase) {
  smallfont_draw_string(col, row, str, color, erase, false);
}

void smallfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed) {
  smallfont_draw_string(col, row, str, color, erase, reversed, COLOR_OFF);
}

void smallfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed, byte seperationColor) {
  font_draw_string(col, row, str, color, &smallFont, erase, reversed, seperationColor);
}

void bigfont_draw_string(int col, int row, const char* str, byte color) {
  bigfont_draw_string(col, row, str, color, true);
}

void bigfont_draw_string(int col, int row, const char* str, byte color, boolean erase) {
  bigfont_draw_string(col, row, str, color, erase, false);
}

void bigfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed) {
  bigfont_draw_string(col, row, str, color, erase, reversed, COLOR_OFF);
}

void bigfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed, byte seperationColor) {
  font_draw_string(col, row, str, color, &bigFont, erase, reversed, seperationColor);
}

void condfont_draw_string(int col, int row, const char* str, byte color) {
  condfont_draw_string(col, row, str, color, true);
}

void condfont_draw_string(int col, int row, const char* str, byte color, boolean erase) {
  condfont_draw_string(col, row, str, color, erase, false);
}

void condfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed) {
  condfont_draw_string(col, row, str, color, erase, reversed, COLOR_OFF);
}

void condfont_draw_string(int col, int row, const char* str, byte color, boolean erase, boolean reversed, byte seperationColor) {
  font_draw_string(col, row, str, color, &condFont, erase, reversed, seperationColor);
}

void adaptfont_draw_string(int col, int row, const char* str, byte color) {
  if (LINNMODEL == 200) bigfont_draw_string(col, row, str, color);
  else                  condfont_draw_string(col, row, str, color);
}

void adaptfont_draw_string(int col, int row, const char* str, byte color, boolean erase) {
  if (LINNMODEL == 200) bigfont_draw_string(col, row, str, color, erase);
  else                  condfont_draw_string(col, row, str, color, erase);
}

// Draw a single character at col,row
void font_draw_char(int col, int row, const char* fontdata, byte color, byte width, byte height, boolean erase, boolean reversed)
{
  for (byte r = 0; r < height; ++r) {
    for (byte c = 0; c < width; ++c) {
      char thechar = *fontdata++;

      int destcol;
      int destrow;

      if (reversed) {
        destcol = col + width - c;
        destrow = row + r;
      }
      else {
        destcol = col + c + 1;
        destrow = row + height - 1 - r;
      }

      if (destcol < 1 || destcol >= NUMCOLS || destrow < 0 || destrow > 7) {
        continue;
      }
      if (erase || thechar != ' ') {
        setLed(destcol, destrow, (thechar != ' ' ? color : 0), cellOn);
      }
    }
  }
}

// Draw a single-pixel-wide blank column
void font_draw_blank_column(int col, int row, byte height, byte seperationColor) {
  if (col >= 1 && col < NUMCOLS) {
    for (byte r = row; r < height; ++r) {
      if (r == row || r == height - 1) {
        setLed(col, r, seperationColor, cellOn);
      }
      else {
        setLed(col, r, COLOR_OFF, cellOff);
      }
    }
  }
}

void small_scroll_text_flipped(const char* str, byte color) {
  font_scroll_text_flipped(&smallFont, str, color);
}

void big_scroll_text_flipped(const char* str, byte color) {
  font_scroll_text_flipped(&bigFont, str, color);
}

void font_scroll_text_flipped(struct Font* font, const char* str, byte color) {
  unsigned long origInterval = ledRefreshInterval;
  ledRefreshInterval = 200;

  animationActive = true;
  stopAnimation = false;

  int totalwidth = font_width_string(str, font);
  int i;
  for (i = totalwidth; i >= -NUMCOLS && !stopAnimation; --i) {
    font_draw_string( -i, 0, str, color, font, true, true, COLOR_OFF);

    if (i < 0) {
      for (byte col = 0; col <= -i; ++col) {
        for (byte row = 0; row < font->height; ++row) {
          clearLed(col, row);
        }
      }
    }

    delayUsecWithScanning(40000);
  }

  clearDisplay();

  animationActive = false;

  ledRefreshInterval = origInterval;
}

void small_scroll_text(const char* str, byte color) {
  font_scroll_text(&smallFont, str, color);
}

void big_scroll_text(const char* str, byte color) {
  font_scroll_text(&bigFont, str, color);
}

void font_scroll_text(struct Font* font, const char* str, byte color) {
  unsigned long origInterval = ledRefreshInterval;
  ledRefreshInterval = 200;

  animationActive = true;
  stopAnimation = false;

  int totalwidth = font_width_string(str, font);
  for (int i = 0; i < totalwidth && !stopAnimation; ++i) {
    font_draw_string( -i, 0, str, color, font, true, false, COLOR_OFF);
    delayUsecWithScanning(40000);
  }

  clearDisplay();

  animationActive = false;

  ledRefreshInterval = origInterval;
}
