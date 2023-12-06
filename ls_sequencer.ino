/******************************* ls_clock: LinnStrument Sequencer *********************************
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
These implement the polyphonic expressive step sequencer, independently for each split.
**************************************************************************************************/

const byte SEQ_FADER_TOP = 3;
byte SEQ_FADER_LENGTH;
byte SEQ_FADER_RIGHT;
byte SEQ_FADER_LEFT;

byte SEQ_PATTERN_SELECTOR_LEFT;
byte SEQ_PATTERN_SELECTOR_RIGHT;
const byte SEQ_PATTERN_SELECTOR_BOTTOM = 6;
const byte SEQ_PATTERN_SELECTOR_TOP = 7;

byte SEQ_NAVIGATION_LEFT;
byte SEQ_NAVIGATION_RIGHT;
byte SEQ_NAVIGATION_BOTTOM;
byte SEQ_NAVIGATION_TOP;

byte SEQ_MUTER_COLUMN;
const byte SEQ_MUTER_BOTTOM = 6;
const byte SEQ_MUTER_TOP = 7;

byte SEQ_VIEW_COLUMN;
const byte SEQ_VIEW_BOTTOM = 4;
const byte SEQ_VIEW_TOP = 5;

byte SEQ_STEPSIZE_LEFT;
byte SEQ_STEPSIZE_RIGHT;
const byte SEQ_STEPSIZE_BOTTOM = 4;
const byte SEQ_STEPSIZE_TOP = 5;

byte SEQ_LOOPSCREEN_COLUMN;
const byte SEQ_LOOPSCREEN_ROW = 5;

byte SEQ_DIRECTION_COLUMN;
const byte SEQ_DIRECTION_ROW = 4;

byte SEQ_CLEAR_COLUMN;
const byte SEQ_CLEAR_ROW = 7;

byte SEQ_COPY_COLUMN;
const byte SEQ_COPY_ROW = 6;

byte SEQ_EVENTS_WIDTH;

const byte SEQ_DURATION_EDIT_PANEL_COUNT = 17;

int32_t FXD_SEQ_DURATION_FADER_RATIO;

static unsigned long sequencerFaderChangeTime[4];
static int sequencerFaderLastX[4];

static short sequencerCopyPatternSource = -1;
static short sequencerCopySplitSource = -1;
static short sequencerCopyStepSource = -1;

static boolean sequencerSwitch1WasUsed = false;

const byte seqDurationEditPanelChoices[SEQ_DURATION_EDIT_PANEL_COUNT] {
   1,  // StepSixtyfourthTriplet
   2,  // StepThirtysecondTriplet
   3,  // StepThirtysecond
   4,  // StepSixteenthTriplet
   6,  // StepSixteenth
   8,  // StepEighthTriplet
   9,  // StepSixteenthDotted
   12, // StepEighth
   16, // StepFourthTriplet
   18, // StepEighthDotted
   24, // StepFourth
   32, // StepHalfTriplet
   36, // StepFourthDotted
   48, // StepHalf
   64, // StepWholeTriplet
   72, // StepHalfDotted
   96  // StepWhole
};

const char* seqDurationEditPanelLabels[SEQ_DURATION_EDIT_PANEL_COUNT] {
  "64t",
  "32t",
  "32",
  "16t",
  "16",
  " 8t",
  "16.",
  " 8",
  " 4t",
  " 8.",
  " 4",
  " 2t",
  " 4.",
  " 2",
  " 1t",
  " 2.",
  " 1"
};

struct StepSequencerState;

struct __attribute__ ((packed)) StepEventState {
  void reset();
  boolean isActive();
  void sendNoteOn(StepEvent& event, byte splitNum);
  void sendNoteOff();
  void tick();
  void highlightCell(StepSequencerState& state, StepEvent& event);
  void unhighlightCell();

  signed char note:8;
  byte remainingDuration:8;
  byte channel:5;
  byte highlightedRow:3;
  byte split:1;
  byte highlightedColumn:5;
  boolean focused:1;
  boolean pendingDelete:1;
};

struct StepDataState {
  StepEventState events[MAX_SEQUENCER_STEP_EVENTS];
  boolean focused;
};

struct StepSequencerState {
  void init(byte sp);
  void clear();
  void handleStepEditingTouch(boolean newVelocity, int noteNum, byte stepNum);
  void createNewEvent(int noteNum, byte stepNum, byte eventNum, StepEvent& event, StepDataState& stepState);
  void handleStepEditingRelease(int noteNum, byte stepNum);
  void turnOn();
  void turnOff(boolean save);
  void turnOffEvents();
  void clearAllFocus();
  void clearEventsFocus();
  boolean isRunning();
  void advanceSequencer();
  void setSongPositionPointer(unsigned spp);
  void setPosition(byte stepNum);
  SequencerPattern& getCurrentPattern();
  StepData& getCurrentPatternStep(byte stepNum);
  byte getSensorSequencerPosition();
  int getRowNoteNum(byte noteRow);
  byte getSensorNotesNoteNum();
  int getSensorRowNoteNum();
  StepData& getSensorStep();
  StepDataState& getSensorStepState();
  StepDataState& getStepState(byte stepNum);
  boolean findNoteSequencerCoordinate(StepEvent& event, byte& col, byte& row);
  void paintCurrentPatternStep(byte stepNum);
  void clearSequencer();
  void paintSequencer();
  void paintSequencerUnbuffered();
  void paintLowRow();
  void paintMuter();
  void paintNavigation();
  void paintPatternSelector();
  void paintPerformanceSettings();
  byte getDirectionColor();
  void paintFocusFaders();
  void clearFocusFader(byte row);
  void paintFocusFader(byte row, byte value);
  void paintDurationFader(byte row, unsigned short duration);
  void paintPitchOffsetFader(byte row, short pitchOffset);
  void clearFocusState();
  void updateFocusState();
  boolean hasFocus();
  boolean getFocusStep(byte& stepNum);
  StepEvent* getFocusEvent(byte& stepNum);
  boolean hasFocusEvent();
  void changeFocus(byte stepNum, short eventNum);
  void setEventFocus(byte stepNum, short eventNum, boolean focused);
  void cycleFocus(byte stepNum);
  CellDisplay getEventCellDisplay(byte stepNum, byte eventNum);
  byte getPositionLedColor(byte stepNum);
  void updatePositionLed(byte stepNum);
  byte getCurrentPositionColor();
  byte getOtherPositionColor();
  void selectPreviousPattern();
  void selectNextPattern();
  void selectPattern(byte pattern);

  byte split;
  StepDataState steps[MAX_SEQUENCER_STEPS];
  unsigned short ticksUntilNextStep;
  unsigned short clock24PPQOffset;
  byte positionOffset;
  byte rowOffset;
  short currentPosition;
  short nextPosition;
  short currentPattern;
  short nextPattern;
  boolean focused;
  boolean focusedEvent;
  boolean switchPatternOnBeat;
  boolean running;
  boolean editing;
  boolean muted;
  boolean advancingForward;
  boolean switch2Waiting;
  boolean isBeingTurnedOff;

  StepEventState previewEvent;
};
StepSequencerState seqState[MAX_SEQUENCERS];

void initializeSequencer() {
  SEQ_MUTER_COLUMN = NUMCOLS - 1 - 1;  
  SEQ_PATTERN_SELECTOR_RIGHT = SEQ_MUTER_COLUMN - 1;
  SEQ_PATTERN_SELECTOR_LEFT = SEQ_PATTERN_SELECTOR_RIGHT - (MAX_SEQUENCER_PATTERNS - 1);

  SEQ_VIEW_COLUMN = NUMCOLS - 1 - 3;
  SEQ_STEPSIZE_LEFT = SEQ_VIEW_COLUMN + 1;
  SEQ_STEPSIZE_RIGHT = SEQ_STEPSIZE_LEFT + 1;

  SEQ_LOOPSCREEN_COLUMN = NUMCOLS - 1;

  SEQ_DIRECTION_COLUMN = NUMCOLS - 1;

  SEQ_CLEAR_COLUMN = NUMCOLS - 1;
  SEQ_COPY_COLUMN = NUMCOLS - 1;

  if (LINNMODEL == 200) {
    SEQ_EVENTS_WIDTH = 16;
    SEQ_FADER_LENGTH = 8;

    SEQ_NAVIGATION_LEFT = SEQ_PATTERN_SELECTOR_LEFT;
    SEQ_NAVIGATION_RIGHT = SEQ_NAVIGATION_LEFT + 1;
    SEQ_NAVIGATION_BOTTOM = 4;
    SEQ_NAVIGATION_TOP = 5;
  }
  else if (LINNMODEL == 128) {
    SEQ_EVENTS_WIDTH = 8;
    SEQ_FADER_LENGTH = 6;

    SEQ_NAVIGATION_LEFT = SEQ_PATTERN_SELECTOR_LEFT;
    SEQ_NAVIGATION_RIGHT = SEQ_NAVIGATION_LEFT + 3;
    SEQ_NAVIGATION_BOTTOM = 1;
    SEQ_NAVIGATION_TOP = 2;
  }

  SEQ_FADER_RIGHT = NUMCOLS - 1;
  SEQ_FADER_LEFT = SEQ_FADER_RIGHT - (SEQ_FADER_LENGTH - 1);

  FXD_SEQ_DURATION_FADER_RATIO = FXD_DIV(FXD_FROM_INT(SEQ_FADER_LENGTH), FXD_FROM_INT(SEQ_DURATION_EDIT_PANEL_COUNT));

  for (byte f = 0; f < 4; ++f) {
    sequencerFaderChangeTime[f] = 0;
    sequencerFaderLastX[f] = 0;
  }

  for (byte q = 0; q < MAX_SEQUENCERS; ++q) {
    for (byte p = 0; p < MAX_SEQUENCER_PATTERNS; ++p) {
      Project.sequencer[q].patterns[p].clear();

      Project.sequencer[q].patterns[p].stepSize = StepEighth;
      Project.sequencer[q].patterns[p].sequencerDirection = sequencerForward;
      Project.sequencer[q].patterns[p].loopScreen = false;
      Project.sequencer[q].patterns[p].swing = false;
      if (LINNMODEL == 200)       Project.sequencer[q].patterns[p].length = 16;
      else if (LINNMODEL == 128)  Project.sequencer[q].patterns[p].length = 8;
    }

    Project.sequencer[q].seqDrumNotes[0] = 36;  // Bass Drum
    Project.sequencer[q].seqDrumNotes[1] = 38;  // Snare Drum
    Project.sequencer[q].seqDrumNotes[2] = 37;  // Sidestick
    Project.sequencer[q].seqDrumNotes[3] = 42;  // Hihat Closed
    Project.sequencer[q].seqDrumNotes[4] = 46;  // Hihat Open
    Project.sequencer[q].seqDrumNotes[5] = 54;  // Tamborine
    Project.sequencer[q].seqDrumNotes[6] = 69;  // Cabasa

    Project.sequencer[q].seqDrumNotes[7] = 43;  // Low Tom
    Project.sequencer[q].seqDrumNotes[8] = 47;  // Mid Tom
    Project.sequencer[q].seqDrumNotes[9] = 50;  // High Tom
    Project.sequencer[q].seqDrumNotes[10] = 49; // Crash Cymbal
    Project.sequencer[q].seqDrumNotes[11] = 51; // Ride Cymbal
    Project.sequencer[q].seqDrumNotes[12] = 55; // Splash Cymbal
    Project.sequencer[q].seqDrumNotes[13] = 39; // Hand Clap

    seqState[q].init(q);
  }
}

void applySequencerSettings() {
  fxd4CurrentTempo = FXD4_FROM_INT(Project.tempo);  
}

boolean requiresSequencerSlideTracking() {
  if (isSequencerEditing()) return true;
  if (sensorCol >= SEQ_FADER_LEFT && sensorRow <= SEQ_FADER_TOP && (!isWithinSequencerNavigationArea())) return true;
  return false;
}

inline void setSequencerSongPositionPointer(unsigned spp) {
  seqState[LEFT].setSongPositionPointer(spp);
  seqState[RIGHT].setSongPositionPointer(spp);
}

inline void checkAdvanceSequencer() {
  seqState[LEFT].advanceSequencer();
  seqState[RIGHT].advanceSequencer();
}

boolean sequencerFlashTempoOn() {
  return (clock24PPQ - seqState[Global.currentPerSplit].clock24PPQOffset) == 0;
}

boolean isSequencerActive() {
  return Split[Global.currentPerSplit].sequencer;
}

boolean isVisibleSequencer() {
  return Split[Global.currentPerSplit].sequencer && displayMode == displayNormal;
}

boolean isVisibleSequencerForSplit(byte split) {
  return Split[split].sequencer && displayMode == displayNormal && Global.currentPerSplit == split;
}

boolean isSequencerEditing() {
  return isVisibleSequencer() && seqState[Global.currentPerSplit].editing;
}

