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
#include <cmath>
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
  // how many scans the cell a slide enters is ahead of the one it leaves, since the firmware adds what
  // it last read of the other one
  static constexpr int slideLead = 1;
  bool release(int col, int row);
  bool isTouched(int col, int row) const;

  // where a touch sits in its cell and how hard it presses, 0 to 1, for drawing it
  struct TouchView {
    float xFraction = 0.5f, yFraction = 0.5f, pressure = 0;
  };
  bool touchAt(int col, int row, TouchView& view) const;

  // how hard touches press, which held touches without a pressure of their own follow at once
  void setPressure(int rawZ);
  // how hard a press strikes, from 1 for the softest to 12 for the hardest, whatever pressure it
  // then rises to
  void setStrike(int strength) { strike = juce::jlimit(1, 12, strength); }
  // the firmware takes its velocity samples right after each other when it first reads a touch,
  // and here they all see the Z of that scan, so the velocity of a strike only depends on the
  // level it lands at. The softest lands just over the touch threshold (DEFAULT_SENSOR_LO_Z) and
  // the hardest gets a velocity of 127.
  static double strikeLevel(int strength) { return 150.0 * std::pow(1100.0 / 150.0, (strength - 1) / 11.0); }
  // the firmware's pass over the cells drifts against these scans, since it reads a new touch
  // several times in a row, so a strike stays at its level for two scans to be sure that it's read
  static constexpr int landingScans = 2;
  // how fast a press rises towards a pressure, in raw Z per millisecond. On a LinnStrument 128, soft
  // and medium presses took most of a second to get to a middling pressure of 1000 and only hard
  // strikes rose fast, in about 15 ms. Pressing harder means tensing harder and the silicone gets
  // stiffer as it's squeezed, so a higher pressure is reached sooner, in a time that shrinks with
  // its square root.
  static double riseRate(int strength, double pressure) {
    return 1000.0 / (800.0 * std::pow(15.0 / 800.0, std::pow((strength - 1) / 11.0, 3.0))) * std::pow(pressure / 1000.0, 1.5);
  }
  // a finger takes about 50 ms to leave, and its pressure drops faster towards the end
  static constexpr double liftMillis = 50;
  // the longest a press waits for the firmware to let go of the cell first, the sixteen scans a
  // cell takes while the sequencer is edited with time to spare
  static constexpr double liftWaitMillis = 150;
  void setFootSwitch(int which, bool down);

  // the running model's column count, readable from any thread
  int columnCount() const { return columns; }

  // MIDI into the firmware as if it came in on the virtual port, and the MIDI it sent since the
  // last call (the newest keptMessages are kept), for a driver without a MIDI port of its own
  void sendMidi(const uint8_t* data, size_t length);
  // a change to make in the firmware, made on its own thread between two of its loops, in the order
  // asked, as its own touch handlers would make it; from any thread
  void onFirmware(std::function<void(firmware::Instance&)> change);
  std::vector<std::vector<uint8_t>> takeMidiOut();
  static constexpr size_t keptMessages = 4096;
  // every message the firmware sends, on the firmware's thread; set it before start()
  std::function<void(const juce::MidiMessage&)> onMidiOut;
  // the starting settings when start() finds none saved, on the thread that calls it
  std::function<void()> onFreshFlash;
  // where each model's flash is saved, the app's data folder unless set before start()
  juce::File flashFolder;
  // an engine whose MIDI and settings the program that made it carries has no virtual port and no
  // flash file: set before start(), it then boots with bootFlash, a fresh flash when that's
  // empty, and keeps the flash there when it stops
  bool virtualPorts = true;
  bool flashInFile = true;
  std::vector<uint8_t> bootFlash;
  // the size and the place of the settings in bootFlash when a firmware with another layout saved
  // it, or 0 for this one's; only its settings are then taken, converted
  size_t bootSettingsSize = 0, bootSettingsOffset = 0;
  // the same, from a thread other than the one that asks for currentFlash()
  void setBootFlash(const std::vector<uint8_t>& image, size_t settingsSize = 0, size_t settingsOffset = 0);
  // the flash a later boot would start from, from any thread, taken between two of the firmware's
  // loops while it runs; without a file, with the settings the firmware plays with whether it
  // stored them or not, and empty while a fresh flash has only its starting settings
  std::vector<uint8_t> currentFlash();

  juce::String portName() const { return "Virtual LinnStrument"; }
  // the ports this engine plays out of and listens on, as the lists of MIDI inputs and outputs name
  // them, which JUCE gives without the prefix it marks its own virtual ports with; another Virtual
  // LinnStrument shares their name
  juce::String outputIdentifier() const { return output != nullptr ? listed(output->getIdentifier()) : juce::String(); }
  juce::String inputIdentifier() const { return input != nullptr ? listed(input->getIdentifier()) : juce::String(); }
  static juce::String listed(const juce::String& id) { return id.startsWith("VIRTUAL") ? id.substring(7) : id; }
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
    bool liftFirst;     // pressed where the firmware may not yet have let go of a touch before
    int releaseIn;      // scans until it lets go, when it's the cell a slide is leaving
    int shared = 0;     // the pressure a slide moves from one cell to the next
    int sharedIn = 0;   // scans until the cell a slide is entering has it all
    int strike = 10;    // how hard it struck, which sets how fast it rises
    double landing = 0; // the level it lands at in the first scan
    double rising = 0;  // its pressure as it rises, before it's rounded to z
    int landedScans = 0;
    bool settled = false;
    double lifted = -1; // how many milliseconds it's been letting go, or -1 while it's held
    int liftedFrom = 0;
    double liftWaited = 0; // how long a press has waited for the firmware to let go of the cell
    bool releaseSeen = false; // let go of while it waited, so it lets go once the firmware has it
  };

  void run() override;
  void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override;
  int advanceTouches(double millis);
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
  int strike = 10;
  juce::CriticalSection touchLock;
  std::map<int, Touch> touches;
  juce::MidiDataConcatenator concatenator { 2048 };
  OutputParser parser { *this };
  std::unique_ptr<juce::MidiOutput> output;
  std::unique_ptr<juce::MidiInput> input;
  juce::CriticalSection outLock;
  std::deque<std::vector<uint8_t>> midiOut;
  std::vector<uint8_t> lastSavedFlash;
  bool booted = false;
  std::vector<uint8_t> bootedFrom;
  std::vector<uint8_t> keptFlash();
  // a fresh flash with its starting settings
  std::vector<uint8_t> freshLive;
  std::atomic<bool> flashWanted { false };
  juce::CriticalSection flashLock;
  juce::CriticalSection changesLock;
  std::vector<std::function<void(firmware::Instance&)>> changes;
  juce::WaitableEvent flashTaken;
  std::vector<uint8_t> takenFlash;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Engine)
};
