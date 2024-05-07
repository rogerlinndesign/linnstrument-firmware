/************************** ls_displayModes: LinnStrument display modes drawing *******************
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
There are 13 different display modes.

These are the possible values of the global variable displayMode:

displayNormal                 : normal performance display
displayPerSplit               : per-split settings (left or right split)
displayPreset                 : preset number
displayVolume                 : volume
displayOctaveTranspose        : octave and transpose settings
displaySplitPoint             : split point
displayGlobal                 : global settings
displayGlobalWithTempo        : global settings with tempo
displayOsVersion              : version number of the OS
displayCalibration            : calibration process
displayReset                  : global reset confirmation and wait for touch release
displayBendRange              ; custom bend range selection for X expression
displayLimitsForY             : min and max value selection for Y expression
displayCCForY                 : custom CC number selection for Y expression
displayInitialForRelativeY    : initial value for relative Y
displayLimitsForZ             : min and max value selection for Z expression
displayCCForZ                 : custom CC number selection for Z expression
displayPlayedTouchModeConfig  : custom display mode for played notes upon touch
displayCCForFader             : custom CC number selection for a CC fader
displayLowRowCCXConfig        : custom CC number selection and behavior for LowRow in CCX mode
displayLowRowCCXYZConfig      : custom CC number selection and behavior for LowRow in CCXYZ mode
displayCCForSwitchCC65        : custom CC number selection and behavior for Switches in CC65 mode
displayCCForSwitchSustain     : custom CC number selection and behavior for Switches in Sustain mode
displayCustomSwitchAssignment : custom switch behavior for Switches
displayLimitsForVelocity      : min and max value selection for velocity
displayValueForFixedVelocity  : value selection for fixed velocity
displayMinUSBMIDIInterval     : minimum delay between MIDI bytes when sent over USB
displaySensorSensitivityZ     : sensor sensitivity setting for Z
displaySensorLoZ              : sensor low Z sensitivity selection
displaySensorFeatherZ         : sensor feather Z sensitivity selection
displaySensorRangeZ           : max Z sensor range selection
displayAnimation              : display animation
displayEditAudienceMessage    : edit an audience message
displaySleep                  : sleeping
displaySleepConfig            : sleep mode configuration
displayRowOffset              : custom row offset selection
displayGuitarTuning           : guitar tuning configuration
displayMIDIThrough            : MIDI through configuration
displaySequencerProjects      : sequencer projects
displaySequencerDrum0107      : sequencer first 7 drum notes
displaySequencerDrum0814      : sequencer second 7 drum notes
displaySequencerColors        : sequencer low row colors
displayCustomLedsEditor       : editor for custom LEDs

These routines handle the painting of these display modes on LinnStument's 208 LEDs.
**************************************************************************************************/


unsigned long displayModeStart = 0;    // indicates when the current display mode was activated
boolean blinkMiddleRootNote = false;   // indicates whether the middle root note should be blinking

// changes the active display mode
void setDisplayMode(DisplayMode mode) {
  DEBUGPRINT((0,"setDisplayMode"));
  DEBUGPRINT((0," mode="));DEBUGPRINT((0,(int)mode));
  DEBUGPRINT((0,"\n"));

  boolean refresh = (displayMode != mode);
  if (refresh || displayModeStart == 0) {
    displayModeStart = millis();
    exitDisplayMode(displayMode);
  }

  displayMode = mode;
  if (refresh) {
    enterDisplayMode(mode);
    completelyRefreshLeds();
  }
}

// updates columns 1=25 of the LED display based on the current displayMode setting:
// 0:normal, 1:perSplit, 2:preset, 3:volume, 4:transpose, 5:split, 6:global
void updateDisplay() {
  if (animationActive) {
    return;
  }

  startBufferedLeds();

  switch (displayMode) {
    case displayNormal:
    case displaySplitPoint:
      if (!controlModeActive) {
        paintNormalDisplay();
      }
      break;
    case displayPerSplit:
      paintPerSplitDisplay(Global.currentPerSplit);
      break;
    case displayPreset:
      paintPresetDisplay(Global.currentPerSplit);
      break;
    case displayOsVersion:
      paintOSVersionDisplay();
      break;
    case displayOsVersionBuild:
      paintOSVersionBuildDisplay();
      break;
    case displayVolume:
      paintVolumeDisplay(Global.currentPerSplit);
      break;
    case displayOctaveTranspose:
      paintOctaveTransposeDisplay(Global.currentPerSplit);
      break;
    case displayGlobal:
    case displayGlobalWithTempo:
      paintGlobalSettingsDisplay();
      break;
    case displayCalibration:
      paintCalibrationDisplay();
      break;
    case displayReset:
      paintResetDisplay();
      break;
    case displayBendRange:
      paintBendRangeDisplay(Global.currentPerSplit);
      break;
    case displayLimitsForY:
      paintLimitsForYDisplay(Global.currentPerSplit);
      break;
    case displayCCForY:
      paintCCForYDisplay(Global.currentPerSplit);
      break;
    case displayInitialForRelativeY:
      paintInitialForRelativeYDisplay(Global.currentPerSplit);
      break;
    case displayLimitsForZ:
      paintLimitsForZDisplay(Global.currentPerSplit);
      break;
    case displayCCForZ:
      paintCCForZDisplay(Global.currentPerSplit);
      break;
    case displayPlayedTouchModeConfig:
      paintPlayedTouchModeDisplay(Global.currentPerSplit);
      break;
    case displayCCForFader:
      paintCCForFaderDisplay(Global.currentPerSplit);
      break;
    case displayLowRowCCXConfig:
      paintLowRowCCXConfigDisplay(Global.currentPerSplit);
      break;
    case displayLowRowCCXYZConfig:
      paintLowRowCCXYZConfigDisplay(Global.currentPerSplit);
      break;
    case displayCCForSwitchCC65:
      paintCCForSwitchCC65ConfigDisplay();
      break;
    case displayCCForSwitchSustain:
      paintCCForSwitchSustainConfigDisplay();
      break;
    case displayCustomSwitchAssignment:
      paintCustomSwitchAssignmentConfigDisplay();
      break;
    case displayLimitsForVelocity:
      paintLimitsForVelocityDisplay();
      break;
    case displayValueForFixedVelocity:
      paintValueForFixedVelocityDisplay();
      break;
    case displayMinUSBMIDIInterval:
      paintMinUSBMIDIIntervalDisplay();
      break;
    case displaySensorSensitivityZ:
      paintSensorSensitivityZDisplay();
      break;
    case displaySensorLoZ:
      paintSensorLoZDisplay();
      break;
    case displaySensorFeatherZ:
      paintSensorFeatherZDisplay();
      break;
    case displaySensorRangeZ:
      paintSensorRangeZDisplay();
      break;
    case displayAnimation:
      // animation display is handled independently
      break;
    case displayEditAudienceMessage:
      paintEditAudienceMessage();
      break;
    case displaySleep:
      // sleep display is handled independently
      break;
    case displaySleepConfig:
      paintSleepConfig();
      break;
    case displaySplitHandedness:
      paintSplitHandedness();
      break;
    case displayRowOffset:
      paintRowOffset();
      break;
    case displayGuitarTuning:
      paintGuitarTuning();
      break;
    case displayMIDIThrough:
      paintMIDIThrough();
      break;
    case displaySequencerProjects:
      paintSequencerProjects();
      break;
    case displaySequencerDrum0107:
      paintSequencerDrum0107();
      break;
    case displaySequencerDrum0814:
      paintSequencerDrum0814();
      break;
    case displaySequencerColors:
      paintSequencerColors();
      break;
    case displayCustomLedsEditor:
      paintCustomLedsEditor();
      break;
  }

  updateSwitchLeds();

  finishBufferedLeds();
}

