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
struct ConfigurationV1 configV1;
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
struct ConfigurationV2 configV2;
/**************************************** Configuration V3 ***************************************/
/* This is used by firmware v1.1.1-beta1, v1.1.1-beta2 and v1.1.1-beta3
/*************************************************************************************************/
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
  GlobalSettings global;
  SplitSettingsV2 split[NUMSPLITS];
};
struct ConfigurationV3 {
  DeviceSettingsV3 device;
  PresetSettingsV3 preset[NUMPRESETS];
};
struct ConfigurationV3 configV3;
/**************************************** Configuration V4 ***************************************/
/* This is used by firmware v1.1.1-beta4, v1.1.2-beta1, v1.1.2 and v1.2.0-alpha1
/*************************************************************************************************/
struct ConfigurationV4 {
  DeviceSettings device;
  PresetSettingsV3 settings;
  PresetSettingsV3 preset[NUMPRESETS];
};
struct ConfigurationV4 configV4;
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

        boolean settingsApplied = false;
        byte settingsVersion = buff2[0];
        // if the stored version is newer than what this firmware supports, resort to default settings
        if (settingsVersion > config.device.version) {
          settingsApplied = false;
        }
        // if this is v1 of the configuration format, load it in the old structure and then convert it if the size is right
        else if (settingsVersion == 1 && confSize == sizeof(ConfigurationV1)) {
          memcpy(&configV1, buff2, confSize);

          byte currentVersion = config.device.version;
          copySettingsV1ToSettingsV5(&config, &configV1);
          config.device.version = currentVersion;
          settingsApplied = true;
        }
        // this is the v2 of the configuration configuration, apply it if the size is right
        else if (settingsVersion == 2 && confSize == sizeof(ConfigurationV2)) {
          memcpy(&configV2, buff2, confSize);

          byte currentVersion = config.device.version;
          copySettingsV2ToSettingsV5(&config, &configV2);
          config.device.version = currentVersion;
          settingsApplied = true;
        }
        // this is the v3 of the configuration configuration, apply it if the size is right
        else if (settingsVersion == 3 && confSize == sizeof(ConfigurationV3)) {
          memcpy(&configV3, buff2, confSize);

          byte currentVersion = config.device.version;
          copySettingsV3ToSettingsV5(&config, &configV3);
          config.device.version = currentVersion;
          settingsApplied = true;
        }
        // this is the v4 of the configuration configuration, apply it if the size is right
        else if (settingsVersion == 4 && confSize == sizeof(ConfigurationV4)) {
          memcpy(&configV4, buff2, confSize);

          byte currentVersion = config.device.version;
          copySettingsV4ToSettingsV5(&config, &configV4);
          config.device.version = currentVersion;
          settingsApplied = true;
        }
        // this is the v5 of the configuration configuration, apply it if the size is right
        else if (settingsVersion == 5 && confSize == sizeof(Configuration)) {
          memcpy(&config, buff2, confSize);
          settingsApplied = true;
        }

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

void copySettingsV1ToSettingsV5(void *target, void *source) {
  Configuration *t = (Configuration *)target;
  ConfigurationV1 *s = (ConfigurationV1 *)source;
  GlobalSettingsV1 *g = &(s->global);

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
  initializeAudienceMessages();

  for (byte p = 0; p < NUMPRESETS; ++p) {
    t->preset[p].global.splitPoint = g->splitPoint;
    t->preset[p].global.currentPerSplit = g->currentPerSplit;
    memcpy(t->preset[p].global.mainNotes, g->mainNotes, sizeof(boolean)*12);
    memcpy(t->preset[p].global.accentNotes, g->accentNotes, sizeof(boolean)*12);
    t->preset[p].global.rowOffset = g->rowOffset;
    t->preset[p].global.velocitySensitivity = g->velocitySensitivity;
    t->preset[p].global.pressureSensitivity = g->pressureSensitivity;
    t->preset[p].global.pressureAftertouch = false;
    memcpy(t->preset[p].global.switchAssignment, g->switchAssignment, sizeof(byte)*4);
    memcpy(t->preset[p].global.switchBothSplits, g->switchBothSplits, sizeof(boolean)*4);
    t->preset[p].global.midiIO = g->midiIO;
    t->preset[p].global.arpDirection = g->arpDirection;
    t->preset[p].global.arpTempo = g->arpTempo;
    t->preset[p].global.arpOctave = g->arpOctave;

    copySplitSettingsV1ToSplitSettingsV5(&t->preset[p].split[LEFT], &s->left);
    copySplitSettingsV1ToSplitSettingsV5(&t->preset[p].split[RIGHT], &s->right);
  }
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySplitSettingsV1ToSplitSettingsV5(void *target, void *source) {
  SplitSettings *t = (SplitSettings *)target;
  SplitSettingsV1 *s = (SplitSettingsV1 *)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*8);
  t->bendRange = s->bendRange;
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  t->expressionForY = timbreCC;
  t->ccForY = s->ccForY;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  t->ccForZ = s->ccForZ;
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
}

