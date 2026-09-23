/*
Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
// Helpers that know the firmware's internals, included into the test translation unit after the
// sketch so they can reach every global and function directly.

namespace linn {

void reset(byte model = 200);
void select(byte col, byte row);
void settle();
void scan(byte col, byte row);
void drainMidi();

// Marks a cell as touched with a raw pressure, the way a scan would have left it.
void touch(byte col, byte row, unsigned short rawZ) {
  cellTouched(col, row, touchedCell);
  cell(col, row).currentRawZ = rawZ;
  cell(col, row).shouldRefreshZ = false;
}

// Lets the MIDI output queue drain completely, advancing the fake clock past every interval.
void drainMidi() {
  for (int i = 0; i < 8 || !midiOutQueue.empty(); ++i) {
    host::advanceMicros(20000);
    handlePendingMidi(micros());
  }
}

// Points the sketch's current-cell context at a cell, as the scan loop does before calling
// the touch handlers.
void select(byte col, byte row) {
  sensorCol = col;
  sensorRow = row;
  sensorSplit = Global.splitActive && col >= Global.splitPoint ? RIGHT : LEFT;
  sensorCell = &cell(col, row);
}

// Runs the serial protocol state machine over everything fed to Serial.
void pumpSerial() {
  while (Serial.available() > 0) {
    handleSerialIO();
  }
}

// One pass of the scan loop for a cell, the dispatch modeLoopPerformance() does.
void scan(byte col, byte row) {
  select(col, row);
  sensorCell->shouldRefreshData();
  TouchState previous = sensorCell->touched;
  if (previous != touchedCell && previous != ignoredCell && sensorCell->isMeaningfulTouch()) {
    handleNewTouch();
  } else if (previous == touchedCell && sensorCell->isActiveTouch()) {
    handleXYZupdate();
  } else if (previous != untouchedCell && !sensorCell->isActiveTouch() && sensorCell->isPastDebounceDelay()) {
    handleTouchRelease();
  }
  host::advanceMicros(1000);
}

// Lets the scan loop finish with the cell it was on: a mode change marks that cell as ignored
// until it is seen released, exactly as on the instrument where the next scan does this.
void settle() {
  byte col = sensorCol, row = sensorRow;
  for (int i = 0; i < 8; ++i) {
    host::advanceMicros(50000);
    scan(col, row);
  }
}

// A LinnStrument fresh from the factory, in MIDI mode, with no touches and the clock at zero.
// Follows the order of setup() in linnstrument-firmware.ino, minus the flash boot logic.
void reset(byte model) {
  host::reset();
  dueFlashStorage.erase();
  // nothing of the previous test's settings may survive, not even the calibration columns a
  // 128 never initialises
  memset(&config, 0, sizeof(config));
  LINNMODEL = model;
  NUMCOLS = model == 200 ? 26 : 17;
  NUMROWS = 8;
  initializeSensors();
  initializeCalibration();
  initializeLeds();
  initializeCalibrationData();
  initializeSequencer();
  ::reset();
  initializeCalibrationSamples();
  applyConfiguration();
  // modes the sketch keeps outside its settings, so its reset() leaves them alone
  if (userFirmwareActive) {
    changeUserFirmwareMode(false);
  }
  controlModeActive = false;
  controlButton = -1;
  memset(lastControlPress, 0, sizeof(lastControlPress));
  splitButtonDown = false;
  changedSplitPoint = false;
  operatingMode = modePerformance;
  setDisplayMode(displayNormal);
  waitingForCommands = false;
  codePos = 0;
  settle();
  drainMidi();
  host::clearSerialOutput();
}

}
