/****************************** ls_settings: LinnStrument Settings ********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions handle the changing of any of LinnStrument's panel settings.
**************************************************************************************************/

// These messages correspond to the scrolling texts that will be displayed by default when pressing
// the top-most row in global settings. Only the first 30 characters will be used.
char* defaultAudienceMessages[16] = {
  "LINNSTRUMENT",
  "APPLAUSE",
  "HA HA HA",
  "SINGER SUCKS",
  "WRONG NOTE",
  "SMELLY NIGHTCLUB",
  "HELLO",
  "HELLO NEW YORK",
  "HELLO LOS ANGELES",
  "HELLO SAN FRANCISCO",
  "HELLO LONDON",
  "HELLO MUNICH",
  "HELLO BRUSSELS",
  "HELLO PARIS",
  "HELLO TOKYO",
  "HELLO BARCELONA"
};

unsigned long tempoChangeTime = 0;           // time of last touch for tempo change

void GlobalSettings::setSwitchAssignment(byte whichSwitch, byte assignment) {
  if (Global.switchAssignment[whichSwitch] != assignment) {
    resetSwitchStates(whichSwitch);
    Global.switchAssignment[whichSwitch] = assignment;
  }
}

void switchSerialMode(boolean flag) {
  if (controlModeActive) {
    controlModeActive = false;
    clearDisplay();
    updateDisplay();
  }
  
  Device.serialMode = flag;
  applySerialMode();
}

void applySerialMode() {
  if (Device.serialMode) {
    digitalWrite(35, HIGH);
  }
  else {
    digitalWrite(35, LOW);
  }
}

void initializeStorage() {
  byte bootblock = dueFlashStorage.read(0);

  if (bootblock != 0) {                                   // See if we need to boot from scratch
    if (bootblock == 255) {                               // When a new firmware is uploaded, the first flash byte will be 255
      switchSerialMode(true);                             // Start in serial mode after OS upgrade to be able to receive the settings
      config.device.serialMode = true;
      firstTimeBoot = true;
    }
    else {
      switchSerialMode(false);                            // Start in MIDI mode for all other bootblock values
      config.device.serialMode = false;
    }

    writeSettingsToFlash();                               // Store the initial default settings

    dueFlashStorage.write(0, 0);                          // Zero out the firstTime location.
    setDisplayMode(displayCalibration);                   // Automatically start calibration after firmware update.
    initializeCalibrationSamples();
    
    setLed(0, GLOBAL_SETTINGS_ROW, globalColor, cellOn);
    controlButton = GLOBAL_SETTINGS_ROW;
  } else {
    loadSettings();                                       // On subsequent startups, load settings from Flash
  }

  applyConfiguration();
}

void storeSettings() {
  saveSettings();
  writeSettingsToFlash();
}

void saveSettings() {
  config.device = Device;
  config.settings.global = Global;
  config.settings.split[LEFT] = Split[LEFT];
  config.settings.split[RIGHT] = Split[RIGHT];
}

void writeSettingsToFlash() {
  DEBUGPRINT((2,"storeSettings flash size="));
  DEBUGPRINT((2,sizeof(Configuration)));
  DEBUGPRINT((2," bytes"));
  DEBUGPRINT((2,"\n"));

  unsigned long zeromarker = 0;
  unsigned long now = millis();

  // batch and slow down the flash storage in low power mode
  if (Device.operatingLowPower) {

    clearDisplay();

    // ensure that there's at least 200 milliseconds between refreshing the display lights and writing to flash
    unsigned long displayModeDelta = calcTimeDelta(now, displayModeStart);
    if (displayModeDelta < 200) {
      delayUsec((200 - displayModeDelta) * 1000);
    }

    // clear out the second timestamp that is written after the configuration
    dueFlashStorage.write(4+sizeof(unsigned long)+sizeof(Configuration), (byte*)&zeromarker, sizeof(unsigned long));

    // write the timestamp before starting to write the configuration data
    dueFlashStorage.write(4, (byte*)&now, sizeof(unsigned long));
    delayUsec(1000);

    // write the configuration data
    uint32_t offset = 4+sizeof(unsigned long);

    byte batchsize = 64;
    byte* source = (byte*)&config;
    int total = sizeof(Configuration);
    int i = 0;
    while (i+batchsize < total) {
      dueFlashStorage.write(offset+i, source+i, batchsize);
      i += batchsize;
      delayUsec(500);
    }

    int remaining = total - i;
    if (remaining > 0) {
      dueFlashStorage.write(offset+i, source+i, remaining);
    }
    delayUsec(500);

    // write the timestamp after the configuration data for verification
    dueFlashStorage.write(offset+sizeof(Configuration), (byte*)&now, sizeof(unsigned long));
    delayUsec(1000);

    updateDisplay();
  }
  // do the faster possible flash storage in regular power mode
  else {
    byte b2[sizeof(Configuration)];
    memcpy(b2, &config, sizeof(Configuration));

    // clear out the second timestamp that is written after the configuration
    dueFlashStorage.write(4+sizeof(unsigned long)+sizeof(Configuration), (byte*)&zeromarker, sizeof(unsigned long));
    // write the timestamp before starting to write the configuration data
    dueFlashStorage.write(4, (byte*)&now, sizeof(unsigned long));
    // write the configuration data
    dueFlashStorage.write(4+sizeof(unsigned long), b2, sizeof(Configuration));
    // write the timestamp after the configuration data for verification
    dueFlashStorage.write(4+sizeof(unsigned long)+sizeof(Configuration), (byte*)&now, sizeof(unsigned long));
  }
}

void loadSettings() {
  // read both of the timestamps that were stored to make sure they are identical
  // this allows the detection of corrupted settings storage due to power being removed while writing
  unsigned long now1 = 0;
  unsigned long now2 = 0;
  memcpy(&now1, dueFlashStorage.readAddress(4), sizeof(unsigned long));
  memcpy(&now2, dueFlashStorage.readAddress(4+sizeof(unsigned long)+sizeof(Configuration)), sizeof(unsigned long));

  // if both timestamps are the same, read the configuration that was stored
  if (now1 == now2) {
    memcpy(&config, dueFlashStorage.readAddress(4+sizeof(unsigned long)), sizeof(Configuration));
  }
  // otherwise notify the user of the corrupt state and perform a global reset
  else {
    clearDisplay();
    small_scroll_text("     SETTINGS RESET, SORRY ...", globalColor);

    dueFlashStorage.write(0, 253);
    initializeStorage();
  }
}

void applyPresetSettings(PresetSettings& preset) {
  memcpy(&Global, &preset.global, sizeof(GlobalSettings));
  memcpy(&Split[LEFT], &preset.split[LEFT], sizeof(SplitSettings));
  memcpy(&Split[RIGHT], &preset.split[RIGHT], sizeof(SplitSettings));

  focusedSplit = Global.currentPerSplit;
  applyPitchCorrectHold();
  applyLimitsForY();
  applyLimitsForZ();
  applyLimitsForVelocity();

  updateSplitMidiChannels(LEFT);
  updateSplitMidiChannels(RIGHT);

  applyMidiIo();
}

void applyConfiguration() {
  Device = config.device;
  applySerialMode();
  applyPresetSettings(config.settings);
}

void storeSettingsToPreset(byte p) {
  memcpy(&config.preset[p].global, &Global, sizeof(GlobalSettings));
  memcpy(&config.preset[p].split[LEFT], &Split[LEFT], sizeof(SplitSettings));
  memcpy(&config.preset[p].split[RIGHT], &Split[RIGHT], sizeof(SplitSettings));
}

// The first time after new code is loaded into the Linnstrument, this sets the initial defaults of all settings.
// On subsequent startups, these values are overwritten by loading the settings stored in flash.
void initializeDeviceSettings() {
  config.device.version = 7;
  config.device.serialMode = false;
  config.device.promoAnimation = false;
  config.device.operatingLowPower = false;
  config.device.leftHanded = false;
  config.device.minUSBMIDIInterval = DEFAULT_MIN_USB_MIDI_INTERVAL;

  initializeAudienceMessages();
}

void initializeAudienceMessages() {
  for (byte msg = 0; msg < 16; ++msg) {
    memset(config.device.audienceMessages[msg], '\0', sizeof(config.device.audienceMessages[msg]));
    strncpy(config.device.audienceMessages[msg], defaultAudienceMessages[msg], 30);
    config.device.audienceMessages[msg][30] = '\0';
  }
}

void initializeDeviceSensorSettings() {
  config.device.sensorLoZ = DEFAULT_SENSOR_LO_Z;
  config.device.sensorFeatherZ = DEFAULT_SENSOR_FEATHER_Z;
  config.device.sensorRangeZ = DEFAULT_SENSOR_RANGE_Z;
}

