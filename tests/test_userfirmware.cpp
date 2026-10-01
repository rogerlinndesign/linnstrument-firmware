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

TEST(ufm_decimation_in_low_power_mode_only_accepts_more_than_12_ms) {
  enterUserFirmwareMode();
  Device.operatingLowPower = true;
  applyMidiDecimationRate();
  linn::midiIn({0xB0, 13, 12});
  CHECK_EQ(midiDecimateRate, 12000);
  linn::midiIn({0xB0, 13, 5});
  CHECK_EQ(midiDecimateRate, 12000);
  linn::midiIn({0xB0, 13, 13});
  CHECK_EQ(midiDecimateRate, 13000);
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
