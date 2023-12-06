/***************************** ls_extstorage: LinnStrument Settings *******************************
Copyright 2023 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
***************************************************************************************************
Functions to exchange settings with an external device over a serial handshake protocol.
This is essentially used by the upgrade tool to temporarily store the previous settings while doing
an upgrade and sending them back after the upgrade is finished.
The new firmware is then responsible of applying the received settings and possibly performing
some transformation logic is the settings structure changed for the new firmware.
**************************************************************************************************/

/**************************************** Configuration V1 ****************************************
This is used by firmware v1.0.9 and earlier
**************************************************************************************************/
struct CalibrationYV1 {
  int minY;
  int maxY;
  int32_t fxdRatio;
};
struct GlobalSettingsV1 {
  byte version;                              // to prepare for versioning
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  boolean mainNotes[12];                     // determines which notes receive "main" lights
  boolean accentNotes[12];                   // determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationYV1 calCols[9][MAXROWS];        // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean promoAnimationAtStartup;           // store whether the promo animation should run at startup
};
struct SplitSettingsV1 {
  byte midiMode;                       // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                   // main midi channel, 1 to 16
  byte midiChanPerRow;                 // per-row midi channel, 1 to 16
  boolean midiChanSet[16];             // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  byte bendRange;                      // 1 - 96
  boolean sendX;                       // true to send continuous X, false if not
  boolean sendY;                       // true to send continuous Y, false if not
  boolean sendZ;                       // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;        // true to quantize pitch of initial touch, false if not
  boolean pitchCorrectHold;            // true to quantize pitch when note is held, false if not
  boolean pitchResetOnRelease;         // true to enable pitch bend being set back to 0 when releasing a touch
  unsigned short ccForY;               // 0-127
  boolean relativeY;                   // true when Y should be sent relative to the initial touch, false when it's absolute
  LoudnessExpression expressionForZ;   // the expression that should be used for loudness
  unsigned short ccForZ;               // 0-127
  byte colorMain;                      // color for non-accented cells
  byte colorAccent;                    // color for accented cells
  byte colorPlayed;                    // color for played notes
  byte colorLowRow;                    // color for low row if on
  byte lowRowMode;                     // see LowRowMode values
  unsigned short preset;               // preset number 0-127
  signed char transposeOctave;         // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;          // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;         // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                    // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                 // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                       // true when this split strums the touches of the other split
};
struct ConfigurationV1 {
  GlobalSettingsV1 global;
  SplitSettingsV1 left;
  SplitSettingsV1 right;
};
/**************************************** Configuration V2 ****************************************
This is used by firmware v1.1.0
**************************************************************************************************/
struct GlobalSettingsV2 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  boolean mainNotes[12];                     // determines which notes receive "main" lights
  boolean accentNotes[12];                   // determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
};
struct SplitSettingsV2 {
  byte midiMode;                       // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                   // main midi channel, 1 to 16
  byte midiChanPerRow;                 // per-row midi channel, 1 to 16
  boolean midiChanSet[16];             // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  byte bendRange;                      // 1 - 96
  boolean sendX;                       // true to send continuous X, false if not
  boolean sendY;                       // true to send continuous Y, false if not
  boolean sendZ;                       // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;        // true to quantize pitch of initial touch, false if not
  byte pitchCorrectHold;               // See PitchCorrectHoldSpeed values
  boolean pitchResetOnRelease;         // true to enable pitch bend being set back to 0 when releasing a touch
  TimbreExpression expressionForY;     // the expression that should be used for timbre
  unsigned short ccForY;               // 0-129 (with 128 and 129 being placeholders for PolyPressure and ChannelPressure)
  boolean relativeY;                   // true when Y should be sent relative to the initial touch, false when it's absolute
  LoudnessExpression expressionForZ;   // the expression that should be used for loudness
  unsigned short ccForZ;               // 0-127
  byte colorMain;                      // color for non-accented cells
  byte colorAccent;                    // color for accented cells
  byte colorPlayed;                    // color for played notes
  byte colorLowRow;                    // color for low row if on
  byte lowRowMode;                     // see LowRowMode values
  signed char transposeOctave;         // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;          // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;         // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                    // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                 // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                       // true when this split strums the touches of the other split
};
struct PresetSettingsV2 {
  GlobalSettingsV2 global;
  SplitSettingsV2 split[NUMSPLITS];
};
struct DeviceSettingsV2 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationYV1 calCols[9][MAXROWS];        // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean promoAnimationAtStartup;           // store whether the promo animation should run at startup
  byte currentPreset;                        // the currently active settings preset
};
struct ConfigurationV2 {
  DeviceSettingsV2 device;
  PresetSettingsV2 preset[4];
};
/**************************************** Configuration V3 ****************************************
This is used by firmware v1.1.1-beta1, v1.1.1-beta2 and v1.1.1-beta3
**************************************************************************************************/
struct GlobalSettingsV3 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  boolean mainNotes[12];                     // determines which notes receive "main" lights
  boolean accentNotes[12];                   // determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
};
struct DeviceSettingsV3 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationYV1 calCols[9][MAXROWS];        // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean promoAnimationAtStartup;           // store whether the promo animation should run at startup
  byte currentPreset;                        // the currently active settings preset
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
};
struct PresetSettingsV3 {
  GlobalSettingsV3 global;
  SplitSettingsV2 split[NUMSPLITS];
};
struct ConfigurationV3 {
  DeviceSettingsV3 device;
  PresetSettingsV3 preset[4];
};
/**************************************** Configuration V4 ****************************************
This is used by firmware v1.1.1-beta4, v1.1.2-beta1, v1.1.2 and v1.2.0-alpha1
**************************************************************************************************/
struct DeviceSettingsV4 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationYV1 calCols[9][MAXROWS];        // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean promoAnimationAtStartup;           // store whether the promo animation should run at startup
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean leftHanded;                        // whether to orient the X axis from right to left instead of from left to right
};
struct ConfigurationV4 {
  DeviceSettingsV4 device;
  PresetSettingsV3 settings;
  PresetSettingsV3 preset[4];
};
/**************************************** Configuration V5 ****************************************
This is used by firmware v1.2.0-beta1, v1.2.0-beta2 and v1.2.0
**************************************************************************************************/
struct GlobalSettingsV4 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  boolean mainNotes[12];                     // determines which notes receive "main" lights
  boolean accentNotes[12];                   // determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  unsigned short ccForSwitch;                // 0-127
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
};
struct SplitSettingsV3 {
  byte midiMode;                       // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                   // main midi channel, 1 to 16
  byte midiChanPerRow;                 // per-row midi channel, 1 to 16
  boolean midiChanSet[16];             // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  BendRangeOption bendRangeOption;     // see BendRangeOption
  byte customBendRange;                // 1 - 96
  boolean sendX;                       // true to send continuous X, false if not
  boolean sendY;                       // true to send continuous Y, false if not
  boolean sendZ;                       // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;        // true to quantize pitch of initial touch, false if not
  byte pitchCorrectHold;               // See PitchCorrectHoldSpeed values
  boolean pitchResetOnRelease;         // true to enable pitch bend being set back to 0 when releasing a touch
  TimbreExpression expressionForY;     // the expression that should be used for timbre
  unsigned short customCCForY;         // 0-129 (with 128 and 129 being placeholders for PolyPressure and ChannelPressure)
  boolean relativeY;                   // true when Y should be sent relative to the initial touch, false when it's absolute
  LoudnessExpression expressionForZ;   // the expression that should be used for loudness
  unsigned short customCCForZ;         // 0-127
  unsigned short ccForFader[8];        // each fader can control a CC number ranging from 0-127
  byte colorMain;                      // color for non-accented cells
  byte colorAccent;                    // color for accented cells
  byte colorPlayed;                    // color for played notes
  byte colorLowRow;                    // color for low row if on
  byte lowRowMode;                     // see LowRowMode values
  byte lowRowCCXBehavior;              // see LowRowCCBehavior values
  unsigned short ccForLowRow;          // 0-127
  byte lowRowCCXYZBehavior;            // see LowRowCCBehavior values
  unsigned short ccForLowRowX;         // 0-127
  unsigned short ccForLowRowY;         // 0-127
  unsigned short ccForLowRowZ;         // 0-127
  signed char transposeOctave;         // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;          // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;         // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                    // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                 // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                       // true when this split strums the touches of the other split
  boolean mpe;                         // true when MPE is active for this split
};
struct PresetSettingsV4 {
  GlobalSettingsV4 global;
  SplitSettingsV3 split[NUMSPLITS];
};
struct ConfigurationV5 {
  DeviceSettingsV4 device;
  PresetSettingsV4 settings;
  PresetSettingsV4 preset[4];
};
/**************************************** Configuration V6 ****************************************
This is used by firmware v1.2.1 and v1.2.2
**************************************************************************************************/
struct GlobalSettingsV5 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  byte activeNotes;                          // controls which collection of note lights presets is active
  int mainNotes[12];                         // bitmask array that determines which notes receive "main" lights
  int accentNotes[12];                       // bitmask array that determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  unsigned short minForVelocity;             // 0-127
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  unsigned short ccForSwitch;                // 0-127
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
  SustainBehavior sustainBehavior;           // the way the sustain pedal influences the notes
};
struct SplitSettingsV4 {
  byte midiMode;                       // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                   // main midi channel, 1 to 16
  byte midiChanPerRow;                 // per-row midi channel, 1 to 16
  boolean midiChanSet[16];             // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  BendRangeOption bendRangeOption;     // see BendRangeOption
  byte customBendRange;                // 1 - 96
  boolean sendX;                       // true to send continuous X, false if not
  boolean sendY;                       // true to send continuous Y, false if not
  boolean sendZ;                       // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;        // true to quantize pitch of initial touch, false if not
  byte pitchCorrectHold;               // See PitchCorrectHoldSpeed values
  boolean pitchResetOnRelease;         // true to enable pitch bend being set back to 0 when releasing a touch
  TimbreExpression expressionForY;     // the expression that should be used for timbre
  unsigned short customCCForY;         // 0-129 (with 128 and 129 being placeholders for PolyPressure and ChannelPressure)
  unsigned short minForY;              // 0-127
  unsigned short maxForY;              // 0-127
  boolean relativeY;                   // true when Y should be sent relative to the initial touch, false when it's absolute
  LoudnessExpression expressionForZ;   // the expression that should be used for loudness
  unsigned short customCCForZ;         // 0-127
  unsigned short minForZ;              // 0-127
  unsigned short maxForZ;              // 0-127
  unsigned short ccForFader[8];        // each fader can control a CC number ranging from 0-127
  byte colorMain;                      // color for non-accented cells
  byte colorAccent;                    // color for accented cells
  byte colorNoteon;                    // color for played notes
  byte colorLowRow;                    // color for low row if on
  byte lowRowMode;                     // see LowRowMode values
  byte lowRowCCXBehavior;              // see LowRowCCBehavior values
  unsigned short ccForLowRow;          // 0-127
  byte lowRowCCXYZBehavior;            // see LowRowCCBehavior values
  unsigned short ccForLowRowX;         // 0-127
  unsigned short ccForLowRowY;         // 0-127
  unsigned short ccForLowRowZ;         // 0-127
  signed char transposeOctave;         // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;          // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;         // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                    // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                 // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                       // true when this split strums the touches of the other split
  boolean mpe;                         // true when MPE is active for this split
};
struct PresetSettingsV5 {
  GlobalSettingsV5 global;
  SplitSettingsV4 split[NUMSPLITS];
};
struct ConfigurationV6 {
  DeviceSettingsV4 device;
  PresetSettingsV5 settings;
  PresetSettingsV5 preset[4];
};
/**************************************** Configuration V7 ****************************************
This is used by firmware v1.2.3-beta1, v1.2.3-beta2, v1.2.3-beta3, v1.2.3 and v1.2.4-beta1
**************************************************************************************************/
struct DeviceSettingsV5 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationYV1 calCols[9][MAXROWS];        // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean promoAnimation;                    // store whether the promo animation should run after five minutes of not touching
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean leftHanded;                        // whether to orient the X axis from right to left instead of from left to right
};
struct GlobalSettingsV6 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  byte activeNotes;                          // controls which collection of note lights presets is active
  int mainNotes[12];                         // bitmask array that determines which notes receive "main" lights
  int accentNotes[12];                       // bitmask array that determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  unsigned short minForVelocity;             // 1-127
  unsigned short maxForVelocity;             // 1-127
  unsigned short valueForFixedVelocity;      // 1-127
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  unsigned short ccForSwitch;                // 0-127
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
  SustainBehavior sustainBehavior;           // the way the sustain pedal influences the notes
};
struct PresetSettingsV6 {
  GlobalSettingsV6 global;
  SplitSettingsV4 split[NUMSPLITS];
};
struct ConfigurationV7 {
  DeviceSettingsV5 device;
  PresetSettingsV6 settings;
  PresetSettingsV6 preset[4];
};
/**************************************** Configuration V8 ****************************************
This is used by firmware v1.2.4-beta2 and v1.2.5
**************************************************************************************************/
struct DeviceSettingsV6 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationYV1 calCols[9][MAXROWS];        // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean sleepAnimationActive;              // store whether the promo animation was active last
  boolean sleepActive;                       // store whether LinnStrument should go to sleep automatically
  byte sleepDelay;                           // the number of minutes it takes for sleep to kick in
  boolean sleepAnimation;                    // store whether the promo animation should run during sleep mode
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean leftHanded;                        // whether to orient the X axis from right to left instead of from left to right
};
struct GlobalSettingsV7 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  byte activeNotes;                          // controls which collection of note lights presets is active
  int mainNotes[12];                         // bitmask array that determines which notes receive "main" lights
  int accentNotes[12];                       // bitmask array that determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  byte customRowOffset;                      // the custom row offset that can be configured at the location of the octave setting
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  unsigned short minForVelocity;             // 1-127
  unsigned short maxForVelocity;             // 1-127
  unsigned short valueForFixedVelocity;      // 1-127
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  unsigned short ccForSwitch;                // 0-127
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
  SustainBehavior sustainBehavior;           // the way the sustain pedal influences the notes
};
struct PresetSettingsV7 {
  GlobalSettingsV7 global;
  SplitSettingsV4 split[NUMSPLITS];
};
struct ConfigurationV8 {
  DeviceSettingsV6 device;
  PresetSettingsV7 settings;
  PresetSettingsV7 preset[4];
};
/**************************************** Configuration V9 ****************************************
This is used by firmware v2.0.0-beta1 and v2.0.0-beta2
**************************************************************************************************/
struct DeviceSettingsV7 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][MAXROWS];          // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean sleepAnimationActive;              // store whether the promo animation was active last
  boolean sleepActive;                       // store whether LinnStrument should go to sleep automatically
  byte sleepDelay;                           // the number of minutes it takes for sleep to kick in
  boolean sleepAnimation;                    // store whether the promo animation should run during sleep mode
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean leftHanded;                        // whether to orient the X axis from right to left instead of from left to right
  boolean midiThrough;                       // false if incoming MIDI should be isolated, true if it should be passed through to the outgoing MIDI port
};
struct SplitSettingsV5 {
  byte midiMode;                          // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                      // main midi channel, 1 to 16
  byte midiChanPerRow;                    // per-row midi channel, 1 to 16
  boolean midiChanSet[16];                // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  BendRangeOption bendRangeOption;        // see BendRangeOption
  byte customBendRange;                   // 1 - 96
  boolean sendX;                          // true to send continuous X, false if not
  boolean sendY;                          // true to send continuous Y, false if not
  boolean sendZ;                          // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;           // true to quantize pitch of initial touch, false if not
  byte pitchCorrectHold;                  // See PitchCorrectHoldSpeed values
  boolean pitchResetOnRelease;            // true to enable pitch bend being set back to 0 when releasing a touch
  TimbreExpression expressionForY;        // the expression that should be used for timbre
  unsigned short customCCForY;            // 0-129 (with 128 and 129 being placeholders for PolyPressure and ChannelPressure)
  unsigned short minForY;                 // 0-127
  unsigned short maxForY;                 // 0-127
  boolean relativeY;                      // true when Y should be sent relative to the initial touch, false when it's absolute
  LoudnessExpression expressionForZ;      // the expression that should be used for loudness
  unsigned short customCCForZ;            // 0-127
  unsigned short minForZ;                 // 0-127
  unsigned short maxForZ;                 // 0-127
  boolean ccForZ14Bit;                    // true when 14-bit messages should be sent when Z CC is between 0-31, false when only 7-bit messages should be sent
  unsigned short ccForFader[8];           // each fader can control a CC number ranging from 0-127
  byte colorMain;                         // color for non-accented cells
  byte colorAccent;                       // color for accented cells
  byte colorPlayed;                       // color for played notes
  byte colorLowRow;                       // color for low row if on
  byte colorSequencerEmpty;               // color for sequencer low row step with no events
  byte colorSequencerEvent;               // color for sequencer low row step with events
  byte colorSequencerDisabled;            // color for sequencer low row step that's not being played
  byte lowRowMode;                        // see LowRowMode values
  byte lowRowCCXBehavior;                 // see LowRowCCBehavior values
  unsigned short ccForLowRow;             // 0-127
  byte lowRowCCXYZBehavior;               // see LowRowCCBehavior values
  unsigned short ccForLowRowX;            // 0-127
  unsigned short ccForLowRowY;            // 0-127
  unsigned short ccForLowRowZ;            // 0-127
  signed char transposeOctave;            // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;             // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;            // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                       // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                    // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                          // true when this split strums the touches of the other split
  boolean mpe;                            // true when MPE is active for this split
  boolean sequencer;                      // true when the sequencer of this split is displayed
  SequencerView sequencerView;            // see SequencerView
};
struct PresetSettingsV8 {
  GlobalSettingsV7 global;
  SplitSettingsV5 split[NUMSPLITS];
};
struct ConfigurationV9 {
  DeviceSettingsV7 device;
  PresetSettingsV8 settings;
  PresetSettingsV8 preset[4];
  SequencerProject project;
};
/**************************************** Configuration V10 ****************************************
This is used by firmware v2.0.0 and v2.0.1
**************************************************************************************************/
struct DeviceSettingsV8 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][MAXROWS];          // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean sleepAnimationActive;              // store whether the promo animation was active last
  boolean sleepActive;                       // store whether LinnStrument should go to sleep automatically
  byte sleepDelay;                           // the number of minutes it takes for sleep to kick in
  boolean sleepAnimation;                    // store whether the promo animation should run during sleep mode
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean leftHanded;                        // whether to orient the X axis from right to left instead of from left to right
  boolean midiThrough;                       // false if incoming MIDI should be isolated, true if it should be passed through to the outgoing MIDI port
  short lastLoadedPreset;                    // the last settings preset that was loaded
  short lastLoadedProject;                   // the last sequencer project that was loaded
};
struct ConfigurationV10 {
  DeviceSettingsV8 device;
  PresetSettingsV8 settings;
  PresetSettingsV8 preset[4];
  SequencerProject project;
};
/**************************************** Configuration V11 ****************************************
This is used by firmware v2.0.2
**************************************************************************************************/
struct DeviceSettingsV9 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][MAXROWS];          // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  byte sensorSensitivityZ;                   // the scaling factor of the raw value of Z in percentage
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean sleepAnimationActive;              // store whether an animation was active last
  boolean sleepActive;                       // store whether LinnStrument should go to sleep automatically
  byte sleepDelay;                           // the number of minutes it takes for sleep to kick in
  byte sleepAnimationType;                   // the animation type to use during sleep, see SleepAnimationType
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean leftHanded;                        // whether to orient the X axis from right to left instead of from left to right
  boolean midiThrough;                       // false if incoming MIDI should be isolated, true if it should be passed through to the outgoing MIDI port
  short lastLoadedPreset;                    // the last settings preset that was loaded
  short lastLoadedProject;                   // the last sequencer project that was loaded
  boolean splitActive;                       // false = split off, true = split on
};
struct ConfigurationV11 {
  DeviceSettingsV9 device;
  PresetSettingsV8 settings;
  PresetSettingsV8 preset[4];
  SequencerProject project;
};
/**************************************** Configuration V12 ****************************************
This is used by firmware v2.0.3-beta1
**************************************************************************************************/
struct DeviceSettingsV10 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][MAXROWS];          // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  byte sensorSensitivityZ;                   // the scaling factor of the raw value of Z in percentage
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean sleepAnimationActive;              // store whether an animation was active last
  boolean sleepActive;                       // store whether LinnStrument should go to sleep automatically
  byte sleepDelay;                           // the number of minutes it takes for sleep to kick in
  byte sleepAnimationType;                   // the animation type to use during sleep, see SleepAnimationType
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean leftHanded;                        // whether to orient the X axis from right to left instead of from left to right
  boolean midiThrough;                       // false if incoming MIDI should be isolated, true if it should be passed through to the outgoing MIDI port
  short lastLoadedPreset;                    // the last settings preset that was loaded
  short lastLoadedProject;                   // the last sequencer project that was loaded
};
struct GlobalSettingsV8 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  byte activeNotes;                          // controls which collection of note lights presets is active
  int mainNotes[12];                         // bitmask array that determines which notes receive "main" lights
  int accentNotes[12];                       // bitmask array that determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  byte customRowOffset;                      // the custom row offset that can be configured at the location of the octave setting
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  unsigned short minForVelocity;             // 1-127
  unsigned short maxForVelocity;             // 1-127
  unsigned short valueForFixedVelocity;      // 1-127
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  unsigned short ccForSwitch;                // 0-127
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
  SustainBehavior sustainBehavior;           // the way the sustain pedal influences the notes
  boolean splitActive;                       // false = split off, true = split on
};
struct PresetSettingsV9 {
  GlobalSettingsV8 global;
  SplitSettingsV5 split[NUMSPLITS];
};
struct ConfigurationV12 {
  DeviceSettingsV10 device;
  PresetSettingsV9 settings;
  PresetSettingsV9 preset[4];
  SequencerProject project;
};
/**************************************** Configuration V13 ****************************************
This is used by firmware v2.1.0-beta2 and firmware v2.1.0-beta3
**************************************************************************************************/
struct DeviceSettingsV11 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][MAXROWS];          // store nine columns of calibration data
  uint32_t calCrc;                           // the CRC check value of the calibration data to see if it's still valid
  boolean calCrcCalculated;                  // indicates whether the CRC of the calibration was calculated, previous firmware versions didn't
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  byte sensorSensitivityZ;                   // the scaling factor of the raw value of Z in percentage
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean sleepAnimationActive;              // store whether an animation was active last
  boolean sleepActive;                       // store whether LinnStrument should go to sleep automatically
  byte sleepDelay;                           // the number of minutes it takes for sleep to kick in
  byte sleepAnimationType;                   // the animation type to use during sleep, see SleepAnimationType
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean otherHanded;                       // whether change the handedness of the splits
  byte splitHandedness;                      // see SplitHandednessType
  boolean midiThrough;                       // false if incoming MIDI should be isolated, true if it should be passed through to the outgoing MIDI port
  short lastLoadedPreset;                    // the last settings preset that was loaded
  short lastLoadedProject;                   // the last sequencer project that was loaded
};
struct GlobalSettingsV9 {
  void setSwitchAssignment(byte, byte);

