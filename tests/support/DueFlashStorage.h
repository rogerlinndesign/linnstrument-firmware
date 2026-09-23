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
// Host stand-in for DueFlashStorage: a RAM array that behaves like erased flash (0xFF) until written.
#ifndef LINN_TESTS_DUEFLASHSTORAGE_H
#define LINN_TESTS_DUEFLASHSTORAGE_H

#include "Arduino.h"

#define LINN_FAKE_FLASH_SIZE 262144

class DueFlashStorage {
public:
  DueFlashStorage();
  byte read(uint32_t address);
  byte* readAddress(uint32_t address);
  boolean write(uint32_t address, byte value);
  boolean write(uint32_t address, byte* data, uint32_t length);
  void erase();
  byte storage[LINN_FAKE_FLASH_SIZE];
};
// the sketch declares its own dueFlashStorage instance, as with the real library

#endif
