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
#include "Arduino.h"
#include "DueFlashStorage.h"
#include "host_mock.h"

#include <cstdio>
#include <cstdarg>

static unsigned long fakeMicros = 0;
static unsigned long microsPerRead = 1;
static bool realClock = false;
static std::chrono::steady_clock::time_point clockStart;
static std::mutex serialInLock;
static std::mutex sensorLock;
static host::SerialSink serialSink = NULL;
static int pins[128];
static std::vector<uint8_t> serialOut;
static std::deque<uint8_t> serialIn;

// The sketch addresses a cell and an axis through two bytes to the sensor's shift registers
// (ls_sensor.ino selectSensorCell), then reads the ADC as two bytes. These decode that.
enum { pinSensor = 4, pinAdc = 52 };
static uint16_t sensorX[32][8], sensorY[32][8], sensorZ[32][8];
static uint8_t sensorLsb = 0;
static uint16_t adcWord = 0;
static int adcByte = 0;

HardwareSerial Serial;
SPIClass SPI;

namespace host {

void reset() {
  fakeMicros = 0;
  microsPerRead = 1;
  memset(pins, 0, sizeof(pins));
  serialOut.clear();
  serialIn.clear();
  memset(sensorX, 0, sizeof(sensorX));
  memset(sensorY, 0, sizeof(sensorY));
  memset(sensorZ, 0, sizeof(sensorZ));
  adcWord = 0;
  adcByte = 0;
}

void setSensor(uint8_t col, uint8_t row, uint16_t x, uint16_t y, uint16_t z) {
  std::lock_guard<std::mutex> guard(sensorLock);
  sensorX[col & 31][row & 7] = x;
  sensorY[col & 31][row & 7] = y;
  sensorZ[col & 31][row & 7] = z;
}

void setMicros(unsigned long us) { fakeMicros = us; }
void setMicrosPerRead(unsigned long us) { microsPerRead = us; }
void advanceMicros(unsigned long us) { fakeMicros += us; }
void useRealClock(bool real) {
  realClock = real;
  clockStart = std::chrono::steady_clock::now();
}
int pinState(uint8_t pin) { return pins[pin]; }
void setPin(uint8_t pin, int value) { pins[pin] = value; }
void setSerialSink(SerialSink sink) { serialSink = sink; }

std::vector<uint8_t> serialOutput() { return serialOut; }
std::string serialOutputText() { return std::string(serialOut.begin(), serialOut.end()); }
void clearSerialOutput() { serialOut.clear(); }

void feedSerial(const uint8_t* data, size_t length) {
  std::lock_guard<std::mutex> guard(serialInLock);
  serialIn.insert(serialIn.end(), data, data + length);
}
void feedSerial(const char* text) { feedSerial((const uint8_t*)text, strlen(text)); }

}

static unsigned long realMicros() {
  return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - clockStart).count();
}
unsigned long millis() {
  if (realClock) return realMicros() / 1000;
  fakeMicros += microsPerRead;
  return fakeMicros / 1000;
}
unsigned long micros() {
  if (realClock) return realMicros();
  fakeMicros += microsPerRead;
  return fakeMicros;
}
void delayMicroseconds(unsigned int us) {
  if (realClock) std::this_thread::sleep_for(std::chrono::microseconds(us));
  else fakeMicros += us;
}
void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t pin, uint8_t value) { pins[pin] = value; }
int digitalRead(uint8_t pin) { return pins[pin]; }
int analogRead(uint8_t) { return 0; }
long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }
void randomSeed(unsigned long seed) { srand((unsigned)seed); }

