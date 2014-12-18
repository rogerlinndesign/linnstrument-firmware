/************************** ls_displayModes: LinnStrument display modes drawing *******************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
There are 13 different display modes.

These are the possible values of the global variable displayMode:

displayNormal            : normal performance display
displayPerSplit          : per-split settings (left or right split)
displayPreset            : preset number
displayVolume            : volume
displayOctaveTranspose   : octave and transpose settings
displaySplitPoint        : split point
displayGlobal            : global settings
displayGlobalWithTempo   : global settings with tempo
displayOsVersion         : version number of the OS
displayCalibration       : calibration process
displayReset             : global reset confirmation and wait for touch release
displayCCForY            : custom CC number selection for Y expression
displayCCForZ            : custom CC number selection for Z expression

These routines handle the painting of these display modes on LinnStument's 208 LEDs.
**************************************************************************************************/


unsigned long tapTempoLedOn = 0;       // indicates when the tap tempo clock led was turned on

// clearDisplay:
// Turns all LEDs off in columns 1 or higher
void clearDisplay() {
  for (byte col = 1; col < NUMCOLS; ++col) {
    for (byte row = 0; row < NUMROWS; ++row) {
      setLed(col, row, 0, 0);
    }
  }
}

// updateDisplay:
// updates columns 1=25 of the LED display based on the current displayMode setting:
// 0:normal, 1:perSplit, 2:preset, 3:volume, 4:transpose, 5:split, 6:global
void updateDisplay() {
  if (scrollingActive) {
    return;
  }

  switch (displayMode)
  {
  case displayNormal:            // Display the normal and accent note colors...
  case displaySplitPoint:        // Split point display (which is just the normal display)
    paintNormalDisplay();
    break;
  case displayPerSplit:          // Display per-split settings
    paintPerSplitDisplay(Global.currentPerSplit);
    break;
  case displayPreset:            // Display this split's preset number
    paintPresetDisplay(Global.currentPerSplit);
    break;
  case displayOsVersion:         // Display the OS version
    paintOSVersionDisplay();
    break;
  case displayVolume:            // Display this split's volume value
    paintVolumeDisplay(Global.currentPerSplit);
    break;
  case displayOctaveTranspose:   // Display the octave shift of both splits, and the global pitch and light shift
    paintOctaveTransposeDisplay(Global.currentPerSplit);
    break;
  case displayGlobal:            // Display global settings
  case displayGlobalWithTempo:
    paintGlobalSettingsDisplay();
    break;
  case displayCalibration:       // Display the calibration pattern
    paintCalibrationDisplay();
    break;
  case displayReset:             // Display the reset information
    paintResetDisplay();
    break;
  case displayCCForY:            // Display this split's Y CC number
    paintCCForYDisplay(Global.currentPerSplit);
    break;
  case displayCCForZ:            // Display this split's Z CC number
    paintCCForZDisplay(Global.currentPerSplit);
    break;
  }

  updateSwitchLeds();
}

void updateSwitchLeds() {
  setLed(0, SWITCH_1_ROW, globalColor, switchState[SWITCH_SWITCH_1][focusedSplit] * 3);
  setLed(0, SWITCH_2_ROW, globalColor, switchState[SWITCH_SWITCH_2][focusedSplit] * 3);
  if (splitActive) {
    setLed(0, SPLIT_ROW, Split[focusedSplit].colorMain, 3);
  }
  else {
    setLed(0, SPLIT_ROW, globalColor, 0);
  }
}