  byte splitPoint;                           // leftmost column number of right split (0 = leftmost column of playable area)
  byte currentPerSplit;                      // controls which split's settings are being displayed
  byte activeNotes;                          // controls which collection of note lights presets is active
  int mainNotes[12];                         // bitmask array that determines which notes receive "main" lights
  int accentNotes[12];                       // bitmask array that determines which notes receive accent lights (octaves, white keys, black keys, etc.)
  byte rowOffset;                            // interval between rows. 0 = no overlap, 1-12 = interval, 13 = guitar
  signed char customRowOffset;               // the custom row offset that can be configured at the location of the octave setting
  VelocitySensitivity velocitySensitivity;   // See VelocitySensitivity values
  unsigned short minForVelocity;             // 1-127
  unsigned short maxForVelocity;             // 1-127
  unsigned short valueForFixedVelocity;      // 1-127
  PressureSensitivity pressureSensitivity;   // See PressureSensitivity values
  boolean pressureAftertouch;                // Indicates whether pressure should behave like traditional piano keyboard aftertouch or be continuous from the start
  byte switchAssignment[4];                  // The element values are ASSIGNED_*.  The index values are SWITCH_*.
  boolean switchBothSplits[4];               // Indicate whether the switches should operate on both splits or only on the focused one
  unsigned short ccForSwitchCC65[4];         // 0-127
  unsigned short ccForSwitchSustain[4];      // 0-127
  unsigned short customSwitchAssignment[4];  // ASSIGNED_TAP_TEMPO, ASSIGNED_LEGATO, ASSIGNED_LATCH, ASSIGNED_PRESET_UP, ASSIGNED_PRESET_DOWN, ASSIGNED_REVERSE_PITCH_X, ASSIGNED_SEQUENCER_PLAY, ASSIGNED_SEQUENCER_PREV or ASSIGNED_SEQUENCER_NEXT
  byte midiIO;                               // 0 = MIDI jacks, 1 = USB
  ArpeggiatorDirection arpDirection;         // the arpeggiator direction that has to be used for the note sequence
  ArpeggiatorStepTempo arpTempo;             // the multiplier that needs to be applied to the current tempo to achieve the arpeggiator's step duration
  signed char arpOctave;                     // the number of octaves that the arpeggiator has to operate over: 0, +1, or +2
  SustainBehavior sustainBehavior;           // the way the sustain pedal influences the notes
  boolean splitActive;                       // false = split off, true = split on
};
struct SplitSettingsV6 {
  byte midiMode;                          // 0 = one channel, 1 = note per channel, 2 = row per channel
  byte midiChanMain;                      // main midi channel, 1 to 16
  byte midiChanPerRow;                    // per-row midi channel, 1 to 16
  boolean midiChanPerRowReversed;         // indicates whether channel per row channels count upwards or downwards across the rows
  boolean midiChanSet[16];                // Indicates whether each channel is used.  If midiMode!=channelPerNote, only one channel can be set.
  BendRangeOption bendRangeOption;        // see BendRangeOption
  byte customBendRange;                   // 1 - 96
  boolean sendX;                          // true to send continuous X, false if not
  boolean sendY;                          // true to send continuous Y, false if not
  boolean sendZ;                          // true to send continuous Z, false if not
  boolean pitchCorrectQuantize;           // true to quantize pitch of initial touch, false if not
  byte pitchCorrectHold;                  // See PitchCorrectHoldSpeed values
  boolean pitchResetOnRelease;            // true to enable pitch bend being set back to 0 when releasing a touch
  TimbreExpression expressionForY;        // the expression that should be used for timbre
  unsigned short customCCForY;            // 0-129 (with 128 and 129 being placeholders for PolyPressure and ChannelPressure)
  unsigned short minForY;                 // 0-127
  unsigned short maxForY;                 // 0-127
  boolean relativeY;                      // true when Y should be sent relative to the initial touch, false when it's absolute
  unsigned short initialRelativeY;        // 0-127
  LoudnessExpression expressionForZ;      // the expression that should be used for loudness
  unsigned short customCCForZ;            // 0-127
  unsigned short minForZ;                 // 0-127
  unsigned short maxForZ;                 // 0-127
  boolean ccForZ14Bit;                    // true when 14-bit messages should be sent when Z CC is between 0-31, false when only 7-bit messages should be sent
  unsigned short ccForFader[8];           // each fader can control a CC number ranging from 0-128 (with 128 being placeholder for ChannelPressure)
  byte colorMain;                         // color for non-accented cells
  byte colorAccent;                       // color for accented cells
  byte colorPlayed;                       // color for played notes
  byte colorLowRow;                       // color for low row if on
  byte colorSequencerEmpty;               // color for sequencer low row step with no events
  byte colorSequencerEvent;               // color for sequencer low row step with events
  byte colorSequencerDisabled;            // color for sequencer low row step that's not being played
  byte playedTouchMode;                   // see PlayedTouchMode values
  byte lowRowMode;                        // see LowRowMode values
  byte lowRowCCXBehavior;                 // see LowRowCCBehavior values
  unsigned short ccForLowRow;             // 0-128 (with 128 being placeholder for ChannelPressure)
  byte lowRowCCXYZBehavior;               // see LowRowCCBehavior values
  unsigned short ccForLowRowX;            // 0-128 (with 128 being placeholder for ChannelPressure)
  unsigned short ccForLowRowY;            // 0-128 (with 128 being placeholder for ChannelPressure)
  unsigned short ccForLowRowZ;            // 0-128 (with 128 being placeholder for ChannelPressure)
  signed char transposeOctave;            // -60, -48, -36, -24, -12, 0, +12, +24, +36, +48, +60
  signed char transposePitch;             // transpose output midi notes. Range is -12 to +12
  signed char transposeLights;            // transpose lights on display. Range is -12 to +12
  boolean ccFaders;                       // true to activated 8 CC faders for this split, false for regular music performance
  boolean arpeggiator;                    // true when the arpeggiator is on, false if notes should be played directly
  boolean strum;                          // true when this split strums the touches of the other split
  boolean mpe;                            // true when MPE is active for this split
  boolean sequencer;                      // true when the sequencer of this split is displayed
  SequencerView sequencerView;            // see SequencerView
};
struct PresetSettingsV10 {
  GlobalSettingsV9 global;
  SplitSettingsV6 split[NUMSPLITS];
};
struct ConfigurationV13 {
  DeviceSettingsV11 device;
  PresetSettingsV10 settings;
  PresetSettingsV10 preset[4];
  SequencerProject project;
};
/**************************************** Configuration V14 ****************************************
This is used by firmware v2.1.0
**************************************************************************************************/
struct DeviceSettingsV12 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[MAXCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][MAXROWS];          // store nine columns of calibration data
  uint32_t calCrc;                           // the CRC check value of the calibration data to see if it's still valid
  boolean calCrcCalculated;                  // indicates whether the CRC of the calibration was calculated, previous firmware versions didn't
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  boolean calibrationHealed;                 // indicates whether the calibration data was healed
  unsigned short minUSBMIDIInterval;         // the minimum delay between MIDI bytes when sent over USB
  byte sensorSensitivityZ;                   // the scaling factor of the raw value of Z in percentage
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean sleepAnimationActive;              // store whether an animation was active last
  boolean sleepActive;                       // store whether LinnStrument should go to sleep automatically
  byte sleepDelay;                           // the number of minutes it takes for sleep to kick in
  byte sleepAnimationType;                   // the animation type to use during sleep, see SleepAnimationType
  char audienceMessages[16][31];             // the 16 audience messages that will scroll across the surface
  boolean operatingLowPower;                 // whether low power mode is active or not
  boolean otherHanded;                       // whether change the handedness of the splits
  byte splitHandedness;                      // see SplitHandednessType
  boolean midiThrough;                       // false if incoming MIDI should be isolated, true if it should be passed through to the outgoing MIDI port
  short lastLoadedPreset;                    // the last settings preset that was loaded
  short lastLoadedProject;                   // the last sequencer project that was loaded
};
struct ConfigurationV14 {
  DeviceSettingsV12 device;
  PresetSettingsV10 settings;
  PresetSettingsV10 preset[4];
  SequencerProject project;
};
/**************************************** Configuration V15 ****************************************
This is used by firmware v2.2.0, v2.2.1, v2.2.2
**************************************************************************************************/
struct ConfigurationV15 {
  DeviceSettingsV12 device;
  PresetSettings settings;
  PresetSettings preset[6];
  SequencerProject project;
};
/*************************************************************************************************/