void initializeNoteLights(GlobalSettings& g) {
    g.activeNotes = 0;

    // initialize accentNotes array. Starting with only C within each octave highlighted
    for (byte count = 0; count < 12; ++count) {
      g.accentNotes[count] = 1;
    }

    // initialize mainNotes array (all off).
    for (byte count = 0; count < 12; ++count) {
      g.mainNotes[count] = 0;
    }

    // Major
    g.mainNotes[0] |= 1 << 0;
    g.mainNotes[0] |= 1 << 2;
    g.mainNotes[0] |= 1 << 4;
    g.mainNotes[0] |= 1 << 5;
    g.mainNotes[0] |= 1 << 7;
    g.mainNotes[0] |= 1 << 9;
    g.mainNotes[0] |= 1 << 11;

    // Natural minor
    g.mainNotes[1] |= 1 << 0;
    g.mainNotes[1] |= 1 << 2;
    g.mainNotes[1] |= 1 << 3;
    g.mainNotes[1] |= 1 << 5;
    g.mainNotes[1] |= 1 << 7;
    g.mainNotes[1] |= 1 << 8;
    g.mainNotes[1] |= 1 << 10;

    // Harmonic minor
    g.mainNotes[2] |= 1 << 0;
    g.mainNotes[2] |= 1 << 2;
    g.mainNotes[2] |= 1 << 3;
    g.mainNotes[2] |= 1 << 5;
    g.mainNotes[2] |= 1 << 7;
    g.mainNotes[2] |= 1 << 8;
    g.mainNotes[2] |= 1 << 11;

    // Major Pentatonic
    g.mainNotes[3] |= 1 << 0;
    g.mainNotes[3] |= 1 << 2;
    g.mainNotes[3] |= 1 << 4;
    g.mainNotes[3] |= 1 << 7;
    g.mainNotes[3] |= 1 << 9;

    // Minor Pentatonic
    g.mainNotes[4] |= 1 << 0;
    g.mainNotes[4] |= 1 << 3;
    g.mainNotes[4] |= 1 << 5;
    g.mainNotes[4] |= 1 << 7;
    g.mainNotes[4] |= 1 << 10;

    // Major Blues
    g.mainNotes[5] |= 1 << 0;
    g.mainNotes[5] |= 1 << 3;
    g.mainNotes[5] |= 1 << 4;
    g.mainNotes[5] |= 1 << 7;
    g.mainNotes[5] |= 1 << 9;
    g.mainNotes[5] |= 1 << 10;

    // Minor Blues
    g.mainNotes[6] |= 1 << 0;
    g.mainNotes[6] |= 1 << 3;
    g.mainNotes[6] |= 1 << 5;
    g.mainNotes[6] |= 1 << 6;
    g.mainNotes[6] |= 1 << 7;
    g.mainNotes[6] |= 1 << 10;

    // Diminished
    g.mainNotes[7] |= 1 << 0;
    g.mainNotes[7] |= 1 << 2;
    g.mainNotes[7] |= 1 << 3;
    g.mainNotes[7] |= 1 << 5;
    g.mainNotes[7] |= 1 << 6;
    g.mainNotes[7] |= 1 << 8;
    g.mainNotes[7] |= 1 << 9;
    g.mainNotes[7] |= 1 << 11;

    // Whole Tone
    g.mainNotes[8] |= 1 << 0;
    g.mainNotes[8] |= 1 << 2;
    g.mainNotes[8] |= 1 << 4;
    g.mainNotes[8] |= 1 << 6;
    g.mainNotes[8] |= 1 << 8;
    g.mainNotes[8] |= 1 << 10;

    // Spanish (Phrygian Dominant)
    g.mainNotes[9] |= 1 << 0;
    g.mainNotes[9] |= 1 << 1;
    g.mainNotes[9] |= 1 << 4;
    g.mainNotes[9] |= 1 << 5;
    g.mainNotes[9] |= 1 << 7;
    g.mainNotes[9] |= 1 << 8;
    g.mainNotes[9] |= 1 << 10;

    // Gypsy (Hungarian Minor)
    g.mainNotes[10] |= 1 << 0;
    g.mainNotes[10] |= 1 << 2;
    g.mainNotes[10] |= 1 << 3;
    g.mainNotes[10] |= 1 << 6;
    g.mainNotes[10] |= 1 << 7;
    g.mainNotes[10] |= 1 << 8;
    g.mainNotes[10] |= 1 << 10;

    // Arabic (Major Locrian)
    g.mainNotes[11] |= 1 << 0;
    g.mainNotes[11] |= 1 << 2;
    g.mainNotes[11] |= 1 << 4;
    g.mainNotes[11] |= 1 << 5;
    g.mainNotes[11] |= 1 << 6;
    g.mainNotes[11] |= 1 << 8;
    g.mainNotes[11] |= 1 << 10;
}

void initializePresetSettings() {
  splitActive = false;

  for (byte n = 0; n < NUMPRESETS; ++n) {
    presetBlinkStart[n] = 0;

    PresetSettings& p = config.preset[n];
    GlobalSettings& g = p.global;

    g.splitPoint = 12;
    g.currentPerSplit = LEFT;

    g.rowOffset = 5;
    g.velocitySensitivity = velocityMedium;
    g.minForVelocity = DEFAULT_MIN_VELOCITY;
    g.maxForVelocity = DEFAULT_MAX_VELOCITY;
    g.valueForFixedVelocity = DEFAULT_FIXED_VELOCITY;
    g.pressureSensitivity = pressureMedium;
    g.pressureAftertouch = false;
    g.midiIO = 1;      // set to 1 for USB jacks (not MIDI jacks)

    // initialize switch settings
    g.switchAssignment[SWITCH_FOOT_L] = ASSIGNED_ARPEGGIATOR;
    g.switchAssignment[SWITCH_FOOT_R] = ASSIGNED_SUSTAIN;
    g.switchAssignment[SWITCH_SWITCH_1] = ASSIGNED_SUSTAIN;
    g.switchAssignment[SWITCH_SWITCH_2] = ASSIGNED_ARPEGGIATOR;
    g.switchBothSplits[SWITCH_FOOT_L] = false;
    g.switchBothSplits[SWITCH_FOOT_R] = false;
    g.switchBothSplits[SWITCH_SWITCH_1] = false;
    g.switchBothSplits[SWITCH_SWITCH_2] = false;
    g.ccForSwitch = 65;

    initializeNoteLights(g);

    g.arpDirection = ArpUp;
    g.arpTempo = ArpSixteenth;
    g.arpOctave = 0;

    g.sustainBehavior = sustainHold;

    // initialize all identical values in the keyboard split data
    for (byte s = 0; s < NUMSPLITS; ++s) {
        for (byte chan = 0; chan < 16; ++chan) {
          focusCell[s][chan].col = 0;
          focusCell[s][chan].row = 0;
        }
        p.split[s].midiMode = oneChannel;
        p.split[s].bendRangeOption = bendRange2;
        p.split[s].customBendRange = 24;
        p.split[s].sendX = true;
        p.split[s].sendY = true;
        p.split[s].sendZ = true;
        p.split[s].pitchCorrectQuantize = true;
        p.split[s].pitchCorrectHold = true;
        p.split[s].pitchResetOnRelease = false;
        p.split[s].expressionForY = timbreCC74;
        p.split[s].minForY = 0;
        p.split[s].maxForY = 127;
        p.split[s].customCCForY = 74;
        p.split[s].relativeY = false;
        p.split[s].expressionForZ = loudnessPolyPressure;
        p.split[s].minForZ = 0;
        p.split[s].maxForZ = 127;
        p.split[s].customCCForZ = 11;
        memcpy(&p.split[s].ccForFader, ccFaderDefaults, sizeof(unsigned short)*8);
        p.split[s].colorAccent = COLOR_CYAN;
        p.split[s].colorLowRow = COLOR_YELLOW;
        p.split[s].lowRowCCXBehavior = lowRowCCHold;
        p.split[s].ccForLowRow = 1;
        p.split[s].lowRowCCXYZBehavior = lowRowCCHold;
        p.split[s].ccForLowRowX = 16;
        p.split[s].ccForLowRowY = 17;
        p.split[s].ccForLowRowZ = 18;
        p.split[s].transposeOctave = 0;
        p.split[s].transposePitch = 0;
        p.split[s].transposeLights = 0;
        p.split[s].arpeggiator = false;
        p.split[s].ccFaders = false;
        p.split[s].strum = false;
        p.split[s].mpe = false;
    }

    // initialize values that differ between the keyboard splits
    p.split[LEFT].midiChanMain = 1;
    p.split[LEFT].midiChanSet[0] = false;
    for (byte chan = 1; chan < 8; ++chan) {
      p.split[LEFT].midiChanSet[chan] = true;
    }
    for (byte chan = 8; chan < 16; ++chan) {
      p.split[LEFT].midiChanSet[chan] = false;
    }
    p.split[LEFT].midiChanPerRow = 1;
    p.split[LEFT].colorMain = COLOR_GREEN;
    p.split[LEFT].colorNoteon = COLOR_RED;
    p.split[LEFT].lowRowMode = lowRowNormal;

    p.split[RIGHT].midiChanMain = 15;
    for (byte chan = 0; chan < 8; ++chan) {
      p.split[RIGHT].midiChanSet[chan] = false;
    }
    for (byte chan = 8; chan < 15; ++chan) {
      p.split[RIGHT].midiChanSet[chan] = true;
    }
    p.split[RIGHT].midiChanSet[15] = false;
    p.split[RIGHT].midiChanPerRow = 9;
    p.split[RIGHT].colorMain = COLOR_BLUE;
    p.split[RIGHT].colorNoteon = COLOR_MAGENTA;
    p.split[RIGHT].lowRowMode = lowRowNormal;
  }

  // preset 0 is pre-programmed for one channel sounds from our Logic example file
  config.preset[0].split[LEFT].midiMode = oneChannel;
  config.preset[0].split[RIGHT].midiMode = oneChannel;
  config.preset[0].split[LEFT].bendRangeOption = bendRange12;
  config.preset[0].split[RIGHT].bendRangeOption = bendRange12;
  config.preset[0].split[LEFT].expressionForZ = loudnessPolyPressure;
  config.preset[0].split[RIGHT].expressionForZ = loudnessPolyPressure;

  // preset 1 is pre-programmed for channel per note sounds from our Logic example file
  config.preset[1].split[LEFT].midiMode = channelPerNote;
  config.preset[1].split[RIGHT].midiMode = channelPerNote;
  config.preset[1].split[LEFT].bendRangeOption = bendRange24;
  config.preset[1].split[RIGHT].bendRangeOption = bendRange24;
  config.preset[1].split[LEFT].expressionForZ = loudnessChannelPressure;
  config.preset[1].split[RIGHT].expressionForZ = loudnessChannelPressure;
  config.preset[1].split[LEFT].midiChanMain = 1;
  config.preset[1].split[LEFT].midiChanSet[0] = false;
  config.preset[1].split[RIGHT].midiChanMain = 16;
  config.preset[1].split[RIGHT].midiChanSet[15] = false;

  // we're initializing the current settings with preset 3
  memcpy(&config.settings, &config.preset[3], sizeof(PresetSettings));

  // initialize runtime data
  applyPitchCorrectHold();
  applyLimitsForY();
  applyLimitsForZ();
  applyLimitsForVelocity();
  for (byte s = 0; s < NUMSPLITS; ++s) {
    for (byte c = 0; c < 128; ++c) {
      ccFaderValues[s][c] = 0;
    }
    ccFaderValues[s][7] = 63;
    currentEditedCCFader[s] = 0;
    midiPreset[0] = 0;
    arpTempoDelta[s] = 0;
    splitChannels[s].clear();
  }
}

