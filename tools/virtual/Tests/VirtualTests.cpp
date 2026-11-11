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
// Tests for the Virtual LinnStrument's own code, the parts the firmware's desktop tests can't
// reach: tracking fingers, the surface, the engine's pace and the firmware bridge.
//   VirtualTests [NAME]   runs the tests whose name contains NAME, or all of them
#if defined(__APPLE__)
// before JUCE, whose Point the system's would clash with
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <JuceHeader.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

#include "Engine.h"
#include "Firmware/FirmwareBridge.h"
#include "LinnLookAndFeel.h"
#include "Surface.h"
#include "TouchTracker.h"

namespace {

// the suite the test bodies below run in, for their expect calls
juce::UnitTest* current = nullptr;
void expect(bool condition, const juce::String& what) { current->expect(condition, what); }
void expectNear(double value, double expected, double tolerance, const juce::String& what) {
  current->expectWithinAbsoluteError(value, expected, tolerance, what);
}
#define CHECK(condition) expect((condition), #condition)
#define CHECK_NEAR(value, expected, tolerance) expectNear((value), (expected), (tolerance), #value)

// every test is a function with a name; each suite below runs a group of them
struct Case {
  const char* name;
  void (*run)();
};

// ---------------------------------------------------------------------------------------------
// fingers

struct Recorder {
  std::vector<juce::String> log;
  TouchTracker tracker;
  Recorder() {
    tracker.press = [this](const TouchTracker::Cell& c) { log.push_back("press " + juce::String(c.col) + " " + juce::String(c.row)); };
    tracker.move = [this](const TouchTracker::Cell& c) { log.push_back("move " + juce::String(c.col) + " " + juce::String(c.row)); };
    tracker.release = [this](int col, int row) { log.push_back("release " + juce::String(col) + " " + juce::String(row)); };
    tracker.slide = [this](const TouchTracker::Cell& from, const TouchTracker::Cell& to) {
      log.push_back("slide " + juce::String(from.col) + " " + juce::String(from.row) + " to " + juce::String(to.col));
      return slides;
    };
  }
  bool slides = true;
  juce::String take() {
    auto text = juce::StringArray(log.data(), (int) log.size()).joinIntoString(", ");
    log.clear();
    return text;
  }
};

TouchTracker::Cell cell(int col, int row) {
  TouchTracker::Cell c;
  c.col = col;
  c.row = row;
  c.xFraction = c.yFraction = 0.5f;
  return c;
}

void fingers_play_their_own_cells() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.down(2, cell(5, 4), false);
  r.tracker.up(1);
  CHECK(r.take() == "press 3 2, press 5 4, release 3 2");
  r.tracker.up(2);
  CHECK(r.take() == "release 5 4");
}

void a_slide_presses_the_next_cell_before_letting_go_of_the_last() {
  Recorder r;
  r.slides = false;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.drag(1, cell(3, 2));
  r.tracker.drag(1, cell(4, 2));
  r.tracker.up(1);
  CHECK(r.take() == "press 3 2, move 3 2, slide 3 2 to 4, press 4 2, release 3 2, release 4 2");
}

void a_cell_stays_down_while_another_finger_is_on_it() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.down(2, cell(3, 2), false);
  r.tracker.up(1);
  CHECK(r.take() == "press 3 2, press 3 2");
  r.tracker.up(2);
  CHECK(r.take() == "release 3 2");
}

void moving_into_the_next_cell_of_a_row_slides() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.drag(1, cell(4, 2));
  r.tracker.drag(1, cell(4, 2));
  r.tracker.up(1);
  CHECK(r.take() == "press 3 2, slide 3 2 to 4, move 4 2, release 4 2");
}

void a_move_that_cannot_slide_is_a_press_and_a_release() {
  Recorder r;
  r.slides = false;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.drag(1, cell(4, 2));
  CHECK(r.take() == "press 3 2, slide 3 2 to 4, press 4 2, release 3 2");
}

void moving_to_another_row_or_further_does_not_slide() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.drag(1, cell(3, 3));
  r.tracker.drag(1, cell(5, 3));
  CHECK(r.take() == "press 3 2, press 3 3, release 3 2, press 5 3, release 3 3");
}

void a_cell_another_finger_holds_does_not_slide_away() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.down(2, cell(3, 2), false);
  r.tracker.drag(1, cell(4, 2));
  CHECK(r.take() == "press 3 2, press 3 2, press 4 2");
}

void sliding_off_a_shared_cell_leaves_it_down() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.down(2, cell(3, 2), false);
  r.tracker.drag(1, cell(4, 2));
  CHECK(r.take() == "press 3 2, press 3 2, press 4 2");
}

void a_latched_cell_stays_down_until_touched_again() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), true);
  r.tracker.up(1);
  CHECK(r.take() == "press 3 2");
  CHECK(r.tracker.isLatched(3, 2));
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.up(1);
  CHECK(r.take() == "release 3 2");
  CHECK(!r.tracker.isLatched(3, 2));
}

// a finger holds the cell while the keyboard latches it and lets it go again: the finger keeps it
void the_keyboard_lets_go_only_of_a_cell_no_finger_holds() {
  Recorder r;
  r.tracker.down(1, cell(3, 2), false);
  r.tracker.toggle(cell(3, 2));
  r.tracker.toggle(cell(3, 2));
  CHECK(r.take() == "press 3 2, press 3 2");
  r.tracker.up(1);
  CHECK(r.take() == "release 3 2");
}

void the_keyboard_toggles_a_cell() {
  Recorder r;
  r.tracker.toggle(cell(3, 2));
  r.tracker.toggle(cell(3, 2));
  CHECK(r.take() == "press 3 2, release 3 2");
}

// ---------------------------------------------------------------------------------------------
// the instrument, booted once as a 128 for every test that needs one

// its flash in a folder of its own, so that the app's saved settings never reach the tests
// one of its own for each run of the tests, so that shares run side by side never store into the
// flash file another is reading
juce::File testFlash() {
  static const juce::String run = juce::String::toHexString(juce::Random::getSystemRandom().nextInt64());
  return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("VirtualTestsFlash-" + run);
}

// stopped before the program ends, so that its thread never runs into the shutdown
Engine* started = nullptr;

Engine& engine() {
  static Engine* running = [] {
    auto* e = new Engine();
    testFlash().deleteRecursively();
    e->flashFolder = testFlash();
    e->start(128);
    started = e;
    return e;
  }();
  return *running;
}

bool waitFor(std::function<bool()> condition) {
  for (int i = 0; i < 200; ++i) {
    if (condition()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return false;
}

void the_bridge_reports_a_settings_screen() {
  auto& e = engine();
  // held for a moment, as a finger would, so that the firmware's scan sees it
  auto tap = [&e] {
    e.press(0, 0, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(0, 0);
  };
  CHECK(!engine().firmware().showingSettings());
  tap();
  CHECK(waitFor([] { return engine().firmware().showingSettings(); }));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  tap();
  CHECK(waitFor([] { return !engine().firmware().showingSettings(); }));
}

void nrpn(Engine& e, int parameter, int value);

// the split a screen shows, a program sent in, the volume slid on its screen, and the small font
void the_bridge_reads_a_screens_values_and_the_small_font() {
  auto& e = engine();
  auto tap = [&e](int col, int row) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  auto channels = e.firmware().noteChannels(0);
  CHECK(!channels.empty());
  const uint8_t change[] = { (uint8_t) (0xc0 | (channels[0] - 1)), 41 };
  e.sendMidi(change, sizeof(change));
  CHECK(waitFor([&e] { return e.firmware().program(0) == 42; }));
  tap(0, 5);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::volumeScreen; }));
  tap(16, 7);
  CHECK(waitFor([&e] { return e.firmware().shownSplit() == 1; }));
  tap(15, 7);
  CHECK(waitFor([&e] { return e.firmware().shownSplit() == 0; }));
  for (int col : { 1, e.firmware().columns() - 1 }) {
    tap(col, 3);
    CHECK(waitFor([&e, col] { return col == 1 ? e.firmware().volume(0) < 10 : e.firmware().volume(0) > 117; }));
  }
  tap(0, 5);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
  auto one = e.firmware().smallGlyph('1');
  CHECK(one.width == 2 && one.rows.size() == 8);
  const std::vector<std::string> drawn { "  ", " 0", "00", " 0", " 0", " 0", " 0", "  " };
  CHECK(one.rows == drawn);
  CHECK(e.firmware().smallGlyph('A').width > 0);
  CHECK(e.firmware().smallGlyph('\x01').rows.empty());
  // the colours a split and the global screens draw in, which the firmware sets over NRPN for a split
  nrpn(e, 30, 9);
  CHECK(waitFor([&e] { return e.firmware().mainColor(0) == 9; }));
  nrpn(e, 30, 3);
  CHECK(waitFor([&e] { return e.firmware().mainColor(0) == 3; }));
  CHECK(e.firmware().settingsColor() == 5);
}

// each split's special feature read back as it was set: the arpeggiator, CC faders, strum, the
// sequencer and none
void the_bridge_reads_each_splits_special_feature() {
  auto& e = engine();
  auto& f = e.firmware();
  for (int parameter : { 35, 135 }) {
    int before = f.nrpn(parameter);
    for (int value : { 1, 2, 3, 4, 0 }) {
      nrpn(e, parameter, value);
      CHECK(waitFor([&f, parameter, value] { return f.nrpn(parameter) == value; }));
    }
    nrpn(e, parameter, before);
  }
}

// Notes view's chosen step's notes lit in the played colour, and only its: a step chosen after another
// leaves the other's pads as the playing surface lights them, as the instrument's own choosing does
void the_bridge_lights_only_the_chosen_steps_notes_in_notes_view() {
  auto& e = engine();
  auto& f = e.firmware();
  int special = f.nrpn(35);
  nrpn(e, 35, 4);
  CHECK(waitFor([&f] { return f.screen() == firmware::sequencerScreen; }));
  int s = f.sequencerShown(), p = f.sequencerState(s).pattern;
  e.onFirmware([s, p](firmware::Instance& i) {
    i.playSequencer(s, false);
    i.clearSequencerPattern(s, p);
    i.setSequencerView(s, 1);
    i.addSequencerNote(s, 0, 60, 3, 100, 64);
    i.addSequencerNote(s, 3, 67, 5, 100, 64);
    i.setSequencerView(s, 0);
    i.selectSequencerNote(s, 0, 0);
  });
  CHECK(waitFor([&f, s] { auto st = f.sequencerState(s); return st.view == 0 && st.selectedStep == 0; }));
  auto played = f.sequencerColours(s).played;
  auto litFor = [&f, s, played](int note) {
    std::vector<std::pair<int, int>> lit;
    for (int col = 1; col < f.columns(); ++col)
      for (int row = 1; row < 8; ++row)
        if (f.padNote(s, col, row) == note && f.led(col, row).color == played) lit.push_back({ col, row });
    return lit;
  };
  CHECK(waitFor([&] { return !litFor(60).empty(); }));
  auto first = litFor(60);
  e.onFirmware([s](firmware::Instance& i) { i.selectSequencerNote(s, 3, 0); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).selectedStep == 3; }));
  CHECK(waitFor([&] { return !litFor(67).empty(); }));
  for (auto [col, row] : first) CHECK(f.led(col, row).color != played);
  e.onFirmware([s, p](firmware::Instance& i) { i.clearSequencerPattern(s, p); i.setSequencerView(s, 1); i.selectSequencerNote(s, -1, -1); });
  nrpn(e, 35, special == firmware::Instance::nrpnUnknown ? 0 : special);
  CHECK(waitFor([&f] { return f.screen() != firmware::sequencerScreen; }));
}

// Global Settings' Scale Select view as the firmware keeps it: its View cell touched, then Main again
void the_bridge_reads_whether_scale_select_shows() {
  auto& e = engine();
  auto tap = [&e](int col, int row) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  tap(0, 0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::globalScreen; }));
  // from the lights the tests before left, and back to them
  bool before = e.firmware().scaleSelectShown();
  tap(1, 0);
  CHECK(waitFor([&e] { return !e.firmware().scaleSelectShown(); }));
  tap(1, 2);
  CHECK(waitFor([&e] { return e.firmware().scaleSelectShown(); }));
  tap(1, 0);
  CHECK(waitFor([&e] { return !e.firmware().scaleSelectShown(); }));
  if (before) tap(1, 2);
  tap(0, 0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
}

// a split's CC faders as the firmware keeps them: shown while it's set to play them, the columns
// they span, and each row's value as a CC to it sets it
void the_bridge_reads_a_splits_cc_faders() {
  auto& e = engine();
  CHECK(!e.firmware().ccFaders(0).shown);
  nrpn(e, 35, 2);
  CHECK(waitFor([&e] { return e.firmware().ccFaders(0).shown; }));
  auto faders = e.firmware().ccFaders(0);
  CHECK(faders.left == 1 && faders.length == (e.firmware().nrpn(200) == 1 ? e.firmware().nrpn(202) - 2 : e.firmware().columns() - 2));
  const uint8_t cc[] = { 0xB0, 2, 77 };
  e.sendMidi(cc, sizeof(cc));
  CHECK(waitFor([&e] { return e.firmware().ccFaders(0).values[1] == 77; }));
  nrpn(e, 35, 0);
  CHECK(waitFor([&e] { return !e.firmware().ccFaders(0).shown; }));
}

