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
// Notes left out of the rows: each Note Lights preset can leave pitches out, so that the rows
// play consecutive notes of a scale. The row offset and Transpose Lights count scale notes, an
// octave stays an octave, pitch slides follow the interval to the next cell, and guitar tuning, no
// overlap and the custom light patterns keep chromatic rows.

static const int SR_C_MAJOR = 0x54A;          // C# D# F# G# A# left out
static const int SR_C_PENTATONIC = 0xD6A;     // only C D E G A kept
static const int SR_ONLY_C = 0xFFE;

struct SrEvent {
  uint8_t status;
  uint8_t data1;
  uint8_t data2;
  int bend() const { return data1 | (data2 << 7); }
  bool isBend() const { return (status & 0xF0) == 0xE0; }
  bool isNoteOn() const { return (status & 0xF0) == 0x90 && data2 > 0; }
  bool isNoteOff() const { return (status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && data2 == 0); }
  bool isCC() const { return (status & 0xF0) == 0xB0; }
};

static std::vector<SrEvent> srSent() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  std::vector<SrEvent> events;
  for (size_t i = 0; i < out.size(); ) {
    uint8_t kind = out[i] & 0xF0;
    if (kind < 0x80) { ++i; continue; }
    if (kind == 0xC0 || kind == 0xD0) {
      events.push_back({out[i], i + 1 < out.size() ? out[i + 1] : (uint8_t)0, 0});
      i += 2;
    }
    else if (kind == 0xF0) {
      ++i;
    }
    else {
      if (i + 2 >= out.size()) break;
      events.push_back({out[i], out[i + 1], out[i + 2]});
      i += 3;
    }
  }
  return events;
}

static std::vector<int> srNoteOns(const std::vector<SrEvent>& events) {
  std::vector<int> notes;
  for (const SrEvent& e : events) if (e.isNoteOn()) notes.push_back(e.data1);
  return notes;
}

static std::vector<int> srNoteOffs(const std::vector<SrEvent>& events) {
  std::vector<int> notes;
  for (const SrEvent& e : events) if (e.isNoteOff()) notes.push_back(e.data1);
  return notes;
}

static std::vector<int> srBends(const std::vector<SrEvent>& events) {
  std::vector<int> bends;
  for (const SrEvent& e : events) if (e.isBend()) bends.push_back(e.bend());
  return bends;
}

static int srLastBend(const std::vector<SrEvent>& events) {
  std::vector<int> bends = srBends(events);
  return bends.empty() ? 8192 : bends.back();
}

static int srBendOf(double semitones) {
  return 8192 + (int)(semitones * 8192 / getBendRange(LEFT));
}

// a fresh instrument playing the left split, the given notes left out of the first note lights
// preset, a 24 semitone bend range so that a slide's size shows, and no pitch quantize
static void srSetup(int removed, byte model = 200) {
  linn::reset(model);
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = 90;
  Global.activeNotes = 0;
  Global.removedNotes[0] = removed;
  Split[LEFT].bendRangeOption = bendRange24;
  Split[LEFT].customBendRange = 24;
  Split[LEFT].pitchCorrectQuantize = false;
  Split[LEFT].pitchCorrectHold = pitchCorrectHoldOff;
  srSent();
}

static uint16_t srX(byte col) {
  return (uint16_t)FXD_TO_INT(calculateDefaultMeasuredX(col));
}

static void srPress(byte col, byte row) {
  linn::press(col, row, srX(col), 2000, 400);
}

// a held finger moved within its cell by a part of a cell
static void srMove(byte col, byte row, double cells) {
  uint16_t from = srX(col);
  int to = srX(col) + (int)(cells * (srX(col + 1) - srX(col)));
  for (int i = 1; i <= 20; ++i) {
    host::advanceMicros(10000);
    linn::hold(col, row, (uint16_t)(from + (to - from) * i / 20), 2000, 400, 1);
  }
}

// a finger sliding across whole cells, the pressure handing over from cell to cell as it crosses
// the grooves, then resting at the end long enough for pitch quantize to settle
static void srSlideAcross(byte col, byte row, int cells) {
  int from = srX(col), to = srX(col + cells);
  int steps = 30 * abs(cells);
  int lowest = cells < 0 ? col + cells - 1 : col - 1;
  int highest = cells < 0 ? col + 1 : col + cells + 1;
  for (int i = 1; i <= steps; ++i) {
    int x = from + (to - from) * i / steps;
    host::advanceMicros(10000);
    for (int c = lowest; c <= highest; ++c) {
      if (c < 1 || c >= NUMCOLS) continue;
      int z = 400 - abs(x - srX(c)) * 2;
      host::setSensor(c, row, (uint16_t)x, 2000, (uint16_t)(z > 140 ? z : 0));
      linn::scan(c, row);
    }
  }
  for (int i = 0; i < 40; ++i) {
    host::advanceMicros(10000);
    linn::hold(col + cells, row, (uint16_t)to, 2000, 400, 1);
  }
}

static int srNote(byte col, byte row, byte split = LEFT) {
  return transposedNote(split, col, row);
}

static bool srInMask(int note, int kept) {
  return (kept >> (((note % 12) + 12) % 12)) & 1;
}

static void srUseRowOffset(byte rowOffset, signed char custom = 12) {
  Global.rowOffset = rowOffset;
  Global.customRowOffset = custom;
}

// ---------------------------------------------------------------------------------------------
// the layout

