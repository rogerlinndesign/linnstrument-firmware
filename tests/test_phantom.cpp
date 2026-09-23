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

TEST(phantom_needs_a_complete_rectangle) {
  linn::reset();
  linn::touch(2, 2, 500);
  linn::touch(6, 2, 500);
  linn::select(6, 5);
  linn::touch(6, 5, 100);
  CHECK(!isPhantomTouchContextual());
}

TEST(phantom_single_touch_is_real) {
  linn::reset();
  linn::select(6, 5);
  linn::touch(6, 5, 100);
  CHECK(!isPhantomTouchContextual());
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
