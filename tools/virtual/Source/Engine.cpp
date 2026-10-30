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
#include "BuildInfo.h"

#include <chrono>
#include <stdexcept>
#include <thread>

Engine::Engine() : juce::Thread("Firmware"), instance(firmware::acquire()) {
  if (instance == nullptr) throw std::runtime_error("every copy of the firmware is in use");
}

Engine::~Engine() {
  stop();
  firmware::release(instance);
}

void Engine::start(int wantedModel) {
  stop();
  model = wantedModel;

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
  instance->boot(model, image, [this](const uint8_t* data, size_t length) { serialSink(data, length); });
  columns = instance->columns();
  lastSavedFlash = instance->flashImage();
  // after the boot's image is taken, so that the stop saves these settings
  if (image.empty() && onFreshFlash) onFreshFlash();
  // here, not in run(), which a stop right after this start could otherwise undo
  finishing = false;
  startThread(juce::Thread::Priority::high);
}

void Engine::stop() {
  if (!isThreadRunning()) return;
  // the queue outlives a boot, so what it holds would otherwise go out after the next one; the
  // firmware's thread sends it, being the only one that may read it, then ends
  finishing = true;
  if (!waitForThreadToExit(1000)) {
    signalThreadShouldExit();
    waitForThreadToExit(2000);
  }
  saveFlash();
  if (input != nullptr) input->stop();
  input.reset();
  output.reset();
  const juce::ScopedLock lock(touchLock);
  touches.clear();
  const juce::ScopedLock outGuard(outLock);
  midiOut.clear();
}

// The scan in slices with a pause after each, so that the firmware's continuous tasks, its MIDI
// output among them, run spread over the scan as on the instrument instead of in one burst.
void Engine::run() {
  constexpr int slices = 8;
  scansRun = 0;
  // each scan starts where the last was due to end, so that one that ran late is made up, unless
  // the engine fell far behind, as after the computer slept
  auto start = std::chrono::steady_clock::now();
  int held = 0;
  // stopping, once the queued MIDI is out; stop() gives up on a queue that doesn't drain, as in
  // serial mode, after a second
  while (!threadShouldExit() && !(finishing && !instance->midiPending())) {
    auto now = std::chrono::steady_clock::now();
    if (now - start > std::chrono::milliseconds(20)) start = now;
    int cells = instance->cellsPerScan();
    double length = scanMicros(cells, held);
    for (int slice = 0; slice < slices; ++slice) {
      for (int i = slice * cells / slices; i < (slice + 1) * cells / slices; ++i) {
        instance->loopOnce();
      }
      std::this_thread::sleep_until(start + std::chrono::microseconds((long long) (length * (slice + 1) / slices)));
    }
    start += std::chrono::microseconds((long long) length);
    held = advanceTouches();
    if (++scansRun % 2000 == 0) {
      saveFlash();
    }
  }
}

// one step of every touch's pressure per scan, so the firmware measures a velocity from the rise;
// how many are held, which the next scan takes the time of
int Engine::advanceTouches() {
  const juce::ScopedLock lock(touchLock);
  int held = 0;
  for (auto it = touches.begin(); it != touches.end();) {
    Touch& t = it->second;
    if (t.targetZ == 0) {
      instance->setSensor(t.col, t.row, 0, 0, 0);
      it = touches.erase(it);
      continue;
    }
    // a press that replaced a lifting touch within one scan: the lift goes out first
    if (t.liftFirst) {
      instance->setSensor(t.col, t.row, 0, 0, 0);
      t.liftFirst = false;
      ++it;
      continue;
    }
    if (t.releaseIn > 0 && --t.releaseIn == 0) {
      t.targetZ = 0;
      instance->setSensor(t.col, t.row, 0, 0, 0);
      it = touches.erase(it);
      continue;
    }
    if (t.z < t.targetZ) {
      t.z = juce::jmin(t.targetZ, t.z + juce::jmax(1, t.targetZ / velocityScans));
    }
    else {
      t.z = t.targetZ;
    }
    instance->setSensor(t.col, t.row, t.x, t.y, (uint16_t) t.z);
    held += t.z > 0 ? 1 : 0;
    ++it;
  }
  return held;
}

static int touchKey(int col, int row) { return col * 8 + row; }

// a press on a cell already down keeps its pressure, so the firmware sees no dip
void Engine::press(int col, int row, float xFraction, float yFraction, int rawZ) {
  const juce::ScopedLock lock(touchLock);
  Touch t { col, row, instance->rawX(col, xFraction), instance->rawY(row, yFraction), rawZ > 0 ? rawZ : pressure, 0, xFraction, yFraction, rawZ > 0, false, 0 };
  auto it = touches.find(touchKey(col, row));
  if (it != touches.end()) {
    if (it->second.targetZ > 0) t.z = it->second.z;
    else t.liftFirst = true;
  }
  touches[touchKey(col, row)] = t;
}

