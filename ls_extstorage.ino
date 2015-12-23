/***************************** ls_extstorage: LinnStrument Settings *******************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
Functions to exchange settings with an external device over a serial handshake protocol.
This is essentially used by the upgrade tool to temporarily store the previous settings while doing
an upgrade and sending them back after the upgrade is finished.
The new firmware is then responsible of applying the received settings and possibly performing
some transformation logic is the settings structure changed for the new firmware.
**************************************************************************************************/

/**************************************** Configuration V1 ***************************************/
/* This is used by firmware v1.0.9 and earlier
/*************************************************************************************************/
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
  CalibrationX calRows[NUMCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][NUMROWS];          // store nine columns of calibration data
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
  byte colorNoteon;                    // color for played notes
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
/**************************************** Configuration V2 ***************************************/
/* This is used by firmware v1.1.0
/*************************************************************************************************/
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
  byte colorNoteon;                    // color for played notes
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
  CalibrationX calRows[NUMCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][NUMROWS];          // store nine columns of calibration data
  boolean calibrated;                        // indicates whether the calibration data actually resulted from a calibration operation
  unsigned short sensorLoZ;                  // the lowest acceptable raw Z value to start a touch
  unsigned short sensorFeatherZ;             // the lowest acceptable raw Z value to continue a touch
  unsigned short sensorRangeZ;               // the maximum raw value of Z
  boolean promoAnimationAtStartup;           // store whether the promo animation should run at startup
  byte currentPreset;                        // the currently active settings preset
};
struct ConfigurationV2 {
  DeviceSettingsV2 device;
  PresetSettingsV2 preset[NUMPRESETS];
};
/**************************************** Configuration V3 ***************************************/
/* This is used by firmware v1.1.1-beta1, v1.1.1-beta2 and v1.1.1-beta3
/*************************************************************************************************/
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
  CalibrationX calRows[NUMCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][NUMROWS];          // store nine columns of calibration data
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
  PresetSettingsV3 preset[NUMPRESETS];
};
/**************************************** Configuration V4 ***************************************/
/* This is used by firmware v1.1.1-beta4, v1.1.2-beta1, v1.1.2 and v1.2.0-alpha1
/*************************************************************************************************/
struct DeviceSettingsV4 {
  byte version;                              // the version of the configuration format
  boolean serialMode;                        // 0 = normal MIDI I/O, 1 = Arduino serial mode for OS update and serial monitor
  CalibrationX calRows[NUMCOLS+1][4];        // store four rows of calibration data
  CalibrationY calCols[9][NUMROWS];          // store nine columns of calibration data
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
  PresetSettingsV3 preset[NUMPRESETS];
};
/**************************************** Configuration V5 ***************************************/
/* This is used by firmware v1.2.0-beta1, v1.2.0-beta2 and v1.2.0
/*************************************************************************************************/
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
struct PresetSettingsV4 {
  GlobalSettingsV4 global;
  SplitSettingsV3 split[NUMSPLITS];
};
struct ConfigurationV5 {
  DeviceSettingsV4 device;
  PresetSettingsV4 settings;
  PresetSettingsV4 preset[NUMPRESETS];
};
/**************************************** Configuration V6 ***************************************/
/* This is used by firmware v1.2.1 and 1.2.2
/*************************************************************************************************/
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
struct PresetSettingsV5 {
  GlobalSettingsV5 global;
  SplitSettings split[NUMSPLITS];
};
struct ConfigurationV6 {
  DeviceSettingsV4 device;
  PresetSettingsV5 settings;
  PresetSettingsV5 preset[NUMPRESETS];
};
/*************************************************************************************************/

