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
// The one door between the app and the firmware. Each copy of the firmware is compiled with the
// Arduino stand-ins into a namespace of its own (FirmwareInstance.inc), so that every copy has
// its own globals, and nothing of that reaches the rest of the app: this header uses plain C++
// types only.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace firmware {

enum Display { off = 0, on = 1, fastPulse = 2, slowPulse = 3, focusPulse = 4 };

// the screens whose cells have a fixed meaning that an app can name, or none: the playing display,
// a value being slid, and anything else; apps may rely on the order, so new ones go at the end
enum Screen {
  noScreen, perSplitScreen, globalScreen, sequencerScreen, sequencerProjectsScreen, sequencerDrums1Screen,
  sequencerDrums2Screen, sequencerColorsScreen, presetScreen, volumeScreen, octaveTransposeScreen, splitPointScreen,
  customLightsScreen
};

struct Led {
  uint8_t color;
  uint8_t display;
};

// every MIDI byte the firmware sends, from whichever thread runs loop()
using SerialSink = std::function<void(const uint8_t* data, size_t length)>;

// One copy of the firmware with all of its state. Its engine's thread runs it (boot, loopOnce,
// setSensor); MIDI comes in from any thread, through the stand-ins' locked serial input; the
// lights and the other answers are read from other threads as they are, which is good enough to
// draw and report but never to change the firmware.
class Instance {
public:
  virtual ~Instance() = default;

  // Runs setup() on the calling thread with the given flash contents, or a fresh flash when the
  // image is empty, in which case the instrument boots in MIDI mode with the calibration screen
  // skipped.
  virtual void boot(int model, const std::vector<uint8_t>& flashImage, SerialSink sink) = 0;
  virtual void loopOnce() = 0;
  virtual int cellsPerScan() = 0;

