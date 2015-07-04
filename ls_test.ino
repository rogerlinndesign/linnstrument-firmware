/************************** ls_test: LinnStrument debugging and testing ***************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
Assorted debug functions. 
**************************************************************************************************/


void debugPrint(int level, char* msg) {
  if (Device.serialMode && (debugLevel >= level)) {
    Serial.print(msg);
  }
}

void debugPrintln(int level, char* msg) {
  if (Device.serialMode && (debugLevel >= level)) {
    Serial.println(msg);
  }
}

void debugPrint(int level, int val) {
  if (Device.serialMode && (debugLevel >= level)) {
    Serial.print(val);
  }
}

void debugPrintln(int level, int val) {
  if (Device.serialMode && (debugLevel >= level)) {
    Serial.println(val);
  }
}

// displayXFrame:
// For debug, displays an entire frame of raw X values in the Arduino serial monitor. Values are collected during each full read of the touch surface.
void displayXFrame() {
  if (sensorCell().touched == touchedCell) {
    sensorCell().refreshX();
  }

  static unsigned long lastFrame = 0;
  unsigned long now = micros();
  if (sensorCol == 1 && sensorRow == 0 && calcTimeDelta(now, lastFrame) >= 500000) {
    lastFrame = now;

    Serial.println();
    for (byte x = 0; x < NUMCOLS; ++x) {
      Serial.print(x);
      Serial.print("\t");
    }
    Serial.println();
    for (byte y = NUMROWS; y > 0; --y) {
      for (byte x = 0; x < NUMCOLS; ++x) {
        if (cell(x, y-1).touched == touchedCell) {
          Serial.print(cell(x, y-1).currentRawX);
        }
        else {
          Serial.print("-");
        }
        Serial.print("\t");
      }
      Serial.println();
    }
  }
}

// displayYFrame:
// For debug, displays an entire frame of raw Y values in the Arduino serial monitor. Values are collected during each full read of the touch surface.
void displayYFrame() {
  if (sensorCell().touched == touchedCell) {
    sensorCell().refreshY();
  }

  static unsigned long lastFrame = 0;
  unsigned long now = micros();
  if (sensorCol == 1 && sensorRow == 0 && calcTimeDelta(now, lastFrame) >= 500000) {
    lastFrame = now;
    
    Serial.println();
    for (byte x = 0; x < NUMCOLS; ++x) {
      Serial.print(x);
      Serial.print("\t");
    }
    Serial.println();
    for (byte y = NUMROWS; y > 0; --y) {
      for (byte x = 0; x < NUMCOLS; ++x) {
        if (cell(x, y-1).touched == touchedCell) {
          Serial.print(cell(x, y-1).currentRawY);
        }
        else {
          Serial.print("-");
        }
        Serial.print("\t");
      }
      Serial.println();
    }
  }
}

// displayZFrame:
// For debug, displays an entire frame of raw Z values in the Arduino serial monitor. Values are collected during each full read of the touch surface.
void displayZFrame() {
  static unsigned long lastFrame = 0;
  unsigned long now = micros();
  if (sensorCol == 1 && sensorRow == 0 && calcTimeDelta(now, lastFrame) >= 500000) {
    lastFrame = now;
    
    Serial.println();
    for (byte x = 0; x < NUMCOLS; ++x) {
      Serial.print(x);
      Serial.print("\t");
    }
    Serial.println();
    for (byte y = NUMROWS; y > 0; --y) {
      for (byte x = 0; x < NUMCOLS; ++x) {
        Serial.print(cell(x, y-1).currentRawZ);
        Serial.print("\t");
      }
      Serial.println();
    }
  }
}

// For debug, displays an entire frame of raw Z values in the Arduino serial monitor. Values are collected during each full read of the touch surface.
void displaySurfaceScanTime() { 
  if (sensorCol == 1 && sensorRow == 0) {
    static int scanCount; 
    static unsigned long scanPeriod; 
    if (++scanCount > 255) { 
      Serial.print("Total surface scan time in microseconds: ");
      Serial.println((micros() - scanPeriod) / 256); 
      scanPeriod = micros(); 
      scanCount = 0;   
    }
  }
}

// displayCellTouchedFrame:
// For debug, displays an entire frame of raw Z values in the Arduino serial monitor. Values are collected during each full read of the touch surface.
void displayCellTouchedFrame() {
  Serial.println();
  for (byte x = 0; x < NUMCOLS; ++x) {
    Serial.print(x);
    Serial.print("\t");
  }
  Serial.println();
  for (byte y = NUMROWS; y > 0; --y) {
    for (byte x = 0; x < NUMCOLS; ++x) {
      Serial.print(cell(x, y-1).touched);
      Serial.print("\t");
    }
    Serial.println();
  }
}

void modeLoopManufacturingTest() {
  TouchState previousTouch = sensorCell().touched;
  sensorCell().refreshZ();

  // highlight the touches
  if (previousTouch != touchedCell && sensorCell().isMeaningfulTouch()) {
    cellTouched(touchedCell);

    if (sensorCol == 0) {
      byte color = COLOR_OFF;
      switch (sensorRow)
      {
        case 0:
        case 1:
          color = COLOR_YELLOW;
          break;
        case 2:
          color = COLOR_MAGENTA;
          break;
        case 3:
          color = COLOR_CYAN;
          break;
        case 4:
        case 5:
          color = COLOR_BLUE;
          break;
        case 6:
          color = COLOR_GREEN;
          break;
        case 7:
          color = COLOR_RED;
          break;
      }

      for (byte col = 0; col < NUMCOLS; ++col) {
        for (byte row = 0; row < NUMROWS; ++row) {
          setLed(col, row, color, cellOn);
        }
      }
    }
  }
  else if (previousTouch != untouchedCell && !sensorCell().isActiveTouch()) {
    cellTouched(untouchedCell);

    if (cellsTouched == 0) {
      for (byte col = 0; col < NUMCOLS; ++col) {
        for (byte row = 0; row < NUMROWS; ++row) {
          clearLed(col, row);
        }
      }
    }
  }

  if (sensorCol != 0 && sensorCell().touched != untouchedCell) {
    byte pressure = sensorCell().pressureZ;
    int pressureColumn = FXD_TO_INT(FXD_MUL(FXD_DIV(FXD_FROM_INT(pressure), FXD_CONST_127), FXD_FROM_INT(NUMCOLS-2))) + 1;
      
    for (byte c = 1; c < NUMCOLS; ++c) {
      if (c <= pressureColumn) {
        setLed(c, 0, COLOR_GREEN, cellOn);
      }
      else {
        clearLed(c, 0);
      }
    }
  }

  if (rowsInColsTouched[0] == 0) {
    lightLed(4, 1);
    lightLed(4, 6);
    lightLed(10, 1);
    lightLed(10, 6);
    lightLed(16, 1);
    lightLed(16, 6);
    lightLed(22, 1);
    lightLed(22, 6);
  }

  unsigned long now = micros();
  
  // send out MIDI activity
  midiOutQueue.push((byte)MIDIActiveSensing);
  handlePendingMidi(now);
  handleMidiInput(now);

  checkRefreshLedColumn(now);
  nextSensorCell();
}