void applyPitchCorrectHold() {
  for (byte sp = 0; sp < NUMSPLITS; ++sp) {
    switch (Split[sp].pitchCorrectHold) {
      case pitchCorrectHoldOff:
      {
        pitchHoldDuration[sp] = PITCH_CORRECT_HOLD_SAMPLES_DEFAULT;
        fxdRateXThreshold[sp] = FXD_MAKE(RATEX_THRESHOLD_DEFAULT);
        break;
      }
      case pitchCorrectHoldFast:
      {
        pitchHoldDuration[sp] = PITCH_CORRECT_HOLD_SAMPLES_FAST;
        fxdRateXThreshold[sp] = FXD_MAKE(RATEX_THRESHOLD_FAST);
        break;
      }
      case pitchCorrectHoldMedium:
      {
        pitchHoldDuration[sp] = PITCH_CORRECT_HOLD_SAMPLES_MEDIUM;
        fxdRateXThreshold[sp] = FXD_MAKE(RATEX_THRESHOLD_MEDIUM);
        break;
      }
      case pitchCorrectHoldSlow:
      {
        pitchHoldDuration[sp] = PITCH_CORRECT_HOLD_SAMPLES_SLOW;
        fxdRateXThreshold[sp] = FXD_MAKE(RATEX_THRESHOLD_SLOW);
        break;
      }
    }

    fxdPitchHoldDuration[sp] = FXD_MAKE(pitchHoldDuration[sp]);
  }
}

void applyBendRange(SplitSettings& target, byte bendRange) {
  switch (bendRange) {
    case 2:
      target.bendRangeOption = bendRange2;
      break;
    case 3:
      target.bendRangeOption = bendRange3;
      break;
    case 12:
      target.bendRangeOption = bendRange12;
      break;
    default:
      target.bendRangeOption = bendRange24;
      target.customBendRange = bendRange;
      break;
  }  
}

void applyLimitsForY() {
  for (byte sp = 0; sp < NUMSPLITS; ++sp) {
    int32_t fxd_range = FXD_FROM_INT(Split[sp].maxForY - Split[sp].minForY);
    fxdLimitsForYRatio[sp] = FXD_DIV(fxd_range, FXD_CONST_127);
  }
}

void applyLimitsForZ() {
  for (byte sp = 0; sp < NUMSPLITS; ++sp) {
    int32_t fxd_range = FXD_FROM_INT(Split[sp].maxForZ - Split[sp].minForZ);
    fxdLimitsForZRatio[sp] = FXD_DIV(fxd_range, FXD_CONST_127);
  }
}

void applyLimitsForVelocity() {
  fxdMinVelOffset = FXD_FROM_INT(Global.minForVelocity * 8);
  int32_t fxd_maxVelOffset = FXD_CONST_1016 - FXD_FROM_INT(Global.maxForVelocity * 8);
  fxdVelRatio = FXD_DIV(FXD_CONST_1016 - fxdMinVelOffset - fxd_maxVelOffset, FXD_CONST_1016);
}

// Called to handle press events of the 8 control buttons
void handleControlButtonNewTouch() {
  // if we're in the startup phase after a global reset
  // a new press on a control button terminates the global reset state
  // and makes sure that startup control button combination is reset
  if (globalReset) {
    globalReset = false;
    cellTouched(0, 0, untouchedCell);
    cellTouched(0, 2, untouchedCell);
  }

  // only allow one control button to be pressed at the same time
  // this prevents phantom presses to occur for the control buttons
  // this is not detectable with the regular phantom press algorithm
  if ((rowsInColsTouched[0] & ~(1 << sensorRow)) != 0) {
    return;
  }

  if (sensorRow != SWITCH_1_ROW &&
      sensorRow != SWITCH_2_ROW) {                     // handle non-switch control buttons

    if (sensorRow == SPLIT_ROW) {                      // the split control has custom toggle / hold behavior
      if (controlButton != -1) {
        return;
      }
    }
    else if (controlButton == sensorRow) {             // detect whether this is the toggle off of a previous control press
      lastControlPress[sensorRow] = 0;
      handleControlButtonRelease();                    // in that case act as if it was a button release
      return;
    }
    else if (controlButton != -1) {                    // automatically turn off the led of another previously pressed control button
      clearLed(0, controlButton);
    }

    controlButton = sensorRow;                         // keep track of which control button we're handling
  }
 
  // determine whether a double-tap happened on the switch (ie. second tap within 500 ms)
  bool doubleTap = (calcTimeDelta(millis(), lastControlPress[sensorRow]) < 500);

  lastControlPress[sensorRow] = millis();              // keep track of the last press

  switch (sensorRow) {                                 // which control button is it?
    case GLOBAL_SETTINGS_ROW:                          // global settings button presssed
      resetAllTouches();
      lightLed(0, 0);                                  // light the button
      setDisplayMode(displayGlobal);                   // change to global settings display mode
      resetNumericDataChange();
      updateDisplay();
      break;

    case SPLIT_ROW:                                    // SPLIT button pressed
      resetAllTouches();
      splitButtonDown = true;
      changedSplitPoint = false;
      setLed(0, SPLIT_ROW, globalColor, cellOn);
      setDisplayMode(displaySplitPoint);

      // handle double-tap
      if (doubleTap) {
        Global.currentPerSplit = !Global.currentPerSplit;
        focusedSplit = Global.currentPerSplit;
      }

      updateDisplay();
      break;

    case SWITCH_2_ROW:                                 // SWITCH 2 pressed
      doSwitchPressed(SWITCH_SWITCH_2);
      setLed(0, SWITCH_2_ROW, globalColor, switchState[SWITCH_SWITCH_2][focusedSplit] ? cellOn : cellOff);
      break;

    case SWITCH_1_ROW:                                 // SWITCH 1 pressed
      doSwitchPressed(SWITCH_SWITCH_1);
      setLed(0, SWITCH_1_ROW, globalColor, switchState[SWITCH_SWITCH_1][focusedSplit] ? cellOn : cellOff);
      break;
  
    case OCTAVE_ROW:                                   // OCTAVE button pressed
      resetAllTouches();
      setLed(0, OCTAVE_ROW, globalColor, cellOn);
      setDisplayMode(displayOctaveTranspose);
      updateDisplay();
      break;

    case VOLUME_ROW:                                   // displayVolume button pressed
      resetAllTouches();
      setLed(0, VOLUME_ROW, globalColor, cellOn);
      setDisplayMode(displayVolume);
      updateDisplay();
      break;

    case PRESET_ROW:                                   // displayPreset button pressed
      resetAllTouches();
      setLed(0, PRESET_ROW, globalColor, cellOn);
      for (byte p = 0; p < NUMPRESETS; ++p) {
        presetBlinkStart[p] = 0;
      }
      setDisplayMode(displayPreset);
      resetNumericDataChange();
      updateDisplay();
      break;

    case PER_SPLIT_ROW:                                // PER SPLIT SETTINGs buttons pressed
      resetAllTouches();
      setLed(0, PER_SPLIT_ROW, globalColor, cellOn);
      setDisplayMode(displayPerSplit);
      resetNumericDataChange();
      updateDisplay();
      break;
  }
}

// Called to handle release events of the 8 control buttons
void handleControlButtonRelease() {
  // unless we pressed a new control button, no control button releases in global reset
  // phase will be taken into account, this is needed to allow users to release the
  // control button startup combination without leaving calibration mode
  if (globalReset) {
    return;
  }

  if (sensorRow != SWITCH_1_ROW &&
      sensorRow != SWITCH_2_ROW) {                                          // don't allow simultaneous control buttons except for the switches

    if (controlButton != sensorRow ||                                       // only handle the release of the control button that's currently pressed
        (millis() - lastControlPress[sensorRow] <= SWITCH_HOLD_DELAY &&     // however if this was not a hold press, don't process the release either
         controlButton != SPLIT_ROW)) {                                     // except for the split row, who has its own hold behavior
      return;
    }

    controlButton = -1;                                                     // keep track of which control button we're handling
  }

  switch (sensorRow) {
    // Most of the buttons, when released, revert the display to normal
    // and save the global settings which may have been changed.

    case GLOBAL_SETTINGS_ROW:                                // global settings button released
      if (displayMode == displayReset) {
        // ensure that MPE is actively disabled before resetting
        disableMpe(LEFT);
        disableMpe(RIGHT);

        // reset all values to default
        reset();
      }
      // fallthrough is on purpose

    case PER_SPLIT_ROW:
    case OCTAVE_ROW:                                         // octave button released
    case VOLUME_ROW:                                         // volume button released
    case PRESET_ROW:                                         // preset button released

      clearLed(0, sensorRow);

      setDisplayMode(displayNormal);
      updateDisplay();

      storeSettings();
      break;

    case SPLIT_ROW:                                          // SPLIT button released
      splitButtonDown = false;
      if (changedSplitPoint) {
        storeSettings();
      } else {
        splitActive = !splitActive;
        focusedSplit = Global.currentPerSplit;
      }
      setLed(0, SPLIT_ROW, globalColor, splitActive ? cellOn : cellOff);
      setDisplayMode(displayNormal);
      updateDisplay();
      break;

    case SWITCH_2_ROW:                                       // SWITCH 2 released
      doSwitchReleased(SWITCH_SWITCH_2);
      setLed(0, SWITCH_2_ROW, globalColor, switchState[SWITCH_SWITCH_2][focusedSplit] ? cellOn : cellOff);
      break;

    case SWITCH_1_ROW:                                       // SWITCH 1 released
      doSwitchReleased(SWITCH_SWITCH_1);
      setLed(0, SWITCH_1_ROW, globalColor, switchState[SWITCH_SWITCH_1][focusedSplit] ? cellOn : cellOff);
      break;
  }
}

// chan value is 1-16
void toggleChannel(byte chan) {
  switch (midiChannelSelect) {
    case MIDICHANNEL_MAIN:
      // in MPE mode the only valid main channels are 1 and 16
      if (!Split[Global.currentPerSplit].mpe || chan == 1 || chan == 16) {
        Split[Global.currentPerSplit].midiChanMain = chan;

        // adapt the per-note MPE channels based on the new main channel
        if (Split[Global.currentPerSplit].mpe) {
          activateMpeChannels(Global.currentPerSplit, Split[Global.currentPerSplit].midiChanMain, countMpePolyphonny(Global.currentPerSplit));
        }
      }
      break;

    case MIDICHANNEL_PERNOTE:
      if (Split[Global.currentPerSplit].mpe) {
        // in MPE mode, channels can only be a contiguous range starting from the channel next to the main channel
        if (chan != Split[Global.currentPerSplit].midiChanMain) {
          activateMpeChannels(Global.currentPerSplit, Split[Global.currentPerSplit].midiChanMain, abs(Split[Global.currentPerSplit].midiChanMain-chan));
        }
      }
      else {
        Split[Global.currentPerSplit].midiChanSet[chan-1] = !Split[Global.currentPerSplit].midiChanSet[chan-1];
      }
      break;

    case MIDICHANNEL_PERROW:
      Split[Global.currentPerSplit].midiChanPerRow = chan;
      break;
  }

  updateSplitMidiChannels(Global.currentPerSplit);
}

