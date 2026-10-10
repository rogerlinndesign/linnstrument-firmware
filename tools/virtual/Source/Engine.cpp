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
#include "Engine.h"

#include "Firmware/FirmwareBridge.h"

#include <chrono>
#include <thread>

Engine* Engine::current = nullptr;

Engine::Engine() : juce::Thread("Firmware") {}

Engine::~Engine() {
  stop();
}

void Engine::start(int wantedModel) {
  stop();
  model = wantedModel;
  current = this;

  output = juce::MidiOutput::createNewDevice(portName());
  input = juce::MidiInput::createNewDevice(portName(), this);
  if (input != nullptr) input->start();

  std::vector<uint8_t> image;
  juce::File file = flashFile(model);
  if (file.existsAsFile() && (size_t) file.getSize() == firmware::flashSize()) {
    juce::MemoryBlock block;
    file.loadFileAsData(block);
    image.assign((const uint8_t*) block.getData(), (const uint8_t*) block.getData() + block.getSize());
  }
  firmware::boot(model, image, serialSink);
  lastSavedFlash = firmware::flashImage();
  startThread(juce::Thread::Priority::high);
}

void Engine::stop() {
  if (!isThreadRunning()) return;
  signalThreadShouldExit();
  waitForThreadToExit(2000);
  saveFlash();
  if (input != nullptr) input->stop();
  input.reset();
  output.reset();
  touches.clear();
}

void Engine::run() {
  int scans = 0;
  while (!threadShouldExit()) {
    for (int i = 0; i < firmware::cellsPerScan(); ++i) {
      firmware::loopOnce();
    }
    advanceTouches();
    std::this_thread::sleep_for(std::chrono::microseconds(1500));
    if (++scans % 2000 == 0) {
      saveFlash();
    }
  }
}

// one step of every touch's pressure per scan, so the firmware measures a velocity from the rise
void Engine::advanceTouches() {
  const juce::ScopedLock lock(touchLock);
  for (auto it = touches.begin(); it != touches.end();) {
    Touch& t = it->second;
    if (t.targetZ == 0) {
      firmware::setSensor(t.col, t.row, 0, 0, 0);
      it = touches.erase(it);
      continue;
    }
    if (t.z < t.targetZ) {
      t.z = juce::jmin(t.targetZ, t.z + juce::jmax(1, t.targetZ / velocityScans));
    }
    else {
      t.z = t.targetZ;
    }
    firmware::setSensor(t.col, t.row, t.x, t.y, (uint16_t) t.z);
    ++it;
  }
}

static int touchKey(int col, int row) { return col * 8 + row; }

void Engine::press(int col, int row, float xFraction, float yFraction) {
  const juce::ScopedLock lock(touchLock);
  Touch t { col, row, firmware::rawX(col, xFraction), firmware::rawY(row, yFraction), pressure, 0, xFraction, yFraction };
  touches[touchKey(col, row)] = t;
}

void Engine::move(int col, int row, float xFraction, float yFraction) {
  const juce::ScopedLock lock(touchLock);
  auto it = touches.find(touchKey(col, row));
  if (it == touches.end()) return;
  it->second.x = firmware::rawX(col, xFraction);
  it->second.y = firmware::rawY(row, yFraction);
  it->second.xFraction = xFraction;
  it->second.yFraction = yFraction;
  it->second.targetZ = pressure;
}

void Engine::release(int col, int row) {
  const juce::ScopedLock lock(touchLock);
  auto it = touches.find(touchKey(col, row));
  if (it != touches.end()) it->second.targetZ = 0;
}

bool Engine::isTouched(int col, int row) const {
  const juce::ScopedLock lock(touchLock);
  auto it = touches.find(touchKey(col, row));
  return it != touches.end() && it->second.targetZ > 0;
}

bool Engine::touchAt(int col, int row, TouchView& view) const {
  const juce::ScopedLock lock(touchLock);
  auto it = touches.find(touchKey(col, row));
  if (it == touches.end() || it->second.targetZ <= 0) return false;
  view.xFraction = it->second.xFraction;
  view.yFraction = it->second.yFraction;
  view.pressure = juce::jlimit(0.0f, 1.0f, (float) it->second.z / 3000.0f);
  return true;
}

void Engine::setFootSwitch(int which, bool down) {
  firmware::footSwitch(which, down);
}

juce::String Engine::status() const {
  if (!isThreadRunning()) return "Stopped";
  juce::String text = "LinnStrument " + juce::String(firmware::model()) + ", firmware " + firmware::version();
  text += hasVirtualPorts() ? ", MIDI port \"" + portName() + "\"" : ", no virtual MIDI port on this platform";
  if (!firmware::calibrated()) text += ", default calibration";
  if (firmware::userFirmwareMode()) text += ", user firmware mode";
  if (firmware::serialMode()) text += ", serial mode (no MIDI until OS Update is tapped again)";
  return text;
}

void Engine::serialSink(const uint8_t* data, size_t length) {
  if (current != nullptr) {
    current->concatenator.pushMidiData(data, (int) length, juce::Time::getMillisecondCounterHiRes() * 0.001, (void*) nullptr, current->parser);
  }
}

void Engine::OutputParser::handleIncomingMidiMessage(void*, const juce::MidiMessage& message) {
  if (engine.output != nullptr) engine.output->sendMessageNow(message);
}

void Engine::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) {
  firmware::feedMidi(message.getRawData(), (size_t) message.getRawDataSize());
}

juce::File Engine::flashFile(int forModel) const {
  juce::File dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
  dir = dir.getChildFile("Application Support");
#endif
  return dir.getChildFile("Virtual LinnStrument").getChildFile("flash-" + juce::String(forModel) + ".bin");
}

void Engine::saveFlash() {
  std::vector<uint8_t> image = firmware::flashImage();
  if (image == lastSavedFlash) return;
  juce::File file = flashFile(model);
  file.getParentDirectory().createDirectory();
  if (file.replaceWithData(image.data(), image.size())) {
    lastSavedFlash = image;
  }
}