TEST(scale_rows_play_consecutive_notes_of_the_scale) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  // the lowest cell would be F#2, which C major leaves out, so the rows start on G
  int expected[] = {31, 33, 35, 36, 38, 40, 41, 43, 45, 47, 48};
  for (byte col = 1; col <= 11; ++col) {
    CHECK_EQ(srNote(col, 0), expected[col - 1]);
  }
  for (byte row = 0; row < NUMROWS; ++row) {
    for (byte col = 1; col < NUMCOLS; ++col) {
      int note = srNote(col, row);
      if (note > 127) continue;
      CHECK(srInMask(note, ~SR_C_MAJOR));
      if (col > 1) CHECK(note > srNote(col - 1, row));
      if (col > 1) CHECK(note - srNote(col - 1, row) <= 2);
    }
  }
}

TEST(scale_rows_count_the_row_offset_in_scale_notes) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  CHECK_EQ(srNote(1, 1), 40);
  CHECK_EQ(srNote(1, 2), 48);
  srUseRowOffset(3);
  CHECK_EQ(srNote(1, 1), 36);
  CHECK_EQ(srNote(1, 7), 67);
  for (byte row = 1; row < NUMROWS; ++row) {
    for (byte col = 1; col + 3 < NUMCOLS; ++col) {
      CHECK_EQ(srNote(col, row), srNote(col + 3, row - 1));
    }
  }
  srUseRowOffset(7);
  CHECK_EQ(srNote(1, 1), 43);
}

TEST(scale_rows_keep_an_octave_between_rows_an_octave) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(ROWOFFSET_OCTAVECUSTOM, 12);
  for (byte row = 0; row < NUMROWS; ++row) {
    CHECK_EQ(srNote(1, row), 19 + 12 * row);
    CHECK_EQ(srNote(8, row), 31 + 12 * row);
  }
  Global.removedNotes[0] = SR_C_PENTATONIC;
  for (byte row = 1; row < NUMROWS; ++row) {
    for (byte col = 1; col < NUMCOLS; ++col) {
      if (srNote(col, row) > 127) continue;
      CHECK_EQ(srNote(col, row), srNote(col, row - 1) + 12);
    }
  }
  srUseRowOffset(ROWOFFSET_OCTAVECUSTOM, -12);
  for (byte row = 1; row < NUMROWS; ++row) {
    CHECK_EQ(srNote(1, row), srNote(1, row - 1) - 12);
  }
}

TEST(scale_rows_go_down_with_a_negative_custom_row_offset) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(ROWOFFSET_OCTAVECUSTOM, -5);
  CHECK_EQ(srNote(1, 0), 65);
  CHECK_EQ(srNote(1, 1), 57);
  for (byte row = 1; row < NUMROWS; ++row) {
    for (byte col = 6; col < NUMCOLS; ++col) {
      CHECK_EQ(srNote(col, row), srNote(col - 5, row - 1));
    }
  }
  srUseRowOffset(ROWOFFSET_OCTAVECUSTOM, -1);
  CHECK_EQ(srNote(1, 1), 64);
  CHECK_EQ(srNote(2, 1), 65);
}

TEST(scale_rows_repeat_with_a_zero_row_offset) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(ROWOFFSET_ZERO);
  for (byte row = 0; row < NUMROWS; ++row) {
    CHECK_EQ(srNote(1, row), 31);
    CHECK_EQ(srNote(4, row), 36);
  }
}

TEST(scale_rows_leave_guitar_tuning_and_no_overlap_chromatic) {
  byte offsets[] = {ROWOFFSET_GUITAR, ROWOFFSET_NOOVERLAP, ROWOFFSET_OCTAVECUSTOM};
  for (byte o : offsets) {
    srSetup(0);
    srUseRowOffset(o, -17);
    Global.splitActive = true;
    int chromatic[NUMROWS][MAXCOLS];
    for (byte row = 0; row < NUMROWS; ++row)
      for (byte col = 1; col < NUMCOLS; ++col) chromatic[row][col] = srNote(col, row, getSplitOf(col));
    Global.removedNotes[0] = SR_C_MAJOR;
    for (byte row = 0; row < NUMROWS; ++row)
      for (byte col = 1; col < NUMCOLS; ++col) CHECK_EQ(srNote(col, row, getSplitOf(col)), chromatic[row][col]);
  }
}

// nothing left out, or everything, which only a MIDI message can do, plays the rows chromatically,
// and so do the three custom light patterns, which have no scale
TEST(scale_rows_need_a_scale_preset_with_something_left_out) {
  int masks[] = {0, 0xFFF};
  for (int mask : masks) {
    srSetup(0);
    srUseRowOffset(5);
    int chromatic[NUMROWS][MAXCOLS];
    for (byte row = 0; row < NUMROWS; ++row)
      for (byte col = 1; col < NUMCOLS; ++col) chromatic[row][col] = srNote(col, row);
    Global.removedNotes[0] = mask;
    for (byte row = 0; row < NUMROWS; ++row)
      for (byte col = 1; col < NUMCOLS; ++col) CHECK_EQ(srNote(col, row), chromatic[row][col]);
    CHECK_EQ(srNote(1, 0), 30);
  }

  srSetup(0);
  Global.activeNotes = 9;
  Global.removedNotes[9] = SR_C_MAJOR;
  CHECK_EQ(srNote(1, 0), 30);
  CHECK_EQ(srNote(2, 0), 31);
  CHECK_EQ(removedScaleNotes(), 0);
  Global.activeNotes = 8;
  Global.removedNotes[8] = SR_C_MAJOR;
  CHECK_EQ(srNote(1, 0), 31);
}

TEST(scale_rows_belong_to_their_note_lights_preset) {
  srSetup(0);
  Global.removedNotes[3] = SR_C_PENTATONIC;
  CHECK_EQ(srNote(2, 0), 31);
  Global.activeNotes = 3;
  CHECK_EQ(srNote(1, 0), 31);
  CHECK_EQ(srNote(2, 0), 33);
  CHECK_EQ(srNote(3, 0), 36);
}

