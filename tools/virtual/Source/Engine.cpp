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

  if (virtualPorts) {
    output = juce::MidiOutput::createNewDevice(portName());
    input = juce::MidiInput::createNewDevice(portName(), this);
    if (input != nullptr) input->start();
  }

  // the flash is read and kept under the lock currentFlash() takes, which another thread may call
  const juce::ScopedLock flashGuard(flashLock);
  std::vector<uint8_t> image, settings;
  juce::File file = flashFile(model);
  if (!flashInFile) {
    size_t size = bootSettingsSize, offset = bootSettingsOffset;
    if (bootFlash.size() != firmware::flashSize()) {}
    else if ((size == 0 || size == instance->settingsSize()) && (offset == 0 || offset == instance->settingsOffset())) image = bootFlash;
    else if (size > 0 && offset + sizeof(unsigned long) + 2 * size <= bootFlash.size()) {
      // the newer of its two copies, as loadSettings() picks it
      auto from = bootFlash.begin() + (long) (offset + sizeof(unsigned long) + (bootFlash[offset] == 0 ? 0 : size));
      settings.assign(from, from + (long) size);
    }
  }
  else if (file.existsAsFile() && (size_t) file.getSize() == firmware::flashSize()) {
    juce::MemoryBlock block;
    file.loadFileAsData(block);
    image.assign((const uint8_t*) block.getData(), (const uint8_t*) block.getData() + block.getSize());
  }
  instance->boot(model, image, [this](const uint8_t* data, size_t length) { serialSink(data, length); });
  columns = instance->columns();
  booted = true;
  bootedFrom = image;
  lastSavedFlash = instance->flashImage();
  bool upgraded = !settings.empty() && instance->upgradeSettings(settings);
  // after the boot's image is taken, so that the stop saves these settings
  if (image.empty() && !upgraded && onFreshFlash) onFreshFlash();
  // read before anything else can come in, so that a fresh flash is one with only these
  freshLive.clear();
  if (image.empty() && !upgraded && !flashInFile) {
    for (int i = 0; i < 100000 && instance->midiInPending(); ++i) instance->loopOnce();
    freshLive = instance->liveFlashImage();
  }
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
  if (!flashInFile) {
    const juce::ScopedLock flashGuard(flashLock);
    bootFlash = keptFlash();
    bootSettingsSize = bootSettingsOffset = 0;
  }
  if (input != nullptr) input->stop();
  input.reset();
  output.reset();
  const juce::ScopedLock lock(touchLock);
  touches.clear();
  const juce::ScopedLock outGuard(outLock);
  midiOut.clear();
}

void Engine::onFirmware(std::function<void(firmware::Instance&)> change) {
  const juce::ScopedLock lock(changesLock);
  changes.push_back(std::move(change));
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
    held = advanceTouches(length / 1000.0);
    std::vector<std::function<void(firmware::Instance&)>> due;
    {
      const juce::ScopedLock lock(changesLock);
      due.swap(changes);
    }
    for (auto& change : due) change(*instance);
    if (flashWanted.exchange(false)) {
      takenFlash = flashInFile ? instance->flashImage() : keptFlash();
      flashTaken.signal();
    }
    if (++scansRun % 2000 == 0) {
      saveFlash();
    }
  }
}