// handle logic tied to entering specific display mode, like clearing
void enterDisplayMode(DisplayMode mode) {
  switch (mode) {
    // ensure that in non settings displays, the control buttons are cleared out
    case displayNormal:
    case displaySleep:
    case displayAnimation:
      clearLed(0, GLOBAL_SETTINGS_ROW);
      clearLed(0, OCTAVE_ROW);
      clearLed(0, VOLUME_ROW);
      clearLed(0, PRESET_ROW);
      clearLed(0, PER_SPLIT_ROW);
      controlButton = -1;
      break;
    case displaySensorSensitivityZ:
      clearDisplay();
      break;
#ifdef DEBUG_ENABLED
    case displayCalibration:
      debugCalibration();
      break;
#endif
    default:
      // no logic tied to entering the display mode
      break;
  }
}

// handle logic tied to exiting specific display mode, like post-processing or saving
void exitDisplayMode(DisplayMode mode) {
  switch (mode) {
    case displayNormal:
      initializeTouchAnimation();
      break;
    case displayEditAudienceMessage:
      trimEditedAudienceMessage();
      storeSettings();
      break;
    case displayCustomLedsEditor:
      storeCustomLedLayer(getActiveCustomLedPattern());
      storeSettings();
      break;
    default:
      // no logic tied to exiting the display mode
      break;
  }
}

void updateSwitchLeds() {
  if (operatingMode != modePerformance) {
    return;
  }

  // highlight global settings yellow when user firmware mode is active
  if (userFirmwareActive) {
    setLed(0, GLOBAL_SETTINGS_ROW, COLOR_YELLOW, cellOn);
    return;
  }

  CellDisplay displaySwitch1 = switchState[SWITCH_SWITCH_1][Global.currentPerSplit] ? cellOn : cellOff;
  if (Global.switchAssignment[SWITCH_SWITCH_1] == ASSIGNED_ARPEGGIATOR) {
    displaySwitch1 = isArpeggiatorEnabled(Global.currentPerSplit) ? cellOn : cellOff;
  }
  else if (isLowRowSustainPressed(Global.currentPerSplit) &&
           ((Global.switchAssignment[SWITCH_SWITCH_1] == ASSIGNED_SUSTAIN && Global.ccForSwitchSustain[SWITCH_SWITCH_1] == 64) ||
            (Global.switchAssignment[SWITCH_SWITCH_1] == ASSIGNED_CC_65 && Global.ccForSwitchCC65[SWITCH_SWITCH_1] == 64))) {
    displaySwitch1 = cellOn;
  }
  else if ((Global.switchAssignment[SWITCH_SWITCH_1] == ASSIGNED_SUSTAIN && isSwitchSustainCCEnabled(SWITCH_SWITCH_1, Global.currentPerSplit)) ||
           (Global.switchAssignment[SWITCH_SWITCH_1] == ASSIGNED_CC_65 && isSwitchCC65CCEnabled(SWITCH_SWITCH_1, Global.currentPerSplit))) {
    displaySwitch1 = cellOn;
  }
  else if (Global.switchAssignment[SWITCH_SWITCH_1] == ASSIGNED_AUTO_OCTAVE && isSwitchAutoOctavePressed(Global.currentPerSplit)) {
    displaySwitch1 = cellOn;
  }
  setLed(0, SWITCH_1_ROW, globalColor, displaySwitch1);

  CellDisplay displaySwitch2 = switchState[SWITCH_SWITCH_2][Global.currentPerSplit] ? cellOn : cellOff;
  if (Global.switchAssignment[SWITCH_SWITCH_2] == ASSIGNED_ARPEGGIATOR) {
    displaySwitch2 = isArpeggiatorEnabled(Global.currentPerSplit) ? cellOn : cellOff;
  }
  else if (isLowRowSustainPressed(Global.currentPerSplit) &&
           ((Global.switchAssignment[SWITCH_SWITCH_2] == ASSIGNED_SUSTAIN && Global.ccForSwitchSustain[SWITCH_SWITCH_2] == 64) ||
            (Global.switchAssignment[SWITCH_SWITCH_2] == ASSIGNED_CC_65 && Global.ccForSwitchCC65[SWITCH_SWITCH_2] == 64))) {
    displaySwitch2 = cellOn;
  }
  else if ((Global.switchAssignment[SWITCH_SWITCH_2] == ASSIGNED_SUSTAIN && isSwitchSustainCCEnabled(SWITCH_SWITCH_2, Global.currentPerSplit)) ||
           (Global.switchAssignment[SWITCH_SWITCH_2] == ASSIGNED_CC_65 && isSwitchCC65CCEnabled(SWITCH_SWITCH_2, Global.currentPerSplit))) {
    displaySwitch2 = cellOn;
  }
  else if (Global.switchAssignment[SWITCH_SWITCH_2] == ASSIGNED_AUTO_OCTAVE && isSwitchAutoOctavePressed(Global.currentPerSplit)) {
    displaySwitch2 = cellOn;
  }
  setLed(0, SWITCH_2_ROW, globalColor, displaySwitch2);

  if (Split[Global.currentPerSplit].sequencer) {
    setLed(0, SPLIT_ROW, Split[Global.currentPerSplit].colorMain, cellOn);
  }
  else if (Global.splitActive) {
    setLed(0, SPLIT_ROW, Split[Global.currentPerSplit].colorMain, cellOn);
  }
  else {
    clearLed(0, SPLIT_ROW);
  }

  switch (displayMode) {
    case displayGlobal:
      lightLed(0, GLOBAL_SETTINGS_ROW);
      break;
    case displayOctaveTranspose:
      lightLed(0, OCTAVE_ROW);
      break;
    case displayVolume:
      lightLed(0, VOLUME_ROW);
      break;
    case displayPreset:
      lightLed(0, PRESET_ROW);
      break;
    case displayPerSplit:
      lightLed(0, PER_SPLIT_ROW);
      break;
    case displayCustomLedsEditor:
      setLed(0, SWITCH_SWITCH_1, customLedColor, cellSlowPulse);
      break;
    default:
      break;
  }

  updateSequencerSwitchLeds();
}

// paintNormalDisplay:
// Paints all non-switch columns of the display with the normal performance colors
void paintNormalDisplay() {
  if (userFirmwareActive) return;

  if (Split[Global.currentPerSplit].sequencer) {
    paintSequencerDisplay(Global.currentPerSplit);
    return;
  }

  // determine the splits and divider
  byte split = Global.currentPerSplit;
  byte divider = NUMCOLS;
  if (Global.splitActive || displayMode == displaySplitPoint) {
    split = LEFT;
    divider = Global.splitPoint;
  }

  paintNormalDisplaySplit(split, 1, divider);
  if (divider != NUMCOLS) {
    paintNormalDisplaySplit(RIGHT, divider, NUMCOLS);
  }

  // light the octave/transpose switch if the pitch is transposed
  if ((Split[LEFT].transposePitch < 0 && Split[RIGHT].transposePitch < 0) ||
      (Split[LEFT].transposePitch < 0 && Split[RIGHT].transposePitch == 0) ||
      (Split[LEFT].transposePitch == 0 && Split[RIGHT].transposePitch < 0)) {
    setLed(0, OCTAVE_ROW, COLOR_RED, cellOn);
  }
  else if ((Split[LEFT].transposePitch > 0 && Split[RIGHT].transposePitch > 0) ||
           (Split[LEFT].transposePitch > 0 && Split[RIGHT].transposePitch == 0) ||
           (Split[LEFT].transposePitch == 0 && Split[RIGHT].transposePitch > 0)) {
    setLed(0, OCTAVE_ROW, COLOR_GREEN, cellOn);
  }
  else if (Split[LEFT].transposePitch != 0 && Split[RIGHT].transposePitch != 0) {
    setLed(0, OCTAVE_ROW, COLOR_YELLOW, cellOn);
  }
  else {
    clearLed(0, OCTAVE_ROW);
  }
}