TEST(scale_rows_transpose_by_semitones_on_top_of_the_scale) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  Split[LEFT].transposePitch = 2;
  CHECK_EQ(srNote(1, 0), 33);
  CHECK_EQ(srNote(4, 0), 38);
  Split[LEFT].transposePitch = 0;
  Split[LEFT].transposeOctave = 12;
  CHECK_EQ(srNote(1, 0), 43);
  CHECK_EQ(srNote(4, 0), 48);
}

// Transpose Lights with the same Transpose Pitch moves the scale to another key, as it moves the lights
TEST(scale_rows_follow_the_transposed_lights_into_another_key) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  Split[LEFT].transposeLights = 2;
  Split[LEFT].transposePitch = 2;
  int dMajor = ~(SR_C_MAJOR << 2 | SR_C_MAJOR >> 10) & 0xFFF;
  for (byte col = 1; col < NUMCOLS; ++col) {
    CHECK(srInMask(srNote(col, 0), dMajor));
    if (col > 1) CHECK(srNote(col, 0) - srNote(col - 1, 0) <= 2);
  }
  CHECK_EQ(srNote(1, 0), 30);
  CHECK_EQ(srNote(2, 0), 31);
  CHECK_EQ(srNote(5, 0), 37);
}

TEST(scale_rows_follow_the_left_handed_layout) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  int rightHanded[NUMROWS][MAXCOLS];
  for (byte row = 0; row < NUMROWS; ++row)
    for (byte col = 1; col < NUMCOLS; ++col) rightHanded[row][col] = srNote(col, row);
  Device.otherHanded = true;
  Device.splitHandedness = reversedBoth;
  for (byte row = 0; row < NUMROWS; ++row)
    for (byte col = 1; col < NUMCOLS; ++col) CHECK_EQ(srNote(col, row), rightHanded[row][NUMCOLS - col]);
}

TEST(scale_rows_fit_the_128) {
  srSetup(SR_C_MAJOR, 128);
  srUseRowOffset(5);
  CHECK_EQ(NUMCOLS, 17);
  CHECK_EQ(srNote(1, 0), 31);
  CHECK_EQ(srNote(16, 0), 57);
  Device.otherHanded = true;
  Device.splitHandedness = reversedBoth;
  CHECK_EQ(srNote(16, 0), 31);
  CHECK_EQ(srNote(1, 0), 57);
}

TEST(scale_rows_carry_on_into_the_right_split) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  Global.splitActive = true;
  Global.splitPoint = 12;
  for (byte col = 1; col < NUMCOLS; ++col) {
    CHECK_EQ(srNote(col, 0, getSplitOf(col)), srNote(col, 0, LEFT));
  }
  // the right split's own lights transposition moves its rows to start on E, whatever the left
  // split's is
  Split[RIGHT].transposeLights = 2;
  CHECK_EQ(srNote(13, 0, RIGHT), 48);
  CHECK_EQ(srNote(13, 0, LEFT), 52);
  Split[RIGHT].transposePitch = 2;
  CHECK(srInMask(srNote(13, 0, RIGHT), ~(SR_C_MAJOR << 2 | SR_C_MAJOR >> 10)));
}

// a wide scale runs past the top of the MIDI notes and those cells play nothing, without the
// note number wrapping around into the playable range
TEST(scale_rows_past_the_midi_range_are_off) {
  srSetup(SR_ONLY_C);
  srUseRowOffset(5);
  CHECK_EQ(srNote(1, 0), 36);
  CHECK_EQ(srNote(2, 0), 48);
  CHECK_EQ(srNote(8, 0), 120);
  for (byte row = 0; row < NUMROWS; ++row) {
    for (byte col = 1; col < NUMCOLS; ++col) {
      int expected = 36 + 12 * (5 * row + col - 1);
      CHECK_EQ(srNote(col, row), expected <= 255 ? expected : 255);
    }
  }
  srPress(20, 0);
  CHECK_EQ(srNoteOns(srSent()).size(), 0);
  linn::release(20, 0);

  srUseRowOffset(ROWOFFSET_OCTAVECUSTOM, -16);
  for (byte row = 1; row < NUMROWS; ++row) {
    CHECK(srNote(1, row) < 0 || srNote(1, row) > 127);
  }
  srPress(1, 7);
  CHECK_EQ(srNoteOns(srSent()).size(), 0);
}

TEST(scale_rows_send_the_cells_notes) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  srPress(4, 0);
  srPress(6, 2);
  std::vector<int> ons = srNoteOns(srSent());
  CHECK_EQ(ons.size(), 2);
  if (ons.size() == 2) {
    CHECK_EQ(ons[0], 36);
    CHECK_EQ(ons[1], srNote(6, 2));
    CHECK_EQ(ons[1], 57);
  }
  linn::release(4, 0);
  linn::release(6, 2);
  std::vector<int> offs = srNoteOffs(srSent());
  CHECK_EQ(offs.size(), 2);
  if (offs.size() == 2) {
    CHECK_EQ(offs[0], 36);
    CHECK_EQ(offs[1], 57);
  }
}

// a held note keeps its number when the scale changes under it, so its note off matches
TEST(scale_rows_change_while_a_note_is_held) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  srPress(5, 0);
  CHECK_EQ(srNoteOns(srSent()).size(), 1);
  receivedNrpn(279, 0, 0);
  receivedNrpn(278, 1, 0);
  CHECK(srNote(5, 0) != 38);
  linn::release(5, 0);
  std::vector<int> offs = srNoteOffs(srSent());
  CHECK_EQ(offs.size(), 1);
  if (offs.size() == 1) CHECK_EQ(offs[0], 38);
}

