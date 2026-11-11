#!/usr/bin/env python3
# Copyright 2026 Roger Linn Design (https://www.rogerlinndesign.com)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Proves the Virtual LinnStrument's tests can fail: applies each change below to the app's code,
rebuilds VirtualTests, checks that the named tests fail, and restores the file.

    tools/virtual/mutate_tests.py [NAME]   only the lines whose test name contains NAME

Needs the app built as tools/virtual/README.md says, in build/virtual. A line marked 'copies' runs
in a build with two copies of the firmware, build/virtual-copies, which it configures if missing."""
import re, subprocess, sys, shutil, os, time
root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
src = root + '/tools/virtual/Source/'
def build_dir(copies):
    return root + ('/build/virtual-copies' if copies else '/build/virtual')

def build(copies):
    if copies and not os.path.isdir(build_dir(True)):
        subprocess.run(['cmake', '-S', root + '/tools/virtual', '-B', build_dir(True), '-DCMAKE_BUILD_TYPE=Release', '-DLINN_FIRMWARE_COPIES=2',
                        '-DJUCE_PATH=' + root + '/build/virtual/_deps/juce-src'], capture_output=True)
    return subprocess.run(['cmake', '--build', build_dir(copies), '--config', 'Release', '--target', 'VirtualTests', '-j8'], capture_output=True, text=True)
# the file under Source/, its text that occurs once, the broken text, part of the test's name
M = [
 ('Surface.cpp', '  if (state == lastShown) return false;\n', '', 'idle_surface_isnt_repainted'),
 ('Surface.cpp', '      state.push_back((uint32_t) led.color | (uint32_t) (lit ? 0x100 : 0));', '      state.push_back(0);', 'idle_surface_isnt_repainted'),
 ('Surface.cpp', '      if (engine.touchAt(col, row, touch))\n', '      if (false)\n', 'idle_surface_isnt_repainted'),
 ('Firmware/FirmwareInstance.inc', '    if (userFirmwareActive) changeUserFirmwareMode(false);\n    if (Device.serialMode) {', '    if (Device.serialMode) {', 'back_to_midi'),
 ('Firmware/FirmwareInstance.inc', '      switchSerialMode(false);\n      storeSettings();\n    }\n    updateDisplay();', '      storeSettings();\n    }\n    updateDisplay();', 'back_to_midi'),
 ('Engine.cpp', '  std::vector<uint8_t> image = saveLive ? instance->liveFlashImage() : instance->flashImage();', '  std::vector<uint8_t> image = instance->flashImage();', 'flash_file_saved_live'),
 ('Engine.cpp', '    if (++scansRun % 2000 == 0 || wanted) saveFlash();', '    if (++scansRun % 2000 == 0) saveFlash();', 'flash_file_saved_live'),
 ('Firmware/FirmwareInstance.inc', '      case 35: return Split[split].sequencer ? 4 : Split[split].strum ? 3 : Split[split].ccFaders ? 2 : Split[split].arpeggiator ? 1 : 0;', '      case 35: return Split[split].sequencer ? 4 : 0;', 'reads_each_splits_special'),
 ('Firmware/FirmwareInstance.inc', '    midiSendAllNotesOff(LEFT);\n    midiSendAllNotesOff(RIGHT);', '    midiSendAllNotesOff(LEFT);', 'every_note_of_both_splits'),
 ('Firmware/FirmwareInstance.inc', '  bool scaleSelectShown() override { return lightSettings == LIGHTS_ACTIVE; }', '  bool scaleSelectShown() override { return false; }', 'scale_select'),
 ('Firmware/FirmwareInstance.inc', '    state.changeFocus(step, place);\n    // the whole sequencer painted again, as cycleFocus() does, or Notes view would keep the step\n    // chosen before lit as the chosen one too\n    state.paintSequencer();\n', '    state.changeFocus(step, place);\n', 'notes_view'),
 ('Firmware/FirmwareInstance.inc', '    faders.shown = Split[split].ccFaders && !userFirmwareActive;', '    faders.shown = !userFirmwareActive;', 'cc_faders'),
 ('Firmware/FirmwareInstance.inc', '      faders.values[row] = cc > 128 ? -1 : ccFaderValues[split][cc];', '      faders.values[row] = cc > 128 ? -1 : ccFaderValues[split][row];', 'cc_faders'),
 ('Firmware/FirmwareInstance.inc', '    fxd4CurrentTempo = FXD4_FROM_INT(Project.tempo);\n    writeProjectToFlash(slot);', '    writeProjectToFlash(slot);', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '    Project = played;\n    fxd4CurrentTempo = tempo;', '    fxd4CurrentTempo = tempo;', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '    Device.lastLoadedProject = slot;\n    loadProject(slot);', '    loadProject(slot);', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '    if (!sequencerProjectValid(project) || !inRange(loaded, -1, MAX_PROJECTS - 1)) return;', '    if (!sequencerProjectValid(project) || !inRange(loaded, 0, MAX_PROJECTS - 1)) return;', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '    Device.lastLoadedProject = loaded;\n', '', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '    copy.tempo = FXD4_TO_INT(fxd4CurrentTempo);\n', '', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '    sequencersTurnOff(true);\n    Device.lastLoadedProject = slot;', '    Device.lastLoadedProject = slot;', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '      if (data.events[e].hasData() && data.events[e].getNote() == note && data.events[e].getRow() == row) return;', '      if (false) return;', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    if (!sequencerEditable(split, step) || !inRange(note, 0, 127) || !inRange(row, 1, NUMROWS - 1)) return;', '    if (!sequencerEditable(split, step) || !inRange(note, 0, 127) || !inRange(row, 0, NUMROWS - 1)) return;', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '        state.changeFocus(step, e);\n        return;', '        return;', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    data.events[last].setNewEvent(note, velocity, state.getCurrentPattern().stepSize, timbre, row);', '    data.events[0].setNewEvent(note, velocity, state.getCurrentPattern().stepSize, timbre, row);', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    velocity = constrain(velocity, 1, 127);\n', '', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    for (byte e = place + 1; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {\n      data.events[e - 1] = data.events[e];', '    for (byte e = MAX_SEQUENCER_STEP_EVENTS; e < MAX_SEQUENCER_STEP_EVENTS; ++e) {\n      data.events[e - 1] = data.events[e];', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '      case NoteValue::fineTune: event.setPitchOffset(constrain(value, -50, 50)); break;', '      case NoteValue::fineTune: event.setPitchOffset(value); break;', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '          if (seqDurationEditPanelChoices[i] == value) event.setDuration(value);', '          event.setDuration(value);', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    state.getCurrentPatternStep(step).clear();\n', '', 'sequencers_notes'),
('Firmware/FirmwareInstance.inc', '    if (!validSplit(split) || !Split[split].sequencer || !inRange(note, 0, 127)) return;', '    if (!validSplit(split) || !Split[split].sequencer) return;', 'sequencers_notes'),
('Firmware/FirmwareInstance.inc', '    state.previewEvent.sendNoteOn(event, split);\n', '', 'sequencers_notes'),
('Firmware/FirmwareInstance.inc', '      case 20: return Split[split].sendX;', '      case 20: return Split[split].sendY;', 'panels_settings_as_nrpns'),
('Firmware/FirmwareInstance.inc', '    if (displayMode == displayGlobalWithTempo) updateDisplay();\n', '', 'taps_the_tempo'),
('Firmware/FirmwareInstance.inc', '    return inRange(note, 0, 127) ? note : -1;', '    return note;', 'sequencers_notes'),
('Firmware/FirmwareInstance.inc', '    if (Split[split].sequencerView != sequencerDrums) note -= Split[split].transposePitch + Split[split].transposeOctave;\n', '', 'sequencers_notes'),
('Firmware/FirmwareInstance.inc', '    seqState[LEFT].clearAllFocus();\n    seqState[RIGHT].clearAllFocus();\n    updateDisplay();\n  }\n  // as handleSequencerProjectsHold() saves one', '    updateDisplay();\n  }\n  // as handleSequencerProjectsHold() saves one', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '    Device.lastLoadedProject = loaded;\n    seqState[LEFT].clearAllFocus();\n    seqState[RIGHT].clearAllFocus();', '    Device.lastLoadedProject = loaded;', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '    if (copy.tempo == 0) return false;', '    if (false) return false;', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '    if (copy.tempo == 0) return false;', '    if (!inRange(copy.tempo, 1, 360)) return false;', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '        if (copy.sequencer[s].seqDrumNotes[d] > 127) return false;', '        if (copy.sequencer[s].seqDrumNotes[d] > 255) return false;', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '      default: return false;', '      default: break;', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '!inRange(p.length, 1, MAX_SEQUENCER_STEPS)', '!inRange(p.length, 1, 255)', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '        if (event.hasData() && event.getDuration() == 0) return false;', '        if (false) return false;', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '    if (!patternValid(given)) return;\n', '', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '!inRange(slot, 0, MAX_PROJECTS - 1) || !sequencerProjectValid(project)', '!inRange(slot, 0, MAX_PROJECTS - 1) || project.size() != sizeof(SequencerProject)', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '    if (!sequencerProjectValid(project) || !inRange(loaded, -1, MAX_PROJECTS - 1)) return;', '    if (project.size() != sizeof(SequencerProject) || !inRange(loaded, -1, MAX_PROJECTS - 1)) return;', 'loads_saves'),
('Firmware/FirmwareInstance.inc', '  int settingsVersion() override { return Device.version; }', '  int settingsVersion() override { return Device.version - 1; }', 'loads_saves'),
 ('Firmware/FirmwareInstance.inc', '      target.events[e] = source.events[e];\n      state.steps[to].events[e].focused = false;', '      state.steps[to].events[e].focused = false;', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    if (isVisibleSequencerForSplit(split)) seqState[split].paintSequencer();', '    if (false) seqState[split].paintSequencer();', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '      read.selectedStep = s;\n', '', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    read.withNotes = Split[split].colorSequencerEvent;', '    read.withNotes = Split[split].colorSequencerEmpty;', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '    return std::vector<int>(seqDurationEditPanelChoices, seqDurationEditPanelChoices + SEQ_DURATION_EDIT_PANEL_COUNT);', '    return std::vector<int>(seqDurationEditPanelChoices, seqDurationEditPanelChoices + SEQ_DURATION_EDIT_PANEL_COUNT - 1);', 'sequencers_notes'),
 ('Firmware/FirmwareInstance.inc', '          if (found == row) return min(max(i + 48, 0), 127);', '          if (found == row + 1) return min(max(i + 48, 0), 127);', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '        return Project.sequencer[split].seqDrumNotes[row];', '        return Project.sequencer[split].seqDrumNotes[row % 7];', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    if (!validSplit(split) || !inRange(row, 0, 13)) return -1;', '    if (!validSplit(split)) return -1;', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    seqState[split].nextPosition = step;\n', '', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    read.forward = state.advancingForward;', '    read.forward = false;', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    Global.currentPerSplit = split;\n    setDisplayMode(displayNormal);', '    setDisplayMode(displayNormal);', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    if (view == sequencerNotes && !state.hasFocus() && !state.isBeingTurnedOff) state.changeFocus(0, -1);', '', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    if (pageStart < 0 || pageStart >= MAX_SEQUENCER_STEPS || pageStart % SEQ_EVENTS_WIDTH != 0) return;', '    if (pageStart < 0 || pageStart >= MAX_SEQUENCER_STEPS) return;', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    Project.sequencer[toSplit].patterns[to] = Project.sequencer[fromSplit].patterns[from];', '    Project.sequencer[toSplit].patterns[from] = Project.sequencer[fromSplit].patterns[from];', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '      case StepSixteenthDotted: case StepEighthDotted: case StepFourthDotted: swing = false; break;', '      case StepSixteenthDotted: case StepEighthDotted: case StepFourthDotted: break;', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '      default: return;\n    }\n    StepSequencerState& state = seqState[split];', '    }\n    StepSequencerState& state = seqState[split];', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    if (!validSplit(split) || !Split[split].sequencer || !inRange(length, 1, MAX_SEQUENCER_STEPS)) return;', '    if (!validSplit(split) || !Split[split].sequencer) return;', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    if (validSplit(split) && seqState[split].muted != muted) sequencerToggleMute(split);', '    if (validSplit(split)) sequencerToggleMute(split);', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    memcpy((uint8_t*)&Project.sequencer[split].patterns[pattern], data.data(), data.size());\n', '', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    seqState[split].getCurrentPattern().loopScreen = loop;', '    seqState[split].getCurrentPattern().loopScreen = false;', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    seqState[split].getCurrentPattern().sequencerDirection = (SequencerDirection)direction;', '    seqState[split].getCurrentPattern().sequencerDirection = sequencerForward;', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '    Project.sequencer[split].patterns[pattern].clear();\n    if (isVisibleSequencerForSplit(split) && seqState[split].currentPattern == pattern) {', '    if (isVisibleSequencerForSplit(split) && seqState[split].currentPattern == pattern) {', 'sequencers_patterns'),
 ('Firmware/FirmwareInstance.inc', '  int mainColor(int split) override { return Split[split].colorMain; }', '  int mainColor(int split) override { return Split[split].colorAccent; }', 'values_and_the_small_font'),
 ('Firmware/FirmwareInstance.inc', '  int settingsColor() override { return globalColor; }', '  int settingsColor() override { return 3; }', 'values_and_the_small_font'),
 ('Firmware/FirmwareInstance.inc', '    if (held.touched == untouchedCell || held.lastTouch == 0) return -1;', '    if (held.touched == untouchedCell) return -1;', 'times_a_hold'),
 ('Firmware/FirmwareInstance.inc', '      case Hold::edit: return EDIT_MODE_HOLD_DELAY;', '      case Hold::edit: return CONFIRM_HOLD_DELAY;', 'times_a_hold'),
 ('Firmware/FirmwareInstance.inc', '    return (int) calcTimeDelta(millis(), held.lastTouch);', '    return 0;', 'times_a_hold'),
 ('Engine.cpp', '    for (auto& change : due) change(*instance);\n', '', 'bend_range_as_its_screen'),
 ('Firmware/FirmwareInstance.inc', '    midiSendMpePitchBendRange(split);\n    updateDisplay();', '    updateDisplay();', 'bend_range_as_its_screen'),
 ('Firmware/FirmwareInstance.inc', '    if (!inRange(semitones, 1, 96)) return;\n', '', 'bend_range_as_its_screen'),
 ('Firmware/FirmwareInstance.inc', '      case displayBendRange: return ValueScreen::bendRange;', '      case displayBendRange: return ValueScreen::none;', 'bend_range_as_its_screen'),
 ('Firmware/FirmwareInstance.inc', '      case 25: return Split[split].expressionForY == timbreCC1 ? 1 : Split[split].customCCForY;', '      case 25: return Split[split].customCCForY;', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '      case 54: return Split[split].minForY;', '      case 54: return Split[split].minForZ;', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '      case 253: return Global.customRowOffset == -17 ? 33 : Global.customRowOffset + 16;', '      case 253: return Global.customRowOffset;', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '      case 230: return Global.switchAssignment[SWITCH_FOOT_L];', '      case 230: return Global.switchAssignment[SWITCH_FOOT_R];', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '    if (p >= 263 && p <= 270) return Global.guitarTuning[p - 263];', '    if (p >= 263 && p <= 270) return Global.guitarTuning[270 - p];', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '      p -= 100;\n      split = RIGHT;\n    }\n    switch (p) {\n      case 19: return getBendRange(split);', '      p -= 100;\n    }\n    switch (p) {\n      case 19: return getBendRange(split);', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '    receivedNrpn(parameter, value, Split[parameter >= 100 && parameter < 200 ? RIGHT : LEFT].midiChanMain);\n', '', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '  int selectedSwitch() override { return switchSelect; }', '  int selectedSwitch() override { return 3; }', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '      case displaySequencerDrum0107: case displaySequencerDrum0814: return ValueScreen::drumNotes;', '      case displaySequencerDrum0107: return ValueScreen::drumNotes;', 'the_drum_notes'),
 ('Firmware/FirmwareInstance.inc', '    if (drum < 0 || drum >= 14 || !inRange(note, 0, 127)) return;\n', '', 'the_drum_notes'),
 ('Firmware/FirmwareInstance.inc', '        if (s.midiChanSet[c]) read.noteChannels.push_back(c + 1);\n', '', 'note_sent_on_a_splits'),
 ('Firmware/FirmwareInstance.inc', '      for (int row = 0; row < NUMROWS; ++row) read.noteChannels.push_back((s.midiChanPerRow - 1 + row) % 16 + 1);', '      read.noteChannels.push_back(s.midiChanPerRow);', 'note_sent_on_a_splits'),
 ('Firmware/FirmwareInstance.inc', '    read.mode = s.midiMode;', '    read.mode = 0;', 'note_sent_on_a_splits'),
 ('Firmware/FirmwareInstance.inc', '    read.splitOn = Global.splitActive;', '    read.splitOn = false;', 'note_sent_on_a_splits'),
 ('Firmware/FirmwareInstance.inc', '      case displaySequencerColors: return ValueScreen::sequencerColours;\n', '', 'the_drum_notes'),
 ('Firmware/FirmwareInstance.inc', '    if (!validSplit(split) || !inRange(colour, COLOR_RED, COLOR_PINK) || colour == COLOR_BLACK) return;', '    if (!validSplit(split) || !inRange(colour, COLOR_OFF, COLOR_PINK)) return;', 'the_drum_notes'),
 ('Firmware/FirmwareInstance.inc', '    if (which < 0 || which > 2) return;\n    *colours[which] = colour;', '    *colours[which < 0 || which > 2 ? 0 : which] = colour;', 'the_drum_notes'),
 ('Firmware/FirmwareInstance.inc', '  int drumNote(int split, int drum) override { return Project.sequencer[split].seqDrumNotes[drum]; }', '  int drumNote(int split, int drum) override { return Project.sequencer[split].seqDrumNotes[13 - drum]; }', 'the_drum_notes'),
 ('Firmware/FirmwareInstance.inc', '      case displayEditAudienceMessage: return ValueScreen::audienceMessage;', '', 'audience_messages'),
 ('Firmware/FirmwareInstance.inc', '    int length = (int) kept.copy(made, 30);', '    int length = (int) kept.copy(made, 31);', 'audience_messages'),
 ('Firmware/FirmwareInstance.inc', "      if (c >= ' ' && c <= '~') kept += c;", '      kept += c;', 'audience_messages'),
 ('Firmware/FirmwareInstance.inc', "    while (!text.empty() && text.back() == ' ') text.pop_back();\n", '', 'audience_messages'),
 ('Firmware/FirmwareInstance.inc', '    big_scroll_text_flipped(Device.audienceMessages[message], Split[LEFT].colorMain);\n', '', 'audience_messages'),
 ('Firmware/FirmwareInstance.inc', '  Glyph bigGlyph(char c) override { return glyphOf(bigFont, c); }', '  Glyph bigGlyph(char c) override { return glyphOf(smallFont, c); }', 'audience_messages'),
 ('Firmware/FirmwareInstance.inc', '    Global.setSwitchAssignment(which, assignment, true);', '    Global.switchAssignment[which] = assignment;', 'switches_as_the_grid'),
 ('Firmware/FirmwareInstance.inc', '    if (which < 0 || which >= 5 || !inRange(assignment, ASSIGNED_OCTAVE_DOWN, MAX_ASSIGNED)) return;', '    if (which < 0 || which >= 5) return;', 'switches_as_the_grid'),
 ('Firmware/FirmwareInstance.inc', '    (sustain ? Global.ccForSwitchSustain : Global.ccForSwitchCC65)[which] = cc;', '    Global.ccForSwitchSustain[which] = cc;', 'switches_as_the_grid'),
 ('Firmware/FirmwareInstance.inc', '    if (which < 0 || which >= 5 || !inRange(cc, 0, 127)) return;', '    if (which < 0 || which >= 5) return;', 'switches_as_the_grid'),
 ('Firmware/FirmwareInstance.inc', '    memcpy(Device.audienceMessages[message], made, sizeof(made));', '    memcpy(Device.audienceMessages[(message + 1) % 16], made, sizeof(made));', 'audience_messages'),
 ('Firmware/FirmwareInstance.inc', '      case 58: return Split[split].ccForZ14Bit;', '      case 58: return Split[split].sendZ;', 'panels_settings_as_nrpns'),
 ('Firmware/FirmwareInstance.inc', '  int shownSplit() override { return Global.currentPerSplit; }', '  int shownSplit() override { return 0; }', 'values_and_the_small_font'),
 ('Firmware/FirmwareInstance.inc', '  int program(int split) override { return midiPreset[split] + 1; }', '  int program(int split) override { return midiPreset[split]; }', 'values_and_the_small_font'),
 ('Firmware/FirmwareInstance.inc', '  int volume(int split) override { return ccFaderValues[split][7]; }', '  int volume(int split) override { return ccFaderValues[split][1]; }', 'values_and_the_small_font'),
 ('Firmware/FirmwareInstance.inc', 'emplace_back(character->data + row * character->width, character->width);', 'emplace_back(character->data + row, character->width);', 'values_and_the_small_font'),
 ('Engine.cpp', '    if (t.releaseIn > 0) t.z = t.shared * t.releaseIn / slideScans;', '    if (t.releaseIn > 0) {}', 'keeps_the_pressure_steady'),
 ('Engine.cpp', 't.z = juce::jmax(1, t.z - t.shared * juce::jmax(0, --t.sharedIn - slideLead) / slideScans);', '--t.sharedIn;', 'keeps_the_pressure_steady'),
 ('Engine.cpp', 't.z = juce::jmax(1, t.z - t.shared * juce::jmax(0, --t.sharedIn - slideLead) / slideScans);', 't.z -= t.shared * juce::jmax(0, --t.sharedIn - slideLead) / slideScans;', 'keeps_the_pressure_steady'),
 ('Engine.h', 'static constexpr int slideLead = 1;', 'static constexpr int slideLead = 0;', 'keeps_the_pressure_steady'),
 ('TouchTracker.cpp', 'if (other != source && held.col == cell.col && held.row == cell.row) return true;', 'if (false) return true;', 'stays_down_while_another'),
 ('TouchTracker.cpp', 'if (other != source && held.col == cell.col && held.row == cell.row) return true;', 'if (false) return true;', 'sliding_off_a_shared'),
 ('TouchTracker.cpp', '    press(cell);\n    Cell left = from;\n    from = cell;\n    latched.erase(left.col * 8 + left.row);\n    lift(source, left);', '    Cell left = from;\n    from = cell;\n    latched.erase(left.col * 8 + left.row);\n    lift(source, left);\n    press(cell);', 'a_slide_presses'),
 ('TouchTracker.cpp', '  if (latch) latched.insert(key);', '', 'latched_cell_stays_down'),
 ('LinnLookAndFeel.cpp', '  boldFace() = bold;\n', '', 'painted_bold_text_is_mulish_bold'),
 ('Engine.h', 'return id.startsWith("VIRTUAL") ? id.substring(7) : id; }', 'return id; }', 'listed_by_their_identifiers'),
 ('Engine.h', 'return input != nullptr ? listed(input->getIdentifier())', 'return input != nullptr ? listed(output->getIdentifier())', 'listed_by_their_identifiers'),
 ('Engine.h', 'return 150.0 * std::pow(1100.0 / 150.0, (strength - 1) / 11.0); }', 'return 150.0 * std::pow(1000.0 / 150.0, (strength - 1) / 11.0); }', 'whatever_the_pressure'),
 ('Firmware/FirmwareInstance.inc', '      for (byte c = 0; c < MAXCOLS; ++c) Z_BIAS[r][c] = Z_BIAS_MULTIPLIER;\n', '', 'strikes_alike'),
 ('Engine.cpp', '      t.rising = t.landing;\n', '      t.rising = juce::jmin(t.landing, target);\n', 'whatever_the_pressure'),
 ('Engine.cpp', '      double left = 1.0 - std::pow(t.lifted / liftMillis, 2.0);', '      double left = 0;', 'lowers_the_pressure'),
 ('Firmware/FirmwareInstance.inc', '    return isStatefulSwitchAssignment(Global.switchAssignment[which == 0 ? SWITCH_FOOT_L : SWITCH_FOOT_R]);', '    return isStatefulSwitchAssignment(Global.switchAssignment[SWITCH_FOOT_R]);', 'foot_switches_latch'),
 ('Firmware/FirmwareInstance.inc', '  void liftSensorLimits(bool lifted) override { sensorLimitsLifted = lifted; }', '  void liftSensorLimits(bool) override {}', 'fourth_touch_into_a_column'),
 ('Engine.h', 'return 1000.0 / (800.0 * std::pow(15.0 / 800.0,', 'return 1000.0 / (80.0 * std::pow(15.0 / 80.0,', 'rises_slowly'),
 ('Engine.h', 'return 1000.0 / (800.0 * std::pow(15.0 / 800.0,', 'return 1000.0 / (800.0 * std::pow(150.0 / 800.0,', 'rises_slowly'),
 ('Engine.h', ' * std::pow(pressure / 1000.0, 1.5);', ' * pressure / 1000.0;', 'rises_slowly'),
 ('Firmware/FirmwareInstance.inc', '        for (int row = 0; row < NUMROWS; ++row) channels.push_back((s.midiChanPerRow - 1 + row) % 16 + 1);', '        channels.push_back(s.midiChanPerRow);', 'lights_its_pads'),
 ('Firmware/FirmwareInstance.inc', '      case oneChannel: return { s.midiChanMain };', '      case oneChannel: return { s.midiChanMain % 16 + 1 };', 'lights_its_pads'),
 ('Firmware/FirmwareInstance.inc', '          if (s.midiChanSet[c]) return { c + 1 };', '          if (!s.midiChanSet[c]) return { c + 1 };', 'lights_its_pads'),
 ('TouchTracker.cpp', '    latched.insert(key);\n    press(cell);', '    press(cell);', 'keyboard_toggles'),
 ('Surface.cpp', '  grabKeyboardFocus();\n  keyboardNavigating = false;', '  grabKeyboardFocus();', 'focus_ring'),
 ('Engine.cpp', '    if (t.releaseIn > 0 && --t.releaseIn == 0) {', '    if (false) {', 'bends_the_same_note'),
 ('TouchTracker.cpp', '  if (neighbour && slide &&', '  if (false && neighbour && slide &&', 'into_the_next_cell_of_a_row'),
 ('TouchTracker.cpp', '  bool neighbour = cell.row == from.row && std::abs(cell.col - from.col) == 1;', '  bool neighbour = true;', 'another_row_or_further'),
 ('TouchTracker.cpp', '  if (neighbour && slide && !heldByAnother(source, from) &&', '  if (neighbour && slide &&', 'another_finger_holds_does_not_slide'),
 ('Firmware/FirmwareInstance.inc', '    int x = first + (col - 1) * width + (int)((fraction - 0.5f) * width);', '    int x = first + (col - 1) * width + (int)(fraction * width);', 'follows_the_finger'),
 ('Firmware/FirmwareInstance.inc', '    int x = first + (col - 1) * width + (int)((fraction - 0.5f) * width);', '    int x = first + (col - 1) * width + (int)(fraction * width);', 'glides_from_one_note'),
 ('Engine.cpp', '    if (!t.ownPressure && t.targetZ > 0 && t.releaseIn == 0) t.targetZ = rawZ;', '', 'already_held'),
 ('Firmware/FirmwareInstance.inc', '  bool showingSettings() override { return displayMode != displayNormal; }', '  bool showingSettings() override { return false; }', 'settings_screen'),
 ('Surface.cpp', '  return model == 128 ? plate128 : plate200;', '  return plate200;', 'nameplate_is_the_panels'),
 ('Surface.cpp', '  return model == 128 ? alone128 : alone200;', '  return alone200;', 'nameplate_is_the_panels'),
 ('Surface.cpp', '  return model == 128 ? alone128 : alone200;', '  return nameplate(model);', 'nameplate_is_the_panels'),
 ('Firmware/FirmwareInstance.inc', '          Device.calRows[NUMCOLS][row].fxdMeasuredX = FXD_MAKE(4095);', '', 'every_unit_is_calibrated'),
 ('Firmware/FirmwareInstance.inc', '      Device.calibrated = true;\n      storeSettings();\n    }\n  }', '      storeSettings();\n    }\n  }', 'every_unit_is_calibrated'),
 ('Firmware/FirmwareInstance.inc', '  int tempo() override { return FXD4_TO_INT(fxd4CurrentTempo); }', '  int tempo() override { return 120; }', 'reports_the_tempo'),
 ('Firmware/FirmwareInstance.inc', '  int bendRange() override { return getBendRange(LEFT); }', '  int bendRange() override { return 12; }', 'bend_range'),
 ('Engine.cpp', '  if (image.empty() && !upgraded && onFreshFlash) onFreshFlash();', '  if (!upgraded && onFreshFlash) onFreshFlash();', 'fresh_flash'),
 ('Engine.cpp', '  finishing = true;\n', '', 'fresh_flash'),
 ('Firmware/FirmwareBridge.cpp', '    if (p.all[i] == instance) p.taken[i] = false;', '', 'handed_to_one_engine'),
 ('Engine.cpp', '  if (instance == nullptr) throw std::runtime_error("every copy of the firmware is in use");', '', 'handed_to_one_engine'),
 ('Engine.h', '  static constexpr double microsPerCell = 27.16, microsPerTouch = 580;', '  static constexpr double microsPerCell = 27.16, microsPerTouch = 0;', 'as_long_as_on_the_instrument'),
 ('Engine.cpp', '      std::this_thread::sleep_until(start + std::chrono::microseconds((long long) (length * (slice + 1) / slices)));', '      std::this_thread::sleep_for(std::chrono::microseconds(1500 / slices));', 'scans_at_the_instruments_rate'),
 ('Engine.cpp', '    double length = scanMicros(cells, held);', '    double length = scanMicros(cells, held) * 0.85;', 'scans_at_the_instruments_rate'),
 ('Engine.cpp', '    double length = scanMicros(cells, held);', '    double length = scanMicros(cells, held) * 1.2;', 'scans_at_the_instruments_rate'),
 ('Firmware/FirmwareInstance.inc', '      case displayPerSplit: return firmware::perSplitScreen;', '      case displayPerSplit: return firmware::globalScreen;', 'which_settings_screen'),
 ('Firmware/FirmwareInstance.inc', '      case displayPreset: return firmware::presetScreen;', '', 'which_settings_screen'),
 ('Firmware/FirmwareInstance.inc', '      case displayNormal: return Split[Global.currentPerSplit].sequencer && !userFirmwareActive ? firmware::sequencerScreen : firmware::noScreen;', '      case displayNormal: return firmware::noScreen;', 'sequencer_and_its_settings'),
 ('Firmware/FirmwareInstance.inc', '      case displaySequencerProjects: return firmware::sequencerProjectsScreen;', '', 'sequencer_and_its_settings'),
 ('Firmware/FirmwareInstance.inc', '  bool sequencerNoteSelected() override { return seqState[Global.currentPerSplit].hasFocusEvent(); }', '  bool sequencerNoteSelected() override { return false; }', 'sequencer_and_its_settings'),
 ('Surface.cpp', '    if (onPress) onPress(c.col, c.row);\n', '', 'press_is_told'),
 ('TouchTracker.cpp', '    if (!heldByAnother(-1, cell)) release(cell.col, cell.row);', '    release(cell.col, cell.row);', 'lets_go_only'),
 ('Engine.cpp', '  to.liftFirst = existing != touches.end();', '  to.liftFirst = false;', 'still_lifting'),
 ('Engine.cpp', '      if (instance->cellHeld(t.col, t.row) && t.liftWaited <= liftWaitMillis) {', '      if (false) {', 'quick_double_tap'),
 ('Engine.cpp', '  if (it->second.liftFirst) it->second.releaseSeen = true;\n  else it->second.targetZ = 0;', '  it->second.targetZ = 0;', 'quick_double_tap'),
 ('Engine.cpp', '  t.liftFirst = true;\n  auto it = touches', '  t.liftFirst = false;\n  auto it = touches', 'quick_double_tap'),
 ('Engine.cpp', ' || fromCol < 1 || toCol < 1) return false;', ') return false;', 'stays_among_the_pads'),
 ('Surface.cpp', '  c.col = gutter.contains(p) ? 0 : juce::jlimit(1, columns - 1, 1 + (int) ((p.x - padsArea.getX()) / (padSize + gap)));', '  c.col = gutter.contains(p) ? 0 : 1;', 'lands_on_the_pad'),
 ('Firmware/FirmwareBridge.cpp', '    if (!p.taken[i]) {', '    if (i == 0) {', 'side_by_side', 'copies'),
 ('Engine.cpp', '  if (virtualPorts) {\n    output', '  if (true) {\n    output', 'engine_without_ports'),
 ('Engine.cpp', 'instance->settingsOffset())) image = bootFlash;', 'instance->settingsOffset())) {}', 'engine_without_ports'),
 ('Engine.cpp', '  if (!flashInFile) {\n    size_t size', '  if (!flashInFile && bootFlash.size() == firmware::flashSize()) {\n    size_t size', 'engine_without_ports'),
 ('Engine.cpp', '    bootFlash = keptFlash();\n', '', 'engine_without_ports'),
 ('Engine.cpp', '  return bootedFrom.empty() && live == freshLive ? std::vector<uint8_t>() : live;', '  return live;', 'engine_without_ports'),
 ('Engine.cpp', '      takenFlash = flashInFile ? instance->flashImage() : keptFlash();', '      takenFlash = instance->flashImage();', 'engine_without_ports'),
 ('Engine.cpp', '    for (int i = 0; i < 100000 && instance->midiInPending(); ++i) instance->loopOnce();\n', '', 'engine_without_ports'),
 ('Firmware/FirmwareInstance.inc', '  bool midiInPending() override { return Serial.available() > 0; }', '  bool midiInPending() override { return false; }', 'engine_without_ports'),
 ('Firmware/FirmwareInstance.inc', '    memcpy(slots + (marker == 0 ? sizeof(Configuration) : 0), live.data(), sizeof(Configuration));\n', '', 'hasnt_stored'),
 ('Firmware/FirmwareInstance.inc', '    image[SETTINGS_OFFSET] = marker == 0 ? 1 : 0;\n', '', 'hasnt_stored'),
 ('Firmware/FirmwareInstance.inc', 'sizeof(Configuration)) == 0) return image;', 'sizeof(Configuration)) == 0 && false) return image;', 'hasnt_stored'),
 ('Firmware/FirmwareInstance.inc', '    memcpy(live.data() + offsetof(Configuration, project) + offsetof(SequencerProject, tempo), &tempo, sizeof(tempo));\n', '', 'tempo_it_plays_at'),
 ('Engine.cpp', '  if (!flashInFile) return;\n', '', 'engine_without_ports'),
 ('Engine.cpp', '      if (flashTaken.wait(5)) return takenFlash;', '      if (flashTaken.wait(5)) return {};', 'engine_without_ports'),
 ('Firmware/FirmwareInstance.inc', '    midiClockStatus = midiClockOff;\n', '', 'forgets_the_clock'),
 ('Firmware/FirmwareInstance.inc', '    fxd4CurrentTempo = FXD4_FROM_INT(120);\n', '', 'forgets_the_clock'),
 ('Firmware/FirmwareInstance.inc', '    lastNrpnMsb = 127;\n    lastNrpnLsb = 127;\n', '', 'forgets_the_clock'),
 ('Firmware/FirmwareInstance.inc', '    lastRpnMsb = 127;\n    lastRpnLsb = 127;\n', '', 'forgets_the_clock'),
 ('Firmware/FirmwareInstance.inc', '    if (copy.empty() || !upgradeConfigurationSettings((int32_t)copy.size(), copy.data())) return false;', '    return false;', 'another_layout'),
 ('Engine.cpp', '(bootFlash[offset] == 0 ? 0 : size)', 'size', 'another_layout'),
 ('Engine.cpp', '    else if ((size == 0 || size == instance->settingsSize()) && (offset == 0 || offset == instance->settingsOffset())) image = bootFlash;', '    else if (true) image = bootFlash;', 'another_layout'),
 ('Engine.cpp', '  bool upgraded = !settings.empty() && instance->upgradeSettings(settings);', '  bool upgraded = false;', 'another_layout'),
 ('Engine.cpp', '  if (image.empty() && !upgraded && onFreshFlash) onFreshFlash();', '  if (image.empty() && onFreshFlash) onFreshFlash();', 'another_layout'),
 ('Engine.cpp', '    bootSettingsSize = bootSettingsOffset = 0;\n', '', 'another_layout'),
 ('Engine.cpp', '  if (image.empty() && !upgraded && !flashInFile) {', '  if (image.empty() && !flashInFile) {', 'another_layout'),
 ('Surface.cpp', '  return juce::Colour(0xff111111);', '  return juce::Colour(0xff2a2a2a);', 'reads_on_every_light'),
 ('Engine.cpp', '  else if (virtualPorts) text +=', '  else text +=', 'engine_without_ports'),
 ('Engine.cpp', '  return flashInFile ? instance->flashImage() : keptFlash();', '  return flashInFile ? instance->flashImage() : bootFlash;', 'whole_and_prompt'),
]
only = sys.argv[1] if len(sys.argv) > 1 else ''
bad = 0
for f, old, new, test, *mark in M:
    copies = mark == ['copies']
    if only and only not in test: continue
    path = src + f
    text = open(path).read()
    assert text.count(old) == 1, (f, old)
    shutil.copy(path, path + '.bak')
    try:
        # the build compares times to the second, so a change in the same second as the last
        # build would look like no change at all
        time.sleep(1.1)
        open(path, 'w').write(text.replace(old, new))
        b = build(copies)
        if b.returncode != 0:
            print(f'BUILD FAILED  {test}  {f}: {old.strip().splitlines()[0][:70]}'); bad += 1; continue
        try:
            r = subprocess.run([build_dir(copies) + '/VirtualTests_artefacts/Release/VirtualTests', test], capture_output=True, text=True, timeout=120)
        except subprocess.TimeoutExpired:
            print('TIMED OUT   ' + test); bad += 1; continue
        # caught only by a failed check, never by a crash, which proves nothing about the test
        failed = re.search(r'(\d+) failed checks', r.stdout)
        ok = failed is not None and int(failed[1]) > 0
        print(('caught      ' if ok else 'SURVIVED    ') + test + ('' if ok else f'  {f}: {old.strip().splitlines()[0][:70]}'))
        bad += 0 if ok else 1
    finally:
        time.sleep(1.1)
        shutil.move(path + '.bak', path)
        os.utime(path, None)   # newer than the mutated build, so the next build compiles it again
ran = [x for x in M if not only or only in x[3]]
# the builds the mutations touched, rebuilt from the restored files
if any(len(x) == 4 for x in ran): build(False)
if any(len(x) > 4 for x in ran): build(True)
if not ran: print('no mutation matches ' + only); sys.exit(1)
print('all caught' if bad == 0 else f'{bad} not caught')