void copySettingsV2ToSettingsV5(void *target, void *source) {
  Configuration *t = (Configuration *)target;
  ConfigurationV2 *s = (ConfigurationV2 *)source;

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
  initializeAudienceMessages();

  copyPresetSettingsV2ToSettingsV5(t, s);
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySplitSettingsV2ToSplitSettingsV5(void *target, void *source) {
  SplitSettings *t = (SplitSettings *)target;
  SplitSettingsV2 *s = (SplitSettingsV2 *)source;

  t->midiMode = s->midiMode;
  t->midiChanMain = s->midiChanMain;
  t->midiChanPerRow = s->midiChanPerRow;
  memcpy(t->midiChanSet, s->midiChanSet, sizeof(boolean)*8);
  t->bendRange = s->bendRange;
  t->sendX = s->sendX;
  t->sendY = s->sendY;
  t->sendZ = s->sendZ;
  t->pitchCorrectQuantize = s->pitchCorrectQuantize;
  t->pitchCorrectHold = s->pitchCorrectHold;
  t->pitchResetOnRelease = s->pitchResetOnRelease;
  t->expressionForY = s->expressionForY;
  t->ccForY = s->ccForY;
  t->relativeY = s->relativeY;
  t->expressionForZ = s->expressionForZ;
  t->ccForZ = s->ccForZ;
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

void copyPresetSettingsV2ToSettingsV5(void *target, void *source) {
  Configuration *t = (Configuration *)target;
  ConfigurationV2 *s = (ConfigurationV2 *)source;

  for (byte p = 0; p < NUMPRESETS; ++p) {
    t->preset[p].global.splitPoint = s->preset[p].global.splitPoint;
    t->preset[p].global.currentPerSplit = s->preset[p].global.currentPerSplit;
    memcpy(t->preset[p].global.mainNotes, s->preset[p].global.mainNotes, sizeof(boolean)*12);
    memcpy(t->preset[p].global.accentNotes, s->preset[p].global.accentNotes, sizeof(boolean)*12);
    t->preset[p].global.rowOffset = s->preset[p].global.rowOffset;
    t->preset[p].global.velocitySensitivity = s->preset[p].global.velocitySensitivity;
    t->preset[p].global.pressureSensitivity = s->preset[p].global.pressureSensitivity;
    t->preset[p].global.pressureAftertouch = false;
    memcpy(t->preset[p].global.switchAssignment, s->preset[p].global.switchAssignment, sizeof(byte)*4);
    memcpy(t->preset[p].global.switchBothSplits, s->preset[p].global.switchBothSplits, sizeof(boolean)*4);
    t->preset[p].global.midiIO = s->preset[p].global.midiIO;
    t->preset[p].global.arpDirection = s->preset[p].global.arpDirection;
    t->preset[p].global.arpTempo = s->preset[p].global.arpTempo;
    t->preset[p].global.arpOctave = s->preset[p].global.arpOctave;

    copySplitSettingsV2ToSplitSettingsV5(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2ToSplitSettingsV5(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySettingsV3ToSettingsV5(void *target, void *source) {
  Configuration *t = (Configuration *)target;
  ConfigurationV3 *s = (ConfigurationV3 *)source;

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

  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV3ToSettingsV5(t, s);
  }
  
  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copyPresetSettingsV3ToSettingsV5(void *target, void *source) {
  Configuration *t = (Configuration *)target;
  ConfigurationV3 *s = (ConfigurationV3 *)source;

  for (byte p = 0; p < NUMPRESETS; ++p) {
    memcpy(&t->preset[p].global, &s->preset[p].global, sizeof(GlobalSettings));

    copySplitSettingsV2ToSplitSettingsV5(&t->preset[p].split[LEFT], &s->preset[p].split[LEFT]);
    copySplitSettingsV2ToSplitSettingsV5(&t->preset[p].split[RIGHT], &s->preset[p].split[RIGHT]);
  }

  // we're adding a current settings preset, which we're initializing with preset 0
  memcpy(&t->settings, &t->preset[0], sizeof(PresetSettings));
}

void copySettingsV4ToSettingsV5(void *target, void *source) {
  Configuration *t = (Configuration *)target;
  ConfigurationV4 *s = (ConfigurationV4 *)source;

  t->device = s->device;
  t->device.serialMode = true;
  t->device.operatingLowPower = false;

  copyPresetSettingsV4ToSettingsV5(&t->settings, &s->settings);
  for (byte p = 0; p < NUMPRESETS; ++p) {
    copyPresetSettingsV4ToSettingsV5(&t->preset[p], &s->preset[p]);
  }
}

void copyPresetSettingsV4ToSettingsV5(void *target, void *source) {
  PresetSettings *t = (PresetSettings *)target;
  PresetSettingsV3 *s = (PresetSettingsV3 *)source;

  memcpy(&t->global, &s->global, sizeof(GlobalSettings));

  copySplitSettingsV2ToSplitSettingsV5(&t->split[LEFT], &s->split[LEFT]);
  copySplitSettingsV2ToSplitSettingsV5(&t->split[RIGHT], &s->split[RIGHT]);
}