// a settings cell held times its hold as the firmware's handlers do, and stops when it acts: a
// second on Per-Split Settings' MIDI MODE opens nothing but resets the split's MIDI settings
void the_bridge_times_a_hold_as_the_firmware_does() {
  auto& e = engine();
  using Hold = firmware::Instance::Hold;
  CHECK(e.firmware().holdDelay(Hold::sensor) == 300 && e.firmware().holdDelay(Hold::controlSwitch) == 500);
  CHECK(e.firmware().holdDelay(Hold::confirm) == 800 && e.firmware().holdDelay(Hold::edit) == 1000);
  auto tap = [&e](int col, int row) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  CHECK(e.firmware().heldMillis(1, 7) == -1);
  tap(0, 7);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::perSplitScreen; }));
  e.press(1, 7, 0.5f, 0.5f);
  CHECK(waitFor([&e] { return e.firmware().heldMillis(1, 7) >= 200; }));
  int early = e.firmware().heldMillis(1, 7);
  CHECK(early < 900);
  CHECK(waitFor([&e] { return e.firmware().heldMillis(1, 7) == -1; }));
  e.release(1, 7);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  tap(0, 7);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
}

std::vector<std::vector<uint8_t>> drained(Engine& e) {
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  return e.takeMidiOut();
}

int noteOns(const std::vector<std::vector<uint8_t>>& messages) {
  int n = 0;
  for (auto& m : messages) n += m.size() == 3 && (m[0] & 0xf0) == 0x90 && m[2] > 0 ? 1 : 0;
  return n;
}

int lastBend(const std::vector<std::vector<uint8_t>>& messages) {
  int bend = 8192;
  for (auto& m : messages) {
    if (m.size() == 3 && (m[0] & 0xf0) == 0xe0) bend = m[1] | (m[2] << 7);
  }
  return bend;
}

// from the middle of a pad to its right edge, then into the next pad
void slideRight(Engine& e, int col, int row, bool straddle) {
  e.press(col, row, 0.5f, 0.5f);
  drained(e);
  for (float x = 0.55f; x <= 1.0f; x += 0.05f) {
    e.move(col, row, x, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  if (straddle) e.slide(col, row, col + 1, 0.5f);
  else {
    // lifted first, then pressed: a release that is still pending counts as a fast slide
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    e.press(col + 1, row, 0.0f, 0.5f);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  for (float x = 0.05f; x <= 0.5f; x += 0.05f) {
    e.move(col + 1, row, x, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// a setting changed over MIDI, the way an editor would, on the left split's channel 1
void nrpn(Engine& e, int parameter, int value) {
  uint8_t m[] = { 0xB0, 99, (uint8_t) (parameter >> 7), 0xB0, 98, (uint8_t) (parameter & 127),
                  0xB0, 6, (uint8_t) (value >> 7), 0xB0, 38, (uint8_t) (value & 127),
                  0xB0, 101, 127, 0xB0, 100, 127 };
  e.sendMidi(m, sizeof(m));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// the standard MPE configuration message for both zones, seven channels each, which the firmware
// answers by turning MPE on for both splits at a bend range of 48
void configureMpe(Engine& e) {
  for (uint8_t status : { (uint8_t) 0xB0, (uint8_t) 0xBF }) {
    uint8_t m[] = { status, 101, 0, status, 100, 6, status, 6, 7, status, 38, 0, status, 101, 127, status, 100, 127 };
    e.sendMidi(m, sizeof(m));
  }
}

// a change made on the firmware's thread sets the bend range as the firmware's own screen does,
// telling MPE synths of it, and holding ±24 for a second opens that screen
void the_engine_sets_the_bend_range_as_its_screen_does() {
  auto& e = engine();
  configureMpe(e);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  int before = e.firmware().bendRangeOf(0);
  e.takeMidiOut();
  e.onFirmware([](firmware::Instance& f) { f.setBendRange(0, 36); });
  CHECK(waitFor([&e] { return e.firmware().bendRangeOf(0) == 36; }));
  bool announced = false;
  CHECK(waitFor([&] {
    for (auto& m : e.takeMidiOut()) announced = announced || (m.size() == 3 && (m[0] & 0xf0) == 0xb0 && m[1] == 6 && m[2] == 36);
    return announced;
  }));
  e.onFirmware([](firmware::Instance& f) { f.setBendRange(0, 97); });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  CHECK(e.firmware().bendRangeOf(0) == 36);
  auto tap = [&e](int col, int row, int millis) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  CHECK(e.firmware().valueScreen() == firmware::Instance::ValueScreen::none);
  tap(0, 7, 150);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::perSplitScreen; }));
  tap(7, 4, 1300);
  CHECK(waitFor([&e] { return e.firmware().valueScreen() == firmware::Instance::ValueScreen::bendRange; }));
  // back to the grid as on the instrument: the split shown tapped again
  tap(15 + e.firmware().shownSplit(), 7, 150);
  CHECK(waitFor([&e] { return e.firmware().valueScreen() == firmware::Instance::ValueScreen::none && e.firmware().screen() == firmware::perSplitScreen; }));
  tap(0, 7, 150);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
  e.onFirmware([before](firmware::Instance& f) { f.setBendRange(0, before); });
  CHECK(waitFor([&e, before] { return e.firmware().bendRangeOf(0) == before; }));
}

// what the firmware answers an NRPN query (299) with, on the output of the engine
int askNrpn(Engine& e, int parameter) {
  e.takeMidiOut();
  nrpn(e, 299, parameter);
  int value = firmware::Instance::nrpnUnknown, at = -1, msb = 0;
  waitFor([&] {
    for (auto& m : e.takeMidiOut()) {
      if (m.size() != 3 || (m[0] & 0xf0) != 0xb0) continue;
      if (m[1] == 99) at = m[2] << 7;
      else if (m[1] == 98 && at >= 0) at |= m[2];
      else if (m[1] == 6) msb = m[2];
      else if (m[1] == 38 && at == parameter) value = (msb << 7) | m[2];
    }
    return value != firmware::Instance::nrpnUnknown;
  });
  return value;
}

// a driver reads every setting as the firmware reports it and sets it as an NRPN does
void the_bridge_reads_and_sets_the_panels_settings_as_nrpns_do() {
  auto& e = engine();
  const int parameters[] = { 19, 20, 24, 25, 26, 27, 28, 29, 39, 54, 55, 56, 57, 58, 59, 119, 125, 120, 139, 154, 158, 159,
                             227, 228, 229, 230, 231, 232, 239, 240, 241, 242, 249, 250, 251, 253,
                             255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270 };
  // away from the defaults, so that a read of the wrong field shows
  const int changes[][2] = { { 25, 11 }, { 54, 7 }, { 158, 1 }, { 55, 99 }, { 59, 33 }, { 125, 2 }, { 154, 21 }, { 228, 9 }, { 230, 13 },
                             { 229, 10 }, { 231, 12 }, { 239, 1 }, { 242, 1 }, { 255, 60 }, { 256, 61 }, { 257, 62 }, { 258, 63 },
                             { 249, 5 }, { 250, 120 }, { 251, 77 }, { 253, 20 }, { 259, 66 }, { 262, 67 }, { 263, 31 }, { 120, 0 }, { 270, 70 } };
  std::vector<int> before;
  for (auto& c : changes) before.push_back(e.firmware().nrpn(c[0]));
  for (auto& c : changes) {
    int p = c[0], v = c[1];
    e.onFirmware([p, v](firmware::Instance& f) { f.setNrpn(p, v); });
  }
  CHECK(waitFor([&e] { return e.firmware().nrpn(270) == 70; }));
  for (auto& c : changes) CHECK(e.firmware().nrpn(c[0]) == c[1]);
  for (int p : parameters) {
    int asked = askNrpn(e, p), read = e.firmware().nrpn(p);
    CHECK(asked == read);
    if (asked != read) std::cout << "NRPN " << p << ": the firmware answers " << asked << ", the bridge reads " << read << std::endl;
  }
  // out of range, as the firmware's checks have it, nothing changes
  e.onFirmware([](firmware::Instance& f) { f.setNrpn(54, 128); });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  CHECK(e.firmware().nrpn(54) == 7);
  CHECK(e.firmware().nrpn(1000) == firmware::Instance::nrpnUnknown && e.firmware().nrpn(327) == firmware::Instance::nrpnUnknown);
  for (size_t i = 0; i < before.size(); ++i) {
    int p = changes[i][0], v = before[i];
    e.onFirmware([p, v](firmware::Instance& f) { f.setNrpn(p, v); });
  }
  CHECK(waitFor([&] { return e.firmware().nrpn(270) == before.back(); }));
  // Per-Split Settings' CC1 cell makes Y CC 1 while its stored CC stays 74, which a query answers as 1
  auto tap = [&e](int col, int row) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  tap(0, 7);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::perSplitScreen; }));
  int split = e.firmware().shownSplit(), ccForY = split == 0 ? 25 : 125;
  tap(9, 6);
  CHECK(askNrpn(e, ccForY) == 1 && e.firmware().nrpn(ccForY) == 1);
  tap(9, 5);
  CHECK(waitFor([&] { return e.firmware().nrpn(ccForY) == 74; }));
  tap(0, 7);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
  // Global Settings' SELECT SW, column 7, a row for each switch
  tap(0, 0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::globalScreen; }));
  for (int row : { 3, 1 }) {
    tap(7, row);
    CHECK(waitFor([&e, row] { return e.firmware().selectedSwitch() == row; }));
  }
  tap(7, 0);
  tap(0, 0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
}

// a switch set by its index as the Global Settings grid sets it, both pedals included, the same
// assignment again switching it off
void the_bridge_assigns_the_switches_as_the_grid_does() {
  auto& e = engine();
  int before[5], both[5], sustain[5], cc65[5];
  for (int i = 0; i < 5; ++i) {
    before[i] = e.firmware().switchAssignment(i);
    both[i] = e.firmware().switchBothSplits(i);
    sustain[i] = e.firmware().switchCC(i, true);
    cc65[i] = e.firmware().switchCC(i, false);
  }
  e.onFirmware([](firmware::Instance& f) { f.assignSwitch(4, 9); f.setSwitchBothSplits(4, true); f.setSwitchCC(4, true, 70); f.setSwitchCC(4, false, 71); f.setSwitchCC(4, true, 128); });
  CHECK(waitFor([&e] { return e.firmware().switchCC(4, false) == 71; }));
  CHECK(e.firmware().switchAssignment(4) == 9 && e.firmware().switchBothSplits(4) && e.firmware().switchCC(4, true) == 70);
  CHECK(e.firmware().switchAssignment(0) == before[0] && !e.firmware().switchBothSplits(0) && e.firmware().switchCC(0, true) == sustain[0]);
  e.onFirmware([](firmware::Instance& f) { f.assignSwitch(4, 9); });
  CHECK(waitFor([&e] { return e.firmware().switchAssignment(4) == 255; }));
  e.onFirmware([](firmware::Instance& f) { f.assignSwitch(4, 20); });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  CHECK(e.firmware().switchAssignment(4) == 255);
  e.onFirmware([&before, &both, &sustain, &cc65](firmware::Instance& f) {
    for (int i = 0; i < 5; ++i) {
      int now = f.switchAssignment(i);
      if (before[i] == 255 && now != 255) f.assignSwitch(i, now);
      else if (now != before[i]) f.assignSwitch(i, before[i]);
      f.setSwitchBothSplits(i, both[i]);
      f.setSwitchCC(i, true, sustain[i]);
      f.setSwitchCC(i, false, cc65[i]);
    }
  });
  CHECK(waitFor([&] { return e.firmware().switchAssignment(3) == before[3] && e.firmware().switchCC(4, false) == cc65[4]; }));
}

// the sequencer's drum pages, opened from its settings, set the notes its drum rows play
void the_bridge_reads_and_sets_the_drum_notes() {
  auto& e = engine();
  auto tap = [&e](int col, int row) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  int special = e.firmware().nrpn(35);
  nrpn(e, 35, 4);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::sequencerScreen; }));
  int split = e.firmware().shownSplit();
  CHECK(e.firmware().drumNote(split, 0) == 36 && e.firmware().drumNote(split, 2) == 37 && e.firmware().drumNote(split, 9) == 50);
  tap(0, 3);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::sequencerProjectsScreen; }));
  CHECK(e.firmware().valueScreen() == firmware::Instance::ValueScreen::none);
  for (int page : { 7, 8 }) {
    tap(page, 0);
    CHECK(waitFor([&e] { return e.firmware().valueScreen() == firmware::Instance::ValueScreen::drumNotes; }));
  }
  e.onFirmware([split](firmware::Instance& f) { f.setDrumNote(split, 2, 40); f.setDrumNote(split, 3, 128); f.setDrumNote(split, 14, 41); });
  CHECK(waitFor([&e, split] { return e.firmware().drumNote(split, 2) == 40; }));
  CHECK(e.firmware().drumNote(split, 3) == 42);
  e.onFirmware([split](firmware::Instance& f) { f.setDrumNote(split, 2, 37); });
  CHECK(waitFor([&e, split] { return e.firmware().drumNote(split, 2) == 37; }));
  // the colours page beside them, whose three step colours take any colour but off and black
  tap(9, 0);
  CHECK(waitFor([&e] { return e.firmware().valueScreen() == firmware::Instance::ValueScreen::sequencerColours; }));
  auto colours = e.firmware().sequencerColours(split);
  e.onFirmware([split](firmware::Instance& f) {
    f.setSequencerColour(split, 0, 9);
    f.setSequencerColour(split, 1, 7);
    f.setSequencerColour(split, 2, 0);
    f.setSequencerColour(split, 3, 2);
    f.setSequencerColour(split, 2, 11);
  });
  CHECK(waitFor([&e, split] { return e.firmware().sequencerColours(split).beyond == 11; }));
  auto changed = e.firmware().sequencerColours(split);
  CHECK(changed.empty == 9 && changed.withNotes == colours.withNotes && e.firmware().led(5, 3).color == 9);
  e.onFirmware([split, colours](firmware::Instance& f) {
    f.setSequencerColour(split, 0, colours.empty);
    f.setSequencerColour(split, 2, colours.beyond);
  });
  CHECK(waitFor([&e, split, colours] { return e.firmware().sequencerColours(split).empty == colours.empty && e.firmware().sequencerColours(split).beyond == colours.beyond; }));
  tap(0, 3);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::sequencerScreen; }));
  nrpn(e, 35, special == firmware::Instance::nrpnUnknown ? 0 : special);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
}