// ---------------------------------------------------------------------------------------------
// finding the cells of a note, which lights the played notes and those that come in over MIDI

TEST(scale_rows_find_the_cells_of_every_note) {
  int masks[] = {SR_C_MAJOR, SR_C_PENTATONIC, 0x555, SR_ONLY_C};
  byte offsets[] = {3, 5, 7, ROWOFFSET_OCTAVECUSTOM, ROWOFFSET_ZERO};
  signed char customs[] = {12, -5, -12, 16};
  byte models[] = {200, 128};
  for (byte model : models) {
    for (int mask : masks) {
      for (byte offset : offsets) {
        for (signed char custom : customs) {
          if (offset != ROWOFFSET_OCTAVECUSTOM && custom != 12) continue;
          for (int handed = 0; handed < 2; ++handed) {
            srSetup(mask, model);
            srUseRowOffset(offset, custom);
            Device.otherHanded = handed;
            Device.splitHandedness = reversedBoth;
            Split[LEFT].transposePitch = 3;
            Split[LEFT].transposeLights = -2;
            Split[LEFT].transposeOctave = 12;
            int checked = 0;
            for (byte row = 0; row < NUMROWS; ++row) {
              for (byte col = 1; col < NUMCOLS; ++col) {
                int note = srNote(col, row);
                if (note < 0 || note > 127) continue;
                CHECK_EQ(getNoteNumColumn(LEFT, note, row), col);
                ++checked;
              }
            }
            CHECK(checked > 0);
          }
        }
      }
    }
  }
}

TEST(scale_rows_find_no_cell_for_a_note_left_out) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  for (byte row = 0; row < NUMROWS; ++row) {
    for (int note = 30; note < 100; ++note) {
      if (!srInMask(note, ~SR_C_MAJOR)) CHECK_EQ(getNoteNumColumn(LEFT, note, row), -1);
    }
  }
}

TEST(scale_rows_light_the_other_cells_of_a_played_note) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(3);
  Split[LEFT].playedTouchMode = playedSame;
  updateDisplay();
  srPress(4, 0);
  CHECK_EQ(getLedColor(1, 1, LED_LAYER_PLAYED), Split[LEFT].colorPlayed);
  CHECK_EQ(getLedColor(4, 0, LED_LAYER_PLAYED), Split[LEFT].colorPlayed);
  CHECK_EQ(getLedColor(2, 1, LED_LAYER_PLAYED), COLOR_OFF);
  linn::release(4, 0);
  CHECK_EQ(getLedColor(1, 1, LED_LAYER_PLAYED), COLOR_OFF);
}

TEST(scale_rows_paint_only_notes_of_the_scale) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  Global.mainNotes[0] = 1 << 7;
  Global.accentNotes[0] = 1;
  updateDisplay();
  // the first row runs G A B C, where chromatic rows would run F# G G# A
  CHECK_EQ(getLedColor(1, 0, LED_LAYER_MAIN), Split[LEFT].colorMain);
  CHECK_EQ(getLedColor(4, 0, LED_LAYER_MAIN), Split[LEFT].colorAccent);
  for (byte row = 0; row < NUMROWS; ++row) {
    for (byte col = 1; col < NUMCOLS; ++col) {
      int pc = srNote(col, row) % 12;
      byte expected = pc == 0 ? Split[LEFT].colorAccent : pc == 7 ? Split[LEFT].colorMain : COLOR_OFF;
      if (srNote(col, row) > 127) expected = COLOR_OFF;
      CHECK_EQ(getLedColor(col, row, LED_LAYER_MAIN), expected);
    }
  }
}

// ---------------------------------------------------------------------------------------------
// pitch slides

TEST(scale_rows_slide_by_the_interval_to_the_next_cell) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  srPress(3, 0);
  srSlideAcross(3, 0, 1);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(1), 8);
  linn::release(4, 0);
  srSent();

  srPress(4, 0);
  srSlideAcross(4, 0, 1);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(2), 8);
  linn::release(5, 0);
  srSent();

  srPress(5, 0);
  srSlideAcross(5, 0, -2);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(-3), 8);
}

TEST(scale_rows_slide_part_of_the_way_to_part_of_the_interval) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  srPress(4, 0);
  srMove(4, 0, 0.5);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(1), 10);
  srMove(4, 0, 0.25);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(0.5), 10);
}

// vibrato follows the interval on each side, so it's wider towards the bigger step
TEST(scale_rows_widen_vibrato_towards_the_bigger_step) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  srPress(6, 0);
  srMove(6, 0, 0.25);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(0.25), 10);
  srMove(6, 0, -0.25);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(-0.5), 10);
}

TEST(scale_rows_slide_the_same_as_chromatic_rows_without_a_scale) {
  srSetup(0);
  srPress(6, 0);
  srMove(6, 0, 0.25);
  int chromaticUp = srLastBend(srSent());
  srMove(6, 0, -0.25);
  int chromaticDown = srLastBend(srSent());
  CHECK_NEAR(chromaticUp, srBendOf(0.25), 10);
  CHECK_NEAR(chromaticDown, srBendOf(-0.25), 10);
}

TEST(scale_rows_slide_with_the_left_handed_layout) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  Device.otherHanded = true;
  Device.splitHandedness = reversedBoth;
  CHECK_EQ(srNote(22, 0), 36);
  srPress(22, 0);
  srSlideAcross(22, 0, -1);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(2), 8);
  linn::release(21, 0);
  srSent();
  srPress(22, 0);
  srSlideAcross(22, 0, 1);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(-1), 8);
}

