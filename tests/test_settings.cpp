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
// Settings defaults, flash storage and migration from an older layout.

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

static ConfigurationV16 previousLayoutFromCurrent() {
  ConfigurationV16 old;
  memset(&old, 0, sizeof(old));
  memcpy(&old.device, &config.device, sizeof(DeviceSettings));
  old.device.version = 16;
  old.settings.global = Global;
  return old;
}

TEST(settings_upgrade_from_version_16) {
  linn::reset();
  ConfigurationV16 old = previousLayoutFromCurrent();
  old.settings.global.rowOffset = 7;
  CHECK(upgradeConfigurationSettings(sizeof(ConfigurationV16), (byte*)&old));
  CHECK_EQ(Global.rowOffset, 7);
  CHECK_EQ(Device.version, 17);
}

TEST(settings_upgrade_rejects_wrong_size) {
  linn::reset();
  ConfigurationV16 old = previousLayoutFromCurrent();
  old.settings.global.rowOffset = 7;
  CHECK(!upgradeConfigurationSettings(sizeof(ConfigurationV16) - 1, (byte*)&old));
  CHECK_EQ(Global.rowOffset, 5);
}

TEST(settings_upgrade_rejects_newer_version) {
  linn::reset();
  Configuration newer = config;
  newer.device.version = 99;
  CHECK(!upgradeConfigurationSettings(sizeof(Configuration), (byte*)&newer));
}

TEST(settings_current_version_is_copied_as_is) {
  linn::reset();
  Configuration same = config;
  same.settings.global.rowOffset = 11;
  CHECK(upgradeConfigurationSettings(sizeof(Configuration), (byte*)&same));
  CHECK_EQ(Global.rowOffset, 11);
}
