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
// Settings defaults and flash storage; the migrations are in test_upgrade.cpp.

TEST(settings_defaults) {
  linn::reset();
  CHECK_EQ(Device.version, 18);
  CHECK(!Device.serialMode);
  CHECK(!Device.calibrated);
}

// a hard reset gives a 128 a higher prescale, while the buttons held at power on are read with
// the one both models start with
TEST(settings_sensor_prescale_default_per_model) {
  for (int model : { 200, 128 }) {
    linn::reset(model);
    initializeSensors();
    CHECK_EQ(Device.sensorSensitivityZ, 75);
    dueFlashStorage.write(0, 254);
    initializeStorage();
    CHECK_EQ(Device.sensorSensitivityZ, model == 128 ? 90 : 75);
    // kept as stored on the next boot
    Device.sensorSensitivityZ = 60;
    storeSettings();
    initializeSensors();
    initializeStorage();
    CHECK_EQ(Device.sensorSensitivityZ, 60);
  }
}

TEST(settings_survive_flash_round_trip) {
  linn::reset();
  Global.rowOffset = 7;
  storeSettings();
  Global.rowOffset = 3;
  loadSettings();
  CHECK_EQ(Global.rowOffset, 7);
}

TEST(settings_second_store_keeps_the_newest_copy) {
  linn::reset();
  Global.rowOffset = 7;
  storeSettings();
  Global.rowOffset = 9;
  storeSettings();
  Global.rowOffset = 0;
  loadSettings();
  CHECK_EQ(Global.rowOffset, 9);
}

static uint16_t szX(byte col) {
  return (uint16_t)FXD_TO_INT(calculateDefaultMeasuredX(col));
}

static void szTap(byte col, byte row) {
  linn::press(col, row, szX(col), 2000, 400);
  linn::release(col, row);
}

static void szHold(byte col, byte row) {
  linn::press(col, row, szX(col), 2000, 400);
  for (int i = 0; i < 30; ++i) {
    host::advanceMicros(50000);
    linn::hold(col, row, szX(col), 2000, 400, 1);
  }
  linn::release(col, row);
}

TEST(settings_holding_channel_pressure_chooses_which_touch_it_follows) {
  linn::reset();
  Global.currentPerSplit = LEFT;
  Split[LEFT].expressionForZ = loudnessPolyPressure;
  setDisplayMode(displayPerSplit);
  updateDisplay();
  CHECK_EQ(Split[LEFT].sourceForZ, loudnessSourceLast);

  szTap(10, 5);
  CHECK_EQ(Split[LEFT].expressionForZ, loudnessChannelPressure);
  CHECK_EQ(displayMode, displayPerSplit);
  CHECK_EQ(getLoudnessSourceColor(LEFT), Split[LEFT].colorMain);

  // a touch held there pulses until the hold opens the screen, as the other cells with a hold do
  host::advanceMicros(500000);
  linn::press(10, 5, szX(10), 2000, 400);
  CHECK_EQ(ledVisible(LED_LAYER_MAIN, 10, 5) & B00000111, cellSlowPulse);
  linn::release(10, 5);
  host::advanceMicros(500000);
  szHold(10, 5);
  CHECK_EQ(displayMode, displayLoudnessSource);

  host::advanceMicros(500000);
  szTap(5, 3);
  szTap(8, 3);
  CHECK_EQ(Split[LEFT].sourceForZ, loudnessSourceFirst);
  szTap(11, 3);
  CHECK_EQ(Split[LEFT].sourceForZ, loudnessSourceHighest);
  szTap(14, 3);
  CHECK_EQ(Split[LEFT].sourceForZ, loudnessSourceHighest);
  CHECK_EQ(Split[RIGHT].sourceForZ, loudnessSourceLast);
  CHECK_EQ(getLoudnessSourceColor(LEFT), Split[LEFT].colorAccent);
}