// an audience message's cell held for a second edits it; set one with the characters the instrument
// can show, up to 30; one shown scrolls across the surface while the firmware animates
void the_bridge_edits_and_shows_the_audience_messages() {
  auto& e = engine();
  auto tap = [&e](int col, int row, int millis) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  // the big font's M reaches a row higher than the small font's, and only it has lower case
  auto big = e.firmware().bigGlyph('M');
  const std::vector<std::string> bigM { "     ", "0   0", "00 00", "0 0 0", "0   0", "0   0", "0   0", "     " };
  CHECK(big.rows == bigM && e.firmware().bigGlyph('m').width > 0 && e.firmware().bigGlyph('\x01').rows.empty());
  std::string before = e.firmware().audienceMessage(4);
  tap(0, 0, 150);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::globalScreen; }));
  tap(5, 7, 1300);
  CHECK(waitFor([&e] { return e.firmware().valueScreen() == firmware::Instance::ValueScreen::audienceMessage; }));
  CHECK(e.firmware().audienceMessageEdited() == 4 && e.firmware().audienceMessage(4) == before);
  e.onFirmware([](firmware::Instance& f) { f.setAudienceMessage(4, "HELLO\x01 TOKYO\xc3\xa9"); f.setAudienceMessage(6, std::string(40, 'X')); });
  CHECK(waitFor([&e] { return e.firmware().audienceMessage(4) == "HELLO TOKYO"; }));
  CHECK(e.firmware().audienceMessage(6) == std::string(30, 'X'));
  e.onFirmware([](firmware::Instance& f) { f.setAudienceMessage(6, ""); });
  e.onFirmware([](firmware::Instance& f) { f.showAudienceMessage(4); });
  CHECK(waitFor([&e] { return e.firmware().animating(); }));
  // a column every 40 ms, a few seconds for a short message, more on a busy machine
  bool ended = false;
  for (int i = 0; i < 6 && !ended; ++i) ended = waitFor([&e] { return !e.firmware().animating(); });
  CHECK(ended);
  // leaving the screen as the settings button does keeps it
  tap(0, 0, 150);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
  CHECK(e.firmware().audienceMessage(4) == "HELLO TOKYO" && e.firmware().audienceMessage(6).empty());
  e.onFirmware([before](firmware::Instance& f) { f.setAudienceMessage(4, before); });
  CHECK(waitFor([&e, before] { return e.firmware().audienceMessage(4) == before; }));
}

// pitch quantize pulls a resting finger to the middle of its pad, so it's off to see raw positions
struct NoQuantize {
  Engine& e;
  explicit NoQuantize(Engine& engine) : e(engine) {
    nrpn(e, 21, 0);
    nrpn(e, 22, 0);
  }
  ~NoQuantize() {
    nrpn(e, 21, 1);
    nrpn(e, 22, 1);
  }
};

void the_pitch_follows_the_finger_across_a_pad() {
  auto& e = engine();
  NoQuantize raw(e);
  e.takeMidiOut();
  e.press(5, 3, 0.5f, 0.5f);
  // the middle of a pad is its note, its edges half a semitone either way: 2048 at +-2
  expectNear(lastBend(drained(e)), 8192, 150, "the middle of the pad");
  for (float x = 0.5f; x <= 1.0f; x += 0.05f) e.move(5, 3, x, 0.5f), std::this_thread::sleep_for(std::chrono::milliseconds(10));
  expectNear(lastBend(drained(e)), 8192 + 2048, 300, "its right edge");
  for (float x = 1.0f; x >= 0.0f; x -= 0.05f) e.move(5, 3, x, 0.5f), std::this_thread::sleep_for(std::chrono::milliseconds(10));
  expectNear(lastBend(drained(e)), 8192 - 2048, 300, "its left edge");
  e.release(5, 3);
  drained(e);
}

void a_slide_glides_from_one_note_to_the_next() {
  auto& e = engine();
  NoQuantize raw(e);
  e.takeMidiOut();
  slideRight(e, 5, 3, true);
  auto during = drained(e);
  // no step larger than a sixth of a semitone between two readings, and it ends on the next note
  int previous = -1, biggest = 0;
  for (auto& m : during) {
    if (m.size() != 3 || (m[0] & 0xf0) != 0xe0) continue;
    int bend = m[1] | (m[2] << 7);
    if (previous >= 0) biggest = juce::jmax(biggest, std::abs(bend - previous));
    previous = bend;
  }
  expect(biggest < 4096 / 6, "the biggest step is " + juce::String(biggest));
  expectNear(lastBend(during), 8192 + 4096, 300, "the next note");
  e.release(6, 3);
  drained(e);
}

