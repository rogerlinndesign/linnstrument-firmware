/****************************** ls_rtos: LinnStrument Real Time OS ********************************
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
These functions comprise a simple Real Time OS for LinnStrument.
It consists of a delay function, delayUsec, that updates LinnStrument's LEDs and scans its foot
switches at specific time interals, all in the background. This should be used instead of
Arduino's delayMicroseconds() function.
**************************************************************************************************/


// delayUsec:
// use to insert a brief time delay.
// IMPORTANT: Use instead of Arduino's delayMicroseconds() function because this one handles background LED refresh and foot switch checking while it's waiting
inline void delayUsec(unsigned long delayTime) {    // input the delay time in microseconds
  unsigned long start = micros();                   // start is set to time that function is called
  unsigned long now = start;                        // now is set once at function invocation...
  while (calcTimeDelta(now, start) < delayTime) {   // do the following while the interval between now and start less than delayTime
    performContinuousTasks(now);
    now = micros();                                 // reset now to current time and repeat...
  }
}

// delayUsecWithScanning:
// use to insert a brief time delay but with key scanning still active.
inline void delayUsecWithScanning(unsigned long delayTime) {
  // we can not have scanning unless the full setup routine is done,
  // falling back to regular delay in this case
  if (!setupDone) {
    delayUsec(delayTime);
    return;
  }

  unsigned long start = micros();                        // start is set to time that function is called
  while (calcTimeDelta(micros(), start) < delayTime) {   // use now-start to account for clock reset
    modeLoopPerformance();                               // reset now to current time and repeat...
  }
}

inline void performCheckAdvanceArpeggiator() {
  static boolean continuousAdvanceArpeggiator = false;
  if (!continuousAdvanceArpeggiator) {
    continuousAdvanceArpeggiator = true;
    checkAdvanceArpeggiator();
    continuousAdvanceArpeggiator = false;
  }
}

inline void performCheckAdvanceSequencer() {
  static boolean continuousAdvanceSequencer = false;
  if (!continuousAdvanceSequencer) {
    continuousAdvanceSequencer = true;
    checkAdvanceSequencer();
    continuousAdvanceSequencer = false;
  }
}

inline void performContinuousTasks() {
  performContinuousTasks(micros());
}

inline void performContinuousTasks(unsigned long nowMicros) {
  if (!setupDone || displayMode == displaySleep) {
    return;
  }

  static boolean continuousSerialIO = false;

  boolean ledsRefreshed = false;
  static boolean continuousRefreshLeds = false;
  if (!continuousRefreshLeds && !continuousSerialIO) {
    continuousRefreshLeds = true;
    ledsRefreshed = checkRefreshLedColumn(nowMicros);
    continuousRefreshLeds = false;
  }
  
  if (ledsRefreshed) {
    unsigned long nowMillis = millis();

    static boolean continuousStopBlinkingLeds = false;
    if (!continuousStopBlinkingLeds) {
      continuousStopBlinkingLeds = true;
      checkStopBlinkingLeds(nowMillis);
      continuousStopBlinkingLeds = false;
    }
    
    static boolean continuousAdvanceTouchAnimations = false;
    if (!continuousAdvanceTouchAnimations) {
      continuousAdvanceTouchAnimations = true;
      checkTimeToRefreshTouchAnim(nowMillis);
      continuousAdvanceTouchAnimations = false;
    }

    static boolean continuousLegendDisplayTimeout = false;
    if (!continuousLegendDisplayTimeout) {
      continuousLegendDisplayTimeout = true;
      checkLegendDisplayTimeout(nowMillis);
      continuousLegendDisplayTimeout = false;
    }

    static boolean continuousFootSwitches = false;
    if (!continuousFootSwitches) {
      continuousFootSwitches = true;
      checkTimeToReadFootSwitches(nowMicros);
      continuousFootSwitches = false;
    }

    static boolean continuousRefreshGlobalSettingsDisplay = false;
    if (!continuousRefreshGlobalSettingsDisplay) {
      continuousRefreshGlobalSettingsDisplay = true;
      checkRefreshGlobalSettingsDisplay(nowMicros);
      continuousRefreshGlobalSettingsDisplay = false;
    }

    static boolean continuousSleep = false;
    if (!continuousSleep) {
      continuousSleep = true;
      checkSleep(nowMillis);
      continuousSleep = false;
    }
  }

  static boolean continuousUpdateClock = false;
  boolean clockUpdated = false;
  if (!continuousUpdateClock) {
    continuousUpdateClock = true;
    clockUpdated = checkUpdateClock(nowMicros);
    continuousUpdateClock = false;
  }

  if (clockUpdated) {
    performCheckAdvanceArpeggiator();
    performCheckAdvanceSequencer();
  }

  if (Device.serialMode) {
    if (!continuousSerialIO) {
      continuousSerialIO = true;
      handleSerialIO();
      continuousSerialIO = false;
    }
  }
  else {
    static boolean continuousMidiInput = false;
    if (!continuousMidiInput) {
      continuousMidiInput = true;
      handleMidiInput(nowMicros);
      continuousMidiInput = false;
    }

    static boolean continuousPendingMidi = false;
    if (!continuousPendingMidi) {
      continuousPendingMidi = true;
      handlePendingMidi(nowMicros);
      continuousPendingMidi = false;
    }
  }
}