  virtual int model() = 0;
  virtual bool calibrated() = 0;
  // whether a later boot keeps the calibration, by the check the firmware runs on its saved one
  virtual bool calibrationHolds() = 0;
  virtual int columns() = 0;
  virtual int rows() = 0;
  virtual bool userFirmwareMode() = 0;
  // whether Global Settings' View shows Scale Select, where a hold on custom pattern A, A# or B
  // opens its light editor
  virtual bool scaleSelectShown() = 0;
  virtual bool serialMode() = 0;
  // out of OS update (serial) mode and User Firmware Mode, back to playing over MIDI, as tapping
  // UPDATE OS and holding it again would; from the firmware's thread
  virtual void backToMidi() = 0;
  // a settings screen or anything else that is not the playing display, which uses every cell
  virtual bool showingSettings() = 0;
  virtual Screen screen() = 0;
  // on the sequencer screen, whether a note is selected, which puts its faders on the surface
  virtual bool sequencerNoteSelected() = 0;
  // the left split's pitch bend range in semitones
  virtual int bendRange() = 0;
  // the value screen a held settings cell has opened in place of the settings, none on any other
  enum class ValueScreen { none, bendRange, limitsForY, ccForY, initialForRelativeY, limitsForZ, ccForZ, rowOffset, guitarTuning,
                           ccForSwitchSustain, ccForSwitchCC65, switchAssignment, limitsForVelocity, valueForFixedVelocity, drumNotes,
                           audienceMessage, sequencerColours };
  virtual ValueScreen valueScreen() = 0;
  // a split's bend range, 1 to 96 semitones, and setting it as the firmware's bend range screen does
  // on release, telling MPE synths of it in the same way
  virtual int bendRangeOf(int split) = 0;
  virtual void setBendRange(int split, int semitones) = 0;
  // a setting by its NRPN number (midi.md), its value as the firmware reports it to an NRPN query, for
  // the per-split expression and global settings listed in nrpn(), and setting it as an NRPN received
  // does, range checks included;
  // nrpnUnknown for another
  static constexpr int nrpnUnknown = -1000000;
  virtual int nrpn(int parameter) = 0;
  virtual void setNrpn(int parameter, int value) = 0;
  // the switch Global Settings' SELECT SW shows: 0 the left pedal, 1 the right one, 2 Switch 2,
  // 3 Switch 1, 4 both pedals
  virtual int selectedSwitch() = 0;
  // a switch by that index, both pedals included, which no NRPN reaches: its assignment, set as the
  // Global Settings grid sets it (the state of the one it had let go of, and the same one again
  // switching it off); whether it acts on both splits; the CC its Sustain or CC 65 sends
  virtual int switchAssignment(int which) = 0;
  virtual void assignSwitch(int which, int assignment) = 0;
  virtual bool switchBothSplits(int which) = 0;
  virtual void setSwitchBothSplits(int which, bool both) = 0;
  virtual int switchCC(int which, bool sustain) = 0;
  virtual void setSwitchCC(int which, bool sustain, int cc) = 0;
  // the note a split's sequencer plays for each of its 14 drum rows, from 0, and setting it as the
  // sequencer's drum screens do
  virtual int drumNote(int split, int drum) = 0;
  virtual void setDrumNote(int split, int drum, int note) = 0;
  // the 16 audience messages, without the spaces the edit screen pads one with; the one that screen
  // edits, from 0; setting one, up to 30 of the characters from space to ~, padded again where it's
  // the one the screen edits; and scrolling one across the surface for the audience, as a tap on its
  // cell in Global Settings does, while which the firmware is animating
  virtual std::string audienceMessage(int message) = 0;
  virtual int audienceMessageEdited() = 0;
  virtual void setAudienceMessage(int message, const std::string& text) = 0;
  virtual void showAudienceMessage(int message) = 0;
  virtual bool animating() = 0;
  // lifts the limits that guard against an instrument's sensor, for a touch screen that has none
  // of its cross talk or phantom touches: more than three touches in a column, the fourth corner
  // of a rectangle, the settings switches while notes are held and the pitch of light touches
  // sharing a column (sensorLimitsLifted in linnstrument-firmware.ino)
  virtual void liftSensorLimits(bool lifted) = 0;
  // the channels, 1 to 16, that a note sent in on lights every pad of in a split: the one it takes
  // notes on, or each row's channel in channel per row, where the channel picks the row
  virtual std::vector<int> noteChannels(int split) = 0;
  // how a split plays over MIDI, read directly, so from the firmware's own thread too: split on, its
  // mode (0 one channel, 1 a channel per note, 2 a channel per row), its main channel and whether
  // that sends, the per-note or per-row channels it plays notes on, and its bend range
  struct SplitMidi {
    bool splitOn = false;
    int mode = 0, main = 1, bendRange = 2;
    bool mainSends = true;
    std::vector<int> noteChannels;
    // every channel the split plays on, its main one first
    std::vector<int> channels() const {
      std::vector<int> all { main };
      for (int c : noteChannels)
        if (c != main) all.push_back(c);
      return all;
    }
  };
  virtual SplitMidi splitMidi(int split) = 0;
  // the tempo in beats per minute that the arpeggiator and the sequencer play at, tapped, set over
  // MIDI or followed from a MIDI clock
  virtual int tempo() = 0;
  // a tap of the tempo, timed as a switch assigned to Tap Tempo times it; and whether the firmware
  // follows a MIDI clock coming in, whose tempo it then plays at
  virtual void tapTempo() = 0;
  virtual bool followsMidiClock() = 0;
  // the split the settings screens show and change, 0 the left one
  virtual int shownSplit() = 0;
  // a split's program, 1 to 128, and its MIDI volume (CC 7), 0 to 127
  virtual int program(int split) = 0;
  virtual int volume(int split) = 0;
  // a split's eight CC faders while it plays them in place of notes: the columns they span, from the
  // first and as many after it as the firmware counts (its fader length), and each row's value, 0 to
  // 127, -1 for a row without one
  struct Faders {
    bool shown = false;
    int left = 1, length = 0;
    int values[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
  };
  virtual Faders ccFaders(int split) = 0;
  // a split's main colour and the colour the global screens draw in, as the firmware's colour codes
  virtual int mainColor(int split) = 0;
  virtual int settingsColor() = 0;
  // a character of the firmware's small font, the one its value screens show numbers in: its rows top
  // first, a non-space for each lit dot; no rows for a character the font lacks
  struct Glyph {
    int width = 0;
    std::vector<std::string> rows;
  };
  virtual Glyph smallGlyph(char c) = 0;
  // the same of the big font, the one the audience messages scroll in
  virtual Glyph bigGlyph(char c) = 0;
  // MIDI the firmware has queued and not yet sent
  virtual bool midiPending() = 0;
  // MIDI fed to the firmware that it hasn't read yet
  virtual bool midiInPending() = 0;

