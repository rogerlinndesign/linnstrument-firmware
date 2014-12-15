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



// leds[NUMCOLS][NUMROWS]:
// A 26 by 8 byte array containing one byte for each LED:
// bits 4-6: 3 bits to select the color: 0:off, 1:red, 2:yellow, 3:green, 4:cyan, 5:blue, 6:magenta
// bits 0-2: 0:off, 1-2: 2 brightness levels
byte leds[NUMCOLS][NUMROWS];             // array holding contents of display


// setLed:
// called to change the color (or brightness, not used) of a single LED in the 26 x 8 RGB LED array.
void setLed(byte col,                // Column of LED to be changed
            byte row,                // Row of LED to be changed
            byte color,              // Color of LED to be changed
            boolean on) {            // Should the LED be turned on or off
  if (!on) {
    color = COLOR_BLACK;
  }
  leds[col][row] = (color << 4) | on;      // packs color and brightness into this cell within array
}


// light up a single LED with the default color and brightness
void lightLed(byte col,              // Column of LED to be changed
              byte row) {            // Row of LED to be changed
  setLed(col, row, globalColor, true);
}

// clear a single LED
void clearLed(byte col,              // Column of LED to be changed
              byte row) {            // Row of LED to be changed
  setLed(col, row, COLOR_BLACK, false);
}



// refreshLedColumn:
// Called when it's time to refresh the next column of LEDs. Internally increments the column number every time it's called.
void refreshLedColumn() {                                 // output: none
  static byte ledCol = 0;                                 // current led column counter for refresh
  static byte actualCol;                                  // actual column being refreshed, permitting columns to be lit non-sequentially by using colIndex[] array
  static byte brightnessInterval = 0;                     // brightness setting. Always 0 but reserved for future expansion

  byte red = 0;                                           // red value to be sent
  byte green = 0;                                         // green value to be sent
  byte blue = 0;                                          // blue value to be sent
  byte ledColShifted = 0;                                 // LED column address, which is shifted 2 bits to left within byte

  if (++brightnessInterval >= 3) {                        // allow the user of several levels of brightness by modulating LED's ON time
    brightnessInterval = 0;
  }
  if (++ledCol >= NUMCOLS) ledCol = 0;

  actualCol = colIndex[ledCol];                           // using colIndex[], permits non-sequential lighting of LED columns, which doesn't seem to improve appearance

   // Initialize bytes to send to LEDs over SPI. Each bit represents a single LED on or off. Brightness is controlled by pulse width modulation.
  for (byte rowCount = 0; rowCount < NUMROWS; ++rowCount) {       // step through the 8 rows
    byte color = leds[actualCol][rowCount] >> 4;                  // set temp value 'color' to 4 color bits of this LED within array

    byte brightness = leds[actualCol][rowCount] & B00001111;      // get 'brightness' value

    if (!operatingLowPower && brightness ||
        operatingLowPower && brightness > brightnessInterval) {   // if this LED is not off, process it
      switch (color)                                              // set the color bytes to the correct color
      {
        case 0:  // off-- do nothing
          break;
        case 1:  // red
          red = red | (B00000001 << rowCount);                    // set this row's red bit on
          break;
        case 2:  // yellow
          red = red | (B00000001 << rowCount);                    // set this row's red and green bits on (yellow)
          green = green | (B00000001 << rowCount);
          break;
        case 3:  // green
          green = green | (B00000001 << rowCount);                // set this row's green bit on
          break;
        case 4:  // cyan
          green = green | (B00000001 << rowCount);                // set this row's green and blue bits on (cyan)
          blue = blue | (B00000001 << rowCount);
          break;
        case 5:  // blue
          blue = blue | (B00000001 << rowCount);                  // set this row's blue bit on
          break;
        case 6:  // magenta
          blue = blue | (B00000001 << rowCount);                  // set this row's blue and red bits on (magenta)
          red = red | (B00000001 << rowCount);
          break;
        case 7:  // white
          red = red | (B00000001 << rowCount);                    // set this row's red, green and blue bits on (white). Not used because current drain is too high. 
          green = green | (B00000001 << rowCount);
          blue = blue | (B00000001 << rowCount);
          break;
      }
    }
  }

  ledColShifted = actualCol << 2;
  if ((actualCol & 16) == 0) ledColShifted |= B10000000;          // if column address 4 is 0, set bit 7

  digitalWrite(37, HIGH);                                         // enable the outputs of the LED driver chips
  SPI.transfer(SPI_LEDS, ~ledColShifted, SPI_CONTINUE);           // send column address
  SPI.transfer(SPI_LEDS, blue, SPI_CONTINUE);                     // send blue byte
  SPI.transfer(SPI_LEDS, green, SPI_CONTINUE);                    // send green byte
  SPI.transfer(SPI_LEDS, red);                                    // send red byte
  digitalWrite(37, LOW);                                          // disable the outputs of the LED driver chips
}
