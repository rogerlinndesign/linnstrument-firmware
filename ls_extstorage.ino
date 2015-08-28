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

// Handshake codes for settings transfer
const char* countDownCode = "5, 4, 3, 2, 1 ...\n";
const byte countDownLength = 18;
const char* linnGoCode = "LinnStruments are go!\n"; 
const char* ackCode = "ACK\n";

boolean waitingForCommands = false;

enum linnCommands {
  SendSettings = 's',
  RestoreSettings = 'r'
};

byte codePos = 0;

boolean upgradeConfigurationSettings(int32_t confSize, byte* buff2) {
  boolean result = false;

  byte settingsVersion = buff2[0];

  // if the stored version is newer than what this firmware supports, resort to default settings
  if (settingsVersion > config.device.version) {
    result = false;
  }
  else {
    void* targetConfig = NULL;
    void (*copySettingsFunction)(void* target, void* source) = NULL;

    switch (settingsVersion) {
      // if this is v1 of the configuration format, load it in the old structure and then convert it if the size is right
      case 1:
        if (confSize == sizeof(ConfigurationV1)) {
          targetConfig = new ConfigurationV1();
          copySettingsFunction = &copySettingsV1ToSettingsV7;
        }
        break;
      // this is the v2 of the configuration configuration, apply it if the size is right
      case 2:
        if (confSize == sizeof(ConfigurationV2)) {
          targetConfig = new ConfigurationV2();
          copySettingsFunction = &copySettingsV2ToSettingsV7;
        }
        break;
      // this is the v3 of the configuration configuration, apply it if the size is right
      case 3:
        if (confSize == sizeof(ConfigurationV3)) {
          targetConfig = new ConfigurationV3();
          copySettingsFunction = &copySettingsV3ToSettingsV7;
        }
        break;
      // this is the v4 of the configuration configuration, apply it if the size is right
      case 4:
        if (confSize == sizeof(ConfigurationV4)) {
          targetConfig = new ConfigurationV4();
          copySettingsFunction = &copySettingsV4ToSettingsV7;
        }
        break;
      // this is the v5 of the configuration configuration, apply it if the size is right
      case 5:
        if (confSize == sizeof(ConfigurationV5)) {
          targetConfig = new ConfigurationV5();
          copySettingsFunction = &copySettingsV5ToSettingsV7;
        }
        break;
      // this is the v6 of the configuration configuration, apply it if the size is right
      case 6:
        if (confSize == sizeof(ConfigurationV6)) {
          targetConfig = new ConfigurationV5();
          copySettingsFunction = &copySettingsV6ToSettingsV7;
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
    if (targetConfig && copySettingsFunction) {
      memcpy(targetConfig, buff2, confSize);

      byte currentVersion = config.device.version;
      copySettingsFunction(&config, targetConfig);
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

void handleExtStorage() {
  // if no serial data is available, return
  if (Serial.available() <= 0) {
    return;
  }

  // get the next byte from the serial bus
  byte d = Serial.read();

  // check for a recognized command
  if (waitingForCommands) {
    switch (d) {
      case SendSettings:
      {
        Serial.write(ackCode);

        clearDisplay();
        delayUsec(1000);

        saveSettings();

        int32_t confSize = sizeof(Configuration);

        // send the size of the settings
        byte buff1[sizeof(int32_t)];
        memcpy(buff1, &confSize, sizeof(int32_t));
        Serial.write(buff1, sizeof(int32_t));

        // send the actual settings
        byte buff2[confSize];
        memcpy(buff2, &config, confSize);
        Serial.write(buff2, confSize);

        Serial.write(ackCode);

        break;
      }

      case RestoreSettings:
      {
        Serial.write(ackCode);

        clearDisplay();
        delayUsec(1000);

        // retrieve the size of the settings
        uint32_t lastMoment = millis();

        byte buff1[sizeof(int32_t)];
        for (byte i = 0; i < 4; ++i) {
          // retry if there's no data available
          while (Serial.available() <= 0) {
            // timeout after 2s if no data is coming in anymore
            if (calcTimeDelta(millis(), lastMoment) > 2000) {
              waitingForCommands = false;
              return;
            }
          }

          // read the next byte of the configuration size
          buff1[i] = Serial.read();
          lastMoment = millis();
        }

        int32_t confSize;
        memcpy(&confSize, buff1, sizeof(int32_t));

        // retrieve the actual settings
        lastMoment = millis();
        byte buff2[confSize];
        for (unsigned j = 0; j < confSize; ++j) {
          if (j % 32 == 0) {
            Serial.write(ackCode);
          }
          
          // retry if there's no data available
          while (Serial.available() <= 0) {
            // timeout after 2s if no data is coming in anymore
            if (calcTimeDelta(millis(), lastMoment) > 2000) {
              waitingForCommands = false;
              return;
            }
          }

          // read the next byte of the configuration data
          buff2[j] = Serial.read();
          lastMoment = millis();
        }

        boolean settingsApplied = upgradeConfigurationSettings(confSize, buff2);

        // activate the retrieved settings
        if (settingsApplied) {
          applyConfiguration();
        }

        // send the acknowledgement of success
        Serial.write(ackCode);
        delayUsec(1000000);

        // Turn off OS upgrade mode
        switchSerialMode(false);

        // Enable normal playing mode and ensure calibration is fully turned off
        if (settingsApplied) {
          setDisplayMode(displayNormal);
          controlButton = -1;
          clearLed(0, GLOBAL_SETTINGS_ROW);

          storeSettings();
        }
        // turn on calibration instead of no new settings were applied and default settings are used
        else {
          setDisplayMode(displayCalibration);
          controlButton = 0;
          lightLed(0, GLOBAL_SETTINGS_ROW);
        }

        updateDisplay();
        waitingForCommands = false;

        break;
      }

      default:
      {
        waitingForCommands = false;
        break;
      }
    }
  }
  // handle readyness countdown state
  else {
    if (d == countDownCode[codePos]) {
      codePos++;
      if (codePos == countDownLength) {
        codePos = 0;
        waitingForCommands = true;
        Serial.write(linnGoCode);
      }
    }
    else {
      codePos = 0;
    }
  }
}

void copySettingsV1ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV1* s = (ConfigurationV1*)source;
  GlobalSettingsV1* g = &(s->global);

  t->device.version = g->version;
  memcpy(t->device.calRows, g->calRows, sizeof(CalibrationX)*((NUMCOLS+1) * 4));
  memcpy(t->device.calCols, g->calCols, sizeof(CalibrationY)*(9 * NUMROWS));
  t->device.calibrated = g->calibrated;
  t->device.sensorLoZ = g->sensorLoZ;
  t->device.sensorFeatherZ = g->sensorFeatherZ;
  t->device.sensorRangeZ = g->sensorRangeZ;
  t->device.promoAnimationAtStartup = g->promoAnimationAtStartup;
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  t->device.leftHanded = false;
  initializeAudienceMessages();

  for (byte p = 0; p < NUMPRESETS; ++p) {
    t->preset[p].global.splitPoint = g->splitPoint;
    t->preset[p].global.currentPerSplit = g->currentPerSplit;
    copyGlobalSettingsNoteLightsToSettingsV7(&t->preset[p].global, g->mainNotes, g->accentNotes);
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

    copySplitSettingsV1ToSplitSettingsV7(&t->preset[p].split[LEFT], &s->left);
    copySplitSettingsV1ToSplitSettingsV7(&t->preset[p].split[RIGHT], &s->right);
  }
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySplitSettingsV1ToSplitSettingsV7(void* target, void* source) {
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

void copySettingsV2ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV2* s = (ConfigurationV2*)source;

  t->device.version = s->device.version;
  memcpy(t->device.calRows, s->device.calRows, sizeof(CalibrationX)*((NUMCOLS+1) * 4));
  memcpy(t->device.calCols, s->device.calCols, sizeof(CalibrationY)*(9 * NUMROWS));
  t->device.calibrated = s->device.calibrated;
  t->device.sensorLoZ = s->device.sensorLoZ;
  t->device.sensorFeatherZ = s->device.sensorFeatherZ;
  if (t->device.sensorFeatherZ == 111) {
    t->device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  }
  t->device.sensorRangeZ = s->device.sensorRangeZ;
  t->device.promoAnimationAtStartup = s->device.promoAnimationAtStartup;
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  t->device.leftHanded = false;
  initializeAudienceMessages();

  copyPresetSettingsV2ToSettingsV7(t, s);
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySplitSettingsV2ToSplitSettingsV7(void* target, void* source) {
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

void copyPresetSettingsV2ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV2* s = (ConfigurationV2*)source;

  for (byte p = 0; p < NUMPRESETS; ++p) {
    t->preset[p].global.splitPoint = s->preset[p].global.splitPoint;
    t->preset[p].global.currentPerSplit = s->preset[p].global.currentPerSplit;
    copyGlobalSettingsNoteLightsToSettingsV7(&t->preset[p].global, s->preset[p].global.mainNotes, s->preset[p].global.accentNotes);
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

    copySplitSettingsV2ToSplitSettingsV7(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2ToSplitSettingsV7(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySettingsV3ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV3* s = (ConfigurationV3*)source;

  t->device.version = s->device.version;
  memcpy(t->device.calRows, s->device.calRows, sizeof(CalibrationX)*((NUMCOLS+1) * 4));
  memcpy(t->device.calCols, s->device.calCols, sizeof(CalibrationY)*(9 * NUMROWS));
  t->device.calibrated = s->device.calibrated;
  t->device.sensorLoZ = s->device.sensorLoZ;
  t->device.sensorFeatherZ = s->device.sensorFeatherZ;
  if (t->device.sensorFeatherZ == 111) {
    t->device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  }
  t->device.sensorRangeZ = s->device.sensorRangeZ;
  t->device.promoAnimationAtStartup = s->device.promoAnimationAtStartup;
  t->device.serialMode = true;
  t->device.operatingLowPower = false;
  memcpy(t->device.audienceMessages, s->device.audienceMessages, sizeof(char)*(16 * 31));
  t->device.leftHanded = false;

  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV3ToSettingsV7(t, s);
  }
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copyGlobalSettingsV3ToSettingsV7(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV3* s = (GlobalSettingsV3*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  copyGlobalSettingsNoteLightsToSettingsV7(t, s->mainNotes, s->accentNotes);
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

void copyPresetSettingsV3ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV3* s = (ConfigurationV3*)source;

  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyGlobalSettingsV3ToSettingsV7(&t->preset[p].global, &s->preset[p].global);

    copySplitSettingsV2ToSplitSettingsV7(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2ToSplitSettingsV7(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySettingsV4ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV4* s = (ConfigurationV4*)source;

  copyDeviceSettingsV4ToSettingsV7(&t->device, &s->device);

  copyPresetSettingsV4ToSettingsV7(&t->settings, &s->settings);
  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV4ToSettingsV7(&t->preset[p], &s->preset[p]);
  }
}

void copyDeviceSettingsV4ToSettingsV7(void* target, void* source) {
  DeviceSettings* t = (DeviceSettings*)target;
  DeviceSettingsV4* s = (DeviceSettingsV4*)source;

  t->version = s->version;
  t->serialMode = true;
  memcpy(t->calRows, s->calRows, sizeof(CalibrationX)*((NUMCOLS+1) * 4));
  memcpy(t->calCols, s->calCols, sizeof(CalibrationY)*(9 * NUMROWS));
  t->calibrated = s->calibrated;
  t->MinUSBMIDIInterval = DEFAULT_MIN_USB_MIDI_INTERVAL;
  t->sensorLoZ = s->sensorLoZ;
  t->sensorFeatherZ = s->sensorFeatherZ;
  t->sensorRangeZ = s->sensorRangeZ;
  t->promoAnimationAtStartup = s->promoAnimationAtStartup;

  for (byte msg = 0; msg < 16; ++msg) {
    memset(t->audienceMessages[msg], '\0', sizeof(t->audienceMessages[msg]));
    strncpy(t->audienceMessages[msg], s->audienceMessages[msg], 30);
    t->audienceMessages[msg][30] = '\0';
  }

  t->operatingLowPower = false;
  t->leftHanded = false;
}

void copyPresetSettingsV4ToSettingsV7(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV3* s = (PresetSettingsV3*)source;

  copyGlobalSettingsV3ToSettingsV7(&t->global, &s->global);

  copySplitSettingsV2ToSplitSettingsV7(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV2ToSplitSettingsV7(&t->split[RIGHT], &s->split[RIGHT]);
}

void copySettingsV5ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV5* s = (ConfigurationV5*)source;

  copyDeviceSettingsV4ToSettingsV7(&t->device, &s->device);

  copyPresetSettingsV5ToSettingsV7(&t->settings, &s->settings);
  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV5ToSettingsV7(&t->preset[p], &s->preset[p]);
  }
}

void copyPresetSettingsV5ToSettingsV7(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV4* s = (PresetSettingsV4*)source;

  copyGlobalSettingsV4ToSettingsV7(&t->global, &s->global);

  copySplitSettingsV3ToSplitSettingsV7(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV3ToSplitSettingsV7(&t->split[RIGHT], &s->split[RIGHT]);
}

void copyGlobalSettingsNoteLightsToSettingsV7(void* target, boolean* sourceMainNotes, boolean* sourceAccentNotes) {
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

void copyGlobalSettingsV4ToSettingsV7(void* target, void* source) {
  GlobalSettings* t = (GlobalSettings*)target;
  GlobalSettingsV4* s = (GlobalSettingsV4*)source;

  t->splitPoint = s->splitPoint;
  t->currentPerSplit = s->currentPerSplit;
  copyGlobalSettingsNoteLightsToSettingsV7(t, s->mainNotes, s->accentNotes);
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

void copySplitSettingsV3ToSplitSettingsV7(void* target, void* source) {
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

void copySettingsV6ToSettingsV7(void* target, void* source) {
  Configuration* t = (Configuration*)target;
  ConfigurationV5* s = (ConfigurationV5*)source;

  copyDeviceSettingsV4ToSettingsV7(&t->device, &s->device);

  copyPresetSettingsV6ToSettingsV7(&t->settings, &s->settings);
  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV6ToSettingsV7(&t->preset[p], &s->preset[p]);
  }
}

void copyPresetSettingsV6ToSettingsV7(void* target, void* source) {
  PresetSettings* t = (PresetSettings*)target;
  PresetSettingsV5* s = (PresetSettingsV5*)source;

  copyGlobalSettingsV5ToSettingsV7(&t->global, &s->global);

  t->split[LEFT] = s->split[LEFT];
  t->split[RIGHT] = s->split[RIGHT];
}

void copyGlobalSettingsV5ToSettingsV7(void* target, void* source) {
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