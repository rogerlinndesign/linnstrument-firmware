/*********************************** ls_leds: LinnStrument LEDS ***********************************
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
These functions handle the low-level communication with LinnStrument's 208 RGB LEDs.
**************************************************************************************************/

/*
 LinnStrument contains an array of 208 RGB LEDs arranged in a 26 by 8 matrix.
 Only one column (8 LEDs) can be turned on at a time and the columns are refreshed one at a time.
 This works out to a duty cycle of 1/26, keeping the total current low enough for USB bus power. 

 These are the various functions that together perform the LED tasks:

 Data is sent to the LED board over a 32-bit SPI channel, arranged as follows:

 Byte 0 (column select):
     7:        6:        5:        4:        3:        2:        1:        0:
 colAdr4Inv colAdr4   colAdr3   colAdr2   colAdr1   colAdr0     n/a       n/a
 Note: colAdr4Inv is inverted state of colAdr4, to save an inverter

 Byte 1 (blue on/off bits for each row):
     7:        6:        5:        4:        3:        2:        1:        0:
  blueRow7  blueRow6  blueRow5  blueRow4  blueRow3  blueRow2  blueRow1  blueRow0

 Byte 2 (green on/off bits for each row):
     7:        6:        5:        4:        3:        2:        1:        0:
 greenRow7 greenRow6 greenRow5 greenRow4  greenRow3 greenRow2  greenRow1 greenRow0

 Byte 3 (red on/off bits for each row):
     7:        6:        5:        4:        3:        2:        1:        0:
  redRow7   redRow6   redRow5   redRow4   redRow3   redRow2   redRow1   redRow0
*/