// paintNormalDisplay:
// Paints columns 1-26 of the display with the normal performance colors
void paintNormalDisplay() {

  // determine the splits and divider
  int split = Global.currentPerSplit;
  int divider = NUMCOLS;
  if (splitActive || displayMode == displaySplitPoint) {
    split = LEFT;
    divider = Global.splitPoint;
  }

  paintNormalDisplaySplit(split, 1, divider);
  if (divider != NUMCOLS) {
    paintNormalDisplaySplit(RIGHT, divider, NUMCOLS);
  }

  // light the octave/transpose switch if the pitch is transposed
  if ((Split[LEFT].transposePitch < 0 && Split[RIGHT].transposePitch < 0) ||
      (Split[LEFT].transposePitch < 0 && Split[RIGHT].transposePitch == 0) ||
      (Split[LEFT].transposePitch == 0 && Split[RIGHT].transposePitch < 0)) {
    setLed(0, OCTAVE_ROW, COLOR_RED, 3);
  }
  else if ((Split[LEFT].transposePitch > 0 && Split[RIGHT].transposePitch > 0) ||
           (Split[LEFT].transposePitch > 0 && Split[RIGHT].transposePitch == 0) ||
           (Split[LEFT].transposePitch == 0 && Split[RIGHT].transposePitch > 0)) {
    setLed(0, OCTAVE_ROW, COLOR_GREEN, 3);
  }
  else if (Split[LEFT].transposePitch != 0 && Split[RIGHT].transposePitch != 0) {
    setLed(0, OCTAVE_ROW, COLOR_YELLOW, 3);
  }
  else {
    clearLed(0, OCTAVE_ROW);
  }
}

void paintNormalDisplaySplit(byte split, byte leftEdge, byte rightEdge) {
  for (byte row = 0; row < NUMROWS; ++row) {
    if (Split[split].ccFaders) {
      paintCCFaderDisplayRow(split, row);
    }
    else if (isStrummingSplit(split)) {
      for (byte col = leftEdge; col < rightEdge; ++col) {
        paintStrumDisplayCell(split, col, row);
      }
    }
    else {
      for (byte col = leftEdge; col < rightEdge; ++col) {
        paintNormalDisplayCell(split, col, row);
      }
    }
  }
}

void paintCCFaderDisplayRow(byte split, byte row) {
  byte faderLeft, faderLength;
  determineFaderBoundaries(split, faderLeft, faderLength);

  // when the fader only spans one cell, it acts as a toggle
  if (faderLength == 0) {
      if (ccFaderValues[split][row] > 0) {
        setLed(faderLeft, row, Split[split].colorMain, 3);
      }
      else {
        clearLed(faderLeft, row);
      }
  }
  // otherwise calculate the fader position based on its value and light the appropriate leds
  else {
    int32_t fxdFaderPosition = fxdCalculateFaderPosition(ccFaderValues[split][row], faderLeft, faderLength);

    for (byte col = faderLength + faderLeft; col >= faderLeft; --col ) {
      if (Global.calRows[col][0].fxdReferenceX - CALX_HALF_UNIT > fxdFaderPosition) {
        clearLed(col, row);
      }
      else {
        setLed(col, row, Split[split].colorMain, 3);
      }
    }
  }
}

void paintStrumDisplayCell(byte split, byte col, byte row) {
  // by default clear the cell color
  byte colour = COLOR_BLACK;
  byte brightness = 0;

  if (row % 2 == 0) {
    colour = Split[split].colorAccent;
    brightness = 3;
  }
  else {
    colour = Split[split].colorMain;
    brightness = 3;
  }

  // actually set the cell's color
  setLed(col, row, colour, brightness);
  checkRefreshLedColumn(micros());
}

void paintNormalDisplayCell(byte split, byte col, byte row) {
  // by default clear the cell color
  byte colour = COLOR_BLACK;
  byte brightness = 0;

  int actualnote = transposedNote(split, col, row);

  // the note is out of MIDI note range, disable it
  if (actualnote < 0 || actualnote > 127) {
    colour = COLOR_BLACK;
    brightness = 0;
  }
  else {
    int octaveNote = (getNoteNumber(col,row) - Split[split].transposeLights) % 12;

    // first paint all cells in split to its background color
    if (Global.mainNotes[octaveNote]) {
      colour = Split[split].colorMain;
      brightness = 3;
    }

    // then paint only notes marked as Accent notes with Accent color
    if (Global.accentNotes[octaveNote]) {
      colour = Split[split].colorAccent;
      brightness = 3;
    }
    
    // distinguish middle C (MIDI note number 60) - jas 2014/11/14
    // Random brightness could give some twinkle effect, but color settings
    // for LEDs in untouched cells normally do not get frequent updates,
    // so you can see the brightness differences only after touching middle C.
    if (actualnote == 60) {
      colour = Split[split].colorMiddleC;
      brightness = byte(random(3) + 1);
    }

    // if the low row is anything but normal, set it to the appropriate color
    if (row == 0 && Split[split].lowRowMode != lowRowNormal) {
      colour = Split[split].colorLowRow;
      brightness = 3;
    }
  }

  // actually set the cell's color
  setLed(col, row, colour, brightness);
  checkRefreshLedColumn(micros());
}