// the two cells a finger is across add up to about what the one had, however fast it goes
void a_slide_keeps_the_pressure_steady() {
  auto& e = engine();
  auto pressure = [](const std::vector<uint8_t>& m) {
    return (m[0] & 0xf0) == 0xd0 ? (int) m[1] : (m[0] & 0xf0) == 0xa0 ? (int) m[2] : -1;
  };
  auto lift = [&e](int col) {
    e.release(col, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    e.takeMidiOut();
  };
  for (int z : { 600, 900 }) {
    // the firmware reads the two cells in a different order at each crossing, some add up high and some low
    for (int col : { 6, 8 }) {
      e.press(col, 3, 0.9f, 0.5f, z);
      int steady = -1;
      for (auto& m : drained(e)) if (pressure(m) >= 0) steady = pressure(m);
      CHECK(steady > 0);
      CHECK(e.slide(col, 3, col + 1, 0.5f));
      auto during = drained(e);
      CHECK(noteOns(during) == 0);
      int low = steady, high = steady;
      for (auto& m : during) {
        if (pressure(m) < 0) continue;
        low = juce::jmin(low, pressure(m));
        high = juce::jmax(high, pressure(m));
      }
      expect(high - steady <= steady * 17 / 100 && steady - low <= steady * 22 / 100, "at " + juce::String(z) + " " + juce::String(steady) + " went " + juce::String(low) + " to " + juce::String(high));
      lift(col + 1);
    }
    // a fast swipe starts each slide before the last has finished
    e.press(5, 3, 0.9f, 0.5f, z);
    drained(e);
    for (int col = 5; col < 8; ++col) {
      CHECK(e.slide(col, 3, col + 1, 0.5f));
      // across the pad as a finger goes, the firmware takes a slide only from a cell at its edge
      for (float x : { 0.3f, 0.6f, 0.9f }) {
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
        e.move(col + 1, 3, x, 0.5f);
      }
    }
    auto swiped = drained(e);
    CHECK(noteOns(swiped) == 0);
    for (auto& m : swiped) CHECK(pressure(m) != 0);
    lift(8);
  }
  // easing off during a slide can't take what's still moving over below nothing
  e.press(8, 3, 0.9f, 0.5f, 900);
  drained(e);
  CHECK(e.slide(8, 3, 9, 0.5f));
  CHECK(e.move(9, 3, 0.1f, 0.5f, 130));
  for (int i = 0; i < 200; ++i) {
    Engine::TouchView view;
    CHECK(e.touchAt(9, 3, view) && view.pressure > 0);
    std::this_thread::sleep_for(std::chrono::microseconds(200));
  }
  lift(9);
}

void a_slide_into_the_next_cell_bends_the_same_note() {
  auto& e = engine();
  e.takeMidiOut();
  slideRight(e, 5, 3, true);
  auto during = drained(e);
  CHECK(noteOns(during) == 0);
  // a semitone further along with the default bend range of two
  CHECK(lastBend(during) > 8192 + 3000);
  // the finger has left the first cell
  CHECK(!e.isTouched(5, 3));
  e.release(6, 3);
  drained(e);
}

// a finger lifts off 6,3 and, before the firmware has seen it go, another slides in from 5,3: the
// lift still reaches the firmware, so the note on 6,3 ends
void a_slide_into_a_cell_still_lifting_ends_its_note() {
  auto& e = engine();
  e.takeMidiOut();
  e.press(6, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  // the note 6,3 plays, before 5,3 plays its own
  int lifted = -1;
  for (auto& m : e.takeMidiOut()) if (m.size() == 3 && (m[0] & 0xf0) == 0x90 && m[2] > 0) lifted = m[1];
  e.press(5, 3, 0.9f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  e.takeMidiOut();
  e.release(6, 3);
  CHECK(e.slide(5, 3, 6, 0.5f));
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  bool ended = false;
  for (auto& m : e.takeMidiOut()) {
    bool off = (m.size() == 3 && (m[0] & 0xf0) == 0x80) || (m.size() == 3 && (m[0] & 0xf0) == 0x90 && m[2] == 0);
    if (off && m[1] == lifted) ended = true;
  }
  CHECK(lifted >= 0 && ended);
  e.release(6, 3);
  e.release(5, 3);
  drained(e);
}

// a quick double tap, as a finger on glass makes it, each tap and the gap between them shorter
// than a finger takes to lift and the firmware to let go of a switch: the second tap closes the
// per-split screen the first opened
void a_quick_double_tap_on_a_switch_opens_and_closes_its_screen() {
  auto& e = engine();
  // within half a second of its start, the firmware takes a toggle for the end of a hold
  waitFor([&e] { return e.scanCount() * Engine::scanMicros(e.firmware().cellsPerScan(), 0) > 600000; });
  auto tap = [&e] {
    e.press(0, 7, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    e.release(0, 7);
  };
  tap();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  CHECK(e.firmware().showingSettings());
  tap();
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  CHECK(!e.firmware().showingSettings());
  // two taps so short and close that the second is let go of before the firmware has let go of the
  // first
  auto flick = [&e] {
    e.press(0, 7, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    e.release(0, 7);
  };
  flick();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  flick();
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  CHECK(!e.firmware().showingSettings());
  // and three of those at a fast human pace, ten a second, open it again
  for (int i = 0; i < 3; ++i) {
    flick();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  CHECK(e.firmware().showingSettings());
  tap();
  CHECK(waitFor([&e] { return !e.firmware().showingSettings(); }));
}

void a_slide_stays_among_the_pads() {
  auto& e = engine();
  // held long enough to press, so that only the column keeps them from sliding
  e.press(0, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  CHECK(!e.slide(0, 3, 1, 0.5f));
  e.release(0, 3);
  // that was the octave and transpose switch, whose screen a second tap closes
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  e.press(0, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  e.release(0, 3);
  CHECK(waitFor([&e] { return !e.firmware().showingSettings(); }));
  e.press(1, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  CHECK(!e.slide(1, 3, 0, 0.5f));
  e.release(1, 3);
  drained(e);
}

void lifting_and_pressing_the_next_cell_starts_a_new_note() {
  auto& e = engine();
  e.takeMidiOut();
  slideRight(e, 5, 3, false);
  CHECK(noteOns(drained(e)) == 1);
  e.release(6, 3);
  drained(e);
}

int lastPolyPressure(const std::vector<std::vector<uint8_t>>& messages) {
  int z = -1;
  for (auto& m : messages) if (m.size() == 3 && (m[0] & 0xf0) == 0xa0) z = m[2];
  return z;
}

void a_new_pressure_reaches_the_notes_already_held() {
  auto& e = engine();
  e.setPressure(400);
  e.takeMidiOut();
  e.press(5, 3, 0.5f, 0.5f);
  int light = lastPolyPressure(drained(e));
  e.setPressure(2500);
  int firm = lastPolyPressure(drained(e));
  e.setPressure(400);
  int lightAgain = lastPolyPressure(drained(e));
  expect(light >= 0 && firm > light + 20, "pressure " + juce::String(light) + " then " + juce::String(firm));
  expect(lightAgain < firm - 20, "and back to " + juce::String(lightAgain));
  e.release(5, 3);
  drained(e);
  e.setPressure(600);
}

// with LINN_FIRMWARE_COPIES above 1: two engines at once, each its own instrument
void two_engines_run_their_own_firmware_side_by_side() {
  if (firmware::instanceCount() < 2) return;
  auto& shared = engine();
  Engine other;
  other.flashFolder = testFlash().getChildFile("other");
  other.flashFolder.deleteRecursively();
  other.start(200);
  CHECK(&other.firmware() != &shared.firmware());
  CHECK(other.firmware().model() == 200 && shared.firmware().model() == 128);
  CHECK(other.firmware().columns() == 26 && shared.firmware().columns() == 17);
  // a setting changed on one leaves the other alone
  configureMpe(other);
  CHECK(waitFor([&other] { return other.firmware().bendRange() == 48; }));
  CHECK(shared.firmware().bendRange() == 2);
  other.stop();
  other.flashFolder.deleteRecursively();
}

void each_copy_of_the_firmware_is_handed_to_one_engine_at_a_time() {
  auto& held = engine().firmware();
  CHECK(firmware::instanceCount() >= 1);
  std::vector<firmware::Instance*> taken;
  while (auto* free = firmware::acquire()) taken.push_back(free);
  CHECK((int) taken.size() == firmware::instanceCount() - 1);
  for (auto* instance : taken) CHECK(instance != &held);
  bool refused = false;
  try {
    Engine another;
  }
  catch (const std::runtime_error&) {
    refused = true;
  }
  CHECK(refused);
  firmware::release(&held);
  CHECK(firmware::acquire() == &held);
  for (auto* instance : taken) firmware::release(instance);
}

// Per-Split and Global Settings, and neither once Per-Split Settings is closed again
// a label printed on a switch reads on its light in every colour the firmware gives it, at the
// contrast small text needs (WCAG's 4.5:1)
void a_switch_label_reads_on_every_light() {
  auto luminance = [](juce::Colour c) {
    auto part = [](juce::uint8 v) { double x = v / 255.0; return x <= 0.03928 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4); };
    return 0.2126 * part(c.getRed()) + 0.7152 * part(c.getGreen()) + 0.0722 * part(c.getBlue());
  };
  double ink = luminance(Surface::switchInk());
  for (int color : { 1, 2, 3, 4, 5, 6, 8, 9, 10, 11 }) {
    double light = luminance(Surface::ledColourFor(color));
    CHECK((juce::jmax(light, ink) + 0.05) / (juce::jmin(light, ink) + 0.05) >= 4.5);
  }
}

void the_bridge_reports_which_settings_screen_is_up() {
  auto& e = engine();
  auto tap = [&e](int row) {
    e.press(0, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(0, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  // the firmware visits one control switch every eight scans, so a switch the last test let go
  // still counts as held for a moment, and Per-Split Settings pressed with Global Settings held
  // puts the instrument to sleep
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  CHECK(!e.firmware().showingSettings());
  CHECK(e.firmware().screen() == firmware::noScreen);
  tap(7);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::perSplitScreen; }));
  tap(7);
  tap(0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::globalScreen; }));
  tap(0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
  tap(6);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::presetScreen; }));
  tap(6);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
}

// the sequencer's notes read and changed as its own touches change them: a new note in the first
// empty place and selected, the oldest let go when the step is full, a quick tap's removal moving
// the rest up, values kept in their ranges, a step cleared and copied, and the steps bar lit to match
void the_bridge_reads_and_edits_the_sequencers_notes() {
  auto& e = engine();
  auto& f = e.firmware();
  int special = f.nrpn(35);
  nrpn(e, 35, 4);
  CHECK(waitFor([&f] { return f.screen() == firmware::sequencerScreen; }));
  int s = f.sequencerShown();
  // from a known start, whatever the tests before left: stopped, Scales, nothing selected
  e.onFirmware([s](firmware::Instance& i) { i.playSequencer(s, false); i.setSequencerView(s, 1); i.selectSequencerNote(s, -1, -1); });
  CHECK(waitFor([&f, s] { auto st = f.sequencerState(s); return !st.running && st.view == 1 && st.selectedStep == -1; }));
  auto state = f.sequencerState(s);
  CHECK(s == f.shownSplit() && state.on && !state.running && state.view == 1 && state.selectedStep == -1);
  CHECK(f.sequencerPatterns() == 4 && f.sequencerSteps() == 32 && f.sequencerNotesPerStep() == 4 && f.sequencerStepsShown() == (f.model() == 200 ? 16 : 8));
  auto lengths = f.sequencerLengths();
  CHECK(lengths.size() == 17 && lengths.front() == 1 && lengths[7] == 12 && lengths.back() == 96);
  int p = state.pattern, size = f.sequencerPattern(s, p).stepSize;
  auto colours = f.sequencerColours(s);
  CHECK(f.led(3, 0).color == colours.empty);
  e.onFirmware([s](firmware::Instance& i) { i.addSequencerNote(s, 2, 60, 3, 100, 70); });
  CHECK(waitFor([&f, s, p] { return f.sequencerStep(s, p, 2)[0].used; }));
  auto notes = f.sequencerStep(s, p, 2);
  CHECK(notes[0].note == 60 && notes[0].velocity == 100 && notes[0].length == size && notes[0].timbre == 70 && notes[0].row == 3 && notes[0].fineTune == 0);
  CHECK(!notes[1].used);
  // never a second of the same note on the same row, as a touch on it would take it away instead,
  // and never on the row the steps are on
  auto asked = std::make_shared<std::atomic<bool>>(false);
  e.onFirmware([s, asked](firmware::Instance& i) { i.addSequencerNote(s, 2, 60, 3, 50, 0); i.addSequencerNote(s, 2, 61, 0, 50, 0); asked->store(true); });
  CHECK(waitFor([asked] { return asked->load(); }) && !f.sequencerStep(s, p, 2)[1].used && f.sequencerStep(s, p, 2)[0].velocity == 100);
  state = f.sequencerState(s);
  CHECK(state.selectedStep == 2 && state.selectedNote == 0 && f.sequencerNoteSelected());
  CHECK(waitFor([&f, colours] { return f.led(3, 0).color == colours.withNotes; }));
  // four more: the fifth lets the oldest go and takes the last place, selected
  e.onFirmware([s](firmware::Instance& i) {
    for (int n : { 62, 64, 65, 67 }) i.addSequencerNote(s, 2, n, 4, 300, 200);
  });
  CHECK(waitFor([&f, s, p] { return f.sequencerStep(s, p, 2)[3].note == 67; }));
  notes = f.sequencerStep(s, p, 2);
  CHECK(notes[0].note == 62 && notes[1].note == 64 && notes[2].note == 65 && notes[3].note == 67);
  CHECK(notes[3].velocity == 127 && notes[3].timbre == 127 && f.sequencerState(s).selectedNote == 3);
  // values within their ranges, and only the lengths the firmware offers
  e.onFirmware([s](firmware::Instance& i) {
    using V = firmware::Instance::NoteValue;
    i.setSequencerNoteValue(s, 2, 1, V::velocity, 0);
    i.setSequencerNoteValue(s, 2, 1, V::fineTune, -80);
    i.setSequencerNoteValue(s, 2, 1, V::timbre, 33);
    i.setSequencerNoteValue(s, 2, 1, V::length, 13);
    i.setSequencerNoteValue(s, 2, 2, V::length, 18);
  });
  CHECK(waitFor([&f, s, p] { return f.sequencerStep(s, p, 2)[2].length == 18; }));
  notes = f.sequencerStep(s, p, 2);
  CHECK(notes[1].velocity == 1 && notes[1].fineTune == -50 && notes[1].timbre == 33 && notes[1].length == size);
  // a note let go of moves the rest up; selecting none, a step, then a note
  e.onFirmware([s](firmware::Instance& i) { i.removeSequencerNote(s, 2, 1); i.selectSequencerNote(s, -1, -1); });
  CHECK(waitFor([&f, s, p] { return !f.sequencerStep(s, p, 2)[3].used; }));
  notes = f.sequencerStep(s, p, 2);
  CHECK(notes[0].note == 62 && notes[1].note == 65 && notes[2].note == 67 && f.sequencerState(s).selectedStep == -1);
  e.onFirmware([s](firmware::Instance& i) { i.selectSequencerNote(s, 2, 1); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).selectedNote == 1; }));
  // copied onto step 5, then step 2 cleared
  e.onFirmware([s](firmware::Instance& i) { i.copySequencerStep(s, 2, 5); i.clearSequencerStep(s, 2); });
  CHECK(waitFor([&f, s, p] { return !f.sequencerStep(s, p, 2)[0].used; }));
  notes = f.sequencerStep(s, p, 5);
  CHECK(notes[0].note == 62 && notes[2].note == 67 && !notes[3].used);
  CHECK(f.sequencerState(s).selectedNote == -1 && waitFor([&f, colours] { return f.led(3, 0).color == colours.empty; }));
  e.onFirmware([s](firmware::Instance& i) { i.clearSequencerStep(s, 5); i.selectSequencerNote(s, -1, -1); });
  CHECK(waitFor([&f, s, p] { return !f.sequencerStep(s, p, 5)[0].used; }));
  // a note sounded for a moment as a picked note sounds, its own end after it, and none for a note
  // out of range
  drained(e);
  // a split transposed a fifth up, which the preview leaves out, as the note picked is the note heard
  nrpn(e, 37 + 100 * s, 14);
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  e.onFirmware([s](firmware::Instance& i) { i.previewSequencerNote(s, 38, 96); i.previewSequencerNote(s, 128, 96); });
  std::vector<std::vector<uint8_t>> heard;
  int on = -1, off = -1;
  auto judge = [&] {
    on = off = -1;
    for (size_t m = 0; m < heard.size(); ++m) {
      auto& msg = heard[m];
      if (msg.size() != 3) continue;
      if ((msg[0] & 0xf0) == 0x90 && msg[2] > 0) on = on < 0 && msg[1] == 38 && msg[2] == 96 ? (int) m : 1000;
      else if (((msg[0] & 0xf0) == 0x80 || (msg[0] & 0xf0) == 0x90) && msg[1] == 38) off = (int) m;
    }
  };
  waitFor([&] {
    for (auto& msg : e.takeMidiOut()) heard.push_back(msg);
    judge();
    return off > on && on >= 0;
  });
  // whatever the queue still held behind them
  for (auto& msg : drained(e)) heard.push_back(msg);
  judge();
  nrpn(e, 37 + 100 * s, 7);
  // a pad above the highest note has none: rows sixteen semitones apart, the octave's own offset
  // replaced by a custom one
  int rowOffset = f.nrpn(227), custom = f.nrpn(253);
  nrpn(e, 253, 32);
  nrpn(e, 227, 12);
  CHECK(waitFor([&f] { return f.nrpn(227) == 12 && f.nrpn(253) == 32; }));
  CHECK(f.padNote(s, 1, 7) == -1 && f.padNote(s, 1, 0) >= 0);
  nrpn(e, 227, rowOffset);
  nrpn(e, 253, custom);
  CHECK(waitFor([&f, rowOffset, custom] { return f.nrpn(227) == rowOffset && f.nrpn(253) == custom; }));
  CHECK(on >= 0 && on < 1000 && off > on);
  nrpn(e, 35, special == firmware::Instance::nrpnUnknown ? 0 : special);
  CHECK(waitFor([&f] { return f.screen() == firmware::noScreen; }));
}

// the patterns and their settings as the pattern selector, the step size cells, the navigation and
// the muter set them, a pattern's data kept and given back, and the rows' notes per view
void the_bridge_sets_the_sequencers_patterns_and_settings() {
  auto& e = engine();
  auto& f = e.firmware();
  int special = f.nrpn(35);
  nrpn(e, 35, 4);
  CHECK(waitFor([&f] { return f.screen() == firmware::sequencerScreen; }));
  int s = f.sequencerShown(), first = f.sequencerState(s).pattern, other = (first + 1) % 4;
  auto kept = f.sequencerPatternData(s, other);
  CHECK(!kept.empty());
  // scales from the scale's notes, drums from the drum notes
  CHECK(f.sequencerRowNote(s, 0) == 48 && f.sequencerRowNote(s, 1) == 50 && f.sequencerRowNote(s, 7) == 60 && f.sequencerRowNote(s, 14) == -1);
  e.onFirmware([s](firmware::Instance& i) { i.setSequencerView(s, 2); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).view == 2; }));
  CHECK(f.sequencerRowNote(s, 0) == f.drumNote(s, 0) && f.sequencerRowNote(s, 6) == f.drumNote(s, 6));
  e.onFirmware([s](firmware::Instance& i) { i.showSequencerPage(s, 0, 7); });
  // the rows counted from the bottom of all of them whichever are shown
  CHECK(waitFor([&f, s] { return f.sequencerState(s).rowOffset == 7; }) && f.sequencerRowNote(s, 0) == f.drumNote(s, 0) && f.sequencerRowNote(s, 13) == f.drumNote(s, 13));
  // the notes view selects the first step, as the view cells do
  e.onFirmware([s](firmware::Instance& i) { i.showSequencerPage(s, 0, 0); i.setSequencerView(s, 0); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).view == 0; }) && f.sequencerState(s).selectedStep == 0 && f.sequencerRowNote(s, 0) == -1);
  e.onFirmware([s](firmware::Instance& i) { i.setSequencerView(s, 1); i.selectSequencerNote(s, -1, -1); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).view == 1; }));
  // the second page, only at a page's first step
  int shown = f.sequencerStepsShown();
  // only at a page's first step
  auto settled = std::make_shared<std::atomic<bool>>(false);
  e.onFirmware([s, shown, settled](firmware::Instance& i) { i.showSequencerPage(s, shown + 1, 0); settled->store(true); });
  CHECK(waitFor([settled] { return settled->load(); }) && f.sequencerState(s).pageStart == 0);
  e.onFirmware([s, shown](firmware::Instance& i) { i.showSequencerPage(s, shown, 0); });
  CHECK(waitFor([&f, s, shown] { return f.sequencerState(s).pageStart == shown; }));
  e.onFirmware([s](firmware::Instance& i) { i.showSequencerPage(s, 0, 0); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).pageStart == 0; }));
  // a note on the other pattern, chosen while stopped at once, copied to the other split, cleared
  e.onFirmware([s, other](firmware::Instance& i) { i.selectSequencerPattern(s, other); i.addSequencerNote(s, 0, 50, 1, 90, 0); });
  CHECK(waitFor([&f, s, other] { return f.sequencerState(s).pattern == other && f.sequencerStep(s, other, 0)[0].used; }));
  auto otherKept = f.sequencerPatternData(1 - s, 3);
  e.onFirmware([s, other](firmware::Instance& i) { i.copySequencerPattern(s, other, 1 - s, 3); });
  CHECK(waitFor([&f, s] { return f.sequencerStep(1 - s, 3, 0)[0].note == 50; }));
  // its settings: step size with swing, a dotted size without, direction, loop and length
  e.onFirmware([s](firmware::Instance& i) {
    i.setSequencerStepSize(s, 6, true);
    i.setSequencerDirection(s, 2);
    i.setSequencerLoopPage(s, true);
    i.setSequencerLength(s, 5);
    i.setSequencerLength(s, 33);
    i.setSequencerStepSize(s, 7, false);
  });
  CHECK(waitFor([&f, s, other] { return f.sequencerPattern(s, other).length == 5; }));
  auto settings = f.sequencerPattern(s, other);
  CHECK(settings.stepSize == 6 && settings.swing && settings.direction == 2 && settings.loopPage);
  e.onFirmware([s](firmware::Instance& i) { i.setSequencerStepSize(s, 18, true); });
  CHECK(waitFor([&f, s, other] { return f.sequencerPattern(s, other).stepSize == 18; }) && !f.sequencerPattern(s, other).swing);
  // muted and running, as the muter and Play/Stop do, and a pattern chosen while running waits
  e.onFirmware([s](firmware::Instance& i) { i.muteSequencer(s, true); i.playSequencer(s, true); });
  // once its first step has played, before which a pattern chosen takes over at once
  CHECK(waitFor([&f, s] { return f.sequencerState(s).running && f.sequencerState(s).muted && f.sequencerState(s).position >= 0; }));
  e.onFirmware([s, first](firmware::Instance& i) { i.selectSequencerPattern(s, first); });
  CHECK(waitFor([&f, s, first] { return f.sequencerState(s).nextPattern == first; }) && f.sequencerState(s).pattern == other);
  // where it plays next, as Switch 2 held with a step moves it, going forward
  // the step it plays next is the one moved to, not the one after where it was
  auto from = std::make_shared<std::atomic<int>>(-2);
  e.onFirmware([s, from](firmware::Instance& i) {
    i.setSequencerDirection(s, 0);
    int at = i.sequencerState(s).position, to = (at + 3) % 5;
    i.moveSequencerPlayhead(s, to);
    from->store(at * 10 + to);
  });
  CHECK(waitFor([from] { return from->load() >= 0; }));
  int was = from->load() / 10, to = from->load() % 10;
  CHECK(waitFor([&f, s, was] { return f.sequencerState(s).position != was; }) && f.sequencerState(s).position == to && f.sequencerState(s).forward);
  e.onFirmware([s](firmware::Instance& i) { i.playSequencer(s, false); i.muteSequencer(s, false); });
  CHECK(waitFor([&f, s] { return !f.sequencerState(s).running && !f.sequencerState(s).muted; }));
  // asked for what it already is, it stays so
  auto asked = std::make_shared<std::atomic<bool>>(false);
  e.onFirmware([s, asked](firmware::Instance& i) { i.muteSequencer(s, false); i.playSequencer(s, false); asked->store(true); });
  CHECK(waitFor([asked] { return asked->load(); }) && !f.sequencerState(s).muted && !f.sequencerState(s).running);
  // the data given back as it was, and the copy cleared
  e.onFirmware([s, other, kept](firmware::Instance& i) { i.setSequencerPatternData(s, other, kept); i.clearSequencerPattern(1 - s, 3); });
  CHECK(waitFor([&f, s, other, kept] { return f.sequencerPatternData(s, other) == kept; }));
  CHECK(waitFor([&f, s] { return !f.sequencerStep(1 - s, 3, 0)[0].used; }));
  e.onFirmware([s, otherKept](firmware::Instance& i) { i.setSequencerPatternData(1 - s, 3, otherKept); });
  CHECK(waitFor([&f, s, otherKept] { return f.sequencerPatternData(1 - s, 3) == otherKept; }));
  e.onFirmware([s, first](firmware::Instance& i) { i.selectSequencerPattern(s, first); });
  CHECK(waitFor([&f, s, first] { return f.sequencerState(s).pattern == first; }));
  // the other split shown, as a tap on SPLIT shows it, which has no sequencer on, then this one again
  e.onFirmware([s](firmware::Instance& i) { i.showSequencerSplit(1 - s); });
  CHECK(waitFor([&f, s] { return f.shownSplit() == 1 - s; }) && f.sequencerShown() == -1);
  e.onFirmware([s](firmware::Instance& i) { i.showSequencerSplit(s); });
  CHECK(waitFor([&f, s] { return f.sequencerShown() == s; }));
  nrpn(e, 35, special == firmware::Instance::nrpnUnknown ? 0 : special);
  CHECK(waitFor([&f] { return f.screen() == firmware::noScreen; }));
}