TEST(scale_rows_slide_on_the_128) {
  srSetup(SR_C_MAJOR, 128);
  srUseRowOffset(5);
  srPress(4, 0);
  srSlideAcross(4, 0, 1);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(2), 8);
  linn::release(5, 0);

  srSetup(SR_C_MAJOR, 128);
  srUseRowOffset(5);
  Device.otherHanded = true;
  Device.splitHandedness = reversedBoth;
  srPress(13, 0);
  srSlideAcross(13, 0, -1);
  CHECK_NEAR(srLastBend(srSent()), srBendOf(2), 8);
}

// with pitch quantize on, a slide that comes to rest lands exactly on the next cell's note
TEST(scale_rows_quantize_a_slide_to_the_next_note) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  Split[LEFT].pitchCorrectQuantize = true;
  Split[LEFT].pitchCorrectHold = pitchCorrectHoldMedium;
  linn::press(4, 0, srX(4) + 30, 2000, 400);
  srSlideAcross(4, 0, 1);
  for (int i = 0; i < 100; ++i) {
    host::advanceMicros(10000);
    linn::hold(5, 0, srX(5) + 20, 2000, 400, 1);
  }
  CHECK_NEAR(srLastBend(srSent()), srBendOf(2), 12);
}

// a slide over steps of an octave runs into the end of the bend range and stays there, whatever
// the bend range, instead of overflowing back the other way
TEST(scale_rows_slide_stops_at_the_end_of_the_bend_range) {
  byte ranges[] = {2, 7, 24, 96};
  for (byte range : ranges) {
    srSetup(SR_ONLY_C);
    srUseRowOffset(5);
    Split[LEFT].customBendRange = range;
    srPress(1, 0);
    srSlideAcross(1, 0, 3);
    std::vector<int> bends = srBends(srSent());
    CHECK(!bends.empty());
    for (size_t i = 1; i < bends.size(); ++i) CHECK(bends[i] >= bends[i - 1] - 2);
    if (!bends.empty()) CHECK_NEAR(bends.back(), min(16383, srBendOf(36)), 8);
    linn::release(4, 0);
    srSent();
  }
}

TEST(scale_rows_leave_the_low_row_bend_chromatic) {
  int bends[2];
  for (int scale = 0; scale < 2; ++scale) {
    srSetup(scale ? SR_C_MAJOR : 0);
    srUseRowOffset(5);
    Split[LEFT].lowRowMode = lowRowBend;
    Split[LEFT].lowRowBendBehavior = lowRowBendBend;
    srPress(5, 0);
    srMove(5, 0, 0.25);
    bends[scale] = srLastBend(srSent());
    linn::release(5, 0);
  }
  CHECK(bends[0] != 8192);
  CHECK_EQ(bends[1], bends[0]);
}

TEST(scale_rows_leave_user_firmware_mode_x_alone) {
  std::vector<std::vector<int>> values(2);
  for (int scale = 0; scale < 2; ++scale) {
    srSetup(0);
    linn::midiInNrpn(245, 1, 0);
    linn::settle();
    Global.removedNotes[0] = scale ? SR_C_MAJOR : 0;
    linn::midiIn({0xB0, 10, 1});
    srSent();
    srPress(5, 0);
    srMove(5, 0, 0.4);
    for (const SrEvent& e : srSent()) if (e.isCC() && e.data1 == 5 + 32) values[scale].push_back(e.data2);
    linn::release(5, 0);
  }
  CHECK(values[0].size() > 3);
  CHECK(values[0] == values[1]);
}

// ---------------------------------------------------------------------------------------------
// the Global Settings screen: a tap toggles a light when the finger lifts, holding leaves the
// note out of the rows or brings it back

static void srOpenNoteLights(byte lights) {
  setDisplayMode(displayGlobal);
  lightSettings = lights;
  updateDisplay();
  srSent();
}

static void srTap(byte col, byte row) {
  linn::press(col, row, srX(col), 2000, 400);
  linn::release(col, row);
}

static void srHold(byte col, byte row) {
  linn::press(col, row, srX(col), 2000, 400);
  for (int i = 0; i < 30; ++i) {
    host::advanceMicros(50000);
    linn::hold(col, row, srX(col), 2000, 400, 1);
  }
  linn::release(col, row);
}

static byte srCellDisplay(byte col, byte row) {
  return ledVisible(LED_LAYER_MAIN, col, row) & B00000111;
}

TEST(scale_rows_tapping_a_note_light_toggles_it_when_the_finger_lifts) {
  srSetup(0);
  srOpenNoteLights(LIGHTS_MAIN);
  int before = Global.mainNotes[0];
  linn::press(3, 0, srX(3), 2000, 400);
  CHECK_EQ(Global.mainNotes[0], before);
  linn::release(3, 0);
  CHECK_EQ(Global.mainNotes[0], before ^ (1 << 1));
  CHECK_EQ(Global.removedNotes[0], 0);
  srTap(3, 0);
  CHECK_EQ(Global.mainNotes[0], before);

  // lifted after the tap's time and before the hold's, it does neither
  linn::press(3, 0, srX(3), 2000, 400);
  for (int i = 0; i < 10; ++i) {
    host::advanceMicros(50000);
    linn::hold(3, 0, srX(3), 2000, 400, 1);
  }
  linn::release(3, 0);
  CHECK_EQ(Global.mainNotes[0], before);
  CHECK_EQ(Global.removedNotes[0], 0);

  srOpenNoteLights(LIGHTS_ACCENT);
  int accent = Global.accentNotes[0];
  srTap(2, 1);
  CHECK_EQ(Global.accentNotes[0], accent ^ (1 << 3));
  CHECK_EQ(Global.mainNotes[0], before);
}