// paintPerSplitDisplay:
// paints all cells with per-split settings for a given split
void paintPerSplitDisplay(int side) {

  clearDisplay();

  doublePerSplit = false;  

  // set Midi Mode and channel lights
  switch (Split[side].midiMode)
  {
    case oneChannel:
      setLed(1, 7, Split[side].colorMain, 3);
      break;
    case channelPerNote:
      setLed(1, 6, Split[side].colorMain, 3);
      break;
    case channelPerRow:
      setLed(1, 5, Split[side].colorMain, 3);
      break;
  }

  switch (midiChannelSettings) {
    case MIDICHANNEL_MAIN:
      setLed(2, 7, Split[side].colorMain, 3);
      showMainMidiChannel(side);
      break;
    case MIDICHANNEL_PERNOTE:
      setLed(2, 6, Split[side].colorMain, 3);
      showPerNoteMidiChannels(side);
      break;
    case MIDICHANNEL_PERROW:
      setLed(2, 5, Split[side].colorMain, 3);
      showPerRowMidiChannel(side);
      break;
  }

  switch (Split[side].bendRange)
  {
    case 2:
      setLed(7, 7, Split[side].colorMain, 3);
      break;
    case 3:
      setLed(7, 6, Split[side].colorMain, 3);
      break;
    case 12:
      setLed(7, 5, Split[side].colorMain, 3);
      break;
    case 24:
      setLed(7, 4, Split[side].colorMain, 3);
      break;
    case 48: //-- added range for better compatibility with Haaken Continuum and colOffset>1 - jas 2014/12/11
      setLed(7, 3, Split[side].colorMain, 3);
      break;
    case 96: //-- added range for better compatibility with Haaken Continuum and colOffset>1 - jas 2014/12/11
      setLed(7, 2, Split[side].colorMain, 3);
      break;
  }

  // set Pitch/X settings
  if (Split[side].sendX == true)  {
    setLed(8, 7, Split[side].colorMain, 3);
  }

  if (Split[side].pitchCorrectQuantize == true) {
    setLed(8, 6, Split[side].colorMain, 3);
  }

  if (Split[side].pitchCorrectHold == true) {
    setLed(8, 5, Split[side].colorMain, 3);
  }

  if (Split[side].pitchResetOnRelease == true) {
    setLed(8, 4, Split[side].colorMain, 3);
  }

  // set Timbre/Y settings
  if (Split[side].sendY == true)  {
    setLed(9, 7, Split[side].colorMain, 3);
  }

  switch (Split[side].ccForY)
  {
    case 1:
      setLed(9, 6, Split[side].colorMain, 3);
      break;
    case 74:
      setLed(9, 5, Split[side].colorMain, 3);
      break;
    default:
      setLed(9, 3, Split[side].colorMain, 3);
      break;
  }

  if (Split[side].relativeY == true)
  {
      setLed(9, 4, Split[side].colorMain, 3);
  }

  // set Loudness/Z settings
  if (Split[side].sendZ == true)  {
    setLed(10, 7, Split[side].colorMain, 3);
  }

  switch (Split[side].expressionForZ)
  {
    case loudnessPolyPressure:
      setLed(10, 6, Split[side].colorMain, 3);
      break;
    case loudnessChannelPressure:
      setLed(10, 5, Split[side].colorMain, 3);
      break;
    case loudnessCC:
      if (Split[side].ccForZ == 11) {
        setLed(10, 4, Split[side].colorMain, 3);
      }
      else {
        setLed(10, 3, Split[side].colorMain, 3);
      }
      break;
  }

  // Set "Color" lights
  setLed(11, 7, Split[side].colorMain, 3);
  setLed(11, 6, Split[side].colorAccent, 3);
  setLed(11, 5, Split[side].colorNoteon, 3);
  setLed(11, 4, Split[side].colorLowRow, 3);
  setLed(11, 3, Split[side].colorMiddleC, 3);

  // Set "Low row" lights
  switch (Split[side].lowRowMode)
  {
    case lowRowNormal:
      setLed(12, 7, Split[side].colorMain, 3);
      break;
    case lowRowRestrike:
      setLed(12, 6, Split[side].colorMain, 3);
      break;
    case lowRowStrum:
      setLed(12, 5, Split[side].colorMain, 3);
      break;
    case lowRowArpeggiator:
      setLed(12, 4, Split[side].colorMain, 3);
      break;
    case lowRowSustain:
      setLed(13, 7, Split[side].colorMain, 3);
      break;
    case lowRowBend:
      setLed(13, 6, Split[side].colorMain, 3);
      break;
    case lowRowCC1:
      setLed(13, 5, Split[side].colorMain, 3);
      break;
    case lowRowCCXYZ:
      setLed(13, 4, Split[side].colorMain, 3);
      break;
  }

  // set Arpeggiator
  if (Split[side].arpeggiator == true)  {
    setLed(14, 7, Split[side].colorMain, 3);
  }

  // set CC faders
  if (Split[side].ccFaders == true)  {
    setLed(14, 6, Split[side].colorMain, 3);
  }

  // set strum
  if (Split[side].strum == true)  {
    setLed(14, 5, Split[side].colorMain, 3);
  }

  // set "show split" led
  paintShowSplitSelection(side);
}

