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
// The limits that guard against the instrument's sensor, its cross talk and phantom touches,
// which a desktop build lifts for a touch screen with sensorLimitsLifted.

static int limitNoteOns() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  int n = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0x90) ++n;
  }
  return n;
}

TEST(limits_take_three_touches_in_a_column_unless_lifted) {
  for (bool lifted : { false, true }) {
    linn::reset();
    sensorLimitsLifted = lifted;
    for (byte row = 1; row <= 4; ++row) linn::press(5, row, 2000, 2000, 600);
    CHECK_EQ(limitNoteOns(), lifted ? 4 : 3);
  }
}

TEST(limits_silence_the_fourth_corner_of_a_rectangle_unless_lifted) {
  for (bool lifted : { false, true }) {
    linn::reset();
    sensorLimitsLifted = lifted;
    linn::press(2, 2, 2000, 2000, 400);
    linn::press(6, 2, 2000, 2000, 400);
    linn::press(2, 5, 2000, 2000, 400);
    CHECK_EQ(limitNoteOns(), 3);
    linn::press(6, 5, 2000, 2000, 900);
    CHECK_EQ(limitNoteOns(), lifted ? 1 : 0);
  }
}

TEST(limits_keep_the_settings_switches_away_from_a_playing_hand_unless_lifted) {
  for (bool lifted : { false, true }) {
    linn::reset();
    sensorLimitsLifted = lifted;
    linn::press(5, 3, 2000, 2000, 600);
    linn::press(0, PER_SPLIT_ROW, 2000, 2000, 600);
    CHECK_EQ(cell(0, PER_SPLIT_ROW).touched == ignoredCell, !lifted);
  }
}

TEST(limits_hold_back_the_pitch_of_a_light_touch_sharing_a_column_unless_lifted) {
  for (bool lifted : { false, true }) {
    linn::reset();
    sensorLimitsLifted = lifted;
    linn::touch(5, 2, 600);
    linn::select(5, 4);
    linn::touch(5, 4, 200);
    CHECK_EQ(handleXExpression() != INVALID_DATA, lifted);
  }
}