TEST(scale_rows_holding_a_note_light_leaves_the_note_out_and_back_in) {
  srSetup(0);
  srOpenNoteLights(LIGHTS_MAIN);
  int lights = Global.mainNotes[0];

  // pulsing in its own colour while held, as every cell waiting for a hold does, then steady red
  // once left out
  linn::press(3, 0, srX(3), 2000, 400);
  for (int i = 0; i < 10; ++i) {
    host::advanceMicros(50000);
    linn::hold(3, 0, srX(3), 2000, 400, 1);
  }
  CHECK_EQ(Global.removedNotes[0], 0);
  CHECK_EQ(getLedColor(3, 0, LED_LAYER_MAIN), globalColor);
  CHECK_EQ(srCellDisplay(3, 0), cellSlowPulse);
  CHECK_EQ(getLedColor(2, 0, LED_LAYER_MAIN), globalColor);
  CHECK_EQ(srCellDisplay(2, 0), cellOn);
  for (int i = 0; i < 20; ++i) {
    host::advanceMicros(50000);
    linn::hold(3, 0, srX(3), 2000, 400, 1);
  }
  CHECK_EQ(Global.removedNotes[0], 1 << 1);
  CHECK_EQ(srCellDisplay(3, 0), cellOn);
  linn::release(3, 0);
  CHECK_EQ(Global.mainNotes[0], lights);
  CHECK_EQ(getLedColor(3, 0, LED_LAYER_MAIN), COLOR_RED);
  CHECK_EQ(srCellDisplay(3, 0), cellOn);
  CHECK_EQ(getLedColor(2, 0, LED_LAYER_MAIN), globalColor);
  CHECK_EQ(srCellDisplay(2, 0), cellOn);

  // the accent lights share what's left out
  srOpenNoteLights(LIGHTS_ACCENT);
  CHECK_EQ(getLedColor(3, 0, LED_LAYER_MAIN), COLOR_RED);
  srHold(4, 3);
  CHECK_EQ(Global.removedNotes[0], (1 << 1) | (1 << 11));

  // a note left out pulses red while held to bring it back
  srOpenNoteLights(LIGHTS_MAIN);
  linn::press(3, 0, srX(3), 2000, 400);
  CHECK_EQ(getLedColor(3, 0, LED_LAYER_MAIN), COLOR_RED);
  CHECK_EQ(srCellDisplay(3, 0), cellSlowPulse);
  for (int i = 0; i < 30; ++i) {
    host::advanceMicros(50000);
    linn::hold(3, 0, srX(3), 2000, 400, 1);
  }
  linn::release(3, 0);
  CHECK_EQ(Global.removedNotes[0], 1 << 11);
  CHECK_EQ(Global.mainNotes[0], lights);
  CHECK(getLedColor(3, 0, LED_LAYER_MAIN) != COLOR_RED);
}

// a tap brings a note left out back into the rows, lit or unlit as its light already was
TEST(scale_rows_tapping_a_note_left_out_brings_it_back_with_its_light) {
  srSetup((1 << 1) | (1 << 2));
  srOpenNoteLights(LIGHTS_MAIN);
  int lights = Global.mainNotes[0];
  CHECK(lights & (1 << 2));
  CHECK(!(lights & (1 << 1)));
  srTap(4, 0);
  CHECK_EQ(Global.removedNotes[0], 1 << 1);
  CHECK_EQ(Global.mainNotes[0], lights);
  CHECK_EQ(getLedColor(4, 0, LED_LAYER_MAIN), globalColor);
  CHECK_EQ(srCellDisplay(4, 0), cellOn);
  srTap(3, 0);
  CHECK_EQ(Global.removedNotes[0], 0);
  CHECK_EQ(Global.mainNotes[0], lights);
  CHECK(getLedColor(3, 0, LED_LAYER_MAIN) != COLOR_RED);
  CHECK(srCellDisplay(3, 0) != cellOn);
}

// holding MAIN or ACCENT leaves out every note that neither the main nor the accent lights show,
// the cells that are dark on the playing surface, and holding again brings back all that's left out
static const int SR_MAIN_C_MAJOR = 0xAB5;
static const int SR_UNLIT_BUT_C_SHARP = 0xFFF & ~(SR_MAIN_C_MAJOR | (1 << 1));

static void srLightPreset(int main, int accent) {
  Global.mainNotes[0] = main;
  Global.accentNotes[0] = accent;
}

TEST(scale_rows_holding_main_leaves_out_the_dark_notes_and_brings_them_back) {
  srSetup(0);
  srLightPreset(SR_MAIN_C_MAJOR, 1 << 1);
  srOpenNoteLights(LIGHTS_ACCENT);

  srTap(1, 0);
  CHECK_EQ(lightSettings, LIGHTS_MAIN);
  CHECK_EQ(Global.removedNotes[0], 0);

  linn::press(1, 0, srX(1), 2000, 400);
  for (int i = 0; i < 10; ++i) {
    host::advanceMicros(50000);
    linn::hold(1, 0, srX(1), 2000, 400, 1);
  }
  CHECK_EQ(Global.removedNotes[0], 0);
  CHECK_EQ(getLedColor(1, 0, LED_LAYER_MAIN), globalColor);
  CHECK_EQ(srCellDisplay(1, 0), cellSlowPulse);
  for (int i = 0; i < 20; ++i) {
    host::advanceMicros(50000);
    linn::hold(1, 0, srX(1), 2000, 400, 1);
  }
  CHECK_EQ(Global.removedNotes[0], SR_UNLIT_BUT_C_SHARP);
  linn::release(1, 0);
  CHECK_EQ(Global.removedNotes[0], SR_UNLIT_BUT_C_SHARP);
  CHECK_EQ(Global.mainNotes[0], SR_MAIN_C_MAJOR);
  CHECK_EQ(Global.accentNotes[0], 1 << 1);
  CHECK_EQ(srCellDisplay(1, 0), cellOn);
  CHECK_EQ(getLedColor(2, 1, LED_LAYER_MAIN), COLOR_RED);
  CHECK(getLedColor(3, 0, LED_LAYER_MAIN) != COLOR_RED);
  CHECK_EQ(srNote(1, 0), 31);
  CHECK_EQ(srNote(2, 0), 33);
  CHECK_EQ(srNote(3, 0), 35);
  CHECK_EQ(srNote(4, 0), 36);
  CHECK_EQ(srNote(5, 0), 37);

  srHold(1, 0);
  CHECK_EQ(Global.removedNotes[0], 0);

  // anything left out, by hand or not, is all brought back
  Global.removedNotes[0] = 1 << 4;
  srHold(1, 0);
  CHECK_EQ(Global.removedNotes[0], 0);
  CHECK_EQ(Global.mainNotes[0], SR_MAIN_C_MAJOR);
}