bool Engine::move(int col, int row, float xFraction, float yFraction, int rawZ) {
  const juce::ScopedLock lock(touchLock);
  auto it = touches.find(touchKey(col, row));
  if (it == touches.end() || it->second.targetZ == 0) return false;
  it->second.x = instance->rawX(col, xFraction);
  it->second.y = instance->rawY(row, yFraction);
  it->second.xFraction = xFraction;
  it->second.yFraction = yFraction;
  if (rawZ > 0) { it->second.targetZ = rawZ; it->second.ownPressure = true; }
  else if (!it->second.ownPressure) it->second.targetZ = pressure;
  return true;
}

// On the instrument a finger crossing into the next cell straddles both for a moment, and that is
// how the firmware tells a slide from a new touch (potentialSlideTransferCandidate in
// ls_handleTouches.ino): the cell it leaves is still down with the finger on their shared edge,
// and the new one presses harder. So the old touch moves to that edge and eases off, the new one
// starts there with the finger's pressure, and the old one lets go a few scans later.
bool Engine::slide(int fromCol, int row, int toCol, float yFraction) {
  const juce::ScopedLock lock(touchLock);
  if (std::abs(toCol - fromCol) != 1 || fromCol < 1 || toCol < 1) return false;
  auto it = touches.find(touchKey(fromCol, row));
  if (it == touches.end() || it->second.targetZ == 0 || it->second.z == 0 || it->second.releaseIn > 0) return false;
  auto existing = touches.find(touchKey(toCol, row));
  if (existing != touches.end() && existing->second.targetZ > 0) return false;
  Touch& from = it->second;
  bool right = toCol > fromCol;
  Touch to = from;
  to.col = toCol;
  to.xFraction = right ? 0.0f : 1.0f;
  to.yFraction = yFraction;
  to.x = instance->rawX(toCol, to.xFraction);
  to.y = instance->rawY(row, yFraction);
  // as a press does, a cell still lifting is let go of first, so that its note ends
  to.liftFirst = existing != touches.end();
  to.releaseIn = 0;
  from.xFraction = right ? 1.0f : 0.0f;
  from.x = instance->rawX(fromCol, from.xFraction);
  from.targetZ = juce::jmax(1, to.targetZ * 2 / 3);
  from.z = juce::jmin(from.z, from.targetZ);
  from.releaseIn = slideScans;
  touches[touchKey(toCol, row)] = to;
  return true;
}

void Engine::setPressure(int rawZ) {
  const juce::ScopedLock lock(touchLock);
  pressure = rawZ;
  for (auto& [key, t] : touches) {
    if (!t.ownPressure && t.targetZ > 0 && t.releaseIn == 0) t.targetZ = rawZ;
  }
}

bool Engine::release(int col, int row) {
  const juce::ScopedLock lock(touchLock);
  auto it = touches.find(touchKey(col, row));
  if (it == touches.end() || it->second.targetZ == 0) return false;
  it->second.targetZ = 0;
  return true;
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
  instance->footSwitch(which, down);
}

juce::String Engine::status() const {
  if (!isThreadRunning()) return "Stopped";
  juce::String text = "LinnStrument " + juce::String(instance->model()) + ", firmware " + instance->version() + ", commit " + VIRTUAL_COMMIT;
  text += hasVirtualPorts() ? ", MIDI port \"" + portName() + "\"" : ", no virtual MIDI port on this platform";
  if (!instance->calibrated()) text += ", default calibration";
  if (instance->userFirmwareMode()) text += ", user firmware mode";
  if (instance->serialMode()) text += ", serial mode (no MIDI until OS Update is tapped again)";
  return text;
}

void Engine::serialSink(const uint8_t* data, size_t length) {
  concatenator.pushMidiData(data, (int) length, juce::Time::getMillisecondCounterHiRes() * 0.001, (void*) nullptr, parser);
}

void Engine::OutputParser::handleIncomingMidiMessage(void*, const juce::MidiMessage& message) {
  if (engine.output != nullptr) engine.output->sendMessageNow(message);
  if (engine.onMidiOut) engine.onMidiOut(message);
  const juce::ScopedLock lock(engine.outLock);
  if (engine.midiOut.size() >= keptMessages) engine.midiOut.pop_front();
  engine.midiOut.emplace_back(message.getRawData(), message.getRawData() + message.getRawDataSize());
}

void Engine::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) {
  instance->feedMidi(message.getRawData(), (size_t) message.getRawDataSize());
}

void Engine::sendMidi(const uint8_t* data, size_t length) {
  instance->feedMidi(data, length);
}

std::vector<std::vector<uint8_t>> Engine::takeMidiOut() {
  const juce::ScopedLock lock(outLock);
  std::vector<std::vector<uint8_t>> taken(midiOut.begin(), midiOut.end());
  midiOut.clear();
  return taken;
}

juce::File Engine::flashFile(int forModel) const {
  juce::File dir = flashFolder;
  if (dir == juce::File()) {
    dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    dir = dir.getChildFile("Application Support");
#endif
    dir = dir.getChildFile("Virtual LinnStrument");
  }
  return dir.getChildFile("flash-" + juce::String(forModel) + ".bin");
}

void Engine::saveFlash() {
  std::vector<uint8_t> image = instance->flashImage();
  if (image == lastSavedFlash) return;
  juce::File file = flashFile(model);
  file.getParentDirectory().createDirectory();
  if (file.replaceWithData(image.data(), image.size())) {
    lastSavedFlash = image;
  }
}