void updateSplitMidiChannels(byte sp) {
  switch (Split[sp].midiMode) {
    case channelPerNote:
    {
      splitChannels[sp].clear();
      for (byte ch = 0; ch < 16; ++ch) {
        if (Split[sp].midiChanSet[ch]) {
          splitChannels[sp].add(ch+1);
        }
      }
      break;
    }

    default:
    {
      splitChannels[sp].clear();
      break;
    }
  }
}

byte countMpePolyphonny(byte split) {
  if (!Split[split].mpe) {
    return 0;
  }

  byte result = 0;
  for (byte c = 0; c < 16; ++c) {
    if (Split[split].midiChanSet[c]) {
      result++;
    }
  }

  return result;
}

boolean activateMpeChannels(byte split, byte mainChannel, byte polyphony) {
  // only accept valid channel configurations
  if ((mainChannel != 1 && mainChannel != 16) ||
      polyphony > 15) {
    return false;
  }

  Split[split].midiMode = channelPerNote;

  // reset the per note channels
  for (byte c = 0; c < 16; ++c) {
    Split[split].midiChanSet[c] = false;
  }

  // set up the main channel
  Split[split].midiChanMain = mainChannel;

  // set up the per note channels
  short channelOffset = 0;
  if (mainChannel == 16) {
    channelOffset = 15-polyphony-1;
  }

  for (short c = 1; c <= polyphony; ++c) {
    Split[split].midiChanSet[c+channelOffset] = true;
  }

  updateSplitMidiChannels(split);

  // notify that MPE is on
  midiSendMpeState(mainChannel, polyphony);

  return true;
}

void configureStandardMpeExpression(byte split) {
  Split[split].bendRangeOption = bendRange24;
  Split[split].customBendRange = 24;
  Split[split].expressionForY = timbreCC74;
  Split[split].customCCForY = 74;
  Split[split].expressionForZ = loudnessChannelPressure;

  midiSendMpePitchBendRange(split);
}

void enableMpe(byte split, byte mainChannel, byte polyphony) {
  Split[split].mpe = true;
  if (activateMpeChannels(split, mainChannel, polyphony)) {
    configureStandardMpeExpression(split);
  }
}

void disableMpe(byte split) {
  if (Split[split].mpe) {
    Split[split].mpe = false;
    midiSendMpeState(Split[split].midiChanMain, 0);
  }
}

void setSplitMpeMode(byte split, boolean enabled) {
  if (enabled) {
    enableMpe(split, split == LEFT ? 1 : 16, 7);
  }
  else {
    disableMpe(split);
  }
}

// Return the next color in the color cycle (1 through 6)
byte colorCycle(byte color, boolean includeBlack) {
  if (++color > 6) {
    if (includeBlack) {
      color = 0;
    }
    else {
      color = 1;
    }
  }
  return color;
}

boolean ensureCellBeforeHoldWait(byte resetColor, CellDisplay resetDisplay) {
  if (sensorCell().lastTouch != 0) {
    if (calcTimeDelta(millis(), sensorCell().lastTouch) < SENSOR_HOLD_DELAY) {
      return true;
    }

    setLed(sensorCol, sensorRow, resetColor, resetDisplay);
  }
  return false;
}

boolean isCellPastHoldWait() {
  return sensorCell().lastTouch != 0 && calcTimeDelta(millis(), sensorCell().lastTouch) > EDIT_MODE_HOLD_DELAY;
}

void applyTimbreCC74(byte split) {
  if (Split[split].customCCForY == 128) {
    Split[split].expressionForY = timbrePolyPressure;
  }
  else if (Split[split].customCCForY == 129) {
    Split[split].expressionForY = timbreChannelPressure;
  }
  else {
    Split[split].expressionForY = timbreCC74;
  }
}

void handlePerSplitSettingNewTouch() {
  // start tracking the touch duration to be able to enable hold functionality
  sensorCell().lastTouch = millis();

  switch (sensorCol) {
    // MIDI mode settings
    case 1:
      switch (sensorRow) {
        case 7:
        case 6:
        case 5:
          Split[Global.currentPerSplit].midiMode = 7 - sensorRow;    // values are 0, 1, 2
          if (sensorRow != 6) {
            setSplitMpeMode(Global.currentPerSplit, false);
          }
          updateSplitMidiChannels(Global.currentPerSplit);
          break;
      }
      break;

    // View MIDI channels
    case 2:
      switch (sensorRow) {
        case MIDICHANNEL_MAIN:
        case MIDICHANNEL_PERNOTE:
        case MIDICHANNEL_PERROW:
          midiChannelSelect = sensorRow;
          break;
      }
      break;

    // MIDI Channel configuration
    case 3:
    case 4:
    case 5:
    case 6:
      if (sensorRow >=4 && sensorRow <= 7) {
        // Channels in column 3 are 1,5,9,13, column 4 are 2,6,10,14, column 5 are 3,7,11,15, and column 6 are 4,8,12,16
        byte chan = (7 - sensorRow) * 4 + sensorCol - 2;    // this value should be from 1 to 16
        toggleChannel(chan);
      }
      break;

    // Bend range settings
    case 7:
      switch (sensorRow) {
        case 7:
          Split[Global.currentPerSplit].bendRangeOption = bendRange2;
          break;
        case 6:
          Split[Global.currentPerSplit].bendRangeOption = bendRange3;
          break;
        case 5:
          Split[Global.currentPerSplit].bendRangeOption = bendRange12;
          break;
        case 4:
          Split[Global.currentPerSplit].bendRangeOption = bendRange24;
          break;
      }
      break;

    // Pitch/X settings
    case 8:
      switch (sensorRow) {
        case 7:
          Split[Global.currentPerSplit].sendX = !Split[Global.currentPerSplit].sendX;
          break;
        case 6:
          Split[Global.currentPerSplit].pitchCorrectQuantize = !Split[Global.currentPerSplit].pitchCorrectQuantize;
          break;
        case 5:
          if (cell(sensorCol, 4).touched != untouchedCell) {
            Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldSlow;
          }
          else {
            if (Split[Global.currentPerSplit].pitchCorrectHold == pitchCorrectHoldMedium) {
              Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldOff;
            }
            else {
              Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldMedium;
            }
          }
          applyPitchCorrectHold();
          break;
        case 4:
          if (cell(sensorCol, 5).touched != untouchedCell) {
            Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldSlow;
          }
          else {
            if (Split[Global.currentPerSplit].pitchCorrectHold == pitchCorrectHoldFast) {
              Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldOff;
            }
            else {
              Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldFast;
            }
          }
          applyPitchCorrectHold();
          break;
        case 3:
          Split[Global.currentPerSplit].pitchResetOnRelease = !Split[Global.currentPerSplit].pitchResetOnRelease;
          break;
      }
      break;

    // Timbre/Y settings
    case 9:
      switch (sensorRow) {
        case 7:
          // handled in release
          break;
        case 6:
          Split[Global.currentPerSplit].expressionForY = timbreCC1;
          break;
        case 5:
          applyTimbreCC74(Global.currentPerSplit);
          break;
        case 4:
          Split[Global.currentPerSplit].relativeY = !Split[Global.currentPerSplit].relativeY;
          break;
      }
      break;

    // Loudness/Z settings
    case 10:
      switch (sensorRow) {
        case 7:
          // handled in release
          break;
        case 6:
          Split[Global.currentPerSplit].expressionForZ = loudnessPolyPressure;
          break;
        case 5:
          Split[Global.currentPerSplit].expressionForZ = loudnessChannelPressure;
          break;
        case 4:
          Split[Global.currentPerSplit].expressionForZ = loudnessCC11;
          break;
      }
      break;

    // Color settings
    case 11:
      switch (sensorRow) {
        case 7:
          Split[Global.currentPerSplit].colorMain = colorCycle(Split[Global.currentPerSplit].colorMain, false);
          break;
        case 6:
          Split[Global.currentPerSplit].colorAccent = colorCycle(Split[Global.currentPerSplit].colorAccent, false);
          break;
        case 5:
          Split[Global.currentPerSplit].colorNoteon = colorCycle(Split[Global.currentPerSplit].colorNoteon, true);
          break;
        case 4:
          Split[Global.currentPerSplit].colorLowRow = colorCycle(Split[Global.currentPerSplit].colorLowRow, false);
          break;
      }
      break;

    // Low-row settings 1/2
    case 12:
      switch (sensorRow) {
        case 7:
          Split[Global.currentPerSplit].lowRowMode = lowRowNormal;
          break;
        case 6:
          Split[Global.currentPerSplit].lowRowMode = lowRowRestrike;
          break;
        case 5:
          Split[Global.currentPerSplit].lowRowMode = lowRowStrum;
          break;
        case 4:
          Split[Global.currentPerSplit].lowRowMode = lowRowArpeggiator;
          break;
      }
      break;

    // Low-row settings 2/2
    case 13:
      switch (sensorRow) {
        case 7:
          Split[Global.currentPerSplit].lowRowMode = lowRowSustain;
          break;
        case 6:
          Split[Global.currentPerSplit].lowRowMode = lowRowBend;
          break;
        case 5:
          // handled in release
          break;
        case 4:
          // handled in release
          break;
      }
      break;

    // Special settings
    case 14:
      switch (sensorRow) {
        case 7:
          Split[Global.currentPerSplit].arpeggiator = !Split[Global.currentPerSplit].arpeggiator;
          if (Split[Global.currentPerSplit].arpeggiator) {
            Split[Global.currentPerSplit].strum = false;
            Split[Global.currentPerSplit].ccFaders = false;
          }
          randomSeed(analogRead(0));
          break;
        case 6:
          // handled in release
          break;
        case 5:
          Split[Global.currentPerSplit].strum = !Split[Global.currentPerSplit].strum;
          if (Split[Global.currentPerSplit].strum) {
            Split[RIGHT - Global.currentPerSplit].strum = false; // there can only be one strum split
            Split[Global.currentPerSplit].arpeggiator = false;
            Split[Global.currentPerSplit].ccFaders = false;
          }
          break;
      }
      break;
  }

  updateDisplay();

  // make the sensors that are waiting for hold pulse slowly to indicate that something is going on
  switch (sensorCol) {
    case 1:
      switch (sensorRow) {
        case 6:
          setLed(sensorCol, sensorRow, getMpeColor(sensorSplit), cellSlowPulse);
          break;
      }
      break;

    case 7:
      switch (sensorRow) {
        case 4:
          setLed(sensorCol, sensorRow, getBendRangeColor(sensorSplit), cellSlowPulse);
          break;
      }
      break;

    case 9:
      switch (sensorRow) {
        case 7:
          setLed(sensorCol, sensorRow, getLimitsForYColor(sensorSplit), cellSlowPulse);
          break;
        case 5:
          setLed(sensorCol, sensorRow, getCCForYColor(sensorSplit), cellSlowPulse);
          break;
      }
      break;

    case 10:
      switch (sensorRow) {
        case 7:
          setLed(sensorCol, sensorRow, getLimitsForZColor(sensorSplit), cellSlowPulse);
          break;
        case 4:
          setLed(sensorCol, sensorRow, getCCForZColor(sensorSplit), cellSlowPulse);
          break;
      }
      break;

    case 13:
      switch (sensorRow) {
        case 5:
          setLed(sensorCol, sensorRow, getLowRowCCXColor(sensorSplit), cellSlowPulse);
          break;
        case 4:
          setLed(sensorCol, sensorRow, getLowRowCCXYZColor(sensorSplit), cellSlowPulse);
          break;
      }
      break;

    case 14:
      switch (sensorRow) {
        case 6:
          setLed(sensorCol, sensorRow, getCCFadersColor(sensorSplit), cellSlowPulse);
          break;
      }
      break;
  }
}

