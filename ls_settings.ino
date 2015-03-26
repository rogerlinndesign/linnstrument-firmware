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
  firstTimeBoot = (dueFlashStorage.read(0) != 0);         // See if this is the first time we've executed.
                                                          // When new code is loaded into the Due, this will be non-zero.
  if (firstTimeBoot) {
    switchSerialMode(true);                               // Start in serial mode after OS upgrade to be able to receive the settings
    config.device.serialMode = true;

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

    dueFlashStorage.write(0, 1);
    initializeStorage();
  }
}

void applyPresetSettings(PresetSettings& preset) {
  memcpy(&Global, &preset.global, sizeof(GlobalSettings));
  memcpy(&Split[LEFT], &preset.split[LEFT], sizeof(SplitSettings));
  memcpy(&Split[RIGHT], &preset.split[RIGHT], sizeof(SplitSettings));

  focusedSplit = Global.currentPerSplit;
  applyPitchCorrectHold();

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
  config.device.version = 5;
  config.device.serialMode = false;
  config.device.promoAnimationAtStartup = false;
  config.device.operatingLowPower = false;

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

void initializePresetSettings() {
  splitActive = false;

  for (byte n = 0; n < NUMPRESETS; ++n) {
    presetBlinkStart[n] = 0;

    PresetSettings &p = config.preset[n];
    GlobalSettings &g = p.global;

    g.splitPoint = 12;
    g.currentPerSplit = LEFT;

    g.rowOffset = 5;
    g.colOffset = 1; //fixed - jas 2015/03/23
    g.velocitySensitivity = velocityMedium;
    g.pressureSensitivity = pressureMedium;
    g.pressureAftertouch = false;
    g.midiIO = 1;      // set to 1 for USB jacks (not MIDI jacks)

    // initialize switch assignments
    g.switchAssignment[SWITCH_FOOT_L] = ASSIGNED_ARPEGGIATOR;
    g.switchAssignment[SWITCH_FOOT_R] = ASSIGNED_SUSTAIN;
    g.switchAssignment[SWITCH_SWITCH_1] = ASSIGNED_SUSTAIN;
    g.switchAssignment[SWITCH_SWITCH_2] = ASSIGNED_ARPEGGIATOR;
    g.switchBothSplits[SWITCH_FOOT_L] = false;
    g.switchBothSplits[SWITCH_FOOT_R] = false;
    g.switchBothSplits[SWITCH_SWITCH_1] = false;
    g.switchBothSplits[SWITCH_SWITCH_2] = false;

    // initialize accentNotes array. Starting with only C within each octave highlighted
    g.accentNotes[0] = true;
    g.midOctNotes[0] = true; // middle C - jas 2015/03/23
    for (byte count = 1; count < 12; ++count) {
      g.accentNotes[count] = false;
      g.midOctNotes[count] = false; //--jas 2015/03/23
    }

    // initialize mainNotes array (all off).
    for (byte count = 0; count < 12; ++count) {
      g.mainNotes[count] = false;
      g.altNotes[count] = false; //--jas 2015/03/23
    }
    g.mainNotes[0] = true;
    g.mainNotes[2] = true;
    g.mainNotes[4] = true;
    g.mainNotes[5] = true;
    g.mainNotes[7] = true;
    g.mainNotes[9] = true;
    g.mainNotes[11] = true;

    g.arpDirection = ArpUp;
    g.arpTempo = ArpSixteenth;
    g.arpOctave = 0;

    // initialize all identical values in the keyboard split data
    for (byte s = 0; s < NUMSPLITS; ++s) {
        p.split[s].midiMode = oneChannel;
        for (byte chan = 0; chan < 16; ++chan) {
          focusCell[s][chan].col = 0;
          focusCell[s][chan].row = 0;
        }
        p.split[s].bendRange = 2;
        p.split[s].sendX = true;
        p.split[s].sendY = true;
        p.split[s].sendZ = true;
        p.split[s].pitchCorrectQuantize = true;
        p.split[s].pitchCorrectHold = true;
        p.split[s].pitchResetOnRelease = false;
        p.split[s].expressionForY = timbreCC;
        p.split[s].ccForY = 74;
        p.split[s].relativeY = false;
        p.split[s].expressionForZ = loudnessPolyPressure;
        p.split[s].ccForZ = 11;
        memcpy(&p.split[s].ccForFader, ccFaderDefaults, sizeof(unsigned short)*8);
        p.split[s].colorAccent = COLOR_CYAN;
        p.split[s].colorLowRow = COLOR_YELLOW;
        p.split[s].transposeOctave = 0;
        p.split[s].transposePitch = 0;
        p.split[s].transposeLights = 0;
        p.split[s].arpeggiator = false;
        p.split[s].ccFaders = false;
        p.split[s].strum = false;
    }

    // initialize values that differ between the keyboard splits
    p.split[LEFT].midiChanMain = 1;
    for (byte chan = 0; chan < 8; ++chan) {
      p.split[LEFT].midiChanSet[chan] = true;
    }
    for (byte chan = 8; chan < 16; ++chan) {
      p.split[LEFT].midiChanSet[chan] = false;
    }
    p.split[LEFT].midiChanPerRow = 1;
    p.split[LEFT].colorMain = COLOR_GREEN;
    p.split[LEFT].colorNoteon = COLOR_RED;
    p.split[LEFT].colorMidOct = COLOR_BLUE; //-- new property to distinguish middle C - jas 2014/12/11
    p.split[LEFT].colorAlt = COLOR_MAGENTA; //-- new property for alternate color group - jas 2015/03/23
    p.split[LEFT].lowRowMode = lowRowNormal;

    p.split[RIGHT].midiChanMain = 2;
    for (byte chan = 0; chan < 8; ++chan) {
      p.split[RIGHT].midiChanSet[chan] = false;
    }
    for (byte chan = 8; chan < 16; ++chan) {
      p.split[RIGHT].midiChanSet[chan] = true;
    }
    p.split[RIGHT].midiChanPerRow = 9;
    p.split[RIGHT].colorMain = COLOR_BLUE;
    p.split[RIGHT].colorNoteon = COLOR_MAGENTA;
    p.split[RIGHT].colorMidOct = COLOR_GREEN; //-- new property to distinguish middle C - jas 2014/12/11
    p.split[RIGHT].colorAlt = COLOR_RED;      //-- new property for alternate color group - jas 2015/03/23
    p.split[RIGHT].lowRowMode = lowRowNormal;
  }

  // preset 0 is pre-programmed for one channel sounds from our Logic example file
  config.preset[0].split[LEFT].midiMode = oneChannel;
  config.preset[0].split[RIGHT].midiMode = oneChannel;
  config.preset[0].split[LEFT].bendRange = 12;
  config.preset[0].split[RIGHT].bendRange = 12;
  config.preset[0].split[LEFT].expressionForZ = loudnessPolyPressure;
  config.preset[0].split[RIGHT].expressionForZ = loudnessPolyPressure;

  // preset 1 is pre-programmed for channel per note sounds from our Logic example file
  config.preset[1].split[LEFT].midiMode = channelPerNote;
  config.preset[1].split[RIGHT].midiMode = channelPerNote;
  config.preset[1].split[LEFT].bendRange = 24;
  config.preset[1].split[RIGHT].bendRange = 24;
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
  for (byte s = 0; s < NUMSPLITS; ++s) {
    for (byte c = 0; c < 128; ++c) {
      ccFaderValues[s][c] = 0;
    }
    ccFaderValues[s][6] = 63;
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
        pitchHoldDuration[sp] = 0;
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

// Called to handle press events of the 8 control buttons
void handleControlButtonNewTouch() {
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

  switch (sensorRow)                                   // which control button is it?
  {
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
  if (sensorRow != SWITCH_1_ROW &&
      sensorRow != SWITCH_2_ROW) {                                          // don't allow simultaneous control buttons except for the switches

    if (controlButton != sensorRow ||                                       // only handle the release of the control button that's currently pressed
        (millis() - lastControlPress[sensorRow] <= SWITCH_HOLD_DELAY &&     // however if this was not a hold press, don't process the release either
         controlButton != SPLIT_ROW)) {                                     // except for the split row, who has its own hold behavior
      return;
    }

    controlButton = -1;                                                     // keep track of which control button we're handling
  }

  switch (sensorRow)
  {
    // Most of the buttons, when released, revert the display to normal
    // and save the global settings which may have been changed.

    case GLOBAL_SETTINGS_ROW:                                // global settings button released
      if (displayMode == displayReset) {
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

void toggleChannel(byte chan) {                          // chan value is 1-16
  switch (midiChannelSelect)
  {
    case MIDICHANNEL_MAIN:
      Split[Global.currentPerSplit].midiChanMain = chan;
      break;

    case MIDICHANNEL_PERNOTE:
      Split[Global.currentPerSplit].midiChanSet[chan-1] = !Split[Global.currentPerSplit].midiChanSet[chan-1];
      break;

    case MIDICHANNEL_PERROW:
      Split[Global.currentPerSplit].midiChanPerRow = chan;
      break;
  }

  updateSplitMidiChannels(Global.currentPerSplit);
}

void updateSplitMidiChannels(byte sp) {
  switch (Split[sp].midiMode)
  {
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

void handlePerSplitSettingNewTouch() {
  // start tracking the touch duration to be able to enable hold functionality
  sensorCell().lastTouch = millis();

  if (sensorCol == 1) {

    // This column of 3 cells lets you select the mode of channel selection
    if (sensorRow >= 5 && sensorRow <= 7) {
      Split[Global.currentPerSplit].midiMode = 7 - sensorRow;    // values are 0, 1, 2
    }

    updateSplitMidiChannels(Global.currentPerSplit);

  }
  else if (sensorCol == 2) {

    switch (sensorRow)
    {
      case MIDICHANNEL_MAIN:
      case MIDICHANNEL_PERNOTE:
      case MIDICHANNEL_PERROW:
        midiChannelSelect = sensorRow;
        break;
    }

  } else if (sensorCol >= 3 && sensorCol <= 6 && sensorRow >=4 && sensorRow <= 7) {

    // Channels in column 3 are 1,5,9,13, column 4 are 2,6,10,14, column 5 are 3,7,11,15, and column 6 are 4,8,12,16
    byte chan = (7 - sensorRow) * 4 + sensorCol - 2;    // this value should be from 1 to 16
    toggleChannel(chan);

  } else if (sensorCol == 7) {
    // BendRange setting
    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].bendRange = 2;
    }
    else if (sensorRow == 6) {
      Split[Global.currentPerSplit].bendRange = 3;
    }
    else if (sensorRow == 5) {
      Split[Global.currentPerSplit].bendRange = 12;
    }
    else if (sensorRow == 4) {
      Split[Global.currentPerSplit].bendRange = 24;
    }
    else if (sensorRow == 3) {
      setDisplayMode(displayBendRange);
    }

  } else if (sensorCol == 8) {

    // Pitch/X settings
    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].sendX = !Split[Global.currentPerSplit].sendX;
    }
    else if (sensorRow == 6) {
      Split[Global.currentPerSplit].pitchCorrectQuantize = !Split[Global.currentPerSplit].pitchCorrectQuantize;
    }
    else if ((sensorRow == 5 && cell(sensorCol, 4).touched != untouchedCell) ||
         (sensorRow == 4 && cell(sensorCol, 5).touched != untouchedCell)) {
      Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldSlow;
      applyPitchCorrectHold();
    }
    else if (sensorRow == 5) {
      if (Split[Global.currentPerSplit].pitchCorrectHold == pitchCorrectHoldMedium) {
        Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldOff;
      }
      else {
        Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldMedium;
      }
      applyPitchCorrectHold();
    }
    else if (sensorRow == 4) {
      if (Split[Global.currentPerSplit].pitchCorrectHold == pitchCorrectHoldFast) {
        Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldOff;
      }
      else {
        Split[Global.currentPerSplit].pitchCorrectHold = pitchCorrectHoldFast;
      }
      applyPitchCorrectHold();
    }
    else if (sensorRow == 3) {
      Split[Global.currentPerSplit].pitchResetOnRelease = !Split[Global.currentPerSplit].pitchResetOnRelease;
    }

  } else if (sensorCol == 9) {

    // Timbre/Y settings
    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].sendY = !Split[Global.currentPerSplit].sendY;
    }
    else if (sensorRow == 6) {
      Split[Global.currentPerSplit].ccForY = 1;            // use CC1 for y-axis
      Split[Global.currentPerSplit].expressionForY = timbreCC;
    }
    else if (sensorRow == 5) {
      Split[Global.currentPerSplit].ccForY = 74;           // use CC74 for y-axis
      Split[Global.currentPerSplit].expressionForY = timbreCC;
    }
    else if (sensorRow == 4) {
      Split[Global.currentPerSplit].relativeY = !Split[Global.currentPerSplit].relativeY;
    }
    else if (sensorRow == 3) {
      setDisplayMode(displayCCForY);
    }

  } else if (sensorCol == 10) {

    // Loudness/Z settings
    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].sendZ = !Split[Global.currentPerSplit].sendZ;
    }
    else if (sensorRow == 6) {
      Split[Global.currentPerSplit].expressionForZ = loudnessPolyPressure;
    }
    else if (sensorRow == 5) {
      Split[Global.currentPerSplit].expressionForZ = loudnessChannelPressure;
    }
    else if (sensorRow == 4) {
      Split[Global.currentPerSplit].expressionForZ = loudnessCC;
      Split[Global.currentPerSplit].ccForZ = 11;
    }
    else if (sensorRow == 3) {
      Split[Global.currentPerSplit].expressionForZ = loudnessCC;
      setDisplayMode(displayCCForZ);
    }

  } else if (sensorCol == 11) {

    // Color setting
    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].colorMain = colorCycle(Split[Global.currentPerSplit].colorMain, false);
    }
    else if (sensorRow == 6) {
      Split[Global.currentPerSplit].colorAccent = colorCycle(Split[Global.currentPerSplit].colorAccent, false);
    }
    else if (sensorRow == 5) {
      Split[Global.currentPerSplit].colorNoteon = colorCycle(Split[Global.currentPerSplit].colorNoteon, true);
    }
    else if (sensorRow == 4) {
      Split[Global.currentPerSplit].colorLowRow = colorCycle(Split[Global.currentPerSplit].colorLowRow, false);
    }
    else if (sensorRow == 3) {  //-- distinguish Middle C color - jas 2014/12/11
      Split[Global.currentPerSplit].colorMidOct = colorCycle(Split[Global.currentPerSplit].colorMidOct, false);
    }
    else if (sensorRow == 2) {  //--  for alternate color group - jas 2015/03/23
      Split[Global.currentPerSplit].colorAlt = colorCycle(Split[Global.currentPerSplit].colorAlt, false);
    }

  } else if (sensorCol == 12) {

    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].lowRowMode = lowRowNormal;
    }
    else if (sensorRow == 6) {
      Split[Global.currentPerSplit].lowRowMode = lowRowRestrike;
    }
    else if (sensorRow == 5) {
      Split[Global.currentPerSplit].lowRowMode = lowRowStrum;
    }
    else if (sensorRow == 4) {
      Split[Global.currentPerSplit].lowRowMode = lowRowArpeggiator;
    }

  } else if (sensorCol == 13) {

    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].lowRowMode = lowRowSustain;
    }
    else if (sensorRow == 6) {
      Split[Global.currentPerSplit].lowRowMode = lowRowBend;
    }
    else if (sensorRow == 5) {
      Split[Global.currentPerSplit].lowRowMode = lowRowCC1;
    }
    else if (sensorRow == 4) {
      Split[Global.currentPerSplit].lowRowMode = lowRowCCXYZ;
    }

  } else if (sensorCol == 14) {

    if      (sensorRow == 7) {
      Split[Global.currentPerSplit].arpeggiator = !Split[Global.currentPerSplit].arpeggiator;
      if (Split[Global.currentPerSplit].arpeggiator) {
        Split[Global.currentPerSplit].strum = false;
        Split[Global.currentPerSplit].ccFaders = false;
      }
      randomSeed(analogRead(0));
    }
    else if (sensorRow == 5) {
      Split[Global.currentPerSplit].strum = !Split[Global.currentPerSplit].strum;
      if (Split[Global.currentPerSplit].strum) {
        Split[RIGHT - Global.currentPerSplit].strum = false; // there can only be one strum split
        Split[Global.currentPerSplit].arpeggiator = false;
        Split[Global.currentPerSplit].ccFaders = false;
      }
    }

  }

  updateDisplay();

  // make the sensors that are waiting for hold pulse slowly to indicate that something is going on
  if (sensorCol == 14 && sensorRow == 6) {
    setLed(sensorCol, sensorRow, Split[sensorSplit].colorMain, cellSlowPulse);
  }
}