// a tap through the bridge timed as the Tap Tempo switch times it, the tempo shown at once on Global
// Settings, as the switch shows it
void the_bridge_taps_the_tempo_as_its_switch_does() {
  auto& e = engine();
  auto& f = e.firmware();
  int before = f.tempo();
  auto tapCell = [&e](int row) {
    e.press(0, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(0, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  tapCell(0);
  CHECK(waitFor([&f] { return f.screen() == firmware::globalScreen; }));
  auto lights = [&f] {
    std::vector<int> all;
    for (int col = 0; col < f.columns(); ++col)
      for (int row = 0; row < 8; ++row) all.push_back(f.led(col, row).color * 16 + f.led(col, row).display);
    return all;
  };
  e.onFirmware([](firmware::Instance& i) { i.setNrpn(238, 60); });
  CHECK(waitFor([&f] { return f.tempo() == 60; }));
  auto shown = lights();
  for (int k = 0; k < 3; ++k) {
    e.onFirmware([](firmware::Instance& i) { i.tapTempo(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
  CHECK(f.tempo() > 80 && lights() != shown);
  e.onFirmware([before](firmware::Instance& i) { i.setNrpn(238, before); });
  CHECK(waitFor([&f, before] { return f.tempo() == before; }));
  tapCell(0);
  CHECK(waitFor([&f] { return f.screen() == firmware::noScreen; }));
}

// the projects in the flash as the projects screen loads and saves them: the one played saved with
// its tempo, loaded back with both sequencers stopped, another project stored in a slot with its
// own tempo leaving the one played alone, and the one played given back with the slot it came from
void the_bridge_loads_saves_and_stores_projects() {
  auto& e = engine();
  auto& f = e.firmware();
  int special = f.nrpn(35);
  nrpn(e, 35, 4);
  CHECK(waitFor([&f] { return f.screen() == firmware::sequencerScreen; }));
  // none loaded, whatever an earlier test left
  e.onFirmware([](firmware::Instance& i) { i.restoreSequencerProject(i.currentProject(), -1); });
  CHECK(waitFor([&f] { return f.loadedProject() == -1; }));
  int s = f.sequencerShown(), loaded = f.loadedProject(), tempo = f.tempo();
  CHECK(f.sequencerProjects() == 16);
  auto kept5 = f.storedProject(5), kept6 = f.storedProject(6);
  CHECK(!kept5.empty());
  // every slot as the firmware keeps it taken, never bytes it couldn't have stored
  bool slotsValid = true;
  for (int slot = 0; slot < 16; ++slot) slotsValid = slotsValid && f.sequencerProjectValid(f.storedProject(slot));
  CHECK(slotsValid && f.sequencerProjectValid(f.currentProject()) && !f.sequencerProjectValid({ 1, 2, 3 }));
  // on an empty pattern, whatever the tests before left on it
  e.onFirmware([s](firmware::Instance& i) { i.clearSequencerPattern(s, i.sequencerState(s).pattern); i.addSequencerNote(s, 0, 60, 1, 99, 0); i.setNrpn(238, 140); });
  CHECK(waitFor([&f] { return f.tempo() == 140; }));
  auto played = f.currentProject();
  CHECK(f.projectTempo(played) == 140 && f.projectStep(played, s, f.sequencerState(s).pattern, 0)[0].note == 60);
  CHECK(f.projectPattern(played, s, 0).length == f.sequencerPattern(s, 0).length);
  // a project with a pattern longer than the firmware's steps, a step size it doesn't have, a note
  // on the steps' row or no tempo refused, the one played and the slot left as they were
  {
    size_t patternSize = f.sequencerPatternData(0, 0).size(), steps = (size_t) (f.sequencerSteps() * f.sequencerNotesPerStep() * 6);
    size_t pattern = (size_t) s * (played.size() / 2 & ~(size_t) 3) + (size_t) f.sequencerState(s).pattern * patternSize;
    auto bad = [&](std::function<void(std::vector<uint8_t>&)> spoil, std::function<bool(const std::vector<uint8_t>&)> spoilt) {
      auto blob = played;
      spoil(blob);
      CHECK(spoilt(blob) && !f.sequencerProjectValid(blob));
      return blob;
    };
    auto longer = bad([&](std::vector<uint8_t>& b) { b[pattern + steps + 10] = 40; }, [&](const std::vector<uint8_t>& b) { return f.projectPattern(b, s, f.sequencerState(s).pattern).length == 40; });
    auto sizeless = bad([&](std::vector<uint8_t>& b) { b[pattern + steps] = 0; }, [&](const std::vector<uint8_t>& b) { return f.projectPattern(b, s, f.sequencerState(s).pattern).stepSize == 0; });
    auto lowRow = bad([&](std::vector<uint8_t>& b) { b[pattern] &= 0xfe; b[pattern + 1] = 0; b[pattern + 2] &= 0x7f; }, [&](const std::vector<uint8_t>& b) { return f.projectStep(b, s, f.sequencerState(s).pattern, 0)[0].length == 0; });
    // a note on the low row taken, which Notes view's painting can move it to
    auto onLowRow = played;
    onLowRow[pattern + 4] &= 0xfe;
    onLowRow[pattern + 5] &= 0x3f;
    CHECK(f.projectStep(onLowRow, s, f.sequencerState(s).pattern, 0)[0].row == 0 && f.sequencerProjectValid(onLowRow));
    // the tempo's two bytes, wherever the padding puts them near the end
    size_t at = played.size() - 2;
    while (at > played.size() - 8 && !(played[at] == 140 && played[at + 1] == 0)) --at;
    auto still = bad([&](std::vector<uint8_t>& b) { b[at] = b[at + 1] = 0; }, [&](const std::vector<uint8_t>& b) { return f.projectTempo(b) == 0; });
    // a tempo taps or a MIDI clock took past the settings' 360 taken
    auto fast = played;
    fast[at] = (uint8_t) (365 & 0xff);
    fast[at + 1] = (uint8_t) (365 >> 8);
    CHECK(f.projectTempo(fast) == 365 && f.sequencerProjectValid(fast));
    // the left split's first drum note, after its four patterns
    bad([&](std::vector<uint8_t>& b) { b[4 * patternSize] = 200; }, [](const std::vector<uint8_t>&) { return true; });
    auto data = f.sequencerPatternData(s, f.sequencerState(s).pattern), longPattern = data;
    longPattern[steps + 10] = 40;
    auto kept0 = f.storedProject(0);
    e.onFirmware([longer, sizeless, lowRow, still, longPattern, s](firmware::Instance& i) {
      i.storeSequencerProject(0, longer);
      i.restoreSequencerProject(sizeless, 3);
      i.restoreSequencerProject(lowRow, 3);
      i.restoreSequencerProject(still, 3);
      i.setSequencerPatternData(s, i.sequencerState(s).pattern, longPattern);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(f.storedProject(0) == kept0 && f.currentProject() == played && f.loadedProject() == -1 && f.sequencerPatternData(s, f.sequencerState(s).pattern) == data);
  }
  // saved, then changed, then loaded back
  e.onFirmware([](firmware::Instance& i) { i.saveSequencerProject(5); });
  CHECK(waitFor([&f, played] { return f.storedProject(5) == played; }) && f.loadedProject() == loaded);
  e.onFirmware([s](firmware::Instance& i) { i.clearSequencerPattern(s, i.sequencerState(s).pattern); i.setNrpn(238, 100); i.playSequencer(s, true); });
  CHECK(waitFor([&f, s] { return f.tempo() == 100 && f.sequencerState(s).running; }));
  auto other = f.currentProject();
  e.onFirmware([](firmware::Instance& i) { i.loadSequencerProject(5); });
  CHECK(waitFor([&f] { return f.loadedProject() == 5 && f.tempo() == 140; }));
  CHECK(f.settingsVersion() == 17);
  CHECK(f.currentProject() == played && !f.sequencerState(s).running);
  // another project stored with its own tempo, the one played left as it was
  e.onFirmware([other](firmware::Instance& i) { i.storeSequencerProject(6, other); });
  CHECK(waitFor([&f, other] { return f.storedProject(6) == other; }) && f.projectTempo(f.storedProject(6)) == 100);
  CHECK(f.currentProject() == played && f.tempo() == 140);
  // a note selected let go of where the project loaded has none in its place, so that the step's
  // faders never make one of nothing
  int shownPattern = f.sequencerState(s).pattern;
  e.onFirmware([s](firmware::Instance& i) { i.selectSequencerNote(s, 0, 0); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).selectedNote == 0; }));
  e.onFirmware([](firmware::Instance& i) { i.loadSequencerProject(6); });
  CHECK(waitFor([&f] { return f.loadedProject() == 6; }) && f.sequencerState(s).selectedNote == -1 && f.sequencerState(s).selectedStep == -1 && f.sequencerState(s).pattern == shownPattern);
  e.onFirmware([played](firmware::Instance& i) { i.restoreSequencerProject(played, 5); });
  CHECK(waitFor([&f, played] { return f.currentProject() == played; }));
  e.onFirmware([s](firmware::Instance& i) { i.selectSequencerNote(s, 0, 0); });
  CHECK(waitFor([&f, s] { return f.sequencerState(s).selectedNote == 0; }));
  // the one played before the load given back, with the slot it came from
  e.onFirmware([other, loaded](firmware::Instance& i) { i.restoreSequencerProject(other, loaded); });
  CHECK(waitFor([&f, other] { return f.currentProject() == other; }) && f.loadedProject() == loaded && f.tempo() == 100 && f.sequencerState(s).selectedNote == -1 && f.sequencerState(s).selectedStep == -1);
  // the slots and the tempo as they were
  e.onFirmware([kept5, kept6, tempo](firmware::Instance& i) { i.storeSequencerProject(5, kept5); i.storeSequencerProject(6, kept6); i.setNrpn(238, tempo); });
  CHECK(waitFor([&f, kept5, kept6] { return f.storedProject(5) == kept5 && f.storedProject(6) == kept6; }));
  nrpn(e, 35, special == firmware::Instance::nrpnUnknown ? 0 : special);
  CHECK(waitFor([&f] { return f.screen() == firmware::noScreen; }));
}

// the sequencer has no screen of its own in the firmware: it takes over the playing display
void the_bridge_reports_the_sequencer_and_its_settings() {
  auto& e = engine();
  auto tap = [&e](int col, int row) {
    e.press(col, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  // Per-Split Settings, then SEQUENCER in the Special column, then back to the playing display
  tap(0, 7);
  tap(14, 4);
  tap(0, 7);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::sequencerScreen; }));
  CHECK(!e.firmware().showingSettings());
  CHECK(!e.firmware().sequencerNoteSelected());
  // a note put on the grid is selected, for its faders
  tap(1, 3);
  CHECK(waitFor([&e] { return e.firmware().sequencerNoteSelected(); }));
  tap(9, 3);
  CHECK(waitFor([&e] { return !e.firmware().sequencerNoteSelected(); }));
  // Switch 1 opens the sequencer's settings on its projects page
  tap(0, 3);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::sequencerProjectsScreen; }));
  tap(0, 3);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::sequencerScreen; }));
  // the note taken out again and the sequencer off, for the tests that follow
  tap(1, 3);
  tap(0, 7);
  tap(14, 4);
  tap(0, 7);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
}