byte COL_INDEX[MAXCOLS];
const byte COL_INDEX_200[MAXCOLS] = {0, 1, 6, 11, 16, 21, 2, 7, 12, 17, 22, 3, 8, 13, 18, 23, 4, 9, 14, 19, 24, 5, 10, 15, 20, 25};
const byte COL_INDEX_128[MAXCOLS] = {0, 1, 6, 11, 16, 2, 7, 12, 3, 8, 13, 4, 9, 14, 5, 10, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// array holding contents of display
byte leds[2][LED_ARRAY_SIZE];
byte visibleLeds = 0;
byte bufferedLeds = 0;
#define ledBuffered(layer, col, row)  leds[bufferedLeds][layer * LED_LAYER_SIZE + row * MAXCOLS + col]
#define ledVisible(layer, col, row)  leds[visibleLeds][layer * LED_LAYER_SIZE + row * MAXCOLS + col]

bool ledDisplayEnabled = true;

void initializeLeds() {
  if (LINNMODEL == 200) {
    for (byte i = 0; i < MAXCOLS; ++i) {
      COL_INDEX[i] = COL_INDEX_200[i];
    }
  }
  else if (LINNMODEL == 128) {
    for (byte i = 0; i < MAXCOLS; ++i) {
      COL_INDEX[i] = COL_INDEX_128[i];
    }
  }
}

void initializeLedLayers() {
  memset(leds[bufferedLeds], 0, LED_ARRAY_SIZE);
}

void initializeLedsLayer(byte layer) {
  memset(&leds[bufferedLeds][layer * LED_LAYER_SIZE], 0, LED_LAYER_SIZE);
}

int getActiveCustomLedPattern() {
  return Global.activeNotes - 9;
}

void loadCustomLedLayer(int pattern)
{
  if (pattern < 0 || pattern >= LED_PATTERNS) {
    if (customLedPatternActive) {
      memset(&leds[0][LED_LAYER_CUSTOM1 * LED_LAYER_SIZE], 0, LED_LAYER_SIZE);
      memset(&leds[1][LED_LAYER_CUSTOM1 * LED_LAYER_SIZE], 0, LED_LAYER_SIZE);
    }
    customLedPatternActive = false;
    return;
  }

  memcpy(&leds[0][LED_LAYER_CUSTOM1 * LED_LAYER_SIZE], &Device.customLeds[pattern][0], LED_LAYER_SIZE);
  memcpy(&leds[1][LED_LAYER_CUSTOM1 * LED_LAYER_SIZE], &Device.customLeds[pattern][0], LED_LAYER_SIZE);
  customLedPatternActive = true;
  lightSettings = 2;
  completelyRefreshLeds();
}

void storeCustomLedLayer(int pattern)
{
  if (pattern < 0 || pattern >= LED_PATTERNS) {
    if (customLedPatternActive) {
      memset(&leds[0][LED_LAYER_CUSTOM1 * LED_LAYER_SIZE], 0, LED_LAYER_SIZE);
      memset(&leds[1][LED_LAYER_CUSTOM1 * LED_LAYER_SIZE], 0, LED_LAYER_SIZE);
    }
    customLedPatternActive = false;
    return;
  }

  memcpy(&Device.customLeds[pattern][0], &leds[visibleLeds][LED_LAYER_CUSTOM1 * LED_LAYER_SIZE], LED_LAYER_SIZE);
  customLedPatternActive = true;
  lightSettings = 2;
}

void clearStoredCustomLedLayer(int pattern)
{
  if (pattern < 0 || pattern >= LED_PATTERNS) return;

  memset(&Device.customLeds[pattern][0], 0, LED_LAYER_SIZE);
  if (getActiveCustomLedPattern() == pattern) {
    loadCustomLedLayer(pattern);
  }
}

void startBufferedLeds() {
  bufferedLeds = 1;
  memcpy(leds[bufferedLeds], leds[visibleLeds], LED_ARRAY_SIZE);
}

void finishBufferedLeds() {
  memcpy(leds[visibleLeds], leds[bufferedLeds], LED_ARRAY_SIZE);
  bufferedLeds = 0;
}

inline byte getCombinedLedData(byte col, byte row) {
  byte data = 0;
  byte layer = MAX_LED_LAYERS;
  do {
    layer -= 1;
    // when custom LED editor is active, only display those LEDs
    if (col > 0 && displayMode == displayCustomLedsEditor) {
      if (layer != LED_LAYER_CUSTOM1) continue;
      data = ledBuffered(layer, col, row);
    }
    // don't show the custom layer 1 in user firmware mode
    if (userFirmwareActive || isVisibleSequencer()) {
      if (layer == LED_LAYER_CUSTOM1) continue;
    }
    // don't show the custom layer 2 in regular firmware mode
    if (!userFirmwareActive) {
      if (layer == LED_LAYER_CUSTOM2) continue;
    }
    if (!isVisibleSequencer()) {
      if (layer == LED_LAYER_SEQUENCER) continue;
    }
    // in normal and split point display mode, show all layers and only show the main in other display modes
    if (displayMode == displayNormal || displayMode == displaySplitPoint || layer == LED_LAYER_MAIN) {
      data = ledBuffered(layer, col, row);
    }    
  }
  while (layer > 0 && (data & B00000111) == cellOff);

  return data;
}

void setLed(byte col, byte row, byte color, CellDisplay disp) {
  setLed(col, row, color, disp, LED_LAYER_MAIN);
}

void setLed(byte col, byte row, byte color, CellDisplay disp, byte layer) {
  if (col >= NUMCOLS || row >= NUMROWS || layer > MAX_LED_LAYERS) return;

  if (color == COLOR_OFF) {
    disp = cellOff;
  }
  else if (disp == cellOff) {
    color = COLOR_OFF;
  }
  // packs color and display into this cell within array
  byte data = ((color & B00011111) << 3) | (disp & B00000111);
  if (ledBuffered(layer, col, row) != data) {
    ledBuffered(layer, col, row) = data;
    ledBuffered(LED_LAYER_COMBINED, col, row) = getCombinedLedData(col, row);
  }

  if (bufferedLeds == 1) {
    performContinuousTasks();
  }
}

byte getLedColor(byte col, byte row, byte layer) {
  if (col >= NUMCOLS || row >= NUMROWS || layer > MAX_LED_LAYERS) return COLOR_OFF;
  return (ledVisible(layer, col, row) >> 3) & B00011111;
}

// light up a single LED with the default color
void lightLed(byte col, byte row) {
  setLed(col, row, globalColor, cellOn);
}

// clear a single LED
void clearLed(byte col, byte row) {
  clearLed(col, row, LED_LAYER_MAIN);
  clearLed(col, row, LED_LAYER_LOWROW);
}

void clearLed(byte col, byte row, byte layer) {
  setLed(col, row, COLOR_OFF, cellOff, layer);
}

// Turns all LEDs off
void clearFullDisplay() {
  clearSwitches();
  clearDisplay();
}

// Turns all LEDs off in columns 1 or higher
void clearDisplay() {
  for (byte col = 1; col < NUMCOLS; ++col) {
    clearColumn(col);
  }
}

// Turns all LEDs off in column 0
void clearSwitches() {
  clearColumn(0);
}

void clearColumn(byte col) {
  for (byte row = 0; row < NUMROWS; ++row) {
    clearLed(col, row);
  }
}

void clearRow(byte row) {
  for (byte col = 0; col < NUMCOLS; ++col) {
    clearLed(col, row);
  }
}

void completelyRefreshLeds() {
  for (byte row = 0; row < NUMROWS; ++row) {
    for (byte col = 0; col < NUMCOLS; ++col) {
      ledBuffered(LED_LAYER_COMBINED, col, row) = getCombinedLedData(col, row);
    }
    performContinuousTasks();
  }
}

void clearDisplayImmediately() {
  // disable the outputs of the LED driver chips
  digitalWrite(37, HIGH);
}

void disableLedDisplay() {
  ledDisplayEnabled = false;
  clearDisplayImmediately();
}

void enableLedDisplay() {
  // enable the outputs of the LED driver chips
  digitalWrite(37, LOW);
  ledDisplayEnabled = true;
}

// refreshLedColumn:
// Called when it's time to refresh the next column of LEDs. Internally increments the column number every time it's called.
void refreshLedColumn(unsigned long now) {
  if (!ledDisplayEnabled) return;

  // disabling the power output from the LED driver pins early prevents power leaking into unwanted cells.
  digitalWrite(37, HIGH);                                         // disable the outputs of the LED driver chips

  // keep a steady pulsating going for those leds that need it
  static unsigned long lastPulse = 0;
  static boolean lastPulseOn = true;
  static unsigned long lastSlowPulse = 0;
  static boolean lastSlowPulseOn = true;
  static boolean lastFocusPulseOn = true;

  if (calcTimeDelta(now, lastPulse) > 80000) {
    lastPulse = now;
    lastPulseOn = !lastPulseOn;
  }
  if (calcTimeDelta(now, lastSlowPulse) > 120000) {
    lastSlowPulse = now;
    lastSlowPulseOn = !lastSlowPulseOn;
  }
  if (clock24PPQ < 6) {
    lastFocusPulseOn = false;
  }
  else {
    lastFocusPulseOn = true;
  }

  static byte ledCol = 0;
  static byte displayInterval[MAXCOLS][MAXROWS];

  byte red = 0;                                           // red value to be sent
  byte green = 0;                                         // green value to be sent
  byte blue = 0;                                          // blue value to be sent
  byte actualCol = 0;                                     // actual column being refreshed, permitting columns to be lit non-sequentially by using COL_INDEX[] array
  byte ledColShifted = 0;                                 // LED column address, which is shifted 2 bits to left within byte

  actualCol = COL_INDEX[ledCol];                           // using COL_INDEX[], permits non-sequential lighting of LED columns, which doesn't seem to improve appearance

   // Initialize bytes to send to LEDs over SPI. Each bit represents a single LED on or off
  for (byte rowCount = 0; rowCount < NUMROWS; ++rowCount) {       // step through the 8 rows
    // allow several levels of brightness by modulating LED's ON time
    if (++displayInterval[actualCol][rowCount] >= 12) {
      displayInterval[actualCol][rowCount] = 0;
    }

    byte color = (ledVisible(LED_LAYER_COMBINED, actualCol, rowCount) & B11111000) >> 3;    // set temp value 'color' to 4 color bits of this LED within array
    byte cellDisplay = ledVisible(LED_LAYER_COMBINED, actualCol, rowCount) & B00000111;     // get cell display value

    switch (cellDisplay) {
      case cellFastPulse:
        cellDisplay = lastPulseOn ? cellOn : cellOff;
        break;
      case cellSlowPulse:
        cellDisplay = lastSlowPulseOn ? cellOn : cellOff;
        break;
      case cellFocusPulse:
        cellDisplay = lastFocusPulseOn ? cellOn : cellOff;
        break;
    }

    if (Device.operatingLowPower) {
      if (displayInterval[actualCol][rowCount] % 2 != 0) {
        cellDisplay = cellOff;
      }
    }

    // if this LED is not off, process it
    // set the color bytes to the correct color
    if (cellDisplay) {
      // construct composite colors
      if ((!Device.operatingLowPower && displayInterval[actualCol][rowCount] % 2 != 0) ||
          (Device.operatingLowPower && displayInterval[actualCol][rowCount] % 4 != 0)) {
        switch (color)
        {
          case COLOR_WHITE:
            color = COLOR_CYAN;
            break;
          case COLOR_ORANGE:
            color = COLOR_YELLOW;
            break;
          case COLOR_LIME:
            color = COLOR_GREEN;
            break;
          case COLOR_PINK:
            color = COLOR_YELLOW;
            break;
        }
      }

      switch (color)
      {
        case COLOR_OFF:
        case COLOR_BLACK:
          break;
        case COLOR_RED:
          red = red | (B00000001 << rowCount);
          break;
        case COLOR_YELLOW:
          red = red | (B00000001 << rowCount);
          green = green | (B00000001 << rowCount);
          break;
        case COLOR_GREEN:
          green = green | (B00000001 << rowCount);
          break;
        case COLOR_CYAN:
          green = green | (B00000001 << rowCount);
          blue = blue | (B00000001 << rowCount);
          break;
        case COLOR_BLUE:
          blue = blue | (B00000001 << rowCount);
          break;
        case COLOR_MAGENTA:
          blue = blue | (B00000001 << rowCount);
          red = red | (B00000001 << rowCount);
          break;
        case COLOR_WHITE:
          blue = blue | (B00000001 << rowCount);
          red = red | (B00000001 << rowCount);
          green = green | (B00000001 << rowCount);
          break;
        case COLOR_ORANGE:
          red = red | (B00000001 << rowCount);
          break;
        case COLOR_LIME:
          red = red | (B00000001 << rowCount);
          green = green | (B00000001 << rowCount);
          break;
        case COLOR_PINK:
          blue = blue | (B00000001 << rowCount);
          red = red | (B00000001 << rowCount);
          break;
      }
    }
  }

  if (++ledCol >= NUMCOLS) ledCol = 0;

  ledColShifted = actualCol << 2;
  if ((actualCol & 16) == 0) ledColShifted |= B10000000;          // if column address 4 is 0, set bit 7

  SPI.transfer(SPI_LEDS, ~ledColShifted, SPI_CONTINUE);           // send column address
  SPI.transfer(SPI_LEDS, blue, SPI_CONTINUE);                     // send blue byte
  SPI.transfer(SPI_LEDS, green, SPI_CONTINUE);                    // send green byte
  SPI.transfer(SPI_LEDS, red);                                    // send red byte
  digitalWrite(37, LOW);                                          // enable the outputs of the LED driver chips
}
