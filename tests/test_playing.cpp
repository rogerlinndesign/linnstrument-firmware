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

static std::vector<int> channelPressures(const std::vector<uint8_t>& out) {
  std::vector<int> values;
  for (size_t i = 0; i + 1 < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0xD0) {
      values.push_back(out[i + 1]);
      ++i;
    }
  }
  return values;
}

static void oneChannelPressure() {
  fixedVelocity(77);
  Split[LEFT].midiMode = oneChannel;
  Split[LEFT].expressionForZ = loudnessChannelPressure;
  Split[LEFT].sourceForZ = loudnessSourceHighest;
  Split[LEFT].sendZ = true;
  updateSplitMidiChannels(LEFT);
  sent();
}

static void holdBoth(int passes) {
  for (int i = 0; i < passes; ++i) {
    linn::hold(5, 3, 2000, 2000, 1400, 1);
    linn::hold(8, 3, 2000, 2000, 300, 1);
  }
}

TEST(playing_one_channel_pressure_follows_the_hardest_touch) {
  linn::reset();
  oneChannelPressure();
  linn::press(8, 3, 2000, 2000, 300);
  linn::hold(8, 3, 2000, 2000, 300, 20);
  std::vector<int> light = channelPressures(sent());
  linn::release(8, 3);
  sent();

  linn::press(5, 3, 2000, 2000, 1400);
  linn::press(8, 3, 2000, 2000, 300);
  holdBoth(20);
  std::vector<int> both = channelPressures(sent());
  CHECK(!light.empty());
  CHECK(!both.empty());
  if (!light.empty() && !both.empty()) {
    CHECK(both.back() > light.back() + 20);
  }
}

TEST(playing_one_channel_pressure_stays_up_while_a_touch_remains) {
  linn::reset();
  oneChannelPressure();
  linn::press(5, 3, 2000, 2000, 1400);
  linn::press(8, 3, 2000, 2000, 300);
  holdBoth(20);
  sent();
  linn::release(8, 3);
  linn::hold(5, 3, 2000, 2000, 1400, 5);
  std::vector<int> after = channelPressures(sent());
  for (int value : after) {
    CHECK(value > 0);
  }
  linn::release(5, 3);
  std::vector<int> end = channelPressures(sent());
  CHECK(!end.empty());
  if (!end.empty()) CHECK_EQ(end.back(), 0);
}

// a new note on the channel starts from what the touches already held press, not from silence
TEST(playing_one_channel_pressure_stays_up_when_another_note_starts) {
  linn::reset();
  oneChannelPressure();
  linn::press(5, 3, 2000, 2000, 1400);
  linn::hold(5, 3, 2000, 2000, 1400, 20);
  CHECK(!channelPressures(sent()).empty());
  linn::press(8, 3, 2000, 2000, 300);
  for (int value : channelPressures(sent())) {
    CHECK(value > 0);
  }
}

static int lastPressureAlone(byte col, uint16_t z) {
  linn::press(col, 3, 2000, 2000, z);
  linn::hold(col, 3, 2000, 2000, z, 20);
  std::vector<int> values = channelPressures(sent());
  linn::release(col, 3);
  sent();
  CHECK(!values.empty());
  return values.empty() ? -1 : values.back();
}

static int lastPressureOfBoth(byte firstCol, uint16_t firstZ, byte secondCol, uint16_t secondZ) {
  linn::press(firstCol, 3, 2000, 2000, firstZ);
  linn::press(secondCol, 3, 2000, 2000, secondZ);
  for (int i = 0; i < 20; ++i) {
    linn::hold(firstCol, 3, 2000, 2000, firstZ, 1);
    linn::hold(secondCol, 3, 2000, 2000, secondZ, 1);
  }
  std::vector<int> values = channelPressures(sent());
  CHECK(!values.empty());
  return values.empty() ? -1 : values.back();
}

TEST(playing_one_channel_pressure_can_follow_the_first_touch) {
  linn::reset();
  oneChannelPressure();
  Split[LEFT].sourceForZ = loudnessSourceFirst;
  int light = lastPressureAlone(8, 300);
  int both = lastPressureOfBoth(8, 300, 5, 1400);
  CHECK(abs(both - light) <= 2);
}

// touches in one row share its channel in Channel Per Row, so its pressure follows the same choice
TEST(playing_channel_per_row_pressure_follows_the_hardest_touch_of_a_row) {
  linn::reset();
  oneChannelPressure();
  Split[LEFT].midiMode = channelPerRow;
  updateSplitMidiChannels(LEFT);
  sent();
  int light = lastPressureAlone(8, 300);
  int both = lastPressureOfBoth(5, 1400, 8, 300);
  CHECK(both > light + 20);
}

// each row is a channel of its own in Channel Per Row, so a touch in another row never raises it
TEST(playing_channel_per_row_pressure_stays_apart_between_rows) {
  linn::reset();
  oneChannelPressure();
  Split[LEFT].midiMode = channelPerRow;
  updateSplitMidiChannels(LEFT);
  sent();
  linn::press(5, 2, 2000, 2000, 1400);
  linn::press(8, 3, 2000, 2000, 300);
  for (int i = 0; i < 20; ++i) {
    linn::hold(5, 2, 2000, 2000, 1400, 1);
    linn::hold(8, 3, 2000, 2000, 300, 1);
  }
  sent();
  unsigned short apart = sharedChannelPressure(LEFT, cell(8, 3).channel, false, 0);
  unsigned short hard = sharedChannelPressure(LEFT, cell(5, 2).channel, false, 0);
  CHECK(cell(8, 3).channel != cell(5, 2).channel);
  CHECK(apart + 100 < hard);
}

// a touch of the other split on the same channel is that split's own
TEST(playing_one_channel_pressure_leaves_the_other_split_out) {
  linn::reset();
  oneChannelPressure();
  Global.splitActive = true;
  Global.splitPoint = 12;
  Split[RIGHT].midiMode = oneChannel;
  Split[RIGHT].midiChanMain = Split[LEFT].midiChanMain;
  Split[RIGHT].expressionForZ = loudnessChannelPressure;
  Split[RIGHT].sendZ = true;
  updateSplitMidiChannels(RIGHT);
  sent();
  linn::press(15, 3, 2000, 2000, 1400);
  linn::press(8, 3, 2000, 2000, 300);
  for (int i = 0; i < 20; ++i) {
    linn::hold(15, 3, 2000, 2000, 1400, 1);
    linn::hold(8, 3, 2000, 2000, 300, 1);
  }
  sent();
  CHECK_EQ(cell(8, 3).channel, cell(15, 3).channel);
  unsigned short left = sharedChannelPressure(LEFT, cell(8, 3).channel, false, 0);
  unsigned short right = sharedChannelPressure(RIGHT, cell(15, 3).channel, false, 0);
  CHECK(left + 100 < right);
}

TEST(playing_one_channel_pressure_can_follow_the_last_touch) {
  linn::reset();
  oneChannelPressure();
  Split[LEFT].sourceForZ = loudnessSourceLast;
  int light = lastPressureAlone(8, 300);
  int both = lastPressureOfBoth(5, 1400, 8, 300);
  CHECK(abs(both - light) <= 2);
}
