/************************** ls_test: LinnStrument debugging and testing ***************************
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
Assorted debug functions. 
**************************************************************************************************/


void debugPrint(int level, const char* msg) {
  if (Device.serialMode && (debugLevel >= level)) {
    Serial.print(msg);
  }
}

void debugPrintln(int level, const char* msg) {
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

void displayDigitalPins() {
  static unsigned long lastFrame = 0;
  unsigned long now = micros();
  if (sensorCol == 1 && sensorRow == 0 && calcTimeDelta(now, lastFrame) >= 500000) {
    lastFrame = now;

    Serial.println();
    for (byte p = 0; p < 54; ++p) {
      Serial.print(p);
      Serial.print("\t");
    }
    Serial.println();
    for (byte p = 0; p < 54; ++p) {
      Serial.print(digitalRead(p));
      Serial.print("\t");
    }
    Serial.println();
  }
}

// displayXFrame:
// For debug, displays an entire frame of raw X values in the Arduino serial monitor. Values are collected during each full read of the touch surface.
void displayXFrame() {
  if (sensorCell->touched == touchedCell) {
    sensorCell->refreshX();
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
  if (sensorCell->touched == touchedCell) {
    sensorCell->refreshY();
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
  TouchState previousTouch = sensorCell->touched;
  sensorCell->refreshZ();

  // highlight the touches
  if (previousTouch != touchedCell && sensorCell->isMeaningfulTouch()) {
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
  else if (previousTouch != untouchedCell && !sensorCell->isActiveTouch()) {
    cellTouched(untouchedCell);

    if (cellsTouched == 0) {
      for (byte col = 0; col < NUMCOLS; ++col) {
        for (byte row = 0; row < NUMROWS; ++row) {
          clearLed(col, row);
        }
      }
    }
  }

  if (sensorCol != 0 && sensorCell->touched != untouchedCell) {
    paintLowRowPressureBar();
  }

  if (rowsInColsTouched[0] == 0) {
    if (LINNMODEL == 200) {
      lightLed(4, 1);
      lightLed(4, 6);
      lightLed(10, 1);
      lightLed(10, 6);
      lightLed(16, 1);
      lightLed(16, 6);
      lightLed(22, 1);
      lightLed(22, 6);
    }
    else if (LINNMODEL == 128) {
      lightLed(2, 1);
      lightLed(2, 6);
      lightLed(6, 1);
      lightLed(6, 6);
      lightLed(11, 1);
      lightLed(11, 6);
      lightLed(15, 1);
      lightLed(15, 6);
    }
  }

  unsigned long now = micros();
  
  // send out MIDI activity
  midiOutQueue.push((byte)MIDIActiveSensing);
  handlePendingMidi(now);
  handleMidiInput(now);

  checkRefreshLedColumn(now);
  checkTimeToReadFootSwitches(now);
  nextSensorCell();
}

#ifdef DEBUG_ENABLED

#include <malloc.h>

extern char _end;
extern "C" char* sbrk(int i);
char* ramstart = (char*)0x20070000;
char* ramend = (char*)0x20088000;

void debugFreeRam() {
  static unsigned long lastFrame = 0;
  unsigned long now = micros();
  if (Device.serialMode && sensorCol == 1 && sensorRow == 0 && calcTimeDelta(now, lastFrame) >= 500000) {
    lastFrame = now;

    char* heapend = sbrk(0);
    register char* stack_ptr asm ("sp");
    struct mallinfo mi = mallinfo();
    Serial.print("RAM dynamic:");
    Serial.print(mi.uordblks);
    Serial.print(" static:");
    Serial.print(&_end - ramstart);
    Serial.print(" free:");
    Serial.println(stack_ptr - heapend + mi.fordblks);
  }
}

#endif