// a screen's touches land where the default calibration puts them, so every unit counts as
// calibrated, its CALIBRATE cell in Global Settings lit green
void every_unit_is_calibrated() {
  auto& e = engine();
  CHECK(e.firmware().calibrated());
  // the shared engine is a 128, whose default calibration a later boot would otherwise reject
  CHECK(e.firmware().calibrationHolds());
  auto tap = [&e](int row) {
    e.press(0, row, 0.5f, 0.5f);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    e.release(0, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  };
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  tap(0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::globalScreen; }));
  CHECK(e.firmware().led(16, 3).color == 3);
  tap(0);
  CHECK(waitFor([&e] { return e.firmware().screen() == firmware::noScreen; }));
}

// the name as each model's panel prints it, with its rule and line under it, far longer than high
void the_nameplate_is_the_panels() {
  for (int model : { 128, 200 }) {
    auto plate = Surface::nameplate(model), alone = Surface::name(model);
    CHECK(plate.isValid() && alone.isValid());
    CHECK(plate.getWidth() > 6 * plate.getHeight());
    // the name alone is as wide and only as high as the name
    CHECK(alone.getWidth() == plate.getWidth());
    CHECK(alone.getHeight() < plate.getHeight() * 2 / 3);
  }
  // the 128's name carries its number, so it's longer for its height
  auto aspect = [](const juce::Image& image) { return (float) image.getWidth() / (float) image.getHeight(); };
  CHECK(aspect(Surface::name(128)) > aspect(Surface::name(200)) * 1.15f);
  CHECK(aspect(Surface::nameplate(128)) > aspect(Surface::nameplate(200)) * 1.1f);
}

void the_engines_ports_are_listed_by_their_identifiers() {
  auto& e = engine();
  if (!e.hasVirtualPorts()) return;
  auto has = [&](const juce::Array<juce::MidiDeviceInfo>& ports, const juce::String& id) {
    return std::any_of(ports.begin(), ports.end(), [&](auto& port) { return port.identifier == id && port.name == e.portName(); });
  };
  // JUCE lists a new port once CoreMIDI tells the main thread's run loop about it
  bool listed = false;
  for (int tries = 0; tries < 100 && !listed; ++tries) {
    listed = has(juce::MidiInput::getAvailableDevices(), e.outputIdentifier()) && has(juce::MidiOutput::getAvailableDevices(), e.inputIdentifier());
#if JUCE_MAC
    if (!listed) CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, true);
#else
    if (!listed) std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
  }
  CHECK(listed);
}

void painted_bold_text_is_mulish_bold() {
  auto* lf = dynamic_cast<LinnLookAndFeel*>(&juce::LookAndFeel::getDefaultLookAndFeel());
  CHECK(lf != nullptr);
  CHECK(LinnLookAndFeel::boldFont(13).getTypefacePtr() == lf->bold);
  CHECK(Surface::labelFontOfHeight(13).getTypefacePtr() == lf->bold);
}

void the_bridge_reports_the_bend_range() {
  engine();
  CHECK(engine().firmware().bendRange() == 2);
}

// NRPN 230 assigns the left foot switch, 231 the right
void the_bridge_reports_which_foot_switches_latch() {
  auto& e = engine();
  // the defaults: the arpeggiator on the left and sustain on the right
  CHECK(e.firmware().footSwitchLatches(0) && e.firmware().footSwitchLatches(1));
  // octave up, which acts once
  nrpn(e, 230, 1);
  CHECK(waitFor([&] { return !e.firmware().footSwitchLatches(0); }));
  CHECK(e.firmware().footSwitchLatches(1));
  // back to the arpeggiator, for the tests after
  nrpn(e, 230, 4);
  CHECK(waitFor([&] { return e.firmware().footSwitchLatches(0); }));
}

// NRPN 238 sets the tempo, as an editor would
void a_note_sent_on_a_splits_note_channel_lights_its_pads() {
  auto& e = engine();
  auto lights = [&e] {
    std::vector<int> colours;
    for (int col = 1; col < e.columnCount(); ++col)
      for (int row = 0; row < 8; ++row) colours.push_back(e.firmware().led(col, row).color);
    return colours;
  };
  int reference = -1;
  auto rowsLit = [&](const std::vector<int>& before) {
    std::set<int> rows;
    auto now = lights();
    for (size_t i = 0; i < now.size(); ++i) {
      if (now[i] != before[i]) rows.insert((int) i % 8);
    }
    return (int) rows.size();
  };
  // ending on one channel, the default, for the tests after
  for (int mode : { 1, 2, 0 }) {
    // the left split's MIDI mode, NRPN 0
    uint8_t set[] = { 0xB0, 99, 0, 0xB0, 98, 0, 0xB0, 6, 0, 0xB0, 38, (uint8_t) mode, 0xB0, 101, 127, 0xB0, 100, 127 };
    e.sendMidi(set, sizeof(set));
    drained(e);
    auto before = lights();
    auto channels = e.firmware().noteChannels(0);
    CHECK(!channels.empty());
    // every channel the split plays on, its main one first, then its per-note or per-row ones, each
    // once: the left split's defaults, main channel 1, per-note channels 2 to 8 and rows from channel
    // 1, which no NRPN query answers
    std::vector<int> expected = mode == 1 ? std::vector<int> { 1, 2, 3, 4, 5, 6, 7, 8 }
                              : mode == 2 ? std::vector<int> { 1, 2, 3, 4, 5, 6, 7, 8 } : std::vector<int> { 1 };
    auto midi = e.firmware().splitMidi(0);
    CHECK(midi.channels() == expected && midi.mode == mode && midi.main == 1 && midi.mainSends && midi.bendRange == e.firmware().bendRangeOf(0));
    for (int channel : channels) {
      uint8_t on[] = { (uint8_t) (0x90 | (channel - 1)), 60, 100 };
      e.sendMidi(on, sizeof(on));
    }
    // as many rows as channel per note lights, where it's every possible pad
    CHECK(waitFor([&] { return rowsLit(before) >= juce::jmax(2, reference); }));
    if (reference < 0) reference = rowsLit(before);
    for (int channel : channels) {
      uint8_t off[] = { (uint8_t) (0x80 | (channel - 1)), 60, 0 };
      e.sendMidi(off, sizeof(off));
    }
    CHECK(waitFor([&] { return lights() == before; }));
  }
  // whether split is on, as NRPN 200 sets it
  CHECK(!e.firmware().splitMidi(0).splitOn);
  nrpn(e, 200, 1);
  CHECK(waitFor([&e] { return e.firmware().splitMidi(0).splitOn && e.firmware().splitMidi(1).splitOn; }));
  nrpn(e, 200, 0);
  CHECK(waitFor([&e] { return !e.firmware().splitMidi(0).splitOn; }));
}

// the velocity of a press on a cell, struck that hard with that pressure to rise to, the cell
// held so that the next one can be pressed at once
int velocityOf(Engine& e, int col, int row, int strength, int pressure) {
  e.setStrike(strength);
  e.setPressure(pressure);
  e.takeMidiOut();
  e.press(col, row, 0.5f, 0.5f);
  for (int i = 0; i < 200; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    for (auto& m : e.takeMidiOut()) if (m.size() == 3 && (m[0] & 0xf0) == 0x90 && m[2] > 0) return m[2];
  }
  return -1;
}

void a_strike_sets_the_velocity_whatever_the_pressure() {
  auto& e = engine();
  std::vector<int> light, firm;
  int col = 2;
  for (int strength : { 1, 4, 8, 12 }) {
    light.push_back(velocityOf(e, col++, 3, strength, 300));
    firm.push_back(velocityOf(e, col++, 3, strength, 1500));
  }
  for (int c = 2; c < col; ++c) e.release(c, 3);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  e.setStrike(10);
  e.setPressure(600);
  // from the softest a touch can be to the hardest velocity there is, in even steps
  CHECK(light == std::vector<int>({ 20, 46, 87, 127 }));
  CHECK(firm == light);
}

