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
// Runs the firmware on its own thread at about the scan rate of the instrument, turns its serial
// output into messages on a virtual MIDI port, feeds that port's input back in, holds the
// touches the surface makes and keeps the flash contents in a file between runs.
#pragma once

#include <JuceHeader.h>

#include <map>

class Engine : private juce::Thread,
               private juce::MidiInputCallback {
public:
  Engine();
  ~Engine() override;

  void start(int model);
  void stop();
  bool isRunning() const { return isThreadRunning(); }

  // fractions are 0 to 1 inside the cell, left to right and bottom to top
  void press(int col, int row, float xFraction, float yFraction);
  void move(int col, int row, float xFraction, float yFraction);
  void release(int col, int row);
  bool isTouched(int col, int row) const;

  // where a touch sits in its cell and how hard it presses, 0 to 1, for drawing it
  struct TouchView {
    float xFraction = 0.5f, yFraction = 0.5f, pressure = 0;
  };
  bool touchAt(int col, int row, TouchView& view) const;

  void setPressure(int rawZ) { pressure = rawZ; }
  void setVelocityScans(int scans) { velocityScans = juce::jmax(1, scans); }
  void setFootSwitch(int which, bool down);

  juce::String portName() const { return "Virtual LinnStrument"; }
  juce::String status() const;
  bool hasVirtualPorts() const { return output != nullptr && input != nullptr; }

  // the LEDs blink in the firmware's own rhythm
  static constexpr int refreshHz = 30;

private:
  struct Touch {
    int col, row;
    uint16_t x, y;
    int targetZ;
    int z;
    float xFraction, yFraction;
  };

  void run() override;
  void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override;
  void advanceTouches();
  juce::File flashFile(int model) const;
  void saveFlash();

  struct OutputParser {
    Engine& engine;
    void handleIncomingMidiMessage(void*, const juce::MidiMessage& message);
    void handlePartialSysexMessage(void*, const uint8_t*, int, double) {}
  };
  static void serialSink(const uint8_t* data, size_t length);
  static Engine* current;

  int model = 200;
  int pressure = 600;
  int velocityScans = 3;
  juce::CriticalSection touchLock;
  std::map<int, Touch> touches;
  juce::MidiDataConcatenator concatenator { 2048 };
  OutputParser parser { *this };
  std::unique_ptr<juce::MidiOutput> output;
  std::unique_ptr<juce::MidiInput> input;
  std::vector<uint8_t> lastSavedFlash;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Engine)
};
