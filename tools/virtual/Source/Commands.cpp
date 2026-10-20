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
#include "Commands.h"

#include "Firmware/FirmwareBridge.h"
#include "MainComponent.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

const char* Commands::help() {
  return
    "press COL ROW [X Y [Z]]   touch a cell, X and Y 0 to 1 inside it (default 0.5), Z 1 to 4095 (0 or\n"
    "                          none: the pressure slider)\n"
    "move COL ROW X Y [Z]      move a touch inside its cell, keeping its pressure unless Z is given\n"
    "release COL ROW           lift a touch\n"
    "tap COL ROW [MS]          press and release after MS milliseconds (default 120, at least 20)\n"
    "foot left|right down|up   a foot switch\n"
    "leds                      the lights as \"ROW: c c c ...\" per row from the top, a color number per\n"
    "                          column, 0 off, 7 black (dark)\n"
    "status                    the instrument's status line\n"
    "send BYTE [BYTE ...]      MIDI into the firmware, bytes in hex, like B0 14 05\n"
    "received [SECONDS]        the MIDI the firmware sent since the last received, one message per\n"
    "                          line in hex, after waiting up to SECONDS for a first one (default 0)\n"
    "screenshot FILE.png [S]   write the window to a file (quote a path with spaces), at S times its\n"
    "                          size (default 2, as on a retina screen)\n"
    "model 200|128             restart as the other model; the MIDI port is recreated, so a listener\n"
    "                          has to reconnect\n"
    "sleep SECONDS             wait\n"
    "help                      this text\n"
    "quit                      leave\n";
}

Commands::Commands(MainComponent& mainComponent, const juce::String& source) : main(mainComponent) {
  std::unique_ptr<std::istream> in;
  if (source == "-") {
    fromStdin = true;
    in = std::make_unique<std::istream>(std::cin.rdbuf());
  }
  else {
    auto file = std::make_unique<std::ifstream>(source.toStdString());
    if (!file->is_open()) {
      // the protocol is on stdout, so a driver sees why nothing follows
      std::cout << "error: cannot read " << source << std::endl;
      juce::MessageManager::callAsync([] {
        juce::JUCEApplication::getInstance()->setApplicationReturnValue(1);
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
      });
      return;
    }
    in = std::move(file);
  }
  thread = std::thread([this, state = reader, stream = std::move(in)]() mutable { readLoop(state, std::move(stream)); });
}

// a reader that finished is joined; one still blocked in getline (stdin, a pipe, a fifo) cannot
// be, so it is told to stop, given the moment it needs to finish a command in flight, and detached
Commands::~Commands() {
  reader->stop = true;
  if (!thread.joinable()) return;
  for (int i = 0; i < 600 && reader->busy; ++i) juce::Thread::sleep(10);
  if (reader->done) thread.join();
  else thread.detach();
}

void Commands::readLoop(std::shared_ptr<Reader> state, std::unique_ptr<std::istream> in) {
  std::string line;
  while (!state->stop && std::getline(*in, line)) {
    if (state->stop) break;
    juce::String text = juce::String(line).trim();
    if (text.isEmpty() || text.startsWithChar('#')) continue;
    juce::StringArray words = juce::StringArray::fromTokens(text, " \t", "\"'");
    for (auto& word : words) word = word.unquoted();
    state->busy = true;
    juce::String reply = state->stop ? juce::String("error: stopped") : execute(words);
    std::cout << reply.toStdString() << std::endl;
    state->busy = false;
    if (words[0].toLowerCase() == "quit") break;
  }
  state->done = true;
  // a driver on stdin that went away (its pipe closed) leaves nothing to drive the app with
  if (fromStdin && !state->stop) {
    juce::MessageManager::callAsync([] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });
  }
}