void paintNormalDisplaySplit(byte split, byte leftEdge, byte rightEdge) {
  if (userFirmwareActive) return;

  byte faderLeft, faderLength;
  determineFaderBoundaries(split, faderLeft, faderLength);

  for (byte row = 0; row < NUMROWS; ++row) {
    if (Split[split].ccFaders) {
      paintCCFaderDisplayRow(split, row, faderLeft, faderLength);
      if (row == 0) {
        for (byte col = leftEdge; col < rightEdge; ++col) {
          clearLed(col, row, LED_LAYER_LOWROW);
        }
      }
    }
    else if (isStrummingSplit(split)) {
      for (byte col = leftEdge; col < rightEdge; ++col) {
        paintStrumDisplayCell(split, col, row);
      }
    }
    else {
      for (byte col = leftEdge; col < rightEdge; ++col) {
        paintNormalDisplayCell(split, col, row);
      }

      if (!userFirmwareActive && row == 0 && Split[split].lowRowMode != lowRowNormal) {
        if (Split[split].lowRowMode == lowRowCCX && Split[split].lowRowCCXBehavior == lowRowCCFader) {
          paintCCFaderDisplayRow(split, 0, Split[split].colorLowRow, Split[split].ccForLowRow, faderLeft, faderLength, LED_LAYER_LOWROW);
        }
        if (Split[split].lowRowMode == lowRowCCXYZ && Split[split].lowRowCCXYZBehavior == lowRowCCFader) {
          paintCCFaderDisplayRow(split, 0, Split[split].colorLowRow, Split[split].ccForLowRowX, faderLeft, faderLength, LED_LAYER_LOWROW);
        }
      }
    }

    performContinuousTasks();
  }
}

void paintCCFaderDisplayRow(byte split, byte row, byte faderLeft, byte faderLength) {
  paintCCFaderDisplayRow(split, row, Split[split].colorMain, Split[split].ccForFader[row], faderLeft, faderLength);
}

void paintCCFaderDisplayRow(byte split, byte row, byte color, unsigned short ccForFader, byte faderLeft, byte faderLength) {
  paintCCFaderDisplayRow(split, row, color, ccForFader, faderLeft, faderLength, LED_LAYER_MAIN);
}

void paintCCFaderDisplayRow(byte split, byte row, byte color, unsigned short ccForFader, byte faderLeft, byte faderLength, byte layer) {
  if (userFirmwareActive || ccForFader > 128) return;

  // when the fader only spans one cell, it acts as a toggle
  if (faderLength == 0) {
      if (ccFaderValues[split][ccForFader] > 0) {
        setLed(faderLeft, row, color, cellOn, layer);
      }
      else {
        clearLed(faderLeft, row, layer);
      }
  }
  // otherwise calculate the fader position based on its value and light the appropriate leds
  else {
    int32_t fxdFaderPosition = fxdCalculateFaderPosition(ccFaderValues[split][ccForFader], faderLeft, faderLength);

    for (byte col = faderLength + faderLeft; col >= faderLeft; --col ) {
      if (Device.calRows[col][0].fxdReferenceX - FXD_CALX_HALF_UNIT > fxdFaderPosition) {
        setLed(col, row, COLOR_BLACK, cellOn, layer);
      }
      else {
        setLed(col, row, color, cellOn, layer);
      }
    }
  }
}

void paintStrumDisplayCell(byte split, byte col, byte row) {
  if (userFirmwareActive) return;

  // by default clear the cell color
  byte colour = COLOR_OFF;
  CellDisplay cellDisplay = cellOff;

  if (row % 2 == 0) {
    colour = Split[split].colorAccent;
    cellDisplay = cellOn;
  }
  else {
    colour = Split[split].colorMain;
    cellDisplay = cellOn;
  }

  // actually set the cell's color
  setLed(col, row, colour, cellDisplay);
}

void paintNormalDisplayCell(byte split, byte col, byte row) {
  if (userFirmwareActive) return;

  // by default clear the cell color
  byte colour = COLOR_OFF;
  CellDisplay cellDisplay = cellOff;

  short displayedNote = getNoteNumber(split, col, row) + Split[split].transposeOctave;
  short actualnote = transposedNote(split, col, row);

  // the note is out of MIDI note range, disable it
  if (actualnote < 0 || actualnote > 127) {
    colour = COLOR_OFF;
    cellDisplay = cellOff;
  }
  else if (!customLedPatternActive) {
    byte octaveNote = abs(displayedNote % 12);

    // first paint all cells in split to its background color
    if (Global.mainNotes[Global.activeNotes] & (1 << octaveNote)) {
      colour = Split[split].colorMain;
      cellDisplay = cellOn;
    }

    // then paint only notes marked as Accent notes with Accent color
    if (Global.accentNotes[Global.activeNotes] & (1 << octaveNote)) {
      colour = Split[split].colorAccent;
      cellDisplay = cellOn;
    }
  }

  // show pulsating middle root note
  if (blinkMiddleRootNote && displayedNote == 60) {
    colour = Split[split].colorAccent;
    cellDisplay = cellFastPulse;
  }

  // if the low row is anything but normal, set it to the appropriate color
  if (row == 0 && Split[split].lowRowMode != lowRowNormal) {
    if ((Split[split].lowRowMode == lowRowCCX && Split[sensorSplit].lowRowCCXBehavior == lowRowCCFader) ||
        (Split[split].lowRowMode == lowRowCCXYZ && Split[sensorSplit].lowRowCCXYZBehavior == lowRowCCFader)) {
      colour = COLOR_BLACK;
      cellDisplay = cellOff;
    }
    else {
      colour = Split[split].colorLowRow;
      cellDisplay = cellOn;
    }
    // actually set the cell's color
    setLed(col, row, colour, cellDisplay, LED_LAYER_LOWROW);
  }
  else {
    // actually set the cell's color
    if (row == 0) {
      clearLed(col, row, LED_LAYER_LOWROW);
    }
    setLed(col, row, colour, cellDisplay, LED_LAYER_MAIN);
  }
}

