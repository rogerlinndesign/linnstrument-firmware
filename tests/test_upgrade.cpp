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
// Settings upgrades from every layout version the firmware has ever stored, through
// upgradeConfigurationSettings() in ls_extstorage.ino. Each test builds a blob in the old layout
// from its snapshot structs, plants distinctive values in the fields every layout since version 2
// stores, runs the upgrade and checks that they arrive, that the fields the old layout did not
// have got their defaults, and from version 9 on that the sequencer project came along.

template <typename P>
static void plantPreset(P& p, int seed) {
  p.global.splitPoint = (byte)(seed % 10 + 5);
  p.global.currentPerSplit = (byte)(seed % 2);
  p.global.rowOffset = (byte)(seed % 12 + 1);
  p.global.velocitySensitivity = velocityLow;
  p.global.pressureSensitivity = pressureHigh;
  p.global.switchAssignment[SWITCH_FOOT_L] = ASSIGNED_OCTAVE_UP;
  p.global.switchBothSplits[SWITCH_FOOT_R] = true;
  p.global.midiIO = (byte)(seed % 2);
  p.global.arpDirection = ArpDown;
  p.global.arpTempo = ArpEighth;
  p.global.arpOctave = 1;
  p.split[LEFT].midiMode = channelPerRow;
  p.split[RIGHT].midiMode = channelPerNote;
  p.split[LEFT].midiChanMain = (byte)(seed % 15 + 1);
  p.split[RIGHT].midiChanMain = (byte)(16 - seed % 15);
  p.split[LEFT].midiChanPerRow = 3;
  p.split[RIGHT].midiChanSet[seed % 16] = true;
  p.split[LEFT].sendY = true;
  p.split[RIGHT].sendZ = true;
  p.split[LEFT].pitchCorrectQuantize = true;
  p.split[RIGHT].pitchResetOnRelease = true;
  p.split[LEFT].relativeY = true;
  p.split[RIGHT].expressionForZ = loudnessChannelPressure;
  p.split[LEFT].colorMain = COLOR_GREEN;
  p.split[RIGHT].colorMain = COLOR_PINK;
  p.split[LEFT].colorAccent = COLOR_BLUE;
  p.split[LEFT].colorLowRow = COLOR_YELLOW;
  p.split[RIGHT].lowRowMode = lowRowSustain;
  p.split[LEFT].transposeOctave = 12;
  p.split[RIGHT].transposeOctave = -12;
  p.split[LEFT].transposePitch = 3;
  p.split[RIGHT].transposePitch = -4;
  p.split[LEFT].transposeLights = -2;
  p.split[RIGHT].transposeLights = 5;
  p.split[RIGHT].ccFaders = true;
  p.split[LEFT].arpeggiator = true;
  p.split[RIGHT].strum = true;
}

static void checkPreset(const PresetSettings& p, int seed) {
  CHECK_EQ(p.global.splitPoint, seed % 10 + 5);
  CHECK_EQ(p.global.currentPerSplit, seed % 2);
  CHECK_EQ(p.global.rowOffset, seed % 12 + 1);
  CHECK_EQ(p.global.velocitySensitivity, velocityLow);
  CHECK_EQ(p.global.pressureSensitivity, pressureHigh);
  CHECK_EQ(p.global.switchAssignment[SWITCH_FOOT_L], ASSIGNED_OCTAVE_UP);
  CHECK(p.global.switchBothSplits[SWITCH_FOOT_R]);
  CHECK_EQ(p.global.midiIO, seed % 2);
  CHECK_EQ(p.global.arpDirection, ArpDown);
  CHECK_EQ(p.global.arpTempo, ArpEighth);
  CHECK_EQ(p.global.arpOctave, 1);
  CHECK_EQ(p.split[LEFT].midiMode, channelPerRow);
  CHECK_EQ(p.split[RIGHT].midiMode, channelPerNote);
  CHECK_EQ(p.split[LEFT].midiChanMain, seed % 15 + 1);
  CHECK_EQ(p.split[RIGHT].midiChanMain, 16 - seed % 15);
  CHECK_EQ(p.split[LEFT].midiChanPerRow, 3);
  CHECK(p.split[RIGHT].midiChanSet[seed % 16]);
  CHECK(!p.split[LEFT].sendX);
  CHECK(p.split[LEFT].sendY);
  CHECK(!p.split[LEFT].sendZ);
  CHECK(p.split[RIGHT].sendZ);
  CHECK(p.split[LEFT].pitchCorrectQuantize);
  CHECK(!p.split[RIGHT].pitchCorrectQuantize);
  CHECK(p.split[RIGHT].pitchResetOnRelease);
  CHECK(p.split[LEFT].relativeY);
  CHECK_EQ(p.split[RIGHT].expressionForZ, loudnessChannelPressure);
  CHECK_EQ(p.split[LEFT].expressionForZ, loudnessPolyPressure);
  CHECK_EQ(p.split[LEFT].colorMain, COLOR_GREEN);
  CHECK_EQ(p.split[RIGHT].colorMain, COLOR_PINK);
  CHECK_EQ(p.split[LEFT].colorAccent, COLOR_BLUE);
  CHECK_EQ(p.split[LEFT].colorLowRow, COLOR_YELLOW);
  CHECK_EQ(p.split[RIGHT].lowRowMode, lowRowSustain);
  CHECK_EQ(p.split[LEFT].lowRowMode, lowRowNormal);
  CHECK_EQ(p.split[LEFT].transposeOctave, 12);
  CHECK_EQ(p.split[RIGHT].transposeOctave, -12);
  CHECK_EQ(p.split[LEFT].transposePitch, 3);
  CHECK_EQ(p.split[RIGHT].transposePitch, -4);
  CHECK_EQ(p.split[LEFT].transposeLights, -2);
  CHECK_EQ(p.split[RIGHT].transposeLights, 5);
  CHECK(p.split[RIGHT].ccFaders);
  CHECK(!p.split[LEFT].ccFaders);
  CHECK(p.split[LEFT].arpeggiator);
  CHECK(p.split[RIGHT].strum);
  CHECK(!p.split[LEFT].strum);
}