boolean upgradeConfigurationSettings(int32_t confSize, byte* buff2) {
  boolean result = false;

  byte settingsVersion = buff2[0];

  // if the stored version is newer than what this firmware supports, resort to default settings
  if (settingsVersion > Device.version) {
    result = false;
  }
  else {
    void* sourceConfig = buff2;
    void (*copyConfigurationFunction)(void* target, void* source) = NULL;

    switch (settingsVersion) {
      // if this is v1 of the configuration format, load it in the old structure and then convert it if the size is right
      case 1:
        if (confSize == sizeof(ConfigurationV1)) {
          copyConfigurationFunction = &copyConfigurationV1;
        }
        break;
      // this is the v2 of the configuration configuration, apply it if the size is right
      case 2:
        if (confSize == sizeof(ConfigurationV2)) {
          copyConfigurationFunction = &copyConfigurationV2;
        }
        break;
      // this is the v3 of the configuration configuration, apply it if the size is right
      case 3:
        if (confSize == sizeof(ConfigurationV3)) {
          copyConfigurationFunction = &copyConfigurationV3;
        }
        break;
      // this is the v4 of the configuration configuration, apply it if the size is right
      case 4:
        if (confSize == sizeof(ConfigurationV4)) {
          copyConfigurationFunction = &copyConfigurationV4;
        }
        break;
      // this is the v5 of the configuration configuration, apply it if the size is right
      case 5:
        if (confSize == sizeof(ConfigurationV5)) {
          copyConfigurationFunction = &copyConfigurationV5;
        }
        break;
      // this is the v6 of the configuration configuration, apply it if the size is right
      case 6:
        if (confSize == sizeof(ConfigurationV6)) {
          copyConfigurationFunction = &copyConfigurationV6;
        }
        break;
      // this is the v7 of the configuration configuration, apply it if the size is right
      case 7:
        if (confSize == sizeof(ConfigurationV7)) {
          copyConfigurationFunction = &copyConfigurationV7;
        }
        break;
      // this is the v8 of the configuration configuration, apply it if the size is right
      case 8:
        if (confSize == sizeof(ConfigurationV8)) {
          copyConfigurationFunction = &copyConfigurationV8;
        }
        break;
      // this is the v9 of the configuration configuration, apply it if the size is right
      case 9:
        if (confSize == sizeof(ConfigurationV9)) {
          copyConfigurationFunction = &copyConfigurationV9;
        }
        break;
      // this is the v10 of the configuration configuration, apply it if the size is right
      case 10:
        if (confSize == sizeof(ConfigurationV10)) {
          copyConfigurationFunction = &copyConfigurationV10;
        }
        break;
      // this is the v11 of the configuration configuration, apply it if the size is right
      case 11:
        if (confSize == sizeof(ConfigurationV11)) {
          copyConfigurationFunction = &copyConfigurationV11;
        }
        break;
      // this is the v12 of the configuration configuration, apply it if the size is right
      case 12:
        if (confSize == sizeof(ConfigurationV12)) {
          copyConfigurationFunction = &copyConfigurationV12;
        }
        break;
      // this is the v13 of the configuration configuration, apply it if the size is right
      case 13:
        if (confSize == sizeof(ConfigurationV13)) {
          copyConfigurationFunction = &copyConfigurationV13;
        }
        break;
      // this is the v14 of the configuration configuration, apply it if the size is right
      case 14:
        if (confSize == sizeof(ConfigurationV14)) {
          copyConfigurationFunction = &copyConfigurationV14;
        }
        break;
      // this is the v15 of the configuration configuration, apply it if the size is right
      case 15:
        if (confSize == sizeof(ConfigurationV15)) {
          copyConfigurationFunction = &copyConfigurationV15;
        }
        break;
      // this is the v16 of the configuration configuration, apply it if the size is right
      case 16:
        if (confSize == sizeof(Configuration)) {
          memcpy(&config, buff2, confSize);
          result = true;
        }
        break;
      default:
        result = false;
        break;
    }

    // if a target config and a copy settings fuction were set, use them to transform the old settings into the new
    if (sourceConfig && copyConfigurationFunction) {
      byte currentVersion = Device.version;
      copyConfigurationFunction(&config, sourceConfig);
      Device.version = currentVersion;
      
      result = true;
    }
  }

  return result;
}

