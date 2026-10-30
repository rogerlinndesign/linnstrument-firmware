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
#include <JuceHeader.h>

#include <cmath>
#include <iostream>
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
juce::File testFlash() {
  return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("VirtualTestsFlash");
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

// the two screens a touch app labels, and neither once Per-Split Settings is closed again
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

void the_bridge_reports_the_bend_range() {
  engine();
  CHECK(engine().firmware().bendRange() == 2);
}

// NRPN 238 sets the tempo, as an editor would
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

// ---------------------------------------------------------------------------------------------
// the instrument's pace

void a_scan_takes_as_long_as_on_the_instrument() {
  // measured on a LinnStrument 200: 5459 us idle, 6.10, 6.61, 7.22 and 7.70 ms with 1 to 4 touches
  CHECK_NEAR(Engine::scanMicros(201, 0), 5459, 10);
  const double measured[] = { 6100, 6610, 7220, 7700 };
  for (int touches = 1; touches <= 4; ++touches) CHECK_NEAR(Engine::scanMicros(201, touches), measured[touches - 1], 120);
}

double scansPerSecond(Engine& e) {
  int before = e.scanCount();
  auto start = juce::Time::getMillisecondCounterHiRes();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  return (e.scanCount() - before) * 1000.0 / (juce::Time::getMillisecondCounterHiRes() - start);
}

void the_engine_scans_at_the_instruments_rate() {
  auto& e = engine();
  double idle = 1e6 / Engine::scanMicros(129, 0);
  CHECK_NEAR(scansPerSecond(e), idle, idle * 0.1);
  e.press(5, 3, 0.5f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  double touched = 1e6 / Engine::scanMicros(129, 1);
  CHECK_NEAR(scansPerSecond(e), touched, touched * 0.1);
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

class Suite : public juce::UnitTest {
public:
  Suite(const juce::String& name, std::vector<Case> list) : juce::UnitTest(name, "Virtual LinnStrument"), cases(std::move(list)) {}
  int matching() const {
    int n = 0;
    for (auto& c : cases) n += juce::String(c.name).contains(filter) ? 1 : 0;
    return n;
  }
  void runTest() override {
    current = this;
    for (auto& c : cases) {
      if (!juce::String(c.name).contains(filter)) continue;
      beginTest(juce::String(c.name).replace("_", " "));
      c.run();
    }
  }

private:
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
  { "a_slide_into_the_next_cell_bends_the_same_note", a_slide_into_the_next_cell_bends_the_same_note },
  { "the_pitch_follows_the_finger_across_a_pad", the_pitch_follows_the_finger_across_a_pad },
  { "a_slide_glides_from_one_note_to_the_next", a_slide_glides_from_one_note_to_the_next },
  { "lifting_and_pressing_the_next_cell_starts_a_new_note", lifting_and_pressing_the_next_cell_starts_a_new_note },
  { "a_slide_into_a_cell_still_lifting_ends_its_note", a_slide_into_a_cell_still_lifting_ends_its_note },
  { "a_slide_stays_among_the_pads", a_slide_stays_among_the_pads },
});

Suite expression("Expression", {
  { "a_new_pressure_reaches_the_notes_already_held", a_new_pressure_reaches_the_notes_already_held },
});

Suite bridge("Firmware bridge", {
  { "the_bridge_reports_a_settings_screen", the_bridge_reports_a_settings_screen },
  { "the_bridge_reports_the_sequencer_and_its_settings", the_bridge_reports_the_sequencer_and_its_settings },
  { "the_bridge_reports_the_bend_range", the_bridge_reports_the_bend_range },
  { "every_unit_is_calibrated", every_unit_is_calibrated },
  { "the_bridge_reports_the_tempo", the_bridge_reports_the_tempo },
  { "the_bridge_reports_which_settings_screen_is_up", the_bridge_reports_which_settings_screen_is_up },
  { "each_copy_of_the_firmware_is_handed_to_one_engine_at_a_time", each_copy_of_the_firmware_is_handed_to_one_engine_at_a_time },
  { "two_engines_run_their_own_firmware_side_by_side", two_engines_run_their_own_firmware_side_by_side },
  { "a_fresh_flash_starts_with_the_settings_given_and_a_saved_one_without", a_fresh_flash_starts_with_the_settings_given_and_a_saved_one_without },
});

Suite surface("The surface", {
  { "a_touch_lands_on_the_pad_under_it", a_touch_lands_on_the_pad_under_it },
  { "a_press_is_told_after_the_engine_has_it", a_press_is_told_after_the_engine_has_it },
  { "the_focus_ring_shows_only_while_the_keyboard_moves_it", the_focus_ring_shows_only_while_the_keyboard_moves_it },
});

Suite pace("The instrument's pace", {
  { "a_scan_takes_as_long_as_on_the_instrument", a_scan_takes_as_long_as_on_the_instrument },
  { "the_engine_scans_at_the_instruments_rate", the_engine_scans_at_the_instruments_rate },
});


}

// runs every test, or with NAME only those whose name contains it, and fails on any failure
int main(int argc, char* argv[]) {
  juce::ScopedJuceInitialiser_GUI gui;
  LinnLookAndFeel lookAndFeel;
  juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
  filter = argc > 1 ? argv[1] : "";
  juce::Array<juce::UnitTest*> chosen;
  int tests = 0;
  for (auto* suite : { &fingers, &slides, &expression, &bridge, &pace, &surface }) {
    if (suite->matching() == 0) continue;
    chosen.add(suite);
    tests += suite->matching();
  }
  if (chosen.isEmpty()) {
    std::cout << "no test name contains " << filter << std::endl;
    return 1;
  }
  juce::UnitTestRunner runner;
  runner.setAssertOnFailure(false);
  runner.runTests(chosen);
  int failures = 0;
  for (int i = 0; i < runner.getNumResults(); ++i) failures += runner.getResult(i)->failures;
  if (started != nullptr) started->stop();
  juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
  std::cout << tests << " tests, " << failures << " failed checks" << std::endl;
  return failures == 0 ? 0 : 1;
}
