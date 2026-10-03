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
// Phantom (ghost) touch rejection: a fourth corner of a rectangle of touches is suspect when it
// presses more lightly than the three others.

TEST(phantom_lightest_fourth_corner_is_a_phantom) {
  linn::reset();
  linn::touch(2, 2, 500);
  linn::touch(6, 2, 500);
  linn::touch(2, 5, 500);
  linn::select(6, 5);
  linn::touch(6, 5, 100);
  CHECK(isPhantomTouchContextual());
  CHECK(sensorCell->hasPhantoms());
}

TEST(phantom_firmest_fourth_corner_is_real) {
  linn::reset();
  linn::touch(2, 2, 500);
  linn::touch(6, 2, 500);
  linn::touch(2, 5, 500);
  linn::select(6, 5);
  linn::touch(6, 5, 900);
  CHECK(!isPhantomTouchContextual());
  CHECK(!sensorCell->hasPhantoms());
}

// a touch in the same row and one in the same column are not enough without the fourth corner
TEST(phantom_needs_a_complete_rectangle) {
  linn::reset();
  linn::select(6, 5);
  linn::touch(6, 5, 100);
  CHECK(!isPhantomTouchContextual());
  linn::touch(6, 2, 500);
  linn::touch(2, 5, 500);
  // the missing corner once played a note but is not touched, so only the touch bookkeeping
  // keeps it from counting as a corner
  cell(2, 2).note = 60;
  cell(2, 2).channel = 1;
  linn::select(6, 5);
  CHECK(!isPhantomTouchContextual());
  linn::touch(2, 2, 500);
  linn::select(6, 5);
  CHECK(isPhantomTouchContextual());
}

TEST(phantom_corner_with_a_note_counts_as_firm) {
  linn::reset();
  linn::touch(2, 2, 50);
  cell(2, 2).note = 60;
  cell(2, 2).channel = 1;
  linn::touch(6, 2, 500);
  linn::touch(2, 5, 500);
  linn::select(6, 5);
  linn::touch(6, 5, 100);
  CHECK(isPhantomTouchContextual());
}

static int phantomNoteOns() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  int n = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0x90) ++n;
  }
  return n;
}

// the same geometry through real scans: three sounding corners make the fourth a phantom
// whatever its pressure, since a corner with a note counts as firm, while a press anywhere
// else still sounds
TEST(phantom_fourth_corner_of_three_sounding_notes_does_not_sound) {
  linn::reset();
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = 77;
  linn::press(2, 2, 2000, 2000, 400);
  linn::press(6, 2, 2000, 2000, 400);
  linn::press(2, 5, 2000, 2000, 400);
  CHECK_EQ(phantomNoteOns(), 3);
  linn::press(6, 5, 2000, 2000, 900);
  CHECK_EQ(phantomNoteOns(), 0);
  linn::press(9, 5, 2000, 2000, 900);
  CHECK_EQ(phantomNoteOns(), 1);
}