void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::flush() {}
int HardwareSerial::available() {
  std::lock_guard<std::mutex> guard(serialInLock);
  return (int)serialIn.size();
}
int HardwareSerial::availableForWrite() { return 64; }
int HardwareSerial::read() {
  std::lock_guard<std::mutex> guard(serialInLock);
  if (serialIn.empty()) return -1;
  uint8_t b = serialIn.front();
  serialIn.pop_front();
  return b;
}
size_t HardwareSerial::write(uint8_t b) { return write(&b, 1); }
size_t HardwareSerial::write(const uint8_t* buffer, size_t size) {
  if (serialSink) serialSink(buffer, size);
  else serialOut.insert(serialOut.end(), buffer, buffer + size);
  return size;
}
size_t HardwareSerial::write(const char* text) { return write((const uint8_t*)text, strlen(text)); }
size_t HardwareSerial::print(const char* text) { return write(text); }
size_t HardwareSerial::print(char c) { return write((uint8_t)c); }
static size_t printFormatted(const char* format, ...) {
  char buffer[64];
  va_list args;
  va_start(args, format);
  int n = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  return Serial.write((const uint8_t*)buffer, n < 0 ? 0 : (size_t)n);
}
size_t HardwareSerial::print(int v) { return printFormatted("%d", v); }
size_t HardwareSerial::print(unsigned int v) { return printFormatted("%u", v); }
size_t HardwareSerial::print(long v) { return printFormatted("%ld", v); }
size_t HardwareSerial::print(unsigned long v) { return printFormatted("%lu", v); }
size_t HardwareSerial::print(double v) { return printFormatted("%.2f", v); }
size_t HardwareSerial::println() { return write("\r\n"); }
size_t HardwareSerial::println(const char* text) { return print(text) + println(); }
size_t HardwareSerial::println(char c) { return print(c) + println(); }
size_t HardwareSerial::println(int v) { return print(v) + println(); }
size_t HardwareSerial::println(unsigned int v) { return print(v) + println(); }
size_t HardwareSerial::println(long v) { return print(v) + println(); }
size_t HardwareSerial::println(unsigned long v) { return print(v) + println(); }
size_t HardwareSerial::println(double v) { return print(v) + println(); }

void SPIClass::begin(uint8_t) {}
void SPIClass::setDataMode(uint8_t, uint8_t) {}
void SPIClass::setClockDivider(uint8_t, uint8_t) {}
uint8_t SPIClass::transfer(uint8_t pin, uint8_t data, uint8_t mode) {
  if (pin == pinSensor) {
    if (mode == SPI_CONTINUE) {
      sensorLsb = data;
      return 0;
    }
    uint8_t col = data & 0x1F;
    uint8_t row = sensorLsb & 0x07;
    uint16_t value;
    std::lock_guard<std::mutex> guard(sensorLock);
    if ((sensorLsb & 0x50) == 0x50) {
      value = sensorX[col][row];
    } else if ((sensorLsb & 0x18) == 0x18) {
      value = sensorY[col][row];
    } else {
      value = 4095 - (sensorZ[col][row] > 4095 ? 4095 : sensorZ[col][row]);
    }
    adcWord = (uint16_t)((value & 0xFFF) << 2);
    adcByte = 0;
    return 0;
  }
  if (pin == pinAdc) {
    uint8_t out = adcByte == 0 ? (uint8_t)(adcWord >> 8) : (uint8_t)(adcWord & 0xFF);
    adcByte = (adcByte + 1) % 2;
    return out;
  }
  return 0;
}

DueFlashStorage::DueFlashStorage() { erase(); }
void DueFlashStorage::erase() { memset(storage, 0xFF, sizeof(storage)); }
byte DueFlashStorage::read(uint32_t address) { return storage[address % LINN_FAKE_FLASH_SIZE]; }
byte* DueFlashStorage::readAddress(uint32_t address) { return storage + (address % LINN_FAKE_FLASH_SIZE); }
boolean DueFlashStorage::write(uint32_t address, byte value) {
  storage[address % LINN_FAKE_FLASH_SIZE] = value;
  return true;
}
boolean DueFlashStorage::write(uint32_t address, byte* data, uint32_t length) {
  for (uint32_t i = 0; i < length; ++i) storage[(address + i) % LINN_FAKE_FLASH_SIZE] = data[i];
  return true;
}
