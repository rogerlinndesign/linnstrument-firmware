/********************************** ls_switches: LinnStrument Switches ****************************
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
These routines handle LinnStrument's 2 button switches and 2 foot switch inputs.

The 2 foot switch input lines are pulled up so a read of HIGH means foot switch is open or not
connected:
In order to accomodate both normally-open and normally-closed foot switches, unpressed switch state
is read on startup and saved in leftFootSwitchOffState and rightFootSwitchOffState. When switches
are read subsequently, state is compared to these OFF states to insure valid presses for both
normally-open and normally-closed switches.
**************************************************************************************************/

void initializeSwitches() {
  // read initial state of each in order to determine if nornally-open or
  // normally-closed (like VFP2) switches are connected, or if nothing's connected.
  footSwitchOffState[SWITCH_FOOT_L] = digitalRead(FOOT_SW_LEFT);
  footSwitchOffState[SWITCH_FOOT_R] = digitalRead(FOOT_SW_RIGHT);

  footSwitchState[SWITCH_FOOT_L] = false;
  footSwitchState[SWITCH_FOOT_R] = false;
  footSwitchState[SWITCH_FOOT_B] = false;

  lastSwitchPress[SWITCH_FOOT_L] = 0;
  lastSwitchPress[SWITCH_FOOT_R] = 0;
  lastSwitchPress[SWITCH_FOOT_B] = 0;
  lastSwitchPress[SWITCH_SWITCH_2] = 0;
  lastSwitchPress[SWITCH_SWITCH_1] = 0;

  switchState[SWITCH_FOOT_L][LEFT] = false;
  switchState[SWITCH_FOOT_R][LEFT] = false;
  switchState[SWITCH_FOOT_B][LEFT] = false;
  switchState[SWITCH_SWITCH_2][LEFT] = false;
  switchState[SWITCH_SWITCH_1][LEFT] = false;

  switchState[SWITCH_FOOT_L][RIGHT] = false;
  switchState[SWITCH_FOOT_R][RIGHT] = false;
  switchState[SWITCH_FOOT_B][RIGHT] = false;
  switchState[SWITCH_SWITCH_2][RIGHT] = false;
  switchState[SWITCH_SWITCH_1][RIGHT] = false;

  for (byte i = 0; i < MAX_ASSIGNED; ++i) {
    switchTargetEnabled[LEFT][i] = false;
    switchTargetEnabled[RIGHT][i] = false;
  }

  for (byte i = 0; i < 128; ++i) {
    switchCCEnabled[LEFT][i] = false;
    switchCCEnabled[RIGHT][i] = false;
  }
}

boolean isStatefulSwitchAssignment(byte assignment) {
  return assignment == ASSIGNED_AUTO_OCTAVE ||
         assignment == ASSIGNED_SUSTAIN ||
         assignment == ASSIGNED_CC_65 ||
         assignment == ASSIGNED_ARPEGGIATOR ||
         assignment == ASSIGNED_LEGATO ||
         assignment == ASSIGNED_LATCH ||
         assignment == ASSIGNED_REVERSE_PITCH_X ||
         assignment == ASSIGNED_STANDALONE_MIDI_CLOCK;
}

void doSwitchPressed(byte whichSwitch) {
  byte assignment = Global.switchAssignment[whichSwitch];
  if (!Global.splitActive || assignment == ASSIGNED_ALTSPLIT || !Global.switchBothSplits[whichSwitch]) {
    doSwitchPressedForSplit(whichSwitch, assignment, Global.currentPerSplit);
  }
  else {
    doSwitchPressedForSplit(whichSwitch, assignment, LEFT);
    doSwitchPressedForSplit(whichSwitch, assignment, RIGHT);
  }
}

boolean isSwitchTargetEnabled(byte whichSwitch, byte assignment, byte split) {
  if (assignment == ASSIGNED_CC_65) {
    return isSwitchCC65CCEnabled(whichSwitch, split);
  }
  else if (assignment == ASSIGNED_SUSTAIN) {
    return isSwitchSustainCCEnabled(whichSwitch, split);
  }
  else {
    return switchTargetEnabled[split][assignment];
  }
}

void doSwitchPressedForSplit(byte whichSwitch, byte assignment, byte split) {
  // the switches on the LinnStrument trigger immediately,
  // the foot switches trigger immediately when they're stateful,
  // or when there's no action for the both footswitches press,
  // otherwise they trigger on release
  if (whichSwitch == SWITCH_SWITCH_1 || whichSwitch == SWITCH_SWITCH_2 ||
      Global.switchAssignment[SWITCH_FOOT_B] == ASSIGNED_DISABLED ||
      isStatefulSwitchAssignment(assignment)) {
    doSwitchTriggeredForSplit(whichSwitch, assignment, split);
  }
 }

