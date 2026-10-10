/*********************** ls_midiLights: LinnStrument lights and text over MIDI ********************
Copyright 2023 Roger Linn Design (https://www.rogerlinndesign.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
***************************************************************************************************
CC 20 and 21 position a cursor and CC 22 colors the cell under it. These add, on top of that:

  CC 25  color the cell under the cursor and move the cursor one cell to the right, continuing
         at column 0 of the next row up, so a whole surface is one CC per cell
  CC 26  font for the text below: 0 tiny, 1 small, 2 big, 3 condensed
  CC 27  append a character to the text, 0 clears it
  CC 28  draw the text at the cursor in the given color, then clear it
  CC 29  scroll the text across the cursor's row in the given color, then clear it
  NRPN 272 to 275 answer NRPN 299 with the model, the firmware version, its build and whether
         the instrument is calibrated

user_firmware_mode.md documents them, tests/test_userfirmware.cpp checks every claim there.
**************************************************************************************************/

#define MIDI_TEXT_MAX_CHARS   32
#define MIDI_SCROLL_STEP_MS   40

char midiText[MIDI_TEXT_MAX_CHARS + 1];
byte midiTextLength = 0;
byte midiTextFont = 2;

char midiScrollText[MIDI_TEXT_MAX_CHARS + 1];
byte midiScrollFont = 2;
byte midiScrollRow = 0;
byte midiScrollColor = 0;
byte midiScrollLayer = LED_LAYER_CUSTOM2;
int midiScrollCol = 0;
int midiScrollEnd = 0;
unsigned long midiScrollLastStep = 0;
boolean midiScrollActive = false;

struct Font* midiTextFontFor(byte index) {
  switch (index) {
    case 0: return &tinyFont;
    case 1: return &smallFont;
    case 3: return &condFont;
  }
  return &bigFont;
}

boolean midiLightsAllowed() {
  return displayMode == displayNormal || displayMode == displayCustomLedsEditor;
}

// user firmware mode lights live in their own layer, which is cleared when the mode is left
byte midiLightsLayer() {
  return userFirmwareActive ? LED_LAYER_CUSTOM2 : LED_LAYER_CUSTOM1;
}

void setMidiLed(byte col, byte row, byte color, byte layer) {
  if (color <= COLOR_PINK && color != COLOR_OFF) {
    setLed(col, row, color, cellOn, layer);
  }
  else {
    setLed(col, row, COLOR_OFF, cellOff, layer);
  }
}

// the cursor moves even while the Global Settings panel is open, so a stream stays in step
void midiPaintAndAdvance(byte color) {
  if (midiCellRowCC >= NUMROWS) return;
  if (midiLightsAllowed()) {
    setMidiLed(midiCellColCC, midiCellRowCC, color, midiLightsLayer());
  }
  midiCellColCC += 1;
  if (midiCellColCC >= NUMCOLS) {
    midiCellColCC = 0;
    midiCellRowCC += 1;
  }
}

void midiTextSetFont(byte font) {
  if (font <= 3) {
    midiTextFont = font;
  }
}

void midiTextAppend(byte character) {
  if (character == 0) {
    midiTextLength = 0;
  }
  else if (midiTextLength < MIDI_TEXT_MAX_CHARS) {
    midiText[midiTextLength++] = character;
  }
  midiText[midiTextLength] = '\0';
}

void midiTextDraw(byte color) {
  int col = midiCellColCC < 1 ? 1 : midiCellColCC;
  drawMidiText(col, midiCellRowCC, midiTextFontFor(midiTextFont), midiText, color, midiLightsLayer());
  midiTextAppend(0);
}

void midiTextScroll(byte color) {
  eraseMidiTextScroll();
  if (midiTextLength == 0) {
    return;
  }
  strcpy(midiScrollText, midiText);
  midiTextAppend(0);
  midiScrollFont = midiTextFont;
  midiScrollRow = midiCellRowCC;
  midiScrollColor = color;
  midiScrollLayer = midiLightsLayer();
  midiScrollCol = NUMCOLS;
  midiScrollEnd = -font_width_string(midiScrollText, midiTextFontFor(midiScrollFont));
  midiScrollLastStep = millis();
  midiScrollActive = true;
}

void stopMidiTextScroll() {
  midiScrollActive = false;
}

// stops a running scroll and clears the band of rows it was moving through
void eraseMidiTextScroll() {
  if (!midiScrollActive) return;
  midiScrollActive = false;
  byte height = midiTextFontFor(midiScrollFont)->height;
  for (byte col = 1; col < NUMCOLS; ++col) {
    for (byte row = midiScrollRow; row < midiScrollRow + height && row < NUMROWS; ++row) {
      setLed(col, row, COLOR_OFF, cellOff, midiScrollLayer);
    }
  }
}

void checkAdvanceMidiTextScroll(unsigned long nowMillis) {
  if (!midiScrollActive || calcTimeDelta(nowMillis, midiScrollLastStep) < MIDI_SCROLL_STEP_MS) {
    return;
  }
  midiScrollLastStep = nowMillis;
  midiScrollCol -= 1;
  if (midiScrollCol < midiScrollEnd) {
    midiScrollActive = false;
    return;
  }
  if (!midiLightsAllowed()) return;
  drawMidiText(midiScrollCol, midiScrollRow, midiTextFontFor(midiScrollFont), midiScrollText, midiScrollColor, midiScrollLayer);
}

// col is the first pixel column and row the bottom pixel row, like the firmware's own screens;
// blank pixels inside the text's box are cleared, so redrawing one column over erases the old text
void drawMidiText(int col, int row, struct Font* font, const char* text, byte color, byte layer) {
  for (const char* t = text; *t; ++t) {
    const char* p = strchr(font->chars, *t);
    if (!p) continue;
    Character* ch = font->data[p - font->chars];
    if (col + ch->width - 1 >= 1 && col < NUMCOLS) {
      font_draw_char(col - 1, row, ch->data, color, ch->width, font->height, true, false, layer);
    }
    col += ch->width;
    if (col >= 1 && col < NUMCOLS) {
      for (byte r = row; r < row + font->height && r < NUMROWS; ++r) {
        setLed(col, r, COLOR_OFF, cellOff, layer);
      }
    }
    col += 1;
  }
}

// the digits of a version string, "234" is 234 and ".074" is 74
int versionNumber(const char* version) {
  int value = 0;
  for (const char* p = version; *p; ++p) {
    if (*p >= '0' && *p <= '9') {
      value = value * 10 + (*p - '0');
    }
  }
  return value;
}
