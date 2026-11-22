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
// The low row: the bottom row doing something other than playing notes.

static std::vector<uint8_t> lowRowOutput() {
  linn::drainMidi();
  std::vector<uint8_t> out = host::serialOutput();
  host::clearSerialOutput();
  return out;
}

static int lastControl(const std::vector<uint8_t>& out, uint8_t number) {
  int value = -1;
  for (size_t i = 0; i + 2 < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0xB0 && out[i + 1] == number) value = out[i + 2];
  }
  return value;
}

static int noteOns(const std::vector<uint8_t>& out) {
  int n = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    if ((out[i] & 0xF0) == 0x90) ++n;
  }
  return n;
}

TEST(lowrow_sustain_holds_while_the_row_is_touched) {
  linn::reset();
  Split[LEFT].lowRowMode = lowRowSustain;
  linn::press(5, 0, 2000, 2000, 400);
  std::vector<uint8_t> out = lowRowOutput();
  CHECK_EQ(lastControl(out, 64), 127);
  CHECK_EQ(noteOns(out), 0);
  linn::release(5, 0);
  CHECK_EQ(lastControl(lowRowOutput(), 64), 0);
}

TEST(lowrow_normal_mode_plays_notes_on_the_bottom_row) {
  linn::reset();
  Global.velocitySensitivity = velocityFixed;
  Global.valueForFixedVelocity = 77;
  Split[LEFT].lowRowMode = lowRowNormal;
  linn::press(5, 0, 2000, 2000, 400);
  CHECK_EQ(noteOns(lowRowOutput()), 1);
}

TEST(lowrow_cc_x_sends_the_configured_controller) {
  linn::reset();
  Split[LEFT].lowRowMode = lowRowCCX;
  Split[LEFT].ccForLowRow = 7;
  linn::press(5, 0, 700, 2000, 400);
  std::vector<uint8_t> out = lowRowOutput();
  int before = lastControl(out, 7);
  CHECK_EQ(noteOns(out), 0);
  for (int i = 1; i <= 20; ++i) {
    host::advanceMicros(10000);
    linn::hold(5, 0, (uint16_t)(700 + i * 20), 2000, 400, 1);
  }
  int after = lastControl(lowRowOutput(), 7);
  CHECK(before >= 0);
  CHECK(after > before);
}

static int litBesideTransposeCenter() {
  byte center = 1 + (NUMCOLS - 2) / 2;
  int lit = 0;
  for (byte col = 1; col < NUMCOLS; ++col) {
    byte color = getLedColor(col, 0, LED_LAYER_COMBINED);
    if (col == center) CHECK_EQ(color, Split[LEFT].colorAccent);
    else if (color != COLOR_OFF && color != COLOR_BLACK) ++lit;
  }
  return lit;
}

TEST(lowrow_transpose_hides_the_note_lights_under_it) {
  for (byte model : {200, 128}) {
    linn::reset(model);
    setDisplayMode(displayPerSplit);
    updateDisplay();
    Split[LEFT].lowRowMode = lowRowBend;
    Split[LEFT].lowRowBendBehavior = lowRowBendTranspose;
    setDisplayMode(displayNormal);
    updateDisplay();
    CHECK_EQ(litBesideTransposeCenter(), 0);

    linn::press(0, SPLIT_ROW, 2000, 2000, 600);
    linn::release(0, SPLIT_ROW);
    CHECK(Global.splitActive);
    host::advanceMicros(2000000);
    linn::press(0, SPLIT_ROW, 2000, 2000, 600);
    linn::release(0, SPLIT_ROW);
    CHECK(!Global.splitActive);
    CHECK_EQ(Global.currentPerSplit, LEFT);
    CHECK_EQ(litBesideTransposeCenter(), 0);

    setDisplayMode(displayGlobal);
    updateDisplay();
    activateSleepMode();
    linn::press(5, 3, 2000, 2000, 600);
    linn::release(5, 3);
    CHECK_EQ(displayMode, displayNormal);
    CHECK_EQ(litBesideTransposeCenter(), 0);
  }
}