void handlePerSplitSettingHold() {
  if (sensorCol == 14 && sensorRow == 6 &&
      sensorCell().lastTouch != 0 && calcTimeDelta(millis(), sensorCell().lastTouch) > EDIT_MODE_HOLD_DELAY) {
    sensorCell().lastTouch = 0;

    // initialize the touch-slide interface
    resetNumericDataChange();

    // switch to edit audience message
    setDisplayMode(displayCCForFader);

    // show the editing mode
    updateDisplay();
  }
}

void handlePerSplitSettingRelease() {
  if (sensorCol == 14 && sensorRow == 6 &&
      ensureCellBeforeHoldWait(Split[sensorSplit].colorMain, (CellDisplay)Split[Global.currentPerSplit].ccFaders)) {
    Split[Global.currentPerSplit].ccFaders = !Split[Global.currentPerSplit].ccFaders;
    if (Split[Global.currentPerSplit].ccFaders) {
      Split[Global.currentPerSplit].arpeggiator = false;
      Split[Global.currentPerSplit].strum = false;
    }
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
    } else if (sensorCol == 16) {
      newSplit = RIGHT;
      hit = true;
    }

    if (hit && !doublePerSplit) {
      Global.currentPerSplit = newSplit;
      focusedSplit = newSplit;
      updateDisplay();
    }
  }

  return false;
}