void handlePerSplitSettingHold() {
  if (isCellPastHoldWait()) {
    sensorCell().lastTouch = 0;

    switch (sensorCol) {
      case 1:
        switch (sensorRow) {
          case 6:
            setSplitMpeMode(Global.currentPerSplit, true);
            updateDisplay();
            break;
        }
        break;

      case 7:
        switch (sensorRow) {
          case 4:
            resetNumericDataChange();
            setDisplayMode(displayBendRange);
            updateDisplay();
            break;
        }
        break;

      case 9:
        switch (sensorRow) {
          case 7:
            resetNumericDataChange();
            setDisplayMode(displayLimitsForY);
            updateDisplay();
            break;
          case 5:
            resetNumericDataChange();
            setDisplayMode(displayCCForY);
            updateDisplay();
            break;
        }
        break;

      case 10:
        switch (sensorRow) {
          case 7:
            resetNumericDataChange();
            setDisplayMode(displayLimitsForZ);
            updateDisplay();
            break;
          case 4:
            resetNumericDataChange();
            setDisplayMode(displayCCForZ);
            updateDisplay();
            break;
        }
        break;

      case 13:
        switch (sensorRow) {
          case 5:
            lowRowCCXConfigState = 1;
            resetNumericDataChange();
            setDisplayMode(displayLowRowCCXConfig);
            updateDisplay();
            break;
          case 4:
            lowRowCCXYZConfigState = 3;
            resetNumericDataChange();
            setDisplayMode(displayLowRowCCXYZConfig);
            updateDisplay();
            break;
        }
        break;

      case 14:
        switch (sensorRow) {
          case 6:
            resetNumericDataChange();
            setDisplayMode(displayCCForFader);
            updateDisplay();
            break;
        }
        break;
    }
  }
}

void handlePerSplitSettingRelease() {
  switch (sensorCol) {
    case 1:
      switch (sensorRow) {
        case 6:
          if (ensureCellBeforeHoldWait(getMpeColor(Global.currentPerSplit),
                                       Split[Global.currentPerSplit].midiMode == channelPerNote ? cellOn : cellOff)) {
            setSplitMpeMode(Global.currentPerSplit, false);
          }
          break;
      }
      break;

    case 7:
      switch (sensorRow) {
        case 4:
          if (ensureCellBeforeHoldWait(getBendRangeColor(Global.currentPerSplit),
                                       Split[Global.currentPerSplit].bendRangeOption == bendRange24 ? cellOn : cellOff)) {
            Split[Global.currentPerSplit].bendRangeOption = bendRange24;
          }
          break;
      }
      break;

    case 9:
      switch (sensorRow) {
        case 7:
          if (ensureCellBeforeHoldWait(getLimitsForYColor(Global.currentPerSplit),
                                      Split[Global.currentPerSplit].sendY ? cellOn : cellOff)) {
            Split[Global.currentPerSplit].sendY = !Split[Global.currentPerSplit].sendY;
          }
          break;
        case 5: {
          CellDisplay resetDisplay = cellOff;
          if (Split[Global.currentPerSplit].expressionForY == timbrePolyPressure ||
              Split[Global.currentPerSplit].expressionForY == timbreChannelPressure ||
              Split[Global.currentPerSplit].expressionForY == timbreCC74) {
            resetDisplay = cellOn;
          }
          if (ensureCellBeforeHoldWait(getCCForYColor(Global.currentPerSplit), resetDisplay)) {
            applyTimbreCC74(Global.currentPerSplit);
          }
          break;
        }
      }
      break;

    case 10:
      switch (sensorRow) {
        case 7:
          if (ensureCellBeforeHoldWait(getLimitsForZColor(Global.currentPerSplit),
                                       Split[Global.currentPerSplit].sendZ ? cellOn : cellOff)) {
            Split[Global.currentPerSplit].sendZ = !Split[Global.currentPerSplit].sendZ;
          }
          break;
        case 4:
          if (ensureCellBeforeHoldWait(getCCForZColor(Global.currentPerSplit),
                                       Split[Global.currentPerSplit].expressionForZ == loudnessCC11 ? cellOn : cellOff)) {
            Split[Global.currentPerSplit].expressionForZ = loudnessCC11;
          }
          break;
      }
      break;

    case 13:
      switch (sensorRow) {
        case 5:
          if (ensureCellBeforeHoldWait(getLowRowCCXColor(Global.currentPerSplit),
                                       Split[Global.currentPerSplit].expressionForZ == lowRowCCX ? cellOn : cellOff)) {
            Split[Global.currentPerSplit].lowRowMode = lowRowCCX;
          }
          break;
        case 4:
          if (ensureCellBeforeHoldWait(getLowRowCCXYZColor(Global.currentPerSplit),
                                       Split[Global.currentPerSplit].expressionForZ == lowRowCCXYZ ? cellOn : cellOff)) {
            Split[Global.currentPerSplit].lowRowMode = lowRowCCXYZ;
          }
          break;
      }
      break;

    case 14:
      switch (sensorRow) {
        case 6:
          if (ensureCellBeforeHoldWait(getCCFadersColor(Global.currentPerSplit),
                                       Split[Global.currentPerSplit].ccFaders ? cellOn : cellOff)) {
            Split[Global.currentPerSplit].ccFaders = !Split[Global.currentPerSplit].ccFaders;
            if (Split[Global.currentPerSplit].ccFaders) {
              Split[Global.currentPerSplit].arpeggiator = false;
              Split[Global.currentPerSplit].strum = false;
            }
          }
          break;
      }
      break;
  }

  sensorCell().lastTouch = 0;

  handleShowSplit();

  updateDisplay();
}

// This function handles use of the "Show Split" cells,
// and returns true if one of them was hit.
boolean handleShowSplit() {
  // Two cells in the top row (col 15 and 16) lets you change which side you're controlling
  if (sensorRow == 7) {
    boolean hit = false;
    byte newSplit;

    if (sensorCol == 15) {
      newSplit = LEFT;
      hit = true;
    }
    else if (sensorCol == 16) {
      newSplit = RIGHT;
      hit = true;
    }

    if (hit && !doublePerSplit) {
      // if we're in sub-menus of the per-split settings and the active split cell is tapped again
      // it goes back to the main per-split settings menu
      if (Global.currentPerSplit == newSplit) {
        if (displayMode != displayVolume && displayMode != displayOctaveTranspose) {
          setDisplayMode(displayPerSplit);
        }
      }
      // otherwise switch the split for the currently active panel
      else {
        Global.currentPerSplit = newSplit;
        focusedSplit = newSplit;
      }
      resetNumericDataChange();
      updateDisplay();
    }
  }

  return false;
}

void handlePresetNewTouch() {
  if ((sensorCol == 1 && sensorRow == 7 && midiPreset[Global.currentPerSplit] < 127) ||
      (sensorCol == 1 && sensorRow == 6 && midiPreset[Global.currentPerSplit] > 0)) {
    midiPreset[Global.currentPerSplit] += (sensorRow == 7 ? 1 : -1);
    lastControlPress[PRESET_ROW] = millis() - SWITCH_HOLD_DELAY;
    applyMidiPreset();
    updateDisplay();
  }

  // don't change presets on the row that has the split selection
  if (sensorRow == 7) {
    return;
  }

  if (sensorCol >= NUMCOLS-2) {
    if (sensorRow >= 2 && sensorRow < 2 + NUMPRESETS) {
      // start tracking the touch duration to be able detect a long press
      sensorCell().lastTouch = millis();
      // indicate that a hold operation is being waited for
      setLed(sensorCol, sensorRow, globalColor, cellSlowPulse);
    }
  }
  else if (sensorCol < NUMCOLS-2) {
    if (handleNumericDataNewTouchCol(midiPreset[Global.currentPerSplit], 0, 127, true)) {
      applyMidiPreset();
    }
  }
}

void startPresetLEDBlink(byte p, byte color) {
  unsigned long now = millis();
  if (now == 0) {
    now = ~now;
  }
  presetBlinkStart[p] = now;

  setLed(NUMCOLS-2, p+2, color, cellPulse);
}

