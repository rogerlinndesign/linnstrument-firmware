/*********************************** ls_leds: LinnStrument LEDS ***********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
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

// Two buffers of ...
// A 26 by 8 byte array containing one byte for each LED:
// bits 4-6: 3 bits to select the color: 0:off, 1:red, 2:yellow, 3:green, 4:cyan, 5:blue, 6:magenta
// bits 0-2: 0:off, 1: on, 2: pulse
const unsigned long LED_LAYER_SIZE = MAXCOLS * MAXROWS;
const unsigned long LED_ARRAY_SIZE = (LED_LAYERS+1) * LED_LAYER_SIZE;
// array holding contents of display
byte leds[2][LED_ARRAY_SIZE];
byte visibleLeds = 0;
byte bufferedLeds = 0;
#define ledBuffered(layer, col, row)  leds[bufferedLeds][layer * LED_LAYER_SIZE + row * MAXCOLS + col]
#define ledVisible(layer, col, row)  leds[visibleLeds][layer * LED_LAYER_SIZE + row * MAXCOLS + col]

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
  byte layer = LED_LAYERS;
  do {
    layer -= 1;
    // don't show the custom layer 1 in user firmware mode
    if (userFirmwareActive) {
      if (layer == LED_LAYER_CUSTOM1) continue;
    }
    // don't show the custom layer 2 in regular firmware mode
    else {
      if (layer == LED_LAYER_CUSTOM2) continue;
    }
    if (!isVisibleSequencer()) {
      if (layer == LED_LAYER_SEQUENCER) continue;
    }
    // in normal display mode, show all layers and only show the main in other display modes
    if (displayMode == displayNormal || layer == LED_LAYER_MAIN) {
      data = ledBuffered(layer, col, row);
    }    
  }
  while(layer > 0 && (data & B00000111) == cellOff);

  return data;
}

void setLed(byte col, byte row, byte color, CellDisplay disp) {
  setLed(col, row, color, disp, LED_LAYER_MAIN);
}

void setLed(byte col, byte row, byte color, CellDisplay disp, byte layer) {
  if (col >= NUMCOLS || row >= NUMROWS) return;

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
    performContinuousTasks(micros());
  }
}

// light up a single LED with the default color
void lightLed(byte col, byte row) {
  setLed(col, row, globalColor, cellOn);
}

// clear a single LED
void clearLed(byte col, byte row) {
  clearLed(col, row, LED_LAYER_MAIN);
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

void completelyRefreshLeds() {
  for (byte row = 0; row < NUMROWS; ++row) {
    for (byte col = 0; col < NUMCOLS; ++col) {
      ledBuffered(LED_LAYER_COMBINED, col, row) = getCombinedLedData(col, row);
    }
    performContinuousTasks(micros());
  }
}

void clearDisplayImmediately() {
  // disable the outputs of the LED driver chips
  digitalWrite(37, HIGH);
}

// refreshLedColumn:
// Called when it's time to refresh the next column of LEDs. Internally increments the column number every time it's called.
void refreshLedColumn(unsigned long now) {
  // disabling the power output from the LED driver pins early prevents power leaking into unwanted cells.
  digitalWrite(37, HIGH);                                         // disable the outputs of the LED driver chips

  // keep a steady pulsating going for those leds that need it
  static unsigned long lastPulse = 0;
  static boolean lastPulseOn = true;
  static unsigned long lastSlowPulse = 0;
  static boolean lastSlowPulseOn = true;
  static unsigned long lastFocusPulse = 0;
  static boolean lastFocusPulseOn = true;

  if (calcTimeDelta(now, lastPulse) > 80000) {
    lastPulse = now;
    lastPulseOn = !lastPulseOn;
  }
  if (calcTimeDelta(now, lastSlowPulse) > 120000) {
    lastSlowPulse = now;
    lastSlowPulseOn = !lastSlowPulseOn;
  }
  if (calcTimeDelta(now, lastFocusPulse) > (lastFocusPulseOn ? 900000 : 100000)) {
    lastFocusPulse = now;
    lastFocusPulseOn = !lastFocusPulseOn;
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

  SPI.transfer16(SPI_LEDS, ((((uint16_t)~ledColShifted) <<8) | blue), SPI_CONTINUE);           // send column address
  SPI.transfer16(SPI_LEDS, (((uint16_t)green) << 8) | red);                    // send green byte
  
  digitalWrite(37, LOW);                                          // enable the outputs of the LED driver chips
}