void setSplitSequencerEnabled(byte split, boolean flag) {
  if (!flag && Split[split].sequencer) {
    seqState[split].turnOff(true);
  }
  Split[split].sequencer = flag;
}

void sequencersTurnOn() {
  seqState[LEFT].turnOn();
  seqState[RIGHT].turnOn();
}

void sequencersTurnOff(boolean save) {
  seqState[LEFT].turnOff(save);
  seqState[RIGHT].turnOff(save);
}

void sequencerTurnOn(byte split) {
  if (Split[split].sequencer && !seqState[split].running) {
    seqState[split].turnOn();
  }
}

void sequencerTurnOff(byte split, boolean save) {
  if (Split[split].sequencer && seqState[split].running) {
    seqState[split].turnOff(save);
  }
}

void sequencerTogglePlay(byte split) {
  if (Split[split].sequencer) {
    if (!seqState[split].running) {
      seqState[split].turnOn();
    }
    else {
      seqState[split].turnOff(false);
    }
  }
}

void sequencerToggleMute(byte split) {
  seqState[split].muted = !seqState[split].muted;
  if (seqState[split].muted && seqState[split].running) {
    seqState[split].turnOffEvents();
  }

  if (isVisibleSequencerForSplit(RIGHT)) {
    seqState[LEFT].paintMuter();
  }
  if (isVisibleSequencerForSplit(LEFT)) {
    seqState[RIGHT].paintMuter();
  }
}

void sequencerPreviousPattern(byte split) {
  if (Split[split].sequencer) {
    seqState[split].selectPreviousPattern();
  }
}

void sequencerNextPattern(byte split) {
  if (Split[split].sequencer) {
    seqState[split].selectNextPattern();
  }
}

void sequencerSelectPattern(byte split, byte pattern) {
  if (Split[split].sequencer) {
    seqState[split].selectPattern(pattern);
  }
}

short sequencerCurrentPatternNumber(byte split) {
  return seqState[split].currentPattern;
}

boolean sequencerIsRunning() {
  return seqState[LEFT].isRunning() || seqState[RIGHT].isRunning();
}

boolean isSequencerSettingsDisplayMode() {
  return displayMode == displaySequencerProjects ||
    displayMode == displaySequencerDrum0107 ||
    displayMode == displaySequencerDrum0814 ||
    displayMode == displaySequencerColors;
}

boolean isSequencerDisplayMode() {
  return Split[Global.currentPerSplit].sequencer;
}

boolean isControlButtonForSequencer() {
  if (isSequencerDisplayMode()) {
    switch (sensorRow) {
      case SWITCH_1_ROW:
      case SWITCH_2_ROW:
      case SPLIT_ROW:
        return true;
      default:
        return false;
    }
  }

  if (sensorRow == SPLIT_ROW && (Split[LEFT].sequencer || Split[RIGHT].sequencer)) {
    return true;
  }

  return false;
}

boolean handleSequencerControlButtonNewTouch() {
  if (!isControlButtonForSequencer()) {
    return false;
  }

  switch (sensorRow) {
    case SWITCH_1_ROW:
      sequencerSwitch1WasUsed = false;
      setLed(0, SWITCH_1_ROW, globalColor, cellOn);
      break;

    case SWITCH_2_ROW:
      if (isSwitch1Pressed()) {
        sequencerSwitch1WasUsed = true;      
      }
      seqState[Global.currentPerSplit].switch2Waiting = true;
      sequencerTurnOn(Global.currentPerSplit);
      if (!isSwitch1Pressed()) {
        sequencerTurnOn(otherSplit(Global.currentPerSplit));
      }
      break;

    case SPLIT_ROW:
      resetAllTouches();
      setLed(0, SPLIT_ROW, globalColor, cellOn);
      break;
  }

  return true;
}

boolean handleSequencerControlButtonRelease() {
  if (!isControlButtonForSequencer()) {
    return false;
  }

  switch (sensorRow) {
    case SWITCH_1_ROW:
      clearLed(0, SWITCH_1_ROW);
      if (!sequencerSwitch1WasUsed) {
        clearSwitches();
        if (isSequencerSettingsDisplayMode()) {
          setDisplayMode(displayNormal);
          updateDisplay();
          storeSettings();
        }
        else {
          setDisplayMode(displaySequencerProjects);
          resetNumericDataChange();
          updateDisplay();
        }
      }
      break;

    case SWITCH_2_ROW:
      if (seqState[Global.currentPerSplit].switch2Waiting && calcTimeDelta(millis(), lastControlPress[sensorRow]) <= SWITCH_HOLD_DELAY) {
        sequencerTurnOff(Global.currentPerSplit, true);
        if (!isSwitch1Pressed()) {
          sequencerTurnOff(otherSplit(Global.currentPerSplit), true);
        }
      }
      seqState[Global.currentPerSplit].switch2Waiting = false;
      updateSwitchLeds();
      break;

    case SPLIT_ROW:
      setLed(0, SPLIT_ROW, COLOR_OFF, cellOff);
      Global.currentPerSplit = otherSplit(Global.currentPerSplit);
      setDisplayMode(displayNormal);
      updateDisplay();
      break;
  }

  return true;
}

void handleSequencerTouch(boolean newVelocity) {
  if (sensorCell->velocity) {
    StepSequencerState& state = seqState[sensorSplit];

    // handle fader editing
    if (state.editing) {
      handleSequencerFaderTouch(newVelocity);
    }
    // handle sequencer gestures
    else if (sensorCol >= 1 && sensorCol <= SEQ_EVENTS_WIDTH) {
      // handle low-row interaction
      if (sensorRow == 0) {
        handleSequencerLowRowTouch(newVelocity);
      }
      // handle step editing
      else if (!state.editing) {
        switch (Split[sensorSplit].sequencerView) {
          case sequencerNotes:
            handleStepEditingTouchNotes(newVelocity);
            break;
          case sequencerScales:
            handleStepEditingTouchScales(newVelocity);
            break;
          case sequencerDrums:
            handleStepEditingTouchDrums(newVelocity);
            break;
        }
      }
    }
    else if (newVelocity && isWithinClearFocusArea()) {
      state.clearFocusState();
    }
    // handle muter
    else if (newVelocity && isWithinSequencerMuterArea()) {
      handleSequencerMuterTouch();
    }
    // handle clear
    else if (newVelocity && isOnSequencerClearAction()) {
      handleSequencerClearTouch();
    }
    // handle copy
    else if (newVelocity && isOnSequencerCopyAction()) {
      handleSequencerCopyTouch();
    }
    // handle performance settings
    else if (isWithinSequencerPerformanceSettingsArea()) {
      handleSequencerPerformanceSettingsTouch(newVelocity);
    }
    // handle navigation
    else if (newVelocity && isWithinSequencerNavigationArea()) {
      handleSequencerNavigationTouch();
    }
    // handle pattern selection
    else if (newVelocity && isWithinSequencerPatternArea()) {
      handleSequencerPatternTouch();
    }
    // handle fader editing
    else if (isWithinSequencerFaderArea()) {
      handleSequencerFaderTouch(newVelocity);
    }
  }
}

void handleSequencerRelease() {
  if (sensorCell->velocity) {
    StepSequencerState& state = seqState[sensorSplit];

    // handle faders
    if (state.editing) {
      handleSequencerFaderRelease();
    }
    else if (sensorCol >= 1 && sensorCol <= SEQ_EVENTS_WIDTH) {
      // handle low-row interaction
      if (sensorRow > 0) {
        switch (Split[sensorSplit].sequencerView) {
          case sequencerNotes:
            handleStepEditingReleaseNotes();
            break;
          case sequencerScales:
            handleStepEditingReleaseScales();
            break;
          case sequencerDrums:
            handleStepEditingReleaseDrums();
            break;
        }
      }
    }
    // handle faders
    else if (isWithinSequencerFaderArea()) {
      handleSequencerFaderRelease();
    }
    // handle clear
    else if (isOnSequencerClearAction()) {
      handleSequencerClearRelease();
    }
    // handle copy
    else if (isOnSequencerCopyAction()) {
      handleSequencerCopyRelease();
    }
    // handle performance settings
    else if (isWithinSequencerPerformanceSettingsArea()) {
      handleSequencerPerformanceSettingsRelease();
    }
  }
}

boolean isWithinClearFocusArea() {
  return sensorCol > SEQ_EVENTS_WIDTH && sensorCol < SEQ_FADER_LEFT;
}

boolean isWithinSequencerMuterArea() {
  return sensorCol == SEQ_MUTER_COLUMN && sensorRow >= SEQ_MUTER_BOTTOM && sensorRow <= SEQ_MUTER_TOP;
}

boolean isOnSequencerClearAction() {
  return sensorCol == SEQ_CLEAR_COLUMN && sensorRow == SEQ_CLEAR_ROW;
}

boolean isOnSequencerCopyAction() {
  return sensorCol == SEQ_COPY_COLUMN && sensorRow == SEQ_COPY_ROW;
}

boolean isSequencerNavigationAreaVisible() {
  return LINNMODEL == 200 || !seqState[Global.currentPerSplit].hasFocusEvent();
}

boolean isWithinSequencerNavigationArea() {
  if (!isSequencerNavigationAreaVisible()) {
    return false;
  }

  if (sensorCol >= SEQ_NAVIGATION_LEFT && sensorCol <= SEQ_NAVIGATION_RIGHT) {
    switch (Split[sensorSplit].sequencerView) {
      case sequencerNotes:
        return sensorRow == SEQ_NAVIGATION_BOTTOM;
        break;
      case sequencerScales:
      case sequencerDrums:
        return sensorRow >= SEQ_NAVIGATION_BOTTOM && sensorRow <= SEQ_NAVIGATION_TOP;
    }
  }

  return false;
}

boolean isWithinSequencerPatternArea() {
  return sensorCol >= SEQ_PATTERN_SELECTOR_LEFT && sensorCol <= SEQ_PATTERN_SELECTOR_RIGHT && sensorRow >= SEQ_PATTERN_SELECTOR_BOTTOM && sensorRow <= SEQ_PATTERN_SELECTOR_TOP;
}

boolean isWithinSequencerPerformanceSettingsArea() {
  return sensorCol >= SEQ_VIEW_COLUMN && sensorRow <= SEQ_VIEW_TOP && sensorCol <= SEQ_DIRECTION_COLUMN && sensorRow >= SEQ_VIEW_BOTTOM;
}

boolean isWithinSequencerFaderArea() {
  return seqState[Global.currentPerSplit].hasFocusEvent() && sensorCol >= SEQ_FADER_LEFT && sensorRow <= SEQ_FADER_TOP;
}

void handleSequencerLowRowTouch(boolean newVelocity) {
  if (newVelocity) {
    StepSequencerState& state = seqState[sensorSplit];
    byte stepNum = sensorCol - 1 + state.positionOffset;

    // reposition playhead when switch 2 is pressed
    if (isSwitch2Pressed()) {
      state.nextPosition = stepNum;
      state.switch2Waiting = false;
      cellTouched(ignoredCell);
    }
    // clear step if the switch is pressed
    else if (isSequencerClearPressed()) {
      state.getSensorStep().clear();

      byte position = state.getSensorSequencerPosition();

      for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
        StepEventState& eventState = state.steps[position].events[e];
        eventState.focused = false;
      }

      cellTouched(ignoredCell);
      state.paintSequencer();
    }
    // set copy source of perform copy if the switch is pressed
    else if (isSequencerCopyPressed()) {
      sequencerCopyPatternSource = -1;
      sequencerCopySplitSource = -1;

      if (sequencerCopyStepSource == -1) {
        sequencerCopyStepSource = state.getSensorSequencerPosition();
        state.paintLowRow();
      }
      else {
        byte copyStepTarget = state.getSensorSequencerPosition();
        if (copyStepTarget != sequencerCopyStepSource) {
          StepData& source = state.getCurrentPatternStep(sequencerCopyStepSource);
          StepData& target = state.getCurrentPatternStep(copyStepTarget);
          for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
            target.events[e] = source.events[e];

            StepEventState& eventState = state.steps[copyStepTarget].events[e];
            eventState.focused = false;
          }
        }

        sequencerCopyStepSource = -1;
        state.paintSequencer();
      }
      cellTouched(ignoredCell);
    }
    else if (isSwitch1Pressed()) {
      sequencerSwitch1WasUsed = true;
      state.getCurrentPattern().length = sensorCol + state.positionOffset;
      state.paintLowRow();
    }
    else {
      state.cycleFocus(state.getSensorSequencerPosition());
    }
  }
}

void handleSequencerMuterTouch() {
  byte mutedSplit = 1 - (sensorRow - SEQ_MUTER_BOTTOM);
  sequencerToggleMute(mutedSplit);
}

