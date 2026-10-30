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
 ('TouchTracker.cpp', 'if (other != source && held.col == cell.col && held.row == cell.row) return true;', 'if (false) return true;', 'stays_down_while_another'),
 ('TouchTracker.cpp', 'if (other != source && held.col == cell.col && held.row == cell.row) return true;', 'if (false) return true;', 'sliding_off_a_shared'),
 ('TouchTracker.cpp', '    press(cell);\n    Cell left = from;\n    from = cell;\n    latched.erase(left.col * 8 + left.row);\n    lift(source, left);', '    Cell left = from;\n    from = cell;\n    latched.erase(left.col * 8 + left.row);\n    lift(source, left);\n    press(cell);', 'a_slide_presses'),
 ('TouchTracker.cpp', '  if (latch) latched.insert(key);', '', 'latched_cell_stays_down'),
 ('LinnLookAndFeel.cpp', '  boldFace() = bold;\n', '', 'painted_bold_text_is_mulish_bold'),
 ('Engine.h', 'return id.startsWith("VIRTUAL") ? id.substring(7) : id; }', 'return id; }', 'listed_by_their_identifiers'),
 ('Engine.h', 'return input != nullptr ? listed(input->getIdentifier())', 'return input != nullptr ? listed(output->getIdentifier())', 'listed_by_their_identifiers'),
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
 ('Surface.cpp', '  return plate;\n}\n\njuce::Image Surface::name()', '  return {};\n}\n\njuce::Image Surface::name()', 'nameplate_is_the_panels'),
 ('Surface.cpp', '  return alone;', '  return nameplate();', 'nameplate_is_the_panels'),
 ('Firmware/FirmwareInstance.inc', '          Device.calRows[NUMCOLS][row].fxdMeasuredX = FXD_MAKE(4095);', '', 'every_unit_is_calibrated'),
 ('Firmware/FirmwareInstance.inc', '      Device.calibrated = true;\n      storeSettings();\n    }\n  }', '      storeSettings();\n    }\n  }', 'every_unit_is_calibrated'),
 ('Firmware/FirmwareInstance.inc', '  int tempo() override { return FXD4_TO_INT(fxd4CurrentTempo); }', '  int tempo() override { return 120; }', 'reports_the_tempo'),
 ('Firmware/FirmwareInstance.inc', '  int bendRange() override { return getBendRange(LEFT); }', '  int bendRange() override { return 12; }', 'bend_range'),
 ('Engine.cpp', '  if (image.empty() && onFreshFlash) onFreshFlash();', '  if (onFreshFlash) onFreshFlash();', 'fresh_flash'),
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
 ('Engine.cpp', ' || fromCol < 1 || toCol < 1) return false;', ') return false;', 'stays_among_the_pads'),
 ('Surface.cpp', '  c.col = gutter.contains(p) ? 0 : juce::jlimit(1, columns - 1, 1 + (int) ((p.x - padsArea.getX()) / (padSize + gap)));', '  c.col = gutter.contains(p) ? 0 : 1;', 'lands_on_the_pad'),
 ('Firmware/FirmwareBridge.cpp', '    if (!p.taken[i]) {', '    if (i == 0) {', 'side_by_side', 'copies'),
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
            print(f'BUILD FAILED  {test}'); bad += 1; continue
        try:
            r = subprocess.run([build_dir(copies) + '/VirtualTests_artefacts/Release/VirtualTests', test], capture_output=True, text=True, timeout=120)
        except subprocess.TimeoutExpired:
            print('TIMED OUT   ' + test); bad += 1; continue
        # caught only by a failed check, never by a crash, which proves nothing about the test
        failed = re.search(r'(\d+) failed checks', r.stdout)
        ok = failed is not None and int(failed[1]) > 0
        print(('caught      ' if ok else 'SURVIVED    ') + test)
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
