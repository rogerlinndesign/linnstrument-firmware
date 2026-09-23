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
// Which MIDI note a cell plays, from the row offset and transposition settings.

TEST(notes_default_layout_rises_by_row_offset) {
  linn::reset();
  Global.rowOffset = 5;
  CHECK_EQ(getNoteNumber(LEFT, 1, 0), 30);
  CHECK_EQ(getNoteNumber(LEFT, 2, 0), 31);
  CHECK_EQ(getNoteNumber(LEFT, 1, 1), 35);
  CHECK_EQ(getNoteNumber(LEFT, 1, 7), 65);
}

TEST(notes_row_offset_changes_the_layout) {
  linn::reset();
  Global.rowOffset = 7;
  CHECK_EQ(getNoteNumber(LEFT, 1, 1) - getNoteNumber(LEFT, 1, 0), 7);
}

TEST(notes_transposition_shifts_the_played_note) {
  linn::reset();
  Global.rowOffset = 5;
  Split[LEFT].transposePitch = 2;
  Split[LEFT].transposeOctave = 12;
  CHECK_EQ(transposedNote(LEFT, 1, 0), 44);
  Split[LEFT].transposeLights = 1;
  CHECK_EQ(getNoteNumber(LEFT, 1, 0), 29);
}

TEST(notes_same_formula_on_the_128) {
  linn::reset(128);
  Global.rowOffset = 5;
  CHECK_EQ(getNoteNumber(LEFT, 16, 0), 45);
}
