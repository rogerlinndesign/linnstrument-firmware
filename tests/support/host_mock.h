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
// Control over the fake hardware the sketch runs against on the desktop.
#ifndef LINN_TESTS_HOST_MOCK_H
#define LINN_TESTS_HOST_MOCK_H

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

namespace host {

// Clock back to zero, pins low, serial buffers empty. The flash fake belongs to the sketch's own
// dueFlashStorage instance; call dueFlashStorage.erase() from a test that needs it blank.
void reset();

void setMicros(unsigned long us);
void advanceMicros(unsigned long us);
// The clock moves this much on every millis() or micros() call (default 1 us), so the sketch's
// busy-wait delays terminate; raise it to make long waits cheap.
void setMicrosPerRead(unsigned long us);

int pinState(uint8_t pin);

// Everything the sketch wrote to Serial since the last clear.
std::vector<uint8_t> serialOutput();
std::string serialOutputText();
void clearSerialOutput();

// Bytes the sketch will see on Serial.read().
void feedSerial(const uint8_t* data, size_t length);
void feedSerial(const char* text);

// Raw sensor readings (12 bit) the sketch gets back when it scans a cell. Z is the pressure
// before the sketch's own inversion, so 0 is untouched and larger is firmer.
void setSensor(uint8_t col, uint8_t row, uint16_t x, uint16_t y, uint16_t z);

}

#endif