// moves every touch's pressure on by a scan that took this many milliseconds, so that the firmware
// sees it rise and fall as a finger's would, and returns how many cells are held, since that sets
// how long the next scan takes
int Engine::advanceTouches(double millis) {
  const juce::ScopedLock lock(touchLock);
  int held = 0;
  for (auto it = touches.begin(); it != touches.end();) {
    Touch& t = it->second;
    if (t.targetZ == 0) {
      // the finger is leaving, so the pressure drops to nothing, slowly at first and then faster
      if (t.lifted < 0) {
        t.lifted = 0;
        t.liftedFrom = t.z;
      }
      t.lifted += millis;
      double left = 1.0 - std::pow(t.lifted / liftMillis, 2.0);
      t.z = left > 0 ? (int) (t.liftedFrom * left) : 0;
      if (t.z <= 0) {
        instance->setSensor(t.col, t.row, 0, 0, 0);
        it = touches.erase(it);
        continue;
      }
      instance->setSensor(t.col, t.row, t.x, t.y, (uint16_t) t.z);
      ++held;
      ++it;
      continue;
    }
    // a new press on a cell the firmware still holds, lifting or lifted: the lift goes out first,
    // for as long as the firmware takes to let go of the cell, or a quick second tap would only
    // carry on the first
    if (t.liftFirst) {
      if (instance->cellHeld(t.col, t.row) && t.liftWaited <= liftWaitMillis) {
        instance->setSensor(t.col, t.row, 0, 0, 0);
        t.liftWaited += millis;
        ++it;
        continue;
      }
      t.liftFirst = false;
    }
    // a tap quicker than the firmware takes to let go of the one before still reaches it
    if (t.releaseSeen) {
      t.liftWaited += millis;
      if (instance->cellHeld(t.col, t.row) || t.liftWaited > 2 * liftWaitMillis) {
        t.releaseSeen = false;
        t.targetZ = 0;
        ++it;
        continue;
      }
    }
    if (t.releaseIn > 0 && --t.releaseIn == 0) {
      t.targetZ = 0;
      instance->setSensor(t.col, t.row, 0, 0, 0);
      it = touches.erase(it);
      continue;
    }
    // a strike first lands at its own level, since that's what the firmware takes the velocity
    // from. After that it drops to the pressure if it landed above it, or it rises towards it at
    // its own speed and eases in at the end. Once it's there, it follows changes to the pressure
    // within a few milliseconds.
    double target = t.targetZ;
    if (t.landedScans < landingScans) {
      t.rising = t.landing;
      ++t.landedScans;
    }
    else if (t.settled) {
      t.rising += (target - t.rising) * juce::jmin(1.0, millis / 5.0);
    }
    else if (t.rising > target) {
      t.rising = target;
      t.settled = true;
    }
    else {
      double remaining = target - t.rising, easing = target / 4;
      double step = riseRate(t.strike, target) * millis * (remaining < easing ? juce::jmax(0.1, remaining / easing) : 1.0);
      t.rising = juce::jmin(target, t.rising + step);
      if (target - t.rising < 1) t.settled = true;
    }
    if (t.settled && std::abs(target - t.rising) < 1) t.rising = target;
    t.z = juce::roundToInt(t.rising);
    // a finger crossing into the next cell moves its pressure over, which the firmware adds back up
    if (t.releaseIn > 0) t.z = t.shared * t.releaseIn / slideScans;
    else if (t.sharedIn > 0) t.z = juce::jmax(1, t.z - t.shared * juce::jmax(0, --t.sharedIn - slideLead) / slideScans);
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
  t.strike = strike;
  t.landing = strikeLevel(strike);
  t.liftFirst = true;
  auto it = touches.find(touchKey(col, row));
  if (it != touches.end()) {
    if (it->second.liftFirst) t.liftWaited = it->second.liftWaited;
    else if (it->second.targetZ > 0) {
      t.liftFirst = false;
      t.z = it->second.z;
      t.rising = it->second.rising;
      t.landedScans = it->second.landedScans;
      t.settled = it->second.settled;
    }
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
  to.liftWaited = 0;
  to.releaseIn = 0;
  to.shared = from.shared = juce::roundToInt(from.rising);
  to.sharedIn = slideScans;
  from.xFraction = right ? 1.0f : 0.0f;
  from.x = instance->rawX(fromCol, from.xFraction);
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
  if (it == touches.end() || it->second.targetZ == 0 || it->second.releaseSeen) return false;
  if (it->second.liftFirst) it->second.releaseSeen = true;
  else it->second.targetZ = 0;
  return true;
}

bool Engine::isTouched(int col, int row) const {
  const juce::ScopedLock lock(touchLock);
  auto it = touches.find(touchKey(col, row));
  return it != touches.end() && it->second.targetZ > 0 && !it->second.releaseSeen;
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
  // without virtual ports, its MIDI is carried by the program that made it, which needs no saying
  if (hasVirtualPorts()) text += ", MIDI port \"" + portName() + "\"";
  else if (virtualPorts) text += ", no virtual MIDI port on this platform";
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

void Engine::setBootFlash(const std::vector<uint8_t>& image, size_t settingsSize, size_t settingsOffset) {
  const juce::ScopedLock lock(flashLock);
  bootFlash = image;
  bootSettingsSize = settingsSize;
  bootSettingsOffset = settingsOffset;
}

std::vector<uint8_t> Engine::currentFlash() {
  const juce::ScopedLock lock(flashLock);
  if (isThreadRunning()) {
    flashTaken.reset();
    flashWanted = true;
    // a thread that ends meanwhile, being stopped, leaves the flash to be read here, at once
    for (int waited = 0; waited < 1000 && isThreadRunning(); waited += 5)
      if (flashTaken.wait(5)) return takenFlash;
    flashWanted = false;
  }
  // stopped, the copy holds what it last had, bootFlash only once stop() has kept it
  if (!booted) return flashInFile ? std::vector<uint8_t>() : bootFlash;
  return flashInFile ? instance->flashImage() : keptFlash();
}

// the settings the firmware plays with, stored or not, but none for a fresh flash with only its
// starting settings, which then stays fresh
std::vector<uint8_t> Engine::keptFlash() {
  std::vector<uint8_t> live = instance->liveFlashImage();
  return bootedFrom.empty() && live == freshLive ? std::vector<uint8_t>() : live;
}

void Engine::saveFlash() {
  if (!flashInFile) return;
  std::vector<uint8_t> image = instance->flashImage();
  if (image == lastSavedFlash) return;
  juce::File file = flashFile(model);
  file.getParentDirectory().createDirectory();
  if (file.replaceWithData(image.data(), image.size())) {
    lastSavedFlash = image;
  }
}