void doSwitchTriggeredForSplit(byte whichSwitch, byte assignment, byte split) {
  // by default, the state of this switch will be on when pressed,
  // unless it's determined later to be a toggle action
  boolean resultingState = true;

  // perform the switch assignment on action if this hasn't been enabled
  // by another switch yet
  if (!isSwitchTargetEnabled(whichSwitch, assignment, split)) {
    performSwitchAssignmentOn(whichSwitch, assignment, split);
  }
  // if the switch is enabled for the split,
  // go through the assignment off logic
  else {
    // turn the switch state off
    resultingState = false;
    performSwitchAssignmentOff(whichSwitch, assignment, split);
  }

  // change the state of the switch and the assignment based on the previous logic
  changeSwitchState(whichSwitch, assignment, split, resultingState);

  // keep track of when the last press happened to be able to differentiate
  // between toggle and hold
  lastSwitchPress[whichSwitch] = millis();
}

void doSwitchReleased(byte whichSwitch) {
  byte assignment = Global.switchAssignment[whichSwitch];
  if (!Global.splitActive || assignment == ASSIGNED_ALTSPLIT || !Global.switchBothSplits[whichSwitch]) {
    doSwitchReleasedForSplit(whichSwitch, assignment, Global.currentPerSplit);
  }
  else {
    doSwitchReleasedForSplit(whichSwitch, assignment, LEFT);
    doSwitchReleasedForSplit(whichSwitch, assignment, RIGHT);
  }
}

void doSwitchReleasedForSplit(byte whichSwitch, byte assignment, byte split) {
  // check whether this is a hold operation by comparing the release time with
  // the last time the switch was pressed
  boolean isHeld = (calcTimeDelta(millis(), lastSwitchPress[whichSwitch]) > SWITCH_HOLD_DELAY);

  // foot switches have no hold or toggle havior based on time, but rather based on function
  if (whichSwitch == SWITCH_FOOT_L || whichSwitch == SWITCH_FOOT_R || whichSwitch == SWITCH_FOOT_B) {
    if (isStatefulSwitchAssignment(assignment)) {
      isHeld = true;
    }
    else {
      isHeld = false;

      if (whichSwitch == SWITCH_FOOT_B) {
        switchFootBothReleased = true;
      }
      if (whichSwitch == SWITCH_FOOT_B || !switchFootBothReleased) {
        if (Global.switchAssignment[SWITCH_FOOT_B] != ASSIGNED_DISABLED) {
          doSwitchTriggeredForSplit(whichSwitch, assignment, split);
        }
      }
      if (switchFootBothReleased && !footSwitchState[SWITCH_FOOT_L] && !footSwitchState[SWITCH_FOOT_R]) {
        switchFootBothReleased = false;
      }
    }
  }

  if (isHeld && isStatefulSwitchAssignment(assignment)) {
    // perform the assignment off logic, but when a split is active, it's possible that the
    // switch started being held on the other split, so we need to check which split is actually
    // active before changing the state
    if (Global.splitActive) {
      if (isSwitchTargetEnabled(whichSwitch, assignment, LEFT)) {
        performSwitchAssignmentHoldOff(whichSwitch, assignment, LEFT);
        changeSwitchState(whichSwitch, assignment, LEFT, false);
      }
      if (isSwitchTargetEnabled(whichSwitch, assignment, RIGHT)) {
        performSwitchAssignmentHoldOff(whichSwitch, assignment, RIGHT);
        changeSwitchState(whichSwitch, assignment, RIGHT, false);
      }
    }
    else {
      if (isSwitchTargetEnabled(whichSwitch, assignment, split)) {
        performSwitchAssignmentHoldOff(whichSwitch, assignment, split);
        changeSwitchState(whichSwitch, assignment, split, false);
      }
    }
  }
  // this is a toggle action, however only some assignment have toggle behavior
  // only proceed when the switch is on
  else if (switchState[whichSwitch][split] || assignment == ASSIGNED_ALTSPLIT) {
    // non-stateful assignments don't have visible toggle behaviours, they're more
    // like one shot triggers, never keep the switch on after release for these assignments
    if (!isStatefulSwitchAssignment(assignment)) {
      changeSwitchState(whichSwitch, assignment, split, false);
    }
  }
}