void handlePresetHold() {
  if (sensorCol == NUMCOLS-2 &&
      sensorRow >= 2 && sensorRow < 2 + NUMPRESETS &&
      isCellPastHoldWait()) {
    // store to the selected preset
    byte preset = sensorRow-2;
    storeSettingsToPreset(preset);
    sensorCell().lastTouch = 0;

    saveSettings();
    updateDisplay();
    startPresetLEDBlink(preset, COLOR_RED);
  }
}

void applyMidiPreset() {
  byte chan = Split[Global.currentPerSplit].midiChanMain;
  midiSendPreset(midiPreset[Global.currentPerSplit], chan);
}

void handlePresetRelease() {
  if (sensorCol == NUMCOLS-1) {
    return;
  }

  if (sensorCol < NUMCOLS-2) {
    handleNumericDataReleaseCol(true);
  }
  else if (sensorCol == NUMCOLS-2) {
    if (sensorRow >= 2 && sensorRow < 2 + NUMPRESETS &&
        ensureCellBeforeHoldWait(globalColor, cellOn)) {
      byte preset = sensorRow-2;

      // load the selected preset
      applyPresetSettings(config.preset[preset]);
      sensorCell().lastTouch = 0;

      saveSettings();
      updateDisplay();
      startPresetLEDBlink(preset, COLOR_GREEN);
    }
  }
}

void handleBendRangeNewTouch() {
  handleNumericDataNewTouchCol(Split[Global.currentPerSplit].customBendRange, 1, 96, false);
}

void handleBendRangeRelease() {
  handleNumericDataReleaseCol(true);
}

void handleLimitsForYNewTouch() {
  switch (limitsForYConfigState) {
    case 1:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].minForY, 0, 127, false);
      break;
    case 0:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].maxForY, 0, 127, false);
      break;
  }
  handleNumericDataNewTouchRow(limitsForYConfigState, 0, 1);
}

void handleLimitsForYRelease() {
  handleNumericDataReleaseCol(true);
  handleNumericDataReleaseRow(true);
  applyLimitsForY();
}

void handleCCForYNewTouch() {
  handleNumericDataNewTouchCol(Split[Global.currentPerSplit].customCCForY, 0, 129, false);
  applyCustomCCForY(Global.currentPerSplit);
}

void applyCustomCCForY(byte split) {
  if (Split[split].customCCForY == 128) {
    Split[split].expressionForY = timbrePolyPressure;
  }
  else if (Split[split].customCCForY == 129) {
    Split[split].expressionForY = timbreChannelPressure;
  }
  else {
    Split[split].expressionForY = timbreCC74;
  }
}

void handleCCForYRelease() {
  handleNumericDataReleaseCol(true);
}

void handleLimitsForZNewTouch() {
  switch (limitsForZConfigState) {
    case 1:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].minForZ, 0, 127, false);
      break;
    case 0:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].maxForZ, 0, 127, false);
      break;
  }
  handleNumericDataNewTouchRow(limitsForZConfigState, 0, 1);
}

void handleLimitsForZRelease() {
  handleNumericDataReleaseCol(true);
  handleNumericDataReleaseRow(true);
  applyLimitsForZ();
}

void handleCCForZNewTouch() {
  handleNumericDataNewTouchCol(Split[Global.currentPerSplit].customCCForZ, 0, 127, false);
}

void handleCCForZRelease() {
  handleNumericDataReleaseCol(true);
}

void handleCCForFaderNewTouch() {
  if (sensorCol == NUMCOLS-1) {
    currentEditedCCFader[Global.currentPerSplit] = sensorRow;
    updateDisplay();
  }
  else {
    byte current = currentEditedCCFader[Global.currentPerSplit];
    handleNumericDataNewTouchCol(Split[Global.currentPerSplit].ccForFader[current], 0, 127, false);
  }
}

void handleCCForFaderRelease() {
  if (sensorCol < NUMCOLS-1) {
    handleNumericDataReleaseCol(true);
  }
}

void handleLowRowCCXConfigNewTouch() {
  switch (lowRowCCXConfigState) {
    case 1:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].lowRowCCXBehavior, 0, 1, false);
      break;
    case 0:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].ccForLowRow, 0, 127, false);
      break;
  }
  handleNumericDataNewTouchRow(lowRowCCXConfigState, 0, 1);
}

void handleLowRowCCXConfigRelease() {
  handleNumericDataReleaseCol(true);
  handleNumericDataReleaseRow(true);
}

void handleLowRowCCXYZConfigNewTouch() {
  switch (lowRowCCXYZConfigState) {
    case 3:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].lowRowCCXYZBehavior, 0, 1, false);
      break;
    case 2:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].ccForLowRowX, 0, 127, false);
      break;
    case 1:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].ccForLowRowY, 0, 127, false);
      break;
    case 0:
      handleNumericDataNewTouchCol(Split[Global.currentPerSplit].ccForLowRowZ, 0, 127, false);
      break;
  }
  handleNumericDataNewTouchRow(lowRowCCXYZConfigState, 0, 3);
}

void handleLowRowCCXYZConfigRelease() {
  handleNumericDataReleaseCol(true);
  handleNumericDataReleaseRow(true);
}

void handleCCForSwitchConfigNewTouch() {
  handleNumericDataNewTouchCol(Global.ccForSwitch, 0, 127, false);
}

void handleCCForSwitchConfigRelease() {
  handleNumericDataReleaseCol(false);
}

void handleLimitsForVelocityNewTouch() {
  switch (limitsForVelocityConfigState) {
    case 1:
      handleNumericDataNewTouchCol(Global.minForVelocity, 1, 127, false);
      break;
    case 0:
      handleNumericDataNewTouchCol(Global.maxForVelocity, 1, 127, false);
      break;
  }
  handleNumericDataNewTouchRow(limitsForVelocityConfigState, 0, 1);
}

void handleLimitsForVelocityRelease() {
  handleNumericDataReleaseCol(false);
  handleNumericDataReleaseRow(false);
  applyLimitsForVelocity();
}

void handleValueForFixedVelocityNewTouch() {
  handleNumericDataNewTouchCol(Global.valueForFixedVelocity, 1, 127, false);
}

void handleValueForFixedVelocityRelease() {
  handleNumericDataReleaseCol(false);
}

void handleMinUSBMIDIIntervalNewTouch() {
  handleNumericDataNewTouchCol(Device.minUSBMIDIInterval, 0, 512, false);
}

void handleMinUSBMIDIIntervalRelease() {
  handleNumericDataReleaseCol(false);
  applyMidiInterval();
}

void handleSensorLoZNewTouch() {
  handleNumericDataNewTouchCol(Device.sensorLoZ, max(0, Device.sensorFeatherZ), 1024, false);
}

void handleSensorLoZRelease() {
  handleNumericDataReleaseCol(false);
}

void handleSensorFeatherZNewTouch() {
  handleNumericDataNewTouchCol(Device.sensorFeatherZ, 0, min(1024, Device.sensorLoZ), false);
}

void handleSensorFeatherZRelease() {
  handleNumericDataReleaseCol(false);
}

void handleSensorRangeZNewTouch() {
  handleNumericDataNewTouchCol(Device.sensorRangeZ, 3 * 127, MAX_SENSOR_RANGE_Z - 127, false);
}

void handleSensorRangeZRelease() {
  handleNumericDataReleaseCol(false);
}

void handleVolumeNewTouch(boolean newVelocity) {
  // don't change volume on the row that has the split selection
  if (sensorRow == 7) {
    return;
  }

  if (sensorCell().velocity) {
    // if a touch is already down, only consider new touches from the same row
    // this allows the volume slider to be used anywhere on the surface and
    // for it to be 'played' with multiple touches on the same row without
    // interference from other rows
    if (cellsTouched) {
      for (int r = 0; r < NUMROWS; ++r) {
        if (r != sensorRow && (colsInRowsTouched[r] & ~(1 << sensorCol)) != 0) {
          cellTouched(ignoredCell);
          return;
        }
      }
    }

    // if a new touch happens on the same row that is further down the row, make it
    // take over the touch
    if (newVelocity) {
      for (byte col = NUMCOLS - 1; col >= 1; --col) {
        if (col != sensorCol && cell(col, sensorRow).velocity) {
          transferFromSameRowCell(col);
          return;
        }
      }
    }

    short value = calculateFaderValue(sensorCell().calibratedX(), 1, 24);

    if (value >= 0) {
      short previous = ccFaderValues[Global.currentPerSplit][7];
      ccFaderValues[Global.currentPerSplit][7] = value;

      byte chan = Split[Global.currentPerSplit].midiChanMain;
      midiSendVolume(value, chan);     // Send the MIDI volume controller message
      if (previous != value) {
        paintVolumeDisplayRow(Global.currentPerSplit);
      }
    }
  }
}

void handleVolumeRelease() {
  // see if one of the "Show Split" cells have been hit
  if (handleShowSplit()) {
    return;
  }

  // if another touch is already down on the same row, make it take over the
  if (sensorCell().velocity) {
    for (byte col = NUMCOLS - 1; col >= 1; --col) {
      if (col != sensorCol && cell(col, sensorRow).touched == touchedCell) {
        transferToSameRowCell(col);
        break;
      }
    }
  }
}

void handleOctaveTransposeNewTouch() {
  // handle double per split selection
  if (sensorRow == 7 && (sensorCol == 15 || sensorCol == 16)) {
    doublePerSplit = cell(15, 7).touched == touchedCell && cell(16, 7).touched == touchedCell;
    updateDisplay();
    return;
  }

  if (doublePerSplit) {
    handleOctaveTransposeNewTouchSplit(LEFT);
    handleOctaveTransposeNewTouchSplit(RIGHT);
  }
  else {
    handleOctaveTransposeNewTouchSplit(Global.currentPerSplit);
  }

  updateDisplay();
}

void handleOctaveTransposeNewTouchSplit(byte side) {
  if (sensorRow == OCTAVE_ROW) {
    switch (sensorCol) {
      case 3: Split[side].transposeOctave = -60; break;
      case 4: Split[side].transposeOctave = -48; break;
      case 5: Split[side].transposeOctave = -36; break;
      case 6: Split[side].transposeOctave = -24; break;
      case 7: Split[side].transposeOctave = -12; break;
      case 8: Split[side].transposeOctave = 0; break;
      case 9: Split[side].transposeOctave = 12; break;
      case 10: Split[side].transposeOctave = 24; break;
      case 11: Split[side].transposeOctave = 36; break;
      case 12: Split[side].transposeOctave = 48; break;
      case 13: Split[side].transposeOctave = 60; break;
    }

  } else if (sensorRow == SWITCH_1_ROW) {
    if (sensorCol > 0 && sensorCol < 16) {
      Split[side].transposePitch = sensorCol - 8;
    }
  } else if (sensorRow == SWITCH_2_ROW) {
    if (sensorCol > 0 && sensorCol < 16) {
      Split[side].transposeLights = sensorCol - 8;
    }
  }
}