void copyCalibrationV1(CalibrationX (*calRowsTarget)[MAXCOLS+1][4], CalibrationX (*calRowsSource)[MAXCOLS+1][4], CalibrationY (*calColsTarget)[9][MAXROWS], CalibrationYV1 (*calColsSource)[9][MAXROWS]) {
  for (int i = 0; i < MAXCOLS+1; ++i) {
    for (int j = 0; j < 4; ++j) {
      (*calRowsTarget)[i][j].fxdMeasuredX = (*calRowsSource)[i][j].fxdMeasuredX;
      (*calRowsTarget)[i][j].fxdReferenceX = (*calRowsSource)[i][j].fxdReferenceX;
      (*calRowsTarget)[i][j].fxdRatio = (*calRowsSource)[i][j].fxdRatio;
    }
  }
  for (int i = 0; i < 9; ++i) {
    for (int j = 0; j < MAXROWS; ++j) {
      (*calColsTarget)[i][j].minY = min(max((*calColsSource)[i][j].minY, 0), 4095);
      (*calColsTarget)[i][j].maxY = min(max((*calColsSource)[i][j].maxY, 0), 4095);
      (*calColsTarget)[i][j].fxdRatio = (*calColsSource)[i][j].fxdRatio;
    }
  }
}

void copyCalibrationV2(CalibrationX (*calRowsTarget)[MAXCOLS+1][4], CalibrationX (*calRowsSource)[MAXCOLS+1][4], CalibrationY (*calColsTarget)[9][MAXROWS], CalibrationY (*calColsSource)[9][MAXROWS]) {
  for (int i = 0; i < MAXCOLS+1; ++i) {
    for (int j = 0; j < 4; ++j) {
      (*calRowsTarget)[i][j].fxdMeasuredX = (*calRowsSource)[i][j].fxdMeasuredX;
      (*calRowsTarget)[i][j].fxdReferenceX = (*calRowsSource)[i][j].fxdReferenceX;
      (*calRowsTarget)[i][j].fxdRatio = (*calRowsSource)[i][j].fxdRatio;
    }
  }
  for (int i = 0; i < 9; ++i) {
    for (int j = 0; j < MAXROWS; ++j) {
      (*calColsTarget)[i][j].minY = min(max((*calColsSource)[i][j].minY, 0), 4095);
      (*calColsTarget)[i][j].maxY = min(max((*calColsSource)[i][j].maxY, 0), 4095);
      (*calColsTarget)[i][j].fxdRatio = (*calColsSource)[i][j].fxdRatio;
    }
  }
}