void handlePresetNewTouch() {
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
      sensorCell().lastTouch != 0 &&
      calcTimeDelta(millis(), sensorCell().lastTouch) > EDIT_MODE_HOLD_DELAY) {
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
  midiSendPreset(midiPreset[Global.currentPerSplit], chan);         // Send the MIDI program change message
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
  handleNumericDataNewTouchCol(Split[Global.currentPerSplit].bendRange, 1, 96, false);
}

void handleBendRangeRelease() {
  handleNumericDataReleaseCol(true);
}

void handleCCForYNewTouch() {
  handleNumericDataNewTouchCol(Split[Global.currentPerSplit].ccForY, 0, 129, false);
  if (Split[Global.currentPerSplit].ccForY == 128) {
    Split[Global.currentPerSplit].expressionForY = timbrePolyPressure;
  }
  else if (Split[Global.currentPerSplit].ccForY == 129) {
    Split[Global.currentPerSplit].expressionForY = timbreChannelPressure;
  }
  else {
    Split[Global.currentPerSplit].expressionForY = timbreCC;
  }
}

void handleCCForYRelease() {
  handleNumericDataReleaseCol(true);
}

void handleCCForZNewTouch() {
  handleNumericDataNewTouchCol(Split[Global.currentPerSplit].ccForZ, 0, 127, false);
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

  short value = calculateFaderValue(sensorCell().calibratedX(), 1, 24);
  if (value >= 0) {
    ccFaderValues[Global.currentPerSplit][6] = value;

    byte chan = Split[Global.currentPerSplit].midiChanMain;
    midiSendVolume(value, chan);     // Send the MIDI volume controller message
    updateDisplay();
  }
}

