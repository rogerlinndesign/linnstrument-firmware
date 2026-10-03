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
// Normal playing: a touch becomes a note on the split's MIDI channel, and the channel modes.

static void fixedVelocity(byte value) {
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = value;
}

static std::vector<uint8_t> sent() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  return out;
}

static std::vector<std::vector<uint8_t>> notesIn(const std::vector<uint8_t>& out) {
  std::vector<std::vector<uint8_t>> notes;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0x90 || (out[i] & 0xF0) == 0x80) {
      notes.push_back({out[i], out[i + 1], out[i + 2]});
      i += 2;
    }
  }
  return notes;
}

TEST(playing_a_cell_sends_its_note_on_the_main_channel) {
  linn::reset();
  fixedVelocity(77);
  Global.rowOffset = 5;
  linn::press(5, 3, 2000, 2000, 400);
  std::vector<std::vector<uint8_t>> notes = notesIn(sent());
  CHECK_EQ(notes.size(), 1);
  if (notes.size() == 1) {
    CHECK_EQ(notes[0][0], 0x90);
    CHECK_EQ(notes[0][1], transposedNote(LEFT, 5, 3));
    CHECK_EQ(notes[0][1], 49);
    CHECK_EQ(notes[0][2], 77);
  }
  linn::release(5, 3);
  notes = notesIn(sent());
  CHECK_EQ(notes.size(), 1);
  if (notes.size() == 1) {
    CHECK_EQ(notes[0][0], 0x80);
    CHECK_EQ(notes[0][1], 49);
  }
}

TEST(playing_channel_per_row_uses_the_row) {
  linn::reset();
  fixedVelocity(77);
  Split[LEFT].midiMode = channelPerRow;
  Split[LEFT].midiChanPerRow = 3;
  updateSplitMidiChannels(LEFT);
  sent();
  linn::press(5, 2, 2000, 2000, 400);
  std::vector<std::vector<uint8_t>> notes = notesIn(sent());
  CHECK_EQ(notes.size(), 1);
  if (notes.size() == 1) CHECK_EQ(notes[0][0], 0x90 | (3 + 2 - 1));
}

TEST(playing_channel_per_note_gives_each_touch_its_own_channel) {
  linn::reset();
  fixedVelocity(77);
  Split[LEFT].midiMode = channelPerNote;
  updateSplitMidiChannels(LEFT);
  sent();
  linn::press(5, 3, 2000, 2000, 400);
  linn::press(8, 3, 2000, 2000, 400);
  std::vector<std::vector<uint8_t>> notes = notesIn(sent());
  CHECK_EQ(notes.size(), 2);
  if (notes.size() == 2) {
    CHECK((notes[0][0] & 0x0F) != (notes[1][0] & 0x0F));
    CHECK(Split[LEFT].midiChanSet[notes[0][0] & 0x0F]);
    CHECK(Split[LEFT].midiChanSet[notes[1][0] & 0x0F]);
  }
}

TEST(playing_right_of_the_split_point_uses_the_right_split) {
  linn::reset();
  fixedVelocity(77);
  Global.splitActive = true;
  Global.splitPoint = 12;
  linn::press(15, 3, 2000, 2000, 400);
  std::vector<std::vector<uint8_t>> notes = notesIn(sent());
  CHECK_EQ(notes.size(), 1);
  if (notes.size() == 1) CHECK_EQ(notes[0][0], 0x90 | (Split[RIGHT].midiChanMain - 1));
}

static int lastPitchBend(const std::vector<uint8_t>& out) {
  int value = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0xE0) value = out[i + 1] | (out[i + 2] << 7);
  }
  return value;
}

// a press at the reading column 5 gives at its centre, then a slide of about half a cell
static int bendAfterSliding(int delta) {
  uint16_t centre = (uint16_t)FXD_TO_INT(calculateDefaultMeasuredX(5));
  linn::press(5, 3, centre, 2000, 400);
  sent();
  for (int i = 1; i <= 20; ++i) {
    host::advanceMicros(10000);
    linn::hold(5, 3, (uint16_t)(centre + delta * i / 20), 2000, 400, 1);
  }
  int bend = lastPitchBend(sent());
  linn::release(5, 3);
  sent();
  return bend;
}

TEST(playing_moving_a_finger_sends_pitch_bend) {
  linn::reset();
  fixedVelocity(77);
  Split[LEFT].pitchCorrectQuantize = false;
  Split[LEFT].pitchCorrectHold = pitchCorrectHoldOff;
  int right = bendAfterSliding(100);
  int left = bendAfterSliding(-100);
  CHECK(right > 8192);
  CHECK(left < 8192);
  CHECK(left >= 0);
  // the same slide is a smaller part of a wider bend range
  Split[LEFT].bendRangeOption = bendRange24;
  int wide = bendAfterSliding(100);
  CHECK(wide > 8192);
  CHECK(wide < right);
}

TEST(playing_disabling_x_stops_pitch_bend) {
  linn::reset();
  fixedVelocity(77);
  Split[LEFT].sendX = false;
  linn::press(5, 3, 700, 2000, 400);
  sent();
  for (int i = 1; i <= 20; ++i) {
    host::advanceMicros(10000);
    linn::hold(5, 3, (uint16_t)(700 + i * 5), 2000, 400, 1);
  }
  std::vector<uint8_t> out = sent();
  CHECK(out.size() > 0);
  for (size_t i = 0; i < out.size(); ++i) {
    CHECK((out[i] & 0xF0) != 0xE0);
  }
}
