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

static int phantomNoteOns(int* noteOffs = nullptr) {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  int n = 0;
  if (noteOffs) *noteOffs = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0x90) ++n;
    if (noteOffs && (out[i] & 0xF0) == 0x80) ++*noteOffs;
  }
  return n;
}

// Unusable position readings fall back to the original phantom rule.
TEST(phantom_inconclusive_fourth_corner_falls_back_to_three_sounding_notes) {
  linn::reset();
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = 77;
  linn::press(2, 2, 2000, 2000, 900);
  linn::press(6, 2, 2000, 2000, 900);
  linn::press(2, 5, 2000, 2000, 900);
  CHECK_EQ(phantomNoteOns(), 3);
  linn::press(6, 5, 0, 0, 352);
  CHECK_EQ(phantomNoteOns(), 0);
  linn::press(9, 5, 2000, 2000, 900);
  CHECK_EQ(phantomNoteOns(), 1);
}


// Independent nodal solution for 14k row strips, 15.2k column strips, an
// 8.2k pull-up and 82 ohm switch leads, with contacts at pad centers.
// Conductances in pull-up units are .18, .55, 1.1 and either 0 or .035.
// The open corner still carries pressure well above the release threshold.
TEST(phantom_circuit_detects_fourth_touch_and_release_on_both_models) {
  const uint16_t fixture[2][2][4][3] = {
    {{{1571,1288,556}, {2047,1288,1121}, {1571,2301,1474}, {1571,1288,381}},
     {{1644,1442,631}, {2022,1343,1156}, {1586,2269,1494}, {1686,1532,479}}},
    {{{1036,1288,562}, {1347,1288,1136}, {1036,2301,1519}, {1036,1288,386}},
     {{1083,1441,638}, {1330,1343,1173}, {1045,2270,1538}, {1110,1530,485}}}
  };
  const byte cols[] = {6,8,6,8}, rows[] = {2,2,4,4};
  for (int model = 0; model < 2; ++model) {
    linn::reset(model == 0 ? 128 : 200);
    Global.velocitySensitivity = velocityFixed;
    Global.valueForFixedVelocity = 77;
    for (int i = 0; i < 3; ++i) {
      const uint16_t* pad = fixture[model][0][i];
      linn::press(cols[i], rows[i], pad[0], pad[1], pad[2]);
    }
    CHECK_EQ(phantomNoteOns(), 3);
    int previous = 0;
    for (int state : {0, 1, 1, 0}) {
      for (int i = 0; i < 4; ++i) {
        const uint16_t* pad = fixture[model][state][i];
        host::setSensor(cols[i], rows[i], pad[0], pad[1], pad[2]);
      }
      host::advanceMicros(100000);
      for (int scan = 0; scan < 2 * VELOCITY_TOTAL_SAMPLES; ++scan) linn::scan(8, 4);
      CHECK_EQ(cell(8, 4).hasNote(), state == 1);
      int noteOffs = 0;
      CHECK_EQ(phantomNoteOns(&noteOffs), state > previous ? 1 : 0);
      CHECK_EQ(noteOffs, previous > state ? 1 : 0);
      for (int i = 0; i < 3; ++i) CHECK(cell(cols[i], rows[i]).hasNote());
      previous = state;
    }
  }
}

// Same circuit as above, conductances .08, .09, .10 and 0 or .30.
// A strong fourth is distinguishable by Z even if no idle row is available
// for a reference voltage. An ambiguous three-finger shape takes the fallback.
TEST(phantom_circuit_uses_pressure_when_position_is_unavailable) {
  linn::reset();
  const byte cols[] = {6,8,6,8}, rows[] = {2,2,4,4};
  const uint16_t z[2][4] = {{288,318,347,114}, {409,431,453,848}};
  for (byte row = 0; row < NUMROWS; ++row)
    if (row != 2 && row != 4) linn::touch(12, row, 500);
  for (int state : {0, 1}) {
    for (int i = 0; i < 4; ++i) host::setSensor(cols[i], rows[i], 0, 0, z[state][i]);
    CHECK_EQ(measureRectangleContact(8, 4, 6, 2),
             state ? rectangleCircuit::present : rectangleCircuit::inconclusive);
  }
}