void handleSequencerClearTouch() {
  clearLed(SEQ_COPY_COLUMN, SEQ_COPY_ROW);
  if (cell(SEQ_COPY_COLUMN, SEQ_COPY_ROW).touched != untouchedCell) {
    cellTouched(SEQ_COPY_COLUMN, SEQ_COPY_ROW, ignoredCell);
  }
  setLed(SEQ_CLEAR_COLUMN, SEQ_CLEAR_ROW, Split[sensorSplit].colorMain, cellSlowPulse);
}

void handleSequencerClearRelease() {
  clearLed(SEQ_CLEAR_COLUMN, SEQ_CLEAR_ROW);
}

boolean isSwitch1Pressed() {
  return cell(0, SWITCH_1_ROW).touched == touchedCell;
}

boolean isSwitch2Pressed() {
  return cell(0, SWITCH_2_ROW).touched == touchedCell;
}

boolean isSequencerClearPressed() {
  return cell(SEQ_CLEAR_COLUMN, SEQ_CLEAR_ROW).touched == touchedCell;
}

void handleSequencerCopyTouch() {
  clearLed(SEQ_CLEAR_COLUMN, SEQ_CLEAR_ROW);
  if (cell(SEQ_CLEAR_COLUMN, SEQ_CLEAR_ROW).touched != untouchedCell) {
    cellTouched(SEQ_CLEAR_COLUMN, SEQ_CLEAR_ROW, ignoredCell);
  }
  setLed(SEQ_COPY_COLUMN, SEQ_COPY_ROW, Split[sensorSplit].colorMain, cellSlowPulse);
}

void handleSequencerCopyRelease() {
  clearLed(SEQ_COPY_COLUMN, SEQ_COPY_ROW);
  sequencerCopyPatternSource = -1;
  sequencerCopySplitSource = -1;
  sequencerCopyStepSource = -1;

  seqState[sensorSplit].paintPatternSelector();
  seqState[sensorSplit].paintLowRow();
}

boolean isSequencerCopyPressed() {
  return cell(SEQ_COPY_COLUMN, SEQ_COPY_ROW).touched == touchedCell;
}

void handleSequencerViewModeRelease() {
  StepSequencerState& state = seqState[sensorSplit];
  
  SequencerView mode = sequencerNotes;
  if ((sensorRow == SEQ_VIEW_TOP && cell(sensorCol, SEQ_VIEW_BOTTOM).touched != untouchedCell) ||
      (sensorRow == SEQ_VIEW_BOTTOM && cell(sensorCol, SEQ_VIEW_TOP).touched != untouchedCell)) {
    cellTouched(sensorCol, SEQ_VIEW_TOP, ignoredCell);
    cellTouched(sensorCol, SEQ_VIEW_BOTTOM, ignoredCell);
    mode = sequencerScales;
  }
  else if (sensorRow == SEQ_VIEW_TOP) {
    mode = sequencerNotes;
  }
  else if (sensorRow == SEQ_VIEW_BOTTOM) {
    mode = sequencerDrums;
  }

  if (mode != Split[sensorSplit].sequencerView) {
    if (Split[sensorSplit].sequencerView == sequencerNotes) {
      for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
        for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
          StepDataState& stepState = state.getStepState(s);
          StepEventState& eventState = stepState.events[e];
          eventState.unhighlightCell();
        }
      }
    }
    Split[sensorSplit].sequencerView = mode;
    seqState[sensorSplit].paintSequencer();

    autoSelectFirstStepForNotesView();
  }
}

void autoSelectFirstStepForNotesView() {
  StepSequencerState& state = seqState[sensorSplit];
  if (Split[sensorSplit].sequencerView == sequencerNotes && !state.hasFocus() && !state.isBeingTurnedOff) {
    state.changeFocus(0, -1);
  }
}

void handleSequencerStepSizeRelease() {
  StepSequencerState& state = seqState[sensorSplit];

  const SequencerStepSize currentStepSize = state.getCurrentPattern().stepSize;
  SequencerStepSize newStepSize = currentStepSize;
  boolean newSwing = state.getCurrentPattern().swing;

  if (sensorCol == SEQ_STEPSIZE_RIGHT) {
    if ((sensorRow == SEQ_STEPSIZE_TOP && cell(sensorCol, SEQ_STEPSIZE_BOTTOM).touched != untouchedCell) ||
        (sensorRow == SEQ_STEPSIZE_BOTTOM && cell(sensorCol, SEQ_STEPSIZE_TOP).touched != untouchedCell)) {
      cellTouched(sensorCol, SEQ_STEPSIZE_TOP, ignoredCell);
      cellTouched(sensorCol, SEQ_STEPSIZE_BOTTOM, ignoredCell);

      newSwing = !state.getCurrentPattern().swing;
      switch (currentStepSize) {
        case StepEighth:
        case StepEighthDotted:
        case StepEighthTriplet:
          newStepSize = StepEighth;
          break;
        case StepFourth:
        case StepFourthDotted:
        case StepFourthTriplet:
          newStepSize = StepFourth;
          break;
        case StepSixteenth:
        case StepSixteenthDotted:
        case StepSixteenthTriplet:
          newStepSize = StepSixteenth;
          break;
      }
    }
    else if (sensorRow == SEQ_STEPSIZE_TOP && cell(sensorCol, SEQ_STEPSIZE_BOTTOM).touched == untouchedCell) {
      newSwing = false;
      switch (currentStepSize) {
        case StepEighthDotted:
          newStepSize = StepEighth;
          break;
        case StepEighth:
        case StepEighthTriplet:
          newStepSize = StepEighthDotted;
          break;
        case StepFourthDotted:
          newStepSize = StepFourth;
          break;
        case StepFourth:
        case StepFourthTriplet:
          newStepSize = StepFourthDotted;
          break;
        case StepSixteenthDotted:
          newStepSize = StepSixteenth;
          break;
        case StepSixteenth:
        case StepSixteenthTriplet:
          newStepSize = StepSixteenthDotted;
          break;
      }
    }
    else if (sensorRow == SEQ_STEPSIZE_BOTTOM && cell(sensorCol, SEQ_STEPSIZE_TOP).touched == untouchedCell) {
      newSwing = false;
      switch (currentStepSize) {
        case StepEighthTriplet:
          newStepSize = StepEighth;
          break;
        case StepEighth:
        case StepEighthDotted:
          newStepSize = StepEighthTriplet;
          break;
        case StepFourthTriplet:
          newStepSize = StepFourth;
          break;
        case StepFourth:
        case StepFourthDotted:
          newStepSize = StepFourthTriplet;
          break;
        case StepSixteenthTriplet:
          newStepSize = StepSixteenth;
          break;
        case StepSixteenth:
        case StepSixteenthDotted:
          newStepSize = StepSixteenthTriplet;
          break;
      }
    }
  }
  else if (sensorCol == SEQ_STEPSIZE_LEFT) {
    if ((sensorRow == SEQ_STEPSIZE_TOP && cell(sensorCol, SEQ_STEPSIZE_BOTTOM).touched != untouchedCell) ||
        (sensorRow == SEQ_STEPSIZE_BOTTOM && cell(sensorCol, SEQ_STEPSIZE_TOP).touched != untouchedCell)) {
      cellTouched(sensorCol, SEQ_STEPSIZE_TOP, ignoredCell);
      cellTouched(sensorCol, SEQ_STEPSIZE_BOTTOM, ignoredCell);

      switch (currentStepSize) {
        case StepEighth:
        case StepFourth:
        case StepSixteenth:
          newStepSize = StepFourth;
          break;
        case StepEighthDotted:
        case StepFourthDotted:
        case StepSixteenthDotted:
          newStepSize = StepFourthDotted;
          break;
        case StepEighthTriplet:
        case StepFourthTriplet:
        case StepSixteenthTriplet:
          newStepSize = StepFourthTriplet;
          break;
      }
    }
    else if (sensorRow == SEQ_STEPSIZE_TOP && cell(sensorCol, SEQ_STEPSIZE_BOTTOM).touched == untouchedCell) {
      switch (currentStepSize) {
        case StepEighth:
        case StepFourth:
        case StepSixteenth:
          newStepSize = StepSixteenth;
          break;
        case StepEighthDotted:
        case StepFourthDotted:
        case StepSixteenthDotted:
          newStepSize = StepSixteenthDotted;
          break;
        case StepEighthTriplet:
        case StepFourthTriplet:
        case StepSixteenthTriplet:
          newStepSize = StepSixteenthTriplet;
          break;
      }
    }
    else if (sensorRow == SEQ_STEPSIZE_BOTTOM && cell(sensorCol, SEQ_STEPSIZE_TOP).touched == untouchedCell) {
      switch (currentStepSize) {
        case StepEighth:
        case StepFourth:
        case StepSixteenth:
          newStepSize = StepEighth;
          break;
        case StepEighthDotted:
        case StepFourthDotted:
        case StepSixteenthDotted:
          newStepSize = StepEighthDotted;
          break;
        case StepEighthTriplet:
        case StepFourthTriplet:
        case StepSixteenthTriplet:
          newStepSize = StepEighthTriplet;
          break;
      }
    }
  }

  if (newStepSize != currentStepSize || newSwing != state.getCurrentPattern().swing) {
    if (state.isRunning()) {
      // adapt the remaining ticks until the next step to ensure that it falls on a multiple of the new step size
      short currentPositionTicks = state.currentPosition * currentStepSize + (currentStepSize - state.ticksUntilNextStep);

      short newRemaining = (currentPositionTicks % newStepSize);
      if (newRemaining == 0) {
        state.ticksUntilNextStep = 0;
      }
      else {
        state.ticksUntilNextStep = ((currentPositionTicks / newStepSize) + 1) * newStepSize - currentPositionTicks;
      }
    }

    state.getCurrentPattern().stepSize = newStepSize;
    state.getCurrentPattern().swing = newSwing;
    seqState[sensorSplit].paintSequencer();
  }
}

void handleSequencerLoopScreenTouch(boolean newVelocity) {
  if (newVelocity) {
    StepSequencerState& state = seqState[sensorSplit];
    state.getCurrentPattern().loopScreen = !state.getCurrentPattern().loopScreen;
    seqState[sensorSplit].paintPerformanceSettings();
  }
}

void handleSequencerDirectionTouch(boolean newVelocity) {
  StepSequencerState& state = seqState[sensorSplit];
  if (newVelocity) {
    setLed(sensorCol, sensorRow, state.getDirectionColor(), cellSlowPulse);
  }
  else if (isCellPastConfirmHoldWait()) {
    state.getCurrentPattern().sequencerDirection = sequencerPingPong;
    seqState[sensorSplit].paintPerformanceSettings();
  }
}

void handleSequencerDirectionRelease() {
  StepSequencerState& state = seqState[sensorSplit];

  SequencerDirection direction = (state.getCurrentPattern().sequencerDirection == sequencerForward ? sequencerBackward : sequencerForward);
  if (isCellPastConfirmHoldWait()) {
    direction = sequencerPingPong;
  }

  if (direction != state.getCurrentPattern().sequencerDirection) {
    state.getCurrentPattern().sequencerDirection = direction;
    seqState[sensorSplit].paintPerformanceSettings();
  }
}

void handleSequencerPerformanceSettingsTouch(boolean newVelocity) {
  if (sensorCol == SEQ_LOOPSCREEN_COLUMN && sensorRow == SEQ_LOOPSCREEN_ROW) {
    handleSequencerLoopScreenTouch(newVelocity);
  }
  else if (sensorCol == SEQ_DIRECTION_COLUMN && sensorRow == SEQ_DIRECTION_ROW) {
    handleSequencerDirectionTouch(newVelocity);
  }
}

void handleSequencerPerformanceSettingsRelease() {
  if (sensorCol == SEQ_VIEW_COLUMN) {
    handleSequencerViewModeRelease();
  }
  else if (sensorCol >= SEQ_STEPSIZE_LEFT && sensorCol <= SEQ_STEPSIZE_RIGHT) {
    handleSequencerStepSizeRelease();
  }
  else if (sensorCol == SEQ_DIRECTION_COLUMN && sensorRow == SEQ_DIRECTION_ROW) {
    handleSequencerDirectionRelease();
  }
}

void handleSequencerNavigationTouch() {
  StepSequencerState& state = seqState[sensorSplit];

  int newPositionOffset = (sensorCol-SEQ_NAVIGATION_LEFT)*SEQ_EVENTS_WIDTH;
  int newRowOffset = -1;
  if (Split[sensorSplit].sequencerView != sequencerNotes) {
    if (sensorRow == SEQ_NAVIGATION_BOTTOM) {
      newRowOffset = 0;
    }
    else if (sensorRow == SEQ_NAVIGATION_TOP) {
      newRowOffset = 7;
    }
  }

  boolean refreshDisplay = false;
  if (newPositionOffset != -1 && newPositionOffset != state.positionOffset) {
    state.positionOffset = newPositionOffset;
    refreshDisplay = true;
  }
  if (newRowOffset != -1 && newRowOffset != state.rowOffset) {
    state.rowOffset = newRowOffset;
    refreshDisplay = true;
  }
  if (refreshDisplay) {
    state.paintSequencer();
  }
}