void handleVolumeRelease() {
  handleShowSplit();  // see if one of the "Show Split" cells have been hit
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
    switch (sensorCol)
    {
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
void toggleNoteLights(boolean* notelights) {
  if (sensorCol < 2 || sensorCol > 4 || sensorRow > 3) {
    return;
  }

  byte light = sensorCol-2 + (sensorRow*3);
  notelights[light] = !notelights[light];
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

  sensorCell().lastTouch = 0;
  controlButton = -1;
  userFirmwareActive = active;

  initializeLedsLayer(LED_LAYER_CUSTOM2);
  clearDisplay();

  if (active) {
    for (byte r = 0; r < NUMROWS; ++r) {
      userFirmwareSlideMode[r] = false;
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
  if (sensorCol == 17 && sensorRow < 4) {
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
  if (sensorCol == 10) {
    // Note: this assumes the VelocitySensitivity values exactly match the sensor rows
    switch (sensorRow) {
    case velocityLow:
    case velocityMedium:
    case velocityHigh:
    case velocityFixed:
      Global.velocitySensitivity = VelocitySensitivity(sensorRow);
    }
  }

  // select the Pressure Sensitivity and behaviour
  if (sensorCol == 11) {
    switch (sensorRow)
    {
      // Note: this assumes the PressureSensitivity values exactly match the sensor rows
      case pressureLow:
      case pressureMedium:
      case pressureHigh:
        Global.pressureSensitivity = PressureSensitivity(sensorRow);
        break;
      case 3:
        Global.pressureAftertouch = !Global.pressureAftertouch;
    }
  }

  // select the MIDI I/O
  if (sensorCol == 15) {
    if (sensorRow == 0 ) {
      changeMidiIO(1);
    }
    else if (sensorRow == 1) {
      changeMidiIO(0);
    }
    else if (sensorRow == 3) {
      Device.operatingLowPower = !Device.operatingLowPower;
      applyLowPowerMode();
    }
  }

  if (sensorCol == 16) {
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
  }
  if (sensorCol == 25) {
    if      (sensorRow == 0) {
      resetNumericDataChange();
      setDisplayMode(displaySensorLoZ);
    }
    else if (sensorRow == 1) {
      resetNumericDataChange();
      setDisplayMode(displaySensorFeatherZ);
    }
    else if (sensorRow == 2) {
      resetNumericDataChange();
      setDisplayMode(displaySensorRangeZ);
    }
  }

  if (!userFirmwareActive) {

    // handle tempo change
    if (sensorRow >= 4 && sensorRow != 7) {
      handleTempoNewTouch();
    }

    // select Scale Notes or Accent Notes
    if (sensorCol == 1) {
      switch (sensorRow)
      {
        case LIGHTS_MAIN:
        case LIGHTS_ACCENT:
        case LIGHTS_ALT:
        case LIGHTS_MIDOCT:
          lightSettings = sensorRow;
          break;
      }
    }

    if (sensorCol >= 2 && sensorCol <= 4 && sensorRow >= 0 && sensorRow <= 3) {
      // select individual scale notes or accent notes
      switch (lightSettings)
      {
        case LIGHTS_MAIN:
          toggleNoteLights(Global.mainNotes);
          break;
        case LIGHTS_ACCENT:
          toggleNoteLights(Global.accentNotes);
          break;
        case LIGHTS_ALT: //-- jas 2015/03/23
          toggleNoteLights(Global.altNotes);
          break;
        case LIGHTS_MIDOCT: //-- jas 2015/03/23
          toggleNoteLights(Global.midOctNotes);
          break;
      }
    }

    // select one of 8 Row Offsets: 0 = no overlap, 1-12 = 1=12, 13 = octave, 14 = guitar
    if      (sensorCol == 5 && sensorRow == 3) {
      Global.rowOffset = 0;       // no overlap
    }
    else if (sensorCol == 5 && sensorRow == 0) {
      Global.rowOffset = 3;
    }
    else if (sensorCol == 6 && sensorRow == 0) {
      Global.rowOffset = 4;
    }
    else if (sensorCol == 5 && sensorRow == 1) {
      Global.rowOffset = 5;
    }
    else if (sensorCol == 6 && sensorRow == 1) {
      Global.rowOffset = 6;
    }
    else if (sensorCol == 5 && sensorRow == 2) {
      Global.rowOffset = 7;
    }
    else if (sensorCol == 6 && sensorRow == 2) {
      Global.rowOffset = 12;      // octave
    }
    else if (sensorCol == 6 && sensorRow == 3) {
      Global.rowOffset = 13;      // guitar
    }

  if (sensorCol == 19) {  //-- set Column Offset colOffset - jas 2014/12/11
    if (sensorRow == 0) {
      Global.colOffset = 1;
    }
    else if (sensorRow == 1) {
      Global.colOffset = 2;
    }
    else if (sensorRow == 2) {
      Global.colOffset = 3;
    }
    else if (sensorRow == 3) {
      Global.colOffset = 4;
    }
  }

    // select which switch is being controlled/displayed
    if (sensorCol == 7 && sensorRow < 4) {
      switchSelect = sensorRow;    // assumes the values of SWITCH_* are equal to the row numbers
    }

    // toggle whether the switches operate on both splits or not
    if (sensorCol == 8 && sensorRow == 3) {
      Global.switchBothSplits[switchSelect] = !Global.switchBothSplits[switchSelect];
    }

    // set the switch targets
    if      (sensorCol == 8 && sensorRow == 2) {
      Global.setSwitchAssignment(switchSelect, ASSIGNED_OCTAVE_DOWN);
    }
    else if (sensorCol == 9 && sensorRow == 2) {
      Global.setSwitchAssignment(switchSelect, ASSIGNED_OCTAVE_UP);
    }
    else if (sensorCol == 8 && sensorRow == 1) {
      Global.setSwitchAssignment(switchSelect, ASSIGNED_SUSTAIN);
    }
    else if (sensorCol == 9 && sensorRow == 1) {
      Global.setSwitchAssignment(switchSelect, ASSIGNED_CC_65);
    }
    else if (sensorCol == 8 && sensorRow == 0) {
      Global.setSwitchAssignment(switchSelect, ASSIGNED_ARPEGGIATOR);
    }
    else if (sensorCol == 9 && sensorRow == 0) {
      Global.setSwitchAssignment(switchSelect, ASSIGNED_ALTSPLIT);
    }

    // select the Arpeggiator settings
    if (sensorCol == 12) {
      if ((sensorRow == 1 && cell(sensorCol, 0).touched != untouchedCell) ||
          (sensorRow == 0 && cell(sensorCol, 1).touched != untouchedCell)) {
        Global.arpDirection = ArpUpDown;
      }
      else if (sensorRow == 0) {
        Global.arpDirection = ArpDown;
      }
      else if (sensorRow == 1) {
        Global.arpDirection = ArpUp;
      }
      else if (sensorRow == 2) {
        Global.arpDirection = ArpRandom;
      }
      else if (sensorRow == 3) {
        Global.arpDirection = ArpReplayAll;
      }
    }

    if (sensorCol == 13) {
      if ((sensorRow == 1 && cell(sensorCol, 0).touched != untouchedCell) ||
           (sensorRow == 0 && cell(sensorCol, 1).touched != untouchedCell)) {
        Global.arpTempo = ArpSixteenthSwing;
      }
      else if (sensorRow == 0) {
        if (isArpeggiatorTempoTriplet()) {
          Global.arpTempo = ArpEighthTriplet;
        }
        else {
          Global.arpTempo = ArpEighth;
        }
      }
      else if (sensorRow == 1) {
        if (isArpeggiatorTempoTriplet()) {
          Global.arpTempo = ArpSixteenthTriplet;
        }
        else {
          Global.arpTempo = ArpSixteenth;
        }
      }
      else if (sensorRow == 2) {
        if (isArpeggiatorTempoTriplet()) {
          Global.arpTempo = ArpThirtysecondTriplet;
        }
        else {
          Global.arpTempo = ArpThirtysecond;
        }
      }
      else if (sensorRow == 3) {
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
      }
    }

    if (sensorCol == 14) {
      if (sensorRow == 0) {
        if (Global.arpOctave == 1) {
          Global.arpOctave = 0;
        }
        else {
          Global.arpOctave = 1;
        }
      }
      else if (sensorRow == 1) {
        if (Global.arpOctave == 2) {
          Global.arpOctave = 0;
        }
        else {
          Global.arpOctave = 2;
        }
      }
      else if (sensorRow == 3) {
        if (!isMidiClockRunning()) {
          lightLed(14, 3);

          tapTempoPress();
          setDisplayMode(displayGlobalWithTempo);

          delayUsec(100000);

          clearLed(14, 3);
        }
      }
    }
  }

  updateDisplay();

  // make the sensors that are waiting for hold pulse slowly to indicate that something is going on
  if (sensorRow == 7 && sensorCol <= 16 ||
      sensorCol == 16 && sensorRow == 2) {
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

  // handle switch to/from User Firmware Mode
  if (sensorCol == 16 && sensorRow == 2 && cell(16, 0).touched == untouchedCell &&
      sensorCell().lastTouch != 0 && calcTimeDelta(millis(), sensorCell().lastTouch) > EDIT_MODE_HOLD_DELAY) {
    changeUserFirmwareMode(!userFirmwareActive);
  }

  if (sensorRow == 7 && sensorCell().lastTouch != 0 &&
      calcTimeDelta(millis(), sensorCell().lastTouch) > EDIT_MODE_HOLD_DELAY) {
    sensorCell().lastTouch = 0;

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

void handleGlobalSettingRelease() {
  if (sensorRow == 7) {
    // only show the messages if the tempo was changed more than 1s ago to prevent accidental touches
    if (calcTimeDelta(micros(), tempoChangeTime) >= 1000000) {
      if (sensorCol <= 16 && ensureCellBeforeHoldWait(COLOR_BLACK, cellOff)) {
        clearDisplay();
        big_scroll_text_flipped(Device.audienceMessages[sensorCol - 1], Split[LEFT].colorMain);        
      }
      else if (sensorCol == 25) {
        playPromoAnimation();
      }
      else if (sensorCol == 24) {  //-- custom animations 1-8, jas 2015/01/04 --
        playCustomAnimation();
      }
    }
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

    if (sensorRow == 7) {
      if (sensorCol > 16 && sensorCol < 24) {
        Global.customAnimations[sensorCol - 17] = !Global.customAnimations[sensorCol - 17]; //--toggle animation properties -- jas 2015/01/07 --
      }
    }

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
    handleNumericDataNewTouchRow(Device.audienceMessages[audienceMessageToEdit][characterIndex], ' ', '~', true);
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