// fields no layout before 17 stored, so every converter has to give them their default
static void checkNewFieldDefaults(const PresetSettings& p) {
  CHECK_EQ(p.split[LEFT].lowRowBendBehavior, lowRowBendBend);
  CHECK_EQ(p.split[RIGHT].lowRowBendBehavior, lowRowBendBend);
}

template <typename D>
static void plantDevice(D& d, byte version) {
  d.version = version;
  d.calibrated = true;
  d.sensorRangeZ = 700;
}

static void checkDevice() {
  CHECK_EQ(Device.version, 17);
  CHECK(Device.calibrated);
  CHECK_EQ(Device.sensorRangeZ, 700);
}

static void checkUpgraded(int presets) {
  checkDevice();
  checkPreset(config.settings, 50);
  checkNewFieldDefaults(config.settings);
  for (int p = 0; p < presets; ++p) {
    checkPreset(config.preset[p], 10 + p);
    checkNewFieldDefaults(config.preset[p]);
  }
}

// Versions 2 and 3 stored four presets and no separate current settings; the upgrade takes
// preset 0 as the current settings.
template <typename C>
static void upgradeWithPresetsOnly(byte version) {
  linn::reset();
  C old;
  memset(&old, 0, sizeof(old));
  plantDevice(old.device, version);
  for (int p = 0; p < 4; ++p) plantPreset(old.preset[p], 10 + p);
  CHECK(upgradeConfigurationSettings(sizeof(C), (byte*)&old));
  checkDevice();
  checkPreset(config.settings, 10);
  checkNewFieldDefaults(config.settings);
  for (int p = 0; p < 4; ++p) {
    checkPreset(config.preset[p], 10 + p);
    checkNewFieldDefaults(config.preset[p]);
  }
}

template <typename C>
static void plantSettings(C& old, byte version, int presets) {
  memset(&old, 0, sizeof(old));
  plantDevice(old.device, version);
  plantPreset(old.settings, 50);
  for (int p = 0; p < presets; ++p) plantPreset(old.preset[p], 10 + p);
}

// From version 4 on there are current settings next to the presets.
template <typename C>
static void upgradeWithSettings(byte version, int presets) {
  linn::reset();
  C old;
  plantSettings(old, version, presets);
  CHECK(upgradeConfigurationSettings(sizeof(C), (byte*)&old));
  checkUpgraded(presets);
}

// From version 9 on the sequencer project is stored too and carried over as is.
template <typename C>
static void upgradeWithSettingsAndProject(byte version, int presets) {
  linn::reset();
  C old;
  plantSettings(old, version, presets);
  old.project.tempo = 133;
  CHECK(upgradeConfigurationSettings(sizeof(C), (byte*)&old));
  checkUpgraded(presets);
  CHECK_EQ(config.project.tempo, 133);
}