// paintPerSplitDisplay:
// paints all cells with per-split settings for a given split
void paintPerSplitDisplay(byte side) {
  clearDisplay();

  doublePerSplit = false;  

  // set Midi Mode and channel lights
  switch (Split[side].midiMode) {
    case oneChannel:
    {
      setLed(1, 7, Split[side].colorMain, cellOn);
      break;
    }
    case channelPerNote:
    {
      setLed(1, 6, getMpeColor(side), cellOn);
      break;
    }
    case channelPerRow:
    {
      setLed(1, 5, getChannelPerRowColor(side), cellOn);
      break;
    }
  }

  switch (midiChannelSelect) {
    case MIDICHANNEL_MAIN:
      setLed(2, 7, Split[side].colorMain, cellOn);
      showMainMidiChannel(side);
      break;
    case MIDICHANNEL_PERNOTE:
      setLed(2, 6, Split[side].colorMain, cellOn);
      showPerNoteMidiChannels(side);
      break;
    case MIDICHANNEL_PERROW:
      setLed(2, 5, Split[side].colorMain, cellOn);
      showPerRowMidiChannel(side);
      break;
  }

  switch (Split[side].bendRangeOption) {
    case bendRange2:
      setLed(7, 7, Split[side].colorMain, cellOn);
      break;
    case bendRange3:
      setLed(7, 6, Split[side].colorMain, cellOn);
      break;
    case bendRange12:
      setLed(7, 5, Split[side].colorMain, cellOn);
      break;
    case bendRange24:
      setLed(7, 4, getBendRangeColor(side), cellOn);
      break;
  }

  // set Pitch/X settings
  if (Split[side].sendX == true)  {
    setLed(8, 7, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchCorrectQuantize == true) {
    setLed(8, 6, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchCorrectHold == pitchCorrectHoldMedium ||
      Split[side].pitchCorrectHold == pitchCorrectHoldSlow) {
    setLed(8, 5, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchCorrectHold == pitchCorrectHoldFast ||
      Split[side].pitchCorrectHold == pitchCorrectHoldSlow) {
    setLed(8, 4, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchResetOnRelease == true) {
    setLed(8, 3, Split[side].colorMain, cellOn);
  }

  // set Timbre/Y settings
  if (Split[side].sendY == true)  {
    setLed(9, 7, getLimitsForYColor(side), cellOn);
  }

  switch (Split[side].expressionForY) {
    case timbrePolyPressure:
    case timbreChannelPressure:
    case timbreCC74:
      setLed(9, 5, getCCForYColor(side), cellOn);
      break;
    case timbreCC1:
      setLed(9, 6, Split[side].colorMain, cellOn);
      break;
  }

  if (Split[side].relativeY == true)
  {
    setLed(9, 4, getRelativeYColor(side), cellOn);
  }

  // set Loudness/Z settings
  if (Split[side].sendZ == true)  {
    setLed(10, 7, getLimitsForZColor(side), cellOn);
  }

  switch (Split[side].expressionForZ) {
    case loudnessPolyPressure:
      setLed(10, 6, Split[side].colorMain, cellOn);
      break;
    case loudnessChannelPressure:
      setLed(10, 5, Split[side].colorMain, cellOn);
      break;
    case loudnessCC11:
      setLed(10, 4, getCCForZColor(side), cellOn);
      break;
  }

  // Set "Color" lights
  setLed(11, 7, Split[side].colorMain, cellOn);
  setLed(11, 6, Split[side].colorAccent, cellOn);
  setLed(11, 5, Split[side].colorPlayed, cellOn);
  setLed(11, 4, Split[side].colorLowRow, cellOn);

  // Set "Low row" lights
  switch (Split[side].lowRowMode) {
    case lowRowNormal:
      setLed(12, 7, Split[side].colorMain, cellOn);
      break;
    case lowRowRestrike:
      setLed(12, 6, Split[side].colorMain, cellOn);
      break;
    case lowRowStrum:
      setLed(12, 5, Split[side].colorMain, cellOn);
      break;
    case lowRowArpeggiator:
      setLed(12, 4, Split[side].colorMain, cellOn);
      break;
    case lowRowSustain:
      setLed(13, 7, Split[side].colorMain, cellOn);
      break;
    case lowRowBend:
      setLed(13, 6, Split[side].colorMain, cellOn);
      break;
    case lowRowCCX:
      setLed(13, 5, getLowRowCCXColor(side), cellOn);
      break;
    case lowRowCCXYZ:
      setLed(13, 4, getLowRowCCXYZColor(side), cellOn);
      break;
  }

  // set Arpeggiator
  if (Split[side].arpeggiator)  {
    setLed(14, 7, Split[side].colorMain, cellOn);
  }

  // set CC faders
  if (Split[side].ccFaders)  {
    setLed(14, 6, getCCFadersColor(side), cellOn);
  }

  // set strum
  if (Split[side].strum)  {
    setLed(14, 5, Split[side].colorMain, cellOn);
  }

  // set sequencer
  if (Split[side].sequencer)  {
    setLed(14, 4, Split[side].colorMain, cellOn);
  }

  // set "show split" led
  paintShowSplitSelection(side);
}

byte getMpeColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].mpe) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getChannelPerRowColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].midiChanPerRowReversed) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getBendRangeColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].customBendRange != 24) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getLimitsForYColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].minForY != 0 || Split[side].maxForY != 127) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getCCForYColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].customCCForY != 74) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getRelativeYColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].initialRelativeY != 64) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getLimitsForZColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].minForZ != 0 || Split[side].maxForZ != 127 || Split[side].ccForZ14Bit) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getCCForZColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].customCCForZ != 11) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getLowRowCCXColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].ccForLowRow != 1) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getLowRowCCXYZColor(byte side) {
  byte color = Split[side].colorMain;
  if (Split[side].ccForLowRowX != 16) {
    color = Split[side].colorAccent;
  }
  if (Split[side].ccForLowRowY != 17) {
    color = Split[side].colorAccent;
  }
  if (Split[side].ccForLowRowZ != 18) {
    color = Split[side].colorAccent;
  }
  return color;
}

byte getCCFadersColor(byte side) {
  byte color = Split[side].colorMain;
  for (byte f = 0; f < 8; ++f) {
    if (Split[side].ccForFader[f] != f+1) {
      color = Split[side].colorAccent;
      break;
    }
  }
  return color;
}

byte getCalibrationColor() {
  if (Device.calibrated) {
    return COLOR_GREEN;
  }
  return COLOR_RED;
}

byte getSplitHandednessColor() {
  if (Device.splitHandedness == reversedBoth) {
    return globalColor;
  }
  return globalAltColor;
}

byte getGuitarTuningColor() {
  byte color = globalColor;
  if (Global.guitarTuning[0] != 30 ||
      Global.guitarTuning[1] != 35 ||
      Global.guitarTuning[2] != 40 ||
      Global.guitarTuning[3] != 45 ||
      Global.guitarTuning[4] != 50 ||
      Global.guitarTuning[5] != 55 ||
      Global.guitarTuning[6] != 59 ||
      Global.guitarTuning[7] != 64) {
    color = globalAltColor;
  }
  return color;
}

// paint one of the two leds that indicate which split is being controlled
// (e.g. when you're changing per-split settings, or changing the preset or volume)
void paintShowSplitSelection(byte side) {
  if (side == LEFT || doublePerSplit) {
    setLed(15, 7, Split[LEFT].colorMain, cellOn);
  }
  if (side == RIGHT || doublePerSplit) {
    setLed(16, 7, Split[RIGHT].colorMain, cellOn);
  }
}

void paintOSVersionDisplay() {
  clearDisplay();

  byte color = Split[LEFT].colorMain;
  smallfont_draw_string(0, 0, OSVersion, color);
}

void paintOSVersionBuildDisplay() {
  clearDisplay();

  byte color = Split[LEFT].colorAccent;
  smallfont_draw_string(0, 0, OSVersionBuild, color);
}

// paint the current preset number for a particular side, in large block characters
byte getPresetDisplayColumn() {
  return LINNMODEL == 200 ? NUMCOLS-2 : NUMCOLS-1;
}

void paintPresetDisplay(byte side) {
  clearDisplay();
  setLed(1, 7, COLOR_GREEN, cellOn);
  setLed(1, 6, COLOR_RED, cellOn);
  for (byte p = 0; p < NUMPRESETS; ++p) {
    int color = globalColor;
    if (p == Device.lastLoadedPreset) {
      color = COLOR_CYAN;
    }
    int row = p+2;
    if (row >= 6) row -= 6;
    setLed(getPresetDisplayColumn(), row, color, cellOn);
  }
  paintSplitNumericDataDisplay(side, midiPreset[side]+1, 0, false);
}

void paintBendRangeDisplay(byte side) {
  clearDisplay();
  paintSplitNumericDataDisplay(side, Split[side].customBendRange, 0, false);
}

void paintLimitsForYDisplay(byte side) {
  clearDisplay();

  switch (limitsForYConfigState) {
    case 1:
      condfont_draw_string(0, 0, "L", Split[side].colorMain, true);
      paintSplitNumericDataDisplay(side, Split[side].minForY, 4, true);
      break;
    case 0:
      condfont_draw_string(0, 0, "H", Split[side].colorMain, true);
      paintSplitNumericDataDisplay(side, Split[side].maxForY, 4, true);
      break;
    }
}

void paintCCForYDisplay(byte side) {
  clearDisplay();
  if (Split[side].customCCForY == 128) {
    condfont_draw_string(0, 0, "POPRS", Split[side].colorMain, false);
    paintShowSplitSelection(side);
  }
  else if (Split[side].customCCForY == 129) {
    condfont_draw_string(0, 0, "CHPRS", Split[side].colorMain, false);
    paintShowSplitSelection(side);
  }
  else {
    paintSplitNumericDataDisplay(side, Split[side].customCCForY, 0, false);
  }
}

void paintInitialForRelativeYDisplay(byte side) {
  clearDisplay();
  paintSplitNumericDataDisplay(side, Split[side].initialRelativeY, 0, false);
}

void paintLimitsForZDisplay(byte side) {
  clearDisplay();

  switch (limitsForZConfigState) {
    case 2:
      condfont_draw_string(0, 0, "L", Split[side].colorMain, true);
      paintSplitNumericDataDisplay(side, Split[side].minForZ, 4, true);
      break;
    case 1:
      condfont_draw_string(0, 0, "H", Split[side].colorMain, true);
      paintSplitNumericDataDisplay(side, Split[side].maxForZ, 4, true);
      break;
    case 0:
      if (Split[side].ccForZ14Bit) {
        condfont_draw_string(0, 0, "14BT", Split[side].colorMain, true);
      }
      else {
        condfont_draw_string(3, 0, "7BT", Split[side].colorMain, true);
      }
      paintShowSplitSelection(side);
      break;
  }
}

