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
// Note layouts beyond the default row offset.

TEST(layout_guitar_tuning_uses_the_per_row_notes) {
  linn::reset();
  Global.rowOffset = ROWOFFSET_GUITAR;
  for (byte row = 0; row < NUMROWS; ++row) {
    CHECK_EQ(getNoteNumber(LEFT, 1, row), Global.guitarTuning[row]);
    CHECK_EQ(getNoteNumber(LEFT, 3, row), Global.guitarTuning[row] + 2);
  }
}

TEST(layout_no_overlap_offsets_each_row_by_the_width) {
  linn::reset();
  Global.rowOffset = ROWOFFSET_NOOVERLAP;
  CHECK_EQ(getNoteNumber(LEFT, 1, 1) - getNoteNumber(LEFT, 1, 0), NUMCOLS - 1);
  CHECK_EQ(getNoteNumber(LEFT, NUMCOLS - 1, 0) + 1, getNoteNumber(LEFT, 1, 1));
}

TEST(layout_custom_row_offset) {
  linn::reset();
  Global.rowOffset = ROWOFFSET_OCTAVECUSTOM;
  Global.customRowOffset = 7;
  CHECK_EQ(getNoteNumber(LEFT, 1, 1) - getNoteNumber(LEFT, 1, 0), 7);
}

// the octave setting is the custom row offset, at its default of 12
TEST(layout_octave_offset_starts_an_octave_lower) {
  linn::reset();
  Global.rowOffset = ROWOFFSET_OCTAVECUSTOM;
  Global.customRowOffset = 12;
  CHECK_EQ(getNoteNumber(LEFT, 1, 0), 18);
  CHECK_EQ(getNoteNumber(LEFT, 1, 1), 30);
}

TEST(layout_left_handed_mirrors_the_columns) {
  linn::reset();
  Global.rowOffset = 5;
  byte rightmost = getNoteNumber(LEFT, NUMCOLS - 1, 0);
  Device.otherHanded = true;
  Device.splitHandedness = reversedBoth;
  CHECK_EQ(getNoteNumber(LEFT, 1, 0), rightmost);
  CHECK_EQ(getNoteNumber(LEFT, NUMCOLS - 1, 0), 30);
}

TEST(layout_transpose_lights_shifts_the_whole_layout) {
  linn::reset();
  Global.rowOffset = 5;
  Split[LEFT].transposeLights = 3;
  CHECK_EQ(getNoteNumber(LEFT, 1, 0), 27);
}