TEST(scale_rows_holding_accent_leaves_out_the_same_dark_notes) {
  srSetup(0);
  srLightPreset(SR_MAIN_C_MAJOR, 1 << 1);
  srOpenNoteLights(LIGHTS_ACCENT);
  srHold(1, 1);
  CHECK_EQ(lightSettings, LIGHTS_ACCENT);
  CHECK_EQ(Global.removedNotes[0], SR_UNLIT_BUT_C_SHARP);
  srHold(1, 1);
  CHECK_EQ(Global.removedNotes[0], 0);
}

// a settings preset loaded after a custom pattern leaves the panel's flag set, so MAIN can't be
// selected, and holding it longer must not change the rows behind the Active view
TEST(scale_rows_holding_main_needs_the_view_its_touch_selected) {
  srSetup(0);
  srLightPreset(SR_MAIN_C_MAJOR, 0);
  customLedPatternActive = true;
  srOpenNoteLights(LIGHTS_ACTIVE);
  linn::press(1, 0, srX(1), 2000, 400);
  CHECK_EQ(lightSettings, LIGHTS_ACTIVE);
  CHECK(srCellDisplay(1, 0) != cellSlowPulse);
  for (int i = 0; i < 30; ++i) {
    host::advanceMicros(50000);
    linn::hold(1, 0, srX(1), 2000, 400, 1);
  }
  linn::release(1, 0);
  CHECK_EQ(Global.removedNotes[0], 0);
  customLedPatternActive = false;
}

TEST(scale_rows_holding_main_does_nothing_with_every_light_on_or_off) {
  srSetup(0);
  srLightPreset(0xFFF, 0);
  srOpenNoteLights(LIGHTS_MAIN);
  srHold(1, 0);
  CHECK_EQ(Global.removedNotes[0], 0);

  srLightPreset(0, 0);
  srOpenNoteLights(LIGHTS_MAIN);
  srHold(1, 0);
  CHECK_EQ(Global.removedNotes[0], 0);
}

TEST(scale_rows_keep_at_least_one_note) {
  srSetup(SR_ONLY_C);
  srOpenNoteLights(LIGHTS_MAIN);
  srHold(2, 0);
  CHECK_EQ(Global.removedNotes[0], SR_ONLY_C);
  srHold(3, 0);
  CHECK_EQ(Global.removedNotes[0], SR_ONLY_C & ~(1 << 1));
  srHold(2, 0);
  CHECK_EQ(Global.removedNotes[0], (SR_ONLY_C & ~(1 << 1)) | 1);
}

TEST(scale_rows_cannot_be_set_on_a_custom_light_pattern) {
  srSetup(0);
  setDisplayMode(displayGlobal);
  lightSettings = LIGHTS_ACTIVE;
  updateDisplay();
  srTap(2, 3);
  CHECK_EQ(Global.activeNotes, 9);
  CHECK(customLedPatternActive);
  lightSettings = LIGHTS_MAIN;
  updateDisplay();
  int lights = Global.mainNotes[9];
  srHold(3, 0);
  srTap(4, 0);
  srHold(1, 0);
  CHECK_EQ(Global.removedNotes[9], 0);
  CHECK_EQ(Global.mainNotes[9], lights);

  // a preset loaded onto a custom pattern without its layer, where the panel's flag goes stale,
  // still can't leave notes out that the rows would ignore, nor show them
  customLedPatternActive = false;
  srHold(3, 0);
  CHECK_EQ(Global.removedNotes[9], 0);
  Global.mainNotes[9] = 1;
  linn::press(1, 0, srX(1), 2000, 400);
  CHECK(srCellDisplay(1, 0) != cellSlowPulse);
  linn::release(1, 0);
  srHold(1, 0);
  CHECK_EQ(Global.removedNotes[9], 0);
  Global.removedNotes[9] = 1 << 2;
  updateDisplay();
  CHECK(getLedColor(4, 0, LED_LAYER_MAIN) != COLOR_RED);
}

TEST(scale_rows_selecting_a_preset_still_happens_at_the_touch) {
  srSetup(0);
  srOpenNoteLights(LIGHTS_ACTIVE);
  linn::press(3, 1, srX(3), 2000, 400);
  CHECK_EQ(Global.activeNotes, 4);
  linn::release(3, 1);
  srHold(4, 1);
  CHECK_EQ(Global.activeNotes, 5);
  for (int n = 0; n < 12; ++n) CHECK_EQ(Global.removedNotes[n], 0);
}

