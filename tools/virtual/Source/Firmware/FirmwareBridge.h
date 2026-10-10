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
// The one door between the app and the firmware. The firmware is compiled into FirmwareBridge.cpp
// as a single translation unit with the Arduino stand-ins, and nothing of that reaches the rest
// of the app: this header uses plain C++ types only.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace firmware {

enum Display { off = 0, on = 1, fastPulse = 2, slowPulse = 3, focusPulse = 4 };

struct Led {
  uint8_t color;
  uint8_t display;
};

typedef void (*SerialSink)(const uint8_t* data, size_t length);

// Runs setup() on the calling thread with the given flash contents, or a fresh flash when the
// image is empty, in which case the instrument boots in MIDI mode with the calibration screen
// skipped. Every MIDI byte the firmware sends goes to the sink, from whichever thread runs loop().
void boot(int model, const std::vector<uint8_t>& flashImage, SerialSink sink);
void loopOnce();
int cellsPerScan();

int model();
bool calibrated();
int columns();
int rows();
const char* version();
bool userFirmwareMode();
bool serialMode();

Led led(int col, int row);
bool pulseOn(int display);

// Raw sensor readings for a position inside a cell, from the firmware's own default calibration.
uint16_t rawX(int col, float fraction);
uint16_t rawY(int row, float fraction);
// Z is 0 for no touch; several hundred is a normal touch, a few thousand is firm.
void setSensor(int col, int row, uint16_t x, uint16_t y, uint16_t z);

void feedMidi(const uint8_t* data, size_t length);
void footSwitch(int which, bool down);

std::vector<uint8_t> flashImage();
size_t flashSize();

}