void handleOctaveTransposeRelease() {
  handleShowSplit();  // see if one of the "Show Split" cells have been hit
}

void handleSplitPointNewTouch() {
  if (sensorCol < 2) return;
  changedSplitPoint = true;
  Global.splitPoint = sensorCol;
  updateDisplay();
}

// This manages the toggling of the note light cells (columns 2-4 and rows 0-3)
void toggleNoteLights(int& notelights) {
  if (sensorCol < 2 || sensorCol > 4 || sensorRow > 3) {
    return;
  }

  byte light = sensorCol-2 + (sensorRow*3);
  notelights ^= 1 << light;
}

void setActiveNoteLights() {
  if (sensorCol < 2 || sensorCol > 4 || sensorRow > 3) {
    return;
  }

  Global.activeNotes = sensorCol-2 + (sensorRow*3);
}

boolean isArpeggiatorTempoTriplet() {
  return Global.arpTempo == ArpEighthTriplet || Global.arpTempo == ArpSixteenthTriplet || Global.arpTempo == ArpThirtysecondTriplet;
}

void handleTempoNewTouch() {
  // keep track of how many cells are currently down
  numericActiveColDown++;

  if (!isMidiClockRunning()) {
    unsigned long now = micros();
    byte increment = 1;

    // if the swipe is fast, increment by a larger amount.
    if (calcTimeDelta(now, tempoChangeTime) < 70000) {
      increment = 5;
    }
    else if (calcTimeDelta(now, tempoChangeTime) < 120000) {
      increment = 2;
    }

    if (numericDataChangeCol < 0 ||
        (numericDataChangeCol < sensorCol && numericDataChangeColLast > sensorCol) ||
        (numericDataChangeCol > sensorCol && numericDataChangeColLast < sensorCol)) {
      // First cell hit after starting a data change or change of direction,
      // don't change data yet.
      numericDataChangeCol = sensorCol;
      numericDataChangeColLast = sensorCol;
    }
    else if (sensorCol > numericDataChangeCol) {
      fxd4CurrentTempo = constrain(fxd4CurrentTempo + FXD4_FROM_INT(increment), FXD4_FROM_INT(1), FXD4_FROM_INT(360));
    }
    else if (sensorCol < numericDataChangeCol) {
      fxd4CurrentTempo = constrain(fxd4CurrentTempo - FXD4_FROM_INT(increment), FXD4_FROM_INT(1), FXD4_FROM_INT(360));
    }

    numericDataChangeColLast = sensorCol;
    tempoChangeTime = now;
  }

  setDisplayMode(displayGlobalWithTempo);
  updateDisplay();
}

void changeUserFirmwareMode(boolean active) {
  if (userFirmwareActive == active) return;

  controlButton = -1;
  userFirmwareActive = active;

  initializeLedsLayer(LED_LAYER_CUSTOM2);
  clearDisplay();
  clearSwitches();

  applyMidiDecimationRate();

  if (active) {
    for (byte r = 0; r < NUMROWS; ++r) {
      userFirmwareSlideMode[r] = false;
      userFirmwareXActive[r] = false;
      userFirmwareYActive[r] = false;
      userFirmwareZActive[r] = false;
    }
  }

  midiSendNRPN(245, userFirmwareActive, 9);

  cellTouched(ignoredCell);
  clearLed(0, GLOBAL_SETTINGS_ROW);
  setDisplayMode(displayNormal);
  completelyRefreshLeds();
  updateDisplay();
}

// Called to handle a change in one of the Global Settings,
// meaning that user is holding global settings and touching one of global settings cells
void handleGlobalSettingNewTouch() {

#ifdef DEBUG_ENABLED
  // Column 17 is for controlling debug levels
  if (sensorCol == 17 && sensorRow < 5) {
    debugLevel = sensorRow - 1;
    DEBUGPRINT((-1,"debugLevel = "));
    DEBUGPRINT((-1,debugLevel));
    DEBUGPRINT((-1,"\n"));
  }

  if (sensorCol == 18 && sensorRow < SECRET_SWITCHES) {
    // This is a hidden feature, to make it easy to toggle debug printing of MIDI messages.
    byte ss = sensorRow;
    secretSwitch[ss] = !secretSwitch[ss];
    DEBUGPRINT((-1,"secretSwitch["));
    DEBUGPRINT((-1,ss));
    DEBUGPRINT((-1,"]="));
    DEBUGPRINT((-1,secretSwitch[ss]));
    DEBUGPRINT((-1,"\n"));
  }
#endif

  // start tracking the touch duration to be able to enable hold functionality
  sensorCell().lastTouch = millis();

  // select the Velocity Sensitivity
  switch (sensorCol) {
    case 10:
      // Note: this assumes the VelocitySensitivity values exactly match the sensor rows
      switch (sensorRow) {
        case velocityLow:
        case velocityMedium:
        case velocityHigh:
        case velocityFixed:
          Global.velocitySensitivity = VelocitySensitivity(sensorRow);
          break;
      }
      break;

    // select the Pressure Sensitivity and behaviour
    case 11:
      switch (sensorRow) {
        // Note: this assumes the PressureSensitivity values exactly match the sensor rows
        case pressureLow:
        case pressureMedium:
        case pressureHigh:
          Global.pressureSensitivity = PressureSensitivity(sensorRow);
          break;
        case 3:
          Global.pressureAftertouch = !Global.pressureAftertouch;
      }
      break;

    // select the MIDI I/O
    case 15:
      switch (sensorRow) {
        case 0:
          changeMidiIO(1);
          break;
        case 1:
          changeMidiIO(0);
          break;
        case 3:
          Device.operatingLowPower = !Device.operatingLowPower;
          applyLowPowerMode();
          break;
      }
      break;

    case 16:
      if (sensorRow == 1) {
        setDisplayMode(displayOsVersion);
      }
      // reset feature
      else if ((sensorRow == 2 && cell(sensorCol, 0).touched != untouchedCell) ||
                (sensorRow == 0 && cell(sensorCol, 2).touched != untouchedCell)) {
        if (displayMode != displayReset) {
          reset();
          setDisplayMode(displayReset);
        }
      }
      else if (sensorRow == 3) {
        setDisplayMode(displayCalibration);
        initializeCalibrationSamples();
      }
      break;

    case 25:
      switch (sensorRow) {
        case 0:
          resetNumericDataChange();
          setDisplayMode(displaySensorLoZ);
          break;
        case 1:
          resetNumericDataChange();
          setDisplayMode(displaySensorFeatherZ);
          break;
        case 2:
          resetNumericDataChange();
          setDisplayMode(displaySensorRangeZ);
          break;
      }

      break;
  }

  if (!userFirmwareActive) {

    // handle tempo change
    if (sensorRow >= 4 && sensorRow != 7) {
      handleTempoNewTouch();
    }

    // select Scale Notes or Accent Notes
    switch (sensorCol) {
      case 1:
        switch (sensorRow) {
          case LIGHTS_MAIN:
          case LIGHTS_ACCENT:
          case LIGHTS_ACTIVE:
            lightSettings = sensorRow;
            break;
          // toggle left handed mode
          case 3:
            Device.leftHanded = !Device.leftHanded;
            break;
        }
        break;

      case 2:
      case 3:
      case 4:
        if (sensorRow >= 0 && sensorRow <= 3) {
          // select individual scale notes or accent notes
          switch (lightSettings) {
            case LIGHTS_MAIN:
              toggleNoteLights(Global.mainNotes[Global.activeNotes]);
              break;
            case LIGHTS_ACCENT:
              toggleNoteLights(Global.accentNotes[Global.activeNotes]);
              break;
            case LIGHTS_ACTIVE:
              setActiveNoteLights();
          }
        }
        break;

      // select one of 8 Row Offsets: 0 = no overlap, 1-12 = 1=12, 13 = octave, 14 = guitar
      case 5:
        switch (sensorRow) {
          case 0:
            if (Global.rowOffset == 3) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = 3;
            }
            break;
          case 1:
            if (Global.rowOffset == 5) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = 5;
            }
            break;
          case 2:
            if (Global.rowOffset == 7) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = 7;
            }
            break;
          case 3:
            if (Global.rowOffset == ROWOFFSET_NOOVERLAP) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = ROWOFFSET_NOOVERLAP;
            }
            break;
        }
        break;

      // select more row offsets
      case 6:
        switch (sensorRow) {
          case 0:
            if (Global.rowOffset == 4) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = 4;
            }
            break;
          case 1:
            if (Global.rowOffset == 6) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = 6;
            }
            break;
          case 2:
            if (Global.rowOffset == 12) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = 12;      // octave
            }
            break;
          case 3:
            if (Global.rowOffset == 13) {
              Global.rowOffset = ROWOFFSET_ZERO;
            }
            else {
              Global.rowOffset = 13;      // guitar
            }
            break;
        }
        break;

      case 7:
        // select which switch is being controlled/displayed
        if (sensorRow < 4) {
          switchSelect = sensorRow;    // assumes the values of SWITCH_* are equal to the row numbers
        }
        break;

      // set the switch targets
      case 8:
        switch (sensorRow) {
          case 0:
            Global.setSwitchAssignment(switchSelect, ASSIGNED_ARPEGGIATOR);
            break;
          case 1:
            Global.setSwitchAssignment(switchSelect, ASSIGNED_SUSTAIN);
            break;
          case 2:
            if (cell(9, sensorRow).touched != untouchedCell) {
              Global.setSwitchAssignment(switchSelect, ASSIGNED_AUTO_OCTAVE);
            }
            else {
              Global.setSwitchAssignment(switchSelect, ASSIGNED_OCTAVE_DOWN);
            }
            break;
          case 3:
            // toggle whether the switches operate on both splits or not
            Global.switchBothSplits[switchSelect] = !Global.switchBothSplits[switchSelect];
            break;
        }
        break;

      case 9:
        switch (sensorRow) {
          case 0:
            Global.setSwitchAssignment(switchSelect, ASSIGNED_ALTSPLIT);
            break;
          case 1:
            // handled at release
            break;
          case 2:
            if (cell(8, sensorRow).touched != untouchedCell) {
              Global.setSwitchAssignment(switchSelect, ASSIGNED_AUTO_OCTAVE);
            }
            else {
              Global.setSwitchAssignment(switchSelect, ASSIGNED_OCTAVE_UP);
            }
            break;
        }
        break;

      case 12:
        switch (sensorRow) {
          case 0:
            if (cell(sensorCol, 1).touched != untouchedCell) {
              Global.arpDirection = ArpUpDown;
            }
            else {
              Global.arpDirection = ArpDown;
            }
            break;
          case 1:
            if (cell(sensorCol, 0).touched != untouchedCell) {
              Global.arpDirection = ArpUpDown;
            }
            else {
              Global.arpDirection = ArpUp;
            }
            break;
          case 2:
            Global.arpDirection = ArpRandom;
            break;
          case 3:
            Global.arpDirection = ArpReplayAll;
            break;
        }
        break;

      case 13:
        switch (sensorRow) {
          case 0:
            if (cell(sensorCol, 1).touched != untouchedCell) {
              Global.arpTempo = ArpSixteenthSwing;
            }
            else {
              if (isArpeggiatorTempoTriplet()) {
                Global.arpTempo = ArpEighthTriplet;
              }
              else {
                Global.arpTempo = ArpEighth;
              }
            }
            break;
          case 1:
            if (cell(sensorCol, 0).touched != untouchedCell) {
              Global.arpTempo = ArpSixteenthSwing;
            }
            else {
              if (isArpeggiatorTempoTriplet()) {
                Global.arpTempo = ArpSixteenthTriplet;
              }
              else {
                Global.arpTempo = ArpSixteenth;
              }
            }
            break;
          case 2:
            if (isArpeggiatorTempoTriplet()) {
              Global.arpTempo = ArpThirtysecondTriplet;
            }
            else {
              Global.arpTempo = ArpThirtysecond;
            }
            break;
          case 3:
            switch (Global.arpTempo) {
              case ArpEighth:
                Global.arpTempo = ArpEighthTriplet;
                break;
              case ArpEighthTriplet:
                Global.arpTempo = ArpEighth;
                break;
              case ArpSixteenth:
                Global.arpTempo = ArpSixteenthTriplet;
                break;
              case ArpSixteenthTriplet:
                Global.arpTempo = ArpSixteenth;
                break;
              case ArpThirtysecond:
                Global.arpTempo = ArpThirtysecondTriplet;
                break;
              case ArpThirtysecondTriplet:
                Global.arpTempo = ArpThirtysecond;
                break;
            }
            break;
        }
        break;

      case 14:
        switch (sensorRow) {
          case 0:
            if (Global.arpOctave == 1) {
              Global.arpOctave = 0;
            }
            else {
              Global.arpOctave = 1;
            }
            break;
          case 1:
            if (Global.arpOctave == 2) {
              Global.arpOctave = 0;
            }
            else {
              Global.arpOctave = 2;
            }
            break;
          case 3:
            if (!isMidiClockRunning()) {
              lightLed(14, 3);

              tapTempoPress();
              setDisplayMode(displayGlobalWithTempo);

              delayUsec(100000);

              clearLed(14, 3);
            }
            break;
        }
        break;
    }
  }

  updateDisplay();

  // make the sensors that are waiting for hold pulse slowly to indicate that something is going on
  switch (sensorCol) {
    case 9:
      switch (sensorRow) {
        case 1:
          setLed(sensorCol, sensorRow, getSwitchCC65Color(), cellSlowPulse);
          break;
      }
      break;

    case 10:
      switch (sensorRow) {
        case 0:
        case 1:
        case 2:
          setLed(sensorCol, sensorRow, getVelocityColor(), cellSlowPulse);
          break;
        case 3:
          setLed(sensorCol, sensorRow, getFixedVelocityColor(), cellSlowPulse);
          break;
      }
      break;

    case 15:
      switch (sensorRow) {
        case 0:
          setLed(sensorCol, sensorRow, getMIDIUSBColor(), cellSlowPulse);
          break;
      }
      break;

    case 16:
      switch (sensorRow) {
        case 2:
          setLed(sensorCol, sensorRow, globalColor, cellSlowPulse);
          break;
      }
      break;
  }

  if (sensorRow == 7 && sensorCol <= 16) {
    setLed(sensorCol, sensorRow, globalColor, cellSlowPulse);
  }
}

