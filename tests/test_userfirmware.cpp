/*
Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
// User Firmware Mode as documented in user_firmware_mode.md: every claim there has a check here.

static void enterUserFirmwareMode() {
  linn::reset();
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  linn::drainMidi();
  host::clearSerialOutput();
}

// everything the instrument sent since the previous call
static std::vector<uint8_t> drained() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  return out;
}

// counts complete messages of one status byte (channel ignored) in a drained stream
static int countStatus(const std::vector<uint8_t>& out, uint8_t statusHigh) {
  int n = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    if ((out[i] & 0xF0) == statusHigh) ++n;
  }
  return n;
}

TEST(ufm_nrpn_245_enters_and_leaves) {
  linn::reset();
  linn::midiInNrpn(245, 1, 0);
  CHECK(userFirmwareActive);
  linn::midiInNrpn(245, 0, 0);
  CHECK(!userFirmwareActive);
}

TEST(ufm_any_channel_enters) {
  linn::reset();
  linn::midiInNrpn(245, 1, 11);
  CHECK(userFirmwareActive);
}

TEST(ufm_change_is_announced_with_nrpn_245_on_channel_9) {
  linn::reset();
  linn::midiInNrpn(245, 1, 0);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 18);
  if (out.size() == 18) {
    CHECK_EQ(out[0], 0xB8);
    CHECK_EQ(out[1], 99);
    CHECK_EQ(out[2] * 128 + out[5], 245);
    CHECK_EQ(out[8] * 128 + out[11], 1);
  }
}

TEST(ufm_query_reports_the_state) {
  enterUserFirmwareMode();
  linn::midiInNrpn(299, 245, 0);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 18);
  if (out.size() == 18) {
    CHECK_EQ(out[8] * 128 + out[11], 1);
  }
}

TEST(ufm_starts_from_a_clean_slate) {
  linn::reset();
  linn::midiInNrpn(245, 1, 0);
  linn::midiIn({0xB3, 10, 1});
  linn::midiIn({0xB3, 9, 1});
  CHECK(userFirmwareXActive[3]);
  CHECK(userFirmwareSlideMode[3]);
  linn::midiInNrpn(245, 0, 0);
  linn::midiInNrpn(245, 1, 0);
  CHECK(!userFirmwareXActive[3]);
  CHECK(!userFirmwareSlideMode[3]);
}

TEST(ufm_cell_press_is_note_on_with_column_and_row) {
  enterUserFirmwareMode();
  linn::press(5, 3, 2000, 2000, 400);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 3);
  if (out.size() == 3) {
    CHECK_EQ(out[0], 0x93);
    CHECK_EQ(out[1], 5);
    CHECK(out[2] >= 1 && out[2] <= 127);
  }
}

// The note off carries a release velocity from how fast the pressure fell away.
TEST(ufm_cell_release_is_note_off_with_release_velocity) {
  enterUserFirmwareMode();
  linn::press(5, 3, 2000, 2000, 400);
  drained();
  linn::release(5, 3);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 3);
  byte instant = 0;
  if (out.size() == 3) {
    CHECK_EQ(out[0], 0x83);
    CHECK_EQ(out[1], 5);
    instant = out[2];
    CHECK_EQ(instant, 127);
  }
  linn::press(5, 3, 2000, 2000, 400);
  drained();
  for (int z = 380; z > 60; z -= 20) {
    linn::hold(5, 3, 2000, 2000, (uint16_t)z, 1);
  }
  linn::release(5, 3);
  out = drained();
  CHECK_EQ(out.size(), 3);
  if (out.size() == 3) {
    CHECK_EQ(out[0], 0x83);
    CHECK(out[2] >= 1 && out[2] < instant);
  }
}

TEST(ufm_note_velocity_follows_the_global_velocity_setting) {
  enterUserFirmwareMode();
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = 77;
  linn::press(5, 3, 2000, 2000, 400);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 3);
  if (out.size() == 3) {
    CHECK_EQ(out[2], 77);
  }
}

TEST(ufm_bottom_row_is_channel_1_top_row_channel_8) {
  enterUserFirmwareMode();
  linn::press(7, 0, 2000, 2000, 400);
  linn::press(9, 7, 2000, 2000, 400);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 6);
  if (out.size() == 6) {
    CHECK_EQ(out[0], 0x90);
    CHECK_EQ(out[1], 7);
    CHECK_EQ(out[3], 0x97);
    CHECK_EQ(out[4], 9);
  }
}

TEST(ufm_control_column_is_note_0_except_the_global_settings_row) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 12, 1});
  linn::press(0, 3, 2000, 2000, 400);
  linn::hold(0, 3, 2000, 2000, 700);
  linn::release(0, 3);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 6);
  if (out.size() == 6) {
    CHECK_EQ(out[0], 0x93);
    CHECK_EQ(out[1], 0);
    CHECK_EQ(out[3], 0x83);
  }
  CHECK_EQ(countStatus(out, 0xA0), 0);
  linn::press(0, GLOBAL_SETTINGS_ROW, 2000, 2000, 400);
  linn::release(0, GLOBAL_SETTINGS_ROW);
  CHECK_EQ(drained().size(), 0);
}

TEST(ufm_global_settings_panel_still_opens) {
  enterUserFirmwareMode();
  host::setSensor(0, GLOBAL_SETTINGS_ROW, 2000, 2000, 400);
  for (int i = 0; i < 3; ++i) {
    linn::scan(0, GLOBAL_SETTINGS_ROW);
  }
  CHECK_EQ(displayMode, displayGlobal);
  CHECK(userFirmwareActive);
  linn::release(0, GLOBAL_SETTINGS_ROW);
  CHECK_EQ(drained().size(), 0);
}

TEST(ufm_no_expression_until_enabled) {
  enterUserFirmwareMode();
  linn::press(5, 3, 2000, 2000, 400);
  linn::hold(5, 3, 2200, 2500, 600);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(countStatus(out, 0x90), 1);
  CHECK_EQ(countStatus(out, 0xA0), 0);
  CHECK_EQ(countStatus(out, 0xB0), 0);
}

TEST(ufm_z_data_is_poly_pressure_on_the_row_channel) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 12, 1});
  linn::press(5, 3, 2000, 2000, 400);
  std::vector<uint8_t> out = drained();
  CHECK(countStatus(out, 0xA0) >= 1);
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if (out[i] == 0xA3) {
      CHECK_EQ(out[i + 1], 5);
      CHECK(out[i + 2] > 0);
      break;
    }
  }
}

TEST(ufm_x_data_is_14_bit_lsb_then_msb) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 10, 1});
  linn::press(5, 3, 2000, 2000, 400);
  drained();
  host::advanceMicros(20000);
  linn::hold(5, 3, 2400, 2000, 400);
  std::vector<uint8_t> out = drained();
  int lsbAt = -1, msbAt = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if (out[i] == 0xB3 && out[i + 1] == 37 && lsbAt < 0) lsbAt = (int)i;
    if (out[i] == 0xB3 && out[i + 1] == 5 && msbAt < 0) msbAt = (int)i;
  }
  CHECK(lsbAt >= 0);
  CHECK(msbAt >= 0);
  CHECK(lsbAt < msbAt);
  if (lsbAt >= 0 && msbAt >= 0) {
    int value = out[msbAt + 2] * 128 + out[lsbAt + 2];
    // X is one axis across the row: uncalibrated, the position tracks the raw reading
    CHECK_NEAR(value, 2400, 120);
  }
}

static int xPositionAt(byte col, uint16_t rawX) {
  linn::midiIn({0xB3, 10, 1});
  linn::press(col, 3, rawX, 2000, 400);
  linn::hold(col, 3, rawX, 2000, 400);
  std::vector<uint8_t> out = drained();
  int lsb = -1, msb = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if (out[i] == 0xB3 && out[i + 1] == col + 32) lsb = out[i + 2];
    if (out[i] == 0xB3 && out[i + 1] == col) msb = out[i + 2];
  }
  linn::release(col, 3);
  drained();
  if (lsb < 0 || msb < 0) return -1;
  return msb * 128 + lsb;
}

TEST(ufm_x_range_covers_the_surface_of_each_model) {
  enterUserFirmwareMode();
  int left = xPositionAt(1, 0);
  int right = xPositionAt(25, 4095);
  CHECK(left >= 0 && left <= 90);
  CHECK(right >= 4000 && right <= 4265);

  linn::reset(128);
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  drained();
  left = xPositionAt(1, 0);
  right = xPositionAt(16, 4095);
  CHECK(left >= 0 && left <= 90);
  CHECK(right >= 2600 && right <= 2700);
}

TEST(ufm_y_data_is_cc_64_plus_column) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 11, 1});
  linn::press(5, 3, 2000, 1930, 400);
  std::vector<uint8_t> out = drained();
  int value = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if (out[i] == 0xB3 && out[i + 1] == 69) value = out[i + 2];
  }
  CHECK(value > 0);
}

static int lastValueOf(const std::vector<uint8_t>& out, uint8_t status, uint8_t data1) {
  int value = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if (out[i] == status && out[i + 1] == data1) value = out[i + 2];
  }
  return value;
}

// Y is also one axis across the surface; row 3 spans raw readings of about 1810 to 1965.
TEST(ufm_y_runs_from_the_bottom_to_the_top_of_the_cell) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 11, 1});
  linn::press(5, 3, 2000, 1830, 400);
  linn::hold(5, 3, 2000, 1830, 400);
  int low = lastValueOf(drained(), 0xB3, 69);
  // Y changes are slewed over scans like pressure, so let the new position settle
  for (int i = 0; i < 40; ++i) {
    host::advanceMicros(10000);
    linn::hold(5, 3, 2000, 1960, 400, 1);
  }
  int high = lastValueOf(drained(), 0xB3, 69);
  CHECK(low >= 0 && low < 40);
  CHECK(high > 90 && high <= 127);
}

TEST(ufm_z_runs_from_the_lightest_to_the_firmest_touch) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 12, 1});
  linn::press(5, 3, 2000, 2000, 300);
  int light = lastValueOf(drained(), 0xA3, 5);
  // pressure changes are slewed over scans, so give the firmer touch time to settle
  for (int i = 0; i < 40; ++i) {
    host::advanceMicros(10000);
    linn::hold(5, 3, 2000, 2000, 900, 1);
  }
  int firm = lastValueOf(drained(), 0xA3, 5);
  CHECK(light >= 0);
  CHECK(firm > light);
}

TEST(ufm_expression_switches_are_per_row) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 12, 1});
  linn::press(5, 2, 2000, 2000, 400);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(countStatus(out, 0x90), 1);
  CHECK_EQ(countStatus(out, 0xA0), 0);
}

// There is always a decimation interval; the default is about 8 ms.
TEST(ufm_decimation_is_set_in_milliseconds) {
  enterUserFirmwareMode();
  CHECK_NEAR(midiDecimateRate, 8000, 100);
  linn::midiIn({0xB0, 13, 50});
  CHECK_EQ(midiDecimateRate, 50000);
}

TEST(ufm_decimation_cannot_go_below_12_ms_in_low_power_mode) {
  enterUserFirmwareMode();
  Device.operatingLowPower = true;
  applyMidiDecimationRate();
  linn::midiIn({0xB0, 13, 20});
  CHECK_EQ(midiDecimateRate, 20000);
  linn::midiIn({0xB0, 13, 12});
  CHECK_EQ(midiDecimateRate, 12000);
  linn::midiIn({0xB0, 13, 11});
  CHECK_EQ(midiDecimateRate, 12000);
}

TEST(ufm_decimation_limits_the_update_rate) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 12, 1});
  linn::midiIn({0xB0, 13, 100});
  linn::press(5, 3, 2000, 2000, 300);
  linn::drainMidi();
  host::clearSerialOutput();
  for (int i = 0; i < 10; ++i) {
    linn::hold(5, 3, 2000, 2000, (uint16_t)(400 + i * 30), 1);
  }
  CHECK_EQ(countStatus(drained(), 0xA0), 1);
}

TEST(ufm_entering_turns_the_note_lights_off_and_lights_global_settings_yellow) {
  linn::reset();
  int lit = 0;
  for (byte col = 1; col < NUMCOLS; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      if (getLedColor(col, row, LED_LAYER_MAIN) != COLOR_OFF) ++lit;
    }
  }
  CHECK(lit > 0);
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  lit = 0;
  for (byte col = 1; col < NUMCOLS; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      if (getLedColor(col, row, LED_LAYER_COMBINED) != COLOR_OFF) ++lit;
    }
  }
  CHECK_EQ(lit, 0);
  CHECK_EQ(getLedColor(0, GLOBAL_SETTINGS_ROW, LED_LAYER_COMBINED), COLOR_YELLOW);
}

TEST(ufm_leds_are_set_with_cc_20_21_22) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 20, 5, 0xB0, 21, 3, 0xB0, 22, COLOR_GREEN});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_CUSTOM2), COLOR_GREEN);
  linn::midiIn({0xB0, 22, 0});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_CUSTOM2), COLOR_OFF);
  linn::midiIn({0xB0, 22, COLOR_PINK});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_CUSTOM2), COLOR_PINK);
  linn::midiIn({0xB0, 22, 12});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_CUSTOM2), COLOR_OFF);
}

// a light set outside the mode is hidden while it is on and shows again when it is left, and
// the mode's own lights are gone by then
TEST(ufm_leds_use_their_own_layer) {
  linn::reset();
  linn::midiIn({0xB0, 20, 5, 0xB0, 21, 3, 0xB0, 22, COLOR_RED});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_COMBINED), COLOR_RED);
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_COMBINED), COLOR_OFF);
  linn::midiIn({0xB0, 22, COLOR_BLUE});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_COMBINED), COLOR_BLUE);
  linn::midiInNrpn(245, 0, 0);
  linn::settle();
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_COMBINED), COLOR_RED);
}

TEST(ufm_slide_mode_is_enabled_per_row) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 9, 1});
  CHECK(userFirmwareSlideMode[3]);
  CHECK(!userFirmwareSlideMode[2]);
  linn::midiIn({0xB3, 9, 0});
  CHECK(!userFirmwareSlideMode[3]);
}

TEST(ufm_slide_transition_sends_cc_119_then_note_on_then_note_off) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 9, 1});
  // X is one axis across the whole row, about 171 units per cell, so a slide from column 5
  // to 6 crosses the boundary near 768
  linn::press(5, 3, 700, 2000, 400);
  linn::hold(5, 3, 760, 2000, 400);
  std::vector<uint8_t> first = drained();
  byte firstVelocity = first.size() >= 3 ? first[2] : 0;
  for (int i = 0; i < 6; ++i) {
    host::setSensor(5, 3, 765, 2000, (uint16_t)(400 - i * 50));
    host::setSensor(6, 3, 780, 2000, (uint16_t)(200 + i * 60));
    linn::scan(5, 3);
    linn::scan(6, 3);
  }
  std::vector<uint8_t> out = drained();
  int cc119 = -1, on6 = -1, off5 = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if (out[i] == 0xB3 && out[i + 1] == 119 && cc119 < 0) cc119 = (int)i;
    if (out[i] == 0x93 && out[i + 1] == 6 && on6 < 0) on6 = (int)i;
    if (out[i] == 0x83 && out[i + 1] == 5 && off5 < 0) off5 = (int)i;
  }
  CHECK(cc119 >= 0);
  CHECK(on6 >= 0);
  CHECK(off5 >= 0);
  if (cc119 >= 0 && on6 >= 0 && off5 >= 0) {
    CHECK_EQ(out[cc119 + 2], 5);
    CHECK(cc119 < on6);
    CHECK(on6 < off5);
    CHECK_EQ(out[on6 + 2], firstVelocity);
    CHECK_EQ(out[off5 + 2], 6);
  }
}

// the right pedal is sustain by default, nothing is assigned here
TEST(ufm_foot_switches_keep_their_assignment) {
  enterUserFirmwareMode();
  CHECK_EQ(Global.switchAssignment[SWITCH_FOOT_R], ASSIGNED_SUSTAIN);
  footSwitchOffState[SWITCH_FOOT_L] = HIGH;
  footSwitchOffState[SWITCH_FOOT_R] = HIGH;
  footSwitchState[SWITCH_FOOT_R] = false;
  footSwitchState[SWITCH_FOOT_B] = false;
  digitalWrite(FOOT_SW_LEFT, HIGH);
  digitalWrite(FOOT_SW_RIGHT, LOW);
  checkFootSwitches();
  std::vector<uint8_t> out = drained();
  bool sustain = false;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0xB0 && out[i + 1] == 64 && out[i + 2] == 127) sustain = true;
  }
  CHECK(sustain);
}

// Lights, text and the queries, see the Lights and the Asking the instrument sections of the document

// the fixture leaves setup unfinished because the serial tests pump the port themselves,
// and the continuous tasks only run once setup is done
static void scrollStep() {
  host::advanceMicros(41000);
  setupDone = true;
  performContinuousTasks();
  setupDone = false;
}

static void paintColumn(byte col, byte color) {
  for (byte row = 0; row < NUMROWS; ++row) {
    linn::midiIn({0xB0, 20, col, 0xB0, 21, row, 0xB0, 22, color});
  }
}

static int litOnLayer(byte layer) {
  int lit = 0;
  for (byte col = 0; col < NUMCOLS; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      if (getLedColor(col, row, layer) != COLOR_OFF) ++lit;
    }
  }
  return lit;
}

// the value of the one NRPN reply in the drained output, -1 when there is none
static int nrpnReplyValue(int parameter) {
  std::vector<uint8_t> out = drained();
  if (out.size() != 18 || out[2] * 128 + out[5] != parameter) return -1;
  return out[8] * 128 + out[11];
}

static void midiInText(const char* text) {
  for (const char* p = text; *p; ++p) {
    linn::midiIn({0xB0, 27, (uint8_t)*p});
  }
}

TEST(ufm_query_272_to_274_report_the_model_the_version_and_the_build) {
  linn::reset();
  linn::midiInNrpn(299, 272, 0);
  CHECK_EQ(nrpnReplyValue(272), 200);
  linn::midiInNrpn(299, 273, 0);
  CHECK_EQ(nrpnReplyValue(273), atoi(OSVersion));
  linn::midiInNrpn(299, 274, 0);
  CHECK_EQ(nrpnReplyValue(274), atoi(OSVersionBuild + 1));
  linn::reset(128);
  linn::midiInNrpn(299, 272, 0);
  CHECK_EQ(nrpnReplyValue(272), 128);
}

// an uncalibrated instrument reports positions from the default calibration
TEST(ufm_query_275_reports_whether_the_instrument_is_calibrated) {
  linn::reset();
  linn::midiInNrpn(299, 275, 0);
  CHECK_EQ(nrpnReplyValue(275), 0);
  Device.calibrated = true;
  linn::midiInNrpn(299, 275, 0);
  CHECK_EQ(nrpnReplyValue(275), 1);
}

TEST(ufm_query_272_to_274_are_read_only) {
  linn::reset();
  linn::midiInNrpn(272, 128, 0);
  linn::midiInNrpn(299, 272, 0);
  CHECK_EQ(nrpnReplyValue(272), 200);
}

TEST(ufm_cc_25_colors_the_cursor_cell_and_moves_right_then_up_from_column_0) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 20, 24, 0xB0, 21, 3});
  linn::midiIn({0xB0, 25, 1, 0xB0, 25, 2, 0xB0, 25, 3, 0xB0, 25, 4});
  CHECK_EQ(getLedColor(24, 3, LED_LAYER_CUSTOM2), 1);
  CHECK_EQ(getLedColor(25, 3, LED_LAYER_CUSTOM2), 2);
  CHECK_EQ(getLedColor(0, 4, LED_LAYER_CUSTOM2), 3);
  CHECK_EQ(getLedColor(1, 4, LED_LAYER_CUSTOM2), 4);
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 4);
  linn::midiIn({0xB0, 22, 5});
  CHECK_EQ(getLedColor(2, 4, LED_LAYER_CUSTOM2), 5);
}

TEST(ufm_cc_25_stops_above_the_top_row_and_switches_off_with_0_or_12_and_up) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 20, 24, 0xB0, 21, 7});
  linn::midiIn({0xB0, 25, 11, 0xB0, 25, 6, 0xB0, 25, 7});
  CHECK_EQ(getLedColor(24, 7, LED_LAYER_CUSTOM2), COLOR_PINK);
  CHECK_EQ(getLedColor(25, 7, LED_LAYER_CUSTOM2), 6);
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 2);
  linn::midiIn({0xB0, 20, 24, 0xB0, 21, 7});
  linn::midiIn({0xB0, 25, 0, 0xB0, 25, 12});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
}

TEST(ufm_cc_25_fills_a_128_in_136_messages) {
  linn::reset(128);
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  linn::midiIn({0xB0, 20, 0, 0xB0, 21, 0});
  for (int i = 0; i < 136; ++i) {
    linn::midiIn({0xB0, 25, COLOR_BLUE});
  }
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 136);
  CHECK_EQ(getLedColor(16, 7, LED_LAYER_CUSTOM2), COLOR_BLUE);
}

// outside the mode a painted light shows in the normal firmware and is hidden once the mode is on
TEST(ufm_lights_outside_the_mode_use_the_normal_custom_layer) {
  linn::reset();
  linn::midiIn({0xB0, 20, 5, 0xB0, 21, 3, 0xB0, 25, COLOR_RED});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_COMBINED), COLOR_RED);
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_COMBINED), COLOR_OFF);
}

TEST(ufm_scroll_keeps_moving_but_draws_nothing_while_global_settings_is_open) {
  enterUserFirmwareMode();
  midiInText("1");
  linn::midiIn({0xB0, 21, 0, 0xB0, 29, COLOR_GREEN});
  setDisplayMode(displayGlobal);
  scrollStep();
  scrollStep();
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  setDisplayMode(displayNormal);
  scrollStep();
  CHECK_EQ(getLedColor(23, 1, LED_LAYER_CUSTOM2), COLOR_GREEN);
}

TEST(ufm_cc_25_cursor_keeps_moving_while_global_settings_is_open) {
  enterUserFirmwareMode();
  setDisplayMode(displayGlobal);
  linn::midiIn({0xB0, 20, 5, 0xB0, 21, 3, 0xB0, 25, COLOR_RED});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  setDisplayMode(displayNormal);
  linn::midiIn({0xB0, 25, COLOR_GREEN});
  CHECK_EQ(getLedColor(5, 3, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(getLedColor(6, 3, LED_LAYER_CUSTOM2), COLOR_GREEN);
}

TEST(ufm_cc_28_while_global_settings_is_open_draws_nothing_and_discards_the_text) {
  enterUserFirmwareMode();
  setDisplayMode(displayGlobal);
  linn::midiIn({0xB0, 20, 3, 0xB0, 21, 0});
  midiInText("1");
  linn::midiIn({0xB0, 28, COLOR_RED});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  setDisplayMode(displayNormal);
  linn::midiIn({0xB0, 28, COLOR_RED});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
}

TEST(ufm_cc_29_while_global_settings_is_open_starts_nothing_and_discards_the_text) {
  enterUserFirmwareMode();
  setDisplayMode(displayGlobal);
  linn::midiIn({0xB0, 21, 0});
  midiInText("1");
  linn::midiIn({0xB0, 29, COLOR_RED});
  setDisplayMode(displayNormal);
  scrollStep();
  scrollStep();
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  linn::midiIn({0xB0, 29, COLOR_RED});
  scrollStep();
  scrollStep();
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
}

TEST(ufm_query_reply_arrives_on_the_channel_of_the_query) {
  linn::reset();
  linn::midiInNrpn(299, 272, 0);
  std::vector<uint8_t> out = drained();
  CHECK_EQ(out.size(), 18);
  if (out.size() == 18) CHECK_EQ(out[0], 0xB0);
  linn::midiInNrpn(299, 272, 3);
  out = drained();
  CHECK_EQ(out.size(), 18);
  if (out.size() == 18) CHECK_EQ(out[0], 0xB3);
}

// the "only when changed" memory is per cell and reset by every new touch, so a second press at
// the same spot with the same pressure still reports its X, Y and Z
TEST(ufm_repeat_press_at_the_same_spot_sends_its_values_again) {
  enterUserFirmwareMode();
  linn::midiIn({0xB3, 10, 1, 0xB3, 11, 1, 0xB3, 12, 1});
  linn::press(5, 3, 2000, 1930, 400);
  linn::hold(5, 3, 2000, 1930, 400, 10);
  drained();
  linn::release(5, 3);
  drained();
  linn::press(5, 3, 2000, 1930, 400);
  linn::hold(5, 3, 2000, 1930, 400, 10);
  std::vector<uint8_t> out = drained();
  CHECK(countStatus(out, 0xA0) >= 1);
  CHECK(lastValueOf(out, 0xB3, 5) >= 0);
  CHECK(lastValueOf(out, 0xB3, 37) >= 0);
  CHECK(lastValueOf(out, 0xB3, 69) >= 0);
}

// big "1": the bottom pixel row is blank, the row above it is five wide, the stem sits two in
TEST(ufm_cc_28_draws_the_text_at_the_cursor_in_the_font_of_cc_26) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 20, 8, 0xB0, 21, 3, 0xB0, 22, COLOR_RED});
  linn::midiIn({0xB0, 20, 3, 0xB0, 21, 0, 0xB0, 26, 2});
  midiInText("1");
  linn::midiIn({0xB0, 28, COLOR_GREEN});
  CHECK_EQ(getLedColor(3, 0, LED_LAYER_CUSTOM2), COLOR_OFF);
  for (byte col = 3; col <= 7; ++col) {
    CHECK_EQ(getLedColor(col, 1, LED_LAYER_CUSTOM2), COLOR_GREEN);
  }
  CHECK_EQ(getLedColor(5, 6, LED_LAYER_CUSTOM2), COLOR_GREEN);
  CHECK_EQ(getLedColor(3, 6, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(getLedColor(5, 7, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(getLedColor(8, 3, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 12);

  // past the right edge the text is cut off, nothing wraps around
  linn::midiIn({0xB0, 20, 24, 0xB0, 21, 0});
  midiInText("1");
  linn::midiIn({0xB0, 28, COLOR_RED});
  CHECK_EQ(getLedColor(24, 1, LED_LAYER_CUSTOM2), COLOR_RED);
  CHECK_EQ(getLedColor(25, 1, LED_LAYER_CUSTOM2), COLOR_RED);
  CHECK_EQ(getLedColor(0, 1, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(getLedColor(1, 1, LED_LAYER_CUSTOM2), COLOR_OFF);

  // the switch column never holds text, a cursor there starts it at column 1
  linn::midiIn({0xB0, 20, 0, 0xB0, 21, 3});
  midiInText("1");
  linn::midiIn({0xB0, 28, COLOR_GREEN});
  CHECK_EQ(getLedColor(0, 4, LED_LAYER_CUSTOM2), COLOR_OFF);
  for (byte col = 1; col <= 5; ++col) {
    CHECK_EQ(getLedColor(col, 4, LED_LAYER_CUSTOM2), COLOR_GREEN);
  }
}

TEST(ufm_cc_26_selects_tiny_small_big_or_condensed) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 20, 3, 0xB0, 21, 0, 0xB0, 26, 0});
  midiInText("A1");
  linn::midiIn({0xB0, 28, COLOR_GREEN});
  int lit = 0;
  for (byte col = 3; col < 6; ++col) {
    for (byte row = 0; row < 4; ++row) {
      if (getLedColor(col, row, LED_LAYER_CUSTOM2) != COLOR_OFF) ++lit;
    }
  }
  CHECK(lit > 0);
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), lit);
  for (byte row = 4; row < 8; ++row) {
    CHECK_EQ(getLedColor(3, row, LED_LAYER_CUSTOM2), COLOR_OFF);
  }

  // a condensed "A" is three columns wide, so drawn at column 7 its blank separator is column 10
  paintColumn(10, COLOR_RED);
  linn::midiIn({0xB0, 20, 7, 0xB0, 21, 0, 0xB0, 26, 3});
  midiInText("A");
  linn::midiIn({0xB0, 28, COLOR_GREEN});
  CHECK_EQ(getLedColor(10, 0, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(getLedColor(10, 7, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(getLedColor(11, 0, LED_LAYER_CUSTOM2), COLOR_OFF);
  paintColumn(10, COLOR_RED);
  linn::midiIn({0xB0, 20, 6, 0xB0, 21, 0});
  midiInText("A");
  linn::midiIn({0xB0, 28, COLOR_GREEN});
  CHECK_EQ(getLedColor(10, 0, LED_LAYER_CUSTOM2), COLOR_RED);

  // a value above 3 is ignored and the condensed font stays, so the "A" is still three wide
  paintColumn(10, COLOR_RED);
  linn::midiIn({0xB0, 26, 4, 0xB0, 20, 6, 0xB0, 21, 0});
  midiInText("A");
  linn::midiIn({0xB0, 28, COLOR_GREEN});
  CHECK_EQ(getLedColor(10, 0, LED_LAYER_CUSTOM2), COLOR_RED);
}

TEST(ufm_cc_28_erases_blank_pixels_inside_the_text_box_only_and_clears_the_text) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 20, 3, 0xB0, 21, 0});
  for (int i = 0; i < 7; ++i) linn::midiIn({0xB0, 25, COLOR_GREEN});
  linn::midiIn({0xB0, 20, 3, 0xB0, 21, 0, 0xB0, 26, 2});
  midiInText("1");
  linn::midiIn({0xB0, 28, COLOR_RED});
  CHECK_EQ(getLedColor(3, 0, LED_LAYER_CUSTOM2), COLOR_OFF);
  CHECK_EQ(getLedColor(9, 0, LED_LAYER_CUSTOM2), COLOR_GREEN);
  linn::midiIn({0xB0, 20, 12, 0xB0, 21, 0, 0xB0, 28, COLOR_RED});
  CHECK_EQ(getLedColor(12, 1, LED_LAYER_CUSTOM2), COLOR_OFF);
}

TEST(ufm_cc_27_holds_32_characters_and_0_clears_it) {
  linn::reset();
  for (int i = 0; i < 40; ++i) linn::midiIn({0xB0, 27, 'A'});
  CHECK_EQ(midiTextLength, 32);
  linn::midiIn({0xB0, 27, 0});
  CHECK_EQ(midiTextLength, 0);
}

TEST(ufm_cc_29_scrolls_one_column_every_40_ms_and_ends_clear) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 21, 0, 0xB0, 26, 2});
  midiInText("1");
  linn::midiIn({0xB0, 29, COLOR_GREEN});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  // the text went with the scroll, a draw now has nothing to draw
  linn::midiIn({0xB0, 20, 3, 0xB0, 21, 4, 0xB0, 28, COLOR_RED});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  scrollStep();
  CHECK_EQ(getLedColor(25, 1, LED_LAYER_CUSTOM2), COLOR_GREEN);
  CHECK_EQ(getLedColor(24, 1, LED_LAYER_CUSTOM2), COLOR_OFF);
  scrollStep();
  scrollStep();
  CHECK_EQ(getLedColor(23, 1, LED_LAYER_CUSTOM2), COLOR_GREEN);
  CHECK_EQ(getLedColor(25, 1, LED_LAYER_CUSTOM2), COLOR_GREEN);
  for (int i = 0; i < 40; ++i) scrollStep();
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
}

TEST(ufm_cc_29_is_replaced_by_a_new_scroll_and_stops_when_the_mode_is_left) {
  enterUserFirmwareMode();
  linn::midiIn({0xB0, 21, 0});
  midiInText("1");
  linn::midiIn({0xB0, 29, COLOR_GREEN});
  scrollStep();
  CHECK_EQ(getLedColor(25, 1, LED_LAYER_CUSTOM2), COLOR_GREEN);
  midiInText("2");
  linn::midiIn({0xB0, 29, COLOR_GREEN});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  scrollStep();
  CHECK(litOnLayer(LED_LAYER_CUSTOM2) > 0);
  linn::midiIn({0xB0, 29, COLOR_GREEN});
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  scrollStep();
  scrollStep();
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
  midiInText("3");
  linn::midiIn({0xB0, 29, COLOR_GREEN});
  linn::midiInNrpn(245, 0, 0);
  linn::settle();
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  scrollStep();
  scrollStep();
  CHECK_EQ(litOnLayer(LED_LAYER_CUSTOM2), 0);
}
