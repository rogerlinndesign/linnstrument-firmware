/***************************** ls_clock: LinnStrument Musical Clock *******************************
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
These functions keep track of a common musical system clock in 24PPQ.
When incoming MIDI clock is running it will be used, otherwise an internal clock based on the
active tempo will be calculated.
**************************************************************************************************/

const unsigned long INTERNAL_CLOCK_UNIT_BASE = 2500000;  // 1000000 ( microsecond) * 60 ( minutes - bpm) / 24 ( frames per beat)

unsigned long prevClockTimerCount;                       // the last time the microsecond timer was updated for the musical clock

unsigned long lastInternalClockMoment;                   // the last time the internal clock stepped
unsigned char lastInternalClockCount;                    // the count of the internal clock steps, from 0 to 23

signed char previousMidiClockCount;                      // the previous MIDI clock count, used to detect if a change really occurred
signed char previousInternalClockCount;                  // the previous internal clock count, used to detect if a change really occurred

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
  short clockCount;

  if (isSyncedToMidiClock()) {
    clockCount = getMidiClockCount();
    if (previousMidiClockCount == clockCount) {
      return false;
    }

    previousMidiClockCount = clockCount;
  }
  else {
    if (calcTimeDelta(now, prevClockTimerCount) <= 500) {
      return false;
    }
    prevClockTimerCount = now;

    // calculate the time since the last clock tick
    unsigned long internalClockDelta = calcTimeDelta(now, lastInternalClockMoment);

    unsigned long clockUnit = INTERNAL_CLOCK_UNIT_BASE / FXD4_TO_INT(fxd4CurrentTempo);

    // check if the time since the last clock step now has exceeded the delay for the next step, but only if within 10ms of the intended step duration
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

  if (!isSyncedToMidiClock() &&
     (sequencerIsRunning() || isStandaloneMidiClockRunning())) {
    midiSendTimingClock();
  }

  return true;
}

void tapTempoPress() {
  unsigned long now = micros();
  resetClockAdvancement(now);

  unsigned long tapDelta = calcTimeDelta(now, lastTapTempo);

  if (tapDelta < 6000000) { // maximum 6 seconds between taps
      fxd4CurrentTempo -= FXD4_DIV(fxd4CurrentTempo, FXD4_FROM_INT(4));
      fxd4CurrentTempo += FXD4_DIV(FXD4_FROM_INT(60000000 / tapDelta), FXD4_FROM_INT(4));
  }

  lastTapTempo = now;

  if (displayMode == displayGlobal) {
    setDisplayMode(displayGlobalWithTempo);
  }
}