boolean upgradeConfigurationSettings(int32_t confSize, byte* buff2) {
  boolean result = false;

  byte settingsVersion = buff2[0];

  // if the stored version is newer than what this firmware supports, resort to default settings
  if (settingsVersion > config.device.version) {
    result = false;
  }
  else {
    void* targetConfig = NULL;
    void (*copyConfigurationFunction)(void* target, void* source) = NULL;

    switch (settingsVersion) {
      // if this is v1 of the configuration format, load it in the old structure and then convert it if the size is right
      case 1:
        if (confSize == sizeof(ConfigurationV1)) {
          targetConfig = new ConfigurationV1();
          copyConfigurationFunction = &copyConfigurationV1;
        }
        break;
      // this is the v2 of the configuration configuration, apply it if the size is right
      case 2:
        if (confSize == sizeof(ConfigurationV2)) {
          targetConfig = new ConfigurationV2();
          copyConfigurationFunction = &copyConfigurationV2;
        }
        break;
      // this is the v3 of the configuration configuration, apply it if the size is right
      case 3:
        if (confSize == sizeof(ConfigurationV3)) {
          targetConfig = new ConfigurationV3();
          copyConfigurationFunction = &copyConfigurationV3;
        }
        break;
      // this is the v4 of the configuration configuration, apply it if the size is right
      case 4:
        if (confSize == sizeof(ConfigurationV4)) {
          targetConfig = new ConfigurationV4();
          copyConfigurationFunction = &copyConfigurationV4;
        }
        break;
      // this is the v5 of the configuration configuration, apply it if the size is right
      case 5:
        if (confSize == sizeof(ConfigurationV5)) {
          targetConfig = new ConfigurationV5();
          copyConfigurationFunction = &copyConfigurationV5;
        }
        break;
      // this is the v6 of the configuration configuration, apply it if the size is right
      case 6:
        if (confSize == sizeof(ConfigurationV6)) {
          targetConfig = new ConfigurationV5();
          copyConfigurationFunction = &copyConfigurationV6;
        }
        break;
      // this is the v7 of the configuration configuration, apply it if the size is right
      case 7:
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
    if (targetConfig && copyConfigurationFunction) {
      memcpy(targetConfig, buff2, confSize);

      byte currentVersion = config.device.version;
      copyConfigurationFunction(&config, targetConfig);
      config.device.version = currentVersion;

      switch (settingsVersion) {
        case 1:
          delete ((ConfigurationV1*)targetConfig);
          break;
        case 2:
          delete ((ConfigurationV2*)targetConfig);
          break;
        case 3:
          delete ((ConfigurationV3*)targetConfig);
          break;
        case 4:
          delete ((ConfigurationV4*)targetConfig);
          break;
        case 5:
          delete ((ConfigurationV5*)targetConfig);
          break;
      }
      result = true;
    }
  }

  return result;
}

void copyCalibration(CalibrationX (*calRowsTarget)[NUMCOLS+1][4], CalibrationX (*calRowsSource)[NUMCOLS+1][4], CalibrationY (*calColsTarget)[9][NUMROWS], CalibrationY (*calColsSource)[9][NUMROWS]) {
  for (int i = 0; i < NUMCOLS+1; ++i) {
    for (int j = 0; j < 4; ++j) {
      (*calRowsTarget)[i][j] = (*calRowsSource)[i][j];
    }
  }
  for (int i = 0; i < 9; ++i) {
    for (int j = 0; j < NUMROWS; ++j) {
      (*calColsTarget)[i][j] = (*calColsSource)[i][j];
    }
  }
}

void copyAudienceMessages(char (*target)[16][31], char (*source)[16][31]) {
  for (byte msg = 0; msg < 16; ++msg) {
    memset((*target)[msg], '\0', sizeof((*target)[msg]));
    strncpy((*target)[msg], (*source)[msg], 30);
    (*target)[msg][30] = '\0';
  }
}

void copyConfigurationV1(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV1* s = (ConfigurationV1*)source;
  GlobalSettingsV1* g = &(s->global);

  t->device.version = g->version;
  copyCalibration(&(t->device.calRows), &(g->calRows), &(t->device.calCols), &(g->calCols));
  t->device.calibrated = g->calibrated;
  t->device.sensorLoZ = DEFAULT_SENSOR_LO_Z;
  t->device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  t->device.sensorRangeZ = g->sensorRangeZ;
  t->device.promoAnimation = g->promoAnimationAtStartup;
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  t->device.leftHanded = false;
  initializeAudienceMessages();

  for (byte p = 0; p < NUMPRESETS; ++p) {
    t->preset[p].global.splitPoint = g->splitPoint;
    t->preset[p].global.currentPerSplit = g->currentPerSplit;
    copyGlobalSettingsNoteLights(&t->preset[p].global, g->mainNotes, g->accentNotes);
    t->preset[p].global.rowOffset = g->rowOffset;
    t->preset[p].global.velocitySensitivity = g->velocitySensitivity;
    t->preset[p].global.minForVelocity = 0;
    t->preset[p].global.maxForVelocity = DEFAULT_MAX_VELOCITY;
    t->preset[p].global.valueForFixedVelocity = DEFAULT_FIXED_VELOCITY;
    t->preset[p].global.pressureSensitivity = g->pressureSensitivity;
    t->preset[p].global.pressureAftertouch = false;
    memcpy(t->preset[p].global.switchAssignment, g->switchAssignment, sizeof(byte)*4);
    memcpy(t->preset[p].global.switchBothSplits, g->switchBothSplits, sizeof(boolean)*4);
    t->preset[p].global.ccForSwitch = 65;
    t->preset[p].global.midiIO = g->midiIO;
    t->preset[p].global.arpDirection = g->arpDirection;
    t->preset[p].global.arpTempo = g->arpTempo;
    t->preset[p].global.arpOctave = g->arpOctave;
    t->preset[p].global.sustainBehavior = sustainHold;

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
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*8);
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
  t->minForY = 0;
  t->maxForY = 127;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  if (t->expressionForZ == loudnessCC11) {
    t->customCCForZ = s->ccForZ;
  }
  else {
    t->customCCForZ = 11;
  }
  t->minForZ = 0;
  t->maxForZ = 127;
  memcpy(t->ccForFader, ccFaderDefaults, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorNoteon = s->colorNoteon;
  t->colorLowRow = s->colorLowRow;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = lowRowCCHold;
  t->ccForLowRow = 1;
  t->lowRowCCXYZBehavior = lowRowCCHold;
  t->ccForLowRowX = 16;
  t->ccForLowRowY = 17;
  t->ccForLowRowZ = 18;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
  t->mpe = false;
}

void copyConfigurationV2(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV2* s = (ConfigurationV2*)source;

  t->device.version = s->device.version;
  copyCalibration(&(t->device.calRows), &(s->device.calRows), &(t->device.calCols), &(s->device.calCols));
  t->device.calibrated = s->device.calibrated;
  t->device.sensorLoZ = DEFAULT_SENSOR_LO_Z;
  t->device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  t->device.sensorRangeZ = s->device.sensorRangeZ;
  t->device.promoAnimation = s->device.promoAnimationAtStartup;
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  t->device.leftHanded = false;
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
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*8);
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
  t->minForY = 0;
  t->maxForY = 127;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  if (t->expressionForZ == loudnessCC11) {
    t->customCCForZ = s->ccForZ;
  }
  else {
    t->customCCForZ = 11;
  }
  t->minForZ = 0;
  t->maxForZ = 127;
  memcpy(t->ccForFader, ccFaderDefaults, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorNoteon = s->colorNoteon;
  t->colorLowRow = s->colorLowRow;
  t->lowRowMode = s->lowRowMode;
  t->lowRowCCXBehavior = lowRowCCHold;
  t->ccForLowRow = 1;
  t->lowRowCCXYZBehavior = lowRowCCHold;
  t->ccForLowRowX = 16;
  t->ccForLowRowY = 17;
  t->ccForLowRowZ = 18;
  t->transposeOctave = s->transposeOctave;
  t->transposePitch = s->transposePitch;
  t->transposeLights = s->transposeLights;
  t->ccFaders = s->ccFaders;
  t->arpeggiator = s->arpeggiator;
  t->strum = s->strum;
  t->mpe = false;
}

void copyPresetSettingsOfConfigurationV2(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV2* s = (ConfigurationV2*)source;

  for (byte p = 0; p < NUMPRESETS; ++p) {
    t->preset[p].global.splitPoint = s->preset[p].global.splitPoint;
    t->preset[p].global.currentPerSplit = s->preset[p].global.currentPerSplit;
    copyGlobalSettingsNoteLights(&t->preset[p].global, s->preset[p].global.mainNotes, s->preset[p].global.accentNotes);
    t->preset[p].global.rowOffset = s->preset[p].global.rowOffset;
    t->preset[p].global.velocitySensitivity = s->preset[p].global.velocitySensitivity;
    t->preset[p].global.minForVelocity = 0;
    t->preset[p].global.maxForVelocity = DEFAULT_MAX_VELOCITY;
    t->preset[p].global.valueForFixedVelocity = DEFAULT_FIXED_VELOCITY;
    t->preset[p].global.pressureSensitivity = s->preset[p].global.pressureSensitivity;
    t->preset[p].global.pressureAftertouch = false;
    memcpy(t->preset[p].global.switchAssignment, s->preset[p].global.switchAssignment, sizeof(byte)*4);
    memcpy(t->preset[p].global.switchBothSplits, s->preset[p].global.switchBothSplits, sizeof(boolean)*4);
    t->preset[p].global.ccForSwitch = 65;
    t->preset[p].global.midiIO = s->preset[p].global.midiIO;
    t->preset[p].global.arpDirection = s->preset[p].global.arpDirection;
    t->preset[p].global.arpTempo = s->preset[p].global.arpTempo;
    t->preset[p].global.arpOctave = s->preset[p].global.arpOctave;
    t->preset[p].global.sustainBehavior = sustainHold;

    copySplitSettingsV2(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copyConfigurationV3(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV3* s = (ConfigurationV3*)source;

  t->device.version = s->device.version;
  copyCalibration(&(t->device.calRows), &(s->device.calRows), &(t->device.calCols), &(s->device.calCols));
  t->device.calibrated = s->device.calibrated;
  t->device.sensorLoZ = DEFAULT_SENSOR_LO_Z;
  t->device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  t->device.sensorRangeZ = s->device.sensorRangeZ;
  t->device.promoAnimation = s->device.promoAnimationAtStartup;
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  copyAudienceMessages(&(t->device.audienceMessages), &(s->device.audienceMessages));
  t->device.leftHanded = false;

  for (byte p = 0; p < NUMPRESETS; ++p) {
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
  t->minForVelocity = 0;
  t->maxForVelocity = DEFAULT_MAX_VELOCITY;
  t->valueForFixedVelocity = DEFAULT_FIXED_VELOCITY;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitch = 65;
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = sustainHold;
}

void copyPresetSettingsOfConfigurationV3(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV3* s = (ConfigurationV3*)source;

  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyGlobalSettingsV3(&t->preset[p].global, &s->preset[p].global);

    copySplitSettingsV2(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copyConfigurationV4(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV4* s = (ConfigurationV4*)source;

  copyDeviceSettingsV4(&t->device, &s->device);

  copyPresetSettingsV3(&t->settings, &s->settings);
  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV3(&t->preset[p], &s->preset[p]);
  }
}

void copyDeviceSettingsV4(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV4* s = (DeviceSettingsV4*)source;

  t->version = s->version;
  t->serialMode = true;
  copyCalibration(&(t->calRows), &(s->calRows), &(t->calCols), &(s->calCols));
  t->calibrated = s->calibrated;
  t->minUSBMIDIInterval = DEFAULT_MIN_USB_MIDI_INTERVAL;
  t->sensorLoZ = DEFAULT_SENSOR_LO_Z;
  t->sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  t->sensorRangeZ = s->sensorRangeZ;
  t->promoAnimation = s->promoAnimationAtStartup;
  copyAudienceMessages(&(t->audienceMessages), &(s->audienceMessages));
  t->operatingLowPower = false;
  t->leftHanded = false;
}

void copyPresetSettingsV3(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV3* s = (PresetSettingsV3*)source;

  copyGlobalSettingsV3(&t->global, &s->global);

  copySplitSettingsV2(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV2(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyConfigurationV5(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV5* s = (ConfigurationV5*)source;

  copyDeviceSettingsV4(&t->device, &s->device);

  copyPresetSettingsV4(&t->settings, &s->settings);
  for (byte p = 0; p < NUMPRESETS; ++p) {
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
  t->minForVelocity = 0;
  t->maxForVelocity = DEFAULT_MAX_VELOCITY;
  t->valueForFixedVelocity = DEFAULT_FIXED_VELOCITY;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitch = s->ccForSwitch;
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
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*8);
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
  t->minForY = 0;
  t->maxForY = 127;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  t->customCCForZ = s->customCCForZ;
  t->minForZ = 0;
  t->maxForZ = 127;
  memcpy(t->ccForFader, s->ccForFader, sizeof(unsigned short)*8);
  t->colorMain = s->colorMain;
  t->colorAccent = s->colorAccent;
  t->colorNoteon = s->colorNoteon;
  t->colorLowRow = s->colorLowRow;
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

void copyConfigurationV6(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV6* s = (ConfigurationV6*)source;

  copyDeviceSettingsV4(&t->device, &s->device);

  copyPresetSettingsV5(&t->settings, &s->settings);
  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV5(&t->preset[p], &s->preset[p]);
  }
}

void copyPresetSettingsV5(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV5* s = (PresetSettingsV5*)source;

  copyGlobalSettingsV5(&t->global, &s->global);

  t->split[LEFT] = s->split[LEFT];
  t->split[RIGHT] = s->split[RIGHT];
}

void copyGlobalSettingsV5(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV5* s = (GlobalSettingsV5*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  memcpy(t->mainNotes, s->mainNotes, sizeof(int)*12);
  memcpy(t->accentNotes, s->accentNotes, sizeof(int)*12);
  t->rowOffset = s->rowOffset;
  t->velocitySensitivity = s->velocitySensitivity;
  t->minForVelocity = 0;
  t->maxForVelocity = DEFAULT_MAX_VELOCITY;
  t->valueForFixedVelocity = DEFAULT_FIXED_VELOCITY;
  t->pressureSensitivity = s->pressureSensitivity;
  t->pressureAftertouch = s->pressureAftertouch;
  memcpy(t->switchAssignment, s->switchAssignment, sizeof(byte)*4);
  memcpy(t->switchBothSplits, s->switchBothSplits, sizeof(boolean)*4);
  t->ccForSwitch = s->ccForSwitch;
  t->midiIO = s->midiIO;
  t->arpDirection = s->arpDirection;
  t->arpTempo = s->arpTempo;
  t->arpOctave = s->arpOctave;
  t->sustainBehavior = s->sustainBehavior;
}