void handleSequencerPatternTouch() {
  short pattern = sensorCol - SEQ_PATTERN_SELECTOR_LEFT;
  short patternSplit = 1 - (sensorRow - SEQ_PATTERN_SELECTOR_BOTTOM);
  StepSequencerState& state = seqState[patternSplit];
  if (isSequencerClearPressed()) {
    Project.sequencer[patternSplit].patterns[pattern].clear();

    if (isVisibleSequencerForSplit(patternSplit) && state.currentPattern == pattern) {
      state.clearEventsFocus();
      state.paintSequencer();
    }
  }
  else if (isSequencerCopyPressed()) {
    sequencerCopyStepSource = -1;

    if (sequencerCopyPatternSource == -1) {
      sequencerCopyPatternSource = pattern;
      sequencerCopySplitSource = patternSplit;
      state.paintPatternSelector();
    }
    else {
      if (sequencerCopyPatternSource != pattern || sequencerCopySplitSource != patternSplit) {
        SequencerPattern& source = Project.sequencer[sequencerCopySplitSource].patterns[sequencerCopyPatternSource];
        SequencerPattern& target = Project.sequencer[patternSplit].patterns[pattern];
        target = source;

        if (isVisibleSequencerForSplit(patternSplit) && state.currentPattern == pattern) {
          state.clearEventsFocus();
        }
      }

      sequencerCopyPatternSource = -1;
      sequencerCopySplitSource = -1;
      state.paintSequencer();
    }
  }
  else {
    state.selectPattern(pattern);
  }
}

void handleSequencerFaderTouch(boolean newVelocity) {
  StepSequencerState& state = seqState[sensorSplit];
  state.editing = true;

  byte focusStepNum;
  StepEvent* focus = state.getFocusEvent(focusStepNum);
  if (focus && sensorCell->hasUsableX()) {
    boolean changed = false;
    if (newVelocity) {
      // if a new touch happens on the same row, set the fader to its neutral value
      for (byte col = SEQ_FADER_RIGHT; col > 0; --col) {
        if (col != sensorCol && cell(col, sensorRow).velocity && cell(col, sensorRow).touched == touchedCell) {
          cellTouched(sensorCol, sensorRow, ignoredCell);
          focus->setFaderValue(sensorRow, focus->getFaderNeutral(sensorRow, sensorSplit));
          changed = true;
          break;
        }
      }
    }

    if (!changed) {
      changed = focus->calculateSequencerFaderValue(newVelocity);
    }

    // filter that gets rid of anything that's not a visible change
    if (changed || newVelocity) {
      startBufferedLeds();
      
      if (LINNMODEL == 200) {
        state.clearSequencer();
      }
      else if (LINNMODEL == 128) {
        clearDisplay();
      }

      restrictedRow = sensorRow;
      
      switch (sensorRow) {
        case 3:
        {
          paintNumericDataDisplay(Split[sensorSplit].colorMain, focus->getVelocity(), 0, false);
          if (LINNMODEL == 200) state.paintFocusFader(3, focus->getVelocity());
          break;
        }
        case 2:
        {
          int offset = 1;
          const char* durationLabel = seqDurationEditPanelLabels[focus->getFaderValue(sensorRow)];
          if (durationLabel[0] == '1' || (durationLabel[0] == ' ' && durationLabel[1] == '1')) {
            offset += 2;
          }
          if (durationLabel[0] == ' ') {
            offset += 2;
          }
          smallfont_draw_string(offset, 0, (char*)durationLabel, Split[sensorSplit].colorMain, false);
          if (LINNMODEL == 200) state.paintDurationFader(2, focus->getDuration());
          break;
        }
        case 1:
        {
          paintNumericDataDisplay(Split[sensorSplit].colorMain, focus->getPitchOffset(), -4, false);
          if (LINNMODEL == 200) state.paintPitchOffsetFader(1, focus->getPitchOffset());
          break;
        }
        case 0:
        {
          paintNumericDataDisplay(Split[sensorSplit].colorMain, focus->getTimbre(), 0, false);
          if (LINNMODEL == 200) state.paintFocusFader(0, focus->getTimbre());
          break;
        }
      }
      finishBufferedLeds();
    }
  }
}

void handleSequencerFaderRelease() {
  boolean transfered = false;
  for (byte col = SEQ_FADER_RIGHT; col >= SEQ_FADER_LEFT; --col) {
    if (col != sensorCol && cell(col, sensorRow).touched == touchedCell) {
      transferToSameRowCell(col);
      transfered = true;
      break;
    }
  }

  if (!transfered) {
    StepSequencerState& state = seqState[sensorSplit];
    state.editing = false;

    restrictedRow = -1;

    // preview the event when the fader is released
    if (!state.running && !state.muted) {
      byte focusStepNum;
      StepEvent* focus = state.getFocusEvent(focusStepNum);
      state.previewEvent.sendNoteOn(*focus, state.split);
    }

    // repaint the sequencer and remove the numeric display
    startBufferedLeds();
    clearDisplay();
    state.paintSequencerUnbuffered();
    finishBufferedLeds();
  }
}

void handleStepEditingTouchNotes(boolean newVelocity) {
  StepSequencerState& state = seqState[sensorSplit];
  
  byte noteNum = state.getSensorNotesNoteNum();
  byte stepNum;
  if (!state.getFocusStep(stepNum)) {
    return;
  }

  state.handleStepEditingTouch(newVelocity, noteNum, stepNum);
}

void handleStepEditingReleaseNotes() {
  StepSequencerState& state = seqState[sensorSplit];

  byte noteNum = state.getSensorNotesNoteNum();
  byte stepNum;
  if (!state.getFocusStep(stepNum)) {
    return;
  }

  state.handleStepEditingRelease(noteNum, stepNum);
}

void handleStepEditingTouchScales(boolean newVelocity) {
  StepSequencerState& state = seqState[sensorSplit];
  state.handleStepEditingTouch(newVelocity, state.getSensorRowNoteNum(), state.getSensorSequencerPosition());
}

void handleStepEditingReleaseScales() {
  StepSequencerState& state = seqState[sensorSplit];
  state.handleStepEditingRelease(state.getSensorRowNoteNum(), state.getSensorSequencerPosition());
}

void handleStepEditingTouchDrums(boolean newVelocity) {
  StepSequencerState& state = seqState[sensorSplit];
  state.handleStepEditingTouch(newVelocity, state.getSensorRowNoteNum(), state.getSensorSequencerPosition());
}

void handleStepEditingReleaseDrums() {
  StepSequencerState& state = seqState[sensorSplit];
  state.handleStepEditingRelease(state.getSensorRowNoteNum(), state.getSensorSequencerPosition());
}

void updateSequencerSwitchLeds() {
  if (isSequencerDisplayMode()) {
    if (seqState[Global.currentPerSplit].running) {
      setLed(0, SWITCH_2_ROW, COLOR_GREEN, cellOn);
    }
    else {
      clearLed(0, SWITCH_2_ROW);
    }
  }

  if (isSequencerSettingsDisplayMode()) {
    lightLed(0, SWITCH_1_ROW);
  }
}

void paintSequencerDisplay(byte split) {
  startBufferedLeds();
  clearDisplay();
  seqState[split].paintSequencerUnbuffered();
  finishBufferedLeds();

  autoSelectFirstStepForNotesView();
}


//
// Sequencer settings panels
//

static unsigned long lastLegendDisplay = millis();
static boolean legendVisible = false;

void checkLegendDisplayTimeout(unsigned long nowMillis) {
  if (legendVisible && calcTimeDelta(nowMillis, lastLegendDisplay) >= 1500) {
    updateDisplay();
    legendVisible = false;
  }
}

void displaySettingsLegend(const char* str) {
  unsigned long nowMillis = millis();

  for (byte row = 1; row < NUMROWS; ++row) {
    for (byte col = 1; col < NUMCOLS; ++col) {
      clearLed(col, row);
    }
    performContinuousTasks();
  }

  condfont_draw_string(0, 0, str, Split[Global.currentPerSplit].colorMain, false);
  lastLegendDisplay = nowMillis;
  legendVisible = true;
}

void handleSequencerSettingsLowRowTouch() {
  static unsigned long lastLowRowTouch = 0;

  unsigned long nowMillis = millis();

  unsigned long delta = calcTimeDelta(nowMillis, lastLowRowTouch);
  if (sensorCol == 6) {
    if (displayMode == displaySequencerProjects && legendVisible) {
      if (delta > 100) {
        ensureLegendHidden();
      }
    }
    else {
      resetNumericDataChange();
      setDisplayMode(displaySequencerProjects);
      updateDisplay();

      displaySettingsLegend("PROJ");
    }
  }
  else if (sensorCol == 7) {
    if (displayMode == displaySequencerDrum0107 && legendVisible) {
      if (delta > 100) {
        ensureLegendHidden();
      }
    }
    else {
      resetNumericDataChange();
      setDisplayMode(displaySequencerDrum0107);
      updateDisplay();

      displaySettingsLegend("DRU1");
    }
  }
  else if (sensorCol == 8) {
    if (displayMode == displaySequencerDrum0814 && legendVisible) {
      if (delta > 100) {
        ensureLegendHidden();
      }
    }
    else {
      resetNumericDataChange();
      setDisplayMode(displaySequencerDrum0814);
      updateDisplay();

      displaySettingsLegend("DRU2");
    }
  }
  else if (sensorCol == 9) {
    if (displayMode == displaySequencerColors && legendVisible) {
      if (delta > 100) {
        ensureLegendHidden();
      }
    }
    else {
      resetNumericDataChange();
      setDisplayMode(displaySequencerColors);
      updateDisplay();

      displaySettingsLegend("CLRS");
    }
  }

  lastLowRowTouch = nowMillis;
}

void paintSequencerSettingsLowRow() {
  setLed(6, 0, displayMode == displaySequencerProjects ? Split[Global.currentPerSplit].colorPlayed : Split[Global.currentPerSplit].colorLowRow, cellOn);
  setLed(7, 0, displayMode == displaySequencerDrum0107 ? Split[Global.currentPerSplit].colorPlayed : Split[Global.currentPerSplit].colorLowRow, cellOn);
  setLed(8, 0, displayMode == displaySequencerDrum0814 ? Split[Global.currentPerSplit].colorPlayed : Split[Global.currentPerSplit].colorLowRow, cellOn);
  setLed(9, 0, displayMode == displaySequencerColors ? Split[Global.currentPerSplit].colorPlayed : Split[Global.currentPerSplit].colorLowRow, cellOn);
}

void paintSequencerProjects() {
  clearDisplay();

  for (byte p = 0; p < MAX_PROJECTS; ++p) {
    int color = globalColor;
    if (p == Device.lastLoadedProject) {
      color = COLOR_CYAN;
    }
    setLed(6 + p%4, 2 + p/4, color, cellOn);
  }

  paintSequencerSettingsLowRow();
}

boolean ensureLegendHidden() {
  if (legendVisible) {
    updateDisplay();
    legendVisible = false;

    cellTouched(ignoredCell);
    return false;
  }

  return true;
}

void handleSequencerProjectsNewTouch() {
  if (sensorRow == 0) {
    handleSequencerSettingsLowRowTouch();
  }
  else if (ensureLegendHidden()) {
    if (sensorCol >= 6 && sensorCol < 10 && sensorRow >= 2 && sensorRow < 6) {
      // start tracking the touch duration to be able detect a long press
      sensorCell->lastTouch = millis();
      // indicate that a hold operation is being waited for
      setLed(sensorCol, sensorRow, globalColor, cellSlowPulse);
    }
  }
}

void startProjectLEDBlink(byte p, byte color) {
  unsigned long now = millis();
  if (now == 0) {
    now = ~now;
  }
  projectBlinkStart[p] = now;

  setLed(6 + p%4, 2 + p/4, color, cellFastPulse);
}

void handleSequencerProjectsHold() {
  if (sensorCol >= 6 && sensorCol < 10 &&
      sensorRow >= 2 && sensorRow < 6 &&
      isCellPastEditHoldWait()) {
    // store to the selected project
    sequencersTurnOff(true);

    byte project = sensorCol-6 + (sensorRow-2) * 4;

    writeProjectToFlash(project);
    sensorCell->lastTouch = 0;

    updateDisplay();
    startProjectLEDBlink(project, COLOR_RED);
  }
}

void handleSequencerProjectsRelease() {
  if (sensorCol >= 6 && sensorCol < 10 &&
      sensorRow >= 2 && sensorRow < 6 &&
      ensureCellBeforeHoldWait(globalColor, cellOn)) {
    // load the selected project
    sequencersTurnOff(true);

    byte project = sensorCol-6 + (sensorRow-2) * 4;
    Device.lastLoadedProject = project;
    loadProject(project);
    sensorCell->lastTouch = 0;

    updateDisplay();
    startProjectLEDBlink(project, COLOR_GREEN);
  }
}

