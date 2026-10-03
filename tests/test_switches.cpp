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
// Switch and foot switch assignments.

static std::vector<uint8_t> switchOutput() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  return out;
}

static int lastCC(const std::vector<uint8_t>& out, uint8_t number) {
  int value = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0xB0 && out[i + 1] == number) value = out[i + 2];
  }
  return value;
}

TEST(switch_octave_up_and_down_move_by_twelve) {
  linn::reset();
  CHECK_EQ(Split[LEFT].transposeOctave, 0);
  performSwitchAssignmentOn(SWITCH_FOOT_L, ASSIGNED_OCTAVE_UP, LEFT);
  CHECK_EQ(Split[LEFT].transposeOctave, 12);
  performSwitchAssignmentOn(SWITCH_FOOT_L, ASSIGNED_OCTAVE_DOWN, LEFT);
  performSwitchAssignmentOn(SWITCH_FOOT_L, ASSIGNED_OCTAVE_DOWN, LEFT);
  CHECK_EQ(Split[LEFT].transposeOctave, -12);
  CHECK_EQ(Split[RIGHT].transposeOctave, 0);
}

TEST(switch_octave_is_limited_to_five_octaves) {
  linn::reset();
  for (int i = 0; i < 8; ++i) {
    performSwitchAssignmentOn(SWITCH_FOOT_L, ASSIGNED_OCTAVE_UP, LEFT);
  }
  CHECK_EQ(Split[LEFT].transposeOctave, 60);
}

TEST(switch_sustain_sends_cc_64_on_and_off) {
  linn::reset();
  performSwitchAssignmentOn(SWITCH_FOOT_L, ASSIGNED_SUSTAIN, LEFT);
  CHECK_EQ(lastCC(switchOutput(), 64), 127);
  performSwitchAssignmentOff(SWITCH_FOOT_L, ASSIGNED_SUSTAIN, LEFT);
  CHECK_EQ(lastCC(switchOutput(), 64), 0);
}

TEST(switch_cc65_sends_the_configured_controller) {
  linn::reset();
  Global.ccForSwitchCC65[SWITCH_FOOT_L] = 20;
  performSwitchAssignmentOn(SWITCH_FOOT_L, ASSIGNED_CC_65, LEFT);
  std::vector<uint8_t> out = switchOutput();
  CHECK_EQ(lastCC(out, 20), 127);
  CHECK_EQ(lastCC(out, 65), -1);
  performSwitchAssignmentOff(SWITCH_FOOT_L, ASSIGNED_CC_65, LEFT);
  CHECK_EQ(lastCC(switchOutput(), 20), 0);
}

TEST(switch_foot_pedal_follows_the_pedal_and_its_polarity) {
  linn::reset();
  Global.switchAssignment[SWITCH_FOOT_R] = ASSIGNED_SUSTAIN;
  footSwitchOffState[SWITCH_FOOT_L] = HIGH;
  footSwitchOffState[SWITCH_FOOT_R] = HIGH;
  digitalWrite(FOOT_SW_LEFT, HIGH);
  digitalWrite(FOOT_SW_RIGHT, LOW);
  checkFootSwitches();
  CHECK_EQ(lastCC(switchOutput(), 64), 127);
  digitalWrite(FOOT_SW_RIGHT, HIGH);
  checkFootSwitches();
  CHECK_EQ(lastCC(switchOutput(), 64), 0);
}