void copyAudienceMessages(char (*target)[16][31], char (*source)[16][31]) {
  for (byte msg = 0; msg < 16; ++msg) {
    memset((*target)[msg], '\0', 31);
    strncpy((*target)[msg], (*source)[msg], 30);
    (*target)[msg][30] = '\0';
  }
}

void setPromoAnimation(void* target, boolean flag) {
  DeviceSettings* t = (DeviceSettings*)target;
  if (flag) {
    t->sleepActive = true;
    t->sleepDelay = 2;
    t->sleepAnimationType = animationStore;
  }
  else {
    t->sleepActive = false;
    t->sleepDelay = 0;
    t->sleepAnimationType = animationNone;
  }
}

/*************************************************************************************************/

void copyConfigurationV1(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV1* s = (ConfigurationV1*)source;
  GlobalSettingsV1* g = &(s->global);

  t->device.version = g->version;
  copyCalibrationV1(&(t->device.calRows), &(g->calRows), &(t->device.calCols), &(g->calCols));
  t->device.calibrated = g->calibrated;
  t->device.sensorRangeZ = g->sensorRangeZ;
  setPromoAnimation(&t->device, g->promoAnimationAtStartup);
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  initializeAudienceMessages();

  for (byte p = 0; p < 4; ++p) {
    t->preset[p].global.splitPoint = g->splitPoint;
    t->preset[p].global.currentPerSplit = g->currentPerSplit;
    copyGlobalSettingsNoteLights(&t->preset[p].global, g->mainNotes, g->accentNotes);
    t->preset[p].global.rowOffset = g->rowOffset;
    t->preset[p].global.velocitySensitivity = g->velocitySensitivity;
    t->preset[p].global.pressureSensitivity = g->pressureSensitivity;
    memcpy(t->preset[p].global.switchAssignment, g->switchAssignment, sizeof(byte)*4);
    memcpy(t->preset[p].global.switchBothSplits, g->switchBothSplits, sizeof(boolean)*4);
    t->preset[p].global.midiIO = g->midiIO;
    t->preset[p].global.arpDirection = g->arpDirection;
    t->preset[p].global.arpTempo = g->arpTempo;
    t->preset[p].global.arpOctave = g->arpOctave;

    copySplitSettingsV1(&t->preset[p].split[LEFT], &s->left);
    copySplitSettingsV1(&t->preset[p].split[RIGHT], &s->right);
  }
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySplitSettingsV1(void* target, void* source) {
  SplitSettings* t = (SplitSettings*)target;
  SplitSettingsV1* s = (SplitSettingsV1*)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*16);
  applyBendRange(*t, s->bendRange);
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  if (s->ccForY == 1) {
    t->expressionForY = timbreCC1;
    t->customCCForY = 74;
  }
  else {
    t->expressionForY = timbreCC74;
    t->customCCForY = s->ccForY;
  }
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  if (t->expressionForZ == loudnessCC11) {
    t->customCCForZ = s->ccForZ;
  }
  else {
    t->customCCForZ = 11;
  }
  memcpy(t->ccForFader, ccFaderDefaults, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorPlayed = s->colorPlayed;
  t->colorLowRow = s->colorLowRow;
  t->playedTouchMode = playedSame;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = lowRowCCHold;
  t->lowRowCCXYZBehavior = lowRowCCHold;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
}

/*************************************************************************************************/

void copyConfigurationV2(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV2* s = (ConfigurationV2*)source;

  t->device.version = s->device.version;
  copyCalibrationV1(&(t->device.calRows), &(s->device.calRows), &(t->device.calCols), &(s->device.calCols));
  t->device.calibrated = s->device.calibrated;
  t->device.sensorRangeZ = s->device.sensorRangeZ;
  setPromoAnimation(&t->device, s->device.promoAnimationAtStartup);
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  initializeAudienceMessages();

  copyPresetSettingsOfConfigurationV2(t, s);
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySplitSettingsV2(void* target, void* source) {
  SplitSettings* t = (SplitSettings*)target;
  SplitSettingsV2* s = (SplitSettingsV2*)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*16);
  applyBendRange(*t, s->bendRange);
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  switch (s->expressionForY) {
    case timbrePolyPressure:
      t->expressionForY = s->expressionForY;
      t->customCCForY = 128;
      break;
    case timbreChannelPressure:
      t->expressionForY = s->expressionForY;
      t->customCCForY = 129;
      break;
    case timbreCC1:
    case timbreCC74:
      if (s->ccForY == 1) {
        t->expressionForY = timbreCC1;
        t->customCCForY = 74;
      }
      else {
        t->expressionForY = timbreCC74;
        t->customCCForY = s->ccForY;
      }
      break;
  }
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  if (t->expressionForZ == loudnessCC11) {
    t->customCCForZ = s->ccForZ;
  }
  else {
    t->customCCForZ = 11;
  }
  memcpy(t->ccForFader, ccFaderDefaults, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorPlayed = s->colorPlayed;
  t->colorLowRow = s->colorLowRow;
  t->playedTouchMode = playedSame;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = lowRowCCHold;
  t->lowRowCCXYZBehavior = lowRowCCHold;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
}

void copyPresetSettingsOfConfigurationV2(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV2* s = (ConfigurationV2*)source;

  for (byte p = 0; p < 4; ++p) {
    t->preset[p].global.splitPoint = s->preset[p].global.splitPoint;
    t->preset[p].global.currentPerSplit = s->preset[p].global.currentPerSplit;
    copyGlobalSettingsNoteLights(&t->preset[p].global, s->preset[p].global.mainNotes, s->preset[p].global.accentNotes);
    t->preset[p].global.rowOffset = s->preset[p].global.rowOffset;
    t->preset[p].global.velocitySensitivity = s->preset[p].global.velocitySensitivity;
    t->preset[p].global.pressureSensitivity = s->preset[p].global.pressureSensitivity;
    memcpy(t->preset[p].global.switchAssignment, s->preset[p].global.switchAssignment, sizeof(byte)*4);
    memcpy(t->preset[p].global.switchBothSplits, s->preset[p].global.switchBothSplits, sizeof(boolean)*4);
    t->preset[p].global.midiIO = s->preset[p].global.midiIO;
    t->preset[p].global.arpDirection = s->preset[p].global.arpDirection;
    t->preset[p].global.arpTempo = s->preset[p].global.arpTempo;
    t->preset[p].global.arpOctave = s->preset[p].global.arpOctave;

    copySplitSettingsV2(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

/*************************************************************************************************/

void copyConfigurationV3(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV3* s = (ConfigurationV3*)source;

  t->device.version = s->device.version;
  copyCalibrationV1(&(t->device.calRows), &(s->device.calRows), &(t->device.calCols), &(s->device.calCols));
  t->device.calibrated = s->device.calibrated;
  t->device.sensorRangeZ = s->device.sensorRangeZ;
  setPromoAnimation(&t->device, s->device.promoAnimationAtStartup);
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  copyAudienceMessages(&(t->device.audienceMessages), &(s->device.audienceMessages));

  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsOfConfigurationV3(t, s);
  }
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copyGlobalSettingsV3(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV3* s = (GlobalSettingsV3*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  copyGlobalSettingsNoteLights(t, s->mainNotes, s->accentNotes);
  t->rowOffset = s->rowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = sustainHold;
}

void copyPresetSettingsOfConfigurationV3(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV3* s = (ConfigurationV3*)source;

  for (byte p = 0; p < 4; ++p) {
    copyGlobalSettingsV3(&t->preset[p].global, &s->preset[p].global);

    copySplitSettingsV2(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

/*************************************************************************************************/

void copyConfigurationV4(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV4* s = (ConfigurationV4*)source;

  copyDeviceSettingsV4(&t->device, &s->device);

  copyPresetSettingsV3(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV3(&t->preset[p], &s->preset[p]);
  }
}

void copyDeviceSettingsV4(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV4* s = (DeviceSettingsV4*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV1(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->sensorRangeZ = s->sensorRangeZ;
  setPromoAnimation(t, s->promoAnimationAtStartup);
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
}

void copyPresetSettingsV3(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV3* s = (PresetSettingsV3*)source;

  copyGlobalSettingsV3(&t->global, &s->global);

  copySplitSettingsV2(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV2(&t->split[RIGHT], &s->split[RIGHT]);
}

/*************************************************************************************************/

void copyConfigurationV5(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV5* s = (ConfigurationV5*)source;

  copyDeviceSettingsV4(&t->device, &s->device);

  copyPresetSettingsV4(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV4(&t->preset[p], &s->preset[p]);
  }
}

void copyPresetSettingsV4(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV4* s = (PresetSettingsV4*)source;

  copyGlobalSettingsV4(&t->global, &s->global);

  copySplitSettingsV3(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV3(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyGlobalSettingsNoteLights(void* target, boolean* sourceMainNotes, boolean* sourceAccentNotes) {
  GlobalSettings* t = (GlobalSettings*)target;

  initializeNoteLights(*t);
  t->mainNotes[0] = 0;
  t->accentNotes[0] = 0;
  for (byte n = 0; n < 12; ++n) {
    if (sourceMainNotes[n]) {
      t->mainNotes[0] |= 1 << n;
    }
    if (sourceAccentNotes[n]) {
      t->accentNotes[0] |= 1 << n;
    }
  }
}

void copyGlobalSettingsV4(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV4* s = (GlobalSettingsV4*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  copyGlobalSettingsNoteLights(t, s->mainNotes, s->accentNotes);
  t->rowOffset = s->rowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitchCC65[SWITCH_FOOT_L] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_FOOT_R] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_1] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_2] = s->ccForSwitch;
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = sustainHold;
}

void copySplitSettingsV3(void* target, void* source) {
  SplitSettings* t = (SplitSettings*)target;
  SplitSettingsV3* s = (SplitSettingsV3*)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*16);
  t->bendRangeOption = s->bendRangeOption;
  t->customBendRange = s->customBendRange;
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  t->expressionForY = s->expressionForY;
  t->customCCForY = s->customCCForY;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  t->customCCForZ = s->customCCForZ;
  memcpy(t->ccForFader, s->ccForFader, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorPlayed = s->colorPlayed;
  t->colorLowRow = s->colorLowRow;
  t->playedTouchMode = playedSame;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = s->lowRowCCXBehavior;
  t->ccForLowRow = s->ccForLowRow;
  t->lowRowCCXYZBehavior = s->lowRowCCXYZBehavior;
  t->ccForLowRowX = s->ccForLowRowX;
  t->ccForLowRowY = s->ccForLowRowY;
  t->ccForLowRowZ = s->ccForLowRowZ;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
  t->mpe = s->mpe;
}

/*************************************************************************************************/

void copyConfigurationV6(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV6* s = (ConfigurationV6*)source;

  copyDeviceSettingsV4(&t->device, &s->device);

  copyPresetSettingsV5(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV5(&t->preset[p], &s->preset[p]);
  }
}

void copyPresetSettingsV5(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV5* s = (PresetSettingsV5*)source;

  copyGlobalSettingsV5(&t->global, &s->global);

  copySplitSettingsV4(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV4(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyGlobalSettingsV5(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV5* s = (GlobalSettingsV5*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  t->activeNotes = s->activeNotes;
  memcpy(t->mainNotes, s->mainNotes, sizeof(int)*12);
  memcpy(t->accentNotes, s->accentNotes, sizeof(int)*12);
  t->rowOffset = s->rowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitchCC65[SWITCH_FOOT_L] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_FOOT_R] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_1] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_2] = s->ccForSwitch;
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = s->sustainBehavior;
}

void copySplitSettingsV4(void* target, void* source) {
  SplitSettings* t = (SplitSettings*)target;
  SplitSettingsV4* s = (SplitSettingsV4*)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*16);
  t->bendRangeOption = s->bendRangeOption;
  t->customBendRange = s->customBendRange;
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  t->expressionForY = s->expressionForY;
  t->customCCForY = s->customCCForY;
  t->minForY = s->minForY;
  t->maxForY = s->maxForY;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  t->customCCForZ = s->customCCForZ;
  t->minForZ = s->minForZ;
  t->maxForZ = s->maxForZ;
  memcpy(t->ccForFader, s->ccForFader, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorPlayed = s->colorNoteon;
  t->colorLowRow = s->colorLowRow;
  t->playedTouchMode = playedSame;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = s->lowRowCCXBehavior;
  t->ccForLowRow = s->ccForLowRow;
  t->lowRowCCXYZBehavior = s->lowRowCCXYZBehavior;
  t->ccForLowRowX = s->ccForLowRowX;
  t->ccForLowRowY = s->ccForLowRowY;
  t->ccForLowRowZ = s->ccForLowRowZ;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
  t->mpe = s->mpe;
}

/*************************************************************************************************/

void copyConfigurationV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV7* s = (ConfigurationV7*)source;

  copyDeviceSettingsV5(&t->device, &s->device);

  copyPresetSettingsV6(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV6(&t->preset[p], &s->preset[p]);
  }
}

void copyDeviceSettingsV5(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV5* s = (DeviceSettingsV5*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV1(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  setPromoAnimation(t, s->promoAnimation);
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
}

void copyPresetSettingsV6(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV6* s = (PresetSettingsV6*)source;

  copyGlobalSettingsV6(&t->global, &s->global);

  copySplitSettingsV4(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV4(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyGlobalSettingsV6(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV6* s = (GlobalSettingsV6*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  t->activeNotes = s->activeNotes;
  memcpy(t->mainNotes, s->mainNotes, sizeof(int)*12);
  memcpy(t->accentNotes, s->accentNotes, sizeof(int)*12);
  t->rowOffset = s->rowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->minForVelocity = s->minForVelocity;
  t->maxForVelocity = s->maxForVelocity;
  t->valueForFixedVelocity = s->valueForFixedVelocity;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitchCC65[SWITCH_FOOT_L] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_FOOT_R] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_1] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_2] = s->ccForSwitch;
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = s->sustainBehavior;
}

/*************************************************************************************************/

void copyConfigurationV8(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV8* s = (ConfigurationV8*)source;

  copyDeviceSettingsV6(&t->device, &s->device);

  copyPresetSettingsV7(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV7(&t->preset[p], &s->preset[p]);
  }
}

void copyDeviceSettingsV6(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV6* s = (DeviceSettingsV6*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV1(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->sleepAnimationActive = s->sleepAnimationActive;
  t->sleepActive = s->sleepActive;
  t->sleepDelay = s->sleepDelay;
  t->sleepAnimationType = s->sleepAnimation ? animationStore : animationNone;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->otherHanded = s->leftHanded;
}

void copyPresetSettingsV7(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV7* s = (PresetSettingsV7*)source;

  copyGlobalSettingsV7(&t->global, &s->global);

  copySplitSettingsV4(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV4(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyGlobalSettingsV7(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV7* s = (GlobalSettingsV7*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  t->activeNotes = s->activeNotes;
  memcpy(t->mainNotes, s->mainNotes, sizeof(int)*12);
  memcpy(t->accentNotes, s->accentNotes, sizeof(int)*12);
  t->rowOffset = s->rowOffset;
  t->customRowOffset = s->customRowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->minForVelocity = s->minForVelocity;
  t->maxForVelocity = s->maxForVelocity;
  t->valueForFixedVelocity = s->valueForFixedVelocity;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitchCC65[SWITCH_FOOT_L] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_FOOT_R] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_1] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_2] = s->ccForSwitch;
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = s->sustainBehavior;
}

/*************************************************************************************************/

void copyConfigurationV9(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV9* s = (ConfigurationV9*)source;

  copyDeviceSettingsV7(&t->device, &s->device);

  copyPresetSettingsV8(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV8(&t->preset[p], &s->preset[p]);
  }

  memcpy(&t->project, &s->project, sizeof(SequencerProject));
}

void copyDeviceSettingsV7(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV7* s = (DeviceSettingsV7*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV2(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->sleepAnimationActive = s->sleepAnimationActive;
  t->sleepActive = s->sleepActive;
  t->sleepDelay = s->sleepDelay;
  t->sleepAnimationType = s->sleepAnimation ? animationStore : animationNone;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->otherHanded = s->leftHanded;
  t->midiThrough = s->midiThrough;
}

void copyPresetSettingsV8(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV8* s = (PresetSettingsV8*)source;

  copyGlobalSettingsV7(&t->global, &s->global);

  copySplitSettingsV5(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV5(&t->split[RIGHT], &s->split[RIGHT]);
}

void copySplitSettingsV5(void* target, void* source) {
  SplitSettings* t = (SplitSettings*)target;
  SplitSettingsV5* s = (SplitSettingsV5*)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*16);
  t->bendRangeOption = s->bendRangeOption;
  t->customBendRange = s->customBendRange;
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  t->expressionForY = s->expressionForY;
  t->customCCForY = s->customCCForY;
  t->minForY = s->minForY;
  t->maxForY = s->maxForY;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  t->customCCForZ = s->customCCForZ;
  t->minForZ = s->minForZ;
  t->maxForZ = s->maxForZ;
  memcpy(t->ccForFader, s->ccForFader, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorPlayed = s->colorPlayed;
  t->colorLowRow = s->colorLowRow;
  t->colorSequencerEmpty = s->colorSequencerEmpty;
  t->colorSequencerEvent = s->colorSequencerEvent;
  t->colorSequencerDisabled = s->colorSequencerDisabled;
  t->playedTouchMode = playedSame;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = s->lowRowCCXBehavior;
  t->ccForLowRow = s->ccForLowRow;
  t->lowRowCCXYZBehavior = s->lowRowCCXYZBehavior;
  t->ccForLowRowX = s->ccForLowRowX;
  t->ccForLowRowY = s->ccForLowRowY;
  t->ccForLowRowZ = s->ccForLowRowZ;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
  t->mpe = s->mpe;
  t->sequencer = s->sequencer;
  t->sequencerView = s->sequencerView;
}

/*************************************************************************************************/

void copyConfigurationV10(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV10* s = (ConfigurationV10*)source;

  copyDeviceSettingsV8(&t->device, &s->device);

  copyPresetSettingsV8(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV8(&t->preset[p], &s->preset[p]);
  }

  memcpy(&t->project, &s->project, sizeof(SequencerProject));
}

void copyDeviceSettingsV8(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV8* s = (DeviceSettingsV8*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV2(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->sleepAnimationActive = s->sleepAnimationActive;
  t->sleepActive = s->sleepActive;
  t->sleepDelay = s->sleepDelay;
  t->sleepAnimationType = s->sleepAnimation ? animationStore : animationNone;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->otherHanded = s->leftHanded;
  t->midiThrough = s->midiThrough;
  t->lastLoadedPreset = s->lastLoadedPreset;
  t->lastLoadedProject = s->lastLoadedProject;
}

/*************************************************************************************************/

void copyConfigurationV11(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV11* s = (ConfigurationV11*)source;

  copyDeviceSettingsV9(&t->device, &s->device);

  copyPresetSettingsV8(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV8(&t->preset[p], &s->preset[p]);
  }

  memcpy(&t->project, &s->project, sizeof(SequencerProject));
}

void copyDeviceSettingsV9(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV9* s = (DeviceSettingsV9*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV2(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorSensitivityZ = s->sensorSensitivityZ;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->sleepAnimationActive = s->sleepAnimationActive;
  t->sleepActive = s->sleepActive;
  t->sleepDelay = s->sleepDelay;
  t->sleepAnimationType = s->sleepAnimationType;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->otherHanded = s->leftHanded;
  t->midiThrough = s->midiThrough;
  t->lastLoadedPreset = s->lastLoadedPreset;
  t->lastLoadedProject = s->lastLoadedProject;
}

/*************************************************************************************************/

void copyConfigurationV12(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV12* s = (ConfigurationV12*)source;

  copyDeviceSettingsV10(&t->device, &s->device);

  copyPresetSettingsV9(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV9(&t->preset[p], &s->preset[p]);
  }

  memcpy(&t->project, &s->project, sizeof(SequencerProject));
}

void copyDeviceSettingsV10(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV10* s = (DeviceSettingsV10*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV2(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorSensitivityZ = s->sensorSensitivityZ;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->sleepAnimationActive = s->sleepAnimationActive;
  t->sleepActive = s->sleepActive;
  t->sleepDelay = s->sleepDelay;
  t->sleepAnimationType = s->sleepAnimationType;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->otherHanded = s->leftHanded;
  t->midiThrough = s->midiThrough;
  t->lastLoadedPreset = s->lastLoadedPreset;
  t->lastLoadedProject = s->lastLoadedProject;
}

void copyPresetSettingsV9(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV9* s = (PresetSettingsV9*)source;

  copyGlobalSettingsV8(&t->global, &s->global);

  copySplitSettingsV5(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV5(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyGlobalSettingsV8(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV8* s = (GlobalSettingsV8*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  t->activeNotes = s->activeNotes;
  memcpy(t->mainNotes, s->mainNotes, sizeof(int)*12);
  memcpy(t->accentNotes, s->accentNotes, sizeof(int)*12);
  t->rowOffset = s->rowOffset;
  t->customRowOffset = s->customRowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->minForVelocity = s->minForVelocity;
  t->maxForVelocity = s->maxForVelocity;
  t->valueForFixedVelocity = s->valueForFixedVelocity;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitchCC65[SWITCH_FOOT_L] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_FOOT_R] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_1] = s->ccForSwitch;
  t->ccForSwitchCC65[SWITCH_SWITCH_2] = s->ccForSwitch;
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = s->sustainBehavior;
  t->splitActive = s->splitActive;
}

/*************************************************************************************************/

void copyConfigurationV13(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV13* s = (ConfigurationV13*)source;

  copyDeviceSettingsV11(&t->device, &s->device);

  copyPresetSettingsV10(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV10(&t->preset[p], &s->preset[p]);
  }

  // we inserted another touch animation option at the beginning
  // to replace the default one
  t->settings.split[LEFT].playedTouchMode = t->settings.split[LEFT].playedTouchMode + 1;
  t->settings.split[RIGHT].playedTouchMode = t->settings.split[LEFT].playedTouchMode + 1;
  for (byte p = 0; p < 4; ++p) {
    t->preset[p].split[LEFT].playedTouchMode = t->preset[p].split[LEFT].playedTouchMode + 1;
    t->preset[p].split[RIGHT].playedTouchMode = t->preset[p].split[RIGHT].playedTouchMode + 1;
  }

  memcpy(&t->project, &s->project, sizeof(SequencerProject));
}

void copyDeviceSettingsV11(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV11* s = (DeviceSettingsV11*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV2(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calCrc = s->calCrc;
  t->calCrcCalculated = s->calCrcCalculated;
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorSensitivityZ = s->sensorSensitivityZ;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->sleepAnimationActive = s->sleepAnimationActive;
  t->sleepActive = s->sleepActive;
  t->sleepDelay = s->sleepDelay;
  t->sleepAnimationType = s->sleepAnimationType;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->otherHanded = s->otherHanded;
  t->splitHandedness = s->splitHandedness;
  t->midiThrough = s->midiThrough;
  t->lastLoadedPreset = s->lastLoadedPreset;
  t->lastLoadedProject = s->lastLoadedProject;
}

void copyPresetSettingsV10(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV10* s = (PresetSettingsV10*)source;

  copyGlobalSettingsV9(&t->global, &s->global);

  copySplitSettingsV6(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV6(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyGlobalSettingsV9(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV9* s = (GlobalSettingsV9*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  t->activeNotes = s->activeNotes;
  memcpy(t->mainNotes, s->mainNotes, sizeof(int)*12);
  memcpy(t->accentNotes, s->accentNotes, sizeof(int)*12);
  t->rowOffset = s->rowOffset;
  t->customRowOffset = s->customRowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->minForVelocity = s->minForVelocity;
  t->maxForVelocity = s->maxForVelocity;
  t->valueForFixedVelocity = s->valueForFixedVelocity;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  memcpy(t->ccForSwitchCC65, s->ccForSwitchCC65, sizeof(unsigned short)*4);
  memcpy(t->ccForSwitchSustain, s->ccForSwitchSustain, sizeof(unsigned short)*4);
  memcpy(t->customSwitchAssignment, s->customSwitchAssignment, sizeof(unsigned short)*4);
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = s->sustainBehavior;
  t->splitActive = s->splitActive;
}

void copySplitSettingsV6(void* target, void* source) {
  SplitSettings* t = (SplitSettings*)target;
  SplitSettingsV6* s = (SplitSettingsV6*)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanMainEnabled = true;
  t->midiChanPerRow = s->midiChanPerRow;
  t->midiChanPerRowReversed = s->midiChanPerRowReversed;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*16);
  t->bendRangeOption = s->bendRangeOption;
  t->customBendRange = s->customBendRange;
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  t->expressionForY = s->expressionForY;
  t->customCCForY = s->customCCForY;
  t->minForY = s->minForY;
  t->maxForY = s->maxForY;
  t->relativeY = s->relativeY;
  t->initialRelativeY = s->initialRelativeY;
  t->expressionForZ = s->expressionForZ;
  t->customCCForZ = s->customCCForZ;
  t->minForZ = s->minForZ;
  t->maxForZ = s->maxForZ;
  t->ccForZ14Bit = s->ccForZ14Bit;
  memcpy(t->ccForFader, s->ccForFader, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorPlayed = s->colorPlayed;
  t->colorLowRow = s->colorLowRow;
  t->colorSequencerEmpty = s->colorSequencerEmpty;
  t->colorSequencerEvent = s->colorSequencerEvent;
  t->colorSequencerDisabled = s->colorSequencerDisabled;
  t->playedTouchMode = s->playedTouchMode;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = s->lowRowCCXBehavior;
  t->ccForLowRow = s->ccForLowRow;
  t->lowRowCCXYZBehavior = s->lowRowCCXYZBehavior;
  t->ccForLowRowX = s->ccForLowRowX;
  t->ccForLowRowY = s->ccForLowRowY;
  t->ccForLowRowZ = s->ccForLowRowZ;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
  t->mpe = s->mpe;
  t->sequencer = s->sequencer;
  t->sequencerView = s->sequencerView;
}

/*************************************************************************************************/

void copyDeviceSettingsV12(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV12* s = (DeviceSettingsV12*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibrationV2(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calCrc = s->calCrc;
  t->calCrcCalculated = s->calCrcCalculated;
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = s->minUSBMIDIInterval;
  t->sensorSensitivityZ = s->sensorSensitivityZ;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->sleepAnimationActive = s->sleepAnimationActive;
  t->sleepActive = s->sleepActive;
  t->sleepDelay = s->sleepDelay;
  t->sleepAnimationType = s->sleepAnimationType;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->otherHanded = s->otherHanded;
  t->splitHandedness = s->splitHandedness;
  t->midiThrough = s->midiThrough;
  t->lastLoadedPreset = s->lastLoadedPreset;
  t->lastLoadedProject = s->lastLoadedProject;
}

void copyConfigurationV14(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV14* s = (ConfigurationV14*)source;

  copyDeviceSettingsV12(&t->device, &s->device);

  copyPresetSettingsV10(&t->settings, &s->settings);
  for (byte p = 0; p < 4; ++p) {
    copyPresetSettingsV10(&t->preset[p], &s->preset[p]);
  }

  memcpy(&t->project, &s->project, sizeof(SequencerProject));
}

/*************************************************************************************************/

void copyConfigurationV15(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV15* s = (ConfigurationV15*)source;

  copyDeviceSettingsV12(&t->device, &s->device);

  memcpy(&t->settings, &s->settings, sizeof(PresetSettings));
  for (byte p = 0; p < 6; ++p) {
    memcpy(&t->preset[p], &s->preset[p], sizeof(PresetSettings));
  }

  memcpy(&t->project, &s->project, sizeof(SequencerProject));
}