static byte sequencerDrum0107RowNum = 1;

void paintSequencerDrum0107() {
  clearDisplay();

  for (byte r = 1; r < NUMROWS; ++r) {
    setLed(1, r, sequencerDrum0107RowNum == r ? Split[Global.currentPerSplit].colorAccent : Split[Global.currentPerSplit].colorMain, cellOn);
  }

  paintSplitNumericDataDisplay(Global.currentPerSplit, Project.sequencer[Global.currentPerSplit].seqDrumNotes[sequencerDrum0107RowNum-1], 2, true);
  paintSequencerSettingsLowRow();
}

void handleSequencerDrum0107NewTouch() {
  if (sensorRow == 0) {
    handleSequencerSettingsLowRowTouch();
  }
  else if (ensureLegendHidden()) {
    if (sensorCol == 1) {
      sequencerDrum0107RowNum = sensorRow;
      updateDisplay();
    }
    else {
      handleNumericDataNewTouchCol(Project.sequencer[Global.currentPerSplit].seqDrumNotes[sequencerDrum0107RowNum-1], 0, 127, true);
    }
  }
}

void handleSequencerDrum0107Release() {
  handleNumericDataReleaseCol(true);
}

static byte sequencerDrum0814RowNum = 1;

void paintSequencerDrum0814() {
  clearDisplay();

  for (byte r = 1; r < NUMROWS; ++r) {
    setLed(1, r, sequencerDrum0814RowNum == r ? Split[Global.currentPerSplit].colorAccent : Split[Global.currentPerSplit].colorMain, cellOn);
  }

  paintSplitNumericDataDisplay(Global.currentPerSplit, Project.sequencer[Global.currentPerSplit].seqDrumNotes[sequencerDrum0814RowNum - 1 + 7], 2, true);
  paintSequencerSettingsLowRow();
}

void handleSequencerDrum0814NewTouch() {
  if (sensorRow == 0) {
    handleSequencerSettingsLowRowTouch();
  }
  else if (ensureLegendHidden()) {
    if (sensorCol == 1) {
      sequencerDrum0814RowNum = sensorRow;
      updateDisplay();
    }
    else {
      handleNumericDataNewTouchCol(Project.sequencer[Global.currentPerSplit].seqDrumNotes[sequencerDrum0814RowNum - 1 + 7], 0, 127, true);
    }
  }
}

void handleSequencerDrum0814Release() {
  handleNumericDataReleaseCol(true);
}

void paintSequencerColors() {
  clearDisplay();

  paintShowSplitSelection(Global.currentPerSplit);

  setLed(7, 4, Split[Global.currentPerSplit].colorMain, cellOn);
  setLed(8, 5, Split[Global.currentPerSplit].colorAccent, cellOn);

  setLed(5, 3, Split[Global.currentPerSplit].colorSequencerEmpty, cellOn);
  setLed(6, 3, Split[Global.currentPerSplit].colorSequencerEmpty, cellOn);
  setLed(7, 3, Split[Global.currentPerSplit].colorSequencerEvent, cellOn);
  setLed(8, 3, Split[Global.currentPerSplit].colorSequencerEvent, cellOn);
  setLed(9, 3, Split[Global.currentPerSplit].colorSequencerDisabled, cellOn);
  setLed(10, 3, Split[Global.currentPerSplit].colorSequencerDisabled, cellOn);

  paintSequencerSettingsLowRow();
}

void handleSequencerColorsNewTouch() {
  if (sensorRow == 0) {
    handleSequencerSettingsLowRowTouch();
  }
  else if (ensureLegendHidden()) {
    if (sensorRow == 3) {
      if (sensorCol == 5 || sensorCol == 6) {
        Split[Global.currentPerSplit].colorSequencerEmpty = colorCycle(Split[Global.currentPerSplit].colorSequencerEmpty, false);
        updateDisplay();
      }
      else if (sensorCol == 7 || sensorCol == 8) {
        Split[Global.currentPerSplit].colorSequencerEvent = colorCycle(Split[Global.currentPerSplit].colorSequencerEvent, false);
        updateDisplay();
      }
      else if (sensorCol == 9 || sensorCol == 10) {
        Split[Global.currentPerSplit].colorSequencerDisabled = colorCycle(Split[Global.currentPerSplit].colorSequencerDisabled, false);
        updateDisplay();
      }
    }
  }
}

void handleSequencerColorsRelease() {
  handleShowSplit();
}


//
// SequencerPattern methods
//

void SequencerPattern::clear() {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    steps[s].clear();
  }
}

void SequencerPattern::operator=(const SequencerPattern& p) {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    steps[s] = p.steps[s];
  }
  stepSize = p.stepSize;
  sequencerDirection = p.sequencerDirection;
  loopScreen = p.loopScreen;
  swing = p.swing;
  length = p.length;
}


//
// StepData methods
//

void StepData::clear() {
  for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
    events[e].clear();
  }
}

void StepData::operator=(const StepData& d) {
  for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
    events[e] = d.events[e];
  }
}


//
// StepEvent methods
//
  
boolean StepEvent::hasData() {
  return getVelocity() > 0;
}

void StepEvent::clear() {
  memset(data, 0, 6);
}

void StepEvent::operator=(const StepEvent& e) {
  memcpy(data, e.data, 6);
}

void StepEvent::setNewEvent(byte note, byte velocity, unsigned short duration, byte timbre, byte row) {
  clear();
  setNote(note);
  setVelocity(velocity);
  setDuration(duration);
  setTimbre(timbre);
  setRow(row);
}

byte StepEvent::getNote() {
  return (data[0] & B11111110) >> 1;
}

void StepEvent::setNote(byte note) {
  data[0] = (data[0] & B00000001) | (note & B01111111) << 1;
}

unsigned short StepEvent::getDuration() {
  return (data[0] & B00000001) << 9 | (data[1] << 1) | (data[2] & B10000000) >> 7;
}

void StepEvent::setDuration(unsigned short duration) {
  byte data0 = (duration & (B00000001 << 9)) >> 9;
  byte data1 = (duration & (B11111111 << 1)) >> 1;
  byte data2 = (duration &  B00000001) << 7;
  data[0] = (data[0] & B11111110) | data0;
  data[1] = data1;
  data[2] = (data[2] & B01111111) | data2;
}

byte StepEvent::getVelocity() {
  return data[2] & B01111111;
}

void StepEvent::setVelocity(byte note) {
  data[2] = (data[2] & B10000000) | (note & B01111111);
}

signed char StepEvent::getPitchOffset() {
  return (signed char)data[3];
}

void StepEvent::setPitchOffset(signed char pitchOffset) {
  data[3] = pitchOffset;
}

byte StepEvent::getTimbre() {
  return (data[4] & B11111110) >> 1;
}

void StepEvent::setTimbre(byte timbre) {
  data[4] = (data[4] & B00000001) | (timbre & B01111111) << 1;
}

byte StepEvent::getRow() {
  return (data[4] & B00000001) << 2 | (data[5] & B11000000) >> 6;
}

void StepEvent::setRow(byte row) {
  data[4] = (data[4] & B11111110) | (row & B00000100) >> 2;
  data[5] = (data[5] & B00111111) | (row & B00000011) << 6;
}

int StepEvent::getFaderValue(byte fader) {
  switch (fader) {
    case 3:
      return getVelocity();
    case 2: {
      unsigned short duration = getDuration();
      for (short index = SEQ_DURATION_EDIT_PANEL_COUNT - 1; index >= 0; --index) {
        if (duration >= seqDurationEditPanelChoices[index]) {
          return index;
        }
      }
      return 0;
    }
    case 1:
      return getPitchOffset();
    case 0:
      return getTimbre();
  }
  return 0;
}

void StepEvent::setFaderValue(byte fader, int value) {
  switch (fader) {
    case 3:
      setVelocity(value);
      break;
    case 2:
      setDuration(seqDurationEditPanelChoices[value]);
      break;
    case 1:
      setPitchOffset(value);
      break;
    case 0:
      setTimbre(value);
      break;
  }
}

int StepEvent::getFaderMin(byte fader) {
  switch (fader) {
    case 3:
      return 1;
    case 2:
      return 0;
    case 1:
      return -50;
    case 0:
      return 0;
  }
  return 0;
}

int StepEvent::getFaderMax(byte fader) {
  switch (fader) {
    case 3:
      return 127;
    case 2:
      return SEQ_DURATION_EDIT_PANEL_COUNT-1;
    case 1:
      return 50;
    case 0:
      return 127;
  }
  return 0;
}

int StepEvent::getFaderNeutral(byte fader, byte split) {
  switch (fader) {
    case 3:
      return 96;
    case 2:
    {
      StepSequencerState& state = seqState[split];
      short duration = state.getCurrentPattern().stepSize;
      for (short index = SEQ_DURATION_EDIT_PANEL_COUNT - 1; index >= 0; --index) {
        if (duration >= seqDurationEditPanelChoices[index]) {
          return index;
        }
      }
      return 0;
    }
    case 1:
      return 0;
    case 0:
      return 0;
  }
  return 0;
}

boolean StepEvent::calculateSequencerFaderValue(boolean newVelocity) {
  if (sensorRow > 3) return 0;

  byte fader = sensorRow;
  int prev = getFaderValue(fader);
  int value = prev;
  byte sensitivity = 75;
  if (sensorRow == 2) {
    sensitivity = 150;
  }

  if (newVelocity) {
    sequencerFaderLastX[fader] = FXD_TO_INT(sensorCell->fxdInitialReferenceX());
  }

  short movedX = sensorCell->calibratedX() - sequencerFaderLastX[fader];
  if (abs(movedX) <= sensitivity) {
    return false;
  }

  unsigned long now = micros();

  byte increment = 1;
  if (calcTimeDelta(now, sequencerFaderChangeTime[fader]) < 70000) {
    increment = 5;
  }
  else if (calcTimeDelta(now, sequencerFaderChangeTime[fader]) < 80000) {
    increment = 3;
  }
  else if (calcTimeDelta(now, sequencerFaderChangeTime[fader]) < 100000) {
    increment = 2;
  }

  if (movedX > 0) {
    value = constrain(prev + increment, getFaderMin(fader), getFaderMax(fader));
  }
  else {
    value = constrain(prev - increment, getFaderMin(fader), getFaderMax(fader));
  }

  sequencerFaderChangeTime[fader] = now;
  sequencerFaderLastX[fader] = sensorCell->calibratedX();

  setFaderValue(fader, value);

  return prev != value;
}


//
// StepEventState methods
//

void StepEventState::reset() {
  if (isActive()) {
    sendNoteOff();
  }

  split = 0;
  note = -1;
  channel = 0;
  remainingDuration = 0;

  focused = false;
  pendingDelete = false;
  highlightedColumn = 0;
  highlightedRow = 0;
}

boolean StepEventState::isActive() {
  return remainingDuration > 0 && note >= 0;
}

void StepEventState::sendNoteOn(StepEvent& event, byte splitNum) {
  if (isActive()) {
    sendNoteOff();
  }

  split = splitNum;
  channel = takeChannel(split, event.getRow()-1);
  note = event.getNote();
  if (Split[split].sequencerView != sequencerDrums) {
    note = min(max(note + Split[splitNum].transposePitch + Split[splitNum].transposeOctave, 0), 127);
  }
  remainingDuration = event.getDuration();

  if (Split[split].sendX) {
    short range = getBendRange(split);
    midiSendPitchBend((event.getPitchOffset() * 8192) / (100 * range), channel);
  }

  if (Split[split].sendY) {
    preSendTimbre(split, event.getTimbre(), note, channel);
  }

  midiSendNoteOn(split, note, event.getVelocity(), channel);  

  StepSequencerState& state = seqState[split];
  if (Split[split].sequencerView == sequencerNotes) {
    if (state.running && !state.hasFocus()) {
      highlightCell(state, event);
    }
  }
}

void StepEventState::sendNoteOff() {
  if (note < 0) {
    return;
  }

  midiSendNoteOff(split, note, channel);
  releaseChannel(split, channel);

  if (Split[split].sequencerView == sequencerNotes) {
    unhighlightCell();
  }

  remainingDuration = 0;
  split = 0;
  note = -1;
  channel = 0;
}

void StepEventState::tick() {
  remainingDuration -= 1;
}

void StepEventState::highlightCell(StepSequencerState& state, StepEvent& event) {
  byte col, row;
  if (state.findNoteSequencerCoordinate(event, col, row)) {
    highlightedColumn = col;
    highlightedRow = row;
    setLed(col, row, Split[state.split].colorPlayed, cellOn, LED_LAYER_SEQUENCER);
  }
}