void changeSwitchState(byte whichSwitch, byte assignment, byte split, boolean enabled) {
  // all switches are mutually exclusive, so we always reset the state of other switches with the same target assignment
  for (byte sw = 0; sw < 4; ++sw) {
    if (sw != whichSwitch && Global.switchAssignment[sw] == assignment) {
      switchState[sw][split] = false;
      lastSwitchPress[sw] = 0;
    }
  }

  // the state of the switch target assigment always reflects the state of the last switch that interacted with it
  if (assignment == ASSIGNED_CC_65) {
    switchCCEnabled[split][Global.ccForSwitchCC65[whichSwitch]] = enabled;
  }
  else if (assignment == ASSIGNED_SUSTAIN) {
    switchCCEnabled[split][Global.ccForSwitchSustain[whichSwitch]] = enabled;
  }
  else {
    switchTargetEnabled[split][assignment] = enabled;
    if (assignment == ASSIGNED_ALTSPLIT || assignment == ASSIGNED_STANDALONE_MIDI_CLOCK) {
      switchTargetEnabled[otherSplit(split)][assignment] = enabled;
    }
  }

  // set the state of the switch
  switchState[whichSwitch][split] = enabled;
  if (assignment == ASSIGNED_STANDALONE_MIDI_CLOCK) {
    switchState[whichSwitch][otherSplit(split)] = enabled;
  }
}

void switchTransposeOctave(byte split, int interval) {
  Split[split].transposeOctave = constrain(Split[split].transposeOctave + interval, -60, 60);
  displayModeStart = millis();
  blinkMiddleRootNote = true;
  updateDisplay();
}

void performSwitchAssignmentOn(byte whichSwitch, byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_AUTO_OCTAVE:
      // reset the note number that is used for the interval
      latestNoteNumberForAutoOctave = -1;
      break;

    case ASSIGNED_OCTAVE_DOWN:
      switchTransposeOctave(split, -12);
      break;

    case ASSIGNED_OCTAVE_UP:
      switchTransposeOctave(split, 12);
      break;

    case ASSIGNED_SUSTAIN:
      preSendSwitchSustain(whichSwitch, split, 127);
      break;

    case ASSIGNED_CC_65:
      preSendSwitchCC65(whichSwitch, split, 127);
      break;

    case ASSIGNED_ALTSPLIT:
      performAltSplitAssignment();
      break;

    case ASSIGNED_ARPEGGIATOR:
      performArpeggiatorToggle();
      break;

    case ASSIGNED_TAP_TEMPO:
      if (!isSyncedToMidiClock()) {
        tapTempoPress();
        if (displayMode == displayGlobalWithTempo) {
          updateDisplay();
        }
      }
      break;

    case ASSIGNED_PRESET_UP:
      performPresetDelta(1);
      break;

    case ASSIGNED_PRESET_DOWN:
      performPresetDelta(-1);
      break;

    case ASSIGNED_REVERSE_PITCH_X:
      performReverseSendXToggle();
      break;

    case ASSIGNED_SEQUENCER_PLAY:
      sequencerTogglePlay(split);
      break;

    case ASSIGNED_SEQUENCER_PREV:
      sequencerPreviousPattern(split);
      break;

    case ASSIGNED_SEQUENCER_NEXT:
      sequencerNextPattern(split);
      break;

    case ASSIGNED_STANDALONE_MIDI_CLOCK:
      standaloneMidiClockStart();
      break;

    case ASSIGNED_SEQUENCER_MUTE:
      sequencerToggleMute(split);
      break;
  }
}

void performPresetDelta(int delta) {
  midiPreset[Global.currentPerSplit] = min(max(midiPreset[Global.currentPerSplit] + delta, 0), 127);
  applyMidiPreset();
  if (displayMode == displayPreset) {
    updateDisplay();
  }
}

void performArpeggiatorToggle() {
  Split[Global.currentPerSplit].arpeggiator = !Split[Global.currentPerSplit].arpeggiator;
  if (Split[Global.currentPerSplit].arpeggiator) {
    temporarilyEnableArpeggiator();
  }
  else {
    disableTemporaryArpeggiator();
  }
  if (displayMode == displayPerSplit) {
    updateDisplay();
  }
}

void performReverseSendXToggle() {
  Split[Global.currentPerSplit].sendX = !Split[Global.currentPerSplit].sendX;
  if (displayMode == displayPerSplit) {
    updateDisplay();
  }
}

void performAltSplitAssignment() {
  resetAllTouches();
  Global.currentPerSplit = otherSplit(Global.currentPerSplit);
  updateDisplay();
}

