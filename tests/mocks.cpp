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
// A queue that is still not empty after thousands of passes is a firmware that stopped sending,
// which fails the test instead of hanging it.
void drainMidi() {
  for (int i = 0; i < 8 || (!midiOutQueue.empty() && i < 5000); ++i) {
    host::advanceMicros(20000);
    handlePendingMidi(micros());
  }
  CHECK(midiOutQueue.empty());
}

// Feeds incoming MIDI bytes through the sketch's MIDI parser, as if they arrived over USB.
void midiIn(std::initializer_list<uint8_t> bytes) {
  std::vector<uint8_t> data(bytes);
  host::feedSerial(data.data(), data.size());
  for (int i = 0; i < 1024 && Serial.available() > 0; ++i) {
    handleMidiInput(micros());
  }
}

void midiInNrpn(int parameter, int value, byte channel) {
  byte status = 0xB0 | (channel & 0x0F);
  midiIn({status, 99, (byte)(parameter >> 7), status, 98, (byte)(parameter & 0x7F),
          status, 6, (byte)(value >> 7), status, 38, (byte)(value & 0x7F)});
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

// A finger landing on a cell: pressure ramps up over the velocity window, then holds.
void press(byte col, byte row, uint16_t x, uint16_t y, uint16_t z) {
  int ramp = VELOCITY_TOTAL_SAMPLES;
  for (int i = 1; i <= ramp; ++i) {
    host::setSensor(col, row, x, y, (uint16_t)(z * i / ramp));
    scan(col, row);
  }
  host::setSensor(col, row, x, y, z);
  for (int i = 0; i < 4; ++i) {
    scan(col, row);
  }
}

// A held finger moving to new readings.
void hold(byte col, byte row, uint16_t x, uint16_t y, uint16_t z, int scans = 3) {
  host::setSensor(col, row, x, y, z);
  for (int i = 0; i < scans; ++i) {
    scan(col, row);
  }
}

// The finger lifting off, past the release debounce.
void release(byte col, byte row) {
  host::setSensor(col, row, 0, 0, 0);
  for (int i = 0; i < 4; ++i) {
    scan(col, row);
    host::advanceMicros(30000);
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
  sensorLimitsLifted = false;
  NUMROWS = 8;
  initializeSensors();
  initializeCalibration();
  initializeLeds();
  // the scan starts at cell 0 like the sketch's, never at a cell beyond this model's columns;
  // leaving user firmware mode marks that cell as touched, so this comes before the sketch's
  // reset() clears the touch bookkeeping
  select(0, 0);
  if (userFirmwareActive) {
    changeUserFirmwareMode(false);
  }
  controlModeActive = false;
  initializeCalibrationData();
  initializeSequencer();
  ::reset();
  initializeCalibrationSamples();
  applyConfiguration();
  // state the sketch keeps outside its settings, so its reset() leaves it alone
  controlButton = -1;
  memset(lastControlPress, 0, sizeof(lastControlPress));
  splitButtonDown = false;
  changedSplitPoint = false;
  operatingMode = modePerformance;
  setDisplayMode(displayNormal);
  updateDisplay();
  waitingForCommands = false;
  codePos = 0;
  stopMidiTextScroll();
  midiTextAppend(0);
  midiTextFont = 2;
  midiCellColCC = 0;
  midiCellRowCC = 0;
  // a unit always has a project with a tempo; the clock divides by it on the continuous tasks
  Project.tempo = 120;
  fxd4CurrentTempo = FXD4_FROM_INT(120);
  settle();
  drainMidi();
  host::clearSerialOutput();
}

// Points the sketch's current-cell context at a cell, as the scan loop does before calling
// the touch handlers.
void select(byte col, byte row) {
  sensorCol = col;
  sensorRow = row;
  sensorSplit = getSplitOf(col);
  sensorCell = &cell(col, row);
}

// Runs the serial protocol state machine over everything fed to Serial.
void pumpSerial() {
  while (Serial.available() > 0) {
    handleSerialIO();
  }
}

}