void StepEventState::unhighlightCell() {
  if (highlightedColumn != 0 && highlightedRow != 0) {
    setLed(highlightedColumn, highlightedRow, COLOR_OFF, cellOff, LED_LAYER_SEQUENCER);
    highlightedColumn = 0;
    highlightedRow = 0;
  }
}


//
// StepSequencerState methods
//

void StepSequencerState::init(byte sp) {
  clear();

  split = sp;
}

void StepSequencerState::clear() {
  for (byte q = 0; q < MAX_SEQUENCERS; ++q) {
    for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
      steps[s].focused = false;
      
      for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
        steps[s].events[e].reset();
      }
    }

    ticksUntilNextStep = 0;
    clock24PPQOffset = 0;
    positionOffset = 0;
    rowOffset = 0;
    currentPosition = -1;
    nextPosition = -1;
    currentPattern = 0;
    nextPattern = -1;
    switchPatternOnBeat = false;
    editing = false;
    running = false;
    muted = false;
    advancingForward = true;
    switch2Waiting = false;
    focused = false;
    focusedEvent = false;
    isBeingTurnedOff = false;

    previewEvent.reset();
  }
}

void StepSequencerState::handleStepEditingTouch(boolean newVelocity, int noteNum, byte stepNum) {
  if (noteNum < 0 || noteNum > 127) return;
  if (stepNum < 0 || stepNum >= MAX_SEQUENCER_STEPS) return;

  StepData& step = getCurrentPatternStep(stepNum);
  StepDataState& stepState = getStepState(stepNum);

  // if this is a new touch, determine if it should create a new event or delete an existing one
  if (newVelocity) {
    boolean handled = false;

    // iterate over this step's events to determine if this could be deleted
    // if not held for too long
    for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
      StepEvent& event = step.events[e];
      StepEventState& eventState = stepState.events[e];
      if (event.getNote() == noteNum && event.getRow() == sensorRow) {
        eventState.pendingDelete = true;
        handled = true;
      }
    }

    // if no event was deleted, add it in the first available event slot
    if (!handled) {
      for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
        StepEvent& event = step.events[e];
        if (!event.hasData()) {
          createNewEvent(noteNum, stepNum, e, event, stepState);
          handled = true;
          break;
        }
      }
    }

    // there are no free step events anymore, clear the oldest one
    if (!handled) {
      // clear the oldest event
      StepEvent& eventOldest = step.events[0];
      StepEventState& eventOldestState = stepState.events[0];
      if (eventOldest.hasData()) {
        eventOldestState.reset();
        eventOldest.clear();
      }

      // shift all events one upwards
      for (byte e = 1; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
        StepEvent& event = step.events[e];
        StepEventState& eventState = stepState.events[e];

        step.events[e-1] = event;
        stepState.events[e-1] = eventState;
        event.clear();
        eventState.reset();
      }

      // store the new event
      byte e = MAX_SEQUENCER_STEP_EVENTS-1;
      StepEvent& event = step.events[e];
      createNewEvent(noteNum, stepNum, e, event, stepState);

      paintSequencer();
    }
  }

  // detect whether a pending delete becomes a focused event
  for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
    StepEventState& eventState = stepState.events[e];

    // if the pending delete if past the hold time, transform is into a focused event
    if (eventState.pendingDelete) {
      if (calcTimeDelta(millis(), sensorCell->lastTouch) >= SENSOR_HOLD_DELAY) {
        eventState.pendingDelete = false;
        changeFocus(stepNum, e);
      }
    }
  }
}

void StepSequencerState::createNewEvent(int noteNum, byte stepNum, byte eventNum, StepEvent& event, StepDataState& stepState) {
  // create a new event
  event.setNewEvent(noteNum, sensorCell->velocity, getCurrentPattern().stepSize, sensorCell->calibratedY(), sensorRow);

  // set the focus to this event
  changeFocus(stepNum, eventNum);
}

void StepSequencerState::handleStepEditingRelease(int noteNum, byte stepNum) {
  if (noteNum < 0 || noteNum > 127) return;
  if (stepNum < 0 || stepNum >= MAX_SEQUENCER_STEPS) return;

  StepData& step = getCurrentPatternStep(stepNum);
  StepDataState& stepState = getStepState(stepNum);

  boolean deletedEvent = false;
  for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
    StepEvent& event = step.events[e];
    StepEventState& eventState = stepState.events[e];

    // if we deleted an event, copy the next ones one position upwards
    if (deletedEvent) {
      step.events[e-1] = event;
      stepState.events[e-1] = eventState,
      event.clear();
      eventState.reset();
    }

    // this event has a pending delete, perform the delete at touch release
    if (eventState.pendingDelete) {
        event.clear();

        if (eventState.focused) {
          focusedEvent = false;
        }
        eventState.reset();

        deletedEvent = true;
    }
  }

  // update the display
  if (deletedEvent) {
    paintSequencer();
  }
}

void StepSequencerState::turnOn() {
  if (!Split[split].sequencer) {
    return;
  }

  if (!isSyncedToMidiClock() && !sequencerIsRunning() && !isStandaloneMidiClockRunning()) {
    midiSendStart();
    midiSendTimingClock();
  }

  if (isSyncedToMidiClock()) {
    if (ticksUntilNextStep == 0) {
      int clockModulo = clock24PPQ % getCurrentPattern().stepSize;
      if (clockModulo == 0) {
        ticksUntilNextStep = 0;
      }
      else {
        ticksUntilNextStep = getCurrentPattern().stepSize - clockModulo;
      }
    }
    clock24PPQOffset = 0;
  }
  else {
    ticksUntilNextStep = 0;
    clock24PPQOffset = clock24PPQ;
  }

  if (getCurrentPattern().loopScreen) {
    nextPosition = positionOffset;
  }
  else if (nextPosition == -1) {
    nextPosition = 0;
  }

  running = true;
  switch2Waiting = false;
  if (!hasFocus() && !getCurrentPattern().loopScreen) {
    positionOffset = 0;
  }

  if (isVisibleSequencerForSplit(split)) {
    paintSequencer();
    updateSwitchLeds();
  }
}

void StepSequencerState::turnOff(boolean save) {
  if (!Split[split].sequencer) {
    return;
  }
  
  isBeingTurnedOff = true;
  running = false;
  currentPosition = -1;
  nextPosition = -1;
  ticksUntilNextStep = 0;
  clock24PPQOffset = 0;
  nextPattern = -1;
  switchPatternOnBeat = false;

  turnOffEvents();

  if (isVisibleSequencerForSplit(split)) {
    paintSequencer();
    updateSwitchLeds();
  }

  if (save) {
    storeSettings();
  }

  if (!isSyncedToMidiClock() && !sequencerIsRunning() && !isStandaloneMidiClockRunning()) {
    midiSendStop();
  }

  isBeingTurnedOff = false;
}

void StepSequencerState::turnOffEvents() {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
      StepEventState& eventState = steps[s].events[e];
      if (eventState.isActive()) {
        eventState.sendNoteOff();
      }
    }
  }
}

void StepSequencerState::clearAllFocus() {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    steps[s].focused = false;

    for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
      StepEventState& eventState = steps[s].events[e];
      eventState.focused = false;
    }
  }

  focused = false;
  focusedEvent = false;
}

void StepSequencerState::clearEventsFocus() {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
      StepEventState& eventState = steps[s].events[e];
      eventState.focused = false;
    }
  }
  focusedEvent = false;
}

boolean StepSequencerState::isRunning() {
  return running;
}

void StepSequencerState::advanceSequencer() {
  // handle the preview event's duration
  if (previewEvent.isActive()) {
    previewEvent.tick();
    if (previewEvent.remainingDuration == 0) {
      previewEvent.sendNoteOff();
      previewEvent.reset();
    }
  }

  // step sequencer advancement logic
  if (isRunning()) {

    // count down all active step events
    for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
      for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
        StepEventState& eventState = steps[s].events[e];
        if (eventState.isActive()) {
          eventState.tick();

          if (eventState.remainingDuration == 0) {
            eventState.sendNoteOff();

            performContinuousTasks();
          }
        }
      }
    }

    performContinuousTasks();

    // check if the sequencer should advance to the next position
    boolean repaintSequencer = false;
    if (ticksUntilNextStep == 0) {
      // prevent sleep from kicking in
      lastTouchMoment = millis();

      short position = 0;
      if (nextPosition >= 0) {
        position = nextPosition;
        nextPosition = -1;
      }
      else {
        int start = 0;
        int end = getCurrentPattern().length - 1;

        if (getCurrentPattern().loopScreen) {
          start = positionOffset;
          end = min(end, start + SEQ_EVENTS_WIDTH - 1);
        }

        switch (getCurrentPattern().sequencerDirection) {
          case sequencerForward:
            position = currentPosition + 1;
            if (position < start || position > end) {
              position = start;
            }
            advancingForward = true;
            break;
          case sequencerBackward:
            position = currentPosition - 1;
            if (position < start || position > end) {
              position = end;
            }
            advancingForward = false;
            break;
          case sequencerPingPong:
            if (advancingForward) {
              position = currentPosition + 1;
              if (position < start) {
                position = start;
              }
              else if (position > end) {
                position = end - 1;
                advancingForward = false;
              }
            }
            else {
              position = currentPosition - 1;
              if (position < start) {
                position = start + 1;
                advancingForward = true;
              }
              else if (position > end) {
                position = end;
              }
            }
          break;
        }

        // if there's no focused event, automatically switch between
        // the sequencer pages
        if (!hasFocus()) {
          if ((advancingForward && ((position - currentPosition == 1 && position % SEQ_EVENTS_WIDTH == 0) || (position == 0 && currentPosition == end))) ||
              (!advancingForward && ((currentPosition - position == 1 && position % SEQ_EVENTS_WIDTH == (SEQ_EVENTS_WIDTH-1)) || (position == end && currentPosition == 0)))) {
            positionOffset = (position / SEQ_EVENTS_WIDTH) * SEQ_EVENTS_WIDTH;
            repaintSequencer = true;
          }
        }
      }

      // check if the sequencer should switch to the next pattern
      if (nextPattern != -1 && (position == 0 || switchPatternOnBeat)) {
        position = 0;
        currentPattern = nextPattern;
        nextPattern = -1;
        switchPatternOnBeat = false;
        positionOffset = 0;
        clearAllFocus();
        if (isVisibleSequencerForSplit(split)) {
          seqState[split].paintSequencer();
        }
        else if (isVisibleSequencerForSplit(1 - split)) {
          paintPatternSelector();
        }
        repaintSequencer = false;
      }

      performContinuousTasks();

      // update the step timer
      ticksUntilNextStep = getCurrentPattern().stepSize;

      // adapt for swing
      if (getCurrentPattern().swing) {
        if (position % 2 == 0) {
          ticksUntilNextStep += ticksUntilNextStep / 6;
        }
        else {
          ticksUntilNextStep -= ticksUntilNextStep / 6;
        }
      }

      // switch to the next position
      setPosition(position);

      // repaint the sequencer when needed
      if (repaintSequencer) {
        repaintSequencer = false;
        if (isVisibleSequencerForSplit(split)) {
          paintSequencer();
        }
      }
    }

    ticksUntilNextStep -= 1;
  }
}

CellDisplay StepSequencerState::getEventCellDisplay(byte stepNum, byte eventNum) {
  if (steps[stepNum].focused && steps[stepNum].events[eventNum].focused) {
    return cellFocusPulse;
  }
  else {
    return cellOn;
  }
}

byte StepSequencerState::getPositionLedColor(byte stepNum) {
  byte color = Split[split].colorSequencerEmpty;

  if (isVisibleSequencerForSplit(split) && stepNum >= positionOffset && stepNum < positionOffset + SEQ_EVENTS_WIDTH) {
    if (getCurrentPatternStep(stepNum).events[0].hasData()) {
      color = Split[split].colorSequencerEvent;
    }

    if (stepNum >= getCurrentPattern().length) {
      color = Split[split].colorSequencerDisabled;
    }

    if (running && stepNum < getCurrentPattern().length && stepNum == currentPosition) {
      color = getCurrentPositionColor();
    }
  }

  return color;
}

void StepSequencerState::updatePositionLed(byte stepNum) {
  if (isVisibleSequencerForSplit(split) && stepNum >= positionOffset && stepNum < positionOffset + SEQ_EVENTS_WIDTH) {
    CellDisplay display = cellOn;

    if (steps[stepNum].focused) {
      display = cellFocusPulse;
    }

    if (sequencerCopyStepSource == stepNum) {
      display = cellFastPulse;
    }

    setLed(1 + stepNum - positionOffset, 0, getPositionLedColor(stepNum), display);
  }
}

byte StepSequencerState::getCurrentPositionColor() {
  if (Split[split].colorPlayed == COLOR_OFF) {
    return COLOR_RED;
  }
  return Split[split].colorPlayed;
}

byte StepSequencerState::getOtherPositionColor() {
  return Split[split].colorLowRow;
}