// paint one of the two leds that indicate which split is being controlled
// (e.g. when you're changing per-split settings, or changing the preset or volume)
void paintShowSplitSelection(int side) {
  if (side == LEFT || doublePerSplit) {
    setLed(15, 7, Split[LEFT].colorMain, 3);
  }
  if (side == RIGHT || doublePerSplit) {
    setLed(16, 7, Split[RIGHT].colorMain, 3);
  }
}

void paintOSVersionDisplay() {

  clearDisplay();

  byte color = Split[LEFT].colorMain;
  smallfont_draw_string(0, 0, OSVersion, color);
}

// paint the current preset number for a particular side, in large block characters
void paintPresetDisplay(int side) {
  paintMidiDataDisplay(side, Split[side].preset+1);
}

void paintCCForYDisplay(int side) {
  paintMidiDataDisplay(side, Split[side].ccForY);
}

void paintCCForZDisplay(int side) {
  if (Split[side].expressionForZ != loudnessCC) {
    displayMode = displayPerSplit;
    updateDisplay();
  }
  else {
    paintMidiDataDisplay(side, Split[side].ccForZ);
  }
}

void paintMidiDataDisplay(int side, byte value) {

  clearDisplay();
  
  doublePerSplit = false;  

  char str[4];
  byte color = Split[side].colorMain;
  char *format = "%2d";

  if (value >= 100) {
    // Handle the "1" character specially, to get the spacing right
    smallfont_draw_string(0, 0, "1", color);
    value -= 100;
    format = "%02d";     // to make sure a leading zero is included
  }

  snprintf(str, sizeof(str), format, value);
  smallfont_draw_string(5, 0, str, color);

  paintShowSplitSelection(side);
}

// draw a horizontal line to indicate volume for a particular side
void paintVolumeDisplay(int side) {

  clearDisplay();
  
  doublePerSplit = false;  

  int32_t fxdFaderPosition = fxdCalculateFaderPosition(ccFaderValues[side][6], 1, 24);

  for (byte col = 25; col >= 1; --col ) {
    if (Global.calRows[col][0].fxdReferenceX - CALX_HALF_UNIT <= fxdFaderPosition) {
      setLed(col, 5, Split[side].colorMain, 3);
    }
  }

  paintShowSplitSelection(side);
}

