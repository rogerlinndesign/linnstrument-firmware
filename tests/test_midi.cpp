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
// MIDI output framing and NRPN handling.

TEST(midi_note_on_leaves_as_three_bytes) {
  linn::reset();
  queueMidiMessage(MIDINoteOn, 60, 100, 2);
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  CHECK_EQ(out.size(), 3);
  if (out.size() == 3) {
    CHECK_EQ(out[0], 0x92);
    CHECK_EQ(out[1], 60);
    CHECK_EQ(out[2], 100);
  }
}

TEST(midi_channel_pressure_leaves_as_two_bytes) {
  linn::reset();
  queueMidiMessage(MIDIChannelPressure, 50, 0, 3);
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  CHECK_EQ(out.size(), 2);
  if (out.size() == 2) {
    CHECK_EQ(out[0], 0xD3);
    CHECK_EQ(out[1], 50);
  }
}

TEST(midi_messages_keep_their_order) {
  linn::reset();
  queueMidiMessage(MIDINoteOn, 60, 100, 0);
  queueMidiMessage(MIDINoteOff, 60, 0, 0);
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  CHECK_EQ(out.size(), 6);
  if (out.size() == 6) {
    CHECK_EQ(out[0], 0x90);
    CHECK_EQ(out[3], 0x80);
  }
}

TEST(nrpn_sets_left_handed_operation) {
  linn::reset();
  receivedNrpn(246, 1, 0);
  CHECK(Device.otherHanded);
  receivedNrpn(246, 0, 0);
  CHECK(!Device.otherHanded);
}

TEST(nrpn_ignores_out_of_range_values) {
  linn::reset();
  receivedNrpn(246, 5, 0);
  CHECK(!Device.otherHanded);
}

// The reply is the NRPN number, its value, and a null RPN to close the parameter: six CCs.
TEST(nrpn_query_answers_with_the_value_and_a_null_rpn) {
  linn::reset();
  Device.otherHanded = true;
  receivedNrpn(299, 246, 0);
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  CHECK_EQ(out.size(), 18);
  if (out.size() == 18) {
    CHECK_EQ(out[0], 0xB0);
    CHECK_EQ(out[1], 99);
    CHECK_EQ(out[2] * 128 + out[5], 246);
    CHECK_EQ(out[7], 6);
    CHECK_EQ(out[8] * 128 + out[11], 1);
    CHECK_EQ(out[13], 101);
    CHECK_EQ(out[16], 100);
  }
}