void StepSequencerState::setSongPositionPointer(unsigned spp) {
  SequencerPattern& pattern = getCurrentPattern();
  unsigned sppClockTicks = spp * 6;
  unsigned swingTicks = pattern.stepSize / 6;

  // calculate the number of MIDI ticks in an entire pattern
  unsigned patternTicks = pattern.length * pattern.stepSize;
  // adapt this number for swing timing in case the pattern doesn't have an even length
  if (pattern.swing && pattern.length % 2 == 1) {
    patternTicks += swingTicks;
  }

  short sppStep = (sppClockTicks % patternTicks) / pattern.stepSize;
  unsigned sppTicksRemaining = 0;

  // unless we're right on the step boundary with the song position pointer,
  // adapt the next step to be the following one with the appropriate ticks remaining
  // to get there
  boolean alignedStepBoundary = (sppClockTicks % pattern.stepSize == 0);
  if ((!pattern.swing && !alignedStepBoundary) ||
      (pattern.swing && spp != 0)) {
    sppStep += 1;
    sppTicksRemaining = (sppStep * pattern.stepSize) - (sppClockTicks % patternTicks);
  }

  // adapt for swing timing
  if (pattern.swing) {
    // handle swing 0-based beat numbers that are odd
    if (sppStep % 2 == 1) {
      sppTicksRemaining += swingTicks;
    }
    // determine if we're not into the initial ticks of the even beat numbers
    // that should actually still be part of the odd beat duration
    else if (sppStep > 0 && pattern.stepSize - sppTicksRemaining < swingTicks) {
      sppStep -= 1;
      sppTicksRemaining = swingTicks - (pattern.stepSize - sppTicksRemaining);
    }
  }

  short stepNum = sppStep % pattern.length;

  nextPosition = stepNum;
  ticksUntilNextStep = sppTicksRemaining;
}

void StepSequencerState::setPosition(byte stepNum) {
  byte previousPosition = currentPosition;

  currentPosition = stepNum;

  StepData& step = getCurrentPatternStep(stepNum);
  for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
    StepEvent& event = step.events[e];
    // check if the event has data for the current step
    if (event.hasData()) {
      StepEventState& eventState = steps[stepNum].events[e];
      // if this event still is ongoing, stop it first
      if (eventState.isActive()) {
        eventState.sendNoteOff();
      }
      if (!muted && stepNum < getCurrentPattern().length) {
        // start the event
        eventState.sendNoteOn(event, split);
      }
    }
  }

  updatePositionLed(previousPosition);
  updatePositionLed(currentPosition);
}

SequencerPattern& StepSequencerState::getCurrentPattern() {
  return Project.sequencer[split].patterns[currentPattern];
}

StepData& StepSequencerState::getCurrentPatternStep(byte stepNum) {
  SequencerPattern& pattern = getCurrentPattern();
  return pattern.steps[stepNum];
}

byte StepSequencerState::getSensorSequencerPosition() {
  return sensorCol - 1 + positionOffset;
}

int StepSequencerState::getRowNoteNum(byte noteRow) {
  switch (Split[split].sequencerView) {
    case sequencerNotes:
      return -1;
    case sequencerScales: {
      int row = -1;
      for (byte i = 0; i < 128; ++i) {
        if (((Global.mainNotes[Global.activeNotes] >> (i % 12)) & 1) ||
            ((Global.accentNotes[Global.activeNotes] >> (i % 12)) & 1)) {
          row += 1;
        }
        if (noteRow + rowOffset == row) {
          return min(max(i + 48, 0), 127);
        }
      }
      return -1;
    }
    case sequencerDrums: {
      byte row = noteRow + rowOffset;
      if (row < SEQ_DRUM_NOTES) {
        return Project.sequencer[split].seqDrumNotes[row];
      }
      return -1;
    }
  }
  return -1;
}

byte StepSequencerState::getSensorNotesNoteNum() {
  return getNoteNumber(split, sensorCol, sensorRow);
}

int StepSequencerState::getSensorRowNoteNum() {
  return getRowNoteNum(sensorRow - 1);
}

StepDataState& StepSequencerState::getStepState(byte stepNum) {
  return steps[stepNum];
}

StepData& StepSequencerState::getSensorStep() {
  return getCurrentPatternStep(getSensorSequencerPosition());
}

StepDataState& StepSequencerState::getSensorStepState() {
  return getStepState(getSensorSequencerPosition());
}

boolean StepSequencerState::findNoteSequencerCoordinate(StepEvent& event, byte& col, byte& row) {
  boolean foundMatch = false;
  short rowDistance = 0;
  while (!foundMatch && abs(rowDistance) < NUMROWS) {
    short r = event.getRow() + rowDistance;
    if (r >= 0 && r < NUMROWS) {
      for (byte c = 1; c <= SEQ_EVENTS_WIDTH; ++c) {
        short noteNum = getNoteNumber(split, c, r);
        if (noteNum == event.getNote()) {
          event.setRow(r);
          col = c;
          row = r;
          foundMatch = true;
          break;
        }
      }
    }
    if (rowDistance <= 0) {
      rowDistance = abs(rowDistance) + 1;
    }
    else {
      rowDistance = -rowDistance;
    }
  }
  return foundMatch;
}

void StepSequencerState::paintCurrentPatternStep(byte stepNum) {
  StepData& step = seqState[split].getCurrentPatternStep(stepNum);
  for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
    StepEvent& event = step.events[e];
    if (event.hasData()) {
      switch (Split[split].sequencerView) {
        case sequencerNotes:
        {
          byte col, row;
          if (findNoteSequencerCoordinate(event, col, row)) {
            setLed(col, row, Split[split].colorPlayed, seqState[split].getEventCellDisplay(stepNum, e));
          }
          break;
        }

        case sequencerScales:
        case sequencerDrums: {
          for (byte seqRow = 0; seqRow < 7; ++seqRow) {
            short seqCol = stepNum - positionOffset;
            if (seqCol >= 0 && seqCol < SEQ_EVENTS_WIDTH && getRowNoteNum(seqRow) == event.getNote()) {
              // determine the cell's color based on the velocity value
              byte cellColor = COLOR_OFF;
              if (event.getVelocity() > 96) {
                cellColor = Split[split].colorAccent;
              }
              else {
                cellColor = Split[split].colorMain;
              }

              byte row = 1 + seqRow;
              event.setRow(row);
              setLed(1 + seqCol, row, cellColor, seqState[split].getEventCellDisplay(stepNum, e));
              break;
            }
          }
          break;
        }
      }
    }
    performContinuousTasks();
  }
}

void StepSequencerState::clearSequencer() {
  for (byte col = 1; col <= SEQ_EVENTS_WIDTH; ++col) {
    for (byte row = 1; row < 8; ++row) {
      clearLed(col, row);
    }
    performContinuousTasks();
  }
}

void StepSequencerState::paintSequencer() {
  startBufferedLeds();
  paintSequencerUnbuffered();
  finishBufferedLeds();
}

void StepSequencerState::paintSequencerUnbuffered() {
  switch (Split[split].sequencerView) {
    case sequencerNotes:
      for (byte row = 1; row < 8; ++row) {
        for (byte col = 1; col <= SEQ_EVENTS_WIDTH; ++col) {
          paintNormalDisplayCell(split, col, row);
        }
        performContinuousTasks();
      }

      byte stepNum;
      if (getFocusStep(stepNum)) {
        paintCurrentPatternStep(stepNum);
      }
      break;
    case sequencerScales:
    case sequencerDrums:
      for (byte row = 1; row < 8; ++row) {
        for (byte col = 1; col <= SEQ_EVENTS_WIDTH; ++col) {
          clearLed(col, row);
        }
        performContinuousTasks();
      }

      for (byte seqCol = 0; seqCol < SEQ_EVENTS_WIDTH; ++seqCol) {
        paintCurrentPatternStep(seqCol + positionOffset);
      }
      break;
  }
  performContinuousTasks();
  paintLowRow();
  performContinuousTasks();
  paintMuter();
  paintPatternSelector();
  paintPerformanceSettings();
  performContinuousTasks();
  paintFocusFaders();
  performContinuousTasks();
  paintNavigation();
}

void StepSequencerState::paintLowRow() {
  for (byte seqCol = 0; seqCol < SEQ_EVENTS_WIDTH; ++seqCol) {
    byte pos = seqCol + positionOffset;
    updatePositionLed(pos);
  }
}

void StepSequencerState::paintMuter() {
  setLed(SEQ_MUTER_COLUMN, SEQ_MUTER_TOP, seqState[LEFT].muted ? COLOR_RED : COLOR_OFF, cellOn);
  setLed(SEQ_MUTER_COLUMN, SEQ_MUTER_BOTTOM, seqState[RIGHT].muted ? COLOR_RED : COLOR_OFF, cellOn);
}

void StepSequencerState::paintPatternSelector() {
  for (byte pattern = 0; pattern < MAX_SEQUENCER_PATTERNS; ++pattern) {
    int col = SEQ_PATTERN_SELECTOR_LEFT + pattern;
    byte leftColor = (pattern == seqState[LEFT].currentPattern ? Split[LEFT].colorAccent : Split[LEFT].colorMain);
    CellDisplay leftDisplay = (pattern == seqState[LEFT].nextPattern ? cellSlowPulse : cellOn);
    if (sequencerCopySplitSource == LEFT && sequencerCopyPatternSource == pattern) {
      leftDisplay = cellFastPulse;
    }

    setLed(col, SEQ_PATTERN_SELECTOR_TOP, leftColor, leftDisplay);

    byte rightColor = (pattern == seqState[RIGHT].currentPattern ? Split[RIGHT].colorAccent : Split[RIGHT].colorMain);
    CellDisplay rightDisplay = (pattern == seqState[RIGHT].nextPattern ? cellSlowPulse : cellOn);
    if (sequencerCopySplitSource == RIGHT && sequencerCopyPatternSource == pattern) {
      rightDisplay = cellFastPulse;
    }

    setLed(col, SEQ_PATTERN_SELECTOR_BOTTOM, rightColor, rightDisplay);
  }
}

void StepSequencerState::paintPerformanceSettings() {
  CellDisplay modeCellDisplayTop = cellOff;
  CellDisplay modeCellDisplayBottom = cellOff;
  switch (Split[split].sequencerView) {
    case sequencerNotes:
      modeCellDisplayTop = cellOn;
      break;
    case sequencerScales:
      modeCellDisplayTop = cellOn;
      modeCellDisplayBottom = cellOn;
      break;
    case sequencerDrums:
      modeCellDisplayBottom = cellOn;
      break;
  }
  setLed(SEQ_VIEW_COLUMN, SEQ_VIEW_TOP, Split[split].colorMain, modeCellDisplayTop);
  setLed(SEQ_VIEW_COLUMN, SEQ_VIEW_BOTTOM, Split[split].colorMain, modeCellDisplayBottom);

  CellDisplay stepSize1CellDisplayTop = cellOff;
  CellDisplay stepSize1CellDisplayBottom = cellOff;
  switch (getCurrentPattern().stepSize) {
    case StepSixteenth:
    case StepSixteenthDotted:
    case StepSixteenthTriplet:
      stepSize1CellDisplayTop = cellOn;
      stepSize1CellDisplayBottom = cellOff;
      break;
    case StepFourth:
    case StepFourthDotted:
    case StepFourthTriplet:
      stepSize1CellDisplayTop = cellOn;
      stepSize1CellDisplayBottom = cellOn;
      break;
    case StepEighth:
    case StepEighthDotted:
    case StepEighthTriplet:
      stepSize1CellDisplayTop = cellOff;
      stepSize1CellDisplayBottom = cellOn;
      break;
  }
  setLed(SEQ_STEPSIZE_LEFT, SEQ_STEPSIZE_TOP, Split[split].colorMain, stepSize1CellDisplayTop);
  setLed(SEQ_STEPSIZE_LEFT, SEQ_STEPSIZE_BOTTOM, Split[split].colorMain, stepSize1CellDisplayBottom);

  CellDisplay stepSize2CellDisplayTop = cellOff;
  CellDisplay stepSize2CellDisplayBottom = cellOff;
  if (getCurrentPattern().swing) {
    stepSize2CellDisplayTop = cellOn;
    stepSize2CellDisplayBottom = cellOn;
  }
  else {
    switch (getCurrentPattern().stepSize) {
      case StepEighth:
      case StepFourth:
      case StepSixteenth:
        stepSize2CellDisplayTop = cellOff;
        stepSize2CellDisplayBottom = cellOff;
        break;
      case StepEighthDotted:
      case StepFourthDotted:
      case StepSixteenthDotted:
        stepSize2CellDisplayTop = cellOn;
        stepSize2CellDisplayBottom = cellOff;
        break;
      case StepEighthTriplet:
      case StepFourthTriplet:
      case StepSixteenthTriplet:
        stepSize2CellDisplayTop = cellOff;
        stepSize2CellDisplayBottom = cellOn;
        break;
    }
  }
  setLed(SEQ_STEPSIZE_RIGHT, SEQ_STEPSIZE_TOP, Split[split].colorMain, stepSize2CellDisplayTop);
  setLed(SEQ_STEPSIZE_RIGHT, SEQ_STEPSIZE_BOTTOM, Split[split].colorMain, stepSize2CellDisplayBottom);

  setLed(SEQ_LOOPSCREEN_COLUMN, SEQ_LOOPSCREEN_ROW, Split[split].colorMain, getCurrentPattern().loopScreen ? cellOn : cellOff);

  CellDisplay directionCellDisplay = cellOff;
  if (getCurrentPattern().sequencerDirection != sequencerForward) {
    directionCellDisplay = cellOn;
  }
  setLed(SEQ_DIRECTION_COLUMN, SEQ_DIRECTION_ROW, getDirectionColor(), directionCellDisplay);
}

