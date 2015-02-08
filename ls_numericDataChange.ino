/****************************** ls_settings: LinnStrument Settings ********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions allow for changing numeric values by sliding over the LinnStrument surface.
**************************************************************************************************/

short numericActiveColDown = 0;              // Number of columns currently held down, during numeric data changes
signed char numericDataChangeCol = -1;       // If -1, button has been pressed, but a starting column hasn't been set
signed char numericDataChangeColLast = -1;   // The last column that was pressed
unsigned long numericDataChangeColTime = 0;  // time of last touch for value change

short numericActiveRowDown = 0;              // Number of rows currently held down, during numeric data changes
signed char numericDataChangeRow = -1;       // If -1, button has been pressed, but a starting row hasn't been set
signed char numericDataChangeRowLast = -1;   // The last row that was pressed
unsigned long numericDataChangeRowTime = 0;  // time of last touch for value change

void resetNumericDataChange() {
  resetNumericDataChangeCol();
  resetNumericDataChangeRow();
}

void resetNumericDataChangeCol() {
  numericDataChangeCol = -1;
  numericDataChangeColLast = -1;
  numericActiveColDown = 0;
}

void resetNumericDataChangeRow() {
  numericDataChangeRow = -1;
  numericDataChangeRowLast = -1;
  numericActiveRowDown = 0;
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
  if (numericActiveColDown <= 0 && calcTimeDelta(now, numericDataChangeColTime) > 500000) {
    resetNumericDataChangeCol();
  }

  // keep track of how many columns are currently down
  numericActiveColDown++;

  int newData = currentData;

  if (sensorCol != numericDataChangeColLast &&                     // only react when the column is actually different  
      calcTimeDelta(now, numericDataChangeRowTime) > 300000) {     // and prevent these too quickly after a vertical slide to make the interface feel more stable
    byte increment = 1;

    // If the swipe is fast, increment by a larger amount.
    if (calcTimeDelta(now, numericDataChangeColTime) < 70000) {
      increment = 4;
    }
    else if (calcTimeDelta(now, numericDataChangeColTime) < 120000) {
      increment = 2;
    }

    if (numericDataChangeCol < 0 ||
        (numericDataChangeCol < sensorCol && numericDataChangeColLast > sensorCol) ||
        (numericDataChangeCol > sensorCol && numericDataChangeColLast < sensorCol)) {
      // First cell hit after starting a data change or change of direction,
      // don't change data yet.
      numericDataChangeCol = sensorCol;
      numericDataChangeColLast = sensorCol;
    }
    else {
      if (useFineChanges && abs(numericDataChangeCol - sensorCol) <= 3) {
        increment = 1;
      }

      if (sensorCol > numericDataChangeCol) {
        newData = constrain(currentData + increment, minimum, maximum);
      } else if (sensorCol < numericDataChangeCol) {
        newData = constrain(currentData - increment, minimum, maximum);
      }

      numericDataChangeColTime = now;
    }

    numericDataChangeColLast = sensorCol;
  }

  return newData;
}

void handleNumericDataReleaseCol(boolean handleSplitSelection) {
  // The top row doesn't change the data, it only lets you change which side you're controlling
  if (handleSplitSelection && handleShowSplit()) {  // see if one of the "Show Split" cells have been hit
    return;
  }

  numericActiveColDown--;
}

boolean handleNumericDataNewTouchRow(unsigned short &currentData, unsigned short minimum, unsigned short maximum, boolean useFineChanges) {
  unsigned short newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchRow(byte &currentData, byte minimum, byte maximum, boolean useFineChanges) {
  byte newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchRow(char &currentData, char minimum, char maximum, boolean useFineChanges) {
  char newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

boolean handleNumericDataNewTouchRow(short &currentData, short minimum, short maximum, boolean useFineChanges) {
  short newData = handleNumericDataNewTouchRowRaw(currentData, minimum, maximum, useFineChanges);
  if (newData != currentData) {
    currentData = newData;
    updateDisplay();
    return true;
  }

  return false;
}

int handleNumericDataNewTouchRowRaw(int currentData, int minimum, int maximum, boolean useFineChanges) {
  unsigned long now = micros();

  // If there are no rows down, reset so that things are
  // relative to the next new touch
  if (numericActiveRowDown <= 0 && calcTimeDelta(now, numericDataChangeRowTime) > 500000) {
    resetNumericDataChangeRow();
  }

  // keep track of how many rows are currently down
  numericActiveRowDown++;

  int newData = currentData;

  if (sensorRow != numericDataChangeRowLast &&                     // only react when the row is actually different  
      calcTimeDelta(now, numericDataChangeColTime) > 300000) {     // and prevent these too quickly after a horizontal slide to make the interface feel more stable
    byte increment = 1;

    // If the swipe is fast, increment by a larger amount.
    if (calcTimeDelta(now, numericDataChangeRowTime) < 70000) {
      increment = 4;
    }
    else if (calcTimeDelta(now, numericDataChangeRowTime) < 120000) {
      increment = 2;
    }

    if (numericDataChangeRow < 0 ||
        (numericDataChangeRow < sensorRow && numericDataChangeRowLast > sensorRow) ||
        (numericDataChangeRow > sensorRow && numericDataChangeRowLast < sensorRow)) {
      // First cell hit after starting a data change or change of direction,
      // don't change data yet.
      numericDataChangeRow = sensorRow;
      numericDataChangeRowLast = sensorRow;
    }
    else {
      if (useFineChanges && abs(numericDataChangeRow - sensorRow) <= 3) {
        increment = 1;
      }

      if (sensorRow > numericDataChangeRow) {
        newData = constrain(currentData + increment, minimum, maximum);
      } else if (sensorRow < numericDataChangeRow) {
        newData = constrain(currentData - increment, minimum, maximum);
      }

      numericDataChangeRowTime = now;
    }

    numericDataChangeRowLast = sensorRow;
  }

  return newData;
}

void handleNumericDataReleaseRow(boolean handleSplitSelection) {
  // The top row doesn't change the data, it only lets you change which side you're controlling
  if (handleSplitSelection && handleShowSplit()) {  // see if one of the "Show Split" cells have been hit
    return;
  }

  numericActiveRowDown--;
}