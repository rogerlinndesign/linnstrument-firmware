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
// The arpeggiator steps through the held notes.

static std::vector<std::vector<uint8_t>> arpNotes() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  std::vector<std::vector<uint8_t>> notes;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0x90 || (out[i] & 0xF0) == 0x80) {
      notes.push_back({out[i], out[i + 1], out[i + 2]});
      i += 2;
    }
  }
  return notes;
}

static void holdTwoNotes() {
  linn::reset();
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = 77;
  Global.rowOffset = 5;
  Split[LEFT].arpeggiator = true;
  Global.arpOctave = 0;
  linn::press(5, 3, 2000, 2000, 400);
  linn::press(7, 3, 2000, 2000, 400);
  arpNotes();
}

TEST(arpeggiator_up_steps_through_the_held_notes) {
  holdTwoNotes();
  Global.arpDirection = ArpUp;
  advanceArpeggiatorForSplit(LEFT);
  std::vector<std::vector<uint8_t>> step1 = arpNotes();
  CHECK_EQ(step1.size(), 1);
  if (step1.size() == 1) {
    CHECK_EQ(step1[0][0], 0x90);
    CHECK_EQ(step1[0][1], 49);
    CHECK_EQ(step1[0][2], 77);
  }
  advanceArpeggiatorForSplit(LEFT);
  std::vector<std::vector<uint8_t>> step2 = arpNotes();
  CHECK_EQ(step2.size(), 2);
  if (step2.size() == 2) {
    CHECK_EQ(step2[0][0], 0x80);
    CHECK_EQ(step2[0][1], 49);
    CHECK_EQ(step2[1][0], 0x90);
    CHECK_EQ(step2[1][1], 51);
  }
  advanceArpeggiatorForSplit(LEFT);
  std::vector<std::vector<uint8_t>> step3 = arpNotes();
  CHECK_EQ(step3.size(), 2);
  if (step3.size() == 2) CHECK_EQ(step3[1][1], 49);
}

TEST(arpeggiator_replay_all_plays_every_held_note_each_step) {
  holdTwoNotes();
  Global.arpDirection = ArpReplayAll;
  advanceArpeggiatorForSplit(LEFT);
  std::vector<std::vector<uint8_t>> step = arpNotes();
  int ons = 0;
  for (size_t i = 0; i < step.size(); ++i) {
    if (step[i][0] == 0x90) ++ons;
  }
  CHECK_EQ(ons, 2);
}

TEST(arpeggiator_octave_adds_the_note_an_octave_up) {
  holdTwoNotes();
  Global.arpDirection = ArpReplayAll;
  Global.arpOctave = 1;
  advanceArpeggiatorForSplit(LEFT);
  std::vector<std::vector<uint8_t>> step = arpNotes();
  bool high = false;
  for (size_t i = 0; i < step.size(); ++i) {
    if (step[i][0] == 0x90 && step[i][1] == 49 + 12) high = true;
  }
  CHECK(high);
}

TEST(arpeggiator_stops_when_the_notes_are_released) {
  holdTwoNotes();
  Global.arpDirection = ArpUp;
  advanceArpeggiatorForSplit(LEFT);
  CHECK_EQ(arpNotes().size(), 1);
  linn::release(5, 3);
  linn::release(7, 3);
  arpNotes();
  advanceArpeggiatorForSplit(LEFT);
  std::vector<std::vector<uint8_t>> step = arpNotes();
  int ons = 0;
  for (size_t i = 0; i < step.size(); ++i) {
    if (step[i][0] == 0x90) ++ons;
  }
  CHECK_EQ(ons, 0);
}
