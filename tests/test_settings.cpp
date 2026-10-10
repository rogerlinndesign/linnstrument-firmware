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
  CHECK_EQ(Device.version, 17);
  CHECK(!Device.serialMode);
  CHECK(!Device.calibrated);
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
