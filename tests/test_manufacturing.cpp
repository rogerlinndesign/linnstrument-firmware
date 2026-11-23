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
// The manufacturing test screen, entered by holding Per-Split Settings at power on.

static void manufacturingScan(byte row, uint16_t z) {
  host::setSensor(0, row, 0, 0, z);
  linn::select(0, row);
  sensorCell->shouldRefreshData();
  modeLoopManufacturingTest();
  host::advanceMicros(1000);
}

TEST(manufacturing_control_buttons_light_all_leds_in_row_colors) {
  const byte expected[8] = {COLOR_ORANGE, COLOR_WHITE, COLOR_MAGENTA, COLOR_BLUE,
                            COLOR_CYAN, COLOR_GREEN, COLOR_YELLOW, COLOR_RED};
  for (int model = 0; model < 2; ++model) {
    linn::reset(model == 0 ? 200 : 128);
    operatingMode = modeManufacturingTest;
    for (byte row = 0; row < NUMROWS; ++row) {
      for (int i = 1; i <= VELOCITY_TOTAL_SAMPLES + 4; ++i) {
        manufacturingScan(row, (uint16_t)(1000 * min(i, (int)VELOCITY_TOTAL_SAMPLES) / VELOCITY_TOTAL_SAMPLES));
      }
      CHECK_EQ(getLedColor(1, 0, LED_LAYER_MAIN), expected[row]);
      CHECK_EQ(getLedColor(NUMCOLS - 1, NUMROWS - 1, LED_LAYER_MAIN), expected[row]);

      for (int i = 0; i < 4; ++i) {
        manufacturingScan(row, 0);
        host::advanceMicros(30000);
      }
      CHECK_EQ(getLedColor(1, 0, LED_LAYER_MAIN), COLOR_OFF);
    }
  }
}