  virtual Led led(int col, int row) = 0;
  virtual bool pulseOn(int display) = 0;

  // Raw sensor readings for a position inside a cell, from the firmware's own default calibration.
  virtual uint16_t rawX(int col, float fraction) = 0;
  virtual uint16_t rawY(int row, float fraction) = 0;
  // Z is 0 for no touch; several hundred is a normal touch, a few thousand is firm.
  virtual void setSensor(int col, int row, uint16_t x, uint16_t y, uint16_t z) = 0;
  // whether the firmware still takes a cell as touched, until it has let go of it, which takes a
  // control switch, visited every eight scans, two visits without a touch, and a note at least
  // 35 ms from its start
  virtual bool cellHeld(int col, int row) = 0;
  // how long the firmware has timed a cell's hold, in milliseconds from its touch, as its hold
  // handlers do; -1 for a cell not held or whose hold the firmware has acted on and stopped timing
  virtual int heldMillis(int col, int row) = 0;
  // how long the firmware waits for each kind of hold before it acts
  enum class Hold { sensor, controlSwitch, confirm, edit };
  virtual int holdDelay(Hold hold) = 0;

  virtual void feedMidi(const uint8_t* data, size_t length) = 0;
  virtual void footSwitch(int which, bool down) = 0;
  // whether a foot switch's assignment stays on until switched off, as sustain or the arpeggiator
  // do, rather than acting once, as an octave or a preset change does
  virtual bool footSwitchLatches(int which) = 0;