TEST(upgrade_from_v1_fills_the_presets_from_the_single_setup) {
  linn::reset();
  ConfigurationV1 old;
  memset(&old, 0, sizeof(old));
  old.global.version = 1;
  old.global.calibrated = true;
  old.global.sensorRangeZ = 700;
  old.global.rowOffset = 7;
  old.global.splitPoint = 9;
  old.left.midiChanMain = 5;
  old.right.midiChanMain = 11;
  old.left.transposeOctave = 12;
  old.right.transposeOctave = -12;
  old.left.transposePitch = 3;
  old.right.transposeLights = 5;
  old.left.colorMain = COLOR_GREEN;
  old.right.colorMain = COLOR_PINK;
  CHECK(upgradeConfigurationSettings(sizeof(ConfigurationV1), (byte*)&old));
  checkDevice();
  for (int p = 0; p < 4; ++p) {
    CHECK_EQ(config.preset[p].global.rowOffset, 7);
    CHECK_EQ(config.preset[p].global.splitPoint, 9);
    CHECK_EQ(config.preset[p].split[LEFT].midiChanMain, 5);
    CHECK_EQ(config.preset[p].split[RIGHT].midiChanMain, 11);
    CHECK_EQ(config.preset[p].split[LEFT].transposeOctave, 12);
    CHECK_EQ(config.preset[p].split[RIGHT].transposeOctave, -12);
    CHECK_EQ(config.preset[p].split[LEFT].transposePitch, 3);
    CHECK_EQ(config.preset[p].split[RIGHT].transposePitch, 0);
    CHECK_EQ(config.preset[p].split[RIGHT].transposeLights, 5);
    CHECK_EQ(config.preset[p].split[LEFT].colorMain, COLOR_GREEN);
    CHECK_EQ(config.preset[p].split[RIGHT].colorMain, COLOR_PINK);
    checkNewFieldDefaults(config.preset[p]);
  }
  CHECK_EQ(Global.rowOffset, 7);
  CHECK_EQ(Split[RIGHT].midiChanMain, 11);
}

TEST(upgrade_from_v2) { upgradeWithPresetsOnly<ConfigurationV2>(2); }
TEST(upgrade_from_v3) { upgradeWithPresetsOnly<ConfigurationV3>(3); }
TEST(upgrade_from_v4) { upgradeWithSettings<ConfigurationV4>(4, 4); }
TEST(upgrade_from_v5) { upgradeWithSettings<ConfigurationV5>(5, 4); }
TEST(upgrade_from_v6) { upgradeWithSettings<ConfigurationV6>(6, 4); }
TEST(upgrade_from_v7) { upgradeWithSettings<ConfigurationV7>(7, 4); }
TEST(upgrade_from_v8) { upgradeWithSettings<ConfigurationV8>(8, 4); }
TEST(upgrade_from_v9) { upgradeWithSettingsAndProject<ConfigurationV9>(9, 4); }
TEST(upgrade_from_v10) { upgradeWithSettingsAndProject<ConfigurationV10>(10, 4); }
TEST(upgrade_from_v11) { upgradeWithSettingsAndProject<ConfigurationV11>(11, 4); }
TEST(upgrade_from_v12) { upgradeWithSettingsAndProject<ConfigurationV12>(12, 4); }
TEST(upgrade_from_v13) { upgradeWithSettingsAndProject<ConfigurationV13>(13, 4); }
TEST(upgrade_from_v14) { upgradeWithSettingsAndProject<ConfigurationV14>(14, 4); }
TEST(upgrade_from_v15) { upgradeWithSettingsAndProject<ConfigurationV15>(15, 6); }
TEST(upgrade_from_v16) { upgradeWithSettingsAndProject<ConfigurationV16>(16, 6); }

TEST(upgrade_copies_the_current_version_as_is) {
  linn::reset();
  Configuration same = config;
  same.settings.global.rowOffset = 11;
  CHECK(upgradeConfigurationSettings(sizeof(Configuration), (byte*)&same));
  CHECK_EQ(Global.rowOffset, 11);
}

TEST(upgrade_refuses_a_newer_version) {
  linn::reset();
  Configuration newer = config;
  newer.device.version = 18;
  CHECK(!upgradeConfigurationSettings(sizeof(Configuration), (byte*)&newer));
}

// A blob whose size does not match its version's layout is refused and leaves the settings alone.
TEST(upgrade_refuses_a_size_that_does_not_match_the_version) {
  linn::reset();
  Global.rowOffset = 3;
  ConfigurationV9 old;
  plantSettings(old, 9, 4);
  CHECK(!upgradeConfigurationSettings(sizeof(ConfigurationV9) + 1, (byte*)&old));
  CHECK(!upgradeConfigurationSettings(sizeof(ConfigurationV8), (byte*)&old));
  CHECK_EQ(Global.rowOffset, 3);
  CHECK_EQ(Device.version, 17);
}
