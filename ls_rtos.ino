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
  unsigned long start = micros();                        // start is set to time that function is called
  while (calcTimeDelta(micros(), start) < delayTime) {   // use now-start to account for clock reset
    modeLoopPerformance();                               // reset now to current time and repeat...
  }
}

inline void performContinuousTasks(unsigned long now) {
  if (checkRefreshLedColumn(now)) {
    checkStopMiddleRootNoteBlink();
    checkTimeToReadFootSwitches(now);
    checkRefreshGlobalSettingsDisplay(now);
  }

  checkAdvanceArpeggiator(now);  
  if (Device.serialMode) {
    handleExtStorage();
  }
  else {
    handleMidiInput(now);
    handlePendingMidi(now);
  }
}

// checks to see if it's time to refresh the next LED column, and if so, does it
// the return value indicate whether the LEDs were updated, so that we can use it
// as a coarse trigger to piggy-back other continuous tasks off of
inline boolean checkRefreshLedColumn(unsigned long now) {
  if (calcTimeDelta(now, prevLedTimerCount) > ledRefreshInterval) {        // is it time to refresh the next LED column?
    refreshLedColumn(now);                                                 // yes-- refresh the next LED column...
    prevLedTimerCount = now;                                               // and reset the LED timer count to current time
    return true;
  }
  return false;
}

// checks to see if it's time to refresh the next LED column, and if so, does it
inline void checkTimeToReadFootSwitches(unsigned long now) {
  if (calcTimeDelta(now, prevFootSwitchTimerCount) > FOOT_SWITCH_REFRESH_INTERVAL) {    // is it time to check the foot switches?
    checkFootSwitches();                                                                // yes-- check the foot switches and if state has changed, handle the event, then...
    prevFootSwitchTimerCount = now;                                                     // reset the foot switch timer to current time
  }
}

// checks to see if it's time to refresh the global settings display, and if so, does it
inline void checkRefreshGlobalSettingsDisplay(unsigned long now) {
  if (calcTimeDelta(now, prevGlobalSettingsDisplayTimerCount) > GLOBAL_SETTINGS_DISPLAY_REFRESH_INTERVAL &&   // is it time to refresh the global settings display
      (displayMode == displayGlobal || displayMode == displayGlobalWithTempo) && !animationActive) {
    paintGlobalSettingsFlashTempo(now);                                                                       // yes-- refresh the display...
    prevGlobalSettingsDisplayTimerCount = now;                                                                // and reset the timer count to current time
  }
}

// checks whether it's time to stop blinking the middle root note
inline void checkStopMiddleRootNoteBlink() {
  if ((displayMode == displayNormal || displayMode == displaySplitPoint) && 
      blinkMiddleRootNote &&
      calcTimeDelta(millis(), displayModeStart) > 600) {
    blinkMiddleRootNote = false;
    updateDisplay();
  }
}
