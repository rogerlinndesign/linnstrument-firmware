/******************************* ls_faders: LinnStrument CC Faders ********************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These functions handle the CC faders for each split
**************************************************************************************************/

#define CC_FADER_NUMBER_OFFSET 1

void handleFaderTouch(byte z, boolean newVelocity) {
  if (z && cell().velocity) {
    byte faderLeft, faderLength;
    determineFaderBoundaries(sensorSplit, faderLeft, faderLength);

    int value = -1;

    // when the fader only spans one cell, it acts as a toggle
    if (faderLength == 0) {
      if (newVelocity) {
        if (ccFaderValues[sensorSplit][sensorRow] > 0) {
          value = 0;
        }
        else {
          value = 127;
        }
      }
    }
    // otherwise it's a real fader and we calculate the value based on its position
    else {
      value = calculateFaderValue(cell().calibratedX(), faderLeft, faderLength);
    }

    if (value >= 0) {
      ccFaderValues[sensorSplit][sensorRow] = value;
      preSendControlChange(sensorSplit, sensorRow + CC_FADER_NUMBER_OFFSET, value);
      paintCCFaderDisplayRow(sensorSplit, sensorRow);
    }
  }
}

void determineFaderBoundaries(byte split, byte &faderLeft, byte &faderLength) {
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

byte calculateFaderValue(int x, byte faderLeft, byte faderLength) {
  int32_t fxdFaderRange = FXD_MUL(FXD_FROM_INT(faderLength), CALX_FULL_UNIT);
  int32_t fxdFaderPosition = FXD_FROM_INT(x) - Global.calRows[faderLeft][0].fxdReferenceX;
  int32_t fxdFaderRatio = FXD_DIV(fxdFaderPosition, fxdFaderRange);
  int32_t fxdFaderValue = FXD_MUL(FXD_FROM_INT(127), fxdFaderRatio);
  return constrain(FXD_TO_INT(fxdFaderValue), 0, 127);
}

int32_t fxdCalculateFaderPosition(byte value, byte faderLeft, byte faderLength) {
  int32_t fxdFaderRange = FXD_MUL(FXD_FROM_INT(faderLength), CALX_FULL_UNIT);
  int32_t fxdFaderRatio = FXD_DIV(FXD_FROM_INT(value ), FXD_FROM_INT( 127));
  int32_t fxdFaderPosition = FXD_MUL(fxdFaderRange, fxdFaderRatio) + Global.calRows[faderLeft][0].fxdReferenceX;
  return fxdFaderPosition;
}