  // The step sequencer, one per split, read as it is and changed as its own screen changes it.
  // Every change runs on the firmware's thread, between two of its loops.
  // its sizes: patterns per split, steps per pattern, notes per step, and steps the surface shows
  // at once (16 on a 200, 8 on a 128), which is one page
  virtual int sequencerPatterns() = 0;
  virtual int sequencerSteps() = 0;
  virtual int sequencerNotesPerStep() = 0;
  virtual int sequencerStepsShown() = 0;
  // a note of a step: its MIDI note, velocity 1 to 127, length in 96ths of a whole note (one of
  // sequencerLengths()), fine tune in cents, -50 to 50, timbre 0 to 127, and the row it was entered
  // on, which together with its note tells it apart; none where the place is empty
  struct SequencerNote {
    bool used = false;
    int note = 0, velocity = 0, length = 0, fineTune = 0, timbre = 0, row = 0;
  };
  // the 17 lengths a note can have, shortest first, in 96ths
  virtual std::vector<int> sequencerLengths() = 0;
  // a pattern's settings: its step size in 96ths (6 a sixteenth, 12 an eighth, 24 a quarter, with
  // their dotted and triplet sizes), swing, direction (0 forward, 1 back, 2 ping-pong), looping the
  // page shown, and its length, 1 to 32 steps
  struct SequencerPattern {
    int stepSize = 0, direction = 0, length = 0;
    bool swing = false, loopPage = false;
  };
  virtual SequencerPattern sequencerPattern(int split, int pattern) = 0;
  // a step's notes in their places, as many as sequencerNotesPerStep()
  virtual std::vector<SequencerNote> sequencerStep(int split, int pattern, int step) = 0;
  // a split's sequencer as it plays and shows: on its screen at all, running, muted, the pattern
  // playing and the one queued after it (-1 for none), the step playing (-1 before the first),
  // the first step of the page and the first row shown, the view (0 notes, 1 scales, 2 drums), the
  // step and note selected (-1 for none), and whether ping-pong is going forward
  struct SequencerState {
    bool on = false, running = false, muted = false, forward = true;
    int pattern = 0, nextPattern = -1, position = -1, pageStart = 0, rowOffset = 0, view = 0;
    int selectedStep = -1, selectedNote = -1;
  };
  virtual SequencerState sequencerState(int split) = 0;
  // the split whose sequencer the surface shows, or -1; and showing a split's, as a tap on SPLIT
  // switches between them
  virtual int sequencerShown() = 0;
  virtual void showSequencerSplit(int split) = 0;
  // the note a row of Scales or Drums plays, counted from the bottom of all fourteen whichever the
  // surface shows; -1 for none
  virtual int sequencerRowNote(int split, int row) = 0;
  // the note a pad of a split plays, as Notes view's pads enter them; -1 for none
  virtual int padNote(int split, int col, int row) = 0;
  // the colours the sequencer draws a split in, as the firmware's colour codes: its main and accent
  // colours, the playing step's, and a step's on the steps bar while empty, with notes and past the end
  struct SequencerColours {
    int main = 0, accent = 0, played = 0, lowRow = 0, empty = 0, withNotes = 0, beyond = 0;
  };
  virtual SequencerColours sequencerColours(int split) = 0;
  // a step's colour on the steps bar while empty (0), with notes (1) or past the end (2), as the
  // sequencer's colours screen cycles them: any colour but off and black
  virtual void setSequencerColour(int split, int which, int colour) = 0;
  // a new note on a step of the pattern playing, where the firmware's own touch puts one: in the
  // first empty place, or in the last with the oldest let go when the step is full, then selected;
  // its velocity and timbre given, its length the pattern's step size; none where the step has that
  // note on that row already, rows from 1. The step's notes are edited, selected and the view
  // changed only while the split's sequencer is the one shown, as on the instrument
  virtual void addSequencerNote(int split, int step, int note, int row, int velocity, int timbre) = 0;
  // a step's note let go of, those after it moving up a place, as a quick tap on it does
  virtual void removeSequencerNote(int split, int step, int place) = 0;
  // a note selected, which plays it while the sequencer is stopped and not muted, or a step with
  // none of its notes (place -1); nothing selected for step -1
  virtual void selectSequencerNote(int split, int step, int place) = 0;
  // a note's values changed, each within its range
  enum class NoteValue { velocity, length, fineTune, timbre };
  virtual void setSequencerNoteValue(int split, int step, int place, NoteValue what, int value) = 0;
  // a note heard as the firmware lets a selected note be heard, while the sequencer is stopped and
  // not muted, through the split's own channels, for the length of a step
  virtual void previewSequencerNote(int split, int note, int velocity) = 0;
  // as the steps bar's Clear and Copy do with a step, on the pattern playing
  virtual void clearSequencerStep(int split, int step) = 0;
  virtual void copySequencerStep(int split, int from, int to) = 0;
  // as the pattern selector does: a pattern chosen, cleared, or copied over another's
  virtual void selectSequencerPattern(int split, int pattern) = 0;
  virtual void clearSequencerPattern(int split, int pattern) = 0;
  virtual void copySequencerPattern(int fromSplit, int from, int toSplit, int to) = 0;
  // the pattern playing's step size and swing, set as its step size cells set them, a running
  // sequencer's next step realigned to the new size as they realign it; its direction, its loop of
  // the page shown and its length
  virtual void setSequencerStepSize(int split, int stepSize, bool swing) = 0;
  virtual void setSequencerDirection(int split, int direction) = 0;
  virtual void setSequencerLoopPage(int split, bool loop) = 0;
  virtual void setSequencerLength(int split, int length) = 0;
  // the view, as the view cells choose it; the page and the first row shown, as the navigation
  // cells move them
  virtual void setSequencerView(int split, int view) = 0;
  virtual void showSequencerPage(int split, int pageStart, int rowOffset) = 0;
  // muting a split's sequencer and starting or stopping it, as the muter and Play/Stop do; moving
  // where it plays next, as a step tapped with Switch 2 held does
  virtual void muteSequencer(int split, bool muted) = 0;
  virtual void playSequencer(int split, bool running) = 0;
  virtual void moveSequencerPlayhead(int split, int step) = 0;
  // the sixteen projects the flash keeps: one as loading it would read it, the one played now (its
  // tempo the one playing), and the one last loaded, -1 before any; loading one and saving the one played over one,
  // as the projects screen's tap and hold do, both sequencers stopping; another project stored in a
  // slot as it is, its own tempo kept; and the one played given back with the slot it was loaded
  // from, as it was before a load
  virtual int sequencerProjects() = 0;
  virtual std::vector<uint8_t> storedProject(int slot) = 0;
  virtual std::vector<uint8_t> currentProject() = 0;
  virtual int loadedProject() = 0;
  // the settings' layout, as a backup names it
  virtual int settingsVersion() = 0;
  // whether a project's bytes hold only what the firmware could have stored, which the edits that
  // take a project or a pattern in insist on
  virtual bool sequencerProjectValid(const std::vector<uint8_t>& project) = 0;
  virtual void loadSequencerProject(int slot) = 0;
  virtual void saveSequencerProject(int slot) = 0;
  virtual void storeSequencerProject(int slot, const std::vector<uint8_t>& project) = 0;
  virtual void restoreSequencerProject(const std::vector<uint8_t>& project, int loaded) = 0;
  // a project's contents read as the firmware reads them: a pattern's settings and a step's notes,
  // and its tempo
  virtual SequencerPattern projectPattern(const std::vector<uint8_t>& project, int split, int pattern) = 0;
  virtual std::vector<SequencerNote> projectStep(const std::vector<uint8_t>& project, int split, int pattern, int step) = 0;
  virtual int projectTempo(const std::vector<uint8_t>& project) = 0;
  // a pattern as the flash keeps it, to keep and give back as it was
  virtual std::vector<uint8_t> sequencerPatternData(int split, int pattern) = 0;
  virtual void setSequencerPatternData(int split, int pattern, const std::vector<uint8_t>& data) = 0;

  virtual std::vector<uint8_t> flashImage() = 0;
  // the flash as the firmware would store it now, with the settings it plays with, which it only
  // stores on leaving a settings screen and the like; the flash itself when they're what it holds
  virtual std::vector<uint8_t> liveFlashImage() = 0;
  // the settings' size and where in the flash they're kept, which a firmware with another layout
  // has others of
  virtual size_t settingsSize() = 0;
  virtual size_t settingsOffset() = 0;
  // settings of another layout converted, as the Updater's restore does, into a firmware booted
  // fresh; false when they can't be, which leaves the defaults
  virtual bool upgradeSettings(const std::vector<uint8_t>& settings) = 0;
  virtual size_t flashSize() = 0;
  virtual const char* version() = 0;
};

// the copies compiled into the app; acquire() hands out one that nothing uses, or nullptr when
// they're all taken, and release() gives it back
int instanceCount();
Instance* acquire();
void release(Instance* instance);
// each copy registers itself as the program starts
void add(Instance* instance);

// the same for every copy, answered by the first
const char* version();
size_t flashSize();

}