void paintCCForZDisplay(byte side) {
  clearDisplay();
  if (Split[side].expressionForZ != loudnessCC11) {
    setDisplayMode(displayPerSplit);
    updateDisplay();
  }
  else {
    paintSplitNumericDataDisplay(side, Split[side].customCCForZ, 0, false);
  }
}

void paintCCForFaderDisplay(byte side) {
  clearDisplay();
  for (byte r = 0; r < NUMROWS; ++r) {
    setLed(NUMCOLS-1, r, globalColor, cellOn);
  }
  setLed(NUMCOLS-1, currentEditedCCFader[side], COLOR_GREEN, cellOn);
  unsigned short cc = Split[side].ccForFader[currentEditedCCFader[side]];
  if (cc == 128) {
    condfont_draw_string(0, 0, "CHPRS", Split[side].colorMain, false);
    paintShowSplitSelection(side);
  }
  else {
    paintSplitNumericDataDisplay(side, cc, 0, false);
  }
}

void paintPlayedTouchModeDisplay(byte side) {
  clearDisplay();
  switch(Split[side].playedTouchMode) {
    case playedCell:
      adaptfont_draw_string(0, 0, "CELL", Split[side].colorMain, true);
      break;
    case playedSame:
      adaptfont_draw_string(0, 0, LINNMODEL == 200 ? "SAME" : "SAM", Split[side].colorMain, true);
      break;
    case playedCrosses:
      adaptfont_draw_string(0, 0, "CROS", Split[side].colorMain, true);
      break;
    case playedCircles:
      adaptfont_draw_string(0, 0, "CIRC", Split[side].colorMain, true);
      break;
    case playedSquares:
      adaptfont_draw_string(0, 0, "SQUA", Split[side].colorMain, true);
      break;
    case playedDiamonds:
      adaptfont_draw_string(0, 0, LINNMODEL == 200 ? "DIAM" : "DIA", Split[side].colorMain, true);
      break;
    case playedStars:
      adaptfont_draw_string(0, 0, "STAR", Split[side].colorMain, true);
      break;
    case playedSparkles:
      adaptfont_draw_string(0, 0, "SPAR", Split[side].colorMain, true);
      break;
    case playedCurtains:
      adaptfont_draw_string(0, 0, "CURT", Split[side].colorMain, true);
      break;
    case playedBlinds:
      adaptfont_draw_string(0, 0, "BLIN", Split[side].colorMain, true);
      break;
    case playedTargets:
      adaptfont_draw_string(0, 0, "TARG", Split[side].colorMain, true);
      break;
    case playedUp:
      adaptfont_draw_string(0, 0, "UP", Split[side].colorMain, true);
      break;
    case playedDown:
      adaptfont_draw_string(0, 0, "DOW", Split[side].colorMain, true);
      break;
    case playedLeft:
      adaptfont_draw_string(0, 0, "LEFT", Split[side].colorMain, true);
      break;
    case playedRight:
      adaptfont_draw_string(0, 0, "RIGH", Split[side].colorMain, true);
      break;
    case playedOrbits:
      adaptfont_draw_string(0, 0, "ORB", Split[side].colorMain, true);
      break;
  }
  paintShowSplitSelection(side);
}

void paintLowRowCCXConfigDisplay(byte side) {
  clearDisplay();
  switch (lowRowCCXConfigState) {
    case 1:
      switch (Split[Global.currentPerSplit].lowRowCCXBehavior) {
        case lowRowCCHold:
          adaptfont_draw_string(0, 0, "HLD", Split[side].colorMain, true);
          break;
        case lowRowCCFader:
          adaptfont_draw_string(0, 0, "FDR", Split[side].colorMain, true);
          break;
      }
      paintShowSplitSelection(side);
      break;
    case 0:
      if (Split[side].ccForLowRow == 128) {
        condfont_draw_string(0, 0, "CHPRS", Split[side].colorMain, false);
        paintShowSplitSelection(side);
      }
      else {
        paintSplitNumericDataDisplay(side, Split[side].ccForLowRow, 0, false);
      }
      break;
    }
}

void paintLowRowCCXYZConfigDisplay(byte side) {
  clearDisplay();
  switch (lowRowCCXYZConfigState) {
    case 3:
      switch (Split[Global.currentPerSplit].lowRowCCXYZBehavior) {
        case lowRowCCHold:
          adaptfont_draw_string(0, 0, "HLD", Split[side].colorMain, true);
          break;
        case lowRowCCFader:
          adaptfont_draw_string(0, 0, "FDR", Split[side].colorMain, true);
          break;
      }
      paintShowSplitSelection(side);
      break;
    case 2:
      if (Split[side].ccForLowRowX == 128) {
        condfont_draw_string(0, 0, "XCHPR", Split[side].colorMain, false);
        paintShowSplitSelection(side);
      }
      else {
        condfont_draw_string(0, 0, "X", Split[side].colorMain, true);
        paintSplitNumericDataDisplay(side, Split[side].ccForLowRowX, 4, true);
      }
      break;
    case 1:
      if (Split[side].ccForLowRowY == 128) {
        condfont_draw_string(0, 0, "YCHPR", Split[side].colorMain, false);
        paintShowSplitSelection(side);
      }
      else {
        condfont_draw_string(0, 0, "Y", Split[side].colorMain, true);
        paintSplitNumericDataDisplay(side, Split[side].ccForLowRowY, 4, true);
      }
      break;
    case 0:
      if (Split[side].ccForLowRowZ == 128) {
        condfont_draw_string(0, 0, "ZCHPR", Split[side].colorMain, false);
        paintShowSplitSelection(side);
      }
      else {
        condfont_draw_string(0, 0, "Z", Split[side].colorMain, true);
        paintSplitNumericDataDisplay(side, Split[side].ccForLowRowZ, 4, true);
      }
      break;
  }
}

void paintCCForSwitchCC65ConfigDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Global.ccForSwitchCC65[switchSelect], 0, false);
}

void paintCCForSwitchSustainConfigDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Global.ccForSwitchSustain[switchSelect], 0, false);
}

void paintCustomSwitchAssignmentConfigDisplay() {
  clearDisplay();
  switch (Global.customSwitchAssignment[switchSelect]) {
    case ASSIGNED_TAP_TEMPO:
      adaptfont_draw_string(0, 0, "TAP", globalColor, true);
      break;
    case ASSIGNED_LEGATO:
      adaptfont_draw_string(0, 0, "LEG", globalColor, true);
      break;
    case ASSIGNED_LATCH:
      adaptfont_draw_string(0, 0, "LAT", globalColor, true);
      break;
    case ASSIGNED_PRESET_UP:
      adaptfont_draw_string(0, 0, "PR+", globalColor, true);
      break;
    case ASSIGNED_PRESET_DOWN:
      adaptfont_draw_string(0, 0, "PR-", globalColor, true);
      break;
    case ASSIGNED_REVERSE_PITCH_X:
      adaptfont_draw_string(0, 0, "PCH", globalColor, true);
      break;
    case ASSIGNED_SEQUENCER_PLAY:
      adaptfont_draw_string(0, 0, "PLAY", globalColor, true);
      break;
    case ASSIGNED_SEQUENCER_PREV:
      adaptfont_draw_string(0, 0, "PREV", globalColor, true);
      break;
    case ASSIGNED_SEQUENCER_NEXT:
      adaptfont_draw_string(0, 0, "NEXT", globalColor, true);
      break;
    case ASSIGNED_STANDALONE_MIDI_CLOCK:
      adaptfont_draw_string(0, 0, "CLK", globalColor, true);
      break;
    case ASSIGNED_SEQUENCER_MUTE:
      adaptfont_draw_string(0, 0, "MUTE", globalColor, true);
      break;
  }
}

