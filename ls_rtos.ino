/****************************** ls_rtos: LinnStrument Real Time OS ********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
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

inline void performContinuousTasks(unsigned long nowMicros) {
  if (displayMode == displaySleep) {
    return;
  }

  static boolean continuousRefreshLeds = false;
  static boolean continuousStopBlinkingLeds = false;
  static boolean continuousFootSwitches = false;
  static boolean continuousRefreshGlobalSettingsDisplay = false;
  static boolean continuousSleep = false;
  static boolean continuousUpdateClock = false;
  static boolean continuousAdvanceArpeggiator = false;
  static boolean continuousAdvanceSequencer = false;
  static boolean continuousSerialIO = false;
  static boolean continuousMidiInput = false;
  static boolean continuousPendingMidi = false;

  bool ledsRefreshed = false;
  if (!continuousRefreshLeds && !continuousSerialIO) {
    continuousRefreshLeds = true;
    ledsRefreshed = checkRefreshLedColumn(nowMicros);
    continuousRefreshLeds = false;
  }
  if (ledsRefreshed) {
    unsigned long nowMillis = millis();

    if (!continuousStopBlinkingLeds) {
      continuousStopBlinkingLeds = true;
      checkStopBlinkingLeds(nowMillis);
      continuousStopBlinkingLeds = false;
    }

    if (!continuousFootSwitches) {
      continuousFootSwitches = true;
      checkTimeToReadFootSwitches(nowMicros);
      continuousFootSwitches = false;
    }

    if (!continuousRefreshGlobalSettingsDisplay) {
      continuousRefreshGlobalSettingsDisplay = true;
      checkRefreshGlobalSettingsDisplay(nowMicros);
      continuousRefreshGlobalSettingsDisplay = false;
    }

    if (!continuousSleep) {
      continuousSleep = true;
      checkSleep(nowMillis);
      continuousSleep = false;
    }
  }

  bool clockUpdated = false;
  if (!continuousUpdateClock) {
    continuousUpdateClock = true;
    clockUpdated = checkUpdateClock(nowMicros);
    continuousUpdateClock = false;
  }

  if (clockUpdated) {
    if (!continuousAdvanceArpeggiator) {
      continuousAdvanceArpeggiator = true;
      checkAdvanceArpeggiator();
      continuousAdvanceArpeggiator = false;
    }

    if (!continuousAdvanceSequencer) {
      continuousAdvanceSequencer = true;
      checkAdvanceSequencer();
      continuousAdvanceSequencer = false;
    }
  }

  if (Device.serialMode) {
    if (!continuousSerialIO) {
      continuousSerialIO = true;
      handleSerialIO();
      continuousSerialIO = false;
    }
  }
  else {
    if (!continuousMidiInput) {
      continuousMidiInput = true;
      handleMidiInput(nowMicros);
      continuousMidiInput = false;
    }

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

// checks to see if it's time to sleep LinnStrument
inline void checkSleep(unsigned long now) {
  if (Device.sleepActive && Device.sleepDelay > 0 && displayMode != displayPromo && displayMode != displaySleep &&
      calcTimeDelta(now, lastTouchMoment) > Device.sleepDelay * 60000) {
    if (Device.sleepAnimation) {
      playPromoAnimation();
    }
    else {
      activateSleepMode();
    }
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
        setLed(NUMCOLS-2, p+2, globalColor, cellOn);
        presetBlinkStart[p] = 0;
      }
    }
  }
}