void paintOctaveTransposeDisplay(int side) {

  clearDisplay();

  // Paint the octave shift value
  if (!doublePerSplit || Split[LEFT].transposeOctave == Split[RIGHT].transposeOctave) {
    paintOctave(Split[Global.currentPerSplit].colorMain, 8, OCTAVE_ROW, Split[side].transposeOctave);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposeOctave) > abs(Split[RIGHT].transposeOctave)) {
      paintOctave(Split[LEFT].colorMain, 8, OCTAVE_ROW, Split[LEFT].transposeOctave);
      paintOctave(Split[RIGHT].colorMain, 8, OCTAVE_ROW, Split[RIGHT].transposeOctave);
    }
    else {
      paintOctave(Split[RIGHT].colorMain, 8, OCTAVE_ROW, Split[RIGHT].transposeOctave);
      paintOctave(Split[LEFT].colorMain, 8, OCTAVE_ROW, Split[LEFT].transposeOctave);
    }
  }

  // Paint the pitch transpose values
  if (!doublePerSplit || Split[LEFT].transposePitch == Split[RIGHT].transposePitch) {
    paintTranspose(Split[Global.currentPerSplit].colorMain, SWITCH_1_ROW, Split[side].transposePitch);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposePitch) > abs(Split[RIGHT].transposePitch)) {
      paintTranspose(Split[LEFT].colorMain, SWITCH_1_ROW, Split[LEFT].transposePitch);
      paintTranspose(Split[RIGHT].colorMain, SWITCH_1_ROW, Split[RIGHT].transposePitch);
    }
    else {
      paintTranspose(Split[RIGHT].colorMain, SWITCH_1_ROW, Split[RIGHT].transposePitch);
      paintTranspose(Split[LEFT].colorMain, SWITCH_1_ROW, Split[LEFT].transposePitch);
    }
  }

  // Paint the light transpose values
  if (!doublePerSplit || Split[LEFT].transposeLights == Split[RIGHT].transposeLights) {
    paintTranspose(Split[Global.currentPerSplit].colorMain, SWITCH_2_ROW, Split[side].transposeLights);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposeLights) > abs(Split[RIGHT].transposeLights)) {
      paintTranspose(Split[LEFT].colorMain, SWITCH_2_ROW, Split[LEFT].transposeLights);
      paintTranspose(Split[RIGHT].colorMain, SWITCH_2_ROW, Split[RIGHT].transposeLights);
    }
    else {
      paintTranspose(Split[RIGHT].colorMain, SWITCH_2_ROW, Split[RIGHT].transposeLights);
      paintTranspose(Split[LEFT].colorMain, SWITCH_2_ROW, Split[LEFT].transposeLights);
    }
  }

  paintShowSplitSelection(side);
}

void paintOctave(byte color, byte midcol, byte row, int octave) {

  setLed(midcol, row, Split[Global.currentPerSplit].colorAccent, 3);
  if (0 == color) color = octave > 0 ? COLOR_GREEN : COLOR_RED ;

  switch (octave) {
  case -60:
    setLed(midcol-5, row, color, 2);
    // lack of break here is purposeful, we want to fall through...
  case -48:
    setLed(midcol-4, row, color, 2);
    // lack of break here is purposeful, we want to fall through...
  case -36:
    setLed(midcol-3, row, color, 3);
    // lack of break here is purposeful, we want to fall through...
  case -24:
    setLed(midcol-2, row, color, 3);
    // lack of break here is purposeful, we want to fall through...
  case -12:
    setLed(midcol-1, row, color, 3);
    break;

  case 60:
    setLed(midcol+5, row, color, 2);
    // lack of break here is purposeful, we want to fall through...
  case 48:
    setLed(midcol+4, row, color, 2);
    // lack of break here is purposeful, we want to fall through...
  case 36:
    setLed(midcol+3, row, color, 3);
    // lack of break here is purposeful, we want to fall through...
  case 24:
    setLed(midcol+2, row, color, 3);
    // lack of break here is purposeful, we want to fall through...
  case 12:
    setLed(midcol+1, row, color, 3);
    break;
  }
}

void paintTranspose(byte color, byte row, int transpose) {

  byte midcol = 8;
  setLed(midcol, row, Split[Global.currentPerSplit].colorAccent, 3);    // paint the center cell of the transpose range

  if (transpose != 0) {
    if (0 == color) color = transpose < 0 ? COLOR_RED : COLOR_GREEN;
    byte col_from = (transpose < 0) ? (midcol + transpose) : (midcol + 1);
    byte col_to = (transpose > 0) ? (midcol + transpose) : (midcol - 1);
    for (byte c = col_from; c <= col_to; ++c) {
      setLed(c, row, color, 3);
    }
  }
}