void paintLimitsForVelocityDisplay() {
  clearDisplay();

  switch (limitsForVelocityConfigState) {
    case 1:
      condfont_draw_string(0, 0, "L", globalColor, true);
      paintNumericDataDisplay(globalColor, Global.minForVelocity, 4, true);
      break;
    case 0:
      condfont_draw_string(0, 0, "H", globalColor, true);
      paintNumericDataDisplay(globalColor, Global.maxForVelocity, 4, true);
      break;
  }
}

void paintValueForFixedVelocityDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Global.valueForFixedVelocity, 0, true);
}

void paintSleepConfig() {
  clearDisplay();

  switch (sleepConfigState) {
    case 1:
      switch (Device.sleepAnimationType) {
        case animationNone:
          adaptfont_draw_string(0, 0, "SLP", globalColor, true);
          break;
        case animationStore:
          adaptfont_draw_string(0, 0, "STR", globalColor, true);
          break;
        case animationChristmas:
          adaptfont_draw_string(0, 0, "XMS", globalColor, true);
          break;
      }
      break;
    case 0:
      if (Device.sleepDelay == 0) {
        adaptfont_draw_string(0, 0, "NOW", globalColor, true);
      }
      else {
        adaptfont_draw_string(0, 0, "D", globalColor, true);
        paintNumericDataDisplay(globalColor, Device.sleepDelay, 4, true);
      }
      break;
  }
}

void paintSplitHandedness() {
  clearDisplay();
  switch (Device.splitHandedness) {
    case reversedBoth:
      adaptfont_draw_string(0, 0, "REV", globalColor, true);
      break;
    case reversedLeft:
      adaptfont_draw_string(0, 0, "REVL", globalColor, true);
      break;
    case reversedRight:
      adaptfont_draw_string(0, 0, "REVR", globalColor, true);
      break;
  }
}

void paintRowOffset() {
  clearDisplay();
  if (Global.customRowOffset == -17) {
    condfont_draw_string(0, 0, "-GUI", globalColor, false);
  }
  else {
    paintNumericDataDisplay(globalColor, Global.customRowOffset, 0, false);
  }
}

void paintGuitarTuning() {
  clearDisplay();

  for (byte r = 0; r < NUMROWS; ++r) {
    setLed(1, r, guitarTuningRowNum == r ? Split[Global.currentPerSplit].colorAccent : Split[Global.currentPerSplit].colorMain, cellOn);
  }

  paintNoteDataDisplay(globalColor, Global.guitarTuning[guitarTuningRowNum], LINNMODEL == 200 ? 2 : 1);
}

void paintMIDIThrough() {
  clearDisplay();
  if (Device.midiThrough) {
    adaptfont_draw_string(0, 0, "THRU", globalColor, true);
  }
  else {
    adaptfont_draw_string(0, 0, LINNMODEL == 200 ? "NORM" : "NRM", globalColor, true);
  }
}

void paintMinUSBMIDIIntervalDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Device.minUSBMIDIInterval, 0, true);
}

void paintSensorSensitivityZDisplay() {
  for (byte row = 1; row < NUMROWS; ++row) {
    clearRow(row);
  }
  paintNumericDataDisplay(globalColor, Device.sensorSensitivityZ, 0, false);
}

void paintSensorLoZDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Device.sensorLoZ, 0, false);
}

void paintSensorFeatherZDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Device.sensorFeatherZ, 0, false);
}

void paintSensorRangeZDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Device.sensorRangeZ, 0, false);
}

void paintSplitNumericDataDisplay(byte side, unsigned short value, byte offset, boolean condensed) {
  paintShowSplitSelection(side);
  paintNumericDataDisplay(Split[side].colorMain, value, offset, condensed);
}

void paintNumericDataDisplay(byte color, short value, short offset, boolean condensed) {
  char str[10];
  const char* format;
  byte pos;

  if (value < 100) {
    format = "%2d";
    pos = condensed ? 3 : 5;
  }
  else if (value >= 100 && value < 200) {
    // Handle the "1" character specially, to get the spacing right
    if (condensed) {
      condfont_draw_string(offset, 0, "1", color, false);
    }
    else {
      smallfont_draw_string(offset + 2, 0, "1", color, false);
    }
    value -= 100;
    format = "%02d";     // to make sure a leading zero is included
    pos = condensed ? 3 : 5;
  }
  else {
    format = "%-d";
    pos = 0;
  }

  snprintf(str, sizeof(str), format, value);
  if (condensed) {
    condfont_draw_string(pos+offset, 0, str, color, false);
  }
  else {
    smallfont_draw_string(pos+offset, 0, str, color, false);
  }
}

void paintNoteDataDisplay(byte color, short noteNumber, short offset) {
  char str[10];
  const char* format;

  switch (noteNumber % 12) {
    case 0: format = "C%d"; break;
    case 1: format = "Db%d"; break;
    case 2: format = "D%d"; break;
    case 3: format = "Eb%d"; break;
    case 4: format = "E%d"; break;
    case 5: format = "F%d"; break;
    case 6: format = "Gb%d"; break;
    case 7: format = "G%d"; break;
    case 8: format = "Ab%d"; break;
    case 9: format = "A%d"; break;
    case 10: format = "Bb%d"; break;
    case 11: format = "B%d"; break;
    default: format = "%d"; break;
  }

  snprintf(str, sizeof(str), format, int(noteNumber/12) - 2);
  condfont_draw_string(offset, 0, str, color, false);
}

// draw a horizontal line to indicate volume for a particular side
void paintVolumeDisplay(byte side) {
  clearDisplay();
  paintVolumeDisplayRow(side);
  paintShowSplitSelection(side);
}

void paintVolumeDisplayRow(byte side) {
  paintCCFaderDisplayRow(side, 5, Split[side].colorMain, 7, 1, NUMCOLS-2);
}

void paintOctaveTransposeDisplay(byte side) {
  clearDisplay();
  blinkMiddleRootNote = true;

  // Paint the octave shift value
  if (!doublePerSplit || Split[LEFT].transposeOctave == Split[RIGHT].transposeOctave) {
    paintOctave(Split[Global.currentPerSplit].colorMain, 8, OCTAVE_ROW, Split[side].transposeOctave);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposeOctave) > abs(Split[RIGHT].transposeOctave)) {
      paintOctave(Split[LEFT].colorMain, 8, OCTAVE_ROW, Split[LEFT].transposeOctave);
      paintOctave(Split[RIGHT].colorMain, 8, OCTAVE_ROW, Split[RIGHT].transposeOctave);
    }
    else {
      paintOctave(Split[RIGHT].colorMain, 8, OCTAVE_ROW, Split[RIGHT].transposeOctave);
      paintOctave(Split[LEFT].colorMain, 8, OCTAVE_ROW, Split[LEFT].transposeOctave);
    }
  }

  // Paint the pitch transpose values
  if (!doublePerSplit || Split[LEFT].transposePitch == Split[RIGHT].transposePitch) {
    paintTranspose(Split[Global.currentPerSplit].colorMain, SWITCH_1_ROW, Split[side].transposePitch);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposePitch) > abs(Split[RIGHT].transposePitch)) {
      paintTranspose(Split[LEFT].colorMain, SWITCH_1_ROW, Split[LEFT].transposePitch);
      paintTranspose(Split[RIGHT].colorMain, SWITCH_1_ROW, Split[RIGHT].transposePitch);
    }
    else {
      paintTranspose(Split[RIGHT].colorMain, SWITCH_1_ROW, Split[RIGHT].transposePitch);
      paintTranspose(Split[LEFT].colorMain, SWITCH_1_ROW, Split[LEFT].transposePitch);
    }
  }

  // Paint the light transpose values
  if (!doublePerSplit || Split[LEFT].transposeLights == Split[RIGHT].transposeLights) {
    paintTranspose(Split[Global.currentPerSplit].colorMain, SWITCH_2_ROW, Split[side].transposeLights);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposeLights) > abs(Split[RIGHT].transposeLights)) {
      paintTranspose(Split[LEFT].colorMain, SWITCH_2_ROW, Split[LEFT].transposeLights);
      paintTranspose(Split[RIGHT].colorMain, SWITCH_2_ROW, Split[RIGHT].transposeLights);
    }
    else {
      paintTranspose(Split[RIGHT].colorMain, SWITCH_2_ROW, Split[RIGHT].transposeLights);
      paintTranspose(Split[LEFT].colorMain, SWITCH_2_ROW, Split[LEFT].transposeLights);
    }
  }

  paintShowSplitSelection(side);
}

