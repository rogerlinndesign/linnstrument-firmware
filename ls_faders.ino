/******************************* ls_faders: LinnStrument CC Faders ********************************
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
These functions handle the CC faders for each split
**************************************************************************************************/

#define CC_FADER_NUMBER_OFFSET 1

void handleFaderTouch(boolean newVelocity) {
  if (sensorCell->velocity) {
    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);
    handleFaderTouch(newVelocity, faderLeft, faderLength);
  }
}

void handleFaderTouch(boolean newVelocity, byte faderLeft, byte faderLength) {
  if (sensorCell->velocity) {
    unsigned short ccForFader = Split[sensorSplit].ccForFader[sensorRow];
    if (ccForFader > 128) return;

    // only proceed when this is the touch on the highest row in the same split when the CC numbers
    // are the same, only one fader with the same cc number can be used at a time
    for (byte r = 7; r > sensorRow; --r) {
      if (Split[sensorSplit].ccForFader[r] == ccForFader && hasTouchInSplitOnRow(sensorSplit, r)) {
        return;
      }
    }

    short value = -1;

    // when the fader only spans one cell, it acts as a toggle
    if (faderLength == 0) {
      if (newVelocity) {
        if (ccFaderValues[sensorSplit][ccForFader] > 0) {
          value = 0;
        }
        else {
          value = 127;
        }
      }
    }
    // otherwise it's a real fader and we calculate the value based on its position
    else {
      // if a new touch happens on the same row that is further down the row, make it
      // take over the touch
      if (newVelocity) {
        for (byte col = faderLength + faderLeft; col >= faderLeft; --col) {
          if (col != sensorCol && cell(col, sensorRow).velocity) {
            transferFromSameRowCell(col);
            return;
          }
        }
      }

      // initialize the initial fader touch
      value = calculateFaderValue(sensorCell->calibratedX(), faderLeft, faderLength);
    }

    if (value >= 0) {
      ccFaderValues[sensorSplit][ccForFader] = value;
      preSendControlChange(sensorSplit, ccForFader, value, false);
      paintCCFaderDisplayRow(sensorSplit, sensorRow, faderLeft, faderLength);
      // update other faders with the same CC number
      for (byte f = 0; f < 8; ++f) {
        if (f != sensorRow && Split[sensorSplit].ccForFader[f] == ccForFader) {
          performContinuousTasks();
          paintCCFaderDisplayRow(sensorSplit, f, faderLeft, faderLength);
        }
      }
    }
  }
}

void handleFaderRelease() {
  byte faderLeft, faderLength;
  determineFaderBoundaries(sensorSplit, faderLeft, faderLength);
  handleFaderRelease(faderLeft, faderLength);
}

void handleFaderRelease(byte faderLeft, byte faderLength) {
  // if another touch is already down on the same row, make it take over the touch
  if (sensorCell->velocity) {
    if (faderLength > 0) {
      for (byte col = faderLength + faderLeft; col >= faderLeft; --col) {
        if (col != sensorCol && cell(col, sensorRow).touched == touchedCell) {
          transferToSameRowCell(col);
          break;
        }
      }
    }
  }
}

void determineFaderBoundaries(byte split, byte& faderLeft, byte& faderLength) {
  faderLeft = 1;
  faderLength = NUMCOLS-2;
  if (Global.splitActive || displayMode == displaySplitPoint) {
    if (split == LEFT) {
      faderLength = Global.splitPoint - 2;
    }
    else {
      faderLeft = Global.splitPoint;
      faderLength = NUMCOLS - Global.splitPoint - 1;
    }
  }
}

byte calculateFaderValue(short x, byte faderLeft, byte faderLength) {
  int32_t fxdFaderRange = FXD_MUL(FXD_FROM_INT(faderLength), FXD_CALX_FULL_UNIT);
  int32_t fxdFaderPosition = FXD_FROM_INT(x) - Device.calRows[faderLeft][0].fxdReferenceX;
  int32_t fxdFaderRatio = FXD_DIV(fxdFaderPosition, fxdFaderRange);
  int32_t fxdFaderValue = FXD_MUL(FXD_CONST_127, fxdFaderRatio);
  return constrain(FXD_TO_INT(fxdFaderValue), 0, 127);
}

int32_t fxdCalculateFaderPosition(byte value, byte faderLeft, byte faderLength) {
  return fxdCalculateFaderPosition(value, faderLeft, faderLength, FXD_CONST_127);
}

int32_t fxdCalculateFaderPosition(byte value, byte faderLeft, byte faderLength, int32_t fxdMaxValue) {
  int32_t fxdFaderRange = FXD_MUL(FXD_FROM_INT(faderLength), FXD_CALX_FULL_UNIT);
  int32_t fxdFaderRatio = FXD_DIV(FXD_FROM_INT(value), fxdMaxValue);
  int32_t fxdFaderPosition = FXD_MUL(fxdFaderRange, fxdFaderRatio) + Device.calRows[faderLeft][0].fxdReferenceX;
  return fxdFaderPosition;
}