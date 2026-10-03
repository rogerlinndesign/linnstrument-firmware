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
// The default calibration: Y maps a row's raw band onto 0 to 127, X follows the raw reading, and
// the phantom plausibility check only applies once the instrument is calibrated.

TEST(calibration_y_maps_the_row_band_to_0_127) {
  linn::reset();
  linn::select(5, 3);
  short minY = Device.calCols[1][3].minY;
  short maxY = Device.calCols[1][3].maxY;
  CHECK_EQ(calculateCalibratedY(minY), 0);
  CHECK_EQ(calculateCalibratedY(maxY), 127);
  CHECK_NEAR(calculateCalibratedY((minY + maxY) / 2), 64, 4);
  CHECK_EQ(calculateCalibratedY(minY - 100), 0);
  CHECK_EQ(calculateCalibratedY(maxY + 100), 127);
}

TEST(calibration_y_is_monotonic_across_the_band) {
  linn::reset();
  linn::select(9, 5);
  int previous = -1;
  int first = calculateCalibratedY(Device.calCols[2][5].minY);
  for (short raw = Device.calCols[2][5].minY; raw <= Device.calCols[2][5].maxY; raw += 10) {
    int value = calculateCalibratedY(raw);
    CHECK(value >= previous);
    previous = value;
  }
  CHECK(previous > first);
}

// Each column has a reference position; the default calibration maps the reading the
// uncalibrated sensor gives at that position onto it, with a slope close to one.
// The 128 has six calibration columns, not nine: its rightmost column maps through the sixth one
// alone, with no neighbour to blend with. A band planted there that the defaults do not have shows
// which column is read; on x86 the uninitialised seventh column would trap if it were reached.
TEST(calibration_y_rightmost_column_of_the_128_maps_through_the_sixth_column) {
  linn::reset(128);
  Device.calCols[5][3].minY = 1000;
  Device.calCols[5][3].maxY = 1254;
  Device.calCols[5][3].fxdRatio = FXD_DIV(FXD_FROM_INT(254), FXD_CALY_FULL_UNIT);
  linn::select(16, 3);
  CHECK_EQ(calculateCalibratedY(1000), 0);
  CHECK_EQ(calculateCalibratedY(1254), 127);
  CHECK_NEAR(calculateCalibratedY(1127), 64, 2);
  CHECK_EQ(calculateCalibratedY(1900), 127);
}

TEST(calibration_x_defaults_map_the_expected_reading_to_the_reference) {
  linn::reset();
  linn::select(5, 3);
  short reference = FXD_TO_INT(calculateReferenceX(5));
  short measured = FXD_TO_INT(calculateDefaultMeasuredX(5));
  CHECK_NEAR(calculateCalibratedX(measured), reference, 3);
  CHECK_NEAR(calculateCalibratedX(measured + 60), reference + 60, 10);
}

TEST(calibration_impossible_x_needs_a_calibrated_instrument) {
  linn::reset();
  short measured = FXD_TO_INT(calculateDefaultMeasuredX(5));
  host::setSensor(5, 3, (uint16_t)(measured + 300), 2000, 400);
  linn::select(5, 3);
  sensorCell->shouldRefreshData();
  CHECK(!hasImpossibleX());
  CHECK(!isPhantomTouchIndividual());
  Device.calibrated = true;
  sensorCell->shouldRefreshData();
  CHECK(hasImpossibleX());
  host::setSensor(5, 3, (uint16_t)(measured + 20), 2000, 400);
  sensorCell->shouldRefreshData();
  CHECK(!hasImpossibleX());
}