void performSwitchAssignmentHoldOff(byte whichSwitch, byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_OCTAVE_DOWN:
      switchTransposeOctave(split, 12);
      break;

    case ASSIGNED_OCTAVE_UP:
      switchTransposeOctave(split, -12);
      break;

    case ASSIGNED_SUSTAIN:
    case ASSIGNED_CC_65:
    case ASSIGNED_ARPEGGIATOR:
    case ASSIGNED_LEGATO:
    case ASSIGNED_LATCH:
    case ASSIGNED_REVERSE_PITCH_X:
    case ASSIGNED_STANDALONE_MIDI_CLOCK:
      performSwitchAssignmentOff(whichSwitch, assignment, split);
      break;

    case ASSIGNED_ALTSPLIT:
      performAltSplitAssignment();
      break;
  }
}

void performSwitchAssignmentOff(byte whichSwitch, byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_SUSTAIN:
      preSendSwitchSustain(whichSwitch, split, 0);
      break;

    case ASSIGNED_CC_65:
      preSendSwitchCC65(whichSwitch, split, 0);
      break;

    case ASSIGNED_ARPEGGIATOR:
      performArpeggiatorToggle();
      break;

    case ASSIGNED_LEGATO:
    case ASSIGNED_LATCH:
      noteTouchMapping[split].releaseLatched();
      break;

    case ASSIGNED_REVERSE_PITCH_X:
      performReverseSendXToggle();
      break;

    case ASSIGNED_STANDALONE_MIDI_CLOCK:
      standaloneMidiClockStop();
      break;
  }
}

void handleFootSwitchState(byte whichSwitch, boolean state) {
  if (footSwitchState[whichSwitch] != state) {                       // if state if changed since last read...
    DEBUGPRINT((2,"handleFootSwitchState"));
    DEBUGPRINT((2," pedal="));DEBUGPRINT((2,(int)whichSwitch));
    DEBUGPRINT((2," state="));DEBUGPRINT((2,(int)state));
    DEBUGPRINT((2,"\n"));

    footSwitchState[whichSwitch] = state;

    // merely light leds in test mode
    if (operatingMode == modeManufacturingTest) {
      switchState[whichSwitch][Global.currentPerSplit] = state;
      if (state) {
        setLed(NUMCOLS - 1, 5 + whichSwitch, COLOR_GREEN, cellOn);
      }
      else {
        clearLed(NUMCOLS - 1, 5 + whichSwitch);
      }
    }
    // handle the foot switch assignments in regular mode
    else {
      // if the switch is pressed...
      if (state) {
        doSwitchPressed(whichSwitch);
      }
      // if the switch is released...
      else {
        doSwitchReleased(whichSwitch);
      }
    }
    
    updateSwitchLeds();
  }
}

void resetSwitchStates(byte whichSwitch) {
  for (byte sp = 0; sp < NUMSPLITS; ++ sp) {
    if (switchState[whichSwitch][sp]) {
      byte assignment = Global.switchAssignment[whichSwitch];
      changeSwitchState(whichSwitch, assignment, sp, false);
    }
  }
}

// checkFootSwitches:
// Once every 20 ms, this is called to read foot switch inputs and take action if state changed
void checkFootSwitches() {
  bool state_left = digitalRead(FOOT_SW_LEFT);
  bool state_right = digitalRead(FOOT_SW_RIGHT);

  // If a normally-open switch (or no switch) is connected, invert state of switch read
  if (footSwitchOffState[SWITCH_FOOT_L] == HIGH)  state_left = !state_left;
  if (footSwitchOffState[SWITCH_FOOT_R] == HIGH)  state_right = !state_right;

  handleFootSwitchState(SWITCH_FOOT_B, state_left &
                                       state_right);  // check the combined input state
  handleFootSwitchState(SWITCH_FOOT_L, state_left);   // check left input state
  handleFootSwitchState(SWITCH_FOOT_R, state_right);  // check right input state
}

inline boolean isSwitchAutoOctavePressed(byte split) {
  return switchTargetEnabled[split][ASSIGNED_AUTO_OCTAVE];
}

inline boolean isSwitchLegatoPressed(byte split) {
  return switchTargetEnabled[split][ASSIGNED_LEGATO];
}

inline boolean isSwitchLatchPressed(byte split) {
  return switchTargetEnabled[split][ASSIGNED_LATCH];
}

inline boolean isSwitchCC65CCEnabled(byte whichSwitch, byte split) {
  return switchCCEnabled[split][Global.ccForSwitchCC65[whichSwitch]];
}

inline boolean isSwitchSustainCCEnabled(byte whichSwitch, byte split) {
  return switchCCEnabled[split][Global.ccForSwitchSustain[whichSwitch]];
}