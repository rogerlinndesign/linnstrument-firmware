/***************************** ls_clock: LinnStrument Musical Clock *******************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions keep track of a common musical system clock in 24PPQ.
When incoming MIDI clock is running it will be used, otherwise an internal clock based on the
active tempo will be calculated.
**************************************************************************************************/

const unsigned long internalClockUnitBase = 2500000;  // 1000000 ( microsecond) * 60 ( minutes - bpm) / 24 ( frames per beat)

unsigned long prevClockTimerCount;                    // the last time the microsecond timer was updated for the musical clock

unsigned long lastInternalClockMoment;                // the last time the internal clock stepped
unsigned char lastInternalClockCount;                 // the count of the internal clock steps, from 0 to 23

signed char previousMidiClockCount;                   // the previous MIDI clock count, used to detect if a change really occurred
signed char previousInternalClockCount;               // the previous internal clock count, used to detect if a change really occurred

void initializeClock() {
  prevClockTimerCount = micros();

  clock24PPQ = 0;
  previousMidiClockCount = 0;
  lastInternalClockCount = 0;

  resetClockAdvancement(0);
}

void resetClockAdvancement(unsigned long now) {
  lastInternalClockMoment = now;
  lastInternalClockCount = 0;
  previousMidiClockCount = -1;
  previousInternalClockCount = -1;
}

inline boolean checkUpdateClock(unsigned long now) {
  if (calcTimeDelta(now, prevClockTimerCount) <= 500) {
    return false;
  }
  prevClockTimerCount = now;

  short clockCount;

  if (isMidiClockRunning()) {
    clockCount = getMidiClockCount();
    if (previousMidiClockCount == clockCount) {
      return false;
    }

    previousMidiClockCount = clockCount;
  }
  else {
    // calculate the time since the last arpeggiator step
    unsigned long internalClockDelta = calcTimeDelta(now, lastInternalClockMoment);

    // check if the time since the last arpeggiator step
    unsigned long clockUnit = FXD4_TO_INT(FXD4_DIV(FXD4_FROM_INT(internalClockUnitBase), fxd4CurrentTempo));

    // check if the time since the last arpeggiator step now has exceeded the delay for the next step, but only if within 10ms of the intended step duration
    if (internalClockDelta >= clockUnit && (internalClockDelta % clockUnit) < 10000) {
      lastInternalClockCount = (lastInternalClockCount + 1) % 24;
      lastInternalClockMoment += ((now - lastInternalClockMoment) / clockUnit) * clockUnit;

      // flash the tempo led in the global display when it is on
      updateGlobalSettingsFlashTempo(now);

      if (previousInternalClockCount == lastInternalClockCount) {
        return false;
      }

      clockCount = lastInternalClockCount;
      previousInternalClockCount = clockCount;
    }
    else {
      return false;
    }
  }

  clock24PPQ = clockCount;

  return true;
}

void tapTempoPress() {
  unsigned long now = micros();
  resetClockAdvancement(now);

  unsigned long tapDelta = calcTimeDelta(now, lastTapTempo);

  if (tapDelta < 6000000) { // minimum 6 seconds between taps
      fxd4CurrentTempo -= FXD4_DIV(fxd4CurrentTempo, FXD4_FROM_INT(4));
      fxd4CurrentTempo += FXD4_DIV(FXD4_FROM_INT(60000000 / tapDelta), FXD4_FROM_INT(4));
  }

  lastTapTempo = now;
}