byte StepSequencerState::getDirectionColor() {
  if (getCurrentPattern().sequencerDirection == sequencerPingPong) {
    return Split[split].colorAccent;
  }
  return Split[split].colorMain;
}

void StepSequencerState::paintNavigation() {
  if (isSequencerNavigationAreaVisible()) {
    int highlightIndex = SEQ_NAVIGATION_LEFT + positionOffset/SEQ_EVENTS_WIDTH;
    boolean highlightTop = (rowOffset != 0 && Split[split].sequencerView != sequencerNotes);

    for (byte c = SEQ_NAVIGATION_LEFT; c <= SEQ_NAVIGATION_RIGHT; ++c) {
      setLed(c, SEQ_NAVIGATION_BOTTOM, (c == highlightIndex && !highlightTop ? getCurrentPositionColor(): getOtherPositionColor()), cellOn);
      switch (Split[split].sequencerView) {
        case sequencerNotes:
          clearLed(c, SEQ_NAVIGATION_TOP);
          break;
        case sequencerScales:
        case sequencerDrums:
          setLed(c, SEQ_NAVIGATION_TOP, (c == highlightIndex && highlightTop ? getCurrentPositionColor(): getOtherPositionColor()), cellOn);
          break;
      }
    }
  }
}

void StepSequencerState::paintFocusFaders() {
  byte focusStepNum;
  StepEvent* focus = getFocusEvent(focusStepNum);

  if (focus) {
    paintFocusFader(3, focus->getVelocity());
    paintDurationFader(2, focus->getDuration());
    paintPitchOffsetFader(1, focus->getPitchOffset());
    paintFocusFader(0, focus->getTimbre());
  }
  else {
    for (byte row = 0; row <= 3; ++row) {
      clearFocusFader(row);
    }
  }
}

void StepSequencerState::clearFocusFader(byte row) {
  for (byte col = SEQ_FADER_LEFT; col <= SEQ_FADER_RIGHT; ++col) {
    clearLed(col, row);
  }
}

void StepSequencerState::paintFocusFader(byte row, byte value) {
  int32_t fxdFaderPosition = fxdCalculateFaderPosition(value, SEQ_FADER_LEFT, SEQ_FADER_LENGTH) - FXD_CALX_HALF_UNIT;

  for (byte col = SEQ_FADER_RIGHT; col >= SEQ_FADER_LEFT; --col) {
    if (Device.calRows[col][0].fxdReferenceX - FXD_CALX_HALF_UNIT > fxdFaderPosition) {
      setLed(col, row, Split[split].colorMain, cellOn);
    }
    else {
      setLed(col, row, Split[split].colorAccent, cellOn);
    }
  }
}

void StepSequencerState::paintDurationFader(byte row, unsigned short duration) {
  short index;
  for (index = SEQ_DURATION_EDIT_PANEL_COUNT - 1; index >= 0; --index) {
    if (duration >= seqDurationEditPanelChoices[index]) {
      break;
    }
  }

  CellDisplay display;
  if (duration <= seqDurationEditPanelChoices[SEQ_DURATION_EDIT_PANEL_COUNT - 1]) {
    display = cellOn;
  }
  else {
    display = cellFocusPulse;
  }

  int faderPosition = SEQ_FADER_LEFT + FXD_TO_INT(FXD_MUL(FXD_FROM_INT(index), FXD_SEQ_DURATION_FADER_RATIO));
  for (byte col = SEQ_FADER_RIGHT; col >= SEQ_FADER_LEFT; --col) {
    if (col > faderPosition) {
      setLed(col, row, Split[split].colorMain, display);
    }
    else {
      setLed(col, row, Split[split].colorAccent, display);
    }
  }
}

void StepSequencerState::paintPitchOffsetFader(byte row, short pitchOffset) {
  int32_t fxdFaderPosition = fxdCalculateFaderPosition(abs(pitchOffset), SEQ_FADER_LEFT, SEQ_FADER_LENGTH, FXD_CONST_50) - FXD_CALX_HALF_UNIT;

  if (pitchOffset >= 0) {
    for (byte col = SEQ_FADER_RIGHT; col >= SEQ_FADER_LEFT; --col) {
      if (Device.calRows[col][0].fxdReferenceX - FXD_CALX_HALF_UNIT > fxdFaderPosition) {
        clearLed(col, row);
      }
      else {
        setLed(col, row, Split[split].colorAccent, cellOn);
      }
    }
  }
  else {
    for (byte col = SEQ_FADER_LEFT; col <= SEQ_FADER_RIGHT; ++col) {
      if (Device.calRows[SEQ_FADER_RIGHT - col + SEQ_FADER_LEFT][0].fxdReferenceX - FXD_CALX_HALF_UNIT > fxdFaderPosition) {
        clearLed(col, row);
      }
      else {
        setLed(col, row, Split[split].colorPlayed, cellOn);
      }
    }
  }
}

void StepSequencerState::clearFocusState() {
  if (!focused) {
    return;
  }

  focused = false;
  focusedEvent = false;

  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    steps[s].focused = false;
    for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
      steps[s].events[e].focused = false;
    }
  }

  paintSequencer();
}

void StepSequencerState::updateFocusState() {
  focused = false;
  focusedEvent = false;
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    if (steps[s].focused) {
      focused = true;
      for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
        if (steps[s].events[e].focused) {
          focusedEvent = true;
          return;
        }
      }
      return;
    }
  }
}

boolean StepSequencerState::hasFocus() {
  return focused;
}

boolean StepSequencerState::getFocusStep(byte& stepNum) {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    if (steps[s].focused) {
      stepNum = s;
      return true;
    }
  }
  return false;
}

StepEvent* StepSequencerState::getFocusEvent(byte& stepNum) {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    if (steps[s].focused) {
      for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
        if (steps[s].events[e].focused) {
          stepNum = s;
          return &getCurrentPatternStep(s).events[e];
        }
      }
      performContinuousTasks();
    }
  }
  return NULL;
}

boolean StepSequencerState::hasFocusEvent() {
  return focusedEvent;
}

void StepSequencerState::changeFocus(byte stepNum, short eventNum) {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    if (s != stepNum && !steps[s].focused) continue;

    for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
      setEventFocus(s, e, (stepNum == s) && (eventNum == e));
    }

    boolean focusedStep = (stepNum == s);
    boolean refresh = (steps[s].focused != focusedStep || s == stepNum);

    steps[s].focused = focusedStep;
    if (refresh) {
      paintCurrentPatternStep(s);
    }
  }

  updateFocusState();

  paintLowRow();
  paintFocusFaders();
  paintNavigation();
}

void StepSequencerState::setEventFocus(byte stepNum, short eventNum, boolean focused) {
  StepEventState& eventState = steps[stepNum].events[eventNum];
  eventState.focused = focused;

  if (focused) {
    StepData& step = seqState[split].getCurrentPatternStep(stepNum);
    StepEvent& event = step.events[eventNum];
    if (!running && !muted) {
      previewEvent.sendNoteOn(event, split);
    }
  }
}

void StepSequencerState::cycleFocus(byte stepNum) {
  for (byte s = 0; s < MAX_SEQUENCER_STEPS; ++s) {
    // turn off focus of other steps
    if (s != stepNum) {
      if (steps[s].focused) {
        for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
          steps[s].events[e].focused = false;
        }
        steps[s].focused = false;
      }
    }
    // cycle through the focus of the step
    else {
      StepData& step = seqState[split].getCurrentPatternStep(stepNum);
      boolean focusedStep = false;

      // if no focus exists
      if (!steps[s].focused) {
        switch (Split[split].sequencerView) {
          case sequencerNotes: {
            int focusedEventIdx = -1;
            for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
              StepEvent& event = step.events[e];
              if (event.hasData() &&
                  (focusedEventIdx == -1 || (event.getNote() < step.events[focusedEventIdx].getNote() || event.getRow() < step.events[focusedEventIdx].getRow()))) {
                focusedEventIdx = e;
              }
            }
            if (focusedEventIdx >= 0) {
              setEventFocus(s, focusedEventIdx, true);
            }
            break;
          }

          case sequencerScales:
          case sequencerDrums: {
            // focus on the lowest row event, then step focusing
            for (byte seqRow = 0; seqRow < 8 && !focusedStep; ++seqRow) {
              for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
                StepEvent& event = step.events[e];
                if (event.hasData() && getRowNoteNum(seqRow) == event.getNote()) {
                  setEventFocus(s, e, true);
                  focusedStep = true;
                  break;
                }
              }
            }
            break;
          }
        }

        // even if no event was focused, focus on the step
        focusedStep = true;
      }
      // if a focus exists, focus the next event on a higher row,
      // if there are no higher events, turn off focus for this step
      else {
        boolean unfocusedEvent = false;
        switch (Split[split].sequencerView) {
          case sequencerNotes: {
            int previousFocusedEventIdx = -1;
            for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
              if (steps[s].events[e].focused) {
                previousFocusedEventIdx = e;
                steps[s].events[e].focused = false;
                break;
              }
            }

            if (previousFocusedEventIdx >= 0) {
              int focusedEventIdx = -1;
              StepEvent& previousFocusedEvent = step.events[previousFocusedEventIdx];
              for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
                StepEvent& event = step.events[e];
                if (event.hasData() &&
                    (focusedEventIdx == -1 || (event.getNote() < step.events[focusedEventIdx].getNote() || event.getRow() < step.events[focusedEventIdx].getRow())) &&
                    (event.getNote() > previousFocusedEvent.getNote() || event.getRow() > previousFocusedEvent.getRow())) {
                  focusedEventIdx = e;
                }
              }
              if (focusedEventIdx >= 0) {
                setEventFocus(s, focusedEventIdx, true);
                focusedStep = true;
              }
            }

            break;
          }

          case sequencerScales:
          case sequencerDrums: {
            for (byte seqRow = 0; seqRow < 8 && !focusedStep; ++seqRow) {
              for (byte e = 0; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {
                StepEvent& event = step.events[e];
                if (event.hasData() && getRowNoteNum(seqRow) == event.getNote()) {
                  if (!unfocusedEvent && steps[s].events[e].focused) {
                    steps[s].events[e].focused = false;
                    unfocusedEvent = true;
                  }
                  else if (unfocusedEvent && !focusedStep && !steps[s].events[e].focused) {
                    setEventFocus(s, e, true);
                    focusedStep = true;
                    break;
                  }
                }
              }
            }
            break;
          }
        }
      }

      steps[s].focused = focusedStep;
    }
  }

  updateFocusState();

  paintSequencer();
}

void StepSequencerState::selectPreviousPattern() {
  int p = currentPattern;
  if (nextPattern != -1) {
    p = nextPattern;
  }
  p -= 1;
  if (p < 0) {
    p += MAX_SEQUENCER_PATTERNS;
  }
  selectPattern(p);
}

void StepSequencerState::selectNextPattern() {
  int p = currentPattern;
  if (nextPattern != -1) {
    p = nextPattern;
  }
  p = (p+1) % MAX_SEQUENCER_PATTERNS;
  selectPattern(p);
}

void StepSequencerState::selectPattern(byte pattern) {
  // clear the next pattern if the current pattern is selected again
  if (currentPattern == pattern) {
      if (nextPattern != -1) {
        nextPattern = -1;
        switchPatternOnBeat = false;
      }
  }
  else {
    // when the sequencer is not running, switch immediately
    if (!isRunning()) {
      currentPattern = pattern;
      nextPattern = -1;
      switchPatternOnBeat = false;
      clearAllFocus();
      if (isVisibleSequencerForSplit(split)) {
        paintSequencer();
      }
    }
    else {
      // a double tap on an already scheduled next pattern, schedules it at the beginning of the next beat
      if (nextPattern == pattern) {
        switchPatternOnBeat = true;
      }
      // schedule the pattern as the next one
      else {
        nextPattern = pattern;
        switchPatternOnBeat = false;
      }
    }
  }
  if (isVisibleSequencer()) {
    paintPatternSelector();
  }
}
