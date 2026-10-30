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
// The one door between the app and the firmware. Each copy of the firmware is compiled with the
// Arduino stand-ins into a namespace of its own (FirmwareInstance.inc), so that every copy has
// its own globals, and nothing of that reaches the rest of the app: this header uses plain C++
// types only.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace firmware {

enum Display { off = 0, on = 1, fastPulse = 2, slowPulse = 3, focusPulse = 4 };

// the screens whose cells have a fixed meaning that an app can name, or none: the playing display,
// a value being slid, and anything else; apps may rely on the order, so new ones go at the end
enum Screen {
  noScreen, perSplitScreen, globalScreen, sequencerScreen, sequencerProjectsScreen, sequencerDrums1Screen,
  sequencerDrums2Screen, sequencerColorsScreen, presetScreen, volumeScreen, octaveTransposeScreen, splitPointScreen,
  customLightsScreen
};

struct Led {
  uint8_t color;
  uint8_t display;
};

// every MIDI byte the firmware sends, from whichever thread runs loop()
using SerialSink = std::function<void(const uint8_t* data, size_t length)>;

// One copy of the firmware with all of its state. Its engine's thread runs it (boot, loopOnce,
// setSensor); MIDI comes in from any thread, through the stand-ins' locked serial input; the
// lights and the other answers are read from other threads as they are, which is good enough to
// draw and report but never to change the firmware.
class Instance {
public:
  virtual ~Instance() = default;

  // Runs setup() on the calling thread with the given flash contents, or a fresh flash when the
  // image is empty, in which case the instrument boots in MIDI mode with the calibration screen
  // skipped.
  virtual void boot(int model, const std::vector<uint8_t>& flashImage, SerialSink sink) = 0;
  virtual void loopOnce() = 0;
  virtual int cellsPerScan() = 0;

  virtual int model() = 0;
  virtual bool calibrated() = 0;
  virtual int columns() = 0;
  virtual int rows() = 0;
  virtual bool userFirmwareMode() = 0;
  virtual bool serialMode() = 0;
  // a settings screen or anything else that is not the playing display, which uses every cell
  virtual bool showingSettings() = 0;
  virtual Screen screen() = 0;
  // on the sequencer screen, whether a note is selected, which puts its faders on the surface
  virtual bool sequencerNoteSelected() = 0;
  // the left split's pitch bend range in semitones
  virtual int bendRange() = 0;
  // MIDI the firmware has queued and not yet sent
  virtual bool midiPending() = 0;

  virtual Led led(int col, int row) = 0;
  virtual bool pulseOn(int display) = 0;

  // Raw sensor readings for a position inside a cell, from the firmware's own default calibration.
  virtual uint16_t rawX(int col, float fraction) = 0;
  virtual uint16_t rawY(int row, float fraction) = 0;
  // Z is 0 for no touch; several hundred is a normal touch, a few thousand is firm.
  virtual void setSensor(int col, int row, uint16_t x, uint16_t y, uint16_t z) = 0;

  virtual void feedMidi(const uint8_t* data, size_t length) = 0;
  virtual void footSwitch(int which, bool down) = 0;

  virtual std::vector<uint8_t> flashImage() = 0;
  virtual size_t flashSize() = 0;
  virtual const char* version() = 0;
};

// the copies compiled into the app; acquire() hands out one that nothing uses, or nullptr when
// they're all taken, and release() gives it back
int instanceCount();
Instance* acquire();
void release(Instance* instance);
// each copy registers itself as the program starts
void add(Instance* instance);

// the same for every copy, answered by the first
const char* version();
size_t flashSize();

}