void paintOctave(byte color, byte midcol, byte row, short octave) {
  setLed(midcol, row, Split[Global.currentPerSplit].colorAccent, cellOn);
  if (0 == color) color = octave > 0 ? COLOR_GREEN : COLOR_RED ;

  switch (octave) {
    case -60:
      setLed(midcol-5, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case -48:
      setLed(midcol-4, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case -36:
      setLed(midcol-3, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case -24:
      setLed(midcol-2, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case -12:
      setLed(midcol-1, row, color, cellOn);
      break;

    case 60:
      setLed(midcol+5, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case 48:
      setLed(midcol+4, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case 36:
      setLed(midcol+3, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case 24:
      setLed(midcol+2, row, color, cellOn);
      // lack of break here is purposeful, we want to fall through...
    case 12:
      setLed(midcol+1, row, color, cellOn);
      break;
  }
}

void paintTranspose(byte color, byte row, short transpose) {
  byte midcol = 8;
  setLed(midcol, row, Split[Global.currentPerSplit].colorAccent, cellOn);    // paint the center cell of the transpose range

  if (transpose != 0) {
    if (0 == color) color = transpose < 0 ? COLOR_RED : COLOR_GREEN;
    byte col_from = (transpose < 0) ? (midcol + transpose) : (midcol + 1);
    byte col_to = (transpose > 0) ? (midcol + transpose) : (midcol - 1);
    for (byte c = col_from; c <= col_to; ++c) {
      setLed(c, row, color, cellOn);
    }
  }
}

void displayNoteLights(int notelights) {
  for (byte row = 0; row < 4; ++row) {
    for (byte col = 0; col < 3; ++col) {
      byte light = col + (row * 3);
      if (notelights & 1 << light) {
        lightLed(2+col, row);
      }
    }
  }
}

void displayActiveNotes() {
  for (byte row = 0; row < 4; ++row) {
    for (byte col = 0; col < 3; ++col) {
      byte light = col + (row * 3);
      if (light == Global.activeNotes) {
        setLed(2 + col, row, globalColor, cellOn);
      }
    }
  }
}

void paintSwitchAssignment(byte mode) {
  switch (mode) {
    case ASSIGNED_TAP_TEMPO:
    case ASSIGNED_LEGATO:
    case ASSIGNED_LATCH:
    case ASSIGNED_PRESET_UP:
    case ASSIGNED_PRESET_DOWN:
    case ASSIGNED_REVERSE_PITCH_X:
    case ASSIGNED_SEQUENCER_PLAY:
    case ASSIGNED_SEQUENCER_PREV:
    case ASSIGNED_SEQUENCER_NEXT:
    case ASSIGNED_STANDALONE_MIDI_CLOCK:
    case ASSIGNED_SEQUENCER_MUTE:
      setLed(9, 3, getSwitchTapTempoColor(), cellOn);
      break;
    case ASSIGNED_AUTO_OCTAVE:
      lightLed(8, 2);
      lightLed(9, 2);
      break;
    case ASSIGNED_OCTAVE_DOWN:
      lightLed(8, 2);
      break;
    case ASSIGNED_OCTAVE_UP:
      lightLed(9, 2);
      break;
    case ASSIGNED_SUSTAIN:
      setLed(8, 1, getSwitchSustainColor(), cellOn);
      break;
    case ASSIGNED_CC_65:
      setLed(9, 1, getSwitchCC65Color(), cellOn);
      break;
    case ASSIGNED_ARPEGGIATOR:
      lightLed(8, 0);
      break;
    case ASSIGNED_ALTSPLIT:
      lightLed(9, 0);
      break;
  }
}

void updateGlobalSettingsFlashTempo(unsigned long now) {  
  if (displayMode == displayGlobal || displayMode == displayGlobalWithTempo) {
    paintGlobalSettingsFlashTempo(now);
  }
  else if (controlButton != GLOBAL_SETTINGS_ROW &&
           !isSyncedToMidiClock() &&
           (isArpeggiatorEnabled(Global.currentPerSplit) ||
            isVisibleSequencer() ||
            isStandaloneMidiClockRunning())) {
    paintGlobalSettingsFlashTempo(now, 0, 0);
  }
}

inline void paintGlobalSettingsFlashTempo(unsigned long now) {
    paintGlobalSettingsFlashTempo(now, 14, 3);
}

inline void paintGlobalSettingsFlashTempo(unsigned long now, byte col, byte row) {
  if (!animationActive && !userFirmwareActive) {
    bool flash_on = false;
    if (isVisibleSequencer())
    {
      flash_on = sequencerFlashTempoOn();
    }
    else
    {
      flash_on = (clock24PPQ == 0);
    }

    // flash the tap tempo cell at the beginning of the beat
    if (flash_on) {
      lightLed(col, row);
      tempoLedOn = now;
    }

    // handle turning off the tap tempo led after minimum 30ms
    if (tempoLedOn != 0 && calcTimeDelta(now, tempoLedOn) > LED_FLASH_DELAY) {
      tempoLedOn = 0;
      clearLed(col, row);
    }
  }
}

// paintGlobalSettingsDisplay:
// Paints LEDs with state of all global settings
void paintGlobalSettingsDisplay() {
  clearDisplay();

  // This code assumes the velocitySensitivity and pressureSensitivity
  // values are equal to the LED rows.
  if (Global.velocitySensitivity == velocityFixed) {
    setLed(10, Global.velocitySensitivity, getFixedVelocityColor(), cellOn);
  }
  else {
    setLed(10, Global.velocitySensitivity, getVelocityColor(), cellOn);
  }
  setLed(11, Global.pressureSensitivity, getPressureColor(), cellOn);

  // Show the MIDI input/output configuration
  if (Global.midiIO == 1) {
    setLed(15, 0, getMIDIUSBColor(), cellOn); // for MIDI over USB
  }
  else {
    setLed(15, 1, getMIDIThroughColor(), cellOn); // for MIDI jacks
  }

  // set light for sleep mode
  if (Device.sleepActive) {
    lightLed(15, 2);
  }

  // Show the low power mode
  if (Device.operatingLowPower) {
    lightLed(15, 3);
  }

  // set light for serial mode
  if (Device.serialMode) {
    lightLed(16, 2);
  }

  // clearly indicate the calibration status
  setLed(16, 3, getCalibrationColor(), cellOn);

  if (!userFirmwareActive) {

    if (Device.otherHanded) {
      setLed(1, 3, getSplitHandednessColor(), cellOn);
    }

    switch (lightSettings) {
      case LIGHTS_MAIN:
        if (!customLedPatternActive) {
          lightLed(1, 0);
          displayNoteLights(Global.mainNotes[Global.activeNotes]);
        }
        break;
      case LIGHTS_ACCENT:
        if (!customLedPatternActive) {
          lightLed(1, 1);
          displayNoteLights(Global.accentNotes[Global.activeNotes]);
        }
        break;
      case LIGHTS_ACTIVE:
        lightLed(1, 2);
        displayActiveNotes();
        break;
    }

    switch (Global.rowOffset) {
      case ROWOFFSET_NOOVERLAP: // no overlap
        lightLed(5, 3);
        break;
      case 3:        // +3
        lightLed(5, 0);
        break;
      case 4:        // +4
        lightLed(6, 0);
        break;
      case 5:        // +5
        lightLed(5, 1);
        break;
      case 6:        // +6
        lightLed(6, 1);
        break;
      case 7:        // +7
        lightLed(5, 2);
        break;
      case ROWOFFSET_OCTAVECUSTOM:      // +octave or custom
        setLed(6, 2, getRowOffsetColor(), cellOn);
        break;
      case ROWOFFSET_GUITAR:            // guitar tuning
        setLed(6, 3, getGuitarTuningColor(), cellOn);
        break;
      case ROWOFFSET_ZERO:
        // no nothing
        break;
    }

    // This code assumes that switchSelect values are the same as the row numbers
    if (switchSelect == SWITCH_FOOT_B) {
      lightLed(7, SWITCH_FOOT_L);
      lightLed(7, SWITCH_FOOT_R);
    }
    else {
      lightLed(7, switchSelect);
    }
    paintSwitchAssignment(Global.switchAssignment[switchSelect]);

    // Indicate whether switches operate on both splits or not
    if (Global.switchBothSplits[switchSelect]) {
      lightLed(8, 3);
    }

    // Indicate whether pressure is behaving like traditional aftertouch or not
    if (Global.pressureAftertouch) {
      lightLed(11, 3);
    }

    // Set the lights for the Arpeggiator settings
    switch (Global.arpDirection) {
      case ArpDown:
        lightLed(12, 0);
        break;
      case ArpUp:
        lightLed(12, 1);
        break;
      case ArpUpDown:
        lightLed(12, 0);
        lightLed(12, 1);
        break;
      case ArpRandom:
        lightLed(12, 2);
        break;
      case ArpReplayAll:
        lightLed(12, 3);
        break;
    }

    switch (Global.arpTempo) {
      case ArpSixteenthSwing:
        lightLed(13, 0);
        lightLed(13, 1);
        break;
      case ArpEighth:
        lightLed(13, 0);
        break;
      case ArpEighthTriplet:
        lightLed(13, 0);
        lightLed(13, 3);
        break;
      case ArpSixteenth:
        lightLed(13, 1);
        break;
      case ArpSixteenthTriplet:
        lightLed(13, 1);
        lightLed(13, 3);
        break;
      case ArpThirtysecond:
        lightLed(13, 2);
        break;
      case ArpThirtysecondTriplet:
        lightLed(13, 2);
        lightLed(13, 3);
        break;
      case ArpFourth:
      case ArpSixtyfourthTriplet:
        // not available as panel settings
        break;
    }

    // show the arpeggiator octave
    if (Global.arpOctave == 1) {
      lightLed(14, 0);
    }
    else if (Global.arpOctave == 2) {
      lightLed(14, 1);
    }

    paintGlobalSettingsFlashTempo(micros());
  }

  if (displayMode == displayGlobalWithTempo) {
    byte color = Split[LEFT].colorMain;
    char str[4];
    const char* format = "%3d";
    snprintf(str, sizeof(str), format, FXD4_TO_INT(fxd4CurrentTempo));
    tinyfont_draw_string(0, 4, str, color);
  }

#ifdef DEBUG_ENABLED
  // Colum 17 is for setting/showing the debug level
  // The value of debugLevel is from -1 up.
  lightLed(17, debugLevel + 1);

  // The columns in column 18 are secret switches.
  for (byte ss = 0; ss < SECRET_SWITCHES; ++ss) {
    if (secretSwitch[ss]) {
      lightLed(18, ss);
    }
  }
#endif
}

void paintCustomLedsEditor() {
  // nothing to do, everything is handled in the regular LED rendering routine
}

byte getRowOffsetColor() {
  if (Global.customRowOffset != 12) {
    return globalAltColor;
  }
  return globalColor;
}

byte getSwitchCC65Color() {
  if (Global.ccForSwitchCC65[switchSelect] != 65) {
    return globalAltColor;
  }
  return globalColor;
}

byte getSwitchSustainColor() {
  if (Global.ccForSwitchSustain[switchSelect] != 64) {
    return globalAltColor;
  }
  return globalColor;
}

byte getSwitchTapTempoColor() {
  if (Global.customSwitchAssignment[switchSelect] != ASSIGNED_TAP_TEMPO) {
    return globalAltColor;
  }
  return globalColor;
}

byte getVelocityColor() {
  if (Global.minForVelocity != DEFAULT_MIN_VELOCITY ||
      Global.maxForVelocity != DEFAULT_MAX_VELOCITY) {
    return globalAltColor;
  }
  return globalColor;
}

byte getFixedVelocityColor() {
  if (Global.valueForFixedVelocity != DEFAULT_FIXED_VELOCITY) {
    return globalAltColor;
  }
  return globalColor;
}

byte getPressureColor() {
  return globalColor;
}

byte getMIDIUSBColor() {
  if (Device.minUSBMIDIInterval != DEFAULT_MIN_USB_MIDI_INTERVAL) {
    return globalAltColor;
  }
  return globalColor;
}

byte getMIDIThroughColor() {
  if (Device.midiThrough) {
    return globalAltColor;
  }
  return globalColor;
}

byte getSleepColor() {
  return globalColor;
}

void paintCalibrationDisplay() {
  clearDisplay();

  switch (calibrationPhase) {
    case calibrationRows:
      for (byte c = 1; c < NUMCOLS; ++c) {
        setLed(c, 0, COLOR_BLUE, cellOn);
        setLed(c, 2, COLOR_BLUE, cellOn);
        setLed(c, 5, COLOR_BLUE, cellOn);
        setLed(c, 7, COLOR_BLUE, cellOn);
      }
      break;
    case calibrationCols:
      for (byte r = 0; r < NUMROWS; ++r) {
        setLed(1, r, COLOR_BLUE, cellOn);
        setLed(4, r, COLOR_BLUE, cellOn);
        setLed(7, r, COLOR_BLUE, cellOn);
        setLed(10, r, COLOR_BLUE, cellOn);
        setLed(13, r, COLOR_BLUE, cellOn);
        setLed(16, r, COLOR_BLUE, cellOn);
        setLed(19, r, COLOR_BLUE, cellOn);
        setLed(22, r, COLOR_BLUE, cellOn);
        setLed(25, r, COLOR_BLUE, cellOn);
      }
      break;
  }
}

void paintResetDisplay() {
  clearDisplay();

  condfont_draw_string(0, 0, LINNMODEL == 200 ? "RESET" : "RSET", globalColor, true);
  for (byte row = 0; row < NUMROWS; ++row) {
    clearLed(0, row);
  }
}

void paintEditAudienceMessage() {
  bigfont_draw_string(audienceMessageOffset, 0, Device.audienceMessages[audienceMessageToEdit], Split[LEFT].colorMain, true, false, Split[LEFT].colorAccent);
}

// chan value is 1-16
void setMidiChannelLed(byte chan, byte color) {
    if (chan > 16) {
      chan -= 16;
    }
    byte row = 7 - (chan - 1) / 4;
    byte col = 3 + (chan - 1) % 4;
    setLed(col, row, color, cellOn);
}

// light per-split midi mode and single midi channel lights
void showMainMidiChannel(byte side) {
  if (Split[side].midiMode == 0 || Split[side].midiChanMainEnabled) {
    setMidiChannelLed(Split[side].midiChanMain, Split[side].colorMain);
  }
}

void showPerRowMidiChannel(byte side) {
  for (byte i = 0; i < 8; ++i) {
    setMidiChannelLed(Split[side].midiChanPerRow + i, Split[side].colorMain);
  }
}

// light per-split midi mode and multi midi channel lights
void showPerNoteMidiChannels(byte side) {
  for (byte chan = 1; chan <= 16; ++chan) {
    // use accent color to show that in MPE mode the main channel is special
    if (Split[side].mpe &&
        Split[side].midiChanMainEnabled &&
        chan == Split[side].midiChanMain) {
      setMidiChannelLed(chan, Split[side].colorAccent);
    }
    else if (Split[side].midiChanSet[chan-1]) {
      setMidiChannelLed(chan, Split[side].colorMain);
    }
  }
}

void paintLowRowPressureBar() {
  int pressureColumn = FXD_TO_INT(FXD_MUL(FXD_DIV(FXD_FROM_INT(sensorCell->pressureZ), FXD_CONST_1016), FXD_FROM_INT(NUMCOLS-2))) + 1;
    
  for (byte c = 1; c < NUMCOLS; ++c) {
    if (c <= pressureColumn) {
      setLed(c, 0, COLOR_GREEN, cellOn);
    }
    else {
      clearLed(c, 0);
    }
  }
}