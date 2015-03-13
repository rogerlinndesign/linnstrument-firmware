/******************************* ls_faders: LinnStrument CC Faders ********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions handle the CC faders for each split
**************************************************************************************************/

#define CC_FADER_NUMBER_OFFSET 1

boolean hasTouchInSplitOnRow(byte split, byte row) {
  if (colsInRowsTouched[row]) {
    // if split is not active and there a touch on the row, it's obviously in the current split
    if (!splitActive) {
      return true;
    }

    // determine which columns need to be active in the touched row for this to be considered
    // part of either split
    if (split == LEFT && (colsInRowsTouched[row] & ((int32_t)(1 << Global.splitPoint) - 1))) {
      return true;
    }
    if (split == RIGHT && (colsInRowsTouched[row] & ~((int32_t)(1 << Global.splitPoint) - 1))) {
      return true;
    }
  }

  return false;
}

void handleFaderTouch(boolean newVelocity) {
  if (sensorCell().velocity) {
    unsigned short ccForFader = Split[sensorSplit].ccForFader[sensorRow];

    // only proceed when this is the touch on the highest row in the same split when the CC numbers
    // are the same, only one fader with the same cc number can be used at a time
    for (byte r = 7; r > sensorRow; --r) {
      if (Split[sensorSplit].ccForFader[r] == ccForFader && hasTouchInSplitOnRow(sensorSplit, r)) {
        return;
      }
    }

    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);

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
      if (newVelocity) {
        for (byte col = faderLength + faderLeft; col >= faderLeft; --col ) {
          if (col != sensorCol && cell(col, sensorRow).velocity) {
            transferFromSameRowCell(col);
            return;
          }
        }
      }

      // initialize the initial fader touch
      value = calculateFaderValue(sensorCell().calibratedX(), faderLeft, faderLength);
    }

    if (value >= 0) {
      ccFaderValues[sensorSplit][ccForFader] = value;
      preSendControlChange(sensorSplit, ccForFader, value);
      paintCCFaderDisplayRow(sensorSplit, sensorRow);
      // update other faders with the same CC number
      for (byte f = 0; f < 8; ++f) {
        if (f != sensorRow && Split[sensorSplit].ccForFader[f] == ccForFader) {
          performContinuousTasks(micros());
          paintCCFaderDisplayRow(sensorSplit, f);
        }
      }
    }
  }
}

void handleFaderRelease() {
  if (sensorCell().velocity) {
    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);
    if (faderLength > 0) {
      for (byte col = faderLength + faderLeft; col >= faderLeft; --col ) {
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
  faderLength = NUMCOLS - 2;
  if (splitActive || displayMode == displaySplitPoint) {
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
  int32_t fxdFaderRange = FXD_MUL(FXD_FROM_INT(faderLength), CALX_FULL_UNIT);
  int32_t fxdFaderPosition = FXD_FROM_INT(x) - Device.calRows[faderLeft][0].fxdReferenceX;
  int32_t fxdFaderRatio = FXD_DIV(fxdFaderPosition, fxdFaderRange);
  int32_t fxdFaderValue = FXD_MUL(FXD_FROM_INT(127), fxdFaderRatio);
  return constrain(FXD_TO_INT(fxdFaderValue), 0, 127);
}

int32_t fxdCalculateFaderPosition(byte value, byte faderLeft, byte faderLength) {
  int32_t fxdFaderRange = FXD_MUL(FXD_FROM_INT(faderLength), CALX_FULL_UNIT);
  int32_t fxdFaderRatio = FXD_DIV(FXD_FROM_INT(value), FXD_FROM_INT( 127));
  int32_t fxdFaderPosition = FXD_MUL(fxdFaderRange, fxdFaderRatio) + Device.calRows[faderLeft][0].fxdReferenceX;
  return fxdFaderPosition;
}