void setNoteLights(boolean* notelights) {
  if (notelights[0]) {
    lightLed(2, 0);
  }
  if (notelights[1]) {
    lightLed(3, 0);
  }
  if (notelights[2]) {
    lightLed(4, 0);
  }
  if (notelights[3]) {
    lightLed(2, 1);
  }
  if (notelights[4]) {
    lightLed(3, 1);
  }
  if (notelights[5]) {
    lightLed(4, 1);
  }
  if (notelights[6]) {
    lightLed(2, 2);
  }
  if (notelights[7]) {
    lightLed(3, 2);
  }
  if (notelights[8]) {
    lightLed(4, 2);
  }
  if (notelights[9]) {
    lightLed(2, 3);
  }
  if (notelights[10]) {
    lightLed(3, 3);
  }
  if (notelights[11]) {
    lightLed(4, 3);
  }
}

void paintSwitchAssignment(byte mode) {
  switch (mode) {
    case ASSIGNED_OCTAVE_DOWN:
      lightLed(8, 2);
      break;
    case ASSIGNED_OCTAVE_UP:
      lightLed(9, 2);
      break;
    case ASSIGNED_SUSTAIN:
      lightLed(8, 1);
      break;
    case ASSIGNED_CC_65:
      lightLed(9, 1);
      break;
    case ASSIGNED_ARPEGGIATOR:
      lightLed(8, 0);
      break;
    case ASSIGNED_ALTSPLIT:
      lightLed(9, 0);
      break;
  }
}

void updateGlobalDisplay() {
  if (displayMode == displayGlobal || displayMode ==  displayGlobalWithTempo) {
    updateDisplay();
  }
}

// paintGlobalSettingsDisplay:
// Paints LEDs with state of all global settings
void paintGlobalSettingsDisplay() {

  clearDisplay();

  switch (lightSettings) {
    case LIGHTS_MAIN:
      lightLed(1, 0);
      setNoteLights(Global.mainNotes);
      break;
    case LIGHTS_ACCENT:
      lightLed(1, 1);
      setNoteLights(Global.accentNotes);
      break;
  }

  switch (Global.rowOffset)
  {
    case 0:        // no overlap
      lightLed(5, 3);
      break;
    case 3:        // +3
      lightLed(5, 0);
      break;
    case 4:        // +4
      lightLed(6, 0);
      break;
    case 5:        // +5
      lightLed(5, 1);
      break;
    case 6:        // +6
      lightLed(6, 1);
      break;
    case 7:        // +7
      lightLed(5, 2);
      break;
    case 12:      // +octave
      lightLed(6, 2);
      break;
    case 13:      // guitar tuning
      lightLed(6, 3);
      break;
  }

  switch (Global.colOffset) //-- add this new setting to unused column 19 - jas 2014/12/11 --
  {
    case 1:        // 1 semitone (default)
      lightLed(19, 0);
      break;
    case 2:        // 2 semitone
      lightLed(19, 1);
      break;
    case 3:        // 3 semitone
      lightLed(19, 2);
      break;
    case 4:        // 4 semitone
      lightLed(19, 3);
      break;
  }

  // This code assumes that switchSelect values are the same as the row numbers
  lightLed(7, switchSelect);
  paintSwitchAssignment(Global.switchAssignment[switchSelect]);

  // This code assumes the velocitySensitivity and pressureSensitivity
  // values are equal to the LED rows.
  lightLed(10, Global.velocitySensitivity);
  lightLed(11, Global.pressureSensitivity);

  // Set the lights for the Arpeggiator settings
  switch (Global.arpDirection) {
    case ArpDown:
      lightLed(12, 0);
      break;
    case ArpUp:
      lightLed(12, 1);
      break;
    case ArpUpDown:
      lightLed(12, 0);
      lightLed(12, 1);
      break;
    case ArpRandom:
      lightLed(12, 2);
      break;
    case ArpReplayAll:
      lightLed(12, 3);
      break;
  }

  switch (Global.arpTempo) {
    case ArpSixteenthSwing:
      lightLed(13, 0);
      lightLed(13, 1);
      break;
    case ArpEighth:
      lightLed(13, 0);
      break;
    case ArpEighthTriplet:
      lightLed(13, 0);
      lightLed(13, 3);
      break;
    case ArpSixteenth:
      lightLed(13, 1);
      break;
    case ArpSixteenthTriplet:
      lightLed(13, 1);
      lightLed(13, 3);
      break;
    case ArpThirtysecond:
      lightLed(13, 2);
      break;
    case ArpThirtysecondTriplet:
      lightLed(13, 2);
      lightLed(13, 3);
      break;
  }

  // show the arpeggiator octave
  if (Global.arpOctave == 1) {
    lightLed(14, 0);
  }
  else if (Global.arpOctave == 2) {
    lightLed(14, 1);
  }

  unsigned long now = micros();

  // flash the tap tempo cell at the beginning of the beat
  if ((isMidiClockRunning() && getMidiClockCount() == 0) ||
      (!isMidiClockRunning() && getInternalClockCount() == 0 )) {
    lightLed(14, 3);
    tapTempoLedOn = now;
  }

  // handle turning off the tap tempo led after minimum 30ms
  if (tapTempoLedOn != 0 && calcTimeDelta(now, tapTempoLedOn) > LED_FLASH_DELAY) {
    tapTempoLedOn = 0;
    setLed(14, 3, COLOR_BLACK, 0);
  }

  // Show the MIDI input/output configuration
  if (Global.midiIO == 1) {
    lightLed(15, 0);       // for MIDI over USB
  } else {
    lightLed(15, 1);       // for MIDI jacks
  }

  // set light for serial mode
  if (Global.serialMode) {
    lightLed(16, 2);
  }

#ifdef DEBUG_ENABLED
  // Column 17 is for setting/showing the debug level
  // The value of debugLevel is from -1 up.
  lightLed(17, debugLevel + 1);

  // The rows in column 18 are secret switches.
  for (int ss = 0; ss < SECRET_SWITCHES; ++ss) {
    if (secretSwitch[ss]) {
      lightLed(18, ss);
    }
  }
#endif


  // Column 19 is set above, for Global.colOffset, following Global.rowOffset - jas 2014/12/11

  if (displayMode == displayGlobalWithTempo) {
    byte color = Split[LEFT].colorMain;
    char str[4];
    char *format = "%3d";
    snprintf(str, sizeof(str), format, FXD4_TO_INT(fxd4CurrentTempo));
    tinyfont_draw_string(0, 4, str, color);
  }
}

