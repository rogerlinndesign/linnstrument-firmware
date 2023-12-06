/****************************** ls_settings: LinnStrument Settings ********************************
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
These functions allow for changing numeric values by sliding over the LinnStrument surface.
**************************************************************************************************/

short numericActiveColDown = 0;               // Number of columns currently held down, during numeric data changes
signed char numericDataChangeCol = -1;        // If -1, button has been pressed, but a starting column hasn't been set
signed char numericDataChangeColLast = -1;    // The last column that was pressed
unsigned long numericDataChangeColTime = 0;   // time of last touch for value change
unsigned long numericDataReleaseColTime = 0;  // time of last touch release

short numericActiveRowDown = 0;               // Number of rows currently held down, during numeric data changes
signed char numericDataChangeRow = -1;        // If -1, button has been pressed, but a starting row hasn't been set
signed char numericDataChangeRowLast = -1;    // The last row that was pressed
unsigned long numericDataChangeRowTime = 0;   // time of last touch for value change
unsigned long numericDataReleaseRowTime = 0;  // time of last touch release

void resetNumericDataChange() {
  resetNumericDataChangeCol();
  resetNumericDataChangeRow();
}

void resetNumericDataChangeCol() {
  numericDataChangeCol = -1;
  numericDataChangeColLast = -1;
  numericActiveColDown = 0;
  numericDataChangeColTime = 0;
  numericDataReleaseColTime = 0;
}

void resetNumericDataChangeRow() {
  numericDataChangeRow = -1;
  numericDataChangeRowLast = -1;
  numericActiveRowDown = 0;
  numericDataChangeRowTime = 0;
  numericDataReleaseRowTime = 0;
}