// the result and the event live in shared state, so a lambda that runs after a timeout writes
// nothing that is gone; the wait ends early when the app is leaving
juce::String Commands::onMessageThread(std::function<juce::String()> action) {
  struct Pending {
    juce::String result;
    juce::WaitableEvent done;
  };
  auto pending = std::make_shared<Pending>();
  juce::MessageManager::callAsync([pending, action] {
    pending->result = action();
    pending->done.signal();
  });
  for (int waited = 0; !pending->done.wait(50); waited += 50) {
    if (reader->stop) return "error: stopped";
    if (waited >= 5000) return "error: timed out waiting for the app";
  }
  return pending->result;
}

// A number and nothing else, finite, within lo..hi: clamped when clamp is set, refused
// otherwise. Parsed once here, so no other parser gets to disagree.
static bool number(const juce::String& word, double lo, double hi, bool clamp, double& out) {
  const std::string text = word.toStdString();
  if (text.empty() || !word.containsOnly("0123456789+-.eE")) return false;
  char* end = nullptr;
  double value = std::strtod(text.c_str(), &end);
  if (end == nullptr || *end != '\0' || end == text.c_str() || !std::isfinite(value)) return false;
  if (clamp) value = juce::jlimit(lo, hi, value);
  else if (value < lo || value > hi) return false;
  out = value;
  return true;
}
static bool whole(double value) {
  const double fraction = value - std::trunc(value);
  return !(fraction > 0.0 || fraction < 0.0);
}
static constexpr int ROWS = 8;
static constexpr double LONGEST_WAIT_MS = 3600.0 * 1000;

// sleeps in slices so a stop request ends a long wait
void Commands::pause(int milliseconds) {
  while (milliseconds > 0 && !reader->stop) {
    int slice = juce::jmin(50, milliseconds);
    juce::Thread::sleep(slice);
    milliseconds -= slice;
  }
}

static bool cellArguments(const juce::StringArray& words, int columns, int& col, int& row, juce::String& error) {
  if (words.size() < 3) { error = "error: " + words[0] + " needs COL ROW"; return false; }
  double c = 0, r = 0;
  if (!number(words[1], 0, columns - 1, false, c) || !number(words[2], 0, ROWS - 1, false, r) || !whole(c) || !whole(r)) {
    error = "error: column 0 to " + juce::String(columns - 1) + ", row 0 to " + juce::String(ROWS - 1);
    return false;
  }
  col = (int) c;
  row = (int) r;
  return true;
}

