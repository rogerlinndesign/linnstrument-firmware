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
// Runs the firmware on its own thread at the scan rate of the instrument, turns its serial
// output into messages on a virtual MIDI port, feeds that port's input back in, holds the
// touches the surface makes and keeps the flash contents in a file between runs.
#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <vector>

namespace firmware {
class Instance;
}

class Engine : private juce::Thread,
               private juce::MidiInputCallback {
public:
  // takes a copy of the firmware of its own, and fails when none is free
  Engine();
  ~Engine() override;
  firmware::Instance& firmware() const { return *instance; }

  void start(int model);
  void stop();
  bool isRunning() const { return isThreadRunning(); }

  // fractions are 0 to 1 inside the cell, left to right and bottom to top; rawZ 0 takes the
  // pressure the slider sets
  void press(int col, int row, float xFraction, float yFraction, int rawZ = 0);
  bool move(int col, int row, float xFraction, float yFraction, int rawZ = 0);
  // a finger sliding from a cell into the next one in its row; false when it isn't such a move
  bool slide(int fromCol, int row, int toCol, float yFraction);
  // the scans after a slide until the cell left behind lets go, both cells down in all but the last
  static constexpr int slideScans = 4;
  bool release(int col, int row);
  bool isTouched(int col, int row) const;

  // where a touch sits in its cell and how hard it presses, 0 to 1, for drawing it
  struct TouchView {
    float xFraction = 0.5f, yFraction = 0.5f, pressure = 0;
  };
  bool touchAt(int col, int row, TouchView& view) const;

  // how hard touches press, which held touches without a pressure of their own follow at once
  void setPressure(int rawZ);
  void setVelocityScans(int scans) { velocityScans = juce::jmax(1, scans); }
  void setFootSwitch(int which, bool down);

  // the running model's column count, readable from any thread
  int columnCount() const { return columns; }

  // MIDI into the firmware as if it came in on the virtual port, and the MIDI it sent since the
  // last call (the newest keptMessages are kept), for a driver without a MIDI port of its own
  void sendMidi(const uint8_t* data, size_t length);
  std::vector<std::vector<uint8_t>> takeMidiOut();
  static constexpr size_t keptMessages = 4096;
  // every message the firmware sends, on the firmware's thread; set it before start()
  std::function<void(const juce::MidiMessage&)> onMidiOut;
  // the starting settings when start() finds none saved, on the thread that calls it
  std::function<void()> onFreshFlash;
  // where each model's flash is saved, the app's data folder unless set before start()
  juce::File flashFolder;

  juce::String portName() const { return "Virtual LinnStrument"; }
  juce::String status() const;
  bool hasVirtualPorts() const { return output != nullptr && input != nullptr; }

  // A full scan of the surface takes as long as on the instrument, measured on a LinnStrument 200:
  // 27 us for each cell and 0.58 ms more for every touch held. The firmware counts its quantize
  // hold, its smoothing of pressure and timbre and its release delays in scans, so they run at the
  // instrument's speed only at its scan rate.
  static constexpr double microsPerCell = 27.16, microsPerTouch = 580;
  static double scanMicros(int cells, int held) { return cells * microsPerCell + held * microsPerTouch; }
  // scans run since start(), from any thread
  int scanCount() const { return scansRun; }

  // the LEDs blink in the firmware's own rhythm
  static constexpr int refreshHz = 30;

private:
  struct Touch {
    int col, row;
    uint16_t x, y;
    int targetZ;
    int z;
    float xFraction, yFraction;
    bool ownPressure;   // pressed with an explicit Z, which a move without Z keeps
    bool liftFirst;     // replaced a touch the firmware has not yet seen lifted
    int releaseIn;      // scans until it lets go, when it's the cell a slide is leaving
  };

  void run() override;
  void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override;
  int advanceTouches();
  juce::File flashFile(int model) const;
  void saveFlash();

  struct OutputParser {
    Engine& engine;
    void handleIncomingMidiMessage(void*, const juce::MidiMessage& message);
    void handlePartialSysexMessage(void*, const uint8_t*, int, double) {}
  };
  void serialSink(const uint8_t* data, size_t length);

  firmware::Instance* instance;
  int model = 200;
  std::atomic<int> columns { 26 };
  std::atomic<int> scansRun { 0 };
  std::atomic<bool> finishing { false };   // stopping, once the firmware's MIDI is out
  int pressure = 600;
  int velocityScans = 3;
  juce::CriticalSection touchLock;
  std::map<int, Touch> touches;
  juce::MidiDataConcatenator concatenator { 2048 };
  OutputParser parser { *this };
  std::unique_ptr<juce::MidiOutput> output;
  std::unique_ptr<juce::MidiInput> input;
  juce::CriticalSection outLock;
  std::deque<std::vector<uint8_t>> midiOut;
  std::vector<uint8_t> lastSavedFlash;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Engine)
};