TEST(scale_rows_cannot_be_edited_in_user_firmware_mode) {
  srSetup(0);
  linn::midiInNrpn(245, 1, 0);
  linn::settle();
  srOpenNoteLights(LIGHTS_MAIN);
  int lights = Global.mainNotes[0];
  srTap(3, 0);
  srHold(4, 0);
  srHold(1, 0);
  CHECK_EQ(Global.mainNotes[0], lights);
  CHECK_EQ(Global.removedNotes[0], 0);
}

// ---------------------------------------------------------------------------------------------
// MIDI, presets, the sequencer

static int srQuery(int parameter) {
  srSent();
  sendNrpnParameter(parameter, 0);
  int value = -1;
  for (const SrEvent& e : srSent()) if (e.isCC() && e.data1 == 38) value = e.data2;
  return value;
}

TEST(scale_rows_nrpn_276_to_287_leave_notes_out_of_the_active_preset) {
  srSetup(0);
  Global.activeNotes = 2;
  receivedNrpn(276, 1, 0);
  receivedNrpn(287, 1, 0);
  CHECK_EQ(Global.removedNotes[2], 1 | (1 << 11));
  CHECK_EQ(Global.removedNotes[0], 0);
  CHECK_EQ(srQuery(276), 1);
  CHECK_EQ(srQuery(277), 0);
  CHECK_EQ(srQuery(287), 1);
  receivedNrpn(287, 0, 0);
  CHECK_EQ(Global.removedNotes[2], 1);
  CHECK_EQ(srQuery(287), 0);
  receivedNrpn(280, 2, 0);
  CHECK_EQ(Global.removedNotes[2], 1);
  Global.activeNotes = 0;
  CHECK_EQ(srQuery(276), 0);

  // never the last note still played, as the panel refuses it
  Global.removedNotes[0] = 0x7ff;
  receivedNrpn(287, 1, 0);
  CHECK_EQ(Global.removedNotes[0], 0x7ff);

  // nothing for a custom light pattern, which has no notes to leave out
  Global.activeNotes = 9;
  receivedNrpn(276, 1, 0);
  CHECK_EQ(Global.removedNotes[9], 0);
}

TEST(scale_rows_are_stored_in_the_settings_presets) {
  srSetup(SR_C_MAJOR);
  Global.removedNotes[5] = SR_C_PENTATONIC;
  storeSettingsToPreset(1);
  Global.removedNotes[0] = 0;
  Global.removedNotes[5] = 0;
  loadSettingsFromPreset(1);
  CHECK_EQ(Global.removedNotes[0], SR_C_MAJOR);
  CHECK_EQ(Global.removedNotes[5], SR_C_PENTATONIC);
}

TEST(scale_rows_leave_notes_out_of_the_sequencer_scale_view) {
  srSetup(0);
  Split[LEFT].sequencerView = sequencerScales;
  seqState[LEFT].rowOffset = 0;
  CHECK_EQ(seqState[LEFT].getRowNoteNum(0), 48);
  CHECK_EQ(seqState[LEFT].getRowNoteNum(1), 50);
  CHECK_EQ(seqState[LEFT].getRowNoteNum(2), 52);
  Global.removedNotes[0] = 1 << 2;
  CHECK_EQ(seqState[LEFT].getRowNoteNum(0), 48);
  CHECK_EQ(seqState[LEFT].getRowNoteNum(1), 52);
  CHECK_EQ(seqState[LEFT].getRowNoteNum(2), 53);
}

// ---------------------------------------------------------------------------------------------
// Transpose Lights, counted in pads: with a row offset of +5 the first row of chromatic rows starts
// on F#, 30, so in C major it starts on G, and each Lights value moves that start by a note of the
// scale, as many as there are pads in the value, the way chromatic rows move by a semitone a pad

static int srScaleNoteAbove(int note, int kept) {
  do {
    note++;
  } while (!srInMask(note, kept));
  return note;
}

TEST(scale_rows_transpose_lights_move_the_layout_by_pads) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  int kept = ~SR_C_MAJOR & 0xFFF;
  CHECK_EQ(srNote(1, 0), 31);
  // C into the corner, three pads along G A B C
  Split[LEFT].transposeLights = -3;
  CHECK_EQ(srNote(1, 0), 36);
  Split[LEFT].transposeLights = 1;
  CHECK_EQ(srNote(1, 0), 29);
  Split[LEFT].transposeLights = 3;
  CHECK_EQ(srNote(1, 0), 26);
  Split[LEFT].transposeLights = -7;
  CHECK_EQ(srNote(1, 0), 43);
  // every value a pad from the next, none of them the same
  for (int lights = 7; lights > -7; --lights) {
    Split[LEFT].transposeLights = lights;
    int start = srNote(1, 0);
    Split[LEFT].transposeLights = lights - 1;
    CHECK_EQ(srNote(1, 0), srScaleNoteAbove(start, kept));
    for (byte col = 2; col < NUMCOLS; ++col) CHECK_EQ(srNote(col, 0), srScaleNoteAbove(srNote(col - 1, 0), kept));
  }
}

// the value chosen is kept and counts pads of whichever scale is there, or semitones without one
TEST(scale_rows_transpose_lights_follow_the_scale_as_it_changes) {
  srSetup(SR_C_MAJOR);
  srUseRowOffset(5);
  Split[LEFT].transposeLights = -3;
  CHECK_EQ(srNote(1, 0), 36);
  Global.removedNotes[0] = SR_C_PENTATONIC;
  CHECK_EQ(srNote(1, 0), 38);
  Split[LEFT].transposeLights = 2;
  CHECK_EQ(srNote(1, 0), 26);
  Global.removedNotes[0] = SR_ONLY_C;
  CHECK_EQ(srNote(1, 0), 12);
  Global.removedNotes[0] = 0;
  CHECK_EQ(srNote(1, 0), 28);
  CHECK_EQ(Split[LEFT].transposeLights, 2);
}