juce::String Commands::execute(const juce::StringArray& words) {
  const juce::String command = words[0].toLowerCase();
  Engine& engine = main.getEngine();
  const int columns = engine.columnCount();
  int col = 0, row = 0;
  juce::String error;

  if (command == "press" || command == "move") {
    if (!cellArguments(words, columns, col, row, error)) return error;
    if (command == "move" && words.size() < 5) return "error: move needs COL ROW X Y";
    double x = 0.5, y = 0.5, z = 0;
    if ((words.size() > 3 && !number(words[3], 0, 1, true, x)) || (words.size() > 4 && !number(words[4], 0, 1, true, y)) ||
        (words.size() > 5 && !number(words[5], 0, 4095, true, z))) return "error: X, Y and Z are numbers";
    if (command == "press") { engine.press(col, row, (float) x, (float) y, (int) z); return "ok"; }
    return engine.move(col, row, (float) x, (float) y, (int) z) ? "ok" : "error: no touch at " + juce::String(col) + " " + juce::String(row);
  }
  if (command == "release") {
    if (!cellArguments(words, columns, col, row, error)) return error;
    return engine.release(col, row) ? "ok" : "error: no touch at " + juce::String(col) + " " + juce::String(row);
  }
  if (command == "tap") {
    if (!cellArguments(words, columns, col, row, error)) return error;
    double hold = 120;
    if (words.size() > 3 && !number(words[3], 20, LONGEST_WAIT_MS, true, hold)) return "error: MS is a number";
    // a shorter hold than a few scans would be released before the firmware saw it
    int ms = (int) hold;
    engine.press(col, row, 0.5f, 0.5f);
    pause(ms);
    if (reader->stop) return "error: stopped";
    engine.release(col, row);
    return "ok";
  }
  if (command == "foot") {
    if (words.size() < 3) return "error: foot needs left|right down|up";
    const juce::String side = words[1].toLowerCase(), action = words[2].toLowerCase();
    int which = side == "left" ? 0 : side == "right" ? 1 : -1;
    if (which < 0 || (action != "down" && action != "up")) return "error: foot needs left|right down|up";
    bool down = action == "down";
    return onMessageThread([this, state = reader, which, down] { if (state->stop) return juce::String("error: stopped"); main.setFootSwitch(which, down); return juce::String("ok"); });
  }
  if (command == "leds") {
    return onMessageThread([] {
      juce::String text;
      for (int r = firmware::rows() - 1; r >= 0; --r) {
        text += juce::String(r) + ":";
        for (int c = 0; c < firmware::columns(); ++c) {
          firmware::Led led = firmware::led(c, r);
          text += " " + juce::String(led.display == firmware::off ? 0 : led.color);
        }
        text += "\n";
      }
      return text + "ok";
    });
  }
  if (command == "status") {
    return onMessageThread([this, state = reader] { return state->stop ? juce::String("error: stopped") : main.getEngine().status() + "\nok"; });
  }
  if (command == "send") {
    if (words.size() < 2) return "error: send needs MIDI bytes in hex, like B0 14 05";
    std::vector<uint8_t> bytes;
    for (int i = 1; i < words.size(); ++i) {
      juce::String word = words[i].toLowerCase();
      if (word.startsWith("0x")) word = word.substring(2);
      if (word.isEmpty() || word.length() > 2 || !word.containsOnly("0123456789abcdef")) return "error: " + words[i] + " is not a byte in hex";
      bytes.push_back((uint8_t) word.getHexValue32());
    }
    engine.sendMidi(bytes.data(), bytes.size());
    return "ok";
  }
  if (command == "received") {
    double wait = 0;
    if (words.size() > 1 && !number(words[1], 0, LONGEST_WAIT_MS / 1000, true, wait)) return "error: received takes SECONDS to wait";
    std::vector<std::vector<uint8_t>> messages = engine.takeMidiOut();
    for (double waited = 0; messages.empty() && waited < wait * 1000 && !reader->stop; waited += 20) {
      pause(20);
      messages = engine.takeMidiOut();
    }
    juce::String text;
    for (auto& m : messages) text += juce::String::toHexString(m.data(), (int) m.size()).toUpperCase() + "\n";
    return text + "ok";
  }
  if (command == "screenshot") {
    if (words.size() < 2 || words.size() > 3) return "error: screenshot needs FILE.png, quoted when it has spaces, and at most a scale";
    double scale = 2;
    if (words.size() == 3 && !number(words[2], 0.25, 4, false, scale)) return "error: the scale is 0.25 to 4";
    juce::File file = juce::File::getCurrentWorkingDirectory().getChildFile(words[1]);
    return onMessageThread([this, state = reader, file, scale] { if (state->stop) return juce::String("error: stopped"); return main.saveScreenshot(file, (float) scale) ? juce::String("ok") : "error: cannot write " + file.getFullPathName(); });
  }
  if (command == "model") {
    double wanted = 0;
    if (words.size() < 2 || !number(words[1], 0, 1000, false, wanted) || !whole(wanted)) return "error: model needs 200 or 128";
    const int model = (int) wanted;
    if (model != 200 && model != 128) return "error: model needs 200 or 128";
    return onMessageThread([this, state = reader, model] { if (state->stop) return juce::String("error: stopped"); main.setModel(model); return juce::String("ok"); });
  }
  if (command == "sleep") {
    double seconds = 0;
    if (words.size() < 2 || !number(words[1], 0, LONGEST_WAIT_MS / 1000, true, seconds)) return "error: sleep needs SECONDS";
    pause((int) (seconds * 1000));
    return reader->stop ? "error: stopped" : "ok";
  }
  if (command == "help") {
    return juce::String(help()) + "ok";
  }
  if (command == "quit") {
    juce::MessageManager::callAsync([] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });
    return "ok";
  }
  return "error: unknown command " + words[0] + ", help lists them";
}