boolean handleNumericDataNewTouchCol(boolean &currentData) {
  unsigned short newData = handleNumericDataNewTouchColRaw(currentData, false, true, true);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchCol(unsigned short &currentData, unsigned short minimum, unsigned short maximum, boolean useFineChanges) {
  unsigned short newData = handleNumericDataNewTouchColRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchCol(byte &currentData, byte minimum, byte maximum, boolean useFineChanges) {
  byte newData = handleNumericDataNewTouchColRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchCol(char &currentData, char minimum, char maximum, boolean useFineChanges) {
  char newData = handleNumericDataNewTouchColRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchCol(signed char &currentData, signed char minimum, signed char maximum, boolean useFineChanges) {
  signed char newData = handleNumericDataNewTouchColRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchCol(short &currentData, short minimum, short maximum, boolean useFineChanges) {
  short newData = handleNumericDataNewTouchColRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

int handleNumericDataNewTouchColRaw(int currentData, int minimum, int maximum, boolean useFineChanges) {
  unsigned long now = micros();

  // If there are no columns down, reset so that things are
  // relative to the next new touch
  if (numericActiveColDown <= 0 && calcTimeDelta(now, numericDataReleaseColTime) > 200000) {
    resetNumericDataChangeCol();
  }

  // keep track of how many columns are currently down
  numericActiveColDown++;

  int newData = currentData;

  if (sensorCol != numericDataChangeColLast &&                     // only react when the column is actually different  
      calcTimeDelta(now, numericDataChangeRowTime) > 300000) {     // and prevent these too quickly after a vertical slide to make the interface feel more stable
    if (numericDataChangeCol < 0 ||
        (numericDataChangeCol < sensorCol && numericDataChangeColLast > sensorCol) ||
        (numericDataChangeCol > sensorCol && numericDataChangeColLast < sensorCol)) {
      // First cell hit after starting a data change or change of direction,
      // don't change data yet.
      numericDataChangeCol = sensorCol;
    }
    else {
      byte increment = 1;

      // If the swipe is fast, increment by a larger amount.
      if (calcTimeDelta(now, numericDataChangeColTime) < 40000) {
        increment = 20;
      }
      else if (calcTimeDelta(now, numericDataChangeColTime) < 70000) {
        increment = 4;
      }
      else if (calcTimeDelta(now, numericDataChangeColTime) < 120000) {
        increment = 2;
      }

      if (useFineChanges && abs(numericDataChangeCol - sensorCol) <= 3) {
        increment = 1;
      }

      if (sensorCol > numericDataChangeCol) {
        newData = constrain(currentData + increment, minimum, maximum);
      }
      else if (sensorCol < numericDataChangeCol) {
        newData = constrain(currentData - increment, minimum, maximum);
      }

      numericDataChangeColTime = now;

      // reset the vertical slide info when there's a horizontal change
      numericDataChangeRow = sensorRow;
      numericDataChangeRowLast = sensorRow;
    }
  }

  numericDataChangeColLast = sensorCol;

  return newData;
}

void handleNumericDataReleaseCol(boolean handleSplitSelection) {
  // The top row doesn't change the data, it only lets you change which side you're controlling
  if (handleSplitSelection && handleShowSplit()) {  // see if one of the "Show Split" cells have been hit
    return;
  }

  numericActiveColDown--;
  numericDataReleaseColTime = micros();
}

boolean handleNumericDataNewTouchRow(unsigned short &currentData, unsigned short minimum, unsigned short maximum) {
  unsigned short newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchRow(byte &currentData, byte minimum, byte maximum) {
  byte newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchRow(char &currentData, char minimum, char maximum) {
  char newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchRow(short &currentData, short minimum, short maximum) {
  short newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

int handleNumericDataNewTouchRowRaw(int currentData, int minimum, int maximum) {
  unsigned long now = micros();

  // If there are no rows down, reset so that things are
  // relative to the next new touch
  if (numericActiveRowDown <= 0 && calcTimeDelta(now, numericDataReleaseRowTime) > 200000) {
    resetNumericDataChangeRow();
  }

  // keep track of how many rows are currently down
  numericActiveRowDown++;

  int newData = currentData;

  if (sensorRow != numericDataChangeRowLast &&                     // only react when the row is actually different  
      calcTimeDelta(now, numericDataChangeColTime) > 300000) {     // and prevent these too quickly after a horizontal slide to make the interface feel more stable
    if (numericDataChangeRow < 0 ||
        (numericDataChangeRow < sensorRow && numericDataChangeRowLast > sensorRow) ||
        (numericDataChangeRow > sensorRow && numericDataChangeRowLast < sensorRow)) {
      // First cell hit after starting a data change or change of direction,
      // don't change data yet.
      numericDataChangeRow = sensorRow;
    }
    else {
      byte increment = 1;

      // If the swipe is fast, increment by a larger amount.
      if (calcTimeDelta(now, numericDataChangeRowTime) < 70000) {
        increment = 4;
      }
      else if (calcTimeDelta(now, numericDataChangeRowTime) < 120000) {
        increment = 2;
      }

      if (abs(numericDataChangeRow - sensorRow) <= 1) {
        increment = 0;
      }
      else if (abs(numericDataChangeRow - sensorRow) <= 3) {
        increment = 1;
      }

      if (sensorRow > numericDataChangeRow) {
        newData = constrain(currentData + increment, minimum, maximum);
      }
      else if (sensorRow < numericDataChangeRow) {
        newData = constrain(currentData - increment, minimum, maximum);
      }

      numericDataChangeRowTime = now;

      // reset the horizontal slide info when there's a vertical change
      numericDataChangeCol = sensorCol;
      numericDataChangeColLast = sensorCol;
    }
  }

  numericDataChangeRowLast = sensorRow;

  return newData;
}

void handleNumericDataReleaseRow(boolean handleSplitSelection) {
  // The top row doesn't change the data, it only lets you change which side you're controlling
  if (handleSplitSelection && handleShowSplit()) {  // see if one of the "Show Split" cells have been hit
    return;
  }

  numericActiveRowDown--;
  numericDataReleaseRowTime = micros();
}