void changeMidiIO(byte where) {
  if (where == 0) {
    Global.midiIO = 0;       // Set LOW for DIN jacks
  }
  else if (where == 1) {
    Global.midiIO = 1;       // Set HIGH for USB
  }
  applyMidiIo();
}

void handleGlobalSettingHold() {

  if (isCellPastHoldWait()) {
    sensorCell().lastTouch = 0;

    switch (sensorCol) {
      case 9:
        switch (sensorRow) {
          case 1:
            resetNumericDataChange();
            setDisplayMode(displayCCForSwitch);
            updateDisplay();
            break;
        }
        break;

      case 10:
        switch (sensorRow) {
          case 0:
          case 1:
          case 2:
            resetNumericDataChange();
            setDisplayMode(displayLimitsForVelocity);
            updateDisplay();
            break;
          case 3:
            resetNumericDataChange();
            setDisplayMode(displayValueForFixedVelocity);
            updateDisplay();
            break;
        }
        break;

      case 15:
        switch (sensorRow) {
          case 0:
            resetNumericDataChange();
            setDisplayMode(displayMinUSBMIDIInterval);
            updateDisplay();
            break;
        }
        break;

      case 16:
        switch (sensorRow) {
          // handle switch to/from User Firmware Mode
          case 2:
            // ensure that this is not a reset operation instead
            if (cell(16, 0).touched == untouchedCell) {
              changeUserFirmwareMode(!userFirmwareActive);
            }
            break;
        }
        break;
    }

    if (sensorRow == 7 && sensorCol <= 16) {
      // initialize the touch-slide interface
      resetNumericDataChange();

      // switch to edit audience message
      setDisplayMode(displayEditAudienceMessage);

      // set the information of the edited message
      audienceMessageOffset = 0;
      audienceMessageToEdit = sensorCol - 1;

      // fill in all 30 spaces of the message
      int strl = strlen(Device.audienceMessages[audienceMessageToEdit]);
      if (strl < 30) {
        for (byte ch = strl; ch < 30; ++ch) {
          Device.audienceMessages[audienceMessageToEdit][ch] = ' ';
        }
      }
      Device.audienceMessages[audienceMessageToEdit][30] = '\0';

      // calculate the length of the message to edit
      audienceMessageLength = font_width_string(Device.audienceMessages[audienceMessageToEdit], &bigFont) - NUMCOLS;

      // show the editing mode
      updateDisplay();
    }
  }
}

void handleGlobalSettingRelease() {
  if (sensorRow == 7) {
    // only show the messages if the tempo was changed more than 1s ago to prevent accidental touches
    if (calcTimeDelta(micros(), tempoChangeTime) >= 1000000) {
      if (sensorCol <= 16 && ensureCellBeforeHoldWait(COLOR_BLACK, cellOff)) {
        clearDisplay();
        big_scroll_text_flipped(Device.audienceMessages[sensorCol - 1], Split[LEFT].colorMain);        
      }
      else if (sensorCol == 25) {
        Device.promoAnimation = !Device.promoAnimation;
        storeSettings();

        if (Device.promoAnimation) {
          playPromoAnimation();
        }
      }
    }
  }

  if (sensorCol == 9 && sensorRow == 1 &&
      ensureCellBeforeHoldWait(globalColor, Global.switchAssignment[switchSelect] == ASSIGNED_CC_65 ? cellOn : cellOff)) {
    Global.setSwitchAssignment(switchSelect, ASSIGNED_CC_65);
  }

  // Toggle UPDATE OS value
  if (sensorCol == 16 && sensorRow == 2) {
    byte resetColor = COLOR_BLACK;
    CellDisplay resetDisplay = cellOff;
    if (Device.serialMode) {
      resetColor = globalColor;
      resetDisplay = cellOn;
    }

    if (ensureCellBeforeHoldWait(resetColor, resetDisplay)) {
      switchSerialMode(!Device.serialMode);
      storeSettings();
    }
  }

  if (!userFirmwareActive) {

    if (sensorRow >= 4 && sensorRow != 7) {
      handleNumericDataReleaseCol(false);
    }

    if (sensorCol == 16) {
      // Send AllNotesOff
      if (sensorRow == 0) {
        lightLed(16, 0);
        if (splitActive) {
          midiSendAllNotesOff(LEFT);
          midiSendAllNotesOff(RIGHT);
        } else {
          midiSendAllNotesOff(Global.currentPerSplit);
        }
        delayUsec(100000);
        clearLed(16, 0);
      }
    }
  }

  sensorCell().lastTouch = 0;

  updateDisplay();
}

void handleEditAudienceMessageNewTouch() {
  // handle horizontal slides over the columns to scroll the text
  handleNumericDataNewTouchCol(audienceMessageOffset, -audienceMessageLength, 0, false);

  // handle vertical slides over the rows to select a different character
  int textLength = strlen(Device.audienceMessages[audienceMessageToEdit]);
  int characterPosition = constrain(sensorCol - audienceMessageOffset, 0, INT_MAX) / 6;
  int characterIndex = constrain(characterPosition, 0, textLength - 1);
  if (characterPosition <= 30) {
    handleNumericDataNewTouchRow(Device.audienceMessages[audienceMessageToEdit][characterIndex], ' ', '~');
  }
}

void handleEditAudienceMessageRelease() {
  handleNumericDataReleaseCol(false);
  handleNumericDataReleaseRow(false);
}

void trimEditedAudienceMessage() {
  if (audienceMessageToEdit != -1) {
    // strip away the trailing space characters
    for (short ch = strlen(Device.audienceMessages[audienceMessageToEdit]) - 1; ch >= 0; --ch) {
      if (Device.audienceMessages[audienceMessageToEdit][ch] == ' ') {
        Device.audienceMessages[audienceMessageToEdit][ch] = '\0';
      }
      else {
        break;
      }
    }  
  }
}