void every_cell_strikes_alike() {
  auto& e = engine();
  int last = e.columnCount() - 1;
  std::vector<std::pair<int, int>> cells { { 1, 0 }, { last, 0 }, { last / 2, 4 }, { 1, 7 }, { last, 7 } };
  // one at a time, since the firmware takes the fourth corner of a rectangle for a phantom
  std::vector<int> velocities;
  for (auto [col, row] : cells) {
    velocities.push_back(velocityOf(e, col, row, 6, 600));
    e.release(col, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
  }
  e.setStrike(10);
  for (int v : velocities) CHECK(v == velocities.front() && v > 0);
}

void letting_go_lowers_the_pressure_before_the_note_ends() {
  auto& e = engine();
  e.setPressure(1500);
  e.press(5, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  drained(e);
  e.release(5, 3);
  auto lifted = juce::Time::getMillisecondCounterHiRes();
  std::vector<int> pressures;
  double ended = -1;
  for (int i = 0; i < 100 && ended < 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    for (auto& m : e.takeMidiOut()) {
      int status = m[0] & 0xf0;
      if (m.size() == 3 && status == 0xa0) pressures.push_back(m[2]);
      if (m.size() == 2 && status == 0xd0) pressures.push_back(m[1]);
      if (m.size() == 3 && (status == 0x80 || (status == 0x90 && m[2] == 0))) ended = juce::Time::getMillisecondCounterHiRes();
    }
  }
  e.setPressure(600);
  CHECK(ended - lifted >= 25);
  // the firmware sends fewer pressures than scans, and a 0 just before the note off
  CHECK(pressures.size() >= 3 && pressures.back() == 0);
  for (size_t i = 1; i < pressures.size(); ++i) CHECK(pressures[i] < pressures[i - 1]);
}

void a_soft_press_rises_slowly_and_a_hard_one_at_once() {
  auto& e = engine();
  Engine::TouchView view;
  // a soft press is still well short of its pressure after 150 ms, however late a busy machine wakes
  // this test up
  e.setPressure(600);
  e.setStrike(1);
  e.press(5, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  double landed = Engine::strikeLevel(1);
  CHECK(e.touchAt(5, 3, view) && (view.pressure * 3000.0 - landed) / (600 - landed) < 0.6);
  e.release(5, 3);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  // and a higher pressure is reached sooner, which the rate sets whatever the clock does
  auto millisTo = [](double pressure) { return pressure / Engine::riseRate(1, pressure); };
  CHECK(millisTo(600) > 600 && millisTo(2400) < millisTo(600) / 1.5);
  e.setPressure(1500);
  e.setStrike(12);
  e.press(5, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  CHECK(e.touchAt(5, 3, view) && std::abs(view.pressure - 0.5f) < 0.02f);
  e.release(5, 3);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  e.setStrike(10);
  e.setPressure(600);
}

void lifted_sensor_limits_let_a_fourth_touch_into_a_column() {
  auto& e = engine();
  for (bool lifted : { false, true }) {
    e.firmware().liftSensorLimits(lifted);
    e.takeMidiOut();
    for (int row = 1; row <= 4; ++row) e.press(5, row, 0.5f, 0.5f);
    int sounding = 0;
    for (int i = 0; i < 50 && sounding < 4; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
      sounding += noteOns(e.takeMidiOut());
    }
    for (int row = 1; row <= 4; ++row) e.release(5, row);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    CHECK(sounding == (lifted ? 4 : 3));
  }
  e.firmware().liftSensorLimits(false);
}

void the_bridge_reports_the_tempo() {
  auto& e = engine();
  auto set = [&e](int bpm) {
    uint8_t m[] = { 0xB0, 99, 1, 0xB0, 98, 238 - 128, 0xB0, 6, (uint8_t) (bpm >> 7), 0xB0, 38, (uint8_t) (bpm & 127),
                    0xB0, 101, 127, 0xB0, 100, 127 };
    e.sendMidi(m, sizeof(m));
  };
  set(97);
  CHECK(waitFor([&e] { return e.firmware().tempo() == 97; }));
  set(120);
  CHECK(waitFor([&e] { return e.firmware().tempo() == 120; }));
}

void a_fresh_flash_starts_with_the_settings_given_and_a_saved_one_without() {
  auto& e = engine();
  int calls = 0;
  e.onFreshFlash = [&e, &calls] {
    ++calls;
    configureMpe(e);
  };
  e.stop();
  testFlash().deleteRecursively();
  e.start(128);
  CHECK(calls == 1);
  CHECK(waitFor([&e] { return e.firmware().bendRange() == 48; }));
  // stopping waits for the MIDI the MPE settings queued to go out, and no longer
  auto stopping = juce::Time::getMillisecondCounterHiRes();
  e.stop();
  CHECK(juce::Time::getMillisecondCounterHiRes() - stopping < 900);
  // a flash saved before is no fresh flash
  testFlash().createDirectory();
  auto image = e.firmware().flashImage();
  testFlash().getChildFile("flash-128.bin").replaceWithData(image.data(), image.size());
  e.start(128);
  CHECK(calls == 1);
  e.onFreshFlash = nullptr;
  e.stop();
  testFlash().deleteRecursively();
  e.start(128);
  CHECK(e.firmware().bendRange() == 2);
  // nothing the last boot queued for MPE comes out after this one, which sends no RPNs
  e.takeMidiOut();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (auto& m : e.takeMidiOut()) CHECK(!(m.size() == 3 && (m[0] & 0xf0) == 0xb0 && (m[1] == 100 || m[1] == 101)));
}

void an_engine_without_ports_opens_none_and_keeps_its_flash_out_of_files() {
  auto& e = engine();
  e.stop();
  testFlash().deleteRecursively();
  // the app's flash, which an engine without a file never boots from
  e.start(128);
  nrpn(e, 19, 12);
  const uint8_t keep[] = { 0xb0, 23, 0 };
  e.sendMidi(keep, sizeof(keep));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  e.stop();
  juce::MemoryBlock appFlash;
  CHECK(testFlash().getChildFile("flash-128.bin").loadFileAsData(appFlash));
  e.virtualPorts = false;
  e.flashInFile = false;
  e.bootFlash.clear();
  int fresh = 0;
  e.onFreshFlash = [&e, &fresh] {
    ++fresh;
    configureMpe(e);
  };
  e.start(128);
  CHECK(!e.hasVirtualPorts());
  CHECK(!e.status().contains("MIDI port"));
  CHECK(fresh == 1);
  CHECK(waitFor([&e] { return e.firmware().bendRange() == 48; }));
  // nothing stored yet, so a fresh flash stays fresh, its starting settings given again
  CHECK(e.currentFlash().empty());
  e.stop();
  CHECK(e.bootFlash.empty());
  e.start(128);
  CHECK(fresh == 2);
  // a change stored, as saving a light pattern stores everything, is kept, read while it runs
  nrpn(e, 19, 24);
  const uint8_t store[] = { 0xb0, 23, 0 };
  e.sendMidi(store, sizeof(store));
  // read once the firmware has taken the store in, which a busy machine's engine may not have yet:
  // it answers a query sent after the store only once it has
  e.takeMidiOut();
  nrpn(e, 299, 19);
  bool answered = false;
  CHECK(waitFor([&] {
    for (auto& m : e.takeMidiOut()) answered = answered || (m.size() == 3 && (m[0] & 0xf0) == 0xb0 && m[1] == 98 && m[2] == 19);
    return answered;
  }));
  CHECK(e.firmware().bendRange() == 24);
  std::vector<uint8_t> running = e.currentFlash();
  CHECK(running.size() == firmware::flashSize());
  e.stop();
  CHECK(e.bootFlash == running);
  CHECK(e.currentFlash() == running);
  e.start(128);
  CHECK(fresh == 2);
  CHECK(e.firmware().bendRange() == 24);
  e.stop();
  CHECK(e.bootFlash == running);
  juce::MemoryBlock still;
  CHECK(testFlash().getChildFile("flash-128.bin").loadFileAsData(still) && still == appFlash);
  testFlash().deleteRecursively();
  e.onFreshFlash = nullptr;
  e.bootFlash.clear();
  e.virtualPorts = true;
  e.flashInFile = true;
  e.start(128);
  CHECK(e.hasVirtualPorts());
  CHECK(e.firmware().bendRange() == 2);
}

// the flash taken while the instrument plays with settings it hasn't stored, as one
// changed over MIDI is until a settings screen is left
void a_flash_kept_in_memory_holds_the_settings_the_firmware_hasnt_stored() {
  auto& e = engine();
  e.stop();
  e.virtualPorts = false;
  e.flashInFile = false;
  e.bootFlash.clear();
  e.onFreshFlash = [&e] { configureMpe(e); };
  e.start(128);
  CHECK(e.firmware().bendRange() == 48);
  CHECK(e.currentFlash().empty());
  nrpn(e, 19, 24);
  CHECK(waitFor([&e] { return e.firmware().bendRange() == 24; }));
  auto changed = e.currentFlash();
  CHECK(changed.size() == firmware::flashSize());
  e.stop();
  CHECK(e.bootFlash == changed);
  e.start(128);
  CHECK(e.firmware().bendRange() == 24);
  // unchanged since, the same flash
  CHECK(e.currentFlash() == changed);
  e.onFreshFlash = nullptr;
  e.stop();
  e.bootFlash.clear();
  e.virtualPorts = true;
  e.flashInFile = true;
  e.start(128);
}

// a flash saved by a firmware that kept its settings in another layout, elsewhere in the
// flash: they're converted as the Updater's restore converts them, or the flash starts fresh
void a_boot_flash_of_another_layout_gives_its_settings_converted() {
  auto& e = engine();
  e.stop();
  e.virtualPorts = false;
  e.flashInFile = false;
  e.onFreshFlash = [&e] { configureMpe(e); };
  auto flashWith = [&e](int range) {
    e.stop();
    e.bootFlash.clear();
    e.start(128);
    nrpn(e, 19, range);
    return e.currentFlash();
  };
  auto wanted = flashWith(24);
  auto other = flashWith(5);
  CHECK(e.firmware().bendRange() == 5);
  size_t size = e.firmware().settingsSize(), offset = e.firmware().settingsOffset();
  size_t moved = offset + sizeof(unsigned long) + 2 * size;
  CHECK(moved + sizeof(unsigned long) + 2 * size <= other.size());
  // wanted's settings, in the first of two copies at another place, in a flash that holds others
  // where this firmware keeps them
  auto from = wanted.begin() + (long) (offset + sizeof(unsigned long) + (wanted[offset] == 0 ? 0 : size));
  other[moved] = 0;
  std::copy(from, from + (long) size, other.begin() + (long) (moved + sizeof(unsigned long)));
  e.stop();
  e.setBootFlash(other, size, moved);
  e.start(128);
  // with nothing a fresh flash is given after
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  CHECK(e.firmware().bendRange() == 24);
  CHECK(e.currentFlash().size() == firmware::flashSize());
  // kept since in this firmware's layout
  nrpn(e, 19, 12);
  e.stop();
  e.start(128);
  CHECK(e.firmware().bendRange() == 12);
  // a size no layout of the firmware's has
  e.stop();
  e.setBootFlash(other, size + 1, moved);
  e.start(128);
  CHECK(e.firmware().bendRange() == 48);
  e.onFreshFlash = nullptr;
  e.stop();
  e.bootFlash.clear();
  e.virtualPorts = true;
  e.flashInFile = true;
  e.start(128);
}

// the flash taken on one thread while the engine starts or stops on another
void a_flash_kept_in_memory_is_whole_and_prompt_while_the_engine_starts_and_stops() {
  auto& e = engine();
  e.stop();
  e.virtualPorts = false;
  e.flashInFile = false;
  e.bootFlash.clear();
  e.start(128);
  nrpn(e, 19, 24);
  CHECK(waitFor([&e] { return !e.currentFlash().empty(); }));
  std::atomic<bool> done { false };
  std::thread loads([&] {
    for (int i = 0; i < 15; ++i) {
      e.stop();
      e.start(128);
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    done = true;
  });
  double slowest = 0;
  int torn = 0;
  while (!done) {
    auto asked = juce::Time::getMillisecondCounterHiRes();
    auto flash = e.currentFlash();
    slowest = juce::jmax(slowest, juce::Time::getMillisecondCounterHiRes() - asked);
    torn += flash.size() == firmware::flashSize() ? 0 : 1;
  }
  loads.join();
  CHECK(torn == 0);
  CHECK(slowest < 500);
  e.stop();
  e.bootFlash.clear();
  e.virtualPorts = true;
  e.flashInFile = true;
  testFlash().deleteRecursively();
  e.start(128);
}

// ticks at a tempo, in real time, after a start or without one
void clockAt(Engine& e, double bpm, int ticks, bool start) {
  const uint8_t go[] = { 0xfa }, tick[] = { 0xf8 };
  if (start) e.sendMidi(go, 1);
  auto next = std::chrono::steady_clock::now();
  for (int i = 0; i < ticks; ++i) {
    e.sendMidi(tick, 1);
    next += std::chrono::microseconds((long long) (60.0e6 / bpm / 24));
    std::this_thread::sleep_until(next);
  }
}

void a_copy_booted_again_forgets_the_clock_it_followed() {
  auto& e = engine();
  // until the firmware follows it away from the 120 it boots with: a busy machine's late ticks can
  // keep its average from settling on 100, and what's checked is what a boot forgets
  clockAt(e, 100, 48, true);
  for (int beat = 0; beat < 10 && std::abs(e.firmware().tempo() - 120) < 10; ++beat) clockAt(e, 100, 24, false);
  CHECK(std::abs(e.firmware().tempo() - 120) >= 10);
  e.stop();
  e.start(128);
  CHECK(e.firmware().tempo() == 120);
  // ticks without a start are no clock to follow
  clockAt(e, 90, 36, false);
  CHECK(e.firmware().tempo() == 120);
  // nor a parameter chosen before, an NRPN's or an RPN's, which a data entry alone would change
  for (bool registered : { false, true }) {
    const uint8_t choose[] = { 0xb0, (uint8_t) (registered ? 101 : 99), 0, 0xb0, (uint8_t) (registered ? 100 : 98), (uint8_t) (registered ? 0 : 19) };
    e.sendMidi(choose, sizeof(choose));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    e.stop();
    e.start(128);
    int range = e.firmware().bendRange();
    int other = range == 24 ? 12 : 24;
    const uint8_t value[] = { 0xb0, 6, (uint8_t) (registered ? other : 0), 0xb0, 38, (uint8_t) (registered ? 0 : other) };
    e.sendMidi(value, sizeof(value));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(e.firmware().bendRange() == range);
  }
}

// as storing the settings would, which the firmware doesn't while its clock runs
void a_flash_kept_in_memory_keeps_the_tempo_it_plays_at() {
  auto& e = engine();
  e.stop();
  e.virtualPorts = false;
  e.flashInFile = false;
  e.bootFlash.clear();
  e.start(128);
  clockAt(e, 100, 48, true);
  for (int beat = 0; beat < 10 && std::abs(e.firmware().tempo() - 120) < 10; ++beat) clockAt(e, 100, 24, false);
  const uint8_t halt[] = { 0xfc };
  e.sendMidi(halt, 1);
  // the engine takes its MIDI in on its own pace, so the ticks still on their way and the stop can
  // move the tempo after this: it's read once it holds still
  int tempo = e.firmware().tempo();
  for (int wait = 0; wait < 40; ++wait) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    int now = e.firmware().tempo();
    if (now == tempo && wait >= 2) break;
    tempo = now;
  }
  CHECK(std::abs(tempo - 120) >= 10);
  e.stop();
  e.start(128);
  CHECK(e.firmware().tempo() == tempo);
  e.stop();
  e.bootFlash.clear();
  e.virtualPorts = true;
  e.flashInFile = true;
  e.start(128);
}

// ---------------------------------------------------------------------------------------------
// the instrument's pace

void a_scan_takes_as_long_as_on_the_instrument() {
  // measured on a LinnStrument 200: 5459 us idle, 6.10, 6.61, 7.22 and 7.70 ms with 1 to 4 touches
  CHECK_NEAR(Engine::scanMicros(201, 0), 5459, 10);
  const double measured[] = { 6100, 6610, 7220, 7700 };
  for (int touches = 1; touches <= 4; ++touches) CHECK_NEAR(Engine::scanMicros(201, touches), measured[touches - 1], 120);
}

// a busy machine only ever slows the engine down, never speeds it up, so the fastest of a few
// half seconds is its pace
double scansPerSecond(Engine& e, double expected) {
  double fastest = 0;
  for (int window = 0; window < 3 && std::abs(fastest - expected) > expected * 0.1; ++window) {
    int before = e.scanCount();
    auto start = juce::Time::getMillisecondCounterHiRes();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    fastest = std::max(fastest, (e.scanCount() - before) * 1000.0 / (juce::Time::getMillisecondCounterHiRes() - start));
  }
  return fastest;
}

void the_engine_scans_at_the_instruments_rate() {
  auto& e = engine();
  double idle = 1e6 / Engine::scanMicros(129, 0);
  CHECK_NEAR(scansPerSecond(e, idle), idle, idle * 0.1);
  e.press(5, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  double touched = 1e6 / Engine::scanMicros(129, 1);
  CHECK_NEAR(scansPerSecond(e, touched), touched, touched * 0.1);
  e.release(5, 3);
  drained(e);
}

// ---------------------------------------------------------------------------------------------
// the surface: the whole instrument, drawn to fit

struct Whole {
  Surface surface { engine() };
  Whole() { surface.setBounds(0, 0, 1000, 400); }
};

void a_touch_lands_on_the_pad_under_it() {
  Whole l;
  for (int col = 1; col < engine().columnCount(); ++col) {
    for (int row = 0; row < 8; ++row) {
      auto c = l.surface.cellAt(l.surface.padBounds(col, row).getCentre());
      CHECK(c.col == col && c.row == row);
    }
  }
  for (int row = 0; row < 8; ++row) {
    auto c = l.surface.cellAt(l.surface.padBounds(0, row).getCentre());
    CHECK(c.col == 0 && c.row == row);
  }
}

juce::MouseEvent mouseAt(Surface& surface, juce::Point<float> at) {
  auto mouse = juce::Desktop::getInstance().getMainMouseSource();
  return juce::MouseEvent(mouse, at, {}, juce::MouseInputSource::defaultPressure, 0, 0, 0, 0, &surface, &surface,
                          juce::Time::getCurrentTime(), at, juce::Time::getCurrentTime(), 1, false);
}

// a finger's press is told to whoever listens, after the engine has it
void a_press_is_told_after_the_engine_has_it() {
  Whole l;
  int col = -1, row = -1;
  bool held = false;
  l.surface.onPress = [&](int c, int r) {
    col = c;
    row = r;
    held = engine().isTouched(c, r);
  };
  auto at = l.surface.padBounds(5, 3).getCentre();
  l.surface.mouseDown(mouseAt(l.surface, at));
  CHECK(col == 5 && row == 3 && held);
  l.surface.mouseUp(mouseAt(l.surface, at));
  drained(engine());
}

void the_focus_ring_shows_only_while_the_keyboard_moves_it() {
  juce::Component window;
  window.setBounds(0, 0, 1000, 400);
  Whole l;
  window.addAndMakeVisible(l.surface);
  window.addToDesktop(0);
  window.setAlpha(0.0f);
  window.setVisible(true);
  l.surface.grabKeyboardFocus();
  l.surface.keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
  CHECK(l.surface.showsFocusRing());
  auto at = l.surface.padBounds(3, 0).getCentre();
  l.surface.mouseDown(mouseAt(l.surface, at));
  l.surface.mouseUp(mouseAt(l.surface, at));
  CHECK(!l.surface.showsFocusRing());
  window.removeFromDesktop();
}

// JUCE's own test framework: one suite per part of the app; filter, when set, keeps only the
// tests whose name contains it
juce::String filter;
// --shard K/N: every Nth test from the Kth, counted across the suites, so that N of these run side
// by side share the tests out
int shard = 0, shards = 1, counted = 0;

class Suite : public juce::UnitTest {
public:
  Suite(const juce::String& name, std::vector<Case> list) : juce::UnitTest(name, "Virtual LinnStrument"), cases(std::move(list)) {}
  int matching() const {
    int n = 0;
    for (size_t i = 0; i < cases.size(); ++i) n += wanted(i) ? 1 : 0;
    return n;
  }
  void runTest() override {
    current = this;
    for (size_t i = 0; i < cases.size(); ++i) {
      auto& c = cases[i];
      if (!wanted(i)) continue;
      beginTest(juce::String(c.name).replace("_", " "));
      c.run();
    }
  }

  // the first of its tests' places among all of them
  int first = 0;
  int size() const { return (int) cases.size(); }

private:
  bool wanted(size_t i) const { return juce::String(cases[i].name).contains(filter) && (first + (int) i) % shards == shard; }
  std::vector<Case> cases;
};

Suite fingers("Fingers", {
  { "fingers_play_their_own_cells", fingers_play_their_own_cells },
  { "a_slide_presses_the_next_cell_before_letting_go_of_the_last", a_slide_presses_the_next_cell_before_letting_go_of_the_last },
  { "a_cell_stays_down_while_another_finger_is_on_it", a_cell_stays_down_while_another_finger_is_on_it },
  { "sliding_off_a_shared_cell_leaves_it_down", sliding_off_a_shared_cell_leaves_it_down },
  { "moving_into_the_next_cell_of_a_row_slides", moving_into_the_next_cell_of_a_row_slides },
  { "a_move_that_cannot_slide_is_a_press_and_a_release", a_move_that_cannot_slide_is_a_press_and_a_release },
  { "moving_to_another_row_or_further_does_not_slide", moving_to_another_row_or_further_does_not_slide },
  { "a_cell_another_finger_holds_does_not_slide_away", a_cell_another_finger_holds_does_not_slide_away },
  { "a_latched_cell_stays_down_until_touched_again", a_latched_cell_stays_down_until_touched_again },
  { "the_keyboard_toggles_a_cell", the_keyboard_toggles_a_cell },
  { "the_keyboard_lets_go_only_of_a_cell_no_finger_holds", the_keyboard_lets_go_only_of_a_cell_no_finger_holds },
});

Suite slides("Slides through the firmware", {
  { "a_slide_keeps_the_pressure_steady", a_slide_keeps_the_pressure_steady },
  { "a_slide_into_the_next_cell_bends_the_same_note", a_slide_into_the_next_cell_bends_the_same_note },
  { "the_pitch_follows_the_finger_across_a_pad", the_pitch_follows_the_finger_across_a_pad },
  { "a_slide_glides_from_one_note_to_the_next", a_slide_glides_from_one_note_to_the_next },
  { "lifting_and_pressing_the_next_cell_starts_a_new_note", lifting_and_pressing_the_next_cell_starts_a_new_note },
  { "a_slide_into_a_cell_still_lifting_ends_its_note", a_slide_into_a_cell_still_lifting_ends_its_note },
  { "a_quick_double_tap_on_a_switch_opens_and_closes_its_screen", a_quick_double_tap_on_a_switch_opens_and_closes_its_screen },
  { "a_slide_stays_among_the_pads", a_slide_stays_among_the_pads },
});

Suite expression("Expression", {
  { "a_new_pressure_reaches_the_notes_already_held", a_new_pressure_reaches_the_notes_already_held },
});

Suite bridge("Firmware bridge", {
  { "the_bridge_reports_a_settings_screen", the_bridge_reports_a_settings_screen },
  { "the_bridge_reports_the_sequencer_and_its_settings", the_bridge_reports_the_sequencer_and_its_settings },
  { "the_bridge_reads_and_edits_the_sequencers_notes", the_bridge_reads_and_edits_the_sequencers_notes },
  { "the_bridge_sets_the_sequencers_patterns_and_settings", the_bridge_sets_the_sequencers_patterns_and_settings },
  { "the_bridge_loads_saves_and_stores_projects", the_bridge_loads_saves_and_stores_projects },
  { "the_bridge_taps_the_tempo_as_its_switch_does", the_bridge_taps_the_tempo_as_its_switch_does },
  { "the_bridge_reports_the_bend_range", the_bridge_reports_the_bend_range },
  { "a_note_sent_on_a_splits_note_channel_lights_its_pads", a_note_sent_on_a_splits_note_channel_lights_its_pads },
  { "every_unit_is_calibrated", every_unit_is_calibrated },
  { "the_nameplate_is_the_panels", the_nameplate_is_the_panels },
  { "painted_bold_text_is_mulish_bold", painted_bold_text_is_mulish_bold },
  { "the_engines_ports_are_listed_by_their_identifiers", the_engines_ports_are_listed_by_their_identifiers },
  { "the_bridge_reports_the_tempo", the_bridge_reports_the_tempo },
  { "the_bridge_reports_which_foot_switches_latch", the_bridge_reports_which_foot_switches_latch },
  { "lifted_sensor_limits_let_a_fourth_touch_into_a_column", lifted_sensor_limits_let_a_fourth_touch_into_a_column },
  { "a_strike_sets_the_velocity_whatever_the_pressure", a_strike_sets_the_velocity_whatever_the_pressure },
  { "every_cell_strikes_alike", every_cell_strikes_alike },
  { "letting_go_lowers_the_pressure_before_the_note_ends", letting_go_lowers_the_pressure_before_the_note_ends },
  { "a_soft_press_rises_slowly_and_a_hard_one_at_once", a_soft_press_rises_slowly_and_a_hard_one_at_once },
  { "the_bridge_reports_which_settings_screen_is_up", the_bridge_reports_which_settings_screen_is_up },
  { "a_switch_label_reads_on_every_light", a_switch_label_reads_on_every_light },
  { "each_copy_of_the_firmware_is_handed_to_one_engine_at_a_time", each_copy_of_the_firmware_is_handed_to_one_engine_at_a_time },
  { "two_engines_run_their_own_firmware_side_by_side", two_engines_run_their_own_firmware_side_by_side },
  { "a_fresh_flash_starts_with_the_settings_given_and_a_saved_one_without", a_fresh_flash_starts_with_the_settings_given_and_a_saved_one_without },
  { "an_engine_without_ports_opens_none_and_keeps_its_flash_out_of_files", an_engine_without_ports_opens_none_and_keeps_its_flash_out_of_files },
  { "the_bridge_reads_a_screens_values_and_the_small_font", the_bridge_reads_a_screens_values_and_the_small_font },
  { "the_bridge_reads_a_splits_cc_faders", the_bridge_reads_a_splits_cc_faders },
  { "the_bridge_reads_whether_scale_select_shows", the_bridge_reads_whether_scale_select_shows },
  { "the_bridge_lights_only_the_chosen_steps_notes_in_notes_view", the_bridge_lights_only_the_chosen_steps_notes_in_notes_view },
  { "the_bridge_times_a_hold_as_the_firmware_does", the_bridge_times_a_hold_as_the_firmware_does },
  { "the_engine_sets_the_bend_range_as_its_screen_does", the_engine_sets_the_bend_range_as_its_screen_does },
  { "the_bridge_reads_and_sets_the_panels_settings_as_nrpns_do", the_bridge_reads_and_sets_the_panels_settings_as_nrpns_do },
  { "the_bridge_assigns_the_switches_as_the_grid_does", the_bridge_assigns_the_switches_as_the_grid_does },
  { "the_bridge_reads_and_sets_the_drum_notes", the_bridge_reads_and_sets_the_drum_notes },
  { "the_bridge_edits_and_shows_the_audience_messages", the_bridge_edits_and_shows_the_audience_messages },
  { "a_copy_booted_again_forgets_the_clock_it_followed", a_copy_booted_again_forgets_the_clock_it_followed },
  { "a_flash_kept_in_memory_keeps_the_tempo_it_plays_at", a_flash_kept_in_memory_keeps_the_tempo_it_plays_at },
  { "a_flash_kept_in_memory_holds_the_settings_the_firmware_hasnt_stored", a_flash_kept_in_memory_holds_the_settings_the_firmware_hasnt_stored },
  { "a_boot_flash_of_another_layout_gives_its_settings_converted", a_boot_flash_of_another_layout_gives_its_settings_converted },
  { "a_flash_kept_in_memory_is_whole_and_prompt_while_the_engine_starts_and_stops", a_flash_kept_in_memory_is_whole_and_prompt_while_the_engine_starts_and_stops },
});

Suite surface("The surface", {
  { "a_touch_lands_on_the_pad_under_it", a_touch_lands_on_the_pad_under_it },
  { "a_press_is_told_after_the_engine_has_it", a_press_is_told_after_the_engine_has_it },
  { "the_focus_ring_shows_only_while_the_keyboard_moves_it", the_focus_ring_shows_only_while_the_keyboard_moves_it },
});

Suite pace("The instrument's pace", {
  { "a_scan_takes_as_long_as_on_the_instrument", a_scan_takes_as_long_as_on_the_instrument },
  { "the_engine_scans_at_the_instruments_rate", the_engine_scans_at_the_instruments_rate },
  { "the_bridge_reads_each_splits_special_feature", the_bridge_reads_each_splits_special_feature },
});


}

// runs every test, or with NAME only those whose name contains it, or with --shard K/N its share,
// and fails on any failure
int main(int argc, char* argv[]) {
  juce::ScopedJuceInitialiser_GUI gui;
  LinnLookAndFeel lookAndFeel;
  juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
  for (int i = 1; i < argc; ++i) {
    juce::String arg(argv[i]);
    if (arg == "--shard" && i + 1 < argc) {
      juce::String part(argv[++i]);
      shard = part.upToFirstOccurrenceOf("/", false, false).getIntValue();
      shards = juce::jmax(1, part.fromFirstOccurrenceOf("/", false, false).getIntValue());
    }
    else filter = arg;
  }
  juce::Array<juce::UnitTest*> chosen;
  int tests = 0;
  for (auto* suite : { &fingers, &slides, &expression, &bridge, &pace, &surface }) {
    suite->first = counted;
    counted += suite->size();
  }
  for (auto* suite : { &fingers, &slides, &expression, &bridge, &pace, &surface }) {
    if (suite->matching() == 0) continue;
    chosen.add(suite);
    tests += suite->matching();
  }
  if (chosen.isEmpty()) {
    // a shard with nothing left to it has nothing to fail
    if (shards > 1 && filter.isEmpty()) {
      std::cout << "0 tests, 0 failed checks" << std::endl;
      return 0;
    }
    std::cout << "no test name contains " << filter << std::endl;
    return 1;
  }
  juce::UnitTestRunner runner;
  runner.setAssertOnFailure(false);
  runner.runTests(chosen);
  int failures = 0;
  for (int i = 0; i < runner.getNumResults(); ++i) failures += runner.getResult(i)->failures;
  if (started != nullptr) started->stop();
  testFlash().deleteRecursively();
  juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
  std::cout << tests << " tests, " << failures << " failed checks" << std::endl;
  return failures == 0 ? 0 : 1;
}
