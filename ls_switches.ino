/********************************** ls_switches: LinnStrument Switches ****************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
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
  footSwitchOffState[SWITCH_FOOT_L] = digitalRead(FOOT_SW_LEFT);              // check left input
  footSwitchOffState[SWITCH_FOOT_R] = digitalRead(FOOT_SW_RIGHT);             // check right input
  footSwitchState[SWITCH_FOOT_L] = footSwitchOffState[SWITCH_FOOT_L];
  footSwitchState[SWITCH_FOOT_R] = footSwitchOffState[SWITCH_FOOT_R];

  lastSwitchPress[SWITCH_FOOT_L] = 0;
  lastSwitchPress[SWITCH_FOOT_R] = 0;
  lastSwitchPress[SWITCH_SWITCH_2] = 0;
  lastSwitchPress[SWITCH_SWITCH_1] = 0;

  switchState[SWITCH_FOOT_L][LEFT] = (footSwitchState[SWITCH_FOOT_L] != footSwitchOffState[SWITCH_FOOT_L]);
  switchState[SWITCH_FOOT_R][LEFT] = (footSwitchState[SWITCH_FOOT_R] != footSwitchOffState[SWITCH_FOOT_R]);
  switchState[SWITCH_SWITCH_2][LEFT] = false;
  switchState[SWITCH_SWITCH_1][LEFT] = false;
  switchState[SWITCH_FOOT_L][RIGHT] = (footSwitchState[SWITCH_FOOT_L] != footSwitchOffState[SWITCH_FOOT_L]);
  switchState[SWITCH_FOOT_R][RIGHT] = (footSwitchState[SWITCH_FOOT_R] != footSwitchOffState[SWITCH_FOOT_R]);
  switchState[SWITCH_SWITCH_2][RIGHT] = false;
  switchState[SWITCH_SWITCH_1][RIGHT] = false;

  for (byte i = 0; i < 7; ++i) {
    switchTargetEnabled[i][LEFT] = false;
    switchTargetEnabled[i][RIGHT] = false;
  }
}

boolean isStatefulSwitchAssignment(byte assignment) {
  return assignment == ASSIGNED_AUTO_OCTAVE ||
         assignment == ASSIGNED_SUSTAIN ||
         assignment == ASSIGNED_CC_65 ||
         assignment == ASSIGNED_ARPEGGIATOR;
}

void doSwitchPressed(byte whichSwitch) {
  byte assignment = Global.switchAssignment[whichSwitch];
  if (!splitActive || assignment == ASSIGNED_ALTSPLIT || !Global.switchBothSplits[whichSwitch]) {
    doSwitchPressedForSplit(whichSwitch, assignment, Global.currentPerSplit);
  }
  else {
    doSwitchPressedForSplit(whichSwitch, assignment, LEFT);
    doSwitchPressedForSplit(whichSwitch, assignment, RIGHT);
  }
}

void doSwitchPressedForSplit(byte whichSwitch, byte assignment, byte split) {
  // by default, the state of this switch will be on when pressed,
  // unless it's determined later to be a toggle action
  boolean resultingState = true;

  // perform the switch assignment on action if this hasn't been enabled
  // by another switch yet
  if (!switchTargetEnabled[assignment][split]) {
    performSwitchAssignmentOn(assignment, split);
  }
  // if the switch is enabled for the split,
  // go through the assignment off logic
  else {
    // turn the switch state off
    resultingState = false;
    performSwitchAssignmentOff(assignment, split);
  }

  // change the state of the switch and the assignment based on the previous logic
  changeSwitchState(whichSwitch, assignment, split, resultingState);

  // keep track of when the last press happened to be able to differentiate
  // between toggle and hold
  lastSwitchPress[whichSwitch] = millis();
}

void doSwitchReleased(byte whichSwitch) {
  byte assignment = Global.switchAssignment[whichSwitch];
  if (!splitActive || assignment == ASSIGNED_ALTSPLIT || !Global.switchBothSplits[whichSwitch]) {
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
  if (whichSwitch == SWITCH_FOOT_L || whichSwitch == SWITCH_FOOT_R) {
    if (isStatefulSwitchAssignment(assignment)) {
      isHeld = true;
    }
    else {
      isHeld = false;
    }
  }

  if (isHeld && isStatefulSwitchAssignment(assignment)) {
    // perform the assignment off logic, but when a split is active, it's possible that the
    // switch started being held on the other split, so we need to check which split is actually
    // active before changing the state
    if (splitActive) {
      if (switchTargetEnabled[assignment][LEFT]) {
        performSwitchAssignmentHoldOff(assignment, LEFT);
        changeSwitchState(whichSwitch, assignment, LEFT, false);
      }
      if (switchTargetEnabled[assignment][RIGHT]) {
        performSwitchAssignmentHoldOff(assignment, RIGHT);
        changeSwitchState(whichSwitch, assignment, RIGHT, false);
      }
    }
    else {
      if (switchTargetEnabled[assignment][split]) {
        performSwitchAssignmentHoldOff(assignment, split);
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
  switchTargetEnabled[assignment][split] = enabled;
  if (assignment == ASSIGNED_ALTSPLIT) {
    switchTargetEnabled[assignment][!split] = enabled;
  }

  // set the state of the switch
  switchState[whichSwitch][split] = enabled;
}

void switchTransposeOctave(byte split, int interval) {
  Split[split].transposeOctave = constrain(Split[split].transposeOctave + interval, -60, 60);
  displayModeStart = millis();
  blinkMiddleRootNote = true;
  updateDisplay();
}

void performSwitchAssignmentOn(byte assignment, byte split) {
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
      preSendSustain(split, 127);
      break;

    case ASSIGNED_CC_65:
      preSendSwitchCC65(split, 127);
      break;

    case ASSIGNED_ALTSPLIT:
      performAltSplitAssignment();
      break;

    case ASSIGNED_ARPEGGIATOR:
      performArpeggiatorToggle();
      break;
  }
}

void performArpeggiatorToggle() {
  Split[sensorSplit].arpeggiator = !Split[sensorSplit].arpeggiator;
  if (Split[sensorSplit].arpeggiator) {
    temporarilyEnableArpeggiator();
  }
  else {
    disableTemporaryArpeggiator();
  }
  if (displayMode == displayPerSplit) {
    updateDisplay();
  }
}

void performAltSplitAssignment() {
  resetAllTouches();
  Global.currentPerSplit = otherSplit(Global.currentPerSplit);
  updateDisplay();
}

void performSwitchAssignmentHoldOff(byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_OCTAVE_DOWN:
      switchTransposeOctave(split, 12);
      break;

    case ASSIGNED_OCTAVE_UP:
      switchTransposeOctave(split, -12);
      break;

    case ASSIGNED_AUTO_OCTAVE:
    case ASSIGNED_SUSTAIN:
    case ASSIGNED_CC_65:
    case ASSIGNED_ARPEGGIATOR:
      performSwitchAssignmentOff(assignment, split);
      break;

    case ASSIGNED_ALTSPLIT:
      performAltSplitAssignment();
      break;
  }
}

void performSwitchAssignmentOff(byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_AUTO_OCTAVE:
      break;

    case ASSIGNED_SUSTAIN:
      preSendSustain(split, 0);
      break;

    case ASSIGNED_CC_65:
      preSendSwitchCC65(split, 0);
      break;

    case ASSIGNED_ARPEGGIATOR:
      performArpeggiatorToggle();
      break;
  }
}

void handleFootSwitchState(byte whichSwitch, boolean state) {
  if (footSwitchOffState[whichSwitch] == HIGH) state = !state;       // If a normally-open switch (or no switch) is connected, invert state of switch read

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
        setLed(24 + whichSwitch, 6, COLOR_GREEN, cellOn);
      }
      else {
        clearLed(24 + whichSwitch, 6);
      }
    }
    // handle the foot switch assignments in regular mode
    else {
      if (state) {                                                   // if the switch is pressed...
        doSwitchPressed(whichSwitch);
      }
      else {                                                         // if the switch is released...
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
  handleFootSwitchState(SWITCH_FOOT_L, digitalRead(FOOT_SW_LEFT));   // check left input raw state
  handleFootSwitchState(SWITCH_FOOT_R, digitalRead(FOOT_SW_RIGHT));  // check raw right input state
}

inline boolean isSwitchSustainPressed(byte split) {
  return switchTargetEnabled[ASSIGNED_SUSTAIN][split];
}

inline boolean isSwitchAutoOctavePressed(byte split) {
  return switchTargetEnabled[ASSIGNED_AUTO_OCTAVE][split];
}

inline boolean isSwitchCC65Pressed(byte split) {
  return switchTargetEnabled[ASSIGNED_CC_65][split];
}