void paintCalibrationDisplay() {

  clearDisplay();

  switch (calibrationPhase) {
    case calibrationRows:
      for (int c = 1; c < NUMCOLS; ++c) {
        setLed(c, 0, COLOR_BLUE, 3);
        setLed(c, 2, COLOR_BLUE, 3);
        setLed(c, 5, COLOR_BLUE, 3);
        setLed(c, 7, COLOR_BLUE, 3);
      }
      break;
    case calibrationCols:
      for (int r = 0; r < NUMROWS; ++r) {
        setLed(1, r, COLOR_BLUE, 3);
        setLed(4, r, COLOR_BLUE, 3);
        setLed(7, r, COLOR_BLUE, 3);
        setLed(10, r, COLOR_BLUE, 3);
        setLed(13, r, COLOR_BLUE, 3);
        setLed(16, r, COLOR_BLUE, 3);
        setLed(19, r, COLOR_BLUE, 3);
        setLed(22, r, COLOR_BLUE, 3);
        setLed(25, r, COLOR_BLUE, 3);
      }
      break;
  }
}

void paintResetDisplay() {

  clearDisplay();

  smallfont_draw_string(0, 0, "RESET", globalColor, true);
  for (byte row = 0; row < NUMROWS; ++row) {
    setLed(0, row, 0, 0);
  }
}

void setMidiChannelLed(int chan, byte color) {                       // chan value is 1-16
    if (chan > 16) {
      chan -= 16;
    }
    int row = 7 - (chan - 1) / 4;
    int col = 3 + (chan - 1) % 4;
    setLed(col, row, color, 3);
}

// light per-split midi mode and single midi channel lights
void showMainMidiChannel(int side) {
  setMidiChannelLed(Split[side].midiChanMain, Split[side].colorMain);
}

void showPerRowMidiChannel(int side) {
  setMidiChannelLed(Split[side].midiChanPerRow, Split[side].colorMain);
  for (int i = 1; i < 8; ++i) {
    setMidiChannelLed(Split[side].midiChanPerRow + i, Split[side].colorMain);
  }
}

// light per-split midi mode and multi midi channel lights
void showPerNoteMidiChannels(int side) {

  for (int chan = 1; chan <= 16; ++chan) {
    if (Split[side].midiChanSet[chan-1]) {
      setMidiChannelLed(chan, Split[side].colorMain);
    }
  }
}
