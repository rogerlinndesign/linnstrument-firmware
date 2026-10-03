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
// Presets: six stored copies of the global and split settings.

TEST(preset_store_and_load_round_trip) {
  linn::reset();
  Global.rowOffset = 7;
  Split[LEFT].transposePitch = 2;
  storeSettingsToPreset(2);
  Global.rowOffset = 5;
  Split[LEFT].transposePitch = 0;
  loadSettingsFromPreset(2);
  CHECK_EQ(Global.rowOffset, 7);
  CHECK_EQ(Split[LEFT].transposePitch, 2);
  CHECK_EQ(Device.lastLoadedPreset, 2);
}

TEST(preset_loads_over_midi_with_nrpn_243) {
  linn::reset();
  Global.rowOffset = 7;
  storeSettingsToPreset(4);
  Global.rowOffset = 5;
  receivedNrpn(243, 4, 0);
  CHECK_EQ(Global.rowOffset, 7);
  receivedNrpn(243, 6, 0);
  CHECK_EQ(Device.lastLoadedPreset, 4);
}