// checks to see if it's time to refresh the next LED column, and if so, does it
// the return value indicate whether the LEDs were updated, so that we can use it
// as a coarse trigger to piggy-back other continuous tasks off of
inline boolean checkRefreshLedColumn(unsigned long now) {
  if (calcTimeDelta(now, prevLedTimerCount) > ledRefreshInterval) {        // is it time to refresh the next LED column?
    refreshLedColumn(now);                                                 // yes, refresh the next LED column...
    prevLedTimerCount = now;                                               // and reset the LED timer count to current time
    return true;
  }
  return false;
}

inline void checkTimeToRefreshTouchAnim(unsigned long now) {
  if (calcTimeDelta(now, prevTouchAnimTimerCount) > 33) {
    performAdvanceTouchAnimations(now);
    prevTouchAnimTimerCount = now;
  }
}

// checks to see if it's time to refresh the next LED column, and if so, does it
inline void checkTimeToReadFootSwitches(unsigned long now) {
  if (calcTimeDelta(now, prevFootSwitchTimerCount) > 20000) {              // is it time to check the foot switches?
    checkFootSwitches();                                                   // yes, check the foot switches and if state has changed, handle the event, then...
    prevFootSwitchTimerCount = now;                                        // reset the foot switch timer to current time
  }
}

// checks to see if it's time to refresh the global settings display, and if so, does it
inline void checkRefreshGlobalSettingsDisplay(unsigned long now) {
  if ((displayMode == displayGlobal || displayMode == displayGlobalWithTempo) &&
      calcTimeDelta(now, prevGlobalSettingsDisplayTimerCount) > 30000) {                                      // is it time to refresh the global settings display
    paintGlobalSettingsFlashTempo(now);                                                                       // yes, refresh the display...
    prevGlobalSettingsDisplayTimerCount = now;                                                                // and reset the timer count to current time
  }
}

void playSleepAnimation() {
  switch (Device.sleepAnimationType) {
    case animationNone:
      activateSleepMode();
      break;
    case animationStore:
      playPromoAnimation();
      break;
    case animationChristmas:
      playChristmasAnimation();
      break;
  }
}

// checks to see if it's time to sleep LinnStrument
inline void checkSleep(unsigned long now) {
  if (Device.sleepActive && Device.sleepDelay > 0 && displayMode != displayAnimation && displayMode != displaySleep &&
      calcTimeDelta(now, lastTouchMoment) > Device.sleepDelay * 60000) {
    playSleepAnimation();
  }
}

// checks whether it's time to stop blinking various LEDs
inline void checkStopBlinkingLeds(unsigned long now) {
  // should the blinking middle root note be stopped blinking
  if ((displayMode == displayNormal || displayMode == displaySplitPoint) && 
      blinkMiddleRootNote &&
      calcTimeDelta(now, displayModeStart) > (Device.operatingLowPower ? 1200 : 600)) {
    blinkMiddleRootNote = false;
    updateDisplay();
  }

  // check if there are blinking preset LEDs that need to be reset
  if (displayMode == displayPreset) {
    for (byte p = 0; p < NUMPRESETS; ++p) {
      if (presetBlinkStart[p] != 0 && calcTimeDelta(now, presetBlinkStart[p]) > 1200) {
        int color = globalColor;
        if (p == Device.lastLoadedPreset) {
          color = COLOR_CYAN;
        }
        int row = p+2;
        if (row >= 6) row -= 6;
        setLed(getPresetDisplayColumn(), row, color, cellOn);
        presetBlinkStart[p] = 0;
      }
    }
  }

  // check if there are blinking project LEDs that need to be reset
  if (displayMode == displaySequencerProjects) {
    for (byte p = 0; p < MAX_PROJECTS; ++p) {
      if (projectBlinkStart[p] != 0 && calcTimeDelta(now, projectBlinkStart[p]) > 1200) {
        int color = globalColor;
        if (p == Device.lastLoadedProject) {
          color = COLOR_CYAN;
        }
        setLed(6 + p%4, 2 + p/4, color, cellOn);
        projectBlinkStart[p] = 0;
      }
    }
  }
}
